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
//    Copyright (C) 2004-2011 by Tom Lechner
//



#include <lax/interfaces/viewportwithstack.h>
#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/transformmath.h>
#include <lax/colors.h>
#include <lax/colorbox.h>

#include <lax/refptrstack.cc>

#include <iostream>
using namespace std;

#define DBG 


using namespace Laxkit;
namespace LaxInterfaces {
	
//---------------------------- ViewportWithStack -----------------------

/*! \class ViewportWithStack
 * \brief ViewportWindow with an internal stack of objects
 *
 * This class maintains a simple flat (no layers) list of SomeData objects, draws them on 
 * screen in a basic way (one after another), and allows searching through them.
 *
 * It uses plain old ObjectContext objects, which store an index and the object.
 *
 * \todo *** occasionally might be useful to have the list of objects be external..
 */


//! Empty Constructor.
ViewportWithStack::ViewportWithStack(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,Laxkit::Displayer *ndp)
		: ViewportWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,ndp)
{
	vpwsfirsttime=1;

	foundtypeobj=new ObjectContext;
	foundobj=new ObjectContext;
	firstobj=new ObjectContext;
	curobj=new ObjectContext;;
}

ViewportWithStack::~ViewportWithStack()
{
	DBG cerr <<" --in ViewportWithStack  destructor"<<endl;
	 //for debugging purposes, flush here, where it can be tracked down more easily:
	datastack.flush();

	DBG cerr<<"delete curobj..."<<endl;
	if (curobj) delete curobj;

	DBG cerr<<"delete firstobj..."<<endl;
	if (firstobj) delete firstobj;

	DBG cerr<<"delete foundobj..."<<endl;
	if (foundobj) delete foundobj;

	DBG cerr<<"delete foundtypeobj..."<<endl;
	if (foundtypeobj) delete foundtypeobj;
}

void ViewportWithStack::ClearSearch()
{
	firstobj->clear();
	foundobj->clear();
	foundtypeobj->clear();
	searchx=searchy=-1;
	searchtype=NULL;
}

//! Call this to update the context for screen coordinate (x,y).
/*! Clears search and clears curobj, then sets *oc to point to it.
 * 
 * Returns 0.
 */
int ViewportWithStack::ChangeContext(int x,int y,ObjectContext **oc)
{
	ClearSearch();
	curobj->clear();
	if (oc) *oc=curobj;
	return 0;
}

//! Change the current object to be (already existing object) at oc.
/*! Default ViewportWithStack behavior is to change when oc is in fact a valid context.
 * Note that nothing is done if it is not a valid, existing object, and 0 is returned.
 * 
 * Then try to make curobj be what oc points to.\n
 *
 * Returns 1 for current object changed, otherwise 0 for not changed or d not found.
 */
int ViewportWithStack::ChangeObject(ObjectContext *oc, int switchtool)
{
	if (datastack.n==0) return 0;

	if (!oc) return 0;
	if (oc->i<0 || oc->i>=datastack.n) return 0;
	if (oc->obj && oc->obj!=datastack.e[oc->i]) {
		 // find correct context for oc->obj
		int i=datastack.findindex(oc->obj);
		if (i<0) return 0; // object not found!
		curobj->set(i,oc->obj);
	} else {
		if (oc->i>=0) curobj->set(oc->i,oc->obj);
		else return 0; //no obj in oc
	}

	if (switchtool) {
		ViewerWindow *viewer=dynamic_cast<ViewerWindow *>(win_parent); // maybe not always returns non-null
		 // makes sure curtool can take it, and makes it take it.
		if (viewer) viewer->SelectToolFor(curobj->obj->whattype(),curobj);
	}

	return 1;
}

//! Select previous (i==-2) or next (i==-1) object. Return 1 for curobj changed, 0 for not changed.
int ViewportWithStack::SelectObject(int i)
{
	SomeData *obj=curobj->obj;
	if (i==-1) { // select next object
		if (datastack.n>0) {
			if (++curobj->i>=datastack.n) curobj->i=0;
			curobj->SetObject(datastack.e[curobj->i]);
		}
	} else if (i==-2) { // select previous object
		if (datastack.n>0) {
			if (--curobj->i<0) curobj->i=datastack.n-1;
			curobj->SetObject(datastack.e[curobj->i]);
		}
	}
	if (obj==curobj->obj) return 0;

	 // makes sure viewer->curtool is the tool that can handle new object
	ViewerWindow *viewer=dynamic_cast<ViewerWindow *>(win_parent); // maybe not always returns non-null
	if (viewer) viewer->SelectToolFor(curobj->obj->whattype(),curobj);

	return 1;
}

//! Iteratively find an object with whattype dtype under screen position (x,y).
/*! 
 * If dtype==NULL, then search for any object under (x,y), otherwise restrict search
 * to dtype objects, where dtype is data->whattype().
 *
 * Returns 1 if a suitable object is found and oc is set to it. If the search is over and there were
 * no suitable objects found, 0 is returned, and oc set to NULL. If the search is over but there was 
 * a matching object found of a different type, then -1 is returned and oc is set to refer to
 * that object.
 * 
 * If an object of the proper type is found, 
 * internally oc will refer to LaidoutViewport::foundtypeobj. This context gets
 * reset with each call to FindObject.
 *
 * If an object matching the given (x,y) was found, but was of the wrong type, 
 * that object can be found in LaidoutViewport::foundobj. When such an object is 
 * found and a search is over, and an object of the correct type was not found, then 
 * -1 is returned and foundobj is put in oc.
 *
 * If an interface receives an lbdown outside of their object, then it would
 * call viewport->FindObject, which will possibly return an object that the 
 * interface can handle. If so, the interface would then call NewData with it. 
 * The interface can keep searching until it finds one it can handle.
 * If FindObject does not return something the interface can handle, then
 * it should call ChangeObject(that other data).
 * 
 * If start!=0, OR x!=searchx OR y!=searchy then
 * initialize the search so that exclude becomes firstobj. If start==1, then
 * skip this new firstobj. If start==2 or start==0 (but either x or y are different), 
 * then include exclude in the search. If a search is starting, and exclude==NULL, then firstobj
 * is the first item on the datastack.
 *
 * If this is not the start of a search, exclude will always be skipped.
 *
 * The object just before firstobj should be the last one searched against. After that,
 * NULL is returned. NULL is also returned when no matches are found at all.
 */
int ViewportWithStack::FindObject(int x,int y, const char *dtype, 
					SomeData *exclude, int start,ObjectContext **oc)
{
	if (datastack.n==0) {
		if (oc) *oc=NULL;
		return 0;
	}
	 //init the search, if necessary
	int nextindex=-1; // commence search starting from nextindex
	foundtypeobj->clear();
	if (start || x!=searchx || y!=searchy) { 
		foundobj->clear();
		nextindex=datastack.findindex(exclude);
		firstobj->set(nextindex,exclude);
		searchx=x;
		searchy=y;
		searchtype=NULL;
		if (start==2 || start==0) exclude=NULL;
		start=1;
	}
	if (dtype) searchtype=dtype;

	if (firstobj->obj==NULL && start==1) firstobj->set(0,datastack.e[0]);
	if (firstobj->obj==NULL) { // search must be over
		if (foundobj->obj) { 
			if (oc) *oc=foundobj; 
			return -1;
		}
		if (oc) *oc=NULL;
		return 0; // if at end of search already, or unable to begin
	}
	
	int c,n=datastack.n;
	SomeData *maybe=NULL;
	if (nextindex<0) nextindex=datastack.findindex(foundtypeobj->obj);
	if (nextindex<0) nextindex=firstobj->i;
	if (start==0) if (++nextindex>n) nextindex=0;
	flatpoint p=dp->screentoreal(x,y);
	for (c=nextindex; c>=0; c--) {
		if (c==firstobj->i && start==0) break;
		maybe=datastack.e[c];
		if (maybe==exclude) continue;
		if (maybe->pointin(p)) {
			if (searchtype && !strcmp(maybe->whattype(),searchtype)) break; //found right type
			if (!foundobj->obj) foundobj->set(c,maybe);
		}
	}
	 // if item not found, then continue search from top of datastack.
	if (c==-1) {
		for (c=datastack.n-1; c>firstobj->i; c--) {
			maybe=datastack.e[c];
			if (maybe==exclude) continue;
			if (maybe->pointin(p)) {
				if (searchtype && !strcmp(maybe->whattype(),searchtype)) break; //found right type
				if (!foundobj->obj) foundobj->set(c,maybe);
			}
		}
		if (c==firstobj->i) { // no more to search for.
			firstobj->clear();
			if (foundobj->obj) { 
				if (oc) *oc=foundobj; 
				if (searchtype) return -1; else return 1;
			}
			if (oc) *oc=NULL;
			return 0;
		}
	} else {
		if (c==firstobj->i && start==0) { // no more to search for.
			firstobj->clear(); 
			if (foundobj->obj) { 
				if (oc) *oc=foundobj; 
				return -1;
			}
			if (oc) *oc=NULL;
			return 0;
		}
	}
	foundobj->set(c,maybe);
	foundtypeobj->set(c,maybe);
	if (oc) *oc=foundtypeobj;
	//nextindex=c+1;
	//if (nextindex==n) nextindex=0;
	return 1;
}

//! Drop a new object at screen coordinates x,y.
int ViewportWithStack::DropObject(SomeData *d, double x,double y)
{
	d->origin(flatpoint(x,y));
	datastack.push(d);
	int c=datastack.n-1;
	curobj->i=c;
	curobj->SetObject(d);
	return c;
}

//! For brand new data created by interfaces. Pushes data onto top of stack.
/*! Returns the index in the current context of the data after instertion, or -1 for
 * not insterted.
 */
int ViewportWithStack::NewData(SomeData *d,ObjectContext **oc_ret)
{
	if (!d) return -1;
	int c;

	curobj->SetObject(d);
	datastack.push(d);
	c=datastack.n-1; 
	curobj->i=c;

	if (oc_ret) *oc_ret=curobj;
	
	return curobj->i;
}

//! Delete the current object by decrementing its count and removing it from datastack.
/*! Also clears any interface in the stack whose data is 
 * is curobj->obj.
 *
 * Return 1 if object deleted, 0 if not deleted, -1 if there was no current object.
 */
int ViewportWithStack::DeleteObject()
{
	SomeData *todel=curobj->obj;
	if (!todel) return -1;
	for (int c=0; c<interfaces.n; c++) interfaces.e[c]->Clear(todel);
	datastack.remove(datastack.findindex(todel));
	curobj->clear();
	
	//ClearSearch();
	needtodraw=1;
	return 1;
}

//! Simple, default refreshing just tries to draw all items in datastack.
/*! If win_parent can be cast to ViewerWindow, then Refresh tries to find the 
 * appropriate interface in viewerwindow->tools for each item of datastack.
 *
 * If vpwsfirsttime!=0 and win_parent is a ViewerWindow, then try to push on a ColorBox.
 */
void ViewportWithStack::Refresh()
{
	if (vpwsfirsttime) {
		vpwsfirsttime=0;
		ViewerWindow *viewer=dynamic_cast<ViewerWindow *>(win_parent); // maybe not always returns non-null
		if (viewer) {
			ColorBox *colorbox;
			colorbox=new ColorBox(this,"colorbox",NULL,0, 0,0,0,0,1, NULL,object_id,"vpwsColor",LAX_COLOR_RGB,
					1./255, 1.,0.,0.,1.);
			viewer->AddWin(colorbox,1, 50,0,50,50,0, 10,0,50,50,0, -1);
			viewer->Sync(1);
		}
	}

	DBG cerr <<"ViewportWithStack Trying to startdrawing"<<getUniqueNumber()<<endl;

	dp->StartDrawing(this);
	//dp->MakeCurrent(this);

	dp->ClearWindow();
	int c;
	for (c=0; c<interfaces.n; c++) interfaces.e[c]->needtodraw=1;//force refresh all whenever viewport is refreshing
	
	 //draw sample square 200x200
	dp->LineAttributes(1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
	dp->NewFG(.5,.5,.5);
	dp->drawline(-100,-100, 100,-100);
	dp->drawline( 100,-100, 100, 100);
	dp->drawline( 100, 100,-100, 100);
	dp->drawline(-100, 100,-100,-100);

	dp->drawaxes(10);

	
	if (needtodraw) {
		int c2;
		anInterface *ifc=NULL;
		ViewerWindow *viewer=dynamic_cast<ViewerWindow *>(win_parent);
		
		 // ViewportWithStack does not know about all possible ways and things to draw,
		 // so first looks in viewer->tools, then interfaces...
		for (c=0; c<datastack.n; c++) {
			ifc=NULL;
			 // find printer for e[c].data
			if (viewer) {
				for (c2=0; c2<viewer->tools_n(); c2++) {
					if (viewer->tools_e(c2)->draws(datastack.e[c]->whattype())) {
						ifc=viewer->tools_e(c2);
						break;
					}
				}
			}
			if (!ifc) { // look in interfaces if interface is not in viewer->tools
				for (c2=0; c2<interfaces.n; c2++) {
					if (interfaces.e[c2]->draws(datastack.e[c]->whattype())) {
						//cout <<"---DrawData:"<<datastack.e[c]->whattype()<<endl;
						ifc=interfaces.e[c2];
						break;
					}
				}
			}
			if (ifc) {
				dp->PushAndNewTransform(datastack.e[c]->m());
				dp->drawaxes(10);

				DBG cerr <<"...drawing object "<<datastack.e[c]->object_id<<" ("<<datastack.e[c]->whattype()<<")"<<endl;
				ifc->DrawData(datastack.e[c]);
				dp->PopAxes();
			}
		}

		 // Refresh interfaces, should draw whatever SomeData they have
		ObjectContext *oc;
		SomeData *dd;
		dp->DrawReal();
		for (c=0; c<interfaces.n; c++) {
			if (interfaces.e[c]->Needtodraw()) {
				//cout <<" \ndrawing "<<c;
				oc=interfaces.e[c]->Context();
				dd=oc?oc->obj:NULL;
				if (oc) {
					double m[6];
					transformToContext(m,oc,0,1);
					dp->PushAndNewTransform(m);
				}
				if (dd) {
					 //draw bounding box
					dp->LineAttributes(1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
					dp->BlendMode(LAXOP_Over);
					dp->NewFG(128,128,128);
					dp->drawline(flatpoint(dd->minx,dd->miny),flatpoint(dd->maxx,dd->miny));
					dp->drawline(flatpoint(dd->maxx,dd->miny),flatpoint(dd->maxx,dd->maxy));
					dp->drawline(flatpoint(dd->maxx,dd->maxy),flatpoint(dd->minx,dd->maxy));
					dp->drawline(flatpoint(dd->minx,dd->maxy),flatpoint(dd->minx,dd->miny));
				}
				interfaces.e[c]->Refresh();;
				if (oc) dp->PopAxes();
			}
		}

		 // draw real axes length 10
		//dp->drawaxes(10);
		
	} // if needtodraw

	//DBG cerr <<"ctm: "; dumpctm(dp->Getctm());
	//DBG cerr <<"ictm: "; dumpctm(dp->Getictm());

	//flatpoint p=dp->screentoreal(lastm);
	//p=dp->realtoscreen(lastm);
	//dp->drawpoint(p,10,0);
	//dp->drawpoint(lastm,10,0);


	dp->EndDrawing();
	SwapBuffers();
	
	//DBG cerr <<" All done drawing.\n";
	needtodraw=0;
}

int ViewportWithStack::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	lastm.x=x;
	lastm.y=y;
	return ViewportWindow::MouseMove(x,y,state,d);
}

double *ViewportWithStack::transformToContext(double *m,ObjectContext *oc,int invert,int full)
{
	if (!m) m=new double[6];
	transform_identity(m);
	if (!full) return m;
	
	if (oc->obj) {
		if (invert) transform_invert(m,oc->obj->m());
		else transform_copy(m,oc->obj->m());
	}

	return m;
}

/*! Catches a "vpwsColor" from a ColorBox. See Refresh().
 *  
 *  \todo *** should respond to an updateCurcolor, which interfaces can send when
 *  a new point is selected, for instance.
 */
int ViewportWithStack::Event(const EventData *e,const char *mes)
{
	if (!strcmp(mes,"vpwsColor")) {
		 // apply message as new current color, pass on to viewport
		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(e);
		if (!ce) return 0;

		LineStyle linestyle;
		double max=ce->max;
		if (ce->colortype==LAX_COLOR_RGB) {
			linestyle.color.red  =ce->channels[0]/max*0xffff;
			linestyle.color.green=ce->channels[1]/max*0xffff;
			linestyle.color.blue =ce->channels[2]/max*0xffff;
			linestyle.color.alpha=ce->channels[3]/max*0xffff;
		} else {
			cerr << " *** must implement cmyk and gray color event receiving in ViewportWithStack::Event"<<endl;
		}

//		ViewerWindow *viewer=dynamic_cast<ViewerWindow *>(win_parent); // maybe not always returns non-null
//		if (viewer) {
//			if (viewer->curtool) {
//				if (viewer->curtool->UseThis(&linestyle,GCForeground)) needtodraw=1;
//			}
//		} else {
			for (int c=0; c<interfaces.n; c++) interfaces.e[c]->UseThis(&linestyle,GCForeground);
			needtodraw=1;
//		}
		return 0;

	} else if (e->type==LAX_ColorEvent) {
		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(e);
		if (!ce) return 0;

		ColorBox *colorbox=dynamic_cast<ColorBox*>(findChildWindowByName("colorbox"));
		if (!colorbox) return 0;

		if (ce->colortype==LAX_COLOR_GRAY)
			colorbox->SetGray(ce->channels[0]*255/ce->max,ce->channels[1]*255/ce->max);
		if (ce->colortype==LAX_COLOR_RGB)
			colorbox->SetRGB(ce->channels[0]*255/ce->max,
							 ce->channels[1]*255/ce->max,
						  	 ce->channels[2]*255/ce->max,
						 	 ce->channels[3]*255/ce->max);
		if (ce->colortype==LAX_COLOR_CMYK)
			colorbox->SetCMYK(ce->channels[0]*255/ce->max,
							  ce->channels[1]*255/ce->max,
							  ce->channels[2]*255/ce->max,
							  ce->channels[3]*255/ce->max,
							  ce->channels[4]*255/ce->max);
		return 0;

	}
	return ViewportWindow::Event(e,mes);
}


} // namespace LaxInterfaces

