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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_DELAUNAYINTERFACE_H
#define _LAX_DELAUNAYINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/pointset.h>


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
	flatpoint point; //should correspond to VoronoiData::points[] (defined in PointSet)
	Laxkit::NumStack<int> tris; //indices in VoronoiData::triangles
	int next_hull; //is -1 if this point is not on convex hull
	int pindex; //index of point in original point collection

	VoronoiRegion() { next_hull=-1; pindex=-1; }
	bool operator==(const VoronoiRegion &r) { return r.point==point; }
};

class VoronoiData : virtual public LaxInterfaces::SomeData, virtual public Laxkit::PointSet
{
  public:
	bool show_points;
	bool show_delaunay;
	bool show_voronoi;
	bool show_numbers;
	bool custom_radii; //point radius is per point

	Laxkit::DoubleBBox containing_rect;
	Laxkit::NumStack<IndexTriangle> triangles;
	Laxkit::NumStack<VoronoiRegion> regions; //1 to 1 with points
	Laxkit::NumStack<flatpoint> inf_points; //to help approximate infinite rays

	Laxkit::Color *color_delaunay;
	Laxkit::Color *color_voronoi;
	Laxkit::Color *color_points;
	Laxkit::Color *color_bg;

	double width_delaunay;
	double width_voronoi;
	double width_points;

	VoronoiData();
	virtual ~VoronoiData();
	virtual const char *whattype() { return "VoronoiData"; }
	virtual void FindBBox();
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual SomeData *duplicate(SomeData *dup);

	virtual void Flush();
	virtual void Triangulate();
	virtual void RebuildVoronoi(bool triangulate_also=true);
	virtual void Rebuild() { Triangulate(); RebuildVoronoi(); }

	virtual void Width(double newwidth, int which=-1);

	virtual void RelaxBarycenter(int iters, double strength, Laxkit::DoubleBBox box);
	virtual flatpoint Centroid(int triangle);
	virtual flatpoint CentroidRegion(int point, int *is_inf);
	virtual flatpoint BarycenterRegion(int point, int *is_inf);
	virtual int Map(std::function<int(const flatpoint &p, flatpoint &newp)> adjustFunc);
};
	

//------------------------------- DelaunayInterface ---------------------------------


enum DelaunayInterfaceActions {
	VORONOI_ToggleNumbers,
	VORONOI_ToggleArrows,
	VORONOI_ToggleLines,
	VORONOI_ToggleHull,
	VORONOI_TogglePoints,
	VORONOI_ToggleVoronoi,
	VORONOI_ToggleShapes,
	VORONOI_StyleTarget,
	VORONOI_Thicken,
	VORONOI_Thin,
	VORONOI_FileExport,
	VORONOI_FileImport,
	VORONOI_MakeRandomRect,
	VORONOI_MakeRandomCircle,
	VORONOI_MakePoisson,
	VORONOI_SamplePoisson,
	VORONOI_MakeGrid,
	VORONOI_MakeHexChunk,
	VORONOI_RepeatLast,
	VORONOI_Relax,
	VORONOI_RelaxForce,
	VORONOI_New,
	VORONOI_MAX
};

class DelaunayInterface : public anInterface
{
  protected:
	VoronoiData *data;
	ObjectContext *voc;

	int showdecs; 
	int curpoint;
	bool justadded;
	int style_target;
	char *last_export;
	flatpoint move_pos;
	int previous_create;
	ObjectContext *hover_obj; //for sample from

	enum Mode { MODE_Normal, MODE_SelectObj, MODE_StippleFiddle };
	Mode mode;

	//stipple settings
	double cell_min, cell_max;
	double dot_min, dot_max;
	bool invert_sample;
	double poisson_size;
	int hover_action;

	Laxkit::ShortcutHandler *sc;

	virtual void DropNewData();
	virtual void GetDefaultBBox(Laxkit::DoubleBBox &box);
	virtual void UpdateViewportColor();

  public:
	bool show_numbers;
	bool show_arrows;
	int show_lines; //&1 for voronoi &2 for delaunay
	unsigned int delaunay_interface_style;
	int num_random, num_x, num_y; //inputs for creating funcs
	int relax_iters;

	DelaunayInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~DelaunayInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Delaunay"; }
	virtual const char *Name();
	virtual const char *whattype() { return "DelaunayInterface"; }
	virtual const char *whatdatatype();
	virtual int Event(const Laxkit::EventData *e_data, const char *mes);
    virtual int PerformAction(int action);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);

	virtual ObjectContext *Context();

	virtual void Clear(SomeData *d);
	virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int info);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);

	virtual void Triangulate();
};

} // namespace LaxInterfaces

#endif

