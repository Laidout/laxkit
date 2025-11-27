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
	virtual void addtobounds(const flatpoint &p);
	virtual void addtobounds(DoubleBBox *bbox);
	virtual void addtobounds(const double *m, DoubleBBox *bbox);
	virtual void addtobounds(flatpoint *pts,int n);
	virtual void addtobounds(const DoubleRectangle &rect);
	virtual void addtobounds(double min_x, double max_x, double min_y, double max_y);
	virtual void addtobounds_wh(double x, double y, double w, double h);
	virtual void setbounds(DoubleBBox *bbox);
	virtual void setbounds(flatpoint *pts,int n);
	virtual void setbounds(double mix,double max,double miy,double may) { minx=mix; maxx=max; miny=miy; maxy=may; }
	virtual void setboundsXYWH(double x,double y,double w,double h) { minx=x; maxx=x+w; miny=y; maxy=y+h; }
	virtual void setbounds(const DoubleRectangle &rect);
	virtual int validbounds() const { return maxx>=minx && maxy>=miny; }
	virtual int nonzerobounds() { return maxx>minx && maxy>miny; }
	virtual int intersect(double mix,double max,double miy,double may, int settointersection=0);
	virtual int intersect(DoubleBBox *bbox, int settointersection=0);
	virtual bool intersect(const double *m, const DoubleBBox *bbox);
	virtual bool ContainsBox(const double *m, const DoubleBBox *bbox) const;
	virtual int IntersectWithLine(const flatline &line, flatpoint *p1_ret, flatpoint *p2_ret, double *i1_ret=nullptr, double *i2_ret=nullptr);
	virtual int boxcontains(double x, double y) const;
	virtual int boxcontains(const flatpoint &p) const { return boxcontains(p.x, p.y); }
	virtual flatpoint BBoxPoint(double x,double y) const;
	double boxwidth()  const {  return maxx>minx ? maxx-minx : 0; }
	double boxheight() const {  return maxy>miny ? maxy-miny : 0; }
	double MaxDimension() { return boxwidth() > boxheight() ? boxwidth() : boxheight(); }
	double MinDimension() { return boxwidth() < boxheight() ? boxwidth() : boxheight(); }
	virtual double *FitToBox(const DoubleBBox &container, double *m_ret);
	virtual void ShiftBounds(double left, double right, double top, double bottom);
	virtual void ExpandBounds(double amount);
};


class Insets {
  public:
  	double top = 0;
  	double right = 0;
  	double bottom = 0;
  	double left = 0;
  	Insets() {}
  	Insets(double t, double r, double b, double l) { top = t; right = r; bottom = b; left = l; }
  	void ApplyInsets(DoubleBBox &box) const;
  	DoubleBBox InsetBox(const DoubleBBox &box) const;
  	void SetFromBoxDiff(const DoubleBBox &box, const DoubleBBox &inset_box);
  	void Set(double t, double r, double b, double l) { top = t; right = r; bottom = b; left = l; }
};


} // namespace Laxkit

#endif

