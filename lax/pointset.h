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
#ifndef _LAX_POINTSET_H
#define _LAX_POINTSET_H

#include <lax/lists.h>
#include <lax/vectors.h>
#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/doublebbox.h>
#include <lax/utf8string.h>
#include <lax/laximages.h>

#include <functional>


namespace Laxkit {


//---------------------------------- IndexShape ---------------------------------

class IndexShape
{
  public:
  	int num;
    int *p;
	int *t; //tris on other side of p1-p2, p2-p3, p3-p1

	flatpoint circumcenter;

	IndexShape(int n)
	{
		if (n < 0) n = 0;
		num = n;
		p = t = nullptr;
		if (num) {
			p = new int[num];
			t = new int[num];
		}
	}

	~IndexShape() {
		delete[] p;
		delete[] t;
	}

	int Has(int pp) {
		for (int c=0; c<num; c++) if (pp == p[c]) return c+1;
		return 0;
	}
	int HasCCWEdge(int e1,int e2) {
		for (int c=0; c<num; c++) if (p[c] == e2 && p[(c+1)%num] == e1) return c+1;
		return 0;
	}
	int HasCWEdge(int e1,int e2) {
		for (int c=0; c<num; c++) if (p[c] == e1 && p[(c+1)%num] == e2) return c+1;
		return 0;
	}
	bool operator==(const IndexShape &t) {
		if (num != t.num) return false;
		for (int c=0; c<num; c++) if (p[c] != t.p[c]) return false;
		return true;
	}
};


//---------------------------------- PointCollection ---------------------------------
	
/*! \class PointCollection
 * Base class for objects that can be changed by anything meant to work on masses of points.
 */
class PointCollection
{
  public:
	virtual int Depth() = 0; //how many dimensions wide are points stored
	virtual int NumPoints(int dim) = 0; //size of this dimension
	virtual flatpoint Point(int index, ...) = 0; //retrieve point, as many numbers as Depth()
	virtual flatpoint Point(flatpoint newPos, int index, ...) = 0; //set point

	//Update every point with this function
	virtual int Map(std::function<int(const flatpoint &p, flatpoint &newp)> adjustFunc) = 0; //ret num changed
};


//---------------------------------- class PointSet ---------------------------------


class PointSet : public PointCollection, virtual public anObject, virtual public LaxFiles::DumpUtility
{
  public:
 	class PointObj
 	{
 	  public:
 	  	flatpoint p;
 	  	double weight, radius;
 	  	anObject *info;
 	  	PointObj() { info = nullptr; weight = 1; radius = 1; }
 	  	PointObj(flatpoint pp, anObject *i, bool absorb, double w=1, double r=1) { radius = r; weight = w; p = pp; info = i; if (i && !absorb) i->inc_count(); }
 	  	virtual ~PointObj() { if (info) info->dec_count(); }
 	  	virtual void SetInfo(anObject *i, bool absorb);
 	};

 	virtual PointObj *newPointObj(flatpoint pp, anObject *i, bool absorb, double weight) { return new PointObj(pp,i,absorb,weight); }

	Laxkit::PtrStack<PointObj> points;

	Utf8String name; //user made descriptive name, different than Id()

	PointSet();
	virtual ~PointSet();
	virtual const char *whattype() { return "PointSet"; }
	virtual anObject *duplicate(anObject *ref);
	virtual int CopyFrom(PointSet *set, int with_info, int copy_method);

	// from DumpUtility:
	virtual void dump_out(FILE *f,int indent,int what, LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what, LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context);

	// i/o
	virtual int LoadCSV(const char *file, bool has_headers = true, const char *xcolumn="x", const char *ycolumn="y");
	virtual int Save(const char *file, int format); //0=lines of x,y

	// modifying operations
	virtual PointSet *ConvexHull(PointSet *add_to_this = nullptr);
	virtual void SortX(bool ascending);
	virtual void SortY(bool ascending);

	// info
	virtual int Closest(flatpoint to_this);
	virtual flatpoint Barycenter();
	virtual void GetBBox(DoubleBBox &box);

	// list management
	virtual int Insert(int where, flatpoint p, anObject *data = nullptr, bool absorb = false, double weight = 1);
	virtual int Remove(int index);
	virtual int AddPoint(flatpoint p, anObject *data = nullptr, bool absorb=false, double weight=1);
	virtual flatpoint Pop(int which, anObject **data_ret = nullptr);
	virtual int NumPoints() { return points.n; }
	virtual anObject *PointInfo(int index);
	virtual int SetPointInfo(int index, anObject *data = nullptr, bool absorb=false);
	virtual int SetWeight(int index, double weight);
	virtual double Weight(int index);
	virtual int Radius(int index, double radius);
	virtual double Radius(int index);
	virtual int Swap(int index1, int index2);
	virtual int Slide(int index1, int index2);
	virtual void Flush();

	// generators
	virtual void CreateRandomPoints(int num, int seed, double minx, double maxx, double miny, double maxy);
	virtual void CreateRandomRadial(int num, int seed, double x, double y, double radius);
	virtual void CreateGrid(int numx, int numy, double x, double y, double w, double h, int order);
	virtual void CreateHexChunk(double side, int points_on_side);
	virtual void CreateCircle(int numpoints, double x, double y, double radius);
	virtual void CreatePoissonPoints(double radius, int seed, double minx, double maxx, double miny, double maxy);
	virtual void SamplePoissonPoints(int seed, LaxImage *img, bool invert, double min_radius, double max_radius);

	// operations
	virtual void Relax(int maxiterations, double mindist, double damp, DoubleBBox box);
	virtual void RelaxWeighted(int maxiterations, double weightscale, double damp, flatpoint *boundary, int nboundary);
	virtual void MovePoints(double dx, double dy);
	virtual void Shuffle();

	//from PointCollection:
	virtual int Depth() { return 1; } //how many dimensions wide are points stored
	virtual int NumPoints(int dim) { return dim ? 0 : points.n; } //size of this dimension
	virtual flatpoint Point(int index, ...); //retrieve point, as many numbers as Depth()
	virtual flatpoint Point(flatpoint newPos, int index, ...); //set point
	virtual int Map(std::function<int(const flatpoint &p, flatpoint &newp)> adjustFunc);
};


} //namespace Laxkit


#endif

