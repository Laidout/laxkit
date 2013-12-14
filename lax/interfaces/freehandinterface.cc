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
//    Copyright (C) 2004-2006,2011 by Tom Lechner
//



#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/freehandinterface.h>
#include <lax/interfaces/bezinterface.h>
#include <lax/interfaces/somedata.h>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {

//----- BezData listed here only as reference, it is defined in bezinterface.cc:
//#define BEZS_CLOSED	1
//#define BEZS_STIFF_EQUAL	0
//#define BEZS_STIFF_NEQUAL	1
//#define BEZS_NSTIFF_EQUAL	2
//#define BEZS_NSTIFF_NEQUAL	3
//
//class BezData : public anObject
//{
//  public:
//	int style;
//	int npoints; // is all points, not just vertices
//	flatpoint *points;
//	int *pointstyles; // length is n/3+1
//	BezData();
//	BezData(int cstyle);
//	~BezData();
//	int AddPoint(int where,flatpoint c1,flatpoint v,flatpoint c2,int pointstyle);
//	int DeletePoint(int index);
//	const char *whattype() { return "BezData"; }
//};

//----------------------------------------------------------------

/*! \class FreehandInterface
 * \ingroup interfaces
 * \brief Interface to create bezier lines from freehand motion.
 *
 *   On mouse down, this records all mouse movement, and converts
 *   the points into a bez curve (linear/quad/cubic) on mouse up.
 *
 * \todo ***Currently, this sucks. Should figure out how Sodipodi/inkscape does it.
 *  
 * \todo ***should be able to add to a current BezData??
 *   ***make choice beteen make LinesData or BezData (or Path?)
 *   *** make closed when final point==first point
 */


FreehandInterface::FreehandInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	if (app) deconcolor=app->rgbcolor(128,128,128);
	else deconcolor=38066;
	decoffcolor=~0;
	linestyle.color.red  =linestyle.color.alpha=0xffff;
	linestyle.color.green=linestyle.color.blue =0;

	data=NULL;
	mindist=40;
	
	showdecs=1;
	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
	buttondown=0;
	needtodraw=1;
}

FreehandInterface::~FreehandInterface()
{
	if (data) data->dec_count();
	data=NULL;
}

//! Return new FreehandInterface.
/*! If dup!=NULL and it cannot be cast to ImageInterface, then return NULL.
 */
anInterface *FreehandInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new FreehandInterface(id,NULL);
	else if (!dynamic_cast<FreehandInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

int FreehandInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;
	LineStyle *ls;
	if (ls=dynamic_cast<LineStyle *>(nobj), ls!=NULL) {
		if (mask&GCForeground) { 
			if (data) data->linestyle.color=ls->color;
			else linestyle.color=ls->color;
		}
		if (mask&GCLineWidth) {
			if (data) data->linestyle.width=ls->width;
			else linestyle.width=ls->width;
		}
		needtodraw=1;
		return 1;
	}
	return 0;
}

int FreehandInterface::InterfaceOn()
{ 
	showdecs=1;
	needtodraw=1;
	return 0;
}

int FreehandInterface::InterfaceOff()
{ 
	showdecs=0;
	needtodraw=1;
	return 0;
}

/*! \todo *** this is really cheezy, as it just copies bez->Refresh()
 */
int FreehandInterface::Refresh()
{ 
	//DBG cerr <<"  Freehand trying to startdrawing"<<endl;

	if (!data || !data->npoints) {
		if (needtodraw) needtodraw=0;
		return 0;
	}

	if (data->npoints) {
	//DBG cerr <<" Freehand: Drawing..";

		 // draw curve
		dp->NewFG(&data->linestyle.color);
		dp->LineAttributes(data->linestyle.width,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
		if (data->npoints>3) dp->drawbez(data->points,data->npoints/3,data->style&BEZS_CLOSED,50,1,1); // drawcurve
		dp->LineAttributes(0,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
		int c;

		 // draw control points
		if (showdecs) {
			dp->NewFG(deconcolor);
			 // draw little circles
			for (c=0; c<data->npoints; c++) {
				flatpoint p=realtoscreen(data->points[c]);
				int x=(int)p.x,y=(int)p.y;
				if (c%3!=1) dp->draw(x,y,3);
				else {
					switch (data->pointstyles[c/3]) {
						case BEZS_STIFF_EQUAL: dp->draw(x,y,5,5,0,0); break; // circle
						case BEZS_NSTIFF_EQUAL: dp->draw(x,y,5,5,0,3); break; // tri up
						case BEZS_STIFF_NEQUAL: dp->draw(x,y,5,5,0,1); break; // square
						case BEZS_NSTIFF_NEQUAL: dp->draw(x,y,5,5,0,2); break; // diamond
					}
				}
			}
			 // draw control rods
			for (c=0; c<data->npoints; c+=3) {
				if (c+1<data->npoints) dp->drawrline(data->points[c],data->points[c+1]);
				if (c+2<data->npoints) dp->drawrline(data->points[c+1],data->points[c+2]);
			}
		}
	}

	//DBG cerr <<"   Freehand all done drawing.\n";
	needtodraw=0;
	return 0;
}

//! Start a new freehand line.
int FreehandInterface::LBDown(int x,int y,unsigned int state,int count) 
{
	DBG cerr <<"freehandlbd:"<<x<<','<<y<<"..";
	buttondown|=LEFTBUTTON;
	mx=x;
	my=y;

	data=NULL;
	if (somedatafactory) {
		data=static_cast<BezData *>(somedatafactory->newObject(LAX_BEZDATA));
	} 
	if (!data) data=new BezData;
	
	data->linestyle=linestyle;
	flatpoint p=dp->screentoreal(x,y);
	data->AddPoint(-1,p,p,p,BEZS_STIFF_NEQUAL);
	
	DBG cerr <<"../freehandiLbd\n";
	return 0;
}

//! Finish a new freehand line by calling newData with it.
int FreehandInterface::LBUp(int x,int y,unsigned int state) 
{
	DBG cerr <<"  freehandLbup..";
	if (!buttondown) return 0;
	buttondown&=(~LEFTBUTTON);

	 // add the bezcurve to datastack
	 // ***should check for closed path???
	if (viewport) viewport->NewData(data);//count had 1 from LBDown, this incs it by another 1
	data->dec_count();
	data=NULL;

	return 0;
	DBG cerr <<"./bezLbup ";
}

/*! \todo *** this isn't very sophisticated, for elegance, should use some kind of 
 * bez curve fitting to cut down on unnecessary points should use a timer so 
 * stopping makes sharp corners and closer spaced points?
 */
int FreehandInterface::MouseMove(int x,int y,unsigned int state) 
{
	if (!(buttondown==LEFTBUTTON)) return 1;
	//DBG cerr <<" buttondown="<<buttondown<<"  ";

	if ((x-mx)*(x-mx)+(y-my)*(y-my)<mindist*mindist) return 1;
	flatpoint p=dp->screentoreal(x,y);
	data->AddPoint(-1,p,p,p,BEZS_STIFF_NEQUAL);
	if (data->npoints>2*3) {
		int n=data->npoints;
		p=data->points[n-2]-data->points[n-8];
		data->points[n-6]+=((data->points[n-8]-data->points[n-5])||p)/2;
		data->points[n-4]+=((data->points[n-2]-data->points[n-5])||p)/2;
	}
	needtodraw=1;
	my=y;
	mx=x;
	return 1;
}

} // namespace LaxInterfaces

