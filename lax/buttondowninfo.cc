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
//    Copyright (C) 2010 by Tom Lechner
//

#include <lax/buttondowninfo.h>

namespace Laxkit {

/*! \class ButtonDownInfo
 * \brief Class to simplify keeping track of what buttons have been pressed.
 *
 * Typically, windows will have their own instance of one of these classes
 * to track what's been pressed in itself.
 *
 * The data stored is not necessarily related to LaxDevice buttons. You can keep track of
 * anything identified with any integer. You can store arbitrary data, as long as
 * it is contained in an object derived from anObject.
 *
 * When up() is called, it returns a rough approximation of how much the position has been
 * dragged. If the position changes at all, then this drag value will be 1 or more.
 */


//---------------------------ButtonDownInfoSpecific

ButtonDownInfo::ButtonDownInfoSpecific::ButtonDownInfoSpecific(int d, int b, int x, int y, int i1,int i2,
																anObject *e,int absorbcount)
		: device(d), button(b),
		  initial_x(x), initial_y(y),
		  last_x(x), last_y(y),
		  current_x(x), current_y(y),
		  dragged(0),
		  info1(i1),
		  info2(i2),
		  extra(NULL),
		  next(NULL) 
{
	if (e) { extra=e; if (!absorbcount) e->inc_count(); }
}

/*! Deletes next and dec_counts extra.
 */
ButtonDownInfo::ButtonDownInfoSpecific::~ButtonDownInfoSpecific()
{
	if (next) delete next;
	if (extra) extra->dec_count();
}


//---------------------------ButtonDownInfo

//! Return the info object for a device and button that already exists.
ButtonDownInfo::ButtonDownInfoSpecific *ButtonDownInfo::exists(int d, int b)
{
	ButtonDownInfoSpecific *i=info;
	while (i) {
		if (i->device==d && i->button==b) return i;
		i=i->next;
	}
	return NULL;
}

//! Flush any collected info.
void ButtonDownInfo::clear()
{
	if (info) delete info;
	info=NULL;
}

//! Set up to remember that button_id has been pressed on device_id.
/*! If the button/id combo is already there, then simply replace all the old info with the new.
 */
void ButtonDownInfo::down(int device_id, int button_id, int x, int y, int i1,int i2,anObject *e,int absorb)
{
	ButtonDownInfoSpecific *i=NULL;
	if (i=exists(device_id, button_id), i) {
		i->initial_x=i->last_x=i->current_x=x;
		i->initial_y=i->last_y=i->current_y=y;
		i->dragged=0;
		i->info1=i1;
		i->info2=i2;
		if (i->extra) i->extra->dec_count();
		i->extra=e;
		if (e && !absorb) e->inc_count();
		return;
	}
	if (!info) info=new ButtonDownInfoSpecific(device_id, button_id, x,y, i1,i2, e,absorb);
	else {
		ButtonDownInfoSpecific *i=info;
		while (i->next) i=i->next;
		i->next=new ButtonDownInfoSpecific(device_id, button_id, x,y, i1,i2, e,absorb);
	}
}

//! Find the average position of all devices with button_id down.
/*! Returns the number of devices with button_id down.
 *
 * If button_id<0 then average all current position info.
 */
int ButtonDownInfo::average(int button_id, int *xavg, int *yavg)
{
	*xavg=*yavg=0;
	int count=0;
	for (ButtonDownInfoSpecific *i=info; i; i=i->next) {
		if (button_id>=0 && i->button!=button_id) continue;

		*xavg+=i->current_x;
		*yavg+=i->current_y;
		count++;
	}
	if (count) { *xavg/=count; *yavg/=count; }
	return count;
}

//! Updates any position tracking, for any button for the given device.
/*! Please note that if you are looking for lastx and lasty, it might return those
 * coordinates from any button press, maybe not the button you were looking for.
 *
 * Returns whether the mouse is considered to have been dragged yet. This will be very approximately
 * the maximum distance the mouse was from the initial point.
 */
int ButtonDownInfo::move(int device_id, int x, int y, int *lastx, int *lasty)
{
	ButtonDownInfoSpecific *i;
	int dragged=0;
	for (i=info; i; i=i->next) {
		if (i->device!=device_id) continue;

		if (i->dragged==0 && (x!=i->current_x || y!=i->current_y)) i->dragged=1;
		i->last_x=i->current_x;
		i->last_y=i->current_y;
		i->current_x=x;
		i->current_y=y;

		int d=abs(x-i->initial_x) + abs(y-i->initial_y);
		if (d>i->dragged) i->dragged=d;

		if (lastx) *lastx=i->last_x;
		if (lasty) *lasty=i->last_y;

		dragged=i->dragged;
	}
	return dragged;
}

//! Updates any extra info.
/*! If button_id==0 then affect any button for the given device.
 * If device_id==0 then affect any device with the given button down.
 *
 * If oldi1 or oldi2 are not NULL, then set with the old info.
 */
void ButtonDownInfo::moveinfo(int device_id, int button_id, int i1,int i2,int *oldi1,int *oldi2)
{
	ButtonDownInfoSpecific *i;
	for (i=info; i; i=i->next) {
		if (device_id && i->device!=device_id) continue;
		if (button_id && i->button!=button_id) continue;

		if (oldi1) *oldi1=i->info1;
		if (oldi2) *oldi2=i->info2;

		i->info1=i1;
		i->info2=i2;
	}
}

//! Button has come up, so remove tracking of it.
/*! Returns whether there has been dragging since tracking. This will be very approximately
 * the maximum distance the mouse was from the initial point.
 *
 * If the button was not down for the device, -1 is returned.
 *
 * If i1 or i2 are not NULL, then set them to the info1 and info2 for
 * device and button.
 */
int ButtonDownInfo::up(int device_id, int button_id, int *i1, int *i2)
{
	if (!info) return 0;
	ButtonDownInfoSpecific *i=info,*ii=NULL;
	while (i) {
		if (i->device==device_id && i->button==button_id) break;
		ii=i;
		i=i->next;
	}

	if (!i) return -1; //info didn't exist

	 //detach i from info list
	if (!ii) info=i->next; //object==info
	else ii->next=i->next;

	i->next=NULL;
	int d=i->dragged;
	if (i1) *i1=i->info1;
	if (i2) *i2=i->info2;
	delete i;
	return d;
}

//! Return how many of the device id are down for that button, else return 0.
/*! If device_id==0 and button_id!=0, then return how many of any device is down for that button.
 *  If device_id!=0 and button_id==0, then return how many buttons are down on that device.
 * If button_id!=0 and device_id!=0, then return 1 if that device has that button down.
 * If button_id==0 and device_id==0, then return the total number of button-device combinations are active.
 *
 * If device!=NULL, then return the id of the first device that satisfies the above. If there
 * is no such device, then device does not get assigned anything.
 */
int ButtonDownInfo::any(int device_id,int button_id,int *device)
{
	if (!info) return 0;

	int n=0;
	ButtonDownInfoSpecific *i=info;
	while (i) {
		if (device_id==0 && button_id==0) {
			n++;
			if (n==1 && device) *device=i->device;
		} else if (device_id==0 && button_id==i->button) {
			n++;
			if (n==1 && device) *device=i->device;
		} else if (button_id==0 && device_id==i->device) {
			n++;
			if (n==1 && device) *device=i->device;
		} else if (i->device==device_id && i->button==button_id) {
			n++;
			if (n==1 && device) *device=i->device;
			return n; //there can be only one of these, so might as well return
		}

		i=i->next;
	}
	return n;
}

//! Get only the extra info1 and info2 for a button press.
/*! Return 0 for info found, or nonzero for error.
 */
int ButtonDownInfo::getextrainfo(int device_id, int button_id, int *i1,int *i2)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return 1;

	if (i1) *i1=i->info1;
	if (i2) *i2=i->info2;

	return 0;
}

//! Get current position info for a button press.
/*! Return 0 for info found, or nonzero for error.
 */
int ButtonDownInfo::getcurrent(int device_id, int button_id, 
		int *xc, //!< Current x
		int *yc //!< Current y
		)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return 1;

	if (xc) *xc=i->current_x;
	if (yc) *yc=i->current_y;

	return 0;
}

//! Get previous position info for a button press.
/*! Return 0 for info found, or nonzero for error.
 */
int ButtonDownInfo::getlast(int device_id, int button_id, 
		int *xp, //!< Previous x
		int *yp //!< Previous y
		)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return 1;

	if (xp) *xp=i->last_x;
	if (yp) *yp=i->last_y;

	return 0;
}

//! Get initial info for a button press.
/*! Return 0 for info found, or nonzero for error.
 */
int ButtonDownInfo::getinitial(int device_id, int button_id, 
		int *x0, //!< Initial x
		int *y0 //!< Initial y
		)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return 1;

	if (x0) *x0=i->initial_x;
	if (y0) *y0=i->initial_y;

	return 0;
}

//! Get current info for a button press.
/*! Return 0 for info found, or nonzero for error.
 */
int ButtonDownInfo::getinfo(int device_id, int button_id, 
		int *x0, //!< Initial x
		int *y0, //!< Initial y
		int *xp, //!< Last x
		int *yp, //!< Last y
		int *xc, //!< Current x
		int *yc, //!< Current y
		int *i1, //!< Extra info 1
		int *i2  //!< Extra info 2
		)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return 1;

	if (x0) *x0=i->initial_x;
	if (y0) *y0=i->initial_y;

	if (xp) *xp=i->last_x;
	if (yp) *yp=i->last_y;

	if (xc) *xc=i->current_x;
	if (yc) *yc=i->current_y;

	if (i1) *i1=i->info1;
	if (i2) *i2=i->info2;

	return 0;
}

//! Return the device id of the mouse that is down.
/*! If afterthis!=0, then search for any devices down AFTER the device
 * with device id == afterthis. This allows enumerating all devices listed as down.
 *
 * If button_id!=0, then search only for this kind.
 *
 * Returns 0 if none down.
 *
 * To find a device for a specific id or button, see any().
 *
 * \todo this fails when many buttons are down for the same device...
 */
int ButtonDownInfo::whichdown(int afterthis, int button_id)
{
	ButtonDownInfoSpecific *i=info;
	if (afterthis) {
		while (i && i->device!=afterthis) i=i->next;
		if (i) i=i->next;
	}
	if (button_id) while (i && i->button!=button_id) i=i->next;
	if (i) return i->device;
	return 0;
}

//! Return 1 if button_id of device_id is logged as down, else return 0.
/*! If device_id==0, then return the number of devices that say that button is down.
 *
 * If device_id!=0 and i1 or i2 are not NULL, then also return the info1 and info2
 * for that button on that device. Otherwise i1 and i2 are not changed.
 */
int ButtonDownInfo::isdown(int device_id, int button_id, int *i1,int *i2)
{
	if (device_id!=0) {
		ButtonDownInfoSpecific *i=exists(device_id,button_id);
		if (!i) return 0;
		if (i1) *i1=i->info1;
		if (i2) *i2=i->info2;
		return 1;
	}

	int count=0;
	ButtonDownInfoSpecific *i=info;
	while (i) {
		if (i->button==button_id) count++;
		i=i->next;
	}
	return count;
}

//! Return whether the device has been dragged since tracking the button.
int ButtonDownInfo::isdragged(int device_id, int button_id)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return 0;

	return i->dragged;
}

//! Return the extra data if any from the info.
/*! This does nothing to extra's count. If you intend to keep it beyond the buttoninfo, then
 * you must increment the count.
 */
anObject *ButtonDownInfo::getextra(int device_id, int button_id)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return NULL;

	return i->extra;
}

//! Replace arbitrary extra info of a device/button combination.
/*! If the device+button does not exist, then nothing is done, and 1 is returned.
 * Otherwise e replaces any extra data (if any), and 0 is returned.
 *
 * If absorbcount, then absorb e's count, otherwise, its count will be incremented.
 * The old data will have its count decremented.
 *
 * If e==NULL, then any old data is removed, and NULL replaces it.
 */
int ButtonDownInfo::replaceextra(int device_id, int button_id,anObject *e,int absorbcount)
{
	ButtonDownInfoSpecific *i=exists(device_id,button_id);
	if (!i) return 1;

	if (i->extra && i->extra!=e) i->extra->dec_count();
	i->extra=e;
	if (!absorbcount && e) e->inc_count();

	return 0;
}


} //namespace Laxkit

