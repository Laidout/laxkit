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
//    Copyright (C) 2009-2010 by Tom Lechner
//

#ifndef _LAX_WINFRAMEBOX_H
#define _LAX_WINFRAMEBOX_H

#include <lax/anxapp.h>
#include <lax/boxarrange.h>


namespace Laxkit {

////----------------------------------------- WinFrameBox ------------------------------------
class WinFrameBox : public SquishyBox
{
 protected:
	anXWindow *window;
 public:
	WinFrameBox(anXWindow *nwin,
			int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
			int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap);
	WinFrameBox() { window=NULL; }
	virtual ~WinFrameBox();
	virtual void NewWindow(anXWindow *nwin);
	virtual anXWindow *win() { return window; }
	virtual void sync();
	virtual int hideBox(int yeshide);
};


} //namespace Laxkit

#endif

