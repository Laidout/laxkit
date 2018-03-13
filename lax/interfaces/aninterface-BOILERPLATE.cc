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
//    Copyright (C) 2018 by Tom Lechner
//



#include <lax/interfaces/boilerplateinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>


//You only need the following includes if your new classes directly use any of the 
//Laxkit stack templates in lax/lists.h or lax/refptrstack.h. The few templates Laxkit 
//provides are divided into header.h/implementation.cc, so this provides
//template implementation:
//#include <lax/lists.cc>
//#include <lax/refptrstack.cc>


using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//--------------------------- BoilerPlateData -------------------------------------

/*! \class BoilerPlateData
 * \ingroup interfaces
 * \brief Data that BoilerPlateInterface can use.
 */

BoilerPlateData::BoilerPlateData()
{
}

BoilerPlateData::~BoilerPlateData()
{
}



//--------------------------- BoilerPlateInterface -------------------------------------

/*! \class BoilerPlateInterface
 * \ingroup interfaces
 * \brief Exciting interface that does stuff.
 */


BoilerPlateInterface::BoilerPlateInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{ ***
	interface_flags=0;

	showdecs   = 1;
	needtodraw = 1;

	dataoc     = NULL;
	data       = NULL;

	sc = NULL; //shortcut list, define as needed in GetShortcuts()
}

BoilerPlateInterface::~BoilerPlateInterface()
{ ***
	if (dataoc) delete dataoc;
	if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
}

const char *BoilerPlateInterface::whatdatatype()
{ *** 
	return "BoilerPlateData";
	//return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *BoilerPlateInterface::Name()
{ *** return _("BoilerPlate Interface"); }


//! Return new BoilerPlateInterface.
/*! If dup!=NULL and it cannot be cast to BoilerPlateInterface, then return NULL.
 */
anInterface *BoilerPlateInterface::duplicate(anInterface *dup)
{ ***
	if (dup==NULL) dup=new BoilerPlateInterface(NULL,id,NULL);
	else if (!dynamic_cast<BoilerPlateInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

//! Use the object at oc if it is an BoilerPlateData.
int BoilerPlateInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	BoilerPlateData *ndata=dynamic_cast<BoilerPlateData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) deletedata();
	if (dataoc) delete dataoc;
	dataoc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}
	needtodraw=1;
	return 1;
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int BoilerPlateInterface::UseThis(anObject *nobj, unsigned int mask)
{ ***
	if (!nobj) return 1;
	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
	if (ls!=NULL) {
		if (mask&GCForeground) { 
			linecolor=ls->color;
		}
//		if (mask&GCLineWidth) {
//			linecolor.width=ls->width;
//		}
		needtodraw=1;
		return 1;
	}
	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *BoilerPlateInterface::Context()
{
	return dataoc;
}

/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int BoilerPlateInterface::InterfaceOn()
{ *** 
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int BoilerPlateInterface::InterfaceOff()
{ *** 
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

/*! Clear references to d within the interface.
 */
void BoilerPlateInterface::Clear(SomeData *d)
{ ***
	if (dataoc) { delete dataoc; dataoc=NULL; }
	if (data) { data->dec_count(); data=NULL; }
}

void BoilerPlateInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

/*! Return a context specific menu, typically in response to a right click.
 */
Laxkit::MenuInfo *BoilerPlateInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{ ***
	if (no menu for x,y) return menu;

	if (!menu) menu=new MenuInfo;
	if (!menu->n()) menu->AddSep(_("Some new menu header"));

	menu->AddToggleItem(_("New checkbox"), laximage_icon, YOUR_CHECKBOX_ID, checkbox_info, (istyle & STYLEFLAG) /*on*/, -1 /*where*/);
	menu->AddItem(_("Some menu item"), YOUR_MENU_VALUE);
	menu->AddSep(_("Some separator text"));
	menu->AddItem(_("Et Cetera"), YOUR_OTHER_VALUE);
	menp->AddItem(_("Item with info"), YOUR_ITEM_ID, LAX_OFF, items_info);

	 //include <lax/iconmanager.h> if you want access to default icons
	LaxImage icon = iconmanager->GetIcon("NewDirectory");
	menp->AddItem(_("Item with icon"), icon, SOME_ITEM_ID, LAX_OFF, items_info);

	return menu;
}

/*! Intercept events if necessary, such as from the ContextMenu().
 */
int BoilerPlateInterface::Event(const Laxkit::EventData *data, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		 //these are sent by the ContextMenu popup
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int i	= s->info2; //id of menu item
		int info = s->info4; //info of menu item

		if ( i==SOME_MENU_VALUE) {
			...
		}

		return 0; 
	}

	return 1; //event not absorbed
}


/*! Draw some data other than the current data.
 * This is called during screen refreshes.
 */
int BoilerPlateInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<BoilerPlateData *>(ndata)==NULL) return 1;

	BoilerPlateData *bzd=data;
	data=dynamic_cast<BoilerPlateData *>(ndata);

	 // store any other state we need to remember
	 // and update to draw just the temporary object, no decorations
	int td=showdecs,ntd=needtodraw;
	showdecs=0;
	needtodraw=1;

	Refresh();

	 //now restore the old state
	needtodraw=ntd;
	showdecs=td;
	data=bzd;
	return 1;
}


int BoilerPlateInterface::Refresh()
{ *** 

	if (needtodraw==0) return 0;
	needtodraw=0;




	 //draw some text name
	dp->DrawScreen();
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->NewFG(curwindow->win_colors->fg);
	dp->textout((dp->Maxx+dp->Minx)/2,(dp->Maxy+dp->Miny)/2, "Blah!",,-1, LAX_CENTER);
	dp->drawline(dp->Minx,dp->Miny, dp->Maxx,dp->Maxy);
	dp->DrawReal();

	return 0;
}

/*! Check for clicking down on other objects, possibly changing control to that other object.
 *
 * Return 1 for changed object to another of same type.
 * Return 2 for changed to object of another type (switched tools).
 * Return 0 for nothing found at x,y.
 */
int BoilerPlateInterface::OtherObjectCheck(int x,int y,unsigned int state) 
{
	ObjectContext *oc=NULL;
	int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
	SomeData *obj=NULL;
	if (c>=0 && oc && oc->obj && draws(oc->obj->whattype())) obj=oc->obj;

	if (obj) { 
		 // found another BoilerPlateData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		UseThisObject(oc); 
		if (viewport) viewport->ChangeObject(oc,0);
		needtodraw=1;
		return 1;

	} else if (c<0) {
		 // If there is some other type of data underneath (x,y).
		 // if *this is not primary, then switch objects, and switch tools to deal
		 // with that object.
		//******* need some way to transfer the LBDown to the new tool
		if (!primary && c==-1 && viewport->ChangeObject(oc,1)) {
			buttondown.up(d->id,LEFTBUTTON);
			return 2;
		}
	}

	return 0;
}

int BoilerPlateInterface::scan(int x, int y, unsigned int state)
{
	if (on something) return BOILERPLATE_Something;

	return BOILERPLATE_None;
}

int BoilerPlateInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{ ***
	//int device=d->subid; //normal id is the core mouse, not the controlling sub device
	//DBG cerr <<"device: "<<d->id<<"  subdevice: "<<d->subid<<endl;
	//LaxDevice *dv=app->devicemanager->findDevice(device);
	//device_name=dv->name;


	int nhover = scan(x,y,state);
	if (nhover != hover) {
		hover=nhover;
		buttondown.down(d->id,LEFTBUTTON,x,y, nhover);
		needtodraw=1;
	}


	 // Check for clicking down on controls for existing data
	if (data && data->pointin(screentoreal(x,y))) {
		buttondown.down(d->id,LEFTBUTTON,x,y, some_hover_value);

		if ((state&LAX_STATE_MASK)==0) {
			//plain click in object. do something!
			return 0;
		}

		//do something else for non-plain clicks!
		return 0;
	}

	 //clicked down on nothing, release current data if it exists
	deletedata();

	
	 // So, was clicked outside current image or on blank space, make new one or find other one.
	int other = OtherObjectCheck(x,y);
	if (other==2) return 0; //control changed to some other tool
	if (other==1) return 0; //object changed via UseThisObject().. nothing more to do here!

	 //OtherObjectCheck:
	 //  change to other type of object if not primary
	 //  change to other of same object always ok


	 // To be here, must want brand new data plopped into the viewport context
	if (we want new data) {
		//NewDataAt(x,y,state);

		if (viewport) viewport->ChangeContext(x,y,NULL);
		data=newData();
		needtodraw=1;
		if (!data) return 0;

		 //for instance...
		leftp=screentoreal(x,y);
		data->origin(leftp);
		data->xaxis(flatpoint(1,0)/Getmag()/2);
		data->yaxis(flatpoint(0,1)/Getmag()/2);
		DBG data->dump_out(stderr,6,0,NULL);

	} else {
		//we have some other control operation in mind...
	}

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int BoilerPlateInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{ ***
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int BoilerPlateInterface::MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{ ***
	//dragged is a rough gauge of the maximum distance the mouse was from the original point
	int dragged=buttondown.down(d->id,MIDDLEBUTTON,x,y);

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int BoilerPlateInterface::MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{ ***
	buttondown.up(d->id,MIDDLEBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//// NOTE! You probably only need to redefine ContextMenu(), instead of grabbing right button,
//// Default right button is to pop up a Context menu for the coordinate.
//
//int BoilerPlateInterface::RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
//{ ***
//	buttondown.down(d->id,RIGHTBUTTON,x,y);
//	return 0; //return 0 for absorbing event, or 1 for ignoring
//}
//
//int BoilerPlateInterface::RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
//{ ***
//	buttondown.up(d->id,RIGHTBUTTON);
//	return 0; //return 0 for absorbing event, or 1 for ignoring
//}


int BoilerPlateInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{ ***
	if (!buttondown.any()) {
		// update any mouse over state
		int nhover = scan(x,y,state);
		if (nhover != hover) {
			hover=nhover;
			buttondown.down(d->id,LEFTBUTTON,x,y, nhover);

			PostMessage(_("Something based on new hover value"));
			needtodraw=1;
			return 0;
		}
		return 1;
	}

	//else deal with mouse dragging...
	

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int BoilerPlateInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{ ***
	return 1; //wheel up ignored
}

int BoilerPlateInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{ ***
	return 1; //wheel down ignored
}


int BoilerPlateInterface::send()
{ ***
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"BoilerPlateInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int BoilerPlateInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{ ***
	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask|AltMask|MetaMask)) {
		//deal with various modified keys...
	}

	if (ch==LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
		if (nothing selected) return 1; //need to return on plain escape, so that default switching to Object tool happens
		
		 //else..
		ClearSelection();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Up) {
		//*** stuff

	} else {
		 //default shortcut processing

		if (!sc) GetShortcuts();
		int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
		if (action>=0) {
			return PerformAction(action);
		}
	}

	return 1; //key not dealt with, propagate to next interface
}

int BoilerPlateInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{ ***
	return 1; //key not dealt with
}

Laxkit::ShortcutHandler *BoilerPlateInterface::GetShortcuts()
{ ***
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc=new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    sc->Add(BOILERPLATE_Something,  'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),NULL,0);
    sc->Add(BOILERPLATE_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
    sc->Add(BOILERPLATE_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
	sc->Add(BOILERPLATE_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),NULL,0);
	sc->AddShortcut('=',0,0, BOILERPLATE_Something); //add key to existing action

    manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int BoilerPlateInterface::PerformAction(int action)
{ ***
	return 1;
}

} // namespace LaxInterfaces

