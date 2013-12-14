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
//    Copyright (C) 2013 by Tom Lechner
//


#include <lax/interfaces/imagepatchinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/dumpcontext.h>
#include <lax/imagedialog.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <lax/fileutils.h>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {

extern double B[16];
extern double Binv[16];




//------------------------------------- LinePoint ------------------------
class LinePoint
{
  public:
	double s,t;
	flatpoint p; //(s,t) transformed by the mesh
	double weight;

	LinePoint *next;
	LinePoint() { s=t=0; weight=1; next=NULL; }
	LinePoint(double ss, double tt, double ww) { next=NULL; s=ss; t=tt; weight=ww; }

	void Clear();
};

void LinePoint::Clear()
{
	if (next) delete next;
	next=NULL;
}



//------------------------------------- EngraverFillData ------------------------


/*! \class EngraverFillData
 * \ingroup interfaces
 *
 * See EngraverFillInterface.
 */


//! Create a patch with the provided color data, set in unit rectangle x:[0,1] and y:[0,1].
EngraverFillData::EngraverFillData(int iwidth, int iheight, unsigned long *ndata,int disl,
							   double nscale,unsigned int stle)
	: PatchData(0,0,1,1,1,1,0)
{
	usepreview=1;
}

void EngraverFillData::FillRegularLines()
{
	 // create generic lines to experiment with weight painting...
	int n=50;
	double spacing=10;
	double dim=n*spacing;

	lines.flush();
	nlines=50;
	LinePoint *p;

	for (int c=0; c<n; c++) {
		if (c>=nlines) lines.push(new LinePoint);
		p=lines.e[c];
		p->Clear();
		p->s=0;
		p->t=c*spacing;
		p->weight=1;

		for (int c2=1; c2<n; c2++) {
			p->next=new LinePoint(c2*spacing, c*spacing, 1);
			p=p->next;
		}
	}

	zap(flatpoint(0,0), flatpoint(n*spacing,0), flatpoint(0,n*spacing));
}

EngraverFillData::~EngraverFillData()
{
}

SomeData *EngraverFillData::duplicate(SomeData *dup)
{
	EngraverFillData *p=dynamic_cast<EngraverFillData*>(dup);
	if (!p && !dup) return NULL; //was not EngraverFillData!

	char set=1;
	if (!dup && somedatafactory) {
		dup=somedatafactory->newObject(LAX_ENGRAVERFILLDATA,this);
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			set=0;
		}
		p=dynamic_cast<EngraverFillData*>(dup);
	} 
	if (!p) {
		p=new EngraverFillData();
		dup=p;
	}

	return PatchData::duplicate(dup);
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
	return NULL;
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
	mode=EMODE_Thickness;
	//mode=EMODE_Mesh;
	//mode=EMODE_Freehand;
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
	EngraverFillData *cpd=NULL;
	if (somedatafactory) {
		cpd=dynamic_cast<EngraverFillData *>(somedatafactory->newObject(LAX_ENGRAVERFILLDATA));
	} 
	if (!cpd) cpd=new EngraverFillData();//creates 1 count

	//cpd->renderdepth=-recurse;
	//cpd->xaxis(3*cpd->xaxis());
	//cpd->yaxis(3*cpd->yaxis());
	cpd->origin(flatpoint(xx,yy));
	cpd->xaxis(flatpoint(1,0)/Getmag()*100);
	cpd->yaxis(flatpoint(0,1)/Getmag()*100);
	cpd->FillRegularLines();
	cpd->FindBBox();
	return cpd;
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
	ipdata=dynamic_cast<EngraverFillData *>(data);
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
	if (mode==EMODE_Mesh) return PatchInterface::LBDown(x,y,state,count,d);

	if (mode==EMODE_Freehand) {
		return 0;
	}
	
	// else EMODE_Thickness
}
	
int EngraverFillInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (mode==EMODE_Mesh) return PatchInterface::LBUp(x,y,state,d);
}

int EngraverFillInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (mode==EMODE_Mesh) return PatchInterface::MouseMove(x,y,state,count,d);
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
	}

	return PatchInterface::PerformAction(action);
}

Laxkit::ShortcutHandler *EngraverFillInterface::GetShortcuts()
{  return PatchInterface::GetShortcuts(); }

int EngraverFillInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"in EngraverFillInterface::CharInput"<<endl;
	return PatchInterface::CharInput(ch,buffer,len,state,d);
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
	if (!data) { needtodraw=0; return 0; }


	LinePoint *l;
	LinePoint *last=NULL, *last2=NULL;
	for (int c=0; c<cpd->lines.n; c++) {
		l=cpd->lines.e[c];
		last=l;
		l=l->next;
		while (l) {
			if (!last2) {
				***
			}
			***

			last2=last;
			last=l;
			l=l->next;
		}
	}

	return PatchInterface::Refresh();
}


} // namespace LaxInterfaces

