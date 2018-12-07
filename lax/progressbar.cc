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
	InstallColors(THEME_Panel);

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

	Displayer *dp=GetDisplayer();

	if (win_style&PROGRESS_OVAL) {
		int start=oldprogress,w;
		if (needtodraw&1) { // drawall
			DBG cerr <<"  progressbar draw all";
			start=0;
			dp->ClearWindow();
		}
		long c,end=progress;
		if (end<start) {
			c=end;
			end=start;
			start=c;
		}
		for (c=start; c<=end; c++) {
			w=(int)(win_w/2*c/(float)max);
			dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,c/(float)max));
			dp->drawellipseWH(w,0, win_w-2*w,win_h, 0,2*M_PI, 0);
		}
		//a
		//DBG cerr <<<<"drawing"<<endl;
		w=(int)(win_w/2*progress/(float)max);
		dp->NewFG(win_themestyle->bg);
		dp->drawellipseWH(w,0, win_w-2*w,win_h, 0,2*M_PI, 1);
		dp->NewFG(coloravg(win_themestyle->bg, win_themestyle->fg, progress/(float)max));
		dp->drawellipseWH(w,0, win_w-2*w,win_h, 0,2*M_PI, 0);

	} else {
		dp->NewFG(win_themestyle->fg);
		dp->drawrectangle(0,0,(int)((float) progress/(float)max * win_w),win_h, 1);
		dp->NewFG(win_themestyle->bg);
		dp->drawrectangle((int)((float) progress/(float)max * win_w),0,  
				win_w-(int)((float) progress/(float)max * win_w) , win_h, 1);
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

