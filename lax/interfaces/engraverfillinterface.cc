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


// ******* todo *********
//
// Needs clip boundary
// rotate direction on canvas controller
// point generators
// mesh controls are really bad
// need mode based shortcuts
// reasonable caps
//
// DONE direction for default lines not installing correctly when downward
// DONE curved lines, not polylines
//



#include <lax/interfaces/engraverfillinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/dumpcontext.h>
#include <lax/imagedialog.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <lax/fileutils.h>
#include <lax/filedialog.h>

#include <lax/lists.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {

extern double B[16];
extern double Binv[16];




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

	direction=flatvector(1,0);

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
			l->p=getPoint(l->s,l->t); // *** note this is hideously inefficient
			l->needtosync=false;

			l=l->next;
		}
	}
}

void EngraverFillData::FillRegularLines(double weight, double spacing)
{
	 // create generic lines to experiment with weight painting...
	if (spacing<=0) spacing=1./25;

	lines.flush();
	nlines=0;
	LinePoint *p;

	//double totalh=spacing*n;
	if (weight<=0) weight=spacing/10; // *** weight is actual distance, not s,t!!

	flatvector v=direction;
	if (v.x<0) v=-v;
	v.normalize();
	flatvector vt=transpose(v);
	vt*=spacing;

	if (v.y<0) lines.push(new LinePoint(0,1,weight)); //push a (0,0) starter point
	else       lines.push(new LinePoint(0,0,weight)); //push a (0,0) starter point

	 //starter points along y
	double sp;
	if (vt.y) {
		sp=fabs(spacing*spacing/vt.y);
		if (sp<1) {
			for (double yy=sp; yy<=1; yy+=sp) {
				if (v.y<0) lines.push(new LinePoint(0,1-yy, weight));
				else       lines.push(new LinePoint(0,yy, weight));
			}
		}
	}

	 //starter points along x
	if (vt.x) {
		sp=fabs(spacing*spacing/vt.x);
		if (sp<1) {
			for (double xx=sp; xx<=1; xx+=sp) {
				if (v.y<0) lines.push(new LinePoint(xx,1, weight));
				else       lines.push(new LinePoint(xx,0, weight));
			}
		}
	}

	 //grow lines
	flatvector pp;
	for (int c=0; c<lines.n; c++) {
		p=lines.e[c];

		while (p->s>=0 && p->t>=0 && p->s<=1 && p->t<=1) {
			pp=flatpoint(p->s,p->t) + spacing*v;
			p->next=new LinePoint(pp.x, pp.y, weight);
			p->next->prev=p;
			p=p->next;
		}

		nlines++;
	}
}

/*! Like FillRegularLines(), but assumes direction==(1,0).
 */
void EngraverFillData::FillRegularLinesHorizontal(double weight)
{
	 // create generic lines to experiment with weight painting...
	int n=25;
	double spacing=10;
	//double dim=n*spacing;

	lines.flush();
	nlines=0;
	LinePoint *p;

	//double totalh=spacing*n;
	if (weight<=0) weight=1./n/spacing;

	for (int c=0; c<n; c++) {
		if (c>=lines.n) lines.push(new LinePoint);
		p=lines.e[c];
		p->Clear();
		p->s=0;
		p->t=c/(double)n;
		p->weight=weight;

		for (int c2=1; c2<=n; c2++) {
			p->next=new LinePoint(c2/(double)n, c/(double)n, weight);
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
{ // ***
	PatchData::dump_out(f,indent,what,context);
	return;
//	
//	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 
//	if (what==-1) {
//		fprintf(f,"%sfilename whicheverfile.jpg  #name of the image used\n", spc);
//		fprintf(f,"%siwidth  100  #width of the image in pixels for a preview sampling\n",spc);
//		fprintf(f,"%siheight 100  #iheight of the image in pixels for a preview sampling\n",spc);
//		return;
//	}
//	DumpContext *dump=dynamic_cast<DumpContext *>(context);
//	if (dump && dump->basedir) {
//		char *tmp=NULL;
//		if (filename) {
//			if (!dump->subs_only || (dump->subs_only && is_in_subdir(filename,dump->basedir)))
//				tmp=relative_file(filename,dump->basedir,1);
//			fprintf(f,"%sfilename \"%s\"\n",spc,tmp?tmp:filename);
//			if (tmp) { delete[] tmp; tmp=NULL; }
//		}
//	} else fprintf(f,"%sfilename \"%s\"\n",spc,filename);
//	fprintf(f,"%siwidth %d\n",spc,iwidth);
//	fprintf(f,"%siheight %d\n",spc,iheight);
}

//! Reverse of dump_out.
void EngraverFillData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{ // ***
	PatchData::dump_in_atts(att,flag,context);
	return;
//	if (!att) return;
//	char *name,*value;
//	int c;
//	PatchData::dump_in_atts(att,flag,context);
//	for (c=0; c<att->attributes.n; c++) {
//		name= att->attributes.e[c]->name;
//		value=att->attributes.e[c]->value;
//		if (!strcmp(name,"filename")) {
//			DumpContext *dump=dynamic_cast<DumpContext *>(context);
//			if (value && *value!='/' && dump && dump->basedir) {
//				if (filename) delete[] filename;
//				filename=full_path_for_file(value,dump->basedir);
//			} else makestr(filename,value);
//		} else if (!strcmp(name,"iwidth")) {
//			IntAttribute(value,&iwidth);
//		} else if (!strcmp(name,"iheight")) {
//			IntAttribute(value,&iheight);
//		}
//	}
//	SetImage(filename);
//	FindBBox();
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
				"   width=\"744.09448\"\n"
				"   height=\"1052.3622\"\n"
				"   id=\"svg2\"\n"
				"   version=\"1.1\"\n"
				"   inkscape:version=\"0.48+devel r custom\"\n"
				"   viewBox=\"0 0 744.09448 1052.3622\"\n"
				"   sodipodi:docname=\"test-powerstroke.svg\">\n"
				"  <defs id=\"defs4\">\n"
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
	fprintf(f,  "  <g id=\"themesh\"\n>");
 
	// *** output paths
	//  <path d="....the full outline....."
	//        inkscape:path-effect="path-effect4064"
	//        inkscape:original-d="....original path...."

	NumStack<flatvector> points;
	NumStack<flatvector> points2;

	LinePoint *l, *last, *last2;
	flatvector t, tp;
	flatvector p1,p2;
	//double lastwidth;
	//double neww;

	for (int c=0; c<lines.n; c++) {
		points.flush();
		points2.flush();

		l=lines.e[c];
		last=l;
		last2=NULL;
		//lastwidth=l->weight;

		l=l->next;

		while (l) {
			if (!last2) last2=last;

			tp=l->p - last2->p;
			tp.normalize();
			t=transpose(tp);

			//neww=last->weight;

			p1=last->p + last->weight/2*t;
			p2=last->p - last->weight/2*t;

			if (last==lines.e[c]) {
				 //first point cap
				points.push(last->p-last->weight/2*tp);
				points.push(p1-last->weight*.001*tp);
				points.push(p2-last->weight*.001*tp);
			}
			points.push(p1);
			points.push(p2);

			last2=last;
			last=l;
			l=l->next;
		}

		 //do last point... last2 -> last
		points.push(last->p + last->weight/2*t);
		points.push(last->p - last->weight/2*t);

		 //final point cap
		points.push(last->p + last->weight/2*t + last->weight*.001*tp);
		points.push(last->p - last->weight/2*t + last->weight*.001*tp);
		points.push(last->p + last->weight/2*tp);

		if (points.n) {
			 //convert to bez approximation
			points2.push(points.e[0]); //initial cap
			for (int c2=1; c2<points.n-1; c2+=2)    points2.push(points.e[c2]);

			points2.push(points.e[points.n-1]); //final cap
			for (int c2=points.n-2; c2>0; c2-=2) points2.push(points.e[c2]);
			

			BezApproximate(points,points2);


			fprintf(f,"    <path d=\"");
			fprintf(f,"M %f %f ", points.e[0].x,points.e[0].y);
			for (int c2=1; c2<points.n-2; c2+=3) {
				fprintf(f,"C %f %f %f %f %f %f ",
					points.e[c2+1].x,points.e[c2+1].y,
					points.e[c2+2].x,points.e[c2+2].y,
					points.e[c2+3].x,points.e[c2+3].y);
			}

//			for (int c2=0; c2<points.n; c2+=2) {
//				if (c2==0) fprintf(f,"M "); else fprintf(f,"L ");
//
//				fprintf(f,"%f %f ", points.e[c2].x,points.e[c2].y);
//			}
//			for (int c2=points.n-1; c2>=0; c2-=2) {
//				if (c2==0) fprintf(f,"M "); else fprintf(f,"L ");
//
//				fprintf(f,"%f %f ", points.e[c2].x,points.e[c2].y);
//			}

			fprintf(f,"z \" />\n");
		}
	}


	fprintf(f,  "  </g>\n"
				" </g>\n"
				"</svg>\n");


	fclose(f);
}


void EngraverFillData::BezApproximate(Laxkit::NumStack<flatvector> &fauxpoints, Laxkit::NumStack<flatvector> &points)
{
    fauxpoints.flush();

    flatvector v,p, pp,pn;
	flatvector opn, opp;

    //fauxpoints.push(points.e[0]); //ignored control point
    //fauxpoints.push(points.e[0]);
    //fauxpoints.push(points.e[0]);

    int c=1;
    double sx;
    for ( ; c<points.n; c++) {
		if (c==0) opp=points.e[points.n-1]; else opp=points.e[c-1];
		if (c==points.n-1) opn=points.e[0]; else opn=points.e[c+1];


        v=opn-opp;
        v.normalize();

        p=points.e[c];
        sx=norm(p-opp)*.333;
        pp=p - v*sx;

        sx=norm(opn-p)*.333;
        pn=p + v*sx;

        fauxpoints.push(pp);
        fauxpoints.push(p);
        fauxpoints.push(pn);

    }
    //fauxpoints.push(points.e[c]);
    //fauxpoints.push(points.e[c]); //the final point
    //fauxpoints.push(points.e[c]); //ignored final point
}


////------------------------------ EngraverFillInterface -------------------------------


/*! \class EngraverFillInterface
 * \ingroup interfaces
 * \brief Interface for dealing with EngraverFillData objects.
 *
 * *** select multiple datas to adjust. Mesh tinker only on one of them, touch up on many
 */

enum EngraverModes {
	EMODE_Mesh,
	EMODE_Thickness,
	EMODE_Orientation,
	EMODE_Freehand
};

EngraverFillInterface::EngraverFillInterface(int nid,Displayer *ndp) : PatchInterface(nid,ndp)
{
	showdecs=SHOW_Points|SHOW_Edges;
	rendermode=3;
	recurse=0;
	edata=NULL;
	default_spacing=1./25;

	mode=EMODE_Mesh;
	//mode=EMODE_Thickness;
	//mode=EMODE_Freehand;

	brush_radius=40;

	thickness.curvetype=CurveInfo::Bezier;
	thickness.SetSinusoidal(5);
	//thickness.AddPoint(0,1);
	//thickness.AddPoint(1,0);
	thickness.RefreshLookup();

	whichcontrols=Patch_Coons;
}

//! Empty destructor.
EngraverFillInterface::~EngraverFillInterface() 
{
	DBG cerr<<"-------"<<whattype()<<","<<" destructor"<<endl;
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
	ndata->FillRegularLines(1./dp->Getmag(), default_spacing);
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

	}
	return 0;
}

void EngraverFillInterface::deletedata()
{
	PatchInterface::deletedata();
	edata=NULL;
}


//! Catch a double click to pop up an ImageDialog.
int EngraverFillInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (!edata) mode=EMODE_Mesh;

	if (mode==EMODE_Mesh) {
		int c=PatchInterface::LBDown(x,y,state,count,d);
		if (!edata && data) edata=dynamic_cast<EngraverFillData*>(data);
		return c;
	}

	if (mode==EMODE_Freehand) {
		return 0;
	}
	
	if (mode==EMODE_Thickness) {
		buttondown.down(d->id,LEFTBUTTON, x,y);
	}

	return 0;
}
	
int EngraverFillInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (mode==EMODE_Mesh) return PatchInterface::LBUp(x,y,state,d);

	if (mode==EMODE_Thickness) {
		buttondown.up(d->id,LEFTBUTTON);
	}

	return 0;
}

int EngraverFillInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	hover.x=x;
	hover.y=y;

	if (mode==EMODE_Mesh) {
		PatchInterface::MouseMove(x,y,state,d);
		if (buttondown.any() && curpoints.n>0) edata->Sync();
		return 0;

	}
	
	if (mode==EMODE_Thickness) {
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
			m=transform_point_inverse(edata->m(),m);
			flatvector m2=screentoreal(x+brush_radius,y);
			m2=transform_point_inverse(edata->m(),m2);


			double rr=norm2(m2-m);
			double d, a;
			LinePoint *l;

			for (int c=0; c<edata->lines.n; c++) {
				l=edata->lines.e[c];
				while (l) {
					d=norm2(l->p - m);
					if (d<rr) {
						 //point is within
						a=sqrt(d/rr);
						//a=1-thickness.f(a);
						a=thickness.f(a);
						if ((state&LAX_STATE_MASK)==ControlMask) {
							a=1-a*.01;
						} else {
							a=1+a*.01;
						}
						l->weight*=a;
					}

					l=l->next;
				}
			}
			needtodraw=1;
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
	if (!edata) { needtodraw=0; return 0; }


	dp->NewFG(0.,0.,1.,1.);

	LinePoint *l;
	LinePoint *last=NULL, *last2=NULL;
	flatvector t; 

	double mag=dp->Getmag();
	double lastwidth, neww;

	for (int c=0; c<edata->lines.n; c++) {
		l=edata->lines.e[c];
		last=l;
		lastwidth=l->weight*mag;
		dp->LineAttributes(lastwidth,LineSolid,LAXCAP_Round,LAXJOIN_Round);

		l=l->next;

		while (l) {
			if (!last2) last2=last;

			t=l->p - last2->p;

			neww=last->weight*mag;
			if (neww!=lastwidth) {
				lastwidth=neww;
				dp->LineAttributes(lastwidth,LineSolid,LAXCAP_Round,LAXJOIN_Round);
			}
			dp->drawline(last->p,l->p);

			last2=last;
			last=l;
			l=l->next;
		}
	}

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

	if (mode==EMODE_Mesh) PatchInterface::Refresh();
	else if (mode==EMODE_Thickness) {
		dp->DrawScreen();
		dp->NewFG(.5,.5,.5,1.);
		dp->drawpoint(hover.x,hover.y, brush_radius,0);
		dp->DrawReal();
	}

	needtodraw=0;
	return 0;
}

enum EngraveShortcuts {
	ENGRAVE_SwitchMode=PATCHA_MAX,
	ENGRAVE_ExportSvg,
	ENGRAVE_RotateDir,
	ENGRAVE_RotateDirR,
	ENGRAVE_SpacingInc,
	ENGRAVE_SpacingDec,
	ENGRAVE_MAX
};

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

	} else if (action==ENGRAVE_SwitchMode) {
		if (mode==EMODE_Mesh) mode=EMODE_Thickness;
		else if (mode==EMODE_Thickness) mode=EMODE_Mesh;

		//EMODE_Freehand

		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ExportSvg) {
		app->rundialog(new FileDialog(NULL,"Export Svg",_("Export engraving to svg"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"exportsvg",FILES_SAVE, "out.svg"));
		return 0;

	} else if (action==ENGRAVE_RotateDir || action==ENGRAVE_RotateDirR) {
		edata->direction=rotate(edata->direction, (action==ENGRAVE_RotateDir ? M_PI/12 : -M_PI/12), 0);
		edata->FillRegularLines(1./dp->Getmag(),default_spacing);
		edata->Sync();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_SpacingInc || action==ENGRAVE_SpacingDec) {
		if (action==ENGRAVE_SpacingInc) default_spacing*=1.1; else default_spacing*=.9;
		edata->FillRegularLines(1./dp->Getmag(),default_spacing);
		edata->Sync();
		DBG cerr <<"new spacing: "<<default_spacing<<endl;
		needtodraw=1;
		return 0;

	}


	return PatchInterface::PerformAction(action);
}

int EngraverFillInterface::Event(const Laxkit::EventData *data, const char *mes)
{
    if (!strcmp(mes,"exportsvg")) {
        if (!data) return 0;

        const StrEventData *s=dynamic_cast<const StrEventData *>(data);
        if (!s) return 1;
        if (!isblank(s->str)) {
			edata->dump_out_svg(s->str);
			PostMessage(_("Exported."));
		}
        return 0;
    }

    return 1;
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
	sc->Add(ENGRAVE_ExportSvg,   'f',0,0,          "ExportSvg",   _("Export Svg"),NULL,0);
	sc->Add(ENGRAVE_RotateDir,   'r',0,0,          "RotateDir",   _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_RotateDirR,  'R',ShiftMask,0,  "RotateDirR",  _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_SpacingInc,  's',0,0,          "SpacingInc",  _("Increase default spacing"),NULL,0);
	sc->Add(ENGRAVE_SpacingDec,  'S',ShiftMask,0,  "SpacingDec",  _("Decrease default spacing"),NULL,0);

	return sc;
}

int EngraverFillInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"in EngraverFillInterface::CharInput"<<endl;
	

	if (mode==EMODE_Mesh) return PatchInterface::CharInput(ch,buffer,len,state,d);


    if (!sc) GetShortcuts();
    int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
    if (action>=0) {
        return PerformAction(action);
    }


	return 1;
}

int EngraverFillInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (mode==EMODE_Mesh) return PatchInterface::KeyUp(ch,state,d);
	return 1;
}


} // namespace LaxInterfaces

