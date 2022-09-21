#include <Arduino.h>

typedef void (*OnLowVoltageEvent)();

class LowVoltageDetector {
  const float ARDUINO_VOLTAGE = 5.0;
  const float VOLTAGE_DIVIDER = 0.5;
  private:
    float limitVoltage;
    float averageVoltage = 0.0;
    long decayVelocity;
    float batteryMaxVoltage;

    OnLowVoltageEvent onLowVoltage;
  public:
    LowVoltageDetector(float limitVoltage, OnLowVoltageEvent onLowVoltage, float batteryMaxVoltage = 8.4, float decayVelocity = 0.8):
      limitVoltage(limitVoltage),
      onLowVoltage(onLowVoltage),
      batteryMaxVoltage(batteryMaxVoltage),
      decayVelocity(decayVelocity) {
      this->averageVoltage = batteryMaxVoltage;
    }

    bool evaluate(unsigned long value) {
      float qoeficient = LowVoltageDetector::ARDUINO_VOLTAGE / this->batteryMaxVoltage / LowVoltageDetector::VOLTAGE_DIVIDER;
      float voltage = this->mapFloat(value * qoeficient, 0.0, 1023.0, 0.0, this->batteryMaxVoltage);
      
      this->averageVoltage = this->calcEMA(voltage);
      
      if (this->averageVoltage <= this->limitVoltage && this->averageVoltage > 3.0)
      {
        this->onLowVoltage();

        return true;
      }

      return false;
    }

    float calcEMA(float value)
    {
       return (value - this->averageVoltage) * 0.2 + this->averageVoltage;
    }

    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
       return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
};
