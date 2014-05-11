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
//    Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef _LAX_COORDINATE_H
#define _LAX_COORDINATE_H


#include <lax/anobject.h>
#include <lax/vectors.h>

#include <iostream>
using namespace std;


namespace LaxInterfaces {


//-------------------- SegmentControls ---------------------------

class SegmentControls : public Laxkit::anObject
{
 public:
	virtual int iid()=0;
	virtual ~SegmentControls() {}
	virtual SegmentControls *duplicate()=0;
};


//-------------------- Coordinate ---------------------------

 // POINT_MASK is basically what kind of coordinate it is
#define POINT_VERTEX            (1<<0)
#define POINT_TONEXT            (1<<1)
#define POINT_TOPREV            (1<<2)
#define POINT_MIDDLE            (1<<3)
#define POINT_LOOP_TERMINATOR   (1<<14)
#define POINT_TERMINATOR        (1<<15)
#define POINT_MASK              (1|2|4|8|(1<<14)|(1<<15))

 // Point's x and y coordinates can be defined relative to various things.
 // This is to facilitate keeping controls with their vertex. One must
 // define an origin and axis. 
 //  Origin: If none of the following 3 are set, then absolute coordinates
 //  	are assumed.
#define POINT_REL_TO_PREV       (1<<4)
#define POINT_REL_TO_NEXT       (1<<5)
#define POINT_REL_TO_MIDPOINT   (1<<6)

 // Axis:
 //  Axis 1 has the x direction == (v2-v1)/||v2-v1||, y=transpose x
 //  Axis 2 has the x direction == (v2-v1)/2,         y=transpose x
 // If neither axis 1 or 2 is set, then the axis of the points coords is used.
#define POINT_REL_AXIS_1        (1<<7)
#define POINT_REL_AXIS_2        (1<<8)
#define POINT_REL_AXIS_DEFAULT  (1<<9)

 // selectable means that PathInterface can select/move/rotate/etc
#define POINT_SELECTABLE        (1<<10)
#define POINT_READONLY          (1<<11)

 // program should make efforts to make the tangent smooth here
 // (this is attribute of a vertex)
#define POINT_SMOOTH            (1<<12)

 // program should make efforts to make the tangent length equal on both sides of the point
#define POINT_REALLYSMOOTH      (1<<13)

#define POINT_NODELETE          (1<<14)

class Coordinate
{
 public:
	flatpoint fp;
	unsigned long flags;
	int iid,info;
	SegmentControls *controls;
	Coordinate *next,*prev;

	Coordinate();
	Coordinate(flatpoint p);
	Coordinate(double x,double y);
	Coordinate(flatpoint pp,unsigned long nflags,SegmentControls *ctl);
	Coordinate(double x,double y,unsigned long nflags,SegmentControls *ctl);
	Coordinate(const Coordinate &p);
	virtual ~Coordinate();
	const Coordinate &operator=(const Coordinate &p);
	virtual double x() { return fp.x; }
	virtual   void x(double xx) { fp.x=xx; }
	virtual double y() { return fp.y; }
	virtual   void y(double yy) { fp.y=yy; }
	virtual flatpoint p() { return fp; }
	virtual      void p(double xx,double yy) { fp.x=xx; fp.y=yy; }
	virtual      void p(flatpoint pp) { fp=pp; }
	virtual int isAttachedTo(Coordinate *v);
	virtual int isEndpoint();
	virtual Coordinate *previousVertex(int n=0);
	virtual Coordinate *nextVertex(int n=0);
	virtual Coordinate *firstPoint(int v=0); // return the first point in open line, or this
	virtual Coordinate *lastPoint(int v=0); // return the first point in open line, or this
	virtual int hasCoord(Coordinate *co); // return 1 if c is somewhere in paths
	virtual void ShiftPoint(flatpoint p) { fp+=p; } //*** this could be an overloaded (Coordinate)+=(flatpoint)
	virtual Coordinate *duplicate();
	virtual Coordinate *duplicateAll();
	virtual void append(double x,double y,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL);
	virtual int close();
	virtual int isClosed();
	virtual void connect(Coordinate *np,char after=1);
	virtual Coordinate *disconnect(char after=1);
	virtual Coordinate *detach();
	virtual Coordinate *detachThrough(Coordinate *p);
	virtual int insert(Coordinate *c,int after=1);
	virtual int NumPoints(int v);
	virtual flatpoint direction(int after);
};


//----------------------------------- Coordinate Shape Makers ----------------------------------

Coordinate *CoordinatePolygon(flatpoint center, double radius, bool point_on_x_axis, int num_sides, int num_winding);
//Coordinate RoundedRectangle(flatpoint ll, flatpoint ur, double round,double round2);

Coordinate *BezApproximate(flatpoint *l, int n);

} //namespace LaxInterfaces

#endif


