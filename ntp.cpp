// ESP8266 Wordclock
// Copyright (C) 2016 Thoralt Franz, https://github.com/thoralt
//
//  This module contains a simple NTP client. NTP packets are sent using UDP to a
//  configurable NTP server. The server reply is processed and a callback is
//  executed to notifiy the calling application of the current time. The internal
//  state machine is called from an internal timer to take care of timeouts, the
//  NTP request is retried automatically if no reply is being received. The request
//  is being repeated every 59 minutes, so the calling module is updated regularly.
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
//
// This code is based on (heavily modified):
// https://github.com/sandeepmistry/esp8266-Arduino/blob/master/esp8266com/esp8266/libraries/ESP8266WiFi/examples/NTPClient
#include <Arduino.h>
#include <limits.h>
#include "ntp.h"
#include "ledfunctions.h"


//---------------------------------------------------------------------------------------
// CONSTANTS
//---------------------------------------------------------------------------------------
#define NTP_PACKET_SIZE 48
#define LOCAL_PORT 2390
#define NTP_TIMEOUT 5000
#define TIMER_RESOLUTION 100
#define NTP_RELOAD_INTERVAL (59*60*1000)

//---------------------------------------------------------------------------------------
// global instance
//---------------------------------------------------------------------------------------
NtpClass NTP = NtpClass();

//---------------------------------------------------------------------------------------
// tickerFunctionWrapper
//
// Static wrapper which is used as callback function for Ticker and takes supplied
// argument to call the handler tickerFunction() of instance method
//
// -> obj: Instance of class to call the method tickerFunctio() on
// <- --
//---------------------------------------------------------------------------------------
void NtpClass::tickerFunctionWrapper(NtpClass *obj)
{
	obj->tickerFunction();
}

//---------------------------------------------------------------------------------------
// NtpClass
//
// Constructor, sets up a timer to trigger the internal state machine
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
NtpClass::NtpClass()
{
	this->ticker.attach_ms(TIMER_RESOLUTION, NtpClass::tickerFunctionWrapper,
			 this);
}

//---------------------------------------------------------------------------------------
// setServer
//
// Sets the current time server and schedules a new NTP request
//
// -> address: IP address of the new time server
// <- --
//---------------------------------------------------------------------------------------
void NtpClass::setServer(IPAddress address)
{
	this->timeServer = address;
	this->state = NtpState::waitingForReload;
	this->timer = NTP_RELOAD_INTERVAL - 2000;
}

//---------------------------------------------------------------------------------------
// getServer
//
// Gets the current time server
//
// -> --
// <- IPAddress current time server
//---------------------------------------------------------------------------------------
IPAddress NtpClass::getServer()
{
	return this->timeServer;
}

//---------------------------------------------------------------------------------------
// begin
//
// Initializes the class and starts the first NTP request, automatically fires callback
// upon success, repeats every 59 minutes
//
// -> ip: Address of an NTP server
//	  callback: Function to receive the current time (hours, minutes, seconds, ms)
//	  timezone: Hours difference from UTC (will be added to the received time, can be
//			    negative)
//    DST: if true, european daylight savings time is enabled and will be automatically
//         adjusted depending on current date
// <- --
//---------------------------------------------------------------------------------------
void NtpClass::begin(IPAddress ip, TNtpCallback callback, int timezone, bool DST)
{
	this->_callback = callback;
	this->timeServer = ip;
	this->tz = timezone * 3600;
	this->useDST = DST;

	this->udp.begin(LOCAL_PORT);

	// don't wait 2 seconds before starting first request
	Serial.println("NtpClass::begin()"); // Waiting 2 seconds");
	this->state = NtpState::waitingForReload;
	this->timer = NTP_RELOAD_INTERVAL; // - 2000;
}

//---------------------------------------------------------------------------------------
// tickerFunction
//
// Function to be called by the timer, drives the internal state machine
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void NtpClass::tickerFunction()
{
  if (LED.inprogress) Serial.println("NTP Ticker called during pixel-Show");
  
	// increment timer variable
	this->timer += TIMER_RESOLUTION;

	switch (this->state)
	{
	case NtpState::startRequest:
		this->udp.flush(); // clear previously received data
		this->sendPacket(); // request new time data
		this->timer = 0;
		this->state = NtpState::waitingForReply;
		this->syncInProgress = true;
		break;

	case NtpState::waitingForReply:
		if (this->timer >= NTP_TIMEOUT)
		{
			Serial.println("NtpClass: NTP request timeout");
			this->state = NtpState::startRequest;
		}
		else if (udp.parsePacket() > 0)
		{
			Serial.println("NtpClass: Received NTP packet");
			this->parse();
			if (this->_callback)
				this->_callback(this->h, this->m, this->s, this->ms);
			this->timer = 0;
			this->state = NtpState::waitingForReload;
			this->syncInProgress = false;
		}
		break;

	case NtpState::waitingForReload:
		if (this->timer >= NTP_RELOAD_INTERVAL)
		{
			Serial.println("NtpClass: NTP reload timer expired.");
			this->state = NtpState::startRequest;
		}
		break;

	case NtpState::idle:
	default:
		break;
	}
}

//---------------------------------------------------------------------------------------
// dayOfWeek
//
// Calculates the weekday for a given date.
//
// -> y, m, d: year, month, day to calculate the weekday for
// <- weekday (0=Sunday, 1=Monday, ...)
//---------------------------------------------------------------------------------------
int NtpClass::dayOfWeek(int y, int m, int d)
{
	static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	y -= m < 3;
	return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}

//---------------------------------------------------------------------------------------
// lastSunday
//
// Calculates the last Sunday for a given year and month.
//
// -> year, month: ...
//    lastDayInMonth: number of days in given month (out of laziness, should be done
//                    inside the method)
// <- weekday (0=Sunday, 1=Monday, ...)
//---------------------------------------------------------------------------------------
int NtpClass::lastSunday(int year, int month, int lastDayInMonth)
{
	for(int day = lastDayInMonth; day > 0; day--)
	{
		if(this->dayOfWeek(year, month, day) == 0) return day;
	}
	return 0;
}

//---------------------------------------------------------------------------------------
// isDSTactive
//
// Checks if daylight saving time is currently in effect using the internal member
// variables for year, month, day and hour.
//
// -> --
// <- true if DST is active
//---------------------------------------------------------------------------------------
bool NtpClass::isDSTactive()
{
	// active in April ... September
	if(this->month > 3 && this->month < 10) return true;

	int lastSundayInMarch = this->lastSunday(this->year, 3, 31);
	int lastSundayInOctober = this->lastSunday(this->year, 10, 31);

	if(this->month == 3)
	{
		// active after last Sunday in March
		if(this->day > lastSundayInMarch) return true;

		// active on last Sunday in March after 02:00
		if(this->day == lastSundayInMarch && this->h >= 2) return true;
	}

	if(this->month == 10)
	{
		// active before last Sunday in October
		if(this->day < lastSundayInOctober) return true;

		// active on last Sunday in October before 03:00
		if(this->day == lastSundayInMarch && this->h < 3) return true;
	}

	return false;
}

//---------------------------------------------------------------------------------------
// parse
//
// Reads the received UDP packet and decodes the current time, stores result in (this)
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void NtpClass::parse()
{
	byte buf[NTP_PACKET_SIZE];
	bool DST = false;

	Serial.print("NtpClass::parse() (");
	Serial.print(this->timer);

	this->udp.read(buf, NTP_PACKET_SIZE);
	this->udp.flush(); // discard additional data
	unsigned long highWord = word(buf[40], buf[41]);
	unsigned long lowWord = word(buf[42], buf[43]);
	unsigned long secsSince1970 = (highWord << 16 | lowWord) - 2208988800ULL;

	// calculate date and time from timestamp
	this->decodeTime(secsSince1970 + this->tz);
	randomSeed(secsSince1970);

	// check if we need to adjust for daylight savings time
	if(this->useDST)
	{
		// check if we are inside DST window
		DST = this->isDSTactive();
		if(DST)
		{
			// decode date/time again using DST offset
			this->decodeTime(secsSince1970 + this->tz + 3600);
		}
	}
	Serial.printf("ms), local time: %02i:%02i:%02i, date: %i-%02i-%02i, "
			"weekday=%i, DST=%i\r\n", h, m, s, year, month, day, weekday, DST);
}

//---------------------------------------------------------------------------------------
// sendNTPpacket
//
// Requests time from an NTP server
//
// -> --
// <- --
//---------------------------------------------------------------------------------------
void NtpClass::sendPacket()
{
	uint8_t buf[NTP_PACKET_SIZE];

	Serial.println("NtpClass::sendPacket()");
	memset(buf, 0, NTP_PACKET_SIZE);
	buf[0] = 0b11100011;
	buf[1] = 0;
	buf[2] = 6;
	buf[3] = 0xEC;
	buf[12] = 49;
	buf[13] = 0x4E;
	buf[14] = 49;
	buf[15] = 52;
	this->udp.beginPacket(this->timeServer, 123);
	this->udp.write(buf, NTP_PACKET_SIZE);
	this->udp.endPacket();

	this->timer = 0;
}

//---------------------------------------------------------------------------------------
// setTimeZone
//
// Sets the time zone and triggers a new NTP request which in turn will trigger the
// callback to notify the callee of the new time using the new time zone setting.
//
// -> timeZone: New time zone offset to UTC in hours
// <- --
//---------------------------------------------------------------------------------------
void NtpClass::setTimeZone(int timeZone)
{
	this->tz = timeZone * 3600;
	this->timer = NTP_RELOAD_INTERVAL - 1000;
}

//---------------------------------------------------------------------------------------
// decodeTime
//
// Decodes a Unix timestamp (seconds since 1970) to the internal date and time values.
// Results are placed in this->year, this->month, this->day, this->weekday,
// this->yearday, this->hour, this->minute and this->second.
//
// modified/borrowed from:
//   http://git.musl-libc.org/cgit/musl/tree/src/time/__secs_to_tm.c?h=v0.9.15
//
// -> t: Unix timestamp (seconds since 1970)
// <- --
//---------------------------------------------------------------------------------------
// 2000-03-01 (mod 400 year, immediately after feb29)
#define LEAPOCH (946684800LL + 86400*(31+29))
#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)
void NtpClass::decodeTime(long long t)
{
	long long days, secs;
	int remdays, remsecs, remyears;
	int qc_cycles, c_cycles, q_cycles;
	int years, months;
	int wday, yday, leap;
	static const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

	secs = t - LEAPOCH;
	days = secs / 86400;
	remsecs = secs % 86400;
	if(remsecs < 0)
	{
		remsecs += 86400;
		days--;
	}

	wday = (3+days)%7;
	if (wday < 0) wday += 7;

	qc_cycles = days / DAYS_PER_400Y;
	remdays = days % DAYS_PER_400Y;
	if(remdays < 0)
	{
		remdays += DAYS_PER_400Y;
		qc_cycles--;
	}

	c_cycles = remdays / DAYS_PER_100Y;
	if(c_cycles == 4) c_cycles--;
	remdays -= c_cycles * DAYS_PER_100Y;

	q_cycles = remdays / DAYS_PER_4Y;
	if(q_cycles == 25) q_cycles--;
	remdays -= q_cycles * DAYS_PER_4Y;

	remyears = remdays / 365;
	if(remyears == 4) remyears--;
	remdays -= remyears * 365;

	leap = !remyears && (q_cycles || !c_cycles);
	yday = remdays + 31 + 28 + leap;
	if(yday >= 365+leap) yday -= 365+leap;

	years = remyears + 4*q_cycles + 100*c_cycles + 400*qc_cycles;

	for(months=0; days_in_month[months] <= remdays; months++)
		remdays -= days_in_month[months];

	this->year = years + 2000;
	this->month = months + 3;
	if(this->month >= 12)
	{
		this->month -=12;
		this->year++;
	}
	this->day = remdays + 1;
	this->weekday = wday;
	this->yearday = yday;
	this->h = remsecs / 3600;
	this->m = remsecs / 60 % 60;
	this->s = remsecs % 60;
}
