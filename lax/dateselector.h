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
#ifndef _LAX_DATESELECTOR_H
#define _LAX_DATESELECTOR_H

#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

namespace Laxkit {




//------------------------------- DateSelector ------------------------------

#define DATESEL_WITH_TIME     (1<<16)
#define DATESEL_MONDAY_FIRST  (1<<17)

class DateSelector : public anXWindow
{
  protected:
	struct tm date;
	int firstdayshown, firstmonthshown, firstyearshown;
	int headerlines;
	int mo_col, mo_row, mo_month, mo_day, mo_year;

	ButtonDownInfo buttondown;
	virtual int send();
	
  public:
	DateSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int year,int month,int day, int secinday);
	virtual ~DateSelector();
	virtual const char *whattype() { return "DateSelector"; }
	virtual int init();
	virtual void Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const LaxMouse *d);
	//virtual int MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state, const LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int MouseMove(int mx,int my, unsigned int state, const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d);

	//virtual int SetTimeZone(int zone);
	virtual int SetDate(int year, int month, int day, int displayonly);

	 //serializing aids
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
};

} // namespace Laxkit

#endif 

