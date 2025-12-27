// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This module implements functions to manage the WS2812B LEDs. Two buffers contain
//  color information with current state and fade target state and are updated by
//  either simple set operations or integrated screensavers (matrix, stars, heart).
//  Also contains part of the data and logic for the hourglass animation.
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
#include "ledfunctions.h"
#include "ntp.h"
//---------------------------------------------------------------------------------------
#if 1 // variables
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
LEDFunctionsClass LED = LEDFunctionsClass();

uint8_t __attribute__((aligned(4))) fireBuf[NUM_PIXELS];
uint8_t plasmaBuf[NUM_PIXELS];

//---------------------------------------------------------------------------------------
// Shared letter patterns for text animations
// Capital letters are 10 pixels high (rows 0-9)
// Lowercase letters are 7 pixels high (rows 3-9)
//---------------------------------------------------------------------------------------
// Capital letters (10 pixels high)
static const uint8_t LETTER_M_CAP[10] = {0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001};
static const uint8_t LETTER_C_CAP[10] = {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110};
static const uint8_t LETTER_H_CAP[10] = {0b10001, 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001};
static const uint8_t LETTER_A_CAP[10] = {0b01110, 0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001};
static const uint8_t LETTER_P_CAP[10] = {0b11110, 0b10001, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000};
static const uint8_t LETTER_Y_CAP[10] = {0b10001, 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
static const uint8_t LETTER_N_CAP[10] = {0b10001, 0b11001, 0b11001, 0b10101, 0b10101, 0b10011, 0b10011, 0b10001, 0b10001, 0b10001};
static const uint8_t LETTER_E_CAP[10] = {0b11111, 0b10000, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
static const uint8_t LETTER_W_CAP[10] = {0b10001, 0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b11011, 0b10001, 0b10001};
static const uint8_t LETTER_R_CAP[10] = {0b11110, 0b10001, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001, 0b10001, 0b10001};

// Lowercase letters (7 pixels high, rows 3-9)
static const uint8_t LETTER_e_LOW[10] = {0b00000, 0b00000, 0b00000, 0b01110, 0b10001, 0b11111, 0b10000, 0b10001, 0b01110, 0b00000};
static const uint8_t LETTER_r_LOW[10] = {0b00000, 0b00000, 0b00000, 0b10110, 0b11001, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000};
static const uint8_t LETTER_y_LOW[10] = {0b00000, 0b00000, 0b00000, 0b10001, 0b10001, 0b10011, 0b01101, 0b00001, 0b00010, 0b01100};
static const uint8_t LETTER_h_LOW[10] = {0b10000, 0b10000, 0b10000, 0b10110, 0b11001, 0b10001, 0b10001, 0b10001, 0b10001, 0b00000};
static const uint8_t LETTER_i_LOW[10] = {0b00000, 0b00100, 0b00000, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110, 0b00000};
static const uint8_t LETTER_s_LOW[10] = {0b00000, 0b00000, 0b00000, 0b01110, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110, 0b00000};
static const uint8_t LETTER_t_LOW[10] = {0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00100, 0b00101, 0b00010, 0b00000};
static const uint8_t LETTER_a_LOW[10] = {0b00000, 0b00000, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b10011, 0b01101, 0b00000};
static const uint8_t LETTER_m_LOW[10] = {0b00000, 0b00000, 0b00000, 0b10001, 0b11011, 0b10101, 0b10101, 0b10101, 0b10001, 0b00000};
static const uint8_t LETTER_p_LOW[10] = {0b00000, 0b00000, 0b00000, 0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000};
static const uint8_t LETTER_w_LOW[10] = {0b00000, 0b00000, 0b00000, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001, 0b00000};
static const uint8_t LETTER_SPACE[10] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};

//---------------------------------------------------------------------------------------
// variables in PROGMEM (mapping table, images)
//---------------------------------------------------------------------------------------
#include "hourglass_animation.h"

// This defines the LED output for different minutes
// param0 controls whether the hour has to be incremented for the given minutes
// param1 is the matching minimum minute count (inclusive)
// param2 is the matching maximum minute count (inclusive)
#if 1 // code folding minutes template
const std::vector<leds_template_t> LEDFunctionsClass::minutesTemplate =
{
  { 0,  0,  4,{ 107, 108, 109 } },                                  // UUR
  { 0,  5,  9,{ 7, 8, 9, 10, 40, 41, 42, 43 } },                    // VIJF OVER
  { 0, 10, 14,{ 11, 12, 13, 14, 40, 41, 42, 43 } },                 // TIEN OVER
  { 0, 15, 19,{ 28, 29, 30, 31, 32, 40, 41, 42, 43 } },             // KWART OVER
  { 1, 20, 24,{ 11, 12, 13, 14, 18, 19, 20, 21, 33, 34, 35, 36 } }, // TIEN VOOR HALF
  { 1, 25, 29,{ 7, 8, 9, 10, 18, 19, 20, 21, 33, 34, 35, 36 } },    // VIJF VOOR HALF
  { 1, 30, 34,{ 33, 34, 35, 36 } },                                 // HALF
  { 1, 35, 39,{ 7, 8, 9, 10, 22, 23, 24, 25, 33, 34, 35, 36} },     // VIJF OVER HALF
  { 1, 40, 44,{ 11, 12, 13, 14, 22, 23, 24, 25, 33, 34, 35, 36} },  // TIEN OVER HALF
  { 1, 45, 49,{ 28, 29, 30, 31, 32, 44, 45, 46, 47 } },                  // KWART VOOR
  { 1, 50, 54,{ 11, 12, 13, 14, 44, 45, 46, 47} },                 // TIEN VOOR
  { 1, 55, 59,{ 7, 8, 9, 10, 44, 45, 46, 47 } }                     // VIJF VOOR
};
#endif


// This defines the LED output for different hours
// param0 deals with special cases:
//     = 0: matches hour in param1 and param2
//     = 1: matches hour in param1 and param2 whenever minute is < 5
//     = 2: matches hour in param1 and param2 whenever minute is >= 5
// param1: hour to match
// param2: alternative hour to match
#if 1 // code folding hours template
const std::vector<leds_template_t> LEDFunctionsClass::hoursTemplate =
{
  { 0,  0, 12,{ 99, 100, 101, 102, 103, 104 } }, // TWAALF
  { 1,  1, 13,{ 51, 52, 53 } },                  // EEN
  { 2,  1, 13,{ 51, 52, 53 } },                  // EEN (EINS)
  { 0,  2, 14,{ 55, 56, 57, 58 } },              // TWEE
  { 0,  3, 15,{ 62, 63, 64, 65 } },              // DRIE
  { 0,  4, 16,{ 66, 67, 68, 69 } },              // VIER
  { 0,  5, 17,{ 70, 71, 72, 73 } },              // VIJF
  { 0,  6, 18,{ 74, 75, 76 } },                  // ZES
  { 0,  7, 19,{ 77, 78, 79, 80, 81 } },          // ZEVEN
  { 0,  8, 20,{ 88, 89, 90, 91 } },              // ACHT
  { 0,  9, 21,{ 83, 84, 85, 86, 87 } },          // NEGEN
  { 0, 10, 22,{ 91, 92, 93, 94 } },              // TIEN
  { 0, 11, 23,{ 96, 97, 98 } },                  // ELF
};
#endif

#if 1 // code folding mapping table
// this mapping table maps the linear memory buffer structure used throughout the
// project to the physical layout of the LEDs
const uint32_t PROGMEM LEDFunctionsClass::mapping[NUM_PIXELS] = {
	10,   9,   8,   7,   6,   5,   4,   3,   2,   1,   0,
	11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,
	32,  31,  30,  29,  28,  27,  26,  25,  24,  23,  22,
	33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
	54,  53,  52,  51,  50,  49,  48,  47,  46,  45,  44,
	55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,
	76,  75,  74,  73,  72,  71,  70,  69,  68,  67,  66,
	77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,
	98,  97,  96,  95,  94,  93,  92,  91,  90,  89,  88,
	99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
	112, 111, 110, 113
};
#endif

#if 1 // code folding brightness adjust tables
const uint32_t PROGMEM LEDFunctionsClass::brightnessCurveSelect[NUM_PIXELS] = {
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0
		//		0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
		//		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		//		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		//		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
		//		0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1,
		//		0, 1, 0, 1
};

const uint32_t PROGMEM LEDFunctionsClass::brightnessCurvesR[256*NUM_BRIGHTNESS_CURVES] = {
		// LED type 1, 1:1 mapping (neutral)
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
		37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
		54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
		71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
		117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
		130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
		143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155,
		156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
		169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181,
		182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
		195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
		221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
		234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246,
		247, 248, 249, 250, 251, 252, 253, 254, 255,

		// LED type 2
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7,
		7, 7, 9, 9, 9, 9, 11, 11, 11, 11, 13, 13, 13, 13, 15, 15, 15, 15,
		17, 17, 17, 17, 19, 19, 21, 21, 23, 23, 25, 25, 27, 27, 29, 29, 31,
		31, 33, 33, 35, 35, 37, 37, 39, 39, 41, 41, 43, 43, 45, 45, 47, 47,
		49, 49, 50, 50, 53, 53, 53, 53, 55, 55, 57, 57, 59, 59, 61, 61, 63,
		63, 65, 65, 67, 67, 69, 69, 71, 71, 73, 73, 75, 75, 77, 77, 79, 79,
		81, 81, 83, 83, 83, 83, 85, 85, 87, 87, 89, 89, 91, 91, 93, 93, 95,
		95, 97, 97, 99, 99, 101, 101, 103, 103, 105, 105, 107, 107, 109, 109,
		111, 111, 113, 113, 115, 115, 115, 115, 117, 117, 119, 119, 121, 121,
		123, 123, 125, 125, 127, 127, 130, 130, 132, 132, 134, 134, 136, 136,
		138, 138, 140, 140, 142, 142, 144, 144, 144, 144, 146, 146, 148, 148,
		150, 150, 152, 152, 153, 153, 155, 155, 156, 156, 158, 158, 160, 160,
		162, 162, 164, 164, 166, 166, 168, 168, 170, 170, 172, 172, 174, 174,
		176, 176, 178, 178, 180, 180, 182, 182, 184, 184, 184, 184, 186, 186,
		188, 188, 190, 190, 192, 192, 194, 194, 196, 196, 198, 198, 200, 200,
		202, 202, 204, 204, 206, 206, 208, 208, 210, 210, 212, 212, 214, 214,
		216, 216, 218, 218
};

const uint32_t PROGMEM LEDFunctionsClass::brightnessCurvesG[256*NUM_BRIGHTNESS_CURVES] = {
		// LED type 1, 1:1 mapping (neutral)
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
		37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
		54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
		71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
		117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
		130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
		143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155,
		156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
		169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181,
		182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
		195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
		221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
		234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246,
		247, 248, 249, 250, 251, 252, 253, 254, 255,

		// LED type 2
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7, 7,
		7, 9, 9, 9, 9, 11, 11, 11, 11, 13, 13, 13, 13, 15, 15, 15, 15, 17,
		17, 17, 17, 19, 19, 21, 21, 23, 23, 25, 25, 27, 27, 29, 29, 31, 31,
		33, 33, 35, 35, 37, 37, 39, 39, 41, 41, 43, 43, 45, 45, 47, 47, 49,
		49, 51, 51, 53, 53, 54, 54, 56, 56, 57, 57, 59, 59, 61, 61, 63, 63,
		66, 66, 68, 68, 70, 70, 72, 72, 74, 74, 76, 76, 78, 78, 80, 80, 80,
		80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
		80, 80, 80, 80, 80, 80, 80, 80, 108, 108, 110, 110, 112, 112, 113,
		113, 116, 116, 117, 117, 119, 119, 122, 122, 124, 124, 126, 126, 127,
		127, 131, 131, 132, 132, 135, 135, 137, 137, 139, 139, 141, 141, 143,
		143, 145, 145, 147, 147, 149, 149, 151, 151, 153, 153, 155, 155, 157,
		157, 159, 159, 160, 160, 163, 163, 165, 165, 167, 167, 169, 169, 171,
		171, 173, 173, 175, 175, 177, 177, 179, 179, 181, 181, 183, 183, 185,
		185, 187, 187, 189, 189, 191, 191, 194, 194, 197, 197, 198, 198, 201,
		201, 203, 203, 205, 205, 208, 208, 210, 210, 212, 212, 215, 215, 218,
		218, 220, 220, 222, 222, 224, 224, 226, 226, 228, 228, 230, 230, 232,
		232, 234, 234
};

const uint32_t PROGMEM LEDFunctionsClass::brightnessCurvesB[256*NUM_BRIGHTNESS_CURVES] = {
		// LED type 1, 1:1 mapping (neutral)
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
		37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
		54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
		71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
		117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
		130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
		143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155,
		156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
		169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181,
		182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
		195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
		208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220,
		221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
		234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246,
		247, 248, 249, 250, 251, 252, 253, 254, 255,

		// LED type 2
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 5, 5, 5, 5, 5, 5, 7, 7, 7, 7, 9,
		9, 9, 9, 9, 9, 11, 11, 11, 11, 13, 13, 13, 13, 15, 15, 15, 15, 17,
		17, 19, 19, 20, 20, 21, 21, 23, 23, 25, 25, 27, 27, 29, 29, 31, 31,
		33, 33, 35, 35, 37, 37, 39, 39, 41, 41, 42, 42, 43, 43, 45, 45, 47,
		47, 49, 49, 51, 51, 53, 53, 55, 55, 57, 57, 59, 59, 61, 61, 63, 63,
		64, 64, 66, 66, 68, 68, 72, 72, 72, 72, 74, 74, 76, 76, 78, 78, 80,
		80, 82, 82, 84, 84, 86, 86, 88, 88, 90, 90, 91, 91, 93, 93, 94, 94,
		96, 96, 98, 98, 100, 100, 102, 102, 104, 104, 106, 106, 108, 108,
		110, 110, 112, 112, 114, 114, 116, 116, 118, 118, 119, 119, 121, 121,
		123, 123, 124, 124, 127, 127, 129, 129, 131, 131, 133, 133, 135, 137,
		137, 139, 139, 141, 141, 143, 143, 145, 145, 147, 147, 149, 149, 151,
		151, 153, 153, 155, 155, 157, 157, 160, 160, 161, 161, 163, 163, 166,
		166, 168, 168, 170, 170, 171, 171, 174, 174, 176, 176, 178, 178, 180,
		180, 183, 183, 184, 184, 187, 187, 188, 188, 191, 191, 194, 194, 196,
		196, 198, 198, 200, 200, 202, 202, 204, 204, 206, 206, 209, 209, 210,
		210, 212, 212, 215, 215, 217, 217, 219, 219, 222, 222, 224, 224, 226,
		226, 228, 228, 230
};

#endif

#endif

//---------------------------------------------------------------------------------------
#if 1 // getters, setters, data flow
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// ~LEDFunctionsClass
//
// Destructor
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
LEDFunctionsClass::~LEDFunctionsClass()
{
//	for(MatrixObject* m : this->matrix_objects) delete m;
}

//---------------------------------------------------------------------------------------
// LEDFunctionsClass
//
// Constructor, initializes data structures
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
LEDFunctionsClass::LEDFunctionsClass()
{
	// initialize matrix objects with random coordinates
	for (int i = 0; i < NUM_MATRIX_OBJECTS; i++)
	{
		this->matrix.push_back(MatrixObject());
	}

	for (int i = 0; i < NUM_STARS; i++) this->stars.push_back(StarObject());

	for (StarObject& s : this->stars) s.randomize(this->stars);

	for (int i = 0; i < NUM_STARS; i++) {
		this->stars.push_back(StarObject());
	}

	for (StarObject& s : this->stars) {
		s.randomize(this->stars);
	}
}
//---------------------------------------------------------------------------------------
// begin
//
// Initializes the LED driver
//
// -> pin: hardware pin to use for WS2812B data output
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::begin(int pin)
{
#ifdef ESP32
  this->strip = new NeoPixelBus<NeoGrbFeature, NeoWs2812xMethod>(NUM_PIXELS,pin);
#else
  this->strip = new NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod>(NUM_PIXELS);
#endif
  if(this->strip == nullptr) {
    Serial.println(F("ERROR: Failed to allocate NeoPixelBus!"));
    return;
  }
  this->strip->Begin();
}




//---------------------------------------------------------------------------------------
// process
//
// Drives internal data flow, should be called repeatedly from main loop()
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::process()
{
	if(Config.debugMode) return;
	if (Config.debugMode) return;

	// check time values against boundaries
	if (NTP.h > 23 || NTP.h < 0) NTP.h = 0;
	if (NTP.m > 59 || NTP.m < 0) NTP.m = 0;
	if (NTP.s > 59 || NTP.s < 0) NTP.s = 0;
	if (NTP.ms > 999 || NTP.ms < 0) NTP.ms = 0;

	// load palette colors from configuration
	palette_entry palette[] = {
		{Config.bg.r, Config.bg.g, Config.bg.b},
		{Config.fg.r, Config.fg.g, Config.fg.b},
		{Config.s.r,  Config.s.g,  Config.s.b}};
	uint8_t buf[NUM_PIXELS];

	switch(this->mode)
	{
	case DisplayMode::wifiManager:
		this->renderWifiManager();
		break;
	case DisplayMode::yellowHourglass:
		this->renderHourglass(false);
		break;
	case DisplayMode::greenHourglass:
		this->renderHourglass(true);
		break;
	case DisplayMode::update:
		this->renderUpdate();
		break;
	case DisplayMode::updateComplete:
		this->renderUpdateComplete();
		break;
	case DisplayMode::updateError:
		this->renderUpdateError();
		break;
	case DisplayMode::red:
		this->renderRed();
		break;
	case DisplayMode::green:
		this->renderGreen();
		break;
	case DisplayMode::blue:
		this->renderBlue();
		break;
	case DisplayMode::flyingLettersVerticalUp:
	case DisplayMode::flyingLettersVerticalDown:
		this->renderFlyingLetters();
		break;
	case DisplayMode::explode:
		this->renderExplosion();
		break;
	case DisplayMode::matrix:
		this->renderMatrix();
		break;
	case DisplayMode::heart:
		this->renderHeart();
		break;
	case DisplayMode::stars:
		this->renderStars();
		break;
	case DisplayMode::fire:
		this->renderFire();
		break;
	case DisplayMode::plasma:
		this->renderPlasma();
		break;
	case DisplayMode::wakeup:
		this->renderWakeup();
		break;
	case DisplayMode::random:
		this->renderRandom(buf);
		break;
	case DisplayMode::HorizontalStripes:
		this->renderStripes(buf, true);
		break;
	case DisplayMode::VerticalStripes:
		this->renderStripes(buf, false);
		break;
	case DisplayMode::RandomDots:
		this->renderRandomDots();
		break;
	case DisplayMode::RandomStripes:
		this->renderRandomStripes();
		break;
	case DisplayMode::RotatingLine:
		this->renderRotatingLine();
		break;
	case DisplayMode::christmastree:
		this->renderChristmasTree();
		break;
	case DisplayMode::jinglebells:
		this->renderJingleBells();
		break;
	case DisplayMode::merryChristmas:
		// in nightmode, show rendertime instead of christmas greeting
		if (Config.nightmode) {
			this->renderTime(buf);
			this->set(buf, palette, true);
			break;
		} else {
			this->renderMerryChristmas();
			break;
		}
	case DisplayMode::happyNewYear:
		// in nightmode, show rendertime instead of new year greeting
		if (Config.nightmode) {
			this->renderTime(buf);
			this->set(buf, palette, true);
			break;
		} else {
			this->renderHappyNewYear();
			break;
		}
	case DisplayMode::fade:
		this->renderTime(buf);
		this->set(buf, palette, false);
		this->fade();
		break;

	case DisplayMode::plain:
	default:
		this->renderTime(buf);
		this->set(buf, palette, true);
		break;
	}


	// transfer this->currentValues to LEDs
	this->show();

}

//---------------------------------------------------------------------------------------
// setBrightness
//
// Sets the brightness for the WS2812 values. Will be multiplied with each color
// component when sending data to WS2812.
//
// -> brightness: [0...256]
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::setBrightness(int brightness)
{
  if (brightness>0) 	{
    this->brightness = brightness;
  }else {
    this->brightness=1;
  }
}

//---------------------------------------------------------------------------------------
// setMode
//
// Sets the display mode to one of the members of the DisplayMode enum and thus changes
// what will be shown on the display during the next calls of LEDFunctionsClass.process()
//
// -> newMode: mode to be set
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::setMode(DisplayMode newMode)
{
	uint8_t buf[NUM_PIXELS];
	DisplayMode previousMode = this->mode;
	this->mode = newMode;

	// if we changed to an animated letters mode, then start animation
	// even if the current time did not yet change
	if(newMode != previousMode &&
			(newMode == DisplayMode::flyingLettersVerticalUp ||
			newMode == DisplayMode::flyingLettersVerticalDown))
	{
		this->renderTime(buf);
		this->prepareFlyingLetters(buf);
	}

	// if we changed to exploding letters mode, then start animation
	// even if the current time did not yet change
	if(newMode != previousMode && newMode == DisplayMode::explode)
	{
		// cleanup any existing particles to prevent memory leaks
		for(Particle *p : this->particles) delete p;
		this->particles.clear();
		
		this->renderTime(buf);
		this->prepareExplosion(buf);
	}

	// if we changed away from exploding letters mode, cleanup particles
	if(previousMode == DisplayMode::explode && newMode != DisplayMode::explode)
	{
		for(Particle *p : this->particles) delete p;
		this->particles.clear();
	}

	// this->process();
}

//---------------------------------------------------------------------------------------
// set
//
// Sets the internal LED buffer to new values based on an indexed source buffer and an
// associated palette. Does not display colors immediately, fades to new colors instead.
//
// Attention: If buf is PROGMEM, make sure it is aligned at 32 bit and its size is
// a multiple of 4 bytes!
//
// -> buf: indexed source buffer
//	palette: color definition for source buffer
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::set(const uint8_t *buf, palette_entry palette[])
{
	this->set(buf, palette, false);
}

//---------------------------------------------------------------------------------------
// set
//
// Sets the internal LED buffer to new values based on an indexed source buffer and an
// associated palette.
//
// Attention: If buf is PROGMEM, make sure it is aligned at 32 bit and its size is
// a multiple of 4 bytes!
//
// -> buf: indexed source buffer
//	  palette: color definition for source buffer
//	  immediately: if true, display buffer immediately; fade to new colors if false
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::set(const uint8_t *buf, palette_entry palette[],
		bool immediately)
{
  if (Config.nightmode) {
    palette_entry nightpalette[] = {
      {0, 0, 0},
      {0, 0, (uint8_t)(2*(255/this->brightness))},
      {0, 0, 0}
    };
    this->setBuffer(this->targetValues, buf, nightpalette);
    if (immediately)
    {
      this->setBuffer(this->currentValues, buf, nightpalette);
    }
    
  } else {
  	this->setBuffer(this->targetValues, buf, palette);
    if (immediately)
    {
  	  this->setBuffer(this->currentValues, buf, palette);
    }
  }
}

//---------------------------------------------------------------------------------------
// setBuffer
//
// Fills a buffer (e. g. this->targetValues) with color data based on indexed source
// pixels and a palette. Pays attention to 32 bit boundaries, so use with PROGMEM is
// safe.
//
// -> target: color buffer, e. g. this->targetValues or this->currentValues
//    source: buffer with color indexes
//	  palette: colors for indexed source buffer
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::setBuffer(uint8_t *target, const uint8_t *source,
		palette_entry palette[])
{
	uint32_t mapping, palette_index, curveOffset;

	// cast source to 32 bit pointer to ensure 32 bit aligned access
	uint32_t *buf = (uint32_t*) source;
	// this holds the current 4 bytes
	uint32_t currentDWord;
	// this is a pointer to the current 4 bytes for access as single bytes
	uint8_t *currentBytes = (uint8_t*) &currentDWord;
	// this counts bytes from 0...3
	uint32_t byteCounter = 0;
	for (int i = 0; i < NUM_PIXELS; i++)
	{
		// get next 4 bytes
		if (byteCounter == 0) currentDWord = buf[i >> 2];

		palette_index = currentBytes[byteCounter];
		mapping = LEDFunctionsClass::mapping[i] * 3;
		curveOffset = LEDFunctionsClass::brightnessCurveSelect[i] << 8;

		// select color value using palette and brightness correction curves
		target[mapping + 0] = brightnessCurvesR[curveOffset + palette[palette_index].r];
		target[mapping + 1] = brightnessCurvesG[curveOffset + palette[palette_index].g];
		target[mapping + 2] = brightnessCurvesB[curveOffset + palette[palette_index].b];

		byteCounter = (byteCounter + 1) & 0x03;
	}
}

//---------------------------------------------------------------------------------------
// renderRandomDots
//
// Fade one step of the color values from this->currentValues[i] to 
// this->targetValues[i]. Uses non-linear fade speed depending on distance to target
// value.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderRandomDots()
{
  if ((unsigned long)(millis() - this->lastUpdate) >= (unsigned)(100-Config.animspeed)*10) 
  {
    // set target
    for (int i=0;i<NUM_PIXELS*3;i++)
    {
      this->targetValues[i]=0;
    }

    // change current (for 1 dot)
    byte RandomDot=random(NUM_PIXELS);
    this->currentValues[RandomDot*3]=random(2)*255;
    this->currentValues[RandomDot*3+1]=random(2)*255;
    this->currentValues[RandomDot*3+2]=random(2)*255; 

    this->lastUpdate=millis();
  }
  
  this->fade();
}

//---------------------------------------------------------------------------------------
// renderRandomStripes
//
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderRandomStripes()
{
  if ((unsigned long)(millis() - this->lastUpdate) >= (unsigned)(100-Config.animspeed)*10) 
  {
    // set target
    for (int i=0;i<NUM_PIXELS*3;i++)
    {
      this->targetValues[i]=0;
    }

    palette_entry color = { (uint8_t)(random(2)*255), (uint8_t)(random(2)*255), (uint8_t)(random(2)*255) };

    // this->drawLine(this->targetValues,this->X1,this->Y1,this->X2,this->Y2,Config.fg);
    this->drawLine(this->targetValues,this->X1,this->Y1,this->X2,this->Y2,color);

    // calculate next coords
    this->X1+=random(3)-1;
    if (this->X1<0) this->X1=0;
    if (this->X1>10) this->X1=10;

    this->X2+=random(3)-1;
    if (this->X2<0) this->X2=0;
    if (this->X2>10) this->X2=10;

    this->Y1+=random(3)-1;
    if (this->Y1<0) this->Y1=0;
    if (this->Y1>9) this->Y1=9;

    this->Y2+=random(3)-1;
    if (this->Y2<0) this->Y2=0;
    if (this->Y2>9) this->Y2=9;


    this->lastUpdate=millis();
  }
  
  this->fade();
}

//---------------------------------------------------------------------------------------
// renderRotatingLine
//
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderRotatingLine()
{
  if ((unsigned long)(millis() - this->lastUpdate) >= (unsigned)(100-Config.animspeed)*10) 
  {
    // set target
    for (int i=0;i<NUM_PIXELS*3;i++)
    {
      this->targetValues[i]=0;
    }

    // palette_entry color = { random(2)*255, random(2)*255, random(2)*255 };
    palette_entry color[] = {
      {0, 0, 255},
      {0, 255, 0},
      {255, 0, 0},
      {255, 0, 255},
      {255, 255, 0},
      {0, 255, 255},
      {255, 255, 255}
    };

    // this->drawLine(this->targetValues,this->X1,this->Y1,10-this->X1,9-this->Y1,Config.fg);
    this->drawLine(this->targetValues,this->X1,this->Y1,10-this->X1,9-this->Y1,color[random(7)]);

    // calculate next coords
    if (this->Y1==0) {
      // we are on top row
      if (this->X1==10) {
        // we are on top right corner
        this->Y1++;
      } else {
        // we are somewhere else on the top row
        this->X1++;
      }
    } else if (this->Y1==9) {
      // we are on bottom row
      if (this->X1==0) {
        // we are on bottom left corner
        this->Y1--;
      } else {
        // we are somewhere else on bottom row
        this->X1--;
      }
    } else {
      // we are either on left or right col
      if (this->X1==0) {
        // we are on left col
        this->Y1--;
      } else {
        // we are on tright col
        this->Y1++;
      }
    }

    this->lastUpdate=millis();
  }
  
  this->fade();
}



//---------------------------------------------------------------------------------------
// fade
//
// Fade one step of the color values from this->currentValues[i] to 
// this->targetValues[i]. Uses non-linear fade speed depending on distance to target
// value.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::fade()
{
  if ((unsigned long)(millis() - this->lastFadeTick) >= FADEINTERVAL) 
  {
    this->lastFadeTick=millis();
    
  	static int prescaler = 0;
  	if(++prescaler<2) return;
  	prescaler = 0;
  
  
  	int delta;
  	for (int i = 0; i < NUM_PIXELS * 3; i++)
  	{
  		delta = this->targetValues[i] - this->currentValues[i];
      if (delta > 128) this->currentValues[i] += 32;
      else if (delta > 64) this->currentValues[i] += 16;
      else if (delta > 16) this->currentValues[i] += 8;
  		else if (delta > 0) this->currentValues[i]++;
      else if (delta < -128) this->currentValues[i] -= 32;
  		else if (delta < -64) this->currentValues[i] -= 16;
  		else if (delta < -16) this->currentValues[i] -= 8;
  		else if (delta < 0) this->currentValues[i]--;
  	}
  }
}

//---------------------------------------------------------------------------------------
// show
//
// Internal method, copies this->currentValues to WS2812 object while applying brightness
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::show()
{
	uint8_t *data = this->currentValues;
	int ofs = 0;

	// copy current color values to LED object and display it
	for (int i = 0; i < NUM_PIXELS; i++)
	{
  this->strip->SetPixelColor(i,RgbColor(((int) data[ofs + 0] * this->brightness) >> 8,
                                        ((int) data[ofs + 1] * this->brightness) >> 8,  
                                        ((int) data[ofs + 2] * this->brightness) >> 8));
    ofs += 3;
	}
  this->strip->Show();
}

//---------------------------------------------------------------------------------------
// getOffset
//
// Calculates the offset of a given RGB triplet inside the LED buffer.
// Does range checking for x and y, uses the internal mapping table.
//
// -> x: x coordinate
//    y: y coordinate
// <- offset for given coordinates
//---------------------------------------------------------------------------------------
int LEDFunctionsClass::getOffset(int x, int y)
{
	if (x>=0 && y>=0 && x<LEDFunctionsClass::width && y<LEDFunctionsClass::height)
	{
		return LEDFunctionsClass::mapping[x + y*LEDFunctionsClass::width] * 3;
	}
	else
	{
		return 0;
	}
}

//---------------------------------------------------------------------------------------
// fillBackground
//
// Initializes the buffer with either background (=0) or seconds progress (=2),
// part of the background will be illuminated with color 2 depending on current
// seconds/milliseconds value, whole screen is backlit when seconds = 59
//
// -> seconds, milliseconds: Time value which the fill process will base on
//    buf: destination buffer
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::fillBackground(int seconds, int milliseconds, uint8_t *buf)
{
	int pos = (((seconds * 1000 + milliseconds) * 110) / 60000) + 1;
	for (int i = 0; i < NUM_PIXELS; i++) buf[i] = (i < pos) ? 2 : 0;
}

//---------------------------------------------------------------------------------------
// fillTime
//
// Adds words and minutes to buffer), buffer needs to be initialized (e.g. by fillbackground)
//
// -> seconds, milliseconds: Time value which the fill process will base on
//    buf: destination buffer
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::fillTime(int h, int m, uint8_t *target)
{
  // set static LEDs
  target[0] = 1; // H
  target[1] = 1; // E
  target[2] = 1; // T

  target[4] = 1; // I
  target[5] = 1; // S

  // minutes 1...4 for the corners
  for(int i=0; i<=((m%5)-1); i++) target[10 * 11 + i] = 1;

  // iterate over minutes_template
  int adjust_hour = 0;
  for(leds_template_t t : LEDFunctionsClass::minutesTemplate)
  {
    // test if this template matches the current minute
    if(m >= t.param1 && m <= t.param2)
    {
      // set all LEDs defined in this template
      for(int i : t.LEDs) target[i] = 1;
      adjust_hour = t.param0;
      break;
    }
  }

  // adjust hour display if necessary (e. g. 09:45 = quarter to *TEN* instead of NINE)
  h += adjust_hour;
  if(h > 23)  h -= 24;

  // iterate over hours template
  for(leds_template_t t : LEDFunctionsClass::hoursTemplate)
  {
    // test if this template matches the current hour
    if((t.param1 == h || t.param2 == h) &&
       ((t.param0 == 1 && m < 5)  || // special case full hour
      (t.param0 == 2 && m >= 5) || // special case hour + minutes
      (t.param0 == 0)))            // normal case
    {
      // set all LEDs defined in this template
      for(int i : t.LEDs) target[i] = 1;
      break;
    }
  }

}


#endif

//---------------------------------------------------------------------------------------
#if 1 // rendering methods
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
// drawdot
//
// Draws a dot in a selected color
//
// -> target: buffer to be modified (either CurrentValues or TargetValues)
//    x,y: coordinate
//    palette: palette entry
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::drawDot(uint8_t *target, uint8_t x, uint8_t y, palette_entry palette)
{
  // get mapping

  int mapping = LEDFunctionsClass::mapping[y*11+x]*3;

  /* targetValues[mapping]=palette.r;
  targetValues[mapping+1]=palette.g;
  targetValues[mapping+2]=palette.b; */
  target[mapping]=palette.r;
  target[mapping+1]=palette.g;
  target[mapping+2]=palette.b;
  
}

//---------------------------------------------------------------------------------------
// drawline
//
// Draws a line in a selected color
//
// -> target: buffer to be modified: Either currentvalues or targetvalues
//    x1,y1: from coordinate
//    x2,y2: to coordinate
//    color: which color to draw
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::drawLine(uint8_t *target, int8_t x1, int8_t y1, int8_t x2, int8_t y2, palette_entry color)
{
  // calculate 
  // uint8_t number_of_dots = ( (abs(x2-x1)>abs(y2-y1)) ? abs(x2-x1) : abs(y2-y1)); // longest delta either in Y of X

  int8_t TotalDeltaX=abs(x2-x1);
  int8_t TotalDeltaY=abs(y2-y1);
  int number_of_dots=(TotalDeltaX>TotalDeltaY)?TotalDeltaX:TotalDeltaY;
  
  float deltaX = ((float) (x2-x1)) / (float) number_of_dots;
  float deltaY = ((float) (y2-y1)) / (float) number_of_dots;

  for (int i=0;i<=number_of_dots;i++)
  {
    this->drawDot(target , x1+deltaX*i+0.499 , y1+deltaY*i+0.499 , color);
  }
}  



//---------------------------------------------------------------------------------------
// renderRandom
//
// Loads internal buffers with random colors
//
// -> target: buffer to be filled
//    noofcolors: number of colors to populate
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderRandom(uint8_t *target)
{
    // Use static palette to avoid stack overflow and update less frequently
    static palette_entry palette[32];
    static unsigned long lastPaletteUpdate = 0;
    
    // Update palette every 2 seconds instead of every frame
    if (millis() - lastPaletteUpdate > 2000) {
        for (int i = 0; i < 32; i++) {
            palette[i] = {
                (uint8_t)random(256), 
                (uint8_t)random(256), 
                (uint8_t)random(256)
            };
        }
        lastPaletteUpdate = millis();
    }

    if ((unsigned long)(millis() - this->lastUpdate) >= (unsigned)(100-Config.animspeed)*20) 
    {
        for (int i=0;i<NUM_PIXELS;i++)
        {
            target[i]=(uint8_t)random(32);
        }
        this->lastUpdate=millis();
        this->set(target, palette, false);
    }
    this->fade();
}


//---------------------------------------------------------------------------------------
// renderStripes
//
// Loads internal buffers with stripes using palette
//
// -> target: buffer to be filled
//    noofcolors: number of colors to populate
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderStripes(uint8_t *target, bool Horizontal)
{
  int rows=10;
  int cols=11;
  int Offset=0;

  palette_entry palette[] = {
    {0, 0, 0},
    {255, 0, 0},
    {0, 255, 0},
    {0, 0, 255},
    {255, 255, 0},
    {255, 0, 255},
    {0, 255, 255},
    {255, 255, 255}
  };

  
  if ((unsigned long)(millis() - this->lastUpdate) >= (unsigned)(100-Config.animspeed)*20) 
  {
    byte TargetCol=random(8);
    // fill 4 dots
    for (int i=0; i<4; i++){
      target[rows*cols+i]=TargetCol;
      Offset++;
    }

    if (Horizontal)
    {
      for (int y=0;y<rows;y++){
        byte TargetCol=random(8);
        for (int x=0;x<cols;x++){
          target[y*cols+x]=TargetCol;
        }
        Offset++;
      }
    } else {
      for (int x=0;x<cols;x++){
        byte TargetCol=random(8);
        for (int y=0;y<rows;y++){
          target[y*cols+x]=TargetCol;
        }
        Offset++;
      }
    }
	this->lastOffset++;
    this->lastUpdate=millis();
    this->set(target, palette, false);
  }
  this->fade();
}



//---------------------------------------------------------------------------------------
// renderTime
//
// Loads internal buffers with the current time representation
//
// -> target: If NULL, a local buffer is used and time is actually being displayed
//            if not null, time is not being displayed, but the given buffer is being
//            filled with palette indexes representing the time
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderTime(uint8_t *target)
{
	this->fillBackground(NTP.s, NTP.ms, target);
  this->fillTime(NTP.h, NTP.m, target);

	// DEBUG
	static int last_minutes = -1;
	if(last_minutes != NTP.m)
	{
		last_minutes = NTP.m;
		Serial.printf("h=%i, m=%i, s=%i\r\n", NTP.h, NTP.m, NTP.s);
		for(int y=0; y<10; y++)
		{
			for(int x=0; x<11; x++)
			{
				Serial.print(target[y*11+x]);
				Serial.print(' ');
			}
			Serial.println(' ');
		}
	}
}

//---------------------------------------------------------------------------------------
// renderHourglass
//
// Immediately displays an animation step of the hourglass animation.
// ATTENTION: Animation frames must start at 32 bit boundary each!
//
// -> animationStep: Number of current frame [0...HOURGLASS_ANIMATION_FRAMES]
//    green: Flag to switch the palette color 3 to green instead of yellow (used in the
//           second half of the hourglass animation to indicate the short wait-for-OTA
//           window)
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderHourglass(bool green)
{
  if ((unsigned long) (millis()-this->lastUpdate)>100)
  {
    this->lastUpdate=millis();
    if (++this->hourglassState >= HOURGLASS_ANIMATION_FRAMES)
      this->hourglassState = 0;
  }
  
	// colors in palette: black, white, yellow
	palette_entry p[] = {{0, 0, 0}, {255, 255, 255}, {255, 255, 0}, {255, 255, 0}};

	// delete red component in palette entry 3 to make this color green
	if(green) p[3].r = 0;

	if (this->hourglassState >= HOURGLASS_ANIMATION_FRAMES) this->hourglassState = 0;
	this->set(hourglass_animation[this->hourglassState], p, true);
}

//---------------------------------------------------------------------------------------
// renderMatrix
//
// Renders one frame of the matrix animation and displays it immediately
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderMatrix()
{
  // if ((unsigned long) (millis()-this->lastUpdate)>(unsigned)(100-Config.animspeed))
  if ((unsigned long) (millis()-this->lastUpdate)>MATRIXINTERVAL)
  {
    this->lastUpdate=millis();
  	// clear buffer
  	memset(this->currentValues, 0, sizeof(this->currentValues));
  
  	// sort by y coordinate for correct overlapping
  	std::sort(matrix.begin(), matrix.end());
  
  	// iterate over all matrix objects, move and render them
  	for (MatrixObject &m : this->matrix) m.render(this->currentValues);
  }
}

const palette_entry LEDFunctionsClass::firePalette[256] = {
	{0, 0, 0}, {4, 0, 0}, {8, 0, 0}, {12, 0, 0}, {16, 0, 0}, {20, 0, 0}, {24, 0, 0}, {28, 0, 0},
	{32, 0, 0}, {36, 0, 0}, {40, 0, 0}, {44, 0, 0}, {48, 0, 0}, {52, 0, 0}, {56, 0, 0}, {60, 0, 0},
	{64, 0, 0}, {68, 0, 0}, {72, 0, 0}, {76, 0, 0}, {80, 0, 0}, {85, 0, 0}, {89, 0, 0}, {93, 0, 0},
	{97, 0, 0}, {101, 0, 0}, {105, 0, 0}, {109, 0, 0}, {113, 0, 0}, {117, 0, 0}, {121, 0, 0}, {125, 0, 0},
	{129, 0, 0}, {133, 0, 0}, {137, 0, 0}, {141, 0, 0}, {145, 0, 0}, {149, 0, 0}, {153, 0, 0}, {157, 0, 0},
	{161, 0, 0}, {165, 0, 0}, {170, 0, 0}, {174, 0, 0}, {178, 0, 0}, {182, 0, 0}, {186, 0, 0}, {190, 0, 0},
	{194, 0, 0}, {198, 0, 0}, {202, 0, 0}, {206, 0, 0}, {210, 0, 0}, {214, 0, 0}, {218, 0, 0}, {222, 0, 0},
	{226, 0, 0}, {230, 0, 0}, {234, 0, 0}, {238, 0, 0}, {242, 0, 0}, {246, 0, 0}, {250, 0, 0}, {255, 0, 0},
	{255, 0, 0}, {255, 8, 0}, {255, 16, 0}, {255, 24, 0}, {255, 32, 0}, {255, 41, 0}, {255, 49, 0}, {255, 57, 0},
	{255, 65, 0}, {255, 74, 0}, {255, 82, 0}, {255, 90, 0}, {255, 98, 0}, {255, 106, 0}, {255, 115, 0}, {255, 123, 0},
	{255, 131, 0}, {255, 139, 0}, {255, 148, 0}, {255, 156, 0}, {255, 164, 0}, {255, 172, 0}, {255, 180, 0}, {255, 189, 0},
	{255, 197, 0}, {255, 205, 0}, {255, 213, 0}, {255, 222, 0}, {255, 230, 0}, {255, 238, 0}, {255, 246, 0}, {255, 255, 0},
	{255, 255, 0}, {255, 255, 1}, {255, 255, 3}, {255, 255, 4}, {255, 255, 6}, {255, 255, 8}, {255, 255, 9}, {255, 255, 11},
	{255, 255, 12}, {255, 255, 14}, {255, 255, 16}, {255, 255, 17}, {255, 255, 19}, {255, 255, 20}, {255, 255, 22}, {255, 255, 24},
	{255, 255, 25}, {255, 255, 27}, {255, 255, 28}, {255, 255, 30}, {255, 255, 32}, {255, 255, 33}, {255, 255, 35}, {255, 255, 36},
	{255, 255, 38}, {255, 255, 40}, {255, 255, 41}, {255, 255, 43}, {255, 255, 44}, {255, 255, 46}, {255, 255, 48}, {255, 255, 49},
	{255, 255, 51}, {255, 255, 52}, {255, 255, 54}, {255, 255, 56}, {255, 255, 57}, {255, 255, 59}, {255, 255, 60}, {255, 255, 62},
	{255, 255, 64}, {255, 255, 65}, {255, 255, 67}, {255, 255, 68}, {255, 255, 70}, {255, 255, 72}, {255, 255, 73}, {255, 255, 75},
	{255, 255, 76}, {255, 255, 78}, {255, 255, 80}, {255, 255, 81}, {255, 255, 83}, {255, 255, 85}, {255, 255, 86}, {255, 255, 88},
	{255, 255, 89}, {255, 255, 91}, {255, 255, 93}, {255, 255, 94}, {255, 255, 96}, {255, 255, 97}, {255, 255, 99}, {255, 255, 101},
	{255, 255, 102}, {255, 255, 104}, {255, 255, 105}, {255, 255, 107}, {255, 255, 109}, {255, 255, 110}, {255, 255, 112}, {255, 255, 113},
	{255, 255, 115}, {255, 255, 117}, {255, 255, 118}, {255, 255, 120}, {255, 255, 121}, {255, 255, 123}, {255, 255, 125}, {255, 255, 126},
	{255, 255, 128}, {255, 255, 129}, {255, 255, 131}, {255, 255, 133}, {255, 255, 134}, {255, 255, 136}, {255, 255, 137}, {255, 255, 139},
	{255, 255, 141}, {255, 255, 142}, {255, 255, 144}, {255, 255, 145}, {255, 255, 147}, {255, 255, 149}, {255, 255, 150}, {255, 255, 152},
	{255, 255, 153}, {255, 255, 155}, {255, 255, 157}, {255, 255, 158}, {255, 255, 160}, {255, 255, 161}, {255, 255, 163}, {255, 255, 165},
	{255, 255, 166}, {255, 255, 168}, {255, 255, 170}, {255, 255, 171}, {255, 255, 173}, {255, 255, 174}, {255, 255, 176}, {255, 255, 178},
	{255, 255, 179}, {255, 255, 181}, {255, 255, 182}, {255, 255, 184}, {255, 255, 186}, {255, 255, 187}, {255, 255, 189}, {255, 255, 190},
	{255, 255, 192}, {255, 255, 194}, {255, 255, 195}, {255, 255, 197}, {255, 255, 198}, {255, 255, 200}, {255, 255, 202}, {255, 255, 203},
	{255, 255, 205}, {255, 255, 206}, {255, 255, 208}, {255, 255, 210}, {255, 255, 211}, {255, 255, 213}, {255, 255, 214}, {255, 255, 216},
	{255, 255, 218}, {255, 255, 219}, {255, 255, 221}, {255, 255, 222}, {255, 255, 224}, {255, 255, 226}, {255, 255, 227}, {255, 255, 229},
	{255, 255, 230}, {255, 255, 232}, {255, 255, 234}, {255, 255, 235}, {255, 255, 237}, {255, 255, 238}, {255, 255, 240}, {255, 255, 242},
	{255, 255, 243}, {255, 255, 245}, {255, 255, 246}, {255, 255, 248}, {255, 255, 250}, {255, 255, 251}, {255, 255, 253}, {255, 255, 255}
};
const palette_entry LEDFunctionsClass::plasmaPalette[256] = {
	{255, 0, 0}, {255, 6, 0}, {255, 12, 0}, {255, 18, 0}, {255, 24, 0}, {255, 30, 0}, {255, 36, 0}, {255, 42, 0},
	{255, 48, 0}, {255, 54, 0}, {255, 60, 0}, {255, 66, 0}, {255, 72, 0}, {255, 78, 0}, {255, 84, 0}, {255, 90, 0},
	{255, 96, 0}, {255, 102, 0}, {255, 108, 0}, {255, 114, 0}, {255, 120, 0}, {255, 126, 0}, {255, 131, 0}, {255, 137, 0},
	{255, 143, 0}, {255, 149, 0}, {255, 155, 0}, {255, 161, 0}, {255, 167, 0}, {255, 173, 0}, {255, 179, 0}, {255, 185, 0},
	{255, 191, 0}, {255, 197, 0}, {255, 203, 0}, {255, 209, 0}, {255, 215, 0}, {255, 221, 0}, {255, 227, 0}, {255, 233, 0},
	{255, 239, 0}, {255, 245, 0}, {255, 251, 0}, {253, 255, 0}, {247, 255, 0}, {241, 255, 0}, {235, 255, 0}, {229, 255, 0},
	{223, 255, 0}, {217, 255, 0}, {211, 255, 0}, {205, 255, 0}, {199, 255, 0}, {193, 255, 0}, {187, 255, 0}, {181, 255, 0},
	{175, 255, 0}, {169, 255, 0}, {163, 255, 0}, {157, 255, 0}, {151, 255, 0}, {145, 255, 0}, {139, 255, 0}, {133, 255, 0},
	{128, 255, 0}, {122, 255, 0}, {116, 255, 0}, {110, 255, 0}, {104, 255, 0}, {98, 255, 0}, {92, 255, 0}, {86, 255, 0},
	{80, 255, 0}, {74, 255, 0}, {68, 255, 0}, {62, 255, 0}, {56, 255, 0}, {50, 255, 0}, {44, 255, 0}, {38, 255, 0},
	{32, 255, 0}, {26, 255, 0}, {20, 255, 0}, {14, 255, 0}, {8, 255, 0}, {2, 255, 0}, {0, 255, 4}, {0, 255, 10},
	{0, 255, 16}, {0, 255, 22}, {0, 255, 28}, {0, 255, 34}, {0, 255, 40}, {0, 255, 46}, {0, 255, 52}, {0, 255, 58},
	{0, 255, 64}, {0, 255, 70}, {0, 255, 76}, {0, 255, 82}, {0, 255, 88}, {0, 255, 94}, {0, 255, 100}, {0, 255, 106},
	{0, 255, 112}, {0, 255, 118}, {0, 255, 124}, {0, 255, 129}, {0, 255, 135}, {0, 255, 141}, {0, 255, 147}, {0, 255, 153},
	{0, 255, 159}, {0, 255, 165}, {0, 255, 171}, {0, 255, 177}, {0, 255, 183}, {0, 255, 189}, {0, 255, 195}, {0, 255, 201},
	{0, 255, 207}, {0, 255, 213}, {0, 255, 219}, {0, 255, 225}, {0, 255, 231}, {0, 255, 237}, {0, 255, 243}, {0, 255, 249},
	{0, 255, 255}, {0, 249, 255}, {0, 243, 255}, {0, 237, 255}, {0, 231, 255}, {0, 225, 255}, {0, 219, 255}, {0, 213, 255},
	{0, 207, 255}, {0, 201, 255}, {0, 195, 255}, {0, 189, 255}, {0, 183, 255}, {0, 177, 255}, {0, 171, 255}, {0, 165, 255},
	{0, 159, 255}, {0, 153, 255}, {0, 147, 255}, {0, 141, 255}, {0, 135, 255}, {0, 129, 255}, {0, 124, 255}, {0, 118, 255},
	{0, 112, 255}, {0, 106, 255}, {0, 100, 255}, {0, 94, 255}, {0, 88, 255}, {0, 82, 255}, {0, 76, 255}, {0, 70, 255},
	{0, 64, 255}, {0, 58, 255}, {0, 52, 255}, {0, 46, 255}, {0, 40, 255}, {0, 34, 255}, {0, 28, 255}, {0, 22, 255},
	{0, 16, 255}, {0, 10, 255}, {0, 4, 255}, {2, 0, 255}, {8, 0, 255}, {14, 0, 255}, {20, 0, 255}, {26, 0, 255},
	{32, 0, 255}, {38, 0, 255}, {44, 0, 255}, {50, 0, 255}, {56, 0, 255}, {62, 0, 255}, {68, 0, 255}, {74, 0, 255},
	{80, 0, 255}, {86, 0, 255}, {92, 0, 255}, {98, 0, 255}, {104, 0, 255}, {110, 0, 255}, {116, 0, 255}, {122, 0, 255},
	{128, 0, 255}, {133, 0, 255}, {139, 0, 255}, {145, 0, 255}, {151, 0, 255}, {157, 0, 255}, {163, 0, 255}, {169, 0, 255},
	{175, 0, 255}, {181, 0, 255}, {187, 0, 255}, {193, 0, 255}, {199, 0, 255}, {205, 0, 255}, {211, 0, 255}, {217, 0, 255},
	{223, 0, 255}, {229, 0, 255}, {235, 0, 255}, {241, 0, 255}, {247, 0, 255}, {253, 0, 255}, {255, 0, 251}, {255, 0, 245},
	{255, 0, 239}, {255, 0, 233}, {255, 0, 227}, {255, 0, 221}, {255, 0, 215}, {255, 0, 209}, {255, 0, 203}, {255, 0, 197},
	{255, 0, 191}, {255, 0, 185}, {255, 0, 179}, {255, 0, 173}, {255, 0, 167}, {255, 0, 161}, {255, 0, 155}, {255, 0, 149},
	{255, 0, 143}, {255, 0, 137}, {255, 0, 131}, {255, 0, 126}, {255, 0, 120}, {255, 0, 114}, {255, 0, 108}, {255, 0, 102},
	{255, 0, 96}, {255, 0, 90}, {255, 0, 84}, {255, 0, 78}, {255, 0, 72}, {255, 0, 66}, {255, 0, 60}, {255, 0, 54},
	{255, 0, 48}, {255, 0, 42}, {255, 0, 36}, {255, 0, 30}, {255, 0, 24}, {255, 0, 18}, {255, 0, 12}, {255, 0, 6}
};


double _time = 0;
void LEDFunctionsClass::renderPlasma()
{
  // if ((unsigned long) (millis()-this->lastUpdate)>(100-Config.animspeed)*2)
  if ((unsigned long) (millis()-this->lastUpdate)>10)
  {
    this->lastUpdate=millis();
    int color;
    double cx, cy, xx, yy;

    _time += 0.025;

    for (int y=0; y<LEDFunctionsClass::height; y++)
    {
        yy = (double)y / (double)LEDFunctionsClass::height / 3.0;
        for (int x=0; x<LEDFunctionsClass::width; x++)
        {
            xx = (double)x / (double)LEDFunctionsClass::width / 3.0;
            cx = xx + 0.5 * sin(_time / 5.0);
            cy = (double)y/(double)LEDFunctionsClass::height / 3.0 + 0.5 * sin(_time / 3.0);
            color = (
            	sin(
                    sqrt(100 * (cx*cx + cy*cy) + 1 + _time) +
                    6.0 * (xx * sin(_time/2) + yy * cos(_time/3) + _time / 4.0)
                ) + 1.0
			) * 128.0;
            plasmaBuf[x + y * LEDFunctionsClass::width] = color;
        }
    }
    this->set(plasmaBuf, (palette_entry*)plasmaPalette, true);
  }
}

void LEDFunctionsClass::renderFire()
{
  if ((unsigned long) (millis()-this->lastUpdate)>FIREINTERVAL)
  {
    this->lastUpdate=millis();
    int f;

    // iterate over bottom row, create fire seed
    for (int i = 0; i < LEDFunctionsClass::width; i++)
    {
        // only set hot spot with probability of 1/4
        f = (random(4) == 0) ? random(256) : 0;

        // update one pixel in bottom row
        fireBuf[i + (LEDFunctionsClass::height - 1) * LEDFunctionsClass::width] = f;
    }

    int y1, y2, l, r;
    for (int y = 0; y < LEDFunctionsClass::height - 1; y++)
    {
        y1 = y + 1; if (y1 >= LEDFunctionsClass::height) y1 = LEDFunctionsClass::height - 1;
        y2 = y + 2; if (y2 >= LEDFunctionsClass::height) y2 = LEDFunctionsClass::height - 1;
        for (int x = 0; x < LEDFunctionsClass::width; x++)
        {
            l = x - 1; if (l < 0) l = 0;
            r = x + 1; if (r >= LEDFunctionsClass::width) r = LEDFunctionsClass::width - 1;
            fireBuf[x + y * LEDFunctionsClass::width] =
                ((fireBuf[y1 * LEDFunctionsClass::width + l]
                + fireBuf[y1 * LEDFunctionsClass::width + x]
                + fireBuf[y1 * LEDFunctionsClass::width + r]
                + fireBuf[y2 * LEDFunctionsClass::width + x])
                * 32) / 129;
        }
    }
    this->set(fireBuf, (palette_entry*)firePalette, true);
  }
}

//---------------------------------------------------------------------------------------
// renderStars
//
// Renders one frame of the stars animation and displays it immediately
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderStars()
{
  if ((unsigned long) (millis()-this->lastUpdate)>(unsigned)(100-Config.animspeed))
  {
    this->lastUpdate=millis();
  	// clear buffer
  	memset(this->currentValues, 0, sizeof(this->currentValues));
  
  	for(StarObject &s : this->stars) s.render(this->currentValues, this->stars);
  }
}

//---------------------------------------------------------------------------------------
// renderChristmasTree
//
// Renders a Christmas tree with twinkling balls and stars
// 0 = off, 1 = tree (green), 2 = trunk (brown)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderChristmasTree()
{
  if ((unsigned long) (millis()-this->lastUpdate)>(unsigned)(300-Config.animspeed))
  {
    this->lastUpdate=millis();

	// Christmas tree pattern (11x10 grid)
	// 0=off, 1=tree(green), 2=trunk(brown)
	uint8_t tree[NUM_PIXELS] = {
	// Row 0 (indices 0-10) - Top Star / Tip
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	// Row 1 (indices 11-21) - First layer (Narrow)
	0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
	// Row 2 (indices 22-32) - Second layer (Wide)
	0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
	// Row 3 (indices 33-43) - Third layer (Narrow)
	0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
	// Row 4 (indices 44-54) - Fourth layer (Wide) - Small inkeping
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	// Row 5 (indices 55-65) - Fifth layer (Narrow)
	0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
	// Row 6 (indices 66-76) - Sixth layer (Wide) - Diepere inkeping
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
	// Row 7 (indices 77-87) - Seventh layer (Narrow)
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // Volle rij als basis van de boom
	// Row 8 (indices 88-98) - Trunk space
	0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0,
	// Row 9 (indices 99-109) - Trunk
	0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, // Stam van 3 pixels breed

	// Corner LEDs (110-113) - off (Indien nodig voor uw hardware)
	0, 0, 0, 0
	};
    
    
    // Render the static tree
    for(int i = 0; i < NUM_PIXELS; i++)
    {
      switch(tree[i])
      {
        case 1: // Tree - green
          this->targetValues[i*3] = 0; // R
          this->targetValues[i*3+1] = 150; // G
          this->targetValues[i*3+2] = 0; // B
          break;
          
        case 2: // Trunk - brown
          this->targetValues[i*3] = 101; // R
          this->targetValues[i*3+1] = 67; // G
          this->targetValues[i*3+2] = 33; // B
          break;
          
        default: // 0 - off (black)
          this->targetValues[i*3] = 0;
          this->targetValues[i*3+1] = 0;
          this->targetValues[i*3+2] = 0;
          break;
      }
    }
    
    // Add random twinkling balls on green tree parts (value 1)
    for(int i = 0; i < NUM_PIXELS; i++)
    {
      if(tree[i] == 1 && random(100) < 2) // 2% chance on tree parts
      {
        int color = random(3); // 0 red, 1 blue, 2 gold
        if(color == 0)
        {
          this->currentValues[i*3] = 255; // R
          this->currentValues[i*3+1] = 0; // G
          this->currentValues[i*3+2] = 0; // B
        }
        else if(color == 1)
        {
          this->currentValues[i*3] = 0; // R
          this->currentValues[i*3+1] = 0; // G
          this->currentValues[i*3+2] = 255; // B
        }
        else
        {
          this->currentValues[i*3] = 255; // R
          this->currentValues[i*3+1] = 255; // G
          this->currentValues[i*3+2] = 0; // B (gold)
        }
      }
    }
    
    // Add random twinkling stars on off parts (value 0)
    for(int i = 0; i < NUM_PIXELS; i++)
    {
      if(tree[i] == 0 && random(200) < 2)// 1% chance on off parts
      {
        this->currentValues[i*3] = 255; // R
        this->currentValues[i*3+1] = 255; // G
        this->currentValues[i*3+2] = 255; // B (white)
      }
    }
    
  }
  this->fade();
}

//---------------------------------------------------------------------------------------
// renderJingleBells
//
// Renders a jingling bell that gently swings left-right
// Bell colors: gold body, red ribbon, grey clapper
// Slight swing: horizontally shifts outline by -1..+1 over time
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderJingleBells()
{
	if ((unsigned long) (millis()-this->lastUpdate)>(unsigned)(200-Config.animspeed))
	{
		this->lastUpdate=millis();

		// Base pattern indices: 11x10 grid, values:
		// 0=off, 1=bell gold, 2=ribbon red, 3=clapper grey
		uint8_t bell[NUM_PIXELS] = {
			// Row 0
			0,0,0,0,2,2,2,0,0,0,0,
			// Row 1
			0,0,0,1,1,1,1,1,0,0,0,
			// Row 2
			0,0,1,1,1,1,1,1,1,0,0,
			// Row 3
			0,1,1,1,1,1,1,1,1,1,0,
			// Row 4
			0,1,1,1,1,1,1,1,1,1,0,
			// Row 5
			0,1,1,1,1,1,1,1,1,1,0,
			// Row 6
			0,0,1,1,1,1,1,1,1,0,0,
			// Row 7
			0,0,0,1,1,1,1,1,0,0,0,
			// Row 8 (clapper)
			0,0,0,0,0,3,0,0,0,0,0,
			// Row 9 (shadow/off)
			0,0,0,0,0,0,0,0,0,0,0,
			// corners
			0,0,0,0
		};

		// Compute swing offset based on lastOffset: -1,0,+1 cycling
		int swingPhase = (this->lastOffset % 6);
		int xShift = (swingPhase < 2) ? -1 : (swingPhase < 4 ? 0 : 1);
		this->lastOffset++;

		// Clear target
		for(int i=0;i<NUM_PIXELS*3;i++) this->targetValues[i]=0;

		// Render with horizontal shift
		for(int y=0;y<10;y++)
		{
			for(int x=0;x<11;x++)
			{
				int srcIdx = y*11 + x;
				uint8_t v = bell[srcIdx];
				int tx = x + xShift;
				if(tx<0 || tx>=11) continue;
				int ofs = LEDFunctionsClass::mapping[y*11 + tx]*3;
				if(v==1) { // gold
					this->targetValues[ofs+0]=255;
					this->targetValues[ofs+1]=200;
					this->targetValues[ofs+2]=0;
				} else if(v==2) { // red ribbon
					this->targetValues[ofs+0]=220;
					this->targetValues[ofs+1]=0;
					this->targetValues[ofs+2]=0;
				} else if(v==3) { // grey clapper
					this->targetValues[ofs+0]=150;
					this->targetValues[ofs+1]=150;
					this->targetValues[ofs+2]=150;
				}
			}
		}

		// Occasional sparkle on bell body
		for(int i=0;i<NUM_PIXELS;i++)
		{
			int y=i/11, x=i%11;
			int tx=x+xShift; if(tx<0||tx>=11) continue;
			int srcIdx=y*11+x;
			if(bell[srcIdx]==1 && random(120)<2)
			{
				int ofs = LEDFunctionsClass::mapping[y*11 + tx]*3;
				this->currentValues[ofs+0]=255;
				this->currentValues[ofs+1]=255;
				this->currentValues[ofs+2]=180;
			}
		}
	}
	this->fade();
}

//---------------------------------------------------------------------------------------
// blendedColor
//
// calculates the correct in between color based on progress
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
palette_entry LEDFunctionsClass::blendedColor(palette_entry from_color, palette_entry to_color, float progress)
{
  palette_entry resulting_color = { (uint8_t)(from_color.r + progress*(to_color.r - from_color.r)), 
                                    (uint8_t)(from_color.g + progress*(to_color.g - from_color.g)), 
                                    (uint8_t)(from_color.b + progress*(to_color.b - from_color.b)) };  
  return resulting_color;
}

//---------------------------------------------------------------------------------------
// renderMerryChristmas
//
// Renders "MERRY CHRISTMAS" scrolling horizontally with wrong sweater colors
// (bright red, green, yellow) creating a festive ugly Christmas sweater effect
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderMerryChristmas()
{
	// Linear mapping: animspeed 0->500ms, 100->40ms, then 5x faster for rockets
	unsigned int delay = (500 - (Config.animspeed * 460 / 100)) / 5;
	if (delay < 10) delay = 10;
  if ((unsigned long) (millis()-this->lastUpdate) > delay)
  {
    this->lastUpdate = millis();

    // Clear all pixels
    memset(this->currentValues, 0, sizeof(this->currentValues));
    
	// Per-item widths: sprite(21)+1 gap =22, letters 5+1=6
	int letterWidths[20] = {22, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 22, 6};
	bool isSprite[20] = {true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true,false};
	int totalTextWidth = 0;
	for (int i = 0; i < 20; i++) totalTextWidth += letterWidths[i];

		// Total scroll length based on actual widths
		// Add 11 pixels to scroll off screen + 5 pixels pause
		int totalScrollLength = totalTextWidth + 11 + 5;

		// Randomize colors at start (first entry) only once per cycle
		if (!this->merryChristmasColorsInitialized || this->lastOffset == 0)
		{
			this->merryChristmasColors[0] = random(3);
			for (int i = 1; i < 20; i++)
			{
				// Avoid same color as previous letter
				this->merryChristmasColors[i] = (this->merryChristmasColors[i-1] + 1 + random(2)) % 3;
			}
			this->merryChristmasColorsInitialized = true;
		}

		// Update scroll offset
		this->lastOffset++;

		if (this->lastOffset > totalScrollLength)
		{
			this->lastOffset = 0;
		}
    
	// Check if we're in the pause period (after text scrolled off)
	if (this->lastOffset > totalTextWidth + 11)
    {
      // Pause period - keep screen dark
      return;
    }
    
		// Combined Santa + sleigh + reindeer sprite (21x10), RGB per pixel
		static const uint8_t santaSleighSprite[10][21][3] PROGMEM = {
		  { {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,255,255}, {  0,  0,  0}, {  0,  0,  0}},
		  { {  0,  0,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,  0,  0}, {255,255,255}, {  0,  0,  0}},
		  { {  0,  0,  0}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,255,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,  0,  0}, {255,  0,  0}, {255,255,255}},
		  { {  0,  0,  0}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,255,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,255,213}, {255,255,213}, {255,255,255}, {255,255,255}},
		  { {255,142, 71}, {255,142, 71}, {  0,  0,  0}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,255,  0}, {  0,  0,  0}, {142, 71,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {255,  0,  0}, {255,255,255}, {255,255,255}},
		  { {  0,  0,  0}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {255,255,  0}, {255,255,  0}, {255,255,  0}, {255,255,  0}, {255,255,  0}, {255,255,  0}, {142, 71,  0}, {142, 71,  0}, {142, 71,  0}, {255,  0,  0}, {255,  0,  0}, {255,  0,  0}},
		  { {  0,  0,  0}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {255,142, 71}, {255,142, 71}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {142, 71,  0}, {142, 71,  0}, {142, 71,  0}, {255,  0,  0}, {255,  0,  0}, {142, 71,  0}},
		  { {  0,  0,  0}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {255,142, 71}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {142, 71,  0}, {142, 71,  0}, {142, 71,  0}, {142, 71,  0}, {142, 71,  0}, {142, 71,  0}, {142, 71,  0}},
		  { {  0,  0,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {  0,  0,  0}, {113, 57,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {213,213,213}, {213,213,213}, {213,213,213}, {213,213,213}, {213,213,213}, {213,213,213}},
		  { {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {  0,  0,  0}, {213,213,213}, {213,213,213}, {213,213,213}, {213,213,213}, {213,213,213}, {213,213,213}}
		};

		// Sequence: sprite + space + "MERRY CHRISTMAS" + space + sprite + trailing gap
		const uint8_t *letters[] = {
			nullptr, LETTER_SPACE, LETTER_M_CAP, LETTER_e_LOW, LETTER_r_LOW, LETTER_r_LOW, LETTER_y_LOW, LETTER_SPACE,
			LETTER_C_CAP, LETTER_h_LOW, LETTER_r_LOW, LETTER_i_LOW, LETTER_s_LOW, LETTER_t_LOW, LETTER_m_LOW, LETTER_a_LOW, LETTER_s_LOW, LETTER_SPACE,
			nullptr, LETTER_SPACE
		};
		int numLetters = 20;
    
    // For each Y row (0-9)
    for (int y = 0; y < 10; y++)
    {
      // For each X position on screen (0-10)
      for (int x = 0; x < 11; x++)
      {
				// Calculate position in scrolling text
				int scrollPos = (x + this->lastOffset - 11);
        
				// Skip if out of bounds
				if (scrollPos < 0 || scrollPos >= totalTextWidth) continue;
        
				// Determine letter index and position using variable widths
				int accum = 0;
				int letterIndex = -1;
				int posInLetter = -1;
				for (int i = 0; i < numLetters; i++)
				{
					int w = letterWidths[i];
					if (scrollPos >= accum && scrollPos < accum + w)
					{
						letterIndex = i;
						posInLetter = scrollPos - accum;
						break;
					}
					accum += w;
				}
				if (letterIndex < 0) continue;

				bool pixelOn = false;
				uint8_t r = 0, g = 0, b = 0;

				int glyphWidth = isSprite[letterIndex] ? 21 : 5;
				if (posInLetter >= glyphWidth) continue; // skip spacer column

				if (isSprite[letterIndex])
				{
					r = pgm_read_byte(&(santaSleighSprite[y][posInLetter][0]));
					g = pgm_read_byte(&(santaSleighSprite[y][posInLetter][1]));
					b = pgm_read_byte(&(santaSleighSprite[y][posInLetter][2]));
					pixelOn = (r != 0 || g != 0 || b != 0);
				}
				else
				{
					uint8_t pattern = letters[letterIndex][y];
					int bitPos = 4 - posInLetter;
					pixelOn = (pattern & (1 << bitPos)) != 0;
					if (pixelOn)
					{
						int colorPhase = this->merryChristmasColors[letterIndex];
						if (colorPhase == 0)
						{
							r = 255; g = 0; b = 0;      // Red
						}
						else if (colorPhase == 1)
						{
							r = 0; g = 255; b = 0;      // Green
						}
						else
						{
							r = 255; g = 220; b = 0;    // Yellow
						}
					}
				}

				if (pixelOn)
				{
					int mappedIndex = LEDFunctionsClass::mapping[y * 11 + x] * 3;
					this->currentValues[mappedIndex] = r;
					this->currentValues[mappedIndex + 1] = g;
					this->currentValues[mappedIndex + 2] = b;
				}
      }
    }
    
    // Keep corner LEDs (110-113) off - they are already cleared by memset
  }
  
  // this->fade();
}

//---------------------------------------------------------------------------------------
// renderHappyNewYear
//
// Renders fireworks animation followed by "Happy New Year" scrolling text
// Reuses particle system from renderExplosion for fireworks
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderHappyNewYear()
{
  // Linear mapping: animspeed 0->500ms, 100->40ms
  unsigned int delay = 500 - (Config.animspeed * 460 / 100);
  if ((unsigned long) (millis()-this->lastUpdate) > delay)
  {
    this->lastUpdate = millis();

    // Clear all pixels
    memset(this->currentValues, 0, sizeof(this->currentValues));
    
	// State machine: 
	// Offset 0-120: Show fireworks (3 rockets)
	// Offset 120+: Show text
    
	// Calculate cycle lengths
	int fireworksDuration = 120;
    
    // Letter widths for "Happy New Year": H=6, a=6, p=6, p=6, y=6, space=6, N=6, e=6, w=6, space=6, Y=6, e=6, a=6, r=6
    int letterWidths[14] = {6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};
    int totalTextWidth = 0;
    for (int i = 0; i < 14; i++) totalTextWidth += letterWidths[i];
    int totalScrollLength = totalTextWidth + 11 + 5; // Text + scroll off + pause
    
		int totalCycleLength = fireworksDuration + totalScrollLength;

		// Randomize colors at start of cycle (first run) only once per cycle
		if (!this->happyNewYearColorsInitialized || this->lastOffset == 0)
		{
			this->happyNewYearColors[0] = random(3);
			for (int i = 1; i < 14; i++)
			{
				this->happyNewYearColors[i] = (this->happyNewYearColors[i-1] + 1 + random(2)) % 3;
			}
			this->happyNewYearColorsInitialized = true;
		}

		// Advance offset
		this->lastOffset++;

		if (this->lastOffset > totalCycleLength)
		{
			this->lastOffset = 0;
		}
    
    // Fireworks phase
    if (this->lastOffset < fireworksDuration)
    {
      // Use particle system like renderExplosion
      palette_entry palette[] = {
        {Config.bg.r, Config.bg.g, Config.bg.b},
        {255, 200, 0},  // Yellow/orange for fireworks
        {Config.s.r, Config.s.g, Config.s.b}
      };
      
      // Define rocket timing and positions
      // Rocket 1: x=3, launches at frame 10, explodes at y=4
      // Rocket 2: x=5, launches at frame 45, explodes at y=5
      // Rocket 3: x=7, launches at frame 80, explodes at y=4
      
      int rocket1Start = 10;
      int rocket1Explode = 25;
      int rocket2Start = 45;
      int rocket2Explode = 60;
      int rocket3Start = 80;
      int rocket3Explode = 95;
      
      // Rocket 1
      if (this->lastOffset >= rocket1Start && this->lastOffset < rocket1Explode)
      {
        // Rising phase
        int phase = this->lastOffset - rocket1Start;
        int y = 9 - (phase * 5 / (rocket1Explode - rocket1Start));
        if (y >= 4 && y <= 9)
        {
          int mappedIndex = LEDFunctionsClass::mapping[y * 11 + 3] * 3;
          this->currentValues[mappedIndex] = 255;
          this->currentValues[mappedIndex + 1] = 200;
          this->currentValues[mappedIndex + 2] = 0;
        }
      }
			else if (this->lastOffset == rocket1Explode)
			{
				// Create explosion at (3, 4) with immediate particles
				for(Particle *p : this->particles) delete p;
				this->particles.clear();

				const int particleCount = 16;
				const float particleSpeed = 0.90f;
				float angle_increment = 2.0f * 3.141592654f / (float)(particleCount);
				float angle = 0;
				for(int i=0; i<particleCount; i++)
				{
					float vx = particleSpeed * sin(angle);
					float vy = particleSpeed * cos(angle);
					Particle *p = new Particle(3, 4, vx, vy, 0);
					this->particles.push_back(p);
					angle += angle_increment;
				}
			}
      else if (this->lastOffset > rocket1Explode && this->lastOffset < rocket2Start)
      {
        // Render explosion particles
        std::vector<Particle*> particlesToKeep;
        for (Particle *p : this->particles)
        {
          p->render(this->currentValues, palette);
          if (p->alive) particlesToKeep.push_back(p); else delete p;
        }
        this->particles.swap(particlesToKeep);
      }
      
      // Rocket 2
      if (this->lastOffset >= rocket2Start && this->lastOffset < rocket2Explode)
      {
        // Rising phase
        int phase = this->lastOffset - rocket2Start;
        int y = 9 - (phase * 4 / (rocket2Explode - rocket2Start));
        if (y >= 5 && y <= 9)
        {
          int mappedIndex = LEDFunctionsClass::mapping[y * 11 + 5] * 3;
          this->currentValues[mappedIndex] = 0;
          this->currentValues[mappedIndex + 1] = 200;
          this->currentValues[mappedIndex + 2] = 255;
        }
      }
			else if (this->lastOffset == rocket2Explode)
			{
				// Create explosion at (5, 5) with immediate particles
				for(Particle *p : this->particles) delete p;
				this->particles.clear();

				const int particleCount = 16;
				const float particleSpeed = 0.90f;
				float angle_increment = 2.0f * 3.141592654f / (float)(particleCount);
				float angle = 0;
				for(int i=0; i<particleCount; i++)
				{
					float vx = particleSpeed * sin(angle);
					float vy = particleSpeed * cos(angle);
					Particle *p = new Particle(5, 5, vx, vy, 0);
					this->particles.push_back(p);
					angle += angle_increment;
				}
			}
      else if (this->lastOffset > rocket2Explode && this->lastOffset < rocket3Start)
      {
        // Render explosion particles
        std::vector<Particle*> particlesToKeep;
        for (Particle *p : this->particles)
        {
          p->render(this->currentValues, palette);
          if (p->alive) particlesToKeep.push_back(p); else delete p;
        }
        this->particles.swap(particlesToKeep);
      }
      
      // Rocket 3
      if (this->lastOffset >= rocket3Start && this->lastOffset < rocket3Explode)
      {
        // Rising phase
        int phase = this->lastOffset - rocket3Start;
        int y = 9 - (phase * 5 / (rocket3Explode - rocket3Start));
        if (y >= 4 && y <= 9)
        {
          int mappedIndex = LEDFunctionsClass::mapping[y * 11 + 7] * 3;
          this->currentValues[mappedIndex] = 255;
          this->currentValues[mappedIndex + 1] = 0;
          this->currentValues[mappedIndex + 2] = 200;
        }
      }
			else if (this->lastOffset == rocket3Explode)
			{
				// Create explosion at (7, 4) with immediate particles
				for(Particle *p : this->particles) delete p;
				this->particles.clear();

				const int particleCount = 16;
				const float particleSpeed = 0.90f;
				float angle_increment = 2.0f * 3.141592654f / (float)(particleCount);
				float angle = 0;
				for(int i=0; i<particleCount; i++)
				{
					float vx = particleSpeed * sin(angle);
					float vy = particleSpeed * cos(angle);
					Particle *p = new Particle(7, 4, vx, vy, 0);
					this->particles.push_back(p);
					angle += angle_increment;
				}
			}
      else if (this->lastOffset > rocket3Explode && this->lastOffset < fireworksDuration)
      {
        // Render explosion particles
        std::vector<Particle*> particlesToKeep;
        for (Particle *p : this->particles)
        {
          p->render(this->currentValues, palette);
          if (p->alive) particlesToKeep.push_back(p); else delete p;
        }
        this->particles.swap(particlesToKeep);
      }
      
      // Cleanup particles at end of fireworks
      if (this->lastOffset == fireworksDuration - 1)
      {
        for(Particle *p : this->particles) delete p;
        this->particles.clear();
      }
    }
    // Text phase: "Happy New Year" with mixed case
    else
    {
      int textOffset = this->lastOffset - fireworksDuration;
      
      // Check if we're in the pause period
      if (textOffset > totalTextWidth + 11)
      {
        // Pause period - keep screen dark
        return;
      }
      
      // Happy New Year: H(cap) a(low) p(low) p(low) y(low) space N(cap) e(low) w(low) space Y(cap) e(low) a(low) r(low)
      const uint8_t *letters[] = {
        LETTER_H_CAP, LETTER_a_LOW, LETTER_p_LOW, LETTER_p_LOW, LETTER_y_LOW, LETTER_SPACE,
        LETTER_N_CAP, LETTER_e_LOW, LETTER_w_LOW, LETTER_SPACE,
        LETTER_Y_CAP, LETTER_e_LOW, LETTER_a_LOW, LETTER_r_LOW
      };
      int numLetters = 14;
      
      // For each Y row (0-9)
      for (int y = 0; y < 10; y++)
      {
        // For each X position on screen (0-10)
        for (int x = 0; x < 11; x++)
        {
          // Calculate position in scrolling text
          int scrollPos = (x + textOffset - 11);
          
          // Skip if out of bounds
          if (scrollPos < 0 || scrollPos >= totalTextWidth) continue;
          
          // Determine letter index and position
          int accum = 0;
          int letterIndex = -1;
          int posInLetter = -1;
          for (int i = 0; i < numLetters; i++)
          {
            int w = letterWidths[i];
            if (scrollPos >= accum && scrollPos < accum + w)
            {
              letterIndex = i;
              posInLetter = scrollPos - accum;
              break;
            }
            accum += w;
          }
          if (letterIndex < 0) continue;
          
          int glyphWidth = 5;
          if (posInLetter >= glyphWidth) continue; // skip spacer column
          
          uint8_t pattern = letters[letterIndex][y];
          int bitPos = 4 - posInLetter;
          bool pixelOn = (pattern & (1 << bitPos)) != 0;
          
          if (pixelOn)
          {
            uint8_t r, g, b;
            int colorPhase = this->happyNewYearColors[letterIndex];
            if (colorPhase == 0)
            {
              r = 255; g = 0; b = 0;      // Red
            }
            else if (colorPhase == 1)
            {
              r = 0; g = 255; b = 0;      // Green
            }
            else
            {
              r = 255; g = 220; b = 0;    // Yellow
            }
            
            int mappedIndex = LEDFunctionsClass::mapping[y * 11 + x] * 3;
            this->currentValues[mappedIndex] = r;
            this->currentValues[mappedIndex + 1] = g;
            this->currentValues[mappedIndex + 2] = b;
          }
        }
      }
    }
  }
}


//---------------------------------------------------------------------------------------
// renderWakeup
//
// Renders the wakeuplight effect
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderWakeup()
{
// #define USE_SUN
  
  palette_entry palette[3];
#ifdef USE_SUN
  uint8_t buf[] = {
    0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0,
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
    0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
    0, 0, 2, 2, 2, 2, 2, 2, 2, 0, 0,
    0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0,
    0, 0, 0, 0
  };
#else
  uint8_t buf[NUM_PIXELS];
  this->fillBackground(NTP.s, NTP.ms, buf);
  this->fillTime(NTP.h, NTP.m, buf);
#endif  
  
  palette_entry SunColor0 = {0, 0, 0}; // At start
  palette_entry SunColor1 = {15, 0, 35}; // at 33%: LEt it be a bit blue/purple
  palette_entry SunColor2 = {150, 10, 10}; // at 66%: LEt it be red
  palette_entry SunColor3 = {255, 255, 100}; // at 100%: LEt it be yellow, close to white

#ifdef USE_SUN
  palette_entry AirColor0 = {2, 2, 2}; // At start
  palette_entry AirColor1 = {0, 0, 10}; // at 33%: LEt it be a bit blue/purple
  palette_entry AirColor2 = {60, 90, 125}; // at 66%: LEt it be red
  palette_entry AirColor3 = {120, 175, 255}; // at 100%: LEt it be yellow, close to white */ // needs more thinking
#endif
  
  if (this->AlarmProgress<=0.3333){
#ifdef USE_SUN
    palette[0] = this->blendedColor(AirColor0,AirColor1,this->AlarmProgress*3);
#endif
    palette[2] = this->blendedColor(SunColor0,SunColor1,this->AlarmProgress*3);
  } else if (this->AlarmProgress<=0.666) {
#ifdef USE_SUN
    palette[0] = this->blendedColor(AirColor1,AirColor2,(this->AlarmProgress-0.3333)*3);
#endif
    palette[2] = this->blendedColor(SunColor1,SunColor2,(this->AlarmProgress-0.3333)*3);
  } else {
#ifdef USE_SUN
    palette[0] = this->blendedColor(AirColor2,AirColor3,(this->AlarmProgress-0.6666)*3);
#endif
    palette[2] = this->blendedColor(SunColor2,SunColor3,(this->AlarmProgress-0.6666)*3);
  }

  // set the other colors
  palette[1]={ 0,0, (uint8_t)(2*(255/this->brightness)) }; // time words in nightmode color
#ifndef USE_SUN
  palette[0]= palette[2]; // don't show seconds
#endif

  this->set(buf, palette, true);
}


//---------------------------------------------------------------------------------------
// renderHeart
//
// Renders one frame of the heart animation and displays it immediately
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderHeart()
{
//   if ((unsigned long) (millis()-this->lastUpdate)>(unsigned)(100-Config.animspeed))
  if ((unsigned long) (millis()-this->lastUpdate)>RENDERHEARTINTERVAL)
  {
    this->lastUpdate=millis();
  	palette_entry palette[2];
  	uint8_t heart[] = {
  		0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0,
  		1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
  		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
  		0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  		0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
  		0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
  		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
  		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  		1, 1, 1, 1
  	};
  	palette[0] = {0, 0, 0};
  	palette[1] = {(uint8_t)this->heartBrightness, 0, 0};
  	this->set(heart, palette, true);
  
  	switch (this->heartState)
  	{
  	case 0:
  		if (this->heartBrightness >= 255) this->heartState = 1;
  		else this->heartBrightness += 32;
  		break;
  
  	case 1:
  		if (this->heartBrightness < 128) this->heartState = 2;
  		else this->heartBrightness -= 32;
  		break;
  
  	case 2:
  		if (this->heartBrightness >= 255) this->heartState = 3;
  		else this->heartBrightness += 32;
  		break;
  
  	case 3:
  	default:
  		if (this->heartBrightness <= 0) this->heartState = 0;
  		else this->heartBrightness -= 4;
  		break;
  	}
  
  	if (this->heartBrightness > 255) this->heartBrightness = 255;
  	if (this->heartBrightness < 0) this->heartBrightness = 0;  
  }
}

//---------------------------------------------------------------------------------------
// prepareExplosion
//
// Prepare particles based on current screen state
//
//
// -> source: buffer to read the currently active LEDs from
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::prepareExplosion(uint8_t *source)
{
#define PARTICLE_COUNT 16
#define PARTICLE_SPEED 0.15f

	// defensive cleanup: remove any existing particles to prevent memory leaks
	for(Particle *p : this->particles) delete p;
	this->particles.clear();

	float vx, vy, angle;
	int ofs = 0;
	Particle *p;
	int delay;

	// compute angle increment
	float angle_increment = 2.0f * 3.141592654f / (float)(PARTICLE_COUNT);

	// iterate over every position in the screen buffer
	for(int y=0; y<LEDFunctionsClass::height; y++)
	{
		for(int x=0; x<LEDFunctionsClass::width; x++)
		{
			// create entry in particles vector if current pixel is foreground
			if(source[ofs++] == 1)
			{
				// add a random delay of zero to approx. 3 seconds to each
				// explosion
				delay = random(300);

				// start with angle of zero radians, assign velocity vector
				// placed on a circle to each particle
				angle = 0;
				for(int i=0; i<PARTICLE_COUNT; i++)
				{
					// calculate particle speed vector based on angle and
					// absolute speed value
					vx = PARTICLE_SPEED * sin(angle);
					vy = PARTICLE_SPEED * cos(angle);

					// create new particle and add it to particles list
					p = new Particle(x, y, vx, vy, delay);
					this->particles.push_back(p);
					angle += angle_increment;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------------------
// renderExplosion
//
// Renders the exploding letters animation
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderExplosion()
{
  // if ((unsigned long) (millis()-this->lastUpdate)>(unsigned)(100-Config.animspeed))
  if ((unsigned long) (millis()-this->lastUpdate)>EXPLODEINTERVAL)
  {
    this->lastUpdate=millis();
  	std::vector<Particle*> particlesToKeep;
  	uint8_t buf[NUM_PIXELS];
  
  	// load palette colors from configuration
  	palette_entry palette[] = {
  		{Config.bg.r, Config.bg.g, Config.bg.b},
  		{Config.fg.r, Config.fg.g, Config.fg.b},
  		{Config.s.r,  Config.s.g,  Config.s.b}};
  
  	// check if the displayed time has changed
  	if((NTP.m/5 != this->lastM/5) || (NTP.h != this->lastH))
  	{
  		// prepare new animation with old time
  		this->renderTime(buf);
  		this->prepareExplosion(buf);
  	}
  
  	this->lastM = NTP.m;
  	this->lastH = NTP.h;
  
  	// create empty buffer filled with seconds color
  	this->fillBackground(NTP.s, NTP.ms, buf);
  
  	// minutes 1...4 for the corners
  	for(int i=0; i<=((NTP.m%5)-1); i++) buf[10 * 11 + i] = 1;
  
  	// Do we have something to explode?
  	if(this->particles.size() > 0)
  	{
  		// transfer background created by fillBackground to target buffer
  		this->set(buf, palette, true);
  
  		// iterate over all particles
  		for(Particle *p : this->particles)
  		{
  			// move and render current particle
  			p->render(this->currentValues, palette);
  
  			// if particle is still active, keep it; kill it otherwise
  			if(p->alive) particlesToKeep.push_back(p); else delete p;
  		}
  
  		// only keep active particles, discard the rest
  		// -> use particlesToKeep as new list
  		this->particles.swap(particlesToKeep);
  	}
  	else
  	{
  		// present the current time in boring mode with simple fading
  		this->renderTime(buf);
  		this->set(buf, palette, false);
  		this->fade();
  	}
	}
}

//---------------------------------------------------------------------------------------
// prepareFlyingLetters
//
// Sets the current buffer as target state for flying letters, initializes current
// positions of the letters below the visible area with some random jitter
//
// -> source: buffer to read the currently active LEDs from
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::prepareFlyingLetters(uint8_t *source)
{
	// transfer the previous flying letters in the leaving letters vector to prepare for
	// outgoing animation
	this->leavingLetters.clear();
	for(xy_t &p : this->arrivingLetters)
	{
		// delay every letter depending on its position
		// and set new target coordinate
		if(this->mode == DisplayMode::flyingLettersVerticalUp)
		{
			p.delay = p.y * 2 + p.x + 1 + random(5);
			p.yTarget = -1;
		}
		else
		{
			p.delay = (LEDFunctionsClass::height - p.y - 1) * 2 + p.x + 1 + random(5);
			p.yTarget = LEDFunctionsClass::height;
		}
		this->leavingLetters.push_back(p);
	}

	// initialize arriving letters from scratch
	this->arrivingLetters.clear();
	int ofs = 0;

	// iterate over every position in the screen buffer
	for(int y=0; y<LEDFunctionsClass::height; y++)
	{
		for(int x=0; x<LEDFunctionsClass::width; x++)
		{
			// create entry in arrivingLetters vector if current pixel is foreground
			if(source[ofs++] == 1)
			{
				if(this->mode == DisplayMode::flyingLettersVerticalUp)
				{
					xy_t p = {x, y, x, LEDFunctionsClass::height,
							y * 2 + x + 1 + random(5), 200, 0};
					this->arrivingLetters.push_back(p);
				}
				else
				{
					xy_t p = {x, y, x, -1,
							(LEDFunctionsClass::height - y - 1) * 2 + x + 1 + random(5),
							200, 0};
					this->arrivingLetters.push_back(p);
				}
			}
		}
	}

	// DEBUG
	Serial.printf("h=%i, m=%i, s=%i, lastH=%i, lastM=%i\r\n", NTP.h, NTP.m, NTP.s, this->lastH, this->lastM);
	Serial.println("leavingLetters:");
	for(xy_t &p : this->leavingLetters)
	{
		Serial.printf("  counter=%i, delay=%i, speed=%i, x=%i, y=%i, xTarget=%i, yTarget=%i\r\n",
				p.counter, p.delay, p.speed, p.x, p.y, p.xTarget, p.yTarget);
	}
	Serial.println("arrivingLetters:");
	for(xy_t &p : this->arrivingLetters)
	{
		Serial.printf("  counter=%i, delay=%i, speed=%i, x=%i, y=%i, xTarget=%i, yTarget=%i\r\n",
				p.counter, p.delay, p.speed, p.x, p.y, p.xTarget, p.yTarget);
	}
}

//---------------------------------------------------------------------------------------
// renderFlyingLetters
//
// Takes the current arrivingLetters and leavingLetters to render the flying letters
// animation
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderFlyingLetters()
{
  // if ((unsigned long) (millis()-this->lastUpdate)>(unsigned)(100-Config.animspeed))
  if ((unsigned long) (millis()-this->lastUpdate)>FLYINGLETTERSINTERVAL)
  {
    this->lastUpdate=millis();
  	uint8_t buf[NUM_PIXELS];
  
  	// load palette colors from configuration
  	palette_entry palette[] = {
  		{Config.bg.r, Config.bg.g, Config.bg.b},
  		{Config.fg.r, Config.fg.g, Config.fg.b},
  		{Config.s.r,  Config.s.g,  Config.s.b}};
  
  	// check if the displayed time has changed
  	if((NTP.m/5 != this->lastM/5) || (NTP.h != this->lastH))
  	{
  		// prepare new animation
  		this->renderTime(buf);
  		this->prepareFlyingLetters(buf);
  	}
  
  	this->lastM = NTP.m;
  	this->lastH = NTP.h;
  
  
  	// create empty buffer filled with seconds color
  	this->fillBackground(NTP.s, NTP.ms, buf);
  
  	// minutes 1...4 for the corners
  	for(int i=0; i<=((NTP.m%5)-1); i++) buf[10 * 11 + i] = 1;
  
  	// leaving letters animation has priority
  	if(this->leavingLetters.size() > 0)
  	{
  		// count actually moved letters to detect end of animation
  		int movedLetters = 0;
  
  		// iterate over all leavingLetters
  		for(xy_t &p : this->leavingLetters)
  		{
  			// draw letter only if inside visible area
  			if(p.x>=0 && p.y>=0 && p.x<LEDFunctionsClass::width
  					&& p.y<LEDFunctionsClass::height)
  				buf[p.x + p.y * LEDFunctionsClass::width] = 1;
  
  			// continue with next letter if the current letter already
  			// reached its target position
  			if(p.y == p.yTarget && p.x == p.xTarget) continue;
  			p.counter += p.speed;
  			movedLetters++;
  			if(p.counter >= 1000)
  			{
  				p.counter -= 1000;
  				if(p.delay>0)
  				{
  					// do not move if animation of current letter is delayed
  					p.delay--;
  				}
  				else
  				{
  					if(p.y > p.yTarget) p.y--; else p.y++;
  				}
  			}
  		}
  		if(movedLetters == 0) this->leavingLetters.clear();
  	}
  	else
  	{
  		// iterate over all arrivingLetters
  		for(xy_t &p : this->arrivingLetters)
  		{
  			// draw letter only if inside visible area
  			if(p.x>=0 && p.y>=0 && p.x<LEDFunctionsClass::width
  					&& p.y<LEDFunctionsClass::height)
  				buf[p.x + p.y * LEDFunctionsClass::width] = 1;
  
  			// continue with next letter if the current letter already
  			// reached its target position
  			if(p.y == p.yTarget && p.x == p.xTarget) continue;
  			p.counter += p.speed;
  			if(p.counter >= 1000)
  			{
  				p.counter -= 1000;
  				if(p.delay>0)
  				{
  					// do not move if animation of current letter is delayed
  					p.delay--;
  				}
  				else
  				{
  					if(p.y > p.yTarget) p.y--; else p.y++;
  				}
  			}
  		}
  	}
  
  	// present the current content immediately without fading
  	this->set(buf, palette, true);
  }
}

//---------------------------------------------------------------------------------------
// renderRed
//
// Renders a complete red frame for testing purposes.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderRed()
{
	palette_entry palette[2];
	uint8_t heart[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1
	};
	palette[0] = {0, 0, 0};
	palette[1] = {32, 0, 0};
	this->set(heart, palette, true);
}

//---------------------------------------------------------------------------------------
// renderGreen
//
// Renders a complete green frame for testing purposes.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderGreen()
{
	palette_entry palette[2];
	uint8_t heart[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1
	};
	palette[0] = {0, 0, 0};
	palette[1] = {0, 32, 0};
	this->set(heart, palette, true);
}

//---------------------------------------------------------------------------------------
// renderBlue
//
// Renders a complete blue frame for testing purposes.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderBlue()
{
	palette_entry palette[2];
	uint8_t heart[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1
	};
	palette[0] = {0, 0, 0};
	palette[1] = {0, 0, 32};
	this->set(heart, palette, true);
}

//---------------------------------------------------------------------------------------
// renderUpdate
//
// Renders the OTA update screen with progress information depending on
// Config.updateProgress
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderUpdate()
{
	uint8_t update[] = {
		0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
		1, 1, 1, 1
	};
	palette_entry p[] = {{0, 0, 0}, {255, 0, 0}, {42, 21, 0}, {255, 85, 0}};
	for(int i=0; i<110; i++)
	{
		if(i<=Config.updateProgress)
		{
			if(update[i] == 0) update[i] = 2;
			else update[i] = 3;
		}
	}
	this->set(update, p, true);
}

//---------------------------------------------------------------------------------------
// renderUpdateComplete
//
// Renders the OTA update complete screen
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderUpdateComplete()
{
	const uint8_t update_ok[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1
	};
	palette_entry p[] = {{0, 21, 0}, {0, 255, 0}};
	this->set(update_ok, p, true);
}

//---------------------------------------------------------------------------------------
// renderUpdateError
//
// Renders the OTA update error screen
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderUpdateError()
{
	const uint8_t update_err[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1
	};
	palette_entry p[] = {{0, 0, 0}, {255, 0, 0}};
	this->set(update_err, p, true);
}

//---------------------------------------------------------------------------------------
// renderWifiManager
//
// Renders the WifiManager screen
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void LEDFunctionsClass::renderWifiManager()
{
	const uint8_t wifimanager[] = {
		0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0,
		0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
		0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
		1, 1, 1, 1
	};
	palette_entry p[] = {{0, 0, 0}, {255, 255, 0}};
	this->set(wifimanager, p, true);
}

#endif
