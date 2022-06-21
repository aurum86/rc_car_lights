#include <neotimer.h>
#include "blinks.h"

Neotimer blinkTimer = Neotimer();
unsigned long blinkIntervalOn = 400;
unsigned long blinkIntervalOff = 200;
unsigned long blinkInterval = blinkIntervalOn;
bool isOn = true;
void Blink(int pins[2], bool isEnabled) {
  if (blinkTimer.repeat(-1, blinkInterval)) {
    int len = 2;
    bool toggled = false;
    short pin;
    unsigned long brightness;

    for (int i = 0; i < len; i++) {
      pin = pins[i];
      if (pin >= 0) {
        if (!toggled) {
          isOn = !isOn;
          blinkInterval = isOn ? blinkIntervalOff : blinkIntervalOn;
          blinkTimer.set(blinkInterval);
          brightness = isOn ? 0 : 255;
          brightness = isEnabled ? brightness : 0;
        }
        toggled = true;
        analogWrite(pin, brightness);
      }
    }
  } else {
  }
}
