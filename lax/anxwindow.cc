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
//    Copyright (C) 2004-2013 by Tom Lechner
//

#ifdef _LAX_PLATFORM_XLIB
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/X.h>
#endif

#include <lax/anxapp.h>
#include <lax/strmanip.h>
#include <lax/utf8utils.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

//------------------------------------ aDrawable -------------------------------------
/*! \class aDrawable
 * \brief Class to hold basic rendering surface of a window.
 */


#ifdef _LAX_PLATFORM_XLIB

/*! \var Window aDrawable::xlib_window
 * \brief The Xlib window associated with the anXWindow.
 */
/*! \var XdbeBackBuffer xlib_backbuffer
 * \brief A potential back buffer to use.
 */


//! Return some Xlib can draw on.
/*! If which==-1, then return the default drawing surface. When there is double buffering,
 * this means return xlib_backbuffer if it is not 0, otherwise xlib_window.
 * 
 * if (which==0) then return xlib_window.
 * if (which==1) then return xlib_backbuffer, whether or not it is 0.
 */
Drawable aDrawable::xlibDrawable(int which)
{
	if (which==-1) return xlib_backbuffer ? xlib_backbuffer : xlib_window;
	if (which==0) return xlib_window;
	return xlib_backbuffer;
}

#endif //_LAX_PLATFORM_XLIB


//------------------------------------ anXWindow -------------------------------------
/*! \class anXWindow
 *  \brief This is the basic window unit for the Laxkit.
 *
 *  anXWindow is the barest form of a window class available in the Laxkit.
 *  Most people will not use this class directly, using all the various subclasses
 *  instead. If you design your own windows basically from scratch, you would
 *  subclass anXWindow.
 *
 *  anXWindow::event() routes the X events to the corresponding
 *  member functions. Any X events not dealt with there are ignored, but classes derived from anXWindow
 *  can redefine event to catch those. Just be sure to also call anXWindow::event() to ensure
 *  proper focusing and such. Dealt with is resizing the window when the user resizes via the
 *  mouse dragging window manager decorations, and also implements the delete protocol when the 'x'
 *  (provided by your window manager, unrelated to Laxkit) is clicked. It tells anXApp to destroy this
 *  window only if deletenow() returns 1. If deletenow() returns 2, then the window is just unmapped.
 *  If deletenow() returns 3, the window is kept mapped.
 *
 *  Event handling functions must return 0 if they actually deal with an event. If 
 *  a nonzero value is returned, then typically the event will be propagated to the window's parent.
 * 
 * \section windowstyles anXWindow Styles
 *
 * The base style of a window is stored in win_style. Currently, anXWindow reserves the right
 * to uses bits 0..15. Derived classes can use any other bits in win_style if they want.
 * All the bits taken by anXWindow
 * can be screened for with ANXWIN_MASK (==0xffff).
 *
 * \code
 *
 *   When the window is created and this flag is set, then the window will be created
 *   as being tagged as kind of a subprocess of its owner window. In terms of Xlib,
 *   the transient_for hint will be set, referencing the window's owner.
 *  #define ANXWIN_TRANSIENT
 *
 *   Do not let the window have a little "x" decoration on it to let users close it easily.
 *  #define ANXWIN_NOT_DELETEABLE
 *
 *   If this is present, then this window and all its descendents must be considered to
 *   be grayed, meaning you can't interact with it.
 *  #define ANXWIN_GRAYED
 *
 *   Based on a key returned by whattype(), when this type of window is closed, it will
 *   save information about itself in anXApp. See anXApp::Resource() for more info.
 *  #define ANXWIN_REMEMBER
 *
 *   Flag this window as being capable of receiving (and maybe sending) 
 *   drag-n-drop events.
 *  #define ANXWIN_XDND_AWARE
 *
 *   A window with this style will gain the input focus whenever it
 *   receives and EnterNotify event. If this style is not set,
 *   you must click in a window to make the focus be that window.
 *  #define ANXWIN_HOVER_FOCUS
 *   
 *   This is for windows that do not want to be alerted to any input events of any kind.
 *  #define ANXWIN_NO_INPUT
 *
 *   Makes the window with no decorations. In terms of Xlib, this is a window
 *   with the WM_TRANSIENT_FOR property set, together with override_redirect of the 
 *   XSetWindowAttributes struct. These make your window manager ignore the window, and
 *   any focusing has to be done manually. Note that the window will exist on all desktops.
 *  #define ANXWIN_BARE
 *
 *   Center the window when initially mapped
 *  #define ANXWIN_CENTER 
 *
 *   Create an window in fullscreen mode
 *  #define ANXWIN_FULLSCREEN 
 *
 *   Convenience flag to automatically enable closing windows by hitting the escape key.
 *   Works with deletenow()/Finalize()
 *  #define ANXWIN_ESCAPABLE 
 *
 *   Any window with this set is slated for destruction, and should not be messed with in any way.
 *  #define ANXWIN_DOOMED 
 *
 *   Set up a window to be double buffered. This by default will use the X Double Buffer Extension
 *   to create the buffer, which can be swapped win SwapBuffers().
 *  #define ANXWIN_DOUBLEBUFFER
 *
 *   If this window is a top level window, and the mouse is clicked down somewhere outside the window
 *   and outside of any connected top level controls, then destroy this window and its connected top
 *   level controls.
 *  #define ANXWIN_OUT_CLICK_DESTROYS
 * \endcode
 *
 *  \todo implement ANXWIN_BARE to have no decorations, but move with the desktop?
 */
/*! \fn int anXWindow::Needtodraw(int nntd)
 * \brief If 0, then the window does not need refreshing. Otherwise it does.
 *
 * My hat's off to Fltk for using overloaded functions to either set or retrieve
 * values! What a great idea!
 */
/*! \fn void anXWindow::Needtodraw()
 * \brief Default is to return needtodraw.
 *
 * This function is consulted during the event loop, during the refresh and idle stage.
 * If it returns nonzero, then Refresh() is called.
 */
/*! \fn void anXWindow::Refresh()
 * \brief anXWindow::Refresh() is an empty placeholeder. Just calls Needtodraw(0) and returns 0.
 *
 * Any window appearance updating is done here. This is called only when there are no more
 * events in anXApp's event queue.
 */
/*! \fn int anXWindow::Idle(int tid, double delta)
 * \brief anXWindow::Idle() is an empty placeholeder. Just returns 1.
 *
 * This function will be called if there is a timer that ticks off. The 
 * timer's id is passed as tid. delta is the time in seconds since the last tick.
 *
 * If Idle returns 1, then the timer is removed. If Idles returns
 * 0, the timer is left active. This makes it very easy to turn
 * off any residual mouse timers with a check to see if any buttons are still down, and if so, return 1.
 */
/*! \var char anXWindow::win_on
 * \brief Nonzero if the window is mapped.
 */
/*! \var char anXWindow::win_active
 * \brief Should be positive when the window has a keyboard focus, 0 otherwise.
 */
/*! \var RefPtrStack<anXWindow> anXWindow::_kids
 * \brief Stack of children of the window.
 *
 *  Consider this to be readonly. Class anXApp maintains it by 
 *  pushing during addwindow, anXWindow should not access _kids directly.
 *  anXWindow is ultimately responsible for destruction of child windows via deletekid()
 *  which is called by anXApp. The app is responsible for destruction of anxwindow->xlib_window
 *  and topwindows. These things are typically done without extra user intervention,
 *  as long as the normal app->addwindow/app->destroywindow system is used.
 */
/*! \fn char *anXWindow::win_sendthis
 * \brief The type of message that gets sent to owner.
 */
/*! \fn char *anXWindow::win_title
 * \brief The title of the window.
 *
 * If the window is a top level window, then this is the text that is displayed by the window manager.
 */
/*! \fn char *anXWindow::win_name
 * \brief An arbitrary string to be used as an id.
 *
 * This can be used by programmers to help keep track of windows rather than win_title, since
 * win_title is most likely to change.
 */
/*! \fn int anXWindow::init()
 * \brief Empty placeholeder. Just returns 0.
 *
 * When anXApp::addwindow() is called with this window, init() is called after
 * the window is allocated (in Xlib, this is with XCreatWindow()),
 * but before the window is mapped (made visible). Normally, windows will create
 * and install child windows via app->addwindow() from init(),
 * since the anXWindow will now have a valid allocation.
 *
 * The following is relevant for Laxkit using Xlib.
 *
 * Also, if a window wants to set anything in win_hints or win_sizehints, they should do
 * so here or before init() somewhere. anXApp::addwindow() sets all other relevant hints after
 * this init returns, and before the window is mapped. Setting the win_sizehints with USPosition and
 * USSize causes the window to be popped up in a definite place, rather than thrown about 
 * willy nilly by the window manager.
 *
 * As a convenience, if init() returns 1 then it is assumed that win_w and/or win_h are not
 * the same as before init() was called. anXApp::addwindow() then sets up win_sizehints 
 * accordingly, before the window is mapped. If the
 * window has ANXWIN_CENTER set, then the x and y in win_sizehints are also set accordingly,
 * and win_x and win_y are set to the recomputed x and y.
 */

/*! \fn int anXWindow::KeyUp(unsigned int ch,unsigned int state, const LaxMouse *kb)
 * \brief Called when a key is released.
 *
 * See anXWindow::CharInput() for what the ch actually corresponds to. This function is
 * different than CharInput() in that there is no composed character passed in. One would
 * use this function to check whether a shift (LAX_Shift) or control (LAX_Control)
 * key is released, for instance.
 */
/*! \var char *anXWindow::win_tooltip
 * \brief Convenience variable to hold the window's tooltip, if any.
 *
 * The app retrieves win_tooltip with a call to tooltip(). It does not access win_tooltip directly
 */
/*! \fn int anXWindow::ExposeChange(ScreenEventData *e)
 * \brief Default behavior on Expose events is to call Needtodraw(1).
 *
 * For smart refreshing, windows would redefine this to record which areas
 * of the window need to be updated. Imlib2, by the way, provides for this via imlib_updates.
 */
/*! \fn int anXWindow::deletenow()
 * \brief Return whether the window is allowed to be deleted.
 * 
 * Return 1 means ok to unmap AND delete,
 * Return 2 means unmap only.
 * Return 3 to neither unmap nor delete.
 *
 * This function is called during processing of the delete protocol
 */
/*! \fn int anXWindow::MouseMove(int x,int y,unsigned int state, const LaxMouse *m)
 * \brief Empty placeholder, just returns 1.
 */
/*! \fn unsigned long anXWindow::win_owner
 * \brief Who gets control messages from this window.
 */

/*! \fn int anXWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
 * \brief Default is just to return 1. 
 *
 * Count is usually 1, but double click is 2, triple click==3, etc.
 */
/*! \fn int anXWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
 * \brief Default is just to return 1.
 */
/*! \fn int anXWindow::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
 * \brief Default is just to return 1.
 *
 * Count is usually 1, but double click is 2, triple click==3, etc.
 */
/*! \fn int anXWindow::MBUp(int x,int y,unsigned int state,const LaxMouse *d)
 * \brief Default is just to return 1.
 */
/*! \fn int anXWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
 * \brief Default is just to return 1.
 *
 * Count is usually 1, but double click is 2, triple click==3, etc.
 */
/*! \fn int anXWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
 * \brief Default is just to return 1.
 */
/*! \fn int anXWindow::WheelUp(int x,int y,unsigned int state,int count, const LaxMouse *d)
 * \brief The wheel mouse rolling up. Default is just to return 1.
 */
/*! \fn int anXWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
 * \brief The wheel mouse rolling down. Default is just to return 1.
 */
 /*! \var int anXWindow::win_pointer_shape
  * \brief Identifier for a stock mouse shape.
  *
  * Currently, if this value is less than 200, then try to use an X cursor based on that number,
  * with 0 being the default shape.
  *
  * See laxmouseshapes.h for other values.
  */



//! Constructor.
/*! Just assigns all the passed in variables to their given values,
 * and everything else to 0/nullptr.
 *
 * <pre>
 *  Constructor, appends itself to the (incomplete) tab loop in prev.
 *  Later, it is assumed that that loop in prev is not yet a closed loop, ie that
 *  some prev->prev->... is nullptr.
 * </pre>
 *
 */


/*! Note that parnt gets assigned to win_parent, but no checking for existence in _kids
 * is done here. The check for that happens in anXApp::addwindow().
 */
anXWindow::anXWindow(anXWindow *parnt, const char *nname, const char *ntitle,
					unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder,
					anXWindow *prev, //!< prevcontrol gets set to this.
					unsigned long nowner, //!< Who gets control messages from this window
					const char *nsend //!< The control message to send
				)
{
	app = anXApp::app;

	win_screen = -1;
	win_on     = 0;
	win_active = 0;
	needtodraw = 1;
	win_parent = parnt;
	win_x      = xx;
	win_y      = yy;
	win_w      = ww;
	win_h      = hh;
	win_border = brder;
	win_style  = nstyle;
	win_pointer_shape = 0;
	win_cur_uiscale = -1;

	win_tooltip = nullptr;
	win_title   = newstr(ntitle);
	win_name    = newstr(nname);

	 //set window ownership and message
	win_owner = nowner; 
	win_owner_send_mask = 0;
	if (nsend) win_sendthis = newstr(nsend); else win_sendthis = nullptr;
	nextcontrol = nullptr; 
	prevcontrol = nullptr;
	if (prev) prev->ConnectControl(this,1);

	win_theme = nullptr;
	win_themestyle = nullptr;

#ifdef _LAX_PLATFORM_XLIB
	 //set up Xlib specific stuff
	xlib_dnd = nullptr;
	xlib_win_hints = nullptr;
	xlib_win_sizehints = nullptr;
	xlib_win_xattsmask = 0;
	xlib_win_xatts.event_mask = 0;
	xlib_win_xatts.border_pixel = 0; //app->color_inactiveborder;
	xlib_win_xattsmask |= CWEventMask|CWBorderPixel; // set to 0 in anXWindow
	xlib_win_xatts.event_mask |= StructureNotifyMask|ResizeRedirectMask|ExposureMask|VisibilityChangeMask;
#endif //_LAX_PLATFORM_XLIB
}


//! anXWindow destructor. Its X window should have been XDestroy'd before here.
/*! App->destroywindow() must be called for this window for the X window to
 *  be properly destroyed.  The anXApp::destroyqueued() provokes the actual deleting by calling 
 *  win->deletekid(whateverwindow).
 *
 *  If nextcontrol or prevcontrol exist, then take this out of that control loop by connecting
 *  those two.
 */
anXWindow::~anXWindow()
{
	DBG cerr << " in anxwindow("<<WindowTitle()<<") destructor."<<endl;

#ifdef _LAX_PLATFORM_XLIB
	if (xlib_dnd) delete xlib_dnd;
	if (xlib_win_hints) XFree(xlib_win_hints);
	if (xlib_win_sizehints) XFree(xlib_win_sizehints);
#endif //_LAX_PLATFORM_XLIB

	if (win_themestyle) win_themestyle->dec_count();

	if (win_tooltip) delete[] win_tooltip;

	//flush kids manually: don't really need to do this here, but doing it here to make debugging easier
	DBG int t=_kids.n;
	DBG cerr <<"anXWindow "<<WindowTitle()<<" flushing "<<t<<" kids..."<<endl;
	for (int c=0; _kids.n; c++) {
		DBG cerr <<"remove window "<<_kids.e[0]->whattype()<<","<<
		DBG    (_kids.e[0]->WindowTitle())<<" "<<c+1<<"/"<<t<<endl;
		_kids.remove(0);
	}

	if (win_sendthis) delete[] win_sendthis;
	if (win_name) delete[] win_name;
	if (win_title) delete[] win_title;
	if (win_theme) win_theme->dec_count();

	if (nextcontrol) { nextcontrol->prevcontrol=prevcontrol; }
	if (prevcontrol) { prevcontrol->nextcontrol=nextcontrol; } 
}

/*! If win_border == -1, return theme->border_width.
 */
int anXWindow::WindowBorder()
{
	if (win_border >= 0) return win_border;
	if (win_themestyle && win_themestyle->border_width >= 0) return win_themestyle->border_width;
	return 0;
}

//! Return basically the name of the window.
/*! If which==0, then return win_title, or win_name if win_title==nullptr, or "(untitled)" if win_name==nullptr.
 * If which==1, return win_name, or win_title if win_name==nullptr, or "(unnamed)" if win_title==nullptr.
 * If which==2, return win_name, or nullptr if name is nullptr.
 * If which==3, return win_title, or nullptr if title is nullptr.
 */
const char *anXWindow::WindowTitle(int which)
{
	if (which==0) return win_title?win_title:(win_name?win_name:"(untitled)");
	if (which==1) return win_name?win_name:(win_title?win_title:"(unnamed)");
	if (which==2) return win_name;
	if (which==3) return win_title;
	return nullptr;
}

//! Change the title of the window. This text would usually be displayed in the bar provided by a window manager.
void anXWindow::WindowTitle(const char *newtitle)
{
#ifdef _LAX_PLATFORM_XLIB
	if (xlib_window) XStoreName(app->dpy,xlib_window,newtitle);
#endif //_LAX_PLATFORM_XLIB

	makestr(win_title,newtitle);
}


//-------------------------style functions

//! Control various window related basic styling of win_style.
/*! The default is to set and unset style bit in win_style.
 *  Warning: do not randomly call this
 *  function unless you know what you're doing, as most predefined windows have specific ways
 *  to manage win_style.
 *
 * Return 0 for style updated, and 1 for not.
 *
 * \todo must rethink window styling to make it more reasonable and adaptable.
 *    maybe better to have stylebit be more like an enum, so as to lock style types in
 *    laxkit namespace, rather then have a million goofy defines. It would then be up to
 *    the window to interperet the newvalue and set accordingly.
 */
int anXWindow::SetWinStyle(unsigned int stylebit, int newvalue)
{
	//if (stylebit&ANXWIN_MASK) return 1;
	if (newvalue) win_style |= stylebit;
	else win_style &= ~stylebit;
	return 0;
}

//! Currently, simply return win_style&stylebit.
/*! \todo like SetWinStyle(), this is rather limited and needs further thought...
 */
int anXWindow::HasWinStyle(unsigned int stylebit)
{
	return win_style&stylebit;
}


/*! category must be one on THEME_Panel, THEME_Edit, THEME_Button, THEME_Menu, or THEME_Tooltip, otherwise nothing done. 
 * This will install from the default theme (app->theme). Dec_count old and inc_count new.
 * Return true if changed, else false.
 */
bool anXWindow::InstallColors(int category)
{
	if (!app->theme) return false;
	WindowStyle *style = app->theme->GetStyle(category);
	if (!style) return false;
	if (style) style->inc_count();
	if (win_themestyle) win_themestyle->dec_count();
	win_themestyle = style;
	return true;
}

//! Dec_count old and inc_count new. If null, then remove old and replace with nothing.
void anXWindow::InstallColors(WindowStyle *newcolors)
{
	if (!newcolors) return;
	if (newcolors) newcolors->inc_count();
	if (win_themestyle) win_themestyle->dec_count();
	win_themestyle = newcolors;
}

/*! This is called automatically when the theme changes. Note that the theme object
 * might be the same as before, but windows should assume some aspect of it has changed which requires
 * updating ui components.
 *
 * ThemeChanged() will also be called on all children.
 * 
 * Default is to replace the current style with the WindowStyle in theme of the same category.
 * This will call InstalColors() with the new style of the same category as the old style.
 * If a new style of the same category is not found, nothing is done.
 */
void anXWindow::ThemeChanged()
{
	win_cur_uiscale = -1;
	for (int c=0; c<_kids.n; c++) {
	    _kids.e[c]->ThemeChanged();
	}

	if (win_themestyle) {
		// make sure to use the same category of theme components as the previous theme
		WindowStyle *newstyle = GetTheme()->GetStyle(win_themestyle->category);
		if (newstyle) {
			InstallColors(newstyle);
		}
	}
	UIScaleChanged();
	needtodraw = 1;
}

/*! Install a custom theme for the window. Its count will be incremented.
 * Pass in nullptr to use the default theme.
 */
void anXWindow::CustomTheme(Theme *new_theme)
{
	if (new_theme) new_theme->inc_count();
	if (win_theme) win_theme->dec_count();
	win_theme = new_theme;
	ThemeChanged();
}

/*! Return the first defined of win_theme, parent's theme, or app->theme.
 */
Theme *anXWindow::GetTheme() const
{
	if (win_theme) return win_theme;
	if (win_parent) return win_parent->GetTheme();
	return app->theme;
}

/*! Called when there is an ui scale change. Default here is to relay down to _kids.
 * Subclasses should redefine if they need to adjust ui elements.
 */
void anXWindow::UIScaleChanged()
{
	win_cur_uiscale = -1;
	for (int c=0; c<_kids.n; c++) {
		_kids.e[c]->UIScaleChanged();
	}
	needtodraw = 1;
}

/*! This is called when ui scale is changed for any reason. For instance, the theme may have
 * changed, or window scale is adapting to a monitor with a different scale.
 * 
 * Return win_cur_uiscale if > 0, else normally will be win_uiscale * win_parent->UIScale() * theme->ui_scale.
 * The returned value will never be <= 0.
 */
double anXWindow::UIScale()
{
	if (win_cur_uiscale > 0) return win_cur_uiscale;

	double scale = 1.0; //(win_uiscale > 0 ? win_uiscale : 1.0);

	double theme_scale = GetTheme()->ui_scale;

	const char *str = nullptr;
	if (theme_scale <= 0) {
		//theme -1 scale means use window/monitor hints
		//use default gleaned from GTK_SCALE or QT_SCREEN_SCALE_FACTOR, or 1 if neither of those exist
		DBG const char *what = "GDK_SCALE";
		str = getenv("GDK_SCALE");
		if (!str) {
			DBG what = "QT_SCALE_FACTOR";
			str = getenv("QT_SCALE_FACTOR");
		}
		if (str) {
			double s = strtod(str, nullptr);
			if (s > 0) {
				scale *= s;
				DBG cerr << "Using "<<what<<" == "<<s<<endl;
			}
		}
	} else {
		scale *= theme_scale;
	}

	//scale by monitor pixel density
	str = getenv("QT_AUTO_SCREEN_SCALE_FACTOR"); //this should be "0" or "1"
	if (GetTheme()->ui_default_ppi > 0 || (str && str[0] == '1')) {
		int xx, yy;
		screen_coordinates(win_x + win_w/2, win_y + win_h/2, this, &xx, &yy);
		ScreenInformation *monitor = app->FindNearestMonitor(0, xx,yy);
		if (monitor) {
			double mm = sqrt(monitor->mmwidth * monitor->mmwidth + monitor->mmheight * monitor->mmheight);
			if (mm > 0) {
				double ppi = GetTheme()->ui_default_ppi;
				if (ppi <= 0) ppi = 100;
				double px = sqrt(monitor->width * monitor->width + monitor->height * monitor->height);
				double px_per_in = px / mm * 10 * 2.54;
				DBG cerr << "Using monitor pixel density "<<px_per_in<<", extra scale="<<(px_per_in / ppi)<<" for " <<(win_name?win_name:"?")<<endl;
				scale *= px_per_in / ppi;
			}
		}
	}

	win_cur_uiscale = scale;
	for (int c=0; c<_kids.n; c++) {
		_kids.e[c]->win_cur_uiscale = -1;
		_kids.e[c]->UIScale();
	}

	return win_cur_uiscale;
}

//! Return a ShortcutHandler that contains stacks of bound shortcuts and possible window actions.
/*! nullptr means there are none defined for this window.
 *
 * Windows that do use shortcuts, and want them stored in an easy manner should use the
 * ShortcutManager system, accessed with GetDefaultShortcutManager(). They can then install
 * a single list of shortcuts and actions bound to the window type name, and any future window 
 * instances can borrow those.
 */
ShortcutHandler *anXWindow::GetShortcuts()
{ return nullptr; }

/*! This method exists to aid standardizing access to shortcut actions from potential scripting.
 * Return 1 for not found or otherwise not done, or 0 for success.
 * Default is return 1.
 */
int anXWindow::PerformAction(int action_number)
{ return 1; }




//---------------------------- Dump functions

//! Simple dumping function.
/*! Default is to dump out an Attribute retrieved from dump_out_atts(),
 * so subclasses need only redefine dump_out_atts() and dump_in_atts()
 * for all the DumpUtility functions to work.
 */
void anXWindow::dump_out(FILE *f,int indent,int what,DumpContext *context)
{
	Attribute *att=dump_out_atts(nullptr,0,context);
	att->dump_out(f,indent);
	delete att;
}

/*! Append to att if att!=nullptr, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for win_x, win_y, win_w, and win_h.
 *
 * \todo maybe someday have option to overwrite existing attributes in att,
 *   but this opens up potential problems due to possible nesting. How deep
 *   would the replace function?
 */
Attribute *anXWindow::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (!att) att=new Attribute(whattype(),nullptr);
	char scratch[100];

	sprintf(scratch,"%d",win_x);
	att->push("win_x",scratch);

	sprintf(scratch,"%d",win_y);
	att->push("win_y",scratch);

	sprintf(scratch,"%d",win_w);
	att->push("win_w",scratch);

	sprintf(scratch,"%d",win_h);
	att->push("win_h",scratch);

	scratch[0]='\0';
	if (win_style&ANXWIN_GRAYED      ) strcat(scratch,"grayed ");
	if (win_style&ANXWIN_REMEMBER    ) strcat(scratch,"remember ");
	if (win_style&ANXWIN_XDND_AWARE  ) strcat(scratch,"dndaware ");
	if (win_style&ANXWIN_BARE        ) strcat(scratch,"bare ");
	if (win_style&ANXWIN_CENTER      ) strcat(scratch,"center ");
	if (win_style&ANXWIN_FULLSCREEN  ) strcat(scratch,"fullscreen ");
	if (win_style&ANXWIN_ESCAPABLE   ) strcat(scratch,"escapable ");
	if (win_style&ANXWIN_DOUBLEBUFFER) strcat(scratch,"doublebuffer ");
	//add: out_click_destroys
	if (scratch[0]!='\0') {
		att->push("win_flags",scratch);
	}

	 //output indicator for defaults
	if      (win_themestyle->category == THEME_Panel)   att->push("win_themestyle","default_panel");
	else if (win_themestyle->category == THEME_Menu)    att->push("win_themestyle","default_menu");
	else if (win_themestyle->category == THEME_Edit)    att->push("win_themestyle","default_edits");
	else if (win_themestyle->category == THEME_Button)  att->push("win_themestyle","default_buttons");
	else if (win_themestyle->category == THEME_Tooltip) att->push("win_themestyle","default_tooltip");
	// else custom colors, which subclasses should output...

	// *** fonts

	// *** event conditions?

	return att;
}

/*! Default is to read in win_x, win_y, win_w, win_h, win_tooltip, win_flags.
 *
 * \todo  at some point, must ensure that the dimensions read in are in part on screen..
 *    though this is really an anXApp::addwindow() task.
 */
void anXWindow::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"win_x")) {
			IntAttribute(value,&win_x);
		} else if (!strcmp(name,"win_y")) {
			IntAttribute(value,&win_y);
		} else if (!strcmp(name,"win_w")) {
			IntAttribute(value,&win_w);
		} else if (!strcmp(name,"win_h")) {
			IntAttribute(value,&win_h);
		} else if (!strcmp(name,"win_tooltip")) {
			tooltip(value);
		} else if (!strcmp(name,"win_flags")) {
			int n=0;
			char **flags=splitspace(value,&n);
			for (int c2=0; c2<n; c2++) {
				if (!strcmp(flags[c2],"grayed"      )) SetWinStyle(ANXWIN_GRAYED      ,1);
				if (!strcmp(flags[c2],"remember"    )) SetWinStyle(ANXWIN_REMEMBER    ,1);
				if (!strcmp(flags[c2],"dndaware"    )) SetWinStyle(ANXWIN_XDND_AWARE  ,1);
				if (!strcmp(flags[c2],"bare"        )) SetWinStyle(ANXWIN_BARE        ,1);
				if (!strcmp(flags[c2],"center"      )) SetWinStyle(ANXWIN_CENTER      ,1);
				if (!strcmp(flags[c2],"fullscreen"  )) SetWinStyle(ANXWIN_FULLSCREEN  ,1);
				if (!strcmp(flags[c2],"escapable"   )) SetWinStyle(ANXWIN_ESCAPABLE   ,1);
				if (!strcmp(flags[c2],"doublebuffer")) SetWinStyle(ANXWIN_DOUBLEBUFFER,1);
			}
			deletestrs(flags,n);
		} else if (!strcmp(name,"win_flags")) {
			if (!isblank(value)) {
	 			 //input indicator for defaults
				if      (!strcmp(value,"default_panel"))   InstallColors(THEME_Panel);
				else if (!strcmp(value,"default_menu"))    InstallColors(THEME_Menu);
				else if (!strcmp(value,"default_edits"))   InstallColors(THEME_Edit);
				else if (!strcmp(value,"default_buttons")) InstallColors(THEME_Button);
				else if (!strcmp(value,"default_tooltip")) InstallColors(THEME_Tooltip);
				// else custom colors, which subclasses should input...
			}
		}
	}
}


//-----------------------core functions

//! Initialize the backbuffer.
/*! This is a convenience function for windows that have ANXWIN_DOUBLEBUFFER set.
 * If that is not set in win_style, then no back buffer is set up.
 *
 * By default, this will be called by anXApp after it calls anXWindow::init().
 * As long as ANXWIN_DOUBLEBUFFER is set, then normally windows do not need to call this themselves.
 *
 * All it does is this:\n
 * <tt>xlib_backbuffer=XdbeAllocateBackBufferName(app->dpy,window,XdbeBackground);</tt>
 */
void anXWindow::SetupBackBuffer()
{
	if (!(win_style&ANXWIN_DOUBLEBUFFER)) return;

#ifdef _LAX_PLATFORM_XLIB
	if (xlib_backbuffer) return;
	xlib_backbuffer=XdbeAllocateBackBufferName(app->dpy,xlib_window,XdbeBackground);
#endif //_LAX_PLATFORM_XLIB
}

/*! Swap buffers. This should be called from Refresh() if ANXWIN_BACKBUFFER is set in win_style.
 * Default is to call <tt>XdbeSwapBuffers(app->dpy,&swapinfo,1)</tt>
 * with <tt>swapinfo.swap_window=window</tt> and <tt>swapinfo.swap_action=XdbeBackground</tt>.
 * If xlib_backbuffer==0, then nothing is done.
 */
void anXWindow::SwapBuffers()
{
#ifdef _LAX_PLATFORM_XLIB
	 // swap buffers
	if (xlib_backbuffer) {
		XdbeSwapInfo swapinfo;
		swapinfo.swap_window=xlib_window;
		swapinfo.swap_action=XdbeBackground;
		XdbeSwapBuffers(app->dpy,&swapinfo,1);
	}
#endif //_LAX_PLATFORM_XLIB
}

//! Replace the current tooltip, return the current tooltip (after replacing).
/*! If tooltips are active, the anXApp calls tooltip() to find the tip for this window.
 * Thus, the window can redefine this function if there are multiple tooltips for some reason.
 *
 * Note that calling tooltip(nullptr) will remove the tooltip.
 */
const char *anXWindow::tooltip(const char *newtooltip)
{
	return makestr(win_tooltip,newtooltip);
}

//! By default, return win_tooltip.
/*! By default, mouseid is ignored. Derived classes may redefine this function to take into account
 * which mouse is requesting a tooltip.
 */
const char *anXWindow::tooltip(int mouseid)
{
	return win_tooltip;
}

/*! When anXApp::addwindow() is called with this window, preinit() is called before
 * the window is allocated (in xlib, this is with XCreatWindow()).
 * Derived classes would redefine this to do some
 * last minute tinkering before the window is allocated. Windows get another chance
 * to do tinkering when init() is called after allocating, but before being made visible 
 * (aka mapped).
 *
 * In win_style&ANXWIN_REMEMBER and win_w and/or win_h==0, then look up the anXApp::AppResource() corresponding
 * to this->whattype(), and set the win_x,y,w,h to the corresponding values.
 *
 * The return value is currently ignored in the default anXApp, but 0 should mean success.
 * 
 * Just for reference, the following code can be used to make a window get mapped initially
 * to a particular place on the screen. This is basically what is done with windows
 * that have ANXWIN_CENTER set in win_style.
 * \code
 *  // adjusting the x/y and flags|=USPosition or PPosition might make the window open properly??
 *  // check this:
 *  // USPosition/USSize means the user wants it this way, and thats that.
 *  //		PPosition/PSize means the client set it without user involvement?? what does that mean?
 *  if (!xlib_win_sizehints) xlib_win_sizehints=XAllocSizeHints();
 *  if (xlib_win_sizehints && !w->win_parent) {
 *  	xlib_win_sizehints->x=win_x;
 *  	xlib_win_sizehints->y=win_y;
 *  	xlib_win_sizehints->width=win_w;
 *  	xlib_win_sizehints->height=win_h;
 *  	xlib_win_sizehints->min_width=10;
 *  	xlib_win_sizehints->max_width=500;
 *  	xlib_win_sizehints->min_height=10;
 *  	xlib_win_sizehints->max_height=500;
 *  	xlib_win_sizehints->width_inc=1; //pixel increments to allow window resizing
 *  	xlib_win_sizehints->height_inc=1;
 *  	xlib_win_sizehints->flags=PMinSize|PResizeInc|USPosition|USSize;
 *  	//XSetWMNormalHints(dpy,win,win_sizehints); this is called in addwindow, before mapping
 *  }
 * \endcode
 * 
 * \todo must incorporate _NET_FRAME_EXTENTS with ANXWIN_REMEMBER
 */
int anXWindow::preinit()
{
	//if ((win_style&ANXWIN_REMEMBER) && (win_h<=0 || win_w<=0)) {
	if ((win_style&ANXWIN_REMEMBER)) {
		DBG cerr << "Remembering settings for " << whattype() <<endl;
		Attribute *att=const_cast<Attribute *>(app->AppResource(whattype()));//do not delete it!
		if (att) {
			dump_in_atts(att,0,nullptr);

#ifdef _LAX_PLATFORM_XLIB
			if (!xlib_win_sizehints) xlib_win_sizehints=XAllocSizeHints();
			if (xlib_win_sizehints) {
				DBG cerr <<"doing win_sizehintsfor"<<WindowTitle()<<endl;

				//*** The initial x and y become the upper left corner of the window
				//manager decorations. ***how to figure out how much room those decorations take,
				//so as to place things on the screen accurately? like full screen view?
				//_NET_FRAME_EXTENTS can be queried, but only after a window is created? mapped?
				xlib_win_sizehints->x=win_x;
				xlib_win_sizehints->y=win_y;
				xlib_win_sizehints->width=win_w;
				xlib_win_sizehints->height=win_h;
				xlib_win_sizehints->flags=USPosition|USSize;
				//--if you want it super fancy, you can do the following, for instance:
				//xlib_win_sizehints->min_width=10;
				//xlib_win_sizehints->max_width=500;
				//xlib_win_sizehints->min_height=10;
				//xlib_win_sizehints->max_height=500;
				//xlib_win_sizehints->width_inc=1;
				//xlib_win_sizehints->height_inc=1;
				//xlib_win_sizehints->flags=PMinSize|PResizeInc|USPosition|USSize;
			}
#endif //_LAX_PLATFORM_XLIB
		}
	}
	return 0;
}

//! Called by anXApp from anXApp::destroywindow() when a window is to be destroyed.
/*! This function is called before the Xlib window is destroyed, so windows can do any
 * specific cleanup that still depends on that window value here.
 *
 * If win_style&ANXWIN_REMEMBER, then call app->AppResource() with a new Attribute
 * found via dump_out_atts().
 *
 * The Laxkit does not do anything with the default return value, but 0 should mean success.
 *
 * Each child in _kids will also have child->Finalize() called.
 */
int anXWindow::Finalize()
{
	if (win_style&ANXWIN_REMEMBER) {
		Attribute *att = dump_out_atts(nullptr,0,nullptr);
		if (att) app->AppResource(att); //do not delete att!
	}
	for (int c=0; c<_kids.n; c++) _kids.e[c]->Finalize();
	return 0;
}

/*! Find the child window that contains this coordinate, which is in *this's coordinate space.
 * Child window must be on, and within parent window.
 */
anXWindow *anXWindow::findContainingChild(int x, int y)
{
	if (!win_on) return nullptr;
	if (x < 0 || x >= win_w || y < 0 || y >= win_h) return nullptr; //outside window
	for (int c=0; c<_kids.n; c++) {
		anXWindow *ret = _kids.e[c]->findContainingChild(x - _kids.e[c]->win_x - _kids.e[c]->win_border,
														 y - _kids.e[c]->win_y - _kids.e[c]->win_border);
		if (ret) return ret;
	}
	return this;
}

anXWindow *anXWindow::WindowChild(int index)
{
	if (index < 0 || index >= _kids.n) return nullptr;
	return _kids.e[index];
}

//! Find the first immediate child window that has win_name==name.
/*! If name==nullptr, nullptr is returned, not the first window that has a nullptr name.
 *
 * If win_name==nullptr, then win_title is compared against instead.
 *
 * Note that this does not check kids of child windows.
 */
anXWindow *anXWindow::findChildWindowByName(const char *name, bool recurse)
{
	if (!name) return nullptr;
	const char *s=nullptr;
	for (int c=0; c<_kids.n; c++) {
		s=_kids.e[c]->win_name ? _kids.e[c]->win_name : _kids.e[c]->win_title;
		if (s && !strcmp(name,s)) return _kids.e[c];

		if (recurse) {
			anXWindow *win=_kids.e[c]->findChildWindowByName(name,true);
			if (win) return win;
		}
	}
	return nullptr;
}

//! Find the first immediate child window that has win_title==title.
/*! If title==nullptr, nullptr is returned, not the first window that has a nullptr title.
 *
 * Note that this does not check kids of child windows.
 *
 * This will only check against win_title.
 * See also findChildWindowByName().
 */
anXWindow *anXWindow::findChildWindowByTitle(const char *title, bool recurse)
{
	if (!title) return nullptr;
	const char *s=nullptr;
	for (int c=0; c<_kids.n; c++) {
		s=_kids.e[c]->win_title;
		if (s && !strcmp(title,s)) return _kids.e[c];

		if (recurse) {
			anXWindow *win=_kids.e[c]->findChildWindowByTitle(title,true);
			if (win) return win;
		}
	}
	return nullptr;
}

//! Purges child anXWindows from window's child stack.
/*! You should not call this function directly. 
 * This normally is called from anXApp::destroyqueued. anXApp is responsible for
 *  maintaining anXWindow's _kids stack.
 * 
 * Removes only if w is a direct kid, not grand kid or more.
 */
int anXWindow::deletekid(anXWindow *w)
{
	int c=_kids.findindex(w);
	if (c<0) return 1;
	_kids.remove(c);
	return 0;
}

//! Return whether this window is grayed.
/*! If this window does not have gray set, but an ancestor window does, then
 * it is considered gray. The return value is the level back one must go before
 * the next grayed window. 1==this window, 2==win_parent is grayed, and so on.
 *
 * 0 means not grayed.
 */
int anXWindow::Grayed()
{
	if (win_style&ANXWIN_GRAYED) return 1;
	if (win_parent) {
		int c=win_parent->Grayed();
		if (c) return c+1;
	}
	return 0; 
}

//! Set the gray state of this window. Returns Grayed(void).
int anXWindow::Grayed(int g)
{
	if (g && !(win_style&ANXWIN_GRAYED)) {
		win_style|=ANXWIN_GRAYED; 
		needtodraw=1;
	} else if (!g && (win_style&ANXWIN_GRAYED)) {
		win_style=(win_style&~ANXWIN_GRAYED); 
		needtodraw=1;
	}
	return Grayed(); 
}

bool anXWindow::IsFullscreen()
{
	#ifdef _LAX_PLATFORM_XLIB
	if (!app || !app->dpy) return false;
	unsigned long *data = nullptr;
	unsigned long nret = 0;
	Atom actual_type_ret = 0;
	int format = 0;
	unsigned long leftover = 0;
	int status = XGetWindowProperty(app->dpy, xlib_window, _NET_WM_STATE, 0, 64, False,
					AnyPropertyType, /* req_type */
					&actual_type_ret,
					&format,
					&nret,
					&leftover,
					(unsigned char **)(&data));
	if (status != Success) return false;
	for (unsigned long c = 0; c < nret; c++) {
		if (data[c] == _NET_WM_STATE_FULLSCREEN) return true;
	}
	return false;
	#else
	return false;
	#endif	
}

/*! Set fullscreen or not.
 * If the window is not toplevel, nothing happens.
 * Return 0 for success or 1 for error.
 */
int anXWindow::Fullscreen(bool yes)
{
	if (win_parent) return -1;

	#ifdef _LAX_PLATFORM_XLIB
	//XChangeProperty(app->dpy, xlib_window, _NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char*)&_NET_WM_STATE_FULLSCREEN, yes); <- set prop, but no fs
	
	if (!app->dpy) return 1;
	XEvent xev;
    long evmask = SubstructureRedirectMask | SubstructureNotifyMask;

    xev.type = ClientMessage;
    xev.xclient.window = xlib_window;
    xev.xclient.message_type = _NET_WM_STATE;
    xev.xclient.format = 32;

    // xev.xclient.data.l[0] = _NET_WM_STATE_TOGGLE;
    // xev.xclient.data.l[1] = _NET_WM_STATE_FULLSCREEN;
    //-----
    xev.xclient.data.l[0] = yes ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
    xev.xclient.data.l[1] = _NET_WM_STATE_FULLSCREEN;

    xev.xclient.data.l[2] = 0;  /* no second property to toggle */
    xev.xclient.data.l[3] = 1;  /* source indication: 1 means normal application */
    xev.xclient.data.l[4] = 0;  /* unused */

    if(!XSendEvent(app->dpy, DefaultRootWindow(app->dpy), 0, evmask, &xev)) {
        return 2;
    }
	return 0;
	#else
	return 1;
	#endif
}

//! Increment win_active, and highlights the window's border, if the event refers to this window.
/*! The default here is to activate for any focus on from any keyboard.
 *
 *  If it is an actual focus on, then app->xim_ic is updated for this window.
 *  Also the border color is changed to app->color_activeborder.
 *
 *  \todo *** update this for multiple keyboard inputs.. right now, there is only one x input context
 *    at any one time, stored in anXApp::xim_ic. really I don't understand x input contexts, or
 *    any more modern way of doing it, so until then, this'll have to do.
 */
int anXWindow::FocusOn(const FocusChangeData *e)
{
	 // set win_active stuff only if this is a FocusIn event for this window
	if (e->target==this) {
		win_active++;

#ifdef _LAX_PLATFORM_XLIB
		xlib_win_xatts.border_pixel = (win_themestyle ? win_themestyle->active_border.Pixel() : 0);
		XChangeWindowAttributes(app->dpy,xlib_window,CWBorderPixel,&xlib_win_xatts);
		DBG cerr <<WindowTitle()<<": real focus on"<<endl;
 
		XIC xim_ic=app->CreateXInputContext(); //<--returns anXApp::xim_ic
		if (xim_ic) {
			XSetICValues(app->xim_ic,
						XNClientWindow, e->target->xlib_window,
						NULL); //note: using nullptr throws missing sentinel warning
			XSetICFocus(xim_ic);
		}
#endif //_LAX_PLATFORM_XLIB

	} else {
		DBG cerr <<WindowTitle()<<": focuson doesn't refer to this window"<<endl;
	}

	DBG cerr <<"(typ)Focus on "<<WindowTitle()<<endl;
	return 0;
}

//! Decrements win_active, and de-highlights the window's border if win_active==0, if event is a real focus off.
/*! 
 */
int anXWindow::FocusOff(const FocusChangeData *e)
{
	if (e->target==this) {
		win_active--;
		if (win_active<0) win_active=0; //kludge to cover up not receiving focus events sometimes
		if (!win_active) {
#ifdef _LAX_PLATFORM_XLIB
			xlib_win_xatts.border_pixel = (win_themestyle ? win_themestyle->inactive_border.Pixel() : 0);
			if (xlib_window) XChangeWindowAttributes(app->dpy, xlib_window, CWBorderPixel, &xlib_win_xatts);
#endif //_LAX_PLATFORM_XLIB
		}
		DBG cerr <<WindowTitle()<<": real focus off"<<endl;
	} else {
		DBG cerr <<WindowTitle()<<": other focus off"<<endl;
	}

	DBG cerr <<"(typ)Focus off "<<WindowTitle()<<endl;
	return 0;
}

//! Resize with new nw, and nh.
/*! This function will normally be called automatically, in response to an event, and not when
 * the user wants to resize a window. If you need to resize a window yourself, you should
 * probably use MoveResize().
 *
 * This will call XResizeWindow(app->dpy,window,nw,nh), and update win_w and win_h.
 * 
 * If this is called from event(), note that the window is under override redirect as per ICCCM.
 *
 * \todo i want to remove this one, and only have MoveResize, but when I just have this function call
 *   MoveResize, it screws up RowFrame!! what the F? override_redirect funkiness? see icccm 4.2.9
 */
int anXWindow::Resize(int nw,int nh)
{
	DBG cerr << "anXWindow::Resize(obj:"<<object_id<<"):"<<WindowTitle()<<"  w,h:"<<nw<<','<<nh<<endl;
//	return MoveResize(win_x,win_y,nw,nh);
	if (nw<=0 || nh<=0) return 1;

#ifdef _LAX_PLATFORM_XLIB
	if (xlib_window) XResizeWindow(app->dpy,xlib_window,nw,nh);
#endif //_LAX_PLATFORM_XLIB

	win_w = nw;
	win_h = nh;

	Displayer *dp = MakeCurrent();
	dp->CurrentResized(this, nw,nh);

	if (!win_parent) {
		// need to remap sizing if monitor has different scale
		double old_scale = win_cur_uiscale;
		win_cur_uiscale = -1;
		UIScale();
		if (win_cur_uiscale != old_scale) UIScaleChanged();
	}

	needtodraw|=1;
	return 0;
}

//! Move and resize the window.
/*! This will call XMoveResizeWindow(app->dpy,window,nx,ny,nw,nh), and update win_x, win_y, win_w, and win_h.
 *  Please ensure that the supplied values are reasonable.
 */
int anXWindow::MoveResize(int nx,int ny,int nw,int nh) 
{
	if (nw<=0 || nh<=0) return 1;
	DBG cerr << "anXWindow::MoveResize-"<<xlib_window<<":"<<WindowTitle()<<"  x,y:"<<nx<<','<<ny<<"  w,h:"<<nw<<','<<nh<<endl;

#ifdef _LAX_PLATFORM_XLIB
	if (xlib_window) {
		DBG cerr <<"---anXWindow::MoveResizing window: "<<xlib_window<<endl;
		//if (nx==win_x && ny==win_y) XResizeWindow(nw,nh);
		XMoveResizeWindow(app->dpy,xlib_window,nx,ny,nw,nh);
	}
#endif //_LAX_PLATFORM_XLIB

	win_x=nx;
	win_y=ny;
	win_w=nw;
	win_h=nh;

	Displayer *dp = MakeCurrent();
	dp->CurrentResized(this, nw,nh);

	if (!win_parent) {
		// need to remap sizing if monitor has different scale
		double old_scale = win_cur_uiscale;
		win_cur_uiscale = -1;
		UIScale();
		if (win_cur_uiscale != old_scale) UIScaleChanged();
	}

	needtodraw|=1;
	DBG cerr <<"    done MoveResize"<<endl;
	return 0;
}

//! Default event handler.
/*! Events dispatched here automatically are mouse, device, and keyboard events other
 * than LAX_onFocusOn and LAX_onFocusOff events. The events are relayed to the proper
 * LBDown(), RBDown(), etc.
 *
 * None of the events that reach this function are strictly necessary for the functioning
 * of a window at a low level. If you need to override LAX_onFocusOn and LAX_onFocusOff
 * events, you must redefine FocusOn() and FocusOff().
 *
 * Returns 0 for event accepted, or 1 for ignored.
 */
int anXWindow::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"event for "<<data->to<<"  from "<<data->from<<": "<<lax_event_name(data->type)<<" \""<<(mes?mes:"-")<<"\""<<endl;

	if (data->type==LAX_onKeyDown) {
		const KeyEventData *ke=dynamic_cast<const KeyEventData*>(data);
		return CharInput(ke->key, ke->buffer, ke->len, ke->modifiers, ke->device);
	}

	if (data->type==LAX_onKeyUp) {
		const KeyEventData *ke=dynamic_cast<const KeyEventData*>(data);
		return KeyUp(ke->key, ke->modifiers, ke->device);
	}

	if (data->type==LAX_onButtonDown) {
		const MouseEventData *me=dynamic_cast<const MouseEventData*>(data);

		DBG cerr <<"Button "<<me->button<<" down for "<<WindowTitle()<<endl;

		if (me->button==1) return LBDown(me->x, me->y, me->modifiers, me->count, me->device);
		else if (me->button==2) return MBDown(me->x, me->y, me->modifiers, me->count, me->device);
		else if (me->button==3) return RBDown(me->x, me->y, me->modifiers, me->count, me->device);

		if (me->button==4) return WheelUp(me->x, me->y, me->modifiers, me->count, me->device);
		if (me->button==5) return WheelDown(me->x, me->y, me->modifiers, me->count, me->device);
		if (me->button==6) return WheelRight(me->x, me->y, me->modifiers, me->count, me->device);
		if (me->button==7) return WheelLeft(me->x, me->y, me->modifiers, me->count, me->device);

		return ButtonDown(me->button, me->x, me->y, me->modifiers, me->count, me->device);

	}

	if (data->type==LAX_onButtonUp) {
		const MouseEventData *me=dynamic_cast<const MouseEventData*>(data);

		if (me->button==1) return LBUp(me->x, me->y, me->modifiers, me->device);
		else if (me->button==2) return MBUp(me->x, me->y, me->modifiers, me->device);
		else if (me->button==3) return RBUp(me->x, me->y, me->modifiers, me->device);

		if (me->button==4 || me->button==5) return 1; //ignore button 4 and 5 for wheel events

		return ButtonUp(me->button, me->x, me->y, me->modifiers, me->device);

	}

	if (data->type==LAX_onMouseMove) {
		const MouseEventData *me=dynamic_cast<const MouseEventData*>(data);
		return MouseMove(me->x, me->y, me->modifiers, me->device);
	}

	if (data->type == LAX_onDeviceChange) return DeviceChange(dynamic_cast<const DeviceEventData*>(data));

	if (data->type == LAX_onThemeChanged) {
		ThemeChanged();
		return 0;
	}

	if (data->type == LAX_onMouseIn) {
		 //set mouse cursor
		const EnterExitData *ee=dynamic_cast<const EnterExitData*>(data);
		const_cast<LaxMouse*>(dynamic_cast<const LaxMouse*>(ee->device))->setMouseShape(this, win_pointer_shape);
	}

	return 1;
}


#ifdef _LAX_PLATFORM_XLIB

//initialize statics
bool anXWindow::xlib_vars_initialized = false;
Atom anXWindow::LaxDndProperty = 0;
Atom anXWindow::XdndAware = 0;
Atom anXWindow::XdndEnter = 0;
Atom anXWindow::XdndLeave = 0;
Atom anXWindow::XdndPosition = 0;
Atom anXWindow::XdndStatus = 0;
Atom anXWindow::XdndDrop = 0;
Atom anXWindow::XdndFinished = 0;
Atom anXWindow::XdndSelection = 0;
Atom anXWindow::XdndProxy = 0;
Atom anXWindow::XdndActionList = 0;
Atom anXWindow::XdndActionDescription = 0;
Atom anXWindow::XdndActionCopy = 0;
Atom anXWindow::XdndActionMove = 0;
Atom anXWindow::XdndActionLink = 0;
Atom anXWindow::XdndActionAsk = 0;
Atom anXWindow::XdndActionPrivate = 0;
Atom anXWindow::XdndTypeList = 0;
Atom anXWindow::MimeTextUriList = 0;
Atom anXWindow::MimeTextPlain = 0;
Atom anXWindow::MimeTextPlainUtf8 = 0;
Atom anXWindow::MimeTextPlainLatin1 = 0;
Atom anXWindow::Xlib_TEXT = 0;
Atom anXWindow::Xlib_UTF8_STRING = 0;
Atom anXWindow::_NET_WM_STATE = 0;
int  anXWindow::_NET_WM_STATE_REMOVE = 0;
int  anXWindow::_NET_WM_STATE_ADD    = 1;
int  anXWindow::_NET_WM_STATE_TOGGLE = 2;
Atom anXWindow::_NET_WM_STATE_FULLSCREEN = 0;

void anXWindow::InitXlibVars(Display *dpy)
{
	if (xlib_vars_initialized) return;
	xlib_vars_initialized = true;

	LaxDndProperty        = XInternAtom(dpy, "LaxDndProperty", False);
	XdndAware             = XInternAtom(dpy, "XdndAware", False);
	XdndSelection         = XInternAtom(dpy, "XdndSelection", False);
	XdndEnter             = XInternAtom(dpy, "XdndEnter", False);
	XdndLeave             = XInternAtom(dpy, "XdndLeave", False);
	XdndPosition          = XInternAtom(dpy, "XdndPosition", False);
	XdndDrop              = XInternAtom(dpy, "XdndDrop", False);
	XdndFinished          = XInternAtom(dpy, "XdndFinished", False);
	XdndStatus            = XInternAtom(dpy, "XdndStatus", False);
	XdndActionCopy        = XInternAtom(dpy, "XdndActionCopy", False);
	XdndActionMove        = XInternAtom(dpy, "XdndActionMove", False);
	XdndActionLink        = XInternAtom(dpy, "XdndActionLink", False);
	XdndActionAsk         = XInternAtom(dpy, "XdndActionAsk", False);
	XdndActionPrivate     = XInternAtom(dpy, "XdndActionPrivate", False);
	XdndTypeList          = XInternAtom(dpy, "XdndTypeList", False);
	XdndActionList        = XInternAtom(dpy, "XdndActionList", False);
	XdndActionDescription = XInternAtom(dpy, "XdndActionDescription", False);

	// XlibStringType        = XInternAtom(dpy,"XlibStringType",False);
    MimeTextUriList       = XInternAtom(dpy,"text/uri-list",False);
    MimeTextPlain         = XInternAtom(dpy,"text/plain",False);
    MimeTextPlainUtf8     = XInternAtom(dpy,"text/plain;charset=UTF-8",False);
    MimeTextPlainLatin1   = XInternAtom(dpy,"text/plain;charset=ISO-8859-1",False);
    Xlib_TEXT             = XInternAtom(dpy,"TEXT",False);
    Xlib_UTF8_STRING      = XInternAtom(dpy,"UTF8_STRING",False);

    _NET_WM_STATE            = XInternAtom(dpy, "_NET_WM_STATE", False);
    // _NET_WM_STATE_REMOVE     = XInternAtom(dpy, "_NET_WM_STATE_REMOVE", False);
    // _NET_WM_STATE_ADD        = XInternAtom(dpy, "_NET_WM_STATE_ADD", False);
    // _NET_WM_STATE_TOGGLE     = XInternAtom(dpy, "_NET_WM_STATE_TOGGLE", False);
    _NET_WM_STATE_FULLSCREEN = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
}


//! Xlib event receiver.
/*! Under most circumstances, users can safely ignore this function.
 *
 * Focus, Mouse, key, and other input device events are not sent here. Those events are
 * interpreted by anXApp, and the processed events are sent to Event().
 *
 * Selection requests (SelectionNotify and SelectionRequest),
 * as for Xdnd for instance, which uses anXWindow::selectionDropped() and anXWindow::getSelectionData(). 
 *
 * ResizeRequest/ConfigureNotify events are handled as per Xlib/ICCCM spec.
 *
 * The X protocols for WM_DELETE_WINDOW (see also deletenow()) and _NET_WM_PING are handled here.
 *
 * MapNotify and UnmapNotify cause a generic EventData to be sent to Event() with LAX_onMapped or LAX_onUnmapped.
 *
 * Expose events will call ExposeChange() with an appropriate ScreenEventData.
 *
 * Any time an event is absorbed, 0 is returned.
 * Otherwise a non-zero value should be returned. This enables anXApp to maybe try to do something
 * else with the event.
 *
 * If the window has somehow selected for the following X events, they can be intercepted 
 * here by redefining event(). Just remember to call this function first.
 *
 * <pre>
 *  MappingNotify
 *  CreateNotify
 *  ReparentNotify
 *  GraphicsExpose
 *  VisibilityNotify
 *  CreateNotify
 *  DestroyNotify
 *  MapRequest
 *  ConfigureRequest
 *  CirculateNotify
 *  CirculateRequest
 *  PropertyNotify 
 *  ColormapNotify
 *  KeymapNotify
 *  SelectionClear
 * </pre>
 *
 */
int anXWindow::event(XEvent *e)
{
	DBG cerr <<endl<<"xevent anXWindow::event() for \""<<WindowTitle()
	DBG        <<"\", got event "<<xlib_event_name(e->type)<<" "<<e->xany.serial<<endl;

	switch(e->type) {
		case ClientMessage: {
		 	char *aname=XGetAtomName(app->dpy,e->xclient.message_type);
			DBG cerr << "ClientMessage for "<<WindowTitle()<<": "<<aname<<endl;

			if (strcmp(aname,"WM_PROTOCOLS")==0) {
				 //check for delete window
				XFree(aname);
		 		aname=XGetAtomName(app->dpy,e->xclient.data.l[0]);
				DBG cerr <<"...got protocol:"<<aname<<endl;
				if (strcmp(aname,"WM_DELETE_WINDOW")==0) {
					XFree(aname);
					switch (deletenow()) {
						case 1: app->destroywindow(this); return 0; // adds it to app's deletion queue
						case 2: app->unmapwindow(this); return 0;
						case 3: return 0; //do nothing
						default: return 0;
					}
				} else if (strcmp(aname,"_NET_WM_PING")==0) {
					 //this protocol is for window managers to guess whether a window is responding
					 //or might be hung
					XFree(aname);
					e->xany.window=DefaultRootWindow(app->dpy);
					XSendEvent(app->dpy,e->xany.window, False,(SubstructureNotifyMask|SubstructureRedirectMask), e);
					return 0;
				} else XFree(aname);
				aname = XGetAtomName(app->dpy, e->xclient.message_type);

			} else if (strcmp(aname,"XdndEnter")==0) {
				// dnd (drag and drop) started from somewhere!!
				HandleXdndEnter(e);

			} else if (strcmp(aname,"XdndPosition")==0) {
				HandleXdndPosition(e);
     
			} else if (strcmp(aname,"XdndDrop")==0) {
				// Sent from source to target to complete the drop.
				HandleXdndDrop(e); //this calls XConvertSelection, so we need to watch for a SelectionNotify event

			} else if (strcmp(aname,"XdndLeave")==0) {
				DBG cerr << "XdndLeave in "<<WindowTitle()<<endl;

				// need to cancel any possible dnd in process
				if (xlib_dnd) { delete xlib_dnd; xlib_dnd = nullptr; }
				DndHoverCanceled();
			}

			XFree(aname);
			return 1;
		 } break;

		case Expose:  {
			//e->xexpose.count?
			ScreenEventData d(e->xexpose.x,e->xexpose.y,e->xexpose.width,e->xexpose.height);
			ExposeChange(&d);
			return 0;
		  }

		case ConfigureNotify: {
			 //this happens after the user scales a window via window manager decorations,
			 //for instance. Thus, all we need to do is update win_x, win_y, win_w, win_h.

			DBG cerr <<"..typ("<<WindowTitle()<<"):ConfigureNotify..";
			DBG cerr <<" or="<<e->xconfigure.override_redirect<<" ";
			DBG cerr <<" conf to: "<<e->xconfigure.x<<','<<e->xconfigure.y<<','<<" "<<e->xconfigure.width<<','<<e->xconfigure.height<<endl;
			DBG cerr <<" old xywh: "<<win_x<<','<<win_y<<" "<<win_w<<'x'<<win_h<<endl;


			XWindowAttributes actual;
			Window cr;
			int X,Y,W,H;

			DBG //----------------
			DBG if (xlib_backbuffer) {
			DBG 	XGetWindowAttributes(app->dpy, xlib_window, &actual);
			DBG 	 //find where window currently is, which we hope is same as confignotify, but it might not be, esp. x,y
			DBG 	W = actual.width;
			DBG 	H = actual.height;
			DBG 	XTranslateCoordinates(app->dpy, xlib_window, actual.root, 0, 0, &X, &Y, &cr);
			DBG 	cerr <<"getatts xlib_backbuffer says xywh: "<<X<<","<<Y<<" "<<W<<'x'<<H<<"  "<<endl;
			DBG }
			DBG //---------------

			//On ubuntu, maximizing does not properly adjust the coordinates, so we
			//manually query the location and dimensions of the window
			XGetWindowAttributes(app->dpy, xlib_window, &actual);
			 //find where window currently is, which we hope is same as confignotify, but it might not be, esp. x,y
			W = actual.width;
			H = actual.height;
			XTranslateCoordinates(app->dpy, xlib_window, actual.root, 0, 0, &X, &Y, &cr);

			DBG cerr <<"getatts xlib_window says xywh: "<<X<<","<<Y<<" "<<W<<'x'<<H<<"  "<<endl;


			if (win_parent) break; //assume child windows are all explicitly user controlled

//			if (e->xconfigure.override_redirect) { 
//				// break if in a ResizeRequest
//				DBG cerr<<"...in override_redirect, assuming in ResizeRequest"<<endl;
//				break;
//			}

			DBG bool diffloc = false;
			bool diffsize = false;

			 // w, if unmapped, resizes are not setting w/h, why not???
			//if (e->xconfigure.x      != win_x) { win_x = e->xconfigure.x;      diffloc  = true; }
			//if (e->xconfigure.y      != win_y) { win_y = e->xconfigure.y;      diffloc  = true; }
			if (e->xconfigure.width  != win_w) { W     = e->xconfigure.width;  diffsize = true; }
			if (e->xconfigure.height != win_h) { H     = e->xconfigure.height; diffsize = true; }
			if (X != win_x) {
				win_x = X;
				DBG diffloc  = true;
			}
			if (Y != win_y) {
				win_y = Y;
				DBG diffloc  = true;
			}
			//if (W != win_w) { W     = W; diffsize = true; }
			//if (H != win_h) { H     = H; diffsize = true; }

			DBG if (diffloc) cerr <<"  diff location"<<endl;

			// ***  trying manual resize here. Gnome maximizing somehow doesn't
			//    trigger proper window resizing here. Doing a resize here fixes that *most* of the time.
			//    Need to find solution to this!!
			if (win_on) {
				//if (diffloc) MoveResize(X,Y,W,H);
				//else 
				if (!e->xconfigure.override_redirect && diffsize) Resize(W,H);
				else if (!win_parent) {
					double oldscale = win_cur_uiscale;
					win_cur_uiscale = -1;
					UIScale();
					if (win_cur_uiscale != oldscale) UIScaleChanged();
				}
			}

		} break;

		case ResizeRequest: {
			 //icccm says to set override redirect, manually XResizeWindow, then remove override redirect

			DBG cerr <<"..typ("<<WindowTitle()<<"):ResizeRequest.. wh: "<<e->xresizerequest.width<<", "<<e->xresizerequest.height<<endl;
		 	 // set override redirect
			xlib_win_xatts.override_redirect = True;
			XChangeWindowAttributes(app->dpy,xlib_window,CWOverrideRedirect,&xlib_win_xatts);
			 // must manual resize
			Resize(e->xresizerequest.width,e->xresizerequest.height);
			 // remove override redirect
			xlib_win_xatts.override_redirect = False;
			XChangeWindowAttributes(app->dpy,xlib_window,CWOverrideRedirect,&xlib_win_xatts);
			needtodraw=1;
			DBG cerr <<"..end ResizeRequest"<<endl;
		} break;

		case MapNotify: {
				DBG cerr <<"..typ("<<WindowTitle()<<"):MapNotify on"<<endl;
				if (e->xmap.window==e->xmap.event) {
					EventData *d=new EventData(LAX_onMapped,object_id,object_id);
					app->SendMessage(d,object_id,nullptr,object_id);
					win_on=1;
				}
			} break;

		case UnmapNotify: {
				DBG cerr <<"..typ("<<WindowTitle()<<"):MapNotify off"<<endl;
				if (e->xmap.window==e->xmap.event) {
					app->ClearTransients(this);
					EventData *d=new EventData(LAX_onUnmapped,object_id,object_id);
					app->SendMessage(d,object_id,nullptr,object_id);
					win_on=0;
				}
			} break;

		//
		//------- Selection handling. copy/paste and drag and drop:
		//

		case SelectionNotify: {
			DBG cerr <<"\n----SelectionNotify this window:"<<xlib_window<<endl;
			DBG cerr <<	"  requestor:"<<e->xselection.requestor<<endl;

			if (e->xselection.property == 0) {
				DBG cerr <<"selection failed!"<<endl;
				return 0;
			}

			char *selection = XGetAtomName(app->dpy, e->xselection.selection);
			char *target    = XGetAtomName(app->dpy, e->xselection.target);
			char *property  = XGetAtomName(app->dpy, e->xselection.property);

			DBG cerr <<"  selection: "<<(selection ? selection :"(no selection)")<<endl;
			DBG cerr <<"  target: "   <<(target    ? target    :"(no target)")   <<endl;
			DBG cerr <<"  property: " <<(property  ? property  :"(no property)") <<endl;

			Atom actual_type;
			int format;
			unsigned long len, remaining;
			unsigned char *data=nullptr;
			bool drop_accepted = false;
			if (Success == XGetWindowProperty(
							app->dpy,
							e->xselection.requestor,
							e->xselection.property, //property the selection data is stored in
							0,0x8000000L,          //offset and max len into property to get
							True,                 //whether to delete property afterwards
							AnyPropertyType,      //preferred type (this value is #defined 0)
							&actual_type,
							&format, &len, &remaining, &data)) {

				char *actualtype = XGetAtomName(app->dpy,actual_type);
				DBG if (actualtype) cerr <<"  dropping selection of type "<<(actualtype ? actualtype : "(unknown)")<<endl;

				if (e->xselection.selection == XdndSelection && xlib_dnd && xlib_dnd->targetChild) {
					drop_accepted = !xlib_dnd->targetChild->selectionDropped(data,len,actualtype,selection);
				} else drop_accepted = !selectionDropped(data,len,actualtype,selection);

				if (actualtype) XFree(actualtype);
				XFree(data);
			}

			if (e->xselection.selection == XdndSelection) {
				HandleXdndFinished(e, drop_accepted);
				if (xlib_dnd) { delete xlib_dnd; xlib_dnd = nullptr; }
			}

			if (property)  XFree(property);
			if (target)    XFree(target);
			if (selection) XFree(selection);
			return 0;
		} //SelectionNotify
		
		case SelectionRequest: {
			char *selection=XGetAtomName(app->dpy,e->xselectionrequest.selection); 
			char *target   =XGetAtomName(app->dpy,e->xselectionrequest.target);
			char *property =nullptr;
			if (e->xselectionrequest.property) property=XGetAtomName(app->dpy,e->xselectionrequest.property);

			DBG cerr <<"\n----SelectionRequest this window:" <<xlib_window    <<endl;
			DBG cerr <<	"  requestor:" << e->xselectionrequest.requestor  <<endl;
			DBG cerr <<	"  owner:"     << e->xselectionrequest.owner      <<endl; 
			DBG cerr << "  target: "   << (target    ? target    : "(no target)")   <<endl;
			DBG cerr << "  selection: "<< (selection ? selection : "(no selection)")<<endl;
			DBG cerr << "  property: " << (property  ? property  : "(no property)") <<endl;


			 //A window gets these after someone else calls XConvertSelection().
			// ??? if (e->xselectionrequest.selection==XA_SECONDARY) {<--probably a laxkit paste request
				 //probably resulted from a selectionPaste() call.
				 //need to set the data on the requestor window

			int len=0;
			char *data = getSelectionData(&len, property, target, selection);

			if (len==0 && data) {
				cerr <<" *** warning! anXWindow::SelectionRequest crash magnet for binary data!!!"<<endl;
				len=strlen(data);
			}
			if (len) {
				 //change the selection property on the window to hold the requested data
				XChangeProperty(app->dpy,
							e->xselectionrequest.requestor,
							e->xselectionrequest.property,    //format of data
							e->xselectionrequest.target,
							8,
							PropModeReplace,
							(const unsigned char *)data,
							len);
			}

			 //notify requestor, whether or not any data
			XEvent ee;
			ee.xselection.type=SelectionNotify;
			ee.xselection.display=app->dpy;
			ee.xselection.requestor=e->xselectionrequest.requestor;
			ee.xselection.selection=e->xselectionrequest.selection;
			ee.xselection.target=e->xselectionrequest.target;
			ee.xselection.property= len ? e->xselectionrequest.property : None;
			ee.xselection.time=e->xselectionrequest.time;
			XSendEvent(app->dpy,e->xselectionrequest.requestor,False,0,&ee);

			DBG cerr <<" sent XSelectionNotify to "<<e->xselectionrequest.requestor<<endl;


			delete[] data;
			if (property)  XFree(property);
			if (target)    XFree(target);
			if (selection) XFree(selection);

			return 1;
		} //SelectionRequest

		//case SelectionClear:

		////---------------- un-mapped events:
		//case DestroyNotify: {
		//	if (e->xdestroywindow.event==e->xdestroywindow.window) window=0; //*** does this work right? might be
		//	**better than that anXApp mechanism? well, there may be other events in the queue, and we don't
		//	want other windows trying to use this window to send() or whatnot
		//	} break;
		//case CreateNotify: {} break;
		//ReparentNotify
		//GraphicsExpose
		//VisibilityNotify
		//CreateNotify
		//DestroyNotify
		//MapRequest
		//ConfigureRequest
		//CirculateNotify
		//CirculateRequest
		//PropertyNotify 
		//ColormapNotify
		//MappingNotify
		//KeymapNotify
						
		
	} // switch event type

	return 1;
}
#endif //_LAX_PLATFORM_XLIB



//-------------------------------- Control window functions: ------------------


//! Input of character data or control character presses.
/*!
 * If key>0 and key<=0x10ffff, then key is the USC code of a character. In this case, buffer
 * will contain the utf8 representation of key. If key>0x1000000, then key can be compared to
 * various Laxkit defined keys, such as LAX_Pgdown, LAX_Del, etc. In this case, currently,
 * key&0xffffff is the Xlib keysym of the key. Also in this case, buffer will be nullptr.
 *
 * If key==0 but buffer!=nullptr, then buffer contains multiple utf8 characters, all of which
 * should be considered input. This will have resulted from input that was somehow composed
 * from key sequences at a higher level.
 *
 * len is the length in bytes of buffer, if any.
 *
 * state will be a combination of ShiftMask, ControlMask, AltMask, MetaMask, KeypadMask, CapsLockMask,
 * and NumLockMask. Numpad keys will have the KeypadMask bit set in state. You can check what
 * combination of shift, control, alt, and meta masks are presest by examining state&LAX_STATE_MASK.
 *
 * See laxdefs.h for various definitions for keys and LAX_STATE_MASK.
 * 
 * Currently, no distinction is made between left and right control, alt, or meta keys. If
 * you really, really need the distinction, you can intercept the key events in anXWindow::event(),
 * or complain to the developers.
 *
 * Default behavior for this function is for tab press to make the input focus go to 
 * the nextcontrol (if it is not nullptr), shift-tab transfers focus to prevcontrol (if not nullptr).
 * See SelectNextControl() and SelectPrevControl().
 */
int anXWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	DBG cerr <<" CharInput: "<<ch<<endl;
	if (ch==LAX_Esc && (win_style&ANXWIN_ESCAPABLE) && deletenow()) { app->destroywindow(this); return 0; }
	if (ch!='\t') return 1;
	if ((state&LAX_STATE_MASK)==ShiftMask) return SelectPrevControl(kb);
	if ((state&LAX_STATE_MASK)==0) return SelectNextControl(kb);


	// ***************
//	if (ch==' ') {
//		XWindowAttributes actual; 
//		XGetWindowAttributes(app->dpy, xlib_window, &actual);
//		Resize(w,h);
//	}
	return 1;
}

//! Return the window most relevant for tab control loops.
/*! For instance, a LineInput has a label, and a LineEdit embedded in it.
 * For the purposes of adding to a tab loop, the LineEdit should be
 * connected, and not the LineInput. So the LineInput::GetController() 
 * returns its LineEdit rather than itself, which is the default.
 */
anXWindow *anXWindow::GetController() 
{ return this; } 

//! Transfer focus to nextcontrol.
/*! \todo *** must focus the right keyboard!! and at the right time!!!!
 */
int anXWindow::SelectNextControl(const LaxDevice *d) 
{
	DBG cerr <<"SelectNextControl from "<<WindowTitle()<<endl;
	if (win_active && nextcontrol && nextcontrol->win_on) {
		app->setfocus(nextcontrol,0,dynamic_cast<const LaxKeyboard*>(d));
		nextcontrol->ControlActivation(1);
		ControlActivation(0);
		return 0; 
	}
	DBG cerr <<"-- no nextcontrol"<<endl;
	return 1;
}

//! Transfer the focus to prevcontrol.
int anXWindow::SelectPrevControl(const LaxDevice *d)
{
	DBG cerr <<"SelectPrevControl from "<<WindowTitle()<<endl;
	if (win_active && prevcontrol && prevcontrol->win_on) {
		app->setfocus(prevcontrol,0,dynamic_cast<const LaxKeyboard*>(d));
		prevcontrol->ControlActivation(1);
		ControlActivation(0);
		return 0; 
	}
	DBG cerr <<"-- no prevcontrol"<<endl;
	return 1;
}

//! Do special activation or not when controls are activated by tabbing.
/*! This would be done in FocusOn()/FocusOff(), but XI2 seems to send a ton
 * of useless Enter/Leave events upon a button down.
 */
void anXWindow::ControlActivation(int on)
{}

//! Close a tab loop.
/*! Usually this will be called from the init() function of some frame class to signal an end
 * to a tab loop.
 *
 * This just connects the most previous control that is nullptr to the most next control that is nullptr.
 * Assumes that for each control in the current loop, thatcontrol->nextcontrol->prevcontrol==thatcontrol if thatcontrol is
 * not nullptr, some thing for prevcontrol. If controls were added strictly through the anXWindow control loop functions,
 * this will be true.
 */
int anXWindow::CloseControlLoop()
{
	anXWindow *thisc=GetController(),
			  *mostprev=thisc->prevcontrol,
			  *mostnext=thisc->nextcontrol;
	
	int n=1;
	while (mostprev && mostprev->prevcontrol && mostprev!=thisc) {mostprev=mostprev->prevcontrol;  n++;}
	if (mostprev==thisc) return 1; //already closed!
	while (mostnext && mostnext->nextcontrol && mostnext!=thisc) {mostnext=mostnext->nextcontrol;  n++;}
	DBG cerr <<"CloseControlLoop has "<<n<<endl;
	if (mostprev==nullptr) mostprev=thisc;
	if (mostnext==nullptr) mostnext=thisc;
	mostprev->prevcontrol=mostnext;
	mostnext->nextcontrol=mostprev;
	return 1;
}

//! Connect towhat to this. Used for tab loops.
/*! If towhat is passed in as nullptr, then detach this from its loop, connecting next and prev.
 *
 * Actually this function connects towhat->GetController() and this->GetController(). This is so
 * the focus goes to, for instance, the editing part of a LineInput, rather than the LineInput itself,
 * which is just a container.
 * 
 * If towhat is part of an open loop, then the bounds of that loop are squeezed in between this and 
 * either nextcontrol or prevcontrol, depending on whether after==1 or not. If towhat is part
 * of a closed loop, then that loop is cut between towhat and towhat->prevcontrol.
 *
 * Return 0 for successful attachment or detachment. 1 for when there was nothing to attach, that is,
 * GetController() for this or towhat returned nullptr.
 */
int anXWindow::ConnectControl(anXWindow *towhat, int after) // after=1
{
	anXWindow *fromwhat=GetController();
	if (!fromwhat) return 1;
	if (!towhat) { // detach this
		DBG cerr <<"Disconnect "<<fromwhat->WindowTitle()<<endl;
		if (fromwhat->nextcontrol) fromwhat->nextcontrol->prevcontrol=fromwhat->prevcontrol;
		if (fromwhat->prevcontrol) fromwhat->prevcontrol->nextcontrol=fromwhat->nextcontrol;
		fromwhat->prevcontrol=fromwhat->nextcontrol=nullptr;
		return 0;
	}
	if (towhat) towhat=towhat->GetController();
	if (!towhat) return 1;
	
	DBG cerr <<"Connect "<<fromwhat->WindowTitle()<<" to "<<towhat->WindowTitle()<<endl;
	 // find bounds of any loop attached to towhat
	anXWindow *tostart=towhat->prevcontrol,*toend=towhat->nextcontrol;
	while (tostart && tostart->prevcontrol && tostart!=towhat) {tostart=tostart->prevcontrol;}
	if (tostart==towhat) { // closed! must open it...
		tostart->prevcontrol->nextcontrol=nullptr;
		tostart->prevcontrol=nullptr;
	}
	while (toend && toend->nextcontrol) { toend=toend->nextcontrol; }
	if (tostart==nullptr) tostart=towhat;
	if (toend==nullptr) toend=towhat;
	
	anXWindow *temp;
	if (after) {
		temp=fromwhat->nextcontrol;
		fromwhat->nextcontrol=tostart;
		tostart->prevcontrol=fromwhat;
		toend->nextcontrol=temp;
		if (temp) temp->prevcontrol=toend;
	} else {
		temp=fromwhat->prevcontrol;
		fromwhat->prevcontrol=toend;
		toend->nextcontrol=fromwhat;
		tostart->prevcontrol=temp;
		if (temp) temp->nextcontrol=tostart;
	}
	return 0;
}

//! Set the new owner and control message.
/*! Sets win_owner whether or not nowner==nullptr.
 * Sets win_sendthis only if mes!=nullptr.
 *
 * If send_mask!=0, then set win_owner_send_mask to it.
 */
void anXWindow::SetOwner(anXWindow *nowner, const char *mes, unsigned int send_mask)
{
	DBG cerr <<"---SetOwner of "<<WindowTitle()<<": "<<(nowner?nowner->WindowTitle():"(no owner)")<<","<<(mes?mes:"(no mes)")<<endl;

	if (nowner) win_owner=nowner->object_id;
	else win_owner=0;

	if (mes) makestr(win_sendthis,mes);

	if (send_mask!=0) win_owner_send_mask=send_mask;
}

void anXWindow::SetOwner(unsigned long nowner_id,const char *mes, unsigned int send_mask)
{
	if (nowner_id) win_owner=nowner_id;
	else win_owner=0;

	if (mes) makestr(win_sendthis,mes);

	if (send_mask!=0) win_owner_send_mask=send_mask;
}

//! Windows may call this when their contents change.
/*! This would be when a text edit has more text entered, for instance.
 *
 * If (win_owner_send_mask & LAX_onContentChange), then the default here would
 * trigger sending a SimpleEvent of type onContentChange to win_owner.
 */
void anXWindow::contentChanged()
{
	if (!win_owner || !(win_owner_send_mask & LAX_onContentChange)) return;
	app->SendMessage(new SimpleMessage(win_owner, object_id, LAX_onContentChange));
}

//! Windows may call this when their selections change.
/*! This would be when more or less text gets selected in a text edit, for instance.
 *
 * If (win_owner_send_mask & onSelectionChange), then this would
 * trigger sending a SimpleEvent of type onSelectionChange to win_owner.
 */
void anXWindow::selectionChanged()
{
	if (!win_owner || !(win_owner_send_mask & LAX_onSelectionChange)) return;
	app->SendMessage(new SimpleMessage(win_owner, object_id, LAX_onSelectionChange));
}


//------------------ Selection and Drag-and-drop functions

/*! Initiate a copy.
 * 
 * Normally, you will call this function in response to a control-c.
 *
 * In X, this is done by aquiring ownership of the selection.
 * If mid==1 then grab PRIMARY, otherwise grab CLIPBOARD.
 *
 * Return 0 for success, nonzero for some kind of error.
 *
 * No data is actually transferred. This function just lets the powers that be to use this
 * window as the source for the next paste event.
 */
int anXWindow::selectionCopy(char mid)
{
#ifdef _LAX_PLATFORM_XLIB
	Atom atom=mid?XInternAtom(app->dpy,"PRIMARY",False):XInternAtom(app->dpy,"CLIPBOARD",False);
	XSetSelectionOwner(app->dpy, atom, xlib_window, CurrentTime);
#endif //_LAX_PLATFORM_XLIB

	return 0;
}

//! Initiate a paste.
/*! If mid!=0, then this is a middle click paste. Otherwise, it is something like control-v.
 * In X terms, mid!=0 means grab from PRIMARY, rather than CLIPBOARD. 
 *
 * Selections are asynchronous, so pasting has to be done with X events. 
 *
 * If -1 is returned, then there is no selection to paste.
 * If 0 is returned, then the window can expect a SelectionNotify event at some point, which will
 * be relayed to selectionDropped(). 
 * The selection owner can expect a SelectionRequest.
 *
 * targettype can be "STRING" or *** some other things that needs more research to determine!
 * If targettype==nullptr, then use "STRING".
 *
 * \todo figure out what gets passed around in targettype.
 */
int anXWindow::selectionPaste(char mid, const char *targettype)
{
#ifdef _LAX_PLATFORM_XLIB
	Atom atom = mid ? XInternAtom(app->dpy,"PRIMARY",False) : XInternAtom(app->dpy,"CLIPBOARD",False);
	Window selowner=XGetSelectionOwner(app->dpy, atom);
	if (selowner==None) return -1;

	if (!targettype) targettype="STRING";

	Atom target_type = XInternAtom(app->dpy, targettype, False);

	 //some one does own it
	 //a SelectionRequest event is sent to the owner of the selection
	XConvertSelection(app->dpy,
						atom,         //selection
						target_type,  //target type for data
						XA_SECONDARY, //property in which to put data on the target window
						xlib_window,  //requestor
						CurrentTime);
#endif //_LAX_PLATFORM_XLIB

	return 0;
}

/*! Called from a SelectionNotify event. This is used for both generic selection events
 * (see selectionPaste()) and also drag-and-drop (which == XdndSelection) events.
 *
 * Typical actual_type values:
 *  - text/uri-list
 *  - text/plain
 *  - text/plain;charset=UTF-8
 *  - text/plain;charset=ISO-8859-1
 *  - TEXT
 *  - UTF8_STRING
 *
 * If which is non-null, it is usually which selection the data is pulled from.
 * 
 * Returns 0 if used, nonzero otherwise.
 */
int anXWindow::selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type,const char *which)
{
	DBG cerr<<"anXWindow::selectionDropped() on "<<WindowTitle()<<":"<<endl;
	DBG cerr <<"  type: "     <<(actual_type ? actual_type : "(no type)")     <<endl;
	DBG cerr <<"  selection: "<<(which       ? which       : "(no selection)")<<endl;

	DBG if (data) cerr <<"  data: "<<endl<<data<<endl;

	return 1; 
}

//! Return a new char[] and len of data to set for a selection (clipboard paste or drag and drop).
/*! This is used when a SelectionRequest comes through the wire. Whatever calls this function
 * is responsible for calling delete[] on the returned data.
 *
 * \todo this is a little sloppy about types. what if data can be provided not in the given format?
 *     need to do SelectionNotify with no data when appropriate. cut/paste implementation is not
 *     currently totally ICCCM compliant, but it seems to work at the moment
 */
char *anXWindow::getSelectionData(int *len,const char *property,const char *targettype,const char *selection)
{
	DBG cerr << "getSelectionData:"<<endl;
	DBG cerr << "  target: "   << (targettype ? targettype : "(no target)")   <<endl;
	DBG cerr << "  selection: "<< (selection  ? selection  : "(no selection)")<<endl;
	DBG cerr << "  property: " << (property   ? property   : "(no property)") <<endl;
	
	if (len) *len=0;
	return nullptr;
}


//--------------------------- Drawing helper
/*! Basically, get the relevant displayer (also returned) with this->GetDisplayer(),
 * then calls dp->MakeCurrent(this).
 * Subclasses that use other custom displayers should redefine if necessary.
 */
Displayer *anXWindow::MakeCurrent()
{
	Displayer *dp=GetDisplayer();
	dp->MakeCurrent(this);
	return dp;
}

/*! Return the Displayer that should be used in Refresh().
 * Note this does NOT call dp->MakeCurrent(). Use MakeCurrent() to both ensure this window
 * is the current context, and also retrieve the Displayer to use.
 *
 * The default here is to simple return GetDefaultDisplayer().
 */
Displayer *anXWindow::GetDisplayer()
{
	return GetDefaultDisplayer();
}


//------------------------------------ DndState -------------------------------------

#ifdef _LAX_PLATFORM_XLIB

/*! \class DndState
 * Helper class to coordinate Xlib based drag and drop.
 */

#define XDND_VERSION 5

DndState::DndState()
{
	if (!anXWindow::xlib_vars_initialized) anXWindow::InitXlibVars(anXApp::app->dpy);

	version = XDND_VERSION;
	mode    = Unknown;
	status  = Uncertain;
	action  = 0;

	targetTop = targetChild = nullptr;
	source_window           = 0;
	data_types              = nullptr;
	num_data_types          = 0;
	preferred_type          = -1;

	timestamp = 0;
  	lastx = lasty = 0; //in a drop candidate
  	downx = downy = 0; //used when initiating drop
  	inx = iny = inw = inh = 0;
  	button = 0; //anXWindow auto detects button up
}

DndState::~DndState()
{
	if (num_data_types) deletestrs(data_types, num_data_types);
	if (targetTop)   targetTop  ->dec_count();
	if (targetChild) targetChild->dec_count();
}

/*! dec old, inc new, when initializing new dnd targets.
 * Note this is also called from anXApp::destroywindow() to prevent cyclic ref counting.
 */
void DndState::SetTarget(anXWindow *top, anXWindow *child)
{
	if (targetTop)   targetTop  ->dec_count();
	if (targetChild) targetChild->dec_count();
	targetTop   = top;
	targetChild = child;
	if (targetTop)   targetTop  ->inc_count();
	if (targetChild) targetChild->inc_count();
}

/*! Takes names, calling code must not delete.
 */
void DndState::SetNames(char **names, int len)
{
	if (num_data_types) deletestrs(data_types, num_data_types);
	num_data_types = len;
	if (len) {
		data_types = names;
	}
}

void DndState::Clear()
{
	mode = Unknown;
	status = Uncertain;
	source_window = 0;
	if (data_types) deletestrs(data_types, num_data_types);
}

//! Return the version that w is XdndAware for. 0 for not XdndAware.
int anXWindow::isXdndAware(Window w)
{
    Atom actual_type;
    int format;
    unsigned long count, remaining;
    unsigned char *data=nullptr;
    int version=0;
    XGetWindowProperty(app->dpy, w, XdndAware,
                        0, 0x8000000L, False, XA_ATOM, &actual_type, &format,
                        &count, &remaining, &data);

    if (actual_type!=XA_ATOM || format!=32 || count==0 || !data) {
        //XGetWindowProperty failed
        if (data) XFree(data);
        return 0;
    }
    version=*(Atom *)data;
    XFree(data);
    DBG cerr <<"window "<<w<<" isXdndAware version: "<<version<<endl;
    return version;
}

/*! When a drag and drop source hovers over us.
 */
int anXWindow::HandleXdndEnter(XEvent *e)
{
	// see for more in depth description: https://www.freedesktop.org/wiki/Specifications/XDND/
	// also see blender's implementation: https://github.com/dfelinto/blender/blob/master/extern/xdnd/xdnd.c

	DBG cerr << "XdndEnter in "<<WindowTitle()<<", this window xid: "<<xlib_window<<endl;

	if (xlib_dnd) return 1; //ignore any dnd if there is a current dnd

	xlib_dnd = new DndState();

	//determine Xdnd protocol version. warning: no protocol mishap detection, assuming good for XDND_VERSION
	xlib_dnd->version = (e->xclient.data.l[1]&0xff000000)>>24;
	DBG cerr << "  dropping from dnd version: " << xlib_dnd->version <<endl;
    if (xlib_dnd->version > 5) xlib_dnd->version = XDND_VERSION;

	xlib_dnd->mode = DndState::WaitingForDrop;
    xlib_dnd->source_window = (XID)e->xclient.data.l[0];
    DBG cerr << "  source xid: " << xlib_dnd->source_window<<endl;

    bool more_than_three_types = ((e->xclient.data.l[1] & 1) != 0);
    DBG cerr << "  "<< (more_than_three_types ? "more than three types" : "1,2,or 3 types")<<endl;
    //data.l[1]:
    //     Bit 0 is set if the source supports more than three data types.
    //     The high byte contains the protocol version to use (minimum of the source's and target's highest supported versions).
    //     The rest of the bits are reserved for future use.
    //   if more than 3 types, full list is in XdndTypeList on source

    //data.l[2,3,4] contain the first three types that the source supports.
    //   Unused slots are set to None. The ordering is arbitrary since, in general,
    //   the source cannot know what the target prefers.
    char **names = nullptr;
    int num_names = 0;

    if (more_than_three_types) {
		int format;
		unsigned long len, remaining;
		unsigned char *data = nullptr;
		// long *idata;
		Atom actual_type=0;
		int status = XGetWindowProperty(
						app->dpy,
						xlib_dnd->source_window,
						XdndTypeList,     //property the selection data is stored in
						0,0x8000000L,     //offset and max len into property to get
						False,            //whether to delete property afterwards
						AnyPropertyType,  //preferred type (this value is #defined 0)
						&actual_type,
						&format, &len, &remaining, &data);
		if (status == Success) {
			DBG cerr <<"  dnd type list:"<<endl;
			if (actual_type != None){
				DBG char *actual_type_str = XGetAtomName(app->dpy, actual_type);
				DBG cerr << "   actual type: "<<actual_type_str<<", len: "<<len<<", remaining: "<<remaining<<endl;
				DBG XFree(actual_type_str);

				names = new char*[len+1];
				if (XGetAtomNames(app->dpy, (Atom*)data, len, names)) { //returns non-zero on success. hmm.
					for (int c=0; c < (int)len; c++) {
				    	DBG cerr <<"    dnd type available: "<<names[c]<<endl;
			    	}
	    			for (int c=0; c < (int)len; c++) {
	    				char *nm = names[c];
	    				names[c] = newstr(nm);
	    				XFree(nm);
	    			}
	    			num_names = (int)len;
	    			names[len] = nullptr; //null terminate
		    	} else {
		    		delete[] names;
		    		names = nullptr;
		    	}
			}
			XFree(data);
		}
    } else { //1,2,or 3 types only
    	names = new char*[4];
	    for (int c=2; c <= 4; c++) {
	    	long l = e->xclient.data.l[c];
	    	if (l == None) break;
	    	char *tname = XGetAtomName(app->dpy, l);
	    	DBG cerr <<"  dnd type available: "<<tname<<endl;
	    	names[c-2] = newstr(tname);
	    	num_names++;
	    	XFree(tname);
	    }
	    names[num_names] = nullptr;
	}

	//whether we accept a drop is handled on XdndPosition messages
	xlib_dnd->SetNames(names, num_names);
	DndHoverStart();
	return 0;
}

/*! When a drag and drop source moves around over us, after already entering.
 * This results in the target sending an XdndStatus message back to the source.
 */
int anXWindow::HandleXdndPosition(XEvent *e)
{
	DBG cerr << "XdndPosition in "<<WindowTitle()<<endl;

	 // Sent from source to target when the mouse moves. The message contains
	 // a time stamp, screen coordinates of the mouse, and the requested action.
	 // The target (us) then sends an XdndStatus message updating whether it still will
	 // accept the drop, and the bounds within which no more XdndPosition messages
	 // should be sent.

    // data.l[0] contains the XID of the source window.
    // data.l[1] is reserved for future use (flags).
    // data.l[2] contains the coordinates of the mouse position relative to the root window.
    //     data.l[2] = (x << 16) | y
    // data.l[3] contains the time stamp for retrieving the data. (new in version 1)
    // data.l[4] contains the action requested by the user. (new in version 2)

	if (!xlib_dnd) return 0;

	DBG cerr <<"XdndPosition: "<<((e->xclient.data.l[2]>>16)&0xffff)<<","<<(e->xclient.data.l[2]&0xffff)<<endl;
	DBG char *blah = nullptr;
	DBG if (e->xclient.data.l[4]) blah=XGetAtomName(app->dpy,e->xclient.data.l[4]);
	DBG if (blah) { cerr <<"  action from source: "<<blah<<endl; XFree(blah); }

	xlib_dnd->timestamp = e->xclient.data.l[3];

	int x = (e->xclient.data.l[2]>>16)&0xffff; //note these are screen coords
	int y = (e->xclient.data.l[2]    )&0xffff;
	xlib_dnd->lastx = x;
	xlib_dnd->lasty = y;
	DBG cerr << "  Dnd coords: "<<x<<", "<<y<<endl;

	xlib_dnd->action = XdndActionPrivate;
	xlib_dnd->inw = xlib_dnd->inh = 0;
	IntRectangle rect;
	int xx,yy;
	translate_window_coordinates(nullptr, x, y, this, &xx, &yy, nullptr);

	int which_type = -1;
	anXWindow *child_drop = nullptr;
	if (DndWillAcceptDrop(xx,yy,"", rect, xlib_dnd->data_types, &which_type, &child_drop)) { //e->xclient.data.l[4])) <- the action  TODO!!! deal with actions properly
		xlib_dnd->status = DndState::Target_Accepts;
		xlib_dnd->inx = rect.x;
		xlib_dnd->iny = rect.y;
		xlib_dnd->inw = rect.width;
		xlib_dnd->inh = rect.height;
		xlib_dnd->preferred_type = which_type;
		xlib_dnd->SetTarget(this, child_drop);

		if (child_drop && rect.width && rect.height) { //convert child_drop coords to screen coords
			int xxx,yyy;
			translate_window_coordinates(child_drop, rect.x,rect.y, nullptr, &xx,&yy, nullptr);
			translate_window_coordinates(child_drop, rect.x+rect.width,rect.y+rect.height, nullptr, &xxx,&yyy, nullptr);
			rect.x = xx;
			rect.y = yy;
			rect.width = xxx-xx;
			rect.height = yyy-yy;
		}
		
	} else {
		xlib_dnd->status = DndState::Target_Refuses;
		xlib_dnd->action = 0;
	}

	 // send XdndStatus...

    // data.l[0] contains the XID of the target window. (This is required so XdndStatus messages that
    //                                                    arrive after XdndLeave is sent will be ignored.)
    // data.l[1]:
    //     Bit 0 is set if the current target will accept the drop.
    //     Bit 1 is set if the target wants XdndPosition messages while the mouse moves inside the rectangle in data.l[2,3].
    //     The rest of the bits are reserved for future use.
    // data.l[2,3] contains a rectangle in root coordinates that means "don't send another
    //             XdndPosition message until the mouse moves out of here". It is legal 
    //             to specify an empty rectangle. This means "send another message when the mouse
    //             moves". Even if the rectangle is not empty, it is legal for the source to send 
    //             XdndPosition messages while in the rectangle. The rectangle is stored in the standard Xlib format of (x,y,w,h):
    //     data.l[2] = (x << 16) | y
    //     data.l[3] = (w << 16) | h
    // data.l[4] contains the action accepted by the target. This is normally only allowed to be
    //           either the action specified in the XdndPosition message, XdndActionCopy, or XdndActionPrivate.
    //           None should be sent if the drop will not be accepted. (new in version 2)

	XEvent s;
	s.xclient.type         = ClientMessage;
	s.xclient.display      = app->dpy;
	s.xclient.window       = e->xclient.data.l[0];
	s.xclient.message_type = XdndStatus;
	s.xclient.format       = 32;
	s.xclient.data.l[0]    = xlib_window;
	s.xclient.data.l[1]    = (xlib_dnd->action ? 1 : 0) | (xlib_dnd->inw ? 2 : 0);
	s.xclient.data.l[2]    = (((unsigned short)rect.x)     << 16) | (unsigned short)rect.y;  // (x<<16)|y
	s.xclient.data.l[3]    = (((unsigned short)rect.width) << 16) | (unsigned short)rect.height;  // (w<<16)|h
	// s.xclient.data.l[2]    = (((unsigned short)xlib_dnd->inx) << 16) | (unsigned short)xlib_dnd->iny;  // (x<<16)|y
	// s.xclient.data.l[3]    = (((unsigned short)xlib_dnd->inw) << 16) | (unsigned short)xlib_dnd->inh;  // (w<<16)|h
	s.xclient.data.l[4]    = xlib_dnd->action;
	DBG cerr <<"   sent XdndStatus to "<<e->xclient.data.l[0]<<endl;
	XSendEvent(app->dpy, e->xclient.data.l[0], False, 0, &s);

	DndHoverMove(flatpoint(xx,yy), nullptr);
	return 0;
}

/*! Return whether we will take data based on action, and optionally
 * fill in rect with the region of the window this response pertains to.
 * If you do nothing with rect, it is assumed the whole window is relevant.
 * Set type_ret to the index of types for your preferred data type.
 *
 * If a child window under x,y will accept, then return that in child_ret.
 *
 * Default is to reject drop.
 */
bool anXWindow::DndWillAcceptDrop(int x, int y, const char *action, IntRectangle &rect, char **types, int *type_ret, anXWindow **child_ret)
{
	DBG cerr << "anXWindow::DndWillAcceptDrop: no (default)"<<endl;
	return false;
}

int anXWindow::HandleXdndDrop(XEvent *e)
{
    // This is sent from a source to the target saying that the drop
    // should actually happen now. The target then calls XConvertSelection()
    // which results in the source getting a SelectionRequest message.
    // The source then sends a SelectionNotify to the target, to say that
    // the target should check its selection window property for the data.

    // data.l[0] contains the XID of the source window.
    // data.l[1] is reserved for future use (flags).
    // data.l[2] contains the time stamp for retrieving the data. (new in version 1)

    DBG cerr <<"XdndDrop from "<<e->xclient.data.l[0]<<"  format:"<<e->xclient.format<<endl;

    if (xlib_dnd->status != DndState::Target_Accepts) return 0;

     //XConvertSelection, this sends a SelectionRequest to source, which updates prop on xlib_window
    Time timestamp = e->xclient.data.l[2];
    XConvertSelection(app->dpy,
                      XdndSelection,
                      XInternAtom(app->dpy, xlib_dnd->data_types[xlib_dnd->preferred_type], false),
                      LaxDndProperty, //XA_SECONDARY, // property name on target win that data gets put in
                      xlib_window,
                      timestamp);

     // if selection has owner, SelectionRequest sent to owner,
     // then this must wait for SelectionEvent?
     // else a non-send_event SelectionNotify sent to this.
    xlib_dnd->source_window = e->xclient.data.l[0]; //note this SHOULD have been the same as previous source_window from prev events

    DBG cerr << "  ..waiting for XConvertSelection to be ready..."<<endl;
    return 0;
}

int anXWindow::HandleXdndFinished(XEvent *e, bool accepted)
{
	if (!xlib_dnd) return 1;

	// XDndFinished:
	//
    // data.l[0] contains the XID of the target window.
    // data.l[1]:
    //     Bit 0 is set if the current target accepted the drop and successfully performed
    //     the accepted drop action. (new in version 5) (If the version being used by the
    //     source is less than 5, then the program should proceed as if the bit were set,
    //	   regardless of its actual value.)
    //     The rest of the bits are reserved for future use.
    // data.l[2] contains the action performed by the target. None should be sent if the 
    //     current target rejected the drop, i.e., when bit 0 of data.l[1] is zero. (new in version 5)
    //     (Note: Performing an action other than the one that was accepted with the last XdndStatus
    //     message is strongly discouraged because the user expects the action to match the visual
    //     feedback that was given based on the XdndStatus messages!)

	DBG cerr <<"Send XdndFinished to "<<xlib_dnd->source_window<<endl;

	XEvent s;
	s.xclient.type         = ClientMessage;
	s.xclient.display      = app->dpy;
	s.xclient.window       = xlib_dnd->source_window;
	s.xclient.message_type = XdndFinished;
	s.xclient.format       = 32;
	s.xclient.data.l[0]    = xlib_window;
	if (xlib_dnd->version >= 5) {
		s.xclient.data.l[1] = accepted;
		s.xclient.data.l[2] = xlib_dnd->action;
	}

	XSendEvent(app->dpy, xlib_dnd->source_window, False, 0, &s);
	DndHoverSuccess();
	
	return 0;
}


#endif //_LAX_PLATFORM_XLIB



} // namespace Laxkit

