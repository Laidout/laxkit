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
#include <lax/interfaces/somedataref.h>
#include <lax/bezutils.h>
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
		case ELLP_Center:      return _("Center");
		case ELLP_Focus1:      return _("Focus1");
		case ELLP_Focus2:      return _("Focus2");
		case ELLP_StartAngle:  return _("StartAngle");
		case ELLP_EndAngle:    return _("EndAngle");
		case ELLP_XRadius:     return _("XRadius");
		case ELLP_XRadiusN:    return _("XRadiusN");
		case ELLP_YRadius:     return _("YRadius");
		case ELLP_YRadiusN:    return _("YRadiusN");
		case ELLP_OuterRadius: return _("Outer Radius");
		case ELLP_InnerRadius: return _("Inner Radius");
		case ELLP_WildPoint:   return _("Wild point");
		case ELLP_WedgeToggle: return _("Wedge toggle");
		default: {
			DBG cerr << "PointName unknown for: "<<p<<endl;
			return "?";
		}
	}
	return "?";
}

const char *PointMessage(EllipsePoints p)
{
	switch (p) {
		case ELLP_None:        return _("None");
		case ELLP_Center:      return _("Center");
		case ELLP_Focus1:      return _("Focus1, delete to make circle");
		case ELLP_Focus2:      return _("Focus2, delete to make circle");
		case ELLP_StartAngle:  return _("StartAngle, delete to use full 360");
		case ELLP_EndAngle:    return _("EndAngle, delete to use full 360");
		case ELLP_XRadius:     return _("XRadius, delete to make circle");
		case ELLP_XRadiusN:    return _("XRadius, delete to make circle");
		case ELLP_YRadius:     return _("YRadius, delete to make circle");
		case ELLP_YRadiusN:    return _("YRadius, delete to make circle");
		case ELLP_OuterRadius: return _("Outer Radius");
		case ELLP_InnerRadius: return _("Inner Radius, delete to remove");
		case ELLP_WildPoint:   return _("Wild point");
		case ELLP_WedgeToggle: return _("Wedge toggle, drag to toggle wedge, chord, gap");
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
	wedge_type = ELLIPSE_Wedge;
}

EllipseData::~EllipseData()
{
	if (linestyle) linestyle->dec_count();
	if (fillstyle) fillstyle->dec_count();
}

bool EllipseData::UsesAngles()
{
	//difference is close to multiple of 2*pi
	return fmod(fabs(start-end) + .001, 2*M_PI) > .002;
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
	p->wedge_type = wedge_type;
	if (linestyle) p->linestyle = dynamic_cast<LineStyle*>(linestyle->duplicate(nullptr));

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

/*! Get a bezier representation of the path.
 * If which==1, then outer path without wedge.
 * If which == 2, then inner without wedge, in start..end order.
 * If which == 3, then inner+outer including wedge. Note that in this case, inner is in order end..start.
 * 
 * If which is 1 or 2, then pts_ret must have room for 12 points.
 * If which is 3, pts_ret must have room for 24 points, but might return 15 if inner_r == 0.
 * Returned points are c-v-c-c-v-c-c-v-c-c-v-c...
 * Return value is the number of points written to pts_ret.
 */
int EllipseData::GetPath(int which, flatpoint *pts_ret)
{
	if (which == 2) { //inner only, without wedge
		bez_ellipse(pts_ret, 4, center.x, center.y, inner_r*a, inner_r*b, x,y, start,end);
		return 12;
	}
	
	bez_ellipse(pts_ret, 4, center.x, center.y, a, b, x,y, start,end);
	if (which != 3) return 12; //outer only, without wedge

	// add inner points
	if (fabs(inner_r) < 1e-10) { // no inner ring
		int n = 12;
		if (UsesAngles()) {
			flatvector v;
			if (wedge_type == ELLIPSE_Chord) {
				v = (pts_ret[10] - pts_ret[1])/3;
				pts_ret[0] = pts_ret[1] - v;
				pts_ret[11] = pts_ret[10] + v;

			} else if (wedge_type == ELLIPSE_Open) {
				pts_ret[11].info = LINE_Open;

			} else { //ELLIPSE_Wedge, point to center
				v = (pts_ret[10] - center)/3;
				pts_ret[11] = pts_ret[10] - v;
				pts_ret[12] = pts_ret[11] - v;
				pts_ret[13] = center;
				v = (pts_ret[1] - center)/3;
				pts_ret[14] = center + v;
				pts_ret[0] = pts_ret[14] + v;
				n = 15;
			}
		}
		return n;
	}

	//get inner points
	bez_ellipse(pts_ret+12, 4, center.x, center.y, inner_r*a, inner_r*b, x,y, end,start);

	// adjust for wedge
	if (UsesAngles()) {
		flatvector v;
		if (wedge_type == ELLIPSE_Chord) {
			v = (pts_ret[10] - pts_ret[1])/3;
			pts_ret[0] = pts_ret[1] - v;
			pts_ret[11] = pts_ret[10] + v;
			pts_ret[11].info = LINE_Closed;

			v = (pts_ret[22] - pts_ret[13])/3;
			pts_ret[12] = pts_ret[13] - v;
			pts_ret[23] = pts_ret[22] + v;
			pts_ret[23].info = LINE_Closed;

		} else if (wedge_type == ELLIPSE_Open) {
			pts_ret[11].info = LINE_Open;
			pts_ret[23].info = LINE_Open;

		} else { //ELLIPSE_Wedge, connect inner and outer
			v = (pts_ret[10] - pts_ret[13])/3;
			pts_ret[11] = pts_ret[10] - v;
			pts_ret[12] = pts_ret[11] - v;
			v = (pts_ret[1] - pts_ret[22])/3;
			pts_ret[23] = pts_ret[22] + v;
			pts_ret[0] = pts_ret[23] + v;
		}

	} else { //solid rings, only need to terminate first
		pts_ret[11].info = LINE_Open;
		pts_ret[23].info = LINE_Open;
	}
	return 24;
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
	PathsData *paths = dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
	if (!paths) paths = new PathsData();

	flatpoint pts[24];
	int n = GetPath(3, pts);
	for (int c=0; c<n; c++) {
		paths->append(pts[c], c%3 == 0 ? POINT_TONEXT : (c%3 == 1 ? POINT_VERTEX : POINT_TOPREV));
		if (pts[c].info & LINE_Open) paths->pushEmpty();
		else if (pts[c].info & LINE_Closed) {
			paths->close();
			paths->pushEmpty();
		}
	}

	paths->m(m());
	return paths;
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

Attribute *EllipseData::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute();

	if (what==-1) { 
		att->push("id", "somename","String id");
		att->push("start","0","starting angle, default radians");
		att->push("end","0","ending angle. If equal to start, assume full circle");
		att->push("inner_r","0", "Inner radius, as fraction of default radius, if any");
		att->push("a","1","x axis radius");
		att->push("b","1","y axis radius");
		att->push("center","(0,0)","position of ellipse origin (not object origin)");
		att->push("x","(1,0)","x axis of ellipse (not object x axis)");
		att->push("y","(0,1)","y axis of ellipse (not object y axis)");
		att->push("linestyle",nullptr,"(optional) style to draw this ellipse");
		// att->push("flags","circle","tags for how to edit this object");
		att->push("wedge","wedge","Type of wedge when start != end: wedge, open, or chord");
		return att;
	}

	char scratch[100];

	att->push("id", Id());

	if (style & ELLIPSE_IsCircle) att->push("flags", "circle");

	att->push("wedge", wedge_type==ELLIPSE_Wedge ? "wedge" : wedge_type == ELLIPSE_Chord ? "chord" : "open");
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

	if (linestyle) {
		Attribute *att2 = att->pushSubAtt("linestyle");
		linestyle->dump_out_atts(att2, 0, context);
	}
	if (fillstyle) {
		Attribute *att2 = att->pushSubAtt("fillstyle");
		fillstyle->dump_out_atts(att2, 0, context);
	}

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

		} else if (!strcmp(name,"wedge")) {
			if (value) {
				if (!strcmp(value, "wedge")) wedge_type = ELLIPSE_Wedge;
				else if (!strcmp(value, "chord")) wedge_type = ELLIPSE_Chord;
				else wedge_type = ELLIPSE_Open;
			}
		} else if (!strcmp(name,"linestyle")) {
			// *** need to account for ref to line resource
			LineStyle *lstyle = new LineStyle();
			lstyle->dump_in_atts(att->attributes.e[c], flag, context);
			if (linestyle) linestyle->dec_count();
			linestyle = lstyle;

		} else if (!strcmp(name,"fillstyle")) {
			// *** need to account for ref to line resource
			FillStyle *style = new FillStyle();
			style->dump_in_atts(att->attributes.e[c], flag, context);
			if (fillstyle) fillstyle->dec_count();
			fillstyle = style;

		// } else if (!strcmp(name,"flags")) {
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

	allow_foci = true;
	allow_angles = true;
	allow_inner = true;
	show_foci = true;
	color_stroke = true;

	mode = ELLP_None;

	needtodraw = 1;

	sc = NULL;
}

//! Destructor
EllipseInterface::~EllipseInterface()
{
	deletedata();
	if (sc) sc->dec_count();
	if (eoc) delete eoc;
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
	EllipseInterface *edup = dynamic_cast<EllipseInterface *>(dup);
	if (dup == NULL) dup = edup = new EllipseInterface(NULL,id,NULL);
	else if (!dup) return NULL;
	edup->linestyle = linestyle;
	return anInterface::duplicate(dup);
}

//! Accepts LineStyle. For LineStyle, just copies over width and color.
int EllipseInterface::UseThis(anObject *nobj,unsigned int mask)
{
	if (!nobj) return 0;

	if (dynamic_cast<LineStyle *>(nobj)) { 
		DBG cerr <<"Ellipse new color stuff"<<endl;

		LineStyle *nlinestyle = dynamic_cast<LineStyle *>(nobj);
		if (mask & (LINESTYLE_Color | LINESTYLE_Width)) {
			if (mask & LINESTYLE_Color) { if (data) data->linestyle->color = nlinestyle->color; else linestyle.color = nlinestyle->color; }
			if (mask & LINESTYLE_Width)  { if (data) data->linestyle->width = nlinestyle->width; else linestyle.width = nlinestyle->width; }
			needtodraw=1;
		} else {
			if (!data) return 1;
			if (!data->fillstyle) data->fillstyle = new FillStyle();
			data->fillstyle->color = nlinestyle->color;
		}
		return 1;
	}

	return 0;
}

/*! Use oc for the context, but other_object for the final object.
 * If other_object == null, use oc->obj instead.
 * Incs count of other_object.
 *
 * Return 1 for used, 0 for not used.
 */
int EllipseInterface::UseThisObject(ObjectContext *oc, SomeData *other_object)
{
	if (!other_object) other_object = oc->obj;

	EllipseData *ndata = dynamic_cast<EllipseData*>(other_object);
	if (!ndata && dynamic_cast<SomeDataRef*>(other_object)) {
		SomeData *o = dynamic_cast<SomeDataRef*>(oc->obj)->GetFinalObject();
		ndata = dynamic_cast<EllipseData *>(o);
		if (!ndata) return 0;
	}

	if (!ndata) return 0;

	if (data && data != ndata) deletedata();
	if (eoc) delete eoc;
	eoc = oc->duplicate();

	if (data != ndata) {
		data = ndata;
		data->inc_count();
	}

	return 1;
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

	DBG cerr <<"  EllipseRefresh showdecs: "<<showdecs<<"  width: "<<(data->linestyle ? data->linestyle->width : 0)<<endl;
		
	LineStyle *lstyle = data->linestyle;
	if (!lstyle) lstyle = &linestyle;
	FillStyle *fstyle = data->fillstyle;
	if (fstyle && fstyle->function == LAXOP_None) fstyle = nullptr;

	dp->NewFG(&lstyle->color);
	double thin = ScreenLine();
	dp->LineAttributes(lstyle->width, LineSolid, lstyle->capstyle, lstyle->joinstyle);

	flatpoint f1 = getpoint(ELLP_Focus1,false);
	flatpoint f2 = getpoint(ELLP_Focus2,false);

	 // draw just ellipse
	flatpoint pts[12];
	int n = data->GetPath(1, pts);
	dp->moveto(pts[1]);
	flatpoint p1 = pts[1];
	for (int c=2; c<n-1; c+= 3) dp->curveto(pts[c], pts[c+1], pts[c+2]);
	if (!data->UsesAngles()) {
		dp->curveto(pts[11], pts[0], pts[1]);
		dp->closed();
	} else {
		data->GetPath(2, pts);
		if (data->wedge_type == EllipseData::ELLIPSE_Wedge) {
			dp->lineto(pts[10]);
			for (int c=9; c>=3; c-= 3) dp->curveto(pts[c], pts[c-1], pts[c-2]);
			dp->lineto(p1);

		} else if (data->wedge_type == EllipseData::ELLIPSE_Chord) {
			dp->closed();
			dp->moveto(pts[10]);
			for (int c=9; c>=3; c-= 3) dp->curveto(pts[c], pts[c-1], pts[c-2]);
			dp->closed();
		} else { //open
			dp->moveto(pts[10]);
			for (int c=9; c>=3; c-= 3) dp->curveto(pts[c], pts[c-1], pts[c-2]);
		}
	}
	if (fstyle) {
		dp->NewFG(&fstyle->color);
		dp->fill(1);
		dp->NewFG(&lstyle->color);
		dp->stroke(0);
	} else {
		dp->stroke(0);
	}
	
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

		if (hover_point == ELLP_InnerRadius) {
			dp->LineWidthScreen(3*thin);
			flatpoint v = (f2-f1)/2;
			dp->drawfocusellipse(center+v*data->inner_r, center-v*data->inner_r,
				data->inner_r*2*(fabs(data->a)>fabs(data->b)?data->a:data->b), 0,0, 0);
			dp->LineWidthScreen(thin);
		}

		// indicator for wedge
		if (data->UsesAngles()) {
			double s = data->start, e = data->end;
			if (s > e) e += 2*M_PI; else e -= 2*M_PI;
			bez_ellipse(pts, 4, data->center.x, data->center.y, data->a, data->b, data->x,data->y, e, s);
			dp->LineWidthScreen(thin * (curpoint == ELLP_WedgeToggle ? 3 : 1));
			dp->Dashes(2);
			dp->moveto(pts[1]);
			for (int c=2; c<n-1; c+= 3) dp->curveto(pts[c], pts[c+1], pts[c+2]);
			dp->stroke(0);
			dp->Dashes(0);
		}

		// draw a plus at th center
		center = getpoint(ELLP_Center, false);
		dp->LineWidthScreen(thin * (curpoint == ELLP_Center ? 3 : 1));
		dp->drawthing(center, thin*10/dp->Getmag(), thin*10/dp->Getmag(), 0, THING_Plus);
		dp->LineWidthScreen(thin);

		 // angle points
		if (data->UsesAngles() || curpoint == ELLP_StartAngle || curpoint == ELLP_EndAngle) {
			start = getpoint(ELLP_StartAngle,false);  //hovered = (hover_point == ELLP_StartAngle);
			end = getpoint(ELLP_EndAngle,false);
			dp->LineWidthScreen(thin*(hover_point == ELLP_StartAngle ? 3 : 1));
			dp->Dashes(2);
			dp->drawline(center, start);
			dp->LineWidthScreen(thin*(hover_point == ELLP_EndAngle ? 3 : 1));
			dp->Dashes(2);
			dp->drawline(center, end);

			dp->Dashes(0);
			dp->LineWidthScreen(thin);
			dp->drawpoint(start, 3*thin, hover_point == ELLP_StartAngle ? 1 : 0);
			dp->drawpoint(end, 3*thin, hover_point == ELLP_EndAngle ? 1 : 0);
		}
		
		 // open foci
		if (show_foci && allow_foci) {
			dp->drawpoint(f1, 5*thin, hover_point == ELLP_Focus1 ? 1 : 0); // focus1
			if (!data->IsCircle()) { // is not circle so draw second focus
				dp->drawpoint(f2, 5*thin, hover_point == ELLP_Focus2 ? 1 : 0); // focus2
			}
		}

		if (hover_point == ELLP_WildPoint) {
			dp->LineAttributes(thin/dp->Getmag(), LineDoubleDash, LAXCAP_Butt, LAXJOIN_Round);
			flatpoint hp = dp->screentoreal(hover_x, hover_y);
			dp->drawline(f1, hp);
			dp->drawline(f2, hp);
			dp->LineAttributes(thin/dp->Getmag(), LineSolid, LAXCAP_Butt, LAXJOIN_Round);
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
		if (!(show_foci && allow_foci) && (pts[i] == ELLP_Focus1 || pts[i] == ELLP_Focus2)) continue;
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
	//flatpoint startv = p2 - center;
	if (dd < d) {
		d = dd;
		if (state & ShiftMask) closest = ELLP_EndAngle;
		else closest = ELLP_StartAngle;
	}

	// ELLP_EndAngle,
	p2 = data->getpoint(ELLP_EndAngle, true);
	dd = distance(p, center, p2);
	//flatpoint endv = p2 - center;
	if (dd < d) {
		d = dd;
		if (state & ShiftMask) closest = ELLP_StartAngle;
		else closest = ELLP_EndAngle;
	}

	// points defined by ellipse edge
	flatpoint f1 = data->getpoint(ELLP_Focus1, true);
	flatpoint f2 = data->getpoint(ELLP_Focus2, true);
	p2 = data->getpoint(ELLP_XRadius, true);
	double pc = norm(p-f1) + norm(p-f2);
	double dc = norm(p2-f1) + norm(p2-f2);
	dd = fabs(pc - dc);
	if (dd < d && dd < threshhold*2) {
		d = dd;
		if ((state & (ShiftMask|ControlMask)) == (ShiftMask|ControlMask)) closest = ELLP_WildPoint;
		else if ((state & (ShiftMask|ControlMask)) == ControlMask) closest = ELLP_InnerRadius;
		else {
			flatpoint v = p - center;
			if (data->UsesAngles()) {
				// if (clockwise(startv, v) == clockwise(v,endv)) closest = ELLP_WedgeToggle;
				flatpoint pp = data->transformPointInverse(p);
				v.x = pp*data->x/data->x.norm()/(data->a != 0 ? data->a : 1);
				v.y = pp*data->y/data->y.norm()/(data->b != 0 ? data->b : 1);
				double s = (data->start < data->end ? data->start : data->end);
				double e = (data->start < data->end ? data->end : data->start);
				s = s - floor(s/(2*M_PI))*2*M_PI; //should make s in 0..2*pi
				e = s + fabs(data->end - data->start);
				double a = atan2(v.y, v.x);
				if (a < 0) a += 2*M_PI;
				if (e > s && a >= s && a <= e) closest = ELLP_OuterRadius;
				else if (e < s && a <= s && a >= e) closest = ELLP_OuterRadius;
				else closest = ELLP_WedgeToggle;
				DBG cerr << "s: "<<s*180/M_PI<<"  e: "<<e*180/M_PI<<"  a: "<<a*180/M_PI<<"  pt: "<<closest<<endl;
			}
			else closest = ELLP_OuterRadius;
		}
	}

	// check inner
	flatpoint v = (f2-f1) * (data->inner_r/2);
	f1 = center - v;
	f2 = center + v;
	pc = norm(p-f1) + norm(p-f2);
	dc = data->inner_r * dc; //norm(p2-f1) + norm(p2-f2);
	dd = fabs(pc - dc);
	if (dd < d && dd < threshhold*2) {
		d = dd;
		closest = ELLP_InnerRadius;
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

		if (curpoint == ELLP_XRadius) 	    ref_point = data->getpoint(ELLP_XRadiusN, true);
		else if (curpoint == ELLP_XRadiusN) ref_point = data->getpoint(ELLP_XRadius, true);
		else if (curpoint == ELLP_YRadius)  ref_point = data->getpoint(ELLP_YRadiusN, true);
		else if (curpoint == ELLP_YRadiusN) ref_point = data->getpoint(ELLP_YRadius, true);
		else if (curpoint == ELLP_StartAngle) ref_point.x = data->start;
		else if (curpoint == ELLP_EndAngle) ref_point.x = data->end;
		ref_point2 = data->getpoint(ELLP_Center, true);
		if (data->wedge_type == EllipseData::ELLIPSE_Wedge) ref_wedge = 0;
		else if (data->wedge_type == EllipseData::ELLIPSE_Chord) ref_wedge = 1;
		else ref_wedge = 2;
	
		return 0; // other click with data
	}
	DBG cerr <<"  noellipsepointfound  ";

	deletedata();
	
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

		} else if (action == ELLP_WedgeToggle) {
			data->start = data->end = 0;
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

Laxkit::MenuInfo *EllipseInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!data) return menu;

	if (!menu) menu = new MenuInfo();
	if (menu->n()) menu->AddSep(_("Arc"));

	menu->AddToggleItem(_("Show foci"), ELLP_ToggleFoci, 0, show_foci);
	if (data->UsesAngles()) {
		menu->AddItem(_("Close"), ELLP_CloseGap);
		menu->AddToggleItem(_("Wedge"), ELLP_UseWedge, 0, data->wedge_type == EllipseData::ELLIPSE_Wedge);
		menu->AddToggleItem(_("Chord"), ELLP_UseChord, 0, data->wedge_type == EllipseData::ELLIPSE_Chord);
		menu->AddToggleItem(_("Open gap"), ELLP_UseOpen, 0, data->wedge_type == EllipseData::ELLIPSE_Open);
	}
	return menu;
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

	} else if (!strcmp(mes,"set_width")) {
		if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
		data->linestyle->width = num;

	} else if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i = s->info2; //id of menu item

		if (   i == ELLP_CloseGap
			|| i == ELLP_UseWedge
			|| i == ELLP_UseChord 
			|| i == ELLP_UseOpen
			|| i == ELLP_ToggleFoci
		   ) {
			PerformAction(i);
			return 0;
		}

	} else return 1;

	data->FindBBox();
	Modified();
	needtodraw = 1;
	return 0;
}

int EllipseInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse) 
{
	hover_x=x;
	hover_y=y;

	if (!data) return 1;

	if (!buttondown.any()) {
		EllipsePoints over = scan(x,y,state);
		if (over != curpoint || over == ELLP_WildPoint) {
			curpoint = over;
			PostMessage(curpoint != ELLP_None ? PointMessage(curpoint) : nullptr);
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
			Modified();
			needtodraw = 1;

		} else if (curpoint==ELLP_Focus2 || curpoint==ELLP_Focus1) {
			flatpoint f1 = getpoint(ELLP_Focus1, true);
			flatpoint f2 = getpoint(ELLP_Focus2, true);

			if (data->IsCircle()) { f1 += d; f2 += d;} 
			else if (curpoint == ELLP_Focus2) f2 += d;
			else f1 += d; 
			
			data->SetFoci(data->transformPointInverse(f1),data->transformPointInverse(f2),-1); //*** doesn't deal with non-whole ellipse, the segment jumps around
			Modified();
			needtodraw|=2; 

		} else if (curpoint == ELLP_StartAngle) {
			if (data->a > 0 && data->b > 0) {
				flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
				flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
				op.normalize();
				np.normalize();
				ref_point.x += asin(op.cross(np));
				double ang = ref_point.x;
				if (state & ControlMask) ang = (int)(ang / M_PI * 12) * M_PI/12;
				//handle special meaning of end == start
				if (ang > data->start) {
					if (data->start == data->end) data->end = data->start + 2*M_PI;
				} else {
					if (data->start == data->end) data->end = data->start - 2*M_PI;
				}
				data->start = ang;
				if (fabs(data->start - data->end) > 2*M_PI) { //handle when start laps end
					if (data->start > data->end) data->end += 2*M_PI;
					else data->end -= 2*M_PI;
				}
				Modified();
				DBG cerr << "start: "<<data->start<<"  end: "<<data->end<<endl;
				needtodraw = 1;
			}

		} else if (curpoint == ELLP_EndAngle) {
			if (data->a > 0 && data->b > 0) {
				if (data->start == data->end) data->end = data->start + 2*M_PI;
				flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
				flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
				op.normalize();
				np.normalize();
				ref_point.x += asin(op.cross(np));
				double ang = ref_point.x;
				if (state & ControlMask) data->end = (int)(ref_point.x / M_PI * 12) * M_PI/12;
				//handle special meaning of end == start
				if (ang > data->end) {
					if (data->start == data->end) data->end = data->start + 2*M_PI;
				} else {
					if (data->start == data->end) data->end = data->start - 2*M_PI;
				}
				data->end = ang;
				if (fabs(data->start - data->end) > 2*M_PI) { //handle when end laps start
					if (data->start > data->end) data->end += 2*M_PI;
					else data->end -= 2*M_PI;
				}

				Modified();
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

			if ((state & ControlMask) == 0) { //preserve opposite point
				flatpoint constant;
				if (curpoint == ELLP_XRadius)       constant = data->getpoint(ELLP_XRadiusN, true);
				else if (curpoint == ELLP_XRadiusN) constant = data->getpoint(ELLP_XRadius, true);
				else if (curpoint == ELLP_YRadius)  constant = data->getpoint(ELLP_YRadiusN, true);
				else if (curpoint == ELLP_YRadiusN) constant = data->getpoint(ELLP_YRadius, true);
				data->origin(data->origin() + ref_point - constant);
			} else { //preserve original center
				data->origin(data->origin() + ref_point2 - data->getpoint(ELLP_Center, true));
			}

			Modified();

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
			Modified();
			needtodraw = 1;

		} else if (curpoint==ELLP_InnerRadius) {
			if (data->b != 0 && data->a != 0) {
				// flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
				// flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
				// data->inner_r += np.norm() - op.norm();
				// if (data->inner_r < 0) data->inner_r = 0;
				// ----
				// flatpoint op = data->transformPointInverse(screentoreal(lx,ly)) - data->center;
				flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
				double r = data->a / data->b;
				double scale = sqrt((np.x*np.x/r/r) + np.y*np.y) / data->b;
				data->inner_r = scale;
				Modified();
				needtodraw=1;
			}
		
		} else if (curpoint==ELLP_WildPoint) { //move leaves f1,f2 change c
			flatpoint p,p2;
			flatpoint h = screentoreal(x,y);
			p= getpoint(ELLP_Focus1, true) - h;
			p2=getpoint(ELLP_Focus2, true) - h;
			data->SetFoci(getpoint(ELLP_Focus1, false), getpoint(ELLP_Focus2, false),sqrt(p*p)+sqrt(p2*p2));
			// rectify();
			Modified();
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
			Modified();
			needtodraw = 1;

		} else if (curpoint == ELLP_WedgeToggle) {
			if (data->b == 0 || data->a == 0) return 0;
			int ix,iy;
			buttondown.getinitial(mouse->id,LEFTBUTTON,&ix,&iy);
			flatpoint op = data->transformPointInverse(screentoreal(ix,iy)) - data->center;
			flatpoint np = data->transformPointInverse(screentoreal(x,y)) - data->center;
			int w = (np.norm() - op.norm())/(3*NearThreshhold()/dp->Getmag());
			DBG cerr << "wedge norm: "<<(np.norm() - op.norm())<<"  thresh: "<<(NearThreshhold()/dp->Getmag())<<"  w: "<< (np.norm() - op.norm())/(3*NearThreshhold()/dp->Getmag())<<endl;
			int old = data->wedge_type;
			w += ref_wedge;
			if (w >= 2) data->wedge_type = EllipseData::ELLIPSE_Open;
			else if (w <= 0) data->wedge_type = EllipseData::ELLIPSE_Wedge;
			else if (w == 1) data->wedge_type = EllipseData::ELLIPSE_Chord;
			if (old != data->wedge_type) {
				if (data->wedge_type == EllipseData::ELLIPSE_Wedge) PostMessage(_("Use wedge"));
				else if (data->wedge_type == EllipseData::ELLIPSE_Chord) PostMessage(_("Use chord"));
				else PostMessage(_("Use open"));
				Modified();
				needtodraw = 1;
			}
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

		if (curpoint == ELLP_Center) {
			data->center.set(0,0);
			found = true;
		}

		if (found) {
			buttondown.clear();
			data->FindBBox();
			Modified();
			needtodraw = 1;
			curpoint = hover_point = ELLP_None;
		}

		return 0; //don't let delete propagate
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

	sc = new ShortcutHandler(whattype());

	sc->Add(ELLP_ToggleDecs,  'd',0,0,        "ToggleDecs",    _("Toggle decorations"),NULL,0);
	sc->Add(ELLP_ToggleCircle,'c',0,0,        "ToggleCircle",  _("Toggle editing as a circle"),NULL,0);
	sc->Add(ELLP_SetWidth,    'w',0,0,        "ToggleWidth",   _("Toggle width edit"),NULL,0);
	sc->Add(ELLP_ToggleColorDest,'x',0,0,     "ToggleSetColor",_("Toggle set color"),NULL,0);

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
		if (data->b != data->a) {
			data->b = data->a;
			Modified();
			needtodraw=1;
		}
		return 0;

	} else if (action == ELLP_CloseGap) {
		if (!data) return 0;
		data->start = data->end = 0;
		Modified();
		needtodraw=1;
		return 0;

	} else if (action ==  ELLP_UseWedge) {
		if (!data) return 0;
		data->wedge_type = EllipseData::ELLIPSE_Wedge;
		Modified();
		needtodraw=1;
		return 0;

	} else if (action == ELLP_UseChord) {
		if (!data) return 0;
		data->wedge_type = EllipseData::ELLIPSE_Chord;
		Modified();
		needtodraw=1;
		return 0;

	} else if (action == ELLP_UseOpen) {
		if (!data) return 0;
		data->wedge_type = EllipseData::ELLIPSE_Open;
		Modified();
		needtodraw=1;
		return 0;

	} else if (action == ELLP_ToggleFoci) {
		show_foci = !show_foci;
		needtodraw = 1;
		return 0;

	} else if (action == ELLP_SetWidth) {
		if (!data || !data->linestyle) return 0;
		char str[30];
		sprintf(str, "%f", data->linestyle->width);
		LaxFont *font = curwindow->win_themestyle->normal;
		double th = font->textheight();
		double x = (dp->Maxx + dp->Minx)/2;
		double y = (dp->Maxy + dp->Miny)/2;
		DoubleBBox box(x-5*th, x+5*th, y-.75*th, y+.75*th);
		viewport->SetupInputBox(object_id, _("Width"), str, "set_width", box);
		return 0;

	} else if (action == ELLP_ToggleColorDest) {
		color_stroke = !color_stroke;
		PostMessage(color_stroke ? _("Send colors to stroke") : _("Send colors to fill"));
		return 0;
	}

	return 1;
}



} // namespace LaxInterfaces 

