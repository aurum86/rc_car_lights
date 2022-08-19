#include <Arduino.h>

typedef void (*OnLowVoltage)();

class LowVoltageDetector {
  private:
    float limitVoltage;
    float averageVoltage;
    long decayVelocity;
    float batteryMaxVoltage;

    OnLowVoltage onLowVoltage;
  public:
    LowVoltageDetector(float limitVoltage, OnLowVoltage onLowVoltage, float batteryMaxVoltage = 8.4, float decayVelocity = 0.8):
      limitVoltage(limitVoltage),
      onLowVoltage(onLowVoltage),
      batteryMaxVoltage(batteryMaxVoltage),
      decayVelocity(decayVelocity) {
      this->averageVoltage = batteryMaxVoltage;
    }

    bool evaluate(unsigned long value) {
      float voltage = map(value, 0, 1023, 0, this->batteryMaxVoltage);
      
      this->averageVoltage = (this->decayVelocity * voltage) + (1.0 - this->decayVelocity) * this->averageVoltage;
      
      if (this->averageVoltage <= this->limitVoltage)
      {
        this->onLowVoltage();

        return true;
      }

      return false;
    }
};
