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
//
//  ---- Delauney Triangulation below:
//  Adapted from Paul Bourke's C implementation 



#include "delauneyinterface.h"

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/linestyle.h>
#include <lax/filedialog.h>
#include <lax/laxutils.h>
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
int DelauneyTriangulate(flatpoint *pts, int nv, IndexTriangle *tri_ret, int *ntri_ret);


//------------------------------- DelauneyInterface ---------------------------------

/*! \class VoronoiData
 *
 * Class to simplify building and computing simple Delauney triangulation and Voronoi nets.
 */

VoronoiData::VoronoiData()
{
	show_points  =true;
	show_delauney=true;
	show_voronoi =true;
	show_numbers =false;

	color_delauney=new Color();  color_delauney->screen.rgbf(1.0,0.0,0.0);
	color_voronoi =new Color();  color_voronoi ->screen.rgbf(0.0,0.7,0.0);
	color_points  =new Color();  color_points  ->screen.rgbf(1.0,0.0,1.0); 

	//color_delauney=CreateColor_RGB(1.0,0.0,0.0);
	//color_voronoi =CreateColor_RGB(0.0,0.7,0.0);
	//color_points  =CreateColor_RGB(1.0,0.0,1.0); 

	width_delauney=1/10.;
	width_voronoi=1/10.;
	width_points=1/10.;
}

VoronoiData::~VoronoiData()
{
	color_delauney->dec_count();
	color_voronoi ->dec_count();
	color_points  ->dec_count();
}

void VoronoiData::FindBBox()
{
	DoubleBBox::clear();
	for (int c=0; c<points.n; c++) {
		addtobounds(points.e[c]);
	}
}

void VoronoiData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
    char spc[indent+1];
    memset(spc,' ',indent);
    spc[indent]='\0';

	if (what==-1) {
		cerr << " *** need to implement VoronoiData::dump_out() description!!"<<endl;
		return;
	}

    const double *matrix=m();
    fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);

	fprintf(f,"%sshow_points   %s\n", spc, (show_points   ? "yes" : "no"));
	fprintf(f,"%sshow_delauney %s\n", spc, (show_delauney ? "yes" : "no"));
	fprintf(f,"%sshow_voronoi  %s\n", spc, (show_voronoi  ? "yes" : "no"));
	fprintf(f,"%sshow_numbers  %s\n", spc, (show_numbers  ? "yes" : "no"));

	fprintf(f,"%swidth_points   %.10g\n", spc, width_points  );
	fprintf(f,"%swidth_delauney %.10g\n", spc, width_delauney);
	fprintf(f,"%swidth_voronoi  %.10g\n", spc, width_voronoi );

	char *col=color_delauney->dump_out_simple_string();
	if (col) fprintf(f,"%scolor_delauney %s\n", spc, col);
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
			fprintf(f,"%s  %.10g, %.10g  #%d\n", spc, points.e[c].x,points.e[c].y, c);
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

        } else if (!strcmp(name,"show_delauney")) {
			show_delauney=BooleanAttribute(value);

        } else if (!strcmp(name,"show_voronoi" )) {
			show_voronoi =BooleanAttribute(value);

        } else if (!strcmp(name,"show_numbers" )) {
			show_numbers =BooleanAttribute(value);

        } else if (!strcmp(name,"width_points"  )) {
			DoubleAttribute(value, &width_points, NULL);

        } else if (!strcmp(name,"width_delauney")) {
			DoubleAttribute(value, &width_delauney, NULL);

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

					points.push(p);
				} else break;
			}

        }
    }

	RebuildVoronoi(true);
}

/*! Set the width of the lines or points.
 * which<0 sets default for all.
 * &1 is for voronoi lines,
 * &2 is for delauney lines,
 * &3 is for points.
 */
void VoronoiData::Width(double newwidth, int which)
{
	if (which<0 || which&1) width_voronoi  = newwidth;
	if (which<0 || which&2) width_delauney = newwidth;
	if (which<0 || which&4) width_points   = newwidth;
}

/*! Return the centroid of the specified triangle.
 * If triangle invalid, return (0,0).
 */
flatpoint VoronoiData::Centroid(int triangle)
{
	if (triangle<0 || triangle>=triangles.n) return flatpoint(0,0);
	return ( points.e[triangles.e[triangle].p1]
			+points.e[triangles.e[triangle].p2]
			+points.e[triangles.e[triangle].p3]) / 3;
}

void VoronoiData::Triangulate()
{
	if (points.n<3) return;

	triangles.flush_n();
	triangles.Allocate(3*points.n);
	//triangles.flush();
	//triangles.Allocate(3*points.n);

	DelauneyTriangulate(points.e,points.n, triangles.e,&triangles.n);
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
		region=&regions.e[c];
		region->point=points.e[c];
		region->next_hull=-1;
		region->tris.flush();

		 //find a triangle that has the point
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

				if (pos==1)      v=points.e[triangles.e[curtri].p2] - points.e[triangles.e[curtri].p1];
				else if (pos==2) v=points.e[triangles.e[curtri].p3] - points.e[triangles.e[curtri].p2];
				else             v=points.e[triangles.e[curtri].p1] - points.e[triangles.e[curtri].p3];
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

					if (pos==1)      v=points.e[triangles.e[curtri].p3] - points.e[triangles.e[curtri].p1];
					else if (pos==2) v=points.e[triangles.e[curtri].p1] - points.e[triangles.e[curtri].p2];
					else             v=points.e[triangles.e[curtri].p2] - points.e[triangles.e[curtri].p3];
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



//------------------------------- DelauneyInterface ---------------------------------

/*! \class DelauneyInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


DelauneyInterface::DelauneyInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	delauney_interface_style=0;

	show_numbers=false; 
	show_arrows=false;
	show_lines=3;

	showdecs=1;
	needtodraw=1;
	curpoint=-1;
	justadded=false;
	style_target = 0; //0 is voronoi border, 1 is delauney tri edges, 2 is points

	sc=NULL;
	
	data=NULL;
	
	last_export=newstr("voronoi.data");
}

DelauneyInterface::~DelauneyInterface()
{
	if (sc)   sc->dec_count();
	if (data) data->dec_count();
	delete[] last_export;
}

const char *DelauneyInterface::whatdatatype()
{  
	return "VoronoiData";
	//return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *DelauneyInterface::Name()
{ return _("Delauney/Voronoi Tool"); }


//! Return new DelauneyInterface.
/*! If dup!=NULL and it cannot be cast to DelauneyInterface, then return NULL.
 */
anInterface *DelauneyInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new DelauneyInterface(NULL,id,NULL);
	else if (!dynamic_cast<DelauneyInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

void DelauneyInterface::Clear(SomeData *d)
{
	if (!d || d==data) {
		data->dec_count();
		delete voc;
		voc=NULL;
	}
}

int DelauneyInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
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

	show_lines = (data->show_delauney ? 2 : 0) | (data->show_voronoi ? 1 : 0);
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

int DelauneyInterface::Refresh()
{ 
	if (needtodraw==0) return 0;
	needtodraw=0;

	if (!data) return 0;

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->font(anXApp::app->defaultlaxfont);
	dp->fontsize(anXApp::app->defaultlaxfont->textheight() / dp->Getmag());


	 //delauney triangles
	dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg));
	dp->LineWidth(data->width_delauney);
	flatpoint center,p,v;

	dp->NewFG(data->color_delauney);
	for (int c=0; c<data->triangles.n; c++) {
		 //draw edges
		if (show_lines&2) {
			dp->moveto(data->points.e[data->triangles[c].p1]);
			dp->lineto(data->points.e[data->triangles[c].p2]);
			dp->lineto(data->points.e[data->triangles[c].p3]);
			dp->closed();
			dp->stroke(0);
		}

		 //draw arrows indicating edge direction
		center=(data->points.e[data->triangles[c].p1]+data->points.e[data->triangles[c].p2]+data->points.e[data->triangles[c].p3])/3;
		if (show_numbers) dp->drawnum(center.x,center.y, c);

		v=.2*(data->points.e[data->triangles[c].p2]-data->points.e[data->triangles[c].p1]);
		p=data->points.e[data->triangles[c].p1];
		p=p+.3*(center-p);
		if (show_arrows) dp->drawarrow(p,v, 0,1,2,3);
		if (show_numbers) dp->textout(p.x,p.y, "1,",1, LAX_HCENTER);

		v=.2*(data->points.e[data->triangles[c].p3]-data->points.e[data->triangles[c].p2]);
		p=data->points.e[data->triangles[c].p2];
		p=p+.3*(center-p);
		if (show_arrows) dp->drawarrow(p,v, 0,1,2,3);
		if (show_numbers) dp->textout(p.x,p.y, "2,",1, LAX_CENTER);

		v=.2*(data->points.e[data->triangles[c].p1]-data->points.e[data->triangles[c].p3]);
		p=data->points.e[data->triangles[c].p3];
		p=p+.3*(center-p);
		if (show_arrows) dp->drawarrow(p,v, 0,1,2,3);
		if (show_numbers) dp->textout(p.x,p.y, "3,",1, LAX_CENTER);

	}


//	 //hull edges
//	dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
//	IndexTriangle *tri;
//	for (int c=0; c<data->triangles.n; c++) {
//		tri=&data->triangles.e[c];
//		if (tri->t[0]<0) {
//			dp->drawline(data->points.e[tri->p1],data->points.e[tri->p2]);
//		}
//		if (tri->t[1]<0) {
//			dp->drawline(data->points.e[tri->p2],data->points.e[tri->p3]);
//		}
//		if (tri->t[2]<0) {
//			dp->drawline(data->points.e[tri->p3],data->points.e[tri->p1]);
//		}
//	}
//	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);


	 //points
	dp->NewFG(data->color_points);
	for (int c=0; c<data->points.n; c++) {
		double r=(c==curpoint?2:1)*data->width_points;
		//dp->drawpoint(data->points.e[c], (c==curpoint?2:1)*data->width_points, 1);
		dp->drawellipse(data->points.e[c], r,r, 0,2*M_PI, 1);


		if (show_numbers) {
			dp->drawnum(data->points.e[c].x,data->points.e[c].y, c);
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


	 //voronoi lines
	if (show_lines&1) {
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

ObjectContext *DelauneyInterface::Context()
{
	return voc;
}

//! Start a new freehand line.
int DelauneyInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	buttondown.down(d->id,LEFTBUTTON,x,y);

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
		data->points.push(data->transformPointInverse(screentoreal(x,y)));
		Triangulate();
	}


	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Finish a new freehand line by calling newData with it.
int DelauneyInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	if (!data) return 0;

	int dragged=buttondown.up(d->id,LEFTBUTTON);

	if (!justadded) {
		if (dragged<3 && curpoint>=0) data->points.remove(curpoint);
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
int DelauneyInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *m)
{
	if (!data) return 0;

	if (!buttondown.any()) {
		// update any mouse over state
		int oldcp=curpoint;
		int c;
		for (c=0; c<data->points.n; c++) {
			if (realtoscreen(data->transformPoint(data->points.e[c])).distanceTo(flatpoint(x,y))<10) break;
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
	data->points.e[curpoint]+=d;
	Triangulate();
	

	needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

//! Use the object at oc if it is an ImageData.
int DelauneyInterface::UseThisObject(ObjectContext *oc)
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

	show_lines = (data->show_delauney ? 2 : 0) | (data->show_voronoi ? 1 : 0);
	show_numbers = data->show_numbers;

	//SimpleColorEventData *e=new SimpleColorEventData( 65535, 0xffff*data->red, 0xffff*data->green, 0xffff*data->blue, 0xffff*data->alpha, 0);
	//app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);

	needtodraw=1;
	return 1;
}

int DelauneyInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask)
{
    if (!nobj) return 1;

    if (data && dynamic_cast<LineStyle *>(nobj)) {
        //DBG cerr <<"Delauney new color stuff"<< endl;

        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);

        if (nlinestyle->mask&GCForeground) {
			Color *color=NULL;
			if (style_target==0)      color = data->color_delauney;
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


Laxkit::ShortcutHandler *DelauneyInterface::GetShortcuts()
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
	
	
    manager->AddArea(whattype(),sc);
	return sc;
}

int DelauneyInterface::PerformAction(int action)
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
		needtodraw=1;
		return 0; 

	} else if (action==VORONOI_StyleTarget) {
		style_target++;
		if (style_target>=3) style_target=0;
		if (style_target==0) PostMessage(_("Adjust style of voronoi lines"));
		else if (style_target==1) PostMessage(_("Adjust style of delauney lines"));
		else if (style_target==2) PostMessage(_("Adjust style of points"));
		needtodraw=1;
		return 0; 

	} else if (action==VORONOI_Thicken || action==VORONOI_Thin) {
		if (!data) return 0;
		double d=-1;
		double factor = (action==VORONOI_Thicken) ? 1.05 : 1/1.05;

		if (style_target==0)      { data->width_voronoi*=factor;  d=data->width_voronoi;  }
		else if (style_target==1) { data->width_delauney*=factor; d=data->width_delauney; }
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

	}

	return 1;
}

int DelauneyInterface::Event(const Laxkit::EventData *e_data, const char *mes)
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

		fprintf(f,"#Voronoi/Delauney data...\n\n");
		data->dump_out(f, 0, 0, NULL);
		fclose(f);

		DBG cerr <<"...done writing out to "<<s->str<<endl;

		makestr(last_export, s->str);
		PostMessage(_("Saved."));
		return 0;
	}

    return 1;
}

int DelauneyInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{ 
    if (!sc) GetShortcuts();
    int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
    if (action>=0) {
        return PerformAction(action);
    }


	return 1; //key not dealt with, propagate to next interface
}



void DelauneyInterface::Triangulate()
{
	if (!data || data->points.n<3) return;

	data->RebuildVoronoi(true);
	needtodraw=1;
}





//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-------------------- Delauney Triangulation ---------------------------------------------
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
 * Note that the Voronoi diagram is the dual graph of a Delauney triangulation. The Voronoi
 * cells are all points closest to particular points.
 * It is formed by connecting the centers of all circumcircles of the triangles around each point
 * in a Delauney triangulation.
 */
int DelauneyTriangulate(flatpoint *pts, int nv, IndexTriangle *tri_ret, int *ntri_ret)
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
	   tri_ret[c].p1=p[tri_ret[c].p1].info;
	   tri_ret[c].p2=p[tri_ret[c].p2].info;
	   tri_ret[c].p3=p[tri_ret[c].p3].info;
   }


   DBG cerr << "Formed "<<(*ntri_ret)<<" triangles"<<endl;

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

		  //need to grab circumcenter, not necessary for delauney triangles only
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

