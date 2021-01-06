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
//    Copyright (C) 2020 by Tom Lechner
//

#include <lax/pointset.h>


//template implementation:
#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG


using namespace LaxFiles;


namespace Laxkit {

	
//---------------------------------- class PointSet ---------------------------------


/*! \class PointSet
 * Base class for things that contain numerous points.
 */

PointSet::PointSet()
{
}

PointSet::~PointSet()
{
}

anObject *PointSet::duplicate(anObject *ref)
{
	PointSet *set = new PointSet();
	if (ref) {
		PointSet *r = dynamic_cast<PointSet*>(ref);
		if (r) {
			set->CopyFrom(r, true);
		}
	} else set->CopyFrom(this, true);
	return set;
}

/*! If with_info == 0, then ignore info.
 * If with_info == 1, then link info.
 * If with_info == 2, then duplicate info.
 *
 * Warning: does not flush current points, appends.
 *
 * Returns number of points added.
 */
int PointSet::CopyFrom(PointSet *set, int with_info)
{
	if (!set) return 0;

	int n = 0;
	for (int c=0; c<set->points.n; c++) {
		anObject *info = nullptr;
		if (with_info == 1) { info = set->points.e[c]->info; if (info) info->inc_count(); }
		else if (with_info == 2 && set->points.e[c]->info) info = set->points.e[c]->info->duplicate(nullptr);
		AddPoint(set->points.e[c]->p, info, true);
		n++;
	}
	return n;
}

static int cmp_XAscending(const void *v1,const void *v2)
{
   const PointSet::PointObj *p1 = *((PointSet::PointObj**)v1);
   const PointSet::PointObj *p2 = *((PointSet::PointObj**)v2);

   if (p1->p.x < p2->p.x)
      return -1;
   else if (p1->p.x > p2->p.x)
      return 1;
   
   return 0; 
}

static int cmp_XDescending(const void *v1,const void *v2)
{
   const PointSet::PointObj *p1 = *((PointSet::PointObj**)v1);
   const PointSet::PointObj *p2 = *((PointSet::PointObj**)v2);

   if (p1->p.x < p2->p.x)
      return 1;
   else if (p1->p.x > p2->p.x)
      return -1;
   
   return 0; 
}

static int cmp_YAscending(const void *v1,const void *v2)
{
   const PointSet::PointObj *p1 = *((PointSet::PointObj**)v1);
   const PointSet::PointObj *p2 = *((PointSet::PointObj**)v2);

   if (p1->p.y < p2->p.y)
      return -1;
   else if (p1->p.y > p2->p.y)
      return 1;
   
   return 0; 
}

static int cmp_YDescending(const void *v1,const void *v2)
{
   const PointSet::PointObj *p1 = *((PointSet::PointObj**)v1);
   const PointSet::PointObj *p2 = *((PointSet::PointObj**)v2);

   if (p1->p.y < p2->p.y)
      return 1;
   else if (p1->p.y > p2->p.y)
      return -1;
   
   return 0; 
}

void PointSet::SortX(bool ascending)
{
	qsort(points.e, points.n, sizeof(PointSet::PointObj*), ascending ? cmp_XAscending : cmp_XDescending);
}

void PointSet::SortY(bool ascending)
{
	qsort(points.e, points.n, sizeof(PointSet::PointObj*), ascending ? cmp_YAscending : cmp_YDescending);
}

void PointSet::CreateRandomPoints(int num, int seed, double minx, double maxx, double miny, double maxy)
{
	if (seed) srandom(seed);
	for (int c=0; c<num; c++) {
		AddPoint(flatpoint(minx + (maxx-minx)*(double)random()/RAND_MAX, miny + (maxy-miny)*(double)random()/RAND_MAX));
	}
}

void PointSet::CreateRandomRadial(int num, int seed, double x, double y, double radius)
{
	if (seed) srandom(seed);
	for (int c=0; c<num; c++) {
		flatpoint p(-radius + 2*radius*(double)random()/RAND_MAX, -radius + 2*radius*(double)random()/RAND_MAX); 
		if (p.norm() > radius) {
			c--;
			continue;
		}
		AddPoint(p);
	}
}

void PointSet::CreateGrid(int numx, int numy, double x, double y, double w, double h, int order)
{
	flatpoint dir1, dir2, start;

	int num1, num2;
	flatpoint p;

	switch (order) {
		case LAX_LRBT:
			num1 = numx;
			num2 = numy;
			dir1.set(w/(numx-1), 0);
			dir2.set(0, h/(numy-1));
			start.set(x, y);
			break;
		case LAX_RLTB:
			num1 = numx;
			num2 = numy;
			dir1.set(-w/(numx-1), 0);
			dir2.set(0, -h/(numy-1));
			start.set(x+w, y+h);
			break;
		case LAX_RLBT:
			num1 = numx;
			num2 = numy;
			dir1.set(-w/(numx-1), 0);
			dir2.set(0, h/(numy-1));
			start.set(x+w, y);
			break;
		case LAX_TBLR:
			num1 = numy;
			num2 = numx;
			dir1.set(0, -h/(numy-1));
			dir2.set(w/(numx-1), 0);
			start.set(x, y+h);
			break;
		case LAX_TBRL:
			num1 = numy;
			num2 = numx;
			dir1.set(0, -h/(numy-1));
			dir2.set(-w/(numx-1), 0);
			start.set(x+w, y+h);
			break;
		case LAX_BTLR:
			num1 = numy;
			num2 = numx;
			dir1.set(0, h/(numy-1));
			dir2.set(w/(numx-1), 0);
			start.set(x, y);
			break;
		case LAX_BTRL:
			num1 = numy;
			num2 = numx;
			dir1.set(0, h/(numy-1));
			dir2.set(-w/(numx-1), 0);
			start.set(x+w, y);
			break;
		default: //LAX_LRTB
			num1 = numx;
			num2 = numy;
			dir1.set(w/(numx-1), 0);
			dir2.set(0, -h/(numy-1));
			start.set(x, y+h);
			break;
	}

	for (int c2=0; c2<num2; c2++) {
		p = start + dir2 * c2;
		for (int c1=0; c1<num1; c1++) {
			AddPoint(p);
			p += dir1;
		}
	}
}

/*! Create a single hexagon filled with a triangular grid.
 */
void PointSet::CreateHexChunk(double side, int points_on_side)
{
	// int total_wide = points_on_side*2 - 1;
	double col_width = side * sqrt(3)/2 / (points_on_side-1);
	double row_width = side / (points_on_side-1);
	flatpoint x(1,0), y(0,1);

	// left half
	for (int c=0; c<points_on_side; c++) {
		flatpoint p = (c-(points_on_side-1)) * col_width * x + (points_on_side + c)/2.0*row_width * y;

		for (int c2=0; c2 < points_on_side + c; c2++) {
			AddPoint(p);
			p += -row_width * y;
		}
	}

	// right half
	for (int c=1; c<points_on_side; c++) {
		flatpoint p = c*col_width * x + (2*points_on_side - c - 1)/2.0*row_width * y;

		for (int c2=0; c2 < 2*points_on_side - c -1; c2++) {
			AddPoint(p);
			p += -row_width * y;
		}
	}
}

void PointSet::CreateCircle(int numpoints, double x, double y, double radius)
{
    double xx,yy;
	flatpoint o(x,y), p;

	double theta = 2*M_PI / (numpoints); // radians between control points

    for (int c=0; c<numpoints; c++) {
        xx = radius * cos(c*theta);
        yy = radius * sin(c*theta);
        AddPoint(o + flatpoint(xx,yy));
    }
}

/*! Retrieve point, as many numbers as Depth().
 */
flatpoint PointSet::Point(int index, ...)
{
	if (index < 0 || index >= points.n) return flatpoint();
	return points.e[index]->p;
}

/*! Set point.
 */
flatpoint PointSet::Point(flatpoint newPos, int index, ...)
{
	if (index < 0 || index >= points.n) return flatpoint();
	points.e[index]->p = newPos;
	return newPos;
}

/*! Return number of points actually changed.
 *
 * adjustFunc should return nonzero if point actually gets mapped.
 */
int PointSet::Map(std::function<int(const flatpoint &p, flatpoint &newp)> adjustFunc)
{
	int n = 0;
	flatpoint p;
	for (int c=0; c<points.n; c++) {
		if (adjustFunc(points.e[c]->p, p)) {
			points.e[c]->p = p;
			n++;
		}
	}
	return n;
}


//----------------------------- List Management Funcs -----------------------

int PointSet::Insert(int where, flatpoint p, anObject *data, bool absorb)
{
	return points.push(newPointObj(p,data,absorb), -1, where);
}

int PointSet::AddPoint(flatpoint p, anObject *data, bool absorb)
{
	return points.push(newPointObj(p,data,absorb));
}

int PointSet::Remove(int index)
{
	return points.remove(index);
}

anObject *PointSet::PointInfo(int index)
{
	if (index < 0 || index >= points.n) return nullptr;
	return points.e[index]->info;
}

flatpoint PointSet::Pop(int which, anObject **data_ret)
{
	if (which < 0 || which >= points.n) which = points.n-1;
	if (which < 0) {
		if (data_ret) *data_ret = nullptr;
		return flatpoint();
	}

	flatpoint p = points.e[which]->p;
	if (data_ret) {
		*data_ret = points.e[which]->info;
		if (*data_ret) (*data_ret)->inc_count();
		points.remove(which);
	}

	return p;
}

int PointSet::Swap(int index1, int index2)
{
	if (index1<0 || index1 >= points.n || index2 < 0 || index2 >= points.n) return 1;
	points.swap(index1, index2);
	return 0;
}

int PointSet::Slide(int index1, int index2)
{
	if (index1<0 || index1 >= points.n || index2 < 0 || index2 >= points.n) return 1;
	points.slide(index1, index2);
	return 0;
}

void PointSet::Flush()
{
	points.flush();
}

//--------------------------- Info ---------------------

int PointSet::Closest(flatpoint to_this)
{
	double d=1e+100;
	double dd;
	int i = -1;
	for (int c=0; c<points.n; c++) {
		dd = norm2(points.e[c]->p - to_this);
		if (dd < d) {
			d = dd;
			i = c;
		}
	}
	return i;
}

/*! Return evenly weighted average of all the points.
 */
flatpoint PointSet::Barycenter()
{
	flatpoint p;
	for (int c=0; c<points.n; c++) {
		p += points.e[c]->p;
	}
	if (points.n) p /= points.n;
	return p;
}

void PointSet::GetBBox(DoubleBBox &box)
{
	box.ClearBBox();
	for (int c=0; c<points.n; c++) {
		box.addtobounds(points.e[c]->p);
	}
}

//------------------ Operations ----------------------------

/*! Relax trying to maintain at least mindist between points, but also try to
 * stay contained within original bounding box.
 */
void PointSet::Relax(int maxiterations, double mindist, double damp)
{
	if (points.n < 1) return;

	flatpoint p1,p2, v, force;
	double dd;

	// double ff=1; //force multiplier
	DoubleBBox box;
	GetBBox(box);

	// double wholew = box.maxx - box.minx;
	// double wholeh = box.maxy - box.miny;

	flatpoint pts[4];
	pts[0] = flatpoint(box.minx, box.miny);
	pts[1] = flatpoint(box.maxx, box.miny);
	pts[2] = flatpoint(box.maxx, box.maxy);
	pts[3] = flatpoint(box.minx, box.maxy);
	// flatpoint datac = (pts[0]+pts[2])/2;

	flatpoint cc1,cc2;
	flatpoint d;
	// double w,h; //, w2,h2;
	flatpoint dims[points.n];
	// flatpoint centers[points.n];
	flatpoint forces[points.n];
	// double distbtwn;

	for (int c=0; c<points.n; c++) {
		dims[c].x = dims[c].y = 0;
		// centers[c] = points.e[c]->p;
	}
	
	for (int iterations=0; iterations<maxiterations; iterations++) {
		for (int c=0; c<points.n; c++) forces[c].set(0,0);

		for (int c=0; c<points.n; c++) {
			cc1 = points.e[c]->p;
			// w   = dims[c].x;
			// h   = dims[c].y;
			d.x = d.y = 0;

			force = flatpoint(0, 0);
			// p1    = cc1 - flatpoint(w / 2, h / 2);
			// p2    = cc1 + flatpoint(w / 2, h / 2);


			for (int c2 = c+1; c2<points.n; c2++) {
				force.set(0,0);

				// WidthHeight(selection->e(c2), flatpoint(1, 0), flatpoint(0, 1), &w2, &h2, &cc2);
				cc2 = points.e[c2]->p;
				v   = cc1 - cc2;
				dd  = norm(v);
				v.normalize();
				if (dd < 1e-7) { dd = 1e-7; v.set(1,0); }

				// distbtwn = rect_radius(cc1, w, h, dist) + rect_radius(cc2, w2, h2, dist);
				// distbtwn = dd;
				// if (dd < 2 * (mindist + distbtwn)) force -= ff * v / (dd * dd);
				// if (dd < (mindist + distbtwn)) force -= 3 * (ff * v / (dd * dd));
				
				// apply strong force
				if (dd < mindist) force += (mindist - dd) * damp * v;

				// apply "gravity" force: G m1 m2 / r^2
				if (dd > mindist/2) force += damp * mindist*mindist*mindist *1. / (dd*dd) * v;
				//----
				// double fd = (2 * mindist - dd) * damp;
				// if (fd > 0) force += fd * v;
				
				forces[c]  += force;
				forces[c2] -= force;
			}

			// // go toward original bounding box if outside
			// if (!point_is_in(cc1, pts,4)) {
			// 	// if (cc1.x < box.minx) force += flatpoint(box.minx - cc1.x, 0);
			// 	// else if (cc1.x > box.maxx) force += flatpoint(box.maxx - cc1.x, 0);
			// 	// if (cc1.y < box.miny) force += flatpoint(0, box.miny - cc1.y);
			// 	// else if (cc1.y > box.maxy) force += flatpoint(0, box.maxy - cc1.y);
			// 	//------
			// 	v = datac - cc1;
			// 	forces[c] += v / norm(v) * damp;
			// }

		}

		for (int c=0; c<points.n; c++) {
			points.e[c]->p += forces[c];
		}
	}
}


/*! For each point p, set p->info to the index of next hull point, or -1 if not a hull point.
 */
PointSet *PointSet::ConvexHull(PointSet *add_to_this)
{
	cerr << " *** IMPLEMENT PointSet::ConvexHull()!!!"<<endl;
	return nullptr;
}

int PointSet::LoadCSV(const char *file, bool has_headers, const char *xcolumn, const char *ycolumn)
{
	cerr << " *** IMPLEMENT PointSet::LoadCSV()!!!"<<endl;
	return 1;
}

int PointSet::Save(const char *file, int format) //0=lines of x,y
{
	cerr << " *** IMPLEMENT PointSet::Save()!!!"<<endl;
	return 1;
}

void PointSet::dump_out(FILE *f,int indent,int what,DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f, indent);
}

Attribute *PointSet::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (!att) att = new Attribute();
	if (what == -1) {
		Attribute *att2 = att->pushSubAtt("point", "(1,2)", "Vector, with optional info as subattribute");
		att2->push("info", "DataTypeName", "Subattributes will be dependent on DataTypeName");
		return att;
	}

	Utf8String str;
	for (int c=0; c<points.n; c++) {
		str.Sprintf("(%.10g, %.10g)", points.e[c]->p.x, points.e[c]->p.y);
		Attribute *att2 = att->pushSubAtt("point", str.c_str());
		if (points.e[c]->info) {
			Attribute *att3 = att2->pushSubAtt("info", points.e[c]->info->whattype());
			LaxFiles::DumpUtility *dump = dynamic_cast<DumpUtility*>(points.e[c]->info);
			if (dump) dump->dump_out_atts(att3, what, context);
		}
	}

	return att;
}

void PointSet::dump_in_atts(Attribute *att, int what, DumpContext *context)
{
	char *name, *value;
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name, "point")) {
			flatvector v;
			if (FlatvectorAttribute(value, &v)) {
				Attribute *att2 = att->attributes.e[c]->find("info");
				anObject *info = nullptr;
				if (att2) {
					cerr << " *** need to implement import of random info to PointSet!!!"<<endl;
				}
				AddPoint(v, info, true);
			}
		}
	}
}


} //namespace Laxkit

