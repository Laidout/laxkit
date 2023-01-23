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
//    Copyright (C) 2004-2011 by Tom Lechner
//


//----------------------------------------------------
//-- gimp ggr gradient format:
// GIMP Gradient
// Name: blah blah blah
// (the number of segments, a single int)
// ->Then lines with 13 (<gimp 2.3.11) or 15 fields:
//  0          Left endpoint coordinate [0..1]
//  1          Midpoint coordinate
//  2          Right endpoint coordinate
//  3          Left endpoint R [0..1]
//  4          Left endpoint G
//  5          Left endpoint B
//  6          Left endpoint A
//  7          Right endpoint R
//  8          Right endpoint G
//  9          Right endpoint B
// 10          Right endpoint A
// 11          Blending function type:
//                   0 = "linear"
//                   1 = "curved"
//                   2 = "sinusoidal"
//                   3 = "spherical (increasing)"
//                   4 = "spherical (decreasing)")
// 12          Coloring type: 0=RGB, 1=HSV CCW, 2=HSV CW
// 13          Left endpoint color type:
//                   0 = "fixed"
//                   1 = "foreground",
//                   2 = "foreground transparent"
//                   3 = "background",
//                   4 = "background transparent
// 14          Right endpoint color type
//
//
//
//-- inkscape/svg gradient format:
//***
//for instance:
//      <radialGradient id="MyGradient"
//      				gradientUnits="userSpaceOnUse"
//                      cx="400" cy="200" r="300"   <--bounding circle, 100% stop
//                      fx="400" fy="200">          <--focus coord, 0% stop
//        <stop offset="0%" stop-color="red" />
//        <stop offset="50%" stop-color="blue" />
//        <stop offset="100%" stop-color="red" />
//      </radialGradient>
//      <linearGradient id="MyGradient"
//          			x1=0 y1=0 x2=1 y2=1
//         				spreadMethod="pad | reflect | repeat"
//          			gradientTransform = "<transform-list>"
//          			gradientUnits = "userSpaceOnUse | objectBoundingBox"
//          			xlink:href = "<uri>">
//        <stop offset="5%" stop-color="#F60" />
//        <stop offset="95%" stop-color="#FF6" />
//      </linearGradient>
//
//
//
//--css gradients: http://dev.w3.org/csswg/css-images-3/#gradient-box
//
//<gradient> =
//  <linear-gradient()> | <repeating-linear-gradient()> |
//  <radial-gradient()> | <repeating-radial-gradient()>
//
// linear-gradient(to top right, red, white, blue)
//
// linear-gradient() = linear-gradient(
//   [ <angle> | to <side-or-corner> ]? ,
//     <color-stop-list>
// )
// <side-or-corner> = [left | right] || [top | bottom]
//
// radial-gradient() = radial-gradient(
//   [ [ circle               || <length> ]                          [ at <position> ]? , |
//     [ ellipse              || [ <length> | <percentage> ]{2} ]    [ at <position> ]? , |
//     [ [ circle | ellipse ] || <extent-keyword> ]                  [ at <position> ]? , |
//     at <position> ,
//   ]?
//   <color-stop> [ , <color-stop> ]+
// )
// <extent-keyword> = closest-corner | closest-side | farthest-corner | farthest-side




#include <lax/interfaces/somedatafactory.h>
#include <lax/language.h>
#include <lax/colors.h>
#include <lax/colorsliders.h>
#include <lax/interfaces/gradientinterface.h>

#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/lists.cc>


using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG


namespace LaxInterfaces {


//#define GRADIENT_RADIAL (1<<0)

//------------------------------ GradientData ----------------------------------

/*! \class GradientData
 * \ingroup interfaces
 * \brief Handled by GradientInterface
 *
 * Can be a linear or a radial gradient. It is not advisable to move around the color
 * spots manually. Call ShiftPoint(). This is to ensure that the colors.e[] array
 * is ordered from lowest t value to highest.
 *
 * For GRADIENT_RADIAL, the center of the start circle is at p1 with radius r1
 * The center of the ending circle is at p2 with radius r2. The spots' t values are
 * distributed as appropriate along the circles.
 *
 * For GRADIENT_LINEAR, the spots are distributed according to the color[]->t values as
 * mapped to the segment p1 to p2.
 * The color extends perpendicularly away from the line p2-p1. If p2-p1 is zero, then
 * use the x axis instead.
 * Derived classes should remember that r1 and r2 can be negative.
 */
/*! \var int GradientData::a
 * \brief A displacement to place the color spot line when drawing on screen.
 *
 * This is automatically determined by GradientInterface.
 */


//! Default constructor, leaves strip null.
GradientData::GradientData()
{
	r1 = 1;
	r2 = 1;
	strip            = nullptr;
	style            = 0;
	usepreview       = 1;
	use_strip_points = false;
	fill_parent      = false;
	spread_method    = LAXSPREAD_None;
}

//! Create new basic gradient pp1 to pp2. Sets col1 at 0 and col2 at 1
/*! This just passes everything to Set().
 */
GradientData::GradientData(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2,unsigned int stle)
  : GradientData()
{
	//r1 = 1;
	//r2 = 1;
	//strip            = nullptr;
	//style            = 0;
	//usepreview       = 1;
	//use_strip_points = false;
	//fill_parent      = false;

	Set(pp1,pp2,rr1,rr2,col1,col2,stle);
}

GradientData::~GradientData()
{
	if (strip) strip->dec_count();
}

/*! Return localized name of spread method.
 */
const char *SpreadMethodTr(int method) 
{
	if      (method == LAXSPREAD_None)    return _("None");
	else if (method == LAXSPREAD_Pad)     return _("Pad");
	else if (method == LAXSPREAD_Reflect) return _("Reflect");
	else if (method == LAXSPREAD_Repeat)  return _("Repeat");
	return nullptr;
}

flatpoint GradientData::P1(flatpoint p)
{
	if (use_strip_points && strip) return strip->p1 = p;
	else return p1 = p;
}

flatpoint GradientData::P1() const
{
	return use_strip_points && strip ? strip->p1 : p1;
}

flatpoint GradientData::P2(flatpoint p)
{
	if (use_strip_points && strip) return strip->p2 = p;
	else return p2 = p;
}

flatpoint GradientData::P2() const
{
	return use_strip_points && strip ? strip->p2 : p2;
}

float GradientData::R1(float r)
{
	if (use_strip_points && strip) return strip->r1 = r;
	else return r1 = r;
}

float GradientData::R1() const
{
	return use_strip_points && strip ? strip->r1 : r1;
}

float GradientData::R2(float r)
{
	if (use_strip_points && strip) return strip->r2 = r;
	else return r2 = r;
}

float GradientData::R2() const
{
	return use_strip_points && strip ? strip->r2 : r2;
}

SomeData *GradientData::duplicate(SomeData *dup)
{
	GradientData *g=dynamic_cast<GradientData*>(dup);
	if (!g && !dup) return NULL; //was not GradientData!

	if (!dup) {
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_GRADIENTDATA));
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			//set=0;
			g = dynamic_cast<GradientData*>(dup);
		}
	}
	if (!g) {
		g = new GradientData();
		dup = g;
	}

	g->style = style;
	g->r1 = r1;
	g->r2 = r2;
	g->p1 = p1;
	g->p2 = p2;
	g->use_strip_points = use_strip_points;
	g->fill_parent      = fill_parent;
	g->spread_method    = spread_method;
	if (strip) {
		GradientStrip *nstrip = dynamic_cast<GradientStrip*>(strip->duplicate(nullptr));
		g->Set(nstrip, 1, false, true);
	}

	 //somedata elements:
	dup->bboxstyle = bboxstyle;
	dup->m(m());
	return dup;
}

//! Convenience function to set style flag to radial.
void GradientData::SetRadial()
{
	style = (style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_RADIAL;
}

//! Convenience function to set style flag to linear.
void GradientData::SetLinear()
{
	style = (style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_LINEAR;
}

/*! Set radial style and color line. Does not change colors in strip.
 */
void GradientData::SetRadial(flatpoint pp1,flatpoint pp2,double rr1,double rr2)
{
	style = (style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_RADIAL;
	P1(pp1);
	P2(pp2);
	R1(rr1);
	R2(rr2);
}

/*! Set linear style and color line. Does not change colors in strip.
 */
void GradientData::SetLinear(flatpoint pp1,flatpoint pp2,double rr1,double rr2)
{
	style = (style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_LINEAR;
	P1(pp1);
	P2(pp2);
	R1(rr1);
	R2(rr2);
}

void GradientData::SetRadial(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2)
{
	Set(pp1,pp2,rr1,rr2,col1,col2,GRADIENT_RADIAL);
}

void GradientData::SetLinear(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2)
{
	Set(pp1,pp2,rr1,rr2,col1,col2,GRADIENT_LINEAR);
}


/*! If own_resource, then newstrip->SetResourceOwner(this) is called.
 */
int GradientData::Set(Laxkit::GradientStrip *newstrip, int absorb, bool keep_placement, bool own_resource)
{
	flatpoint p1,p2;
	double r1,r2;
	if (keep_placement) { p1 = P1(); p2 = P2(); r1 = R1(); r2 = R2(); }
	if (strip != newstrip) {
		if (strip) strip->dec_count();
		strip = newstrip;
	}
	if (strip && !absorb) strip->inc_count();
	if (keep_placement) { P1(p1); P2(p2); R1(r1); R2(r2); }
	return 0;
}

/*! Create a new GradientStrip and set so gradient is pp1 to pp2. Sets col1 at 0 and col2 at 1.
 */
void GradientData::Set(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2,unsigned int stle)
{
	style = stle;

	if (col1 || col2) {
		GradientStrip *newstrip = new GradientStrip(pp1,pp2, rr1,rr2, col1, col2);
		Set(newstrip, 1, false, true);
	}
	if (!use_strip_points) {
		p1 = pp1;
		p2 = pp2;
		r1 = rr1;
		r2 = rr2;
	}

	touchContents();

	//DBG cerr <<"GradientData::Set"<<endl;
	//DBG dump_out(stderr,2,0,NULL);
}

/*! Return the extra transform that aligns p2-p1 to an x axis, with p1 at the origin.
 * Distance to p2 is still (p2-p1).norm().
 * Transforms points from gradient space to object space, or the inverse if invert.
 * If result == null, return a new double[6].
 */
double *GradientData::GradientTransform(double *result, bool invert)
{
	if (result == nullptr) result = new double[6];
	flatpoint v = P2() - P1();

	if (v.norm() < 1e-5) transform_from_basis(result, P1(), flatpoint(1,0), flatpoint(0,1));
	else {
		v.normalize();
		transform_from_basis(result, P1(), v, v.transpose());
	}

	if (invert) {
		double rr[6];
		transform_copy(rr,result);
		transform_invert(result,rr);
	}
	return result;
}

//! Return if pp transformed to data coords is within the bounds.
/*! unimplemented: in=1 | on=2 | out=0
 */
int GradientData::pointin(flatpoint pp,int pin)
{
	if (!strip) return 0;

	double x,y,mm[6];
	transform_invert(mm,m());
	pp = transform_point(mm,pp); //so now in object coords
	double d = (P1() - P2()).norm();

	GradientTransform(mm, true); //object space to gradient space
	pp = transform_point(mm, pp); //now in gradient space
	x = pp.x;
	y = pp.y;


	if (IsLinear()) {
		int in = 0;
		if (R1() > -R2()) 
			in = x >= 0 && x <= d && y >= -R2() && y <= R1();
		else in = x >= 0 && x <= d && y >= R1() && y <= -R2();
		return in;
		
	} else {
		flatpoint pp1 = transform_point(mm, P1());
		flatpoint pp2 = transform_point(mm, P2());

		 //note that it could just be a line though if o1!=o2, that is not dealt with here...
		if (R2() == 0 && R1() == 0) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 0;
		}
		if ((x-pp1.x)*(x-pp1.x)+y*y < R1()*R1()) {
			DBG cerr <<"point in gradient "<<object_id<<endl;
			return 1;
		} // is in circle 1
		if ((x-pp2.x)*(x-pp2.x)+y*y < R2()*R2()) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 1;
		} // is in circle 2

		 //if (in intermediate space)...
		 //for this method, start circle cannot be totally in end circle, and vice versa.
		 //checks whether is inside the trapezoidal region connecting
		 //the starting and ending circles
		if (pp1.x-fabs(R1())>=pp2.x-fabs(R2()) && pp1.x+fabs(R1())<=pp2.x+fabs(R2())) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 0;
		} // circle 1 is inside circle 2
		if (pp2.x-fabs(R2())>=pp1.x-fabs(R1()) && pp2.x+fabs(R2())<=pp1.x+fabs(R1())) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 0;
		} // circle 2 is inside circle 1


		double d = pp2.x-pp1.x, a2;
		double rr1 = R1(),
			   rr2 = R2(),
			   o1 = pp1.x,
			   o2 = pp2.x;
		if (d<0) { //swap rr1,rr2 and o1,o2
			d=-d;
			a2=o2;
			o2=o1;
			o1=a2;
			a2=rr1;
			rr1=rr2;
			rr2=a2;
		}
		double costheta=((rr2-rr1)/d);
		a2 = costheta*rr2;
		if (x<o1-a2*rr1/rr2 || x>o2-a2) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 0;
		}

		double b2,mmm,x0;
		b2 = sqrt(rr2*rr2-a2*a2);
		mmm = sqrt(1/costheta/costheta-1);
		x0 = a2+x-o2;
		if (y<mmm*(x-x0)+b2 && y>-mmm*(x-x0)-b2) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 1;
		}
		DBG cerr <<"point not in gradient "<<object_id<<endl; return 0;

	}

	DBG cerr <<"point not in gradient "<<object_id<<endl;

	return 0;
}


/*! Reads in from something like the following. p1 and p2 can also be vectors.
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  p1 0
 *  p2 1
 *  r1 0
 *  r2 10
 *  colors
 *    radial
 *    spot
 *      t 0
 *      rgba 255 100 50 255
 *    spot
 *      t 1
 *      rgba 0 0 0 0
 * </pre>
 */
void GradientData::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	char *name,*value;
	char *e;
	int found_type = -1;
	SomeData::dump_in_atts(att,flag,context);

	if (att->find("spot")) { //for backwards compatibility
		if (!strip) strip = new GradientStrip();
		strip->dump_in_atts(att, flag, context);
	}

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"linear")) {
			if (BooleanAttribute(value)) SetLinear();
			else SetRadial();
			found_type = IsRadial() ? 2 : 1;

		} else if (!strcmp(name,"radial")) {
			if (!BooleanAttribute(value)) SetLinear();
			else SetRadial();
			found_type = IsRadial() ? 2 : 1;

		} else if (!strcmp(name,"use_strip_points")) {
			use_strip_points = BooleanAttribute(value);

		} else if (!strcmp(name,"fill_parent")) {
			fill_parent = BooleanAttribute(value);

		} else if (!strcmp(name,"spread_method")) {
			int v = -1;
			if (value) {
				if      (!strcasecmp(value, "none"))    v = LAXSPREAD_None;
				else if (!strcasecmp(value, "pad"))     v = LAXSPREAD_Pad;
				else if (!strcasecmp(value, "reflect")) v = LAXSPREAD_Reflect;
				else if (!strcasecmp(value, "repeat"))  v = LAXSPREAD_Repeat;
			}
			if (v != -1) spread_method = v;

		} else if (!strcmp(name,"colors")) {
			if (!strip) strip = new GradientStrip();
			strip->dump_in_atts(att->attributes.e[c], flag, context);
			if (found_type == -1) {
				if (strip->IsRadial()) SetRadial();
				else SetLinear();
			}

		} else if (!strcmp(name,"p1")) {
			flatpoint p;
            if (!FlatvectorAttribute(value,&p,&e)) {
                double d = 0;
                if (DoubleAttribute(value, &d)) {
                    p.x = d;
                    p.y = 0;
                }
            }
			P1(p);

        } else if (!strcmp(name,"p2")) {
			flatpoint p;
            if (!FlatvectorAttribute(value,&p,&e)) {
                double d = 0;
                if (DoubleAttribute(value, &d)) {
                    p.x = d;
                    p.y = 0;
                }
            }
			P2(p);

        } else if (!strcmp(name,"r1")) {
			double r;
            DoubleAttribute(value,&r,&e);
			R1(r);

        } else if (!strcmp(name,"r2")) {
			double r;
            DoubleAttribute(value,&r,&e);
			R2(r);
		}
	}
	hint_a = 0;
	touchContents();

	FindBBox();
}

Laxkit::Attribute *GradientData::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (att == nullptr) att = new Laxkit::Attribute();

	if (what == -1) {
		att->push("matrix", "1 0 0 1 0 0", "the affine transform affecting this gradient");
        att->push("p1", "(0,0)", "the starting coordinate");
        att->push("p2", "(1,0)", "the ending coordinate");
        att->push("r1", "0",     "the starting radius (radial) or the +y extent (linear)");
        att->push("r2", "0",     "the ending radius (radial) or the -y extent (linear)");
		att->push("use_strip_points", "false", "Use the points within gradient strip");
		att->push("fill_parent", "false", "Whether to expand bounding box to match parent, and extend gradient");
		att->push("spread_method", "pad", "or reflect, repeat, or none. How to extend colors past the main definition");
		att->push("linear", nullptr, "If present, gradient is linear");
		att->push("radial", nullptr, "If present, gradient is radial");

		Attribute *att2 = att->pushSubAtt("colors");
		if (strip) strip->dump_out_atts(att2, -1, savecontext);
		else {
			GradientStrip strp;
			strp.dump_out_atts(att2, -1, savecontext);
		}
		return att;
	}

	char scratch[120];
	sprintf(scratch,"%.10g %.10g %.10g %.10g %.10g %.10g",
				m(0),m(1),m(2),m(3),m(4),m(5));
	att->push("matrix",scratch);

	att->push(IsRadial() ? "radial" : "linear");

	const char *str = nullptr;
	if      (spread_method == LAXSPREAD_None)    str = "None";
	else if (spread_method == LAXSPREAD_Pad)     str = "Pad";
	else if (spread_method == LAXSPREAD_Reflect) str = "Reflect";
	else if (spread_method == LAXSPREAD_Repeat)  str = "Repeat";
	if (str) att->push("spread_method", str);

	att->push("use_strip_points", use_strip_points ? "true" : "false");
	att->push("fill_parent", fill_parent ? "true" : "false");
    sprintf(scratch, "(%.10g, %.10g)", P1().x,P1().y);
    att->push("p1", scratch);
    sprintf(scratch, "(%.10g, %.10g)", P2().x,P2().y);
    att->push("p2", scratch);
    att->push("r1", R1());
    att->push("r2", R2());

	if (strip) {
		Attribute *att2 = att->pushSubAtt("colors");
		strip->dump_out_atts(att2,what,savecontext);
	}

	return att;
}

/*! \ingroup interfaces
 * Dump out a GradientData. Prints matrix, p, v, and the spots.
 *
 * If what==-1, then dump out a psuedocode mockup of what gets dumped. This makes it very easy
 * for programs to keep track of their file formats, that is, when the programmers remember to
 * update this code as change happens.
 * Otherwise dumps out in indented data format as described in dump_in_atts().
 */
void GradientData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
    dump_out_atts(&att,what,context);
    att.dump_out(f,indent);
}

//! Find bounding box of a rectangle for linear, or the circles for radial.
void GradientData::FindBBox()
{
	if (fill_parent && GetParent()) {
		cerr << " *** TODO !! implement gradient fill_parent"<<endl;
	}

	if (!strip || strip->colors.n==0) { maxx=maxy=-1; minx=miny=0; return; }

	if (IsRadial()) {
		ClearBBox();

		addtobounds(P1()-flatpoint(fabs(R1()),0));
		addtobounds(P1()+flatpoint(fabs(R1()),0));
		addtobounds(P1()-flatpoint(0,fabs(R1())));
		addtobounds(P1()+flatpoint(0,fabs(R1())));

		addtobounds(P2()-flatpoint(fabs(R2()),0));
		addtobounds(P2()+flatpoint(fabs(R2()),0));
		addtobounds(P2()-flatpoint(0,fabs(R2())));
		addtobounds(P2()+flatpoint(0,fabs(R2())));

	} else {
		double mm[6];
		GradientTransform(mm, false);

		ClearBBox();
		addtobounds(P1());
		addtobounds(P2());
		addtobounds(transform_point(mm,0, R1()));
		addtobounds(transform_point(mm,0,-R1()));

		double d = (P2()-P1()).norm();
		addtobounds(transform_point(mm,d, R2()));
		addtobounds(transform_point(mm,d,-R2()));
	}
	//DBG cerr <<"gradient "<<object_id<<": x:"<<minx<<','<<maxx<<"  y:"<<miny<<','<<maxy<<endl;
}

/*! Return the t of colors.e[i] mapped to the range [0..1] where
 * 0 is colors.e[0] and 1 is colors.e[colors.n-1].
 */
double GradientData::GetNormalizedT(int i)
{
	if (!strip) return 0;
	if (i < 0 || i >= strip->colors.n) return 0;
	return (strip->colors.e[i]->t - strip->colors.e[0]->t)/(strip->colors.e[strip->colors.n-1]->t - strip->colors.e[0]->t);
}

//! Move the color which to a new t position.
/*! Note this is the GradientData independent t value.
 */
int GradientData::ShiftPoint(int which,double dt, bool clamp)
{
	if (!strip) return -1;
	which = strip->ShiftPoint(which, dt, clamp);
	FindBBox();
	touchContents();
	return which;
}

//! Flip the order of the colors.
void GradientData::FlipColors()
{
	if (!strip) return;
	strip->FlipColors();
	touchContents();
}

int GradientData::NumColors()
{
	if (!strip) return 0;
	return strip->colors.n;
}

//! Takes pointer, does not make duplicate.
int GradientData::AddColor(GradientStrip::GradientSpot *spot)
{
	if (!strip) strip = new GradientStrip(0);
	int c = strip->AddColor(spot);
	touchContents();
	return c;
}

//! Add a spot with the given color, or interpolated, if col==NULL.
int GradientData::AddColor(double t,ScreenColor *col)
{
	if (!strip) strip = new GradientStrip(0);
	if (col) {
		touchContents();
		return strip->AddColor(t, col);
	}

	ScreenColor c;
	WhatColor(t,&c,false);
	touchContents();
	return strip->AddColor(t, &c);
}

//! Place new color in right spot in list.
/*! The color components are in range [0,0xffff].
 */
double GradientData::AddColor(double t,double red,double green,double blue,double alpha)
{
	if (!strip) strip = new GradientStrip(0);
	touchContents();
	return strip->AddColor(t, red,green,blue,alpha);
}

/*! From coordinate in data space, return the color at it.
 * Return 0 for success, or nonzero for coordinate out of range.
 */
int GradientData::WhatColor(flatpoint p, Laxkit::ScreenColor *col, bool is_normalized)
{
	if (!strip) return 1;

	double mm[6];
	GradientTransform(mm, true);
	p = transform_point(mm, p);
	flatpoint pp1 = transform_point(mm, P1());
	flatpoint pp2 = transform_point(mm, P2());

	double x=p.x;
	double y=p.y;

	if (!IsRadial()) {
		 //linear gradient, much easier
		if (R1() >R2()) {
			if (y>R1() || y<R2()) return 1; //out of y bounds
		} else if (y<R1() || y>R2()) return 2;

		if (pp1.x < pp2.x) {
			if (x<pp1.x || x>pp2.x) return 3;
		} else if (x>pp1.x || x<pp2.x) return 4;

		return WhatColor(strip->colors.e[0]->t + (strip->colors.e[strip->colors.n-1]->t - strip->colors.e[0]->t)*(x-pp1.x)/(pp2.x-pp1.x), col, is_normalized);
	}

	 //else radial gradient
//	***
//	if (p2+r2<=p1+r1 && p2-r2>=p1-r1) {
//		 //circle 2 is entirely contained in circle 1
//	} else if (p2+r2>=p1+r1 && p2-r2<=p1-r1) {
//		 //circle 1 is entirely contained in circle 2
//	}
	 // ***** HACK! just looks in plane circle 2 radius centered at p2
	return WhatColor(strip->colors.e[0]->t + (strip->colors.e[strip->colors.n-1]->t-strip->colors.e[0]->t)*norm(p - flatpoint(pp2.x,0))/R2(), col, is_normalized);
}

//! Figure out what color lays at coordinate t.
/*! If t is before the earliest point then the earliest point is used
 * for the color, and -1 is returned. Similarly for beyond the final point, but
 * 1 is returned. Otherwise, the color is linearly interpolated between
 * the nearest points, and 0 is returned.
 */
int GradientData::WhatColor(double t,ScreenColor *col, bool is_normalized)
{
	if (!strip) return -2;
	return strip->WhatColor(t, col, is_normalized);
}

//! Figure out what color lays at coordinate t.
/*! If t is before the earliest point then the earliest point is used
 * for the color, and -1 is returned. Similarly for beyond the final point, but
 * 1 is returned. Otherwise, the color is linearly interpolated between
 * the nearest points, and 0 is returned.
 *
 * The colors are returned as doubles in range [0..1]. It is assumed that col
 * has as many channels as needed for color (with alpha). For most cases,
 * rgba (4 channels, so a double[4]) is sufficient.
 *
 * \todo warning: assumes argb for now... ultimately, should be arranged
 *   according to the color system of the colors
 */
int GradientData::WhatColor(double t,double *col, bool is_normalized)
{
	if (!strip) return -2;
	ScreenColor cc;
	int c = WhatColor(t, &cc, is_normalized);

	col[0]= cc.Alpha();
	col[1]= cc.Red();
	col[2]= cc.Green();
	col[3]= cc.Blue();

	return c;
}

float GradientData::MinT()
{
	return strip ? strip->MinT() : 0;
}

float GradientData::MaxT()
{
	return strip ? strip->MaxT() : 0;
}

//! Render the whole gradient to a buffer.
/*! The entire buffer maps to the gradient's bounding box.
 *
 * bufchannels must be the same number of channels as the number of channels of the colors of the gradient.
 * The last channel is assumed to be the alpha channel.
 * bufstride is the number of bytes each row takes.
 * bufdepth can be either 8 or 16.
 *
 * Currently not antialiased. Please note this is mainly for generating preview images
 * for use on screen. 16 bit stuff should really
 * be implented with a Displayer capable of 16 bit buffers and transforms.
 *
 * Return 0 for success, or nonzero for error at some point.
 */
int GradientData::renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels)
{ // ***
	DBG cerr <<"...need to implement GradientData::renderToBuffer()!!"<<endl;
	return 1;
}

////--------------------------------- GradientInterface ----------------------------


/*! \class GradientInterface
 * \ingroup interfaces
 * \brief Manipulates GradientData objects.
 *
 * With this interface, you can change gradients right on the screen
 * and move around the little color indicators without having to open
 * some other gradient editor.
 *
 * \todo figure out how to incorporate color mapping functions..
 */
/*! \var Laxkit::NumStack<int> GradientInterface::curpoints
 * \brief Stack of currently selected color spots.
 */
/*! \var flatpoint GradientInterface::leftp
 * \brief the point in the gradient space that the button is clicked down on
 */
/*! \var int GradientInterface::usepreview
 * \brief Whether to draw the gradient on each refresh or use a cached preview.
 */
/*! \var int GradientInterface::gradienttype
 * \brief 0 for either radial or linear. 1 for linear only. 2 for radial only.
 */

//internal helpers to remember control points..
//>=0 are the color spots
#define GP_Nothing     -11
#define GP_Nodata      -10
#define GP_OutsideData -9
#define GP_DataNoPoint -8
#define GP_a           -7
#define GP_r1          -6
#define GP_r2          -5
#define GP_p1          -4
#define GP_p1_bar      -3
#define GP_p2          -2
#define GP_p2_bar      -1
//users can still select points down to GP_MinMoveable
#define GP_MinMoveable -7
#define GP_Min         -8

const char *PointName(int which)
{
	if (which == GP_Nothing    ) return "GP_Nothing    ";
	if (which == GP_Nodata     ) return "GP_Nodata     ";
	if (which == GP_OutsideData) return "GP_OutsideData";
	if (which == GP_DataNoPoint) return "GP_DataNoPoint";
	if (which == GP_a          ) return "GP_a          ";
	if (which == GP_r1         ) return "GP_r1         ";
	if (which == GP_r2         ) return "GP_r2         ";
	if (which == GP_p1         ) return "GP_p1         ";
	if (which == GP_p2         ) return "GP_p2         ";
	if (which == GP_p1_bar     ) return "GP_p1_bar     ";
	if (which == GP_p2_bar     ) return "GP_p2_bar     ";
	if (which == GP_MinMoveable) return "GP_MinMoveable";
	if (which == GP_Min        ) return "GP_Min        ";
	return "";
}


//! Default is a linear black to red.
GradientInterface::GradientInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	usepreview=0;
	controlcolor=rgbcolor(0,148,178);
	style = 0;

	col1.rgbf(0.,0.,0.,1.);
	col2.rgbf(1.,1.,1.,1.);

	gradienttype = 0;
	strip = nullptr;
	data  = nullptr;
	goc   = nullptr;
	showdecs = ShowControls;
	curpoint = GP_Nothing;
	creationstyle = 0;
	createv = flatpoint(20,0);
	creationstyle = GradientData::GRADIENT_LINEAR;
	creater1 = 50;
	creater2 = 50;
	strip_data = nullptr;
	show_settings_icon = false;
	settings_icon = nullptr;

	needtodraw = 1;
	sc = NULL;
}

//! Empty destructor.
GradientInterface::~GradientInterface()
{
	deletedata();
	if (sc) sc->dec_count();
	if (strip_data) strip_data->dec_count();
	if (settings_icon) settings_icon->dec_count();
	DBG cerr <<"----in GradientInterface destructor"<<endl;
}

//! Return dup of this. Copies over creationstyle, creatp/v, createlen, col1, col2.
anInterface *GradientInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new GradientInterface(id,NULL);
	else if (!dynamic_cast<GradientInterface *>(dup)) return NULL;
	GradientInterface *dupp=dynamic_cast<GradientInterface *>(dup);
	dupp->col1=col1;
	dupp->col2=col2;
	dupp->creationstyle=creationstyle;
	dupp->creater1=creater1;
	dupp->creater2=creater2;
	dupp->createv=createv;
	dupp->style = style;
	return anInterface::duplicate(dup);
}

/*! Differentiates based on gradienttype.
 */
const char *GradientInterface::IconId()
{
	if (gradienttype==2) return "RadialGradient";
	if (gradienttype==1) return "LinearGradient";
	return "Gradient";
}

/*! Differentiates based on gradienttype.
 */
const char *GradientInterface::Name()
{
	if (gradienttype==2) return _("Radial Gradient");
	if (gradienttype==1) return _("Linear Gradient");
	return _("Gradient");
}

//! Sets showdecs to show colors only, and needtodraw=1.
int GradientInterface::InterfaceOn()
{
	showdecs = ShowColors | ShowControls;
	needtodraw = 1;
	return 0;
}

//! Calls Clear(), sets showdecs=0, and needtodraw=1.
int GradientInterface::InterfaceOff()
{
	Clear(NULL);
	curpoint = GP_Nothing;
	curpoints.flush();
	showdecs = 0;
	needtodraw = 1;
	return 0;
}

//! Basically clear data. Decrement its count, and set to NULL.
void GradientInterface::deletedata()
{
	if (strip) { strip->dec_count(); strip = nullptr; }
	if (data) { data->dec_count(); data = nullptr; }
	if (goc) { delete goc; goc = nullptr; }
	curpoints.flush();
	curpoint = GP_Nothing;
}

void GradientInterface::SetupRect(double x,double y,double w,double h)
{
	if (w<0) { x += w; w = -w; }
	if (h<0) { y += h; h = -h; }
	bounds.setbounds(x,x+w, y,y+h);

	if (data && (style & EditStrip)) {
		data->P1(bounds.BBoxPoint(0,.5));
		data->P2(bounds.BBoxPoint(1,.5));
		data->R1(bounds.boxheight()/2);
		data->R2(bounds.boxheight()/2);
		data->FindBBox();
	}
}

/*! Edit a linear strip in bounds.
 * Will increment the count.
 */
int GradientInterface::UseThisObject(Laxkit::GradientStrip *nstrip)
{
	if (!nstrip) return 0;

	//set up a proxy GradientData corresponding to this strip
	if (nstrip == strip) {
		//strip->inc_count();
		needtodraw=1;
		return 1;
	}

	if (data != strip_data) deletedata();
	if (nstrip != strip) {
		if (strip) strip->dec_count();
		strip = nstrip;
		strip->inc_count();
	}

	style |= EditStrip;

	if (!bounds.validbounds()) {
		bounds.setbounds(0,1, 0,1);
	}

	data = dynamic_cast<GradientData*>(somedatafactory()->NewObject(LAX_GRADIENTDATA));

	data->SetLinear(flatpoint(bounds.minx, bounds.boxheight()/2), flatpoint(bounds.maxx, bounds.boxheight()/2),
				bounds.boxheight()/2, bounds.boxheight()/2,
				nullptr,nullptr);
	data->Set(strip, 0, true, true);
	data->FindBBox();

	curpoints.flush();
	curpoint=GP_Nothing;
	needtodraw=1;
	return 1;
}

int GradientInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	GradientData *ndata=dynamic_cast<GradientData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data != ndata) deletedata();
	if (goc) delete goc;
	goc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}

	curpoints.flush();
	curpoint=GP_Nothing;
	needtodraw=1;
	return 1;
}


ObjectContext *GradientInterface::Context()
{
	return goc; //if strip, this should be null
}

//! Uses GradientData and foreground of a LineStyle
int GradientInterface::UseThis(anObject *newdata,unsigned int) // assumes not use local
{
	if (!newdata) return 0;
	if (newdata==data) return 1;

	if (data && dynamic_cast<LineStyle *>(newdata)) { // make all selected points have this color
		DBG cerr <<"Grad new color stuff"<< endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(newdata);
		if (data && curpoints.n && (nlinestyle->mask & (LINESTYLE_Color | LINESTYLE_Color2))) {
			//int r=data->strip->colors.e[c]->red,g=data->strip->colors.e[c]->green,b=data->strip->colors.e[c]->blue;
			for (int c=0; c<curpoints.n; c++) {
				data->strip->SetColor(curpoints.e[c], &nlinestyle->color);
			}
			data->touchContents();
			needtodraw=1;
		}
		col1=col2;
		col2.red  =nlinestyle->color.red;
		col2.green=nlinestyle->color.green;
		col2.blue =nlinestyle->color.blue;
		col2.alpha=nlinestyle->color.alpha;
		needtodraw=1;
		return 1;

//	} else if (dynamic_cast<GradientData *>(newdata)) {
//		 //change viewport current object to the data, or drop down a new one
//		deletedata();
//		GradientData *ndata=dynamic_cast<GradientData *>(newdata);
//		if (viewport) {
//			if (viewport->ChangeObject(ndata,NULL)==0) {
//				viewport->NewData(ndata);
//			}
//		}
//		curpoints.flush();
//		curpoint=GP_Nothing;
//		data=ndata;
//		data->inc_count();
//		needtodraw=1;
//		return 1;
	}

	return 0;
}

//! Standard drawdata function.
int GradientInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
	if (!ndata || dynamic_cast<GradientData *>(ndata)==NULL) return 1;
	GradientData *gd=data;
	data=dynamic_cast<GradientData *>(ndata);
	int td = showdecs, ntd = needtodraw;
	showdecs = ShowColors;
	needtodraw = 1;
	Refresh();
	needtodraw=ntd;
	showdecs=td;
	data=gd;
	return 1;
}

/*! Draw with Displayer functions.
 */
void GradientInterface::drawLinear2()
{
	flatpoint p1 = data->P1();
	flatpoint p2 = data->P2();

	double offsets[data->strip->colors.n];
	ScreenColor colors[data->strip->colors.n];

	for (int c=0; c<data->strip->colors.n; c++) {
		offsets[c] = data->GetNormalizedT(c);
		colors[c]  = data->strip->colors.e[c]->color->screen;
	}

	dp->setLinearGradient(data->spread_method, p1.x,p1.y, p2.x,p2.y, offsets, colors, data->NumColors());

	double mm[6];
	bool no_parent = true;
	if (data->fill_parent) {
		SomeData *pnt = data->GetParent();
		if (pnt && pnt->nonzerobounds()) {
			no_parent = false;
			transform_invert(mm, data->m());
			dp->moveto(transform_point(mm, pnt->BBoxPoint(0,0,false)));
			dp->lineto(transform_point(mm, pnt->BBoxPoint(1,0,false)));
			dp->lineto(transform_point(mm, pnt->BBoxPoint(1,1,false)));
			dp->lineto(transform_point(mm, pnt->BBoxPoint(0,1,false)));
		}
	}

	if (no_parent) {
		data->GradientTransform(mm, false);
		double d = (p2-p1).norm();

		dp->moveto(transform_point(mm, 0, data->R1()));
		dp->lineto(transform_point(mm, d, data->R1()));
		dp->lineto(transform_point(mm, d,-data->R2()));
		dp->lineto(transform_point(mm, 0,-data->R2()));
	}
	dp->closed();
	dp->fill(0);

	dp->NewFG(dp->FG());
}

//! Draw linear gradient. Called from Refresh.
/*! This assumes that dp has transform to object space.
 *
 * Draws in x:[p1,p2], y:[r1,-r2]
 */
void GradientInterface::drawLinear()
{
	if (dp->Capability(DRAWS_LinearGradient)) { drawLinear2(); return; }


	// if the x span is larger than the y span, then for each
	// x pixel draw a line, otherwise for each y pixel.
	dp->DrawScreen();

	flatpoint cp,x0,x1,v,v1,v2;
	double clen,cstart;
	ScreenColor col,color0,color1;
	int len,c,c2;

	double mm[6];
	data->GradientTransform(mm, false);
	double d = (data->P2() - data->P1()).norm();

	cstart = data->strip->colors.e[0]->t;
	clen = data->strip->colors.e[data->strip->colors.n-1]->t - cstart;

	for (c=0; c<data->strip->colors.n-1; c++) {
		x0 = dp->realtoscreen(transform_point(mm, flatpoint(d*(data->strip->colors.e[c  ]->t-cstart)/clen,0)));
		x1 = dp->realtoscreen(transform_point(mm, flatpoint(d*(data->strip->colors.e[c+1]->t-cstart)/clen,0)));

		if (x1.x<x0.x) {//go in decreasing x dir
			v  = x0-x1;
			v1 = x0;
			x0 = x1;
			x1 = v1;
			color1 = data->strip->colors.e[ c ]->color->screen;
			color0 = data->strip->colors.e[c+1]->color->screen;

		} else {//go in increasing x dir
			v=x1-x0;
			color0 = data->strip->colors.e[ c ]->color->screen;
			color1 = data->strip->colors.e[c+1]->color->screen;
		}

		if (v.x > fabs(v.y)) { // for each x pixel..
			//start=(int)x0.x;
			len = (int)(v.x+.5);

		} else { // for each y pixel
			//start=(int)x0.y;
			len=(int)(v.y+.5);
			if (len<0) {
				len=-len;
				col=color0;
				color0=color1;
				color1=col;
				v=-v;
				x0=x1;
			}
		}

		v1 = dp->realtoscreen(transform_point(mm, flatpoint(0,data->R1())) - dp->realtoscreen(flatpoint(0,0)));
		v2 = dp->realtoscreen(transform_point(mm, flatpoint(0,data->R2())) - dp->realtoscreen(flatpoint(0,0)));

		for (c2=0; c2<=len; c2++) {
			coloravg(&col,&color0,&color1,(float)c2/len);
			cp = x0+v*((float)c2/len);
			dp->NewFG(&col);
			dp->drawline(cp+v1,cp-v2);
		}
	}
	dp->DrawReal();

	// other potential drawing methods: draw first/last, then middle, recursing?
}

/*! Draw with Displayer functions.
 */
void GradientInterface::drawRadial2()
{
	DBG cerr <<" ....drawing with GradientInterface::drawRadial2()"<<endl;

	if (!data->strip) return;

	flatpoint p1 = data->P1();
	flatpoint p2 = data->P2();

	double offsets[data->strip->colors.n];
	ScreenColor colors[data->strip->colors.n];

	for (int c=0; c<data->strip->colors.n; c++) {
		offsets[c] = data->GetNormalizedT(c);
		colors[c]  = data->strip->colors.e[c]->color->screen;
	}

	 //the number: 0=none, 1=repeat, 2=reflect, 3=pad
	dp->setRadialGradient(data->spread_method, p1.x,p1.y,data->R1(), p2.x,p2.y,data->R2(), offsets, colors, data->strip->colors.n);

	double mm[6];
	bool no_parent = true;
	if (data->fill_parent) {
		SomeData *pnt = data->GetParent();
		if (pnt && pnt->nonzerobounds()) {
			no_parent = false;
			transform_invert(mm, data->m());
			dp->moveto(transform_point(mm, pnt->BBoxPoint(0,0,false)));
			dp->lineto(transform_point(mm, pnt->BBoxPoint(1,0,false)));
			dp->lineto(transform_point(mm, pnt->BBoxPoint(1,1,false)));
			dp->lineto(transform_point(mm, pnt->BBoxPoint(0,1,false)));
			dp->closed();
		}
	}

	if (no_parent) {
		dp->drawrectangle(data->minx,data->miny, data->maxx-data->minx, data->maxy-data->miny, 1);
	}
	dp->fill(0);
}

//! Draw radial gradient. Called from Refresh.
/*! This assumes that dp has transform to object space.
 *
 * \todo  this could be rather a lot smarter... like by finding how many
 * XPoints to generate based on the radius of the given circle...
 */
void GradientInterface::drawRadial()
{
	if (dp->Capability(DRAWS_RadialGradient)) { drawRadial2(); return; }


	flatpoint p,cp,O1,O2,o1,o2,xaxis,yaxis,v;
	double R1,R2,r1,r2,r,s,cstart,clen;
	ScreenColor col,color0,color1;
	int len,c,c2;
	//int ell=30;//ell is number of points to approximate circles with
	//flatpoint points[ell];

	dp->DrawScreen();
	O1 = dp->realtoscreen(data->P1()); //the data's p1 and p2
	O2 = dp->realtoscreen(data->P2());
	cstart = data->strip->colors.e[0]->t;
	clen = data->strip->TRange();

	R1 = data->R1();
	R2 = data->R2();

	xaxis = dp->realtoscreen(flatpoint(1,0))-dp->realtoscreen(flatpoint(0,0));
	s = 1/norm(xaxis);
	xaxis *= s;
	yaxis = dp->realtoscreen(flatpoint(0,1))-dp->realtoscreen(flatpoint(0,0));
	yaxis *= s;

	 //for each color segment...
	for (c=0; c<data->NumColors()-1; c++) {
		o1 = O1+(O2-O1)*(data->strip->colors.e[c  ]->t-cstart)/clen; //this color segment's start and end centers
		o2 = O1+(O2-O1)*(data->strip->colors.e[c+1]->t-cstart)/clen;
		r1 = 1/s*(R1+(R2-R1)*(data->strip->colors.e[c  ]->t-cstart)/clen);//segment's start and end radii
		r2 = 1/s*(R1+(R2-R1)*(data->strip->colors.e[c+1]->t-cstart)/clen);
		v = o2-o1;

		color0 = data->strip->colors.e[ c ]->color->screen;
		color1 = data->strip->colors.e[c+1]->color->screen;
		len = (int)(norm(v)+fabs(r1-r2)+.5); //the number of circles to draw so as to have no gaps hopefully

		for (c2=0; c2<len; c2++) {
			cp = o1+v*((float)c2/len); //center of current circle
			//DBG dp->draw((int)cp.x,(int)cp.y,7,7,0,9);
			r = r1+(float)c2/len*(r2-r1); //radius of current circle

			//for (c3=0; c3<ell; c3++) {
			//	p=cp + r*cos((float)c3/(ell-1)*2*M_PI)*xaxis + r*sin((float)c3/(ell-1)*2*M_PI)*yaxis;
			//	points[c3].x=(int)p.x;
			//	points[c3].y=(int)p.y;
			//	//DBG cerr <<c2<<":"<<c3<<": p="<<p.x<<','<<p.y<<"  "<<points[c3].x<<','<<points[c3].y<<endl;
			//}
			coloravg(&col,&color0,&color1,(float)c2/len);
			dp->NewFG(&col);
			dp->drawcircle(cp, r, 0);
			//dp->drawlines(points,ell,1,0);
		}
	}
	dp->DrawReal();
}

//! Draw a single line at t, where t==0 is data->p1 and t==1 is data->p2.
/*! Uses the current color and line width settings. This should only be called
 * from Refresh, as it assumes dp to be set to gradient space.
 */
void GradientInterface::drawRadialLine(double t, bool thick)
{
	flatpoint o,p,cp,O1,O2,o1,o2,xaxis,yaxis,v;
	double r,s;
	int c3,ell=30;
	flatpoint points[ell];


	O1 = dp->realtoscreen(data->P1());
	O2 = dp->realtoscreen(data->P2());
	o = (1-t)*O1+t*O2;

	xaxis = dp->realtoscreen(flatpoint(1,0))-dp->realtoscreen(flatpoint(0,0));
	s = 1/norm(xaxis);
	xaxis *= s;
	yaxis = dp->realtoscreen(flatpoint(0,1))-dp->realtoscreen(flatpoint(0,0));
	yaxis *= s;

	r = ((1-t)*data->R1() + t*data->R2())/s;
	for (c3=0; c3<ell; c3++) {
		p = o + r*cos((float)c3/(ell-1)*2*M_PI)*xaxis + r*sin((float)c3/(ell-1)*2*M_PI)*yaxis;
		points[c3].x = p.x;
		points[c3].y = p.y;
		//DBG cerr <<c2<<":"<<c3<<": p="<<p.x<<','<<p.y<<"  "<<points[c3].x<<','<<points[c3].y<<endl;
	}
	dp->DrawScreen();
	dp->LineWidthScreen(thick ? 6 : 1);
	dp->drawlines(points,ell,1,0);
	dp->DrawReal();
}

//assume in object coords
void GradientInterface::DrawOutline(double width, double r,double g,double b)
{
	dp->LineWidthScreen(width);
	dp->NewFG(r,g,b);

	double mm[6];
	data->GradientTransform(mm, false);

	if (data->IsLinear()) {
		flatpoint ul,ur,ll,lr;
		double d = (data->P2() - data->P1()).norm();
		ul = transform_point(mm, flatpoint(0, data->R1()));
		ur = transform_point(mm, flatpoint(d, data->R1()));
		ll = transform_point(mm, flatpoint(0,-data->R2()));
		lr = transform_point(mm, flatpoint(d,-data->R2()));

		dp->drawline(ul, ur);
		dp->drawline(ur, lr);
		dp->drawline(lr, ll);
		dp->drawline(ll, ul);
	}
}


//! Refresh, calls drawLinear() and drawRadial(), then draws decorations.
/*! \todo the control points should be drawn differently, p1 and p2 should be little
 * triangles that point to each other, and r1 and r2 should point to each other.
 */
int GradientInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!data || !data->strip) {
		if (needtodraw) needtodraw=0;
		return 1;
	}


	//DBG cerr <<"  GradientRefresh-";

	dp->LineAttributes(2,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);

	// DBG dp->drawpoint(dp->realtoscreen(data->transformPointInverse(hoverdbg)), 2,1);
	// DBG cerr <<"Gradient hover scr: "<<screendbg.x<<','<<screendbg.y<<"  hoverdbg: "<<hoverdbg.x << ','<<hoverdbg.y
	// DBG      <<"     bounds: x:"<<bounds.minx<<','<<bounds.maxx<<" y:"<<bounds.miny<<','<<bounds.maxy<<endl;

	// draw the color
	if ((showdecs & ShowColors) && data->strip->colors.n) {
		int d = 1;
		if (usepreview) {
			LaxImage *preview = data->GetPreview();
			if (preview) {
				d = dp->imageout(preview,data->minx,data->miny, data->maxx-data->minx, data->maxy-data->miny);
				if (d<0) d=1; else d=0; //draw lines if problem with image
			}
			DBG if (d) cerr<<"- - - gradient didn't used preview image"<<endl;
			DBG else cerr <<"- - - gradient used preview image "<<preview->w()<<"x"<<preview->h()<<endl;
		}

		if (d) {
			if (data->IsRadial()) drawRadial();
			else drawLinear(); // is GRADIENT_LINEAR
			//else drawLinear2(); // is GRADIENT_LINEAR
		}
	}

	//dp->drawline(data->minx,data->miny, data->maxx,data->maxy);
	//dp->drawline(data->maxx,data->miny, data->minx,data->maxy);
	//dp->drawline(data->P1(), data->P2());


	// draw control points
	if (showdecs & ShowControls) {
		dp->BlendMode(LAXOP_Over);
		double thin = ScreenLine();

		//DBG draw outline:
		DrawOutline(1, 0,0,1);

		dp->NewFG(controlcolor);
		dp->DrawReal();
		dp->LineWidthScreen(thin);

		flatpoint p;
		int c;

		// draw arrow on p1 to p2 and a line between color spots
		//dp->drawline(data->P1(), data->P2());
		if (style & EditStrip) {
		} else dp->drawarrow(data->P1(), data->P2() - data->P1(), 0,1,2);
		//dp->drawarrow(getpoint(GP_p1,0), getpoint(GP_p2,0) - getpoint(GP_p1,0), 0,1,2);


		if (data->IsRadial()) {
			drawRadialLine(0, curpoint == GP_r1 || curpoint == GP_p1 || curpoint == 0);
			drawRadialLine(1, curpoint == GP_r2 || curpoint == GP_p2 ||curpoint == data->NumColors()-1);

			//draw line for color strip on radial
			if (data->strip->colors.n > 1) {
				dp->NewFG(controlcolor);
				dp->LineWidthScreen(1);
				dp->drawline(getpoint(0,0), getpoint(data->strip->colors.n-1, 0));
			}
		}

		dp->DrawScreen();
		dp->LineWidthScreen(1);

		if (data->IsRadial()) { //draw little nuggets for p1 and p2
			dp->NewFG(controlcolor);
			dp->drawpoint(dp->realtoscreen(getpoint(GP_p1,0)),2*thin,1);
			dp->drawpoint(dp->realtoscreen(getpoint(GP_p2,0)),2*thin,1);
		}

		// draw the spots (draw open)
		for (int c=0; c<data->strip->colors.n; c++) {
			p = dp->realtoscreen(getpoint(c,0));
			dp->NewFG(&data->strip->colors.e[c]->color->screen);
			dp->drawpoint(p.x,p.y,(c==curpoint ? 5*thin : 3*thin),1);
			dp->NewFG(controlcolor);

			//if (c<0) dp->draw((int)p.x,(int)p.y,5,5,0,(c==GP_p1 || c==GP_p2)?3:2);
			//else
			dp->drawpoint(p.x, p.y, (c == curpoint ? 5*thin : 3*thin), 0);
		}
		dp->DrawReal();

		//draw special points
		if (curpoint<0 && curpoint>=GP_Min) {
			flatpoint ul,ur,ll,lr;
			double mm[6];
			data->GradientTransform(mm, false);
			double d = (data->P2() - data->P1()).norm();
			ul = transform_point(mm, flatpoint(0, data->R1()));
			ur = transform_point(mm, flatpoint(d, data->R1()));
			ll = transform_point(mm, flatpoint(0,-data->R2()));
			lr = transform_point(mm, flatpoint(d,-data->R2()));
			dp->LineWidthScreen(6);
			dp->NewFG(controlcolor);

			if (data->IsLinear()) {
				if (curpoint == GP_p1 || curpoint == GP_p1_bar) {
					dp->drawline(ul,ll);

				} else if (curpoint == GP_p2 || curpoint == GP_p2_bar) {
					dp->drawline(ur,lr);

				} else if (curpoint == GP_r1) {
					dp->drawline(ul,ur);

				} else if (curpoint == GP_r2) {
					dp->drawline(ll,lr);
				}
			}
		}


		//draw selected color spots bigger
		if (curpoints.n) {
			dp->DrawScreen();
			dp->LineWidthScreen(1);

			for (c=0; c<curpoints.n; c++) {
				p = dp->realtoscreen(getpoint(curpoints.e[c],0));
				dp->NewFG(&data->strip->colors.e[curpoints.e[c]]->color->screen);
				dp->drawpoint((int)p.x,(int)p.y, 5*thin, 1);  // draw curpoint
				dp->NewFG(controlcolor);
				dp->drawpoint((int)p.x,(int)p.y, 5*thin, 0);
			}
			dp->DrawReal();
		}
		dp->BlendMode(LAXOP_Over);
		dp->DrawReal();

	}
	//DBG cerr <<endl;
	needtodraw=0;
	return 0;
}

//! Return a flatpoint corresponding to point index c in data space coordinates, or transformed if (trans).
/*! c is (see GP_* in code):
 * <pre>
 *   -5  a
 *   -4  linear:(a,r1),  radial:(p1,r1) rotated by a
 *   -3  linear:(a,-r2), radial:(p2,r2) rotated by a
 *   -2  (p1,0)
 *   -1  (p2,0)
 *   >=0 color spot index.
 *  </pre>
 *  Otherwise (0,0) is returned.
 *
 *  If trans, then transform the point by data->m().
 */
flatpoint GradientInterface::getpoint(int c, int trans)
{
	if (!data || !(c >= GP_MinMoveable && c<data->NumColors())) return flatpoint();

	flatpoint p;
	double d = (data->P2() - data->P1()).norm();

	if (c == GP_a) {
		if (data->IsRadial())
			p = flatpoint(cos(data->hint_a)*data->R1(), sin(data->hint_a)*data->R1());
		else p = flatpoint(0,data->hint_a);

	} else if (c == GP_r1) //r1
		if (data->IsRadial()) p = flatpoint(0,data->R1());
		else p = flatpoint(0,data->R1());

	else if (c == GP_r2) //r2
		if (data->IsRadial()) p = flatpoint(d,data->R2());
		else p = flatpoint(0,-data->R2());

	else if (c == GP_p2 || curpoint == GP_p2_bar) p.x = d;
	else if (c == GP_p1 || curpoint == GP_p1_bar) p.x = 0;
	else {
		// is a color point
		double cstart = data->strip->colors.e[0]->t,
			   clen = data->strip->colors.e[data->strip->colors.n-1]->t-cstart;

		flatpoint p1,p2;
		if (data->IsRadial()) {
			// distribute at angle ??? corresponding to their spot on screen..
			double hint_a = data->hint_a;
			p1 = flatpoint(0 + cos(hint_a)*data->R1(), sin(hint_a)*data->R1());
			p2 = flatpoint(d + cos(hint_a)*data->R2(), sin(hint_a)*data->R2());
		} else {
			p1 = flatpoint(0, data->hint_a);
			p2 = flatpoint(d, data->hint_a);
		}
		if (clen==0) p = p1;
		else p = p1 + (data->strip->colors.e[c]->t - cstart)/clen * (p2-p1);
	}

	double mm[6];
	data->GradientTransform(mm, false);
	p = transform_point(mm, p);

	if (trans) p = transform_point(data->m(),p);
	return p;
}

//! Scan for point.
/*! Returns:
 * <pre>
 *   -8=no data
 *   -7 data but not on it
 *   -6 on data but not on a point
 *   -5 on controller for data->hint_a
 *   -4 on r1
 *   -3 on r2
 *   -2 on p1
 *   -1 on p2
 *   >=0 color spot index.
 *  </pre>
 */
int GradientInterface::scan(int x,int y)
{
	// picks closest within a distance
	if (!data) return GP_Nodata;

	flatpoint p,pp;
	p = screentoreal(x,y); //<-remember this is not including data's transform
	double d = 5*ScreenLine()/Getmag(), //d eventually is (5 screen pixels in gradient space)^2
		   dd = 0;
	//DBG cout <<" gd scan x,y: "<<x<<','<<y<<"  d="<<d<<"(x,y)="<<p.x<<','<<p.y;
	double threshhold = d;
	d *= d;
	int closest = GP_OutsideData;

	p = data->transformPointInverse(p); //now p in object space

	// check this order: color spots, p2,p1,r2,r1
	int c;
	for (c = data->strip->colors.n-1; c>=0; c--) { //scan for spots
		pp = getpoint(c,0);
		dd = (pp.x - p.x) * (pp.x - p.x) + (pp.y - p.y) * (pp.y - p.y);
		if (dd < d) {
			d = dd;
			closest = c;
		}
	}
	if (closest != GP_OutsideData) return closest;

	 //check for click along edges
	if (data->IsRadial()) {
		//cerr << "----gradient p1: "<<data->P1()<<",  p2: "<<data->P2()<<"  
		dd = norm(p - data->P1());
		if (fabs(dd-fabs(data->R1())) < threshhold) return GP_r1;

		dd = norm(p - data->P2());
		if (fabs(dd - fabs(data->R2())) < threshhold) return GP_r2;

	} else { // linear gradient
		if ((style & EditStrip) == 0) {
			flatpoint ul,ur,ll,lr;
			double mm[6];
			data->GradientTransform(mm, false);
			double sd = (data->P2() - data->P1()).norm();
			ul = flatpoint(transform_point(mm, flatpoint(0,  data->R1())));
			ur = flatpoint(transform_point(mm, flatpoint(sd, data->R1())));
			ll = flatpoint(transform_point(mm, flatpoint(0, -data->R2())));
			lr = flatpoint(transform_point(mm, flatpoint(sd,-data->R2())));

			if (distance(p,ul,ur) < threshhold) return GP_r1;
			if (distance(p,ll,lr) < threshhold) return GP_r2;
			if (distance(p,ul,ll) < threshhold) {
				if (distance(p, data->P1()) < threshhold) return GP_p1;
				return GP_p1_bar;
			}
			if (distance(p,ur,lr) < threshhold) {
				if (distance(p, data->P2()) < threshhold) return GP_p2;
				return GP_p2_bar;
			}
		}
	}


	// if not a color spot point, p, or p+v, then...
	if (closest == GP_OutsideData) { // check if in the color somewhere
		c = data->pointin(data->transformPoint(p));
		if (!c) closest = GP_OutsideData;
		else closest = GP_DataNoPoint;
	}

	if (data->IsRadial()) {
		//check for radial p1 and p2 after everything else
		dd = norm(p - data->P1());
		if (dd < threshhold) return GP_p1;

		dd = norm(p - data->P2());
		if (dd < threshhold) return GP_p2;
	}

	DBG cerr <<" found:"<<closest<<endl;
	return closest;
}

//! Select a color spot (not p (c==-1) or p+v (c==-2)).
/*! Returns 0 on success.
 *
 * This only allows selecting the color spots. Perhaps in future allow select -1 or -2?
 *
 * \todo could have state passed in...
 */
int GradientInterface::SelectPoint(int c)
{
	if (!data) return 0;
	if (c<0 || c>data->strip->colors.n) return 1;
	curpoints.pushnodup(c); // returns 1 if pushed, 0 if already there
	curpoint=c;
	sendcolor(&data->strip->colors.e[c]->color->screen);
	needtodraw|=2;
	return 0;
}

//! Send a "make curcolor" event to the viewport.
/*! \todo bit of a hack here.
*/
int GradientInterface::sendcolor(ScreenColor *col)
{
	if (!col || !curwindow || !curwindow->win_parent) return 0;

	SimpleColorEventData *e=new SimpleColorEventData( 65535, col->red, col->green, col->blue, col->alpha, 0);
	//app->SendMessage(e, curwindow->object_id, "make curcolor", object_id);
	app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);

	return 1;
}

Laxkit::MenuInfo *GradientInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!data) return menu;

    if (!menu) menu = new MenuInfo();

	menu->AddToggleItem(_("Fill parent"), GRAD_Toggle_Fill_Parent, 0, data->fill_parent);
	menu->AddSep(_("Spread"));
	menu->AddToggleItem(_("None"),    GRAD_Spread_None,    0, data->spread_method == LAXSPREAD_None);
	menu->AddToggleItem(_("Repeat"),  GRAD_Spread_Repeat,  0, data->spread_method == LAXSPREAD_Repeat);
	menu->AddToggleItem(_("Reflect"), GRAD_Spread_Reflect, 0, data->spread_method == LAXSPREAD_Reflect);
	menu->AddToggleItem(_("Pad"),     GRAD_Spread_Pad,     0, data->spread_method == LAXSPREAD_Pad);
	menu->AddSep();
	menu->AddToggleItem(_("Linear"), GRAD_MakeLinear, 0, data->IsLinear());
	menu->AddToggleItem(_("Radial"), GRAD_MakeRadial, 0, data->IsRadial());

	return menu;
}

int GradientInterface::Event(const Laxkit::EventData *ev, const char *mes)
{
	if (!strcmp(mes,"newcolor")) {
		if (!data || !curpoints.n) return 0;

		const SimpleColorEventData *ce = dynamic_cast<const SimpleColorEventData *>(ev);
		if (!ce) return 0;
		if (ce->colorsystem != LAX_COLOR_RGB) {
			PostMessage(_("Color has to be rgb currently."));
			return 0;
		}

		double mx = ce->max;
		double cc[5];
		for (int c=0; c<5; c++) cc[c]=ce->channels[c]/mx;

		for (int c=0; c<curpoints.n; c++) {
			data->strip->SetColor(curpoints.e[c], cc[0],cc[1],cc[2],cc[3]);
		}
		data->touchContents();
		needtodraw=1;
		curwindow->Needtodraw(1);
		return 0;

	} else if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(ev);
        int i =s->info2; //id of menu item

        if ( i == GRAD_Spread_None
		  || i == GRAD_Spread_Repeat
		  || i == GRAD_Spread_Reflect
		  || i == GRAD_Spread_Pad
		  || i == GRAD_Spread_Next
		  || i == GRAD_Spread_Prev
		  || i == GRAD_Toggle_Fill_Parent
		  || i == GRAD_MakeLinear
		  || i == GRAD_MakeRadial
		  )
		{
			PerformAction(i);
		}
		return 0;
	}

	return anInterface::Event(ev,mes);
}

#define DRAG_NORMAL      1
#define DRAG_NEW         2
#define DRAG_FROM_INSIDE 3

flatpoint GradientInterface::screentoreal(int x,int y)
{
	if (style & RealSpaceMouse) return dp->screentoreal(x,y);
	return anInterface::screentoreal(x,y);
}

//! Creates new data if not click down on one...
/*! Shift-click makes a new color spot.
 *
 * \todo *** shift-click near an already selected but perhaps obscured point should not select
 * the point that is obscuring it instead
 */
int GradientInterface::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	DBG cerr << "  in grad lbd..  count=" <<count<<endl;
	if (buttondown.any()) return 1;
	mx = x;
	my = y;
	draggingmode = DRAG_NORMAL;
	buttondown.down(d->id,LEFTBUTTON);

	 // straight click
	int c = scan(x,y);
	DBG cerr <<"******************* gradient scan:"<<c<<endl;
	if (c >= GP_MinMoveable) { // scan found a moveable point...
		curpoint = c;
		if (curpoint > 0 && curpoint < data->strip->colors.n-1) draggingmode = DRAG_FROM_INSIDE;
		if (curpoint < 0) { // is a non-color control point
			curpoints.flush();
		} else {
			 // click down on a color spot.
			if (count == 2) {
				buttondown.up(d->id,LEFTBUTTON);
				anXWindow *w = new ColorSliders(NULL,"New Color","New Color",ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS,
				 0,0,200,400,0,
				NULL,object_id,"newcolor",
				LAX_COLOR_RGB, 1./255,
				data->strip->colors.e[curpoints[0]]->color->screen.Red(),
				data->strip->colors.e[curpoints[0]]->color->screen.Green(),
				data->strip->colors.e[curpoints[0]]->color->screen.Blue(),
				data->strip->colors.e[curpoints[0]]->color->screen.Alpha(),
				0,
				//cc[0],cc[1],cc[2],cc[3],cc[4],
				x,y);

				if (w) app->rundialog(w);
				return 0;
			}

			if ((state&LAX_STATE_MASK)==0) {
				 // plain click always deselect else and selects the
				 // one clicked on
				curpoints.flush();
				curpoints.push(c);
				sendcolor(&data->strip->colors.e[c]->color->screen);
				needtodraw|=2;
				return 0;
			} else {
				if (curpoints.pushnodup(curpoint)) {
					 // curpoint is not already in curpoints,
					sendcolor(&data->strip->colors.e[curpoint]->color->screen);
				} else {
					 // curpoint was already selected
					 // if control, then remove the point
					if ((state&LAX_STATE_MASK)==ControlMask) {
						for (c=0; c<curpoints.n; c++) if (curpoints.e[c]==curpoint) {
							curpoints.pop(c);
							break;
						}
						if (c<curpoints.n) {
							if (curpoints.n) curpoint=curpoints.e[0];
							else curpoint=GP_Nothing;
							needtodraw|=2;
						}
					}
				}
			}
		}
		needtodraw|=2;
		return 0;

	} else if (c == GP_DataNoPoint && count != 2) {
		 // point not found but on data, if plain click flush points, return..
		leftp = transform_point_inverse(data->m(),screentoreal(x,y));
		if ((state&LAX_STATE_MASK)==0) {
			curpoints.flush();
			curpoint = GP_DataNoPoint;
			DBG cerr <<"  Gradient leftp: "<<leftp.x<<','<<leftp.y<<endl;
			needtodraw |= 2;
			return 0;
		}
	}
	DBG cerr <<"  no gradient pointfound  "<<endl;

	 // make new color point if shift-click not on an existing point
	if (data && ((state&LAX_STATE_MASK)==ShiftMask || count==2)) {
		//DBG cerr <<"gradient double click..."<<c<<"  curpoint: "<<curpoint<<endl;
		if (curpoint >= GP_MinMoveable) {
			return 0;
		}

		//add new point
		flatpoint p = screentoreal(x,y);
		curpoints.flush();

		double t,clen,cstart;
		cstart = data->strip->colors.e[0]->t;
		clen = data->strip->colors.e[data->NumColors()-1]->t - cstart;
		flatpoint p1,v;

		p1 = getpoint(0,1);
		v = getpoint(data->strip->colors.n-1,1)-p1;

		t = cstart + clen*((p-p1)*v)/(v*v);
		curpoint = data->AddColor(t,NULL);
		curpoints.push(curpoint);
		sendcolor(&data->strip->colors.e[curpoint]->color->screen);

		needtodraw=1;
		return 0;
	}

	 //! Get rid of old data if not clicking in it.
	if (data && c == GP_OutsideData) {
		deletedata();
	}

	 // search for another viewport object to transfer control to
	if (viewport) {
		ObjectContext *oc=NULL;
		GradientData *obj=NULL;
		int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
		if (c>0) obj=dynamic_cast<GradientData *>(oc->obj);
		if (obj) {
			if (data) deletedata();
			data=obj;
			data->inc_count();
			if (goc) delete goc;
			goc=oc->duplicate();
			if (viewport) viewport->ChangeObject(oc,0,true);
			needtodraw=1;
			return 0;
		}
		if (!primary && c==-1 && viewport->ChangeObject(oc,1,true)) {
			buttondown.up(d->id,LEFTBUTTON);
			deletedata();
			return 0;
		}
	}

	 // make new one
	newData(x,y);
	curpoint = GP_p2;
	draggingmode = DRAG_NEW;
	DBG cerr <<"new gradient:"<<endl;
	DBG data->dump_out(stderr,2,0,NULL);

	needtodraw=1;
	return 0;
	DBG cerr <<"..gradlbd done   ";
}

//! Create new data for screen point x,y, with a count of 1 plus viewport counts via viewport->NewData(ndata).
void GradientInterface::newData(double x,double y)
{
	deletedata();

	 //create with count 1
	GradientData *ndata=NULL;
	ndata = dynamic_cast<GradientData *>(somedatafactory()->NewObject(LAX_GRADIENTDATA));
	if (ndata) {
		ndata->Set(screentoreal(x,y),screentoreal(x,y),
						creater1,creater2,&col1,&col2,creationstyle);
	}
	if (!ndata) ndata=new GradientData(screentoreal(x,y),screentoreal(x,y),
						creater1,creater2,&col1,&col2,creationstyle);
	ndata->FindBBox();

	ObjectContext *oc=NULL;
	if (viewport) viewport->NewData(ndata,&oc);
	data = ndata;
	if (goc) delete goc;
	goc = oc ? oc->duplicate() : NULL;
}

//! If data, then call viewport->ObjectMoved(data).
int GradientInterface::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	draggingmode = DRAG_NORMAL;
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	if (curpoint<0) curpoint = GP_Nothing;
	if (data && viewport) viewport->ObjectMoved(goc,1);
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}


int GradientInterface::MouseMove(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	DBG cerr <<"--------------gradient point scan:"<<scan(x,y)<<endl;
	if (!data) { return 1;}

	// DBG screendbg = flatpoint(x,y);
	// DBG hoverdbg = screentoreal(x,y);

	if (!buttondown.isdown(mouse->id,LEFTBUTTON)) {
		int c = scan(x,y);
		if (c != curpoint) {
			curpoint = c;
			if (curpoint < 0) PostMessage(PointName(curpoint));
			else PostMessage2("Color point %d", curpoint);
			needtodraw = 1;
		}
		//data->dump_out(stdout, 2, 0, NULL);
		return 1;
	}

	// DBG cerr << "gradient move point: "<<PointName(curpoint)<<endl;

	flatpoint d = screentoreal(x,y) - screentoreal(mx,my);
	if (curpoint < GP_MinMoveable && curpoint != GP_a && curpoints.n == 0) {
		if (state&ControlMask && state&ShiftMask) { // +^ rotate
			 // rotate around p of gradient based on x movement
			double a;
			a = (x-mx)/180.0*M_PI;
			flatpoint p = transform_point(data->m(),leftp);
			data->xaxis(rotate(data->xaxis(),a,0));
			data->yaxis(rotate(data->yaxis(),a,0));
			data->origin(data->origin()+p-transform_point(data->m(),leftp));

		} else if (state&ControlMask) { // ^ scale w
			double dd = double(x-mx);
			dd = 1+.02*dd;
			if (dd<0.1) dd=0.1;
			flatpoint p = transform_point(data->m(),leftp);
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
	if (movepoint==GP_r1 && data->IsRadial() && (state&LAX_STATE_MASK)==ShiftMask)
		movepoint = GP_p1;

	else if (movepoint == GP_r2 && data->IsRadial() && (state&LAX_STATE_MASK)==ShiftMask)
		movepoint = GP_p2;

	else if (movepoint == 0 && curpoints.n == 1
			  && (state&LAX_STATE_MASK) != ShiftMask
			  && draggingmode != DRAG_FROM_INSIDE) { movepoint = GP_p1; state ^= ShiftMask; }

	else if (movepoint == data->NumColors()-1 && curpoints.n == 1
			  && (state&LAX_STATE_MASK) != ShiftMask
			  && draggingmode != DRAG_FROM_INSIDE)
		{ movepoint = GP_p2; state ^= ShiftMask; }

	 //second round remapping
	if (movepoint == GP_p1 && data->IsRadial() && (state&LAX_STATE_MASK)==ControlMask) movepoint = GP_r1;
	else if (movepoint == GP_p2 && data->IsRadial() && (state&LAX_STATE_MASK)==ControlMask) movepoint = GP_r2;


	double m[6];
	transform_invert(m,data->m());
	d = transform_vector(m,d);
	flatpoint od = d; //change in pos in object space
	double mm[6];
	data->GradientTransform(mm, true);
	d = transform_vector(mm, d); //change in mouse pos in gradient space
	flatpoint pp1 = transform_point(mm, data->P1());
	flatpoint pp2 = transform_point(mm, data->P2());
	//flatpoint v = data->P2() - data->P1();

	DBG cerr <<" ------------ pp1: "<<pp1.x<<","<<pp1.y<<"  pp2: "<<pp2.x<<','<<pp2.y<<endl;

	if (movepoint == GP_a) {
		DBG cerr <<"--- move grad point a"<<endl;
		if (data->IsRadial()) data->hint_a += d.x/180*M_PI;
		else data->hint_a += d.y;

	} else if (movepoint == GP_r1) {
		if (data->IsRadial()) {
			flatpoint newp = data->transformPointInverse(screentoreal(x,y));
			flatpoint oldp = data->transformPointInverse(screentoreal(mx,my));
			double dr = (newp - data->P1()).norm() - (oldp - data->P1()).norm();
			data->R1(data->R1() + dr);
			if (  fabs((pp2.x + fabs(data->R2())) - (pp1.x + fabs(data->R1())))
				< fabs((pp2.x - fabs(data->R2())) - (pp1.x - fabs(data->R1())))) {
				data->hint_a = M_PI;
			} else data->hint_a = 0;

		} else {
			data->R1(data->R1() + d.y);
			if ((state & LAX_STATE_MASK) == (ControlMask & ShiftMask)) data->R2(data->R1());
			else if ((state & LAX_STATE_MASK) != ShiftMask) data->R2(data->R2() + d.y);
		}
		data->touchContents();
		needtodraw=1;

	} else if (movepoint==GP_r2) {
		if (data->IsRadial()) {
			flatpoint newp = data->transformPointInverse(screentoreal(x,y));
			flatpoint oldp = data->transformPointInverse(screentoreal(mx,my));
			double dr = (newp - data->P2()).norm() - (oldp - data->P2()).norm();
			data->R2(data->R2() + dr);
			if ( fabs((pp2.x + fabs(data->R2())) - (pp1.x + fabs(data->R1())))
				<fabs((pp2.x - fabs(data->R2())) - (pp1.x - fabs(data->R1())))) {
				data->hint_a=M_PI;
			} else data->hint_a=0;
		} else {
			data->R2(data->R2() -d.y);
			if ((state & LAX_STATE_MASK) == (ControlMask & ShiftMask)) data->R1(data->R2());
			else if ((state & LAX_STATE_MASK) != ShiftMask) data->R1(data->R1() - d.y);
		}
		data->touchContents();
		needtodraw=1;

	} else if (movepoint==GP_p2_bar || movepoint==GP_p1_bar) {
		//this only happens with linear
		flatpoint v = (data->P2() - data->P1());
		v.normalize();

		if (movepoint==GP_p1_bar) {
			data->P1(data->P1() + (v*od) * v);
		} else {
			data->P2(data->P2() + (v*od) * v);
		}
		needtodraw = 1;

	} else if (movepoint==GP_p2 || movepoint==GP_p1) {
		if (movepoint == GP_p1) data->P1(data->P1() + od);
		else data->P2(data->P2() + od);

		data->touchContents();
		if (data->IsRadial()) {
			if ( fabs((pp2.x + fabs(data->R2())) - (pp1.x + fabs(data->R1())))
				<fabs((pp2.x - fabs(data->R2())) - (pp1.x - fabs(data->R1())))) {
				data->hint_a = M_PI;
			} else data->hint_a = 0;
		}
		needtodraw=1;

	} else {
		 // move curpoints
		int cp;
		double plen,clen,cstart;
		flatpoint pc1 = transform_point(mm, getpoint(0,0));
		flatpoint pc2 = transform_point(mm, getpoint(data->NumColors()-1,0));
		plen = (pc1 - pc2).norm();
		double dx = distparallel(d, pc2-pc1);
		cstart = data->strip->colors.e[0]->t;
		clen = data->strip->colors.e[data->NumColors()-1]->t - cstart;

		if (plen && curpoints.n) {
			for (int c=0; c<curpoints.n; c++) {
				//cout <<"move point "<<curpoints.e[c]<<"  by d.x="<<d.x<<"  d.x/clen="<<d.x/clen<<endl;
				cp = curpoints.e[c];

				DBG if (cp==0) {
				DBG 	cerr <<"*** mv grad point 0"<<endl;
				DBG } else if (cp==data->NumColors()-1) {
				DBG 	cerr <<"*** mv grad point n-1"<<endl;
				DBG }

				 //d is in gradient space, but t shifts in t space
				bool clamp = data->IsRadial();
				curpoints.e[c] = data->ShiftPoint(curpoints.e[c], dx/plen*clen, clamp);
				DBG cerr <<"curpoint["<<c<<"] now is: "<<curpoints.e[c]<<endl;

				 // the shifting reorders the spots, and messes up curpoints so this corrects that
				 //****FIX! though if the shift skips more than one it doesn't!
				if (cp != curpoints.e[c]) {
					if (cp == curpoint) curpoint = curpoints.e[c];
					for (int c2=0; c2<curpoints.n; c2++) {
						if (c2==c) continue;
						if (cp < curpoints.e[c]) {//remember cp is the old value of the shifted point
							if (curpoints.e[c2] > cp && curpoints.e[c2] <= curpoints.e[c])
								curpoints.e[c2]--;
						} else {
							if (curpoints.e[c2] >= curpoints.e[c] && curpoints.e[c2] < cp)
								curpoints.e[c2]++;
						}
					}
				}
			}

			 // check if points were shifted past previous p1 and p2 points
			 // and change p1 and/or p2 if so
			flatpoint v = data->P2() - data->P1();
			v.normalize();
			double nclen = data->strip->colors.e[data->strip->colors.n-1]->t - data->strip->colors.e[0]->t;
			if (data->strip->colors.e[0]->t != cstart || nclen != clen) {
				double m = plen/clen;
				data->P1(data->P1() + (data->strip->colors.e[0]->t - cstart)*m * v);
				data->P2(data->P2() + (data->strip->colors.e[0]->t + nclen - (cstart+clen))*m * v);
			}
		}
	}

	data->FindBBox();
	mx = x; my = y;
	needtodraw |= 2;
	return 0;
}

Laxkit::ShortcutHandler *GradientInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager = GetDefaultShortcutManager();
	sc = manager->NewHandler(whattype());
	if (sc) return sc;

	sc = new ShortcutHandler(whattype());

	sc->Add(GRAD_SelectLeft,  LAX_Left,0,0,  "SelectNext",  _("Select next"), nullptr, 0);
	sc->Add(GRAD_SelectRight, LAX_Right,0,0, "SelectPrev",  _("Select previous"), nullptr, 0);
	sc->Add(GRAD_Decorations, 'd',0,0,       "Decorations", _("Toggle decorations"), nullptr, 0);
	sc->Add(GRAD_Flip,        'f',0,0,       "FlipColors",  _("Flip order of colors"), nullptr, 0);
	sc->Add(GRAD_MakeLinear,  'l',0,0,       "Linear",      _("Convert to linear gradient"), nullptr, 0);
	sc->Add(GRAD_MakeRadial,  'r',0,0,       "Radial",      _("Convert to Radial gradient"), nullptr, 0);
	sc->Add(GRAD_Select,      'a',0,0,       "Select",      _("Select all, or deselect"), nullptr, 0);
	sc->Add(GRAD_Delete,      LAX_Del,0,0,   "Delete",      _("Delete"), nullptr, 0);
	sc->AddShortcut(LAX_Bksp,0,0, GRAD_Delete);

	sc->Add(GRAD_Toggle_Fill_Parent, 'p',0,0,        "ToggleFillParent", _("Toggle fill parent"), nullptr, 0);
	sc->Add(GRAD_Spread_Next,        's',0,0,        "SpreadNext"      , _("Set next color spread type"), nullptr, 0);
	sc->Add(GRAD_Spread_Prev,        's',ShiftMask,0,"SpreadPrev"      , _("Set previous color spread type"), nullptr, 0);

	sc->AddAction(GRAD_Spread_None,        "SpreadNone"      , _("No color spread"), nullptr, 0, 0);
	sc->AddAction(GRAD_Spread_Repeat,      "SpreadRepeat"    , _("Repeat colors"),   nullptr, 0, 0);
	sc->AddAction(GRAD_Spread_Reflect,     "SpreadReflect"   , _("Reflect colors"),  nullptr, 0, 0);
	sc->AddAction(GRAD_Spread_Pad,         "SpreadPad"       , _("Extend colors"),   nullptr, 0, 0);


	manager->AddArea(whattype(),sc);
	return sc;
}

int GradientInterface::PerformAction(int action)
{
	if (action==GRAD_SelectLeft) {
		if (!data) return 0;
		curpoints.flush();
		curpoint--;
		if (curpoint<GP_MinMoveable) curpoint = data->NumColors()-1;
		if (curpoint>=0) curpoints.push(curpoint);
		DBG cerr <<"gradient curpoint="<<curpoint<<endl;
		needtodraw=1;
		return 0;

	} else if (action==GRAD_SelectRight) {
		if (!data) return 0;
		curpoints.flush();
		curpoint++;
		if (curpoint>=data->NumColors()) curpoint=GP_MinMoveable;
		if (curpoint>=0) curpoints.push(curpoint);
		DBG cerr <<"gradient curpoint="<<curpoint<<endl;
		needtodraw=1;
		return 0;

	} else if (action==GRAD_Decorations) {
		if (--showdecs<0) showdecs = ShowControls | ShowColors;
		switch (showdecs) {
			case 0: PostMessage(_("Don't show object on top, no decorations")); break;
			case ShowControls: PostMessage(_("Don't show object on top, but show decorations")); break;
			case ShowColors: PostMessage(_("Show object on top without decorations")); break;
			case ShowControls | ShowColors: PostMessage(_("Show object on top with decorations")); break;
		}
		needtodraw=1;
		return 0;

	} else if (action==GRAD_Flip) {
		if (data) { data->FlipColors(); needtodraw=1; }
		return 0;

	} else if (action==GRAD_MakeLinear) {
		if (!data || data->IsLinear()) { creationstyle = GradientData::GRADIENT_LINEAR; return 0; }
		data->SetLinear();
		data->FindBBox();
		data->hint_a=0;
		needtodraw=1;
		data->touchContents();
		return 0;

	} else if (action==GRAD_MakeRadial) {
		if ((style & EditStrip) == 0) {
			if (!data || data->IsRadial()) { creationstyle = GradientData::GRADIENT_RADIAL; return 0; }
			data->SetRadial();
			data->FindBBox();
			data->touchContents();
			needtodraw=1;
		}
		return 0;

	} else if (action==GRAD_Select) {
		if (curpoints.n || curpoint>=GP_MinMoveable) {
			curpoints.flush();
			curpoint = GP_Nothing;
			needtodraw =1 ;

		} else if (!curpoints.n && data) {
			for (int c=0; c<data->strip->colors.n; c++) curpoints.push(c);
			curpoint = 0;
			needtodraw|=2;
		}
		return 0;

	} else if (action==GRAD_Delete) {
		if (!data || !data->strip || (curpoints.n==0 && curpoint<GP_MinMoveable)) return 0;

		int c;
		if (curpoint==GP_r1) data->R1(0);
		else if (curpoint==GP_r2) data->R2(0);
		else if (curpoints.n) while (curpoints.n) {
			c = curpoints.pop();
			DBG cerr <<"--- deleting gradient spot "<<c<<endl;

			if (data->strip->colors.n>2) data->strip->colors.pop(c);
			if (c==0) {
				 // move p1 to new first color spot
				flatpoint p = getpoint(0,0);
				data->P1(p);
				continue;
			}
			if (c >= data->strip->colors.n) {
				 // move p2 to new last color spot
				flatpoint p = getpoint(data->strip->colors.n-1,0);
				data->P2(p);
				continue;
			}
		}
		curpoint = GP_Nothing;
		data->FindBBox();
		data->touchContents();
		needtodraw=1;
		return 0;

	} else if (action == GRAD_Spread_Next) {
		if (!data) return 0;
		int i = data->spread_method + 1;
		if (i >= LAXSPREAD_MAX) i = LAXSPREAD_None;
		data->spread_method = i;
		needtodraw=1;
		return 0;

	} else if (action == GRAD_Spread_Prev) {
		if (!data) return 0;
		int i = data->spread_method - 1;
		if (i < LAXSPREAD_None) i = LAXSPREAD_MAX-1;
		data->spread_method = i;
		needtodraw=1;
		return 0;

	} else if (action == GRAD_Spread_None) {
		if (!data) return 0;
		data->spread_method = LAXSPREAD_None;
		needtodraw=1;
		return 0;

	} else if (action == GRAD_Spread_Repeat) {
		if (!data) return 0;
		data->spread_method = LAXSPREAD_Repeat;
		needtodraw=1;
		return 0;

	} else if (action == GRAD_Spread_Reflect) {
		if (!data) return 0;
		data->spread_method = LAXSPREAD_Reflect;
		needtodraw=1;
		return 0;

	} else if (action == GRAD_Spread_Pad) {
		if (!data) return 0;
		data->spread_method = LAXSPREAD_Pad;
		needtodraw=1;
		return 0;

	} else if (action == GRAD_Toggle_Fill_Parent) {
		if (!data) return 0;
		data->fill_parent = !data->fill_parent;
		needtodraw=1;
		return 0;

	}

	return 1;
}

int GradientInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	if (ch==LAX_Esc && curpoints.n) return PerformAction(GRAD_Select); //deselect selected points

	DBG if (ch=='p')  { //list points, debug only
	DBG 	if (!data) return 0;
	DBG 	flatpoint p;
	DBG 	cerr <<"GradientData"<<data->object_id<<endl;
	DBG 	data->FindBBox();
	DBG 	data->dump_out(stderr,2,0,NULL);
	DBG 	cerr <<"  minx:"<<data->minx<<endl;
	DBG 	cerr <<"  maxx:"<<data->maxx<<endl;
	DBG 	cerr <<"  miny:"<<data->miny<<endl;
	DBG 	cerr <<"  maxy:"<<data->maxy<<endl;
	DBG 	cerr <<"Grad points, curpoint="<<curpoint<<endl;
	DBG 	for (int c=GP_MinMoveable; c<data->NumColors(); c++) {
	DBG 		flatpoint p = getpoint(c, 0);
	DBG 		cerr << "  point "<<PointName(c)<<": "<<p.x<<", "<<p.y<<endl;
	DBG 	}
	DBG }

	return 1;
}


} // namespace LaxInterfaces

