#include <Arduino.h>
#include <Keyboard.h>

constexpr int n_axes = 4;

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
  axisValues[0] = analogRead(0);
  axisValues[1] = analogRead(1);

  axisValues[2] = analogRead(7);
  axisValues[3] = analogRead(8);
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
  Joystick.X(to_joy(normalizedAxes[1]));
  Joystick.Y(to_joy(normalizedAxes[0]));
  Joystick.sliderLeft(to_joy(normalizedAxes[2]));
  Joystick.Zrotate(to_joy(normalizedAxes[3]));
}

void loop() {
  // put your main code here, to run repeatedly:
  
  readSticks();
  normalizeSticks();

  sendJoystick();

  delay(10);

  // Serial.print("axes ");
  // for (int i = 0; i < n_axes; i++) {
  //   Serial.print(normalizedAxes[i]);
  //   Serial.print(",");
  // }
  // Serial.print("\n");
  // delay(80);
}