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

// #define DEBUG 1 // Wheter or not to include DEBUGGING code

// #include <ESP8266WiFi.h>
#ifdef ESP32
#include <ESPmDNS.h>
// #include <Wifi.h>
#include <WebServer.h>
#else
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#endif
#include <ArduinoOTA.h>
#include <Ticker.h>

#include <DNSServer.h>
#include <WiFiManager.h>

#include "config.h"
#include "ledfunctions.h"
#include "brightness.h"
#include "ntp.h"
#include "iwebserver.h"
// #include "osapi.h"
#include "mqtt.h"


#define LED_RED		15
#define LED_GREEN	12
#define LED_BLUE	13
#ifndef ESP32
#define LED_BUILTIN	2
#endif

//---------------------------------------------------------------------------------------
// Network related variables
//---------------------------------------------------------------------------------------
int OTA_in_progress = 0;

//---------------------------------------------------------------------------------------
// Timer related variables
//---------------------------------------------------------------------------------------
#define TIMER_RESOLUTION 50

Ticker timer;
int lastSecond = -1;
//---------------------------------------------------------------------------------------
// Startup related variables
//---------------------------------------------------------------------------------------

bool startup = true;
bool RecoverFromException = false;

int hourglassState = 0;
int hourglassPrescaler = 0;

// int updateCountdown = 25;
int updateCountdown = 0;

bool NTPTimeAcquired=false;

//---------------------------------------------------------------------------------------
// Loop logic related variables
//---------------------------------------------------------------------------------------
unsigned long NextTick = 0;
uint8_t alarmstate = 255; // 255 means no triggered alarm, otherwise holds the currently active alarm
AlarmType alarmtype; // holds the current alarm type

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

  if (startup && not RecoverFromException) LED.process();
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
void NtpCallback(uint8_t _wd, uint8_t _h, uint8_t _m, uint8_t _s, uint8_t _ms)
{
	Serial.printf("NtpCallback(%2i,%2i,%2i,%2i,%2i)\n",_wd,_h,_m,_s,_ms);
  NTPTimeAcquired=true;


  // TODO: Stop startup interrupt
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
#ifndef ESP32
  // enable watchdog
  wdt_enable(WDTO_8S);
#endif

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

#ifndef ESP32
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
#endif

	// LEDs
  // Serial.println("Starting LED module");
#ifdef ESP32
  LED.begin(16);
#else
  LED.begin(3);
#endif
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
  wifiManager.setConnectTimeout(180);
  wifiManager.setHostname("Wordclock");

  if (!wifiManager.autoConnect(Config.hostname))
	{
		Serial.println("failed to connect, timeout");
		delay(1000);
#ifdef ESP32
    ESP.restart();
#else
		ESP.reset();
#endif
  }
	Serial.println("WiFi connected");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP()); 

	setLED(0, 1, 0);

	// OTA update
	Serial.println("Initializing OTA");
	ArduinoOTA.setPort(8266);
	ArduinoOTA.setHostname(Config.hostname);
	// ArduinoOTA.setPassword("WordClock");
	ArduinoOTA.onStart([]()
	{
    MQTT.PublishStatus("offline");
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
	iWebServer.begin();

  // MQTT
  MQTT.begin();

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
  bool AlarmInProgress = false;
  // handle NTP
  NTP.process();
  
  // do OTA update stuff
  ArduinoOTA.handle();

  // do web server stuff
  iWebServer.process();

  // do Config sutff
  Config.process();

  // do MQTT stuff
  MQTT.process();

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
      // get Current time
      unsigned long CurrentTime=timestamp(NTP.h,NTP.m,NTP.s,NTP.ms);
      
      // overrule any of the above in case of configured alarms
      for (int i=0;i<5;i++)
      {
        if (Config.alarm[i].enabled) {
          unsigned long StartTime=timestamp(Config.alarm[i].h,Config.alarm[i].m,0,0);
          unsigned long EndTime=StartTime+Config.alarm[i].duration*60000;  // Duration is in minutes, 1 min = 60.000 msec.

          if (EndTime<24*3600*1000) { // EndTime is the same day
            if (CurrentTime>=StartTime && CurrentTime<EndTime && ( Config.alarm[i].type==AlarmType::always || 
                                                                   Config.alarm[i].type==AlarmType::oneoff || 
                                                                  (Config.alarm[i].type==AlarmType::weekend && (NTP.weekday==0 || NTP.weekday==6)) ||
                                                                  (Config.alarm[i].type==AlarmType::workingdays && NTP.weekday>0 && NTP.weekday<6)))
            {
              Config.nightmode=false;
              LED.setMode(Config.alarm[i].mode);
              LED.AlarmProgress=(float) (CurrentTime-StartTime) / (float) (EndTime-StartTime); // Let the alarmhandling know how far we are in the progress
              AlarmInProgress=true;
              alarmstate=i;
              alarmtype=Config.alarm[i].type;
            }
          } else { // Endtime is the next day, in which case we have to handle the rollover at 0.00, so check for every start en endtime if it's the correct working day
            if (   (CurrentTime>StartTime && ( Config.alarm[i].type==AlarmType::always || // always 
                                             Config.alarm[i].type==AlarmType::oneoff ||  // always
                                            (Config.alarm[i].type==AlarmType::weekend && (NTP.weekday==0 || NTP.weekday==6)) || // Starttime in the evenings only on saturday and sunday
                                            (Config.alarm[i].type==AlarmType::workingdays && NTP.weekday>0 && NTP.weekday<6) ) ) // Starttime in the week only from mon-fri
                || (CurrentTime<EndTime-24*3600*1000 && ( Config.alarm[i].type==AlarmType::always || // always 
                                                          Config.alarm[i].type==AlarmType::oneoff ||  // always
                                                         (Config.alarm[i].type==AlarmType::weekend && (NTP.weekday==0 || NTP.weekday==6)) || // endtime in morning can only be sunday and monday morning
                                                         (Config.alarm[i].type==AlarmType::workingdays && NTP.weekday>1 && NTP.weekday<7))) ) // endtime in morning can only be tue-sat
            {
              Config.nightmode=false;
              LED.setMode(Config.alarm[i].mode);
              alarmstate=i;
              alarmtype=Config.alarm[i].type;
              if (CurrentTime>StartTime) {
                LED.AlarmProgress=(CurrentTime-StartTime)/(EndTime-StartTime); // Let the alarmhandling know how far we are in the progress
              } else {
                LED.AlarmProgress=(CurrentTime+24*3600*1000-StartTime)/(EndTime-StartTime); // Let the alarmhandling know how far we are in the progress
              }
              AlarmInProgress=true;
            }
          }
        }
      }

      if (not AlarmInProgress)
      {
        // Check if we have to deactivate a one-off alarm
        if (alarmstate!=255) {
          if (Config.alarm[alarmstate].type==AlarmType::oneoff) {
            Serial.printf("Deactivating alarm %i\r\n",alarmstate);
            Config.alarm[alarmstate].enabled=false;
            Config.saveDelayed();
            alarmstate=255;
          }
        }
        // display hourglass until time acquired from NTP Server
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
    if (not RecoverFromException) 
    {
      LED.process();
    }
      
  	// output current time if seconds value has changed
  	if (NTP.s != lastSecond)
  	{
      // blink onboard LED if heartbeat is enabled
#ifdef ESP32
      if (Config.heartbeat) digitalWrite(LED_BUILTIN, HIGH);
#else
      if (Config.heartbeat) digitalWrite(LED_BUILTIN, LOW);
#endif

  		lastSecond = NTP.s;
  
      // calc uptime
      int milliseconds  = millis();
      long seconds=milliseconds/1000;
      int msecs = milliseconds % 1000;
      int secs = seconds % 60;
      int mins = (seconds/60) % 60;
      int hrs = (seconds/3600) % 24;
      int days = (seconds/(3600*24));
  

#ifdef ESP32
      Serial.printf("%02i:%02i:%02i:%02i, filtered ADC=%i.%02, brightness=%i, Free Heap = %i, uptime=%i:%02i:%02i:%02i.%03i\r\n",
          NTP.weekday, NTP.h, NTP.m, NTP.s, (int)(Brightness.avg*100)%100, 
          Brightness.value(),ESP.getFreeHeap(), days,hrs,mins,secs,msecs);
#else
      Serial.printf("%02i:%02i:%02i:%02i, filtered ADC=%i.%02i, heap=%i, heap fragmentation=%i, Max Free Block Size = %i, Free Cont Stack = %i, brightness=%i, uptime=%i:%02i:%02i:%02i.%03i\r\n",
          NTP.weekday, NTP.h, NTP.m, NTP.s, (int)Brightness.avg, (int)(Brightness.avg*100)%100,
          ESP.getFreeHeap(), ESP.getHeapFragmentation(), ESP.getMaxFreeBlockSize(), ESP.getFreeContStack(), Brightness.value(),
          days,hrs,mins,secs,msecs);
#endif
      if (AlarmInProgress) {
        Serial.printf("Alarm in Progress at %2.2f%%\r\n",LED.AlarmProgress*100); 
      }
  	} else {
#ifdef ESP32
        digitalWrite(LED_BUILTIN, LOW);
#else 
        digitalWrite(LED_BUILTIN, LOW); 
#endif
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
#ifdef ESP32
        ESP.restart();
#else
  			ESP.reset();
#endif
  			break;
  
      case 'E':
        Serial.println("Startin exception");
        int *test;
        *test=5;
        Serial.println("test = "+String(*test));
        break;
  
#ifndef ESP32
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
#endif  
  
  		default:
  			Serial.printf("Unknown command '%c'\r\n", incoming);
  			break;
  		}
  	}
#endif
  } 
}
