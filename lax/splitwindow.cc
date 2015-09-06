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
//    Copyright (C) 2004-2013 by Tom Lechner
//



#include <lax/lists.cc>
#include <lax/splitwindow.h>
#include <lax/laxutils.h>
#include <lax/menuinfo.h>
#include <lax/popupmenu.h>
#include <lax/mouseshapes.h>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

#define SPLIT_SPLITS_ASK
#define SPLIT_SPLITS_DUPLICATE
#define SPLIT_UNMAP_WHILE_MOVE

//------------------------------------- PlainWinBox -----------------------------------

/*! \class PlainWinBox
 * \brief Basically a rectangle with a anXWindow.
 *
 * This is used in SplitWindow. The x1,y1,x1,y2 correspond to the
 * gutter of the SplitWindow. The gap width of the SplitWindow is passed
 * in during sync(). The windows' x and y should thus be
 * (x1,y1)+(win_border,win_border).
 */

	
//! Plain constructor, just copies values, no addwindow.
/*! Count of nwin is incremented.
 */
PlainWinBox::PlainWinBox(anXWindow *nwin,int xx1,int yy1,int xx2,int yy2)
{
	window=nwin;
	if (window) window->inc_count();

	prev=next=NULL;
	x1=xx1;
	y1=yy1;
	x2=xx2;
	y2=yy2;
}

/*! Dec count of win.
 */
PlainWinBox::~PlainWinBox()
{
	if (window) window->dec_count();
}

//! Dec count of old, and inc count of new (if any).
void PlainWinBox::NewWindow(anXWindow *nwin)
{
	if (window) window->dec_count();
	window=nwin;
	if (window) window->inc_count();
}

//! Sync the window to the coords.
/*! if mapit==0, then only MoveResize.
 * If mapit==1, then also map the window
 *
 * If win->window does not exist, then this tries to call win->app->addwindow(win), and then
 * only if win->win_parent->window exists.
 *
 * inset is half of SplitWindow::space.
 */
void PlainWinBox::sync(int inset,int mapit)
{
	if (!window) return;
	int w=x2-x1-2*(inset+window->win_border),
		h=y2-y1-2*(inset+window->win_border);
	if (w<1) w=1;
	if (h<1) h=1;
	window->MoveResize(x1+inset, y1+inset, w,h);
	if (!window->xlib_window) {
		window->app->addwindow(window,mapit,0);
	} else if (mapit) window->app->mapwindow(window);
}


//------------------------------------- SplitWindow -----------------------------------

#define NEAR(a,b) (abs((b)-(a)) <= 2*space)

//the following defines are for mode, it is not necessary for them to be known outside splitwindow.cc
#define NORMAL          0
#define MOVE_EDGES      1
#define VERTICAL_SPLIT  2
#define HORIZ_SPLIT     3
#define MAXIMIZED       4


/*! \class SplitWindow
 * \brief A window with resizable panes than can be split and joined.
 *
 * This frame is similar to the windows found in the program Blender, and the 
 * Ion window manager. It is so cool, Laxkit just had to have its own.
 *
 * Uses PlainWinBox to store the windows. The w,h of those boxes are the
 * windows' w and h plus their borders.
 *
 * SPLIT_BEVEL draws a bevel around the panes in the space between them. 
 * SPLIT_DRAG_MAPPED will resize the panes as the edges are dragged, rather than
 * just show lines moving.
 * \code
 *  #define SPLIT_WITH_SAME     (1<<16) ***todo
 *  #define SPLIT_WITH_DEFAULT  (1<<17) ***todo
 *  #define SPLIT_WITH_BLANK    (1<<18) ***todo
 *  #define SPLIT_BEVEL         (1<<19)
 *  #define SPLIT_DRAG_MAPPED   (1<<20)
 *  #define SPLIT_STACKED_PANES (1<<21) ***todo
 *  #define SPLIT_TABBED_PANES  (1<<22) ***todo
 *  #define SPLIT_NO_MAXIMIZE   (1<<23) ***todo do and not do!
 * \endcode
 *
 * \todo *** Resizing should be redone, it is not reversible owning to 
 *   integer rounding errors. perhaps have PlainWinBox have float x,y,w,h to
 *   shadow the windows actual values? or just finish imping validateX() and validateY()
 * \todo *** maximize/minimize with alt-numpad-/ or some such
 * \todo *** implement optional stack of windows for each pane. each is
 *   accessible how(??? tabbed window? through menu), then have option to spread out the stack into 
 *   separate boxes?
 */
/*! \fn int SplitWindow::Mode()
 * \brief Return mode.
 */
/*! \var NumStack<int> SplitWindow::laffected
 * \brief Windows whose left edge is affected by a mouse move stored here.
 */
/*! \var NumStack<int> SplitWindow::raffected
 * \brief Windows whose right edge is affected by a mouse move stored here.
 */
/*! \var NumStack<int> SplitWindow::taffected
 * \brief Windows whose top edge is affected by a mouse move stored here.
 */
/*! \var NumStack<int> SplitWindow::baffected
 * \brief Windows whose bottom edge is affected by a mouse move stored here.
 */
/*! \var int SplitWindow::space
 * \brief The pixel length of the gap between windows.
 */
/*! \var int SplitWindow::mode
 * \brief What's happening: move, join, split
 *
 * <pre>
 * 0=nothing special, 
 * 1=moving,
 * 2=vertical split,
 * 3=horizontal split,
 * 4=curbox temporarily maximized
 * </pre>
 */
/*! \var int SplitWindow::minx
 * \brief The minimum x coordinate to allow edges to be dragged.
 */
/*! \var int SplitWindow::maxx
 * \brief The maximum x coordinate to allow edges to be dragged.
 */
/*! \var int SplitWindow::miny
 * \brief The minimum y coordinate to allow edges to be dragged.
 */
/*! \var int SplitWindow::maxy
 * \brief The maximum y coordinate to allow edges to be dragged.
 */
/*! \var int SplitWindow::curx
 * \brief The current x coordinate during an edge drag.
 */
/*! \var int SplitWindow::cury
 * \brief The current y coordinate during an edge drag.
 */
/*! \var int SplitWindow::sminx
 * \brief The minimum x coordinate of ***
 */
/*! \var int SplitWindow::smaxx
 * \brief The maximum x coordinate of ***
 */
/*! \var int SplitWindow::sminy
 * \brief The minimum y coordinate of ***
 */
/*! \var int SplitWindow::smaxy
 * \brief The maximum y coordinate of ***
 */


//! Constructor, merely set space=6.
SplitWindow::SplitWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long owner,const char *mes)
		: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder, prev,owner,mes)
{
	 // establish cursors??
	space=6;
	bevel=0;
	mousein=0;
	minx=curx=maxx=miny=cury=maxy=0;

	marked=NULL;
	curbox=NULL;
	mode=0;
	defaultwinfunc=-1;
	lastactivewindow=NULL;

	installColors(app->color_panel);
}

//! Adds a single blank pane.
int SplitWindow::init()
{
	if (windows.n==0) {
		 //this mysterious bit of code compensates for when win_w of win_h == 0
		int w=win_w,
			h=win_h;
		if (w<1) w=100;
		if (h<1) h=100;
		windows.push(new PlainWinBox(NULL,0,0,w,h));
	}
	return 0;
}


////! Add a window to the left of the last focused window, by splitting the last focused in half
///*! If nextto is not NULL, then add next to that rather than last focused.
// */
//int SplitWindow::AddWindowLeft(anXWindow *win,PlainWinBox *nextto)
//int SplitWindow::AddWindowLeft(anXWindow *win,anXWindow *nextto)
//{}
//int SplitWindow::AddWindowRight(anXWindow *win)
//{}
//int SplitWindow::AddWindowTop(anXWindow *win)
//{}
//int SplitWindow::AddWindowBottom(anXWindow *win)
//{}



//! Set the space between windows.
/*! Space values less than 2 and greater than 30 are rejected.
 * The new space is the first even number less than or equal to spc.
 * 
 * \todo this needs checking so that skinny windows do not get negative width/height
 */
void SplitWindow::SetSpace(int spc)
{
	if (space<2 || space>30) return;
	space=spc/2*2;
	Resize(win_w,win_h);
	return;
}

//! Refresh, draw boxes with an x through them for any NULL areas, and bevel if necessary.
void SplitWindow::Refresh()
{
	if (!needtodraw || !win_on) return;
	
	Displayer *dp=MakeCurrent();
	//XClearWindow(app->dpy,window);
	
	
	 //*** draw a big X for the panes that do not actually have windows in them
	char blah[300]; // ********* change this to dynamic alloc?
	dp->NewFG(win_colors->fg);

	unsigned long highlight, shadow;
	highlight=coloravg(win_colors->bg,rgbcolor(255,255,255));
	shadow   =coloravg(win_colors->bg,rgbcolor(0,0,0));
	
	dp->BlendMode(LAXOP_Over);

	if (mode==MAXIMIZED) {
		foreground_color(win_colors->bg);
		fill_rectangle(this, 0,0,win_w,win_h);
		if (win_style&SPLIT_BEVEL) {
			draw_bevel(this,space/2,highlight,shadow,LAX_OFF,0,0,win_w,win_h);
		}
		int c=windows.findindex(curbox);
		if (curbox->win())
			if (curbox->win()->win_title) sprintf(blah,"%d, %s",c,curbox->win()->win_title);
			else sprintf(blah,"%d, no title",c);
		else sprintf(blah,"%d",c);
		textout(this, blah,-1,win_w/2,win_h/2,LAX_CENTER);
	} else {
		for (int c=0; c<windows.n; c++) {
			if (!windows.e[c]->win() || !windows.e[c]->win()->win_on) {
				 // draw big x if no window
				foreground_color(win_colors->bg);
				fill_rectangle(this,
							   windows.e[c]->x1+space/2,windows.e[c]->y1+space/2,
							   windows.e[c]->x2-windows.e[c]->x1-space,
							   windows.e[c]->y2-windows.e[c]->y1-space);
				draw_rectangle(this,
							   windows.e[c]->x1+space/2,windows.e[c]->y1+space/2,
							   windows.e[c]->x2-windows.e[c]->x1-space,
							   windows.e[c]->y2-windows.e[c]->y1-space);
				
				foreground_color(win_colors->fg);
				if (windows.e[c]->win())
					if (windows.e[c]->win()->win_title) sprintf(blah,"%d, %s",c,windows.e[c]->win()->win_title);
					else sprintf(blah,"%d, no title",c);
				else sprintf(blah,"%d",c);
				textout(this, blah,-1,(windows.e[c]->x1+windows.e[c]->x2)/2,
									  (windows.e[c]->y1+windows.e[c]->y2)/2,LAX_CENTER);
			}
			if (win_style&SPLIT_BEVEL) {
				draw_bevel(this,space/2,highlight,shadow,LAX_OFF,
						windows.e[c]->x1,windows.e[c]->y1,
						windows.e[c]->x2-windows.e[c]->x1,windows.e[c]->y2-windows.e[c]->y1);
			}
		}
		if (mode==MOVE_EDGES) drawmovemarks(1);
	}
	needtodraw=0;
}

//! Try to join curbox to an adjacent box.
/*! If curbox->win, then app->destroywindow(curbox->win).
 *
 * If curbox cannot be connected to another box, then 0 is returned. Else 1 is returned.
 * Attempts to connect the curbox to the box near (mx,my).
 * 
 * \todo if the pane span is such that several windows might be expanded
 *   to cover it, then do that, rather than demand that 1 window can join to
 *   it.. The super fancy version would somehow figure out how to fill the void
 *   if no join is available (like a tatami mat)
 */
int SplitWindow::RemoveCurbox()
{
	if (windows.n==1) return 0;
	
	if (curbox->win()) app->destroywindow(curbox->win());
	curbox->NewWindow(NULL);

	 //then try to join the previous pane with an adjacent pane
	 //First try last mouse coordinates, then l-r-t-b
	int c=windows.findindex(curbox);
	if (joinwindow(mx,my,1)!=0) 
		if (Join(c,LAX_LEFT,1)<0)
			if (Join(c,LAX_RIGHT,1)<0)
				if (Join(c,LAX_TOP,1)<0)
					if (Join(c,LAX_BOTTOM,1)<0) return 0;
	return 1;
}

//! Figure out which windows can possibly be moved with a click and drag from (x,y).
/*! This modifies the [lrtb]affected stacks to reflect what would need to be changed
 * 	if one were to start dragging right there.
 *  Also sets the segment min/max[xy], and figures out what the mouse should look like.
 *  and sets it.
 *  All the windows that touch the min/max segments get adjusted when the mouse moves.
 *
 *  \todo *** this should probably be redone to not be very dependent on space. that would
 *    allow recovering from resizing to really small windows easier..
 */
int SplitWindow::GetAffected(int x,int y,const LaxMouse *m)
{
	laffected.flush();
	raffected.flush();
	taffected.flush();
	baffected.flush();

	if (x<=2*space || x>=win_w-2*space) {
		// *** disable for now: const_cast<LaxMouse*>(m)->setMouseShape(this,LAX_MOUSE_LeftRight);
		return 1;

	} else if (y<=2*space || y>=win_h-2*space) {
		// *** disable for now: const_cast<LaxMouse*>(m)->setMouseShape(this,LAX_MOUSE_UpDown);
		return 1;
	}
	int l,r,t,b;
	PlainWinBox *box;

	 // First find basic proximity information:
	 //  which is of form "[lLrR][tTbB]\0". A capital letter means that that window
	 //    is one of the primary windows, meaning (x,y) is right next to it.
	 //  curs has info about what the cursor should look like. For each primary,
	 //  	an "l","r","t" or "b" is appended to curs.
	 //  [min|max][xy] hold the allowable movement bounds. This first pass
	 //  	uses only the primary windows, and selects the maximum minimum, and the
	 //  	minimum maximum. The segments are expanded to encompass all other affected
	 //  	boxes in a later pass. The actual use for min/max in other functions is as
	 //  	the bounds for a movement of windows, which is not the same as the segments
	 //  	that touch affected windows.
	char which[windows.n][3], 
		 curs[10]; //holds info about what kind of cursor there might need to be
	curx=x;
	cury=y;
	sminx=sminy=space/2;
	smaxx=win_w-space/2;
	smaxy=win_h-space/2;
	curs[0]='\0';
	int c;
	for (c=0; c<windows.n; c++) {
		box=windows.e[c];
		if (!box) continue;
		
		 // Check each box edge:
		 // -1 is less than box, 0 in range of box, 1 is more than box
		 // On a left edge for instance, consider mouse near when in [x-space,x+2*space]
		 // to detect those instances where the mouse is not right in the gap,
		 // but overlaps a box. This happens at corners especially.
		 
		 // find l
		if (abs(box->x1-x)<space) l=0;
		else if (box->x1<x) l=-1;
		else l=1;

		if (abs(box->x2-x)<space) r=0;
		else if (box->x2<x) r=-1;
		else r=1;

		if (abs(box->y1-y)<space) t=0;
		else if (box->y1<y) t=-1;
		else t=1;

		if (abs(box->y2-y)<space) b=0;
		else if (box->y2<y) b=-1;
		else b=1;

		which[c][0]=which[c][1]=' ';
		which[c][2]='\0'; 
		if (l==0) { // l is in line with x
			if (t<=0 && b>=0) { // is a primary, x,y is on a corner if t or b==0
				strcat(curs,"l");
				which[c][0]='L';
			} else { // is not a primary, but left edge is still in line with x
				which[c][0]='l';
			}
		}
		if (r==0) { // r is inline with x
			if (t<=0 && b>=0) {
				strcat(curs,"r");
				which[c][0]='R'; 
			} else which[c][0]='r';
		}
		if (t==0) { // t is in line with y
			if (l<=0 && r>=0) {
				strcat(curs,"t");
				which[c][1]='T'; 
			} else which[c][1]='t';
		}
		if (b==0) { // b is in line with y
			if (l<=0 && r>=0) {
				strcat(curs,"b");
				which[c][1]='B'; 
			} else which[c][1]='b';
		}
		//DBG cerr <<"which["<<c<<"]: \""<<which[c]<<"\""<<endl;

		 // update primary segment which is [maximum min, minimum max], stored
		 // in sminx,smaxx, sminy,smaxy. Those are initially set to the whole window.
		 // this here is finding the minimum bounds of the primarie panes, not
		 // the segments that touch affected windows....
		if (which[c][0]=='L' || which[c][0]=='R' || which[c][1]=='T' || which[c][1]=='B') {
			if (l==0 || (l<0 && r>0))  // check right against smaxx 
				if (box->x2<smaxx) smaxx=box->x2;
			if (r==0 || (l<0 && r>0)) // check left against sminy 
				if (box->x1>sminx) sminx=box->x1;
			if (t==0 || (t<0 && b>0)) // check bottom against smaxy 
				if (box->y2<smaxy) smaxy=box->y2;
			if (b==0 || (t<0 && b>0)) // check top against sminy 
				if (box->y1>sminy) sminy=box->y1;

			if (l==0 && t<0 && b>0) smaxx=x;
			if (r==0 && t<0 && b>0) sminx=x;
			if (t==0 && l<0 && r>0) smaxy=y;
			if (b==0 && l<0 && r>0) sminy=y;
		}
	}
	
	
	 // Now which holds all that are potentially affected by a move, and smin/smaxx/y hold
	 // the minimum extent of the edges of the primary windows. Now we must
	 // weed out those that are not continuous with the primary vertical and horizontal segment.
	 //
	 // This is tricky because windows may hop over other windows, you cannot simply find
	 // the window right next to the current segment, and expand the segment to the extent
	 // of that new window. We must check to see if a window is already within the current segment.
	 //
	 // Also, the min/max (set up below) hold the movement bounds, and do not necessarily indicate 
	 // which windows are relevant. For instance, clicking in the middle of a horizontal segment
	 // causes sminy,smaxy to be defined to be the whole window (??), but we must not allow windows
	 // on top or bottom of the primaries that have vertical edges in line
	 // with curx to move, since though they are continuous with sminy/smaxy, they are not continuous
	 // with curx,cury and cannot move left to right. In this situation, an extra check must be done
	 // for sminy,smaxy to find the minimum extent of the actually movable windows.
	int continuous;
	if (curs[0]!='\0') {
		for (c=0; c<windows.n; c++) {
			if (!windows.e[c]) continue;
			box=windows.e[c];
			
			//remember: #define NEAR(a,b) (abs((b)-(a)) <= 2*space)

			 // check vertical segment
			if (which[c][0]!=' ') {
				 // check left edge
				continuous=0;
				if (which[c][0]=='L' || which[c][0]=='R') {
					continuous=1;
				} else if (which[c][0]=='l' || which[c][0]=='r') {
					if (NEAR(box->y1,smaxy)) {
						smaxy=box->y2;
						continuous=1;
					} else if (NEAR(box->y2,sminy)) {
						continuous=1;
						sminy=box->y1;
					} else if ((box->y1>sminy && box->y1<smaxy) || (box->y2>sminy && box->y2<smaxy)) {
						 // check for potential hopping over windows
						if (box->y1<sminy) sminy=box->y1;
						if (box->y2>smaxy) smaxy=box->y2;
						continuous=1;
					}
				}
				if (continuous) { 
					if (which[c][0]=='l' || which[c][0]=='L') laffected.push(c); else raffected.push(c);
					which[c][0]=' '; 
					c=-1; 
					continue; 
				}
			}
			
			 // check horizontal segment
			if (which[c][1]!=' ') {
				 // check top and bottom edges
				continuous=0;
				if (which[c][1]=='T' || which[c][1]=='B') {
					continuous=1;
				} else if (which[c][1]=='t' || which[c][1]=='b') {
					if (NEAR(box->x1,smaxx)) { // if left edge is near smaxx
						smaxx=box->x2;
						continuous=1;
					} else if (NEAR(box->x2,sminx)) {  // if right edge is near sminx
						continuous=1;
						sminx=box->x1;
					} else if ((box->x1>sminx && box->x1<smaxx) || (box->x2>sminx && box->x2<smaxx)) {
						 // check for potential hopping over windows
						if (box->x1<sminx) sminx=box->x1;
						if (box->x2>smaxx) smaxx=box->x2;
						continuous=1;
					}
				}
				if (continuous) { 
					if (which[c][1]=='b' || which[c][1]=='B') baffected.push(c); else taffected.push(c); 
					which[c][1]=' '; 
					c=-1; 
					continue; 
				}
			}
		}
	}
	
	 // now all the windows that are actually affected are found, must
	 // check the move bounds once more, since an incidental window might
	 // be shorter than the primaries. For instance on a horizontal segment, the
	 // vertical segment will have to be adjusted to the shortest of the windows...
	 // min/smaxx/y here hold the segments touching the affected windows, not the
	 // bounds for a move...
	 //*** should not be able to move the windows touching the splitwindow border
	 //*** or perhaps that could be a fancy option, to automatically create a void
	 //*** when moving down like that...
	
	 // remove gap leftovers***???
	if (sminx>space) sminx+=space/2;
	if (sminy>space) sminy+=space/2;
	if (smaxx<win_w-space) smaxx-=space/2;
	if (smaxy<win_h-space) smaxy-=space/2;
	
	minx=miny=space;
	maxx=win_w-space;
	maxy=win_h-space;
	if (laffected.n==0) maxx=curx;
	for (c=0; c<laffected.n; c++) {
		box=windows.e[laffected.e[c]];
		if (box->x2<maxx) maxx=box->x2;
	}
	if (raffected.n==0) minx=curx;
	for (c=0; c<raffected.n; c++) {
		box=windows.e[raffected.e[c]];
		if (box->x1>minx) minx=box->x1;
	}
	if (taffected.n==0) maxy=cury;
	for (c=0; c<taffected.n; c++) {
		box=windows.e[taffected.e[c]];
		if (box->y2<maxy) maxy=box->y2;
	}
	if (baffected.n==0) miny=cury;
	for (c=0; c<baffected.n; c++) {
		box=windows.e[baffected.e[c]];
		if (box->y1>miny) miny=box->y1;
	}
	
	//DBG //**** dump affacted:
	//DBG cerr <<"laffected: ";
	//DBG for (c=0; c<laffected.n; c++) cerr <<laffected.e[c]<<", ";
	//DBG cerr <<endl<<"raffected: ";
	//DBG for (c=0; c<raffected.n; c++)  cerr <<raffected.e[c]<<", ";
	//DBG cerr <<endl<<"taffected: ";
	//DBG for (c=0; c<taffected.n; c++)  cerr <<taffected.e[c]<<", ";
	//DBG cerr <<endl<<"baffected: ";
	//DBG for (c=0; c<baffected.n; c++)  cerr <<baffected.e[c]<<", ";
	//DBG cerr <<endl;
	//DBG cerr <<" minx,maxx: "<<minx<<','<<maxx<<"   miny,maxy: "<<miny<<','<<maxy<<endl;
	//DBG cerr <<" sminx,smaxx: "<<sminx<<','<<smaxx<<"   sminy,smaxy: "<<sminy<<','<<smaxy<<endl;
	
	 // Set cursor to left-right, up-down, or 4-way arrows.
	SetCursor(curs,const_cast<LaxMouse*>(m));
	
	return 0;
}

//! Set cursor based on curs.
/*! This is called from GetAffected(). Default is to use
 * the typical up/down, left/right, or 4-way cursor.
 * Subclasses might want to get super fancy and take into account 
 * the multitude of possibilities for cursors.
 * They can also consult the [lrtb]affected stacks for more info. Those
 * stacks must accurately be reflected by curs.
 * 
 * Return 1 for cursor changed, or 0.
 * 
 * <pre>
 *   .  . .    .
 *    #### ####
 *    #### ####
 *    #### ####
 *   . .  . .  .
 *    #### ####
 *   .#### ####. 
 *    #### ####
 *   . .  .    .
 *   curs has info about the cursor: If there are no primaries, then the cursor is off the
 *   window, or is inside a box, or otherwise not near an edge, and curs="", else assuming 
 *   that there is no more than space
 *   between windows, curs will be some permutation of these:
 *   	The well formed ones:
 *  	 "lr"       - vert gap,  cursor points left/right
 *    "tb"       | horiz gap, curs up/down
 *    "lrrtb"    < up/down/left
 *    "llrtb"    > up/down/right
 *    "lrbtt"    v l/r/down
 *    "lrbbt"    ^ l/r/up
 *    "llrrbbtt" + u/d/l/r
 *     
 *      These are on edges where there is only one side of a crack:
 *    "l"     on far left, use cursor up/down
 *    "lltb"  use l/u/d/r
 *    "lb"    low left corner, use u/d/l/r
 *    "lrbb"
 *    "b"
 *    "br"
 *    "r"
 *    "rrtb"
 *    "tr"
 *    "t"
 *    "lrtt"  top center, use l/r/t/b
 *    "lt"    up-left corner, use l/r/t/b
 * </pre>
 */
int SplitWindow::SetCursor(const char *curs,LaxMouse *d)
{
	 //***figure out which it is, and set cursor??
	//DBG cerr <<"  curs="<<curs<<endl;
	
	int cursortype=0;
	if (laffected.n || raffected.n) cursortype++; // for left-right
	if (taffected.n || baffected.n) cursortype+=2; //for up-down

	// *** disabling for now
	//if (cursortype==1) d->setMouseShape(this,LAX_MOUSE_LeftRight);
	//else if (cursortype==2) d->setMouseShape(this,LAX_MOUSE_UpDown);
	//else if (cursortype==3) d->setMouseShape(this,LAX_MOUSE_Pan);

	// **** just use pan always for now
	//d->setMouseShape(this,LAX_MOUSE_Pan);
	return 1;
}


//! Check for any edge of box within space of x,y.
/*! Assumes that l,r,t,b are all already allocated pointers to int,
 * and this function fills them with -1,0, or 1, depending on whether the 
 * corresponding box edge
 * is less than, near, or greater than a box centered on x,y with width 2*space.
 */
void SplitWindow::BoxNearPos(int x,int y,PlainWinBox *box, int *l,int *r,int *t,int *b)
{
	if (box->x1+space < x) *l=-1;
	else if (box->x1-space > x) *l=1;
	else *l=0;

	if (box->x2+space < x) *r=-1;
	else if (box->x2-space > x) *r=1;
	else *r=0;

	if (box->y1+space < y) *t=-1;
	else if (box->y1-space > y) *t=1;
	else *t=0;

	if (box->y2+space < y) *b=-1;
	else if (box->y2-space > y) *b=1;
	else *b=0;
}

//! Check if x,y is within a box, -1 is too negative, 1 is too positive, 0 is inside it.
/*! It is ok for xx or yy to be NULL.
 */
void SplitWindow::PosInBox(int x,int y,PlainWinBox *box, int *xx,int *yy)
{
	if (xx) {
		if (x<box->x1+space/2) *xx=-1;
		else if (x>box->x2+space/2) *xx=1;
		else *xx=0;
	}
	if (yy) {
		if (y<box->y1+space/2) *yy=-1;
		else if (y>box->y2+space/2) *yy=1;
		else *yy=0;
	}
}

//! Find and set the current box based on whether the mouse is in the pane or not.
/*! If x,y are found to be within the bounds of a box, then that becomes
 * the current box. Otherwise, if curbox exists already then it is not changed.
 * If cubox was NULL, then the last focused window is the current box,
 * that is to say the immediate child of the SplitWindow that last held the focus (or
 * any of the child's descendents).
 *
 * This is called by setupsplit() if !curbox, and LBDown() (always).
 */
PlainWinBox *SplitWindow::findcurbox(int x,int y,const LaxMouse *mouse)
{
	 // if mouse is in a pane, then that is the curbox
	for (int c=0; c<windows.n; c++) {
		if (x>=windows.e[c]->x1+space/2 && x<windows.e[c]->x2-space/2 && 
				y>=windows.e[c]->y1+space/2 && y<windows.e[c]->y2-space/2) {
			Curbox(c); 
			break;
		}
	}
	
	if (curbox) return curbox;
	 
	 // attempt to get the last left window stored in app
	if (mouse) {
		anXWindow *win=app->findwindow_by_id(mouse->last_leave_window);
		if (win) {
			while (win->win_parent && win->win_parent!=this) win=win->win_parent;
			if (win->win_parent) {
				int c=FindBox(win);
				if (c>=0) {
					Curbox(c);
					return curbox;
				}
			}
		}
	}
 
	 // easy case when there's only one
	if (windows.n==1) {
		Curbox(0); 
	}

	return curbox;
}

//! Determine whether to split vertically or horizontally and set mode accordingly.
/*! Based on x,y, finds the edge of curbox nearest that point, and sets mode to
 * HORIZ_SPLIT (to cut horizontally, making 2 boxes vertically) or
 * VERTICAL_SPLIT (to cut vertically, making 2 boxes horizontally) accordingly.
 *
 * Also sets curx,cury so that they are within the bounds of curbox.
 */
void SplitWindow::setupsplit(int x,int y)
{
	 // If control-left, then try to split the window...
	 // find which window it is, 
	 // then determine whether to split v or h
	if (!curbox) if (!findcurbox(x,y,NULL)) return;
	if (NEAR(x,curbox->x1+space/2) || NEAR(x,curbox->x2-space/2)) mode=VERTICAL_SPLIT;
	else mode=HORIZ_SPLIT;
	DBG cerr <<"***************changing mode to V or H SPLIT"<<endl;
	
	if (curx<curbox->x1+space/2) curx=curbox->x1+space/2;
	else if (curx>curbox->x2-space/2) curx=curbox->x2-space/2;
	if (cury<curbox->y1+space/2) cury=curbox->y1+space/2;
	else if (cury>curbox->y2-space/2) cury=curbox->y2-space/2;
	return;
}

//! Join curbox along the edge closest to (x,y).
/*! If keepother, then the window of curbox removed rather than the other one.
 *
 * Return 0 on success. 
 * Return 1 if if curbox==NULL.
 * Return 2 if (x,y) is not near an edge of curbox.
 * Return 3 if illegal join.
 * 
 * \todo *** when windows are joined, what happens to the vanquished one?
 *   store it in limbo?
 */
int SplitWindow::joinwindow(int x,int y,char keepother)
{
	if (!curbox) return 1;
	
	 // find the edge and the window to join with.
	int c;
	PlainWinBox *jointo=NULL;
	if (NEAR(x,curbox->x1+space/2)) { // join a right edge with Left edge of curbox
		for (c=0; c<windows.n; c++) {
			if (windows.e[c]==curbox) continue;
			if (NEAR(curbox->x1,windows.e[c]->x2)) { 
				 // found right edge, check that top and bottom match
				if (NEAR(curbox->y1,windows.e[c]->y1) && NEAR(curbox->y2,windows.e[c]->y2)) {
					jointo=windows.e[c];
					curbox->x1=jointo->x1;
					break;
				}
			}
		}
	} else if (NEAR(x,curbox->x2)) { // right of curbox
		for (c=0; c<windows.n; c++) {
			if (windows.e[c]==curbox) continue;
			if (NEAR(curbox->x2,windows.e[c]->x1)) { 
				 // found left edge, check that top and bottom match
				if (NEAR(curbox->y1,windows.e[c]->y1) && NEAR(curbox->y2,windows.e[c]->y2)) {
					jointo=windows.e[c];
					curbox->x2=jointo->x2;
					break;
				}
			}
		}
	} else if (NEAR(y,curbox->y1)) { // top of curbox
		for (c=0; c<windows.n; c++) {
			if (windows.e[c]==curbox) continue;
			if (NEAR(curbox->y1,windows.e[c]->y2)) { 
				 // found bottom edge, check that left and right match
				if (NEAR(curbox->x1,windows.e[c]->x1) && NEAR(curbox->x2,windows.e[c]->x2)) {
					jointo=windows.e[c];
					curbox->y1=jointo->y1;
					break;
				}
			}
		}
	} else if (NEAR(y,curbox->y2)) { // bottom of curbox
		for (c=0; c<windows.n; c++) {
			if (windows.e[c]==curbox) continue;
			if (NEAR(curbox->y2,windows.e[c]->y1)) { 
				 // found top, check that left and right match
				if (NEAR(curbox->x1,windows.e[c]->x1) && NEAR(curbox->x2,windows.e[c]->x2)) {
					jointo=windows.e[c];
					curbox->y2=jointo->y2;
					break;
				}
			}
		}
	} else return 2;
	if (!jointo) return 3;
	
	 //*** if there is a window, should put it in limbo rather than destroy???
	if (keepother) {
		if (curbox->win()) app->destroywindow(curbox->win());
		curbox->NewWindow(jointo->win());
		jointo->NewWindow(NULL);
	} else if (!keepother && jointo->win()) {
		app->destroywindow(jointo->win());
		jointo->NewWindow(NULL);
	}
	curbox->sync(space/2,0);
	windows.remove(windows.findindex(jointo));
	needtodraw=1;
	//DBG cerr <<"***************************** join ***************************************"<<endl;
		
	return 0;
}

//! Initiates a resizing action, split (cntl-click), or join (shift-click).
/*! 
 * \todo Straight clicking starts a move on the whole continuous segment.
 *   would be neat if double clicking moved only the primary windows.
 */
int SplitWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d) 
{  
	if (buttondown.isdown(0,LEFTBUTTON)) return 0;
	buttondown.down(d->id,LEFTBUTTON, x,y);
	
	mx=curx=x; my=cury=y;
	if (mode==NORMAL) {
		if ((state&LAX_STATE_MASK)==ShiftMask) { // join curbox along edge nearest mouse
			joinwindow(x,y,0);
			//buttondown&=~LEFTBUTTON; //*** should delay actual join till LBUp?
			DBG cerr <<"***************changing mode to NORMAL"<<endl;
			mode=NORMAL;
			needtodraw=1;
			return 0;
		}
		if ((state&LAX_STATE_MASK)==ControlMask) { // split the window that the mouse entered the boundary from.
			setupsplit(x,y); //sets mode
			drawsplitmarks();
			return 0;
		}
		if ((state&LAX_STATE_MASK)!=0) return 0;

		 // determine allowable move direction(s) and max/min allowable movement
		mode=MOVE_EDGES;
		DBG cerr <<"***************changing mode to MOVE_EDGES"<<endl;
		GetAffected(x,y,d);
		
		 // if any, unmap affected windows, and initiate xor'd markings
		if (!(win_style&SPLIT_DRAG_MAPPED)) {
			int c;
			for (c=0; c<laffected.n; c++) if (windows.e[laffected.e[c]]->win()) app->unmapwindow(windows.e[laffected.e[c]]->win());
			for (c=0; c<raffected.n; c++) if (windows.e[raffected.e[c]]->win()) app->unmapwindow(windows.e[raffected.e[c]]->win());
			for (c=0; c<taffected.n; c++) if (windows.e[taffected.e[c]]->win()) app->unmapwindow(windows.e[taffected.e[c]]->win());
			for (c=0; c<baffected.n; c++) if (windows.e[baffected.e[c]]->win()) app->unmapwindow(windows.e[baffected.e[c]]->win());
		}

		drawmovemarks(1);
	}
	return 0;
}

//! Sync the affected windows after a drag so that the gap is centered on curx/y.
/*! This is called to aid updating of window dragging from MouseMove() if SPLIT_DRAG_MAPPED.
 * and otherwise from LBUp() only.
 */
void SplitWindow::syncaffected()
{
	int c;
	for (c=0; c<laffected.n; c++) { 
		windows.e[laffected[c]]->x1=curx;
		windows.e[laffected[c]]->sync(space/2,1); 
	}
	for (c=0; c<raffected.n; c++) { 
		windows.e[raffected[c]]->x2=curx;
		windows.e[raffected[c]]->sync(space/2,1); 
	}
	for (c=0; c<taffected.n; c++) { 
		windows.e[taffected[c]]->y1=cury;
		windows.e[taffected[c]]->sync(space/2,1); 
	}
	for (c=0; c<baffected.n; c++) { 
		windows.e[baffected[c]]->y2=cury;
		windows.e[baffected[c]]->sync(space/2,1); 
	}
	needtodraw=1;
}

/*! Split if mode==VERTICAL_SPLIT || mode==HORIZ_SPLIT.
 */
int SplitWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d) 
{ // 
	if (!buttondown.isdown(d->id,LEFTBUTTON)) {
		return 1;
	}
	buttondown.up(d->id,LEFTBUTTON);

	if (mode==MOVE_EDGES) {
		drawmovemarks(0);

		 // resize affected windows and map them
		syncaffected();
		needtodraw=1;

	} else if (mode==VERTICAL_SPLIT || mode==HORIZ_SPLIT) {
		drawsplitmarks();
		splitthewindow();
		needtodraw=1;
	}
	mode=0;
	DBG cerr <<"***************changing mode to NORMAL"<<endl;
	return 0;
}

//! Create a window by calling the winfuncs entry corresponding to wtype.
/*! Returns NULL if no such type exists.
 *
 * If wtype is NULL, then use these:
 * SPLIT_WITH_SAME assumes that winfunc names are the whattype() of the window, and fills
 * the new space with a window of that type (wtype is that whattype, this function is called
 * from splitthewindow()).
 * SPLIT_WITH_DEFAULT returns a window of type defaultwinfunc.
 * SPLIT_WITH_BLANK returns NULL.
 *
 * \todo **** implement the SPLIT_*!!!!
 *
 * If likethis!=NULL, then NewWindow should try to make the new window as much like it as
 * possible. In this default SplitWindow, likethis is ignored.
 */
anXWindow *SplitWindow::NewWindow(const char *wtype,anXWindow *likethis)
{
	if (!wtype) {
		if (defaultwinfunc<0) return NULL;
		else wtype=winfuncs.e[defaultwinfunc]->name;
	}
		
	anXWindow *win=NULL;
	char blah[50];
	for (int c=0; c<winfuncs.n; c++) {
		if (!strcmp(winfuncs.e[c]->name,wtype)) {
			sprintf(blah,"SplitPane%d",c);
			win=winfuncs.e[c]->function(this,blah,winfuncs.e[c]->style,NULL);
			return win;
		}
	}
	return NULL;
}

//! Called usually from LBUp (for which fill=NULL), splits curbox along curx or cury if mode==HORIZ_SPLIT or VERTICAL_SPLIT.
/*! This does the actual splitting of a pane.
 * setupsplit() should have been called before this, which sets mode. mode is not changed
 * within this function.
 * whichside is mainly a hint for which side to put fillwindow.
 * So if you pass LAX_LEFT for a vertical split,
 * it will not make it actually a horizontal split. 
 *
 * fillwindow is reparented here. Note that the value in mode is not actually
 * changed in this function.
 *
 * Returns 0 if the window was actually split, nonzero error otherwise.
 * Remember that the new window is on the top of the windows stack.
 *
 * Current possible errors are:\n
 * 1: There is no current box to split!\n
 * 2: The split bar is too near the window's edge, so no split is possible.
 * 
 */
int SplitWindow::splitthewindow(anXWindow *fillwindow,int whichside)//fillwindow=NULL,whichside=0
{
	DBG cerr <<"SPLIT the window in two"<<endl;
	if (!curbox) return 1;
	
	if (fillwindow) app->reparent(fillwindow,this);
	else fillwindow=NewWindow(curbox->win()?curbox->win()->whattype():NULL,curbox->win());
	
	if (mode==HORIZ_SPLIT) {
		if (NEAR(curx,curbox->x1) || NEAR(curx,curbox->x2)) return 2;
		
		if (whichside==LAX_RIGHT) {
			windows.push(new PlainWinBox(fillwindow, curx,curbox->y1, curbox->x2,curbox->y2));
			curbox->x2=curx;
		} else { //LAX_LEFT
			windows.push(new PlainWinBox(fillwindow, curbox->x1,curbox->y1, curx,curbox->y2));
			curbox->x1=curx;
		}
	} else {
		if (NEAR(cury,curbox->y1) || NEAR(cury,curbox->y2)) return 2;
		
		if (whichside==LAX_TOP) {
			windows.push(new PlainWinBox(fillwindow, curbox->x1,curbox->y1, curbox->x2,cury));
			curbox->y1=cury;			
		} else { //LAX_BOTTOM
			windows.push(new PlainWinBox(fillwindow, curbox->x1,cury, curbox->x2,curbox->y2));
			curbox->y2=cury;
		}
	}
	windows.e[windows.n-1]->sync(space/2,1);
	curbox->sync(space/2,0);
	needtodraw=1;
		
	//DBG cerr <<"***************************** split ***************************************"<<endl;
	return 0;
}

//! Draw the line showing where a window will be split.
/*! This draws a line by xor'ing with the screen, thus drawing
 * the lines on, then off are done with the same operation.
 */
void SplitWindow::drawsplitmarks()
{
	if (!curbox) return;
	drawing_function(LAXOP_Xor);
	foreground_color(win_colors->fg);
	if (mode==HORIZ_SPLIT) {
		draw_line(this, curx,curbox->y1, curx,curbox->y2);
	} else {
		draw_line(this, curbox->x1,cury, curbox->x2,cury);
	}
	drawing_function(LAXOP_Over);
}

//! Draw vertical and/or horizontal lines when adjusting window boundaries.
/*! Draws a horizontal line from (minx,cury) to (maxx,cury) and
 * a vertical line from (curx,miny) to (curx,maxy).
 *
 * This is slightly different then the temporary lines that are drawn when
 * a window is being split.
 */
void SplitWindow::drawmovemarks(int on) //on=1
{
	//DBG if (on) cerr <<"draw on "<<minx<<','<<maxx<<' '<<miny<<','<<maxy<<' '<<curx<<','<<cury<<endl;
	//DBG else cerr <<"draw off "<<minx<<','<<maxx<<' '<<miny<<','<<maxy<<' '<<curx<<','<<cury<<endl;
	//DBG cerr <<"  line: "<<curx<<","<<sminy<<" to "<<curx<<","<<smaxy<<endl;
	//DBG cerr <<"  line: "<<sminx<<","<<cury<<" to "<<smaxx<<","<<cury<<endl;

	if (on) foreground_color(rgbcolor(255,255,255));
	else foreground_color(win_colors->bg);
	drawing_function(LAXOP_Over);

	if (laffected.n+raffected.n>0) draw_line(this, curx,sminy,curx,smaxy);
	if (baffected.n+taffected.n>0) draw_line(this, sminx,cury,smaxx,cury);
}

//! Move the Mouse
/*! If nothing pressed, then GetAffected(). If left pressed, then move affected.
 *  If right pressed then pop up the split/join menu *** but mouse grab
 *  should have been transferred to the popup menu.
 */
int SplitWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *d) 
{
	if (!buttondown.any() && mode==NORMAL) {
		findcurbox(x,y,d);
		if (mousein) GetAffected(x,y,d); //this is to update cursor
		return 0;
	}
	//if (buttondown.isdown(d->id,RIGHTBUTTON)) {
	//	//***split/join menu
	//	return 1;
	//}
	if (buttondown.isdown(d->id,LEFTBUTTON)) { // move the segments, if any
		if (mode==MOVE_EDGES) {
			DBG cerr <<" SplitWindow: MOVE_EDGES"<<endl;
			drawmovemarks(0);
			if (raffected.n+laffected.n>0) {
				curx+=(x-mx);
				if (curx<minx+2*space) curx=minx+2*space;
				else if (curx>maxx-2*space) curx=maxx-2*space;
			}
			if (baffected.n+taffected.n>0) {
				cury+=(y-my);
				if (cury<miny+2*space) cury=miny+2*space;
				else if (cury>maxy-2*space) cury=maxy-2*space;
			}
			drawmovemarks(1);
			if ((win_style&SPLIT_DRAG_MAPPED)) syncaffected();
			needtodraw=1;
		} else if (mode==VERTICAL_SPLIT || mode==HORIZ_SPLIT) {
			drawsplitmarks();
			curx=x;
			cury=y;
			if (curx<curbox->x1+space/2) curx=curbox->x1+space/2;
			else if (curx>curbox->x2-space/2) curx=curbox->x2-space/2;
			if (cury<curbox->y1+space/2) cury=curbox->y1+space/2;
			else if (cury>curbox->y2-space/2) cury=curbox->y2-space/2;
			drawsplitmarks();
		}
		buttondown.move(d->id,x,y);
		mx=x;
		my=y;
	}
	return 1;
}

//! Change the window in pane which (or curbox if which<0) to towhat.
/*! The old window gets app->destroywindow'd.
 *
 * If absorbcount!=0, then take possession of one count of towhat. Otherwise, the calling
 * code will have to dec_count whatever count it had.
 *
 * If there is an error, the count is NOT absorbed.
 *
 * Return 0 if window taken, else nonzero error.
 */
int SplitWindow::Change(anXWindow *towhat,int absorbcount, int which)//which=NULL
{
	PlainWinBox *whichbox=NULL;
	if (which<0 || which>=windows.n) whichbox=curbox;
	else whichbox=windows.e[which];
	if (!whichbox) return 1;

	if (towhat) app->reparent(towhat,this);
	if (whichbox->win()) app->destroywindow(whichbox->win());
	whichbox->NewWindow(towhat);
	if (mode==MAXIMIZED) {
		if (whichbox->win()) whichbox->win()->MoveResize(space/2,space/2,win_w-space,win_h-space);
	} whichbox->sync(space/2,1);

	if (absorbcount && towhat) towhat->dec_count();

	return 0;
}

//! Build and return a menu based on what can be done to curbox.
/*! Makes menu with 'Split', 'Join', and all the desc members of objects
 * in winfuncs in a submenu for 'Change'.
 *
 * The message sent has data.l[0]==curitem, and data.l[1]==id:
 * <pre>
 *  Split --> 1
 *  Join  --> 2
 *  Change to:
 *    winfunc number i (starting from 0)--> 101+i
 *    (Blank) --> 100
 *  Mark  --> 3
 *  Swap with Marked  --> 4
 *  Maximize (Or Reduce) --> 5
 *  
 * </pre>
 *
 * \todo *** swap with....
 */
MenuInfo *SplitWindow::GetMenu()
{
	MenuInfo *menu=new MenuInfo();
	if (mode!=MAXIMIZED) {
		menu->AddItem("Split",SPLITW_Split);
		menu->AddItem("Join",SPLITW_Join);
	}
	if (winfuncs.n) {
		menu->AddItem("Change to");
		menu->SubMenu();
		for (int c=0; c<winfuncs.n; c++) {
			menu->AddItem(winfuncs.e[c]->desc,SPLITW_ChangeTo_Start+c);
		}
		menu->AddItem("(Blank)",SPLITW_ChangeTo_Blank);
		menu->EndSubMenu();
	}
	if (mode!=MAXIMIZED) {
		menu->AddItem("Mark",SPLITW_Mark);
		menu->AddItem("Swap with marked",SPLITW_Swap_With_Mark);
	}
	if (mode==MAXIMIZED) menu->AddItem("Un-Maximize",SPLITW_UnMaximize);
	else menu->AddItem("Maximize",SPLITW_Maximize);
	return menu;
}

//! Pops up the join/split/change window.
/*! \todo *** should only pop a menu if in a blank pane or in gutter..
 * this is because uncaught mouse events are propagated in anXWindow...
 */
int SplitWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d) 
{ //*** 
	if (buttondown.any()) return 0;
	if (mode!=MAXIMIZED && mode!=NORMAL) mode=NORMAL; //******aggg!
	
	// *** disable for now: const_cast<LaxMouse*>(d)->setMouseShape(this,0);
	mousein=0;
	if (mode==NORMAL || mode==MAXIMIZED) {
		MenuInfo *menu=GetMenu();
		if (menu) {
			PopupMenu *pop=new PopupMenu("Popup file menu",NULL, 0,
							0,0,0,0,1, 
							object_id,"popupsplitmenu", 
							d->id,menu,1);
			//pop->menustyle|=MENUSEL_DESTROY_ON_FOCUS_OFF;*** mustn't!
			app->rundialog(pop);
			//buttondown.down(d->id,RIGHTBUTTON,x,y);
			mx=x; my=y;
		}
	}
	return 0;
}

int SplitWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d) 
{ 
	if (!buttondown.isdown(d->id,RIGHTBUTTON)) return 1;
	buttondown.up(d->id,RIGHTBUTTON);
	return 0;
}

//! Must keep track of where the mouse enters from...
/*! The joining, splitting, and resizing of windows depends on the edge
 *  that the mouse enters the spacing through. So EnterNotify, and focus
 *  events need special treatment....***
 *
 * Also Catches 'popupsplitmenu'. See GetMenu().
 *
 * For 1 (split),
 *     2 (join),
 *     3 (mark),
 *     4 (swap with marked),
 *     5 toggle temporary pane maximized
 *  it is assumed that mx and my are relevant coordinates.
 *
 *  100 means change to a blank pane. >=101 means change to
 *  window type n-101.
 */
int SplitWindow::Event(const EventData *e, const char *mes)
{
	//DBG cerr <<"SplitWindow::event:"<<xlib_event_name(e->type)<<endl;
	//DBG if (e->type==EnterNotify || e->type==LeaveNotify) { cerr <<" crossing:"; printxcrossing(this,e); }

	if (e->type==LAX_onMouseOut) {
		//const EnterExitData *ee=dynamic_cast<const EnterExitData*>(e);
		mousein=0;
		if (!buttondown.any()) {
			DBG cerr <<"out of splitwindow, reseting cursor"<<endl;
			// *** disable for now: dynamic_cast<LaxMouse*>(ee->device)->setMouseShape(this,0);
		}
		return 0;

	} else if (e->type==LAX_onMouseIn) {
		const EnterExitData *ee=dynamic_cast<const EnterExitData*>(e);
		mousein=1;
		if (!buttondown.any()) {
			 //*** must establish the proper cursor
			
			// xlib manual says that NotifyInferior will set subwindow to the child
			// window that had the pointer on an EnterNotify, but mine does not!  
			// Must check if that is a Blackbox problem or more general.. 
			// so falling back to the fltk-esque feature of storing the last x events, 
			// in this case the last LeaveNotify. If it is descended from this, then
			// we have a winner.

			anXWindow *win=app->findwindow_by_id(dynamic_cast<LaxMouse*>(ee->device)->last_leave_window);
			if (win) {
				while (win->win_parent && win->win_parent!=this) win=win->win_parent;
				if (win->win_parent) {
					lastactivewindow=win;
					 //mouse entered this from one of its panes
					int c=FindBox(win);
					if (c>=0) Curbox(c);
				}
			}
			//DBG if (!curbox) cerr <<"enter did not find curbox"<<endl;
			
			GetAffected(ee->x,ee->y,dynamic_cast<LaxMouse*>(ee->device));
		}
		return 0;//anXWindow default is just to return

//	} else if (e->type==LAX_onMouseDown || e->type==LAX_onMouseUp) {
//		 // do not respond to propagated events
//		return 0;

	} else 	if (!strcmp(mes,"popupsplitmenu")) {

		const SimpleMessage *ee=dynamic_cast<const SimpleMessage*>(e);
		int what=ee->info2;
		DBG cerr <<"SplitWindow got popupsplitmenu with "<<what<<endl;
	
		if (what==SPLITW_Split) { // split
			setupsplit(mx,my);
			if (mode==HORIZ_SPLIT || mode==VERTICAL_SPLIT) {
				splitthewindow();
			}
			DBG cerr <<"***************changing mode to NORMAL"<<endl;
			mode=0;
			return 0;

		} else if (what==SPLITW_Join) { // join
			joinwindow(mx,my,0);
			return 0;

		} else if (what==SPLITW_Mark) { // mark
			Mark(-1);
			return 0;

		} else if (what==SPLITW_Swap_With_Mark) { // swap with marked
			SwapWithMarked();
			return 0;

		} else if (what==SPLITW_UnMaximize || what==SPLITW_Maximize) { // maximize/unmaximize
			Maximize(-1);
			return 0;
		}
		
		what-=SPLITW_ChangeTo_Start;
		if (what==-1) {
			Change(NULL,0,-1);
		} else if (what>=0 && what<winfuncs.n && curbox) {
			char blah[50];
			int c;
			for (c=0; c<windows.n; c++) if (windows.e[c]==curbox) break;
			sprintf(blah,"SplitPane%d",c);
			anXWindow *newone=winfuncs.e[what]->function(this,blah,winfuncs.e[what]->style,NULL);
			if (Change(newone,1,-1)!=0) delete newone;
		}
		return 0;
	}

	return anXWindow::Event(e,mes);
}

//! Toggle curbox maximized if which==-1, or maximize (1) or un-maximize(0).
/*! Return 1 for is now maximized, or 0 for not, or -1 if you cannot change
 * the maximization state for current mode, or -2 for curbox cannot be found.
 *
 * To maximize, mode must previously have been 0.
 *
 * If there is only one window, you cannot put the window in maximized mode.
 */
int SplitWindow::Maximize(int which)
{
	if (windows.n==1) return -1;
	if (mode!=NORMAL && mode!=MAXIMIZED) return -1;
	if (which==1 && mode==MAXIMIZED) return 1;
	if (which==0 && mode!=MAXIMIZED) return 0;
	 
	 //else toggle maximized
	if (mode==MAXIMIZED) {
		for (int c=0; c<windows.n; c++) 
			if (windows.e[c]->win()) windows.e[c]->sync(space/2,1);
		needtodraw=1;
		DBG cerr <<"***************changing mode to NORMAL"<<endl;
		mode=NORMAL;
	} else {
		if (!curbox) findcurbox(mx,my,NULL);
		if (!curbox) return -2;
		int c;
		 
		 // unmap the other windows
		for (c=0; c<windows.n; c++) 
			if (windows.e[c]!=curbox && windows.e[c]->win()) app->unmapwindow(windows.e[c]->win());
		if (curbox->win()) curbox->win()->MoveResize(space,space,win_w-2*space,win_h-2*space);
		needtodraw=1;
		DBG cerr <<"***************changing mode to MAXIMIZED"<<endl;
		mode=MAXIMIZED;
	}
	return mode==MAXIMIZED;
}

int SplitWindow::FocusOn(const FocusChangeData *e)
{ //***
	return anXWindow::FocusOn(e);
}

int SplitWindow::FocusOff(const FocusChangeData *e)
{ //***
	if (e->target==this) {
		if (mode!=NORMAL && mode!=MAXIMIZED) {
			 //****need better mode switching..
			mode=NORMAL;
			//const_cast<LaxMouse*>(d)->setMouseShape(this,0); //clear which mouse!!!
			DBG cerr <<"***************changing mode to NORMAL"<<endl;
		}
	}
	return anXWindow::FocusOff(e);
}

/*! '/' on the keypad will maximize or de-maximize the current pane.
 */
int SplitWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d) 
{ //*** 
	DBG cerr <<WindowTitle()<<": keypress '"<<((ch>=32 && ch<127)?ch:'?')<<"',"<<((int) ch);
	switch (ch) {
		case LAX_Esc: //esc
			if (mode!=VERTICAL_SPLIT && mode!=HORIZ_SPLIT) return 1;
			drawsplitmarks(); // this ought to turn off the split line.
			DBG cerr <<"***************changing mode to NORMAL"<<endl;
			mode=NORMAL;
			break;
		case 'j':
		case 'J': // join
			DBG cerr <<"join...."<<endl;
			//watch out for propagated!!
			break;
		case 's':
		case 'S': // split
			DBG cerr <<"split...."<<endl;
			break;
		case 'c':
		case 'C': // change
			DBG cerr <<"change...."<<endl;
			break;
		case '/': {
			if ((state&KeypadMask) && (state&LAX_STATE_MASK)==AltMask) {
				if (mode!=MAXIMIZED) {
					int x,y;
					int mid=d->paired_mouse?d->paired_mouse->id:0;
					mouseposition(mid, this,&x,&y,NULL,NULL);
					x=FindBox(x,y);
					if (x>=0) Curbox(x);
				}
				Maximize(-1);
				return 0;
			}
			return 1;
		  }
		default: return 1;
	}
	return 1;
}

//! Return the index in windows corresponding to box, else return < 0.
int SplitWindow::FindBoxIndex(PlainWinBox *box)
{
	return windows.findindex(box);
}

//! Return the index of the box that x,y is in, or -1.
/*! This must be in the seen box. That is, excluding the gap.
 */
int SplitWindow::FindBox(int x,int y)
{
	for (int c=0; c<windows.n; c++) {
		if (x>=windows.e[c]->x1+space/2 && x<windows.e[c]->x2-space/2 && 
				y>=windows.e[c]->y1+space/2 && y<windows.e[c]->y2-space/2) {
			return c; 
		}
	}
	return -1;
}

//! Find the box containing win. Checks to make sure win is an immediate child of this.
/*! Return the index in windows (so >=0 if it is there) of the box containing win.
 * Else return -1.
 *
 * Does not change curbox.
 */
int SplitWindow::FindBox(anXWindow *win)
{
	if (!win) return -1;
	int c;
	for (c=0; c<windows.n; c++) {
		if (win==windows.e[c]->win()) break;
	}
	if (c==windows.n) return -1;
	return c;
}

//! Called on "popupsplitmenu" client message with id==4.
/*! This does the work of swapping with a marked window. The built in
 * swapping uses SplitWindow::marked, and only swaps within this window.
 * Subclasses might redefine this to use any top level SplitWindow.
 *
 * This function is called straight away from receiving the client
 * message. Any checking that a swap should proceed should be done
 * within this function.
 */
int SplitWindow::SwapWithMarked()
{
	if (curbox==NULL || marked==NULL || curbox==marked) return 0;
	anXWindow *w=curbox->win();
	w->inc_count();
	curbox->NewWindow(marked->win());
	marked->NewWindow(w);
	w->dec_count();
	curbox->sync(space/2);
	marked->sync(space/2);
	return 0;
}

//! Set marked to pane index c, or curbox if c==-1.
int SplitWindow::Mark(int c)
{
	if (c==-1) marked=curbox;
	else {
		if (c<0 || c>windows.n) return 1;
		marked=windows.e[c];
	}
	return 0;
}

//! Make curbox this pane, which is an index into windows.
/*! You cannot force curbox to be NULL. Checks to make sure c is valid index of windows.
 * Return c if curbox changed or was already curbox. If there was no curbox,
 * and c<0 then return -1. Else return -2.
 *
 * This function is called whenever curbox is changed.
 * This is particularly useful for derived classes that want to keep track of
 * the last focused text box or view box or whatever.
 */
int SplitWindow::Curbox(int c)
{
	if (c<0 && !curbox) return -1;
	if (c<0 || c>=windows.n) return -2;
	
	DBG cerr <<" splitwindow: set new curbox: "<<c<<"  mode="<<mode<<endl;
	curbox=windows.e[c];
	return c;
}

//! Split the window corresponding to pane index c (or curbox if c<0), put in fillwindow.
/*! If whichside is LAX_RIGHT or LAX_LEFT then split horizontally, else vertically for
 * LAX_TOP or LAX_BOTTOM. Otherwise the default is LAX_BOTTOM. The box is split, and
 * fillwindow is put to the top, bottom, left, or right of the box, according to whichside.
 * If fillwindow==NULL, then a default window is put there.
 *
 * Returns the index in windows of the new part of the split, or a negative number for error.
 * This can be -2 for mode isn't normal (might be maximized or in process for some other mode),
 * or -1 for some other low level mix up.
 * 
 * \todo *** test this
 */
int SplitWindow::Split(int c,int whichside,anXWindow *fillwindow) //fillwindow=NULL
{
	if (mode!=NORMAL) return -2;
	if (c<0) c=windows.findindex(curbox);
	if (c<0 || c>=windows.n) return -1;
	int x,y;
	Curbox(c);
	if (whichside==LAX_TOP || whichside==LAX_BOTTOM) { 
		x=windows.e[c]->x1;
		y=(windows.e[c]->y1+windows.e[c]->y2)/2;
	} else {
		x=(windows.e[c]->x1+windows.e[c]->x2)/2;
		y=windows.e[c]->y1; 
	}
	
	curx=x; cury=y;
	setupsplit(x,y); // sets mode
	if (splitthewindow(fillwindow,whichside)==0) {
		mode=0;
		return windows.n-1;
	}
	mode=0;
	return -1;
}

//! Join pane index c (or curbox if c<0) with the window on whichside (LAX_LEFT, etc).
/*! If c<0, then join curbox. Default is to preserve pane c.
 *
 * Returns the pane index of the remaining window, or -1 if illegal join.
 *
 * \todo *** test this
 */
int SplitWindow::Join(int c,int whichside,char keepother)//keepother=0
{
	if (c<0) c=windows.findindex(curbox);
	if (c<0 || c>=windows.n) return -1;
	int x=0,y=0;
	Curbox(c);
	if (whichside==LAX_LEFT) {
		x=windows.e[c]->x1;
		y=(windows.e[c]->y1+windows.e[c]->y2)/2; 
	} else if (whichside==LAX_RIGHT) {
		x=windows.e[c]->x2;
		y=(windows.e[c]->y1+windows.e[c]->y2)/2; 
	} else if (whichside==LAX_TOP) {
		x=(windows.e[c]->x1+windows.e[c]->x2)/2;
		y=windows.e[c]->y1; 
	} else { //if (whichside==LAX_BOTTOM) {
		x=(windows.e[c]->x1+windows.e[c]->x2)/2;
		y=windows.e[c]->y2;
	}
	if (joinwindow(x,y,keepother)==0) return c;
	return -1;
}

//! Add a new one based on type: return Add(NewWindow(type),whichside);
int SplitWindow::Add(const char *type,unsigned int whichside)
{
	return Add(NewWindow(type),whichside,1);
}

//! This adds a whole section rather than simply splitting windows.
/*! whichside is LAX_LEFT, LAX_RIGHT, LAX_TOP, or LAX_BOTTOM. If whichside
 * is not any of those, then the window is added to the bottom.
 * It is ok to add a NULL window.
 *
 * If there is only one box in the whole window, and it is an empty
 * box (no window in it) then the new window is put in it.
 * Whole window is divided in half, and the existing windows are
 * squished down. The new window is inserted into the new space.
 *
 * If win->window==0 then the window is addwindow'd if this->window!=0.
 *
 * Returns 0 if window is added. Nonzero otherwise.
 *
 * \todo *** TODO: If lots of adding goes on before creating the window, or just after
 *   creating when the dimensions are meaningless, should have some mechanism
 *   to keep the stacking order when the windows are scaled properly!! have a flag
 *   that gets turned off in init?
 * \todo *** when head->window==0 and win is parented elsewhere, does this work correctly?
 */
int SplitWindow::Add(anXWindow *win,unsigned int whichside,int absorbcount)
{
	if (whichside!=LAX_LEFT && whichside!=LAX_RIGHT && whichside!=LAX_TOP && whichside!=LAX_BOTTOM) 
		whichside=LAX_BOTTOM;

	 // If there is one empty window, the new window is dumped into it.
	if (windows.n==0) {
		 //this mysterious bit of code compensates for when win_w of win_h == 0
		int w=win_w,
			h=win_h;
		if (w<1) w=1;
		if (h<1) h=1;
		windows.push(new PlainWinBox(win,0,0,w,h));
	} else if (windows.n==1 && win && windows.e[0]->win()==NULL) {
		windows.e[0]->NewWindow(win);
	} else {
		 // must squish all the others, and put in a new.
		int offx1=0,
			offy1=0,
			offx2=win_w,
			offy2=win_h; //dim and pos of new
		offx1=windows.e[0]->x1;
		offx2=windows.e[0]->x2;
		offy1=windows.e[0]->y1;
		offy2=windows.e[0]->y2;
		PlainWinBox *box;
		for (int c=1; c<windows.n; c++) {
			box=windows.e[c];
			if (box->x1<offx1) offx1=box->x1;
			if (box->x2>offx2) offx2=box->x2;
			if (box->y1<offy1) offy1=box->y1;
			if (box->y2>offy2) offy2=box->y2;
		}
		switch (whichside) {
			case LAX_LEFT: 
				offx1=-win_w;
				offx2=space/2;
				break;
			case LAX_RIGHT: 
				offx1=offx2;
				offx2+=win_w;
				break;
			case LAX_TOP:
				offy1=-win_h;
				offy2=space/2;
				break;
			case LAX_BOTTOM:
				offy1=offy2;
				offy2+=win_h;
				break;
		}
		PlainWinBox *newbox=new PlainWinBox(win, offx1,offy1, offx2,offy2);
		windows.push(newbox);
		scaleWindows();
	}
	if (win) {
		 // need to possibly reparent
		app->reparent(win,this); // app->reparent checks for presence of window, and keeps kids stacks consistent
	}

	if (absorbcount) win->dec_count();

	windows.e[windows.n-1]->sync(space/2,1);
	return 0;
}

/*! \typedef anXWindow *(* NewWindowFunc)(anXWindow *parnt,const char *ntitle,unsigned long style);
 * \ingroup misc
 * \brief Function that returns a new window. Used in SplitWindow.
 */

//! Register a new window making function.
/*! These functions can be used in split and change functions to fill the new windows with
 * specific types of windows.
 *
 * The style gets passed as the window style when calling winfunc. wtype should be the whattype()
 * of the window. ndesc can be a more free form name.
 * 
 * Returns 0 on success, nonzero on failure.
 */
int SplitWindow::AddWindowType(const char *wtype,const char *ndesc,
		unsigned long style,NewWindowFunc winfunc,int settodefault)
{
	int c;
	for (c=0; c<winfuncs.n; c++) if (!strcmp(wtype,winfuncs.e[c]->name)) return 1;
	winfuncs.push(new WinFuncNode(wtype,ndesc,style,winfunc),1);
	if (defaultwinfunc<0) defaultwinfunc=winfuncs.n-1;
	if (settodefault) defaultwinfunc=winfuncs.n-1;
	return 0;
}

//! Scale all the child windows to fit current win_w/h using proportions of oldw/h.
/*! This computes the old bounds of all the panes, then scales them to fit in this
 * window, inset by space/2. If there are any boxes that end up with width or
 * height less then space, then validateX() or validateY() are called. Those
 * functions expand the windows back out. *** not necessary for double panes?
 *
 * This function must be called after the window is resized, because it uses
 * win_w and win_h as the new width and height.
*/
void SplitWindow::scaleWindows()
{
	if (windows.n==0) return;

	float wscale,hscale;
	PlainWinBox *box;
	
	 // find old bounds
	double oldmaxx,oldmaxy,oldminx,oldminy;
	oldminx=windows.e[0]->x1;
	oldmaxx=windows.e[0]->x2;
	oldminy=windows.e[0]->y1;
	oldmaxy=windows.e[0]->y2;
	for (int c=1; c<windows.n; c++) {
		box=windows.e[c];
		if (box->x1<oldminx) oldminx=box->x1;
		if (box->x2>oldmaxx) oldmaxx=box->x2;
		if (box->y1<oldminy) oldminy=box->y1;
		if (box->y2>oldmaxy) oldmaxy=box->y2;
	}
	wscale=(win_w)/(float)(oldmaxx-oldminx);
	hscale=(win_h)/(float)(oldmaxy-oldminy);
	
	int valx=0,valy=0;
	DBG cerr <<"   wscale: "<<wscale<<"   hscale:"<<hscale<<endl;
	for (int c=0; c<windows.n; c++) {
		box=windows.e[c];
		
		DBG cerr <<"===== "<<c<<" before: "<<box->x1<<','<<box->y1<<' '<<box->x2<<','<<box->y2<<"    ";
		box->x1=(int)((box->x1-oldminx)*wscale);
		box->x2=(int)((box->x2-oldminx)*wscale);
		if (box->x2-box->x1<space) valx++;
		
		box->y1=(int)((box->y1-oldminy)*hscale);
		box->y2=(int)((box->y2-oldminy)*hscale);
		if (box->y2-box->y1<space) valy++;
		DBG cerr <<"===== "<<c<<" after: "<<box->x1<<','<<box->y1<<' '<<box->x2<<','<<box->y2<<"    ";
	}
	
	if (valx) validateX();
	if (valy) validateY();
	
	for (int c=0; c<windows.n; c++) {
		if (mode==MAXIMIZED && curbox==windows.e[c]) {
			if (curbox->win()) curbox->win()->MoveResize(space,space,win_w-2*space,win_h-2*space);
		} else {
			windows.e[c]->sync(space/2,0);
		}
	}
	needtodraw=1;
}

/*! \todo ***
 */
void SplitWindow::validateX()
{
	cout <<"  **** SplitWindow::validateX()"<<endl;
}

/*! \todo ***
 */
void SplitWindow::validateY()
{
	cout <<"  **** SplitWindow::validateY()"<<endl;
}

//! MoveResize, calls scaleWindows to perform the scaling.
int SplitWindow::MoveResize(int nx,int ny,int nw,int nh)
{ 
	anXWindow::MoveResize(nx,ny,nw,nh);
	scaleWindows();
	return 0;
}

//! Resize, calls scaleWindows to perform the scaling.
int SplitWindow::Resize(int nw,int nh)
{ 
	// Must proportion the child windows same with the new dimensions
	// must not scale the space though!!!
	
	anXWindow::Resize(nw,nh);
	scaleWindows();
	return 0;
}

} // namespace Laxkit

