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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_DELAUNEYINTERFACE_H
#define _LAX_DELAUNEYINTERFACE_H

#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 



//------------------------------- VoronoiData ---------------------------------

class IndexTriangle
{
  public:
    int p1,p2,p3;
	int t[3]; //tris on other side of p1-p2, p2-p3, p3-p1
	flatpoint circumcenter;

	int Has(int pp) {
		if (p1==pp) return 1;
		if (p2==pp) return 2;
		if (p3==pp) return 3;
		return 0;
	}
	int HasCCWEdge(int e1,int e2) {
		if (p2==e1 && p1==e2) return 1;
		if (p3==e1 && p2==e2) return 2;
		if (p1==e1 && p3==e2) return 3;
		return 0;
	}
	int HasCWEdge(int e1,int e2) {
		if (p1==e1 && p2==e2) return 1;
		if (p2==e1 && p3==e2) return 2;
		if (p3==e1 && p1==e2) return 3;
		return 0;
	}
	bool operator==(const IndexTriangle &t) { return t.p1==p1 && t.p2==p2 && t.p3==p3; }
};

class VoronoiRegion
{
  public:
	flatpoint point;
	Laxkit::NumStack<int> tris; //indices to other VoronoiRegions, point is tri->circumcenter
	int next_hull; //is -1 if this point is not on convex hull
	int pindex; //index of point in original point collection

	VoronoiRegion() { next_hull=-1; pindex=-1; }
	bool operator==(const VoronoiRegion &r) { return r.point==point; }
};

class VoronoiData : virtual public LaxInterfaces::SomeData
{
  public:
	Laxkit::DoubleBBox containing_rect;
	Laxkit::NumStack<flatpoint> points;
	Laxkit::NumStack<IndexTriangle> triangles;
	Laxkit::NumStack<VoronoiRegion> regions; //1 to 1 with points
	Laxkit::NumStack<flatpoint> inf_points; //to help approximate infinite rays

	unsigned int color_delauney;
	unsigned int color_voronoi;
	unsigned int color_points;

	VoronoiData();
	virtual ~VoronoiData();
	virtual const char *whattype() { return "VoronoiData"; }
	virtual void FindBBox();
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);

	virtual void Triangulate();
	virtual void RebuildVoronoi(bool triangulate_also=true);
	virtual void Rebuild() { Triangulate(); RebuildVoronoi(); }
	flatpoint Centroid(int triangle);
};


//------------------------------- DelauneyInterface ---------------------------------


enum DelauneyInterfaceActions {
	VORONOI_ToggleNumbers,
	VORONOI_MAX
};

class DelauneyInterface : public anInterface
{
  protected:
	VoronoiData *data;

	int showdecs; 
	int curpoint;
	bool justadded;

	Laxkit::ShortcutHandler *sc;

  public:
	bool show_numbers;
	unsigned int delauney_interface_style;

	DelauneyInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~DelauneyInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Delauney"; }
	virtual const char *Name();
	virtual const char *whattype() { return "DelauneyInterface"; }
	virtual const char *whatdatatype();
    virtual int PerformAction(int action);
	virtual Laxkit::ShortcutHandler *GetShortcuts();


	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);

	virtual void Triangulate();
};

} // namespace LaxInterfaces

#endif

