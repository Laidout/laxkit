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
//    Copyright (C) 2010-2013 by Tom Lechner
//
#ifndef _LAX_EVENTS_H
#define _LAX_EVENTS_H

#include <lax/configured.h>

#ifdef _LAX_PLATFORM_XLIB
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xutil.h>
#endif //_LAX_PLATFORM_XLIB


#include <sys/times.h>

#include <lax/anobject.h>
#include <lax/laxdevices.h>


namespace Laxkit {

class anXWindow;
class anXApp;
class LaxDevice;
class LaxMouse;
class LaxKeyboard;


//-------------------------- Event related classes ----------------------------------

//----- EventData mask types -------
//These are used in EventData::type.

 //Used by various control windows to determine when
 //to send a message to its owner
enum LaxEventType {
	 LAX_RandomEvent = 0,
	  //input and device events
	 LAX_onFocusOn,
	 LAX_onFocusOff,
	 LAX_onMouseIn,
	 LAX_onMouseOut,
	 LAX_onMouseMove,
	 LAX_onButtonDown,
	 LAX_onButtonUp,
	 LAX_onKeyDown,
	 LAX_onKeyUp,
	 LAX_onDeviceChange,
	 LAX_onFrame,
	  //state events
	 LAX_onSubmit,
	 LAX_onCancel,
	 LAX_onContentChange,
	 LAX_onSelectionChange,
	 LAX_onUpdateByEvent,
	 LAX_onUngrayed,
	 LAX_onGrayed,
	 LAX_onMapped,
	 LAX_onUnmapped,
	 LAX_onThemeChange,
	  //---other events
	  //sent by window controls for various purposes.
	  //event->subtype will be some value that makes sense to the control
	 LAX_ControlEvent,
	 LAX_ButtonEvent,
	 LAX_ShortcutEvent,
	 LAX_ColorEvent,
	 LAX_UserEvent,
	  //for when an event is preempted, a flag to ignore:
	 LAX_DefunctEvent,

	 LAX_MAX_BUILTIN_EVENT
};

const char *lax_event_name(int e);



//-------------------------- EventData
class EventData
{
	friend class anXApp;
 private:
	int isuserevent;
 public:
	LaxEventType type;
	unsigned long subtype;
	int usertype;
	char *send_message;

	unsigned long from; //EventReceiver object_id
	unsigned long to;
	//long info[5];
	int propagate;

	clock_t send_time;       //as returned by times()
	unsigned long xlib_time; //as sent in xlib messages

	EventData *next;

	EventData();
	EventData(const char *message,  unsigned long fromwindow=0, unsigned long towindow=0);
	EventData(LaxEventType message, unsigned long fromwindow=0, unsigned long towindow=0);
	virtual ~EventData();
};

#ifdef _LAX_PLATFORM_XLIB
//-------------------------- XEventData
class XEventData : public EventData
{
 public:
	XEvent *xevent;
};
#endif //_LAX_PLATFORM_XLIB


//-------------------------- StrEventData
class SimpleMessage : public EventData
{
 public:
	char *str;
	anObject *object;
	int info1,info2,info3,info4;

	SimpleMessage() { object=NULL; str=NULL; info1=info2=info3=info4=0; }
	SimpleMessage(anObject *obj);
	SimpleMessage(unsigned long t, unsigned long f, LaxEventType tp, const char *newmes=NULL);
	SimpleMessage(const char *nstr, int i1,int i2,int i3,int i4,
				 const char *message=NULL,unsigned long fromwindow=0, unsigned long towindow=0);
	virtual ~SimpleMessage();
	anObject *TheObject() const;
};

typedef SimpleMessage StrEventData;
typedef SimpleMessage RefCountedEventData;


//-------------------------- StrsEventData
class StrsEventData : public EventData
{
 public:
	anObject *object;
	char **strs;
	int n; //number of strs
	int info,info2,info3;
	StrsEventData();
	StrsEventData(const char *nstr,const char *message, unsigned long fromwindow, unsigned long towindow);
	virtual ~StrsEventData();
};

//-------------------------- InOutData/EnterExitData/FocusChangeData
class InOutData : public EventData
{
 public:
	//****
	LaxDevice *device;
	anXWindow *target;
	int x,y;
	unsigned long child;
	InOutData(LaxEventType ntype);
};

typedef InOutData FocusChangeData;
typedef InOutData EnterExitData;

//-------------------------- ButtonEventData
class MouseEventData : public EventData
{
 public:
	int x,y;
	int button, count, size;
	double pressure, tilt, depth;
	unsigned int modifiers; //of paired keyboard, if any

	anXWindow *target;
	LaxMouse *device;

	MouseEventData(LaxEventType ntype);
	virtual ~MouseEventData();
};

//-------------------------- KeyEventData
class KeyEventData : public EventData
{
 public:
	unsigned int key;
	unsigned int modifiers;
	char *buffer;
	int len;

	LaxKeyboard *device;
	anXWindow *target;

	KeyEventData(LaxEventType ntype);
	virtual ~KeyEventData();
};

//-------------------------- ScreenEventData
class ScreenEventData : public EventData
{
 public:
	int x,y,width,height; //rectangle that needs refreshing
	ScreenEventData(int xx,int yy,int ww,int hh);
};

//-------------------------- DeviceEventData
#define LAX_DeviceStateChange      1
#define LAX_DeviceChanged          2
#define LAX_DeviceSwitched         3
#define LAX_DeviceHierarchyChange  4

class DeviceEventData : public EventData
{
 public:
	int id,subid; //device id and subid of relevant device

	int xflags, xdev, xattachment, xenabled;
	Time xtime;
	DeviceEventData(int i=0,int si=0);
};


//-------------------------- EventReceiver ----------------------------------------
class EventReceiver : virtual public anObject
{
 public:
	EventReceiver();
	virtual ~EventReceiver();
	virtual int Event(const EventData *data,const char *mes);
	virtual int  Idle(int tid, double delta) { return 1; } //1 means remove timer. *** should make a specific Frame event for more detail??
};


} //namespace Laxkit

#endif

