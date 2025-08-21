# esp-temp-fan-control

This project uses a DHT22 sensor to gather temperature data.
This data will be used to control a fan via pwm.
Additionally the recorded temp and the fan speed will be reported via mqtt.

## Configuration

use 'idf.py menuconfig' and set you config under "Custom Configuration" menu

## Wiring

see wiring-diagram-sketch.jpeg in project root (for a non pwm fan with 2 or 3 pins)
