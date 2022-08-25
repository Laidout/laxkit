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



#include <lax/rowframe.h>
#include <lax/laxutils.h>



#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

////----------------------------------------- RowFrame ------------------------------------

/*! \class RowFrame
 * \brief A frame that lays out in multiple rows or by columns, but only one control deep.
 * 
 * One can define separate RowFrame windows (or any other window that can be cast
 * to SquishyBox), and Add(them) to achieve nesting.
 * 
 * Windows are added by rows (or columns) together with metric information
 * including the preferred width, as well as the minimum and maximum width,
 * plus the same for height. Adding a NULL window forces a row break. The min/max are
 * never exceeded, thus for small windows, some of them will fall over the edges of
 * the containing window. All this info is stored in a WinFrameBox.
 *
 * If you want the window to wrap to the extent of the boxes, then call WrapToExtent()
 * before init(), perhaps in
 * preinit(), and before adding to a (parent) frame,
 * so that you can use the computed extents for frame/init setup.
 * 
 * \todo make more clear all the layout options and padding, as well as implement them!!
 * \todo  ***beware when deleting child windows, listbox stack becomes out of sync, it may
 *   have pointers to windows that have been delete'd already...
 *   maybe redefine deletekids to detect that?
 * 
 * \code
 *  #define ROWFRAME_COLUMNS     (1<<16) <-- make columns full of boxes (but is a horizontal SquishyBox)
 *  #define ROWFRAME_VERTICAL    (1<<16) <-- make columns full of boxes (but is a horizontal SquishyBox)
 *  #define ROWFRAME_ROWS        (1<<17) <-- make rows full of boxes (but is a vertical SquishyBox)
 *  #define ROWFRAME_HORIZONTAL  (1<<17) <-- make rows full of boxes (but is a vertical SquishyBox)
 *  
 *  #define ROWFRAME_CENTER      (1<<19|1<<22)
 *  #define ROWFRAME_LEFT        (1<<18)
 *  #define ROWFRAME_HCENTER     (1<<19)
 *  #define ROWFRAME_RIGHT       (1<<20)
 *  #define ROWFRAME_TOP         (1<<21)
 *  #define ROWFRAME_VCENTER     (1<<22)
 *  #define ROWFRAME_BOTTOM      (1<<23)
 *  
 *   // these work with COLUMNS/ROWS/HORIZONTAL/VERTICAL to determine
 *   // flags&BOX_FLOW_MASK
 *  #define ROWFRAME_ROWS_TO_TOP    (0<<24)
 *  #define ROWFRAME_ROWS_TO_BOTTOM (1<<24)
 *  #define ROWFRAME_COLS_TO_LEFT   (0<<24)
 *  #define ROWFRAME_COLS_TO_RIGHT  (1<<24)
 *  #define ROWFRAME_ROWS_LR        (0<<25)
 *  #define ROWFRAME_ROWS_RL        (1<<25)
 *  #define ROWFRAME_COLS_TB        (0<<25)
 *  #define ROWFRAME_COLS_BT        (1<<25)
 *
 *   // how to fill gaps between boxes
 *  #define ROWFRAME_STRETCH     (1<<27|1<<28)
 *  #define ROWFRAME_STRETCHX    (1<<27)
 *  #define ROWFRAME_STRETCHY    (1<<28)
 *  #define ROWFRAME_SPACE       (1<<29|1<<30)
 *  #define ROWFRAME_SPACEX      (1<<29)
 *  #define ROWFRAME_SPACEY      (1<<30)
 *  
 *   // these refer to filling extra (width in cols) or (height in rows)
 *  #define ROWFRAME_STRETCH_IN_ROW (1<<31)
 *  #define ROWFRAME_STRETCH_IN_COL (1<<31)
 * \endcode
 *
 */


RowFrame::RowFrame(anXWindow *parnt, unsigned long nstyle, const char *nname)
  : RowFrame(parnt, nname ? nname : "rows", nullptr, nstyle, 0,0,0,0,0, nullptr,0,nullptr)
{
}


 //! RowFrame constructor.
 /*! If the width or the height are 0, then it is implied that the window should size
  *  itself to wrap around the laid out windows, using a huge value (BOX_SHOULD_WRAP) for whichever
  *  size is 0 while RowFrame arranges everything. So for instance, you can specify a specific
  *  width, while height varies, which makes the whole thing laid out like a long page, and
  *  you'd only need a vertical scroller (not provided by RowFrame).
  *
  *  padi is the amount of space between child windows, and bevel is the size of a bevel
  *  that is put around each window.
  *
  *  \todo Deal with ROWFRAME_LAYOUT_MASK and LRTB, etc better...
  */
RowFrame::RowFrame(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long owner,const char *mes,
						int npad //!< The amount of space to put along the borders of the RowFrame
						)
		: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder, prev,owner,mes)
{
	 // numbers this big imply wrap to the extent of the boxes
	 // *** this is accounted for in Sync(?)
//	if (ww==0) { m[1]=BOX_SHOULD_WRAP; flags|=BOX_WRAP_TO_X_EXTENT; }
//	if (hh==0) { m[2]=BOX_SHOULD_WRAP; flags|=BOX_WRAP_TO_Y_EXTENT; }
	
	arrangedstate = 0;
	padinset = npad;
	pad = npad;
//	bevel = 1;
//	padi = 1;

	flags=BOX_DONT_PROPAGATE_POS; // will set flags here manually, not use default RowColBox()
	 //*** set squishy parameters/flags, should rethink this process a bit?
	if (win_style&ROWFRAME_COLUMNS) { flags|=BOX_HORIZONTAL; }
	if (win_style&ROWFRAME_ROWS) { flags|=BOX_VERTICAL; }
	
	if (flags&BOX_VERTICAL) {
		flags|=BOX_STRETCH_TO_FILL_X;
		if (win_style&ROWFRAME_STRETCHY) flags|=BOX_STRETCH_TO_FILL_Y;
		if (win_style&ROWFRAME_STRETCH_IN_ROW) elementflags|=BOX_STRETCH_TO_FILL_Y;
	} else {
		flags|=BOX_STRETCH_TO_FILL_Y;
		if (win_style&ROWFRAME_STRETCHX) flags|=BOX_STRETCH_TO_FILL_X;
		if (win_style&ROWFRAME_STRETCH_IN_ROW) elementflags|=BOX_STRETCH_TO_FILL_X;
	}
	////DBG cerr <<flags<<endl;
	
	if (win_style&ROWFRAME_STRETCHX) elementflags|=BOX_STRETCH_TO_FILL_X; 
	if (win_style&ROWFRAME_STRETCHY) elementflags|=BOX_STRETCH_TO_FILL_Y;
	////DBG cerr <<flags<<endl;

	if (win_style&ROWFRAME_SPACEX) elementflags|=BOX_SPACE_TO_FILL_X;
	if (win_style&ROWFRAME_SPACEY) elementflags|=BOX_SPACE_TO_FILL_Y;
	////DBG cerr <<flags<<endl;
	
	 // justifying applies to elements in row/col, not placement within extra height of rows/width of cols
	if (win_style&ROWFRAME_HCENTER) flags|=BOX_HCENTER;
	else if (win_style&ROWFRAME_LEFT) flags|=BOX_LEFT;
	else if (win_style&ROWFRAME_RIGHT) flags|=BOX_RIGHT;
	////DBG cerr <<flags<<endl;
	
	if (win_style&ROWFRAME_VCENTER) flags|=BOX_VCENTER;
	else if (win_style&ROWFRAME_TOP) flags|=BOX_TOP;
	else if (win_style&ROWFRAME_BOTTOM) flags|=BOX_BOTTOM;
	////DBG cerr <<flags<<endl;

	InstallColors(THEME_Panel);
}

RowFrame::~RowFrame()
{}

//! Default RowFrame init only calls Sync(1).
/*! Normally, derived classes would redefine init to include their child windows,
 * and it's just easier to call Sync(1) from there. RowFrame::init will always only
 * just call Sync(1).
 */
int RowFrame::init()
{
	Sync(1); // usually derived classes will call this explicitly in their own init
	return 0;
}

//! Call Sync(0) if arrangedstate!=1. 
void RowFrame::Refresh()
{
	if (!win_on || !needtodraw || !list.n) return;
	if (arrangedstate!=1) {
		SquishyBox::sync(win_x, win_y, win_w, win_h);
		// Sync(0);
	}
	needtodraw=0;

	// ...draw box around the elements for debugging..
	//DBG for (int c=0; c<list.n; c++)
	//DBG 	draw_rectangle(this, list.e[c]->x(),list.e[c]->y(), list.e[c]->w(),list.e[c]->h());
	//DBG draw_line(this, 0,0,win_w,win_h);
	//DBG draw_line(this, win_w,0, 0,win_h);
}

void RowFrame::sync()
{
	ListBox::sync(); //syncs children
	// if (x() != win_x || y() != win_y || w() != win_w || h() != win_h) {
	// 	MoveResize(x(),y(), w(),h());
	// }
}

void RowFrame::sync(int xx,int yy,int ww,int hh)
{
	SquishyBox::sync(xx,yy,ww,hh);
	MoveResize(xx,yy,ww,hh);
}

//! Sync the boxes to win_w/win_h, which are assumed set already.
/*! Calls sync(), which is SquishyBox::sync() which in turn calls arrangeBoxes(1) 
 * unless a derived class redefines sync().
 * Then, if (add) then app->addwindow on any window boxes that do not have window nonzero.
 *
 * After a Sync, then arrangedstate will be 1.
 */
int RowFrame::Sync(int add) // add=0, if 1 means addwindow
{
	DBG cerr << "RowFrame::Sync..."<<endl;

	if (add) {
		anXWindow *win;
		WinFrameBox *winbox;
		for (int c=0; c<wholelist.n; c++) {
			if (!wholelist.e[c]) continue;
			 // the element might be a box containing a window, or a window derived from SquishyBox
			winbox=dynamic_cast<WinFrameBox *>(wholelist.e[c]);
			if (winbox) win=winbox->win();
			else win=dynamic_cast<anXWindow *>(wholelist.e[c]);
			if (win) app->addwindow(win,1,0); //does nothing if xlib_window already exists...
		}
	}

	//m[0],m[6] are box x,y and should be non zero only for top level boxes (not quite the same as window)
	w(win_w?win_w:BOX_SHOULD_WRAP); // the BOX_SHOULD_WRAP forces a wrap to extent
	h(win_h?win_h:BOX_SHOULD_WRAP);
	sync(); //this is supposed to map dimensions and coords of all subwindows

	if (w() != win_w || h() != win_h) {
		MoveResize(x(),y(), w(),h());
	}

	arrangedstate=1;
	return 0;
}

SquishyBox *RowFrame::findBox(anXWindow *win)
{
	int c;
	anXWindow *w;
	WinFrameBox *wf;
	for (c=0; c<wholelist.n; c++) {
		w=dynamic_cast<anXWindow *>(wholelist.e[c]);
		if (w && win==w) return wholelist.e[c];
		
		wf=dynamic_cast<WinFrameBox *>(wholelist.e[c]);
		if (wf && wf->win()) {
			if (win==wf->win()) return wholelist.e[c];
		}
	}
	return NULL;
}

//! Return the window in RowColBox::wholelist that has name as its win_name or win_title.
anXWindow *RowFrame::findWindow(const char *name)
{
	int c;
	anXWindow *w;
	WinFrameBox *wf;
	for (c=0; c<wholelist.n; c++) {
		w=dynamic_cast<anXWindow *>(wholelist.e[c]);
		if (w) {
			if (w->win_name  && !strcmp(name,w->win_name))  return w;
			if (w->win_title && !strcmp(name,w->win_title)) return w;
		}
		
		wf=dynamic_cast<WinFrameBox *>(wholelist.e[c]);
		if (wf && wf->win()) {
			w=wf->win();
			if (w->win_name  && !strcmp(name,w->win_name))  return w;
			if (w->win_title && !strcmp(name,w->win_title)) return w;
		}
	}
	return NULL;
}

/*! Return the anXWindow associated with index. If none, return NULL.
 */
anXWindow *RowFrame::findWindowFromIndex(int index)
{
	if (index<0 || index>=wholelist.n) return NULL;

	anXWindow *w=dynamic_cast<anXWindow *>(wholelist.e[index]);
	if (w) return w;
	
	WinFrameBox *wf=dynamic_cast<WinFrameBox *>(wholelist.e[index]);
	if (wf) return wf->win();

	return NULL;
}

//! Return the index in RowColBox::wholelist containing a window with name as the title.
/*! The returned index can be used to position new windows with AddNull(), and AddWin().
 */
int RowFrame::findWindowIndex(const char *name)
{
	int c;
	anXWindow *w;
	WinFrameBox *wf;
	for (c=0; c<wholelist.n; c++) {
		w=dynamic_cast<anXWindow *>(wholelist.e[c]);
		if (w) {
			if (w->win_name  && !strcmp(name,w->win_name))  return c;
			if (w->win_title && !strcmp(name,w->win_title)) return c;
		}
		
		wf=dynamic_cast<WinFrameBox *>(wholelist.e[c]);
		if (wf && wf->win()) {
			w=wf->win();
			if (w->win_name  && !strcmp(name,w->win_name))  return c;
			if (w->win_title && !strcmp(name,w->win_title)) return c;
		}
	}
	return -1;
}

//! Call this to insert a line break into the window stack.
/*! Return value is 0, and really means nothing. */
int RowFrame::AddNull(int where) //where=-1
{
	Push(NULL,0,where);
	return 0;
}

//! Add a window using the window's width and height with no squishability.
/*! Reparents if win->win_parent!=this.
 * If the window is derived from SquishyBox, then pass it on to SquishyBox::Push(win,3,where).
 * 
 * The end result is for win to be pushed onto position where of wholelist.
 * where==-1 means to top.
 * 
 * Returns 0 on success, else nonzero 
 */
int RowFrame::AddWin(anXWindow *win,int absorbcount,int where)//where=-1
{
	if (!win) return AddNull(where);
	if (win->win_parent!=this) app->reparent(win,this);
	SquishyBox *s=dynamic_cast<SquishyBox *>(win);
	if (s) { 
		Push(s,3,where);
		if (absorbcount) win->dec_count();
		return 0; 
	}
	return AddWin(win,absorbcount, win->win_w,0,0,50,0, win->win_h,0,0,50,0, where);
}


int RowFrame::AddHSpacer(int npw, int where)
{
	return AddWin(NULL,0, npw,0,0,50,0, 0,0,0,0,0, where);
}

int RowFrame::AddHSpacer(int npw,int nws,int nwg,int nhalign, int where)
{
	return AddWin(NULL,0, npw,nws,nwg,nhalign,0, 0,0,0,0,0, where);
}

int RowFrame::AddVSpacer(int npw,int nws,int nwg,int nhalign, int where)
{
	return AddWin(NULL,0, 0,0,0,0,0, npw,nws,nwg,nhalign,0, where);
}

int RowFrame::AddSpacer(int npw,int nws,int nwg,int nhalign,
						  int nph,int nhs,int nhg,int nvalign,
						  int where)
{
	return AddWin(NULL,0, npw,nws,nwg,nhalign,0, nph,nhs,nhg,nvalign,0, where);
}

//! Add a window with squish values.
/*! Return 0 on success, else non zero error code.
 * 
 * The end result is for win to be pushed onto the top of wholelist.
 */
int RowFrame::AddWin(anXWindow *win,      //!< The window to add
					int absorbcount,      //!< Whether to absorb one count of win
					int npw,              //!< New preferred width
					int nws,              //!< New horizontal shrink
					int nwg,              //!< New horizontal grow
					int nhalign,          //!< New horizontal align, 0=bottom/right, 100=top/left, 50=center, etc.
					int nhgap,            //!< New horizontal gap
					int nph,              //!< New preferred height
					int nhs,              //!< New vertical shrink
					int nhg,              //!< New vertical grow
					int nvalign,          //!< New vertical align, 0=bottom/right, 100=top/left, 50=center, etc.
					int nvgap,            //!< New vertical gap
					int where             //!< Where in wholelist to add the window
					) 
{
	WinFrameBox *wf;
//	WinFrameBox(anXWindow *nwin,
//				int nx,int nw,int npw,int nws,int nwg,int nhalign,  
//				int ny,int nh,int nph,int nhs,int nhg,int nvalign);
	wf=new WinFrameBox(win, 0,npw,npw,nws,nwg,nhalign,nhgap, 0,nph,nph,nhs,nhg,nvalign,nvgap);
	if (win) {
		wf->pad=win->WindowBorder();
		if (win->win_parent!=this) app->reparent(win,this);
		if (absorbcount) win->dec_count();
	}
	Push(wf,1,where);
	needtodraw=1;
	return 0;
}

//! Add an already made WinFrameBox
/*! This is really just a convenience function for any
 * derived classes. It is not really necessary to create a whole
 * box just in order to add a window.
 * 
 * The end result is for win to be pushed onto the top of wholelist.
 *
 * Generally islocal will be 0 or 1 depending on whether we need to
 * take position of the reference or not. This value is ultimately
 * fed into a RefPtrStack.
 */
int RowFrame::AddWin(WinFrameBox *box,char islocal,int where)//where=-1 
{ 
	if (!box) return 1;
	if (box->win() && box->win()->win_parent!=this) app->reparent(box->win(),this);
	Push(box,islocal,where);
	return 0;
}
	
/*! Pop a box.
 * Return a reference if popped!=NULL. The window is not destroyed in this case.
 * If popped==NULL, then destroy the window.
 */
int RowFrame::Pop(int which, anXWindow **popped)
{
	int er=0;
	if (which<0 || which>=wholelist.n) which=wholelist.n-1;
	if (which==-1) return 1;

	WinFrameBox *w=dynamic_cast<WinFrameBox*>(wholelist.e[which]);
	if (w && w->win()) {
		if (popped) {
			*popped=w->win();
		} else {
			app->destroywindow(w->win());
		}
	}

	er=wholelist.remove(which);
	arrangedstate=0;
	return er;
}

int RowFrame::Pop(int which)
{
	return Pop(which,NULL);
}


//! Calls anXWindow::Resize, then Sync(0).
int RowFrame::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	arrangedstate=0;
	needtodraw=1;
	return 0;
}

//! Calls anXWindow::MoveResize, then Sync(0).
int RowFrame::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	arrangedstate=0;
	needtodraw=1;
	return 0;
}




} // namespace Laxkit


