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
//    Copyright (C) 2004-2007,2011 by Tom Lechner
//



#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/bezinterface.h>
#include <lax/bezutils.h>
#include <lax/doublebbox.h>


#include <iostream>
using namespace std;
#define DBG 

using namespace Laxkit;


namespace LaxInterfaces {

//------------------------------------- BezData -------------------------------------
//#define BEZS_CLOSED         1
//#define BEZS_STIFF_EQUAL    0
//#define BEZS_STIFF_NEQUAL   1
//#define BEZS_NSTIFF_EQUAL   2
//#define BEZS_NSTIFF_NEQUAL  3
//
/*! \class BezData
 * \ingroup interfaces
 * \brief Data object for plain cubic bezier curves.
 *
 * Points are in chunks of 3: control-vertex-control, etc.
 * npoints is the total number of vertices plus number of controls.
 * So the number of vertices is npoints/3.
 */

	
//! Set all to NULL, and style=cstyle.
BezData::BezData(int cstyle)//cstyle=0
{
	style=cstyle;
	npoints=0;
	points=NULL;
	pointstyles=NULL;
}

//! Delete points and pointstyles.
BezData::~BezData()
{
	if (npoints) {
		delete[] points;
		delete[] pointstyles;
	}
}

//! Return tangent vector found starting at vertex sv at parameter t toward tov, 
/*! t==0 is at s, t==1 is at tov. This is useful for arrows when control points are
 *  too close to vertex (***Not so! when control point==vertex gives 0 tangent!)
 */
flatpoint BezData::beztangent(int sv,int tov,double t)
{
	if (sv<0 || sv>=npoints || tov<0 || tov>=npoints) return flatpoint(0,0);
	flatpoint bp;
	double a1,a2,a3,a4;
	a1=-3*t*t+6*t-3;
	a2=9*t*t-12*t+3;
	a3=-9*t*t+6*t;
	a4=3*t*t;
	int c1,c2;
	c1=(sv>tov?sv-1:sv+1);
	c2=(sv>tov?tov+1:tov-1);
	bp.x=(a1*points[sv].x + a2*points[c1].x + a3*points[c2].x + a4*points[tov].x);
	bp.y=(a1*points[sv].y + a2*points[c1].y + a3*points[c2].y + a4*points[tov].y);
	return bp;
}

//! Return the bezier point on the segment starting at point index (s/3)*3+1.
/*! t==0 is points[(s/3)*3+1] and t==1 is points[(s/3)*3+4]. If the t==1 point is beyond
 * npoints and the curve is closed, then of course t==1 is points[1].
 *
 * If s<0, return points[1]. uses s=1, and s+4>npoints-1 uses s=npoints**********
 * is clamped to sane values.
 */
flatpoint BezData::bezpoint(int s,double t)
{
	int e;
	if (style&BEZS_CLOSED) {
		s=((s%npoints)/3)*3+1;
		e=(s+2)%npoints;
	} else {
		s=(s/3)*3+1;
		if (s<1) return points[1];
		if (s+4>npoints-1) return points[npoints-2];
		e=s+2;
	}
	DBG cerr <<"  s="<<s<<"  e="<<e<<endl;
	flatpoint bp;
	double a1,a2,a3,a4;
	a1=(1-t)*(1-t)*(1-t);
	a2=3*t*(1-t)*(1-t);
	a3=3*t*t*(1-t);
	a4=t*t*t;
	bp.x=(a1*points[s].x + a2*points[s+1].x + a3*points[e].x + a4*points[e+1].x);
	bp.y=(a1*points[s].y + a2*points[s+1].y + a3*points[e].y + a4*points[e+1].y);
	return bp;
}

//! This only finds bounding box of verts and controls, not actual curve extent...
void BezData::FindBBox()
{
	if (!npoints) return;
	int c,c2;
	flatpoint pp;
	maxx=minx=points[0].x;
	maxy=miny=points[0].y;
	for (c=0; c<npoints; c+=3) {
		for (c2=0; c2<50; c2++) {
			pp=bezpoint(c,(double) c2/50);
	        if (pp.x<minx) minx=pp.x;
	        else if (pp.x>maxx) maxx=pp.x;
	        if (pp.y<miny) miny=pp.y;
	        else if (pp.y>maxy) maxy=pp.y;
		}
	}
}

/*! c1------v-----c2, c1,c2,v are all absolute coords\n
 * insert before where
 * 
 * Returns index of added v.
 */
int BezData::AddPoint(int where,flatpoint c1,flatpoint v,flatpoint c2,int pointstyle)
{
	if (where<0 || where>=npoints) where=npoints/3*3;
	where=where/3*3;
	flatpoint *temp;
	int c,*temps;
	temp=new flatpoint[npoints+3];
	temps=new int[npoints/3+1];
	
	for (c=0; c<where; c++) temp[c]=points[c];
	temp[where]=c1;
	temp[where+1]=v;
	temp[where+2]=c2;
	for (c=where; c<npoints; c++) temp[c+3]=points[c];
	delete[] points;
	points=temp;
	for (c=0; c<where/3; c++) temps[c]=pointstyles[c];
	temps[where/3]=pointstyle;
	for (c=where/3; c<npoints/3; c++) temps[c+1]=pointstyles[c];
	delete[] pointstyles;
	pointstyles=temps;
	npoints+=3;
	return where+1;
}

//! Delete the point at index.
int BezData::DeletePoint(int index)
{	// returns either index or if no point at index then index-1
	if (index<0 || index>=npoints) return -1;
	if (index%3!=1) { // on control point
		if (pointstyles[index/3]==BEZS_STIFF_EQUAL || pointstyles[index/3]==BEZS_NSTIFF_EQUAL) {
			points[index/3*3]=points[index/3*3+2]=points[index/3*3+1];
		} else {
			points[index]=points[index/3*3+1];
		}
		return index/3*3+1;
	} else { // vertex
		if (npoints==3) {
			delete[] points;
			points=NULL;
			delete[] pointstyles;
			pointstyles=NULL;
			npoints=0;
			return -1;
		}
		flatpoint *temp;
		int c,*temps;
		temp=new flatpoint[npoints-3];
		temps=new int[npoints/3-1];
		index=index/3*3;
		for (c=0; c<index; c++) temp[c]=points[c];
		for (c=index; c<npoints-3; c++) temp[c]=points[c+3];
		delete[] points;
		points=temp;
		for (c=0; c<index/3; c++) temps[c]=pointstyles[c];
		for (c=index/3; c<npoints/3-1; c++) temps[c]=pointstyles[c+1];
		delete[] pointstyles;
		pointstyles=temps;
		npoints-=3;
		return index>=npoints?(npoints-1)/3*3+1:index;
	}
}

//------------------------------------- BezInterface -------------------------------------


/*! \class BezInterface
 * \ingroup interfaces
 * \brief Interface to manipulate plain cubic bezier curves stored in BezData objects.
 *
 * If you want to manipulate lines that are composed of more than just bezier points, 
 * see PathInterface.
 *
 * \todo be able to restrict to monotonic increasing or decreasing functions,
 *   or y=f(x) (every x has 1 y)
 * \todo
 *  smart scanning,
 *  only refresh what's necessary?
 *   delete final/initial seg fault,
 *   delete screwing up control points,
 *   shift click: select more than one point, then middle is move 
 *       just those rather than object movements,
 *   showdecs: show only curpoint +- a couple, rather than all control points,
 *   *** work out mouse button/drag combos!!!
 *   *** need subdivide/other form of cut?, join, scale, rotate,
 *   *** when control point on vertex, need "visual" tangent not mathematical,
 *   *** deselecting single should occur on mouseup if mouse doesn't move,
 */


BezInterface::BezInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	creationstyle=0;
	curpointstyle=BEZS_STIFF_EQUAL;
	curpoint=-1;
	data=NULL;
	deconcolor=38066;
	decoffcolor=~0;
	linestyle.color.red  =linestyle.color.alpha=0xffff;
	linestyle.color.green=linestyle.color.blue  =0;
	showdecs=1;
	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
	buttondown=0;
	needtodraw=1;
}

BezInterface::~BezInterface()
{
	deletedata();
	DBG cerr <<"----in BezInterface destructor"<<endl;
}

int BezInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

int BezInterface::InterfaceOff()
{
	showdecs=0;
	needtodraw=1;
	return 0;
}

void BezInterface::deletedata()
{
	if (data) data->dec_count();
	data=NULL;
}

int BezInterface::UseThis(anObject *newdata,unsigned int mask)
{
	if (!newdata) return 0;
	if (dynamic_cast<BezData *>(newdata)) {
		if (data) deletedata();
		curpoints.flush();
		curpoint=-1;
		data=dynamic_cast<BezData *>(newdata);
		needtodraw=1;
		return 1;
	} else if (dynamic_cast<LineStyle *>(newdata)) { 
		DBG cerr <<"Bez new color stuff"<< endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(newdata);
		if (mask&GCForeground) {
			if (data) data->linestyle.color=nlinestyle->color;
			else linestyle.color=nlinestyle->color;
		}
		if (mask&GCLineWidth) {
			if (data) data->linestyle.width=nlinestyle->width;
			else linestyle.width=nlinestyle->width;
		}
		needtodraw=1;
	}
	return 0;
}

int BezInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
    if (!ndata || dynamic_cast<BezData *>(ndata)==NULL) return 1;
    BezData *bzd=data;
    data=dynamic_cast<BezData *>(ndata);
    int td=showdecs,ntd=needtodraw;
    showdecs=0;
    needtodraw=1;
    Refresh();
    needtodraw=ntd;
    showdecs=td;
    data=bzd;
    return 1;
}

DBG flatpoint ppp;

int BezInterface::Refresh()
{
	//cout <<"  Bez trying to startdrawing"<<endl;

	if (!data || !data->npoints) {
		if (needtodraw) needtodraw=0;
		return 0;
	}
	
	if (data->npoints) {
		//cout <<" Bez: Drawing..";

		int c;

		 // draw curve
		dp->NewFG(&data->linestyle.color);
		dp->LineAttributes(data->linestyle.width,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
		
		//if (data->npoints>3) dp->drawbez(data->points,data->npoints/3,data->style&BEZS_CLOSED,50,1,1); // drawcurve
		if (data->npoints>3) dp->drawbez(data->points,data->npoints/3,data->style&BEZS_CLOSED,50,1,1); // drawcurve

		dp->LineAttributes(0,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);

		 // draw control points
		if (showdecs) {
			dp->NewFG(deconcolor);
			
//----debugging:	//*****draw bounding boxes for segments
//			DoubleBBox bbox;
//			flatpoint ul,lr,p2,p3;
//			double extrema[5];
//			int extt;
//			if (data->npoints>4)
//				for (c=1; c<data->npoints-3; c+=3) {
//					bbox.minx=bbox.maxx=data->points[c].x;
//					bbox.miny=bbox.maxy=data->points[c].y;
//					extt=bezbbox(data->points[c],data->points[c+1],data->points[c+2],data->points[c+3],&bbox,extrema);
//					cout <<" num extrema:"<<extt<<endl;
//					for (int c2=0; c2<extt; c2++) {
//						p2=data->bezpoint(c,extrema[c2]);
//						cout <<"extrema:"<<extrema[c2]<<" at:"<<p2.x<<','<<p2.y;
//						p3=data->beztangent(c,c+3,extrema[c2]);
//						cout <<"  beztangent:"<<p3.x<<','<<p3.y<<endl;
//						dp->draw(dp->realtoscreen(p2),5);
//					}
//					dp->drawrline(flatpoint(bbox.minx,bbox.miny),flatpoint(bbox.maxx,bbox.miny));
//					dp->drawrline(flatpoint(bbox.maxx,bbox.miny),flatpoint(bbox.maxx,bbox.maxy));
//					dp->drawrline(flatpoint(bbox.maxx,bbox.maxy),flatpoint(bbox.minx,bbox.maxy));
//					dp->drawrline(flatpoint(bbox.minx,bbox.maxy),flatpoint(bbox.minx,bbox.miny));
//				}
				
//----more debugging:	//*****draw from bez_points
			DBG dp->NewFG(app->rgbcolor(0,255,0));
			DBG int resolution=20;
			DBG int n=data->npoints/3;
			DBG flatpoint pnts[resolution*n];
			DBG bez_to_points(pnts,data->points,n,resolution,1);
			DBG if (point_is_in_bez(ppp,data->points,data->npoints/3)) {
			DBG 	dp->drawlines(1,1,1,n*resolution,pnts);
			DBG }
			DBG dp->drawrlines(pnts,n*resolution,1);


			
			
			dp->NewFG(deconcolor);
			
			 //draw tmppoint 
			if (curpoint==-2) {
				flatpoint p=dp->realtoscreen(tmppoint);
				dp->draw((int)p.x,(int)p.y,7,7,0,0);
			}

			
			 // draw little arrow in dir of control point of curpoint, or 0, *** this don't work if p2==p1
			int cp=1;
			if (curpoint>=0) cp=curpoint/3*3+1;
			dp->drawarrow(data->points[cp],data->beztangent(cp,cp+1,0),6,15); //*** use "visual" tangent??
			
			 // draw little circles
			for (c=0; c<data->npoints; c++) {
				flatpoint p=dp->realtoscreen(data->points[c]);
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
			 // draw curpoint
			if (curpoints.n>0) for (c=0; c<curpoints.n; c++) {
				flatpoint fp=dp->realtoscreen(data->points[curpoints.e[c]]);
				int xx=(int)fp.x,yy=(int)fp.y;
				if (curpoints.e[c]%3==1) {
					switch (data->pointstyles[curpoints.e[c]/3]) {
						case BEZS_STIFF_EQUAL: dp->draw(xx,yy,5,5,1,0); break; // circle
						case BEZS_NSTIFF_EQUAL: dp->draw(xx,yy,5,5,1,3); break; // tri up
						case BEZS_STIFF_NEQUAL: dp->draw(xx,yy,5,5,1,1); break; // square
						case BEZS_NSTIFF_NEQUAL: dp->draw(xx,yy,5,5,1,2); break; // diamond
					}
					
				} else dp->drawf(xx,yy,3);  // draw curpoint
			}
		}
	}

	//cout <<"   Bez all done drawing.\n";
	needtodraw=0;
	return 0;
}

int BezInterface::scan(int x,int y,int startat,int pref) //startat=0  pref=-1;
{ //***smart scanning not working off by 1 *** preference to v, ^v+1 +v-1
	if (!data || data->npoints==0) return -1;
//
//	if (pref==-1) pref=startat%3;

	int c,c2=0;
	startat--;
	if (startat<0) startat=0;
	flatpoint p=dp->screentoreal(x,y);
	double r2=5/dp->Getmag();
	r2*=r2;
	do { // scans vertex first, forward control, then backward control
		for (c=1; c<data->npoints; c+=3) {
			//if ((p-data->points[c+c2])*(p-data->points[c+c2])<r2) return c;
			if ((p.x-data->points[c+c2].x)*(p.x-data->points[c+c2].x)+(p.y-data->points[c+c2].y)*(p.y-data->points[c+c2].y)<r2) return c+c2;
		}
		c2++;
		if (c2==2) c2=-1;
	} while (c2!=0);
	return -1;
}

//! Return new BezData with count 1.
BezData *newBezData(int creationstyle)
{
	BezData *ndata=NULL;
	if (somedatafactory) {
		ndata=static_cast<BezData *>(somedatafactory->newObject(LAX_BEZDATA));
		ndata->style=creationstyle;
	} 
	if (!ndata) ndata=new BezData(creationstyle);

//	data->AddPoint(0,flatpoint(-100,-100),flatpoint(-100,-100),flatpoint(-75,100),BEZS_STIFF_EQUAL);
//	data->AddPoint(3,flatpoint(75,100),flatpoint(100,-100),flatpoint(100,-100),BEZS_STIFF_EQUAL);

	return ndata;
}

//! Left button down.
int BezInterface::LBDown(int x,int y,unsigned int state,int count) 
{ // ***
	DBG cerr <<"bezlbd:"<<x<<','<<y<<"..";
	buttondown|=LEFTBUTTON;
	mx=x;
	my=y;
	lx=ly=10000;

	if (state&ControlMask) { // ***select underneath***necessary? preferential to control points?
		int p=scan(x,y,0,1); //*** cntl-lb doing funky.. should default to add v point-move c if no scan found?
		if (p>=0) { if (curpoints.pushnodup(p)) { curpoint=curpoints.e[curpoints.n-1]; needtodraw=1; } }
	} else { // data->npoints==total points, not num vertex,  --add new vertex
		int p;

		if (p=scan(x,y,curpoint,1),p>=0) { 
			if (curpoints.pushnodup(p)) { // found a point not already selected
				if (!(state&ShiftMask)) { // not shift, so select it only
					curpoints.flush();
					curpoints.push(p);
					curpoint=p;
					needtodraw=1;
					return 0;
				} // else adjust curpoint (it was pushed in the if above), and allow moving all points by returning
				curpoint=curpoints.e[curpoints.n-1]; needtodraw=1; 
			} else { // found a point already selected
				if (state&ShiftMask) { // deselect just this point
					curpoints.pop(p); 
					needtodraw=1; 
					return 0;
				}
				curpoints.flush();
				curpoints.push(p);
				curpoint=p;
				needtodraw=1;
				return 0;
			}
		} else { // no point found
			if (!data) {
				BezData *obj=NULL;
				ObjectContext *oc=NULL;
				int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
				if (c>0) obj=dynamic_cast<BezData *>(oc->obj);
				if (obj) { 
					 // found another BezData to work on.
					 // If this is primary, then it is ok to work on other bez, but not click onto
					 // other types of objects.
					 //***check this
					data=obj;
					if (viewport) viewport->ChangeObject(obj,oc); // this incs count
					else data->inc_count();
					needtodraw=1;
					return 0;
				} else if (c<0) {
					 // If there is some other non-bez data underneath (x,y) and
					 // this is not primary, then switch objects, and switch tools to deal
					 // with that object.
					if (!primary && c==-1 && viewport->ChangeObject(NULL,oc)) {
						buttondown&=~LEFTBUTTON;
						return 0;
					}
				}

				 // To be here, must want brand new data plopped into the viewport context
				if (viewport) viewport->ChangeContext(x,y,NULL);
				obj=newBezData(creationstyle); //data has count of 1
				obj->linestyle=linestyle;
				if (viewport) viewport->NewData(obj);//this sets data=NULL, makes obj count at least 2
				obj->dec_count(); // interface should directly cause only 1 count
				data=obj;

				curpoints.flush();
			}
			if (curpoints.n) {
				if (!(state&ShiftMask)) { curpoints.flush(); } // added point becomes only one selected
				else { lx=x; ly=y; return 0; } // point adds on mouse up if mouse doesn't move
			}
			
			flatpoint np=screentoreal(x,y);
			curpoints.push(data->AddPoint(curpoint+3,np,np,np,curpointstyle)); //***must addpoint after curpoint
			curpoint=curpoints.e[curpoints.n-1]; 
			//***if (state&ControlMask) curpoint=curpoint/3*3;
			//***else if (state&ShiftMask) curpoint=curpoint/3*3+2;
			needtodraw=1;
		}
	}
	DBG cerr <<"../bezLbd"<<endl;
	return 0;
}

int BezInterface::LBUp(int x,int y,unsigned int state) 
{
	DBG cerr <<"  bezLbup..";
	if (!(buttondown&LEFTBUTTON)) return 1;
	buttondown&=(~LEFTBUTTON);
	if (x==lx && y==ly)  {
		flatpoint np=dp->screentoreal(x,y);
		curpoints.push(data->AddPoint(curpoint+3,np,np,np,curpointstyle)); //***must addpoint after curpoint
		curpoint=curpoints.e[curpoints.n-1]; 
		needtodraw=1;
		lx=ly=10000;
	}
	return 0;
	DBG cerr <<"./bezLbup ";
}

int BezInterface::MouseMove(int x,int y,unsigned int state) 
{ 
	if (!data) return 0;
	if (!buttondown==LEFTBUTTON && data->npoints>3) {
		 //display bez point closest to mouse point
		flatpoint p=dp->screentoreal(x,y);
		
		DBG ppp=p;
		DBG cerr <<"point in:"<<point_is_in_bez(p,data->points,data->npoints/3)<<endl;

		mx=x; my=y;
		needtodraw=1;
		return 0;

		
//		 //display bez point closest to mouse point
//		flatpoint p=dp->screentoreal(x,y);
//		int c,at_c;
//		double d=1e+10,dd,
//			   tt,t;
//		for (c=1; c<data->npoints-2; c+=3) {
//			tt=bez_closest_point(p,data->points[c],data->points[c+1],data->points[c+2],data->points[c+3],15,&dd);
//			if (dd<d) { d=dd; t=tt; at_c=c; }
//			DBG cerr <<"dd="<<dd<<"  d="<<d<<"  at_c="<<at_c<<"  c="<<c<<endl;
//		}
//		if (data->style&BEZS_CLOSED) {
//			tt=bez_closest_point(p,data->points[c],data->points[c+1],data->points[0],data->points[1],15,&dd);
//			if (dd<d) { d=dd; t=tt; at_c=c; }
//		}
//		curpoint=-2;
//		tmppoint=data->bezpoint(at_c,t);
//		needtodraw=1;
//		return 0;
	}
	
	
	if (curpoints.n==0) return 1;
	
	//cout <<" buttondown="<<buttondown<<"  ";

	if (buttondown&LEFTBUTTON) {
		if (x==mx && y==my) return 1;
		if (data->npoints) {
			flatpoint d=dp->screentoreal(x,y)-dp->screentoreal(mx,my);
			double dx=d.x,dy=d.y;
			int curp,v;
			for (int c=0; c<curpoints.n; c++) {
				curp=curpoints.e[c]; 
				v=curp/3*3+1;
				//*** this is horrible must move control points first, then vertices lazy just now
				if (curp%3==0) {  // moves v-1, should be shiftmask usually
					switch (data->pointstyles[curp/3]) {
						case BEZS_STIFF_EQUAL: {
							data->points[v-1].x+=dx;
							data->points[v-1].y+=dy;
						 	data->points[v+1]=2*data->points[v]-data->points[v-1];
						 } break;
						case BEZS_NSTIFF_EQUAL: {
							data->points[v-1].x+=dx;
							data->points[v-1].y+=dy;
							double n23=norm(data->points[v-1]-data->points[v]),
								   n21=norm(data->points[v+1]-data->points[v]);
							if (n21!=0) data->points[v+1]=data->points[v] + n23/n21*(data->points[v+1]-data->points[v]);
							else { // if c1,2 start at v give c2 a stiff direction
						 		data->points[v+1]=2*data->points[v]-data->points[v-1];
							}
						 } break;
						case BEZS_STIFF_NEQUAL: { 
							data->points[v-1].x+=dx;
							data->points[v-1].y+=dy;
							double n23=norm(data->points[v-1]-data->points[v]),
								   n21=norm(data->points[v+1]-data->points[v]);
							if (n23!=0) data->points[v+1]=data->points[v] - n21/n23*(data->points[v-1]-data->points[v]);
						} break;
						case BEZS_NSTIFF_NEQUAL: { 
							data->points[v-1].x+=dx;
							data->points[v-1].y+=dy;
						} break;
					}
				} else if (curp%3==2) { // moves v+1, should be controlmask usually
					switch(data->pointstyles[curp/3]) {
						case BEZS_STIFF_EQUAL: {
							data->points[v+1].x+=dx;
							data->points[v+1].y+=dy;
						 	data->points[v-1]=2*data->points[v]-data->points[v+1];
						 } break;
						case BEZS_NSTIFF_EQUAL: {
							data->points[v+1].x+=dx;
							data->points[v+1].y+=dy;
							double n23=norm(data->points[v-1]-data->points[v]),
								   n21=norm(data->points[v+1]-data->points[v]);
							if (n23!=0) data->points[v-1]=data->points[v] + n21/n23*(data->points[v-1]-data->points[v]);
							else { // if c1,2 start at v give c1 a stiff direction
						 		data->points[v-1]=2*data->points[v]-data->points[v+1];
							}
						 } break;
						case BEZS_STIFF_NEQUAL: {
							data->points[v+1].x+=dx;
							data->points[v+1].y+=dy;
							double n23=norm(data->points[v-1]-data->points[v]),
								   n21=norm(data->points[v+1]-data->points[v]);
							if (n21!=0) data->points[v-1]=data->points[v] - n23/n21*(data->points[v+1]-data->points[v]);
						} break;
						case BEZS_NSTIFF_NEQUAL: {
							data->points[v+1].x+=dx;
							data->points[v+1].y+=dy;
						} break;
					}
				} else { // is on vertex
					data->points[v].x+=dx;
					data->points[v].y+=dy;
					data->points[v+1].x+=dx;
					data->points[v+1].y+=dy;
					data->points[v-1].x+=dx;
					data->points[v-1].y+=dy;
				}
			}
			
			mx=x; my=y;
			needtodraw=1;
		}
		return 0;
	} 

	my=y;
	mx=x;
	return 1;
}

/*! 
 * <pre>
 *  cntl    switch to moving the associated vertex
 *  shift   switch to moving the associated vertex
 * </pre>
 */
int BezInterface::CharRelease(unsigned int ch,unsigned int state)
{
	switch (ch) {
		case LAX_Shift: // shift 
		case LAX_Control: // control
		{ 
			if (buttondown&LEFTBUTTON && data && data->npoints && curpoint>=0) { // to v
				curpoints.pop(curpoint);
				curpoint=curpoint/3*3+1;
				curpoints.pushnodup(curpoint);
				flatpoint p=realtoscreen(data->points[curpoint]);
				XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-mx,(int)p.y-my);
				mx=(int)p.x;
				my=(int)p.y;
				needtodraw|=2;
			}
		} return 0;
	}
	return 1;
}

/*! 
 * <pre>
 *  enter   start a new path
 *  'C'     arrange selected points in a circle
 *  'c'     toggle whether the path is closed, cut after current point
 *  'a'     select all or none
 *  cntl    switch to moving the associated control point
 *  shift   switch to moving the other associated control point
 * </pre>
 */
int BezInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state) 
{
	DBG fprintf(stderr,"BezInterface:%x\n",ch);
	switch(ch) {
		case LAX_Control: //control
			if (buttondown&LEFTBUTTON && data && data->npoints && curpoint>=0) { // to v-1
				curpoints.pop(curpoint);
				curpoint=curpoint/3*3;
				curpoints.pushnodup(curpoint);
				flatpoint p=realtoscreen(data->points[curpoint]);
				XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-mx,(int)p.y-my);
				mx=(int)p.x;
				my=(int)p.y;
				needtodraw|=2;
			}
			return 0;
		case LAX_Shift: //shift
			if (buttondown&LEFTBUTTON && data && data->npoints && curpoint>=0) { // to v+1
				curpoints.pop(curpoint);
				curpoint=curpoint/3*3+2;
				curpoints.pushnodup(curpoint);
				flatpoint p=realtoscreen(data->points[curpoint]);
				XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-mx,(int)p.y-my);
				mx=(int)p.x;
				my=(int)p.y;
				needtodraw|=2;
			}
			return 0;
		case LAX_Del: // del
		case LAX_Bksp:	{ // backspace
				DBG cerr <<"  bezbs  ";
				if (buttondown) return 1;
				while (curpoints.n) {
					curpoint=data->DeletePoint(curpoints.pop());
				}
				needtodraw=1;
				curpoint=-1;
			} return 0;

		case 'a': {
			if (curpoints.n) {
				curpoints.flush();
				curpoint=-1;
			} else {
				for (int c=0; c<data->npoints; c++) { curpoints.push(c); curpoint=curpoints.e[curpoints.n-1]; }
			}
			needtodraw=1;
			return 0;
		}

		case 'c': toggleclosed(); needtodraw=1; return 0;
		case 'C': MakeCircle(); return 0;
		case 'p':  {
			for (int c=0; c<curpoints.n; c++) { // *** redo this so you can press p on control or vertex, and it toggles vert once only
				data->pointstyles[curpoints.e[c]/3]=(data->pointstyles[curpoints.e[c]/3]+1)%4;
			}
			needtodraw=1;
		} return 0;
		case 'd':  showdecs=!showdecs; needtodraw=1; return 0;
		case LAX_Enter: { // ***start a new one
				if (!data) return 0;
				deletedata();
			} return 0;
		default: return 1;
	}
	return 1;
}

//! Make a circle out of selected points, or if none selected, from all the points
/*! 
 * To make 2 vertex points lie \f$\theta\f$ degrees apart on a circle of radius r,
 * then the control rods will have length v:
 * \f[
 * 	v=\frac{4\:r}{3}\:\frac{2\;sin(\theta/2)-sin(\theta)}{1-cos(\theta)};
 * \f]
 */
void BezInterface::MakeCircle()
{
	if (!data || data->npoints<6) return;

	 // first make curpoints have only control points
	int c;
	for (c=curpoints.n-1; c>=0; c--) {
		if (curpoints.e[c]%3!=1) curpoints.pop(c);
	}
	if (curpoints.n==1) curpoints.flush();
	if (curpoints.n==0) for (c=1; c<data->npoints; c+=3) curpoints.push(c);

	 // should sort curpoints, or transformation looks bizarre
	//***
	
	 // next find bounding box for those points
	double minx,maxx,miny,maxy;
	minx=maxx=data->points[curpoints.e[0]].x;
	miny=maxy=data->points[curpoints.e[0]].y;
	for (c=1; c<curpoints.n; c++) {
		if (data->points[curpoints.e[c]].x<minx) minx=data->points[curpoints.e[c]].x;
		else if (data->points[curpoints.e[c]].x>maxx) maxx=data->points[curpoints.e[c]].x;
		if (data->points[curpoints.e[c]].y<miny) miny=data->points[curpoints.e[c]].y;
		else if (data->points[curpoints.e[c]].y>maxy) maxy=data->points[curpoints.e[c]].y;
	}
	double w = (maxx - minx)/2,
		h=(maxy-miny)/2,
		theta=2*3.141592653589/(curpoints.n),
		r,v;
	r=(w>h?w:h);

	
	v=4*r*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta));
//	v2=r*4*(-cos(theta)-1+2*cos(theta/2))/3/sin(theta);
	DBG cerr <<"MakeCircle: r="<<r<<" theta="<<theta/3.1415926535*180<<" v="<<v<<endl;
	flatpoint center=flatpoint((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);
	double xx,yy;
	for (c=0; c<curpoints.n; c++) {
		xx=cos(c*theta);
		yy=sin(c*theta);
		data->points[curpoints.e[c]]=center + flatpoint(r*xx,r*yy);
		data->points[curpoints.e[c]-1]=data->points[curpoints.e[c]] + flatpoint(v*yy,-v*xx);
		data->points[curpoints.e[c]+1]=data->points[curpoints.e[c]] + flatpoint(-v*yy,v*xx);
	}
	DBG cerr <<endl;
	needtodraw=1;
}

void BezInterface::toggleclosed()
{ //*** maybe also toggle creationstyle?
	if (!data) return;
	if (data->style&BEZS_CLOSED) data->style&=~BEZS_CLOSED;
	else data->style|=BEZS_CLOSED;
	needtodraw|=2;
}

} // namespace Laxkit

