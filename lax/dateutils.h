//
//	
//    The Laxkit, a windowing toolkit
//    Please consult https://github.com/Laidout/laxkit about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 3 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; If not, see <http://www.gnu.org/licenses/>.
//
//    Copyright (C) 2011 by Tom Lechner
//
#ifndef _LAX_DATEUTILS_H
#define _LAX_DATEUTILS_H

namespace Laxkit {

//---------------------------- date functions -----------------------------------------
int is_leap_year(int year);
int days_in_month(int month, int year);
int days_in_year(int year);
int day_of_week(int year, int month, int day, bool start_sunday);
const char *dayofweek(int day, int lvl, int sunday);
const char *monthname(int month,int lvl);


//---------------------------- LaxTime -----------------------------------------

class LaxTime
{
  protected:

  public:
	int hour = 0; // 0..23
	int minute = 0; // 0..59
	double second = 0.0; // 0 to 60

	LaxTime() { hour = 0; minute = 0; second = 0.0; }
	LaxTime(int h, int m, double s);
	int Hour() const { return hour; }
	int Minute() const { return minute; }
	double Second() const { return second; }
	double TotalSeconds() const { return (hour*60 + minute)*60 + second; }
	double TotalMinutes() const { return hour*60 + minute + second/60; }
	double TotalHours()   const { return hour + minute/60.0 + second/3600; }
	void SetToNow();

	static LaxTime Now();
	static LaxTime FromSeconds(double s);
};

double operator-(const LaxTime &t1, const LaxTime &t2); // seconds between instances
bool operator< (const LaxTime &t1, const LaxTime &t2);
bool operator> (const LaxTime &t1, const LaxTime &t2);
bool operator<=(const LaxTime &t1, const LaxTime &t2);
bool operator>=(const LaxTime &t1, const LaxTime &t2);
bool operator==(const LaxTime &t1, const LaxTime &t2);


//---------------------------- LaxDate -----------------------------------------

class LaxDate
{
  protected:
	int year;
	int month; // 1..12
	int day;   // 1..31
	LaxTime _time; //todo: finish adding this

  public:
	LaxDate();
	LaxDate(int year, int month, int day);
	LaxDate(const LaxDate &d);
	LaxDate &operator=(LaxDate &d);
	int Set(int year, int month, int day);
	void SetToToday();

	void AddDays(int days);
	void AddWeeks(int weeks);
	int  AddMonths(int months);
	void AddYears(int years);
	void Add(int years, int months, int days);

	int Format(const char *fmt, char *buffer, int &max);

	int Year()  const { return year;  }
	int Month() const { return month; }
	int Day()   const { return day;   }
	int DayOfWeek() const;
	int DayOfYear() const;
	bool IsLeapYear() const { return is_leap_year(year); }
};

int  operator- (const LaxDate &d1,const LaxDate &d2); //number of days between dates
bool operator< (const LaxDate &d1,const LaxDate &d2);
bool operator> (const LaxDate &d1,const LaxDate &d2);
bool operator<=(const LaxDate &d1,const LaxDate &d2);
bool operator>=(const LaxDate &d1,const LaxDate &d2);
bool operator==(const LaxDate &d1,const LaxDate &d2);


} //namespace Laxkit

#endif


