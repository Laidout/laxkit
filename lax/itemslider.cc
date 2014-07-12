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
//    Copyright (C) 2004-2010 by Tom Lechner
//


#include <lax/itemslider.h>
#include <lax/laxutils.h>
#include <unistd.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

/*! \class ItemSlider
 * \brief Abstract base class for a kind of button that increments and decrements itself
 * 		based on where the mouse is clicked.
 *
 * 	If you click to the left of center, then the item is decremented, and to the right
 * 	increments. Holding down the button and dragging progressively slides the values
 * 	up or down depending on whether you move the mouse right or left. Users of the program
 * 	Blender will be familiar with this sort of control.
 *
 * 	If EDITABLE, then leave an area in the center, clicking of which activates editing mode.
 *
 * \todo *** perhaps add basic popup mechanism to this class?
 *
 */ 


ItemSlider::ItemSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsendthis)
	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendthis)
{
	movewidth=10;
	curitem=-1;
	hover=0;

	installColors(app->color_panel);
}

//! Empty virtual.
ItemSlider::~ItemSlider()
{}

//! Do nothing init.
int ItemSlider::init()
{
	return 0;
}

//! Sends message to owner.
/*! Sends long data, with <tt>xclient.data.l[0]=getid(curitem)</tt>.
 */
int ItemSlider::send()
{
	if (!win_owner || !win_sendthis) return 0;
	SimpleMessage *ievent=new SimpleMessage(NULL, getid(curitem),0,0,0);
	app->SendMessage(ievent, win_owner, win_sendthis, object_id);
	needtodraw=1;
	return 1;
}

 //! Select the previous item.
 /*! Returns id of the new item.
  * Default ignores multiplier. During MouseMove(), if shift and/or control are pressed,
  * multiplier will be larger to say we need to make larger changes than just 1 to curitem.
  */
int ItemSlider::SelectPrevious(double multiplier)
{ 
	if (curitem==-1) return -1;
	curitem--;
	if (curitem<0) curitem=numitems()-1;
	if (win_style & SENDALL) send();
	DBG cerr <<" Previous Item:"<<curitem<<endl;
	needtodraw=1;
	return getid(curitem);
}

 //! Select the next item.
 /*! Returns id of the new item.
  * Default ignores multiplier. During MouseMove(), if shift and/or control are pressed,
  * multiplier will be larger to say we need to make larger changes than just 1 to curitem.
  */
int ItemSlider::SelectNext(double multiplier)
{
	if (curitem==-1) return -1;
	curitem++;
	if (curitem==numitems()) curitem=0;
	if (win_style & SENDALL) send();
	needtodraw=1;
	return getid(curitem);
}

//! Selects item with id equal to id.
/*! Please note this is different than selecting an index. This uses
 * getid() to compare id to the items' ids.
 *
 * Returns the id of the current item.
 */
int ItemSlider::Select(int id)
{
	if (getid(curitem)==id) return id;
	
	int c;
	for (c=0; c<numitems(); c++) if (getid(c)==id) break;
	if (c!=numitems()) {
		curitem=c;
		if (win_style & SENDALL) send();
		needtodraw=1;
	}
	return getid(curitem);
}

 //! Returns id, not index
int ItemSlider::GetCurrentItemId()
{
	return getid(curitem);
}

//! Gets ready to select next or previous...
/*! LBUp, Idle, and MouseMove do the actual Selecting.
 */
int ItemSlider::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id,LEFTBUTTON, x,y, curitem);
	return 0;
}

//! Select a next or previous item.
/*! If button is up on the left side, then SelectPrevious is called.
 * If button is on the right side, then SelectNext is called.
 *
 * If EDITABLE, then use a pad of default text height on either side of the window
 * for direction arrows. Clicking in the center activates edit mode, with
 * Mode(1). This base class does not implement any editing.
 */
int ItemSlider::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int lbitem;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &lbitem);

	int ww=win_w/2;

	if (win_style&EDITABLE) {
		ww=text_height();
		if (dragged<movewidth && x>=ww && x<win_w-ww) {
			Mode(1);
			return 0;
		}
	}

	if (dragged<movewidth && x<ww) SelectPrevious(1);
	if (dragged<movewidth && x>=win_w-ww) SelectNext(1);
	if (curitem!=lbitem) send();
	return 0;
}

/*! Change the mode of the window.
 *
 * 0 is normal mode. 1 is editing mode (if any).
 *
 * Returns current mode. This base class does not implement any actual editing,
 * so mode is never changed, and 0 is returned.
 */
int ItemSlider::Mode(int newmode)
{ return 0; }

//! Dragging the mouse horizontally selects previous or next item.
/*! If the mouse is dragged more than movewidth then the next
 * or previous item is selected. 
 */
int ItemSlider::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) {
		int nhover=0;
		int ww=win_w/2;
		if (win_style&EDITABLE) ww=text_height();

		if (x<ww) nhover=LAX_LEFT;
		else if (x>win_w-ww) nhover=LAX_RIGHT;
		else if (x>0 && x<win_w) nhover=LAX_CENTER;
		if (nhover!=hover) {
			hover=nhover;
			needtodraw=1;
		}
		return 1;
	}

	int mx,my;
	buttondown.move(d->id, x,y, &mx,&my);

	double multiplier=1;
	if ((state&(ShiftMask|ControlMask))==ShiftMask) multiplier=10;
	else if ((state&(ShiftMask|ControlMask))==ControlMask) multiplier=10;
	else if ((state&(ShiftMask|ControlMask))==(ShiftMask|ControlMask)) multiplier=20;

	if (win_style&YSHIFT) {
		if (my-y>movewidth) SelectPrevious(multiplier);
		else if (y-my>movewidth) SelectNext(multiplier);
	} else {
		if (mx-x>movewidth) SelectPrevious(multiplier);
		else if (x-mx>movewidth) SelectNext(multiplier);
	}
	return 0;
}

int ItemSlider::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	double multiplier=1;
	if ((state&(ShiftMask|ControlMask))==ShiftMask) multiplier=10;
	else if ((state&(ShiftMask|ControlMask))==ControlMask) multiplier=10;
	else if ((state&(ShiftMask|ControlMask))==(ShiftMask|ControlMask)) multiplier=20;

	SelectNext(multiplier);
	send();
	return 0;
}

int ItemSlider::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	double multiplier=1;
	if ((state&(ShiftMask|ControlMask))==ShiftMask) multiplier=10;
	else if ((state&(ShiftMask|ControlMask))==ControlMask) multiplier=10;
	else if ((state&(ShiftMask|ControlMask))==(ShiftMask|ControlMask)) multiplier=20;

	SelectPrevious(multiplier); 
	send();
	return 0;
}


} // namespace Laxkit

