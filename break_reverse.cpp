typedef void (*OnBreakEvent)(bool);
typedef void (*OnReverseEvent)(bool);

// Brake + reverse share CH2 <= brakeReverseHi (see doc/brake_reverse_spec.md).
class BreakReverseState {
  private:
    static const unsigned long BAND_DEBOUNCE_MS = 50;
    static const unsigned long FORWARD_HOLD_MS = 120;
    static const unsigned long LATCH_CLEAR_MS = 400;

    unsigned long brakeReverseHi;
    unsigned long forwardLo;
    unsigned long reverseEngageMs;

    int previousState = NEUTRAL;
    int previousBand = 0;
    unsigned long previousMillis = 0;
    unsigned long forwardHeldSince = 0;
    unsigned long sharedEnteredAt = 0;
    unsigned long leftSharedAt = 0;
    bool forwardTrip = false;
    bool reverseLatched = false;

    static const int BAND_SHARED = 1;
    static const int BAND_NEUTRAL = 2;
    static const int BAND_FORWARD = 3;

    int throttleBand(unsigned long throttle) const {
      if (throttle <= this->brakeReverseHi) {
        return BAND_SHARED;
      }
      if (throttle < this->forwardLo) {
        return BAND_NEUTRAL;
      }
      return BAND_FORWARD;
    }

    bool inSharedRange(unsigned long throttle) const {
      return throttle <= this->brakeReverseHi;
    }

    bool forwardConfirmed(unsigned long nowMs) const {
      return this->forwardHeldSince > 0
          && this->forwardHeldSince + FORWARD_HOLD_MS <= nowMs;
    }

    // After forward only: brake this long in shared range before reverse lamp.
    bool brakeHoldElapsed(unsigned long nowMs) const {
      return this->sharedEnteredAt > 0
          && this->sharedEnteredAt + this->reverseEngageMs <= nowMs;
    }

    void updateForwardTrip(unsigned long throttle, unsigned long nowMs) {
      int band = this->throttleBand(throttle);
      if (band == BAND_FORWARD) {
        if (this->forwardHeldSince == 0) {
          this->forwardHeldSince = nowMs;
        }
        if (this->forwardConfirmed(nowMs)) {
          this->forwardTrip = true;
        }
      } else {
        this->forwardHeldSince = 0;
      }
    }

    void updateSharedTimer(unsigned long throttle, unsigned long nowMs) {
      if (this->inSharedRange(throttle)) {
        if (this->sharedEnteredAt == 0) {
          this->sharedEnteredAt = nowMs;
        }
        this->leftSharedAt = 0;
      } else {
        this->sharedEnteredAt = 0;
        if (this->leftSharedAt == 0) {
          this->leftSharedAt = nowMs;
        } else if (this->leftSharedAt + LATCH_CLEAR_MS <= nowMs) {
          this->reverseLatched = false;
          this->forwardTrip = false;
        }
      }
    }
  public:
    int lastBand = 0;
    int lastFsmState = NEUTRAL;
    bool lastForwardTrip = false;

    static const int NEUTRAL = 1;
    static const int FORWARDING = 2;
    static const int REVERSING = 3;
    static const int BREAKING = 4;

    static const char* bandName(int band) {
      switch (band) {
        case BAND_SHARED: return "SHR";
        case BAND_NEUTRAL: return "NEU";
        case BAND_FORWARD: return "FWD";
        default: return "?";
      }
    }

    static const char* stateName(int state) {
      switch (state) {
        case NEUTRAL: return "NEUTRAL";
        case FORWARDING: return "FWD";
        case REVERSING: return "REV";
        case BREAKING: return "BRK";
        default: return "?";
      }
    }

    BreakReverseState(unsigned long brakeReverseHi, unsigned long forwardLo,
        unsigned long reverseEngageMs = 500):
      brakeReverseHi(brakeReverseHi),
      forwardLo(forwardLo),
      reverseEngageMs(reverseEngageMs)
    {}

    int getState(unsigned long throttle, unsigned long nowMs) {
      if (this->previousMillis > nowMs) {
        this->previousMillis = nowMs;
      }

      int band = this->throttleBand(throttle);
      if (band == this->previousBand && this->previousMillis + BAND_DEBOUNCE_MS > nowMs) {
        return this->previousState;
      }

      this->updateForwardTrip(throttle, nowMs);
      this->updateSharedTimer(throttle, nowMs);
      this->lastForwardTrip = this->forwardTrip;

      int state = NEUTRAL;

      if (band == BAND_FORWARD) {
        state = FORWARDING;
      } else if (!this->inSharedRange(throttle)) {
        state = NEUTRAL;
      } else if (!this->forwardTrip) {
        state = REVERSING;
      } else if (this->brakeHoldElapsed(nowMs)) {
        state = REVERSING;
      } else {
        state = BREAKING;
      }

      if (state == REVERSING) {
        this->reverseLatched = true;
      }

      this->previousState = state;
      this->previousBand = band;
      this->previousMillis = nowMs;
      this->lastBand = band;
      this->lastFsmState = state;

      return state;
    }

    void resetOutputs() {
      this->reverseLatched = false;
      this->forwardTrip = false;
      this->forwardHeldSince = 0;
      this->sharedEnteredAt = 0;
      this->leftSharedAt = 0;
      this->previousState = NEUTRAL;
      this->lastFsmState = NEUTRAL;
      this->lastBand = 0;
      this->lastForwardTrip = false;
    }
};

class BreakReverse {
  private:
    BreakReverseState state;
    OnReverseEvent onReverse;
    OnBreakEvent onBreak;
  public:
    bool lastBrakeOn = false;
    bool lastReverseOn = false;

    BreakReverse(BreakReverseState state, OnReverseEvent onReverse, OnBreakEvent onBreak):
      state(state),
      onReverse(onReverse),
      onBreak(onBreak)
    {}

    void evaluate(unsigned long throttle, unsigned long nowMs) {
      bool brakeOn = false;
      bool reverseOn = false;

      switch (this->state.getState(throttle, nowMs)) {
        case BreakReverseState::BREAKING:
          brakeOn = true;
          break;
        case BreakReverseState::REVERSING:
          reverseOn = true;
          break;
        default:
          break;
      }

      this->lastBrakeOn = brakeOn;
      this->lastReverseOn = reverseOn;
      this->onBreak(brakeOn);
      this->onReverse(reverseOn);
    }

    void reset() {
      this->state.resetOutputs();
      this->lastBrakeOn = false;
      this->lastReverseOn = false;
      this->onBreak(false);
      this->onReverse(false);
    }
};
