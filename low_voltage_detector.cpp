#include <Arduino.h>

typedef void (*OnLowVoltage)();

class LowVoltageDetector {
  private:
    float limitVoltage;
    float averageVoltage;
    long decayVelocity;
    float batteryNominalVoltage;

    OnLowVoltage onLowVoltage;
  public:
    LowVoltageDetector(float limitVoltage, OnLowVoltage onLowVoltage, float batteryNominalVoltage = 8.4, float decayVelocity = 0.8):
      limitVoltage(limitVoltage),
      onLowVoltage(onLowVoltage),
      batteryNominalVoltage(batteryNominalVoltage),      
      decayVelocity(decayVelocity) {
      this->averageVoltage = batteryNominalVoltage;
    }

    bool evaluate(unsigned long value) {
      float voltage = map(value, 0, 1023, 0, this->batteryNominalVoltage);
      
      this->averageVoltage = (this->decayVelocity * voltage) + (1.0 - this->decayVelocity) * this->averageVoltage;
      
      if (this->averageVoltage <= this->limitVoltage)
      {
        this->onLowVoltage();

        return true;
      }

      return false;
    }
};
