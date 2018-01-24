#!/bin/bash

avr-gcc -w -Os -DF_CPU=16000000UL -mmcu=atmega32 -std=c99 -c -o main.o main.c
avr-gcc -w -mmcu=atmega32 main.o -o main
avr-objcopy -O ihex -R .eeprom main main.hex


