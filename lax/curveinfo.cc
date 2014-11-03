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
//    Copyright (C) 2013-2014 by Tom Lechner
//

#include <lax/curveinfo.h>
#include <lax/strmanip.h>
#include <lax/bezutils.h>


#include <iostream>
using namespace std;
#define DBG 


using namespace LaxFiles;


namespace Laxkit {


//-------------------------------- CurveInfo ----------------------------------
/*! \class CurveInfo
 * Info for x -> y transformation.
 * Each x maps onto one y, but y can have more than one x.
 *
 * By default, the range goes linearly from 0 to 1.
 *
 * If wrap==true, then assume the curve is supposed to wrap around, thus f(xmin)==f(xmax).
 *
 * This class approximates mappings from a list of points that are either a polyline (composed
 * of straight segments), a bezier line, or an autosmoothed line which is just a bezier approximation
 * of a collection of points.
 *
 * There needs to be only one point defined.
 * If not wrapping, then initial y is defined as the same as the y of the first point. Similarly
 * for the last point. If wrapping, then initial and final are interpolated according to curvetype
 * when needed.
 *
 * Autosmooth compiles points to fauxpoints for computations.
 *
 * \todo *** Currently the bezier mode is not implemented.
 *
 * Sometimes for speed you want to have a handy lookup table. In this case there are
 * convenience functions to compute this table. Note the lookup table and related
 * functions are not used by f(). f() only uses points (or fauxpoints for autosmooth).
 * Use RefreshLookup() to replenish a lookup table using the existing number of samples.
 * Defualt is 256, and default range is [0..255].
 * Use RefreshLookup(int nsamples, int nmin, int nmax) to set the number of samples. Y values
 * are mapped [ymin..ymax] -> [nmin..nmax].
 * See MakeLookupTable(), and LookupDump() for more info.
 */


CurveInfo::CurveInfo(const char *ntitle,
			  const char *xl, double nxmin, double nxmax,
			  const char *yl, double nymin, double nymax)
{
	base_init();

	xmin=nxmin;  xmax=nxmax;
	ymin=nymin;  ymax=nymax;

	xlabel=newstr(xl);
	ylabel=newstr(yl);
	title=newstr(ntitle);

	 //for simplicity, all points are [0..1]. They are scaled to samples when needed.
	points.push(flatpoint(0,0));
	points.push(flatpoint(1,1));
}

CurveInfo::CurveInfo()
{
	base_init();

	 //for simplicity, all points are [0..1]. They are scaled to samples when needed.
	points.push(flatpoint(0,0));
	points.push(flatpoint(1,1));
}

void CurveInfo::base_init()
{
	curvetype=Autosmooth;
	wrap=false; //whether x wraps around

	xmin=0;  xmax=1;
	ymin=0;  ymax=1;

	xlabel=NULL;
	ylabel=NULL;
	title=NULL;

	numsamples=0;
	lookup=NULL;
	lookup_min=0;
	lookup_max=255;
}

CurveInfo::~CurveInfo()
{
	if (xlabel) delete[] xlabel;
	if (ylabel) delete[] ylabel;
	if (title ) delete[] title;
}

CurveInfo &CurveInfo::operator=(CurveInfo &l)
{
	SetTitle(l.title);
	SetXBounds(l.xmin,l.xmax,l.xlabel,false);
	SetYBounds(l.ymin,l.ymax,l.ylabel,false);
	curvetype=l.curvetype;
	wrap=l.wrap;
	SetDataRaw(l.points.e,l.points.n);
	return l;
}

void CurveInfo::dump_out(FILE *f,int indent,int what,anObject *context)
{
    Attribute att;
    dump_out_atts(&att,0,context);
    att.dump_out(f,indent);
}

LaxFiles::Attribute *CurveInfo::dump_out_atts(LaxFiles::Attribute *att,int what,anObject *context)
{
	if (!att) att=new Attribute(whattype(),NULL);

	if (what==-1) {
		// *** format description
		return att;
	}

	if (title) att->push("title",title);
	if (xlabel) att->push("xlabel",xlabel);
	if (ylabel) att->push("ylabel",ylabel);

	if (curvetype==Linear)     att->push("type","linear");
	if (curvetype==Autosmooth) att->push("type","autosmooth");
	if (curvetype==Bezier)     att->push("type","bezier");

	att->push("xmin",xmin,-1);
	att->push("xmax",xmax,-1);
	att->push("ymin",ymin,-1);
	att->push("ymax",ymax,-1);

	char ss[50], *str=NULL;
	for (int c=0; c<points.n; c++) {
		sprintf(ss,"%.10g,%.10g\n",points.e[c].x,points.e[c].y);
		appendstr(str,ss);
	}

	att->push("points",str);

	return att;
}

/*! Please note RefreshLookup() is NOT called here.
 */
void CurveInfo::dump_in_atts(LaxFiles::Attribute *att,int flag,anObject *context)
{
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"title")) {
			makestr(title,value);

		} else if (!strcmp(name,"type")) {
			if (!value) continue;
			if (!strcmp(value,"linear"))          curvetype=Linear;
			else if (!strcmp(value,"autosmooth")) curvetype=Autosmooth;
			else if (!strcmp(value,"bezier"))     curvetype=Bezier;

		} else if (!strcmp(name,"xlabel")) {
			makestr(xlabel,value);

		} else if (!strcmp(name,"ylabel")) {
			makestr(ylabel,value);

		} else if (!strcmp(name,"xmin")) {
			DoubleAttribute(value,&xmin,NULL);

		} else if (!strcmp(name,"xmax")) {
			DoubleAttribute(value,&xmax,NULL);

		} else if (!strcmp(name,"ymin")) {
			DoubleAttribute(value,&ymin,NULL);

		} else if (!strcmp(name,"ymax")) {
			DoubleAttribute(value,&ymax,NULL);

		} else if (!strcmp(name,"points")) {
			points.flush();
			char *endptr=NULL;
			double x,y;
			while (value) {
				if (!DoubleAttribute(value,&x,&endptr)) break;
				value=endptr;
				while (*value && isspace(*value)) value++;
				if (*value==',') value++;
				if (!DoubleAttribute(value,&y,&endptr)) break;
				points.push(flatpoint(x,y));
				value=endptr;
			}
			 
		}
	}

	 //some sanity checking:
	if (points.n==0) { //default to linear 0..1 when no points defined
		points.push(flatpoint(0,0));
		points.push(flatpoint(1,1));
	}
	for (int c=0; c<points.n; c++) { //clamp to [0..1]
		if (points.e[c].y<0) points.e[c].y=0;
		else if (points.e[c].y>1) points.e[c].y=1;

		if (points.e[c].x<0) points.e[c].x=0;
		else if (points.e[c].x>1) points.e[c].x=1;
	}
 	 //ensure that each point.x is > x of previous point
	for (int c=1; c<points.n-1; c++) {
		// *** todo: it would be more polite to sort, rather than adjust like this:
		if (points.e[c].x<points.e[c-1].x) points.e[c].x=points.e[c-1].x;
	}

	//RefreshLookup();
}

/*! Write out a list of the this->lookup sample table.
 */
void CurveInfo::LookupDump(const char *label,FILE *f)
{
	if (!lookup) RefreshLookup();

	fprintf(f,"%s curve dump:\n",label?label:"");
	for (int c=0; c<numsamples; c++) {
		fprintf(f,"%d  %d\n",c,lookup[c]);
	}
}

//! Refresh the integer approximation lookup table.
void CurveInfo::RefreshLookup()
{
	if (!lookup) {
		if (numsamples<=0) numsamples=256;
		lookup=new int[numsamples];
	}
	MakeLookupTable(lookup, numsamples, lookup_min, lookup_max);
}

//! Refresh the integer approximation lookup table, adjusting sample size and bounds.
/*! If nsamples is > 0, then use that as new sample size and bounds.
 */
void CurveInfo::RefreshLookup(int nsamples, int nmin, int nmax)
{
	if (nsamples>numsamples) {
		if (lookup) delete[] lookup;
		lookup=new int[nsamples];
		numsamples=nsamples;
	} else if (nsamples>0) numsamples=nsamples;

	lookup_min=nmin;
	lookup_max=nmax;

	MakeLookupTable(lookup, numsamples, lookup_min, lookup_max);
}

/*! numentries must be 2 or greater. Calls f() for each element of table.
 * NOTE: Assumes table is already allocated. When called from RefreshLookup(), it uses this->lookup.
 *
 * Return -1 for error, or 0 for success.
 */
int CurveInfo::MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue) 
{
	if (numentries<2) return -1;
	double y;
	double x;
	for (int c=0; c<numentries; c++) {
		x=((double)c)/(numentries-1)*(xmax-xmin) + xmin;
		y=f(x);
		table[c]=((y-ymin)/(ymax-ymin)) * (maxvalue-minvalue) + minvalue;
	}
	return 0;
}

/*! Flush points, and add a singe point at xmin,y.
 */
void CurveInfo::SetFlat(double y)
{
	points.flush();
	fauxpoints.flush();
	AddPoint(xmin,y);
}

/*! Clears points. 
 * If !leaveblank, then also add 2 point linear, 0 to 1 (unmapped).
 * Does NOT change bounds.
 */
void CurveInfo::Reset(bool leaveblank)
{
	points.flush();
	fauxpoints.flush();

	if (!leaveblank) {
		points.push(flatpoint(0,0));
		points.push(flatpoint(1,1));
	}
}

/*! Flush current points, and install the given points.
 * Warning, does NOT make bounds snap to points,
 * but it DOES make points snap to 
 * existing bounds, meaning they are clamped to 0..1 in sample space.
 *
 * Points are copied.
 */
void CurveInfo::SetData(flatpoint *p, int n)
{
	points.flush();
	fauxpoints.flush();

	for (int c=0; c<n; c++) AddPoint(p[c].x,p[c].y);
}

/*! Copies points that are already sorted, and in range [0..1].
 * Does NOT check to ensure this!
 * Also flushes fauxpoints.
 */
void CurveInfo::SetDataRaw(flatpoint *p, int n)
{
	points.flush();
	fauxpoints.flush();

	points.Allocate(n);
	//points.CopyRange(0, p,n);
	memcpy(points.e,p,n*sizeof(flatpoint));
	points.n=n;
}

/*! Flip the y range over miny..maxy.
 */
void CurveInfo::InvertY()
{
	for (int c=0; c<points.n; c++) {
		points.e[c].y=1-points.e[c].y;
	}
	fauxpoints.flush();
}

/*! Flip the x range over miny..maxy.
 */
void CurveInfo::InvertX()
{
	for (int c=0; c<points.n; c++) {
		points.e[c].x=1-points.e[c].x;
	}
	fauxpoints.flush();
}


/*! Set whether values wrap around in the x direction.
 * This implies f(xmin)==f(xmax).
 */
void CurveInfo::Wrap(bool wrapx)
{
	if (wrapx!=wrap) fauxpoints.flush();
	wrap=wrapx;
}

/*! x,y are in actual range defined by current xmin,xmax and ymin,ymax (as opposed to points space 0 to 1).
 * Point is clamped to proper range.
 * If the x is the same as an existing point, then the old y value is replaced with the new one.
 *
 * Return 0 for point added. Return 1 for existing point modified.
 */
int CurveInfo::AddPoint(double x,double y)
{
	flatpoint p((x-xmin)/(xmax-xmin), (y-ymin)/(ymax-ymin));
	if (p.x<0) p.x=0;
	else if (p.x>1) p.x=1;
	if (p.y<0) p.y=0;
	else if (p.y>1) p.y=1;

	for (int c=0; c<points.n; c++) {
		if (p.x<points.e[c].x) {
			points.push(p,c);
			return 0;
		}
		if (p.x==points.e[c].x) {
			points.e[c].y=p.y;
			return 1;
		}
	}
	//if (p.x<1) points.push(p,points.n-1); //push just before final point
	points.push(p,points.n); //push at end
	return 0;
}

/*! Adjust existing point.
 * x value is clamped to adjacent bounds.
 *
 * If wrap==true and moving an endpoint, move the other endpoint to be the same.
 *
 */
int CurveInfo::MovePoint(int index, double x,double y)
{
	flatpoint p((x-xmin)/(xmax-xmin), (y-ymin)/(ymax-ymin));
	if (p.x<0) p.x=0;
	else if (p.x>1) p.x=1;
	if (p.y<0) p.y=0;
	else if (p.y>1) p.y=1;

	if (index==0) {
		if (p.x!=0) p.x=0;
	} else if (index==points.n-1) {
		if (p.x!=1) p.x=1;
	} else if (p.x<points.e[index-1].x) p.x=points.e[index-1].x;
	else if (p.x>points.e[index+1].x) p.x=points.e[index+1].x;

	points.e[index]=p;
	if (wrap) {
		if (p.x==0) points.e[points.n-1]=p;
		else if (p.x==1) points.e[0]=p;
	}
	return 0;
}

void CurveInfo::SetTitle(const char *ntitle)
{
	makestr(title,ntitle);
}

/*! If remap, then remap existing points to adjust for the new bounds. Otherwise,
 * keeps points as old values within the [0..1] range.
 *
 *
 * Only sets xlabel if nxlabel!=NULL.
 * \todo decide which is better, remapping or not remapping
 */
void CurveInfo::SetXBounds(double nxmin, double nxmax, const char *nxlabel, bool remap)
{
	if (remap) for (int c=0; c<points.n; c++) {
		points.e[c].x = ((points.e[c].y*(xmax-xmin)+xmin)-nxmin)/(nxmax-nxmin);
	}

	xmin=nxmin;
	xmax=nxmax;
	if (nxlabel) makestr(xlabel,nxlabel);

	fauxpoints.flush();
}

/*! If remap, then remap existing points to adjust for the new bounds. Otherwise,
 * keeps points as old values within the [0..1] range.
 *
 * Only sets ylabel if nylabel!=NULL.
 */
void CurveInfo::SetYBounds(double nymin, double nymax, const char *nylabel, bool remap)
{
	if (remap) for (int c=0; c<points.n; c++) {
		points.e[c].y = ((points.e[c].y*(ymax-ymin)+ymin)-nymin)/(nymax-nymin);
	}
	ymin=nymin;
	ymax=nymax;
	if (nylabel) makestr(xlabel,nylabel);

	fauxpoints.flush();
}

/*! This is for convenience to call f_bezier(), f_autosmooth(), or f_linear(),
 * depending on the value of curvetype.
 */
double CurveInfo::f(double x)
{
	if (curvetype==Bezier) return f_bezier(x);
	if (curvetype==Autosmooth) return f_autosmooth(x);
	return f_linear(x);
}

//! Return y value for x. Out of range x and y are clamped to bounds.
/*! This approximates directly from points array.
 */
double CurveInfo::f_linear(double x)
{
	//clamp x:
	if (xmax>xmin) {
		if (x<xmin) x=xmin;
		else if (x>xmax) x=xmax;
	} else {
		if (x>xmin) x=xmin;
		else if (x<xmax) x=xmax;
	}

	x=(x-xmin)/(xmax-xmin); //scale to 0..1

	 //find point segment
	int c=0;
	for ( ; c<points.n; c++) {
		if (x<=points.e[c].x) break;
	}

	if (c==0 && !wrap) {
		return (points.e[0].y) * (ymax-ymin) + ymin;
	} else if (c==points.n && !wrap) {
		return (points.e[points.n-1].y) * (ymax-ymin) + ymin;
	} else {
		flatpoint cp;
		if (c==0 && wrap) { cp=points.e[points.n-1]; cp.x-=1; }
		else cp=points.e[c-1];
		
		 //interpolate for segment
		flatpoint v=points.e[c]-cp;
		if (v.x==0) return (cp.y+v.y) * (ymax-ymin) + ymin;

		double d=(x-cp.x)/v.x;
		return (cp.y+d*v.y) * (ymax-ymin) + ymin;
	}
}

//! Return y value for x. Out of range x and y are clamped to bounds.
/*! This approximates from intersecting a line segment with the fauxpoints array.
 * If fauxpoints.n==0, then call MakeFakeCurve() first.
 */
double CurveInfo::f_autosmooth(double x)
{
	//clamp x:
	if (xmax>xmin) {
		if (x<xmin) x=xmin;
		else if (x>xmax) x=xmax;
	} else {
		if (x>xmin) x=xmin;
		else if (x<xmax) x=xmax;
	}
	x=(x-xmin)/(xmax-xmin); //scale to 0..1

	if (!fauxpoints.n) MakeFakeCurve();

	flatpoint p;

	 //check easy points first to avoid false misses
	for (int c=0; c<fauxpoints.n; c++) {
		if (fauxpoints.e[c].x==x) {
			//DBG cerr <<"*** found match for x:"<<x<<" -> "<<points.e[c].y<<endl;
			return fauxpoints.e[c].y*(ymax-ymin) + ymin;
		}
	}

	 //else check hopefully more thoroughly:
	int hit=bez_intersections(flatpoint(x,0),flatpoint(x,1), 1, fauxpoints.e+1,fauxpoints.n-2, 30, 0,
							  &p,1, NULL,0, NULL);

	double y=0;
	if (hit) y=p.y;
	else {
		DBG if (!hit) cerr << "*** no hit for x=="<<x<<"!!!"<<endl;
	}

	if (y<0) y=0;
	else if (y>1) y=1;

	return y*(ymax-ymin) + ymin;
}

//! Return y value for x. Out of range x and y are clamped to bounds.
/*! This computes directly from points array, assuming that points is actually
 * a list of bezier points, and starts and ends with a vertex (not control handle).
 *
 * Note this is very not efficient.
 */
double CurveInfo::f_bezier(double x)
{
	 //sanity check the points
	while (points.n%3!=1) points.push(points.e[points.n-1]);

	//clamp x:
	if (xmax>xmin) {
		if (x<xmin) x=xmin;
		else if (x>xmax) x=xmax;
	} else {
		if (x>xmin) x=xmin;
		else if (x<xmax) x=xmax;
	}

	x=(x-xmin)/(xmax-xmin); //scale to 0..1

	flatpoint p;
	int hit=bez_intersections(flatpoint(x,0),flatpoint(x,1), 1, points.e,points.n, 20, 0,
							  &p,1, NULL,0, NULL);
	double y=0;
	if (hit) y=p.y;

	if (y<0) y=0;
	else if (y>1) y=1;

	return y*(ymax-ymin) + ymin;
}

/*! Make a sine like curve with max point at minx, min point at maxx.
 * samples needs to be >= 2.
 *
 * Note this makes a CurveInfo::Autosmooth type, simply making sample points
 * on an actual sine curve. It is NOT a minimal bezier representation.
 */
void CurveInfo::SetSinusoidal(int samples)
{
	if (samples<2) return;
	curvetype=CurveInfo::Autosmooth;

	points.flush();
	flatvector p;

	samples--;

	for (int c=0; c<=samples; c++) {
		p.x=((double)c)/samples;
		p.y=.5+.5*cos(M_PI*c/samples);
		points.push(p);
	}
	MakeFakeCurve();
}

//! Fills fauxpoints with c-v-c-c-v-c...
/*! Basically for each point, push handles that are parallel to a line connecting
 * the points on either side.
 *
 * Note this is not a very good way to approximate points when there are a lot of points.
 * It does ok for a very sparse collection of points.
 */
void CurveInfo::MakeFakeCurve()
{
	fauxpoints.flush();

	flatvector v,p, pp,pn, opp,opn;
	double sx;

	if (points.n==0) return;
	if (points.n==1) {
		fauxpoints.push(flatpoint(0,points.e[0].y));
		fauxpoints.push(flatpoint(0,points.e[0].y));
		fauxpoints.push(flatpoint(0,points.e[0].y));
		fauxpoints.push(flatpoint(1,points.e[0].y));
		fauxpoints.push(flatpoint(1,points.e[0].y));
		fauxpoints.push(flatpoint(1,points.e[0].y));
		return;
	}

	for (int c=0; c<points.n; c++) {
		 //find a previous point to work with
		if (c==0) {
			if (wrap) opp=points.e[points.n-1]-flatpoint(1,0);
			else { opp=points.e[0]; opp.x=0; }
		} else opp=points.e[c-1];

		 //find a next point to work with
		if (c==points.n-1) {
			if (wrap) opn=points.e[0]+flatpoint(1,0);
			else { opn=points.e[c]; opn.x=1; }
		} else opn=points.e[c+1];

		v=opn-opp;
		v.normalize();

		p=points.e[c];
		sx=(p.x-opp.x)*.5;
		pp=p - v*sx;
//		if (pp.y>1) {
//			pp=p-v*(1-p.y);
//		} else if (pp.y<0) {
//			pp=p-v*(p.y);
//		}

		sx=(opn.x-p.x)*.5;
		pn=p + v*sx;
//		if (pn.y>1) {
//			pn=p+v*(1-p.y);
//		} else if (pn.y<0) {
//			pn=p+v*(p.y);
//		}

		fauxpoints.push(pp);
		fauxpoints.push(p);
		fauxpoints.push(pn);
	}

	 //add previous and final points if necessary
	if (wrap) {
		int added=0;
		if (fauxpoints.e[1].x>0) {
		    fauxpoints.push(fauxpoints.e[fauxpoints.n-1]-flatpoint(1,0), 0);
		    fauxpoints.push(fauxpoints.e[fauxpoints.n-2]-flatpoint(1,0), 0);
		    fauxpoints.push(fauxpoints.e[fauxpoints.n-3]-flatpoint(1,0), 0);
			added=3;
		}
		if (fauxpoints.e[points.n-2].x<1) {
		    fauxpoints.push(fauxpoints.e[added+0]+flatpoint(1,0));
		    fauxpoints.push(fauxpoints.e[added+1]+flatpoint(1,0));
		    fauxpoints.push(fauxpoints.e[added+2]+flatpoint(1,0));
		}
	} else {
		if (fauxpoints.e[1].x>0) {
			double y=fauxpoints.e[1].y;
			fauxpoints.e[0]=fauxpoints.e[1];
		    fauxpoints.push(flatpoint(0,y), 0);
		    fauxpoints.push(flatpoint(0,y), 0);
		    fauxpoints.push(flatpoint(0,y), 0);
		}
		if (fauxpoints.e[points.n-2].x<1) {
			double y=fauxpoints.e[fauxpoints.n-2].y;
			fauxpoints.e[fauxpoints.n-1]=fauxpoints.e[fauxpoints.n-2];
		    fauxpoints.push(flatpoint(1,y));
		    fauxpoints.push(flatpoint(1,y));
		    fauxpoints.push(flatpoint(1,y));
		}
	}
}

/*! p is a point with x,y in range [0..1].
 * Return a point scaled to [xmin..xmax],[ymin..ymax].
 */
flatpoint CurveInfo::MapUnitPoint(flatpoint p)
{
	p.x=p.x*(xmax-xmin)+xmin;
	p.y=p.y*(ymax-ymin)+ymin;
	return p;
}

/*! p is a point with x,y in range [xmin..xmax],[ymin..ymax].
 * Return a point scaled to [0..1].
 */
flatpoint CurveInfo::MapToUnitPoint(flatpoint p)
{
	p.x=(p.x-xmin)/(xmax-xmin);
	p.y=(p.y-ymin)/(ymax-ymin);
	return p;
}





} // namespace Laxkit


