// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  See starobject.cpp for description.
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

#ifndef STAROBJECT_H_
#define STAROBJECT_H_

#include <vector>
#include <stdint.h>

class StarObject
{
public:
	StarObject();
	void render(uint8_t *buf, std::vector<StarObject> &allStars);
	void randomize(std::vector<StarObject> &allStars);

private:
	const static int minimumDistanceSquared = 5;
	int x = -1;
	int y = -1;
	int speed = 0;
	int count = 0;
	int brightness = 0;
	int state = 0;
	void update(std::vector<StarObject> &allStars);
};

#endif /* STAROBJECT_H_ */
