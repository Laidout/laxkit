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
#ifndef _LAX_LAXDEVICES_H
#define _LAX_LAXDEVICES_H

#ifdef _LAX_PLATFORM_XLIB
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef LAX_USES_XINPUT2
#include <X11/extensions/XInput2.h>
#else
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>
#endif //LAX_USES_XINPUT2
#endif //_LAX_PLATFORM_XLIB

#include <lax/events.h>
#include <lax/lists.h>
#include <lax/screeninformation.h>

namespace Laxkit {


class anXWindow;
class EventData;
class EventReceiver;


//-------------------------- LaxDevice ----------------------------------------
class LaxDevice
{
 private:
 public:
	int id;
	int subid; //an identifier, for instance the xid of the slave device which is current for a master device

#ifdef _LAX_PLATFORM_XLIB
	XID xid; //XInput2 uses xid. 0 if no associated x device
#endif
	
	char *name;
	int active; //1 if device is open for business. 0 for not. 
				//touch devices might implement touch by having 20 possible blobs,
				//but at any one time, only some of those will actually be active.

	int screen; //the current screen the device is active on
	int input_head; //tag to indicate which XI2 master device this device is slave to
	int input_group; //groups such as a keyboard, mouse, and tablet
	int input_source; //occasionally, a physical device will produce many devices, like touch points
					  //this value would be the same for all those devices

	virtual const char *DeviceName();
	virtual const char *DeviceType();

	virtual int GetKey(int k); //0 returns number of buttons, 1=on, 0=off
	virtual int GetButton(int b); //0 returns number of buttons, 1=on, 0=off
	virtual double GetValue(int v, int w); //0 returns number of valuators, w=0,1,2=value, min, max

	virtual int numFields();//-2 if unititialized, -1 field type info not available
	virtual int getFieldTypeInfo(int f,const char **nme, int *type, int *min, int *max, int *res);

	virtual int usesX(); //nonzero if uses X
	virtual int fd(); //file descriptor to watch with select(), if not using X
	virtual EventData *getEvents(); //return an event stream if any events pending
	virtual int eventFilter(EventData **events_ret,XEvent *xev,anXWindow *target,int &isinput); //turn an XEvent into EventData
	virtual int selectForWindow(anXWindow *win,unsigned long);
	virtual int clearReceiver(EventReceiver *receiver);

	LaxDevice();
	virtual ~LaxDevice();
	virtual const char *Name(const char *nname);
};


//-------------------------- LaxMouse ----------------------------------------
class LaxKeyboard;

class LaxMouse : public LaxDevice
{
 protected:
 public:
	int buttoncount;
	int button_for_count;
	Time last_button_time;
	unsigned long buttonwindow;
	unsigned long last_leave_window;

	clock_t ttendlimit;
	clock_t ttthreshhold;
	anXWindow *ttwindow;
	unsigned long last_tt;

	LaxKeyboard *paired_keyboard;
	LaxMouse();
	virtual ~LaxMouse();
	virtual int clearReceiver(EventReceiver *receiver);

	virtual void buttonReleased(int button,anXWindow *ww);
	virtual void buttonPressed(Time time, int button,unsigned long windowid);

	virtual int setMouseShape(anXWindow *win, int shape);
	virtual int grabDevice(anXWindow *win);
	virtual int ungrabDevice();
	virtual int getInfo(anXWindow *win,
						int *screen, anXWindow **child,
						double *x, double *y, unsigned int *mods,
						double *pressure, double *tiltx, double *tilty,
						ScreenInformation **screenInfo) = 0;
	virtual int Pressure();
	virtual int TiltX();
	virtual int TiltY();
};


//-------------------------- LaxKeyboard ----------------------------------------
class LaxKeyboard : public LaxDevice
{
 public:
	LaxMouse *paired_mouse;
	anXWindow *current_focus;
	clock_t focus_time;
	LaxKeyboard();
	virtual ~LaxKeyboard();
	virtual int SetFocus(anXWindow *w, clock_t time,int notifyonly);
	virtual unsigned long QueryModifiers();
	virtual int clearReceiver(EventReceiver *receiver);
};


//---------------------------------- DeviceManager --------------------------
class DeviceManager
{
 protected:
 public:
	PtrStack<LaxDevice> devices;
	DeviceManager();
	virtual ~DeviceManager();

	virtual int init();
	virtual int selectForWindow(anXWindow *win,unsigned long); //set proper settings for the window to select for device events
	virtual int eventFilter(EventData **events_ret,XEvent *xev,anXWindow *target,int &isinput); //turn an XEvent into EventData
	virtual int SetFocus(anXWindow *win, LaxKeyboard *kb, clock_t t, int notifyonly);
	virtual int clearReceiver(EventReceiver *receiver);

	virtual int filedescriptors(fd_set *fds, int *n);
	virtual EventData *getEvents(fd_set *fds);//for any devices that do not use X
	virtual LaxDevice *findDevice(int id);
	virtual LaxDevice *findDeviceSubID(int subid);
	virtual LaxMouse *findMouse(int id);

	virtual int NumDevices() { return devices.n; }
	virtual LaxDevice *Device(int i) { return (i>=0 && i<devices.n-1) ? devices.e[i] : NULL; }
};


//---------------------------------- DeviceManagerXlib --------------------------

DeviceManager *newCoreXlibDeviceManager(Display *dpy);


//------------------------------------- CoreXlibPointer -----------------------------------
class CoreXlibKeyboard;

class CoreXlibPointer : public LaxMouse
{
 protected:
 public:
	CoreXlibPointer(CoreXlibKeyboard *kb);
	virtual int usesX() { return 1; } 
	virtual int selectForWindow(anXWindow *win,unsigned long);
	virtual int eventFilter(EventData **events_ret, XEvent *xev, anXWindow *ww, int &isinput);
	virtual int setMouseShape(anXWindow *win, int shape);
	virtual int grabDevice(anXWindow *win);
	virtual int ungrabDevice();
	virtual int getInfo(anXWindow *win,
						int *screen, anXWindow **child,
						double *x, double *y, unsigned int *mods,
						double *pressure, double *tiltx, double *tilty,
						ScreenInformation **screenInfo);
};

//------------------------------------- CoreXlibKeyboard -----------------------------------
class CoreXlibKeyboard : public LaxKeyboard
{
 protected:
 public:
	CoreXlibKeyboard(CoreXlibPointer *p);
	virtual int usesX() { return 1; } 
	virtual int selectForWindow(anXWindow *win,unsigned long);
	virtual int eventFilter(EventData **events_ret, XEvent *xev, anXWindow *ww, int &isinput);
	virtual int SetFocus(anXWindow *win, clock_t t, int notifyonly);
};


//---------------------------------- DeviceManagerXInput2 --------------------------

#ifdef LAX_USES_XINPUT2

DeviceManager *newXInput2DeviceManager(Display *dpy, int which);


//-------------------------- XInput2MasterPointer ----------------------------------------
class XInput2Pointer : public LaxMouse
{
 public:
	int use;
	XInput2Pointer(XIDeviceInfo *d);
	virtual int usesX() { return 1; }
	virtual int eventFilter(EventData **events_ret,XEvent *xev,anXWindow *target,int &isinput);
	virtual int selectForWindow(anXWindow *win,unsigned long);
	virtual int setMouseShape(anXWindow *win, int shape);
	virtual int grabDevice(anXWindow *win);
	virtual int ungrabDevice();
	virtual int getInfo(anXWindow *win,
						int *screen, anXWindow **child,
						double *x, double *y, unsigned int *mods,
						double *pressure, double *tiltx, double *tilty,
						ScreenInformation **screenInfo);
};


//-------------------------- XInput2MasterKeyboard ----------------------------------------
class XInput2Keyboard : public LaxKeyboard
{
 public:
	int use;
	XInput2Keyboard(XIDeviceInfo *d);
	virtual int usesX() { return 1; } 
	virtual int selectForWindow(anXWindow *win,unsigned long);
	virtual int eventFilter(EventData **events_ret, XEvent *xev, anXWindow *ww, int &isinput);
	virtual int SetFocus(anXWindow *win, clock_t t,int notifyonly);
};



#endif //LAX_USES_XINPUT2


//---------------------------------- DeviceManagerXInput1 --------------------------
//DeviceManager *createXInput1DeviceManager(Display *dpy, int which);


} //namespace Laxkit

#endif

