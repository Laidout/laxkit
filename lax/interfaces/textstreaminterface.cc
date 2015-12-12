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
//    Copyright (C) 2015 by Tom Lechner
//



#include <lax/interfaces/textstreaminterface.h>

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

/*! \class TextStreamInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


TextStreamInterface::TextStreamInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	textstream_interface_style=0;

	showdecs=1;
	needtodraw=1;

	sc=NULL; //shortcut list, define as needed in GetShortcuts()

	extrahover=NULL;
	extra_type=0;
}

TextStreamInterface::~TextStreamInterface()
{
	if (sc) sc->dec_count();
	if (extrahover) { delete extrahover; extrahover=NULL; }
}

const char *TextStreamInterface::whatdatatype()
{ 
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *TextStreamInterface::Name()
{ return _("TextStreamInterface tool"); }


//! Return new TextStreamInterface.
/*! If dup!=NULL and it cannot be cast to TextStreamInterface, then return NULL.
 */
anInterface *TextStreamInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new TextStreamInterface(NULL,id,NULL);
	else if (!dynamic_cast<TextStreamInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int TextStreamInterface::UseThis(anObject *nobj, unsigned int mask)
{
//	if (!nobj) return 1;
//	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
//	if (ls!=NULL) {
//		if (mask&GCForeground) { 
//			linecolor=ls->color;
//		}
////		if (mask&GCLineWidth) {
////			linecolor.width=ls->width;
////		}
//		needtodraw=1;
//		return 1;
//	}
	return 0;
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int TextStreamInterface::InterfaceOn()
{ 
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int TextStreamInterface::InterfaceOff()
{ 
	Clear(NULL);
	if (extrahover) { delete extrahover; extrahover=NULL; }
	showdecs=0;
	needtodraw=1;
	return 0;
}

void TextStreamInterface::Clear(SomeData *d)
{
}

void TextStreamInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

Laxkit::MenuInfo *TextStreamInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{ 
//	if (no menu for x,y) return NULL;
//
//	if (!menu) menu=new MenuInfo;
//	if (!menu->n()) menu->AddSep(_("Some new menu header"));
//
//	menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path, LAX_ISTOGGLE|(istyle&FREEHAND_Raw_Path)?LAX_CHECKED:0);
//	menu->AddItem(_("Some menu item"), SOME_MENU_VALUE);
//	menu->AddSep(_("Some separator text"));
//	menu->AddItem(_("Et Cetera"), SOME_OTHER_VALUE);
//	return menu;

	return NULL;
}

int TextStreamInterface::Event(const Laxkit::EventData *data, const char *mes)
{
//    if (!strcmp(mes,"menuevent")) {
//        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
//        int i =s->info2; //id of menu item
//
//        if ( i==SOME_MENU_VALUE) {
//			...
//		}
//
//		return 0; 
//	}

	return 1; //event not absorbed
}



int TextStreamInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;

	DBG cerr <<"--------------TextStreamInterface::Refresh()-------------"<<endl;

	if (extrahover) {
		DBG cerr <<"--------------drawing extrahover-------------"<<endl;
		dp->NewFG(255,0,255);
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
		
		double m[6];
		//viewport->transformToContext(m,extrahover,1,-1);
		//dp->PushAndNewTransform(m);

		viewport->transformToContext(m,extrahover,0,-1);
		dp->PushAndNewTransform(m);
		dp->moveto(extrahover->obj->minx, extrahover->obj->miny);
		dp->lineto(extrahover->obj->maxx, extrahover->obj->miny);
		dp->lineto(extrahover->obj->maxx, extrahover->obj->maxy);
		dp->lineto(extrahover->obj->minx, extrahover->obj->maxy);
		dp->closed();
		dp->stroke(0);
		dp->PopAxes(); 

		//dp->PopAxes();

	}



	 //draw some text name
	//dp->DrawScreen();
	//dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	//dp->NewFG(curwindow->win_colors->fg);
	//dp->textout((dp->Maxx+dp->Minx)/2,(dp->Maxy+dp->Miny)/2, "Blah!",,-1, LAX_CENTER);
	//dp->drawline(dp->Minx,dp->Miny, dp->Maxx,dp->Maxy);
	//dp->DrawReal();

	return 0;
}

int TextStreamInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	buttondown.down(d->id,LEFTBUTTON,x,y);

	if (extrahover) { delete extrahover; extrahover=NULL; }

	//int device=d->subid; //normal id is the core mouse, not the controlling sub device
	//DBG cerr <<"device: "<<d->id<<"  subdevice: "<<d->subid<<endl;
	//LaxDevice *dv=app->devicemanager->findDevice(device);
	//device_name=dv->name;

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int TextStreamInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}


enum TextStreamActions {
	TXT_None=0,
	TXT_Hover_Outside,
	TXT_Hover_Outside_Stroke,
	TXT_Hover_Inside,
	TXT_Hover_Inside_Stroke,
	TXT_Hover_Area,
	TXT_Hover_New,
	TXT_Offset,
	TXT_Position,
	TXT_MAX
};

int TextStreamInterface::scan(int x,int y,unsigned int state)
{
	return TXT_None;
}

/*! copies oc.
 */
int TextStreamInterface::Track(ObjectContext *oc)
{
	if (!oc) {
		DBG cerr <<" -- tracking nothing"<<endl;
		if (extrahover) { delete extrahover; extrahover=NULL; }
		needtodraw=1;
		return 0;
	}

	DBG cerr <<" -- track "<<oc->obj->Id()<<endl;
	if (extrahover && oc->isequal(extrahover)) return 0; //already tracked!

	if (extrahover) delete extrahover; 
	extrahover=oc->duplicate();

	//determine inset path in which to lay text
	//determine outline path.. depending on mode, we need:
	//    base path
	//    outer stroke line
	//    inner stroke line
	//
	//hover move changes insertion point
	//hover in area activates rotation handles
	//drag in area change baseline offset
	//click and drag to change offset off line


	needtodraw=1;

	return 0;
}

int TextStreamInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{

	if (!buttondown.any()) {
		int hover=scan(x,y,state);

		if (hover==TXT_None) {
			 //set up to outline potentially editable other captiondata
			ObjectContext *oc=NULL;
			int c=viewport->FindObject(x,y, NULL, NULL,1,&oc);

			if (c>0) {
				DBG cerr <<"caption mouse over: "<<oc->obj->Id()<<endl;

				Track(oc);
				hover=TXT_Hover_New;
			} else Track(NULL);

		}

		if (hover!=TXT_None && hover!=TXT_Hover_New && extrahover) {
			delete extrahover;
			extrahover=NULL;
			needtodraw=1;
		}

		return 0;
	}

	//else deal with mouse dragging...
	

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int TextStreamInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1; //wheel up ignored
}

int TextStreamInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1; //wheel down ignored
}


int TextStreamInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"TextStreamInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int TextStreamInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{

	if (!sc) GetShortcuts();
	int action=(sc ? sc->FindActionNumber(ch,state&LAX_STATE_MASK,0) : 0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1; //key not dealt with, propagate to next interface
}

int TextStreamInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	return 1; //key not dealt with
}

Laxkit::ShortcutHandler *TextStreamInterface::GetShortcuts()
{
//	if (sc) return sc;
//    ShortcutManager *manager=GetDefaultShortcutManager();
//    sc=manager->NewHandler(whattype());
//    if (sc) return sc;
//
//    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);
//
//    sc=new ShortcutHandler(whattype());
//
//	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
//    sc->Add(CAPT_BaselineJustify, 'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),NULL,0);
//    sc->Add(CAPT_BottomJustify,   'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
//    sc->Add(CAPT_Decorations,     'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
//	sc->Add(VIEWPORT_ZoomIn,      '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),NULL,0);
//	sc->AddShortcut('=',0,0, VIEWPORT_ZoomIn); //add key to existing action
//
//    manager->AddArea(whattype(),sc);
    return sc;
}

int TextStreamInterface::PerformAction(int action)
{
	return 1;
}

} // namespace LaxInterfaces

