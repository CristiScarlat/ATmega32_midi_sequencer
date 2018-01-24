#define F_CPU 16000000UL

#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <string.h>
#include "hd44780_settings.h"
#include "hd44780.h"
#include "hd44780.c"

char strbuff[10];
extern uint8_t tempo;
extern uint8_t end_step;
extern uint8_t step_index;
extern uint8_t flags;
extern uint8_t push;
extern uint8_t menu;
extern uint8_t play_mode;
extern uint8_t prev_val;
extern volatile uint8_t enc_val;
extern uint16_t note_length;

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

static int debounce(int pin) {
  _delay_ms(20);
  if (!(PIND & (1 << pin)))return 1;// check if button stable = debounce
  return 0;
}

void exitMenu() {
	lcd_setCursor(0, 0);
	lcd_puts("|");
	lcd_setCursor(0, 5);
	lcd_puts("|");
	lcd_setCursor(0, 11);
	lcd_puts("|");
	lcd_setCursor(0, 15);
	lcd_puts("|");	
	lcd_setCursor(1, 0);
	lcd_puts("|");
	lcd_setCursor(1, 5);
	lcd_puts("|");
	lcd_setCursor(1, 11);
	lcd_puts("|");
	lcd_setCursor(1, 15);
	lcd_puts("|");		  
}

void displayPlayMode(){
	switch(play_mode){
		case 0:
		lcd_setCursor(1, 1);
		lcd_puts("    ");
		lcd_setCursor(1, 1);
		lcd_puts(" >> ");
		break;
		case 1:
		lcd_setCursor(1, 1);
		lcd_puts("    ");
		lcd_setCursor(1, 1);
		lcd_puts(" << ");
		break;
		case 2:
		lcd_setCursor(1, 1);
		lcd_puts("    ");
		lcd_setCursor(1, 1);
		lcd_puts(" <> ");
		break;
		case 3:
		lcd_setCursor(1, 1);
		lcd_puts("    ");
		lcd_setCursor(1, 1);
		lcd_puts("rand");
		break;
	}
}

void displayMenuCursor() {
	if(push == 1){
		lcd_setCursor(0, 0);
		lcd_puts("|");
		lcd_setCursor(0, 5);
		lcd_puts("|");
		lcd_setCursor(0, 11);
		lcd_puts("|");
		lcd_setCursor(0, 15);
		lcd_puts("|");
		switch(menu){
			case 0: //set mode
			lcd_setCursor(0, 0);
			lcd_puts("[");
			lcd_setCursor(0, 5);
			lcd_puts("]");
			break;
			case 1://set Tempo
			lcd_setCursor(0, 5);
			lcd_puts("[");
			lcd_setCursor(0, 11);
			lcd_puts("]");
			break;
			case 2://set end step
			lcd_setCursor(0, 11);
			lcd_puts("[");
			lcd_setCursor(0, 15);
			lcd_puts("]");
			break;
		}
	}
	else if(push == 2){
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
}

void editMenu() {
	switch(push){
		case 1:
		if(enc_val != prev_val){
			prev_val = enc_val;
			menu = enc_val;    
			displayMenuCursor();		
			}
		break;
		case 2:
		if(menu == 0 && (!((flags >> 0) & 1)) && enc_val != prev_val){
			  if(enc_val > prev_val && play_mode < 4)play_mode++;
				  else if(enc_val < prev_val && play_mode > 0)play_mode--;
			  displayPlayMode();
			  if(play_mode == 1)step_index = end_step;
			  else step_index = 0;
			  prev_val = enc_val;
		  }
		  else if(menu == 1 && enc_val != prev_val){
			  if(enc_val > prev_val && tempo < 250)tempo++;
			    else if(enc_val < prev_val && tempo > 10)tempo--;
			  note_length = (600000 / tempo) / 4; //16th note rezolution
			  lcd_setCursor(1, 7);
			  lcd_puts("   ");
			  lcd_setCursor(1, 7);
			  lcd_puts(itoa(tempo, strbuff, 10));
			  prev_val = enc_val;								  		
		  }
		  else if(menu == 2 && enc_val != prev_val){
			  if(enc_val > prev_val && end_step < 16)end_step++;
				 else if(enc_val < prev_val && end_step > 1)end_step--;
			  lcd_setCursor(1, 13);
			  lcd_puts("  ");
			  lcd_setCursor(1, 13);
		      lcd_puts(itoa(end_step, strbuff, 10));
			  prev_val = enc_val;
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
			displayMenuCursor();
		}
    while (!(PIND & (1 << PIND4))) {}// take your finger off the button
  }
}
