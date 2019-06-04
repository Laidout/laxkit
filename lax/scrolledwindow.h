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
//    Copyright (C) 2004-2006,2010,2019 by Tom Lechner
//
#ifndef _LAX_SCROLLEDWINDOW_H
#define _LAX_SCROLLEDWINDOW_H

#include <lax/scroller.h>
#include <lax/pancontroller.h>
#include <lax/panpopup.h>
#include <lax/rectangles.h>


namespace Laxkit {


enum ScrolledWindowStyles {
	 // Put an x scroller on either top or bottom
	SW_TOP         = (1<<16),
	SW_BOTTOM      = (1<<17),
	 // Put a y scroller on either left or right
	SW_LEFT        = (1<<18),
	SW_RIGHT       = (1<<19),
	 // inlude a PanWindow in corner between the scrollers
	SW_INCLUDE_PAN = (1<<20),
	 // Always have x or y scrollers, they do not go away when not needed.
	SW_ALWAYS_X    = (1<<21),
	SW_ALWAYS_Y    = (1<<22),
	 // Allow scrolling in the x or y direction
	SW_X_ZOOMABLE  = (1<<23),
	SW_Y_ZOOMABLE  = (1<<24),
	 // When zooming, preserve the aspect ratio of the boxed area.
	SW_SYNC_XY     = (1<<25),
	 // Move around a subwindow in response to scroll events,
	 // Otherwise default is to simply relay pan changes to thewindow.
	SW_MOVE_WINDOW = (1<<26),
};


class ScrolledWindow : public PanUser, public anXWindow
{
 protected:
	virtual void findoutrect();
	virtual void adjustinrect();

 public:
	IntRectangle inrect,outrect;
	int scrollwidth;
	PanPopup *panpopup;
	anXWindow *thewindow;
	Scroller *xscroller, *yscroller;

	ScrolledWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner=0,const char *nsend = NULL,
		PanController *pan = NULL);
	virtual int init();
	virtual int send();
	virtual void syncWindows();
	virtual int UseThisWindow(anXWindow *nwindow);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int Event(const EventData *e,const char *mes);
};

} // namespace Laxkit

#endif

