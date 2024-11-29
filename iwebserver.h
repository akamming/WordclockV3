// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  See webserver.cpp for description.
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
#ifndef _WEBSERVER_H_
#define _WEBSERVER_H_
// #define DEBUG

#include <stdint.h>
#ifdef ESP32
// #include <Wifi.h>
#include <WebServer.h>
#else
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif
#include "config.h"
#include <LittleFS.h>               // Filesystem

// uploadform
const char HTTP_UPLOAD_FORM[] PROGMEM = "<form method=\"post\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"name\"><input class=\"button\" type=\"submit\" value=\"Upload\"></form>";



class WebServerClass
{
public:
	WebServerClass();
	virtual ~WebServerClass();
	void begin();
	void process();

private:
#ifdef ESP32
   WebServer *server = NULL;
#else
	ESP8266WebServer *server = NULL;
#endif

  // object for uploading files
  File fsUploadFile;


	bool serveFile(const char url[]);
  bool endsWith(const char* what, const char* withwhat);
	void handleSaveConfig();
	void handleLoadConfig();
	void handleSetColor();
	void handleNotFound();
	void handleSetTimeZone();
	void handleSetMode();
	void handleSetHeartbeat();
	void handleInfo();
  void handleSetBrightness();
	void handleGetADC();
	void handleSetNtpServer();
  void handleResetWifiCredentials();
  void handleFactoryReset();
  void handleReset();
  void handleSetNightMode();
  void handleGetConfig();
  void handleSetAlarm();
  void handleSetHostname();
  void handleSetAnimSpeed();
  void sendUploadForm();
  void handleFileUpload();
  void sendOK();

#ifdef DEBUG
  void handleShowCrashLog();
  void handleClearCrashLog();
  void handleH();
  void handleM();
  void handleR();
  void handleG();
  void handleB();
  void handleDebug();
#endif

	void extractColor(char argName[], palette_entry& result);
};

extern WebServerClass iWebServer;

#endif
