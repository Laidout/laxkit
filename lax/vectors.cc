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
//	  Copyright (C) 2004-2009,2012-2013 by Tom Lechner
//


#include <cmath>
#include <cstdlib>

#include <lax/vectors.h>

//for SvgToFlatpoints
#include <lax/attributes.h>
#include <lax/doublebbox.h>
#include <lax/laxdefs.h>

//template implementation:
#include <lax/lists.cc>

#define DBG
#include <iostream>
//using namespace std;


/*! \defgroup vectors Various functions for dealing with math vectors
 *	This group defines lots of functions and classes that are handy in dealing with
 *	2 and 3 dimensional vectors. Basically, this means adding and  subtracting
 *	vectors, cross products, dot products, bases, lines, planes, and intersections
 *	among these things.
 * 
 *  \#include <lax/vectors.h>
 *
 * @{
 */

static int vectorop_error=0;

int vector_error()
{
	return vectorop_error;
}

//---------------------------------- spacevector ---------------------------------
/*! \class spacevector
 * \brief 3 Dimensional vector (x,y,z)
 */

//! Make a unit vector, preserving angle. If null vector, do nothing.
void spacevector::normalize()
{
	if (x==0 && y==0 && z==0) return;

	double d=sqrt(x*x+y*y+z*z);
	x/=d;
	y/=d;
	z/=d;
}

void spacevector::setLength(double l)
{
	double d = norm();
	if (d == 0) return;
	x *= l/d;
	y *= l/d;
}

//! nonzero if x==x, y==y and z==z
int operator==(spacevector v1,spacevector v2)
{
	return v1.x==v2.x && v1.y==v2.y && v1.z==v2.z;
}

//! nonzero if x!=x || y!=y || z!=z
int operator!=(spacevector v1,spacevector v2)
{
	return v1.x!=v2.x || v1.y!=v2.y || v1.z!=v2.z;
}

//! (a,b,c)+(d,e,f)=(a+d, b+e, c+f)
spacevector operator+(spacevector a,spacevector b)
{
	return(spacevector(a.x+b.x, a.y+b.y, a.z+b.z));
}

//! v+=(a,b,c)  => (v.x+a, v.y+b, v.z+c)
spacevector operator+=(spacevector &a,spacevector b)
{
	return a=a+b;
}

//! (a,b,c)-(d,e,f)=(a-d, b-e, c-f)
spacevector operator-(spacevector a,spacevector b)
{
	return(spacevector(a.x-b.x, a.y-b.y, a.z-b.z));
}

//! v-=(a,b,c)  => (v.x-a, v.y-b, v.z-c)
spacevector operator-=(spacevector &a,spacevector b)
{
	return a=a-b;
}

//! -v  => (-v.x, -v.y, -v.z)
spacevector spacevector::operator-()
{
	return(spacevector(-x,-y,-z));
}

//! r*(x,y,z)=(r*x, r*y, r*z)
spacevector operator*(double r, spacevector a)  /*const times spacevector */
{
	return(spacevector(r*a.x, r*a.y, r*a.z));
}

//! (x,y,z)*r=(r*x, r*y, r*z)
spacevector operator*(spacevector a, double r)  /*const times spacevector */
{
	return(spacevector(r*a.x, r*a.y, r*a.z));
}

//! v*=r does v=v*r
spacevector operator*=(spacevector &a, double r)  /*const times spacevector */
{
	return a=a*r;
}

//! Cross product A x B
/*! Note the amazing math fact that: ||A x B|| = ||A|| ||B|| sin(theta),
 * where theta is the angle between the two. The cross product direction
 * can be determined with the right hand rule. Put you fingers pointing
 * in the a direction. Now curl your fingers in the direction of the
 * b direction, and stick your thumb away from your fingers.
 * Your thumb now points in the direction of A x B.
 *
 * All this means that you can determine whether an ange is clockwise
 * or counterclockwise by examining the direction of the cross product.
 */
spacevector operator/(spacevector a,spacevector b)  /* cross product */
{
	return(spacevector(a.y*b.z-a.z*b.y,
			a.z*b.x-a.x*b.z,
			a.x*b.y-a.y*b.x));
}					

//! Cross product, A = A x B
spacevector operator/=(spacevector &a, spacevector &b)
{
	return a=a/b;
}

//! v/r= (v.x/r, v.y/r, v.z/r)
spacevector operator/(spacevector a,double r)  /* divide y double */
{
	if (r==0)
	{
//printf("Div y 0 in v-a/f-b.");
		vectorop_error=1;
		return(a);
	} else return(spacevector(a.x/r, a.y/r, a.z/r));
}

//! Does v=v/r
spacevector operator/=(spacevector &v,double r)
{
	return v=v/r;
}

//! Dot product (a,b,c)*(d,e,f) =  a*d + b*e + c*f
double operator*(spacevector a,spacevector b)  /*dot product */
{
	return(a.x*b.x+a.y*b.y+a.z*b.z);
}

//! a||b = the part of a that is parallel to b
spacevector operator||(spacevector a, spacevector b) /*gives part of a parallel to b */
{
	return(((a*b)/(b*b))*b);
}

//! a|=b = the part of a that is perpendicular to b
spacevector operator|=(spacevector a, spacevector b) /*gives part of a perp. to b */
{
	spacevector c=a||b;
	return(a-c);
}

//! Nonzero if any fields of a are nonzero.
int isvector(spacevector a)
{
	return(!(a.x==0 && a.y==0 && a.z==0));
}

//! Nonzero if all fields of a are zero
int isnotvector(spacevector a)
{
	return(a.x==0 && a.y==0 && a.z==0);
}

//! True (nonzero) if v1 and v2 are parallel.
int areparallel(spacevector v1, spacevector v2)
{
	spacevector a=(v1/v2);
	return isnotvector(a);
}

//! The norm of p1=sqrt(p1*p1).
double norm(spacevector p1)
{
	return(sqrt(p1*p1));
}

//! The square of the norm, (p1*p1).
double norm2(spacevector p1)
{
	return p1*p1;
}

/*! Assume a right handed coordinate system (positive x is to the right, positive y is
 * up). Return true if v extending from p points clockwise around the origin. Else false.
 *
 * Found from the sign of the z component of the cross product,
 * thus, return (p.cross(p+v)<=0).
 */
bool clockwise(flatvector p, flatvector v)
{
	return p.cross(p+v)<=0;
}

/*! Return which of north ((0,1) == LAX_TOP), south((0,-1) == LAX_BOTTOM), east((1,0) == LAX_RIGHT),
 * or west((-1,0) == LAX_LEFT) a vector most points in the direction of.
 */
int cardinal_direction(flatvector v)
{
	if (v.x > 0) {
		if (v.x > fabs(v.y)) return LAX_RIGHT;
		if (v.y > 0) return LAX_TOP;
		return LAX_BOTTOM;
	}
	if (-v.x > fabs(v.y)) return LAX_LEFT;
	if (v.y > 0) return LAX_TOP;
	return LAX_BOTTOM;
}

//! Return the winding number for p in the polyline points.
/*! Automatically closes points.
 */
int point_is_in(flatpoint p,flatpoint *points, int n)
{
	int w=0;
	flatpoint t1,t2,v;
	t1=points[0]-p;
	double tyx;
	for (int c=1; c<=n; t1=t2, c++) {
		if (c==n) t2=points[0]-p;
			else t2=points[c]-p;
		if (t1.x<0 && t2.x<0) continue;
		if (t1.x>=0 && t2.x>=0) {
			if (t1.y>0 && t2.y<=0) { w++; continue; }
			else if (t1.y<=0 && t2.y>0) { w--; continue; }
		}
		if (!((t1.y>0 && t2.y<=0) || (t1.y<=0 && t2.y>0))) continue;
		v=t2-t1;
		tyx=t1.y/t1.x;
		if (t1.x<=0) { // note that this block looks identical to next block
			if (t1.y>0) { //-+ to +-
				if (v.y/v.x>=tyx) w++;
				continue;
			} else { //-- to ++
				if (v.y/v.x<tyx) w--;
				continue;
			}
		} else {
			if (t1.y>0) { //++ to --
				if (v.y/v.x>=tyx) w++;
				continue;
			} else { //+- to -+
				if (v.y/v.x<tyx) w--;
				continue;
			}
		}
	}
	return w;
}

typedef spacevector spacepoint;

//---------------------------------- flatvector ---------------------------------
/*! \class flatvector
 * \brief 2 Dimensional vector (x,y)
 */
/*! \var flatvector::info
 * This is used by some functions to store the type of point this represents, for
 * instance by points returned from draw_thing_coordinates(), or points drawn by
 * Displayer::drawFormattedPoints().
 *
 * Namely, see PointInfoTags.
 */

//! Make a unit vector, preserving angle. If null vector, do nothing.
void flatvector::normalize()
{
	if (x==0 && y==0) return;

	double d=sqrt(x*x+y*y);
	x/=d;
	y/=d;
}

void flatvector::setLength(double l)
{
	double d = norm();
	if (d == 0) return;
	x *= l/d;
	y *= l/d;
}

//! v=-v
flatvector flatvector::operator-()
{
	return flatvector(-x,-y); 
}

/*! Return which axis 'x' or 'y' v most closely aligns with, or 0 if neither.
 */
char NearestAxis(flatvector v)
{
	if (v.norm2() < 1e-6) return 0;
	return fabs(v.y) > fabs(v.x) ? 'y' : 'x';
}

//! (x,y) --> (-y,x)
flatvector transpose(flatvector v)
{
	return flatvector((-1)*v.y,v.x);
}

/*! Return how aligned we are with another vector, as if on a path.
 * Two vectors going in the same direction are considered a corner.
 * LINE_Corner for corner. LINE_Smooth for smooth but unequal, LINE_Equal for smooth and equal.
 */
int flatvector::SmoothnessFlag(const flatvector &v) const
{
	double n1 = norm();
	double n2 = v.norm();
	if (*this * v > -.999999 * n1*n2) return LINE_Corner;
	if (fabs(n1-n2) < 1e-6) return LINE_Equal;
	return LINE_Smooth;
}

//! True if x==x and y==y
int operator==(flatvector v1, flatvector v2)
{
	return v1.x==v2.x && v1.y==v2.y;
}

//! nonzero if x!=x || y!=y
int operator!=(flatvector v1, flatvector v2)
{
	return v1.x!=v2.x || v1.y!=v2.y;
}

//! Vector addition.
flatvector operator+(flatvector a,flatvector b)
{
	return(flatvector(a.x+b.x, a.y+b.y));
}

//! Vector addition
flatvector operator+=(flatvector &a,flatvector b)
{
	return a=a+b;
}

//! Vector subtraction.
flatvector operator-(flatvector a,flatvector b)
{
	return(flatvector(a.x-b.x, a.y-b.y));
}

//! Vector subtraction
flatvector operator-=(flatvector &a,flatvector b)
{
	return a=a-b;
}

//! r*v
flatvector operator*(double r, flatvector a)  /*const times flatvector */
{
	return(flatvector(r*a.x, r*a.y));
}

//! v*r
flatvector operator*(flatvector a, double r)  /*const times flatvector */
{
	return(flatvector(r*a.x, r*a.y));
}

//! does v=v*r
flatvector operator*=(flatvector &a, double r)  /*const times flatvector */
{
	return a=a*r;
}

//! (x,y)/r=(x/r,y/r)
flatvector operator/(flatvector a,double r)  /*flatvector by const */
{
	if (r==0)
	{
//printf("Div by 0 in fv-x/y.");
		vectorop_error=1;
		return(a);
	} else return(flatvector(a.x/r,a.y/r));
}

//! does v=v/r
flatvector operator/=(flatvector &a,double r)  /*flatvector by const */
{
	return a=a/r;
}

//! Dot product:  a.x*b.x + a.y*b.y
double operator*(flatvector a,flatvector b)  /*dot product */
{
	return(a.x*b.x+a.y*b.y);
}

//! Norm of a parallel to b (which is a*b/norm(b))
double distparallel(flatvector a, flatvector b) /*gives part of a parallel to b*/
{
	return(a*b/sqrt(b*b));
}

//! Part of a parallel to b (which is a*b/(b*b)*b)
flatvector operator||(flatvector a, flatvector b) /*gives part of  */
{								   /*x parallel to y*/
	return(a*b/(b*b)*b);
}

//! Part of a perpendicular to b
flatvector operator|=(flatvector a, flatvector b) /*gives part of x*/
{  						  /*perp to y	*/
	flatvector c=a||b;
	return(a-c);
}

//! True if any field of v is nonzero
int isnotvector(flatvector v)
{
	return(v.x==0 && v.y==0);
}

//! Return 0 if v1 and v2 not parallel. 1 if parallel in same direction. -1 if parallel in opposite direction.
int areparallel(flatvector v1, flatvector v2)
{
	if (v1.x*v2.y!=v2.x*v1.y) return 0;
	if (v1.x==0) {
		if (v1.y>0 && v2.y>0) return 1;
	} else if (v1.x>0 && v2.x>0) return 1;
	return -1;
}

//! Norm of v.
double norm(flatvector v)
{
	return(sqrt(v*v));
}

//! Square of the norm of v, which is v*v.
double norm2(flatvector v)
{
	return v*v;
}

typedef flatvector flatpoint;

//---------------------------------- Basis ---------------------------------
/*! \class Basis
 * \brief Basis with a spacepoint (p) and 3 dimensional axes x,y,z.
 */

Basis::Basis()
	: p(), x(1,0,0), y(0,1,0), z(0,0,1)
{}

Basis::Basis(spacepoint pp, spacepoint xx, spacepoint yy, spacepoint zz)
{
	p=pp;
	x=xx;
	y=yy;
	z=zz;
}

/*! p=p1, z=p2-p1, x points from p3 perpendicularly away from z
 */
Basis::Basis(spacepoint p1, spacepoint p2, spacepoint p3)
{
	spacevector temp;
	temp=p2-p1;
	z=temp/(norm(temp));
	temp=p3-p1;
	temp=temp|=z;
	x=temp/norm(temp);
	y=z/x;
	p=p1;
}

//! True if origin is zero, and axes are normal unit x,y,z.
bool Basis::isUnity()
{
	if (!p.isZero()) return false;
	if (x.x!=1 || x.y!=0 || x.z!=0) return false;
	if (y.x!=0 || y.y!=1 || y.z!=0) return false;
	if (z.x!=0 || z.y!=0 || z.z!=1) return false;
	return true;
}

/*! p=p1, z=p2-p1, x points from p3 perpendicularly away from z
 */
void Basis::Set(spacepoint p1, spacepoint p2, spacepoint p3)
{
	spacevector temp;
	temp=p2-p1;
	z=temp/(norm(temp));
	temp=p3-p1;
	temp=temp|=z;
	x=temp/norm(temp);
	y=z/x;
	p=p1;
}

//! Create the basis from 4 arrays of 3 doubles.
Basis::Basis(double *np,double *nx,double *ny,double*nz)
{
	p=spacepoint(np[0],np[1],np[2]);
	x=spacepoint(nx[0],nx[1],nx[2]);
	y=spacepoint(ny[0],ny[1],ny[2]);
	z=spacepoint(nz[0],nz[1],nz[2]);
}

//! nonzero if p==p, x==x, y==y and z==z
int operator==(Basis b1,Basis b2)
{
	return b1.p==b2.p && b1.x==b2.x && b1.y==b2.y && b1.z==b2.z;
}

//! Transform point v to coordinates in basis.
spacepoint Basis::transformTo(spacepoint &v)
{
	spacevector p=(v-p);
	return spacevector(p*x/(x*x),p*y/(y*y),p*z/(z*z));
}

//! Transform point v to coordinates from basis.
spacepoint Basis::transformFrom(spacepoint &v)
{
	return p + v.x*x + v.y*y + v.z*z;
}

//---------------------------------- spaceline ---------------------------------
/*! \class spaceline
 * \brief 3 dimensional line with point p and direction v.
 */


//---------------------------------- Plane ---------------------------------
/*! \class Plane
 * \brief Plane with point p and normal n.
 */


//! The distance from point p to line l.
double distance(spacepoint p, spaceline l)
{
	spacepoint t=p-l.p;
	t=t|=l.v;
	return sqrt(t.x*t.x+t.y*t.y+t.z*t.z);
}

//! Distance from point p to plane pln.
double distance(spacepoint p, Plane pln)
{
	spacepoint t=p-pln.p;
	t=t||pln.n;
	return sqrt(t.x*t.x+t.y*t.y+t.z*t.z);
}

//! True (nonzero if p1 and p2 describe the same plane
int issameplane(Plane p1,Plane p2)
{	spacevector t=p1.n/p2.n;
	return distance(p1.p,p2)==0 && isnotvector(t);	
}

//! Return the intersection of l1 and l2, without error checking.
/*! If lines do not intersect, (0,0,0) is returned.
 */
spacepoint operator*(spaceline l1, spaceline l2)
{
	vectorop_error=0;
	spacevector temp1,temp2;
	temp1=l1.v/l2.v;
	temp2=l2.p-l1.p;
	if (isnotvector(temp1))
	{	if (distance(l1.p,l2)==0)
			{ vectorop_error=-1; return(l1.p);  } /* same line */
		vectorop_error=1; return spacepoint();  /* parallel skew lines */
	}
	if (temp1*temp2==0)
	{
		temp2=temp2/l2.v;
		double t=0;
		if (temp1.x != 0) t=temp2.x/temp1.x;
		else if (temp1.y != 0) t=temp2.y/temp1.y;
		else if (temp1.z != 0) t=temp2.z/temp1.z;
		vectorop_error=0;
		temp1=t*l1.v;
		return(l1.p+temp1);
	}
	vectorop_error=2;  /* non parallel skew lines */
	return spacepoint();
}

//! Return the intersection of l1 and l2, with error checking.
/*! If lines do not intersect, (0,0,0) is returned.
 *
 * err is 0 if lines interesect, 1 if the lines are nonintersecting
 * and parallel, -1 if the lines are the same line, 2 if the lines
 * are skew (non-parallel, non-interesecting).
 */
spacepoint intersection(spaceline l1, spaceline l2, int &err)
{					 /* 2=skew, 1=parallel, -1=same line, 0=ok */
	vectorop_error=0;
	spacepoint p=l1*l2;
	err=vectorop_error;
	return p;
}

//! Return the intersection of p1 and p2, no error checking.
spaceline operator*(Plane p1, Plane p2)
{
	spaceline l;
	l.v=p1.n/p2.n;
	if (isnotvector(l.v)) {
		l.p=l.v;
		if (distance(p1.p,p2)==0)	/* same plane */
			{ vectorop_error=-1; return l; } 
		vectorop_error=1; /* skew planes */
		return l;
	}
	spacevector t=p2.p-p1.p,t2=l.v/p1.n,t3=l.v/p1.n; 
	l.p=p1.p + t*p2.n/(t2*p2.n)* t3;
	vectorop_error=0;
	return(l);
}

//! Return the intersection of p1 and p2, no error checking.
/*! err is 0 if the planes intersect, 1 if planes are parallel and
 * not the same, and -1 if p1 and p2 are the same plane.
 */
spaceline intersection(Plane p1, Plane p2, int &err)
{
	spaceline l=p1*p2;
	err=vectorop_error;
	return l;
}

//! The intersection of line l and plane p, no error checking.
spacepoint operator*(spaceline l, Plane p)
{
	double temp=l.v*p.n;
	if (temp){
		vectorop_error=0;
		spacevector t=p.p-l.p;
		return(l.p+l.v*(t*p.n/temp));
	}
	if (distance(l.p,p)==0) vectorop_error=-1; /* line in plane */
	else vectorop_error=1;  			/* line parallel */
	return(spacepoint(0,0,0));
}

//! The intersection of line l and plane p, with error checking.
/*! err is 0 for success, -1 for line is in the plane, 1 for line
 * is not in the plane and not intersecting.
 */
spacepoint intersection(spaceline l, Plane pl, int &err)
{							/* -1=line in plane, 1=parallel */
	vectorop_error=0;			 /* 0=ok */
	spacepoint p=l*pl;
	err=vectorop_error;
	return p;
}

//! Return the plane formed from lines l1 and l2.
/*! If possible, the plane's point is the intersection of
 * the lines. else ***
 * other erorrs***
 */
Plane operator+(spaceline l1, spaceline l2)
{
	Plane pl;
	pl.p=l1*l2;
	if (vectorop_error==1) { 
		spacevector t=l2.p-l1.p;
		pl=Plane(l1,t);
		vectorop_error=0; 
	} else if (vectorop_error==2) { vectorop_error=1; }
	else if (vectorop_error==0) pl.n=l1.v/l2.v;
	return pl;
}

//! Return the plane formed from the 2 lines.
/*! err is 0 for success, -1 for the lines being the same,
 * and 1 for lines being skew.
 */
Plane linesplane(spaceline l1, spaceline l2, int &err)  /* same line=-1 */
{								 /* skew lines=1 */
	vectorop_error=0;
	Plane pl=l1+l2;
	err=vectorop_error;
	return pl;
}

//! Make v=v/sqrt(v*v). Do nothing if v*v==0.
void normalize(flatvector &v)
{
	double d=v*v;
	if (!d) return;
	v=v/sqrt(d);
}

//! Make v=v/sqrt(v*v). Do nothing if v*v==0.
void normalize(spacevector &v)
{
	double d=v*v;
	if (!d) return;
	v=v/sqrt(d);
}

//! Make the axes be a normalized basis (all axes have unit lengths).
/*! This first normalizes the z axis, then recomputes
 * the x axis to be the part of x perpendicular to z,
 * then y is found as the cross product of z and x. This
 * corrects for any previous skewing of the basis axes.
 */
void normalize(Basis &b)
{
	normalize(b.z);
	b.x=(b.x|=b.z);
	normalize(b.x);
	b.y=b.z/b.x;
}

//! Return the angle between p1 and p2.
/*! This pretends that both vectors are in the same plane.
 */
double angle(spacevector p1,spacevector p2,int dec)//dec=0
{
	if (isnotvector(p1) || isnotvector(p2))
	{	vectorop_error=1;
		return(0);
	} else
	return((dec ? 180/M_PI : 1)*acos(p1*p2/norm(p1)/norm(p2)));
}

//! Return the angle between p1 and p2. Will be in range [0..pi].
double angle(flatvector p1,flatvector p2,int dec)//dec=0
{
	if (isnotvector(p1) || isnotvector(p2))
	{	vectorop_error=1;
		return(0);
	} else
	return((dec ? 180/M_PI : 1)*acos(p1*p2/sqrt((p1*p1)*(p2*p2))));
}

/*! Return the angle between p1 and p2. Will be in range [-pi..pi].
 * If a negative angle, then it is in clockwise direction, else counterclockwise,
 * in a right handed coordinate system.
 */
double angle_full(flatvector p1,flatvector p2,int dec)//dec=0
{
	if (p1.isZero() || p2.isZero()) {
		vectorop_error=1;
		return(0);
	}

	double a=acos(p1*p2/sqrt((p1*p1)*(p2*p2)));
	spacevector z=spacevector(p1.x,p1.y,0)/spacevector(p2.x,p2.y,0); //cross product
	if (z.z<0) a=-a; //is clockwise

	if (dec) return 180/M_PI * a;
	return a;
}

//! Return a signed angle using the asin of the norm of the 3-d cross product of p1 x p2, which will be an all z number.
/*! Return value will be in range [-pi/2..pi/2].
 */
double angle2(flatvector p1,flatvector p2,int dec)
{ // **** double check this math!!!!!!!
	spacevector pp1(p1.x,p1.y,0),pp2(p2.x,p2.y,0);
	pp1/=norm(pp1);
	pp2/=norm(pp2);
	spacevector z=pp1 / pp2;
	return (dec ? 180/M_PI : 1)*asin(z.z);
}

//---------------------------------- flatline ---------------------------------
/*! \class flatline
 * \brief 2 dimensional line with point p and direction v.
 *
 * The form flatline(p1,p2) will produce a line with p=p1, and v=p2-p1.
 */


//! Distance between p1 and p2
double distance(spacepoint p1,spacepoint p2)
{
	spacepoint t;
	t=p1-p2;
	return(sqrt(t*t));
}

//! Distance between points p1 and p2.
double distance(flatpoint p1,flatpoint p2)
{
	flatpoint t;
	t=p1-p2;
	return(sqrt(t*t));
}

//! Distance of point p to line l
double distance(flatpoint p, flatline l)
{
	if (l.v.x==0 && l.v.y==0) return 5000;
	return fabs((p-l.p)*flatvector(-l.v.y,l.v.x)/sqrt(l.v.x*l.v.x+l.v.y*l.v.y));
}

//! Distance of point p to segment between p1 to p2.
/*! If the point is too far off the segment on either end, then it is just
 * the distance to the corresponding segment endpoint.
 */
double distance(flatpoint p, flatpoint p1, flatpoint p2)
{
	if (p1==p2) return distance(p,p1);
	flatline l(p2,p1);
	double t=findindex(p,l);
	if (t>1) return distance(p,p1);
	if (t<0) return distance(p,p2);
	return distance(p,l);
}

//! Return intersection of l1 and l2, no error checking.
flatpoint operator*(flatline l1, flatline l2)
{
	flatvector temp; temp=transpose(l1.v);
	if (l2.v*temp==0)
		{ 	if (distance(l2.p,l1)==0) {
				vectorop_error=-1;   /* same */
				return flatvector();
			}
			vectorop_error=1;
			return flatvector();  /* parallel */
		}
	vectorop_error=0;
	return(l2.p+l2.v*((l1.p-l2.p)*temp/(l2.v*temp)));
}

//! Return intersection of l1 and l2, put point in p, return error code.
/*! Returns -1 if the lines are the same line, 1 if they are different but
 * parallel, and 0 if they intersect in a single point.
 *
 * \todo not threadsafe!!
 */
int intersection(flatline l1, flatline l2, flatpoint &p)
{						/* returns -1=same, 1=parallel, 0=ok */
	vectorop_error=0;
	p=l1*l2;
	return vectorop_error;
}

//! Find intersection of l1 and l2, and associated data.
/*! Returns 2 if the lines are the same line, 1 if they are different but
 * parallel. -2 and -1 respectively if pointing in opposite directions.
 * Returns 0 if they intersect in a single point.
 *
 * The actual intersection point, if any, will be put in p, and is equal to 
 * (l1.p+index1*l1.v) and also to (l1.p+index1*l1.v).
 * The indices will be returned when index1 or 2 are not NULL.
 *
 * If there is an error, p, index1, index2 are not changed.
 */
int intersection(flatline l1, flatline l2, flatpoint *p, double *index1, double *index2)
{
	flatvector temp; temp=transpose(l1.v);
	if (l2.v*temp==0) {
		int sn=1;
		if ((l1.v.x>0 && l2.v.x<0) || (l1.v.x<0 && l2.v.x>0)) sn=-1;
		else if ((l1.v.y>0 && l2.v.y<0) || (l1.v.y<0 && l2.v.y>0)) sn=-1;
		if (distance(l2.p,l1)==0) return sn*2; //same line
		return sn*1; //lines are parallel, not same
	}

	if (index2) *index2=(l1.p-l2.p)*temp/(l2.v*temp);
	if (index1) { temp=transpose(l2.v); *index1=(l2.p-l1.p)*temp/(l1.v*temp); }
	if (p) *p=l2.p+l2.v*(*index2);

	return 0;
}

//! For a point p on l, p=l.p + t*l.v, finds and returns t.
/*!  return l.v*(p-l.p)/(l.v*l.v);
 *
 * This is not really using the point p, but the point on the line where
 * a line extending from p goes through the line perpendicularly.
 */
double findindex(flatpoint p,flatline l)   /* p=lp+t*lv, find t */
{						/* assumes p on l: returns l.v/norm^2(l/v) * (p-l.p) */
	return l.v*(p-l.p)/(l.v*l.v);

//	err=1;
//	if ((p.x-l.p.x)*l.v.y!=(p.y-l.p.y)*l.v.x) { 
//	//cout <<"..p not on l..";
//		err=0; return 0; 
//	}
//	if (l.v.x!=0) return (p.x-l.p.x)/l.v.x;
//	return (p.y-l.p.y)/l.v.y;
}

//! For segment between p1 and p2, find intersection with line l.
/*! Returns 1 if there is an intersection, or 0 if there is not.
 */
int segmentandline(flatpoint p1,flatpoint p2,flatline l,flatpoint &p)
{						   /* err 0=not on line, 1=ok */
	flatline l2(p1,p2);
	int e; //remove e, not needed
	e=intersection(l2,l,p);
	if (e!=0) { return 0; }
	double t=findindex(p,l2);
	if (t>=0 && t<=1) return 1;
	return 0;
}

//! Find the intersection of two segments [a1,a2] and [b1,b2], put answer in p.
/*! Returns 1 if there is an intersection, or 0 if there is not.
 */
int segmentcross(flatpoint a1,flatpoint a2,flatpoint b1,flatpoint b2,flatpoint &p)
{							  /* err 0= no intersect, 1=ok */
	flatline l2(a1,a2),l(b1,b2);
	int e;
	e=intersection(l2,l,p);
	if (e!=0) { return 0; }
	double t=findindex(p,l2);
	//cout << "..segIndex="<<t<<"..";
	if (t<0 || t>1) return 0;
	t=findindex(p,l);
	if (t<0 || t>1) return 0;
	return 1;
}

//double distbetweenline(spaceline l1,spaceline l2) /* shortest distance of any point on l1 to any on l2 */
//{
//	spacevector a,b;	**** not work if lines are parallel or same
//	a=(l2.p-l1.p)|=l1.v;
//	b=l1.v/l2.v;
//	return a*b/sqrt(a*a);
//}

//double distbetweenseg(spaceline s1,spaceline s2) /* shortest d of any on s1 to any on s2 */
//{ *** }

//! Flatten the line l3d to be the orthographic projection onto basis x,y.
flatline flatten(spaceline l3d,Basis bas)
{				/* orthographic proj onto basis x-y */
	flatline l;
	l.p=flatpoint((l3d.p-bas.p)*bas.x,(l3d.p-bas.p)*bas.y);
	l.v=flatvector(l3d.v*bas.x,l3d.v*bas.y);
	return(l);
}

//! Flatten the point to be the orthographic projection onto basis x,y.
flatpoint flatten(spacepoint pnt, Basis bas)
{				/* orthographic proj onto basis x-y */
	return(flatpoint((pnt-bas.p)*bas.x,(pnt-bas.p)*bas.y));
}

//! Rotate the flatvector p angle ang.
/*! This is counterclockwise for possitive ang, looking at
 * standard mathematical basis having positive x to the right,
 * and positive y upward. Would be clockwise for typical computer
 * viewing.
 */
flatpoint rotate(flatpoint p, double ang, int dec)//dec=0
{				/* cw for y+ */
	if (dec) ang*=(M_PI/180);
	return flatvector(p.x*cos(ang)-p.y*sin(ang),
				p.y*cos(ang)+p.x*sin(ang));
}

//! Rotate p around orig by angle ang
flatpoint rotate(flatpoint p, flatpoint orig, double ang, int dec)//dec=0
{
	if (dec) ang*=(M_PI/180);
	flatvector pt,x,y;
	pt=(p-orig);
	x.x=orig.x+pt.x*cos(ang)-pt.y*sin(ang);
	x.y=orig.y+pt.y*cos(ang)+pt.x*sin(ang);
	return(x);
}

//! Rotate p around axis.
spacepoint rotate(spacepoint p, spaceline axis, double ang, int dec)//dec=0
{
	if (dec) ang*=(M_PI/180);
	spacevector pt,x,y;
	double t;
	x=(p-axis.p)|=axis.v;
	if (isnotvector(x)) return(p);
	t=norm(x);
	pt=p-x;
	x=x/t;
	y=axis.v/x; y=y/norm(y);
	return(pt+(t*cos(ang))*x+(t*sin(ang))*y);
}

//! Rotate the basis around its 'x', 'y', or 'z' axis.
void rotate(Basis &b, char w, double ang, int dec)//dec=0
{
	if (dec) ang*=(M_PI/180);
	Basis nb;
	nb.p=b.p;
	if (w=='z') {
		nb.z=b.z;
		nb.x=cos(ang)*b.x+sin(ang)*b.y;
		nb.y=-sin(ang)*b.x+cos(ang)*b.y;
	} else if (w=='y') {
		nb.y=b.y;
		nb.x=cos(ang)*b.x+sin(ang)*b.z;
		nb.z=-sin(ang)*b.x+cos(ang)*b.z;

	} else if (w=='x') {
		nb.x=b.x;
		nb.y=cos(ang)*b.y+sin(ang)*b.z;
		nb.z=-sin(ang)*b.y+cos(ang)*b.z;
	}
	b=nb;
}

//! Rotate vector v about an axis.
spacevector rotate(spacevector v, spacevector axis, double ang, int dec)//dec=0
{
	//DBG double normva=norm(v/axis);
	//DBG std::cerr<<"norm(v/axis): "<<normva;
	if (norm(v/axis)==0) {
		DBG std::cerr<<"   no rotation necessary"<<std::endl;
		return v;
	}
	//DBG std::cerr<<"  rotating..."<<std::endl;
	if (dec) ang*=M_PI/180;
	//--------------------------
	spacepoint y,x;
	double r;
	y=axis/v;
	x=y/axis;
	x/=norm(x);
	y/=norm(y);
	r=norm(v|=axis);

	spacepoint ans=v||axis;
	ans+=r*cos(ang)*x + r*sin(ang)*y;
	return ans;
	//--------------------------
//	Basis b=Basis(spacevector(),axis,v);
//	//return norm(v|=axis)*(cos(ang)*b.x+sin(ang)*b.y)+norm(v||axis)*b.z;
//	
//	//DBG:
//	spacepoint vecperp=v|=axis,
//			   vecpara=v||axis;
//	double vperp=norm(vecperp),
//		   vpara=norm(vecpara);
//	spacepoint ans= vperp*(cos(ang)*b.x+sin(ang)*b.y) + vpara*b.z;
//	return ans;
}

//! Rotate the basis about an axis w.
void rotate(Basis &b, spacevector w) /* ||w||=ang,w=axis */
{
	if (w*w==0) return;
	double ang=norm(w);
	b.x=rotate(b.x,w,ang);
	b.y=rotate(b.y,w,ang);
	b.z=rotate(b.z,w,ang);	
}

//! Rotate the basis about an axis w, pointing away from b.p.
void rotate(Basis &b, spacevector w, double ang)
{
	if (!ang) return;
	b.x=rotate(b.x,w,ang);
	b.y=rotate(b.y,w,ang);
	b.z=rotate(b.z,w,ang);	
}

//! Transform flatline l to coordinates in basis b.
void transform(flatline &l,Basis b)
{
	flatvector xv=flatvector(b.x.x,b.x.y),yv=flatvector(b.y.x,b.y.y),
			   bp=flatvector(b.p.x,b.p.y);	
	l.p=flatpoint((l.p-bp)*xv,(l.p-bp)*yv);
	l.v=flatvector(l.v*xv,l.v*yv);
}

//! Transform line l to coordinates in basis b.
void transform(spaceline &l,Basis b)
{
	spacevector p=(l.p-b.p);
	double xx=b.x*b.x,
		   yy=b.y*b.y,
		   zz=b.z*b.z;
	l.p=spacepoint(p*b.x/xx,p*b.y/yy,p*b.z/zz);
	l.v=spacevector(l.v*b.x/xx,l.v*b.y/yy,l.v*b.z/zz);
}

//! Transform point v to coordinates in basis b.
void transform(spacevector &v,Basis b)
{
	spacevector p=(v-b.p);
	v=spacevector(p*b.x/(b.x*b.x),p*b.y/(b.y*b.y),p*b.z/(b.z*b.z));
}

//! Invert point p through orig: return orig-(p-orig)
spacepoint invert(spacepoint p, spacepoint orig)
{
	return(2*orig-p);
}

/*! write out the points to cerr
 */
void dump_points(const char *label, flatpoint *p,int n, int offset)
{
	if (label) std::cerr <<"----"<<label<<"----"<<std::endl;
	for (int c=0; c<n; c++) {
		std::cerr <<p[c].x<<','<<p[c].y<<"  c="<<c+offset<<"  info2="<<p[c].info2<<"  ";
		if (p[c].info&LINE_Start   ) std::cerr <<"Start    ";
		if (p[c].info&LINE_Vertex  ) std::cerr <<"Vertex   ";
		if (p[c].info&LINE_Bez     ) std::cerr <<"Bez      ";
		if (p[c].info&LINE_Closed  ) std::cerr <<"Closed   ";
		if (p[c].info&LINE_Open    ) std::cerr <<"Open     ";
		if (p[c].info&LINE_End     ) std::cerr <<"End      ";
		if (p[c].info&LINE_Corner  ) std::cerr <<"Corner   ";
		if (p[c].info&LINE_Equal   ) std::cerr <<"Equal    ";
		if (p[c].info&LINE_Auto    ) std::cerr <<"Auto     ";
		if (p[c].info&LINE_Join    ) std::cerr <<"Join     ";
		if (p[c].info&LINE_Cap     ) std::cerr <<"Cap      ";
		if (p[c].info&LINE_Original) std::cerr <<"Original ";
		std::cerr <<std::endl;
	}
	if (label) std::cerr <<"---- end "<<label<<"----"<<std::endl;
}


//! Convert an SVG path data snippet to a flatpoint list.
/*!
 * The d attribute of Svg paths allows definition of lines composed of a mixture of
 * straight lines, cubic bezier lines, quartic bezier lines, circle arcs, and ellipses.
 *
 * If normalize==0, then parse points as in d.
 * If normalize==1, then scale to fit centered within a 1x1 box, with coordinates in range [0..1].
 *
 * Unless how&4==4, if d contains multiple paths, the paths are all parsed into a single list.
 * 
 * If everything in d is parsed, then endptr will be pointing to the terminating null
 * character of the string.
 *
 * Any path in d that corresponds to path with a single coordinate will be ignored.
 * 
 * If how&3==0, then allow mixed poly lines and bezier segments.
 * If how&3==1, (todo) then the returned path must be only a poly line.
 * If how&3==2, (todo) then the returned path must be all bezier segments.
 * if how&4==4 then return a normal closed path upon parsing a 'z'. Otherwise parse whole list.
 *
 * Ellipsoidal arcs are converted into cubic bezier segments.
 *
 * Svg path data follows the following format. The letter is a command. If more than the
 * given numbers are present, then assume they are parameters for another of the same command,
 * except for moveto, which switches to lineto for subsequent x,y.
 * <pre>
 *   M x y    move to absolute coordinate (x,y)
 *   m x y    move to coordinate (x,y) relative to previous coordinate
 *       Any further x y are implied lineto commands
 *
 *   L x y    draw a line to absolute coordinate (x,y)
 *   l x y    draw a line to coordinate (x,y) relative to previous coordinate
 *
 *   H x      draw a horizontal line to absolute coordinate (x,current y)
 *   h x      draw a horizontal line to coordinate ((current x)+x,(current y))
 *
 *   V x      draw a vertical line to absolute coordinate ((current x), y)
 *   v x      draw a vertical line to coordinate ((current x),(current y)+y)
 *
 *   C x1 y1 x2 y2 x y   draw a cubic bezier segment. 1 and 2 are controls, (x,y) is vertex
 *   c x1 y1 x2 y2 x y   like C, but using relative coordinates
 *
 *   S x2 y2 x y   draw a cubic bezier segment.  2 is the second controls, (x,y) is vertex,
 *   s x2 y2 x y     the first control is reflection of control from previous bezier segment,
 *                   or the current point, if there was no previous segment. 's' is for
 *                   relative coordinates
 *
 *   Q x1 y1 x y   draw a quartic bezier segment. 1 is control point, (x,y) is vertex
 *   q x1 y1 x y   like q, but using relative coordinates
 *
 *   T x y         draw a quartic bezier segment. (x,y) is vertex,
 *   t x y           the control point is reflection of control from previous bezier segment,
 *                   or the current point, if there was no previous segment. 't' is for
 *                   relative coordinates
 *
 *   A/a rx ry x-axis-rotation large-arc-flag sweep-flag x y
 *               draw an arc from the current point to the point (x,y)
 *               rx and ry are the x and y radii.
 *               the ellipse is rotated by x-axis-rotation (in degrees).
 *               if large-arc-flag==1, then the swept out arc is large. 0 means small.
 *               if sweep-flag==1, then the arc is traced in a positive angle direction,
 *               otherwise 0 means in a negative direction (hard to describe without pictures).
 *   
 *
 *   Z or z   close the current path by with a straight line. If a move does not follow it,
 *            then use the initial point of the path just closed as the starting point for
 *            the new path
 * </pre>
 *
 * \todo *** right now how&3 bits are ignored..
 * \todo *** if d doesn't start with a move, what is the spec'd default starting point??
 *    right now, failure results if not starting with a move
 */
flatpoint *SvgToFlatpoints(const char *d, char **endptr, int how, flatpoint *buffer, int buffersize, int *totalpoints, int normalize)
{
	if (!d) return NULL;

	Laxkit::NumStack<flatpoint> points;

	const char *p=d;
	char *ee;
	char command=0;
	char hascurpoint=0;
	char lastwasmove=0;//when previous point resulted from a move
	flatpoint curpoint,
			  lastcontrol, //Vector with length and direction of last control 
			  			   // handle pointing at the associated vertex
						   // ***TODO this needs to be implemented fully!!
			  fp; //temp vector
	flatpoint *last=NULL;
	int numpoints=0; //number of points in current contiguous path
	//int error=0;

	try {
	  while (*p) {
		while (isspace (*p)) p++;
		if (!*p) break;

		if (isdigit(*p) || *p=='.' || *p=='-') {
			//is a number, so assume we are continuing last command
		} else {
			command=*p;
			p++;
		}

		if (command=='M' || command=='m') { 
			//if (command=='m' && !hascurpoint) command='M'; //initial 'm' is treated as 'M'

			if (numpoints>0 && !(points.e[points.n-1].info&(LINE_Open|LINE_Closed|LINE_End))) {
			//if (numpoints>0 && !(****last->info&(LINE_Open|LINE_Closed|LINE_End))) {
				 //move, if not first command is supposed to start a new subpath
				 //so terminate current path, by adding a point with a terminator flag
				last->info|=LINE_Open;
			}

			 // move curpoint
			fp.x=strtod(p,&ee);
			if (ee==p) throw (2); //bad number

			p=ee;
			while (isspace (*p) || *p==',') p++;
			fp.y=strtod(p,&ee);
			if (ee==p) throw (3); //bad number

			if (command=='m') {
				if (hascurpoint) fp=fp+curpoint; //lowercase command is relative coords
				command='l'; //subsequent numbers are implied lineto commands
			} else command='L';
			curpoint=fp;
			hascurpoint=1;
			numpoints=0;
			lastwasmove=1;
			
			p=ee;
			continue;

		} else if (command=='L' || command=='l') {
			 // line to: l (x y)+
			 // can have one or more x,y pairs
			if (!hascurpoint) throw (4);
			fp.x=strtod(p,&ee);
			if (ee==p) throw (5);

			p=ee;
			while (isspace (*p) || *p==',') p++;
			fp.y=strtod(p,&ee);
			if (ee==p) throw (6);

			p=ee;
			while (isspace (*p) || *p==',') p++;

			if (lastwasmove) {
				 //needed to add initial point of line
				points.push(curpoint);
				numpoints++;
			}

			if (command=='l') fp+=curpoint; //relative line

			points.push(fp);
			numpoints++;

			curpoint=fp;
			lastwasmove=0;
			continue;

		} else if (command=='H' || command=='h') {
			 //one or more x values, constant y
			if (!hascurpoint) throw (4);

			fp.y=curpoint.y;
			fp.x=strtod(p,&ee);
			if (ee==p) throw (5);
			p=ee;

			if (command=='h') fp.x+=curpoint.x;
			if (lastwasmove) {
				 //needed to add initial point of line
				points.push(curpoint);
				numpoints++;
			}
			points.push(fp);
			numpoints++;

			curpoint=fp;
			lastwasmove=0;
			continue;

		} else if (command=='V' || command=='v') {
			 //one or more y values, constant x
			if (!hascurpoint) throw (4);

			fp.x=curpoint.x;
			fp.y=strtod(p,&ee);
			if (ee==p) throw (6);
			p=ee;

			if (command=='v') fp.y+=curpoint.y;
			if (lastwasmove) {
				 //needed to add initial point of line
				points.push(curpoint);
				numpoints++;
			}
			points.push(fp);
			curpoint=fp;
			lastwasmove=0;
			continue;

		} else if (command=='C' || command=='c') {
			 //C (x1 y1 x2 y2 x y)+
			 //x1,y1 == first control point on curpoint
			 //x2,y2 == control point for x,y
			 // x,y  == final point
			 // 'c' has each of the 3 points relative TO CURRENT POINT !?!!

			if (!hascurpoint) throw (7);

			double dd[6];
			int n=LaxFiles::DoubleListAttribute(p,dd,6,&ee);
			if (ee==p || n!=6) throw (8);
			p=ee;

			if (lastwasmove) {
				 //needed to add initial point of line
				points.push(curpoint);
				numpoints++;
			}
			points.push(flatpoint(dd[0],dd[1], LINE_Bez));
			if (command=='c') { points.e[points.n-1].x+=curpoint.x; points.e[points.n-1].y+=curpoint.y; }

			points.push(flatpoint(dd[2],dd[3], LINE_Bez));
			if (command=='c') { points.e[points.n-1].x+=curpoint.x; points.e[points.n-1].y+=curpoint.y; }

			points.push(flatpoint(dd[4],dd[5]));
			if (command=='c') { points.e[points.n-1].x+=curpoint.x; points.e[points.n-1].y+=curpoint.y; }

			curpoint.set(points.e[points.n-1].x,points.e[points.n-1].y);

			numpoints+=3;

			lastcontrol=points.e[points.n-2] - points.e[points.n-3];
			lastwasmove=0;
			continue;

//		} else if (command=='S' || command=='s') {
//			 //smooth curve:
//			 //S (x2 y2 x y)+
//			 //x2,y2 == control point for x,y
//			 // x,y  == final point
//			 //control point for curpoint is reflection about curpoint of the control
//			 //point of previous segment. If the previous command was not s, S, c, or C,
//			 //then assume the control point is the same as curpoint.

//		} else if (command=='Q' || command=='q') {
//			 quadratic bezier curve: x1 y1 x y

//		} else if (command=='T' || command=='t') {
//			 smooth quadratic bezier curve continuation: (x y)+

//		} else if (command=='A' || command=='a') {
			 //create an arc
			//   A/a rx ry x-axis-rotation large-arc-flag sweep-flag x y
			//
			//      draw an arc. rx and ry are the x and y radii.
			//      the ellipse is rotated by x-axis-rotation (degrees).
			//	    An ellipse can touch both points in 2 ways (or none), and traced from p1 to p2 in two ways.
			//      Which of the 4 possible segments used in determined by various combinations of the flags.
			//      if large-arc-flag==1, then the swept out arc is large. 0 means small.
			//      if sweep-flag==1, then the arc is traced in a positive angle direction,
			//      otherwise 0 means in a negative direction (hard to describe without pictures).
			//			
			//		if either rx or ry is zero, then insert a straight line segment.
			//		if endpoint is same as current point, then skip.
			//		ignore negatives on rx or ry
			//		if there is no solution, scale up until there is
			//		for the flags, any nonzero value is taken to be 1
			//
			//		Implementation below is guided by svg 1.1 spec implementation notes F.6

//			if (!hascurpoint) throw (9);
//
//			double dd[7];
//			int n=LaxFiles::DoubleListAttribute(p,dd,7,&ee);
//			if (ee==p || n!=7) throw (10);
//
//			double x1=curpoint.x,
//				   y1=curpoint.y,
//				   xr  =fabs(dd[0]),
//				   yr  =fabs(dd[1]),
//				   xrot=dd[2]*M_PI/180,
//				   la  =(dd[3]==0 ? 0 : 1),
//				   sw  =(dd[4]==0 ? 0 : 1),
//				   x2  =dd[5],
//				   y2  =dd[6];
//
//			if (curpoint.x==x2 && curpoint.y==y2) continue; //skip when same as curpoint
//
//			flatpoint mid=(curpoint-flatpoint(x2,y2))/2;
//			double x1p= cos(xrot)*mid.x + sin(xrot)*mid.y;
//			double y1p=-sin(xrot)*mid.x + cos(xrot)*mid.y;
//			double lambda=x1p*x1p/(rx*rx) + y1p*y1p/(ry*ry);
//			if (lambda>1) { //correction for scaling up when no solution otherwise
//				rx*=sqrt(lambda);
//				ry*=sqrt(lambda);
//			}
//
//			double sq=sqrt((rx*rx*ry*ry - rx*rx*y1p*y1p - ry*ry*x1p*x1p) / (rx*rx*y1p*y1p + ry*ry*x1p*x1p));
//			if (la==sw) sq=-sq;
//
//			double cxp=sq*rx*y1p/ry;
//			double cyp=-sq*ry*x1p/rx;
//
//			double cx=cos(xrot)*cxp-sin(xrot)*cyp + (x1+x2)/2;
//			double cy=sin(xrot)*cxp+cos(xrot)*cyp + (y1+y2)/2;
//
//			flatvector u,v;
//			u.x=1; u.y=0;
//			v.x=(x1p-cxp)/rx;  v.y=(y1p-cyp)/ry;
//#define SIGN(a) ((a)<0?-1:((a)>0?1:0))
//			double theta1=SIGN(u.x*v.y-u.y*v.x)*acos(u*v/norm(u)/norm(v)); //returns in range 0..pi
//			double thetad=***;
//
//			if (sw==0 && theta1>0) theta1-=2*M_PI;  *** impl notes are mixed about whether this is theta1 or thetad!!!!
//			else if (sw!=0 && theta1<0) theta1+=2*M_PI;
//
//			 //now ellipse is centered at cx,cy, radii rx,ry, rotateh xrot, from theta1 to (theta1+thetad)
//			***
//
//			lastwasmove=0;
//			continue;

		} else if (command=='Z' || command=='z') {
			 //close path. If command other than a moveto follows a z, then the next subpath
			 //begins at the same INITIAL point as the current subpath!!
			if (!hascurpoint || numpoints<=1) throw(10);
			
			if (how&4) {
				 //makes a single path, by
				 //connecting with beginning of curve and returning
				points.e[points.n-1].info|=LINE_Closed;
				while (isspace(*p)) p++;
				if (endptr) *endptr=const_cast<char*>(p);
				break;
			}

			points.e[points.n-1].info|=LINE_Closed;
			curpoint=points.e[points.n-1];
			 //find first point in line
			for (int c=points.n-2; c>=0; c--) {
				if (points.e[c].info&(LINE_Closed|LINE_Open)) {
					curpoint=points.e[c+1];
				}
			}
			numpoints=0; 

			lastwasmove=0;
			continue;

		} else if (isalpha(*p)) {
			std::cerr << " *** must implement '"<<*p<<"' in SvgToFlatpoints()!!"<<std::endl;
			if (endptr) *endptr=const_cast<char *>(p);
			break;

		} else {
			 //hit a non-letter with unclaimed command. this halts parsing
			if (endptr) *endptr=const_cast<char *>(p);
			break;
		}
	  } //while more characters

	} catch (int e) {
		 //was error
		DBG std::cerr <<"svgtoflatpoints() had error in parsing: "<<e<<std::endl;
		points.flush();
		if (endptr) *endptr=const_cast<char *>(p);
	}

	if (buffer && buffersize<points.n) {
		 //supplied buffer not big enough to store the points!!
		*totalpoints = points.n;
		return NULL;		
	}

	flatpoint *newpoints = points.extractArray(totalpoints);
	if (buffer) {
		//memcpy(buffer, newpoints, (*totalpoints)*sizeof(flatpoint));
		for (int c=0; c<*totalpoints; c++) buffer[c] = newpoints[c];
		delete[] newpoints;
		newpoints=buffer;
	}

	if (normalize) {
		Laxkit::DoubleBBox box;
		for (int c=0; c<*totalpoints; c++) {
			 //note this is not a true bounds if bez curves stick way out...
			 //to do properly requires min/max computations
			
			//if (!(newpoints[c].info&LINE_Bez)) box.addtobounds(newpoints[c]);
			box.addtobounds(newpoints[c]);
		}
		double width =box.boxwidth();
		double height=box.boxheight();

		if (width!=0 || height!=0) {
			double scale=1;
			if (width>height) scale=1/width;
			else scale=1/height;

			flatpoint center((box.minx+box.maxx)/2,(box.miny+box.maxy)/2);
			for (int c=0; c<*totalpoints; c++) {
				newpoints[c].x = (newpoints[c].x-center.x)*scale + .5;
				newpoints[c].y = (newpoints[c].y-center.y)*scale + .5;
			}
		}
	}

	//DBG dump_points("svgtoflatpoint: ",newpoints,*totalpoints, 2);


	return newpoints;
}

//---------------------------------- Quaternion ---------------------------------
/*! \class Quaternion
 * 4 Dimensional vector (x,y,z,w) == x*i + y*j + z*k + w
 *
 * where i*i = j*j == k*k = i*j*k = -1.
 *
 */

//! Divide by norm. If null vector, do nothing.
void Quaternion::normalize()
{
	if (x==0 && y==0 && z==0 && w==0) return;

	double d=sqrt(x*x+y*y+z*z+w*w);
	x/=d;
	y/=d;
	z/=d;
	w/=w;
}

void Quaternion::SetEuler(spacevector v)
{
	double roll  = v.x;  // yaw (Z), pitch (Y), roll (X)
	double pitch = v.y;
	double yaw   = v.z;

	// Abbreviations for the various angular functions
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);

    w = cr * cp * cy + sr * sp * sy;
    x = sr * cp * cy - cr * sp * sy;
    y = cr * sp * cy + sr * cp * sy;
    z = cr * cp * sy - sr * sp * cy;
}

spacevector Quaternion::GetEuler()
{
	double roll, pitch, yaw;
	
    // roll (x-axis rotation)
    double sinr_cosp = 2 * (w * x + y * z);
    double cosr_cosp = 1 - 2 * (x * x + y * y);
    roll = std::atan2(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    double sinp = 2 * (w * y - z * x);
    if (std::abs(sinp) >= 1)
        pitch = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
    else
        pitch = std::asin(sinp);

    // yaw (z-axis rotation)
    double siny_cosp = 2 * (w * z + x * y);
    double cosy_cosp = 1 - 2 * (y * y + z * z);
    yaw = std::atan2(siny_cosp, cosy_cosp);

    return spacevector(roll, pitch, yaw);
}

//! nonzero if x==x, y==y and z==z
int operator==(Quaternion v1,Quaternion v2)
{
	return v1.x==v2.x && v1.y==v2.y && v1.z==v2.z && v1.w==v2.w;
}

//! nonzero if x!=x || y!=y || z!=z
int operator!=(Quaternion v1,Quaternion v2)
{
	return v1.x!=v2.x || v1.y!=v2.y || v1.z!=v2.z || v1.w!=v2.w;
}

//! (a,b,c)+(d,e,f)=(a+d, b+e, c+f)
Quaternion operator+(Quaternion a,Quaternion b)
{
	return(Quaternion(a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w));
}

//! v+=(a,b,c)  => (v.x+a, v.y+b, v.z+c)
Quaternion operator+=(Quaternion &a,Quaternion b)
{
	return a=a+b;
}

//! (a,b,c)-(d,e,f)=(a-d, b-e, c-f)
Quaternion operator-(Quaternion a,Quaternion b)
{
	return(Quaternion(a.x-b.x, a.y-b.y, a.z-b.z, a.w-b.w));
}

//! v-=(a,b,c)  => (v.x-a, v.y-b, v.z-c)
Quaternion operator-=(Quaternion &a,Quaternion b)
{
	return a=a-b;
}

//! -v  => (-v.x, -v.y, -v.z)
Quaternion Quaternion::operator-()
{
	return(Quaternion(-x,-y,-z,-w));
}

//! r*(x,y,z)=(r*x, r*y, r*z)
Quaternion operator*(double r, Quaternion a)  /*const times Quaternion */
{
	return(Quaternion(r*a.x, r*a.y, r*a.z, r*a.w));
}

//! (x,y,z)*r=(r*x, r*y, r*z)
Quaternion operator*(Quaternion a, double r)  /*const times Quaternion */
{
	return(Quaternion(r*a.x, r*a.y, r*a.z, r*a.w));
}

//! v*=r does v=v*r
Quaternion operator*=(Quaternion &a, double r)  /*const times Quaternion */
{
	return a=a*r;
}

//! v/r= (v.x/r, v.y/r, v.z/r)
Quaternion operator/(Quaternion a,double r)  /* divide y double */
{
	if (r==0) {
		vectorop_error=1;
		return(a); //division by zero!
	}
	//else
	return(Quaternion(a.x/r, a.y/r, a.z/r, a.w/r));
}

//! Does v=v/r
Quaternion operator/=(Quaternion &v,double r)
{
	return v=v/r;
}

//! Product. Note multiplication is noncommutative, ie a*b != b*a.
Quaternion operator*(Quaternion a,Quaternion b)  /*dot product */
{
	return Quaternion(
		(a.w*b.x + a.x*b.w - a.y*b.z + a.z*b.y), //x
		(a.w*b.y + a.x*b.z + a.y*b.w - a.z*b.x), //y
		(a.w*b.z - a.x*b.y + a.y*b.x + a.z*b.w), //z
		(a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z)  //w
	);
}

//! Return the conjugate w - x*i - y*j - z*k
Quaternion Quaternion::conjugate()
{
	return Quaternion(-x,-y,-z,w);
}


/*! @} */


