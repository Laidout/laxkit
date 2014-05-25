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
//    Copyright (C) 2014 by Tom Lechner
//



#include <lax/interfaces/boilerplateinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>


//You need this if you use any of the Laxkit stack templates in lax/lists.h
#include <lax/lists.cc>


using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class BoilerPlateInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


BoilerPlateInterface::BoilerPlateInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{ ***
	boiler_interface_style=0;

	showdecs=1;
	needtodraw=1;
	device=0;
}

BoilerPlateInterface::~BoilerPlateInterface()
{ ***
}

const char *BoilerPlateInterface::whatdatatype()
{ *** 
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *BoilerPlateInterface::Name()
{ *** return _("Pressure Mapper"); }


//! Return new BoilerPlateInterface.
/*! If dup!=NULL and it cannot be cast to BoilerPlateInterface, then return NULL.
 */
anInterface *BoilerPlateInterface::duplicate(anInterface *dup)
{ ***
	if (dup==NULL) dup=new BoilerPlateInterface(NULL,id,NULL);
	else if (!dynamic_cast<BoilerPlateInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
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

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int BoilerPlateInterface::InterfaceOn()
{ *** 
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int BoilerPlateInterface::InterfaceOff()
{ *** 
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void BoilerPlateInterface::Clear(SomeData *d)
{ ***
}

Laxkit::MenuInfo *BoilerPlateInterface::ContextMenu(int x,int y,int deviceid)
{ ***
	if (no menu for x,y) return NULL;

	MenuInfo *menu=new MenuInfo;
	menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path, (freehand_style&FREEHAND_Raw_Path)?LAX_CHECKED:0);
	menu->AddItem(_("Some menu item"), SOME_MENU_VALUE);
	menu->AddSep(_("Some separator text"));
	menu->AddItem(_("Et Cetera"), SOME_OTHER_VALUE);
	return menu;
}

int BoilerPlateInterface::Event(const Laxkit::EventData *data, const char *mes)
{
    if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i =s->info2; //id of menu item

        if ( i==SOME_MENU_VALUE) {
			...
		}

		return 0; 
	}

	return 1; //event not absorbed
}



int BoilerPlateInterface::Refresh()
{ *** 

	if (needtodraw==0) return 0;
	needtodraw=0;




	 //draw some text name
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->DrawScreen();
	dp->NewFG(curwindow->win_colors->fg);
	dp->textout((dp->Maxx+dp->Minx)/2,(dp->Maxy+dp->Miny)/2, "Blah!",,-1, LAX_CENTER);
	dp->DrawReal();

	return 0;
}

//! Start a new freehand line.
int BoilerPlateInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{ ***
	buttondown.down(d->id,LEFTBUTTON,x,y);

	//device=d->subid; //normal id is the core mouse, not the controlling sub device
	//DBG cerr <<"device: "<<d->id<<"  subdevice: "<<d->subid<<endl;
	//LaxDevice *dv=app->devicemanager->findDevice(device);
	//device_name=dv->name;

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Finish a new freehand line by calling newData with it.
int BoilerPlateInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{ ***
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Start a new freehand line.
int BoilerPlateInterface::MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{ ***
	buttondown.down(d->id,MIDDLEBUTTON,x,y);

	//device=d->subid; //normal id is the core mouse, not the controlling sub device
	//LaxDevice *dv=app->devicemanager->findDevice(device);
	//device_name=dv->name;

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Finish a new freehand line by calling newData with it.
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
//
//	device=d->subid; //normal id is the core mouse, not the controlling sub device
//	DBG cerr <<"device: "<<d->id<<"  subdevice: "<<d->subid<<endl;
//	//LaxDevice *dv=app->devicemanager->findDevice(device);
//	//device_name=dv->name;
//	needtodraw=1;
//	return 0; //return 0 for absorbing event, or 1 for ignoring
//}
//
//int BoilerPlateInterface::RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
//{ ***
//	buttondown.up(d->id,RIGHTBUTTON);
//	return 0; //return 0 for absorbing event, or 1 for ignoring
//}


/*! \todo *** this isn't very sophisticated, for elegance, should use some kind of 
 * bez curve fitting to cut down on unnecessary points should use a timer so 
 * stopping makes sharp corners and closer spaced points?
 */
int BoilerPlateInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{ ***
	if (!buttondown.any()) {
		// update any mouse over state
		// ...
		return 1;
	}

	//else deal with mouse dragging...
	

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
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
		for (int c=0; c<histogram.n; c++) histogram.e[c]=0;
		max_histogram_value=0;
		needtodraw=1;
		return 0;
	}

	return 1; //key not dealt with, propagate to next interface
}

int BoilerPlateInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{ ***
	return 1; //key not dealt with
}

Laxkit::ShortcutHandler *BoilerPlateInterface::GetShortcuts()
{ ***
}

int BoilerPlateInterface::PerformAction(int action)
{ ***
}

} // namespace LaxInterfaces

