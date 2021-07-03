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
//    Copyright (C) 2004-2011 by Tom Lechner
//



#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedata.h>
#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/language.h>
#include <lax/popupmenu.h>
#include <lax/lineedit.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>

// DBG !!!!!
#include <lax/displayer-cairo.h>


#include <iostream>
using namespace std;

#define DBG 


using namespace Laxkit;
namespace LaxInterfaces {
	
//---------------------------- ObjectContext -----------------------
/*! \class ObjectContext
 * \ingroup interfaces
 * \brief Class to make searching for objects convenient and expandable in a ViewportWindow.
 *
 * Interfaces should consider any ObjectContext it receives from a ViewportWindow
 * to be opaque, because there is no requirement that subclassing ObjectContext
 * preserves the meaning of anything other than ObjectContext::obj.
 */
/*! \fn int ObjectContext::isequal(const ObjectContext *oc)
 * \brief Return (oc && (i==oc->i)).
 */
/*! \var int ObjectContext::i
 * \brief Equals -1 for NULL obj.
 *
 * In ViewportWithStack, i is the index in the stack of the object.
 */
/*! \fn void ObjectContext::ObjectContext(int ii, SomeData *o)
 * \brief Set i=ii and obj=o. If o==NULL, then i is set to -1, and ii is ignored.
 */
/*! \fn ObjectContext::~ObjectContext()
 * \brief Empty virtual placeholder. obj is not deleted.
 */

ObjectContext::ObjectContext()
{
	i=-1;
	obj=NULL;
}

ObjectContext::ObjectContext(int ii, SomeData *o)
{
	i=ii;
	obj=o;
	if (obj) obj->inc_count();
}

ObjectContext::~ObjectContext()
{
	if (obj) { obj->dec_count(); obj=NULL; } 
}

/*! Decs count of old obj, incs count of o, if any.
 * Does not change anything but obj.
 */
void ObjectContext::SetObject(SomeData *o)
{
	if (obj==o) return; 
	if (obj) obj->dec_count();
	obj=o;
	if (obj) obj->inc_count();
}

int ObjectContext::Set(ObjectContext *oc)
{
	if (!oc) return 1;

	if (obj!=oc->obj) {
		if (obj) obj->dec_count();
		obj=oc->obj;
		if (obj) obj->inc_count();
	}
	i=oc->i;
	return 0;
}

/*! Decs count of old obj, incs count of o, if any.
 *
 * If !o, then i gets set to -1.
 */
void ObjectContext::set(int ii, SomeData *o)
{
	SetObject(o);
	if (o) i=ii; else i=-1;
}
	
void ObjectContext::clear()
{ 
	i=-1;
	if (obj) { obj->dec_count(); obj=NULL; } 
}

//! Return a duplicate of the context.
ObjectContext *ObjectContext::duplicate()
{
	ObjectContext *o=new ObjectContext(i,obj);
	return o;
}

//---------------------------- ViewportWindow -----------------------

/*! \class ViewportWindow
 * \brief Class specifically to use anInterface classes.
 *
 * ViewportWindow has a stack of anInterfaces. 
 * Input filters down from highest to lowest index of interfaces.
 * Also contains a Displayer and has basic screen orientation controls including
 * shifting, scaling, and rotation via the right button and Shift, Control, and
 * Shift+Control respectively. Any number of interfaces
 * can stack on this window. Key and mouse events are filtered down from the top
 * (highest index) of the stack to the bottom. Interfaces can choose to absorb
 * the event or not. The convention here is for 0 to be returned from an interface
 * if it does absorb the event.
 * 
 * The viewport can automatically notify rulers and scrollbars of changes to the 
 * viewport configuration. However, this class does not maintain those things itself.
 * See ViewerWindow or ScrolledWindow for a class that does
 * that. Any scroller and ruler here is assumed to be non-local, that is, they are not
 * created, held, or destroyed by this class.
 *
 * In terms of actual object handling, this class is really an empty shell. See
 * ViewportWithStack for a class that implements a plain stack of objects, and is
 * able to search through the list and also draw the items in its stack.
 * 
 * In general, a subclassed Viewport should (re)define these functions:
 *  NewData(),
 *  DeleteObject(),
 *  ChangeObject(),
 *  ChangeContext(),
 *  ObjectMoved(),
 *  FindObject(),
 *  FindObjects(),
 *  SelectObject(),
 *  transformToContext(double *m,ObjectContext *oc,int invert,int full).
 *
 * \todo must have some mechanism to notify anyone who wants to know where the mouse is,
 *   like mouse coordinate displays...***-> incorporate PanController that uses doubles?
 * \todo the zoom handles on scrollers propagate shearing errors.. need to make a double 
 *   scroller or something.
 * 
 * Window styles:
 * \code
 * #define VIEWPORT_NO_XSCROLLER    (1<<16)
 * #define VIEWPORT_NO_YSCROLLER    (1<<17)
 * #define VIEWPORT_NO_SCROLLERS    (1<<18)
 * #define VIEWPORT_NO_XRULER       (1<<19)
 * #define VIEWPORT_NO_YRULER       (1<<20)
 * #define VIEWPORT_NO_RULERS       (1<<21)
 * #define VIEWPORT_ROTATABLE       (1<<22)
 * #define VIEWPORT_BACK_BUFFER     (1<<23)
 * #define VIEWPORT_RIGHT_HANDED    (1<<24)
 * \endcode
 */
/*! \var Laxkit::RefPtrStack<anInterface> ViewportWindow::interfaces
 * \brief Stack of interfaces acting on the current display.
 * 
 * It is important for viewport to have its own instances of the interfaces. The usual method 
 * implemented by this class together with ViewerWindow is for ViewerWindow to keep a stack
 * of possible interfaces (tools), and push and pop them to this->interfaces.
 */
/*! \var Laxkit::Displayer *ViewportWindow::dp
 * \brief This a local dp that is used by the interfaces acting on the window.
 *
 * It is a pointer rather than a reference so that viewports are not restricted to
 * the default xlib based Displayer.
 * 
 * Interfaces would hold a reference to this dp, but by default it is the repsponsibility of 
 * the ViewportWindow to create and delete the dp-> This of course assumes that the ViewportWindow
 * and its dp outlast all the interfaces.
 */


//! Constructor, pushes itself onto panner's tell stack.
/*! If ndp is not null, then use that Displayer rather than the default xlib
 * based Displayer. The pointer is transfered to this viewport (not copied or duplicated),
 * and is deleted in the destructor.
 */
ViewportWindow::ViewportWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder, Laxkit::Displayer *ndp)
		: PanUser(NULL),
		  anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder, NULL,0,NULL)
		  
{
	 //make our own local copy of colors
	win_themestyle = new WindowStyle;
	*win_themestyle = *app->theme->GetStyle(THEME_Edit);

	xruler=yruler=NULL;
	xscroller=yscroller=NULL;

	interfacemenu=-1; //used in RBDown when there is a popup menu

	dp=ndp;
	if (!dp) dp=newDisplayer(this);
	if (win_style&VIEWPORT_RIGHT_HANDED) {
		dp->defaultRighthanded(true);
		dp->NewTransform(1,0,0,-1,0,0);
	}
	
	panner->pan_style|=PANC_SYNC_XY|PANC_ALLOW_SMALL|PANC_PAGE_IS_PERCENT|PANC_ELEMENT_IS_PERCENT;
	panner->tell(dynamic_cast<anXWindow *>(this));
	dp->SetSpace(-500,500,-500,500);
	dp->UseThisPanner(panner); // displayer should sync panner to the values in displayer, not the other way around.

	firsttime=1;
	searchtype=NULL;
	searchx=searchy=0;

	 //ui helper info
	view_shift_amount=3;
	last_mouse=0; //this is a hack to work around the abstracting done for shortcuts/WindowActions

	sc=NULL;

	selection=NULL;

	copysource=NULL;
	pastedest=NULL;

	temp_input=NULL;
	temp_input_label=NULL;
	temp_input_interface=0;
}

//! Deletes dp.
ViewportWindow::~ViewportWindow()
{
	DBG cerr <<" --in ViewportWindow  destructor"<<endl;
	
	delete[] temp_input_label;
	delete dp;

	if (sc) sc->dec_count();
	if (selection)  selection ->dec_count();
	if (copysource) copysource->dec_count();
	if (pastedest)  pastedest ->dec_count();
}

//! Default is app->postmessage(mes), unless parent is ViewerWindow, then try to set its message bar to mes.
/*! \todo *** implement a printf-ish postmessage("format string %d",i,....)
 */
void ViewportWindow::PostMessage(const char *mes)
{
	ViewerWindow *viewer=dynamic_cast<ViewerWindow *>(win_parent); // maybe not always returns non-null
	if (viewer) viewer->PostMessage(mes);
	else app->postmessage(mes);
}

//! Return the extra transform needed to transform points in oc to viewer space.
/*! dp->Getctm() holds a transform mapping viewer space to screen space.
 * The transform mapping context space to screen space would be m*dp->Getctm().
 *
 * If m is NULL, then return a new double[6]. Otherwise, m is assumed to be a double[6],
 * and the transform is put there.
 *
 * The default here is just to return identity.
 *
 * If (invert) then m will map points from viewer space to context space.
 *
 * If (!full), then return the transform up to but not including the object in oc.
 * For instance, frequently, one deals with a context describing a particular object, but
 * the transform one wants is to the space containing that object, not the object space
 * itself, so one would pass full==0 in that case. This exists so that one may use
 * SomeData::pointin() easily, because that function takes points in object parent space,
 * not object space. If full<0 then use the entire context. If full>0, then use only that
 * number of first components.
 */
double *ViewportWindow::transformToContext(double *m,ObjectContext *oc,int invert,int full)
{
	if (!m) m=new double[6];
	return transform_identity(m);
}

/*! true for yes, false for no.
 *
 * Subclasses need to redefine this to return something meaningful.
 * This placeholder just returns true.
 */
bool ViewportWindow::IsValidContext(ObjectContext *oc)
{
	cerr << "Warning! Using unredefined ViewportWindow::IsValidContext()!"<<endl;
	return true;
}

/*! Return what oc points to, if anything. Note this is maybe NOT oc->obj.
 * Usually, use this function to help check validity of the oc.
 */
SomeData *ViewportWindow::GetObject(ObjectContext *oc)
{
	cerr << "Warning! Using unredefined ViewportWindow::GetObject(ObjectContext *oc)!"<<endl;
	return NULL;
}

/*! Return a pointer to the current context. Calling code should not 
 * delete or do anything else with the returned context. It is owned by the viewport.
 */
ObjectContext *ViewportWindow::CurrentContext()
{
	return NULL;
}

/*! Make sure references in sel actually point to something.
 * Note this modifies the contexts in sel, and removes any that are bad.
 *
 * If sel==NULL, then use this->selection.
 */
int ViewportWindow::UpdateSelection(Selection *sel)
{
	cerr << "Warning! Using unredefined ViewportWindow::UpdateSelection()!"<<endl;
	return -1;
}

//! Call this to update the context to correspond to screen coordinate (x,y).
/*! Default here is to set *co=NULL (if oc!=NULL).
 *
 * oc will be set to point to the new context.
 *
 * Derived classes would change the current context to refer to a page different
 * from previous page, for instance. Interfaces would call this when there is a click
 * on blank space. The ViewportWindow then updates accordingly, and then the 
 * interface can set up new data according to this new context.
 * 
 * Return 0 for context changed, nonzero for not.
 */
int ViewportWindow::ChangeContext(int x,int y,ObjectContext **oc)
{
	if (oc) *oc=NULL;
	return 0;
}

//! Call this to update the context to correspond to oc.
/*! This is a reduced form of NewData(), for when you simply
 * want to change context, rather than introduce new data.
 * The contents of oc must not be modified.
 *
 * The default here is just an empty placeholder.
 * Return 0 for context changed.
 */
int ViewportWindow::ChangeContext(ObjectContext *oc)
{
	return 0;
}

//! Change the current object to be (already existing object) at oc.
/*! Returns 1 for current object changed, otherwise 0 for not changed or bad context.
 *
 * This function is an empty placeholder, and just returns 0.
 * 
 * \todo *** need mechanism to pass the LBDown grab to the object's interface.. This function
 * is most often called when interfaces cannot find objects of their own to work on on an LBDown.
 */
int ViewportWindow::ChangeObject(ObjectContext *oc, int switchtool, bool update_selection)
{
	return 0;
}


//! Used when an interface wants to insert a brand new object d at the current context.
/*!
 *  The index of where d is placed in the current context is returned, or if there is
 * an error, -1 is returned and no object inserted. If you have an object that was found
 * from a viewport search, then you do not want to use this function. This function is 
 * for brand new data only. Re-inserting already existing objects will most likely
 * make things shift around in unexpected ways. Note that this will most likely not crash the
 * program, since the objects are reference counted.
 *
 * Subclasses must redefine this to be something useful, such as to push the
 * data onto the appropriate page (or whereever).
 *
 * If oc_ret!=NULL, then an ObjectContext is returned there. This should be copied with
 * oc->duplicate(), as usually it is stored internally by the viewport.
 *
 * This default function does nothing, and returns -1.
 *
 * If clear_selection, then selection should be cleared, and then populated with only the new data.
 * Otherwise, the new data is added to the selection.
 */
int ViewportWindow::NewData(SomeData *d, ObjectContext **oc_ret, bool clear_selection)
{
	return -1;
}

	
//! Delete the current object.
/*! This is a placeholder, and only calls anInterface::Clear() on all current interfaces.
 *  Derived classes should redefine to do something useful, which might not be calling
 *  Clear() on the interfaces!
 *
 * Return 1 if object deleted, 0 if not deleted, -1 if there was no current object.
 */
int ViewportWindow::DeleteObject()
{
	for (int c=0; c<interfaces.n; c++) interfaces.e[c]->Clear();
	needtodraw=1;
	return 1;
}


/*! Called when moving objects around, this ensures that a move can
 * make the object go to a different page, for instance. Typically,
 * interfaces will call this after dragging, when the button is released.
 *
 * Returns NULL if no modifications. If the context does change, then return an object
 * with the new context. In this case, if modifyoc, then oc is changed to the new context,
 * and oc is returned. If modifyoc==0, then return a brand new ObjectContext object.
 * 
 * If oc==NULL and modifyoc==1, then NULL is returned.
 *
 * Default here does nothing but return NULL. Subclasses should redefine to do something meaningful.
 */
ObjectContext *ViewportWindow::ObjectMoved(ObjectContext *oc, int modifyoc)
{ return NULL; }

//! Select previous (i==-2) or next (i==-1) object. Return 1 for current object changed, 0 for not changed.
/*! This function is a dummy placeholder. It does nothing here. It is not used by
 * the interfaces, and within the Laxkit is only called from CharInput in this class in order
 * to select previous and next object.
 *
 * Return 0 for no change, nonzero for change.
 */
int ViewportWindow::SelectObject(int i)
{
	return 1;
}

/*! Warning, may be NULL.
 */
Selection *ViewportWindow::GetSelection()
{
	return selection;
}

/*! Passing NULL will clear the selection as known by the viewport.
 */
int ViewportWindow::SetSelection(Selection *nselection)
{
	if (selection==nselection) return 0;

	if (!nselection) {
		if (selection) selection->Flush();
	} else {
		if (selection) selection->dec_count();
		selection=nselection;
		selection->inc_count();
	}
	return 0;
}

int ViewportWindow::selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type, const char *which)
{
	for (int c=0; c<interfaces.n; c++) {
		if (interfaces.e[c]->Paste((const char *)data,len, NULL, actual_type)==0) { 
			//DBG cerr <<"interface "<<interfaces.e[c]->whattype()<<" needs to draw "<<interfaces.e[c]->needtodraw<<endl;
			needtodraw=1; 
			return 0;
		}
	}

	return 1;
}

int ViewportWindow::PasteRequest(anInterface *interf, const char *targettype)
{
	if (pastedest != interf) {
		if (pastedest) pastedest->dec_count();
		pastedest=interf;
		if (pastedest) pastedest->inc_count();
	}

	selectionPaste(0, targettype);
	return 0;
}

int ViewportWindow::SetCopySource(anInterface *source)
{
	if (copysource != source) {
		if (copysource) copysource->dec_count();
		copysource=source;
		if (copysource) copysource->inc_count();
	}

	selectionCopy(0);
	return 0;
}

//! Return a list of all the objects within box.
/*! If real, then box contains real bounds, rather than screen bounds.
 *  If ascurobj, then grab only those objects in box, and at the same base level
 *  as curobj.
 *
 *  Returns the number of objects found. If none are found, data_ret and c_ret are
 *  set to NULL. Otherwise they (if the pointer is provided) are set to a null terminated
 *  list of the objects (so it is an array of 1 more than the number the function returns),
 *  and the corresponding object contexts. The contexts are local
 *  objects to the list, so the calling code is responsible for deleting them. The data
 *  are not local, nor are their counts incremented, so the calling code should do something
 *  with them quick before they are deleted.
 *
 *  This function is very specific to the actual viewport, so the default here is to
 *  return no objects. Subclasses don't really have to redefine this function unless
 *  they really want provide this sort of ability. The only interface that tries to
 *  access this is ObjectInterface.
 *
 *  \todo need flag for IN box or just TOUCHING box
 */
int ViewportWindow::FindObjects(Laxkit::DoubleBBox *box, char real, char ascurobj,
								SomeData ***data_ret, ObjectContext ***c_ret)
{
	if (data_ret) *data_ret=NULL;
	if (c_ret) *c_ret=NULL;
	return 0;
}

/*! \fn int ViewportWindow::FindObject(int x,int y, const char *dtype, 
 *			SomeData *exclude, int start,ObjectContext **oc, int searcharea)
 * \brief Iteratively find an object with whattype dtype under screen position (x,y).
 *
 * This default function does nothing but set oc to NULL and returns 0. It should be
 * redefined in subclasses.
 *
 * Interfaces expect this function to behave as follows:
 *
 * \todo *** this needs a little more clarification...
 *  
 * This function is used to progressively find objects that may be hidden
 * but are still under screen point (x,y). If start==0 and *oc points to something,
 * then the search starts with the object immediately after oc.
 *
 * If dtype==NULL, then search for any object under (x,y), otherwise restrict search
 * to dtype objects, where dtype is data->whattype().
 *
 * Returns 1 if a suitable object is found and oc is set to it. If the search is over and there were
 * no suitable objects found, 0 is returned, and oc set to NULL. If the search is over but there was 
 * a matching object found of a different type, then -1 is returned and oc is set to refer to
 * that object. The oc returned in this way must be owned by the ViewportWindow, and must
 * not be deleted by whatever calls here.
 * 
 * If an object matching the given (x,y) was found, but was of the wrong type, 
 * then that object will be available returned in oc, and -1 is returned.
 *
 * If an interfaces receives a lbdown outside of their object, then it would
 * call viewport->FindObject, which will possibly return an object that the 
 * interface can handle. If so, the interface would call ChangeObject(data,0) with it. The interface
 * can keep searching until it finds one it can handle.
 * If FindObject does not return something the interface can handle, then
 * the interface should call ChangeObject(that other data, 1), which should result in
 * another interface being set to work on that data.
 * 
 * ViewportWindow keeps the following convenience variables about the search hanging around.
 * Derived classes with their own search can use them however they want:
 * <pre>
 * startx,starty: the x and y position of the search
 * searchtype:    the whattype string for a restricted search
 * </pre>
 *
 * If start!=0, OR x!=searchx OR y!=searchy then
 * initialize the search so that exclude becomes firstobj. If start==1, then
 * skip this new firstobj. If start==2 or start==0 (but either x or y are different), 
 * then include exclude in the search. 
 *
 * If this is not the start of a search, exclude will always be skipped.
 *
 * Subclasses should ensure that searching does not go on infinitely, by checking against
 * a first object. If a search steps through all objects without success, then NULL should be returned from then on.
 *
 * No actual viewport state should be changed, other than the search aids. This function only returns what is asked for.
 * An interface may then take what is returned and tell the Viewport to make that the
 * current object, if it likes. 
 */
int ViewportWindow::FindObject(int x,int y, const char *dtype, 
					SomeData *exclude, int start,ObjectContext **oc, int searcharea)
{
	if (oc) *oc=NULL;
	return 0;
}


/*! Calls:
 * \code
 *	dp->WrapWindow(this);
 *	dp->CenterReal();
 *	dp->Newmag(1);
 *	dp->syncPanner();
 *	syncWithDp();
 *
 *	XSetWindowBackground(app->dpy,window,win_xatts.background_pixel);
 *	dp->NewFG(255,255,255);
 *	dp->NewBG(0,0,0);
 *
 *	if (win_style&VIEWPORT_BACK_BUFFER) SetupBackBuffer();
 * \endcode
 */
int ViewportWindow::init()
{
	if (!xlib_window) return 1;

	dp->WrapWindow(this);
	dp->CenterReal();
	dp->Newmag(1);
	dp->syncPanner();
	syncWithDp();

	//syncrulers();

	dp->NewFG(255,255,255);
	dp->NewBG(0,0,0);

	return 0;
}

//! Set the viewport space bounds.
/*! If the space is allowed to be rotated, then the space bounds
 * may not translate directly to screen bounds. The screen bounds of the
 * space will be larger to accomodate the rotated rectangle.
 *
 * \todo *** return value? success? on change?
 */
int ViewportWindow::SetSpace(double minx,double maxx,double miny, double maxy)
{
	dp->SetSpace(minx,maxx,miny,maxy);
	syncWithDp();

	return 0;
}

//! Tell the viewport to coordinate with these scrollers, replacing the current ones.
int ViewportWindow::UseTheseScrollers(Scroller *x,Scroller *y)
{ 
	if (x) {
		if (xscroller) panner->tellPop(xscroller);
		xscroller=x;
		xscroller->SetOwner(this,"xscroller"); // rely on panner to send messages
		xscroller->UseThisPanner(panner);
	}
	if (y) {
		if (yscroller) panner->tellPop(yscroller);
		yscroller=y;
		yscroller->SetOwner(this,"yscroller"); // rely on panner to send messages
		yscroller->UseThisPanner(panner);
	}
	dp->syncPanner(1);
	syncWithDp();

	return 0;
}

//! Use these x and y rulers.
/*! ViewportWindow configures the rulers to auto track the mouse
 * wherever the window is in relation to the viewport.
 */
int ViewportWindow::UseTheseRulers(RulerWindow *x,RulerWindow *y)
{
	xruler=x;
	if (x) {
		 // set up so the offset is correct, assign owner, etc.
		x->SetOwner(this,"xruler");
		x->TrackThisWindow(this);	
	}
	yruler=y;
	if (y) {
		 // set up so the offset is correct, assign owner, etc.
		y->SetOwner(this,"yruler");
		y->TrackThisWindow(this);	
		if (!dp->righthanded()) y->win_style|=RULER_UP_IS_POSITIVE;
	}
	syncWithDp();
	return 0;
}

//! Return whether the viewport needs refreshing.
/*! Currently, if any of the interfaces have needtodraw, then return 1.
 */
int ViewportWindow::Needtodraw()
{
	//DBG if (needtodraw) cerr <<"  Yes, it needs to draw.\n"; else cerr <<"  viewport doesn't need to draw"<<endl;
	if (needtodraw) return needtodraw;
	for (int c=0; c<interfaces.n; c++) {
		if (interfaces.e[c]->needtodraw) { 
			//DBG cerr <<"interface "<<interfaces.e[c]->whattype()<<" needs to draw "<<interfaces.e[c]->needtodraw<<endl;
			needtodraw=1; 
		}
	}
	return needtodraw;
}

//! Deal with messages, usually scroller events.
/*! Relay "pan change" events to the displayer, and potentially respond to ruler events.
 *  Scroller and PanPopup changes are sent via the panner. The panner sends
 * the "pan change" in response to those changes.
 *
 * Subclass should also respond to "xruler" and "yruler" events if they want to.
 */
int ViewportWindow::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onUnmapped) {
		for (int c=0; c<interfaces.n; c++) { interfaces.e[c]->Unmapped(); }
		return anXWindow::Event(e,mes);

	} else if (e->type==LAX_onMapped) {
		for (int c=0; c<interfaces.n; c++) { interfaces.e[c]->Mapped(); }
		return anXWindow::Event(e,mes);
	}

	if (!strcmp(mes,"pan change")) {
		dp->syncFromPanner();
		syncrulers();
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"viewportmenu")) {
		if (interfacemenu<0 || interfacemenu>=interfaces.n) return 0;
		interfaces.e[interfacemenu]->Event(e,"menuevent");
		needtodraw|=interfaces.e[interfacemenu]->Needtodraw();
		return 0;

	} else if (!strcmp(mes,"xruler") || !strcmp(mes,"yruler")) {
		 //units change from ruler
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 0;
		if (s->info1!=RULER_Units) return 0; //only deal with units change
		if (xruler) xruler->SetCurrentUnits(s->info2);
		if (yruler) yruler->SetCurrentUnits(s->info2);
		return 0;
		
	}
	
	return anXWindow::Event(e,mes);
}

//! Return whether an interface with the given id is on the interfaces stack.
/*! Return value is index+1 if found, or 0 if not.
 */
int ViewportWindow::HasInterface(int iid)
{
	for (int c=0; c<interfaces.n; c++) 
		if (interfaces.e[c]->id==iid) return c+1;
	return 0;
}

//! Return whether an interface with the given whattype is on the interfaces stack.
/*! Return value is index+1 if found, or 0 if not.
 */
anInterface *ViewportWindow::HasInterface(const char *name, int *index_ret)
{
	for (int c=0; c<interfaces.n; c++)  {
		if (!strcmp(interfaces.e[c]->whattype(),name)) {
			if (index_ret) *index_ret=c;
			return interfaces.e[c];
		}
	}
	if (index_ret) *index_ret=-1;
	return NULL;
}

//! Push i onto the stack, call i->Dp(dp), then i->InterfaceOn();
/*!  Try to insert at position where.
 *
 * \todo If the interface is already on the stack, then it and any descendents are brought to the top.
 * *** that is not implemented yet, not sure if it should be.. just pushnodup's for now.
 */
int ViewportWindow::Push(anInterface *i,int where, int absorbcount)
{
	if (!i) return 1;
	DBG cerr <<".....pushing "<<i->whattype()<<endl;
	if (interfaces.pushnodup(i,LISTS_DELETE_Refcount,where)>=0) { 
		// i was already on the stack
	}
	if (absorbcount) i->dec_count();
	i->Dp(dp);
	i->InterfaceOn();
	needtodraw = 1;
	return 0;
}

//! Search for the interface with the given id, then remove and return it.
anInterface *ViewportWindow::PopId(int iid,char deletetoo)//deletetoo=0
{
	for (int c=0; c<interfaces.n; c++)
		if (iid==interfaces.e[c]->id) return Pop(interfaces.e[c],deletetoo);
	return NULL;
}

//! Call i->InterfaceOff(), Pop i and children of i, and either return i, or delete it.
/*! If the item was pushed with a local=0, then the item is not deleted.
 * If deletetoo==0, then return the popped interface.
 *
 * This pops off not only the interface, but also any descendents of it that happen
 * to be on the interfaces stack (i->child, i->child->child, etc.).
 * It first calls i->InterfaceOff() then tries to remove any remaining children
 * (calls interfaces.remove(thechildinterface)
 */
anInterface *ViewportWindow::Pop(anInterface *i,char deletetoo)//deletetoo=0
{
	int c=interfaces.findindex(i);
	if (c<0) return NULL;
	DBG cerr <<".....popping "<<i->whattype()<<endl;
	i->InterfaceOff();

	if (i==copysource) { copysource->dec_count(); copysource=NULL; }
	if (i==pastedest)  { pastedest ->dec_count(); pastedest=NULL;  }

	 //Remove child interfaces
	anInterface *d, *p;
	while (i->child) { 
		 // This child removal process is a little convoluted in order to protect
		 // against when an interface has children that may not be on the interfaces stack.
		 // This would occur when an interface keeps a child private for some reason, but the
		 // child of the child is on the stack and must be removed...
		 // not sure if that is even a useful scenario, but just in case...
		 // Also handles when i->InterfaceOff() does not remove the children from interfaces itself.
		p=i;
		d=i->child;
		while (d->child) { p=d; d=d->child; } //go to lowest child

		d->owner=NULL; //isolate this child for removal
		Pop(d,deletetoo); // if d not on stack, Pop just returns, but we still need to check further children..
		p->child=NULL;
	}

	 //Remove this
	if (i->owner) { i->owner->child=NULL; i->owner=NULL; }
	if (deletetoo) { interfaces.remove(interfaces.findindex(i)); return NULL; }

	interfaces.popp(i);
	needtodraw = 1;
	return i;
}

//! Draw things before doing interface refreshes.
/*! This is called from the default ViewportWindow::Refresh() to simplify
 * subclassing refreshing.
 */
void ViewportWindow::RefreshUnder()
{}

//! Draw things after doing interface refreshes.
/*! This is called from the default ViewportWindow::Refresh() to simplify
 * subclassing refreshing.
 */
void ViewportWindow::RefreshOver()
{}


//! Default refresh just refreshes the interfaces.
/*!
 * Here is a very basic template for a Refresh function:
 * \code
 *   //ensure drawing if an interface needs to draw
 *  for (int c=0; c<interfaces.n; c++) interfaces.e[c]->needtodraw=1;
 *  
 *  if (needtodraw) {
 *    dp->StartDrawing(this);
 *    
 *    --- draw all the stuff ---
 *
 *    dp->EndDrawing();
 *    if (backbuffer) SwapBuffers();
 *  }
 *  
 *  needtodraw=0;
 * \endcode
 */
void ViewportWindow::Refresh()
{
	//DBG cerr <<"ViewportWindow default refresh "<<getUniqueNumber()<<endl;

	dp->StartDrawing(this);

	dp->Updates(0);

	dp->NewBG(win_themestyle->bg);
	dp->ClearWindow();
	int c;
	for (c=0; c<interfaces.n; c++) interfaces.e[c]->needtodraw=1;//***
	
	
	if (needtodraw) {
		 // Refresh interfaces, should draw whatever SomeData they have
		//DBG cerr <<"  drawing interface..";

		RefreshUnder();

		ObjectContext *oc;

		 //refresh normal interfaces
		for (c=0; c<interfaces.n; c++) {
			if (interfaces.e[c]->interface_type==INTERFACE_Overlay) continue;
			if (interfaces.e[c]->Needtodraw()) {
				//cout <<" \ndrawing "<<c;
				oc=interfaces.e[c]->Context();
				if (oc) {
					double m[6];
					transformToContext(m,oc,0,1);
					dp->PushAndNewTransform(m);
				}
				interfaces.e[c]->Refresh();
				if (oc) { dp->PopAxes(); oc=NULL; }
			}
		}
		 
		 //refresh overlays after everything else
		for (c=0; c<interfaces.n; c++) {
			if (interfaces.e[c]->interface_type!=INTERFACE_Overlay) continue;
			if (interfaces.e[c]->Needtodraw()) {
				//cout <<" \ndrawing "<<c;
				oc=interfaces.e[c]->Context();
				if (oc) {
					double m[6];
					transformToContext(m,oc,0,1);
					dp->PushAndNewTransform(m);
				}
				interfaces.e[c]->Refresh();
				if (oc) { dp->PopAxes(); oc=NULL; }
			}
		}

		RefreshOver();

		if (temp_input && temp_input_label) {
			dp->DrawScreen();
 			dp->NewBG(temp_input->win_themestyle->bg);
 			dp->NewFG(temp_input->win_themestyle->fg);
			int th=dp->textheight();
			dp->drawRoundedRect(temp_input->win_x-5,temp_input->win_y-1.2*th-5,          
	                            temp_input->win_w+temp_input->win_border*2+10, 1.2*th+temp_input->win_h+temp_input->win_border*2,
								5, 0, 5, 0, 2);
 			dp->NewFG(temp_input->win_themestyle->fg);
			dp->textout(temp_input->win_x,temp_input->win_y, temp_input_label,-1, LAX_LEFT|LAX_BOTTOM);
			dp->DrawReal();
		}
		
		 // draw real axes length 10
		//dp->drawaxes(10);
		
	} // if needtodraw

	dp->EndDrawing();
	dp->Updates(1);
	SwapBuffers();
	
	//DBG cerr <<" All done default refreshing.\n";
	needtodraw=0;
}

/*! Redefined from anXWindow, we need to catch when we have an input window up..
 */
int ViewportWindow::deletekid(anXWindow *w)
{
	if (!temp_input || w!=temp_input) return anXWindow::deletekid(w);

	temp_input=NULL;
	temp_input_interface=0;
	needtodraw=1;
	win_style|=temp_grab;
	if (temp_grab&ANXWIN_HOVER_FOCUS) app->setfocus(this, 0, NULL);

	return anXWindow::deletekid(w);
}

void ViewportWindow::ClearInputBox()
{
	if (temp_input) {
		app->destroywindow(temp_input);
		temp_input = NULL;
	}
}

/*! Return NULL if temp_input already there. Else return the newly created window.
 * If send_controls, send on pressing up, down, or tab.
 */
Laxkit::anXWindow *ViewportWindow::SetupInputBox(unsigned long owner_id, const char *label, const char *text, const char *message,
									const Laxkit::DoubleBBox &bounds, const char *ntooltip, bool send_controls)
{
	if (temp_input) return NULL;

	 //1. set up a LineEdit to get some input
	 //2. temporarily toggle off viewport grab mode if necessary, to keep input in the edit
	int border = 3;
	int x=bounds.minx-border, y=bounds.miny-border, w=bounds.maxx-bounds.minx, h=bounds.maxy-bounds.miny;
	if (fabs(h) < 1e-5) {
		h = 1.2 * win_themestyle->normal->textheight();
		y -= h/2;
	}

	if (y+h>win_h) y-=y+h-win_h+8;
	else if (y - (label ? app->defaultlaxfont->textheight() : 0)<0) y = (label ? app->defaultlaxfont->textheight() : 0);
	if (x+w>win_w) x-=x+w-win_w;
	else if (x<0) x=0;

	LineEdit *le= new LineEdit(this, label,label,
								(send_controls ? LINEEDIT_SEND_CONTROLS : 0)
								 | LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE|ANXWIN_OUT_CLICK_DESTROYS|ANXWIN_HOVER_FOCUS,
								x,y,w,h, border,
								NULL,owner_id,message,
								text);
	if (ntooltip) le->tooltip(ntooltip);
	le->padx=le->pady=dp->textheight()*.1;
	le->SetSelection(0,-1);
	app->addwindow(le);

	temp_input=le;
	temp_input_interface=owner_id;
	makestr(temp_input_label, label);

	temp_grab = win_style&ANXWIN_HOVER_FOCUS;
	win_style &= ~ANXWIN_HOVER_FOCUS;

	needtodraw=1;
	return le;
}

//! Scroll, and zoom.
/*! Plain wheel scrolls vertically, faster with shift.
 * Control-wheel scrolls horizontally, faster with shift.
 * Alt-wheel zooms.
 */
int ViewportWindow::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	for (int c=interfaces.n-1; c>=0; c--) { //***
		if (!interfaces.e[c]->WheelUp(x,y,state,count,d)) 
			return 0;
	}
	int w;
	if ((state&LAX_STATE_MASK)==ControlMask) { // alt-but zoom
		double z=1.15;
//cout <<"..Zoom:"<<z<<"  ";
		dp->Zoom(z,x,y);
		syncWithDp();
		needtodraw=1;
	} else if ((state&LAX_STATE_MASK)==ShiftMask) { // shift x
		//if (state&ShiftMask) w=win_w*1/2;
		//else w=10;
		w=20;
		dp->ShiftScreen(w,0);
//		if (xscroller) xscroller->SetCurPos(xscroller->GetCurPos()+w);
		syncrulers();
		needtodraw=1;
	} else {
		//if (state&ShiftMask) w=win_h*1/2;
		//else w=10;
		w=20;
		dp->ShiftScreen(0,w);
//		if (yscroller) yscroller->SetCurPos(yscroller->GetCurPos()+w);
		syncrulers();
		needtodraw=1;
	}
	return 0;
}

//! Scroll, and zoom.
/*! Plain wheel scrolls vertically, faster with shift.
 * Control-wheel scrolls horizontally, faster with shift.
 * Alt-wheel zooms.
 */
int ViewportWindow::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	for (int c=interfaces.n-1; c>=0; c--) { //***
		if (!interfaces.e[c]->WheelDown(x,y,state,count,d)) 
			return 0;
	}
	int w;
	if ((state&LAX_STATE_MASK)==ControlMask) { // alt-but zoom
		double z=1/1.15;
//cout <<"..Zoom:"<<z<<"  ";
		dp->Zoom(z,x,y);
		syncWithDp();
		needtodraw=1;
	} else if ((state&LAX_STATE_MASK)==ShiftMask) { // shift x
		//if (state&ShiftMask) w=-win_w*1/2;
		//else w=-10;
		w=-20;
		dp->ShiftScreen(w,0);
//		if (xscroller) xscroller->SetCurPos(xscroller->GetCurPos()+w);
		syncrulers();
		needtodraw=1;
	} else {
		//if (state&ShiftMask) w=-win_h*1/2;
		//else w=-10;
		w=-20;
		dp->ShiftScreen(0,w);
//		if (yscroller) yscroller->SetCurPos(yscroller->GetCurPos()+w);
		syncrulers();
		needtodraw=1;
	}
	return 0;
}

/*! Relay LBDown to the interfaces. If not interfaces take it, then try to select an object
 * at screen coordinate (x,y).
 * 
 * \todo *** need mechanism to pass the LBDown grab to the object's interface.. This function
 * is most often called when interfaces cannot find objects of their own to work on on an LBDown.
 * most likely, this would require a flag for ChangeObject, which would get called from 
 * the interface...?
 */
int ViewportWindow::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	DBG cerr <<"in ViewportWindow::lbdown:"<<x<<','<<y<<".."<<endl;


	for (int c=interfaces.n-1; c>=0; c--) {
		if (interfaces.e[c]->interface_type!=INTERFACE_Overlay) continue;
		if (!interfaces.e[c]->LBDown(x,y,state,count,d)) {
			buttondown.down(d->id,LEFTBUTTON, x,y, state, interfaces.e[c]->id);
			return 0; //interface claimed the event
		}
	}

	for (int c=interfaces.n-1; c>=0; c--) {
		if (interfaces.e[c]->interface_type==INTERFACE_Overlay) continue;
		if (!interfaces.e[c]->LBDown(x,y,state,count,d)) {
			buttondown.down(d->id,LEFTBUTTON, x,y, state, interfaces.e[c]->id);
			return 0; //interface claimed the event
		}
	}

	
	buttondown.down(d->id,LEFTBUTTON, x,y, state, 0);

	 // if here, then mouse down on nothing, so try to select an object...
	ObjectContext *oc;
	int c=FindObject(x,y,NULL,NULL,1,&oc);
	if (c==1) ChangeObject(oc,1, true);

	DBG cerr <<"../ViewportWindow::lbdown\n";
	return 0;
}

//! Relay LBUp to interfaces.
int ViewportWindow::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"  ViewportWindow::lbup.."<<endl;

	buttondown.up(d->id,LEFTBUTTON);
	for (int c=interfaces.n-1; c>=0; c--) {
		if (interfaces.e[c]->interface_type!=INTERFACE_Overlay) continue;
		if (!interfaces.e[c]->LBUp(x,y,state,d)) 
			return 0; //interface claimed the event
	}

	for (int c=interfaces.n-1; c>=0; c--) {
		if (interfaces.e[c]->interface_type==INTERFACE_Overlay) continue;
		if (!interfaces.e[c]->LBUp(x,y,state,d)) 
			return 0; //interface claimed the event
	}
	DBG cerr <<".lbupDone "<<endl;
	return 0;
}

//! Relay MBDown to interfaces.
/*! \todo *** have special behavior for middle button, which affects how the mouse
 *    wheel behaves. mb double click toggle between scroll up/down, scroll
 *    right/left, rotate, or zoom in/out
 */
int ViewportWindow::MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"in ViewportWindow::mbdown..";
	for (int c=interfaces.n-1; c>=0; c--) { //***
		if (!interfaces.e[c]->MBDown(x,y,state,count,d)) 
			return 0; //interface claimed the event
	}
	buttondown.down(d->id,MIDDLEBUTTON, x,y,state);
	needtodraw=1;
	DBG cerr <<"..mbdownDone\n";
	return 0;
}

//! Relay MBUp to interfaces.
int ViewportWindow::MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"  ViewportWindow::mbup..";
	if (!buttondown.isdown(d->id,MIDDLEBUTTON)) {
		for (int c=interfaces.n-1; c>=0; c--) { //***
			if (!interfaces.e[c]->MBUp(x,y,state,d)) 
				return 0; //interface claimed the event
		}
	}
	buttondown.up(d->id,MIDDLEBUTTON);
	DBG cerr <<".mbupDone " <<endl;
	return 0;
}

//! Relay RBUp to interfaces.
/*! If no interfaces claim it, then plain click pops up the context menu (***not imp),
 * shift-drag moves viewport, control-drag scales viewport, control-shift-drag rotates the screen.
 */
int ViewportWindow::RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"in ViewportWindow::rbdown..\n";

	for (int c=interfaces.n-1; c>=0; c--) { //***
		if (!interfaces.e[c]->RBDown(x,y,state,count,d)) 
			return 0; //interface claimed the event
	}
	buttondown.down(d->id,RIGHTBUTTON, x,y,state);

	if (state&ShiftMask && state&ControlMask && win_style&VIEWPORT_ROTATABLE) { // rotate
	} else if (state&ControlMask) {	// zoom
	} else if (state&ShiftMask) { // shift screen 
	} else { // no state mask
		 
		 //pop up a context sensitive menu
		DBG cerr << "  Menu.."<<endl;
		MenuInfo *menu=NULL;
		for (int c=interfaces.n-1; c>=0; c--) { 
			 //***use the first interface that returns a menu.
			 //   in future, this shoulde lump all that return a menu
			 //   into one menu...
			//menu=interfaces.e[c]->ContextMenu(x,y,d->id, NULL); 
			if (interfaces.e[c]->owner==NULL) menu=interfaces.e[c]->ContextMenu(x,y,d->id,NULL); 
			if (menu && menu->n()>0) {
				interfacemenu=c;
				break;
			}
		}
		if (!menu) return 0;
		if (menu && menu->n()==0) {
			delete menu;
		}

		 //***send message to self, must pass on to interface.....
		app->rundialog(new PopupMenu("Viewport Menu","Viewport Menu", 0,
									 0,0,0,0,1,
									 object_id,"viewportmenu",
									 d->id,
									 menu,1,NULL,
									 TREESEL_LEFT));
		buttondown.clear();
	}
	
	DBG cerr <<"..rbdownDone\n";
	return 0;
}

//! Relay RBUp to interfaces.
int ViewportWindow::RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"  ViewportWindow::rbup..\n";
	for (int c=interfaces.n-1; c>=0; c--) { //***
		if (!interfaces.e[c]->RBUp(x,y,state,d)) 
			return 0; //interface claimed the event
	}
	buttondown.up(d->id,RIGHTBUTTON);
	DBG cerr <<".rbupDone \n";
	return 0;
}

/*! MouseMove events are relayed to all interfaces.
 *
 * ***Currently, middle button drags are mapped to right button, plain middle is right-shift
 *
 * If the viewport caught an RBDown, then it will shift, scale, or rotate the
 * viewport when the mouse is moved.
 */
int ViewportWindow::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	last_mouse=d->id;

	//DBG flatpoint pp=dp->screentoreal(x,y);
	//DBG cerr <<"mouse: "<<x<<','<<y<<"  real:"<<pp.x<<','<<pp.y<<endl;

	if (xruler) xruler->SetPosFromScreen(x);
	if (yruler) yruler->SetPosFromScreen(y);
	
	int mx,my, rmx,rmy;
	buttondown.move(d->id,x,y, &mx,&my);
	buttondown.getinitial(d->id,RIGHTBUTTON, &rmx,&rmy);

	for (int c=interfaces.n-1; c>=0; c--) { //***should all interfaces get motion??
		if (!interfaces.e[c]->MouseMove(x,y,state,d)) ; // do nothing, all interfaces get the mouse move event
	}

	if (!buttondown.isdown(d->id,LEFTBUTTON) && (buttondown.isdown(d->id,RIGHTBUTTON) || buttondown.isdown(d->id,MIDDLEBUTTON))) {
		if (buttondown.isdown(d->id,MIDDLEBUTTON) && (state&(ShiftMask|ControlMask))==0) state|=ShiftMask;
		if (state&ShiftMask && state&ControlMask) {// rotate left==ccw, right==cw

			if (win_style&VIEWPORT_ROTATABLE && x-mx) { //rotate canvas
				DBG cerr <<"Laxkit::ViewportWindow mousemove rotate start: "<<endl; dumpctm(dp->Getctm());
				dp->Rotate(x-mx,rmx,rmy,1); 
				DBG cerr <<"Laxkit::ViewportWindow mousemove rotate after rotate: "<<endl; dumpctm(dp->Getctm());
				syncWithDp();
				DBG cerr <<"Laxkit::ViewportWindow mousemove rotate after syncWithDp: "<<endl; dumpctm(dp->Getctm());
				needtodraw=1;
			}

		} else if (state&ControlMask) { // zoom: left==out, right==in
			if (x-mx) { 
				double z=pow(1.1,x-mx);
				//cout <<"..Zoom:"<<z<<"  ";
				dp->Zoom(z,rmx,rmy);
				syncWithDp();
				needtodraw=1;
			}

		} else if (state&ShiftMask) { // shift screen
			if (x-mx!=0 || y-mx!=0) {
				dp->ShiftScreen(x-mx,y-my);
				syncWithDp();
				needtodraw=1;
			}
		}
	}

	return 0;
}

int ViewportWindow::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	for (int c=interfaces.n-1; c>=0; c--) { 
		if (!interfaces.e[c]->KeyUp(ch,state,d)) 
			return 0; //interface claimed the event
	}
	return 1;
}

Laxkit::ShortcutHandler *ViewportWindow::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(VIEWPORT_ZoomIn,        '+',ShiftMask,0,   _("ZoomIn"),        _("Zoom in"),NULL,0);
	sc->AddShortcut('=',0,0, VIEWPORT_ZoomIn);
	sc->Add(VIEWPORT_ZoomOut,       '-',0,0,           _("ZoomOut"),       _("Zoom out"),NULL,0);
	sc->Add(VIEWPORT_CenterReal,    ' ',0,0,           _("CenterReal"),    _("Center the real origin in the viewport"),NULL,0);
	sc->Add(VIEWPORT_ResetView,     ' ',ControlMask|ShiftMask,0,_("ResetView"),_("Reset the view, center and align axes"),NULL,0);
	sc->AddShortcut(' ',ControlMask,0, VIEWPORT_ResetView);

	sc->Add(VIEWPORT_ResetView,     ' ',ControlMask|ShiftMask,0,_("ResetView"),_("Reset the view, center and align axes"),NULL,0);

	sc->Add(VIEWPORT_NextObject,    '.',0,0,           _("NextObject"),    _("Select next object"),NULL,0);
	sc->Add(VIEWPORT_PreviousObject,',',0,0,           _("PreviousObject"),_("Select previous object"),NULL,0);
	sc->Add(VIEWPORT_DeleteObj, LAX_Bksp,0,0,           _("DeleteObject"), _("Delete current object"),NULL,0);
	sc->AddShortcut(LAX_Del,0,0, VIEWPORT_DeleteObj);

	sc->Add(VIEWPORT_ShiftLeft, 'j',0,0,           _("ShiftLeft"), _("Shift view left"),NULL,0);
	sc->Add(VIEWPORT_ShiftRight,'l',0,0,           _("ShiftRight"),_("Shift view right"),NULL,0);
	sc->Add(VIEWPORT_ShiftUp,   'i',0,0,           _("ShiftUp"),   _("Shift view Up"),NULL,0);
	sc->Add(VIEWPORT_ShiftDown, 'k',0,0,           _("ShiftDown"), _("Shift view Down"),NULL,0);
	sc->Add(VIEWPORT_IncShift,'[',ControlMask,0,   _("IncShift"),_("Increase view shift amount"),NULL,0);
	sc->Add(VIEWPORT_DecShift,']',ControlMask,0,   _("DecShift"),_("Decrease view shift amount"),NULL,0);

	sc->Add(VIEWPORT_Undo,'z',ControlMask,0,   _("Undo"),_("Undo"),NULL,0);
	sc->Add(VIEWPORT_Redo,'y',ControlMask,0,   _("Redo"),_("Redo"),NULL,0);
	sc->AddShortcut('Z',ControlMask|ShiftMask,0, VIEWPORT_Redo);

	manager->AddArea(whattype(),sc);
	return sc;
}

int ViewportWindow::PerformAction(int action)
{
	if (action==VIEWPORT_ZoomIn) {
		 // get mouse coords zoom around that
		int x=win_w/2,y=win_h/2;
		if (last_mouse)	mouseposition(last_mouse,this,&x,&y,NULL,NULL);
		dp->Zoom(1.25,x,y);
		syncWithDp();
		needtodraw=1; 
		return 0;

	} else if (action==VIEWPORT_ZoomOut) {
		 // get mouse coords zoom around that
		int x=win_w/2,y=win_h/2;
		if (last_mouse)	mouseposition(last_mouse,this,&x,&y,NULL,NULL);
		dp->Zoom(.75,x,y);
		syncWithDp();
		needtodraw=1; 
		return 0;

	} else if (action==VIEWPORT_Undo) {
		DBG cerr <<" attempting undo..."<<endl;
		InterfaceManager *imanager=InterfaceManager::GetDefault(true);
		UndoManager *undomanager=imanager->GetUndoManager();
		if (undomanager) undomanager->Undo();
		return 0;

	} else if (action==VIEWPORT_Redo) {
		DBG cerr <<" attempting redo..."<<endl;
		InterfaceManager *imanager=InterfaceManager::GetDefault(true);
		UndoManager *undomanager=imanager->GetUndoManager();
		if (undomanager) undomanager->Redo();
		return 0;

	} else if (action==VIEWPORT_CenterReal || action==VIEWPORT_Center_View) {
		 //center something or other on screen
		dp->CenterReal();
		syncWithDp();
		needtodraw=1;
		return 0;

	} else if (action==VIEWPORT_ResetView || action==VIEWPORT_Default_Zoom) {
		 //zap axes to normal x/y, and center origin in middle of viewport

		flatpoint p=flatpoint((dp->Maxx+dp->Minx)/2, (dp->Maxy+dp->Miny)/2);

		double m[6];
		transform_copy(m,dp->Getctm());
		double xscale=sqrt(m[0]*m[0] + m[1]*m[1]);
		double yscale=xscale * (dp->defaultRighthanded() ? -1 : 1);
		
		transform_set(m, xscale,0,0,yscale, p.x,p.y);
		dp->NewTransform(m);
		syncWithDp(); 
		needtodraw=1;
		return 0;

	//} else if (action==VIEWPORT_Default_Zoom) { 
	//} else if (action==VIEWPORT_Set_Default_Zoom) {
	//} else if (action==VIEWPORT_Center_Object || action==VIEWPORT_Zoom_To_Object) { 
	//} else if (action==VIEWPORT_Zoom_To_Fit) {
	//} else if (action==VIEWPORT_Zoom_To_Width) {
	//} else if (action==VIEWPORT_Zoom_To_Height) {
	
	} else if (action==VIEWPORT_Reset_Rotation) {
		double m[6];
		transform_copy(m, dp->Getctm());
		double xscale=sqrt(m[0]*m[0] + m[1]*m[1]);
		double yscale=xscale * (dp->defaultRighthanded() ? -1 : 1);
		
		flatpoint p=dp->screentoreal(flatpoint((dp->Maxx+dp->Minx)/2, (dp->Maxy+dp->Miny)/2));
		transform_set(m, xscale,0,0,yscale, 0,0);
		p=flatpoint((dp->Maxx+dp->Minx)/2, (dp->Maxy+dp->Miny)/2)-transform_point(m, p);
		m[4]=p.x;
		m[5]=p.y;
		dp->NewTransform(m);
		syncWithDp(); 
		needtodraw=1;
		return 0;

	} else if (action==VIEWPORT_Rotate_90 ||
			   action==VIEWPORT_Rotate_180 || 
			   action==VIEWPORT_Rotate_270 
			) {
		Affine m(dp->Getctm());

		double angle=0;
		if (action==VIEWPORT_Rotate_90) {
			angle=M_PI/2;
		} else if (action==VIEWPORT_Rotate_180) {
			angle=M_PI;
		} else if (action==VIEWPORT_Rotate_270) { 
			angle=3*M_PI/2;
		}

		m.Rotate(angle, transform_point(m.m(), dp->screentoreal(flatpoint((dp->Maxx+dp->Minx)/2, (dp->Maxy+dp->Miny)/2))));

		dp->NewTransform(m.m());
		syncWithDp(); 
		needtodraw=1;
		return 0; 

	} else if (action==VIEWPORT_DeleteObj) {
		if (!DeleteObject()) return 1;
		return 0;

	} else if (action==VIEWPORT_NextObject) {
		SelectObject(-1);
		return 0;

	} else if (action==VIEWPORT_PreviousObject) {
		SelectObject(-2);
		return 0;

	} else if (action==VIEWPORT_ShiftLeft) {
		dp->ShiftScreen(-view_shift_amount,0);
		syncWithDp();
		return 0;

	} else if (action==VIEWPORT_ShiftRight) {
		dp->ShiftScreen(view_shift_amount,0);
		syncWithDp();
		return 0;

	} else if (action==VIEWPORT_ShiftUp) {
		dp->ShiftScreen(0,view_shift_amount);
		syncWithDp();
		return 0;

	} else if (action==VIEWPORT_ShiftDown) {
		dp->ShiftScreen(0,-view_shift_amount);
		syncWithDp();
		return 0;

	} else if (action==VIEWPORT_IncShift) {
		view_shift_amount*=1.2;
		return 0;

	} else if (action==VIEWPORT_DecShift) {
		view_shift_amount*=.8;
		if (view_shift_amount<3) view_shift_amount=3;
		return 0;
	}

	return 1;
}

/*! <pre>
 *  del      delete current object
 *  bksp     delete current object
 * </pre>
 */
int ViewportWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{ 
	DBG cerr << (ch>=32&&ch<255?ch:'*')<<endl;

	for (int c=interfaces.n-1; c>=0; c--) { 
		if (!interfaces.e[c]->CharInput(ch,buffer,len,state,d))
			return 0; //interface claimed the event
	}

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
	    return PerformAction(action);
	}

	DBG cerr <<" key in viewportwindow not interface"<<endl;

	switch(ch) {
		case LAX_Shift: //shift
			if (buttondown.isdown(d->paired_mouse->id,RIGHTBUTTON)) { // reset where rmx,rmy are
				int x,y;
				mouseposition(d->paired_mouse->id,this,&x,&y,NULL,NULL);
				buttondown.up(d->paired_mouse->id,RIGHTBUTTON);
				buttondown.down(d->paired_mouse->id,RIGHTBUTTON,x,y);
			}
			break;
		case LAX_Control: //cntl
			if (buttondown.isdown(d->paired_mouse->id,RIGHTBUTTON)) { // reset where rmx,rmy are
				int x,y;
				mouseposition(d->paired_mouse->id,this,&x,&y,NULL,NULL);
				buttondown.up(d->paired_mouse->id,RIGHTBUTTON);
				buttondown.down(d->paired_mouse->id,RIGHTBUTTON,x,y);
			}
			break;


		default: return 1;
	}
	return 0;
}

/*! By default just return the UndoManager from InterfaceManager.
 */
Laxkit::UndoManager *ViewportWindow::GetUndoManager()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	return imanager->GetUndoManager();
}

//! This will take the space settings in dp, and coordiate the rulers and scrollers.
/*! Displayer keeps track of the workspace and the window screen.
 * The tricky part is when the workspace is rotated, and when the axes
 * are of different lengths, or not orthogonal.
 *
 * This currently only calls syncrulers(3). Assumes that the panner and scrollers can
 * take care of themselves.
 */
void ViewportWindow::syncWithDp()
{
	 // sync up the rulers
	syncrulers(3);	

//	 // sync up the scrollers done with panner mechanism?
}

//! Sync xruler if (which&1), and yruler if (which&2).
void ViewportWindow::syncrulers(int which)
{
	flatpoint p=dp->realtoscreen(flatpoint(0,0));
	if (xruler && which&1) {
		xruler->SetMag(dp->GetVMag(1,0));
		xruler->Set(-p.x/dp->GetVMag(1,0));
		xruler->Track();
	}
	if (yruler && which&2) {
		yruler->SetMag(dp->GetVMag(0,1));
		yruler->Set(-p.y/dp->GetVMag(0,1));
		yruler->Track();
	}
}

//int ViewportWindow::FocusIn() {}
//int ViewportWindow::FocusOut() {}

/*! Does this:
 * \code
 *     	anXWindow::MoveResize(nx,ny,nw,nh);
 *     	flatpoint center=dp->screentoreal(win_w/2,win_h/2);
 *     	dp->WrapWindow(this);
 *     	if (firsttime) { 
 *     	  if (firsttime==1) dp->CenterReal(); 
 *     	  firsttime=0;
 *     	} else { dp->CenterPoint(center); }
 *     	dp->syncPanner();
 *     	syncWithDp();
 *     	for (int c=0; c<interfaces.n; c++) { interfaces.e[c]->ViewportResized(); }
 *     	return 0;
 * \endcode
 *
 * Then calls ViewportResized() on each interface.
 */
int ViewportWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	flatpoint center=dp->screentoreal(win_w/2,win_h/2);
	dp->WrapWindow(this);
	if (firsttime) { 
	  if (firsttime==1) dp->CenterReal(); 
	  firsttime=0;
	} else { dp->CenterPoint(center); }
	dp->syncPanner();
	syncWithDp();
	for (int c=0; c<interfaces.n; c++) { interfaces.e[c]->ViewportResized(); }
	return 0;
}

/*! Does this:
 * \code
 *  anXWindow::Resize(nw,nh);
 *  flatpoint center=dp->screentoreal(win_w/2,win_h/2);
 *  dp->WrapWindow(this);
 *  if (firsttime) { 
 *    dp->CenterReal(); 
 *    firsttime=0;
 *  }  else { dp->CenterPoint(center); }
 *  dp->syncPanner();
 *  syncWithDp();
 *  for (int c=0; c<interfaces.n; c++) { interfaces.e[c]->ViewportResized(); }
 * \endcode
 *
 * Then calls ViewportResized() on each interface.
 */
int ViewportWindow::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	flatpoint center=dp->screentoreal(win_w/2,win_h/2);
	dp->WrapWindow(this);
	if (firsttime) { firsttime=0; dp->CenterReal(); }
	else { dp->CenterPoint(center); }
	dp->syncPanner();
	syncWithDp();
	for (int c=0; c<interfaces.n; c++) { interfaces.e[c]->ViewportResized(); }

	dp->EndDrawing();

	return 0;
}

//! Do a little extra checking to find what point r should correspond to.
/*! If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp.
 *
 * Default is just return dp->realtoscreen(r).
 */
flatpoint ViewportWindow::realtoscreen(flatpoint r)
{
	return dp->realtoscreen(r);
}

//! Do a little extra checking to find what point (x,y) should correspond to.
/*! If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp. In general, if an interface has an active object
 * this should coordinates in the object's parent space.
 *
 * Default is just return dp->screentoreal(r).
 */
flatpoint ViewportWindow::screentoreal(int x,int y)
{
	//dp:return flatpoint(ictm[4] + ictm[0]*p.x + ictm[2]*p.y, ictm[5]+ictm[1]*p.x+ictm[3]*p.y);
	return dp->screentoreal(x,y);
}

//! Do a little extra checking to find what the magnification is.
/*! If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp.
 *
 * Default is just return dp->Getmag(c).
 */
double ViewportWindow::Getmag(int c)
{
	//dp:if (c) return sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]);
	//        return sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]);
	return dp->Getmag(c);
}

//! Do a little extra checking to find what the magnification is.
/*! If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp->
 *
 * Default is just return dp->GetVMag(x,y).
 */
double ViewportWindow::GetVMag(int x,int y)
{
	//dp: flatpoint v=screentoreal(x,y),v2=screentoreal(0,0);
	//        return sqrt((x*x+y*y)/((v-v2)*(v-v2)));
	return dp->GetVMag(x,y);
}


//------------------------------------ UndoDatas -------------------------------

class HierarchyUndoData : public UndoData
{
  public:
    enum ObjUndoTypes { ObjAdded, ObjIndex, ObjDelete, ObjReparent };
    int type;

    ObjectContext *old_oc;
    ObjectContext *oc;

    HierarchyUndoData(Undoable *context, int type, ObjectContext *noc, int isauto);
    virtual ~HierarchyUndoData();
};

HierarchyUndoData::HierarchyUndoData(Undoable *context, int type, ObjectContext *noc, int isauto)
  : UndoData(isauto)
{
    this->context = context;
    if (context && dynamic_cast<anObject*>(context)) {
        dynamic_cast<anObject*>(context)->inc_count();
    }

    this->type = type;
    oc = noc;
}

HierarchyUndoData::~HierarchyUndoData()
{
    delete oc;
    delete old_oc;
}



} // namespace LaxInterfaces

