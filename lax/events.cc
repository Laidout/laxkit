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



#include <lax/anxapp.h>
#include <lax/events.h>
#include <lax/strmanip.h>

namespace Laxkit {

//--------------------------------- Utilities -----------------------------
const char *lax_event_name(int e)
{
	if (e==LAX_RandomEvent) return "LAX_RandomEvent";
	if (e==LAX_onFocusOn) return "LAX_onFocusOn";
	if (e==LAX_onFocusOff) return "LAX_onFocusOff";
	if (e==LAX_onMouseIn) return "LAX_onMouseIn";
	if (e==LAX_onMouseOut) return "LAX_onMouseOut";
	if (e==LAX_onMouseMove) return "LAX_onMouseMove";
	if (e==LAX_onButtonDown) return "LAX_onButtonDown";
	if (e==LAX_onButtonUp) return "LAX_onButtonUp";
	if (e==LAX_onKeyDown) return "LAX_onKeyDown";
	if (e==LAX_onKeyUp) return "LAX_onKeyUp";
	if (e==LAX_onDeviceChange) return "LAX_onDeviceChange";
	if (e==LAX_onSubmit) return "LAX_onSubmit";
	if (e==LAX_onCancel) return "LAX_onCancel";
	if (e==LAX_onContentChange) return "LAX_onContentChange";
	if (e==LAX_onSelectionChange) return "LAX_onSelectionChange";
	if (e==LAX_onUpdateByEvent) return "LAX_onUpdateByEvent";
	if (e==LAX_onUngrayed) return "LAX_onUngrayed";
	if (e==LAX_onGrayed) return "LAX_onGrayed";
	if (e==LAX_onMapped) return "LAX_onMapped";
	if (e==LAX_onUnmapped) return "LAX_onUnmapped";
	if (e==LAX_ControlEvent) return "LAX_ControlEvent";
	if (e==LAX_ButtonEvent) return "LAX_ButtonEvent";
	if (e==LAX_ShortcutEvent) return "LAX_ShortcutEvent";
	if (e==LAX_ColorEvent) return "LAX_ColorEvent";
	if (e==LAX_UserEvent) return "LAX_UserEvent";
	return "(unknown)";
}


//----------------------- EventData classes ------------------------
/*! \class EventData
 * \brief Class for sending data messages between windows.
 * 
 * The default type is LAX_UserEvent.
 *
 * Arbitrary data can be sent between windows with classes derived from EventData.
 * from within a window: \link Laxkit::anXApp::SendMessage app->SendMessage()\endlink
 * <-- app fills in time and window.
 *
 * anXApp calls anXWindow::Event(const EventData *,const char *mes) is called, where mes is the string corresponding to
 * EventData::send_message.
 */


EventData::EventData()
{
	isuserevent=1;
	send_message=NULL;
	type=LAX_UserEvent;

	from=0;
	to=0;
	send_time=0; 
	propagate=0;

	next=NULL;
}

EventData::EventData(const char *message, unsigned long fromwindow, unsigned long towindow)
{
	isuserevent=1;
	type=LAX_UserEvent;
	send_message=newstr(message);
	from=fromwindow;
	to=towindow;
	send_time=0; 
	propagate=0;
	next=NULL;
}

EventData::EventData(int message, unsigned long fromwindow, unsigned long towindow)
{
	isuserevent=1;
	type=message;
	send_message=NULL;
	from=fromwindow;
	to=towindow;
	send_time=0; 
	propagate=0;
	next=NULL;
}

EventData::~EventData()
{
	if (send_message) delete[] send_message;
	if (next) delete next;
}


//---------------------------- RefCountedEventData ----------------------------
/*! \class RefCountedEventData
 * \brief Class to send a reference counted object.
 *
 */

/*! If obj!=NULL, then the object's count is incremented.
 */
RefCountedEventData::RefCountedEventData(anObject *obj)
{
	object=obj;
	if (object) object->inc_count();
	info1=info2=info3=info4=0; 
}

/*! object is deleted.
 */
RefCountedEventData::~RefCountedEventData()
{
	if (object) object->dec_count();
}

//! Return pointer to the object.
/*! The returned object needs to have it's count incremented if it is to be used.
 */
anObject *RefCountedEventData::TheObject()
{ return object; }


//----------------------------- StrEventData ----------------------------------
/*! \class StrEventData
 * \brief A EventData with a new'd char[] (copied from nstr in constructor), since they are so common.
 * 
 * Sent with app->\link Laxkit::anXApp::SendMessage SendMessage\endlink(senddata,atom) <-- app fills in time and window.
 *
 * info can be any number, for instance could indicate the type of thing in str.
 */

StrEventData::StrEventData(unsigned long t, unsigned long f, unsigned long tp, const char *newmes)
{
	to=t;
	from=f;
	type=tp;
	str=NULL;
	info1=info2=info3=info4=0; 
}
 
StrEventData::StrEventData(const char *nstr, int i1,int i2,int i3,int i4,
						   const char *message,unsigned long fromwindow, unsigned long towindow)
		: EventData(message,fromwindow,towindow),
		  info1(i1),
		  info2(i2),
		  info3(i3),
		  info4(i4)
{
	str=NULL;
	makestr(str,nstr); 
}


//------------------------- StrsEventData --------------------------------

/*! \class StrsEventData
 * \brief A EventData with a new'd char[][] (first element copied from nstr in constructor), since they are so common.
 * 
 * Sent with \link Laxkit::anXApp::SendMessage app->SendMessage\endlink(data,towindow,sendthis) <-- app fills in time and window.
 *
 * info can be any number, for instance could indicate the type of thing in str.
 * n is the number of elements in strs;
 */
StrsEventData::StrsEventData()
{
	strs=NULL;
	n=0;
	info=info2=info3=0; 
}

StrsEventData::StrsEventData(const char *nstr,const char *message, unsigned long fromwindow, unsigned long towindow)
		: EventData(message,fromwindow,towindow) 
{
	strs=new char*[1];
	strs[0]=NULL;
	makestr(strs[0],nstr); 
	info=info2=info3=0; 
}

StrsEventData::~StrsEventData()
{
	if (strs) deletestrs(strs,n);
}


//-------------------------- InOutData/FocusChangeData ------------------------
/*! \class InOutData
 * \brief Wrapper for info about a focus on/off, or Enter/Exit for a window or subwindow.
 *
 * ntype can be either LAX_onFocusOn or LAX_onFocusOff.
 */
InOutData::InOutData(int ntype)
  : device(NULL),
	target(NULL),
	x(0),
	y(0),
	child(0)
{ type=ntype; }


//-------------------------- MouseEventData
/*! \class MouseEventData 
 * \brief Hold info about mouse button and motion events.
 *
 * modifiers holds what modifiers are pressed based on a keyboard paired with the mouse,
 * if any.
 *
 * For LAX_onMouseMove, button should be ignored.
 *
 * \todo implement adequate controls for retrieving attached valuator data like pressure, etc.
 */

MouseEventData::MouseEventData(int ntype)
  : x(0),
	y(0),
	button(0),
	count(0),
	size(0),
	pressure(1),
	tilt(0),
	depth(0),
	modifiers(0)
{ type=ntype; }

MouseEventData::~MouseEventData()
{}


//-------------------------- KeyEventData
/*! \class KeyEventData 
 * \brief Hold info about key press or release.
 *
 * For LAX_onKeyUp, buffer and len are NULL and 0.
 *
 * For LAX_onKeyDown, buffer and len might have data, which usually will have
 * resulted from some extra input method helper, like with dead keys to produce
 * composed characters.
 *
 * If the key is a control, shift, alt or similar key, the code will be in key,
 * and buffer will be NULL.
 */

KeyEventData::KeyEventData(int ntype)
  : buffer(NULL),
	len(0)
{ type=ntype; }

KeyEventData::~KeyEventData()
{
	if (buffer) delete[] buffer;
}


//-------------------------- ScreenEventData ------------------------
/*! \class ScreenEventData 
 * \brief Wrapper for areas of a screen that need redrawing.
 *
 * Passed to anXWindow::ExposeChange().
 */
ScreenEventData::ScreenEventData(int xx,int yy,int ww,int hh)
	: x(xx),
	  y(yy),
	  width(ww),
	  height(hh)
{}


//-------------------------- DeviceEventData ------------------------
/*! \class DeviceEventData
 * \brief Event class for device events other than common mouse and keyboard events.
 *
 * Particularly LAX_onDeviceChange, which can say that either some state of a device
 * has changed (LAX_DeviceStateChange), a device's own capabilities have changed
 * (LAX_DeviceChanged), device's controller has changed (for xinput2, this means the
 * active slave device has changed, LAX_DeviceSwitched), or the device hierarchy has
 * changed (LAX_DeviceHierarchyChange). The event->subtype will be set to one
 * of these things.
 *
 * For LAX_DeviceSwitched, this indicates that the new xinput2 slave device 
 * has an xid=subid.
 *
 * LAX_DeviceHierarchyChange events are for each instance of a change in a hierarchy.
 * For instance, when a new mouse is plugged in, X might send 2 hierarchy messages,
 * one to say a new slave device is added, and one to say it is enabled. These 
 * events are translated into DeviceEventData objects. This is mainly useful only
 * for low level utilities, so the X specific data is passed on raw.
 */
//! Device id and subid of relevant device
DeviceEventData::DeviceEventData(int i,int si)
{
	id=i;
	subid=si;

	xflags=xdev=xattachment=xenabled=0;
	xtime=0;
}


//-------------------------- EventReceiver ----------------------------------------
/*! \class EventReceiver
 * \brief Base class for any object that can receive events.
 */
EventReceiver::EventReceiver()
{
	if (anXApp::app) anXApp::app->RegisterEventReceiver(this);
}

EventReceiver::~EventReceiver()
{
	if (anXApp::app) anXApp::app->UnregisterEventReceiver(this);
}

//! Return 1 if event not absorbed, or 0 if it is.
int EventReceiver::Event(const EventData *data,const char *mes)
{ return 1; }



} //namespace Laxkit


