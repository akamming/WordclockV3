// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  See config.cpp for description.
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
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <IPAddress.h>
#include <ArduinoJson.h>

// constants
#define NUM_PIXELS 114
#define EEPROM_SIZE 512
#define CONFIGWRITETIMEOUT 10000
#define CONFIGSTRINGSIZE 25
#define CONFIGFILE  "/config.json"                // name of the config file on the SPIFFS image


enum class DisplayMode
{
  plain, fade, flyingLettersVerticalUp, flyingLettersVerticalDown, explode,
  random, matrix, heart, fire, plasma, stars, wakeup, HorizontalStripes, VerticalStripes,
  RandomDots, RandomStripes, RotatingLine, red, green, blue,
  yellowHourglass, greenHourglass, update, updateComplete, updateError,
  wifiManager, invalid
};

enum class AlarmType
{
  oneoff, always, weekend, workingdays
};

// structure to encapsulate a color value with red, green and blue values
typedef struct _palette_entry
{
	uint8_t r, g, b;
} palette_entry;

typedef struct _alarm
{
  uint8_t h,m,duration;
  DisplayMode mode;
  bool enabled;
  AlarmType type;
} t_alarm;

// structure with configuration data to be stored in EEPROM
typedef struct _config_struct
{
	uint32_t magic;
	palette_entry bg;
	palette_entry fg;
	palette_entry s;
	uint8_t ntpserver[4];
	bool heartbeat;
	uint32_t mode;
	uint32_t timeZone;
  uint32_t brightnessOverride;
  bool nightmode;
  t_alarm alarm[5];
  char hostname[CONFIGSTRINGSIZE];
  uint8_t animspeed;
  bool usemqtt;
  bool mqttpersistence;
  char mqttserver[CONFIGSTRINGSIZE];
  int mqttport;
  bool usemqttauthentication = false;
  char mqttuser[CONFIGSTRINGSIZE];
  char mqttpass[CONFIGSTRINGSIZE];
} config_struct;

class ConfigClass
{
public:
	// public methods
	ConfigClass();
	virtual ~ConfigClass();
	void begin();
	void save();
	void saveDelayed();
	void load();
	void reset();
  JsonDocument json();
  int Configsize();
  void process();

	// public configuration variables
	palette_entry fg;
	palette_entry bg;
	palette_entry s;
	IPAddress ntpserver = IPAddress(0, 0, 0, 0);
	bool heartbeat = true;
	bool debugMode = false;
  bool nightmode = false;

	// DisplayMode defaultMode = DisplayMode::explode;
  DisplayMode defaultMode = DisplayMode::fade;

	int updateProgress = 0;
	int timeZone = 0;

	int delayedWriteTimer = 0;
  int lastMillis=0;

  t_alarm alarm[5];

  char hostname[CONFIGSTRINGSIZE];
  uint8_t animspeed=50; // value from 1..100

  // MQTT Settingds
  bool usemqtt;
  bool mqttpersistence;
  char mqttserver[CONFIGSTRINGSIZE];
  int mqttport;
  bool usemqttauthentication = false;
  char mqttuser[CONFIGSTRINGSIZE];
  char mqttpass[CONFIGSTRINGSIZE];

private:
	// copy of EEPROM content
	config_struct *config = (config_struct*) eeprom_data;
	uint8_t eeprom_data[EEPROM_SIZE];
};

extern ConfigClass Config;

#endif /* CONFIG_H_ */
