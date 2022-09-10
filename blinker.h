#ifndef BLINKER_H
#define BLINKER_H

class Neotimer;

class Blinker {
  private:
    Neotimer *blinkTimer;
    unsigned long blinkIntervalOn;
    unsigned long blinkIntervalOff;
    unsigned long blinkInterval;
    bool isOn;
  public:
    Blinker(
      unsigned long blinkIntervalOn = 400,
      unsigned long blinkIntervalOff = 200
    );
    void Blink(int pins[2], bool isEnabled);
};

#endif
