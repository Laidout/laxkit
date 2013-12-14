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
 * \todo *** perhaps add basic popup mechanism to this class?
 *
 * \code
 *  // SENDALL means send sendthis message on every Idle,Select/Previous/Next
 *  // AUTOSHIFT means move mouse outside window then sit also selects
 *  //   prev/next based on (how far out of window)/movewidth
 * #define ITEMSLIDER_XSHIFT    (1<<16)
 * #define ITEMSLIDER_YSHIFT    (1<<17)
 * #define ITEMSLIDER_SENDALL   (1<<18)
 * #define ITEMSLIDER_AUTOSHIFT (1<<19)
 * #define ITEMSLIDER_POPUP     (1<<20)
 * \endcode
 *
 */ 
/*! \var int ItemSlider::lbitem
 * \brief The index of the current item at the moment the left button is pressed down.
 */


ItemSlider::ItemSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsendthis)
	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendthis)
{
	movewidth=10;
	curitem=-1;
	timerid=0;
	buttondown=0;
	buttondowndevice=0;

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
 /*! Returns id of the new item. */
int ItemSlider::SelectPrevious()
{ 
	if (curitem==-1) return -1;
	curitem--;
	if (curitem<0) curitem=numitems()-1;
	if (win_style & ITEMSLIDER_SENDALL) send();
	DBG cerr <<" Previous Item:"<<curitem<<endl;
	needtodraw=1;
	return getid(curitem);
}

 //! Select the next item.
 /*! Returns id of the new item. */
int ItemSlider::SelectNext()
{
	if (curitem==-1) return -1;
	curitem++;
	if (curitem==numitems()) curitem=0;
	if (win_style & ITEMSLIDER_SENDALL) send();
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
		if (win_style & ITEMSLIDER_SENDALL) send();
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
	buttondown|=LEFTBUTTON;
	buttondowndevice=d->id;

	mx=x; my=y;
	lx=x; ly=y;
	lbitem=curitem;
	//if (!timerid) timerid=app->addmousetimer(this);
	return 0;
}

//! If button is down, then next or previous items are selected rapidly.
/*! Pressing the button down and not moving it causes points to
 * cycle through, as if you were rapidly clicking on the left or
 * right region of the control. Moving the button even just a little
 * cancels this operation.
 *
 * \todo *** need to implement fast scrolling with shift/control!!
 */
int ItemSlider::Idle(int tid)
{
	DBG cerr <<"itemslider idle"<<endl;
	if (!tid) return 0;
	if (tid!=timerid) { app->removetimer(this,tid); return 0; }
	if (!buttondown) { timerid=0; app->removetimer(this,timerid); return 0; }
	if (mx<win_w/2) SelectPrevious();
	if (mx>=win_w/2) SelectNext();
	if (curitem!=lbitem && (win_style & ITEMSLIDER_SENDALL)) send();
	return 0;
}

//! Select a next or previous item.
/*! If button is up on the left side, then SelectPrevious is called.
 * If button is on the right side, then SelectNext is called.
 */
int ItemSlider::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown&LEFTBUTTON || d->id!=buttondowndevice) return 0;
	buttondown&=~LEFTBUTTON;
	if (mx==x && my==y && x<win_w/2) SelectPrevious();
	if (mx==x && my==y && x>=win_w/2) SelectNext();
	if (curitem!=lbitem) send();
	return 0;
}

//! Dragging the mouse horizontally selects previous or next item.
/*! If the mouse is dragged more than movewidth then the next
 * or previous item is selected. If the mouse moves just a little bit,
 * it cancels any idle selecting by setting lx and ly to unreasonable
 * values, which causes Idle to stop the cycling.
 */
int ItemSlider::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (timerid) { timerid=0; app->removetimer(this,timerid); }
	if (!buttondown || d->id!=buttondowndevice) return 1;
	lx=ly=10000;
	if (win_style&ITEMSLIDER_YSHIFT) {
		if (my-y>movewidth) { SelectPrevious(); mx=x; my=y; }
		else if (y-my>movewidth) { SelectNext(); mx=x; my=y; }
	} else {
		if (mx-x>movewidth) { SelectPrevious(); mx=x; my=y; }
		else if (x-mx>movewidth) { SelectNext(); mx=x; my=y; }
	}
	return 0;
}

} // namespace Laxkit

