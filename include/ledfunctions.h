// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  See ledfunctions.cpp for description.
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
#ifndef _LEDFUNCTIONS_H_
#define _LEDFUNCTIONS_H_

// #define FASTLED
#define NEOPIXELBUS

#ifdef FASTLED
// #define FASTLED_INTERRUPT_RETRY_COUNT 1
#include <FastLED.h> // FastLED
#elif defined(NEOPIXELBUS)
#include <NeoPixelBus.h>
#else
#include <Adafruit_NeoPixel.h>
#endif

#include <stdint.h>
#include <vector>

#include "config.h"
#include "matrixobject.h"
#include "starobject.h"
#include "particle.h"

typedef struct _leds_template_t
{
	int param0, param1, param2;
	const std::vector<int> LEDs;
} leds_template_t;

typedef struct _xy_t
{
	int xTarget, yTarget, x, y, delay, speed, counter;
} xy_t;

#define NUM_MATRIX_OBJECTS 25
#define NUM_STARS 10
#define NUM_BRIGHTNESS_CURVES 2
#define DEFAULTICKTIME 20

#define FADEINTERVAL 15
#define RENDERHEARTINTERVAL 10
#define MATRIXINTERVAL 10
#define FLYINGLETTERSINTERVAL 10
#define EXPLODEINTERVAL 15
#define FIREINTERVAL 100

class LEDFunctionsClass
{
public:
	LEDFunctionsClass();
	~LEDFunctionsClass();
	void begin(int pin);
	void process();
	void setBrightness(int brightness);
	void setMode(DisplayMode newMode);
	void show();

	static int getOffset(int x, int y);
	static const int width = 11;
	static const int height = 10;
	uint8_t currentValues[NUM_PIXELS * 3];
  float AlarmProgress=0; // let the alarm know how much % of the time has passed

  // Effect vars
  unsigned long lastUpdate=0; // for some effects
  int lastOffset=0; // for some effects
  int X1 = 0;
  int X2 = 10;
  int Y1 = 0;
  int Y2 = 9;

#ifdef FASTLED
  CRGB leds[NUM_PIXELS]; // FastLed
  const char* UsedLedLib="Fastled";
#elif defined(NEOPIXELBUS)
#ifdef ESP32
  NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod> *strip = NULL;
#else
  NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> *strip = NULL;
  // NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
#endif
  const char* UsedLedLib="NeoPixelBus";
#else
  Adafruit_NeoPixel *pixels = NULL;
  const char* UsedLedLib="Adafruit NeoPixel";
#endif

private:
	static const std::vector<leds_template_t> hoursTemplate;
	static const std::vector<leds_template_t> minutesTemplate;
	static const palette_entry firePalette[];
	static const palette_entry plasmaPalette[];
  int hourglassState = 0;

	DisplayMode mode = DisplayMode::plain;

	std::vector<Particle*> particles;
	std::vector<xy_t> arrivingLetters;
	std::vector<xy_t> leavingLetters;
	std::vector<MatrixObject> matrix;
	std::vector<StarObject> stars;
	uint8_t targetValues[NUM_PIXELS * 3];

  
	int heartBrightness = 0;
	int heartState = 0;
	int brightness = 96;
	int lastM = -1;
	int lastH = -1;
  unsigned long lastFadeTick=0;


  void drawDot(uint8_t *target, uint8_t x, uint8_t y, palette_entry palette);
  void drawLine(uint8_t *target, int8_t x1, int8_t y1, int8_t x2, int8_t y2, palette_entry color);
	void fillBackground(int seconds, int milliseconds, uint8_t *buf);
  void fillTime(int h, int m, uint8_t *target);
	void renderRed();
	void renderGreen();
	void renderBlue();
	void renderMatrix();
	void renderHeart();
	void renderFire();
	void renderPlasma();
  void renderStars();
  void renderChristmas();
  palette_entry blendedColor(palette_entry from_color, palette_entry to_color, float progress);
  void renderWakeup();
  void renderRandom(uint8_t *target);
	void renderUpdate();
	void renderUpdateComplete();
	void renderUpdateError();
	void renderHourglass(bool green);
	void renderWifiManager();
	void renderTime(uint8_t *target);
	void renderFlyingLetters();
	void prepareFlyingLetters(uint8_t *source);
  void renderExplosion();
  void renderRandomDots();
  void renderRandomStripes();
  void renderRotatingLine();
  void renderStripes(uint8_t *target, bool Horizontal);
	void prepareExplosion(uint8_t *source);
	void fade();
	void set(const uint8_t *buf, palette_entry palette[]);
	void set(const uint8_t *buf, palette_entry palette[], bool immediately);
	void setBuffer(uint8_t *target, const uint8_t *source, palette_entry palette[]);

	// this mapping table maps the linear memory buffer structure used throughout the
	// project to the physical layout of the LEDs
	static const uint32_t PROGMEM mapping[NUM_PIXELS];

	static const uint32_t PROGMEM brightnessCurveSelect[NUM_PIXELS];
	static const uint32_t PROGMEM brightnessCurvesR[256*NUM_BRIGHTNESS_CURVES];
	static const uint32_t PROGMEM brightnessCurvesG[256*NUM_BRIGHTNESS_CURVES];
	static const uint32_t PROGMEM brightnessCurvesB[256*NUM_BRIGHTNESS_CURVES];;
};

extern LEDFunctionsClass LED;

#endif
