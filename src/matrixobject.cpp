// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This class represents one "Matrix Pixel" for the matrix screen saver.
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

#include "matrixobject.h"
#include "ledfunctions.h"

//---------------------------------------------------------------------------------------
// initializes the static gradient palette
//---------------------------------------------------------------------------------------
const std::vector<palette_entry> MatrixObject::MatrixGradient = {
	{ 255, 255, 255 },
	{ 128, 255, 128 },
	{ 64, 255, 64 },
	{ 0, 255, 0 },
	{ 0, 128, 0 },
	{ 0, 64, 0 },
	{ 0, 32, 0 },
	{ 0, 16, 0 },
	{ 0, 8, 0 },
	{ 0, 2, 0 },
	{ 0, 1, 0 },
	{ 0, 0, 0 } };

//---------------------------------------------------------------------------------------
// MatrixObject
//
// Constructor. Initializes coordinates and speed with random values.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
MatrixObject::MatrixObject()
{
	this->randomize();
}

//---------------------------------------------------------------------------------------
// move
//
// Moves the matrix object using prescaler. Assigns new random coordinates if object
// leaves screen.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MatrixObject::move()
{
	this->prescaler += this->speed;
	if (this->prescaler > 30000)
	{
		this->prescaler -= 30000;
		this->y++;
		int limit = LEDFunctionsClass::height + MatrixObject::MatrixGradient.size();
		if(this->y > limit) this->randomize();
	}
}

//---------------------------------------------------------------------------------------
// randomize
//
// Assigns new random coordinates, initializes speed and prescaler counter.
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void MatrixObject::randomize()
{
	this->x = random(LEDFunctionsClass::width); //   0 ... width
	this->y = random(25) - 25;                  // -25 ... -1
	this->speed = MatrixObject::MinMatrixSpeed
			+ random(MatrixObject::MaxMatrixSpeed - MatrixObject::MinMatrixSpeed);
	this->prescaler = 0;
}

//---------------------------------------------------------------------------------------
// render
//
// Moves and renders the matrix object to an RGB buffer. Uses the MatrixGradient palette
// to draw the matrix stream object from bottom to top.
//
// -> buf: Pointer to render target (linear RGB buffer)
// <- --
//---------------------------------------------------------------------------------------
void MatrixObject::render(uint8_t *buf)
{
	this->move();

	// check boundaries
	if(this->x<0 || this->x >= LEDFunctionsClass::width) return;

	int currentY = this->y;
	int ofs;

	for (palette_entry p : MatrixObject::MatrixGradient)
	{
		// only render if inside screen rectangle
		if(currentY>=0 && currentY<LEDFunctionsClass::height)
		{
			ofs = LEDFunctionsClass::getOffset(this->x, currentY);
			buf[ofs + 0] = p.r;
			buf[ofs + 1] = p.g;
			buf[ofs + 2] = p.b;
		}
		currentY--;
	}
}
