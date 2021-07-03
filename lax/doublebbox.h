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
//    Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef _LAX_DOUBLEBBOX_H
#define _LAX_DOUBLEBBOX_H

#include <lax/vectors.h>
#include <lax/rectangles.h>

namespace Laxkit {

class DoubleBBox {
 public:
	double minx,maxx,miny,maxy;
	DoubleBBox() { minx=miny=0; maxx=maxy=-1; }
	DoubleBBox(flatpoint p) { minx=maxx=p.x; miny=maxy=p.y; }
	DoubleBBox(double mix,double max,double miy,double may) { minx=mix; maxx=max; miny=miy; maxy=may; }
	DoubleBBox(const DoubleBBox &box) { minx=box.minx; maxx=box.maxx; miny=box.miny; maxy=box.maxy; }
	DoubleBBox(const DoubleRectangle &rect);
	virtual ~DoubleBBox() {}
	virtual void ClearBBox() { minx=miny=0; maxx=maxy=-1; }
	virtual void addtobounds(double x,double y);
	virtual void addtobounds(flatpoint p);
	virtual void addtobounds(DoubleBBox *bbox);
	virtual void addtobounds(const double *m, DoubleBBox *bbox);
	virtual void addtobounds(flatpoint *pts,int n);
	virtual void addtobounds(const DoubleRectangle &rect);
	virtual void setbounds(DoubleBBox *bbox);
	virtual void setbounds(flatpoint *pts,int n);
	virtual void setbounds(double mix,double max,double miy,double may) { minx=mix; maxx=max; miny=miy; maxy=may; }
	virtual void setboundsXYWH(double x,double y,double w,double h) { minx=x; maxx=x+w; miny=y; maxy=y+h; }
	virtual void setbounds(const DoubleRectangle &rect);
	virtual int validbounds() { return maxx>=minx && maxy>=miny; }
	virtual int nonzerobounds() { return maxx>minx && maxy>miny; }
	virtual int intersect(double mix,double max,double miy,double may, int settointersection=0);
	virtual int intersect(DoubleBBox *bbox, int settointersection=0);
	virtual int intersect(const double *m,DoubleBBox *bbox, int touching, int settointersection);
	virtual int IntersectWithLine(const flatline &line, flatpoint *p1_ret, flatpoint *p2_ret, double *i1_ret=nullptr, double *i2_ret=nullptr);
	virtual int boxcontains(double x, double y);
	virtual flatpoint BBoxPoint(double x,double y) const;
	double boxwidth()  const {  return maxx>minx ? maxx-minx : 0; }
	double boxheight() const {  return maxy>miny ? maxy-miny : 0; }
	double MaxDimension() { return boxwidth() > boxheight() ? boxwidth() : boxheight(); }
	double MinDimension() { return boxwidth() < boxheight() ? boxwidth() : boxheight(); }
	virtual double *FitToBox(const DoubleBBox &container, double *m_ret);
	virtual void ShiftBounds(double left, double right, double top, double bottom);
	virtual void ExpandBounds(double amount);
};
	
} // namespace Laxkit

#endif

