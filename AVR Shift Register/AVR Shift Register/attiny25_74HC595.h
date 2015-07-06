#ifndef attiny84_74HC595.h
#define attiny84_74HC595.h

#include <avr/io.h> //standard AVR include file

#define shift_register_PORT   PORTB	//we'll be using port B in this example
#define shift_register_DDR    DDRB

// #define DS_pin PB0   //data pin
#define DS_pin PB1		//pin6 on ATTiny45 data pin

// #define SHCP_pin PB1 //shift clock pin (serial clock)
#define SHCP_pin PB2    // pin7 on ATTiny45

// #define STCP_pin PB2 //store clock pin (latch/register clock)
#define STCP_pin PB4    // pin3 on ATTiny45

#define HC595DataHigh() (shift_register_PORT |=  (1<<DS_pin)) //just some basic macros
#define HC595DataLow()  (shift_register_PORT &= ~(1<<DS_pin))

void initialise_74HC595()
{
   shift_register_DDR|=((1<<SHCP_pin)|(1<<STCP_pin)|(1<<DS_pin));	//sets all the pins to output
}


void pulse_shift_clock()	//sends a serial pulse to the shift clock
{
   shift_register_PORT |=  (1<<SHCP_pin);

   shift_register_PORT &= ~(1<<SHCP_pin);

}

void write_74HC595(uint8_t data)
{
    PORTB &= ~(1<<STCP_pin);	//set the "latch" low
    
    for(uint8_t i=0; i<8; i++)	//send the data serially
    {
        if(data & 0b10000000)   //Mask out the MSB bit of the data
		//if(data & 0b00000001)
        {
            HC595DataHigh();    //if the bit is high, set the data pin high
        }
      
        else
        {
            HC595DataLow();     //if bit is low, set the data pin low
        }

        pulse_shift_clock();	//send a serial pulse
        data = data<<1;			//shift the data and do it again
		//data = data>>1;

   }

   PORTB |= (1<<STCP_pin);		//set the latch high to display the results
}

#endif