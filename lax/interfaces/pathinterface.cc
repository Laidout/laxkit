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


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/svgcoord.h>
#include <lax/laxutils.h>
#include <lax/bezutils.h>
#include <lax/transformmath.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG

DBG flatpoint POINT;

//square of screen pixel distance for a point to be close enough to a point to select it
#define SELECTRADIUS2 25
#define SELECTRADIUS   5

#define DIRSELECTRADIUS 35


//for use in selectPoint():
#define PSELECT_FlushPoints  (1<<0)
#define PSELECT_PushPoints   (1<<1)
#define PSELECT_SelectPathop (1<<2)
#define PSELECT_SyncVertex   (1<<3)

enum PathHoverType {
	HOVER_None=0,
	HOVER_Point,
	HOVER_Vertex,
	HOVER_AddingPoint,
	HOVER_Handle,
	HOVER_AddPoint,
	HOVER_Endpoint,
	HOVER_MergeEndpoints,
	HOVER_AddSegment,
	HOVER_CutSegment,
	HOVER_Direction,
	HOVER_DirectionSelect,
	HOVER_AddToSelection,
	HOVER_RemoveFromSelection,
	HOVER_MAX
};


namespace LaxInterfaces {


//------------------------------------- PathUndo -------------------------------------

enum PathUndoTypes {
	PATHUNDO_Move_Point,
	PATHUNDO_Add_Point,
	PATHUNDO_Delete_Point,
	PATHUNDO_Point_Type_Change,
	PATHUNDO_Width_Change,
	PATHUNDO_Width1_Change,
	PATHUNDO_Width2_Change,

	PATHUNDO_Add_Path,
	PATHUNDO_Delete_Path,

	PATHUNDO_LineStyle_Change,
	PATHUNDO_FillStyle_Change,
	PATHUNDO_MAX
};


//-------------------- PathInfo ---------------------------

/*! \class PathInfo
 * Styling info about a path, such as width information.
 * These can be positioned anywhere along a path.
 */
class PathInfo
{
  public:
	int pathi; //!< path index
	double pt; //!< position in path
	
	double w1;
	double w2;

	PathInfo *prev, *next;
};


//------------------------------------- Path -------------------------------------

/*! \class Path
 * \ingroup interfaces
 * \brief Path contains only one path.
 * \code #include <lax/interfaces/pathinterface.h> \endcode
 *
 *  Paths can use any PathOperator object found in basepathops to aid in constructing
 *  and maintaining path data. Straight lines and bezier lines are handled without
 *  any special PathOperators.
 *
 *  Path class exists just to contain a single (connected) path with its own linestyle.
 *  PathsData holds objects with multiple paths.
 */
/*! \var PtrStack<PathOperator> Path::basepathops
 * 	\brief A reference pool of PathOperator classes.
 *
 * 	This static pool is necessary to allow Path or PathsData, for instance, to figure out their
 * 	actual bounds and to dump_out properly, independently of any particular PathInterface
 * 	or PathOperator instance.
 */

PtrStack<PathOperator> Path::basepathops;

Path::Path()
  : path(NULL), linestyle(NULL)
{}

/*! Incs count of linestyle.
 */
Path::Path(Coordinate *np,LineStyle *nls)
{
	path=np;
	linestyle=nls;
	if (linestyle) linestyle->inc_count();
}

//! Destructor always deletes path.
/*! Dec_count of linestyle.
 */
Path::~Path()
{
	delete path;
	if (linestyle) linestyle->dec_count();
}

//! Flush all points.
void Path::clear()
{
	delete path;
	path=NULL;
}

Path *Path::duplicate()
{
	Path *newpath=new Path(NULL,linestyle);
	if (path) newpath->path=path->duplicateAll();

	return newpath;
}

//! Dump out path contents.
/*! If what==-1, then dump out pseudocode mockup of format.
 *
 * Dumps out something like:
 * <pre>
 *    linestyle
 *      ...     #standard linestyle attributes
 *    closed    #flag to indicate that this path is a closed path
 *    points \
 *      1 2                   #a vertex point
 *      p 1.5 2.5             #a control point of the previous vertex
 *      n 3 5                 #a control point of the next vertex
 *    segment controllername  #a non-straight-line and non-bezier segment
 *      asbezier p 3 5 n 2 4 5 6  # Important! bezier approximation of the segment.
 *                                # if not present and the controller cannot be found,
 *                                # then this segment is ignored.
 *      ...                   #any further attributes defining the segment. exactly what
 *                            #they are is dependent of the actual controller of the segment.
 *                            #If for some reason the segment controller is not found, then
 *                            #the bezier approximation is appended to the path as bezier points
 * </pre>
 */
void Path::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%slinestyle\n",spc);
		fprintf(f,"%s  ...      #standard linestyle attributes\n",spc);
		fprintf(f,"%sclosed     #flag to indicate that this path is a closed path\n",spc);
		fprintf(f,"%spoints \\\n",spc);
		fprintf(f,"%s  1 2                   #a vertex point with corner controls\n",spc);
		fprintf(f,"%s  vs 1 2                #a vertex point with smooth controls\n",spc);
		fprintf(f,"%s  vS 1 2                #a vertex point with really smooth controls\n",spc);
		fprintf(f,"%s  vc 1 2                #a vertex point with corner controls\n",spc);
		fprintf(f,"%s  ve 1 2                #a vertex point with corner equal controls\n",spc);
		fprintf(f,"%s  p 1.5 2.5             #a bezier control point of the previous vertex\n",spc);
		fprintf(f,"%s  n 3 5                 #a bezier control point of the next vertex\n",spc);
		fprintf(f,"%ssegment controllername  #a non-straight-line and non-bezier segment\n",spc);
		fprintf(f,"%s  asbezier p 3 5 n 2 4 5 6  # Important! bezier approximation of the segment.\n",spc);
		fprintf(f,"%s                            # if not present and the controller cannot be found,\n",spc);
		fprintf(f,"%s                            # then this segment is ignored.\n",spc);
		fprintf(f,"%s  ...                   #any further attributes defining the segment. exactly what\n",spc);
		fprintf(f,"%s                        #they are is dependent of the actual controller of the segment.\n",spc);
		fprintf(f,"%s                        #If for some reason the segment controller is not found, then\n",spc);
		fprintf(f,"%s                        #the bezier approximation is appended to the path as bezier points\n",spc);
		return;
	}

	if (!path) return;

	if (linestyle) {
		fprintf(f,"%slinestyle\n",spc);
		linestyle->dump_out(f,indent+2,what, context);
	}
	if (path==path->lastPoint(0)) fprintf(f,"%sclosed\n",spc);

	Coordinate *p=path, *p2=NULL;
	int n;
	do {
		 //do a string of bezier points
		p2=p;
		n=0;
		while (!p2->controls) { p2=p2->next; n++; if (!p2 || p2==path) break; }
		 //now p2 points to the point just after the last uncontrolled vertex

		if (n>0) {
			 //output straight line segments for a seemingly long string of vertex points
			fprintf(f,"%spoints \\\n",spc);
			do {
				if (p->flags&POINT_VERTEX) {
					int pt=p->flags&BEZ_MASK;
					if (pt==BEZ_STIFF_EQUAL) pt='S';
					else if (pt==BEZ_STIFF_NEQUAL) pt='s';
					else if (pt==BEZ_NSTIFF_EQUAL) pt='e';
					else if (pt==BEZ_NSTIFF_NEQUAL) pt='c';
					else pt=0;
					if (pt) fprintf(f,"%s  v%c %.10g %.10g\n",spc, pt,p->x(),p->y());
					else fprintf(f,"%s  %.10g %.10g\n",spc, p->x(),p->y());
				} else if (p->flags&POINT_TOPREV) fprintf(f,"%s  p %.10g %.10g\n",spc, p->x(),p->y());
				else if (p->flags&POINT_TONEXT) fprintf(f,"%s  n %.10g %.10g\n",spc, p->x(),p->y());
				p=p->next;
			} while (p!=p2);
			continue;
		}

		 //check for controlled points
		if (p->controls) {
			p2=p;
			while (p2->controls==p->controls) { p2=p2->next; if (!p2 || p2==path) break; }
		 	 //now p2 points to the point just after the last point with same controller

			int c2;
			for (c2=0; c2<basepathops.n; c2++) if (p2->controls->iid()==basepathops.e[c2]->id) break;
			if (c2==basepathops.n) {
				 //ignore if pathop not found during runtime!! should never happen anyway
				p=p2;
				continue;
			}
			fprintf(f,"%ssegment %s\n",spc,basepathops.e[c2]->whattype());
			basepathops.e[c2]->dumpOut(f, indent+2, p->controls, 0, context);
			p=p2;
		}


	} while (p && p!=path);
}

/*! Dump in the linestyle and coordinates..
 *
 * If a linestyle is listed, then dec_count of old and install a new one.
 *
 * See dump_out() for format expected.
 */
void Path::dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	int c2,closed=0;
	if (path) { delete path; path=NULL; }

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"linestyle")) {
			if (linestyle) linestyle->dec_count();
			linestyle=new LineStyle;
			linestyle->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"closed")) {
			closed=1;

		} else if (!strcmp(name,"points")) {
			appendBezFromStr(value);

		} else if (!strcmp(name,"segment")) {
			if (isblank(value)) continue;

			for (c2=0; c2<basepathops.n; c2++) if (!strcmp(value,basepathops.e[c]->whattype())) break;
			if (c2==basepathops.n) {
				Attribute *a=att->attributes.e[c]->find("asbezier");
				if (a) appendBezFromStr(a->value);
				continue;
			}
			basepathops.e[c2]->dumpIn(path->lastPoint(0), att->attributes.e[c], context);
		}
	}
	if (closed && path) path->close();
}

//! Append bezier points taken from the string.
/*! The string must be something like "1 2 p 3 4 n 5 6 7 8". Any pair of numbers
 * not prefaced by a 'n' or 'p' is a vertex point. A 'n' indicates it is a
 * control point for the following point (which must be a vertex). A 'p' is for
 * control points for a previous vertex point. Any violation of these terms stops
 * the appending of points.
 *
 * On path vertices may be tagged with 'v', and an optional point type indicator,
 * one of 'c','s','S', or 'e'.
 */
void Path::appendBezFromStr(const char *value)
{
	double d[2];

	const char *s=value;
	char *ptr=NULL;
	int type=0,lasttype=0;
	int c2;
	int pointtype;
	do {
		while (isspace(*s)) s++;

		type=0;
		pointtype=0;
		if (*s=='p') { type=1; s++; }
		else if (*s=='n') { type=2; s++; }
		else if (*s=='v') {
			type=0; s++;
			if (*s=='S') { pointtype=BEZ_STIFF_EQUAL; s++; }
			else if (*s=='s') { pointtype=BEZ_STIFF_NEQUAL; s++; }
			else if (*s=='e') { pointtype=BEZ_NSTIFF_EQUAL; s++; }
			else if (*s=='c') { pointtype=BEZ_NSTIFF_NEQUAL; s++; }
			else pointtype=0;
		}

		while (isspace(*s)) s++;
		c2=DoubleListAttribute(s,d,2,&ptr);
		if (c2!=2) break;
		s=ptr;

		if (lasttype==2 && type!=0) break; //tonext not to a vertex!!
		if (lasttype==1 && type==1) break; //2 toprev in a row!!

		if (type==0) append(d[0],d[1], POINT_VERTEX|pointtype,NULL);
		else if (type==1) append(d[0],d[1], POINT_TOPREV,NULL);
		else if (type==2) append(d[0],d[1], POINT_TONEXT,NULL);

		lasttype=type;
	} while (s && *s);
}

//! Make sure that path points to the first vertex point of the line, if possible.
void Path::fixpath()
{
	if (!path) return;
	path=path->firstPoint(1); // note that firstPoint doesn't return NULL,
							// but it might return a non-vertex
}

//! Return the last point (vertex or control) that occurs at the end, or just before path.
/*! If v!=0, then return the last vertex, rather than the last of any kind of point.
 */
Coordinate *Path::lastPoint(int v)
{
	if (!path) return NULL;
	return path->lastPoint(v);
}

void Path::append(double x,double y,unsigned long flags,SegmentControls *ctl)
{
	if (path==NULL) {
		path=new Coordinate(x,y,flags,ctl);
		return;
	}
	path->append(x,y,flags,ctl);
}

/*! ctl count will be incremented.
 */
void Path::append(flatpoint p,unsigned long flags,SegmentControls *ctl)
{
	append(p.x,p.y,flags,ctl);
}

//! The path takes possession of coord. Calling code should not delete it.
void Path::append(Coordinate *coord)
{
	if (!path) path=coord;
	else {
		Coordinate *p=path->lastPoint(0);
		p->connect(coord,1);
	}
}

//! Make sure the path is closed.
void Path::close()
{
	if (!path) return;
	path->close();
}

//! Find the intersection(s) of the segment (or infinite line) from p1 to p2 and the curve.
int Path::Intersect(flatpoint P1,flatpoint P2, int isline, double startt, flatpoint *pts,int ptsn, double *t,int tn)
{
	if (ptsn<=0 && tn<=0) return 0;
	int num=0;

	NumStack<flatpoint> points;

	Coordinate *start=path->firstPoint(1);
	if (!start) return 0;
	Coordinate *p=start,*p2;

	points.push(p->p()); //add initial vertex
	do { //one iteration for each segment (a vertex to next vertex)
		p2=p->next;
		if (!p2) break;

		 //add c1
		if (p2->flags&POINT_TOPREV) {
			points.push(p2->p());
			p=p2;
		} else {
			points.push(p->p());
		}
		p=p->next;
		if (!p) break;

		 //add c2
		if (p->flags&POINT_TONEXT) {
			points.push(p->p());
			p=p->next;
			if (!p) break;
		} else {
			points.push(p->p());
		}

		 //add vertex
		points.push(p->p());

	} while (p!=start);
	
	num=bez_intersections(P1,P2,isline, points.e,points.n, 30, startt,pts,ptsn, t,tn,NULL);

	return num;
}

/*! t is measured from path->firstVertex(1), and thus should be positive.
 *
 * Returns 1 for point found, -1 for point clamped to beginning point, -2 clamped to end,
 * or 0 if t is not on the line, path doesn't exist.
 *
 * \todo be able to wind around the path if continuous, or go backwards (for negative t)
 * \todo must implement find tangent at clamped endpoints
 * \todo *** note that if you have a loop of null points, and you are going along based
 *   on distance, than this will not break!!
 */
int Path::PointAlongPath(double t, int tisdistance, flatpoint *point, flatpoint *tangent)
{
	if (!path) return 0;

	Coordinate *start=path->firstPoint(1);
	if (!start) return 0;
	Coordinate *p=start, *c1, *c2, *v2;
	double dd;
	flatpoint fp;

	if (t<=0) {
		 //currently always clamp to start point
		 // *** need to implement negative t!!
		if (point) *point=start->p();
		if (tangent) {
			if (start->next && start->next->flags&POINT_TOPREV) *tangent=start->next->p()-start->p();
			else *tangent=start->direction(1);
		}
		return -1;
	}

	do { //one iteration for each segment (a vertex to next vertex)
		c1=c2=v2=NULL;
		v2=p->next;
		if (!v2) break;

		if (v2->flags&POINT_TOPREV) { c1=v2; v2=v2->next; if (!v2) break; }
		else { c1=p;  }

		if (v2->flags&POINT_TONEXT) { c2=v2; v2=v2->next; if (!v2) break; }
		else { c2=v2; }

		if (c1==p && c2==v2) {
			 //we have the simpler case of a line segment
			fp=v2->p()-p->p(); //current segment
			if (tisdistance) {
				dd=norm(fp);
				if (t>dd) { t-=dd; p=v2; continue; } //not on this segment!

				if (point) *point=(dd ? p->p()+t*fp/dd : p->p());
				if (tangent) *tangent=fp;
				return 1;

			} else {
				 //easy to check when using t parameter
				if (t>1) { p=v2; t-=1; continue; }
				
				if (point) *point=p->p()+t*fp;
				if (tangent) *tangent=fp;
				return 1;
			}

		} else {
			 //must deal with a bezier segment
			double tt=0;
			if (tisdistance) {
				dd=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), 50);
				if (t>dd) { t-=dd; p=v2; continue; } //not on this segment!
				tt=bez_distance_to_t(t, p->p(),c1->p(),c2->p(),v2->p(), 50);
			} else {
				 //easy to check when using t parameter
				if (t>1) { p=v2; t-=1; continue; }
				tt=t;
			}

			if (point) *point=bez_point(tt, p->p(),c1->p(),c2->p(),v2->p());
			if (tangent) {
				*tangent=bez_point(tt+.01, p->p(),c1->p(),c2->p(),v2->p())
				        -bez_point(tt, p->p(),c1->p(),c2->p(),v2->p());
			}
			return 1;
		}
	} while (1);

	 //if we broke out of the loop, then we hit the endpoint, so clamp to it
	p=start->lastPoint(1);
	if (point) *point=p->p();
	if (tangent) {
		*tangent=p->direction(1);
	}
	return -1;
}

//! Find the point on any of the paths closests to point.
/*! point is assumed to already be in data coordinates.
 * Returns the distance between those, and the t parameter to that path point.
 */
flatpoint Path::ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist)
{
	if (!path) return flatpoint();
	Coordinate *start=path->firstPoint(1);
	if (!start) return flatpoint();


	// *** note this assumes start and path refer to the same vertex!

	double dd=0, d=10000000; //distance to path
	double vdd=0, vd=0;  //visual distance: running total, and for found
	double tt=0, t=0; //t distance: running total, and for found
	flatpoint found, ff;
	Coordinate *p=start, *c1, *c2, *v2;

	do { //one iteration for each segment (a vertex to next vertex)
		c1=c2=v2=NULL;
		v2=p->next;
		if (!v2) break;

		if (v2->flags&POINT_TOPREV) {
			c1=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c1=p;
		}

		if (v2->flags&POINT_TONEXT) {
			c2=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c2=v2;
		}

		if (c1==p && c2==v2) {
			 //we have the simpler case of a line segment
			flatpoint aa=(point-p->p()); //vector from p to point
			flatpoint bb=(v2->p()-p->p());   //current segment
			flatpoint vv=-(aa|=bb);         //vector perpendicular to segment starting at point
			ff=point+vv;
			dd=norm(vv); //here's a distance, but we are not yet sure if perpendicular actually touches segment

			double ss=bb*bb; //square of the length of the segment
			double sp=(ss?(aa*bb)/ss:0); //guard against p and v2 being the same point

			if (dd<d && sp>=0 && sp<=1) { //distance is closer AND we are near the segment
				found=ff;
				d=dd;            //update distance to path
				t=tt+sp;         //update tdist for found point
				vd=vdd+sp*sqrt(ss); //update visual distance for closest
			}
			vdd+=ss;   //update running visual distance

		} else {
			 //else need to search bez segment
			double vddd=0;
			double ttt=bez_closest_point(point, p->p(),c1->p(),c2->p(),v2->p(), 50, &dd,&vddd,&ff);
			if (dd<d) {
				found=ff;
				d=dd; //update distance to path
				t=tt+ttt;  //update tdist for found point
				vd=vdd+vddd; //update visual distance for closest
			}
			vdd+=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), 30);   //update running visual distance
		}

		p=v2;
		tt+=1;

	} while (p!=start);

	if (disttopath) *disttopath=d;
	if (distalongpath) *distalongpath=vd;
	if (tdist) *tdist=t;
	return found;
}

//! Find the distance along the path between the bounds, or whole length if tend<tstart.
double Path::Length(double tstart,double tend)
{
	if (!path) return 0;
	Coordinate *start=path->firstPoint(1);
	if (!start) return 0;
	Coordinate *p=start, *c1, *c2, *v2;
	double d=0, t=0;

	if (tend<tstart) {
		DBG cerr <<" *** need to implement partial path length!!"<<endl;
	}

	do { //one iteration for each segment (a vertex to next vertex)
		c1=c2=v2=NULL;
		v2=p->next;
		if (!v2) break;

		if (v2->flags&POINT_TOPREV) {
			c1=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c1=p;
		}

		if (v2->flags&POINT_TONEXT) {
			c2=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c2=v2;
		}

		if (c1==p && c2==v2) {
			 //we have the simpler case of a line segment
			d+=norm(v2->p() - p->p());

		} else {
			 //else need to search bez segment
			d+=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), 30);   //update running visual distance
		}

		p=v2;
		t+=1;

	} while (p!=start);

	return d;
}

/*! If the tt is on the path, set *err=1. else *err=0.
 *
 * \todo If tt<0 this fails... it shouldn't!!
 */
double Path::t_to_distance(double tt, int *err)
{
	if (!path) {
		if (err) *err=0;
		return 0;
	}
	Coordinate *start=path->firstPoint(1);
	if (!start) {
		if (err) *err=0;
		return 0;
	}
	Coordinate *p=start, *c1, *c2, *v2;
	double d=0, sd, t=0;

	do { //one iteration for each segment (a vertex to next vertex)
		c1=c2=v2=NULL;
		v2=p->next;
		if (!v2) break;

		if (v2->flags&POINT_TOPREV) {
			c1=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c1=p;
		}

		if (v2->flags&POINT_TONEXT) {
			c2=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c2=v2;
		}

		if (c1==p && c2==v2) {
			 //we have the simpler case of a line segment
			sd=norm(v2->p() - p->p());
			if (tt>t+1) {
				d+=sd;
				p=v2;
				t++;
				continue;
			}
			d+=sd*(tt-t);
			if (err) *err=1;
			return d;

		} else {
			 //else need to search bez segment
			sd=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), 30);   //update running visual distance
			if (tt>t+1) {
				d+=sd;
				p=v2;
				t++;
				continue;
			}
			d+=bez_t_to_distance(tt-t, p->p(),c1->p(),c2->p(),v2->p(), 30);
			if (err) *err=1;
			return d;
		}

		p=v2;
		t+=1;

	} while (p!=start);

	return d;
}

/*! If the tt is on the path, set *err=1. else *err=0.
 */
double Path::distance_to_t(double distance, int *err)
{
	if (!path) {
		if (err) *err=0;
		return 0;
	}
	Coordinate *start=path->firstPoint(1);
	if (!start) {
		if (err) *err=0;
		return 0;
	}
	Coordinate *p=start, *c1, *c2, *v2;
	double d=0, sd, t=0;

	do { //one iteration for each segment (a vertex to next vertex)
		c1=c2=v2=NULL;
		v2=p->next;
		if (!v2) break;

		if (v2->flags&POINT_TOPREV) {
			c1=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c1=p;
		}

		if (v2->flags&POINT_TONEXT) {
			c2=v2;
			v2=v2->next;
			if (!v2) break;
		} else {
			c2=v2;
		}

		if (c1==p && c2==v2) {
			 //we have the simpler case of a line segment
			sd=norm(v2->p() - p->p());
			if (distance>sd) { 
				distance-=sd;
				p=v2;
				t++;
				continue;
			}
			t+=distance/sd;
			if (err) *err=1;
			return t;

		} else {
			 //else need to search bez segment
			sd=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), 30);   //update running visual distance
			if (distance>sd) { //not terribly efficient here
				distance-=sd;
				p=v2;
				t++;
				continue;
			}
			t+=bez_distance_to_t(distance, p->p(),c1->p(),c2->p(),v2->p(), 30);
			if (err) *err=1;
			return t;
		}

		p=v2;
		t+=1;

	} while (p!=start);

	if (err) *err=0;
	return d;
}


//! Reverse the direction of the path.
/*! Return 0 for success or 1 for some error.
 *
 * \todo *** this ignores pathop ownership!
 */
int Path::Reverse()
{
	Coordinate *start,*p,*pp, *h;
	if (!path) return 1;
	p=path->firstPoint(0);
	if (!p) return 1;
	start=p;
	do {
		 //update flags
		if      (p->flags&POINT_TOPREV) p->flags=(p->flags&~POINT_TOPREV)|POINT_TONEXT;
		else if (p->flags&POINT_TONEXT) p->flags=(p->flags&~POINT_TONEXT)|POINT_TOPREV;

		 //swap next and prev pointers
		pp=p->next;
		h=p->next;
		p->next=p->prev;
		p->prev=h;

		p=pp;

	} while (p && p!=start);
	return 0;
}

//---------------------------- PathsData -----------------------------------

/*! \class PathsData
 * \ingroup interfaces
 * \brief Basically a stack of Path objects with extra fill rules.
 *
 * \code #include <lax/interfaces/pathinterface.h> \endcode
 *
 * This is the main vehicle for Path data.
 *
 * Each subpath can have its own linestyle, but
 * any fill style in Path::linestyle is
 * preempted by the fill style defined in PathsData::fillstyle.
 *
 * See also Path, and Coordinate.
 *
 * Here are the fill defines, these might get defined somewhere else later....:
 * \code
 * FILL_NONZERO 
 * FILL_ZERO
 * FILL_EVEN_ODD
 * FILL_ODD_EVEN
 * FILL_NONE
 * \endcode
 */


PathsData::PathsData(unsigned long ns)
{
	linestyle=NULL;
	fillstyle=NULL;
	style=ns; 
}

/*! Dec count of linestyle and fillstyle.
 */
PathsData::~PathsData()
{
	if (linestyle) linestyle->dec_count();
	if (fillstyle) fillstyle->dec_count();
}

SomeData *PathsData::duplicate(SomeData *dup)
{
	PathsData *newp=new PathsData(style);
	Path *path;
	for (int c=0; c<paths.n; c++) {
		path=paths.e[c]->duplicate();
		newp->paths.push(path);
	}

	newp->linestyle=linestyle; if (linestyle) linestyle->inc_count();
	newp->fillstyle=fillstyle; if (fillstyle) fillstyle->inc_count();
	
	return newp;
}

/*! If which==-1, flush all paths. If not, then remove path with that index.
 */
void PathsData::clear(int which)
{
	if (which==-1) {
		paths.flush();
		return;
	}
	if (which>=0 && which<paths.n) paths.remove(which);
}

//! Make this the new line style.
/*! If cap, join, or width are less than 0, then do not change.
 *
 * See LaxCapStyle and LaxJoinStyle.
 */
int PathsData::line(double width,int cap,int join,ScreenColor *color)
{
	if (!linestyle) linestyle=new LineStyle;

	if (width>=0) linestyle->width=width;
	if (cap>=0) linestyle->capstyle=cap;
	if (join>=0) linestyle->joinstyle=join;
	if (color) linestyle->Color(color->red,color->green,color->blue,color->alpha);

	return 0;
}

//! Fill with this color, or none if color==NULL.
int PathsData::fill(Laxkit::ScreenColor *color)
{
	if (!color) {
		if (fillstyle) fillstyle->fillstyle=FillNone;
		return 0;
	}
	if (!fillstyle) fillstyle=new FillStyle();
	fillstyle->Color(color->red,color->green,color->blue,color->alpha);
	fillstyle->fillstyle=FillSolid;

	return 0;
}

/*! \ingroup interfaces
 * \todo Make this access the basepathops in PathInterface to dump out the right
 * owner info... right now, not calling any dump_out in Path or in Coordinate..
 *
 * Dump out a PathsData, assume f is open already.
 * prepend indent number of spaces before each line.
 *
 * Something like:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  defaultlinestyle
 *    ...
 *  style 2
 *  path
 *    ...
 * </pre>
 *
 * Ignores what. Uses 0 for it.
 */
void PathsData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%slinestyle     #default line style\n",spc);
		fprintf(f,"%s  ...         #standard linestyle attributes\n",spc);
		fprintf(f,"%sfillstyle\n",spc);
		fprintf(f,"%s  ...         #standard fillstyle attributes\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #standard transform matrix\n",spc);
		fprintf(f,"%spath          #none or more of these, defines single paths\n",spc);
		fprintf(f,"%sd m 1 1 l 1 2 #paths defined by svg d path string\n",spc);
		Path p;
		p.dump_out(f,indent,-1,NULL);
		return;
	}

	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,m(0),m(1),m(2),m(3),m(4),m(5));

	if (linestyle) {
		fprintf(f,"%slinestyle\n",spc);
		linestyle->dump_out(f,indent+2,0,context);
	}

	if (fillstyle) {
		fprintf(f,"%sfillstyle\n",spc);
		fillstyle->dump_out(f,indent+2,0,context);
	}

	fprintf(f,"%sstyle %lu\n",spc,style);

	for (int c=0; c<paths.n; c++) {
		fprintf(f,"%spath %d\n",spc,c);
		paths.e[c]->dump_out(f,indent+2,what,context);
	}
}

//! Basically reverse of dump_out..
/*! If the dump is supposed to replace the current settings, then
 * they should have been flushed previously. New paths read in here
 * are appended to paths stack.
 */
void PathsData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	SomeData::dump_in_atts(att,flag,context);
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"linestyle")) {
			if (linestyle) linestyle->dec_count();
			linestyle=new LineStyle();
			linestyle->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"fillstyle")) {
			if (fillstyle) fillstyle->dec_count();
			fillstyle=new FillStyle();
			fillstyle->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"style")) {
			long stle;
			if (LongAttribute(value,&stle)) style=(unsigned long)stle;

		} else if (!strcmp(name,"path")) {
			Path *newpath=new Path();
			newpath->dump_in_atts(att->attributes.e[c],flag,context);
			paths.push(newpath);

		} else if (!strcmp(name,"d")) {
			SvgToPathsData(this, value, NULL);
		}
	}
	FindBBox();
}

double PathsData::Length(int pathi, double tstart,double tend)
{
	if (pathi<0 || pathi>=paths.n) return 0;
	return paths.e[pathi]->Length(tstart,tend);
}

//! This creates an empty path and puts it on top of the stack.
/*! This is handy when creating page layout views, for instance, so you can
 * just do pathsdata->append.... pathsdata->pushempty(); pathsdata->append....
 * to simply create several paths.
 *
 * Count of nls is incremented.
 */
void PathsData::pushEmpty(int where,LineStyle *nls)
{
	paths.push(new Path(NULL,nls),1,where);
}

//! Return the last point of the top path, or NULL if that doesn't exist
Coordinate *PathsData::LastVertex()
{
	if (!paths.n) return NULL;
	if (paths.e[paths.n-1]->path==NULL) return NULL;

	return paths.e[paths.n-1]->lastPoint(1);
}

/*! Create a circular arc, with center, and last point determines radius.
 * Sweep out angle radians. Add at least one new vertex point, and 
 * associated control points.
 */
void PathsData::appendBezArc(flatpoint center, double angle, int num_vertices)
{
	if (num_vertices<1) num_vertices=1;


    double xx,yy;
	flatpoint cp,p,cn;
	Coordinate *pp=LastVertex();
	if (!pp) { append(center); p=center; }
	else p=pp->p() - center;

	double r=p.norm();
	double start_angle=atan2(p.y,p.x);
	double theta=angle/(num_vertices); //radians between control points
    double v=4*r*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta)); //length of control handle

    for (int c=0, i=0; c<num_vertices+1; c++, i+=3) {
        xx=cos(start_angle + c*theta);
        yy=sin(start_angle + c*theta);

        p = center + flatpoint(r*xx,r*yy);
        cp= p + flatpoint(v*yy,-v*xx);
        cn= p + flatpoint(-v*yy,v*xx);

		if (c>0) append(cp,POINT_TONEXT);
		if (c>0) append(p,POINT_VERTEX);
		if (c<num_vertices) append(cn,POINT_TOPREV);
    }

}

//! Convenience to add a rectangle at (x,y) with width and height to whichpath.
/*! This also calls FindBBox().
 */
void PathsData::appendRect(double x,double y,double w,double h,SegmentControls *ctl,int whichpath)
{
	append(x,y,POINT_VERTEX,ctl,whichpath);
	append(x+w,y,POINT_VERTEX,ctl,whichpath);
	append(x+w,y+h,POINT_VERTEX,ctl,whichpath);
	append(x,y+h,POINT_VERTEX,ctl,whichpath);
	close(whichpath);
	FindBBox();
}

//! Just calls append(p.x,p.y,...).
void PathsData::append(flatpoint p,unsigned long flags,SegmentControls *ctl,int whichpath)
{ append(p.x,p.y,flags,ctl,whichpath); }

//! Append a point to whichpath via Coordinate::append.
/*! If which==-1, then append to path with greatest index. If there is not any path, than a new
 * path is created. If there is an empty path, then the point replaces the empty path.
 *
 * This is really a convenience function to allow easy construction of simple lines.
 */
void PathsData::append(double x,double y,unsigned long flags,SegmentControls *ctl,int whichpath)
{
	if (whichpath<0 || whichpath>=paths.n) whichpath=paths.n-1;
	if (paths.n==0) paths.push(new Path());
	if (whichpath<0) whichpath=0;
	paths.e[whichpath]->append(x,y,flags,ctl);
}

//! Return the coordinate after which t points to.
Coordinate *PathsData::GetCoordinate(int pathi, double t)
{
	if (pathi<0 || pathi>=paths.n) return NULL;

	Coordinate *p;
	p=paths.e[pathi]->path;
	while (p && t>1) {
		p=p->nextVertex(0);
		t-=1;
	}
	return p;
}

//! Close the whichpath.
/*! If which==-1, then close the path with greatest index. If the path is closed
 * already, then nothing is done.
 *
 * This is really a convenience function to allow easy construction of simple lines.
 */
void PathsData::close(int whichpath)
{
	if (whichpath<0 || whichpath>=paths.n) whichpath=paths.n-1;
	if (whichpath<0) return;
	paths.e[whichpath]->close();
}

/*! If index==-1, then call Path::fixpath() on only the top subpath. 
 * If index==-2, then fixpath on all subpaths.
 * else fixpath() on only the specified index.
 */
void PathsData::fixpath(int index)
{
	int s=0, e=paths.n-1;
	if (index>=paths.n || index==-1) s=e;
	else if (index>=0) s=e=index;
	for (int c=s; c<=e; c++) {
		paths.e[c]->fixpath();
	}
}

//! Return 1 if the given path contains co, else 0.
int PathsData::pathHasCoord(int pathindex,Coordinate *co)
{
	if (pathindex<0 || pathindex>=paths.n) return 0;
	if (!paths.e[pathindex]->path) return 0;
	return paths.e[pathindex]->path->hasCoord(co);
}

//! Return the index on the paths stack of the path that contains c, or -1.
/*! v=1 means c must be a vertex also.
 */
int PathsData::hasCoord(Coordinate *co)
{
	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c]->path) continue;
		if (paths.e[c]->path->hasCoord(co)) return c;
			//---was: return paths.e[c]->path;
	}
	return -1;
}

//! Sets SomeData::minx,etc, based on the union of all the paths.
void PathsData::FindBBox()
{
	DoubleBBox::clear();
	if (paths.n==0) return;

	Coordinate *p=NULL,*t,*start;
	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c] || !paths.e[c]->path) continue;

	 	 // First find a vertex point
		start=t=paths.e[c]->path->firstPoint(1);
		if (!(t->flags&POINT_VERTEX)) continue;//only mysterious control points

		addtobounds(t->p());

		 // step through all the rest of the vertices
		flatpoint c1,c2;
		while (t) {
			p=t->next;
			if (!p || p==start) break;

			if (p->flags&POINT_VERTEX) {
				 //simple case, just a line segment
				addtobounds(p->p());
				t=p;
				continue;
			}
			 //else assume bez, find 1st control
			if (p->flags&POINT_TOPREV) {
				c1=p->p(); 
				p=p->next;
				if (!p) break;
			} else c1=t->p();

			 //find second control
			if (p->flags&POINT_TONEXT) {
				c2=p->p(); 
				p=t->nextVertex();
				if (!p) break;
			} else {
				p=t->nextVertex();
				c2=p->p();
			}

			bez_bbox(t->p(),c1,c2,p->p(), this, NULL);

			t=p;
			if (t==start) break;
		}
	}
}

//! Find the point lying on subpath pathindex, at t.
/*! If tisdistance==0, then t is the usual t parameter. Else it is physical distance, starting from subpath->path.
 *
 * Return 1 for point found, otherwise 0.
 */
int PathsData::PointAlongPath(int pathindex, double t, int tisdistance, flatpoint *point, flatpoint *tangent)
{
	if (pathindex<0 || pathindex>=paths.n) return 0;
	return paths.e[pathindex]->PointAlongPath(t,tisdistance,point,tangent);
}

//! Return the point on any of the paths closest to p.
/*!  point is assumed to already be in data coordinates.
 *
 * Optionally return that distance from the path in dist, the t parameter in tdist, and the path index in pathi.
 */
flatpoint PathsData::ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist, int *pathi)
{
	double d=100000000,dalong=0,t=0, dto,dd,tt;
	flatpoint p,pp;
	int pi=0;
	for (int c=0; c<paths.n; c++) {
		pp=paths.e[c]->ClosestPoint(point, &dto,&dd,&tt);
		if (dd<d) {
			p=pp; //point
			d=dto;//dist to path
			dalong=dd;//dist along
			t=tt; 
			pi=c;
		}
	}

	if (disttopath) *disttopath=d;
	if (distalongpath) *distalongpath=dalong;
	if (tdist) *tdist=t;
	if (pathi) *pathi=pi;
	return p;
}

//! Intersect a line with one subpath. p1 and p2 are in path coordinates.
/*! ptsn is the number of points allocated in pts, and tn is the number allocated for t.
 * Search will commence for as many intersection points as will fill pts and t.
 *
 * t is for t parameters, when a t unit of 1 is the length between any two adjacent vertices.
 *
 * If isline==0, then interesect with segment [p1,p2]. Else interesect with an infinite line
 * that goes through points p1 and p2.
 *
 * Return value is the number of intersections parsed.
 */
int PathsData::Intersect(int pathindex,flatpoint p1,flatpoint p2, int isline, double startt,flatpoint *pts,int ptsn, double *t,int tn)
{
	if (ptsn<=0 && tn<=0) return 0;
	int num=0;
	for (int c=pathindex; c<=pathindex; c++) {
		num+=paths.e[c]->Intersect(p1,p2,isline, startt, pts+num,ptsn-num, t+num,tn-num);
	}
	return num;
}

/*! Return 0 for success or 1 for some error.
 */
int PathsData::ReversePath(int pathindex)
{
	if (pathindex<0 && pathindex>=paths.n) return 1;
	return paths.e[pathindex]->Reverse();
}

//! Transform each point by m(), and make m() be identity.
void PathsData::ApplyTransform()
{
	Coordinate *p,*start;
	const double *mm=m();
	for (int c=0; c<paths.n; c++) {
		start=p=paths.e[c]->path;
		do {
			p->p(transform_point(mm,p->p()));
			p=p->next;
		} while (p && p!=start);
	}
	setIdentity();
	FindBBox();
}

//! Just call MatchTransform(const double *mm).
void PathsData::MatchTransform(Affine &affine)
{
	MatchTransform(affine.m());
}

/*! Change the path transform, and change point coordinates so they
 * stay in the same places.
 *
 * Say the old transform is O. Then each point gets transformed
 * by: O*inverse(mm)
 */
void PathsData::MatchTransform(const double *newm)
{
	Coordinate *p,*start;
	double mmm[6], mm[6];
	transform_invert(mmm,newm);
	transform_mult(mm,m(),mmm);

	for (int c=0; c<paths.n; c++) {
		start=p=paths.e[c]->path;
		do {
			p->p(transform_point(mm,p->p()));
			p=p->next;
		} while (p && p!=start);
	}
	m(newm);
	FindBBox();
}



//----------------------- svgtoPathsData ----------------------------------
//! Turn an svg 'd' attribute to a PathsData.
/*! This parses via SvgToCoordinate(), then converts the Coordinate list to
 * a PathsData format.
 *
 * Return the PathsData object on success, or NULL for failure.
 *
 * If existingpath==NULL, then return a new PathsData, else append new Path
 * objects to existingpath. existingpath is also returned on success.
 */
PathsData *SvgToPathsData(PathsData *existingpath, const char *d,char **end_ptr)
{
	Coordinate *coord=SvgToCoordinate(d,0,end_ptr,NULL);
	if (!coord) return NULL;

	PathsData *paths=existingpath;
	if (!paths) {
		if (somedatafactory) {
			paths=dynamic_cast<PathsData*>(somedatafactory->newObject(LAX_PATHSDATA));
		}
		if (!paths) paths=new PathsData();//creates 1 count
	}
	Coordinate *p=coord;
	int neednew=1;
	for ( ; p; p=p->next) {
		if (neednew) { paths->pushEmpty(); neednew=0; }
		if (p->flags&POINT_LOOP_TERMINATOR) {
			paths->close();
			neednew=1;
			continue;
		}
		if (p->flags&POINT_TERMINATOR) {
			neednew=1;
			continue;
		}
		paths->append(p->p(),p->flags);
	}
	delete coord;

	paths->FindBBox();
	return paths;
}


//----------------------------- PathOperator -----------------------------------

/*! \class PathOperator
 * \ingroup interfaces
 * \brief Abstract base class of path operators.
 * \code #include <lax/interfaces/pathinterface.h> \endcode
 *
 * PathOperator objects provide helper functions to operate on Path objects. They allow
 * constructing and manipulating segments of paths that are not just plain bezier curves.
 *
 * Path::basepathops is a publically accessible static stack of PathOperators that
 * any Path object should be able to use. Thus, actual PathOperator instances must not
 * actually contain any state specific to any one path or interface.
 *
 * For a person to actually interacting with data, getInterface() will return an anInterface
 * object that can be used for that purpose, if any is available.
 */

/*! \fn Coordinate *PathOperator::newPoint(flatpoint p)
 * \brief Return a new point and associated points.
 * \param p Where a point was clicked.
 *
 * Must return the most previous point in the newly added group of points.
 * It must not return closed loops. Should return a segment that starts and ends
 * with a vertex.
 *
 * \todo *** also return which point should be currently selected!
 */

PathOperator::PathOperator(int nid)
{
	id=nid;
	if (nid<0) id=getUniqueNumber();
}

PathOperator::~PathOperator()
{
}

/*! \fn anInterface *PathOperator::getInterface(Laxkit::Displayer *dp,PathsData *data)
 * \brief Return an interface that can manipulate relevant data
 *
 * \todo *** describe requirements of any interface returned here!!
 */


////------------------------------ PathInterface ------------------------------------

/*! \class PathInterface
 * \ingroup interfaces
 *  \brief The maintainer of all PathOperator classes.
 * \code #include <lax/interfaces/pathinterface.h> \endcode
 *
 *  There should only be one main PathInterface. Sub-interfaces for particular segments
 *  are found automatically from the Path::basepathops list, and then
 *  PathOperator::getInterface(). That interface (if any) is then made a child
 *  of the main PathInterface.
 *
 *
 * \todo PathInterface should be able to draw single Path objects, not just PathsData?
 * \todo *** unknown PathOperators should be marked with a question mark at middle of path
 * \todo *** MUST make sure that PathsData/Path heads are all vertices. That makes dealing with breakpoints easier
 * \todo *** undo is very important for path editing!!
 * \todo *** must incorporate shift-move on grid lines, angles on 0/90/45/30/60/custom, etc.
 * \todo *** operations like make circle, arrange, flip, etc?
 * \todo *** constrain x/y
 */

/*! \var int PathInterface::addafter
 *  \brief Says whether actions should be after or before curvertex.
 *  If addafter==0 then all adding, opening paths acts on the path previous to curvertex.
 *  Otherwise, those act on the segment after (in the next direction from) curvertex.
 */
/*! \var PtrStack<Coordinate> PathInterface::curpoints
 *  \brief Contains list of currently selected points on a path.
 *
 * If a controlled segment is selected, then only one bezier approximation point is 
 * stored in curpoints for that segment. More complicated point selection must be performed
 * by interfaces returned by PathOperator::getInterface().
 */
/*! \var LineStyle *PathInterface::linestyle
 * \brief Pointer to what should be considered the current line style for the current data.
 *
 * Default is for this to always point to either data->linestyle or defaultline.
 */
/*! \var FillStyle *PathInterface::fillstyle
 * \brief Pointer to what should be considered the current fill style for the current data.
 *
 * Default is for this to always point to defaultfill.
 */
/*! \var int PathInterface::showdecs
 * \brief How to show point decorations.
 *
 * 0 is don't show. 1 is show all control points. 2 is show only control points for active vertices.
 * 3 is show only control points available to the current path operator.
 */

//Add point is for polylines, add bez adds 3 points, with smooth vertex and sets curpoint as the next control handle.
#define ADDMODE_Point   1
#define ADDMODE_Bezier  2

#define EDITMODE_AddPoints    1
#define EDITMODE_SelectPoints 2

//! Contstructor for PathInterface
PathInterface::PathInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	pathi_style=0;
	addmode=ADDMODE_Bezier;
	editmode=EDITMODE_AddPoints;
	colortofill=0;
	addafter=1;
	widthstep=1.2;

	linestyle=defaultline=new LineStyle();
	defaultline->inc_count();
	linestyle->Color(0xffff,0,0,0xffff);

	fillstyle=defaultfill=new FillStyle();
	defaultfill->inc_count();
	fillstyle->fillstyle=FillNone;

	controlcolor=rgbcolor(0,148,178); // defaults to blueish-white, change right after creation otherwise
	constrain=3;

	curvertex=NULL;
	curpath=NULL;
	curpathop=NULL;
	curdirp=NULL;

	child=owner=NULL;

	data=NULL;
	poc=NULL;

	lbfound=NULL;
	lbselected=1; //whether the most recent left button down added a point to selection that was not previously selected
	drawhover=0;
	hoverdevice=0;
	lasth=0;
	showdecs=1;
	verbose=1;

	creationstyle=0;

	needtodraw=1;

	sc=NULL;
}

//! Destructor, delete linestyle and fillstyle, call deletedata().
PathInterface::~PathInterface()
{
	DBG cerr <<"----in PathInterface destructor"<<endl;

	deletedata();
	if (defaultline) defaultline->dec_count();
	if (linestyle) linestyle->dec_count();
	if (defaultfill) defaultfill->dec_count();
	if (fillstyle) fillstyle->dec_count();

	if (sc) sc->dec_count();
}

const char *PathInterface::Name()
{
	return _("Path Tool");
}

//! Set this's dp.
void PathInterface::Dp(Displayer *ndp)
{//***needed??
	anInterface::Dp(ndp);
}

//! Return a new instance of PathInterface with duplicates of all in pathops.
/*! Also copies over creationstyle, controlcolor, and linestyle (contents are copied,
 * not linestyle pointer).
 */
anInterface *PathInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new PathInterface(id,NULL);
	PathInterface *dupp=dynamic_cast<PathInterface *>(dup);
	if (!dupp) return NULL;
	dupp->creationstyle=creationstyle;
	dupp->controlcolor=controlcolor;
	if (linestyle) (*dupp->linestyle)=(*linestyle);
	if (fillstyle) (*dupp->fillstyle)=(*fillstyle);
	return anInterface::duplicate(dup);
}

 //! When turning on, make sure we have lock on data, and turn on decorations
int PathInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

//! Path is being deselected, so flush the curpoints stack, and release lock on data
/*! ViewportWindow/ViewerWindow autopops curpathop, since it is the child of this.
 */
int PathInterface::InterfaceOff()
{
	Clear();
	showdecs=0;
	needtodraw=1;
	return 0;
}

//! Clear the pathinterface of its PathsData stored in data, flush curpoints.
/*! Calls deletedata().
 */
void PathInterface::Clear(SomeData *d)
{
	if (d && d!=data) return;
	deletedata(); // this ends in setting data=NULL
	SetCurvertex(NULL);
	curpath=NULL;
	curpoints.flush();
}

//! Flush the data.
/*! Also flushes curpoints.
 * Set fillstyle=defaultfill and linestyle=defaultline.
 */
void PathInterface::deletedata()
{
	if (data) { data->dec_count(); data=NULL; }
	if (poc) { delete poc; poc=NULL; }

	curpoints.flush();
	SetCurvertex(NULL);
	curpath=NULL;

	if (fillstyle) fillstyle->dec_count();
	fillstyle=defaultfill;
	if (fillstyle) fillstyle->inc_count();

	if (linestyle) linestyle->dec_count();
	linestyle=defaultline;
	if (linestyle) linestyle->inc_count();
}

////! Add apathop to pathop stack.
//void PathInterface::RegisterOp(PathOperator *apathop)
//{***
//	if (apathop) {
//		pathops.push(apathop);
//		apathop->Dp(dp);
//	}
//}

//! Delete all points in curpoints.
/*! If the point is a vertex, this will remove any attached bezier handles.
 */
int PathInterface::DeleteCurpoints()
{
	while (curpoints.n) {
		if (DeletePoint(curpoints.e[0])!=0) break; //this will remove from curpoints, break out of loop on fail
	}
	if (data) data->FindBBox();
	needtodraw=1;
	return 0;
}

//! Delete the point p, and make sure that path heads maintain their integrity.
/*! Returns 0 success, nonzero failure.
 *
 * If the point is part of a controlled segment, then delete the whole segment.
 * Usually, the segment will have its own interface to intercept delete events,
 * so hopefully this should be ok.
 */
int PathInterface::DeletePoint(Coordinate *p)
{
	if (!p) return 0;

	 // a pathop might handle deleting differently, for instance, when one deletes a control point for
	 // a bezier curve, the default is for the point to not be deleted, but to zap back to its vertex.
	 // Please be advised that if the pathop actually does delete points from the path, it will have
	 // to do all the things the rest of this procedure does. Normal use of this option is for the
	 // the pathop to not actually delete anything, just rearrange things, like deleting a focus in an
	 // ellipse, but if this function is called, the whole controlled segment is deleted.

	Path *path;
	int pathi=data->hasCoord(p);
	if (pathi<0) return 1;
	path=data->paths.e[pathi];

	Coordinate *s,*e;
	s=e=p;

	if (p->controls) {
		 //find range of points we need to delete
		while (s->prev && s->prev->controls==p->controls && s->prev!=p) s=s->prev;
		while (e->next && e->next->controls==p->controls && e->next!=p) e=e->next;
	} else if (p->flags&POINT_VERTEX) { //is vertex, remove any associated uncontrolled controls
		if (p->prev && !p->prev->controls && p->prev->flags&POINT_TONEXT) s=p->prev;
		if (p->next && !p->next->controls && p->next->flags&POINT_TOPREV) e=p->next;
	} // else assume is just a normal bezier control point, remove only that one

	p=s->detachThrough(e); //now s through e is an open path
	if (!p) {
		path->path=NULL; //deleting removed all points from path, so delete whole path
		data->paths.remove(pathi);
		curpath=NULL;
	} else {
		 //there is still a remnant of a path
		path->path=p;
		path->fixpath();
	}

	 //reassign curvertex if necessary
	if (curvertex && s->hasCoord(curvertex)) SetCurvertex(NULL);
	if (curvertex==NULL && p) SetCurvertex(p);//note p might be null if whole path was deleted
	if (!curvertex && data->paths.n && data->paths.e[0]->path) SetCurvertex(data->paths.e[0]->path);

	 //remove any points being deleted from curvertex or curpoints
	int i;
	for (p=s; p; p=p->next) {
		i=curpoints.findindex(p);
		if (i>=0) curpoints.pop(i);
	}

	delete s;

	drawhover=0;
	needtodraw=1;
	return 0;
}

//! Toggle the path containing curvertex open and closed.
/*! c==-1 toggle, c==0 want open, c==1 want closed
 *
 *  When opening, the connection between the current segment that curvertex is
 *  a part of is severed and the next segment (if addafter) or previous segment (addafter=0).
 *
 *  Return 0 success, nonzero error.
 */
int PathInterface::toggleclosed(int c) //c=-1
{
	if (!data || !data->paths.n) return 1;
	if (!curvertex) {
		curpath=data->paths.e[0];
		SetCurvertex(data->paths.e[0]->path);
	}
	if (!curvertex) return 1;
	if (!curpath) {
		int i=data->hasCoord(curvertex);
		if (i>=0) curpath=data->paths.e[i];
	}
	if (!curpath) return 1;

	int closed=curpath->path->isClosed();
	if (c==0 && !closed) return 0;
	if (c==1 &&  closed) return 0;

	if (!closed) {
		curpath->close();
		needtodraw=1;
		SetCurvertex(curvertex);
		return 0;
	}

	 //else is closed, and we must open it
	if (addafter) {
		Coordinate *p=curvertex;
		if (curvertex->controls) {
			 //find break point between segments
			while (p->next!=curvertex && p->next->controls==curvertex->controls) {
				p=p->next;
			}
		} else {
			 //must break on a vertex without a TOPREV control, or a TOPREV control
			if (p->flags&POINT_TONEXT) {
				p=p->prev;
			}
			if ((p->flags&POINT_VERTEX) && (p->next->flags&POINT_TOPREV)) {
				p=p->next;
			}
		}
		p->next->prev=NULL;
		p->next=NULL;
		curpath->fixpath();

	} else { //break prev
		Coordinate *p=curvertex;
		if (curvertex->controls) {
			while (p->prev!=curvertex && p->prev->controls==curvertex->controls) {
				p=p->prev;
			}
		} else {
			 //must break on a vertex without a TONEXT control, or a TONEXT control
			if (p->flags&POINT_TOPREV) {
				p=p->next;
			}
			if ((p->flags&POINT_VERTEX) && (p->prev->flags&POINT_TONEXT)) {
				p=p->prev;
			}
		}
		p->prev->next=NULL;
		p->prev=NULL;
		curpath->fixpath();
	}

	data->FindBBox();
	SetCurvertex(curvertex);
	needtodraw=1;
	return 0;
}

int PathInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	PathsData *ndata=dynamic_cast<PathsData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) deletedata();
	if (poc) delete poc;
	poc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}

	if (linestyle) linestyle->dec_count();
	linestyle=data->linestyle;
	if (!linestyle) linestyle=defaultline;
	if (linestyle) linestyle->inc_count();

	if (fillstyle) fillstyle->dec_count();
	fillstyle=data->fillstyle;
	if (!fillstyle) fillstyle=defaultfill;
	if (fillstyle) fillstyle->inc_count();

	curvertex=NULL;
	curpath=NULL;
	curpoints.flush();

	if (data->paths.n && data->paths.e[data->paths.n-1]->path) {
		SetCurvertex(data->paths.e[data->paths.n-1]->path->lastPoint(1));
	}

	needtodraw=1;
	return 1;
}

//! Generic interface to be assigned a PathsData or to use a LineStyle.
/*! *** should tell curpathop to usethis?
 */
int PathInterface::UseThis(anObject *newdata,unsigned int mask)
{
	if (!newdata || newdata==data) return 0;

	if (dynamic_cast<LineStyle *>(newdata)) {
		 //***
		DBG cerr <<"LineInterface new color stuff"<<endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(newdata);
		if (mask&GCForeground) {
			if (colortofill) { if (data) data->fill(&nlinestyle->color); }
			else {
				if (data && data->linestyle) data->linestyle->color=nlinestyle->color;
				else linestyle->color=nlinestyle->color;
			}
		}
		if (mask&GCLineWidth) {
			if (data && data->linestyle) data->linestyle->width=nlinestyle->width;
			else linestyle->width=nlinestyle->width;
		}
		needtodraw=1;
		return 1;
	}
	return 0;
}

/*! Draw ndata with linestyle a1 and fillstyle a2.
 * *** used for drawing stuff while not owning it.
 *
 * Tries to make a1 be a LineStyle and a2 be a FillStyle. These preempt any in the object itself.
 *
 * Pushes ndata->transform onto dp, draws, then popsaxes
 */
int PathInterface::DrawDataDp(Displayer *ndp,SomeData *ndata,anObject *a1,anObject *a2,int info)//info=0
{
	Displayer *odp=dp;
	Dp(ndp);
	DrawData(ndata,a1,a2,info);
	Dp(odp);
	return 1; 
}

int PathInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)//info=0
{
	if (!ndata || !dynamic_cast<PathsData *>(ndata)) return 0;
	int ntd=needtodraw,
		sd=showdecs;
	needtodraw=1;
	showdecs=0;
	PathsData *d=data;
	data=dynamic_cast<PathsData *>(ndata);

	LineStyle *ls=linestyle;
	linestyle=dynamic_cast<LineStyle *>(a1);
	if (!linestyle) { linestyle=ls; ls=NULL; }

	FillStyle *fs=fillstyle;
	fillstyle=dynamic_cast<FillStyle *>(a2);
	if (!fillstyle) { fillstyle=fs; fs=NULL; }

	Refresh(); // pushes and pops m in Refresh..

	data=d;
	if (ls) linestyle=ls;
	if (fs) fillstyle=fs;
	showdecs=sd;
	needtodraw=ntd;
	return 1; //*** should return 1 only if drawn?
}

//! Refresh draws the path and overlays the appropriate control points of the various PathOperator classes.
/*! \todo need mechanism to only draw what's necessary
 */
int PathInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!data || !data->paths.n) {
		if (needtodraw) needtodraw=0;
		return 1;
	}
	needtodraw=0;


	//************temp:
	//dp->DrawScreen();
	//dp->drawpoint(POINT+flatpoint(20,0), 10,1);
	//dp->drawthing(POINT.x+50,POINT.y, 20,20,0, THING_Circle);
	//dp->drawthing(POINT.x,POINT.y, 20,20,1, THING_Square);
	//dp->drawthing(POINT.x-50,POINT.y, 20,20,2, THING_Circle);
	//dp->DrawReal();

	// ------------ draw data-----------------


	Coordinate *start,*p,*p2;
	//PathOperator *pathop=NULL;
	Path *pdata;
	flatpoint fp;
	LineStyle *lstyle=NULL;
	FillStyle *fstyle=NULL;

	if (pathi_style&PATHI_Path_Is_M_Real) {
		dp->PushAndNewTransform(data->m());
	}

	int olddraw=dp->DrawImmediately(0);
	dp->DrawReal();
	for (int cc=0; cc<data->paths.n; cc++) {
		// position p to be the first point that is a vertex
		pdata=data->paths.e[cc];
		p=pdata->path->firstPoint(1);
		if (!(p->flags&POINT_VERTEX)) { // is degenerate path: no vertices
			DBG cerr <<"Degenerate path (shouldn't happen!)"<<endl;
			continue;
		}

		 //build the path to draw
		flatpoint c1,c2;
		start=p;
		dp->moveto(p->p());
		do { //one loop per vertex point
			p2=p->next; //p points to a vertex
			if (!p2) break;

			//p2 now points to first Coordinate after the first vertex
			if (p2->flags&(POINT_TOPREV|POINT_TONEXT)) {
				 //we do have control points
				if (p2->flags&POINT_TOPREV) {
					c1=p2->p();
					p2=p2->next;
				} else c1=p->p();
				if (!p2) break;

				if (p2->flags&POINT_TONEXT) {
					c2=p2->p();
					p2=p2->next;
				} else { //otherwise, should be a vertex
					//p2=p2->next;
					c2=p2->p();
				}

				dp->curveto(c1,c2,p2->p());
			} else {
				 //we do not have control points, so is just a straight line segment
				dp->lineto(p2->p());
			}
			p=p2;
		} while (p && p->next && p!=start);
		if (p==start) dp->closed();
	} //loop over paths for building before drawing


	 //now path is all built, just need to fill and/or stroke
	 //
	if (linestyle && linestyle!=data->linestyle && linestyle!=defaultline) {
		 //this is overriding the default data linestyle, usually because we have
		 //been called from DrawData()
		lstyle=linestyle;
	} else {
		//lstyle=pdata->linestyle; <- maybe option for using per subpath styles? 
		if (!lstyle) lstyle=data->linestyle;//default for all data paths
		if (!lstyle) lstyle=defaultline;   //default for interface
	}
	int hasstroke=lstyle ? (lstyle->function!=LAXOP_Dest) : 0;

	if (fillstyle && fillstyle!=data->fillstyle && fillstyle!=defaultfill) {
		 //this is overriding the default data fillstyle, usually because we have
		 //been called from DrawData()
		fstyle=fillstyle;
	} else {
		fstyle=data->fillstyle;//default for all data paths
		if (!fstyle) fstyle=defaultfill;   //default for interface
	}

	if (fstyle && fstyle->fillstyle!=FillNone) {
		dp->FillAttributes(fstyle->fillstyle,fstyle->fillrule);
		dp->NewFG(&fstyle->color);

		dp->fill(hasstroke?1:0); //fill and preserve path
	}

	if (hasstroke) {
		dp->NewFG(&lstyle->color);
		dp->LineAttributes(lstyle->width*(lstyle->widthtype==0?1:dp->Getmag()),
						   (lstyle->dotdash && lstyle->dotdash!=~0)?LineOnOffDash:LineSolid,
						   lstyle->capstyle,
						   lstyle->joinstyle);
		//DBG cerr <<"-------width:"<<lstyle->width*dp->Getmag()<<endl;

		dp->stroke(0);
	}
	 
	dp->DrawImmediately(1);


	 //show decorations
	if (showdecs) {
		for (int cc=0; cc<data->paths.n; cc++) {
			// position p to be the first point that is a vertex
			pdata=data->paths.e[cc];
			p=pdata->path->firstPoint(1);
			start=p;

			if (!(p->flags&POINT_VERTEX)) { // is degenerate path: no vertices
				DBG cerr <<"Degenerate path (shouldn't happen!)"<<endl;
				continue;
			}

			dp->NewFG(controlcolor);

			 //draw corners just outside path bounding box
			dp->LineAttributes(1,LineSolid,lstyle->capstyle,lstyle->joinstyle);
			double o=5/dp->Getmag(), //5 pixels outside, 15 pixels long
				   ow=(data->maxx-data->minx)/15,
				   oh=(data->maxy-data->miny)/15;
			dp->drawline(data->minx-o,data->miny-o, data->minx+ow,data->miny-o);
			dp->drawline(data->minx-o,data->miny-o, data->minx-o,data->miny+oh);
			dp->drawline(data->minx-o,data->maxy+o, data->minx-o,data->maxy-oh);
			dp->drawline(data->minx-o,data->maxy+o, data->minx+ow,data->maxy+o);
			dp->drawline(data->maxx+o,data->maxy+o, data->maxx-ow,data->maxy+o);
			dp->drawline(data->maxx+o,data->maxy+o, data->maxx+o,data->maxy-oh);
			dp->drawline(data->maxx+o,data->miny-o, data->maxx-ow,data->miny-o);
			dp->drawline(data->maxx+o,data->miny-o, data->maxx+o,data->miny+oh);

			//pathop=NULL;
			p=start->firstPoint();
			start=p;
			p2=NULL;
			Coordinate *nextp=NULL;
			do { //one loop for each vertex to vertex segment
				if (p->controls) {
					 //first find the bounds of the bezier approximated segment
					 //these bezier points do not get normal bezier control points drawn.
					 //Control points to actually draw are in p->controls
					nextp=p->next;
					while (nextp && nextp!=start && nextp->controls==p->controls) nextp=nextp->next;
					p=nextp;

					int c2;
					for (c2=0; c2<Path::basepathops.n; c2++)
						if (p->controls->iid()==Path::basepathops.e[c2]->id) break;
					if (c2!=Path::basepathops.n) {
						Path::basepathops.e[c2]->drawControls(dp, -1, p->controls, showdecs, this);
					} //else ignore if pathop not found during runtime!! should never happen anyway

				} else {
					 //draw normal control points
					char on=(curpoints.findindex(p)>=0);
					flatpoint sp=dp->realtoscreen(p->p());
					dp->DrawScreen();

					if (p->flags&POINT_VERTEX) {
						int f=p->flags&BEZ_MASK;
						if (!f) {
							if (p->flags&POINT_SMOOTH) f=BEZ_STIFF_NEQUAL;
							else if (p->flags&POINT_REALLYSMOOTH) f=BEZ_STIFF_EQUAL;
						}
						switch (f) {
							 //Inkscape uses: square  = smooth AND smooth-equal
							 //               diamond = corner
							 //               circle  = autosmooth
							case BEZ_STIFF_EQUAL:   dp->drawthing(sp.x,sp.y,5,5,on,THING_Circle);      break;
							case BEZ_STIFF_NEQUAL:  dp->drawthing(sp.x,sp.y,5,5,on,THING_Diamond);     break;
							case BEZ_NSTIFF_NEQUAL: dp->drawthing(sp.x,sp.y,5,5,on,THING_Square);      break;
							case BEZ_NSTIFF_EQUAL:  dp->drawthing(sp.x,sp.y,5,5,on,THING_Triangle_Up); break;
							default: dp->drawthing(sp.x,sp.y,5,5,on,THING_Circle); break;
						}
					} else if (p->flags&POINT_TONEXT) {
						dp->drawthing(sp.x,sp.y,3,3,on,THING_Circle);
						dp->drawline(sp,dp->realtoscreen(p->next->p()));
					} else if (p->flags&POINT_TOPREV) {
						dp->drawthing((int)sp.x,(int)sp.y,3,3,on,THING_Circle);
						dp->drawline(sp,dp->realtoscreen(p->prev->p()));
					}
					dp->DrawReal();
				}

				p2=p;
				p=p->next;
			} while (p && p!=start);

			 // draw little arrow on curvertex in the next direction (indicates where new points go)
			//if (curvertex || drawhover==HOVER_Direction) {
			if (curvertex || curdirp) {
				flatpoint p, p2;
				flatpoint v;
				if (curvertex) p=curvertex->p(); else p=curdirp->p();
				dp->DrawReal();
				if (editmode==EDITMODE_AddPoints) {
					if (addafter) dp->NewFG(0.,.75,0.);
					else dp->NewFG(1.,0.,0.);
				} else dp->NewFG(controlcolor);

				v=curdirv;
				p=curdirp->p();
				p2=p+v;

				if (drawhover==HOVER_Direction) dp->LineAttributes(3,LineSolid,LAXCAP_Round,LAXJOIN_Miter);
				else dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
				p=dp->realtoscreen(p);
				p2=dp->realtoscreen(p2);
				dp->DrawScreen();
				dp->drawarrow(p,p2-p,6,15,0);
				dp->DrawReal();

			}
		} // loop over paths

		if (drawhover==HOVER_DirectionSelect) {
			drawNewPathIndicator(curdirp->p(),curdirp->info);
		}

		//DBG //*** for debugging: show number of each
		//DBG start=pdata->path->firstPoint(1);
		//DBG p=start->firstPoint();
		//DBG start=p;
		//DBG int c=0;
		//DBG do {
		//DBG 	fp=dp->realtoscreen(p->p());
		//DBG 	dp->drawnum((int)fp.x,(int)fp.y,c++);
		//DBG 	p=p->next;
		//DBG } while (p && p!=start);

	} // showdecs

	 //draw various hovered points
	if (drawhover==HOVER_AddPoint) {
		dp->NewFG(0.,.75,0.);
		dp->LineAttributes(2,LineSolid,LAXCAP_Butt,LAXJOIN_Round);
		dp->drawpoint(hoverpoint, 3,0);

	} else if (drawhover==HOVER_Endpoint || drawhover==HOVER_MergeEndpoints) {
		dp->NewFG(0.,.75,0.);
		dp->LineAttributes(2,LineSolid,LAXCAP_Butt,LAXJOIN_Round);
		dp->drawpoint(hoverpoint, 7,1);

	} else if (drawhover==HOVER_Vertex || drawhover==HOVER_Point || drawhover==HOVER_Handle) {
		dp->NewFG(controlcolor);
		dp->LineAttributes(2,LineSolid,LAXCAP_Butt,LAXJOIN_Round);
		int r= (drawhover==HOVER_Handle?3:5);

		dp->DrawScreen();
		flatpoint sp=dp->realtoscreen(hoverpoint);
		switch (hoverpointtype) {
			 //Inkscape uses: square  = smooth AND smooth-equal
			 //               diamond = corner
			 //               circle  = autosmooth
			case BEZ_STIFF_EQUAL:   dp->drawthing(sp.x,sp.y,r,r,1,THING_Circle);      break;
			case BEZ_STIFF_NEQUAL:  dp->drawthing(sp.x,sp.y,r,r,1,THING_Diamond);     break;
			case BEZ_NSTIFF_NEQUAL: dp->drawthing(sp.x,sp.y,r,r,1,THING_Square);      break;
			case BEZ_NSTIFF_EQUAL:  dp->drawthing(sp.x,sp.y,r,r,1,THING_Triangle_Up); break;
			default: dp->drawthing(sp.x,sp.y,r,r,1,THING_Circle); break;
		}
		dp->DrawReal();

	} else if (drawhover==HOVER_RemoveFromSelection || drawhover==HOVER_AddToSelection) {
		int x1,y1, x2,y2;
		buttondown.getinitial(hoverdevice,LEFTBUTTON, &x1,&y1);
		buttondown.getcurrent(hoverdevice,LEFTBUTTON, &x2,&y2);
		dp->NewFG(controlcolor);
		dp->LineAttributes(1,LineOnOffDash,LAXCAP_Butt,LAXJOIN_Round);
		dp->DrawScreen();
		dp->drawline(x1,y1, x2,y1);
		dp->drawline(x2,y1, x2,y2);
		dp->drawline(x2,y2, x1,y2);
		dp->drawline(x1,y2, x1,y1);
		dp->DrawReal();
	}


	if (pathi_style&PATHI_Path_Is_M_Real) {
		dp->PopAxes();
	}

	dp->DrawImmediately(olddraw);

	return 0;
}


/*! Draw the pop up circular menu for new path or new subpath, centered on p. which==1 is top, which==2 is bottom.
 */
void PathInterface::drawNewPathIndicator(flatpoint p,int which)
{
	dp->DrawScreen();
	p=dp->realtoscreen(p);

	double radius=DIRSELECTRADIUS;
	dp->NewFG(1.,1.,1.);
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->drawpoint(p,radius,1); //whole circle
	dp->NewFG(.8,.8,.8);
	dp->drawpoint(p,radius,0); //whole circle outline

	 //highlighting of half circle
	dp->NewFG(.9,.9,.9);
	if (which==1) {
		dp->drawellipse(p.x,p.y, radius,radius, M_PI,2*M_PI, 1);
	} else if (which==2) {
		dp->drawellipse(p.x,p.y, radius,radius, 0,M_PI,      1);
	}

	 //draw heavy circle with hole, for new subpath
	dp->NewFG(0.,0.,0.);
	dp->LineAttributes(radius/5,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->drawpoint(p-flatpoint(0,radius/2), radius/6,0);

	 //draw squiggle for new path
	dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	flatpoint pts[5];
	pts[1]=p+flatpoint(-radius/6, radius/2);
	pts[2]=p+flatpoint(-radius/6, radius/2+radius/6);
	pts[3]=p+flatpoint( radius/6, radius/2+-radius/6);
	pts[4]=p+flatpoint( radius/6, radius/2);
	dp->drawbez(pts,2,0,0);

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	if (verbose) {
		if (which==1) {
			p.y+=-radius-dp->textheight()*1.5;
			dp->textout(p.x,p.y, _("New Subpath"));
		} else if (which==2) {
			p.y+=radius+dp->textheight()*1.5;
			dp->textout(p.x,p.y, _("New Path Object"));
		}
	}

	dp->DrawReal();
}

//! Set curvertex and related states, based on p.
/*! Currently this means curvertex, curpath, curdirp, curdirv.
 *
 * If path>=0, then assume p is in that path. Else search for p in paths.
 * If p is a TOPREV or TONEXT, set with the vertex connected to it.
 */
void PathInterface::SetCurvertex(Coordinate *p, int path)
{
	if (p==NULL) { curdirp=NULL; curpath=NULL; curvertex=NULL; return; }

	int i=path;
	if (i<0) i=data->hasCoord(p);
	if (i>=0) curpath=data->paths.e[i];

	if (!(p->flags&POINT_VERTEX)) {
		if (p->flags&POINT_TOPREV) {
			while (p->prev && !(p->flags&POINT_VERTEX)) p=p->prev;
		} else while (p && p->flags&POINT_TONEXT) p=p->next;
	}

	curvertex=p;
	curdirp=curvertex;
	if (addafter) curdirv=curvertex->direction(1); //uses "visual" tangent
	else          curdirv=curvertex->direction(0);
}

//! Make curdirp be valid for curvertex.
void PathInterface::UpdateDir()
{
	curdirp=curvertex;
	if (addafter) curdirv=curvertex->direction(1); //uses "visual" tangent
	else          curdirv=curvertex->direction(0);
}

//! Note this returns an element in PathInterface's own instances of available pathops, not the PathOp pool
PathOperator *PathInterface::getPathOpFromId(int iid)
{
	for (int c=0; c<Path::basepathops.n; c++) {
		if (Path::basepathops.e[c]->id==iid) {
			return Path::basepathops.e[c];
		}
	}
	return NULL;
}

//! Returns the most relevant PathOperator to operate on the given coordinate.
/*!
 */
PathOperator *PathInterface::getPathOp(Coordinate *p)
{
	if (!p) return NULL;
	if (!p->controls) return NULL;

	return getPathOpFromId(p->controls->iid());
}

//! This is supposed to return points from data context space to screen space.
/*! Redefined from anInterface to catch PATHI_Path_Is_M_Real and related styles.
 */
flatpoint PathInterface::realtoscreen(flatpoint r)
{
	if (pathi_style&PATHI_Path_Is_M_Real) {
		//if (viewport) return viewport->realtoscreen(r);
		return dp->realtoscreen(r);

	} else if (pathi_style&PATHI_Path_Is_Real) {//***might not work
		return dp->realtoscreen(r);

	} else if (pathi_style&PATHI_Path_Is_M_Screen) {//***might not work
		return transform_point(data->m(),r);

	} else if (pathi_style&PATHI_Path_Is_Screen) {//***might not work
		return r;
	}
	
	return anInterface::realtoscreen(r);
}

//! This is supposed to return points in data context space.
/*! Redefined from anInterface to catch PATHI_Path_Is_M_Real and related styles.
 */
flatpoint PathInterface::screentoreal(int x,int y)
{
	if (pathi_style&PATHI_Path_Is_M_Real) {
		//if (viewport) return viewport->screentoreal(x,y);
		return dp->screentoreal(x,y);

	} else if (pathi_style&PATHI_Path_Is_Real) {
		return dp->screentoreal(x,y);//***might not work

	} else if (pathi_style&PATHI_Path_Is_M_Screen) {
		return transform_point_inverse(data->m(),flatpoint(x,y));//***might not work

	} else if (pathi_style&PATHI_Path_Is_Screen) {
		return flatpoint(x,y);//***might not work
	}

	return anInterface::screentoreal(x,y);
}

//! Scan within real radius of p. If (u) then point has to be unselected
/*! Meant to be called in order to start a scan underneath a given point.
 *
 * \todo **** not yet implemented!
 */
Coordinate *PathInterface::scannear(Coordinate *p,char u,double radius) //***radius=25
{ //***
//	 // find which path p is on
//	int c=hasCoord(p);
//	if (c<0) return NULL;
//
//	int c2=c;
//	*** scan down through the paths, wrap around, stop at just before p
//	while (c2<data->paths.n) {
//	}
//	while (c2<c) {
//	}

	return NULL;
}

//! Scan for various hovering things.
/*! Such as a cutpoint, a segment to cut, a segment to join, or the direction arrow.
 */
int PathInterface::scanHover(int x,int y,unsigned int state)
{
	if (!data) return HOVER_None;

	flatpoint fp=transform_point_inverse(data->m(),screentoreal(x,y));

	if (      ((pathi_style&PATHI_Plain_Click_Add) && (state&LAX_STATE_MASK)==0)
		  || (!(pathi_style&PATHI_Plain_Click_Add) && (state&LAX_STATE_MASK)==ControlMask)) {
		 //scan for closest point to cut
		int dist2;
		hoverpoint=data->ClosestPoint(fp, NULL,NULL,NULL,NULL);
		dist2=norm2(realtoscreen(transform_point(data->m(), hoverpoint))-flatpoint(x,y));
		if (dist2<SELECTRADIUS2) return HOVER_AddPoint;
		return HOVER_None;

	} else if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask) && curpoints.n==0) {
		 //hover over segment to cut out
		// ***
		// return HOVER_CutSegment

	} else if (curvertex) {
		flatpoint p=realtoscreen(transform_point(data->m(), curdirp->p()));
		flatpoint v=realtoscreen(transform_point(data->m(), curdirp->p()+curdirv))-p;
		v/=norm(v);
		p+=transpose(v)*6;

		double dist=distance(flatpoint(x,y), p,p+15*v);
		if (dist<SELECTRADIUS) return HOVER_Direction;
	}

	return HOVER_None;
}

//! Scan only against endpoints
Coordinate *PathInterface::scanEndpoints(int x,int y,int *pathindex,Coordinate *exclude)
{
	if (!data) return NULL;
	Coordinate *s,*e,*p=NULL;
	flatpoint fp=flatpoint(x,y);
	if (exclude->flags&POINT_TONEXT) exclude=exclude->next;
	else if (exclude->flags&POINT_TOPREV) exclude=exclude->prev;

	for (int c=0; c<data->paths.n; c++) {
		if (!data->paths.e[c]->path) continue;
		s=data->paths.e[c]->path->firstPoint(1);
		e=data->paths.e[c]->path->lastPoint(1);
		if (s==e) continue; //is closed path

		if (s!=exclude && norm2(realtoscreen(transform_point(data->m(), s->p()))-fp)<SELECTRADIUS2) p=s;
		if (e!=exclude && norm2(realtoscreen(transform_point(data->m(), e->p()))-fp)<SELECTRADIUS2) p=e;

		DBG cerr<<"endpoint s:"<<norm2(realtoscreen(transform_point(data->m(), s->p()))-fp)<<endl;
		DBG cerr<<"endpoint e:"<<norm2(realtoscreen(transform_point(data->m(), e->p()))-fp)<<endl;

		if (p==exclude) p=NULL;
		if (p) {
			if (pathindex) *pathindex=c;
			return p;
		}
	}
	return NULL;
}


//! Scan for a point at screen coordinates (x,y).
/*! pmask is the iid that the found point must match. If pmask=0, then point can be any point
 *  does not do smart scanning: starts at c=0 each time, only checks points, no on-line jazz.
 *
 *  Scan order is to search vertices first, then control points. pmask is the interface id of
 *  the owning pathop. The point has to have this iid, except when pmask is zero, in which case
 *  the point's iid can be anything.
 *
 *  NOTE: Extra pathops are placed on the window's interface stack above PathInterface, so if the pathop
 *  provides extra control points, such as the midpoint of a line segment or the points of a
 *  control rectangle, they must redefine LBDown/Up. Control points that are actual points in
 *  the path are handled here.
 */
Coordinate *PathInterface::scan(int x,int y,int pmask, int *pathindex) // pmask=0
{
	if (!data) return NULL;
	int c=0;
	flatpoint p,p2;
	Coordinate *cp,*start;

	//flatpoint point=dp->screentoreal(x,y);

	// scan for selected points first, going down from top of stack
	//***only check the top of the stack (for now)
	if (curpoints.n) {
		cp=curpoints.e[curpoints.n-1];
		p=realtoscreen(cp->p());
		if ((!pmask || (pmask && cp->iid==pmask)) && (p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)<SELECTRADIUS2) return cp;
	}

	// scan for vertex points
	PathOperator *op=NULL;
	for (c=0; c<data->paths.n; c++) {
		start=cp=data->paths.e[c]->path;
		if (!start) continue;
		if (cp) {
			cp=cp->firstPoint(1);
			start=cp;
			do {
				if (cp->controls && cp->controls->iid()==pmask) {
					op=getPathOpFromId(cp->controls->iid());
					if (op && op->scan(dp,cp->controls,x,y)) {
						if (pathindex) *pathindex=c;
						return cp;
					}

				} else if (cp->flags&POINT_VERTEX && !pmask) {
					p=realtoscreen(transform_point(data->m(),cp->p()));
					if ((p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)<SELECTRADIUS2) {
						DBG cerr <<" path scan found vertex on "<<c<<endl;
						if (pathindex) *pathindex=c;
						return cp;
					}
				}
				cp=cp->nextVertex();
			} while (cp && cp!=start);

		}
	}

	DBG cerr <<"path scan pmask: "<<pmask<<endl;
	if (pmask) return NULL; //don't scan for controls when there is an active pathop

	// scan for control points
	for (c=0; c<data->paths.n; c++) { //only search for normal control points, controlled was checked above
		start=cp=data->paths.e[c]->path; //*** doesn't check that paths.e[c] exists, should be ok, though
		DBG cerr <<" path scan for controls on 1. path "<<c<<"/"<<data->paths.n<<endl;
		if (cp) {
			DBG cerr <<" path scan for controls on path "<<c<<endl;
			cp=cp->firstPoint(0);
			start=cp;
			do {
				if (!cp->controls && !(cp->flags&POINT_VERTEX) ) {
					p=realtoscreen(transform_point(data->m(),cp->p()));
					if ((p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)<SELECTRADIUS2) {
						DBG cerr <<" path scan found control on "<<c<<endl;
						if (pathindex) *pathindex=c;
						return cp;
					}
				}
				cp=cp->next;
			} while (cp && cp!=start);
		}
		DBG cerr <<" path scan for controls on 2. path "<<c<<"/"<<data->paths.n<<endl;
	}
	return NULL;
}

//---------- RIGHT CLICKING:
//	*** make popup with pathop names:
//	Pathops must define whether AddControl functions, and whether it
//	functions on vertices and/or controls.
//	If clicking on control point, the upper ChangeTo part refers to the
//	segment
//		---Change to?---
//		|* Line        |
//		|  Bez         |
//		|  Ellipse     |
//		|  Function    |
//		|  Spiral      |
//		|--------------|
//		| Add control >|------
//		|--------------| bez |
//		| Copy location|------
//		| Seg to bez   |
//		| Path to bez  |
//		| Delete       |
//		----------------
Laxkit::MenuInfo *PathInterface::ContextMenu(int x,int y,int deviceid)
{ //***
	return NULL;
}

//!---------- LEFT CLICKING:
/*! if some are selected, pathinterface is not primary, selected points must have owner
 *  same as curpathop. If they are unowned, they become owned by curpathop. If they are
 *  loose owned by another pathop, the extraneous control points must be deleted, and the vertex
 *  is made to be owned by curpathop. In any case a point must have Selectable and !RoForOthers
 *  set to be acted on by PathInterface.
 *
 *  <pre>
 *  click plain:
 *  	Not clicked on any point, add new point after curvertex if primary,
 *  		flush curpoints, new point is selected and active point
 *  	On a point, flush curpoints, the point is selected and active
 *
 *  click with ShiftMask:
 *  	On a point, shift on MouseUP, not moved:, toggle it on/off.
 *  		if it does move, then  shift all curpoints, don't toggle
 *  	Not on point: (nothing)
 *
 *  click with ControlMask:
 *  	On Point: if mouse moves, then scale curpoints around centroid
 *  	Not on Point: (nothing)
 *
 *  click with +^:
 *  	On Point: if mouse moves, rotate curpoints
 *  	Not on Point: (nothing)
 *
 *  ***other modes: shear/flip/arrange
 *  </pre>
 *
 * lbfound is set to which if any point is found.
 * If the mouse is moved, lbfound is set to NULL, and moved set to 1.
 */
int PathInterface::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	DBG cerr << "  in PathInterface lbd.."<<endl;
	if (buttondown.isdown(0,LEFTBUTTON)) return 0; //some other device has the lb already

	buttondown.down(d->id,LEFTBUTTON,x,y, state,HOVER_None);

	if (drawhover==HOVER_Direction) {
		buttondown.moveinfo(d->id,LEFTBUTTON, state,HOVER_Direction);
		defaulthoverp.p(curdirp->p());
		curdirp=&defaulthoverp;
		curdirp->info=0;
		PostMessage(_("Flip add direction, or drag to select next add"));
		//needtodraw=1;
		return 0;
	}

	// if control is here, then curpathop did not intercept any left click,
	// so PathInterface is free to do its thing.

	// scan does not take primary into account, so do it manually here
	if (curpathop && primary) lbfound=scan(x,y,curpathop->id);
	else lbfound=scan(x,y);
	lbselected=1;
	drawhover=0;

	// Add Point if did not find any point, and is primary.
	// also flush curpoints,
	// If new point is toprev or tonext, make curvertex the corresponding vertex.

	if (!lbfound) {
		if (drawhover==HOVER_AddPoint) {
			 //add new point at mouse position
			if (curpoints.n) curpoints.flush();
			CutNear(hoverpoint);
			curpoints.pushnodup(curvertex,0);
			needtodraw=1;
			drawhover=0;
			return 0;
		}

		drawhover=0;

		if ((state&(ControlMask|AltMask|MetaMask|ShiftMask))==0) {
			//plain click
			if (!primary && viewport) {
				 //possibly switch to existing object on canvas
				PathsData *obj=NULL;
				ObjectContext *oc=NULL;
				int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
				if (c>0) obj=dynamic_cast<PathsData *>(oc->obj);
				if (obj && obj!=data) {
					deletedata();
					data=obj; data->inc_count();
					poc=oc->duplicate();
					viewport->ChangeObject(oc,0);//adds 1 to count
					if (data->paths.n && data->paths.e[data->paths.n-1]->path) {
						SetCurvertex(data->paths.e[data->paths.n-1]->path->lastPoint(1));
					}
					needtodraw=1;
					return 0;
				} else {
					if (c==-1 && viewport->ChangeObject(oc,1)) { // change to other object type
						buttondown.up(d->id,LEFTBUTTON);
						return 0;
					}
				}
			}
			if (viewport && !data) viewport->ChangeContext(x,y,NULL);

			//if (primary) AddPoint(screentoreal(x,y));
			AddPoint(screentoreal(x,y));
			buttondown.moveinfo(d->id,LEFTBUTTON, state,HOVER_AddingPoint); //1 means we are adding a point, used to control
														   //if adding bez or poly point

		} else if ((state&(ControlMask|AltMask|MetaMask|ShiftMask))==ShiftMask) {
			if (data) {
				buttondown.moveinfo(d->id,LEFTBUTTON, state,HOVER_AddToSelection);
				drawhover=HOVER_AddToSelection;
				hoverdevice=d->id;
			}

		} else if ((state&(ControlMask|AltMask|MetaMask|ShiftMask))==ControlMask) {
			if (data) {
				buttondown.moveinfo(d->id,LEFTBUTTON, state,HOVER_RemoveFromSelection);
				drawhover=HOVER_RemoveFromSelection;
				hoverdevice=d->id;
			}
		}

	} else { // point found
		DBG cerr <<"-=-=- lbfound, state="<<state<<endl;

		drawhover=0;

		if ((state&(ControlMask|AltMask|MetaMask|ShiftMask))==0) {
			 // plain click means flush old points, select new point.
			 // Point found means that it is either ok with the current pathop
			 // or curpathop is not primary.
			if (curpoints.findindex(lbfound)>=0) lbselected=0; //point was already selected
			selectPoint(lbfound,PSELECT_FlushPoints|PSELECT_PushPoints|PSELECT_SelectPathop|PSELECT_SyncVertex);

		} else if ((state&(ControlMask|AltMask|MetaMask|ShiftMask))==ShiftMask) {
			if (curpoints.n==1) {
				 //check if curpoint is an endpoint, and we click up on another endpoint
				if (curpoints.e[0]->isEndpoint()) {
					int pathi=-1;
					Coordinate *topoint=scanEndpoints(x,y, &pathi,curpoints.e[0]);
					if (topoint && topoint!=curpoints.e[0] && topoint->isEndpoint()) {
						ConnectEndpoints(curpoints.e[0],-1, topoint,pathi);
						needtodraw=1;
						drawhover=0;
						return 0;
					}
				}
			}
			selectPoint(lbfound,PSELECT_PushPoints|PSELECT_SelectPathop|PSELECT_SyncVertex);

		}
		//else other changes are handled when moving and LBUp
	}

	needtodraw=1;
	return 0;
}

////! Connect 2 end points. Return 0 for success or nonzero for error and not connected.
//int PathInterface::Connect(Coordinate *p1,Coordinate *p2)
//{
//}

//! Select a point p with various options.
/*! This assumes that p is already a path point. This does not add points to the path.
 *
 * <pre>
 * 		flag&1: flush points
 * 		flag&2: push point onto curpoints
 * 		flag&4: select curpathop if allowed
 * 		flag&8: sync curvertex to point
 * 	</pre>
 */
void PathInterface::selectPoint(Coordinate *p,char flag)
{
	if (!p) return;

	if (flag & PSELECT_FlushPoints) { // flush curpoints
		curpoints.flush();
	}

	if (flag & PSELECT_PushPoints) { // push point
		curpoints.pushnodup(p,0);
	}

	if (flag & PSELECT_SelectPathop) { // select proper PathOperator
		if (p->controls) {
			if (curpathop && curpathop->id==p->controls->iid()) ; //already right pathop
			else ChangeCurpathop(p->controls->iid());
		} else if (curpathop) ChangeCurpathop(0);
	}

	if (flag & PSELECT_SyncVertex) { // sync curvertex to the point
		SetCurvertex(p);
	}
}

//! Swap out the old pathop for the new on the ViewportWindow interface stack
/*! Return 0 success, nonzero error.
 *
 * Note that this only works when this acts on a ViewportWindow.
 */
int PathInterface::ChangeCurpathop(int newiid)
{
	if (!viewport) return 1;

	// Must sync up curpathop only if necessary.
	if (curpathop && curpathop->id==newiid) return 0;

	// pop old curpathop from window's interfaces stack, and turn it off
	int n;
	if (curpathop) { viewport->PopId(curpathop->id); }
	n=viewport->interfaces.findindex(this)+1;

	// now n is the index of this PathInterface plus 1. The new
	// PathOperator is pushed right after PathInterface.

	// Push new curpathop onto window's interfaces stack
	curpathop=getPathOpFromId(newiid);

	if (curpathop) {
		child=curpathop->getInterface(dp,data);
		viewport->Push(child,n,0);
		child->UseThis(data);
	}

	SetCurvertex(NULL);
	curpoints.flush();

	 // post a message somewhere saying what is current pathop
	char blah[100];
	strcpy(blah,"PathInterface now using: ");
	if (curpathop) strcat(blah,curpathop->whattype());
	else strcat(blah," plain pathinterface");
	viewport->postmessage(blah);
	return 0;
}

//! Return data after making it a new, checked out with count of 2 instance of a PathsData
PathsData *PathInterface::newPathsData()
{
	PathsData *ndata=NULL;
	if (somedatafactory) {
		ndata=dynamic_cast<PathsData*>(somedatafactory->newObject(LAX_PATHSDATA));
		if (ndata) ndata->style=creationstyle;
	}
	if (!ndata) ndata=new PathsData(creationstyle);//creates 1 count

	if (!linestyle) { linestyle=defaultline; if (linestyle) linestyle->inc_count(); }
	if (!ndata->linestyle) ndata->linestyle=new LineStyle(*linestyle); //***bit of a hack here

	if (viewport) {
		ObjectContext *oc=NULL;
		viewport->NewData(ndata,&oc);
		if (poc) delete poc;
		poc=oc->duplicate();
	}
	data=ndata;
	return data;
}

//! Perhaps update undo and send message to owner that the path has changed.
void PathInterface::Modified(int level)
{
	if (owner) {
		EventData *ev=new EventData("child");
		anXApp::app->SendMessage(ev, owner->object_id, "child",object_id);
		return;
	}
}


/*! Flush curpoints, set curvertex, curpath to NULL.
 */
void PathInterface::clearSelection()
{
	curpoints.flush();
	SetCurvertex(NULL);
	curpath=NULL;
}

//! Insert at real point p in path after/before relevant break near curvertex.
/*! Flushes curpoints, assigns curvertex to something meanindful.
 * Assumes that p is real coordinates, not screen coordinates, and
 * that what we are adding starts and stops with a vertex.
 */
int PathInterface::AddPoint(flatpoint p)
{

	// If there is no data, we must create a new data.
	if (!data) newPathsData();
	p=transform_point_inverse(data->m(),p);

	// Any new point is added to the appropriate point after curvertex.
	// For currently degenerate paths, this is simply making the path be a
	// new unit from curpathop, or a plain vertex if no curpathop.
	// Otherwise, the appropriate insertion point must be
	// found, and the correctly constructed unit is inserted.


	// If there is no valid existing curvertex (ie, data has no paths, or only has null paths),
	// it must be created, essentially by assigning the created node to a path.

	if (!curvertex) {
		 //start new subpath
		data->pushEmpty();
		curpath=data->paths.e[data->paths.n-1];
//		-----------------------
//		 // must find a path and position to add point to.
//		if (!curpath && data->paths.n) curpath=data->paths.e[data->paths.n-1];
//		if (curpath && curpath->path) { // tack onto final path, if it exists
//			SetCurvertex(curpath->path->lastPoint(1));
//
//		} else {
//			 //there was no path to add onto, so manually start one
//			if (!curpath) {
//				data->pushEmpty();
//				curpath=data->paths.e[data->paths.n-1];
//			}
//		}
	}

	//now curpath exists, curvertex might exist.

	Coordinate *np=NULL, *cp=NULL;
	if (curpathop) {
		np=curpathop->newPoint(p); // newPoint defaults to a full unit
		// *** need to assign cp! maybe pathop has preference for which point you should be on
	} else {
		if (addmode==ADDMODE_Point) np=new Coordinate(flatpoint(p),POINT_VERTEX,NULL);
		else { //ADDMODE_Bezier
			np=new Coordinate(p,POINT_VERTEX|POINT_REALLYSMOOTH,NULL);
			np->prev=new Coordinate(p, POINT_TONEXT,NULL);
			np->prev->next=np;
			np->next=new Coordinate(p, POINT_TOPREV,NULL);
			np->next->prev=np;
			if (addafter) cp=np->next; else cp=np->prev;
		}
	}

	 //If curvertex==NULL, then curpath is currently a null path
	if (!curvertex) {
		curvertex=np->nextVertex(1);
		curpath->path=curvertex;

		SetCurvertex(curvertex);
		curpoints.flush();
		if (!cp) cp=curvertex;
		curpoints.push(cp,0);
		data->FindBBox();
		return 0;
	}

	
	 // There is a curpath and a curvertex, figure out how to insert a new thing in the path.

	 //now add to existing path
	Coordinate *npend=NULL;
	npend=np;
	while (npend->next && npend->next!=np) npend=npend->next;

	Coordinate *ins=curvertex;
	if (addafter) {
		 //position ins to be right before where to add the point
		if (curvertex->controls) {
			while (ins->next && ins->next!=curvertex && ins->next->controls==curvertex->controls)
				ins=ins->next;
		} else {
			if ((ins->flags&POINT_VERTEX) && ins->next && (ins->next->flags&POINT_TOPREV))
				ins=ins->next;
		}
		
		ins->insert(np->firstPoint(0),1);

	} else {
		 //position ins to be right after where to add the point
		if (curvertex->controls) {
			while (ins->prev && ins->prev!=curvertex && ins->prev->controls==curvertex->controls)
				ins=ins->prev;
		} else {
			if ((ins->flags&POINT_VERTEX) && ins->prev && (ins->prev->flags&POINT_TONEXT))
				ins=ins->prev;
		}
		
		ins->insert(np,0);
	}

	SetCurvertex(np->nextVertex(1)); // *** warning! this might return NULL!! shouldn't really happen though ... really?
	curpoints.flush();
	if (!cp) cp=curvertex;
	curpoints.push(cp,0);
	data->FindBBox();
	needtodraw=1;
	return 0;
}

//! Remove a controlled range of points.
/* If p is an uncontrolled bezier control point, only that point is removed.
 * If p is an uncontrolled vertex, nothing is done.
 *
 * The actual Coordinates are deleted, and must not be accesed again.
 */
void PathInterface::removeSegment(Coordinate *p)
{
	if (!p) return;
	if (!p->controls) {
		if (p->flags&POINT_VERTEX) return;
		if (p->flags&(POINT_TOPREV|POINT_TONEXT)) {
			 //remove ordinary bezier control points
			p->detach();
			delete p;
		}
		return;
	}

	Coordinate *start,*end;
	start=end=p;
	while (end->next && end->next->controls==p->controls) end=end->next;
	while (start->prev && start->prev->controls==p->controls) start=start->prev;
	if (start->prev) start->prev->next=end->next;
	if (end->next) end->next->prev=start->prev;
	start->prev=end->next=NULL;
	delete start;
}

//! For all curpoints, set the point smoothness, or switch to next if newtype==-1.
/*! newtype is one of BEZ_STIFF_EQUAL, BEZ_STIFF_NEQUAL, BEZ_NSTIFF_EQUAL, BEZ_NSTIFF_NEQUAL.
 */
void PathInterface::SetPointType(int newtype)
{
	Coordinate *v=NULL;
	for (int c=0; c<curpoints.n; c++) { 
		if (curpoints.e[c]->flags&POINT_VERTEX) v=curpoints.e[c];
		else {
			if (curpoints.e[c]->flags&POINT_TOPREV) v=curpoints.e[c]->prev;
			else v=curpoints.e[c]->next;
			if (curpoints.findindex(v)>=0) continue; //vertex is already in curpoints, 
													//don't toggle more than once for vertex!
		}
		SetPointType(v,newtype);
	}
}

/*! newtype is one of BEZ_STIFF_EQUAL, BEZ_STIFF_NEQUAL, BEZ_NSTIFF_EQUAL, BEZ_NSTIFF_NEQUAL.
 * Or -1 for next type, or -2 to prev type.
 */
void PathInterface::SetPointType(Coordinate *v,int newtype)
{
	const char *mes=NULL;
	int f=v->flags&BEZ_MASK;
	if (!f) {
		if (v->flags&POINT_SMOOTH) f=BEZ_STIFF_NEQUAL;
		else if (v->flags&POINT_REALLYSMOOTH) f=BEZ_STIFF_EQUAL;
	}
	if (!v->controls) {
		if (newtype>0) f=newtype;
		else {
			if (newtype==-1) {
				switch (f) {
					case BEZ_STIFF_EQUAL: f=BEZ_STIFF_NEQUAL;   break;
					case BEZ_STIFF_NEQUAL: f=BEZ_NSTIFF_NEQUAL;  break;
					//case BEZ_STIFF_NEQUAL: f=BEZ_NSTIFF_EQUAL;  break;
					//case BEZ_NSTIFF_EQUAL: f=BEZ_NSTIFF_NEQUAL; break;
					case BEZ_NSTIFF_NEQUAL: f=BEZ_STIFF_EQUAL;  break;
					default: f=BEZ_NSTIFF_NEQUAL; break;
				}
			} else {
				switch (f) {
					case BEZ_STIFF_EQUAL: f=BEZ_NSTIFF_NEQUAL;   break;
					case BEZ_STIFF_NEQUAL: f=BEZ_STIFF_EQUAL;  break;
					//case BEZ_NSTIFF_EQUAL: f=BEZ_STIFF_NEQUAL; break;
					//case BEZ_NSTIFF_NEQUAL: f=BEZ_NSTIFF_EQUAL;  break;
					case BEZ_NSTIFF_NEQUAL: f=BEZ_STIFF_NEQUAL;  break;
					default: f=BEZ_NSTIFF_NEQUAL; break;
				}
			}
		}
	}
	v->flags=(v->flags&~(BEZ_MASK|POINT_SMOOTH|POINT_REALLYSMOOTH))|f;

	mes=NULL;
	switch (f) {
		case BEZ_STIFF_EQUAL:   mes=_("Smooth, equal");   break;
		case BEZ_STIFF_NEQUAL:  mes=_("Smooth, unequal"); break;
		case BEZ_NSTIFF_EQUAL:  mes=_("Corner, equal");   break;
		//case BEZ_NSTIFF_NEQUAL: mes=_("Corner, unequal"); break;
		case BEZ_NSTIFF_NEQUAL: mes=_("Corner"); break;
		default: mes=_("Corner"); break;
	}
	if (mes) viewport->postmessage(mes);
	needtodraw=1;
}

int PathInterface::ConnectEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi)
{
	 //find which sides they are endpoints
	 //make to be same path direction as from
	if (fromi<0) fromi=data->hasCoord(from);
	if (toi<0) toi=data->hasCoord(to);
	if (toi<0 || fromi<0) return 0;

	int fromdir=from->isEndpoint();
	int todir  =to->isEndpoint();
	if (!fromdir || !todir) return 0;

	if ((fromdir>0 && todir>0) || (fromdir<0 && todir<0)) data->ReversePath(toi);

	Coordinate *fp;
	Coordinate *tp;

	if (fromdir>0) {
		fp=from->lastPoint(0);
		tp=to->firstPoint(0);
	} else {
		fp=to->lastPoint(0);
		tp=from->firstPoint(0);
	}
	fp->next=tp;
	tp->prev=fp;

	data->paths.e[fromi]->path=to->firstPoint(1);
	if (fromi==toi) {
	} else {
		 //remove other path
		data->paths.e[toi]->path=NULL;
		data->paths.remove(toi);
	}

	curpoints.flush();
	curpoints.push(to);
	SetCurvertex(to);
	needtodraw=1;

	return 1;
}

//! Take 2 end points, and make them the same point, merging paths if necessary.
int PathInterface::MergeEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi)
{
	//find which sides they are endpoints
	//make to be same path direction as from
	
	if (fromi<0) fromi=data->hasCoord(from);
	if (toi<0) toi=data->hasCoord(to);
	if (toi<0 || fromi<0) return 0;

	int fromdir=from->isEndpoint();
	int todir  =to->isEndpoint();
	if (!fromdir || !todir) return 0;
	curpoints.flush();

	if ((fromdir>0 && todir>0) || (fromdir<0 && todir<0)) data->ReversePath(toi);

	 //need to take prev of from and put on to, removing to's prev (for fromdir>0)
	Coordinate *fp;
	Coordinate *tp;
	Coordinate *tmp;

	 //standardize connect order for code below
	if (fromdir<0) {
		tmp=from;
		from=to;
		to=tmp;
		int tt=fromi;
		fromi=toi;
		toi=tt;
	}

	 //we connect from final path point to beginning path point
	fp=from->lastPoint(0);
	tp=to->firstPoint(0);

	 //first remove in between control points
	if (fp->flags&POINT_TOPREV) {
		fp=fp->prev;
		tmp=fp->next;
		fp->next=NULL;
		tmp->prev=NULL;
		delete tmp;
	}
	if (tp->flags&POINT_TONEXT) {
		tp=tp->next;
		tmp=tp->prev;
		tp->prev=NULL;
		tmp->next=NULL;
		delete tmp;
	}

	 //now fp and tp should be vertices, need to remove fp
	fp=fp->prev;
	tmp=fp->next;
	fp->next=NULL;
	tmp->prev=NULL;
	delete tmp;

	 //finally connect the terminal points
	fp->next=tp;
	tp->prev=fp;

	data->paths.e[fromi]->path=to->firstPoint(1);
	if (fromi!=toi) {
		 //remove other path
		data->paths.e[toi]->path=NULL;
		data->paths.remove(toi);
	}

	curpoints.pushnodup(to,0);
	SetCurvertex(to);
	needtodraw=1;

	return 1;
}

int PathInterface::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int i1;
	int action;
	int xi,yi;
	buttondown.getinitial(d->id,LEFTBUTTON, &xi,&yi);
	int moved=buttondown.up(d->id,LEFTBUTTON, &i1,&action);
	if (moved<0) return 1;

	if (action==HOVER_Direction) {
		if (!moved) {
			addafter=!addafter;
			curdirv=-curdirv;
			drawhover=HOVER_None;
			needtodraw=1;
		}
		return 0;

	} else if (action==HOVER_DirectionSelect) {
		if (curdirp->info==1) {
			SetCurvertex(NULL);
			PostMessage(_("Next point starts subpath"));

		} else if (curdirp->info==2) {
			if (pathi_style&PATHI_One_Path_Only) return 0;
			deletedata();
			needtodraw=1;
			PostMessage(_("Next point starts new path object"));

		} else PostMessage(" ");

		curdirp=curvertex;
		drawhover=HOVER_None;
		needtodraw=1;
		return 0;

	} else if (drawhover==HOVER_RemoveFromSelection || drawhover==HOVER_AddToSelection) {
		if (hoverdevice!=d->id) return 0;

		DoubleBBox box;
		box.addtobounds(xi,yi);
		box.addtobounds(x,y);
		DBG cerr <<"scan box "<<box.minx<<','<<box.miny<<" "<<box.maxx<<','<<box.maxy<<endl;

		Coordinate *start, *cp;
		flatpoint p;
		for (int c=0; c<data->paths.n; c++) {
			start=cp=data->paths.e[c]->path;
			if (!start) continue;

			cp=cp->firstPoint(0);
			start=cp;
			do {
				p=realtoscreen(transform_point(data->m(),cp->p()));
				DBG cerr <<"scan point "<<p.x<<','<<p.y<<endl;
				if (box.boxcontains(p.x,p.y)) {
					if (drawhover==HOVER_RemoveFromSelection) {
						curpoints.remove(curpoints.findindex(cp));
					} else { //drawhover==HOVER_AddToSelection
						curpoints.pushnodup(cp,0);
					}
				}
				cp=cp->next;
			} while (cp && cp!=start);
		}

		drawhover=0;
		hoverdevice=0;
		needtodraw=1;
		return 0;
	}

	switch (state&(ControlMask|AltMask|MetaMask|ShiftMask)) {
		case (0): { // plain click
				if (curpoints.n==1 && moved) {
					 //check if moving endpoint onto another endpoint
					if (abs(curpoints.e[0]->isEndpoint())==1) {
						int pathi=-1;
						Coordinate *topoint=scanEndpoints(x,y, &pathi,curpoints.e[0]);
						if (topoint && topoint!=curpoints.e[0] && abs(topoint->isEndpoint())==1) {
							MergeEndpoints(curpoints.e[0],-1, topoint,pathi);
							needtodraw=1;
							drawhover=0;
							return 0;
						}
					}
				}

				if (lbfound && !moved) { // clicked on a point and didn't move
					 //flush-push-setcpop-syncCV
					if (curpoints.n==1 && lbfound==curpoints.e[0] && (lbfound->flags&POINT_VERTEX) && !lbselected) {
						 //click down then up on only selected vertex, toggle selected
						curpoints.flush();
					} else selectPoint(lbfound,PSELECT_FlushPoints|PSELECT_PushPoints|PSELECT_SelectPathop|PSELECT_SyncVertex);
					needtodraw|=2;
					return 0;

				} else if (!lbfound && !moved && action==HOVER_AddingPoint && curvertex) {
					 //we clicked down then up without moving and added a new point, so trim the control points
					curpoints.flush();
					Coordinate *cv=curvertex;
					if (curvertex->next && (curvertex->next->flags&POINT_TOPREV)) curpoints.push(curvertex->next,0);
					if (curvertex->prev && (curvertex->prev->flags&POINT_TONEXT)) curpoints.push(curvertex->prev,0);
					DeleteCurpoints(); //makes curvertex null
					SetCurvertex(cv);
					curpoints.push(curvertex,0);
					needtodraw=1;
					return 0;
				}
			} break;
		case (ShiftMask): {
				if (!lbfound && moved && !curpoints.n) { // was selecting with a box
					//*** add point within the box.
				} else if (lbfound && !moved) { // toggle selection of lbfound
					int c=curpoints.findindex(lbfound);
					if (c>=0) curpoints.pop(c); // toggle off
					else { // found unselected point, toggle on
						selectPoint(lbfound,PSELECT_PushPoints|PSELECT_SelectPathop|PSELECT_SyncVertex); //push-setcpop-syncCV
					}
					needtodraw|=2;
				}
			} break;
		case (ControlMask): {
				if (lbfound && !moved) {
					int c=curpoints.findindex(lbfound);
					if (c>=0) { // found point  --  not moved  --  selected
						//***needs testing*** scan underneath swapping point if necessary, otherwise leave point as is

						Coordinate *np;
						np=scannear(lbfound,1);  // scannear have option that 1 means that the point has to be an unselected one??
						int c2=(np?curpoints.findindex(np):-1);

						if (np) {
							if (c2<0) { // found point is not selected, so deselect lbfound, select np
								curpoints.pop(c);
								selectPoint(np,PSELECT_PushPoints|PSELECT_SelectPathop|PSELECT_SyncVertex); //push-setcpop-syncCV
							} else { // found point is already selected
								 // rearrange stack by swapping np and lbfound***whatthehell is this? is it remotely useful?
								curpoints.e[c]=np;
								curpoints.e[c2]=lbfound;
							}
						} else { //did not find a near point, so deselect this point
							curpoints.pop(c); // toggle off
						}

					} else { // found point  --  not moved  --  not selected
						selectPoint(lbfound,PSELECT_PushPoints|PSELECT_SelectPathop|PSELECT_SyncVertex); //push-setcpop-syncCV
					}
					needtodraw|=2;
					return 0;

				}
			} break;
		case (ShiftMask|ControlMask): {
			} break;
	}
	return 0;
}

//! Add a new point at closest point to hoverpoint, which is in data coordinates.
int PathInterface::CutNear(flatpoint hoverpoint)
{
	double t;
	int pathi=-1;
	//flatpoint newpoint=data->ClosestPoint(hoverpoint, NULL,NULL,&t,&pathi);
	data->ClosestPoint(hoverpoint, NULL,NULL,&t,&pathi);
	Coordinate *p1=data->GetCoordinate(pathi,t);
	if (!p1) return 1;
	t=t-(int)t;

	//need to install a point between v and v->nextVertex
	flatpoint pts[5];
	Coordinate *c1,*c2,*p2;
	p2=p1->next;
	if (p2->flags&POINT_TOPREV) {
		c1=p2;
		p2=p2->next;
	} else c1=p1;
	if (p2->flags&POINT_TONEXT) {
		c2=p2;
		p2=p2->next;
	} else c2=p2;


	bez_subdivide(t, p1->p(), c1->p(), c2->p(), p2->p(),  pts);
	if (c1!=p1) c1->p(pts[0]);
	if (c2!=p2) c2->p(pts[4]);
	Coordinate *np, *cp;
	if (c1==p1 && c2==p2) {
		 //add segment point, not bez
		np=new Coordinate(pts[2],POINT_VERTEX|BEZ_NSTIFF_NEQUAL,NULL);
		cp=np;
	} else {
		 //add bez
		np=new Coordinate(pts[1],POINT_TONEXT,NULL);
		np->connect(new Coordinate(pts[2],POINT_VERTEX|POINT_REALLYSMOOTH|BEZ_STIFF_EQUAL,NULL),1);
		np->next->connect(new Coordinate(pts[3],POINT_TOPREV,NULL),1);
		cp=np->next;
	}

	curpath=data->paths.e[pathi];
	curvertex=p1;
	Coordinate *v=curvertex;
	if (curvertex->next && (curvertex->next->flags&POINT_TOPREV)) v=v->next;
	v->insert(np);
	SetCurvertex(cp);

	return 0;
}

int PathInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	//if (curpoints.n==1 && lbfound==curpoints.e[0] && (lbfound->flags&POINT_VERTEX) && !lbselected) {
	//***SetPointType(-1);

	Coordinate *p=scan(x,y, 0,NULL);
	if (!p || !(p->flags&POINT_VERTEX)) return 1;
	SetPointType(p,-1);
	hoverpointtype=p->flags&BEZ_MASK;

	return 0;
}

int PathInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	Coordinate *p=scan(x,y, 0,NULL);
	if (!p || !(p->flags&POINT_VERTEX)) return 1;
	SetPointType(p,-2);
	hoverpointtype=p->flags&BEZ_MASK;

	return 0;
}


int PathInterface::shiftBezPoint(Coordinate *pp,flatpoint d)
{
	if (d.x==0 && d.y==0) return 0; // should have ZERO=1e-10 or something rather than 0 because of rounding errors???
	
	//double dx=d.x,dy=d.y;
	Coordinate *c,*v;
	unsigned long f;
	
		 
	 // Must shift the point and also the control point on other side of vertex if exists and is necessary
	if (pp->flags&POINT_VERTEX) { // is on vertex: c1-< V >-c2
		pp->ShiftPoint(d);
		if (pp->next && !pp->next->controls && (pp->next->flags&POINT_TOPREV)) pp->next->ShiftPoint(d);
		if (pp->prev && !pp->prev->controls && (pp->prev->flags&POINT_TONEXT)) pp->prev->ShiftPoint(d);
		return 1;

	} else {
		if (pp->flags&POINT_TOPREV) {  // is on c1-v-< C2 >
			 // set v=the vertex, c=the 'prev' control, pp is the 'next' control
			v=pp->prev;
			if (!v->controls) {
				f=v->flags&BEZ_MASK;
				if (!f) {
					if (v->flags&POINT_SMOOTH) f=BEZ_STIFF_NEQUAL;
					else if (v->flags&POINT_REALLYSMOOTH) f=BEZ_STIFF_EQUAL;
				}
			} else f=BEZ_NSTIFF_NEQUAL;

			if (v->prev && !v->prev->controls && (v->prev->flags&POINT_TONEXT)) c=v->prev; else c=NULL;

		} else { // assume POINT_TONEXT
			 // set v=the vertex, c=the 'next' control, pp is the 'prev' control
			v=pp->next;
			if (!v->controls) {
				f=v->flags&BEZ_MASK;
				if (!f) {
					if (v->flags&POINT_SMOOTH) f=BEZ_STIFF_NEQUAL;
					else if (v->flags&POINT_REALLYSMOOTH) f=BEZ_STIFF_EQUAL;
				}
			} else f=BEZ_NSTIFF_NEQUAL;
			if (v->next && !v->next->controls && (v->next->flags&POINT_TOPREV)) c=v->next; else c=NULL;

		}
		pp->ShiftPoint(d);
		if (c) switch (f) {
			case BEZ_STIFF_EQUAL: {
					DBG cerr <<"BEZ_STIFF_EQUAL"<<endl;
					c->p(2*v->p() - pp->p());
				} break;
			case BEZ_NSTIFF_EQUAL: {
					DBG cerr <<"BEZ_NSTIFF_EQUAL"<<endl;
					double nvc=norm(pp->p() - v->p()),
					       ncv=norm(c->p() -  v->p());
					if (ncv!=0) c->p(v->p() + nvc/ncv*(c->p() - v->p()));
					else { // if c1,2 start at v give c2 a stiff direction
						c->p(2*v->p() - pp->p());
					}
				} break;
			case BEZ_STIFF_NEQUAL: { 
					DBG cerr <<"BEZ_STIFF_NEQUAL"<<endl;
					double nvc=norm(pp->p() - v->p()),
					       ncv=norm(c->p() -  v->p());
					if (nvc!=0) c->p(v->p() - ncv/nvc*(pp->p() - v->p()));
				} break;
			case BEZ_NSTIFF_NEQUAL:  //Already shifted point, so do nothing!
				DBG cerr <<"BEZ_STIFF_EQUAL"<<endl;
				break;

		} // switch
		return 1;
	}
		
	return 0; // return 1 if the point is shifted, else 0
}

/*! Moves the points in curpoints by displacement d. The point's owner can
 * ShiftPoints itself, otherwise the default shifting is done, that is d is
 * just added straight to the point.
 */
int PathInterface::shiftSelected(flatpoint d)
{
	if (!data) return 0;
	DBG cerr <<"----move selected"<<endl;
	PathOperator *pathop;
	Coordinate *p;

	for (int c=0; c<curpoints.n; c++) {
		pathop=NULL;
		if (curpoints.e[c]->controls) pathop=getPathOpFromId(curpoints.e[c]->controls->iid());
		if (pathop) pathop->ShiftPoint(curpoints.e[c],d);
		else {
			 //If trying to move a control point, and we are also moving a vertex at some
			 //point, then skip moving the control point
			p=curpoints.e[c];
			if (p->flags&POINT_TONEXT) p=p->next;
			else if (p->flags&POINT_TOPREV) p=p->prev;
			else p=NULL;

			if (p) {
				int c2;
				for (c2=0; c2<curpoints.n; c2++) {
					if (c2==c) continue;
					if (curpoints.e[c2]==p) break;
				}
				if (c2!=curpoints.n) continue;
			}

			shiftBezPoint(curpoints.e[c],d);
		}
	}
	UpdateDir();
	data->FindBBox();
	needtodraw=1;
	return 0;
}

int PathInterface::scaleSelected(flatpoint center,double f,int constrain)
{
	if (!data) return 0;
	DBG cerr <<"PathInterface::scaleSelected(), constrain "<<constrain<<endl;
	PathOperator *pathop;
	for (int c=0; c<curpoints.n; c++) {
		pathop=NULL;
		if (curpoints.e[c]->controls) pathop=getPathOpFromId(curpoints.e[c]->controls->iid());
		if (pathop) pathop->Scale(center,f,constrain,curpoints.e[c]);
		else {
			flatpoint p2=center+(curpoints.e[c]->p()-center)*f;
			if (constrain&2) curpoints.e[c]->y(p2.y);
			if (constrain&1) curpoints.e[c]->x(p2.x);
		}
	}
	UpdateDir();
	data->FindBBox();
	needtodraw=1;
	return 0;
}

int PathInterface::rotateSelected(flatpoint center,double angle)
{
	if (!data) return 0;
	PathOperator *pathop;
	for (int c=0; c<curpoints.n; c++) {
		pathop=NULL;
		if (curpoints.e[c]->controls) pathop=getPathOpFromId(curpoints.e[c]->controls->iid());
		if (pathop) pathop->Rotate(center,angle,curpoints.e[c]);
		else {
			flatpoint p=rotate(curpoints.e[c]->p(),center,angle,1);
			curpoints.e[c]->p(p);
		}
	}
	UpdateDir();
	data->FindBBox();
	needtodraw=1;
	return 0;
}

//! Post a status message based on drawhover.
void PathInterface::hoverMessage()
{
	const char *mes="";

	if (drawhover==HOVER_Vertex || drawhover==HOVER_Point || drawhover==HOVER_Handle) mes="";
	else if (drawhover==HOVER_Endpoint) mes=_("Click to add segment between endpoints");
	else if (drawhover==HOVER_MergeEndpoints) mes=_("Merge endpoints");
	else if (drawhover==HOVER_AddPoint) mes=_("Click to add point to segment");
	else if (drawhover==HOVER_Direction) mes=_("Click to flip add direction, or drag to select next add");
	//else if (drawhover==HOVER_Direction) mes=_("Click to change add direction, or drag to start new subpath");

	PostMessage(mes);
}

/*! 
 * \todo
 *   If you click on the middle of a straight line segment, there should be an option
 *   to move that line in direction perpendicular to itself, but keep adjacent (straight)
 *   line segments in same place, just adjust intersection with the moved line..
 */
int PathInterface::MouseMove(int x,int y,unsigned int state,const LaxMouse *mouse)
{
	DBG flatpoint fp=dp->screentoreal(x,y);
	DBG cerr <<"path mousemove: "<<fp.x<<','<<fp.y;
	DBG fp=screentoreal(x,y);
	DBG cerr <<"  rts: "<<fp.x<<','<<fp.y<<endl;

	//DBG needtodraw=1;
	//DBG POINT=flatpoint(x,y);

	if (!buttondown.isdown(mouse->id,LEFTBUTTON)) {
		int oldhover=drawhover;
		drawhover=HOVER_None;

		 //check for hovering over endpoint when another endpoint is selected
		if (data && (state&ShiftMask) && curpoints.n==1) {
			int pathi=-1;
			Coordinate *topoint=scanEndpoints(x,y, &pathi,curpoints.e[0]);
			if (topoint && topoint!=curpoints.e[0] && abs(topoint->isEndpoint())==1) {
				drawhover=HOVER_Endpoint;
				hoverpoint=topoint->p();

				DBG cerr<<"Found hover over endpoint"<<endl;
				hoverMessage();
				needtodraw=1;
				return 0;
			}
		}

		 //scan for ordinary points
		Coordinate *h=scan(x,y,curpathop?curpathop->id:0);

		if (h) {
			 //found hovering over existing control or vertex
			drawhover=((h->flags&POINT_VERTEX)?HOVER_Vertex:HOVER_Handle);
			if (drawhover==HOVER_Vertex) {
				hoverpointtype=h->flags&BEZ_MASK;
				if (!hoverpointtype) {
					if (h->flags&POINT_SMOOTH) hoverpointtype=BEZ_STIFF_NEQUAL;
					else if (h->flags&POINT_REALLYSMOOTH) hoverpointtype=BEZ_STIFF_EQUAL;
				}
			} else hoverpointtype=BEZ_STIFF_EQUAL;

			if (drawhover!=oldhover) needtodraw|=2;
			hoverpoint=h->p();
			hoverMessage();
			return 0;
		}

		 //else maybe something else to hover over:
		drawhover=scanHover(x,y,state);

//		 //maybe hover an add point indicator
//		if (data 
//			  && (((pathi_style&PATHI_Plain_Click_Add) && (state&LAX_STATE_MASK)==0)
//			     || (!(pathi_style&PATHI_Plain_Click_Add) && (state&LAX_STATE_MASK)==ControlMask))) {
//			 //maybe add point to a path
//			fp=transform_point_inverse(data->m(),screentoreal(x,y));
//			int dist2;
//			hoverpoint=data->ClosestPoint(fp, NULL,NULL,NULL,NULL);
//			dist2=norm2(realtoscreen(transform_point(data->m(), hoverpoint))-flatpoint(x,y));
//			if (dist2<SELECTRADIUS2) drawhover=HOVER_AddPoint; else drawhover=0;
//			hoverMessage();
//
//			DBG cerr<<"---pathinterface hoverpoint: "<<hoverpoint.x<<','<<hoverpoint.y<<endl;
//			needtodraw=1;
//			return 0;
//
//		}

		if (drawhover!=oldhover || drawhover==HOVER_AddPoint) {
			needtodraw=1;
			hoverMessage();
		}
		return 0;
	}
	
	//now left button down assumed..
	// lbfound=... Note that lbfound is no longer set to NULL ***check all modes??
	if (!data) return 0;

	int oldx,oldy;
	int i1,action;
	buttondown.move(mouse->id,x,y, &oldx,&oldy);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &i1,&action);

	if (curpoints.n==1 && action!=HOVER_Direction && action!=HOVER_DirectionSelect) {
		 //check if moving endpoint onto another endpoint
		DBG cerr <<"curpoint is endpoint: "<<abs(curpoints.e[0]->isEndpoint())<<endl;

		if (abs(curpoints.e[0]->isEndpoint())==1) {
			int pathi=-1;
			Coordinate *topoint=scanEndpoints(x,y, &pathi,curpoints.e[0]);
			int oldhover=drawhover;
			if (topoint && topoint!=curpoints.e[0] && abs(topoint->isEndpoint())==1) {
				drawhover=HOVER_MergeEndpoints;
				hoverpoint=topoint->p();

				DBG cerr<<"Found hover over endpoint"<<endl;
				needtodraw=1;
			} else {
				drawhover=0;
			}
			if (oldhover!=drawhover) {
				hoverMessage();
				needtodraw=1;
			}
		}
	}


	flatpoint p1=screentoreal(x,y);
	flatpoint p2=screentoreal(oldx,oldy);
	//if (!curpoints && ***in bounds) shift object
	flatpoint d=transform_point_inverse(data->m(),p1)-transform_point_inverse(data->m(),p2);

	DBG cerr <<"path mm action: "<<action<<endl;
	if (action==HOVER_Direction) {
		if (pathi_style&PATHI_One_Path_Only) return 0;

		 //if distance far enough away from down spot, pop up the new path symbols
		int firstx,firsty;
		buttondown.getinitial(mouse->id,LEFTBUTTON, &firstx,&firsty);
		flatpoint first(firstx,firsty);
		flatpoint v=flatpoint(x,y)-first;
		flatpoint pp;
		if (norm2(v)>200) {
			drawhover=HOVER_DirectionSelect;
			buttondown.moveinfo(mouse->id,LEFTBUTTON, state,HOVER_DirectionSelect);
			pp=first + v*2 + DIRSELECTRADIUS*(v/norm(v));
			curdirp->p(transform_point_inverse(data->m(),screentoreal(pp.x,pp.y)));
			needtodraw=1;
		}
		return 0;

	} else if (action==HOVER_DirectionSelect) {
		int which=0;
		flatpoint pp=realtoscreen(transform_point(data->m(),curdirp->p()));
		if (norm2(pp-flatpoint(x,y))>DIRSELECTRADIUS*DIRSELECTRADIUS) which=0;
		else if (y<pp.y) which=1;
		else which=2;

		if (which!=curdirp->info) {
			curdirp->info=which;
			if (curdirp->info==1) {
				PostMessage(_("Next point starts subpath"));
			} else if (curdirp->info==2) {
				PostMessage(_("Next point starts new path object"));
			} else PostMessage(" ");
			needtodraw=1;
		}
		return 0;
	}

	if (action==HOVER_AddToSelection || action==HOVER_RemoveFromSelection) {
		hoverdevice=mouse->id;
		needtodraw=1;
		return 0;
	}

	if (curpoints.n==1) {
		if ((curpoints.e[0]->flags&POINT_VERTEX) && (state&LAX_STATE_MASK)==ControlMask) {
			 //control click on a single selected vertex switches to unmade handle
			Coordinate *p=curpoints.e[0];
			int nonext=(!p->next || (p->next && !(p->next->flags&POINT_TOPREV)));
			int noprev=(!p->prev || (p->prev && !(p->prev->flags&POINT_TONEXT)));

			if (nonext && noprev) {
				 //create new control nearest to direction you are dragging
				flatvector vnext=p->direction(1);
				flatvector vprev=p->direction(0);
				vnext/=norm(vnext);
				vprev/=norm(vprev);
				double dnext=norm2(d-vnext);
				double dprev=norm2(d-vnext);
				if (dnext<dprev) {
					lasth=1;
					PerformAction(PATHIA_CurpointOnHandle);
				} else {
					lasth=-1;
					PerformAction(PATHIA_CurpointOnHandle);
				}
			} if (nonext) {
				lasth=1;
				PerformAction(PATHIA_CurpointOnHandle);
			} else if (noprev) {
				lasth=-1;
				PerformAction(PATHIA_CurpointOnHandle);
			}
		}
	}

	if (curpoints.n==1) state=0;
	switch (state&(ControlMask|AltMask|MetaMask|ShiftMask)) {
		case 0: { // plain click
				if (curpoints.n) {
					shiftSelected(d);
				} else {
					data->origin(data->origin()+(p1-p2));
					needtodraw=1;
				} 
			} break;

		case (ShiftMask): { // +move == shift curpoints
				if (!lbfound && !curpoints.n) ; //***expand selection box;
				else if (lbfound || curpoints.n) {
					shiftSelected(d);
				}
			} break;

		case (ControlMask): { // ^move == scale curpoints
					//*** this is scale in and out from center point
					//*** maybe have option to scale by +-d??
				if (curpoints.n) {
					flatpoint center,p2;
					int c;

					 //---center == centroid of points
					for (c=0; c<curpoints.n; c++) center+=curpoints.e[c]->p();
					center/=curpoints.n;

					 //--center == mouse point
					//center=dp->screentoreal(x,y);

					double f=1+(x-oldx)/10.; //*** the scaling factor should be customizable
					scaleSelected(center,f,constrain);
					needtodraw=1;
				}
			} break;

		case (ShiftMask|ControlMask): { // ^+move == rotate curpoints
				if (curpoints.n) {
					flatpoint center,p2;
					int c;

					 //---center == centroid of points
					for (c=0; c<curpoints.n; c++) center+=curpoints.e[c]->p();
					center/=curpoints.n;

					double angle=(x-oldx);
					rotateSelected(center,angle);
					needtodraw=1;
				}
			} break;
		DBG default: cerr<<" pathi->move unknown mod mask: "<<(state&(ControlMask|AltMask|MetaMask|ShiftMask))<<endl; break;
	} //switch

	Modified(0);

// ***** These can be imp as PathInterface modes??
//	*** flip
//	*** circle arrange
//	*** bez arrange
//	*** distribute
//	*** shear
//	*** other arrange??

	return 0;
}

Laxkit::ShortcutHandler *PathInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(PATHIA_CurpointOnHandle,  'h',0,0,        "HandlePoint",  _("Switch between handle and vertex"),NULL,0);
	sc->Add(PATHIA_CurpointOnHandleR, 'H',ShiftMask,0,"HandlePointR", _("Switch between handle and vertex"),NULL,0);
	sc->Add(PATHIA_Pathop,            'o',0,0,        "Pathop",       _("Change path operator"),NULL,0);
	sc->Add(PATHIA_ToggleFillRule,    'F',ShiftMask,0,"ToggleFillRule",_("Toggle fill rule"),NULL,0);
	sc->Add(PATHIA_ToggleFill,        'f',0,0,        "ToggleFill",   _("Toggle fill"),NULL,0);
	sc->Add(PATHIA_ToggleStroke,      's',0,0,        "ToggleStroke", _("Toggle stroke"),NULL,0);
	sc->Add(PATHIA_ColorFillOrStroke, 'x',0,0,        "ColorDest",    _("Send colors to fill or to stroke"),NULL,0);
	sc->Add(PATHIA_RollNext,          LAX_Right,0,0,  "RollNext",     _("Select next points"),NULL,0);
	sc->Add(PATHIA_RollPrev,          LAX_Left,0,0,   "RollPrev",     _("Select previous points"),NULL,0);
	sc->Add(PATHIA_ToggleAddAfter,    'n',0,0,        "AddAfter",     _("Toggle add after"),NULL,0);
	sc->Add(PATHIA_TogglePointType,   'p',0,0,        "PointType",    _("Change point type"),NULL,0);
	sc->Add(PATHIA_Select,            'a',0,0,        "Select",       _("Select all or deselect"),NULL,0);
	sc->Add(PATHIA_SelectInPath,      'A',ShiftMask,0,"SelectInPath", _("Select all in path or deselect"),NULL,0);
	sc->Add(PATHIA_Close,             'c',0,0,        "ToggleClosed", _("Toggle closed"),NULL,0);
	sc->Add(PATHIA_Decorations,       'd',0,0,        "Decorations",  _("Toggle decorations"),NULL,0);
	sc->Add(PATHIA_StartNewPath,      'B',ShiftMask,0,"NewPath",      _("Start a totally new path object"),NULL,0);
	sc->Add(PATHIA_StartNewSubpath,   'b',0,0,        "NewSubpath",   _("Start a new subpath"),NULL,0);
	sc->Add(PATHIA_Wider,             'w',0,0,        "Wider",        _("Thicken the line"),NULL,0);
	sc->Add(PATHIA_Thinner,           'W',ShiftMask,0,"Thinner",      _("Thin the line"),NULL,0);
	sc->Add(PATHIA_WidthStep,         'w',ControlMask,0,"WidthStep",  _("Change how much change for width changes"),NULL,0);
	sc->Add(PATHIA_WidthStepR,        'W',ControlMask|ShiftMask,0,"WidthStepR", _("Change how much change for width changes"),NULL,0);
	sc->Add(PATHIA_MakeCircle,        '0',0,0,        "Circle",       _("Arrange points in a circle"),NULL,0);
	sc->Add(PATHIA_MakeRect,          'R',ShiftMask,0,"Rect",         _("Arrange points in a rectangle"),NULL,0);
	sc->Add(PATHIA_Reverse,           'r',0,0,        "Reverse",      _("Reverse direction of current path"),NULL,0);
	sc->Add(PATHIA_Delete,            LAX_Del,0,0,    "Delete",       _("Delete selected points"),NULL,0);
	sc->AddShortcut(LAX_Bksp,0,0, PATHIA_Delete);

	//sc->Add(PATHIA_Combine,           'k',ControlMask,0,    "Combine",      _("Combine multiple path objects into a single path object"),NULL,0);
	//sc->Add(PATHIA_ExtractPath,       'K',ShiftMask,0,      "ExtractPath",  _("Move paths of current points to a new path object"),NULL,0);
	//sc->Add(PATHIA_ExtractAll,        'K',ControlMask|ShiftMask,0,"ExtractPaths", _("Create new path objects of each subpath"),NULL,0);
	//sc->Add(PATHIA_Copy,              'c',ControlMask,0,    "Copy",         _("Copy current points to clipboard"),NULL,0);
	//sc->Add(PATHIA_Cut,               'x',ControlMask,0,    "Cut",          _("Delete selected points"),NULL,0);
	//sc->Add(PATHIA_Paste,             'v',ControlMask,0,    "Paste",        _("Paste clipboard if it is points"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

int PathInterface::PerformAction(int action)
{
	if (action==PATHIA_CurpointOnHandle || action==PATHIA_CurpointOnHandleR) {
		//toggle between being on a vertex, next point, or prev point
		
		int dir=lasth;
		if (!dir) dir=lasth=(action==PATHIA_CurpointOnHandle?1:-1);
		if (curpoints.n!=1) return 0;

		Coordinate *pp=NULL;
		Coordinate *curpoint=curpoints.e[0];
		int toflag=0, flag=curpoint->flags&(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV);

		 //find destination curpoint
		if (flag==POINT_VERTEX) {
			if (dir==1) toflag=POINT_TOPREV; else toflag=POINT_TONEXT;
		} else toflag=POINT_VERTEX;
		if (flag==POINT_TOPREV) lasth=-1; else if (flag==POINT_TONEXT) lasth=1;

		DBG cerr <<"toflag="<<toflag<<endl;


		 //switch to moving a TOPREV control point
		if (toflag==POINT_TOPREV) {
			if (flag!=POINT_TOPREV) {
				 //make current point be the following TOPREV bezier point
				pp=curpoint;
				if (pp->flags&POINT_TONEXT) pp=pp->next;
				if (pp->flags&POINT_VERTEX) {
					if (pp->next && (pp->next->flags&POINT_TOPREV)) {
						pp=pp->next; //positions on an existing TOPREV
					} else {
						 //no TOPREV, so we have to add one
						Coordinate *np=new Coordinate(pp->p(),POINT_TOPREV,NULL);
						pp->connect(np,1);
						pp=pp->next;
					}
				}
			}

		} else  if (toflag==POINT_TONEXT) {
			if (flag!=POINT_TONEXT) {
				 //make current point be the following bezier point
				pp=curpoint;
				if (pp->flags&POINT_TOPREV) pp=pp->prev;
				if (pp->flags&POINT_VERTEX) {
					if (pp->prev && (pp->prev->flags&POINT_TONEXT)) {
						pp=pp->prev; //positions on an existing TONEXT
					} else {
						 //no TONEXT, so we have to add one
						Coordinate *np=new Coordinate(pp->p(),POINT_TONEXT,NULL);
						pp->connect(np,0);
						pp=pp->prev;
					}
				}
			}

		} else { //toflag==POINT_VERTEX
			pp=curpoint;
			if (flag==POINT_TOPREV) pp=pp->prev; //was on a TOPREV
			else if (flag==POINT_TONEXT) pp=pp->next; //was on a TONEXT
			//else already on a vertex!
		}

		if (pp) {
			curpoints.flush();
			curpoints.push(pp,0);
			needtodraw=1;
		}
		return 0;

	} else if (action==PATHIA_Pathop) {
		 // Select different pathop
		if (Path::basepathops.n==0) return 0;
		int curpop,idofnew=0;

		if (curpathop) { // select next pathop on pathops stack
			curpop=Path::basepathops.findindex(curpathop)+1;
			if (curpop>=Path::basepathops.n) idofnew=0;
				else idofnew=Path::basepathops.e[curpop]->id;
		} else {
			idofnew=Path::basepathops.e[0]->id;
		}

		ChangeCurpathop(idofnew);
		return 0;

	} else if (action==PATHIA_ToggleFillRule) {
		 //toggle fill rule
		if (!data) return 0;

		if (!data->fillstyle) data->fillstyle=new FillStyle(*defaultfill);
		if (data->fillstyle->fillrule==WindingRule) {
			data->fillstyle->fillrule=EvenOddRule;
			PostMessage(_("Fill odd winding"));
		} else {
			data->fillstyle->fillrule=WindingRule; 
			PostMessage(_("Fill all"));
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleFill) {
		 //toggle fill or not
		if (!data) return 0;

		if (data->fillstyle && data->fillstyle->fillstyle!=FillNone) {
			data->fillstyle->fillstyle=FillNone;
			PostMessage(_("Don't fill"));
		} else {
			if (!data->fillstyle) data->fillstyle=new FillStyle(*defaultfill);
			data->fillstyle->fillstyle=FillSolid;
			PostMessage(_("Fill"));
		} 
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleStroke) {
		if (data->linestyle && data->linestyle->function!=0) {
			data->linestyle->function=0;
			PostMessage(_("Don't stroke"));
		} else {
			if (!data->linestyle) data->linestyle=new LineStyle(*defaultline);
			data->linestyle->function=LAXOP_Source;
			PostMessage(_("Stroke"));
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ColorFillOrStroke) {
		 //toggle send colors to fill or to stroke
		colortofill=!colortofill;
		const char *mes=colortofill?_("Send color to fill"):_("Send color to stroke");
		PostMessage(mes);
		return 0;

	} else if (action==PATHIA_RollNext) {
		 // Roll curpoints next
		for (int c=0; c<curpoints.n; c++) {
			if (curpoints.e[c]->next) curpoints.e[c]=curpoints.e[c]->next;
			else if (curpoints.n>1) curpoints.pop(c--);
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_RollPrev) {
		 // Roll curpoints prev
		for (int c=0; c<curpoints.n; c++) {
			if (curpoints.e[c]->prev) curpoints.e[c]=curpoints.e[c]->prev;
			else if (curpoints.n>1) curpoints.pop(c--);
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleAddAfter) {
		 // Toggle addafter
		addafter=!addafter;
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_TogglePointType) {
		 //toggle vertex smoothness type
		SetPointType(-1);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Select) {
		 // If any points are selected then deselect them,
		 // Otherwise select all points
		if (!data) return 1;
		if (curpoints.n) {
			curpoints.flush();
		} else {
			Coordinate *p,*p2=NULL,*pt;
			for (int c=0; c<data->paths.n; c++) {
				p=data->paths.e[c]->path;
				if (p) p2=p=p->firstPoint(0);
				do {
					if (p2) {
						curpoints.push(p2,0);
						if (p2->controls) {
							 //select as curpoint only 1 point in a controlled segment
							pt=p2;
							while (p2 && p2->controls==pt->controls) p2=p2->next;
						} else p2=p2->next;
					}
				} while (p2 && p2!=p);
			}
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_SelectInPath) {
		 // like 'a' but only with the single path containing curvertex
		if (!data || !curvertex) return 0;
		Coordinate *p,*p2=NULL;

		int n=0,c2,cv;
		cv=data->hasCoord(curvertex);
		 // scan for any selected points that are in same path as curvertex
		for (int c=0; c<curpoints.n; c++) {
			c2=data->hasCoord(curpoints.e[c]);
			if (c2==cv) {
				n++;
				curpoints.pop(c);
				c--;
			}
		}
		if (!n && cv>=0) { // push all in curpath
			p=data->paths.e[cv]->path;
			if (p) p2=p=p->firstPoint(0);
			do {
				if (p2) {
					curpoints.push(p2,0);
					if (p2->controls) {
						 //select as curpoint only 1 point in a controlled segment
						Coordinate *pt=p2;
						while (p2 && p2->controls==pt->controls) p2=p2->next;
					} else p2=p2->next;
				}
			} while (p2 && p2!=p);
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Close) {
		 // Close/Open current path at curpoint
		if (!data) return 0;
		toggleclosed();

		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Decorations) {
		 // Turn decorations on/off
		 // *** have different levels of decorations?? show all control points, some control points, etc
		showdecs=!showdecs;
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_StartNewPath) {
		 // free current data, so next lb click makes whole new data.
		if (!data) return 0;
		if (pathi_style&PATHI_One_Path_Only) return 0;
		deletedata();
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_StartNewSubpath) {
		//***
		cout <<" *** start new subpath"<<endl;

	} else if (action==PATHIA_Wider) {
		if (!data) return 0;
		if (!data->linestyle->width) data->linestyle->width=1;
		data->linestyle->width*=widthstep;

		char buffer[100];
		sprintf(buffer,_("Width %.10g"),data->linestyle->width);
		PostMessage(buffer);

		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Thinner) {
		if (!data) return 0;
		if (!data->linestyle->width) data->linestyle->width=1;
		data->linestyle->width/=widthstep;

		char buffer[100];
		sprintf(buffer,_("Width %.10g"),data->linestyle->width);
		PostMessage(buffer);

		needtodraw=1;
		return 0;

	} else if (action==PATHIA_WidthStep || action==PATHIA_WidthStepR) {
		if (action==PATHIA_WidthStep) widthstep*=1.01;
		else  widthstep*=.99;
		if (widthstep<1) widthstep=1.00001;

		char buffer[100];
		sprintf(buffer,_("Width step %.10g"),widthstep);
		PostMessage(buffer);
		return 0;

	} else if (action==PATHIA_MakeCircle) {
		MakeCircle();
		Modified(0);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_MakeRect) {
		MakeRect();
		Modified(0);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Reverse) {
		if (!data || !curpath) return 0;
		data->ReversePath(data->paths.findindex(curpath));
		SetCurvertex(curvertex);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Delete) {
		DeleteCurpoints();
		return 0;
	}

	return 1;
}

int PathInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	 //check for a couple of things before shortcuts
	if (ch==LAX_Control) {
		if (curpoints.n==1 && (curpoints.e[0]->flags&(POINT_TOPREV|POINT_TONEXT))) {
			if (curpoints.e[0]->flags&POINT_TOPREV) lasth=-1; else lasth=1;
			//PerformAction(PATHIA_CurpointOnHandle);
			Coordinate *p=curvertex;
			curpoints.flush();
			if (p) curpoints.push(p,0);
			needtodraw=1;
			return 0;
		}
		return 1;

	} else 	if (ch==LAX_Esc) {
		if (curpoints.n)  { 
			curpoints.flush();
			//SetCurvertex(NULL);
			//curpath=NULL;
			needtodraw=1;
			return 0;
		}
		if (owner && (pathi_style&PATHI_Esc_Off_Sub)) {
			owner->RemoveChild();
			return 0;
		}
		return 1;

	} else if (ch==LAX_Enter) {
		if (owner && (pathi_style&PATHI_Esc_Off_Sub)) {
			owner->RemoveChild();
			return 0;
		}

	}

	 //check shortcuts
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1;
}

int PathInterface::KeyUp(unsigned int ch,unsigned int state, const LaxKeyboard *kb)
{
	if (ch==LAX_Control) {
		if (curpoints.n==1 && (curpoints.e[0]->flags&POINT_VERTEX)) {
			if (lasth==1) lasth=-1; else lasth=1;
			PerformAction(PATHIA_CurpointOnHandle);
			needtodraw=1;
			return 0;
		}
	}
	return 1;
}

void PathInterface::MakeRect()
{
    for (int c=curpoints.n-1; c>=0; c--) { //remove control points, leaving only vertices
        if (!(curpoints.e[c]->flags&POINT_VERTEX)) curpoints.pop(c);
    }

	 //find rect bounds
	DoubleBBox bounds;
	for (int c=0; c<curpoints.n; c++) bounds.addtobounds(curpoints.e[c]->p());

	 //arrange in rect of bounds
	Coordinate *p;
	int peredge =curpoints.n/4;
	int leftover=curpoints.n%4;
	int i=0;

	 //top edge
	int en=peredge;
	if (leftover) { en++; leftover--; }
	for (int c=0; c<en; c++) {
		p=curpoints.e[i++];
		if (p->prev && p->prev->flags&POINT_TONEXT) DeletePoint(p->prev);
		if (p->next && p->next->flags&POINT_TOPREV) DeletePoint(p->next);
		p->p(flatpoint(bounds.minx+c*(bounds.maxx-bounds.minx)/en,bounds.maxy));
	}

	 //right edge
	en=peredge;
	if (leftover) { en++; leftover--; }
	for (int c=0; c<en; c++) {
		p=curpoints.e[i++];
		if (p->prev && p->prev->flags&POINT_TONEXT) DeletePoint(p->prev);
		if (p->next && p->next->flags&POINT_TOPREV) DeletePoint(p->next);
		p->p(flatpoint(bounds.maxx,bounds.maxy-c*(bounds.maxy-bounds.miny)/en));
	}

	 //bottom edge
	en=peredge;
	if (leftover) { en++; leftover--; }
	for (int c=0; c<en; c++) {
		p=curpoints.e[i++];
		if (p->prev && p->prev->flags&POINT_TONEXT) DeletePoint(p->prev);
		if (p->next && p->next->flags&POINT_TOPREV) DeletePoint(p->next);
		p->p(flatpoint(bounds.maxx-c*(bounds.maxx-bounds.minx)/en,bounds.miny));
	}

	 //left edge
	en=peredge;
	if (leftover) { en++; leftover--; }
	for (int c=0; c<en; c++) {
		p=curpoints.e[i++];
		if (p->prev && p->prev->flags&POINT_TONEXT) DeletePoint(p->prev);
		if (p->next && p->next->flags&POINT_TOPREV) DeletePoint(p->next);
		p->p(flatpoint(bounds.minx,bounds.miny+c*(bounds.maxy-bounds.miny)/en));
	}
}

//! Make selected points into a bezier approximated circle.
/*! \todo need to weed out those points that have segment controls
 */
void PathInterface::MakeCircle()
{
    if (!data) return;

     // first make curpoints have only control points
    int c;
    for (c=curpoints.n-1; c>=0; c--) { //remove control points, leaving only vertices
        if (!(curpoints.e[c]->flags&POINT_VERTEX)) curpoints.pop(c);
    }
    if (curpoints.n<2) return;
    //if (curpoints.n==0) for (c=1; c<data->npoints; c+=3) curpoints.push(c);  //select all if none selected

     // should sort curpoints, or transformation looks bizarre
    //***

     // next find bounding box for those points
    double minx,maxx,miny,maxy;
    minx=maxx=curpoints.e[0]->x();
    miny=maxy=curpoints.e[0]->y();
    for (c=1; c<curpoints.n; c++) {
        if (curpoints.e[c]->x()<minx) minx=curpoints.e[c]->x();
        else if (curpoints.e[c]->x()>maxx) maxx=curpoints.e[c]->x();
        if (curpoints.e[c]->y()<miny) miny=curpoints.e[c]->y();
        else if (curpoints.e[c]->y()>maxy) maxy=curpoints.e[c]->y();
    }
    double w = (maxx - minx)/2,
        h=(maxy-miny)/2,
        theta=2*3.141592653589/(curpoints.n),
        r,v;
    r=(w>h?w:h);


    v=4*r*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta));
    DBG cerr <<"MakeCircle: r="<<r<<" theta="<<theta/3.1415926535*180<<" v="<<v<<endl;

    flatpoint center=flatpoint((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);
	flatpoint vert;
    double xx,yy;
    for (c=0; c<curpoints.n; c++) {
        xx=cos(c*theta);
        yy=sin(c*theta);
		vert=center + flatpoint(r*xx,r*yy);

        curpoints.e[c]->p(vert); //vertex

		if (!(curpoints.e[c]->next && curpoints.e[c]->next->flags&POINT_TOPREV)) {
			 //no TOPREV, so we have to add one
			Coordinate *np=new Coordinate(flatpoint(0,0),POINT_TOPREV,NULL);
			curpoints.e[c]->connect(np,1);
		}
		curpoints.e[c]->next->p(vert + flatpoint(-v*yy,v*xx)); //next

		if (!(curpoints.e[c]->prev && curpoints.e[c]->prev->flags&POINT_TONEXT)) {
			 //no TONEXT, so we have to add one
			Coordinate *np=new Coordinate(flatpoint(0,0),POINT_TONEXT,NULL);
			curpoints.e[c]->connect(np,0);
		}
        curpoints.e[c]->prev->p(vert + flatpoint(v*yy,-v*xx));
    }
    DBG cerr <<endl;

	 //right now, curpoints is only vertex points, add control points??
	int n=curpoints.n;
	for (int c=0; c<n; c++) {
		if (curpoints.e[c]->prev && (curpoints.e[c]->prev->flags&POINT_TONEXT)) curpoints.pushnodup(curpoints.e[c]->prev,0);
		if (curpoints.e[c]->next && (curpoints.e[c]->next->flags&POINT_TOPREV)) curpoints.pushnodup(curpoints.e[c]->next,0);
	}

    needtodraw=1;
}



} // namespace LaxInterfaces

