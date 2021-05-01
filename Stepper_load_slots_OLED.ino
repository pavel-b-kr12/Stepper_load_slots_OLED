//#define debug
#define LCD_active_only_on_input
#define LCD_upd_dt 50 //frame time
#define nextLCD_off_il_DT 20 //number of LCD updates, when LCD stay active after set nextLCD_off_il. Each upd cause nextLCD_off_il--;

#define motor_p			3 
#define dir_p			6 

#define btn_slot_next	8
#define btn_run			9
#define pot_spd			A0
#define btn_stop		A1
byte slot=0;
long steps[]={1, 10, 100, 1000, 5000, 10000, 100000, 500000,-999, 0};
long step_target=0;

#include "TimerOne.h"
#include <EEPROM.h>
long EEPROMupd_t=0;

#define OLED_I2C_ADDRESS 0x3C
#define useAdafruit_OLED

#ifdef useAdafruit_OLED
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <splash.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

//for an SSD1306 display connected to I2C (SDA, SCL pins)
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1	//4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#else  //this seems slow clear and cause flicker or I do it wrong
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"//@https://github.com/greiman/SSD1306Ascii/blob/master/examples/TickerAdcDemo/TickerAdcDemo.ino
#define RST_PIN -1 // Define proper RST_PIN if required
SSD1306AsciiAvrI2c oled;
#endif


long next_btns_read_t=0;
bool bbtn_wait_off=false;
long LCD_next_upd_t=0;
long LCD_next_off_t=10000;


#define ttab	Serial.print("\t");


byte il=0;
long dl=0;
long next_step_t=0;
bool bStepState;

void setup()
{
		delay(200);
  #ifdef debug
  Serial.begin(1000000);
  #endif
  
  
pinMode(btn_slot_next, INPUT_PULLUP);
pinMode(btn_run, INPUT_PULLUP);
pinMode(pot_spd, INPUT_PULLUP);
pinMode(btn_stop, INPUT_PULLUP);

#ifdef OLED_I2C_ADDRESS
	#ifdef useAdafruit_OLED
		if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) { // Address 0x3C for 128x32
			#ifdef debug
			Serial.println(F("oled alloc fail"));
			#endif
		}
		#ifndef save_mem
		display.clearDisplay(); display.display();
		#endif
	#else

	#if RST_PIN >= 0
	  oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS, RST_PIN);
	#else // RST_PIN >= 0
	  oled.begin(&Adafruit128x64, OLED_I2C_ADDRESS);
	#endif // RST_PIN >= 0
	  // Call oled.setI2cClock(frequency) to change from the default frequency.

	  oled.setFont(Adafruit5x7);
	  oled.set2X();
	  //oled.clear();
	#endif
#endif

//-- motor shield cnc
	//pinMode(8, OUTPUT);	digitalWrite(8,0); //EN low
	//step
	//pinMode(2, OUTPUT);
	pinMode(motor_p, OUTPUT);
	//pinMode(4, OUTPUT);
	//dir
	//pinMode(5, OUTPUT);
	pinMode(dir_p, OUTPUT);
	//pinMode(7, OUTPUT);

	//digitalWrite(5,1);
	//digitalWrite(6,1);
	//digitalWrite(7,1);

	
	slot_set(EEPROM.read(10));
  #ifdef debug
  Serial.print("load: "); ttab
  Serial.println(slot);
  #endif
  
  Timer1.initialize(100);         // initialize timer1, mks period
//Timer1.pwm(9, 512);                // setup pwm on pin 9, 50% duty cycle
Timer1.attachInterrupt(callback);  // attaches callback() as a timer overflow interrupt
}
void slot_set(byte N){
	slot=N;  if(slot>9) slot=0;
	EEPROMupd_t=millis()+5000;
}

void callback() //step by timer
{
		if(step_target>0)
		{
			if(micros()>next_step_t)
			{
				next_step_t=micros()+10+dl;
					if(bStepState)step_target--;
					digitalWrite(motor_p, bStepState);
					bStepState=!bStepState;
			}
		}
		else
		{
			//Timer1.restart();
			//Timer1.stop();
		}
		
		//stepper.run(); //C:\_wr\_Arduino\position_recorder_stepper_motor\pr\pot_recorder\pot_recorder.ino
}


void loop()
{
	if(EEPROMupd_t!=0 && millis()>EEPROMupd_t)
	{
		EEPROMupd_t=0;
		//EEPROM.put(10, steps[slot]); //does not rewrites the value if it didn't change.
		EEPROM.update(10, slot);
  #ifdef debug
  Serial.print("save: "); ttab
  Serial.println(slot);
  #endif
	}
	
	//---------------------------------------------- UI


if(millis()>next_btns_read_t)
{
						if(il==0)
						{
							dl=(dl+analogRead(pot_spd))/2;
							if(dl>900 || dl<10) dl=100;
							else dl*=4;
						}
						il++;
						
	if(!digitalRead(btn_stop))
	{
		step_target=0; LCD_hold_for(30000);
	}
				
	if(bbtn_wait_off)
	{
		next_btns_read_t=millis()+40;
		
		if(
		digitalRead(btn_slot_next) &&
		digitalRead(btn_run) 
			) bbtn_wait_off=false;
	}
	else
	{
		next_btns_read_t=millis()+80;
	
		if(!digitalRead(btn_slot_next))
		{
			if(!bLCD_off()) slot_set(++slot); 
			//else 1st click activate LCD

			LCD_hold_for(60000); bbtn_wait_off=true;
		}	
		else
		if(!digitalRead(btn_run))
		{
			step_target=steps[slot];
			digitalWrite(dir_p, step_target>0);
			step_target=abs(step_target);
			LCD_hold_for(65000); bbtn_wait_off=true;
  #ifdef debug
  Serial.print("go to: "); ttab
  Serial.println(steps[slot]);
  #endif
		}	
	}
}

//----------------------------------------------
	//LCD_upd();
	if(millis()>LCD_next_upd_t)
	{
		LCD_next_upd_t=millis()+LCD_upd_dt;
#ifdef OLED_I2C_ADDRESS
	#ifdef useAdafruit_OLED
		display.clearDisplay();
	#else
		oled.clear();
	#endif
#endif

		
		if(bLCD_off()) //off LCD only if selected: 179 199 249
		{
			display.display();
		}
		else
		{
			oled_display();
		}
	}
//---------------------------------------------- step wo timer
	/*
	if(step_target>0)
	{
		step_target--;
		//digitalWrite(2,1);
		digitalWrite(motor_p,1);
		//digitalWrite(4,1);
		delayMicroseconds(200+dl);
		//digitalWrite(2,0);
		digitalWrite(motor_p,0);
		//digitalWrite(4,0);
		delayMicroseconds(200+dl);
	}*/

}


void oled_display()
{
 #ifdef OLED_I2C_ADDRESS
 
 	#ifdef useAdafruit_OLED
		display.setTextSize(2); 
		display.setTextColor(SSD1306_WHITE);
		
		display.setCursor(0,0);             // Start at top-left corner
		display.print(slot);	display.print(" : ");	display.print(steps[slot]);  display.println();
		display.print(step_target);		  display.print("  ");		  display.print(dl);
		
		display.display();
	#else
		 
		 
		  // uint32_t m = micros();
		  oled.print(slot);		  oled.print(" : ");		  oled.print(steps[slot]);		  oled.println();
		  oled.print(step_target);		  oled.print("  ");		  oled.print(dl);
		  
		  // oled.println();
		  // oled.set1X();
		  // oled.print(T_pw);
		  // oled.print("  ");
		  // oled.print(motor_pw);
		  
		  // oled.print("\nmicros: ");
		  // oled.print(micros() - m);
	#endif
	

 #endif
}
void LCD_hold_for(uint16_t dt)
{
	LCD_next_off_t=millis()+dt;
}
bool bLCD_off(){
	return millis()>LCD_next_off_t;
}