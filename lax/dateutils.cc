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


#include <lax/dateutils.h>
#include <lax/language.h>

#include <ctime>
#include <cstring>
//#include <cstdio>
//#include <cstdlib>

#include <iostream>
using namespace std;
#define DBG

namespace Laxkit {

//---------------------------- date functions -----------------------------------------

/*! A leap year is divisible by 4, but NOT divisible by 100 EXCEPT when divisible by 400.
 */
int is_leap_year(int year)
{
	if (year%4!=0) return 0;
	if (year%100==0 && year%400==0) return 1;
	if (year%100==0) return 0;
	return 1;
}

/*! If leap, then return 29 for February, else 28 for February.
 * Otherwise, April, June, September, November have 30, and the others have 31.
 */
int days_in_month(int month, int year)
{
	if (month==4 || month==6 || month==9 || month==11) return 30;
	int leap=is_leap_year(year);
	if (month==2) return leap?29:28;
	return 31;
}

//! Returns 366 for leap years, or else 365.
int days_in_year(int year)
{
	return is_leap_year(year)?366:365;
}

/*! Return in range 0..6. Formula based on Julian days, from stackoverflow question 6054016. */
int day_of_week(int year, int month, int day, bool start_sunday)
{
	int dow = (
          day
        + ((153 * (month + 12 * ((14 - month) / 12) - 3) + 2) / 5)
        + (365 * (year + 4800 - ((14 - month) / 12)))
        + ((year + 4800 - ((14 - month) / 12)) / 4)
        - ((year + 4800 - ((14 - month) / 12)) / 100)
        + ((year + 4800 - ((14 - month) / 12)) / 400)
        - 32045
      ) % 7;
	  if (!start_sunday) dow = (dow+6) % 7;
	  return dow;
}

//! Return text for day of the week.
/*! Return S,M,T,W... for lvl==0. Sun,Mon,Tue,... for lvl==1, or Sunday,Monday,...
 *
 * sunday is which position "sunday" occupies. 0 means the first position. Say sunday==6,
 * then day==0 will return "Monday".
 */
const char *dayofweek(int day, int lvl, int sunday)
{
	day = (day+sunday) % 7;

	const char *str = nullptr;
	if      (day == 0) str = _("Sunday");
	else if (day == 1) str = _("Monday");
	else if (day == 2) str = _("Tuesday");
	else if (day == 3) str = _("Wednesday");
	else if (day == 4) str = _("Thursday");
	else if (day == 5) str = _("Friday");
	else if (day == 6) str = _("Saturday");
	else return nullptr;

	if (lvl > 1) return str;

	 //bit of a kludge to ease translating, automatically extract
	 //the first 1 or 3 letters of the day as necessary.
	 //each character can theoretically be 1 to 6 bytes long.
	if (lvl == 0) {
		 //single character abreviation
		static char abrv1[56];
		int i=0,d=day*7;
		abrv1[d+i]=str[i]; i++; while (str[i]&128) { abrv1[d+i]=str[i]; i++; }
		abrv1[d+i]='\0';
		return abrv1+d;

	} else if (lvl == 1) {
		 //three character abreviation
		static char abrv3[140];
		int i=0,d=day*20;
		abrv3[d+i]=str[i]; i++; while (str[i]&128) { abrv3[d+i]=str[i]; i++; }
		abrv3[d+i]=str[i]; i++; while (str[i]&128) { abrv3[d+i]=str[i]; i++; }
		abrv3[d+i]=str[i]; i++; while (str[i]&128) { abrv3[d+i]=str[i]; i++; }
		abrv3[d+i]='\0';
		return abrv3+d;
	}

	return str;
}

//! Return the month name. Month is [1..12].
/*! If lvl==0, return Jan, Feb, ...
 *  If lvl==1, return January, February, ...
 */
const char *monthname(int month,int lvl)
{
	const char *str = nullptr;
	if      (month== 1) str=_("January");
	else if (month== 2) str=_("February");
	else if (month== 3) str=_("March");
	else if (month== 4) str=_("April");
	else if (month== 5) str=_("May");
	else if (month== 6) str=_("June");
	else if (month== 7) str=_("July");
	else if (month== 8) str=_("August");
	else if (month== 9) str=_("September");
	else if (month==10) str=_("October");
	else if (month==11) str=_("November");
	else if (month==12) str=_("December");
	else return nullptr;
	if (lvl == 1) return str;

	 //bit of a kludge to ease translating, automatically extract
	 //the first 3 utf8 letters of month as necessary.
	 //each character can theoretically be 1 to 6 bytes long.
	int i = 0, m = (month-1)*20;
	static char abrv[240];
	abrv[m+i]=str[i]; i++; while (str[i]&128) { abrv[m+i]=str[i]; i++; }
	abrv[m+i]=str[i]; i++; while (str[i]&128) { abrv[m+i]=str[i]; i++; }
	abrv[m+i]=str[i]; i++; while (str[i]&128) { abrv[m+i]=str[i]; i++; }
	abrv[m+i]='\0';
	return abrv+m;
}


//---------------------------- LaxDate -----------------------------------------

/*! \class LaxDate
 * \brief Small class to help slightly with date manipulation, based on the Gregorian calendar.
 *
 * LaxDate will always have a valid date. That is, assuming there are no programming bugs.
 *
 * This class assumes a purely Gregorian calendar, and this ignores the deletion of 10 days in
 * October 1582. In 8000 years or so when the calendar is off by a day, you will be screwed.
 */


//! Initialize to current date.
LaxDate::LaxDate()
  : year(0), month(0), day(0)
{
	SetToToday();
}

LaxDate::LaxDate(int year, int month, int day)
{
	Set(year,month,day);
}

LaxDate::LaxDate(const LaxDate &d)
{
	year      = d.year;
	month     = d.month;
	day       = d.day;
}

LaxDate &LaxDate::operator=(LaxDate &d)
{
	year      = d.year;
	month     = d.month;
	day       = d.day;
	return d;
}

/*! If it is an invalid date, then a nonzero value is returned. Else date is set, and 0 is returned.
 *
 * day starts at 1. Month is in range [1..12]. year can be anything.
 */
int LaxDate::Set(int y, int m, int d)
{
	if (m<1 || m>12) return 1;
	if (d < 1 || d > days_in_month(m, y)) return 2;

	year      = y;
	month     = m;
	day       = d;

	return 0;
}

void LaxDate::SetToToday()
{
	struct tm d;
	memset(&d, 0, sizeof(struct tm));
	time_t t = time(nullptr);
	localtime_r(&t, &d); //seconds from the epoch

	year      = d.tm_year + 1900;
	month     = d.tm_mon + 1; // to make it [1..12]
	day       = d.tm_mday;    // is [1..31]

	_time.SetToNow();
}

//! Return the number of days after a Sunday. That is, if on a sunday, return 0, monday is 1, etc.
int LaxDate::DayOfWeek() const
{
	int dayofweek = day_of_week(year, month, day, true);
	// int dayofweek = (*this - LaxDate(2011,10,16))%7;
	// if (dayofweek<0) dayofweek = 7 + dayofweek;
	return dayofweek;
}

/*! 0 is the first day.
 */
int LaxDate::DayOfYear() const
{
	int dayofyear = 0;
	for (int c = 1; c < month; c++) dayofyear += days_in_month(c,year);
	dayofyear += day - 1;
	return dayofyear;
}

/*! 0 is the first week.
 */
int LaxDate::WeekOfYear() const
{
	return (DayOfYear() - DayOfWeek() - 1 + 7) / 7;
}

void LaxDate::AddDays(int days)
{
	day += days;

	while (day > days_in_month(month,year)) {
		day -= days_in_month(month,year);
		month++;
		if (month>=12) { month=12; year++; }
	}
	while (day < 1) {
		month--;
		if (month<0) { month=11; year--; }
		day += days_in_month(month,year);
	}
}

/*! Just return AddDays(weeks*7).
 */
void LaxDate::AddWeeks(int weeks)
{
	return AddDays(7*weeks);
}

/*! If you add one month and the day is greater than number of days in the new month,
 * then the day is clamped to the end of that month. If it is clamped, then 1 is returned.
 * Else 0 is returned.
 */
int LaxDate::AddMonths(int months)
{
	year  += months/12;
	month += months%12;

	if (month >= 12) { month -= 12; year++; }
	else if (month < 0) { month += 12; year--; }

	int mdays = days_in_month(month, year);
	if (day >= mdays) { day = mdays; return 1; }
	return 0;
}

void LaxDate::AddYears(int years)
{
	year += years;
}

/*! Add years first, then months, then days.
 * Warning: The month add might change the day! See AddMonths().
 */
void LaxDate::Add(int years, int months, int days)
{
	AddYears(years);
	AddMonths(months);
	AddDays(days);
}


//! Return the number of days between dates. Note this is negative when d1 is later than d2.
int operator-(const LaxDate &d2, const LaxDate &d1)
{
	// *** this is a lazy way to do it:
	int n=0, y1,y2;
	if (d1<d2) {
		y1=d1.Year(); y2=d2.Year();
		for (int y=y1+1; y<y2-1; y++) n+=days_in_year(y);
		if (y1==y2) n+=d2.DayOfYear()-d1.DayOfYear();
		else n+=d2.DayOfYear()+days_in_year(y1)-d1.DayOfYear();
	} else {
		y1=d2.Year(); y2=d1.Year();
		for (int y=y1+1; y<y2-1; y++) n-=days_in_year(y);
		if (y1==y2) n+=d2.DayOfYear()-d1.DayOfYear();
		else n-=d1.DayOfYear()+days_in_year(y2)-d2.DayOfYear();
	}
	return n;
}

/*! This is a wrapper for strftime(), and has the same return value, which is
 * the number of characters including a terminating null written to buffer if that
 * number is less than or equal to max. If greater than max, then the number is
 * returned and buffer is not modified.
 *
 * Example:
 * - `date.Format("%Y-%m-%dT%H:%M:%SZ", buffer, max);` -> `2025-06-29T11:30:45Z`
 */
int LaxDate::Format(const char *fmt, char *buffer, int &max)
{
	struct tm d;
	memset(&d, 0, sizeof(struct tm));
	d.tm_sec  = (int)_time.Second();    /* Seconds          [0, 60] */
	d.tm_min  = _time.Minute();    /* Minutes          [0, 59] */
	d.tm_hour = _time.Hour();   /* Hour             [0, 23] */
	d.tm_mday = day;   /* Day of the month [1, 31] */
	d.tm_mon  = month-1;   /* Month            [0, 11]  (January = 0) */
	d.tm_year = year-1900;   /* Year minus 1900 */
	d.tm_wday = DayOfWeek();   /* Day of the week  [0, 6]   (Sunday = 0) */
	d.tm_yday = DayOfYear();   /* Day of the year  [0, 365] (Jan/01 = 0) */
	//d.tm_isds = t;  /* Daylight savings flag */
	//long tm_gmtoff; /* Seconds East of UTC */
	//const char *tm_zone;   /* Timezone abbreviation */

	return strftime(buffer, max, fmt, &d);
}

bool operator<(const LaxDate &d1, const LaxDate &d2)
{
	if (d1.Year()<d2.Year()) return true;
	if (d1.Year()>d2.Year()) return false;
	if (d1.Month()<d2.Month()) return true;
	if (d1.Month()>d2.Month()) return false;
	if (d1.Day()<d2.Day()) return true;
	if (d1.Day()>d2.Day()) return false;
	return false; //they are the same Day()
}

bool operator>(const LaxDate &d1, const LaxDate &d2)
{
	if (d1.Year()>d2.Year()) return true;
	if (d1.Year()<d2.Year()) return false;
	if (d1.Month()>d2.Month()) return true;
	if (d1.Month()<d2.Month()) return false;
	if (d1.Day()>d2.Day()) return true;
	if (d1.Day()<d2.Day()) return false;
	return false; //they are the same Day()
}

bool operator<=(const LaxDate &d1, const LaxDate &d2)
{
	if (d1.Year()<d2.Year()) return true;
	if (d1.Year()>d2.Year()) return false;
	if (d1.Month()<d2.Month()) return true;
	if (d1.Month()>d2.Month()) return false;
	if (d1.Day()<=d2.Day()) return true;
	return false;
}

bool operator>=(const LaxDate &d1, const LaxDate &d2)
{
	if (d1.Year()>d2.Year()) return true;
	if (d1.Year()<d2.Year()) return false;
	if (d1.Month()>d2.Month()) return true;
	if (d1.Month()<d2.Month()) return false;
	if (d1.Day()>=d2.Day()) return true;
	return false;
}

bool operator==(const LaxDate &d1, const LaxDate &d2)
{
	return d1.Year() == d2.Year() && d1.Month() == d2.Month() && d1.Day() == d2.Day();
}


//------------------------------------ LaxTime --------------------------------

LaxTime::LaxTime(int h, int m, double s)
{
	hour   = h;
	minute = m;
	second = s;
}

void LaxTime::SetToNow()
{
	struct timespec ts; // tv_sec, tv_nsec (nanoseconds)
	clock_gettime(CLOCK_REALTIME, &ts); // sec+nsec since the epoch, Jan 1 1970

	struct tm d;
	memset(&d, 0, sizeof(struct tm));
	localtime_r(&ts.tv_sec, &d);

	hour = d.tm_hour;
	minute = d.tm_min;
	second = d.tm_sec + ts.tv_nsec * 1e-9;
}

/*! Static constructor. */
LaxTime LaxTime::Now()
{
	LaxTime t;
	t.SetToNow();
	return t;
}

/*! Static constructor */
LaxTime LaxTime::FromSeconds(double s)
{
	LaxTime t;
	t.hour = s/60/60;
	s -= t.hour*60*60;
	t.minute = s/60;
	s -= t.minute*60;
	t.second = s;
	return t;
}


// seconds between instances
double operator-(const LaxTime &t1, const LaxTime &t2)
{
	double s = (t1.Hour() - t2.Hour())*60*60 + (t1.Minute() - t2.Minute())*60 + (t1.Second() - t2.Second());
	return s;
}

bool operator< (const LaxTime &t1, const LaxTime &t2)
{
	if (t1.hour > t2.hour) return false;
	if (t1.hour < t2.hour) return true;
	if (t1.minute > t2.minute) return false;
	if (t1.minute < t2.minute) return true;
	return t1.second < t2.second;
}

bool operator> (const LaxTime &t1, const LaxTime &t2)
{
	if (t1.hour > t2.hour) return true;
	if (t1.hour < t2.hour) return false;
	if (t1.minute > t2.minute) return true;
	if (t1.minute < t2.minute) return false;
	return t1.second > t2.second;
}

bool operator<=(const LaxTime &t1, const LaxTime &t2)
{
	if (t1.hour > t2.hour) return false;
	if (t1.hour < t2.hour) return true;
	if (t1.minute > t2.minute) return false;
	if (t1.minute < t2.minute) return true;
	return t1.second <= t2.second;
}

bool operator>=(const LaxTime &t1, const LaxTime &t2)
{
	if (t1.hour > t2.hour) return true;
	if (t1.hour < t2.hour) return false;
	if (t1.minute > t2.minute) return true;
	if (t1.minute < t2.minute) return false;
	return t1.second >= t2.second;
}

bool operator==(const LaxTime &t1, const LaxTime &t2)
{
	return t1.hour == t2.hour && t1.minute == t2.minute && t1.second == t2.second;
}


} //namespace Laxkit


