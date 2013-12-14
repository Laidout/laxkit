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
#ifndef _LAX_ROWFRAME_H
#define _LAX_ROWFRAME_H

#include <lax/anxapp.h>
#include <lax/boxarrange.h>
#include <lax/winframebox.h>


#define ROWFRAME_COLUMNS     (1<<16)
#define ROWFRAME_VERTICAL    (1<<16)
#define ROWFRAME_ROWS        (1<<17)
#define ROWFRAME_HORIZONTAL  (1<<17)

#define ROWFRAME_CENTER      (1<<19|1<<22)
#define ROWFRAME_LEFT        (1<<18)
#define ROWFRAME_HCENTER     (1<<19)
#define ROWFRAME_RIGHT       (1<<20)
#define ROWFRAME_TOP         (1<<21)
#define ROWFRAME_VCENTER     (1<<22)
#define ROWFRAME_BOTTOM      (1<<23)

 // these work with COLUMNS/ROWS/HORIZONTAL/VERTICAL to determine
 // flags&BOX_FLOW_MASK
#define ROWFRAME_ROWS_TO_TOP    (0<<24)
#define ROWFRAME_ROWS_TO_BOTTOM (1<<24)
#define ROWFRAME_COLS_TO_LEFT   (0<<24)
#define ROWFRAME_COLS_TO_RIGHT  (1<<24)
#define ROWFRAME_ROWS_LR        (0<<25)
#define ROWFRAME_ROWS_RL        (1<<25)
#define ROWFRAME_COLS_TB        (0<<25)
#define ROWFRAME_COLS_BT        (1<<25)

 // how to fill gaps between boxes
 // these refer to filling whole window
#define ROWFRAME_STRETCH     (1<<27|1<<28)
#define ROWFRAME_STRETCHX    (1<<27)
#define ROWFRAME_STRETCHY    (1<<28)
#define ROWFRAME_SPACE       (1<<29|1<<30)
#define ROWFRAME_SPACEX      (1<<29)
#define ROWFRAME_SPACEY      (1<<30)

 // these refer to filling extra (width in cols) or (height in rows)
#define ROWFRAME_STRETCH_IN_ROW (1<<31)
#define ROWFRAME_STRETCH_IN_COL (1<<31)

namespace Laxkit {

////----------------------------------------- RowFrame ------------------------------------
class RowFrame : public anXWindow, public RowColBox
{ 
 protected:
 public:	
	unsigned long highlight,shadow,mobkcolor,bkcolor;
	RowFrame(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long owner,const char *mes,
						int npad=0);
	virtual ~RowFrame();
	virtual int init();
	virtual int Sync(int add=0); // add=0, if 1 means addwindow
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int findWindowIndex(const char *name);
	virtual anXWindow *findWindow(const char *name);
	virtual int AddNull(int where=-1);
	virtual int AddHSpacer(int npw,int nws,int nwg,int nhalign, int where=-1);
	virtual int AddVSpacer(int npw,int nws,int nwg,int nhalign, int where=-1);
	virtual int AddSpacer(int npw,int nws,int nwg,int nhalign,
						  int nph,int nhs,int nhg,int nvalign,
						  int where=-1);
	virtual int AddWin(WinFrameBox *box,char islocal=1,int where=-1);
	virtual int AddWin(anXWindow *win,int absorbcount,int where); // adds with what is w/h in window, no stretch
	virtual int AddWin(anXWindow *win,int absorbcount,
					int npw,int nws,int nwg,int nhalign,int nhgap,
					int nph,int nhs,int nhg,int nvalign,int nvgap,
					int where);
	virtual void Refresh();
};

} // namespace Laxkit

#endif

