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
	//freehand_style=FREEHAND_Poly_Path;
	freehand_style=FREEHAND_Bez_Outline;

	linecolor .rgbf(0,0,.5);
	pointcolor.rgbf(.5,.5,.5);

	linestyle.color.red  =linestyle.color.alpha=0xffff;
	linestyle.color.green=linestyle.color.blue =0;

	mindist=40;
	brush_size=60;

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

		 //draw pressure indicator
		dp->NewFG(1.,0.,1.,1.);
		flatvector vt;
		dp->moveto(line->e[0]->p);
		for (int c2=1; c2<line->n-1; c2++) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;
			vt=line->e[c2+1]->p - line->e[c2-1]->p;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
			//dp->drawline(line->e[c2]->p + vt, line->e[c2]->p - vt);
			dp->lineto(line->e[c2]->p + vt);
		}
		dp->stroke(0);

		dp->moveto(line->e[0]->p);
		for (int c2=1; c2<line->n-1; c2++) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;
			vt=line->e[c2+1]->p - line->e[c2-1]->p;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
			//dp->drawline(line->e[c2]->p + vt, line->e[c2]->p - vt);
			dp->lineto(line->e[c2]->p - vt);
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

//! Finish a new freehand line by calling newData with it.
int FreehandInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"  freehandLbup..";

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	int i=findLine(d->id);
	if (i<0) return 0; //line missing! do nothing

	DBG cerr <<"  *** FreehandInterface should check for closed path???"<<endl;

	if (dragged && lines.e[i]->n>1) {
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

	RawPoint *pp=new RawPoint(p);
	pp->time=times(NULL);
	const_cast<LaxMouse*>(d)->getInfo(NULL,NULL,NULL,NULL,NULL,NULL,&pp->pressure,&pp->tiltx,&pp->tilty);
	if (pp->pressure<0 || pp->pressure>1) pp->pressure=1; //non-pressure sensitive map to full pressure
	line->push(pp);

//	if (data->npoints>2*3) {
//		int n=data->npoints;
//		p=data->points[n-2]-data->points[n-8];
//		data->points[n-6]+=((data->points[n-8]-data->points[n-5])||p)/2;
//		data->points[n-4]+=((data->points[n-2]-data->points[n-5])||p)/2;
//	}

	needtodraw=1;
	return 0;
}


/*! Returns a simplified line based on lines.e[i].
 *
 * This implements the Ramer–Douglas–Peucker algorithm for reducing the number of points
 * in a polyline based on being within a given threshhold distance to a base line.
 * Start with the end points, l is the segment between them. Find the point between the endpoints
 * that is the farthest from l. If that point p is < e (the threshhold distance), then all
 * points are assumed to belong to l. Otherwise, keep p, and recursively call with new line segments
 * [start,p] and [p,end].
 */
RawPointLine *FreehandInterface::Reduce(int i, double epsilon)
{
	if (i<0 || i>=lines.n) return NULL;

	RawPointLine *l_orig=lines.e[i];
	for (int c=0; c<l_orig->n; c++) l_orig->e[c]->flag=0;
	RecurseReduce(l_orig, 0,l_orig->n-1, epsilon);

	RawPointLine *l=new RawPointLine;
	RawPoint *p;
	for (int c=0; c<l_orig->n; c++) {
		p=new RawPoint;
		*p=*l_orig->e[c];
		if (l_orig->e[c]->flag!=0) l->push(p);
	}

	return l;
}

void FreehandInterface::RecurseReduce(RawPointLine *l, int start, int end, double epsilon)
{
	if (end<=start+1) return; 

	flatvector v=l->e[end]->p - l->e[start]->p;
	flatvector vt=transpose(v);
	vt.normalize();

	l->e[start]->flag=1;
	l->e[end  ]->flag=1;

	int i=-1;
	double d=0, dd;
	for (int c=start+1; c<end; c++) {
		dd=fabs((l->e[c]->p - l->e[start]->p)*vt);
		if (dd>d) { d=dd; i=c; }
	}

	if (d<epsilon) {
		;
		//for (int c=start+1; c<end; c++) l->e[c]->flag=0;
	} else {
		RecurseReduce(l, start,i, epsilon);
		RecurseReduce(l, i,end,   epsilon);
	}
}

///*! Makes fauxpoints be a bezier list: c-p-c-c-p-c-...-c-p-c
// */
//Coordinate *FreehandInterface:BezApproximate(RawPointLine *l)
//{
//	// There are surely better ways to do this. Not sure how powerstroke does it.
//	// It is not simplied/optimized at all. Each point gets control points to smooth it out.
//
//	Coordinate *coord=NULL;
//
//    flatvector v,p, pp,pn;
//	flatvector opn, opp;
//
//
//    double sx;
//	//caps are at points index 0 and points.n/2
//	
//    for (int c=0; c<points.n; c++) {
//        p=points.e[c];
//
//		if (c==0) {
//			***
//
//		} else if (c==points.n-1) {
//			***
//
//		} else {
//			opp=points.e[c-1];
//			opn=points.e[c+1];
//		}
//
//        v=opn-opp;
//        v.normalize();
//
//        sx=norm(p-opp)*.333;
//        pp=p - v*sx;
//
//        sx=norm(opn-p)*.333;
//        pn=p + v*sx;
//
//        fauxpoints.push(pp);
//        fauxpoints.push(p);
//        fauxpoints.push(pn);
//
//    }
//}

int FreehandInterface::send(int i)
{
	if (i<0 || i>=lines.n) return 1;

	if (freehand_style&FREEHAND_Raw_Path) {
		RawPointLine *line=lines.e[i];
	
		PathsData *paths=new PathsData;
		for (int c=0; c<line->n; c++) {
			paths->append(line->e[c]->p);
		}
		paths->FindBBox();

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(paths);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(paths,NULL);
		}

		paths->dec_count();
	}
	

	if (freehand_style&FREEHAND_Poly_Path) {
		 //return a reduced polyline
		RawPointLine *line=Reduce(i,2/dp->Getmag());

		PathsData *paths=new PathsData;
		for (int c=0; c<line->n; c++) {
			paths->append(line->e[c]->p);
		}
		paths->FindBBox();

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(paths);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(paths,NULL);
		}

		paths->dec_count();
		delete line;
	}


//	if (freehand_style&FREEHAND_Bez_Path) {
//		 //return a bezierified line, based on a reduced polyline
//		RawPointLine *line=Reduce(i,2/dp->Getmag());
//		Coordinate *coord=BezApproximate(line);
//
//		paths=new PathsData;
//		paths.append(coord);
//
//		if (owner) {
//			RefCountedEventData *data=new RefCountedEventData(paths);
//			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);
//
//		} else {
//			if (viewport) viewport->NewData(paths,NULL);
//		}
//
//		paths->dec_count();
//		delete line;
//	}


	if (freehand_style&FREEHAND_Bez_Outline) {
		RawPointLine *line=Reduce(i,2/dp->Getmag());

		PathsData *paths=new PathsData;

		flatvector vt, pp,pn;
		for (int c2=0; c2<line->n; c2++) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;

			if (c2==0) pp=line->e[c2]->p; else pp=line->e[c2-1]->p;
			if (c2==line->n-1) pn=line->e[c2]->p; else pn=line->e[c2+1]->p;

			vt=pn-pp;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
			paths->append(line->e[c2]->p + vt);
		}

		for (int c2=line->n-1; c2>=0; c2--) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;

			if (c2==0) pp=line->e[c2]->p; else pp=line->e[c2-1]->p;
			if (c2==line->n-1) pn=line->e[c2]->p; else pn=line->e[c2+1]->p;

			vt=pn-pp;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
			paths->append(line->e[c2]->p - vt);
		}
		paths->close();
		paths->FindBBox();

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(paths);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(paths,NULL);
		}

		paths->dec_count();
		delete line;
	}

//	if (freehand_style&FREEHAND_Mesh) {
//		 //return a mesh based on a bezierified line, which is based on a reduced polyline
//		 ***
//	}


	return 0;
}

} // namespace LaxInterfaces

