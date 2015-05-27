/*
 * AVR_Shift_Register.c
 * Inspired by : https://www.youtube.com/watch?v=d7Au3I4ZdZc
 *
 * Created		: 2015-5-23 6:49:39
 * last Edite	: 2015-5-23
 *  Author		: Su Gao
 */ 

#define F_CPU 1000000UL // In first try it shows about 4 sec. per digit under 8000000UL. And I think the 8 divider (CKDI8) in fuse works.

#include <avr/io.h>
#include <avr/interrupt.h>
#include "attiny84_74HC595.h"
#include <util/delay.h>

volatile uint16_t voltage_reading;
volatile uint8_t  firstdigit;
volatile uint8_t  pwmFAN;
volatile uint8_t  dutyCycle = 127; //percent value of PWM dutyCycle.

//  LSFST,(8 bits stand for: DP,GFEDCBA on 7 segment display respectively)
//  0-ON, 1-OFF
//  elements from 0 - 9 is with dot ON, 10 - 19 is with dot OFF.
uint8_t LED_patterns[20] =
{
	//without dot
	0b11000000,	
	0b11111001,
	0b10100100,
	0b10110000,
	0b10011001,
	0b10010010,
	0b10000010,
	0b11111000,
	0b10000000,
	0b10010000,
	
	// with dot
	0b01000000,
	0b01111001,
	0b00100100,
	0b00110000,
	0b00011001,
	0b00010010,
	0b00000010,
	0b01111000,
	0b00000000,
	0b00010000,
};


// TODO : This Author is using interrupt to read ADC.
//        I'd like to do a small research to figure out if he is using the same pin as mine.
//        Beside, I should really read the ATTiny's datasheet about how it works.
//        We should disable the Reset pin reset function. In oder to save a 10K resistor
void initialise_ADC(void)
{
	ADCSRA |= 1<<ADEN | 1<<ADIE | 1<<ADPS0 | 1<<ADPS2;
	ADMUX  |= 1<<MUX1 | 1<<MUX0;						// PB3 as ADC pin
}

void start_conversion(void)
{
	ADCSRA |= 1<<ADSC;
}


int main(void)
{
	sei();
	initialise_74HC595();
	initialise_ADC();
	start_conversion();
	
	DDRB |= (1<<PB0);									// PWM control pin, as output
	TCCR0A |= (1<<COM0A1) | (1<<WGM00) | (1<<WGM01);	// fast PWM mode + small settings
	
	TIMSK |= (1<<TOIE0);								// Here some differences.
	
	OCR0A = 125;										// should later be comment out to let interrupt handle this problem.
	//OCR0A = 191;//75/100*255;
	
	TCCR0B |= (1<<CS00);								// no prescaler
	
		
	while(1)
	{
		
		_delay_ms(100);
		
		dutyCycle += 5;
		if(dutyCycle > 255)
			dutyCycle = 127;							// 127.5 = 50%
			
		
	}
}



ISR(ADC_vect)
{
	voltage_reading = ADC;
	//firstdigit = ((voltage_reading * 5000L / 1023)+500) / 1000;
	// voltage_reading 0 ~ 1023.
	firstdigit = ((voltage_reading * 10000L / 1023)+500) / 1000; //12V through voltage divider have a max. voltage 3.69V. equal to 755.71
	write_74HC595(LED_patterns[firstdigit]);
	// TODO :   Add a variable to send back to main loop, in order to drive FAN with the Speed it should using.
	ADCSRA |= 1<<ADSC;
}

ISR(TIMER0_OVF_vect)
{
	OCR0A = dutyCycle;
}