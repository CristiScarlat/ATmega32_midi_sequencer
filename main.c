/***********************************************************************
 * LCD 16x2 pin connection
 * PORTB 7 = RS
 * PORTB 6 = E
 * PORTB 5 = D4
 * PORTB 4 = D5
 * PORTB 3 = D6
 * PORTB 2 = D7
 * *********************************************************************
 * LED Display 16 leds
 * PORTC = led anode
 * PORTB 0 = select first 8 leds
 * PORTB 1 select second 8 leds
 * *********************************************************************
 * MIDI RX/TX
 * PORTD 0 MIDI RX
 * PORTD 1 MIDI TX
 * *********************************************************************
 * Rotary encoder connections
 * PORTD 2 = encoder A (INT0)
 * PORTD 3 = encoder B (INT1)
 * PORTD 4 = encoder button
 * *********************************************************************
 * PORTD 5 = Button step_FFW
 * PORTD 6 = Button step_REW
 * PORTD 7 = Button PLAY
***********************************************************************/
#define F_CPU 16000000UL

#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "init_setup.h"
#include "user_interface.h"

#define note_on 9
#define note_off 8


//Steps Data
uint8_t notes[16] = {28, 31, 33, 31, 38, 36, 38, 40, 44, 46, 44, 46, 44, 46, 44, 46};
uint8_t velocitys[16] = {100, 45, 45, 45, 100, 45, 45, 45, 100, 45, 45, 45, 100, 45, 100, 100};
uint8_t note_gate[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
//
volatile uint16_t no_of_hundred_us = 0;
volatile uint8_t enc_val = 0;
uint16_t note_length;
uint8_t tempo = 10;
uint8_t step_index = 0;
uint8_t led_step = 0;
uint8_t flags;//8 flags = (bit0 = play/stop, bit1 = edit, bit2 = flip/flop for pendul play mode)
uint8_t inMIDInote = 0;
uint8_t inMIDIvel = 0;
uint8_t end_step = 16;
uint8_t prev_val = 0;
uint8_t push = 0;
uint8_t menu = 0;
uint8_t play_mode = 0;

/*
void MIDI_TX(uint8_t cmd, uint8_t channel, uint8_t pitch, uint8_t velocity)
{
  uint8_t cmd_tx = (cmd << 4) + (channel - 1);
  Serial.write(cmd_tx);
  Serial.write(pitch);
  Serial.write(velocity);
}
*/

int main(void) {
	display_init();
	initSetup();
	note_length = (600000 / tempo) / 4; //16th note rezolution
 while(1){  
   checkPlay();
   checkEdit();
   if((flags >> 1) & 1)editMenu();
  }
}

void ledDisplay(){
	if(step_index > 7){
		if(!((PORTB >> 1) & 1))PORTB |= 1 << 1;
		if((PORTB >> 0) & 1)PORTB &= ~(1 << 0);
		PORTC = 0;
		PORTC |= 1 << (step_index - 8);
	}
	else if(step_index < 8){
		if(!((PORTB >> 0) & 1))PORTB |= 1 << 0;
		if((PORTB >> 1) & 1)PORTB &= ~(1 << 1);
		PORTC = 0;
		PORTC |= 1 << step_index;
	}
}

void playMode1(){
	ledDisplay();
	step_index++;
    if (step_index >= end_step)step_index = 0; 
}

void playMode2(){
	ledDisplay();	
	step_index--;
    if (step_index == 255)step_index = end_step-1;
}

void playMode3(){
	if(!((flags >> 2) & 1)){
		ledDisplay();
		step_index++;
        if (step_index == end_step){
			flags |= 1 << 2;
			step_index -= 2;
		}
		UDR = step_index;
	}
	else {
		ledDisplay();
		step_index--;
		if (step_index == 0)flags &= ~(1 << 2);
		UDR = step_index;
	}
}

void playMode4(){
	ledDisplay();
	step_index = rand() % 16;
	UDR = step_index;
}

ISR(TIMER1_COMPA_vect) {
  // keep a track of number of 100us intervals
  no_of_hundred_us++;
  // if number of 100us is eql to duration of the 16th_note and we are in play mode
  if (no_of_hundred_us >= note_length && ((flags >> 0) & 1)){// NOTE: '>=' used instead of '=='
    //Play
    //(step_index == 0) ? MIDI_TX(note_off, 1, notes[15], 0x00) : MIDI_TX(note_off, 1, notes[step_index - 1], 0x00);// prev note off
   // if (note_gate[step_index] == 1)MIDI_TX(note_on, 1, notes[step_index], velocitys[step_index]); // midi note on
    //
    no_of_hundred_us = 0;   // reset 100us interval counter
    if(play_mode == 0)playMode1();  
    if(play_mode == 1)playMode2();
    if(play_mode == 2)playMode3();
    if(play_mode == 3)playMode4();
    
  }
}

ISR(USART_RXC_vect){
	//char ReceivedByte ;
	//ReceivedByte = UDR ; // Fetch the received byte value into the variable " ByteReceived "
	//UDR = ReceivedByte ; // Echo back the received byte back to the computer
}

ISR(INT0_vect){
  if ((PIND & (1 << PIND2)) && !(PIND & (1 << PIND3))) enc_val++;
}

ISR(INT1_vect){
  if ((PIND & (1 << PIND3)) && !(PIND & (1 << PIND2))) enc_val--;
}
