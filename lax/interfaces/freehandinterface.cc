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
//    Copyright (C) 2004-2006,2011,2014 by Tom Lechner
//



#include <lax/interfaces/freehandinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class FreehandInterface
 * \ingroup interfaces
 * \brief Interface to create bezier or straight lines from freehand motion.
 *
 *   On mouse down, this records all mouse movement, and converts
 *   the points into a bez curve (linear or cubic) on mouse up.
 *
 * \todo ***Currently, this sucks. Should figure out how Sodipodi/inkscape does it.
 *  
 * \todo *** make closed when final point is close to first point
 */


FreehandInterface::FreehandInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	freehand_style=0;

	linecolor .rgbf(0,0,.5);
	pointcolor.rgbf(.5,.5,.5);

	linestyle.color.red  =linestyle.color.alpha=0xffff;
	linestyle.color.green=linestyle.color.blue =0;

	mindist=40;

	showdecs=1;
	needtodraw=1;
}

FreehandInterface::~FreehandInterface()
{
}

const char *FreehandInterface::Name()
{ return _("Freehand Lines"); }


//! Return new FreehandInterface.
/*! If dup!=NULL and it cannot be cast to ImageInterface, then return NULL.
 */
anInterface *FreehandInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new FreehandInterface(NULL,id,NULL);
	else if (!dynamic_cast<FreehandInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

int FreehandInterface::UseThis(anObject *nobj, unsigned int mask)
{ // ***
	return 1;
//	if (!nobj) return 1;
//	LineStyle *ls;
//	if (ls=dynamic_cast<LineStyle *>(nobj), ls!=NULL) {
//		if (mask&GCForeground) { 
//			if (data) data->linestyle.color=ls->color;
//			else linestyle.color=ls->color;
//		}
//		if (mask&GCLineWidth) {
//			if (data) data->linestyle.width=ls->width;
//			else linestyle.width=ls->width;
//		}
//		needtodraw=1;
//		return 1;
//	}
//	return 0;
}

int FreehandInterface::InterfaceOn()
{ 
	showdecs=1;
	needtodraw=1;
	return 0;
}

int FreehandInterface::InterfaceOff()
{ 
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void FreehandInterface::Clear(SomeData *d)
{
	lines.flush();
	deviceids.flush();
}

/*! \todo *** this is really cheezy, as it just copies bez->Refresh()
 */
int FreehandInterface::Refresh()
{ 
	//DBG cerr <<"  Freehand trying to startdrawing"<<endl;

	if (needtodraw==0) return 0;
	needtodraw=0;

	if (lines.n==0) return 0;
	
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

	RawPointLine *line;
	for (int c=0; c<lines.n; c++) {
		line=lines.e[c];
		if (line->n==0) continue;

		 // draw curve
		dp->NewFG(&linecolor);
		for (int c2=0; c2<line->n; c2++) {
			if (c2==0) dp->moveto(line->e[c2]->p);
			else dp->lineto(line->e[c2]->p);
		}
		dp->stroke(0);

		 // draw control points
		if (showdecs) {
			dp->NewFG(&pointcolor);
			 // draw little circles
			for (int c2=0; c2<line->n; c2++) {
				dp->drawpoint(line->e[c2]->p,3,1);
			}
		}
	}

	//DBG cerr <<"   Freehand all done drawing.\n";
	return 0;
}

int FreehandInterface::findLine(int id)
{
	for (int c=0; c<deviceids.n; c++) if (deviceids[c]==id) return c;
	return -1;
}

//! Start a new freehand line.
int FreehandInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"freehandlbd:"<<x<<','<<y<<"..";
	buttondown.down(d->id,LEFTBUTTON,x,y);

	int i=findLine(d->id);
	if (i>=0) {
		lines.remove(i);
		deviceids.remove(i);
	}

	RawPointLine *line=new RawPointLine;
	flatpoint p=dp->screentoreal(x,y);
	line->push(new RawPoint(p));
	lines.push(line);
	deviceids.push(d->id);
	
	DBG cerr <<"../freehandiLbd\n";
	return 0;
}

int FreehandInterface::send(int i)
{
	if (i<0 || i>=lines.n) return 1;

	RawPointLine *line=lines.e[i];
	
	PathsData *paths=new PathsData;
	for (int c=0; c<line->n; c++) {
		paths->append(line->e[c]->p);
	}

	if (owner) {
		RefCountedEventData *data=new RefCountedEventData(paths);
		app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

	} else {
		if (viewport) viewport->NewData(paths,NULL);
	}

	paths->dec_count();
	
	return 0;
}

//! Finish a new freehand line by calling newData with it.
int FreehandInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"  freehandLbup..";

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	int i=findLine(d->id);
	if (i<0) return 0; //line missing! do nothing

	DBG cerr <<"  *** FreehandInterface should check for closed path???"<<endl;

	if (!dragged && lines.e[i]->n>1) {
		send(i);
	}

	deviceids.remove(i);
	lines.remove(i);

	needtodraw=1;
	return 0;
}

/*! \todo *** this isn't very sophisticated, for elegance, should use some kind of 
 * bez curve fitting to cut down on unnecessary points should use a timer so 
 * stopping makes sharp corners and closer spaced points?
 */
int FreehandInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) return 1;

	int i=findLine(d->id);
	if (i<0) return 0;

	int mx=0,my=0;
	buttondown.move(d->id, x,y, &mx,&my);

	//if ((x-mx)*(x-mx)+(y-my)*(y-my)<mindist*mindist) return 1;
	flatpoint p=dp->screentoreal(x,y);
	RawPointLine *line=lines.e[i];
	line->push(new RawPoint(p)); // *** need to extract pressure and time

//	if (data->npoints>2*3) {
//		int n=data->npoints;
//		p=data->points[n-2]-data->points[n-8];
//		data->points[n-6]+=((data->points[n-8]-data->points[n-5])||p)/2;
//		data->points[n-4]+=((data->points[n-2]-data->points[n-5])||p)/2;
//	}

	needtodraw=1;
	return 0;
}

} // namespace LaxInterfaces

