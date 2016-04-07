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
//    Copyright (C) 2004-2007,2010-2012 by Tom Lechner
//

#include <lax/bezutils.h>
#include <lax/transformmath.h>
#include <lax/drawingdefs.h>


#include <iostream>
using namespace std;
#define DBG 


/*! \file
 * \todo  should probably look into lib2geom
 */


namespace Laxkit {


//------------------------------- Bez utils -------------------------------------------
//! Update a bounding box to include the given bezier segment.
/*! \ingroup math
 * This assumes that p has already been figured in bbox, and bbox is 
 *   already a suitable bbox. That allows easy bounds checking per segment
 *   without checking the endpoints more than once.
 *
 * If extrema is not null, it should be a double[5]. It gets filled with the t parameters
 * corresponding to the extrema of the bez segment (where the tangent to the curve is
 * either vertical or horizontal), where the segment from p to q 
 * corresponds to t=[0,1]. Repeated extrema are only included once.
 *  
 *  Returns the number of extrema (up to 4).
 *  
 * \todo *** something is fishy with quad, occasionally finds way out extreme??
 */
int bez_bbox(flatpoint p,flatpoint c,flatpoint d,flatpoint q,DoubleBBox *bbox,double *extrema)//extrema=NULL
{
	 // check max min for endpoint q, assume p was done in last 
	 // check, or prior to calling here
	bbox->addtobounds(q);

	 // check max/min at extrema
	 // b =                a1 *t^3 +             a2 *t^2 +        a3 *t + p
	 // b = (-p + 3c -3d + q) *t^3 + (3p - 6c + 3d) *t^2 + (-3p + 3c)*t + p
	 // b'=(-3p + 9c -9d + 3q)*t^2 + (6p - 12c + 6d)*t   + (-3p + 3c)
	 // b'=              3*a1 *t^2 +           2*a2 *t   +        a3
	flatpoint a1=-p + 3*c -3*d + q,
			a2=3*p - 6*c + 3*d,
			a3=-3*p + 3*c,
			bp;
	double t1,t;
	int extt=0;
	
	 // ----------Check x extrema
	 // The curve can be constant, linear, quadratic, cubic.
	 // thus, only need to check extrema for quad and cubic
	if (a1.x==0 && a2.x!=0) { // is quadratic, must deal with sep. from cubic because div by 0
		t1=-a3.x/2/a2.x;
		if (t1>=0 && t1<=1) { // found 1 extrema in range
			if (extrema) extrema[extt++]=t1;
			bp=(a2*t1 + a3)*t1 + p; // find the bez point
			DBG cerr <<"x quad ext:"<<t1<<" at:"<<bp.x<<','<<bp.y<<endl;
			bbox->addtobounds(bp);
		}
	} else if (a1.x!=0) { // is full cubic, otherwise is just a straight line
		t=4*a2.x*a2.x - 4*(3*a1.x)*a3.x;  // t=b^2-4*a*c
		//t=p.x*(q.x-d.x)+c.x*c.x+d.x*d.x-c.x*(d.x+q.x); //t=b^2-4*a*c (radical in quadratic)
		if (t>=0) { // one extrema
			t=sqrt(t);
			t1=(-2*a2.x+t)/(2*3*a1.x); // t1= (-b + sqrt(b^2-4*a*c))/2a
			DBG cerr <<"x ext:"<<t1;
			if (t1>=0 && t1<=1) { // found 1 extrema in range
				if (extrema) extrema[extt++]=t1;
				bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
				DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
				bbox->addtobounds(bp);
			}
			DBG else cerr <<endl;
			if (t) { // if is not a double root
				t1=(-2*a2.x-t)/(2*3*a1.x); // t1= (-b - sqrt(b^2-4*a*c))/2a
				DBG cerr <<"x2 ext:"<<t1;
				if (t1>=0 && t1<=1) { // found another x extrema in range
					if (extrema) extrema[extt++]=t1;
					bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
					DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
					bbox->addtobounds(bp);
				}
				DBG else cerr <<endl;
			}
		} // else no extrema
	}

	 // --------Check y extrema
	if (a1.y==0 && a2.y!=0) { // is quadratic, must deal with sep. from cubic because div by 0
		t1=-a3.y/2/a2.y;
		if (t1>=0 && t1<=1) { // found 1 extrema in range
			if (extrema) extrema[extt++]=t1;
			bp=(a2*t1 + a3)*t1 + p; // find the bez point
			DBG cerr <<"y quad ext:"<<t1<<" at:"<<bp.x<<','<<bp.y<<endl;
			bbox->addtobounds(bp);
		}
	} else if (a1.y!=0) { // full cubic
		t=4*a2.y*a2.y - 4*3*a1.y*a3.y;  // t=b^2-4*a*c
		if (t>=0) { // one extrema
			t=sqrt(t);
			t1=(-2*a2.y+t)/(2*3*a1.y); // t1= (-b + sqrt(b^2-4*a*c))/2a
			DBG cerr <<"y ext:"<<t1;
			if (t1>=0 && t1<=1) {
				if (extrema) extrema[extt++]=t1;
				bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
				DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
				bbox->addtobounds(bp);
			}
			DBG else cerr <<endl;
			if (t) {
				t1=(-2*a2.y-t)/(2*3*a1.y); // t1= (-b - sqrt(b^2-4*a*c))/2a
				DBG cerr <<"y2 ext:"<<t1;
				if (t1>=0 && t1<=1) {
					if (extrema) extrema[extt++]=t1;
					bp=((a1*t1 + a2)*t1 + a3)*t1 + p; // find the bez point
					DBG cerr <<" at:"<<bp.x<<','<<bp.y<<endl;
					bbox->addtobounds(bp);
				}
				DBG else cerr <<endl;
			}
		} // else no extrema
	}
	return extt;
}

//! Return the t parament for the point closest to p in the bezier segment p1,c1,c2,p2.
/*! \ingroup math
 *  This merely finds the least distance to p of any point at t=0,1/maxpoints,2/maxpoints,...,1.0.
 * 
 * maxpoints must somehow be chosen so that for each segment [t,t+1/maxpoints], its length
 * is no longer than the distance one is really looking for.
 *
 * If d_ret!=NULL, then return the minimum distance to the path in it.
 */
double bez_closest_point(flatpoint p, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int maxpoints,
		double *d_ret, double *dalong_ret, flatpoint *found)
{
	flatpoint bp,last,laststart;
	double d=1e+10,
		   dd;
	double da=0, dat=0, lastda=0, dastart=0;
	double at_t=1e+10,
		   t,dt,a1,a2,a3,a4;
	double start=0,end=1;
	dt=1/(double)maxpoints;
	last=p1;

	for (int recurse=0; recurse<2; recurse++) {
		for (t=start; t<=end; t+=dt) {
			a1=(1-t)*(1-t)*(1-t);
			a2=3*t*(1-t)*(1-t);
			a3=3*t*t*(1-t);
			a4=t*t*t;
			bp.x=(a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x);
			bp.y=(a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y);
			dd=(bp.x-p.x)*(bp.x-p.x)+(bp.y-p.y)*(bp.y-p.y);//square of dist to p

			if (dd<d) {
				d=dd; at_t=t; laststart=last; dastart=lastda; da=dat; if (found) *found=bp;
			}
			if (dalong_ret) { lastda=dat; dat+=norm(bp-last); last=bp; }
		}

		start=at_t-dt;
		dat=dastart;
		last=laststart;
		if (start<0) start=0;
		end  =at_t+dt;
		if (end>1) end=1;
		dt=(end-start)/maxpoints;
	}

	if (d_ret) *d_ret=d;
	if (dalong_ret) *dalong_ret=da;
	return at_t;
}

//! Return the physical length of the segment, by approximating with npoints.
double bez_segment_length(flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int npoints)
{
	double d=0;
	double dt=1/(double)npoints;
	double lx,ly, x,y;
	double a1,a2,a3,a4;
	for (double t=0; t<=1.0; t+=dt) {
		a1=(1-t)*(1-t)*(1-t);
		a2=3*t*(1-t)*(1-t);
		a3=3*t*t*(1-t);
		a4=t*t*t;
		x=(a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x);
		y=(a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y);

		if (t!=0) d+=sqrt((x-lx)*(x-lx)+(y-ly)*(y-ly));
		lx=x;
		ly=y;
	}
	return d;
}

/*! Subdivide the segment with the famous bezier subdivision by midpoints algorithm.
 * npm is the new point on the path, the other points are associated new control points.
 */
void bez_midpoint(flatpoint p1,flatpoint c1, flatpoint c2, flatpoint p2, 
				flatpoint &nc1, flatpoint &npp, flatpoint &npm, flatpoint &npn, flatpoint &nc2)
{
	nc1.set((p1.x+c1.x)/2,(p1.y+c1.y)/2);
	nc2.set((p2.x+c2.x)/2,(p2.y+c2.y)/2);
	double mx=(c1.x+c2.x)/2, my=(c1.y+c2.y)/2;
	npp.set((mx+nc1.x)/2,(my+nc1.y)/2);
	npn.set((mx+nc2.x)/2,(my+nc2.y)/2);
	npm.set((npp.x+npn.x)/2,(npp.y+npn.y)/2);
}

/*! For when you only need one intersection on one bezier segment.
 * Return 1 for hit found, or 0.
 *
 * This just calls the fuller bez_intersections() with appropriate settings.
 */
int bez_intersection(flatpoint p1,flatpoint p2, int isline,
					flatpoint bp1, flatpoint bc1, flatpoint bc2, flatpoint bp2,
					int resolution, flatpoint *point_ret, double *t_ret)
{
	flatpoint b[4];
	b[0]=bp1;
	b[1]=bc1;
	b[2]=bc2;
	b[3]=bp2;

	int hits=bez_intersections(p1,p2, isline, b,4, resolution, 0, point_ret,1, t_ret,1, NULL);
	if (!hits) *point_ret=flatpoint();

	return hits;
}

/*! Transform points to coordinate system where p1 is the origin, and p2 corresponds
 * to point (1,0). Then it is easy to find intersections through the segment, or
 * through the line going through p1 and p2, since that is wherever the new x axis is crossed.
 * 
 * Assumes points is an array structured as v-c-c-v-c-c...c-c-v. For closed paths, you must ensure
 * the final vertex is the same as the initial. So there should be n/3+1 vertices in the list.
 *
 * This is a kind of primitive approximation, based on sampling resolution number of points per
 * v-c-c-v bezier segment.
 *
 * Return value is number of hits actually parsed. If the whole path was not processed, then
 * endt is assigned the ending t, else it gets 0.
 */
int bez_intersections(flatpoint P1,flatpoint P2, int isline,
					  flatpoint *points, //!< array of v-c-c-v
					  int n,             //!< number of flatpoints in points
					  int resolution,    //!< how many linear segments to begin search for each segment
					  double startt,     //!< offset this many segments before searching
					  flatpoint *points_ret, //!< this must be allocated already
					  int np,            //!< number of points allocated in points_ret, return up to this many hits
					  double *t_ret,     //!< this must be allocated already (optional, can be NULL)
					  int nt,            //!< number of doubles allocated in t_ret
					  double *endt)      //!< t at which searching stopped (hit max of np)
{
	double m[6],mi[6];
	transform_from_basis(m, P1,P2-P1,transpose(P2-P1));
	transform_invert(mi,m);

	int numhits=0;
	flatpoint p1,c1,c2,p2;
	flatpoint t1,t2,v,p;

	double tsofar=0;
	double dt,t,tt,ttt, a1,a2,a3;
	dt=1.0/resolution;

	p1=transform_point(mi,points[0]);
	for (int c=0; c<n-1; p1=p2, c+=3) {
		p2=transform_point(mi,points[c+3]);
		while (startt>=1) { startt-=1; continue; } //resume where we might have left off

		c1=transform_point(mi,points[c+1]);
		c2=transform_point(mi,points[c+2]);

		t1=p1;
		for (t=startt; t<1.0; t+=dt) {
			tt=t*t;
			ttt=tt*t;
			a1=1-3*t+3*tt-  ttt;
			a2=  3*t-6*tt+3*ttt;
			a3=	  3*tt-3*ttt;
			t2.x= a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x;
			t2.y= a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y;

			if ((t1.y>0 && t2.y<=0) || (t1.y<=0 && t2.y>0)) {
				 //found a hit! now interpolate for further approximation of x axis cross.
				 //note that case where t1.y==t2.y==0 (where segment is ON x axis) does not qualify here
				p.y=0;
				p.x=t2.x-t2.y*(t2.x-t1.x)/(t2.y-t1.y);

				if (!isline && (p.x<0 || p.x>1)) ; //not a hit for segment intersect!
				else {
					 //add p to hit list!
					p=transform_point(m,p);//back to original coordinates
					points_ret[numhits]=p;
					if (t_ret) t_ret[numhits]=tsofar+t;
					numhits++;
					if (numhits>=np || (t_ret && numhits>=nt)) {
						 //hit maximum allocated hits
						if (endt) *endt=tsofar+t;
						return numhits;
					}
				}
			}
			t1=t2;
		} //loop over one v-c-c-v segment
		tsofar+=1;
	}

	if (endt) *endt=0;
	return numhits;
}

//! From a physical distance, return the corresponding t parameter value.
/*! Note that this is probably not very reliable for long segments.
 */
double bez_distance_to_t(double dist, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int resolution)
{
	double dd=0, ddd;
	double x,y, lx,ly;
	double t,tt,ttt, a1,a2,a3;
	double dt=1.0/(resolution-1);
	int recurse=0;
	double end=1.0;

	for (t=0; t<=end; t+=dt) {
		tt=t*t;
		ttt=tt*t;
		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;

		x=a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x;
		y=a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y;

		if (t>0) {
			ddd=sqrt((x-lx)*(x-lx)+(y-ly)*(y-ly));
			if (dd+ddd>dist) {
				recurse++;
				if (recurse>1) break;

				end=t;
				t-=dt;
				dt=dt/(resolution-1);
				continue;
			}
			dd+=ddd;
		}
		lx=x;
		ly=y;
	}
	return t;
}

//! From a t parameter, return the corresponding distance value.
/*! Note that this is probably not very reliable for long segments. It only checks
 * against resolution number of straight line segments.
 */
double bez_t_to_distance(double T, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int resolution)
{
	double dd=0;
	double x,y, lx,ly;
	double t,tt,ttt, a1,a2,a3;
	double dt=1.0/(resolution-1);
	int recurse=0;
	double end=1.0;

	for (t=0; t<=end+dt; t+=dt) {
		if (t>=T || t>1) {
			recurse++;
			if (recurse>2) return dd;

			end=t;
			t-=dt;
			dt/=resolution;
			continue;
		}

		tt=t*t;
		ttt=tt*t;
		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;

		x=a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x;
		y=a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y;

		if (t>0) dd+=sqrt((x-lx)*(x-lx)+(y-ly)*(y-ly));

		lx=x;
		ly=y;
	}
	return dd;
}

//! Return the numerical tangent at t.
/*! Note that this is NOT necessarily the visual tangent! If a control point is on the vertex,
 * then the tangent there is the null vector.
 */
flatpoint bez_tangent(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	double tt, a1,a2,a3,a4;
	tt=t*t;
	a1= -3 + 6*t -3*tt;
	a2=  3 -12*t +9*tt;
	a3=      6*t -9*tt;
	a4=           3*tt;

	return (flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x),
					  (a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y)));
}

//! Return the numerical acceleration vector at t.
/*! In other words, the double derivative of the path with respect to t.
 */
flatpoint bez_acceleration(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	double a1,a2,a3,a4;
	a1=   6 -  6*t;
	a2= -12 + 18*t;
	a3=   6 - 18*t;
	a4=        6*t;

	return (flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x),
					  (a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y)));
}

//! Return the visual tangent at t.
/*! If t>0 and t<1, then just return bez_tangent(). Otherwise, approximate a vector with a point just
 * off the path. If t<0 or t>1, then a null vector is returned.
 *
 * Please note the returned vector is not normalized (unit length).
 *
 * \todo this could use L'Hopital's rule, which says if two functions in this case x(t) and y(t) approach
 *   0, then x'(t) and y'(t) are such that x/y=x'/y' when limit x'/y' exists...
 *   
 */
flatpoint bez_visual_tangent(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	if (t>0 && t<1) return bez_tangent(t,p1,c1,c2,p2);

	if (t==0) {
		flatpoint pp=bez_point(.00001, p1,c1,c2,p2);
		return pp-p1;
	}

	if (t==1) {
		flatpoint pp=bez_point(.99999, p1,c1,c2,p2);
		return p2-pp;
	}

	return flatpoint(0,0);
}

///*! Find a bezier subsegment within the given segment. *** better to have subdivide with t[]?
// *
// * points_ret must have room for 4 points: (new first point) - (control 1) - (control 2) - (new final point)
// */
//void bez_subsegment(double t1,double t2, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, flatpoint *points_ret)
//{
//	flatpoint pts[5];
//	bez_subdivide(t1, p1,c1,c2,p2, pts);
//
//
//	bez_subdivide(t1, p1,c1,c2,p2);
//}

//! Cut the bezier segment in two at t.
/*! points_ret must be an already allocated array of 5 points. It is filled with the
 * new found points as follows:
 * <pre>
 *   points_ret[0] = new handle of p1
 *   points_ret[1] = new tonext of new vertex
 *   points_ret[2]= new vertex
 *   points_ret[3]= new toprev of new vertex
 *   points_ret[4]= new handle of p2
 * </pre>
 */
void bez_subdivide(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, flatpoint *points_ret)
{
	flatpoint nv=bez_point(t,p1,c1,c2,p2);
	flatpoint nt=bez_tangent(t,p1,c1,c2,p2);

	points_ret[0]=p1+t*(c1-p1);
	points_ret[1]=nv-t*nt/3;
	points_ret[2]=nv;
	points_ret[3]=nv+(1-t)*nt/3;
	points_ret[4]=p2+(1-t)*(c2-p2);
}

//! Return the cubic bezier point at t. t==0 is p1, t==1 is p2.
/*! \ingroup math
 *  \todo *** make sure this is really optimized!
 */
flatpoint bez_point(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
//	---------
//	double a1,a2,a3;
//	a1=(1-t)*(1-t)*(1-t); //1 - 3*t + 3*t^2 - t^3
//	a2=3*t*(1-t)*(1-t);   //      t - 3*t^2 + t^3
//	a3=3*t*t*(1-t);       //            t^2 - t^3
//	a4=t*t*t;             //                  t^3 
//	return (flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x),
//					  (a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y));
//	---------
	double tt,ttt, a1,a2,a3;
	tt=t*t;
	ttt=tt*t;
	a1=1-3*t+3*tt-  ttt;
	a2=  3*t-6*tt+3*ttt;
	a3=      3*tt-3*ttt;
	return (flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x),
					  (a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y)));
}

//! Break down numsegs bezier segments to a polyline with resolution*numsegs points.
/*! If numsegs==1, then from_points is an array of points: v-c-c-v. Each additional
 * segment means that two control points and another vertex follow (-c-c-v).
 *
 * If to_points==NULL, then return a new flatpoint[numsegs*resolution];
 */
flatpoint *bez_points(flatpoint *to_points,int numsegs,flatpoint *from_points,int resolution)
{
	if (to_points==NULL) to_points=new flatpoint[resolution*numsegs];

	for (int c=0,i=0; c<numsegs; c++,i+=resolution) {
		bez_points(to_points+i,from_points+c*3,resolution,c==0?0:1);
	}
	return to_points;
}

//! Break down numsegs bezier segments to a polyline with resolution*numsegs points.
/*! from_points is a list of bezier vertices and control points. If atend==0, then the
 * array starts with a vertex, and that vertex's previous control point is at the end of 
 * from_points. Otherwise it starts with a control point, then a vertex.
 * numsegs is the number of bezier segments in from_points. Each segment is defined by
 * 4 points. If closed==1, then the final vertex connects to the first vertex (after 2 control
 * points). Note that an array c-v-c is valid, provided that isclosed==1 and atend==0.
 *
 * If to_points==NULL, then return a new flatpoint[numsegs*resolution];
 */
flatpoint *bez_points(flatpoint *to_points,int numsegs,flatpoint *from_points,int resolution,int isclosed,int atend)
{
	cout <<"*** imp bez_points!"<<endl;
	return NULL;
//	if (to_points==NULL) to_points=new flatpoint[resolution*numsegs];
//
//	for (int c=0,i=0; c<numsegs; c++,i+=resolution) {
//		bez_points(to_points+i,from_points+c*3,resolution,c==0?0:1);
//	}
//	return to_points;
}

//! Break down the bezier segment to a polyline with resolution points.
/*! \ingroup math
 * If ignorefirst, do not compute the first point. This allows code to call this repeatedly
 * without calculating vertices twice.
 *
 * If to_points!=NULL, it must have room for resolution number of points.
 * If to_points==NULL, then return a new flatpoint[resolution].
 * In either case, the generated points array is returned.
 * to_points[0] will correspond to v1 (from_points[0]), and
 * to_points[resolution-1] will correspond to v2 (from_points[3]).
 *
 * from_points is an array of the 4 flatpoints of the bezier segment: v1-c1-c2-v2.
 *
 * \todo optimize me
 */
flatpoint *bez_points(flatpoint *to_points,flatpoint *from_points,int resolution,int ignorefirst)
{
	if (to_points==NULL) to_points=new flatpoint[resolution];
	
	double t,tt,ttt, a1,a2,a3, dt;
	dt=1.0/(resolution-1);
	DBG int i=0;
	for (int c=(ignorefirst?1:0); c<resolution; c++) {
		t=dt*c;
		tt=t*t;
		ttt=tt*t;
		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;
		to_points[c]=flatpoint((a1*from_points[0].x + a2*from_points[1].x + a3*from_points[2].x + ttt*from_points[3].x),
						 	   (a1*from_points[0].y + a2*from_points[1].y + a3*from_points[2].y + ttt*from_points[3].y));
		DBG i++;
	}
	//DBG cerr <<"bez_points made "<<i<<" points, res="<<resolution<<endl;
	return to_points;
}

/*! Return a subdivided bezier curve, at the points in T. T must be ordered in increasing order, and
 * each t must be in [0..1].
 *
 * The list returned is (point at 0)-control-control-(point at T[0])-c- ... c-(point at 1).
 * If as_tangents, then each control is a tangent vector at that t.
 * Otherwise, they are locations of bezier control handles.
 *
 * If to_points==NULL, then a new flatpoint[] is returned with 3*n+4 points in it.
 * Otherwise, to_points must be large enough to hold 3*n+4 points.
 */
flatpoint *bez_segments_at_samples(flatpoint *to_points,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2,double *T,int n,int as_tangents)
{
	DBG cerr <<" *** finish this: bez_segments_at_samples!!"<<endl;

	if (to_points==NULL) to_points=new flatpoint[3*n+4];
	
	double t,tt,ttt, a1,a2,a3,a4;
	flatpoint tangent;
	int i=0;

	for (int c=0; c<n; c++) {
		t=T[c];
		tt=t*t;
		ttt=tt*t;

		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;

		to_points[i].x=a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x;
		to_points[i].y=a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y;

		a1= -3 + 6*t -3*tt;
		a2=  3 -12*t +9*tt;
		a3=      6*t -9*tt;
		a4=           3*tt;

		tangent.x=a1*p1.x + a2*c1.x + a3*c2.x + a4*p2.x;
		tangent.y=a1*p1.y + a2*c1.y + a3*c2.y + a4*p2.y;

		if (as_tangents) {
			if (c<=n) to_points[i+1]=tangent;
			if (i>0) to_points[i-1]=-tangent;
		} else {
			if (c<=n) to_points[i+1]=to_points[i]+tangent/3; // *** warning ! does not scale correctly
			if (i>0) to_points[i-1]=to_points[i]-tangent/3;
		}

		i+=3;
	}
	return to_points;
}

/*! Compute points along a bezier segment at the specified t values, of which there are n.
 * If ignorefirst, then don't compute the first value.
 */
flatpoint *bez_points_at_samples(flatpoint *to_points,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2,double *T,int n,int ignorefirst)
{
	if (to_points==NULL) to_points=new flatpoint[n];
	
	double t,tt,ttt, a1,a2,a3;
	for (int c=(ignorefirst?1:0); c<n; c++) {
		t=T[c];
		tt=t*t;
		ttt=tt*t;
		a1=1-3*t+3*tt-  ttt;
		a2=  3*t-6*tt+3*ttt;
		a3=      3*tt-3*ttt;
		to_points[c]=flatpoint((a1*p1.x + a2*c1.x + a3*c2.x + ttt*p2.x),
						 	   (a1*p1.y + a2*c1.y + a3*c2.y + ttt*p2.y));
	}
	return to_points;
}

//! Break down the bezier segment to a polyline with resolution points.
/*! \ingroup math
 * If ignorefirst, do not compute the first point. This allows code to call this repeatedly
 * without calculating vertices twice.
 *
 * If to_points!=NULL, it must have room for resolution number of points.
 * If to_points==NULL, then return a new flatpoint[resolution].
 * In either case, to_points is returned.
 *
 * p1,c1,c2,p2 define the bezier segment. This just puts the points in an array and calls
 * bezpoints(flatpoint *,flatpoint *,int,int).
 */
flatpoint *bez_points(flatpoint *to_points,
					  flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2,int resolution,int ignorefirst)
{
	flatpoint p[4];
	p[0]=p1;
	p[1]=c1;
	p[2]=c2;
	p[3]=p2;
	return bez_points(to_points,p,resolution,ignorefirst);
}

//! Return the distance p is from the bezier curve in points.
/*! \ingroup math
 * points is v-c-c-v-c-c-v-...-v, and n is the number of all points including control points.
 * So (n mod 3) must be 1.
 * 
 * Returns the shortest distance of p to the curve.
 * t_ret gets filled with the t index [0..1] within the segment starting at i_ret.
 * i_ret gets filled with the index in points of the vertex for the segment the point is just after.
 *  If there is no point within 1e+10 units of the curve, then i_ret gets -1 and 1e+10 is returned.
 *
 * This is a very rough approximation. Only checks against maxpoints sample points per segment.
 *
 * \todo I'm sure this could be optimized and improved somehow.
 */
double bez_near_point(flatpoint p,flatpoint *points,int n,
					  int maxpoints,
					  double *t_ret,int *i_ret)
{
	int c,at_c=-1;
	double d=1e+10,dd,
		   tt,t=-1;
	for (c=0; c<n-1; c+=3) {
		tt=bez_closest_point(p,points[c],points[c+1],points[c+2],points[c+3],maxpoints,&dd,NULL,NULL);
		if (dd<d) { d=dd; t=tt; at_c=c; }
	}
	if (i_ret) *i_ret=at_c;
	if (t_ret) *t_ret=t;
	return d;
}

//! Just like bez_near_point() but with a list of pointers to points, rather than directly at points.
/*! \ingroup math
 *
 * This makes it slightly easier to process bezier curves that are not stored as a simple array
 * of flatpoints.
 */
double bez_near_point_p(flatpoint p,flatpoint **points,int n,
					  int maxpoints,
					  double *t_ret,int *i_ret)
{
	int c,at_c=-1;
	double d=1e+10,dd,
		   tt,t=-1;
	for (c=0; c<n; c+=3) {
		tt=bez_closest_point(p,*points[c],*points[c+1],*points[c+2],*points[c+3],maxpoints,&dd,NULL,NULL);
		if (dd<d) { d=dd; t=tt; at_c=c; }
	}
	if (i_ret) *i_ret=at_c;
	if (t_ret) *t_ret=t;
	return d;
}

//! Return the winding number of p relative to the bezier curve in points. 0 means point is inside
/*! \ingroup math
 *  points is assumed to be: c-v-c-c-v-...-v-c, and n must be number of vertices. So there must be
 *  n*3 points in the array.
 *
 * This breaks down the curve into (number of vertices-1)*resolution points, and then finds
 * the winding number for the resulting polyline.
 */
int point_is_in_bez(flatpoint p,flatpoint *points,int n,int resolution)//res=20
{
	flatpoint pnts[resolution*n];
	bez_to_points(pnts,points,n,resolution,1);
	return point_is_in(p,pnts,n*resolution);
}

//! Decompose the bezier curve to a polyline.
/*! There are n vertices, and n*3 points in from_points. The first bezier segment is 
 * from_points[1]-from_points[2],from_points[3],from_points[4]. If closed is nonzero, then assume
 * the final 2 points and the first 2 points of from_points make up the final segment of the curve.
 *
 * to_points is a flatpoint[n*resolution] array. If to_points==NULL, then a new'd array is returned,
 * else be sure that to_points has enough space for n*resolution flatpoints.
 * 
 * If n<2, NULL is returned.
 */
flatpoint *bez_to_points(flatpoint *to_points,flatpoint *from_points,int n,int resolution,int closed)
{
	if (n<2) return NULL;
	if (!to_points) to_points=new flatpoint[resolution*n];
	int c;
	for (c=0; c<n-1; c++) {
		bez_points(to_points+c*resolution,from_points+c*3+1,resolution+1,(c>0));
	}
	 // final segment
	if (closed) bez_points(to_points+c*resolution,
						   from_points[c*3+1],from_points[c*3+2],from_points[0],from_points[1],resolution,1);
	return to_points;
}

//! Return an approximate circle, with numpoints control points, or 4 if numpoints<=1.
/*! Center at (x,y) with radius r.
 *
 * To make 2 vertex points lie \f$\theta\f$ degrees apart on a circle of radius r,
 * then the control rods will have length v:
 * \f[
 * 	v=\frac{4\:r}{3}\:\frac{2\;sin(\theta/2)-sin(\theta)}{1-cos(\theta)};
 * \f]
 *
 * The first point in the returned array is a control point for the second point in the array.
 * So points alternate handle-node-handle - handle-node-handle - ...
 * So there will be numpoints*3 points in the returned array.
 *
 * If points==NULL, then return a new flatpoint[3*numpoints], else it is assumed that pts has
 * at least 3*numpoints allocated.
 */
flatpoint *bez_circle(flatpoint *points, int numpoints, double x,double y,double r)
{
	if (numpoints<=1) numpoints=4;
	int n=numpoints*3;

	if (!points) points=new flatpoint[n];

	double theta=2*3.141592653589/(numpoints); //radians between control points
	double v=4*r*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta)); //length of control handle

	flatpoint center=flatpoint(x,y);
	double xx,yy;
	for (int c=0, i=0; c<numpoints; c++, i+=3) {
		xx=cos(c*theta);
		yy=sin(c*theta);
		points[i+1]=center + flatpoint(r*xx,r*yy);        points[i+1].info=LINE_Vertex;
		points[i  ]=points[i+1] + flatpoint(v*yy,-v*xx);  points[i  ].info=LINE_Bez;
		points[i+2]=points[i+1] + flatpoint(-v*yy,v*xx);  points[i+2].info=LINE_Bez;
	}

	return points;
}

double bez_arc_handle_length(double radius, double theta)
{
	return 4*radius*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta));
}

//! Create an ellipse composed of numsegments bezier segments, or 4 if numsegments<=1.
/*! Start and end in radians. If start==end, then assume a full circle.
 */
flatpoint *bez_ellipse(flatpoint *points, int numsegments,
					   double x,double y,
					   double xr,double yr,
					   flatvector xaxis,flatvector yaxis,
					   double start_angle,double end_angle)
{
	if (numsegments<=1) numsegments=4;
	int n=numsegments*3;

	if (!points) points=new flatpoint[n];

	double theta;
	if (end_angle==start_angle) {
		end_angle=start_angle+2*M_PI;
		theta=(end_angle-start_angle)/(numsegments); //radians between control points
	} else {
		theta=(end_angle-start_angle)/(numsegments-1); //radians between control points
	}
	double v=4*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta)); //length of control handle

	flatpoint center=flatpoint(x,y);
	double mm[6];
	transform_from_basis(mm, center,xr*xaxis,yr*yaxis);

	double xx,yy;
	for (int c=0, i=0; c<numsegments; c++, i+=3) {
		 //first find for unit circle
		xx=cos(start_angle+c*theta);
		yy=sin(start_angle+c*theta);

		points[i+1]=flatpoint(xx,yy); //the vertex
		points[i  ]=points[i+1] + flatpoint(v*yy,-v*xx); //previous control
		points[i+2]=points[i+1] + flatpoint(-v*yy,v*xx); //next control

		 //now map to ellipse
		points[i  ]=transform_point(mm,points[i  ]);
		points[i+1]=transform_point(mm,points[i+1]);
		points[i+2]=transform_point(mm,points[i+2]);

		points[i  ].info=LINE_Bez;
		points[i+1].info=LINE_Vertex;
		points[i+2].info=LINE_Bez;
	}

	return points;
}



 /*! Based on Tavmjong Bah's very clear explanation for curvature for
 * extrapolated joins here: http://tavmjong.free.fr/SVG/LINEJOIN/index.html).
 *
 * k= (v x v')/norm(v)^3
 *
 * If the curvature is infinite (p2-c3==0), then an arbitrarily large
 * number is returned (in this case 1e+15), though with the correct sign;
 */
double curvature_at_t(double t, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	flatvector v=bez_tangent(t,p1,c1,c2,p2);
	flatvector a=bez_acceleration(t,p1,c1,c2,p2);
	double vn=norm(v);
	double k=v.cross(a);

	if (vn==0) { if (k>0) return 1e+15; else return -1e+15; }

	return k/(vn*vn*vn);
}

/*! Return the curvature of the curve at p2. This is 1/r, where r is the radius of
 * a circle with the same curvature. The side of the line the circle is on is determined
 * by the sign of the returned value.
 *
 * The curvature at the endpoint is: 
 * \f[
 *   \kappa(1) = {2\over3}{(p_2-c_2)\times((c_1-c_2)+(p_2-c_2))\over|p_2-c_2|^3}
 * \f]
 *
 * (please read Tavmjong Bah's very clear explanation for curvature for
 * extrapolated joins here: http://tavmjong.free.fr/SVG/LINEJOIN/index.html)
 *
 * If the curvature is infinite (p2-c3==0), then an arbitrarily large
 * number is returned (in this case 1e+15), though with the correct sign;
 */
double end_curvature(flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	flatvector v=p2-c2;
	double vvv=norm(v);
	double k=2./3*v.cross(c1-c2 + v);
	if (vvv==0) { if (k>0) return 1e+15; else return -1e+15; }
	vvv=vvv*vvv*vvv;
	return k/vvv;
}


/*! Determine the order along a circle of p1,p2,p3, starting at p1.
 * int2 and int3 will either be 1 or 2.
 *
 * ang1 is angle to first point, ang2 angle to further point.
 */
void order(flatpoint o, flatpoint p1, flatpoint p2, flatpoint p3,
		int &int2, int &int3,
		double &ang1, double &ang2)
{
	double a1,a2,a3;
	a1=angle_full(p1-o, flatpoint(1,0));
	a2=angle_full(p2-o, flatpoint(1,0));
	a3=angle_full(p3-o, flatpoint(1,0));

	if (a2<a1) a2+=2*M_PI;
	if (a3<a1) a3+=2*M_PI;
	if (a2<a3) { int2=1; int3=2; ang1=a2-a1; ang2=a3-a1; }
	else { int2=2; int3=1; ang1=a3-a1; ang2=a2-a1; }
}

/*! Intersect the line defined by p, pointing parallel to v, with the circle
 * centered at o, with radius r.
 * The number of intersections is returned. If there are no intersections,
 * p1 and p2 are unchanged. If only one intersection, p1 and p2 are set to it.
 * Else p1 and p2 are set to the intersection.
 */
int circle_line_intersection(flatpoint o, double r, flatpoint p, flatpoint v, flatpoint &p1, flatpoint &p2)
{
	flatpoint vv=transpose(v);
	vv.normalize();
	double d=vv*(p-o);
	if (fabs(d)>r) return 0;
	if (fabs(d)==r) {
		p1=p2=o+vv*d;
		return 1;
	}
	p=o+r*vv;
	v.normalize();
	d=sqrt(r*r-d*d);
	p1=p+d*v;
	p2=p-d*v;

	return 2;
}

/*! If there are no intersections, and circle 1 is inside the other, return -1.
 * If no intersections, and circle 2 is inside the other, return -2.
 * If the circles are the same circle, then return -3.
 * If no intersections, and the circles do not overlap at all, 0 is returned.
 *
 * Returns the number of intersections, and p1 and p2 get set with them (if any).
 *
 */
int circle_circle_intersection(flatpoint o1, double r1, flatpoint o2, double r2, flatpoint &p1, flatpoint &p2)
{
	flatpoint v=o1-o2;
	double d=norm(v);
	if (d==0 && r1==r2) return -3; //same circle
	if (d+r2<r1) return -2; //circle 2 is inside the other
	if (d+r1<r2) return -1; //circle 1 is inside the other
	if (d>r1+r2) return 0;

	v/=d;
	double d1=((r1*r1-r2*r2)/d + d)/2;
	flatpoint pp=o1+d1*v;
	v=transpose(v);
	double x=sqrt(r1*r1-d1*d1);
	p1=pp+x*v;
	p2=pp-x*v;
	if (d==r1+r2) return 1;
	return 2;
}

/*! If ret==NULL, then return a NULL flatpoint[].
 * Otherwise put in ret. If so, ret should have room for at least 7 points.
 * 
 * Points returned are either vertex points or bez control points.
 * If control points, they will have LINE_Bez in their info.
 * The returned points are always new new points, not any of the original points.
 * They would be inserted directly between ap2 and bp1.
 */
flatpoint *join_paths(int jointype, double miterlimit,
			flatpoint ap1,flatpoint ac1,flatpoint ac2,flatpoint ap2,
			flatpoint bp1,flatpoint bc1,flatpoint bc2,flatpoint bp2,
			int *n, flatpoint *ret)
{
	if (jointype==LAXJOIN_Extrapolate) {
		DBG cerr <<"join extrapolate..."<<endl;
		//adds at most 5 points
	
		 //large k means very small circle, also could mean control point coincident with the vertex
		 //small k means very large circle, thus like a straight line.
		double k1=end_curvature(ap1,ac1,ac2,ap2);
		double k2=end_curvature(bp2,bc2,bc1,bp1);
		if (fabs(k1)<1e-10) k1=0;
		if (fabs(k2)<1e-10) k2=0;

		if (k1==0 && k2==0) {
			jointype=LAXJOIN_Miter; //2 lines

		} else {
			 //first figure out curvature circles
			flatpoint o1=ap2, o2=bp1;
			double r1=0,r2=0;
			flatpoint v;

			if (k1!=0) {
				v=transpose(ap2-ac2);
				r1=1/k1;
				v*=r1/norm(v);
				o1+=v;
				v=transpose(v);
			} else {
				 //k1 is flat line
				r1=0;
			}

			if (k2!=0) {
				v=transpose(bp1-bc1);
				r2=1/k2;
				v*=r2/norm(v);
				o2+=v;
				v=transpose(v);
			} else {
				 //k2 is flat line
				r2=0;
			}

			 //next find intersections, if any
			if (r1==0 && r2==0) {
				 //2 lines
				DBG cerr <<" --- extrapolate: two lines"<<endl;
				jointype=LAXJOIN_Miter;

			} else if (r1==0 || r2==0) {
				DBG cerr <<" --- extrapolate: one circle, one line"<<endl;

				 //one line, one circle
				flatpoint p,o;
				double r;
				if (r1==0) { r=fabs(r2); o=o2; p=ap2; }
				else       { r=fabs(r1); o=o1; p=bp1; }

				flatpoint p1,p2;
				int status=circle_line_intersection(o, r, p, v, p1,p2);

				if (status==0) {
					 //circle does not intersect line
					double vv=r*4./3*(sqrt(2)-1);
					if (!ret) ret=new flatpoint[2];
					flatpoint p,p2;
					if (r1==0) {
						v=bp1-bc1;
						v*=vv/norm(v);
						p2=bp1+v;
						p=ap2;
					} else {
						v=ap2-ac2;
						v*=vv/norm(v);
						p=ap2+v;
						p2=bp1;
					}
					p .info|=LINE_Bez;
					p2.info|=LINE_Bez;

					if (!ret) ret=new flatpoint[2];
					ret[0]=p;
					ret[1]=p2;
					*n=2;
					return ret;
				} //if circle doesn't intersect

				 //else circle does intersect a
				int pos1, pos2;
				double ang1, ang2;
				order(o, r1==0 ? bp1 : ap2,  p1, p2,  pos1, pos2, ang1, ang2);
				//***

			} else {

				 //2 circles
				r1=fabs(r1);
				r2=fabs(r2);
				flatpoint p1,p2;
				int status=circle_circle_intersection(o1,r1, o2,r2, p1, p2);

				if (status==-3) {
					 //miraculously has same circle
					//***
					DBG cerr <<" --- extrapolate: Same circle"<<endl;

				} else if (status==-2 || status==-1) {
					 //one circle is inside the other
					//***
					DBG cerr <<" --- extrapolate: One circle inside the other"<<endl;

				} else if (status==0) {
					 //circles don't touch
					//***
					DBG cerr <<" --- extrapolate: Circles don't touch"<<endl;

				} else {
					 //has intersections
					DBG cerr <<" --- extrapolate: 2 intersections"<<endl;

					int pos11=0, pos12=0, pos21=0, pos22=0;
					double a11, a12, b11, b12;
					order(o1, ap2,  p1, p2,  pos11, pos12, a11, a12);
					order(o2, bp1,  p1, p2,  pos21, pos22, b11, b12);

					flatpoint i;
					double a;

					v=ap2-o1;
					double radius=norm(v);
					if (pos11==1 && clockwise(v,ap2-ac2)) { i=p1; a=a11; } else { i=p2; a=a12; }
					double len=radius*bez_arc_handle_length(1, a);

					 // *** choose particular intersection
					if (!ret) ret=new flatpoint[5];
					v=ap2-ac2;
					v.normalize();
					ret[0]=ap2+v*len;
					ret[0].info|=LINE_Bez;

					ret[1]=ret[2]=ret[3]=i;
					ret[1].info|=LINE_Bez;
					ret[3].info|=LINE_Bez; 

					v=bp1-o2;
					radius=norm(v);
					if (pos21==1) { i=p1; a=a11; } else { i=p2; a=a12; }
					len=radius*bez_arc_handle_length(1, a);
					v=bp1-bc1;
					v.normalize();
					ret[4]=bp1+v*len;
					ret[4].info|=LINE_Bez;
					*n=5;
					return ret;
				}
			} //if 2 circles

			//****
			jointype=LAXJOIN_Round;
		} //if at least one circle
	}//if extrapolate


	if (jointype==LAXJOIN_Round) {
		DBG cerr <<"join round..."<<endl;

		//adds at most 5 points
		flatline l1(ap2, 2*ap2-ac2);
		flatline l2(bp1, 2*bp1-bc1);

		if (l1.v.isZero()) l1.v= bez_visual_tangent(1,ap1,ac1,ac2,ap2);
		if (l2.v.isZero()) l2.v=-bez_visual_tangent(0,bp1,bc1,bc2,bp2);

		flatpoint p;
		double index1,index2;
		int status=intersection(l1,l2, &p, &index1,&index2);
		DBG cerr <<"line intersection status: "<<status<<endl;

		if (status==2 || status==-2) {
			//is same path, just connect points
			jointype=LAXJOIN_Bevel;

		} else if (status==1 || status==-1) {
			 //parallel lines
			double r=4./3*distance(l2.p,l1)/2;

			if (!ret) ret=new flatpoint[2];
			*n=2;

			l1.v.normalize();
			l2.v.normalize();
			ret[0]=ap2+4/3*r*l1.v; ret[0].info=LINE_Bez;
			ret[1]=bp1+4/3*r*l2.v; ret[1].info=LINE_Bez;
			return ret;

		} else {  //if (status==0)
			double ang=angle_full(l1.v,l2.v,0);
			if (ang<0) ang=-ang;
			if (ang>M_PI/2) ang=M_PI-ang;
			ang=M_PI-ang;
			double vr=4./3*(2*sin(ang/2)-sin(ang))/(1-cos(ang)); //goes to infinity as ang -> 0


			ang=M_PI-ang;
			//DBG cerr <<" angle: "<<ang/M_PI*180<<"  i1: "<<index1<<"  i2:"<<index2<<"  vr: "<<vr;
			double r=tan(ang/2)*ap2.distanceTo(p);
			//DBG cerr <<"  r1="<<r*vr;
			l1.v*=fabs(vr*r)/norm(l1.v);
			r=tan(ang/2)*bp1.distanceTo(p);
			DBG cerr <<"  r2="<<r*vr<<endl;
			l2.v*=fabs(vr*r)/norm(l2.v);

			if (!ret) ret=new flatpoint[2];
			ret[0]=ap2+l1.v; ret[0].info=LINE_Bez;
			ret[1]=bp1+l2.v; ret[1].info=LINE_Bez;
			*n=2;
			return ret;
		}
		
	}

	if (jointype==LAXJOIN_Miter) {
		DBG cerr <<"join miter..."<<endl;

		flatline l1(ap2, 2*ap2-ac2);
		flatline l2(bp1, 2*bp1-bc1);

		if (l1.v.isZero()) l1.v= bez_visual_tangent(1,ap1,ac1,ac2,ap2);
		if (l2.v.isZero()) l2.v=-bez_visual_tangent(0,bp1,bc1,bc2,bp2);

		flatpoint p;
		double index1,index2;
		int status=intersection(l1,l2, &p, &index1,&index2);
		cerr <<"status: "<<status<<endl;

		if (status==0) {
			if (index1>0 && index2>0) {
				double d1=p.distanceTo(ap2);
				double d2=p.distanceTo(bp1);
				if (fabs(d1)>miterlimit && fabs(d2)>miterlimit) {
					status=1;

				} else {
					if (!ret) ret=new flatpoint[1];
					ret[0]=p;
					ret[0].info|=LINE_Corner;
					*n=1;
					return ret;
				}
			}
			//else is a backwards intersection, use jointype=LAXJOIN_Bevel;

		}
		
		if (status==1) {
			 //parallel lines, use miter limit
			if (!ret) ret=new flatpoint[2];
			*n=2;
			ret[0]=ap2+l1.v/norm(l1.v)*miterlimit; ret[0].info|=LINE_Corner;
			ret[1]=bp1+l2.v/norm(l2.v)*miterlimit; ret[1].info|=LINE_Corner;
			return ret;
		}
		
		jointype=LAXJOIN_Bevel;
	}

	if (jointype==LAXJOIN_Bevel) {
		DBG cerr <<"join bevel..."<<endl;
		 //no extra points to add!
		*n=0;
		return NULL;
	}

	*n=0;
	return NULL;
}


/*! This will return in result a list with 3*n points, arranged c-v-c - c-v-c ... c-v-c.
 * If result==NULL, then return a new flatpoint[3*numpoints].
 *
 * This is a primitive approximation, where each point gets control points added before
 * and after, such that the control rods are of length 1/3 the distance between the point
 * and the next point, and the rods are parallel to the line connecting the previous
 * and next point. End points behave as if control handle is at the end point.
 *
 * Points that have info with LINE_Corner, will stay corners, by having bez handles of zero length.
 * New bezier handles will get LINE_Bez in their info.
 *
 * This will call bez_from_points(flatpoint *result, flatpoint *points, int start, int numpoints, bool isclosed)
 * with the appropriate things set.
 */
flatpoint *bez_from_points(flatpoint *result, flatpoint *points, int numpoints)
{
	//return bez_from_points(result, points, 0,numpoints, points[numpoints-1].info&LINE_Closed);
	return bez_from_points(result, points, numpoints, 0,numpoints);
}


/*! This will return in result a list with 3*n points, arranged c-v-c - c-v-c ... c-v-c.
 * If result==NULL, then return a new flatpoint[3*numpoints].
 *
 * This is a primitive approximation, where each point gets control points added before
 * and after, such that the control rods are of length 1/3 the distance between the point
 * and the next point, and the rods are parallel to the line connecting the previous
 * and next point. End points behave as if control handle is at the end point.
 *
 * Points that have info with LINE_Corner, will stay corners, by having bez handles of zero length.
 * New bezier handles will get LINE_Bez in their info.
 *
 * points[start] is assumed to be the start of a segment to be approximated within a larger points array.
 * This means that when start!=0, then it is the same as if that point is the start of an open path,
 * BUT the segment MAY wrap around to points[0] if isclosed is true.
 *
 * If the final point has LINE_Closed, then wrap around to points[0]. If start!=0 in this case, then
 * ONLY the segment from start to start+numpoints is approximated, NOT any continuation at
 * points[0]. Points are grabbed from points[0] only to approximate the later points. When start!=0,
 * the segment from points[0]..points[start-1] would have to be computed separately.
 */
flatpoint *bez_from_points(flatpoint *result, flatpoint *points, int totalpoints, int start, int numpoints)
{
	if (!result) result=new flatpoint[3*numpoints];

	// There are surely better ways to do this. Not sure how powerstroke does it.
	// It is not simplied/optimized at all. Each point gets control points to smooth it out.
	//
	// tangents at points are || to (p+1)-(p-1).
	// Lengths of control rods are 1/3 of distance to adjacent points

    flatvector v,p, pp,pn;
	flatvector opn, opp;
    double sx;
	int i=0;
	bool isclosed=((points[totalpoints-1].info&LINE_Closed) && (points[totalpoints-1].info&(LINE_Join|LINE_Cap))==0) ;
	//bool isclosed=(points[totalpoints-1].info&LINE_Closed);
	//int lastwascorner=0;
	
    for (int c=start; c<start+numpoints; c++) {
        p=points[c];


		if (p.info&LINE_Corner) opp=p;
		else if (c==start) {
			if (isclosed && start==0) opp=points[totalpoints-1];
			else opp=p;
		} else opp=points[c-1];

		if (p.info&LINE_Corner) opn=p;
		else if (c==start+numpoints-1) {
			if (isclosed && c==totalpoints-1) opn=points[0];
			else opn=p;
		} else opn=points[c+1];

        v=opn-opp;
        v.normalize();

        sx=norm(p-opp)*.333;
        result[i]=p - v*sx;
		result[i].info|=LINE_Bez;

		result[i+1]=p;
		if (points[c].info&LINE_Corner) result[i+1].info|=LINE_Corner;

        sx=norm(opn-p)*.333;
        result[i+2]=p + v*sx;
		result[i+2].info|=LINE_Bez;

		i+=3;

		//if (p.info&LINE_Corner) lastwascorner=1; else lastwascorner=0;
    }

	return result;
}

//! Marks any points it thinks should be in the line with flag=1.
static void reduce_polyline_recurse(flatpoint *result, int &ii, flatpoint *points,  int start, int end, double epsilon)
{
    if (end<=start+1) return;

    flatvector v=points[end] - points[start];
	if (v.isZero()) { v.x=1; v.y=0; }
    flatvector vt=transpose(v);
    vt.normalize();

	 //find maximum distance away from line between start and end
    int i=-1; //will be the index of point furthest off the line
    double d=0, dd;
    for (int c=start+1; c<end; c++) {
        dd=fabs((points[c] - points[start])*vt);
        if (dd>d) { d=dd; i=c; }
    }

    if (d<epsilon) {
		 //all in between points were near line, we can ignore them
		DBG cerr <<"reducing "<<start+1<<" to "<<end-1<<endl;
    } else {
        reduce_polyline_recurse(result,ii,  points, start,i, epsilon);
		if (points[i]!=result[ii-1]) result[ii++]=points[i];
        reduce_polyline_recurse(result,ii,  points, i,end,   epsilon);
		if (points[end]!=result[ii-1]) result[ii++]=points[end];
    }
}

/*! For a collection of points, reduce to the simplest polyline, such that any
 * points removed are within epsilon distance of the polyline connecting the
 * remaining points.
 *
 * Returns the number of points in the reduced polyline.
 * result should be allocated to hold n points for the worst case scenario.
 *
 * It is safe to have result==points, since this function will reduce in an orderly
 * manner.
 */
int reduce_polyline(flatpoint *result, flatpoint *points, int n, double epsilon)
{
	result[0]=points[0];
	int ii=1;
    reduce_polyline_recurse(result, ii, points, 0,n-1, epsilon);
	if (points[n-1]!=result[ii-1]) result[ii++]=points[n-1];

    return ii;
}



} // namespace Laxkit

