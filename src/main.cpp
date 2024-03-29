#include <Arduino.h>
#include <Keyboard.h>
#include <Bounce2.h>
#include <Metro.h>
#include <functional>
#include <Adafruit_DotStar.h>
#include "config_types.hpp"

/**
 * BEGIN CONFIGURATION
*/

#define SUPPORT_RAT_DRIVER // comment out this line if you want to disable host driver support.

// stick -> motion curves you'll reference in your StickModes. Tweak these and make more if you want.
constexpr auto panCurve = make_curve(25); // maximum movement speed, linear curve
constexpr auto orbitCurve = make_curve(10, 0.5); // max speed with slight "expo" factor
constexpr auto zoomScrollCurve = make_curve(1400, 0.6); // note that scroll speed is on a different order of magnitude, also major expo

// set up the list of stick modes for each axis.
// note the fancy named initializer list thingie. Note that even if there are gaps, fields must be referenced
// in the order they're defined. Check `config_types.hpp` for details.
constexpr auto stickModes = declare_mode_map(
  std::array { // stick 0
    StickMode { MovementMode::REWIND, panCurve,  .indicator = rgb(50, 0, 0), {false, true, false} },
    StickMode { MovementMode::STUTTER, panCurve, .indicator = rgb(0, 50, 0), {false, true, false}, .motionThreshold = 25 },
    StickMode { MovementMode::STUTTER, panCurve, .indicator = rgb(0, 0, 50), {false, false, true} }
  },
  std::array { // stick 1
    StickMode { MovementMode::SCROLL, zoomScrollCurve, .indicator = rgb(50, 0, 0), .horDir = NULL_AXIS},
    StickMode { MovementMode::REWIND, orbitCurve,      .indicator = rgb(0, 50, 0), {false, true, false}, KEY_LEFT_SHIFT },
    StickMode { MovementMode::REWIND, orbitCurve,      .indicator = rgb(0, 0, 50), {true, false, false}}
  }
);

#ifdef SUPPORT_RAT_DRIVER
// when the driver is in use, these stick modes define the motion behavior for dynamic modes.
// the actual buttons clicked/held will be adjusted via messages from the driver.
// If you're not using the driver, you can leave these alone.
auto dynamicModes = std::array {
  StickMode { MovementMode::STUTTER, panCurve}, // dynamic pan
  StickMode { MovementMode::REWIND, orbitCurve}, // dynamic orbit
  StickMode { MovementMode::SCROLL, zoomScrollCurve, .horDir = NULL_AXIS} // dynamic zoom
};
#endif 

// global mouse/stick behavior options

constexpr float deadzone = 0.08; // absolute normalized axis value must be above this to be considered active
constexpr int max_unwind_step = 100; // how many pixels per HID report to move the mouse during unwinding
constexpr int stutter_step = 1000; // how far to move the cursor in stutter mode before stuttering back.
constexpr int button_debounce_interval = 25; // how many ms any button needs to be held for to register.
constexpr int mouse_interval = 10; // minimum interval between mouse motion updates, in ms. (not recommended to change)
// note that changing the mouse_interval will affect all mouse and scrolling speeds

// Key macros.

constexpr uint32_t macroColors[] = {
  rgb(0x42, 0xFF, 0x00),
  rgb(0x42, 0xFF, 0x00),
  rgb(0x42, 0xFF, 0x00),
  rgb(50, 0, 50),
  rgb(50, 0, 50),
  rgb(50, 0, 50)
};

using MacroFunction = std::function<void(bool)>;
const std::array<MacroFunction, 6> buttonMacros = {
  [](bool pressed) {
    if (pressed) {
      Mouse.press(MOUSE_RIGHT);
    }
    else {
      Mouse.release(MOUSE_RIGHT);
    }
  },

  [](bool pressed) {
    if (pressed) {
      Mouse.press(MOUSE_MIDDLE);
    }
    else {
      Mouse.release(MOUSE_MIDDLE);
    }
  },

  [](bool pressed) {
    if (pressed) {
      Mouse.press(MOUSE_LEFT);
    }
    else {
      Mouse.release(MOUSE_LEFT);
    }
  },

  [](bool pressed) {
    if (pressed) {
      Keyboard.press(KEY_LEFT_SHIFT);
    }
    else {
      Keyboard.release(KEY_LEFT_SHIFT);
    }
  },
  
  [](bool pressed) {
    if (pressed) {
      Keyboard.press(KEY_LEFT_ALT);
    }
    else {
      Keyboard.release(KEY_LEFT_ALT);
    }
  },

  [](bool pressed) {
    if (pressed) {
      Keyboard.press(KEY_LEFT_CTRL);
    }
    else {
      Keyboard.release(KEY_LEFT_CTRL);
    }
  }
};


// firmware behavior.

#ifdef SUPPORT_RAT_DRIVER
constexpr bool support_driver = true; // turn on firmware support for the `rat_tail` driver program
#else
constexpr bool support_driver = false;
#endif

/* calibration options */
constexpr bool center_on_startup = true; // record startup reading from sticks as center value?
constexpr bool autocal = true; // attempt to automatically calibrate during use.

/* HID joystick options */
constexpr bool send_joystick_hid = true; // (also) send HID joystick records for the sticks and buttons?
constexpr int joystick_interval = 10; // minimum interval between joystick HID reports, in ms.

/* USB serial options */
constexpr bool serial_status_reports = false; // (also) output structured serial reports
constexpr bool serial_raw_axes = false; // report raw axis values
constexpr bool serial_norm_axes = false; // report normalized axis values
constexpr bool serial_buttons = true; // report button presses
constexpr int serial_interval = 100; // minimum interval between serial axis reports, in ms
/**
 * Serial reports are of the formats:
 * AXES a0 a1 a2 ...   // raw axis values, from 0 to 1023.
 * NORM a0 a1 a2 ...   // normalized axis values, from -100 to +100.
 * BP index             // `index` button was pressed
 * BR index             // `index` button was released
*/

/* HARDWARE CONFIGURATION */

constexpr int macroKeyCount = 6; // number of macro keys.

/**
 * Axis, stick, and button ordering:
 * 
 * Sticks are made up of consecutive pairs of axes. First the X axis for that stick, then the Y axis.
 * So stick 0 is made of axis 0 (x) and axis 1 (y), stick 1 is axis 2 (x) and axis 3 (y), etc.
 * 
 * The first two buttons are the integrated buttons for stick 0 and stick 1 respectively.
**/

constexpr int n_axes = 4; // number of axes we're going to sample. Every pair of axes is a "stick".

// calibration: low value, center, high value.
// note that this is intentionally not `constexpr` to permit live calibration.
int16_t axisExtents[][3] = {
  {10, 520, 1015},
  {10, 498, 1015},
  {10, 530, 1015},
  {10, 513, 1015}
};

// for each axis, set inversion mode. This should be set up so that the joystick reports
// match the physical stick directions. Do not use this configuration value to control
// the direction of mouse motion relative to the stick--use .vertDir and .horDir in
// your StickMode to control the mouse motion inversion.
// Basically, use these inversions to compensate for how you've mounted/rotated
// your sticks.
constexpr int8_t axisInvert[] = {
  INVERT_AXIS, INVERT_AXIS, INVERT_AXIS, INVERT_AXIS
};

// stuff below here requires different hardware than OrbitRat is designed for. It's useful
// if you're trying to hack this software to support new input devices or
// if you built your rat differently than I do.

// pin assignments

// Which pin goes to which axis? These are Teensy ANALOG pin numbers.
constexpr uint8_t axisPins[] = {
  // 1, 0, 8, 7    // breadboard rat
  7, 8, 0, 1       // pcb rat
};

// Which LED indices go with the sticks?
constexpr uint8_t stickLEDs[] = {
  0, 2
};

constexpr int n_buttons = 2 + macroKeyCount; // number of buttons

// which pins are buttons attached to? These are Teensy DIGITAL pin numbers.
// first two pins listed should be for the buttons _on_ stick 0 and stick 1.
// the rest are for the macro buttons.
// something neat! since this is constexpr, and `n_buttons` is also const
// the compiler seems to know that the unused values aren't referenced and
// doesn't keep them in the table if you have the macros disabled.
constexpr uint8_t buttonPins[] = { 
  //16, 23, 0       // breadboard rat
  23, 16,      // pcb rat
  4, 5, 6, 7, 8, 9 // macro rat
};

// Which LED index is the first of the macro keys?
constexpr uint8_t buttonLEDMap[] = {
  0, 2, 
  3, 4, 5, 6, 7, 8
};

/**
 * END CONFIGURATION
*/



// raw state sampled from analog inputs
int16_t axisValues[n_axes] = {0, 0, 0, 0};

// button debouncers
std::array<Bounce, n_buttons> buttons {};

// button state
std::array<bool, n_buttons> buttonState {};

// button colors before they're turned off during press.
std::array<uint32_t, n_buttons> buttonColorCache {};

// axis values in the range (-1, 1)
std::array<float, n_axes> normalizedAxes{};

// LEDs
Adafruit_DotStar ledStrip(9, 11, 13, DOTSTAR_BRG);

// forward declaration
void doUnwind(int unwindAccumulator[2]);

template <typename T>
constexpr bool eitherMagAbove(T vec[2], T threshold){
  return abs(vec[0]) >= threshold || abs(vec[1]) >= threshold;
}

// sample a curve from a stick position and return the value.
StickCurve::value_type sampleExpoCurve(StickCurve const& curve, float pos) {
  int point = abs(pos * (curve.size() - 1));
  auto sign = pos >= 0 ? 1 : -1;
  return curve[point] * sign;
}


// primary stick handler class
struct StickState {
  const int index; // which stick is this?
  const int xAxis, yAxis; // which axes are we using?

  int activeStickMode = 0; // which mode in the mode map are we using now?

  int moveAccum[2] = {0, 0}; // accumulates unsent motion, used to smooth motion and reduce errant clicks
  int scrollAccum[2] = {0, 0}; // accumulates scrolling motion, which is converted into distinct mousewheel clicks
  int unwindAccum[2] = {0, 0}; // accumulates _sent_ motion, used to unwind the cursor position.
  
  bool stickActive = false; // is the stick currently in control?
  bool buttonsActivated = false; // are we currently holding down mousebuttons and keys?

  StickState(int index) : index(index), xAxis(index * 2), yAxis(index * 2 + 1) {}

  // advance the active mode by one, looping around at the end.button
  void advanceActiveMode() {
    activeStickMode = (activeStickMode + 1) % stickModes.count(index);
    updateStickLED();
  }

  void updateStickLED() {
    ledStrip.setPixelColor(stickLEDs[index], mode().indicator);  
  }

  float x() const { return normalizedAxes[xAxis]; }
  float y() const { return normalizedAxes[yAxis]; }


  // Checks whether the stick is in the deadzone.
  bool checkDeadzone() {
    return abs(x()) < deadzone && abs(y()) < deadzone;
  }

  // gets the current mode
  StickMode const& mode() {
    return stickModes.getMode(index, activeStickMode);
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
    stickActive = true;
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

    stickActive = false;
  }

  // stutters the mouse back to continue an active move.
  void stutterBack() {
    endActiveMotion();
    //delay(25);
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
    acc[0] += sampleExpoCurve(curve, x()) * mode().horDir;
    acc[1] += sampleExpoCurve(curve, y()) * mode().vertDir;
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

  // the scroll accumulator has to get this magnitude to trigger a scroll wheel click
  static constexpr int scroll_saturate = 10000;

  // Updates the given directional scroll, and returns +/-1 to indicate the mouse wheel should click.
  int getDirectionScroll(int index) {
    auto sv = scrollAccum[index];
    auto sign = sv >= 0 ? 1 : -1;

    if (abs(sv) >= scroll_saturate) {
      scrollAccum[index] -= sign * scroll_saturate;
      return sign;
    }
    return 0;
  }

  // update scrolling based on stick axes.
  void moveScrollMotion(bool firstMove) {
    accumulateMotion(scrollAccum);
    if (firstMove) { // force a move on the first step
      for (int & s : scrollAccum) {
        if (s < 0) {
          s += -scroll_saturate;
        }
        else if (s > 0) {
          s += scroll_saturate;
        }
      }
    }
    int scrollX = getDirectionScroll(0), scrollY = getDirectionScroll(1);
    if (scrollX || scrollY) {
      Mouse.scroll(scrollY, scrollX);
    }
  }

  // integrated update function that handles all movement modes.
  void moveActiveMotion(bool firstMove = false) {
    switch (mode().move){
      case MovementMode::SCROLL:
        moveScrollMotion(firstMove);
        break;
      default:
       moveMouseMotion();
    }
    //delay(10);
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
    if (axisValues[i] < axisExtents[i][1]) { // which half of the stick are we on?
      if (axisValues[i] < axisExtents[i][0]) { // are we oversaturated?
        if constexpr (autocal) { // if we're in autocal mode, change the limit
          axisExtents[i][0] = axisValues[i];
        }
        else {
          axisValues[i] = axisExtents[i][0]; // if we're not in autocal, just clamp it.
        }
      }

      normalizedAxes[i] = -1 + normalize(axisExtents[i][0], axisValues[i], axisExtents[i][1]);
    }
    else {  
      if (axisValues[i] > axisExtents[i][2]) { // oversatured?
        if constexpr (autocal) {
          axisExtents[i][2] = axisValues[i];
        }
        else {
          axisValues[i] = axisExtents[i][2];
        }
      }
      
      normalizedAxes[i] = normalize(axisExtents[i][1], axisValues[i], axisExtents[i][2]);
    }

    normalizedAxes[i] = axisInvert[i] * normalizedAxes[i]; // invert all channels to get normalized motion.
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
  if (activeStick) { // are we in an active move?
    if (activeStick->checkDeadzone()) { // did it stop?
      activeStick->deactivate();
      activeStick = nullptr;
    }
    else { // still active, so keep moving.
      activeStick->moveActiveMotion(false);
    }
  }
  else {
    for (StickState & stick : sticks) { // check for the first stick out of its deadzone
      if (!stick.checkDeadzone()) {
        activeStick = &stick;
        activeStick->activate(); // bust a move.
        activeStick->moveActiveMotion(true);
        return;
      }
    }
  }
}

// typedef for a button activation callback.
using button_func = std::function<void(int, bool)>;

// button does nothing.
void null_button(int, bool) {}

void macro_button(int button, bool pressed) {
  const auto macro = button - 2;
  buttonMacros[macro](pressed);
}

// basic button callback that simply advances the active stick mode.
void advance_mode(int button, bool pressed) {
  if (!pressed) return; // only fire on press-down

  if (sticks[button].stickActive) return; // don't change the mode on the active stick.
  sticks[button].advanceActiveMode();
};

const button_func button_clicked[] = {
  advance_mode, // stick buttons
  advance_mode,

  macro_button,
  macro_button,
  macro_button,
  macro_button,
  macro_button,
  macro_button
};

// update buttons and call callbacks when they're pressed.
void updateButtons() {
  for (int i = 0; i < n_buttons; i++) {
    buttons[i].update();

    if (buttons[i].fell()) {
      if constexpr (serial_status_reports && serial_buttons) {
        Serial.print("BP ");
        Serial.println(i);
      }

      buttonState[i] = true;
      button_clicked[i](i, true);

      buttonColorCache[i] = ledStrip.getPixelColor(buttonLEDMap[i]);
      ledStrip.setPixelColor(buttonLEDMap[i], 0);
    }
    else if (buttons[i].rose()) {
      if constexpr (serial_status_reports && serial_buttons) {
        Serial.print("BR ");
        Serial.println(i);
      }

      ledStrip.setPixelColor(buttonLEDMap[i], buttonColorCache[i]);

      buttonState[i] = false;
      button_clicked[i](i, false);
    }
  }
}

/**
 * ARDUINO HOOKS
*/

void setup() {
  if constexpr (serial_status_reports) {
    Serial.begin(38400);
  }

  // set up button with the Bounce library.
  for (int i = 0; i < n_buttons; i++) {
    buttons[i].attach(buttonPins[i], INPUT_PULLUP);
    buttons[i].interval(button_debounce_interval);
  }

  if constexpr (center_on_startup) {
    // calibrate stick centers.
    readSticks();
    for (int i = 0; i < n_axes; i++) {
      axisExtents[i][1] = axisValues[i];
    }
  }

  pinMode(LED_BUILTIN, OUTPUT);

  // initialize RGB LEDs
  ledStrip.begin();
  ledStrip.fill(0x50, 0, 9);
  ledStrip.setBrightness(20);

  sticks[0].updateStickLED();
  sticks[1].updateStickLED();

  for (int i = 0; i < macroKeyCount; i++) {
    ledStrip.setPixelColor(3 + i, macroColors[i]);
  }

  ledStrip.show();
}

Metro joystickMetro(joystick_interval);
Metro mouseMetro(mouse_interval);
Metro serialMetro(serial_interval);

void loop() {
  digitalWrite(LED_BUILTIN, false);
  readSticks();
  updateButtons();
  normalizeSticks();

  #ifdef SUPPORT_RAT_DRIVER
  uint8_t hidbuf[64];
  if (RawHID.recv(hidbuf, 0)) {
    Serial.println((char *) hidbuf);
  }
  #endif

  if constexpr (send_joystick_hid) {
    if (joystickMetro.check()) {
      sendJoystick();
      joystickMetro.reset();
    }
  }

  if (mouseMetro.check()) {
    sendMouse();
    mouseMetro.reset();
  }

  if constexpr (serial_status_reports) {
    if (serialMetro.check()) {

      if constexpr (serial_raw_axes) {
        Serial.print("AXES ");
        for (auto const& a : axisValues) {
          Serial.print(a);
          Serial.print(" ");
        }
        Serial.print("\n");
      }
      
      if constexpr (serial_norm_axes) {
        Serial.print("NORM ");
        for (auto const& a : normalizedAxes) {
          Serial.print((int) (a * 100));
          Serial.print(" ");
        }
        Serial.print("\n");
        serialMetro.reset();
      }
    }
  }

  ledStrip.show();
  digitalWrite(LED_BUILTIN, true);
}

