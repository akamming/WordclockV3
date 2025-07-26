// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This class represents a start object for the stars screen saver.
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

#include "starobject.h"
#include "ledfunctions.h"

//---------------------------------------------------------------------------------------
// StarObject
//
// Constructor
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
StarObject::StarObject()
{
}

//---------------------------------------------------------------------------------------
// randomize
//
// Assigns new random coordinates and speed, retries until new coordinates have a
// distance of minimum 2 LEDs.
//
// -> allStars: Reference to outer container for distance calculation to other stars
// <- --
//---------------------------------------------------------------------------------------
void StarObject::randomize(std::vector<StarObject> &allStars)
{
	// set coordinates of self to default value
	this->x = -1;
	this->y = -1;

	int retryCount = 0;
	bool distanceOK;
	int newX = 0, newY = 0, dx, dy;

	do
	{
		// assume distance is OK
		distanceOK = true;

		// create new random pair
		newX = random(LEDFunctionsClass::width);
		newY = random(LEDFunctionsClass::height);
		retryCount++;

		// iterate over all other stars and check distance to newly generated coordinate
		for (StarObject s : allStars)
		{
			// skip if default value
			if (s.x == -1) continue;

			// calculate distance
			dx = newX - s.x;
			dy = newY - s.y;
			if (dx * dx + dy * dy < StarObject::minimumDistanceSquared)
			{
				// retry if distance to any star is below limit
				distanceOK = false;
				break;
			}
		}
	} while (!distanceOK && retryCount < 100);

	this->x = newX;
	this->y = newY;
	this->speed = 15 + random(15);
	this->state = 0;
	this->brightness = 0;
}

//---------------------------------------------------------------------------------------
// update
//
// Updates the state of the star object. Increases brightness up to maximum, then
// decreases to zero, then randomizes to new coordinates and speed and starts again.
//
// -> allStars: Reference to vector containing all stars, necessary for distance
//              calculation when creating new random position
// <- --
//---------------------------------------------------------------------------------------
void StarObject::update(std::vector<StarObject> &allStars)
{
	// increase or decrease brightness depending on current state
	if (this->state == 0)
	{
		this->brightness += this->speed;
		if (this->brightness >= 255)
		{
			// switch to decreasing mode
			this->brightness = 255;
			this->state = 1;
		}
	}
	else
	{
		this->brightness -= this->speed;
		if (this->brightness <= 0)
		{
			// switch to increasing mode and get new random coordinates
			this->brightness = 0;
			this->state = 0;
			this->randomize(allStars);
		}
	}
}

//---------------------------------------------------------------------------------------
// render
//
// Updates own status (see StarObject::update()) and renders self to buffer.
//
// -> buf: RGB buffer for LED colors
//    allStars: Reference to vector containing all stars, necessary for distance
//              calculation when creating new random position
// <- --
//---------------------------------------------------------------------------------------
void StarObject::render(uint8_t* buf, std::vector<StarObject> &allStars)
{
	this->update(allStars);

	// write brightness to target buffer
	int offset = LEDFunctionsClass::getOffset(this->x, this->y);
	buf[offset + 0] = this->brightness;
	buf[offset + 1] = this->brightness;
	buf[offset + 2] = this->brightness;
}
