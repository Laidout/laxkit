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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_ITEMSLIDER_H
#define _LAX_ITEMSLIDER_H


#include <lax/anxapp.h>

namespace Laxkit {

 // SENDALL means send sendthis message on Idle,Select/Previous/Next
 // AUTOSHIFT means move mouse outside window then sit also selects prev/next based on (how far out of window)/movewidth
#define ITEMSLIDER_XSHIFT    (1<<16)
#define ITEMSLIDER_YSHIFT    (1<<17)
#define ITEMSLIDER_SENDALL   (1<<18)
#define ITEMSLIDER_AUTOSHIFT (1<<19)
#define ITEMSLIDER_POPUP     (1<<20)

class ItemSlider : public anXWindow
{
  protected:
	int timerid;
	int nitems;
	int curitem,lbitem;
	int mx,my,lx,ly;
	int ox,oy;
	int buttondown, buttondowndevice;
	virtual int send();
	virtual int getid(int i)=0;
	virtual int numitems()=0;
  public:
	int movewidth; // moving past this num in x/y dirs selects next or prev
	ItemSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsendthis);
	virtual ~ItemSlider();
	virtual int init();
	virtual int Idle(int tid=0);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d) { SelectNext(); send(); return 0; }
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d) { SelectPrevious(); send(); return 0; }
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);

	virtual void Refresh() = 0;

	virtual int GetCurrentItemId();
	virtual int SelectPrevious();
	virtual int SelectNext();
	virtual int Select(int id);
};

} // namespace Laxkit

#endif

