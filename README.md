# RC car Lights
* Tested on Arduino nano

## Features
1. Daylights (for front and rear). Fade-in/fade-out effect (if using pwm pin).
2. "Xenon" lights. Blinking effect when turning on/off (if using pwm pin).
3. Turn signals (non-blocking blinks)
4. Emergency signals (non-blocking blinks)
5. Reverse light
6. Break signal
7. Low voltage detection - turn signals shortly blink each second if battery voltage drops bellow the predefined critical voltage limit (default is 6.8V).

## Requirements
* Arduino nano (or other compatible board)
  * 2 output pwm pins
  * 4 digital output pins
  * 2 digital input pins (to capture servo and throttle signals)
    * (+1 digital input recommended to control various modes)
  * 1 analog input for battery voltage monitoring.
* Arduino IDE libraries:
  * [NeoTimer library](https://github.com/jrullan/neotimer) for non-blocking delay

## How to setup
### Calibration
* This is already tested and calibrated for Sanwa RX472 receiver. Therefore, if your receiver is not the same, it is likely that calibration will be needed in order for lights to work as expected.
#### How to calibrate
* For calibration you will have to test your receiver by temporaly enabling the *debug mode* - in the main source file set the Debug variable to true. Then turn the ESC on and plug in the Arduino board to PC using USB cable. Observe the output values of all three channels and make the fine-tunning of initial values in the "INITIALIZATION" section in the main source file.
### Disabling the features you dont need
* Features that you do not need can be disabled by commenting out the corresponding code lines in the main file - rc_car_lights.ino.
### Voltage monitor feature
* Make sure that the voltage being measured is ALWAYS below 5V - Arduino will burn otherwise. Therefore, for 2S battery you will have to divide the battery voltage by half using two 1kOhm resistors.
* Note: the voltage monitor will do false alarm for a few seconds after turning ESC on, if the analog pin for monitor is not connected at all. This is due to some side-effects that are not solved yet.

## Donation
If you like this project, and it helped you to save your precious time, please, don't hesitate to donate some fraction of your coins. You can donate to these addresses:

BITCOIN: bc1qpzgtjrdw04ry2kz9ex6g5ygap3qlp7nx38wgge
