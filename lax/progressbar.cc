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


#include <lax/progressbar.h>
#include <lax/laxutils.h>

#include <cmath>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

//------------------------------

/*! \class ProgressBar
 * \brief Where would we be without the ability to measure progress?
 *
 * This is your basic, almost-no frills progress bar. The choice is a bar
 * of color that goes left to right, or something like an Eye of Sauron progress
 * bar, where as whatever progresses, it is displayed as a sort of vertical iris 
 * constricting. 
 *
 * \todo *** not currently implemented are the options to have Cancel and Pause/Resume
 *   buttons with the actual bar.. Might not even implement this ever!
 * \todo *** have optional label and call function which would check just for pending
 *   map/unmap events, then Refresh().
 * \todo need to add some kind of threaded mechanism to make this actually useful!!
 * 
 * \code
 * #define PROGRESS_CANCEL (1<<16)
 * #define PROGRESS_PAUSE  (1<<17)
 * #define PROGRESS_OVAL   (1<<18)      <-- this is the style for the iris
 * \endcode
*/ 
/*! \var long ProgressBar::progress
 * \brief How far along the whatever is.
 * 
 * This can be any value from and including 0 to and including max.
*/
/*! \var long ProgressBar::max
 * \brief This is the maximum value that progress can attain.
 *
 * The variable progress goes from 0 to this value.
 */


//! Constructor. Nothing cuckoo.
/*! Merely sets the background color field in win_xatts, and progress and max to 100.
 * 
 * \todo ***should add bounds, colors to the constructor! 
 */
ProgressBar::ProgressBar(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder)
	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder, NULL,0,NULL)
{
	installColors(app->color_panel);

	needtodraw=1;
	oldprogress=0;
	progress=max=100;
}

//! Set the progress and also the maximum value (if nmax>0).
/*! p would normally have the range [0,max]. */
int ProgressBar::Set(long p,long nmax) // max==0
{
	progress=p;
	if (nmax>0) max=nmax;
	needtodraw|=2;
	return 0;
}

//! Called from Refresh.
/*! Derived classes would redefine this class for an alternate style
 *  of progress. This one draws either a plain color bar for progress,
 *  or else a kind of closing oval if (win_style&PROGRESS_OVAL).
 */
void ProgressBar::Drawbar()
{
	if (win_w<4 || win_h<4 || win_w>1600 || win_h>1600) return;
	DBG cerr <<"progress bar "<<WindowTitle()<<": "<<win_w<<','<<win_h<<endl;

	if (win_style&PROGRESS_OVAL) {
		int start=oldprogress,w;
		if (needtodraw&1) { // drawall
			DBG cerr <<"  progressbar draw all";
			start=0;
			clear_window(this);
		}
		long c,end=progress;
		if (end<start) {
			c=end;
			end=start;
			start=c;
		}
		for (c=start; c<=end; c++) {
			w=(int)(win_w/2*c/(float)max);
			foreground_color(coloravg(win_colors->bg,win_colors->fg,c/(float)max));
			draw_arc_wh(this, w,0, win_w-2*w,win_h, 0,2*M_PI);
		}
		//a
		//DBG cerr <<<<"drawing"<<endl;
		w=(int)(win_w/2*progress/(float)max);
		foreground_color(win_colors->bg);
		fill_arc_wh(this, w,0, win_w-2*w,win_h, 0,2*M_PI);
		foreground_color(coloravg(win_colors->bg,win_colors->fg,progress/(float)max));
		draw_arc_wh(this, w,0, win_w-2*w,win_h, 0,2*M_PI);

	} else {
		foreground_color(win_colors->fg);
		fill_rectangle(this, 0,0,(int)((float) progress/(float)max * win_w),win_h);
		foreground_color(win_colors->bg);
		fill_rectangle(this, (int)((float) progress/(float)max * win_w),0,  
				win_w-(int)((float) progress/(float)max * win_w) , win_h);
	}
}

void ProgressBar::Refresh()
{
	if (!win_on || !needtodraw || !max) return;
	MakeCurrent();
	Drawbar();
	needtodraw=0;
}

} // namespace Laxkit

