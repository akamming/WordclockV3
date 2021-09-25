// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This project is the firmware for a Wordclock consisting of 114 WS2812B LEDs.
//  It implements:
//   - NTP client for time synchronization
//   - a web server for configuration access
//   - mDNS client for easy discovery
//   - class for easy LED access with fading
//   - OTA (over the air) updates
//   - WiFi manager for easy configuration in unknown WiFi networks
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Ticker.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include "ledfunctions.h"
#include "brightness.h"
#include "ntp.h"
#include "webserver.h"
#include "config.h"
#include "osapi.h"

#define DEBUG(...) Serial.printf(__VA_ARGS__);

#define LED_RED		15
#define LED_GREEN	12
#define LED_BLUE	13
#define LED_BUILTIN	2

//---------------------------------------------------------------------------------------
// Network related variables
//---------------------------------------------------------------------------------------
int OTA_in_progress = 0;

//---------------------------------------------------------------------------------------
// Timer related variables
//---------------------------------------------------------------------------------------
#define TIMER_RESOLUTION 10
#define HOURGLASS_ANIMATION_PERIOD 100
Ticker timer;
int h = 0;
int m = 0;
int s = 0;
int ms = 0;
int lastSecond = -1;
bool timeVarLock = false;
bool startup = true;

int hourglassState = 0;
int hourglassPrescaler = 0;

int updateCountdown = 25;

//---------------------------------------------------------------------------------------
// timerCallback
//
// Increments time, decrements timeout and NTP timer
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void timerCallback()
{
	// update time variables
	if (!timeVarLock)
	{
		timeVarLock = true;
		ms += TIMER_RESOLUTION;
		if (ms >= 1000)
		{
			ms -= 1000;
			if (++s > 59)
			{
				s = 0;
				if (++m > 59)
				{
					m = 0;
					if (++h > 23) h = 0;
				}
			}
		}
		timeVarLock = false;
	}

	// decrement delayed EEPROM config timer
	if(Config.delayedWriteTimer)
	{
		Config.delayedWriteTimer--;
		if(Config.delayedWriteTimer == 0) Config.delayedWriteFlag = true;
	}

	// blink onboard LED if heartbeat is enabled
	if (ms == 0 && Config.heartbeat) digitalWrite(LED_BUILTIN, LOW);
	else digitalWrite(LED_BUILTIN, HIGH);

	hourglassPrescaler += TIMER_RESOLUTION;
	if (hourglassPrescaler >= HOURGLASS_ANIMATION_PERIOD)
	{
		hourglassPrescaler -= HOURGLASS_ANIMATION_PERIOD;
		if (++Config.hourglassState >= HOURGLASS_ANIMATION_FRAMES)
			Config.hourglassState = 0;

		// trigger LED processing for hourglass during startup
		if(startup) LED.process();
	}
}

//---------------------------------------------------------------------------------------
// configModeCallback
//
// ...
//
// ->
// <- --
//---------------------------------------------------------------------------------------
void configModeCallback(WiFiManager *myWiFiManager)
{
	LED.setMode(DisplayMode::wifiManager);
	Serial.println("Entered config mode");
	Serial.println(WiFi.softAPIP());
	Serial.println(myWiFiManager->getConfigPortalSSID());
}

//---------------------------------------------------------------------------------------
// NtpCallback
//
// Is called by the NTP class upon successful reception of an NTP data packet. Updates
// the global hour, minute, second and millisecond values.
//
// ->
// <- --
//---------------------------------------------------------------------------------------
void NtpCallback(uint8_t _h, uint8_t _m, uint8_t _s, uint8_t _ms)
{
	Serial.println("NtpCallback()");

	// wait if timer variable lock is set
	while (timeVarLock) delay(1);

	// lock timer variables to prevent changes during interrupt
	timeVarLock = true;
	h = _h;
	m = _m;
	s = _s;
	ms = _ms;
	timeVarLock = false;
}

void setLED(unsigned char r, unsigned char g, unsigned char b)
{
	digitalWrite(LED_RED, r);
	digitalWrite(LED_GREEN, g);
	digitalWrite(LED_BLUE, b);
}

//---------------------------------------------------------------------------------------
// setup
//
// Initializes everything
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void setup()
{
	// ESP8266 LED
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);

	setLED(1, 0, 0);

	// serial port
	Serial.begin(115200);
	Serial.println();
	Serial.println();
	Serial.println("ESP8266 WordClock setup() begin");

	Serial.print("ESP.getResetReason(): ");
	Serial.println(ESP.getResetReason());
	Serial.print("ESP.getResetInfo(): ");
	Serial.println(ESP.getResetInfo());

	system_set_os_print(0);

	// timer
	Serial.println("Starting timer");
	timer.attach(TIMER_RESOLUTION / 1000.0, timerCallback);

	// configuration
	Serial.println("Loading configuration");
	Config.begin();
//	Config.reset();
//	Config.save();

	// LEDs
	Serial.println("Starting LED module");
	LED.begin(D6);
	LED.setMode(DisplayMode::yellowHourglass);

	// WiFi
	Serial.println("Initializing WiFi");
	WiFiManager wifiManager;
	wifiManager.setAPCallback(configModeCallback);
	if (!wifiManager.autoConnect("WordClock"))
	{
		Serial.println("failed to connect, timeout");
		delay(1000);
		ESP.reset();
	}
/*	WiFi.mode(WIFI_STA);
	WiFi.begin("Funkturm", "*******");
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	Serial.println("");*/
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());

	setLED(0, 0, 1);

	// OTA update
	Serial.println("Initializing OTA");
	ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname("WordClock" );
	ArduinoOTA.setPassword("WordClock");
	ArduinoOTA.onStart([]()
	{
		LED.setMode(DisplayMode::update);
		Config.updateProgress = 0;
    Config.nightmode=false;
		OTA_in_progress = 1;
		Serial.println("OTA Start");
	});
	ArduinoOTA.onEnd([]()
	{
		LED.setMode(DisplayMode::updateComplete);
		Serial.println("\nOTA End");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
	{
		LED.setMode(DisplayMode::update);
		Config.updateProgress = (progress) * 110 / total;
		Serial.printf("OTA Progress: %u%%\r\n", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error)
	{
		LED.setMode(DisplayMode::updateError);
		Serial.printf("OTA Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();

	// NTP
	Serial.println("Starting NTP module");
	NTP.begin(Config.ntpserver, NtpCallback, 1, true);

	// web server
	Serial.println("Starting HTTP server");
	WebServer.begin();

//	telnetServer.begin();
//	telnetServer.setNoDelay(true);

	startup = false;
	Serial.println("Startup complete.");
}

//-----------------------------------------------------------------------------------
// loop
//-----------------------------------------------------------------------------------
void loop()
{
	delay(10);

	// do OTA update stuff
	ArduinoOTA.handle();

	// update LEDs
	LED.setBrightness(Brightness.value());
	LED.setTime(h, m, s, ms);
	LED.process();

	// do not continue if OTA update is in progress
	// OTA callbacks drive the LED display mode and OTA progress
	// in the background, the above call to LED.process() ensures
	// the OTA status is output to the LEDs
	if (OTA_in_progress)
		return;

	// show the hourglass animation with green corners for the first 2.5 seconds
	// after boot to be able to reflash with OTA during that time window if
	// the firmware hangs afterwards
	if(updateCountdown)
	{
		setLED(0, 1, 0);
		LED.setMode(DisplayMode::greenHourglass);
		Serial.print(".");
		delay(100);
		updateCountdown--;
		if(updateCountdown == 0)
		{
			LED.setMode(Config.defaultMode);
			setLED(0, 0, 0);
		}
		return;
	}

	// set mode depending on current time
	/*if (Config.nightmode) LED.setMode(Config.defaultMode);
	else if(h == 13 && m == 37) LED.setMode(DisplayMode::matrix);
	else if(h == 19 && m == 00) LED.setMode(DisplayMode::matrix);
	else if(h == 20 && m == 00) LED.setMode(DisplayMode::plasma);
	else if(h == 21 && m == 00) LED.setMode(DisplayMode::fire);
	else if(h == 22 && m == 00) LED.setMode(DisplayMode::heart);
	else if(h == 23 && m == 00) LED.setMode(DisplayMode::stars);
	else LED.setMode(Config.defaultMode);*/

  LED.setMode(Config.defaultMode);
  bool AlarmInProgress = false;
  for (int i=0;i<5;i++)
  {
    if (Config.alarm[i].enabled) {
      if (h==Config.alarm[i].h && m==Config.alarm[i].m) {
        Config.nightmode=false;
        LED.setMode(Config.alarm[i].mode);
      }
    }
  }

	// do web server stuff
	WebServer.process();

	// save configuration to EEPROM if necessary
	if(Config.delayedWriteFlag)
	{
		DEBUG("Config timer expired, writing configuration.\r\n");
		Config.delayedWriteFlag = false;
		Config.save();
	}

	// output current time if seconds value has changed
	if (s != lastSecond)
	{
		lastSecond = s;
		DEBUG("%02i:%02i:%02i, filtered ADC=%i.%02i, heap=%i, brightness=%i\r\n",
			  h, m, s, (int)Brightness.avg, (int)(Brightness.avg*100)%100,
			  ESP.getFreeHeap(), Brightness.value());
	}

	if (Serial.available())
	{
		int incoming = Serial.read();
		switch (incoming)
		{
		case 'i':
			Serial.println("WordClock ESP8266 ready.");
			break;

		case 'X':
			WiFi.disconnect();
			ESP.reset();
			break;

		default:
			Serial.printf("Unknown command '%c'\r\n", incoming);
			break;
		}
	}
}
// ./esptool.py --port /dev/tty.usbserial --baud 460800 write_flash --flash_size=8m 0 /var/folders/yh/bv744591099f3x24xbkc22zw0000gn/T/build006b1a55228a1b90dda210fcddb62452.tmp/test.ino.bin
// FlashSize 1M (128k SPIFFS)
// C:\Python27\python.exe "C:\Program Files\esptool\espota.py" --ip=192.168.178.95 --port=8266 --progress --file=${workspace_loc}\${project_path}\Release\${project_name}.bin
