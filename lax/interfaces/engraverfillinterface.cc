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
// rotate direction
// point generators
// curved lines, not polylines
// mesh controls are really bad
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
	usepreview=1;

	direction.x=1;

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

void EngraverFillData::FillRegularLines(double weight)
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
		//p->weight=weight /spacing*2;

		for (int c2=1; c2<=n; c2++) {
			//p->next=new LinePoint(c2/(double)n, c/(double)n, weight * (c2)/spacing*2);
			p->next=new LinePoint(c2/(double)n, c/(double)n, weight);
			p->next->prev=p;
			p=p->next;
		}

		nlines++;
	}

	//zap(flatpoint(0,0), flatpoint(n*spacing,0), flatpoint(0,n*spacing));
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
 
	// *** output paths
	//  <path d="....the full outline....."
	//        inkscape:path-effect="path-effect4064"
	//        inkscape:original-d="....original path...."

	NumStack<flatvector> points;

	LinePoint *l, *last, *last2;
	flatvector t, tp;
	flatvector p1,p2;
	//double lastwidth;
	//double neww;

	for (int c=0; c<lines.n; c++) {
		points.flush();

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

			points.push(p1);
			points.push(p2);

			last2=last;
			last=l;
			l=l->next;
		}

		// *** do last point... last2 -> last
		points.push(last->p + last->weight/2*t);
		points.push(last->p - last->weight/2*t);

		if (points.n) {
			fprintf(f,"    <path d=\"");
			for (int c2=0; c2<points.n; c2+=2) {
				if (c2==0) fprintf(f,"M "); else fprintf(f,"L ");

				fprintf(f,"%f %f ", points.e[c2].x,points.e[c2].y);
			}
			for (int c2=points.n-1; c2>=0; c2-=2) {
				if (c2==0) fprintf(f,"M "); else fprintf(f,"L ");

				fprintf(f,"%f %f ", points.e[c2].x,points.e[c2].y);
			}

			fprintf(f,"\" />\n");
		}
	}


	fprintf(f,  " </g>\n"
				"</svg>\n");


	fclose(f);
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
	EMODE_Freehand
};

EngraverFillInterface::EngraverFillInterface(int nid,Displayer *ndp) : PatchInterface(nid,ndp)
{
	rendermode=1;
	recurse=0;

	//mode=EMODE_Thickness;
	mode=EMODE_Mesh;
	//mode=EMODE_Freehand;

	brush_radius=40;

	thickness.curvetype=CurveInfo::Bezier;
	thickness.AddPoint(0,1);
	thickness.AddPoint(1,0);
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

	ndata->FillRegularLines(1./dp->Getmag());
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

//! Catch a double click to pop up an ImageDialog.
int EngraverFillInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
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
						a=1-thickness.f(a);
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
	return PatchInterface::DrawData(ndata,a1,a2,info);
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
	}

	return PatchInterface::PerformAction(action);
}

Laxkit::ShortcutHandler *EngraverFillInterface::GetShortcuts()
{
    if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

	PatchInterface::GetShortcuts();

	sc->Add(ENGRAVE_SwitchMode,       'm',0,0,          "SwitchMode",  _("Switch edit mode"),NULL,0);

	return sc;
}

int EngraverFillInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"in EngraverFillInterface::CharInput"<<endl;
	
	if (edata && ch=='f') {
		 // ***
		edata->dump_out_svg("out.svg");
		return 0;
	}


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

