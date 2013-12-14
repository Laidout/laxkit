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
#ifndef _LAX_BEZUTILS_H
#define _LAX_BEZUTILS_H

#include <lax/vectors.h>
#include <lax/doublebbox.h>

namespace Laxkit {
	
int bez_bbox(flatpoint p,flatpoint c,flatpoint d,flatpoint q,Laxkit::DoubleBBox *bbox,double *extrema=0);
double bez_segment_length(flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int npoints);

flatpoint bez_point(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2);
flatpoint bez_tangent(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2);
flatpoint bez_visual_tangent(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2);
double bez_distance_to_t(double dist, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int resolution);
double bez_t_to_distance(double T, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int resolution);

flatpoint *bez_to_points(flatpoint *to_points,flatpoint *from_points,int n,int resolution,int closed);
flatpoint *bez_points(flatpoint *to_points,int numsegs,flatpoint *from_points,int resolution);
flatpoint *bez_points(flatpoint *to_points,flatpoint *from_points,int resolution,int ignorefirst);
flatpoint *bez_points(flatpoint *to_points,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2,int resolution,int ignorefirst);
void bez_subdivide(double t,flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, flatpoint *points_ret);

int bez_intersection(flatpoint p1,flatpoint p2, int isline,
					flatpoint bp1, flatpoint bc1, flatpoint bc2, flatpoint bp2,
					int resolution, flatpoint *point_ret, double *t_ret);
int bez_intersections(flatpoint p1,flatpoint p2, int isline, flatpoint *points, int n, int resolution,
					  double startt, flatpoint *points_ret,int np, double *t_ret,int nt, double *endt);
double bez_closest_point(flatpoint p, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, int maxpoints,
						 double *d_ret,double *dalong_ret, flatpoint *found);
double bez_near_point(flatpoint p,flatpoint *points,int n,int maxpoints,double *t_ret,int *i_ret);
double bez_near_point_p(flatpoint p,flatpoint **points,int n,int maxpoints,double *t_ret,int *i_ret);

int point_is_in_bez(flatpoint p,flatpoint *points,int n,int resolution=20);
flatpoint *bez_circle(flatpoint *points, int numpoints, double x,double y,double r);
flatpoint *bez_ellipse(flatpoint *points, int numsegments,
					   double x,double y,
					   double xr,double yr,
					   flatvector xaxis,flatvector yaxis,
					   double start_angle,double end_angle);

} // namespace Laxkit

#endif


