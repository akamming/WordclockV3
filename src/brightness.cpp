// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This class reads the analog input with an attached light dependent resistor
//  (pullup to VCC, LDR to ground), filters the resulting ADC values with an
//  moving average filter and calculates a brightness value using an interpolation
//  on a curve defined by several points.
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
#include "brightness.h"

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
BrightnessClass Brightness = BrightnessClass();

//---------------------------------------------------------------------------------------
// BrightnessClass
//
// Constructor, initializes filter with current ADC value
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
BrightnessClass::BrightnessClass()
{
	this->avg = analogRead(A0);
}

//---------------------------------------------------------------------------------------
// filter
//
// Exponential moving average filter, implements the following behaviour:
//
//   output = (1-coeff)*last_output + coeff*input with coeff = 0.01
//
// -> input: filter input
// <- --
//---------------------------------------------------------------------------------------
float BrightnessClass::filter(float input)
{
#define COEFF 0.01f
	return (1.0f - COEFF) * this->avg + (COEFF * input);

//	float result = (1.0f - COEFF) * this->avg + (COEFF * input);
//	Serial.print("BrightnessClass::filter(input="); Serial.print(input, 3);
//	Serial.print("): this->avg="); Serial.print(this->avg, 3);
//	Serial.print(", result="); Serial.print(result, 3); Serial.print("\r\n");
//	return result;

//#define ALPHA(x) ((uint16_t)(x * 65535.0f))
//#define FILTER_COEFFICIENT ALPHA(0.01)
//	uint32_t tmp = (65536 - FILTER_COEFFICIENT) * this->avg
//			     + FILTER_COEFFICIENT * (uint32_t)input;
//	uint32_t result = (tmp + 32768) >> 16;
//	Serial.printf("BrightnessClass::filter(input=%d): this->avg=%d, tmp=%d, result=%d\r\n",
//			input, this->avg, tmp, result);
//	return (tmp + 32768) >> 16;
}

//---------------------------------------------------------------------------------------
// getBrightnessForADCValue
//
// Calculates a brightness value for a given ADC value using a table and interpolation.
//
// -> adcValue: input value [0...1023]
// <- brightness [0...256]
//---------------------------------------------------------------------------------------
uint32_t BrightnessClass::getBrightnessForADCValue(uint32_t adcValue)
{
//	float lightTable[] = {
//		0, 15,
//		59, 18,
//		67, 32,
//		155, 64,
//		600, 128,
//		800, 256,
//		1024, 256,
//		-1, -1
//	};

	float lightTable[] = {
		0, 160,
		300, 96,
		680, 64,
		800, 32,
		900, 15,
		1023, 15,
		-1, -1
	};

	if(this->brightnessOverride<256)
	{
		return this->brightnessOverride;
	}

	int i = 0;
	while (lightTable[i] != -1)
	{
		if (adcValue >= lightTable[i] && adcValue < lightTable[i + 2])
		{
			float x1 = lightTable[i + 0];
			float y1 = lightTable[i + 1];
			float x2 = lightTable[i + 2];
			float y2 = lightTable[i + 3];
			float result = y1 + ((float)adcValue - x1) * (y2 - y1) / (x2 - x1);
			return (int) result;
		}
		i += 2;
	}

	return 0;
}

//---------------------------------------------------------------------------------------
// value
//
// Triggers a new sample to be read from light sensor, returns value from average filter
//
// -> --
// <- low pass filtered brightness value
//---------------------------------------------------------------------------------------
uint32_t BrightnessClass::value()
{
	// read next ADC value and apply filter
	this->avg = this->filter(analogRead(A0));

	// calculate brightness value for filtered ADC value
	return this->getBrightnessForADCValue(this->avg);
}
