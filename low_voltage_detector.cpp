#include <Arduino.h>

typedef void (*OnLowVoltageEvent)();

class LowVoltageDetector {
  private:
    float limitVoltage;
    float averageVoltage;
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
