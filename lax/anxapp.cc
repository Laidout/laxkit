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
//    Copyright (C) 2004-2013 by Tom Lechner
//

 
#include <fstream>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <cerrno>


#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <X11/Xatom.h>
#include <X11/keysymdef.h>

#ifdef LAX_USES_XINPUT2
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#else
#include <X11/extensions/XI.h>
#include <X11/extensions/XInput.h>
#ifndef GenericEvent
#  define GenericEvent 36
#endif
#endif


#include <lax/configured.h>


//-------backends---------
//#ifdef LAX_USES_IMLIB
#include <lax/laximlib.h>
#include <Imlib2.h>
//#endif

#ifdef LAX_USES_CAIRO
#include <lax/laxcairo.h>
#endif


#include <lax/anxapp.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/tooltip.h>
#include <lax/utf8utils.h>
#include <lax/laxutils.h>
#include <lax/lists.cc>
#include <lax/refptrstack.cc>

#include <iostream>
using namespace std;
#define DBG 




/*! \page todo
 *
 * \section anxapp anXApp todos
 * <pre>
 *  TODO:
 * 
 * 
 * *** VITAL!! improve focus handling, which currently REALLY SUCKS!
 * ***  if leave a window, then enter again, put focus on what was focused last
 * ***  focus handling generally pretty bad.
 * ***  AFTER_CLICK_FOCUS???
 * *** shortcuts, an event catch-all, if events propagate past topwindows.
 * *** cooperate somewhat with various freedesktop.org guidelines
 * ***    and/or import and export to how those guidelines say to keep track of things.
 * *** readup window classes and imp as per said guidelines
 * *** support for joysticks, wiimotes, tuio, osc, other
 * *** gl, fontconfig/freetype support?
 * *** multiple screen support
 * *** be able to ensure some degree of thread safety
 * 
 * *** WM_ICON_NAME, XGetIconSizes, ...
 * *** WM_ICON_SIZE <- usually set by window manager?
 * *** urgent hint in XWMHints??
 * 
 * *** think some more about fltk's callbacks and gtkmm signal connecting... could implement something
 * sort of similar (see ShortCut/action stuff shortcut.cc)
 * 
 * </pre>
 */


//! The mother of all Laxkit classes.
namespace Laxkit { 


//This tmsstruct is used as a dummy to call times() with.. 
//Man page says calling times(NULL) is ok on Linux, but not necessarily on non-Linux.
//Using this struct just in case..
static struct tms tmsstruct;

//------------------------- Extra utilities ---------------------------------

//TopWindow, IsWindowChild, and eventscreen are used in anXApp,
//xlib_event_name() is just used for debugging messages.

int eventscreen(int e_type,unsigned long mask);
const char *xlib_event_name(int e_type);

//! Check if a window (check) is the same as or is descended from another (top). 
/*!  \ingroup misc
 * Returns 1 if top==check,
 *  or if check is a child, returns n+1 where n is the number of
 *  generations below top. Thus if check is a direct child of top, then return 2.
 *  3 if it is a grandkid, etc.
 *
 *  Otherwise returns 0.
 */
int IsWindowChild(anXWindow *top,anXWindow *check)
{
	if (check==top) return 1;
	int n=1;
	while (check && check!=top) { check=check->win_parent; n++; }
	if (check) return n;
	return 0;
}

//! Return the top level window that win is somewhere nested within.
/*! \ingroup misc
 * A top level window has win_parent==NULL.
 */
anXWindow *TopWindow(anXWindow *win)
{
	if (!win) return NULL;
	while (win->win_parent) win=win->win_parent;
	return win;
}





//---------------------- WindowColors ---------------------------------
/*! \class WindowColors
 * \brief Holds basic color styling info.
 */

WindowColors::WindowColors()
{
	 //these are standard "Light" colors for anXApp::color_panel
	fg  	  =rgbcolor( 32, 32, 32);
	bg  	  =rgbcolor(192,192,192);
	hfg  	  =rgbcolor(  0,  0,  0);
	hbg  	  =rgbcolor(127,127,127);
	moverfg	  =rgbcolor( 32, 32, 32);
	moverbg	  =rgbcolor(164,164,164);
	grayedfg  =rgbcolor(100,100,100);
	color1	  =rgbcolor(128,128,128);
	color2	  =rgbcolor(128,128,128);
	activate  =rgbcolor(  0,200,  0);
	deactivate=rgbcolor(255,100,100);
}

//! Initialize the color info.
WindowColors::WindowColors(const WindowColors &l)
{
	fg  	  = l.fg;
	bg  	  = l.bg;
	hfg  	  = l.hfg;
	hbg  	  = l.hbg;
	moverfg	  = l.moverfg;
	moverbg	  = l.moverbg;
	grayedfg  = l.grayedfg;
	color1	  = l.color1;
	color2	  = l.color2;
	activate  = l.activate;
	deactivate= l.deactivate;
}

//! Copy the color info.
WindowColors &WindowColors::operator=(WindowColors &l)
{
	fg  	  = l.fg;
	bg  	  = l.bg;
	hfg  	  = l.hfg;
	hbg  	  = l.hbg;
	moverfg	  = l.moverfg;
	moverbg	  = l.moverbg;
	grayedfg  = l.grayedfg;
	color1	  = l.color1;
	color2	  = l.color2;
	activate  = l.activate;
	deactivate= l.deactivate;
	return l;
}


//---------------------- WindowFonts ---------------------------------
///*! \class WindowFonts
// * \brief Holds basic styling info about fonts.
// */
//
//WindowFonts::WindowFonts()
//{	normal=bold=italic=edit=NULL; }
//
//WindowFonts::WindowFonts(LaxFont *nnormal, LaxFont *nbold, LaxFont *nitalic, LaxFont *nmono)
//{
//	normal=nnormal;
//	bold=nbold;
//	italic=nitalic;
//	mono=nmono;
//
//	if (normal) normal->inc_count();
//	if (bold) bold->inc_count();
//	if (italic) italic->inc_count();
//	if (mono) mono->inc_count();
//}
//WindowFonts::~WindowFonts()
//{	
//	if (normal) normal->dec_count();
//	if (bold) bold->dec_count();
//	if (italic) italic->dec_count();
//	if (edit) edit->dec_count();
//}


//---------------------------------- TimerInfo --------------------------------

/*! \struct TimerInfo
 * \brief The TimerInfo class is used with anXApp.
 *
 * Timers can be added to any window, by calling anXApp::addtimer() or
 *  anXApp::addmousetimer(), and if
 *  they don't automatically expire, are deleted with anXApp::removetimer(). Users
 *  shouldn't actually have to deal with TimerInfo structs directly. addtimer 
 *  returns a timerid, which is later passed to anXWindow::Idle().
 *
 *  \todo for mouse button down timers, maybe use info field set to which button pressed, if it was
 *    a mouse timer, for instance, then have app check when the button is up,
 *    and remove the timer then?
 */


/*! All the time values are passed in as milliseconds, but internally, they are
 * converted to clock ticks. If duration==-1, then duration is taken to be 1 million,
 * which is 1000 hours.
 * 
 * For the span of the timer, win->Idle(timer->id) is called every tickt milliseconds (the first 
 * tick is sent after firstt milliseconds), until the time is after the current time plus duration.
 *
 * Please not that if the current time is several ticks ahead of the last tick, then only one tick is sent
 * to Idle().
 */
TimerInfo::TimerInfo(EventReceiver *nwin,int duration,int firstt,int tickt,int nid,long ninfo)
{
	win=nwin;
	id=nid;
	info=ninfo;
	if (tickt<=0) tickt=100;

	clock_t curtime=times(NULL);
	firsttick=firstt  *sysconf(_SC_CLK_TCK)/1000; // convert firstt to clock ticks
	ticktime= tickt*sysconf(_SC_CLK_TCK)/1000; // convert ticktime to clock ticks
	if (duration!=-1) {
		duration= duration*sysconf(_SC_CLK_TCK)/1000; // convert duration to clock ticks
		endtime=curtime+duration;
	} else endtime=-1;

	nexttime=curtime+firsttick;
}

//! Check if a tick needs to be sent, and call win->Idle(id) if necessary.
/*! Updates nexttime, which is the next time at which a tick should occur.
 * Returns -1 if the timer has expired, else return the number of ticks that have happened.
 * Thus, 0 means a tick has not occured. If a window.Idle(id) returns nonzero, then
 * anXApp removes the timer.
 */
int TimerInfo::checktime(clock_t tm)
{
	int t=0;
	while (nexttime<=tm) { t++; nexttime+=ticktime; }

	if (t && win && win->Idle(id)) {
		return -1; //nonzero win->Idle means remove timer
	}

	if (endtime!=-1 && nexttime>endtime) {
		return -1;
	}
	
	return t;
}

//-------------------------- aDrawable ----------------------------------------
/*! \class aDrawable
 * \brief Class to facilitate various double buffer and in-memory pixmap rendering.
 */

//------------------------------------ anXApp -----------------------------------

anXApp *anXApp::app=NULL;

/*! \class anXApp
 *  \brief This class is the control unit for programs.
 *
 *  All the lowest level state required to run an X program is kept and maintained
 *  in this class and in anXWindow. 
 *  Also, fonts are kept here via
 *  a reference counting FontManager instance, and outside functions can get 
 *  some common color functions such as converting a 3 number (int,int,int) color 
 *  specification to a pixel value that can be used with the various Xlib functions.
 *
 *  A typical main() function looks like the following. Derived classes can
 *  redefine the init function to be more interesting:
 *  \code
 *  int main(int argc, char **argv)
 *    anXApp app;
 *    app.init(argc,argv);
 *    
 *    (...app.addwindow(...) and such...)
 *
 *    return app.run();
 *  }
 *  \endcode
 *  
 *  If at any point the program wants to stop running, then app->quit() should be called,
 *  which sets dontstop=0, and forces a break out of the run loop.
 *
 * <pre>
 * NOTES
 * 
 * Apparently between XCreating a window and the initial mapping, XResizes don't have any effect:
 * seems the XResize goes through (check return value!), but then a ConfigureNotify is sent with old values.
 * Can you explain this?
 * </pre>
 */
/*! \var static anXApp *anXApp::app
 * \brief In the constructor, this variable is set to this.
 * 
 * This can be used by any helper functions to access anything in an application's anXApp instance
 * (of which there should be only one).
 */

/*! \var unsigned int anXApp::dblclk
 * \brief Milliseconds within which a button has to be pressed again to be a double click, or triple, etc.
 */
/*! \var unsigned int anXApp::firstclk
 * \brief Number of clock ticks to wait before throwing another click for when a button is held down.
 */
/*! \var unsigned int anXApp::idleclk
 * \brief Number of clock ticks to wait between clicks after the first click when a button is held down.
 */

/*! \var PtrStack<anXWindow> anXApp::dialogs
 * \brief Stack of dialogs. See rundialog().
 */
/*! \var RefPtrStack<anXWindow> anXApp::topwindows
 * \brief Stack of top level windows. These windows have no parent.
 */
/*! \var RefPtrStack<anXWindow> anXApp::todelete
 * \brief Stack of windows to delete after the event queue is depleted.
 *
 * Windows are pushed onto here from destroywindow(), and they are actually deleted
 * from destroyqueued().
 */
/*! \var PtrStack<TimerInfo> anXApp::timers
 * \brief Stack of timers.
 */

/*! \var int anXApp::tooltips
 * \brief Number of milliseconds to wait once a mouse enters a window before popping up a tooltip.
 *
 * If 0, then tooltips are disabled.
 */
/*! \var int anXApp::ttcount
 * \brief Special tag for whether tooltips are temporarily disabled.
 *
 * Calls to Tooltips() increment and decrement this. During pointer grabs, for instance,
 * tooltips would wreck havoc, so this flag is to temporarily disable them.
 * 
 * ttcount==0 means use tooltips, and any other value means do not use tooltips.
 *
 * \todo maybe a negative value could signal that tooltips should pop up automatically
 *   when entering a new window. This would be when you hover over a window and get a tooltip,
 *   and the user wants to scan the tooltips of all adjacent windows without waiting the hover
 *   time.
 */

/*! \var Visual *anXApp::vis
 * Used to set up color shift masks for use in anXApp::rgbcolor() and anXApp::colorrgb().
 * Also passed to XCreateWindow in anXApp::addwindow().
 *
 * \todo *** figure out what is necessary tweaking of vis to easily enable opengl
 *   integration. little test on my machine allowed gl windows using default vis..
 *
 * for gl tweaking, need to incorporate:
 * <pre>
 * at anXApp level:
 *
 * static int  attributeListDouble[]  =  {
 *                 GLX_RGBA,               
 *                 GLX_DOUBLEBUFFER,               
 *                 GLX_RED_SIZE,   1,      
 *                 GLX_GREEN_SIZE, 1,
 *                 GLX_BLUE_SIZE,  1,
 *                 GLX_DEPTH_SIZE, 1,
 *                 None 
 *         };
 *  GLXContext cx;
 *  XVisualInfo *glvi;
 *  glvi = glXChooseVisual(dpy, DefaultScreen(dpy), attributeListDouble);
 *  cx = glXCreateContext(dpy, glvi, 0, GL_TRUE);
 *
 * at anXWindow level:
 *  call glXMakeCurrent(dpy, win->xlib_window, cx);  before gl calls
 * </pre>
 */
/*! \var LaxFiles::Attribute anXApp::app_resources
 * \brief A simple resource holder.
 *
 * This is used by, for instance, FileDialog to remember where the dialog was on
 * the screen if the dialog is closed and opened. 
 *
 * If you want an application to remember such settings between starting and stopping,
 * then your class derived from anXApp must implement its own file save and load. This is
 * simply a matter of choosing a file to save to, and calling app_resources.dump_out(FILE*,int)
 * and app_resources.dump_in(file,0,NULL).
 */
/*! \var char *anXApp::app_profile
 * \brief The default application profile, if any.
 *
 * The only profiles built in are "Dark" and "Light". Subclasses would set this variable
 * to something before init() is called. In init(), getlaxrc(NULL,app_profile) is called, in
 * order to try to grab any further profile values from the laxrc file if any.
 */
/*! \var int anXApp::screen
 * \brief The default screen for dpy, the X connection.
 */
/*! \var int *anXApp::xscreeninfo
 * \brief Width, height, and depth of each available screen.
 *
 * xscreeninfo[0] is the number of screens, and each group of 5 ints after that are w,h,mmw,mmh,d.
 * It is necessary to have this separate, because some devices that exist outside of X need
 * info provided by X, and the threads they catch their own events do not play nice with X
 * (as of May, 2011, anyway). See ScreenInfo() for more.
 */
/*! \var char *anXApp::textfontstr
 * \brief The default font for text inside text edits. It is a fontconfig pattern string.
 */
/*! \var char *anXApp::controlfontstr
 * \brief The default font string. It is a fontconfig pattern string.
 */
/*! \var char anXApp::use_xinput
 * \brief What type of extended devices to get data from.
 *
 * If 0, then use only the core pointer and keyboard, no extension devices.
 * If 1, then use old style XInput extension functions.
 * If 2, then use XInput2 extension functions, if possible. If that is not possible, then
 * fall back to XInput1 devices.
 */
/*! \var int  anXApp::buttoncount
 * \brief Number of times a mouse button has been pressed in succession.
 *
 * anXApp keeps this to check on the simplest case of a single button being pressed
 * rapidly. Any other buttons also being pressed during the same time period will
 * reset this counter.
 */
/*! \var unsigned long anXApp::dialog_mask
 * \brief Mask used for screening events when dialogs are up.
 *
 * The default is not allow input and focus events, and let other events like expose
 * and enter/exit events pass.
 */

/*! \fn void anXApp::quit()
 * \brief Called anytime from anybody, tells the application to shut down.
 *
 * This basically forces a break out of the event loop, thus ending the program.
 */

/*! \var anXApp::maxtimeout
 * \brief Wait time to check for new messages, in microseconds. 
 *
 * If bump() doesn't seem to work, set this to something nonzero to force breaking
 * out of select().
 */
/*! \var char *anXApp::default_language
 * Normally empty string "". You can change this with Locale(const char*) to be
 * some specific language. Otherwise, uses the current language settings
 * for your environment.
 */

//! Constructor. Sets anXApp::app==this.
/*! Sets load_dir and save_dir to the current directory.
 *
 * Initializes various variables not dependent on an X connection nor on a laxrc file.
 */
anXApp::anXApp()
{
	 //--------- note that nothing in this constructor depends on an x connections
	XInitThreads();

	app=this;
	dpy=NULL;
	vis=NULL;
	dialog_mask=LAX_DIALOG_INPUTS;
	donotusex=0;
	use_xinput=2;
	devicemanager=NULL;
	//bump_fd=-1;
	bump_xid=0;
	maxtimeout=0; //override timeout when bump() doesn't work. microseconds

	default_language=newstr("");

	screen=0;
	xscreeninfo=NULL;

	 //base default styling
	color_panel=color_menu=color_edits=color_buttons=NULL; //these are initialized in init()

	 // set up the default loading and saving directories to the current directory.
	char *dir=getcwd(NULL,0);
	if (dir) {
		load_dir=newstr(dir);
		save_dir=newstr(dir);
		free(dir);
	} else load_dir=save_dir=NULL;
	
	copybuffer=NULL;
	currentfocus=NULL;
	app_profile=NULL;
	backend=LAX_DEFAULT_BACKEND;
	ttcount=0;
	tooltips=1000; // time to wait till tooltip pops up in milliseconds, 0==no tooltips

	dontstop=1;
	
	 // default click times, in clock ticks
	dblclk=(unsigned int)(1./5*1000); // _SC_CLK_TCK=ticks per second, put dblclk in milliseconds, which is the time in events..
	firstclk=sysconf(_SC_CLK_TCK)/7;
	idleclk=sysconf(_SC_CLK_TCK)/15;
	DBG cerr <<"_SC_CLK_TCK="<<sysconf(_SC_CLK_TCK)<<"  dblclk:"<<dblclk<<" firstclk:"<<firstclk<<" idleclk:"<<idleclk<<endl;

	dataevents=dataevente=NULL;

	xim_current_device=NULL;
	xim=NULL;
	xim_ic=NULL;
	xim_fontset=NULL;
	xim_deadkey=0;

	fontmanager=NULL;
	defaultlaxfont=NULL;

	textfontstr   =newstr("sans-12");//***maybe should migrate this to fontmanager?
	controlfontstr=newstr("sans-12");//***maybe should migrate this to fontmanager?

	default_icon_file=NULL;
	default_icon=NULL;


	default_border_width=1;
	default_padx=5;
	default_pady=5; 
	default_bevel=5;

	 //set up standard mutexes
	pthread_mutex_init(&event_mutex,NULL);
}

//! Destructor.
/*! Calls close(), and generally deallocates any remaining things.
 */
anXApp::~anXApp()
{
	close();

	if (default_icon) default_icon->dec_count();

	delete[] default_language;
	delete[] default_icon_file;
	if (load_dir) delete[] load_dir;
	if (save_dir) delete[] save_dir;
	if (controlfontstr) delete[] controlfontstr;
	if (app_profile) app_profile=NULL;
	if (copybuffer) delete[] copybuffer;

	
	if (color_panel)   color_panel->dec_count();
	if (color_menu)    color_menu->dec_count();
	if (color_edits)   color_edits->dec_count();
	if (color_buttons) color_buttons->dec_count();

	if (xscreeninfo) delete[] xscreeninfo;

	pthread_mutex_destroy(&event_mutex);
}

/*! Set the string representing the default language. Use "" for system default.
 * Then calls setlocale(LC_ALL,default_language).
 */
void anXApp::Locale(const char *lang)
{
	makestr(default_language, lang?lang:"");
	setlocale(LC_ALL,default_language);
}

/*! This should be called immediately after contstructor.
 * It merely sets app_profile to theme. app_profile is used 
 * later on to determine default color options.
 *
 * Default is to simply return 0.
 */
int anXApp::Theme(const char *theme)
{
	makestr(app_profile,theme);
	return 0;
}

/*! If they have been compiled in, which can be one of:
 *   "imlib"
 *   "cairo"
 *   "gl"
 *
 * Return 0 for success, or 1 for cannot use.
 */
int anXApp::Backend(const char *which)
{
	if (!strcmp(which,"imlib")) {
#ifdef LAX_USES_IMLIB
		backend="xlib";
		return 1;
#else
		cerr <<" Warning! Imlib2 has not been compiled in!"<<endl;
		return 1;
#endif
	}

	if (!strcmp(which,"cairo")) {
#ifdef LAX_USES_CAIRO
		backend="cairo";
		return 1;
#else
		cerr <<" Warning! Cairo has not been compiled in!"<<endl;
		return 1;
#endif
	}

	if (!strcmp(which,"gl")) {
#ifdef LAX_USES_GL
		backend="gl";
		return 1;
#else
		cerr <<" Warning! GL has not been compiled in!"<<endl;
		return 1;
#endif
	}

	DBG cerr <<"Backend is now: "<<backend<<endl;
	return 1;
}

//! This is supposed to initialize for certain locales that require extras to input their language.
/*! This is called every time a window is focused to set up xim_ic to something proper.
 * The old xim_ic is destroyed, and a new one created to take its place. When a window calls
 * this function, right afterwards, it installs itself into the returned XIC.
 *
 * The return value is xim_ic.
 *
 * \todo for multiple keyboard devices, there would be one xim_ic for each keyboard, since
 *    there would be one focus per keyboard
 */
XIC anXApp::CreateXInputContext()
{
 	 //***this is adapted from fl_new_ic() in src/x11/run.cxx from Fltk.org

	if (!xim) return NULL;

	if (xim_ic) XDestroyIC(xim_ic);
	xim_ic=NULL;

	if (!xim_fontset) {
		char          **missing_list;
		int           missing_count;
		char          *def_string; //do not free this, owned by Xlib if returned non-null
		xim_fontset = XCreateFontSet(dpy, "-misc-fixed-medium-r-normal--14-*",
				&missing_list, &missing_count, &def_string);
		if (missing_count) {
			DBG for (int c=0; c<missing_count; c++) cerr<<"xim_fontset create: missing charset "<<missing_list[c]<<endl;
			XFreeStringList(missing_list);
		}
	}

	XIMStyles* xim_styles=NULL;
	if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL, NULL) 
			|| !xim_styles 
			|| !xim_styles->count_styles) {
		DBG cerr << "No XIM style found\n" <<endl;
		XCloseIM(xim);
		xim=NULL;
		if (xim_styles) XFree(xim_styles);
		return NULL;
	}
	
	//DBG cerr <<"X input methods:"<<endl;
	//DBG for (int c=0; c<xim_styles->count_styles; c++)
	//DBG		cerr <<"  "<<xim_styles->supported_styles[c]<<endl;

	bool preedit = false;
	bool statusarea = false;
	XRectangle status_area_rect;
	for (int i = 0; i < xim_styles->count_styles; ++i) {
		XIMStyle* style = xim_styles->supported_styles+i;
		if (*style == (XIMPreeditPosition | XIMStatusArea)) {
			statusarea = true;
			preedit = true;
		} else if (*style == (XIMPreeditPosition | XIMStatusNothing)) {
			preedit = true;
		}
	}
	XFree(xim_styles);

	if (preedit) {
		XPoint	spot;
		spot.x = 0;
		spot.y = 0;
		XVaNestedList preedit_attr = XVaCreateNestedList(0,
										XNSpotLocation, &spot,
										XNFontSet, xim_fontset, NULL);

		if (statusarea) {
			XVaNestedList status_attr = XVaCreateNestedList(0,
											XNAreaNeeded, &status_area_rect,
											XNFontSet, xim_fontset, NULL);
			xim_ic = XCreateIC(xim,
								XNInputStyle, (XIMPreeditPosition | XIMStatusArea),
								XNPreeditAttributes, preedit_attr,
								XNStatusAttributes, status_attr,
								NULL);
			XFree(status_attr);
		}
		if (!xim_ic)
			xim_ic = XCreateIC(xim,
								XNInputStyle,XIMPreeditPosition | XIMStatusNothing,
								XNPreeditAttributes, preedit_attr,
								NULL);
		XFree(preedit_attr);
		if (xim_ic) {
			xic_is_over_the_spot = 1;
			XVaNestedList status_attr;
			status_attr = XVaCreateNestedList(0, XNAreaNeeded, &status_area_rect, NULL);
			if (status_area_rect.height != 0)
			XGetICValues(xim_ic, XNStatusAttributes, status_attr, NULL);
			XFree(status_attr);
			return xim_ic;
		}
	}

	 // create a non-preedit input context:
	xic_is_over_the_spot = 0;
	xim_ic = XCreateIC(xim,
						XNInputStyle,
						(XIMPreeditNothing | XIMStatusNothing),
						NULL);
	if (!xim_ic) {
		DBG cerr << "XCreateIC() failed\n" <<endl;
		XCloseIM(xim);
		xim=NULL;
	}
	
	return xim_ic;
}

//! Upkeep anXApp::ttcount.
/*! Inc if (on) else dec. ttcount will never be less than 0.
 * Returns ttcount==0, that is, whether tooltips are active.
 */
int anXApp::Tooltips(int on)
{
	if (on) ttcount--;
	else ttcount++;
	if (ttcount<0) ttcount=0;
	return ttcount==0;
}


//! Return pointer to the resource named name.
/*! The returned Attribute objects are not copies or reference counted, so calling code
 * should do something with it quick, in case the resource is otherwise deleted.
 * The calling code should NOT delete the returned Attribute.
 *
 * This is currently used by various dialogs to store their window size and position settings,
 * stored by their class name. Also used to store file bookmarks as a resource named "Bookmarks".
 */
LaxFiles::Attribute *anXApp::AppResource(const char *name)
{
	return app_resources.find(name);
}

//! Append to or replace a resource. The resource belongs to the type of thing in resource->name.
/*! IMPORTANT: If resource is different than the one already in *this, then it is removed, and
 * totally replaced with the new one. Otherwise, nothing is done, since it's the same one on the stack.
 * LaidoutApp takes possession of resource, meaning it becomes responsible for deleting it.
 *
 * Return 0 for success, or nonzero for error.
 *
 * \todo might be wise to have reference counted resources, but that's maybe getting too complicated
 *    for what these resources are mainly meant for, that is: info used upon dialog creation
 */
int anXApp::AppResource(LaxFiles::Attribute *resource)
{
	if (!resource) return 1;
	LaxFiles::Attribute *att=app_resources.find(resource->name);
	if (att && att==resource) return 0; //already there

	 //else remove old, push new
	app_resources.attributes.remove(app_resources.attributes.findindex(att));
	app_resources.push(resource,-1);
	return 0;
}

//! Easy check for whether there are various capabilities in the application.
/*! This can test for whether various capabilities have been compiled into the Laxkit,
 * or is otherwise available. 
 *
 * By default, you can check for:
 * <pre>
 *  LAX_HAS_IMLIB2
 *  LAX_HAS_CAIRO
 *  LAX_HAS_GL
 *  LAX_HAS_XINPUT2
 * </pre>
 */
int anXApp::has(int what)
{
	if (what==LAX_HAS_IMLIB2) {
#ifdef LAX_USES_IMLIB
		return 1;
#else 
		return 0;
#endif

	} else if (what==LAX_HAS_CAIRO) {
#ifdef LAX_USES_CAIRO
		return 1;
#else 
		return 0;
#endif

	} else if (what==LAX_HAS_GL) {
#ifdef LAX_USES_GL
		return 1;
#else 
		return 0;
#endif

	} else if (what==LAX_HAS_XINPUT2) {
#ifdef LAX_USES_XINPUT2
		return 1;
#else 
		return 0;
#endif
	}
	return 0;
}


//! Init the app. The default is to call initX() if donotusex==0, otherwise wall initNoX().
int anXApp::init(int argc,char **argv)
{
	return donotusex ? initNoX(argc,argv) : initX(argc,argv);
}

//! Init the app without accessing X.
/*! Responsibilities here include:\n
 * - set up color masks and such
 * - read in from any default laxrc
 * - set up default graphics context for drawing controls
 * - set up default styles/colors by reading in the proper sequence of laxrc files
 *
 * Currently, no options are recognized, but that may change.
 * 
 * \todo *** this can be further broken down to call specialized functions for each task
 */
int anXApp::initNoX(int argc,char **argv)
{
	 //prepare rgbcolor() and colorrgb() to work with 8 bits per channel
	set_color_shift_info(0xff, 0xff00, 0xff0000, 0xff000000);

	 // setup default colors and border width, then update with any rc files
	setupdefaultcolors();
	default_border_width=1;
	getlaxrc(NULL,app_profile);
	
	 // setup default graphics context
	//*** maybe set up some kind of buffer area on demand?

	return 0; 
}

//! Init the app. Open an X connection and set default stuff that depends on that.
/*! Responsibilities here include:\n
 * - open x connection
 * - initialize the x cut buffer
 * - read in from any default laxrc
 * - set up color masks and such
 * - set up default font/XOM/XIM/XIC/XOC
 * - set up default graphics context for drawing controls
 * - set up default styles/colors by reading in the proper sequence of laxrc files
 *
 * Currently, no options are recognized, but that may change.
 * 
 * \todo *** this can be further broken down to call specialized functions for each task
 */
int anXApp::initX(int argc,char **argv)
{
	 //---------- set up connection to x server
	dpy=XOpenDisplay(NULL);
	if (!dpy) { 
		cerr << "Cannot open X server.\n"; 
		exit(1); 
	}

	screen=DefaultScreen(dpy);
	int numscreens=XScreenCount(dpy);
	if (xscreeninfo) delete[] xscreeninfo;
	xscreeninfo=new int[1+5*numscreens];
	xscreeninfo[0]=numscreens;
	for (int c=0; c<numscreens; c++) {
		xlib_ScreenInfo(c,&xscreeninfo[1+3*c  ], //w
						  &xscreeninfo[1+3*c+1], //h
						  &xscreeninfo[1+3*c+2], //mm w
						  &xscreeninfo[1+3*c+3], //mm h
						  &xscreeninfo[1+3*c+4]  //depth
					   );
	}

	vis=DefaultVisual(dpy,screen);
	if (vis->c_class!=DirectColor && vis->c_class!=TrueColor) {
		cerr << "This program must be run with TrueColor or DirectColor.\n";
		exit(1);
	}

	DBG cerr <<"Attempting backend: "<<(backend?backend:"(none specified)")<<endl;

	if (!strcmp(backend,"xlib")) {
		//InitImlib2Backend();
		InitLaxImlib(900);
		if (!fontmanager) fontmanager=GetDefaultFontManager();

	} else if (!strcmp(backend,"cairo")) {
#ifdef LAX_USES_CAIRO
		InitLaxCairo();
		if (!fontmanager) fontmanager=GetDefaultFontManager();
#endif

	} else if (!strcmp(backend,"gl")) {
		cerr <<" ** Error! gl backend not implemented yet. Lazy programmers!!!"<<endl;
	} 

	GetDefaultDisplayer(); //initializes if null
	defaultlaxfont=fontmanager->MakeFontFromStr(controlfontstr,getUniqueNumber());
	DBG defaultlaxfont->suppress_debug=1;
	//if (load_image==NULL) InitDefaultBackend(); //<-- have this be something configure made?


	 //--------------- initialize the cut buffer
	unsigned char b[2]={0,0};
	Window rw=DefaultRootWindow(dpy);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER0,XA_STRING,8,PropModeAppend,b,0);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER1,XA_STRING,8,PropModeAppend,b,0);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER2,XA_STRING,8,PropModeAppend,b,0);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER3,XA_STRING,8,PropModeAppend,b,0);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER4,XA_STRING,8,PropModeAppend,b,0);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER5,XA_STRING,8,PropModeAppend,b,0);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER6,XA_STRING,8,PropModeAppend,b,0);
	XChangeProperty(dpy,rw,XA_CUT_BUFFER7,XA_STRING,8,PropModeAppend,b,0);
	

	 //prepare rgbcolor() and colorrgb() to work right for vis
	set_color_shift_info(vis->red_mask, vis->green_mask, vis->blue_mask, 0);

	 //------------- setup default colors and border width, then update with any rc files
	setupdefaultcolors();
	getlaxrc(NULL,app_profile);
	

	 //----------- setup default font and xim stuff
	XSetLocaleModifiers("");
	xim=XOpenIM(dpy, NULL, NULL, NULL);


	 //----------------set up device manager if necessary
	if (!devicemanager) {
		if (use_xinput) {
#ifdef LAX_USES_XINPUT2
			if (use_xinput==2) devicemanager=newXInput2DeviceManager(dpy,XIAllMasterDevices);
#endif
			//devicemanager=new DeviceManagerXInput1();
		} 
		if (!devicemanager) devicemanager=newCoreXlibDeviceManager(dpy);


		if (devicemanager) devicemanager->init();
	}

	 //create a dummy window that makes it easy to break out of select from a different thread
	XSetWindowAttributes xatts;
	bump_xid=XCreateWindow(dpy,
							DefaultRootWindow(dpy),
							0,0,1,1,
							0,
							CopyFromParent, //depth
							CopyFromParent, //class, from: InputOutput, InputOnly, CopyFromParent
							vis,
							0,&xatts);


	return 0; 
}


//! Shut down the app manually.
/*! This will remove any resources that have been allocated.
 * If dpy==NULL, then of course just remove those bits that are not
 * dependent on an x connection.
 *
 * This function is provided just in case you want to shut down, then
 * do other stuff for some reason. It is also called from the destructor.
 */
int anXApp::close()
{
	 //not necessarily x dependent stuff:
	if (fontmanager) { delete fontmanager; fontmanager=NULL; }

	if (!dpy) return 0;


	 //close all x dependent stuff:

	SetDefaultDisplayer(NULL);

	 //-------close down any device manager
	if (devicemanager) { delete devicemanager; devicemanager=NULL; }

	 //-------- close down any remaining windows
	 // topwindows autodestructs, but must call any remaining necessary XDestroyWindow
	for (int c=0; c<topwindows.howmany(); c++) {
		if (topwindows.e[c]->xlib_window!=0) {
			XDestroyWindow(dpy,topwindows.e[c]->xlib_window); // also destroys sub(Window)s
			topwindows.e[c]->xlib_window=0;
		}
	}
	dialogs.flush();
	DBG cerr <<"removing remaining topwindows..."<<endl;
	topwindows.flush(); // not really necessary, done here just to remind
	if (bump_xid) {
		XDestroyWindow(dpy,bump_xid);
		bump_xid=0;
	}
	
	 //----------cleanup X input method stuff
    if (xim) { 
		if (xim_ic) { XDestroyIC(xim_ic); xim_ic=NULL; }
		XCloseIM(xim); 
		xim=NULL; 
	}

	 //--------destroy any x dependent things
	if (defaultlaxfont) { defaultlaxfont->dec_count(); defaultlaxfont=NULL; }

	 //------------close display
	DBG cerr <<"closing display.."<<endl;
	XCloseDisplay(dpy);
	dpy=NULL; 
	DBG cerr <<"closing display done."<<endl;


	return 0;
}

//! Copy stuff to the X cutbuffer.
/*! len<0 means make len=strlen(stuff)
 */
int anXApp::CopytoBuffer(const char *stuff,int len)
{
	if (!stuff || len==0) return 1;
	if (len<0) len=strlen(stuff);
	if (!dpy) return 1;

	makenstr(copybuffer,stuff,len);
	
	DBG cerr <<"anxapp--copy"<<endl;
	XRotateBuffers(dpy,1);
	XStoreBuffer(dpy,stuff,len,0);
	return 0;
}

//! Get a new'd copy of stuff from the X cutbuffer.
/*! \todo *** this appears to be broken, cannot copy stuff from terminal...
 */
char *anXApp::GetBuffer()
{
	DBG cerr <<"--paste";
	int n;
	char *blah=XFetchBuffer(dpy,&n,0);
	
	DBG cerr <<": "<<n<<": "<<blah<<endl;
	if (n==0) return NULL;
	char *ret=new char[n+1];
	strcpy(ret,blah);
	ret[n]='\0';
	XFree(blah);
	return ret;
}

//! Set the default icon to the image in file.
/*! This only works after anXApp has opened an X connection, and should be done
 * before any windows are added. You must also call InitLaxImlib() before calling this.
 * This function does nothing if Imlib2 is not available.
 *
 * Top level windows get their icon hint set to this.
 *
 * Returns 0 for success, or nonzero for error.
 *
 * \todo must implement checking for appropriate size for icons, currently just
 *    assumes 48x48 is proper, and that is what is in file...
 * \todo must implement freedesktop _NET_WM_ICON. currently just does xwmhints
 * \todo must implement some way to avoid direct dependence on Imlib
 */
int anXApp::DefaultIcon(const char *file)
{
	makestr(default_icon_file,file);
	return 0;
}

//! Use this data and mask for the default icon for windows.
/*! Top level windows get their icon hint set to this.
 *
 * \todo this function should probably do existence checking,
 *     scaling, and freeing of old pixmaps..
 */
int anXApp::DefaultIcon(LaxImage *image, int absorb_count)
{
	if (default_icon) default_icon->dec_count();

	default_icon=image;
	if (!absorb_count) default_icon->inc_count();
	return 0;
}

//! Return information about a screen, finding it on the fly with Xlib, rather than consult xscreeninfo.
/*! This is like ScreenInfo(), but is separate to prevent strange device thread 
 * related snafus. If you create a device that catches events in its own thread,
 * do not use this function, as your program will likely crash.
 */
int anXApp::xlib_ScreenInfo(int screen,int *width,int *height,int *mmwidth,int *mmheight,int *depth)
{
	Window root=RootWindow(dpy,screen);
	Window rootret;
	int x,y;
	unsigned int w,h,border,d;
	XGetGeometry(dpy,root,&rootret, &x,&y, &w,&h, &border,&d);
	int n=0;
	if (width) { *width=w;  n++; }
	if (height){ *height=h; n++; }
	if (depth) { *depth=d;  n++; }
	if (mmwidth)  { *mmwidth=DisplayWidthMM(dpy,screen);  n++; }
	if (mmheight) { *mmwidth=DisplayHeightMM(dpy,screen); n++; }
	return n;
}

//! Return information about a screen, based on information in xscreeninfo which gets defined in init().
/*! Return the width, height, and depth information about a screen, if the
 * corresponding pointers are not NULL. The mmwidth and mmheight are in millimeters. These are usually
 * returned by the monitor, and may or may not be accurate.
 *
 * Return how many of the requested variables were found (this 0 for none found).
 */
int anXApp::ScreenInfo(int screen,int *x,int *y, int *width,int *height,int *mmwidth,int *mmheight,int *depth,int *virt)
{
	if (!xscreeninfo || screen<0 || screen>=xscreeninfo[0]) return 0;

	int n=0;
	if (x) { *x=0; n++; }
	if (y) { *y=0; n++; }
	if (width)   { *width   =xscreeninfo[1+3*screen];   n++; }
	if (height)  { *height  =xscreeninfo[1+3*screen+1]; n++; }
	if (mmwidth) { *mmwidth =xscreeninfo[1+3*screen+2]; n++; }
	if (mmheight){ *mmheight=xscreeninfo[1+3*screen+3]; n++; }
	if (depth)   { *depth   =xscreeninfo[1+3*screen+4]; n++; }
	if (virt) { *virt=0; n++; }
	return n;
}

//! This will be called in response to a device hierarchy change via XInput2.
/*! This recurses through all existing windows, and calls devicemanager->selectForWindow().
 */
void anXApp::reselectForXEvents(anXWindow *win)
{
	if (!win) {
		for (int c=0; c<topwindows.n; c++) reselectForXEvents(topwindows.e[c]);
		return;
	}

	devicemanager->selectForWindow(win,~0);
	for (int c=0; c<win->_kids.n; c++) reselectForXEvents(win->_kids.e[c]);
	return;
}

//! Quick way to code setting window and win_on=0 for w and all subwindows,
/*! Called from destroywindow(), this has the effect of removing the windows
 * from the event queue. Actual X Window destroying is done in destroywindow(), which
 * calls this function just before it XDestroys the windows.
 *
 * This will set ANXWIN_DOOMED in w->win_style and all of w's children.
 */
void anXApp::resetkids(anXWindow *w)
{
	GetDefaultDisplayer()->ClearDrawable(w);

	w->xlib_backbuffer=0;
	w->xlib_window=0;
	w->win_on=0;
	w->win_style|=ANXWIN_DOOMED;

	for (int c=0; c<w->_kids.n; c++) resetkids(w->_kids.e[c]);
}

//! Tell the anXApp that w should be destroyed.
/*!  Meant to be called from within windows any old time.
 *  Means window did delete protocol, or just wants to be destroyed.
 *
 *  First w->close() is called, then
 *  anXWindow::xlib_window and its subwindows are XDestroyWindowed here and set to 0, and 
 *  any associated timers and tooltip checking removed, but
 *  the anXWindow instance is really destroyed at end of event loop in run(), or in app->close().
 *  Windows are ultimately responsible for deleting sub-anxwindows, via win->deletekid(), 
 *  which is usually called by app in destroyqueued(), so the user typically 
 *  doesn't have to do anything special. Also, if the stated window is a dialog, 
 *  then it is naturally removed from the dialog stack.
 */ 
int anXApp::destroywindow(anXWindow *w) 
{
	if (!w) return 1;
	if (w->win_style&ANXWIN_DOOMED) return 0;
	DBG cerr <<"== Destroywindow(\""<<w->whattype()<<" (count:"<<w->_count<<":"
	DBG    <<w->WindowTitle()<<"\")...topwindows.n="<<topwindows.n<<endl;

	 //-----close the window and destroy xlib bits of it
	w->close();
	Window xxx=w->xlib_window;
	resetkids(w); //dp->ClearDrawable() on w and all kids, removes from event check, sets ANXWIN_DOOMED
	if (xxx) {
		XDestroyWindow(dpy,xxx); // note: destroys all subwindows, so it's ok if 
								 //       xlib_window gets set to zero in resetkids()
	}


	 //reset currentfocus if is w or kid of w. *** fix for multi-keyboard!!
	 //also remove toolip consideration if necessary
	devicemanager->clearReceiver(w);

	 //remove any dialogs from the dialog stack.
	int c=dialogs.findindex(w);
	if (c>=0) dialogs.pop(c); // dialog is always a toplevel window

	 //remove all timers associated with w and its children
	for (int c=0; c<timers.n; c++) {
		if (dynamic_cast<anXWindow*>(timers.e[c]->win)==w || IsWindowChild(w,dynamic_cast<anXWindow*>(timers.e[c]->win)))
			{ timers.remove(c); c--; }
	}
	
	
	 // cannot delete w here, since this function is most likely called from within w, must stack to delete
	 // pop from topwindows stack if it is there.
	todelete.pushnodup(w); //incs count on w

	if (w->win_parent==NULL) {
		topwindows.remove(topwindows.findindex(w)); //dec's count
		outclickwatch.remove(outclickwatch.findindex(w)); //dec's count
	}
	
	 // to fight against the never ending scourge of delete race conditions,
	 // no element of todelete can be descended from any other element of todelete. If a parent
	 // is slated for destruction, all its _kids will be deleted in destroyqueued(), and
	 // we don't need to specifically remember it.
	 // w is the only new element on the stack, and is in the topmost position, so we need to
	 // remove any windows that are descended from it..
	for (int c=todelete.n-2; c>=0; c--) {
		if (IsWindowChild(w,todelete.e[c])>1) {
			todelete.remove(c); //dec's count of window at c
		}
	}

	return 0;
}

//! Return DefaultGC(dpy,screen).
GC anXApp::gc(int scr, int id) //0=default
{
	return DefaultGC(dpy,scr);
}

//! Called after all pending events have been processed, deletes all windows that have been anXApp::destroywindow()'d.
/*!  This may only be called from app::run(), outside of event handler
 *  else beware delete race conditions.
 *  Assumes only parents or topwindows in todelete, that is, for any window listed in todelete, there
 *  must not be a descendent of that window also listed in todelete. This should be so if they were 
 *  tagged for delete only from destroywindow(). 
 *  Also assumes window is already XDestroyWindow'd, which 
 *  should have happened in destroywindow().
 */
void anXApp::destroyqueued()
{
	DBG cerr <<"-DestoryQueued..."<<endl;
	anXWindow *w=NULL;
	while (todelete.n) {
		w=todelete.e[todelete.n-1];
		DBG cerr <<"  DestroyQueued: object "<<w->object_id<<", title:"<<w->WindowTitle()<<endl;
		
		if (w->win_parent!=NULL) w->win_parent->deletekid(w); //removes parent count
		todelete.remove(-1); //remove top item, dec's count
	}
	DBG cerr <<" -done destroyqueued..."<<endl;
}

//! Reparent kid to be a child of newparent.
/*! Attempts to reparent to a window that has ANXWIN_DOOMED in win_style will fail.
 *
 * To reparent as a top level window, the newparent should be NULL.
 *
 * If the parent is not created yet (parent->xlib_window==0), then only the anXWindow::win_parent
 * is adjusted. The window is not addwindow'd. If the calling code addwindows the child before addwindowing
 * the parent, then the child window is simply put in the parent window's _kids stack.
 * When the parent is addwindow'd, all the windows in _kids will be addwindow'd then. If the kid's
 * xlib_window is already created, but the parent is not, then the X window is XReparentWindow'd in addwindow.
 * 
 * If the parent->xlib_window exists, but the kid->xlib_window does not, then the kid is addwindow'd here.
 * 
 * The kid keeps its x,y,w,h after it gets reparented.
 * 
 * Return 1 on error, 0 on success.
 *
 * \todo maybe after reparenting, check to make sure window is on screen if it becomes top level
 * \todo *** not sure, but there might problem when newparent is a child of kid...
 * \todo implement reparenting to different screens
 *   
 */
int anXApp::reparent(anXWindow *kid,anXWindow *newparent)
{
	DBG cerr <<"reparent counts "<<(kid->win_name?kid->win_name:"")<<"->"<<(newparent->win_name?newparent->win_name:"")
	DBG   <<" before k, new: "<<kid->_count<<"  "<<newparent->_count<<endl;

	 //must not reparent to a doomed window
	if (!kid || (newparent && (newparent->win_style&ANXWIN_DOOMED))) return 1;

	 //if kid is already child of newparent, nothing to do
	if (kid->win_parent==newparent && newparent->_kids.findindex(kid)>=0) return 0;

	 // remove from previous _kids/topwindows stacks
	kid->inc_count();//so as to keep from deleting if only ref is from parent or topwindows
	if (kid->win_parent) { // remove from parent's _kids stack
		if (kid->win_parent!=newparent) 
			kid->win_parent->_kids.remove(kid->win_parent->_kids.findindex(kid));//decs count if present
	} else { // remove from toplevel stack
		topwindows.remove(topwindows.findindex(kid)); //decs count if present
		outclickwatch.remove(outclickwatch.findindex(kid)); //decs count if present
	}

	 // now kid->win_parent is undefined, so make it newparent
	kid->win_parent=newparent;

	 // add to new _kids or topwindows stacks
	if (newparent) newparent->_kids.pushnodup(kid); // shouldn't need no dup, but just in case funny business
		else topwindows.pushnodup(kid); // new parent is NULL
	kid->dec_count(); //remove extra count added above

	 // If kid is already created, but the new parent is not, then when the parent is addwindow'd, 
	 // the kid is XReparented there. In that case return now since we
	 // don't want to call X functions with bad window!
	if (newparent && !newparent->xlib_window) {
		DBG cerr <<"reparent counts "<<(kid->win_name?kid->win_name:"")<<"->"<<(newparent->win_name?newparent->win_name:"")
		DBG   <<" after k, new: "<<kid->_count<<"  "<<newparent->_count<<endl;
		return 0;
	}

	 // Otherwise, if the kid->window is defined, reparent.
	if (kid->xlib_window) 
		XReparentWindow(dpy,kid->xlib_window,
						newparent?newparent->xlib_window:DefaultRootWindow(dpy),
						kid->win_x,kid->win_y);
	else { // else the parent->xlib_window exists, and the kid has not been addwindow'd, so:
		addwindow(kid,1,0);
	}

	DBG cerr <<"reparent counts "<<(kid->win_name?kid->win_name:"")<<"->"<<(newparent->win_name?newparent->win_name:"")
	DBG   <<" after k, new: "<<kid->_count<<"  "<<newparent->_count<<endl;
	return 0;
	
//	XReparentWindow does:
//	 perform UnmapWindow request if mapped
//	 rehierarchizes it on top of new siblings
//	 ReparentNotify sent
//	 MapRequest sent if was originally mapped
	 
}

//! Unmaps a window and sends UnmapNotify events to it and its subwindows.
/*! Clients only wishing to unmap the top window can do so with XUnmapWindow.
 * This function merely calls that, then XUnmapSubwindows().
 */
int anXApp::unmapwindow(anXWindow *w) 
{
	if (!w || !w->xlib_window) return 1;
	XUnmapWindow(dpy,w->xlib_window);
	XUnmapSubwindows(dpy,w->xlib_window);
	return 0;
}

//! Maps a window and sends MapNotify events to it and its subwindows.
/*! Clients only wishing to map the top window can do so with XMapWindow.
 * This function merely calls XMapSubwindows(), then XMapWindow().
 */
 // Note that subwindows only are mapped with XMapSubWindows..
int anXApp::mapwindow(anXWindow *w) 
{
	if (!w || !w->xlib_window) return 1;
	XMapSubwindows(dpy,w->xlib_window);
	XMapWindow(dpy,w->xlib_window);
	return 0;
}

//! Check to see if some kinds of event types are in mask, and so allow them through.
/*! Note that only KeyPress/KeyRelease/ButtonPress/ButtonRelease/MotionNotify/Enter/Leave/FocusIn/Out
 * 	GraphicsExpose/Expose events are screened. All others pass right through.
 * 	
 * Returns nonzero for do NOT allow event (matched in mask), 0 for allow event
 */
int eventscreen(int e_type,unsigned long mask) 
{
	DBG cerr <<"+++ Screen event:"<<xlib_event_name(e_type)<<" mask"<<mask<<"  ";

	int c=1;
	switch (e_type) {
		case KeyPress : c= !(KeyPressMask&mask); break;
		case KeyRelease : c= !(KeyReleaseMask&mask); break;
		case ButtonPress : c= !(ButtonPressMask&mask); break;
		case ButtonRelease : c= !(ButtonReleaseMask&mask); break;
		case MotionNotify : c= !(PointerMotionMask&mask); break;
		//case EnterNotify : c= !(EnterWindowMask&mask); break;
		//case LeaveNotify : c= !(LeaveWindowMask&mask); break;
		case FocusIn :
		case FocusOut :	c= !(FocusChangeMask&mask); break;
		case GraphicsExpose : 
		case Expose : c= !(ExposureMask&mask); break;

		case KeymapNotify : 
		case NoExpose : 
		case VisibilityNotify : 
		case CreateNotify : 
		case DestroyNotify : 
		case UnmapNotify : 
		case MapNotify : 
		case MapRequest : 
		case ReparentNotify : 
		case ConfigureNotify : 
		case ConfigureRequest : 
		case GravityNotify : 
		case ResizeRequest : 
		case CirculateNotify : 
		case CirculateRequest : 
		case PropertyNotify : 
		case SelectionClear : 
		case SelectionRequest : 
		case SelectionNotify : 
		case ColormapNotify : 
		case ClientMessage : 
		case MappingNotify : 
		case LASTEvent : c=1;
	}
	DBG cerr <<c<<endl;
	return c;
}

/*! Return 0 for success, nonzero for not registered.
 */
int anXApp::RegisterEventReceiver(EventReceiver *ev)
{
	int s=0, e=eventreceivers.n-1, m;

	if (eventreceivers.n==0) { eventreceivers.push(ev,0); return 0; }

	 //push sorted by object id for easy binary search later
	if (ev->object_id<eventreceivers.e[0]->object_id) { eventreceivers.push(ev,0,0); return 0; }
	if (ev->object_id>eventreceivers.e[e]->object_id) { eventreceivers.push(ev,0,-1); return 0; }
	if (ev->object_id==eventreceivers.e[0]->object_id || ev->object_id==eventreceivers.e[e]->object_id)
		return 0; //already there!

	while (s!=e) {
		m=(s+e)/2;
		 //s and e have already been checked against
		if (m==s || m==e) { eventreceivers.push(ev,0,s+1); return 0; }

		if (ev->object_id==eventreceivers.e[m]->object_id) return 0; //already there!
		if (ev->object_id<eventreceivers.e[m]->object_id) { e=m; continue; }
		s=m;
	}

	return 0;
}

int anXApp::UnregisterEventReceiver(EventReceiver *e)
{
	removetimer(e,0);
	return eventreceivers.remove(eventreceivers.findindex(e));
}

//! Return the object with id in eventreceivers stack.
EventReceiver *anXApp::findEventObj(unsigned long id)
{
	int s=0, e=eventreceivers.n-1, m=0;
	if (e<0) return NULL;

	if (eventreceivers.e[s]->object_id==id) return eventreceivers.e[s];
	if (eventreceivers.e[e]->object_id==id) return eventreceivers.e[e];

	if (eventreceivers.e[s]->object_id>id) return NULL;
	if (eventreceivers.e[e]->object_id<id) return NULL;

	while (s<e) {
		m=(s+e)/2;
		if (eventreceivers.e[m]->object_id==id) return eventreceivers.e[m];

		if (eventreceivers.e[m]->object_id>id) e=m;
		else if (s==m) break;
		else s=m;
	}
	return NULL;
}

//! Force dealing with any pending messages.
/*! Sometimes messages sent via SendMessage() do not also make the application actually deal
 * with those messages.
 */
void anXApp::bump()
{
	if (!bump_xid || !dpy) return;

	XEvent e;
	e.xclient.type=ClientMessage;
	e.xclient.display=dpy;
	e.xclient.message_type=0;
	e.xclient.window=bump_xid;
	e.xclient.format=8;
	XLockDisplay(dpy);
	XSendEvent(dpy,bump_xid,False,0,&e);
	XUnlockDisplay(dpy);

// **** This below doesn't work, I don't know how to make a file descriptor that breaks select():
//	if (bump_fd<=0) {
//		char path[200];
//		sprintf(path,"/tmp/.lax-%d",getpid());
//		bump_fd=open(path,O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
//		//unlink(path);
//	}
//
//	 //bump_fd is added to the fds that select() monitors, thus this will break it out of just waiting around
//	DBG cerr <<"bumping..."<<endl;
//	write(bump_fd,"0",1);
//	lseek(bump_fd,0,SEEK_SET);
}

//! Send arbitrary data to a window.
/*! Applications can derive classes from EventData to easily pass arbitrary chunks of data 
 * between windows. This data will be deleted by anXApp eventually. So once SendMessage() is
 * called, the calling code should forget about the data.
 *
 * anXWindows receive these messages through anXWindow::Event(const EventData *data,const char *mes) 
 * If the window accepts it, then it must return 0. If nonzero is returned from the window,
 * then the message has not been dealt with, and it is propagated to its parent.
 *
 * If towindow, mes, or sendwindow are not 0, then those fields of data are set.
 * data->send_time is automatically set to the current time (milliseconds since boot).
 *
 * Data passed in here are added to a list of data to send. These are processed in
 * processdataevents(). If the target window does not exist at that time, the data is
 * deleted.
 *
 * Messages are recorded in a thread safe way with event_mutex.
 */
int anXApp::SendMessage(EventData *data, unsigned long toobj, const char *mes, unsigned long fromobj)//mes=0, sendwindow=0
{
	if (!data) return 1;

	if (mes) makestr(data->send_message,mes);
	if (fromobj) data->from=fromobj;
	if (toobj)   data->to  =toobj;
	data->send_time=times(&tmsstruct); //*** is the tmsstruct necessary? pass in NULL?
	

	pthread_mutex_lock(&event_mutex);

	if (dataevente) {
		dataevente->next=data;
		dataevente=data;
	} else {
		dataevents=dataevente=data;
	}

	pthread_mutex_unlock(&event_mutex);

	DBG cerr <<" ***** anXApp queued message: "<<(data->send_message?data->send_message:lax_event_name(data->type))<<endl;
	
	return 0;
}

//! Process the queue of EventData objects.
/*! Accumulated messages, whether from SendMessage() or through other means, are dispatched to the target windows.
 *
 * \todo if event propagation ever really becomes an issue, it should probably be fleshed out here!
 */
int anXApp::processdataevents()
{
	EventData *data;
	while (1) {
		pthread_mutex_lock(&event_mutex);
		if (!dataevents) {
			pthread_mutex_unlock(&event_mutex);
			break;
		}

		 //detach the oldest event data from pending events
		data=dataevents;
		dataevents=dataevents->next;
		if (!dataevents) dataevente=NULL;
		data->next=NULL;
		pthread_mutex_unlock(&event_mutex);
		
		processSingleDataEvent(NULL,data);
		delete data;
	}
	return 0;
}

/*! Return nonzero for event could not be sent, or 0 for event sent or otherwise processed.
 *  The event is not deleted here.
 *
 *  If obj==NULL, then use findEventObj(data->to).
 */
int anXApp::processSingleDataEvent(EventReceiver *obj,EventData *ee)
{
	if (!obj) obj=findEventObj(ee->to);
	if (!obj) return 1;

	//DBG cerr <<" ***** anXApp sending message: "<<(ee->send_message?ee->send_message:"(--)")
	//DBG      << " mesid="<<ee->type<<" to "<<obj->whattype()<<endl;


	if (!obj) obj=findEventObj(ee->to);
	if (!obj) return 1;
	anXWindow *ww=dynamic_cast<anXWindow*>(obj);

	if (ww && tooltips && ttcount==0) tooltipcheck(ee,ww);

	if (ee->type==LAX_onButtonDown && outclickwatch.n) {
		if (checkOutClicks(obj,dynamic_cast<MouseEventData*>(ee))!=0) {
			ee->type=LAX_DefunctEvent;
			return 0;
		}
	}

	 //focus events need additional processing for the correct processing of input,
	 //so these are not sent through window->Event(), but instead go directly
	 //to FocusOn() and FocusOff(). This is so only non-essential events are
	 //delivered to window->Event().
	if (ww && ee->type==LAX_onFocusOn)  {
		ww->FocusOn(dynamic_cast<const FocusChangeData*>(ee));

	} else if (ww && ee->type==LAX_onFocusOff) {
		ww->FocusOff(dynamic_cast<const FocusChangeData*>(ee));

	} else if (obj->Event(ee,ee->send_message?ee->send_message:"")!=0 && ee->propagate) {
		 //If event rejected, then possibly propagate to parent
		 // Only mouse and key events are so passed on.
		if (ww && ww->win_parent && ww->win_parent->xlib_window) { 
			int c=0;
			if (ee->type==LAX_onMouseMove || ee->type==LAX_onButtonDown || ee->type==LAX_onButtonUp) {
				 // Mouse event, musttranslate x,y to new window, ***assume win_x and win_y are accurate?
				 //****must make sure they are!! Check through all the configure notify stuff
				 //what about window manager decorations?
				MouseEventData *me=dynamic_cast<MouseEventData*>(ee);
				me->x+=ww->win_x+ww->win_border;
				me->y+=ww->win_y+ww->win_border;
				c=1;
			} else if (ee->type==LAX_onKeyDown || ee->type==LAX_onKeyUp) c=1;
			if (c) {
				anXWindow *w=ww->win_parent;
				while (w) {
					if (w->Event(ee,ee->send_message?ee->send_message:"")==0) break;
					w=w->win_parent;
				} 
				//if (!w) eventCatchAll(ee);
			}
		}
	}

	return 0;
}

//! See if a mouse down event is down outside of any top windows in outclickwatch or any controls connected to them.
/*! destroywindow() if the mouse is down outside of any of them.
 *
 * Return 0 for was not a relevant out click, or 1 for it was, and windows have been removed.
 *
 * \todo If there is any other mouse in associated controls, should keep that group up.
 */
int anXApp::checkOutClicks(EventReceiver *obj,MouseEventData *ee)
{
	if (!ee) return 1;
	int n=0;
	int outclicked=0;
	anXWindow *w, *s, *e, *p;
	
	int x,y; //root coordinates
	int xx,yy; //window coordinates
	translate_window_coordinates(ee->target,ee->x,ee->y, NULL,&x,&y, NULL);

	for (int c=0; c<outclickwatch.n; c++) {
		w=outclickwatch.e[c];
		DBG cerr <<"Checking out click for "<<w->WindowTitle()<<endl;
		n=0; //0 if mouse in no windows connected to w

		translate_window_coordinates(NULL,x,y, w,&xx,&yy, NULL);
		if (xx<0 || yy<0 || xx>w->win_w || yy>w->win_h) {
			 //mouse is outside of a watched window, must make sure it is not in any of that
			 //window's connected top level controls.
			s=w; e=w;
			while (s->prevcontrol && s->prevcontrol!=w) s=s->prevcontrol;
			while (e->nextcontrol && e->nextcontrol!=w) e=e->nextcontrol;
			p=s;
			do {
				translate_window_coordinates(NULL,x,y, p,&xx,&yy, NULL); //root -> p coords
				if (xx>=0 && yy>=0 && xx<p->win_w && yy<p->win_h) {
					n++; //mouse was inside one of the controls!
					break;
				}
				p=p->nextcontrol;
			} while (p && p!=s);

			if (!n) {
				 //mouse was not in any connected windows so must destroy all in s..e
				outclicked=1;
				p=s;
				do {
					outclickwatch.remove(c);
					destroywindow(p); //remember, this removes from outclickwatch
					p=p->nextcontrol;
				} while (p && p!=s);
				c=-1; //reset counter, since outclickwatch stack has been fiddled with
			}
		}
	}

	return outclicked;
}

//! Run a (top-level) dialog window that blocks inputs to other windows (runs a so called modal dialog)
/*! A dialog is a top level window that eventually
 * that discards messages that are masked in dialog_mask that belong to other windows, or
 * windows that are not descended from dialog. Often times a dialog will bring up
 * another dialog like a message box or another popup menu, and 
 * anXApp maintains a stack of such dialogs. If the dialog already is on the stack, then move it to the top. 
 *
 * Usually either the owner of the dialog or the dialog itself is responsible for
 * extracting and/or delivering any state from the dialog and
 * destroywindow'ing the dialog. After that point, event processing returns to normal.
 *
 * rundialog() addwindow's the dialog if it is a toplevel, sets up the event handler
 * to toss out those other messages, and returns 1. Please note that the event screener
 * potentially could upset windows that keep track
 * of button down/up; for instance when a dialog is popped up when the button is
 * pressed down, rather than on a button up. Any generated
 * events are screened from this point until destroywindow() is called on the dialog. 
 *
 * Sometimes multiple dialogs should be receiving all events. In that case, wingroup
 * should point to a window that is already on the dialogs stack.
 * Events are routed only through the top of the stack, and to windows on the stack that
 * are of the same group. 
 *
 * If the dialog is not toplevel (ie win_parent!=NULL), nothing is added and 0 is returned. 
 *
 * The dialog is added to the application via <tt>addwindow(ndialog,1,wislocal)</tt>.
 * 
 * Returns 0 for success, or nonzero for error.
 */
int anXApp::rundialog(anXWindow *ndialog,anXWindow *wingroup,char absorb_count)//wingroup=NULL, absorb_count=1
{
	if (ndialog->win_parent || addwindow(ndialog,1,absorb_count)) return 1;

	 //find a unique number for the window group
	char dislocal=-2; //the islocal stack in dialogs stack is really a window group id
					  //created automatically. it'll be -2 or less.
	int c;
	for (c=0; c<dialogs.n; c++) {
		if (dialogs.e[c]==wingroup) { dislocal=dialogs.islocal[c]; break; } //found wingroup
		 //make dislocal 1 less than the lowest wingroup
		if (dialogs.islocal[c]<dislocal) dislocal=dialogs.islocal[c]-1;
	}
	c=dialogs.pushnodup(ndialog,dislocal);
	//if (absorb_count) ndialog->dec_count();//***uncomment if dialogs becomes reference counted!!
	if (c>0) { // was on the stack already and/or was not at front of stack, so move it to the front
		dialogs.pop(c);
		dialogs.push(ndialog,dislocal,0);
	}
	return 0;
}

//! Add a window, optionally map it, optionally decrement its count.
/*! In the course of adding the window, the following steps are taken:
 * 
 * If w==NULL or w->xlib_window already exists (just checks for nonzero value) then do nothing and return 1.
 * 
 * If the window is a child of a window that has not been addwindow'd, that is, parent->xlib_window==0, 
 * then the the window is not created here. It is merely inserted into the parent->_kids stack.
 * No preinit(), etc are called now, they are called when the child->xlib_window is actually created,
 * which will be when addwindow() is called on the parent.
 * When the parent is addwindow'd, then the kids are all also addwindow'd using the same mapit value.
 * Sometimes a child is already XCreated, but the parent is not. In that case, the child is XReparenteWindow'd
 * to the newly created parent here.
 * 
 * First, w->preinit() is called.
 * 
 * Second, XCreateWindow is called. If win_w or win_h are zero, the window is created with
 * 1 instead of 0, but win_w and win_h still are set to 0. This is because calling with 0
 * causes problems with Xlib.
 *
 * Any reparenting that has to be done on any child windows listed in the w->_kids stack
 * is done at this point, before w->init() is called. If the kid->xlib_window doesn't exist, addwindow(kid) is called.
 * Only the immediate children are addwindow'd here. If all the kids are already addwindow'd,
 * but the kid has non-addwindow'd kids, those are not addwindow'd. Hopefully this is ok.....
 * 
 * Third, w->init() is called, which normally is where a window's children are addwindow'd
 *
 * Fourth, XMapWindow is called on the window, if mapit!=0.
 * 
 * Returns 0 on success, nonzero on error
 */
int anXApp::addwindow(anXWindow *w,char mapit,char absorb_count) // mapit==1, absorb_count==1
{
	if (w==NULL || w->xlib_window) return 1; // do not create if it is already created.
	if (w->app!=this) {
		DBG cerr << "win app!=app.\n";
		w->app=this;
	}
	
	 //----- add window to app's internal stack:
	if (w->win_parent==NULL) {
		topwindows.push(w);
	} else {
		w->win_parent->_kids.pushnodup(w); //increased count on window
		if (w->win_parent->xlib_window==0) {
			if (absorb_count) w->dec_count();
			return 0; // do not create if the parent window is 0!! otherwise crashes
		}
	}

	if (absorb_count) w->dec_count();

	 //add to watch list. clicking anywhere outside this window will result in the window being destroyed
	if (w->win_style&ANXWIN_OUT_CLICK_DESTROYS) {
		outclickwatch.push(w);
	}

	 //set default background color
	WindowColors *wc=w->win_colors;
	if (!wc) wc=color_panel;
	if (wc) {
		w->xlib_win_xatts.background_pixel=wc->bg; 
		w->xlib_win_xattsmask|=CWBackPixel;
	}

	if (w->win_pointer_shape) {
		 //***need fuller implementation of mouse cursors!!
		Cursor cursor=0;
		if (w->win_pointer_shape<200) {
			 //use x cursor font
			cursor=XCreateFontCursor(dpy,w->win_pointer_shape);
		}
		if (cursor) {
			w->xlib_win_xatts.cursor=cursor;
			w->xlib_win_xattsmask|=CWCursor;
		}
	}

	 //for no-decorations style, *** this also stays on every desktop!!
	if (w->win_style&ANXWIN_BARE) {
		w->xlib_win_xatts.override_redirect=True;
		w->xlib_win_xattsmask|=CWOverrideRedirect;
	}
	
	 // preinit exists because I haven't figured out why XResizeWindow doesn't seem to have any effect
	 // on the window if called after XCreateWindow but before mapping. Speaking of which, there is 
	 // another strange lack of resizing when the window is otherwise unmapped..
	w->preinit(); 
	
	 //---------center on screen or in parent-------------
	DBG fprintf(stderr,"addwindow: style: %lx\n",w->win_style);

	XSizeHints *sizehints=w->xlib_win_sizehints;
	if (w->win_style&ANXWIN_CENTER) {
		DBG cerr << "addwindow: Centering "<<w->WindowTitle()<<endl;
		if (!w->win_parent) {
			if (!sizehints) sizehints=XAllocSizeHints();
			Screen *scr=DefaultScreenOfDisplay(dpy);
			w->win_x=(scr->width-w->win_w)/2;
			w->win_y=(scr->height-w->win_h)/2;
			if (sizehints) {
				DBG cerr <<"doingwin_sizehintsfor"<<w->WindowTitle()<<endl;
				// The initial x and y become the upper left corner of the window
				//manager decorations. how to figure out how much room those decorations take,
				//so as to place things on the screen accurately? like full screen view?
				sizehints->x=w->win_x;
				sizehints->y=w->win_y;
				sizehints->width=w->win_w;
				sizehints->height=w->win_h;
				sizehints->flags|=USPosition|USSize;
			}
		} else {
			w->win_x=(w->win_parent->win_w-w->win_w)/2;
			w->win_y=(w->win_parent->win_h-w->win_h)/2;
		}
	} else if (!w->win_parent && w->win_h>1 && w->win_w>1) {
		 //attempt to position on screen when no parent...
		if (!sizehints) sizehints=XAllocSizeHints();
		Screen *scr=DefaultScreenOfDisplay(dpy);

		if (w->win_x>scr->width) w->win_x=scr->width-10;
		else if (w->win_x+w->win_w<0) w->win_x=10-w->win_w;
		if (w->win_y>scr->height) w->win_y=scr->height-10;
		else if (w->win_y+w->win_h<0) w->win_y=10-w->win_h;

		sizehints->x=w->win_x;
		sizehints->y=w->win_y;
		sizehints->width=w->win_w;
		sizehints->height=w->win_h;
		sizehints->flags|=USPosition|USSize;
	}
	
	DBG cerr << "addwindow::create:"<<w->WindowTitle()<<"  x,y:"<<w->win_x<<','<<w->win_y<<"  w,h:"<<w->win_w<<','<<w->win_h<<endl;
	Window win=XCreateWindow(dpy,
					(w->win_parent?w->win_parent->xlib_window:DefaultRootWindow(dpy)),
					w->win_x,w->win_y, (w->win_w?w->win_w:1),(w->win_h?w->win_h:1),
					w->win_border,
					CopyFromParent, //depth
					CopyFromParent, //class, from: InputOutput, InputOnly, CopyFromParent
					vis,
					w->xlib_win_xattsmask,&w->xlib_win_xatts);
	if (!win) {
		DBG cerr <<" ----win==0 for "<<w->WindowTitle()<<", aborting add, unable to XCreateWindow"<<endl;
		w->xlib_window=0;
		return 1;
	}
	w->xlib_window=win;
	DBG cerr <<"addwindow  window XCreated: \""<<w->WindowTitle()<<"\" = "<<w->xlib_window<<endl;

	if (w->win_title) {
		XStoreName(dpy, w->xlib_window, w->win_title);
		//not sure if window manager always changes name on its own in _NET_WM_NAME and _NET_WM_VISIBLE_NAME
		//it seems to on my machine..
	}

	 //automatically select for key, mouse, and exposure events
	if (!(w->win_style&ANXWIN_NO_INPUT)) {
		 //select for device input
		if (devicemanager) devicemanager->selectForWindow(w,KeyPressMask|
															KeyReleaseMask|
															PointerMotionMask|
															ButtonPressMask|
															ButtonReleaseMask);
		else w->xlib_win_xatts.event_mask|=KeyPressMask| KeyReleaseMask| PointerMotionMask| ButtonPressMask| ButtonReleaseMask;
	}

	if (w->win_style&ANXWIN_FULLSCREEN) {
		Atom prop_fs = XInternAtom(app->dpy, "_NET_WM_STATE_FULLSCREEN", False);
		Atom prop_state = XInternAtom(app->dpy, "_NET_WM_STATE", False);
		XChangeProperty(app->dpy, w->xlib_window, prop_state, XA_ATOM, 32, PropModeReplace, (unsigned char *)&prop_fs, 1);
	}
	
	 // Check for proper parenting..
	 // If kid->window exists, then w->xlib_window must be made its parent, win is the newly created X window of w.
	if (w->_kids.n>0) {
		for (int c=0; c<w->_kids.n; c++) {
			if (w->_kids.e[c]->xlib_window) {
				DBG cerr <<"---Correcting parent of kid number "<<c<<" of "<<w->WindowTitle()<<endl;
				XReparentWindow(dpy,w->_kids.e[c]->xlib_window,w->xlib_window,w->_kids.e[c]->win_x,w->_kids.e[c]->win_y);
			} else {
				DBG cerr <<"---Adding kid number "<<c<<" of "<<w->WindowTitle()<<endl;
				addwindow(w->_kids.e[c],mapit,0);
			}
		}
	} else {
		DBG cerr <<"---No kids of "<<w->WindowTitle()<<" to add."<<endl;
	}
	
	 // set XdndAware on toplevel windows
	if (!w->win_parent && w->win_style&ANXWIN_XDND_AWARE) {
		Atom XdndAware=XInternAtom(app->dpy,"XdndAware",False);
		int version=4;
		XChangeProperty(dpy,w->xlib_window,XdndAware,XA_ATOM,
						sizeof(int)*8, //number of bits element in property
						PropModeReplace,
						(unsigned char *)&version,
						1); //num elements
	}


	if (!w->win_colors) w->installColors(color_panel); //just in case

	int c=w->init(); // window must set win_hints, win_sizehints here, if wanted
	if (c!=0) { //window size has been changed... *** please note this doesn't work as expected!! 
				//xlib seems to ignore size changes between xcreate and mapping
		if (!sizehints) sizehints=XAllocSizeHints();
		 // put win_w,etc in w->win_sizehints, and assume they are different then
		 // before init() was called.
		sizehints->width=w->win_w;
		sizehints->height=w->win_h;
		sizehints->flags|=USSize;
		if (w->win_style&ANXWIN_CENTER) {
			if (!w->win_parent) {
				Screen *scr=DefaultScreenOfDisplay(dpy);
				sizehints->x=w->win_x=(scr->width-w->win_w)/2;
				sizehints->y=w->win_y=(scr->height-w->win_h)/2;
			} else {
				sizehints->x=w->win_x=(w->win_parent->win_w-w->win_w)/2;
				sizehints->y=w->win_y=(w->win_parent->win_h-w->win_h)/2;
			}
			sizehints->flags|=USPosition;
		}
	}

	if (w->win_style&ANXWIN_DOUBLEBUFFER) w->SetupBackBuffer();

	 // ***urgency hint in XWMHints? demanding transient?
	 

	 // --------- Setup Window Properties ----------------	 

	 // WM_PROTOCOLS: WM_DELETE_WINDOW, WM_TAKE_FOCUS, _NET_WM_PING
	// ??? check style for non-top levels trying to be big dog?? does it matter? assuming not
	if (w->win_parent!=NULL && !(w->win_style&ANXWIN_NOT_DELETEABLE)) w->win_style|=ANXWIN_NOT_DELETEABLE;
	int np=0;
	Atom newprots[3];
	newprots[np++]=XInternAtom(dpy,"_NET_WM_PING",False);
	if (!(w->win_style&ANXWIN_NOT_DELETEABLE)) 
		newprots[np++]=XInternAtom(dpy,"WM_DELETE_WINDOW",False);
	//if (w->win_style&(ANXWIN_LOCAL_ACTIVE | ANXWIN_GLOBAL_ACTIVE))
	if (!(w->win_style&ANXWIN_NO_INPUT))
		newprots[np++]=XInternAtom(dpy,"WM_TAKE_FOCUS",False);
	if (np) XSetWMProtocols(dpy,w->xlib_window,newprots,np);
	 
	 // set WM_TRANSIENT_FOR, sets for owner
	if ((w->win_style&ANXWIN_TRANSIENT) && w->win_owner) {
		//XSetTransientForHint(dpy,w->xlib_window,DefaultRootWindow(dpy));
		anXWindow *owner=dynamic_cast<anXWindow *>(findEventObj(w->win_owner));
		if (owner && owner->xlib_window) 
			XSetTransientForHint(dpy,w->xlib_window,owner->xlib_window);
	}
	
	 // set WM_HINTS, hints has Input field for WM_TAKE_FOCUS
	 // urgency hint would be set here
	 // I'm going to assume all windows want to be local active, which in xlib means that
	 // a client will set the focus only when it has it already, and not grab it from windows belonging
	 // to other connections.
	XWMHints *xh=w->xlib_win_hints;
	//if (w->win_style&(ANXWIN_PASSIVE| ANXWIN_LOCAL_ACTIVE)) {
	if (!(w->win_style&ANXWIN_NO_INPUT)) { //we want input and maybe will set focus
		if (!xh) xh=XAllocWMHints(); 
		xh->input=True;
		xh->flags|=InputHint;
	//} else if (w->win_style&(ANXWIN_NO_INPUT| ANXWIN_GLOBAL_ACTIVE)) {
	} else {
		if (!xh) xh=XAllocWMHints(); 
		xh->input=False;
		xh->flags|=InputHint;
	}

	 //set window icon
	if (!w->win_parent && (default_icon || default_icon_file)) {
		if (!default_icon && load_image!=NULL) {
			default_icon=load_image(default_icon_file);
			if (!default_icon) {
				cerr <<" WARNING! could not load default icon: "<<default_icon_file<<endl;
			}
		}

		if (default_icon) {
			 //set new school icon hint
			// *** it is not clear the below works (or even should work):
//			unsigned char *data=default_icon->getImageBuffer();
//			int width =default_icon->w();
//			int height=default_icon->h();
//			unsigned char ndata[4*(2+width*height)];
//			memcpy(ndata+8,data,4*width*height);
//			ndata[0]=0;
//			ndata[1]=0;
//			ndata[2]=(width&0xff00)>>16;
//			ndata[3]=(width&0xff);
//			ndata[4]=0;
//			ndata[5]=0;
//			ndata[6]=(height&0xff00)>>16;
//			ndata[7]=(height&0xff);
//
//			Atom _net_wm_icon=XInternAtom(dpy,"_NET_WM_ICON",False);
//			XChangeProperty(dpy,
//							w->xlib_window,
//							_net_wm_icon,
//							XA_CARDINAL, //???
//							8,
//							PropModeReplace,
//							ndata, 4*(2+width*height));
//
//			default_icon->doneWithBuffer(data);
		}

		 //set old school icon hint if Imlib is handy to retrieve X pixmap/clipmask
#ifdef LAX_USES_IMLIB
		if (!xh || (xh->flags&IconPixmapHint)==0) {
			 //***this is clunky!!

			Pixmap default_icon_data=0;
			Pixmap default_icon_mask=0;

			XSetWindowAttributes xatts;
			Window win=XCreateWindow(dpy,
							DefaultRootWindow(dpy),
							0,0,1,1,
							0,
							CopyFromParent, //depth
							CopyFromParent, //class, from: InputOutput, InputOnly, CopyFromParent
							vis,
							0,&xatts);
			imlib_context_set_drawable(win);
			Imlib_Image imlibimage=imlib_load_image(default_icon_file);
			if (imlibimage) {
				imlib_context_set_image(imlibimage);
				imlib_render_pixmaps_for_whole_image(&default_icon_data, &default_icon_mask);
				imlib_free_image();
			} else {
				cerr <<" WARNING! could not load default icon for xh: "<<default_icon_file<<endl;
			}
			XDestroyWindow(dpy,win);

			if (!xh) xh=XAllocWMHints();
			if (default_icon_data) {
				xh->flags|=IconPixmapHint|IconMaskHint;
				xh->icon_pixmap=default_icon_data;
				xh->icon_mask  =default_icon_mask;
			}
		}
#endif
	}

	if (xh) XSetWMHints(dpy,w->xlib_window,xh);
	if (!w->xlib_win_hints && xh) XFree(xh);
	 
	 // set WM_NORMAL_HINTS and free sizehints if necessary
	if (sizehints) {
		XSetWMNormalHints(dpy,w->xlib_window,sizehints);
		if (!w->xlib_win_sizehints) XFree(sizehints);
	}
	
	 // ------ map the window
	if (mapit) {
		XMapWindow(dpy,w->xlib_window);
		w->win_on=1;
	}



	DBG cerr <<"Done app->addwindowing "<<w->WindowTitle()<<"\n";
	return 0;
}

//! Find the anXWindow having the given object_id.
anXWindow *anXApp::findwindow_by_id(unsigned long id)
{
	if (id==0) return NULL;
	anXWindow *ww=NULL;
	for (int c=0; c<topwindows.n; c++) {
		if (id==topwindows.e[c]->object_id) return topwindows.e[c];
		ww=find_subwindow_by_id(topwindows.e[c],id);
		if (ww) return ww;
	}
	return NULL;
}

//! Find the anXWindow having win, and ancestor w
/*! called from a loop over topwindows in run. 
 *
 * Perhaps this should be a window function?
 */
anXWindow *anXApp::find_subwindow_by_id(anXWindow *w,unsigned long id)
{
	if (w->object_id==id) return w;
	anXWindow *ww=NULL;
	for (int c=0; c<w->_kids.n; c++) {
		ww=find_subwindow_by_id(w->_kids.e[c],id);
		if (ww) return ww;
	}
	return NULL;
}

//! Find the anXWindow having win, and ancestor w
/*! called from a loop over topwindows in run. 
 *
 * Perhaps this should be a window function?
 */
anXWindow *anXApp::findsubwindow_xlib(anXWindow *w,Window win)
{
	if (!w) return NULL;
	if (w->xlib_window==win) return w;
	anXWindow *ww=NULL;
	for (int c=0; c<w->_kids.n; c++) {
		ww=findsubwindow_xlib(w->_kids.e[c],win);
		if (ww) return ww;
	}
	return NULL;
}

//! Find the anXWindow associated with Window win.
/*! This can be used by anyone to search all the windows that
 *  anXApp knows about, which are the top level windows, and all 
 *  child windows in the anXWindow::_kids stacks.
 */
anXWindow *anXApp::findwindow_xlib(Window win)
{
	if (win==0) return NULL;
	anXWindow *ww=NULL;
	for (int c=0; c<topwindows.n; c++) {
		if (win==topwindows.e[c]->xlib_window) return topwindows.e[c];
		ww=findsubwindow_xlib(topwindows.e[c],win);
		if (ww) return ww;
	}
	return NULL;
}

//! Examine timers and set time to wait in preparation for a call to select().
/*! If a timer is due, then that window's anXWindow::Idle() function is called from here.
 *
 * This is called from anXApp::run().
 */
void anXApp::settimeout(struct timeval *timeout)
{
	//DBG cerr <<" --- settimeout"<<endl;

	//timeout->tv_sec=0;
	//timeout->tv_usec=100000;
	//return;

	if (maxtimeout>0) {
		timeout->tv_sec =maxtimeout/1000000;
		timeout->tv_usec=maxtimeout%1000000;
	} else {
		timeout->tv_sec=2000000000;
		timeout->tv_usec=0;
	}
	if (timers.n==0 && !tooltipmaybe.n) return;

	clock_t currenttime; 
	clock_t earliest=0;
	currenttime=times(&tmsstruct); // get current time

	for (int c=0; c<timers.n; c++) {
		if (timers.e[c]->checktime(currenttime)<0) { 
			DBG cerr <<"removing timer "<<c<<", id: "<<timers.e[c]->id<<endl;
			timers.remove(c--); continue; 
		}
		if (c==0) earliest=timers.e[0]->nexttime;
		if (timers.e[c]->nexttime<earliest) earliest=timers.e[c]->nexttime;
	}


	 // tooltip timer, update earliest against possibly many tooltips to pop up...
	if (tooltips && ttcount==0 && tooltipmaybe.n) {
		LaxMouse *tt;
		for (int c=0; c<tooltipmaybe.n; c++) {
			tt=dynamic_cast<LaxMouse*>(tooltipmaybe.e[c]); //tooltipmaybe should ONLY have mice

			if (currenttime>tt->ttendlimit) { // is time, so pop up
				tt->ttendlimit=0;
				newToolTip(tt->ttwindow->tooltip(tt->id),tt->id);
				tt->ttwindow->dec_count(); tt->ttwindow=NULL;
				tooltipmaybe.pop(c);
				c--;

			} else { // if not time, just check against earliest
				if (earliest==0 || tt->ttendlimit<earliest) earliest=tt->ttendlimit;
			}
		}

	} else {
		//DBG cerr <<"not checking tooltips in settimeout"<<endl;
	}


	//DBG cerr <<"earliest time:"<<earliest<<"  currenttime:"<<currenttime<<endl;
	if (earliest) {
		 //convert earliest, which at the moment is clock ticks, to microseconds
		earliest-=currenttime;
		earliest=earliest*1000000/sysconf(_SC_CLK_TCK);
	}
	if (maxtimeout>0 && earliest>maxtimeout) earliest=maxtimeout;
	if (earliest==0) earliest=2000000000;

	//DBG cerr <<"earliest secs:"<<earliest<<endl;

	timeout->tv_sec=earliest/1000000;
	timeout->tv_usec=earliest%1000000;
}

//! Create and add a new tooltip, ensuring there is only one per mouse up at any one time.
void anXApp::newToolTip(const char *text,int mouseid)
{
	ToolTip *tt;
	for (int c=0; c<topwindows.n; c++) {
		tt=dynamic_cast<ToolTip*>(topwindows.e[c]);
		if (!tt) continue;
		if (tt->mouse_id==mouseid) destroywindow(tt);
	}
	anXWindow *ttmaybe=new ToolTip(text,mouseid);
	addwindow(ttmaybe);
}

//! This is the main event loop.
/*! If dpy==NULL, then this will immediately return 1.
 *
 * Return 0 for successful run. Return nonzero for unsuccessful, such as when dpy==NULL.
 *
 * \todo must implement other file descriptor input for the call to select. This will allow listening for 
 *   unusual input sources like wiimotes or socket communication..
 */
int anXApp::run()
{
	if (!dpy) return 1;

	DBG cerr <<"Entering run()....."<<endl;

	XEvent event;
	//clock_t tm;
	int c;
	int anytodraw;

	  
	 //--- event loop
	struct timeval timeout;
	fd_set fdset[3];
	int xlibfd=ConnectionNumber(dpy);
	int maxfd=xlibfd;

	while (dontstop) {
		anytodraw=1;
		
		 //--- Dispatch any EventData events
		//DBG cerr <<"-dispatch data events"<<endl;
		if (dataevents) processdataevents();
		
		 //--- do any idling
		 // idle is recursive loop on the specified window
		//DBG cerr <<"-idling"<<endl;
		// *** this is do nothing as idle is controlled by timers now...
		//for (c=0; c<topwindows.n; c++) idle(topwindows.e[c]);

		 // Do X events
		//DBG cerr <<"-x events"<<endl;
		//while (XEventsQueued(dpy,QueuedAfterFlush)) {
		while (XPending(dpy)) {
			//DBG cerr <<"--events pending: "<<XPending(dpy)<<endl;
			XNextEvent(dpy,&event);
			//DBG cerr <<"----xevent: "<<xlib_event_name(event.type)<<endl;
			processXevent(&event);
		}
		
		//DBG cerr <<"-destroy queued"<<endl;
		 //--- destroy any requested destruction (before idling and refreshing)
		if (todelete.howmany()) destroyqueued();

		 //set the timeout limit due to any timers here, before refresh. Sometimes
		 //windows want to redraw in response to a timer event..
		 //Note that this will call any window->Idle() that a timer says to
		settimeout(&timeout);

		 //--- do any refreshing as last step in run loop...*** this can
		 //be a little more intelligent.. for refresh to occur, there should be nothing
		 //immediately pending, and all events processed?
		 // refresh is recursive loops on the specified window
		if (anytodraw) {
			anytodraw=0;
			for (c=0; c<topwindows.n; c++) anytodraw+=refresh(topwindows.e[c]);
			//DBG cerr <<"anytodraw:"<<anytodraw<<endl;
		}
		XSync(dpy,False);
		//DBG cerr <<"-just refreshed "<<anytodraw<<endl;
		

		if (dontstop==0 || topwindows.n==0) { dontstop=0; break; }

		 //---- Wait for events or timers
		 // man select and select_tut for what this does..
		 // Currently, just monitors the X channel, later add support for arbitrary fds?
		 // like an object event pipe?
		 // 
		 //It is necessary to check for pending here, because anything above might have triggered
		 //a send event, for instance, and the event will already be pending, but it will not cause
		 //the X file descriptor to change state..
		if (!dataevents && !XPending(dpy) && !anytodraw && !todelete.howmany()) { 
			FD_ZERO(&fdset[0]);
			FD_ZERO(&fdset[1]);
			FD_ZERO(&fdset[2]);
			FD_SET(xlibfd,&fdset[0]);
//			if (bump_fd>0) {
//				FD_SET(bump_fd,&fdset[0]);
//				if (bump_fd>maxfd) maxfd=bump_fd;
//			}
			//if (devicemanager && devicemanager->hasFD()) devicemanager->Setfd(&fdset[0],&maxfd);

			//DBG timeout.tv_sec=1;
			//DBG cerr <<"entering select...."<<endl;
			//settimeout(&timeout);//****this is done above now
			//int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
			c=select(maxfd+1, &fdset[0], &fdset[1], &fdset[2], &timeout);

//			if (FD_ISSET(bump_fd,&fdset[0])) {
//				 //clear the bump file 
//				DBG cerr <<"*** clearing bump"<<endl;
//				//char buffer[100];
//				//while (read(bump_fd,buffer,100)>0) ;
//				ftruncate(bump_fd,0);
//			}

			if (c<0) perror("select returned error: ");
		}
		
		//if (c==0) processTimers(); //c==0 means time expired, rather than event ready
		//--- timer handling is done in settimeout()
	}
	return 0;
}

//void anXApp::processTimers()
//{}


//! Does tooltip related checking on events.
/*! Currently called from run() via processSingleDataEvent() while processing events.
 * Checks for enter events and initializes
 * whether to potentially popup a tooltip soon and turns off check
 * when leave event or on events from related devices.
 *
 * In settimeout(), there is a check for whether it is time to pop up a tip, and a new
 * ToolTip window is created if necessary.
 * 
 * <pre>
 * |- time entered a window
 * |-------------------anXApp::tooltips--------------------------|
 * |--a little initial movement allowed--|-----non-move time-----| <- end of allowed time
 *                         ttthreshhold--|
 * </pre>
 *
 * See also settimeout().
 */
void anXApp::tooltipcheck(EventData *event, anXWindow *ww)
{

	if (event->type==LAX_onMouseIn) {
		 //possibly initialize a check for a tooltip when entering a window

		//DBG cerr <<"anXApp::tooltipcheck enter.."<<endl;
		EnterExitData *ee=dynamic_cast<EnterExitData*>(event);
		LaxMouse *m=dynamic_cast<LaxMouse *>(ee->device);
		int c=tooltipmaybe.findindex(m); //if c>=0 then there was already a window under consideration

		 // Initialize checking for whether to show a tooltip for this window..
		if (tooltips && ttcount==0 && ww->tooltip()) {
			m->ttendlimit=  times(&tmsstruct) + tooltips*  sysconf(_SC_CLK_TCK)/1000;
			m->ttthreshhold=times(&tmsstruct) + tooltips/2*sysconf(_SC_CLK_TCK)/1000;
			if (m->ttwindow) {
				DBG cerr <<"anxapp tooltip check dec_count on "<<m->ttwindow->object_id<<endl;
				m->ttwindow->dec_count();
			}
			ww->inc_count();
			m->ttwindow=ww;
			if (c<0) tooltipmaybe.push(m,0);

		} else if (c>=0) tooltipmaybe.pop(c); //window has no tooltip, so remove check!

		//DBG cerr <<"anXApp::tooltipcheck enter (end).."<<endl;
		return;
	}

	if (event->type==LAX_onMouseOut) {
		 //stop considering to display a tooltip for window if we leave a window

		EnterExitData *ee=dynamic_cast<EnterExitData*>(event);
		int c=tooltipmaybe.findindex(ee->device);
		LaxMouse *m=dynamic_cast<LaxMouse *>(ee->device);

		 //clear any tip up already
		for (int c=0; c<topwindows.n; c++) {
			ToolTip *tt=dynamic_cast<ToolTip*>(topwindows.e[c]);
			if (!tt) continue;
			if (tt->mouse_id==m->id) destroywindow(tt);
		}

		if (c<0) return; //the mouse out device does not correspond to any current tooltip check

		tooltipmaybe.pop(c);
		if (m->ttwindow) m->ttwindow->dec_count();
		m->ttwindow=NULL;
		m->ttendlimit=0;
		m->ttthreshhold=0;
		return;
	}

	if (!tooltipmaybe.n) return; //no tooltip under consideration
	if (event->type!=LAX_onMouseMove && event->type!=LAX_onButtonDown && event->type!=LAX_onButtonUp)
		return; //below, only care about motion and button events

	 //so there has been a mouse event, and we need to see if it disqualifies the tooltip

	MouseEventData *ee=dynamic_cast<MouseEventData*>(event);
	LaxMouse *m=ee->device;
	for (int c=0; c<tooltipmaybe.n; c++) {
		if (tooltipmaybe.e[c]==m) {
			clock_t curtime=times(&tmsstruct);
			if (curtime<m->ttthreshhold) {
				 //event is within threshhold time, so do nothing
				return;
			}
			m->ttthreshhold=0;
			m->ttendlimit=0;
			tooltipmaybe.pop(c);
			return;
		}
	}
}


//! Try to set the keyboard focus to win. If t==0 then use the current time.
/*! This assumes that win is available to actually accept the focus.
 * 
 * Return 0 for success. -1 for win is not able to accept focus.
 * 1 for window is IsUnviewable or IsUnmapped, and so cannot set focus to it (trying to do
 * so will crash the program). Must be IsViewable as returned in map_state in 
 * the Xlib function XGetWindowAttributes().
 * 
 * If devicemanager!=NULL, then actually set the focus with DeviceManager::SetFocus().
 * Otherwise, fall back on Xlib call XSetInputFocus(), which will set a generic core pointer focus.
 */
int anXApp::setfocus(anXWindow *win, clock_t t, const LaxKeyboard *kb)
{
	if (!win || !win->xlib_window) return -1;
	 
	XWindowAttributes atts;
	XGetWindowAttributes(dpy,win->xlib_window, &atts);
	
	if (atts.map_state==IsViewable) {
		 //setting focus on unviewable will crash program!
		devicemanager->SetFocus(win,const_cast<LaxKeyboard*>(kb),t,0);
		return 0;
	}
	return 1;
}


//! Handle Xlib based events.
/*! Called from run().
 * Converts x events to EventData events as possible before sending the event to the window.
 *
 * If there's a dialog on then screens out improper events, does managefocus().
 *
 * \todo what to do with GenericEvents with no easily discernable targets? is this even an issue?
 */
void anXApp::processXevent(XEvent *xevent)
{
	//DBG cerr <<"processXevent() on "<<xlib_event_name(xevent->xany.type)<<endl;

	EventReceiver *rr=NULL;
	anXWindow *ww=NULL; //note that rr might not have an associated ww!!

	 //find target window for the event, if any
	if (xevent->xany.type!=GenericEvent && xevent->xany.window) ww=findwindow_xlib(xevent->xany.window);
	rr=ww;
	
	//*** if by chance a DestroyNotify is received, should remove that window?
	
	 //convert raw mouse, key, and other device x events to EventData objects.
	 //This is particularly for mapping strange device events. LaxMouse and LaxKeyboard
	 //have the ability to remember certain aspects of recent events.
	EventData *events=NULL;
	int isinputevent=0;
	if (devicemanager) {
		 //devicemanager may convert x events to EventData, and/or absorb the event entirely
		if (devicemanager->eventFilter(&events,xevent,ww, isinputevent)) {
			 //the xevent was either absorbed or converted to EventData.
			 //If the event was a GenericEvent, we still need to free the associated cookie data
		}
	}

#ifdef LAX_USES_XINPUT2
	if (xevent && xevent->xany.type==GenericEvent) {
		 //for some reason, subsequent calls to get cookie data fail,
		 //and device classes do not free that data once they retrieve it, so do it here.
		if (xevent->xcookie.data) XFreeEventData(dpy, &xevent->xcookie);
		if (!events) return; //down below we don't care about GenericEvents 
							//that are not handled by the devicemanager.
	}
#endif
	if (events) xevent=NULL; //if there has been conversion, we don't care about the original xevent any more

	if (!rr && ww) rr=ww;
	if (!rr && !events) {
		 //there was no EventReceiver easily accessible from the event, and there were no
		 //translated events generated, so just do nothing with the event
		return;
	}

	 // *** special hack to deliver X hierarchy changes to top level windows
	 // *** need a better way to deal with these.. they are events selected on root window
	if (events && events->type==LAX_onDeviceChange && events->subtype==LAX_DeviceHierarchyChange) {
		DBG cerr <<" ***** found hierarchy event!!"<<endl;
		EventData *ee=NULL;
		while (events) {
			for (int c=0; c<topwindows.n; c++) {
				rr=topwindows.e[c];
				processSingleDataEvent(rr,events);
			}
			ee=events;
			events=events->next;
			ee->next=NULL;
			delete ee;
		}
		return;
	}

	 //find a target destination
	if (!rr && events) {
		if (!events->to) {
			 //There is no designated target for the event, so
			// *** need to process at app level!!
			//while (events) {
			//	eventCatchAll(events);
			//	ee=events;
			//	events=events->next;
			//	ee->next=NULL;
			//	delete ee;
			//}
			return;
		} else {
			rr=findEventObj(events->to);
			ww=dynamic_cast<anXWindow*>(rr);
		}
	}

	if (events && events->type==LAX_onButtonDown && outclickwatch.n) {
		if (checkOutClicks(NULL,dynamic_cast<MouseEventData*>(events))!=0) {
			events->type=LAX_DefunctEvent;
			isinputevent=0;
		}
	}

	 //--- Dialog event screening for windows
	 //Must screen out any unwanted events if there is a blocking dialog running.
	 // Events are sent only to windows descended from dialog group of dialog 
	 // on the top of the dialog stack.
	//DBG cerr <<"  dialog screen..."<<endl;
	if (ww && isinputevent && dialogs.n) { 
		 //Input related events destined for windows contained by dialogs of the same group
		 //as the top of the dialogs stack are allowed to pass.

		int dialog_group=dialogs.islocal[dialogs.n-1]; //active dialog window group id.
		anXWindow *ww2=TopWindow(ww); //topwindow containing target window

		int c2;
		c2=dialogs.findindex(ww2); //find top of ww in dialogs
		if (c2>=0) {                                   //target window is contained in a dialog
			if (dialogs.islocal[c2]!=dialog_group && isinputevent) //but not in same window group 
				rr=NULL; //And is an input related event, then don't send event
		} else if (isinputevent) rr=NULL; // top not in dialogs, and is an input event

		//at this point, if rr==NULL, then 
		// event did not pass screening, but we don't return until after managefocus() function
	}
	//**** what happens when there is a focus on/off in a restricted window?? just let it on through?

	 // Manage the focus, after initial dialog screen 
	if (managefocus(ww,events)) rr=NULL; // managefocus absorbed the event

	if (xevent) {
		 //Usually these will be events relating to selections, and more obscure window to window manager
		 //communication.
		if (ww && ww->event(xevent)) {  
			//event unprocessed...
		}
	}

	if (!rr) {
		 // this is here because managefocus() still had to be called after screening,
		 // for when the window failed the dialog event screener..
		if (events) delete events;
		return;
	}

	 // Finally dispatch event to window
	//DBG cerr <<"  call ww->event ifww..."<<endl;
	if (events) {
		EventData *ee=NULL;
		while (events) {
			if (rr->object_id!=events->to) rr=findEventObj(events->to);
			processSingleDataEvent(rr,events);
			ee=events;
			events=events->next;
			ee->next=NULL;
			delete ee;
		}
	}

	return;
}

void printxcrossing(anXWindow *win,XEvent *e);

//! Called from event loop, is supposed to do reasonable things with focus related events.
/*! Window ww is found for event e.
 * 
 * Catches LAX_onButtonDown and LAX_onMouseIn.
 * Always focus on a window that is clicked in.
 * Set focus on any window with with ANXWIN_HOVER_FOCUS when the mouse enters.
 * 
 * Returns 0 if it does not absorb event, 1 if it does.
 *
 * \todo sometimes, my WM not sending FocusIn/Out on Enter/Leave, but is changing focus regardless... 
 *   need to check if this is still true
 */
int anXApp::managefocus(anXWindow *ww, EventData *ev)
{
	if (!ww) return 0;
	if (!ev) return 0;

	//handled in DeviceManager:
	//		case FocusIn
	//		case FocusOut
			
	if (ev->type==LAX_onButtonDown) {
		// always set focus if isn't active on button click
		MouseEventData *bev=dynamic_cast<MouseEventData*>(ev);
		LaxMouse *mouse=dynamic_cast<LaxMouse*>(bev?bev->device:NULL);
		if (mouse && mouse->paired_keyboard 
				&& mouse->paired_keyboard->current_focus
				&& mouse->paired_keyboard->current_focus->object_id!=ev->to) {
			devicemanager->SetFocus(ww,mouse->paired_keyboard,times(NULL),0);
		}
		return 0;

	} else if (ev->type==LAX_onMouseIn) {
		 // set focus if necessary 
		EnterExitData *ee=dynamic_cast<EnterExitData*>(ev);
		LaxMouse *mouse=dynamic_cast<LaxMouse*>(ee?ee->device:NULL);
		if ((ww->win_style&ANXWIN_HOVER_FOCUS) && mouse->paired_keyboard) {
			devicemanager->SetFocus(ww,mouse->paired_keyboard,times(NULL),0);
		}
		return 0;
	}
	return 0;
}

/*!
 * <pre>
 * *** whats up with this? is it still used??
 * //quick fix for window manager not sending FocusOut 
 * //to a sub-window if focus leaves its top window
 * </pre>
 */
void anXApp::deactivatekids(anXWindow *ww)
{
	if (ww->win_active) {
		XEvent e;
		e.xfocus.type=FocusOut;
		e.xfocus.display=dpy;
		e.xfocus.window=ww->xlib_window;
		ww->event(&e);
	}
	for (int c=0; c<ww->_kids.n; c++) {
		deactivatekids(ww->_kids.e[c]);
	}
}

/*! Returns old max timeout */
int anXApp::SetMaxTimeout(int timeoutmax)
{
	int m=maxtimeout;
	maxtimeout=timeoutmax;
	return m;
}

//! Add a timer. Return the timer id.
/*! strt,next are in milliseconds, they get converted to clock ticks in TimerInfo.
 *
 * Once a timer is established, after the specified time, then a windows anXWindow::Idle()
 * function will be called, with the associated timer id.
 *
 * A duration of -1 means about 1000 hours, so basically forever.
 */
int anXApp::addtimer(EventReceiver *win, //!< The window to create the timer for
					int strt, //!< Time to wait for the first timer event (milliseconds)
					int next, //!< Time to wait for each successive timer event (milliseconds)
					int duration //!< How long the timer should last (milliseconds)
				)
{
	if (!win) return 0;
	//TimerInfo(anXWindow *nwin,int duration,int firstt,int tickt,int nid,long ninfo);
	int nid=getUniqueNumber();
	timers.push(new TimerInfo(win,duration,strt,next,nid,0));

	DBG cerr <<"addtimer: "<<win->object_id<<"  id:"<<nid<<"  duration:"<<duration<<"  next:"<<next<< " ms"<<"   numtimers="<<timers.n<<endl;
	return nid;
}

//! This removes a timer manually.
/*! Timers can be created with a number of ticks to last, and the timer is automatically
 *  removed when there are no more ticks. Users can also choose to manually remove
 *  timers here.
 *
 * If timerid==0, then remove any timer of w.
 */
int anXApp::removetimer(EventReceiver *w,int timerid)
{
	int c;
	for (c=0; c<timers.n; c++) {
		if (w==timers.e[c]->win) {
			if (timerid>0 && timerid==timers.e[c]->id) break;
			if (!timerid) { timers.remove(c); c--; }
		}
	}
	if (c>=timers.n) return 1;
	DBG cerr <<"remove timer:"<<timerid<<endl;
	timers.remove(c);
	return 0;
}

//! Add a timer with the default mouse button delays. Returns timer id.
/*! Please note that the timer is not automatically destroyed
 * when the button is up. 
 *
 * \todo maybe figure out how to make automatic turnoff when button up???
 */
int anXApp::addmousetimer(EventReceiver *win)
{
	long c=sysconf(_SC_CLK_TCK);
	return addtimer(win,firstclk*1000/c,idleclk*1000/c,-1);
}

//! Handles idling (non-timer calls) for the window and its children.
/*! Called from a loop over topwindows in run. Is supposed to call w->Idle(0) if
 * necessary, then recursively call idle() with the kids of w.
 * Default just returns.
 *
 * \todo remove this function? is do nothing now
 */
void anXApp::idle(anXWindow *w)
{
	return;
	//if (!w) return;
	//for (int c=0; c<w->_kids.n; c++) idle(w->_kids.e[c]);
}

//! Handles refreshing for window w and its children.
/*! Called from a loop over topwindows in run. Calls w->Refresh() only
 * if (w->Needtodraw() && w->win_on). The window must clear needtodraw itself.
 *
 * Returns the number of windows saying they still need to be refreshed.
 */
int anXApp::refresh(anXWindow *w)
{
	if (!w) return 0;
	int n=0;
	if (w->Needtodraw() && w->win_on) { 
		w->Refresh();
		if (w->Needtodraw()) {
			DBG cerr <<"Needs to draw: "<<w->WindowTitle()<<" child of "<<(w->win_parent?w->win_parent->WindowTitle():"null")
			DBG      <<" index: "<<(w->win_parent?w->win_parent->_kids.findindex(w):-1)<<"  "<<w->whattype()<<endl;
			n++; 
		}
	}
	for (int c=0; c<w->_kids.n; c++) n+=refresh(w->_kids.e[c]);
	return n;
}

//! Anything can call app->postmessage when they want to make some status statement.
/*! Builtin default is to do nothing. (well, cout something in debug version)
 */
void anXApp::postmessage(const char *str)
{
	DBG cout <<str<<endl;
}

//! Find a window to potentially drop things into.
/*! x,y are coordinates in ref. If ref==NULL, then they are coordinates of the root window.
 *
 * If drop!=NULL, then assume you only want a window that this application knows about.
 * Otherwise, also search for any window that X knows about. The window that is found is
 * returned in drop.
 *
 * Ultimately this will be used for more full featured drag and drop.
 *
 * See also mouseposition(), which uses XInput2Pointer::getInfo() which seems to be pretty reliable.
 *
 * \todo cannot find windows that X knows about that the application doesn't, so cannot
 *   drop to other programs (yet)!
 * \todo this needs work... especially when there are windows on different screens..
 *   laxkit in general does not handle multiple screens well (ie, at all)..
 * \todo  ***** this ignores stacking order!!!
 * \todo this needs to be redone to use a mouse device, which is much easier to find drop candidates..
 *        it is not so necessary to find candidates for arbitrary coordinates.
 */
anXWindow *anXApp::findDropCandidate(anXWindow *ref,int x,int y,anXWindow **drop, Window *xlib_window_ret)
{
	 //find suitable xlib source window
	Window xwin=0;

	if (ref && ref->xlib_window) {
		xwin=ref->xlib_window;

	} else {	
		while (ref) {
			if (x>=0 && x<ref->win_w && y>=0 && y<ref->win_h) break;
			x+=ref->win_x;
			y+=ref->win_y;
			ref=ref->win_parent;
		}
		xwin=DefaultRootWindow(dpy);
	}

	//now x,y are coordinates in xwin

	int nx,ny;
	Window destwin=DefaultRootWindow(dpy);
	Window child;
	//ScreenOfRoot?? *** how to find screen number from arbitrary window??
	//Screen *DefaultScreenOfDisplay(dpy)
	//Screen *ScreenOfDisplay(dpy, screen_number);
	//Window XRootWindow(dpy, screen_number);
	//int ScreenCount(dpy);
	

	//XTranslateCoordinates(Display *display, Window src_w, dest_w, int src_x, int src_y, int *dest_x_return, int *dest_y_return, Window *child_return)
	Bool status=XTranslateCoordinates(dpy, xwin, destwin, x,y, &nx,&ny, &child);
	if (status==False) {
		//src and dest are on different screens
		if (xlib_window_ret) *xlib_window_ret=0;
		if (drop) *drop=NULL;
		return NULL;
	}
	if (xlib_window_ret) {
		 //top level x window, must reread dnd spec, seem to remember it being defined only for top level windows
		if (child) *xlib_window_ret=child;
		else *xlib_window_ret=0;
	}

	while (child) {
		xwin=destwin;
		destwin=child;
		x=nx;
		y=ny;
		status=XTranslateCoordinates(dpy, xwin, destwin, x,y, &nx,&ny, &child);
	}

	 //now destwin is final resting place of x,y
	
	anXWindow *win_ret=findwindow_xlib(destwin);
	if (drop) *drop=win_ret;
	return win_ret;



//	 // now either x and y are root coords
//	int c;
//	for (c=0; c<topwindows.n; c++) {
//		ref=topwindows.e[c];
//		if (x>=ref->win_x && x<ref->win_x+ref->win_w && y>=ref->win_y && y<ref->win_y+ref->win_h) break;
//	}
//	if (c==topwindows.n) ref=NULL;
//
//	if (!ref) {
//		 // is not a window known to the application
//		//***must do the whole xdnd thing..
//		//if (xlib_window_ret) *xlib_window_ret=foundwindow;
//		if (drop) *drop=NULL;
//		return NULL;
//	}
//
//	 //now ref points to the topwindow that contains the coordinates.
//	 //Need to find which subwindow actually contains them.
//	int d=0; //subwindow depth, for debugging purposes
//	anXWindow *refc=NULL;
//	do { //one iteration for each level of subwindow
//		if (ref->_kids.n==0) break;
//		for (c=0; c<ref->_kids.n; c++) {
//			refc=ref->_kids.e[c];
//			if (x>=refc->win_x && x<refc->win_x+refc->win_w 
//					&& y>=refc->win_y && y<refc->win_y+refc->win_h) {
//				 //coordinates are in refc
//				x-=refc->win_x;
//				y-=refc->win_y;
//				break;
//			}
//		}
//		if (c==ref->_kids.n) break; //coordinates are in ref, but not in any child of ref
//		ref=refc;
//		d++;
//	} while (1);
//
//	if (drop) *drop=ref;
//	return ref;
}



// done with anXApp... now for a couple of debugging helpers
//------------------------- general utilities, not really necessary -------------/

const char *xlib_extension_event_name(int e_type)
{
	  //-----------extension events
#ifdef LAX_USES_XINPUT2
	const char *s="(unknown)";
	if (e_type==XI_DeviceChanged) s="XI_DeviceChanged";    
	else if (e_type==XI_KeyPress) s="XI_KeyPress";         
	else if (e_type==XI_KeyRelease) s="XI_KeyRelease";       
	else if (e_type==XI_ButtonPress) s="XI_ButtonPress";      
	else if (e_type==XI_ButtonRelease) s="XI_ButtonRelease";    
	else if (e_type==XI_Motion) s="XI_Motion";           
	else if (e_type==XI_Enter) s="XI_Enter";            
	else if (e_type==XI_Leave) s="XI_Leave";            
	else if (e_type==XI_FocusIn) s="XI_FocusIn";          
	else if (e_type==XI_FocusOut) s="XI_FocusOut";         
	else if (e_type==XI_HierarchyChanged) s="XI_HierarchyChanged"; 
	else if (e_type==XI_PropertyEvent) s="XI_PropertyEvent";    
	else if (e_type==XI_RawKeyPress) s="XI_RawKeyPress";      
	else if (e_type==XI_RawKeyRelease) s="XI_RawKeyRelease";    
	else if (e_type==XI_RawButtonPress) s="XI_RawButtonPress";   
	else if (e_type==XI_RawButtonRelease) s="XI_RawButtonRelease"; 
	else if (e_type==XI_RawMotion) s="XI_RawMotion";        
	return s;
#else
	return NULL;
#endif
}

//! Return name for an xlib core event
/*! \ingroup misc */
const char *xlib_event_name(int e_type) 
{
  static char text[80];

  switch (e_type) {
    case KeyPress : sprintf(text, "KeyPress"); break;
    case KeyRelease : sprintf(text, "KeyRelease"); break;
    case ButtonPress : sprintf(text, "ButtonPress"); break;
    case ButtonRelease : sprintf(text, "ButtonRelease"); break;
    case MotionNotify : sprintf(text, "MotionNotify"); break;
    case EnterNotify : sprintf(text, "EnterNotify"); break;
    case LeaveNotify : sprintf(text, "LeaveNotify"); break;
    case FocusIn : sprintf(text, "FocusIn"); break;
    case FocusOut : sprintf(text, "FocusOut"); break;
    case KeymapNotify : sprintf(text, "KeymapNotify"); break;
    case Expose : sprintf(text, "Expose"); break;
    case GraphicsExpose : sprintf(text, "GraphicsExpose"); break;
    case NoExpose : sprintf(text, "NoExpose"); break;
    case VisibilityNotify : sprintf(text, "VisibilityNotify"); break;
    case CreateNotify : sprintf(text, "CreateNotify"); break;
    case DestroyNotify : sprintf(text, "DestroyNotify"); break;
    case UnmapNotify : sprintf(text, "UnmapNotify"); break;
    case MapNotify : sprintf(text, "MapNotify"); break;
    case MapRequest : sprintf(text, "MapRequest"); break;
    case ReparentNotify : sprintf(text, "ReparentNotify"); break;
    case ConfigureNotify : sprintf(text, "ConfigureNotify"); break;
    case ConfigureRequest : sprintf(text, "ConfigureRequest"); break;
    case GravityNotify : sprintf(text, "GravityNotify"); break;
    case ResizeRequest : sprintf(text, "ResizeRequest"); break;
    case CirculateNotify : sprintf(text, "CirculateNotify"); break;
    case CirculateRequest : sprintf(text, "CirculateRequest"); break;
    case PropertyNotify : sprintf(text, "PropertyNotify"); break;
    case SelectionClear : sprintf(text, "SelectionClear"); break;
    case SelectionRequest : sprintf(text, "SelectionRequest"); break;
    case SelectionNotify : sprintf(text, "SelectionNotify"); break;
    case ColormapNotify : sprintf(text, "ColormapNotify"); break;
    case ClientMessage : sprintf(text, "ClientMessage"); break;
    case MappingNotify : sprintf(text, "MappingNotify"); break;
    case GenericEvent : sprintf(text, "GenericEvent"); break;
    case LASTEvent : sprintf(text, "LASTEvent"); break;
  }
  return text;
}

/*! \ingroup misc 
 * \brief cerr an XCrossingEvent. This is used in debugging mode..
 */
void printxcrossing(anXWindow *win,XEvent *e)
{
	// XCrossingEvent
	cerr <<"XCrossingEvent for window "<<e->xcrossing.window<<", win="
			<<win->xlib_window<<", subwindow="<<e->xcrossing.subwindow<<": "<<endl<<"  ";
	if (e->xcrossing.type==EnterNotify) cerr << win->WindowTitle() <<" EnterNotify:  "; else cerr <<win->WindowTitle()<<" LeaveNotify:  ";
	switch(e->xcrossing.mode) {
			case NotifyNormal: cerr <<"NotifyNormal, "; break;
			case NotifyGrab: cerr <<"NotifyGrab, "; break;
			case NotifyUngrab: cerr <<"NotifyUngrab, "; break;
	}
	switch(e->xcrossing.detail) {
			case NotifyAncestor: cerr <<"NotifyAncestor, "; break;
			case NotifyVirtual: cerr <<"NotifyVirtual, "; break;
			case NotifyInferior: cerr <<"NotifyInferior, "; break;
			case NotifyNonlinear: cerr <<"NotifyNonlinear, "; break;
			case NotifyNonlinearVirtual: cerr <<"NotifyNonlinearVirtual, "; break;
	}
	if (e->xcrossing.focus==True) cerr <<"focus=true"; else cerr <<"focus==false";
	if (e->xcrossing.subwindow==win->xlib_window) cerr <<", this window"<<endl;
	else if (e->xcrossing.subwindow==0) cerr <<", no subwindow"<<endl;
	else if (win->app->findwindow_xlib(e->xcrossing.subwindow)) {
		cerr <<", apps windows ("<<win->app->findwindow_xlib(e->xcrossing.subwindow)->whattype()<<")but not this"<<endl;
	} else cerr<<", some other window"<<endl;
}



//--------------------------------- key composing via deadkeys ------------------------
//                       This section is adapted from fltk.org compose.cxx
//                which was Copyright 1998-2006 by Bill Spitzak and others, LGPL

unsigned int filterkeysym(KeySym keysym,unsigned int *state);
unsigned int composekey(unsigned int k1, unsigned int k2);

// Before searching anything the following conversions are made:
// ';' -> ":"     "|" -> "/"    "=",'_' -> "-"

// This table starts at character 0xA0 (non-breaking space)
// The characters may be typed in either order after the compose key.
// If the second character is a space then only the first character
// needs to be typed.
// I changed these slightly from fltk 1.0 to match X compose
// sequences in cases when my version did not use the same characters
// as the X sequence. Comments show the original versions.

static const char compose_pairs[] = {
  "  "	// nbsp
  "! "	// inverted !
  "c/"	// cent		 (was "% ")
  "l-"	// pound	 (was "# ")
  "xo"	// currency	 (was "$ ")
  "y-"	// yen
  "/ "	// broken bar
  "s "	// section	 (was "& ", X uses "so")
  ": "	// dieresis
  "c "	// copyright	 (X uses "co")
  "a "	// superscript a (X uses "a-")
  "<<"	// <<
  "-,"	// not sign	 (was "~ ")
  "- "	// hyphen
  "r "	// registered	 (X uses "ro")
  "--"	// macron	 (was "_ ", X uses "-^")
  "0 "	// superscript 0 (degree, was "* ", X uses "0^")
  "+-"	// plusminus
  "2 "	// superscript 2 (X uses "2^")
  "3 "	// superscript 3 (X uses "3^")
  "' "	// acute
  "u "	// mu
  "p "	// paragraph
  ". "	// centered dot
  ", "	// cedilla
  "1 "	// superscript 1 (X uses "1^")
  "o "	// superscript o (X uses "o-")
  ">>"	// >>
  "14"	// 1/4
  "12"	// 1/2
  "34"	// 3/4
  "? "	// inverted ?
  "A`"
  "A'"
  "A^"
  "A~"
  "A:"
  "A*"
  "AE"
  "C,"
  "E`"
  "E'"
  "E^"
  "E:"
  "I`"
  "I'"
  "I^"
  "I:"
  "D-"
  "N~"
  "O`"
  "O'"
  "O^"
  "O~"
  "O:"
  "x "	// multiply
  "O/"
  "U`"
  "U'"
  "U^"
  "U:"
  "Y'"
  "TH"
  "ss"
  "a`"
  "a'"
  "a^"
  "a~"
  "a:"
  "a*"
  "ae"
  "c,"
  "e`"
  "e'"
  "e^"
  "e:"
  "i`"
  "i'"
  "i^"
  "i:"
  "d-"
  "n~"
  "o`"
  "o'"
  "o^"
  "o~"
  "o:"
  "-:"	// divide
  "o/"
  "u`"
  "u'"
  "u^"
  "u:"
  "y'"
  "th"
  "y:"
  // End of ISO-8859-1, start of Unicode:
  "A-" // U+0100
  "a-"
  "Au" // A+ugonek?
  "au" // a+ugonek?
  "A," // ?
  "a,"
  "C'"
  "c'"
  "C^"
  "c^"
  "C."
  "c."
  "Cv"
  "cv"
  "D'" // has a 'v' over a D
  "d'"
  "D-" // U+0110
  "d-"
  "E-"
  "e-"
  "Eu"
  "eu"
  "E."
  "e."
  "E,"
  "e,"
  "Ev"
  "ev"
  "G^"
  "g^"
  "Gu"
  "gu"
  "G." // U+0120
  "g."
  "G,"
  "g'"
  "H^"
  "h^"
  "H-"
  "h-"
  "I~"
  "i~"
  "I-"
  "i-"
  "Iu"
  "iu"
  "I,"
  "i,"
  "I." // U+0130
  "i " // dot-less i?
  "IJ"
  "ij"
  "J^"
  "j^"
  "K,"
  "k,"
  "k " // small capital K?
  "L'"
  "l'"
  "L,"
  "l,"
  "L'"
  "l'"
  "L."
  "l." // U+0140
  "L/"
  "l/"
  "N'"
  "n'"
  "N,"
  "n,"
  "Nv"
  "nv"
  "n " // 'n
  "N,"
  "n,"
  "O-"
  "o-"
  "Ou"
  "ou"
  "O\"" // U+0150
  "o\""
  "OE"
  "oe"
  "R'"
  "r'"
  "R,"
  "r,"
  "Rv"
  "rv"
  "S'"
  "s'"
  "S^"
  "s^"
  "S,"
  "s,"
  "Sv" // U+0160
  "sv"
  "T,"
  "t,"
  "Tv" // has a 'v' over it
  "t'" // has a quote
  "T-"
  "t-"
  "U~"
  "u~"
  "U-"
  "u-"
  "Uu"
  "uu"
  "U*"
  "u*"
  "U\"" // U+0170
  "u\""
  "U,"
  "u,"
  "W^"
  "w^"
  "Y^"
  "y^"
  "Y:"
  "Z'"
  "z'"
  "Z."
  "z."
  "Zv"
  "zv"
};

// X dead-key lookup table.  This turns a dead-key keysym into the
// first of two characters for one of the compose sequences.  These
// keysyms start at 0xFE50.
// Win32 handles the dead keys before fltk can see them.  This is
// unfortunate, because you don't get the preview effect.
static const char dead_keys[] = {
  '`',	// XK_dead_grave
  '\'',	// XK_dead_acute
  '^',	// XK_dead_circumflex
  '~',	// XK_dead_tilde
  '-',	// XK_dead_macron
  'u',	// XK_dead_breve
  '.',	// XK_dead_abovedot
  ':',	// XK_dead_diaeresis
  '*',	// XK_dead_abovering
  '"',	// XK_dead_doubleacute
  'v',	// XK_dead_caron
  ',',	// XK_dead_cedilla
  ','	// XK_dead_ogonek
//   0,	// XK_dead_iota
//   0,	// XK_dead_voiced_sound
//   0,	// XK_dead_semivoiced_sound
//   0	// XK_dead_belowdot
};

//! From 2 keys, typically an ascii key and a dead key, compose another.
/*! If k1 and k2 are not deadkeys and k1 and k2 are greater than 127, then 0 is returned.
 *
 * If k2==0, then try to find a compose code for k1 on its own. In this case,
 * if none is found, 0 is returned.
 *
 * If both k1 and k2 are valid keys, and if the combination is not recognized,
 * then k2 is returned.
 *
 * This is used by anXApp to decipher key input for characters with accents and such.
 */
unsigned int composekey(unsigned int k1, unsigned int k2)
{
	char ch1,ch2;

	if (k1 >= 0xfe50 && k1 <= 0xfe5b) ch1 = dead_keys[k1-0xfe50];
	else if (k1>127) return 0;
	else ch1=k1;

	if (k2 >= 0xfe50 && k2 <= 0xfe5b) ch2 = dead_keys[k1-0xfe50];
	else if (k2>127) return 0;
	else ch2=k2;
	
	if      (ch1 == ';') ch1 = ':';
	else if (ch1 == '|') ch1 = '/';
	else if (ch1 == '_' || ch1 == '=') ch1 = '-';

	if      (ch2 == ';') ch2 = ':';
	else if (ch2 == '|') ch2 = '/';
	else if (ch2 == '_' || ch2 == '=') ch2 = '-';

    // this happens when composekey then some key is pressed, check to see if the
	// single key in ch1 maps to anything
	if (ch2==0) {
		for (const char *p = compose_pairs; *p; p += 2) {
			if (p[0] == ch1 || p[1] == ch1) {
				// prefer the single-character versions:
				if (p[1] == ' ') {
				  int code = (p-compose_pairs)/2+0xA0;
				  return code;
				}
			}
		}
		return 0; //no match found
	}

	 //so now we have 2 keys to look up..
	 //search for the pair in either order:
	for (const char *p = compose_pairs; *p; p += 2) {
		if (((p[0] == ch2) && (p[1] == ch1)) || (p[1] == ch2 && p[0] == ch1)) {
			int code = (p-compose_pairs)/2+0xA0;
			return code;
		}
	}

	return k2;//no match found
}



//----------------key composition helper **** needs work!! not mpx compliant ---------------------

/*! \todo since I don't understand how XIM works, this stuff will stay here for the time being.
 *    really it should be mpx compliant!!!
 *  \todo *** this is rather messy and needs serious debugging:
 */
int anXApp::filterKeyEvents(LaxKeyboard *kb, anXWindow *win,
							XEvent *e,unsigned int &key, char *&buffer, int &len, unsigned int &state)
{
	if (kb!=xim_current_device || win->object_id!=xim_current_window) {
		 //reset xim
		xim_current_device=kb;
		xim_current_window=win->object_id;
		CreateXInputContext();

		XSetICValues(xim_ic,
					XNClientWindow, win->xlib_window,
					NULL);
		XSetICFocus(xim_ic);
	}

	buffer=NULL;
	len=0;
	key=0;
	state=e->xkey.state;

	KeySym keysym=0;  
	int maxbuf=20;
	buffer=new char[maxbuf];
	buffer[0]='\0';

	if (xim) {
		DBG cerr <<"anXApp::filterKeyEvents(): using x input method on keypress..."<<endl;
		 //using x input method...
		Status status;
		buffer[0]=0;
		do {
			len=Xutf8LookupString(xim_ic, &e->xkey, buffer, maxbuf-1, &keysym, &status);
			if (status==XBufferOverflow) {
				DBG cerr <<"  buffer not big enough"<<endl;
				delete[] buffer;
				buffer = new char[maxbuf = len+1];
				continue;
			} else if (status==XLookupNone) { 
				DBG cerr <<"  utf8lookup returned no text."<<endl;
				delete[] buffer; 
				buffer=NULL;
				return 1; //no character data to return yet
			}
			 //else status is one of: XLookupChars | XLookupKeySym | XLookupBoth
			break;
		} while (1);
		buffer[len]='\0';
		if (status==XLookupChars || status==XLookupBoth) {
			DBG cerr <<"  "<<(status==XLookupChars?"XLookupChars":"XLookupBoth")<<endl;

			 //remove the control-char remapping that Xutf8LookupString() apparently does
			if (status==XLookupBoth && keysym<255) key=filterkeysym(keysym,&state);
			else key=utf8decode(buffer,buffer+len,&len);

			DBG cerr <<"  "<<len<<" chars inputed: "<<(len?buffer:"(none)")<<", 1st="<<(int)buffer[0]<<endl;
		} else {
			 //was XLookupKeysym, use the old standby. probably not an actual printing character:
			delete[] buffer; buffer=NULL; len=0;
			char ch;
			len=XLookupString(&e->xkey,&ch,1,&keysym,NULL);
			key=filterkeysym(keysym,&state);

			DBG cerr<<"  ***keysym but no translated string:"<<keysym<<endl;
		}
		if (keysym>=0xfe50 && keysym<=0xfe62) {
			 //is a dead key
			if (!xim_deadkey) { 
				 //next key input will be combined with this deadkey
				xim_deadkey=keysym; 
				delete[] buffer; 
				return 1;  //no final output yet
			}
			key=keysym;
		}
		if (key==8) key=LAX_Bksp;
		else if (key==127) key=LAX_Del;
		else if (key==27) key=LAX_Esc;
		if (key==LAX_Bksp || key==LAX_Del || key==LAX_Esc) { delete[] buffer; buffer=NULL; len=0; }
	} else {
		 //no app->xim, use plain old Latin-1 lookup
		DBG cerr <<"not using x input method on keypress...";
		len=XLookupString(&e->xkey,buffer,maxbuf,&keysym,NULL);
		buffer[len]='\0';
		key=filterkeysym(keysym,&state);
		DBG cerr <<"  key:"<<(int)key<<endl;
	}
	
	//DBG cerr << win_title<<": keypress ";
	//DBG cerr.setf(ios_base::hex,ios_base::basefield);
	//DBG cerr <<"  keysym:"<<keysym;
	//DBG cerr.setf(ios_base::dec,ios_base::basefield);
	//DBG cerr <<"  name:"<<XKeysymToString(keysym)<<endl;

	 //***I have no idea if this actually works as intended
	if (keysym==XK_Multi_key) { xim_deadkey=~0; delete[] buffer; return 1; }
	else if (xim_deadkey==(unsigned int)~0) { 
		 //might be composekey then some single key, rather than compose then 2 keys..
		xim_deadkey=composekey(key,0);
		if (xim_deadkey) {
			 //modify buffer
			if (!buffer) buffer=new char[len+1];
			len=utf8encode(xim_deadkey,buffer);
			buffer[len]='\0';
			key=xim_deadkey;
			xim_deadkey=0;
		} else {
			xim_deadkey=key;
			delete[] buffer;
			return 1; //no final output yet
		}
	} else if (xim_deadkey) {
		 //combine current key with xim_deadkey
		key=composekey(xim_deadkey,key);
		xim_deadkey=0;
	}

	return 0;
}

//------------------------------end key compose section--------------------------------


//! Converts an Xlib keysym to a Laxkit key value.
/*! This is called when deciphering key input from X via Xutf8LookupString(), when it returns 
 * a keysym but no buffer data.
 * It is then assumed that the keysym corresponds to some control or function key.
 *
 * If the keysym corresponds directly to a UCS value, then that value is returned. This happens
 * when the keysym>0x01000000 and less than 0x02000000. The UCS value is then keysym&0x00ffffff.
 *
 * If a keypad key is detected, then state is modified to have KeypadMask set.
 *
 * Look in laxdefs.h for a number of key codes corresponding to various "extra" keys found
 * on some keyboards.
 *
 * \todo If the keysym corresponds to any of the keys in laxdefs.h, then it is mapped to those values.
 *   Otherwise...? CharInput() says if the returned value is > 0x1000000, then it is either
 *   a Laxkit value, or the X keysym plus 0x1000000, which is not quite how it is coded right now.
 *   need to check if the keysym can still be converted to a unicode value. keysyms correspond
 *   pretty well to it, and should have been converted to the buffer in event() if so, but not sure of that.
 */
unsigned int filterkeysym(KeySym keysym,unsigned int *state)
{
	unsigned int uch=0;

	 //detect keypad keys
	if (keysym>=0xff80 && keysym<=0xffb9) *state|=KeypadMask;
	if (keysym>=XK_KP_0 && keysym<=XK_KP_9) return '0'+(keysym-XK_KP_0); //keypad numbers

	 //return UCS if there already
	if ((keysym&0xff000000) == 0x01000000) return keysym&0xffffff;

	 //map control keys
	switch(keysym) {
		case 0:					uch=0;           break;
		case XK_Shift_L:			
		case XK_Shift_R:		uch=LAX_Shift;   break;
		case XK_Control_L:		
		case XK_Control_R:		uch=LAX_Control; break;
		case XK_Escape: 		uch=LAX_Esc;     break;
		case XK_Menu:           uch=LAX_Menu;    break; // The menu button isn't like alt/cntl/shift/meta
		case XK_Pause:			uch=LAX_Pause;   break;
		case XK_Alt_L:
		case XK_Alt_R:			uch=LAX_Alt;     break;
		case XK_Super_L:
		case XK_Super_R:		uch=LAX_Meta;    break; // that extra mod key
		case XK_KP_Delete:
		case XK_Delete: 		uch=LAX_Del;     break;
		case XK_BackSpace:		uch=LAX_Bksp;    break;
		case XK_ISO_Left_Tab:
		case XK_KP_Tab:
		case XK_Tab: 			uch=LAX_Tab;     break;
		case XK_KP_Insert:
		case XK_Insert:			uch=LAX_Ins;     break;
		case XK_KP_Home:
		case XK_Home:			uch=LAX_Home;    break;
		case XK_KP_End:
		case XK_End:			uch=LAX_End;     break;
		case XK_KP_Enter:
		case XK_Return:			uch=LAX_Enter;   break;
		case XK_KP_Page_Up:
		case XK_Page_Up:		uch=LAX_Pgup;    break;
		case XK_KP_Page_Down:
		case XK_Page_Down: 		uch=LAX_Pgdown;  break;
		case XK_KP_F1:
		case XK_F1:				uch=LAX_F1;      break;
		case XK_KP_F2:
		case XK_F2:				uch=LAX_F2;      break;
		case XK_KP_F3:
		case XK_F3:				uch=LAX_F3;      break;
		case XK_KP_F4:
		case XK_F4:				uch=LAX_F4;      break;
		case XK_F5:				uch=LAX_F5;      break;
		case XK_F6:				uch=LAX_F6;      break;
		case XK_F7:				uch=LAX_F7;      break;
		case XK_F8:				uch=LAX_F8;      break;
		case XK_F9:				uch=LAX_F9;      break;
		case XK_F10:			uch=LAX_F10;     break;
		case XK_F11:			uch=LAX_F11;     break;
		case XK_F12:			uch=LAX_F12;     break;
		case XK_KP_Left:
		case XK_Left: 			uch=LAX_Left;    break;
		case XK_KP_Up:
		case XK_Up:				uch=LAX_Up;      break;
		case XK_KP_Down:
		case XK_Down: 			uch=LAX_Down;    break;
		case XK_KP_Right:
		case XK_Right:			uch=LAX_Right;   break;

		case XK_KP_Space:       uch=' ';         break;
		case XK_KP_Equal:       uch='=';         break;
		case XK_KP_Multiply:    uch='*';         break;
		case XK_KP_Add:         uch='+';         break;
		case XK_KP_Subtract:    uch='-';         break;
		case XK_KP_Decimal:     uch='.';         break;
		case XK_KP_Divide:      uch='/';         break;
		//case XK_KP_Begin:       uch=???;         break;
		//case XK_KP_Separator:   uch=???;         break;
		//case XK_KP_Begin:       uch=???;         break;
	}
	if (!uch && keysym>=32) return (unsigned int) keysym;
	return uch;
}

} // namespace Laxkit

