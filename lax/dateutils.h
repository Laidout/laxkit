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
const char *dayofweek(int day, int lvl, int sunday);
const char *monthname(int month,int lvl);


//---------------------------- LaxDate -----------------------------------------

class LaxDate
{
  protected:
	int year,month,day;
	int dayofweek, dayofyear;

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

	int Year()  { return year;  }
	int Month() { return month; }
	int Day()   { return day;   }
	int DayOfWeek();
	int DayOfYear();
};

int operator-(LaxDate d1,LaxDate d2); //number of days between dates
int operator<(LaxDate d1,LaxDate d2);
int operator>(LaxDate d1,LaxDate d2);
int operator<=(LaxDate d1,LaxDate d2);
int operator>=(LaxDate d1,LaxDate d2);
int operator==(LaxDate d1,LaxDate d2);

} //namespace Laxkit

#endif


