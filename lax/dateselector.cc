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


#include <lax/dateselector.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/dateutils.h>
#include <lax/language.h>


#include <iostream>
using namespace std;
#define DBG 

using namespace LaxFiles;

namespace Laxkit {


#define DBUT_None      0
#define DBUT_PrevYear  1
#define DBUT_NextYear  2
#define DBUT_PrevMonth 3
#define DBUT_NextMonth 4
#define DBUT_EditMonth 5
#define DBUT_EditYear  6
#define DBUT_EditTime  7

#define HOVER 1000


//------------------------------- DateSelector ------------------------------
/*! \class DateSelector
 * \brief Pick a date from a gregorian calendar.
 */



DateSelector::DateSelector(anXWindow *parnt,const char *nname,const char *ntitle, unsigned long nstyle,
							int nx,int ny,int nw,int nh,int brder,
							anXWindow *prev,unsigned long nowner,const char *mes,
							int year,int month,int day, int secinday)
	: anXWindow(parnt,nname,ntitle,nstyle,nx,ny,nw,nh,brder,prev,nowner,mes)
{
	InstallColors(THEME_Edit);

	memset(&date, 0, sizeof(struct tm));
	SetDate(year,month,day, 0);
	date.tm_hour=secinday/60/60;
	date.tm_min=(secinday-date.tm_hour*60)/60;
	date.tm_sec=(secinday-date.tm_hour*60)*60-date.tm_min*60;

	if (win_h==0 || win_w==0) {
		int h=app->defaultlaxfont->textheight();
		if (win_h<=0) win_h=h*(8+((win_style&DATESEL_WITH_TIME)?1:0));
		if (win_w<=0) win_w=getextent("   00  00  00  00  00  00  00   ",-1,NULL,NULL);
	}

	headerlines=((win_style&DATESEL_WITH_TIME)?1:0)+2;
	mo_col=mo_row=-1;
	mo_month=mo_day=mo_year=-1;
}

DateSelector::~DateSelector()
{}

//! month is in [1..12], day should be [1..max days of that month]. If out of range, use current date.
/*! If displayonly, do not set current day, but move the currently displayed date to the given date.
 */
int DateSelector::SetDate(int year, int month, int day, int displayonly)
{
	struct tm d;
	memset(&d, 0, sizeof(struct tm));

	if (day<=0 || day>31 || month<=0 || month>12) {
		//struct tm *localtime_r(const time_t *timep, struct tm *result);
		time_t t=time(NULL);
		localtime_r(&t, &d); //seconds from the epoch
		d.tm_year+=1900;
		d.tm_mon++; //to make it [1..12]

	} else {
		char buf[100];
		sprintf(buf,"%d/%d/%d",year,month,day);
		strptime(buf, "%Y/%m/%d", &d);
		d.tm_year+=1900;
		d.tm_mon++; //to make it [1..12]
	}

	firstdayshown  =d.tm_mday;
	firstmonthshown=d.tm_mon;
	firstyearshown =d.tm_year;
	if (!(firstdayshown==1 && d.tm_wday==((win_style&DATESEL_MONDAY_FIRST)?1:0))) {
		firstmonthshown=firstmonthshown-1;
		if (firstmonthshown<1) {
			firstmonthshown=12;
			firstyearshown--;
		}
		firstdayshown=days_in_month(firstmonthshown,firstyearshown)-d.tm_wday+1;
	}

	if (!displayonly) {
		date.tm_mday=d.tm_mday;
		date.tm_mon =d.tm_mon;
		date.tm_year=d.tm_year;
		date.tm_wday=d.tm_wday;
		date.tm_yday=d.tm_yday;
	}

	needtodraw=1;
	return 0;
}

int DateSelector::init()
{
	return 0;
}


/*! Puts coloreventdata->RGBA_max(red,green,blue,alpha, max).
 */
int DateSelector::send()
{
	if (!win_owner || !win_sendthis) return 0;

	char buf[40];
	strftime(buf,40,"%T", &date);
	SimpleMessage *e=new SimpleMessage("time???",date.tm_year,date.tm_mon,date.tm_mday, 0, win_sendthis, object_id, win_owner);
	app->SendMessage(e, win_owner,win_sendthis, object_id);

	return 1;
}

void DateSelector::Refresh()
{
	if (!win_on || !needtodraw) return;

	int textheight=app->defaultlaxfont->textheight();
	int h=textheight;
	int y=0;
	int bufmax=300;
	char buf[300];
	double colwidth=win_w/7.;

	int c=0;
	int day=firstdayshown, month=firstmonthshown, year=firstyearshown;
	int primarymonth,primaryyear; //whichever month is most prominent in the display??
	if (mo_month>0) {
		primarymonth=mo_month;
		primaryyear=mo_year;
	} else {
		if (day==1) {
			primarymonth=month;
			primaryyear=year;
		} else {
			primaryyear=year;
			primarymonth=month+1;
			if (primarymonth>12) {
				primaryyear++;
				primarymonth=1;
			}
		}
	}

	 //blank out background
	clear_window(this);
	foreground_color(coloravg(win_themestyle->bg,win_themestyle->fg,.2));
	fill_rectangle(this, 0,0,win_w,headerlines*h);


//           5:23 pm
//  <<  <  September 2011  >  >>
//    s   m   t   w   t   f   s
//   31   1   2   3   4   5   6
//
//colors: weekday bg,
//        weekend bg,
//        time col,
//        month/year/arrows color
//        day of week name bg

	 //draw time
	if (win_style&DATESEL_WITH_TIME) {
		//size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
		size_t n=strftime(buf,bufmax, "%r", &date);
		textout(this, buf,n, win_w/2,y, LAX_HCENTER|LAX_TOP);
		y+=textheight;
	}

	 //draw arrow buttons, month and year
	foreground_color(win_themestyle->fg.Pixel());
	draw_thing(this, h/2,y+h/2, h/2,h/2, 1, THING_Double_Triangle_Left);
	draw_thing(this, 2*h,y+h/2, h/4,h/2, 1, THING_Triangle_Left);
	sprintf(buf,"%s %d",monthname(primarymonth,1),primaryyear);
	textout(this, buf,-1, win_w/2,y, LAX_HCENTER|LAX_TOP);
	draw_thing(this, win_w-2*textheight,y+h/2, h/4,h/2, 1, THING_Triangle_Right);
	draw_thing(this, win_w-h/2,y+h/2, h/2,h/2, 1, THING_Double_Triangle_Right);
	y+=textheight;

	 //draw day of week names
	for (int c=0; c<7; c++) {
		textout(this, dayofweek(c,0,(win_style&DATESEL_MONDAY_FIRST)?1:0),-1, (c+.5)*colwidth,y, LAX_HCENTER|LAX_TOP);
	}
	y+=textheight;

	 //draw days

	int r=headerlines;
	while (y<win_h) {
		 //highlight mouse over box
		if (c==mo_col && r==mo_row) {
			foreground_color(coloravg(win_themestyle->fg,win_themestyle->bg,.9));
			fill_rectangle(this, c*win_w/7,y, win_w/7,textheight);
		}

		 //box around current day
		if (day==date.tm_mday && month==date.tm_mon && year==date.tm_year) {
			foreground_color(win_themestyle->fg.Pixel());
			draw_rectangle(this, c*win_w/7,y, win_w/7,textheight);
		}

		 //make primary month more prominent
		if (month==primarymonth && year==primaryyear) foreground_color(win_themestyle->fg.Pixel());
		else foreground_color(coloravg(win_themestyle->fg,win_themestyle->bg,.5));

		sprintf(buf,"%d",day);
		textout(this, buf,-1, (c+.5)*colwidth,y, LAX_HCENTER|LAX_TOP);

		day++;
		if (day>days_in_month(month,year)) {
			month++;
			if (month>12) {
				year++;
				month=1;
			}
			day=1;
		}

		c++;
		if (c==7) {
			r++;
			y+=textheight;
			c=0;
		}
	}

	SwapBuffers();
	needtodraw=0;
	return;
}

////! Change blue by default.
//int DateSelector::RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
//{
//	buttondown.down(d->id, RIGHTBUTTON, x,y);
//	return 0;
//}
//
//int DateSelector::RBUp(int x,int y, unsigned int state, const LaxMouse *d)
//{
//	buttondown.up(d->id, RIGHTBUTTON);
//	return 0;
//}
//
//int DateSelector::MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
//{ return 0; }
//
//int DateSelector::MBUp(int x,int y, unsigned int state, const LaxMouse *d)
//{ return 0; }

int DateSelector::LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	buttondown.down(d->id,LEFTBUTTON,x,y);
	return 0;
}

int DateSelector::LBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	int dragged=buttondown.up(d->id,LEFTBUTTON);
	if (dragged) return 0;

	if (mo_row==headerlines-2) {
		int m=firstmonthshown, y=firstyearshown, d=firstdayshown;
		if (d!=1) {
			d=1;
			m=m+1;
			if (m>12) { m=1; y++; }
		}
		if (mo_col==DBUT_PrevYear) {
			SetDate(y-1,m,d, 1);
			return 0;
		}
		if (mo_col==DBUT_NextYear) {
			SetDate(y+1,m,d, 1);
			return 0;
		}
		if (mo_col==DBUT_PrevMonth) {
			m--;
			if (m<1) {
				y--;
				m=12;
			}
			if (d>days_in_month(m,y)) d=days_in_month(m,y);
			SetDate(y,m,d, 1); return 0; 
		}
		if (mo_col==DBUT_NextMonth) {
			m++;
			if (m>12) {
				y++;
				m=1;
			}
			if (d>days_in_month(m,y)) d=days_in_month(m,y);
			SetDate(y,m,d, 1); return 0; 
		}
		return 0;
	}

	return 0;
}

/*! Drag left, middle, right click for red, green, blue. 
 */
int DateSelector::MouseMove(int x,int y,unsigned int state, const LaxMouse *d)
{
	buttondown.move(d->id,x,y);

	int textheight=app->defaultlaxfont->textheight();
	int row=-1,col=-1;

	mo_month=mo_day=mo_year=-1;

	if (buttondown.isdown(d->id,LEFTBUTTON) && buttondown.isdragged(d->id,LEFTBUTTON)) {
		row=-1;
		col=-1;
	} else {
		row=y/textheight;
		if (row>=headerlines) {
			col=x/(win_w/7);
			mo_month=firstmonthshown;
			mo_day  =firstdayshown+col;
			mo_year =firstyearshown;
			for (int n=row-headerlines; n>=0; n--) {
				if (n!=0) mo_day+=7;
				if (mo_day>days_in_month(mo_month,mo_year)) {
					mo_day-=days_in_month(mo_month,mo_year);
					mo_month++;
					if (mo_month>12) {
						mo_month=1;
						mo_year++;
					}
				}
			}
		} else {
			if (row==0 && headerlines==3) col=DBUT_EditTime;
			else if (row==headerlines-2) {
				if (x<1.5*textheight) col=DBUT_PrevYear;
				else if (x<2.5*textheight) col=DBUT_PrevMonth;
				else if (x>win_w-1.5*textheight) col=DBUT_NextYear;
				else if (x>win_w-2.5*textheight) col=DBUT_NextMonth;
				else col=DBUT_EditMonth;
			}
		}
	}
	if (row!=mo_row || col!=mo_col) {
		mo_row=row;
		mo_col=col;
		needtodraw=1;
	}

	return 0;
}

int DateSelector::WheelUp(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	firstdayshown-=7;
	if (firstdayshown<1) {
		firstmonthshown--;
		if (firstmonthshown<1) {
			firstyearshown--;
			firstmonthshown=12;
		}
		firstdayshown+=days_in_month(firstmonthshown,firstyearshown);
	}
	needtodraw=1;
	return 0;
}

int DateSelector::WheelDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	firstdayshown+=7;
	int oldmd=days_in_month(firstmonthshown,firstyearshown);
	if (firstdayshown>oldmd) {
		firstmonthshown++;
		if (firstmonthshown>12) {
			firstyearshown++;
			firstmonthshown=1;
		}
		firstdayshown-=oldmd;
	}
	needtodraw=1;
	return 0;
}

/*! While a button is pressed, pushing r,g,b,c,m,y,k will make that button shift
 * the corresponding color.
 */
int DateSelector::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d)
{
	if (ch==' ') {
		SetDate(date.tm_year,date.tm_mon,date.tm_mday, 1);
		return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state,d);
}

/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *DateSelector::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	if (what==-1) {
		att->push("withtime","Present if show a time field also");
		att->push("date","Current date, in format 2012-12-31");
		att->push("time","13:00:34 PST");
		return att;
	}

	if (win_style&DATESEL_WITH_TIME) att->push("withtime");

	int bufmax=30;
	char buf[bufmax];

	strftime(buf,bufmax, "%Y-%m-%d", &date);
	att->push("date",buf);

	strftime(buf,bufmax, "%r", &date);
	att->push("time",buf);

	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void DateSelector::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"withtime")) {
			setWinStyle(DATESEL_WITH_TIME,1);

		} else if (!strcmp(name,"date")) {
			strptime(value, "%Y-%m-%d", &date);

		} else if (!strcmp(name,"time")) {
			strptime(value, "%r", &date);

		}
	}
}

} // namespace Laxkit

