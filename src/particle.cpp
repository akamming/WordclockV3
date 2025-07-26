// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This module encapsulates a single particle used for the exploding letters effect.
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
#include <math.h>
#include "particle.h"
#include "ledfunctions.h"

//---------------------------------------------------------------------------------------
// brightness gradient for moving particle
//---------------------------------------------------------------------------------------
const float Particle::ParticleGradient[MAX_PARTICLE_DISTANCE] = {
	1, 0.75, 0.5, 0.25, 0.125, 0.06, 0.03, 0.01 };

//---------------------------------------------------------------------------------------
// Particle
//
// Constructor. Initializes coordinates and speed with given values.
//
// -> x: x of start coordinate
//    y: y of start coordinate
//    vx: x velocity
//    vy: y velocity
//    delay: time to wait until the particle starts moving
// <- --
//---------------------------------------------------------------------------------------
Particle::Particle(float x, float y, float vx, float vy, int delay)
{
	this->x = x;
	this->y = y;
	this->vx = vx;
	this->vy = vy;
	this->x0 = x;
	this->y0 = y;
	this->delay = delay;
	this->alive = true;
}

//---------------------------------------------------------------------------------------
// ~Particle
//
// Destructor
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
Particle::~Particle()
{
}

//---------------------------------------------------------------------------------------
// distance
//
// Calculates the distance to the starting point of the particle
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
float Particle::distance()
{
	return this->distanceTo(this->x0, this->y0);
}

//---------------------------------------------------------------------------------------
// distanceTo
//
// Calculates the distance to a given point
//
// -> x, y: Point for distance test
// <- --
//---------------------------------------------------------------------------------------
float Particle::distanceTo(float x, float y)
{
	float dx = this->x - x;
	float dy = this->y - y;
	return sqrt(dx*dx + dy*dy);
}

//---------------------------------------------------------------------------------------
// move
//
// Moves the particle according to the current speed.
//
// -> --
// <- distance to starting point
//---------------------------------------------------------------------------------------
float Particle::move()
{
	// do not move until given delay has expired
	if(this->delay)
	{
		this->delay--;
		return 0;
	}

	this->x += this->vx;
	this->y += this->vy;

	// mark movement as finished if distance has reached maximum
	float d = this->distance();
	if(d > MAX_PARTICLE_DISTANCE) this->alive = false;

	return d;
}

//---------------------------------------------------------------------------------------
// render
//
// Moves the particle, renders it to the given buffer.
//
// -> target: RGB target buffer (i. e. LEDFunctions::currentValues)
//    palette: palette with background color, foreground color
// <- --
//---------------------------------------------------------------------------------------
void Particle::render(uint8_t *target, palette_entry palette[])
{
	// move particle and save traveled distance
	int d = (int)this->move();

	// check boundaries
	if(this->x<0 || this->x >= LEDFunctionsClass::width) return;
	if(this->y<0 || this->y >= LEDFunctionsClass::height) return;

	// limit distance
	if(d >= MAX_PARTICLE_DISTANCE) d = MAX_PARTICLE_DISTANCE - 1;

	// get palette color for foreground
	float pr = (float)palette[1].r;
	float pg = (float)palette[1].g;
	float pb = (float)palette[1].b;

	// calculate offset in buffer from given coordinates
	int ofs = LEDFunctionsClass::getOffset(this->x, this->y);

	// calculate fading color depending on distance from starting point and add it
	// to the previous value of the pixel corresponding to the particle
	float r = (float)target[ofs + 0] + pr * ParticleGradient[d];
	float g = (float)target[ofs + 1] + pg * ParticleGradient[d];
	float b = (float)target[ofs + 2] + pb * ParticleGradient[d];

	// limit brightness value of each component to foreground color values
	if(r > pr) r = pr;
	if(g > pg) g = pg;
	if(b > pb) b = pb;

	// write back pixel color
	target[ofs + 0] = r;
	target[ofs + 1] = g;
	target[ofs + 2] = b;
}
