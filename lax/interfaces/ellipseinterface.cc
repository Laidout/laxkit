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
//    Copyright (C) 2004-2006,2011,2014 by Tom Lechner
//


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/ellipseinterface.h>
#include <lax/language.h>

using namespace Laxkit;
using namespace LaxFiles;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {



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
 * start is the starting angle in radians.
 * end   is the ending angle in radians.
 */


EllipseData::EllipseData()
{
	center=flatpoint(0,0);
	x=flatpoint(1,0);
	y=flatpoint(0,1);
	a=b=1;
	style=0; 
	start=0;
	end=0;
	inner_r = 0;
	outer_r = 1;
	//end=2*3.14159265358979; 
}

EllipseData::~EllipseData()
{
}

SomeData *EllipseData::duplicate(SomeData *dup)
{
	EllipseData *p=dynamic_cast<EllipseData*>(dup);
	if (!p && !dup) return NULL;

	if (!dup) {
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject("EllipseData"));
		if (dup) {
			p=dynamic_cast<EllipseData*>(dup);
		}
	} 

	if (!p) {
		p=new EllipseData();
		dup=p;
	}

	p->center  = center;
	p->x       = x;
	p->y       = y;
	p->a       = a;
	p->style   = style;
	p->start   = start;
	p->end     = end;
	p->inner_r = inner_r;
	p->outer_r = outer_r;
	p->style   = style;

	return dup;
}

bool EllipseData::GetStyle(unsigned int s)
{ return style&s; }

void EllipseData::SetStyle(unsigned int s, bool on)
{
	if (on) style|=s;
	else style&=~s;
}

 // *** this is a trashy way to do it, howzabout using max/min
void EllipseData::FindBBox()
{
	flatpoint pp = center + a*x;
	maxx = minx = pp.x;
	maxy = miny = pp.y;

	for (double t=0; t<2*3.1415926535; t+=2*3.1415926535/20) {
		pp=center + (a*x) + (a*y);
		if (pp.x<minx) minx = pp.x;
		else if (pp.x>maxx) maxx = pp.x;
		if (pp.y<miny) miny = pp.y;
		else if (pp.y>maxy) maxy = pp.y;
	}
}

void EllipseData::usefocus(flatpoint f1,flatpoint f2,double c) //c==-1
{
	if (c==-1) { if (a>b) c=2*a; else c=2*b; }
	center = (f1+f2)/2;
	x = (f1-f2);
	double f = norm(x);
	if (c<f) c = f;
	if (f) x /= f;
	y = transpose(x);
	a = 2*c;
	b = sqrt(a*a-f*f/4);
}

flatpoint EllipseData::getpoint(EllipsePoints c, bool transform_to_parent)
{
	flatpoint p((maxx+minx)/2, (maxy+miny)/2);

	if (c==ELLP_Focus1 || c==ELLP_Focus2) {
		double a2=a*a, b2=b*b;
		if (a2 > b2) {
			if (c==ELLP_Focus1) p= center + sqrt(a2 - b2) * x;
			else  p= center - sqrt(a2 - b2) * x;
		} else {
			if (c==ELLP_Focus1) p= center + sqrt(b2 - a2) * y;
				else   p= center - sqrt(b2 - a2) * y;
		}

	} else if (c==ELLP_Center) {      p= center;
	} else if (c==ELLP_Right) {       p= center + a*x; 
	} else if (c==ELLP_BottomRight) { p= center + a*x - b*y; 
	} else if (c==ELLP_Bottom) {      p= center       - b*y; 
	} else if (c==ELLP_BottomLeft) {  p= center - a*x - b*y;
	} else if (c==ELLP_Left) {        p= center - a*x; 
	} else if (c==ELLP_TopLeft) {     p= center - a*x + b*y; 
	} else if (c==ELLP_Top) {         p= center       + b*y; 
	} else if (c==ELLP_TopRight) {    p= center + a*x + b*y;

	} else if (c==ELLP_StartAngle) {  p= center + rotate(a*x,start);
	} else if (c==ELLP_EndAngle)   {  p= center + rotate(a*x,end);

	} else if (c==ELLP_XRadius) { p= center + a*x;
	} else if (c==ELLP_YRadius) { p= center + b*y;

	} else if (c==ELLP_OuterRadius) { p=center + outer_r*cos(start)*x + outer_r*sin(start)*y;
	} else if (c==ELLP_InnerRadius) { p=center + inner_r*cos(start)*x + inner_r*sin(start)*y;
	}

	if (transform_to_parent) return transformPoint(p);
	return p;
}

void EllipseData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *EllipseData::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	if (!att) att=new Attribute();

	//unsigned int style;
	//double start,end;
	//double outer_r, inner_r;
	//double a,b;
	//flatpoint center,x,y; //center, x and y axis (in addition to this->m())
	//LineStyle linestyle;

	if (what==-1) { 
		att->push("id", "somename #String id");
		att->push("start","0 #starting angle, default radians");
		att->push("end","0 #ending angle. If equal to start, assume full circle");
		att->push("inner_r","0 #Inner radius, as fraction of default radius, if any");
		att->push("outer_r","1 #Outer radius, as fraction of default radius, if any");
		att->push("a","1 #x axis radius");
		att->push("b","1 #y axis radius");
		att->push("center","(0,0) #position of ellipse origin (not object origin)");
		att->push("x","(1,0) #x axis of ellipse (not object x axis)");
		att->push("y","(0,1) #y axis of ellipse (not object y axis)");
		att->push("linestyle"," #(optional) style to draw this ellipse");
		att->push("flags","circle #tags for how to edit this object");
		return att;
	}

	char scratch[100];

	att->push("id", Id());

	if (style & ELLIPSE_ISCIRCLE) att->push("flags", "circle");

	att->push("start", start);
	att->push("end", end);
	att->push("inner_r", inner_r);
	att->push("outer_r", outer_r);
	att->push("a", a);
	att->push("b", b);

	sprintf(scratch,"(%.10g, %.10g)", center.x, center.y);
	att->push("center", scratch);
	sprintf(scratch,"(%.10g, %.10g)", x.x, x.y);
	att->push("x", scratch);
	sprintf(scratch,"(%.10g, %.10g)", y.x, y.y);
	att->push("y", scratch);

	//att->push("linestyle", ); 

	return att;
}

void EllipseData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;

	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) Id(value);

		} else if (!strcmp(name,"start")) {
			DoubleAttribute(value, &start);

		} else if (!strcmp(name,"end")) {
			DoubleAttribute(value, &end);

		} else if (!strcmp(name,"inner_r")) {
			DoubleAttribute(value, &inner_r);

		} else if (!strcmp(name,"outer_r")) {
			DoubleAttribute(value, &outer_r);

		} else if (!strcmp(name,"a")) {
			DoubleAttribute(value, &a);

		} else if (!strcmp(name,"b")) {
			DoubleAttribute(value, &b);

		} else if (!strcmp(name,"center")) {
			FlatvectorAttribute(value, &center);

		} else if (!strcmp(name,"x")) {
			FlatvectorAttribute(value, &x);

		} else if (!strcmp(name,"y")) {
			FlatvectorAttribute(value, &y); 

		} else if (!strcmp(name,"linestyle")) {
			// ***

		} else if (!strcmp(name,"flags")) {
			// ***
		}
	}
}

//----------------------------- EllipseInterface ------------------------

/*! \class EllipseInterface
 * \ingroup interfaces
 * \brief Interface for EllipseData objects.
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
 * \todo *** must be able to fill also?
 */

//! Constructor.
/*! This keeps an internal RectInterface. Does not push onto viewport.
 */
EllipseInterface::EllipseInterface(anInterface *nowner, int nid,Displayer *ndp)
  : anInterface(nowner, nid,ndp),
	rinterf(0,ndp)
{
	linestyle.color.rgbf(1.,0,0);
	controlcolor.rgbf(.5,.5,.5,1);
	data=NULL;
	eoc=NULL;

	rdata.style=RECT_CANTCREATE;
	rinterf.style=RECT_CANTCREATE|RECT_INVISIBLECENTER;
	rinterf.UseThis(&rdata,0);
	rinterf.Dp(ndp);
	inrect=false;

	showdecs=1;
	curpoint=ELLP_None;
	creationstyle=0;
	createfrompoint=0; // 0=lbd=ulc, 1=lbd=center
	createangle=0;
	createx=flatpoint(1,0);
	createy=flatpoint(0,1);

	mode = ELLP_None;
	
	needtodraw=1;

	sc=NULL;
}

//! Destructor, deletes rinterf.
EllipseInterface::~EllipseInterface()
{
	if (sc) sc->dec_count();
	DBG cerr <<"----in EllipseInterface destructor"<<endl;
}

void EllipseInterface::Dp(Laxkit::Displayer *ndp)
{
	anInterface::Dp(ndp);
	rinterf.Dp(ndp);
}

const char *EllipseInterface::Name()
{
	return _("Ellipse");
}

//! Return new EllipseInterface.
/*! If dup!=NULL and it cannot be cast to ImageInterface, then return NULL.
 */
anInterface *EllipseInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new EllipseInterface(NULL,id,NULL);
	else if (!dynamic_cast<EllipseInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

//! Accepts LineStyle. For LineStyle, just copies over width and color.
int EllipseInterface::UseThis(anObject *nobj,unsigned int mask)
{
	if (!nobj) return 0;

	if (dynamic_cast<LineStyle *>(nobj)) { 
		DBG cerr <<"Ellipse new color stuff"<<endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);
		if (mask&GCForeground) { if (data) data->linestyle.color=nlinestyle->color; else linestyle.color=nlinestyle->color; }
		if (mask&GCLineWidth)  { if (data) data->linestyle.width=nlinestyle->width; else linestyle.width=nlinestyle->width; }
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
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	curpoint=ELLP_None;
	inrect=false;
	return 0;
}

void EllipseInterface::Clear(SomeData *d)
{
	if (d==data) deletedata();
}

void EllipseInterface::deletedata()
{
	if (data) data->dec_count();
	data=NULL;
	curpoint=ELLP_None;
	inrect=false;
}

//! Make rdata reflect data
void EllipseInterface::rectify()
{
	if (!data) return;

	rdata.xaxis(data->x);
	rdata.yaxis(data->y);
	rdata.maxx = rdata.minx = data->a;
	rdata.maxy = rdata.miny = data->b;
	rdata.origin(data->center);
	rdata.centercenter();
}

//! Make data reflect rdata.
void EllipseInterface::erectify()
{
	if (!data) return;

	data->x = rdata.xaxis();
	data->y = rdata.yaxis();
	data->center = rdata.origin();
	data->a = (rdata.maxx-rdata.minx)/2;
	data->b = (rdata.maxy-rdata.miny)/2;
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
	if (!dp || (!needtodraw && !rinterf.needtodraw)) return 0;
	if (!data) {
		if (needtodraw) needtodraw=0;
		return 1;
	}

	//cout <<"  EllipseRefresh";
		
	dp->NewFG(&data->linestyle.color);

	 // draw just ellipse
	dp->LineAttributes(data->linestyle.width,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
	dp->drawfocusellipse(getpoint(ELLP_Focus1),getpoint(ELLP_Focus2),
			2*(fabs(data->a)>fabs(data->b)?data->a:data->b),data->start,data->end,1);
	dp->LineAttributes(0,LineSolid,data->linestyle.capstyle,data->linestyle.joinstyle);
	
	 // draw control points;
	if (showdecs) { 
		dp->NewFG(&controlcolor);
		flatpoint p;

		 // angle points
		p=dp->realtoscreen(getpoint(ELLP_StartAngle));
		dp->drawpoint((int)p.x,(int)p.y, 3,0);
		p=dp->realtoscreen(getpoint(ELLP_EndAngle));
		dp->drawpoint((int)p.x,(int)p.y, 3,0);
		
		 // open foci
		p=dp->realtoscreen(getpoint(ELLP_Focus1));
		dp->drawpoint((int)p.x,(int)p.y, 5,0); // focus1
		if (!data->GetStyle(ELLIPSE_ISCIRCLE)) { // is not circle so draw second focus
			p=dp->realtoscreen(getpoint(ELLP_Focus2));
			dp->drawpoint((int)p.x,(int)p.y, 5,0); // focus2
		}
		
		 // curpoint
		if (curpoint>=ELLP_Focus1) {
			flatpoint fp=dp->realtoscreen(getpoint(curpoint));
			dp->drawpoint((int)fp.x,(int)fp.y,(curpoint!=ELLP_Focus1 && curpoint!=ELLP_Focus2)?3:5,1);  // draw curpoint
		}

		 // angles
		if (curpoint==ELLP_StartAngle || curpoint==ELLP_EndAngle) {
			dp->LineAttributes(0,LineDoubleDash, LAXCAP_Butt, LAXJOIN_Round);
			dp->drawline(data->center,dp->screentoreal(hover_x,hover_y));
			dp->drawline(data->center,getpoint(curpoint==ELLP_StartAngle?ELLP_EndAngle:ELLP_StartAngle)); // draw line of other angle point
			dp->LineAttributes(0,LineSolid,LAXCAP_Butt,LAXJOIN_Round);
		}
		 
		 // draw dotted box
		//cout<<"-b"<<rinterf.needtodraw;
		rinterf.Dp(dp);
		rinterf.needtodraw=1;
		rinterf.Refresh();
	}
	needtodraw=0;
	//DBG cerr<<endl;
	return 0;
}

flatpoint EllipseInterface::getpoint(EllipsePoints c, bool inparent)
{
	if (!data || c==ELLP_WildPoint) return dp->screentoreal(hover_x,hover_y);
	return data->getpoint(c,inparent);
}

/*! Note, never returns ELLP_WildPoint.
 * Picks closest within a distance, but not the standard rect points. Those have
 * to be scanned for elsewhere with rectinterface.scan().
 */
EllipsePoints EllipseInterface::scan(int x,int y) //*** only checks points, no online jazz
{
	if (!data) return ELLP_None;

	EllipsePoints closest=ELLP_None;

	flatpoint p,p2;
	p=screentoreal(x,y);
	double d=5/dp->Getmag(),dd;
	DBG cerr <<"scan d="<<d<<"(x,y)="<<p.x<<','<<p.y<<endl;
	d*=d;

	 // scan for control points
	int c;
	for (c=(int)ELLP_WildPoint-1; c>=(int)ELLP_Focus1; c--) {
		p2=getpoint((EllipsePoints)c);
		dd=(p2.x-p.x)*(p2.x-p.x)+(p2.y-p.y)*(p2.y-p.y);
		DBG cerr <<"  ellipse scan "<<c<<", d="<<dd<<"  ";
		if (dd<d) {
			d=dd;
			closest=(EllipsePoints)c;
		}
	}
	DBG cerr <<endl;
	return closest; // scan never checks a wildpoint
}

int EllipseInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	DBG cerr << "  in ellipse lbd..";

	buttondown.down(d->id,LEFTBUTTON,x,y);
	hover_x=x;
	hover_y=y;

	if (data) {
		if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) { // +^lb move around wildpoint
			flatpoint p=getpoint(ELLP_Focus1),p2=getpoint(ELLP_Focus2);
			data->x=getpoint(ELLP_Focus1)-data->center;
			if (data->x.isZero()==false) data->x/=norm(data->x);
			else data->x=flatpoint(1,0);
			data->y=transpose(data->x);
			double f2=(p-p2)*(p-p2);
			p=  p-screentoreal(x,y);
			p2=p2-screentoreal(x,y);
			double c=sqrt(p*p)+sqrt(p2*p2);
			data->a=c/2; // automatically puts major axis on x
			data->b=sqrt(data->a*data->a - f2);
			rectify();
			curpoint=ELLP_WildPoint;
			needtodraw|=1;
			return 0;

		} else if ((state&LAX_STATE_MASK)==ControlMask) { // ^lb focus or end angle
			EllipsePoints c=scan(x,y);
			if (c==ELLP_Focus1 || c==ELLP_Focus2) { // remove circle restriction
				curpoint=c;
				data->SetStyle(ELLIPSE_ISCIRCLE, false);
				needtodraw|=2;
				return 0;
			}

		} else if ((state&LAX_STATE_MASK)==ShiftMask) { // +lb start angle
	
		} else if ((state&LAX_STATE_MASK)==0) { // straight click
			EllipsePoints c=scan(x,y);
			if (c!=ELLP_None) { // scan found one...
				curpoint=c;
				needtodraw|=2;
				return 0;
			}
		}

		if (rinterf.LBDown(x,y,state,count,d)==0) {
			inrect=true;
			needtodraw=1;
			return 0;
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
		 // found another EllipseData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		data=obj;
		data->inc_count();
		if (eoc) delete eoc;
		eoc=oc->duplicate();

		if (viewport) viewport->ChangeObject(oc,0,true);
		needtodraw=1;
		return 0;

	} else if (c<0) {
		 // If there is some other non-image data underneath (x,y) and
		 // this is not primary, then switch objects, and switch tools to deal
		 // with that object.
		//******* need some way to transfer the LBDown to the new tool
		if (!primary && c==-1 && viewport->ChangeObject(oc,1,true)) {
			buttondown.up(d->id,LEFTBUTTON);
			return 1;
		}
	}

	 // To be here, must want brand new data plopped into the viewport context
	if (viewport) viewport->ChangeContext(x,y,NULL);

	EllipseData *ndata=NULL; //creates with 1 count
	ndata = dynamic_cast<EllipseData *>(somedatafactory()->NewObject(LAX_ELLIPSEDATA));
	if (!ndata) ndata=new EllipseData;

	ndata->linestyle = linestyle;
	if (viewport) {
		oc=NULL;
		viewport->NewData(ndata,&oc);//viewport adds only its own counts
		if (eoc) delete eoc;
		if (oc) eoc=oc->duplicate();
	}

	//ndata->dec_count(); // interface should directly cause only 1 count
	data = ndata;

	curpoint = ELLP_None;
	flatpoint p = screentoreal(x,y);
	data->style = creationstyle; 
	data->center = p;
	data->x = createx;
	data->y = createy;
	data->a = data->b = 0;
	data->FindBBox();
	createp = p;
	rectify();

	inrect=true;
	rinterf.LBDown(x,y,state,count,d); 

	needtodraw = 1;
	DBG cerr <<"..ellipselbd done   ";

	return 0;
}

int EllipseInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);

	if (inrect) {
		inrect=false;
		rinterf.LBUp(x,y,state,d);
		needtodraw=1;
		return 0;
	}

	if (curpoint==ELLP_WildPoint) { curpoint=ELLP_None; needtodraw|=2; }
	needtodraw=1;
	return 0;
}

int EllipseInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse) 
{
	hover_x=x;
	hover_y=y;

	if (!buttondown.any() || !data) return 1;

	int lx,ly;
	buttondown.move(mouse->id, x,y, &lx,&ly);

	DBG cerr <<" mcp:"<<curpoint<<"\n";
	if (inrect) { // is doing box modifications
		rinterf.MouseMove(x,y,state,mouse);
		erectify();
		needtodraw=1;
		return 0;
	}

	flatpoint d = screentoreal(x,y)-screentoreal(lx,ly);

	if (curpoint!=ELLP_None) {
		if (curpoint==ELLP_Focus1 || curpoint==ELLP_Focus1) {
			flatpoint p1=getpoint(ELLP_Focus1);
			flatpoint p2=getpoint(ELLP_Focus2);

			if (data->style & ELLIPSE_ISCIRCLE) { p2+=d; p1+=d;} 
			else if (curpoint==ELLP_Focus2) p2+=d; else p1+=d; 
			data->usefocus(p1,p2,-1); //*** doesn't deal with non-whole ellipse, the segment jumps around

			rectify();
			needtodraw|=2; 

		} else if (curpoint==ELLP_StartAngle) { // start angle
			//***
			//needtodraw!=1;

		} else if (curpoint==ELLP_EndAngle) { // end angle
			//***
			//needtodraw!=1;

		} else if (curpoint==ELLP_WildPoint) { // wildpoint, move point leaves f1,f2 change c
			flatpoint p,p2;
			p= getpoint(ELLP_Focus1) - screentoreal(x,y);
			p2=getpoint(ELLP_Focus2) - screentoreal(x,y);
			data->usefocus(getpoint(ELLP_Focus1),getpoint(ELLP_Focus2),sqrt(p*p)+sqrt(p2*p2));
			rectify();
			needtodraw|=1;
		} 

		hover_x=x; hover_y=y;
	}
	needtodraw|=2;
	return 0;
}

/*! \todo maybe combine rectinterface shortcuts with ellipse, since it is a permanent child?
 */
int EllipseInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{
	 //check shortcuts
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	
	if (inrect) {
		if (rinterf.CharInput(ch,buffer,len,state,d)==0) return 0;
	}


	return 1; 
}


int EllipseInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb) 
{
	if (inrect) {
		return rinterf.KeyUp(ch,state,kb);
	}

	if (ch==LAX_Shift) { // shift
		if (!buttondown.any() || curpoint==ELLP_None) return 1;

		data->SetStyle(ELLIPSE_ISCIRCLE, false); // toggle off circle
		return 0;
	}

	return 1; 
}


enum EllipseShortcutActions {
	ELLIPSEA_ToggleDecs,
	ELLIPSEA_ToggleCircle,
	ELLIPSEA_MAX
};

Laxkit::ShortcutHandler *EllipseInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;


	sc=new ShortcutHandler(whattype());

	sc->Add(ELLIPSEA_ToggleDecs,  'd',0,0,        "ToggleDecs",  _("Toggle decorations"),NULL,0);
	sc->Add(ELLIPSEA_ToggleCircle,'c',0,0,        "ToggleCircle",  _("Toggle editing as a circle"),NULL,0);

	//sc->Add(PATHIA_WidthStepR,        'W',ControlMask|ShiftMask,0,"WidthStepR", _("Change how much change for width changes"),NULL,0);
	//sc->AddShortcut(LAX_Bksp,0,0, PATHIA_Delete);

	manager->AddArea(whattype(),sc);
	return sc;
}

int EllipseInterface::PerformAction(int action)
{
	if (action==ELLIPSEA_ToggleDecs) {
		showdecs=!showdecs;
		needtodraw=1;
		return 0;

	} else if (action==ELLIPSEA_ToggleCircle) {
		if (!data) return 0;
		bool circle=data->GetStyle(ELLIPSE_ISCIRCLE);
		data->SetStyle(ELLIPSE_ISCIRCLE, !circle);
		needtodraw=1;
		return 0;
	}

	return 1;
}



} // namespace LaxInterfaces 

