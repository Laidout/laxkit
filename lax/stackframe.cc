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


#include <lax/stackframe.h>
#include <lax/laxutils.h>
#include <lax/mouseshapes.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//-------------------------------- StackFrame -------------------------

/*! \class StackFrame 
 * \brief A frame for a stack of windows that can be sized in one dimension.
 *
 * This is analogous to GtkPaned frames in Gtk. StackFrame is a little more
 * robust by allowing any number of windows, not just two, and is best thought
 * of as a sort of 1-d SplitWindow.
 *
 * There is a bar between each pane that can be dragged around. If you drag a bar
 * right next to the edge of an adjacent window, that window becomes hidden, and
 * you will see 2 bars next to each other. To make it visible again, just drag out 
 * the bar again.
 *
 * \todo Currently, the window is not technically hidden, it is merely squashed to
 *   a width of 1. would be better to have real hidden capability so applications can
 *   respond with special behavior.
 *
 * The total width of the window will be:\n
 *   (sum of widths of panes) + gap*(number of panes-1)
 *
 * Window styles. If STACKF_BEVEL, then draw a bevel around each box. Otherwise,
 * the default is to draw some dots in the middle of each box, and have no
 * special border around the boxes.
 *
 * If NOT_SIZEABLE, then you cannot shift the bars around.
 * <pre>
 *  #define STACKF_VERTICAL      (1<<16)
 *  #define STACKF_NOT_SIZEABLE  (1<<17)
 *  #define STACKF_ALLOW_SWAP    (1<<18)
 *  #define STACKF_BEVEL         (1<<19)
 * </pre>
 *
 * \todo bevel should draw around whole window, not just in the crack
 */
/*! \var double *StackFrame::pos
 * \brief Positions from 0 to 1.0 of draggable bars.
 *
 * There will be the same number of elements in pos as there are child boxes.
 * pos[numboxes-1]==1.0.
 */


StackFrame::StackFrame(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend,
		int ngap)
	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	installColors(app->color_panel);

	if (win_style&STACKF_VERTICAL) flags=(flags&~BOX_HORIZONTAL)|BOX_VERTICAL;
	else flags=(flags&~BOX_VERTICAL)|BOX_HORIZONTAL;

	gap=ngap/2*2;
	if (gap<0) gap=0;

	pos=NULL;
	whichbar=-1;

	curshape=0;
	//if (win_style&STACKF_VERTICAL) win_pointer_shape=LAX_MOUSE_UpDown;
	//else win_pointer_shape=LAX_MOUSE_LeftRight;
}

StackFrame::~StackFrame()
{
	if (pos) delete[] pos;
}

/*! Default just calls anXWindow::init().
 *
 * \todo need enter/leave cursor adjustment, or shade bar so you know you can move it
 */
int StackFrame::init()
{
	if (!pos) UpdatePos(1);
	anXWindow::init();
	Sync(1);

	return 0;
}

void StackFrame::sync()
{
	DBG cerr <<"StackFrame::sync()"<<endl;

	if (!pos) UpdatePos(1);
	MoveResize(x(),y(), w(),h());
	Sync(0);
	//ListBox::sync();
}

//! Return pointer to the childe window at index.
anXWindow *StackFrame::childWindow(int index)
{
	if (index<0 || index>=list.n) return NULL;

	WinFrameBox *winbox=NULL;

	 // the element might be a box containing a window, or a window derived from SquishyBox
	winbox=dynamic_cast<WinFrameBox *>(list.e[index]);
	if (winbox) return winbox->win();

	return dynamic_cast<anXWindow *>(list.e[index]);
}

void StackFrame::Refresh()
{
	needtodraw=0;
	if (!list.n || gap<=0) return;

	Displayer *dp=MakeCurrent();

	unsigned long highlight=coloravg(win_colors->bg,rgbcolor(255,255,255));
	unsigned long shadow   =coloravg(win_colors->bg,rgbcolor(0,0,0));
	unsigned long dots     =coloravg(win_colors->bg,win_colors->fg);
	
	dp->ClearWindow();

	SquishyBox *b;
	for (int c=0; c<list.n; c++) {
		b=list.e[c];

		if (win_style&STACKF_BEVEL) {
			dp->drawBevel(gap/2, highlight,shadow, LAX_OFF, b->x()-gap/2,b->y()-gap/2, b->w()+gap,b->h()+gap);

		} else {
			 //draw little dots on draggable bars if gap>0
			dp->NewFG(dots);
			if (c==list.n-1) continue;

			int p;
			if (win_style&STACKF_VERTICAL) {
				p=(int)(pos[c]*win_h);
				dp->drawthing(win_w/2  ,p, 2,2, 1,THING_Circle);
				dp->drawthing(win_w/2-5,p, 2,2, 1,THING_Circle);
				dp->drawthing(win_w/2+5,p, 2,2, 1,THING_Circle);
			} else {
				p=(int)(pos[c]*win_w);
				dp->drawthing(p,win_h/2  , 2,2, 1,THING_Circle);
				dp->drawthing(p,win_h/2-5, 2,2, 1,THING_Circle);
				dp->drawthing(p,win_h/2+5, 2,2, 1,THING_Circle);
			}
		} 
	}
}

int StackFrame::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onMouseOut) {
		if (curshape!=0) {
			const EnterExitData *ee=dynamic_cast<const EnterExitData*>(e);
			dynamic_cast<LaxMouse*>(ee->device)->setMouseShape(this,0);
			curshape=0;
		}
	} else if (e->type==LAX_onMouseIn) {
		const EnterExitData *ee=dynamic_cast<const EnterExitData*>(e);
		int whichbar=findWhichBar(ee->x,ee->y);
		if (whichbar>=0) {
			if (win_style&STACKF_VERTICAL) curshape=LAX_MOUSE_UpDown;
			else curshape=LAX_MOUSE_LeftRight;
		} else curshape=0;

		dynamic_cast<LaxMouse*>(ee->device)->setMouseShape(this,curshape);
	}

	return anXWindow::Event(e,mes);
}

int StackFrame::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (list.n<=1) return 0;

	whichbar=findWhichBar(x,y);
	buttondown.down(d->id,LEFTBUTTON,x,y,whichbar);

	return 0;
}

//! Return the index in pos of the bar under (x,y) or -1.
int StackFrame::findWhichBar(int x,int y)
{
	if (win_style&STACKF_VERTICAL) {
		for (int c=0; c<list.n; c++) {
			if (y>=list.e[c]->y()-list.e[c]->pad-gap && y<list.e[c]->y()-list.e[c]->pad)
				return c-1;
		}
	} else {
		for (int c=1; c<list.n; c++) {
			if (x>=list.e[c]->x()-list.e[c]->pad-gap && x<list.e[c]->x()-list.e[c]->pad)
				return c-1;
		}
	}
	return -1;
}

int StackFrame::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}

int StackFrame::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{ return 1; }

int StackFrame::MBUp(int x,int y,unsigned int state,const LaxMouse *d)
{ return 1; }

int StackFrame::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{ return 1; }

int StackFrame::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{ return 1; }

int StackFrame::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if ((win_style&STACKF_NOT_SIZEABLE) || !buttondown.isdown(d->id,LEFTBUTTON) || whichbar<0) return 0;

	int lastx,lasty;
	buttondown.move(d->id, x,y, &lastx,&lasty);

	if ((win_style&STACKF_VERTICAL)) {
		if (y==lasty) return 0;
		MoveBar(whichbar, y-lasty, 0);
	} else {
		if (x==lastx) return 0;
		MoveBar(whichbar, x-lastx, 0);
	}

	return 0;
}

/*! Sync() panes in addition to default resize.
 */
int StackFrame::MoveResize(int nx,int ny,int nw,int nh)
{
	DBG cerr <<"StackFrame::MoveResize"<<endl;

	int c=anXWindow::MoveResize(nx,ny,nw,nh);
	Sync(0);
	return c;
}

/*! Sync() panes in addition to default resize.
 */
int StackFrame::Resize(int nw,int nh)
{
	DBG cerr <<"StackFrame::Resize"<<endl;

	int c=anXWindow::Resize(nw,nh);
	Sync(0);
	return c;
}

//! With values in the pos array, resize the panes appropriately.
/*! If add!=0, then addwindow() the child windows, if necessary.
 */
int StackFrame::Sync(int add)
{
	if (!list.n) return 1;
	if (!pos) UpdatePos(1);


//	if (pos==NULL) {
//		pos=new double[list.n];
//	}

	if (list.n==1) {
		list.e[0]->sync(0,0,win_w,win_h);
		return 0;
	}

	//int sofar=0;
	int maxdim=((win_style&STACKF_VERTICAL) ? win_h : win_w);
	
	 //the numbers in pos point to the exact middle of the gap between windows
	double lastpos=0;
	int seg,  //width of box
		segp; //left or top position of box
	int x=0,y=0,w=0,h=0;

	if (win_style&STACKF_VERTICAL) w=win_w; else h=win_h;

	for (int c=0; c<list.n; c++) {
		segp=(int)(maxdim*lastpos)+gap/2;
		seg=(int)(maxdim*(pos[c]-lastpos)-gap);

		if (seg<1) seg=1;
		
		if (win_style&STACKF_VERTICAL) {
			y=segp;
			h=seg;
		} else {
			x=segp;
			w=seg;
		}

		list.e[c]->sync(x,y,w,h);
		lastpos=pos[c];
	}

	if (add) {
		anXWindow *win;
		WinFrameBox *winbox;

		for (int c=0; c<list.n; c++) {
			 // the element might be a box containing a window, or a window derived from SquishyBox
			winbox=dynamic_cast<WinFrameBox *>(list.e[c]);
			if (winbox) win=winbox->win();
			else win=dynamic_cast<anXWindow *>(list.e[c]);

			if (win && win->xlib_window==0) app->addwindow(win,1,0);
		}
	}

	return 0;
}

//! Create or update this->pos from the dimensions in the panes.
/*! If useactual!=0, then use the w() and h() of the panes. Otherwise,
 * use the pw() and ph().
 *
 * This will destroy any info in pos already, and remap pos based on
 * pane width and height. This function should only be called after installing
 * new windows.
 *
 * Return 0 success, 1 for not enough useful information to construct pos, such
 * as having only 0 or 1 panes.
 *
 * pos[0] is the left edge of a box (when horizontal stack).
 */
int StackFrame::UpdatePos(int useactual)
{
	if (pos) delete[] pos;
	pos=new double[list.n];

	if (list.n<=1) {
		pos[0]=0;
		return 1;
	}

	int totalwidth=gap*list.n;
	for (int c=0; c<list.n; c++) {
		if (win_style&STACKF_VERTICAL)
			 totalwidth += useactual ? list.e[c]->h() : list.e[c]->ph();
		else totalwidth += useactual ? list.e[c]->w() : list.e[c]->pw();
	}
	double x=0;
	for (int c=0; c<list.n-1; c++) {
		x+=gap;

		if (win_style&STACKF_VERTICAL)
			 x += useactual ? list.e[c]->h() : list.e[c]->ph();
		else x += useactual ? list.e[c]->w() : list.e[c]->pw();

		pos[c]=x/totalwidth;

		DBG cerr <<"pos["<<c<<"]="<<pos[c]<<endl;
	}
	pos[list.n-1]=1.0;

	return 0;
}

//! From the preferred dimensions of the panes and gap, make this sized just enough to contain them. 
int StackFrame::WrapToExtent()
{
	w(BOX_SHOULD_WRAP);
	h(BOX_SHOULD_WRAP);

	//sync();
	//double squishx=0,squishy=0;
	//figureDimensions(this,NULL,NULL,0, &squishx,&squishy);
	figureDimensions(this,NULL,NULL,0, NULL,NULL);
	win_w=w();
	win_h=h();

	UpdatePos(0);

	return 0;
}

/*! When shifting a bar, you can move just the bar (shift==0),
 * or move all the bars on the right the same amount (shift==1), 
 * or move all the bars on the left the same amount (shift==2).
 *
 * Return 0 for bar moved, else nonzero.
 *
 * \todo actually implement the shifting of adjacent bars
 */
int StackFrame::MoveBar(int index, int pixelamount, int shift)
{
	if (index<0 || index>=list.n-1) return 1;
	if (!pixelamount) return 0;

	 //If moving the bar makes it cross over another bar, then just don't
	double mag=(win_style&STACKF_VERTICAL)?win_h:win_w;
	int curpos=(int)(mag*pos[index]);
	int topos=curpos+pixelamount;
	if (pixelamount>0) {
		if (index==list.n-2 && topos>mag-gap) return 1;
		else if (index<list.n-2 && (topos+gap)/mag>pos[index+1]) return 1;
	} else {
		if (index==0 && topos<gap) return 1;
		else if (index>0 && (topos-gap)/mag<pos[index-1]) return 1;
		DBG cerr <<"move back: "<<(topos-gap)/mag<<",  "<<pos[index-1]<<endl;
	}
	pos[index]=topos/mag;

	Sync(0);
	needtodraw=1;
	return 0;
}

//! Use the given value for the pixel gap between windows.
/*! This does not Sync() the windows.
 */
int StackFrame::Gap(int ngap)
{
	if (ngap>=2) gap=ngap;
	gap=(gap/2)*2;
	return gap;
}


 //-------------------- basic window container function
/*! \todo maybe the AddWin(), ReplaceWin() functions should belong
 *     to a window container base class..
 */
 
//! Replace the window on the stack at index with the given window.
/*! Return 0 for window replaced, or nonzero for error.
 */
int StackFrame::ReplaceWin(anXWindow *win, int absorbcount, int index)
{
	if (!win) return 1;
	if (index<0 || index>=list.n) return 2;

	WinFrameBox *box=dynamic_cast<WinFrameBox *>(list.e[index]);
	if (!box) {
		//retain the metrics of the box, but replace the whole box with a new one
		// ***
		list.remove(index);
		AddWin(win,absorbcount,index);
	} else {
		app->reparent(win,this);
		if (box->win()) app->destroywindow(box->win());
		box->NewWindow(win);
		if (absorbcount) win->dec_count();
	}

	return 0;
}

//! Add an already made WinFrameBox.
/*! This is really just a convenience function for any
 * derived classes. It is not really necessary to create a whole
 * box just in order to add a window.
 * 
 * The end result is for win to be pushed onto the top of the stack.
 */
int StackFrame::AddWin(WinFrameBox *box,char islocal,int where)
{
	if (!box) return 1;
	if (box->win() && box->win()->win_parent!=this) app->reparent(box->win(),this);
	Push(box,islocal,where); //this is SquishyBox::Push()
	needtodraw=1;
	return 0;
}

//! Add a window with squish values.
/*! Return 0 on success, else non zero error code.
 * 
 * The end result is for win to be pushed onto the top of the stack.
 * Calls AddWin(WinFrameBox *,char,int) for the actual pushing.
 *
 * If win==NULL, then the given dimensions is for a spacer block.
 */
int StackFrame::AddWin(anXWindow *win,int absorbcount,
						int npw,int nws,int nwg,int nhalign,int nhgap,
						int nph,int nhs,int nhg,int nvalign,int nvgap,
						int where)//where==-1
{
	if (!win) return 1;

	WinFrameBox *wf;
	wf=new WinFrameBox(win, 0,npw,npw,nws,nwg,nhalign,nhgap, 0,nph,nph,nhs,nhg,nvalign,nvgap);
	if (win) {
		wf->pad=win->win_border;
		if (win->win_parent!=this) app->reparent(win,this);
		if (absorbcount) win->dec_count();
	}
	AddWin(wf,1,where);
	needtodraw=1;
	return 0;
}

//! Add a window using the window's width and height with no squishability.
/*! Reparents if win->win_parent!=this.
 * If the window is derived from SquishyBox, then pass it on to SquishyBox::Push().
 * Else make a WinFrameBox and pass to AddWin(WinFrameBox *,char,int).
 * 
 * The end result is for win to be pushed onto position where of the stack.
 * where==-1 means to top.
 * 
 * Returns 0 on success, else nonzero 
 */
int StackFrame::AddWin(anXWindow *win,int absorbcount,int where)//where==-1
{
	if (!win) return 1;
	if (win->win_parent!=this) app->reparent(win,this);

	if (dynamic_cast<SquishyBox *>(win)) {
		Push((SquishyBox *)win,where);
		return 0;
	}

	return AddWin(win,absorbcount, win->win_w?win->win_w:5,0,0,50,0, win->win_h?win->win_h:5,0,0,50,0, where);
}




} //namespace Laxkit

