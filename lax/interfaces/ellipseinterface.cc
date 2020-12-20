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


const char *PointName(EllipsePoints p)
{
	switch (p) {
		case ELLP_None:        return _("None");
		// case ELLP_TopLeft: return _("TopLeft");
		// case ELLP_Top: return _("Top");
		// case ELLP_TopRight: return _("TopRight");
		// case ELLP_Left: return _("Left");
		case ELLP_Center:      return _("Center");
		// case ELLP_Right: return _("Right");
		// case ELLP_BottomLeft: return _("BottomLeft");
		// case ELLP_Bottom: return _("Bottom");
		// case ELLP_BottomRight: return _("BottomRight");
		case ELLP_Focus1:      return _("Focus1");
		case ELLP_Focus2:      return _("Focus2");
		case ELLP_StartAngle:  return _("StartAngle");
		case ELLP_EndAngle:    return _("EndAngle");
		case ELLP_XRadius:     return _("XRadius");
		case ELLP_XRadiusN:     return _("XRadiusN");
		case ELLP_YRadius:     return _("YRadius");
		case ELLP_YRadiusN:     return _("YRadiusN");
		case ELLP_OuterRadius: return _("Outer Radius");
		case ELLP_InnerRadius: return _("Inner Radius");
		case ELLP_WildPoint:   return _("Wild point");
		default: {
			DBG cerr << "PointName unknown for: "<<p<<endl;
			return "?";
		}
	}
	return "?";
}


/*! \class EllipseData
 * \ingroup interfaces
 * \brief Data class for EllipseInterface.
 * 
 * This should be modified so that the center is always at the space origin, and
 * the SomeData min/max/x/y bounds hold the major and minor axes.
 *
 * Equation of an ellipse:
 * \f$ (x/a)^2 + (y/b)^2 = 1 \f$
 * \f$ f^2 = | a^2 - b^2 |   \f$
 *
 * f is the length from the center to a focus.
 * start is the starting angle in radians.
 * end   is the ending angle in radians.
 */


EllipseData::EllipseData()
{
	center = flatpoint(0, 0);
	x      = flatpoint(1, 0);
	y      = flatpoint(0, 1);
	a = b     = 1;
	style     = 0;
	start     = 0;
	end       = 0;
	inner_r   = 0;
	linestyle = nullptr;
	fillstyle = nullptr;
}

EllipseData::~EllipseData()
{
	if (linestyle) linestyle->dec_count();
	if (fillstyle) fillstyle->dec_count();
}

bool EllipseData::UsesAngles()
{
	//difference is close to multiple of 2*pi
	return fmod(fabs(start-end) + .001, 2*M_PI) < .002;
}

void EllipseData::InstallDefaultLineStyle()
{
	if (linestyle) linestyle->dec_count();
	linestyle = new LineStyle();
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
		p = new EllipseData();
		dup = p;
	}

	p->center  = center;
	p->x       = x;
	p->y       = y;
	p->a       = a;
	p->b       = b;
	p->start   = start;
	p->end     = end;
	p->inner_r = inner_r;
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

void EllipseData::FindBBox()
{
	flatpoint pp = center + a*x;
	maxx = minx = pp.x;
	maxy = miny = pp.y;

	double e = end;
	if (end == start) e = start + 2*M_PI;

	ClearBBox();
	double step = (e-start)/20; //propably slightly more efficient to actually compute exact bounds, not just approximate
	flatvector v;

	for (double t=start; (e > start && t<=e) || (e < start && t >= e); t+=step) {
		v = cos(t) * a *x + sin(t) * b * y;;
		pp = center + v;
		addtobounds(pp);
		if (inner_r > 0) {
			pp = center + inner_r * v;
			addtobounds(pp);
		}
	}
}

/*! If focal points are not both on x or both on y axis, that adjust transform accordingly.
 * This results in origin being ellipse center.
 */
void EllipseData::MakeAligned()
{
	cerr << " *** IMPLEMENT EllipseData::MakeAligned()!!!!"<<endl;
}

PathsData *EllipseData::ToPath()
{
	cerr << " *** IMPLEMENT EllipseData::ToPath()!!!!"<<endl;
	return nullptr;
}

/*! In object coordintates. c is the length of a string you would connect f1, f2, and path points with.
 */
void EllipseData::SetFoci(flatpoint f1,flatpoint f2,double c) //c==-1
{
	if (c==-1) { if (a>b) c=2*a; else c=2*b; }
	center = (f1+f2)/2;
	x = (f1-f2);
	double f = norm(x);
	if (c<f) c = f;
	if (f) x /= f;
	else x = flatpoint(1,0);
	y = transpose(x);
	a = c/2;
	b = sqrt(a*a-f*f/4);

	FindBBox();
}

/*! c is the length of string that would have endpoints on the foci.
 * If parameter is null, don't return that one.
 */
void EllipseData::GetFoci(flatpoint *f1,flatpoint *f2,double *c)
{
	double f = sqrt(fabs(a*a - b*b));
	if (c) *c = 2*sqrt((a > b ? b : a)*(a > b ? b : a) + f*f);
	if (f1) *f1 = center + (a > b ? x : y) * f;
	if (f2) *f2 = center - (a > b ? x : y) * f;
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

	} else if (c==ELLP_StartAngle) {  p= center + a*cos(start)*x + b*sin(start)*y;
	} else if (c==ELLP_EndAngle)   {  p= center + a*cos(end)*x   + b*sin(end)*y;

	} else if (c==ELLP_XRadius)  { p= center + a*x;
	} else if (c==ELLP_XRadiusN) { p= center - a*x;
	} else if (c==ELLP_YRadius)  { p= center + b*y;
	} else if (c==ELLP_YRadiusN) { p= center - b*y;

	} else if (c==ELLP_OuterRadius) { p=center + a*cos(start)*x + b*sin(start)*y;
	} else if (c==ELLP_InnerRadius) { p=center + inner_r*a*cos(start)*x + inner_r*b*sin(start)*y;
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

	if (what==-1) { 
		att->push("id", "somename #String id");
		att->push("start","0 #starting angle, default radians");
		att->push("end","0 #ending angle. If equal to start, assume full circle");
		att->push("inner_r","0 #Inner radius, as fraction of default radius, if any");
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

	if (style & ELLIPSE_IsCircle) att->push("flags", "circle");

	att->push("start", start);
	att->push("end", end);
	att->push("inner_r", inner_r);
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
  : anInterface(nowner, nid,ndp)
{
	linestyle.color.rgbf(1., 0, 0);
	controlcolor.rgbf(.5, .5, .5, 1);
	data = NULL;
	eoc  = NULL;

	showdecs        = 1;
	curpoint        = ELLP_None; //point clicked on
	hover_point     = ELLP_None; //point hovered over, changes when !mouse down
	creationstyle   = 0;
	createfrompoint = 0;  // 0=lbd=ulc, 1=lbd=center
	createangle     = 0;
	createx         = flatpoint(1, 0);
	createy         = flatpoint(0, 1);
	linestyle.width = 1;

	mode = ELLP_None;

	needtodraw = 1;

	sc = NULL;
}

//! Destructor
EllipseInterface::~EllipseInterface()
{
	deletedata();
	if (sc) sc->dec_count();
	DBG cerr <<"----in EllipseInterface destructor"<<endl;
}

void EllipseInterface::Dp(Laxkit::Displayer *ndp)
{
	anInterface::Dp(ndp);
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
		LineStyle *nlinestyle = dynamic_cast<LineStyle *>(nobj);
		if (mask&GCForeground) { if (data) data->linestyle->color = nlinestyle->color; else linestyle.color = nlinestyle->color; }
		if (mask&GCLineWidth)  { if (data) data->linestyle->width = nlinestyle->width; else linestyle.width = nlinestyle->width; }
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
	showdecs   = 0;
	needtodraw = 1;
	curpoint   = ELLP_None;
	hover_point = ELLP_None;
	return 0;
}

void EllipseInterface::Clear(SomeData *d)
{
	if (!d || d == data) deletedata();
}

void EllipseInterface::deletedata()
{
	if (data) data->dec_count();
	data     = NULL;
	curpoint = ELLP_None;
	hover_point = ELLP_None;
}

int EllipseInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
	if (!ndata || dynamic_cast<EllipseData *>(ndata) == NULL) return 1;

	EllipseData *bzd = data;
	data = dynamic_cast<EllipseData *>(ndata);
	int td = showdecs;
	int ntd = needtodraw;
	showdecs   = 0;
	needtodraw = 1;

	Refresh();

	needtodraw = ntd;
	showdecs   = td;
	data       = bzd;
	return 1;
}

int EllipseInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!data) {
		if (needtodraw) needtodraw=0;
		return 1;
	}

	DBG cerr <<"  EllipseRefresh showdecs: "<<showdecs<<"  width: "<<data->linestyle->width<<endl;
		
	dp->NewFG(&data->linestyle->color);
	double thin = ScreenLine();
	dp->LineAttributes(data->linestyle->width,LineSolid,data->linestyle->capstyle,data->linestyle->joinstyle);

	 // draw just ellipse
	flatpoint f1 = getpoint(ELLP_Focus1,false);
	flatpoint f2 = getpoint(ELLP_Focus2,false);
	dp->drawfocusellipse(f2, f1,
			2*(fabs(data->a)>fabs(data->b)?data->a:data->b),
			data->start,data->end, 0);
	if (data->inner_r != 0) {
		flatpoint f = f1;
		flatpoint center = getpoint(ELLP_Center, false);
		flatpoint v = (f-center) * data->inner_r;
		f = center + v;
		dp->drawfocusellipse(center - v,f,
			data->inner_r * 2*(fabs(data->a)>fabs(data->b)?data->a:data->b),
			data->start,data->end, 0);
	}

	// bool hovered;
	
	 // draw control points;
	if (showdecs) {
		dp->LineWidthScreen(thin);
		dp->NewFG(&controlcolor);
		flatpoint p, center, start, end;

		if (hover_point == ELLP_OuterRadius) {
			dp->LineWidthScreen(3*thin);
			dp->drawfocusellipse(f2, f1, 2*(fabs(data->a)>fabs(data->b)?data->a:data->b), 0,0, 0);
			dp->LineWidthScreen(thin);
		}

		center = getpoint(ELLP_Center, false);

		 // angle points
		if (data->UsesAngles() || curpoint == ELLP_StartAngle || curpoint == ELLP_EndAngle) {
			start = getpoint(ELLP_StartAngle,false);  //hovered = (hover_point == ELLP_StartAngle);
			end = getpoint(ELLP_EndAngle,false);
			dp->LineAttributes(thin*(hover_point == ELLP_StartAngle ? 3 : 1), LineDoubleDash, LAXCAP_Butt, LAXJOIN_Round);
			dp->drawline(center, start);
			dp->LineAttributes(thin*(hover_point == ELLP_EndAngle ? 3 : 1), LineDoubleDash, LAXCAP_Butt, LAXJOIN_Round);
			dp->drawline(center, end);

			dp->LineAttributes(thin, LineSolid, LAXCAP_Butt, LAXJOIN_Round);
			dp->drawpoint(start, 3*thin, hover_point == ELLP_StartAngle ? 1 : 0);
			dp->drawpoint(end, 3*thin, hover_point == ELLP_EndAngle ? 1 : 0);
		}
		
		 // open foci
		dp->drawpoint(f1, 5*thin, hover_point == ELLP_Focus1 ? 1 : 0); // focus1
		if (!data->IsCircle()) { // is not circle so draw second focus
			dp->drawpoint(f2, 5*thin, hover_point == ELLP_Focus2 ? 1 : 0); // focus2
		}

		if (hover_point == ELLP_WildPoint) {
			dp->LineAttributes(thin, LineDoubleDash, LAXCAP_Butt, LAXJOIN_Round);
			flatpoint hp = dp->screentoreal(hover_x, hover_y);
			dp->drawline(f1, hp);
			dp->drawline(f2, hp);
			dp->LineAttributes(thin, LineSolid, LAXCAP_Butt, LAXJOIN_Round);
		}
		
		 // axes
		p = getpoint(ELLP_XRadius,false);
		dp->drawpoint(p, 3*thin, hover_point == ELLP_XRadius ? 1 : 0);
		p = getpoint(ELLP_XRadiusN,false);
		dp->drawpoint(p, 3*thin, hover_point == ELLP_XRadiusN ? 1 : 0);
		p = getpoint(ELLP_YRadius,false);
		dp->drawpoint(p, 3*thin, hover_point == ELLP_YRadius ? 1 : 0);
		p = getpoint(ELLP_YRadiusN,false);
		dp->drawpoint(p, 3*thin, hover_point == ELLP_YRadiusN ? 1 : 0);


		//  // angles
		// if (hover_point == ELLP_StartAngle || hover_point == ELLP_EndAngle) {
		// 	dp->LineAttributes(thin,LineDoubleDash, LAXCAP_Butt, LAXJOIN_Round);
		// 	dp->drawline(data->center, dp->screentoreal(hover_x,hover_y));
		// 	dp->drawline(data->center, getpoint(hover_point==ELLP_StartAngle?ELLP_EndAngle:ELLP_StartAngle,false)); // draw line of other angle point
		// 	dp->LineAttributes(thin,LineSolid,LAXCAP_Butt,LAXJOIN_Round);
		// }
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

// bool EllipseInterface::Near(EllispePoints pnt, double threshhold)
// {
// 	flatpoint p2 = getpoint(ELLP_Center);
// 	dd = (p2-p1).norm2();
// 	if (dd < d) return ELLP_Center;
// }

/*! Note, never returns ELLP_WildPoint.
 * Picks closest within a distance, but not the standard rect points. Those have
 * to be scanned for elsewhere with rectinterface.scan().
 */
EllipsePoints EllipseInterface::scan(double x,double y, unsigned int state) //*** only checks points, no online jazz
{
	if (!data) return ELLP_None;

	flatpoint p2;
	flatpoint p = screentoreal(x,y);
	
	double dd, d = 1e+10, threshhold = NearThreshhold()/dp->Getmag();
	// DBG cerr <<"scan d="<<d<<"(x,y)="<<p.x<<','<<p.y<<endl;
	
	EllipsePoints closest = ELLP_None;

	// first check normal points:
	static const EllipsePoints pts[] = {
		 ELLP_Center,
		 ELLP_Focus1,
		 ELLP_Focus2,
		 ELLP_XRadius,
		 ELLP_XRadiusN,
		 ELLP_YRadius,
		 ELLP_YRadiusN,
		 ELLP_None
		};

	for (int i = 0; pts[i] != ELLP_None; i++) {
		p2 = getpoint(pts[i], true);
		dd = (p2-p).norm();
		if (dd < d) {
			d = dd;
			closest = pts[i];
		}
	}
	if (d < 3*threshhold) return closest;
	
	flatpoint center = data->getpoint(ELLP_Center, true);

	// ELLP_StartAngle, check segment from center
	p2 = data->getpoint(ELLP_StartAngle, true);
	dd = distance(p, center, p2);
	DBG cerr << "   dist to start: "<<dd<<endl;
	if (dd < d) { d = dd; closest = ELLP_StartAngle; }

	// ELLP_EndAngle,
	p2 = data->getpoint(ELLP_EndAngle, true);
	dd = distance(p, center, p2);
	DBG cerr << "   dist to end: "<<dd<<endl;
	if (dd < d) { d = dd; closest = ELLP_EndAngle; }

	// points defined by ellipse edge
	flatpoint f1 = data->getpoint(ELLP_Focus1, true);
	flatpoint f2 = data->getpoint(ELLP_Focus2, true);
	p2 = data->getpoint(ELLP_XRadius, true);
	double pc = norm(p-f1) + norm(p-f2);
	double dc = norm(p2-f1) + norm(p2-f2);
	dd = fabs(pc - dc);
	if (dd < d && dd < threshhold) {
		d = dd;
		DBG cerr << "  --------state: "<<state<<endl;
		if ((state & (ShiftMask|ControlMask)) == (ShiftMask|ControlMask)) closest = ELLP_WildPoint;
		else if ((state & (ShiftMask|ControlMask)) == ControlMask) closest = ELLP_InnerRadius;
		else closest = ELLP_OuterRadius;
		// *** clamp outer rim check on outside
	}
	
	DBG cerr << "ellipse closest: d: "<<d<<"  "<<PointName(closest)<<endl;
	return closest;
}

int EllipseInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	DBG cerr << "  in ellipse lbd..";

	hover_x=x;
	hover_y=y;

	EllipsePoints over = scan(x,y, state);

	if (data && over != ELLP_None) {
		buttondown.down(d->id,LEFTBUTTON,x,y, over);
		curpoint = over;
	
		// if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) { // +^lb move around wildpoint
		// 	flatpoint p = getpoint(ELLP_Focus1,true),p2=getpoint(ELLP_Focus2, true);
		// 	data->x=getpoint(ELLP_Focus1, true)-data->center;
		// 	if (data->x.isZero()==false) data->x/=norm(data->x);
		// 	else data->x=flatpoint(1,0);
		// 	data->y=transpose(data->x);
		// 	double f2=(p-p2)*(p-p2);
		// 	p=  p-screentoreal(x,y);
		// 	p2=p2-screentoreal(x,y);
		// 	double c=sqrt(p*p)+sqrt(p2*p2);
		// 	data->a=c/2; // automatically puts major axis on x
		// 	data->b=sqrt(data->a*data->a - f2);
		// 	// rectify();
		// 	curpoint=ELLP_WildPoint;
		// 	needtodraw|=1;
		// 	return 0;

		// } else if ((state&LAX_STATE_MASK)==ControlMask) { // ^lb focus or end angle
		// 	if (over == ELLP_Focus1 || over == ELLP_Focus2) { // remove circle restriction
		// 		curpoint = over;
		// 		data->SetStyle(EllipseData::ELLIPSE_IsCircle, false);
		// 		needtodraw|=2;
		// 		return 0;
		// 	}

		// } else if ((state&LAX_STATE_MASK)==ShiftMask) { // +lb start angle
	
		// } else if ((state&LAX_STATE_MASK)==0) { // straight click
		// 	curpoint = over;
		// 	needtodraw |= 2;
		// 	return 0;
		// }

		return 0; // other click with data
	}
	DBG cerr <<"  noellipsepointfound  ";

	
	 // make new one
	EllipseData *obj=NULL;
	ObjectContext *oc=NULL;
	int c = viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
	if (c>0) obj = dynamic_cast<EllipseData *>(oc->obj);

	if (obj) { 
		 // found another EllipseData to work on.
		 // If this is primary, then it is ok to work on other ellipse objs, but not click onto
		 // other types of objects.
		data = obj;
		data->inc_count();
		if (eoc) delete eoc;
		eoc=oc->duplicate();

		if (viewport) viewport->ChangeObject(oc,0,true);
		buttondown.up(d->id,LEFTBUTTON);
		needtodraw=1;
		return 0;

	} else if (c < 0) {
		 // If there is some other non-image data underneath (x,y) and
		 // this is not primary, then switch objects, and switch tools to deal
		 // with that object.
		if (!primary && c==-1 && viewport->ChangeObject(oc,1,true)) {
			buttondown.up(d->id,LEFTBUTTON);
			return 1;
		}
	}


	buttondown.down(d->id,LEFTBUTTON,x,y, ELLP_DragRect);
	
	 // To be here, must want brand new data plopped into the viewport context
	if (viewport) viewport->ChangeContext(x,y,NULL);

	EllipseData *ndata=NULL; //creates with 1 count
	ndata = dynamic_cast<EllipseData *>(somedatafactory()->NewObject(LAX_ELLIPSEDATA));
	if (!ndata) ndata=new EllipseData;
	ndata->InstallDefaultLineStyle();
	*(ndata->linestyle) = linestyle;

	if (viewport) {
		oc=NULL;
		viewport->NewData(ndata,&oc);//viewport adds only its own counts
		if (eoc) delete eoc;
		if (oc) eoc=oc->duplicate();
	}

	data = ndata;

	curpoint = ELLP_DragRect;
	flatpoint p = screentoreal(x,y);
	data->style = creationstyle; 
	data->origin(p);
	data->x = createx;
	data->y = createy;
	data->a = data->b = 0;
	data->FindBBox();
	createp = p;

	needtodraw = 1;
	DBG cerr <<"..ellipselbd done   ";

	return 0;
}

int EllipseInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	int action = ELLP_None;
	int dragged = buttondown.up(d->id,LEFTBUTTON, &action);

	if (dragged < 5 && action != ELLP_None) {
		// *** number input if click on various things
		const char *label = nullptr;
		const char *msg = nullptr;
		double f = 0;

		if (action == ELLP_Focus1 || action == ELLP_Focus2) {
			f = sqrt(fabs(data->a*data->a - data->b*data->b));
			msg = "focus_num";
			label = _("Focus");

		} else if (action == ELLP_XRadius || action == ELLP_XRadiusN) {
			f = data->a;
			msg = "a_num";
			label = _("X radius");

		} else if (action == ELLP_YRadius || action == ELLP_YRadiusN) {
			f = data->b;
			msg = "b_num";
			label = _("Y radius");

		} else if (action == ELLP_WildPoint) {
			f = 2*(data->a > data->b ? data->a : data->b);
			msg = "c_num";
			label = _("String");

		} else if (action == ELLP_OuterRadius) {
			// f = (data->transformPointInverse(screentoreal(x,y)) - center).norm();
			f = 1;
			msg = "r_num";
			label = _("Size");
			// ref_point = data->transformPointInverse(screentoreal(x,y));

		} else if (action == ELLP_InnerRadius) {
			f = data->inner_r;
			msg = "inner_num";
			label = _("Inner ring");

		} else if (action == ELLP_StartAngle) {
			f = data->start * 180 / M_PI;
			msg = "start_num";
			label = _("Start angle");

		} else if (action == ELLP_EndAngle) {
			f = data->end * 180 / M_PI;
			msg = "end_num";
			label = _("End angle");
		}

		if (msg) {
			char str[30];
			sprintf(str, "%f", f);
			LaxFont *font = curwindow->win_themestyle->normal;
			double th = font->textheight();
			DoubleBBox box(x-5*th, x+5*th, y-.75*th, y+.75*th);
			viewport->SetupInputBox(object_id, label, str, msg, box);
		}
	}

	curpoint = ELLP_None;
	needtodraw=1;
	return 0;
}

int EllipseInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
	if (!data || !s || isblank(s->str)) return 1;
	double num = strtod(s->str, nullptr);

	if (!strcmp(mes,"focus_num")) {
		if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
		flatpoint f1,f2;
		double c;
		data->GetFoci(&f1, &f2, &c);
		flatpoint v = f2-f1;
		if (v.isZero()) v = flatpoint(1,0);
		flatpoint center = (f2+f1)/2;
		v.normalize();
		data->SetFoci(center - num * v, center + num * v, c);

	} else if (!strcmp(mes,"a_num")) {
		if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
		data->a = num;

	} else if (!strcmp(mes,"b_num")) {
		if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
		data->b = num;

	} else if (!strcmp(mes,"c_num")) {
		flatpoint f1,f2;
		data->GetFoci(&f1, &f2, nullptr);
		if (num < norm(f1-f2)) { PostMessage(_("Number must be greater than distance between foci")); return 0;}
		data->SetFoci(f1,f2, num);

	} else if (!strcmp(mes,"r_num")) {
		// double old = (ref_point - data->center).norm();
		// double scale = num/old;
		if (num <= 0) { PostMessage(_("Number must be positive")); return 0; }
		data->a *= num;
		data->b *= num;

	} else if (!strcmp(mes,"inner_num")) {
		if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
		data->inner_r = num;

	} else if (!strcmp(mes,"start_num")) {
		data->start = num * M_PI/180;

	} else if (!strcmp(mes,"end_num")) {
		data->end = num * M_PI/180;

	} else return 1;

	data->FindBBox();
	needtodraw = 1;
	return 0;
}

int EllipseInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse) 
{
	hover_x=x;
	hover_y=y;

	if (!data) return 1;

	if (!buttondown.any()) {
		// *** update hover
		EllipsePoints over = scan(x,y,state);
		if (over != curpoint || over == ELLP_WildPoint) {
			curpoint = over;
			PostMessage(curpoint != ELLP_None ? PointName(curpoint) : nullptr);
			needtodraw = 1;
		}
		hover_point = curpoint;
		return 1;
	}

	int lx,ly;
	buttondown.move(mouse->id, x,y, &lx,&ly);

	flatpoint d = screentoreal(x,y)-screentoreal(lx,ly);

	if (curpoint != ELLP_None) {
		if (curpoint == ELLP_Center) {
			data->center += data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(lx,ly));
			needtodraw = 1;

		} else if (curpoint==ELLP_Focus2 || curpoint==ELLP_Focus1) {
			flatpoint f1 = getpoint(ELLP_Focus1, true);
			flatpoint f2 = getpoint(ELLP_Focus2, true);

			if (data->IsCircle()) { f1 += d; f2 += d;} 
			else if (curpoint == ELLP_Focus2) f2 += d;
			else f1 += d; 
			
			data->SetFoci(data->transformPointInverse(f1),data->transformPointInverse(f2),-1); //*** doesn't deal with non-whole ellipse, the segment jumps around
			needtodraw|=2; 

		} else if (curpoint == ELLP_StartAngle) {
			if (data->a > 0 && data->b > 0) {
				flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
				flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
				op.normalize();
				np.normalize();
				data->start += asin(op.cross(np));
				// double oa = atan2(op.x / data->a, op.y / data->b);
				// double na = atan2(np.x / data->a, np.y / data->b);
				// data->start -= na-oa;
				DBG cerr << "start: "<<data->start<<"  end: "<<data->end<<endl;
				needtodraw = 1;
			}

		} else if (curpoint == ELLP_EndAngle) {
			if (data->a > 0 && data->b > 0) {
				flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
				flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
				// double oa = atan2(op.x / data->a, op.y / data->b);
				// double na = atan2(np.x / data->a, np.y / data->b);
				// data->end -= na-oa;
				op.normalize();
				np.normalize();
				data->end += asin(op.cross(np));
				DBG cerr << "start: "<<data->start<<"  end: "<<data->end<<endl;
				needtodraw = 1;
			}
		
		} else if (curpoint == ELLP_XRadius || curpoint == ELLP_XRadiusN
				|| curpoint == ELLP_YRadius || curpoint == ELLP_YRadiusN) {
			int dox = (curpoint == ELLP_XRadius ? 1 : (curpoint == ELLP_XRadiusN ? -1 : 0));
			int doy = (curpoint == ELLP_YRadius ? 1 : (curpoint == ELLP_YRadiusN ? -1 : 0));
			if (state & ShiftMask) {
				if (dox) doy = -1;
				else if (doy) dox = 1;
			}

			if (dox) {
				flatpoint v = data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(lx,ly));
				double adiff = data->a;
				data->a += (v*data->x)/data->x.norm() * dox;
				if (data->a < 0) data->a = 0;
				adiff -= data->a;
				if (adiff && (state & ControlMask)) { //move origin


				}
				needtodraw = 1;
			}

			if (doy) {
				flatpoint v = data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(lx,ly));
				data->b += (v*data->y)/data->y.norm() * doy;
				if (data->b < 0) data->b = 0;
				needtodraw = 1;
			}

		} else if (curpoint==ELLP_OuterRadius) {
			flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
			flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
			if (data->b == 0 || data->a == 0) {
				//just do straight scale
				double scale = norm(np);
				if (scale > 1e-5) {
					scale = norm(op) / scale;
					data->a *= scale;
					data->b *= scale;
				}
			} else {
				double r = data->a / data->b;
				double newb = sqrt((np.x*np.x/r/r) + np.y*np.y);
				data->a = newb * r;
				data->b = newb;
			}
			needtodraw = 1;

		} else if (curpoint==ELLP_InnerRadius) {
			flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
			flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
			data->inner_r += np.norm() - op.norm();
			if (data->inner_r < 0) data->inner_r = 0;
		
		} else if (curpoint==ELLP_WildPoint) { //move leaves f1,f2 change c
			flatpoint p,p2;
			flatpoint h = screentoreal(x,y);
			p= getpoint(ELLP_Focus1, true) - h;
			p2=getpoint(ELLP_Focus2, true) - h;
			data->SetFoci(getpoint(ELLP_Focus1, false), getpoint(ELLP_Focus2, false),sqrt(p*p)+sqrt(p2*p2));
			// rectify();
			needtodraw|=1;

		} else if (curpoint == ELLP_DragRect) {
			//shift: centered, control: circle
			int ix,iy;
			buttondown.getinitial(mouse->id,LEFTBUTTON,&ix,&iy);
			flatpoint p1 = screentoreal(ix,iy);
			flatpoint p2 = screentoreal(x,y);
			double mult = 1;
			if (state & ShiftMask) data->origin(p1);
			else {
				data->origin((p1+p2)/2);
				mult = 2;
			}
			p1 = data->transformPointInverse(p1);
			p2 = data->transformPointInverse(p2);
			data->a = fabs((p2.x-p1.x)/mult);
			data->b = fabs((p2.y-p1.y)/mult);
			if ((state & (ControlMask | ShiftMask)) == (ControlMask | ShiftMask)) {
				//centered circles
				data->a = data->b = norm(p1-p2);

			} else if (state & ControlMask) {
				double min = (data->a > data->b ? data->b : data->a);
				data->a = data->b = min;

				#define sgn(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
				if ((state & ShiftMask) == 0)
					data->origin(data->transformPoint(p1 + flatvector(sgn(p2.x-p1.x) * data->a, sgn(p2.y - p1.y) * data->b)));
			}
			// data->FindBBox();
			needtodraw = 1;
		}

		hover_x=x; hover_y=y;
		if (needtodraw) data->FindBBox();
	}

	return 0;
}

/*! \todo maybe combine rectinterface shortcuts with ellipse, since it is a permanent child?
 */
int EllipseInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{
	if (ch == LAX_Shift || ch == LAX_Control) { // intercept to update mouse overs
		DBG cerr << "fake mouse for ch "<<ch<<"  shift: "<<(state & ShiftMask)<<"  control: "<<(state&ControlMask)<<endl;
		LaxMouse *mouse = app->devicemanager->findMouse(0);
		MouseMove(hover_x, hover_y, state | (ch == LAX_Shift ? ShiftMask : 0) | (ch==LAX_Control ? ControlMask : 0), mouse);

	} else if (data && (ch == LAX_Bksp || ch == LAX_Del)) {
		bool found = false;
		if (curpoint == ELLP_Focus1 || curpoint == ELLP_Focus2) {
			double min = (data->a > data->b ? data->b : data->a);
			data->a = data->b = min;
			found = true;
		}

		if (curpoint == ELLP_InnerRadius) {
			data->inner_r = 0;
			found = true;
		}

		if (curpoint == ELLP_XRadius || curpoint == ELLP_XRadiusN) {
			data->a = data->b;
			found = true;
		}

		if (curpoint == ELLP_YRadius || curpoint == ELLP_YRadiusN) {
			data->b = data->a;
			found = true;
		}

		if (curpoint == ELLP_StartAngle) {
			data->start = data->end = 0;
			found = true;
		}

		if (curpoint == ELLP_EndAngle) {
			data->end = data->start;
			found = true;
		}

		if (found) {
			buttondown.clear();
			data->FindBBox();
			needtodraw = 1;
			curpoint = hover_point = ELLP_None;
			return 0;
		}
	}

	 //check shortcuts
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}
	
	return 1; 
}

int EllipseInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb) 
{
	if (ch == LAX_Shift || ch == LAX_Control) { // intercept to update mouse overs
		LaxMouse *mouse = app->devicemanager->findMouse(0);
		if (ch == LAX_Shift) state &= ~ShiftMask;
		else if (ch == LAX_Control) state &= ~ControlMask;
		MouseMove(hover_x, hover_y, 0, mouse);
		return 0;
	}

	return 1; 
}

Laxkit::ShortcutHandler *EllipseInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;


	sc=new ShortcutHandler(whattype());

	sc->Add(ELLP_ToggleDecs,  'd',0,0,        "ToggleDecs",  _("Toggle decorations"),NULL,0);
	sc->Add(ELLP_ToggleCircle,'c',0,0,        "ToggleCircle",  _("Toggle editing as a circle"),NULL,0);

	//sc->Add(PATHIA_WidthStepR,        'W',ControlMask|ShiftMask,0,"WidthStepR", _("Change how much change for width changes"),NULL,0);
	//sc->AddShortcut(LAX_Bksp,0,0, PATHIA_Delete);

	manager->AddArea(whattype(),sc);
	return sc;
}

int EllipseInterface::PerformAction(int action)
{
	if (action == ELLP_ToggleDecs) {
		showdecs = !showdecs;
		needtodraw=1;
		return 0;

	} else if (action == ELLP_ToggleCircle) {
		if (!data) return 0;
		data->b = data->a;
		// bool circle=data->GetStyle(EllipseData::ELLIPSE_IsCircle);
		// data->SetStyle(EllipseData::ELLIPSE_IsCircle, !circle);
		needtodraw=1;
		return 0;
	}

	return 1;
}



} // namespace LaxInterfaces 

