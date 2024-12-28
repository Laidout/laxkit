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
//    Copyright (C) 2023-present by Tom Lechner
//


#include <lax/interfaces/beznetinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>

#include <lax/debug.h>
using namespace std;

using namespace Laxkit;


namespace LaxInterfaces {


//--------------------------- BezNetInterface -------------------------------------

BezNetToolSettings::BezNetToolSettings()
{
	max_arrow_length = .2;
	default_vertex_color.rgbf(1.0,0.,0.);
	default_edge_color.rgbf(0.,0.,1.);
	default_face_color.rgbf(0.0, 1.0, 0.0, .5); //transparentish faces
}

BezNetToolSettings::~BezNetToolSettings()
{
	DBGL("BezNetToolSettings destructor");
}


//--------------------------- BezNetInterface -------------------------------------

/*! \class BezNetInterface
 * \ingroup interfaces
 * Interface to create and modify networks of non-intersecting bezier lines.
 */

// static variable initialization
Laxkit::SingletonKeeper BezNetInterface::settingsObject;


BezNetInterface::BezNetInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_flags=0;

	showdecs   = 1;
	needtodraw = 1;

	dataoc     = nullptr;
	data       = nullptr;

	sc = nullptr; //shortcut list, define as needed in GetShortcuts()

	settings = dynamic_cast<BezNetToolSettings*>(settingsObject.GetObject());
	if (!settings) {
		settings = new BezNetToolSettings();
		settingsObject.SetObject(settings, false);
		settings->dec_count();
	}
}

BezNetInterface::~BezNetInterface()
{
	DBGM("BezNetInterface destructor start")
	if (dataoc) delete dataoc;
	if (data) { data->dec_count(); data = nullptr; }
	if (sc) sc->dec_count();
	DBGM("BezNetInterface destructor end")
}


const char *BezNetInterface::whatdatatype()
{ 
	return "BezNetData";
}


/*! Name as displayed in menus, for instance.
 */
const char *BezNetInterface::Name()
{ return _("Bezier Net"); }


//! Return new BezNetInterface.
/*! If dup!=nullptr and it cannot be cast to BezNetInterface, then return nullptr.
 */
anInterface *BezNetInterface::duplicate(anInterface *dup)
{
	if (dup == nullptr) dup = new BezNetInterface(nullptr,id,nullptr);
	else if (!dynamic_cast<BezNetInterface *>(dup)) return nullptr;
	return anInterface::duplicate(dup);
}


//! Use the object at oc if it is an BezNetData.
int BezNetInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	BezNetData *ndata=dynamic_cast<BezNetData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data != ndata) deletedata();
	if (dataoc) delete dataoc;
	dataoc = oc->duplicate();

	if (data != ndata) {
		data = ndata;
		data->inc_count();
	}
	needtodraw = 1;
	return 1;
}


/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int BezNetInterface::UseThis(anObject *nobj, unsigned int mask)
{
// 	if (!nobj) return 1;
// 	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
// 	if (ls!=nullptr) {
// 		if (mask & (LINESTYLE_Color | LINESTYLE_Color2)) { 
// 			linecolor=ls->color;
// 		}
// //		if (mask & LINESTYLE_Width) {
// //			linecolor.width=ls->width;
// //		}
// 		needtodraw=1;
// 		return 1;
// 	}
	return 0;
}


/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *BezNetInterface::Context()
{
	return dataoc;
}


/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int BezNetInterface::InterfaceOn()
{
	showdecs = 1;
	needtodraw = 1;
	return 0;
}


/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int BezNetInterface::InterfaceOff()
{
	Clear(nullptr);
	showdecs   = 0;
	needtodraw = 1;
	return 0;
}


/*! Clear references to d within the interface.
 */
void BezNetInterface::Clear(SomeData *d)
{
	if (dataoc) { delete dataoc; dataoc = nullptr; }
	if (data) { data->dec_count(); data = nullptr; }
	needtodraw = 1;
}


void BezNetInterface::ClearSelection()
{
	selected.flush();
}


/*! Return a context specific menu, typically in response to a right click.
 */
Laxkit::MenuInfo *BezNetInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	// if (no menu for x,y) return menu;

	// if (!menu) menu=new MenuInfo;
	// if (!menu->n()) menu->AddSep(_("Some new menu header"));

	// menu->AddToggleItem(_("New checkbox"), laximage_icon, YOUR_CHECKBOX_ID, checkbox_info, (istyle & STYLEFLAG) /*on*/, -1 /*where*/);
	// menu->AddItem(_("Some menu item"), YOUR_MENU_VALUE);
	// menu->AddSep(_("Some separator text"));
	// menu->AddItem(_("Et Cetera"), YOUR_OTHER_VALUE);
	// menp->AddItem(_("Item with info"), YOUR_ITEM_ID, LAX_OFF, items_info);

	//  //include <lax/iconmanager.h> if you want access to default icons
	// LaxImage icon = iconmanager->GetIcon("NewDirectory");
	// menp->AddItem(_("Item with icon"), icon, SOME_ITEM_ID, LAX_OFF, items_info);

	return menu;
}


/*! Intercept events if necessary, such as from the ContextMenu().
 */
int BezNetInterface::Event(const Laxkit::EventData *evdata, const char *mes)
{
	// if (!strcmp(mes,"menuevent")) {
	// 	 //these are sent by the ContextMenu popup
	// 	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(evdata);
	// 	int i	= s->info2; //id of menu item
	// 	int info = s->info4; //info of menu item

	// 	// check actions first
	// 	if ( i == SOME_MENU_VALUE
	// 	  || i == SOME_OTHER_VALUE
	// 	  ) {
	// 		PerformAction(i);
	// 		return 0;
	// 	}

	// 	return 0; 
	// }

	return 1; //event not absorbed
}


/*! Draw some data other than the current data.
 * This is called during screen refreshes.
 */
int BezNetInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<BezNetData *>(ndata) == nullptr) return 1;

	BezNetData *bzd = data;
	data = dynamic_cast<BezNetData *>(ndata);

	// store any other state we need to remember
	// and update to draw just the temporary object, no decorations
	int td     = showdecs;
	int ntd    = needtodraw;
	showdecs   = 0;
	needtodraw = 1;

	Refresh();

	// now restore the old state
	needtodraw = ntd;
	showdecs   = td;
	data       = bzd;
	return 1;
}


int BezNetInterface::Refresh()
{ 

	if (needtodraw == 0) return 0;
	needtodraw = 0;
	if (!data) return 0;

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->LineWidthScreen(ScreenLine());
	double gap = ScreenLine() * 5 / dp->Getmag();

	dp->NewFG(settings->default_face_color);
	dp->fontsize(.1);
	// draw faces
	char ch[12];
	for (int c=0; c<data->faces.n; c++) {
		BezFace *face = data->faces.e[c];
		if (!face->halfedge) continue;

		HalfEdge *e = face->halfedge;
		flatpoint p;
		int n = 0;
		do {
			p += e->vertex->p;
			n++;
			e = e->next;
		} while (e && e != face->halfedge);
		p /= n;
		dp->drawpoint(p, 5*ScreenLine(), 1);
		//p = dp->realtoscreen(p);

		dp->NewFG(0.,0.,0.);
		sprintf(ch, "%d", c);
		DBGL("face "<<c<<" at "<<p.x<<','<<p.y);
		dp->textout(p.x,p.y,ch,strlen(ch),LAX_CENTER);
		//dp->drawnum(p.x,p.y, c);

		dp->NewFG(settings->default_face_color);
		if (hover_face == c) {
			DBGL("  face point count: "<<n<<"  cache points: "<<face->cache_outline.n);
			dp->drawlines(face->cache_outline.e, face->cache_outline.n, 1, 1);

			flatpoint p;
			dp->LineWidthScreen(4*ScreenLine());
			dp->NewFG(1.0,0.,1.);
			HalfEdge *e = face->halfedge;
			do {
				DrawEdgeArrow(e, 1, gap);
				e = e->next;
			} while (e && e != face->halfedge);

			dp->NewFG(settings->default_face_color);
			dp->LineWidthScreen(ScreenLine());
		}
	}

	dp->NewFG(settings->default_edge_color);
	flatpoint p1, p2;
	// double l;
	
	// draw edges
	for (int c=0; c<data->edges.n; c++) {
		if (data->edges.e[c]->path) {
			// draw bezier path between vertices

		} else if (!data->edges.e[c]->path && !(data->edges.e[c]->twin && data->edges.e[c]->twin->path)) {
			// just draw a straight line
			HalfEdge *edge = data->edges.e[c];

			p1 = edge->vertex->p;
			p2 = edge->twin->vertex->p;

			dp->drawline(p1, p2);
			flatpoint mid = (p1 + p2)/2;
			flatpoint v = (p2 - p1)/2;
			v.setLength(MIN(v.norm(), settings->max_arrow_length));
			// double l = v.norm();
			if (edge->face      ) dp->drawarrow(mid, -v,  gap, 1, 2, 1, true);
			if (edge->twin->face) dp->drawarrow(mid, v/2, gap, 1, 2, 1, true);
		}
	}

	// draw points
	dp->NewFG(settings->default_vertex_color);
	for (int c=0; c<data->vertices.n; c++) {
		dp->drawpoint(data->vertices.e[c]->p, (hover == c && hover_type == BEZNET_Vertex ? 2 : 1)*5*ScreenLine(), 1);
	}


	//if (showdecs) {
	//	// draw interface decorations on top of interface data
	//}

	return 0;
}

/*! If which&1, draw main edge arrow, which&2 draw twin edge arrow. */
void BezNetInterface::DrawEdgeArrow(HalfEdge *edge, int which, double gap)
{
	flatvector p1 = edge->vertex->p;
	flatvector p2 = edge->twin->vertex->p;

	// dp->drawline(p1, p2);
	flatpoint mid = (p1 + p2)/2;
	flatpoint v = (p2 - p1)/2;
	v.setLength(MAX(v.norm(), settings->max_arrow_length));
	if ((which&1) && edge->face      ) dp->drawarrow(mid, -v,  gap, 1, 2, 1, true);
	if ((which&2) && edge->twin->face) dp->drawarrow(mid, v/2, gap, 1, 2, 1, true);
}

void BezNetInterface::deletedata()
{
	if (data) { data->dec_count(); data = nullptr; }
    if (dataoc) { delete dataoc; dataoc = nullptr; }
}


BezNetData *BezNetInterface::newData()
{
	BezNetData *obj = dynamic_cast<BezNetData*>(somedatafactory()->NewObject(LAX_BEZNETDATA));
	if (!obj) {
		obj = new BezNetData();

		VoronoiData vdata;
		vdata.CreateRandomPoints(5, 1, 0,5, 0,5);
		
		// vdata.AddPoint(flatpoint(2,2));
		// vdata.AddPoint(flatpoint(3,2));
		// vdata.AddPoint(flatpoint(4,2));
		// vdata.AddPoint(flatpoint(3,3));
		// vdata.AddPoint(flatpoint(3,1));
		
		vdata.Rebuild();
		obj = BezNetData::FromVoronoi(&vdata);
		// obj = BezNetData::FromDelaunay(&vdata);
	}
	return obj;	
}


/*! Check for clicking down on other objects, possibly changing control to that other object.
 *
 * Return 1 for changed object to another of same type.
 * Return 2 for changed to object of another type (switched tools).
 * Return 0 for nothing found at x,y.
 */
int BezNetInterface::OtherObjectCheck(int x,int y,unsigned int state) 
{
	ObjectContext *oc = nullptr;
	int c = viewport->FindObject(x,y,whatdatatype(),nullptr,1,&oc);
	SomeData *obj = nullptr;
	if (c >= 0 && oc && oc->obj && draws(oc->obj->whattype())) obj = oc->obj;

	if (obj) { 
		 // found another BezNetData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		UseThisObject(oc); 
		if (viewport) viewport->ChangeObject(oc, false, true);
		needtodraw=1;
		return 1;

	} else if (c<0) {
		 // If there is some other type of data underneath (x,y).
		 // if *this is not primary, then switch objects, and switch tools to deal
		 // with that object.
		//******* need some way to transfer the LBDown to the new tool
		if (!primary && c == -1 && viewport->ChangeObject(oc, true, true)) {
			buttondown.up(buttondown.whichdown(0,LEFTBUTTON), LEFTBUTTON);
			return 2;
		}
	}

	return 0;
}


int BezNetInterface::scanFaces(double x, double y, unsigned int state)
{
	if (!data) return -1;

	flatpoint p = screentoreal(x,y);
	p = transform_point_inverse(data->m(), p);

	for (int c=0; c<data->faces.n; c++) {
		if (point_is_in(p, data->faces.e[c]->cache_outline.e, data->faces.e[c]->cache_outline.n)) {
			return c;
		}
	}

	return -1;
}

int BezNetInterface::scan(double x, double y, unsigned int state, int *type_ret)
{
	*type_ret = BEZNET_None;

	if (!data) return BEZNET_None;

	flatpoint p = screentoreal(x,y);
	p = transform_point_inverse(data->m(), p);

	double closest_dist = 100000000.0;
	int closest_index = -1;

	for (int c=0; c<data->vertices.n; c++) {
		double dist = (data->vertices.e[c]->p - p).norm();
		if (dist < closest_dist) {
			closest_index = c;
			closest_dist = dist;
		}
	}

	// DBGM("p: "<<p.x<<","<<p.y<<"  closest_dist: "<<closest_dist<<"  index: "<< closest_index);

	if (closest_index >= 0) {
		*type_ret = BEZNET_Vertex;
		return closest_index;
	}

	return BEZNET_None;
}


int BezNetInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	// cerr << " bez down, any: "<<buttondown.any()<<endl;
	int nhover_type = 0;
	int nhover = scan(x,y,state, &nhover_type);
	if (nhover != hover || nhover_type != hover_type) {
		hover = nhover;
		hover_type = nhover_type;
		needtodraw = 1;
	}

	if (hover != BEZNET_None) {
		buttondown.down(d->id,LEFTBUTTON,x,y, hover);
		return 0;
	}


	 // Check for clicking down on controls for existing data
	if (data) {
	//if (data && data->pointin(screentoreal(x,y))) {
		// int action1 = something;
		// int action2 = something_else;
		// buttondown.down(d->id,LEFTBUTTON,x,y, action1, action2);

		// if ((state&LAX_STATE_MASK)==0) {
		// 	//plain click in object. do something!
		// 	return 0;
		// }

		//do something else for non-plain clicks!
		return 0;
	}

	 //clicked down on nothing, release current data if it exists
	deletedata();

	
	 // So, was clicked outside current image or on blank space, make new one or find other one.
	int other = OtherObjectCheck(x,y,state);
	if (other==2) return 0; //control changed to some other tool
	if (other==1) return 0; //object changed via UseThisObject().. nothing more to do here!

	 //OtherObjectCheck:
	 //  change to other type of object if not primary
	 //  change to other of same object always ok


	 // To be here, must want brand new data plopped into the viewport context
	if (true) { //we want new data
		//NewDataAt(x,y,state);

		viewport->ChangeContext(x,y,nullptr);
		data = newData();

		// insert data
		if (data) {
			ObjectContext *oc = nullptr;
			viewport->NewData(data, &oc);
			if (dataoc) delete dataoc;
			dataoc = oc->duplicate();
		}

		needtodraw = 1;
		if (!data) return 0;

		 //for instance...
		//leftp = screentoreal(x,y);
		//data->origin(leftp);
		//data->xaxis(flatpoint(1,0)/Getmag()/2);
		//data->yaxis(flatpoint(0,1)/Getmag()/2);
		//DBG data->dump_out(stderr,6,0,nullptr);

		hover = BEZNET_None;
		buttondown.down(d->id,LEFTBUTTON,x,y, hover);

	} else {
		//we have some other control operation in mind...
	}

	return 0; //return 0 for absorbing event, or 1 for ignoring
}


int BezNetInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	// cerr << " bez up, any: "<<buttondown.any()<<endl;
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}


int BezNetInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	// DBG cerr << " bez move, any: "<<buttondown.any()<<endl;
	if (!buttondown.any()) {
		// update any mouse over state
		int nhover_type = 0;
		int nhover = scan(x,y,state, &nhover_type);
		if (nhover != hover || nhover_type != hover_type) {
			hover = nhover;
			hover_type = nhover_type;
			
			const char *msg = nullptr;
			if (nhover_type == BEZNET_Vertex) msg = "vertex";
			else if (nhover_type == BEZNET_Edge) msg = "edge";
			else if (nhover_type == BEZNET_Face) msg = "face";
			if (msg) PostMessage2(_("Index %d type %s"), hover, msg);
			else PostMessage2(_("Index %d type %d"), hover, nhover_type);
			needtodraw = 1;
			return 0;
		}

		int hface = scanFaces(x,y,state);
		DBGL("hover face: "<< hface);
		if (hface != hover_face) {
			hover_face = hface;
			needtodraw = 1;
		}
		return 1;
	}

//	//else deal with mouse dragging...
//
//	int oldx, oldy;
//    int action1, action2;
//    buttondown.move(d->id,x,y, &oldx,&oldy);
//    buttondown.getextrainfo(d->id,LEFTBUTTON, &action1, &action2);

	

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}


int BezNetInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"BezNetInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,nullptr);
//	}

	return 0;
}

int BezNetInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	//if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask|AltMask|MetaMask)) {
	//	//deal with various modified keys being pressed...
	//}

	if (ch == LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
		if (selected.n == 0) return 1; //need to return on plain escape, so that default switching to Object tool happens
		
		 //else..
		ClearSelection();
		needtodraw = 1;
		return 0;

	} 

	// default shortcut processing
	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action >= 0) {
		return PerformAction(action);
	}

	return 1; //key not dealt with, propagate to next interface
}


//int BezNetInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
//{ ***
//	if ((state&LAX_STATE_MASK) == (ControlMask|ShiftMask|AltMask|MetaMask)) {
//		//deal with various modified keys being released...
//	}
//
//	return 1; //key not dealt with
//}


Laxkit::ShortcutHandler *BezNetInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager = GetDefaultShortcutManager();
    sc = manager->FindHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc = new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    //sc->Add(BEZNET_Something,  'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),nullptr,0);
    //sc->Add(BEZNET_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),nullptr,0);
    //sc->Add(BEZNET_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),nullptr,0);
	//sc->Add(BEZNET_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),nullptr,0);
	//sc->AddShortcut('=',0,0, BEZNET_Something); //add key to existing action

    manager->AddArea(whattype(),sc);
    return sc;
}


/*! Return 0 for action performed, or nonzero for unknown action.
 */
int BezNetInterface::PerformAction(int action)
{
	return 1;
}


} // namespace LaxInterfaces
