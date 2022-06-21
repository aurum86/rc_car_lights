# RC car Lights
* Tested on Arduino nano

## Features
1. Daylights (for front and rear). Fade-in/fade-out effect (if using pwm pin).
2. "Xenon" lights. Blinking effect when turning on/off (if using pwm pin).
3. Turn signals (non-blocking blinks)
4. Emergency signals (non-blocking blinks)
5. Reverse light
6. Break signal

## Requirements
* Arduino nano (or other compatible board)
  * 2 output pwm pins
  * 4 digital output pins
  * 2 digital input pins (to capture servo and throttle signals)
    * (+1 digital input recommended to control various modes)
* Arduino IDE libraries:
  * [NeoTimer library](https://github.com/jrullan/neotimer) for non-blocking delay

## Donation
If you like this project, and it helped you to save your precious time, please, don't hesitate to donate some of your coins. You can donate to these addresses:

BITCOIN: bc1qpzgtjrdw04ry2kz9ex6g5ygap3qlp7nx38wgge
