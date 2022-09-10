#include "blinker.h"
#include <neotimer.h>

Blinker::Blinker(
      unsigned long blinkIntervalOn,
      unsigned long blinkIntervalOff
    ):
      blinkIntervalOn(blinkIntervalOn),
      blinkIntervalOff(blinkIntervalOff)
    {
      this->blinkTimer = new Neotimer();
      this->blinkInterval = blinkIntervalOn;
      this->isOn = true;
    };

void Blinker::Blink(int pins[2], bool isEnabled) {
  if (this->blinkTimer->repeat(-1, blinkInterval)) {
    int len = 2;
    bool toggled = false;
    short pin;
    unsigned long brightness;

    for (int i = 0; i < len; i++) {
      pin = pins[i];
      if (pin >= 0) {
        if (!toggled) {
          this->isOn = !this->isOn;
          this->blinkInterval = this->isOn ? this->blinkIntervalOff : this->blinkIntervalOn;
          this->blinkTimer->set(this->blinkInterval);
          brightness = this->isOn ? 0 : 255;
          brightness = isEnabled ? brightness : 0;
        }
        toggled = true;
        analogWrite(pin, brightness);
      }
    }
  }
};
