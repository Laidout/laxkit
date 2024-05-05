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
//    Copyright (C) 2004-2010,2014 by Tom Lechner
//
#ifndef _LAX_ITEMSLIDER_H
#define _LAX_ITEMSLIDER_H


#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

namespace Laxkit {

class ItemSlider : public anXWindow
{
  protected:
	int nitems;
	int curitem;

	Laxkit::ButtonDownInfo buttondown;
	Laxkit::ButtonDownInfo buttonglobal;
	int hover;

	virtual int send();
	virtual int getid(int i) = 0;
	virtual int numitems()   = 0;

	virtual int Mode(int newmode);

  public:
	 // SENDALL means send sendthis message on Idle,Select/Previous/Next
	 // AUTOSHIFT means move mouse outside window then sit also selects prev/next based on (how far out of window)/movewidth
	 //!these are passed in with nstyle in constructor
	enum ItemSliderFlags {
		XSHIFT    =(1<<16),
		YSHIFT    =(1<<17),
		SENDALL   =(1<<18),
		AUTOSHIFT =(1<<19),
		POPUP     =(1<<20),
		EDITABLE  =(1<<21),
		MAX       =(1<<21)
	};

	int movewidth; // moving past this num in x/y directions selects next or prev

	ItemSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsendthis);
	virtual ~ItemSlider();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int scan(int x,int y,unsigned int state);

	virtual void Refresh() = 0;

	virtual int GetCurrentItemId();
	virtual int SelectPrevious(double multiplier);
	virtual int SelectNext(double multiplier);
	virtual int Select(int id);
};

} // namespace Laxkit

#endif

