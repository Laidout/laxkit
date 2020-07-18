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
//    Copyright (C) 2004-2018 by Tom Lechner
//
#ifndef _LAX_ANXAPP_H
#define _LAX_ANXAPP_H

#include <lax/configured.h>

#ifdef _LAX_PLATFORM_XLIB
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
#endif //_LAX_PLATFORM_XLIB


#include <sys/times.h>
#include <pthread.h>

#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/lists.h>
#include <lax/laxdefs.h>
#include <lax/laxdevices.h>
#include <lax/events.h>
#include <lax/fontmanager.h>
#include <lax/attributes.h>
#include <lax/misc.h>
#include <lax/rectangles.h>
#include <lax/tagged.h>
#include <lax/laxdevices.h>
#include <lax/shortcuts.h>
#include <lax/laximages.h>
#include <lax/themes.h>


namespace Laxkit {

class anXWindow;
class anXApp;
class DeviceManager;
class Displayer;
class FontManager;
class LaxFont;
class DndState;


//----------------------------- Misc ------------------------------

anXWindow *TopWindow(anXWindow *win);
int IsWindowChild(anXWindow *top,anXWindow *check);
unsigned int composekey(unsigned int k1, unsigned int k2);


#ifdef _LAX_PLATFORM_XLIB
unsigned int filterkeysym(KeySym keysym,unsigned int *state);
const char *xlib_event_name(int e_type);
const char *xlib_extension_event_name(int e_type);
#endif //_LAX_PLATFORM_XLIB


//-------------------------- aDrawable ----------------------------------------
class aDrawable
{
 public:
#ifdef _LAX_PLATFORM_XLIB
	XdbeBackBuffer xlib_backbuffer;
	Window   xlib_window;
	Drawable xlibDrawable(int which=-1);

	aDrawable(Drawable d=0) { xlib_window=d; xlib_backbuffer=None; }
	virtual int ValidDrawable() { if (xlib_window) return 1; else return 0; }
#elif defined(_LAX_PLATFORM_QT)
#elif defined(_LAX_PLATFORM_HEADLESS)
	aDrawable(Drawable d=0) {}
	virtual int ValidDrawable() { return 0; }
#endif

	virtual ~aDrawable() {}
	virtual int DrawableType() { return 1; }
};


//-------------------------- anXWindow ----------------------------------------

//for minimal window size sanity checking:
#define LAX_MAX_WINDOW_SIZE 10000

#define ANXWIN_MASK           (0xffff)
#define ANXWIN_TRANSIENT      (1<<0)
#define ANXWIN_NOT_DELETEABLE (1<<1)
#define ANXWIN_GRAYED         (1<<2)
#define ANXWIN_REMEMBER       (1<<3)
#define ANXWIN_XDND_AWARE     (1<<4)
#define ANXWIN_HOVER_FOCUS    (1<<5)
#define ANXWIN_NO_INPUT       (1<<6)
#define ANXWIN_BARE           (1<<7)
//center window when initially mapped
#define ANXWIN_CENTER         (1<<8)
//make fullscreen on initial map
#define ANXWIN_FULLSCREEN     (1<<9)
//works with deletenow()
#define ANXWIN_ESCAPABLE      (1<<10)
#define ANXWIN_DOUBLEBUFFER   (1<<11)
#define ANXWIN_DOOMED         (1<<12)
#define ANXWIN_OUT_CLICK_DESTROYS (1<<13)
//-------------------

 // note that care must be taken here, are these defines constant across Xlibs?
 // should be KeyPress/KeyRelease/ButtonPress/ButtonRelease/MotionNotify/Enter/Leave/FocusIn/Out/GraphicsExpose/Expose 
#define LAX_DIALOG_ALL_MASK (1|2|4|8|16|32|64|(1<<15)|(1<<21))
 // INPUTS blocks pointer/key/enter/leave/focusin/out events, lets Expose through
#define LAX_DIALOG_INPUTS (1|2|4|8|16|32|64|(1<<21))
	

const int AUTOPLACE = -100000;

class anXWindow : virtual public EventReceiver, 
				  virtual public Tagged,
				  virtual public LaxFiles::DumpUtility,
				  virtual public aDrawable
{
	friend class anXApp;


#ifdef _LAX_PLATFORM_XLIB
  protected:

	DndState *xlib_dnd;

  public:
 	 // Very X specific stuff about window status and event capture.
	XWMHints      *xlib_win_hints;
	XSizeHints    *xlib_win_sizehints;
	XSetWindowAttributes xlib_win_xatts;
	unsigned long  xlib_win_xattsmask;

	static Atom XdndAware;
    static Atom XdndEnter;
    static Atom XdndLeave;
    static Atom XdndPosition;
    static Atom XdndStatus;
    static Atom XdndDrop;
    static Atom XdndFinished;
    static Atom XdndSelection;
    static Atom XdndProxy;

    // static Atom JXSelectionWindowProperty;
    static Atom XdndActionList;
    static Atom XdndActionDescription;
    static Atom XdndActionCopy;
	static Atom XdndActionMove;
	static Atom XdndActionLink;
	static Atom XdndActionAsk;
	static Atom XdndActionPrivate;

    static Atom XdndTypeList;
    static Atom MimeTextUriList;
    static Atom MimeTextPlain;
    static Atom MimeTextPlainUtf8;
    static Atom MimeTextPlainLatin1;
    static Atom Xlib_TEXT;
    static Atom Xlib_UTF8_STRING;

    static Atom LaxDndProperty;

  	static bool xlib_vars_initialized;
  	static void InitXlibVars(Display *dpy);

	virtual int event(XEvent *e);
	int isXdndAware(Window w);
  	int HandleXdndEnter(XEvent *e);
  	int HandleXdndPosition(XEvent *e);
	int HandleXdndDrop(XEvent *e);

#endif //_LAX_PLATFORM_XLIB


#ifdef _LAX_PLATFORM_QT
#endif

 protected:
	char        *win_tooltip;
	int          needtodraw;

	RefPtrStack<anXWindow> _kids; 
	virtual int deletekid(anXWindow *w);

	 // double buffer specific stuff:
	virtual void SwapBuffers();
	virtual void SetupBackBuffer();

	 //drag and drop helper functions
	virtual int   selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type, const char *which);
	virtual int   selectionPaste(char mid, const char *targettype);
	virtual int   selectionCopy(char mid);
	virtual char *getSelectionData(int *len,const char *property,const char *targettype,const char *selection);

 public:

	WindowStyle   *win_themestyle;
	anXApp        *app;
 	char          *win_name;
 	char          *win_title;
	anXWindow     *win_parent;
	int           win_screen;
	unsigned long win_style;
	int           win_x,win_y,win_w,win_h;
	int           win_border;
	int           win_pointer_shape;
	char          win_on;
	char          win_active;
	double        win_uiscale; //while this can be per window, usually it reflects global theme->ui_scale
	
	 //core functions needed by anXApp
	anXWindow(anXWindow *parnt, const char *nname, const char *ntitle,
			unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsend);
	virtual ~anXWindow();
	virtual const char *whattype() { return "anXWindow"; }
	virtual int WindowBorder();
	virtual const char *WindowTitle(int which=0);
	virtual void WindowTitle(const char *newtitle);
	virtual const char *tooltip(int mouseid=0);
	virtual const char *tooltip(const char *newtooltip);
	virtual anXWindow *findChildWindowByTitle(const char *title, bool recurse=false);
	virtual anXWindow *findChildWindowByName(const char *name, bool recurse=false);
	virtual anXWindow *findContainingChild(int x, int y);
	virtual int  Grayed();
	virtual int  Grayed(int g);
	virtual int  preinit();
	virtual int  init() { return 0; }
	virtual int  close();
	virtual int  Idle(int tid, double delta) { return 1; }
	virtual void Refresh() { Needtodraw(0); }
	virtual Displayer *MakeCurrent();
	virtual Displayer *GetDisplayer();
	virtual int  Needtodraw() { return needtodraw; }
	virtual void Needtodraw(int nntd) { needtodraw=nntd; }
	virtual int  deletenow() { return 1; }

	 //style functions
	virtual int SetWinStyle(unsigned int stylebit, int newvalue);
	virtual int HasWinStyle(unsigned int stylebit);
	virtual bool InstallColors(int category);
	virtual void InstallColors(WindowStyle *newcolors);
	virtual ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action_number);

	 //event dispatching functions
	virtual int Event(const EventData *data,const char *mes);
	virtual int ExposeChange(ScreenEventData *e) { Needtodraw(1); return 0; }

	virtual int DeviceChange(const DeviceEventData *e) { return 1; }
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int KeyUp(unsigned int ch,unsigned int state, const LaxKeyboard *kb) { return 1; }
	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m) { return 1; }
	virtual int ButtonDown(int button, int x,int y,unsigned int state,int count, const LaxMouse *m) { return 1; }
	virtual int ButtonUp  (int button, int x,int y,unsigned int state, const LaxMouse *m) { return 1; }
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d) { return 1; }
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d) { return 1; }
	virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d) { return 1; }
	virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d) { return 1; }
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d) { return 1; }
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d) { return 1; }
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d) { return 1; }
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d) { return 1; }
	virtual int ThemeChange(Theme *theme);

	virtual int FocusOn(const FocusChangeData *e);
	virtual int FocusOff(const FocusChangeData *e);

	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);

 public:
	 //control related stuff
	unsigned long win_owner;
	unsigned int  win_owner_send_mask;
	char         *win_sendthis;

	virtual void  contentChanged(); //this sends a ContentChange message to owners
	virtual void  selectionChanged(); //calling this sends a SelectionChange message to owners
	virtual bool DndWillAcceptDrop(int x, int y, const char *action, IntRectangle &rect, char **types, int *type_ret, anXWindow **child_ret);


	anXWindow *nextcontrol,*prevcontrol;
	virtual anXWindow *GetController();
	virtual int SelectNextControl(const LaxDevice *d);
	virtual int SelectPrevControl(const LaxDevice *d);
	virtual void ControlActivation(int on);
	virtual int AddPrevControl(anXWindow *prev) { return ConnectControl(prev,0); }
	virtual int AddNextControl(anXWindow *next) { return ConnectControl(next,1); }
	virtual int ConnectControl(anXWindow *towhat, int after=1);
	virtual int CloseControlLoop();
	virtual void SetOwner(anXWindow *nowner,const char *mes=NULL, unsigned int send_mask=0);
	virtual void SetOwner(unsigned long nowner_id,const char *mes=NULL, unsigned int send_mask=0);

	 //serializing aids
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//-------------------------- TimerInfo ----------------------------------------
struct TimerInfo
{
	int id;
	long info;
	clock_t endtime,firsttick,ticktime;
	clock_t nexttime;
	clock_t starttime, lastactualtime;
	double delta;
	EventReceiver *win;
	
	TimerInfo() { info=0; id=0; endtime=firsttick=ticktime=nexttime=0; win=NULL; }
	TimerInfo(EventReceiver *nwin,int duration,int firstt,int tickt,int nid,long ninfo);
	int checktime(clock_t tm);
	void Update(int next, int duration);
};


//---------------------------- anXApp --------------------------------------
class anXApp : virtual public anObject
{

#ifdef _LAX_PLATFORM_XLIB
 protected:
	 //X specific protected functions
	virtual void settimeout(struct timeval *timeout);
	virtual void processXevent(XEvent *event);

  public:
	 //X specific variables
	char              donotusex;
	char              use_xinput;
	Display          *dpy;
	//int               screen;
	//int               default_vscreen;
	Visual           *vis;
	Window            bump_xid;

	 //x input method variables *** not mpx compliant, fix me!!
	LaxDevice        *xim_current_device;
	unsigned long     xim_current_window;
	char              xic_is_over_the_spot;
	XIM               xim;
	XIC               xim_ic;
	XFontSet          xim_fontset;
	unsigned int      xim_deadkey;
	int filterKeyEvents(LaxKeyboard *kb, anXWindow *win,
							XEvent *e,unsigned int &key, char *&buffer, int &len, unsigned int &state);

	 //X specific public functions
	virtual XIC CreateXInputContext();
	virtual GC gc(int scr=0, int id=0);
	virtual anXWindow *findwindow_xlib(Window win);
	virtual anXWindow *findsubwindow_xlib(anXWindow *w,Window win);
	virtual int xlib_ScreenInfo(int screen, int *xx,int *yy, int *width,int *height,int *mmwidth,int *mmheight,int *depth);
	virtual void reselectForXEvents(anXWindow *win);
#endif //_LAX_PLATFORM_XLIB

#ifdef _LAX_PLATFORM_QT
#endif //_LAX_PLATFORM_QT


	virtual anXWindow *findwindow_by_id(unsigned long id);
	virtual anXWindow *find_subwindow_by_id(anXWindow *w,unsigned long id);

 private:
 protected:
	char                   *default_language;
	LaxImage               *default_icon;
	char                   *default_icon_file;
	char                   *copybuffer;

	ScreenInformation      *screeninfo;

	char                    dontstop;
	unsigned long           dialog_mask;
	LaxFiles::Attribute     app_resources;
	PtrStack<anXWindow>     dialogs;
	RefPtrStack<anXWindow>  topwindows;
	RefPtrStack<anXWindow>  outclickwatch;
	RefPtrStack<anXWindow>  todelete;
	PtrStack<EventReceiver> eventreceivers;
	EventData              *dataevents,*dataevente;
	PtrStack<TimerInfo>     timers;
	pthread_mutex_t         event_mutex;
	int maxtimeout;
	//int                     bump_fd;

	int                     ttcount;
	PtrStack<LaxDevice>     tooltipmaybe;
	void 					newToolTip(const char *text,int mouseid, anXWindow *ttwindow);

	 //initialization helpers
	virtual void setupdefaultcolors();
	virtual int getlaxrc(const char *filename,const char *profile);
	virtual void dump_in_rc(LaxFiles::Attribute *att, const char *profile);
	virtual void dump_out_rc(FILE *f, const char *profile, int indent, int what);

	 //event loop helper functions
	virtual void destroyqueued();
	virtual void resetkids(anXWindow *w);
	virtual void idle(anXWindow *w);
	virtual int refresh(anXWindow *w);
	virtual int processdataevents();
	virtual int processSingleDataEvent(EventReceiver *obj,EventData *ee);
	virtual int checkOutClicks(EventReceiver *obj,MouseEventData *ee);
	virtual int managefocus(anXWindow *ww, EventData *ev);
	virtual void tooltipcheck(EventData *event, anXWindow *ww);

  public:

	 //Other public variables
	anXWindow     *currentfocus;
	static anXApp *app;
	const char    *backend;
	char          *app_profile;
	int            buttoncount;
	FontManager   *fontmanager;
	LaxFont       *defaultlaxfont;
	char          *controlfontstr;
	char          *textfontstr;
	DeviceManager *devicemanager;

	int            tooltips;

	 //default Styling
	Theme         *theme;
//	unsigned long  color_tooltip_bg, color_tooltip_fg;
//	unsigned long  color_activeborder, color_inactiveborder;
//	int            default_border_width;
//	int			   default_padx;
//	int			   default_pady;
//	int			   default_bevel;
	int            max_window_size;

	unsigned int   dblclk,firstclk,idleclk;

	char          *load_dir;
	char          *save_dir;
	
	 //Other functions
	anXApp();
	virtual ~anXApp();
	virtual const char *whattype() { return "anXApp"; }

	 //app main operational functions
	virtual int SetTheme(const char *themeName);
	virtual int Backend(const char *which);
	virtual int    init(int argc,char **argv);
	virtual int   initX(int argc,char **argv);
	virtual int initNoX(int argc,char **argv);
	virtual int run(); 
	virtual int close();
	virtual void quit() { dontstop=0; }

	 //resources
	virtual int Tooltips(int on);
	virtual int has(int what);
	virtual LaxFiles::Attribute *AppResource(const char *name);
	virtual int AppResource(LaxFiles::Attribute *resource);
	virtual int DefaultIcon(const char *file);
	virtual int DefaultIcon(LaxImage *image, int absorb_count);
	virtual int ScreenInfo(int screen,int *x,int *y, int *width,int *height,int *mmwidth,int *mmheight,int *depth,int *monitor);
	virtual const char *Locale() const { return default_language; }
	virtual void Locale(const char *);

	 //special event functions
	virtual void bump();
	virtual EventReceiver *findEventObj(unsigned long id);
	virtual int RegisterEventReceiver(EventReceiver *e);
	virtual int UnregisterEventReceiver(EventReceiver *e);
	virtual int SendMessage(EventData *data, unsigned long toobj=0,
							const char *mes=0, unsigned long fromobj=0);

	 //window management functions
	virtual int rundialog(anXWindow *ndialog,anXWindow *wingroup=NULL,char absorb_count=1);
	virtual int addwindow(anXWindow *w,char mapit=1,char absorb_count=1);
	virtual int mapwindow(anXWindow *w);
	virtual int unmapwindow(anXWindow *w);
	virtual int reparent(anXWindow *kid,anXWindow *newparent);
	virtual int setfocus(anXWindow *win,clock_t t=0,const LaxKeyboard *kb=NULL);
	virtual int destroywindow(anXWindow *w);
	virtual int ClearTransients(anXWindow *w);

	 //system level queries
	virtual void RefreshScreenInfo();
	virtual ScreenInformation *FindNearestMonitor(int screen, double x, double y);

	 //drag-n-drop, cutting and pasting helpers
	virtual void postmessage(const char *str);
	virtual int CopytoBuffer(const char *stuff,int len);
	virtual char *GetBuffer();
#ifdef _LAX_PLATFORM_XLIB
	virtual anXWindow *findDropCandidate(anXWindow *ref,int x,int y,anXWindow **drop, Window *xlib_window_ret);
#endif //_LAX_PLATFORM_XLIB
	virtual anXWindow *findDropCandidate(anXWindow *ref,int x,int y,anXWindow **drop);

	 //timer management
	virtual int SetMaxTimeout(int timeoutmax);
	virtual int addtimer(EventReceiver *win,int strt,int next,int duration);
	virtual int modifytimer(EventReceiver *win, int timerid,int next,int duration);
	virtual int addmousetimer(EventReceiver *win);
	virtual int removetimer(EventReceiver *w,int timerid);
};


//---------------------------- DndState --------------------------------------

#ifdef _LAX_PLATFORM_XLIB

class DndState
{
  public:
  	int version;
  	anXWindow *targetTop;
  	anXWindow *targetChild;
  	XID source_window;
  	int preferred_type; //among data_types
  	char **data_types;  //null terminated
  	int num_data_types;
  	Atom action;

  	long timestamp;
  	int lastx, lasty; //in a drop candidate
  	int downx, downy; //used when initiating drop
  	int inx, iny, inw, inh;
  	int button; //anXWindow auto detects button up

  	enum DndStatus {
  		Uncertain,
  		Target_Accepts,
  		Target_Refuses
  	};
  	DndStatus status;
  	enum DndWhat {
  		Unknown,
  		Dropping,
  		WaitingForDrop
  	};
  	DndWhat mode;

  	DndState();
  	~DndState();
  	void SetNames(char **names, int len);
  	void SetTarget(anXWindow *top, anXWindow *child);
  	void Clear();

};

#endif //_LAX_PLATFORM_XLIB


} // namespace Laxkit

#endif

