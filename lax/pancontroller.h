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
//    Copyright (C) 2004-2010,2012 by Tom Lechner
//
#ifndef _LAX_PAN_CONROLLER_H
#define _LAX_PAN_CONROLLER_H

#include <lax/anobject.h>
#include <lax/anxapp.h>

namespace Laxkit {

 // Setting this lets the whole space to be smaller than the selbox. Otherwise
 // no area of the selbox is allowed to not be a part of the whole space.
#define PANC_ALLOW_SMALL         (1<<0)
 // If the whole space is smaller than the selection rectangle, then
 // selecting CENTER_SMALL causes the space to always be centered within the selbox, rather
 // than allowing the space to be shifted around the selbox.
#define PANC_CENTER_SMALL        (1<<1)
 // Whether the pagesize or element size is absolute in workspace coordinates,
 // or if they are a percent of (start-end).
#define PANC_PAGE_IS_PERCENT     (1<<2)
#define PANC_ELEMENT_IS_PERCENT  (1<<3)
 // whether x and y should scale in a synchronized way
#define PANC_SYNC_XY             (1<<4)

class PanController : virtual public anObject
{
 protected:
	PtrStack<anXWindow> tellstack;
 public:
	anXWindow *donttell;
	unsigned int pan_style;
	int sendstatus;
	long minsel[2],maxsel[2],min[2],max[2],start[2],end[2]; // workspace info, 0=x 1=y
	long pagesize[2],elementsize[2];
	int boxaspect[2];
	double pixelaspect;  // =pixw/pixh

	PanController();
	PanController(const PanController &pan);
	PanController& operator=(PanController &pan);
	PanController(long xmin,long xmax,long xstart,long xend,
				  long ymin,long ymax,long ystart,long yend,
				  int w,int h,unsigned long panstyle=PANC_ELEMENT_IS_PERCENT|PANC_PAGE_IS_PERCENT);
	virtual ~PanController();
	virtual long SetPageSize(int which,long nps);
	virtual long GetPageSize(int which);
	virtual double GetMagToBox(int which,int boxwidth,long *wholestartret,long *wholeendret);
	virtual double GetMagToWhole(int which,int trackwidth,long *boxstartret,long *boxendret);
	virtual void SetSelBounds(int which, long small,long big);
	virtual void SetPixelAspect(double npixaspect=1.0);
	virtual double findpixelaspect();
	virtual void SetBoxAspect(int w,int h);
	virtual int SetWholebox(long xmin,long xmax,long ymin,long ymax);
	virtual int SetStuff(int which,long nmin,long nmax,long nps,long nes,long posstart,long posend);
	virtual int SetSize(int which, long nmin,long nmax,long nps);
	virtual long GetCurPos(int which,long *curpos=NULL,long *curposend=NULL);
	virtual long SetCurPos(int which,long pos);
	virtual long SetCurPos(int which,long poss,long pose);
	virtual int validateSelbox(int which=3);
	virtual int adjustSelbox(int which=2,char validatetoo=1);

	virtual int Center(int which=3);
	virtual long Shift(int which,long d, long wholelen=0, long boxlen=0);
	virtual int ShiftEnd(int which,long d,int center=0,long wholelen=0,long boxlen=0);
	virtual int ShiftStart(int which,long ds,int center=0,long wholelen=0,long boxlen=0);

	virtual long OneUp(int which);
	virtual long OneDown(int which);
	virtual long PageUp(int which,int numpages=1);
	virtual long PageDown(int which,int numpages=1);

	virtual void sendMessages();
	virtual void tell(anXWindow *win);
	virtual void tellPop(anXWindow *win=NULL);
	virtual void dontTell(anXWindow *win);
};

} // namespace Laxkit

#endif

