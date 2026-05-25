typedef void (*OnBreakEvent)(bool);
typedef void (*OnReverseEvent)(bool);

class BreakReverseState {
  private:
    static const unsigned long DELAY_MS = 50;
    static const unsigned long REVERSE_LATCH_CLEAR_MS = 400;
    unsigned long standLo;
    unsigned long standHi;
    short breakTimeout;

    int previousState = NEUTRAL;
    int previousBand = 0;
    unsigned long previousMillis = 0;
    unsigned long currentStateStartedAt = 0;
    bool wasInForwardBand = false;
    bool brakeSessionActive = false;
    bool reverseLightsLatched = false;
    unsigned long leftReverseZoneAt = 0;

    static const int BAND_REVERSE = 1;
    static const int BAND_NEUTRAL = 2;
    static const int BAND_FORWARD = 3;

    int throttleBand(unsigned long throttle) const {
      if (throttle <= this->standLo) {
        return BAND_REVERSE;
      }
      if (throttle < this->standHi) {
        return BAND_NEUTRAL;
      }
      return BAND_FORWARD;
    }

    // Top of neutral band (near NeutralHi) = transmitter idle for this receiver.
    bool idleNeutral(unsigned long throttle) const {
      if (throttle <= this->standLo || throttle >= this->standHi) {
        return false;
      }
      return throttle >= this->standHi - 25;
    }

    bool belowForward(unsigned long throttle) const {
      return throttle < this->standHi;
    }
  public:
    static const int NEUTRAL = 1;
    static const int FORWARDING = 2;
    static const int REVERSING = 3;
    static const int BREAKING = 4;

    BreakReverseState(unsigned long standLo, unsigned long standHi, short breakTimeout = 0):
      standLo(standLo),
      standHi(standHi),
      breakTimeout(breakTimeout)
    {}

  int getState(unsigned long throttle, unsigned long currentMillis) {
    if (this->previousMillis > currentMillis) {
      this->previousMillis = currentMillis;
    }

    int band = this->throttleBand(throttle);
    if (band == this->previousBand && this->previousMillis + DELAY_MS > currentMillis) {
      return this->previousState;
    }

    bool inReverseZone = band == BAND_REVERSE;
    bool inNeutralZone = band == BAND_NEUTRAL;
    bool inForwardZone = band == BAND_FORWARD;
    bool belowFwd = this->belowForward(throttle);
    bool atIdle = this->idleNeutral(throttle);

    if (inForwardZone || (inNeutralZone && throttle >= this->standHi - 40)) {
      this->wasInForwardBand = true;
    }

    int state = 0;

    if (inForwardZone) {
      this->brakeSessionActive = false;
      state = BreakReverseState::FORWARDING;
    } else if (atIdle) {
      this->brakeSessionActive = false;
      state = BreakReverseState::NEUTRAL;
      this->reverseLightsLatched = false;
      this->leftReverseZoneAt = 0;
    } else if (belowFwd && (this->wasInForwardBand || this->brakeSessionActive)) {
      this->brakeSessionActive = true;
      state = BreakReverseState::BREAKING;
    } else if (inReverseZone) {
      state = BreakReverseState::REVERSING;
    } else if (inNeutralZone) {
      state = BreakReverseState::NEUTRAL;
      this->reverseLightsLatched = false;
      this->leftReverseZoneAt = 0;
    } else {
      state = BreakReverseState::NEUTRAL;
    }

    if (!inReverseZone) {
      if (this->leftReverseZoneAt == 0) {
        this->leftReverseZoneAt = currentMillis;
      } else if (this->leftReverseZoneAt + REVERSE_LATCH_CLEAR_MS <= currentMillis) {
        this->reverseLightsLatched = false;
      }
    } else {
      this->leftReverseZoneAt = 0;
    }

    if (inReverseZone && this->reverseLightsLatched) {
      state = BreakReverseState::REVERSING;
    } else if (inReverseZone && this->brakeSessionActive) {
      state = BreakReverseState::BREAKING;
    }

    if (this->previousState != state) {
      this->currentStateStartedAt = currentMillis;
    }

    if (this->breakTimeout > 0
        && inReverseZone
        && state == BreakReverseState::BREAKING
        && this->currentStateStartedAt + this->breakTimeout <= currentMillis) {
      state = BreakReverseState::REVERSING;
    }

    if (state == BreakReverseState::REVERSING) {
      this->reverseLightsLatched = true;
    }

    this->previousState = state;
    this->previousBand = band;
    this->previousMillis = currentMillis;

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
      bool brakeOn = false;
      bool reverseOn = false;

      switch (this->breakReverse.getState(throttle, currentMillis)) {
        case BreakReverseState::BREAKING:
          brakeOn = true;
          break;
        case BreakReverseState::REVERSING:
          reverseOn = true;
          break;
        default:
          break;
      }

      this->onBreak(brakeOn);
      this->onReverse(reverseOn);
    }
};
