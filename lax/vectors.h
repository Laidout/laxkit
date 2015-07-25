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
//    Copyright (C) 2004-2009,2012-2013 by Tom Lechner
//
#ifndef _LAX_VECTORS_H
#define _LAX_VECTORS_H

#include <cmath>

//namespace LaxMath {

int vector_error();

enum PointInfoTags {
	LINE_Start   =(1<<0),
	LINE_Vertex  =(1<<1),
	LINE_Bez     =(1<<2),
	LINE_Closed  =(1<<3),
	LINE_Open    =(1<<4),
	LINE_End     =(1<<5),

	LINE_Corner  =(1<<6),
	LINE_Equal   =(1<<7),
	LINE_Auto    =(1<<8),
	LINE_Join    =(1<<9),
	LINE_Cap     =(1<<10),
	LINE_Original=(1<<11),

	LAXLINE_MAX =(11)
};

class spacevector
{
  public:
	double x,y,z;
	int info;
	spacevector(void) {x=y=z=0; info=0; }
	spacevector(double xx, double yy, double zz) {x=xx; y=yy; z=zz; info=0;}
	spacevector(double *v) { x=v[0]; y=v[1]; z=v[2]; info=0; }
	spacevector(const spacevector &vec) {  x=vec.x; y=vec.y; z=vec.z; info=vec.info;  }
	spacevector operator-();
	void normalize();
	void set(double xx,double yy,double zz) { x=xx; y=yy; z=zz; }

	bool isZero() const { return x==0 && y==0 && z==0; }
	double norm() const { return sqrt(x*x+y*y+z*z); }
	double norm2() const { return x*x+y*y+z*z; }
};

int operator==(spacevector v1,spacevector v2);
int operator!=(spacevector v1,spacevector v2);
spacevector operator+(spacevector a,spacevector b);
spacevector operator+=(spacevector &a,spacevector b);
spacevector operator-(spacevector a,spacevector b);
spacevector operator-=(spacevector &a,spacevector b);
spacevector operator*(double r, spacevector a);  /*const times spacevector */
spacevector operator*(spacevector a, double r);  /*const times spacevector */
spacevector operator*=(spacevector &a, double r);  /*const times spacevector */
spacevector operator/(spacevector a,spacevector b);  /* cross product */
spacevector operator/=(spacevector &a, spacevector &b);
spacevector operator/(spacevector a,double r);  /* divide y double */
spacevector operator/=(spacevector &a,double r);
double operator*(spacevector a,spacevector b);  /*dot product */
spacevector operator||(spacevector a, spacevector b); /*gives part of a parallel to b */
spacevector operator|=(spacevector a, spacevector b); /*gives part of a perp. to b */
int isvector(spacevector a);
int isnotvector(spacevector a);
int areparallel(spacevector v1, spacevector v2);
double norm(spacevector p1);
double norm2(spacevector p1);
typedef spacevector spacepoint;

class flatvector
{
  public:
	double x,y;
	int info;
	int info2;
	flatvector(void) {x=y=0; info=0; info2=0; }
	flatvector(double xx, double yy) { x=xx; y=yy; info=0; info2=0; }
	flatvector(double xx, double yy, int ninfo) { x=xx; y=yy; info=ninfo; info2=0; }
	flatvector(double *v) { x=v[0]; y=v[1]; info=0; info2=0; }
	flatvector(const flatvector &vec) {  x=vec.x; y=vec.y; info=vec.info; info2=0; }
	flatvector operator-();
	void normalize();
	void set(double xx,double yy) { x=xx; y=yy; }

	bool isZero() const { return x==0 && y==0; }
	double angle() const { return atan2(y,x); }
	double norm() const { return sqrt(x*x+y*y); }
	double norm2() const { return x*x+y*y; }
	double cross(flatvector v) const { return x*v.y-y*v.x; } /*magnitude and sign of cross product, which points in z direction */
	double distanceTo(flatvector v) const { return sqrt((v.x-x)*(v.x-x)+(v.y-y)*(v.y-y)); }
};

flatvector transpose(flatvector v);
int operator==(flatvector v1, flatvector v2);
int operator!=(flatvector v1, flatvector v2);
flatvector operator+(flatvector a,flatvector b);
flatvector operator+=(flatvector &a,flatvector b);
flatvector operator-(flatvector a,flatvector b);
flatvector operator-=(flatvector &a,flatvector b);
flatvector operator*(double r, flatvector a);  /*const times flatvector */
flatvector operator*(flatvector a, double r);  /*const times flatvector */
flatvector operator*=(flatvector &a, double r);  /*const times flatvector */
flatvector operator/(flatvector a,double r);  /*flatvector by const */
flatvector operator/=(flatvector &a,double r);  /*flatvector by const */
double operator*(flatvector a,flatvector b);  /*dot product */
flatvector operator||(flatvector a, flatvector b); /*gives part of x parallel to y*/
flatvector operator|=(flatvector a, flatvector b); /*gives part of x*/
double distparallel(flatvector a, flatvector b); /*gives part of a parallel to b*/
int isnotvector(flatvector v);
int areparallel(flatvector v1, flatvector v2);
double norm(flatvector p1);
double norm2(flatvector p1);
bool clockwise(flatvector p, flatvector v);
int point_is_in(flatvector p,flatvector *points, int n);
typedef flatvector flatpoint;

class Basis
{
  public:
	spacepoint p;
	spacevector x,y,z;
	Basis();
	Basis(spacepoint p1, spacepoint p2, spacepoint p3);  /* p=p1, z=p2-p1, x->p3 */
	Basis(spacepoint pp, spacepoint xx, spacepoint yy, spacepoint zz);
	Basis(double *np,double *nx,double *ny,double*nz);
	void Set(spacepoint p1, spacepoint p2, spacepoint p3);
	spacepoint transformFrom(spacepoint &v);
	spacepoint transformTo(spacepoint &v);
	bool isUnity();
};

class spaceline
{
  public:
	spacepoint p;
	spacevector v;
	spaceline() {p=spacepoint(); v=spacevector();}
	spaceline(spacepoint p1, spacepoint p2) { p=p1; v=p1-p2; }
    spaceline(double *v1,double *v2) { p=spacevector(v1); v=spacevector(v2); } 
};

class Plane
{
  public:
	spacepoint p;
	spacevector n;
	Plane() { p=spacepoint(); n=spacevector(); }
	Plane(spacepoint p1,spacepoint p2,spacepoint p3) 
		{  p=p3-p2; spacevector t=p1-p2; n=p/t; p=p2; }
	Plane(spacepoint p1, spacevector n1) { p=p1; n=n1; }
	Plane(spaceline l, spacevector v) { p=l.p; n=l.v/v; }
	Plane(double *v1,double *v2) { p=spacevector(v1); n=spacevector(v2); }
};

int operator==(Basis b1,Basis b2);
double distance(spacepoint p, spaceline l);
double distance(spacepoint p, Plane pln);
int issameplane(Plane p1,Plane p2);
spacepoint operator*(spaceline l1, spaceline l2);
spacepoint intersection(spaceline l1, spaceline l2, int &err); /* 2=skew, 1=parallel, -1=same line, 0=ok */
spaceline operator*(Plane p1, Plane p2);
spaceline intersection(Plane p1, Plane p2, int &err);
spacepoint operator*(spaceline l, Plane p);
spacepoint intersection(spaceline l, Plane pl, int &err);
Plane operator+(spaceline l1, spaceline l2);
Plane linesplane(spaceline l1, spaceline l2, int &err);  /* same line=-1 */
void normalize(flatvector &v);
void normalize(spacevector &v);
void normalize(Basis &b);
double angle(spacevector p1,spacevector p2,int dec=0);
double angle(flatvector p1,flatvector p2,int dec=0);
double angle_full(flatvector p1,flatvector p2,int dec=0);
double angle2(flatvector p1,flatvector p2,int dec=0);
class flatline
{
  public:
	flatpoint p;
	flatvector v;
	flatline() { p=flatpoint(); v=flatvector(1,0); }
	flatline(const flatline &fl) { p=fl.p; v=fl.v; }
	flatline(flatpoint p1, flatpoint p2) { p=p1; v=p2-p1; }
	flatline(double *v1,double *v2) { p=flatvector(v1); v=flatvector(v2); }
};

double distance(spacepoint p1,spacepoint p2);
double distance(flatpoint p1,flatpoint p2);
double distance(flatpoint p, flatpoint p1, flatpoint p2);
double distance(flatpoint p, flatline l);
flatpoint operator*(flatline l1, flatline l2);
int intersection(flatline l1, flatline l2, flatpoint &p);
int intersection(flatline l1, flatline l2, flatpoint *p, double *index1, double *index2);
double findindex(flatpoint p,flatline l);   /* p=lp+t*lv, find t,uses p-l.p||l.v */
int segmentandline(flatpoint p1,flatpoint p2,flatline l,flatpoint &p);
int segmentcross(flatpoint a1,flatpoint a2,flatpoint b1,flatpoint b2,flatpoint &p);
flatline flatten(spaceline l3d,Basis bas);
flatpoint flatten(spacepoint pnt, Basis bas);
flatpoint rotate(flatpoint p, double ang, int dec=0);
flatpoint rotate(flatpoint p, flatpoint orig, double ang, int dec=0);
spacepoint rotate(spacepoint p, spaceline axis, double ang, int dec=0);
void rotate(Basis &b, char w, double ang, int dec=0);
spacevector rotate(spacevector v, spacevector axis, double ang, int dec=0);
void rotate(Basis &b, spacevector w); /* ||w||=ang,w=axis */
void rotate(Basis &b, spacevector w, double ang);
void transform(flatline &l,Basis b);
void transform(spaceline &l,Basis b);
void transform(spacevector &v,Basis b);
spacepoint invert(spacepoint p, spacepoint orig);

void dump_points(const char *label, flatpoint *p,int n, int offset=0);

//} //namespace LaxMath

#endif

