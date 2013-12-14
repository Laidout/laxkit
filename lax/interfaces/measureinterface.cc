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
//    Copyright (C) 2009 by Tom Lechner
//




#include <lax/interfaces/somedatafactory.h>
#include <lax/language.h>
#include "measureinterface.h"

#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxFiles;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {




////--------------------------------- MeasureInterface ----------------------------


/*! \class MeasureInterface
 * \ingroup interfaces
 * \brief Measure length, angle and area in various ways.
 *
 * Adjustable units: correspond to screen (inches or pixels), viewport, viewport+other transform,
 * or any of those plus a correction factor, so you can have actual units be inches, for instance,
 * but measure distances using 1 lightyear == .5 inches.
 *
 * Left-click and drag with no current produces a line. Shows on screen the measurment.
 * With current line, hover over endpoint, click drag to produce an attached line, measures both lines
 * and angles. With more than one line, dragging a line to the initial point will close the line,
 * producing an area measurement.
 *
 * Modes: 
 *  1. Measure line segments by default
 *  2. Measure angles not distances, assumes at least 2 line segments.
 *  3. Measure area, assumes free form line dragged out, close path automatically
 *
 */

#define MODE_Distances 1
#define MODE_Angles    2
#define MODE_Area      4

#define TARGET_Screen   1
#define TARGET_Viewport 2
#define TARGET_Extra    3

/*! Sets target to viewport.
 */
MeasureInterface::MeasureInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	controlcolor=app->rgbcolor(0,148,178); 
	linestyle.color.red=0;
	linestyle.color.green=148<<8;
	linestyle.color.blue=178<<8;
	
	showdecs=3;
	curpoint=points=NULL;
	mode=MODE_Distances|MODE_Angles|MODE_Area;
	target=TARGET_Viewport;

	buttondown=0;
	style=0;

	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;

	needtodraw=1;
}

//! Empty destructor.
MeasureInterface::~MeasureInterface() 
{
	DBG cerr <<"----in MeasureInterface destructor"<<endl;
}

//! Return dup of this. Copies over creationstyle, creatp/v, createlen, col1, col2.
anInterface *MeasureInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new MeasureInterface(id,NULL);
	else if (!dynamic_cast<MeasureInterface *>(dup)) return NULL;
	MeasureInterface *dupp=dynamic_cast<MeasureInterface *>(dup);
	dupp->target=target;
	dupp->mode=mode;
	transform_copy(dupp->extra,extra);
	return anInterface::duplicate(dup);
}

//! Sets showdecs=1, and needtodraw=1.
int MeasureInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

//! Calls Clear(), sets showdecs=0, and needtodraw=1.
int MeasureInterface::InterfaceOff()
{
	deletedata();
	showdecs=0;
	needtodraw=1;
	return 0;
}

//! Delete points.
void MeasureInterface::deletedata()
{
	if (points) { delete points; points=NULL; }
	curpoint=NULL;
}

//! Standard drawdata function, but do nothing since there is no persistent data.
int MeasureInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
	return 1;
}

int MeasureInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!points) {
		if (needtodraw) needtodraw=0;
		return 1;
	}

	dp->LineAttributes(linestyle.width,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);

	Coordinate *p;

	 // draw distances
	if (mode&MODE_Distances) {
		***draw line with ruler ticks, plus text for the distance
	}

	 // draw points
	p=points;
	do {
		if (p==curpoint) ***draw bigger circle for it;
		else ***draw normal circle for it;
	} while (p && p!=points);
	
	 // draw angles
	if (mode&MODE_Angles) {
		*** //draw arc to show what's measured, plus text for the angle
	}

	 // draw area
	if (mode&MODE_Area) {
		//***shade in area, display text for the area
	}

	needtodraw=0;
	return 0;
}

//! Scan for point.
Coordinate *MeasureInterface::scan(int x,int y) 
{
	if (!points) return NULL;

	 // picks the first point within range
	flatpoint p,p2;
	p=screentoreal(x,y); //<-remember this is not including data's transform
	double d=5/Getmag(), //d eventually is (5 pixels in gradient space)^2
		   dd;
	d*=d;
	DBG cerr <<" gd scan d="<<sqrt(d)<<"(x,y)="<<p.x<<','<<p.y<<endl;

	for (Coordinate *pp=points; pp; pp=pp->next) { 
		p2=pp->p();
		dd=(p2.x-p.x)*(p2.x-p.x)+(p2.y-p.y)*(p2.y-p.y);
		if (dd<d) return p;
	}

	return NULL;
}

//! Creates new data if not click down on one...
/*! Shift-click makes a new color spot.
 *
 * \todo *** shift-click near an already selected but perhaps obscured point should not select
 * the point that is obscuring it instead
 */
int MeasureInterface::LBDown(int x,int y,unsigned int state,int count)
{
	mx=x;
	my=y;
	buttondown|=LEFTBUTTON;
	 // straight click
	Coordinate *p=scan(x,y);
	if (!p) {
		 //delete any old data, and create a new line segment with 0 length..
		deletedata();
		points=new Coordinate(x,y);
		curpoint=new Coordinate(x,y);
		points.connect(curpoint,1);
		return 0;
	}

	//a point was clicked on, so make that curpoint!
	curpoint=p;
	return 0;
}

//! If data, then call viewport->ObjectMoved(data).
int MeasureInterface::LBUp(int x,int y,unsigned int state) 
{
	if (!(buttondown&LEFTBUTTON)) return 1;
	buttondown&=~LEFTBUTTON;
	return 0;
}


int MeasureInterface::MouseMove(int x,int y,unsigned int state) 
{***
	DBG cerr <<"--------------gradient point scan:"<<scan(x,y)<<endl;
	if (!data) { return 1;}
	if (!(buttondown&LEFTBUTTON)) {
		int c=scan(x,y);
		if (c!=curpoint) { curpoint=c; needtodraw=1; }
		return 1;
	}
	
	flatpoint d=screentoreal(x,y) - screentoreal(mx,my);
	//flatpoint op=screentoreal(mx,my) - data->p;
	//flatpoint np=op + d;

	if (curpoint<GP_MinMoveable && curpoint!=GP_a && curpoints.n==0) {
		if (state&ControlMask && state&ShiftMask) { // +^ rotate
			 // rotate around p of gradient based on x movement
			double a;
			a=(x-mx)/180.0*3.1415926535;
			flatpoint p=transform_point(data->m(),leftp);
			data->xaxis(rotate(data->xaxis(),a,0));
			data->yaxis(rotate(data->yaxis(),a,0));
			data->origin(data->origin()+p-transform_point(data->m(),leftp));
		//} else if (state&ControlMask && !(data->style&GRADIENT_RADIAL)) { // ^ scale w
		} else if (state&ControlMask) { // ^ scale w
			double dd=double(x-mx);
			dd=1+.02*dd;
			if (dd<0.1) dd=0.1;
			flatpoint p=transform_point(data->m(),leftp);
			data->xaxis(dd*data->xaxis());
			data->yaxis(dd*data->yaxis());
			data->origin(data->origin()+p-transform_point(data->m(),leftp));
		} else { // move whole gradient
			data->origin(data->origin()+d);
		}
		needtodraw=1;
		mx=x; my=y;
		return 0;
	}
	
	 // move curpoints 
	int movepoint=curpoint;
	if (movepoint==GP_r1 && (data->style&GRADIENT_RADIAL) && (state&LAX_STATE_MASK)==ShiftMask)
		movepoint=GP_p1;
	else if (movepoint==GP_r2 && (data->style&GRADIENT_RADIAL) && (state&LAX_STATE_MASK)==ShiftMask)
		movepoint=GP_p2;
	else if (movepoint==0 && curpoints.n==1 
			  && (state&LAX_STATE_MASK)!=ShiftMask
			  && draggingmode!=DRAG_FROM_INSIDE) { movepoint=GP_p1; state^=ShiftMask; }
	else if (movepoint==data->colors.n-1 && curpoints.n==1 
			  && (state&LAX_STATE_MASK)!=ShiftMask
			  && draggingmode!=DRAG_FROM_INSIDE) 
		{ movepoint=GP_p2; state^=ShiftMask; }
	//else if (movepoint==0 && (data->style&GRADIENT_RADIAL)) movepoint=GP_p1;
	//else if (movepoint==data->colors.n-1 && (data->style&GRADIENT_RADIAL)) movepoint=GP_p2;

	 //second round remapping
	if (movepoint==GP_p1 && (data->style&GRADIENT_RADIAL) && (state&LAX_STATE_MASK)==ControlMask) movepoint=GP_r1;
	else if (movepoint==GP_p2 && (data->style&GRADIENT_RADIAL) && (state&LAX_STATE_MASK)==ControlMask) movepoint=GP_r2;
	
	
	double m[6];
	transform_invert(m,data->m());
	d=transform_vector(m,d);
	if (movepoint==GP_a) { 
		DBG cerr <<"--- move grad point a"<<endl;
		if (data->style&GRADIENT_RADIAL) data->a+=d.x/180*M_PI;
		else data->a+=d.y;
	} else if (movepoint==GP_r1) { 
		if (data->style&GRADIENT_RADIAL) {
			flatpoint r1=d+transform_point_inverse(data->m(),screentoreal(mx,my))-getpoint(GP_p1,0);
			data->r1=norm(r1);
			if (fabs((data->p2+fabs(data->r2))-(data->p1+fabs(data->r1)))
					<fabs((data->p2-fabs(data->r2))-(data->p1-fabs(data->r1)))) {
				data->a=M_PI;
			} else data->a=0;
		} else {
			data->r1+=d.y;
			if ((state&LAX_STATE_MASK)!=ShiftMask) data->r2+=d.y;
		}
		needtodraw=1;
	} else if (movepoint==GP_r2) { 
		if (data->style&GRADIENT_RADIAL) {
			flatpoint r2=d+transform_point_inverse(data->m(),screentoreal(mx,my))-getpoint(GP_p2,0);
			data->r2=norm(r2);
			if (fabs((data->p2+fabs(data->r2))-(data->p1+fabs(data->r1)))
					<fabs((data->p2-fabs(data->r2))-(data->p1-fabs(data->r1)))) {
				data->a=M_PI;
			} else data->a=0;
		} else {
			data->r2-=d.y;
			if ((state&LAX_STATE_MASK)!=ShiftMask) data->r1-=d.y;
		}
		needtodraw=1;
	} else if (movepoint==GP_p2 || movepoint==GP_p1) { 
		if ((state&LAX_STATE_MASK)==ShiftMask && draggingmode==DRAG_NORMAL
			 || (state&LAX_STATE_MASK)==0 && draggingmode==DRAG_NEW) {
			 //rotate as well as shift p1 or p2
			d=screentoreal(x,y) - screentoreal(mx,my);
			flatpoint ip, //invariant point
					  oldp1=getpoint(GP_p1,1),
			          oldp2=getpoint(GP_p2,1),
					  newp2=oldp2+d,
					  op,np;
			if (movepoint==GP_p2) ip=oldp1; else ip=oldp2;
			op=oldp2-oldp1;
			np=newp2-oldp1;
			if ((op*op)*(np*np)!=0) {
				double a=asin((op.x*np.y-op.y*np.x)/sqrt((op*op)*(np*np)));
				if (movepoint==GP_p1) a=-a;
				data->xaxis(rotate(data->xaxis(),a,0));
				data->yaxis(rotate(data->yaxis(),a,0));
			}
			 //sync up the invariant point
			if (movepoint==GP_p1) ip=getpoint(GP_p2,1)-ip;
				else ip=getpoint(GP_p1,1)-ip;
			data->origin(data->origin()-ip);

			 //remap d
			d=transform_vector(m,d||np);
		}
		if (movepoint==GP_p1) data->p1+=d.x;
		else data->p2+=d.x;
		if (data->style&GRADIENT_RADIAL) {
			if (fabs((data->p2+fabs(data->r2))-(data->p1+fabs(data->r1)))
					<fabs((data->p2-fabs(data->r2))-(data->p1-fabs(data->r1)))) {
				data->a=M_PI;
			} else data->a=0;
		}
		needtodraw=1;
	} else {
		 // move curpoints
		int cp;
		double plen,clen,cstart;
		plen=data->p2-data->p1;
		cstart=data->colors.e[0]->t;
		clen=data->colors.e[data->colors.n-1]->t - cstart;
		if (plen && curpoints.n) {
			for (int c=0; c<curpoints.n; c++) {
				//cout <<"move point "<<curpoints.e[c]<<"  by d.x="<<d.x<<"  d.x/clen="<<d.x/clen<<endl;
				cp=curpoints.e[c];
				
				DBG if (cp==0) {
				DBG 	cerr <<"*** mv grad point 0"<<endl;
				DBG } else if (cp==data->colors.n-1) {
				DBG 	cerr <<"*** mv grad point n-1"<<endl;
				DBG }

				 //d is in p space, but t shifts in t space
				curpoints.e[c]=data->ShiftPoint(curpoints.e[c],d.x/plen*clen);
				DBG cerr <<"curpoint["<<c<<"] now is: "<<curpoints.e[c]<<endl;

				 // the shifting reorders the spots, and messes up curpoints so this corrects that
				 //****FIX! though if the shift skips more than one it doesn't!
				if (cp!=curpoints.e[c]) {
					if (cp==curpoint) curpoint=curpoints.e[c];
					for (int c2=0; c2<curpoints.n; c2++) {
						if (c2==c) continue;
						if (cp<curpoints.e[c]) {//remember cp is the old value of the shifted point
							if (curpoints.e[c2]>cp && curpoints.e[c2]<=curpoints.e[c]) 
								curpoints.e[c2]--;
						} else {
							if (curpoints.e[c2]>=curpoints.e[c] && curpoints.e[c2]<cp)
								curpoints.e[c2]++;
						}
					}
				}
			}
			 // check if points were shifted past previous p1 and p2 points
			 // and change p1 and/or p2 if so
			double nclen=data->colors.e[data->colors.n-1]->t-data->colors.e[0]->t;
			if (data->colors.e[0]->t!=cstart || nclen!=clen) {
				double m=plen/clen;
				data->p1+=(data->colors.e[0]->t-cstart)*m;
				data->p2+=(data->colors.e[0]->t+nclen - (cstart+clen))*m;
			}
		}
	}
	data->FindBBox();
	mx=x; my=y;
	needtodraw|=2;
	return 0;
}

int MeasureInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state) 
{
	if (ch==LAX_Esc) {
		deletedata();
		needtodraw=1;
		return 0;
	}
	return 1; 
}


} // namespace LaxInterfaces

