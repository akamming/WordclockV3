// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  See particle.cpp for description.
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
#ifndef PARTICLE_H_
#define PARTICLE_H_

#include <vector>
#include <algorithm>
#include "config.h"

#define MAX_PARTICLE_DISTANCE 8

class Particle
{
private:
	static const float ParticleGradient[MAX_PARTICLE_DISTANCE];
	float x, y, vx, vy, x0, y0;
	int delay;

	float move();

public:
	bool alive;

	Particle(float x, float y, float vx, float vy, int delay);
	virtual ~Particle();

	void render(uint8_t *target, palette_entry palette[]);
	float distance();
	float distanceTo(float x, float y);
};

#endif /* PARTICLE_H_ */
