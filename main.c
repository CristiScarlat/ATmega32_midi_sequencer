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
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "hd44780_settings.h"
#include "hd44780.h"
#include "hd44780.c"

#define note_on 9
#define note_off 8

char strbuff[10];
//Steps Data
uint8_t notes[16] = {28, 31, 33, 31, 38, 36, 38, 40, 44, 46, 44, 46, 44, 46, 44, 46};
uint8_t velocitys[16] = {100, 45, 45, 45, 100, 45, 45, 45, 100, 45, 45, 45, 100, 45, 100, 100};
uint8_t note_gate[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
//
volatile uint16_t no_of_milis = 0;
volatile uint8_t enc_val = 0;
uint16_t note_length;
uint8_t tempo = 10;
uint8_t step_index = 0;
uint8_t led_step = 0;
uint8_t flags;//8 flags = (bit0 = play/stop, bit1 = edit)
uint8_t inMIDInote = 0;
uint8_t inMIDIvel = 0;
uint8_t end_step = 16;
uint8_t prev_val = 0;
uint8_t push = 0;
uint8_t menu = 0;
uint8_t play_mode = 0;

void pinINT0_init() {
  MCUCR |= 1 << ISC00;
  MCUCR |=  1 << ISC01;
  GICR |= 1 << INT0;
  MCUCR |= 1 << ISC10;
  MCUCR |=  1 << ISC11;
  GICR |= 1 << INT1;
}

void timer1_init()
{  
  // turn on CTC mode
  // set up timer with prescaler = 64 and CTC mode
  TCCR1B |= (1 << WGM12)|(1 << CS11)|(1 << CS10);  
  TCNT1  = 0;//initialize counter value to 
  //compare match register = [ 16,000,000Hz/ (prescaler * desired interrupt frequency) ] - 1
  OCR1A = 249;// = (16*10^6) / (1000*64) - 1 (must be <65536)/
  // enable timer compare interrupt
  TIMSK |= (1 << OCIE1A);  
}

void io_init(){
  DDRD = 0;//PORTD as input
  PORTD = 255;// PULLUP enable on PORTD
  DDRC = 255;// PORTC as output
  PORTC = 1;//initial value for PORTC
  DDRB |= 1 << 0;//PORTB 0 as output
  DDRB |= 1 << 1;//PORTB 1 as output
  PORTB |= 1 << 0;
  PORTB &= ~(1 << 1);
}

void display_init(){
  lcd_init();
  lcd_clrscr();
  lcd_puts("|Mode|Tempo|End|");
  lcd_setCursor(1, 0);
  lcd_puts("| >> |     |   |");
  lcd_setCursor(1, 7);
  lcd_puts(itoa(tempo, strbuff, 10));
  lcd_setCursor(1, 13);
  lcd_puts(itoa(end_step, strbuff, 10));
}
/*
void MIDI_TX(uint8_t cmd, uint8_t channel, uint8_t pitch, uint8_t velocity)
{
  uint8_t cmd_tx = (cmd << 4) + (channel - 1);
  Serial.write(cmd_tx);
  Serial.write(pitch);
  Serial.write(velocity);
}
*/
int debounce(int pin) {
  _delay_ms(20);
  if (!(PIND & (1 << pin)))return 1;// check if button stable = debounce
  else return 0;
}

void checkPlay(){
	if (!(PIND & (1 << PIND7))) {
    if (debounce(PIND7) == 1) {
      flags ^= 1 << 0;
    }
    while (!(PIND & (1 << PIND7))) {}// take your finger off the button
  }
}

void checkEdit(){
	if (!(PIND & (1 << PIND4))) {
		if (debounce(PIND4) == 1) {		
			enc_val = 0;
			flags |= 1 << 1; 
			push++;   
			lcd_setCursor(1, 1);
			lcd_puts(" ");
			lcd_setCursor(1, 1);
			lcd_puts(itoa(push, strbuff, 10));
			displayMenuCursor();
		}
    while (!(PIND & (1 << PIND4))) {}// take your finger off the button
  }
}

void displayMenuCursor() {
	lcd_setCursor(1, 0);
	lcd_puts("|");
	lcd_setCursor(1, 5);
	lcd_puts("|");
	lcd_setCursor(1, 11);
	lcd_puts("|");
	lcd_setCursor(1, 15);
	lcd_puts("|");
	switch(menu){
		case 0: //set mode
		lcd_setCursor(1, 0);
		lcd_puts("[");
		lcd_setCursor(1, 5);
		lcd_puts("]");
		break;
		case 1://set Tempo
		lcd_setCursor(1, 5);
		lcd_puts("[");
		lcd_setCursor(1, 11);
		lcd_puts("]");
		break;
		case 2://set end step
		lcd_setCursor(1, 11);
		lcd_puts("[");
		lcd_setCursor(1, 15);
		lcd_puts("]");
		break;
	}
}

void exitMenu() {
	lcd_setCursor(1, 0);
	lcd_puts("|");
	lcd_setCursor(1, 5);
	lcd_puts("|");
	lcd_setCursor(1, 11);
	lcd_puts("|");
	lcd_setCursor(1, 15);
	lcd_puts("|");		  
}

void editMenu() {
	switch(push){
		case 1:
		if(enc_val != prev_val){
			prev_val = enc_val;
			menu = enc_val;    
			displayMenuCursor();
			lcd_setCursor(1, 3);
			lcd_puts("  ");
			lcd_setCursor(1, 3);
			lcd_puts(itoa(enc_val, strbuff, 10));			
			}
		break;
		case 2:
		if(menu == 0 && enc_val < 4){
			  play_mode = enc_val;
		  }
		  else if(menu == 1){
			  if(enc_val != prev_val){
				  if(enc_val < 10)enc_val = 10;
				  else if(enc_val > 250)enc_val = 250;
					prev_val = enc_val;
					tempo = enc_val;
					note_length = (60000 / tempo) / 4; //16th note rezolution
					lcd_setCursor(1, 7);
					lcd_puts("   ");
					lcd_setCursor(1, 7);
					lcd_puts(itoa(tempo, strbuff, 10));									  		
				}
		  }
		  else if(menu == 2){
			  if(enc_val != prev_val){
				if(enc_val < 1)enc_val = 1;
				else if(enc_val > 16)enc_val = 16;
				prev_val = enc_val;
				end_step = enc_val;
				lcd_setCursor(1, 13);
				lcd_puts("  ");
				lcd_setCursor(1, 13);
				lcd_puts(itoa(end_step, strbuff, 10));
				}
			}
		 break;
		 case 3:
		 menu = 0;
		 enc_val = 0;
		 push = 0;
		 flags &= ~(1 << 1);
		 exitMenu();	
		break;
	}
}

void initSetup(){
  cli();
  timer1_init();
  pinINT0_init();
  sei();
  io_init();
  display_init();
  note_length = (60000 / tempo) / 4; //16th note rezolution
} 

int main(void) {
	initSetup();
 while(1){  
   checkPlay();
   checkEdit();
   if((flags >> 1) & 1)editMenu();
  }
}

ISR(TIMER1_COMPA_vect) {
  // keep a track of number of miliseconds
  no_of_milis++;
  // if number of millis is eql to duration of the 16th_note and we are in play mode
  if (no_of_milis >= note_length && ((flags >> 0) & 1)){// NOTE: '>=' used instead of '=='
    //Play
    //(step_index == 0) ? MIDI_TX(note_off, 1, notes[15], 0x00) : MIDI_TX(note_off, 1, notes[step_index - 1], 0x00);// prev note off
   // if (note_gate[step_index] == 1)MIDI_TX(note_on, 1, notes[step_index], velocitys[step_index]); // midi note on
    //
    no_of_milis = 0;   // reset miliseconds counter
    step_index++;
    led_step++;
    if (step_index >= end_step)step_index = 0;   
    if(led_step == 8 || led_step >= end_step){
      led_step=0;
      PORTB ^= 1 << 0;
      PORTB ^= 1 << 1;
    }   
    PORTC = 0;
    PORTC |= 1<<led_step;
  }
}

ISR(INT0_vect)
{
  if ((PIND & (1 << PIND2)) && !(PIND & (1 << PIND3))) enc_val++;
}
ISR(INT1_vect)
{

  if ((PIND & (1 << PIND3)) && !(PIND & (1 << PIND2))) enc_val--;
}
