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
//    Copyright (C) 2021 by Tom Lechner
//



#include <lax/interfaces/mirrorinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>



using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//--------------------------- MirrorToolSettings -------------------------------------

MirrorToolSettings::MirrorToolSettings()
{
	knob.rgbf(.8, .3, .8);
	line.rgbf(.6, .1, .6);
	line_width = 1;
	knob_size = 5;
}


//--------------------------- MirrorData -------------------------------------

/*! \class MirrorData
 * \ingroup interfaces
 * \brief Data that MirrorInterface can use.
 */

MirrorData::MirrorData()
{
	p1.set(-1,0);
	p2.set(1,0);
	merge = false;
	merge_threshhold = .01;
}

MirrorData::~MirrorData()
{
}

void MirrorData::FindBBox()
{
	//compute bounds in minx,maxx, miny,maxy.
	ClearBox();
	addtobounds(p1);
	addtobounds(p2);
}

SomeData *MirrorData::duplicate(SomeData *dup)
{
	MirrorData *d = dynamic_cast<MirrorData*>(dup);
	if (dup && !d) return nullptr; // wrong type!!
	if (!d) d = new MirrorData();

	d->p1 = p1;
	d->p2 = p2;
	d->merge = merge;
	d->merge_threshhold = merge_threshhold;

	return d;
}



//--------------------------- MirrorInterface -------------------------------------

/*! \class MirrorInterface
 * \ingroup interfaces
 * 
 * Interface for moving around a segment.
 * This can be used, for instance, to move linear guides, mirror axes, or rays.
 *
 *
 */




MirrorInterface::MirrorInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_flags=0;

	showdecs   = 1;
	needtodraw = 1;
	settings   = new MirrorToolSetting();

	dataoc     = NULL;
	data       = NULL;

	sc = NULL; //shortcut list, define as needed in GetShortcuts()
}

MirrorInterface::~MirrorInterface()
{
	settings->dec_count();
	if (dataoc) delete dataoc;
	if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
}

const char *MirrorInterface::whatdatatype()
{ 
	return "MirrorData";
	//return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *MirrorInterface::Name()
{ return _("Mirror"); }


//! Return new MirrorInterface.
/*! If dup!=NULL and it cannot be cast to MirrorInterface, then return NULL.
 */
anInterface *MirrorInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new MirrorInterface(NULL,id,NULL);
	else if (!dynamic_cast<MirrorInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

//! Use the object at oc if it is an MirrorData.
int MirrorInterface::UseThisObject(ObjectContext *oc)
{ ***
	if (!oc) return 0;

	MirrorData *ndata=dynamic_cast<MirrorData *>(oc->obj);
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
int MirrorInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;
	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *MirrorInterface::Context()
{
	return dataoc;
}

/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int MirrorInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int MirrorInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

/*! Clear references to d within the interface.
 */
void MirrorInterface::Clear(SomeData *d)
{
	if (dataoc) { delete dataoc; dataoc=NULL; }
	if (data) { data->dec_count(); data=NULL; }
}

void MirrorInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

/*! Return a context specific menu, typically in response to a right click.
 */
Laxkit::MenuInfo *MirrorInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo;

	if (menu->n()) menu->AddSep(_("Mirror"));

	menu->AddItem(_("Mirror X"), MIRROR_X);
	menu->AddItem(_("Mirror Y"), MIRROR_X);
	menu->AddItem(_("Mirror 45 deg"), MIRROR_45);
	menu->AddItem(_("Mirror -45 deg"), MIRROR_135);

	menu->AddSep(_("Some separator text"));
	menu->AddToggleItem(_("Merge"), nullptr, MIRROR_Merge, 0, mirror_data->merge);
	
	return menu;
}

/*! Intercept events if necessary, such as from the ContextMenu().
 */
int MirrorInterface::Event(const Laxkit::EventData *evdata, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		 //these are sent by the ContextMenu popup
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(evdata);
		int i	= s->info2; //id of menu item
		int info = s->info4; //info of menu item

		if ( i == MIRROR_X
		  || i == MIRROR_Y
		  || i == MIRROR_45
		  || i == MIRROR_135
		  || i == MIRROR_Merge) {
			PerformAction(i);
			return 0;
		}

		return 0; 
	}

	return 1; //event not absorbed
}


/*! Draw some data other than the current data.
 * This is called during screen refreshes.
 */
int MirrorInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{ ***
	if (!ndata || dynamic_cast<MirrorData *>(ndata)==NULL) return 1;

	MirrorData *bzd=data;
	data=dynamic_cast<MirrorData *>(ndata);

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


int MirrorInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;


	DoubleBBox box(dp->Minx,dp->Maxx,dp->Miny,dp->Maxy);
	flatline line(dp->realtoscreen(mirrordata->p1), dp->realtoscreen(mirrordata->p2));
	flatpoint p1,p2;
	int n = box.IntersectWithLine(line, &p1, &p2, nullptr,nullptr);
	if (n != 2) return 0;

	 //draw some text name
	dp->DrawScreen();
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->LineWidthScreen(settings->line_width);
	dp->NewFG(mirrordata->color);

	dp->drawline(dp->Minx,dp->Miny, dp->Maxx,dp->Maxy);

	if (mirrordata->label.Bytes()) {
		flatpoint v = p2-p1;
		dp->textout(atan2(v.y, v.x), p1, mirrordata->label.c_str(),-1, LAX_LEFT|LAX_BOTTOM);
	}

	dp->DrawReal();


//	if (showdecs) {
//		// draw interface decorations on top of interface data
//	}

	return 0;
}

void MirrorInterface::deletedata()
{
	if (data) { data->dec_count(); data=NULL; }
    if (dataoc) { delete dataoc; dataoc=NULL; }
}

MirrorData *MirrorInterface::newData()
{
}

/*! Check for clicking down on other objects, possibly changing control to that other object.
 *
 * Return 1 for changed object to another of same type.
 * Return 2 for changed to object of another type (switched tools).
 * Return 0 for nothing found at x,y.
 */
int MirrorInterface::OtherObjectCheck(int x,int y,unsigned int state) 
{
	ObjectContext *oc=NULL;
	int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
	SomeData *obj=NULL;
	if (c>=0 && oc && oc->obj && draws(oc->obj->whattype())) obj=oc->obj;

	if (obj) { 
		 // found another MirrorData to work on.
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

int MirrorInterface::scan(int x, int y, unsigned int state)
{
	if (!mirrordata) return MIRROR_None;

	flatpoint p = screentoreal(x,y);

	if ((p - mirrordata->p1).norm() < scandist) return MIRROR_P1;
	if ((p - mirrordata->p2).norm() < scandist) return MIRROR_P2;

	double t;
	double d = distance(p, flatline(mirrordata->p2, mirrordata->p1), &t);
	//if (t > 0 && t < 1) return MIRROR_Segment;
	double threshhold = NearThreshhold() / Getmag();
	if (d < threshhold) return MIRROR_Line;

	return MIRROR_None;
}

int MirrorInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{

	int nhover = scan(x,y,state);
	if (nhover != hover) {
		hover = nhover;
		buttondown.down(d->id,LEFTBUTTON,x,y, nhover);
		needtodraw=1;
		return 0;
	}


//	 // Check for clicking down on controls for existing data
//	if (data && data->pointin(screentoreal(x,y))) {
//		int action1 = something;
//		int action2 = something_else;
//		buttondown.down(d->id,LEFTBUTTON,x,y, action1, action2);
//
//		if ((state&LAX_STATE_MASK)==0) {
//			//plain click in object. do something!
//			return 0;
//		}
//
//		//do something else for non-plain clicks!
//		return 0;
//	}
//
//	 //clicked down on nothing, release current data if it exists
//	deletedata();
//
//	
//	 // So, was clicked outside current image or on blank space, make new one or find other one.
//	int other = OtherObjectCheck(x,y);
//	if (other==2) return 0; //control changed to some other tool
//	if (other==1) return 0; //object changed via UseThisObject().. nothing more to do here!
//
//	 //OtherObjectCheck:
//	 //  change to other type of object if not primary
//	 //  change to other of same object always ok


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

int MirrorInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	if (!buttondown.any()) return 1;

	int action = MIRROR_None;
	double dragged = buttondown.up(d->id,LEFTBUTTON, &action);

	if (dragged < DragThreshhold()) {
		if (action == MIRROR_P1 || action == MIRROR_P2) {
			// start input dialog for point
			***
			return 0;

		} else if (action == MIRROR_Line) {
			// send that we clicked on the line
			// or maybe edit the label
			***
			return 0;
		}
	}

	return 0; //return 0 for absorbing event, or 1 for ignoring
}


int MirrorInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		// update any mouse over state
		int nhover = scan(x,y,state);
		if (nhover != hover) {
			hover = nhover;
			buttondown.down(d->id,LEFTBUTTON,x,y, nhover);

			if (hover == MIRROR_P1) PostMessage(_("Move p1"));
			else if (hover == MIRROR_P2) PostMessage(_("Move p2"));
			else if (hover == MIRROR_Line) PostMessage(_("Move line"));
			needtodraw=1;
			return 0;
		}
		return 1;
	}

	//else deal with mouse dragging...

	int oldx, oldy;
    int action1, action2;
    buttondown.move(d->id,x,y, &oldx,&oldy);
    buttondown.getextrainfo(d->id,LEFTBUTTON, &action1, &action2);

	if (action1 == MIRROR_P1) {
		flatpoint diff = screentoreal(x,y) - screentoreal(oldx,oldy);
		mirrordata->p1 += diff;
		needtodraw = 1;
		return 0;
	}

	if (action1 == MIRROR_P2) {
		flatpoint diff = screentoreal(x,y) - screentoreal(oldx,oldy);
		mirrordata->p2 += diff;
		needtodraw = 1;
		return 0;
	}
	
	if (action1 == MIRROR_Line) {
		flatpoint diff = screentoreal(x,y) - screentoreal(oldx,oldy);
		mirrordata->p1 += diff;
		mirrordata->p2 += diff;
		needtodraw = 1;
		return 0;
	}

	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}


int MirrorInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"MirrorInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int MirrorInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
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

int MirrorInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	return 1; //key not dealt with
}

Laxkit::ShortcutHandler *MirrorInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager = GetDefaultShortcutManager();
    sc=manager->FindHandler(whattype());
    if (sc) return sc;

    sc = new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    sc->Add(MIRROR_X,  'x',0,0, "MirrorH", _("Make the mirror a horizontal line"),NULL,0);
    sc->Add(MIRROR_Y,  'y',0,0, "MirrorV", _("Make the mirror a vertical line"  ),NULL,0);
    sc->Add(MIRROR_45, '\\',0,0,"Mirror45",_("Set mirror to 45 degrees (or -45)"),NULL,0);
	sc->Add(MIRROR_135,'+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),NULL,0);

	sc->Add(MIRROR_Merge,'m',0,0,   "ToggleMerge", _("Toggle merge"),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int MirrorInterface::PerformAction(int action)
{
	switch (action) {
		case MIRROR X:
			double len = (p2-p1).norm();
			mirrordata->p2 = flatpoint(mirrordata->p1.x + len, mirrordata->p1.y);
			if (mirrordata->p2.x == mirrordata->p1.x) mirrordata->p2.x = mirrordata->p1.x+1;
			needtodraw = 1;
			return 0;

		case MIRROR_Y:
			double len = (p2-p1).norm();
			mirrordata->p2 = flatpoint(mirrordata->p1.x, mirrordata->p1.y + len);
			if (mirrordata->p2.y == mirrordata->p1.y) mirrordata->p2.y = mirrordata->p1.y+1;
			needtodraw = 1;
			return 0;

		case MIRROR_45:
			***
		case MIRROR_135:
			***
		case MIRROR_Rotate:
			***

		case MIRROR_Merge:
			mirrordata->merge = !mirrordata->merge;
			UpdateData();
			return 0;
	}
	return 1;
}

} // namespace LaxInterfaces

