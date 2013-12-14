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
//    Copyright (C) 2004-2006,2010 by Tom Lechner
//
#ifndef _LAX_SCROLLEDWINDOW_H
#define _LAX_SCROLLEDWINDOW_H

#include <lax/scroller.h>
#include <lax/pancontroller.h>
#include <lax/panpopup.h>
#include <lax/intrectangle.h>

 // Put an x scroller on either top or bottom
#define SW_TOP         (1<<16)
#define SW_BOTTOM      (1<<17)
 // Put a y scroller on either left or right
#define SW_LEFT        (1<<18)
#define SW_RIGHT       (1<<19)
 // inlude a PanWindow in corner between the scrollers
#define SW_INCLUDE_PAN (1<<20)
 // Always have x or y scrollers, they do not go away when not needed.
#define SW_ALWAYS_X    (1<<21)
#define SW_ALWAYS_Y    (1<<22)
 // Allow scrolling in the x or y direction
#define SW_X_ZOOMABLE  (1<<23)
#define SW_Y_ZOOMABLE  (1<<24)
 // When zooming, preserve the aspect ratio of the boxed area.
#define SW_SYNC_XY     (1<<25)
 // Move around a subwindow in response to scroll events,
 // Otherwise default is to simply relay pan changes to thewindow.
#define SW_MOVE_WINDOW (1<<26)

namespace Laxkit {

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
	Scroller *xscroller,*yscroller;
	ScrolledWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner=0,const char *nsend=NULL,
		PanController *pan=NULL);
	virtual int init();
	virtual int send();
	virtual void syncWindows(int useinrect=0);
	virtual int UseThisWindow(anXWindow *nwindow);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int Event(const EventData *e,const char *mes);
};

} // namespace Laxkit

#endif

