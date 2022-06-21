typedef void (*OnEmergencyLightsBlink)(bool);

class EmergencyLights {
  private:
    unsigned long controlLo;
    unsigned long controlHi;

    OnEmergencyLightsBlink onBlink;
  public:
    EmergencyLights(unsigned long controlLo, unsigned long controlHi, OnEmergencyLightsBlink onBlink):
      controlLo(controlLo),
      controlHi(controlHi),
      onBlink(onBlink)
      {}

    bool evaluate(unsigned long CH3) {
      bool on = false;
      if ((CH3 > this->controlLo) && (CH3 < this->controlHi)) {
        on = true;
        this->onBlink(on);
      }

      return on;
    }
};
