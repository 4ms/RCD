// VERSION 1.1.1: Reversed orientation of Spread switch
// Version 1.1:--Added Spread Mode


/* Jumper/Breakout info 

Open	Short	Pin#	Variable_name
Up	Down	PD5	DOWNBEATMODE_JUMPER
Trig	Gate	PD4	GATEMODE_JUMPER
Div32	Div8	PC2	DIVIDEBY32_SWITCH
Div16	Div8	PC3	DIVIDEBY16_SWITCH
off	Sprd	PC4	SPREADMODE_SWITCH
off	Rst16	PC5	AUTO_RESET_SWITCH
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

#define RESET_SWITCH (SWITCH_PIN & (1<<PC1))
#define DIVIDEBY32_SWITCH (SWITCH_PIN & (1<<PC2))
#define DIVIDEBY16_SWITCH (SWITCH_PIN & (1<<PC3))
#define SPREADMODE_SWITCH (!(SWITCH_PIN & (1<<PC4)))
#define AUTO_RESET_SWITCH (!(SWITCH_PIN & (1<<PC5)))


/** MACROS **/

#define ALLON(p,x) p &= ~(x)
#define ALLOFF(p,x) p |= (x)
#define ON(p,x) p &= ~(1<<(x))
#define OFF(p,x) p |= (1<<(x))

uint8_t rotate_table[8]= {0,1,2,3,5,7,11,15};

uint8_t divby(uint8_t rot_amt, uint8_t max_divby, uint8_t jack, uint8_t spread){
	if (spread) {
		if (max_divby==15)  // /2 /4 /6 /8 /10 /12 /14 /16 rotating within 1-16
			return ((((jack<<1)+1)+rot_amt) & 15);
		else if (max_divby==31) // /4 /8 /12... /32 rotating within 1-32
			return ((((jack<<2)+3)+rot_amt) & 31);
		else if (max_divby==63) // /8 /16 /20.../64 rotating within 1-64
			return ((((jack<<3)+7)+rot_amt) & 63);
		else
			return rotate_table[((jack+rot_amt) & 7)];
		
	} else { //no spread, so jacks are 1...8 (or 1..16, etc depending on maxdivby)
		return ((jack+rot_amt) & max_divby);
	}
}

/** MAIN **/


int main(void){

	uint8_t clock_up=0,clock_down=0, reset2_up=0,autoreset=0;
	uint8_t o0=0,o1=0,o2=0,o3=0,o4=0,o5=0,o6=0,o7=0;
	
	uint8_t div0=0,div1=1,div2=2,div3=3,div4=4,div5=5,div6=6,div7=7;

	uint8_t adc=0,old_adc=0xFF;
	uint8_t d=0;
	uint8_t t=7;

	uint8_t divideby16_switch=0, old_divideby16_switch=0xFF;
	uint8_t divideby32_switch=0, old_divideby32_switch=0xFF;
	uint8_t spreadmode_switch=0, old_spreadmode_switch=0xFF;;
	uint8_t recalc_divs=1;

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
		
		if (adc!=old_adc){
			old_adc=adc;
			recalc_divs=1;
		}
	
		/** READ SWITCHES **/
	
		divideby16_switch=DIVIDEBY16_SWITCH;
		divideby32_switch=DIVIDEBY32_SWITCH;
		spreadmode_switch=SPREADMODE_SWITCH;
		if (divideby16_switch!=old_divideby16_switch){
			old_divideby16_switch=divideby16_switch;
			recalc_divs=1;
		}
		if (divideby32_switch!=old_divideby32_switch){
			old_divideby32_switch=divideby32_switch;
			recalc_divs=1;
		}
		if (spreadmode_switch!=old_spreadmode_switch){ 
			old_spreadmode_switch=spreadmode_switch;
			recalc_divs=1;
		}	


		if (RESET_SWITCH) {
			if (reset2_up==0){
				reset2_up=1;
				o0=0;o1=0;o2=0;o3=0;o4=0;o5=0;o6=0;o7=0;
			}
		} else  reset2_up=0;


		/** RECALCULATE THE DIVIDEBY AMOUNTS FOR EACH JACK (if necessary) **/

		if (recalc_divs){
			recalc_divs=0;

			if (spreadmode_switch){
				if (divideby16_switch){
					if (divideby32_switch){
						t=63;
						d=adc>>2; //0..63
					} else {
						t=15;
						d=adc>>4; //0..15
					}
				} else {
					if (divideby32_switch){
						t=31;
						d=adc>>3; //0..31
					} else {
						t=7;
						d=adc>>5; //0..7 (/1-/8)
					}
				}
			} else {
				if (divideby16_switch){
					if (divideby32_switch){
						t=63;
						d=(adc>>3)+32; //31..95 masked by 63 gives 31-63,0-30 (/32-/64,/1-/31)
					} else {
						t=15;
						d=(adc>>4)+8; //8...23 masked by 15 gives 8-15,0-7 (/9-/16,/1-/8)
					}
				} else {
					if (divideby32_switch){
						t=31;
						d=(adc>>3)+16; //16..47 masked by 31 gives 16-31,0-15 (/17-/32,/1-/16)
					} else {
						t=7;
						d=adc>>5; //0..7 (/1-/8)
					}
				}
			}

			div0=divby(d,t,0,spreadmode_switch);
			div1=divby(d,t,1,spreadmode_switch);
			div2=divby(d,t,2,spreadmode_switch);
			div3=divby(d,t,3,spreadmode_switch);
			div4=divby(d,t,4,spreadmode_switch);
			div5=divby(d,t,5,spreadmode_switch);
			div6=divby(d,t,6,spreadmode_switch);
			div7=divby(d,t,7,spreadmode_switch);
		}


		if (CLOCK_IN){
			clock_down=0;
			if (!clock_up){
				clock_up=1;//rising edge only						
				ON(CLOCK_LED_PORT,CLOCK_LED_pin);

				if (DOWNBEATMODE_JUMPER){
					if (o0==0) ON(OUT_PORT1,0);
					if (o1==0) ON(OUT_PORT1,1);
					if (o2==0) ON(OUT_PORT1,2);
					if (o3==0) ON(OUT_PORT1,3);
					if (o4==0) ON(OUT_PORT1,4);
					if (o5==0) ON(OUT_PORT1,5);
					if (o6==0) ON(OUT_PORT2,7);
					if (o7==0) ON(OUT_PORT2,6);
					
					if (GATEMODE_JUMPER){
						if (o0==((div0>>1)+1)) OFF(OUT_PORT1,0);
						if (o1==((div1>>1)+1)) OFF(OUT_PORT1,1);
						if (o2==((div2>>1)+1)) OFF(OUT_PORT1,2);
						if (o3==((div3>>1)+1)) OFF(OUT_PORT1,3);
						if (o4==((div4>>1)+1)) OFF(OUT_PORT1,4);
						if (o5==((div5>>1)+1)) OFF(OUT_PORT1,5);
						if (o6==((div6>>1)+1)) OFF(OUT_PORT2,7);
						if (o7==((div7>>1)+1)) OFF(OUT_PORT2,6);
					} else {
						if (++o0>div0) o0=0;
						if (++o1>div1) o1=0;
						if (++o2>div2) o2=0;
						if (++o3>div3) o3=0;
						if (++o4>div4) o4=0;
						if (++o5>div5) o5=0;
						if (++o6>div6) o6=0;
						if (++o7>div7) o7=0;
					}
				} else { //DOWNBEAT is off (upbeat)
					if (GATEMODE_JUMPER){
						if (o0==((div0>>1)+1)) ON(OUT_PORT1,0);
						if (o1==((div1>>1)+1)) ON(OUT_PORT1,1);
						if (o2==((div2>>1)+1)) ON(OUT_PORT1,2);
						if (o3==((div3>>1)+1)) ON(OUT_PORT1,3);
						if (o4==((div4>>1)+1)) ON(OUT_PORT1,4);
						if (o5==((div5>>1)+1)) ON(OUT_PORT1,5);
						if (o6==((div6>>1)+1)) ON(OUT_PORT2,7);
						if (o7==((div7>>1)+1)) ON(OUT_PORT2,6);

						if (o0==0) OFF(OUT_PORT1,0);
						if (o1==0) OFF(OUT_PORT1,1);
						if (o2==0) OFF(OUT_PORT1,2);
						if (o3==0) OFF(OUT_PORT1,3);
						if (o4==0) OFF(OUT_PORT1,4);
						if (o5==0) OFF(OUT_PORT1,5);
						if (o6==0) OFF(OUT_PORT2,7);
						if (o7==0) OFF(OUT_PORT2,6);
					} else {
						if (++o0>div0){ o0=0;ON(OUT_PORT1,0);}
						if (++o1>div1){ o1=0;ON(OUT_PORT1,1);}
						if (++o2>div2){ o2=0;ON(OUT_PORT1,2);}
						if (++o3>div3){ o3=0;ON(OUT_PORT1,3);}
						if (++o4>div4){ o4=0;ON(OUT_PORT1,4);}
						if (++o5>div5){ o5=0;ON(OUT_PORT1,5);}
						if (++o6>div6){ o6=0;ON(OUT_PORT2,7);}
						if (++o7>div7){ o7=0;ON(OUT_PORT2,6);}
					}
				}

				if (AUTO_RESET_SWITCH){
					//(t+1)*2: 16, 32, 64, 128
					if (++autoreset>=((t+1)*2)){
						autoreset=0;
						o0=0;o1=0;o2=0;o3=0;o4=0;o5=0;o6=0;o7=0;

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
						if (o0==((div0+1)>>1)) OFF(OUT_PORT1,0);
						if (o1==((div1+1)>>1)) OFF(OUT_PORT1,1);
						if (o2==((div2+1)>>1)) OFF(OUT_PORT1,2);
						if (o3==((div3+1)>>1)) OFF(OUT_PORT1,3);
						if (o4==((div4+1)>>1)) OFF(OUT_PORT1,4);
						if (o5==((div5+1)>>1)) OFF(OUT_PORT1,5);
						if (o6==((div6+1)>>1)) OFF(OUT_PORT2,7);
						if (o7==((div7+1)>>1)) OFF(OUT_PORT2,6);
					} else {
						if (o0==((div0+1)>>1)) ON(OUT_PORT1,0);
						if (o1==((div1+1)>>1)) ON(OUT_PORT1,1);
						if (o2==((div2+1)>>1)) ON(OUT_PORT1,2);
						if (o3==((div3+1)>>1)) ON(OUT_PORT1,3);
						if (o4==((div4+1)>>1)) ON(OUT_PORT1,4);
						if (o5==((div5+1)>>1)) ON(OUT_PORT1,5);
						if (o6==((div6+1)>>1)) ON(OUT_PORT2,7);
						if (o7==((div7+1)>>1)) ON(OUT_PORT2,6);
					}
					if (++o0>div0){ o0=0;}
					if (++o1>div1){ o1=0;}
					if (++o2>div2){ o2=0;}
					if (++o3>div3){ o3=0;}
					if (++o4>div4){ o4=0;}
					if (++o5>div5){ o5=0;}
					if (++o6>div6){ o6=0;}
					if (++o7>div7){ o7=0;}
				} else {
					ALLOFF(OUT_PORT1,OUT_MASK1);
					ALLOFF(OUT_PORT2,OUT_MASK2);
				}
			}
		}
	
	}	//endless loop


	return(1);
}
