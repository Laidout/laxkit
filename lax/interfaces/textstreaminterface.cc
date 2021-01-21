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
//    Copyright (C) 2015 by Tom Lechner
//



#include <lax/interfaces/textstreaminterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/pathinterface.h>
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
	tstream_style = TXT_On_Stroke | TXT_In_Area | TXT_Draggable_Area;

	showdecs=1;
	needtodraw=1;

	sc=NULL; //shortcut list, define as needed in GetShortcuts()

	extrahover=NULL;
	extra_hover=0;
	outline_index=-1;

	flowdir.x = 1;
	fontheight = 36./72;
	close_dist = 20;

	outline.style |= PathsData::PATHS_Ignore_Weights;
	ScreenColor color(1.,0.,1.,1.);
	outline.line(2, -1, -1, &color);
	outline.linestyle->widthtype=0;//screen width
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
//      ***
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
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
		
		double m[6];
		//viewport->transformToContext(m,extrahover,1,-1);
		//dp->PushAndNewTransform(m);

		viewport->transformToContext(m,extrahover,0,-1);
		dp->PushAndNewTransform(m);

		//draw path around object
		dp->LineWidthScreen(3);
		//dp->NewFG(255,255,255);
		//dp->moveto(extrahover->obj->minx, extrahover->obj->miny);
		//dp->lineto(extrahover->obj->maxx, extrahover->obj->miny);
		//dp->lineto(extrahover->obj->maxx, extrahover->obj->maxy);
		//dp->lineto(extrahover->obj->minx, extrahover->obj->maxy);
		//dp->closed();
		//dp->stroke(1);

		//dp->LineWidthScreen(2);
		dp->NewFG(255,0,255);
		//dp->stroke(0);


		InterfaceManager *imanager = InterfaceManager::GetDefault();
		imanager->DrawDataStraight(dp, &outline);

		//------------------
		dp->PushClip(0);
		SetClipFromPaths(dp, &outline, NULL, true);

		DoubleBBox box;
		box.addtobounds(extrahover->obj->minx, extrahover->obj->miny);
		box.addtobounds(extrahover->obj->maxx, extrahover->obj->miny);
		box.addtobounds(extrahover->obj->maxx, extrahover->obj->maxy);
		box.addtobounds(extrahover->obj->minx, extrahover->obj->maxy);


		flatpoint p1, p2;
		flatpoint ydir = transpose(flowdir);
		p1 = flatpoint(extrahover->obj->minx,extrahover->obj->miny);
		//double mag = get_imagnification(m, 1,0);

		for (int c=0; c<10; c++) {
			p2 = p1 + flowdir*10;

			//p1 = flatpoint(outline.minx,outline.miny);
			//dp->drawline(transform_point_inverse(outline.m(),p1), transform_point_inverse(outline.m(),p2));
			//dp->drawline(transform_point(m,p1), transform_point(outline.m(), p2));

			dp->drawline(p1, p2);

			p1+=fontheight*ydir;
			//p1+=fontheight*ydir*mag;
		}

		dp->PopClip();
		dp->PopAxes(); 
//		------------------
//		dp->PopAxes(); 
//		DoubleBBox box;
//		box.addtobounds(transform_point(m, extrahover->obj->minx, extrahover->obj->miny));
//		box.addtobounds(transform_point(m, extrahover->obj->maxx, extrahover->obj->miny));
//		box.addtobounds(transform_point(m, extrahover->obj->maxx, extrahover->obj->maxy));
//		box.addtobounds(transform_point(m, extrahover->obj->minx, extrahover->obj->maxy));
//
//
//		flatpoint p1, p2;
//		flatpoint ydir = transpose(flowdir);
//		p1 = flatpoint(extrahover->obj->minx,extrahover->obj->miny);
//		double mag = get_imagnification(m, 1,0);
//
//		for (int c=0; c<10; c++) {
//			p2 = p1 + flowdir*10;
//
//			//p1 = flatpoint(outline.minx,outline.miny);
//			//dp->drawline(transform_point_inverse(outline.m(),p1), transform_point_inverse(outline.m(),p2));
//			//dp->drawline(transform_point(m,p1), transform_point(outline.m(), p2));
//
//			dp->drawline(p1, p2);
//
//			p1+=fontheight*ydir;
//			//p1+=fontheight*ydir*mag;
//		}
//		------------------

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


/*! Return what (x,y) is most near, plus the index of the path in extrahover.
 * Note: does not change the currently tracked object! Need to use Track() for that.
 */
int TextStreamInterface::scan(int x,int y,unsigned int state, int &index, flatpoint &hovered)
{
	if (!extrahover) return TXT_None;


	double m[6];
	flatpoint p = dp->screentoreal(x,y);
	viewport->transformToContext(m,extrahover,0,1);
	p = transform_point_inverse(m, p); //p should now be in object space


	PathsData *pathsobj = dynamic_cast<PathsData*>(extrahover->obj);
	if (pathsobj) {

		double dist=0, distalong=0, tdist=0;
		//double mag = ;
		int pathi;
		flatpoint pp = pathsobj->ClosestPoint(p, &dist, &distalong, &tdist, &pathi);

		// *** check distance within bounds
		// *** check direction compared to path

		if (pathi>=0) {
			index=pathi;
			hovered=pp;
			return TXT_Hover_Stroke;
		}

	} else {
		if (extrahover->obj->pointin(extrahover->obj->transformPoint(p), 1)) {
			index=0;
			return TXT_Hover_Area;
		}

	}

	index=-1;
	return TXT_None;
}

/*! Track the object defined by oc (copies oc). 
 * Does not select proper subline, nor scan for point closeness.
 * Only installs this object as most relevant for scanning.
 */
int TextStreamInterface::Track(ObjectContext *oc)
{
	if (!oc) {
		DBG cerr <<" -- tracking nothing"<<endl;
		if (extrahover) { delete extrahover; extrahover=NULL; }
		needtodraw=1;
		DBG PostMessage("");
		return 0;
	}

	DBG cerr <<" -- track "<<oc->obj->Id()<<endl;
	DBG PostMessage(oc->obj->Id());

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


	 //cache the path for mouseover use

	DefineOutline(0);


	needtodraw=1;

	return 0;
}

int TextStreamInterface::DefineOutline(int which)
{
	outline.clear();

	if (dynamic_cast<PathsData*>(extrahover->obj)) {
		PathsData *paths = dynamic_cast<PathsData*>(extrahover->obj);

		if (outline_index<0) outline_index=0;
		Path *npath = paths->GetOffsetPath(outline_index);
		if (npath) outline.paths.push(npath); 
		return 0;
	}
	
	 //create basic bounding box outline as default:
	Affine m=extrahover->obj->GetTransformToContext(false, 0);
	outline.moveTo(m.transformPoint(flatpoint(extrahover->obj->minx,extrahover->obj->miny)));
	DBG Coordinate *cc=outline.LastVertex();
	DBG cerr <<"--hover\n  moveto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;

	outline.lineTo(m.transformPoint(flatpoint(extrahover->obj->maxx,extrahover->obj->miny)));
	DBG cc=outline.LastVertex(); cerr <<"  lineto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;

	outline.lineTo(m.transformPoint(flatpoint(extrahover->obj->maxx,extrahover->obj->maxy)));
	DBG cc=outline.LastVertex(); cerr <<"  lineto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;

	outline.lineTo(m.transformPoint(flatpoint(extrahover->obj->minx,extrahover->obj->maxy)));
	DBG cc=outline.LastVertex(); cerr <<"  lineto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;

	outline.close();
	
	return 0;
}

int TextStreamInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{

	if (!buttondown.any()) {
		int index=-1;
		flatpoint hovered;
		int hover=scan(x,y,state, index,hovered); //searches for hits on last known object

		if (hover==TXT_None) {
			 //set up to outline potentially editable other captiondata
			ObjectContext *oc=NULL;
			int c=viewport->FindObject(x,y, NULL, NULL,1,&oc);

			if (c>0) {
				 //found object, so set up with it
				DBG cerr <<"textstream mouse over: "<<oc->obj->Id()<<endl;

				needtodraw=1;
				Track(oc);
				hover = scan(x,y,state, index,hovered);
				DefineOutline(index);

				if (hover == TXT_None) hover = TXT_Hover_New;
			} else Track(NULL); 
		}

		if (extrahover && hover==TXT_None) {
			delete extrahover;
			extrahover=NULL;
			needtodraw=1;
		}

		if (hover != TXT_Hover_New && outline_index != index) {
			DefineOutline(index);
			needtodraw=1;
		}

		outline_index = index;
		extra_hover = hover;
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

void TextStreamInterface::FlowDir(flatpoint dir)
{
	flowdir = dir;
}

void TextStreamInterface::FontHeight(double height)
{
	fontheight = height;
}

} // namespace LaxInterfaces

