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
//
//  ---- Delaunay Triangulation below:
//  Adapted from Paul Bourke's C implementation 



#include "delaunayinterface.h"

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/linestyle.h>
#include <lax/filedialog.h>
#include <lax/laxutils.h>
#include <lax/utf8string.h>
#include <lax/language.h>


//You need this if you use any of the Laxkit stack templates in lax/lists.h
#include <lax/lists.cc>


using namespace Laxkit;
using namespace LaxFiles;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//forward declarations...
int DelaunayTriangulate(flatpoint *pts, int nv, IndexTriangle *tri_ret, int *ntri_ret);
int DelaunayTriangulate(PointSet::PointObj **pts, int nv, IndexTriangle *tri_ret, int *ntri_ret);


//------------------------------- Point list generators ---------------------------------

flatpoint *GenerateRandomRectPoints(int seed, flatpoint *points, int n, double minx, double maxx, double miny, double maxy)
{
	if (!points) points = new flatpoint[n];

	if (seed) srandom(seed);

	for (int c=0; c<n; c++) {
		points[c] = flatpoint(minx + (maxx-minx)*(double)random()/RAND_MAX, miny + (maxy-miny)*(double)random()/RAND_MAX);
	}

	return points;
}

flatpoint *GenerateGridPoints(flatpoint *p, int numx, int numy, double minx, double maxx, double miny, double maxy)
{
	if (!p) p = new flatpoint[numx*numy];

	int i = 0;
	double xx, yy;
	double dx = (maxx-minx)/numx;
	double dy = (maxy-miny)/numy;

	yy = miny;
	for (int y=0; y<numy; y++) {
	  xx = minx;
	  for (int x=0; x<numx; x++) {
		p[i].set(xx, yy);
		xx += dx;
	  }
	  yy += dy;
	}

	return p;
}

/*! Return an array of random doubles. If seed!=0, use that for srandom().
 * If d==NULL, then return a new double[n]. Else return d, which is assumed to be able to hold n doubles.
 */
double *RandomDoubles(int seed, double *d, int n, double min, double max)
{
	if (seed) srandom(seed);

	if (!d) d = new double[n];
	for (int c=0; c<n; c++) d[0] = min + (max-min)*((double)random()/RAND_MAX);

	return d;
}

/*! Return an array of random ints. If seed!=0, use that for srandom().
 * If a==NULL, then return a new int[n]. Else return d, which is assumed to be able to hold n ints.
 */
int *RandomInts(int seed, int *a, int n, int min, int max)
{
	if (seed) srandom(seed);

	if (!a) a = new int[n];
	for (int c=0; c<n; c++) a[c] = min + (double)random()/RAND_MAX*(max-min) + .5;

	return a;
}


//------------------------------- VoronoiData ---------------------------------

/*! \class VoronoiData
 *
 * Class to simplify building and computing simple Delaunay triangulation and Voronoi nets.
 */

VoronoiData::VoronoiData()
{
	show_points  =true;
	show_delaunay=true;
	show_voronoi =true;
	show_numbers =false;

	color_delaunay=new Color();  color_delaunay->screen.rgbf(1.0,0.0,0.0);
	color_voronoi =new Color();  color_voronoi ->screen.rgbf(0.0,0.7,0.0);
	color_points  =new Color();  color_points  ->screen.rgbf(1.0,0.0,1.0); 

	//color_delaunay=CreateColor_RGB(1.0,0.0,0.0);
	//color_voronoi =CreateColor_RGB(0.0,0.7,0.0);
	//color_points  =CreateColor_RGB(1.0,0.0,1.0); 

	width_delaunay=1/10.;
	width_voronoi=1/10.;
	width_points=1/10.;
}

VoronoiData::~VoronoiData()
{
	color_delaunay->dec_count();
	color_voronoi ->dec_count();
	color_points  ->dec_count();
}

void VoronoiData::FindBBox()
{
	DoubleBBox::ClearBBox();
	for (int c=0; c<points.n; c++) {
		addtobounds(points.e[c]->p);
	}
}

void VoronoiData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
    char spc[indent+1];
    memset(spc,' ',indent);
    spc[indent]='\0';

	if (what==-1) {
		Attribute att;
		dump_out_atts(&att, -1, context);
		att.dump_out(f, indent);
		return;
	}

    const double *matrix=m();
    fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);

	fprintf(f,"%sshow_points   %s\n", spc, (show_points   ? "yes" : "no"));
	fprintf(f,"%sshow_delaunay %s\n", spc, (show_delaunay ? "yes" : "no"));
	fprintf(f,"%sshow_voronoi  %s\n", spc, (show_voronoi  ? "yes" : "no"));
	fprintf(f,"%sshow_numbers  %s\n", spc, (show_numbers  ? "yes" : "no"));

	fprintf(f,"%swidth_points   %.10g\n", spc, width_points  );
	fprintf(f,"%swidth_delaunay %.10g\n", spc, width_delaunay);
	fprintf(f,"%swidth_voronoi  %.10g\n", spc, width_voronoi );

	char *col=color_delaunay->dump_out_simple_string();
	if (col) fprintf(f,"%scolor_delaunay %s\n", spc, col);
	delete[] col;

	col=color_voronoi->dump_out_simple_string();
	if (col) fprintf(f,"%scolor_voronoi  %s\n", spc, col);
	delete[] col;

	col=color_points->dump_out_simple_string();
	if (col) fprintf(f,"%scolor_points   %s\n", spc, col);
	delete[] col;
	

	if (points.n) {
		fprintf(f,"%spoints \\\n",spc);
		for (int c=0; c<points.n; c++) {
			fprintf(f,"%s  %.10g, %.10g  #%d\n", spc, points.e[c]->p.x,points.e[c]->p.y, c);
		}
	}

	if (triangles.n) {
		fprintf(f,"%striangles \\ #(ignored on loading) p1 p2 p3  t1 t2 t3 (<- the triangles on other side of edge)  circumcenter x,y\n",spc);
		for (int c=0; c<triangles.n; c++) {
			fprintf(f,"%s  %d %d %d  %d %d %d  %.10g, %.10g  #%d\n", spc, 
					triangles.e[c].p1,   triangles.e[c].p2,   triangles.e[c].p3,
					triangles.e[c].t[0], triangles.e[c].t[1], triangles.e[c].t[2],
					triangles.e[c].circumcenter.x,triangles.e[c].circumcenter.y,
					c);
		}
	}

}

LaxFiles::Attribute *VoronoiData::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att = new Attribute();

	if (what==-1) {
		att->push("matrix", "1 0 0 1 0 0");
			
		att->push("show_points",    "yes", "or no");
		att->push("show_delaunay",  "yes", "or no");
		att->push("show_voronoi",   "yes", "or no");
		att->push("show_numbers",   "yes", "or no");

		att->push("width_points",   ".1", "Default radius of point indicators" );
		att->push("width_delaunay", ".1", "Line width of Delaunay triangles" );
		att->push("width_voronoi",  ".1", "Line width of Voronoi cells" );

		att->push("color_delaunay", "rgbf(1.0,0.0,0.0)");
		att->push("color_voronoi",  "rgbf(0.0,0.7,0.0)");
		att->push("color_points",   "rgbf(1.0,0.0,1.0)");
		
		att->push("points", "1.0, 1.0\n...", "List of points");

		att->push("triangles", "0 1 2  3 4 5 .5 .5\n...", "(ignored on loading) p1 p2 p3  t1 t2 t3 (<- the triangles on other side of edge)  circumcenter x,y");

		return att;
	}

    const double *matrix=m();
    att->pushStr("matrix", -1, "%.10g %.10g %.10g %.10g %.10g %.10g",
			matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);

	att->push("show_points",    (show_points   ? "yes" : "no"));
	att->push("show_delaunay",  (show_delaunay ? "yes" : "no"));
	att->push("show_voronoi",   (show_voronoi  ? "yes" : "no"));
	att->push("show_numbers",   (show_numbers  ? "yes" : "no"));

	att->push("width_points",   width_points  );
	att->push("width_delaunay", width_delaunay);
	att->push("width_voronoi",  width_voronoi );

	char *col=color_delaunay->dump_out_simple_string();
	if (col) att->push("color_delaunay", col);
	delete[] col;

	col=color_voronoi->dump_out_simple_string();
	if (col) att->push("color_voronoi", col);
	delete[] col;

	col=color_points->dump_out_simple_string();
	if (col) att->push("color_points", col);
	delete[] col;
	

	Utf8String s, s2;
	if (points.n) {
		Attribute *att2 = att->pushSubAtt("points");
		for (int c=0; c<points.n; c++) {
			s2.Sprintf("%.10g, %.10g\n", points.e[c]->p.x,points.e[c]->p.y); //oh no! can't really output per line comments with atts!
			s.Append(s2);
		}
		att2->value = s.ExtractBytes(nullptr,nullptr,nullptr);
	}

	if (triangles.n) {
		Attribute *att2 = att->pushSubAtt("triangles", nullptr, "(ignored on loading) p1 p2 p3  t1 t2 t3 (<- the triangles on other side of edge)  circumcenter x,y\n");
		for (int c=0; c<triangles.n; c++) {
			s.Sprintf("%d %d %d  %d %d %d  %.10g, %.10g  #%d\n", 
					triangles.e[c].p1,   triangles.e[c].p2,   triangles.e[c].p3,
					triangles.e[c].t[0], triangles.e[c].t[1], triangles.e[c].t[2],
					triangles.e[c].circumcenter.x,triangles.e[c].circumcenter.y
				);
			att2->value = s.ExtractBytes(nullptr,nullptr,nullptr);
		}
	}

	return att;
}

void VoronoiData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
    if (!att) return;
    char *name,*value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"matrix")) {
			double mm[6];
            if (DoubleListAttribute(value,mm,6)==6) m(mm);

        } else if (!strcmp(name,"show_points"  )) {
			show_points  =BooleanAttribute(value);

        } else if (!strcmp(name,"show_delaunay")) {
			show_delaunay=BooleanAttribute(value);

        } else if (!strcmp(name,"show_voronoi" )) {
			show_voronoi =BooleanAttribute(value);

        } else if (!strcmp(name,"show_numbers" )) {
			show_numbers =BooleanAttribute(value);

        } else if (!strcmp(name,"width_points"  )) {
			DoubleAttribute(value, &width_points, NULL);

        } else if (!strcmp(name,"width_delaunay")) {
			DoubleAttribute(value, &width_delaunay, NULL);

        } else if (!strcmp(name,"width_voronoi" )) {
			DoubleAttribute(value, &width_voronoi, NULL);

        } else if (!strcmp(name,"points")) {
			points.flush();

			double x;
			flatpoint p;
			while (1) {
				DoubleAttribute(value,&x,&name);
				if (name!=value) {
					p.x=x;
					while (isspace(*name) || *name==',') name++;
					DoubleAttribute(name,&p.y,&value);
					if (value==name) break;

					AddPoint(p);
				} else break;
			}

        }
    }

	RebuildVoronoi(true);
}

/*! Set the width of the lines or points.
 * which<0 sets default for all.
 * &1 is for voronoi lines,
 * &2 is for delaunay lines,
 * &3 is for points.
 */
void VoronoiData::Width(double newwidth, int which)
{
	if (which<0 || which&1) width_voronoi  = newwidth;
	if (which<0 || which&2) width_delaunay = newwidth;
	if (which<0 || which&4) width_points   = newwidth;
}

void VoronoiData::RelaxBarycenter(int iters, double strength, DoubleBBox box)
{
	// *** TODO implement within box, needs to add extra points to construct boundary

	if (NumPoints() < 3) return;

	if (!triangles.n) {
		RebuildVoronoi(true);
	}

	NumStack<flatpoint> barycenters;
	for (int c=0; c<points.n; c++) barycenters.push(flatpoint()); //initiate proper size
	int is_inf = 0;

	flatpoint p;
	for (int i=0; i<iters; i++) {
		for (int c=0; c<points.n; c++) {
			barycenters.e[c] = BarycenterRegion(c, &is_inf);
		}

		double smallestdist = 1000000;
		for (int c = 0; c < points.n; c++) {
			if (barycenters.e[c].info != 0) continue; //leave alone points of infinite regions

			flatpoint v = barycenters.e[c] - points.e[c]->p;
			double l = v.norm2();
			if (l < smallestdist) smallestdist = l;
			points.e[c]->p += v * strength;
		}
		RebuildVoronoi(true);
	}
}

/*! Return the centroid of the specified triangle.
 * If triangle invalid index, return (0,0).
 */
flatpoint VoronoiData::Centroid(int triangle)
{
	if (triangle<0 || triangle>=triangles.n) return flatpoint(0,0);
	return ( points.e[triangles.e[triangle].p1]->p
			+points.e[triangles.e[triangle].p2]->p
			+points.e[triangles.e[triangle].p3]->p) / 3;
}

/*! Return centroid of specified index of voronoi region.
 * Note infinite regions will set is_inf to 1, else it is 0.
 * If 1, the returned point contains average of all the non-infinite points, and its info will we 1.
 *
 * If point is a bad index, is_inf is set to -1.
 */
flatpoint VoronoiData::BarycenterRegion(int point, int *is_inf)
{
	if (is_inf) *is_inf = 0;
	if (point < 0 || point >= regions.n) {
		if (is_inf) *is_inf = -1;
		return flatpoint();
	}

	flatpoint pp;
	int noninf = 0;
	for (int c=0; c<regions.e[point].tris.n; c++) {
		if (regions.e[point].tris.e[c] < 0) {
			if (is_inf) *is_inf = 1;
		} else {
			noninf++;
			int i = regions.e[point].tris.e[c];
			pp += triangles.e[i].circumcenter;
		}
	}

	if (noninf > 0) pp /= noninf;
	if (noninf != regions.e[point].tris.n) pp.info = 1;
	return pp;
}

void VoronoiData::Triangulate()
{
	if (points.n<3) return;

	triangles.flush_n();
	triangles.Allocate(3*points.n);
	//triangles.flush();
	//triangles.Allocate(3*points.n);

	DelaunayTriangulate(points.e,points.n, triangles.e,&triangles.n);
	FindBBox();

	 //reset triangle links
	for (int c=0; c<triangles.n; c++) {
		triangles.e[c].t[0]=triangles.e[c].t[1]=triangles.e[c].t[2]=-1;
	}

	 //find triangle links
	IndexTriangle *tri;
	int e;
	for (int c=0; c<triangles.n; c++) {
		tri=&triangles.e[c];

		 //find triangle connections, the triangle on other side of edges
		if (tri->t[0]==-1) {
			for (int c2=c+1; c2<triangles.n; c2++) {
				e=triangles.e[c2].HasCCWEdge(tri->p1,tri->p2);
				if (e) {
					tri->t[0]=c2;
					triangles.e[c2].t[e-1]=c;
				}
			}
		}

		 //find t2, the triangle on other side of p2-p3 edge
		if (tri->t[1]==-1) {
			for (int c2=c+1; c2<triangles.n; c2++) {
				e=triangles.e[c2].HasCCWEdge(tri->p2,tri->p3);
				if (e) {
					tri->t[1]=c2;
					triangles.e[c2].t[e-1]=c;
				}
			}
		}

		 //find t3, the triangle on other side of p3-p1 edge
		if (tri->t[2]==-1) {
			for (int c2=c+1; c2<triangles.n; c2++) {
				e=triangles.e[c2].HasCCWEdge(tri->p3,tri->p1);
				if (e) {
					tri->t[2]=c2;
					triangles.e[c2].t[e-1]=c;
				}
			}
		}
	}
}

/*! If triangulate_also, call Triangulate() first. Otherwise, assume that has already been called,
 * thus each triangle has valid triangle links and circumcenter points.
 */
void VoronoiData::RebuildVoronoi(bool triangulate_also)
{
	if (triangulate_also) Triangulate();


	inf_points.flush();
	regions.flush();
	regions.Allocate(points.n);
	regions.n=points.n;

	IndexTriangle *tri;
	VoronoiRegion *region;
	int pos;
	int first=-1; //index in triangles
	int ntri, curtri;
	flatpoint v;

	for (int c=0; c<points.n; c++) {
		region            = &regions.e[c];
		region->point     = points.e[c]->p;
		region->next_hull = -1;
		region->tris.flush();

		// find a triangle that has the point
		pos=-1;
		for (int c2=0; c2<triangles.n; c2++) {
			pos=triangles.e[c2].Has(c);
			if (pos) {
				tri=&triangles.e[c2];
				region->tris.push(c2);
				first=c2;
				curtri=c2;
				break;
			}
		}
		if (pos<=0) break; //error! no triangle has point!

		 //find next triangles, going clockwise
		while (1) {
			ntri=tri->t[pos-1];

			if (ntri<0) {
				 //hull edge, need to somehow add infinite ray
				//region->next_hull=tri->p[pos%3];
				region->tris.push(-inf_points.n-1);

				if (pos==1)      v = points.e[triangles.e[curtri].p2]->p - points.e[triangles.e[curtri].p1]->p;
				else if (pos==2) v = points.e[triangles.e[curtri].p3]->p - points.e[triangles.e[curtri].p2]->p;
				else             v = points.e[triangles.e[curtri].p1]->p - points.e[triangles.e[curtri].p3]->p;
				v=transpose(v);
				v.normalize();

				inf_points.push(triangles.e[region->tris.e[region->tris.n-2]].circumcenter + (maxx-minx + maxy-miny)*v);

				break;
			}

			if (ntri==first) break; //found full region, hooray!

			curtri=ntri;
			tri=&triangles.e[ntri];
			region->tris.push(ntri);
			pos=tri->Has(c);
		}

		 //find prev triangles, if necessary. This happens only when point is on the hull
		if (ntri!=first) { //ntri will be -1
			tri=&triangles.e[first];
			curtri=first;
			pos=triangles.e[curtri].Has(c);

			while (1) {
				ntri=tri->t[(pos-2+3)%3];

				if (ntri<0) {
					 //hull edge, need to somehow add infinite ray
					region->tris.push(-inf_points.n-1, 0);

					if (pos==1)      v = points.e[triangles.e[curtri].p3]->p - points.e[triangles.e[curtri].p1]->p;
					else if (pos==2) v = points.e[triangles.e[curtri].p1]->p - points.e[triangles.e[curtri].p2]->p;
					else             v = points.e[triangles.e[curtri].p2]->p - points.e[triangles.e[curtri].p3]->p;
					v=-transpose(v);
					v.normalize();

					inf_points.push(triangles.e[region->tris.e[1]].circumcenter + (maxx-minx + maxy-miny)*v);

					break;
				}

				if (ntri==first) break; //found full region, shouldn't actually happen

				curtri=ntri;
				tri=&triangles.e[ntri];
				region->tris.push(ntri,0);
				pos=tri->Has(c);
			}
		}


		//*** construct flatpoint[] with info about if the point is for infinite ray..
		//    having separate array from tris makes it slightly faster to do mouse over

		//*****
	}
}

//int VoronoiData::FindNextTri(int p1,int p2)
//{
//	for (int c=0; c<triangles.n; c++) {
//		if (triangles.e[c]->HasCCWEdge(p1,p2)) return c;
//	}
//	return -1;
//}

int VoronoiData::Map(std::function<int(const flatpoint &p, flatpoint &newp)> adjustFunc)
{
	int n = PointSet::Map(adjustFunc);
	// int n = 0;
	// flatpoint p;
	// for (int c=0; c<points.n; c++) {
	// 	if (adjustFunc(points.e[c]->p, p)) {
	// 		points.e[c]->p = p;
	// 		n++;
	// 	}
	// }
	RebuildVoronoi(true);
	return n;
}

void VoronoiData::Flush()
{
	PointSet::Flush();
	triangles.flush();
	regions.flush();
	inf_points.flush();
	FindBBox();
}

SomeData *VoronoiData::duplicate(SomeData *dup)
{
	VoronoiData *newobj = dynamic_cast<VoronoiData*>(dup);
	if (!newobj && dup) return nullptr; //was not a VoronoiData!

	if (!dup) {
		dup = dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_VORONOIDATA));
		newobj = dynamic_cast<VoronoiData*>(dup);
	}
	if (!newobj) {
		newobj = new VoronoiData();
		dup = newobj;
	}

	for (int c=0; c<points.n; c++) newobj->AddPoint(points.e[c]->p);

	newobj->containing_rect = containing_rect;

	newobj->width_delaunay = width_delaunay;
	newobj->width_voronoi = width_voronoi;
	newobj->width_points = width_points;

	newobj->show_points = show_points;
	newobj->show_delaunay = show_delaunay;
	newobj->show_voronoi = show_voronoi;
	newobj->show_numbers = show_numbers;

	newobj->color_delaunay->dec_count();
	newobj->color_voronoi->dec_count();
	newobj->color_points->dec_count();
	newobj->color_delaunay = color_delaunay->duplicate();
	newobj->color_voronoi = color_voronoi->duplicate();
	newobj->color_points = color_points->duplicate();

	 //somedata elements:
	dup->setbounds(minx,maxx,miny,maxy);
	dup->bboxstyle = bboxstyle;
	dup->m(m());

	RebuildVoronoi(true);
	return dup;
}


//------------------------------- DelaunayInterface ---------------------------------

/*! \class DelaunayInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


DelaunayInterface::DelaunayInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	delaunay_interface_style=0;
	num_random = 20;
	num_x = 5;
	num_y = 5;
	poisson_size = .25;
	previous_create = VORONOI_MakeRandomRect;
	relax_iters = 1;
	
	show_numbers = false; 
	show_arrows  = false;
	show_lines   = 3;

	showdecs     = 1;
	needtodraw   = 1;
	curpoint     = -1;
	justadded    = false;
	style_target = 0; //0 is voronoi border, 1 is delaunay tri edges, 2 is points
	last_export  = NULL;

	sc   = NULL;
	data = NULL;
	voc  = NULL;
	
	last_export=newstr("voronoi.data");
}

DelaunayInterface::~DelaunayInterface()
{
	if (sc)   sc->dec_count();
	if (data) data->dec_count();
	if (voc)  delete voc;
	delete[] last_export;
}

const char *DelaunayInterface::whatdatatype()
{  
	return "VoronoiData";
	//return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *DelaunayInterface::Name()
{ return _("Delaunay/Voronoi"); }


//! Return new DelaunayInterface.
/*! If dup!=NULL and it cannot be cast to DelaunayInterface, then return NULL.
 */
anInterface *DelaunayInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new DelaunayInterface(NULL,id,NULL);
	else if (!dynamic_cast<DelaunayInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

void DelaunayInterface::Clear(SomeData *d)
{
	if (!d || d==data) {
		if (data) data->dec_count();
		data = NULL;
		if (voc) delete voc;
		voc=NULL;
	}

	curpoint = -1;
}

int DelaunayInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<VoronoiData *>(ndata)==NULL) return 1;

	VoronoiData *bzd=data;
	data=dynamic_cast<VoronoiData *>(ndata);

	int tcurpoint=curpoint;
	int td=showdecs, ntd=needtodraw;
	int tshow_lines  =show_lines;
	int tshow_arrows =show_arrows;
	int tshow_numbers=show_numbers;
	curpoint=-1;
	showdecs=0;
	needtodraw=1;

	show_lines = (data->show_delaunay ? 2 : 0) | (data->show_voronoi ? 1 : 0);
	show_numbers = false;
	//show_numbers = data->show_numbers;

	Refresh();

	curpoint=tcurpoint;
	show_lines  =tshow_lines;
	show_arrows =tshow_arrows;
	show_numbers=tshow_numbers;
	showdecs=td;
	data=bzd;
	needtodraw=ntd;
	return 1;
}

int DelaunayInterface::Refresh()
{ 
	if (needtodraw==0) return 0;
	needtodraw=0;

	if (!data) return 0;

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	if (curwindow) dp->font(curwindow->win_themestyle->normal, curwindow->win_themestyle->normal->textheight() / dp->Getmag());


	 //delaunay triangles
	dp->LineWidth(data->width_delaunay);
	flatpoint center,p,v;

	dp->NewFG(data->color_delaunay);
	for (int c=0; c<data->triangles.n; c++) {
		 //draw edges
		if (data->show_delaunay) {
			dp->moveto(data->points.e[data->triangles[c].p1]->p);
			dp->lineto(data->points.e[data->triangles[c].p2]->p);
			dp->lineto(data->points.e[data->triangles[c].p3]->p);
			dp->closed();
			dp->stroke(0);
		}

		 //draw arrows indicating edge direction
		center = (data->points.e[data->triangles[c].p1]->p
				+ data->points.e[data->triangles[c].p2]->p
				+ data->points.e[data->triangles[c].p3]->p)/3;
		if (show_numbers) dp->drawnum(center.x,center.y, c);

		v = .2*(data->points.e[data->triangles[c].p2]->p - data->points.e[data->triangles[c].p1]->p);
		p = data->points.e[data->triangles[c].p1]->p;
		p = p+.3*(center-p);
		if (show_arrows) dp->drawarrow(p,v, 0,1,2,3);
		if (show_numbers) dp->textout(p.x,p.y, "1,",1, LAX_HCENTER);

		v=.2*(data->points.e[data->triangles[c].p3]->p - data->points.e[data->triangles[c].p2]->p);
		p=data->points.e[data->triangles[c].p2]->p;
		p=p+.3*(center-p);
		if (show_arrows) dp->drawarrow(p,v, 0,1,2,3);
		if (show_numbers) dp->textout(p.x,p.y, "2,",1, LAX_CENTER);

		v=.2*(data->points.e[data->triangles[c].p1]->p - data->points.e[data->triangles[c].p3]->p);
		p=data->points.e[data->triangles[c].p3]->p;
		p=p+.3*(center-p);
		if (show_arrows) dp->drawarrow(p,v, 0,1,2,3);
		if (show_numbers) dp->textout(p.x,p.y, "3,",1, LAX_CENTER);
	}



//	 //hull edges
//	if (data->show_hull) {
//		dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
//		IndexTriangle *tri;
//		for (int c=0; c<data->triangles.n; c++) {
//			tri=&data->triangles.e[c];
//			if (tri->t[0]<0) {
//				dp->drawline(data->points.e[tri->p1]->p, data->points.e[tri->p2]->p);
//			}
//			if (tri->t[1]<0) {
//				dp->drawline(data->points.e[tri->p2]->p, data->points.e[tri->p3]->p);
//			}
//			if (tri->t[2]<0) {
//				dp->drawline(data->points.e[tri->p3]->p, data->points.e[tri->p1]->p);
//			}
//		}
//		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
//	}


	 //points
	 if (data->show_points) {
		dp->NewFG(data->color_points);
		for (int c=0; c<data->points.n; c++) {
			double r=(c==curpoint?2:1)*data->width_points;
			//dp->drawpoint(data->points.e[c]->p, (c==curpoint?2:1)*data->width_points, 1);
			dp->drawellipse(data->points.e[c]->p, r,r, 0,2*M_PI, 1);


			if (show_numbers) {
				dp->drawnum(data->points.e[c]->p.x,data->points.e[c]->p.y, c);
			}

			if (c==curpoint && data->regions.n) {
				 //highlight this voronoi cell
				if (data->regions.e[c].tris.n==0) continue;

				dp->LineWidth(2*data->width_voronoi);
				int i=data->regions.e[c].tris.e[0];
				if (i>=0) dp->moveto(data->triangles.e[i].circumcenter);
				else dp->lineto(data->inf_points.e[-i-1]);

				for (int c2=1; c2<data->regions.e[c].tris.n; c2++) {
					i=data->regions.e[c].tris.e[c2];
					if (i>=0) dp->lineto(data->triangles.e[i].circumcenter);
					else dp->lineto(data->inf_points.e[-i-1]);
				}
				dp->closed();
				dp->stroke(0);
			}
		}
	}


	 //voronoi lines
	// if (show_lines&1) {
	if (data->show_voronoi) {
		dp->NewFG(data->color_voronoi);
		dp->LineWidth(data->width_voronoi);
		int i;
		for (int c=0; c<data->regions.n; c++) {
			if (data->regions.e[c].tris.n==0) continue;

			i=data->regions.e[c].tris.e[0];
			if (i>=0) dp->moveto(data->triangles.e[i].circumcenter);
			else dp->lineto(data->inf_points.e[-i-1]);

			for (int c2=1; c2<data->regions.e[c].tris.n; c2++) {
				i=data->regions.e[c].tris.e[c2];
				if (i>=0) dp->lineto(data->triangles.e[i].circumcenter);
				else dp->lineto(data->inf_points.e[-i-1]);
			}
			dp->stroke(0);
		}
	}


	//dp->DrawReal();

	return 0;
}

ObjectContext *DelaunayInterface::Context()
{
	return voc;
}

Laxkit::MenuInfo *DelaunayInterface::ContextMenu(int x,int y,int deviceid, MenuInfo *menu)
{
	if (!menu) menu = new MenuInfo();

	menu->AddItem(_("Make random points"), VORONOI_MakeRandomRect);
	menu->AddItem(_("Make random Poisson points"), VORONOI_MakePoisson);
	menu->AddItem(_("Make random points in circle"), VORONOI_MakeRandomCircle);
	menu->AddItem(_("Make grid"), VORONOI_MakeGrid);
	menu->AddItem(_("Make tri grid in hexagon"), VORONOI_MakeHexChunk);

	if (data) {
		menu->AddSep();
		menu->AddToggleItem(_("Show voronoi shapes"), VORONOI_ToggleVoronoi, 0, data->show_voronoi);
		menu->AddToggleItem(_("Show triangles"),      VORONOI_ToggleShapes,  0, data->show_delaunay);
		menu->AddToggleItem(_("Show points"),         VORONOI_TogglePoints,  0, data->show_points);
		menu->AddSep();
		menu->AddItem(_("Relax"), VORONOI_Relax);
		menu->AddItem(_("Relax forces"), VORONOI_RelaxForce);
		menu->AddSep();
		menu->AddItem(_("New"), VORONOI_New);
	}

	return menu;
}

int DelaunayInterface::InterfaceOn()
{
    needtodraw=1;
    DBG cerr <<"Delaunay On()"<<endl;
    return 0;
}

int DelaunayInterface::InterfaceOff()
{
    Clear(NULL);
    needtodraw=1;
    DBG cerr <<"Delaunay Off()"<<endl;
    return 0;
}

//! Start a new freehand line.
int DelaunayInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	justadded = false;

	if (curpoint<0) {
		if (!data) {
			data=dynamic_cast<VoronoiData *>(somedatafactory()->NewObject(LAX_VORONOIDATA));
			if (!data) data=new VoronoiData; 

			viewport->ChangeContext(x,y,NULL);
			ObjectContext *oc=NULL;
			viewport->NewData(data,&oc);//viewport adds only its own counts
			if (voc) { delete voc; voc=NULL; }
			if (oc) voc=oc->duplicate();

		}

		curpoint=data->points.n;
		justadded=true;
		data->AddPoint(data->transformPointInverse(screentoreal(x,y)));
		data->touchContents();
		Triangulate();
	}

	buttondown.down(d->id,LEFTBUTTON,x,y, curpoint);


	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Finish a new freehand line by calling newData with it.
int DelaunayInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	if (!data) return 0;

	int point = -1;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &point);

	if (!justadded && point == curpoint) {
		if (dragged<3 && curpoint>=0) { data->Remove(curpoint); data->touchContents(); }
		Triangulate();
	}


	needtodraw=1;
	justadded=false;

	return 0; //return 0 for absorbing event, or 1 for ignoring
}



/*! \todo *** this isn't very sophisticated, for elegance, should use some kind of 
 * bez curve fitting to cut down on unnecessary points should use a timer so 
 * stopping makes sharp corners and closer spaced points?
 */
int DelaunayInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *m)
{
	move_pos.set(x,y);

	if (!data) return 0;

	if (!buttondown.any()) {
		// update any mouse over state
		int oldcp=curpoint;
		int c;
		for (c=0; c<data->points.n; c++) {
			if (realtoscreen(data->transformPoint(data->points.e[c]->p)).distanceTo(flatpoint(x,y))<10) break;
		}
		if (c!=data->points.n) curpoint=c;
		else curpoint=-1;

		if (curpoint!=oldcp) needtodraw=1;
		return 1;
	}

	if (curpoint<0) return 0;

	int lx,ly;
	buttondown.move(m->id, x,y, &lx,&ly);

	flatpoint d=data->transformPointInverse(screentoreal(x,y))-data->transformPointInverse(screentoreal(lx,ly));
	data->points.e[curpoint]->p += d;
	Triangulate();
	data->touchContents();

	needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

//! Use the object at oc if it is an ImageData.
int DelaunayInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	VoronoiData *ndata=dynamic_cast<VoronoiData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) {
		data->dec_count();
		data=NULL;
	}
	if (voc) delete voc;
	voc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}

	show_lines = (data->show_delaunay ? 2 : 0) | (data->show_voronoi ? 1 : 0);
	show_numbers = data->show_numbers;

	//SimpleColorEventData *e=new SimpleColorEventData( 65535, 0xffff*data->red, 0xffff*data->green, 0xffff*data->blue, 0xffff*data->alpha, 0);
	//app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);

	needtodraw=1;
	return 1;
}

int DelaunayInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask)
{
    if (!nobj) return 1;

    if (data && dynamic_cast<LineStyle *>(nobj)) {
        //DBG cerr <<"Delaunay new color stuff"<< endl;

        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);

        if (nlinestyle->mask & (LINESTYLE_Color | LINESTYLE_Color2)) {
			Color *color=NULL;
			if (style_target==0)      color = data->color_delaunay;
			else if (style_target==1) color = data->color_voronoi;
			else if (style_target==2) color = data->color_points;

            color->screen.red  =nlinestyle->color.red;
            color->screen.green=nlinestyle->color.green;
            color->screen.blue =nlinestyle->color.blue;
            color->screen.alpha=nlinestyle->color.alpha;

            needtodraw=1;
        }

        return 1;
    }


    return 0;

}


Laxkit::ShortcutHandler *DelaunayInterface::GetShortcuts()
{
    if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    sc=new ShortcutHandler(whattype());

    sc->Add(VORONOI_ToggleNumbers, 'n',0,0,          "ToggleNumbers",  _("Toggle numbers"),NULL,0);
    sc->Add(VORONOI_ToggleArrows,  'a',0,0,          "ToggleArrows",   _("Toggle arrows"),NULL,0);
    sc->Add(VORONOI_ToggleLines,   'l',0,0,          "ToggleLines",    _("Toggle lines"),NULL,0);
    sc->Add(VORONOI_StyleTarget,   'c',0,0,          "StyleTarget",    _("Change target for color and line width changes"),NULL,0); 
    sc->Add(VORONOI_Thicken,       'w',0,0,          "Thicken",        _("Thicken style target"),NULL,0);
    sc->Add(VORONOI_Thin,          'W',ShiftMask,0,  "Thin",           _("Thin style target"),NULL,0);
    sc->Add(VORONOI_FileExport,    'f',0,0,          "FileOut",        _("Export this point set to a file"),NULL,0); 
    sc->Add(VORONOI_FileImport,    'i',0,0,          "FileIn",         _("Import a point set from a file"),NULL,0); 
    sc->Add(VORONOI_RepeatLast,    'r',0,0,          "RepeatLast",     _("Repeat last generator"),NULL,0); 
    sc->Add(VORONOI_Relax,         'R',ShiftMask,0,             "Relax",          _("Relax points"),NULL,0); 
    sc->Add(VORONOI_RelaxForce,    'R',ShiftMask|ControlMask,0, "RelaxForce",     _("Relax points using forces between points"),NULL,0); 

    manager->AddArea(whattype(),sc);
	return sc;
}

int DelaunayInterface::PerformAction(int action)
{
    if (action==VORONOI_ToggleNumbers) {
		show_numbers=!show_numbers;
		needtodraw=1;
		return 0;
		
	} else if (action==VORONOI_ToggleArrows) {
		show_arrows=!show_arrows;
		needtodraw=1;
		return 0; 

	} else if (action==VORONOI_ToggleLines) {
		show_lines++;
		if (show_lines>3) show_lines=0;
		if (data) {
			data->show_delaunay = ((show_lines&2) != 0);
			data->show_voronoi = ((show_lines&1) != 0);
		}
		if      (show_lines == 0) PostMessage(_("Don't show shapes"));
		else if (show_lines == 1) PostMessage(_("Show voronoi shapes"));
		else if (show_lines == 2) PostMessage(_("Show delaunay triangles"));
		else if (show_lines == 3) PostMessage(_("Show voronoi and delaunay shapes"));
		needtodraw=1;
		return 0; 

	} else if (action==VORONOI_TogglePoints) {
		if (!data) return 0;
		data->show_points = !data->show_points;
		needtodraw = 0;
		return 0;

	} else if (action==VORONOI_ToggleVoronoi) {
		if (!data) return 0;
		data->show_voronoi = !data->show_voronoi;
		needtodraw = 0;
		return 0;

	} else if (action==VORONOI_ToggleShapes) {
		if (!data) return 0;
		data->show_delaunay = !data->show_delaunay;
		needtodraw = 0;
		return 0;

	} else if (action==VORONOI_New) {
		Clear(nullptr);
		return 0;

	} else if (action==VORONOI_StyleTarget) {
		style_target++;
		if (style_target>=3) style_target=0;
		if (style_target==0) PostMessage(_("Adjust style of voronoi lines"));
		else if (style_target==1) PostMessage(_("Adjust style of delaunay lines"));
		else if (style_target==2) PostMessage(_("Adjust style of points"));
		needtodraw=1;
		return 0; 

	} else if (action==VORONOI_Thicken || action==VORONOI_Thin) {
		if (!data) return 0;
		double d=-1;
		double factor = (action==VORONOI_Thicken) ? 1.05 : 1/1.05;

		if (style_target==0)      { data->width_voronoi*=factor;  d=data->width_voronoi;  }
		else if (style_target==1) { data->width_delaunay*=factor; d=data->width_delaunay; }
		else if (style_target==2) { data->width_points*=factor;   d=data->width_points;   }

		if (d>0) {
			char scratch[70];
			sprintf(scratch,"Line width: %f",d);
			PostMessage(scratch);
			needtodraw=1;
		}

		return 0; 

	} else if (action==VORONOI_FileExport) {
		if (!data) {
			PostMessage(_("Nothing to export!"));
			return 0;
		}

		app->rundialog(new FileDialog(NULL,"Export points",_("Export points..."),
							  ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
							  object_id,"savepoints",
							  FILES_SAVE|FILES_PREVIEW, 
							  last_export));
		return 0;

	} else if (action==VORONOI_FileImport) {
		app->rundialog(new FileDialog(NULL,"Import points",_("Import points..."),
							  ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
							  object_id,"loadpoints",
							  FILES_OPEN_ONE|FILES_PREVIEW, 
							  NULL));
		return 0;

	} else if (action == VORONOI_MakeRandomRect) {
		if (!data) DropNewData();
		else data->Flush();
		DoubleBBox box;
		GetDefaultBBox(box);
		data->CreateRandomPoints(num_random, 0, box.minx, box.maxx, box.miny, box.maxy);
		Triangulate();
		previous_create = VORONOI_MakeRandomRect;
		return 0;

	} else if (action == VORONOI_MakePoisson) {
		if (!data) DropNewData();
		else data->Flush();
		DoubleBBox box;
		GetDefaultBBox(box);
		data->CreatePoissonPoints(poisson_size, 0, box.minx, box.maxx, box.miny, box.maxy);
		Triangulate();
		previous_create = VORONOI_MakePoisson;
		return 0;

	
	} else if (action == VORONOI_MakeRandomCircle) {
		if (!data) DropNewData();
		else data->Flush();
		DoubleBBox box;
		GetDefaultBBox(box);
		flatpoint o = box.BBoxPoint(.5,.5);
		double r1 = box.boxwidth()/2;
		double r2 = box.boxheight()/2;
		data->CreateRandomRadial(num_random, 0, o.x, o.y, r1 < r2 ? r1 : r2);
		Triangulate();
		previous_create = VORONOI_MakeRandomCircle;
		return 0;

	} else if (action == VORONOI_MakeGrid) {
		if (!data) DropNewData();
		else data->Flush();
		DoubleBBox box;
		GetDefaultBBox(box);
		data->CreateGrid(num_x, num_y, box.minx,box.miny, box.boxwidth(), box.boxheight(), LAX_LRTB);
		Triangulate();
		previous_create = VORONOI_MakeGrid;
		return 0;

	} else if (action == VORONOI_MakeHexChunk) {
		if (!data) DropNewData();
		else data->Flush();
		DoubleBBox box;
		GetDefaultBBox(box);
		flatpoint o = box.BBoxPoint(.5,.5);
		double r1 = box.boxwidth()/2;
		double r2 = box.boxheight()/2;
		data->CreateHexChunk(r1 < r2 ? r1 : r2, num_x);
		data->MovePoints(o.x, o.y);
		Triangulate();
		previous_create = VORONOI_MakeHexChunk;
		return 0;

	} else if (action == VORONOI_Relax) {
		if (!data) return 0;
		DoubleBBox box;
		GetDefaultBBox(box);
		// data->Relax(relax_iters, .5, .1, box);
		data->RelaxBarycenter(relax_iters, 1, box);
		Triangulate();
		PostMessage(_("Relaxing..."));
		needtodraw = 1;
		return 0;

	} else if (action == VORONOI_RelaxForce) {
		if (!data) return 0;
		DoubleBBox box;
		GetDefaultBBox(box);
		data->Relax(relax_iters, .5, .1, box);
		// data->Relax(relax_iters, 1);
		Triangulate();
		PostMessage(_("Relaxing..."));
		needtodraw = 1;
		return 0;

	} else if (action == VORONOI_RepeatLast) {
		return PerformAction(previous_create);
	}

	return 1;
}

void DelaunayInterface::GetDefaultBBox(Laxkit::DoubleBBox &box)
{
	// w/dp->Getmag()
	box.ClearBBox();
	flatpoint p = dp->screentoreal(dp->Minx, dp->Miny);
	flatpoint o = dp->screentoreal((dp->Minx+dp->Maxx)/2, (dp->Miny+dp->Maxy)/2);
	box.addtobounds(o + (p-o)*.7);
	p = dp->screentoreal(dp->Maxx, dp->Miny);
	box.addtobounds(o + (p-o)*.7);
	p = dp->screentoreal(dp->Maxx, dp->Maxy);
	box.addtobounds(o + (p-o)*.7);
	p = dp->screentoreal(dp->Minx, dp->Maxy);
	box.addtobounds(o + (p-o)*.7);
}

void DelaunayInterface::DropNewData()
{
	Clear(nullptr);
	
	int x = (dp->Minx + dp->Maxx)/2;
	int y = (dp->Miny + dp->Maxy)/2;

	data = dynamic_cast<VoronoiData *>(somedatafactory()->NewObject(LAX_VORONOIDATA));
	if (!data) data = new VoronoiData; 

	viewport->ChangeContext(x,y,NULL);
	ObjectContext *oc = NULL;
	viewport->NewData(data,&oc);//viewport adds only its own counts
	if (voc) { delete voc; voc=NULL; }
	if (oc) voc = oc->duplicate();
}

int DelaunayInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{
	if (!strcmp(mes,"loadpoints")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) {
			PostMessage(_("Could not load points."));
			return 0;
		}

		LaxFiles::Attribute att;
		if (att.dump_in(s->str)) {
			PostMessage(_("Could not parse points."));
			return 0;
		}

		if (!data) {
			data=dynamic_cast<VoronoiData *>(somedatafactory()->NewObject(LAX_VORONOIDATA));
			if (!data) data=new VoronoiData; 

			viewport->ChangeContext((dp->Minx+dp->Maxx)/2, (dp->Miny+dp->Maxy)/2, NULL);
			ObjectContext *oc=NULL;
			viewport->NewData(data,&oc);//viewport adds only its own counts
			if (voc) { delete voc; voc=NULL; }
			if (oc) voc=oc->duplicate();
		}

		data->dump_in_atts(&att,0,NULL);
		return 0;

	} else if (!strcmp(mes,"savepoints")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) {
			PostMessage(_("Could not save points."));
			return 0;
		}

		FILE *f=fopen(s->str, "w");
		if (!f) {
			PostMessage(_("Could not write to file!"));			
			return 0;
		}

		fprintf(f,"#Voronoi/Delaunay data...\n\n");
		data->dump_out(f, 0, 0, NULL);
		fclose(f);

		DBG cerr <<"...done writing out to "<<s->str<<endl;

		makestr(last_export, s->str);
		PostMessage(_("Saved."));
		return 0;

	} else if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i =s->info2; //id of menu item

		if (i == VORONOI_MakeRandomRect
		 || i == VORONOI_MakePoisson
		 || i == VORONOI_MakeRandomCircle
		 || i == VORONOI_MakeGrid
		 || i == VORONOI_MakeHexChunk) {
			//ask for number
			const char *mes = nullptr;
			const char *label = nullptr;

			char str[50];
			if (i == VORONOI_MakeRandomRect) {
				mes = "randomrectN";
				label = _("Num points");
				sprintf(str, "%d", num_random);

			} else if (i == VORONOI_MakePoisson) {
				mes = "poissonSize";
				label = _("Poisson cell size");
				sprintf(str, "%f", poisson_size);

			} else if (i == VORONOI_MakeRandomCircle) {
				mes = "randomcircleN";
				label = _("Num points");
				sprintf(str, "%d", num_random);

			} else if (i == VORONOI_MakeGrid) {
				mes = "gridN";
				label = _("Num wide, tall");
				sprintf(str, "%d, %d", num_x, num_y);

			} else if (i == VORONOI_MakeHexChunk) {
				mes = "hexN";
				label = _("Num along edge");
				sprintf(str, "%d", num_x);
			}

			double th = app->defaultlaxfont->textheight();
			DoubleBBox bounds(move_pos.x-5*th, move_pos.x+5*th, move_pos.y-th/2, move_pos.y+th/2);
			viewport->SetupInputBox(object_id, label, str, mes, bounds);
			
		} else if (i == VORONOI_Relax
		 || i == VORONOI_RelaxForce
		 || i == VORONOI_TogglePoints
		 || i == VORONOI_ToggleVoronoi
		 || i == VORONOI_ToggleShapes
		 || i == VORONOI_New
		 || i == VORONOI_RepeatLast
		 ) {
		 	PerformAction(i);
		}

		return 0;

	} else if (!strcmp(mes,"randomrectN")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i = 0;
        if (IntAttribute(s->str, &i) && i > 0) {
        	num_random = i;
        	PerformAction(VORONOI_MakeRandomRect);
        } else PostMessage(_("Huh?"));
        return 0;

    } else if (!strcmp(mes,"poissonSize")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        double d = 0;
        if (DoubleAttribute(s->str, &d) && d > 0) {
        	poisson_size = d;
        	PerformAction(VORONOI_MakePoisson);
        } else PostMessage(_("Huh?"));
        return 0;

	} else if (!strcmp(mes,"randomcircleN")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i = 0;
        if (IntAttribute(s->str, &i) && i > 0) {
        	num_random = i;
        	PerformAction(VORONOI_MakeRandomCircle);
        } else PostMessage(_("Huh?"));
        return 0;

	} else if (!strcmp(mes,"gridN")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i[2];
        i[0] = i[1] = 0;
        if (IntListAttribute(s->str, i, 2, nullptr) == 2 && i[0] > 0 && i[1] > 0) {
        	num_x = i[0];
        	num_y = i[1];
        	PerformAction(VORONOI_MakeGrid);
        } else PostMessage(_("Huh?"));
        return 0;

	} else if (!strcmp(mes,"hexN")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i = 0;
        if (IntAttribute(s->str, &i) && i > 0) {
        	num_x = i;
        	PerformAction(VORONOI_MakeHexChunk);
        } else PostMessage(_("Huh?"));
        return 0;
	}

    return 1;
}

int DelaunayInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{ 
    if (!sc) GetShortcuts();
    int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
    if (action>=0) {
        return PerformAction(action);
    }


	return 1; //key not dealt with, propagate to next interface
}



void DelaunayInterface::Triangulate()
{
	if (!data || data->points.n<3) return;

	data->RebuildVoronoi(true);
	needtodraw=1;
}





//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-------------------- Delaunay Triangulation ---------------------------------------------
//---------------- Adapted from Paul Bourke's C implementation ----------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------



//forward declarations...
int CompareXCoord(const void *v1,const void *v2);
int CircumCircle(double xp,double yp,
				 double x1,double y1,double x2,double y2,double x3,double y3,
				 double *xc,double *yc,double *rsqr);
int Triangulate(int nv,flatpoint *pxyz,IndexTriangle *v,int *ntri);



/*! tri_ret should be large enough to hold 3*nv triangles. The actual number of triangles is returned in n_ret.
 *
 * Return 0 for success, 1 for not enough points (need more than 2).
 *
 * Note that the Voronoi diagram is the dual graph of a Delaunay triangulation. The Voronoi
 * cells are all points closest to particular points.
 * It is formed by connecting the centers of all circumcircles of the triangles around each point
 * in a Delaunay triangulation.
 */
int DelaunayTriangulate(flatpoint *pts, int nv, IndexTriangle *tri_ret, int *ntri_ret)
{
   if (nv < 3) return 1;

   flatpoint p[nv+3];
   for (int c=0; c<nv; c++) pts[c].info=c;
   memcpy(p,pts, nv*sizeof(flatpoint));

    //need to order points by x
   qsort(p,nv,sizeof(flatpoint),CompareXCoord);
   Triangulate(nv,p, tri_ret,ntri_ret);

    //need to convert indices to what they were before qsort
   for (int c=0; c<*ntri_ret; c++) {
	   tri_ret[c].p1 = p[tri_ret[c].p1].info;
	   tri_ret[c].p2 = p[tri_ret[c].p2].info;
	   tri_ret[c].p3 = p[tri_ret[c].p3].info;
   }


   DBG cerr << "DelaunayTriangulate: Formed "<<(*ntri_ret)<<" triangles"<<endl;
   return 0;
}

int DelaunayTriangulate(PointSet::PointObj **pts, int nv, IndexTriangle *tri_ret, int *ntri_ret)
{
	if (nv < 3) return 1;

	flatpoint p[nv+3];
	for (int c=0; c<nv; c++) {
		p[c] = pts[c]->p;
		p[c].info = c;
	}

	//need to order points by x
	qsort(p,nv,sizeof(flatpoint),CompareXCoord);
	Triangulate(nv,p, tri_ret,ntri_ret);

	//need to convert indices to what they were before qsort
	for (int c=0; c<*ntri_ret; c++) {
	   tri_ret[c].p1 = p[tri_ret[c].p1].info;
	   tri_ret[c].p2 = p[tri_ret[c].p2].info;
	   tri_ret[c].p3 = p[tri_ret[c].p3].info;
	}

	DBG cerr << "DelaunayTriangulate: Formed "<<(*ntri_ret)<<" triangles"<<endl;
	return 0;
}



int CompareXCoord(const void *v1,const void *v2)
{
   const flatpoint *p1,*p2;
   p1 = (const flatpoint*)v1;
   p2 = (const flatpoint*)v2;
   if (p1->x < p2->x)
      return(-1);
   else if (p1->x > p2->x)
      return(1);
   else 
      return(0); 
}       


//--------------------------

class IndexEdge
{
  public:
    int p1,p2;
};


/*!
 *    Triangulation subroutine
 *    Takes as input NV vertices in array pxyz
 *    Returned is a list of ntri triangular faces in the array v
 *    These triangles are arranged in a consistent clockwise order.
 *    The triangle array 'v' should be malloced to 3 * nv
 *    The vertex array pxyz must be big enough to hold 3 more points
 *    The vertex array MUST be sorted in increasing x values say
 * 
 *   \code
 *    qsort(p,nv,sizeof(flatpoint),CompareXCoord);
 *       :
 *    int CompareXCoord(void *v1,void *v2)
 *    {
 *       flatpoint *p1,*p2;
 *       p1 = v1;
 *       p2 = v2;
 *       if (p1->x < p2->x)
 *          return(-1);
 *       else if (p1->x > p2->x)
 *          return(1);
 *       else
 *          return(0);
 *    }
 *   \endcode
 */
int Triangulate(int nv,flatpoint *pxyz,IndexTriangle *v,int *ntri)
{
   int *complete = NULL;
   IndexEdge *edges = NULL;
   int nedge = 0;
   int trimax,emax = 200;

   int inside;
   int i,j,k;
   double xp,yp,x1,y1,x2,y2,x3,y3,xc,yc,r;
   double xmin,xmax,ymin,ymax,xmid,ymid;
   double dx,dy,dmax;


   /* Allocate memory for the completeness list, flag for each triangle */
   trimax = 4 * nv;
   complete=new int[trimax];
   if (!complete) return 1;

   /* Allocate memory for the edge list */
   edges = new IndexEdge[emax]; //reallocated below as needed
   if (!edges) {
	   delete[] complete;
	   return 2;
   }


   /*
      Find the maximum and minimum vertex bounds.
      This is to allow calculation of the bounding triangle
   */
   xmin = pxyz[0].x;
   ymin = pxyz[0].y;
   xmax = xmin;
   ymax = ymin;
   for (i=1;i<nv;i++) {
      if (pxyz[i].x < xmin) xmin = pxyz[i].x;
      if (pxyz[i].x > xmax) xmax = pxyz[i].x;
      if (pxyz[i].y < ymin) ymin = pxyz[i].y;
      if (pxyz[i].y > ymax) ymax = pxyz[i].y;
   }
   dx = xmax - xmin;
   dy = ymax - ymin;
   dmax = (dx > dy) ? dx : dy;
   xmid = (xmax + xmin) / 2.0;
   ymid = (ymax + ymin) / 2.0;

   /*
      Set up the supertriangle
      This is a triangle which encompasses all the sample points.
      The supertriangle coordinates are added to the end of the
      vertex list. The supertriangle is the first triangle in
      the triangle list.
   */
   pxyz[nv+0].x = xmid - 20 * dmax;
   pxyz[nv+0].y = ymid - dmax;
   //pxyz[nv+0].z = 0.0;
   pxyz[nv+1].x = xmid;
   pxyz[nv+1].y = ymid + 20 * dmax;
   //pxyz[nv+1].z = 0.0;
   pxyz[nv+2].x = xmid + 20 * dmax;
   pxyz[nv+2].y = ymid - dmax;
   //pxyz[nv+2].z = 0.0;
   v[0].p1 = nv;
   v[0].p2 = nv+1;
   v[0].p3 = nv+2;
   complete[0] = false;
   *ntri = 1;

   /*
      Include each point one at a time into the existing mesh
   */
   for (i=0; i<nv; i++) {

      xp = pxyz[i].x;
      yp = pxyz[i].y;
      nedge = 0;

      /*
         Set up the edge buffer.
         If the point (xp,yp) lies inside the circumcircle then the
         three edges of that triangle are added to the edge buffer
         and that triangle is removed.
      */
      for (j=0; j<(*ntri); j++) {
         if (complete[j]) continue;

         x1 = pxyz[v[j].p1].x;
         y1 = pxyz[v[j].p1].y;
         x2 = pxyz[v[j].p2].x;
         y2 = pxyz[v[j].p2].y;
         x3 = pxyz[v[j].p3].x;
         y3 = pxyz[v[j].p3].y;
         inside = CircumCircle(xp,yp,x1,y1,x2,y2,x3,y3,&xc,&yc,&r);
         if (xc < xp && ((xp-xc)*(xp-xc)) > r)
				complete[j] = true;

		 v[j].circumcenter.x=xc;
		 v[j].circumcenter.y=yc;

         if (inside) {
            /* Check that we haven't exceeded the edge list size */
            if (nedge+3 >= emax) {
               emax += 100;
			   IndexEdge *newedges = new IndexEdge[emax];
               if (!newedges) {
				  delete[] edges; 
				  delete[] complete;
				  return 3;
               }
			   memcpy(newedges,edges, (emax-100)*sizeof(IndexEdge));
			   delete[] edges;
			   edges=newedges;
            }

            edges[nedge+0].p1 = v[j].p1;
            edges[nedge+0].p2 = v[j].p2;
            edges[nedge+1].p1 = v[j].p2;
            edges[nedge+1].p2 = v[j].p3;
            edges[nedge+2].p1 = v[j].p3;
            edges[nedge+2].p2 = v[j].p1;
            nedge += 3;
            v[j] = v[(*ntri)-1];
            complete[j] = complete[(*ntri)-1];
            (*ntri)--;
            j--;
         }
      }

      /*
         Tag multiple edges
         Note: if all triangles are specified anticlockwise then all
               interior edges are opposite pointing in direction.
      */
      for (j=0; j<nedge-1; j++) {
         for (k=j+1; k<nedge; k++) {
            if ((edges[j].p1 == edges[k].p2) && (edges[j].p2 == edges[k].p1)) {
               edges[j].p1 = -1;
               edges[j].p2 = -1;
               edges[k].p1 = -1;
               edges[k].p2 = -1;
            }
            /* Shouldn't need the following, see note above */
            if ((edges[j].p1 == edges[k].p1) && (edges[j].p2 == edges[k].p2)) {
               edges[j].p1 = -1;
               edges[j].p2 = -1;
               edges[k].p1 = -1;
               edges[k].p2 = -1;
            }
         }
      }

      /*
         Form new triangles for the current point
         Skipping over any tagged edges.
         All edges are arranged in clockwise order.
      */
      for (j=0; j<nedge; j++) {
         if (edges[j].p1 < 0 || edges[j].p2 < 0) continue;

         if ((*ntri) >= trimax) {
			delete[] edges;
			delete[] complete;
            return 4;
         }

         v[*ntri].p1 = edges[j].p1;
         v[*ntri].p2 = edges[j].p2;
         v[*ntri].p3 = i;

		  //need to grab circumcenter, not necessary for delaunay triangles only
         CircumCircle(0,0,
				 pxyz[v[*ntri].p1].x,pxyz[v[*ntri].p1].y,
				 pxyz[v[*ntri].p2].x,pxyz[v[*ntri].p2].y,
				 pxyz[v[*ntri].p3].x,pxyz[v[*ntri].p3].y,
				 &xc,&yc,&r);
		 v[*ntri].circumcenter.x=xc;
		 v[*ntri].circumcenter.y=yc;

         complete[*ntri] = false;
         (*ntri)++;
      }
   }

   /*
      Remove triangles with supertriangle vertices
      These are triangles which have a vertex number greater than nv
   */
   for (i=0; i<(*ntri); i++) {
      if (v[i].p1 >= nv || v[i].p2 >= nv || v[i].p3 >= nv) {
         v[i] = v[(*ntri)-1];
         (*ntri)--;
         i--;
      }
   }


   return 0;
}


#define EPSILON (1e-10)


/*
   Return true if a point (xp,yp) is inside the circumcircle made up
   of the points (x1,y1), (x2,y2), (x3,y3)
   The circumcircle centre is returned in (xc,yc) and the radius r
   NOTE: A point on the edge is inside the circumcircle
*/
int CircumCircle(double xp,double yp,
				 double x1,double y1,double x2,double y2,double x3,double y3,
				 double *xc,double *yc,double *rsqr)
{
   double m1,m2,mx1,mx2,my1,my2;
   double dx,dy,drsqr;
   double fabsy1y2 = fabs(y1-y2);
   double fabsy2y3 = fabs(y2-y3);

   /* Check for coincident points */
   if (fabsy1y2 < EPSILON && fabsy2y3 < EPSILON)
       return(false);

   if (fabsy1y2 < EPSILON) {
      m2 = - (x3-x2) / (y3-y2);
      mx2 = (x2 + x3) / 2.0;
      my2 = (y2 + y3) / 2.0;
      *xc = (x2 + x1) / 2.0;
      *yc = m2 * (*xc - mx2) + my2;

   } else if (fabsy2y3 < EPSILON) {
      m1 = - (x2-x1) / (y2-y1);
      mx1 = (x1 + x2) / 2.0;
      my1 = (y1 + y2) / 2.0;
      *xc = (x3 + x2) / 2.0;
      *yc = m1 * (*xc - mx1) + my1;

   } else {
      m1 = - (x2-x1) / (y2-y1);
      m2 = - (x3-x2) / (y3-y2);
      mx1 = (x1 + x2) / 2.0;
      mx2 = (x2 + x3) / 2.0;
      my1 = (y1 + y2) / 2.0;
      my2 = (y2 + y3) / 2.0;
      *xc = (m1 * mx1 - m2 * mx2 + my2 - my1) / (m1 - m2);
      if (fabsy1y2 > fabsy2y3) {
         *yc = m1 * (*xc - mx1) + my1;
      } else {
         *yc = m2 * (*xc - mx2) + my2;
      }
   }

   dx = x2 - *xc;
   dy = y2 - *yc;
   *rsqr = dx*dx + dy*dy;

   dx = xp - *xc;
   dy = yp - *yc;
   drsqr = dx*dx + dy*dy;

   // Original
   //return((drsqr <= *rsqr) ? true : false);
   // Proposed by Chuck Morris
   return((drsqr - *rsqr) <= EPSILON ? true : false);
}


} // namespace LaxInterfaces

