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

#include <lax/interfaces/coordinate.h>
#include <lax/bezutils.h>

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

namespace LaxInterfaces {


//-------------------- SegmentControls ---------------------------

/*! \class SegmentControls
 * \brief Class to hold info about how to generate associated Coordinate points.
 * 
 * Path and PathsData objects hold simple bezier based paths. However, each of the points
 * in those paths may point to a SegmentControls object, which determines how those
 * points were generated in the first place.
 */
/*! \var int SegmentControls::iid()
 * \brief Return an interface id for what can understand this segment control.
 */




//------------------ Coordinate ---------------------------
////NOTE: Coordinate conflicts with nurbs++ (though it is in namespace PLib there)
/*! \class Coordinate
 * \brief The basic coordinate class for points.
 * \code #include <lax/interfaces/pathinterface.h> \endcode
 *
 * \todo perhaps make this ref counting?
 * 
 * The point's actual values are wrapped in x(), y(), and p(). This will
 * help out making coordinate references later on.. Many of the functions in
 * here must be called with care, lest memory leaks occur. 
 * PathInterface takes, or at least ought to be
 * taking, the appropriate precautions.
 *
 *
 * Here are the defines for flags:
 * \code
 *  //POINT_MASK is basically what kind of coordinate it is
 * #define POINT_VERTEX            (1<<0)
 * #define POINT_TONEXT            (1<<1)
 * #define POINT_TOPREV            (1<<2)
 * #define POINT_MIDDLE            (1<<3)
 * #define POINT_LOOP_TERMINATOR   (1<<14)
 * #define POINT_TERMINATOR        (1<<15)
 * #define POINT_MASK              (1|2|4|8|(1<<14)|(1<<15))
 * 
 *  // Point's x and y coordinates can be defined relative to various things.
 *  // This is to facilitate keeping controls with their vertex. One must
 *  // define an origin and axis. 
 *  //  Origin: If none of the following 3 are set, then absolute coordinates
 *  //  	are assumed.
 * #define POINT_REL_TO_PREV       (1<<4)
 * #define POINT_REL_TO_NEXT       (1<<5)
 * #define POINT_REL_TO_MIDPOINT   (1<<6)
 * 
 *  // Axis:
 *  //  Axis 1 has the x direction == (v2-v1)/||v2-v1||, y=transpose x
 *  //  Axis 2 has the x direction == (v2-v1)/2,         y=transpose x
 *  // If neither axis 1 or 2 is set, then the axis of the points coords is used.
 * #define POINT_REL_AXIS_1        (1<<7)
 * #define POINT_REL_AXIS_2        (1<<8)
 * #define POINT_REL_AXIS_DEFAULT  (1<<9)
 * 
 *  // selectable means that PathInterface can select/move/rotate/etc
 * #define POINT_SELECTABLE        (1<<10)
 * #define POINT_READONLY          (1<<11)
 * 
 *  // program should make efforts to make the tangent smooth here
 *  // (this is attribute of a vertex)
 * #define POINT_SMOOTH            (1<<12)
 * 
 *  // program should make efforts to make the tangent length equal on both sides of the point
 * #define POINT_REALLYSMOOTH      (1<<12|1<<13)
 * \endcode
 */
/*! \var double Coordinate::next_s
 * For vertex points, a hint about the length of the following segment to the next vertex point.
 */
 

//! Your basic constructor.
Coordinate::Coordinate()
{
	next=prev=NULL;
	controls=NULL;
	flags=POINT_VERTEX;
	iid=0;
	info=0;
	next_s=0;
	//DBG cerr <<"+++ New coordinate vertex: "<<fp.x<<','<<fp.y<<endl;
}

//! Constructor to make a vertex point at p.
Coordinate::Coordinate(flatpoint p)
{
	fp=p;
	next=prev=NULL;
	controls=NULL;
	flags=POINT_VERTEX;
	iid=0;
	info=0;
	next_s=0;
	//DBG cerr <<"+++ New coordinate vertex: "<<fp.x<<','<<fp.y<<endl;
}

//! Constructor to make a vertex point at (x,y).
Coordinate::Coordinate(double x,double y)
{
	fp=flatpoint(x,y);
	next=prev=NULL;
	controls=NULL;
	flags=POINT_VERTEX;
	iid=0;
	info=0;
	next_s=0;
	//DBG cerr <<"+++ New coordinate vertex: "<<fp.x<<','<<fp.y<<endl;
}

//! Constructor to make a vertex point at (x,y). Incs count of ctl.
Coordinate::Coordinate(double x,double y,unsigned long nflags,SegmentControls *ctl) 
{ 
	fp=flatpoint(x,y); 
	next=prev=NULL; 
	controls=ctl;
	if (controls) controls->inc_count();
	flags=nflags; 
	iid=0; 
	info=0; 
	next_s=0;
	//DBG cerr <<"+++ New coordinate: "<<fp.x<<','<<fp.y<<endl;
}

/*! Incs count of ctl.
 */
Coordinate::Coordinate(flatpoint pp,unsigned long nflags,SegmentControls *ctl) 
{ 
	fp=pp; 
	next=prev=NULL; 
	controls=ctl;
	if (controls) controls->inc_count();
	flags=nflags; 
	iid=0; 
	info=0; 
	next_s=0;
	//DBG cerr <<"+++ New coordinate: "<<fp.x<<','<<fp.y<<endl;
}

//! Creates copy with next and prev equal to NULL.
/*! This will result in count of this->controls being incremented by 1.
 */
Coordinate::Coordinate(const Coordinate &p)
{
	controls=p.controls;
	if (controls) controls->inc_count();
	fp=p.fp;
	flags=p.flags;
	iid=p.iid;
	info=p.info;
	next=prev=NULL;
	next_s=0;
}

 //! Deletes both prev and next if they exist.
 /*! Assumes simple connections to other nodes,
  *  Care taken here to destroy loops of unknown size, meaning that if this point
  *  is connected to a loop, the whole loop is deleted properly, without segfaulting.
  */
Coordinate::~Coordinate()
{
	//DBG cerr <<"-=-=--=-=-=Coordinate destructor"<<endl;
	if (controls) controls->dec_count();

	Coordinate *p=this->next;
	while (p && p!=this) p=p->next;
	if (p) { // is closed loop, so must open it.
		prev->next=NULL;
		prev=NULL;
	}
	if (prev) {
		prev->next=NULL;
		delete prev;
	}
	if (next) {
		next->prev=NULL;
		delete next;
	}
}

/*! Does not modify next and prev. Everything else is copied over.
 * controls count will end up being incremented by 1.
 */
const Coordinate &Coordinate::operator=(const Coordinate &p)
{
	controls=p.controls;
	if (controls) controls->inc_count();
	fp=p.fp;
	flags=p.flags;
	iid=p.iid;
	info=p.info;

	return *this;
}

//! Sort of a workaround for copy constructor, copy point, flags, iid, info, but set next, prev to NULL
/*! I might simply not understand copy constructors, but this function exists in case there
 * are points that are supposed to be accessed and set the same as a normal Coordinate, but
 * the internals are worked around somehow. Normal copy constructors do not afford such flexiblity.
 *
 * controls count will end up being incremented by 1.
 */
Coordinate *Coordinate::duplicate()
{
	Coordinate *blah=new Coordinate(p(),flags,controls);
	blah->info=info;
	blah->next=blah->prev=NULL;

	if (controls) controls->inc_count();
	blah->controls=controls;

	return blah;
}

//! Return a complete copy of the path, SegmentControls and all.
Coordinate *Coordinate::duplicateAll()
{
	SegmentControls *lastcontrols=controls;
	SegmentControls *ncontrols;
	ncontrols=(controls?controls->duplicate():NULL);

	Coordinate *npstart=new Coordinate(p(),flags,ncontrols);
	if (ncontrols) ncontrols->dec_count();
	npstart->info=info;

	Coordinate *np=npstart;
	Coordinate *pp=next;

	while (pp && pp!=this) {

		np->next=new Coordinate(pp->p(),pp->flags,NULL);
		np->next->prev=np;
		np=np->next;

		if (lastcontrols && pp->controls==lastcontrols) {
			np->controls=ncontrols;
			ncontrols->inc_count();
		} else if (pp->controls) {
			lastcontrols=pp->controls;
			ncontrols=pp->controls->duplicate();
			np->controls=ncontrols;
		} else {
			lastcontrols=NULL;
			ncontrols=NULL;
		}

		pp=pp->next;

		if (pp==this) {
			np->next=npstart;
			npstart->prev=np;
		}
	}


	return npstart;
}


/*! Make this a closed loop if it is not already, and return 1;
 * Does nothing if it is already closed, returns 0 in this case.
 */
int Coordinate::close()
{
	Coordinate *last=lastPoint(0);
	if (last==this) return 0; //already closed

	Coordinate *first=firstPoint(0);
	last->next=first;
	first->prev=last;
	return 1;
}

//! Return 1 if this==v. Return 2 if is TOPREV of v, or -1 if is TONEXT of v. Else 0.
int Coordinate::isAttachedTo(Coordinate *v)
{
	if (this==v) return 1;
	if ((flags&POINT_TOPREV) && v && v->next==this) return 2;
	if ((flags&POINT_TONEXT) && v && v->prev==this) return -1;
	return 0;
}

//! Return whether this is an endpoint, or is a handle connected to an endpoint.
/*! If this is a handle, return 2 if it is a TOPREV and there is no next vertex, or
 * -2 if is a TONEXT and there is no previous vertex.
 *
 * If this is a vertex, return 1 if there is no next vertex, or -1 if there is no previous vertex.
 *
 * Else return 0.
 */
int Coordinate::isEndpoint()
{
	Coordinate *v=this;
	int ret=1;
	if (flags&POINT_TOPREV) {
		v=v->prev;
		ret=2;
	} else if (flags&POINT_TONEXT) {
		v=v->next;
		ret=2;
	}

	v=nextVertex(0);
	if (!v) return ret;
	v=previousVertex(0);
	if (!v) return -ret;

	return 0;
}

//! Return whether the coordinate is part of a closed path or not.
int Coordinate::isClosed()
{
	Coordinate *p=this;
	do {
		p=p->next;
	} while (p && p!=this);
	if (!p) return 0;
	return 1;
}

//! Append a new vertex point (x,y) to the next NULL, with iid=0.
/*! Puts the point in the next most NULL. If this is a closed loop, then
 * puts in this->next. It does not check for consistency with controls...
 *
 * This function is for convenience of making simple lines on the fly.
 * If ctl!=NULL, then its count is incremented.
 */
void Coordinate::append(double x,double y,unsigned long flags,SegmentControls *ctl)
{
	//Coordinate *newpoint=new Coordinate(flatpoint(x,y),(flags&~(POINT_TONEXT|POINT_TOPREV|POINT_MIDDLE))|POINT_VERTEX,0);
	Coordinate *newpoint=new Coordinate(flatpoint(x,y),flags,0);
	if (ctl) ctl->inc_count();
	newpoint->controls=ctl;
	Coordinate *here=lastPoint(0);

	if (here->next) {
		here->next->prev=newpoint;
		newpoint->next=here->next;
	}
	here->next=newpoint;
	newpoint->prev=here;
}

 //! This connects a SINGLE point np either after or before (after==0).
 /*! Assumes that np->next==np->prev==NULL. To insert a string of points, use insert().
  *
  * This function only concerns itself with the immediately next connection. It
  * does not check for consistency with control points. That is the duty of PathInterface.
  * It does not scan down the line. It assumes that the relevant next/prevs
  * are of this and of np are all NULL. It does not do error checking.
  */
void Coordinate::connect(Coordinate *np,char after)//after=1
{
	if (after) {
		if (next) next->prev=np;
		np->next=next;
		next=np;
		np->prev=this;
	} else {
		if (prev) prev->next=np;
		np->prev=prev;
		prev=np;
		np->next=this;
	}
}

 //! This severs the link between this and this->next (or prev if after=0)
 /*! This function should only be called by PathInterface, PathsData, or Path,
  *  which take the necessary precautions.
  *
  *  Returns pointer to the coordinate on the other side of the break, if any.
  */
Coordinate *Coordinate::disconnect(char after)//after=1
{
	if (after) {
		if (next) next->prev=NULL;
		Coordinate *pp=next;
		next=NULL;
		return pp;
	} else {
		if (prev) prev->next=NULL;
		Coordinate *pp=prev;
		prev=NULL;
		return pp;
	}
}

//! Removes all points from this to and including p from the path, closing the connection of the remaining path.
/*! This completely removes the points from the line by setting next=prev=NULL, and
 * next->prev=next, and prev->next=next if next and/or prev exist. p is assumed to be in
 * the next direction from this.
 *
 * Returns a coordinate that is on the remaining path, if any. If this was on a closed path, then the segment
 * this..p will now be open.
 *
 * This function would only be called by PathInterface, PathsData, or Path,
 * which take the necessary precautions to ensure that no path heads are
 * screwed up.
 */
Coordinate *Coordinate::detachThrough(Coordinate *p)
{
	 //make sure p is actually connected in line
	Coordinate *e=this;
	while (e!=p && e->next && e->next!=this) e=e->next;

	if (prev) {
		if (prev==e) { e->next=NULL; prev=NULL; } //special case for small closed paths!
		else prev->next=e->next;
	}
	if (e->next) {
		e->next->prev=prev;
	}

	Coordinate *still=e->next;
	if (!still) still=prev;

	e->next=prev=NULL;
	return still;
}

//! Removes this from the path.
/*! This completely removes *this from the line by setting next=prev=NULL, and
 * next->prev=next, and prev->next=next if next and/or prev exist.
 *
 * Returns a coordinate that is on the remaining path, if any.
 *
 * This function would only be called by PathInterface, PathsData, or Path,
 * which take the necessary precautions to ensure that no path heads are
 * screwed up.
 */
Coordinate *Coordinate::detach()
{
	if (next) next->prev=prev;
	if (prev) prev->next=next;

	Coordinate *still=next;
	if (!still) still=prev;

	next=prev=NULL;
	return still;
}

//! Insert point after *this (if after!=0) else before *this.
/*! Assumes that c is the most previous point in a list of one or
 *  more points, that is open or closed. Also assumes that the segment
 *  is sectioned correctly, that is, it does not sever control segments.
 *  PathInterface is supposed to check for that. Returns 1 on success.
 *
 *  Please note this does not check for TOPREV or TONEXT. It will insert
 *  directly between *this and this->next (or *this and this->prev).
 */
int Coordinate::insert(Coordinate *c,int after) //after=1;
{ 
	if (!c) return 0;
	Coordinate *e=c->lastPoint();
	c=c->firstPoint();
	if (after) {
		c->prev=this;
		e->next=next;
		if (next) next->prev=e;
		next=c;
	} else {
		e->next=this;
		c->prev=prev;
		if (prev) prev->next=c;
		prev=e;
	}
	return 1;
}

 //! Return 1 if co is somewhere in path, else 0.
 /*! See also PathsData::hasCoord().
  *
  * If index!=NULL, then return the offset of (number of vertex points between) co from *this.
  */
int Coordinate::hasCoord(Coordinate *co, int *index)
{
	Coordinate *c=firstPoint(0), *d=c;
	int v=-1;
	do {
		if (c && c->flags&POINT_VERTEX) v++;
		if (co==c) {
			if (index) *index=v;
			return 1;
		}
		c=c->next;
	} while (c && c!=d);

	 //point not found!
	if (index) *index=-1;
	return 0;
}

 //! Return the most next point in open line, or this, if the line is a closed loop.
 /*! If v!=0, find the last vertex. If there is no last vertex (path is all
  *  control points) returns the last point (or this if closed loop).
  *  lastPoint will never return NULL.
  */
Coordinate *Coordinate::lastPoint(int v) //v=0
{
	Coordinate *p,*p2;
	p=this;
	while (p->next && p->next!=this) p=p->next; 
	if (p->next!=NULL) p=this;

	 // now p->next is either NULL or this

	if (v) { // find first vertex >= p
		p2=p->previousVertex(1);
		if (p2) p=p2;
	}
	return p;
}

 //! Return the most previous point in line, or this if line is closed. 
 /*! If v!=0, find the first vertex. If there is no first vertex (path is all
  * control points) returns the first point (or this if closed loop).
  * firstPoint will never return NULL.
  */
Coordinate *Coordinate::firstPoint(int v) //v=0
{
	Coordinate *p,*p2;
	p=this;
	while (p->prev && p->prev!=this) p=p->prev; 
	 // now p->prev is either NULL or this
	if (p->prev!=NULL) p=this;

	if (v) { // find first vertex >= p
		p2=p->nextVertex(1);
		if (p2) p=p2;
	}
	return p;
}

//! Return the number of individual points (v==0), or the number of vertices if v==1.
int Coordinate::NumPoints(int v)
{
	int num=0;
	Coordinate *p,*start;
	p=start=firstPoint(v);
	do {
		if (v) { if (p->flags&POINT_VERTEX) num++; }
		else num++;
		p=p->next;
	} while (p && p!=start);

	return num;
}

 //! Finds the next vertex after this.
 /*! Returns next vertex starting AFTER this, if n==0. \n
  *  Returns next vertex starting WITH this, if n==1. \n
  *  returns NULL if there is no such next vertex.
  *
  *  Please note, this ignores SegmentControl objects.
  */
Coordinate *Coordinate::nextVertex(int n) //n=0
{
	if (n && this->flags&POINT_VERTEX) return this;
	Coordinate *p=next;
	while (p && p!=this && !(p->flags&POINT_VERTEX)) {
		p=p->next;
	}
	if (p==this && !(p->flags&POINT_VERTEX)) return NULL;
	return p;
}

 //! Find the previous vertex from this.
 /*! Returns previous vertex starting BEFORE this, if n==0. \n
  *  Returns previous vertex starting WITH this, if n==1. \n
  *  Returns NULL if can't find any previous vertex.
  *
  *  Please note, this ignores SegmentControl objects.
  */
Coordinate *Coordinate::previousVertex(int n) //n=0
{
	if (n && this->flags&POINT_VERTEX) return this;
	Coordinate *p=prev;
	while (p && p!=this && !(p->flags&POINT_VERTEX)) {
		p=p->prev;
	}
	if (p==this && !(p->flags&POINT_VERTEX)) return NULL;
	return p;
}

//! Return a vector pointing in the visual direction the line seems to go in.
/*! Note this is different than a tangent. If prev or next points are attached,
 * then use those, but if there is a null tangent, then the numerical tangent
 * will be 0. In this case, assume bezier points, and calculate the visual direction.
 * If we are on a terminator, then grab the tangent from the non-null side.
 *
 * A unit vector will always be returned. Default on not possible is (1,0).
 */
flatpoint Coordinate::direction(int after)
{
	flatvector v;
	double len;
	int neg=0;

	if (after) {
		if (next) {
			if (next->flags&(POINT_TOPREV|POINT_VERTEX)) v=next->p()-p();
		} else if (prev) {
			if (prev->flags&(POINT_TONEXT|POINT_VERTEX)) v=-prev->p()+p();
		}
		len=v*v;
		if (len>1e-15) return v/sqrt(len);
		if (!next && prev) { neg=1; after=!after; }
	} else {
		if (prev) {
			if (prev->flags&(POINT_TONEXT|POINT_VERTEX)) v=prev->p()-p();
		} else if (next) {
			if (next->flags&(POINT_TOPREV|POINT_VERTEX)) v=-next->p()+p();
		}
		len=v*v;
		if (len>1e-15) return v/sqrt(len);
		if (!prev && next) { neg=1; after=!after; }
	}

	 //null tangent! resort to point finding

	Coordinate *c1,*c2,*p2;
	if (after) {
		p2=next;
		if (!p2) return flatpoint(1,0);
		if (p2->flags&POINT_TOPREV) {
			c1=p2;
			p2=p2->next;
		} else c1=this;
		if (!p2) return flatpoint(1,0);
		if (p2->flags&POINT_TONEXT) {
			c2=p2;
			p2=p2->next;
		} else c2=p2;
	} else {
		p2=prev;
		if (!p2) return flatpoint(1,0);
		if (p2->flags&POINT_TONEXT) {
			c1=p2;
			p2=p2->prev;
		} else c1=this;
		if (!p2) return flatpoint(1,0);
		if (p2->flags&POINT_TOPREV) {
			c2=p2;
			p2=p2->prev;
		} else c2=p2;
	}
	if (!p2) return flatpoint(1,0);
	v=Laxkit::bez_point(.01, p(),c1->p(),c2->p(),p2->p())-p();
	v/=norm(v);

	return neg ? -v : v;
}

/*! Return a bezier segment in p1,c1,c2,p2, based on the segment following *this.
 * If the segment is a straight line then return 1.
 * If a bezier segment return 2.
 * If there is no next segment, return 0.
 *
 * When there is only one next or prev control point, the missing control point is taken
 * to be the same as the adjacent vertex.
 */
int Coordinate::resolveToControls(flatpoint &p1, flatpoint &c1, flatpoint &c2, flatpoint &p2)
{
	if (!nextVertex(0)) return 0;

	p1=p();
	Coordinate *pn=next;
	if (pn->flags&(POINT_TOPREV|POINT_TONEXT)) {
		 //we do have control points
		if (pn->flags&POINT_TOPREV) {
			c1=pn->p();
			pn=pn->next;
		} else c1=p();
		if (!pn) return 0; //no next vertex!

		if (pn->flags&POINT_TONEXT) {
			c2=pn->p();
			pn=pn->next;
		} else { //otherwise, should be a vertex
			c2=pn->p();
		}
		
		p2=pn->p();
		return 2;

	}
	
	 //we do not have control points, so is just a straight line segment
	c1=p1;
	c2=pn->p();
	p2=c2;
	return 1;
}



//----------------------------------- Coordinate Shape Makers ----------------------------------


/*! Returns polygon arranged in counterclockwise direction as viewed in right handed
 * coordinate system.
 */
Coordinate *CoordinatePolygon(flatpoint center, double radius, bool point_on_x_axis, int num_sides, int num_winding)
{
	Coordinate *coord=NULL;
	Coordinate *cc=NULL;

	double angle=num_winding*2*M_PI/num_sides;
	double start=0;
	if (!point_on_x_axis) start=angle/2;
	double a=start;

	flatpoint p;
	for (int c=0; c<num_sides; c++) {
		p=center+flatpoint(radius*cos(a), radius*sin(a));
		a+=angle;

		if (cc==NULL) {
			coord=cc=new Coordinate(p);
		} else {
			cc->next=new Coordinate(p);
			cc->next->prev=cc;
			cc=cc->next;
		}
	}

	coord->prev=cc;
	cc->next=coord;

	return coord;
}

///*! round==1 is maximum, meaning edge to midpoint is rounded.
// * round is the round extent counterclockwise from a point, round2 on the
// * clockwise side.
// */
//Coordinate RoundedRectangle(flatpoint ll, flatpoint ur, double round,double round2)
//{
//	***
//	
//}


/*! This will return a Coordinate list with 3*n points, arranged c-v-c - c-v-c ... c-v-c.
 *
 * This is a primitive approximation, where each point gets control points added before
 * and after, such that the control rods are of length 1/3 the distance between the point
 * and the next point, and the rods are parallel to the line connecting the previous
 * and next point.
 *
 * This is just like bez_from_points(), but adapted to Coordinate objects.
 */
Coordinate *BezApproximate(flatpoint *l, int n)
{
	// There are surely better ways to do this. Not sure how powerstroke does it.
	// It is not simplied/optimized at all. Each point gets control points to smooth it out.
	//
	// tangents at points are || to (p+1)-(p-1).
	// Lengths of control rods are 1/3 of distance to adjacent points

	Coordinate *coord=NULL;
	Coordinate *curp=NULL;

    flatvector v,p, pp,pn;
	flatvector opn, opp;
    double sx;
	
    for (int c=0; c<n; c++) {
        p=l[c];

		if (c==0)   opp=p; else opp=l[c-1];
		if (c==n-1) opn=p; else opn=l[c+1];

        v=opn-opp;
        v.normalize();

        sx=norm(p-opp)*.333;
        pp=p - v*sx;

        sx=norm(opn-p)*.333;
        pn=p + v*sx;

		 //create tonext control
		if (!curp) coord=curp=new Coordinate(pp,POINT_TONEXT,NULL);
		else {
			curp->next=new Coordinate(pp,POINT_TONEXT,NULL);
			curp->next->prev=curp;
			curp=curp->next;
		}

		 //create vertex
		curp->next=new Coordinate(p);
		curp->next->prev=curp;
		curp=curp->next;

		 //create toprev control
		curp->next=new Coordinate(pn,POINT_TOPREV,NULL);
		curp->next->prev=curp;
		curp=curp->next;
    }

	return coord;
}


/*! Create a flatpoint list from the coord.
 * This list is formatted to render properly with Displayer::drawFormattedPoints().
 *
 * Return a new flatpoint[]. 
 */
flatpoint *CoordinateToFlatpoint(Coordinate *coord, int *n_ret)
{
	Laxkit::NumStack<flatpoint> points;
	Coordinate *p=coord;
	Coordinate *p2, *start;
	flatpoint c1,c2;

	p=start=p->firstPoint(1);
	points.push(p->p()); points.e[points.n-1].info=LINE_Vertex;

    do { //one loop per vertex point
        p2=p->next; //p points to a vertex
        if (!p2) break;

        //p2 now points to first Coordinate after the first vertex
        if (p2->flags&(POINT_TOPREV|POINT_TONEXT)) {
             //we do have control points
            if (p2->flags&POINT_TOPREV) {
                c1=p2->p();
                p2=p2->next;
            } else c1=p->p();
            if (!p2) break;

            if (p2->flags&POINT_TONEXT) {
                c2=p2->p();
                p2=p2->next;
            } else { //otherwise, should be a vertex
                //p2=p2->next;
                c2=p2->p();
            }

			points.push(c1); points.e[points.n-1].info=LINE_Bez;
			points.push(c2); points.e[points.n-1].info=LINE_Bez;
			points.push(p2->p()); points.e[points.n-1].info=LINE_Vertex;

        } else {
             //we do not have control points, so is just a straight line segment
			points.push(p2->p()); points.e[points.n-1].info=LINE_Vertex;
        }

        p=p2;
    } while (p && p->next && p!=start);

    if (p==start) {
		points.e[points.n-1].info|=LINE_Closed;
	}

	
	flatpoint *pts=points.extractArray(n_ret);
	*n_ret=points.n;
	return pts;
}


} //namespace LaxInterfaces


