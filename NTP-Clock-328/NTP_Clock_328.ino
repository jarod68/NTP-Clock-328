//
// NTP-Clock-328
//
// Description of the project
// Developed with [embedXcode](http://embedXcode.weebly.com)
//
// Author 		Matt
// 				Matthieu Holtz
//
// Date			15/07/2016 11:15
// Version		<#version#>
//
// Copyright	© Matt, 2016
// Licence		<#licence#>
//
// See         ReadMe.txt for references
//


// Core library for code-sense - IDE-based
#if defined(WIRING) // Wiring specific
    #include "Wiring.h"
#elif defined(MAPLE_IDE) // Maple specific
    #include "WProgram.h"
#elif defined(MPIDE) // chipKIT specific
    #include "WProgram.h"
#elif defined(DIGISPARK) // Digispark specific
    #include "Arduino.h"
#elif defined(ENERGIA) // LaunchPad specific
    #include "Energia.h"
#elif defined(LITTLEROBOTFRIENDS) // LittleRobotFriends specific
    #include "LRF.h"
#elif defined(MICRODUINO) // Microduino specific
    #include "Arduino.h"
#elif defined(SPARK) || defined(PARTICLE) // Particle / Spark specific
    #include "Arduino.h"
#elif defined(TEENSYDUINO) // Teensy specific
    #include "Arduino.h"
#elif defined(REDBEARLAB) // RedBearLab specific
    #include "Arduino.h"
#elif defined(ESP8266) // ESP8266 specific
    #include "Arduino.h"
#elif defined(ARDUINO) // Arduino 1.0 and 1.5 specific
    #include "Arduino.h"
#else // error
    #error Platform not defined
#endif // end IDE

#include <SPI.h>
#include <TM1637Display.h>
#include <Ethernet.h>

#include "NTPClock.h"
#include "NTPClient.h"

// Module connection pins (Digital Pins)
#define CLK 3
#define DIO 8

// The amount of time (in milliseconds) between tests
#define TEST_DELAY   2000

const uint8_t SEG_DONE[] = {
	SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
	SEG_C | SEG_E | SEG_G,                           // n
	SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};


TM1637Display display(CLK, DIO);


/************ ETHERNET STUFF ************/
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0, 177 };
EthernetServer server(80);

NTPClient * ntpProvider = NULL;
NTPClock * ntpClock = NULL;

// Add setup code
void setup()
{
	Serial.begin(115200);
	display.setBrightness(0x0f);
	
	
	// Debugging complete, we start the server!
	Ethernet.begin(mac, ip);
	server.begin();
	
	ntpProvider = new NTPClient("145.238.203.10");
	
	ntpClock = new NTPClock(ntpProvider);
	
}

void handleHTTPRequest(EthernetClient * client, String& data)
{

	String key = "";
	String value = "";

	boolean hasProperty = false;
	boolean hasValue = false;
	
	
	for (unsigned int i = 0; i< data.length(); i++)
	{
		char c = data.charAt(i);
		
		switch (c)
		{
			case '?':
				hasProperty = true;
				break;
				
			case '=' :
				hasValue = true;
				break;
				
			default:
				
				if (hasProperty && !hasValue)
					key+=c;
				else if (hasProperty && hasValue)
				{
					value +=c;
				}
				
				break;
		}
		
	}

	if(key == "hours" && value != "")
	{

		
		client->println("HTTP/1.1 503 OK");
		client->println("Content-Type: text/json");
		client->println();
		
		client->print("{ ");
		client->print("status = \"NOK\"");
		client->print(" }");

	}
	else if(key == "minutes" && value != "")
	{
		ntpClock->setTimezoneOffset(2);
		
		client->println("HTTP/1.1 503 OK");
		client->println("Content-Type: text/json");
		client->println();
		
		client->print("{ ");
		client->print("status = \"NOK\"");
		client->print(" }");
		
	}
	else if(key == "seconds" && value != "")
	{
		ntpClock->setTimezoneOffset(2);
		
		client->println("HTTP/1.1 503 OK");
		client->println("Content-Type: text/json");
		client->println();
		
		client->print("{ ");
		client->print("status = \"NOK\"");
		client->print(" }");
		
	}
	else if(key == "offset" && value != "")
	{
		ntpClock->setTimezoneOffset(value.toInt());
		
		client->println("HTTP/1.1 200 OK");
		client->println("Content-Type: text/json");
		client->println();
		
		client->print("{ ");
		client->print("status = \"OK\"");
		client->print(" }");
		
	}
	else if(key == "sync")
	{
		ntpClock->synchronize();
		
		client->println("HTTP/1.1 200 OK");
		client->println("Content-Type: text/json");
		client->println();
		
		client->print("{ ");
		client->print("status = \"OK\"");
		client->print(" }");
		
	}
	else
	{
		client->println("HTTP/1.1 200 OK");
		client->println("Content-Type: text/json");
		client->println();

		client->print("{ ");
		
		client->print("hours = ");
		client->print(ntpClock->getHours_UTC());
		
		client->print(" , minutes = ");
		client->print(ntpClock->getMinutes());
		
		client->print(" , seconds = ");
		client->print(ntpClock->getSeconds());
		
		client->print(" , offset = ");
		client->print(ntpClock->getTimezoneOffset());
		
		client->print(" }");
		
	}
	
}


boolean colon = false;

// Add loop code
void loop()
{
	
	unsigned int hours = ntpClock->getHours();
	unsigned int minutes = ntpClock->getMinutes();
	
	uint8_t d1 = display.encodeDigit(hours%10);
	hours/=10;
	uint8_t d2 = display.encodeDigit(hours%10);

	uint8_t d3 = display.encodeDigit(minutes%10);
	minutes/=10;
	uint8_t d4 = display.encodeDigit(minutes%10);
	
	uint8_t digits[] = {d2,d1,d4,d3};
	
	display.setSegments(digits, sizeof(digits), 0, colon);
	colon = !colon;
	
	delay(500);
	
	
	EthernetClient client = server.available();
	if (client) {
		// an http request ends with a blank line
		boolean requestLineParsed = false;
		
		String requestMethod = "";
		boolean requestMethodParsed = false;
		String requestValue = "";
		boolean requestValueParsed = false;
		
		while (client.connected()) {
			if (client.available()) {

				char c = client.read();
				//Serial.print(c);
				// if we've gotten to the end of the line (received a newline
				// character) and the line is blank, the http request has ended,
				// so we can send a reply
				if (requestMethodParsed && requestValueParsed)
				{
					
					handleHTTPRequest(&client, requestValue);
					
					break;
					
				}

					if (!requestMethodParsed)
					{
						if (c == ' ')
							requestMethodParsed = true;
						else
							requestMethod +=c;
					}
					else if (!requestValueParsed)
					{
						if (c == ' ')
							requestValueParsed = true;
						else
							requestValue +=c;
					}

			}
		}
		// give the web browser time to receive the data
		delay(1);
		client.stop();
	}
	
	
}
