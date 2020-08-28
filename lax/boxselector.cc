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


#include <lax/boxselector.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

//------------------------- SelBox -------------------------------------

/*! \class SelBox
 * \brief Adds id,info,state to SquishyBox. Used in BoxSelector.
 *
 * StrIconSelector, for instance, redefines this to also hold a label and an icon.
 */
/*! \var unsigned int SelBox::state
 * \brief Holds pointer state related to the box.
 *
 * This will hold LAX_ON, LAX_OFF, LAX_GRAY, which is then or'd with LAX_MOUSEIN
 * and LAX_CCUR. LAX_CCUR is used to indicate where key actions are
 * directed to.
 */
/*! \var int SelBox::id
 * \brief An identifying number for this particular box.
 */
/*! \var int SelBox::info
 * \brief Extra information that might be useful for this box.
 */
/*! \var int SelBox::mousecount
 * \brief How many mice are inside the box.
 */


SelBox::SelBox(int nid) //nid=0
{ 
	state=LAX_OFF; 
	info=0;
	id=nid; 
	mousecount=0;
}

SelBox::SelBox(int xx,int yy,int ww,int hh,int nid)
	: SquishyBox(0, xx,ww,ww,0,0,50,0, yy,hh,hh,0,0,50,0)
{
	state=LAX_OFF;
	info=0;
	id=nid;
	mousecount=0;
}

//------------------------- BoxSelector -------------------------------------

/*! \class BoxSelector
 * \brief Abstract base class for selectors based on boxed items.
 *
 *  In most cases, derived classes need only define drawbox(which) and
 *  also provide an AddWhateverBox(whatever) that ultimately does something
 *  like wholelist.push(TheNewBox). That whatever box must be derived from SelBox.
 *  Arrangement and clicking of wholelist is handled here.
 *  For an example of class that needs to redefine others, StrIconSelector has to
 *  Free the pixmaps in its destructor before ~BoxSelector is called.
 *
 *  This class is similar to RowFrame in that it is also derived from RowColBox,
 *  but uses manual tracking of the mouse over wholelist, rather than having separate
 *  windows.  It is to be used for selecting things, not for holding windows as in
 *  RowFrame.
 *
 * \code
 *  #define BOXSEL_COLUMNS     (1<<16)
 *  #define BOXSEL_VERTICAL    (1<<16)
 *  #define BOXSEL_ROWS        (1<<17)
 *  #define BOXSEL_HORIZONTAL  (1<<17)
 *  
 *  #define BOXSEL_CENTER      (1<<19|1<<22)
 *  #define BOXSEL_LEFT        (1<<18)
 *  #define BOXSEL_HCENTER     (1<<19)
 *  #define BOXSEL_RIGHT       (1<<20)
 *  #define BOXSEL_TOP         (1<<21)
 *  #define BOXSEL_VCENTER     (1<<22)
 *  #define BOXSEL_BOTTOM      (1<<23)
 *  
 *   // how to fill gaps between boxes
 *   // these refer to filling whole window
 *  #define BOXSEL_STRETCH     (1<<24|1<<25)
 *  #define BOXSEL_STRETCHX    (1<<24)
 *  #define BOXSEL_STRETCHY    (1<<25)
 *  #define BOXSEL_SPACE       (1<<26|1<<27)
 *  #define BOXSEL_SPACEX      (1<<26)
 *  #define BOXSEL_SPACEY      (1<<27)
 *  
 *   // these refer to filling extra (width in cols) or (height in rows)
 *  #define BOXSEL_STRETCH_IN_ROW (1<<28)
 *  #define BOXSEL_STRETCH_IN_COL (1<<28)
 *  
 *   // make the panel look flat, mouse over highlights, selected ones are elevated.
 *   // Otherwise default is all are either up or down, or flat if grayed
 *  #define BOXSEL_FLAT        (1<<29)
 *  #define BOXSEL_ONE_ONLY    (1<<30)
 * \endcode
 *  
 */
/*! \var int BoxSelector::curbox
 * \brief The box most recently selected.
 */
/*! \fn void BoxSelector::drawbox(int which)
 * \brief Draw the box on the screen.
 *
 * If box->mousecount is greater that zero, then that many mice are currently
 * over the box. Whether the box is actually on or not is read from box->state&(LAX_ON|LAX_OFF).
 */



//! Constructor.
/*! Uses default SquishyBox constructor, and manually changes flags.
 */
BoxSelector::BoxSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,int nid,
						int npad, //!< padinset of *this is set to npad
						int box_style)
		: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendmes)
{
	InstallColors(THEME_Panel);

	padinset=npad;
	bevel=1;
	padi=1;
	curbox=-1;
	hoverbox=-1;

	selection_style = SEL_Mixed;
	if (win_style & BOXSEL_ONE_ONLY) selection_style = SEL_One_Only;

	display_style = BOXES_Beveled;
	if (win_style & BOXSEL_FLAT) display_style = BOXES_Flat;

//	filterflags();*** see RowColBox::filterflags
//	if (win_style&BOXSEL_STRETCHX) elementflags|=BOX_STRETCH_TO_FILL_X;

	flags=0; // will set flags here manually, not use default RowColBox()
	 //*** set squishy parameters/flags, should rethink this process a bit?
	if (win_style&BOXSEL_COLUMNS) { flags|=BOX_HORIZONTAL; }
	if (win_style&BOXSEL_ROWS) { flags|=BOX_VERTICAL; }
	
	if (flags&BOX_VERTICAL) {
		flags|=BOX_STRETCH_TO_FILL_X;
		if (win_style&BOXSEL_STRETCHY) flags|=BOX_STRETCH_TO_FILL_Y;
		if (win_style&BOXSEL_STRETCH_IN_ROW) elementflags|=BOX_STRETCH_TO_FILL_Y;
	} else {
		flags|=BOX_STRETCH_TO_FILL_Y;
		if (win_style&BOXSEL_STRETCHX) flags|=BOX_STRETCH_TO_FILL_X;
		if (win_style&BOXSEL_STRETCH_IN_ROW) elementflags|=BOX_STRETCH_TO_FILL_X;
	}
	
	if (win_style&BOXSEL_STRETCHX) elementflags|=BOX_STRETCH_TO_FILL_X; 
	if (win_style&BOXSEL_STRETCHY) elementflags|=BOX_STRETCH_TO_FILL_Y;

	if (win_style&BOXSEL_SPACEX) elementflags|=BOX_SPACE_TO_FILL_X;
	if (win_style&BOXSEL_SPACEY) elementflags|=BOX_SPACE_TO_FILL_Y;
	
	 // justifying applies to elements in row/col, not placement within extra height of rows/width of cols
	if (win_style&BOXSEL_HCENTER) flags|=BOX_HCENTER;
	else if (win_style&BOXSEL_LEFT) flags|=BOX_LEFT;
	else if (win_style&BOXSEL_RIGHT) flags|=BOX_RIGHT;
	
	if (win_style&BOXSEL_VCENTER) flags|=BOX_VCENTER;
	else if (win_style&BOXSEL_TOP) flags|=BOX_TOP;
	else if (win_style&BOXSEL_BOTTOM) flags|=BOX_BOTTOM;

}

BoxSelector::~BoxSelector()
{
}

//! Wraps the window to the bounding box of the laid out boxes
int BoxSelector::WrapToExtent()
{
	// *** this should have a WrapToWidth(width)
	win_w = BOX_SHOULD_WRAP;
	win_h = BOX_SHOULD_WRAP;
	w(BOX_SHOULD_WRAP);
	h(BOX_SHOULD_WRAP);
	sync();

	if (ValidDrawable()) {
		//has been initialized into platform already, we have to use resize mechanism
		//TODO: this may fail when window is created but not mapped.. TEST!
		MoveResize(win_x, win_y, w(), h());
	} else {
		win_w = w();
		win_h = h();
	}
	
//	 // must shift all wholelist.e[c]->x/y
//	int dx = bbox.x,
//		dy = bbox.y;
//	bbox.x = bbox.y = 0;
//	for (int c=0; c<wholelist.n; c++) {
//		wholelist.e[c]->x -= dx;
//		wholelist.e[c]->y -= dy;
//	}

	return 0;
}

//! Default BoxSelector init only calls sync().
/*! Also sets the hightlight and shadow colors.
 */
int BoxSelector::init()
{ 
	sync();
    highlight = coloravg(win_themestyle->bg.Pixel(),rgbcolor(255,255,255));
	shadow = coloravg(win_themestyle->bg.Pixel(),rgbcolor(0,0,0));
	return 0;
}

//! Set the w/h, then calls SquishyBox::sync().
/*! This is called from init() and from the SquishyBox arranger.
 * If win_w or win_h are 0, then they are set to BOX_SHOULD_WRAP instead to force a wrap.
 * Then ***not yet*** if style is BOXSEL_ONE_ONLY, then sync ensures that there
 * is, in fact, one selected.
 * Finally, SquishyBox::sync() is called.
 */
void BoxSelector::sync()
{
	w(win_w?win_w:BOX_SHOULD_WRAP); // the BOX_SHOULD_WRAP forces a wrap to extent of child boxes
	h(win_h?win_h:BOX_SHOULD_WRAP);
	 //*** for ONE_ONLY, make sure there is one selected!! 
	 //*** must count how many selected if none, select first non-gray box, and turn off
	if (win_style&BOXSEL_ONE_ONLY && wholelist.n==0) {
//		SelBox *b=dynamic_cast<SelBox *>(wholelist.e[0]);
//		if (b) b->state=(b->state&(LAX_ON));
	}

	 //check to see if we need to moveresize the window
//	if (x()!=win_x || y()!=win_y || w()!=win_w || h()!=win_h) 

	ListBox::sync();

	 //resize this window if necessary
	if (x()!=win_x || y()!=win_y || w()!=win_w || h()!=win_h) {
		 //only resize if width and height seem reasonable, ie, not BOX_SHOULD_WRAP
		//if (w()<3000 && h()<3000) MoveResize(x(),y(), w(),h());
		//***this seems to move window up to the parent origin!!!! grrrr
	}

	arrangedstate=1;
	needtodraw=1;
}

//! Send a message to the owner.
/*! A SimpleMessage is filled thus:	
 * <pre>
 * 	info1=curbox; // current box*** if LAX_ON only?? or first on box?
 * 	info2=(curbox>=0?wholelist.e[curbox]->id:0); // id of curbox
 * 	info3=n; // num that are on
 * 	info4=0; // ***num that are newly changed
 * </pre>
 *
 * Remember that curbox can by -1 meaning no current box.
 */	
int BoxSelector::send()
{
	SimpleMessage *mevent=new SimpleMessage;

	int n=0;
	for (int c=0; c<wholelist.n; c++) {
		if (wholelist.e[c] && ((SelBox *)(wholelist.e[c]))->state&LAX_ON) n++;
	}
	mevent->info1=curbox; // current box*** if LAX_ON only?? or first on box?
	mevent->info2=(curbox>=0?((SelBox *)wholelist.e[curbox])->id:0); // id of curbox
	mevent->info3=n; // num that are on
	mevent->info4=0; // ***num that are newly changed

	app->SendMessage(mevent,win_owner,win_sendthis,object_id);
	return 1;
}

//! Draw the boxes
/*! TODO: smart refreshing to only draw a list of changes?
 */
void BoxSelector::Refresh()
{
	if (!needtodraw || !win_on) { needtodraw = 0; return; }
	if (arrangedstate!=1) sync();

	Displayer *dp = MakeCurrent();
	dp->ClearWindow();
	dp->font(win_themestyle->normal);

	for (int c=0; c<wholelist.n; c++) {
		if (!wholelist.e[c]) continue;

		DBG SelBox *box=dynamic_cast<SelBox *>(wholelist.e[c]);
		DBG if (box) cerr <<" ---drawbox "<<c<<"("<<box->state<<"):"<<
		DBG 	box->x()<<','<<box->y()<<','<<box->w()<<','<<box->h()<<endl;

		drawbox(c);
	}
	
	needtodraw=0;
	SwapBuffers();
}

//! Return index of box the mouse is in.
int BoxSelector::MouseInWhich(int x,int y)
{ 
	for (int c=0; c<wholelist.n; c++) {
		if (!wholelist.e[c]) continue;
		if (  x>=wholelist.e[c]->x()-wholelist.e[c]->pad && 
			  x< wholelist.e[c]->x()+wholelist.e[c]->w()+wholelist.e[c]->pad &&
			  y>=wholelist.e[c]->y()-wholelist.e[c]->pad && 
			  y< wholelist.e[c]->y()+wholelist.e[c]->h()+wholelist.e[c]->pad 
			) return c;
	}
	return -1;
}

//! Toggles box with index (not id) which on/off. Note this does NOT select/deselect the box.
/*! Does nothing if state is not LAX_ON or LAX_OFF.
 * If db, then also call drawbox(which).
 */
void BoxSelector::togglebox(int which,int db) //db=1
{
	if (which<0 || which>=wholelist.n || !wholelist.e[which])  return;
	SelBox *b=dynamic_cast<SelBox *>(wholelist.e[which]);
	if (!b || !(b->state&(LAX_ON|LAX_OFF))) return;
	b->state ^= (LAX_ON|LAX_OFF);
	if (db) drawbox(which);
}

//! Find what box is down in (set lbdown to index of it), and toggle it.
int BoxSelector::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	int lbdown=MouseInWhich(x,y);
	DBG cerr << "BoxSelector mouse in: "<<lbdown<<endl;
	if (lbdown<0) return 0; //do not record press if not pressing in a box!
	if (buttondown.isdown(0,LEFTBUTTON)) return 0; //only allow one device to be pressing buttons at a time

	buttondown.down(d->id,LEFTBUTTON, x,y, lbdown);
	if (lbdown>=0 && lbdown<wholelist.n) togglebox(lbdown);
	needtodraw=1;
	return 0;
}

//! Keeps track of where mouse is, whether or not a button is pressed.
int BoxSelector::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	int wherenow=MouseInWhich(x,y);
	
	
	if (!buttondown.isdown(d->id, LEFTBUTTON)) {
		if (hoverbox!=wherenow) {
			hoverbox=wherenow;
			needtodraw=1;
		}
		return 0;
	}

	//button is down


	if (wherenow==hoverbox) return 0; //nothing to do if not crossing box boundaries

	int lbdown=-1;
	buttondown.getextrainfo(d->id,LEFTBUTTON, &lbdown);

	SelBox *selbox=NULL;

	if (wherenow == lbdown && hoverbox!=lbdown) {
		 //reentering the down box
		selbox=dynamic_cast<SelBox*>(wholelist.e[lbdown]);
		selbox->state^=(LAX_ON|LAX_OFF);
		needtodraw=1;

	} else if (wherenow != lbdown && hoverbox==lbdown) {
		 //leaving the down box
		selbox=dynamic_cast<SelBox*>(wholelist.e[lbdown]);
		selbox->state^=(LAX_ON|LAX_OFF);
		needtodraw=1;
	}

	hoverbox=wherenow;
	return 0;
}

//! Select which box mouse is in, if it is the same as it was clicked down on.
/*! Selects with SelectN(lbdown), and resets lbdown to -1.
 */
int BoxSelector::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int lbdown=-1;
	buttondown.up(d->id,LEFTBUTTON, &lbdown);

	int wherenow = MouseInWhich(x,y);
	DBG cerr << "BoxSelector mouse up: "<<wherenow<<"  lbdown: "<<lbdown<<endl;
	if (wherenow==lbdown) {
		DBG cerr <<" BoxSelector should send() here"<<endl; //***

		SelectN(wherenow);
		send();
	}
	needtodraw=1;
	return 0;
}

//! Select a box based on box->id, and return curbox.
/*! This searchs for whichID in boxes, and then returns SelectN(thatbox).
 */
int BoxSelector::Select(int whichID)
{ 
	int c;
	SelBox *b;
	for (c=0; c<wholelist.n; c++) {
		b=dynamic_cast<SelBox *>(wholelist.e[c]);
		if (!b) continue;
		if (b->id==whichID) return SelectN(c);
	}
	return curbox;
}

//! Select a box based on its index.
/*! This acts the same as a mouse clicking it or selecting with space bar.
 *  Derived classes would redefine this to react to which box selected, such
 *  as wanting selection of "normal font" to clear all the other boxes.
 *  Box state should already have been toggled, and have the correct value
 *  before this function is called. This is because LBDown/Up makes a show of
 *  selecting a box without actually selecting it (previews what it would look
 *  like after selecting).
 *
 *  If whichindex is NULL or is grayed, then nothing is done.
 *  If BOXSEL_ONE_ONLY, then all the boxes are cleared and redrawn, and the new box selected and drawn.
 *
 *  Returns curbox.
 */
int BoxSelector::SelectN(int whichindex)
{
	if (whichindex<0 || whichindex>=wholelist.n || !wholelist.e[whichindex]) return curbox;

	SelBox *b;
	b = dynamic_cast<SelBox *>(wholelist.e[whichindex]);
	if (!b || b->state & LAX_GRAY) return curbox;
	curbox = whichindex;

	if (selection_style == SEL_One_Only) {
		 // turn OFF all wholelist that are ON
		for (int c=0; c<wholelist.n; c++) {
			b=dynamic_cast<SelBox *>(wholelist.e[c]);
			if (!b || !(b->state&(LAX_ON|LAX_OFF))) continue;
			if (b->state & LAX_ON) {
				b->state &= ~LAX_ON;
				b->state |= LAX_OFF;
				drawbox(c);
			}
		}
		 // call default toggle for box, whichindex here would just turn it ON
		togglebox(whichindex);
	}
	return curbox;
}

void BoxSelector::Flush()
{
	needtodraw = 1;
	RowColBox::Flush();
}

//! Catch EnterNotify and LeaveNotify.
int BoxSelector::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onMouseOut) {
		//DBG cerr <<" Leave:"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;
		//DBG cerr <<"  BoxSelector::event:Leave:"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;

//		if (!buttondown.isdown(0,LEFTBUTTON) && curbox>=0 && curbox<wholelist.n) {
//			 // if leaving window and button is not pressed, make sure to not draw curbox with hover color...
//			SelBox *b=dynamic_cast<SelBox *>(wholelist.e[curbox]);
//			if (b) {
//				b->state&=~LAX_MOUSEIN;
//				drawbox(curbox);
//			}
//			curbox=-1;
//		}

		if (!buttondown.isdown(0, LEFTBUTTON) && hoverbox>=0) {
			 // if leaving window and button is not pressed, make sure to not draw curbox with hover color...
			hoverbox=-1;
			curbox=-1;
			needtodraw=1;
		}
	}
	return anXWindow::Event(e,mes);
}

//! Toggle curbox. ***needs work selecting!=toggling
int BoxSelector::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	togglebox(curbox);
	return 0;
}

//! Toggle curbox. 
int BoxSelector::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	togglebox(curbox);
	return 0;
}

//! Call anXWindow::MoveResize, and set arrangedstate=0.
int BoxSelector::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	arrangedstate=0;
	return 0;
}

//! Call anXWindow::Resize, and set arrangedstate=0.
int BoxSelector::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	arrangedstate=0;
	return 0;
}



} // namespace Laxkit


