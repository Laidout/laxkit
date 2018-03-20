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
#include <lax/interfaces/objectinterface.h>
#include <lax/interfaces/rectpointdefs.h>
#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {
	


//----------------------------- ObjectInterface --------------------------------


/*! \class ObjectInterface
 * \ingroup interfaces
 * \brief Allows selecting and moving around several objects all at once.
 *
 * This holds a stack of SomeData for the selection, and transforms each according
 * to how a containing RectData is transformed.
 * 
 * If style&OBJECT_SELECT_TOUCHING, then the interface should select those objects that
 * are merely touching a dragged out rectangle. Otherwise, the objects must be totally inside
 * the dragged rectangle. (unimplemented)
 * 
 * \todo ********* must have mechanism to get viewport events like layer up, home,
 *   end, etc, to affect all in selection!!
 */



ObjectInterface::ObjectInterface(int nid,Displayer *ndp) : RectInterface(nid,ndp)
{
	dontclear=0;
	selection=new Selection;
}

ObjectInterface::~ObjectInterface() 
{
	DBG cerr <<"---- in ObjectInterface destructor"<<endl;
	if (selection) selection->dec_count();
}
		
const char *ObjectInterface::Name()
{ return _("Object"); }

anInterface *ObjectInterface::duplicate(anInterface *dup)//dup=NULL
{
	ObjectInterface *r;
	if (dup==NULL) r=new ObjectInterface(id,dp);
	else {r=dynamic_cast<ObjectInterface *>(dup);
		if (r==NULL) return NULL;
	}
	return RectInterface::duplicate(r);
}

/*! Does nothing but return 0. Cannot use any data.
 */
int ObjectInterface::UseThis(anObject *newdata,unsigned int) // assumes not use local
{
	return 0;
}

////! Set up xaxislen, yaxislen, xdir, and ydir from somedata.
///*! If first==1 then the data is newly installed. Otherwise, this
// * is called from an LBDown, and compensates for any changes made
// * from outside the interface (which are assumed to not have made
// * the transform invalid *** maybe do that check anyway?).
// */
//void ObjectInterface::syncFromData(int first)
//{***
//	if (!somedata) return;
//	xdir=somedata->xaxis();
//	ydir=somedata->yaxis();
//	origin=somedata->origin();
//	xaxislen=norm(xdir);
//	if (xaxislen) xdir/=xaxislen;
//	yaxislen=norm(ydir);
//	if (yaxislen) ydir/=yaxislen;
//	
//	// zero check done only when first initializing from data, not on just any lbdown
//	if (first) {
//		if (fabs(yaxislen)<1e-10 && fabs(xaxislen)<1e-10) { 
//			xdir=flatpoint(1,0);
//			ydir=flatpoint(0,1);
//		} else if (xaxislen && fabs(yaxislen)<1e-10) {
//			xdir=-transpose(ydir);
//		} else if (fabs(xaxislen)<1e-10 && yaxislen) {
//			ydir=transpose(xdir);
//		}
//	}
//}
//
////! Set the somedata's axes and origin from xdir, etc.
//void ObjectInterface::syncToData()
//{***
//	if (!somedata) return;
//	somedata->origin(origin);
//	somedata->xaxis((fabs(xaxislen)>1e-10?xaxislen:1e-10)*xdir);
//	somedata->yaxis((fabs(yaxislen)>1e-10?yaxislen:1e-10)*ydir);
//}

int ObjectInterface::InterfaceOn()
{
	if (selection != viewport->GetSelection()) {
		if (viewport->GetSelection()) {
			 //replace selection with the one in viewport
			selection->dec_count();
			selection=viewport->GetSelection();
			selection->inc_count();
			RemapBounds();
		} else {
			 //install object tool's selection in viewport
			viewport->SetSelection(selection);
		}
	}

	return RectInterface::InterfaceOn();
}

int ObjectInterface::InterfaceOff()
{
	if (style&RECT_FLIP_LINE) style=(style&~RECT_FLIP_LINE)|RECT_FLIP_AT_SIDES;
	showdecs=0;
	needtodraw=1;

	if (selection==viewport->GetSelection()) {
		selection->dec_count();
		selection=new Selection();
	}
	FreeSelection();

	return 0;
}

/*! Redefine Clear() to always clear the selection. The default was
 * only to clear if d==somedata.
 */
void ObjectInterface::Clear(SomeData *d)
{
	if (dontclear) return;
	//if (d && d!=somedata) return;
	FreeSelection();
}

//! Immediately returns.
/*! This function is not necessary because selections are not independent objects,
 * though in time this might change.
 */
int ObjectInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
	style|=RECT_OBJECT_SHUNT;
	return 1;
}

//! Draw the outlines of all selection objects.
int ObjectInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!somedata) {
		needtodraw=0;
		return 1;
	}

	DBG cerr <<"  ObjectInterface draw"<<endl;
	
	 // First draw the containing RectData.
	if (data) {
		//dp->PushAndNewTransform(data->m());
		int sd=showdecs;
		if (!selection->n()) showdecs=0;
		RectInterface::Refresh();
		showdecs=sd;
		//dp->PopAxes();
	}

	ScreenColor black(0.,0.,0.,1.), col;
	dp->NewFG(coloravg(&col, &controlcolor, &black));
	flatpoint pn[4];
	dp->LineAttributes(-1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
	SomeData *obj;
	ObjectContext *oc;
	double m[6];

	for (int c=0; c<selection->n(); c++) {
		 // Now draw outlines of each element in the selection.
		oc=selection->e(c);
		obj=oc->obj;
		if (!obj) continue;

		if (viewport) {
			viewport->transformToContext(m,oc,0,1);
			dp->PushAndNewTransform(m);
		} else dp->PushAndNewTransform(obj->m());

		dp->LineWidthScreen(1);
		pn[0]=flatpoint(obj->minx,obj->miny);
		pn[1]=flatpoint(obj->maxx,obj->miny);
		pn[2]=flatpoint(obj->maxx,obj->maxy);
		pn[3]=flatpoint(obj->minx,obj->maxy);
		dp->drawlines(pn,4,1,0);

		dp->PopAxes();
	}

	DBG cerr <<"  ObjectInterface end draw"<<endl;
	
	needtodraw=0;
	return 0;
}

////! Recompute the bounds of the selection
///*! When a group is moved between pages, for instance, this
// * provides the mechanism to update the bounds of the selection.
// */
//void ObjectInterface::RedoBounds()
//{
//	if (selection->n()==0) return;
//	
//	if (!data) {
//		somedata=data=new RectData();
//	}
//	data->maxx=data->minx-1;
//	data->maxy=data->miny-1;
//	transform_identity(data->m());
//	double m[6];
//	transform_identity(m);
//
//	if (viewport) viewport->transformToContext(m,oc,0,0);
//	if (selection->n()==1) {
//		transform_copy(data->m(), selection->e(0)->obj->m());
//		data->addtobounds(selection->e(0));
//	} else for (int c=0; c<selection->n(); c++) {
//		data->addtobounds(selection.e[c]->m(), selection->e(c)->obj);
//	}
//}

//! Add many objects to selection. Return number added.
int ObjectInterface::AddToSelection(Selection *nselection)
{
	if (!nselection) return 0;

	int n=0;
	for (int c=0; c<nselection->n(); c++) {
		n+=AddToSelection(nselection->e(c));
	}

	RemapBounds();
	return n;
}

//! Add many objects to selection. Return number added.
int ObjectInterface::AddToSelection(Laxkit::PtrStack<ObjectContext> &nselection)
{
	int n=0;
	for (int c=0; c<nselection.n; c++) {
		n+=AddToSelection(nselection.e[c]);
	}
	return n;
}

/*! Recompute bounds for existing selection.
 */
void ObjectInterface::RemapBounds()
{
	if (!data) {
		if (!selection->n()) return;
		somedata=data=new RectData();
	}
	data->clear();
	
	double m[6];
	if (selection->n()==1) {
		 //handle selection of 1 separately so that the selection rectangle exactly
		 //matches the object rectangle. Otherwise, it might be skewed.
		if (viewport) viewport->transformToContext(m,selection->e(0),0,1);
		else transform_copy(m,selection->e(0)->obj->m());
		data->m(m);
		data->addtobounds(selection->e(0)->obj);

	} else for (int c=0; c<selection->n(); c++) {
		if (viewport) viewport->transformToContext(m,selection->e(c),0,1);
		else transform_copy(m,selection->e(c)->obj->m());
		data->addtobounds(m, selection->e(c)->obj);
	}
	syncFromData(1);
	data->centercenter();
	center1=flatpoint((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);
}

//! Add an object to the selection, and resize bounding rectangle as appropriate.
/*! A copy of oc is taken.
 *
 * Return 1 for object added, or 0 for not.
 */
int ObjectInterface::AddToSelection(ObjectContext *oc)
{
	if (!(oc && oc->obj)) return 0;

	int c;
	for (c=0; c<selection->n(); c++) {
		if (oc->obj==selection->e(c)->obj) return 0; //object already in selection
	}
	selection->Add(oc,-1);

	if (!data) somedata=data=new RectData();

	data->maxx=data->minx-1;
	data->maxy=data->miny-1;
	data->flags|=(oc->obj->flags&(SOMEDATA_KEEP_1_TO_1|SOMEDATA_KEEP_ASPECT));

	double m[6];
	transform_identity(m);
	data->m(m);

	if (selection->n()==1) {
		 //handle selection of 1 separately so that the selection rectangle exactly
		 //matches the object rectangle. Otherwise, it might be skewed.
		if (viewport) viewport->transformToContext(m,oc,0,1);
		else transform_copy(m,selection->e(0)->obj->m());
		data->m(m);
		data->addtobounds(selection->e(0)->obj);

	} else for (int c=0; c<selection->n(); c++) {
		if (viewport) viewport->transformToContext(m,selection->e(c),0,1);
		else transform_copy(m,selection->e(c)->obj->m());
		data->addtobounds(m, selection->e(c)->obj);
	}
	syncFromData(1);
	data->centercenter();
	center1=flatpoint((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);
	
	DBG cerr <<"--Added to selection, ("<<selection->n()<<"): "<<c<<endl;
	needtodraw=1;
	return 1;
}

void ObjectInterface::deletedata()
{
	RectInterface::deletedata();
	selection->Flush();
}

//! Flush the selection and remove the bounding rectangle.
int ObjectInterface::FreeSelection()
{
	DBG cerr <<"=== FreeSelection()"<<endl;
	deletedata();

	selection->Flush();

	if (style&RECT_FLIP_LINE) style=(style&~RECT_FLIP_LINE)|RECT_FLIP_AT_SIDES;
	needtodraw=1;
	return 0;
}

//! Return whether a point is in any of the objects of the selection.
/*! Note that this is different than being within the bounds of data,
 * which is what RectInterface::scan() checks for.
 */
int ObjectInterface::PointInSelection(int x,int y)
{
	flatpoint p2, p=dp->screentoreal(x,y);
	double m[6];
	transform_identity(m);
	for (int c=0; c<selection->n(); c++) {
		if (viewport) viewport->transformToContext(m,selection->e(c),1,0);
		p2=transform_point(m,p);
		if (selection->e(c)->obj->pointin(p2)) return 1;
	}
	return 0;
}

 //remember dragmode&1 means don't show arrow handles
#define DRAG_NONE                0
#define DRAG_NEW_SELECTION       (1|(1<<1))
#define DRAG_ADD_SELECTION       (1|(2<<1))
#define DRAG_SUBTRACT_SELECTION  (1|(3<<1))

/*! Always creates a new RectData if not click on a point
 *
 * Click selects any object. Shift or Control click toggles whether an object
 * is in the selection or not *** NOTE that this disables the fancy 3 point transforms.
 * For selections of more than one object, they must
 * all be in the same level of the layer tree.
 *
 * Clicking on nothing deselects all.
 *
 * \todo Double click ought to switch to the objects proper tool.
 */
int ObjectInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	//DBG cerr << "  MAYBE in obj lbd..";
	//if (buttondown.any()) return 0;
	DBG cerr << "  in obj lbd..";

	buttondown.down(d->id,LEFTBUTTON,x,y);
	dragmode=DRAG_NONE;

	if (data) {
		 //there is already a selection
		style|=RECT_OBJECT_SHUNT;
		RectInterface::LBDown(x,y,state,count,d);

		int curpoint;
		buttondown.getextrainfo(d->id,LEFTBUTTON,&curpoint);
		UpdateInitial();
		if (curpoint!=RP_None && !(curpoint==RP_Move && !PointInSelection(x,y))) return 0;
	}
	
	 //! Get rid of old data if not clicking in it.
	if ((state&LAX_STATE_MASK)==0 && data && !PointInSelection(x,y)) {
		FreeSelection();
		UpdateInitial();
	}
		

	 // search for another viewport object to grab
	if (viewport) {
		ObjectContext *oc=NULL;
		SomeData *obj=NULL;
		int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
		if (c>0) obj=oc->obj;
		if (obj) {
			viewport->ChangeObject(oc,0);
			AddToSelection(oc);
			UpdateInitial();
			buttondown.moveinfo(d->id,LEFTBUTTON,RP_Move);
			showdecs|=SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES;
			showdecs&=~SHOW_TARGET;
			needtodraw=1;
			return 0;
		}
	}
	
	 // Not clicked on an object, so make new drag area
	if (viewport) viewport->ChangeContext(x,y,NULL);

	deletedata();//makes somedata=data=NULL
	FreeSelection();
	if ((state&LAX_STATE_MASK)==0) dragmode=DRAG_NEW_SELECTION;
	if ((state&LAX_STATE_MASK)==ShiftMask) dragmode=DRAG_ADD_SELECTION;
	if ((state&LAX_STATE_MASK)==ControlMask) dragmode=DRAG_SUBTRACT_SELECTION;

	somedata=NULL;
	somedata=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_RECTDATA));
	if (!somedata) somedata=new RectData;
	data=dynamic_cast<RectData *>(somedata); //has count=1
	if (!data) return 0;


	buttondown.down(d->id,LEFTBUTTON,x,y,RP_Scale_NE);
	flatpoint p=dp->screentoreal(x,y); //use dp level, NOT viewport context
	data->style=creationstyle; 
	data->setbounds(p.x,p.x+1e-5,p.y,p.y+1e-5);
	data->centercenter();
	createp=p;
	syncFromData(1);
	
	needtodraw=1;
	DBG cerr <<"..objlbd done   ";
	
	return 0;
}

/*! Called before doing something that is undoable.
 * Makes initial stack reflect the current state of transforms in selection.
 * Note that if GetUndoManager() returns NULL, nothing is done.
 */
void ObjectInterface::UpdateInitial()
{
	UndoManager *undomanager=GetUndoManager();
	if (!undomanager) return;

	while (initial.n<selection->n()) {
		initial.push(new SomeData());
	}

	for (int c=0; c<selection->n(); c++) {
		initial.e[c]->m(selection->e(c)->obj->m());
		initial.e[c]->setbounds(selection->e(c)->obj);
	}
}

/*! Assuming UpdateInitial() has been called before changes made, this will install
 * undo objects to GetUndoManager(). Does nothing if GetUndoManager() returns NULL.
 *
 * Also calls UpdateInitial() to sync up with current state.
 *
 * Returns the number of undo items added.
 */
int ObjectInterface::InstallTransformUndo()
{
	UndoManager *undomanager=GetUndoManager();
	if (!undomanager) return 0;

	if (initial.n != selection->n()) {
		DBG cerr << " *** Undo not initialized in ObjectInterface!!"<<endl;
	}

	for (int c=0; c<selection->n(); c++) {
		undomanager->AddUndo(new SomeDataUndo(selection->e(c)->obj,
									initial.e[c],NULL,
									selection->e(c)->obj,NULL,
									SomeDataUndo::SDUNDO_Transform,
									(c==0 ? false : true)));
	}

	UpdateInitial();
	return selection->n();
}

/*! \todo should be able to add or subtract from selection with the drag out
 */
int ObjectInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	 // This is for dragging out an initial rectangle to capture objects in.
	//if (selection->n()==0 && data) {
	int dragged=buttondown.isdragged(d->id,LEFTBUTTON);
	if (dragged && data &&
			(   dragmode==DRAG_ADD_SELECTION 
			 || dragmode==DRAG_NEW_SELECTION 
			 || dragmode==DRAG_SUBTRACT_SELECTION)) {
		GrabSelection(state);
		buttondown.up(d->id,LEFTBUTTON);
		return 0;
	}
	
	int status=RectInterface::LBUp(x,y,state,d);

	if (dragged && selection->n() && viewport) {
		for (int c=0; c<selection->n(); c++) {
			viewport->ObjectMoved(selection->e(c),1);
		}
		InstallTransformUndo();

		syncFromData(1);
	}

	return status;
}

//! From the bounds in data, make a new selection of what is inside.
/*! Viewports should define their own regional searching function to
 * grab more than one object at a time. This currently only flushes data,
 * and nothing else.
 *
 * Subclasses must call UpdateInitial() after changing the selection, if
 * you want undo to work.
 *
 * Return value is the number of new things added to the selection.
 *
 * \todo *** imp me with something more useful!!
 * \todo have flag for select inrect or touchingrect
 */
int ObjectInterface::GrabSelection(unsigned int state)
{//***
	if (!data) return 0;

	deletedata();//*** AddToSelection takes care of this...
	needtodraw=1;
	return 0;
}

/*! move drags point, control-move rotates and shears
 * <pre>
 *     + is preserve aspect, todo is have some other toggle for square..
 *     ^ on corner is rotate
 *     ^ on midpoint is shear
 *   x,w+---->
 *  y,h 1    8    7
 *  +   2    9    6
 *  |   3    4    5
 *  v
 * </pre>
 *
 * \todo *** preserve aspect on a shift-resize
 */
int ObjectInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"objinterf pointinsel: "<<PointInSelection(x,y)<<endl;
	
	if (!buttondown.isdown(d->id,LEFTBUTTON) || !data) { return RectInterface::MouseMove(x,y,state,d); }
	
	//DBG cerr <<"MouseMove in box: "<<data->minx<<","<<data->miny<<" -> "<<data->maxx<<","<<data->maxy
	//DBG      <<"   width: "<<data->maxx-data->minx<<"  height:"<<data->maxy-data->miny<<endl;
	//DBG cerr <<"MouseMove in box: "<<data->origin().x<<","<<data->origin().y<<"  x:"<<data->xaxis().x<<","<<data->xaxis().y
	//DBG      <<"   y:"<<data->yaxis().x<<","<<data->yaxis().y<<endl;
	
	if (!selection->n()) return RectInterface::MouseMove(x,y,state,d);
	
	 // See RectInterface::MouseMove for what's up here.
	double M[6],M2[6],N[6];
	transform_copy(M2,data->m());
	transform_invert(M,M2);
	RectInterface::MouseMove(x,y,state,d);
	transform_copy(M2,data->m());
	
	transform_mult(N,M,M2); //so now N is the transform in dp space for moving the selection rectangle
	TransformSelection(N);

	//RedoBounds();
	return 0;
}

void ObjectInterface::Rotate(double angle)
{
	if (!somedata) return;

	Affine t;
	flatpoint p;
	if (extrapoints&HAS_CENTER1) p=somedata->transformPoint(center1);
	else p=(somedata->transformPoint(flatpoint(somedata->minx,somedata->miny))+
				 somedata->transformPoint(flatpoint(somedata->maxx,somedata->maxy)))/2;
	t.Rotate(angle,p);
	somedata->Rotate(angle,p);
	TransformSelection(t.m());
	syncFromData(0);
	needtodraw=1;
}

void ObjectInterface::Flip(int type)
{
	if (!selection->n()) return;
	if (type!=RP_Flip_Go && type!=RP_Flip_H && type!=RP_Flip_V) return;

	double M[6],M2[6],N[6];
	transform_copy(M2,data->m());
	transform_invert(M,M2);
	RectInterface::Flip(type);
	transform_copy(M2,data->m());
	
	transform_mult(N,M,M2); //so now N is the transform in dp space for moving the selection rectangle
	TransformSelection(N);
}

//! Transform the selection rectangle by N in dp space.
/*! If anInterface::viewport!=NULL, then use ViewportWindow::transformToContext()
 * to figure out how to adjust the transform of the selected objects in place.
 *
 * Otherwise, assume the objects are on the same level as the selection rectangle.
 *
 * If s and e are greater than -1, then only transform the range [s..e].
 */
void ObjectInterface::TransformSelection(const double *N, int s, int e) 
{
	double M[6],M2[6],T[6];
	if (s<0) s=0;
	if (s>=selection->n()) s=selection->n()-1;
	if (e<0 || e>=selection->n()) e=selection->n()-1;

	for (int c=s; c<=e; c++) {
		if (viewport) {
			 //say an object A is n deep (A1-A2-A3-...-(dp space),
			 //then a transform to dp space would be: A1 * A2 * ... *An
			 //these points get moved by transform N found above. 
			 //If A' is the objects transform after the move, then
			 //  A1 * ... * An * N == A1' * A2 * A3 * ... * An
			 //so A' == (A1 * ... * An) * N * (An^-1 * A(n-1)^-1 * ... A2^-1)
			viewport->transformToContext(M,selection->e(c),0,1);
			transform_invert(M2,M);
			transform_mult(T,M2,selection->e(c)->obj->m());

			 //now M is the transform to space of the object
			 //    T is a partial inverse of that.
			 //the object transform after moving is M * N * T
			transform_mult(M2,M,N);
			transform_mult(M,M2,T);

			selection->e(c)->obj->m(M);
		} else {
			 //assume nothing between objects and dp if not in a viewport
			transform_mult(M,selection->e(c)->obj->m(),N);
			selection->e(c)->obj->m(M);
		}

	}
}

//! For subclasses to toggle whether the selected objects are grouped or not.
/*! Return 1 for state changed, or 0.
 *
 * The default here is to do nothing, since object groups are not currently defined
 * in the Laxkit. Grouping and parenting often have specific enough requirements
 * to not have groups, but a distant todo is to implement basic groups within
 * the Laxkit.
 */
int ObjectInterface::ToggleGroup()
{
	return 0;
}

/*! Group all in the selection, even if there is only one object selected..
 */
int ObjectInterface::GroupObjects()
{ 
	return 0;
}

/*! Ungroup any in the selection that are groups.
 */
int ObjectInterface::UngroupObjects()
{ 
	return 0;
}

Laxkit::ShortcutHandler *ObjectInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=RectInterface::GetShortcuts();

	sc->Add(OIA_Group,       'G',ShiftMask|AltMask|ControlMask,0,"Group",_("Group any, even if only one selected"),NULL,0);
	sc->Add(OIA_ToggleGroup, 'g',ControlMask,0,           "ToggleGroup", _("Group if many selected, or ungroup if only one group selected"),NULL,0);
	sc->Add(OIA_Ungroup,     'G',ShiftMask|ControlMask,0, "Ungroup",     _("Ungroup any selected that are groups"),NULL,0);

	return sc;
}

/*! Implements group/ungroup, and RIA_Normalize, RIA_Rectify for the whole group.
 */
int ObjectInterface::PerformAction(int action)
{
	if (action==OIA_ToggleGroup) {
		 // group and ungroup
		ToggleGroup();
		return 0;

	} else if (action==OIA_Group) {
		GroupObjects();
		return 0;

	} else if (action==OIA_Ungroup) {
		UngroupObjects();
		return 0;

	} else if (action==RIA_Normalize || action==RIA_Rectify) {
		 //apply normalization to selection
		if (!somedata) return 1;
		
		double M[6],M2[6],N[6];
		transform_copy(M2,data->m());
		transform_invert(M,M2);
		
		if (action==RIA_Rectify) {
			double x=norm(somedata->xaxis());
			somedata->xaxis(flatpoint(x,0));
		}

		flatpoint center=getpoint(RP_Center1,1);
		somedata->yaxis(transpose(somedata->xaxis()));
		center=center-getpoint(RP_Center1,1);
		somedata->origin(somedata->origin()+center);
		syncFromData(0);
		
		transform_copy(M2,data->m());
		transform_mult(N,M,M2); //now N is the transform representing the move in dp space
		TransformSelection(N);
		needtodraw=1;
		return 0;

	} else if (action==RIA_FlipHorizontal || action==RIA_FlipVertical) {
		UpdateInitial();
		int status = RectInterface::PerformAction(action);
		InstallTransformUndo();
		return status;

	}

	return RectInterface::PerformAction(action);
}

/*!
 */
int ObjectInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{

	if (ch==LAX_Esc) {
		int c=RectInterface::CharInput(ch,buffer,len,state,d); 

		if (c!=0 && (state&LAX_STATE_MASK)==0) { 
			if (extrapoints) {
				 //clear extra points
				extrapoints=0;
				needtodraw=1;
				return 0;
			}

			if (selection->n() == 1) {
				ObjectContext *oc = selection->e(0);

				if (oc->obj) {
					 //set context to current object's parent space
					oc->Up();
					oc->SetObject(NULL);
					viewport->ChangeContext(oc);
					FreeSelection(); 
				}
				needtodraw=1;
				return 0;

			} else if (selection->n() == 0 ) {
				ObjectContext *oc = viewport->CurrentContext();

				 //context was empty
				SomeData *data = viewport->GetObject(oc);
				if (data && data->Selectable()) {
					oc->SetObject(data);
					AddToSelection(oc);
				}
				//------------------
				// //try to select parent group
				//SomeData *parent = oc->obj->GetParent();

				//if (parent && parent->Selectable()) {
				//	if (oc->Up()) {
				//		PostMessage(_("Selected parent."));
				//		RemapBounds();
				//	}
				//}

				needtodraw=1;
				return 0;
			}

			 //when selection->n>1, unselect
			FreeSelection();
			return 0;
		}
		return c;

	} else if ((ch==LAX_Del || ch==LAX_Bksp) && (state&LAX_STATE_MASK)==0) { //delete
		dontclear=1;
		Selection *sel = selection->duplicate();
		while (sel->n()) {
			viewport->ChangeObject(sel->e(0),0);
			viewport->DeleteObject();
			sel->Remove(0);
		}
		delete sel;
		selection->Flush();
		dontclear=0;
		deletedata();
		needtodraw=1;
		return 0;
	}

	return RectInterface::CharInput(ch,buffer,len,state,d); 
}
 


} //namespace LaxInterfaces


