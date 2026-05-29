typedef void (*OnBreakEvent)(bool);
typedef void (*OnReverseEvent)(bool);

// doc/brake_reverse_spec.md — shared range CH2 <= brakeReverseHi; lamps follow live CH2 only.
class BreakReverse {
  private:
    static const unsigned long FORWARD_HOLD_MS = 120;
    static const unsigned long TRIP_CLEAR_MS = 400;
    static const unsigned long SHARED_ENTER_HOLD_MS = 30;

    unsigned long brakeReverseHi;
    unsigned long forwardLo;
    unsigned long brakeBeforeReverseMs;

    unsigned long forwardHeldSince = 0;
    unsigned long sharedEnteredAt = 0;
    unsigned long centerIdleSince = 0;
    bool forwardTrip = false;
    bool brakeSessionFromForward = false;

    OnReverseEvent onReverse;
    OnBreakEvent onBreak;

    bool inSharedRange(unsigned long throttle) const {
      return throttle <= this->brakeReverseHi;
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
    BreakReverse(unsigned long brakeReverseHi, unsigned long forwardLo,
        unsigned long brakeBeforeReverseMs, OnReverseEvent onReverse, OnBreakEvent onBreak):
      brakeReverseHi(brakeReverseHi),
      forwardLo(forwardLo),
      brakeBeforeReverseMs(brakeBeforeReverseMs),
      onReverse(onReverse),
      onBreak(onBreak)
    {}

    void evaluate(unsigned long throttle, unsigned long nowMs) {
      this->updateForwardTrip(throttle, nowMs);
      this->updateSharedTimer(throttle, nowMs);

      bool brakeOn = false;
      bool reverseOn = false;

      if (this->inSharedRange(throttle) && this->sharedStable(nowMs)) {
        if (!this->forwardTrip) {
          reverseOn = true;
        } else if (this->brakeHoldElapsed(nowMs)) {
          reverseOn = true;
        } else {
          brakeOn = true;
        }
      }

      this->onBreak(brakeOn);
      this->onReverse(reverseOn);
    }

    void reset() {
      this->forwardTrip = false;
      this->brakeSessionFromForward = false;
      this->forwardHeldSince = 0;
      this->sharedEnteredAt = 0;
      this->centerIdleSince = 0;
      this->onBreak(false);
      this->onReverse(false);
    }
};
