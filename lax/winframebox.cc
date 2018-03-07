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
 * It is always assumed that nwin is reference counted, thus its count is increased when adding,
 * and decreased in the destructor.
 */


//! This will inc count of nwin (if not NULL).
WinFrameBox::WinFrameBox(anXWindow *nwin,
			int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
			int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap)
	: SquishyBox(0, nx,nw,npw,nws,nwg,nhalign,nhgap,  ny,nh,nph,nhs,nhg,nvalign,nvgap)
{
	//flags=0; ???that ok?
	window=nwin;
	if (window) window->inc_count();
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

	 // some basic sanity checking, make windows smaller than 2000, clamps w,h, not pref w,h
	if (w()>2000) w(2000);
	else if (w()<0) w(0);
	if (h()>2000) h(2000);
	else if (h()<0) h(0);

	//**** something messed up here: not doing border right*** not sub from width??
	//is this still true?
	window->MoveResize(x()-window->win_border,y()-window->win_border, w(),h());
}


} //namespace Laxkit

