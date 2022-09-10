//for Arduino
//rc car lights

#include "headlights.cpp"
#include "turns.cpp"
#include "emergency_lights.cpp"
#include "backfire.cpp"
#include "break_reverse.cpp"
#include "blinker.h"
#include "low_voltage_detector.cpp"


bool Debug = false;

int pinCh1 = 2;
int pinCh2 = 3;
int pinCh3 = 4;
int pinVoltageMetter = 2;

int pinExhaust = 8;
int pinLights1 = 9; //this should be pwm pin
int pinLights2 = 12; //this should be pwm pin
int pinLeft = 5;
int pinRight = 6;
int pinReverse = 7;
int pinBreak = 11;

Blinker turnsBlinker = Blinker();
Blinker lowVoltageBlinker = Blinker(200, 1000);

void HLights2Toggle(bool isTurnedOn) {
  int brightness = 200;
  if (isTurnedOn) {
    analogWrite(pinLights2, brightness - 50);
    delay(50);
    analogWrite(pinLights2, 0);
    delay(50);
    analogWrite(pinLights2, brightness - 50);
    delay(50);
    analogWrite(pinLights2, 0);
    delay(50);
    analogWrite(pinLights2, brightness);
  } else {
    for (int i = brightness; i >= 0; i--) {
      analogWrite(pinLights2, i);
      delay(20);
    }
  }
}

void HLights1Toggle(bool isTurnedOn) {
  int brightness = 150;
  if (isTurnedOn) {
    for (int i = 0; i <= brightness; i++) {
      analogWrite(pinLights1, i);
      delay(2);
    }
  } else {
    for (int i = brightness; i >= 0; i--) {
      analogWrite(pinLights1, i);
      delay(2);
    }
  }
}

void OnReverse(bool isReversing) {  
  if (isReversing) {
    analogWrite(pinReverse, 180);
  } else {
    analogWrite(pinReverse, 0);
  }
}

void OnBreak(bool isBreaking) {
  if (isBreaking) {
    analogWrite(pinBreak, 255);
  } else {
    analogWrite(pinBreak, 0);
  }
}

void Blink1(int turn = 3) {
  if (turn < 1) {
    int pins[2] = {pinLeft, pinRight};
    turnsBlinker.Blink(pins, false);
    return;
  }

  int pins[2] = {-1, -1};
  if (turn == 1 or turn == 3) {
    pins[0] = pinLeft;
  }
  if (turn == 2 or turn == 3) {
    pins[1] = pinRight;
  }

  turnsBlinker.Blink(pins, true);
}

void OnEmergency(bool isOn) {
  int pins[2] = {pinLeft, pinRight};
  turnsBlinker.Blink(pins, isOn);
}

void OnLowVoltage() {
  int pins[2] = {pinLeft, pinRight};
  lowVoltageBlinker.Blink(pins, true);
}

void OnBackFire() {
  analogWrite(pinExhaust, 127);
  delay(100);
  analogWrite(pinExhaust, 0);
  delay(100);
  analogWrite(pinExhaust, 127);
  delay(50);
  analogWrite(pinExhaust, 0);
  delay(60);
  analogWrite(pinExhaust, 127);
  delay(300);
  analogWrite(pinExhaust, 0);
  delay(250);
}

int controlLo = 700;
int controlHi = 1600;
Headlights HLights1 = Headlights(controlLo, controlHi, HLights1Toggle);
Headlights HLights2 = Headlights(controlLo, controlHi, HLights2Toggle);

int turnLeftLo = 1500;
int turnRightHi = 1460;
int throttleLo = 1350;
int throttleHi = 1450;
Turns turns = Turns(turnLeftLo, turnRightHi, throttleLo, throttleHi, Blink1);

EmergencyLights emergencyLights = EmergencyLights(1000, 1200, OnEmergency);
EmergencyLights emergencyLightsWithDaylights = EmergencyLights(1700, 1900, OnEmergency);

BackFire backFire = BackFire(1500, OnBackFire);

int NeutralLo = 1375;
int NeutralHi = 1400;
BreakReverseState breakReverseState = BreakReverseState(NeutralLo, NeutralHi, 0);
BreakReverse breakReverse = BreakReverse(breakReverseState, OnReverse, OnBreak);

LowVoltageDetector lowVoltageDetector = LowVoltageDetector(6.8, OnLowVoltage);

unsigned long voltage;
unsigned long CH3;
unsigned long CH2;
unsigned long CH1;

void printDebug() {
  Serial.print("CH1: ");
  Serial.print(CH1);
  Serial.print("     ");

  Serial.print("CH2: ");
  Serial.print(CH2);
  Serial.print("     ");

  Serial.print("CH3: ");
  Serial.print(CH3);

  Serial.print("\n");
  delay(100);
}

void initDigitalOuts() {
  int len = 7;

  int pins[7] = {
    pinBreak, pinLights1, pinLights2,
    pinLeft, pinRight, pinExhaust,
    pinReverse
  };

  for (int i = 0; i < len; i++) {
    int pin = pins[i];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void setup() {
  initDigitalOuts();

  pinMode(pinCh1, INPUT);
  pinMode(pinCh2, INPUT);
  pinMode(pinCh3, INPUT);

  if (Debug) {
    Serial.begin(9600);
  }
}

void loop() {
  CH1 = pulseIn(pinCh1, HIGH);
  CH2 = pulseIn(pinCh2, HIGH);
  CH3 = pulseIn(pinCh3, HIGH);
  voltage = analogRead(pinVoltageMetter);

  if (Debug) {
    printDebug();
  }

  if (lowVoltageDetector.evaluate(voltage)) {
    OnLowVoltage();

    return;
  }

  unsigned long millisec = millis();
  if (not emergencyLights.evaluate(CH3) && not emergencyLightsWithDaylights.evaluate(CH3)) {
    turns.evaluate(millisec, CH1, CH2);
  }
  
  HLights1.evaluate(CH3);
  HLights2.evaluate(CH3);
//  backFire.evaluate(CH2);
  breakReverse.evaluate(CH2, millisec);
}
