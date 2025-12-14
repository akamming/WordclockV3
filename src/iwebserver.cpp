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
//g
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
#include "iwebserver.h"
#include "mqtt.h"
#include "ntp.h"
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#ifdef DEBUG
#include "EspSaveCrash.h"
#endif

#define GETCONFIGMESSAGESIZE 2000
#define INFOMESSAGESIZE 600

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
WebServerClass iWebServer = WebServerClass();
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
	// LittleFS.begin(); // Also called by Config, so can be removed here
#ifdef ESP32
  this->server = new WebServer(80);
#else
  this->server = new ESP8266WebServer(80);
#endif
	this->server->on("/setcolor", std::bind(&WebServerClass::handleSetColor, this));
	this->server->on("/info", std::bind(&WebServerClass::handleInfo, this));
	this->server->on("/saveconfig", std::bind(&WebServerClass::handleSaveConfig, this));
	this->server->on("/loadconfig", std::bind(&WebServerClass::handleLoadConfig, this));
	this->server->on("/setheartbeat", std::bind(&WebServerClass::handleSetHeartbeat, this));
	this->server->on("/setntpserver", std::bind(&WebServerClass::handleSetNtpServer, this));
  this->server->on("/setbrightness", std::bind(&WebServerClass::handleSetBrightness, this));
	this->server->on("/getadc", std::bind(&WebServerClass::handleGetADC, this));
	this->server->on("/setmode", std::bind(&WebServerClass::handleSetMode, this));
  this->server->on("/setanimspeed", std::bind(&WebServerClass::handleSetAnimSpeed, this));
	this->server->on("/settimezone", std::bind(&WebServerClass::handleSetTimeZone, this));
  this->server->on("/resetwificredentials", std::bind(&WebServerClass::handleResetWifiCredentials, this));
  this->server->on("/factoryreset", std::bind(&WebServerClass::handleFactoryReset, this));
  this->server->on("/reset", std::bind(&WebServerClass::handleReset, this));
  this->server->on("/setnightmode", std::bind(&WebServerClass::handleSetNightMode, this));
  this->server->on("/getconfig", std::bind(&WebServerClass::handleGetConfig, this));
  this->server->on("/config.json", std::bind(&WebServerClass::handleGetConfig, this));
  this->server->on("/setalarm", std::bind(&WebServerClass::handleSetAlarm, this));
  this->server->on("/sethostname", std::bind(&WebServerClass::handleSetHostname, this));
  this->server->on("/upload", HTTP_GET, std::bind(&WebServerClass::sendUploadForm, this));  
  this->server->on("/upload", HTTP_POST, std::bind(&WebServerClass::sendOK, this), std::bind(&WebServerClass::handleFileUpload, this));


#ifdef DEBUG
  this->server->on("/h", std::bind(&WebServerClass::handleH, this));
  this->server->on("/m", std::bind(&WebServerClass::handleM, this));
  this->server->on("/r", std::bind(&WebServerClass::handleR, this));
  this->server->on("/g", std::bind(&WebServerClass::handleG, this));
  this->server->on("/b", std::bind(&WebServerClass::handleB, this));
  this->server->on("/showcrashlog", std::bind(&WebServerClass::handleShowCrashLog, this));
  this->server->on("/clearcrashlog", std::bind(&WebServerClass::handleClearCrashLog, this));
  this->server->on("/debug", std::bind(&WebServerClass::handleDebug, this));
#endif
  

	this->server->onNotFound(std::bind(&WebServerClass::handleNotFound, this));

  // Generic code which passess all webrequeststs.
#ifndef ESP32
  this->server->addHook([](const String & method, const String & url, WiFiClient * client, ESP8266WebServer::ContentTypeFunction contentType) {
    /* (void)method;      // GET, PUT, ...
    (void)url;         // example: /root/myfile.html
    (void)client;      // the webserver tcp client connection
    (void)contentType; // contentType(".html") => "text/html" */

    Serial.printf("%s called\n\r",url.c_str());

    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });
#endif
 
	this->server->begin();
}

//---------------------------------------------------------------------------------------
// sendOK
//
// Sends 200 code to user
//
//---------------------------------------------------------------------------------------
void WebServerClass::sendOK()
{
  Serial.println("sendOK");

  this->server->send(200);       //Response to the HTTP request
}

//---------------------------------------------------------------------------------------
// sendUploadForm
//
// Sends uploadform back to user
//
//---------------------------------------------------------------------------------------
void WebServerClass::sendUploadForm()
{
  Serial.println("sendUploadForm");

  this->server->send(200, "text/html", (String("Use this form to upload index.html, favicon.ico and index.css<BR /><BR />")+String(HTTP_UPLOAD_FORM)).c_str());       //Response to the HTTP request
}

//---------------------------------------------------------------------------------------
// handleFileUpload
//
// Store file on LittleFS 
//
//---------------------------------------------------------------------------------------
void WebServerClass::handleFileUpload(){ // upload a new file to the LittleFS
  HTTPUpload& upload = this->server->upload();
  if(upload.status == UPLOAD_FILE_START){
    Serial.println("Upload file start");
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    Serial.println("HandleFileUpload Name "+filename);
    fsUploadFile = LittleFS.open(filename, "w");            // Open the file for writing in LittleFS (create if it doesn't exist)
    if (!fsUploadFile) {
      this->server->send(500, "text/plain", "500: couldn't create file");
    }
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    Serial.println("Upload file write");
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    Serial.println("Upload file end");
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); 
      Serial.println(upload.totalSize);
      // server.send(200, "text/plain", "File succesfully uploaded");
      this->server->send(200, "text/html", (String(upload.filename)+
                                      String(" succesfully uploaded (")+
                                      String(upload.totalSize)+String(" bytes), do you want to upload another file?<BR /><BR />")+
                                      String(HTTP_UPLOAD_FORM)).c_str()); // send form to upload another file
    } else {
      this->server->send(500, "text/plain", "500: couldn't create file");
    }
  }
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
	if (LittleFS.exists(path))
	{
		File file = LittleFS.open(path, "r");
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
    else if (this->endsWith(path,".json") ) this->server->streamFile(file, "application/json");
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

  /* Serial.println("Resetting Wifi Credentials");
  WiFiManager wifiManager;
  wifiManager.resetSettings(); */

  this->server->send(200, "text/plain", "OK"); 
  MQTT.PublishStatus("offline");

  delay(500); // wait half a second, then reset
#ifdef ESP32
  ESP.restart();
#else
  ESP.reset();
#endif
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
  MQTT.PublishStatus("offline");

  this->server->send(200, "text/plain", "OK"); 
  
  delay(500); // wait half a second, then reset
#ifdef ESP32
  ESP.restart();
#else
  ESP.reset();
#endif
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
  MQTT.PublishStatus("offline");
  this->server->send(200, "text/plain", "OK");

#ifdef ESP32
  ESP.restart();
#else 
  ESP.reset();
#endif
}

#ifdef DEBUG
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

//---------------------------------------------------------------------------------------
// handleM
//
// Handles the /m request, increments the minutes counter (for testing purposes)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleM()
{
	if(++NTP.m>59) NTP.m = 0;
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
	if(++NTP.h>23) NTP.h = 0;
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
// handleSetNightMode
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


void WebServerClass::handleGetADC()
{
  
	int __attribute__ ((unused)) temp = Brightness.value(); // to trigger A/D conversion

  char buf[318];
  sprintf(buf,"%f",Brightness.avg);
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
// handleSetAnimspeed
//
// Handles the /setanimspeed request. Sets the animationspeed between 1..100
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetAnimSpeed()
{
  if(this->server->hasArg("value"))
  {
    // get value
    byte animspeed=this->server->arg("value").toInt();
    
    // Check value
    if (animspeed>0&&animspeed<=100)
    {
      Config.animspeed=animspeed;
      Config.saveDelayed();
      this->server->send(200, "text/plain", "OK");
    } else {
      this->server->send(200, "text/plain", "Value should be min 1 or max 100");
    }
  } else {
    this->server->send(200, "text/plain", "Missing value");
  }
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
    if(this->server->arg("value") == "5") mode = DisplayMode::plasma;
    if(this->server->arg("value") == "6") mode = DisplayMode::matrix;
    if(this->server->arg("value") == "7") mode = DisplayMode::heart;
    if(this->server->arg("value") == "8") mode = DisplayMode::fire;
    if(this->server->arg("value") == "9") mode = DisplayMode::stars;
    if(this->server->arg("value") == "10") mode = DisplayMode::random;
    if(this->server->arg("value") == "11") mode = DisplayMode::HorizontalStripes;
    if(this->server->arg("value") == "12") mode = DisplayMode::VerticalStripes;
    if(this->server->arg("value") == "13") mode = DisplayMode::RandomDots;
    if(this->server->arg("value") == "14") mode = DisplayMode::RandomStripes;
    if(this->server->arg("value") == "15") 
    {
      mode = DisplayMode::RotatingLine;
      LED.X1=0;
      LED.Y1=0;
    }
    if(this->server->arg("value") == "16") mode = DisplayMode::christmastree;
    if(this->server->arg("value") == "17") mode = DisplayMode::christmasstar;
	}

	if(mode == DisplayMode::invalid)
	{
		this->server->send(400, "text/plain", "ERR");
	}
	else
	{
		LED.setMode(mode);
    LED.lastOffset=0; // in case of moving effects, reset from start
		Config.defaultMode = mode;
		Config.save();
		this->server->send(200, "text/plain", "OK");
	}
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
	JsonDocument json;
	json["heap"] = ESP.getFreeHeap();
	json["sketchsize"] = ESP.getSketchSize();
	json["sketchspace"] = ESP.getFreeSketchSpace();
	json["cpufrequency"] = ESP.getCpuFreqMHz();
	json["sdkversion"] = ESP.getSdkVersion();
  json["usedledlibrary"] = LED.UsedLedLib;
#ifdef ESP32
  json["chiprevision"] = ESP.getChipRevision();
#else
  json["resetreason"] = ESP.getResetReason();
  json["resetinfo"] = ESP.getResetInfo();
  json["chipid"] = ESP.getChipId();
  json["coreversion"] = ESP.getCoreVersion ();
  json["bootversion"] = ESP.getBootVersion();
  json["bootmode"] = ESP.getBootMode();
  json["flashid"] = ESP.getFlashChipId();
  json["flashspeed"] = ESP.getFlashChipSpeed();
  json["flashsize"] = ESP.getFlashChipRealSize();
#endif
  json["configsize"] = Config.Configsize();
  json["MQTTConnected"] = MQTT.connected(); // ? "Yes " : "No" ;

  switch (NTP.weekday)
  {
    case 0:
      json["CurrentDay"]="Sunday"; break;
    case 1:
      json["CurrentDay"]="Monday"; break;
    case 2:
      json["CurrentDay"]="Tuesday"; break;
    case 3:
      json["CurrentDay"]="Wednesday"; break;
    case 4:
      json["CurrentDay"]="Thursday"; break;
    case 5:
      json["CurrentDay"]="Friday"; break;
    case 6:
      json["CurrentDay"]="Saturday"; break;
    default:
      json["CurrentDay"]="Unknown"; 
  }

  int milliseconds  = millis();
  long seconds=milliseconds/1000;
  int msecs = milliseconds % 1000;
  int secs = seconds % 60;
  int mins = (seconds/60) % 60;
  int hrs = (seconds/3600) % 24;
  int days = (seconds/(3600*24));

#ifdef ESP32
  // for some reasons ESP32 crashes when using char buffers
  json["uptime"] = String(days)+" days, "+String(hrs)+" hours, "+String(mins)+" mins, "+String(secs)+" seconds, "+String(msecs)+" milliseconds";
  json["CurrentTime"] = String(NTP.h)+":"+String(NTP.m)+":"+String(NTP.s)+"."+String(NTP.ms);
  json["CurrentDate"] = String(NTP.day)+"/"+String(NTP.month)+"/"+String(NTP.year);
#else
  char buffer[100];
  sprintf(buffer, "%i days, %i hours, %i mins, %i seconds, %i milliseconds", days, hrs, mins, secs, msecs);
  json["uptime"] = buffer;
  sprintf(buffer, "%02i:%02i:%02i.%03i",NTP.h,NTP.m,NTP.s,NTP.ms);
  json["CurrentTime"] = buffer;
  sprintf(buffer, "%i/%i/%i",NTP.day, NTP.month,NTP.year);
  json["CurrentDate"] = buffer;
#endif

  String buf;
  serializeJsonPretty(json,buf);
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
  this->extractColor((char *)"fg", Config.fg);
	this->extractColor((char *)"bg", Config.bg);
	this->extractColor((char *)"s", Config.s);
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
  Serial.println("handleSaveConfig");
  String Message;
  
  // for Debugging purposes
  Serial.println("Handling Command: Number of args received: "+this->server->args());
  for (int i = 0; i < this->server->args(); i++) {
    Serial.println ("Argument "+String(i)+" -> "+this->server->argName(i)+": "+this->server->arg(i));
  } 

  // try to deserialize
  JsonDocument json;
  DeserializationError error = deserializeJson(json, this->server->arg("plain"));
  if (error) {
    Message=String("Invalid JSON: ")+error.f_str();
    this->server->send(500, "text/plain", Message.c_str());
    return;
  } else {
    //save the custom parameters to FS
    Serial.println("saving config");

    // hostname
    strncpy(Config.hostname,json["hostname"],CONFIGSTRINGSIZE);
    // ntp server    
    IPAddress ip;
		if (ip.fromString(String(json["ntpserver"])))
		{
			// set IP address in config
			Config.ntpserver = ip;

			// set IP address in client
			NTP.setServer(ip);
		}

    // Check if password was changed
    if (json["mqttpass"]=="*****") {
      json["mqttpass"]=Config.mqttpass;
    }

    // mqttsettings
    Config.usemqtt=json["usemqtt"];
    Config.usemqttauthentication=json["usemqttauthentication"];
    strncpy(Config.mqttserver,json["mqttserver"],CONFIGSTRINGSIZE);
    Config.mqttport=json["mqttport"];
    strncpy(Config.mqttuser,json["mqttuser"],CONFIGSTRINGSIZE);
    strncpy(Config.mqttpass,json["mqttpass"],CONFIGSTRINGSIZE);
    Config.mqttpersistence=json["mqttretained"];

    Config.save();
    MQTT.PublishStatus("offline");

    this->server->send(200, "text/plain", "New Config Saved");
    delay(500); // wait for server send to finish
#ifdef ESP32
    ESP.restart();
#else
    ESP.reset();
#endif

  }
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

    if (this->server->hasArg("duration")) {
      Config.alarm[i].duration=server->arg("duration").toInt();
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
      else if (this->server->arg("mode").equalsIgnoreCase("wakeup"))
        Config.alarm[i].mode = DisplayMode::wakeup;
      else
        Config.alarm[i].mode = DisplayMode::plasma;  // default
    }

    if (this->server->hasArg("type")) {
      if (this->server->arg("type").equalsIgnoreCase("eenmalig")) 
        Config.alarm[i].type = AlarmType::oneoff; 
      else if (this->server->arg("type").equalsIgnoreCase("altijd")) 
        Config.alarm[i].type = AlarmType::always; 
      else if (this->server->arg("type").equalsIgnoreCase("werkdagen")) 
        Config.alarm[i].type = AlarmType::workingdays; 
      else if (this->server->arg("type").equalsIgnoreCase("weekend")) 
        Config.alarm[i].type = AlarmType::weekend; 
      else
        Config.alarm[i].type = AlarmType::oneoff;  // default
    }
    
    this->server->send(200, "text/plain", "OK");
  } else {
    this->server->send(200, "text/plain", "invalid arg");
  }

  Config.saveDelayed();
}

//---------------------------------------------------------------------------------------
// handleSetHostname
//
// Sets the currently active hostname
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void WebServerClass::handleSetHostname()
{
  if(this->server->hasArg("value"))
  {
    strncpy(Config.hostname,this->server->arg("value").c_str(),25);
    MQTT.PublishStatus("offline");

    Config.save();
#ifdef ESP32
    ESP.restart();
#else
    ESP.reset();
#endif
    this->server->send(200, "text/plain", Config.hostname);
  } else {
    this->server->send(200, "text/plain", "invalid arg"); 
  }
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
  // Get Config
  JsonDocument json = Config.json();

  // Overrulle password with 5 stars
  json["mqttpass"] = "*****"; 

  String buf;
  serializeJsonPretty(json, buf);
  this->server->send(200, "application/json", buf); 
}
