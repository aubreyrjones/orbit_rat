#include <Arduino.h>
#include <Keyboard.h>

constexpr int n_axes = 4;
constexpr int max_mouse_speed = 25;
constexpr int orbit_mouse_speed = 10;

float axisExtents[][3] = {
  {3, 520, 1021},
  {6, 498, 1019},
  {3, 530, 1021},
  {2, 513, 1022}
};

int axisValues[n_axes] = {0, 0, 0, 0};
bool buttons[2] = {false, false};

float normalizedAxes[n_axes] = {0, 0, 0, 0};

void readSticks() {
  axisValues[0] = analogRead(1);
  axisValues[1] = analogRead(0);

  axisValues[2] = analogRead(8);
  axisValues[3] = analogRead(7);
}

float normalize(float low, float val, float high) {
  auto range = high - low;
  return (val - low) / range;
}

unsigned int to_joy(float val) {
  return (unsigned int) ((val + 1.f) / 2.f * 1023);
}


void normalizeSticks() {
  for (int i = 0; i < n_axes; i++) {
    if (axisValues[i] < axisExtents[i][1]) {
      normalizedAxes[i] = -(1 - normalize(axisExtents[i][0], axisValues[i], axisExtents[i][1]));
    }
    else {
      normalizedAxes[i] = normalize(axisExtents[i][1], axisValues[i], axisExtents[i][2]);
    }
    normalizedAxes[i] = -normalizedAxes[i];
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(38400);

}

void sendJoystick() {
  Joystick.X(to_joy(normalizedAxes[0]));
  Joystick.Y(to_joy(normalizedAxes[1]));
  Joystick.sliderLeft(to_joy(normalizedAxes[3]));
  Joystick.Zrotate(to_joy(normalizedAxes[2]));
}

int unwindAccumulator[2];
bool inMove = false;

int max_step(int const& accum) {
  if (accum > 0) {
    return -min(accum, 100);
  }
  else if (accum < 0) {
    return min(100, -accum);
  }

  return 0;
}

void doUnwind() {
  while (unwindAccumulator[0] != 0 || unwindAccumulator[1] != 0) {
    int xMove = max_step(unwindAccumulator[0]);
    int yMove = max_step(unwindAccumulator[1]);

    Mouse.move(xMove, yMove);
    unwindAccumulator[0] += xMove;
    unwindAccumulator[1] += yMove;
  }
}

int motionStartIndex = -1;

bool checkDeadzone(int startIndex) {
  return abs(normalizedAxes[startIndex]) < 0.10 && abs(normalizedAxes[startIndex + 1]) < 0.10;
}

void sendMouse() {

  if (inMove && checkDeadzone(motionStartIndex)) {
    Mouse.set_buttons(0, 0, 0);
    if (motionStartIndex == 2) {
      Keyboard.release(KEY_LEFT_SHIFT);
    }
    doUnwind();
    inMove = false;
    return;    
  }

  if (!checkDeadzone(0)) {
    motionStartIndex = 0;
  }
  else if (!checkDeadzone(2)) {
    motionStartIndex = 2;
  }
  else { 
    return;
  }

  if (!inMove) {
    unwindAccumulator[0] = unwindAccumulator[1] = 0;
    inMove = true;
    if (motionStartIndex == 2) {
      Keyboard.press(KEY_LEFT_SHIFT);
    }
    Mouse.set_buttons(0, 1, 0);
  }

  auto speed = motionStartIndex == 0 ? max_mouse_speed : orbit_mouse_speed;

  int xMove = speed * normalizedAxes[motionStartIndex];
  int yMove = speed * normalizedAxes[motionStartIndex + 1];

  Mouse.move(xMove, yMove);

  unwindAccumulator[0] += xMove;
  unwindAccumulator[1] += yMove;
}

void loop() {
  // put your main code here, to run repeatedly:
  
  readSticks();
  normalizeSticks();

  sendJoystick();
  sendMouse();

  delay(10);

  // Serial.print("axes ");
  // for (int i = 0; i < n_axes; i++) {
  //   Serial.print(normalizedAxes[i]);
  //   Serial.print(",");
  // }
  // Serial.print("\n");
  // delay(80);
}