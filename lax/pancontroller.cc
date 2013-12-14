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


#include <lax/pancontroller.h>
#include <lax/panuser.h>

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

/*! \class PanController
 * \brief A convenience for scroller types of things.
 *
 * This class keeps track of one rectangle (herein referred to as the selbox)
 * sliding around in (or around) another rectangle (hereafter referred to as the wholebox). 
 * Also can handle zooming. It is used, for instance, by 
 * Scroller, particularly when the Scroller is one of a pair, and they both
 * have zoom handles.
 *
 * This class is a little comparable to the Adjustment widget of Gtkmm, except
 * that it is 2 dimensional, and handles zooming.
 * 
 * The rectangle that is the bounds of workspace runs x: [min[0],max[0]] and 
 * y: [min[1],max[1]]. The amount supposedly viewable is [start,end] within
 * that range. Internally, the positions and bounds are stored as type long.
 *
 * \todo Get/set pagesize, clean up the Get/Set to be more like fltk (overloaded functions?)
 * \todo *** must work out when messages are actually sent, by which functions.. don't want
 *   to unnecessarily duplicate messages.\n
 * \todo *** please note that panuser/pancontroller setup is hell with threads..
 *
 * pan_style holds:
 * \code
 *  // Setting this lets the whole space to be smaller than the selbox. Otherwise
 *  // no area of the selbox is allowed to not be a part of the whole space.
 * #define PANC_ALLOW_SMALL         (1<<0)
 *  // If the whole space is smaller than the selection rectangle, then
 *  // selecting CENTER_SMALL causes the space to always be centered within the selbox, rather
 *  // than allowing the space to be shifted around the selbox.
 * #define PANC_CENTER_SMALL        (1<<1)
 *  // Whether the pagesize or element size is absolute in workspace coordinates,
 *  // or if they are a percent of (start-end).
 * #define PANC_PAGE_IS_PERCENT     (1<<2)
 * #define PANC_ELEMENT_IS_PERCENT  (1<<3)
 *  // whether x and y should scale in a synchronized way
 * #define PANC_SYNC_XY             (1<<4)
 * \endcode
 */
/*! \var unsigned int PanController::pan_style
 * \brief Style of the controller.
 */
/*! \var double PanController::pixelaspect
 * \brief The relative scaling between x and y: (pixel width/pixel height).
 * 
 * boxaspect = pixelaspect * (selboxwidth/selboxheight)
 */
/*! \var int PanController::boxaspect[2]
 * \brief Basically the aspect of the window corresponding to the selection box.
 *
 * This is stored so that zooming by dragging control handles in one dimension causes
 * the proper changes in the other dimension.
 */
/*! \var long PanController::maxsel[2]
 * \brief The maximum allowable selection which == end-start+1.
 */
/*! \var long PanController::minsel[2]
 * \brief The minimum allowable selection which == end-start+1.
 */
/*! \var long PanController::min[2]
 * \brief The minimum coordinate of the space (min[0],min[1]).
 */
/*! \var long PanController::max[2]
 * \brief The maximum coordinate of the space (max[0],max[1]).
 */
/*! \var long PanController::start[2]
 * \brief The minimum coordinate of the selection box (start[0],start[1]).
 */
/*! \var long PanController::end[2]
 * \brief The maximum coordinate of the selection box (end[0],end[1]).
 */
/*! \var long PanController::pagesize[2]
 * \brief The pagesize used when calling PageUp/Down.
 *
 * If PANC_PAGE_IS_PERCENT is set in pan_style, then pagesize is that percent (100=100%)
 * of the selbox size. Otherwise, pagesize is that absolute number of whole space units.
 */
/*! \var long PanController::elementsize[2]
 * \brief The element used when calling OneUp/Down.
 *
 * If PANC_ELEMENT_IS_PERCENT is set in pan_style, then elementsize is that percent (100=100%)
 * of the selbox size. Otherwise, elementsize is that absolute number of whole space units.
 */
/*! \fn PtrStack<anXWindow> PanController::tellstack
 * \brief Stack of windows to notify when settings change.
 *
 * If the windows in tellstack can be cast to PanUser, then proper deleting procedures
 * will occur. That is, when either a window or a panner is deleted, the relevant members
 * in the other are reset.
 */

//! Default constructor, sets minsel to 1, maxsel to 1000000, and style to PANC_ELEMENT_IS_PERCENT|PANC_PAGE_IS_PERCENT.
PanController::PanController()
{
	donttell=NULL;
	sendstatus=1;
	for (int c=0; c<2; c++) {
		min[c]=max[c]=start[c]=end[c]=0;
		pagesize[c]=90;
		elementsize[c]=1;
		minsel[c]=1;
		maxsel[c]=1000000;
	}
	pixelaspect=1.0;
	boxaspect[0]=boxaspect[1]=100;
	pan_style=PANC_ELEMENT_IS_PERCENT|PANC_PAGE_IS_PERCENT;
}

//! Copy constructor. Does not copy tellstack.
PanController::PanController(const PanController &pan)
{
	donttell=NULL;
	sendstatus=1;
	pan_style=pan.pan_style;
	pixelaspect=pan.pixelaspect;
	for (int c=0; c<2; c++) {
		boxaspect[c]=pan.boxaspect[c];
		pagesize[c]=pan.pagesize[c];
		elementsize[c]=pan.elementsize[c];
		minsel[c]=pan.minsel[c];
		maxsel[c]=pan.maxsel[c];
		min[c]=pan.min[c];
		max[c]=pan.max[c];
		start[c]=pan.start[c];
		end[c]=pan.end[c];
	}
}

//! Assignment operator does NOT copy the tellstack. Leaves tellstack as is.
PanController& PanController::operator=(PanController &pan)
{
	sendstatus=1;
	pan_style=pan.pan_style;
	pixelaspect=pan.pixelaspect;
	for (int c=0; c<2; c++) {
		boxaspect[c]=pan.boxaspect[c];
		pagesize[c]=pan.pagesize[c];
		elementsize[c]=pan.elementsize[c];
		minsel[c]=pan.minsel[c];
		maxsel[c]=pan.maxsel[c];
		min[c]=pan.min[c];
		max[c]=pan.max[c];
		start[c]=pan.start[c];
		end[c]=pan.end[c];
	}
	return pan;
}

//! Constructor
/*! \todo *** does no sanity checking yet....
 */
PanController::PanController(long xmin,long xmax,long xstart,long xend,
							 long ymin,long ymax,long ystart,long yend,
							 int w,int h,unsigned long panstyle)
{
	donttell=NULL;
	sendstatus=1;
	
	pan_style=panstyle;
	pagesize[0]=pagesize[1]=90;
	elementsize[0]=elementsize[1]=10;
	
	min[0]=xmin;
	max[0]=xmax;
	start[0]=xstart;
	end[0]=xend;

	min[1]=ymin;
	max[1]=ymax;
	start[1]=ystart;
	end[1]=yend;
	
	 // default minsel/maxsel is 0 and max-min
	minsel[0]=minsel[1]=1;
	maxsel[0]=max[0]-min[0]+1;
	maxsel[1]=max[1]-min[1]+1;
	
	boxaspect[0]=end[0]-start[0]+1;
	boxaspect[1]=end[1]-start[1]+1;
	pixelaspect=1.0;
}

//! Destructor, turns off the panner in any PanUser in tellstack.
/*! This is done by calling UseThisPanner(NULL) on anything that
 * can be cast to PanUser. This prevents any PanUser classes from calling
 * a panner that has just been destroyed. This of course assumes that
 * the panner is destroyed before those windows!! Otherwise the pointers
 * in the tellstack may not point to valid values.
 *
 * IMPORTANT: For this destructing mechanism to work, in your class definition of
 * the PanUser-derived class,
 * you MUST declare the PanUser BEFORE the window part, otherwise the PanUser
 * will not pop itself from this tellstack during its destructor.
 *
 * The tellstack is not flushed here. It flushes in the natural automatic way.
 */
PanController::~PanController()
{
	DBG cerr <<"in PanController destructor: tellstack.n:"<<tellstack.n<<endl;
//	for (int c=0; c<tellstack.n; c++) {
//		if (dynamic_cast<PanUser *>(tellstack.e[c]))
//			dynamic_cast<PanUser *>(tellstack.e[c])->UseThisPanner(NULL);
//	}
}

//! Push win onto the stack of windows to notify of changes.
void PanController::tell(anXWindow *win)
{
	DBG int c;
	if (win)
		DBG c=
		tellstack.pushnodup(win,0);
	DBG cerr <<" ---TELL---"<<win->WindowTitle(1)<<"  "<<c<<endl;
}

//! Pop win from the tellstack. If win==NULL, then flush the tellstack.
/*! If the window is already on the stack, it is not pushed again.
 */
void PanController::tellPop(anXWindow *win)
{
	if (win==NULL) tellstack.flush();
	else tellstack.popp(win);
	DBG cerr <<" ---TELLPOP---"<<win->WindowTitle(1)<<endl;
}

//! Exhempt win from getting messages sent to it, until dontTell(NULL) or dontTell(someotherwindow) is called.
/*! There can only be one exhemption at a time.
 */
void PanController::dontTell(anXWindow *win)
{
	donttell=win;
}

//! Send messages to all who want to know what's what with the pan.
/*! If sendstatus!=1, then no messages are sent.
 *
 * The message is filled thus:
 * <pre>
 * 	info1=start[0];
 * 	info2=end[0];
 * 	info3=start[1];
 * 	info4=end[1];
 * </pre>
 *
 * \todo send indication of what has changed? 1=x move change, 2=move ychange, 3=x selsize change, x wholesizechange, etc...
 */
void PanController::sendMessages()
{
	if (!tellstack.n || sendstatus!=1) return;
	DBG cerr <<"----- In sendMessages to "<<tellstack.n<<" windows.."<<endl;

	SimpleMessage *data=NULL;
	for (int c=0; c<tellstack.n; c++) {
		if (tellstack.e[c]==NULL) { tellstack.pop(c); c--; continue; } // cover for sloppy programmers
		if (tellstack.e[c]==donttell) continue;

		data=new SimpleMessage(NULL, start[0], end[0], start[1], end[1]);
		
		//app->SendMessage(tellstack.e[c],"pan change"); <-- should send a normal ClientMessage
		anXApp::app->SendMessage(data, tellstack.e[c]->object_id, "pan change", 0);
	}
}

//! Set so that end-start+1==nps. Returns new actual end-start+1;
long PanController::SetPageSize(int which,long nps)
{
	if (which!=1 && which!=2) return 0;
	which--;
	end[which]=start[which]+nps-1;
	validateSelbox();
	return end[which]-start[which]+1;
}

//! Return end[which]-start[which]+1;
long PanController::GetPageSize(int which)
{
	if (which!=1 && which!=2) return 0;
	which--;
	return end[which]-start[which]+1;
}

//! Return the start of x (which==1) or y (which==2) dimension.
/*! If curpos or curposend are not NULL, then the start and end are put there.
 */
long PanController::GetCurPos(int which,long *curpos,long *curposend)//curpos=curposend=NULL
{
	if (which!=1 && which!=2) return 0;
	which--;
	if (curpos) *curpos=start[which];
	if (curposend) *curposend=end[which];
	return start[which];
}

//! Get the horizontal (which==1) or vertical(=2) magnification of the box to the screen (screen=Getmag*real)
/*! Use this when you have a screen size boxwidth that must correspond
 * to the selection box. If wholestart/endret are not NULL, then return what would
 * be the whole space start and end if viewed in screen coordinates. The origin
 * is taken to be the start of the transformed box (screen coords).
 *
 * If boxwidth<=0, then use end-start+1 as boxwidth.
 * 
 * Note that end-start+1 must != 0 or else your program will crash!
 */
double PanController::GetMagToBox(int which,int boxwidth,long *wholestartret,long *wholeendret)
{
	if (which!=1 && which!=2) return 0;
	which--;
	if (boxwidth<=0) boxwidth=end[which]-start[which]+1;
	double mag=(double)boxwidth/(end[which]-start[which]+1);
	if (wholestartret) *wholestartret=(long)(mag*(min[which]-start[which])+.5);
	if (wholeendret)   *wholeendret=  (long)(mag*(max[which]-start[which])+.5);
	return mag;
}

//! Get the horizontal (which==1) or vertical(=2) magnification of the whole space to the screen (screen=Getmag*real)
/*! Use this when you have a screen size trackwidth that must correspond
 * to the whole space. If boxstart/endret are not NULL, then return what would
 * be the selection box start and end if viewed in screen coordinates. Coordinates
 * are scaled so that the whole space maps to the range [0,trackwidth].
 *
 * If trackwidth<=0, then use max-min+1 as trackwidth.
 * 
 * This is akin to getting the placement of a track box in a scroller window.
 */
double PanController::GetMagToWhole(int which,int trackwidth,long *boxstartret,long *boxendret)
{
	if (which!=1 && which!=2) return 0;
	which--;
	if (trackwidth<=0) trackwidth=max[which]-min[which]+1;
	double mag=(double)trackwidth/(max[which]-min[which]+1);
	if (boxstartret) *boxstartret=(long)(mag*(start[which]-min[which])+.5);
	if (boxendret)   *boxendret=  (long)(mag*(end[which]  -min[which])+.5);
	return mag;
}

//! Compute the new pixelaspect based on other info.....
/*! boxaspect = pixelaspect * (selboxwidth/selboxheight)
 * 
 * If max[0]-min[0]==0 then return 1.0, else the new pixelaspect.
 */
double PanController::findpixelaspect()
{
	if (end[0]-start[0]+1==0) return 1.0;
	return pixelaspect=((double)boxaspect[0]/boxaspect[1] * (end[1]-start[1]+1)) / (end[0]-start[0]+1);
}

//! Set the level of difference between x and y zoom.
/*! Nothing is changed if npixaspect is <=0.
 *
 * Suppose you want pixels to be twice as wide as tall, then pixelaspect=2.
 * Basically, the selbox width/height is equal to 
 * boxaspect/pixelaspect.
 * 
 * boxaspect = pixelaspect * (selboxwidth/selboxheight)
 */
void PanController::SetPixelAspect(double npixaspect)
{
	if (npixaspect<=0 || npixaspect==pixelaspect) return;
	pixelaspect=npixaspect;
	DBG cerr <<" == New pixelaspect: "<<pixelaspect<<endl;
	adjustSelbox(0,1);
	sendMessages();
}

//! Set the preferred dimensions of a target window.
/*! Any zooming done tries to preserve the aspect w/h. These values
 * basically would correspond to a window's width and height. This is separate
 * from pixelaspect so as to preserve the zooming ratio between x and y while
 * adjusting the actual shape of the selbox.
 *
 * Basically, the selbox width/height is equal to 
 * boxaspect/pixelaspect.
 */
void PanController::SetBoxAspect(int w,int h)
{
	if (w<=0 || h<=0) return;
	double ww=((double)(end[0]-start[0]+1))/boxaspect[0]; // ww=wholeSpaceUnits/boxaspectUnits
	boxaspect[0]=w;
	boxaspect[1]=h;
	end[0]=(long)(start[0]-1+ww*w+.5);
	adjustSelbox(1,1);
	sendMessages();
}

//! Reconfigure the selbox based on boxaspect and pixelaspect.
/*! If which is nonzero, then try to keep which's metrics. That is to say,
 * the selbox in which dimension stays the same, and others is adjusted to match it.
 * Default is to keep y. It is assumed that the start/end for which are
 * already valid.
 *
 * Internally, this is called by SetBoxAspect and SetPixelAspect.
 */
int PanController::adjustSelbox(int which,char validatetoo)//validatetoo=1
{
	if (boxaspect[0]==0 || boxaspect[1]==0) return 0;
	if (which!=1 && which!=2) which=2;
	which--;
	int other=(which==1?0:1);
	
	 // boxaspectw/h = pixelaspect * (selboxwidth/selboxheight)
	 // selboxwidth= boxaspect/pixelaspect * selboxheight
	double bs=(pixelaspect*boxaspect[1])/boxaspect[0];
	long dimother=(long)((end[which]-start[which]+1) * (which==0?bs:1/bs) + .5);
	long diff=(end[other]-start[other]+1 - dimother)/2;

	 // so now which has its preferred setup, and we must convert start[other] and end[other]
	 // to acceptible values.
	start[other]+=diff;
	end[other]=start[other]+dimother-1;
	int c=0;
	if (validatetoo) c=validateSelbox(other+1);
	return c|(other+1);
}

//! Make sure the selbox is completely in wholebox or vice versa, as necessary.
/*! Returns 1 if x needed to be adjusted, 2 if y, 3 if both.
 * Note that if PANC_ALLOW_SMALL is not set, then the selbox has to be contained by the wholebox, 
 * and the selbox is scaled to as necessary. Otherwise, then either selbox is in whole or whole is in selbox is ok.
 * If both PANC_ALLOW_SMALL and PANC_CENTER_SMALL are set, then whenever the selbox is larger than
 * the wholebox, then the wholebox is centered in the selbox.
 *
 * This assumes the selbox is already ok according to boxaspect and pixelaspect.
 *
 * Internally, this function is called by SetCurPos, SetWholebox, and adjustSelbox, and indirectly
 * (through adjustSelbox) from SetPixelAspect and SetBoxAspect.
 *
 * Should return which dim had to be adjusted. *** must check that..
 */
int PanController::validateSelbox(int which)//which=3
{
	//DBG cerr <<"   validate.. keep "<<which<<endl;
	int changed=0;
	int other;
	for (int c=0; c<2; c++) {
		if (!((c+1)&which)) continue; // only check which
		other=(c==0?1:0);

		 // handle selbox>wholebox
		if (pan_style&PANC_ALLOW_SMALL && end[c]-start[c]>max[c]-min[c]) {
			if (pan_style&PANC_CENTER_SMALL) { changed|=Center(c+1); continue; }
			if (start[c]<min[c] && end[c]<max[c]) { // move end to max
				start[c]+=max[c]-end[c];
				end[c]=max[c];
				changed|=c+1;
				continue;
			}
			if (start[c]>min[c] && end[c]>max[c]) { // move start to min
				end[c]+=start[c]-min[c];
				start[c]=min[c];
				changed|=c+1;
				continue;
			}
			continue;
		}
		 // handle selbox<wholebox
		if (start[c]<min[c]) {
			//DBG cerr <<"pancontroller--- start past min"<<endl;
			
			changed|=c+1;
			end[c]+=min[c]-start[c];
			start[c]=min[c];
			if (end[c]>max[c]) { // if in here, then zooming is required
				end[c]=max[c];
				//DBG cerr <<"PanController::validate "<<c<<"  must shrink other="<<other<<"!!"<<endl;
				
				adjustSelbox(other,0); // tries to preserve which, shrink, do not validate to prevent race!
			}
		}
		//DBG cerr <<"pancontroller--- end past min"<<endl;
		if (end[c]>max[c]) {
			changed|=c+1;
			start[c]-=end[c]-max[c];
			end[c]=max[c];
			if (start[c]<min[c]) { // if in here, then zooming is required
				start[c]=min[c];
				//DBG cerr <<"PanController::validate "<<c<<" must shrink other="<<other<<"!!"<<endl;

				adjustSelbox(other,0); // tries to preserve which, shrink, do not validate to prevent race!
			}
		}
	}
	return changed;
}

//! Make the whole and sel centers coincide. 1=x, 2=y, 0=neither, other=both
/*! The changes are made to start and end, not to min and max.
 *
 * Returns which had to be changed.
 */
int PanController::Center(int which)
{
	if (which==0) return 0;
	if (which<1 || which>2) which=3;
	if (which&1) {
		long w=end[0]-start[0];
		if (start[0]==(max[0]+min[0]-w)/2) which&=~1;
		else {
			start[0]=(max[0]+min[0]-w)/2;
			end[0]  =(max[0]+min[0]+w)/2+w%2;
		}
	}
	if (which&2) {
		long w=end[1]-start[1];
		if (start[1]==(max[1]+min[1]-w)/2) which&=~2;
		else {
			start[1]=(max[1]+min[1]-w)/2;
			end[1]  =(max[1]+min[1]+w)/2+w%2;
		}
	}
	sendMessages();
	return which;
}

//! Shift the selbox x or y (if which==1 or 2) sel box by d units coords where whole maps to max-min+1.
/*! If wholelen==0, then d is in space units.
 * 
 * If the wholebox is larger than the selbox, then the selbox can move freely around within the wholebox.
 * If the wholebox is smaller than the selbox, then the wholebox can move freely around within the selbox,
 * as long as PANC_ALLOW_SMALL is set. Then, if PANC_CENTER_SMALL is set, then the whole box is always
 * centered within the selbox. Thus one box can fully contain the other box. Partial containment is not
 * allowed. *** must check for this in the other functions!!
 * 
 * Returns the amount of change (newpos-oldpos).
 */
long PanController::Shift(int which, long d, long wholelen, long boxlen) //wholelen=boxlen=0
{
	if (which!=1 && which!=2) return 0;
	which--;
	if (d) {
		if (wholelen) { // convert d to wholespace, where wholelen corresponds to wholespace
			int c=d>0?1:-1;
			d= d * (max[which]-min[which]+1) / wholelen;
			if (d==0) d=c;
		} else if (boxlen) { // convert d, where wholelen corresponds to the selbox
			int c=d>0?1:-1;
			d= d * (end[which]-start[which]+1) / boxlen;
			if (d==0) d=c;
		}
		if (d>0) {
			 // PANC_ALLOW_SMALL means the wholebox can be less than selbox
			 // normally selbox is less than wholebox
			if (end[which]-start[which]<max[which]-min[which]) { // whole larger than selbox
				if (end[which]+d>max[which]) d=max[which]-end[which];
			} else { // selbox is larger than wholebox, assume PANC_ALLOW_SMALL is set
				if (pan_style&PANC_CENTER_SMALL) d=0; // should already be in its proper place
				else if (start[which]+d>min[which]) d=min[which]-start[which];
			}
		} else {
			if (end[which]-start[which]<max[which]-min[which]) { // whole larger than selbox
				if (start[which]+d<min[which]) d=min[which]-start[which];
			} else { // selbox is larger than wholebox, assume PANC_ALLOW_SMALL is set
				if (pan_style&PANC_CENTER_SMALL) d=0; // should already be in its proper place
				else if (end[which]+d<max[which]) d=max[which]-end[which];
			}
		}
		start[which]+=d;
		end[which]+=d;
	}
	if (d) sendMessages();
	return d;
}

//! Shift the screen sel box end by de units. wholelen maps to whole space (max-min+1).
/*! This basically zooms. Say which=1, de is nonzero. Then x is scaled
 * up (or down) proportionally. The y start
 * and end are changed so that after the expansion (or contraction) of
 * the y selection box, its center doesn't move (if possible). If 
 * PANC_SYNC_XY is not set, then y start and end are not affected by changes
 * in x, and vice versa.
 *
 * This action is equivalent to dragging a zoom handle in a Scroller. If center is nonzero,
 * then move the start point the same amount the end point is moved.
 * 
 * Return 1 if x info changes, 2 if y changes, 3 if both change.
 */
int PanController::ShiftEnd(int which,long de,int center,long wholelen,long boxlen) //wholelen=boxlen=0,center=0
{
	if ((which!=1 && which!=2) || de==0) return 0;
	which--;

	int c,ds=0;
	if (de) {
		if (wholelen) {
			c=de>0?1:-1;
			de= de * (max[which]-min[which]) / wholelen;
			if (de==0) de=c;
		} else if (boxlen) { // convert d, where wholelen corresponds to the selbox
			int c=de>0?1:-1;
			de= de * (end[which]-start[which]+1) / boxlen;
			if (de==0) de=c;
		}
		 // adjust de to be within the minsel and maxsel
		if (de>0) {
			if (end[which]+de-start[which]>maxsel[which]) de=maxsel[which]-end[which]+start[which];
		} else {
			if (end[which]+de-start[which]<minsel[which]) de=minsel[which]-end[which]+start[which];
		}
		if (center) { // this causes a change in start only if there is a change in end
			if (de>0) {
				if (end[which]+de+ds-start[which]>maxsel[which]) ds=(maxsel[which]-end[which]+start[which])-de;
				else ds=de;
			} else if (de<0) {
				if (end[which]+de+ds-start[which]<minsel[which]) ds=(minsel[which]-end[which]+start[which])-de;
				else ds=de;
			} 
		}
	}
	 // at this point, de is adjusted so end-start stays within [minsel,maxsel]
	 // if there is an actual change, adjust it, clamp to ALLOW_SMALL settings, 
	 // and adjust the other dimension if necessary.
	if (de) {
		end[which]+=de;
		start[which]-=ds;
		 // now clamp to proper ALLOW_SMALL settings..
		if (!(pan_style&PANC_ALLOW_SMALL)) { // selbox must be contained by wholebox
			if (end[which]>max[which]) end[which]=max[which];
			if (start[which]<min[which]) start[which]=min[which];
		} else if (pan_style&PANC_CENTER_SMALL && max[which]-min[which]<end[which]-start[which]) {
			 // selbox can be larger than wholebox here. if it is, then center whole within sel.
			Center(which);
		}
	}

	c=0;
	if ((pan_style&PANC_SYNC_XY) && (de || ds)) { // fix the other dimension
		c=adjustSelbox(which+1,1); // tries to preserve which
	}
	if (!(pan_style&PANC_SYNC_XY)) findpixelaspect();//***
	c|=validateSelbox();
	if (ds || de) sendMessages();
	return c|(ds||de?which+1:0);
}

//! Shift the screen sel box start by de units. wholelen maps to whole space (max-min+1).
/*! This basically zooms. Say which=1, de is nonzero. Then x is scaled
 * up (or down) proportionally. The y start
 * and end are changed so that after the expansion (or contraction) of
 * the y selection box, its center doesn't move (if possible). If 
 * PANC_SYNC_XY is not set, then y start and end are not affected by changes
 * in x, and vice versa.
 *
 * This action is equivalent to dragging a zoom handle in a Scroller. If center is nonzero,
 * then move the start point the same amount the end point is moved.
 * 
 * Return 1 if x info changes, 2 if y changes, 3 if both change.
 */
int PanController::ShiftStart(int which,long ds,int center,long wholelen,long boxlen) //wholelen=boxlen=center=0
{
	if ((which!=1 && which!=2) || ds==0) return 0;
	which--;

	int c,de=0;
	if (ds) {
		if (wholelen) { // adjust ds to wholespace units
			c=ds>0?1:-1;
			ds= ds * (max[which]-min[which]) / wholelen;
			if (ds==0) ds=c;
		} else if (boxlen) { // convert d, where wholelen corresponds to the selbox
			int c=ds>0?1:-1;
			ds= ds * (end[which]-start[which]+1) / boxlen;
			if (ds==0) ds=c;
		}
		 // adjust ds to be within the minsel and maxsel
		if (ds>0) {
			if (end[which]-(start[which]+ds)<minsel[which]) ds=end[which]-start[which]-minsel[which];
		} else {
			if (end[which]-(start[which]+ds)>maxsel[which]) ds=end[which]-start[which]-maxsel[which];
		}
		if (center) { // change end only if start changes
			if (ds>0) {
				if (end[which]-(start[which]+ds+de)<minsel[which]) de=end[which]-start[which]-minsel[which]-ds;
				else de=ds;
			} else {
				if (end[which]-(start[which]+ds+de)>maxsel[which]) de=end[which]-start[which]-maxsel[which]-ds;
				else de=ds;
			}
		}
	}
	if (ds) {
		start[which]+=ds;
		end[which]-=de;
		 // now clamp to proper ALLOW_SMALL settings..
		if (!(pan_style&PANC_ALLOW_SMALL)) { // selbox must be contained by wholebox
			if (end[which]>max[which]) end[which]=max[which];
			if (start[which]<min[which]) start[which]=min[which];
		} else if (pan_style&PANC_CENTER_SMALL && max[which]-min[which]<end[which]-start[which]) {
			 // selbox can be larger than wholebox here. if it is, then center whole within sel.
			long w=end[which]-start[which];
			start[which]=(max[which]+min[which]-w)/2;
			end[which]  =(max[which]+min[which]+w)/2+w%2;
		}
	}
		
	c=0;
	if ((pan_style&PANC_SYNC_XY) && (de || ds)) { // fix the other dimension
		c=adjustSelbox(which+1,1);
	}
	if (!(pan_style&PANC_SYNC_XY)) findpixelaspect();//*** this right?
	c|=validateSelbox();
	if (ds || de) sendMessages();
	return c|(ds||de?which+1:0);
}


//! Move selbox one element size in the negative direction.
/*! Returns the amount of change (new start-old start).
 */
long PanController::OneUp(int which)
{
	if (which!=1 && which!=2) return 0;
	which--;
	long move;
	if (pan_style&PANC_ELEMENT_IS_PERCENT) move=elementsize[which]*(start[which]-end[which])/100;
	else move=-elementsize[which];
	return Shift(which+1,move);
}

//! Move selbox one element size in the positive direction.
/*! Returns the amount of change (new start-old start).
 */
long PanController::OneDown(int which)
{
	if (which!=1 && which!=2) return 0;
	which--;
	long move;
	if (pan_style&PANC_ELEMENT_IS_PERCENT) move=elementsize[which]*(end[which]-start[which])/100;
	else move=elementsize[which];
	return Shift(which+1,move);
}

//! Move selbox one page size in the negative direction.
/*! Returns the amount of change (new start-old start).
 */
long PanController::PageUp(int which,int numpages)
{
	if (which!=1 && which!=2) return 0;
	which--;
	long move;
	if (pan_style&PANC_ELEMENT_IS_PERCENT) move=pagesize[which]*(start[which]-end[which])/100;
	else move=-pagesize[which];
	return Shift(which+1,move*numpages);
}

//! Move selbox one page size in the positive direction.
/*! Returns the amount of change (new start-old start).
 */
long PanController::PageDown(int which,int numpages)
{
	if (which!=1 && which!=2) return 0;
	which--;
	long move;
	if (pan_style&PANC_ELEMENT_IS_PERCENT) move=pagesize[which]*(end[which]-start[which])/100;
	else move=pagesize[which];
	return Shift(which+1,move*numpages);
}

//! Set the start. Shortcut for calling Shift(pos-start).
/*! Returns the new pos minus the old pos.
 */
long PanController::SetCurPos(int which,long pos)
{
	if ((which!=1 && which!=2) || start[which-1]==pos) return 0;
	which--;
	return Shift(which,pos-start[which]);
}

//! Set the start and the end.
/*! currently just sets start/end, calls validateSelbox
 *
 *  Returns the new pos minus the old pos.
 */
long PanController::SetCurPos(int which,long poss,long pose)
{
	if ((which!=1 && which!=2) || pose<poss || (start[which-1]==poss && end[which-1]==pose)) return 0;
	int old=start[which];
	which--;
	start[which]=poss;
	end[which]=pose;
	validateSelbox();
	return start[which]-old;
}

//! Set the min/max, and adjust start,end based on nps.
/*! nps is used to set things so that end-start+1==nps. The pagesize[]
 * is not modified, same goes for nes. start is changed only if necessary
 * to accomodate nps.
 *
 * Does not try to synchronize the other dimension.
 *
 * Returns which on success.
 */
int PanController::SetSize(int which, long nmin,long nmax,long nps)
{
	if (which!=1 && which!=2) return 0;
	which--;
	
	min[which]=nmin;
	max[which]=nmax;
	end[which]=start[which]+nps;
	if (!(pan_style&PANC_ALLOW_SMALL)) {
		if (end[which]>nmax) {
			end[which]=nmax;
			start[which]=nmax-nps+1;
			if (start[which]<nmin) start[which]=nmin;
		}
	} else {
		if (pan_style&PANC_CENTER_SMALL && nmax-nmin<end[which]-start[which]) Center(which);
	}
	sendMessages();
	return which+1;
}

//! Set the minimum selection size and maximum selection size.
/*! \todo make sure the current selbox doesn't exceed bounds.
 */
void PanController::SetSelBounds(int which, long small,long big)
{
	if ((which!=1 && which!=2) || small<0 || big<small) return;
	which--;
	
	minsel[which]=small;
	maxsel[which]=big;
}

//! Set the metrics of the space of one of the dimensions: 1==x, 2==y.
/*! If PANC_PAGESIZE_IS_PERCENT, then nps is a percent, not absolute coordinates.
 * and same goes for nes.
 *
 * If posstart<nmin, then make posstart=nmin. Same for posend.
 * If posend>nmax then make posend=nmax, same for posstart.
 * if nps or nes are <=0, then do not change them.
 * 
 * This also sets boxaspect to be end-start+1.
 * 
 * Return 1 if stuff was set, else 0.
 */
int PanController::SetStuff(int which,long nmin,long nmax,long nps,long nes,long posstart,long posend)
{
	if ((which!=1 && which!=2) || nmax<nmin || posend<posstart) return 0;
	which--;
	
	if (posstart<nmin) posstart=nmin;
	if (posend<nmin) posend=nmin;
	if (posstart>nmax) posstart=nmax;
	if (posend>nmax) posend=nmax;

	min[which]=nmin;
	max[which]=nmax;
	start[which]=posstart;
	end[which]=posend;

	boxaspect[which]=end[which]-start[which]+1;
	
	if (maxsel[which]<=minsel[which]) maxsel[which]=max[which]-min[which];

	if (nps>0) pagesize[which]=nps;
	if (nes>0) elementsize[which]=nes;
	
	sendMessages();
	return 1;
}

//! Set both the dimensions of the workspace, preserving proportionally the selbox.
/*! Returns the value of validateSelbox, which should indicate whether
 * some other values had to be changed.
 */
int PanController::SetWholebox(long xmin,long xmax,long ymin,long ymax)
{
	if (max[0]-min[0]) {
		double s=(start[0]-min[0])/(max[0]-min[0]), 
			   e=(end[0]-min[0])/(max[0]-min[0]);
		start[0]=(long)(s*(xmax-xmin)+xmin+.5);
		end[0]=  (long)(e*(xmax-xmin)+xmin+.5);
	}
	
	if (max[1]-min[1]) {
		double s=(start[1]-min[1])/(max[1]-min[1]),
			   e=(end[1]-min[1])/(max[1]-min[1]);
		start[1]=(long)(s*(ymax-ymin)+ymin+.5);
		end[1]=  (long)(e*(ymax-ymin)+ymin+.5);
	}
	
	min[0]=xmin;
	max[0]=xmax;
	min[1]=ymin;
	max[1]=ymax;
	DBG cerr <<"=======Panner::SetWholebox: x:"<<min[0]<<','<<max[0]<<"  y:"<<min[1]<<','<<max[1]<<endl;
	int c=validateSelbox(); 
	sendMessages();
	return c;
}


} // namespace Laxkit


