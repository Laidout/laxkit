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
//    Copyright (C) 2004-2007,2010-2016 by Tom Lechner
//


#include <lax/interfaces/somedataref.h>
#include <lax/interfaces/groupdata.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/shapebrush.h>
#include <lax/interfaces/svgcoord.h>
#include <lax/laxutils.h>
#include <lax/bezutils.h>
#include <lax/transformmath.h>
#include <lax/colorevents.h>
#include <lax/language.h>


using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG


//for use in selectPoint():
#define PSELECT_FlushPoints  (1<<0)
#define PSELECT_PushPoints   (1<<1)
#define PSELECT_SelectPathop (1<<2)
#define PSELECT_SyncVertex   (1<<3)


namespace LaxInterfaces {


//------------------------------------- various Path helper stuff -------------------------------------

enum PathHoverType {
	HOVER_None=0,
	HOVER_MaybeNewPoint, 
	HOVER_Point,
	HOVER_Vertex,
	HOVER_AddingPoint, //helps keep track of whether we add a vertex only, or point with controls
	HOVER_Handle,
	HOVER_AddPointOn, //for adding point under mouse to existing path
	HOVER_Endpoint,
	HOVER_MergeEndpoints,
	HOVER_AddSegment,
	HOVER_CutSegment,
	HOVER_Direction,
	HOVER_DirectionSelect,
	HOVER_AddToSelection,
	HOVER_RemoveFromSelection,
	HOVER_AddWeightNode,
	HOVER_RemoveWeightNode,
	HOVER_Weight,
	HOVER_WeightOffset,
	HOVER_WeightTop,
	HOVER_WeightBottom,
	HOVER_WeightPosition,
	HOVER_WeightAngle,
	HOVER_MAX
};


//------------------------------------- PathCoordinate -------------------------------------
 /* *** Is this useful at all? idea would be to only update cache on affected points,
  * thus all cache stored with in PathCoordinate objects, not in Path....
  * for complicated paths with custom caps/joins and irregular dash patterns
  * maybe better?
  */
class PathCoordinate : public Coordinate
{
  public:
	int cache_n;
	flatpoint *cache_p;
};


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


//------------------------------------- PathWeightNode -------------------------------------
PathWeightNode::PathWeightNode()
{
	width=1;
	offset=0;
	angle=0;
	t=.5;
	type=Default;
	//cache_status=-1; 

	center_t=-1;
}

PathWeightNode::PathWeightNode(double nt,double noffset, double nwidth, double nangle, int ntype)
{
	width = nwidth;
	offset= noffset;
	angle = nangle;
	t     = nt;
	type  = ntype;
	//cache_status=-1;

	center_t=-1; 
}


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
 *
 *  Note for Inkscape users, a knot in Inkscape is equivalent to a vertex here.
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
{
	// cache_mod_time=0;
	needtorecache = 1;
	cache_types   = 0;  // if 1, then also compute cache_top and cache_bottom
	cache_samples = 10;
	defaultwidth  = 10. / 72;
	absoluteangle = false;
	profile       = nullptr;
	brush         = nullptr;
	generator_data = nullptr;

	// save_cache=true;
	save_cache = false;
}

/*! Incs count of linestyle.
 */
Path::Path(Coordinate *np,LineStyle *nls)
 : Path()
{
	path = np;
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
	if (profile) profile->dec_count();
	if (brush) brush->dec_count();
	if (generator_data) generator_data->dec_count();
}

//! Flush all points.
void Path::clear()
{
	delete path;
	path=NULL;
	pathweights.flush();
	// if (profile) { profile->dec_count(); profile = nullptr; }
	needtorecache=1;
}

Path *Path::duplicate()
{
	Path *newpath = new Path(NULL,linestyle);
	if (profile) newpath->ApplyLineProfile(profile, true);
	newpath->defaultwidth = defaultwidth;
	if (brush) newpath->UseShapeBrush(brush);
	if (path) newpath->path = path->duplicateAll();

	for (int c=0; c<pathweights.n; c++) {
		newpath->AddWeightNode(pathweights.e[c]->t,pathweights.e[c]->offset,pathweights.e[c]->width,pathweights.e[c]->angle);
	}

	return newpath;
}

/*! Attach a LineProfile. If linked, then path must be updated whenever points change.
 * If !linked, then apply the profile as width nodes immediately, overwriting any existing weight nodes.
 */
int Path::ApplyLineProfile(LineProfile *p, bool linked)
{
	if (profile != p) {
		if (profile) profile->dec_count();
		profile = p;
		if (profile) profile->inc_count();
	}
	if (!linked) ApplyLineProfile();
	needtorecache = 1;
	return 1;
}

/*! Apply current profile. Return 1 for success, 0 for no profile to apply.
 */
int Path::ApplyLineProfile()
{
	if (!profile) return 0;
	pathweights.flush();

	for (int c=0; c<profile->pathweights.n; c++) {
		PathWeightNode *o = profile->pathweights.e[0];
		PathWeightNode *w = new PathWeightNode(o->t, o->offset, o->width, o->angle, o->type);
		pathweights.push(w,1);
	}

	needtorecache = 1;
	return 1;
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
void Path::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
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
		fprintf(f,"%sweight 1.5 0 1 0        #zero or more weight nodes.\n",spc);
		fprintf(f,"%s                        #numbers are (t bez parameter) (offset from normal path) (width) (angle, optional)\n",spc);
		fprintf(f,"%ssegment controllername  #a non-straight-line and non-bezier segment\n",spc);
		fprintf(f,"%s  asbezier p 3 5 n 2 4 5 6  # Important! bezier approximation of the segment.\n",spc);
		fprintf(f,"%s                            # if not present and the controller cannot be found,\n",spc);
		fprintf(f,"%s                            # then this segment is ignored.\n",spc);
		fprintf(f,"%s  ...                   #any further attributes defining the segment. exactly what\n",spc);
		fprintf(f,"%s                        #they are is dependent of the actual controller of the segment.\n",spc);
		fprintf(f,"%s                        #If for some reason the segment controller is not found, then\n",spc);
		fprintf(f,"%s                        #the bezier approximation is appended to the path as bezier points\n",spc);
		fprintf(f,"%scache                   #This is for reference only. Ignored on read in.\n",spc);
		fprintf(f,"%s  ...                   #This is for reference only. Ignored on read in.\n",spc);
		fprintf(f,"%sprofile                 #Optional line profile\n",spc);
		return;
	}

	if (!path) return;

	if (linestyle) {
		fprintf(f,"%slinestyle\n",spc);
		linestyle->dump_out(f,indent+2,what, context);
	}
	if (path==path->lastPoint(0)) fprintf(f,"%sclosed\n",spc);

	if (profile) {
		fprintf(f,"%sprofile\n",spc);
		profile->dump_out(f, indent+2, what, context);
	}

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
					if (pt==0) pt=BEZ_STIFF_EQUAL;
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

	for (int c=0; c<pathweights.n; c++) {
		fprintf(f,"%sweight %.10g %.10g %.10g %.10g\n",spc,
				pathweights.e[c]->t,pathweights.e[c]->offset,pathweights.e[c]->width,pathweights.e[c]->angle);
	}

	if (save_cache) { 
		fprintf(f,"%scache outline\n",spc);
		for (int c=0; c<outlinecache.n; c++) { 
			fprintf(f, "%s  %.10g, %.10g   %d  ", spc, outlinecache.e[c].x, outlinecache.e[c].y, c);
			if (outlinecache.e[c].info&LINE_Start   ) fprintf(f,"Start    ");
			if (outlinecache.e[c].info&LINE_Vertex  ) fprintf(f,"Vertex   ");
			if (outlinecache.e[c].info&LINE_Bez     ) fprintf(f,"Bez      ");
			if (outlinecache.e[c].info&LINE_Closed  ) fprintf(f,"Closed   ");
			if (outlinecache.e[c].info&LINE_Open    ) fprintf(f,"Open     ");
			if (outlinecache.e[c].info&LINE_End     ) fprintf(f,"End      "); 
			if (outlinecache.e[c].info&LINE_Corner  ) fprintf(f,"Corner   ");
			if (outlinecache.e[c].info&LINE_Equal   ) fprintf(f,"Equal    ");
			if (outlinecache.e[c].info&LINE_Auto    ) fprintf(f,"Auto     ");
			if (outlinecache.e[c].info&LINE_Join    ) fprintf(f,"Join     ");
			if (outlinecache.e[c].info&LINE_Cap     ) fprintf(f,"Cap      ");
			if (outlinecache.e[c].info&LINE_Original) fprintf(f,"Original ");
			fprintf(f,"\n");
		}
	}
}

Laxkit::Attribute *Path::dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context)
{ 
	if (what==-1) {
		if (!att) att=new Attribute;
		att->push("linestyle", nullptr, "standard linestyle attributes");
		att->push("closed",nullptr,"flag to indicate that this path is a closed path");
		att->push("points", "1 2       \n"
							"vs 1 2    \n"
							"vS 1 2    \n"
							"vc 1 2    \n"
							"ve 1 2    \n"
							"p 1.5 2.5 \n"
							"n 3 5     \n",    "v is vertex, p or n is control for previous or next, s smooth, S really smooth, c corner, e corner-equal");
		att->push("weight", "1.5 0 1 0",     "zero or more weight nodes. Numbers are (t bez parameter) (offset from normal path) (width) (angle, optional)");
		//att->push("segment controllername","a non-straight-line and non-bezier segment");
		//att->push("  asbezier p 3 5 n 2 4 5 6  # Important! bezier approximation of the segment.");
		//att->push("                            # if not present and the controller cannot be found,");
		//att->push("                            # then this segment is ignored.");
		//att->push("  ...                   #any further attributes defining the segment. exactly what");
		//att->push("                        #they are is dependent of the actual controller of the segment.");
		//att->push("                        #If for some reason the segment controller is not found, then");
		//att->push("                        #the bezier approximation is appended to the path as bezier points");
		att->push("cache", nullptr, "This is for reference only. Ignored on read in.");
		att->push("profile", nullptr, "Optional line profile");

		return att;
	}

	if (!path) return att;

	if (!att) att=new Attribute;

	if (linestyle) {
		Attribute *att2=att->pushSubAtt("linestyle");
		linestyle->dump_out_atts(att2, what, context);
	}

	if (profile) {
		Attribute *att2 = att->pushSubAtt("profile");
		profile->dump_out_atts(att2, what, context);
	}

	if (path==path->lastPoint(0)) att->push("closed");

	Coordinate *p=path, *p2=NULL;
	char scratch[100];
	char *pstr=NULL;
	int n;

	do {
		 //do a string of bezier points
		p2=p;
		n=0;
		while (!p2->controls) { p2=p2->next; n++; if (!p2 || p2==path) break; }
		 //now p2 points to the point just after the last uncontrolled vertex

		if (n>0) {
			 //output straight line segments for a seemingly long string of vertex points

			do {
				if (p->flags&POINT_VERTEX) {
					int pt=p->flags&BEZ_MASK;
					if (pt==0) pt=BEZ_STIFF_EQUAL;
					if (pt==BEZ_STIFF_EQUAL) pt='S';
					else if (pt==BEZ_STIFF_NEQUAL) pt='s';
					else if (pt==BEZ_NSTIFF_EQUAL) pt='e';
					else if (pt==BEZ_NSTIFF_NEQUAL) pt='c';
					else pt=0;

					 
					if (pt) sprintf(scratch, "v%c %.10g %.10g\n", pt, p->x(),p->y());
					else sprintf(scratch, "%.10g %.10g\n", p->x(),p->y());
					appendstr(pstr, scratch);

				} else if (p->flags&POINT_TOPREV) {
					sprintf(scratch, "p %.10g %.10g\n", p->x(),p->y());
					appendstr(pstr, scratch);

				} else if (p->flags&POINT_TONEXT) {
					sprintf(scratch, "n %.10g %.10g\n", p->x(),p->y());
					appendstr(pstr, scratch);
				}

				p=p->next;
			} while (p!=p2);

			att->push("points", pstr);
			delete[] pstr; // *** really need more intelligent char[] caching.. this is really memory inefficient
			continue;
		}

		 //check for controlled points
		if (p->controls) {
			cerr << " *** lazy developer! need to implement Path::dump_out_atts for non-null segment controls!"<<endl;
			//p2=p;
			//while (p2->controls==p->controls) { p2=p2->next; if (!p2 || p2==path) break; }
		 	// //now p2 points to the point just after the last point with same controller
            //
			//int c2;
			//for (c2=0; c2<basepathops.n; c2++) if (p2->controls->iid()==basepathops.e[c2]->id) break;
			//if (c2==basepathops.n) {
			//	 //ignore if pathop not found during runtime!! should never happen anyway
			//	p=p2;
			//	continue;
			//}
			//att->push("segment %s",basepathops.e[c2]->whattype());
			//basepathops.e[c2]->dumpOut(f, indent+2, p->controls, 0, context);
			//p=p2;
		} 

	} while (p && p!=path);

	for (int c=0; c<pathweights.n; c++) {
		sprintf(scratch, "%.10g %.10g %.10g %.10g",
				pathweights.e[c]->t,pathweights.e[c]->offset,pathweights.e[c]->width,pathweights.e[c]->angle);
		att->push("weight", scratch);
	}

	if (save_cache) { 
		char *cstr=newstr("outline\n");

		for (int c=0; c<outlinecache.n; c++) { 
			sprintf(scratch, "%.10g, %.10g   %d  ", outlinecache.e[c].x, outlinecache.e[c].y, c);
			appendstr(cstr, scratch);

			if (outlinecache.e[c].info&LINE_Start   ) appendstr(cstr,"Start    ");
			if (outlinecache.e[c].info&LINE_Vertex  ) appendstr(cstr,"Vertex   ");
			if (outlinecache.e[c].info&LINE_Bez     ) appendstr(cstr,"Bez      ");
			if (outlinecache.e[c].info&LINE_Closed  ) appendstr(cstr,"Closed   ");
			if (outlinecache.e[c].info&LINE_Open    ) appendstr(cstr,"Open     ");
			if (outlinecache.e[c].info&LINE_End     ) appendstr(cstr,"End      "); 
			if (outlinecache.e[c].info&LINE_Corner  ) appendstr(cstr,"Corner   ");
			if (outlinecache.e[c].info&LINE_Equal   ) appendstr(cstr,"Equal    ");
			if (outlinecache.e[c].info&LINE_Auto    ) appendstr(cstr,"Auto     ");
			if (outlinecache.e[c].info&LINE_Join    ) appendstr(cstr,"Join     ");
			if (outlinecache.e[c].info&LINE_Cap     ) appendstr(cstr,"Cap      ");
			if (outlinecache.e[c].info&LINE_Original) appendstr(cstr,"Original ");
			appendstr(cstr,"\n");
		}

		att->push("cache", cstr);
	}

	return att;
}

/*! Dump in the linestyle and coordinates..
 *
 * If a linestyle is listed, then dec_count of old and install a new one.
 *
 * See dump_out() for format expected.
 */
void Path::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
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

		} else if (!strcmp(name,"weight")) {
			double d[4];
			int c2=DoubleListAttribute(value,d,4,NULL);
			if (c2==3) { d[3]=0; c2=4; } //add angle if not there
			if (c2!=4) continue;

			AddWeightNode(d[0],d[1],d[2],d[3]);

		} else if (!strcmp(name,"profile")) {
			cerr << " *** NEED TO IMPLEMENT Path::profile dump in !!"<<endl;

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
	// *** should probably sanitize the weight list
	needtorecache=1;
}

int Path::UseShapeBrush(ShapeBrush *newbrush)
{
	if (newbrush == brush) return 0;
	if (brush) brush->dec_count();
	brush = newbrush;
	if (brush) {
		brush->inc_count();
	}
	needtorecache = 1;

	return 0;
}

/*! Return 0 for success.
 */
int Path::Line(LineStyle *nlinestyle)
{
	if (nlinestyle == linestyle) return 0;
	if (linestyle) linestyle->dec_count();
	linestyle = nlinestyle;
	if (linestyle) {
		defaultwidth = linestyle->width;
		linestyle->inc_count();
	}
	needtorecache=1;

	return 0;
}

/*! Set the color of the path.
 * Note this will create a new linestyle if currently NULL, but if it
 * exists, it will just put it in. Anything else using that linestyle will be affected.
 *
 * Return 0 if color set, else 1.
 */
int Path::LineColor(Laxkit::ScreenColor *ncolor)
{
	if (!ncolor) return 1;

	if (!linestyle) {
		linestyle=new LineStyle;
		linestyle->width=defaultwidth;
	}
	linestyle->color=*ncolor;

	return 0;
}

/*! For any vertex point where point->next_s<=0, compute the length of the next segment.
 * If all, then force recomputation of all segment lengths.
 *
 * These s computations should be considered rough approximations.
 */
void Path::UpdateS(bool all, int resolution)
{
	 // *** this maybe should compute on the offset path, not the base path

	if (!path) return;

	Coordinate *p=path,*start=path, *p2;
	flatpoint c1,c2;

	do {
		p2=p->next;
		if (!p2) break;

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
				if (!p2) break;
			} else { //otherwise, should be a vertex
				//p2=p2->next;
				c2=p2->p();
			}
		}

		if (all || p->next_s<=0) {
			p->next_s = bez_segment_length(p->p(),c1,c2,p2->p(), resolution);
		}

		p=p->nextVertex(0);
	} while (p && p!=start);
}


// ********************** PUT SOMEWHERE USEFUL!!!! Vvvvvvvvvvv

/*! list contains possibly a mixture of bare sample points and bezier segments.
 * list can have one or more paths within, terminated by points that have LINE_Closed
 * or LINE_Open in their info.
 *
 * If epsilon>0, then list will be adjusted so that the number of bare sample points is reduced by
 * the Ramer–Douglas–Peucker polyline reduction algorithm. If epsilon<0, then use a default
 * value. If epsilon==0, then do not reduce.
 *
 * Any strings of bare points in list will get a previous bezier control point, and a
 * next control point attached to it.
 *
 * Any points tagged with LINE_Join or LINE_Cap are meant to have the join inserted AFTER calling this
 * function. The point immediately after that point is assumed to be the next on-line point.
 */
int bez_reduce_approximate(NumStack<flatpoint> *list, double epsilon)
{
	DBG cerr <<"....start bez_reduce_approximate...."<<endl;

	if (epsilon<0) epsilon=1e-5;

	int thisclosed=0;
	int thisstart=0, thisend;
	NumStack<flatpoint> bsamples;

	for (int c=0; c<list->n; c=thisend+1) { //one loop per line in list

		 //so as not to do buffer overruns below,
		 //need to find length of current line, which might not be list->n
		thisstart=c;
		for (int c2=thisstart; c2<list->n; c2++) { 
			if      (list->e[c2].info&LINE_Closed) thisclosed= 1;
			else if (list->e[c2].info&LINE_Open)   thisclosed=-1;
			else if (c2==list->n-1) thisclosed=-1;
			else thisclosed=0;

			if (thisclosed!=0) {
				thisend=c2;
				break;
			}
		}

		 //the current subpath is contained in thisstart...thisstart+thisn.
		 //Now we need to find the segments of this subpath that need to be approximated.
		 //It is assumed that segments with LINE_Bez are already approximated, and are skipped.
		int segstart=thisstart, segend=-1, segn;
		int diffn;
		int wrap;

		for (int c2=thisstart; c2<=thisend; c2++) { //one loop per segment
			 //skip bez segments
			segstart=c2;
			while (c2<thisend && (list->e[c2].info&LINE_Bez)) { c2++; segstart=c2; }

			// **** sometimes we need to wrap around, a gnarly situation!
			//if (c2==thisstart && thisclosed==1 && (list->e[c2].info&LINE_Corner)==0) {
			//	 //need to do special check for wrapping backwards
			//	segstart=thisend;
			//	while (segstart>c2 && (list->e[segstart].info&(LINE_Corner|LINE_Join))==0) segstart--;
			//}
			//
			//if (c2==thisend) {
			//	if (thisclosed==1) wraps=1; else wraps=0;
			//}


			 //now we should be on either a raw point or a vertex, need to find end of segment
			
			if (c2==thisend) break; //at end
			if (list->e[c2].info&(LINE_Join|LINE_Cap)) continue; //need to skip the join gap, it will be filled in later

			c2++; //position one past segstart
			if (list->e[c2].info&LINE_Bez) continue; //need to skip bez segments

			 //find all raw points following segstart
			while (c2<thisend && (list->e[c2].info&(LINE_Corner|LINE_Cap|LINE_Join|LINE_Open|LINE_Closed)) == 0) {
				c2++;
			}

			//DBG if (list->e[c2].info&LINE_Cap) cerr <<"Line cap at: "<<c2<<endl;
			//DBG dump_points("seg found:", list->e+segstart, c2-segstart+1, segstart);

			segend=c2;
			segn=segend-segstart+1;

			if (segn<=2) continue; //segment was just a point or a single line, so go to next


			//DBG cerr <<"------list.n="<<list->n<<"   thisstart="<<thisstart<<"   thisend="<<thisend<<endl;
			//DBG cerr <<"   segstart="<<segstart<<"   segend="<<segend<<"   segn="<<segn<<endl;

			//DBG for (int cc=0; cc<list->n; cc++) { list->e[cc].info2=cc; }

			 //reduce as necessary
//			diffn=0;
//			if (epsilon>0) {
//				dump_points("list before:",list->e,list->n);
//				int newn=reduce_polyline(list->e, list->e+segstart, segn, epsilon);
//				diffn=segn-newn;
//				if (diffn && c2!=list->n-1)
//					memmove(list->e+segstart+newn, list->e+segend+1, (list->n-segend-1)*sizeof(flatpoint));
//				list->n-=diffn;
//				thisend-=diffn;
//				c2     -=diffn;
//
//				dump_points("list after:",list->e,list->n);
//			}


			 //bezier approximate
			bsamples.flush_n();
			bsamples.Allocate(3*segn);
			bsamples.n=3*segn;
			bez_from_points(bsamples.e, list->e+thisstart,thisend-thisstart+1, segstart-thisstart,segn);
			//bez_from_points(bsamples.e, list->e,list->n, segstart,segn);
			bsamples.e[bsamples.n-2].info&=~(LINE_Closed|LINE_Open);
			//DBG dump_points("bsamples",bsamples.e,bsamples.n);

			//DBG for (int cc=0; cc<bsamples.n; cc++) { bsamples.e[cc].info2=1000+cc; }
			//DBG cerr <<"   bsamples.n="<<bsamples.n<<endl;
			//DBG dump_points("list before",list->e,list->n);

			wrap=0;
			bool onbreak=((list->e[thisend].info&(LINE_Join|LINE_Cap))!=0);
			if (segstart==thisstart && thisclosed==1 && !onbreak) wrap|=1;
			if (segend==thisend     && thisclosed==1 && !onbreak) wrap|=2;


			// *** should do only one memmove, combine with epsilon above
			list->Allocate(list->n+2*segn);
			if (c2!=list->n-1) {
				 //move points beyond current segment out past where new points must go
				//memmove(list->e+segstart + 3*segn-2 + (((wrap&1)?1:0)+((wrap&2)?1:0)),  list->e+c2+1, (list->n-c2-1)*sizeof(flatpoint));
				//----------
				int num = (list->n-c2-1);
				int from = c2+1;
				int to = segstart + 3*segn-2 + (((wrap&1)?1:0)+((wrap&2)?1:0));
				if (from > to) for (int c3=0; c3<num; c3++) list->e[to+c3] = list->e[from+c3];
				else for (int c3=num-1; c3>=0; c3--) list->e[to+c3] = list->e[from+c3];

				//DBG cerr <<" memmove( list->e + "<<segstart + 3*segn-2 + (((wrap&1)?1:0)+((wrap&2)?1:0))<<", "<<c2+1<<", "<<(list->n-c2-1)<<"*sizeof(flatpoint))"<<endl;
			}
			//memcpy(list->e+segstart, bsamples.e + ((wrap&1) ? 0 : 1), (3*segn-(((wrap&1)?0:1)+((wrap&2)?0:1)))*sizeof(flatpoint));
			for (int c3=0; c3<(3*segn-(((wrap&1)?0:1)+((wrap&2)?0:1))); c3++) list->e[segstart+c3] = bsamples.e[((wrap&1) ? 0 : 1) + c3];
			//DBG cerr <<" memcpy( list->e + "<<segstart<<", bsamples+"<<((wrap&1) ? 0 : 1)<<", "<<(3*segn-(((wrap&1)?0:1)+((wrap&2)?0:1)))<<"*sizeof(flatpoint))"<<endl;

			diffn=2*segn-2+(((wrap&1)?1:0)+((wrap&2)?1:0));

			list->n+=diffn;
			thisend+=diffn;
			c2     +=diffn;

			if (c2==thisend) {
				if (thisclosed==1) list->e[c2].info|=LINE_Closed;
				else list->e[c2].info|=LINE_Open; 
			}


			//DBG dump_points("list after",list->e,list->n);
		}

		c=thisend;
	}

	DBG cerr <<"....end bez_reduce_approximate...."<<endl;

	return 0;
}

//replace(dest, dstart, dend,   source, sstart, send)
//{
//	dn-1-(dend+1)+1 = dn-dend-1
//	mv   dend+1 .. dn-1  to  dstart + (dend-dstart+1)-(send-sstart+1)
//						 to  dend + 1 - (send-sstart+1) = dend - (send-sstart)
//	cp   dstart,  sstart,send
//}

// ^^^^^^^^^^^^^^^^^^^ PUT SOMEWHERE USEFUL!!!! ^^^^^^^^


/*! Rebuild cache_angle, cache_offset, cache_width.
 */
void Path::UpdateWidthCache()
{
	Coordinate *p=path->firstPoint(1);
	if (!(p->flags&POINT_VERTEX)) { // is degenerate path: no vertices
		DBG cerr <<"Degenerate path (shouldn't happen!)"<<endl;
		return;
	}

	bool hasangle=Angled();

	cache_angle .Reset(true); //removes all points and leaves blank
	cache_offset.Reset(true);
	cache_width .Reset(true);
	outlinecache.flush();
	centercache .flush();

	flatpoint wtop,wbottom;
	flatpoint woffset,wwidth;

	double ymax=0,ymin=0;
	double amax=0,amin=0;
	bool isclosed;


	 //---------------------------------
	 //
	 //first find the number of vertex points in the line, which
	 //is the number of bezier segments in the line, which also
	 //is the x bounds of the width curve.
	int n=NumVertices(&isclosed);

	if (!isclosed) n--; //path is open, so num_segments = num_vertices - 1
	if (n==0) return; //single point, open path == no path!

	 //set the x range for the cached curves
	cache_offset.SetXBounds(0,n,NULL,false);
	cache_width .SetXBounds(0,n,NULL,false);
	cache_angle .SetXBounds(0,n,NULL,false);

	cache_offset.Wrap(isclosed);
	cache_width .Wrap(isclosed);
	cache_angle .Wrap(isclosed);

	 //gather bounds and points to define the offset curves
	if (pathweights.n==0) {
		 //no actual weight nodes
		woffset=flatpoint(0,0);
		wwidth =flatpoint(0,linestyle ? linestyle->width : defaultwidth);
		ymin=ymax=woffset.y;
		if (wwidth.y>ymax) ymax=wwidth.y;
		else if (wwidth.y<ymin) ymin=wwidth.y;
		amin=amax=0;

	} else {
		amin=amax=pathweights.e[0]->angle;

		for (int c=0; c<pathweights.n; c++) {
			woffset=flatpoint(pathweights.e[c]->t, pathweights.e[c]->offset);
			wwidth =flatpoint(pathweights.e[c]->t, pathweights.e[c]->width);

			if (woffset.y>ymax) ymax=woffset.y;
			if (woffset.y<ymin) ymin=woffset.y;
			if (wwidth.y>ymax)  ymax=wwidth.y;
			if (wwidth.y<ymin)  ymin=wwidth.y;

			if (hasangle) {
				if (pathweights.e[c]->angle>amax) amax=pathweights.e[c]->angle;
				if (pathweights.e[c]->angle<amin) amin=pathweights.e[c]->angle;
			}
		}
	}

	 //finalize width and offset bounds, for simplicity, use same bounds, since they are approximately on same scale
	if (ymax==ymin) ymin=ymax-1;
	ymax+=(ymax-ymin)*.25;
	ymin-=(ymax-ymin)*.25;

	cache_offset.SetYBounds(ymin,ymax,NULL,true);
	cache_width .SetYBounds(ymin,ymax,NULL,true);

	 //finalize bounds of cache_angle
	if (amax==amin) amin=amax-1;
	double aamax=amax;
	amax+=(amax-amin)*.25;
	amin-=(amax-amin)*.25;
	cache_angle.SetYBounds(amin,amax,NULL,false);

	 //Now we have the bounds of the curve info, now need to push the actual points
	if (!hasangle) cache_angle.SetFlat(aamax);

	if (pathweights.n==0) {
		if (hasangle) cache_angle.AddPoint(0,0);
		cache_width .AddPoint(0,linestyle ? linestyle->width : defaultwidth);
		cache_offset.AddPoint(0,0);

	} else {
		for (int c=0; c<pathweights.n; c++) {
			if (hasangle) cache_angle.AddPoint(pathweights.e[c]->t,pathweights.e[c]->angle);

			cache_offset.AddPoint(pathweights.e[c]->t,pathweights.e[c]->offset);
			cache_width .AddPoint(pathweights.e[c]->t,pathweights.e[c]->width); 
		}
	}

	if (!isclosed && linestyle && (linestyle->capstyle == LAXCAP_Zero_Width || linestyle->endcapstyle == LAXCAP_Zero_Width)) {
		 //add zero width to start and end for this cap style
		if (linestyle->capstyle == LAXCAP_Zero_Width) 
			cache_width.AddPoint(0,0);
		if (linestyle->endcapstyle == LAXCAP_Zero_Width 
				|| (linestyle->endcapstyle == 0 && linestyle->capstyle == LAXCAP_Zero_Width)) {
			cache_width.AddPoint(n,0);
		}
	}
}

/*! Does nothing if needtorecache==0.
 * Otherwise rebuild outlinecache and centercache. centercache is the center of the stroke,
 * inside of which is to be filled. For nonweighted, non-offset paths, this is the same as the base line.O
 *
 * \todo this could be more intelligent in having cache tied to points somehow, so that you only
 *    need to update cache for affected areas. otherwise this will bog down when point count is high
 * \todo need to handle dash patterns
 */
void Path::UpdateCache()
{
	if (needtorecache == 0) return;

	cache_angle .Reset(true); //removes all points and leaves blank
	cache_offset.Reset(true);
	cache_width .Reset(true);
	outlinecache.flush();
	centercache .flush();

	cache_top   .flush();
	cache_bottom.flush();

	if (!path) return;
	//if (!Weighted()) { needtorecache=0; return; } // *** always create cache, as custom joins and caps must be dealt with


	// 1. strategy is to approximate the angle, and top and bottom offsets along the curve in an abstract
	//    space not tied to the windings of the base path.
	// 2. Next basically subdivide the base path a couple of times to generate sample points.
	// 3. from the sample points,  create a top, bottom, and center path, based on the cached curves for angle and offsets found in 1.
	// 4. Finally, connect the top and bottom paths as appropriate.


	Coordinate *p=path->firstPoint(1);
	if (!(p->flags&POINT_VERTEX)) { // is degenerate path: no vertices
		DBG cerr <<"Degenerate path (shouldn't happen!)"<<endl;
		return;
	}

	bool hasangle=Angled();
	bool hasoffset=HasOffset();

	Coordinate *p2, *start=p;
	flatpoint wtop,wbottom;
	flatpoint woffset,wwidth;
	NumStack<flatpoint> topp,bottomp;
	NumStack<flatpoint> offsetp,widthp;
	NumStack<flatpoint> centerp; //destined for centercache, this is a path with 0 offset

	bool isclosed;



	 //---------------------------------
	 //
	 //first find the number of vertex points in the line, which
	 //is the number of bezier segments in the line, which also
	 //is the x bounds of the width curve.
	UpdateWidthCache();

	int n=NumVertices(&isclosed); 
	if (!isclosed) n--; //path is open, so num_segments = num_vertices - 1
	if (n==0) return; //single point, open path == no path!


	 //--------------------------
	 //
	 //cache_angle, cache_offset, cache_width curves now built,
	 //now need to build the actual paths,
	 //topp, bottomp, and centerp get filled with sample points derived from those offset curves,
	 //applied to the original paths
	 //


	flatpoint c1,c2;
	p=start;
	flatpoint pp,po,vv,vt;
	flatpoint sht, shb;
	//int ignorefirst=(isclosed?1:0); //index to begin render to segment.. for open paths, must not ignore first
	int ignorefirst=0; //we need to render the 1st sample point in a segment


	int nsamples=cache_samples;
	NumStack<flatpoint> bez;
	NumStack<double> bezt;
	bezt.Allocate(2*nsamples);
	bezt.Delta(2*nsamples);



	//
	//now parse over this's path, adding sampled points segment by segment..
	//Note that there is surely a more intelligent way to do this...
	//

	int cp=0; //vertex counter
	bool isline;
	flatpoint vvv;
	double width;
	double EPSILON = 1e-10;

	do { //one loop per vertex point
		p2=p->next; //p points to a vertex
		if (!p2) break;
		if (p2->flags&POINT_VERTEX) {
			//double segdist = norm2(p2->p()-p->p());
			if (norm2(p2->p()-p->p()) < EPSILON) {
				 //skip zero-ish length segments
				p=p2;
				continue;
			}
		}

		 //figure out where to sample the current bezier segment. When there are weight nodes,
		 //things can get crazy, so sample a bunch of times in between nodes, not just evenly
		 //along the whole bezier segment.
		bezt.flush_n();
		bez.flush_n();
		double lastw=-1, t;
		double rrr;
		for (int c=0; c<=pathweights.n; c++) {
			if (c==pathweights.n) {
				t=cp+1;
			} else {
				if (pathweights.e[c]->t<=cp) continue;
				if (pathweights.e[c]->t>=cp+1) continue;
				t=pathweights.e[c]->t;
			}
	
			if (lastw<cp) lastw=cp;
			rrr=(t-lastw)/(nsamples-1);
			for (int cc=(bezt.n==0 ? 0 : 1); cc<nsamples; cc++) bezt.push(lastw+rrr*cc-cp);
			lastw=t;
		}

		bez.Allocate(bezt.n);
		bez.n=bezt.n;

		//p2 now points to first Coordinate after the first vertex
		//find next 2 control points and next vertex
		//
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
				if (!p2) break;
			} else { //otherwise, should be a vertex
				//p2=p2->next;
				c2=p2->p();
			}

			bez_points_at_samples(bez.e, p->p(), c1,c2, p2->p(), bezt.e,bezt.n, ignorefirst);
			isline=false;

		} else {
			 //we do not have control points, so is just a straight line segment
			vvv=(p2->p()-p->p());
			//vvv.normalize();
			for (int bb=0; bb<bezt.n; bb++) {
				bez.e[bb]=p->p() + bezt.e[bb]*vvv;
			}
			isline=true;

			c1=p->p();
			c2=p2->p();
		}


		 //compute sample points of top, bottom, and center, based on found bezier segment
		 //
		for (int bb=ignorefirst; bb<bez.n; bb++) {
			//DBG cerr <<"bez: "<<bez.e[bb].x<<"  "<<bez.e[bb].y<<endl;

			if (isline) vv=vvv;
			else {
				vv=bez_visual_tangent(bezt.e[bb], p->p(), c1,c2, p2->p());
			}
			vv.normalize();
			vt=transpose(vv);
			vt.normalize();
			po=bez.e[bb] + vt*cache_offset.f(cp+bezt.e[bb]);
			if (hasangle) {
				if (absoluteangle) vt=rotate(flatpoint(1,0), cache_angle.f(cp+bezt.e[bb]));
				else vt=rotate(vt, cache_angle.f(cp+bezt.e[bb]));
			}

			if (hasoffset) centerp.push(po);

			width = cache_width.f(cp+bezt.e[bb]);
			if (brush) {
				brush->MinMax(0, vt, shb, sht);
				topp   .push(po + (sht.x * vv + sht.y * vt)*width/2);
				bottomp.push(po + (shb.x * vv + shb.y * vt)*width/2);

			} else {
				topp   .push(po + vt*width/2);
				bottomp.push(po - vt*width/2);
			}

			if (cache_types & 1) {
				cache_top   .push(topp   .e[topp.n   -1]);
				cache_bottom.push(bottomp.e[bottomp.n-1]);
			}
		}

		if (!hasoffset) {
			 //center path is the same as the original path
			if (isline) {
				vv=(p2->p() - p->p())/3;
				if (!ignorefirst) {
					centerp.push(p->p());  centerp.e[centerp.n-1].info|=LINE_Vertex;
				}
				centerp.push(p->p()+vv);   centerp.e[centerp.n-1].info|=LINE_Bez;
				centerp.push(p->p()+2*vv); centerp.e[centerp.n-1].info|=LINE_Bez;
				centerp.push(p2->p());     centerp.e[centerp.n-1].info|=LINE_Vertex;
			} else { //add bez
				if (!ignorefirst) {
					centerp.push(p->p()); centerp.e[centerp.n-1].info|=LINE_Vertex;
				}
				centerp.push(c1);         centerp.e[centerp.n-1].info|=LINE_Bez;
				centerp.push(c2);         centerp.e[centerp.n-1].info|=LINE_Bez;
				centerp.push(p2->p());    centerp.e[centerp.n-1].info|=LINE_Vertex;
			}
		}

		if (ignorefirst==0) ignorefirst=1;

		 //need to check join with next segment, special handling for corners...
		if (p2->nextVertex(0)) {
			flatpoint curv, nextv;

			curv=p2->direction(0);
			nextv=p2->direction(1);

			if (areparallel(curv, nextv)!=-1) {
				centerp.e[centerp.n-1].info|=LINE_Join;
				topp   .e[topp.n-1]   .info|=LINE_Join;
				bottomp.e[bottomp.n-1].info|=LINE_Join;
				ignorefirst=0;

			} else if (p2==start) {
				 //is closed path and is continuous at the end
				centerp.pop();
				topp.pop();
				bottomp.pop();
			}
		}


		p=p2;
		cp++;
	} while (p && p->next && p!=start); //loop over original line

	int closed=(p==start);

	//DBG dump_points("centerp",centerp.e,centerp.n);

	// Now to build actual cache paths from topp, bottomp, and centerp sample points raw materials.
	//we take the topp and bottomp points, and create a path that incorporates both,
	//temporarily installing all points to topp...
	

	if (!closed) {
		 //top contour is connected to bottom with caps..
		topp.e[topp.n-1].info|=LINE_Cap;

		// *** add cap at 1st topp.n (original end of path)
		for (int c=bottomp.n-1; c>=0; c--) {
			topp.push(bottomp.e[c]);
			if (topp.e[topp.n-1].info&LINE_Join) {
				topp.e[topp.n-1].info&=~LINE_Join;
				if (topp.n>1) topp.e[topp.n-2].info|=LINE_Join;
			}
		}
		// *** add cap at new topp.n (original beginning of path)
		topp.e[topp.n-1].info|=LINE_Cap;
		topp.e[topp.n-1].info|=LINE_Closed;

	} else {
		 //path is closed...
		if (centerp.e[centerp.n-1]==centerp.e[0]) centerp.n--; //first point will maybe have been repeated
		centerp.e[centerp.n-1].info|=LINE_Closed;

		 //bottom contour is a seperate path than top contour
		if (topp.e[topp.n-1]==topp.e[0]) topp.n--; //first point will maybe have been repeated
		if (bottomp.e[bottomp.n-1]==bottomp.e[0]) bottomp.n--; //first point will maybe have been repeated

		topp.e[topp.n-1].info|=LINE_Closed;
		if (bottomp.e[bottomp.n-1].info&LINE_Join) bottomp.e[0].info|=LINE_Join;
		for (int c=bottomp.n-1; c>=0; c--) {
			topp.push(bottomp.e[c]);
			if (topp.e[topp.n-1].info&LINE_Join && c>0) {
				topp.e[topp.n-1].info&=~LINE_Join;
				if (topp.n>1) topp.e[topp.n-2].info|=LINE_Join;
			}
		}
		topp.e[topp.n-1].info|=LINE_Closed;
	}

	//DBG dump_points("topp:",topp.e,topp.n);

	//----------------
	//
	//Now raw paths for outline and centerline are constructed. need to bezier approximate
	//from the raw sample points, and add joins where necessary.
	//

	int thisclosed, thisstart, thisn;

	 //bezier approximate the sample points between join points
	for (int pth=0; pth<2; pth++) {
		NumStack<flatpoint> *list;
		NumStack<flatpoint> newlist;

		if (pth==0) list=&topp;
		else list=&centerp;


		DBG if (pth==0) cerr <<"------approximate for topp..."<<endl;
		DBG if (pth==1) cerr <<"------approximate for centerp..."<<endl;
		bez_reduce_approximate(list, maxx>minx ? (maxx-minx + maxy-miny)/10000 : 1e-7);
	}

	//DBG dump_points("topp after approximate:",topp.e,topp.n);

	 //do joins for center and outline paths
	int njoin;
	flatpoint join[8];
	flatpoint line[8];
	flatpoint samples[3];

	for (int pth=0; pth<2; pth++) {
	  NumStack<flatpoint> *list;
	  if (pth==0) list=&topp;
	  else list=&centerp;

	  thisclosed=0;
	  thisstart=0;
	  thisn=-1;

	  DBG if (pth==0) cerr <<"------joins for topp..."<<endl;
	  DBG if (pth==1) cerr <<"------joins for centerp..."<<endl;

	  for (int c=0; c<list->n; c++) {
		//DBG cerr <<list->e[c].x<<"   "<<list->e[c].y<<endl;
		if (thisn<0 || (thisn>0 && c>=thisstart+thisn)) {
			 //need to find length of current line, which might not be list->n, so as
			 //not to do buffer overruns below
			thisstart=c;
			for (int c2=thisstart; c2<list->n; c2++) {
				if      (list->e[c2].info&LINE_Closed) thisclosed= 1;
				else if (list->e[c2].info&LINE_Open)   thisclosed=-1;
				else if (c2==list->n-1) thisclosed=-1;
				else thisclosed=0;

				if (thisclosed!=0) {
					thisn=c2-thisstart+1;
					break;
				}
			}
		}

		int jointype = 0;
		if ((list->e[c].info&LINE_Cap)!=0) {
			if (linestyle) {
				if (c == list->n-1) {
					//start cap
					if      (linestyle->capstyle == LAXCAP_Butt)  jointype = LAXJOIN_Bevel;
					else if (linestyle->capstyle == LAXCAP_Round) jointype = LAXJOIN_Round;
					else jointype = LAXJOIN_Bevel;
				} else {
					//end cap
					int capstyle = linestyle->endcapstyle;
					if (capstyle == 0) capstyle = linestyle->capstyle;

					if      (capstyle == LAXCAP_Butt)  jointype = LAXJOIN_Bevel;
					else if (capstyle == LAXCAP_Round) jointype = LAXJOIN_Round;
					else jointype = LAXJOIN_Bevel;
				}
			}
			if (!jointype) jointype = LAXJOIN_Bevel;
		}
		if ((list->e[c].info&LINE_Join)!=0 && !jointype) {
			jointype = linestyle ? linestyle->joinstyle : LAXJOIN_Round;
		} 
		if (!jointype) continue; //nothing special to do

		//DBG cerr <<"maybe join, dist to next point="<<norm2(list->e[c]-list->e[c+1])<<endl; 

		// *** trouble spot with following line, should wrap around as appropriate
		if (c<thisstart+thisn-1 && norm2(list->e[c]-list->e[c+1])<1e-5) continue; //don't bother if points really close together

		DBG cerr << "**** JOIN at "<<c<<'/'<<list->n<<endl;


		if (list->e[thisstart + (c-thisstart+thisn-1)%thisn].info&LINE_Bez) {
			 //prev points were bez segment
			line[3]=list->e[c];
			line[2]=list->e[thisstart + (c-thisstart+thisn-1)%thisn];
			line[1]=list->e[thisstart + (c-thisstart+thisn-2)%thisn];
			line[0]=list->e[thisstart + (c-thisstart+thisn-3)%thisn];

		} else {
			 //prev points were straight segment
			samples[1]=list->e[c];
			samples[0]=list->e[thisstart + (c-thisstart+thisn-1)%thisn];
			flatpoint vv=(samples[1]-samples[0])/3;
			
			line[3]=samples[1];
			line[2]=samples[1]-vv;
			line[1]=samples[1]-2*vv;
			line[0]=samples[0];
		}

		//if (list->e[thisstart + (c-thisstart+1)%thisn].info&LINE_Bez) {
		if (list->e[thisstart + (c-thisstart+2)%thisn].info&LINE_Bez) {
			 //next points were bez segment
			line[4]=list->e[thisstart + (c-thisstart+1)%thisn];
			line[5]=list->e[thisstart + (c-thisstart+2)%thisn];
			line[6]=list->e[thisstart + (c-thisstart+3)%thisn];
			line[7]=list->e[thisstart + (c-thisstart+4)%thisn];

		} else {
			 //next points were straight segment
			samples[0]=list->e[thisstart + (c-thisstart+1)%thisn];
			samples[1]=list->e[thisstart + (c-thisstart+2)%thisn];
			flatpoint vv=(samples[1]-samples[0])/3;

			line[4]=samples[0];
			line[5]=samples[0]+vv;
			line[6]=samples[0]+2*vv;
			line[7]=samples[1];
		}

		njoin=0;
		join_paths(jointype,
				   linestyle ? linestyle->miterlimit : defaultwidth*200,
				   line[0],line[1],line[2],line[3],
				   line[4],line[5],line[6],line[7],
				   &njoin, join);

		if (njoin>0 && c==thisstart+thisn-1) list->e[c].info&=~(LINE_Closed|LINE_Open);
		for (int cc=0; cc<njoin; cc++) {
			list->push(join[cc],c+1);
			c++;
			thisn++;
		}

		if (c==thisstart+thisn-1) {
			if (thisclosed==1)       list->e[c].info|=LINE_Closed;
			else if (thisclosed==-1) list->e[c].info|=LINE_Open;
			thisn=-1;
		}
	  }
	}


	 //finally install topp to outlinecache
	flatpoint *aa=topp.extractArray(&n);
	outlinecache.insertArray(aa,n);


	 //install centerp to centercache
	aa=centerp.extractArray(&n);
	centercache.insertArray(aa,n);
	if (closed) centercache.e[centercache.n-1].info|=LINE_Closed;

	if (cache_types & 1) {
		 //we create a matched top and bottom point list, ignoring joins (for now)
		 //such as can be easily used as a basis for a mesh...
		flatpoint *ppp=new flatpoint[cache_top.n*3];
		bez_from_points(ppp, cache_top.e,cache_top.n);
		cache_top.insertArray(ppp,cache_top.n*3);

		ppp=new flatpoint[cache_bottom.n*3];
		bez_from_points(ppp, cache_bottom.e,cache_bottom.n);
		cache_bottom.insertArray(ppp,cache_bottom.n*3);
	}

	needtorecache = 0;
}


//! Sets DoubleBBox::minx,etc.
void Path::FindBBox()
{
	DoubleBBox::ClearBBox();
	if (!path) return;

	if (Weighted()) {
		UpdateCache();
		addtobounds(outlinecache.e, outlinecache.n);

	} else {
		FindBBoxBase(this);
		if (linestyle) {
			ExpandBounds(linestyle->width/2);
		}
	}
}

void Path::FindBBoxBase(DoubleBBox *ret)
{
	if (!ret) ret = this;
	ret->ClearBBox();
	if (!path) return;

	Coordinate *p=NULL,*t,*start;

	 // First find a vertex point
	start = t = path->firstPoint(1);
	if (!(t->flags & POINT_VERTEX)) return;//path contains only mysterious control points

	ret->addtobounds(t->p());

	 // step through all the rest of the vertices
	flatpoint c1,c2;
	while (t) {
		p=t->next;
		if (!p || p==start) break;

		if (p->flags&POINT_VERTEX) {
			 //simple case, just a line segment
			ret->addtobounds(p->p());
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

		bez_bbox(t->p(),c1,c2,p->p(), ret, NULL);

		t=p;
		if (t==start) break;
	}
}

void Path::FindBBoxWithWidth(DoubleBBox *ret)
{
	if (!ret) ret = this;
	ret->ClearBBox();
	if (!path) return;

	if (!Weighted()) {
		FindBBoxBase(ret);
		if (linestyle) {
			ret->ExpandBounds(linestyle->width);
		}
	} else {
		UpdateCache();
		ret->addtobounds(outlinecache.e, outlinecache.n);
	}
}


/*! Compute axis aligned bounding box of content transformed by transform.
 * Note this is more specific than the normal FindBBox().
 */
void Path::ComputeAABB(const double *transform, DoubleBBox &box)
{
	if (!path) return;

	Coordinate *p=NULL,*t,*start;

	if (Weighted()) {
		UpdateCache();
		for (int c=0; c<outlinecache.n; c++) {
			box.addtobounds(transform_point(transform, outlinecache.e[c]));
		}

	} else {

		 // First find a vertex point
		start = t = path->firstPoint(1);

		if (!(t->flags&POINT_VERTEX)) return;//only mysterious control points

		box.addtobounds(transform_point(transform, t->p()));

		 // step through all the rest of the vertices
		flatpoint c1,c2;
		while (t) {
			p=t->next;
			if (!p || p==start) break;

			if (p->flags&POINT_VERTEX) {
				 //simple case, just a line segment
				box.addtobounds(transform_point(transform, p->p()));
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

			bez_bbox(t->p(),c1,c2,p->p(), this, transform);

			t=p;
			if (t==start) break;
		}
	}
}

/*! Always true if absoluteangle is true.
 * If not absoluteangle, then true if any weight node angle is nonzero.
 */
bool Path::Angled()
{
	if (pathweights.n==0) return false;
	if (absoluteangle==true) return true;
	for (int c=0; c<pathweights.n; c++) {
		if (pathweights.e[c]->angle!=0) return true;
	}
	return false;
}

/*! true if any weight node has nonzero offset.
 */
bool Path::HasOffset()
{
	for (int c=0; c<pathweights.n; c++) {
		if (pathweights.e[c]->offset!=0) return true;
	}
	return false;
}

/*! True if there are no weight nodes, or the width is the same in all nodes.
 */
bool Path::ConstantWidth()
{
	if (pathweights.n==0) return true;

	for (int c=1; c<pathweights.n; c++) {
		if (pathweights.e[c]->width!=pathweights.e[0]->width) return false;
	}
	return true;
}

/*! Return true for path has variable width, any offset, or any non-zero angle,
 * or return false for constant width, no offset, no angles (and no absolute angles).
 *
 * A true return value indicates that the UpdateCache mechanism must be used to
 * compute the outlines of the path, as it might differ substantially from the base path
 * with a traditional thickness.
 *
 * Using LAXCAP_Zero_Width, or the extrapolate join in linestyle will return a true.
 */
bool Path::Weighted()
{
	if (pathweights.n==0) return false;
	if (linestyle && (linestyle->capstyle == LAXCAP_Zero_Width || linestyle->endcapstyle == LAXCAP_Zero_Width)) return true;
	if (linestyle && linestyle->joinstyle == LAXJOIN_Extrapolate) return true;

	if (pathweights.e[0]->offset!=0) return true;
	if (pathweights.e[0]->angle!=0)  return true;
	if (pathweights.n==1)            return false; //so 1 weight value, no offset

	double w=pathweights.e[0]->width;
	for (int c=1; c<pathweights.n; c++) {
		if (pathweights.e[c]->angle!=0)  return true;
		if (pathweights.e[c]->offset!=0) return true;
		if (pathweights.e[c]->width!=w)  return true;
	}
	return false;
}

/*! Make the base line of the path be the current centerline, updating all weight
 * nodes to the new t distribution.
 *
 * If !HasOffset(), nothing is done.
 *
 * Return 0 for success, 1 for error.
 */
int Path::ApplyOffset()
{
	if (!HasOffset()) return 0;

	if (needtorecache) UpdateCache();

	NumStack<flatpoint> weights;
	flatpoint p,v;
	for (int c=0; c<pathweights.n; c++) {
		PointAlongPath(pathweights.e[c]->t,0, &p,&v);
		v=transpose(v);
		v.normalize();
		weights.push(p+v*cache_offset.f(pathweights.e[c]->t));
	}

	delete path;
	path=FlatpointToCoordinate(centercache.e,centercache.n);
	path=path->firstPoint(1);

	for (int c=0; c<pathweights.n; c++) {
		ClosestPoint(weights.e[c], NULL,NULL, &pathweights.e[c]->t);
		pathweights.e[c]->offset=0;
	}

	needtorecache=1;
	return 1;
}

/*! Replace path with the upper edge of the stroke, and default width.
 */
int Path::ApplyUpperStroke()
{
	if (needtorecache) UpdateCache();
	Coordinate *coord = FlatpointToCoordinate(cache_top.e, cache_top.n);
	if (!coord) return 0;

	ClearWeights();
	bool closed = IsClosed();
	delete path;
	path = coord;
	if (closed) close();

	needtorecache=1;
	return 1;
}

/*! Replace path with the lower edge of the stroke, and default width.
 */
int Path::ApplyLowerStroke()
{
	if (needtorecache) UpdateCache();
	Coordinate *coord = FlatpointToCoordinate(cache_bottom.e, cache_bottom.n);
	if (!coord) return 0;

	ClearWeights();
	bool closed = IsClosed();
	delete path;
	path = coord;
	if (closed) close();

	needtorecache=1;
	return 1;
}

/*! Make all weight nodes have towhat as offset.
 * If towhat==0 and there are no nodes, then add one with given offset.
 *
 * If diff, then add to current offset for all nodes. In this case, if no nodes,
 * then set to this value.
 *
 * Return 0 for success, 1 for error.
 */
int Path::SetOffset(double towhat, bool diff)
{
	if (pathweights.n==0) {
		AddWeightNode(.5, towhat, linestyle ? linestyle->width : defaultwidth, 0);

	} else {
		for (int c=0; c<pathweights.n; c++) {
			if (diff) pathweights.e[c]->offset+=towhat;
			else pathweights.e[c]->offset=towhat;
		}
	}

	needtorecache=1;
	return 0;
}

/*! Make all weight nodes have towhat as the angle.
 * If towhat==0 and there are no nodes, then do nothing.
 *
 * Return 0 for success, 1 for error.
 */
int Path::SetAngle(double towhat, int absolute)
{
	if (absolute) absoluteangle=true; else absoluteangle=false;

	if (pathweights.n!=0) {
		for (int c=0; c<pathweights.n; c++) {
			pathweights.e[c]->angle=towhat;
		}
	}

	needtorecache=1;
	return 0;
}

/*! from MUST be previous to to.
 * Traverse from to to, making any segment between any vertices be straight lines,
 * and change vertex point type to corner.
 * If asbez, then insert bezier control handles on thirds of segment length.
 * If !asbez, then have no control points between vertices.
 *
 * If from==NULL, then use path. If to=NULL then use rest of path since from.
 *
 * Return number of vertices traversed.
 */
int Path::MakeStraight(Coordinate *from, Coordinate *to, bool asbez)
{
	if (from==NULL) from=path; else from=from->nextVertex(1);
	if (to==NULL) {
		to=from->lastPoint(1);
	} else to=to->previousVertex(1);

	int numv=0;
	Coordinate *p=from, *p2;
	do {
		p2=p->nextVertex(0);
		if (!p2) break;
		numv++;

		 //remove any existing control points..
		while (p->next != p2) {
			Coordinate *tmp=p->next;
			p->next->prev=NULL;
			p->next=p->next->next;
			tmp->next=NULL;
			p->next->prev=p;

			delete tmp;
		}

		 //add line control point if necessary
		if (asbez) {
			flatpoint v=(p2->p() - p->p())/3;
			p ->insert(new Coordinate(p->p()+v,   POINT_TOPREV, NULL), 1);
			p2->insert(new Coordinate(p->p()+2*v, POINT_TONEXT, NULL), 0);
		}

		p->flags&=~(POINT_SMOOTH|POINT_REALLYSMOOTH);
		p2->flags&=~(POINT_SMOOTH|POINT_REALLYSMOOTH);
		p=p2;
	} while (p && p!=to);


	needtorecache=1;
	return numv;
}

/*! Shift the position of a weight node, ensuring that the list stays sorted.
 *
 * Returns the new index of the node.
 */
int Path::MoveWeight(int which, double nt)
{
	if (which<0 || which>=pathweights.n) return -1;

	if (nt<0) {
		DBG cerr <<" *** WARNING! NEGATIVE t in Path::MoveWeight!!"<<endl;
		nt=0;
	}

	PathWeightNode *w=pathweights.e[which];
	w->t=nt;
	int npos=which;
	while (npos>0 && nt<pathweights.e[npos-1]->t) npos--;

	if (npos==which) {
		while (npos<pathweights.n-1 && nt>pathweights.e[npos+1]->t) npos++;
	}
	if (npos!=which) {
		//if (npos>which) npos--;
		pathweights.pop(which);
		pathweights.push(w,1,npos);
	}

	//UpdateWeightCache(w);
	return npos;
}

/*! which is an index into weights stack.
 * Return 0 for success, or 1 for which out of bounds.
 */
int Path::RemoveWeightNode(int which)
{
	if (which<0 || which>=pathweights.n) return 1;
	double width = pathweights.e[which]->width;
	pathweights.remove(which);
	if (pathweights.n==0) defaultwidth = width;
	needtorecache=1;
	return 0;
}

/*! Add a weight node at nt, and grab the current offset, width, and angle at that t value.
 */
void Path::InsertWeightNode(double nt)
{
	double no,nw,nangle;
	GetWeight(nt, &nw, &no, &nangle);
	AddWeightNode(nt,no,nw,nangle);
}

/*! Add a weight node at nt with the given offset, width and angle.
 */
void Path::AddWeightNode(double nt, double noffset, double nwidth, double nangle)
{
	//insert sorted...
	int c2;
	for (c2=0; c2<pathweights.n; c2++) {
		if (nt > pathweights.e[c2]->t) continue;
		if (nt < pathweights.e[c2]->t) break;

		//overwrite!
		pathweights.e[c2]->t      = nt;
		pathweights.e[c2]->offset = noffset;
		pathweights.e[c2]->width  = nwidth;
		pathweights.e[c2]->angle  = nangle;
		c2=-1;
		break;
	}

	if (c2>=0) { //wasn't found, so need to insert fresh one
		PathWeightNode *w = new PathWeightNode(nt, noffset,nwidth,nangle, PathWeightNode::Default);
		pathweights.push(w,1,c2);
	}

	//UpdateWeightCache(w);
	needtorecache=1;
}

/*! Retrieve line characteristics at any point along the path.
 *
 * Return 0 for success.
 */
int Path::GetWeight(double t, double *width, double *offset, double *angle)
{
	if (needtorecache) UpdateCache();

	if (pathweights.n==0) {
		if (width)  *width=defaultwidth;
		if (offset) *offset=0;
		if (angle)  *angle=0;

	} else {
		if (width || offset) {
			if (width)  *width  = cache_width .f(t);
			if (offset) *offset = cache_offset.f(t);
		}
		if (angle) *angle=cache_angle.f(t);
	}

	return 0;
}


//! Append bezier points taken from the string.
/*! The string must be something like "1 2 p 3 4 n 5 6 7 8". Any pair of numbers
 * not prefaced by a 'n' or 'p' is a vertex point. A 'n' indicates it is a
 * control point for the following point (which must be a vertex). A 'p' is for
 * control points for a previous vertex point. Any violation of these terms stops
 * the appending of points.
 *
 * On path vertices may be tagged with 'v', and an optional point type indicator,
 * one of 'c' (corner, not equal), 's' (smooth, not equal),
 * 'S' (smooth, equal), or 'e' (corner, equal).
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

/*! Assuming p is in the path somewhere, remove it and associated control points.
 * Makes sure that path heads and weight nodes maintain their integrity.
 *
 * If the point is part of a controlled segment, then delete the whole segment.
 * This is determined by any adjacent point that has the same Coordinate::controls.
 *
 * If deletetoo, then "delete p" and other control points connected to it. Otherwise,
 * p and its points will merely be detached. 
 *
 * If this->path==NULL, return 1, and do nothing to p.
 * If p==NULL, return 2, do nothing else.
 * If p is not contained in this->path, return -1, and do nothing to p.
 * If p is in this->path, detach (or delete), and return 0.
 */
int Path::removePoint(Coordinate *p, bool deletetoo)
{
	if (!path) return 1;
	if (!p) return 2;

	// a pathop might handle deleting differently, for instance, when one deletes a control point for
	// a bezier curve, the default is for the point to not be deleted, but to zap back to its vertex.
	// Please be advised that if the pathop actually does delete points from the path, it will have
	// to do all the things the rest of this procedure does. Normal use of this option is for the
	// the pathop to not actually delete anything, just rearrange things, like deleting a focus in an
	// ellipse, but if this function is called, the whole controlled segment is deleted.

	int index=-1; //index of point in path, whole numbers are vertices
	if (path->hasCoord(p, &index)==0) return -1; //not in path!


	Coordinate *s,*e;
	s=e=p;

	if (p->controls) {
		//find range of points we need to delete, which is any range of points sharing the same SegmentControl
		while (s->prev && s->prev->controls==p->controls && s->prev!=p) s=s->prev;
		while (e->next && e->next->controls==p->controls && e->next!=p) e=e->next;

	} else if (p->flags&POINT_VERTEX) { //is vertex, remove any associated uncontrolled bezier control handles
		if (p->prev && !p->prev->controls && p->prev->flags&POINT_TONEXT) s=p->prev;
		if (p->next && !p->next->controls && p->next->flags&POINT_TOPREV) e=p->next;
	} // else assume is just a normal bezier control point, remove only that one

	bool isclosed;
	int oldpathlen = NumVertices(&isclosed);
	if (!isclosed) oldpathlen--;

	int s_is_endpoint = s->isEndpoint();
	int e_is_endpoint = e->isEndpoint();
	if (s_is_endpoint>0) s_is_endpoint=-1;
	if (e_is_endpoint<0) e_is_endpoint=1;

	p = s->detachThrough(e); //now s through e is an open path, p points to something remaining
	int v = 0;
	for (Coordinate *pp=s; pp!=NULL; pp=pp->next) {
		if (pp->flags&POINT_VERTEX) v++;
	}

	if (v != 0) {
		//there were vertices within the chain that is deleted, so we
		//have to ensure that this->path points to something correct, and make
		//sure that weight nodes remain positioned in a reasonable manner


		//first deal with this->path
		if (!p) {
			path = NULL; //deleting removed all points from path, so delete whole path
		} else {
			//there is still a remnant of a path, need to make sure this->path is valid
			path = p->firstPoint(1);
		}

		//next deal with weight nodes
		if (s_is_endpoint) {
			 //we might have to insert an extra node to compensate for truncation
			double weight_at_end=0, offset_at_end=0, angle_at_end=0;
			bool need_to_insert = false;
			if (pathweights.n > 0) {
				GetWeight(1, &weight_at_end, &offset_at_end, &angle_at_end);
			}

			//remove any points at t<1, dec the rest
			for (int c=pathweights.n-1; c>=0; c--) {
				if (pathweights.e[c]->t<1) {
					need_to_insert = true;
					RemoveWeightNode(c);
				} else pathweights.e[c]->t--;
			}

			if (need_to_insert) { //adding node to new truncated end
				if (pathweights.e[0]->t != 0) AddWeightNode(0, offset_at_end, weight_at_end, angle_at_end);
			}

		} else if (e_is_endpoint) {
			 //we might have to insert an extra node to compensate for truncation
			double weight_at_end=0, offset_at_end=0, angle_at_end=0;
			bool need_to_insert = false;
			if (pathweights.n > 0) {
				GetWeight(oldpathlen-1, &weight_at_end, &offset_at_end, &angle_at_end);
			}

			 //remove any points at t > oldpathlen-1
			for (int c=pathweights.n-1; c>=0; c--) {
				if (pathweights.e[c]->t > oldpathlen-1) {
					RemoveWeightNode(c);
					need_to_insert = true;
				}
			}

			if (need_to_insert) { //adding node to new truncated end
				if (pathweights.e[pathweights.n-1]->t < oldpathlen-1)
					AddWeightNode(oldpathlen-1, offset_at_end, weight_at_end, angle_at_end);
			}

		} else {
			//deleted point was an interior vertex, not an endpoint.
			//need to redistribute nodes in segments adjacent to t=index
			for (int c=0; c<pathweights.n; c++) {
				if (pathweights.e[c]->t > index-1 && pathweights.e[c]->t <= index) 
					pathweights.e[c]->t = (pathweights.e[c]->t+index-1)/2;

				else if (pathweights.e[c]->t > index && pathweights.e[c]->t < index+1) 
					pathweights.e[c]->t = (pathweights.e[c]->t+index-1)/2;

				else if (pathweights.e[c]->t >= index+1) pathweights.e[c]->t--;
			} 
		} 
	} //if verts deleted

	if (deletetoo) delete s;

	needtorecache=1;
	return 0;
}

void Path::ClearWeights()
{
	pathweights.flush();
	needtorecache=1;
}

/*! Use this when for some reason your weights have been scrambled and are not
 * sorted by t. This is most common after opening a path at random points. When
 * that happens, high t values get converted to low t values.
 */
void Path::SortWeights()
{
	int lowest;
	for (int c=0; c<pathweights.n; c++) {
		lowest=c;
		for (int c2=c+1; c2<pathweights.n; c2++) {
			if (pathweights.e[c2]->t<pathweights.e[c]->t) lowest=c2;
		}
		if (lowest!=c) pathweights.swap(c,lowest);
	}
}

//! Return the last point (vertex or control) that occurs at the end, or just before path.
/*! If v!=0, then return the last vertex, rather than the last of any kind of point.
 */
Coordinate *Path::lastPoint(int v)
{
	if (!path) return NULL;
	return path->lastPoint(v);
}

/*! Replace the path with a rounded rectangle.
 *  Returns number of vertices in the resulting path.
 * 
 * If numsizes is 1, then sizes[0].x is horizontal radius, and .y is vertical radius for all points.
 * If numsizes is 4, then sizes is for lower left, lower right, upper right, upper left.
 * If numsizes is 2 or 3, then it behaves as if it was 1.
 * If numsizes is > 4, only the first 4 values are used.
 *
 * The path points are updated with proper points and flags for a total of between 12 and 24 points, depending
 * on if the radius at a point is 0 or not. Any extra points already present are removed.
 *
 */
int Path::MakeRoundedRect(double x, double y, double w, double h, flatpoint *sizes, int numsizes)
{
	if (!path) path = new Coordinate();

	Coordinate *p = path->firstPoint(0);
	path = p;
	if (p->prev) p->disconnect(false);
	
	int i[4];
	i[0] = i[1] = i[2] = i[3] = 0;
	if (numsizes >= 4) {
		i[1]=1;
		i[2]=2;
		i[3]=3;
	}

	flatpoint op[4];
	op[0].set(x,y);
	op[1].set(x+w,y);
	op[2].set(x+w,y+h);
	op[3].set(x,y+h);

	double ins[8]; //in out in out in out ...
	for (int c=0; c<8; c++) {
		if (c%2 == 0) ins[c] = sizes[i[c/2]].y;
		else ins[c] = sizes[i[c/2]].x;
	}

	double unit_r = bez_arc_handle_length(1, M_PI/2);

	flatpoint vprev, vnext, inp, outp, v;
	Coordinate *last = nullptr;
	// double dprev, dnext;
	int numv = 0;
	bool skipv;

	for (int c=0; c<4; c++) {
		vprev = op[(c+3)%4] - op[c];
		vnext = op[(c+1)%4] - op[c];
		// dprev = vprev.norm();
		// dnext = vnext.norm();
		vprev.normalize();
		vnext.normalize();
		inp  = op[c] + vprev * ins[2*c];
		outp = op[c] + vnext * ins[2*c+1];
		skipv = false;

		if (c>0) { //connect to prev vertex
			v = (last->fp - inp)/3;
			if (v.norm() > 1e-6) {
				p->fp = inp + 2*v;
				p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TOPREV;
				if (!p->next) p->connect(new Coordinate(), true);
				p = p->next;

				p->fp = inp + v;
				p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TONEXT;
				if (!p->next) p->connect(new Coordinate(), true);
				p = p->next;

			} else skipv = true;
		}

		if (!skipv) {
			p->p(inp);
			p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_VERTEX;
			last = p;
			numv++;
			if (!p->next) p->connect(new Coordinate(), true);
			p = p->next;
		}

		if (ins[c*2] != 0 || ins[c*2+1] != 0) {
			//curved corner, v-c-c-v
			v = op[c] - inp;
			p->fp = inp + unit_r * v;
			p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TOPREV;
			if (!p->next) p->connect(new Coordinate(), true);
			p = p->next;
			
			v = op[c] - outp;
			p->fp = outp + unit_r * v;
			p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TONEXT;
			if (!p->next) p->connect(new Coordinate(), true);
			p = p->next;

			p->fp = outp;
			p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_VERTEX;
			last = p;
			numv++;
			if (!p->next) p->connect(new Coordinate(), true);
			p = p->next;

		} // else  simple corner, 1 vertex only, already done
	}

	// connect final segment
	v = (path->fp - last->fp)/3;
	p->fp = last->fp + v;
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TOPREV;
	if (!p->next) p->connect(new Coordinate(), true);
	p = p->next;

	p->fp = last->fp + 2*v;
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TONEXT;

	if (p->next) {
		//delete any extraneous points
		last = p->next;
		last->disconnect(false);
		delete last;
	}

	p->next = path;
	path->prev = p;

	//remove extraneous path weights
	for (int c=pathweights.n-1; c >= 0; c--) {
		if (pathweights.e[c]->t >= numv) {
			pathweights.remove(c);
		}
	}

	needtorecache = 1;
	return numv;
}

/*! Replace the path with a sort of squircle.
 *  Returns number of vertices in the resulting path.
 *
 * sizes is a list of handle lengths, as a proportion of the dimension,
 * in order [top-left, top-right, right-up,right-down, bottom-right, bottom-left, left-down, left-up].
 *
 * For instance, if top-left is 1, then the handle length is w/2.
 *
 * The resulting path will always be 12 points, with vertices at middle top, middle right, middle bottom, and middle left.
 */
int Path::MakeSquircleCubic(double x, double y, double w, double h, double *sizes, int numsizes)
{
	if (!path) path = new Coordinate();

	Coordinate *p = path->firstPoint(0);
	path = p;
	if (p->prev) p->disconnect(false);
	
	flatpoint vprev, vnext, inp, outp, v;
	Coordinate *last = nullptr;
	// double dprev, dnext;
	int numv = 4;
	
	int i = 0;

	//top-left
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TONEXT;
	p->fp = flatpoint(x+w/2 - w/2*sizes[i%numsizes], y);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//top
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_VERTEX;
	p->fp = flatpoint(x+w/2, y);
	if (!p->next) p->next = new Coordinate();
	p = p->next;

	//top-right
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TOPREV;
	p->fp = flatpoint(x+w/2 + w/2*sizes[i%numsizes], y);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//right-up
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TONEXT;
	p->fp = flatpoint(x+w, y+h/2 - h/2*sizes[i%numsizes]);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//right
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_VERTEX;
	p->fp = flatpoint(x+w, y+h/2);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//right-down
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TOPREV;
	p->fp = flatpoint(x+w, y+h/2 + h/2*sizes[i%numsizes]);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//bottom-right
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TONEXT;
	p->fp = flatpoint(x+w/2 + w/2*sizes[i%numsizes], y+h);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//bottom
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_VERTEX;
	p->fp = flatpoint(x+w/2, y+h);
	if (!p->next) p->next = new Coordinate();
	p = p->next;

	//bottom-left
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TOPREV;
	p->fp = flatpoint(x+w/2 - w/2*sizes[i%numsizes], y+h);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//left-down
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TONEXT;
	p->fp = flatpoint(x, y+h/2 + h/2*sizes[i%numsizes]);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//left
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_VERTEX;
	p->fp = flatpoint(x, y+h/2);
	if (!p->next) p->next = new Coordinate();
	i++;
	p = p->next;

	//left-up
	p->flags = (p->flags & ~(POINT_TONEXT|POINT_VERTEX|POINT_TOPREV)) | POINT_TOPREV;
	p->fp = flatpoint(x, y+h/2 - h/2*sizes[i%numsizes]);
	
	if (p->next) {
		//delete any extraneous points
		last = p->next;
		last->disconnect(false);
		delete last;
	}

	p->next = path;
	path->prev = p;
	path = p->next;

	//remove extraneous path weights
	for (int c=pathweights.n-1; c >= 0; c--) {
		if (pathweights.e[c]->t > numv) {
			pathweights.remove(c);
		}
	}

	needtorecache = 1;
	return numv;
}



/*! If merge_ends >= 0, then collapse endpoints when distance is <= merge_ends.
 * If absorb_path, then deletes p and transfers all Coordinate objects to ourself.
 * at is which vertex to insert path at, or < 1 for insert at end. So if at==0, then
 * basically prepend p to ourself.
 */
void Path::AppendPath(Path *p, bool absorb_path, double merge_ends, int at)
{
	DBG cerr << " *** IMPLEMENT Path::AppendPath()"<<endl;
}

/*! Control points (info & LINE_BEZ) in pts must occur in pairs, so vertex-bez-bez-vertex-vertex-...
 */
void Path::append(flatpoint *pts, int n)
{
	Coordinate *coord = FlatpointToCoordinate(pts,n);
	append(coord);
}

/*! Note: adds at end, does not rejigger weight nodes.
 */
void Path::append(double x,double y,unsigned long flags,SegmentControls *ctl)
{
	if (path==NULL) {
		path=new Coordinate(x,y,flags,ctl);
		return;
	}
	Coordinate *p=path->previousVertex(0);
	if (p!=NULL) {
		 //path is closed, and p currently points to the next most vertex
		while (p->flags&POINT_TOPREV) p=p->next;
		p->append(x,y,flags,ctl);
	} else {
		p=path->lastPoint(0);
		p->append(x,y,flags,ctl);
	}
}

/*! ctl count will be incremented.
 */
void Path::append(flatpoint p,unsigned long flags,SegmentControls *ctl)
{
	append(p.x,p.y,flags,ctl);
}

//! The path takes possession of coord. Calling code should not delete it.
/*! coord can be a string of points or a single point, open or closed. 
 *
 * As this always puts at the end, no special reconfiguring of weight nodes is done.
 */
void Path::append(Coordinate *coord)
{
	if (!path) path=coord;
	else {
		Coordinate *p=path->lastPoint(0);
		p->insert(coord,1);
	}
}

/*! For a single path, this is the same as lineTo().
 * In a PathsData, this starts a new subpath.
 */
void Path::moveTo(flatpoint p)
{ lineTo(p); }

/*! same as append(p,POINT_VERTEX,NULL).
 */
void Path::lineTo(flatpoint p)
{ append(p,POINT_VERTEX,NULL); }

/*! If no existing points, first do a lineTo(c1).
 * Else, is a shortcut for:
 *
 * append(c1,POINT_TOPREV,NULL)\n
 * append(c2,POINT_TONEXT,NULL)\n
 * append(p2,POINT_VERTEX,NULL)\n
 */
void Path::curveTo(flatpoint c1, flatpoint c2, flatpoint p2)
{
	if (!path) append(c1,POINT_VERTEX,NULL);
	append(c1,POINT_TOPREV,NULL);
	append(c2,POINT_TONEXT,NULL);
	append(p2,POINT_VERTEX,NULL);
}

//! Make sure the path is closed.
/*! Return 1 for path made to be closed. 0 for already closed, or no path.
 */
int Path::close()
{
	if (!path) return 0;
	if (path->close()) {
		needtorecache=1;
		return 1;
	}
	//path points to the firstPoint, so no special checking for weight node placement necessary

	return 0;

}

/*! For each point, lerp. Only goes point for point. Does not add points for smoother morph.
 * Return 0 for success, nonzero error.
 */
int Path::LerpSimple(Path *a, Path *b, double t)
{
	if (!a || !a->path || !b ||!b->path) return 1;

	Coordinate *aa, *bb, *astart, *bstart, *p, *pstart, *pprev = nullptr;
	p = pstart = path;
	aa = astart = a->path;
	bb = bstart = b->path;

	do {
		if (!p) {
			p = new Coordinate();
			if (pprev) pprev->insert(p, 1);
		} else if (p == pstart) {
			p = new Coordinate();
			pstart->insert(p, 0);
		}
		p->p(t*aa->p() + (1-t)*bb->p());

		aa = aa->next;
		bb = bb->next;
		pprev = p;
		p = p->next;
	} while(aa && aa != astart && bb && bb != bstart);

	bool closed = IsClosed();
	if (aa == astart || bb == bstart) { //make sure path is closed
		if (!closed) close();

		while (p != pstart) {
			Coordinate *pp = p->next;
			p->detach();
			delete p;
			p = pp;
		}

	} else { //make sure path is open
		if (closed) {
			Coordinate *pp = path;
			if (pp->prev->flags & POINT_TONEXT) pp = pp->prev;
			pp->disconnect(false);
		}
		if (p && p != pstart) {
			p->prev->disconnect();
			delete p;
		}
	}

	return 0;
}

void Path::Transform(const double *mm)
{
	if (!path) return;

	Coordinate *p,*start;
	start = p = path;
	do {
		p->p(transform_point(mm,p->p()));
		p = p->next;
	} while (p && p!=start);
}


/*! If segment_loops, then check for bezier loops within each segment.
 * If self_path_only, then only check for path intersections with themselves.
 * If pathi == -1, then check all paths. If pathi == -1 and !self_path_only, then
 * also check for intersections between all paths.
 *
 * Return value is number of vertices added.
 */
int PathsData::AddAtIntersections(bool segment_loops, bool self_path_only, int pathi)
{
	flatpoint pts1[4];
	flatpoint pts2[4];
	flatpoint pret[10];
	double foundt1[9], foundt2[9];
	Coordinate *start1, *p1, *p1next, *c1, *c2;
	Coordinate *start2, *p2, *p2next;
	int isline;
	double threshhold = 1e-5;
	double maxdepth = 0;
	int num_slices = 0;

	if (segment_loops) {
		// check for self loops
		
		// check for loops in each segment
		for (int c3 = 0; c3 < paths.n; c3++) { //foreach path in pathsdata1
			if (pathi != -1 && pathi != c3) continue;
			if (!paths.e[c3]->path) continue;

			start1 = p1 = paths.e[c3]->path;

			pts1[0] = p1->p();
			if (p1->getNext(pts1[1], pts1[2], p1next, isline) != 0) break;
			pts1[3] = p1next->p();

			do { //foreach segment in path1
				flatpoint p;
				double tt[2];
				if (bez_self_intersection(pts1[0], pts1[1], pts1[2], pts1[3], &p, &tt[0], &tt[1])) {
					int n = 2;
					if (tt[0] < 1e-6) { //very close to 0.0, ignore
						tt[0] = tt[1];
						n = 1;
					}
					if (fabs(tt[1] - 1) < 1e-6) { //very close to 1.0, ignore
						n--;
					}
					if (n > 0) {
						bez_subdivide(tt, n, pts1[0], pts1[1], pts1[2], pts1[3], pret);
						if (p1->next->flags & POINT_TOPREV) {
							c1 = p1->next;
							c1->p(pret[1]);
							p1 = c1;
						} else { //no toprev, need to add
							p1->insert(new Coordinate(pts1[1], POINT_TOPREV, nullptr), true);
							p1 = p1->next;
						}

						if (p1->next->flags & POINT_TONEXT) {
							c2 = p1->next;
							c2->p(pret[2]);
							p1 = c2;
						} else { //no tonext, need to add
							p1->insert(new Coordinate(pts1[2], POINT_TONEXT, nullptr), true);
							p1 = p1->next;
						}

						p1->insert(new Coordinate(pts1[3], POINT_VERTEX, nullptr), true);
						p1 = p1->next;
						p1->insert(new Coordinate(pts1[4], POINT_TOPREV, nullptr), true);
						p1 = p1->next;
						p1->insert(new Coordinate(pts1[5], POINT_TONEXT, nullptr), true);
						p1 = p1->next;

						if (n > 1) {
							p1->insert(new Coordinate(pts1[6], POINT_VERTEX, nullptr), true);
							p1 = p1->next;
							p1->insert(new Coordinate(pts1[7], POINT_TOPREV, nullptr), true);
							p1 = p1->next;
							p1->insert(new Coordinate(pts1[8], POINT_TONEXT, nullptr), true);
							p1 = p1->next;
						}

						num_slices += n;
					}
				}

				p1 = p1next;
				pts1[0] = p1->p();
				if (p1->getNext(pts1[1], pts1[2], p1next, isline) != 0) break;
				pts1[3] = p1next->p();
			} while (p1 && p1 != start1);

		}
	}

	NumStack<double> tvals;

	// now check for intersections between whole segments
	for (int c3 = 0; c3 < paths.n; c3++) {
		if (!paths.e[c3]->path) continue;
		if (pathi != -1 && pathi != c3) continue;

		start1 = p1 = paths.e[c3]->path;
		double t_p1 = 0;
		double t_p2 = 0;

		do { //foreach segment in path1
			pts1[0] = p1->p();
			if (p1->getNext(pts1[1], pts1[2], p1next, isline) != 0) break;
			pts1[3] = p1next->p();

			for (int c4 = c3; c4 < paths.n; c4++) { //foreach path in pathsdata2
				if (!paths.e[c4]->path) continue;
				if (pathi != -1 && pathi != c4) continue;

				start2 = p2 = paths.e[c4]->path;
				t_p2 = 0;

				do { //foreach segment in path2
					pts2[0] = p2->p();
					if (p2->getNext(pts2[1], pts2[2], p2next, isline) != 0) break;
					pts2[3] = p2next->p();

					if (p2 == p1) continue; // don't intersect identical segments

					//pts2[0] = m2to1.transformPoint(pts2[0]);
					//pts2[1] = m2to1.transformPoint(pts2[1]);
					//pts2[2] = m2to1.transformPoint(pts2[2]);
					//pts2[3] = m2to1.transformPoint(pts2[3]);

					int num = 0;
					//DBG cerr <<"---at bez t1: "<<t_p1<<" t2: "<<t_p2<<endl;
					bez_intersect_bez(
							pts1[0], pts1[1], pts1[2], pts1[3],
							pts2[0], pts2[1], pts2[2], pts2[3],
							pret, foundt1, foundt2, num,
							threshhold,
							0,0,1,
							1, maxdepth
						);

					for (int c5 = 0; c5 < num; c5++) {
						DBG cerr << "bez_intersect: t1: "<<(t_p1 + foundt1[c5])<<" t2: "<<(t_p2 + foundt2[c5])<<endl;

						tvals.push(t_p1 + foundt1[c5]);
						//p1->SliceSegment(foundt1, num);
					}
					DBG if (num == 0) cerr <<"bez_intersect: No intersections"<<endl;

					p2 = p2next;
					t_p2 += 1;
				} while (p2 && p2 != start2);
			}

			p1 = p1next;
			t_p1 += 1;
		} while (p1 && p1 != start1);

		if (tvals.n > 0) {
			paths.e[c3]->AddAt(tvals.e, tvals.n, nullptr);
			num_slices += tvals.n;
			tvals.flush();
		}
	}

	return num_slices;
}


/*! Insert np either after curvertex (after!=0), or before (after==0).
 * curvertex must be in this->path somewhere.
 *
 * np must be an open string of Coordinates.
 *
 * Return 0 for success, or nonzero error, such as curvertex not in path somewhere.
 */
int Path::AddAt(Coordinate *curvertex, Coordinate *np, int after)
{
	int i;
	if (path->hasCoord(curvertex, &i)==0) return 1;
	if (!after) i--;
	if (i<0 && IsClosed()) i+=NumVertices(NULL);


	int newverts=np->NumPoints(1);

	 //now add to existing path
	Coordinate *npend=NULL;
	npend=np;
	np=np->firstPoint(0);
	while (npend->next && npend->next!=np) npend=npend->next;


	Coordinate *ins=curvertex;
	if (after) {
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

	for (int c=0; c<pathweights.n; c++) {
		if (pathweights.e[c]->t>i) pathweights.e[c]->t+=newverts;
	}

	return 0;
}

/*! Return 1 for success, 0 for failure (such as curvertex not found).
 */
int Path::CutSegment(Coordinate *curvertex, int after, Path **remainder)
{
	if (remainder) *remainder = nullptr;

	if (IsClosed()) {
		openAt(curvertex, after);
		return 1;
	}

	// ensure we are at a valid vertex
	curvertex = curvertex->previousVertex(after ? 1 : 0);
	if (!curvertex) return 0;

	// cut the path
	Coordinate *cutafter = curvertex;
	while (cutafter->next && (cutafter->next->flags & POINT_TOPREV)) cutafter = cutafter->next;

	Path *remains = nullptr;
	Coordinate *atnext = cutafter->disconnect(true);
	if (atnext) {
		if (remainder) {
			remains = *remainder = new Path(atnext, linestyle);
			remains->defaultwidth = defaultwidth;
			if (profile) { remains->profile = profile; profile->inc_count(); }
		} else delete atnext;
	}

	//divide weight nodes
	if (pathweights.n) {
		int n = NumVertices(nullptr);
		for (int c=pathweights.n-1; c >= 0; c--) {
			if (pathweights.e[c]->t <= n) break;
			PathWeightNode *node = pathweights.pop(c);
			if (pathweights.e[c]->t <= n+1) {
				// node in the cut zone
				delete node;
				continue;
			}
			if (remains) {
				node->t -= n;
				remains->pathweights.push(node, 1, 0);
			}
		}
	}

	needtorecache = 1;
	return 1;
}

/*! Make the path an open path by cutting the segment either after cc or before.
 *
 * curvertex must be a vertex. Anything else will be skipped when locating curvertex in path.
 * If curvertex is not found in path, then nothing is done.
 * If curvertex is null, then tries to break before this->path.
 *
 * If path is already open, nothing is done, and 0 is returned.
 * If the segment is cut, 1 is returned.
 *
 * Any weight nodes in the cut segment are removed. Other nodes are repositioned
 * to the new bezier indexing.
 *
 * Return 0 success, or nonzero for not opened,
 * such as when path already open, or path is degenerate, or curvertex is not in path.
 */
int Path::openAt(Coordinate *curvertex, int after)
{
	if (!path) return 1;

	if (curvertex == NULL) {
		curvertex = path->previousVertex(0);
		if (!curvertex) return 2;  // is null when path is open
	}

	int i = 0;
	Coordinate *p = path;
	while (p && p != curvertex) {
		p = p->nextVertex(0);
		i++;
		if (p == path) break;
	}

	if (!p) return 2; //path already open!
	if (p != curvertex) return 3; //curvertex not in path!!

	if (pathweights.n) {
		if (!after) i--;
		int numverts = NumVertices(NULL);
		if (i<0) i=numverts-1;

		//So the vertex to cut after is at index i, and the new first point is at i+1
		//If there is variable width across the cut path, need to create new approximated nodes at new endpoints,
		//and delete any old nodes within cut path
		if (!ConstantWidth() || HasOffset() || Angled()) {
			 //we need to add a couple of nodes at the new endpoints...
			if (pathweights.n>1) {
				InsertWeightNode(i);
				InsertWeightNode(i+1<numverts ? i+1 : 0);
			} else {
				if (pathweights.e[0]->t>i && pathweights.e[0]->t<i+1) pathweights.e[0]->t=i;
			}
		}

		 //...and remove any in the cut area
		for (int c=pathweights.n-1; c>=0; c--) {
			if (pathweights.e[c]->t>i && pathweights.e[c]->t<i+1) 
				RemoveWeightNode(c);
		}

		//reposition nodes if necessary
		if (i+1!=numverts) for (int c=0; c<pathweights.n; c++) {
			if (pathweights.e[c]->t<=i) pathweights.e[c]->t+=numverts-(i+1);
			else if (pathweights.e[c]->t>=i+1) pathweights.e[c]->t-=i+1;
		}
	}

	if (after) {
		p=curvertex;
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

	} else { //break prev
		p=curvertex;
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
	}

	path=path->firstPoint(1);

	needtorecache=1;
	return 0;
}

int Path::RemoveDoubles(double threshhold)
{
	threshhold*=threshhold;

	int n=0;
	Coordinate *p=path, *start=path, *p2, *pp;

	do {
		if (!(p->flags&POINT_VERTEX)) { p=p->next; continue; }
		p2=p->nextVertex(0);
		if (p2==start) break;

		if (norm2(p2->p() - p->p()) <= threshhold) {
			p2=p2->next;
			do {
				pp=p->next;
				pp->detach();
				delete pp;
			} while (p->next != p2);

			n++;
		}

		p=p->nextVertex(0);
	} while (p && p!=start);

	return n;
}

//! Return the coordinate that is the closest <= t, or NULL if no such t exists.
Coordinate *Path::GetCoordinate(double t)
{
	if (!path) return NULL;

	Coordinate *p=path;
	while (p && t>1) {
		p=p->nextVertex(0);
		t-=1;
	}
	return p;
}

/*! For each t, insert an extra point, preserving the curve shape.
 * Return 0 for success or nonzero error.
 *
 * If t_ret != null, return the new t values corresponding to the old.
 */
int Path::AddAt(double *t, int n, double *t_ret)
{
	if (!t || n<=0) return 1;

	bool closed = false;
	int max = NumVertices(&closed);
	if (closed) max++;
	for (int c=0; c<n; c++) if (t[c] < 0 || t[c]>max) return 1;

	double *tt = new double[n];
	memcpy(tt, t, n*sizeof(double));
	for (int c=0; c<n; c++) {
		double tv = tt[c];
		AddAt(tv);
		//update remaining t values
		double tfloor = floor(tv);
		double tceil = ceil(tv);
		for (int c2=0; c2<n; c2++) {
			if (tt[c2] >= tceil) tt[c2]++;
			else if (tt[c2] > tfloor && tt[c2] <= tv) {
				tt[c2] = tfloor + (tt[c2]-tfloor) / (tv - tfloor);
			} else if (tt[c2] > tv && tt[c2] < tceil) {
				tt[c2] = tv + (tceil - tt[c2]) / (tceil - tv);
			}
		}
	}

	if (t_ret) memcpy(t_ret, tt, n*sizeof(double));
	delete[] tt;
	return 0;
}

/*! Create a new point at t, and return a reference to that new point.
 * It divies up adjacent control handles to maintain shape. If there are no control points, then
 * a linear point is added (1 Coordinate). If there are control handles adjacent, then
 * a 3 Coordinate point (control-vertex-control) is added and the vertex point is returned.
 *
 * If t does not exist in the line, then return NULL.
 *
 * \todo *** need to sort what happens when you try to cut a segment that has non-bez SegmentControls..
 */
Coordinate *Path::AddAt(double t)
{
	Coordinate *p1=GetCoordinate(t);
	if (!p1) return NULL;
	int index=t;
	double tt=t-(int)t;

	 //need to install a point between p1 and next vertex p2
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


	bez_subdivide(tt, p1->p(), c1->p(), c2->p(), p2->p(),  pts);
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

	c1->insert(np,1);

	 //update weight nodes
	for (int c=0; c<pathweights.n; c++) {
		if (pathweights.e[c]->t>=index && pathweights.e[c]->t<=t && tt!=0)
			pathweights.e[c]->t=index + (pathweights.e[c]->t-index)/tt; //node is to left of AddAt t within same bez segment

		else if (pathweights.e[c]->t>t && pathweights.e[c]->t<=index+1 && 1-tt!=0)
			pathweights.e[c]->t=index + 1 + (pathweights.e[c]->t-t)/(1-tt); //node is to right of AddAt t within same bez segment

		else if (pathweights.e[c]->t>index+1) pathweights.e[c]->t++; //node is in future bez segment
	}

	needtorecache=1;
	return cp;
}

/*! Cut the path at t. Optionally return the remainder path in new_path_ret.
 * Return value is 0 for success, or nonzero for error and no cut happens.
 */
int Path::CutAt(double t, Path **new_path_ret)
{
	if (new_path_ret) *new_path_ret = nullptr;

	Coordinate *p1 = GetCoordinate(t);
	if (!p1) return 1;
	int index = t;
	double tt = t-(int)t;
	bool closed = IsClosed();

	 //need to install a point between p1 and next vertex p2
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

	bez_subdivide(tt, p1->p(), c1->p(), c2->p(), p2->p(),  pts);
	if (c1 != p1) c1->p(pts[0]);
	if (c2 != p2) c2->p(pts[4]);
	Coordinate *np, *npp = nullptr, *cp;
	if (c1==p1 && c2==p2) {
		 //add segment point, not bez
		np = new Coordinate(pts[2], POINT_VERTEX | BEZ_NSTIFF_NEQUAL, NULL);
		if (new_path_ret) npp = new Coordinate(pts[2], POINT_VERTEX | BEZ_NSTIFF_NEQUAL, NULL);
		cp = np;
	} else {
		 //add bez
		np = new Coordinate(pts[1],POINT_TONEXT,NULL);
		np->connect(new Coordinate(pts[2],POINT_VERTEX|POINT_REALLYSMOOTH|BEZ_STIFF_EQUAL,NULL),1);
		np->next->connect(new Coordinate(pts[3],POINT_TOPREV,NULL),1);
		if (new_path_ret) {
			npp = new Coordinate(pts[1],POINT_TONEXT,NULL);
			npp->connect(new Coordinate(pts[2],POINT_VERTEX|POINT_REALLYSMOOTH|BEZ_STIFF_EQUAL,NULL),1);
			npp->next->connect(new Coordinate(pts[3],POINT_TOPREV,NULL),1);
		}
		cp = np->next;
	}

	c1->insert(np,1);
	if (!closed) {
		if (cp->next && (cp->next->flags & POINT_TOPREV)) cp = cp->next;
		Coordinate *nxt = cp->disconnect(true);
		if (new_path_ret) {
			npp->lastPoint(0)->connect(nxt);
			*new_path_ret = new Path(npp);
		} else {
			delete nxt;
		}
	}

	 //update weight nodes
	int fornew = -1;
	for (int c=0; c<pathweights.n; c++) {
		if (pathweights.e[c]->t>=index && pathweights.e[c]->t<=t && tt!=0) {
			pathweights.e[c]->t=index + (pathweights.e[c]->t-index)/tt; //node is to left of t within same bez segment

		} else if (pathweights.e[c]->t>t && pathweights.e[c]->t<=index+1 && 1-tt!=0) {
			pathweights.e[c]->t=index + 1 + (pathweights.e[c]->t-t)/(1-tt); //node is to right of t within same bez segment
			if (!closed && fornew == -1) fornew = c;

		} else if (pathweights.e[c]->t>index+1) {
			pathweights.e[c]->t++; //node is in future bez segment
		}
	}
	if (fornew >= 0) {
		for (int c=fornew; c<pathweights.n; c++) {
			if (new_path_ret) {
				PathWeightNode *node = pathweights.e[c];
				(*new_path_ret)->AddWeightNode(node->t, node->offset, node->width, node->angle);
			}
		}
		while (pathweights.n > fornew) pathweights.remove(fornew);
	}

	needtorecache=1;
	return 2;
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

/*! A more thorough version of PointAlongPath().
 * 
 * Returns 1 for point found, -1 for point clamped to beginning point, -2 clamped to end,
 * or 0 if there is not a valid path available.
 *
 * \todo tangentbefore doesn't compute correctly at corners
 */
int Path::PointInfo(double t, int tisdistance, flatpoint *point, flatpoint *tangentafter, flatpoint *tangentbefore,
						            flatpoint *ptop, flatpoint *pbottom,
									double *offset_ret, double *width_ret, double *angle_ret)
{
	flatpoint pp,po,tan;
	int status=PointAlongPath(t,tisdistance, &pp,&tan);
	if (status==0) return 0;

	if (point) *point=pp;
	if (tangentafter)  *tangentafter =tan;
	if (tangentbefore) *tangentbefore=-tan; // *** LAZY HACK!! before not necessarily -tan at corners

	if (ptop || pbottom || offset_ret || width_ret || angle_ret) {
		UpdateCache();
		if (tisdistance) t=distance_to_t(t,NULL);
		double a, offset,width;

		a     =cache_angle .f(t);
		offset=cache_offset.f(t);
		width =cache_width .f(t);

		if (angle_ret)  *angle_ret =a;
		if (width_ret)  *width_ret =width;
		if (offset_ret) *offset_ret=offset;

		if (ptop || pbottom) {
			flatpoint vt=transpose(tan);
			vt.normalize();
			po=pp+vt*offset;

			if (absoluteangle) vt=rotate(flatpoint(1,0),a);
			else vt=rotate(transpose(vt), a);

			if (ptop)    *ptop   =po + vt*width/2;
			if (pbottom) *pbottom=po - vt*width/2;
		}
	}

	return status;
}

/*! t is measured from path->firstVertex(1), and thus should be positive.
 *
 * Returns 1 for point found, -1 for point clamped to beginning point, -2 clamped to end,
 * or 0 if there is not a valid path available.
 *
 * \todo be able to wind around the path if continuous, or go backwards (for negative t)
 * \todo must implement find tangent at clamped endpoints
 * \todo *** note that if you have a loop of null points, and you are going along based
 *   on distance, than this will not break!!
 */
int Path::PointAlongPath(double t, //!< Either visual distance or bezier parameter, depending on tisdistance
						 int tisdistance, //!< 1 for visual distance, 0 for t is bez parameter
						 flatpoint *point, //!< Return point
						 flatpoint *tangent,
						 int resolution) //!< Return tangent at point
{
	if (!path) return 0;

	Coordinate *start=path->firstPoint(1);
	if (!start) return 0;
	Coordinate *p=start, *c1, *c2, *v2;
	double dd;
	flatpoint fp;

	if (t<=0 && !IsClosed()) {
		if (point) *point=start->p();
		if (tangent) {
			if (start->next && start->next->flags&POINT_TOPREV) *tangent=start->next->p()-start->p();
			else *tangent=start->direction(1);
		}
		return -1;
	}

	if (t<=0) { //is a closed path
		if (!tisdistance) {
			int n=NumVertices(NULL);
			while (t<0) t+=n;
		}
	}

	if (t<0 && tisdistance) {
		 //to be here, t is distance AND the path is closed..

		do { //one iteration for each segment (a vertex to next vertex)
			c1=c2=v2=NULL;
			v2=p->prev;
			if (!v2) break;

			if (v2->flags&POINT_TONEXT) { c1=v2; v2=v2->prev; if (!v2) break; }
			else { c1=p;  }

			if (v2->flags&POINT_TOPREV) { c2=v2; v2=v2->prev; if (!v2) break; }
			else { c2=v2; }

			if (c1==p && c2==v2) {
				 //we have the simpler case of a line segment
				fp=v2->p()-p->p(); //current segment

				dd=norm(fp);
				if (-t>dd) { t+=dd; p=v2; continue; } //not on this segment!

				if (point) *point=(dd ? p->p()-t*fp/dd : p->p());
				if (tangent) *tangent=-fp;
				return 1;

			} else {
				 //must deal with a bezier segment
				double tt=0;

				dd=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), resolution);
				if (-t>dd) { t+=dd; p=v2; continue; } //not on this segment!
				tt=bez_distance_to_t(-t, p->p(),c1->p(),c2->p(),v2->p(), resolution);
				
				if (point) *point=bez_point(tt, p->p(),c1->p(),c2->p(),v2->p());
				if (tangent) {
					*tangent = -bez_point(tt+.01, p->p(),c1->p(),c2->p(),v2->p())
							   +bez_point(tt, p->p(),c1->p(),c2->p(),v2->p());
				}
				return 1;
			}
		} while (1);

		//theoretically you'll never break out of the loop
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
				dd=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), resolution);
				if (t>dd) { t-=dd; p=v2; continue; } //not on this segment!
				tt=bez_distance_to_t(t, p->p(),c1->p(),c2->p(),v2->p(), resolution);
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
flatpoint Path::ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist, int resolution)
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
			vdd+=sqrt(ss);   //update running visual distance

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
			vdd+=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), resolution);   //update running visual distance
		}

		p=v2;
		tt+=1;

	} while (p!=start);

	if (disttopath) *disttopath=d;
	if (distalongpath) *distalongpath=vd;
	if (tdist) *tdist=t;
	return found;
}

/*! Find all extrema in the path. For each point,
 * A point.info is a direction according to its acceleration direction (2nd derivative), so:
 * if a point is a left, vertical extrema, its info is LAX_RIGHT.
 * if a point is a right, vertical extrema, its info is LAX_LEFT.
 * if a point is a top, horizontal extrema, its info is LAX_BOTTOM.
 * if a point is a bottom, horizontal extrema, its info is LAX_TOP.
 */
int Path::FindExtrema(NumStack<flatpoint> *points_ret, NumStack<double> *t_ret)
{
	if (!path) return 0;

	int n = 0;
	Coordinate *p = path, *p2;
	Coordinate *start = p;
	flatpoint c1, c2;
	int isline;
	double extrema[5];
	flatpoint extremap[5];

	do {
		if (p->getNext(c1,c2,p2,isline) != 0) break;

		int ne = bez_extrema(p->p(), c1, c2, p2->p(), extrema, extremap);
		for (int c=0; c<ne; c++) {
			if (points_ret)	points_ret->push(extremap[c]);
			if (t_ret) t_ret->push(extrema[c]);
		}

		n += ne;
		p = p2;
	} while (p && p != start);
	return n;
}

/*! Return the number of vertices in the path (points on the line, not control handles).
 *
 * If isclosed_ret!=NULL, then fill it with whether the path is closed or not.
 */
int Path::NumVertices(bool *isclosed_ret)
{
	if (!path) return 0;

	int n=0;
	Coordinate *start=path->firstPoint(1);
	Coordinate *p=start;
	do {
		p=p->nextVertex(0);
		n++;
	} while (p && p!=start);

	if (isclosed_ret) *isclosed_ret=(p==start);
	return n;
}

bool Path::IsClosed()
{
	if (!path) return false;

	Coordinate *start=path->firstPoint(1);
	Coordinate *p=start;

	do {
		p=p->nextVertex(0);
	} while (p && p!=start);

	return (p==start);
}

/*! Return a vertex index. For instance if p==path, then 0 is returned.
 * If p==path->nextVertex(0), then 1 is returned. Note that p MUST 
 * actually be a vertex, not a control handle.
 *
 * If ignore_controls==true, then count any bezier vertex.
 * Otherwise, skip all segments that use the same SegmentControls to get
 * to the next vertex.
 *
 * \todo (NOTE: ignore_controls NOT IMPLEMENTED!!)
 *
 * If p is not found in path, then -1 is returned.
 */
int Path::GetIndex(Coordinate *p, bool ignore_controls)
{
	int i=0;
	Coordinate *pp=path;
	while (pp && pp!=p) {
		pp=pp->nextVertex(0);
		i++;
		if (pp==path) break;
	}
	if (pp==NULL || (pp==path && i>0)) return -1;
	return i;
}

/*! Return 1 for *this completely contains otherpath.
 * Return -1 for *this contains some, but not all of otherpath.
 * Return 0 for no points of otherpath in *this.
 *
 * This is a very rough approximation that naively checks whether any vertex (control points ignored),
 * is contained in *this.
 */
int Path::Contains(Path *otherpath)
{
	if (!otherpath || !otherpath->path) return 0;
	if (!path) return 0;
	UpdateCache();

	Coordinate *p, *start;
	p = start = otherpath->path;
	int n = 0, ni = 0;
	do {
		if (p->flags & POINT_VERTEX) {
			n++;
			if (point_is_in_bez(p->p(), centercache.e, centercache.n))
				ni++;
		}
		p = p->next;
	} while (p && p != start);

	if (!ni) return 0;
	if (n == ni) return 1;
	return -1;
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
			d += bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), 30);   //update running visual distance
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
double Path::t_to_distance(double tt, int *err, int resolution)
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
			sd=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), resolution);   //update running visual distance
			if (tt>t+1) {
				d+=sd;
				p=v2;
				t++;
				continue;
			}
			d+=bez_t_to_distance(tt-t, p->p(),c1->p(),c2->p(),v2->p(), resolution);
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
double Path::distance_to_t(double distance, int *err, int resolution)
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
			sd=bez_segment_length(p->p(),c1->p(),c2->p(),v2->p(), resolution);   //update running visual distance
			if (distance>sd) { //not terribly efficient here
				distance-=sd;
				p=v2;
				t++;
				continue;
			}
			t+=bez_distance_to_t(distance, p->p(),c1->p(),c2->p(),v2->p(), resolution);
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
	int nv=-1;
	do {
		if (p->flags&POINT_VERTEX) nv++;

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

 	bool closed=(p==start);
	if (closed) {
		nv++;
	} else {
		path=path->firstPoint(0);
	}

	 //now must update weight nodes, if any

	for (int c=0; c<pathweights.n; c++) {
		pathweights.e[c]->t = nv-pathweights.e[c]->t;
		pathweights.e[c]->offset = -pathweights.e[c]->offset;
	}
	for (int c=0; c<pathweights.n/2; c++) {
		pathweights.swap(c,pathweights.n-1-c);
	}

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
 * See also Path, and Coordinate, LineStyle, and FillStyle.
 */


PathsData::PathsData(unsigned long ns)
{
	linestyle      = nullptr;
	fillstyle      = nullptr;
	generator_data = nullptr;
	generator      = -1;
	style          = ns;
}

/*! Dec count of linestyle and fillstyle.
 */
PathsData::~PathsData()
{
	if (linestyle) linestyle->dec_count();
	if (fillstyle) fillstyle->dec_count();
	if (generator_data) generator_data->dec_count();
}

/*! Note paths with single points count as non-empty.
 */
bool PathsData::IsEmpty()
{
	for (int c=0; c<paths.n; c++) {
		if (paths.e[c]->path) return false;
	}
	return true;
}

int PathsData::Map(std::function<int(const flatpoint &p, flatpoint &newp)> adjustFunc)
{
	int changed = 0;
	flatpoint p;
	for (int c=0; c<paths.n; c++) {
		Coordinate *coord = paths.e[c]->path;
		if (!coord) continue;
		Coordinate *first = coord;
		do {
			if (adjustFunc(coord->fp, coord->fp)) changed++;
			coord = coord->next;
		} while (coord && coord != first);
	}
	if (changed) touchContents();
	return changed;
}

SomeData *PathsData::duplicate(SomeData *dup)
{
	PathsData *newp=dynamic_cast<PathsData*>(dup);
	if (!newp && dup) return NULL; //was not a PathsData!

	if (!dup) {
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
		if (dup) dup->setbounds(minx,maxx,miny,maxy);
		newp=dynamic_cast<PathsData*>(dup);
	} 
	if (!newp) {
		newp=new PathsData(style);
		dup=newp;
	}
	newp->style=style;

	Path *path;
	for (int c=0; c<paths.n; c++) {
		path=paths.e[c]->duplicate();
		newp->paths.push(path);
	}

	newp->linestyle=linestyle; if (linestyle) linestyle->inc_count();
	newp->fillstyle=fillstyle; if (fillstyle) fillstyle->inc_count();
	
	return newp;
}

/*! If whichpath>=0, then return thatpath->Weighted().
 * Else return true if ANY path is weighted.
 */
bool PathsData::Weighted(int whichpath)
{
	if (whichpath>=0 && whichpath<paths.n) return paths.e[whichpath]->Weighted();
	for (int c=0; c<paths.n; c++) if (paths.e[c]->Weighted()) return true;
	return false;
}

/*! If whichpath>=0, then return thatpath->HasOffset().
 * Else return true if ANY path has offset.
 */
bool PathsData::HasOffset(int whichpath)
{
	if (whichpath>=0 && whichpath<paths.n) return paths.e[whichpath]->HasOffset();
	for (int c=0; c<paths.n; c++) if (paths.e[c]->HasOffset()) return true;
	return false;
}

/*! If whichpath>=0, then return thatpath->Angled().
 * Else return true if ANY path has Angled().
 */
bool PathsData::Angled(int whichpath)
{
	if (whichpath>=0 && whichpath<paths.n) return paths.e[whichpath]->Angled();
	for (int c=0; c<paths.n; c++) if (paths.e[c]->Angled()) return true;
	return false;
}

/*! Make each subpath need to recache. If now, then recache right now,
 * otherwise, just set path->needtorecache=1 on each subpath.
 */
void PathsData::Recache(bool now)
{
	for (int c=0; c<paths.n; c++) {
		paths.e[c]->needtorecache=1;
		if (now) paths.e[c]->UpdateCache();
	}
}

/*! If whichpath<0 then ApplyOffset() on all paths, else just do that one.
 */
int PathsData::ApplyOffset(int whichpath)
{
	if (whichpath>=0 && whichpath<paths.n) paths.e[whichpath]->ApplyOffset();
	else for (int c=0; c<paths.n; c++) paths.e[c]->ApplyOffset();

	return 0;
}

/*! If whichpath<0 then SetOffset() on all paths, else just do that one.
 */
int PathsData::SetOffset(int whichpath, double towhat)
{
	if (whichpath>=0 && whichpath<paths.n) paths.e[whichpath]->SetOffset(towhat);
	else for (int c=0; c<paths.n; c++) paths.e[c]->SetOffset(towhat);

	return 0;
}

/*! If whichpath<0 then SetOffset() on all paths, else just do that one.
 */
int PathsData::SetAngle(int whichpath, double towhat, int absolute)
{
	if (whichpath>=0 && whichpath<paths.n) paths.e[whichpath]->SetAngle(towhat, absolute);
	else for (int c=0; c<paths.n; c++) paths.e[c]->SetAngle(towhat, absolute);

	return 0;
}

/*! If whichpath>=0 than do just that one. Else call MakeStraight(NULL,NULL,asbez) for all paths.
 */
int PathsData::MakeStraight(int whichpath, Coordinate *from, Coordinate *to, bool asbez)
{
	if (whichpath>=0 && whichpath<paths.n) paths.e[whichpath]->MakeStraight(from,to,asbez);
	else for (int c=0; c<paths.n; c++) paths.e[c]->MakeStraight(from,to,asbez);

	return 0;
}

/*! If which==-1, flush all paths. If not, then remove path with that index.
 */
void PathsData::clear(int which)
{
	touchContents();
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

	if (width > 0) linestyle->width = width;
	if (cap >= 0)  linestyle->capstyle = cap;
	if (join >= 0) linestyle->joinstyle = join;
	if (color) linestyle->Color(color->red, color->green, color->blue, color->alpha);

	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c]->linestyle || paths.e[c]->linestyle == linestyle) {
			paths.e[c]->defaultwidth = width;
		}
	}

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
	fillstyle->fillstyle= FillSolid;
	if (fillstyle->function == LAXOP_None) fillstyle->function = LAXOP_Over;

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
void PathsData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%smatrix 1 0 0 1 0 0  #standard transform matrix\n",spc);
		fprintf(f,"%slinestyle     #default line style\n",spc);
		fprintf(f,"%s  ...         #standard linestyle attributes\n",spc);
		fprintf(f,"%sfillstyle\n",spc);
		fprintf(f,"%s  ...         #standard fillstyle attributes\n",spc);
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

	LineStyle *ls;
	for (int c=0; c<paths.n; c++) {
		fprintf(f,"%spath %d\n",spc,c);
		if (paths.e[c]->linestyle==linestyle) {
			 //blank out the linestyle when is same as overall linestyle
			ls=linestyle;
			paths.e[c]->linestyle=NULL;
		} else ls=NULL;
		paths.e[c]->dump_out(f,indent+2,what,context);
		if (ls!=NULL) {
			 //restore blanked out linestyle
			paths.e[c]->linestyle=ls;
		}
	}
}

Laxkit::Attribute *PathsData::dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("matrix","1 0 0 1 0 0","standard transform matrix");
		att->push("linestyle",nullptr,"default line style");
		att->push("fillstyle",nullptr,"default fill style");
		att->push("d","m 1 1 l 1 2","optional paths defined by svg d path string");
		Attribute *att2 = att->pushSubAtt("path",nullptr,"none or more of these, defines single paths");
		Path p;
		p.dump_out_atts(att2, what, context);
		return att;
	}

	char scratch[200];

	sprintf(scratch, "%.10g %.10g %.10g %.10g %.10g %.10g", m(0),m(1),m(2),m(3),m(4),m(5));
	att->push("matrix", scratch);

	if (linestyle) {
		Attribute *att2 = att->pushSubAtt("linestyle");
		linestyle->dump_out_atts(att2, what, context);
	}

	if (fillstyle) {
		Attribute *att2 = att->pushSubAtt("fillstyle");
		fillstyle->dump_out_atts(att2, what, context);
	}

	att->push("style", style);

	LineStyle *ls;
	for (int c=0; c<paths.n; c++) {
		att->push("path", c);
		Attribute *att2 = att->Top();

		if (paths.e[c]->linestyle==linestyle) {
			 //temporarily blank out the linestyle when is same as overall linestyle
			 //so we don't needlessly duplicate output
			ls=linestyle;
			paths.e[c]->linestyle=NULL;
		} else ls=NULL;

		paths.e[c]->dump_out_atts(att2,what,context);

		if (ls!=NULL) {
			 //restore blanked out linestyle
			paths.e[c]->linestyle=ls;
		}
	}

	return att;
}

//! Basically reverse of dump_out..
/*! If the dump is supposed to replace the current settings, then
 * they should have been flushed previously. New paths read in here
 * are appended to paths stack.
 */
void PathsData::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	SomeData::dump_in_atts(att,flag,context);

	char *name,*value;
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
			SvgToPathsData(this, value, NULL, NULL);
		}
	}

	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c]->linestyle) paths.e[c]->Line(linestyle);
	}

	FindBBox();
}

//! Find the distance along the path between the bounds, or whole length if tend<tstart.
double PathsData::Length(int pathi, double tstart,double tend)
{
	if (pathi<0 || pathi>=paths.n) return 0;
	return paths.e[pathi]->Length(tstart,tend);
}

/*! Connect paths via closing them. So connecting endpoints from same path will simple close it.
 * Connecting form different paths will merge the paths, and "close" the gap between the endpoints.
 *
 * If frompathi<0 or toi<0, then search for a path containing from, likewise for to.
 * from and to must be endpoints, and may or may not be on same path.
 *
 * Return 0 for success, or nonzero for couldn't connect and nothing done.
 */
int PathsData::ConnectEndpoints(Coordinate *from,int frompathi, Coordinate *to,int topathi)
{
	 //find which sides they are endpoints
	 //make to be same path direction as from
	if (from==to) return 1;
	if (frompathi<0) frompathi=hasCoord(from);
	if (topathi<0) topathi=hasCoord(to);
	if (topathi<0 || frompathi<0) return 2;

	int fromdir=from->isEndpoint();
	int todir  =to->isEndpoint();
	if (!fromdir || !todir) return 3;
	if (frompathi==topathi) {
		 //simple, endpoints are on same path, so just close the path
		paths.e[frompathi]->close();
		return 0;
	}

	 //non-simple case, need to connect two different paths...
	int numfromverts=paths.e[frompathi]->NumVertices(NULL);
	if ((fromdir>0 && todir>0) || (fromdir<0 && todir<0)) ReversePath(topathi);

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

	paths.e[frompathi]->path=to->firstPoint(1);
	paths.e[frompathi]->needtorecache=1;
	paths.e[topathi]  ->needtorecache=1;

	 //remove other path
	for (int c=0; c<paths.e[topathi]->pathweights.n; c++) {
		paths.e[frompathi]->AddWeightNode(
				paths.e[topathi]->pathweights.e[c]->t + numfromverts,
				paths.e[topathi]->pathweights.e[c]->offset,
				paths.e[topathi]->pathweights.e[c]->width,
				paths.e[topathi]->pathweights.e[c]->angle);
	}
	paths.e[topathi]->path=NULL;
	paths.remove(topathi);

	return 0;
}

/*! Take 2 end points, and make them the same point, merging paths if necessary.
 * If there is merging and fromi != toi, path toi is removed. Note that if fromi < toi,
 * then the actual remaining path aftewards will be fromi-1.
 *
 * Return 0 for nothing done such as when fromi and toi are bad indices.
 * Return 1 for merged.
 */
int PathsData::MergeEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi)
{
	//find which sides they are endpoints
	//make to be same path direction as from

	if (fromi<0) fromi = hasCoord(from);
	if (toi<0)   toi = hasCoord(to);
	if (toi<0 || fromi<0) return 0;

	int fromdir = from->isEndpoint();
	int todir   = to->isEndpoint();
	if (!fromdir || !todir) return 0;

	if ((fromdir>0 && todir>0) || (fromdir<0 && todir<0)) ReversePath(toi);

	//need to take prev of from and put on to, removing to's prev (for fromdir>0)
	Coordinate *fp;
	Coordinate *tp;
	Coordinate *tmp;

	//standardize connect order for code below
	if (fromdir < 0) {
		tmp    = from;
		from   = to;
		to     = tmp;
		int tt = fromi;
		fromi  = toi;
		toi    = tt;
	}

	//we connect from final path point to beginning path point
	fp = from->lastPoint(0);
	tp = to->firstPoint(0);

	//first remove in between control points
	if (fp->flags & POINT_TOPREV) {
		fp        = fp->prev;
		tmp       = fp->next;
		fp->next  = NULL;
		tmp->prev = NULL;
		delete tmp;
	}
	if (tp->flags & POINT_TONEXT) {
		tp        = tp->next;
		tmp       = tp->prev;
		tp->prev  = NULL;
		tmp->next = NULL;
		delete tmp;
	}

	// now fp and tp should be vertices, need to remove fp
	fp        = fp->prev;
	tmp       = fp->next;
	fp->next  = NULL;
	tmp->prev = NULL;
	delete tmp;

	// finally connect the terminal points
	fp->next = tp;
	tp->prev = fp;

	paths.e[fromi]->path = to->firstPoint(1);
	paths.e[fromi]->needtorecache = 1;
	
	if (fromi != toi) {
		// remove other path
		paths.e[toi]->path = NULL;
		paths.remove(toi);
	}

	return 1;
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
	if (where<0 || where>paths.n) where=paths.n;
	paths.push(new Path(NULL,nls),1,where);
	if (!paths.e[where]->linestyle && linestyle) {
		paths.e[where]->Line(linestyle);
		paths.e[where]->defaultwidth = linestyle->width;
	}
}

/*! Incs count on style, unless it is already installed (takes ownership of newlinestyle).
 */
void PathsData::InstallLineStyle(LineStyle *newlinestyle)
{
	if (linestyle==newlinestyle) return;
	LineStyle *old=linestyle;
	if (linestyle) linestyle->dec_count();
	linestyle=newlinestyle;
	if (newlinestyle) newlinestyle->inc_count();

	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c]->linestyle || paths.e[c]->linestyle==old) paths.e[c]->Line(newlinestyle);
	}
}

/*! Incs count on style, unless it is already installed.
 */
void PathsData::InstallFillStyle(FillStyle *newfillstyle)
{
	if (fillstyle==newfillstyle) return;
	if (fillstyle) fillstyle->dec_count();
	fillstyle=newfillstyle;
	if (newfillstyle) newfillstyle->inc_count();
}

//! Return the last point of the top path, or NULL if that doesn't exist
Coordinate *PathsData::LastVertex()
{
	if (!paths.n) return NULL;
	if (paths.e[paths.n-1]->path==NULL) return NULL;

	return paths.e[paths.n-1]->lastPoint(1);
}

/*! Create an ellipse, using num_vertices to sweep out angle radians.
 * If closed == 1, then close. if closed == 2, then close, but include center point.
 */
void PathsData::appendEllipse(flatpoint center, double xradius, double yradius, double angle, double offset, int num_vertices, int closed)
{
	if (num_vertices<1) num_vertices = 1;
	if (angle == 0) angle = 2*M_PI;

	//if (closed) num_vertices++;

    double xx,yy;
	flatpoint cp, p, cn;

	double start_angle = offset + angle;
	double theta = angle / (num_vertices);                              // radians between control points
	double v = 4 * (2 * sin(theta / 2) - sin(theta)) / 3 / (1 - cos(theta));  // length of control handle

	flatpoint xv(xradius,0);
	flatpoint yv(0,yradius);

    if (closed == 2) append(center);

    for (int c=0; c<num_vertices; c++) {
        xx = cos(start_angle + c*theta);
        yy = sin(start_angle + c*theta);

        p  = center +    xx*xv +    yy*yv;
        cp =   p    +  v*yy*xv + -v*xx*yv;
        cn =   p    + -v*yy*xv +  v*xx*yv;

		//if (closed && c==num_vertices-1) break;
		if (!closed || (closed && c != 0) || (closed && c == 0 && (angle == 0 || angle == 2*M_PI)))
			append(cp,POINT_TONEXT);
		append(p,POINT_VERTEX);
		if (!closed || (closed && c < num_vertices-1) || (closed && c == num_vertices-1 && (angle == 0 || angle == 2*M_PI)))
			append(cn,POINT_TOPREV);
    }

    if (closed) close();
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

void PathsData::appendSvg(const char *d)
{
	SvgToPathsData(this, d, NULL, NULL);
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

/*! Convenience function to append directly from a DoubleBBox.
 * If the box is not valid, nothing is added and false is returned. Else true is returned.
 */
bool PathsData::appendRect(DoubleBBox *box, int whichpath)
{
	if (!box || !box->validbounds()) return false;

	append(box->minx,box->miny, POINT_VERTEX, nullptr, whichpath);
	append(box->maxx,box->miny, POINT_VERTEX, nullptr, whichpath);
	append(box->maxx,box->maxy, POINT_VERTEX, nullptr, whichpath);
	append(box->minx,box->maxy, POINT_VERTEX, nullptr, whichpath);
	close(whichpath);
	FindBBox();
	return true;
}

/*! Replace subpath with a rounded rectangle.
 * If pathi is out of range, then push a new subpath.
 * Returns number of vertices in the resulting path.
 */
int PathsData::MakeRoundedRect(int pathi, double x, double y, double w, double h, flatpoint *sizes, int numsizes)
{
	if (pathi < 0 || pathi >= paths.n) {
		pushEmpty();
		pathi = paths.n-1;
	}
	int ret = paths.e[pathi]->MakeRoundedRect(x,y,w,h,sizes,numsizes);
	FindBBox();
	return ret;
}

/*! Replace subpath with a cubic squircle.
 * If pathi is out of range, then push a new subpath.
 * Returns number of vertices in the resulting path.
 */
int PathsData::MakeSquircleCubic(int pathi, double x, double y, double w, double h, double *sizes, int numsizes)
{
	if (pathi < 0 || pathi >= paths.n) {
		pushEmpty();
		pathi = paths.n-1;
	}
	int ret = paths.e[pathi]->MakeSquircleCubic(x,y,w,h,sizes,numsizes);
	FindBBox();
	return ret;
}

void PathsData::appendCoord(Coordinate *coord,int whichpath)
{
	if (whichpath<0 || whichpath>=paths.n) whichpath=paths.n-1;
	if (paths.n==0) {
		paths.push(new Path());
		if (!paths.e[paths.n-1]->linestyle && linestyle) {
			paths.e[paths.n-1]->Line(linestyle);
		}
	}
	if (whichpath<0) whichpath=0;
	paths.e[whichpath]->append(coord);
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
	if (paths.n==0) {
		paths.push(new Path());
		if (!paths.e[paths.n-1]->linestyle && linestyle) {
			paths.e[paths.n-1]->Line(linestyle);
		}
	}
	if (whichpath<0) whichpath=0;
	paths.e[whichpath]->append(x,y,flags,ctl);
}

/*! Starts a new subpath. If whichpath>=0, then make the subpath at that index, otherwise add to end.
 */
void PathsData::moveTo(flatpoint p,int whichpath)
{
	if (whichpath<0 || whichpath>=paths.n) whichpath=paths.n;
	pushEmpty(whichpath);
	append(p,POINT_VERTEX,NULL,whichpath);
}

void PathsData::lineTo(flatpoint p,int whichpath)
{
	append(p,POINT_VERTEX,NULL,whichpath);
}

void PathsData::curveTo(flatpoint c1, flatpoint c2, flatpoint p2, int whichpath)
{
	if (whichpath<0 || whichpath>=paths.n) whichpath=paths.n-1;
	if (whichpath<0) {
		whichpath=0;
		pushEmpty();
	}
	if (paths.e[whichpath]->path==NULL) append(c1,POINT_VERTEX,NULL,whichpath);

	append(c1,POINT_TOPREV,NULL,whichpath);
	append(c2,POINT_TONEXT,NULL,whichpath);
	append(p2,POINT_VERTEX,NULL,whichpath);
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

/*! Remove specified path, return 0 on success. -1 if path doesn't exist.
 * If popped_ret != NULL, then return the removed path there. Otherwise path is dec_counted.
 * Calling code needs to delete any returned path.
 */
int PathsData::RemovePath(int index, Path **popped_ret)
{
	if (index<0 || index >=paths.n) return 1;
	if (popped_ret) {
		*popped_ret = paths.pop(index);
		return 0;
	}

	paths.remove(index);
	touchContents();
	return 0;
}

/*! Return 0 for success or nonzero error.
 */
int PathsData::AddAt(int pathindex, double t)
{
	if (pathindex < 0 || pathindex >= paths.n) return 1;
	if (paths.e[pathindex]->AddAt(t)) return 0;
	return 2;
}

/*! Return 0 for success or nonzero error.
 * If paths == null, then assume path 0.
 */
int PathsData::AddAt(int n, int *paths, double *t)
{
	NumStack<int> ii;
	NumStack<double> tt;
	if (paths) {
		for (int c=0; c<n; c++) {
			if (paths[c] < 0 || paths[c] >= n) return 1;
			ii.push(paths[c]);
		}
	} else {
		for (int c=0; c<n; c++) ii.push(0);
	}
	
	for (int c=0; c<n; c++) {
		int pathi = ii.e[c];
		if (pathi < 0) continue;

		tt.flush();
		for (int c2=0; c2<n; c2++) {
			if (ii.e[c2] == pathi) tt.push(t[c2]);
		}

		if (this->paths.e[pathi]->AddAt(tt.e, tt.n, nullptr) != 0) {
			return 1;
		}

		for (int c2=0; c2<n; c2++) {
			if (ii.e[c2] == pathi) ii.e[c2] = -1;
		}
	}

	return 0;
}

/*! For each t in path indices paths, cut the path, possibly creating new paths.
 * Return 0 for success or nonzero error.
 */
int PathsData::CutAt(int n, int *paths, double *t)
{
	NumStack<int> ii;
	NumStack<double> tt;

	if (paths) {
		for (int c=0; c<n; c++) {
			if (paths[c] < 0 || paths[c] >= n) return 1;
			ii.push(paths[c]);
		}
	} else {
		for (int c=0; c<n; c++) ii.push(0);
	}
	
	for (int c=0; c<n; c++) {
		int pathi = ii.e[c];
		if (pathi < 0) continue;

		tt.flush();
		for (int c2=0; c2<n; c2++) {
			if (ii.e[c2] == pathi) tt.push(t[c2]);
		}

		double t = tt.e[c];
		double tf = floor(t);
		int status = CutAt(pathi, t);

		if (status == 1) { //path is cut, but it's just open now
			//need to cycle points, cut point is now end of line
			bool closed;
			int max = this->paths.e[pathi]->NumVertices(&closed);
			for (int c2=0; c2<n; c2++) {
				if (ii.e[c2] != pathi) continue;
				if (tt.e[c2] < t) {
					if (tt.e[c2] > tf) {
						tt.e[c2] = max + (tt.e[c2] - tf)/(t-tf);
					} else tt.e[c2] += max - tf;
				} else {
					if (tt.e[c2] < t) {
						tt.e[c2] = tt.e[c2] - tf + (tt.e[c2]-t)/(tf+1-t);
					} else tt.e[c2] -= tf;
				}
			}

		} else if (status == 2) { //path is cut, but added a new path
			// old path still has same start
			// new path starts at cut point
			for (int c2=0; c2<n; c2++) {
				if (ii.e[c2] != pathi) continue;
				if (tt.e[c2] <= t) {
					if (tt.e[c2] > tf) {
						tt.e[c2] = floor(t) + (tt.e[c]-tf)/(t-tf);
					}
				} else {
					if (tt.e[c2] >= tf) {
						tt.e[c2] -= tf;
					} else {
						if (t == tf+1) tt.e[c2] = 0;
						else tt.e[c2] = (tt.e[c2]-t)/(tf+1-t);
					}
				}
			}

		} else { // 0 means problem, aborted
			return 1;
		}
		
		// done with this path
		for (int c2=0; c2<n; c2++) {
			if (ii.e[c2] == pathi) ii.e[c2] = -1;
		}
	}

	return 0;
}

/*! Cut path at t.
 * If the path is closed, it becomes open, and 1 is returned.
 * If it is open, the path is split into two paths, the new path will be at pathindex+1,
 * and contain the upper t points. 2 is returned.
 * If t or pathindex is out of range, 0 is returned.
 */
int PathsData::CutAt(int pathindex, double t)
{
	if (pathindex < 0 || pathindex >= paths.n) return 0;

	Path *newpath = nullptr;
	int status = paths.e[pathindex]->CutAt(t, &newpath);
	if (status != 0) return 0;
	if (newpath) paths.push(newpath, -1, pathindex+1);
	return newpath ? 2 : 1;
}

/*! Cut the segment in the after direction from coord.
 * If the path was closed, it just becomes open.
 * If the path was open already, then a new subpath will be created from the remainder.
 * When the cut operation results in a new subpath of a single point, that
 * subpath is removed or not according to remove_dangling.
 * If an open path of two vertices is cut, the original path will become a path of one point
 * and the dangling point is removed according to remove_dangling.
 *
 * Return 0 for coord not found.
 * Return 1 for successful cutting, and coord is still in the path.
 * Return 2 for successful cutting and a dangling coord was also cut.
 */
int PathsData::CutSegment(Coordinate *coord, bool after, bool remove_dangling)
{
	int pathi = hasCoord(coord);
	if (pathi < 0) return 0;

	Path *path = paths.e[pathi];
	if (path->IsClosed()) {
		path->openAt(coord, after);
		return 1; //note if it was a two point closed curve, we will still have a visible segment
	}

	Path *remainder = nullptr;
	path->CutSegment(coord, after, &remainder);
	int ret = 1;
	if (remainder) {
		if (remainder->NumVertices(nullptr) == 1 && remove_dangling) {
			delete remainder; //we don't really care about the dangling
			// *** Path should really be refcounted
			// remainder->dec_count(); //we don't really care about the dangling
			ret = 2;
		} else {
			paths.push(remainder, -1, pathi+1);
		}
	}

	FindBBox();
	return ret;
}

/*! Return that path, or if index==-1, return top path. Returns direct reference to it, NOT a copy.
 * Else Return NULL if index out of range.
 */
Path *PathsData::GetPath(int index)
{
	if (index==-1) index=paths.n-1;
	if (index>=0 && index<paths.n) return paths.e[index];
	return NULL;
}

/*! Return a new path that is the old path with offset applied.
 * Calling code is responsible for deleting the returned path.
 */
Path *PathsData::GetOffsetPath(int index)
{
	Path *path = GetPath(index);
	if (!path) return NULL;

	Path *opath = path->duplicate();
	if (opath->HasOffset()) opath->ApplyOffset();
	return opath;
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
	}
	return -1;
}

//! Sets SomeData::minx,etc, based on the union of all the paths.
void PathsData::FindBBox()
{
	DoubleBBox::ClearBBox();
	if (paths.n==0) return;

	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c] || !paths.e[c]->path) continue;
		paths.e[c]->FindBBox();
		addtobounds(paths.e[c]);
	}
}

//! Sets SomeData::minx,etc, based on the union of all the paths.
void PathsData::FindBBoxBase(DoubleBBox *ret)
{
	if (ret == nullptr) ret = this;
	ret->ClearBBox();
	if (paths.n==0) return;

	DoubleBBox b;
	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c] || !paths.e[c]->path) continue;
		b.ClearBBox();
		paths.e[c]->FindBBoxBase(&b);
		ret->addtobounds(&b);
	}
}

void PathsData::FindBBoxWithWidth(DoubleBBox *ret)
{
	if (ret == nullptr) ret = this;

	ret->ClearBBox();
	if (paths.n==0) return;

	DoubleBBox b;
	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c] || !paths.e[c]->path) continue;
		b.ClearBBox();
		paths.e[c]->FindBBoxWithWidth(&b);
		ret->addtobounds(&b);
	}
}

/*! Compute axis aligned bounding box of content transformed by transform.
 * Note this is more specific than the normal FindBBox().
 */
void PathsData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	if (paths.n==0) return;

	for (int c=0; c<paths.n; c++) {
		if (!paths.e[c] || !paths.e[c]->path) continue;
		paths.e[c]->ComputeAABB(transform, box);
	}
}

//! Find the point lying on subpath pathindex, at t.
/*! If tisdistance==0, then t is the usual t parameter. Else it is physical distance, starting from subpath->path.
 *
 * Return 1 for point found, otherwise 0.
 */
int PathsData::PointAlongPath(int pathindex, double t, int tisdistance, flatpoint *point, flatpoint *tangent, int resolution)
{
	if (pathindex<0 || pathindex>=paths.n) return 0;
	return paths.e[pathindex]->PointAlongPath(t,tisdistance,point,tangent, resolution);
}

//! Return the point on any of the paths closest to p.
/*!  point is assumed to already be in data coordinates.
 *
 * Optionally return that distance from the path in dist, the t parameter in tdist, and the path index in pathi.
 */
flatpoint PathsData::ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist, int *pathi, int resolution)
{
	double d=100000000,dalong=0,t=0, dto,dd,tt;
	flatpoint p,pp;
	int pi=0;
	for (int c=0; c<paths.n; c++) {
		pp=paths.e[c]->ClosestPoint(point, &dto,&dd,&tt, resolution);
		DBG cerr << " ************* scanning along path "<<c<<", d="<<dd<<"..."<<endl;
		//if (dd<d) {
		if (dto<d) {
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

int PathsData::FindExtrema(int pathindex, NumStack<flatpoint> *points_ret, NumStack<double> *t_ret)
{
	if (pathindex < 0 || pathindex >= paths.n) return 0;
	return paths.e[pathindex]->FindExtrema(points_ret, t_ret);
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
/*! \todo need to modify weight nodes for new scaling?
 */
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

/*! p is usually in range (0..1,0..1) with 0 being minimum and 1 being maximum.
 * This will transform all points and move the pathsdata origin so that the 
 * new pathsdata origin is at the original bbox point.
 */
void PathsData::SetOriginToBBoxPoint(flatpoint p)
{
	p = BBoxPoint(p.x, p.y, false);
	flatpoint pp = transformPoint(p);
	// flatpoint dparent = pp - origin();
	// flatpoint dlocal = p;

	Affine affine(*this);
	affine.origin(pp);
	MatchTransform(affine);
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

/*! Combine path objects, and optionally merge near endpoints.
 * If otherPath == this, then extract_paths_from_other is ignored, and duplicate paths from ourself are inserted but
 * still transformed by transform_from_other.
 *
 * If endpoint_merge_threshhold < 0, then simply append other paths to ourself. Otherwise,
 * connect any old endpoints that are near any new endpoints.
 *
 * if extract_paths_from_other, then transfer the subpath objects directly rather than creating duplicate subpaths.
 *
 * if !return_new, then append any new subpaths to *this, otherwise, don't modify *this and return a new object.
 */
PathsData *PathsData::MergeWith(PathsData *otherPath,
								double *transform_from_other,
								double endpoint_merge_threshhold,
								bool extract_paths_from_other,
								bool return_new)
{
	if (!otherPath) {
		DBG cerr << "Trying to PathsData::MergeWith( other path == null)!"<<endl;
		return nullptr;
	}

	if (otherPath == this) extract_paths_from_other = false;

	PathsData *path_ret = nullptr;
	if (return_new) {
		path_ret = dynamic_cast<PathsData*>(duplicate(nullptr));
	} else path_ret = this;

	//int cur_path_count = paths.n;
	for (int c=0; otherPath->paths.n; c++) {
		Path *path = nullptr;
		if (extract_paths_from_other) {
			otherPath->RemovePath(c, &path);
			c--;
		}
		else {
			path = otherPath->paths[c]->duplicate();
		}
		paths.push(path);
		if (transform_from_other) {
			Coordinate *p, *start;

			start = p = path->path;
			do {
				p->p(transform_point(transform_from_other,p->p()));
				p = p->next;
			} while (p && p!=start);
		}
	}

	if (endpoint_merge_threshhold >= 0) {
		cerr <<" **** TODO merge path combine"<<endl;
	}

	path_ret->FindBBox();
	return path_ret;
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
 *
 * If powerstroke, then take that as an Inkscape powerstroke definition, and
 * parse accordingly.
 */
PathsData *SvgToPathsData(PathsData *existingpath, const char *d,char **end_ptr, Laxkit::Attribute *powerstroke)
{
	Coordinate *coord=SvgToCoordinate(d,0,end_ptr,NULL);
	if (!coord) return NULL;

	PathsData *paths=existingpath;
	if (!paths) {
		paths=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
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

	if (powerstroke) {
		try {
			const char *str=powerstroke->findValue("is_visible");
			if (!str || strcmp(str,"true")) throw(1);

			//  <inkscape:path-effect
			//       effect="powerstroke"
			//       id="path-effect3338"
			//       is_visible="true"
			//       offset_points="0.060282486,35.326279 | 0.48001872,30.037074 | 1,1"
			//       sort_points="true"
			//       interpolator_type="Linear"
			//       interpolator_beta="0.2"
			//       start_linecap_type="round"
			//       linejoin_type="round"
			//       miter_limit="4"
			//       end_linecap_type="round" />

			str=powerstroke->findValue("offset_points");
			if (isblank(str)) throw(2);

			Path *path=paths->paths.e[paths->paths.n-1];
			char *str2=newstr(str);
			int n, nn;
			char **list=spliton(str2, '|', &n);
			double d[2];

			for (int c=0; c<n; c++) {
				nn=DoubleListAttribute(list[c], d, 2, NULL);
				if (nn!=2) continue;

				path->AddWeightNode(d[0], 0, d[1], 0);
			}
			
		} catch (int e) {
		}
	}

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
	id = nid;
	if (nid < 0) id = getUniqueNumber();
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
 *  \brief Edits PathsData objects. Also maintains all PathOperator classes.
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
 * \todo *** undo is very important for path editing!!
 * \todo *** must incorporate shift-move on grid lines, angles on 0/90/45/30/60/custom, etc.
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
PathInterface::PathInterface(int nid,Displayer *ndp, unsigned long nstyle) : anInterface(nid,ndp)
{
	primary = 1;

	show_weights        = true;  // show and allow edit weights.. see also PATHI_No_Weights
	show_points         = true;  // show and allow edit points and handles
	hide_other_controls = true; // show all control points, or just those on currently selected vertices
	show_baselines      = false;
	show_outline        = false;
	defaultweight.width = .01;

	pathi_style = nstyle;
	addmode     = ADDMODE_Bezier;
	editmode    = EDITMODE_AddPoints;
	colortofill = 0; //obsolete?
	addafter    = 1;
	widthstep   = 1.2;

	linestyle = defaultline = new LineStyle();
	linestyle->width        = .01;
	defaultline->inc_count();
	linestyle->Color(0xffff, 0, 0, 0xffff);

	fillstyle = defaultfill = new FillStyle();
	defaultfill->inc_count();
	fillstyle->fillstyle = FillNone;

	controlcolor = rgbcolor(0, 148, 178);  // defaults to blueish-white, change right after creation otherwise
	addcolor     = rgbcolor(100,255,100);
	constrain    = 3;
	select_radius2 = select_radius * select_radius;

	curvertex = NULL;
	curpath   = NULL;
	curpathop = NULL;
	curdirp   = NULL;

	child = owner = NULL;

	data = NULL;
	poc  = NULL;

	lbfound = NULL;
	lbselected = 1;  // whether the most recent left button down added a point to selection that was not previously selected
	drawhover     = 0;
	drawhoveri    = -1;
	drawpathi     = -1;
	hoverdevice   = 0;
	lasth         = 0;
	showdecs      = 1;
	verbose       = 1;
	show_addpoint = true;
	arrow_size    = 30;
	last_action   = 0;
	recent_action = 0;

	creationstyle = 0;

	needtodraw = 1;

	sc = NULL;
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
	return _("Path");
}

/*! Set various flags in pathi_style. Returns old state.
 *
 * \todo NOTE: currently no sanity checking done on flag..
 */
bool PathInterface::Setting(unsigned int flag, bool on)
{
	bool old=(pathi_style&flag) ? true : false;

	if (on) pathi_style|=flag;
	else pathi_style&=~flag;

	if (pathi_style&PATHI_No_Weights) show_weights=false;

	//needtodraw=1;
	return old;
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

/*! Return the number of vertices (not control points) selected.
 */
int PathInterface::VerticesSelected()
{
	int n=0;
	for (int c=0; c<curpoints.n; c++) {
		if (curpoints.e[c]->flags&POINT_VERTEX) n++;
	}
	return n;
}

//! Delete all points in curpoints.
/*! If the point is a vertex, this will remove any attached bezier handles.
 */
int PathInterface::DeleteCurpoints()
{
	for (int c=curpoints.n-1; c>=0; c--) { 
		if (pathi_style&PATHI_Two_Point_Minimum) {
			int path=data->hasCoord(curpoints.e[c]);
			if (path>=0) {
				if (data->paths.e[path]->NumVertices(NULL)<=2) continue;
			}
		}

		if (DeletePoint(curpoints.e[0])!=0) break; //this will remove from curpoints, break out of loop on fail
	}
	if (data) data->FindBBox();
	Modified(0);
	needtodraw=1;
	return 0;
}

//! Delete the point p, and make sure that path heads and weight nodes maintain their integrity.
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
	//int index=-1; //index of point in path, whole numbers are vertices
	//int pathi=data->hasCoord(p, &index);
	int pathi=data->hasCoord(p);
	if (pathi<0) return 1;
	path=data->paths.e[pathi];

	path->removePoint(p, false);
	if (path->path==NULL) { data->clear(pathi); path=NULL; }
	p=p->firstPoint(0);

	 //reassign curvertex if necessary
	if (curvertex && p->hasCoord(curvertex)) SetCurvertex(NULL);
	if (curvertex==NULL && path!=NULL) SetCurvertex(path->path);
	if (!curvertex && data->paths.n && data->paths.e[0]->path) SetCurvertex(data->paths.e[0]->path);

	 //remove any points being deleted from curvertex or curpoints
	int i;
	for (Coordinate *pp=p; pp; pp=pp->next) {
		i=curpoints.findindex(pp);
		if (i>=0) curpoints.pop(i);
	}

	delete p;

	drawhover=0;
	needtodraw=1;
	return 0;
}

/*! Cut segments in addafter direction near hoverpoint. 
 * If segment ends in an endpoint, then remove that endpoint.
 * If there is only one segment, then remove that subpath.
 * If segment is closed, then just open the segment.
 * If segment is open, then create new subpath at the cut.
 *  Return number of segments cut (0 or 1).
 */
int PathInterface::CutSegment()
{
	if (!data) return 0;

	curpoints.flush();

	double t;
	int pathi=-1;
	//flatpoint newpoint=data->ClosestPoint(hoverpoint, NULL,NULL,&t,&pathi);
	data->ClosestPoint(hoverpoint, NULL,NULL,&t,&pathi);

	Coordinate *v = data->GetCoordinate(pathi, t);
	if (!v) return 0;

	SetCurvertex(v, pathi);

	int n =0;
	// Coordinate *dangling = nullptr;
	// dangling = (addafter ? curpoints.e[c]->nextVertex(0) : curpoints.e[c]->previousVertex(0));
	data->CutSegment(curvertex, addafter, true);
	n++;
	//--------------
	// int n =0;
	// Coordinate *dangling = nullptr;
	// for (int c=0; curpoints.n; ) {
	// 	dangling = (addafter ? curpoints.e[c]->nextVertex(0) : curpoints.e[c]->previousVertex(0));
	// 	if (data->CutSegment(curpoints.e[c], addafter, true) == 2)
	// 		curpoints.remove(dangling);
	// 	n++;
	// }

	// nuke any resulting paths that only have one point
	if (data->paths.e[pathi]->NumVertices(nullptr) == 1) data->RemovePath(pathi, nullptr);
	if (pathi < data->paths.n && data->paths.e[pathi]->NumVertices(nullptr) == 1) data->RemovePath(pathi, nullptr);
	
	SetCurvertex(nullptr, -1);
	UpdateAddHint();
	Modified(0);
	needtodraw=1;
	return n;
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
		data->FindBBox();
		needtodraw=1;
		SetCurvertex(curvertex);
		return 0;
	}

	 //else is closed, and we must open it
	curpath->openAt(curvertex, addafter);
	data->FindBBox();
	SetCurvertex(curvertex);
	needtodraw=1;
	return 0;
}

/*! Install an orphan PathsData with an additional matrix.
 * Sets mode PATHI_Path_Is_MM_Real.
 */
int PathInterface::UseThisObject(PathsData *ndata, const double *extramatrix)
{
	if (data && data!=ndata) deletedata();
	if (data != ndata) {
		data = ndata;
		data->inc_count();
		UpdateViewportColor();
	}
	if (poc) delete poc;
	poc = nullptr;
	extram.m(extramatrix);

	pathi_style &= ~(PATHI_Path_Is_Screen | PATHI_Path_Is_M_Screen | PATHI_Path_Is_Real | PATHI_Path_Is_M_Real);
	pathi_style |= PATHI_Path_Is_MM_Real;
	needtodraw = 1;
	return 1;
}

int PathInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	PathsData *ndata = nullptr;
	if (dynamic_cast<SomeDataRef*>(oc->obj)) {
		SomeData *o = dynamic_cast<SomeDataRef*>(oc->obj)->GetFinalObject();
		ndata = dynamic_cast<PathsData *>(o);
		if (!ndata) return 0;
	}

	if (!ndata) ndata = dynamic_cast<PathsData *>(oc->obj);
	if (!ndata) return 0;

	if ((data && data != ndata) || (poc && poc->obj != oc->obj)) deletedata();
	if (poc) delete poc;
	poc = oc->duplicate();

	if (data != ndata) {
		data = ndata;
		data->inc_count();
	}

	if (linestyle) linestyle->dec_count();
	linestyle = data->linestyle;
	if (!linestyle) linestyle = defaultline;
	if (linestyle) linestyle->inc_count();

	if (fillstyle) fillstyle->dec_count();
	fillstyle = data->fillstyle;
	if (!fillstyle) fillstyle = defaultfill;
	if (fillstyle) fillstyle->inc_count();

	curvertex = NULL;
	curpath   = NULL;
	curpoints.flush();

	if (data->paths.n && data->paths.e[data->paths.n-1]->path) {
		SetCurvertex(data->paths.e[data->paths.n-1]->path->lastPoint(1));
	}

	UpdateViewportColor();
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
		if (mask & LINESTYLE_Color) {
			if (data && data->linestyle) data->linestyle->color=nlinestyle->color;
			else linestyle->color=nlinestyle->color;
		}
		if (mask & LINESTYLE_Color2) {
			if (data) data->fill(&nlinestyle->color);
		}
		if (mask & LINESTYLE_Width) {
			if (data && data->linestyle) data->linestyle->width=nlinestyle->width;
			else linestyle->width=nlinestyle->width;
		}
		needtodraw=1;
		return 1;

	} else if (dynamic_cast<PathsData *>(newdata)) {
		 //use a generic PathsData that is not tied to any context
		if (newdata==data) return 1; //already used!

		PathsData *ndata=dynamic_cast<PathsData *>(newdata);
		if (!ndata) return 0;

		deletedata(); //makes both data and poc NULL
		data=ndata;
		data->inc_count();

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
	int ntd = needtodraw,
		sd = showdecs,
		showadd = show_addpoint;
	unsigned long olddefer = pathi_style&PATHI_Defer_Render;
	pathi_style &= ~PATHI_Defer_Render;
	needtodraw = 1;
	showdecs = 0;
	show_addpoint = false;

	PathsData *d = data;
	data = dynamic_cast<PathsData *>(ndata);

	LineStyle *ls = linestyle;
	linestyle = dynamic_cast<LineStyle *>(a1);
	if (!linestyle) { linestyle=ls; ls=NULL; }

	FillStyle *fs=fillstyle;
	fillstyle=dynamic_cast<FillStyle *>(a2);
	if (!fillstyle) { fillstyle=fs; fs=NULL; }

	Refresh(); // pushes and pops m in Refresh..

	pathi_style|=olddefer;
	data = d;
	if (ls) linestyle=ls;
	if (fs) fillstyle=fs;
	showdecs = sd;
	needtodraw = ntd;
	show_addpoint = showadd;
	return 1; //*** should return 1 only if drawn?
}

//! Refresh draws the path and overlays the appropriate control points of the various PathOperator classes.
/*! \todo need mechanism to only draw what's necessary
 */
int PathInterface::Refresh()
{
	if (pathi_style&PATHI_Defer_Render) {
		 //this helps with some interfaces that use PathInterface as child interface, but
		 //need tighter control of render order than default ViewportWindow
		needtodraw=0;
		return 0;
	}

	if (!dp || !needtodraw) return 0;
	if (!data || !data->paths.n) {
		if (needtodraw) needtodraw=0;
		return 1;
	}
	needtodraw=0;

	// if (!strcmp_safe(data->Id(), "PathsData18")) {
	// 	cout << "REFRESH"<<endl;
	// 	data->dump_out(stdout, 2, 0, nullptr);
	// }

	// ------------ draw data-----------------

	Coordinate *start,*p,*p2;
	//PathOperator *pathop=NULL;
	Path *pdata;
	flatpoint fp;
	LineStyle *lstyle=NULL;
	FillStyle *fstyle=NULL;

	if (pathi_style&PATHI_Path_Is_M_Real) {
		dp->PushAndNewTransform(data->m());

	} else if (pathi_style&PATHI_Path_Is_MM_Real) {
		dp->PushAndNewTransform(extram.m());
		dp->PushAndNewTransform(data->m());
	}

	int olddraw=dp->DrawImmediately(0);
	dp->DrawReal();

	 //
	 //determine whether we need to fill and/or stroke...
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
	bool hasstroke=lstyle ? (lstyle->width!=0 && lstyle->function!=LAXOP_None && lstyle->function!=LAXOP_Dest) : false;

	if (fillstyle && fillstyle!=data->fillstyle && fillstyle!=defaultfill) {
		 //this is overriding the default data fillstyle, usually because we have
		 //been called from DrawData()
		fstyle=fillstyle;
	} else {
		fstyle=data->fillstyle;//default for all data paths
		if (!fstyle) fstyle=defaultfill;   //default for interface
	}
	bool hasfill=(fstyle && fstyle->fillstyle != FillNone && fstyle->function != LAXOP_None);
	bool ignoreweights= (data->style&PathsData::PATHS_Ignore_Weights) 
				|| !(pathi_style&PATHI_Render_With_Cache);

	if (pathi_style&PATHI_Hide_Path) { hasfill=false; hasstroke=false; }


	 //set up the fill region, also used as path region for non-cache rendering....
	if (!ignoreweights) {
		 //this path is only used for fill. stroke path is constructed separately below
		if (hasfill) {
			 //use path->centercache for fill area, and later path->outlinecache for stroke
			for (int cc=0; cc<data->paths.n; cc++) {
				// position p to be the first point that is a vertex
				pdata=data->paths.e[cc];
				pdata->UpdateCache();
				dp->drawFormattedPoints(pdata->centercache.e, pdata->centercache.n, 1);
			}
		}

	} else if (hasfill || hasstroke) { //used for default system fill/stroke
		for (int cc=0; cc<data->paths.n; cc++) {
			// position p to be the first point that is a vertex
			pdata = data->paths.e[cc];
			if (!pdata->path) continue;
			p = pdata->path->firstPoint(1);
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
	} //if ignore path cache, and there's either fill or stroke


	 //
	 //now path is all built, just need to fill and/or stroke

	if (hasfill) {
		dp->FillAttributes(fstyle->fillstyle,fstyle->fillrule);
		dp->NewFG(&fstyle->color);

		dp->fill(hasstroke && ignoreweights ? 1 : 0); //fill and preserve path maybe
	}

	if (hasstroke) {
		dp->NewFG(&lstyle->color);
		// DBG  cerr <<"New line fg: "<<lstyle->color.red<<"  "<<lstyle->color.green<<"  "<<lstyle->color.blue<<"  "<<lstyle->color.alpha<<endl;

		if (!ignoreweights) {
			 //we need to rebuild path and fill the stroke, since it uses a non-standard outline
			dp->FillAttributes(FillSolid, LAXFILL_Nonzero);
			//dp->FillAttributes(FillSolid, LAXFILL_EvenOdd);
			for (int cc=0; cc<data->paths.n; cc++) {
				// position p to be the first point that is a vertex
				pdata=data->paths.e[cc];
				pdata->UpdateCache();
				dp->drawFormattedPoints(pdata->outlinecache.e, pdata->outlinecache.n, 1);
			}
			dp->fill(0);

		} else { //ordinary system stroke
			dp->LineAttributes(-1,
							   (lstyle->dotdash && lstyle->dotdash!=~0)?LineOnOffDash:LineSolid,
							   lstyle->capstyle,
							   lstyle->joinstyle);
			if (lstyle->widthtype==0) dp->LineWidthScreen(lstyle->width);
			else dp->LineWidth(lstyle->width);
			dp->stroke(0);
		}
	}
	 
	dp->DrawImmediately(1);

	double thin = ScreenLine();
	double BIGR = 5*thin;
	double SMALLR = 3*thin;

	 //show decorations
	if (showdecs) {
		if (show_outline) DrawOutlines();
		if (show_baselines) DrawBaselines();



		for (int cc=0; cc<data->paths.n; cc++) {
			// position p to be the first point that is a vertex
			pdata = data->paths.e[cc];
			if (!pdata->path) {
				DBG cerr <<"Degenerate path (shouldn't happen!)"<<endl;
				continue;
			}
			p = pdata->path->firstPoint(1);
			start = p;

			if (!(p->flags&POINT_VERTEX)) { // is degenerate path: no vertices
				DBG cerr <<"Degenerate path (shouldn't happen!)"<<endl;
				continue;
			}

			dp->NewFG(controlcolor);

			 //draw corners just outside path bounding box
			dp->LineWidthScreen(thin);
			double o=thin * 5/dp->Getmag(), //5 pixels outside, 15 pixels long
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

			// draw the points
			if (show_points) {
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
						bool on = (curpoints.findindex(p) >= 0);
						bool con = IsNearSelected(p);
						flatpoint sp=dp->realtoscreen(p->p());
						dp->DrawScreen();
						dp->LineWidthScreen(1);

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
								case BEZ_STIFF_EQUAL:   dp->drawthing(sp.x,sp.y,BIGR,BIGR,on,THING_Circle);      break;
								case BEZ_STIFF_NEQUAL:  dp->drawthing(sp.x,sp.y,BIGR,BIGR,on,THING_Diamond);     break;
								case BEZ_NSTIFF_NEQUAL: dp->drawthing(sp.x,sp.y,BIGR,BIGR,on,THING_Square);      break;
								case BEZ_NSTIFF_EQUAL:  dp->drawthing(sp.x,sp.y,BIGR,BIGR,on,THING_Triangle_Up); break;
								default: dp->drawthing(sp.x,sp.y,BIGR,BIGR,on,THING_Circle); break;
							}

						} else if (p->flags&POINT_TONEXT) {
							bool doit = !hide_other_controls || con;
							if (doit) {
								dp->drawthing(sp.x,sp.y,SMALLR,SMALLR,on,THING_Circle);
								dp->drawline(sp,dp->realtoscreen(p->next->p()));
							}

						} else if (p->flags&POINT_TOPREV) {
							bool doit = !hide_other_controls || con;
							if (doit) {
								dp->drawthing((int)sp.x,(int)sp.y,SMALLR,SMALLR,on,THING_Circle);
								dp->drawline(sp,dp->realtoscreen(p->prev->p()));
							}
						}
						dp->DrawReal();
					}

					p2=p;
					p=p->next;
				} while (p && p!=start);
			} //if show_points

			 // draw little arrow on curvertex in the next direction (indicates where new points go)
			//if (curvertex || drawhover==HOVER_Direction) {
			if (curvertex || curdirp) {
				flatpoint p, p2;
				flatpoint v;
				if (curvertex) p=curvertex->p(); else p=curdirp->p();
				dp->DrawReal();

				v=curdirv;
				p=curdirp->p();
				p2=p+v;

				p =dp->realtoscreen(p);
				p2=dp->realtoscreen(p2);
				dp->DrawScreen();

				if (drawhover==HOVER_Direction) dp->LineWidthScreen(8*thin);
				else dp->LineWidthScreen(5*thin);
				dp->NewFG(1.,1.,1.);
				dp->drawarrow(p,p2-p, (addafter ? 1 : -1) * arrow_size/2*thin, arrow_size*thin, 0, 3, true);

				if (drawhover==HOVER_Direction) dp->LineWidthScreen(5*thin);
				else dp->LineWidthScreen(3*thin);
				if (editmode==EDITMODE_AddPoints) {
					if (addafter) dp->NewFG(0.,.75,0.);
					else dp->NewFG(1.,0.,0.);
				} else dp->NewFG(controlcolor);

				dp->drawarrow(p,p2-p, (addafter ? 1 : -1) * arrow_size/2*thin, arrow_size*thin, 0, 3, true);

				dp->DrawReal();

			}

			 //draw weights
			if (show_weights) {
				flatpoint ppo, pp,vv,vt,ppt,ptop,pbottom;
				PathWeightNode *weight=&defaultweight;
				int highlight=0;

				for (int cw=(pdata->pathweights.n>0 ? 0 : -1); cw<pdata->pathweights.n; cw++) {
					if (cw==-1) weight=&defaultweight; else weight=pdata->pathweights.e[cw];

					if (drawpathi==cc && drawhoveri==cw) {
					  if (     drawhover==HOVER_Weight
							|| drawhover==HOVER_WeightPosition
							|| drawhover==HOVER_WeightOffset
							|| drawhover==HOVER_WeightTop
							|| drawhover==HOVER_WeightBottom
							|| drawhover==HOVER_WeightAngle)
						highlight=2;
					  else if (drawhover==HOVER_RemoveWeightNode)
						highlight=-1;
					} else highlight=0;

					//if (pdata->PointAlongPath(weight->t, 0, &pp, &vv)==0) continue; 
					//drawWeightNode(pp,vv, weight->topOffset(),weight->bottomOffset(), highlight, weight->angle,pdata->absoluteangle);
					drawWeightNode(pdata, weight, highlight);
				}
			}
		} // loop cc over paths

		if (drawhover==HOVER_DirectionSelect) {
			drawNewPathIndicator(curdirp->p(),curdirp->info);
		}

		if (show_addpoint && curvertex && (drawhover == HOVER_None || drawhover == HOVER_Endpoint) && !buttondown.any()) {
			dp->NewFG(addcolor);
			dp->LineWidthScreen(ScreenLine());
			dp->moveto(curvertex->p());
			dp->curveto(add_point_hint[0], add_point_hint[1], add_point_hint[2]);
			if (show_addpoint > 1) {
				dp->curveto(add_point_hint[3], add_point_hint[4], add_point_hint[5]);
			}
			dp->stroke(0);
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
	if (drawhover == HOVER_AddPointOn) {
		dp->NewFG(0.,.75,0.);
		dp->LineWidthScreen(2);
		dp->drawpoint(hoverpoint, 3,0);

	} else if (drawhover == HOVER_CutSegment) {
		flatpoint sp = dp->realtoscreen(hoverpoint);
		dp->DrawScreen();
		double thin = ScreenLine();
		dp->NewFG(1.,1.,1.);
		dp->LineWidthScreen(thin*5);
		dp->drawthing(sp, thin*20, thin*20, 0, THING_X);
		dp->LineWidthScreen(thin*3);
		dp->NewFG(1.,0.,0.);
		dp->drawthing(sp, thin*20, thin*20, 0, THING_X);
		dp->DrawReal();

	} else if (drawhover==HOVER_AddWeightNode) {
		int hpathi=-1;
		double t;
		data->ClosestPoint(hoverpoint, NULL,NULL,&t,&hpathi);
		if (hpathi>=0) {
			PathWeightNode node(t, defaultweight.offset, defaultweight.width, defaultweight.angle);
			node.angle=defaultweight.angle;
			drawWeightNode(data->paths.e[drawpathi], &node, 0);
		}

	} else if (drawhover==HOVER_Endpoint || drawhover==HOVER_MergeEndpoints) {
		dp->NewFG(0.,.75,0.);
		dp->LineWidthScreen(2);
		dp->drawpoint(hoverpoint, 7,1);

	} else if (drawhover==HOVER_Vertex || drawhover==HOVER_Point || drawhover==HOVER_Handle) {
		dp->NewFG(controlcolor);
		int r= (drawhover==HOVER_Handle ? SMALLR : BIGR);

		dp->DrawScreen();
		dp->LineWidthScreen(2);
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
		dp->DrawScreen();
		dp->LineWidthScreen(1);
		dp->drawline(x1,y1, x2,y1);
		dp->drawline(x2,y1, x2,y2);
		dp->drawline(x2,y2, x1,y2);
		dp->drawline(x1,y2, x1,y1);
		dp->DrawReal();
	}


	if (pathi_style&PATHI_Path_Is_M_Real) {
		dp->PopAxes();
	} else if (pathi_style&PATHI_Path_Is_MM_Real) {
		dp->PopAxes();
		dp->PopAxes();
	}

	dp->DrawImmediately(olddraw);

	return 0;
}

/*! For each explicitly weighted line, draw outline for debugging purposes.
 */
void PathInterface::DrawOutlines()
{
	dp->LineWidthScreen(2);
	dp->FillAttributes(FillSolid,EvenOddRule);

	Path *path;
	for (int cc=0; cc<data->paths.n; cc++) {
		path=data->paths.e[cc];
		//if (!path->Weighted()) continue;

		if (path->needtorecache) path->UpdateCache();

		dp->NewFG(.8,0.,.8);
		dp->NewBG(0.,0.,.8);

		//dp->drawbez(path->outlinecache.e, path->outlinecache.n, 1, 2);
		dp->drawFormattedPoints(path->outlinecache.e, path->outlinecache.n, 2);
	}
}

void PathInterface::DrawBaselines()
{
	Path *pdata;
	Coordinate *p, *p2, *start;

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
					if (!p2) break;
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

	dp->NewFG(1.,1.,1.);
	dp->LineWidthScreen(2);
	dp->stroke(1);
	dp->NewFG(1.,0.,0.);
	dp->LineWidthScreen(1);
	dp->stroke(0);


	 //draw offset path
	for (int cc=0; cc<data->paths.n; cc++) {
		// position p to be the first point that is a vertex
		pdata=data->paths.e[cc];

		dp->NewFG(1.,1.,1.);
		dp->LineWidthScreen(2);
		dp->drawFormattedPoints(pdata->centercache.e, pdata->centercache.n, 0);
		dp->NewFG(0.,0.,1.);
		dp->drawFormattedPoints(pdata->centercache.e, pdata->centercache.n, 0);
	}

}


/*! isfornew==1 means draw whole thing green, as for adding.
 * isfornew==-1 means draw whole thing red, as for removing.
 * isfornew==2 means go by drawhover for what is to be emphasized.
 *
 * pp and vv are in data space, and it is assumed dp->realtoscreen() incorporates data->m().
 */
void PathInterface::drawWeightNode(Path *path, PathWeightNode *weight, int isfornew)
{
	double thin = ScreenLine();
	double arc = select_radius*2*thin;

	//double wtop   =weight->topOffset();
	//double wbottom=weight->bottomOffset();
	//double angle  =weight->angle;
	//bool absoluteangle=path->absoluteangle;

	flatpoint pp,po, ptop,pbottom, vv,vt;
	if (WeightNodePosition(path, weight, &pp,&po, &ptop,&pbottom, &vv,&vt, 2)!=0) return;

	dp->NewFG(controlcolor);
	dp->DrawScreen();

	if (isfornew==1) {
		 //adding
		dp->LineWidthScreen(3*thin);
		dp->NewFG(0.,.75,0.);
	} else if (isfornew==-1) {
		 //removing 
		dp->LineWidthScreen(3*thin);
		dp->NewFG(.75,0.,0.);
	} else dp->LineWidthScreen(1*thin);


	 //draw main arrow heads
	if (isfornew==2) {
		if (drawhover==HOVER_WeightTop)
			 dp->LineWidthScreen(3*thin);
		else dp->LineWidthScreen(1*thin);
	}
	dp->drawline(ptop + arc/3*(-vv+vt), ptop);
	dp->drawline(ptop, ptop + arc/3*(vv+vt));

	if (isfornew==2) {
		if (drawhover==HOVER_WeightBottom)
			dp->LineWidthScreen(3*thin);
		else dp->LineWidthScreen(1*thin);
	}
	dp->drawline(pbottom + arc/3*(-vv-vt), pbottom);
	dp->drawline(pbottom               , pbottom+arc/3*(vv-vt));

	 //draw curve connecting the arrow heads
	if (isfornew==2) {
		if (drawhover == HOVER_WeightPosition)
			 dp->LineWidthScreen(3*thin);
		else dp->LineWidthScreen(1*thin);
	}
	dp->moveto(ptop);
	dp->curveto(ptop + 1.33*arc*vt,
				ptop + 1.33*arc*vt - arc*vv,
				ptop - arc*vv);
	dp->lineto(pbottom  - arc*vv);
	dp->curveto(pbottom - 1.33*arc*vt - arc*vv,
				pbottom - 1.33*arc*vt,
				pbottom);
	dp->stroke(0);

	 //draw angle arrows
	if (isfornew == 2) {
		if (drawhover == HOVER_WeightAngle) {
			dp->LineWidthScreen(3 * thin);
		} else dp->LineWidthScreen(1 * thin);
	}
	dp->moveto(ptop + 1.4*arc*vt + .3*arc*vv);
	dp->lineto(ptop +     arc*vt);
	dp->curveto(ptop+2.33*arc*vt, ptop + 2.33*arc*vt - arc*vv, ptop + arc*vt - arc*vv);
	dp->lineto(ptop + 1.4*arc*vt - arc*vv - .3*arc*vv);
	dp->stroke(0);

	//draw offset arrows
	if (!(pathi_style & PATHI_No_Offset)) {
		if (isfornew == 2) {
			if (drawhover == HOVER_WeightOffset) {
				dp->LineWidthScreen(3 * thin);
			} else {
				dp->LineWidthScreen(1 * thin);
			}
		}
		pp = pbottom - arc/2 * vv - 1.5 * arc * vt;
		po = pbottom - arc/2 * vv - 3.0 * arc * vt;
		dp->moveto(pp);
		dp->lineto(po);

		double sz = .25;
		dp->moveto(pp - sz * arc * vv - sz * arc * vt);
		dp->lineto(pp);
		dp->lineto(pp + sz * arc * vv - sz * arc * vt);

		dp->moveto(po - sz * arc * vv + sz * arc * vt);
		dp->lineto(po);
		dp->lineto(po + sz * arc * vv + sz * arc * vt);
		dp->stroke(0);
	}

	dp->DrawReal();
}

/*! Draw the pop up circular menu for new path or new subpath, centered on p. which==1 is top, which==2 is bottom.
 */
void PathInterface::drawNewPathIndicator(flatpoint p,int which)
{
	dp->DrawScreen();
	p=dp->realtoscreen(p);

	double radius = dir_select_radius*ScreenLine();
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
	if (p == NULL) { curdirp=NULL; curpath=NULL ;  curvertex=NULL; return; }

	int i = path;
	if (i < 0) i = data->hasCoord(p);
	if (i >= 0) curpath = data->paths.e[i];

	 //make sure p is on nearest vertex
	if (!(p->flags & POINT_VERTEX)) {
		if (p->flags & POINT_TOPREV) {
			while (p->prev && !(p->flags&POINT_VERTEX)) p = p->prev;
		} else while (p && p->flags&POINT_TONEXT) p = p->next;
	}

	curvertex = p;
	curdirp = curvertex;
	if (addafter) curdirv = curvertex->direction(1); //uses "visual" tangent
	else          curdirv = curvertex->direction(0);

	UpdateAddHint();
}

void PathInterface::UpdateAddHint()
{
	if (!data || !curvertex) return;
	
	flatpoint p1,c1,c2,p2;
	if (curvertex->resolveToControls(p1,c1,c2,p2, addafter) == 0) {
		show_addpoint = 1;
		if (addafter && curvertex->next && (curvertex->next->flags & POINT_TOPREV))
			add_point_hint[0] = curvertex->next->p();
		else if (!addafter && curvertex->prev && (curvertex->prev->flags & POINT_TONEXT))
			add_point_hint[0] = curvertex->prev->p();
		else add_point_hint[0] = curvertex->p();
	} else {
		show_addpoint = 2;
		add_point_hint[0] = c1;
		add_point_hint[4] = c2;
		add_point_hint[5] = p2;
	}
}

//! Make curdirp be valid for curvertex.
void PathInterface::UpdateDir()
{
	curdirp = curvertex;
	if (addafter) curdirv = curvertex->direction(1); //uses "visual" tangent
	else          curdirv = curvertex->direction(0);

	UpdateAddHint();
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

const double *PathInterface::datam()
{
	if (poc && poc->obj != data) return poc->obj->m();
	if (!data) return nullptr;
	return data->m();
}

//! This is supposed to return points from data context space to screen space.
/*! Redefined from anInterface to catch PATHI_Path_Is_M_Real and related styles.
 */
flatpoint PathInterface::realtoscreen(flatpoint r)
{
	if (pathi_style&PATHI_Path_Is_M_Real) {
		//if (viewport) return viewport->realtoscreen(r);
		return dp->realtoscreen(r);

	} else if (pathi_style&PATHI_Path_Is_MM_Real) {
		return dp->realtoscreen(extram.transformPoint(r));

	} else if (pathi_style&PATHI_Path_Is_Real) {//***might not work
		return dp->realtoscreen(r);

	} else if (pathi_style&PATHI_Path_Is_M_Screen) {//***might not work
		return transform_point(datam(),r);

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

	} else if (pathi_style&PATHI_Path_Is_MM_Real) {
		return extram.transformPointInverse(dp->screentoreal(x,y));

	} else if (pathi_style&PATHI_Path_Is_Real) {
		return dp->screentoreal(x,y);//***might not work

	} else if (pathi_style&PATHI_Path_Is_M_Screen) {
		return transform_point_inverse(datam(),flatpoint(x,y));//***might not work

	} else if (pathi_style&PATHI_Path_Is_Screen) {
		return flatpoint(x,y);//***might not work
	}

	return anInterface::screentoreal(x,y);
}

/*! Return whether coord is part of a v-toprev-tonext-vertex-toprev-tonext-v cluster, any of which is currently seleceted.
 */
bool PathInterface::IsNearSelected(Coordinate *coord)
{
	if (curpoints.findindex(coord) >= 0) return true;

	Coordinate *prev = coord, *next = coord;
	if (prev->flags & POINT_TONEXT) {
		while (prev->prev && (prev->flags & (POINT_TONEXT | POINT_TOPREV))) prev = prev->prev;
		while (next->next && (next->flags & (POINT_TONEXT | POINT_TOPREV))) next = next->next;
		if (next->next && (next->next->flags & POINT_TOPREV)) next = next->next;

	} else if (prev->flags & POINT_TOPREV) {
		prev = prev->prev; //should now be on vertex
		if (prev->prev && prev->prev->flags & POINT_TONEXT) prev = prev->prev;
		while (next->next && (next->flags & (POINT_TONEXT | POINT_TOPREV))) next = next->next;
	}

	while (1) {
		if (curpoints.findindex(prev) >= 0) return true;
		if (prev == next) break;
		prev = prev->next;
	}

	return false;
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

/*! Return 1 if node not on path.
 * Return 0 for success.
 *
 * needtotransform==1 means transform with realtoscreen().
 * needtotransform==2 means transform with dp->realtoscreen().
 *
 * vv_ret and vt_ret are normalized.
 */
int PathInterface::WeightNodePosition(Path *path, PathWeightNode *weight,
									  flatpoint *pp_ret, flatpoint *po_ret, flatpoint *ptop_ret, flatpoint *pbottom_ret,
									  flatpoint *vv_ret, flatpoint *vt_ret,
									  int needtotransform)
{
	flatpoint pp,po, vv,vt, ptop,pbottom;

	if (path->PointAlongPath(weight->t, 0, &pp, &vv)==0) return 1;

	if (vv.isZero()) path->PointAlongPath(weight->t+.00001, 0, NULL,&vv);
	vv.normalize();
	vt=transpose(vv);
	po=pp+vt*weight->offset;
	if (path->absoluteangle) vv=rotate(flatpoint(1,0), weight->angle);
	else if (weight->angle!=0) vv=rotate(vv, weight->angle);
	vt=transpose(vv);

	ptop=po+vt*weight->width/2;
	if (needtotransform==1)      ptop =     realtoscreen(transform_point(data->m(), ptop));
	else if (needtotransform==2) ptop = dp->realtoscreen(ptop);
	else ptop=realtoscreen(ptop);

	pbottom=po-vt*weight->width/2;
	if (needtotransform==1)      pbottom =     realtoscreen(transform_point(data->m(), pbottom));
	else if (needtotransform==2) pbottom = dp->realtoscreen(pbottom);
	else pbottom=realtoscreen(pbottom);

	if (needtotransform==1) 
		vv =realtoscreen(transform_point(datam(),po+vv)) - realtoscreen(transform_point(datam(),po));
	else if (needtotransform==2) 
		vv =dp->realtoscreen(po+vv) - dp->realtoscreen(po);
	else vv=realtoscreen(po+vv)-realtoscreen(po);

	vv.normalize();
	vt=ptop-pbottom;
	vt.normalize();

	*pp_ret=pp;
	*po_ret=po;
	*ptop_ret=ptop;
	*pbottom_ret=pbottom;
	*vv_ret=vv;
	*vt_ret=vt;

	return 0;
}

/*! Each weight node has arrows that point to the position in the path, at a distance
 * of the line top and bottom. Dragging the bar off to the side repositions. Dragging
 * either the top or bottom moves one or both, depending on state.
 */
int PathInterface::scanWeights(int x,int y,unsigned int state, int *pathindex, int *index)
{
	 //   /--\  <- angle thing
	 //   /--\  <- this is (arc) wide
	 //   |  v
	 //   |
	 // --|--pp--- vv ->
	 //   |        vt
	 //   |  ^      |
	 //   \--/      v
	 //    ^
	 //    |
	 //    v

	if (!show_weights || !data) return HOVER_None;

	double arc=select_radius*2*ScreenLine();
	double yyt,yyb,xx;
	flatpoint fp(x,y);
	flatpoint sp;
	PathWeightNode *weight;
	Path *path;
	flatpoint pp,po,vv, vt, ptop,pbottom;

	for (int c=0; c<data->paths.n; c++) {
		path=data->paths.e[c];

		for (int cw=(path->pathweights.n>0 ? 0 : -1); cw<path->pathweights.n; cw++) {
			if (cw==-1) weight=&defaultweight; else weight=path->pathweights.e[cw];

			if (WeightNodePosition(path, weight, &pp,&po, &ptop,&pbottom, &vv,&vt, 1)!=0) continue;

			xx=(fp-ptop)*vv;
			if (xx<-arc*1.5 || xx>arc) continue;
			yyt=(fp-ptop)*vt;
			yyb=(fp-pbottom)*vt;

			DBG cerr <<"xx: "<<xx<<"  yt: "<<yyt<<"  yyb:"<<yyb<<"  arc:"<<arc<<endl;

			if (xx<-arc/2 && yyt<arc && yyb>-arc) {
				*index=cw;
				*pathindex=c;
				return HOVER_WeightPosition;
			}
			if (yyt>=arc && yyt<2*arc) {
				*index=cw;
				*pathindex=c;
				return HOVER_WeightAngle;
			}
			if (yyt>=0 && yyt<arc) {
				*index=cw;
				*pathindex=c;
				return HOVER_WeightTop;
			}
			if (yyb<=0 && yyb>-arc) {
				*index=cw;
				*pathindex=c;
				return HOVER_WeightBottom;
			}
			if (yyb <= -arc && yyb >-3*arc) {
				*index = cw;
				*pathindex = c;
				return HOVER_WeightOffset;
			}
		}
	}


	return HOVER_None;
}

//! Scan for various hovering things.
/*! Such as a cutpoint, a segment to cut, a segment to join, or the direction arrow.
 */
int PathInterface::scanHover(int x,int y,unsigned int state, int *pathi)
{
	if (!data) return HOVER_None;

	flatpoint fp = transform_point_inverse(datam(),screentoreal(x,y));

	if (      ((pathi_style&PATHI_Plain_Click_Add) && (state&LAX_STATE_MASK)==0)
		  || (!(pathi_style&PATHI_Plain_Click_Add) &&
		  	 ((state&LAX_STATE_MASK) == ControlMask || (state&LAX_STATE_MASK) == ShiftMask))) {
		// scan for closest point to cut
		int dist2;
		int hpathi = -1;
		hoverpoint = data->ClosestPoint(fp, NULL, NULL, NULL, &hpathi);
		dist2      = norm2(realtoscreen(transform_point(data->m(), hoverpoint)) - flatpoint(x, y));
		if (dist2 < select_radius2*ScreenLine()) {
			*pathi = hpathi;
			if ((state&LAX_STATE_MASK)==ControlMask)
				return HOVER_AddPointOn;
			return HOVER_CutSegment;
		}
		return HOVER_None;

	} else if (!(pathi_style&PATHI_No_Weights) &&
 			(state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
		 //hover over segment to add a weight node
		int dist2;
		double t;
		int hpathi=-1;
		hoverpoint=data->ClosestPoint(fp, NULL,NULL,&t,&hpathi);

		if (hpathi>=0 && (pathi_style&PATHI_Single_Weight) && data->paths.e[hpathi]->pathweights.n==1)
			return HOVER_None;

		dist2=norm2(realtoscreen(transform_point(datam(), hoverpoint))-flatpoint(x,y));
		DBG cerr << " ************* scanned along path "<<hpathi<<", d="<<sqrt(dist2)<<"..."<<endl;
		if (dist2<select_radius2*2*ScreenLine()) {
			DBG cerr << " ************* scanned and found add weight node..."<<endl;
			//virtual int PointAlongPath(double t, int tisdistance, flatpoint *point, flatpoint *tangent);
			data->PointAlongPath(hpathi, t, 0, NULL,&hoverdir);
			*pathi=hpathi;
			return HOVER_AddWeightNode;
		}
		return HOVER_None;

	} else if (curvertex) {
		// scan for direction arrow in rect aligned with curve arrow_size/2 off point, arrow_size long
		flatpoint p = realtoscreen(transform_point(datam(), curdirp->p()));
		flatpoint v = realtoscreen(transform_point(datam(), curdirp->p()+curdirv))-p;
		if (!addafter) v *= -1;
		v /= norm(v);
		p += (transpose(v) - v)*arrow_size/2*ScreenLine();

		double dist = distance(flatpoint(x,y), p, p + arrow_size*v*ScreenLine());
		if (dist < select_radius*ScreenLine() || dist < arrow_size/3*ScreenLine()) return HOVER_Direction;
	}

	return HOVER_None;
}

//! Scan only against endpoints
Coordinate *PathInterface::scanEndpoints(int x,int y,int *pathindex,Coordinate *exclude, bool *is_first)
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

		if (s!=exclude && norm2(realtoscreen(transform_point(datam(), s->p()))-fp)<select_radius2*ScreenLine()) p = s;
		if (e!=exclude && norm2(realtoscreen(transform_point(datam(), e->p()))-fp)<select_radius2*ScreenLine()) p = e;

		DBG cerr<<"endpoint s:"<<norm2(realtoscreen(transform_point(datam(), s->p()))-fp)<<endl;
		DBG cerr<<"endpoint e:"<<norm2(realtoscreen(transform_point(datam(), e->p()))-fp)<<endl;

		if (p==exclude) p=NULL;
		if (p) {
			if (pathindex) *pathindex = c;
			if (is_first) {
				*is_first = (p == s);
			}
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
	for (int c=curpoints.n-1; c >= 0; c--) {
		cp = curpoints.e[c];
		if (pmask && cp->iid != pmask) continue;
		p = realtoscreen(transform_point(datam(),cp->p())) - flatpoint(x,y);
		if (p.norm() < NearThreshhold()) return cp;
	}

	if (!show_points) return nullptr;

	// scan for vertex points
	PathOperator *op=NULL;
	for (c=0; c<data->paths.n; c++) {
		start=cp=data->paths.e[c]->path;
		if (!start) continue;

		if (cp) {
			cp = cp->firstPoint(1);
			start = cp;
			do {
				if (cp->controls && cp->controls->iid() == pmask) {
					op = getPathOpFromId(cp->controls->iid());
					if (op && op->scan(dp,cp->controls,x,y)) {
						if (pathindex) *pathindex = c;
						return cp;
					}

				} else if (cp->flags&POINT_VERTEX && !pmask) {
					p = realtoscreen(transform_point(datam(),cp->p()));
					if ((p.x-x)*(p.x-x)+(p.y-y)*(p.y-y)<select_radius2*ScreenLine()) {
						DBG cerr <<" path scan found vertex on "<<c<<endl;
						if (pathindex) *pathindex=c;
						return cp;
					}
				}

				cp = cp->nextVertex();
			} while (cp && cp!=start);

		}
	}

	DBG cerr <<"path scan pmask: "<<pmask<<endl;
	if (pmask) return NULL; //don't scan for controls when there is an active pathop

	// scan for control points of curpoints
	if (show_points == 1) {

	}

	// scan for control points
	// if (show_points == 2) {}
	for (c=0; c<data->paths.n; c++) { //only search for normal control points, controlled was checked above
		start = cp = data->paths.e[c]->path; //*** doesn't check that paths.e[c] exists, should be ok, though
		//DBG cerr <<" path scan for controls on 1. path "<<c<<"/"<<data->paths.n<<endl;
		if (cp) {
			//DBG cerr <<" path scan for controls on path "<<c<<endl;
			cp = cp->firstPoint(0);
			start = cp;
			do {
				if (!cp->controls && !(cp->flags & POINT_VERTEX) ) {
					if (hide_other_controls && !IsNearSelected(cp)) {
						cp = cp->next;
						continue;
					}

					p = realtoscreen(transform_point(datam(),cp->p()));
					if ((p.x-x)*(p.x-x)+(p.y-y)*(p.y-y) < select_radius2*ScreenLine()) {
						//DBG cerr <<" path scan found control on "<<c<<endl;
						if (pathindex) *pathindex = c;
						return cp;
					}
				}
				cp = cp->next;
			} while (cp && cp != start);
		}
		//DBG cerr <<" path scan for controls on 2. path "<<c<<"/"<<data->paths.n<<endl;
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
Laxkit::MenuInfo *PathInterface::ContextMenu(int x,int y,int deviceid, MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo();


	if (curpoints.n) {
		unsigned int ptype = curpoints.e[0]->flags & BEZ_MASK;
		if (ptype == 0) ptype = BEZ_STIFF_EQUAL;
		for (int c=1; c<curpoints.n; c++) if ((curpoints.e[c]->flags&BEZ_MASK)!=ptype) { ptype=-1; break; }
		menu->AddSep(_("Point type"));
		menu->AddToggleItem(_("Smooth"),          PATHIA_PointTypeSmooth,        0, (ptype==BEZ_STIFF_EQUAL));
		menu->AddToggleItem(_("Smooth, unequal"), PATHIA_PointTypeSmoothUnequal, 0, (ptype==BEZ_STIFF_NEQUAL));
		menu->AddToggleItem(_("Corner"),          PATHIA_PointTypeCorner,        0, (ptype==BEZ_NSTIFF_NEQUAL));
	}                                                                                   

	menu->AddSep(_("Join Style"));
	int jstyle = curpath && curpath->linestyle ? curpath->linestyle->joinstyle : -1;
	if (jstyle<0 && data && data->linestyle) jstyle = data->linestyle->joinstyle;
	menu->AddToggleItem(_("Bevel"),       PATHIA_Bevel,       0, (jstyle==LAXJOIN_Bevel));
	menu->AddToggleItem(_("Miter"),       PATHIA_Miter,       0, (jstyle==LAXJOIN_Miter));
	menu->AddToggleItem(_("Round"),       PATHIA_Round,       0, (jstyle==LAXJOIN_Round));
	menu->AddToggleItem(_("Extrapolate"), PATHIA_Extrapolate, 0, (jstyle==LAXJOIN_Extrapolate));

	menu->AddSep(_("Cap Style"));
	int cstyle = curpath && curpath->linestyle ? curpath->linestyle->capstyle : -1;
	if (cstyle<0 && data && data->linestyle) cstyle =data->linestyle->capstyle;
	if (cstyle<0) cstyle = defaultline->capstyle;
	int ecstyle = curpath && curpath->linestyle ? curpath->linestyle->endcapstyle : -1;
	if (ecstyle<0 && data && data->linestyle) ecstyle = data->linestyle->endcapstyle;
	if (ecstyle<0) ecstyle = defaultline->endcapstyle;
	menu->AddToggleItem(_("Butt"),        PATHIA_CapButt,  0, (cstyle==LAXCAP_Butt));
	menu->AddToggleItem(_("Round"),       PATHIA_CapRound, 0, (cstyle==LAXCAP_Round));
	menu->AddToggleItem(_("Zero tips"),   PATHIA_CapZero,  0, (cstyle==LAXCAP_Zero_Width));
	menu->AddItem(_("End caps"),   0);
	menu->SubMenu(_("End caps"));
		menu->AddToggleItem(_("Same as start"),PATHIA_EndCapSame,  0, (ecstyle==0));
		menu->AddToggleItem(_("Butt"),         PATHIA_EndCapButt,  0, (ecstyle==LAXCAP_Butt));
		menu->AddToggleItem(_("Round"),        PATHIA_EndCapRound, 0, (ecstyle==LAXCAP_Round));
		menu->AddToggleItem(_("Zero tips"),    PATHIA_EndCapZero,  0, (ecstyle==LAXCAP_Zero_Width));
	menu->EndSubMenu();

	if (!(pathi_style&PATHI_One_Path_Only)) {
		if (menu->n()) menu->AddSep();
		menu->AddItem(_("Start new subpath"),PATHIA_StartNewSubpath);
		menu->AddItem(_("Start new path object"),PATHIA_StartNewPath);
	}

	//if (***multiple paths available) menu->AddItem(_("Separate with holes"),PATHIA_BreakApart);
	//if (***multiple paths available) menu->AddItem(_("Separate all"),PATHIA_BreakApart);
	//if (***multiple path objects) menu->AddItem(_("Combine"),PATHIA_Combine);
	// *** need mechanism to insert custom path actions

	// misc decoration showing
	if (menu->n()) menu->AddSep();

	menu->AddToggleItem(_("Show points"),  PATHIA_ToggleShowPoints, 0, show_points);
	menu->AddToggleItem(_("Hide other controls"),  PATHIA_ToggleHideControls, 0, hide_other_controls);

	if (!(pathi_style&PATHI_No_Weights)) {
		menu->AddToggleItem(_("Show base and center lines"), PATHIA_ToggleBaseline, 0, show_baselines);
		menu->AddToggleItem(_("Show weight nodes"), PATHIA_ToggleWeights, 0, show_weights);

		bool angled=false;
		if (curpath) angled = curpath->absoluteangle;
		else if (data && data->paths.n) angled = data->paths.e[0]->absoluteangle;
		menu->AddToggleItem(_("Absolute angles"),  PATHIA_ToggleAbsAngle, 0, angled);
	}

	//misc actions
	if (data) {
		if (menu->n()) menu->AddSep();

		if (data->Angled()) {
			menu->AddItem(_("Reset angle"),PATHIA_ResetAngle);
		}
		if (data->HasOffset()) {
			menu->AddItem(_("Apply offset"),PATHIA_ApplyOffset);
			menu->AddItem(_("Reset offset"),PATHIA_ResetOffset);
		}
		menu->AddItem(_("Convert to straight lines"),  PATHIA_MakeStraight);
		menu->AddItem(_("Convert to straight beziers"),PATHIA_MakeBezStraight);
		//menu->AddItem(_("New object from stroke"),     PATHIA_NewFromStroke); 

		if (data->paths.n) {
			//menu->AddItem(_("Break apart all"),PATHIA_BreakApart);
			//menu->AddItem(_("Break apart chunks"),PATHIA_BreakApartChunks);
		}

		//---- shape brushes
		InterfaceManager *imanager = InterfaceManager::GetDefault(true);
		ResourceManager *rm = imanager->GetResourceManager();
		ResourceType *resources = rm->FindType("ShapeBrush");
		if (resources && resources->NumResources()) {
			menu->AddItem(_("Use Shape Brush"));
			menu->SubMenu();
			int numadded = 0;
			resources->AppendMenu(menu, true, &numadded, PATHIA_MAX, PATHIA_UseShapeBrush);
			if (numadded) menu->AddSep();
			resources->AppendMenu(menu, false, &numadded, PATHIA_MAX, PATHIA_UseShapeBrush);
			menu->EndSubMenu();
		}
		menu->AddItem(_("Save as a Shape Brush"), PATHIA_SaveAsShapeBrush);
	}

	//menu->AddItem(_("Combine"),PATHIA_***);

	return menu;
}


int PathInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e_data);
		int i = s->info2; //id of menu item

		if (i > PATHIA_None && i < PATHIA_MAX) PerformAction(i);
		if (i > PATHIA_MAX) {
			//is a resource, shape brush?? line profile
			unsigned int obj_id = i - PATHIA_MAX;
			InterfaceManager *imanager = InterfaceManager::GetDefault(true);
			ResourceManager *rm = imanager->GetResourceManager();

			if (s->info4 == PATHIA_UseShapeBrush) {
				ShapeBrush *brush = dynamic_cast<ShapeBrush*>(rm->FindResourceFromRID(obj_id, "ShapeBrush"));
				if (brush) { // user selected use shape brush
					PostMessage(_("TODO!!!"));
				}
			}
		}
		
		return 0;

	} else if (!strcmp(mes,"setangle")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e_data);
		if (!s || !s->str) return 0;

		if (drawpathi >= 0 && drawpathi < data->paths.n && drawhoveri >= 0 && drawhoveri < data->paths.e[drawpathi]->pathweights.n) {
			PathWeightNode *node = data->paths.e[drawpathi]->pathweights[drawhoveri];
			double d = strtod(s->str, nullptr) * M_PI / 180;

			if (fabs(d - node->angle) > 1e-6) {
				node->angle = d;
				data->paths.e[drawpathi]->needtorecache = 1;
				needtodraw = 1;
			}
		}
		return 0;
	
	} else if (!strcmp(mes,"setpos")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e_data);
		if (!s || !s->str) return 0;

		if (drawpathi >= 0 && drawpathi < data->paths.n && drawhoveri >= 0 && drawhoveri < data->paths.e[drawpathi]->pathweights.n) {
			PathWeightNode *node = data->paths.e[drawpathi]->pathweights[drawhoveri];
			double d = strtod(s->str, nullptr);

			if (d != node->t) {
				data->paths.e[drawpathi]->MoveWeight(drawhoveri, d);
				needtodraw = 1;
			}
		}
		return 0;

	} else if (!strcmp(mes,"setoffset")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e_data);
		if (!s || !s->str) return 0;

		if (drawpathi >= 0 && drawpathi < data->paths.n && drawhoveri >= 0 && drawhoveri < data->paths.e[drawpathi]->pathweights.n) {
			PathWeightNode *node = data->paths.e[drawpathi]->pathweights[drawhoveri];
			double d = strtod(s->str, nullptr);

			if (d != node->offset) {
				node->offset = d;
				data->paths.e[drawpathi]->needtorecache = 1;
				needtodraw = 1;
			}
		}
		return 0;

	} else if (!strcmp(mes,"setwidth")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e_data);
		if (!s || !s->str) return 0;

		if (drawpathi >= 0 && drawpathi < data->paths.n && drawhoveri >= 0 && drawhoveri < data->paths.e[drawpathi]->pathweights.n) {
			PathWeightNode *node = data->paths.e[drawpathi]->pathweights[drawhoveri];
			double d = strtod(s->str, nullptr);

			if (d != node->width) {
				node->width = d;
				data->paths.e[drawpathi]->needtorecache = 1;
				needtodraw = 1;
			}
		}
		return 0;
	
	// } else if (!strcmp(mes,"curcolor")) {
	}

	return 1;
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
	lbfound=NULL;

	if (drawhover==HOVER_Direction) {
		buttondown.moveinfo(d->id,LEFTBUTTON, state,HOVER_Direction);
		defaulthoverp.p(curdirp->p());
		curdirp=&defaulthoverp;
		curdirp->info=0;
		PostMessage(_("Flip add direction, or drag to select next add"));
		//needtodraw=1;
		return 0;
	}

	//catch simple cases
	if (       drawhover == HOVER_WeightTop
			|| drawhover == HOVER_WeightBottom
			|| drawhover == HOVER_WeightPosition
			|| drawhover == HOVER_WeightAngle
			|| drawhover == HOVER_WeightOffset
			|| drawhover == HOVER_AddWeightNode
			|| drawhover == HOVER_RemoveWeightNode
			|| drawhover == HOVER_CutSegment
		  ) {
		buttondown.moveinfo(d->id,LEFTBUTTON, state,drawhover);
		return 0;

	}

	// if control is here, then curpathop did not intercept any left click,
	// so PathInterface is free to do its thing.

	// scan does not take primary into account, so do it manually here
	if (curpathop && primary) lbfound=scan(x,y,curpathop->id);
	else lbfound=scan(x,y);
	lbselected=1;

	// Add Point if did not find any point, and is primary.
	// also flush curpoints,
	// If new point is toprev or tonext, make curvertex the corresponding vertex.

	if (!lbfound) {
		if (drawhover == HOVER_AddPointOn) {
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
					viewport->ChangeObject(oc,0,true);//adds 1 to count
					if (data->paths.n && data->paths.e[data->paths.n-1]->path) {
						SetCurvertex(data->paths.e[data->paths.n-1]->path->lastPoint(1));
					}
					UpdateViewportColor();
					needtodraw=1;
					return 0;

				} else {
					if (c==-1 && viewport->ChangeObject(oc,1,true)) { // change to other object type
						buttondown.up(d->id,LEFTBUTTON);
						return 0;
					}
				}
			}
			if (viewport && !data) viewport->ChangeContext(x,y,NULL);

			if (AddPoint(screentoreal(x,y)) == 0)
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
				//check if curpoint is an endpoint, and we click up on another endpoint,
				//so we need to put a segment between the endpoints
				if (curpoints.e[0]->isEndpoint()) {
					int pathi=-1;
					Coordinate *topoint=scanEndpoints(x,y, &pathi,curpoints.e[0], nullptr);
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
	PostMessage(blah);
	return 0;
}

//! Return data after making it a new, checked out with count of 2 instance of a PathsData
PathsData *PathInterface::newPathsData()
{
	PathsData *ndata=NULL;

	ndata=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
	if (ndata) ndata->style=creationstyle;

	if (!ndata) ndata=new PathsData(creationstyle);//creates 1 count

	if (!linestyle) { linestyle=defaultline; if (linestyle) linestyle->inc_count(); }
	if (!ndata->linestyle) ndata->linestyle=new LineStyle(*linestyle); //***bit of a hack here

	//defaultweight.width=ndata->linestyle->width;
	defaultweight.width=3/dp->Getmag();
	defaultweight.offset=0;
	ndata->linestyle->width=defaultweight.width;

	if (viewport) {
		ObjectContext *oc=NULL;
		viewport->NewData(ndata,&oc);
		if (poc) delete poc;
		poc=oc->duplicate();
	}
	data=ndata;
	return data;
}

/*! Flush curpoints, set curvertex, curpath to NULL.
 */
void PathInterface::clearSelection()
{
	curpoints.flush();
	SetCurvertex(NULL);
	curpath=NULL;
	needtodraw=1;
}

//! Insert at real point p in path after/before relevant break near curvertex.
/*! Flushes curpoints, assigns curvertex to something meanindful.
 * Assumes that p is real coordinates, not screen coordinates, and
 * that what we are adding starts and stops with a vertex.
 *
 * Returns 0 for added, nonzero for not addded.
 */
int PathInterface::AddPoint(flatpoint p)
{

	// If there is no data, we must create a new data.
	if (!data) {
		newPathsData();
		UpdateViewportColor();
	}
	p = transform_point_inverse(datam(),p);

	// Any new point is added to the appropriate point after curvertex.
	// For currently degenerate paths, this is simply making the path be a
	// new unit from curpathop, or a plain vertex if no curpathop.
	// Otherwise, the appropriate insertion point must be
	// found, and the correctly constructed unit is inserted.


	// If there is no valid existing curvertex (ie, data has no paths, or only has null paths),
	// it must be created, essentially by assigning the created node to a path.

	if (!curvertex) {
		//start new subpath
		if ((pathi_style&PATHI_One_Path_Only) && data->paths.n >= 1) return 1;
		data->pushEmpty();
		curpath = data->paths.e[data->paths.n-1];
	}

	//now curpath exists, curvertex might exist.
	curpath->needtorecache=1;

	// allocate new cluster of Coordinate objects
	Coordinate *np=NULL, *cp=NULL;
	if (curpathop) {
		np = curpathop->newPoint(p); // newPoint defaults to a full unit
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

	if (show_addpoint) {
		add_point_hint[1] = add_point_hint[2] = add_point_hint[3] = p;
		needtodraw = 1;
	}

	// If curvertex==NULL, then curpath is currently a null path, thus with no weight nodes
	if (!curvertex) {
		curvertex = np->nextVertex(1);
		curpath->path = curvertex;
		curpath->needtorecache = 1;

		SetCurvertex(curvertex);
		curpoints.flush();
		if (!cp) cp = curvertex;
		curpoints.push(cp, 0);
		data->FindBBox();
		return 0;
	}

	// There is a curpath and a curvertex, figure out how to insert a new thing in the path.

	if (curpath->AddAt(curvertex, np, addafter)==0) {
		SetCurvertex(np->nextVertex(1)); // *** warning! this might return NULL!! shouldn't really happen though ... right? right?
		curpoints.flush();
		if (!cp) cp=curvertex;
		curpoints.push(cp,0);
		curpath->needtorecache=1;
		data->FindBBox();
		Modified(0);
		needtodraw=1;

	} else {
		delete np;
		PostMessage(_("Couldn't add point"));
	}

	return 0;
}

//! Remove a controlled range of points.
/* If p is an uncontrolled bezier control point, only that point is removed.
 * If p is an uncontrolled vertex, nothing is done.
 *
 * The actual Coordinates are deleted, and must not be accessed again.
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
	if (mes) PostMessage(mes);
	needtodraw=1;
}

/*! Return 0 for success, nonzero for couldn't connect.
 */
int PathInterface::ConnectEndpoints(Coordinate *from,int frompathi, Coordinate *to,int topathi)
{
	if (!data) return 1;

	int status=data->ConnectEndpoints(from,frompathi,to,topathi);

	if (status==0) {
		curpoints.flush();
		curpoints.pushnodup(to,0);
		SetCurvertex(to);
		needtodraw=1;
	}

	return status;
}

/*! Take 2 end points, and make them the same point, merging paths if necessary.
 * If there is merging and fromi != toi, path toi is removed.
 */
int PathInterface::MergeEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi)
{
	//find which sides they are endpoints
	//make to be same path direction as from

	if (fromi < 0) fromi = data->hasCoord(from);
	if (toi < 0)   toi   = data->hasCoord(to);
	if (fromi > toi) { // make sure fromi < toi so the curpoint updating below works as expected
		Coordinate *cc = from;
		from = to;
		to = cc;
		int ii = fromi;
		fromi = toi;
		toi = ii;
	}

	if (!data->MergeEndpoints(from,fromi, to,toi)) {
		return 0;
	}

	curpoints.flush();
	curpoints.pushnodup(data->paths.e[fromi]->path,0);
	SetCurvertex(data->paths.e[fromi]->path);
	needtodraw=1;

	return 1;
	// ---------------

	// if (fromi<0) fromi=data->hasCoord(from);
	// if (toi<0) toi=data->hasCoord(to);
	// if (toi<0 || fromi<0) return 0;

	// int fromdir=from->isEndpoint();
	// int todir  =to->isEndpoint();
	// if (!fromdir || !todir) return 0;
	// curpoints.flush();

	// if ((fromdir>0 && todir>0) || (fromdir<0 && todir<0)) data->ReversePath(toi);

	// //need to take prev of from and put on to, removing to's prev (for fromdir>0)
	// Coordinate *fp;
	// Coordinate *tp;
	// Coordinate *tmp;

	// //standardize connect order for code below
	// if (fromdir<0) {
	// 	tmp=from;
	// 	from=to;
	// 	to=tmp;
	// 	int tt=fromi;
	// 	fromi=toi;
	// 	toi=tt;
	// }

	// //we connect from final path point to beginning path point
	// fp=from->lastPoint(0);
	// tp=to->firstPoint(0);

	// //first remove in between control points
	// if (fp->flags&POINT_TOPREV) {
	// 	fp=fp->prev;
	// 	tmp=fp->next;
	// 	fp->next=NULL;
	// 	tmp->prev=NULL;
	// 	delete tmp;
	// }
	// if (tp->flags&POINT_TONEXT) {
	// 	tp=tp->next;
	// 	tmp=tp->prev;
	// 	tp->prev=NULL;
	// 	tmp->next=NULL;
	// 	delete tmp;
	// }

	// //now fp and tp should be vertices, need to remove fp
	// fp=fp->prev;
	// tmp=fp->next;
	// fp->next=NULL;
	// tmp->prev=NULL;
	// delete tmp;

	// //finally connect the terminal points
	// fp->next=tp;
	// tp->prev=fp;

	// data->paths.e[fromi]->path=to->firstPoint(1);
	// data->paths.e[fromi]->needtorecache=1;
	// if (fromi!=toi) {
	// 	//remove other path
	// 	data->paths.e[toi]->path=NULL;
	// 	data->paths.remove(toi);
	// }

	// curpoints.pushnodup(to,0);
	// SetCurvertex(to);
	// needtodraw=1;

	// return 1;
}

int PathInterface::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int i1;
	int action;
	int xi,yi;
	buttondown.getinitial(d->id,LEFTBUTTON, &xi,&yi);
	int moved = buttondown.up(d->id,LEFTBUTTON, &i1,&action);
	if (moved < 0) return 1;

	//DBG cerr <<"  ***** PathInterface::LBUp action: "<<action<<endl;

	if (action==HOVER_Direction) {
		if (!moved) {
			addafter   = !addafter;
			curdirv    = -curdirv;
			drawhover  = HOVER_None;
			needtodraw = 1;
			UpdateAddHint();
			MouseMove(x,y,state,d); //force remap of scan
		}
		return 0;

	} else if (action == HOVER_CutSegment) {
		PerformAction(PATHIA_CutSegment);
		return 0;

	} else if (action == HOVER_AddWeightNode) {
		int pathi=-1;
		double t;
		data->ClosestPoint(hoverpoint, NULL,NULL,&t, &pathi);

		double width=defaultweight.width, offset=0, angle=defaultweight.angle;
		data->paths.e[pathi]->GetWeight(t, &width, &offset, &angle);
		data->paths.e[pathi]->AddWeightNode(t,offset,width,angle);

		Modified(0);
		show_weights=true;
		drawhover=HOVER_None;
		needtodraw=1;
		return 0;

	} else if (action==HOVER_RemoveWeightNode) {
		data->paths.e[drawpathi]->RemoveWeightNode(drawhoveri);

		Modified(0);
		drawhover=HOVER_None;
		needtodraw=1;
		return 0;

	} else if (action==HOVER_WeightAngle 
		    || action==HOVER_WeightPosition
		    || action==HOVER_WeightOffset
		    || action==HOVER_WeightTop
		    || action==HOVER_WeightBottom) {
		//drawhover=HOVER_None;
		drawhover=action;

		if (moved < DraggedThreshhold()) {
			const char *label = nullptr;
			const char *mes = nullptr;
			char str[50];
			if (drawpathi >= 0 && drawpathi < data->paths.n) {
				if (drawhoveri==-1) {
					 //we need to install a new path weight node
					data->paths.e[drawpathi]->AddWeightNode(defaultweight.t, defaultweight.offset, defaultweight.width, defaultweight.angle);
					drawhoveri=0;
				}
				if (drawhoveri >= 0 && drawhoveri < data->paths.e[drawpathi]->pathweights.n) {
					PathWeightNode *node = data->paths.e[drawpathi]->pathweights[drawhoveri];

					if (action==HOVER_WeightAngle)         { sprintf(str, "%g", node->angle*180/M_PI);  mes = "setangle";  label = _("Angle degrees"); }
					else if (action==HOVER_WeightPosition) { sprintf(str, "%g", node->t);      mes = "setpos";    label = _("Position"); }
					else if (action==HOVER_WeightOffset)   { sprintf(str, "%g", node->offset); mes = "setoffset"; label = _("Offset"); }
					else if (action==HOVER_WeightBottom || action == HOVER_WeightTop)
						{ sprintf(str, "%g", node->width);  mes = "setwidth";  label = _("Width"); }
				
					double th = UIScale() * app->defaultlaxfont->textheight();
					DoubleBBox bounds(x-5*th, x+5*th, y-th/2, y+th/2);
					viewport->SetupInputBox(object_id, label, str, mes, bounds);
					edit_pathi = drawpathi;
					edit_weighti = drawhoveri;
				}
			}
		}

		needtodraw=1;
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

		UpdateAddHint();
		curdirp = curvertex;
		drawhover = HOVER_None;
		needtodraw = 1;
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
				p=realtoscreen(transform_point(datam(),cp->p()));
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

	//below is for various special cases of merging and point selection
	switch (state&(ControlMask|AltMask|MetaMask|ShiftMask)) {
		case (0): { // plain click
					  if (curpoints.n==1 && moved) {
						  //check if moving endpoint onto another endpoint
						  if (abs(curpoints.e[0]->isEndpoint())==1) {
							  int pathi=-1;
							  Coordinate *topoint=scanEndpoints(x,y, &pathi,curpoints.e[0], nullptr);
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
								  if (c>=0) ; //curpoints.pop(c); // toggle off
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

void PathInterface::Modified(int level)
{
	data->touchContents();
	anInterface::Modified(level);
}

//! Add a new point at closest point to hoverpoint, which is in data coordinates.
/*! Return 0 for point added, or 1 for error and couldn't add.
 */
int PathInterface::CutNear(flatpoint hoverpoint)
{
	double t;
	int pathi=-1;
	//flatpoint newpoint=data->ClosestPoint(hoverpoint, NULL,NULL,&t,&pathi);
	data->ClosestPoint(hoverpoint, NULL,NULL,&t,&pathi);

	Coordinate *p1=data->paths[pathi]->AddAt(t);
	if (!p1) return 1;

	curpath=data->paths.e[pathi];
	curpath->needtorecache=1;
	SetCurvertex(p1,pathi);

	Modified(0); 
	return 0;
}

int PathInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (state&LAX_STATE_MASK) return 1;

	//plain wheel to change point type
	Coordinate *p=scan(x,y, 0,NULL);
	if (!p || !(p->flags&POINT_VERTEX)) return 1;
	SetPointType(p,-1);
	hoverpointtype=p->flags&BEZ_MASK;

	return 0;
}

int PathInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (state&LAX_STATE_MASK) return 1;

	//plain wheel to change point type
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

	if      (drawhover == HOVER_Vertex || drawhover==HOVER_Point || drawhover==HOVER_Handle) mes="";
	else if (drawhover == HOVER_Endpoint)         mes = _("Click to add segment between endpoints");
	else if (drawhover == HOVER_MergeEndpoints)   mes = _("Merge endpoints");
	else if (drawhover == HOVER_AddPointOn)       mes = _("Click to add point to segment");
	else if (drawhover == HOVER_CutSegment)       mes = _("Cut segment");
	else if (drawhover == HOVER_AddWeightNode)    mes = _("Click to add new width node");
	else if (drawhover == HOVER_RemoveWeightNode) mes = _("Click to remove this width node");
	else if (drawhover == HOVER_WeightPosition)   mes = _("Drag weight node position");
	else if (drawhover == HOVER_WeightTop)        mes = _("Drag weight node top");
	else if (drawhover == HOVER_WeightBottom)     mes = _("Drag weight node bottom");
	else if (drawhover == HOVER_WeightAngle)      mes = _("Drag weight node angle");
	else if (drawhover == HOVER_WeightOffset)     mes = _("Drag path offset");
	else if (drawhover == HOVER_Direction)        mes = _("Click to flip add direction, or drag to select next add");
	
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
	DBG cerr <<"path  rts: "<<fp.x<<','<<fp.y<<endl;
	DBG cerr <<"path  drawhover: "<<drawhover<<endl;


	if (!buttondown.isdown(mouse->id,LEFTBUTTON)) {
		int oldhover=drawhover;
		int oldpathi=drawhoveri;
		int oldhoveri=drawhoveri;
		drawpathi=-1;
		drawhover=HOVER_None;
		drawhoveri=-1;

		if (show_addpoint && data) {
			add_point_hint[1] = add_point_hint[2] = add_point_hint[3] = transform_point_inverse(datam(), screentoreal(x,y));
			needtodraw = 1;
		}

		//check for hovering over endpoint when another endpoint is selected
		if (data && (state&ShiftMask) && curpoints.n==1) {
			int pathi=-1;
			bool isfirst = false;
			Coordinate *topoint = scanEndpoints(x,y, &pathi,curpoints.e[0], &isfirst);
			if (topoint && topoint!=curpoints.e[0] && abs(topoint->isEndpoint())==1) {
				drawhover = HOVER_Endpoint;
				hoverpoint = topoint->p();

				show_addpoint = 1;
				add_point_hint[2] = topoint->p();
				if (isfirst) {
					add_point_hint[1] = (topoint->prev && (topoint->prev->flags & POINT_TONEXT)) ? topoint->prev->p() : topoint->p();
				} else {
					add_point_hint[1] = (topoint->next && (topoint->next->flags & POINT_TOPREV)) ? topoint->next->p() : topoint->p();
				}

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

		 //else maybe over weight nodes
		if (show_weights) {
			drawhover = scanWeights(x,y,state, &drawpathi, &drawhoveri);

			if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)
				  && (drawhover==HOVER_WeightPosition
				   || drawhover==HOVER_WeightTop
				   || drawhover==HOVER_WeightBottom
				   || drawhover==HOVER_WeightAngle
				   || drawhover==HOVER_WeightOffset)) {
				drawhover = HOVER_RemoveWeightNode;
			}
		}

		 //else maybe something else to hover over:
		if (drawhover == HOVER_None) drawhover = scanHover(x,y,state, &drawpathi);

		if (drawhover==HOVER_AddWeightNode) {
			 // *** should grab characteristics from the found path...
			DBG cerr <<"drawpathi: "<<drawpathi<<endl;
			defaultweight.offset=0;
			if (data->paths.e[drawpathi]->linestyle)
				defaultweight.width=data->paths.e[drawpathi]->linestyle->width;
			else if (data->linestyle)
				defaultweight.width=data->linestyle->width;
		}

		if (drawhover != oldhover
		 || drawpathi != oldpathi
		 || drawhoveri != oldhoveri
		 || drawhover == HOVER_AddPointOn
		 || drawhover == HOVER_AddWeightNode) {
			needtodraw=1;
			hoverMessage();
		}
		return 0;
	}

	//now left button down assumed..
	if (!data) return 0;

	int oldx,oldy;
	int i1,action;
	buttondown.move(mouse->id,x,y, &oldx,&oldy);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON, &i1,&action);

	DBG cerr <<"mouse move action:"<<action<<endl;

	flatpoint p1=screentoreal(x,y);
	flatpoint p2=screentoreal(oldx,oldy);
	flatpoint d=transform_point_inverse(datam(),p1)-transform_point_inverse(datam(),p2);

	DBG cerr <<"path mm action: "<<action<<endl;

	if (action==HOVER_AddWeightNode || action==HOVER_RemoveWeightNode) {
		return 0;

	} else if (action==HOVER_WeightAngle) {
		if (drawpathi<0) return 0;
		Path *path=data->paths.e[drawpathi];
		if (drawhoveri==-1) {
			 //we need to install a new path weight node
			path->AddWeightNode(defaultweight.t, defaultweight.offset, defaultweight.width, defaultweight.angle);
			drawhoveri=0;
		}

		PathWeightNode *weight=path->pathweights.e[drawhoveri];
		flatpoint pp;

		if (path->PointAlongPath(weight->t, 0, &pp, NULL)==0) return 0; //not a valid position

		flatpoint vnew=transform_point_inverse(data->m(),p1) - pp;
		flatpoint vold=transform_point_inverse(data->m(),p2) - pp;
		double aa=angle_full(vnew,vold);
		weight->angle-=aa;
		//while (weight->angle<-M_PI) weight->angle+=2*M_PI; // *** maybe we want weird angles though!!
		//while (weight->angle>M_PI) weight->angle-=2*M_PI; // *** maybe we want weird angles though!!
		DBG cerr <<" ---- moving weight angle by: "<<aa<<", new angle: "<<weight->angle<<endl;

		path->needtorecache=1;
		Modified(0);
		needtodraw=1;
		return 0;

	} else if (action==HOVER_WeightPosition) {
		if (drawpathi<0) return 0;

		Path *path=data->paths.e[drawpathi];
		if (drawhoveri==-1) {
			 //we need to install a new path weight node
			path->AddWeightNode(defaultweight.t, defaultweight.offset, defaultweight.width, defaultweight.angle);
			drawhoveri=0;
		}

		flatpoint pp;
		//--------
		double t;
		path->ClosestPoint(transform_point_inverse(datam(),p1), NULL, NULL, &t);
		//drawhoveri=path->MoveWeight(drawhoveri, t);
		//---------
		// //try to map node based on relation as when originally clicked down (needs work)
		//PathWeightNode *weight=path->pathweights.e[drawhoveri];
		//
		//if (path->PointAlongPath(weight->t, 0, &pp, NULL)==0) return 0;
		//pp+=d;
		//double t=weight->t;
		//path->ClosestPoint(pp, NULL, NULL, &t);
		//--------------

		drawhoveri=path->MoveWeight(drawhoveri, t);
		DBG cerr <<"t:"<<t<<endl;
		// *** data->remapWidths(drawhoveri);

		Modified(0);
		path->needtorecache=1;
		needtodraw=1;
		return 0;

	} else if (action==HOVER_WeightTop || action==HOVER_WeightBottom || action == HOVER_WeightOffset) {
		if (drawpathi<0) return 0;

		Path *path=data->paths.e[drawpathi];
		if (drawhoveri==-1) {
			 //we need to install a new path weight node
			path->AddWeightNode(defaultweight.t, defaultweight.offset, defaultweight.width, defaultweight.angle);
			drawhoveri=0;
		}

		PathWeightNode *weight=path->pathweights.e[drawhoveri];
		flatpoint pp,vv, vt, ptop,pbottom;

		if (path->PointAlongPath(weight->t, 0, &pp, &vv)==0) return 0; //not a valid point

		if (vv.isZero()) path->PointAlongPath(weight->t+.00001, 0, NULL,&vv);
		vv.normalize();
		vt=transpose(vv);
		if (path->Angled()) {
			if (path->absoluteangle) vt=rotate(flatpoint(1,0), weight->angle);
			else vt=rotate(vt, weight->angle);
		}


		double toffset=weight->topOffset();
		double boffset=weight->bottomOffset();
		double dd=d*vt;

		if (action == HOVER_WeightTop) {
			toffset += dd;

			if (!(pathi_style&PATHI_No_Offset) && (state&LAX_STATE_MASK)==ControlMask)
				; //do nothing to bottom when control to move only one arrow
			else boffset -= dd;

		} else if (action == HOVER_WeightBottom) {
			boffset += dd;

			if (!(pathi_style&PATHI_No_Offset) && (state&LAX_STATE_MASK)==ControlMask)
				; //do nothing to top when control to move only one arrow
			else toffset -= dd;
		} else { // action == HOVER_WeightOffset
			if (!(pathi_style & PATHI_No_Offset)) {
				toffset += dd;
				boffset += dd;
			}
		}

		weight->width  = toffset-boffset;
		weight->offset = (toffset+boffset)/2;

		if (weight->width < 0) {
			if (action == HOVER_WeightTop) action=HOVER_WeightBottom; else action=HOVER_WeightTop;
			buttondown.moveinfo(mouse->id,LEFTBUTTON, state,action);
			weight->width = -weight->width;
		}

		data->line(weight->width);
		path->needtorecache=1;

		Modified(0);
		needtodraw=1;
		return 0;

	} else if (action==HOVER_Direction) {
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
			pp=first + v*2 + dir_select_radius*ScreenLine()*(v/norm(v));
			curdirp->p(transform_point_inverse(datam(),screentoreal(pp.x,pp.y)));
			needtodraw=1;
		}
		Modified(0);
		return 0;

	} else if (action==HOVER_DirectionSelect) {
		int which=0;
		flatpoint pp=realtoscreen(transform_point(datam(),curdirp->p()));
		if (norm2(pp-flatpoint(x,y))>dir_select_radius*dir_select_radius*ScreenLine()*ScreenLine()) which=0;
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

	if (curpoints.n==1 && action!=HOVER_Direction && action!=HOVER_DirectionSelect) {
		 //check if moving endpoint onto another endpoint
		DBG cerr <<"curpoint is endpoint: "<<abs(curpoints.e[0]->isEndpoint())<<endl;

		if (abs(curpoints.e[0]->isEndpoint())==1) {
			int pathi=-1;
			Coordinate *topoint=scanEndpoints(x,y, &pathi,curpoints.e[0], nullptr);
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
					if (poc && poc->obj != data)
						poc->obj->origin(poc->obj->origin()+(p1-p2));
					else data->origin(data->origin()+(p1-p2));
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

	data->Recache(false);
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

	sc->Add(PATHIA_CurpointOnHandle,  LAX_Up,0,0,        "HandlePoint",  _("Switch between handle and vertex"),NULL,0);
	sc->Add(PATHIA_CurpointOnHandleR, LAX_Up,ShiftMask,0,"HandlePointR", _("Switch between handle and vertex"),NULL,0);
	sc->Add(PATHIA_Pathop,            'o',0,0,        "Pathop",       _("Change path operator"),NULL,0);
	sc->Add(PATHIA_ToggleAbsAngle,    '^',ShiftMask,0,"ToggleAbsAngle",_("Toggle absolute angles in subpaths"),NULL,0);
	sc->Add(PATHIA_ToggleFillRule,    'F',ShiftMask,0,"ToggleFillRule",_("Toggle fill rule"),NULL,0);
	sc->Add(PATHIA_ToggleFill,        'f',0,0,        "ToggleFill",   _("Toggle fill"),NULL,0);
	sc->Add(PATHIA_ToggleStroke,      's',0,0,        "ToggleStroke", _("Toggle showing stroke"),NULL,0);
	// sc->Add(PATHIA_ColorFillOrStroke, 'x',0,0,        "ColorDest",    _("Send colors to fill or to stroke"),NULL,0);
	sc->Add(PATHIA_RollNext,          LAX_Right,0,0,  "RollNext",     _("Select next points"),NULL,0);
	sc->Add(PATHIA_RollPrev,          LAX_Left,0,0,   "RollPrev",     _("Select previous points"),NULL,0);
	sc->Add(PATHIA_ToggleAddAfter,    'n',0,0,        "AddAfter",     _("Toggle add after"),NULL,0);
	sc->Add(PATHIA_TogglePointType,   'p',0,0,        "PointType",    _("Change point type"),NULL,0);
	sc->Add(PATHIA_Select,            'a',0,0,        "Select",       _("Select all or deselect"),NULL,0);
	sc->Add(PATHIA_SelectInPath,      'A',ShiftMask,0,"SelectInPath", _("Select all in path or deselect"),NULL,0);
	sc->Add(PATHIA_SelectInvert,      'i',ControlMask,0,"SelectInvert",_("Invert selection"),NULL,0);
	sc->Add(PATHIA_Close,             'c',0,0,        "ToggleClosed", _("Toggle closed"),NULL,0);
	sc->Add(PATHIA_CutSegment,        'C',ShiftMask,0,"CutSegment",   _("Cut next segment"),NULL,0);
	sc->Add(PATHIA_Decorations,       'd',0,0,        "Decorations",  _("Toggle decorations"),NULL,0);
	sc->Add(PATHIA_StartNewPath,      'B',ShiftMask,0,"NewPath",      _("Start a totally new path object"),NULL,0);
	sc->Add(PATHIA_StartNewSubpath,   'b',0,0,        "NewSubpath",   _("Start a new subpath"),NULL,0);
	sc->Add(PATHIA_ToggleOutline,     '_',ShiftMask,0,"ToggleOutline",_("Toggle indicating outlines (DEBUGGING)"),NULL,0);
	sc->Add(PATHIA_ToggleBaseline,    '|',ShiftMask,0,"ToggleBaseline",_("Toggle indicating base line references"),NULL,0);
	sc->Add(PATHIA_ToggleWeights,     'w',0,0,        "ToggleWeights",_("Toggle editing of weights along lines"),NULL,0);
	sc->Add(PATHIA_Wider,             'l',0,0,        "Wider",        _("Thicken the line"),NULL,0);
	sc->Add(PATHIA_Thinner,           'L',ShiftMask,0,"Thinner",      _("Thin the line"),NULL,0);
	sc->Add(PATHIA_WidthStep,         'l',ControlMask,0,"WidthStep",  _("Change how much change for width changes"),NULL,0);
	sc->Add(PATHIA_WidthStepR,        'L',ControlMask|ShiftMask,0,"WidthStepR", _("Change how much change for width changes"),NULL,0);
	sc->Add(PATHIA_MakeCircle,        '0',0,0,        "Circle",       _("Arrange points in a circle"),NULL,0);
	sc->Add(PATHIA_MakeRect,          'R',ShiftMask,0,"Rect",         _("Arrange points in a rectangle"),NULL,0);
	sc->Add(PATHIA_Reverse,           'r',0,0,        "Reverse",      _("Reverse direction of current path"),NULL,0);
	sc->Add(PATHIA_Delete,            LAX_Del,0,0,    "Delete",       _("Delete selected points"),NULL,0);
	sc->AddShortcut(LAX_Bksp,0,0, PATHIA_Delete);

	sc->Add(PATHIA_FlipVertically,    'v',0,0,        "FlipVertically",  _("Flip selected points vertically"),NULL,0);
	sc->Add(PATHIA_FlipHorizontally,  'h',0,0,        "FlipHorizontally",_("Flip selected points horizontally"),NULL,0); 

	sc->Add(PATHIA_ApplyOffset,       '_',ControlMask|ShiftMask,0, "ApplyOffset", _("Apply offset to current paths"),NULL,0);
	sc->Add(PATHIA_ResetOffset,       '|',ControlMask|ShiftMask,0, "ResetOffset", _("Make offset values (if any) be 0 of current paths"),NULL,0);
	sc->Add(PATHIA_MakeStraight,      '\\',ControlMask,0,"MakeStraight",    _("Make segments of current points be straight"),NULL,0);
	sc->Add(PATHIA_MakeBezStraight,   '\\',0,0,          "MakeBezStraight", _("Make segments of current points be straight with bezier handles"),NULL,0);
	sc->Add(PATHIA_ResetAngle,        '<',ShiftMask,0,   "ResetAngle",      _("Make weight angles be zero, and set to not absolute angles"),NULL,0);

	//sc->Add(PATHIA_Combine,           'k',ControlMask,0,    "Combine",      _("Combine multiple path objects into a single path object"),NULL,0);
	//sc->Add(PATHIA_BreakApart,        'K',ShiftMask,0,      "BreakApart",   _("Break paths of current points to new path objects"),NULL,0);
	//sc->Add(PATHIA_BreakApartChunks,  'K',ControlMask|ShiftMask,0,"BreakApartChunks", _("Break subpaths into new objects, preserving holes"),NULL,0);
	//sc->Add(PATHIA_Copy,              'c',ControlMask,0,    "Copy",         _("Copy current points to clipboard"),NULL,0);
	//sc->Add(PATHIA_Cut,               'x',ControlMask,0,    "Cut",          _("Delete selected points"),NULL,0);
	//sc->Add(PATHIA_Paste,             'v',ControlMask,0,    "Paste",        _("Paste clipboard if it is points"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

int PathInterface::PerformAction(int action)
{
	last_action = recent_action;
	recent_action = action;

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

	} else if (action == PATHIA_ToggleShowPoints) {
		show_points = !show_points;
		needtodraw = 1;
		return 0;

	} else if (action == PATHIA_ToggleHideControls) {
		hide_other_controls = !hide_other_controls;
		needtodraw = 1;
		return 0;

	} else if (action==PATHIA_FlipHorizontally || action==PATHIA_FlipVertically) {
		if (!data || !curpoints.n) {
			PostMessage(_("No points to flip!"));
			return 0;
		}

		DoubleBBox box;
		for (int c=0; c<curpoints.n; c++) {
			box.addtobounds(curpoints.e[c]->p());
		}

		for (int c=0; c<curpoints.n; c++) {
			flatpoint fp = curpoints.e[c]->p();
			if (action == PATHIA_FlipHorizontally)
				fp.x = box.maxx - (fp.x - box.minx);
			else fp.y = box.maxy - (fp.y - box.miny);
			curpoints.e[c]->p(fp);
		}

		data->Recache();
		data->FindBBox();
		PostMessage(_("Points flipped"));
		needtodraw = 1;
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

	} else if (action==PATHIA_Copy) {
		// copy points .. from a path only? copy preserving subpaths?
		PostMessage(_("NEED TO IMPLEMENT PATHIA_Copy"));
		return 0;

	} else if (action==PATHIA_Cut) {
		PostMessage(_("NEED TO IMPLEMENT PATHIA_Cut"));
		return 0;

	} else if (action==PATHIA_Paste) {
		// 
		PostMessage(_("NEED TO IMPLEMENT PATHIA_Paste"));
		return 0;

	} else if (action==PATHIA_Subdivide) {
		// if previous action was subdivide, unsubdivide then subdivide with other number
		PostMessage(_("NEED TO IMPLEMENT PATHIA_Subdivide"));
		return 0;

	} else if (action==PATHIA_ToggleAbsAngle) {
		if (!data) return 0;
		for (int c=0; c<data->paths.n; c++) {
			data->paths.e[c]->absoluteangle=!data->paths.e[c]->absoluteangle;
			data->paths.e[c]->needtorecache=1;
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleFillRule) {
		 //toggle fill rule
		if (!data) return 0;

		if (!data->fillstyle) data->fillstyle=new FillStyle(*defaultfill);
		if (data->fillstyle->fillrule==LAXFILL_Nonzero) {
			data->fillstyle->fillrule=LAXFILL_EvenOdd;
			PostMessage(_("Fill odd winding"));
		} else {
			data->fillstyle->fillrule=LAXFILL_Nonzero; 
			PostMessage(_("Fill all"));
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleFill) {
		 //toggle fill or not
		if (!data) return 0;

		if (data->fillstyle && data->fillstyle->function != LAXOP_None && data->fillstyle->fillstyle != FillNone) {
			data->fillstyle->fillstyle = FillNone;
			data->fillstyle->function = LAXOP_None;
			PostMessage(_("Don't fill"));
		} else {
			if (!data->fillstyle) data->fillstyle=new FillStyle(*defaultfill);
			data->fillstyle->fillstyle=FillSolid;
			data->fillstyle->function = LAXOP_Over;
			PostMessage(_("Fill"));
		} 
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleStroke) {
		if (data->linestyle && data->linestyle->function!=LAXOP_None) {
			data->linestyle->function=LAXOP_None;
			PostMessage(_("Don't stroke"));
		} else {
			if (!data->linestyle) data->linestyle=new LineStyle(*defaultline);
			data->linestyle->function=LAXOP_Over;
			PostMessage(_("Stroke"));
		}
		needtodraw=1;
		return 0;

	// } else if (action==PATHIA_ColorFillOrStroke) {
	// 	 //toggle send colors to fill or to stroke ..... *** obsolete???
	// 	colortofill=!colortofill;
	// 	const char *mes=colortofill?_("Send color to fill"):_("Send color to stroke");
	// 	UpdateViewportColor();
	// 	PostMessage(mes);
	// 	return 0;

	} else if (action==PATHIA_Bevel || action==PATHIA_Miter || action==PATHIA_Round || action==PATHIA_Extrapolate) {
		int j=LAXJOIN_Bevel;
		if (action==PATHIA_Miter) j=LAXJOIN_Miter;
		else if (action==PATHIA_Round) j=LAXJOIN_Round;
		else if (action==PATHIA_Extrapolate) j=LAXJOIN_Extrapolate;

		data->linestyle->joinstyle=j;
		for (int c=0; c<data->paths.n; c++) {
			if (data->paths.e[c]->linestyle) data->paths.e[c]->linestyle->joinstyle=j;
			else data->paths.e[c]->Line(data->linestyle);
			data->paths.e[c]->needtorecache=1;
		}

		if (action==PATHIA_Bevel) PostMessage(_("Bevel join"));
		else if (action==PATHIA_Miter) PostMessage(_("Miter join"));
		else if (action==PATHIA_Round) PostMessage(_("Round join"));
		else if (action==PATHIA_Extrapolate) PostMessage(_("Extrapolate join"));

		needtodraw=1;
		return 0;

	} else if (action==PATHIA_CapButt || action==PATHIA_CapRound || action==PATHIA_CapZero) {
		int cap = LAXCAP_Butt;
		const char *message = _("Cap butt");
		if      (action==PATHIA_CapZero)  { cap=LAXCAP_Zero_Width; message=_("Zero tip caps"); }
		else if (action==PATHIA_CapRound) { cap=LAXCAP_Round;      message=_("Round caps");    }

		if (data && data->linestyle) {
			data->linestyle->capstyle = cap;
			for (int c=0; c<data->paths.n; c++) {
				if (data->paths.e[c]->linestyle) data->paths.e[c]->linestyle->capstyle = cap;
				else data->paths.e[c]->Line(data->linestyle);
				data->paths.e[c]->needtorecache = 1;
			}
		} else defaultline->capstyle = cap;

		PostMessage(message);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_EndCapSame || action==PATHIA_EndCapButt || action==PATHIA_EndCapRound || action==PATHIA_EndCapZero) {
		int cap = LAXCAP_Butt;
		const char *message=_("End cap butt");

		if      (action == PATHIA_EndCapSame)  { cap = 0;                 message = _("End cap same as start cap"); }
		else if (action == PATHIA_EndCapZero)  { cap = LAXCAP_Zero_Width; message = _("Zero tip end cap"); }
		else if (action == PATHIA_EndCapRound) { cap = LAXCAP_Round;      message = _("Round end cap");    }

		if (data && data->linestyle) {
			data->linestyle->endcapstyle = cap;
			for (int c=0; c<data->paths.n; c++) {
				if (data->paths.e[c]->linestyle) data->paths.e[c]->linestyle->endcapstyle = cap;
				else data->paths.e[c]->Line(data->linestyle);
				data->paths.e[c]->needtorecache = 1;
			}
		} else defaultline->endcapstyle = cap;

		PostMessage(message);
		needtodraw=1;
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
		addafter = !addafter;
		if (curvertex) SetCurvertex(curvertex);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_TogglePointType) {
		 //toggle vertex smoothness type
		SetPointType(-1);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_PointTypeSmooth) {
		SetPointType(BEZ_STIFF_EQUAL);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_PointTypeSmoothUnequal) {
		SetPointType(BEZ_STIFF_NEQUAL);
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_PointTypeCorner) {
		SetPointType(BEZ_NSTIFF_NEQUAL);
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

	} else if (action==PATHIA_SelectInvert) {
		if (curpoints.n == 0) PerformAction(PATHIA_Select);
		Laxkit::PtrStack<Coordinate> points;
		Coordinate *p,*p2=NULL,*pt;
		for (int c=0; c<data->paths.n; c++) {
			p = data->paths.e[c]->path;
			if (p) {
				p2 = p = p->firstPoint(0);
				do {
					if (curpoints.findindex(p2) < 0) points.push(p2,0);
					if (p2->controls) {
						 //select as curpoint only 1 point in a controlled segment
						pt = p2;
						while (p2 && p2->controls==pt->controls) p2=p2->next;
					} else p2 = p2->next;
				} while (p2 && p2!=p);
			}
		}
		int n;
		char *local = nullptr;
		Coordinate **e = points.extractArrays(&local, &n);
		curpoints.insertArrays(e,local,n);
		needtodraw = 1;

		PostMessage(_("Selection inverted"));
		return 0;

	} else if (action==PATHIA_SelectInPath) {
		 // like 'a' but only with the single path containing curvertex
		if (!data || !curpoints.n) return 0;

		NumStack<int> paths;
		Coordinate *p, *p2;

		for (int c=0; c<curpoints.n; c++) {
			int c2=data->hasCoord(curpoints.e[c]);
			paths.pushnodup(c2);
		}
		for (int c2=0; c2<paths.n; c2++) {
			p=data->paths.e[paths.e[c2]]->path; 
			p2=p=p->firstPoint(0);

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

	} else if (action == PATHIA_CutSegment) {
		 // Close/Open current path at curpoint
		if (!data) return 0;

		CutSegment();
	
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
		SetCurvertex(NULL);
		PostMessage(_("Next point starts subpath"));
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleOutline) {
		show_outline=!show_outline;
		if (show_outline) PostMessage(_("Show outlines (DEBUGGING)"));
		else PostMessage(_("Don't show outlines (DEBUGGING)"));
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleBaseline) {
		show_baselines=!show_baselines;
		if (show_baselines) PostMessage(_("Show base lines"));
		else PostMessage(_("Don't show base lines"));
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_ToggleWeights) {
		if (pathi_style&PATHI_No_Weights) show_weights=false;
		else show_weights=!show_weights;
		if (show_weights) PostMessage(_("Show weight controls"));
		else PostMessage(_("Don't show weight controls"));
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Wider) {
		if (!data) return 0;
		if (!data->linestyle) data->linestyle=new LineStyle;
		if (!data->linestyle->width) data->linestyle->width=1;
		data->linestyle->width*=widthstep;
		data->Recache();

		defaultweight.width=data->linestyle->width;

		char buffer[100];
		sprintf(buffer,_("Width %.10g"),data->linestyle->width);
		PostMessage(buffer);

		needtodraw=1;
		return 0;

	} else if (action==PATHIA_Thinner) {
		if (!data) return 0;
		if (!data->linestyle) data->linestyle=new LineStyle;
		if (!data->linestyle->width) data->linestyle->width=1;
		data->linestyle->width/=widthstep;
		data->Recache();

		defaultweight.width=data->linestyle->width;

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

	} else if (action==PATHIA_ApplyOffset) {
		if (!data) return 0;

		if (curpoints.n==0) {
			data->ApplyOffset(-1);
		} else {
			curpath->ApplyOffset();
		}
		return 0;

	} else if (action==PATHIA_ResetOffset) {
		 //need some way to determine paths of curpoints, and affect only those.
		 //If no curpoints, then reset for all
		if (curpoints.n==0) {
			data->SetOffset(-1,0);
		} else {
			curpath->SetOffset(0);
		}
		return 0;

	} else if (action==PATHIA_ResetAngle) {
		 //need some way to determine paths of curpoints, and affect only those.
		 //If no curpoints, then reset for all
		if (curpoints.n==0) {
			data->SetAngle(-1,0,0);
		} else {
			curpath->SetAngle(0,0);
		}
		needtodraw=1;
		return 0;

	} else if (action==PATHIA_MakeStraight || action==PATHIA_MakeBezStraight) {
		if (!data) return 0;

		bool asbez=(action==PATHIA_MakeBezStraight);

		if (curpoints.n==0) {
			data->MakeStraight(-1,NULL,NULL,asbez);
		} else {
			Coordinate *from,*to, *next, *prev;
			Path *path;
			int i;
			for (int c=0; c<curpoints.n; c++) {
				from=curpoints.e[c];
				if ((from->flags&POINT_VERTEX)==0) continue;

				i=data->hasCoord(from);
				path=data->paths.e[i];
				to=NULL;
				next=from->nextVertex(0);
				prev=from->previousVertex(0);
				for (int c2=c+1; c2<curpoints.n; c2++) {
					if (curpoints.e[c2]==next) {
						to=curpoints.e[c2];
						path->MakeStraight(from,to,asbez);

					} else if (curpoints.e[c2]==prev) {
						to=curpoints.e[c2];
						path->MakeStraight(to,from,asbez);
					}
				}
			}
		}
		clearSelection();
		return 0;

	} else if (action==PATHIA_NewFromStroke) {
		PostMessage(" *** need to implement PATHIA_NewFromStroke!!");
		return 0;

	} else if (action==PATHIA_BreakApart) {
		PostMessage(" *** need to implement PATHIA_BreakApart!!");
//		----------
//		if (data && data->paths.n<=1) return 0;
//		PathsData *newpaths=NULL;
//		int n=data->BreakApart(&newpaths);
//		for (int c=0; c<n; c++) {
//			viewport->NewData(newpaths[c], NULL);
//
//		}
//		*** clear object selection, add data and all in newpaths to new selection
		return 0;

	} else if (action==PATHIA_BreakApartChunks) {
		PostMessage(" *** need to implement PATHIA_BreakApartChunks!!");
		return 0;

	} else if (action == PATHIA_SaveAsShapeBrush) {
		if (!data) return 0;
		if (data->IsEmpty()) {
			PostMessage(_("Shape brushes can't be empty"));
			return 0;
		}
		InterfaceManager *imanager = InterfaceManager::GetDefault(true);
		ResourceManager *rm = imanager->GetResourceManager();

		ShapeBrush *brush = new ShapeBrush();
		brush->CopyFrom(data);
		brush->Id(data->Id());
	
		rm->AddResource("ShapeBrush", brush, nullptr, brush->Id(), brush->Id(), nullptr, nullptr, nullptr, false);
		brush->dec_count();
		return 0;

	}
	
	return 1;
}

/*! Update the viewport color box */
void PathInterface::UpdateViewportColor()
{
	if (!data) return;

	ScreenColor *col;
	if (!data->linestyle) {
		LineStyle *style = dynamic_cast<LineStyle*>(defaultline->duplicate(nullptr));
		data->InstallLineStyle(style);
	}
	if (!data->fillstyle) {
		// ScreenColor col(1.,1.,1.,1.);
		data->fill(&data->linestyle->color);
		data->fillstyle->function = LAXOP_None;
	}
	col = &data->fillstyle->color;
	
	SimpleColorEventData *e=new SimpleColorEventData( 65535, col->red, col->green, col->blue, col->alpha, 0);
	e->colormode = COLOR_StrokeFill;
	e->colorindex = 1;
	app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);

	col = &data->linestyle->color;
	e = new SimpleColorEventData( 65535, col->red, col->green, col->blue, col->alpha, 0);
	e->colormode = COLOR_StrokeFill;
	e->colorindex = 0;
	app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);
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
			curpoints.flush();
			return 0;
		}
		return 1;

	} else if (ch==LAX_Enter) {
		if (owner && (pathi_style&PATHI_Esc_Off_Sub)) {
			owner->RemoveChild();
			curpoints.flush();
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

/*! Convert selected points into a rectangle, removing any control handles, leaving only vertices.
 */
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

	data->Recache();
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

	data->Recache();
    needtodraw=1;
}

/*! Return number of PathsData objects contained in obj.
 * If obj is a GroupData, then also recursively tally all child paths in that.
 * If obj is a SomeDataRef, then check its final object.
 * If num_other != null, add number of non path objects to its existing value.
 */
int GetNumPathsData(SomeData *obj, int *num_other)
{
	int n = 0;
	int other = 0;

	SomeDataRef *ref = dynamic_cast<SomeDataRef*>(obj);
	if (ref) {
		SomeData *o = ref->GetFinalObject();
		if (o) n += GetNumPathsData(o, num_other);
	}

	PathsData *pd = dynamic_cast<PathsData*>(obj);
	if (pd) n++;
	else other++;

	GroupData *group = dynamic_cast<GroupData*>(obj);
	if (group) {
		for (int c=0; c<group->NumKids(); c++) {
			n += GetNumPathsData(group->Child(c), num_other);
		}
	}

	return n;
}

//! Append clipping paths to dp.
/*!
 * Converts a a group of PathsData, a SomeDataRef to a PathsData, 
 * or a single PathsData to a clipping path. The final region is just 
 * the union of all the paths there.
 *
 * Non-PathsData elements in a group does not break the finding.
 * Those extra objects are just ignored.
 *
 * Returns the number of single paths interpreted, or negative number for error.
 *
 * \todo *** currently, uses all points (vertex and control points)
 *   in the paths as a polyline, not as the full curvy business 
 *   that PathsData are capable of. when ps output of paths is 
 *   actually more implemented, this will change..
 * \todo this would be good to transplant into laxkit
 */
int SetClipFromPaths(Laxkit::Displayer *dp, LaxInterfaces::SomeData *outline, const double *extra_m, bool real)
{
	PathsData *path=dynamic_cast<PathsData *>(outline);

	 //If is not a path, but is a reference to a path
	if (!path && dynamic_cast<SomeDataRef *>(outline)) {
		SomeDataRef *ref;
		 // skip all nested SomeDataRefs
		do {
			ref=dynamic_cast<SomeDataRef *>(outline);
			if (ref) outline=ref->thedata;
		} while (ref);
		if (outline) path=dynamic_cast<PathsData *>(outline);
	}

	int n=0; //the number of objects interpreted and that have non-empty paths
	
	 // If is not a path, and is not a ref to a path, but is a group,
	 // then check its elements 
	if (!path && dynamic_cast<GroupData *>(outline)) {
		GroupData *g=dynamic_cast<GroupData *>(outline);
		SomeData *d;
		double m[6];

		for (int c=0; c<g->NumKids(); c++) {
			d=g->Child(c);

			 //add transform of group element
			if (extra_m) transform_mult(m,d->m(),extra_m);
			else transform_copy(m,d->m());

			n+=SetClipFromPaths(dp,d,m, real);
		}
	}
	
	if (!path) {
		return n;
	}

     // finally append to clip path

//	--------------
    Coordinate *start,*p, *p2;
	flatpoint c1,c2;
    flatpoint pp;

    for (int c=0; c<path->paths.n; c++) {
        if (!path->paths.e[c]->path) continue;
		if (!path->paths.e[c]->IsClosed()) continue; // only include closed paths

 
		start = p = path->paths.e[c]->path->firstPoint(1);

		if (extra_m) dp->moveto(transform_point(extra_m, p->p()));
		else dp->moveto(p->p());

		do {
			p2=p->next; //p points to a vertex
			if (!p2) break;

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

				if (extra_m)
					dp->curveto(transform_point(extra_m, c1), transform_point(extra_m, c2), transform_point(extra_m, p2->p()));
				else dp->curveto(c1,c2,p2->p());
			} else {
				 //we do not have control points, so is just a straight line segment
				if (extra_m)
					dp->lineto(transform_point(extra_m, p2->p()));
				else dp->lineto(p2->p());
			}

			p=p2;
		} while (p && p!=start);
		dp->closed();

		n++;
    }

	if (n) {
		if (!real) dp->DrawScreen();
		dp->Clip(true);
		if (!real) dp->DrawReal();
	}

    return n;
}


//-------------------------------- PathUndo ---------------------------------

PathUndo::PathUndo(PathsData *object, int ntype, int nisauto)
 : UndoData(nisauto)
{
	context = object;
	if (object) object->inc_count();

	type = ntype;
}

PathUndo::~PathUndo()
{
	if (path) delete path;
}

const char *PathUndo::Description()
{
	switch (type) {
		case MovePoints:   return _("Move points");
		case DelPoints:    return _("Delete points");
		case AddPoints:    return _("Add points");
		case AddPath:      return _("Add Path");
		case DelPath:      return _("Delete path");
		case Reorder:      return _("Reorder points");
		case ReplacePath:  return _("Replace path");
		case WeightMove:   return _("Move weight");
		case WeightAdd:    return _("Add weight");
		case WeightDel:    return _("Delete weight");
		case WeightValues: return _("Adjust weight");
	}
	return nullptr;
}

/*! Return 0 for sucess, nonzero erro
 */
int PathsData::Undo(UndoData *data)
{
	PathUndo *undo = dynamic_cast<PathUndo*>(data);
	if (!undo) return 1;


	if (undo->type == PathUndo::MovePoints) {
		for (int p=0; p<undo->path_indices.n; p++) {
			Path *path = paths.e[p];

			for (int c=0; c<undo->indices.n; c++) {
				Coordinate *coord = path->path;
				coord = coord->Traverse(undo->indices.e[c]);
				if (!coord) return 1;
				flatpoint curp = coord->p();
				coord->p(undo->points.e[c]);
				undo->points.e[c] = curp;
			}
		}
	
	} else if (undo->type == PathUndo::AddPath) {
		//path was added, need to remove it
		undo->path = paths.pop(undo->path_indices.e[0]);

	} else if (undo->type == PathUndo::DelPath) {
		//path was deleted, need to add it back
		paths.push(undo->path, -1, undo->path_indices.e[0]);
		undo->path = nullptr;

	} else if (undo->type == PathUndo::AddPoints
			|| undo->type == PathUndo::DelPoints
			|| undo->type == PathUndo::ReplacePath
			|| undo->type == PathUndo::WeightMove
			|| undo->type == PathUndo::WeightAdd
			|| undo->type == PathUndo::WeightDel
			|| undo->type == PathUndo::WeightValues
			) {
		// all of these just replace the path because weight maintenance is such a pain
		Path *npath = paths.pop(undo->path_indices.e[0]);
		paths.push(undo->path, undo->path_indices.e[0]);
		undo->path = npath;
	
	// } else if (type == Reorder) {

	} else return 1;

	touchContents();
	return 0;
}

/*! Return 0 for sucess, nonzero erro
 */
int PathsData::Redo(UndoData *data)
{
	PathUndo *undo = dynamic_cast<PathUndo*>(data);
	if (!undo) return 1;

	if (undo->type == PathUndo::MovePoints) {
		for (int p=0; p<undo->path_indices.n; p++) {
			Path *path = paths.e[p];

			for (int c=0; c<undo->indices.n; c++) {
				Coordinate *coord = path->path;
				coord = coord->Traverse(undo->indices.e[c]);
				if (!coord) return 1;
				flatpoint curp = coord->p();
				coord->p(undo->points.e[c]);
				undo->points.e[c] = curp;
			}
		}

	} else if (undo->type == PathUndo::AddPath) {
		//path was removed in undo, need to add it back
		paths.push(undo->path, -1, undo->path_indices.e[0]);
		undo->path = nullptr;

	} else if (undo->type == PathUndo::DelPath) {
		//path was deleted in undo, need to add it back
		undo->path = paths.pop(undo->path_indices.e[0]);

	} else if (undo->type == PathUndo::AddPoints
			|| undo->type == PathUndo::DelPoints
			|| undo->type == PathUndo::ReplacePath
			|| undo->type == PathUndo::WeightMove
			|| undo->type == PathUndo::WeightAdd
			|| undo->type == PathUndo::WeightDel
			|| undo->type == PathUndo::WeightValues
			) {
		// all of these just replace the path because weight maintenance is such a pain
		Path *npath = paths.pop(undo->path_indices.e[0]);
		paths.push(undo->path, -1, undo->path_indices.e[0]);
		undo->path = npath;

	// } else if (undo->type == Reorder) {

	} else return 1;

	touchContents();
	return 0;
}



} // namespace LaxInterfaces

