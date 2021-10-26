// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This module encapsulates a small webserver. It replies to requests on port 80
//  and triggers actions, manipulates configuration attributes or serves files
//  from the internal flash file system.
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
#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>

#include "ledfunctions.h"
#include "brightness.h"
#include "webserver.h"
#include "ntp.h"
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#ifdef DEBUG
#include "EspSaveCrash.h"
#endif

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
WebServerClass WebServer = WebServerClass();
#ifdef DEBUG
EspSaveCrash SaveCrash;
#endif
//---------------------------------------------------------------------------------------
// WebServerClass
//
// Constructor, currently empty
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
WebServerClass::WebServerClass()
{
}

//---------------------------------------------------------------------------------------
// ~WebServerClass
//
// Destructor, removes allocated web server object
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
WebServerClass::~WebServerClass()
{
	if (this->server)
		delete this->server;
}

//---------------------------------------------------------------------------------------
// begin
//
// Sets up internal handlers and starts the server at port 80
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::begin()
{
	SPIFFS.begin();

	this->server = new ESP8266WebServer(80);
	this->server->on("/setcolor", std::bind(&WebServerClass::handleSetColor, this));
	this->server->on("/info", std::bind(&WebServerClass::handleInfo, this));
	this->server->on("/saveconfig", std::bind(&WebServerClass::handleSaveConfig, this));
	this->server->on("/loadconfig", std::bind(&WebServerClass::handleLoadConfig, this));
	this->server->on("/setheartbeat", std::bind(&WebServerClass::handleSetHeartbeat, this));
	this->server->on("/getheartbeat", std::bind(&WebServerClass::handleGetHeartbeat, this));
	this->server->on("/getcolors", std::bind(&WebServerClass::handleGetColors, this));
	this->server->on("/getntpserver", std::bind(&WebServerClass::handleGetNtpServer, this));
	this->server->on("/setntpserver", std::bind(&WebServerClass::handleSetNtpServer, this));
	this->server->on("/h", std::bind(&WebServerClass::handleH, this));
	this->server->on("/m", std::bind(&WebServerClass::handleM, this));
	this->server->on("/r", std::bind(&WebServerClass::handleR, this));
	this->server->on("/g", std::bind(&WebServerClass::handleG, this));
	this->server->on("/b", std::bind(&WebServerClass::handleB, this));
  this->server->on("/getbrightness", std::bind(&WebServerClass::handleGetBrightness, this));
  this->server->on("/setbrightness", std::bind(&WebServerClass::handleSetBrightness, this));
	this->server->on("/getadc", std::bind(&WebServerClass::handleGetADC, this));
	this->server->on("/setmode", std::bind(&WebServerClass::handleSetMode, this));
	this->server->on("/getmode", std::bind(&WebServerClass::handleGetMode, this));
	this->server->on("/settimezone", std::bind(&WebServerClass::handleSetTimeZone, this));
	this->server->on("/gettimezone", std::bind(&WebServerClass::handleGetTimeZone, this));
  this->server->on("/debug", std::bind(&WebServerClass::handleDebug, this));
  this->server->on("/resetwificredentials", std::bind(&WebServerClass::handleResetWifiCredentials, this));
  this->server->on("/factoryreset", std::bind(&WebServerClass::handleFactoryReset, this));
  this->server->on("/reset", std::bind(&WebServerClass::handleReset, this));
  this->server->on("/setnightmode", std::bind(&WebServerClass::handleSetNightMode, this));
  this->server->on("/getnightmode", std::bind(&WebServerClass::handleGetNightMode, this));
  this->server->on("/getconfig", std::bind(&WebServerClass::handleGetConfig, this));
  this->server->on("/getalarms", std::bind(&WebServerClass::handleGetAlarms, this));
  this->server->on("/setalarm", std::bind(&WebServerClass::handleSetAlarm, this));
  
#ifdef DEBUG
  this->server->on("/showcrashlog", std::bind(&WebServerClass::handleShowCrashLog, this));
  this->server->on("/clearcrashlog", std::bind(&WebServerClass::handleClearCrashLog, this));
#endif

	this->server->onNotFound(std::bind(&WebServerClass::handleNotFound, this));

  // Generic code which passess all webrequeststs.
  this->server->addHook([](const String & method, const String & url, WiFiClient * client, ESP8266WebServer::ContentTypeFunction contentType) {
    /* (void)method;      // GET, PUT, ...
    (void)url;         // example: /root/myfile.html
    (void)client;      // the webserver tcp client connection
    (void)contentType; // contentType(".html") => "text/html" */

    Serial.printf("%s called\n\r",url.c_str());
    WebServer.lastAction=millis();
    
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });
 
	this->server->begin();
}

//---------------------------------------------------------------------------------------
// endsWith
//
// checks if strings ends with another string
//
// -> filename: name of the file
// <- HTML content type matching file extension
//---------------------------------------------------------------------------------------
bool WebServerClass::endsWith(const char* what, const char* withwhat)
{
    int l1 = strlen(withwhat);
    int l2 = strlen(what);
    if (l1 > l2)
        return 0;

    return strcmp(withwhat, what + (l2 - l1)) == 0;
}


//---------------------------------------------------------------------------------------
// process
//
// Must be called repeatedly from main loop
//
// ->
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::process()
{
	this->server->handleClient();
}

//---------------------------------------------------------------------------------------
// serveFile
//
// Looks up a given file name in internal flash file system, streams the file if found
//
// -> path: name of the file; "index.html" will be added if name ends with "/"
// <- true: file was found and served to client
//	false: file not found
//---------------------------------------------------------------------------------------
bool WebServerClass::serveFile(const char url[])
{
	Serial.printf("WebServerClass::serveFile(): %s\n\rE",url);

  char path[50];
  
	if (url[strlen(url)-1]=='/') {
		sprintf (path,"%sindex.html",url);
	} else {
    sprintf(path,"%s",url);
	}
	if (SPIFFS.exists(path))
	{
		File file = SPIFFS.open(path, "r");
    if (this->server->hasArg("download")) this->server->streamFile(file, "application/octet-stream");
		else if (this->endsWith(path,".htm") or this->endsWith(path,".html")) this->server->streamFile(file, "text/html");
    else if (this->endsWith(path,".css") ) this->server->streamFile(file, "text/css");
    else if (this->endsWith(path,".png") ) this->server->streamFile(file, "image/png");
    else if (this->endsWith(path,".gif") ) this->server->streamFile(file, "image/gif");
    else if (this->endsWith(path,".jpg") ) this->server->streamFile(file, "image/jpeg");
    else if (this->endsWith(path,".ico") ) this->server->streamFile(file, "image/x-icon");
    else if (this->endsWith(path,".xml") ) this->server->streamFile(file, "text/xml");
    else if (this->endsWith(path,".pdf") ) this->server->streamFile(file, "application./x-pdf");
    else if (this->endsWith(path,".zip") ) this->server->streamFile(file, "application./x-zip");
    else if (this->endsWith(path,".gz") ) this->server->streamFile(file, "application./x-gzip");
    else this->server->streamFile(file, "text/plain");
		file.close();
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------------------
// handleFactoryReset
//
// Handles the /factoryreset request, reset wifi credentials, reset config and reboot
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleFactoryReset() 
{
  Serial.println("Reset Config");
  Config.reset();
  Config.save();

  Serial.println("Resetting Wifi Credentials");
  WiFiManager wifiManager;
  wifiManager.resetSettings(); 
  this->server->send(200, "text/plain", "OK"); 

  delay(500); // wait half a second, then reset

  ESP.reset();
}

//---------------------------------------------------------------------------------------
// handleReset
//
// Handles the /reset request, just reboot the device
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleReset() 
{
  Serial.println("Manual reboot");

  this->server->send(200, "text/plain", "OK"); 
  
  delay(500); // wait half a second, then reset
  ESP.reset();
}


//---------------------------------------------------------------------------------------
// handleResetWifiCredentials
//
// Handles the /resetWifiCredentials request, reset wifi credentials and reboot, so wifimanager will take over
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleResetWifiCredentials() 
{
  Serial.println("Resetting Wifi Credentials");
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  this->server->send(200, "text/plain", "OK");
  ESP.reset();
}


//---------------------------------------------------------------------------------------
// handleM
//
// Handles the /m request, increments the minutes counter (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
extern int h, m;
void WebServerClass::handleM()
{
	if(++m>59) m = 0;
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleH
//
// Handles the /h request, increments the hours counter (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleH()
{
	if(++h>23) h = 0;
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleR
//
// Handles the /r request, sets LED matrix to all red (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleR()
{
	LED.setMode(DisplayMode::red);
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleG
//
// Handles the /g request, sets LED matrix to all green (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleG()
{
	LED.setMode(DisplayMode::green);
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleB
//
// Handles the /b request, sets LED matrix to all blue (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleB()
{
	LED.setMode(DisplayMode::blue);
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleGetBrightness
//
// Handles the /getbrightness request, returns the brightness value
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetBrightness()
{
  char buf[5];
  sprintf(buf,"%d",Brightness.brightnessOverride);
  this->server->send(200, "text/plain", buf);
}


//---------------------------------------------------------------------------------------
// handleSetBrightness
//
// Handles the /setbrightness request, sets LED matrix to selected brightness, 256 = auto
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetBrightness()
{
	if(this->server->hasArg("value"))
	{
    Serial.println("SetBrightness");
		Brightness.brightnessOverride = this->server->arg("value").toInt();
    Config.save();
		this->server->send(200, "text/plain", "OK");
	}
}

//---------------------------------------------------------------------------------------
// handleNightMode
//
// Handles the /setnightmode request, sets LED matrix to night mode palette
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetNightMode()
{
  Serial.println("SetNightMode");
  if(this->server->hasArg("value"))
  {
    Config.nightmode = (this->server->hasArg("value") && this->server->arg("value") == "1");
    Config.saveDelayed();
    this->server->send(200, "text/plain", "OK");
  }
}

//---------------------------------------------------------------------------------------
// handleGetNightmode
//
// Returns the state of the heartbeat flag.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetNightMode()
{
  if(Config.nightmode) this->server->send(200, "text/plain", "1");
  else this->server->send(200, "text/plain", "0");
}



void WebServerClass::handleDebug()
{
	if(this->server->hasArg("led") &&
			   this->server->hasArg("r") &&
			   this->server->hasArg("g") &&
			   this->server->hasArg("b"))
	{
		int led = this->server->arg("led").toInt();
		int r = this->server->arg("r").toInt();
		int g = this->server->arg("g").toInt();
		int b = this->server->arg("b").toInt();
		if(led < 0) led = 0;
		if(led >= NUM_PIXELS) led = NUM_PIXELS - 1;
		if(r < 0) r = 0;
		if(r > 255) r = 255;
		if(g < 0) g = 0;
		if(g > 255) g = 255;
		if(b < 0) b = 0;
		if(b > 255) b = 255;

		LED.currentValues[led*3+0] = r;
		LED.currentValues[led*3+1] = g;
		LED.currentValues[led*3+2] = b;
		LED.show();
		Config.debugMode = 1;
	}

	if(this->server->hasArg("clear"))
	{
		for(int i=0; i<3*NUM_PIXELS; i++) LED.currentValues[i] = 0;
		LED.show();
	}

	if(this->server->hasArg("end"))
	{
		Config.debugMode = 0;
	}
	this->server->send(200, "text/plain", "OK");
}

void WebServerClass::handleGetADC()
{
  
	int __attribute__ ((unused)) temp = Brightness.value(); // to trigger A/D conversion

  char buf[5];
  sprintf(buf,"%s",Brightness.avg);
	this->server->send(200, "text/plain", buf);
}

//---------------------------------------------------------------------------------------
// handleSetTimeZone
//
// Handles the /settimezone request. Saves the time zone
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetTimeZone()
{
  int newTimeZone = -999;
	if(this->server->hasArg("value"))
	{
		newTimeZone = this->server->arg("value").toInt();
		if(newTimeZone < - 12 || newTimeZone > 14)
		{
			this->server->send(400, "text/plain", "ERR");
		}
		else
		{
			Config.timeZone = newTimeZone;
			Config.save();
			NTP.setTimeZone(Config.timeZone);
			this->server->send(200, "text/plain", "OK");
		}
	}
}

//---------------------------------------------------------------------------------------
// handleGetTimeZone
//
// Handles the /gettimezone request, delivers offset to UTC in hours.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetTimeZone()
{

  char buf[3];
  sprintf(buf,"%u",Config.timeZone);
	this->server->send(200, "text/plain", buf);
}

//---------------------------------------------------------------------------------------
// handleSetMode
//
// Handles the /setmode request. Sets the display mode to one of the allowed values,
// saves it as the new default mode.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetMode()
{
	DisplayMode mode = DisplayMode::invalid;

	if(this->server->hasArg("value"))
	{
		// handle each allowed value for safety
		if(this->server->arg("value") == "0") mode = DisplayMode::plain;
		if(this->server->arg("value") == "1") mode = DisplayMode::fade;
		if(this->server->arg("value") == "2") mode = DisplayMode::flyingLettersVerticalUp;
		if(this->server->arg("value") == "3") mode = DisplayMode::flyingLettersVerticalDown;
		if(this->server->arg("value") == "4") mode = DisplayMode::explode;
	}

	if(mode == DisplayMode::invalid)
	{
		this->server->send(400, "text/plain", "ERR");
	}
	else
	{
		LED.setMode(mode);
		Config.defaultMode = mode;
		Config.save();
		this->server->send(200, "text/plain", "OK");
	}
}

//---------------------------------------------------------------------------------------
// handleGetMode
//
// Handles the /getmode request and returns the current default display mode.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetMode()
{
	char mode[5] = "";
	switch(Config.defaultMode)
	{
	case DisplayMode::plain:
		strcat(mode,"0"); break;
	case DisplayMode::fade:
		strcat(mode,"1"); break;
	case DisplayMode::flyingLettersVerticalUp:
		strcat(mode,"2"); break;
	case DisplayMode::flyingLettersVerticalDown:
		strcat(mode,"3"); break;
	case DisplayMode::explode:
		strcat(mode,"4"); break;
	default:
		strcat(mode,"0"); break;
	}
	this->server->send(200, "text/plain", mode);
}

//---------------------------------------------------------------------------------------
// handleNotFound
//
// Handles all requests not bound to other handlers, tries to serve a file if found in
// flash, responds with 404 otherwise
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleNotFound()
{

	// first, try to serve the requested file from flash
	if (!serveFile(this->server->uri().c_str()))
	{
    // create 404 message if no file was found for this URI
    char message[1000];
    sprintf(message,"File Not Found\n\nURI: %s\nMethod: %s\nArguments %d\n",
        this->server->uri().c_str(), this->server->method() == HTTP_GET ? "GET" : "POST",
        this->server->args());
		for (uint8_t i = 0; i < this->server->args(); i++)
		{
      char buf[100];
      sprintf(buf," %s,%s\n\r",this->server->argName(i).c_str(),this->server->arg(i).c_str());
      strcat (message,buf);
		}
		this->server->send(404, "text/plain", message);
	}
}

//---------------------------------------------------------------------------------------
// handleGetNtpServer
//
// Delivers the currently configured NTP server IP address
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetNtpServer()
{

  char NTPServer[20];
  sprintf(NTPServer,"%u.%u.%u.%u",Config.ntpserver[0],Config.ntpserver[1],Config.ntpserver[2],Config.ntpserver[3]);

  // this->server->send(200, "application/json", Config.ntpserver.toString());
  this->server->send(200, "application/json", NTPServer);
}

//---------------------------------------------------------------------------------------
// handleSetNtpServer
//
// Sets a new IP address for the NTP client
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetNtpServer()
{
	if (this->server->hasArg("ip"))
	{
		IPAddress ip;
		if (ip.fromString(this->server->arg("ip")))
		{
			// set IP address in config
			Config.ntpserver = ip;
			Config.save();

			// set IP address in client
			NTP.setServer(ip);
		}
	}
	this->server->send(200, "application/json", "OK");
}

//---------------------------------------------------------------------------------------
// handleInfo
//
// Handles requests to "/info", replies with JSON structure containing system status
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleInfo()
{
	StaticJsonDocument<512> json;
	char buf[512];
	json["heap"] = ESP.getFreeHeap();
	json["sketchsize"] = ESP.getSketchSize();
	json["sketchspace"] = ESP.getFreeSketchSpace();
	json["cpufrequency"] = ESP.getCpuFreqMHz();
	json["chipid"] = ESP.getChipId();
	json["sdkversion"] = ESP.getSdkVersion();
	json["bootversion"] = ESP.getBootVersion();
	json["bootmode"] = ESP.getBootMode();
	json["flashid"] = ESP.getFlashChipId();
	json["flashspeed"] = ESP.getFlashChipSpeed();
	json["flashsize"] = ESP.getFlashChipRealSize();
	json["resetreason"] = ESP.getResetReason();
	json["resetinfo"] = ESP.getResetInfo();
  json["freeheap"] = ESP.getFreeHeap();
  json["configsize"] = Config.Configsize();

  int milliseconds  = millis();
  long seconds=milliseconds/1000;
  int msecs = milliseconds % 1000;
  int secs = seconds % 60;
  int mins = (seconds/60) % 60;
  int hrs = (seconds/3600) % 24;
  int days = (seconds/(3600*24));

  char buffer[50];
  sprintf(buffer, "%i days, %i hours, %i mins, %i seconds, %i milliseconds", days, hrs, mins, secs, msecs);
  json["uptime"] = buffer;
  
  serializeJson(json,buf);
	this->server->send(200, "application/json", buf);
}

//---------------------------------------------------------------------------------------
// extractColor
//
// Converts the given web server argument to a color struct
// -> argName: Name of the web server argument
//	result: Pointer to palette_entry struct to receive result
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::extractColor(char argName[], palette_entry& result)
{
	char c[3];


	if (this->server->hasArg(argName) && this->server->arg(argName).length() == 6)
	{
		String color = this->server->arg(argName);
		color.substring(0, 2).toCharArray(c, sizeof(c));
		result.r = strtol(c, NULL, 16);
		color.substring(2, 4).toCharArray(c, sizeof(c));
		result.g = strtol(c, NULL, 16);
		color.substring(4, 6).toCharArray(c, sizeof(c));
		result.b = strtol(c, NULL, 16);
	}
}

//---------------------------------------------------------------------------------------
// handleSetColor
//
// Handles the "/setcolor" request, expects arguments:
//	/setcolor?fg=xxxxxx&bg=yyyyyy&s=zzzzzz
//	with xxxxxx, yyyyyy and zzzzzz being hexadecimal HTML colors (without leading '#')
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetColor()
{
  this->extractColor("fg", Config.fg);
	this->extractColor("bg", Config.bg);
	this->extractColor("s", Config.s);
	this->server->send(200, "text/plain", "OK");
	Config.saveDelayed();
}

//---------------------------------------------------------------------------------------
// handleSaveConfig
//
// Saves the current configuration to EEPROM
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSaveConfig()
{
	Config.save();
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleLoadConfig
//
// Loads the current configuration from EEPROM
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleLoadConfig()
{
	Config.load();
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleSetHeartbeat
//
// Sets or resets the heartbeat flag in the configuration based on argument "state"
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetHeartbeat()
{
	Config.heartbeat = (this->server->hasArg("value") && this->server->arg("value") == "1");
	Config.save();
	this->server->send(200, "text/plain", "OK");
}

//---------------------------------------------------------------------------------------
// handleGetHeartbeat
//
// Returns the state of the heartbeat flag.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetHeartbeat()
{
	if(Config.heartbeat) this->server->send(200, "text/plain", "1");
	else this->server->send(200, "text/plain", "0");
}

//---------------------------------------------------------------------------------------
// handleGetColors
//
// Outputs the currently active colors as comma separated list for background, foreground
// and seconds color with 3 values each (red, green, blue)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetColors()
{
  char message[50];
  sprintf(message,"%u,%u,%u,%u,%u,%u,%u,%u,%u",Config.bg.r,Config.bg.g,Config.bg.b,
                                               Config.fg.r,Config.fg.g,Config.fg.b,
                                               Config.s.r, Config.s.g, Config.s.b);
  this->server->send(200, "text/plain", message);
}

//---------------------------------------------------------------------------------------
// handleGetAlarms
//
// gets the alarms
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetAlarms()
{
  char alarmmode[10];
  char message[256]="";

  for (int i=0;i<5;i++) {
    char displaymode[20];
    
    switch(Config.alarm[i].mode)
    {
    case DisplayMode::matrix:
      strcpy(displaymode,"matrix"); break;
    case DisplayMode::plasma:
      strcpy(displaymode,"plasma"); break;
    case DisplayMode::fire:
      strcpy(displaymode,"fire"); break;
    case DisplayMode::heart:
      strcpy(displaymode,"heart"); break;
    case DisplayMode::stars:
      strcpy(displaymode,"stars"); break;
    default:
      strcpy(displaymode,"unknown"); break;
    }

    // buildup alarmchararray
    char buffer[20];
    sprintf(buffer,"%02d:%02d,%s,%s",Config.alarm[i].h,Config.alarm[i].m,displaymode,Config.alarm[i].enabled ? "on," : "off,");
    strcat(message,buffer); 
  }
  this->server->send(200, "text/plain", message);

}

//---------------------------------------------------------------------------------------
// handleSetAlarm
//
// Sets an alarm
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetAlarm()
{
  if (this->server->hasArg("number")) {
    
    int i = this->server->arg("number").toInt();

    if (this->server->hasArg("time")) {
      // set time
      int index=this->server->arg("time").indexOf(":");
      int length=this->server->arg("time").length();
      Config.alarm[i].h=this->server->arg("time").substring(0,index).toInt();
      Config.alarm[i].m=this->server->arg("time").substring(index+1,length).toInt();
    }

    if (this->server->hasArg("enabled") && this->server->arg("enabled").equalsIgnoreCase("On")) {
      Config.alarm[i].enabled=true;
    } else {
      Config.alarm[i].enabled=false;
    }

    if (this->server->hasArg("mode")) {
      if (this->server->arg("mode").equalsIgnoreCase("matrix")) 
        Config.alarm[i].mode = DisplayMode::matrix; 
      else if (this->server->arg("mode").equalsIgnoreCase("plasma"))
        Config.alarm[i].mode = DisplayMode::plasma; 
      else if (this->server->arg("mode").equalsIgnoreCase("fire"))
        Config.alarm[i].mode = DisplayMode::fire; 
      else if (this->server->arg("mode").equalsIgnoreCase("heart"))
        Config.alarm[i].mode = DisplayMode::heart;
      else if (this->server->arg("mode").equalsIgnoreCase("stars"))
        Config.alarm[i].mode = DisplayMode::stars;
      else
        Config.alarm[i].mode = DisplayMode::plasma;  // default
    }
    
    this->server->send(200, "text/plain", "OK");
  } else {
    this->server->send(200, "text/plain", "invalid arg");
  }

  Config.saveDelayed();
}


//---------------------------------------------------------------------------------------
// handleGetConfig
//
// Outputs the currently active config in JSON
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleGetConfig()
{

  StaticJsonDocument<1024> json;
  char buf[1024];

  int displaymode = 0;
  switch(Config.defaultMode)
  {
  case DisplayMode::plain:
    displaymode = 0; break;
  case DisplayMode::fade:
    displaymode = 1; break;
  case DisplayMode::flyingLettersVerticalUp:
    displaymode = 2; break;
  case DisplayMode::flyingLettersVerticalDown:
    displaymode = 3; break;
  case DisplayMode::explode:
    displaymode = 4; break;
  default:
    displaymode = 0; break;
  }
  JsonObject background = json.createNestedObject("backgroundcolor");
  background["r"] = Config.bg.r;
  background["g"] = Config.bg.g;
  background["b"] = Config.bg.b;

  JsonObject foreground = json.createNestedObject("foregroundcolor");
  foreground["r"] = Config.fg.r;
  foreground["g"] = Config.fg.g;
  foreground["b"] = Config.fg.b;

  JsonObject seconds = json.createNestedObject("secondscolor");
  seconds["r"] = Config.s.r;
  seconds["g"] = Config.s.g;
  seconds["b"] = Config.s.b;


  json["displaymode"] =  displaymode;
  json["timezone"] = Config.timeZone;
  json["nightmode"] = Config.nightmode ? "\"on\"" : "\"off\"";
  json["heartbeat"] = (Config.heartbeat==1) ? "\"on\"" : "\"off\"";
  char NTPServer[20];
  sprintf(NTPServer,"%u.%u.%u.%u",Config.ntpserver[0],Config.ntpserver[1],Config.ntpserver[2],Config.ntpserver[3]);

  json["NTPServer"] = NTPServer;
  json["Brightness"] = Brightness.brightnessOverride;

  
  for (int i=0;i<5;i++) {
    String alarmmode;
  
    JsonArray Alarm = json.createNestedArray("Alarm");
    switch(Config.alarm[i].mode)
    {
    case DisplayMode::matrix:
      alarmmode = "matrix"; break;
    case DisplayMode::plasma:
      alarmmode = "plasma"; break;
    case DisplayMode::fire:
      alarmmode  = "fire"; break;
    case DisplayMode::heart:
      alarmmode = "heart"; break;
    case DisplayMode::stars:
      alarmmode = "stars"; break;
    default:
      alarmmode = "unknown"; break;
    }

    JsonObject alarmobject = Alarm.createNestedObject();
    alarmobject["h"]=String(Config.alarm[i].h);  
    alarmobject["m"]=String(Config.alarm[i].m);  
    alarmobject["mode"]=alarmmode;  
    alarmobject["enabled"]=Config.alarm[i].enabled ? "\"on\"" : "\"off\"";  

  }
  serializeJson(json,buf);
  this->server->send(200, "application/json", buf); 
}

#ifdef DEBUG
//---------------------------------------------------------------------------------------
// handleCrashLog()
//
// Outputs the crashlog
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleShowCrashLog()
{
  char buffer[2048]="";
  SaveCrash.print(buffer,sizeof(buffer));
  this->server->send(200, "text/plain", buffer);
}


//---------------------------------------------------------------------------------------
// clearCrashLog()
//
// clears the crashlog
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleClearCrashLog()
{
  SaveCrash.clear();
  this->server->send(200, "text/plain", "OK");
}
#endif
