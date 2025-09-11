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
//    Copyright (C) 2004-2006,2011 by Tom Lechner
//


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linesinterface.h>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//#define LINESDATA_CLOSED   1
//#define LINESDATA_LINES    2
//#define LINESDATA_POLYGON  4
//#define LINESDATA_SYMPOLY  8

// // most of these are for polygon creation, and where the corners point
//#define LINESDATA_UP       16
//#define LINESDATA_DOWN     32
//#define LINESDATA_RIGHT    64
//#define LINESDATA_LEFT     128
//#define LINESDATA_PLUSHALF 256
//#define LINESDATA_RECT     512
//#define LINESDATA_MAXINBOX 1024

////-------------------------------------- LinesData --------------------------
/*! \class LinesData
 * \ingroup interfaces
 * \brief Data class for LinesInterface.
 */

void LinesData::FindBBox()
{
	if (npoints==0) { minx=maxx=miny=maxy=0; return; }
	minx=maxx=points[0].x; 
	miny=maxy=points[0].y; 
	if (npoints==1) return;
	for (int c=1; c<npoints; c++) {
		if (points[c].x<minx) minx=points[c].x;
		else if (points[c].x>maxx) maxx=points[c].x;
		if (points[c].y<miny) miny=points[c].y;
		else if (points[c].y>maxy) maxy=points[c].y;
	}
}

int LinesData::Delete(int which) // <0 error, else npoints
{			//*** moves points, not new array
	if (npoints==0 || which<0 || which>=npoints) return -1;
	for (int c=which; c<npoints-1; c++) points[c]=points[c+1];
	return --npoints;
}

int LinesData::AddAfter(int afterwhich, flatpoint p)
{
	DBG cerr <<"\nAddAfter:"<<afterwhich<<" np:"<< npoints<<" point "<<p.x<<','<<p.y<<"   ";
	if (afterwhich<0 || afterwhich>=npoints) afterwhich=npoints-1;
	npoints++;
	flatpoint *temp=new flatpoint[npoints];
	int c=0;
	if (afterwhich>=0) for (c=0; c<=afterwhich; c++) temp[c]=points[c];
	temp[c]=p;
	for (c=afterwhich+2; c<npoints; c++) temp[c]=points[c-1];
	if (points) delete[] points;
	points=temp;
			
	DBG cerr << " LineData:Point list:"<< npoints<<endl;
	DBG for (int c=0; c<npoints; c++) {
	DBG 	cerr <<points[c].x<<','<<points[c].y<<endl;
	DBG }

	return afterwhich+1;
}


////-------------------------------------- LinesInterface --------------------------
/*! \class LinesInterface
 * \ingroup interfaces
 * \brief Interface for LinesData, just straight point to point lines.
 *
 * \todo this is really old code and needs a good going over
 */


LinesInterface::LinesInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	linestyle.Color(0xffff,0,0,0xffff);
	controlcolor=38066; // defaults to white, change right after creation otherwise
	creating=0;
	data=NULL;
	showdecs=1;
	curpoint=-1;
	buttondown=0;
	creationstyle=0;
	Setupcreation(4,1);
	creationgravity=NorthWestGravity;
	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
	
	needtodraw=1;
}

int LinesInterface::UseThis(int id,int ndata)
{
	DBG cerr<<" linesUseThis id:"<<id<<','<<ndata<<"  ";
	if (id==2 && ndata>2) {
		creationsides=ndata;
		char c[100];
		sprintf(c,"New creationsides:%d",creationsides);
		app->postmessage(c);
		return 0;
	} else if (id==3 && ndata>0) {
		creationturns=ndata;
		char c[100];
		sprintf(c,"New creationturns:%d",creationturns);
		app->postmessage(c);
		return 0;
	}
	return 1;
}

int LinesInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

int LinesInterface::InterfaceOff()
{
	Clear();
	showdecs=0;
	needtodraw=1;
	return 0;
}

//! Returns whether data is now closed.
int LinesInterface::toggleclosed(int c)
{
	if (!data) return creationstyle&LINESDATA_CLOSED;
	needtodraw|=2;
	if (data->style&LINESDATA_CLOSED) {
		data->style&=~LINESDATA_CLOSED;
		if (data->npoints!=0) if (curpoint==data->npoints*2-1) {
			curpoint=data->npoints*2-2;
			if (curpoint<0) curpoint=0;
		}
		return 0;
	} else {
		data->style|=LINESDATA_CLOSED;
		return 1;
	}
}

void LinesInterface::deletedata()
{
	if (data) data->dec_count();
	data=NULL;
}

/*! \todo does viewport->NewData(newdata).. maybe newdata exists in it already?
 */
int LinesInterface::UseThis(anObject *newdata,unsigned int mask)
{
	if (!newdata) return 0;
	if (dynamic_cast<LinesData *>(newdata)) {
		if (data) deletedata();
		curpoint=-1;
		LinesData *ndata=dynamic_cast<LinesData *>(newdata);
		if (viewport) viewport->NewData(ndata); //incs count by 1
		data=ndata;
		needtodraw=1;
		return 1;
	} else if (dynamic_cast<LineStyle *>(newdata)) { 
		DBG cerr <<"LinesInterface new color stuff"<<endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(newdata);

		if (mask & (LINESTYLE_Color | LINESTYLE_Color2)) {
			if (data) data->linestyle.color=nlinestyle->color;
			else linestyle.color=nlinestyle->color;
		}
		if (mask & LINESTYLE_Width) {
			if (data) data->linestyle.width=nlinestyle->width;
			else linestyle.width=nlinestyle->width;
		}
		needtodraw=1;
	}
	return 0;
}

int LinesInterface::Refresh() //***need mechanism to only draw what's necessary
{
	if (!dp || !needtodraw) return 0;
	if (!data || !data->npoints) {
		if (needtodraw) needtodraw=0;
		return 1;
	}
	int c;

	//DBG cerr <<"  LinesRefresh";

		// draw data
	dp->NewFG(&data->linestyle.color);
	dp->LineAttributes(data->linestyle.width,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
	if (creating) {
		flatpoint fp[data->npoints];
		for (c=0; c<data->npoints; c++) {
			fp[c]=data->points[c].x*newa*createx + data->points[c].y*newb*createy + createp;
		}
		dp->drawrlines(fp,data->npoints,data->style&LINESDATA_CLOSED);
	} else dp->drawrlines(data->points,data->npoints,data->style&LINESDATA_CLOSED);
	dp->LineAttributes(0,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
	
		// draw control points;
	 // if creating, draw dotted box
	if (creating) {
		dp->NewFG(controlcolor);
		dp->LineAttributes(0,LineDoubleDash,LAXCAP_Butt,LAXJOIN_Miter);
		flatpoint p[4];
		p[0]=createp;
		p[1]=createp+createx*newa;
		p[2]=p[1]+createy*newb;
		p[3]=createp+createy*newb;
		dp->drawrlines(p,4,1);
		dp->LineAttributes(0,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
	} else if (showdecs) { 
		dp->NewFG(controlcolor);
		flatpoint fp;
		int x,y,x2,y2;
		fp=dp->realtoscreen(data->points[0]);
		x=(int)fp.x;
		y=(int)fp.y;
		dp->draw(x,y,5);
		for (c=1; c<data->npoints; c++) {
			fp=dp->realtoscreen(data->points[c]);
			x2=(int)fp.x;
			y2=(int)fp.y;
			dp->draw(x2,y2,5);
			dp->draw((x+x2)/2,(y+y2)/2,3);
			x=x2;
			y=y2;
		}
		if (data->style&LINESDATA_CLOSED && data->npoints>1) {
			fp=dp->realtoscreen(data->points[0]);
			x=(int)fp.x;
			y=(int)fp.y;
			dp->draw((x+x2)/2,(y+y2)/2,3);
		}
		if (curpoint>=0) {
			if (curpoint&1) { // control point
				if (curpoint==data->npoints*2-1) fp=dp->realtoscreen((data->points[data->npoints-1]+data->points[0])/2);
				else {
					//DBG cerr <<"  cp/2="<<curpoint/2<<" cp/2+1="<<curpoint/2+1<<"  ";
					fp=dp->realtoscreen((data->points[curpoint/2]+data->points[curpoint/2+1])/2);
				}
			} else { // vertex
				fp=dp->realtoscreen(data->points[curpoint/2]);
			}
			dp->drawf((int)fp.x,(int)fp.y,(curpoint&1)?3:5);  // draw curpoint
		}
		 
	}
	needtodraw=0;
	return 0;
}

 //**** maybe pick closest, not just within a distance?? how to differentiate creation/modifying
int LinesInterface::scan(int x,int y) //***does not do smart scanning, starts at c=0 each time, only checks points, no online jazz
{ // * NOTE: scan returns not the index, but 2*index, and +1 if control point
	if (!data) return -1;
	int c,dx,dy;
	flatpoint p,p2;
	 // scan for vertex points
	for (c=0; c<data->npoints; c++) {
		p=realtoscreen(data->points[c]);
		if ((p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)<25) {
			DBG cerr <<" scanlinefound "<<2*c<<"  ";
			return 2*c;
		}
	}
	 // scan for control points
	for (c=0; c<data->npoints-1; c++) {
		p=realtoscreen(data->points[c]);
		p2=realtoscreen(data->points[c+1]);
		dx=(int)((p.x+p2.x)/2)-x;
		dy=(int)((p.y+p2.y)/2)-y;
		if (dx*dx+dy*dy<25) {
			DBG cerr <<" scanlinefound "<<2*c+1<<"  ";
			return 2*c+1;
		}
	}
	 // scan for final control point if closed path
	if (data->npoints>1 && data->style&LINESDATA_CLOSED) {
		p=realtoscreen(data->points[0]);
		p2=realtoscreen(data->points[c]);
		dx=(int)((p2.x+p.x)/2)-x;
		dy=(int)((p2.y+p.y)/2)-y;
		if (dx*dx+dy*dy<25) {
			DBG cerr <<" scanlinefound "<<2*c+1<<"  ";
			return 2*c+1;
		}
	}
	return -1;
}

int LinesInterface::LBDown(int x,int y,unsigned int state,int count)
{
	DBG cerr << "  in data lbd..";
	buttondown|=1;
	mx=x;
	my=y;
	int c=scan(x,y);
	if (c>=0) { // scan found one...
		curpoint=c;
		DBG cerr <<"  linesfoundone:"<<curpoint<<"   ";
		needtodraw|=2;
		return 0;
	}
	DBG cerr <<"  nolinepointfound  ";

	 // create new of point not found..
	char j=0;
	if (!data) { 
		LinesData *obj=NULL;
		ObjectContext *oc=NULL;
		int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
		if (c>0) obj=dynamic_cast<LinesData *>(oc->obj); //***actually don't need the cast, if c>0 then obj is ImageData
	 	if (obj) { 
			 // found another LinesData to work on.
			 // If this is primary, then it is ok to work on other images, but not click onto
			 // other types of objects.
			data=obj;
			if (viewport) viewport->ChangeObject(obj,oc); // this incs count
			else data->inc_count();
			needtodraw=1;
			return 0;
		} else if (c<0) {
			 // If there is some other non-image data underneath (x,y) and
			 // this is not primary, then switch objects, and switch tools to deal
			 // with that object.
			if (!primary && c==-1 && viewport->ChangeObject(NULL,oc)) {
				buttondown&=~LEFTBUTTON;
				return 0;
			}
		}

		 // To be here, must want brand new data plopped into the viewport context
		if (viewport) viewport->ChangeContext(x,y,NULL);
		
		j=1; 
		LinesData *ndata=NULL;
		if (somedatafactory) {
			ndata=static_cast<LinesData *>(somedatafactory->newObject(LAX_LINESDATA));
		} 
		if (!ndata) ndata=new LinesData;//creates 1 count
		
		ndata->linestyle=linestyle;
		ndata->style=creationstyle; 
		if (viewport) viewport->NewData(ndata);//incs count
		ndata->dec_count();
		data=ndata;
	}
	if (data->style&LINESDATA_LINES) {
		DBG cerr <<"  LinesLBDcurpoint="<<curpoint<<endl;
		curpoint=2*data->AddAfter(curpoint/2,dp->screentoreal(x,y)); 
		needtodraw=1;
		return 0;
		DBG cerr <<"..lineslbd done   ";
	} else { // is not lines
		if (j==1) { // just created one
			creating=1;
			data->style|=LINESDATA_CLOSED;
			createp=dp->screentoreal(x,y);
			createx=flatpoint(1,0); //***must incorporate createangle
			createy=flatpoint(0,1);
			newa=newb=0;
			int c;
			double maxx=0,minx=0,miny=0,maxy=0;
			flatpoint p;
			for (c=0; c<creationsides; c++) {
				p=flatvector(cos(c*2*M_PI*creationturns/creationsides+creationangle),
						sin(c*2*M_PI*creationturns/creationsides+creationangle));
				if (c==0) { minx=maxx=p.x; miny=maxy=p.y; }
				else {
					if (p.x<minx) minx=p.x;
					else if (p.x>maxx) maxx=p.x;
					if (p.y<miny) miny=p.y;
					else if (p.y>maxy) maxy=p.y;
				}
				data->AddAfter(-1,p);
			}
			for (c=0; c<creationsides; c++) {
				if (creationstyle&(LINESDATA_RECT | LINESDATA_MAXINBOX)) { // expand data to fit max in box
					data->points[c].x-=minx;
					data->points[c].y-=miny;
					data->points[c].x/=(maxx-minx);
					data->points[c].y/=(maxy-miny);
				} else {
					data->points[c].x+=1;
					data->points[c].y+=1;
					data->points[c].x/=2;
					data->points[c].y/=2;
				}
			}
		} else {
			if (data->style&LINESDATA_POLYGON) {
			} else if (data->style&LINESDATA_SYMPOLY) {
			}
		}
	}
	DBG cerr <<"..lineslbd done   ";
	return 0;
}

void LinesInterface::Setupcreation(int sides,int turns)
{
	if (creationstyle==0) creationstyle=LINESDATA_LINES | LINESDATA_RIGHT;
	if (creationstyle&LINESDATA_RECT) {
		creationsides=4;
		creationturns=1;
	} else {
		if (sides) creationsides=sides;
		if (turns) creationturns=turns;
		if (!creationsides) creationsides=4;
		if (!creationturns) creationturns=1;
	}
	//***factor out common factors sides/turns?
	if (creationstyle&LINESDATA_RECT) creationangle=0;
	else if (creationstyle&LINESDATA_UP) creationangle=M_PI/2;
	else if (creationstyle&LINESDATA_DOWN) creationangle=3*M_PI/2;
	else if (creationstyle&LINESDATA_LEFT) creationangle=M_PI;
	else creationangle=0;
	if (creationstyle&(LINESDATA_PLUSHALF | LINESDATA_RECT)) creationangle+=M_PI*creationturns/creationsides;

	//***
	DBG cerr <<" line-create:";
	DBG switch(creationstyle&(2|4|8)) {
	DBG 	case LINESDATA_LINES: cerr <<"data"; break;
	DBG 	case LINESDATA_POLYGON: cerr <<"pgon"; break;
	DBG 	case LINESDATA_SYMPOLY: cerr <<"spoly"; break;
	DBG }
	DBG cerr <<" sides,turns,angle:"<<creationsides<<','<<creationturns<<','<<creationangle<<endl;
}

int LinesInterface::LBUp(int x,int y,unsigned int state) 
{
	if (!(buttondown&1)) return 1;
	buttondown&=~1;
	if (creating) {
		for (int c=0; c<data->npoints; c++) {
			data->points[c]=data->points[c].x*newa*createx + data->points[c].y*newb*createy + createp;
		}
		creating=0;
		needtodraw=1;
	}
	return 0;
}

void LinesInterface::movecontrol(int which,int x,int y) 
	{ movecontrol(which,dp->screentoreal(x,y)-dp->screentoreal(mx,my)); }
	
void LinesInterface::movecontrol(int which,flatpoint d)
{
	if ((which&1)!=1 || which<0 || which>data->npoints*2-1) return;
	int l1=which/2,
		l2=(l1-1)%data->npoints,
		r1=(l1+1)%data->npoints,
		r2=(l1+2)%data->npoints;
	if (l2<0) l2=data->npoints-1;
	//DBG cerr << "np,l1,l2,r1,r2 "<<data->npoints<<':'<<l1<<","<<l2<<","<<r1<<','<<r2<<endl;
	flatline l(data->points[l1],data->points[r1]), // ***assumes l(1,2)= 1->2: l.v=2-1
		 ll(data->points[l1],data->points[l2]),
		 lr(data->points[r1],data->points[r2]);
	l.p+=d; 
	if ((data->style&LINESDATA_CLOSED && data->npoints>2) || (which>1 && which<2*data->npoints-3)) { // middle control point
		data->points[l1]=l*ll; //**** danger no parallel checking
		data->points[r1]=l*lr; //**** danger no parallel checking
	} else if (which==1) {  // first control point, open path or closed and n==2
		if (data->npoints==2) {
			data->points[0]+=d;
			data->points[1]+=d;
		} else { 
			data->points[1]=l*lr; //**** danger no parallel checking
			//data->points[0]+=(d|=l.v); // moves first point perp to line
			data->points[0]=data->points[1]-l.v; // moves last point to keep rod same length
		}
	} else { // final control point
		if (data->npoints==2) { //***unreachable?
			data->points[0]+=d;
			data->points[1]+=d;
		} else {
			data->points[data->npoints-2]=l*ll; //**** danger no parallel checking
			//data->points[data->npoints-1]+=(d|=l.v); // moves last point perp to line
			data->points[data->npoints-1]=data->points[data->npoints-2]+l.v; // moves last point to keep rod same length
		}
	}
}

int LinesInterface::MouseMove(int x,int y,unsigned int state) 
{
	//***if creating polygon is same as symmetric polygon
	
	if (!(buttondown&1) || !data) return 1;
	if (creating) {
		flatpoint np=screentoreal(x,y)-createp;
		if (state&ControlMask) { // rotate createx,y
			flatpoint op=screentoreal(mx,my)-createp;
			if ((op*op)*(np*np)!=0) {
				double a=asin((op.x*np.y-op.y*np.x)/sqrt((op*op)*(np*np)));
				createx=rotate(createx,a,0);
				createy=rotate(createy,a,0);
			}
		}
		newa=np*createx; // assume createx,y normalized
		newb=np*createy; // these are actually 2*a,2*b with a,b meas from center
		if (state&ShiftMask) {
			if (fabs(newa)>fabs(newb)) newa=fabs(newb)*(newa>0?1:-1);
			else newb=fabs(newa)*(newb>0?1:-1);
		}
		
		mx=x; my=y;
		needtodraw|=1;
		return 0;
	}
	if (curpoint<0 || !data->npoints) return 1;
	if ((curpoint&1)==1 && (data->style&(LINESDATA_LINES|LINESDATA_POLYGON))) { // on control point => npoints>1
		 // move line by control point 
		movecontrol(curpoint,x,y);
		mx=x; my=y;
	} else if (data->style&LINESDATA_LINES) { // on vertex for data
		 // must be on vertex point here
		flatpoint d=screentoreal(x,y)-screentoreal(mx,my);
		mx=x; my=y;
		double dx=d.x,dy=d.y;
		data->points[curpoint/2].x+=dx;
		data->points[curpoint/2].y+=dy;
	} else if (data->style&LINESDATA_POLYGON) { // on vertex point: move curpoint+-1
		if (data->npoints==1) return 0;
		if (!(data->style&LINESDATA_CLOSED) && curpoint==0) { // if on first vertext
			movecontrol(curpoint+1,x,y);
		} else if (!(data->style&LINESDATA_CLOSED) && curpoint==data->npoints*2-2) { // if on last vertext
			movecontrol(curpoint-1,x,y);
		} else { // in the middle somewhere or closed path
			if (curpoint>0) {
				movecontrol(curpoint+1,x,y);
				movecontrol(curpoint-1,x,y);
			} else if (curpoint==0) { // has to be closed path
				movecontrol(data->npoints*2-1,x,y);
				movecontrol(curpoint+1,x,y);
			} 
		}
		mx=x; my=y;
		needtodraw=1;
	} else if (data->style&LINESDATA_SYMPOLY) { // on vertex or control ***needs work, more flexible??
		flatpoint centroid,p,p2;
		for (int c=0; c<data->npoints; c++) centroid+=data->points[c];
		centroid/=data->npoints;
		p=screentoreal(x,y)-centroid;
		p2=screentoreal(mx,my)-centroid;
		if (p2*p2==0) return 0;
		double mag=sqrt((p*p)/(p2*p2));
		for (int c=0; c<data->npoints; c++) {
			data->points[c]=centroid+(data->points[c]-centroid)*mag;
		}
		needtodraw=1;
		mx=x; my=y;
	}
	needtodraw|=2;
	return 0;
}

/*! 
 * <pre>
 *  del/bksp  delete a point
 *  '?'       post help message
 *  ' '       make a creating rectangle square with real
 *  'c'       toggle a closed path
 *  'd'       toggle decorations
 *  'm'       change mode between lines, polygon, and symmetric polygon
 *  'r'       make rectangles
 *  'b'       begin new one
 * </pre>
 */
int LinesInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state) 
{ //***
	switch(ch) {
		case LAX_Shift: { // shift
			//***do square bounds
		} break;
//		case LAX_Control: { // cntl
//			*** start moving wildpoint
//		} break;
		case LAX_Del: // del
		case LAX_Bksp:	{ // backspace //*** del on a control point removes that whole segment?
			if (state&ControlMask) {
				if (data) deletedata();
				curpoint=-1;
				creating=0;
				needtodraw=1;
				return 0;
			}
			if (data->style&LINESDATA_LINES) {
				if (!data || !data->npoints) return 0;
				if (curpoint/2>=data->npoints || curpoint<0) return 0;
				data->Delete(curpoint/2);
				if (curpoint<2 && data->npoints==0) curpoint=-1;
				else if (curpoint/2>=data->npoints) curpoint=data->npoints*2-2;
				needtodraw=1;
			}
		} return 0;
		case '?': { // display help
			DBG cerr <<"HELP"<<endl;
			app->postmessage("' '=straighten, m=line mode, r=rect/no rect, b=start new");
			return 0;
		  }
		case ' ': { // make creating rect up and down
			if (!creating) return 1;
			createx=flatpoint(1,0);
			createy=flatpoint(0,1);
			MouseMove(mx,my,0);
			return 0;
		} break;

		DBG case 'p':
		DBG cerr << " LineData:Point list:"<< data->npoints<<endl;
		DBG for (int c=0; c<data->npoints; c++) {
		DBG	 cerr <<data->points[c].x<<','<<data->points[c].y<<endl;
		DBG }
		DBG cerr <<"createx, createy:"<<createx.x<<','<<createx.y<<' '<<createy.x<<','<<createy.y<<endl;
		DBG cerr <<"newa,newb:"<<newa<<','<<newb<<endl;
		DBG break;
		
		case 'c': if (!data) return 0;
			if (data->style&LINESDATA_CLOSED) {
				data->style&=~LINESDATA_CLOSED;
				if (curpoint==data->npoints*2-1) curpoint=-1;
			} else data->style|=LINESDATA_CLOSED;
			needtodraw=1;
			return 0;
		case 'd': {
			showdecs=!showdecs;
			needtodraw=1;
			return 0;
		}
		case 'm': { // change mode
			char m[100];
			if (!data) {
				if (creationstyle&LINESDATA_LINES) { creationstyle=(creationstyle&~LINESDATA_LINES) | LINESDATA_POLYGON; }
				else if (creationstyle&LINESDATA_POLYGON) { creationstyle=(creationstyle&~LINESDATA_POLYGON) | LINESDATA_SYMPOLY; }
				else if (creationstyle&LINESDATA_SYMPOLY) { creationstyle=(creationstyle&~LINESDATA_SYMPOLY) | LINESDATA_LINES; }
				strcpy(m,"Create: ");
				switch(creationstyle&(LINESDATA_LINES|LINESDATA_POLYGON|LINESDATA_SYMPOLY)) {
					case LINESDATA_LINES: strcat(m,"Lines"); break;
					case LINESDATA_POLYGON: strcat(m,"Polygon"); break;
					case LINESDATA_SYMPOLY: strcat(m,"Symmetric Polygon"); break;
				}
				app->postmessage(m);
			} else {
				if (data->style&LINESDATA_LINES) { data->style=(data->style&~LINESDATA_LINES) | LINESDATA_POLYGON; }
				else if (data->style&LINESDATA_POLYGON) { data->style=(data->style&~LINESDATA_POLYGON) | LINESDATA_SYMPOLY; }
				else if (data->style&LINESDATA_SYMPOLY) { data->style=(data->style&~LINESDATA_SYMPOLY) | LINESDATA_LINES; }
				strcpy(m,"Lines: ");
				switch(data->style&(LINESDATA_LINES|LINESDATA_POLYGON|LINESDATA_SYMPOLY)) {
					case LINESDATA_LINES: strcat(m,"Lines"); break;
					case LINESDATA_POLYGON: strcat(m,"Polygon"); break;
					case LINESDATA_SYMPOLY: strcat(m,"Symmetric Polygon"); break;
				}
				app->postmessage(m);
			}
		} return 0; // case 'm'
		case 'r': creationstyle^=LINESDATA_RECT;
			if (creationstyle&LINESDATA_RECT) {
				app->postmessage("Create rectangles");
				creationstyle&=~(LINESDATA_LINES|LINESDATA_SYMPOLY);
				creationstyle|=LINESDATA_POLYGON;
				Setupcreation(4,1);
			} else {
				app->postmessage("Don't make rectangles");
				Setupcreation(creationsides,creationturns);
			}
			return 0;
		case 'b': { // ***start a new one
				deletedata();
				needtodraw=1;
			} return 0;
	}
	return 1; 
}

int LinesInterface::CharRelease(unsigned int ch,unsigned int state) 
{ //***
	switch(ch) {
		case LAX_Shift: { // shift
			//***not do square bounds
		} break;
//		case LAX_Control: { // cntl
//			*** start moving wildpoint
//		} break;
	}
	return 1; 
}


} // namespace LaxInterfaces

