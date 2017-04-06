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
//    Copyright (C) 2004-2012 by Tom Lechner
//

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/aninterface.h>
//#include <lax/interfaces/undo.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/strmanip.h>

using namespace Laxkit;
using namespace LaxFiles;


#include <iostream>
using namespace std;
#define DBG 

//! Namespace for the Laxkit interfaces, surprisingly enough.
namespace LaxInterfaces {


//----------------------------------------------------------------------
//----------------------------- Interfaces -----------------------------
//----------------------------------------------------------------------

/*! \defgroup interfaces Interfaces
 *
 * The Laxkit defines a number of useful interfaces to aid in manipulating
 * various kinds of visual data. They all ultimately derive from 
 * anInterface. \link LaxInterfaces::ViewportWindow ViewportWindow\endlink
 * and \link LaxInterfaces::ViewerWindow ViewerWindow\endlink 
 * are the usual classes that incorporate the
 * interfaces into somewhat easy to use, scrollable, resizable system.
 * Application programmers will typically subclass those to meet their needs.
 *
 * If an interface has primary==1, then when the left button is clicked down, then
 * it is ok to work on other data of the same type, but not on data of different types.
 * If clicking down on empty space, then the interface makes a new object.
 * primary==1 is like using a creation tool, rather than a select tool.
 *
 * If primary==0, then clicking on blank space usually will deselect the object. If there
 * is another object under the mouse, then that object is made the current one.
 * 
 * anInterfaces can work with a ViewportWindow, which
 * provides various aids to manipulating the interface's Displayer, and provides means to search
 * for objects. Search information is held in ObjectContext objects. 
 * Any ObjectContext an anInterface gets from a ViewportWindow should be considered
 * to be opaque. That is, its internals should not be accessed by an interface,
 * even if it can.
 * Furthermore, interfaces should not create and pass ObjectContext
 * objects by itself. They should only pass ObjectContext objects it gets from
 * the ViewportWindow back to the same ViewportWindow.
 *
 * All the event functions in anInterface must return 0 if they use the event, and
 * nonzero if they do not. This is used in ViewportWindow to propagate events to different
 * interfaces.
 * 
 * The currently available interfaces that basically work include:
 *  - ImageInterface, uses ImageData
 *  - ImagePatchInterface, uses ImagePatchData
 *  - ColorPatchInterface, uses ColorPatchData
 *  - PatchInterface, uses PatchData
 *  - GradientInterface, uses GradientData, radial and linear gradients
 *  - PathInterface, uses PathsData, needs work
 *  - RectInterface, can use RectData, but can work on any SomeData
 *  - ObjectInterface, can shift around any SomeData
 *
 * These interfaces exist, but they are horribly implemented and need a lot of work to update
 * to the current way of doing things:
 *  - LinesInterface
 *  - CaptionInterface
 *  - EllipseInterface
 *  - FreehandInterface
 *  - MeasureInterface
 *
 * \todo ***  BezInterface has many useful
 *   features that I haven't ported over to BezPathOperator yet. The others (including BezInterface)
 *   are a bit old, and somewhat broken with how interfaces are used currently. Same goes
 *   for LinesInterface
 *
 * \todo should there should be interfaces for all postscript language level 3 / pdf  gradient
 *   types. linear, radial, and tensor/coons gradients are implemented more or less, but not the triangle
 *   strip and triangle lattice gradients.. nor the type 1 generic function gradients..
 *
 * \todo The bez patches are currently more like a bezier patch lattice, rather than how Postscript
 *   LL3 does it, which is a freeform bezier patch setup. Postscript has triangle lattices,
 *   but not bez patch lattices!! What a foolish oversight!! should be able to do the
 *   standard ps way in addition to the lattice way.
 *
 * \todo *** edit interface docs, do a better how to (see lax/interfaces/interfaces.txt)
 *
 */




//----------------------------- anInterface -----------------------------

/*! \class anInterface
 * \ingroup interfaces
 * \brief A class for key and mouse interfaces acting on particular windows.
 *
 * anInterfaces are for use with, for instance, a ViewportWindow, which maintains
 *  a stack of interfaces, down through which input is sent.
 *  The convention used in ViewportWindow is that the input related member functions like LBDown()
 *  and WheelDown() must return 0 if they absorb the event, and nonzero if they do not.
 *  When it is a button, if the absorb an LBDown(), they MUST also absorb a corresponding
 *  LBUp().
 *
 *  For any derived classes, if you ever use the duplicate function please always remember
 *  to actually define your own duplicate function! Otherwise, your duplicate will just
 *  return NULL.
 *
 * Interfaces designed for use in a ViewportWindow store their own data internally.
 * Any time they create new data, they must
 * call viewport->NewData(newdata). This allows the viewport to place the data properly.
 * Interfaces generally are all able to search for objects of types they may or may not be able
 * to use. After a search, they call viewport->NewData(data,context_returned_by_search_function).
 */
/*! \var unsigned long anInterface::interface_style
 * \brief Style flags for the interface. Meaning depends on the interface.
 */
/*! \var unsigned long anInterface::interface_type
 * \brief What sort of interface this is. Default is INTERFACE_Tool.
 *
 * Can be INTERFACE_Overlay, INTERFACE_Tool, or INTERFACE_Child.
 */
/*! \fn int anInterface::draws(const char *atype)
 * \brief Returns !strcmp(whatdatatype(),atype).
 *
 * Derived classes that can draw more than one kind of object would redefine this
 * appropriately. Just return nonzero if the interface can draw the type, or 0 if it cannot.
 */
/*! \var Laxkit::anXApp *anInterface::app
 * \brief The application this interface works with.
 */
/*! \var Laxkit::anXWindow *anInterface::curwindow
 * \brief The window the interface currently works on.
 */
/*! \var anInterface *anInterface::owner
 * \brief The interface that owns this one.
 *
 * Interfaces can have one parent and one child. This might be, for instance,
 * a BezPathOperator (the child) that controls things that are really held by
 * PathInterface (the owner). Or an EllipseInterface (owner) that uses the controls provided
 * by the RectInterface (child).
 *
 * Usually, when sending a message to owner, it will use owner_message, or whattype() if
 * owner_message is null.
 *
 * When interfaces are pushed and popped of a ViewportWindow's stack, complete threads
 * of interfaces are pushed on and off, not simply individual ones.
 */
/*! \var anInterface *anInterface::child
 * \brief The child of this interface. See anInterface::owner. This is dec_counted in destructor.
 */
/*! \var char *anInterface::name
 * \brief An instance name.. ***don't think this is used anywhere at the moment
 */
/*! \var int anInterface::primary
 * \brief Whether this is supposed to be a 'main' interface, or merely a helper.
 */
/*! \var int anInterface::id
 * \brief Must be positive, other values are reserved for internal use.
 */
/*! \var int anInterface::needtodraw
 * \brief Whether the interface thinks it has to refresh.
 */
/*! \fn int anInterface::Needtodraw()
 * \brief Must return nonzero if the data needs to be drawn, that is to say Refresh must be called.
 */
/*! \fn int anInterface::DrawData(Laxkit::anObject *ndata,
 * 				Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0)
 * \brief Redefine this for interfaces that can draw data not owned, without loosing current data.
 *
 * Return 1 for nothing drawn, 0 for something drawn. 
 */
/*! \fn int anInterface::draws(const char *atype)
 * \brief Return 1 if the interface can draw data of the given type.
 */
/*! \fn int anInterface::InterfaceOn()
 * \brief Called when, for instance data is selected, and the interface 
 * 	must now show control points, and expect input.
 */
/*! \fn int anInterface::InterfaceOff() { return 0; } 
 * \brief Called when the interface is no longer required to handle display of data.
 */
/*! \fn int anInterface::UseThis(int id,int ndata)
 * \brief Return 1 if the id/ndata is used, otherwise zero.
 */
/*! \fn int anInterface::UseThis(Laxkit::anObject *ndata,unsigned int mask=0) { return 0; } 
 * \brief Return 1 if interface can use that data, otherwise, the calling code must appropriately dispose of ndata.
 */
/*! \fn Laxkit::MenuInfo *anInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
 * \brief Return a context sensitive menu for screen position (x,y).
 *
 * This should be a menu instance that gets deleted by the popped up menu. 
 * The interface should not refer to it again. 
 *
 * Default implementation here is to simply return whatever was in menu. Usually if menu already exists,
 * then this instance will append items to it.
 */
/*! \fn int anInterface::Event(const Laxkit::EventData *e, const char *mes)
 * \brief Respond to events, particularly menu events from a menu created from ContextMenu().
 */
/*! \var ViewportWindow *anInterface::viewport
 * \brief curwindow dynamically cast to ViewportWindow. Thus, it will be NULL if it is not a ViewportWindow.
 */
/*! \var Displayer *anInterface::dp
 * \brief The Displayer used by the controlling window.
 *
 * The transform in dp is interpreted as a kind of base transform to the view space. If
 * the space has further transformations from current pages or current groups, then it is
 * up to the ViewportWindow to keep track of that separately. To access that transform, you can
 * call anInterface::realtoscreen() and anInterface::screentoreal(). Otherwise,
 * the plain dp->realtoscreen and screentoreal work fine.
 */
/*! \fn void anInterface::Clear(SomeData *d)
 * \brief Clear the data from the interface only if d is the interface's current data.
 *
 * If d==NULL, then the interface should clear any data it has. Otherwise, clear only if
 * the interface is using d.
 */


/*! \fn void anInterface::ViewportResized()
 * \brief Called after the parent viewport gets resized.
 */
/*! \fn void anInterface::Mapped()
 * \brief Called after the parent viewport gets mapped (made visible).
 */
/*! \fn void anInterface::Unmapped()
 * \brief Called after the parent viewport gets mapped (made invisible, such as offscreen, not merely obscured).
 */

	
//!  This constructor assigns id=getUniqueNumber().
anInterface::anInterface()
{ 
	child=owner=NULL; 
	owner_message=NULL;
	app=anXApp::app; 
	curwindow=NULL; 
	name=NULL; 
	primary=0;
	id=getUniqueNumber();
	interface_style=0;
	interface_type=INTERFACE_Tool;
	needtodraw=1; 
}

//! Constructor to assign owner and id.
/*! 
 * \todo *** this isn't so hot, maybe do some check to ensure that owner->child points here?
 */
anInterface::anInterface(anInterface *nowner,int nid)
{
	child=NULL;
	owner=nowner;
	owner_message=NULL;
	app=anXApp::app; 
	if (nowner) {
		curwindow=nowner->curwindow;
	} else {
		curwindow=NULL;
	}
	id=nid;
	name=NULL;
	interface_style=0;
	interface_type=INTERFACE_Tool;
	needtodraw=1; 
}

//! Constructor to assign just the id, set other stuff to 0.
anInterface::anInterface(int nid)
{
	child=owner=NULL;
	owner_message=NULL;
	app=anXApp::app; 
	id=nid;
	curwindow=NULL;
	name=NULL;
	interface_style=0;
	interface_type=INTERFACE_Tool;
	needtodraw=1; 
}

anInterface::anInterface(int nid,Displayer *ndp)
{ 
	child=owner=NULL;
	owner_message=NULL;
	app=anXApp::app; 
	id=nid;
	curwindow=NULL;
	name=NULL;
	interface_style=0;
	interface_type=INTERFACE_Tool;
	needtodraw=1; 

	dp=ndp; 
	if (dp) curwindow=dynamic_cast<anXWindow*>(dp->GetDrawable());
	viewport=dynamic_cast<ViewportWindow *>(curwindow); 
}

anInterface::anInterface(anInterface *nowner,int nid,Displayer *ndp)
{
	child=NULL;
	owner=nowner;
	owner_message=NULL;
	app=anXApp::app; 
	if (nowner) {
		curwindow=nowner->curwindow;
	} else {
		curwindow=NULL;
	}
	id=nid;
	name=NULL;
	interface_style=0;
	interface_type=INTERFACE_Tool;
	needtodraw=1; 

	dp=ndp; 
	if (dp) curwindow=dynamic_cast<anXWindow*>(dp->GetDrawable());
	viewport=dynamic_cast<ViewportWindow *>(curwindow); 
}

anInterface::~anInterface()
{ 
	if (child) child->dec_count();
	delete[] owner_message;
	DBG cerr<<"--- anInterface "<<whattype()<<","<<" destructor"<<endl; 
}


//! Return or modify to almost duplicate instance.
/*! If dup==NULL, then return NULL. Otherwise modify the existing dup.
 *
 * Copies app, name, interface_style, id. The rest are initialized to NULL.
 *
 * Normally, subclassed anInterface objects will return a new anInterface if dup=NULL,
 * or apply changes to the given dup object, assuming it is of the correct class.
 * This is especially important when setting up a ViewportWindow/ViewerWindow system.
 * In that scenario, the dp and the data if present should not be copied, as they will
 * be assigned new stuff by the window, thus those things are not transferred to the
 * duplicate. Typically, the specific interface will create their
 * own blank instance of themselves, and in doing so, the dp and and data will
 * be set to NULL there.
 *
 * Typical duplicate function in an interface looks like this:\n
 *
 * \code
 * anInterface *TheInterface::duplicate()
 * {
 *    dup=new TheInterface();
 *    // add any other TheInterface specific initialization
 *    dup->somefield = somefield;
 *
 *    return anInterface::duplicate(dup);
 * }
 * \endcode
 */
anInterface *anInterface::duplicate(anInterface *dup)
{
	if (!dup) return NULL; //dup=new anInterface();<- wrong! anInterface is abstract class..
	makestr(dup->name,name);
	dup->id=id;
	dup->interface_style=interface_style;
	return dup;
}

/*! Classes can use this to tell owner when something's changed. This is not used in anInterface class.
 * Sends owner_message, or whattype() if owner_message is NULL.
 * event->usertype is set to level.
 *
 * If interface_style & INTERFACE_DontSendOnModified, then don't send when Modified() called.
 */
void anInterface::Modified(int level)
{
	if ((interface_style & INTERFACE_DontSendOnModified) != 0) return;
									 
    if (owner) {
        DBG cerr << whattype() <<" Modified(), sending to "<<(owner->Id()?owner->Id():owner->whattype())<<endl;
        const char *message=owner_message;
        if (!message) message=whattype();
        EventData *ev=new EventData(message);
		ev->usertype = level;
        anXApp::app->SendMessage(ev, owner->object_id, message,object_id);
        return;
    }
}

//! Return a ShortcutHandler that contains stacks of bound shortcuts and possible actions.
/*! NULL means there are none defined for this interface.
 *
 * Interfaces that do use shortcuts, and want them stored in an easy manner should use the
 * ShortcutManager system, accessed with GetDefaultShortcutManager(). They can then install
 * a single list of shortcuts and actions bound to the type name of the interface, and any future 
 * window instances can borrow those.
 */
ShortcutHandler *anInterface::GetShortcuts()
{ return NULL; }

/*! This method exists to aid standardizing access to shortcut actions from potential scripting.
 * Return 1 for not found or otherwise not done, or 0 for success.
 * Default is return 1.
 */
int anInterface::PerformAction(int actionnumber)
{ return 1; }


//! Default just calls Clear(NULL).
void anInterface::Clear()
{
	Clear(NULL);
}

/*! Applications may call this on the very first time a tool is instantiated.
 * It should install any tool settings object, and any resource types to
 * InterfaceManager::tools and InterfaceManager::datafactory.
 * The tool settings object should have the same name as the tool type,
 * that is, it should be tool->whattype().
 *
 * Default here is to do nothing.
 */
int anInterface::InitializeResources()
{
	return 0;
}

/*! Set the window the interface works on to ncur. Returns ncur. If ncur==NULL, then just return current viewport.
 * Otherwise returns the new current window.
 */
Laxkit::anXWindow *anInterface::CurrentWindow(Laxkit::anXWindow *ncur)
{
	if (!ncur) return viewport;
	viewport=dynamic_cast<ViewportWindow*>(ncur);
	return curwindow=ncur;
}

/*! Will not add ch if child!=NULL.
 * If addbefore!=0, then add at index-1 in viewport->interfaces. Else after *this.
 *
 * If viewport==NULL, then just install as child.
 *
 * Return 0 success, or nonzero for not added.
 */
int anInterface::AddChild(LaxInterfaces::anInterface *ch, int absorbcount, int addbefore)
{
	if (!ch) return 1;
	if (!child) return 2;

	if (viewport) {
		int i=viewport->interfaces.findindex(this);
		if (!addbefore) i++;
		viewport->Push(ch,i,0);
	}
	child=ch;
	child->owner=this;
	if (!absorbcount) child->inc_count();
	return 0;
}

//! If there is a child, do something to remove it.
/*! If this interface is in a ViewportWindow, then the child will be popped off that port's stack.
 */
int anInterface::RemoveChild()
{
	if (!child) return 1;
	anInterface *i=child;

	child->owner=NULL;
	child->dec_count();
	child=NULL;

	if (viewport) viewport->Pop(i,1); //decs count
	//else i->dec_count();

	needtodraw=1;
	return 0;
}

//------------------------ Copy and paste

/*! Paste the given data.
 *
 * Return nonzero for could not paste. Return 0 for accepts paste.
 *
 * Generally called from ViewportWindow in response to a get selection event. The viewport may
 * do some processing to get obj. Otherwise, the more usual past results in a simple string.
 */
int anInterface::Paste(const char *txt,int len, Laxkit::anObject *obj, const char *formathint)
{
	return 1;
}

/*! Return nonzero for could not comply.
 */
int anInterface::GetForCopy(const char **txt,int *len, Laxkit::anObject **obj, const char *formathint)
{
	return 1;
}

/*! Basically, call viewport->selectionCopy().
 *
 * Return 0 for success, or nonzero for unable.
 */
int anInterface::InitForCopy()
{
	if (viewport) viewport->SetCopySource(this);
	return 1;
}


//------------------------SomeData and Viewport related functions:

//! Use a different dp to draw data with.
/*! Temporarily replaces dp with tdp while it passes data on to anInterface::DrawData(data,a1,a2,info).
 * When the Laxkit interfaces use a1 or a2, the convention is that a1 is a LineStyle, and a2 is
 * a FillStyle.
 *
 * Returns this->DrawData(data,a1,a2,info). Derived classes need only redefine
 * DrawData(anObject *,anObject *,anObject *,int).
 */
int anInterface::DrawDataDp(Displayer *tdp,SomeData *data,anObject *a1,anObject *a2,int info)//a1=a2=NULL, info=1
{
	Displayer *ttdp=dp;
	anXWindow *ttw =curwindow;
	ViewportWindow *ttvp=viewport;
	dp=tdp;
	curwindow=dynamic_cast<anXWindow*>(dp->GetDrawable());
	viewport=dynamic_cast<ViewportWindow *>(curwindow); 

	int c=this->DrawData((anObject *)data,a1,a2,info);
	dp=ttdp;
	curwindow=ttw;
	viewport=ttvp;
	return c;
}

//! Do a little extra checking to find what point r should correspond to.
/*! If curwindow is a ViewportWindow, then return same function from ViewportWindow. Otherwise
 * return the usual function from dp.
 *
 * If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp.
 */
flatpoint anInterface::realtoscreen(flatpoint r)
{
	if (!viewport) return dp->realtoscreen(r);
	return viewport->realtoscreen(r);
}

//! Do a little extra checking to find what point (x,y) should correspond to.
/*! If curwindow is a ViewportWindow, then return same function from ViewportWindow. Otherwise
 * return the usual function from dp.
 *
 * If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp.
 */
flatpoint anInterface::screentoreal(int x,int y)
{
	if (!viewport) return dp->screentoreal(x,y);
	return viewport->screentoreal(x,y);
}

//! Do a little extra checking to find what the magnification is.
/*! If curwindow is a ViewportWindow, then return same function from ViewportWindow. Otherwise
 * return the usual function from dp.
 *
 * If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp.
 */
double anInterface::Getmag(int c)
{
	if (!viewport) return dp->Getmag(c);
	return viewport->Getmag(c);
}

//! Do a little extra checking to find what the magnification is.
/*! If curwindow is a ViewportWindow, then return same function from ViewportWindow. Otherwise
 * return the usual function from dp.
 *
 * If a viewport defines pages or groups, for instance, then this allows
 * derived interfaces to use coords on those pages and groups, rather than the
 * plain view transform in dp.
 */
double anInterface::GetVMag(int x,int y)
{
	if (!viewport) return dp->GetVMag(x,y);
	return viewport->GetVMag(x,y);
}

/*! Interfaces can choose to automate drawing of whatever data they have hold of.
 * If Context() returns something, then the
 * anInterface::dp will have that context applied before the Refresh() is called.
 *
 * Default here is to return NULL. If subclasses do not return something here, they must
 * apply any extra transform to their Displayer before refreshing. Generally, this means
 * keeping track if Refresh() is called directly from the viewport, or if it is called
 * from DrawDataDp().
 */
ObjectContext *anInterface::Context()
{ return NULL; }

//! Set the dp to ndp, and update curwindow/viewport.
void anInterface::Dp(Displayer *ndp) 
{
	dp=ndp; 
	if (dp) { 
		curwindow=dynamic_cast<anXWindow*>(dp->GetDrawable());
		viewport=dynamic_cast<ViewportWindow *>(curwindow); 
	}
}

//! If viewport, use that, else app->postmessage().
void anInterface::PostMessage(const char *message)
{
	if (viewport) viewport->postmessage(message);
	else app->postmessage(message);
}


//! Default settings saving is to output nothing.
void anInterface::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext)
{
}

//! Placeholder for dumping in settings.
void anInterface::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *loadcontext)
{
}

/*! By default just return InterfaceManager::GetUndoManager() from the default manager..
 */
Laxkit::UndoManager *anInterface::GetUndoManager()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	return imanager->GetUndoManager();
}



} // namespace LaxInterfaces

