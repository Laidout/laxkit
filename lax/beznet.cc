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

#include <lax/bezutils.h>


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {
namespace Bezier {


// Implementation of half edges to define regions.
// Half edges make building faces from edges very easy, and explicitly encodes order of lines around vertices.


//-------------------------- Funcs -------------------------------

BezNetData *DelaunayToNet(VoronoiData *voronoi)
{
	BezNetData *net = new BezNetData();
	net->m(voronoi->m());

	for (int c=0; c<voronoi->points.n; c++) {
		net->AddVertex(voronoi->points.e[c]->p);
	}

	for (int c=0; c<voronoi->triangles.n; c++) {
		***
		//net->AddEdge(net->vertices[voronoi->triangles.e[c].p1], net->vertices[voronoi->triangles.e[c].p1]);
		net->DefineFace(voronoi->triangles.e[c].p1, voronoi->triangles.e[c].p2, voronoi->triangles.e[c].p3);
	}
}


//-------------------------- Vertex -------------------------------

class Vertex
{
public:
	flatpoint p;
	HalfEdge *edge = nullptr; // memory fo edges are assumed to be managed elsewhere.
	Vertex(flatpoint pp, HalfEdge *h) { p = pp; edge = h; }

	int AllEdges(NumStack<HalfEdge> &edges);
};


/*! Return all edges in order around vertex. Return value is the number of edges added to edges.
 * If edges isn't empty, edges will be put on the top of the edges stack.
 */
int Vertex::AllEdges(NumStack<HalfEdge> &edges)
{
	if (!edge) return 0;
	HalfEdge *ee = edge;

	int starti = edges.n;

	do {
		edges.push(ee);
		ee = ee->NextAroundVertex();
	} while (ee && ee != edge);

	if (!ee) { //we hit a discontinuity, we might have initial edges to insert
		ee = ee->PreviousAroundVertex();
		while (ee) {
			edges.push(ee, starti);
			ee = ee->PreviousAroundVertex();
		}
	}
	return edges.n;
}


//-------------------------- HalfEdge -------------------------------

class HalfEdge
{
public:
	// these are all just links to things, the memory is not managed by HalfEdge.
	Vertex *vertex = nullptr; // originating vertex
	HalfEdge *twin = nullptr; // note, unlike "pure" implementations, we always have twins, but twin will have null face if non-manifold
	HalfEdge *next = nullptr;
	HalfEdge *prev = nullptr;
	BezEdge  *edge = nullptr; 
	BezFace  *face = nullptr; // face that's on the "primary" side. *** define primary!!!

	HalfEdge *NextAroundVertex(HalfEdge **twin_ret = nullptr);
	HalfEdge *PreviousAroundVertex(HalfEdge **twin_ret = nullptr);
};

/*! Return the next outgoing halfedge around vertex, counter clockwise.
 * If the next edge is non-manifold (no twin with same vertex), then return this->prev in twin_ret, and return nullptr.
 * Note in that case, twin_ret is just the adjacent edge in the same face.
 */
HalfEdge *HalfEdge::NextAroundVertex(HalfEdge **twin_ret = nullptr)
{
	if (!prev) return nullptr;
	if (!prev->twin) {
		//next edge does not have an edge outgoing from current vertex
		if (twin_ret) *twin_ret = prev;
		return nullptr;
	}
	return prev->twin;
}

HalfEdge *HalfEdge::PreviousAroundVertex()
{
	if (!twin) return nullptr;
	if (twin->next) return twin->next;
	return nullptr;
}


//-------------------------- BezEdge -------------------------------

/*! BezEdge allocates the halfedges used in a BezNetData. BezNetData handles management of BezEdge objects.
 * There can be any number of length of bezier path that corresponds to the half edge, 
 * which are assumed to not intersect with any other BezEdge except at endpoints.
 * coord runs along halfedge, and the reverse of coord runs along twin;
 * If coord == nullptr, then use the points defined in the Vertex objects.
 */
class BezEdge
{
public:
	HalfEdge *halfedge = nullptr;
	HalfEdge *twin = nullptr;
	int tick = 0;

	Coordinate *coord = nullptr; //custom bezier edge. if null, then use the twin's coord, but in reverse order
	
	BezEdge() {}
	virtual ~BezEdge() { if (coord) delete coord; }
};


//-------------------------- BezFace -------------------------------

class BezFace
{
public:
	HalfEdge *halfedge = nullptr; // initial edge for face definition for which this BezFace is the target face for halfedge.

	int tick = 0;
	int info = 0;
	anObject *extra_info = nullptr;

	BezFace();
	virtual ~BezFace()
	{
		if (extra_info) extra_info->dec_count();
	}

	HalfEdge *FirstOuterBoundary(int *index = nullptr);
	HalfEdge *FirstOuterBoundary(int face_a, int op, int face_b);

	//danger zone:
	void ReassignFace(BezFace *new_face);
};

/*! Set the face of each halfedge to new_face (which might be null).
 * Will set halfedge to nullptr.
 */
void BezFace::ReassignFace(BezFace *new_face)
{
	if (!haldedge) return;
	HalfEdge *e = halfedge;
	do {
		e->face = new_face;
		e = e->next;
	} while (e && e != halfedge);
	halfedge = nullptr;
}

/*! Return first edge that is not bound by two faces.
 */
HalfEdge *BezFace::FirstOuterBoundary(int *index)
{
	HalfEdge *edge = halfedge;
	int i = 0;
	do {
		if (edge->twin == nullptr || edge->twin->face == nullptr) {
			if (index) *index = i;
			return edge;
		}
		i++;
		edge = edge->next;

	} while (edge && edge != halfedge);

	return nullptr;
}

/*! Return first edge that is not matched by the op.
 */
HalfEdge *BezFace::FirstBoundary(int face_a, int op, int face_b, bool ignore_ticked_edges)
{
	HalfEdge *edge = halfedge;
	do {
		if (!ignore_ticked_edges || (ignore_ticked_edges && egde->edge->tick == 0)) {
			if (edge->twin == nullptr || edge->twin->face == nullptr) {
				return edge;
			}

			if (!MatchFace(edge->twin->face->info & face_a, op, edge->twin->face->info & face_b)) {
				return edge;
			}
		}
		edge = edge->next;

	} while (edge && edge != halfedge);

	return nullptr;
}


//-------------------------- BezNetData -------------------------------

/*! Class to facilitate net building and path boolean ops.
 */
class BezNetData : virtual public SomeData
{
public:
	PtrStack<Vertex> vertices;
	PtrStack<BezEdge> edges;
	PtrStack<BezFace> faces;

	BezNetData();
	virtual ~BezNetData();
	virtual const char *whattype() { return "BezNetData"; }
	virtual void FindBBox();
	virtual SomeData *duplicate(SomeData *dup);

	int AddPath(PathsData *pdata);

	int RemoveFace(BezFace *face);
	int RemoveEdge(HalfEdge *at_edge);
	int BisectEdge(HalfEdge *at_edge);
	int AddEdge(Vertex *from, Vertex *to);
	int AddVertex(flatpoint p);

	void RemoveDanglingEdges(BezFace *face);

	void InitTick();
};


/*! Set all face->tick to 0. These are used for stepping over all faces without repeating.
 * Note: not thread safe.
 */
void BezNetData::InitTick()
{
	for (int c=0; c<faces.n; c++)
		faces.e[c]->tick = 0;
	for (int c=0; c<edges.n; c++)
		edges.e[c]->tick = 0;
}

void BezNetData::RemoveDanglingEdges(BezFace *face)
{
	if (!face || !face->halfedge) return;

	HalfEdge *h = face->halfedge;
	do {
		if (h->next == h->twin) {
			h->twin->vertex->halfedge = nullptr; //*** need to clean up orphaned vertices
			if (h->prev != h->twin) {
				h->prev->next = h->twin->next;
				h->twin->next->prev = h->prev;
			} else {
				//whole edge was dangling... face was just a loop around a single edge??
				h->vertex->halfedge = nullptr;
				edges.remove(h->edge);
				faces.remove(face);
				return;
			}
			HalfEdge *hh = h->prev;
			edges.remove(h->edge);
			h = hh;

		} else if (h->prev == h->twin) {
			h->vertex->halfedge = nullptr;
			if (h->next != h->twin) {
				h->next->prev = h->twin->prev;
				h->twin->prev->next = h->next;
			} else {
				//whole edge was dangling... face was just a loop around a single edge??
				h->twin->vertex->halfedge = nullptr; //*** need to clean up orphaned vertices
				edges.remove(h->edge);
				faces.remove(face);
				return;	
			}
			HalfEdge *hh = h->prev;
			edges.remove(h->edge);
		}

		h = h->next;
	} while (h != face->halfedge);
}

int BezNetData::RemoveEdge(BezEdge *edge)
{
	return RemoveEdge(edge->halfedge);
}

int BezNetData::RemoveEdge(HalfEdge *at_edge)
{
	if (!at_edge) return 1;

	// bare edge
	//   connected at both vertices
	//   connected at one vertex
	//   stand alone
	// edge with one face
	// edge with two faces
	 
	if (at_edge->face == nullptr && at_edge->twin->face != nullptr) {
		at_edge = at_edge->twin;
	}

	if (at_edge->face) {
		// either we are between two faces, or at the outer boundary of one face
		if (at_edge->twin && at_edge->twin->face) {
			//between two faces, we need to merge the faces
			BezFace *keep_face = at_edge->face;
			at_edge->twin->face->ReassignFace(keep_face);

			//update edge linked lists
			at_edge->next->prev = at_edge->twin->prev;
			at_edge->twin->prev->next = at_edge->next;

			at_edge->prev->next = at_edge->twin->next;
			at_edge->twin->next->prev = at_edge->prev;
			
			if (keep_face->halfedge == at_edge)
				keep_face->halfedge = at_edge->next;

			if (at_edge->vertex->halfedge == at_edge)
				at_edge->vertex->halfedge = at_edge->next;

			edges.remove(edges.findindex(at_edge->edge));

			RemoveDanglingEdges(keep_face);

		} else {
			// a single face on an exterior boundary. must ressign face to null
			***
		}
	} else {
		// edge has no faces
		***
	}

	if (!at_edge->face && !at_edge->twin->face) {
		// raw edge with no attached faces
		*** remove ref from adjacent connections
		edges.remove(edges.findindex(at_edge));
		return 0;
	}



	return 0;
}

/*! Add a point to the vertices list, with assumption that it is not connected to anything.
 */
int BezNetData::AddVertex(flatpoint p)
{
	return vertices.push(new Vertex(p, nullptr));
}

/*! Add regions enclosed by the path, and assign the face_mask to the new regions.
 * Return 0 for success, or nonzero for failure, nothing done.
 */
int BezNetData::AddPath(PathsData *pdata, int face_mask)
{
	if (!pdata || !pdata->paths.n) return 1;

	//detect path self intersections
	***

	
	
	// for each bezier segment:
	//   detect if start point near existing vertex, need to insert edge if so
	//   detect if start point near existing edge, subdivide if so
	//   intersect with existing edges
}

enum class PathOp {
	Noop,
	Union,
	Intersection,
	AMinusB,
	BMinusA,
	Xor
};


/*! face_a and face_b are bit masks.
 */
bool MatchFace(int face_a, int op, int face_b)
{
	switch (op)
	{
		case PathOp::Union:        return (face_a | face_b) != 0;
		case PathOp::Intersection: return (face_a & face_b) != 0;
		case PathOp::AMinusB:      return (face_a & (!face_b)) != 0;
		case PathOp::BMinusA:      return ((!face_a) & face_b) != 0;
		case PathOp::AXorB:        return (face_a ^ face_b) != 0;
	}
	return false;
}

PathsData *BezNetData::ResolveRegion(int face_a, int op, int face_b, PathsData *existing)
{
	int start_face = -1;

	InitTick();

	//First tag any face that matches the boolean op
	for (int c=0; c<faces.n; c++) {
		if (!MatchFace(faces.e[c]->info & face_a, op, faces.e[c]->info & face_b)) continue;

		faces.e[c]->tick = 1; //should be contained one way or another

		//HalfEdge *boundary = faces.e[c]->FirstOuterBoundary(); ***NOT SO! might be occupied by reject face
		HalfEdge *boundary = faces.e[c]->FirstBoundary(face_a, op, face_b, false);

		if (boundary) {
		 	faces.e[c]->tick = 1; //should be contained one way or another
		 	if (start_face < 0) {
				start_face = c;
			}
		}
	}

	if (start_face < 0) return nullptr;

	PathsData *pathsdata = existing;
	if (!pathsdata) pathsdata = dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));


	// for each unticked boundary edge, walk around from it to build path edge
	int num_paths_added = 0;
	for (int c=start_face; c<faces.n; c++) {
		HalfEdge *e = faces.e[c]->FirstBoundary(face_a, op, face_b, true);
		if (e) {
			if (num_paths_added > 0) {
				pathsdata->pushempty();
			}
			num_paths_added++;

			HalfEdge *boundary = e;
			HalfEdge *start_edge = e;

			pathsdata->append(boundary->vertex->p); //first point

			do {
				//curve to boundary
				pathsdata->append(boundary->next->vertex->p); // *** use actual edge curve
				boundary->edge->tick = 1;

				HalfEdge *next = boundary->next;
				while (next->twin->face && next->twin->face->tick == 1) {
					//adjacent face is included, so get next
					next = next->twin->next;
				}
				if (next == start_edge) break;
				boundary = next;

			} while (boundary != start_edge);
		}
	}

	return pathsdata;
}


} //Bezier
} //LaxInterfaces