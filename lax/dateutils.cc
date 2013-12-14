//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

//! Return text for day of the week.
/*! Return S,M,T,W... for lvl==0. Sun,Mon,Tue,... for lvl==1, or Sunday,Monday,...
 *
 * sunday is which position "sunday" occupies. 0 means the first position. Say sunday==6,
 * then day==0 will return "Monday".
 */
const char *dayofweek(int day, int lvl, int sunday)
{
	day=(day+sunday)%7;

	const char *str=NULL;
	if (day==0) str=_("Sunday");
	else if (day==1) str=_("Monday");
	else if (day==2) str=_("Tuesday");
	else if (day==3) str=_("Wednesday");
	else if (day==4) str=_("Thursday");
	else if (day==5) str=_("Friday");
	else if (day==6) str=_("Saturday");
	else return NULL;

	if (lvl>1) return str;

	 //bit of a kludge to ease translating, automatically extract
	 //the first 1 or 3 letters of the day as necessary.
	 //each character can theoretically be 1 to 6 bytes long.
	if (lvl==0) {
		 //single character abreviation
		static char abrv1[56];
		int i=0,d=day*7;
		abrv1[d+i]=str[i]; i++; while (str[i]&128) { abrv1[d+i]=str[i]; i++; }
		abrv1[d+i]='\0';
		return abrv1+d;

	} else if (lvl==1) {
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
	const char *str=NULL;
	if (month== 1) str=_("January");
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
	else return NULL;
	if (lvl==1) return str;

	 //bit of a kludge to ease translating, automatically extract
	 //the first 3 letters of month as necessary.
	 //each character can theoretically be 1 to 6 bytes long.
	int i=0,m=(month-1)*20;
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
 * LaxDate will always have a valid date. That is, assuming there are no programming bugs
 *
 * This class assumes a purely Gregorian calendar, and this ignores the deletion of 10 days in
 * October 1582. In 8000 years or so when the calendar is off by a day, you will be screwed.
 */


//! Initialize to current date.
LaxDate::LaxDate()
  : year(0),month(0),day(0),dayofweek(-1),dayofyear(-1)
{
	SetToToday();
}

LaxDate::LaxDate(int year, int month, int day)
{
	Set(year,month,day);
}

LaxDate::LaxDate(const LaxDate &d)
{
	year=d.year;
	month=d.month;
	day=d.day;
	dayofweek=d.dayofweek;
	dayofyear=d.dayofyear;
}

LaxDate &LaxDate::operator=(LaxDate &d)
{
	year=d.year;
	month=d.month;
	day=d.day;
	dayofweek=d.dayofweek;
	dayofyear=d.dayofyear;
	return d;
}

/*! If it is an invalid date, then a nonzero value is returned. Else date is set, and 0 is returned.
 *
 * day starts at 1. Month is in range [1..12]. year can be anything.
 */
int LaxDate::Set(int y, int m, int d)
{
	if (m<1 || m>12) return 1;
	if (d<1 || d>days_in_month(m,y)) return 2;

	year=y;
	month=m;
	day=d;
	dayofweek=-1;
	dayofyear=-1;

	return 0;
}

void LaxDate::SetToToday()
{
	struct tm d;
	memset(&d, 0, sizeof(struct tm));
	time_t t=time(NULL);
	localtime_r(&t, &d); //seconds from the epoch

	year =d.tm_year+1900;
	month=d.tm_mon+1; //to make it [1..12]
	day  =d.tm_mday;
	dayofweek=d.tm_wday;
	dayofyear=d.tm_yday;
}

//! Return the number of days after a Sunday. That is, if on a sunday, return 0, monday is 1, etc.
int LaxDate::DayOfWeek()
{
	if (dayofweek<0){
		dayofweek= (*this-LaxDate(2011,10,16))%7;
		//DBG cerr <<"dow diff:"<<(LaxDate(2011,10,16)-*this)
		//DBG      <<"dow diff:"<<(*this-LaxDate(2011,10,16))<<"  %7:"<<dayofweek<<endl;
		if (dayofweek<0) dayofweek=7+dayofweek;
	}
	return dayofweek;
}

int LaxDate::DayOfYear()
{
	if (dayofyear<0){
		dayofyear=0;
		for (int c=1; c<month; c++) dayofyear+=days_in_month(c,year);
		dayofyear+=day-1;
	}
	return dayofyear;
}

//! Return the number of days between dates. Note this is negative when d1 is later than d2.
int operator-(LaxDate d2,LaxDate d1)
{
	DBG cerr <<"date "<<d2.Year()<<"/"<<d2.Month()<<"/"<<d2.Day()<<" - "<<d1.Year()<<"/"<<d1.Month()<<"/"<<d1.Day()<<" = ";

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

	DBG cerr <<n<<endl;
	return n;
}

int operator<(LaxDate d1,LaxDate d2)
{
	if (d1.Year()<d2.Year()) return 1;
	if (d1.Year()>d2.Year()) return 0;
	if (d1.Month()<d2.Month()) return 1;
	if (d1.Month()>d2.Month()) return 0;
	if (d1.Day()<d2.Day()) return 1;
	if (d1.Day()>d2.Day()) return 0;
	return 0; //they are the same Day()
}

int operator>(LaxDate d1,LaxDate d2)
{
	if (d1.Year()>d2.Year()) return 1;
	if (d1.Year()<d2.Year()) return 0;
	if (d1.Month()>d2.Month()) return 1;
	if (d1.Month()<d2.Month()) return 0;
	if (d1.Day()>d2.Day()) return 1;
	if (d1.Day()<d2.Day()) return 0;
	return 0; //they are the same Day()
}

int operator<=(LaxDate d1,LaxDate d2)
{
	if (d1.Year()<d2.Year()) return 1;
	if (d1.Year()>d2.Year()) return 0;
	if (d1.Month()<d2.Month()) return 1;
	if (d1.Month()>d2.Month()) return 0;
	if (d1.Day()<=d2.Day()) return 1;
	return 0;
}

int operator>=(LaxDate d1,LaxDate d2)
{
	if (d1.Year()>d2.Year()) return 1;
	if (d1.Year()<d2.Year()) return 0;
	if (d1.Month()>d2.Month()) return 1;
	if (d1.Month()<d2.Month()) return 0;
	if (d1.Day()>=d2.Day()) return 1;
	return 0;
}

int operator==(LaxDate d1,LaxDate d2)
{
	return d1.Year()==d2.Year() && d1.Month()==d2.Month() && d1.Day()==d2.Day();
}


} //namespace Laxkit


