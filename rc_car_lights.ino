//for Arduino
//rc car lights

#include "headlights.cpp"
#include "turns.cpp"
#include "emergency_lights.cpp"
#include "backfire.cpp"
#include "break_reverse.cpp"
#include "blinker.h"
#include "low_voltage_detector.cpp"
#include "fade_curve.h"

bool Debug = true;
// true = tab-separated label:value lines for Tools → Serial Plotter (numeric traces only).
bool DebugPlotter = true;
// Serial + printDebug() block the loop; rate-limit so RC sampling and brake logic stay stable.
static const unsigned long DEBUG_PRINT_INTERVAL_MS = 100;
static unsigned long lastDebugPrintMs = 0;
// When true, skips RC logic and drives every light output steady-on for wiring checks.
bool Troubleshoot = false;

static const unsigned long RC_PULSE_TIMEOUT_US = 25000UL;
static const unsigned long RC_PULSE_MIN_VALID_US = 900UL;
static const unsigned long RC_PULSE_MAX_VALID_US = 2100UL;

static bool isValidRcPulse(unsigned long pulseUs) {
  return pulseUs >= RC_PULSE_MIN_VALID_US && pulseUs <= RC_PULSE_MAX_VALID_US;
}

int pinCh1 = 2; //servo
int pinCh2 = 3; //throttle
int pinCh3 = 4; //control
int pinVoltageMetter = 7;

// D7 exhaust, D8 brake, D9 reverse (see doc/nano_pcb_topology_recommendation.md).
int pinExhaust = 9;
int pinBreak = 8;
int pinReverse = 7;

// Brake: HIGH = on. Reverse: false = HIGH on (true if your driver is inverted).
const bool REVERSE_LED_ACTIVE_LOW = false;
int pinLightsR = 12; //rear lights
int pinLights1 = 11; //daylight rear, this should be pwm pin
int pinLights2 = 10; //xenon, this should be pwm pin
int pinLeft = 5;
int pinRight = 6;

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
    // Total fade ≈ (maxB + 1) × stepDelayMs (was 201 × 20 ms linear); exponential in progress.
    const int maxB = brightness;
    const int numSteps = maxB + 1;
    const unsigned long stepDelayMs = 10;
    const float decay = 7.0f;

    for (int k = 0; k < numSteps; k++) {
      float t = (numSteps <= 1) ? 1.0f : (float)k / (float)(numSteps - 1);
      float factor = fadeExponentialTailFactor(t, decay);
      int level = (int)(maxB * factor + 0.5f);
      if (level < 0) {
        level = 0;
      }
      if (level > maxB) {
        level = maxB;
      }
      analogWrite(pinLights2, level);
      delay(stepDelayMs);
    }
  }
}

void HLights1Toggle(bool isTurnedOn) {
  int brightness = 150;
  if (isTurnedOn) {
    for (int i = 0; i <= brightness; i++) {
      analogWrite(pinLights1, i);
      analogWrite(pinLightsR, i);
      delay(2);
    }
  } else {
    for (int i = brightness; i >= 0; i--) {
      analogWrite(pinLightsR, i);
      analogWrite(pinLights1, i);
      delay(2);
    }
  }
}

static void setReverseLamp(bool on) {
  bool levelHigh = on ? !REVERSE_LED_ACTIVE_LOW : REVERSE_LED_ACTIVE_LOW;
  digitalWrite(pinReverse, levelHigh ? HIGH : LOW);
}

void OnReverse(bool isReversing) {
  setReverseLamp(isReversing);
}

void OnBreak(bool isBreaking) {
  digitalWrite(pinBreak, isBreaking ? HIGH : LOW);
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

  // Blinker only writes pins listed as >= 0. After a fast L↔R change the
  // inactive side keeps its last PWM level unless we clear it here.
  if (turn == 1) {
    analogWrite(pinRight, 0);
  } else if (turn == 2) {
    analogWrite(pinLeft, 0);
  }

  turnsBlinker.Blink(pins, true);
}

void OnEmergency(bool isOn) {
  int pins[2] = {pinLeft, pinRight};
  turnsBlinker.Blink(pins, isOn);
}

void OnLowVoltage(float voltage) {
  int pins[2] = {pinLeft, pinRight};
  lowVoltageBlinker.Blink(pins, true);
}

void OnBackFire(unsigned long intensity) {
  if (intensity > 255) {
    intensity = 255;
  }

  // D7 has no hardware PWM — intensity drives pop count and timing, not brightness.
  int pops = 1;
  if (intensity > 70) {
    pops = 2;
  }
  if (intensity > 160) {
    pops = 3;
  }
  if (intensity > 220 && random(0, 100) < 35) {
    pops = 4;
  }

  for (int i = 0; i < pops; i++) {
    analogWrite(pinExhaust, 255);

    unsigned long flashMs = 10 + intensity / 20;
    if (flashMs > 30) {
      flashMs = 30;
    }
    if (i > 0) {
      flashMs = flashMs * 3 / 4;
    }
    delay(flashMs);

    analogWrite(pinExhaust, 0);

    if (i < pops - 1) {
      unsigned long gapMs = 20 + (unsigned long)random(0, 45);
      if (intensity > 180) {
        gapMs = gapMs * 2 / 3;
      }
      delay(gapMs);
    }
  }
}

// INITIALIZATION
Headlights HLights1 = Headlights(1500, 1900, HLights1Toggle);
Headlights HLights2 = Headlights(1600, 1900, HLights2Toggle);

int turnLeftLo = 1500;
int turnRightHi = 1460;
int throttleLo = 1370;
int throttleHi = 1390;
Turns turns = Turns(turnLeftLo, turnRightHi, throttleLo, throttleHi, Blink1);

EmergencyLights emergencyLights = EmergencyLights(1000, 1200, OnEmergency);
EmergencyLights emergencyLightsWithDaylights = EmergencyLights(1700, 1900, OnEmergency);

BackFire backFire = BackFire(1500, OnBackFire);

// Brake/reverse zones (doc/brake_reverse_spec.md): shared ≤1370, neutral 1371–1389, forward ≥1390.
int NeutralLo = 1375;
int NeutralHi = 1400;
// After forward only: brake lamp duration in shared range before reverse (idle→back is instant).
unsigned long BrakeBeforeReverseMs = 1500;
BreakReverseState breakReverseState = BreakReverseState(NeutralLo, NeutralHi, BrakeBeforeReverseMs);
BreakReverse breakReverse = BreakReverse(breakReverseState, OnReverse, OnBreak);

LowVoltageDetector lowVoltageDetector = LowVoltageDetector(6.8, OnLowVoltage);

// end of INITIALIZATION

unsigned long voltage;
unsigned long CH3;
unsigned long CH2;
unsigned long CH1;

static const unsigned long PLOT_Y_MIN = 1200UL;
static const unsigned long PLOT_Y_MAX = 1600UL;

// Map discrete codes into 1200..1600 so state traces share CH2's plot scale.
static unsigned long plotScaled(unsigned long value, unsigned long valueMin, unsigned long valueMax) {
  if (valueMax <= valueMin) {
    return PLOT_Y_MIN;
  }
  if (value <= valueMin) {
    return PLOT_Y_MIN;
  }
  if (value >= valueMax) {
    return PLOT_Y_MAX;
  }
  return PLOT_Y_MIN + (value - valueMin) * (PLOT_Y_MAX - PLOT_Y_MIN) / (valueMax - valueMin);
}

static unsigned long plotLamp(bool on, unsigned long offY, unsigned long onY) {
  return on ? onY : offY;
}

void printDebug(unsigned long nowMs) {
  if (nowMs - lastDebugPrintMs < DEBUG_PRINT_INTERVAL_MS) {
    return;
  }
  lastDebugPrintMs = nowMs;

  if (DebugPlotter) {
    // band 1=REV 2=NEU 3=FWD; fsm 1=NEUTRAL 2=FWD 3=REV 4=BRK.
    Serial.print("CH2:");
    Serial.print(CH2);
    Serial.print("\tCH1:");
    Serial.print(CH1);
    Serial.print("\tCH3:");
    Serial.print(CH3);
    Serial.print("\tband:");
    Serial.print(plotScaled(breakReverseState.lastBand, 1, 3));
    Serial.print("\tfsm:");
    Serial.print(plotScaled(breakReverseState.lastFsmState, 1, 4));
    Serial.print("\tbrake:");
    Serial.print(plotLamp(breakReverse.lastBrakeOn, 1240UL, 1480UL));
    Serial.print("\treverse:");
    Serial.println(plotLamp(breakReverse.lastReverseOn, 1280UL, 1560UL));
    return;
  }

  Serial.print("CH1:");
  Serial.print(CH1);
  Serial.print(" CH2:");
  Serial.print(CH2);
  Serial.print(" CH3:");
  Serial.print(CH3);
  Serial.print(" band:");
  Serial.print(BreakReverseState::bandName(breakReverseState.lastBand));
  Serial.print(" fsm:");
  Serial.print(BreakReverseState::stateName(breakReverseState.lastFsmState));
  Serial.print(" brake:");
  Serial.print(breakReverse.lastBrakeOn ? 1 : 0);
  Serial.print(" reverse:");
  Serial.println(breakReverse.lastReverseOn ? 1 : 0);
}

void initDigitalOuts() {
  int len = 8;

  int pins[len] = {
    pinBreak, pinLightsR, pinLights1, pinLights2,
    pinLeft, pinRight, pinExhaust,
    pinReverse
  };

  for (int i = 0; i < len; i++) {
    int pin = pins[i];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  setReverseLamp(false);
}

// Steady levels matching the brightest values used in normal operation (see HLights1Toggle, HLights2Toggle, OnReverse, OnBreak, Blinker).
void applyTroubleshootAllLightsOn() {
  analogWrite(pinLights1, 150);
  analogWrite(pinLightsR, 150);
  analogWrite(pinLights2, 200);
  analogWrite(pinLeft, 255);
  analogWrite(pinRight, 255);
  setReverseLamp(true);
  digitalWrite(pinBreak, HIGH);
  analogWrite(pinExhaust, 0);
}

void setup() {
  initDigitalOuts();

  pinMode(pinCh1, INPUT);
  pinMode(pinCh2, INPUT);
  pinMode(pinCh3, INPUT);

  if (Debug) {
    // 115200 keeps printDebug() from blocking pulseIn/debounce (~10× less than 9600).
    Serial.begin(115200);
  }
}

void loop() {
  if (Troubleshoot) {
    applyTroubleshootAllLightsOn();
    return;
  }

  CH1 = pulseIn(pinCh1, HIGH, RC_PULSE_TIMEOUT_US);
  CH2 = pulseIn(pinCh2, HIGH, RC_PULSE_TIMEOUT_US);
  CH3 = pulseIn(pinCh3, HIGH, RC_PULSE_TIMEOUT_US);
  voltage = analogRead(pinVoltageMetter);

  if (lowVoltageDetector.evaluate(voltage)) {
    OnLowVoltage(0);
    OnReverse(false);
    OnBreak(false);
    return;
  }

  unsigned long millisec = millis();
  if (not emergencyLights.evaluate(CH3) && not emergencyLightsWithDaylights.evaluate(CH3)) {
    turns.evaluate(millisec, CH1, CH2);
  }
  
  HLights1.evaluate(CH3);
  HLights2.evaluate(CH3);
  backFire.evaluate(CH2, millisec);

  if (isValidRcPulse(CH2)) {
    breakReverse.evaluate(CH2, millisec);
  } else {
    breakReverse.reset();
  }

  if (Debug) {
    printDebug(millisec);
  }
}
