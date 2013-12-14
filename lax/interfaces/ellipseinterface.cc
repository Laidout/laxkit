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
#include <lax/interfaces/ellipseinterface.h>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {

//#define ELLIPSES_ISCIRCLE 1

// x+---->
//y 1     8     7
//+ 2  11 9 10  6
//| 3     4     5
//|
//10 focus 1
//11 focus 2
//12 start angle
//13 end angle
//14 is wildpoint
//1-9 box points from rect

/*! \class EllipseData
 * \ingroup interfaces
 * \brief Data class for EllipseInterface.
 * 
 * This should be modified so that the center is always at the space origin, and
 * the SomeData min/max/x/y bounds hold the major and minor axes.
 *
 * Equation of an ellipse:
 * \f$(x/a)^2 + (y/b)^2 = 1\f$
 *
 * s is the starting angle in radians.
 * e is the ending angle in radians.
 */
//class EllipseData : public SomeData
//{
//  protected:
//  	
//  public:
//	unsigned int style;
//	double s,e,a,b;  //'width'=2*a, 'height'=2*b
//	flatpoint center,x,y;
//	LineStyle linestyle;
//	EllipseData();
//	virtual void usefocus(flatpoint f1,flatpoint f2,double c=-1);
//	const char *whattype() { return "EllipseData"; }
//	void FindBBox();
//	flatpoint focus1();
//	flatpoint focus2();
//};

EllipseData::EllipseData()
{
	center=flatpoint(0,0);
	x=flatpoint(1,0);
	y=flatpoint(0,1);
	a=b=0;
	style=0; 
	s=0;
	e=2*3.14159265358979; 
}

 // *** this is a trashy way to do it, howzabout using max/min
void EllipseData::FindBBox()
{
	flatpoint pp=center + a*x;
	maxx=minx=pp.x;
	maxy=miny=pp.y;
	for (double t=0; t<2*3.1415926535; t+=2*3.1415926535/20) {
		pp=center + (a*x) + (a*y);
		if (pp.x<minx) minx=pp.x;
		else if (pp.x>maxx) maxx=pp.x;
		if (pp.y<miny) miny=pp.y;
		else if (pp.y>maxy) maxy=pp.y;
	}
}

void EllipseData::usefocus(flatpoint f1,flatpoint f2,double c) //c==-1
{
	if (c==-1) if (a>b) c=2*a; else c=2*b;
	center=(f1+f2)/2;
	x=(f1-f2);
	double f=norm(x);
	if (c<f) c=f;
	if (f) x/=f;
	y=transpose(x);
	a=2*c;
	b=sqrt(a*a-f*f/4);
}

//----------------------------- EllipseInterface ------------------------

/*! \class EllipseInterface
 * \ingroup interfaces
 * \brief *** fix me!Interface for EllipseData objects.
 *  
 * <pre>
 *  old:
 *   TO DO:
 *  
 *   draw only whats necessary
 *   ellipses start==end==0 means whole
 *   start/end angles
 *   move focus don't work circle kind of flaky/ out of phase with rect
 *  
 *   from inkscape/sodipodi:
 *    move start/end inside makes chord, outside makes pie
 * </pre>
 * 
 * \todo *** totally broken, needs some serious rewrite.. 
 * \todo *** must be able to fill also?
 */

//! Constructor.
/*! *** this keeps an internal RectInterface. Does not push onto viewport.
 */
EllipseInterface::EllipseInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	linestyle.color=app->rgbcolor(255,0,0);
	controlcolor=38066; // defaults to white, change right after creation otherwise
	data=NULL;
	rinterf=new RectInterface(0,dp);
	rdata.style=RECT_CANTCREATE;
	rinterf->style=RECT_CANTCREATE|RECT_INVISIBLECENTER;
	rinterf->UseThis(&rdata,0);
	dataislocal=0;
	showdecs=1;
	curpoint=0;
	buttondown=0;
	creationstyle=0;
	createfrompoint=0; // 0=lbd=ulc, 1=lbd=center
	createangle=0;
	createx=flatpoint(1,0);
	createy=flatpoint(0,1);
	showdecs=1;
	mask=ButtonPressMask|ButtonReleaseMask|PointerMotionMask|KeyPressMask|KeyReleaseMask;
	buttonmask=Button1Mask;
	
	needtodraw=1;
}

//! Destructor, deletes rinterf.
EllipseInterface::~EllipseInterface()
{ 	
	DBG cerr <<"----in EllipseInterface destructor"<<endl;
	delete rinterf; //*** maybe just create this when InterfaceOn???
}

//! Return new EllipseInterface.
/*! If dup!=NULL and it cannot be cast to ImageInterface, then return NULL.
 */
anInterface *EllipseInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new EllipseInterface(id,NULL);
	else if (!dynamic_cast<EllipseInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

//! Accepts LineStyle or EllipseData. For LineStyle, just copies over width and color.
/*! \todo **** returns 1 with LineStyle, but does not take over it.. is that ok??
 */
int EllipseInterface::UseThis(anObject *nobj,unsigned int mask)
{
	if (!nobj) return 0;
	if (dynamic_cast<EllipseData *>(nobj)) {
		if (data) deletedata();
		EllipseData *ndata=dynamic_cast<EllipseData *>(nobj);
		***viewport->newData(ndata,0); //incs count by 1
		data=ndata;
		rectify();
		return 1;
	} else if (dynamic_cast<LineStyle *>(nobj)) { 
		DBG cerr <<"Ellipse new color stuff"<<endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);
		if (mask&GCForeground) if (data) data->linestyle.color=nlinestyle->color; else linestyle.color=nlinestyle->color;
		if (mask&GCLineWidth) if (data) data->linestyle.width=nlinestyle->width; else linestyle.width=nlinestyle->width;
		needtodraw=1;
		return 1;
	}
	return 0;
}

/*! \todo *** should push the RectInterface onto the viewport interfaces stack...
 */
int EllipseInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! should  Pop the RectInterface onto the viewport interfaces stack.
 */
int EllipseInterface::InterfaceOff()
{
	Clear();
	showdecs=0;
	needtodraw=1;
	curpoint=0;
	return 0;
}

void EllipseInterface::deletedata()
{
	if (data) data->dec_count();
	data=NULL;
}

//! Make rdata reflect data
void EllipseInterface::rectify()
{
	if (!data) return;
	rdata.x=data->x;
	rdata.y=data->y;
	rdata.w=2*data->a;
	rdata.h=2*data->b;
	rdata.p=data->center - data->a*data->x - data->b*data->y;
	rdata.centercenter();
}

//! Make data reflect rdata.
void EllipseInterface::erectify()
{
	if (!data) return;
	data->x=rdata.x;
	data->y=rdata.y;
	data->a=rdata.w/2;
	data->b=rdata.h/2;
	data->center=rdata.p + data->a*data->x + data->b*data->y;
}

int EllipseInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
	if (!ndata || dynamic_cast<EllipseData *>(ndata)==NULL) return 1;
	EllipseData *bzd=data;
	data=dynamic_cast<EllipseData *>(ndata);
	int td=showdecs,ntd=needtodraw;
	showdecs=0;
	needtodraw=1;
	
	Refresh();

	needtodraw=ntd;
	showdecs=td;
	data=bzd;
	return 1;
}

int EllipseInterface::Refresh()
{
	if (!dp || (!needtodraw && !rinterf->needtodraw)) return 0;
	if (!data) {
		if (needtodraw) needtodraw=0;
		return 1;
	}

	//cout <<"  EllipseRefresh";
		
	dp->NewFG(data->linestyle.color);

	 // draw just ellipse
	dp->LineAttributes(data->linestyle.width,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
	dp->drawfocusellipse(getpoint(10),getpoint(11),2*(fabs(data->a)>fabs(data->b)?data->a:data->b),data->s,data->e,1);
	dp->LineAttributes(0,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
	
	 // draw control points;
	if (showdecs) { 
		dp->NewFG(controlcolor);
		flatpoint p;

		 // angle points
		for (int c=12; c<14; c++) {
			p=dp->realtoscreen(getpoint(c));
			dp->draw((int)p.x,(int)p.y,3);
		}
		
		 // open foci
		p=dp->realtoscreen(getpoint(10));
		dp->draw((int)p.x,(int)p.y,5); // focus1
		if (!(data->style&ELLIPSES_ISCIRCLE)) { // is not circle so draw second focus
			p=dp->realtoscreen(getpoint(11));
			dp->draw((int)p.x,(int)p.y,5); // focus2
		}
		
		 // curpoint
		if (curpoint>9) {
			flatpoint fp=dp->realtoscreen(getpoint(curpoint));
			dp->drawf((int)fp.x,(int)fp.y,(curpoint!=10 && curpoint!=11)?3:5);  // draw curpoint
		}

		 // angles
		if (curpoint==12 || curpoint==13) {
			dp->LineAttributes(0,LineDoubleDash, LAXCAP_Butt, LAXJOIN_Miter);
			dp->drawrline(data->center,dp->screentoreal(mx,my));
			dp->drawrline(data->center,getpoint(curpoint==7?8:7)); // draw line of other angle point
			dp->LineAttributes(0,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
		}
		 
		 // draw dotted box
		//cout<<"-b"<<rinterf->needtodraw;
		rinterf->needtodraw=1;
		rinterf->Refresh();
	}
	needtodraw=0;
	//DBG cerr<<endl;
	return 0;
}

/*! <pre>
 *   0=none
 *   1=-x-y 
 *   2=-x
 *   3=-xy
 *   4=y
 *   5=xy
 *   6=x
 *   7=x-y
 *   8=-y
 *   9=0
 *   10=f1
 *   11=f2
 *   12=s
 *   13=e
 *   14=wp 
 *  </pre>
 */
flatpoint EllipseInterface::getpoint(int c)
{ 
	if (c>0 && c<10) return rinterf->getpoint(c);
	switch (c) {
		case 10: // f1
		case 11: { // f2 
			double a2=data->a*data->a, b2=data->b*data->b;
			if (a2 > b2) 
			if (c==10) return data->center + sqrt(a2 - b2) * data->x;
				else  return data->center - sqrt(a2 - b2) * data->x;
			else if (c==10) return data->center + sqrt(b2 - a2) * data->y;
					else   return data->center - sqrt(b2 - a2) * data->y;
		}
		case 12: // start angle
			return data->center + rotate(data->a*data->x,data->s);
		case 13: // end angle
			return data->center + rotate(data->a*data->x,data->e);
		case 14: // wild point
			return dp->screentoreal(mx,my);
//		case 6: // x
//			return data->center + data->a*data->x;
//		case 7: // x-y
//			return data->center + data->a*data->x - data->b*data->y;
//		case 8: // -y
//			return data->center - data->b*data->y;
//		case 9: // -x-y
//			return data->center - data->a*data->x - data->b*data->y;
//		case 10: // -x
//			return data->center - data->a*data->x;
//		case 11: // -x+y
//			return data->center - data->a*data->x + data->b*data->y;
//		case 12: // +y
//			return data->center + data->b*data->y;
//		case 13: // xy
//			return data->center + data->a*data->x + data->b*data->y;
	}
	return flatpoint();
}

/*! \todo *** doesn't pick wild,start,end,  picks closest within a distance
 */
int EllipseInterface::scan(int x,int y) //*** only checks points, no online jazz
{
	if (!data) return 0;
	int closest=0;
	if (closest=rinterf->scan(x,y),closest>0) return closest;
	flatpoint p,p2;
	p=screentoreal(x,y);
	double d=5/dp->Getmag(),dd;
	DBG cerr <<"scan d="<<d<<"(x,y)="<<p.x<<','<<p.y<<endl;
	d*=d;
	 // scan for control points
	int c;
	for (c=13; c>9; c--) {
		//if (c==5) continue; // scan never looks for wildpoint
		p2=getpoint(c);
		dd=(p2.x-p.x)*(p2.x-p.x)+(p2.y-p.y)*(p2.y-p.y);
		DBG cerr <<"  scan "<<c<<", d="<<dd<<"  ";
		if (dd<d) {
			d=dd;
			closest=c;
		}
	}
	DBG cerr <<endl;
	return closest; // scan never checks a wildpoint
}

int EllipseInterface::LBDown(int x,int y,unsigned int state,int count)
{
	DBG cerr << "  in ellipse lbd..";
	buttondown|=1;
	mx=x;
	my=y;
	if (data) {
		if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) { // +^lb move around wildpoint
			flatpoint p=getpoint(1),p2=getpoint(2);
			data->x=getpoint(1)-data->center;
			data->x/=norm(data->x);
			data->y=transpose(data->x);
			double f2=(p-p2)*(p-p2);
			p=  p-screentoreal(x,y);
			p2=p2-screentoreal(x,y);
			double c=sqrt(p*p)+sqrt(p2*p2);
			data->a=c/2; // automatically puts major axis on x
			data->b=sqrt(data->a*data->a - f2);
			rectify();
			curpoint=14;
			needtodraw|=1;
			return 0;
		} else if ((state&LAX_STATE_MASK)==ControlMask) { // ^lb focus or end angle
			int c;
			if (c=scan(x,y), c==10 || c==11) { // remove circle restriction
				curpoint=c;
				data->style&=~ELLIPSES_ISCIRCLE;
				needtodraw|=2;
				return 0;
			}
			 // else end angle, warp pointer to current
	//		flatpoint p=realtoscreen(getpoint(13));
	//		XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-x,(int)p.y-y);
	//		mx=(int)p.x;
	//		my=(int)p.y;
	//		curpoint=4;
	//		needtodraw|=2;
	//		return 0;
		} else if ((state&LAX_STATE_MASK)==ShiftMask) { // +lb start angle
	//		flatpoint p=realtoscreen(getpoint(12));
	//		XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-x,(int)p.y-y);
	//		mx=(int)p.x;
	//		my=(int)p.y;
	//		curpoint=3;
	//		needtodraw|=2;
	//		return 0;
		} else if ((state&LAX_STATE_MASK)==0) { // straight click
			int c=scan(x,y); //*** how separate scan box/ellipse
			if (c>0) { // scan found one...
				rinterf->LBDown(x,y,state,count); //*** maybe make sure curpoint stays curpoint: rinterf->selectpoint(int c)
				rinterf->SelectPoint(c);
				curpoint=c;
				needtodraw|=2;
				return 0;
			}
		}
		return 0; // other click with data
	}
	DBG cerr <<"  noellipsepointfound  ";

	 // make new one
	EllipseData *obj=NULL;
	ObjectContext *oc=NULL;
	int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
	if (c>0) obj=dynamic_cast<EllipseData *>(oc->obj);
	if (obj) { 
		 // found another ImageData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		somedata=data=obj;
		dataislocal=0;
		if (viewport) viewport->NewData(obj,&oc); // this incs count
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

	EllipseData *ndata=NULL; //creates with 1 count
	if (somedatafactory) {
		ndata=static_cast<EllipseData *>(somedatafactory->newObject(LAX_ELLIPSEDATA));
	} 
	if (!ndata) ndata=EllipseData;
	ndata->linestyle=linestyle;
	viewport->newData(ndata,0);//calls deletedata() and adds 1 count
	ndata->dec_count(); // interface should directly cause only 1 count
	data=ndata;
	
	curpoint=5;
	flatpoint p=screentoreal(x,y);
	data->style=creationstyle; 
	data->center=p;
	data->x=createx;
	data->y=createy;
	data->a=data->b=0;
	createp=p;
	rectify();
	rinterf->LBDown(x,y,state,count); 
	rinterf->SelectPoint(5);
	DBG CharInput('p',0);//***lists current points
	needtodraw=1;
	return 0;
	DBG cerr <<"..ellipselbd done   ";
}

int EllipseInterface::LBUp(int x,int y,unsigned int state) 
{
	buttondown&=~LEFTBUTTON;
	if (curpoint==14) { curpoint=0; needtodraw|=2; }
	if (curpoint>0 && curpoint<10) rinterf->LBUp(x,y,state);
	needtodraw=1;
	return 0;
}

/*! 
 * \todo *** focus move broken!
 */
int EllipseInterface::MouseMove(int x,int y,unsigned int state) 
{
	if (!(buttondown&LEFTBUTTON) || !data) return 1;
	DBG cerr <<" mcp:"<<curpoint<<"\n";
	if (curpoint>0 && curpoint<10) { // is doing box modifications
		rinterf->MouseMove(x,y,state);
		erectify();
		needtodraw=1; //*** doens't set mx,my.. problem?
		return 0;
	}
	flatpoint d=screentoreal(x,y)-screentoreal(mx,my);
//	flatpoint np=screentoreal(x,y)-data->center;
	if (curpoint>0) { 
		switch(curpoint) {
			case 10: 
			case 11: {
				flatpoint p1=getpoint(10);
				flatpoint p2=getpoint(11);
				if (data->style&ELLIPSES_ISCIRCLE) { p2+=d; p1+=d;} 
				else if (curpoint==11) p2+=d; else p1+=d; 
				data->usefocus(p1,p2,-1); //*** doesn't deal with non-whole ellipse, the segment jumps around
				rectify();
				needtodraw|=2; 
				break;
			}
			case 3: // start angle
				//***
				//needtodraw!=1;
				break;
			case 4: // end angle
				//***
				//needtodraw!=1;
				break;
			case 5: { // wildpoint, move point leaves f1,f2 change c
				flatpoint p,p2;
				p= getpoint(10) - screentoreal(x,y);
				p2=getpoint(11) - screentoreal(x,y);
				data->usefocus(getpoint(10),getpoint(11),sqrt(p*p)+sqrt(p2*p2));
				rectify();
				needtodraw|=1;
				break;
			} 
		}
		mx=x; my=y;
	}
	needtodraw|=2;
	return 0;
}

/*! 
 * <pre>
 * Shift    edit start instead
 * Control  edit end instead
 * Del/Bksp deletes a focus point (makes a circle)
 * 'c'      center the thing
 * 'd'      toggle decorations
 * </pre>
 */
int EllipseInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state) 
{
	if (curpoint>0 && curpoint<10) if (rinterf->CharInput(ch,state)==0) return 0;
	switch(ch) {
		case LAX_Shift: { // shift
			if (curpoint==LAX_Enter) { // switch to start
				curpoint=12;
				 // warp pointer to start point
				flatpoint p=realtoscreen(data->center+rotate(data->a*data->x,data->s));
				XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-mx,(int)p.y-my);
				mx=(int)p.x;
				my=(int)p.y;
				needtodraw|=1;
				return 0;
			}
		} break;
		case LAX_Control: { // cntl
			if (curpoint==12) { // switch to end
				curpoint=13;
				flatpoint p=realtoscreen(data->center+rotate(data->a*data->x,data->e));
				XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-mx,(int)p.y-my);
				mx=(int)p.x;
				my=(int)p.y;
				needtodraw|=1;
				return 0;
			}
		} break;
		case LAX_Del: // delete
		case LAX_Bksp: { // backspace
			if (curpoint==10 || curpoint==11) { 
				data->center=getpoint(curpoint==10?11:10); 
				data->a=data->b; 
				data->style|=ELLIPSES_ISCIRCLE; 
				rectify();
				needtodraw|=2; 
				return 0; 
			}
		} break;
//		case LAX_Left: { // left //***unmodified from before rect
//			if (curpoint<6) return 0;
//			curpoint--;
//			if (curpoint<6) curpoint=13;
//			flatpoint p=realtoscreen(getpoint(curpoint));
//			XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-mx,(int)p.y-my);
//			mx=(int)p.x;
//			my=(int)p.y;
//			needtodraw|=2;
//			return 0;
//		}
//		case LAX_Right: { // right
//			if (curpoint<6) return 0;
//			curpoint++;
//			if (curpoint>13) curpoint=6;
//			flatpoint p=realtoscreen(getpoint(curpoint));
//			XWarpPointer(app->dpy,None,None,0,0,0,0,(int)p.x-mx,(int)p.y-my);
//			mx=(int)p.x;
//			my=(int)p.y;
//			needtodraw|=2;
//			return 0;
//		}
		//case LAX_Up: // up
		//case LAX_Down: // down
			break;
		case 'p': { //***list points, debug only
			DBG flatpoint p;
			DBG cerr <<"\nEllipse Curpoint="<<curpoint<<endl;
			DBG for (int c=1; c<15; c++) {
			DBG 	p=getpoint(c);
			DBG 	cerr <<" "<<c<<":"<<p.x<<","<<p.y<<endl;
			DBG }
		} break;
		case ' ':
			if (curpoint>9 || !buttondown) return 1;
		case 'c': {
			data->x=createx=flatpoint(1,0);
			data->y=createy=flatpoint(0,1);
			rectify();
			MouseMove(mx,my,0);
			return 0;
		} break;
		case 'd': {
			showdecs=!showdecs;
			needtodraw=1;
			return 0;
		}
	}
	return 1; 
}

int EllipseInterface::CharRelease(unsigned int ch,unsigned int state) 
{ //*** shift/noshift toggle end/start point
	switch(ch) {
		case LAX_Shift: { // shift
			if (!buttondown || curpoint==0 || curpoint>10) return 1;
			data->style&=~ELLIPSES_ISCIRCLE; // toggle off circle
			return rinterf->CharRelease(ch,state);
		} break;
		//case LAX_Control: { } break; // cntl
	}
	return 1; 
}


} // namespace LaxInterfaces 

