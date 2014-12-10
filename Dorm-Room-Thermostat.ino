//       ___ __ 
//     _{___{__}\
//    {_}      `\)            
//   {_}        `            _.-''''--.._
//   {_}                    //'.--.  \___`.
//    { }__,_.--~~~-~~~-~~-::.---. `-.\  `.)
//     `-.{_{_{_{_{_{_{_{_//  -- 8;=- `
//        `-:,_.:,_:,_:,.`\\._ ..'=- , 
//            // // // //`-.`\`   .-'/
//           << << << <<    \ `--'  /----)
//            ^  ^  ^  ^     `-.....--'''
// By Alberto J. Fedeli
// FedelA@rpi.edu
//
// With help from Mark R. Florkowski
// FlorkM@rpi.edu

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <Servo.h> 
#include "DHT.h"


// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

//This is pin data
#define STMPE_CS 8
#define TFT_CS 10
#define TFT_DC 9
DHT dht(2, DHT11);//temp sensor pin (+5, Data, GND), and type of sensor
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//Global Variables
int desiredTemp = 22;
int curentTemp = 20;
int burstTimer = 300; //In seconds
boolean burstMode = false; //false = burst not running, true = burst mode is running
boolean systemState = true; //true = system is in standby, false = system is in auto
boolean heatingMode = true; // false = system is cooling room, true = system in heating room
int timer = 0;
int burstModeStart = 0;
const int minTemp = 15;
const int maxTemp = 30;
Servo myservo;

void blowerHigh()
{
	myservo.write(10);
}

void blowerLow()
{
	myservo.write(120);
}

void setup(void)
{
	myservo.attach(5);
	pinMode(3, OUTPUT);
	digitalWrite(3, HIGH); //HERE BE THE TEST LINE
	Serial.begin(9600);
	dht.begin();
	tft.begin();
	if (!ts.begin()) 
	{ 
		Serial.println("Unable to start touch screen.");
	}	 
	else 
	{ 
		Serial.println("Touch screen started."); 
	}

	//Set Background
	tft.fillScreen(ILI9341_BLACK);
	
	//Run the GUI
	guiWaiting();
	guiDraw();
	guiDrawDesiredTempature();
	guiAutoStndbyMode();
	guiHeatingCoolingMode();
	guiBurstMode();
}

//updates the burstMode button
void guiBurstMode()
{
	if(burstMode == false)
	{
		tft.fillRoundRect(161, 231, 68, 68, 8, tft.color565(255, 70, 0));
		tft.fillRect(161, 231, 59, 68, tft.color565(255, 70, 0));
		tft.setCursor(166, 257);
		tft.setTextColor(ILI9341_WHITE);
		tft.setTextSize(2);
		tft.println("Burst");
	}
}

void guiBurstMode(int time)
{
	if(burstMode == true)
	{
		tft.fillRoundRect(161, 231, 68, 68, 8, ILI9341_BLACK);
		tft.fillRect(161, 231, 59, 68, ILI9341_BLACK);
		tft.setCursor(172, 257);
		tft.setTextColor(ILI9341_WHITE);
		tft.setTextSize(2);
		tft.print(time/60);
		tft.print(":");

		if(time%60<10)
		{
			tft.print("0");
			tft.print(time%60);
		}
		else
		{
			tft.print(time%60);
		}
	}
}

//updates the AutoStandby button
void guiAutoStndbyMode()
{
	if(systemState == true)
	{
		tft.fillRect(81, 231, 79, 68, ILI9341_YELLOW);
		tft.setCursor(85, 257);
		tft.setTextColor(ILI9341_BLACK);
		tft.setTextSize(2);
		tft.println("StndBy");
	}

	if(systemState == false)
	{
		tft.fillRect(81, 231, 79, 68, ILI9341_GREEN);
		tft.setCursor(97, 257);
		tft.setTextColor(ILI9341_WHITE);
		tft.setTextSize(2);
		tft.println("Auto");
	}
}

//updates the HeatingCooling button
void guiHeatingCoolingMode()
{
	if(heatingMode==true)
	{
		tft.fillRoundRect(11, 231, 69, 68, 8, ILI9341_RED);
		tft.fillRect(21, 231, 59, 68, ILI9341_RED);
		tft.setCursor(21, 257);
		tft.setTextColor(ILI9341_WHITE);
		tft.setTextSize(2);
		tft.println("Heat");
	}

	if(heatingMode == false)
	{
		tft.fillRoundRect(11, 231, 69, 68, 8, ILI9341_BLUE);
		tft.fillRect(21, 231, 59, 68, ILI9341_BLUE);
		tft.setCursor(21, 257);
		tft.setTextColor(ILI9341_WHITE);
		tft.setTextSize(2);
		tft.println("Cool");
	}
}

//updates the heater to say "System is: Cooling"
void guiCooling() 
{
	tft.fillRoundRect(10, 20, 220, 25, 8, ILI9341_BLUE);
	tft.fillRect(10, 30, 220, 15, ILI9341_BLUE);
	//"System is"
	tft.setCursor(20, 25);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("System is:");
	tft.setCursor(140, 25);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("Cooling");

}

//updates the heater to say "System is: Heating"
void guiHeating()
{
	
	tft.fillRoundRect(10, 20, 220, 25, 8, ILI9341_RED);
	tft.fillRect(10, 30, 220, 15, ILI9341_RED);
	//"System is"
	tft.setCursor(20, 25);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("System is:");
	tft.setCursor(140, 25);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("Heating");
}

//updates the heater to say "System is: Waiting"
void guiWaiting()
{

	tft.fillRoundRect(10, 20, 220, 25, 8, ILI9341_YELLOW);
	tft.fillRect(10, 30, 220, 15, ILI9341_YELLOW);
	//"System is"
	tft.setCursor(20, 25);
	tft.setTextColor(ILI9341_BLACK);
	tft.setTextSize(2);
	tft.println("System is:");
	tft.setCursor(140, 25);
	tft.setTextSize(2);
	tft.println("Waiting");
}

//draws the desired temperature
void guiDrawDesiredTempature()
{
	tft.fillRect(15, 55, 95, 65, ILI9341_BLACK);
	tft.setCursor(20, 60);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(8);
	tft.println(desiredTemp);
}

//draws the current temperature
void guiDrawCurrentTempature(int temp)
{
	Serial.println("temp updated");
	tft.fillRect(140, 80, 60, 40, ILI9341_BLACK);
	tft.setCursor(140, 81);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(5);
	curentTemp = temp;
	tft.println(temp);
}

void guiDraw()
{
	//Print Location in top left
	tft.setCursor(0, 0);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(1);
	tft.println("Alberto's Room"); //Location text
	//Top Large Rectangle
	tft.drawRoundRect(10, 20, 220, 185, 8, ILI9341_WHITE);
	//Degree sign for desired temp
	tft.setCursor(115, 48);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("o");
	//"Set Temp"
	tft.setCursor(20, 127);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(1);
	tft.println("Set Temp");
	//Degree sign for curent temp
	tft.setCursor(200, 67);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("o");
	//"Current Temp"
	tft.setCursor(140, 127);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(1);
	tft.println("Current Temp");
	//Up Triangle
	tft.fillTriangle(175, 150, 135, 195, 215, 195, ILI9341_WHITE);
	//Down Triangle
	tft.fillTriangle(65, 195, 025, 150, 105, 150, ILI9341_WHITE);
	//Box around triangles
	tft.drawRoundRect(10, 140, 220, 65, 8, ILI9341_WHITE);
	//line between each triangle
	tft.drawLine(120, 140, 120, 203, ILI9341_WHITE);
	//Smaller bottom rectangle
	tft.drawRoundRect(10, 230, 220, 70, 8, ILI9341_WHITE);
	//left most line splitting the box into 1/3's
	tft.drawLine(80, 230, 80, 298, ILI9341_WHITE);
	//right most line splitting the box into 1/3's
	tft.drawLine(160, 230, 160, 298, ILI9341_WHITE);
	//"Designed by text" Go ahead, remove it, I'm not even sad. I bet you're adopted. 
	tft.setCursor(0, 310);
	tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
	tft.println("Designed by Alberto J. Fedeli 2014");
}
//---------------------------------void loop()---------------------------------------------------------------------------
void loop() 
{
	if((timer % 10)==0)
	{
		int temp = dht.readTemperature();
		if(temp != curentTemp)
		{
			Serial.println("temps are not the same");
			guiDrawCurrentTempature(temp);
		}
	}
	else
	{
		digitalWrite(3, HIGH); //HERE BE THE TEST LINE
	}
	
	if(burstMode == true && (timer % 10) == 0)
	{
		if(burstModeStart == 0)
		{
			burstModeStart = timer;
			Serial.println("burst mode start fixes");
		}
		guiBurstMode((4500 - (timer - burstModeStart))/10);
		if((timer - burstModeStart)==4500)
		{
			burstMode = false;
			guiBurstMode();
		}
	}



	int bufferSize = 0;
	if (!ts.bufferEmpty())
	{
		if (bufferSize = 0)
		{
			bufferSize = ts.bufferSize();
		}

		// Retrieve a point  
		TS_Point p = ts.getPoint();
		delay(50);
		// Scale using the calibration #'s
		p.x = map(p.x, TS_MINY, TS_MAXY, 0, tft.height());
		p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
		int y = tft.height() - p.x;
		int x = p.y;

		//Serial.print(x);
		//Serial.print(" ");
		//Serial.println(y);
		//Serial.println(ts.touched());
		//Serial.println(ts.bufferSize());

		//temp-down
		if (x > 100 && x < 150 && y > 160 && y < 300 && ts.bufferSize()==bufferSize && desiredTemp != minTemp)
		{
			Serial.println("temp down");
			tft.fillTriangle(65, 195, 025, 150, 105, 150, ILI9341_BLUE); //Down Triangle
			desiredTemp-=1;
			delay(100);
			tft.fillTriangle(65, 195, 025, 150, 105, 150, ILI9341_WHITE); //Down Triangle
			guiDrawDesiredTempature();
			if(desiredTemp == minTemp)
			{
				tft.fillTriangle(65, 195, 025, 150, 105, 150, tft.color565(100, 100, 100));
			}
			if(desiredTemp == (maxTemp-1))
			{
				tft.fillTriangle(175, 150, 135, 195, 215, 195, ILI9341_WHITE);
			}
		}

		//temp-up
		if (x > 100 && x < 150 && y > 30 && y < 160 && ts.bufferSize()==bufferSize && desiredTemp != maxTemp)
		{
			Serial.println("temp up");
			tft.fillTriangle(175, 150, 135, 195, 215, 195, ILI9341_RED);
			desiredTemp+=1;
			delay(100);
			tft.fillTriangle(175, 150, 135, 195, 215, 195, ILI9341_WHITE);
			guiDrawDesiredTempature();
			if(desiredTemp == maxTemp)
			{
				tft.fillTriangle(175, 150, 135, 195, 215, 195,tft.color565(100, 100, 100));
			}
			if(desiredTemp == minTemp+1)
			{
				tft.fillTriangle(65, 195, 025, 150, 105, 150, ILI9341_WHITE);
			}
		}

		//heatingMode
		if(x > 170 && x < 220 && y > 215 && y < 300 && ts.bufferSize()==bufferSize)
		{
			heatingMode = !heatingMode;
			guiHeatingCoolingMode(); 
			burstMode = false;
			guiBurstMode();
			systemState = true;
			guiAutoStndbyMode();
		}

		//mode standby
		if(x > 170 && x < 220 && y > 110 && y < 215 && ts.bufferSize()==bufferSize)
		{
			systemState = !systemState;
			guiAutoStndbyMode();
			burstMode = false;
			guiBurstMode();
		}

		//BurstMode
		if(x > 170 && x < 220 && y > 20 && y < 110 && ts.bufferSize()==bufferSize)
		{
			Serial.println("Burst"); 
			burstMode = !burstMode;
			burstModeStart = 0;
			guiBurstMode();
		}

		if(systemState == false)
		{
			if(heatingMode == true && desiredTemp > curentTemp)
			{
				guiHeating();
				blowerHigh();
			}
			else if(heatingMode == true && desiredTemp == curentTemp)
			{
				guiWaiting();
				blowerLow();
			}
		}
		
		if(systemState == true)
		{
			guiWaiting();
			blowerLow();
		}
	
	}
	delay(100);
	timer+=1;
}