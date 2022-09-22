typedef void (*OnSwitchHeadlights)(bool);

class Headlights {
  private:
    bool on = false;
    unsigned long low;
    unsigned long high;

    OnSwitchHeadlights onSwitch;
  public:
    Headlights(unsigned long low, unsigned long high, OnSwitchHeadlights onSwitch):
      low(low),
      high(high),
      onSwitch(onSwitch) {}
    void evaluate(unsigned long value) {
      bool new_status = value < high && value > low;
      if (new_status == this->on) {
        return;
      }

      this->on = new_status;

      this->onSwitch(this->on);
    }
};
