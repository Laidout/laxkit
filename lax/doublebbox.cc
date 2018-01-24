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
/*! \fn double DoubleBBox::boxwidth() 
 * Return maxx-minx if maxx>minx. Else return 0.
 */
/*! \fn double DoubleBBox::boxheight() 
 * Return maxy-miny if maxy>miny. Else return 0.
 */
	
DoubleBBox::DoubleBBox(const DoubleRectangle &rect)
{
	minx = rect.x;
	miny = rect.y;
	maxx = rect.x + rect.width;
	maxy = rect.y + rect.height;
}

void DoubleBBox::setbounds(const DoubleRectangle &rect)
{
	minx = rect.x;
	miny = rect.y;
	maxx = rect.x + rect.width;
	maxy = rect.y + rect.height;
}


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

void DoubleBBox::addtobounds(flatpoint *pts,int n)
{
	for (int c=0; c<n; c++) addtobounds(pts[c]);
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
/*! Note if bbox is invalid, nothing is changed about current bbox.
 */
void DoubleBBox::addtobounds(DoubleBBox *bbox)
{
	if (!bbox) return;
	if (bbox->maxx<bbox->minx || bbox->maxy<bbox->miny) return;
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

/*! Basically adds points (x,y) and (x+width, y+height).
 * Does not care if rect width or height are negative.
 */
void DoubleBBox::addtobounds(const DoubleRectangle &rect)
{
	addtobounds(rect.x, rect.y);
	addtobounds(rect.x+rect.width, rect.y+rect.height);
}

//! Return whether the transformed box touches. **** incomplete implementation
/*! If settointersection, then set the bounds of *this to the bounding box of the
 * intersection.
 *
 * If touching, then return true if any part touches this.
 * If !touching, then return true only if bbox is totally inside this.
 *
 * \todo *** this is not well implemented, settointersection not implemented.
 * \todo *** this needs some rethinking.. options are confusing, touching is ambiguous
 */
int DoubleBBox::intersect(const double *m,DoubleBBox *bbox, int touching, int settointersection)//s=0
{
	if (!m || !bbox) return 0;
	int n=0;
	flatpoint ul,ur,ll,lr;

	 //this here checks if any of the corners are inside the bounding box.
	ll=transform_point(m,flatpoint(bbox->minx,bbox->miny));
	if (ll.x>=minx && ll.x<=maxx && ll.y>=miny && ll.y<=maxy) n++;

	ul=transform_point(m,flatpoint(bbox->minx,bbox->maxy));
	if (ul.x>=minx && ul.x<=maxx && ul.y>=miny && ul.y<=maxy) n++;

	ur=transform_point(m,flatpoint(bbox->maxx,bbox->maxy));
	if (ur.x>=minx && ur.x<=maxx && ur.y>=miny && ur.y<=maxy) n++;

	lr=transform_point(m,flatpoint(bbox->maxx,bbox->miny));
	if (lr.x>=minx && lr.x<=maxx && lr.y>=miny && lr.y<=maxy) n++;

	if (!n && touching) {
		////flatpoint p1(minx,miny), p2(minx,maxy), p3(maxx,maxy), p4(maxx,miny);
		////if (segmentcross(p1,p2, ul,ur, p)) return 1;
		////if (segmentcross(p1,p2, ur,lr, p)) return 1;
		////if (segmentcross(p1,p2, lr,ll, p)) return 1;
		////if (segmentcross(p1,p2, ll,ul, p)) return 1;
		//
		////if (segmentcross(p2,p3, ul,ur, p)) return 1;
		////if (segmentcross(p2,p3, ur,lr, p)) return 1;
		////if (segmentcross(p2,p3, lr,ll, p)) return 1;
		////if (segmentcross(p2,p3, ll,ul, p)) return 1;
		//
		////if (segmentcross(p3,p4, ul,ur, p)) return 1;
		////if (segmentcross(p3,p4, ur,lr, p)) return 1;
		////if (segmentcross(p3,p4, lr,ll, p)) return 1;
		////if (segmentcross(p3,p4, ll,ul, p)) return 1;
		//
		////if (segmentcross(p4,p1, ul,ur, p)) return 1;
		////if (segmentcross(p4,p1, ur,lr, p)) return 1;
		////if (segmentcross(p4,p1, lr,ll, p)) return 1;
		////if (segmentcross(p4,p1, ll,ul, p)) return 1;
		//
		//------------------
		//flatpoint pts[4];
		//pts[0].x=minx; pts[0].y=miny;
		//pts[1].x=maxx; pts[1].y=miny;
		//pts[2].x=maxx; pts[2].y=maxy;
		//pts[3].x=minx; pts[3].y=maxy;
		//if (intersections(ll,lr, pts,4,true)) return 1;
		//if (intersections(lr,ur, pts,4,true)) return 1;
		//if (intersections(ur,ul, pts,4,true)) return 1;
		//if (intersections(ul,ll, pts,4,true)) return 1;
		return 0;
	}
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
 * \todo should be more specific about the kind of intersection: all inside, all outside, or just touching.
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

/*! Return the absolute point given bbox points.
 * That is, point (0,0) is returned as (minx,miny)
 * and (1,1) is returned as (maxx, maxy).
 */
flatpoint DoubleBBox::BBoxPoint(double x,double y) const
{ return flatpoint(minx+x*(maxx-minx),miny+y*(maxy-miny)); }

/*! Find an affine transform that makes *this fit inside container, scaling up or down as
 * necessary to fit. If m_ret!=NULL, then put the matrix in m_ret. Else return a new double[6].
 */
double *DoubleBBox::FitToBox(const DoubleBBox &container, double *m_ret)
{
	if (!m_ret) m_ret = new double[6];
	transform_identity(m_ret);

	double scale = 1;
	double scalex = container.boxwidth() / boxwidth();
	double scaley = container.boxheight() / boxheight();

	//if (scalex > 1 && scaley >= 1) scale = (scalex < scaley ? scalex : scaley);
	//else if (scalex <= 1 && scaley <= 1) scale = (scalex < scaley ? scalex : scaley);
	scale = (scalex < scaley ? scalex : scaley);

	m_ret[0] = m_ret[3] = scale;

	 //make centers coincide
	flatpoint o  = transform_point(m_ret, (minx+maxx)/2, (miny+maxy)/2);
	flatpoint oc = container.BBoxPoint(.5,.5);
	m_ret[4] = oc.x - o.x;
	m_ret[5] = oc.y - o.y;

	return m_ret;
}

/*! Add these amounts to the bounds. No validity checking is done.
 * top goes on miny, and bottom goes to maxy.
 */
void DoubleBBox::ShiftBounds(double left, double right, double top, double bottom)
{
	minx += left;
	maxx += right;
	miny += top;
	maxy += bottom;
}

/*! Add this amount to each side. This is for convenience to call ExpandBounds(-amount, amount, -amount, amount).
 */
void DoubleBBox::ExpandBounds(double amount)
{
	ShiftBounds(amount, amount, amount, amount);
}


} // namespace Laxkit 
