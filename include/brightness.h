// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  See brightness.cpp for description.
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
#ifndef _BRIGHTNESS_H_
#define _BRIGHTNESS_H_

#include <stdint.h>

class BrightnessClass
{
public:
	BrightnessClass();
	uint32_t value();

	float avg = 0;
	uint32_t brightnessOverride = 255;  // Set to 256 to have ADC control brightness

private:
	uint32_t getBrightnessForADCValue(uint32_t adcValue);
	float filter(float input);
};

extern BrightnessClass Brightness;

#endif
