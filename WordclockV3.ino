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
#define TIMER_RESOLUTION 100
#define HOURGLASS_ANIMATION_PERIOD 100
#define TICKTIME 20 // no of millisecs between clock display updates

Ticker timer;
int h = 0;
int m = 0;
int s = 0;
int ms = 0;
unsigned long CurrentTime=0;

int lastSecond = -1;
bool timeVarLock = false;
bool startup = true;
bool RecoverFromException = false;

int hourglassState = 0;
int hourglassPrescaler = 0;

// int updateCountdown = 25;
int updateCountdown = 0;

bool NTPTimeAcquired=false;

unsigned long NextTick = 0;

//---------------------------------------------------------------------------------------
// timestamp()
//
// converts h,m,s,ms to usigned long with no of msec sinds 0:00:00.000
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
unsigned long timestamp(int _h, int _m, int _s, int _ms)
{
  return (_h*3600 + _m*60 + _s) * 1000 + _ms;
}


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
    CurrentTime=timestamp(h,m,s,ms);
		timeVarLock = false;
	}

	// decrement delayed EEPROM config timer
	if(Config.delayedWriteTimer)
	{
		Config.delayedWriteTimer--;
		if(Config.delayedWriteTimer == 0) Config.delayedWriteFlag = true;
	}

	hourglassPrescaler += TIMER_RESOLUTION;
	if (hourglassPrescaler >= HOURGLASS_ANIMATION_PERIOD)
	{
		hourglassPrescaler -= HOURGLASS_ANIMATION_PERIOD;
		if (++Config.hourglassState >= HOURGLASS_ANIMATION_FRAMES)
			Config.hourglassState = 0;

		// trigger LED processing for hourglass during startup
		if(startup && not RecoverFromException) LED.process();
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
  LED.process();
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
  NTPTimeAcquired=true;

	// wait if timer variable lock is set
	while (timeVarLock) delay(1);

	// lock timer variables to prevent changes during interrupt
	timeVarLock = true;
	h = _h;
	m = _m;
	s = _s;
	ms = _ms;
  CurrentTime=timestamp(h,m,s,ms);
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
  // enable watchdog
  wdt_enable(WDTO_8S);

	// ESP8266 LED
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);

	setLED(1, 0, 0);

  // configuration
  // Serial.println("Loading configuration");
  Config.begin();

  
	// serial port
	Serial.begin(115200);
	Serial.println();
	Serial.println();
	Serial.println("ESP8266 WordClock setup() begin");

	Serial.print("ESP.getResetReason(): ");
	Serial.println(ESP.getResetReason());
	Serial.print("ESP.getResetInfo(): ");
	Serial.println(ESP.getResetInfo());

 

  rst_info* rtc_info = ESP.getResetInfoPtr();
  Serial.println(rtc_info->reason);

  if  (rtc_info->reason ==  REASON_WDT_RST  ||
       rtc_info->reason ==  REASON_EXCEPTION_RST  ||
       rtc_info->reason ==  REASON_SOFT_WDT_RST)
       // rtc_info->reason ==  REASON_SOFT_RESTART)
  {
     RecoverFromException = true; // Let the program know we are recovering
     if (rtc_info->reason ==  REASON_EXCEPTION_RST) 
     {
       Serial.printf("Fatal exception (%d):\n", rtc_info->exccause);
     }
     Serial.printf("epc1=0x%08x,  epc2=0x%08x,  epc3=0x%08x,  excvaddr=0x%08x,  depc=0x%08x\n",
                     rtc_info->epc1,  rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr,  rtc_info->depc);//The address of  the last  crash is  printed,  which is  used  to debug garbled output.
  }

	// LEDs
  // Serial.println("Starting LED module");
  LED.begin(D6);
  if (not RecoverFromException) 
  { 
    LED.setMode(DisplayMode::yellowHourglass);
    LED.process();
  }
  
  // timer
	Serial.println("Starting timer");
	timer.attach(TIMER_RESOLUTION / 1000.0, timerCallback);
  
	// WiFi
	Serial.println("Initializing WiFi");
  WiFi.setAutoReconnect(true);
  // WiFi.mode(WIFI_STA); //WiFi mode station (connect to wifi router only)
  // WiFi.setSleepMode(WIFI_NONE_SLEEP);
  // WiFi.persistent(true);
	WiFiManager wifiManager;
	wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect(Config.hostname))
	{
		Serial.println("failed to connect, timeout");
		delay(1000);
		ESP.reset();
	}
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());

	setLED(0, 0, 1);

	// OTA update
	Serial.println("Initializing OTA");
	ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname(Config.hostname);
	// ArduinoOTA.setPassword("WordClock");
	ArduinoOTA.onStart([]()
	{
		LED.setMode(DisplayMode::update);
		Config.updateProgress = 0;
    Config.nightmode=false;
		OTA_in_progress = 1;
    LED.process();
		Serial.println("OTA Start");
	});
	ArduinoOTA.onEnd([]()
	{
		LED.setMode(DisplayMode::updateComplete);
    LED.process();
		Serial.println("\nOTA End");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
	{
		LED.setMode(DisplayMode::update);
		Config.updateProgress = (progress) * 110 / total;
    LED.process();
    Serial.printf("OTA Progress: %u%%\r\n", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error)
	{
		LED.setMode(DisplayMode::updateError);
    LED.process();
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

  // Set NextTick to now
  NextTick=millis();
}

//-----------------------------------------------------------------------------------
// loop
//-----------------------------------------------------------------------------------
void loop()
{
  // handle NTP
  NTP.process();
  
  // do OTA update stuff
  ArduinoOTA.handle();

  // do web server stuff
  WebServer.process();

  // save configuration to EEPROM if necessary
  if(Config.delayedWriteFlag)
  {
    DEBUG("Config timer expired, writing configuration.\r\n");
    Config.delayedWriteFlag = false;
    Config.save();
  }

  // Only process LED functions every TICKTIME mseconds 
  if (millis()>=NextTick) 
  {
    // increase Next Tick
    NextTick = millis()+TICKTIME;  
  
  	// do not continue if OTA update is in progress
  	// OTA callbacks drive the LED display mode and OTA progress
  	// in the background, the above call to LED.process() ensures
  	// the OTA status is output to the LEDs
  	if (OTA_in_progress==0) 
    {
  
    	// show the hourglass animation with green corners for configured time
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
    	} else {    
       
        // overrule any of the above in case of configured alarms
        bool AlarmInProgress = false;
        for (int i=0;i<5;i++)
        {
          if (Config.alarm[i].enabled) {
            if (h==Config.alarm[i].h && m==Config.alarm[i].m) {
              Config.nightmode=false;
              LED.setMode(Config.alarm[i].mode);
              AlarmInProgress = true;
            }
          }
        }
      
        // display hourglass until time acquired from NTP Server
        if (not AlarmInProgress)
        {
          if (NTPTimeAcquired){
            if (RecoverFromException) {
              RecoverFromException=false;
              LED.setMode(DisplayMode::plain);
            } else {
              LED.setMode(Config.defaultMode);
            }
          } else {
            LED.setMode(DisplayMode::greenHourglass);
          }
        }
    	}
    
      // update LEDs
      LED.setBrightness(Brightness.value());
      LED.setTime(h, m, s, ms);
      if (not RecoverFromException) 
      {
        LED.process();
#ifdef NEOPIXELBUS
        // Make sure command is finished before new code is executed
        while (LED.strip->CanShow()==false)
        {
          delay(1);
        }
#endif
      }
        
    	// output current time if seconds value has changed
    	if (s != lastSecond)
    	{
        // blink onboard LED if heartbeat is enabled
        if (Config.heartbeat) digitalWrite(LED_BUILTIN, LOW);

    		lastSecond = s;
    
        // calc uptime
        int milliseconds  = millis();
        long seconds=milliseconds/1000;
        int msecs = milliseconds % 1000;
        int secs = seconds % 60;
        int mins = (seconds/60) % 60;
        int hrs = (seconds/3600) % 24;
        int days = (seconds/(3600*24));
    
    
    		DEBUG("%02i:%02i:%02i, filtered ADC=%i.%02i, heap=%i, heap fragmentation=%i, Max Free Block Size = %i, brightness=%i, uptime=%i:%02i:%02i:%02i.%03i\r\n",
    			  h, m, s, (int)Brightness.avg, (int)(Brightness.avg*100)%100,
    			  ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize(), Brightness.value(),
    			  days,hrs,mins,secs,msecs);
    	} else {
          digitalWrite(LED_BUILTIN, HIGH); 
    	}
    

#ifndef NEOPIXELBUS
      // does not work with NEOPIXELBUS, due to DMA method conflicting with incoming Serial
    	if (Serial.available())
    	{
    		int incoming = Serial.read();
    		switch (incoming)
    		{
    		case 'i':
    			Serial.println("WordClock ESP8266 ready.");
    			break;
    
    		case 'X':
    			// WiFi.disconnect();
    			ESP.reset();
    			break;
    
        case 'E':
          Serial.println("Startin exception");
          int *test;
          *test=5;
          Serial.println("test = "+String(*test));
          break;
    
        case 'S':
          Serial.println("Trigger software watchdog");
          wdt_disable();
          wdt_enable(WDTO_15MS);
          while (1) {}
          break;
    
        case 'H':
          Serial.println("Trigger hardware watchdog");
          wdt_disable();
          while (1) {}
          break;
    
    
    		default:
    			Serial.printf("Unknown command '%c'\r\n", incoming);
    			break;
    		}
    	}
#endif

    }
  } 
}
// ./esptool.py --port /dev/tty.usbserial --baud 460800 write_flash --flash_size=8m 0 /var/folders/yh/bv744591099f3x24xbkc22zw0000gn/T/build006b1a55228a1b90dda210fcddb62452.tmp/test.ino.bin
// FlashSize 1M (128k SPIFFS)
// C:\Python27\python.exe "C:\Program Files\esptool\espota.py" --ip=192.168.178.95 --port=8266 --progress --file=${workspace_loc}\${project_path}\Release\${project_name}.bin
