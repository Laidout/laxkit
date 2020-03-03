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
//    Copyright (C) 2019 by Tom Lechner
//
#ifndef _LAX_FOLDOUTWINDOW_H
#define _LAX_FOLDOUTWINDOW_H

#include <lax/scroller.h>
#include <lax/pancontroller.h>
#include <lax/panpopup.h>
#include <lax/rectangles.h>
#include <lax/boxarrange.h>


namespace Laxkit {


//enum FoldoutWindowStyles {
//};


class FoldoutWindow : public anXWindow, public SquishyBox
{
  protected:
	anXWindow *thewindow;
	bool expanded;
	double pad;
	double labelheight;
	bool needtosync;
	bool lbdown;
	char *label;

	//virtual void findoutrect();
	//virtual void adjustinrect();

  public:
	IntRectangle inrect,outrect;

	FoldoutWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner=0,const char *nsend = NULL,
		const char *label, anXWindow *subwindow = nullptr);
	virtual ~FoldoutWindow();
	virtual int init();
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int Event(const EventData *e,const char *mes);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
    virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
    virtual void Refresh();


	virtual void Expand();
	virtual void Collapse();
	virtual int send();
	virtual void SyncWindows();
	virtual int UseThisWindow(anXWindow *nwindow);
};

} // namespace Laxkit

#endif

