#!/bin/bash

avr-gcc -Wall -Os -mmcu=atmega32 -std=c99  main.c init_setup.c user_interface.c -o main.o
#avr-gcc -w -mmcu=atmega32 main.o -o main
avr-objcopy -O ihex -R .eeprom main.o main.hex


