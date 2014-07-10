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
//    Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef _LAX_DOUBLEBBOX_H
#define _LAX_DOUBLEBBOX_H

#include <lax/vectors.h>

namespace Laxkit {

class DoubleBBox {
 public:
	double minx,maxx,miny,maxy;
	DoubleBBox() { minx=miny=0; maxx=maxy=-1; }
	DoubleBBox(flatpoint p) { minx=maxx=p.x; miny=maxy=p.y; }
	DoubleBBox(double mix,double max,double miy,double may) { minx=mix; maxx=max; miny=miy; maxy=may; }
	virtual ~DoubleBBox() {}
	virtual void clear() { minx=miny=0; maxx=maxy=-1; }
	virtual void addtobounds(double x,double y);
	virtual void addtobounds(flatpoint p);
	virtual void addtobounds(DoubleBBox *bbox);
	virtual void addtobounds(const double *m, DoubleBBox *bbox);
	virtual void setbounds(DoubleBBox *bbox);
	virtual void setbounds(flatpoint *pts,int n);
	virtual void setbounds(double mix,double max,double miy,double may) { minx=mix; maxx=max; miny=miy; maxy=may; }
	virtual int validbounds() { return maxx>=minx && maxy>=miny; }
	virtual int intersect(double mix,double max,double miy,double may, int settointersection=0);
	virtual int intersect(DoubleBBox *bbox, int settointersection=0);
	virtual int intersect(const double *m,DoubleBBox *bbox, int touching, int settointersection);
	virtual int boxcontains(double x, double y);
	virtual flatpoint BBoxPoint(double x,double y);
};
	
} // namespace Laxkit

#endif

