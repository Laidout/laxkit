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
	InstallColors(THEME_Panel);

	if (win_style & STACKF_VERTICAL)
		 flags = (flags & ~BOX_HORIZONTAL) | BOX_VERTICAL;
	else flags = (flags & ~BOX_VERTICAL) | BOX_HORIZONTAL;

	if (ngap < 0) gap = win_themestyle->normal->textheight() / 2;
	else gap = ngap / 2 * 2;

	needtoremap = 1;

	pos = nullptr;
	whichbar = -1;

	curshape = 0;
	// if (win_style&STACKF_VERTICAL) win_pointer_shape=LAX_MOUSE_UpDown;
	// else win_pointer_shape=LAX_MOUSE_LeftRight;
}


/*! Convenience static constructor to return a StackFrame set to horizontal, with name "hbox". */
StackFrame *StackFrame::HBox()
{
	return new StackFrame(nullptr, "hbox", nullptr, 0,
		0,0,0,0,0,
		nullptr, 0, nullptr,
		-1);
}

/*! Convenience static constructor to return a StackFrame set to vertical. */
StackFrame *StackFrame::VBox()
{
	return new StackFrame(nullptr, "vbox", nullptr, STACKF_VERTICAL,
		0,0,0,0,0,
		nullptr, 0, nullptr,
		-1);
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
	anXWindow::init();
	SyncFromPos(true);
	for (int c=0; c<list.n; c++) {
		if (list.e[c]->hidden()) {
			HideSubBox(c);
		}
	}

	return 0;
}

/*! Return the window, if any, contained in box at index.
 */
anXWindow *StackFrame::GetWindow(int index)
{
	if (index < 0 || index >= NumBoxes()) return nullptr;

	anXWindow *w = dynamic_cast<anXWindow *>(GetBox(index));
	if (w) return w;
	
	WinFrameBox *wf = dynamic_cast<WinFrameBox *>(GetBox(index));
	if (wf) return wf->win();

	return nullptr;
}


void StackFrame::Refresh()
{
	if (needtoremap) {
		SyncFromPos(0);
		needtoremap = 0;
	}
	needtodraw = 0;
	if (NumVisibleBoxes() == 0 || gap<=0) return;

	Displayer *dp = MakeCurrent();

	unsigned long highlight=coloravg(win_themestyle->bg,rgbcolor(255,255,255));
	unsigned long shadow   =coloravg(win_themestyle->bg,rgbcolor(0,0,0));
	unsigned long dots     =coloravg(win_themestyle->bg,win_themestyle->fg);
	
	dp->ClearWindow();

	SquishyBox *b;
	int i = -1; //index of visible
	for (int c = 0; c < list.n; c++) {
		b = list.e[c];
		if (b->hidden()) continue;
		i++;

		if (win_style & STACKF_BEVEL) {
			dp->drawBevel(gap/2, highlight,shadow, LAX_OFF, b->x()-gap/2,b->y()-gap/2, b->w()+gap,b->h()+gap);

		} else {
			// draw little dots on draggable bars if gap>0
			dp->NewFG(dots);
			if (i == NumVisibleBoxes()-1) continue;
			// if (c == list.n-1) continue;

			int p;
			if (win_style & STACKF_VERTICAL) {
				p = (int)(pos[c] * win_h);

				if (c == whichbar) {
					dp->NewFG(coloravg(win_themestyle->bg, win_themestyle->fg, .2));
					dp->drawrectangle(0,p-gap/2, win_w,gap, 1);
					dp->NewFG(dots);
				}

				dp->drawthing(win_w/2  ,p, 2,2, 1,THING_Circle);
				dp->drawthing(win_w/2-5,p, 2,2, 1,THING_Circle);
				dp->drawthing(win_w/2+5,p, 2,2, 1,THING_Circle);
			} else {
				p = (int)(pos[c]*win_w);

				if (c == whichbar) {
					dp->NewFG(coloravg(win_themestyle->bg, win_themestyle->fg, .2));
					dp->drawrectangle(p-gap/2,0, gap,win_h, 1);
					dp->NewFG(dots);
				}

				dp->drawthing(p,win_h/2  , 2,2, 1,THING_Circle);
				dp->drawthing(p,win_h/2-5, 2,2, 1,THING_Circle);
				dp->drawthing(p,win_h/2+5, 2,2, 1,THING_Circle);
			}
		}
	}
}

int StackFrame::Event(const EventData *e,const char *mes)
{
	if (e->type == LAX_onMouseOut) {
		if (curshape != 0) {
			const EnterExitData *ee = dynamic_cast<const EnterExitData *>(e);
			dynamic_cast<LaxMouse *>(ee->device)->setMouseShape(this, 0);
			curshape = 0;
		}
		if (!buttondown.any()) {
			whichbar = -1;
			needtodraw = 1;
		}
		DBG cerr << "Stackframe exit: "<<whichbar<<endl;

	} else if (e->type == LAX_onMouseIn) {
		const EnterExitData *ee = dynamic_cast<const EnterExitData *>(e);

		if (!buttondown.any()) {
			whichbar = findWhichBar(ee->x, ee->y);
		}
		needtodraw = 1;
		DBG cerr << "Stackframe enter: "<<whichbar<<endl;

		if (whichbar >= 0) {
			if (win_style & STACKF_VERTICAL)
				 curshape = LAX_MOUSE_UpDown;
			else curshape = LAX_MOUSE_LeftRight;

		} else curshape = 0;

		dynamic_cast<LaxMouse*>(ee->device)->setMouseShape(this,curshape);
	}

	return anXWindow::Event(e,mes);
}

int StackFrame::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (list.n <= 1) return 0;

	whichbar = findWhichBar(x, y);
	if (whichbar >= 0) buttondown.down(d->id, LEFTBUTTON, x, y, whichbar);
	needtodraw = 1;

	return 0;
}

//! Return the index in pos of the bar under (x,y) or -1.
int StackFrame::findWhichBar(int x,int y)
{
	int i = -1;
	if (win_style & STACKF_VERTICAL) {
		for (int c = 0; c < list.n; c++) {
			if (list.e[c]->hidden()) continue;
			if (y >= list.e[c]->y() - list.e[c]->pad - gap && y < list.e[c]->y() - list.e[c]->pad) {
				i = c-1;
				break;
			}
		}
	} else {
		for (int c = 1; c < list.n; c++) {
			if (list.e[c]->hidden()) continue;
			if (x >= list.e[c]->x() - list.e[c]->pad - gap && x < list.e[c]->x() - list.e[c]->pad) {
				i = c-1;
				break;
			}
		}
	}
	while (i > 0 && pos[i] < 0) i--;
	return i;
}

int StackFrame::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id,LEFTBUTTON);
	needtodraw = 1;
	return 0;
}

int StackFrame::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if ((win_style&STACKF_NOT_SIZEABLE) || !buttondown.isdown(d->id,LEFTBUTTON) || whichbar<0) return 0;

	int lastx,lasty;
	buttondown.move(d->id, x,y, &lastx,&lasty);

	if ((win_style&STACKF_VERTICAL)) {
		if (y == lasty) return 0;
		MoveBar(whichbar, y-lasty, 0);
	} else {
		if (x == lastx) return 0;
		MoveBar(whichbar, x-lastx, 0);
	}

	return 0;
}


/*! SyncFromPos() panes in addition to default resize.
 */
int StackFrame::MoveResize(int nx,int ny,int nw,int nh)
{
	DBG cerr <<"StackFrame::MoveResize"<<endl;

	int c = anXWindow::MoveResize(nx,ny,nw,nh);
	needtoremap = 1;
	needtodraw = 1;
	// RebuildPos(true);
	// SyncFromPos(0);
	return c;
}

/*! SyncFromPos() panes in addition to default resize.
 */
int StackFrame::Resize(int nw,int nh)
{
	DBG cerr <<"StackFrame::Resize"<<endl;

	int c = anXWindow::Resize(nw,nh);
	needtoremap = 1;
	needtodraw = 1;
	// RebuildPos(true);
	// SyncFromPos(0);
	return c;
}


/*! This will be called from parents after this's dimensions have been set after a parent gets configured.
 * The StackFrame should then recompute child positions and sizes, and tell them to also sync().
 */
void StackFrame::sync()
{
	DBG cerr <<"StackFrame::sync()"<<endl;

	MoveResize(x(),y(), w(),h()); //ends up queueing SyncFromPos, which is called at next Refresh
	//SyncFromPos(0); <- this is called from MoveResize(..)
	//ListBox::sync();
}

//! With values in the pos array, resize the visible children appropriately.
/*! If add != 0, then addwindow() the child windows, if necessary.
 */
int StackFrame::SyncFromPos(int add)
{
	if (!list.n) { needtoremap = 0; return 1; }
	if (!pos) RebuildPos(false);

	if (list.n == 1) {
		list.e[0]->sync(0,0,win_w,win_h);
		needtoremap = 0;
		return 0;
	}

	//int sofar=0;
	int maxdim = ((win_style & STACKF_VERTICAL) ? win_h : win_w);
	
	 //the numbers in pos point to the exact middle of the gap between windows
	double lastpos = 0;
	int seg,  //width of box
		segp; //left or top position of box
	int x=0, y=0, w=0, h=0;

	if (win_style & STACKF_VERTICAL) w = win_w; else h = win_h;

	for (int c = 0; c < list.n; c++) {
		if (list.e[c]->hidden()) continue;

		segp = (int)(maxdim*lastpos) + gap/2;
		seg  = (int)(maxdim*(pos[c] - lastpos) - gap);

		if (seg < 1) seg = 1;
		
		if (win_style & STACKF_VERTICAL) {
			y = segp;
			h = seg;
		} else {
			x = segp;
			w = seg;
		}

		list.e[c]->sync(x,y,w,h);
		lastpos = pos[c];
	}

	if (add) {
		anXWindow *win;
		WinFrameBox *winbox;

		for (int c = 0; c < list.n; c++) {
			 // the element might be a box containing a window, or a window derived from SquishyBox
			winbox = dynamic_cast<WinFrameBox *>(list.e[c]);
			if (winbox) win = winbox->win();
			else win = dynamic_cast<anXWindow *>(list.e[c]);

			if (win) app->addwindow(win,1,0);
		}
	}

	needtoremap = 0;
	return 0;
}

//! Create or update this->pos from the dimensions in the panes.
/*! If useactual!=0, then use the w() and h() of the panes. Otherwise,
 * use the pw() and ph().
 *
 * This will destroy any info in pos already, and remap pos based on
 * pane width and height. This function should ONLY be called after installing
 * new windows.
 *
 * Return 0 success, 1 for not enough useful information to construct pos, such
 * as having only 0 or 1 panes.
 *
 * pos is the right edge of a box in list when horizontal stack, and bottom edge for vertical
 * stack.
 */
int StackFrame::RebuildPos(bool useactual)
{
	if (pos) delete[] pos;

	if (list.n <= 1) {
		pos = new double[1];
		pos[0] = 1.0;
		return 1;
	}
	pos = new double[list.n];

	// pass 1, get total width
	int totaln = NumVisibleBoxes();
	int totalwidth = gap * totaln; //list.n;
	for (int c = 0; c < list.n; c++) {
		if (list.e[c]->hidden()) continue;
		if (win_style & STACKF_VERTICAL)
			 totalwidth += useactual ? list.e[c]->h() : list.e[c]->ph();
		else totalwidth += useactual ? list.e[c]->w() : list.e[c]->pw();
	}

	// pass 2, compute normalized bar positions
	double x = 0;
	int c = 0;
	int i = 0;
	for ( ; i < totaln-1; c++, i++) {
		while (list.e[c]->hidden()) { c++; pos[c] = -1; }
		x += gap;

		if (win_style & STACKF_VERTICAL)
			 x += useactual ? list.e[c]->h() : list.e[c]->ph();
		else x += useactual ? list.e[c]->w() : list.e[c]->pw();

		pos[c] = x / totalwidth;

		DBG cerr <<"pos["<<c<<"]="<<pos[c]<<endl;
	}

	// there should be at most 1 non-hidden box remaining
	while (c < list.n) {
		if (list.e[c]->hidden()) pos[c] = -1;
		else pos[c] = (i == totaln-1 ? 1.0 : -1);
		c++;
	}

	return 0;
}

//! From the preferred dimensions of the panes and gap, make this sized just enough to contain them. 
int StackFrame::WrapToExtent()
{
	w(BOX_SHOULD_WRAP);
	h(BOX_SHOULD_WRAP);

	//sync();
	//double squishx=0,squishy=0;
	//figureDimensions(this,nullptr,nullptr,0, &squishx,&squishy);
	figureDimensions(this,nullptr,nullptr,0, nullptr,nullptr);
	win_w = w();
	win_h = h();

	RebuildPos(0);

	return 0;
}

/*! When shifting a bar, you can move just the bar (shift==0),
 * or move all the bars on the right the same amount (shift==1), 
 * or move all the bars on the left the same amount (shift==2).
 *
 * Return 0 for bar moved, else nonzero.
 * If pos[index] == -1 (the box is hidden), return 1, and do nothing.
 *
 * \todo actually implement the shifting of adjacent bars
 */
int StackFrame::MoveBar(int index, int pixelamount, int shift)
{
	if (index < 0 || index >= list.n-1 || pos[index] == -1) return 1;
	if (!pixelamount) return 0;

	if (!pos) RebuildPos(true);

	// If moving the bar makes it cross over another bar, then just don't
	double mag = (win_style & STACKF_VERTICAL) ? win_h : win_w;
	int curpos = (int)(mag*pos[index]);
	int to_pos = curpos + pixelamount;

	int next_index = index + 1;
	while (next_index < list.n && pos[next_index] == -1) next_index++;
	int next_index2 = next_index + 1;
	while (next_index2 < list.n && pos[next_index2] == -1) next_index2++;
	int prev_index = index - 1;
	while (prev_index >= 0 && pos[prev_index] == -1) prev_index--;
	int prev_index2 = prev_index - 1;
	while (prev_index2 >= 0 && pos[prev_index2] == -1) prev_index2--;

	if (pixelamount > 0) {
		if (next_index >= list.n && to_pos > mag - gap) return 1;
		else if (next_index < list.n && (to_pos+gap) / mag > pos[next_index]) return 1;
	} else {
		if ((prev_index < 0 || index == 0) && to_pos < gap) return 1;
		else if (index > 0 && (to_pos - gap) / mag < pos[index-1]) return 1;

		//DBG cerr <<"move back: "<<(to_pos-gap)/mag<<",  "<<pos[index-1]<<endl;
	}
	pos[index] = to_pos / mag;

	SyncFromPos(0);
	needtodraw = 1;
	return 0;
}

//! Use the given value for the pixel gap between windows.
/*! This does not SyncFromPos() the windows.
 */
int StackFrame::Gap(int ngap)
{
	if (ngap >= 2) gap = ngap;
	gap = (gap/2)*2;
	return gap;
}


//-------------------- basic window container function

bool StackFrame::HideSubBox(int index)
{
	if (!ListBox::HideSubBox(index)) return false;
	SquishyBox *box = list.e[index];
	anXWindow *win = dynamic_cast<anXWindow*>(box);
	if (!win) {
		WinFrameBox *wbox = dynamic_cast<WinFrameBox*>(box);
		if (wbox) win = wbox->win();
	}
	if (win) {
		app->unmapwindow(win);
	}
	if (pos) pos[index] = -1;
	needtoremap = 1;
	return true;
}

bool StackFrame::ShowSubBox(int index)
{
	if (!ListBox::ShowSubBox(index)) return false;
	SquishyBox *box = list.e[index];
	anXWindow *win = dynamic_cast<anXWindow*>(box);
	if (!win) {
		WinFrameBox *wbox = dynamic_cast<WinFrameBox*>(box);
		if (wbox) win = wbox->win();
	}
	if (win) {
		app->mapwindow(win);
	}

	// force recomputation of pos down the line somewhere
	delete[] pos;
	pos = nullptr;

	// int next_index = index + 1;
	// while (next_index < list.n && pos[next_index] == -1) next_index++;
	// int next_index2 = next_index + 1;
	// while (next_index2 < list.n && pos[next_index2] == -1) next_index2++;
	// int prev_index = index - 1;
	// while (prev_index >= 0 && pos[prev_index] == -1) prev_index--;
	// int prev_index2 = prev_index - 1;
	// while (prev_index2 >= 0 && pos[prev_index2] == -1) prev_index2--;
	
	// if (prev_index >= 0) {
	// 	pos[index] = pos[prev_index];
	// 	if (prev_index2 >= 0) pos[prev_index] = (pos[prev_index2] + pos[index])/2;
	// 	else pos[prev_index] = pos[index]/2;
		

	// 	if (next_index < list.n) pos[index] = (pos[next_index])
	// }

	needtoremap = 1;
	return true;
}


//! Replace the window on the stack at index with the given window.
/*! Return 0 for window replaced, or nonzero for error.
 */
int StackFrame::ReplaceWin(anXWindow *win, int absorbcount, int index)
{
	if (!win) return 1;
	if (index < 0 || index >= list.n) return 2;

	WinFrameBox *box = dynamic_cast<WinFrameBox *>(list.e[index]);
	if (!box) {
		//retain the metrics of the box, but replace the whole box with a new one
		// ***
		list.remove(index);
		AddWin(win, absorbcount, index);
	} else {
		app->reparent(win, this);
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
	if (box->win() && box->win()->win_parent != this) app->reparent(box->win(), this);
	Push(box, islocal, where); //this is SquishyBox::Push()
	needtodraw = 1;
	needtoremap = 1;
	return 0;
}

//! Add a window with squish values.
/*! Return 0 on success, else non zero error code.
 * 
 * The end result is for win to be pushed onto the top of the stack.
 * Calls AddWin(WinFrameBox *,char,int) for the actual pushing.
 *
 * If win==nullptr, then the given dimensions is for a spacer block.
 */
int StackFrame::AddWin(anXWindow *win,int absorbcount,
						int npw,int nws,int nwg,int nhalign,int nhgap,
						int nph,int nhs,int nhg,int nvalign,int nvgap,
						int where)//where==-1
{
	if (!win) return 1;

	WinFrameBox *wf;
	wf = new WinFrameBox(win, 0,npw,npw,nws,nwg,nhalign,nhgap, 0,nph,nph,nhs,nhg,nvalign,nvgap);
	if (win) {
		wf->pad = win->win_border;
		if (win->win_parent != this) app->reparent(win, this);
		if (absorbcount) win->dec_count();
	}
	AddWin(wf,1,where);
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
int StackFrame::AddWin(anXWindow *win,int absorbcount,int where)
{
	if (!win) return 1;
	if (win->win_parent!=this) app->reparent(win,this);

	if (dynamic_cast<SquishyBox *>(win)) {
		Push(dynamic_cast<SquishyBox *>(win),where);
		if (absorbcount) win->dec_count();
		return 0;
	}

	return AddWin(win,absorbcount, win->win_w ? win->win_w : 5, 0,0,50,0, win->win_h ? win->win_h : 5, 0,0,50,0, where);
}

//! Remove item with index which from list.
/*! Default is to pop the top of the stack.
 *
 * Returns 1 for item removed, or 0 for which out of bounds.
 */
int StackFrame::Remove(int which) //which=-1
{ 
	if (which < 0 || which >= list.n) return 0;

	anXWindow *win = dynamic_cast<anXWindow*>(list.e[which]);
	if (!win) {
		WinFrameBox *w = dynamic_cast<WinFrameBox*>(list.e[which]);
		if (w) win = w->win();
	}
	if (win) app->destroywindow(win);

	needtoremap = 1;
	needtodraw = 1;
	return list.remove(which); 
}

Attribute *StackFrame::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	anXWindow::dump_out_atts(att, what, context);

	if (!att) att = new Attribute();
	Attribute *metrics = att->pushSubAtt("metrics");

	for (int c=0; c<list.n; c++) {
		SquishyBox *box = list.e[c];
		if (!box) continue;

		Attribute *boxa = metrics->pushSubAtt("box");
		boxa->push("x"     , box->x()     );
		boxa->push("w"     , box->w()     );
		boxa->push("pw"    , box->pw()    );
		boxa->push("ws"    , box->ws()    );
		boxa->push("wg"    , box->wg()    );
		boxa->push("halign", box->halign());
		boxa->push("hgap"  , box->hgap()  );
		boxa->push("y"     , box->x()     );
		boxa->push("h"     , box->w()     );
		boxa->push("ph"    , box->pw()    );
		boxa->push("hs"    , box->ws()    );
		boxa->push("hg"    , box->wg()    );
		boxa->push("valign", box->halign());
		boxa->push("vgap"  , box->hgap()  );
		boxa->push("hidden", box->hidden() ? "yes" : "no");

		anXWindow *win = GetWindow(c);
		if (win) {
			Attribute *wina = boxa->pushSubAtt("window", win->whattype());
			win->dump_out_atts(wina, what, context);
		}
	}

	return att;
}

void StackFrame::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	anXWindow::dump_in_atts(att, flag, context);

	// This assumes that boxes have already been installed, and reading in here
	// is only to assign configuration.
	Attribute *metrics = att->find("metrics");
	if (metrics) {
		const char *name;
		const char *value;
		int i = 0;

		for (int c = 0; c < att->attributes.n; c++) {
			name  = att->attributes.e[c]->name;
			value = att->attributes.e[c]->value;

			if (i >= list.n) break;

			if (!strcmp(name, "box")) {

				for (int c2 = 0; c2 < att->attributes.e[c]->attributes.n; c2++) {
					name  = att->attributes.e[c]->attributes.e[c2]->name;
					value = att->attributes.e[c]->attributes.e[c2]->value;

					double d = 0;
					DoubleAttribute(value, &d);
					if      (!strcmp(name, "x"     )) list[i]->x(d);
					else if (!strcmp(name, "w"     )) list[i]->w(d);
					else if (!strcmp(name, "pw"    )) list[i]->pw(d);
					else if (!strcmp(name, "ws"    )) list[i]->ws(d);
					else if (!strcmp(name, "wg"    )) list[i]->wg(d);
					else if (!strcmp(name, "halign")) list[i]->halign(d);
					else if (!strcmp(name, "hgap"  )) list[i]->hgap(d);
					else if (!strcmp(name, "y"     )) list[i]->x(d);
					else if (!strcmp(name, "h"     )) list[i]->w(d);
					else if (!strcmp(name, "ph"    )) list[i]->pw(d);
					else if (!strcmp(name, "hs"    )) list[i]->ws(d);
					else if (!strcmp(name, "hg"    )) list[i]->wg(d);
					else if (!strcmp(name, "valign")) list[i]->halign(d);
					else if (!strcmp(name, "vgap"  )) list[i]->hgap(d);
					else if (!strcmp(name, "hidden")) list[i]->hideBox(BooleanAttribute(value));
					else if (!strcmp(name, "window")) {
						cerr << " *** FINISH StackFrame::dump_in_atts for window" << endl;
					}
				}
				i++;
			}
		}
	}
}


} //namespace Laxkit

