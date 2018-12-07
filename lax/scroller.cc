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
//    Copyright (C) 2004-2010 by Tom Lechner
//






#include <lax/scroller.h>
#include <lax/laxutils.h>

#include <cmath>
#include <unistd.h>


#include <iostream>
using namespace std;
#define DBG 


using namespace LaxFiles;

namespace Laxkit {

/*! \class Scroller
 * 
 * \brief A vertical or horizontal scroll bar, with optional zooming handles, and togglable arrow positions.
 *
 * \todo *** all this assumes max>min, should allow min>max?? flip(pos) { return max-(pos-min); }
 * \todo *** watch out for ALLOW_SMALL
 *
 *  This uses a PanController to keep track of the space being scrolled through. This is particularly
 *  convenient when the scroller is part of a pair, and they have zoom handles, as the PanController
 *  helps to synchronize them.
 * 
 *  Control-left-click or typing any of '.' ',' '<' or '>' toggles the arrow style,
 *  which can be one arrow at each end (SC_ASPLIT), both arrows on top or left (SC_ATOP), or both 
 *  arrows at the bottom or right (SC_ABOTTOM). Or one can have no arrows at all (SC_NOARROWS).
 *  
 *  When there is a change in the scroll bar handle position, a SimpleMessage is filled thus:
 * <pre>
 * 	mevent->info1 = amount of change, basically curpos-oldcurpos;
 * 	mevent->info2 = start position;
 * 	mevent->info3 = end position; 
 * 	mevent->info4 = pagesizechange; <-- 1 if zoom handle was changed
 * </pre>
 *
 *  Style Defines:
 * \code
 * #define SC_XSCROLL             (1<<16)
 * #define SC_YSCROLL             (1<<17)
 * #define SC_ASPLIT              (1<<18)
 * #define SC_ATOP                (1<<19)
 * #define SC_ABOTTOM             (1<<20)
 * #define SC_NOARROWS            (1<<21)
 * #define SC_ZOOM                (1<<22)
 * #define SC_PAGE_IS_PERCENT     (1<<23)
 * #define SC_ELEMENT_IS_PERCENT  (1<<24)
 * #define SC_ALLOW_SMALL         (1<<25) <--- ***not imp yet
 * \endcode
 * <pre>
 * |--- a   
 * | ^
 * |--- b 
 * | |  c  (c-a)= toff, where track box begins
 * | |
 * | z  c  
 * | z  d (d-c)+1= zh, the height of a zoom handle
 * | +  e  
 * | +
 * | m  ]<-- minboxlen is the minimum size of the trackbox (not including zoom handles)
 * | m  ]
 * | +
 * | +    
 * | z  f (g-f)+1= zh
 * | z  g (g-c+1)=  the track box height
 * | |
 * | |
 * | |
 * |--- h  (h-c)= th, the height or length of the track
 * | v     
 * |--- i
 * 
 *  bw,bh is the width,height of track box, xscroll or yscroll
 *  
 *  </pre>
 *
 *  \todo implement 2 point zooming by dragging tracker with 2 devices
 */
/*! \var int Scroller::a1off
 * \brief The offset from the left (or top) to the first arrow.
 */
/*! \var int Scroller::a2off
 * \brief The offset from the left (or top) to the second arrow.
 */
/*! \var int Scroller::toff
 * \brief The offset from the left (or top) to the track.
 */
/*! \var int Scroller::zh
 * \brief The height of an individual zoom handle.
 */
/*! \var int Scroller::ah
 * \brief The height of an individual arrow.
 */


//! Constructor. Creates a new panner if npan==NULL.
/*! If npan is not NULL, then all the parameters after npan are ignored, so it's a good
 * thing the constructor is overloaded. 
 */
Scroller::Scroller(anXWindow *parnt,const char *nname, const char *ntitle, unsigned long nwstyle,
				int nx,int ny,int nw,int nh,int brder,
				anXWindow *prev, unsigned long nowner,const char *mes,
				PanController *npan,
				long nmins, //!< Minimum value
				long nmaxs, //!< Maximum value
				long nps,   //!< Page size
				long nes,  //!< Element size
				long ncp,  //!< Curpos
				long ncpe) //!< Curpos end. Use -1 if you do not need an actual range for the curpos.
	: PanUser(npan),
	  anXWindow(parnt,nname,ntitle,nwstyle,nx,ny,nw,nh,brder,prev,nowner,mes)
	  
{
	DBG cerr <<"scroller win_style:"<<win_style<<"  nwstyle:"<<nwstyle<<endl;

//	numticks=app->firstclk;
//	DBG cerr<<"scroller-numticks="<<numticks<<endl;

	tid=0;
		
	zh=0;
	minboxlen=5;
	redoarrows(); // this sets a12off, toff, ah and zh if necessary
	if (ncp<nmins) ncp=nmins;
	if (ncpe<ncp) ncpe=ncp+nes;
	if (ncpe>nmaxs) ncpe=nmaxs;

	//installColors(app->color_panel);
	InstallColors(THEME_Panel);

	if (npan == NULL) { // pan has just been created, must put the given values into it.
		panner->SetStuff(win_style&SC_XSCROLL?1:2,nmins,nmaxs,nps,nes,ncp,ncpe);
	}
}

//! Empty default destructor.
Scroller::~Scroller()
{}


//int Scroller::ThemeChange(Theme *theme)
//{
//	installColors(theme->GetStyle(THEME_Panel));
//	return 0;
//}


//------------------- interface elements:

//! Control-, and control-. toggle the arrow style.
/*! \todo ***: somehow there should be a mechanism to not grab
 *    the focus even if the mouse is clicked here.
 */
int Scroller::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if ((ch==',' || ch=='.' || ch=='<' || ch=='>')) {
		if (win_style&SC_ATOP)
			win_style=(win_style&~(SC_ATOP|SC_ABOTTOM|SC_ASPLIT))|SC_ABOTTOM;
		else if (win_style&SC_ABOTTOM)
			win_style=(win_style&~(SC_ATOP|SC_ABOTTOM|SC_ASPLIT))|SC_ASPLIT;
		else win_style=(win_style&~(SC_ATOP|SC_ABOTTOM|SC_ASPLIT))|SC_ATOP;
		redoarrows();
		return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state,d);
}

//! Shift the bar numpages down.
/*! This does not send an event. It is assumed that the application calls this,
 *  and already knows that it is calling and so does not need to be notified of changes.
 *
 *  Returns amount of change.
 */
long Scroller::PageUp(int numpages) 
{
	long m;
	if (m=panner->PageDown(win_style&SC_XSCROLL?1:2, numpages), m) {
		needtodraw|=2;
		return m;
	}
	return 0;
}

//! Shift the bar numpages up.
/*! This does not send an event. It is assumed that the application calls this,
 *  and already knows that it is calling and so does not need to be notified of changes.
 *
 *  Returns amount of change.
 */
long Scroller::PageDown(int numpages) 
{
	long m;
	if (m=panner->PageUp(win_style&SC_XSCROLL?1:2, numpages), m) {
		needtodraw|=2;
		return m;
	}
	return 0;
}

//! Move the track down one element unit.
/*!  Returns amount of change.
 */
long Scroller::OneUp()
{
	long m;
	if (m=panner->OneDown(win_style&SC_XSCROLL?1:2), m) { needtodraw|=2; return m; }
	return 0;
}

//! Move the track up one unit
/*!  Returns 1 if movement occurs.
 */
long Scroller::OneDown()
{
	long m;
	if (m=panner->OneUp(win_style&SC_XSCROLL?1:2), m) { needtodraw|=2; return m; }
	return 0;
}

#define UP_ARROW     1
#define DOWN_ARROW   2
#define TRACKER      3
#define PAGE_UP      4
#define PAGE_DOWN    5
#define ZOOM_UP      6
#define ZOOM_DOWN    7

//! Return which of the various elements the mouse is in.
/*! <pre>
 *  purpose here is to find where the mouse was when pressed
 *  1 left/up arrow
 *  2 right/down arrow
 *  3 on tracker
 *  4 pgleft/up
 *  5 pgright/down
 *  6 left/up zoom handle
 *  7 right/down zoom handle
 * </pre>
 */
int Scroller::getpos(int mx,int my)
{
	if (win_style&SC_XSCROLL) {
		if (my<0 || my>win_h) return 0;
		my=mx;
	} else { // is vertical (y) scroller
		if (mx<0 || mx>win_w) return 0;
	}
	long poss,pose;
	int zzh=(win_style&SC_ZOOM?zh:0);
	int wholelen=(win_style&SC_XSCROLL?win_w:win_h)-2*ah-2*zzh-minboxlen;
	
	panner->GetMagToWhole((win_style&SC_XSCROLL?1:2), wholelen, &poss,&pose);
	if (poss<0 || poss>wholelen || pose<0 || pose>wholelen) {
		panner->GetMagToBox((win_style&SC_XSCROLL?1:2), wholelen,&poss,&pose);
		wholelen=0;
	}
	 //toff is the start of the track
	poss+=toff+zzh; // start of box after first zoom handle
	pose+=toff+zzh+minboxlen; // end of box, before final zoom handle
	
	if      (my>=a1off && my<a1off+ah) return UP_ARROW;   // up 
	else if (my>=a2off && my<a2off+ah) return DOWN_ARROW; // down
	else if (my<poss-zh) return PAGE_UP;    // pgup 
	else if (my>pose+zh) return PAGE_DOWN; // pgdown
	else if (my<poss) return ZOOM_UP;     // upper zoom handle
	else if (my>pose) return ZOOM_DOWN;  // lower zoom handle
	else return 3;	// on tracker
}

//! Right clicking on the arrows causes pageup/down at the next idle or on RBUp
int Scroller::RBDown(int mx,int my,unsigned int state,int count,const LaxMouse *d)
{
	idlemx=mx; idlemy=my;
	int rbdown=getpos(mx,my);
	if (rbdown!=0) {
		buttondown.down(d->id, RIGHTBUTTON, rbdown,(win_style&SC_XSCROLL?mx:my));
		//if (!tid) tid=app->addmousetimer(this);
	}
	return 0;
}

//! Right clicking on the arrows causes pageup/down
/*! Control-Right click changes the arrow styles.
 */
int Scroller::RBUp(int mx,int my, unsigned int state,const LaxMouse *d)
{
	if (tid && !buttondown.any()) { app->removetimer(this,tid); tid=0; }
	if (!buttondown.isdown(d->id,RIGHTBUTTON)) { return 0; }

	int rbdown;
	if (buttondown.getinfo(d->id,RIGHTBUTTON, &rbdown,NULL, NULL,NULL,NULL,NULL)!=0) return 0;
	buttondown.up(d->id,RIGHTBUTTON);
	if (tid && !buttondown.any()) { app->removetimer(this,tid); tid=0; }

	int rbend=getpos(mx,my);
	//DBG cerr <<"rbd:"<<rbdown<<" rbe:"<<rbend;
	int change=0;
	if (rbdown==UP_ARROW && rbend==UP_ARROW) {	 		  // pgup
		change=PageUp(state&ShiftMask?5:1);
	} else if (rbdown==DOWN_ARROW && rbend==DOWN_ARROW) { // pgdown 
		change=PageDown(state&ShiftMask?5:1);
	}
	if (change) {
		needtodraw|=2;
		send(change);
	}
	return 0;
}

//! LBDown
int Scroller::LBDown(int mx,int my,unsigned int state,int count,const LaxMouse *d)
{
	idlemx=mx; idlemy=my;
	int lbdown=getpos(mx,my);

	 //buttondown info1 gets which chuck clicked down on, and info2 is the mouse position
	buttondown.down(d->id, LEFTBUTTON, 0,(win_style&SC_XSCROLL?mx:my), lbdown, 0);

	DBG int i;
	DBG buttondown.getextrainfo(d->id,LEFTBUTTON,&i);
	DBG cerr <<"---Scroller lbdown: "<<lbdown<<"  buttondown i:"<<i<<endl;

	//if (!tid) tid=app->addmousetimer(this);
	return 0;
}

int Scroller::LBUp(int mx,int my,unsigned  int state,const LaxMouse *d)
{
	int lbdown;
	if (!buttondown.isdown(d->id,LEFTBUTTON)) {
		 //button is not logged as down
		if (tid && !buttondown.any()) { app->removetimer(this,tid); tid=0; }
		return 0;
	}

	int lbend=getpos(mx,my);
	buttondown.up(d->id,LEFTBUTTON,&lbdown,NULL);

	long change=0;
	if (lbdown==UP_ARROW && lbend==UP_ARROW) {    // up 
		change=OneUp();
	} else if (lbdown==DOWN_ARROW && lbend==DOWN_ARROW) {  // down
		change=OneDown();
	} else if (lbdown==PAGE_UP && lbend==PAGE_UP) {	 // pgup
		change=PageUp(state&ShiftMask?5:1);
	} else if (lbdown==PAGE_DOWN && lbend==PAGE_DOWN) {	   // pgdown
		change=PageDown(state&ShiftMask?5:1);
	} // else on tracker, or zoom handles

	if (change) {
		needtodraw|=2;
		send(change);
	}
	return 0;
}

//! Wheel up. Move 1 element, shift or control moves 1 page, shift+control moves 5 pages.
int Scroller::WheelUp(int mx,int my,unsigned  int state,int count,const LaxMouse *d)
{
	if (buttondown.any()) return 1;

	long change;
	if (state&ShiftMask && state&ControlMask) change=PageUp(5);
	else if (state&ShiftMask || state&ControlMask) change=PageUp(1); 
	else change=OneUp();
	if (change) { needtodraw|=2; send(change); }
	return 0;
}

//! Wheel down. Move 1 element, shift moves 1 page, shift+control moves 5 pages.
int Scroller::WheelDown(int mx,int my,unsigned  int state,int count,const LaxMouse *d)
{
	if (buttondown.any()) return 1;

	long change;
	if (state&ShiftMask && state&ControlMask) change=PageDown(5);
	else if (state&ShiftMask || state&ControlMask) change=PageDown(1); 
	else change=OneDown();
	if (change) { needtodraw|=2; send(change); }
	return 0;
}

//! Mouse Move.
/*!  This might not work as expected when selbox larger than wholebox... fancy zoom allows scaling
 * even though dragging causes cross over sel==whole boundary.
 */
int Scroller::MouseMove(int mx,int my,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.any(d->id)) return 0;

	idlemx=mx; idlemy=my;
	
	 // keep track of the mouse pos when there is and is not a change..
	 //if no change, do not update last pos, and do no more changes until mouse goes back down again..
	
	if (buttondown.isdown(d->id,LEFTBUTTON)==0) return 0; //return if no left buttons down

	int oldpos=0;
	int lbdown=0;
	int changed=0;
	buttondown.getinfo(d->id, LEFTBUTTON, NULL,NULL, NULL,NULL, &changed,&oldpos, &lbdown,NULL);
	int dpos=(win_style&SC_XSCROLL?mx:my)-oldpos; //change in position of the mouse

	DBG cerr<<"  scroller before lbdown="<<lbdown<<"  dpos="<<dpos<<"  changed="<<changed<<endl;

	if (dpos==0) return 0; //no movement

	 // changed<0 if last move was going up and nothing happened
	if (changed<0 && dpos<0) return 0;

	 // changed>0 if last move was going down and nothing happened
	if (changed>0 && dpos>0) return 0;

	long change=0;
	int psc=0;
	int boxlen=0,wholelen=(win_style&SC_XSCROLL?win_w:win_h)-2*ah-2*(win_style&SC_ZOOM?zh:0)-minboxlen;
	 // if selbox is larger than wholebox, then must swap wholelen for boxlen
	int which=(win_style&SC_XSCROLL?1:2)-1;
	if (panner->start[which]<panner->min[which] || panner->start[which]>panner->max[which]
		 || panner->end[which]<panner->min[which] || panner->end[which]>panner->max[which]) {

		DBG cerr <<"========= use boxlen, not wholelen"<<endl;
		boxlen=wholelen; 
		wholelen=0; 
		dpos=-dpos;
		changed=0;
	}
	which++;

	if (lbdown!=TRACKER && lbdown!=ZOOM_UP && lbdown!=ZOOM_DOWN) return 0;//return if not on the tracker, tracker zoom handles

	if (lbdown==TRACKER) { // on tracker
		change=panner->Shift(which, dpos, wholelen, boxlen);

	} else if (lbdown==ZOOM_UP || lbdown==ZOOM_DOWN) { // upper=6 lower=7
		long poss,pose;
		GetCurPos(&poss,&pose);
		if (lbdown==ZOOM_UP) { // on upper zoom handle
			 // remember that ShiftStart/End returns which dimension has changed, not a pixel length change!!
			panner->ShiftStart(which, dpos, state&ControlMask?0:1, wholelen, boxlen);
			change=panner->start[which-1]-poss;
		} else { // lower zoom handle
			panner->ShiftEnd(which, dpos, state&ControlMask?0:1, wholelen, boxlen);
			change=panner->end[which-1]-pose;
		}
		if (change) psc=1;
	}
	if (change) { 
		changed=0; 
		send(change,psc); 
		needtodraw|=2;
	} else if (dpos) { // mouse has moved, but nothing happened.
		changed=((dpos>0)?1:-1);
		if (wholelen==0) changed=-changed;
	}
	buttondown.move(d->id, changed,(win_style&SC_XSCROLL?mx:my));
	DBG cerr<<"  scroller after  lbdown="<<lbdown<<"  dpos="<<dpos<<"  changed="<<changed<<endl;
	return 0;
}

/*! \todo put in shift/control on idling too.. maybe have call to WheelUp/WheelDown
 *
 * \todo this does not properly distinguish between left and right clicks...
 */
int Scroller::Idle(int id, double delta) 
{
	DBG cerr <<"  scroller idle, buttondown="<<buttondown.any()<<"..."<<endl;

	if (id!=tid) { app->removetimer(this,tid); tid=0; return 0; }
	if (!buttondown.any()) { app->removetimer(this,tid); tid=0; return 0; }

	int lbi;
	int mouse=buttondown.whichdown(0);
	buttondown.getextrainfo(mouse,LEFTBUTTON,&lbi,NULL);

	DBG cerr <<"  scroller idle  mouse="<<mouse<<"  lbi="<<lbi<<endl;

	 // go through this rigamorale to find shift/control/mod mask
	unsigned int state=0;
	//mouseposition(NULL,NULL,NULL,&state,NULL);
	
	long change=0;
	
	if (lbi==UP_ARROW) {
		if (state&ShiftMask && state&ControlMask) change=PageUp(5);
		else if (state&ShiftMask || state&ControlMask) change=PageUp(1); 
		else change=OneUp();

	} else if (lbi==DOWN_ARROW) {
		if (state&ShiftMask && state&ControlMask) change=PageDown(5);
		else if (state&ShiftMask || state&ControlMask) change=PageDown(1); 
		else change=OneDown();

	} else if (lbi==PAGE_DOWN) {
		change=PageUp();

	} else if (lbi==PAGE_UP) {
		change=PageDown();

	} // else on tracker, or zoom handles, no idle action

	if (change) {
		needtodraw|=2;
		send(change);
	}
	return 0;
}

int Scroller::Event(const EventData *e,const char *mes)
{
	if (!strcmp(mes,"pan change")) { 
		DBG cerr <<"************Scroller got a pan change"<<endl;
		needtodraw|=1;
		return 0;
	}
	return anXWindow::Event(e,mes);
}

int Scroller::send()
{
	DBG cerr <<" -s-s-s-- scroller::send"<<endl;
	send(panner->GetCurPos(win_style&SC_XSCROLL?1:2));
	return 0;//*** what should this return?
}

//! Send a control message. If change==0, then nothing is sent.
/*! Fills a SimpleMessage thus:
 * <pre>
 *  info1 = amount of change, basically curpos-oldcurpos;
 * 	info2 = start position;
 * 	info3 = end position; 
 * 	info4 = pagesizechange; 
 * </pre>
 * pagesizechange should be nonzero if there has been some zooming action.
 */
void Scroller::send(long change,int pagesizechange) //pagesizechange=0
{
	if (!win_sendthis || !win_owner || !change) return;

	DBG cerr <<WindowTitle()<<" change="<<change<<endl;

	long poss,pose;
	panner->GetCurPos(win_style&SC_XSCROLL?1:2, &poss,&pose);

	SimpleMessage *data=new SimpleMessage(NULL, change, poss, pose, pagesizechange);
	app->SendMessage(data, win_owner, win_sendthis, object_id);
}

//------------------- refreshing:

//! Essentially clears the window, then drawtrack(), then drawtrackbox().
void Scroller::Refresh()
{
	if (!win_on || !needtodraw) return; 
	//DBG cerr <<"REFRESH "<<WindowTitle()<<endl;

	// blank out whole window
	Displayer *dp=MakeCurrent();
	dp->ClearWindow();
	dp->LineAttributes(1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
	dp->BlendMode(LAXOP_Over);

	drawtrack(); // draws the track including arrows
	drawtrackbox();

	needtodraw=0;
}

//! Draw the track, including arrows. Default is draw a line, then call drawarrows.
void Scroller::drawtrack()
{
	// draw the track line
	Displayer *dp = GetDisplayer();
	dp->NewFG(&win_themestyle->fg);
	if (win_style & SC_XSCROLL) dp->drawline(0,int(win_h/2)+.5, win_w,int(win_h/2)+.5);
	else dp->drawline(int(win_w/2)+.5,0, int(win_w/2)+.5,win_h);
	drawarrows();
}

//! Draw the arrows in their proper place based on a1off, a2off, and ah.
void Scroller::drawarrows()
{
	if (!ah) return;
	int x1,x2,y1,y2,r;
	DrawThingTypes t1,t2;
	if (win_style&SC_XSCROLL) {
		x1=a1off+ah/2; 
		y1=y2=win_h/2;
		x2=a2off+ah/2;
		t1=THING_Triangle_Left; t2=THING_Triangle_Right;
		r=(ah<win_h?ah:win_h)/2;
	} else {
		x1=x2=win_w/2; 
		y1=a1off+ah/2;
		y2=a2off+ah/2;
		t1=THING_Triangle_Up; t2=THING_Triangle_Down;
		r=(ah<win_w?ah:win_w)/2;
	}
	Displayer *dp = GetDisplayer();
	dp->drawthing(x1,y1,r,r, t1, win_themestyle->fg.Pixel(), win_themestyle->color1.Pixel());
	dp->drawthing(x2,y2,r,r, t2, win_themestyle->fg.Pixel(), win_themestyle->color1.Pixel());
}

//! Draw the track box including the zoom handles if required.
void Scroller::drawtrackbox()
{
	Displayer *dp = GetDisplayer();

	 // get placement of the track box
	long poss,pose;
	int zzh=(win_style&SC_ZOOM?zh:0);
	int wholelen=(win_style&SC_XSCROLL?win_w:win_h)-2*ah-2*zzh-minboxlen;
	panner->GetMagToWhole((win_style&SC_XSCROLL?1:2), wholelen, &poss,&pose);
			
	 //***must imp ALLOW SMALL
	if (poss<0 || poss>wholelen || pose<0 || pose>wholelen) {
		if (!(win_style&SC_ALLOW_SMALL)) {
			DBG cerr<<"*** must imp scroller allow small"<<endl;
			 // SMALL lets the wholespace be less than the selbox, in fact,
			 // the selbox can be anything in relation to the wholebox, including
			 // the case where the two boxes do not overlap!! should this be
			 // checked for in pancontroller???
			 // *** should have some way to deal with this, like drawing a little
			 // arrow to indicate the the selbox extends off.
			panner->GetMagToBox((win_style&SC_XSCROLL?1:2), wholelen,&poss,&pose);
			wholelen=0; //<-- used as a tag later to do special indication of the selbox
		} else {
			poss=0;
			pose=wholelen;
		}
	}
	
	poss+=toff+zzh; // start of box after first zoom handle
	pose+=toff+minboxlen+zzh; // end of box, before final zoom handle

	 // draw the box without zoom handles
	if (!(win_style&SC_ZOOM) || zzh==0) {
		if (win_style&SC_XSCROLL) {
			dp->NewFG(&win_themestyle->color1); 

			//fill_arc_wh(this, poss,0, (pose-poss),win_h-1, 0,2*M_PI);
			dp->drawellipseWH(poss,0, (pose-poss),win_h-1, 0,2*M_PI, 1);

			dp->NewFG(win_themestyle->fg); 
			//draw_arc_wh(this, poss,0, (pose-poss),win_h-1, 0,2*M_PI);
			dp->drawellipseWH(poss,0, (pose-poss),win_h-1, 0,2*M_PI, 0);
			if (!wholelen) // for selbox too big 
				//draw_arc_wh(this, poss+(pose-poss)/4,(win_h-1)/4, (pose-poss)/2,(win_h-1)/2, 0,2*M_PI);
				dp->drawellipseWH(poss+(pose-poss)/4,(win_h-1)/4, (pose-poss)/2,(win_h-1)/2, 0,2*M_PI, 0);
		} else {
			dp->NewFG(win_themestyle->color1); 
			//fill_arc_wh(this, 0,poss, win_w-1,(pose-poss), 0,2*M_PI);
			dp->drawellipseWH(0,poss, win_w-1,(pose-poss), 0,2*M_PI, 1);
			dp->NewFG(win_themestyle->fg); 
			//draw_arc_wh(this, 0,poss, win_w-1,(pose-poss), 0,2*M_PI);
			dp->drawellipseWH(0,poss, win_w-1,(pose-poss), 0,2*M_PI, 0);
			if (!wholelen) // for selbox larger than wholebox, draw another oval inside trackbox
				//draw_arc_wh(this, (win_w-1)/4,poss+(pose-poss)/4, (win_w-1)/2,(pose-poss)/2, 0,2*M_PI);
				dp->drawellipseWH((win_w-1)/4,poss+(pose-poss)/4, (win_w-1)/2,(pose-poss)/2, 0,2*M_PI, 0);
		}
	} else { // draw trackbox with zoom handles
		 // draw track box body
		int x,y,w,h;
		if (win_style&SC_XSCROLL) {
			x=poss; y=0; w=(pose-poss); h=win_h-1;
		} else {
			x=0; y=poss; w=win_w-1; h=(pose-poss);
		}

		 // draw zoom handles
		dp->NewFG(win_themestyle->color1); 
		 // draw filled
		if (win_style&SC_XSCROLL) {
			//fill_arc_wh(this, poss-zh,0, 2*zh-1,win_h-1, 0,0);
			//fill_arc_wh(this, pose-zh,0, 2*zh-1,win_h-1, 0,0);
			dp->drawellipseWH(poss-zh,0, 2*zh-1,win_h-1, 0,0, 1);
			dp->drawellipseWH(pose-zh,0, 2*zh-1,win_h-1, 0,0, 1);
		} else {
			//fill_arc_wh(this, 0,poss-zh, win_w-1,2*zh-1, 0,0);
			//fill_arc_wh(this, 0,pose-zh, win_w-1,2*zh-1, 0,0);
			dp->drawellipseWH(0,poss-zh, win_w-1,2*zh-1, 0,0, 1);
			dp->drawellipseWH(0,pose-zh, win_w-1,2*zh-1, 0,0, 1);
		}
		 // draw outline
		dp->NewFG(win_themestyle->fg); 
		if (win_style&SC_XSCROLL) {
			//draw_arc_wh(this, poss-zh,0, 2*zh-1,win_h-1, 0,0);
			//draw_arc_wh(this, pose-zh,0, 2*zh-1,win_h-1, 0,0);
			dp->drawellipseWH(poss-zh,0, 2*zh-1,win_h-1, 0,0, 0);
			dp->drawellipseWH(pose-zh,0, 2*zh-1,win_h-1, 0,0, 0);
		} else {
			//draw_arc_wh(this, 0,poss-zh, win_w-1,2*zh-1, 0,0);
			//draw_arc_wh(this, 0,pose-zh, win_w-1,2*zh-1, 0,0);
			dp->drawellipseWH(0,poss-zh, win_w-1,2*zh-1, 0,0, 0);
			dp->drawellipseWH(0,pose-zh, win_w-1,2*zh-1, 0,0, 0);
		}

		 // draw central rectangular part of the trackbox
		dp->NewFG(win_themestyle->color1); 
		dp->drawrectangle(x,y, w,h, 1);
		dp->NewFG(&win_themestyle->fg);
		dp->drawrectangle(x,y, w,h, 0);
		if (!wholelen) // for selbox larger than wholebox, draw another oval inside trackbox
			//draw_arc_wh(this, x+w/4,y+h/4, w/2,h/2, 0,2*M_PI);
			dp->drawellipseWH(x+w/4,y+h/4, w/2,h/2, 0,2*M_PI, 0);
		
	}
}

//------------------------ getting and setting:

//! Set lots of attributes
/*! nps and nes are interpreted however the panner interprets them. That is, whether
 * they are percent of the selbox or absolute space units.
 *
 * Returns whatever panner->SetStuff returns, currently just 1 if stuff was set else 0. 
 */
long Scroller::SetSize(long nmins, //!< New minimum number
						long nmaxs, //!< New maximum number
						long ncurpos, //!< New current position
						long ncurposend, //!< New current end position
						long nps, //!< New page size (could be percent, depends on panner settings)
						long nes //!< New element size (could be percent, depends on panner settings)
						)
{
	long c=panner->SetStuff(win_style&SC_XSCROLL?1:2, nmins,nmaxs,ncurpos,ncurposend,nps,nes);
	needtodraw|=2;
	return c;
}

//! Calling this one assumes you want definite pagesize, not pagesize=curposend-curpos.
/*! Returns curpos. */
long Scroller::SetSize(long nmins, //!< New minimum size
						long nmaxs, //!< New maximym size
						long nps //!< New size for curposend-curpos+1.
						)// nps=0
{
	long c=panner->SetSize(win_style&SC_XSCROLL?1:2, nmins,nmaxs,nps);
	needtodraw|=2;
	return c;
}

//! Sets current position and end position, assigning new page size. 
/*! Returns difference between new curpos and old curpos.
 */
long Scroller::SetCurPos(long start, long end)
{
	long c=panner->SetCurPos(win_style&SC_XSCROLL?1:2,start,end);
	needtodraw|=2;
	return c;
}

//! Sets current position, keeping the old page size. Returns actual new curpos.
/*! Returns difference between old and new pos.
 */
long Scroller::SetCurPos(long newcurpos)
{
	long c=panner->SetCurPos(win_style&SC_XSCROLL?1:2,newcurpos);
	needtodraw|=2;
	return c;
}

//! Sets pagesize (meaning curposend-curpos+1), returns actual new pagesize
long Scroller::SetPageSize(long nps)
{
	long ps=panner->SetPageSize(win_style&SC_XSCROLL?1:2,nps);
	needtodraw|=2;
	return ps;
}

//! Return end-start
long Scroller::GetPageSize()
{
	return panner->GetPageSize(win_style&SC_XSCROLL?1:2);
}

//! Just returns panner->GetCurPos(poss,pose);
long Scroller::GetCurPos(long *poss,long *pose) //poss=pose=NULL
{
	return panner->GetCurPos(win_style&SC_XSCROLL?1:2,poss,pose);
}

//! Returns the panner->end.
long Scroller::GetCurPosEnd()
{
	long e;
	panner->GetCurPos(win_style&SC_XSCROLL?1:2,NULL,&e);
	return e;
}

//! Sets ah,zh,toff based on a presumed changed in win_w/win_h
/*! zoom height is 2/3 of the window height (for x scrollers, for instance)
 */
void Scroller::redoarrows() // does arrow coords, toff
{
	if (win_style&SC_XSCROLL) {
		if (win_style&SC_NOARROWS) ah=0; else ah=win_h*4/5;
		if (win_style&SC_ZOOM) zh=win_h*2/3;
	} else {
		if (win_style&SC_NOARROWS) ah=0; else ah=win_w*4/5;
		if (win_style&SC_ZOOM) zh=win_w*2/3;
	}
	if (win_style&SC_ATOP) { 
		toff=2*ah;	
		a1off=0; 
		a2off=ah; 
	} else if (win_style&SC_ABOTTOM) { 
		toff=0; 
		a1off=((win_style&SC_XSCROLL)?win_w:win_h)-2*ah; 
		a2off=a1off+ah; 
	} else { 
		toff=ah; 
		a1off=0; 
		a2off=((win_style&SC_XSCROLL)?win_w:win_h)-ah; 
	}
	needtodraw|=2;
}

//! Calls anXWindow::Resize, then redoarrows.
int Scroller::Resize(int nw,int nh)
{ 
	anXWindow::Resize(nw,nh); // used for configure notify
	DBG cerr <<"New size for "<<WindowTitle()<<" w:"<<nw<<" h:"<<nh<<endl;
	redoarrows();
	needtodraw|=2;
	return 1;
}

//! Calls anXWindow::MoveResize, then redoarrows.
int Scroller::MoveResize(int nx,int ny,int nw,int nh)
{  
	anXWindow::MoveResize(nx,ny,nw,nh); // used for configure notify
	DBG cerr <<"New movesize for "<<WindowTitle()<<" x:"<<nx<<" y:"<<ny<<" w:"<<nw<<" h:"<<nh<<endl;
	redoarrows();
	needtodraw|=2;
	return 1;
}

/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *Scroller::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	if (what==-1) {
		att->push("vertical","boolean");
		att->push("horizontal","boolean");
		att->push("curpos","Top of page position");
		att->push("curposend","End of page position");
		att->push("single_step","Clicking arrows steps one of these");
		att->push("page_step","Doing page up or down does one of these");
		att->push("panner","Id of panner to use");
		return att;
	}

	if (win_style&SC_XSCROLL) att->push("vertical");
	else att->push("horizontal");

	long curpos,curposend;
	int which=((win_style&SC_XSCROLL)?1:2);
	GetCurPos(&curpos,&curposend);
	char buf[30];

	sprintf(buf,"%lu",curpos);
	att->push("curpos",buf);

	sprintf(buf,"%lu",curposend);
	att->push("curposend",buf);

	sprintf(buf,"%lu",panner->elementsize[which]);
	att->push("single_step",buf);

	sprintf(buf,"%lu",panner->pagesize[which]);
	att->push("page_step",buf);

	sprintf(buf,"%lu",panner->minsel[which]);
	att->push("minsize",buf);

	sprintf(buf,"%lu",panner->maxsel[which]);
	att->push("maxsize",buf);

	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void Scroller::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	long ncp,ncpe,nmins,nmaxs,nps,nes;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"vertical")) {
			setWinStyle(SC_YSCROLL,1);

		} else if (!strcmp(name,"horizontal")) {
			setWinStyle(SC_XSCROLL,1);

		} else if (!strcmp(name,"curpos")) {
			LongAttribute(value,&ncp);

		} else if (!strcmp(name,"curposend")) {
			LongAttribute(value,&ncpe);

		} else if (!strcmp(name,"single_step")) {
			LongAttribute(value,&nes);

		} else if (!strcmp(name,"page_step")) {
			LongAttribute(value,&nps);

		} else if (!strcmp(name,"minsize")) {
			LongAttribute(value,&nmins);

		} else if (!strcmp(name,"maxsize")) {
			LongAttribute(value,&nmaxs);
		}
	}
	panner->SetStuff(win_style&SC_XSCROLL?1:2,nmins,nmaxs,nps,nes,ncp,ncpe);
}


} // namespace Laxkit

