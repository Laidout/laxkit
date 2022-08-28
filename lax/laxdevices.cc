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
//    Copyright (C) 2010 by Tom Lechner
//

#include <lax/anxapp.h>
#include <lax/laxdevices.h>
#include <lax/strmanip.h>
#include <lax/lists.cc>
#include <lax/mouseshapes.h>

#include <sys/times.h>

//stdint needed before XI2proto.h
#include <stdint.h>
//#include <X11/extensions/XI2proto.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

// *** I'm never quite sure if times(NULL) has any bad side effects!!
static clock_t gettime()
{
	static struct tms tms;
	return times(&tms);
}

//-------------------------- LaxDevice ----------------------------------------
/*! \class LaxDevice
 * \brief Class to hold info about various input devices.
 *
 * These might be knobs, midi devices, or perhaps wiimotes.
 * 
 * \todo maybe have default device objects for LaxMouse, LaxKeyboard, 
 * 		Touchpad, MultiTouchPad, Midi, Wiimote
 */

//class Valuator
//{
// public:
//	 string name; //not in x, individually name each field...
//	 int minvalue;
//	 int maxvalue;
//	 int resolution;
//};


LaxDevice::LaxDevice()
	: id(getUniqueNumber()),
	  subid(0),
	  xid(0),
	  name(NULL),
	  active(1),
	  screen(0),
	  input_head(0),
	  input_group(0),
	  input_source(0)
{}

//! Empty virtual destructor.
LaxDevice::~LaxDevice() 
{
	if (name) delete[] name;
}

//! Name the device.
const char *LaxDevice::Name(const char *nname)
{
	makestr(name,nname);
	return name;
}

//! Default is just to return name.
const char *LaxDevice::DeviceName()
{ return name; }

//! Default just returns NULL.
const char *LaxDevice::DeviceType()
{ return NULL; }

//! Remove all reference to receiver.
/*! Return 0 for successful clearing, or nonzero for error or not found.
 */
int LaxDevice::clearReceiver(EventReceiver *receiver)
{ return 1; }

//----------Device querying functions

//! -1 returns number of buttons. Otherwise check status of that button number: return 1=on, 0=off.
int LaxDevice::GetKey(int k)
{
	cerr <<" *** must implement LaxDevice::GetKey()!!"<<endl;
	return 0;
}
			
//! -1 returns number of buttons. Otherwise check status of that button number: return 1=on, 0=off.
int LaxDevice::GetButton(int b)
{
	//XDeviceState *state=XQueryDeviceState(app->dpy,xlib_device);
	//XFreeDeviceState(state);
	//if (b==0) {
	//	if (xlib_deviceinfo) 
	//}

	cerr <<" *** must implement LaxDevice::GetButton()!!"<<endl;
	return 0;
}

//! -1 returns number of valuators. Otherwise check information of that valuator.
/*! w=0,1,2=value, min, max
 */
double LaxDevice::GetValue(int v, int w)
{
	cerr <<" *** must implement LaxDevice::GetValue()!!"<<endl;
	return 0;
}

//-----------Device info, input, and message selecting functions

/*! Return -2 if unititialized, -1 field type info not available
 */
int LaxDevice::numFields()
{ return -1; }

//! Retrieve device field info.
/*! nme is a name describing the field, such as "Keys", "Buttons", or "Pressure".
 *
 * Type can be 0 for key, 1 for button, 2 for value.
 *
 * For keys, min and max are the min and max keycodes.*** really?
 * For buttons, min is 0, and max is the number of buttons available.
 * For values, returns the minimum value, maximum value, and resolution of the value.
 *
 * Return 0 for successful return of info, else 1 for info not available.
 */
int LaxDevice::getFieldTypeInfo(int f, const char **nme, int *type, int *min, int *max, int *res)
{ return 1; }

//! Return nonzero if the device gets messages through X.
int LaxDevice::usesX()
{ return 0; }

//file descriptor to watch with select(), if not using X
int LaxDevice::fd()
{ return 0; }

//! Return an event stream if any events pending
EventData *LaxDevice::getEvents()
{ return NULL; }

//! Turn an XEvent into EventData object(s).
/*! Return 1 if event is absorbed, and nothing else should try to parse it. Use the returned EventDatas instead.
 * Return 0 if event is ignored.
 */
int LaxDevice::eventFilter(EventData **events_ret,XEvent *xev,anXWindow *target,int &isinput)
{ return 0; }

//! Called after anXWindow::xlib_window exists, this sets up anything specific so the window gets related events.
/*! Return 0 for successful selection, nonzero for nothing done.
 */
int LaxDevice::selectForWindow(anXWindow *win,unsigned long)
{
	return 1;
}

//-------------------------- LaxMouse ----------------------------------------
/*! \class LaxMouse
 * \brief LaxDevice subclass for pointers.
 *
 * This class will keep track of how many times a button is pressed rapidly, in
 * the same window. Note that it will keep track of only only one button at a time.
 */
/*! \var int LaxMouse::buttoncount
 * \brief The running count of a button being pressed rapidly.
 *
 * If double clicking (clicking under the time in anXApp::dblclk), buttoncount==2, for instance.
 * 
 * If you click ten times and each click is within anXApp::dblclk of the last click, then
 * buttoncount will be 10.
 */
/*! \var int LaxMouse::button_for_count
 * \brief Index of button, tracked to update buttoncount.
 */
/*! \var Time LaxMouse::last_button_time
 * \brief Time of the last button press of the same type for the same window.
 */
/*! \var unsigned long LaxMouse::buttonwindow
 * \brief The object_id of the last window the button was clicked down in.
 */
/*! \var anXWindow *LaxMouse::ttwindow
 * \brief A window under consideration for a tooltip.
 */
/*! \var clock_t LaxMouse::ttendlimit
 * \brief Clock time that must be passed without movement for there to be a tooltip.
 */
/*! \var clock_t LaxMouse::ttthreshhold
 * \brief Clock time after entering to allow movement, before considering tooltips.
 */

LaxMouse::LaxMouse()
  :	buttoncount(0),
	button_for_count(-1),
	last_button_time(0),
	buttonwindow(0),
	last_leave_window(0),
	ttendlimit(0),
	ttthreshhold(0),
	ttwindow(NULL),
	last_tt(0),
	paired_keyboard(NULL)
{}

LaxMouse::~LaxMouse()
{
	if (ttwindow) ttwindow->dec_count();
}

//! Clear ttwindow if necessary.
int LaxMouse::clearReceiver(EventReceiver *receiver)
{
	if (ttwindow==receiver) {
		if (ttwindow) {
			DBG cerr <<"clearing laxmouse receiver: "<<receiver->object_id<<endl;
			ttwindow->dec_count();
		}
		ttwindow=NULL;
		ttthreshhold=ttendlimit=0;
	}
	return 0;
}

//! Resets buttoncount, which helps keep track of double, triple, etc clicks.
void LaxMouse::buttonReleased(int button,anXWindow *ww)
{
	if (!ww || (ww && buttonwindow!=ww->object_id) || button != button_for_count) buttoncount=0;
}

//! Update buttoncount, which helps keep track of double, triple, etc clicks.
void LaxMouse::buttonPressed(Time time, int button,unsigned long windowid)
{
	if (time-last_button_time < anXApp::app->dblclk) {
		if (windowid==buttonwindow && button==button_for_count) buttoncount++;
	} else buttoncount=1;

	last_button_time=time;
	button_for_count=button;
	buttonwindow=windowid;
}

/*! Return 0 for success, nonzero for error.
 */
int LaxMouse::setMouseShape(anXWindow *win, int shape)
{ return 1; }

/*! Return 0 for success, 1 for failure.
 */
int LaxMouse::grabDevice(anXWindow *win)
{ return 1; }

/*! Return 0 for success, 1 for failure.
 */
int LaxMouse::ungrabDevice()
{ return 1; }

double LaxMouse::Pressure() const
{
	double pressure = 1;
	getInfo(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &pressure, nullptr, nullptr, nullptr);
	return pressure;
}

double LaxMouse::TiltX() const
{
	double tilt = 0;
	getInfo(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &tilt, nullptr, nullptr);
	return tilt;
}

double LaxMouse::TiltY() const
{
	double tilt = 0;
	getInfo(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &tilt, nullptr);
	return tilt;
}

void LaxMouse::Tilt(double *x, double *y) const
{
	getInfo(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, x, y, nullptr);
}


//-------------------------- LaxKeyboard ----------------------------------------
/*! \class LaxKeyboard
 * \brief LaxDevice subclass for keyboards.
 *
 * \todo *** XIM: x input context stuff is crazy! need either a good explanation with working
 *   examples, or some simpler system.
 */


LaxKeyboard::LaxKeyboard()
 :  paired_mouse(NULL),
	current_focus(NULL),
	focus_time(0)
{}

LaxKeyboard::~LaxKeyboard()
{
	if (current_focus) current_focus->dec_count();
}

/*! Remember that keyboard stores time as returned by times(), not Xlib time.
 * If notifyonly, then this has been called in response to an event, not as a request
 * to grab the focus.
 *
 * Return 0 for focus changed. 1 for not changed, an error occured perhaps.
 */
int LaxKeyboard::SetFocus(anXWindow *w, clock_t time,int notifyonly)
{
	if (current_focus!=w) {
		if (current_focus) current_focus->dec_count();
		current_focus=w;
		if (current_focus) current_focus->inc_count();
	}
	if (time) focus_time=time;
	return 0;
}

//! Set current_focus to NULL if receiver->object_id==current_focus->object_id.
int LaxKeyboard::clearReceiver(EventReceiver *receiver)
{
	if (current_focus && receiver && receiver->object_id==current_focus->object_id) {
		current_focus->dec_count();
		current_focus=NULL;
	}
	return 0;
}

unsigned long LaxKeyboard::QueryModifiers()
{
	cerr <<" *** must implement LaxKeyboard::QueryModifiers()!!"<<endl;
	return 0;
}


//---------------------------------- DeviceManager --------------------------

/*! \class DeviceManager
 * \brief Class to translate various incoming device events into Lax events.
 *
 * If you care about events outside of core X events, you would use a DeviceManagerXinput2.
 * Otherwise, for only core events, use a DeviceManagerXlib.
 *
 * \todo *** the plan is to have a handler not only for Xinput1, Xinput2, but also any
 *   other devices, such as TUIO, wiimotes, midi, general osc, etc... still
 *   debating the best way to do this.
 */


DeviceManager::DeviceManager() {}
DeviceManager::~DeviceManager() {}

int DeviceManager::init() { return 0; }


//! Set proper settings for the window to select for device events.
/*! Default is simply to call LaxDevice::selectForWindow() with each device.
 *
 * Note that this will be called automatically by anXApp::addwindow() after
 * anXWindow::xlib_window exists.
 */
int DeviceManager::selectForWindow(anXWindow *win,unsigned long mask)
{
	for (int c=0; c<devices.n; c++) {
		devices.e[c]->selectForWindow(win,mask);// *** how should mask work here?!?!?
	}
	return 0;
}

//! Tries to get the keyboard to set the focus.
/*! If the focus cannot be set based on the given information, return 1.
 * If successful setting of focus, return 0;
 *
 * If notifyonly, then this has been called in response to an event, not as a request
 * to grab the focus.
 *
 * This will typically prompt future focus on and off events. For instance,
 * a core Xlib keyboard will use XSetInputFocus(), which will generate 
 * Xlib FocusIn and FocusOut events.
 *
 * \todo *** need to figure out an mpx way of handling WM_TAKE_FOCUS!!
 */
int DeviceManager::SetFocus(anXWindow *win, LaxKeyboard *kb, clock_t t,int notifyonly)
{
	DBG if (!kb) cerr<<"WARNING!!! SetFocus with NULL keyboard!!"<<endl;

	if (!kb) {
		 //if no keyboard specified, just grab 1st keyboard you find..
		 //This is really to cover up for WM_TAKE_FOCUS message, which doesn't say what mouse
		 //just entered a window.. ***need to figure out an mpx way of handling WM_TAKE_FOCUS!!
		for (int c=0; c<devices.n; c++) {
			if (dynamic_cast<LaxKeyboard*>(devices.e[c])) { kb=dynamic_cast<LaxKeyboard*>(devices.e[c]); break; }
		}
	}

	if (kb) return kb->SetFocus(win,t,notifyonly);
	return 1;
}

//! Maybe turn an XEvent into zero or more EventData objects.
/*! This will overwrite whataver was in *events_ret, replacing it with either a new event list,
 * or NULL.
 *
 * Return 0 if event is ignored.
 * Return 1 for XEvent absorbed, and the returned EventData objects should be used instead (if any).
 */
int DeviceManager::eventFilter(EventData **events_ret,XEvent *xev,anXWindow *target,int &isinput)
{
	EventData *e=NULL;
	for (int c=0; c<devices.n; c++) {
		if (devices.e[c]->usesX() && devices.e[c]->eventFilter(&e,xev,target,isinput)) {
			*events_ret=e;
			return 1;
		}
	}
	*events_ret=NULL;
	return 0;
}

//! Set any file descriptors in fds to pass on to select() in event loop.
/*! Return the number of file descriptors set in n_ret.
 * Return value is the maximum value of the descriptors (select() needs this).
 *
 * This uses LaxDevice::fd() and FD_SET() to set on the fds set.
 */
int DeviceManager::filedescriptors(fd_set *fds, int *n_ret)
{
	int max=0,fd=-1;
	for (int c=0; c<devices.n; c++) {
		fd=devices.e[c]->fd();
		if (fd) {
			FD_SET(fd,fds);
			if (fd>max) max=fd;
		}
	}
	return fd;
}

//! Return a pointer to the device with the given id, or NULL if not found.
LaxDevice *DeviceManager::findDevice(int id)
{
	for (int c=0; c<devices.n; c++) if (devices.e[c]->id==id) return devices.e[c];
	return NULL;
}

//! Return a pointer to the device with the given id, or NULL if not found.
LaxDevice *DeviceManager::findDeviceSubID(int subid)
{
	for (int c=0; c<devices.n; c++) if (devices.e[c]->subid == subid) return devices.e[c];
	return NULL;
}

//! Find a LaxMouse with the id, or any mouse if id==0.
LaxMouse *DeviceManager::findMouse(int id)
{
	for (int c=0; c<devices.n; c++) 
		if (dynamic_cast<LaxMouse*>(devices.e[c]) && (id==0 || devices.e[c]->id==id)) 
			return dynamic_cast<LaxMouse*>(devices.e[c]);
	return NULL;
}

//! For any devices that do not use X, this allows devices to parse any pending events.
EventData *DeviceManager::getEvents(fd_set *fds)
{
	cerr <<" *** implement something right for DeviceManager::getEvents()"<<endl;
	return NULL; 
}

//! This gets called when a window is removed, and all references to it must be cleared.
/*! Removes focus reference.
 * Also remove any tooltip consideration reference.
 *
 * Return 0 for focus removed. nonzero for focus not found.
 */
int DeviceManager::clearReceiver(EventReceiver *receiver)
{
	if (!receiver) return 1;

	 //remove focus reference if w is a focus, or the focus is in a child of w
	LaxKeyboard *kb=NULL;
	for (int c=0; c<devices.n; c++) {
		kb=dynamic_cast<LaxKeyboard*>(devices.e[c]);
		if (!kb || !kb->current_focus) continue;
		if (receiver==kb->current_focus 
			|| IsWindowChild(dynamic_cast<anXWindow*>(receiver),kb->current_focus)) kb->SetFocus(NULL,0,1);
	}

	 //clear window from tooltip consideration
	//***if (w==ttmaybe || (ttmaybe && IsWindowChild(w,ttmaybe))) { ttmaybe=NULL; ttendlimit=0; }

	return 0;
}


//------------------------------------- CoreXlibPointer -----------------------------------
/*! \class CoreXlibPointer
 * \brief Device that selects for core mouse events.
 */


CoreXlibPointer::CoreXlibPointer(CoreXlibKeyboard *kb)
{
	Name("Core Pointer");
	paired_keyboard=kb;
	if (kb && kb->paired_mouse!=this) kb->paired_mouse=this;
}

/*! Return 0 for success, nonzero for error.
 */
int CoreXlibPointer::setMouseShape(anXWindow *win, int shape)
{
	if (!shape) { XUndefineCursor(anXApp::app->dpy, win->xlib_window); return 0; }
	if (shape==LAX_MOUSE_Cancel) shape=0;
	Cursor cursor;
	cursor=XCreateFontCursor(anXApp::app->dpy, shape);
	XDefineCursor(anXApp::app->dpy, win->xlib_window, cursor);
	XFreeCursor(anXApp::app->dpy,cursor);
	return 0;
}

/*! Return 0 for success, 1 for failure.
 *
 * \todo what on earth does XUngrabPointer() actually return??
 */
int CoreXlibPointer::ungrabDevice()
{
	return XUngrabPointer(anXApp::app->dpy,CurrentTime);
}

/*! Return 0 for success, 1 for failure.
 */
int CoreXlibPointer::grabDevice(anXWindow *win)
{
	if (!win || !win->xlib_window) return 1;

	if (XGrabPointer(anXApp::app->dpy,win->xlib_window,False,
					 ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
					 GrabModeAsync,GrabModeAsync,
					 None,None,CurrentTime) == GrabSuccess) return 0;
	return 1;
}

//! Get information about the mouse state.
/*! If you pass NULL for any field, then that field is ignored.
 *
 * If win!=NULL, then return coordinates in win space. If the window is not on the same screen as
 * the mouse, then return 1, and only screen gets set.
 *
 * Otherwise return screen coordinates. The screen the mouse is actually in gets set.
 *
 * Return 0 for success, or 1 for mouse not on same screen, or other number for mouse info
 * not available for some reason.
 */
int CoreXlibPointer::getInfo(anXWindow *win,
							 int *screen, anXWindow **child,
							 double *x, double *y, unsigned int *mods,
							 double *pressure, double *tiltx, double *tilty, ScreenInformation **screenInfo) const //extra goodies
{
	Window rt=0,chld=0, xwin=0;
	int rx,ry,xx,yy;
	unsigned int mask;

	if (win)   xwin=win->xlib_window;
	if (!xwin) xwin=DefaultRootWindow(anXApp::app->dpy);
	
	 // if query returns true, then xx,yy do indeed refer to window coords, and chld
	 // is the child of win that contains the pointer. rx,ry are root coords.
	 // If query returns false, then mouse is not on same screen as specified window,
	 // but rt does get set to the root window of the screen the mouse is actually in.
	int er=XQueryPointer(anXApp::app->dpy,xwin,&rt,&chld,&rx,&ry,&xx,&yy,&mask);
	if (er==False && !win) er=XQueryPointer(anXApp::app->dpy, rt,&rt,&chld,&rx,&ry,&xx,&yy,&mask);
	if (screen) {
		int numscreens=ScreenCount(anXApp::app->dpy);
		int c;
		for (c=0; c<numscreens; c++) 
			if (rt==RootWindow(anXApp::app->dpy,c)) { *screen=c; break; }
		//if (c==numscreens) ; --->  hopefully this never happens!!
	}
	if (er==False) return 1; //diff screen than default root window...

	if (child) { *child=(chld ? anXApp::app->findwindow_xlib(chld) : NULL); }
	if (x) { *x=xx; }
	if (y) { *y=yy; }
	if (mods) { *mods=mask; }

	if (pressure) { *pressure=1; } //full pressure for core xlib!
	if (tiltx) { *tiltx=0; } //ignore for core xlib!
	if (tilty) { *tilty=0; } //ignore for core xlib!
	
	return 0;
}

//! Return 0 for success. 1 for error.
/*! Selects for key up/down, button up/down, pointer motion, enter/leave, focus change,
 * using core Xlib if ANXWIN_NO_INPUT is not set in win->win_style.
 */
int CoreXlibPointer::selectForWindow(anXWindow *win,unsigned long)
{
	if (!win) return 1;

	win->xlib_win_xattsmask|=CWEventMask;
	if (!(win->win_style&ANXWIN_NO_INPUT)) {
		win->xlib_win_xatts.event_mask|=PointerMotionMask
									  | ButtonPressMask
									  | ButtonReleaseMask
									  | EnterWindowMask
									  | LeaveWindowMask;
		if (win->xlib_window)
			XChangeWindowAttributes(anXApp::app->dpy, 
									win->xlib_window,
									win->xlib_win_xattsmask,
									&win->xlib_win_xatts);
	}
	return 0;
}

/*! Return 1 if event is absorbed, and nothing else should try to parse it.
 * Return 0 for event ignored.
 */
int CoreXlibPointer::eventFilter(EventData **events_ret,
										  XEvent *xev,
										  anXWindow *ww,//!< The target window for the event
										  int &isinput) //!< Set to 1 if the processed event is input related
{
	if (!ww) return 0; //we are expecting a target window

	if (xev->type==ButtonPress) {
		int button=0;
		if (xev->xbutton.button==Button1) button=1;
		else if (xev->xbutton.button==Button2) button=2;
		else if (xev->xbutton.button==Button3) button=3;
		else if (xev->xbutton.button==Button4) button=4;
		else if (xev->xbutton.button==Button5) button=5;
		else button=xev->xbutton.button;

		buttonPressed(xev->xbutton.time, button, ww->object_id);

		MouseEventData *b=new MouseEventData(LAX_onButtonDown);
		b->to=ww->object_id;
		b->target=ww;
		b->device=this;
		b->button   =button;
		b->count	=buttoncount;
		b->x		=xev->xbutton.x;
		b->y		=xev->xbutton.y;
		b->modifiers=xev->xbutton.state;

		isinput=1;
		*events_ret=b;
		return 1;

	} else if (xev->type==ButtonRelease) {
		int button=0;
		if (xev->xbutton.button==Button1) button=1;
		else if (xev->xbutton.button==Button2) button=2;
		else if (xev->xbutton.button==Button3) button=3;
		else if (xev->xbutton.button==Button4) button=4;
		else if (xev->xbutton.button==Button5) button=5;
		else button=xev->xbutton.button;

		buttonReleased(button,ww);

		MouseEventData *b=new MouseEventData(LAX_onButtonUp);
		b->to=ww->object_id;
		b->target=ww;
		b->device=this;
		b->button   =button;
		b->x		=xev->xbutton.x;
		b->y		=xev->xbutton.y;
		b->modifiers=xev->xbutton.state;

		isinput=1;
		*events_ret=b;
		return 1;

	} else if (xev->type==MotionNotify) {
		MouseEventData *b=new MouseEventData(LAX_onMouseMove);
		b->to=ww->object_id;
		b->target=ww;
		b->device=this;
		b->x		=xev->xmotion.x;
		b->y		=xev->xmotion.y;
		b->modifiers=xev->xmotion.state;

		isinput=1;
		*events_ret=b;
		return 1;

	} else if (xev->type==EnterNotify) {
		EnterExitData *e=new EnterExitData(LAX_onMouseIn);
		e->to=ww->object_id;
		e->device=this;
		e->xlib_time=xev->xcrossing.time;
		e->to=ww->object_id;
		e->target=ww;
		e->x=xev->xcrossing.x;
		e->y=xev->xcrossing.y;

		isinput=1;
		*events_ret=e;
		return 1;

//********tooltip check:
//		 // Initialize checking for whether to show a tooltip for this window..
//		if (tooltips && ttcount==0 && ww->tooltip()) {
//			ttendlimit=  times(&tmsstruct)+tooltips*  sysconf(_SC_CLK_TCK)/1000;
//			ttthreshhold=times(&tmsstruct)+tooltips/4*sysconf(_SC_CLK_TCK)/1000;
//			ttmaybe=ww;
//		}
////------------enter/exit events-------------------
//	} else if (xev->type==EnterNotify) lastenter=*((XCrossingEvent *)xev);
//	else if (xev->type==LeaveNotify) lastleave=*((XCrossingEvent *)xev);
////			printxcrossing(this,xev); 
//}

	} else if (xev->type==LeaveNotify) {
		EnterExitData *e=new EnterExitData(LAX_onMouseOut);
		e->to=ww->object_id;
		e->device=this;
		e->xlib_time=xev->xcrossing.time;
		e->to=ww->object_id;
		e->target=ww;
		e->x=xev->xcrossing.x;
		e->y=xev->xcrossing.y;

		last_leave_window=ww->object_id;
//*********tooltip check:
//		if (tooltips && ttcount==0) { if (ww==ttmaybe) { ttmaybe=NULL; ttendlimit=0; } }

		isinput=1;
		*events_ret=e;
		return 1;

	}

	return 0;
}

//------------------------------------- CoreXlibKeyboard -----------------------------------
/*! \class CoreXlibKeyboard
 * \brief Device that selects for core keyboard events.
 */


CoreXlibKeyboard::CoreXlibKeyboard(CoreXlibPointer *p)
{
	Name("Core Keyboard");
	paired_mouse=p;
	if (p && p->paired_keyboard!=this) p->paired_keyboard=this;
}


//! Return 0 for success. 1 for error.
/*! Selects for key up/down, button up/down, pointer motion, enter/leave, focus change,
 * using core Xlib if ANXWIN_NO_INPUT is not set in win->win_style.
 */
int CoreXlibKeyboard::selectForWindow(anXWindow *win,unsigned long)
{
	if (!win) return 1;

	win->xlib_win_xattsmask|=CWEventMask;
	if (!(win->win_style&ANXWIN_NO_INPUT)) {
		win->xlib_win_xatts.event_mask|=KeyPressMask
									  | KeyReleaseMask
									  | FocusChangeMask;
		if (win->xlib_window)
			XChangeWindowAttributes(anXApp::app->dpy, 
									win->xlib_window,
									win->xlib_win_xattsmask,
									&win->xlib_win_xatts);
	}
	return 0;
}

/*! This is supposed to translate any device related events to EventData objects.
 * This also means updating any focus and tooltip checks.
 *
 * Return 1 if event is absorbed, and nothing else should try to parse it.
 * Return 0 for event ignored.
 */
int CoreXlibKeyboard::eventFilter(EventData **events_ret,
										  XEvent *xev,
										  anXWindow *ww,//!< The target window for the event
										  int &isinput) //!< Set to 1 if the processed event is input related
{
	if (!ww) return 0; //we are expecting a target window

	if (xev->type==KeyPress) {
		char *buffer=NULL;
		int len=0;
		unsigned int key=0;
		unsigned int state=xev->xkey.state;
		if (anXApp::app->filterKeyEvents(this, ww, xev,key,buffer,len,state)!=0) return 0;

		KeyEventData *k=new KeyEventData(LAX_onKeyDown);
		k->propagate=1;
		k->to=ww->object_id;
		k->target=ww;
		k->device=this;
		k->buffer=buffer;
		k->len=len;
		k->key=key;
		k->modifiers=state;

		isinput=1;
		*events_ret=k;
		return 1;

	} else if (xev->type==KeyRelease) {
		if (!ww->win_active) return 0; //ignore if window doesn't have a focus

		KeyEventData *k=new KeyEventData(LAX_onKeyUp);

		char ch;
		KeySym keysym;  
		unsigned int state=xev->xkey.state;

		 //simple lookup, we are really only interested in control, shift, alt, meta,
		 //or perhaps a space bar going up..
		XLookupString(&xev->xkey,&ch,1,&keysym,NULL);
		k->key=filterkeysym(keysym,&state); //convert to a Laxkit keycode

		k->propagate=1;
		k->to=ww->object_id;
		k->device=this;
		k->target=ww;
		k->modifiers=state;

		isinput=1;
		*events_ret=k;
		return 1;

	} else if (xev->type==FocusIn) {
		if (xev->xfocus.detail==NotifyInferior || xev->xfocus.detail==NotifyAncestor || xev->xfocus.detail==NotifyNonlinear) {
			 //is an actual focus in, not just a child focus in
			FocusChangeData *f=new FocusChangeData(LAX_onFocusOn);
			f->to=ww->object_id;
			f->send_time=gettime();
			f->target=ww;
			f->to=ww->object_id;
			f->device=this;

			tms tms_;
			SetFocus(ww,times(&tms_),1);

			anXApp::app->xim_deadkey=0;
			*events_ret=f;
			isinput=1;
			return 1;
		}
		return 0;

	} else if (xev->type==FocusOut) {
		//***old lax deactivated child windows if top window had a focus out...
		if (xev->xfocus.detail==NotifyInferior || xev->xfocus.detail==NotifyAncestor || xev->xfocus.detail==NotifyNonlinear) {
			 //is an actual focus in, not just a child focus in
			FocusChangeData *f=new FocusChangeData(LAX_onFocusOff);
			f->to=ww->object_id;
			f->target=ww;
			tms tms_;
			f->send_time=times(&tms_);
			f->device=this;

			anXApp::app->xim_deadkey=0;
			if (anXApp::app->xim_ic && ww!=current_focus) {
				XUnsetICFocus(anXApp::app->xim_ic);
			}
			*events_ret=f;
			isinput=1;
			return 1;
		}
		return 0;
	}

	return 0;
}

//! Called by anXApp when the focus needs to be set to win.
/*! Return 0 for success. nonzero for error.
 *
 * For instance, when using core Xlib, XSetInputFocus() would be called from here.
 */
int CoreXlibKeyboard::SetFocus(anXWindow *win, clock_t t,int notifyonly)
{
	//***should probably check other keyboards, and if they have same focus window, then remove it from them
	tms tms_;
	LaxKeyboard::SetFocus(win,times(&tms_),1);

	if (!notifyonly) {
		XWindowAttributes atts;
		XGetWindowAttributes(anXApp::app->dpy,win->xlib_window, &atts);
		
		if (atts.map_state==IsViewable) {
			XSetInputFocus(anXApp::app->dpy,win->xlib_window,RevertToParent,(CurrentTime));
		}
	}

	return 0;
}


//------------------------------------ DeviceManagerXlib -----------------------------------------

//! Return a DeviceManager with 2 devices: a core Xlib keyboard and mouse.
DeviceManager *newCoreXlibDeviceManager(Display *dpy)
{
	DeviceManager *dm=new DeviceManager();
	CoreXlibPointer *p=new CoreXlibPointer(NULL);
	dm->devices.push(p);
	CoreXlibKeyboard *kb=new CoreXlibKeyboard(p);
	dm->devices.push(kb);

	return dm;
}


//-------------------------- XInput2 devices ----------------------------------------
#ifdef LAX_USES_XINPUT2

static int xinput2_opcode=0;

//------------------------------------ DeviceManagerXInput2 -----------------------------------------

class XInput2DeviceManager : public DeviceManager
{
  public:
	Display *dpy;
	XInput2DeviceManager(Display *disp, int which);
	virtual int init();

	virtual int RemapHierarchy(int which=0);
	virtual int flushXDevices();
};

XInput2DeviceManager::XInput2DeviceManager(Display *disp, int which)
{
	dpy=disp;
	RemapHierarchy(which);
}

int XInput2DeviceManager::init()
{
	return DeviceManager::init();
}

//! Returns the number of devices removed.
int XInput2DeviceManager::flushXDevices()
{
	int n=0;
	for (int c=0; c<devices.n; ) {
		if (devices.e[c]->usesX()) { devices.remove(c); n++; continue; }
		c++;
	}
	return n;
}

//! Flush all X devices and repopulate with the current XInput2 devices.
/*! Default is to add only master devices.
 *
 * Return 0 for success, nonzero for error.
 */
int XInput2DeviceManager::RemapHierarchy(int which)
{
	DBG cerr <<"-------------XInput2DeviceManager::RemapHierarchy()--------------"<<endl;

	if (dpy==NULL) dpy=anXApp::app->dpy;
	if (dpy==NULL) return 1;

	flushXDevices();
	
	 //do some XInput2 initialization
	 // We support XI 2.0 
	int event, error;
	if (!XQueryExtension(anXApp::app->dpy, "XInputExtension", &xinput2_opcode, &event, &error)) {
		DBG cerr<<"X Input extension not available!"<<endl;
		return 1;
	}

	int major=2, minor=0;
	int rc = XIQueryVersion(anXApp::app->dpy, &major, &minor);
	if (rc == BadRequest) {
		printf("No XI2 support. Server supports version %d.%d only.\n", major, minor);
		return 2;
	} else if (rc != Success) {
		DBG cerr <<  "XIQueryVersion Internal Error! This is a bug in Xlib."<<endl;
	}
	DBG cerr << "XI2 supported. Server provides version "<<major<<'.'<<minor<<endl;


	 //foreach master device, dm->devices.push(p);

	XIDeviceInfo *info, *dev;
	int ndevices;
	int i;

	if (which==0) which=XIAllMasterDevices;

	info = XIQueryDevice(anXApp::app->dpy, which, &ndevices);

	for(i = 0; i < ndevices; i++) {
		dev = &info[i];
		DBG cerr <<"Adding "<<dev->name<<", id:"<<dev->deviceid<<endl;

		if (dev->use==XIMasterPointer) {
			devices.push(new XInput2Pointer(dev));

		} else if (dev->use==XIMasterKeyboard) {
			devices.push(new XInput2Keyboard(dev));
		}
		 //else:
		 //  case XISlavePointer: type = "slave pointer"; break;
		 //  case XISlaveKeyboard: type = "slave keyboard"; break;
		 //  case XIFloatingSlave: type = "floating slave"; break;
	}

	 //pair up devices...
	XInput2Pointer  *ptr;
	XInput2Keyboard *kbd;
	int infoi=-1;
	for (int c=0; c<devices.n; c++) {
		 //find device info struct for this device
		for (int c2=0; c2<ndevices; c2++) {
			if ((unsigned int)info[c2].deviceid==devices.e[c]->xid) { infoi=c2; break; }
		}

		 //find and attach a keyboard to a mouse
		ptr=dynamic_cast<XInput2Pointer*>(devices.e[c]);
		if (ptr && !ptr->paired_keyboard && infoi>=0) {
			 //find attached keyboard...
			for (int c2=c+1; c2<devices.n; c2++) {
				if ((unsigned int)info[infoi].attachment==devices.e[c2]->xid) {
					kbd=dynamic_cast<XInput2Keyboard*>(devices.e[c2]);
					if (kbd) {
						ptr->paired_keyboard=kbd;
						kbd->paired_mouse=ptr;
						break;
					}
				}
			}
			continue;
		}

		 //find and attach a mouse to a keyboard
		kbd=dynamic_cast<XInput2Keyboard*>(devices.e[c]);
		if (kbd && !kbd->paired_mouse && infoi>=0) {
			 //find attached pointer...
			for (int c2=c+1; c2<devices.n; c2++) {
				if ((unsigned int)info[infoi].attachment==devices.e[c2]->xid) {
					ptr=dynamic_cast<XInput2Pointer*>(devices.e[c2]);
					if (ptr) {
						ptr->paired_keyboard=kbd;
						kbd->paired_mouse=ptr;
						break;
					}
				}
			}
			continue;
		}
	}

	XIFreeDeviceInfo(info);

	DBG cerr <<"-------------done XInput2DeviceManager::RemapHierarchy()--------------"<<endl;
	return 0;
}


//---------------------
//! Return a new XInput2DeviceManager object, loaded with some XInput2 based devices.
/*! Default is to add only master devices.
 */
DeviceManager *newXInput2DeviceManager(Display *dpy, int which)
{
	return new XInput2DeviceManager(dpy,which);
}



//-------------------------- XInput2Pointer ----------------------------------------
/*! \class XInput2Pointer
 * \brief Class corresponding to, if you can believe it, an XInput2 pointer.
 *
 * Of most interest will be LAX_onDeviceChange events, which will be sent when a subdevice
 * takes over input for a master device. The default for Laxkit is for only master devices
 * to be listed as actual devices.
 */

XInput2Pointer::XInput2Pointer(XIDeviceInfo *d)
{
	Name(d->name);
	xid=d->deviceid;
	use=d->use;
}

/*! Return 0 for success, nonzero for error.
 */
int XInput2Pointer::setMouseShape(anXWindow *win, int shape)
{
	if (!shape) { XIUndefineCursor(anXApp::app->dpy, xid, win->xlib_window); return 0; }

	if (shape==LAX_MOUSE_Cancel) shape=0;
	Cursor cursor;
	cursor=XCreateFontCursor(anXApp::app->dpy, shape);
	XIDefineCursor(anXApp::app->dpy, xid, win->xlib_window, cursor);
	XFreeCursor(anXApp::app->dpy,cursor);
	return 0;
}

/*! Return 0 for success, 1 for failure.
 */
int XInput2Pointer::ungrabDevice()
{
	return XIUngrabDevice(anXApp::app->dpy, xid, CurrentTime);
}

/*! Return 0 for success, 1 for failure.
 */
int XInput2Pointer::grabDevice(anXWindow *win)
{
	if (!win || !win->xlib_window) return 1;

    unsigned char mask[4] = { 0, 0, 0, 0 };
    XIEventMask evmask;
    evmask.mask = mask;
    evmask.mask_len = sizeof(mask); //see also XIMaskLen(XI_LASTEVENT)
    evmask.deviceid = xid;

    XISetMask(mask, XI_DeviceChanged);
    XISetMask(mask, XI_ButtonPress);
    XISetMask(mask, XI_ButtonRelease);
    XISetMask(mask, XI_Motion);
    XISetMask(mask, XI_Enter);
    XISetMask(mask, XI_Leave);

	if (XIGrabDevice(anXApp::app->dpy, xid, win->xlib_window, CurrentTime,
					 None, GrabModeAsync, GrabModeAsync,
					 False, &evmask) == GrabSuccess) return 0;
	return 1;
}

////-2 if unititialized, -1 field type info not available
//int XInput2MasterPointer::numFields()
//{
//	if (!xid) return -2;
//	return -2;
//}

//! Get information about the mouse state.
/*! If you pass NULL for any field, then that field is ignored.
 *
 * If win!=NULL, then return coordinates in win space. If the window is not on the same screen as
 * the mouse, then return 1, and only screen gets set.
 *
 * Otherwise return screen coordinates. The screen the mouse is actually in gets set.
 *
 * Return 0 for success, or 1 for mouse not on same screen, or other number for mouse info
 * not available for some reason.
 */
int XInput2Pointer::getInfo(anXWindow *win,
							 int *screen, anXWindow **child,
							 double *x, double *y, unsigned int *mods,
							 double *pressure, double *tiltx, double *tilty, ScreenInformation **screenInfo) const //extra goodies
{
	Window xwin = 0;
	if (win)   xwin = win->xlib_window;
	if (!xwin) xwin = DefaultRootWindow(anXApp::app->dpy);

	// XInput2:
	double drx, dry, dxx, dyy; //sub pixel accuracy!
	Window rt=0, chld=0;
	XIButtonState buttonstate; //these are structs. buttonstate->mask must be free'd
	XIModifierState modstate;
	XIGroupState groupstate;

	Bool er = XIQueryPointer(
			anXApp::app->dpy,//    Display*            display,
			xid,             //    int                 deviceid,
			xwin,            //    Window              win,
			&rt,             //    Window*             root,
			&chld,           //    Window*             child, is ONLY immediate child of xwin!!! usually a wm decoration window, not useful
			&drx,            //    double*             root_x,
			&dry,            //    double*             root_y,
			&dxx,            //    double*             win_x,
			&dyy,            //    double*             win_y,
			&buttonstate,    //    XIButtonState       *buttons,
			&modstate,       //    XIModifierState     *mods,
			&groupstate      //    XIGroupState        *group
		);
	free(buttonstate.mask);

	if (er == False && !win) { //we are looking for root window coords, but don't know the root window...
		 //requery using root of screen mouse is actually in, 
		 //rather than our guess of DefaultRootWindow
		er = XIQueryPointer(
			anXApp::app->dpy,//    Display*            display,
			xid,             //    int                 deviceid,
			rt,              //    Window              win,
			&rt,             //    Window*             root,
			&chld,           //    Window*             child,
			&drx,            //    double*             root_x,
			&dry,            //    double*             root_y,
			&dxx,            //    double*             win_x,
			&dyy,            //    double*             win_y,
			&buttonstate,    //    XIButtonState       *buttons,
			&modstate,       //    XIModifierState     *mods,
			&groupstate      //    XIGroupState        *group
		);
		free(buttonstate.mask);
	}

	int screen_num = -1;
	if (screen || screenInfo) {
		int numscreens = ScreenCount(anXApp::app->dpy);
		int c;
		for (c=0; c<numscreens; c++) if (rt == RootWindow(anXApp::app->dpy,c)) { screen_num = c; break; }
		//if (c==numscreens) ; --->  hopefully this never happens!!
		
		if (screenInfo) {
			//find monitor nearest to rx,ry
			*screenInfo = anXApp::app->FindNearestMonitor(screen_num, drx, dry);
		}

		if (screen) *screen = screen_num;
	}

	if (er == False) return 1;

	if (child) {
		 //we need to zero in on the actual child window
		while (chld!=0) {
			xwin=chld;
			er=XIQueryPointer(
				anXApp::app->dpy,//    Display*            display,
				xid,             //    int                 deviceid,
				xwin,            //    Window              win,
				&rt,             //    Window*             root,
				&chld,           //    Window*             child, is ONLY immediate child of xwin!!!
				&drx,            //    double*             root_x,
				&dry,            //    double*             root_y,
				&dxx,            //    double*             win_x,
				&dyy,            //    double*             win_y,
				&buttonstate,    //    XIButtonState       *buttons,
				&modstate,       //    XIModifierState     *mods,
				&groupstate      //    XIGroupState        *group
			);
			free(buttonstate.mask);
		}
		*child=(xwin ? anXApp::app->findwindow_xlib(xwin) : NULL);
	}
	if (x) { *x=dxx; }
	if (y) { *y=dyy; }
	if (mods) { *mods=modstate.effective; }

	if (pressure || tiltx || tilty) {
		int n=0;
		XIDeviceInfo *devinfo=XIQueryDevice(anXApp::app->dpy, xid, &n);
		XIValuatorClassInfo *val;

		if (pressure) *pressure=1;
		if (tiltx) *tiltx=0;
		if (tilty) *tilty=0;

		for (int c=0; c<devinfo->num_classes; c++) {
			if (devinfo->classes[c]->type!=XIValuatorClass) continue;

			val=(XIValuatorClassInfo*)(devinfo->classes[c]);
			//val->number == physical axis of this valuator....
			//val->label  == Atom, description of axis
			//val->min
			//val->max
			//val->value
			//val->resolution
			//val->mode

			 //need to do a different call to find valuator info...
			if (pressure && val->number==2) {
				 //it appears to vary as to how various plain mice map to pressure.
				 //one, for instance, maps to 0, and min/max is 0,-1
				 //another has mni==max==0
				if (val->min>=val->max) *pressure=1; 
				else *pressure=(val->value-val->min)/(val->max-val->min);
			}
			if (tiltx && val->number == 3 && val->max != val->min) { *tiltx = (val->value-val->min)/(val->max-val->min); }
			if (tilty && val->number == 4 && val->max != val->min) { *tilty = (val->value-val->min)/(val->max-val->min); }
		}

		XIFreeDeviceInfo(devinfo);
	}
	
	return 0;
}

int XInput2Pointer::selectForWindow(anXWindow *win,unsigned long)
{
	if (!win || !win->xlib_window) return 1;

    unsigned char mask[4] = { 0, 0, 0, 0 };
    XIEventMask evmask;
    evmask.mask = mask;
    evmask.mask_len = sizeof(mask); //see also XIMaskLen(XI_LASTEVENT)
    evmask.deviceid = xid;

    XISetMask(mask, XI_DeviceChanged);
    XISetMask(mask, XI_ButtonPress);
    XISetMask(mask, XI_ButtonRelease);
    XISetMask(mask, XI_Motion);
    XISetMask(mask, XI_Enter);
    XISetMask(mask, XI_Leave);

    XISelectEvents(anXApp::app->dpy, win->xlib_window, &evmask, 1);


	 //select separate for hierarchy changes
	//XISetMask(mask, XI_HierarchyChanged);
	//------

	 // *** this is a bit of a hack!! You cannot select for hierarchy notifies per window.
	 // You select on the root window! These are later sent by XInput2Pointer to top windows
	 // individually...
	unsigned char hmask[2] = { 0, 0 };

	XISetMask(hmask, XI_HierarchyChanged);
	evmask.deviceid = XIAllDevices;
	evmask.mask_len = sizeof(hmask);
	evmask.mask = hmask;

	XISelectEvents(anXApp::app->dpy, DefaultRootWindow(anXApp::app->dpy), &evmask, 1);

	return 0;
}

/*! Return 1 if event is absorbed, and nothing else should try to parse it.
 * Return 0 for event ignored.
 */
int XInput2Pointer::eventFilter(EventData **events_ret, XEvent *xev, anXWindow *ww, int &isinput)
{
	if (xev->xany.type!=GenericEvent) return 0;
	XGenericEventCookie *cookie = &xev->xcookie;
	if (cookie->extension != xinput2_opcode) return 0;
	if (!cookie->data) if (!XGetEventData(anXApp::app->dpy, cookie)) return 0;
	
	//DBG cerr <<"XInput2Pointer("<<xid<<")::eventFilter("<<xlib_extension_event_name(cookie->evtype)<<")"<<endl;

	if (cookie->evtype==XI_DeviceChanged) {
		XIDeviceChangedEvent *dev=(XIDeviceChangedEvent*)cookie->data;
		//dev->reason; //XISlaveSwitch, XIDeviceChange
		if (dev->reason==XISlaveSwitch && subid!=dev->sourceid) {
			 //slave device has changed since last event
			subid=dev->sourceid;
			DeviceEventData *e=new DeviceEventData();
			e->type=LAX_onDeviceChange;
			e->subtype=LAX_DeviceSwitched;
			e->id=id;       //id of this pointer
			e->subid=subid; //subid of this pointer
			*events_ret=e;
			return 1;
		}
		return 1;
	}

	if (cookie->evtype==XI_HierarchyChanged) {
		 //device tree has changed
		XIHierarchyEvent *dev=(XIHierarchyEvent*)cookie->data;
		DeviceEventData *ee=NULL;
		DeviceEventData *e=new DeviceEventData();
		e->type=LAX_onDeviceChange;
		e->subtype=LAX_DeviceHierarchyChange;
		*events_ret=e;

		 //There will be one hierarchy event for each dev->info that indicates a change.
		for (int c=0; c<dev->num_info; c++) {
			if (dev->info[c].flags) {
				if (ee) {
					ee->next=new DeviceEventData();
					ee->next->type=LAX_onDeviceChange;
					ee->next->subtype=LAX_DeviceHierarchyChange;
					ee=(DeviceEventData *)ee->next;
				} else ee=e;

				ee->xflags=dev->info[c].flags;
				ee->xattachment=dev->info[c].attachment;
				ee->xdev=dev->info[c].deviceid;
				ee->xenabled=dev->info[c].enabled;
				ee->xtime=dev->time;
				break;
			}
		}

		//DBG cerr <<"Pointer got a XI_HierarchyChanged"<<endl;
		//DBG for (int c=0; c<dev->num_info; c++) {
		//DBG 	cerr <<"device:"<<dev->info[c].deviceid
		//DBG 		 <<"  attachment:"<<dev->info[c].attachment
		//DBG 		 <<"  use:"<<dev->info[c].use
		//DBG 		 <<"  enabled:"<<dev->info[c].enabled
		//DBG 		 <<"  flags:"<<dev->info[c].flags
		//DBG 		 <<endl;
		//DBG }

		//dev->flags is combo of XIMasterAdded,   XIMasterRemoved, XISlaveAdded,    XISlaveRemoved,
		//                       XISlaveAttached, XISlaveDetached, XIDeviceEnabled, XIDeviceDisabled
		//dev->num_info is how many XIHierarchyInfo structs are in dev->info. There is one struct
		//for EACH affected devices, including deleted ones.
		// XIHierarchyInfo {
		//   int deviceid;
		//   int attachment;
		//   int use;
		//   Bool enabled;
		//   int flags;   <-- will have the relevant flags, as in dev->flags
		// }

		 // *** bit of a hack here to force internal upkeep of device tree:
		XInput2DeviceManager *dm=dynamic_cast<XInput2DeviceManager *>(anXApp::app->devicemanager);
		if (dm) {
			dm->RemapHierarchy(0);
			anXApp::app->reselectForXEvents(NULL);
		}
		return 1;
	}

	if (cookie->evtype==XI_ButtonPress) {
		XIDeviceEvent *dev=(XIDeviceEvent*)cookie->data;
		if (dev->deviceid!=(int)xid) return 0; //is event from another device, so ignore
		if (subid==0) subid=dev->sourceid;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event); //find target window
		if (!ww) return 0; //if no target window

		int button=dev->detail;
		buttonPressed(dev->time, button, ww->object_id);

		DBG cerr <<"Button down "<<ww->WindowTitle()<<endl;

		MouseEventData *b=new MouseEventData(LAX_onButtonDown);
		b->to=ww->object_id;
		b->target=ww;
		b->device=this;
		b->button   =button;
		b->count	=buttoncount;
		b->x		=dev->event_x;
		b->y		=dev->event_y;
		b->modifiers=dev->mods.effective;//***is this right???

		isinput=1;
		*events_ret=b;
		return 1;

	} else if (cookie->evtype==XI_ButtonRelease) {
		XIDeviceEvent *dev=(XIDeviceEvent*)cookie->data;
		if (dev->deviceid!=(int)xid) return 0;
		if (subid==0) subid=dev->sourceid;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event);
		if (!ww) return 0;

		int button=dev->detail;
		buttonReleased(button,ww);

		MouseEventData *b=new MouseEventData(LAX_onButtonUp);
		b->to=ww->object_id;
		b->target=ww;
		b->device=this;
		b->button   =button;
		b->x		=dev->event_x;
		b->y		=dev->event_y;
		b->modifiers=dev->mods.effective;//***is this right???

		isinput=1;
		*events_ret=b;
		return 1;

	} else if (cookie->evtype==XI_Motion) {
		XIDeviceEvent *dev=(XIDeviceEvent*)cookie->data;
		if (dev->deviceid!=(int)xid) return 0;
		if (subid==0) subid=dev->sourceid;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event);
		if (!ww) return 0;

		//DBG cerr <<"Motion "<<ww->WindowTitle()<<endl;

		MouseEventData *b=new MouseEventData(LAX_onMouseMove);
		b->to=ww->object_id;
		b->target=ww;
		b->device=this;
		b->x		=dev->event_x;
		b->y		=dev->event_y;
		b->modifiers=dev->mods.effective;//***is this right???

		isinput=1;
		*events_ret=b;
		return 1;

	} else if (cookie->evtype==XI_Enter) {
		XIEnterEvent *dev=(XIEnterEvent*)cookie->data;
		if (dev->deviceid!=(int)xid) return 0;
		if (subid==0) subid=dev->sourceid;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event);
		//if (!ww || !ww->win_active) return 0;
		if (!ww) return 0;

		DBG cerr <<"Enter "<<(ww->WindowTitle())<<endl;
		DBG cerr <<"    mode:"<<dev->mode<<"  detail:"<<dev->detail<<endl;
		//mode is NotifyNormal=0, NotifyGrab 1, NotifyUngrap 2, NotifyWhileGrabbed 3
		//detail can be  NotifyAncestor, NotifyVirtual, NotifyInferior, NotifyNonlinear, NotifyNonlinearVirtual (0-4)

		if (dev->mode!=NotifyNormal) return 0;

		EnterExitData *e=new EnterExitData(LAX_onMouseIn);
		e->to=ww->object_id;
		e->device=this;
		e->xlib_time=dev->time;
		e->to=ww->object_id;
		e->target=ww;
		e->x=dev->event_x;
		e->y=dev->event_y;

		isinput=1;
		*events_ret=e;
		return 1;

//********tooltip check:
//		 // Initialize checking for whether to show a tooltip for this window..
//		if (tooltips && ttcount==0 && ww->tooltip()) {
//			ttendlimit=  times(&tmsstruct)+tooltips*  sysconf(_SC_CLK_TCK)/1000;
//			ttthreshhold=times(&tmsstruct)+tooltips/4*sysconf(_SC_CLK_TCK)/1000;
//			ttmaybe=ww;
//		}
////------------enter/exit events-------------------
//	} else if (xev->type==EnterNotify) lastenter=*((XCrossingEvent *)xev);
//	else if (xev->type==LeaveNotify) lastleave=*((XCrossingEvent *)xev);
////			printxcrossing(this,xev); 
//}

	} else if (cookie->evtype==XI_Leave) {
		XIEnterEvent *dev=(XIEnterEvent*)cookie->data;
		if (dev->deviceid!=(int)xid) return 0;
		if (subid==0) subid=dev->sourceid;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event);
		//if (!ww || !ww->win_active) return 0;
		if (!ww) return 0;

		DBG cerr <<"Leave "<<(ww->WindowTitle())<<endl;
		DBG cerr <<"    mode:"<<dev->mode<<"  detail:"<<dev->detail<<endl;

		if (dev->mode!=NotifyNormal) return 0;

		EnterExitData *e=new EnterExitData(LAX_onMouseOut);
		e->to=ww->object_id;
		e->device=this;
		e->xlib_time=dev->time;
		e->to=ww->object_id;
		e->target=ww;
		e->x=dev->event_x;
		e->y=dev->event_y;

		last_leave_window=ww->object_id;
//*********tooltip check:
//		if (tooltips && ttcount==0) { if (ww==ttmaybe) { ttmaybe=NULL; ttendlimit=0; } }

		isinput=1;
		*events_ret=e;
		return 1;

	}

	return 0;
}


//-------------------------- XInput2Keyboard ----------------------------------------
/*! \class XInput2Keyboard
 * \brief Class corresponding to, if you can believe it, an XInput2 master device.
 *
 * \todo figure out a rational way to handle WM_TAKE_FOCUS.
 */

//*******for multipointer, WM_TAKE_FOCUS is not well defined. ignoring for now.
//	if (e) {
//		if (e->type==ClientMessage) {
//			 // for WM_PROTOCOLS --> WM_TAKE_FOCUS
//			 // Xlib sends this to tell top level windows that they should get the focus.
//			 // *** need to research this a bit to see if it really is relevant for modern X with mpx
//			if (ww->win_style&ANXWIN_NO_INPUT) return 1; //absorbs the event
//
//			char *aname=XGetAtomName(dpy,e->xclient.message_type);
//			DBG cerr << "anXApp::mangefocus:ClientMessage: "<<aname<<endl;
//			if (strcmp(aname,"WM_PROTOCOLS")==0) {
//				XFree(aname);
//				aname=XGetAtomName(dpy,e->xclient.data.l[0]);
//				DBG cerr <<"...got protocol:"<<aname<<endl;
//				if (strcmp(aname,"WM_TAKE_FOCUS")==0) {
//					 //***this eventually should set to currentfocus of topwindow, not simply the topwindow
//					DBG cerr <<"...set focus to \""<<(ww->WindowTitle())<<"\""<<endl;
//					devicemanager->SetFocus(ww,NULL,times(NULL),0);
//					XFree(aname);
//					return 1;
//				} 
//			}
//			XFree(aname);
//			return 0;
//		 }
//		return 0;
//	}

XInput2Keyboard::XInput2Keyboard(XIDeviceInfo *d)
{
	Name(d->name);
	xid=d->deviceid;
	use=d->use;
}


int XInput2Keyboard::selectForWindow(anXWindow *win,unsigned long)
{
	if (!win || !win->xlib_window) return 1;

    unsigned char mask[4] = { 0, 0, 0, 0 };
    XIEventMask evmask;
    evmask.mask = mask;
    evmask.mask_len = sizeof(mask); //see also XIMaskLen(XI_LASTEVENT)
    evmask.deviceid = xid;

    XISetMask(mask, XI_DeviceChanged);
    XISetMask(mask, XI_KeyPress);
    XISetMask(mask, XI_KeyRelease);
    XISetMask(mask, XI_FocusIn);
    XISetMask(mask, XI_FocusOut);

    XISelectEvents(anXApp::app->dpy, win->xlib_window, &evmask, 1);

	return 0;
}

/*! Return 1 if event is absorbed, and nothing else should try to parse it.
 * Return 0 for event ignored.
 */
int XInput2Keyboard::eventFilter(EventData **events_ret,XEvent *xev,anXWindow *ww,int &isinput)
{
	//note that the GenericEvents we want hide the window in cookie data, so we have to extract it..

	//DBG cerr <<"XInput2Keyboard::eventFilter("<<xlib_event_name(xev->xany.type)<<")"<<endl;

	XGenericEventCookie *cookie = &xev->xcookie;
	if (cookie->type != GenericEvent || cookie->extension != xinput2_opcode) return 0;
	if (!cookie->data && !XGetEventData(anXApp::app->dpy, cookie)) return 0;


	if (cookie->evtype == KeyPress) {
		XIDeviceEvent *dev = (XIDeviceEvent*)cookie->data;
		if (dev->deviceid != (int)xid) return 0;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event);
		if (!ww || !ww->win_active) return 0;

		char *buffer = NULL;
		int len = 0;
		unsigned int key = 0;
		unsigned int state = dev->mods.effective;

		XEvent kev;//**** HACK!! not sure how to sanely translate XI key events to normal input
		kev.xkey.type    = KeyPress;
		kev.xkey.time    = dev->time;
		kev.xkey.display = anXApp::app->dpy;
		kev.xkey.keycode = dev->detail;
		kev.xkey.state   = state;

		//***** not working: if (anXApp::app->filterKeyEvents(this, ww, &kev,key,buffer,len,state)!=0) return 0;
		char ch;
		KeySym keysym;  
		XLookupString(&kev.xkey, &ch,1, &keysym, NULL);
		key = filterkeysym(keysym, &state); //convert to a Laxkit keycode

		DBG cerr <<"key down: device "<<dev->deviceid<<",  source "<<dev->sourceid<<", detail:"<<dev->detail
		DBG		 <<" fake:"<<key <<endl;
		DBG 
		DBG cerr <<" XLookupKeysyms for "<<kev.xkey.keycode<<": "
		DBG 	 <<     XLookupKeysym(&kev.xkey, 0)
		DBG		 <<' '<<XLookupKeysym(&kev.xkey, 1)
		DBG 	 <<' '<<XLookupKeysym(&kev.xkey, 2)
		DBG 	 <<' '<<XLookupKeysym(&kev.xkey, 3)<<' '<<endl;

		KeyEventData *k = new KeyEventData(LAX_onKeyDown);
		k->propagate = 1;
		k->device    = this;
		k->to        = ww->object_id;
		k->target    = ww;
		k->buffer    = buffer;
		k->len       = len;
		k->keycode   = dev->detail;
		k->key       = key;
		k->modifiers = state;

		isinput = 1;
		*events_ret = k;
		return 1;

	} else if (cookie->evtype == XI_KeyRelease) {
		XIDeviceEvent *dev = (XIDeviceEvent*)cookie->data;
		if (dev->deviceid != (int)xid) return 0;
		if (!ww) ww = anXApp::app->findwindow_xlib(dev->event);
		if (!ww || !ww->win_active) return 0;

		KeyEventData *k = new KeyEventData(LAX_onKeyUp);

		char ch;
		KeySym keysym;  
		unsigned int state = dev->mods.effective;

		XKeyEvent kev;//**** HACK!! not sure how to sanely translate XI key events to normal input, so using a fake old key event
		kev.display = anXApp::app->dpy;
		kev.keycode = dev->detail;
		kev.state   = state;


		 //simple lookup, we are really only interested in control, shift, alt, meta,
		 //or perhaps a space bar going up..
		XLookupString(&kev,&ch,1,&keysym,NULL);
		k->key = filterkeysym(keysym,&state); //convert to a Laxkit keycode

		k->device    = this;
		k->propagate = 1;
		k->to        = ww->object_id;
		k->target    = ww;
		k->modifiers = state;
		k->keycode   = dev->detail;

		isinput = 1;
		*events_ret = k;
		return 1;

	} else if (cookie->evtype==XI_FocusIn) {
		XIEnterEvent *dev=(XIEnterEvent*)cookie->data;
		if (dev->deviceid!=(int)xid) return 0;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event);
		if (!ww) return 0;

		if (dev->detail==NotifyInferior || dev->detail==NotifyAncestor || dev->detail==NotifyNonlinear) {
			 //is an actual focus in, not just a child focus in
		
			FocusChangeData *f=new FocusChangeData(LAX_onFocusOn);
			f->to=ww->object_id;
			f->send_time=gettime();
			f->target=ww;
			f->to=ww->object_id;
			f->device=this;
			tms tms_;
			SetFocus(ww,times(&tms_),1);

			anXApp::app->xim_deadkey=0;
			*events_ret=f;
			isinput=1;
			return 1;
		}
		return 0;

	} else if (cookie->evtype==XI_FocusOut) {
		//***old lax deactivated child windows if top window had a focus out...

		XIEnterEvent *dev=(XIEnterEvent*)cookie->data;
		if (dev->deviceid!=(int)xid) return 0;
		if (!ww) ww=anXApp::app->findwindow_xlib(dev->event);
		if (!ww) return 0;

		if (dev->detail==NotifyInferior || dev->detail==NotifyAncestor || dev->detail==NotifyNonlinear) {
			 //is an actual focus in, not just a child focus in
			FocusChangeData *f=new FocusChangeData(LAX_onFocusOff);
			f->to=ww->object_id;
			f->target=ww;
			f->send_time=times(NULL);
			f->device=this;

			anXApp::app->xim_deadkey=0;
			if (anXApp::app->xim_ic && ww!=current_focus) {
				XUnsetICFocus(anXApp::app->xim_ic);
			}
			*events_ret=f;
			isinput=1;
			return 1;
		}
		return 0;
	}

	return 0;
}

int XInput2Keyboard::SetFocus(anXWindow *win, clock_t t,int notifyonly)
{
	tms tms_;
	LaxKeyboard::SetFocus(win,times(&tms_),1);

	if (!notifyonly) {
		XWindowAttributes atts;
		XGetWindowAttributes(anXApp::app->dpy,win->xlib_window, &atts);
		
		if (atts.map_state==IsViewable) {
			XISetFocus(anXApp::app->dpy, xid, win->xlib_window, (CurrentTime));
		}
	}
	return 0;
}


#endif // LAX_USES_XINPUT2



//------------------------------------ DeviceManagerXInput1 -----------------------------------------
///*! \class DeviceManagerXInput1
// * \brief Device manager that sets up for old style XInput1 events.
// */


//DeviceManagerXInput1::DeviceManagerXInput1()
//{

//	//init devices:
//	if (device_info) XFreeDeviceList(device_info);
//	if (!dpy) { device_info=NULL; return 1; }
//
//	int n=0;
//	device_info=XListInputDevices(anXApp::app->dpy, &n);
//
//	LaxDevice *d=NULL;
//	LaxMouse *m=NULL;
//	LaxKeyboard *k=NULL;
//	for (int c=0; c<n; c++) {
//		d=m=k=NULL;
//		if (device_info[c].use==IsXPointer) d=m=new LaxMouse();
//		else if (device_info[c].use==IsXKeyboard) d=m=new LaxKeyboard();
//		else d=new LaxDevice(); //IsXExtensionDevice assumed
//
//		d->xlib_device=device_info[c];
//		inputdevices.push(d,1);
//	}
//	return inputdevices.n;
//
//	----------Xi1----------
//	XSelectExtensionEvent
//
//	  DeviceKeyPress
//	  DeviceKeyRelease
//	  DeviceButtonPress
//	  DeviceButtonRelese
//	  DeviceMotionNotify
//	  DeviceFocusIn
//	  DeviceFocusOut
//	  ProximityIn
//	  ProximityOut
//	  DeviceStateNotify
//	  DeviceMappiingNotify
//	  ChangeDeviceNotify
//	  DevicePointerMotionHint
//	  DeviceButton1Motion
//	  DeviceButton2Motion
//	  DeviceButton3Motion
//	  DeviceButton4Motion
//	  DeviceButton5Motion
//	  DeviceButtonMotion
//	  DeviceOwnerGrabButton
//	  DeviceButtonPressGrab
//	  NoExtensionEvent
//}



} //namespace Laxkit


