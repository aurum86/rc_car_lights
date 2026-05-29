typedef void (*OnBreakEvent)(bool);
typedef void (*OnReverseEvent)(bool);

// doc/brake_reverse_spec.md — shared range CH2 <= brakeReverseHi; lamps follow live CH2 only.
class BreakReverseState {
  private:
    static const unsigned long FORWARD_HOLD_MS = 120;
    static const unsigned long TRIP_CLEAR_MS = 400;
    static const unsigned long SHARED_EXIT_HYST_US = 12;
    static const unsigned long SHARED_ENTER_HOLD_MS = 30;

    unsigned long brakeReverseHi;
    unsigned long forwardLo;
    unsigned long brakeBeforeReverseMs;

    unsigned long forwardHeldSince = 0;
    unsigned long sharedEnteredAt = 0;
    unsigned long centerIdleSince = 0;
    bool forwardTrip = false;
    bool brakeSessionFromForward = false;

    bool inSharedRange(unsigned long throttle) const {
      return throttle <= this->brakeReverseHi;
    }

    bool aboveSharedExit(unsigned long throttle) const {
      return throttle > this->brakeReverseHi + SHARED_EXIT_HYST_US;
    }

    bool sharedStable(unsigned long nowMs) const {
      return this->sharedEnteredAt > 0
          && this->sharedEnteredAt + SHARED_ENTER_HOLD_MS <= nowMs;
    }

    bool inForwardRange(unsigned long throttle) const {
      return throttle >= this->forwardLo;
    }

    bool inCenterIdle(unsigned long throttle) const {
      return throttle > this->brakeReverseHi && throttle + 12 >= this->forwardLo;
    }

    bool forwardConfirmed(unsigned long nowMs) const {
      return this->forwardHeldSince > 0
          && this->forwardHeldSince + FORWARD_HOLD_MS <= nowMs;
    }

    bool brakeHoldElapsed(unsigned long nowMs) const {
      return this->sharedEnteredAt > 0
          && this->sharedEnteredAt + this->brakeBeforeReverseMs <= nowMs;
    }

    void updateForwardTrip(unsigned long throttle, unsigned long nowMs) {
      if (this->inForwardRange(throttle)) {
        if (this->forwardHeldSince == 0) {
          this->forwardHeldSince = nowMs;
        }
        if (this->forwardConfirmed(nowMs)) {
          this->forwardTrip = true;
        }
      } else {
        this->forwardHeldSince = 0;
      }

      if (this->inSharedRange(throttle) && this->forwardTrip) {
        this->brakeSessionFromForward = true;
      }

      if (!this->inSharedRange(throttle) && this->brakeSessionFromForward) {
        this->forwardTrip = false;
        this->brakeSessionFromForward = false;
        this->sharedEnteredAt = 0;
      }

      if (this->inCenterIdle(throttle)) {
        if (this->centerIdleSince == 0) {
          this->centerIdleSince = nowMs;
        } else if (this->centerIdleSince + TRIP_CLEAR_MS <= nowMs) {
          this->forwardTrip = false;
          this->brakeSessionFromForward = false;
        }
      } else {
        this->centerIdleSince = 0;
      }
    }

    void updateSharedTimer(unsigned long throttle, unsigned long nowMs) {
      if (!this->inSharedRange(throttle)) {
        this->sharedEnteredAt = 0;
      } else if (this->sharedEnteredAt == 0) {
        this->sharedEnteredAt = nowMs;
      }
    }
  public:
    int lastBand = 0;
    int lastFsmState = NEUTRAL;
    bool lastForwardTrip = false;
    bool lastInShared = false;

    static const int NEUTRAL = 1;
    static const int FORWARDING = 2;
    static const int REVERSING = 3;
    static const int BREAKING = 4;

    static const char* bandName(int band) {
      switch (band) {
        case 1: return "SHR";
        case 2: return "NEU";
        case 3: return "FWD";
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
        unsigned long brakeBeforeReverseMs = 1500):
      brakeReverseHi(brakeReverseHi),
      forwardLo(forwardLo),
      brakeBeforeReverseMs(brakeBeforeReverseMs)
    {}

    void updateContext(unsigned long throttle, unsigned long nowMs) {
      this->updateForwardTrip(throttle, nowMs);
      this->updateSharedTimer(throttle, nowMs);
      this->lastForwardTrip = this->forwardTrip;
      this->lastInShared = this->inSharedRange(throttle);

      if (this->inForwardRange(throttle)) {
        this->lastBand = 3;
        this->lastFsmState = FORWARDING;
      } else if (this->inSharedRange(throttle)) {
        this->lastBand = 1;
        if (!this->forwardTrip) {
          this->lastFsmState = REVERSING;
        } else if (this->brakeHoldElapsed(nowMs)) {
          this->lastFsmState = REVERSING;
        } else {
          this->lastFsmState = BREAKING;
        }
      } else {
        this->lastBand = 2;
        this->lastFsmState = NEUTRAL;
      }
    }

    void lampOutputs(unsigned long throttle, unsigned long nowMs, bool& brakeOn, bool& reverseOn) {
      this->updateContext(throttle, nowMs);

      brakeOn = false;
      reverseOn = false;

      if (this->aboveSharedExit(throttle) || !this->inSharedRange(throttle)) {
        return;
      }

      if (!this->sharedStable(nowMs)) {
        return;
      }

      if (!this->forwardTrip) {
        reverseOn = true;
      } else if (this->brakeHoldElapsed(nowMs)) {
        reverseOn = true;
      } else {
        brakeOn = true;
      }
    }

    void resetOutputs() {
      this->forwardTrip = false;
      this->brakeSessionFromForward = false;
      this->forwardHeldSince = 0;
      this->sharedEnteredAt = 0;
      this->centerIdleSince = 0;
      this->lastFsmState = NEUTRAL;
      this->lastBand = 0;
      this->lastForwardTrip = false;
      this->lastInShared = false;
    }
};

class BreakReverse {
  private:
    BreakReverseState state;
    OnReverseEvent onReverse;
    OnBreakEvent onBreak;
    bool pinReverseOn = false;
    bool pinBreakOn = false;
  public:
    bool lastBrakeOn = false;
    bool lastReverseOn = false;
    bool lastForwardOn = false;
    bool lastForwardTrip = false;

    BreakReverse(BreakReverseState state, OnReverseEvent onReverse, OnBreakEvent onBreak):
      state(state),
      onReverse(onReverse),
      onBreak(onBreak)
    {}

    void evaluate(unsigned long throttle, unsigned long nowMs) {
      bool brakeOn = false;
      bool reverseOn = false;

      this->state.lampOutputs(throttle, nowMs, brakeOn, reverseOn);

      this->lastBrakeOn = brakeOn;
      this->lastReverseOn = reverseOn;
      this->lastForwardOn = this->state.lastFsmState == BreakReverseState::FORWARDING;
      this->lastForwardTrip = this->state.lastForwardTrip;

      this->onBreak(brakeOn);
      this->onReverse(reverseOn);
      this->pinBreakOn = brakeOn;
      this->pinReverseOn = reverseOn;
    }

    void reset() {
      this->state.resetOutputs();
      this->lastBrakeOn = false;
      this->lastReverseOn = false;
      this->lastForwardOn = false;
      this->lastForwardTrip = false;
      this->pinBreakOn = false;
      this->pinReverseOn = false;
      this->onBreak(false);
      this->onReverse(false);
    }
};
