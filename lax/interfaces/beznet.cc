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
#include <lax/interfaces/beznet.h>
#include <lax/interfaces/somedatafactory.h>


#include <lax/debug.h>


using namespace Laxkit;


namespace LaxInterfaces {


// Implementation of half edge meshes to define 2D regions bound by bezier segments.
// Half edges make building faces from edges very easy, and explicitly encodes order of lines around vertices.


//-------------------------- Funcs -------------------------------

/*! Static function to convert the Delaunay triangles of data to a BezNetData.
 */
BezNetData *BezNetData::FromDelaunay(VoronoiData *data)
{
	BezNetData *net = new BezNetData();
	net->m(data->m());

	for (int c = 0; c < data->points.n; c++) {
		net->AddVertex(data->points.e[c]->p);
	}

	for (int c = 0; c < data->triangles.n; c++) {
		net->DefinePolygon(3, data->triangles.e[c].p1, data->triangles.e[c].p2, data->triangles.e[c].p3);
	}

	return net;
}


/*! Static function to convert the Voronoi regions of data to a BezNetData.
 */
BezNetData *BezNetData::FromVoronoi(VoronoiData *data)
{
	BezNetData *net = new BezNetData();
	net->m(data->m());

	NumStack<flatpoint> vertices;
	NumStack<flatpoint> points;
	NumStack<flatpoint> pointsi;
	NumStack<int> vertex_indices;

	for (int c = 0; c < data->points.n; c++) {
		data->GetRegionPolygon(c, points);
		for (int c2=0; c2<points.n; c2++) {
			vertices.pushnodup(points.e[c]);
		}
	}

	for (int c = 0; c < data->points.n; c++) {
		data->GetRegionPolygon(c, points);
		for (int c2=0; c2<points.n; c2++) {
			int index = vertices.findindex(points.e[c]);
			vertex_indices.push(index, 0);
		}
		net->DefinePolygon(vertex_indices);
	}

	return net;
}


/*! Face info matching. face_a and face_b are bit masks.
 */
bool MatchFace(int face_a, int op, int face_b)
{
	switch ((PathOp)op)
	{
		case PathOp::Union:        return (face_a | face_b)    != 0;
		case PathOp::Intersection: return (face_a & face_b)    != 0;
		case PathOp::AMinusB:      return (face_a & (!face_b)) != 0;
		case PathOp::BMinusA:      return ((!face_a) & face_b) != 0;
		case PathOp::Xor:          return (face_a ^ face_b)    != 0;
		case PathOp::Noop:         return face_a               != 0;
	}
	return false;
}


//-------------------------- HalfEdgeVertex -------------------------------


/*! \class HalfEdgeVertex
 * A vertex point used by HalfEdge and BezNet.
 */


/*! Return all edges in order around vertex. Return value is the number of edges added to edges.
 * If edges isn't empty, edges will be put on the top of the edges stack.
 */
int HalfEdgeVertex::AllEdges(PtrStack<HalfEdge> &edges)
{
	if (!halfedge) return 0;
	HalfEdge *ee = halfedge;

	int starti = edges.n;

	do {
		edges.push(ee, 0);
		ee = ee->NextAroundVertex();
	} while (ee && ee != halfedge);

	if (!ee) { //we hit a discontinuity, we might have initial edges to insert
		ee = ee->PreviousAroundVertex();
		while (ee) {
			edges.push(ee, 0, starti);
			ee = ee->PreviousAroundVertex();
		}
	}
	return edges.n;
}


//-------------------------- HalfEdge -------------------------------

/*! \class HalfEdge
 * Each half edge has a twin that is the opposite side of the edge.
 * next and prev are edges for the primary polygon this halfedge belongs to.
 *
 * Note this assumes edges can only have two faces attached to it, and that vertices are surrounded by
 * faces that are orientable in a 2d circular direction.
 */


/*! Return the next outgoing halfedge around vertex, counter clockwise.
 * If the adjacent edge around the vertex is non-manifold (no twin with same vertex), then return this->prev in twin_ret, and return nullptr.
 * Note in that case, twin_ret is just the adjacent edge in the same face.
 */
HalfEdge *HalfEdge::NextAroundVertex(HalfEdge **twin_ret)
{
	if (twin_ret) *twin_ret = nullptr;
	if (!prev) {
		return nullptr;
	}
	if (!prev->twin) {
		//next edge does not have an edge outgoing from current vertex
		if (twin_ret) *twin_ret = prev;
		return nullptr;
	}
	return prev->twin;
}

HalfEdge *HalfEdge::PreviousAroundVertex(HalfEdge **twin_ret)
{
	if (twin_ret) *twin_ret = nullptr;
	
	if (!twin) return nullptr;
	if (twin->next) return twin->next;
	return nullptr;
}


//-------------------------- BezEdge -------------------------------

/*! \class BezEdge
 * BezEdge allocates the halfedges used in a BezNetData. BezNetData handles management of BezEdge objects.
 * There can be any number of segments of bezier path that corresponds to the half edge, 
 * which are assumed to not intersect with any other BezEdge except at endpoints.
 * coord runs along halfedge, and the reverse of coord runs along twin;
 * If coord == nullptr, then use the points defined in the HalfEdgeVertex objects.
 */


//-------------------------- BezFace -------------------------------

/*! \class BezFace
 * Class using HalfEdge to define regions bound by non-intersecting bezier segments.
 * Networks of these comprise a BezNetData.
 */


/*! Set the face of each halfedge to new_face (which might be null).
 * Will set halfedge to nullptr.
 */
void BezFace::ReassignFace(BezFace *new_face)
{
	if (!halfedge) return;
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
HalfEdge *BezFace::FirstOuterBoundary(int face_a, int op, int face_b, bool ignore_ticked_edges)
{
	HalfEdge *edge = halfedge;
	do {
		if (!ignore_ticked_edges || (ignore_ticked_edges && edge->edge->tick == 0)) {
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

BezNetData::BezNetData()
{}

BezNetData::~BezNetData()
{}

void BezNetData::FindBBox()
{
	//FIXME! ***
	minx = miny = 0;
	maxx = maxy = 2;
}

SomeData *BezNetData::duplicate(SomeData *dup)
{
	//FIXME! ***
	//BezNetData *obj = dynamic_cast<BezNetData*>(somedatafactory()->NewObject(LAX_BEZNETDATA));
	return new BezNetData();
}

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

/*! Step through edges of face, and if the edge has a vertex with only one edge on it, remove the edge.
 */
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
			//HalfEdge *hh = h->prev; **** FIXME
			edges.remove(h->edge);
		}

		h = h->next;
	} while (h != face->halfedge);
}


/*! Remove edge from being referenced by anything inside this BezNetData.
 * This involves finding and removing the two halfedges belonging to edge,
 * and merging faces that made the edge.
 *
 * Return 0 for success, or nonzero for some kind of error.
 */
int BezNetData::RemoveEdge(BezEdge *edge)
{
	return RemoveEdge(edge->halfedge);
}


/*! Return 0 for success, or nonzero for some kind of error.
 */
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
			// ***
		}
	} else {
		// edge has no faces
		// ***
	}

	if (!at_edge->face && !at_edge->twin->face) {
		// raw edge with no attached faces
		//*** remove ref from adjacent connections
		//edges.remove(edges.findindex(at_edge));
		return 0;
	}



	return 0;
}


/*! Add a point to the vertices list, with assumption that it is not connected to anything.
 */
int BezNetData::AddVertex(flatpoint p)
{
	return vertices.push(new HalfEdgeVertex(p, nullptr));
}


/*! Add regions enclosed by the path, and assign the face_mask to the new regions.
 * Return 0 for success, or nonzero for failure, nothing done.
 */
int BezNetData::AddPath(PathsData *pdata, int face_mask)
{
	if (!pdata || !pdata->paths.n) return 1;

	// //detect path individual segment self intersections
	// ***

	// // detect self intersections between path segments
	// ***
	
	
	// // for each bezier segment:
	// //   detect if start point near existing vertex, need to insert edge if so
	// //   detect if start point near existing edge, subdivide if so
	// //   intersect with existing edges
	   
	return 1;
}


/*! Define a new face by referencing previously defined vertices.
 * Return 0 for success or nonzero for error.
 */
int BezNetData::DefinePolygon(int num_points, ...)
{
	int err = 0;
	NumStack<int> pts;
	va_list argptr;
    va_start(argptr, num_points);
    for (int c=0; c<num_points; c++) {
    	int index = va_arg(argptr, int);
    	if (index < 0 || index >= vertices.n) {
    		DBGE("bad beznet index def");
    		err = 1;
    		break;
    	}
    	pts.push(index);
    }
    va_end(argptr);

    if (err != 0) return err;
    if (pts.n < 3) {
    	return 2;
    }

    return DefinePolygon(pts);
}


BezEdge *BezNetData::FindEdge(HalfEdgeVertex *v1, HalfEdgeVertex *v2, int *dir)
{
	for (int c=0; c<edges.n; c++) {
		BezEdge *edge = edges.e[c];
		if (edge->halfedge) {
			if (edge->halfedge->vertex == v1) {
				if (edge->twin) {
					if (edge->twin->vertex == v2) {
						if (dir) *dir = 1;
						return edge;
					}
				} else if (edge->halfedge->next) {
					if (edge->halfedge->next->vertex == v2) {
						if (dir) *dir = 1;
						return edge;
					}
				}
			} else if (edge->halfedge->vertex == v2) {
				if (edge->twin) {
					if (edge->twin->vertex == v1) {
						if (dir) *dir = -1;
						return edge;
					}
				} else if (edge->halfedge->next) {
					if (edge->halfedge->next->vertex == v1) {
						if (dir) *dir = -1;
						return edge;
					}
				}
			}
		} else if (edge->twin) {
			if (edge->twin->vertex == v1) {
				if (edge->twin->next && edge->twin->next->vertex == v2) {
					if (dir) *dir = -1;
					return edge;
				}
			} else if (edge->twin->vertex == v2) {
				if (edge->twin->next && edge->twin->next->vertex == v1) {
					if (dir) *dir = 1;
					return edge;
				}
			}
		}
	}

	return nullptr;
}


/*! Define a new region based on 3 or more already existing vertices.
 */
int BezNetData::DefinePolygon(Laxkit::NumStack<int> &points)
{
	if (points.n < 3) return 1;

	HalfEdge *first = nullptr;
	HalfEdge *previous = nullptr;

	//todo: *** check winding, we want all faces CCW

	BezFace *face = new BezFace();

    for (int c=0; c<points.n; c++) {
    	int dir = 0;
    	HalfEdgeVertex *v1 = vertices.e[points[c]];
    	HalfEdgeVertex *v2 = vertices.e[points[(c+1)%points.n]];

    	BezEdge *edge = FindEdge(v1, v2, &dir);

    	// edge will be one of:
    	// - null: need to create a new edge, attach to vertices
    	// - half: presumably we will add face to the empty half
    	// - full: bad! trying to add to occupied edge!
    	if (!edge || (edge->halfedge == nullptr && edge->twin == nullptr)) {
    		//we had either empty edge or no edge
    		if (!edge) {
    			edge = new BezEdge();
    			edges.push(edge);
    		}

    		//now we have an empty edge, need to define things on it
    		edge->halfedge = new HalfEdge();
    		edge->halfedge->edge = edge;
    		edge->halfedge->face = face;
    		edge->halfedge->vertex = v1;

    		if (previous) edge->halfedge->prev = previous;
    		if (c == points.n-1) {
    			edge->halfedge->next = first;
    			first->prev = edge->halfedge;
    		}

    		previous = edge->halfedge;
    		if (c == 0) first = edge->halfedge;

    	} else if (edge->halfedge != nullptr && edge->twin == nullptr)  {
    		//assume we are filling in the opposite edge.. ***should probably sanity check way more in here
    		edge->twin = new HalfEdge();
    		edge->halfedge->twin = edge->twin;
    		edge->twin->twin = edge->halfedge;
    		edge->twin->edge = edge;
    		edge->twin->face = face;
    		edge->twin->vertex = v2;
    		edge->twin->prev = previous;
    		if (c == points.n-1) {
    			edge->twin->next = first;
    			first->prev = edge->twin;
    		}
    		previous = edge->twin;
    		if (c == 0) first = edge->twin;

    	} else if (edge->halfedge == nullptr && edge->twin != nullptr)  {
    		//assume we are filling in the opposite edge.. ***should probably sanity check way more in here
    		edge->halfedge = new HalfEdge();
    		edge->twin->twin = edge->halfedge;
    		edge->halfedge->twin = edge->twin;
    		edge->halfedge->edge = edge;
    		edge->halfedge->face = face;
    		edge->halfedge->vertex = v1;
    		edge->halfedge->prev = previous;
    		if (c == points.n-1) {
    			edge->twin->next = first;
    			first->prev = edge->twin;
    		}
    		previous = edge->halfedge;
			if (c == 0) first = edge->halfedge;

    	} else if (edge->halfedge != nullptr && edge->twin != nullptr)  {
    		DBGE("Trying to add to full edge, ahhh!");
    		//*** clean up how!?!
    		delete face;
    		return 1;

    	}
    }

	faces.push(face);
    return 0;
}


PathsData *BezNetData::ResolveRegion(int face_a, PathOp op, int face_b, PathsData *existing)
{
	int start_face = -1;

	InitTick();

	//First tag any face that matches the boolean op
	for (int c=0; c<faces.n; c++) {
		if (!MatchFace(faces.e[c]->info & face_a, (int)op, faces.e[c]->info & face_b)) continue;

		faces.e[c]->tick = 1; //should be contained one way or another

		//HalfEdge *boundary = faces.e[c]->FirstOuterBoundary(); ***NOT SO! might be occupied by reject face
		HalfEdge *boundary = faces.e[c]->FirstOuterBoundary(face_a, (int)op, face_b, false);

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
		HalfEdge *e = faces.e[c]->FirstOuterBoundary(face_a, (int)op, face_b, true);
		if (e) {
			if (num_paths_added > 0) {
				pathsdata->pushEmpty();
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


} //LaxInterfaces
