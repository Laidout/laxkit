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


#include <lax/winframebox.h>

namespace Laxkit {

//--------------------------- WinFrameBox -----------------------------------


/*! \class WinFrameBox
 * \brief Extends SquishyBox to store anXWindow for use in RowFrame, for instance.
 *
 * nwin's count is increased when adding, and decreased in WinFrameBox's destructor.
 */


//! This will inc count of nwin (if not NULL).
WinFrameBox::WinFrameBox(anXWindow *nwin,
			int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
			int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap)
	: SquishyBox(0, nx,nw,npw,nws,nwg,nhalign,nhgap,  ny,nh,nph,nhs,nhg,nvalign,nvgap)
{
	window = nwin;
	if (window) {
		last_scale = window->UIScale();
		window->inc_count();
	}
}

/*! Dec count of window.
 */
WinFrameBox::~WinFrameBox()
{
	if (window) window->dec_count();
}

void WinFrameBox::NewWindow(anXWindow *nwin)
{
	if (nwin!=window) {
		if (window) window->dec_count();
		window=nwin;
		if (window) window->inc_count();
	}
}


#define LAX_MAX_WINDOW_SIZE 10000


//! Resize the window to have the current settings for x,y,w,h.
/*! The SquishyBox x,y,w,h should have been set already, this
 * function just resizes the window to those values.
 */
void WinFrameBox::sync()
{
	//DBG if (window) cerr <<"-=-=-=-==< WFresize<sync "<<window->WindowTitle()<<": ";
	//DBG else cerr <<"-=-=-=-==< WFresize NULL"<<": ";
	//DBG cerr <<m[0]<<','<<m[6]<<"  "<<m[1]<<"x"<<m[7]<<" pref:("<<m[2]<<','<<m[3]<<','
	//DBG 	<<m[4]<<" x "<<m[8]<<','<<m[9]<<','<<m[10]<<")"<<endl;
	
	SquishyBox::sync();
	
	//DBG if (window) cerr <<"-=-=-=-==< WFresize>sync "<<window->WindowTitle()<<": ";
	//DBG else cerr <<"-=-=-=-==< WFresize NULL"<<": ";
	//DBG cerr <<m[0]<<','<<m[6]<<"  "<<m[1]<<"x"<<m[7]<<" pref:("<<m[2]<<','<<m[3]<<','
	//DBG 	<<m[4]<<" x "<<m[8]<<','<<m[9]<<','<<m[10]<<")"<<endl;

	if (!window) return;

	// some basic sanity checking, clamps w,h, not pref w,h
	if (w()>LAX_MAX_WINDOW_SIZE) w(LAX_MAX_WINDOW_SIZE);
	else if (w()<0) w(0);
	if (h()>LAX_MAX_WINDOW_SIZE) h(LAX_MAX_WINDOW_SIZE);
	else if (h()<0) h(0);

	window->MoveResize(x() - window->WindowBorder(), y()-window->WindowBorder(), w(),h());
}

int WinFrameBox::hideBox(int yeshide)
{
	bool old = hidden();
	int status = SquishyBox::hideBox(yeshide);
	if (window) {
		if (old) anXApp::app->unmapwindow(window);
		else anXApp::app->mapwindow(window);
	}
	return status;
}


} //namespace Laxkit

