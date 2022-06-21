typedef void (*OnBreakEvent)(bool);
typedef void (*OnReverseEvent)(bool);

class BreakReverseState {
  private:
    unsigned long standLo;
    unsigned long standHi;
    short breakDelay;

    int previousState = NEUTRAL;
  public:
    static const int NEUTRAL = 1;
    static const int FORWARDING = 2;
    static const int REVERSING = 3;
    static const int BREAKING = 4;

    BreakReverseState(unsigned long standLo, unsigned long standHi, short breakTimeout = 0):
    standLo(standLo),
    standHi(standHi),
    breakDelay(breakDelay)
    {}

  int getState(unsigned long throttle, unsigned long currentMillis) {
    int state = 0;
    if ((throttle > this->standLo) && (throttle < this->standHi)) {
      state = BreakReverseState::NEUTRAL;
    } else if (throttle >= this->standHi) {
      state = BreakReverseState::FORWARDING;
    } else {
      state = BreakReverseState::REVERSING;
    };

    if (state == BreakReverseState::REVERSING) {
      if ((this->previousState == BreakReverseState::FORWARDING) || (this->previousState == BreakReverseState::BREAKING)) {
        state = BreakReverseState::BREAKING;
      }
    }

    this->previousState = state;

    return state;
  }
};

class BreakReverse {
  private:
    BreakReverseState breakReverse;
    OnReverseEvent onReverse;
    OnBreakEvent onBreak;
  public:
    BreakReverse(BreakReverseState breakReverse, OnReverseEvent onReverse, OnBreakEvent onBreak):
      breakReverse(breakReverse),
      onReverse(onReverse),
      onBreak(onBreak)
      {}

    void evaluate(unsigned long throttle, unsigned long currentMillis) {
      switch(this->breakReverse.getState(throttle, currentMillis)) {
        case BreakReverseState::BREAKING:
          this->onBreak(true);
          this->onReverse(false);
          break;
        case BreakReverseState::REVERSING:
          this->onBreak(false);
          this->onReverse(true);
          break;
        default: // neutral or forward
          this->onBreak(false);
          this->onReverse(false);
      };
    }
};
