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

#define NUM_PIXELS 114
#define HOURGLASS_ANIMATION_FRAMES 8

// structure to encapsulate a color value with red, green and blue values
typedef struct _palette_entry
{
	uint8_t r, g, b;
} palette_entry;

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
} config_struct;

#define EEPROM_SIZE 512

enum class DisplayMode
{
	plain, fade, flyingLettersVerticalUp, flyingLettersVerticalDown, explode,
	random, matrix, heart, fire, plasma, stars, red, green, blue,
	yellowHourglass, greenHourglass, update, updateComplete, updateError,
	wifiManager, invalid
};

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

	// public configuration variables
	palette_entry fg;
	palette_entry bg;
	palette_entry s;
	IPAddress ntpserver = IPAddress(0, 0, 0, 0);
	bool heartbeat = true;
	bool debugMode = false;
  bool nightmode = false;

	DisplayMode defaultMode = DisplayMode::explode;

	int updateProgress = 0;
	int hourglassState = 0;
	int timeZone = 0;

	int delayedWriteTimer = 0;
	bool delayedWriteFlag = false;

private:
	// copy of EEPROM content
	config_struct *config = (config_struct*) eeprom_data;
	uint8_t eeprom_data[EEPROM_SIZE];
};

extern ConfigClass Config;

#endif /* CONFIG_H_ */
