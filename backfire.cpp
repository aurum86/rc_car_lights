typedef void (*OnFire)(unsigned long intensity);

class BackFire {
  private:
    static const unsigned long COOLDOWN_MS = 350;
    static const unsigned long MIN_DELTA_US = 25;

    unsigned long previousThrottle = 1375;
    unsigned long trottleThreshHold;
    unsigned long lastFireMs = 0;

    OnFire onFire;
  public:
    BackFire(unsigned long trottleThreshHold, OnFire onFire):
      trottleThreshHold(trottleThreshHold),
      onFire(onFire)
      {}

    void evaluate(unsigned long throttle, unsigned long nowMs) {
      long delta = (long)throttle - (long)this->previousThrottle;

      if (delta >= MIN_DELTA_US
          && throttle > this->trottleThreshHold
          && (nowMs - this->lastFireMs) >= COOLDOWN_MS) {

        int intensity = (int)((throttle - this->trottleThreshHold) / 2);
        if (intensity > 255) {
          intensity = 255;
        }
        if (intensity < 1) {
          intensity = 1;
        }

        this->onFire((unsigned long)intensity);
        this->lastFireMs = nowMs;
      }

      this->previousThrottle = throttle;
    }
};
