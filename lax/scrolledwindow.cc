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
//    Copyright (C) 2009,2010 by Tom Lechner
//

#include <lax/scrolledwindow.h>
#include <lax/displayer.h>

#include <iostream>
#define DBG
using namespace std;

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
 * move around.
 */
/*! \var IntRectangle ScrolledWindow::inrect
 * \brief The rectangle that thewindow resides in.
 *
 * If thewindow is nullptr, then derived classes can use this as the area to do their thing in.
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
		anXWindow *the_window,
		PanController *pan)
	: PanUser(pan),
	  anXWindow (parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	scrollwidth = 15;
	thewindow = nullptr;
	xscroller = yscroller = nullptr;
	panpopup = nullptr;

	if (the_window) UseThisWindow(the_window);
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

	panner->tell(this);
//	panner->SetStuff(1, -5*win_w,5*win_w, 90,5, -win_w,win_w);
//	panner->SetStuff(2, -5*win_h,5*win_h, 90,5, -win_h,win_h);
//	panner->SetSpace(-5*win_w,5*win_w,-5*win_h,5*win_h);
//	panner->SetCurPos(1,-win_w,win_w);
//	panner->SetCurPos(2,-win_h,win_h);
	// panner->SetBoxAspect(win_w-scrollwidth,win_h-scrollwidth);

	// if (HasWinStyle(SW_MOVE_WINDOW)) {
	// 	if (thewindow) {
	// 		panner->SetWholebox(0,thewindow->win_w, 0,thewindow->win_h);
	// 		panner->SetSelection(0,inrect.width, 0,inrect.height);
	// 	}
	// }

	if (win_style&(SW_TOP|SW_BOTTOM)) {
		if (!xscroller) xscroller = new Scroller(this,"sw-xscroller",nullptr,
							SC_XSCROLL | SC_ABOTTOM | (win_style & SW_X_ZOOMABLE ? SC_ZOOM : 0),
							0,(win_style & SW_TOP ? 0 : win_h-scrollwidth), win_w,scrollwidth, 0,
							nullptr,0,"xscroll",
							panner,0);
		app->addwindow(xscroller);
	}
	if (win_style&(SW_LEFT|SW_RIGHT)) {
		if (!yscroller) yscroller=new Scroller(this,"sw-yscroller",nullptr,
							SC_YSCROLL | SC_ABOTTOM | (win_style & SW_X_ZOOMABLE ? SC_ZOOM : 0),  //*** abottom adjustable??
							(win_style & SW_RIGHT ? win_w-scrollwidth : 0),(win_style & SW_TOP ? scrollwidth : 0), //x,y
							scrollwidth,win_h-scrollwidth, 0, //w,h,border
							nullptr,0,"yscroll",
							panner,0);
		app->addwindow(yscroller);
	}
	if (win_style&SW_INCLUDE_PAN && !panpopup && (yscroller || xscroller)) {
		panpopup=new PanPopup(this,"sw-panpopup",nullptr,0, 0,0,0,0, 0, nullptr,0,nullptr, panner);
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
 * Returns 0 on success. If nwindow is nullptr, nothing is done and 1 is returned.
 * 
 * The old window is app->destroywindow'd.
 */
int ScrolledWindow::UseThisWindow(anXWindow *nwindow)
{
	if (!nwindow) return 1;
	if (thewindow && thewindow != nwindow) {
		panner->tellPop(thewindow);
		app->destroywindow(thewindow);
	}

	thewindow = nwindow;
	panner->tell(thewindow);
	// panner->dontTell(this);
	panner->SetWholeboxOnly(0, thewindow->win_w, 0, thewindow->win_h, false, false);
	// panner->dontTell(nullptr);
	app->reparent(thewindow,this); // app takes care of whether kid/parent->windows exist yet.
	syncWindows();
	return 0;
}

int ScrolledWindow::Event(const EventData *e,const char *mes)
{
	if (!strcmp(mes, "pan change")) {
		syncWindows();
	}

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
	outrect.x = 0;
	outrect.y = 0;
	outrect.width  = win_w;
	outrect.height = win_h;
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
/*! Calls findoutrect(), then sets the scrollers, then sets inrect, 
 * calls adjustinrect(), then sets thewindow position.
 */
void ScrolledWindow::syncWindows()
{
	// get rect to put the goodies in.. derived classes might want to take up some real estate....
	findoutrect();
	inrect = outrect;
	
	int x = outrect.x, y = outrect.y, w = outrect.width, h = outrect.height;
	bool xon = false, yon = false;

	// figure out if scroller should be on or off
	if (win_style & SW_MOVE_WINDOW) {
		if (!thewindow) {
			if (xscroller && xscroller->win_on) { xon = false; }
			if (yscroller && yscroller->win_on) { yon = false; }

		} else {
			if (thewindow->win_w >= outrect.width - scrollwidth) {
				if (xscroller) xon = true;
			} else if (xscroller) xon = false;

			if (thewindow->win_h >= outrect.height - scrollwidth) {
				if (yscroller) yon = true;
			} else if (yscroller) yon = false;
		}
		if (HasWinStyle(SW_ALWAYS_X)) xon = true;
		if (HasWinStyle(SW_ALWAYS_Y)) yon = true;

	} else {
		// pan events get relayed to window, but thewindow doesn't actually move
		xon = (xscroller && xscroller->win_on);  // <- ***FIXME! win_on not set yet!! has to wait for MapNotify!!
		yon = (yscroller && yscroller->win_on);
	}


	// yscroller is placed first
	if (yscroller) {
		if (yon) {
			x = (win_style & SW_LEFT) ? outrect.x : outrect.x + outrect.width - scrollwidth;
			w = scrollwidth;
			if (panpopup && panpopup->win_on) {
				h = outrect.height - scrollwidth;
				if (win_style & SW_TOP) y = outrect.y + scrollwidth;
				else y = outrect.y;
			} else { y = outrect.y; h = outrect.height; }

			if (!yscroller->win_on) app->mapwindow(yscroller);
			yscroller->MoveResize(x,y,w,h);
		
		} else {
			app->unmapwindow(yscroller);
		}
	}
	
	// xscroller placed inset by yscroller
	if (xscroller) {
		if (xon) {
			y = (win_style & SW_TOP) ? 0 : outrect.y + outrect.height - scrollwidth;
			h = scrollwidth;
			if ((yscroller && yscroller->win_on) || (panpopup && panpopup->win_on)) {
				w = outrect.width - scrollwidth;
				if (win_style&SW_LEFT) x = outrect.x + scrollwidth;
				else x = outrect.x;
			} else { x = outrect.x; w = outrect.width; }

			if (!yscroller->win_on) app->mapwindow(yscroller);
			xscroller->MoveResize(x,y,w,h);
		
		} else {
			app->unmapwindow(xscroller);
		}
	}

	// the scroller positioning above already took into account existence of panpopup..
	if (panpopup && panpopup->win_on) {
		if (win_style & SW_TOP)  y = outrect.y; else y = outrect.y + outrect.height - scrollwidth;
		if (win_style & SW_LEFT) x = outrect.x; else x = outrect.x + outrect.width  - scrollwidth;
		panpopup->MoveResize(x,y,scrollwidth,scrollwidth);
	}

	// define inrect.. note this is done after scroller positioning above
	if (yscroller && yon) {
		w = outrect.width - 1.5*scrollwidth;
		if (win_style & SW_LEFT) x = outrect.x + scrollwidth; else x = outrect.x;
	} else { x = outrect.x;  w = outrect.width; }

	if (xscroller && xon) {
		h = outrect.height - scrollwidth;
		if (win_style & SW_TOP) y = outrect.y + scrollwidth; else y = outrect.y;
	} else { y = outrect.y;  h = outrect.height; }

	inrect.x      = x;
	inrect.y      = y;
	inrect.width  = w;
	inrect.height = h;
	adjustinrect();
	// where thewindow has to go:
	x = inrect.x     ;
	y = inrect.y     ;
	w = inrect.width ;
	h = inrect.height;

	 // Sync up thewindow, if present
	if (thewindow) {
		if (HasWinStyle(SW_MOVE_WINDOW)) {
			//thewindow is wholebox and slides around in the scrollwindow
			
			panner->SetSelectionSize(inrect.width, inrect.height, false);
			//---
			// panner->SetSelection(panner->start[0], panner->start[0]+win_w, panner->start[1], panner->start[1]+win_h);
			panner->pagesize[1] = inrect.height*2/3;
			if (panner->pagesize[1] < 1) panner->pagesize[1] = 1;

			AdjustTheWindow();
			panner->validateSelbox();

			//TODO: should be optional to resize x instead of always map to inrect.width
			thewindow->MoveResize(x-(panner->start[0]-panner->min[0]), y-(panner->start[1]-panner->min[1]),
					inrect.width, thewindow->win_h);
					// thewindow->win_w < inrect.width ? inrect.width : win_w, thewindow->win_h);

		} else {
			w -= thewindow->WindowBorder()*2;
			h -= thewindow->WindowBorder()*2;
			thewindow->MoveResize(x,y,w,h);
			panner->SetBoxAspect(thewindow->win_w,thewindow->win_h);
		}
	} else {
		panner->dontTell(this);
		panner->SetBoxAspect(inrect.width,inrect.height);
		panner->dontTell(nullptr);
	}
}

/*! Mainly for `win_style & SW_MOVE_WINDOW`, make any adjustments on thewindow right
 * after a new inrect has been found. This might happen, for instance, if thewindow is a RowFrame,
 * and needs to reflow to a new win_w, which causes a new win_h.
 *
 * Default here does nothing.
 */
void ScrolledWindow::AdjustTheWindow()
{}

//! Just calls anXWindow::MoveResize, then syncWindows().
int ScrolledWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);

	// if (win_style & SW_MOVE_WINDOW) {
	// 	panner->SetSelection(panner->start[0], panner->start[0]+win_w, panner->start[1], panner->start[1]+win_h);
	// 	panner->pagesize[1] = win_h*2/3;
	// }

	syncWindows();
	return 0;
}

//! Just calls anXWindow::Resize, then syncWindows().
int ScrolledWindow::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);

	// if (win_style & SW_MOVE_WINDOW) {
	// 	panner->SetSelection(panner->start[0], panner->start[0]+win_w, panner->start[1], panner->start[1]+win_h);
	// }

	syncWindows();
	return 0;
}

/*! Scroll the contents */
int ScrolledWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!panner) return 1;
	panner->OneUp(2);
	return 0;
}

/*! Scroll the contents */
int ScrolledWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!panner) return 1;
	panner->OneDown(2);
	return 0;
}

void ScrolledWindow::Refresh()
{
	if (first) {
		first = false;
	}
	needtodraw = 0;
}


} // namespace Laxkit

