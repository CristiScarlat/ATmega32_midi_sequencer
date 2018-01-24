
#include <avr/io.h>
#include <avr/interrupt.h>

# define USART_BAUDRATE 9600
# define BAUD_PRESCALE (((( 16000000UL / 16) + ( USART_BAUDRATE / 2) ) / ( USART_BAUDRATE )) - 1)

void USART_init(){
	UCSRB = (1 << RXEN ) | (1 << TXEN ); // Turn on the transmission and reception circuitry
	UCSRC = (1 << URSEL ) | (1 << UCSZ0 ) | (1 << UCSZ1 ); // Use 8- bit character sizes
	UBRRH = ( BAUD_PRESCALE >> 8) ; // Load upper 8- bits of the baud rate value into the high byte of the UBRR register
	UBRRL = BAUD_PRESCALE ; // Load lower 8 - bits of the baud rate value into the low byte of the UBRR register
	UCSRB |= (1 << RXCIE ); // Enable the USART Recieve Complete interrupt ( USART_RXC )
}

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
  OCR1A = 24;// = (16*10^6) / (10000*64) - 1 (must be <65536)/10kHz || 100us
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
  PORTB |= 1 << 1;
  PORTB &= ~(1 << 0);
}

void initSetup(){
  cli();
  timer1_init();
  pinINT0_init();
  USART_init();
  sei();
  io_init();
}
