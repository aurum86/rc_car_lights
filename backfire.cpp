typedef void (*OnFire)(unsigned long intensity);

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
      int intensity = (throttle - 1650) / 10;
      if (intensity < 0) {
        intensity = 0;
      }

      if (throttle < (this->previousThrottle + 200) && (throttle > this->trottleThreshHold)) {
        this->onFire(intensity);
      }
      this->previousThrottle = throttle;
    }
};
