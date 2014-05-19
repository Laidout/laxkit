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
//    Copyright (C) 2014 by Tom Lechner
//



#include <lax/interfaces/engraverfillinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/dumpcontext.h>
#include <lax/imagedialog.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <lax/fileutils.h>
#include <lax/filedialog.h>
#include <lax/interfaces/freehandinterface.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/interfaces/somedataref.h>

// *** DBG:
#include <lax/laximages-imlib.h>


#include <lax/lists.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {



//------------------------------------- LinePoint ------------------------
/*! \class LinePoint
 * Kind of a temp node for EngraverFillData.
 */


void LinePoint::Clear()
{
	if (next) delete next;
	next=NULL;
}

//! Add an OPEN line segment right after *this.
void LinePoint::Add(LinePoint *np)
{
	LinePoint *pp=np;
	while (pp->next) pp=pp->next;

	pp->next=next;
	if (next) next->prev=pp;

	next=np;
	np->prev=this;
}

/*! Copy over everything except next and prev.
 */
void LinePoint::Set(LinePoint *pp)
{
	s=pp->s;
	t=pp->t;
	row=pp->row;
	col=pp->col;
	weight=pp->weight;
	on=pp->on;
	needtosync=pp->needtosync;
	p=pp->p;
}


//------------------------------------- EngraverPointGroup ------------------------

/*! \class EngraverPointGroup
 *
 * Info for groups of points in an EngraverFillData.
 *
 * Built in generator types are: linear, radial, spiral, circular
 */


/*! Default to linear.
 */
EngraverPointGroup::EngraverPointGroup()
{
	id=getUniqueNumber(); //the group number in LinePoint
	name=NULL;
	type=PGROUP_Linear; //what manner of lines
	type_d=0;   //parameter for type, for instance, an angle for spirals

	spacing=10;
	dash_length=spacing*2;
	zero_threshhold=0;
	broken_threshhold=0;
}

/*! Creates a unique new number for id if nid<0.
 */
EngraverPointGroup::EngraverPointGroup(int nid,const char *nname, int ntype, flatpoint npos, flatpoint ndir, double ntype_d)
{
	id=nid;
	if (id<0) id=getUniqueNumber(); //the group number in LinePoint
	name=newstr(nname);
	type=ntype; //what manner of lines
	type_d=ntype_d;   //parameter for type, for instance, an angle for spirals
	position=npos;
	direction=ndir;

	spacing=10;
	dash_length=spacing*2;
	zero_threshhold=0;
	broken_threshhold=0;
}

EngraverPointGroup::~EngraverPointGroup()
{
	delete[] name;
}

/*! Provide a direction vector for specified point. This is used to grow lines
 * in EngraverFillData objects. If you need exact lines, you will want to use
 * LineFrom(), since building from Direction() here will introduce too many
 * rounding errors. For instance, you will never build exact circles only
 * from a direction field.
 */
flatpoint EngraverPointGroup::Direction(double s,double t)
{
	if (type==PGROUP_Linear) {
		return direction;

	} else if (type==PGROUP_Circular) {
		return transpose(flatpoint(s,t)-position);

	} else if (type==PGROUP_Radial) {
		return flatpoint(s,t)-position;

	} else if (type==PGROUP_Spiral) {
		return rotate(transpose(flatpoint(s,t)-position), type_d);
	}

	return flatpoint();
}

/*! Create a line extending from coordinate s,t.
 */
LinePoint *EngraverPointGroup::LineFrom(double s,double t)
{
	if (type==PGROUP_Linear) {

	} else if (type==PGROUP_Circular) {
	} else if (type==PGROUP_Radial) {
	} else if (type==PGROUP_Spiral) {
	}

	return NULL;
}

/*! fill in x,y = 0..1,0..1
 */
void EngraverPointGroup::Fill(EngraverFillData *data)
{
	if (type==PGROUP_Linear) {
		FillRegularLines(data);

	} else if (type==PGROUP_Circular) {
	} else if (type==PGROUP_Radial) {
	} else if (type==PGROUP_Spiral) {
	}

}

/*! spacing is an object distance (not in s,t space) to be used as the distance between line centers.
 * If spacing<0, then use 1/25 of the x or y dimension, whichever is smaller
 * If weight<0, then use spacing/10.
 * Inserts lines folling this->direction, which is in (s,t) space.
 */
void EngraverPointGroup::FillRegularLines(EngraverFillData *data)
{
	double spacing=data->default_spacing;
	double weight=-1;


	if (weight<=0) weight=spacing/10; //remember, weight is actual distance, not s,t!!


	LinePoint *p;

	flatvector v=direction; //this is s,t space
	if (v.x<0) v=-v;
	v.normalize();
	//flatvector vt=transpose(v);

	 //we need to find the s,t equivalent of spacing along direction
	flatpoint vp=data->getPoint(.1*v.x+.5, .1*v.y+.5) - data->getPoint(.5,.5);
	v*=.1*spacing/norm(vp);
	double vv=norm2(v);

	//vp=getPoint(.1*vt.x+.5, .1*vt.y+.5)-getPoint(.5,.5);
	//vt*=.1*spacing/norm(vp);

	double s_spacing= (v.y==0 ? -1 : fabs(vv/v.y));
	double t_spacing= (v.x==0 ? -1 : fabs(vv/v.x));

	//if (xsize>4) s_spacing/= xsize/3;
	//if (ysize>4) t_spacing/= ysize/3;

	if (v.y<0) data->lines.push(new LinePoint(0,1,weight,id)); //push a (0,0) starter point
	else       data->lines.push(new LinePoint(0,0,weight,id)); //push a (0,0) starter point

	 //starter points along y
	if (t_spacing>0) {
		for (double yy=t_spacing; yy<=1; yy+=t_spacing) {
			if (v.y<0) data->lines.push(new LinePoint(0,1-yy, weight,id));
			else       data->lines.push(new LinePoint(0,yy, weight,id));
		}
	}

	 //starter points along x
	if (s_spacing>0) {
		for (double xx=s_spacing; xx<=1; xx+=s_spacing) {
			if (v.y<0) data->lines.push(new LinePoint(xx,1, weight,id));
			else       data->lines.push(new LinePoint(xx,0, weight,id));
		}
	}

	 //grow lines
	flatvector pp;
	for (int c=0; c<data->lines.n; c++) {
		p=data->lines.e[c];

		while (p->s>=0 && p->t>=0 && p->s<=1 && p->t<=1) {
			pp=flatpoint(p->s,p->t) + v;
			p->next=new LinePoint(pp.x, pp.y, weight,id);
			p->next->prev=p;
			p=p->next;
		}
	}
}


//------------------------------------- EngraverFillData ------------------------


/*! \class EngraverFillData
 * \ingroup interfaces
 *
 * See EngraverFillInterface.
 */


EngraverFillData::EngraverFillData()
  : PatchData(0,0,1,1,1,1,0)
{
	usepreview=0;

	zero_threshhold=.005;
	broken_threshhold=0;
	default_spacing=.1;


	direction=flatvector(1,0);
	fillstyle.color.red=0;
	fillstyle.color.green=0;
	fillstyle.color.blue=65535;
	fillstyle.color.alpha=65535;

	maxx=maxy=1;
}

////! Create a patch with the provided color data, set in unit rectangle x:[0,1] and y:[0,1].
//EngraverFillData::EngraverFillData(int iwidth, int iheight, unsigned long *ndata,int disl,
//							   double nscale,unsigned int stle)
//  : PatchData(0,0,1,1,1,1,0)
//{
//	usepreview=1;
//
//	direction.x=1;
//}

EngraverFillData::~EngraverFillData()
{
}

SomeData *EngraverFillData::duplicate(SomeData *dup)
{
	EngraverFillData *p=dynamic_cast<EngraverFillData*>(dup);
	if (!p && !dup) return NULL; //was not EngraverFillData!

	//char set=1;
	if (!dup && somedatafactory) {
		dup=somedatafactory->newObject(LAX_ENGRAVERFILLDATA,this);
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			//set=0;
		}
		p=dynamic_cast<EngraverFillData*>(dup);
	} 
	if (!p) {
		p=new EngraverFillData();
		dup=p;
	}

	p->fillstyle=fillstyle;
	p->direction=direction;
	p->zero_threshhold=zero_threshhold;
	p->broken_threshhold=broken_threshhold;
	p->nlines=lines.n;

	LinePoint *pp, *lp;
	for (int c=0; c<lines.n; c++) {
		pp=new LinePoint();
		pp->Set(lines.e[c]);
		p->lines.push(pp);

		lp=lines.e[c]->next;
		while (lp) {
			pp->next=new LinePoint();
			pp->next->prev=pp;
			pp->next->Set(lp);
			pp=pp->next;
			lp=lp->next;
		}
	}

	return PatchData::duplicate(dup);
}

/*! Make lines->p be the transformed s,t.
 */
void EngraverFillData::Sync()
{
	LinePoint *l;
	for (int c=0; c<lines.n; c++) {
		l=lines.e[c];

		while (l) {
			l->p=getPoint(l->s,l->t); // *** note this is hideously inefficient, matrices are not cached with getPoint!!!
			l->needtosync=0;

			l=l->next;
		}
	}
}

/*! Assume lines->p are accurate, and we need to map back to s,t mesh coordinates.
 */
void EngraverFillData::ReverseSync(bool asneeded)
{
	LinePoint *l;
	flatpoint pp;
	int in=0;

	for (int c=0; c<lines.n; c++) {
		l=lines.e[c];

		while (l) {
			if (!asneeded || (asneeded && l->needtosync==2)) {
				pp=getPointReverse(l->p.x,l->p.y, &in); // *** note this is hideously inefficient
				if (in) {
					l->s=pp.x;
					l->t=pp.y;
					l->needtosync=0;
				}
			}

			l=l->next;
		}
	}
}

/*! Return whether a point is considered on by the criteria of the data.
 * Default is point must be on, and weight>=zero_threshhold.
 */
int EngraverFillData::PointOn(LinePoint *p)
{
	if (!p->on) return 0;
	if (p->weight<zero_threshhold) return 0;
	return 1;
}

/*! spacing is an object distance (not in s,t space) to be used as the distance between line centers.
 * If spacing<0, then use 1/25 of the x or y dimension, whichever is smaller
 * If weight<0, then use spacing/10.
 * Inserts lines folling this->direction, which is in (s,t) space.
 */
void EngraverFillData::FillRegularLines(double weight, double spacing)
{
	 // create generic lines to experiment with weight painting...
	if (spacing<=0) {
		if (maxx-minx<maxy-miny) spacing=(maxx-minx)/25; else spacing=(maxy-miny)/25;
	}
	default_spacing=spacing;

	lines.flush();
	nlines=0;
	LinePoint *p;


	if (weight<=0) weight=spacing/10; //remember, weight is actual distance, not s,t!!

	flatvector v=direction; //this is s,t space
	if (v.x<0) v=-v;
	v.normalize();
	//flatvector vt=transpose(v);

	 //we need to find the s,t equivalent of spacing along direction
	flatpoint vp=getPoint(.1*v.x+.5, .1*v.y+.5)-getPoint(.5,.5);
	v*=.1*spacing/norm(vp);
	double vv=norm2(v);

	//vp=getPoint(.1*vt.x+.5, .1*vt.y+.5)-getPoint(.5,.5);
	//vt*=.1*spacing/norm(vp);

	double s_spacing= (v.y==0 ? -1 : fabs(vv/v.y));
	double t_spacing= (v.x==0 ? -1 : fabs(vv/v.x));

	//if (xsize>4) s_spacing/= xsize/3;
	//if (ysize>4) t_spacing/= ysize/3;

	if (v.y<0) lines.push(new LinePoint(0,1,weight)); //push a (0,0) starter point
	else       lines.push(new LinePoint(0,0,weight)); //push a (0,0) starter point

	 //starter points along y
	if (t_spacing>0) {
		for (double yy=t_spacing; yy<=1; yy+=t_spacing) {
			if (v.y<0) lines.push(new LinePoint(0,1-yy, weight));
			else       lines.push(new LinePoint(0,yy, weight));
		}
	}

	 //starter points along x
	if (s_spacing>0) {
		for (double xx=s_spacing; xx<=1; xx+=s_spacing) {
			if (v.y<0) lines.push(new LinePoint(xx,1, weight));
			else       lines.push(new LinePoint(xx,0, weight));
		}
	}

	 //grow lines
	flatvector pp;
	for (int c=0; c<lines.n; c++) {
		p=lines.e[c];

		while (p->s>=0 && p->t>=0 && p->s<=1 && p->t<=1) {
			pp=flatpoint(p->s,p->t) + v;
			p->next=new LinePoint(pp.x, pp.y, weight);
			p->next->prev=p;
			p=p->next;
		}

		nlines++;
	}
}

void EngraverFillData::Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	PatchData::Set(xx,yy,ww,hh,nr,nc,stle);
}

/*! \ingroup interfaces
 * Dump out an EngraverFillData.
 *
 * Something like:
 * <pre>
 *  filename blah.jpg
 *  iwidth 100
 *  iheight 100
 *  matrix 1 0 0 1 0 0
 *  xsize 4
 *  ysize 4
 *  points \
 *    1 2
 *    3 4
 *    5 6
 * </pre>
 * 
 * \todo *** assumes data is from filename. It shouldn't. It might be random buffer information.
 *
 * Calls PatchData::dump_out(f,indent,0,context), then puts out filename, and the
 * pixel dimensions of the image in filename.
 * If what==-1, then output a pseudocode mockup of the format. Otherwise
 * output the format as above.
 */
void EngraverFillData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 

//	if (what==-1) {
//		fprintf(f,"%sfilename whicheverfile.jpg  #name of the image used\n", spc);
//		fprintf(f,"%siwidth  100  #width of the image in pixels for a preview sampling\n",spc);
//		fprintf(f,"%siheight 100  #iheight of the image in pixels for a preview sampling\n",spc);
//		return;
//	}

	fprintf(f,"%smesh\n",spc);
	PatchData::dump_out(f,indent+2,what,context);

	fprintf(f,"%sdirection (%.10g, %.10g)\n",spc, direction.x,direction.y);
	fprintf(f,"%sspacing %.10g\n",spc, default_spacing);
	
	fprintf(f,"%scolor rgbaf(%.10g,%.10g,%.10g,%.10g)\n",spc, 
			fillstyle.color.red/65535.,
			fillstyle.color.green/65535.,
			fillstyle.color.blue/65535.,
			fillstyle.color.alpha/65535.);

	LinePoint *p;
	for (int c=0; c<nlines; c++) {
		fprintf(f,"%sline \\ #%d\n",spc,c);
		p=lines.e[c];
		while (p) {
			fprintf(f,"%s  (%.10g, %.10g) %.10g %s\n",spc, p->s,p->t,p->weight, p->on?"on":"off");
			p=p->next;
		}
	}

	return;
}

//! Reverse of dump_out.
void EngraverFillData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"mesh")) {
			PatchData::dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"direction")) {
			FlatvectorAttribute(value,&direction);

		} else if (!strcmp(name,"spacing")) {
			DoubleAttribute(value,&default_spacing, NULL);

		} else if (!strcmp(name,"color")) {
			unsigned long color;
			SimpleColorAttribute(value, &color);
			double b=(color&0xff)/255.;
			double g=((color&0xff00)>>8)/255.;
			double r=((color&0xff0000)>>16)/255.;
			double a=((color&0xff000000)>>24)/255.;
			fillstyle.color.rgbf(r,g,b,a);

		} else if (!strcmp(name,"line")) {
			char *end_ptr=NULL;
			flatpoint v;
			int status;
			double w;
			bool on=true;
			LinePoint *lstart=NULL, *ll=NULL;

			do {
				 //get (s,t) point
				status=FlatvectorAttribute(value, &v, &end_ptr);
				if (status==0) break;

				 //get weight
				value=end_ptr;
				status=DoubleAttribute(value, &w, &end_ptr);
				if (status==0) break;

				value=end_ptr;
				while (isspace(*value)) value++;
				if (*value=='o' && value[1]=='n') { on=true; value+=2; }
				else if (*value=='o' && value[1]=='f' && value[2]=='f') { on=false; value+=3; }

				if (!lstart) { lstart=ll=new LinePoint(v.x,v.y, w); ll->on=on; }
				else {
					ll->next=new LinePoint(v.x,v.y, w);
					ll->next->prev=ll;
					ll->next->on=on;
					ll=ll->next;
				}

				while (isspace(*value)) value++;

			} while (*value!='\0');

			if (lstart) lines.push(lstart);
		}
	}

	nlines=lines.n;
	FindBBox();
	Sync();
}

PathsData *EngraverFillData::MakePathsData()
{
	PathsData *paths=NULL;
    if (somedatafactory) 
		paths=dynamic_cast<PathsData*>(somedatafactory->newObject(LAX_PATHSDATA,NULL));
    else paths=new PathsData();
	paths->m(m());

	//currently, makes a PathsData with the outline of all the strokes...

	//Todo: does not currently handly 0 weight segments properly

	NumStack<flatvector> points;
	NumStack<flatvector> points2;

	LinePoint *l;
	flatvector t, tp;
	flatvector p1,p2;
	//double lastwidth;
	//double neww;

	for (int c=0; c<lines.n; c++) {
		l=lines.e[c];

		 //make points be a list of points:
		 //   2  4  6 
		 // 1 *--*--*--8   gets rearranged to: 1 2 4 6 8 3 5 7
		 //   3  5  7
		 //points 1 and 8 are cap point references, converted to rounded ends later
		while (l) {
			if (!PointOn(l)) { l=l->next; continue; }

			if (l->next && l->prev) tp=l->next->p - l->prev->p;
			else if (l->next && !l->prev) tp=l->next->p - l->p;
			else if (!l->next && l->prev) tp=l->p - l->prev->p;
			else tp=flatpoint(1,0); //<- a line with a single point

			tp.normalize(); 
			t=transpose(tp);

			if (points.n==0) {
				 //add a first point cap
				points.push(l->p - l->weight/2*tp);
			}

			 //add top and bottom points for l
			p1=l->p + l->weight/2*t;
			p2=l->p - l->weight/2*t;

			points.push(p1);
			points.push(p2);


			if (!l->next || !l->next->on) {
				 //need to add a path!
	
				 //add last cap
				tp=points.e[points.n-1]-points.e[points.n-2];
				tp.normalize();
				tp=transpose(tp);
				points.push(l->p + l->weight/2*tp);


				 //convert to bez approximation
				 //make points2 be points rearranged according to outline
				points2.push(points.e[0]); //initial cap
				for (int c2=1; c2<points.n-1; c2+=2)    points2.push(points.e[c2]);

				points2.push(points.e[points.n-1]); //final cap
				for (int c2=points.n-2; c2>0; c2-=2) points2.push(points.e[c2]);
				

				BezApproximate(points,points2);

				paths->moveTo(points.e[1]);
				for (int c2=1; c2<points.n; c2+=3) {
					paths->curveTo(points.e[c2+1], points.e[(c2+2)%points.n], points.e[(c2+3)%points.n]);
				}

				paths->close();

				points.flush();
				points2.flush();
			}

			l=l->next;
		}


	}

	return paths;
}

void EngraverFillData::dump_out_svg(const char *file)
{
	FILE *f=fopen(file,"w");
	if (!f) return;

	//powerstroke path effect goes in defs, BUT the outline is in the path->d,
	//the original path is in an attribute of the path...
	

	fprintf(f,  "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
				"<!-- Created with Inkscape (http://www.inkscape.org/) -->\n"
				"\n"
				"<svg\n"
				"   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
				"   xmlns:cc=\"http://creativecommons.org/ns#\"\n"
				"   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
				"   xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
				"   xmlns=\"http://www.w3.org/2000/svg\"\n"
				"   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n"
				"   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n"
				//"   width=\"%.10g\"\n"
				//"   height=\"%.10g\"\n"
				"   width=\"1000\"\n"
				"   height=\"1000\"\n"
				"   id=\"svg2\"\n"
				"   version=\"1.1\"\n"
				"   inkscape:version=\"0.48+devel r custom\"\n"
				"   viewBox=\"0 0 744.09448 1052.3622\"\n"
				"   sodipodi:docname=\"test-powerstroke.svg\">\n"
				"  <defs id=\"defs4\">\n" 
					//1.1*(maxx-minx), 1.1*(maxy-miny)
			);


	// *** output path effect defs
	//    <inkscape:path-effect
	//       effect="powerstroke"
	//       id="path-effect4064"
	//       is_visible="true"
	//       offset_points="0,1 | 3.0017209,-52.700851 | 2.2287256,-40.548946 | 5,-2.3889435"
	//       sort_points="true"
	//       interpolator_type="CubicBezierJohan"
	//       interpolator_beta="0.2"
	//       start_linecap_type="round"
	//       linejoin_type="round"
	//       miter_limit="4"
	//       end_linecap_type="round" />


	fprintf(f,  "  </defs>\n");
	fprintf(f,  " <g\n"
				"     inkscape:label=\"Layer 1\"\n"
				"     inkscape:groupmode=\"layer\"\n"
				"     id=\"layer1\">\n");
	double s;
	if (maxx-minx>maxy-miny) s=900/(maxx-minx);
	else s=900/(maxy-miny);
	fprintf(f,  "  <g id=\"themesh\" "
	                 //"transform=\"matrix(1 0 0 -1 0 0)\" \n   >"
	                 "transform=\"matrix(%.10g 0 0 -%.10g %.10g %.10g)\" \n   >",
						s,s,
						-minx*s,miny*s+1000
						//-m(4),-m(5)
		         );
 
	// *** output paths
	//  <path d="....the full outline....."
	//        inkscape:path-effect="path-effect4064"
	//        inkscape:original-d="....original path...."

	NumStack<flatvector> points;
	NumStack<flatvector> points2;

	LinePoint *l;
	flatvector t, tp;
	flatvector p1,p2;
	//double lastwidth;
	//double neww;

	for (int c=0; c<lines.n; c++) {
		l=lines.e[c];

		 //make points be a list of points:
		 //   2  4  6 
		 // 1 *--*--*--8   gets rearranged to: 1 2 4 6 8 3 5 7
		 //   3  5  7
		 //points 1 and 8 are cap point references, converted to rounded ends later
		while (l) {
			if (!PointOn(l)) { l=l->next; continue; }

			if (l->next && l->prev) tp=l->next->p - l->prev->p;
			else if (l->next && !l->prev) tp=l->next->p - l->p;
			else if (!l->next && l->prev) tp=l->p - l->prev->p;
			else tp=flatpoint(1,0); //<- a line with a single point

			tp.normalize(); 
			t=transpose(tp);

			if (points.n==0) {
				 //add a first point cap
				points.push(l->p - l->weight/2*tp);
			}

			 //add top and bottom points for l
			p1=l->p + l->weight/2*t;
			p2=l->p - l->weight/2*t;

			points.push(p1);
			points.push(p2);


			if (!l->next || !PointOn(l->next)) {
				 //need to add a path!
	
				 //add last cap
				tp=points.e[points.n-1]-points.e[points.n-2];
				tp.normalize();
				tp=transpose(tp);
				points.push(l->p + l->weight/2*tp);


				 //convert to bez approximation
				 //make points2 be points rearranged according to outline
				points2.push(points.e[0]); //initial cap
				for (int c2=1; c2<points.n-1; c2+=2)    points2.push(points.e[c2]);

				points2.push(points.e[points.n-1]); //final cap
				for (int c2=points.n-2; c2>0; c2-=2) points2.push(points.e[c2]);
				

				BezApproximate(points,points2);

				fprintf(f,"    <path d=\"");
				fprintf(f,"M %f %f ", points.e[1].x,points.e[1].y);
				for (int c2=1; c2<points.n; c2+=3) {
					fprintf(f,"C %f %f %f %f %f %f ",
						points.e[c2+1].x,points.e[c2+1].y,
						points.e[(c2+2)%points.n].x,points.e[(c2+2)%points.n].y,
						points.e[(c2+3)%points.n].x,points.e[(c2+3)%points.n].y);
				}


				fprintf(f,"z \" />\n");

				points.flush();
				points2.flush();
			}

			l=l->next;
		} //while (l)
	} //foreach line


	fprintf(f,  "  </g>\n"
				" </g>\n"
				"</svg>\n");


	fclose(f);
}

/*! Add more sample points between all existing points.
 *
 * Approximate each line with a bezier curve, and grab the center of each segment.
 *
 * Afterwards, you will need to call ReverseSync(true).
 */
void EngraverFillData::MorePoints()
{
	NumStack<flatpoint> pts;
	flatpoint *bez=NULL;
	int n,i;
	LinePoint *ll;
	LinePoint *l;

	for (int c=0; c<lines.n; c++) {
		l=lines.e[c];
		n=0;

		pts.flush();
		while (l) { pts.push(l->p); l=l->next; }

		if (3*pts.n>n) {
			if (bez) delete[] bez;
			bez=new flatpoint[3*pts.n];
			n=3*pts.n;
		}
		bez_from_points(bez, pts.e,pts.n);

		i=1;
		l=lines.e[c];
		while (l) {
			if (!l->next) break;
	
			ll=new LinePoint();
			ll->weight=(l->weight+l->next->weight)/2;
			ll->spacing=(l->spacing+l->next->spacing)/2;
			ll->on=(l->on || l->next->on);
			ll->s=(l->s+l->next->s)/2;
			ll->t=(l->t+l->next->t)/2; //just in case reverse map doesn't work
			ll->p=bez_point(.5, l->p, bez[i+1], bez[i+2], l->next->p);

			ll->next=l->next;
			l->next->prev=ll;
			l->next=ll;
			ll->prev=l;
			ll->needtosync=2;

			i+=3;
			l=l->next->next;
		}
	}
}

/*! points is a special list of sample points, meaning points that lie on the line.
 * This makes fauxpoints be a bezier list: c-p-c-c-p-c-...-c-p-c smoothly connecting all the points.
 *
 * points is assumed to be a list of top points, then of matched bottom points, so
 * points[0] is on top top points[points.n/2], and so on.
 * Currently only round caps are applied.
 */
void EngraverFillData::BezApproximate(Laxkit::NumStack<flatvector> &fauxpoints, Laxkit::NumStack<flatvector> &points)
{
	// There are surely better ways to do this. Not sure how powerstroke does it.
	// This is not simplied/optimized at all. Each point gets control points to smooth it out.
	// no fancy corner handling done yet

    fauxpoints.flush();

    flatvector v,p, pp,pn;
	flatvector opn, opp;


    double sx;
	//caps are at points index 0 and points.n/2
	
    for (int c=0; c<points.n; c++) {
        p=points.e[c];

		if (c==0) {
			 //on first cap
			opp=p+3*.5522*(points.e[points.n-1]-points.e[1])/2;
			opn=p+3*.5522*(points.e[1]-points.e[points.n-1])/2;

		} else if (c==points.n/2) {
			 //on final cap
			opp=p+3*.5522*(points.e[c-1]-points.e[c+1])/2;
			opn=p+3*.5522*(points.e[c+1]-points.e[c-1])/2;

		} else {
			if (c==1 || c==points.n/2+1) {
				pp=(p+points.e[points.n-c])/2;
				v=points.e[c-1]-pp;
				opp=p+3*.5522*v;
			} else opp=points.e[c-1];

			if (c==points.n-1 || c==points.n/2-1) {
				pp=(p+points.e[points.n-c])/2;
				v=points.e[(c+1)%points.n]-pp;
				opn=p+3*.5522*v;
			} else opn=points.e[c+1];
		}

        v=opn-opp;
        v.normalize();

        sx=norm(p-opp)*.333;
        pp=p - v*sx;

        sx=norm(opn-p)*.333;
        pn=p + v*sx;

        fauxpoints.push(pp);
        fauxpoints.push(p);
        fauxpoints.push(pn);

    }
}



////------------------------------ EngraverTraceSettings -------------------------------

EngraverTraceSettings::EngraverTraceSettings()
{
	continuous_trace=false; 
	group=-1;
	traceobj_opacity=1;
	traceobject=NULL;
	tw=th=0;
	trace_ref_bw=NULL;
	identifier=NULL;

	trace_sample_cache=NULL;
	samplew=sampleh=0;
}

EngraverTraceSettings::~EngraverTraceSettings()
{
	if (traceobject) traceobject->dec_count();
	delete[] identifier;
}

void EngraverTraceSettings::ClearCache(bool obj_too)
{
	delete[] identifier;
	identifier=NULL;

	delete[] trace_sample_cache;
	trace_sample_cache=NULL;
	samplew=sampleh=0;

	if (obj_too) {
		traceobject->dec_count();
		traceobject=NULL;
	}
}


////------------------------------ EngraverFillInterface -------------------------------


/*! \class EngraverFillInterface
 * \ingroup interfaces
 * \brief Interface for dealing with EngraverFillData objects.
 *
 * \todo *** select multiple datas to adjust. Mesh tinker only on one of them, touch up on many
 */

enum EngraveShortcuts {
	ENGRAVE_SwitchMode=PATCHA_MAX,
	ENGRAVE_SwitchModeR,
	ENGRAVE_ExportSvg,
	ENGRAVE_RotateDir,
	ENGRAVE_RotateDirR,
	ENGRAVE_SpacingInc,
	ENGRAVE_SpacingDec,
	ENGRAVE_ShowPoints,
	ENGRAVE_MorePoints,
	ENGRAVE_ToggleTrace,
	ENGRAVE_MAX
};

enum EngraveControls {
	 //on canvas controls:
	ENGRAVE_None=0,

	ENGRAVE_Trace_Box,
	ENGRAVE_Trace_Weight_Map,
	ENGRAVE_Trace_Once,
	ENGRAVE_Trace_Load,
	ENGRAVE_Trace_Clear,
	ENGRAVE_Trace_Continuous,
	ENGRAVE_Trace_Object,
	ENGRAVE_Trace_Opacity,
	ENGRAVE_Trace_Identifier,
	ENGRAVE_Trace_Map,
	ENGRAVE_Trace_Move_Mesh,

	ENGRAVE_Orient,
	ENGRAVE_Orient_Spacing,
	ENGRAVE_Orient_Position,
	ENGRAVE_Orient_Direction,

	 //modes:
	EMODE_Controls,
	EMODE_Mesh,
	EMODE_Thickness,
	EMODE_Orientation,
	EMODE_Freehand,
	EMODE_Blockout,
	EMODE_Drag, 
	EMODE_PushPull,
	EMODE_AvoidToward,
	EMODE_Twirl,
	EMODE_Turbulence,
	EMODE_Trace,
	EMODE_Resolution //change sample point distribution

};

EngraverFillInterface::EngraverFillInterface(int nid,Displayer *ndp)
  : PatchInterface(nid,ndp),
	curvemapi(0,ndp)
{
	primary=1;
	//selection=NULL;

	showdecs=SHOW_Points|SHOW_Edges;
	rendermode=3;
	recurse=0;
	edata=NULL;

	default_spacing=1./25;
	default_zero_threshhold=0;
	default_broken_threshhold=0;

	show_points=0;
	submode=0;
	mode=controlmode=EMODE_Mesh;


	curvemapi.owner=this;
	curvemapi.ChangeEditable(CurveMapInterface::YMax, 1);
	brush_radius=40;

	makestr(thickness.title,_("Brush Ramp"));
	thickness.SetSinusoidal(6);
	thickness.RefreshLookup();

	//DBG CurveWindow *ww=new CurveWindow(NULL,"curve","curve",0,0,0,400,400,0,NULL,0,NULL);
	//DBG ww->SetInfo(&thickness);
	//DBG app->addwindow(ww);

	whichcontrols=Patch_Coons;

	continuous_trace=false;
	show_trace=false;


	modes.AddItem(_("Mesh mode"),                                              NULL, EMODE_Mesh         );
	modes.AddItem(_("Thickness, shift for brush size, control to thin"),       NULL, EMODE_Thickness    );
	modes.AddItem(_("Blockout mode, shift for brush size, control to turn on"),NULL, EMODE_Blockout     );
	modes.AddItem(_("Drag mode, shift for brush size"),                        NULL, EMODE_Drag         );
	modes.AddItem(_("Push or pull. Shift for brush size"),                     NULL, EMODE_PushPull     );
	//modes.AddItem(_("Avoid or pull toward. Shift for brush size"),             NULL, EMODE_AvoidToward  );
	modes.AddItem(_("Twirl, Shift for brush size"),                            NULL, EMODE_Twirl        );
	modes.AddItem(_("Randomly push sample points"),                            NULL, EMODE_Turbulence   );
	//modes.AddItem(_("Add or remove sample points"),                            NULL, EMODE_Resolution   );
	modes.AddItem(_("Orientation mode"),                                       NULL, EMODE_Orientation  );
	modes.AddItem(_("Freehand mode"),                                          NULL, EMODE_Freehand     );
	modes.AddItem(_("Trace adjustment mode"),                                  NULL, EMODE_Trace        );

	fgcolor.rgbf(0.,0.,0.);
	bgcolor.rgbf(1.,1.,1.);
}

//! Empty destructor.
EngraverFillInterface::~EngraverFillInterface() 
{
	DBG cerr<<"-------"<<whattype()<<","<<" destructor"<<endl;
	//if (selection) selection->dec_count();
}

const char *EngraverFillInterface::Name()
{ return _("Engraver Fill Tool"); }

anInterface *EngraverFillInterface::duplicate(anInterface *dup)//dup=NULL;
{
	EngraverFillInterface *dupp;
	if (dup==NULL) dupp=new EngraverFillInterface(id,NULL);
	else dupp=dynamic_cast<EngraverFillInterface *>(dup);
	if (!dupp) return NULL;

	dupp->recurse=recurse;
	dupp->rendermode=rendermode;
	return PatchInterface::duplicate(dupp);
}

//! Return new local EngraverFillData
PatchData *EngraverFillInterface::newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	EngraverFillData *ndata=NULL;
	if (somedatafactory) {
		ndata=dynamic_cast<EngraverFillData *>(somedatafactory->newObject(LAX_ENGRAVERFILLDATA));
	} 
	if (!ndata) ndata=new EngraverFillData();//creates 1 count

	ndata->Set(xx,yy,ww,hh,nr,nc,stle);
	//ndata->renderdepth=-recurse;
	//ndata->xaxis(3*ndata->xaxis());
	//ndata->yaxis(3*ndata->yaxis());
	//ndata->origin(flatpoint(xx,yy));
	//ndata->xaxis(flatpoint(1,0)/Getmag()*100);
	//ndata->yaxis(flatpoint(0,1)/Getmag()*100);

	ndata->style|=PATCH_SMOOTH;
	ndata->FillRegularLines(1./dp->Getmag(), -1);
	default_spacing=ndata->default_spacing;
	ndata->Sync();
	ndata->FindBBox();
	return ndata;
}

//! id==4 means make recurse=ndata.
int EngraverFillInterface::UseThis(int id,int ndata)
{
	if (id!=4) return PatchInterface::UseThis(id,ndata);
	char blah[100];
	if (id==4) { // recurse depth
		if (ndata>0) {
			recurse=ndata;
			sprintf(blah,_("New Recurse Depth %d: "),recurse);
			PostMessage(blah);
			if (rendermode>0) needtodraw=1; 
		}
		return 1;
	}
	return 0;
}

int EngraverFillInterface::UseThisObject(ObjectContext *oc)
{
	int c=PatchInterface::UseThisObject(oc);
	edata=dynamic_cast<EngraverFillData *>(data);
	return c;
}

/*! Accepts EngraverFillData, or ImageInfo.
 *
 * Returns 1 to used it, 0 didn't
 *
 * If nobj is an ImageInfo, then change the file, cacheimage, description of the current data..
 */
int EngraverFillInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask) // assumes not use local
{
    if (!nobj) return 0;

	if (dynamic_cast<EngraverFillData *>(nobj)) { 
		return PatchInterface::UseThis(nobj,mask);

	} else if (dynamic_cast<LineStyle *>(nobj) && edata) {
        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);
		edata->fillstyle.color=nlinestyle->color;
        needtodraw=1;
        return 1;
	}


	return 0;
}

void EngraverFillInterface::deletedata()
{
	PatchInterface::deletedata();
	edata=NULL;
}


int EngraverFillInterface::scanEngraving(int x,int y, int *category)
{
	*category=0;

	if ((show_trace || mode==EMODE_Trace)) {
		double th=dp->textheight();
		double pad=2;

	    if (tracebox.boxcontains(x,y)) {
			*category=ENGRAVE_Trace_Box;

			if(y<tracebox.miny+2+th*4/3) {
				if (x<(tracebox.minx+tracebox.maxx)/2)
					return ENGRAVE_Trace_Once;
				else return ENGRAVE_Trace_Continuous;
			}
			if (y>tracebox.maxy-pad-th) return ENGRAVE_Trace_Identifier;
			if (y>tracebox.maxy-pad-2*th) return ENGRAVE_Trace_Opacity;
			if (x>tracebox.minx+1.5*th && y<tracebox.maxy-pad-2*th) return ENGRAVE_Trace_Map;
			return ENGRAVE_Trace_Box;
	    }

		if (edata) {
			flatpoint p=screentoreal(x,y); //p is in edata->parent space
			if (edata->pointin(p)) return ENGRAVE_Trace_Move_Mesh;

			//Affine a=edata->GetTransformToContext(true, 0);
			//p=a.transformPoint(p);
			p=dp->screentoreal(x,y);
			if (trace.traceobject && trace.traceobject->pointin(p)) {
				return ENGRAVE_Trace_Object;
			}
		}
	}

	if (mode==EMODE_Orientation) {
		*category=ENGRAVE_Orient;
		//ENGRAVE_Orient_Direction
		//ENGRAVE_Orient_Position
		//ENGRAVE_Orient_Spacing
	}


	return ENGRAVE_None;
}

//! Catch a double click to pop up an ImageDialog.
int EngraverFillInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (child) return 1;

	 //catch trace box overlay first
	if (lasthover==ENGRAVE_Trace_Box
		 || lasthover==ENGRAVE_Trace_Load
		 || lasthover==ENGRAVE_Trace_Opacity
		 || lasthover==ENGRAVE_Trace_Identifier
		 || lasthover==ENGRAVE_Trace_Map
		 || lasthover==ENGRAVE_Trace_Once
		 || lasthover==ENGRAVE_Trace_Continuous) {

		if ((state&LAX_STATE_MASK)!=0) lasthover=ENGRAVE_Trace_Box;

		//if (count==2 && lasthover==ENGRAVE_Trace_Map) {
		if (lasthover==ENGRAVE_Trace_Map) {
			double pad=2;
			double th=dp->textheight();
			curvemapi.Dp(dp);
			curvemapi.SetInfo(&trace.value_to_weight);

			DoubleBBox box;
			box.minx=tracebox.minx+2*th+2*pad;
			box.miny=tracebox.miny+4./3*th;
			box.maxx=tracebox.maxx-pad;
			box.maxy=tracebox.maxy-pad-2*th-1.5*th;
			curvemapi.SetupRect(box.minx,box.miny, box.maxx-box.minx,box.maxy-box.miny);

			child=&curvemapi;
			child->inc_count();
			if (curvemapi.LBDown(x,y,state,count,d)==1) {
				child->dec_count();
				child=NULL;
				lasthover=ENGRAVE_Trace_Box;
			}
			//-----------
			//CurveWindow *ww=new CurveWindow(NULL,"curve","curve",0,0,0,400,400,0,NULL,object_id,"valuemap");
			//ww->ChangeEditable(CurveWindow::YMax, 1);
			//ww->SetInfo(&trace.value_to_weight);
			//app->addwindow(ww);

			needtodraw=1;
			return 0;
		}

		buttondown.down(d->id,LEFTBUTTON,x,y,lasthover);
		controlmode=EMODE_Controls;
		needtodraw=1;

		if (lasthover==ENGRAVE_Trace_Opacity) {
			double pad=2;
			trace.traceobj_opacity=(x-(tracebox.minx+pad))/(tracebox.maxx-tracebox.minx-2*pad);
			if (trace.traceobj_opacity<0) trace.traceobj_opacity=0;
			else if (trace.traceobj_opacity>1) trace.traceobj_opacity=1;
			cout << " *** need to implement actual trace object opacity"<<endl;
			needtodraw=1;
		}

		return 0;
	}

	if (mode==EMODE_Trace) {
		 //we haven't clicked on the tracing box, so search for images to grab..
		//RectInterface *rect=new RectInterface(0,dp);
		//rect->style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
		//dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
		//rect->owner=this;
        //rect->UseThis(&base_cells,0);
        //child=rect;
        //AddChild(rect,0,1);

		if (lasthover==ENGRAVE_Trace_Object || lasthover==ENGRAVE_Trace_Move_Mesh) {
			buttondown.down(d->id,LEFTBUTTON,x,y,lasthover);
			controlmode=EMODE_Controls;
			needtodraw=1;
			return 0;
		}

		if (!trace.traceobject) {
			SomeData *obj=NULL;
			ObjectContext *oc=NULL;
			int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
			if (c>0) obj=oc->obj;

			if (obj && obj!=edata) {
				 //set up proxy object
				SomeDataRef *ref=dynamic_cast<SomeDataRef*>(LaxInterfaces::somedatafactory->newObject("SomeDataRef"));
				ref->Set(obj, false);
				ref->flags|=SOMEDATA_KEEP_ASPECT;
				double m[6]; //,m2[6],m3[6];
				viewport->transformToContext(m,oc,0,1);
				//viewport->transformToContext(m2,poc,0,1);//of current mesh
				//transform_invert(m3,m2);
				//transform_mult(m2,m,m3);
				//ref->m(m2);
				ref->m(m);

				trace.traceobject=ref;

				trace.ClearCache(false);
				delete[] trace.identifier;
				trace.identifier=new char[strlen(_("ref: %s"))+strlen(ref->thedata_id)+1];
				sprintf(trace.identifier,_("ref: %s"),ref->thedata_id);

				needtodraw=1;
			}
		}


		return 0;
	}

	if (mode==EMODE_Freehand) {
		FreehandInterface *freehand=new FreehandInterface(this,-1,dp);
		freehand->freehand_style=FREEHAND_Color_Mesh|FREEHAND_Remove_On_Up;
		viewport->Push(freehand,-1,0);
		freehand->LBDown(x,y,state,count,d);
		child=freehand;
		return 0;
	}
	
	if (!edata) ChangeMode(EMODE_Mesh);

	if (mode==EMODE_Mesh) {
		int c=PatchInterface::LBDown(x,y,state,count,d);
		if (!edata && data) edata=dynamic_cast<EngraverFillData*>(data);
		return c;
	}

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  ) {
		if (count==2) {
			 // *** in future, should be on symmetric brush ramp editing
			CurveMapInterface *ww=new CurveMapInterface(-1,dp,_("Brush Ramp"));
			ww->SetInfo(&thickness);

			child=ww;
            ww->owner=this;
			int pad=(dp->Maxx-dp->Minx)*.1;
			ww->SetupRect(dp->Minx+pad,dp->Miny+pad, dp->Maxx-dp->Minx-2*pad,dp->Maxy-dp->Miny-2*pad);
            viewport->Push(ww,-1,0);
			submode=0;
			return 0;
		}
		buttondown.down(d->id,LEFTBUTTON, x,y, mode);
		return 0;
	}

	return 0;
}
	
int EngraverFillInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (child) {
		if (child==&curvemapi) { //to be here, curvemapi must have taken the lbdown
			child->LBUp(x,y,state,d);
			child->dec_count();
			child=NULL;
			needtodraw=1;
			return 0;
		}
		return 1;
	}

	 //catch trace box overlay first
	int over=0;
	buttondown.getextrainfo(d->id,LEFTBUTTON, &over);

	if (over==ENGRAVE_Trace_Box
		 || over==ENGRAVE_Trace_Once
		 || over==ENGRAVE_Trace_Load
		 || lasthover==ENGRAVE_Trace_Object
		 || lasthover==ENGRAVE_Trace_Move_Mesh
		 || lasthover==ENGRAVE_Trace_Opacity
		 || lasthover==ENGRAVE_Trace_Identifier
		 || lasthover==ENGRAVE_Trace_Continuous) {

		buttondown.up(d->id,LEFTBUTTON);
		controlmode=mode;

		if (lasthover==over && over==ENGRAVE_Trace_Continuous) {
			continuous_trace=!continuous_trace;
			if (continuous_trace) Trace();

		} else if (lasthover==over && over==ENGRAVE_Trace_Identifier) {
			if (!trace.identifier) {
				app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));
			} else {
				if (trace.traceobject) trace.ClearCache(true);
			}

		} else if (lasthover==over && over==ENGRAVE_Trace_Once) {
			Trace();

		} else if (lasthover==over && over==ENGRAVE_Trace_Load) {
			needtodraw=1;
			app->rundialog(new FileDialog(NULL,"Export Svg",_("Export engraving to svg"),
									  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));
		}

		needtodraw=1;
		return 0;
	}

	if (mode==EMODE_Mesh) {
		PatchInterface::LBUp(x,y,state,d);
		if (!edata && data) edata=dynamic_cast<EngraverFillData*>(data);
		if (edata) edata->Sync();
		//if (continuous_trace) Trace(); ...done in move
		return 0;
	}

	if (mode==EMODE_Freehand) {
//		if (child) {
//			RemoveChild();
//			needtodraw=1;
//		}
		return 0;
	}

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  ) {
		buttondown.up(d->id,LEFTBUTTON);

		if ( mode==EMODE_Drag
		  || mode==EMODE_Turbulence
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl)
			edata->ReverseSync(true);

		 //...done in move:
		//if (continuous_trace && dragged && (mode==EMODE_Thickness)) continuous_trace=false;
		//if (continuous_trace) Trace();

		needtodraw=1;
	}

	return 0;
}

void EngraverFillInterface::ChangeMessage(int forwhich)
{
	if (forwhich==ENGRAVE_Trace_Once) PostMessage(_("Trace once"));
	else if (forwhich==ENGRAVE_Trace_Load) PostMessage(_("Load an image to trace"));
	else if (forwhich==ENGRAVE_Trace_Continuous) PostMessage(_("Toggle continuous tracing"));
	else if (forwhich==ENGRAVE_Trace_Opacity) PostMessage(_("Trace object opacity"));
	else if (forwhich==ENGRAVE_Trace_Identifier) {
		if (trace.identifier) PostMessage(_("Click to remove trace object"));
		else PostMessage(_("Click to load an image to trace"));

	}
}

int EngraverFillInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (child) {
		if (child==&curvemapi) {
			child->MouseMove(x,y,state,d);
			if (continuous_trace) Trace();

			needtodraw=1;
			return 0;
		}
		return 1;
	}

	hover.x=x;
	hover.y=y;
	int category=0;
	int newhover= scanEngraving(x,y, &category);
	if (newhover!=lasthover) {
		lasthover=newhover;
		ChangeMessage(lasthover);
		needtodraw=1;
	}
	DBG cerr <<"eng lasthover: "<<lasthover<<endl;

	if (controlmode==EMODE_Controls) {
		if (buttondown.any()) {
			int over=0,lx,ly;
			buttondown.getextrainfo(d->id,LEFTBUTTON, &over);
			buttondown.move(d->id,x,y, &lx,&ly);

			if (over== ENGRAVE_Trace_Object) {
				if ((state&LAX_STATE_MASK)==ControlMask) {
					 //scale trace object
					double s=1+.01*(x-lx);
					if (s<.8) s=.8;
					for (int c=0; c<4; c++) {
						trace.traceobject->m(c,trace.traceobject->m(c)*s);
					}

				} else {
					flatpoint p=dp->screentoreal(x,y) - dp->screentoreal(lx,ly);
					trace.traceobject->origin(trace.traceobject->origin()+p);
				}
				if (continuous_trace) Trace();
				needtodraw=1;

			} else if (over==ENGRAVE_Trace_Move_Mesh) {
				flatpoint p=screentoreal(x,y)-screentoreal(lx,ly);
				edata->origin(edata->origin()+p);

				if (continuous_trace) Trace();
				needtodraw=1;

			} else if (over==ENGRAVE_Trace_Opacity) {
				double pad=2;
				trace.traceobj_opacity=(x-(tracebox.minx+pad))/(tracebox.maxx-tracebox.minx-2*pad);
				if (trace.traceobj_opacity<0) trace.traceobj_opacity=0;
				else if (trace.traceobj_opacity>1) trace.traceobj_opacity=1;
				needtodraw=1;

			} else if (over==ENGRAVE_Trace_Box) {
				if ((state&LAX_STATE_MASK)==ControlMask) {
					 //scale up box...
					double s=1+.01*(x-lx);
					if (s<.8) s=.8;
					tracebox.maxx=tracebox.minx+s*(tracebox.maxx-tracebox.minx);
					tracebox.maxy=tracebox.miny+s*(tracebox.maxy-tracebox.miny);

				} else {
					tracebox.minx+=x-lx;
					tracebox.maxx+=x-lx;
					tracebox.miny+=y-ly;
					tracebox.maxy+=y-ly;
				}
				needtodraw=1;
			}
		}
		return 0;
	}

	if (mode==EMODE_Freehand && !child) {
		needtodraw=1;
		return 0;
	}

	if (mode==EMODE_Mesh) {
		PatchInterface::MouseMove(x,y,state,d);
		if (buttondown.any() && curpoints.n>0) edata->Sync();

		if (continuous_trace) Trace();
		return 0;

	}
	
	if (    mode==EMODE_Thickness
		 || mode==EMODE_Blockout
		 || mode==EMODE_Turbulence
		 || mode==EMODE_Drag
		 || mode==EMODE_PushPull
		 || mode==EMODE_Twirl
		 ) {

		if (!buttondown.any()) {
			needtodraw=1;
			return 0;
		}

		int lx,ly;
		buttondown.move(d->id,x,y, &lx,&ly);

		if ((state&LAX_STATE_MASK)==ShiftMask) {
			 //change brush size
			brush_radius+=(x-lx)*2;
			if (brush_radius<5) brush_radius=5;
			needtodraw=1;

		} else {
			flatvector m=screentoreal(x,y);
			m=transform_point_inverse(edata->m(),m); //center of brush
			flatvector m2=screentoreal(x+brush_radius,y);
			m2=transform_point_inverse(edata->m(),m2); //on edge of brush radius


			double rr=norm2(m2-m); //radius of brush in object coordinates
			double d, a;
			LinePoint *l;
			flatpoint dv=m-transform_point_inverse(edata->m(),screentoreal(lx,ly));
			flatpoint pp;
			double nearzero=.001; // *** for when tracing makes a value at 0, thicken makes it this

			for (int c=0; c<edata->lines.n; c++) {
				l=edata->lines.e[c];
				while (l) {
					d=norm2(l->p - m); //distance point to brush center

					if (d<rr) { //point is within...

						if (mode==EMODE_Thickness) {
							a=sqrt(d/rr);
							a=thickness.f(a);

							if ((state&LAX_STATE_MASK)==ControlMask) {
								 //thin
								a=1-a*.05;
							} else {
								 //thicken
								a=1+a*.05;
								if (l->weight<=0) l->weight=nearzero;
							}
							l->weight*=a;

						} else if (mode==EMODE_Blockout) {
							if ((state&LAX_STATE_MASK)==ControlMask) 
								l->on=true;
							else l->on=false;

						} else if (mode==EMODE_Drag) {
							a=sqrt(d/rr);
							a=thickness.f(a);
							l->p+=dv*a; //point without mesh
							l->needtosync=2;

						} else if (mode==EMODE_Turbulence) {
							a=sqrt(d/rr);
							a=thickness.f(a);
							l->p+=rotate(dv,drand48()*2*M_PI);
							l->needtosync=2;

						} else if (mode==EMODE_PushPull) {
							a=sqrt(d/rr);
							a=thickness.f(a);
							pp=(l->p-m)*.03;

							if ((state&LAX_STATE_MASK)==ControlMask) {
								l->p-=pp*a*d/rr;
							} else {
								l->p+=pp*a;
							}
							l->needtosync=2;

						} else if (mode==EMODE_Twirl) {
							a=sqrt(d/rr);
							a=thickness.f(a);

							if ((state&LAX_STATE_MASK)==ControlMask) {
								l->p=m+rotate(l->p-m,a*.1);
							} else {
								l->p=m+rotate(l->p-m,-a*.1);
							}
							l->needtosync=2;

						} else if (mode==EMODE_AvoidToward) {
						}
					}

					l=l->next;
				}//foreach point in line
			} //foreach line
			needtodraw=1;

			if (continuous_trace && (mode==EMODE_Thickness)) continuous_trace=false;
			if (continuous_trace) Trace();
		}

		return 0;
	}

	return 0;
}

//! Checks for EngraverFillData, then calls PatchInterface::DrawData(ndata,a1,a2,info).
int EngraverFillInterface::DrawData(Laxkit::anObject *ndata,anObject *a1,anObject *a2,int info) // info=0
{
	if (!ndata || dynamic_cast<EngraverFillData *>(ndata)==NULL) return 1;

	EngraverFillData *ee=edata;
	edata=dynamic_cast<EngraverFillData *>(ndata);
	int c=PatchInterface::DrawData(ndata,a1,a2,info);
	edata=ee;

	return c;
}

int EngraverFillInterface::Refresh()
{
	if (!needtodraw) return 0;

	if (mode==EMODE_Freehand && !child) {
		 //draw squiggly lines near mouse
		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->NewFG(0.,0.,1.);
		dp->DrawScreen();
		double s=10;
		for (int c=-1; c<2; c++) {
			dp->moveto(hover-flatpoint(2*s,c*s));
			dp->curveto(hover-flatpoint(s*1.5,c*s+5), hover+flatpoint(-s/2,-c*s+s/2), hover+flatpoint(0,-c*s));
			dp->stroke(0);
		}
		dp->DrawReal();
		needtodraw=0;
		return 0;
	}

	if (!edata) { needtodraw=0; return 0; }


	 //draw the trace object if necessary
	if (trace.traceobject && trace.traceobj_opacity>.5 // **** .5 since actual opacity not working
			&& (mode==EMODE_Trace || show_trace)) {

		Affine a=edata->GetTransformToContext(true, 0);//supposed to be inverse from edata to base real
		dp->PushAndNewTransform(a.m());
		dp->PushAndNewTransform(trace.traceobject->m());
		viewport->DrawSomeData(trace.traceobject, NULL,NULL,0);
		dp->PopAxes();
		dp->PopAxes();

	} else if (trace.traceobject) {
		 //draw outline
		dp->NewFG(.9,.9,.9);
		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

		Affine a=edata->GetTransformToContext(true, 0);
		dp->PushAndNewTransform(a.m());
		SomeData *o=trace.traceobject;
		dp->moveto(transform_point(o->m(),flatpoint(o->minx,o->miny)));
		dp->lineto(transform_point(o->m(),flatpoint(o->maxx,o->miny)));
		dp->lineto(transform_point(o->m(),flatpoint(o->maxx,o->maxy)));
		dp->lineto(transform_point(o->m(),flatpoint(o->minx,o->maxy)));
		dp->closed();
		dp->stroke(0);
		dp->PopAxes();
	}


	 //----draw the actual lines
	LinePoint *l;
	LinePoint *last=NULL;

	double mag=dp->Getmag();
	double lastwidth, neww;
	double tw;
	flatpoint lp,v;

	dp->NewFG(&edata->fillstyle.color);

	for (int c=0; c<edata->lines.n; c++) {
		l=edata->lines.e[c];
		last=NULL;
		lastwidth=-1;

		while (l) {
			if (!last) {
				 //establish a first point of a visible segment
	
				//if (l->on && l->width>data->zero_threshhold) {
				//if (l->on) {
				if (edata->PointOn(l)) {
					last=l;
					lastwidth=l->weight*mag;
					dp->LineAttributes(lastwidth,LineSolid,LAXCAP_Round,LAXJOIN_Round);
				}
				l=l->next;
				continue;
			}

			if (!edata->PointOn(l)) {
				if (!last->prev || !edata->PointOn(last->prev)) {
					 //draw just a single dot
					dp->drawline(last->p,last->p);
				}
				last=NULL;
				lastwidth=-1;
				l=l->next;
				continue;
			}

			neww=l->weight*mag;
			if (neww!=lastwidth) {
				lp=last->p;
				v=(l->p-last->p)/9.;
				for (int c2=1; c2<10; c2++) {
					tw=lastwidth+c2/9.*(neww-lastwidth);
					dp->LineAttributes(tw,LineSolid,LAXCAP_Round,LAXJOIN_Round);
					dp->drawline(lp+v*(c2-1), lp+v*c2);
				}

				lastwidth=neww;

			} else {
				if (edata->PointOn(last) && edata->PointOn(l)) dp->drawline(last->p,l->p);
			}

			last=l;
			l=l->next;

		}

		if (show_points) {
			 //show little red dots for all the sample points
			flatpoint pp;
			l=edata->lines.e[c];
			dp->NewFG(1.,0.,0.);
			int p=1;
			char buffer[50];
			while (l) {
				dp->drawpoint(l->p, 2, 1);
				if (show_points==2) {
					sprintf(buffer,"%d,%d",c,p);
					dp->textout(l->p.x,l->p.y, buffer,-1, LAX_BOTTOM|LAX_HCENTER);
					p++;
				}
				l=l->next;
			}
			dp->NewFG(&edata->fillstyle.color);
		}
	}


	 //draw other tool decorations
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

	 //draw outline of mesh
	if (data->npoints_boundary) {
		dp->NewFG(150,150,150);
		dp->LineAttributes(1,LineSolid,linestyle.capstyle,linestyle.joinstyle);
		dp->drawbez(data->boundary_outline,data->npoints_boundary/3,1,0);
	}
		
	if (mode==EMODE_Mesh) {
		PatchInterface::Refresh();

	} else if (mode==EMODE_Orientation) {
		flatpoint center((edata->minx+edata->maxx)/2, (edata->miny+edata->maxy)/2);
		orient_position=center;

		flatpoint yy=edata->getPoint(.5,.55)-edata->getPoint(.5,.5);
		yy=yy/norm(yy)*edata->default_spacing;
		flatpoint xx=-transpose(yy)*4;

		dp->LineAttributes(5,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->NewFG(1.,0.,0.);
		dp->drawline(center, center+yy);
		dp->NewFG(0.,1.,0.);
		dp->drawline(center, center+xx);

	} else if (mode==EMODE_Thickness
			|| mode==EMODE_Blockout
			|| mode==EMODE_Drag
			|| mode==EMODE_Turbulence
			|| mode==EMODE_PushPull
		    || mode==EMODE_Twirl
			) {


		dp->DrawScreen();

		 //set colors
		dp->NewFG(.5,.5,.5,1.);
		if (mode==EMODE_Thickness) {
			dp->LineAttributes(2,LineSolid,linestyle.capstyle,linestyle.joinstyle);
			dp->NewFG(.5,.5,.5,1.);
		} else if (mode==EMODE_Turbulence) dp->NewFG(.5,.5,.5,1.);
		else if (mode==EMODE_Drag || mode==EMODE_PushPull || mode==EMODE_Twirl) {
			if (submode==2) dp->NewFG(.5,.5,.5);
			else dp->NewFG(0.,0.,.7,1.);
		} else if (mode==EMODE_Blockout) { //blockout
			if (submode==1) dp->NewFG(0,200,0);
			else if (submode==2) dp->NewFG(.5,.5,.5);
			else dp->NewFG(255,100,100);
		}

		 //draw circle
		if (mode==EMODE_Turbulence) {
			 //draw jagged circle
			double xx,yy, r;
			for (int c=0; c<30; c++) {
				r=1 + .2*drand48()-.1;
				xx=hover.x + brush_radius*r*cos(c*2*M_PI/30);
				yy=hover.y + brush_radius*r*sin(c*2*M_PI/30);
				dp->lineto(xx,yy);
			}
			dp->closed();
			dp->stroke(0);

		} else if (mode==EMODE_Twirl) {
			int s=1;
			if (submode==1) s=-1;
			for (int c=0; c<10; c++) {
				dp->moveto(hover.x+brush_radius*cos(s*c/10.*2*M_PI), hover.y+brush_radius*sin(s*c/10.*2*M_PI));
				dp->curveto(flatpoint(hover.x+brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+brush_radius*sin(s*(c+1)/10.*2*M_PI)),
							flatpoint(hover.x+.85*brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+.85*brush_radius*sin(s*(c+1)/10.*2*M_PI)),
							flatpoint(hover.x+.85*brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+.85*brush_radius*sin(s*(c+1)/10.*2*M_PI)));
				dp->stroke(0);
			}

		} else if (mode==EMODE_PushPull) {
			dp->drawpoint(hover.x,hover.y, brush_radius,0);

			dp->LineAttributes(1,LineOnOffDash, LAXCAP_Butt, LAXJOIN_Miter);
			if (submode==1) dp->drawpoint(hover.x,hover.y, brush_radius*.85,0);
			else dp->drawpoint(hover.x,hover.y, brush_radius*1.10,0);
			dp->LineAttributes(1,LineSolid, LAXCAP_Butt, LAXJOIN_Miter);

		} else {
			 //draw plain old circle
			dp->drawpoint(hover.x,hover.y, brush_radius,0);
		}

		if (mode==EMODE_Blockout) dp->drawpoint(hover.x,hover.y, brush_radius*.85,0); //second inner circle

		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		if (submode==2) { //brush size change arrows
			dp->drawarrow(hover+flatpoint(brush_radius+10,0), flatpoint(20,0), 0, 20, 1, 3);
			dp->drawarrow(hover-flatpoint(brush_radius+10,0), flatpoint(-20,0), 0, 20, 1, 3);
		}

		dp->DrawReal();
	}

	if (show_trace || mode==EMODE_Trace) DrawTracingTools();

	needtodraw=0;
	return 0;
}

/*! Draw the range of line widths in a strip, as for a curve map control.
 */
void EngraverFillInterface::DrawLineGradient(double minx,double maxx,double miny,double maxy)
{
	double sp=(maxy - miny)/10;
	double minsp=edata->zero_threshhold/edata->default_spacing*sp;

	dp->NewFG(&fgcolor);
	dp->DrawScreen();

	for (int c=0; c<10; c++) {
		dp->LineAttributes(minsp+(sp-minsp)*(10-c)/10., LineSolid, CapButt, JoinMiter);
		dp->drawline(minx,sp/2+miny+(maxy-miny)*c/10.,
		             maxx,sp/2+miny+(maxy-miny)*c/10.);
	}

	//dp->drawrectangle(minx-1,miny-1, maxx-minx+2,maxy-miny+2,0);
	dp->DrawReal();
}

/*! Draw a continuous tone gradient in a strip, as for a curve map control.
 */
void EngraverFillInterface::DrawShadeGradient(double minx,double maxx,double miny,double maxy)
{
	dp->DrawScreen();
	ScreenColor col;

	dp->LineAttributes(2, LineSolid, CapButt, JoinMiter);
	for (int c=minx; c<maxx; c+=2) {
		dp->NewFG(coloravg(rgbcolor(0,0,0),rgbcolor(255,255,255), 1-(c-minx)/(maxx-minx)));
		dp->drawline(c,miny, c,maxy);
	}

	//dp->drawrectangle(minx-1,miny-1, maxx-minx+2,maxy-miny+2,0);
	dp->DrawReal();
}

void EngraverFillInterface::DrawTracingTools()
{
	dp->DrawScreen();

	double uiscale=1;
	double th=dp->textheight();
	double r=th*2/3;
	int pad=2;

	if (!tracebox.validbounds()) {
		tracebox.minx=tracebox.miny=10;
		tracebox.maxx=6*1.5*th;
		tracebox.maxy=6*1.5*th+th+4./3*th+th+pad;
	}

	 //blank out trace controls rect
	ScreenColor col;
	coloravg(&col, &fgcolor,&bgcolor, .9);
	dp->NewFG(&col);
	dp->drawrectangle(tracebox.minx,tracebox.miny, tracebox.maxx-tracebox.minx,tracebox.maxy-tracebox.miny, 1);

	coloravg(&col, &fgcolor,&bgcolor, .95);
	dp->NewFG(&col);
	if (lasthover==ENGRAVE_Trace_Once) {
		dp->drawrectangle(tracebox.minx,tracebox.miny, (tracebox.maxx-tracebox.minx)/2,2*r+pad, 1);

	} else if (lasthover==ENGRAVE_Trace_Continuous) {
		dp->drawrectangle(tracebox.minx+(tracebox.maxx-tracebox.minx)/2,tracebox.miny+pad, (tracebox.maxx-tracebox.minx)/2,2*r, 1);

	} else if (lasthover==ENGRAVE_Trace_Opacity) {
		dp->drawrectangle(tracebox.minx+pad,tracebox.maxy-pad-2*th, (tracebox.maxx-tracebox.minx),th, 1);

	} else if (lasthover==ENGRAVE_Trace_Identifier) {
		if (trace.identifier) {
			ScreenColor red;
			red.rgbf(1.,0.,0.);
			coloravg(&col,&bgcolor,&red,.1);
			dp->NewFG(&col);
		}
		dp->drawrectangle(tracebox.minx+pad,tracebox.maxy-pad-th, (tracebox.maxx-tracebox.minx),th, 1);
	}


	DoubleBBox box;
	box.setbounds(&tracebox);

	dp->LineAttributes(3,LineSolid, CapButt, JoinMiter);

	 //continuous trace circle
	if (continuous_trace) dp->NewFG(0,200,0); else dp->NewFG(255,100,100);
	dp->drawellipse((tracebox.minx+tracebox.maxx)/2+th/2+r,pad+tracebox.miny+r,
                        r*uiscale,r*uiscale,
                        0,2*M_PI,
                        0);

	 //single trace square
	if (lasthover==ENGRAVE_Trace_Once) dp->NewFG(0,200,0); else dp->NewFG(255,100,100);
	dp->drawrectangle((tracebox.minx+tracebox.maxx)/2-th/2-2*r, pad+tracebox.miny, r*2,r*2, 0);

	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);
	dp->NewFG(.5,.5,.5);
	dp->drawrectangle(tracebox.minx,tracebox.miny,tracebox.maxx-tracebox.minx,tracebox.maxy-tracebox.miny, 0);


	 //draw opacity slider
	dp->drawline(tracebox.minx+pad,tracebox.maxy-pad-1.5*th, tracebox.maxx-2*pad, tracebox.maxy-pad-1.5*th);
	dp->drawpoint(flatpoint(tracebox.minx+pad + trace.traceobj_opacity*(tracebox.maxx-tracebox.minx-2*pad),tracebox.maxy-pad-1.5*th), th/3, 1);

	dp->textout(tracebox.minx+pad,tracebox.maxy-pad, 
			trace.identifier ? trace.identifier : "...",-1,
			LAX_LEFT|LAX_BOTTOM);



	box.miny+=2*r+2*pad;
	box.minx+=pad;
	box.maxx-=pad;
	box.maxy-=pad+2*th;
	int ww=2*th;
	DrawLineGradient(box.minx,box.minx+ww, box.miny,box.maxy-ww);
	DrawShadeGradient(box.minx+ww,box.maxx, box.maxy-ww,box.maxy);

	box.minx+=ww;
	box.maxy-=ww; //now box is where the value to weight curve goes

	//if (child!=&curvemapi) {
		curvemapi.Dp(dp);
		curvemapi.SetInfo(&trace.value_to_weight);
		curvemapi.SetupRect(box.minx,box.miny, box.maxx-box.minx,box.maxy-box.miny);
		curvemapi.needtodraw=1;
		curvemapi.Refresh();
	//}

	dp->DrawReal();
}

int EngraverFillInterface::PerformAction(int action)
{
	if (action==PATCHA_RenderMode) {
		if (rendermode==0) rendermode=1;
		else if (rendermode==1) rendermode=2;
		else rendermode=0;

		if (rendermode==0) PostMessage(_("Render with grid"));
		else if (rendermode==1) PostMessage(_("Render with preview"));
		else if (rendermode==2) PostMessage(_("Render recursively"));

		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_SwitchMode || action==ENGRAVE_SwitchModeR) {

		int i=modes.findIndex(mode);

		if (action==ENGRAVE_SwitchMode) {
			i++;
			if (i>=modes.n()) i=0;
		} else {
			i--;
			if (i<0) i=modes.n()-1;
		}

		ChangeMode(modes.e(i)->id);

		submode=0;
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ExportSvg) {
		app->rundialog(new FileDialog(NULL,"Export Svg",_("Export engraving to svg"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"exportsvg",FILES_SAVE, "out.svg"));
		return 0;

	} else if (action==ENGRAVE_RotateDir || action==ENGRAVE_RotateDirR) {
		edata->direction=rotate(edata->direction, (action==ENGRAVE_RotateDir ? M_PI/12 : -M_PI/12), 0);
		edata->FillRegularLines(1./dp->Getmag(),edata->default_spacing);
		edata->Sync();
		if (continuous_trace) Trace();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_SpacingInc || action==ENGRAVE_SpacingDec) {
		if (action==ENGRAVE_SpacingInc) edata->default_spacing*=1.1; else edata->default_spacing*=.9;
		edata->FillRegularLines(1./dp->Getmag(),edata->default_spacing);
		edata->Sync();
		if (continuous_trace) Trace();
		DBG cerr <<"new spacing: "<<edata->default_spacing<<endl;
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ShowPoints) {
		show_points++;
		if (show_points>2) show_points=0;
		if (show_points==2) PostMessage(_("Show sample points with numbers"));
		else if (show_points==1) PostMessage(_("Show sample points"));
		else PostMessage(_("Don't show sample points"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_MorePoints) {
		edata->MorePoints();
		edata->ReverseSync(true);
		if (continuous_trace) Trace();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleTrace) {
		show_trace=!show_trace;
		if (show_trace) continuous_trace=false;
		if (show_trace) PostMessage(_("Show tracing controls"));
		else PostMessage(_("Don't show tracing controls"));
		needtodraw=1;
		return 0;
	}


	return PatchInterface::PerformAction(action);
}

int EngraverFillInterface::Trace()
{
	if (!trace.traceobject || !edata) return 1;

	if (!trace.trace_sample_cache) {
 		 //we need to render the trace object to a grayscale sample board

		double w,h;
		w=trace.traceobject->maxx-trace.traceobject->minx;
		h=trace.traceobject->maxy-trace.traceobject->miny;
		if (w<500 && h<500) {
			if (w<h) {
				double a=w/h;
				h=500;
				w=h*a;
			} else {
				double a=h/w;
				w=500;
				h=500*a;
			}
		}


		Displayer *ddp=newDisplayer(NULL);
		ddp->CreateSurface((int)w,(int)h);

		 // setup ddp to have proper scaling...
		ddp->NewTransform(1.,0.,0.,-1.,0.,0.);
		//ddp->NewTransform(1.,0.,0.,1.,0.,0.);
		DoubleBBox bbox;
		bbox.addtobounds(trace.traceobject);
		ddp->SetSpace(bbox.minx,bbox.maxx,bbox.miny,bbox.maxy);
		ddp->Center(bbox.minx,bbox.maxx,bbox.miny,bbox.maxy);

		ddp->NewBG(255,255,255); // *** this should be the paper color for paper the page is on...
		ddp->NewFG(0,0,0,255);
		//ddp->m()[4]=0;
		//ddp->m()[5]=2*h;
		//ddp->Newmag(w/(bbox.maxx-bbox.minx));
		ddp->ClearWindow();


		viewport->DrawSomeData(ddp,trace.traceobject, NULL,NULL,0);
		ddp->EndDrawing();

		LaxImage *img=ddp->GetSurface();
		if (!img) {
			DBG cerr <<"could not render trace object"<<endl;
			return 1;
		}

		trace.samplew=img->w();
		trace.sampleh=img->h();
		trace.trace_sample_cache=new unsigned char[4*trace.samplew*trace.sampleh];

		unsigned char *data=img->getImageBuffer();
		memcpy(trace.trace_sample_cache, data, 4*trace.samplew*trace.sampleh);
		img->doneWithBuffer(data);

		// **** DBG:
		LaxImlibImage *iimg=dynamic_cast<LaxImlibImage*>(img);
		imlib_context_set_image(iimg->image);
	    imlib_image_set_format("png");
		imlib_save_image("trace.png");
	}

	int samplew=trace.samplew;
	int sampleh=trace.sampleh;

	int x,y, i;
	int sample, samplea;
	double me[6],mti[6];
	unsigned char *rgb;
	double a;

	Affine aa=edata->GetTransformToContext(false, 0);//supposed to be from edata to base real
	SomeData *to=trace.traceobject;
	transform_invert(mti,to->m());
	transform_mult(me, aa.m(),mti);

	flatpoint pp;
	for (int c=0; c<edata->lines.n; c++) {
		LinePoint *l=edata->lines.e[c];

		while (l) {
			pp=transform_point(me,l->p);
			//pp=l->p;
			//pp=transform_point(mti,pp);

			x=samplew*(pp.x-to->minx)/(to->maxx-to->minx);
			y=sampleh*(pp.y-to->miny)/(to->maxy-to->miny);

			if (x>=0 && x<samplew && y>=0 && y<sampleh) {
				i=4*(x+(sampleh-y)*samplew);
				rgb=trace.trace_sample_cache+i;

				samplea=rgb[3];
				sample=0.3*rgb[0] + 0.59*rgb[1] + 0.11*rgb[2];
				if (sample>255) {
					sample=255;
				}

				a=(255-sample)/255.;
				a=trace.value_to_weight.f(a);
				l->weight=edata->default_spacing*a;
				l->on = samplea>0 ? true : false;
			} else {
				l->weight=0;
				l->on=false;
			}

			l=l->next;
		}
	}

	needtodraw=1;
	return 0;
}

/*! Return old value of mode.
 */
int EngraverFillInterface::ChangeMode(int newmode)
{
	if (newmode==mode) return mode;

	int c=0;
	for (c=0; c<modes.n(); c++) {
		if (modes.e(c)->id==newmode) break;
	}
	if (c==modes.n()) return mode;

	int oldmode=mode;
	mode=newmode;

	if (mode==EMODE_Trace) { continuous_trace=false; }

	needtodraw=1;
	PostMessage(modes.e(c)->name);
	return oldmode;

}

int EngraverFillInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int i =s->info2; //id of menu item
		
		if ( i==EMODE_Mesh
		  || i==EMODE_Thickness
		  || i==EMODE_Orientation
		  || i==EMODE_Freehand
		  || i==EMODE_Blockout
		  || i==EMODE_Drag 
		  || i==EMODE_PushPull
		  || i==EMODE_AvoidToward
		  || i==EMODE_Twirl
		  || i==EMODE_Turbulence
		  || i==EMODE_Trace
		  || i==EMODE_Resolution) {

			ChangeMode(i);
			return 0;
		}

		if (i==ENGRAVE_Trace_Load) {
			app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));
			return 0;

		} else if (i==ENGRAVE_Trace_Clear) {
			if (trace.traceobject) trace.ClearCache(true);
			return 0;
		}

		return 0;

	} else if (!strcmp(mes,"valuemap")) {
		 //in floating curve window, value map was changed...
		 // *** this needs to be changed to the on canvas interface
		if (!edata) return 0;

        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s) return 0;
		if (continuous_trace) Trace();
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"exportsvg")) {
        if (!edata) return 0;

        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
        if (!s) return 1;
        if (!isblank(s->str)) {
			edata->dump_out_svg(s->str);
			PostMessage(_("Exported."));
		}
        return 0;

	} else if (!strcmp(mes,"loadimage")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) return 0;
		LaxImage *img=load_image(s->str);
		const char *bname=lax_basename(s->str);
		if (!img) {
			char buf[strlen(_("Could not load %s"))+strlen(bname)+1];
			sprintf(buf,_("Could not load %s"),bname);
			PostMessage(buf);
			return 0;
		}

		int sx=dp->Maxx-dp->Minx;
		int sy=dp->Maxy-dp->Miny;
		flatpoint p1=screentoreal(dp->Minx+sx*.1,dp->Miny+sy*.1);
		flatpoint p2=screentoreal(dp->Maxx-sx*.1,dp->Maxy-sy*.1);
		DoubleBBox box;
		box.addtobounds(p1);
		box.addtobounds(p2);

		if (trace.traceobject) {
			trace.traceobject->dec_count();
			trace.traceobject=NULL;
		}
		trace.traceobject=new ImageData(s->str);
		trace.traceobject->fitto(NULL,&box,50,50,2);

		trace.ClearCache(false);
		delete[] trace.identifier;
		trace.identifier=new char[strlen(_("img: %s"))+strlen(bname)+1];
		sprintf(trace.identifier,_("img: %s"),bname);

		continuous_trace=false;

		needtodraw=1;
		PostMessage(_("Image to trace loaded."));
		return 0;

	} else if (!strcmp(mes,"FreehandInterface")) {
		 //got new freehand mesh

        const RefCountedEventData *s=dynamic_cast<const RefCountedEventData *>(e_data);
		if (!s) return 1;

		PatchData *patch=dynamic_cast<PatchData*>(const_cast<anObject*>(s->TheObject()));
		if (!patch) return 1;


		deletedata();
		EngraverFillData *newdata=dynamic_cast<EngraverFillData*>(newPatchData(0,0,1,1,1,1,PATCH_SMOOTH));
		newdata->m(patch->m());
		newdata->CopyMeshPoints(patch);
		if (viewport) {
			ObjectContext *oc=NULL;
			viewport->NewData(newdata,&oc);

			if (oc) poc=oc->duplicate();
		}
		data=newdata;
		data->linestyle=linestyle;
		data->FindBBox();
		curpoints.flush();

		edata=dynamic_cast<EngraverFillData*>(data);
		edata->Sync();

		needtodraw=1;
		return 0;
    }

    return 1;
}


Laxkit::MenuInfo *EngraverFillInterface::ContextMenu(int x,int y,int deviceid)
{
	if (child) return NULL;

	MenuInfo *menu=new MenuInfo();

	int category=0;
	int where=scanEngraving(x,y, &category);
	if (mode==EMODE_Trace
		 || where==ENGRAVE_Trace_Box
		 || where==ENGRAVE_Trace_Once
		 || where==ENGRAVE_Trace_Load
		 || where==ENGRAVE_Trace_Continuous) {
		menu->AddSep(_("Trace"));
		menu->AddItem(_("Load image to trace..."),ENGRAVE_Trace_Load, LAX_OFF);
		menu->AddItem(_("Clear trace object"), ENGRAVE_Trace_Clear, LAX_OFF);
	}

	menu->AddSep(_("Mode"));
	MenuItem *i;
	for (int c=0; c<modes.n(); c++) {
		i=modes.e(c);
		menu->AddItem(i->name, i->id, LAX_OFF|LAX_ISTOGGLE|(mode==i->id ? LAX_CHECKED : 0));
	}

	return menu;
}


Laxkit::ShortcutHandler *EngraverFillInterface::GetShortcuts()
{
    if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

	PatchInterface::GetShortcuts();

	 //convert all patch shortcuts to EMODE_Mesh mode
	WindowAction *a;
	ShortcutDef *s;
	for (int c=0; c<sc->NumActions(); c++)   { a=sc->Action(c);   a->mode=EMODE_Mesh; }
	for (int c=0; c<sc->NumShortcuts(); c++) { s=sc->Shortcut(c); s->mode=EMODE_Mesh; }

	 //any mode shortcuts
	sc->Add(ENGRAVE_SwitchMode,  'm',0,0,          "SwitchMode",  _("Switch edit mode"),NULL,0);
	sc->Add(ENGRAVE_SwitchModeR, 'M',ShiftMask,0,  "SwitchModeR", _("Switch to previous edit mode"),NULL,0);
	sc->Add(ENGRAVE_ExportSvg,   'f',0,0,          "ExportSvg",   _("Export Svg"),NULL,0);
	sc->Add(ENGRAVE_RotateDir,   'r',0,0,          "RotateDir",   _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_RotateDirR,  'R',ShiftMask,0,  "RotateDirR",  _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_SpacingInc,  's',0,0,          "SpacingInc",  _("Increase default spacing"),NULL,0);
	sc->Add(ENGRAVE_SpacingDec,  'S',ShiftMask,0,  "SpacingDec",  _("Decrease default spacing"),NULL,0);
	sc->Add(ENGRAVE_ShowPoints,  'p',0,0,          "ShowPoints",  _("Toggle showing sample points"),NULL,0);
	sc->Add(ENGRAVE_MorePoints,  'p',ControlMask,0,"MorePoints",  _("Subdivide all lines to have more sample points"),NULL,0);
	sc->Add(ENGRAVE_ToggleTrace, 't',0,0,          "ToggleTrace", _("Toggle showing tracing controls"),NULL,0);

	return sc;
}

int EngraverFillInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"in EngraverFillInterface::CharInput"<<endl;
	
	if (child) return 1;

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  ) {

		if (ch==LAX_Control) {
			submode=1;
			needtodraw=1;
			return 0;
		} else if (ch==LAX_Shift) {
			submode=2;
			needtodraw=1;
			return 0;
		}
	}

    if (!sc) GetShortcuts();
    int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
    if (action>=0) {
        return PerformAction(action);
    }


	if (mode==EMODE_Mesh) return PatchInterface::CharInput(ch,buffer,len,state,d);


	return 1;
}

int EngraverFillInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (child) return 1;

	if (mode==EMODE_Mesh) return PatchInterface::KeyUp(ch,state,d);

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  ) {

		if (ch==LAX_Control || ch==LAX_Shift) {
			submode=0;
			needtodraw=1;
			return 0;
		}
	}

	return 1;
}


} // namespace LaxInterfaces

