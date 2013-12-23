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
//    Copyright (C) 2004-2010 by Tom Lechner
//


#include <lax/doublebbox.h>
#include <lax/transformmath.h>

namespace Laxkit {
	
/*! \class DoubleBBox
 * \brief Class with double minx,maxx,miny,maxy.
 */
/*! \fn	int DoubleBBox::validbounds()
 * \brief Returns maxx>=minx && maxy>=miny.
 */
/*! \fn DoubleBBox::DoubleBBox() 
 * \brief Create empty invalid bbox (maxx<minx and maxy<miny).
 */
/*! \fn DoubleBBox::DoubleBBox(flatpoint p) 
 * \brief Create box whose bounds are p.
 */
/*! \fn DoubleBBox::DoubleBBox(double mix,double max,double miy,double may) 
 * \brief Create box whose bounds are as given.
 */
/*! \fn void DoubleBBox::setbounds(double mix,double max,double miy,double may)
 * \brief Set the bounds to the specified values. 
 */
/*! \fn void DoubleBBox::clear()
 * \brief Make box invalid: set minx=miny=0 and maxx=maxy=-1. 
 */
	

//! Just copy over the bounds.
void DoubleBBox::setbounds(DoubleBBox *bbox)
{
	minx=bbox->minx;
	maxx=bbox->maxx;
	miny=bbox->miny;
	maxy=bbox->maxy;
}

//! Make this bbox be the bounds for the given n points. (does not add to previous bounds)
void DoubleBBox::setbounds(flatpoint *pts,int n)
{
	minx=maxx=pts[0].x;
	miny=maxy=pts[0].y;
	for (int c=1; c<n; c++) addtobounds(pts[c]);
}

//! Add bbox to bounds, first transforming it by 6 element transform matrix m.
void DoubleBBox::addtobounds(const double *m, DoubleBBox *bbox)
{
	addtobounds(transform_point(m,flatpoint(bbox->minx,bbox->miny)));
	addtobounds(transform_point(m,flatpoint(bbox->maxx,bbox->miny)));
	addtobounds(transform_point(m,flatpoint(bbox->maxx,bbox->maxy)));
	addtobounds(transform_point(m,flatpoint(bbox->minx,bbox->maxy)));
}

//! Add the bbox to the bounds. Set the bounds if current bounds are invalid.
void DoubleBBox::addtobounds(DoubleBBox *bbox)
{
	if (!bbox) return;
	if (maxx<minx || maxy<miny) { setbounds(bbox); return; }
	addtobounds(flatpoint(bbox->minx,bbox->miny));
	addtobounds(flatpoint(bbox->maxx,bbox->miny));
	addtobounds(flatpoint(bbox->maxx,bbox->maxy));
	addtobounds(flatpoint(bbox->minx,bbox->maxy));
}

//! Expand bounds to contain p. Set the bounds to p if current bounds are invalid.
void DoubleBBox::addtobounds(flatpoint p)
{
	if (maxx<minx || maxy<miny) { minx=maxx=p.x; miny=maxy=p.y; return; }
	if (p.x<minx) minx=p.x;
	else if (p.x>maxx) maxx=p.x;
	if (p.y<miny) miny=p.y;
	else if (p.y>maxy) maxy=p.y;
}

void DoubleBBox::addtobounds(double x,double y)
{ addtobounds(flatpoint(x,y)); }


//! Does the transformed box touch. **** incomplete implementation
/*! If settointersection, then set the bounds of *this to the bounding box of the
 * intersection. If touching
 *
 * \todo *** this is not well implemented, settointersection not implemented.
 * \todo *** this needs work so that there is option 
 */
int DoubleBBox::intersect(double *m,DoubleBBox *bbox, int touching, int settointersection)//s=0
{
	if (!m || !bbox) return 0;
	int n=0;
	flatpoint p;
	 //this here checks if any of the corners are inside the bounding box.
	p=transform_point(m,flatpoint(bbox->minx,bbox->miny));
	if (p.x>=minx && p.x<=maxx && p.y>=miny && p.y<=maxy) n++;
	p=transform_point(m,flatpoint(bbox->minx,bbox->maxy));
	if (p.x>=minx && p.x<=maxx && p.y>=miny && p.y<=maxy) n++;
	p=transform_point(m,flatpoint(bbox->maxx,bbox->maxy));
	if (p.x>=minx && p.x<=maxx && p.y>=miny && p.y<=maxy) n++;
	p=transform_point(m,flatpoint(bbox->maxx,bbox->miny));
	if (p.x>=minx && p.x<=maxx && p.y>=miny && p.y<=maxy) n++;

	if (!n) return 0;
	if (touching) return n; //true if any corners in bounds
	return n==4; //true when all 4 corners are in bounds
}

//! Just return intersect(bbox.minx,...,settointersection).
int DoubleBBox::intersect(DoubleBBox *bbox, int settointersection)//s=0
{
	return intersect(bbox->minx,bbox->maxx,bbox->miny,bbox->maxy,settointersection);
}

//! Intersect the given bounds with this's bounds. Return 1 for non-empty intersection
/*! If settointersection!=0, then set current bounds to the intersection or invalid bounds
 * if there is no intersection.
 *
 * \todo should me more specific about the kind of intersection: all inside, all outside, or just touching.
 */
int DoubleBBox::intersect(double mix,double max,double miy,double may, int settointersection)//s=0
{
	if (minx>mix) mix=minx;
	if (maxx<max) max=maxx;
	if (miny>miy) miy=miny;
	if (maxy<may) may=maxy;
	if (settointersection) {
		minx=mix;
		maxx=max;
		miny=miy;
		maxy=may;
	}
	return max>=mix && may>=miy;
}

//! Return whether the given point is contained within or on the bounds.
/*! Invalid bounds will always return 0.
 */
int DoubleBBox::boxcontains(double x, double y)
{
	if (maxx>=minx && maxy>=miny && x>=minx && x<=maxx && y>=miny && y<=maxy) return 1;
	return 0;
}

} // namespace Laxkit 
