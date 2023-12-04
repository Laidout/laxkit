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
//    Copyright (C) 2023-present by Tom Lechner
//


#include <lax/lists.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/delaunayinterface.h>


namespace LaxInterfaces {


//-------------------------- HalfEdgeVertex -------------------------------

class HalfEdge;
class BezFace;


/*! \class HalfEdgeVertex
 * A vertex point used by HalfEdge and BezNet.
 */
class HalfEdgeVertex
{
public:
	Laxkit::flatpoint p;
	Laxkit::anObject *extra_info = nullptr;
	HalfEdge *halfedge = nullptr; // memory of edges are assumed to be managed in BezNetData::edges.
	HalfEdgeVertex(const Laxkit::flatpoint &pp, HalfEdge *h) { p = pp; halfedge = h; }
	~HalfEdgeVertex() { if (extra_info) extra_info->dec_count(); }

	int AllEdges(Laxkit::PtrStack<HalfEdge> &edges);
};


//-------------------------- HalfEdge -------------------------------

class HalfEdge
{
public:
	// these are all just links to things, the memory is not managed by HalfEdge.
	HalfEdgeVertex *vertex = nullptr; // originating vertex. HalfEdgeVertex object is allocated in BezNetData::vertices
	HalfEdge *twin   = nullptr; // note, unlike "pure" implementations, we always have twins, but twin will have null face if non-manifold
	HalfEdge *next   = nullptr; // HalfEdge objects allocated in BezEdge objects, which are in BezNetData::edges
	HalfEdge *prev   = nullptr;
	HalfEdge *ahead  = nullptr; // A hint that this edge semantically connects forward to this, such as when the net was generated from an overlapping path.
	HalfEdge *behind = nullptr; // A hint that this edge semantically connects backward to this.
	BezFace  *face   = nullptr; // face of this edge. twin will have separate face. only two faces per total edge allowed.
	int tick         = 0;

	HalfEdge *NextAroundVertex(HalfEdge **twin_ret = nullptr);
	HalfEdge *PreviousAroundVertex(HalfEdge **twin_ret = nullptr);
};


//-------------------------- BezFace -------------------------------

class BezFace
{
public:
	HalfEdge *halfedge = nullptr; // link to initial edge for face definition for which this BezFace is the target face for halfedge.

	int tick = 0;
	int info = 0;
	Laxkit::anObject *extra_info = nullptr;

	BezFace() {}
	virtual ~BezFace()
	{
		if (extra_info) extra_info->dec_count();
	}

	HalfEdge *FirstOuterBoundary(int *index);
	HalfEdge *FirstOuterBoundary(int face_a, int op, int face_b, bool ignore_ticked_edges);

	//danger zone:
	void ReassignFace(BezFace *new_face);
};


//-------------------------- BezNetData -------------------------------

enum class PathOp : int {
	Noop  = 0,
	Union,
	Intersection,
	AMinusB,
	BMinusA,
	Xor
};

class BezNetData : virtual public LaxInterfaces::SomeData
{
public:
	// actual allocation for vertices, edges and faces in the net. Internal links in these things, such as the next edge
	// in a face, all point to something allocated here.
	Laxkit::PtrStack<HalfEdgeVertex> vertices;
	Laxkit::PtrStack<HalfEdge> edges;
	Laxkit::PtrStack<BezFace> faces;

	BezNetData();
	virtual ~BezNetData();
	virtual const char *whattype() { return "BezNetData"; }
	virtual void FindBBox();
	virtual SomeData *duplicate(SomeData *dup);
	virtual void dump_in_atts(Laxkit::Attribute *att, int flag, Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);

	int AddPath(PathsData *pdata, int face_mask);

	int RemoveFace(BezFace *face);
	//int RemoveEdge(BezEdge *edge);
	int RemoveEdge(HalfEdge *at_edge);
	int BisectEdge(HalfEdge *at_edge);
	int AddEdge(HalfEdgeVertex *from, HalfEdgeVertex *to);
	int AddVertex(Laxkit::flatpoint p);
	int DefinePolygon(int num_points, ...); //explicit list of indices into existing vertices array
	//int DefinePolygon(Laxkit::PtrStack<Laxkit::flatpoint> &points);
	int DefinePolygon(Laxkit::NumStack<int> &points);

	HalfEdgeVertex *FindClosestVertex(double threshhold); //use 0 to reqiure exact match
	HalfEdge *FindEdge(HalfEdgeVertex *v1, HalfEdgeVertex *v2, int *direction_ret);


	void RemoveDanglingEdges(BezFace *face);

	void InitTick();

	PathsData *ResolveRegion(int face_a, PathOp op, int face_b, PathsData *existing);


	// creation functions:
	static BezNetData *FromDelaunay(VoronoiData *data);
	static BezNetData *FromVoronoi(VoronoiData *data);
	static BezNetData *FromPath(PathsData *data);
};


} // namespace LaxInterfaces
