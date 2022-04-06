#include <Arduino.h>
#include <Keyboard.h>
#include <Bounce2.h>
#include <functional>

constexpr bool calibrate_on_startup = true; // record startup reading from sticks as center value?
constexpr bool send_joystick_hid = false; // (also) send HID joystick records for the sticks and buttons?
constexpr int n_axes = 4; // number of axes we're going to sample.
constexpr int n_buttons = 2; // number of buttons
constexpr int pan_speed = -25; // max speed of pan motion (first stick). Negative to invert motion.
constexpr int orbit_speed = -10; // max speed of orbit motion (second stick)
constexpr float deadzone = 0.02; // absolute normalized axis value must be above this to be considered active
constexpr int max_unwind_step = 100; // how many pixels per HID report to move the mouse during unwinding
constexpr int stutter_step = 1000;
constexpr int button_debounce_interval = 25; 

// Which pin goes to which axis? These are Teensy ANALOG pin numbers.
// pan stick horizontal, pan stick vertical, orbit stick horizontal, orbit stick vertical
constexpr int axisPins[] = {
  1, 0, 8, 7
};

// which pins are stick-press buttons attached to? These are Teensy DIGITAL pin numbers.
constexpr int buttonPins[] = {
  16, 23
};

// calibration: low value, center, high value.
// note that this is intentionally not `constexpr` to permit calibration.
float axisExtents[][3] = {
  {10, 520, 1000},
  {10, 498, 1000},
  {10, 530, 1000},
  {10, 513, 1000}
};


enum class MovementMode {
  REWIND, // do a motion then unwind the cursor position
  STUTTER, // continuously move, then unwind after a certain threshold, then continue
  SIMPLE, // don't unwind at all
  CHASE, // move, then activate keyboard command, continuously.
  SCROLL // translate stick movements into vertical and horizontal scroll
};

constexpr float exp_entry(float expCoef, float x) {
  return pow(x, 2.71828 * expCoef);
}

using ExpoTable = std::array<float, 40>;

constexpr ExpoTable makeExpoTable(float expCoef) {
  ExpoTable retval = {};
  float step = 1.0f / (retval.size() - 1);
  for (unsigned int i = 0; i < retval.size(); i++) {
    retval[i] = exp_entry(expCoef, i * step);
  }
  return retval;
}

constexpr ExpoTable linearCurve = makeExpoTable(0.4);

float sampleExpoCurve(ExpoTable const& curve, float pos) {
  int point = abs(pos * (curve.size() - 1));
  auto sign = pos >= 0 ? 1 : -1;
  Serial.print(point);
  Serial.print(",");
  Serial.println(curve[point] * sign);
  return curve[point] * sign;
}

struct StickMode {
  MovementMode move; // which movement mode?
  int speedHorizontal; // how fast to move the mouse or scroll wheel?
  int speedVertical;
  bool activeButtons[3]; // which buttons to press? left, middle, right
  int activeKey = 0; // which key to hold down during mouse motion

  ExpoTable const& curve = linearCurve;

  int chaseKey = 0; // key to press after each motion step to "chase"
  int chaseMods = 0; // modifiers to press along with the chase key.

  float motionThreshold = 1; // how many pixels of movement are required to trigger mouse motion?


  bool hasButtons() const {
    return activeButtons[0] || activeButtons[1] || activeButtons[2];
  }

  bool hasKey() const {
    return activeKey;
  }
};

constexpr int n_stick_modes = 2; // how many modes for each stick?

// Configure the modes for each stick.
StickMode modeMap[n_axes / 2][n_stick_modes] = {
  {
    StickMode { MovementMode::REWIND, pan_speed, pan_speed, {false, true, false}, 0 },
    StickMode { MovementMode::STUTTER, pan_speed, pan_speed, {false, true, false}, 0, linearCurve, 0, 0, 25 },
    //StickMode { MovementMode::CHASE, -pan_speed, {false, true, false}, 0, KEY_M, MODIFIERKEY_LEFT_ALT | MODIFIERKEY_LEFT_SHIFT }
  },
  {
    StickMode { MovementMode::SCROLL, 0, -100, {false, false, false}},
    StickMode { MovementMode::REWIND, orbit_speed, orbit_speed, {false, true, false}, KEY_LEFT_SHIFT },
    //StickMode { MovementMode::REWIND, pan_speed, {false, true, false}, KEY_LEFT_SHIFT },
  }
};

// raw state sampled from analog inputs
int axisValues[n_axes] = {0, 0, 0, 0};

// button debouncers
Bounce buttons[n_buttons] = {Bounce(), Bounce()};

bool buttonState[n_buttons] = {false, false};

// axis values in the range (-1, 1)
float normalizedAxes[n_axes] = {0, 0, 0, 0};


// forward declaration
void doUnwind(int unwindAccumulator[2]);

template <typename T>
constexpr bool eitherMagAbove(T vec[2], T threshold){
  return abs(vec[0]) >= threshold || abs(vec[1]) >= threshold;
}

// primary stick handler class
struct StickState {
  const int index; // which stick is this?
  const int xAxis, yAxis; // which axes are we using?

  int activeStickMode = 0; // which mode in the mode map are we using now?

  float moveAccum[2] = {0, 0}; // accumulates unsent motion, used to smooth motion and reduce errant clicks
  int scrollAccum[2] = {0, 0}; // accumulates scrolling motion, which is converted into distinct mousewheel clicks
  int unwindAccum[2] = {0, 0}; // accumulates _sent_ motion, used to unwind the cursor position.
  
  bool buttonsActivated = false; // are we currently holding down mousebuttons and keys?

  StickState(int index) : index(index), xAxis(index * 2), yAxis(index * 2 + 1) {}

  float x() const { return normalizedAxes[xAxis]; }
  float y() const { return normalizedAxes[yAxis]; }


  // Checks whether the stick is in the deadzone.
  bool checkDeadzone() {
    return abs(x()) < deadzone && abs(y()) < deadzone;
  }

  // gets the current mode
  StickMode const& mode() {
    return modeMap[index][activeStickMode];
  };

  // clears all the motion accumulators.
  void clearMotion() {
    unwindAccum[0] = unwindAccum[1] = 0;
    moveAccum[0] = moveAccum[1] = 0;
    scrollAccum[0] = scrollAccum[1] = 0;
  }

  // presses down buttons and keys prior to active motion
  void precedeActiveMotion(){
    if (buttonsActivated) return;

    setKeys(true);
    if (mode().hasButtons()) {
      Mouse.set_buttons(mode().activeButtons[0], mode().activeButtons[1], mode().activeButtons[2]);
      delay(10);
    }
    buttonsActivated = true;
  }

  // releases buttons and keys after active motion
  void endActiveMotion() {
    if (!buttonsActivated) return;

    if (mode().hasButtons()) {
      Mouse.set_buttons(0, 0, 0);
      delay(10);
    }
    setKeys(false);
    buttonsActivated = false;
  }

  // call this when this stick is active and controlling the cursor.
  void activate() {
    clearMotion();
  }

  // call this when the stick is done.
  void deactivate() {
    endActiveMotion();
    switch (mode().move) {
      case MovementMode::REWIND:
      case MovementMode::STUTTER:
        unwindMotion();
        break;
      default:
        break;
    }
  }

  // stutters the mouse back to continue an active move.
  void stutterBack() {
    delay(25);
    endActiveMotion();
    unwindMotion();
  }

  // send the mouse cursor motion if it's above the motionThreshold.
  void sendMotion() {
    if (eitherMagAbove(moveAccum, mode().motionThreshold)) {
      precedeActiveMotion();
      int xMove = (int) moveAccum[0];
      int yMove = (int) moveAccum[1];

      Mouse.move(xMove, yMove);
      unwindAccum[0] += xMove;
      unwindAccum[1] += yMove;
      moveAccum[0] -= xMove;
      moveAccum[1] -= yMove;
    }
  }

  // integrate stick and speed into the given accumulator.
  template <typename T>
  void accumulateMotion(T acc[2]) {
    auto const& curve = mode().curve;
    acc[0] += sampleExpoCurve(curve, x()) * mode().speedHorizontal;
    acc[1] += sampleExpoCurve(curve, y()) * mode().speedVertical;
  }

  // update mouse motion from the sticks axes.
  void moveMouseMotion() {
    accumulateMotion(moveAccum);

    sendMotion();

    if (mode().move == MovementMode::STUTTER) {
      if (eitherMagAbove(unwindAccum, stutter_step)) {
        stutterBack();
      }
    }
    else if (mode().move == MovementMode::CHASE) {
      if (eitherMagAbove(unwindAccum, 25)) {
        Keyboard.set_modifier(mode().chaseMods);
        Keyboard.set_key1(mode().chaseKey);
        Keyboard.send_now();

        Keyboard.set_key1(0);
        Keyboard.set_modifier(0);
        Keyboard.send_now();
        
        stutterBack();
      }
    }
  }

  // Updates the given directional scroll, and returns +/-1 to indicate the mouse wheel should click.
  int getDirectionScroll(int index) {
    auto sv = scrollAccum[index];
    auto sign = sv >= 0 ? 1 : -1;

    if (abs(sv) >= 1000) {
      scrollAccum[index] -= sign * 1000;
      return sign;
    }
    return 0;
  }

  // update scrolling based on stick axes.
  void moveScrollMotion() {
    accumulateMotion(scrollAccum);
    int scrollX = getDirectionScroll(0), scrollY = getDirectionScroll(1);
    if (scrollX || scrollY) {
      Mouse.scroll(scrollY, scrollX);
    }
  }

  // integrated update function that handles all movement modes.
  void moveActiveMotion() {
    switch (mode().move){
      case MovementMode::SCROLL:
        moveScrollMotion();
        break;
      default:
       moveMouseMotion();
    }
  }

  // unwind sent cursor motion to return the cursor to its start position
  void unwindMotion() {
    doUnwind(unwindAccum);
  }

  // hold down the mode's keys.
  void setKeys(bool press) {
    auto key = mode().activeKey;
    
    if (!key) return;

    if (press) {
      Keyboard.press(key);
    }
    else {
      Keyboard.release(key);
    }

    delay(10);
  }
};

StickState sticks[n_axes / 2] = {
  StickState(0),
  StickState(1)
};

// read the analog sticks.
void readSticks() {
  for (int i = 0; i < n_axes; i++) {
    axisValues[i] = analogRead(axisPins[i]);
  }
}

// normalize the value within the half space.
float normalize(float low, float val, float high) {
  auto range = high - low;
  return (val - low) / range;
}

// convert the normalized stick values to the 0-1023 values for the joystick system.
unsigned int to_joy(float val) {
  return (unsigned int) ((val + 1.f) / 2.f * 1023);
}

// Normalize the raw values read from the analog sticks.
void normalizeSticks() {
  for (int i = 0; i < n_axes; i++) {
    if (axisValues[i] < axisExtents[i][1]) {
      if (axisValues[i] < axisExtents[i][0]) {
        axisValues[i] = axisExtents[i][0];
      }
      normalizedAxes[i] = -(1 - normalize(axisExtents[i][0], axisValues[i], axisExtents[i][1]));
    }
    else {
      if (axisValues[i] > axisExtents[i][2]) {
        axisValues[i] = axisExtents[i][2];
      }
      normalizedAxes[i] = normalize(axisExtents[i][1], axisValues[i], axisExtents[i][2]);
    }
    normalizedAxes[i] = -normalizedAxes[i]; // invert all channels to get normalized motion.
  }
}

// Do arduino setup.
void setup() {
  Serial.begin(38400);

  // set up button with the Bounce library.
  for (int i = 0; i < n_buttons; i++) {
    buttons[i].attach(buttonPins[i], INPUT_PULLUP);
    buttons[i].interval(button_debounce_interval);
  }

  if /*constexpr*/ (calibrate_on_startup) {
    // calibrate stick centers.
    readSticks();
    for (int i = 0; i < n_axes; i++) {
      axisExtents[i][1] = axisValues[i];
    }
  }
}

// Send joystick updates.
void sendJoystick() {
  Joystick.X(to_joy(normalizedAxes[0]));
  Joystick.Y(to_joy(normalizedAxes[1]));
  Joystick.sliderLeft(to_joy(normalizedAxes[3]));
  Joystick.Zrotate(to_joy(normalizedAxes[2]));
}

// When we unwind the mouse motion, find the largest step we can move
// to reduce the given accumulator. This is necessary because the
// teensy HID system only spec's mouse moves between -127 and 127.
int max_step(int const& accum) {
  if (accum > 0) {
    return -min(accum, max_unwind_step);
  }
  else if (accum < 0) {
    return min(max_unwind_step, -accum);
  }

  return 0;
}

// Move the mouse back to its start point.
void doUnwind(int unwindAccumulator[2]) {
  while (unwindAccumulator[0] != 0 || unwindAccumulator[1] != 0) {
    int xMove = max_step(unwindAccumulator[0]);
    int yMove = max_step(unwindAccumulator[1]);

    Mouse.move(xMove, yMove);
    unwindAccumulator[0] += xMove;
    unwindAccumulator[1] += yMove;
  }
}

StickState *activeStick = nullptr;

// Update mouse motion state and send HID reports.
void sendMouse() {

  // are we in a move that has now stopped?
  if (activeStick && activeStick->checkDeadzone()) {
    activeStick->deactivate();
    activeStick = nullptr;
    return;    
  }

  if (!activeStick) {
    for (StickState & stick : sticks) {
      if (!stick.checkDeadzone()) {
        activeStick = &stick;
        goto start_move;
      }
    }
    return; // skipped by goto if we have a live stick
    
    start_move:
    activeStick->activate();
  }

  activeStick->moveActiveMotion();
}

// typedef for a button activation callback.
using button_func = std::function<void(int)>;

// basic button callback that simply advances the active stick mode.
void advance_mode(int button) {
  //Serial.print("clicked "); Serial.println(button);
  sticks[button].activeStickMode = (sticks[button].activeStickMode + 1) % n_stick_modes;
};

button_func button_clicked[] = {
  advance_mode,
  advance_mode
};

// update buttons and call callbacks when they're pressed.
void updateButtons() {
  for (int i = 0; i < n_buttons; i++) {
    buttons[i].update();

    if (buttons[i].fell()) {
      buttonState[i] = true;
      button_clicked[i](i);
    }
    else if (buttons[i].rose() && buttonState[i]) {
      buttonState[i] = false;
    }
  }
}

void loop() {
  readSticks();
  normalizeSticks();
  updateButtons();

  if constexpr (send_joystick_hid) { 
    sendJoystick();
  }

  sendMouse();

  delay(10); // approximately 100 updates a second. It's actually less because of delays elsewhere, but it's plenty fast for CAD or whatever.

  // temporarily uncomment this block to get values printed out to serial for calibration purposes.
  // Serial.print("axes ");
  // for (int i = 0; i < n_axes; i++) {
  //   Serial.print(axisValues[i]);
  //   Serial.print(",");
  // }
  // Serial.print("\n");
  // delay(80);
}