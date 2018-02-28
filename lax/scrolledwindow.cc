//
//	
//    The Laxkit, a windowing toolkit
//    Copyright (C) 2004-2006,2010 by Tom Lechner
//    Please consult http://laxkit.sourceforge.net about where to send any
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
//    correspondence about this software.
//

#include <lax/scrolledwindow.h>

namespace Laxkit {

/*! \class ScrolledWindow
 * \brief Class to have a window associated with a vertical and horizontal scroller.
 *
 * This is a somewhat simple container designed to have ONLY scrollers, a single main window,
 * and an optional PanPopup in the corner. 
 * The scrollers can come and go or be permanent.
 *
 * \todo *** should have option to either resize thewindow to fit within the space not taken
 * by scrollers, and pass on panchange events to
 * that window, OR have thewindow be just a huge window that the scrollbars automatically
 * move around!!! That would be quite useful for containing dialogs....
 * 
 * \code
 *  #include <lax/scrolledwindow.h>
 * 
 *   // If you do not specify TOP or BOTTOM, then the horizontal scroller is not included.
 *   // Likewise for LEFT and RIGHT regarding the vertical scroller.
 *   // Put an x scroller on either top or bottom
 *  #define SW_TOP         (1<<16)
 *  #define SW_BOTTOM      (1<<17)
 *   // Put a y scroller on either left or right
 *  #define SW_LEFT        (1<<18)
 *  #define SW_RIGHT       (1<<19)
 *   // inlude a PanWindow in corner between the scrollers
 *  #define SW_INCLUDE_PAN (1<<20)
 *   // Always have x or y scrollers, they do not go away when not needed.
 *  #define SW_ALWAYS_X    (1<<21)
 *  #define SW_ALWAYS_Y    (1<<22)
 *   // Allow scrolling in the x or y direction
 *  #define SW_X_ZOOMABLE  (1<<23)
 *  #define SW_Y_ZOOMABLE  (1<<24)
 *   // When zooming, preserve the aspect ratio of the boxed area.
 *  #define SW_SYNC_XY     (1<<25)
 *   // Move around a subwindow in response to scroll events,
 *   // Otherwise default is to simply relay pan changes to thewindow.
 *  #define SW_MOVE_WINDOW (1<<26)
 * \endcode
 */
/*! \var IntRectangle ScrolledWindow::inrect
 * \brief The rectangle that thewindow resides in.
 *
 * If thewindow is NULL, then derived classes can use this as the area to do their thing in.
 * It corresponds also to the selbox of the panner.
 */
/*! \var IntRectangle ScrolledWindow::outrect
 * \brief The rectangle that holds all of thewindow and scrollers.
 *
 * Default is the whole window.
 * Derived classes can redefine findoutrect() to modify outrect to 
 * not include any space that they want to use.
 */


/*! ****maybe should not have pan inputable here.. should allow inputing spacerect bounds, and/or thewindow...
 */
ScrolledWindow::ScrolledWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend,
		PanController *pan)
	: PanUser(pan),
	  anXWindow (parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	scrollwidth=15;
	thewindow=NULL;
	xscroller=yscroller=NULL;
	panpopup=NULL;
}

//! Send what? huh?***
int ScrolledWindow::send()
{
	return 0;
}

//! Create the scrollers and panpopup. Add them to panner tell stack.
/*! Derived classes or other functions can create their own scrollers or panpopup
 * before this function is called. They are created fresh only if they do not
 * exist yet. In either case they are pushed (pushnodup) onto the panner tell stack.
 */
int ScrolledWindow::init()
{
	//Scroller(anXWindow *parnt,const char *ntitle, unsigned long nstyle,
	//				int nx,int ny,int nw,int nh,unsigned int nbrder,
	//				anXWindow *prev,Window nowner,const char *mes,int nid,
	//				PanController *npan,
	//				long nmins=0,long nmaxs=0,long nps=0,long nes=0,long ncp=-1,long ncpe=-1); // ncp=ncpe=-1

	 //***got to improve panner api
	panner->tell(this);
//	panner->SetStuff(1, -5*win_w,5*win_w, 90,5, -win_w,win_w);
//	panner->SetStuff(2, -5*win_h,5*win_h, 90,5, -win_h,win_h);
//	panner->SetSpace(-5*win_w,5*win_w,-5*win_h,5*win_h);
//	panner->SetCurPos(1,-win_w,win_w);
//	panner->SetCurPos(2,-win_h,win_h);
	panner->SetBoxAspect(win_w-scrollwidth,win_h-scrollwidth);

	if (win_style&(SW_TOP|SW_BOTTOM)) {
		if (!xscroller) xscroller=new Scroller(this,"sw-xscroller",NULL,
							SC_XSCROLL|SC_ABOTTOM|(win_style&SW_X_ZOOMABLE?SC_ZOOM:0),
							0,(win_style&SW_TOP?0:win_h-scrollwidth), win_w,scrollwidth, 0,
							NULL,0,"xscroll",
							panner,0);
		app->addwindow(xscroller);
	}
	if (win_style&(SW_LEFT|SW_RIGHT)) {
		if (!yscroller) yscroller=new Scroller(this,"sw-yscroller",NULL,
							SC_YSCROLL|SC_ABOTTOM|(win_style&SW_X_ZOOMABLE?SC_ZOOM:0),  //*** abottom adjustable??
							(win_style&SW_RIGHT?win_w-scrollwidth:0),(win_style&SW_TOP?scrollwidth:0), //x,y
							scrollwidth,win_h-scrollwidth, 0, //w,h,border
							NULL,0,"yscroll",
							panner,0);
		app->addwindow(yscroller);
	}
	if (win_style&SW_INCLUDE_PAN && !panpopup && (yscroller || xscroller)) {
		panpopup=new PanPopup(this,"sw-panpopup",NULL,0, 0,0,0,0, 0, NULL,0,NULL, panner);
		app->addwindow(panpopup);
	}
	if (xscroller) panner->tell(xscroller);
	if (yscroller) panner->tell(yscroller);
	if (panpopup)  panner->tell(panpopup);
	syncWindows();
	return 0;
}

//! Use nwindow as the nested window.
/*! Any old window must be disposed of, and nwindow installed, which
 * means reparenting to this, and setting where messages should go.
 *
 * Returns 0 on success. If nwindow is NULL, nothing is done and 1 is returned.
 * 
 * The old window is app->destroywindow'd.
 */
int ScrolledWindow::UseThisWindow(anXWindow *nwindow)
{
	if (!nwindow) return 1;
	if (thewindow) {
		panner->tellPop(thewindow);
		app->destroywindow(thewindow);
	}
	thewindow=nwindow;
	panner->tell(thewindow);
	app->reparent(thewindow,this); // app takes care of whether kid/parent->windows exist yet.
	syncWindows();
	return 0;
}

/*! \todo *** this must respond to panner events and pass them on to the subwindows...
 */
int ScrolledWindow::Event(const EventData *e,const char *mes)
{
	return anXWindow::Event(e,mes);
}

//! Define the area that the scrollers and thewindow should go in.
/*! Default is the whole window.
 * Derived classes can redefine this to remove any space that they want to use.
 *
 * Scrollers are carved out of outrect to produce inrect, and then adjustinrect()
 * is called to allow subclasses to further reduce actual item drawing area.
 */
void ScrolledWindow::findoutrect()
{
	outrect.x=0;
	outrect.y=0;
	outrect.width= win_w;
	outrect.height=win_h;
}

//! Called as final command in syncWindows.
/*! Does nothing. Derived classes can redefine to further reduce inrect
 * if they wish.
 *
 * inrect is where the main window contents should be drawn into. It is
 * a subset of the whole window, minus the space reserved for scrollbars.
 */
void ScrolledWindow::adjustinrect() {}
	
//! MoveResize the scrollers, panwindow, and thewindow.
/*! Calls findoutrect(), then syncs the windows, then sets inrect, and finally
 * calls adjustinrect().
 *
 * \todo parameter useinrect is not used, in fact, i can't remember what I put it there for...
 * maybe it'll come back to me..
 *
 * \todo *** window shifting rather than relay messages is not implemented yet
 */
void ScrolledWindow::syncWindows(int useinrect)//useinrect==0
{
	//***get rect to put the goodies in.. derived classes might want to take up some real estate....
	
	findoutrect();
	inrect = outrect;
	int x, y, w,h;

	 // yscroller is placed first
	if (yscroller && yscroller->win_on) {
		x = (win_style & SW_LEFT) ? outrect.x : outrect.x + outrect.width - scrollwidth;
		w = scrollwidth;
		if (panpopup && panpopup->win_on) {
			h = outrect.height - scrollwidth;
			if (win_style & SW_TOP) y = outrect.y + scrollwidth;
			else y = outrect.y;
		} else { y = outrect.y; h = outrect.height; }
		yscroller->MoveResize(x,y,w,h);
	}
	
	 // xscroller placed inset by yscroller
	if (xscroller && xscroller->win_on) {
		y = (win_style & SW_TOP) ? 0 : outrect.y + outrect.height - scrollwidth;
		h = scrollwidth;
		if ((yscroller && yscroller->win_on) || (panpopup && panpopup->win_on)) {
			w = outrect.width - scrollwidth;
			if (win_style&SW_LEFT) x = outrect.x + scrollwidth;
			else x = outrect.x;
		} else { x = outrect.x; w = outrect.width; }
		xscroller->MoveResize(x,y,w,h);
	}

	 // the scroller positioning above already took into account existence of panpopup..
	if (panpopup && panpopup->win_on) {
		if (win_style & SW_TOP)  y = outrect.y; else y = outrect.y + outrect.height - scrollwidth;
		if (win_style & SW_LEFT) x = outrect.x; else x = outrect.x + outrect.width  - scrollwidth;
		panpopup->MoveResize(x,y,scrollwidth,scrollwidth);
	}

	 // define inrect
	if (yscroller && yscroller->win_on) {
		w = outrect.width - scrollwidth;
		if (win_style & SW_LEFT) x = outrect.x + scrollwidth; else x = outrect.x;
	} else { x = outrect.x;  w = outrect.width; }
	if (xscroller && xscroller->win_on) {
		h = outrect.height - scrollwidth;
		if (win_style & SW_TOP) y = outrect.y + scrollwidth; else y = outrect.y;
	} else { y = outrect.y;  h = outrect.height; }

	inrect.x      = x;
	inrect.y      = y;
	inrect.width  = w;
	inrect.height = h;
	adjustinrect();

	 // Sync up thewindow, if present
	if (thewindow) {
		if (!(win_style&SW_MOVE_WINDOW)) {
			w -= thewindow->win_border*2;
			h -= thewindow->win_border*2;
			thewindow->MoveResize(x,y,w,h);
			panner->SetBoxAspect(thewindow->win_w,thewindow->win_h);
		} else { //*** just move the window around, no resizing required.?? tell panner what?
		}
	} else {
		panner->SetBoxAspect(inrect.width,inrect.height);
	}
}

//! Just calls anXWindow::MoveResize, then syncWindows().
int ScrolledWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	syncWindows();
	return 0;
}

//! Just calls anXWindow::Resize, then syncWindows().
int ScrolledWindow::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	syncWindows();
	return 0;
}


} // namespace Laxkit

