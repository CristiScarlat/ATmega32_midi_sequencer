#!/bin/bash
avrdude -F -V -c arduino -P /dev/ttyUSB0 -p ATmega32 -b115200 -D  -U flash:w:main.hex
