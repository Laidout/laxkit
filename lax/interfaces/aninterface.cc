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
//    Copyright (C) 2004-2012 by Tom Lechner
//

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/strmanip.h>
#include <lax/colorevents.h>

#include <lax/debug.h>


using namespace Laxkit;


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
 *  return nullptr.
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
 * 				Laxkit::anObject *a1=nullptr,Laxkit::anObject *a2=nullptr,int info=0)
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
/*! \fn int anInterface::Event(const Laxkit::EventData *e, const char *mes)
 * \brief Respond to events, particularly menu events from a menu created from ContextMenu().
 */
/*! \var ViewportWindow *anInterface::viewport
 * \brief curwindow dynamically cast to ViewportWindow. Thus, it will be nullptr if it is not a ViewportWindow.
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
 * If d==nullptr, then the interface should clear any data it has. Otherwise, clear only if
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
	dp              = nullptr;
	viewport        = nullptr;
	curwindow       = nullptr;
	child = owner   = nullptr;
	owner_id        = 0;
	owner_message   = nullptr;
	app             = anXApp::app;
	name            = nullptr;
	primary         = 0;
	id              = getUniqueNumber();
	interface_style = 0;
	interface_type  = INTERFACE_Tool;
	needtodraw      = 1;
	last_message    = nullptr;
	last_message_n  = 0;
}

//! Constructor to assign owner and id.
/*! 
 * \todo *** this isn't so hot, maybe do some check to ensure that owner->child points here?
 */
anInterface::anInterface(anInterface *nowner,int nid)
 : anInterface()
{
	if (nowner) curwindow = nowner->curwindow;
	id = nid;
	if (id < 0) id = getUniqueNumber();
}

//! Constructor to assign just the id, set other stuff to 0.
anInterface::anInterface(int nid)
 : anInterface()
{
	id = nid;
	if (id < 0) id = getUniqueNumber();
}

anInterface::anInterface(int nid,Displayer *ndp)
 : anInterface()
{ 
	id = nid;
	if (id < 0) id = getUniqueNumber();
	dp = ndp; 
	if (dp) curwindow = dynamic_cast<anXWindow*>(dp->GetDrawable());
	viewport = dynamic_cast<ViewportWindow *>(curwindow); 
}

anInterface::anInterface(anInterface *nowner,int nid,Displayer *ndp)
 : anInterface()
{
	owner = nowner;
	if (nowner) curwindow = nowner->curwindow;
	id = nid;
	if (id < 0) id = getUniqueNumber();
	dp = ndp; 
	if (dp) curwindow = dynamic_cast<anXWindow*>(dp->GetDrawable());
	viewport = dynamic_cast<ViewportWindow *>(curwindow); 
}

anInterface::~anInterface()
{ 
	if (child) child->dec_count();
	delete[] owner_message;
	delete[] last_message;
	DBGM("--- anInterface "<<whattype()<<" "<<object_id<<" "<<(object_idstr?object_idstr:"(no id)")<<","<<" destructor");
}


//! Return or modify to almost duplicate instance.
/*! If dup==nullptr, then return nullptr. Otherwise modify the existing dup.
 *
 * Copies app, name, interface_style, id. The rest are initialized to nullptr.
 *
 * Normally, subclassed anInterface objects will return a new anInterface if dup=nullptr,
 * or apply changes to the given dup object, assuming it is of the correct class.
 * This is especially important when setting up a ViewportWindow/ViewerWindow system.
 * In that scenario, the dp and the data if present should not be copied, as they will
 * be assigned new stuff by the window, thus those things are not transferred to the
 * duplicate. Typically, the specific interface will create their
 * own blank instance of themselves, and in doing so, the dp and and data will
 * be set to nullptr there.
 *
 * Typical duplicate function in an interface looks like this:\n
 *
 * \code
 * anInterface *TheInterface::duplicateInterface()
 * {
 *    dup = new TheInterface();
 *    // add any other TheInterface specific initialization
 *    dup->somefield = somefield;
 *
 *    return anInterface::duplicateInterface(dup);
 * }
 * \endcode
 */
anInterface *anInterface::duplicateInterface(anInterface *dup)
{
	if (!dup) return nullptr; //dup=new anInterface();<- wrong! anInterface is abstract class..
	makestr(dup->name,name);
	dup->id = id;
	dup->interface_style = interface_style;
	return dup;
}

/*! Subclasses can use this to tell owner when something's changed.
 * Sends owner_message, or whattype() if owner_message is nullptr.
 * event->usertype is set to level.
 *
 * If interface_style & INTERFACE_DontSendOnModified, then don't send when Modified() called.
 */
void anInterface::Modified(int level)
{
	if ((interface_style & INTERFACE_DontSendOnModified) != 0) return;
									 
    if (owner || owner_id) {
        DBG if (owner) std::cerr << whattype() <<" Modified(), sending to "<<(owner->Id()?owner->Id():owner->whattype())<<std::endl;
        DBG else       std::cerr << whattype() <<" Modified(), sending to "<<owner_id<<std::endl;

        const char *message = owner_message;
        if (!message) message = whattype();
        EventData *ev = BuildModifiedMessage(level);
        if (!ev) {
	        ev = new EventData(message);
			ev->usertype = level;
		}
        anXApp::app->SendMessage(ev, owner_id ? owner_id : owner->object_id, message, object_id);
        return;
    }
}

//! Return a ShortcutHandler that contains stacks of bound shortcuts and possible actions.
/*! nullptr means there are none defined for this interface.
 *
 * Interfaces that do use shortcuts, and want them stored in an easy manner should use the
 * ShortcutManager system, accessed with GetDefaultShortcutManager(). They can then install
 * a single list of shortcuts and actions bound to the type name of the interface, and any future 
 * window instances can borrow those.
 */
ShortcutHandler *anInterface::GetShortcuts()
{ return nullptr; }

/*! This method exists to aid standardizing access to shortcut actions from potential scripting.
 * Return 1 for not found or otherwise not done, or 0 for success.
 * Default is return 1.
 */
int anInterface::PerformAction(int actionnumber)
{ return 1; }


//! Default just calls Clear(nullptr).
void anInterface::Clear()
{
	Clear(nullptr);
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

/*! Set the window the interface works on to ncur. Returns ncur. If ncur==nullptr, then just return current viewport.
 * Otherwise returns the new current window.
 */
Laxkit::anXWindow *anInterface::CurrentWindow(Laxkit::anXWindow *ncur)
{
	if (!ncur) return viewport;
	viewport=dynamic_cast<ViewportWindow*>(ncur);
	return curwindow=ncur;
}

/*! Update the viewport color box.
 * If type_hint is 0, then make the color box use 1 color, either stroke or fill whichever is not null.
 * Otherwise, assume you want a 2 color box, specify either COLOR_StrokeFill to show as a stroke on a fill,
 * or COLOR_FGBG to show one box on another box.
 * If type_hint is -1, then use 0 if only one of stroke or fill is not null, otherwise defaults to COLOR_StrokeFill.
 */
void anInterface::UpdateViewportColor(ScreenColor *stroke, ScreenColor *fill, int type_hint)
{
	if (type_hint < 0) {
		if ((stroke && !fill) || (!stroke && fill)) type_hint = 0;
		else if (stroke && fill) type_hint = COLOR_StrokeFill;
	}

	SimpleColorEventData *e;
	if (type_hint == 0 && (fill && !stroke)) { stroke = fill; fill = nullptr; }
	if (stroke) {
		e = new SimpleColorEventData( 65535, stroke->red, stroke->green, stroke->blue, stroke->alpha, 0);
		e->colormode = type_hint;
		e->colorindex = 0;
		app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);
	}

	if (fill) {
		e = new SimpleColorEventData( 65535, fill->red, fill->green, fill->blue, fill->alpha, 0);
		e->colormode = type_hint;
		e->colorindex = 1;
		app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);
	}
}

/*! Will not add ch if child!=nullptr.
 * If addbefore!=0, then add at index-1 in viewport->interfaces. Else after *this.
 *
 * If viewport==nullptr, then just install as child.
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

	child->owner=nullptr;
	child->dec_count();
	child=nullptr;

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
int anInterface::DrawDataDp(Displayer *tdp,SomeData *data,anObject *a1,anObject *a2,int info)//a1=a2=nullptr, info=1
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
 * 
 * If you have a current object, this will generally be the object's parent space.
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

double anInterface::UIScale()
{
	if (curwindow) return curwindow->UIScale();
	return 1.0;
}

void anInterface::ThemeChanged()
{
	UIScaleChanged();
}

/*! This is called when there is a change to the app-wide UI scale.
 */
void anInterface::UIScaleChanged()
{}

double anInterface::ScreenLine()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	return UIScale() * imanager->ScreenLine();
}

double anInterface::NearThreshhold()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	return UIScale() * imanager->NearThreshhold();
}

double anInterface::NearThreshhold2()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	return UIScale() * imanager->NearThreshhold2();
}

/*! Screen pixels below which a click down and up is considered not moved.
 */
double anInterface::DraggedThreshhold()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	return UIScale() * imanager->DraggedThreshhold();
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
 * Default here is to return nullptr. If subclasses do not return something here, they must
 * apply any extra transform to their Displayer before refreshing. Generally, this means
 * keeping track if Refresh() is called directly from the viewport, or if it is called
 * from DrawDataDp().
 */
ObjectContext *anInterface::Context()
{ return nullptr; }

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
	if (message == nullptr) message = "";
	if (viewport) viewport->PostMessage(message);
	else app->postmessage(message);
}

/*! Printf style message.
 */
void anInterface::PostMessage2(const char *fmt, ...)
{
	va_list arg;

    va_start(arg, fmt);
    int c = vsnprintf(last_message, last_message_n, fmt, arg);
    va_end(arg);

    if (c >= last_message_n) {
        delete[] last_message;
        last_message_n = c+100;
        last_message = new char[last_message_n];
        va_start(arg, fmt);
        vsnprintf(last_message, last_message_n, fmt, arg);
        va_end(arg);
    }

	if (viewport) viewport->PostMessage(last_message);
	else app->postmessage(last_message);
}


//! Write out settings. Default dumps out the att returned from dump_out_atts().
void anInterface::dump_out(FILE *f,int indent,int what,DumpContext *savecontext)
{
	Attribute att;
	dump_out_atts(&att, what, savecontext);
	att.dump_out(f,indent);
}

//! Placeholder for dumping in settings. Currently does nothing.
void anInterface::dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext)
{
}

/*! Placeholder for dumping out settings. Default just returns att.
 * Subclasses that do output need to create a new Attribute if att==null, and fill with
 * appropriate settings that will be read back in with dump_in_atts().
 */
Attribute *anInterface::dump_out_atts(Attribute *att,int what,DumpContext *savecontext)
{
	return att;
}


/*! By default just return InterfaceManager::GetUndoManager() from the default manager..
 */
Laxkit::UndoManager *anInterface::GetUndoManager()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	return imanager->GetUndoManager();
}


/*! Convenient function to return InterfaceManager::GetDefault(true)->GetResourceManager().
 */ 
Laxkit::ResourceManager *anInterface::GetResourceManager()
{
	InterfaceManager *imanager = InterfaceManager::GetDefault(true);
	return imanager->GetResourceManager();
}


/*! Return a context sensitive menu for screen position (x,y).
 *
 * This should be a menu instance that gets deleted by the popped up menu. 
 * The interface should not refer to it again. 
 *
 * Default implementation here is to simply return whatever was in menu. Usually if menu already exists,
 * then this instance will append items to it.
 *
 * The ViewportWindow will call this function, and spawn a PopupWindow.
 * When the menu sends, the event->info2 will have the item->id, and event->info4 will have the item->info,
 * as a "viewportmenu" message.
 */
Laxkit::MenuInfo *anInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	return menu;
}


/*! Return whether we will take data based on action, and optionally
 * fill in rect with the region of the window this response pertains to.
 * If you do nothing with rect, it is assumed the whole window is relevant.
 *
 * Set type_ret to the index of types for your preferred data type.
 *
 * If a child window under x,y will accept, then return that in child_ret.
 *
 * Default is to reject drop.
 */
bool anInterface::DndWillAcceptDrop(int x, int y, const char *action, Laxkit::IntRectangle &rect, char **types, int *type_ret)
{
	return false;
}

/*! Called from a SelectionNotify event. This is used for both generic selection events
 * (see selectionPaste()) and also drag-and-drop events.
 *
 * Typical actual_type values:
 *  - text/uri-list
 *  - text/plain
 *  - text/plain;charset=UTF-8
 *  - text/plain;charset=ISO-8859-1
 *  - TEXT
 *  - UTF8_STRING
 *
 * Returns 0 if used, nonzero otherwise.
 */
int anInterface::selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type,const char *which)
{
	return 1;
}


} // namespace LaxInterfaces

