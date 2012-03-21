/* Jumper/Breakout info 

Open	Short	Pin#	Variable_name
Up	Down	PD5	DOWNBEATMODE_JUMPER
Trig	Gate	PD4	GATEMODE_JUMPER
Div32	Div8	PC2	ROTATE_JUMPER1
Div16	Div8	PC3	ROTATE_JUMPER2
off	Rst16	PC4	AUTO_RESET1_SWITCH
off	Rst24	PC5	AUTO_RESET2_SWITCH
*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <ctype.h>

/** MODES **/


/** SETTINGS **/



/** PINS **/

#define CLOCK_IN_pin PD2
#define CLOCK_IN_init DDRD &= ~(1<<CLOCK_IN_pin)
#define CLOCK_IN (PIND & (1<<CLOCK_IN_pin))

#define CLOCK_LED_pin PD3
#define CLOCK_LED_init DDRD |=(1<<CLOCK_LED_pin)
#define CLOCK_LED_PORT PORTD

#define DOWNBEATMODE_pin PD5
#define DOWNBEATMODE_init DDRD &= ~(1<<DOWNBEATMODE_pin)
#define DOWNBEATMODE_init_pullup PORTD |= (1<<DOWNBEATMODE_pin) 
#define DOWNBEATMODE_JUMPER (!(PIND & (1<<DOWNBEATMODE_pin)))

#define GATEMODE_pin PD4
#define GATEMODE_init DDRD &= ~(1<<GATEMODE_pin)
#define GATEMODE_init_pullup PORTD |= (1<<GATEMODE_pin) 
#define GATEMODE_JUMPER (!(PIND & (1<<GATEMODE_pin)))


#define OUT_PORT1 PORTB
#define OUT_DDR1 DDRB
#define OUT_MASK1 0b00111111
#define OUT_init1 OUT_DDR1 |= OUT_MASK1

#define OUT_PORT2 PORTD
#define OUT_DDR2 DDRD
#define OUT_MASK2 0b11000000
#define OUT_init2 OUT_DDR2 |= OUT_MASK2

#define SWITCH_MASK 0b111100
#define SWITCH_PULLUP PORTC
#define SWITCH_PIN PINC
#define SWITCH_DDR DDRC
#define SWITCH_init SWITCH_DDR &= ~(SWITCH_MASK)

#define ADC_DDR DDRC
#define ADC_PORT PORTC
#define ADC_pin PC0

#define ROTATE_JUMPER1 PC2
#define ROTATE_JUMPER2 PC3
#define RESET_SWITCH ((SWITCH_PIN & (1<<PC1)))
#define ROTATE_SWITCH (SWITCH_PIN & ((1<<ROTATE_JUMPER1) | (1<<ROTATE_JUMPER2)))
#define AUTO_RESET1_SWITCH (!(SWITCH_PIN & (1<<PC4)))
#define AUTO_RESET2_SWITCH (!(SWITCH_PIN & (1<<PC5)))


/** MACROS **/

#define ALLON(p,x) p &= ~(x)
#define ALLOFF(p,x) p |= (x)
#define ON(p,x) p &= ~(1<<(x))
#define OFF(p,x) p |= (1<<(x))


/** MAIN **/


int main(void){

	unsigned char clock_up=0,clock_down=0, reset2_up=0,autoreset=0;
	unsigned char o0=0,o1=0,o2=0,o3=0,o4=0,o5=0,o6=0,o7=0;



	unsigned char adc=0;
	unsigned char d=0;
	unsigned char t=7;
	unsigned char switchread;


	CLOCK_IN_init; 
	CLOCK_LED_init;

	DOWNBEATMODE_init;
	DOWNBEATMODE_init_pullup;
	GATEMODE_init;
	GATEMODE_init_pullup;


	OUT_init1;
	OUT_init2;

	SWITCH_init; 
	SWITCH_PULLUP |= SWITCH_MASK; //turn pullups on

	//init the ADC:
	ADC_DDR &= ~(1<<ADC_pin); //adc input
	ADC_PORT &= ~(1<<ADC_pin); //disable pullup
	ADCSRA = (1<<ADEN);	//Enable ADC
	ADMUX = (1<<ADLAR);	//Left-Adjust
	
	ADCSRA |= (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);

	ADCSRA |= (1<<ADSC);		//set the Start Conversion Flag in the ADC Status Register

	while(1){


		/** READ ADC **/
		while( !(ADCSRA & (1<<ADIF)) );
		ADCSRA |= (1<<ADIF);		// Clear the flag by sending a logical "1"
		adc=ADCH;
		
		ADCSRA |= (1<<ADSC);		//set the Start Conversion Flag in the ADC Status Register

		/** READ SWITCHES **/
		switchread = ROTATE_SWITCH; //jumpers that set the rotate length
		
		//Jumper IN is Low (L), Jumper Out is High (H)
		//LL=0 -> divide 1-8, LH=8 -> divide 1-16, HL=16 -> divide 1-32, HH -> divide 1-64

		if (switchread==0) {
			t=7;
			d=adc>>5;} 			//LL: d=0..7, masked by t=0b00000111 gives on jack 1 DIV 1-8
		else if (switchread==(1<<ROTATE_JUMPER2)) {
			t=15;
			d=(adc>>4)+8;} 		//LH: d=8..23, masked by t=0b00001111 gives: DIV 9-16|1-8
		else if (switchread==(1<<ROTATE_JUMPER1)) {
			t=31;
			d=(adc>>3)+16;}		//HL: d=16..47, masked by t=0b00011111 gives: DIV 17-32|1-16
		else {
			t=63;
			d=(adc>>3)+32;}		//HH: d=31..95, masked by t=0b00111111 gives: DIV 32-64|1-31


		if (RESET_SWITCH) {
			if (reset2_up==0){
				reset2_up=1;
				o0=0;o1=0;o2=0;o3=0;o4=0;o5=0;o6=0;o7=0;
//				ALLON(OUT_PORT1,OUT_MASK1);
//				ALLON(OUT_PORT2,OUT_MASK2);

			}
		} else {
			reset2_up=0;
		}



		if (CLOCK_IN){
			clock_down=0;

			if (!clock_up){
				clock_up=1;//rising edge only						

				ON(CLOCK_LED_PORT,CLOCK_LED_pin);

				if (DOWNBEATMODE_JUMPER){
					if (o0==0){ON(OUT_PORT1,0);}
					if (o1==0){ON(OUT_PORT1,1);}
					if (o2==0){ON(OUT_PORT1,2);}
					if (o3==0){ON(OUT_PORT1,3);}
					if (o4==0){ON(OUT_PORT1,4);}
					if (o5==0){ON(OUT_PORT1,5);}
					if (o6==0){ON(OUT_PORT2,7);}
					if (o7==0){ON(OUT_PORT2,6);}
					
					if (GATEMODE_JUMPER){
						if (o0==((((0+d)&t)/2)+1)){OFF(OUT_PORT1,0);}
						if (o1==((((1+d)&t)/2)+1)){OFF(OUT_PORT1,1);}
						if (o2==((((2+d)&t)/2)+1)){OFF(OUT_PORT1,2);}
						if (o3==((((3+d)&t)/2)+1)){OFF(OUT_PORT1,3);}
						if (o4==((((4+d)&t)/2)+1)){OFF(OUT_PORT1,4);}
						if (o5==((((5+d)&t)/2)+1)){OFF(OUT_PORT1,5);}
						if (o6==((((6+d)&t)/2)+1)){OFF(OUT_PORT2,7);}
						if (o7==((((7+d)&t)/2)+1)){OFF(OUT_PORT2,6);}
					} else {
						if (++o0>((0+d)&t)){ o0=0;}
						if (++o1>((1+d)&t)){ o1=0;}
						if (++o2>((2+d)&t)){ o2=0;}
						if (++o3>((3+d)&t)){ o3=0;}
						if (++o4>((4+d)&t)){ o4=0;}
						if (++o5>((5+d)&t)){ o5=0;}
						if (++o6>((6+d)&t)){ o6=0;}
						if (++o7>((7+d)&t)){ o7=0;}
					}

				} else {


					if (GATEMODE_JUMPER){
						if (o0==((((0+d)&t)/2)+1)){ON(OUT_PORT1,0);}
						if (o1==((((1+d)&t)/2)+1)){ON(OUT_PORT1,1);}
						if (o2==((((2+d)&t)/2)+1)){ON(OUT_PORT1,2);}
						if (o3==((((3+d)&t)/2)+1)){ON(OUT_PORT1,3);}
						if (o4==((((4+d)&t)/2)+1)){ON(OUT_PORT1,4);}
						if (o5==((((5+d)&t)/2)+1)){ON(OUT_PORT1,5);}
						if (o6==((((6+d)&t)/2)+1)){ON(OUT_PORT2,7);}
						if (o7==((((7+d)&t)/2)+1)){ON(OUT_PORT2,6);}


						if (o0==0){OFF(OUT_PORT1,0);}
						if (o1==0){OFF(OUT_PORT1,1);}
						if (o2==0){OFF(OUT_PORT1,2);}
						if (o3==0){OFF(OUT_PORT1,3);}
						if (o4==0){OFF(OUT_PORT1,4);}
						if (o5==0){OFF(OUT_PORT1,5);}
						if (o6==0){OFF(OUT_PORT2,7);}
						if (o7==0){OFF(OUT_PORT2,6);}

					} else {

						if (++o0>((0+d)&t)){ o0=0;ON(OUT_PORT1,0);}
						if (++o1>((1+d)&t)){ o1=0;ON(OUT_PORT1,1);}
						if (++o2>((2+d)&t)){ o2=0;ON(OUT_PORT1,2);}
						if (++o3>((3+d)&t)){ o3=0;ON(OUT_PORT1,3);}
						if (++o4>((4+d)&t)){ o4=0;ON(OUT_PORT1,4);}
						if (++o5>((5+d)&t)){ o5=0;ON(OUT_PORT1,5);}
						if (++o6>((6+d)&t)){ o6=0;ON(OUT_PORT2,7);}
						if (++o7>((7+d)&t)){ o7=0;ON(OUT_PORT2,6);}


					}


				}

				if (AUTO_RESET1_SWITCH){
					if (AUTO_RESET2_SWITCH){
						//auto reset switches 1 and 2
						//(t+1)*4
						if (++autoreset>=((t+1)*4)){
							autoreset=0;
							o0=0;o1=0;o2=0;o3=0;o4=0;o5=0;o6=0;o7=0;

						}
					} else {
						//auto reset switch 1 only
						//(t+1)*2: 16, 32, 64, 128
						if (++autoreset>=((t+1)*2)){
							autoreset=0;
							o0=0;o1=0;o2=0;o3=0;o4=0;o5=0;o6=0;o7=0;

						}
					}
				} else {
					if (AUTO_RESET2_SWITCH){
						//auto reset switch 2 only
						//(t+1)*3: 24, 48, 96, 192
						if (++autoreset>=((t+1)*3)){
							autoreset=0;
							o0=0;o1=0;o2=0;o3=0;o4=0;o5=0;o6=0;o7=0;

						}
					}
				}


			}
		}else{

			clock_up=0;

			if (!clock_down){
				clock_down=1;//rising edge only						

				OFF(CLOCK_LED_PORT,CLOCK_LED_pin);

				if (GATEMODE_JUMPER){
					if (DOWNBEATMODE_JUMPER){

						if (o0==(((0+d)&t)+1)/2){OFF(OUT_PORT1,0);}
						if (o1==(((1+d)&t)+1)/2){OFF(OUT_PORT1,1);}
						if (o2==(((2+d)&t)+1)/2){OFF(OUT_PORT1,2);}
						if (o3==(((3+d)&t)+1)/2){OFF(OUT_PORT1,3);}
						if (o4==(((4+d)&t)+1)/2){OFF(OUT_PORT1,4);}
						if (o5==(((5+d)&t)+1)/2){OFF(OUT_PORT1,5);}
						if (o6==(((6+d)&t)+1)/2){OFF(OUT_PORT2,7);}
						if (o7==(((7+d)&t)+1)/2){OFF(OUT_PORT2,6);}

					} else {

						if (o0==(((0+d)&t)+1)/2){ON(OUT_PORT1,0);}
						if (o1==(((1+d)&t)+1)/2){ON(OUT_PORT1,1);}
						if (o2==(((2+d)&t)+1)/2){ON(OUT_PORT1,2);}
						if (o3==(((3+d)&t)+1)/2){ON(OUT_PORT1,3);}
						if (o4==(((4+d)&t)+1)/2){ON(OUT_PORT1,4);}
						if (o5==(((5+d)&t)+1)/2){ON(OUT_PORT1,5);}
						if (o6==(((6+d)&t)+1)/2){ON(OUT_PORT2,7);}
						if (o7==(((7+d)&t)+1)/2){ON(OUT_PORT2,6);}


					}
					
					if (++o0>((0+d)&t)){ o0=0;}
					if (++o1>((1+d)&t)){ o1=0;}
					if (++o2>((2+d)&t)){ o2=0;}
					if (++o3>((3+d)&t)){ o3=0;}
					if (++o4>((4+d)&t)){ o4=0;}
					if (++o5>((5+d)&t)){ o5=0;}
					if (++o6>((6+d)&t)){ o6=0;}
					if (++o7>((7+d)&t)){ o7=0;}

				} else {
					ALLOFF(OUT_PORT1,OUT_MASK1);
					ALLOFF(OUT_PORT2,OUT_MASK2);
				}
			}
		}
	
	}	//endless loop


	return(1);
}
