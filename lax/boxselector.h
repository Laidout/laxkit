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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_BOXSELECTOR_H
#define _LAX_BOXSELECTOR_H

#include <lax/anxapp.h>
#include <lax/boxarrange.h>
#include <lax/buttondowninfo.h>


 //arrange boxes in columns or in rows
#define BOXSEL_COLUMNS     (1<<16)
#define BOXSEL_VERTICAL    (1<<16)
#define BOXSEL_ROWS        (1<<17)
#define BOXSEL_HORIZONTAL  (1<<17)

 //how to align the boxes in the window
#define BOXSEL_CENTER      (1<<19|1<<22)
#define BOXSEL_LEFT        (1<<18)
#define BOXSEL_HCENTER     (1<<19)
#define BOXSEL_RIGHT       (1<<20)
#define BOXSEL_TOP         (1<<21)
#define BOXSEL_VCENTER     (1<<22)
#define BOXSEL_BOTTOM      (1<<23)

 // how to fill gaps between boxes, whether to stretch them, or put a gap between them
 // these refer to filling whole window
#define BOXSEL_STRETCH     (1<<24|1<<25)
#define BOXSEL_STRETCHX    (1<<24)
#define BOXSEL_STRETCHY    (1<<25)
#define BOXSEL_SPACE       (1<<26|1<<27)
#define BOXSEL_SPACEX      (1<<26)
#define BOXSEL_SPACEY      (1<<27)

 // these refer to filling extra (width in cols) or (height in rows)
#define BOXSEL_STRETCH_IN_ROW (1<<28)
#define BOXSEL_STRETCH_IN_COL (1<<28)

 // make the panel look flat, mouse over highlights, selected ones are elevated.
 // Otherwise default is all are either up or down, or flat if grayed
#define BOXSEL_FLAT        (1<<29)
#define BOXSEL_ONE_ONLY    (1<<30)



namespace Laxkit {

//------------------------------- SelBox --------------------------------
class SelBox : public SquishyBox
{
  public:
	unsigned int state;
	int info,id;
	int mousecount;
	
	SelBox(int xx,int yy,int ww,int hh,int nid);
	SelBox(int nid=0);
	virtual ~SelBox() {}
};

//------------------------------- BoxSelector --------------------------------

class BoxSelector : public anXWindow, public RowColBox
{ 
  protected:
	int hoverbox;
	int curbox;

	ButtonDownInfo buttondown;

	virtual void togglebox(int which,int db=1);

  public:	
	unsigned long highlight,shadow;
	int pad,padi,bevel;

	BoxSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int nid=0,int npad=0);
	virtual ~BoxSelector();
	virtual const char *whattype() { return "BoxSelector"; }
	virtual void sync();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
//	virtual void WrapToExtent();
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual void Refresh();
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d); // toggle curbox
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);

	virtual int SelectN(int whichindex);
	virtual int Select(int whichID);
	virtual int MouseInWhich(int x,int y);
	virtual void drawbox(int which) = 0;
	virtual int send();
};

} // namespace Laxkit

#endif

