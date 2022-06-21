typedef void (*OnFire)();

class BackFire {
  private:
    unsigned long previousThrottle = 0;
    unsigned long trottleThreshHold;

    OnFire onFire;
  public:
    BackFire(unsigned long trottleThreshHold, OnFire onFire):
      trottleThreshHold(trottleThreshHold),
      onFire(onFire)
      {}

    void evaluate(unsigned long throttle) {
      if (throttle < (this->previousThrottle + 200) && (throttle > this->trottleThreshHold)) {
        this->onFire();
      }
      this->previousThrottle = throttle;
    }
};
