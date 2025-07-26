// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This is the configuration module. It contains methods to load/save the
//  configuration from/to the internal EEPROM (simulated EEPROM in flash).
//  Data is loaded into this->eeprom_data[EEPROM_SIZE] which shares RAM with
//  this->config. Configuration variables are copied to public class members
//  ntpserver, heartbeat, ... where they can be used by other modules. Upon
//  save, the public members are copied back to this->config/this->eeprom_data[]
//  and then written to the EEPROM.
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
#include <EEPROM.h>
#include <LittleFS.h>               // Filesystem
#include "config.h"
#include "brightness.h"


//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
ConfigClass Config = ConfigClass();

//---------------------------------------------------------------------------------------
// ConfigClass
//
// Constructor, loads default values
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
ConfigClass::ConfigClass()
{
	this->reset();
}

//---------------------------------------------------------------------------------------
// ~ConfigClass
//
// destructor
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
ConfigClass::~ConfigClass()
{
}

//---------------------------------------------------------------------------------------
// begin
//
// Initializes the class and loads current configuration from EEPROM into class members.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void ConfigClass::begin()
{
  EEPROM.begin(EEPROM_SIZE);
  LittleFS.begin();
	this->load();
  this->lastMillis=millis();
}

//---------------------------------------------------------------------------------------
// saveDelayed
//
// Copies the current class member values to EEPROM buffer and writes it to the EEPROM
// after 10 seconds.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void ConfigClass::saveDelayed()
{
	this->delayedWriteTimer = CONFIGWRITETIMEOUT; // No of msecs to count down.
}

//---------------------------------------------------------------------------------------
// process
//
// handles any periodic handling.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void ConfigClass::process()
{
  // decrement delayed EEPROM config timer
  if(this->delayedWriteTimer>0)
  {
    this->delayedWriteTimer-=(unsigned long)(millis() - this->lastMillis);
    this->lastMillis=millis();
    if(this->delayedWriteTimer <= 0) 
    {
      this->delayedWriteTimer=0; // make sure we don't save to often.
      Serial.println("Config timer expired, writing configuration.");
      this->save();
    }
  }
}

//---------------------------------------------------------------------------------------
// Configsize
//
// retrieves the config size.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
int ConfigClass::Configsize()
{
  return int(sizeof(_config_struct));
}


//---------------------------------------------------------------------------------------
// json
//
// Copies the current config and writes it to a json object.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
JsonDocument ConfigClass::json()
{
  // Create json object
  JsonDocument json;

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
  case DisplayMode::wakeup:
    displaymode = 5; break;
  case DisplayMode::matrix:
    displaymode = 6; break;
  case DisplayMode::heart:
    displaymode = 7; break;
  case DisplayMode::fire:
    displaymode = 8; break;
  case DisplayMode::stars:
    displaymode = 9; break;
  case DisplayMode::random:
    displaymode = 10; break;
  case DisplayMode::HorizontalStripes:
    displaymode = 11; break;
  case DisplayMode::VerticalStripes:
    displaymode = 12; break;
  case DisplayMode::RandomDots:
    displaymode = 13; break;
  case DisplayMode::RandomStripes:
    displaymode = 14; break;
  case DisplayMode::RotatingLine:
    displaymode = 15; break;
  default:
    displaymode = 1; break;
  }
 
  JsonObject background = json["backgroundcolor"].to<JsonObject>();
  background["r"] = Config.bg.r;
  background["g"] = Config.bg.g;
  background["b"] = Config.bg.b;

  JsonObject foreground = json["foregroundcolor"].to<JsonObject>();
  foreground["r"] = Config.fg.r;
  foreground["g"] = Config.fg.g;
  foreground["b"] = Config.fg.b;

  JsonObject seconds = json["secondscolor"].to<JsonObject>();
  seconds["r"] = Config.s.r;
  seconds["g"] = Config.s.g;
  seconds["b"] = Config.s.b;


  json["displaymode"] =  displaymode;
  json["animspeed"] = Config.animspeed;
  json["timezone"] = Config.timeZone;
  json["nightmode"] = Config.nightmode;
  json["heartbeat"] = Config.heartbeat==1;
  char NTPServer[20];
  sprintf(NTPServer,"%u.%u.%u.%u",Config.ntpserver[0],Config.ntpserver[1],Config.ntpserver[2],Config.ntpserver[3]);

  json["NTPServer"] = NTPServer;
  json["Brightness"] = Brightness.brightnessOverride;
  json["hostname"] = Config.hostname;

  JsonArray Alarm = json["Alarm"].to<JsonArray>();
  for (int i=0;i<5;i++) {
    String alarmmode,alarmtype;
  
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
    case DisplayMode::wakeup:
      alarmmode = "wakeup"; break;
    default:
      alarmmode = "unknown"; break;
    }

    switch(Config.alarm[i].type)
    {
      case AlarmType::oneoff:
        alarmtype="Eenmalig"; break;
      case AlarmType::always:
        alarmtype="Altijd"; break;
      case AlarmType::weekend:
        alarmtype="Weekend"; break;
      case AlarmType::workingdays:
        alarmtype="Werkdagen"; break;
      default:
        alarmtype="Onbekend"; break;
    }

    char timestr[12];
    sprintf(timestr,"%02d:%02d",Config.alarm[i].h,Config.alarm[i].m);

    JsonObject alarmobject = Alarm.add<JsonObject>();
    alarmobject["time"]=timestr;
    alarmobject["duration"]=Config.alarm[i].duration;  
    alarmobject["mode"]=alarmmode;  
    alarmobject["enabled"]=Config.alarm[i].enabled;  
    alarmobject["type"]=alarmtype;
  }

  json["hostname"] = Config.hostname;

  // mqtt settings
  json["usemqtt"] = Config.usemqtt; 
  json["mqttpersistence"] = Config.mqttpersistence;
  json["mqttserver"] = Config.mqttserver;
  json["mqttport"] =Config.mqttport;
  json["usemqttauthentication"] = Config.usemqttauthentication;
  json["mqttuser"] = Config.mqttuser;
  json["mqttpass"] = Config.mqttpass; 
 
  return json;
}

//---------------------------------------------------------------------------------------
// save
//
// Copies the current class member values to EEPROM buffer and writes it to the EEPROM.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void ConfigClass::save()
{
  this->delayedWriteTimer = 0; // Make sure we are not saving again after timer expires

	this->config->bg = this->bg;
	this->config->fg = this->fg;
	this->config->s = this->s;
	this->config->timeZone = this->timeZone;
	this->config->heartbeat = this->heartbeat;
  this->config->nightmode = this->nightmode;
	this->config->mode = (uint32_t) this->defaultMode;
	for (int i = 0; i < 4; i++)
		this->config->ntpserver[i] = this->ntpserver[i];
  this->config->brightnessOverride=Brightness.brightnessOverride;
  for (int i=0; i<5;i++)
    this->config->alarm[i]=this->alarm[i];

  strncpy(this->config->hostname,this->hostname,CONFIGSTRINGSIZE);
  this->config->animspeed=this->animspeed;

  // mqtt settings
  this->config->usemqtt = this->usemqtt; 
  this->config->mqttpersistence = this->mqttpersistence;
  strncpy(this->config->mqttserver,this->mqttserver,CONFIGSTRINGSIZE);
  this->config->mqttport = this->mqttport;
  this->config->usemqttauthentication=this->usemqttauthentication;
  strncpy(this->config->mqttuser,this->mqttuser,CONFIGSTRINGSIZE);
  strncpy(this->config->mqttpass,this->mqttpass,CONFIGSTRINGSIZE);

	for (int i = 0; i < EEPROM_SIZE; i++)
		EEPROM.write(i, this->eeprom_data[i]);
	EEPROM.commit();

  // save the new file
  File configFile = LittleFS.open(CONFIGFILE, "w");
  if (!configFile) {
    Serial.println("unable to open "+String(CONFIGFILE));
  } else {
    // Save the file
    serializeJson(this->json(), configFile);
    configFile.close();
    Serial.println("Configfile saved");
  }  
}

//---------------------------------------------------------------------------------------
// reset
//
// Sets default values in EEPROM buffer and member variables.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void ConfigClass::reset()
{
	this->config->magic = 0xDEADBEEF;
	this->config->bg =
	{	0, 0, 0};
	this->bg = this->config->bg;

	this->config->fg =
	{	255, 255, 255};
	this->fg = this->config->fg;

	this->config->s =
	{	32, 0, 21};
	this->s = this->config->s;

	this->config->heartbeat = false;
	this->heartbeat = this->config->heartbeat;
  
  this->config->nightmode= false;
  this->nightmode= this->config->nightmode;

	this->defaultMode = DisplayMode::fade;
	this->config->mode = (uint32_t) this->defaultMode;
	this->timeZone = 0;

	// this->config->ntpserver[0] = 129;
	// this->config->ntpserver[1] = 6;
	// this->config->ntpserver[2] = 15;
	// this->config->ntpserver[3] = 28;

  // Replaced with ntp.google.com which is much faster
  this->config->ntpserver[0] = 216;
  this->config->ntpserver[1] = 239;
  this->config->ntpserver[2] = 35;
  this->config->ntpserver[3] = 8;  // For ntp.google.com, 0,4 or 12 will work as well
  
	this->ntpserver[0] = this->config->ntpserver[0];
	this->ntpserver[1] = this->config->ntpserver[1];
	this->ntpserver[2] = this->config->ntpserver[2];
	this->ntpserver[3] = this->config->ntpserver[3];

  // reset all alarms

  // Default times
  this->alarm[0]={ 19, 00, 1, DisplayMode::matrix, false, AlarmType::oneoff};
  this->alarm[1]={ 20, 00, 1, DisplayMode::plasma, false, AlarmType::oneoff};
  this->alarm[2]={ 21, 00, 1, DisplayMode::fire, false, AlarmType::oneoff};
  this->alarm[3]={ 22, 00, 1, DisplayMode::heart, false, AlarmType::oneoff};
  this->alarm[4]={ 23, 00, 1, DisplayMode::stars, false, AlarmType::oneoff};

  strcpy(this->hostname,"WordClock");

  this->animspeed = 50; // just a default value, should be between 1 and 100

  // mqtt settings
  this->usemqtt = false; 
  this->mqttpersistence = false;
  strcpy(this->mqttserver,"");
  this->mqttport = 1883;
  this->usemqttauthentication = false;
  strcpy(this->mqttuser,"");
  strcpy(this->mqttpass,"");
}

//---------------------------------------------------------------------------------------
// load
//
// Reads the content of the EEPROM into the EEPROM buffer and copies the values to the
// public member variables. Resets (and saves) the values to their defaults if the
// EEPROM data is not initialized.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void ConfigClass::load()
{
	Serial.println("Reading EEPROM config");
	for (int i = 0; i < EEPROM_SIZE; i++)
		this->eeprom_data[i] = EEPROM.read(i);
	if (this->config->magic != 0xDEADBEEF)
	{
		Serial.println("EEPROM config invalid, writing default values");
		this->reset();
		this->save();
	}
	this->bg = this->config->bg;
	this->fg = this->config->fg;
	this->s = this->config->s;
	this->defaultMode = (DisplayMode) this->config->mode;
  this->heartbeat = this->config->heartbeat;
  this->nightmode = this->config->nightmode;
	this->timeZone = this->config->timeZone;
	for (int i = 0; i < 4; i++)
		this->ntpserver[i] = this->config->ntpserver[i];
  Brightness.brightnessOverride=this->config->brightnessOverride;
  
  for (int i=0; i<5;i++)
    this->alarm[i]=this->config->alarm[i];
  
  strncpy(this->hostname,this->config->hostname,CONFIGSTRINGSIZE);
  this->hostname[CONFIGSTRINGSIZE-1]='\0'; // prevent crash by forcing 0 termination
  this->animspeed = this->config->animspeed;

    // mqtt settings
  this->usemqtt = this->config->usemqtt; 
  this->mqttpersistence = this->config->mqttpersistence;
  strncpy(this->mqttserver,this->config->mqttserver,CONFIGSTRINGSIZE);
  this->mqttserver[CONFIGSTRINGSIZE-1]='\0';  // prevent crash by forcing 0 termination
  this->mqttport = this->config->mqttport;
  this->usemqttauthentication = this->config->usemqttauthentication;
  strncpy(this->mqttuser,this->config->mqttuser,CONFIGSTRINGSIZE);
  this->mqttuser[CONFIGSTRINGSIZE-1]='\0';// prevent crash by forcing 0 termination
  strncpy(this->mqttpass,this->config->mqttpass,CONFIGSTRINGSIZE);
  this->mqttpass[CONFIGSTRINGSIZE-1]='\0'; // prevent crash by forcing 0 termination
}
