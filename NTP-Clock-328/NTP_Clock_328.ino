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

// Prototypes

void setup();
void loop();
void handleHTTPRequest(EthernetClient * client, String& data);
void setOffsetHandler (EthernetClient * client, String& variable, String& value);
void syncHandler (EthernetClient * client, String& variable, String& value);
void getHandler (EthernetClient * client, String& variable, String& value);

// TM1637 pinout (Digital Pins)
#define CLK 3
#define DIO 8

// Display instance
TM1637Display display(CLK, DIO);

// Ethernet stuff
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 0, 177 };
EthernetServer server(80);

// NTP provider
NTPClient * ntpProvider = NULL;

// IP of NTP server to fetch
#define NTP_IP "145.238.203.10"

// The NTP clock
NTPClock * ntpClock = NULL;

struct httpHandler
{
	String command;
	void  (* callback) (EthernetClient *, String&, String&);
};

struct httpHandler handlers[] =
{
	
	{"offset",	 &setOffsetHandler},
	{"sync",	 &syncHandler},
	{"get",		 &getHandler},
	{"",		 &getHandler}
	
	
};

void setOffsetHandler (EthernetClient * client, String& variable, String& value)
{
	ntpClock->setTimezoneOffset(value.toInt());
	
	client->println("HTTP/1.1 200 OK");
	client->println("Content-Type: text/json");
	client->println();
	
	client->print("{ ");
	client->print("status = \"OK\"");
	client->print(" }");
}

void syncHandler (EthernetClient * client, String& variable, String& value)
{
	ntpClock->synchronize();
	
	client->println("HTTP/1.1 200 OK");
	client->println("Content-Type: text/json");
	client->println();
	
	client->print("{ ");
	client->print("status = \"OK\"");
	client->print(" }");
}

void getHandler (EthernetClient * client, String& variable, String& value)
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

void setup()
{
	Serial.begin(115200);
	display.setBrightness(0x0f);
	
	Ethernet.begin(mac, ip);
	server.begin();
	
	ntpProvider = new NTPClient(NTP_IP);
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
	struct httpHandler handler;
	for ( unsigned int c = 0; c < sizeof(handlers); c++)
	{
		handler = handlers[c];
		
		if (key == handler.command)
		{
			handler.callback (client, key, value);
			
			return; // don't fall in ERROR_HANDLER
		}
		
	}
	
ERROR_HANDLER:
	client->println("HTTP/1.1 500 Internal Server Error");
	client->println();
}


boolean colon = false;

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
