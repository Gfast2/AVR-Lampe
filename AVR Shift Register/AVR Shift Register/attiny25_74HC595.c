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
#include "attiny25_74HC595.h"
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


// This Author is using interrupt to read ADC.
/* Before the voltage divider :
 *		@ 0.75v		LEDs start to light up
 *		@ 0.61v		lightness changes don't obviously
 *		@ 14.9mv	voltage at maximum lightness (about 12V x 1.124A = 13.488W)
 *
 * After the voltage divider :
 *		@ 230.9mv	LEDs start to light up
 *		@ 200.0mv	lightness changes don't obviously (or maybe is @ 165mv,can't be located easily)
 *		@   3.9mv	voltage at maximum lightness
 *
 * Attiny25 uses a 10bit ADC. So the voltage after voltage divider translated to number in programm (0 - 1023)
 *		47.288		LEDs start to light up
 *		40.960		lightness changes don't obviously (or maybe is 33.792 equal to @165mv)
 *		 0.799		voltage at maximum lightness
 */
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
		
		//_delay_ms(100);
		
		/*
		dutyCycle += 5;
		if(dutyCycle > 255)
			dutyCycle = 127;							// 127.5 = 50%
		*/
		if(firstdigit > 7)
			dutyCycle = (int)(255 * 1);
		else if(firstdigit > 5)
			dutyCycle = (int)(255 * 0.75);
		else if(firstdigit > 3)
			dutyCycle = (int)(255 * 0.55);
		else
			dutyCycle = 0;
	}
}



ISR(ADC_vect)
{
	voltage_reading = ADC;
	//firstdigit = ((voltage_reading * 5000L / 1023)+500) / 1000;
	// voltage_reading 0 ~ 1023.
	//firstdigit = ((voltage_reading * 5000L / 1023)+500) / 1000; //12V through voltage divider have a max. voltage 3.69V. equal to 755.71
	// target: 
	//		readingValue > 48		-	Display : 0
	//		readingValue 48 - 0		-	Display : 0 - 9
	if (voltage_reading > 45) // Try to tune it to the point really start to light up.
	{
		firstdigit = 0;//voltage_reading; 
		//write_74HC595(LED_patterns[firstdigit]);
	}	
	else if(voltage_reading < 4.8) 
	{
		firstdigit = 18; // This is a special '8' with dot, to show the max. performance of LEDs
		//write_74HC595(LED_patterns[firstdigit]);
	}
	else if(voltage_reading < 9.6) // This block is really hard coded. Later find out the problem is caused by the contro PIN of LED driver board don't stable. Add a 4.7µF Cap the problem is all gone.
		firstdigit = 9;
	else if(voltage_reading < 14.4)
		firstdigit = 8;
	else if(voltage_reading < 19.2)
		firstdigit = 7;
	else if(voltage_reading < 24)
		firstdigit = 6;
	else if(voltage_reading < 28.8)
		firstdigit = 5;
	else if(voltage_reading < 33.6)
		firstdigit = 4;
	else if(voltage_reading < 38.4)
		firstdigit = 3;
	else if(voltage_reading < 43.2)
		firstdigit = 2;
	else if(voltage_reading <= 45)
		firstdigit = 1;
		
	write_74HC595(LED_patterns[firstdigit]);
	
	
	/*
	
	else // on theory this should be the last possibility
	{
		for(int i = 0; i < 10; i++)
		{
			int k = (int) (48 - 4.8*i);
			if (voltage_reading > k)
			{
				firstdigit = k;
				write_74HC595(LED_patterns[firstdigit]);
				break;
			}
			
		}
		
	}
	*/
	
		//firstdigit = (int) map(voltage_reading, 48,0, 0, 9);
	
	//write_74HC595(LED_patterns[firstdigit]);
	// TODO :   Add a variable to send back to main loop, in order to drive FAN with the Speed it should using.
	ADCSRA |= 1<<ADSC;
}

ISR(TIMER0_OVF_vect)
{
	OCR0A = dutyCycle;
}