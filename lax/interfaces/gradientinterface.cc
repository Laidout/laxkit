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
#include <lax/interfaces/gradientinterface.h>

#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/lists.cc>

using namespace Laxkit;
using namespace LaxFiles;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//#define GRADIENT_RADIAL (1<<0)

//------------------------------ GradientDataSpot ----------------------------------

/*! \class GradientDataSpot
 * \ingroup interfaces
 * \brief GradientData keeps a stack of these.
 *
 * Keeps position t, and color. The color components are in range [0,0xffff].
 */


GradientDataSpot::GradientDataSpot(double tt,ScreenColor *col)
{
	t=tt;
	color=*col;
}

GradientDataSpot::GradientDataSpot(double tt,int rr,int gg,int bb,int aa)
{
	t=tt;
	color.red  =(unsigned short)rr;
	color.green=(unsigned short)gg;
	color.blue =(unsigned short)bb;
	color.alpha=(unsigned short)aa; 
}
	
//! Dump in an attribute, then call dump_in_atts(thatatt,0,context).
/*! If Att!=NULL, then return the attribute used to read in the stuff.
 * This allows
 * holding classes to have extra attributes within the spot field to
 * exist and not be discarded.
 *
 * \todo *** allow import of Gimp, Inkscape/svg, scribus gradients
 */
void GradientDataSpot::dump_in(FILE *f,int indent,LaxFiles::DumpContext *context, Attribute **Att)
{
	Attribute *att=new Attribute;
	att->dump_in(f,indent);
	dump_in_atts(att,0,context);
	if (Att) *Att=att;
	else delete att;
}

//! Fill the t, red, green, blue, alpha, based on the corresponding attributes.
void GradientDataSpot::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	int c,c2=0;
	char *value,*name;
	for (c=0; c<att->attributes.n; c++) { 
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"rgba")) {
			int i[4];
			c2=IntListAttribute(value,i,4);
			DBG if (c2!=4) cerr <<"---gradient spot not right number of color components!!"<<endl;
			for (int c3=0; c3<c2; c3++) {
				if (c3==0) color.red=i[c3];
				else if (c3==1) color.green=i[c3];
				else if (c3==2) color.blue=i[c3];
				else if (c3==3) color.alpha=i[c3];
			}
		} else if (!strcmp(name,"t")) {
			DoubleAttribute(value,&t);
		}
	}
	DBG cerr <<"spot out:"<<endl;
	DBG dump_out(stderr,2,0,NULL);
}

/*! Outputs something like:\n
 * <pre>
 *  t .754
 *  rgba 100 255 34 65535
 * </pre>
 *
 * If what==-1, then dump out a psuedocode mockup of what gets dumped.
 * Otherwise dumps out in indented data format described above.
 *
 * \todo could have rgba vs. rgba16
 */
void GradientDataSpot::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%st 1     #the spot on the x axis to place the color, customarily the spots will\n",spc);
		fprintf(f,"%s        #cover the whole range [0..1] but that is not mandatory\n",spc);
		fprintf(f,"%srgba 0 255 65535 65535  #the red, green, blue, and alpha components, values from 0 to 65535\n",spc);
		return;
	}
	fprintf(f,"%st %.10g\n",spc,t);
	fprintf(f,"%srgba %u %u %u %u\n",spc,color.red,color.green,color.blue,color.alpha);
}

//------------------------------ GradientData ----------------------------------

/*! \class GradientData
 * \ingroup interfaces
 * \brief Handled by GradientInterface
 *
 * Can be a linear or a radial gradient. It is not advisable to move around the color
 * spots manually. Call ShiftPoint(). This is to ensure that the colors.e[] array
 * is ordered from lowest t value to highest. GradientInterfaces places the color
 * spots along the xaxis.
 *
 * For GRADIENT_RADIAL, the center of the start circle is at (p1,0) with radius r1
 * The center of the ending
 * circle is at (p2,0) with radius r2. The spots' t values are distributed as appropriate
 * along the circles.
 * 
 * For GRADIENT_LINEAR, the spots are distributed according to the color[]->t values as
 * mapped to the segment (p1,0) to (p2,0). 
 * The color extends in the y direction from r1 down to -r2. Derived classes should 
 * remember that r1 and r2 can be negative.
 *
 * \todo *** move the change between radial and linear to here from GradientInterface?
 *   or separate linear from radial.. editing radial is really obnoxious right now..
 * \todo *** perhaps have the colors array be potentially read only (or make new on change),
 *   and be able to be stored elsewhere, thus be able to read stand alone gradient files....
 * \todo *** care must be taken that final t != initial t (esp is ShiftPoint())!!
 * \todo it is assumed here that p1<p2, maybe should make that explicit somehow?
 */
/*! \fn GradientData::~GradientData()
 * \brief Empty virtual destructor.
 */
/*! \var int GradientData::a
 * \brief A displacement to place the color spot line when drawing on screen.
 *
 * This is automatically determined by GradientInterface.
 */

//! Default constructor, makes a filled circle radius 10.
GradientData::GradientData()
{
	p1=p2=r1=0;
	p2=0;
	r2=10;
	style=0; 
	usepreview=1;
} 

//! Create new basic gradient pp1 to pp2. Sets col1 at 0 and col2 at 1
/*! This just passes everything to Set().
 */
GradientData::GradientData(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2,unsigned int stle)
{
	Set(pp1,pp2,rr1,rr2,col1,col2,stle);
	usepreview=1;
}

SomeData *GradientData::duplicate(SomeData *dup)
{
	GradientData *g=dynamic_cast<GradientData*>(dup);
	if (!g && !dup) return NULL; //was not GradientData!

	char set=1;
	if (!dup) {
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_GRADIENTDATA));
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			//set=0;
			g=dynamic_cast<GradientData*>(dup);
		}
	} 
	if (!g) {
		g=new GradientData();
		dup=g;
	}
	if (set) {
		g->style=style;
		g->p1=p1;
		g->p2=p2;
		g->r1=r1;
		g->r2=r2;
		g->a=a;

		for (int c=0; c<colors.n; c++) {
			g->colors.push(new GradientDataSpot(colors.e[c]->t,&colors.e[c]->color));
		}
	}

	 //somedata elements:
	dup->bboxstyle=bboxstyle;
	dup->m(m());
	return dup;
}

//! Set so gradient is pp1 to pp2. Sets col1 at 0 and col2 at 1.
void GradientData::Set(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2,unsigned int stle)
{
	if (norm(pp2-pp1)<1e-5) {
		xaxis(flatpoint(1,0));
		yaxis(flatpoint(0,1));
		p1=0;
		p2=0;

	} else {
		xaxis(pp2-pp1);
		yaxis(transpose(pp2-pp1));
		p1=0;
		p2=1;
	}
	origin(pp1);

	r1=rr1;
	r2=rr2;
	a=0;

	style=stle; 
	colors.flush();
	if (col1) colors.push(new GradientDataSpot(0,col1));	
	if (col2) colors.push(new GradientDataSpot(1,col2)); 

	touchContents();

	DBG cerr <<"new GradientData:Set"<<endl;
	DBG dump_out(stderr,2,0,NULL);
}

//! Return if pp transformed to data coords is within the bounds.
/*! unimplemented: in=1 | on=2 | out=0 
 */
int GradientData::pointin(flatpoint pp,int pin)
{ 
	double x,y,mm[6];
	transform_invert(mm,m());
	pp=transform_point(mm,pp);
	x=pp.x;
	y=pp.y;
		
	if (style&GRADIENT_RADIAL) {
		 //note that it could just be a line though if o1!=o2, that is not dealt with here...
		if (r2==0 && r1==0) { 
			DBG cerr <<"point not in gradient "<<object_id<<endl; 
			return 0; 
		} 
		if ((x-p1)*(x-p1)+y*y < r1*r1) { 
			DBG cerr <<"point in gradient "<<object_id<<endl;
			return 1; 
		} // is in circle 1
		if ((x-p2)*(x-p2)+y*y < r2*r2) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 1;
		} // is in circle 2

		 //if (in intermediate space)...
		 //for this method, start circle cannot be totally in end circle, and vice versa.
		 //checks whether is inside the trapezoidal region connecting
		 //the starting and ending circles
		if (p1-fabs(r1)>=p2-fabs(r2) && p1+fabs(r1)<=p2+fabs(r2)) { 
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 0;
		} // circle 1 is inside circle 2
		if (p2-fabs(r2)>=p1-fabs(r1) && p2+fabs(r2)<=p1+fabs(r1)) { 
			DBG cerr <<"point not in gradient "<<object_id<<endl; 
			return 0;
		} // circle 2 is inside circle 1
		
		 
		double d=p2-p1,a2;
		double rr1=r1,
			   rr2=r2,
			   o1=p1,
			   o2=p2;
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
		a2=costheta*rr2;
		if (x<o1-a2*rr1/rr2 || x>o2-a2) { 
			DBG cerr <<"point not in gradient "<<object_id<<endl; 
			return 0; 
		}

		double b2,mm,x0;
		b2=sqrt(rr2*rr2-a2*a2);
		mm=sqrt(1/costheta/costheta-1);
		x0=a2+x-o2;
		if (y<mm*(x-x0)+b2 && y>-mm*(x-x0)-b2) {
			DBG cerr <<"point not in gradient "<<object_id<<endl;
			return 1; 
		}
		DBG cerr <<"point not in gradient "<<object_id<<endl; return 0; 
	} else {
		return x>=minx && x<=maxx && y>=miny && y<=maxy;
	}

	DBG cerr <<"point not in gradient "<<object_id<<endl; 
	
	return 0; 
}


/*! Reads in from something like:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  p1 0
 *  p2 1
 *  r1 0
 *  r2 10
 *  spot
 *    t 0
 *    rgba 255 100 50 255
 *  spot
 *    t 1
 *    rgba 0 0 0 0
 *  radial
 * </pre>
 */
void GradientData::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;
	char *name,*value,*e;
	SomeData::dump_in_atts(att,flag,context);
	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"p1")) {
			DoubleAttribute(value,&p1,&e);
		} else if (!strcmp(name,"p2")) {
			DoubleAttribute(value,&p2,&e);
		} else if (!strcmp(name,"r1")) {
			DoubleAttribute(value,&r1,&e);
		} else if (!strcmp(name,"r2")) {
			DoubleAttribute(value,&r2,&e);
		//} else if (!strcmp(name,"a")) {
		//	DoubleAttribute(value,&a,&e);
		} else if (!strcmp(name,"spot")) {
			GradientDataSpot *spot=new GradientDataSpot(0,0,0,0,0);
			spot->dump_in_atts(att->attributes.e[c],flag,context);
			colors.push(spot);
		} else if (!strcmp(name,"linear")) {
			if (BooleanAttribute(value)) style=(style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_LINEAR;
			else style=(style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_RADIAL;
		} else if (!strcmp(name,"radial")) {
			if (!BooleanAttribute(value)) style=(style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_LINEAR;
			else style=(style&~(GRADIENT_LINEAR|GRADIENT_RADIAL))|GRADIENT_RADIAL;
		}
	}
	a=0;
	touchContents();

	FindBBox();
}

/*! \ingroup interfaces
 * Dump out a GradientData. Prints matrix, p, v, and the spots.
 *
 * If what==-1, then dump out a psuedocode mockup of what gets dumped. This makes it very easy
 * for programs to keep track of their file formats, that is, when the programmers remember to
 * update this code as change happens.
 * Otherwise dumps out in indented data format as described in dump_in_atts().
 */
void GradientData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%s#Gradients lie on the x axis from p1 to p2\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #the affine transform affecting this gradient\n",spc);
		fprintf(f,"%sp1 0    #the starting x coordinate\n",spc);
		fprintf(f,"%sp2 1    #the ending x coordinate\n",spc);
		fprintf(f,"%sr1 0    #the starting radius (radial) or the +y extent (linear)\n",spc);
		fprintf(f,"%sr2 0    #the ending radius (radial) or the -y extent (linear)\n",spc);
		//fprintf(f,"%sa  0    #an offset to place the color controls of the gradient spots\n",spc);
		fprintf(f,"%sradial  #Specifies a radial gradient\n",spc);
		fprintf(f,"%slinear  #Specifies a linear gradient\n",spc);
		fprintf(f,"%sspot    #There will be at least two gradient data spots, such as this:\n",spc);
		if (colors.n) colors.e[0]->dump_out(f,indent+2,-1,NULL);
		else {
			GradientDataSpot g(0,0,30000,65535,65535);
			g.dump_out(f,indent+2,-1,NULL);
		}
		//colors.e[colors.n-1]->dump_out(f,indent+2,-1);//*** should probably check that there are always 2 and not ever 0!
		return;
	}
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,m(0),m(1),m(2),m(3),m(4),m(5));
	fprintf(f,"%sp1 %.10g\n",spc,p1);
	fprintf(f,"%sp2 %.10g\n",spc,p2);
	fprintf(f,"%sr1 %.10g\n",spc,r1);
	fprintf(f,"%sr2 %.10g\n",spc,r2);
	//fprintf(f,"%sa %.10g\n",spc,a);
	fprintf(f,"%s%s\n",spc,(style&GRADIENT_RADIAL?"radial":"linear"));
	for (int c=0; c<colors.n; c++) {
		fprintf(f,"%sspot #%d\n",spc,c);
		colors.e[c]->dump_out(f,indent+2,0,context);
	}
}

//! Find bounding box of a rectangle for linear, or the circles for radial.
void GradientData::FindBBox()
{
	if (colors.n==0) { maxx=maxy=-1; minx=miny=0; return; }

	if (style&GRADIENT_RADIAL) {
		setbounds(p1-fabs(r1),p1+fabs(r1), -fabs(r1),fabs(r1));

		addtobounds(flatpoint(p2-fabs(r2), -fabs(r2)));
		addtobounds(flatpoint(p2-fabs(r2),  fabs(r2)));
		addtobounds(flatpoint(p2+fabs(r2),  fabs(r2)));
		addtobounds(flatpoint(p2+fabs(r2), -fabs(r2)));
	} else {
		setbounds(p1, p2, r1, -r2);
		double t;
		if (maxy<miny) {
			t=maxy;
			maxy=miny;
			miny=t; 
		}
		if (maxx<minx) {
			t=maxx;
			maxx=minx;
			minx=t; 
		}
	}
	DBG cerr <<"gradient "<<object_id<<": x:"<<minx<<','<<maxx<<"  y:"<<miny<<','<<maxy<<endl;
}

/*! Return the t of colors.e[i] mapped to the range [0..1] where
 * 0 is colors.e[0] and 1 is colors.e[colors.n-1].
 */
double GradientData::GetNormalizedT(int i)
{
	if (i<0 || i>=colors.n) return 0;
	return (colors.e[i]->t - colors.e[0]->t)/(colors.e[colors.n-1]->t - colors.e[0]->t);
}

//! Move the color which to a new t position.
/*! Note this is the GradientData independent t value.
 */
int GradientData::ShiftPoint(int which,double dt)
{
	if (which<0 || which>=colors.n) return which;
	colors.e[which]->t+=dt;
	GradientDataSpot *tmp=colors.e[which];
	while (which>0 && tmp->t<colors.e[which-1]->t) {
		colors.e[which]=colors.e[which-1];
		which--;
		colors.e[which]=tmp;
	}
	while (which<colors.n-1 && tmp->t>colors.e[which+1]->t) {
		colors.e[which]=colors.e[which+1];
		which++;
		colors.e[which]=tmp;
	}
	FindBBox();
	touchContents();
	return which;
}

//! Flip the order of the colors.
void GradientData::FlipColors()
{
	GradientDataSpot *tt;
	double tmax=colors.e[colors.n-1]->t, tmin=colors.e[0]->t;
	for (int c=0; c<colors.n; c++) {
		colors.e[c]->t=tmax - (colors.e[c]->t-tmin);
	}
	for (int c=0; c<colors.n/2; c++) {
		tt=colors.e[c];
		colors.e[c]=colors.e[colors.n-c-1];
		colors.e[colors.n-c-1]=tt;
	}
	touchContents();
}

//! Takes pointer, does not make duplicate.
int GradientData::AddColor(GradientDataSpot *spot)
{
	int c=0;
	while (c<colors.n && spot->t>colors.e[c]->t) c++;
	colors.push(spot,1,c);
	DBG cerr <<"Gradient add color to place"<<c<<endl;
	//FindBBox();
	touchContents();
	return c;
}

//! Add a spot with the given color, or interpolated, if col==NULL.
int GradientData::AddColor(double t,ScreenColor *col)
{
	if (col) return AddColor(t,col->red,col->green,col->blue,col->alpha);
	ScreenColor c;
	WhatColor(t,&c);
	touchContents();
	return AddColor(t,c.red,c.green,c.blue,c.alpha);
}
	
//! Place new color in right spot in list.
/*! The color components are in range [0,0xffff].
 */
int GradientData::AddColor(double t,int red,int green,int blue,int alpha)
{
	int c=0;
	if (t<colors.e[0]->t) {
		 // move p1
		double clen=colors.e[colors.n-1]->t-colors.e[0]->t;
		p1-=(colors.e[0]->t-t)/clen*(p2-p1);
	} else if (t>colors.e[colors.n-1]->t) {
		 // move p2
		double clen=colors.e[colors.n-1]->t-colors.e[0]->t;
		p2-=(colors.e[colors.n-1]->t-t)/clen*(p2-p1);
	}
	while (c<colors.n && t>colors.e[c]->t) c++;
	GradientDataSpot *gds=new GradientDataSpot(t,red,green,blue,alpha);
	colors.push(gds,1,c);
	//FindBBox();
	DBG cerr <<"Gradient add color "<<c<<endl;

	touchContents();
	return c;
}

/*! From coordinate in data space, return the color at it.
 * Return 0 for success, or nonzero for coordinate out of range.
 */
int GradientData::WhatColor(flatpoint p, Laxkit::ScreenColor *col)
{
	double x=p.x;
	double y=p.y;
	
	if (!(style&GRADIENT_RADIAL)) {
		 //linear gradient, much easier
		if (r1>r2) {
			if (y>r1 || y<r2) return 1; //out of y bounds
		} else if (y<r1 || y>r2) return 2;

		if (p1<p2) {
			if (x<p1 || x>p2) return 3;
		} else if (x>p1 || x<p2) return 4;

		return WhatColor(colors.e[0]->t + (colors.e[colors.n-1]->t-colors.e[0]->t)*(x-p1)/(p2-p1), col);
	}
	
	 //else radial gradient
//	***
//	if (p2+r2<=p1+r1 && p2-r2>=p1-r1) {
//		 //circle 2 is entirely contained in circle 1
//	} else if (p2+r2>=p1+r1 && p2-r2<=p1-r1) {
//		 //circle 1 is entirely contained in circle 2
//	}
	 // ***** HACK! just looks in plane circle 2 radius centered at p2
	return WhatColor(colors.e[0]->t + (colors.e[colors.n-1]->t-colors.e[0]->t)*norm(p-flatpoint(p2,0))/r2, col);
}

//! Figure out what color lays at coordinate t.
/*! If t is before the earliest point then the earliest point is used
 * for the color, and -1 is returned. Similarly for beyond the final point, but
 * 1 is returned. Otherwise, the color is linearly interpolated between
 * the nearest points, and 0 is returned.
 */
int GradientData::WhatColor(double t,ScreenColor *col)
{
	int c=0;
	while (c<colors.n && t>colors.e[c]->t) c++;
	if (c==0) { *col=colors.e[0]->color; return -1; }
	if (c==colors.n)  { *col=colors.e[colors.n-1]->color; return 1; }
	ScreenColor *c1=&colors.e[c-1]->color,
				 *c2=&colors.e[c]->color;
	t=(t-colors.e[c-1]->t)/(colors.e[c]->t-colors.e[c-1]->t);
	col->red  = (unsigned short) (t*c2->red   + (1-t)*c1->red);
	col->green= (unsigned short) (t*c2->green + (1-t)*c1->green);
	col->blue = (unsigned short) (t*c2->blue  + (1-t)*c1->blue);
	col->alpha= (unsigned short) (t*c2->alpha + (1-t)*c1->alpha);
	return 0;
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
int GradientData::WhatColor(double t,double *col)
{
	int c=0;
	while (c<colors.n && t>colors.e[c]->t) c++;
	if (c==0) {
		col[0]= (double) (colors.e[0]->color.alpha) /65535;
		col[1]= (double) (colors.e[0]->color.red)   /65535;
		col[2]= (double) (colors.e[0]->color.green) /65535;
		col[3]= (double) (colors.e[0]->color.blue)  /65535;
		return -1; 
	}
	if (c==colors.n)  {
		col[0]= (double) (colors.e[colors.n-1]->color.alpha) /65535;
		col[1]= (double) (colors.e[colors.n-1]->color.red)   /65535;
		col[2]= (double) (colors.e[colors.n-1]->color.green) /65535;
		col[3]= (double) (colors.e[colors.n-1]->color.blue)  /65535;
		return 1; 
	}
	ScreenColor *c1=&colors.e[c-1]->color,
				 *c2=&colors.e[c]->color;
	t=(t-colors.e[c-1]->t)/(colors.e[c]->t-colors.e[c-1]->t);

	col[0]= (double) (t*c2->alpha + (1-t)*c1->alpha) /65535;
	col[1]= (double) (t*c2->red   + (1-t)*c1->red)   /65535;
	col[2]= (double) (t*c2->green + (1-t)*c1->green) /65535;
	col[3]= (double) (t*c2->blue  + (1-t)*c1->blue)  /65535;

	return 0;
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
 *
 * \todo must rethink about rendering to buffers! must be able to handle 16 bit per channel buffers,
 *   but to be effective, this really means being able to handle arbitrary transformations, which in turn
 *   says what actually has to be rendered...
 * \todo *** rendering radial gradients VERY inefficient here..
 * \todo radial draw assumes argb
 */
int GradientData::renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels)
{
	DBG cerr <<"...GradientData::renderToBuffer()"<<endl;

	int i=0;
	int numchan=4; //***
	if (bufchannels!=numchan) return 1;
	int c,y,x;
	bufdepth/=8;
	if (bufdepth!=1 && bufdepth!=2) return 2;
	if (bufstride==0) bufstride=bufw*bufchannels*bufdepth;

	memset(buffer, 0, bufstride*bufh*bufdepth);

	double color[numchan];
	int tempcol;
	if (!(style&GRADIENT_RADIAL)) {
		 //linear gradient, easy!
		for (i=0,x=0; x<bufw; x++) {
			WhatColor(colors.e[0]->t+((double)x/bufw)*(colors.e[colors.n-1]->t-colors.e[0]->t), color);
			for (c=0; c<numchan; c++) { //apparently in byte order, it goes bgra
				if (bufdepth==1) {
					buffer[i]=(unsigned char)(color[3-c]*255+.5);
					//if (c==3) buffer[i]=128; else buffer[i]=255;
					i++;
				} else {
					tempcol=(int)(color[c]*65535+.5);
					buffer[i]=(tempcol&0xff00)>>8;
					i++;
					buffer[i]=(tempcol&0xff);
					i++;
				}
			}
		}
		 //now copy that row for each of the other rows
		 //*** this could be slightly sped up by copying the 1st row, then copying those 2 rows, 
		 //then those 4 rows, etc, rather than do one by one
		for (i=bufstride,y=1; y<bufh; y++, i+=bufstride) {
			memcpy(buffer+i, buffer, bufstride);//dest,src,n
		}
		return 0;
	}


	 //--- else is radial gradient
	double scalex=bufw/(maxx-minx);
	double px,py;
	double cp,O1,O2,o1,o2,v;
	double R1,R2,r1x,r2x,r,ry,cstart,clen;
	ScreenColor col,col0,col1;
	int len,c2,c3;
	int ell; //number of points to approximate circles with

	O1=(p1-minx)*scalex;
	O2=(p2-minx)*scalex;
	cstart=colors.e[0]->t;
	clen=colors.e[colors.n-1]->t - cstart;

	R1=r1;
	R2=r2;

	 //for each color segment...
	for (c=0; c<colors.n-1; c++) {
		o1=O1+(O2-O1)*(colors.e[c  ]->t-cstart)/clen; //this color segment's start and end centers
		o2=O1+(O2-O1)*(colors.e[c+1]->t-cstart)/clen;
		r1x=scalex * (R1+(R2-R1)*(colors.e[c  ]->t-cstart)/clen);//segment's start and end radii
		r2x=scalex * (R1+(R2-R1)*(colors.e[c+1]->t-cstart)/clen);
		v=fabs(o2-o1);

		col0.red  =colors.e[ c ]->color.red;
		col0.green=colors.e[ c ]->color.green;
		col0.blue =colors.e[ c ]->color.blue;
		col0.alpha=colors.e[ c ]->color.alpha;
		col1.red  =colors.e[c+1]->color.red;
		col1.green=colors.e[c+1]->color.green;
		col1.blue =colors.e[c+1]->color.blue;
		col1.alpha=colors.e[c+1]->color.alpha;

		//len=(int)((v+fabs(r1x-r2x))*1.4); //the number of circles to draw so as to have no gaps hopefully
		len=(int)((v+fabs(r1x-r2x))*2); //the number of circles to draw so as to have no gaps hopefully
		for (c2=0; c2<len; c2++) {
			cp=o1+v*((float)c2/len); //center of current circle
			r=r1x+(float)c2/len*(r2x-r1x); //radius of current circle
			ry=r*bufh/bufw/(maxy-miny)*(maxx-minx);
			coloravg(&col,&col0,&col1,(float)c2/len);

			ell=(int)(2*M_PI*r*2);
			for (c3=0; c3<ell; c3++) { 
				px=(int)(cp     +  r*cos((float)c3/(ell-1)*2*M_PI) + .5);
				py=(int)(bufh/2 + ry*sin((float)c3/(ell-1)*2*M_PI) + .5);

				//DBG cerr <<"render radial: p:("<<px<<","<<py<<") r="<<r<<endl;
				//DBG if (px<0 || px>=bufw || py<0 || py>=bufh) cerr <<" ********* Warning! gradient render out of bounds!!"<<endl;

				if (px<0) px=0; else if (px>=bufw) px=bufw-1;
				if (py<0) py=0; else if (py>=bufh) py=bufh-1;

				i=py*bufstride + px*bufchannels*bufdepth;

				 //put in buffer
				if (bufdepth==1) { //8bit per channel
					buffer[i++]=(unsigned char)((col.blue &0xff00)>>8);
					buffer[i++]=(unsigned char)((col.green&0xff00)>>8);
					buffer[i++]=(unsigned char)((col.red  &0xff00)>>8);
					buffer[i++]=(unsigned char)((col.alpha&0xff00)>>8);
				} else { //16bit per channel
					buffer[i++]=(unsigned char)((col.alpha&0xff00)>>8);
					buffer[i++]=(unsigned char) (col.alpha&0xff);
					buffer[i++]=(unsigned char)((col.red  &0xff00)>>8);
					buffer[i++]=(unsigned char) (col.red  &0xff);
					buffer[i++]=(unsigned char)((col.green&0xff00)>>8);
					buffer[i++]=(unsigned char) (col.green&0xff);
					buffer[i++]=(unsigned char)((col.blue &0xff00)>>8);
					buffer[i++]=(unsigned char) (col.blue &0xff);
				}
			}
		}
	}

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
#define GP_Nothing     -9
#define GP_Nodata      -8
#define GP_OutsideData -7
#define GP_DataNoPoint -6
#define GP_a           -5
#define GP_r1          -4
#define GP_r2          -3
#define GP_p1          -2
#define GP_p2          -1
//users can still select points down to GP_MinMoveable
#define GP_MinMoveable -5
#define GP_Min         -6

//! Default is a linear black to red.
GradientInterface::GradientInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	usepreview=0;
	//usepreview=1;
	controlcolor=rgbcolor(0,148,178); 

	//col1.red =col1.blue =0; col1.green=col1.alpha=0xffff;
	col1.red =col1.green=0; col1.blue=col1.alpha=0xffff;
	col2.blue=col2.green=0; col2.red =col2.alpha=0xffff;

	gradienttype=0;
	data=NULL;
	goc=NULL;
	showdecs=3;
	curpoint=GP_Nothing;
	creationstyle=0;
	createv=flatpoint(20,0);
	creationstyle=GRADIENT_LINEAR;
	creater1=50;
	creater2=50;

	needtodraw=1;
	sc=NULL;
}

//! Empty destructor.
GradientInterface::~GradientInterface() 
{ 
	deletedata();
	if (sc) sc->dec_count();
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
	if (gradienttype==2) return _("Radial Gradient Tool");
	if (gradienttype==1) return _("Linear Gradient Tool");
	return _("Gradient Tool");
}

//! Sets showdecs=1, and needtodraw=1.
int GradientInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

//! Calls Clear(), sets showdecs=0, and needtodraw=1.
int GradientInterface::InterfaceOff()
{ 
	Clear(NULL);
	curpoint=GP_Nothing;
	curpoints.flush();
	showdecs=0;
	needtodraw=1;
	return 0;
}

//! Basically clear data. Decrement its count, and set to NULL.
void GradientInterface::deletedata()
{
	if (data) { data->dec_count(); data=NULL; }
	if (goc) { delete goc; goc=NULL; }
}

int GradientInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	GradientData *ndata=dynamic_cast<GradientData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) deletedata();
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

//! Uses GradientData and foreground of a LineStyle
int GradientInterface::UseThis(anObject *newdata,unsigned int) // assumes not use local
{
	if (!newdata) return 0;
	if (newdata==data) return 1;

	if (data && dynamic_cast<LineStyle *>(newdata)) { // make all selected points have this color
		DBG cerr <<"Grad new color stuff"<< endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(newdata);
		if (nlinestyle->mask&GCForeground) if (data && curpoints.n) {
			//int r=data->colors.e[c]->red,g=data->colors.e[c]->green,b=data->colors.e[c]->blue;
			for (int c=0; c<curpoints.n; c++) {
				data->colors.e[curpoints.e[c]]->color.red  =nlinestyle->color.red;
				data->colors.e[curpoints.e[c]]->color.green=nlinestyle->color.green;
				data->colors.e[curpoints.e[c]]->color.blue =nlinestyle->color.blue;
				data->colors.e[curpoints.e[c]]->color.alpha=nlinestyle->color.alpha;
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
	int td=showdecs,ntd=needtodraw;
	showdecs=2;
	needtodraw=1;
	Refresh();
	needtodraw=ntd;
	showdecs=td;
	data=gd;
	return 1;
}

void GradientInterface::drawLinear2()
{
	flatpoint p1(data->p1,0);
	flatpoint p2(data->p2,0);

	double offsets[data->colors.n];
	ScreenColor colors[data->colors.n];

	for (int c=0; c<data->colors.n; c++) {
		offsets[c]=data->GetNormalizedT(c);
		colors[c] =data->colors.e[c]->color;
	}

	dp->setLinearGradient(3, p1.x,p1.y, p2.x,p2.y, offsets, colors, data->colors.n);

	//flatpoint v1=flatpoint(0,data->r1) - flatpoint(0,0);
	//flatpoint v2=flatpoint(0,data->r2) - flatpoint(0,0);

	dp->moveto(data->p1,data->r1);
	dp->lineto(data->p2,data->r1);
	dp->lineto(data->p2,-data->r2);
	dp->lineto(data->p1,-data->r2);
	dp->closed();
	dp->fill(0);
}

//! Draw linear gradient. Called from Refresh.
/*! This assumes that dp has transform to object space.
 *
 * Draws in x:[p1,p2], y:[r1,-r2]
 */
void GradientInterface::drawLinear()
{
	if (dp->Capability(DRAW_LinearGradient)) { drawLinear2(); return; }


	// if the x span is larger than the y span, then for each
	// x pixel draw a line, otherwise for each y pixel.
	dp->DrawScreen();

	flatpoint cp,x0,x1,v,v1,v2;
	double clen,cstart;
	ScreenColor col,col0,col1; //***note this shadows GradientInterface::col1
	int len,c,c2;

	cstart=data->colors.e[0]->t;
	clen=data->colors.e[data->colors.n-1]->t - cstart;
	for (c=0; c<data->colors.n-1; c++) {
		x0=dp->realtoscreen(flatpoint(data->p1+(data->p2-data->p1)*(data->colors.e[c  ]->t-cstart)/clen,0));
		x1=dp->realtoscreen(flatpoint(data->p1+(data->p2-data->p1)*(data->colors.e[c+1]->t-cstart)/clen,0));

		if (x1.x<x0.x) {//go in decreasing x dir
			v=x0-x1;
			v1=x0;
			x0=x1;
			x1=v1;
			col1.red  =data->colors.e[ c ]->color.red;
			col1.green=data->colors.e[ c ]->color.green;
			col1.blue =data->colors.e[ c ]->color.blue;
			col1.alpha=data->colors.e[ c ]->color.alpha;
			col0.red  =data->colors.e[c+1]->color.red;
			col0.green=data->colors.e[c+1]->color.green;
			col0.blue =data->colors.e[c+1]->color.blue;
			col0.alpha=data->colors.e[c+1]->color.alpha;

		} else {//go in increasing x dir
			v=x1-x0;
			col0.red  =data->colors.e[ c ]->color.red;
			col0.green=data->colors.e[ c ]->color.green;
			col0.blue =data->colors.e[ c ]->color.blue;
			col0.alpha=data->colors.e[ c ]->color.alpha;
			col1.red  =data->colors.e[c+1]->color.red;
			col1.green=data->colors.e[c+1]->color.green;
			col1.blue =data->colors.e[c+1]->color.blue;
			col1.alpha=data->colors.e[c+1]->color.alpha;
		}

		if (v.x>fabs(v.y)) { // for each x pixel..
			//start=(int)x0.x; 
			len=(int)(v.x+.5); 

		} else { // for each y pixel
			//start=(int)x0.y;
			len=(int)(v.y+.5);
			if (len<0) {
				len=-len;
				col=col0;
				col0=col1;
				col1=col;
				v=-v;
				x0=x1;
			}
		}
		
		v1=dp->realtoscreen(flatpoint(0,data->r1)) - dp->realtoscreen(flatpoint(0,0));
		v2=dp->realtoscreen(flatpoint(0,data->r2)) - dp->realtoscreen(flatpoint(0,0));

		for (c2=0; c2<=len; c2++) {
			coloravg(&col,&col0,&col1,(float)c2/len);
			cp=x0+v*((float)c2/len);
			dp->NewFG(&col);
			dp->drawline(cp+v1,cp-v2);
		}
	}
	dp->DrawReal();

	// other potential drawing methods: draw first/last, then middle, recursing?
}

void GradientInterface::drawRadial2()
{
	DBG cerr <<" ....drawing with GradientInterface::drawRadial2()"<<endl;

	flatpoint p1(data->p1,0);
	flatpoint p2(data->p2,0);

	double offsets[data->colors.n];
	ScreenColor colors[data->colors.n];

	for (int c=0; c<data->colors.n; c++) {
		offsets[c]=data->GetNormalizedT(c);
		colors[c] =data->colors.e[c]->color;
	}

	 //the number: 0=none, 1=repeat, 2=reflect, 3=pad
	dp->setRadialGradient(0, p1.x,p1.y,data->r1, p2.x,p2.y,data->r2, offsets, colors, data->colors.n);

	//flatpoint v1=flatpoint(0,data->r1) - flatpoint(0,0);
	//flatpoint v2=flatpoint(0,data->r2) - flatpoint(0,0);

	dp->drawrectangle(data->minx,data->miny, data->maxx-data->minx, data->maxy-data->miny, 1);
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
	if (dp->Capability(DRAW_RadialGradient)) { drawRadial2(); return; }


	flatpoint p,cp,O1,O2,o1,o2,xaxis,yaxis,v;
	double R1,R2,r1,r2,r,s,cstart,clen;
	ScreenColor col,col0,col1; //***note this shadows GradientInterface::col1
	int len,c,c2,c3,ell=30;//ell is number of points to approximate circles with
	flatpoint points[ell];

	dp->DrawScreen();
	O1=dp->realtoscreen(flatpoint(data->p1,0)); //the data's p1 and p2
	O2=dp->realtoscreen(flatpoint(data->p2,0));
	cstart=data->colors.e[0]->t;
	clen=data->colors.e[data->colors.n-1]->t - cstart;

	R1=data->r1;
	R2=data->r2;

	xaxis=dp->realtoscreen(flatpoint(1,0))-dp->realtoscreen(flatpoint(0,0));
	s=1/norm(xaxis);
	xaxis*=s;
	yaxis=dp->realtoscreen(flatpoint(0,1))-dp->realtoscreen(flatpoint(0,0));
	yaxis*=s;

	 //for each color segment...
	for (c=0; c<data->colors.n-1; c++) {
		o1=O1+(O2-O1)*(data->colors.e[c  ]->t-cstart)/clen; //this color segment's start and end centers
		o2=O1+(O2-O1)*(data->colors.e[c+1]->t-cstart)/clen;
		r1=1/s*(R1+(R2-R1)*(data->colors.e[c  ]->t-cstart)/clen);//segment's start and end radii
		r2=1/s*(R1+(R2-R1)*(data->colors.e[c+1]->t-cstart)/clen);
		v=o2-o1;
		col0.red  =data->colors.e[ c ]->color.red;
		col0.green=data->colors.e[ c ]->color.green;
		col0.blue =data->colors.e[ c ]->color.blue;
		col0.alpha=data->colors.e[ c ]->color.alpha;
		col1.red  =data->colors.e[c+1]->color.red;
		col1.green=data->colors.e[c+1]->color.green;
		col1.blue =data->colors.e[c+1]->color.blue;
		col1.alpha=data->colors.e[c+1]->color.alpha;
		len=(int)(norm(v)+fabs(r1-r2)+.5); //the number of circles to draw so as to have no gaps hopefully
		for (c2=0; c2<len; c2++) {
			cp=o1+v*((float)c2/len); //center of current circle
			//DBG dp->draw((int)cp.x,(int)cp.y,7,7,0,9);
			r=r1+(float)c2/len*(r2-r1); //radius of current circle
			for (c3=0; c3<ell; c3++) { 
				p=cp + r*cos((float)c3/(ell-1)*2*M_PI)*xaxis + r*sin((float)c3/(ell-1)*2*M_PI)*yaxis;
				points[c3].x=(int)p.x;
				points[c3].y=(int)p.y;
				//DBG cerr <<c2<<":"<<c3<<": p="<<p.x<<','<<p.y<<"  "<<points[c3].x<<','<<points[c3].y<<endl;
			}
			coloravg(&col,&col0,&col1,(float)c2/len);
			dp->NewFG(&col);
			dp->drawlines(points,ell,1,0);
		}
	}
	dp->DrawReal();
}

//! Draw a single line at t, where t==0 is data->p1 and t==1 is data->p2.
/*! Uses the current color and line width settings. This should only be called
 * from Refresh, as it assumes dp to be set to gradient space.
 */
void GradientInterface::drawRadialLine(double t)
{
	flatpoint o,p,cp,O1,O2,o1,o2,xaxis,yaxis,v;
	double r,s;
	int c3,ell=30;
	flatpoint points[ell];


	O1=dp->realtoscreen(flatpoint(data->p1,0));
	O2=dp->realtoscreen(flatpoint(data->p2,0));
	o=(1-t)*O1+t*O2;
	
	xaxis=dp->realtoscreen(flatpoint(1,0))-dp->realtoscreen(flatpoint(0,0));
	s=1/norm(xaxis);
	xaxis*=s;
	yaxis=dp->realtoscreen(flatpoint(0,1))-dp->realtoscreen(flatpoint(0,0));
	yaxis*=s;

	r=((1-t)*data->r1 + t*data->r2)/s;
	for (c3=0; c3<ell; c3++) {
		p=o + r*cos((float)c3/(ell-1)*2*M_PI)*xaxis + r*sin((float)c3/(ell-1)*2*M_PI)*yaxis;
		points[c3].x=(int)p.x;
		points[c3].y=(int)p.y;
		//DBG cerr <<c2<<":"<<c3<<": p="<<p.x<<','<<p.y<<"  "<<points[c3].x<<','<<points[c3].y<<endl;
	}
	dp->DrawScreen();
	dp->LineWidthScreen(6);
	dp->drawlines(points,ell,1,0);
	dp->DrawReal();
}

//! Refresh, calls drawLinear() and drawRadial(), then draws decorations.
/*! \todo the control points should be drawn differently, p1 and p2 should be little
 * triangles that point to each other, and r1 and r2 should point to each other.
 */
int GradientInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!data) {
		if (needtodraw) needtodraw=0;
		return 1;
	}


	//DBG cerr <<"  GradientRefresh-";

	dp->LineAttributes(2,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);


	// draw the color
	if (showdecs&2 && data->colors.n) { 
		int d=1;
		if (usepreview) {
			LaxImage *preview=data->GetPreview();
			if (preview) {
				d=dp->imageout(preview,data->minx,data->miny, data->maxx-data->minx, data->maxy-data->miny);
				if (d<0) d=1; else d=0; //draw lines if problem with image
			}
			DBG if (d) cerr<<"- - - gradient didn't used preview image"<<endl;
			DBG else cerr <<"- - - gradient used preview image "<<preview->w()<<"x"<<preview->h()<<endl;
		}

		if (d) {
			if (data->style&GRADIENT_RADIAL) drawRadial();
			else drawLinear(); // is GRADIENT_LINEAR
			//else drawLinear2(); // is GRADIENT_LINEAR
		}
	}

	// draw control points
	if (showdecs&1) { 
		dp->BlendMode(LAXOP_Over);
		dp->NewFG(controlcolor);
		dp->DrawReal();
		dp->LineWidthScreen(1);

		flatpoint p;
		int c;

		// draw arrow on p1 to p2 and a line between color spots
		dp->drawarrow(getpoint(GP_p1,0),getpoint(GP_p2,0)-getpoint(GP_p1,0),0,1,2);
		dp->drawline(getpoint(0,0),getpoint(data->colors.n-1,0));

		// draw the spots (draw open)
		dp->DrawScreen();
		dp->LineWidthScreen(1);
		for (int c=0; c<data->colors.n; c++) {
			p=dp->realtoscreen(getpoint(c,0));
			dp->NewFG(&data->colors.e[c]->color);
			dp->drawpoint((int)p.x,(int)p.y,(c==curpoint?5:3),1);
			dp->NewFG(controlcolor);

			//if (c<0) dp->draw((int)p.x,(int)p.y,5,5,0,(c==GP_p1 || c==GP_p2)?3:2);
			//else
			dp->drawpoint((int)p.x,(int)p.y,(c==curpoint?5:3),0);
		}
		dp->DrawReal();

		//		 // curpoints (draw closed)
		//		 // non-color controls
		//		if (curpoint>-5 && curpoint<0) {
		//			dp->NewFG(controlcolor);
		//			p=dp->realtoscreen(getpoint(curpoint,0));
		//			dp->draw((int)p.x,(int)p.y,5,5,1,(curpoint==GP_p1 || curpoint==GP_p2)?3:2);
		//		}

		if (curpoint<0 && curpoint>=GP_Min) {
			flatpoint ul,ur,ll,lr;
			//ul=transform_point(data->m(),data->p1,data->r1);
			//ur=transform_point(data->m(),data->p2,data->r1);
			//ll=transform_point(data->m(),data->p1,-data->r2);
			//lr=transform_point(data->m(),data->p2,-data->r2);
			ul=flatpoint(data->p1,data->r1);
			ur=flatpoint(data->p2,data->r1);
			ll=flatpoint(data->p1,-data->r2);
			lr=flatpoint(data->p2,-data->r2);
			dp->LineWidthScreen(6);
			dp->NewFG(controlcolor);

			if (data->style&GRADIENT_RADIAL) {
				if (curpoint==GP_r1) {
					drawRadialLine(0);
				} else if (curpoint==GP_r2) {
					drawRadialLine(1);
				}
			} else {
				if (curpoint==GP_p1) {
					dp->drawline(ul,ll);
				} else if (curpoint==GP_p2) {
					dp->drawline(ur,lr);
				} else if (curpoint==GP_r1) {
					dp->drawline(ul,ur);
				} else if (curpoint==GP_r2) {
					dp->drawline(ll,lr);
				}
			}
		}


		//draw selected color spots bigger
		if (curpoints.n) {
			dp->DrawScreen();
			dp->LineWidthScreen(1);

			for (c=0; c<curpoints.n; c++) {
				p=dp->realtoscreen(getpoint(curpoints.e[c],0));
				dp->NewFG(&data->colors.e[curpoints.e[c]]->color);
				dp->drawpoint((int)p.x,(int)p.y,5,1);  // draw curpoint
				dp->NewFG(controlcolor);
				dp->drawpoint((int)p.x,(int)p.y,5,0);
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
/*! c is:
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
	if (!data || !(c>=-4 && c<data->colors.n)) return flatpoint();
	flatpoint p;

	if (c==GP_a) {
		if (data->style&GRADIENT_RADIAL) 
			p=flatpoint(data->p1+cos(data->a)*data->r1,sin(data->a)*data->r1);
		else p=flatpoint(0,data->a);

	} else if (c==GP_r1) //r1
		if (data->style&GRADIENT_RADIAL) p=flatpoint(data->p1,data->r1);
		else p=flatpoint(0,data->r1);

	else if (c==GP_r2) //r2
		if (data->style&GRADIENT_RADIAL) p=flatpoint(data->p2,data->r2);
		else p=flatpoint(0,-data->r2);

	else if (c==GP_p2) p.x=data->p2;
	else if (c==GP_p1) p.x=data->p1;
	else {
		double cstart=data->colors.e[0]->t,
			   clen=data->colors.e[data->colors.n-1]->t-cstart;
		if (clen==0) p=flatpoint(data->p1,0);
		else {
			flatpoint p1,p2;
			if (data->style&GRADIENT_RADIAL) {
				// distribute at angle ??? corresponding to their spot on screen..
				p1=flatpoint(data->p1+cos(data->a)*data->r1,sin(data->a)*data->r1);
				p2=flatpoint(data->p2+cos(data->a)*data->r2,sin(data->a)*data->r2);
			} else {
				p1=flatpoint(data->p1,data->a);
				p2=flatpoint(data->p2,data->a);
			}
			p=p1+(data->colors.e[c]->t-cstart)/clen*(p2-p1);
		}
	}

	if (trans) p=transform_point(data->m(),p);
	return p;
}

//! Scan for point.
/*! Returns:
 * <pre>
 *   -8=no data
 *   -7 data but not on it
 *   -6 on data but not on a point 
 *   -5 on controller for data->a
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
	flatpoint p,p2;
	p=screentoreal(x,y); //<-remember this is not including data's transform
	double d=5/Getmag(), //d eventually is (5 pixels in gradient space)^2
		   dd;
	DBG cerr <<" gd scan d="<<d<<"(x,y)="<<p.x<<','<<p.y<<endl;
	d*=d;
	int closest=GP_OutsideData;

	// check this order: color spots, p2,p1,r2,r1
	int c;
	for (c=data->colors.n-1; c>=0; c--) { //scan for spots
		p2=getpoint(c,1);
		dd=(p2.x-p.x)*(p2.x-p.x)+(p2.y-p.y)*(p2.y-p.y);
		if (dd<d) {
			d=dd;
			closest=c;
		}
	}
	if (closest!=GP_OutsideData) return closest;

	 //check for click along edges
	p2=transform_point_inverse(data->m(),p); //p2 is point in gradient space
	flatpoint p3;
	if (data->style&GRADIENT_RADIAL) {
		d=sqrt(d);
		dd=norm(p2-flatpoint(data->p1,0));
		if (fabs(dd-fabs(data->r1))<d) return GP_r1;

		dd=norm(p2-flatpoint(data->p2,0));
		if (fabs(dd-fabs(data->r2))<d) return GP_r2;
		
	} else { // linear gradient
		flatpoint ul,ur,ll,lr;
		ul=transform_point(data->m(),data->p1,data->r1);
		ur=transform_point(data->m(),data->p2,data->r1);
		ll=transform_point(data->m(),data->p1,-data->r2);
		lr=transform_point(data->m(),data->p2,-data->r2);
		d=sqrt(d);

		if (distance(p,ul,ur)<d) return GP_r1;
		if (distance(p,ll,lr)<d) return GP_r2;
		if (distance(p,ul,ll)<d) return GP_p1;
		if (distance(p,ur,lr)<d) return GP_p2;
	}



	//	 //check for data->a
	//	if (closest==GP_OutsideData) {
	//		flatpoint p1;
	//		if (data->style&GRADIENT_RADIAL) {
	//			p1=getpoint(0,1);
	//			p2=getpoint(data->colors.n-1,1);
	//			double dd=(p2-p1)*(p-p1);
	//			if (dd>=0 && dd<=1) if (norm((p2-p1)|=(p-p1))<d) return GP_a;
	//		} else {
	//			p1=transform_point(data->m(),flatpoint(data->p1,data->a));
	//			p2=transform_point(data->m(),flatpoint(data->p2,data->a));
	//			double dd=(p2-p1)*(p-p1);
	//			if (dd>=0 && dd<=1) if (norm((p2-p1)|=(p-p1))<d) return GP_a;
	//
	//		}
	//	}

	// if not a color spot point, p, or p+v, then...
	if (closest==GP_OutsideData) { // check if in the color somewhere
		c=data->pointin(p);
		if (!c) closest=GP_OutsideData;
		else closest=GP_DataNoPoint;
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
	if (c<0 || c>data->colors.n) return 1;
	curpoints.pushnodup(c); // returns 1 if pushed, 0 if already there
	curpoint=c;
	sendcolor(&data->colors.e[c]->color);
	needtodraw|=2;
	return 0;
}

//! Send a "make curcolor" event to the viewport.
/*! \todo bit of a hack here.
*/
int GradientInterface::sendcolor(ScreenColor *col)
{
	if (!col || !curwindow || !curwindow->win_parent || !curwindow->win_parent->xlib_window) return 0;

	SimpleColorEventData *e=new SimpleColorEventData( 65535, col->red, col->green, col->blue, col->alpha, 0);
	//app->SendMessage(e, curwindow->object_id, "make curcolor", object_id);
	app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);

	return 1;
}

#define DRAG_NORMAL      1
#define DRAG_NEW         2
#define DRAG_FROM_INSIDE 3

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
	mx=x;
	my=y;
	draggingmode=DRAG_NORMAL;
	buttondown.down(d->id,LEFTBUTTON);

	 // straight click
	int c=scan(x,y);
	DBG cerr <<"******************* gradient scan:"<<c<<endl;
	if (c>=GP_MinMoveable) { // scan found a moveable point...
		curpoint=c;
		if (curpoint>0 && curpoint<data->colors.n-1) draggingmode=DRAG_FROM_INSIDE;
		if (curpoint<0) { // is a non-color control point
			curpoints.flush();
		} else {
			 // click down on a color spot.
			if ((state&LAX_STATE_MASK)==0) {
				 // plain click always deselect else and selects the
				 // one clicked on
				curpoints.flush();
				curpoints.push(c);
				sendcolor(&data->colors.e[c]->color);
				needtodraw|=2;
				return 0;
			} else {
				if (curpoints.pushnodup(curpoint)) {
					 // curpoint is not already in curpoints, 
					sendcolor(&data->colors.e[curpoint]->color);
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
	} else if (c==GP_DataNoPoint && count!=2) { 
		 // point not found but on data, if plain click flush points, return..
		leftp=transform_point_inverse(data->m(),screentoreal(x,y));
		if ((state&LAX_STATE_MASK)==0) {
			curpoints.flush();
			curpoint=GP_DataNoPoint;
			DBG cerr <<"  Gradient leftp: "<<leftp.x<<','<<leftp.y<<endl;
			needtodraw|=2;
			return 0;
		}
	}
	DBG cerr <<"  no gradient pointfound  "<<endl;
	
	 // make new color point if shift-click not on an existing point
	if (data && ((state&LAX_STATE_MASK)==ShiftMask || count==2)) {
		if (curpoint>=GP_MinMoveable) return 0;
		flatpoint p=screentoreal(x,y);
		curpoints.flush();

		double t,clen,cstart;
		cstart=data->colors.e[0]->t;
		clen=data->colors.e[data->colors.n-1]->t - cstart;
		flatpoint p1,v;
		
		//if (data->style&GRADIENT_RADIAL) {
			//p1=getpoint(0,1);
			//v=getpoint(data->colors.n-1,1)-p1;
		//} else {
			//p1=getpoint(GP_p1,1);
			//v=getpoint(GP_p2,1)-p1;
		//}
		p1=getpoint(0,1);
		v=getpoint(data->colors.n-1,1)-p1;
		
		t=cstart + clen*((p-p1)*v)/(v*v);
		//ScreenColor cc;
		//data->WhatColor(t,&cc);
		//curpoint=data->AddColor(t,&cc);
		curpoint=data->AddColor(t,NULL);
		curpoints.push(curpoint);
		sendcolor(&data->colors.e[curpoint]->color);

		needtodraw=1;
		return 0;
	}

	 //! Get rid of old data if not clicking in it.
	if (data && c==GP_OutsideData) {
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
			if (viewport) viewport->ChangeObject(oc,0);
			needtodraw=1;
			return 0;
		}
		if (!primary && c==-1 && viewport->ChangeObject(oc,1)) {
			buttondown.up(d->id,LEFTBUTTON);
			deletedata();
			return 0;
		}
	}
	
	 // make new one
	newData(x,y);
	curpoint=GP_p2;
	draggingmode=DRAG_NEW;
	DBG cerr <<"new gradient:"<<endl;
	DBG data->dump_out(stderr,2,0,NULL);

	needtodraw=1;
	return 0;
	DBG cerr <<"..gradlbd done   ";
}

//! Create new data for screen point x,y, with a count of 1 plus viewport counts via viewport->NewData(ndata).
void GradientInterface::newData(int x,int y)
{
	deletedata();

	 //create with count 1
	GradientData *ndata=NULL;
	ndata=dynamic_cast<GradientData *>(somedatafactory()->NewObject(LAX_GRADIENTDATA));
	if (ndata) {
		ndata->Set(screentoreal(x,y),screentoreal(x,y),
						creater1,creater2,&col1,&col2,creationstyle);
	} 
	if (!ndata) ndata=new GradientData(screentoreal(x,y),screentoreal(x,y),
						creater1,creater2,&col1,&col2,creationstyle);
	
	ObjectContext *oc=NULL;
	if (viewport) viewport->NewData(ndata,&oc);
	data=ndata;
	if (goc) delete goc;
	goc=oc?oc->duplicate():NULL;
	data->FindBBox();
}

//! If data, then call viewport->ObjectMoved(data).
int GradientInterface::LBUp(int x,int y,unsigned int state,const LaxMouse *d) 
{
	draggingmode=DRAG_NORMAL;
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	if (curpoint<0) curpoint=GP_Nothing;
	if (data && viewport) viewport->ObjectMoved(goc,1);
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}


int GradientInterface::MouseMove(int x,int y,unsigned int state,const LaxMouse *mouse) 
{
	DBG cerr <<"--------------gradient point scan:"<<scan(x,y)<<endl;
	if (!data) { return 1;}
	if (!buttondown.isdown(mouse->id,LEFTBUTTON)) {
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
			a=(x-mx)/180.0*M_PI;
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
		data->touchContents();
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
		data->touchContents();
		needtodraw=1;

	} else if (movepoint==GP_p2 || movepoint==GP_p1) { 
		if (((state&LAX_STATE_MASK)==ShiftMask && draggingmode==DRAG_NORMAL)
			 || ((state&LAX_STATE_MASK)==0 && draggingmode==DRAG_NEW)) {
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
			data->modtime=time(NULL);

			 //remap d
			d=transform_vector(m,d||np);
		}
		if (movepoint==GP_p1) data->p1+=d.x;
		else data->p2+=d.x;
		data->touchContents();
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

enum GradientInterfaceActions {
	GRAD_SelectLeft,
	GRAD_SelectRight,
	GRAD_Decorations,
	GRAD_Flip,
	GRAD_MakeLinear,
	GRAD_MakeRadial,
	GRAD_Select,
	GRAD_Delete,
	GRAD_MAX
};

Laxkit::ShortcutHandler *GradientInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(GRAD_SelectLeft,  LAX_Left,0,0,  "SelectNext",  _("Select next"),NULL,0);
	sc->Add(GRAD_SelectRight, LAX_Right,0,0, "SelectPrev",  _("Select previous"),NULL,0);
	sc->Add(GRAD_Decorations, 'd',0,0,       "Decorations", _("Toggle decorations"),NULL,0);
	sc->Add(GRAD_Flip,        'f',0,0,       "FlipColors",  _("Flip order of colors"),NULL,0);
	sc->Add(GRAD_MakeLinear,  'l',0,0,       "Linear",      _("Convert to linear gradient"),NULL,0);
	sc->Add(GRAD_MakeRadial,  'r',0,0,       "Radial",      _("Convert to Radial gradient"),NULL,0);
	sc->Add(GRAD_Select,      'a',0,0,       "Select",      _("Select all, or deselect"),NULL,0);
	sc->Add(GRAD_Delete,      LAX_Del,0,0,   "Delete",      _("Delete"),NULL,0);
	sc->AddShortcut(LAX_Bksp,0,0, GRAD_Delete);

	manager->AddArea(whattype(),sc);
	return sc;
}

int GradientInterface::PerformAction(int action)
{
	if (action==GRAD_SelectLeft) {
		if (!data) return 0;
		curpoints.flush();
		curpoint--;
		if (curpoint<GP_MinMoveable) curpoint=data->colors.n-1;
		if (curpoint>=0) curpoints.push(curpoint);
		DBG cerr <<"gradient curpoint="<<curpoint<<endl;
		needtodraw=1;
		return 0;

	} else if (action==GRAD_SelectRight) {
		if (!data) return 0;
		curpoints.flush();
		curpoint++;
		if (curpoint>=data->colors.n) curpoint=GP_MinMoveable;
		if (curpoint>=0) curpoints.push(curpoint);
		DBG cerr <<"gradient curpoint="<<curpoint<<endl;
		needtodraw=1;
		return 0;

	} else if (action==GRAD_Decorations) {
		if (--showdecs<0) showdecs=3;
		switch (showdecs) {
			case 0: PostMessage(_("Don't show object on top, no decorations")); break;
			case 1: PostMessage(_("Don't show object on top, but show decorations")); break;
			case 2: PostMessage(_("Show object on top without decorations")); break;
			case 3: PostMessage(_("Show object on top with decorations")); break;
		}
		needtodraw=1;
		return 0;

	} else if (action==GRAD_Flip) {
		if (data) { data->FlipColors(); needtodraw=1; }
		return 0;

	} else if (action==GRAD_MakeLinear) {
		if (!data || !(data->style&GRADIENT_RADIAL)) { creationstyle=GRADIENT_LINEAR; return 0; }
		data->style&=~(GRADIENT_RADIAL|GRADIENT_LINEAR);
		data->style|=GRADIENT_LINEAR;
		//data->v=data->colors.e[data->colors.n-1]->t-data->colors.e[0]->t;
		data->FindBBox();
		data->a=0;
		needtodraw=1;
		data->touchContents();
		return 0;

	} else if (action==GRAD_MakeRadial) {
		if (!data || data->style&GRADIENT_RADIAL) { creationstyle=GRADIENT_RADIAL; return 0; }
		data->style&=~(GRADIENT_RADIAL|GRADIENT_LINEAR);
		data->style|=GRADIENT_RADIAL;
		//data->p=data->colors.e[0]->t;
		//data->v=data->p;
		data->FindBBox();
		data->touchContents();
		needtodraw=1;
		return 0;

	} else if (action==GRAD_Select) {
		if (curpoints.n || curpoint>=GP_MinMoveable) {
			curpoints.flush();
			curpoint=GP_Nothing;
			needtodraw=1;
		} else if (!curpoints.n && data) {
			for (int c=0; c<data->colors.n; c++) curpoints.push(c);
			curpoint=0;
			needtodraw|=2;
		}
		return 0;

	} else if (action==GRAD_Delete) {
		if (!data || (curpoints.n==0 && curpoint<GP_MinMoveable)) return 0;
		int c;
		if (curpoint==GP_r1) data->r1=0;
		else if (curpoint==GP_r2) data->r2=0;
		else if (curpoints.n) while (curpoints.n) {
			c=curpoints.pop();
			DBG cerr <<"--- deleting gradient spot "<<c<<endl;
			if (data->colors.n>2) data->colors.pop(c);
			if (c==0) { 
				 // move p1 to new first color spot
				flatpoint p=getpoint(0,0);
				data->p1=p.x;
				continue; 
			}
			if (c>=data->colors.n) { 
				 // move p2 to new last color spot
				flatpoint p=getpoint(data->colors.n-1,0);
				data->p2=p.x;
				continue; 
			}
		}
		curpoint=GP_Nothing;
		data->FindBBox();
		data->touchContents();
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
	DBG }

	return 1; 
}

//int GradientInterface::CharRelease(unsigned int ch,unsigned int state) 

} // namespace LaxInterfaces

