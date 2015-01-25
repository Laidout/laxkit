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
//    Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/laxutils.h>
#include <lax/colors.h>
#include <lax/language.h>
using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {

extern double B[16];
extern double Binv[16];

//------------------------------------- ColorPatchData ------------------------

/*! \class ColorPatchData
 * \ingroup interfaces
 * \brief Cubic tensor product patch with colors defined at each vertex.
 *
 * In any square segment, the color inside is the linear blend of the
 * colors of the 4 corners.
 *
 * See ColorPatchInterface.
 *
 * \todo *** this must be expanded to enable use of somewhat user
 *   definable functions for color determination, (see postscript spec for more).
 *   also should implement the coons patch as a special case of this.. the coons
 *   patch would simply interpolate the inner control points from the outer.
 * \todo *** the implementation here is rather a mess
 *
 */


//! Passes info to PatchData and creates new colors array with unspecified colors.
/*! Note that the PatchData constructor calls Set(), which does not call ColorPatchData::Set().
 */
ColorPatchData::ColorPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
	: PatchData(xx,yy,ww,hh,nr,nc,stle)
{
	usepreview=1;
	renderdepth=0;
	colors=new ScreenColor[(xsize/3+1)*(ysize/3+1)];

	DBG cerr <<"colorpatchdata create with "<<(xsize/3+1)*(ysize/3+1)<<" elements"<<endl;
}

//! Creates new colors array if (xsize && ysize) else colors=NULL.
ColorPatchData::ColorPatchData()
{
	usepreview=1;
	if (xsize && ysize) colors=new ScreenColor[(xsize/3+1)*(ysize/3+1)];
	else colors=NULL;

	DBG cerr <<"colorpatchdata create with "<<(xsize/3+1)*(ysize/3+1)<<" elements"<<endl;
}

ColorPatchData::~ColorPatchData()
{
	DBG cerr <<"COLORPATCH DESTRUCTOR KABOOM"<<endl;
	if (colors) delete[] colors;
}


SomeData *ColorPatchData::duplicate(SomeData *dup)
{
	ColorPatchData *p=dynamic_cast<ColorPatchData*>(dup);
	if (!p && !dup) return NULL; //was not ColorPatchData!

	char set=1;
	if (!dup && somedatafactory) {
		dup=somedatafactory->newObject(LAX_COLORPATCHDATA,this);
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			set=0;
		}
		p=dynamic_cast<ColorPatchData*>(dup);
	} 
	if (!p) {
		p=new ColorPatchData();
		dup=p;
	}
	if (set) {
		if (colors) {
			if (p->colors) delete[] p->colors;
			p->colors=new ScreenColor[(xsize/3+1)*(ysize/3+1)];
			for (int c=0; c<(xsize/3+1)*(ysize/3+1); c++) {
				p->colors[c]=colors[c];
			}
		}
	}

	return PatchData::duplicate(dup);
}

//! Set in rect xx,yy,ww,hh with nr rows and nc columns. Removes old info.
/*! Afterward the colors can be anything. Might want to code preserving what is possible
 * to preserve...
 */
void ColorPatchData::Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	PatchData::Set(xx,yy,ww,hh,nr,nc,stle);

	if (colors) delete[] colors;
	colors=new ScreenColor[(xsize/3+1)*(ysize/3+1)];
}

/*! \ingroup interfaces
 * Dump out a ColorColorPatchData 
 *
 * Something like:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  griddivisions 10
 *  xsize 4
 *  ysize 4
 *  points \
 *   1 2     0 65535 65535 65535
 *   3 4 65535     0 65535 65535
 *   5 6 65535 65535     0 65535
 * </pre>
 *
 * If what==-1, then output a pseudocode mockup of the format. Otherwise
 * output the format as above.
 * 
 * \todo *** should allow floating point representation of colors in range [0.0, 0.1]
 *   and specification that colors are 8bit, or other bit depth, like have field
 *   "colordepth 16|8|float"
 */
void ColorPatchData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 
	if (what==-1) {
		fprintf(f,"%smatrix 1 0 0 1 0 0 #the affine matrix affecting the patch\n",spc);
		fprintf(f,"%sgriddivisions 10   #number of grid lines to display when not rendering colors\n",spc);
		fprintf(f,"%sxsize 4            #number of points in the x direction\n",spc);
		fprintf(f,"%sysize 4            #number of points in the y direction\n",spc);
		fprintf(f,"%sstyle smooth       #when dragging controls do it so patch is still smooth\n",spc);
		fprintf(f,"%scontrols full      #can also be linear, coons, or border\n",spc);
		fprintf(f,"%sbase_path          #If mesh is defined along path, include this single Path object\n",spc);
		fprintf(f,"%s  ...\n",spc);
		fprintf(f,"%spoints \\           #all xsize*ysize points, a list by rows of: x y r g b a\n",spc);
		fprintf(f,"%s  1.0 1.0     0 65535 65535 65535  #patch corners, not control points\n",spc);
		fprintf(f,"%s  2.0 1.0                          #have colors assigned to them, with\n",spc);
		fprintf(f,"%s  1.0 2.0                          #values that range from 0 to 65535\n",spc);
		fprintf(f,"%s  2.0 2.0 65535     0 65535 65535\n",spc);
		fprintf(f,"%s  #etc... there are 16 points in the smallest patch\n",spc);

		return;
	}
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,m(0),m(1),m(2),m(3),m(4),m(5));
	fprintf(f,"%sgriddivisions %d\n",spc,griddivisions);
	fprintf(f,"%sxsize %d\n",spc,xsize);
	fprintf(f,"%sysize %d\n",spc,ysize);

	if (style&PATCH_SMOOTH) fprintf(f,"%sstyle smooth\n",spc);
	
	if (controls==Patch_Linear)           fprintf(f,"%scontrols linear\n",spc);
	else if (controls==Patch_Coons)       fprintf(f,"%scontrols coons\n",spc);
	else if (controls==Patch_Border_Only)  fprintf(f,"%scontrols border\n",spc); 
	else if (controls==Patch_Full_Bezier) fprintf(f,"%scontrols full\n",spc);

	if (base_path) {
		fprintf(f,"%sbase_path\n",spc);
		base_path->dump_out(f,indent+2,what,context);
	}

	fprintf(f,"%spoints \\ #%dx%d\n",spc, xsize,ysize);
	spc[indent]=spc[indent+1]=' '; spc[indent+2]='\0';
	int cr,cc,ci;
	for (int c=0; c<xsize*ysize; c++) {
		fprintf(f,"%s%.10g %.10g ",spc,
				points[c].x,points[c].y);
		cr=cc=-1;
		if (c/xsize%3==0) cr=c/xsize/3;
		if (c%xsize%3==0) cc=c%xsize/3;
		if (cr>=0 && cc>=0) {
			ci=cr*(xsize/3+1)+cc;
			//fprintf(f,"%hd %hd %hd %hd", colors[ci].red,colors[ci].green,colors[ci].blue,colors[ci].alpha);
			fprintf(f,"%u %u %u %u", colors[ci].red,colors[ci].green,colors[ci].blue,colors[ci].alpha);
		}
		if (c%xsize==0) fprintf(f," #row %d\n",c/xsize);
		else fprintf(f,"\n");
	}
}

//! Reverse of dump_out().
void ColorPatchData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	int p=-1,c;

	SomeData::dump_in_atts(att,flag,context);

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"griddivisions")) {
			IntAttribute(value,&griddivisions);

		} else if (!strcmp(name,"xsize")) {
			IntAttribute(value,&xsize);

		} else if (!strcmp(name,"ysize")) {
			IntAttribute(value,&ysize);

		} else if (!strcmp(name,"points")) {
			p=c;

		} else if (!strcmp(name,"controls")) {
			if (!strcmp(value,"full"))        controls=Patch_Full_Bezier;
			else if (!strcmp(value,"linear")) controls=Patch_Linear;
			else if (!strcmp(value,"coons"))  controls=Patch_Coons;
			else if (!strcmp(value,"border")) controls=Patch_Border_Only;

		} else if (!strcmp(name,"base_path")) {
			if (!base_path) base_path=new PathsData();
			else base_path->clear();
			base_path->dump_in_atts(att->attributes.e[c], flag, context);
		}
	}

	 // read in points after all atts initially parsed, so as to retrieve xsize and ysize.
	if (p>-1) {
		double d[6];
		name=value=att->attributes.e[p]->value;
		//int nnn=(xsize/3+1)*(ysize/3+1);

		if (colors) delete[] colors;
		colors=new ScreenColor[(xsize/3+1)*(ysize/3+1)];
		if (points) delete[] points;
		points=new flatpoint[xsize*ysize];
		
		int cc,cr,ci;
		char *nl;
		for (c=0; c<xsize*ysize; c++) {
			nl=value;
			while (nl && isspace(*nl)) nl++;
			nl=strchr(nl,'\n');
			if (nl) *nl='\0';
			p=DoubleListAttribute(value,d,6,&name);
			
			//DBG cerr <<"p="<<p<<"  d=";
			//DBG for (int ccc=0; ccc<p; ccc++) cerr <<d[ccc]<<' '; cerr <<endl;
			
			if (p<2) { //*** broken point list
				if (nl) *nl='\n';
				continue;
			}
			points[c].x=d[0];
			points[c].y=d[1];
			cr=cc=-1;
			if (c/xsize%3==0) cr=c/xsize/3;
			if (c%xsize%3==0) cc=c%xsize/3;
			if (cr>=0 && cc>=0) ci=cr*(xsize/3+1)+cc; else ci=-1;
			if (ci>=0) {
				//if (p>=3) colors[ci].red  =(unsigned short)d[2]; else colors[ci].red  =0;
				//if (p>=4) colors[ci].green=(unsigned short)d[3]; else colors[ci].green=0;
				//if (p>=5) colors[ci].blue =(unsigned short)d[4]; else colors[ci].blue =0;
				//if (p>=6) colors[ci].alpha=(unsigned short)d[4]; else colors[ci].alpha=65535;
				if (p>=3) colors[ci].red  =(int)d[2]; else colors[ci].red  =0;
				if (p>=4) colors[ci].green=(int)d[3]; else colors[ci].green=0;
				if (p>=5) colors[ci].blue =(int)d[4]; else colors[ci].blue =0;
				if (p>=6) colors[ci].alpha=(int)d[5]; else colors[ci].alpha=65535;
				//DBG if (ci>=0) fprintf(stderr,"colors %d: %d %d %d %d\n",ci,colors[ci].red,colors[ci].green,colors[ci].blue,255);
				//if (p>=6) colors[ci].alpha=(short)d[5]; else colors[ci].alpha=255;
			}
			value=name;
			if (nl) *nl='\n';
		}

	} else {
		//*** need to set up a default point list!! previous colors and points
		//might have old data in them
	}

	if (base_path) UpdateFromPath();


	FindBBox();
	touchContents();
}

//! Return whether there are any points in the patch yet.
int ColorPatchData::hasColorData()
{ 
	return xsize>0 || ysize>0;
}

//! Retrieve the color for point s,t in the patch, where s and t are in range [0..1].
/*! Returns 0 if there is a non-null patch, else 1.
 */
int ColorPatchData::WhatColor(double s,double t,ScreenColor *color_ret)
{
	if (xsize==0 || ysize==0) return 1;

	int c,r;
	double ss,tt;
	resolveToSubpatch(s,t, c,ss,r,tt);
	ScreenColor col1,col2;
	int cxsize=xsize/3+1;
	coloravg(&col1, &colors[r*cxsize+c], &colors[r*cxsize+c+1], ss);
	coloravg(&col2, &colors[(r+1)*cxsize+c], &colors[(r+1)*cxsize+c+1], ss);
	coloravg(color_ret,&col1, &col2, tt);
	return 0;
}

void ColorPatchData::SetColor(int pr,int pc, Laxkit::ScreenColor *col)
{
	SetColor(pr,pc, col->red,col->green,col->blue,col->alpha);
}

/*! pr,pc is row/column of vertex, 0=first vertex, 1=second vertex, ...
 */
void ColorPatchData::SetColor(int pr,int pc,int red,int green,int blue,int alpha)
{
	if (pr<0 || pr>ysize/3 || pc<0 || pc>xsize/3) return;
	 // assumes colors in bounds
	DBG static int num=0;
	DBG cerr <<"SetColor "<<pr<<','<<pc<<"  "<<red<<','<<green<<','<<blue<<"   at num:"<<(++num)<<endl;
	
	colors[pr*(xsize/3+1)+pc].red=red;
	colors[pr*(xsize/3+1)+pc].green=green;
	colors[pr*(xsize/3+1)+pc].blue=blue;
	colors[pr*(xsize/3+1)+pc].alpha=alpha;
	touchContents();
}

static void resolveToSubpatch2(int xsize,int ysize, double s,double t,
							   int &c,double &ss,int &r,double &tt)
{
	 //a remapping so we can grab column and row from old values of xsize,ysize
	 //xsize and ysize are number of patches, not points
	 
	int nc=xsize, //num of patches horizontally
		nr=ysize; //num of patches vertically

	c=(int) (s*nc);
	if (c>=nc) c=nc-1;
	ss=s*nc-c;

	r=(int) (t*nr);
	if (r>=nr) r=nr-1;
	tt=t*nr-r;
}

/*! Call PatchData::UpdateFromPath(), then update the color information according
 * to new mesh size.
 */
int ColorPatchData::UpdateFromPath()
{
	int oldxsize=xsize/3+1,oldysize=ysize/3+1;
	int status=PatchData::UpdateFromPath();
	if (status!=0) return status;

	TransferColors(oldxsize, oldysize);

	return 0;
}

/*! Call PatchData::CopyMeshPoints(patch) and update color array to be proper size.
 */
void ColorPatchData::CopyMeshPoints(PatchData *patch, bool usepath)
{
	int oldxsize=xsize/3+1,oldysize=ysize/3+1;
	PatchData::CopyMeshPoints(patch, usepath); 
	TransferColors(oldxsize, oldysize);
}

/*! Low level function to sample current colors, and potentially reallocate
 * the colors array.
 *
 * Assumes xsize and ysize do NOT correspond to the current points array.
 * This function is used to sync up the colors array (assumed to be old) to
 * match the current points array.
 *
 * oldxsize and oldysize are the number of mesh squares, NOT the number of points,
 * i.e. they do NOT have the same metric as this->xsize and this->ysize.
 */
int ColorPatchData::TransferColors(int oldxsize, int oldysize)
{
	int nxs,nys;
	nxs=xsize/3+1;
	nys=ysize/3+1;

	if (nxs!=oldxsize || nys!=oldysize) {
		int colorsize=nxs*nys;
		ScreenColor *ncolors=new ScreenColor[colorsize];
		ScreenColor *oldcolors=colors;
		colors=ncolors;
	
		double s,t;
		ScreenColor color;
		ScreenColor col1,col2;
		int c,r;
		double ss,tt;
		int cxsize;

		for (int x=0; x<nxs; x++) {
			for (int y=0; y<nxs; y++) { 
				resolveFromSubpatch(x,0, y,0, s,t);

				resolveToSubpatch2(oldxsize-1,oldysize-1, s,t, c,ss,r,tt);

				cxsize=oldxsize;
				coloravg(&col1, &oldcolors[r*cxsize+c],     &oldcolors[r*cxsize+c+1],     ss);
				coloravg(&col2, &oldcolors[(r+1)*cxsize+c], &oldcolors[(r+1)*cxsize+c+1], ss);
				coloravg(&color,&col1, &col2, tt);
				
				SetColor(y,x, &color);
			}
		}

		delete[] oldcolors;
	}

	return 0;
}

//! Grow the patch off an edge.
/*! If where==0, add a column to the left.
 *  If where==1, add a row to the top.
 *  If where==2, add a column to the right.
 *  If where==3, add a row to the bottom.
 *
 * The new edge is the oldedge+v, and the intervening controls are interpolated.
 * The new colors are duplicates of the old edge's colors.
 *
 * This uses PatchData::grow(), then adjusts the color array accordingly.
 */
void ColorPatchData::grow(int where, double *tr)
{
	int oldxsize=xsize/3+1,oldysize=ysize/3+1;
	PatchData::grow(where,tr);
	
	int nxs,nys;
	nxs=xsize/3+1;
	nys=ysize/3+1;
	int colorsize=nxs*nys;
	ScreenColor *ncolors=new ScreenColor[colorsize];
		
	if (where==0) { 
		 //add to the left
		for (int r=0; r<nys; r++) {
			memcpy(ncolors+r*nxs+1, colors+r*oldxsize,oldxsize*sizeof(ScreenColor));
			ncolors[r*nxs]=colors[r*oldxsize];
		}
	} else if (where==1) { 
		 //add to the top
		memcpy(ncolors,colors,oldxsize*oldysize*sizeof(ScreenColor));
		memcpy(ncolors+oldxsize*oldysize,colors+oldxsize*(oldysize-1),oldxsize*sizeof(ScreenColor));
	} else if (where==2) { 
		 //add to the right
		for (int r=0; r<nys; r++) {
			memcpy(ncolors+r*nxs, colors+r*oldxsize,oldxsize*sizeof(ScreenColor));
			ncolors[(r+1)*nxs-1]=colors[(r+1)*oldxsize-1];
		}
	} else {
		 //add to the bottom
		memcpy(ncolors+oldxsize,colors,oldxsize*oldysize*sizeof(ScreenColor));
		memcpy(ncolors,colors,oldxsize*sizeof(ScreenColor));
	}
	
	delete[] colors;
	colors=ncolors;
}

//! Merge rows, and/or columns.
/*! r and c are subpatch indices, not point indices. So to merge the first column
 * of patches and the next column of patches, then pass c=1. If c<1 then no columns
 * are merged. Similarly for rows. 
 *
 * If r>0 and c>0, then columns are merged first, then rows.
 *
 * This uses PatchData::collapse(), then adjusts the color array accordingly.
 */
void ColorPatchData::collapse(int rr,int cc) 
{
	int oldxsize=xsize/3+1,oldysize=ysize/3+1;
	PatchData::collapse(rr,cc);
	
	int r,c;
	if (cc>=0 && oldxsize>1 && cc<oldxsize) {
		int nxs,nys;
		nxs=xsize/3+1;
		nys=ysize/3+1;
		int colorsize=nxs*nys;
		ScreenColor *ncolors=new ScreenColor[colorsize];
		
		for (r=0; r<oldysize; r++) {
			for (c=0; c<cc; c++) {
				ncolors[c+r*nxs]=colors[c+r*oldxsize];
			}
			for (c=cc+1; c<oldxsize; c++) {
				ncolors[c-1+r*nxs]=colors[c+r*oldxsize];
			}
		}
		delete[] colors;
		colors=ncolors;
	}
	
	if (rr>=0 && oldysize>1 && rr<oldysize) {
		int nxs,nys;
		nxs=xsize/3+1;
		nys=ysize/3+1;
		int colorsize=nxs*nys;
		ScreenColor *ncolors=new ScreenColor[colorsize];
		
		for (c=0; c<oldxsize; c++) {
			for (r=0; r<rr; r++) {
				ncolors[c+r*nxs]=colors[c+r*oldxsize];
			}
			for (r=rr+1; r<oldysize; r++) {
				ncolors[c+(r-1)*nxs]=colors[c+r*oldxsize];
			}
		}
		delete[] colors;
		colors=ncolors;
	}

}

//! Subdivide a single row number r, position rt, and a single column c, position ct.
/*! If r<0, then do not subdivide on a row. Same for when c<0. if r or c are
 * to large, then do not subdivide.
 *
 * r and c are border numbers, so r*3 is index int points. rt and ct are in
 * the range [0..1], with rt==0 being row r, and rt==1 being row r+1.
 *
 * Return 0 for success, nonzero for failure (nothing changed).
 *
 * This uses PatchData::subdivide(), then adjusts the color array accordingly.
 *
 * \todo *** clean me up! simplify me! there is probably an efficient way to extract bits of
 *   this and the other subdivide() to shorten the code...
 */
int ColorPatchData::subdivide(int r,double rt,int c,double ct)
{
	int oldxsize=xsize/3+1, oldysize=ysize/3+1;
	if (PatchData::subdivide(r,rt,c,ct)) return 1; //nothing changed

	int newxsize=xsize/3+1, newysize=ysize/3+1;
	int colorsize=newxsize*newysize;
	ScreenColor *ncolors=new ScreenColor[colorsize];
	
	int rr,cc, ro=0,co=0;
	for (rr=0; rr<oldysize; rr++) {
		co=0;
		for (cc=0; cc<oldxsize; cc++) {
			if (r>=0 && rr==r+1) ro=1;
			if (c>=0 && cc==c+1) co=1;
			ncolors[(rr+ro)*newxsize+(cc+co)]=colors[rr*oldxsize+cc];
			DBG cerr <<"new:   "<<(rr+ro)*newxsize+(cc+co)<<"  from old:"<<rr*oldxsize+cc<<endl;
			if (r>=0 && rr==r) {
				coloravg(&ncolors[(rr+1)*newxsize+(cc+co)], &colors[rr*oldxsize+cc], &colors[(rr+1)*oldxsize+cc],rt);
				DBG cerr <<"rnew:  "<<(rr+1)*newxsize+(cc+co)<<"  from old avg:"<<rr*oldxsize+cc<<" + "<<(rr+1)*oldxsize+cc<<endl;
			}
			if (c>=0 && cc==c) {
				coloravg(&ncolors[(rr+ro)*newxsize+(cc+1)], &colors[rr*oldxsize+cc], &colors[(rr)*oldxsize+cc+1],ct);
				DBG cerr <<"cnew: "<<(rr+ro)*newxsize+(cc+1)<<"  from old:"<<rr*oldxsize+cc<<" + "<<(rr)*oldxsize+cc+1<<endl;
			}
			if (r>=0 && rr==r && c>=0 && cc==c) {
				ScreenColor col1,col2;
				coloravg(&col1, &colors[rr*oldxsize+cc  ], &colors[(rr+1)*oldxsize+cc  ],rt);
				coloravg(&col2, &colors[rr*oldxsize+cc+1], &colors[(rr+1)*oldxsize+cc+1],rt);
				coloravg(&ncolors[(rr+1)*newxsize+(cc+1)],&col1,&col2,ct);
				DBG cerr <<"rcnew:"<<(rr+1)*newxsize+(cc+1)<<"  from old:"<<rr*oldxsize+cc<<
				DBG												" + "<<(rr+1)*oldxsize+cc <<
				DBG 											" , "<<rr*oldxsize+cc+1 <<
				DBG 											" + "<<(rr+1)*oldxsize+cc+1<<endl;
			}
		}
	}
	delete[] colors;
	colors=ncolors;
	
	return 0;
}

//! Calls PatchData::subdivide(), then does special subdividing of colors array.
/*! This should preserve the appearance from before the subdivision. The colors
 * for the new vertex points will have the proper colors as they were in the original patch.
 *
 * Return 0 for success, nonzero for failure (nothing changed).
 */
int ColorPatchData::subdivide(int xn,int yn) //xn,yn=2
{
	int oldxsize=xsize,oldysize=ysize;
	if (PatchData::subdivide(xn,yn)) return 1;
	
	//DBG cerr <<"begin ColorPatchData::subdivide"<<endl;
	
	 // special resize colors
	int colorsize=(xsize/3+1)*(ysize/3+1);
	ScreenColor *ncolors=new ScreenColor[colorsize];
	int i, r,c,c2,r2,cs=(oldxsize/3+1),rs=(oldysize/3+1);
	
	ScreenColor col, colt, cols, *col1, *col2, *col3, *col4;
	double s,t;

	//DBG cerr <<"new color array size: "<<colorsize<<endl;
	//DBG cerr <<" cs:"<<cs<<" rs:"<<rs<<" xn:"<<xn<<" yn:"<<yn<<endl;
	
	for (r=0; r<rs-1; r++) {
		for (c=0; c<cs-1; c++) {
			col1=colors + r*cs     +   c  ;
			col2=colors + r*cs     + (c+1);
			col3=colors + (r+1)*cs +   c  ;
			col4=colors + (r+1)*cs + (c+1);
			
			//DBG cerr <<"r: "<<r<<"  c:"<<c;
			//DBG cerr <<endl<<" col1:"<<r*cs+c<<" col2:"<<r*cs+(c+1)<<" col3:"<<(r+1)*cs+c<<" col4:"<<(r+1)*cs+(c+1)<<endl;
			for (r2=0; r2<=yn; r2++) {
				for (c2=0; c2<=xn; c2++) {
					s=((double)c2)/xn;
					t=((double)r2)/yn;
					coloravg(&col,coloravg(&cols,col1,col2,s),coloravg(&colt,col3,col4,s),t);
					i=((r*yn+r2)*((cs-1)*xn+1)) + (c*xn+c2);
					//DBG fprintf(stderr,"r2:%d c2:%d  s:%5.2f  t:%5.2f  i:%d\n",r2,c2,s,t,i);
					ncolors[i]=col;
				}
			}
		}
	}
	//DBG cerr <<endl;
	
	delete[] colors;
	colors=ncolors;

	//DBG cerr <<"-- end color patch subdivide"<<endl;
		
	return 0;
}

////------------------------------ ColorPatchInterface -------------------------------


/*! \class ColorPatchInterface
 * \ingroup interfaces
 * \brief Interface for dealing with ColorPatchData objects.
 */


#define RENDER_NONE  0
#define RENDER_GRID  1
#define RENDER_COLOR 2
#define RENDER_MAX   2


ColorPatchInterface::ColorPatchInterface(int nid,Displayer *ndp) : PatchInterface(nid,ndp)
{
	cdata=NULL;
	rendermode=1;
	recurse=5;
	//recurse=0;
}

//! Empty destructor.
ColorPatchInterface::~ColorPatchInterface() 
{ 
	DBG cerr<<"-------"<<whattype()<<","<<" destructor"<<endl;
}

const char *ColorPatchInterface::Name()
{ return _("Color Patch Tool"); }

anInterface *ColorPatchInterface::duplicate(anInterface *dup)//dup=NULL;
{
	if (dup==NULL) dup=new ColorPatchInterface(id,NULL);
	else if (!dynamic_cast<ColorPatchInterface *>(dup)) return NULL;
	ColorPatchInterface *dupp=dynamic_cast<ColorPatchInterface *>(dup);
	dupp->recurse=recurse;
	dupp->rendermode=rendermode;
	return PatchInterface::duplicate(dup);
}

//! Return new local ColorPatchData
PatchData *ColorPatchInterface::newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	ColorPatchData *cpd=NULL;
	if (somedatafactory) {
		cpd=dynamic_cast<ColorPatchData *>(somedatafactory->newObject(LAX_COLORPATCHDATA));
		cpd->Set(xx,yy,ww,hh,nr,nc,stle);
	} 
	if (!cpd) cpd=new ColorPatchData(xx,yy,ww,hh,nr,nc,stle);//creates 1 count

	cpd->renderdepth=-recurse;
	 //void SetColor(int pr,int pc,int red,int green,int blue,alpha);
	cpd->SetColor(0,0, 0xffff,      0,      0, 0xffff);
	cpd->SetColor(0,1,      0, 0xffff,      0, 0xffff);
	cpd->SetColor(1,0,      0,      0, 0xffff, 0xffff);
	cpd->SetColor(1,1, 0xffff, 0xffff,      0, 0xffff);
	cpd->FindBBox();
	return cpd;
}

int ColorPatchInterface::UseThis(int id,int ndata)
{
	if (id!=4) return PatchInterface::UseThis(id,ndata);
	char blah[100];
	if (id==4) { // recurse depth
		if (ndata>0) {
			recurse=ndata;
			sprintf(blah,_("New Recurse Depth %d: "),recurse);
			app->postmessage(blah);
			//if (rendermode!=RENDER_NONE) needtodraw=1; 
			needtodraw=1; 
		}
		return 1;
	}
	return 0;
}

int ColorPatchInterface::SelectPoint(int c,unsigned int state)
{
	int n=PatchInterface::SelectPoint(c,state);
	if (n==0) return 0;

	if (c%data->xsize%3!=0 || c/data->xsize%3!=0) return n;
	
	int pr=c/(data->xsize)/3,
		pc=c%data->xsize/3;

	cdata=dynamic_cast<ColorPatchData *>(data);
	sendcolor(&cdata->colors[pr*(data->xsize/3+1)+pc]);
	return n;
}

//! Send a "make curcolor" event to the viewport.
/*! \todo bit of a hack here.
 */
int ColorPatchInterface::sendcolor(ScreenColor *col)
{
	if (!col || !curwindow || !curwindow->win_parent) return 0;
	SimpleColorEventData *e=new SimpleColorEventData(65535, col->red,col->green,col->blue,col->alpha, 0);
	app->SendMessage(e,curwindow->win_parent->object_id,"make curcolor",object_id);
	return 1;
}

int ColorPatchInterface::UseThisObject(ObjectContext *oc)
{
	int c=PatchInterface::UseThisObject(oc);
	cdata=dynamic_cast<ColorPatchData *>(data);
	return c;
}

/*! Accepts ColorPatchData or LineStyle objects.
 * Does not take possession of the LineStyle, but does take the ColorPatchData.
 *
 * *** warning, uses LineStyle::color, rather than the red/green/blue
 *
 * Returns 1 to used it, 0 didn't
 */
int ColorPatchInterface::UseThis(anObject *nobj,unsigned int mask) // assumes not use local
{
    if (!nobj) return 0;
	if (dynamic_cast<ColorPatchData *>(nobj)) { 
		return PatchInterface::UseThis(nobj,mask);
	} else if (dynamic_cast<LineStyle *>(nobj)) {
		//DBG cerr <<"ColorPatchInterface: new linestyle"<<endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);
		if (mask&GCForeground) if (data) {
			for (int c=0; c<curpoints.n; c++) {
				 // make all selected vertices have this color
				if (curpoints.e[c]%data->xsize%3!=0 || curpoints.e[c]/data->xsize%3!=0) continue;
				cdata=dynamic_cast<ColorPatchData *>(data);
				cdata->SetColor(curpoints.e[c]/(data->xsize)/3,curpoints.e[c]%data->xsize/3,
						nlinestyle->color.red,
						nlinestyle->color.green,
						nlinestyle->color.blue,
						nlinestyle->color.alpha
					);
			}
			data->touchContents();
		}
		//if (mask&GCLineWidth) if (data) data->linestyle.width=nlinestyle->width; else linestyle.width=nlinestyle->width;
		return 1;
	}
	return 0;
}

int ColorPatchInterface::PerformAction(int action)
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

Laxkit::ShortcutHandler *ColorPatchInterface::GetShortcuts()
{ return PatchInterface::GetShortcuts(); }

int ColorPatchInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	//DBG cerr <<"in ColorPatchInterface::CharInput"<<endl;
	return PatchInterface::CharInput(ch,buffer,len,state,d);
}

//! Draw a single point as a little filled colored circle.
/*! This is for the hoverpoint, and is called from PatchInterface::Refresh().
 */
void ColorPatchInterface::drawControlPoint(int i)
{
	if (!data || i<0 || i>=data->xsize*data->ysize) return;

	flatpoint p;
	p=dp->realtoscreen(data->points[i]);
	
	cdata=dynamic_cast<ColorPatchData *>(data);
	int r=(i/data->xsize),c=i%data->xsize;
	unsigned long color;
	if (r%3==0 && i%3==0) {
		color=pixelfromcolor(cdata->colors+(r/3*(data->xsize/3+1)+c/3));
	} else color=controlcolor;

	dp->DrawScreen();
	dp->NewFG(color);
	dp->drawpoint((int)p.x,(int)p.y,5,1);
	dp->NewFG(~0);
	dp->drawpoint((int)p.x,(int)p.y,5,0);
	dp->NewFG(0,0,0);
	dp->drawpoint((int)p.x,(int)p.y,6,0);
	dp->DrawReal();
}

//*! Refuses to draw anything but ColorPatchData objects.
int ColorPatchInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info) // info=0
{
	if (!ndata || dynamic_cast<ColorPatchData *>(ndata)==NULL) return 1;
	return PatchInterface::DrawData(ndata,a1,a2,info);
}

//! Draws one patch.
/*! called by drawpatches(). 
 * The whole patch is made of potentially a whole lot of adjacent
 * patches. If rendermode==0, then just draw the wire outline. Otherwise
 * do the whole color thing.
 *
 * This function prepares up col1,col2,col3,col4 and Cx,Cy matrices for patchpoint2().
 *
 * roff,coff is which patch, point start is == xoff*3
 */
void ColorPatchInterface::drawpatch(int roff,int coff)
{
	//DBG cerr <<"Draw Color Patch: roff:"<<roff<<"  coff:"<<coff<<"   mode:"<<rendermode<<endl;
	if (rendermode==0) { PatchInterface::drawpatch(roff,coff); return; }

	DBG cerr <<" - - - ColorPatchInterface::drawpatch()"<<endl;

	int r,c;
	flatpoint fp;
	double C[16],Gty[16],Gtx[16];
	data->getGt(Gtx,roff*3,coff*3,0);
	data->getGt(Gty,roff*3,coff*3,1);
	for (r=0; r<4; r++) {
		for (c=0; c<4; c++) {
			fp=flatpoint(Gtx[c*4+r],Gty[c*4+r]);
			fp=dp->realtoscreen(fp);
			Gtx[c*4+r]=fp.x;
			Gty[c*4+r]=fp.y;
		}
	}

	cdata=dynamic_cast<ColorPatchData *>(data);
	
	col1=cdata->colors + roff*(data->xsize/3+1)+coff;
	col2=cdata->colors + roff*(data->xsize/3+1)+coff+1;
	col3=cdata->colors + (roff+1)*(data->xsize/3+1)+coff;
	col4=cdata->colors + (roff+1)*(data->xsize/3+1)+coff+1;
	
	//DBG cerr <<"col1,2,3,4:"<<col1<<','<<col2<<','<<col3<<','<<col4<<endl;
	
	PatchRenderContext context;
	m_times_m(B,Gty,C);
	m_times_m(C,B, context.Cy);
	m_times_m(B,Gtx,C);
	m_times_m(C,B, context.Cx);  //Cx = B Gtx B
	
	patchpoint2(&context);//approximates with quadrilaterals
//	patchpoint(&context,0,0,1,1); //draw all points
}

//! No recursion, just draw n s,t rects for patch.
/*! Assumes col1,col2,col3,col4 are already set to the approprate colors
 * for the current segment.
 * 
 * \todo optimize this a la imagepatchinterface....
 */
void ColorPatchInterface::patchpoint2(PatchRenderContext *context)
{
	int r,c;
	int n,i;
	n=1<<recurse;
	flatpoint c1,c2,c3,c4;
	double T[4],S[4],s,t;
	getT(T,0);
	double d=1.0/n;
	flatpoint pp[(n+1)*2];

	int olddm=dp->DrawScreen();
	for (r=0,t=0; r<=n; r++,t+=d) {
		getT(T,t);
		for (c=0,s=0; c<=n; c++,s+=d) {
			getT(S,s);
			if (r%2==0) 
				if (c%2==0) i=2*c;    // row even, column even
				else i=2*c+1;         // row even, column odd
			else if (c%2==0) i=2*c+1; // row odd,  column even
				else i=2*c;           // row odd,  column odd
			pp[i]=context->getPoint(S,T); // computes (S Cx T,S Cy T), is already in screen coords, and is XPoint
			
			if (r>0 && c>0) {
				dp->NewFG(pixelfromcolor(coloravg(&col,coloravg(&cola,col1,col2,s),coloravg(&colb,col3,col4,s),t)));
				dp->drawlines(pp+(c-1)*2,4,1,1); //draw a filled quadrilateral
				//XFillPolygon(app->dpy,dp->GetWindow(),app->gc(),pp+(c-1)*2,4,Convex,CoordModeOrigin);
			}
		}
		
	}
	if (olddm) dp->DrawReal();
}

/*! \todo **** mondo problems with this one, keeps recursing forever
 */
void ColorPatchInterface::patchpoint(PatchRenderContext *context,double s1,double t1, double s2,double t2) // p1,p2 are s1,t1, s2,t2
{
	static int d=0;
	d++;
	DBG if (d>10) cerr <<"patchpoint "<<d<<endl;
	
	flatpoint c1,c2,c3,c4;
	double T[4],S[4];
	getT(T,t1);
	getT(S,s1);
	
	c1=context->getPoint(S,T); // computes (S Cx T,S Cy T), is already in screen coords
	getT(T,t1);
	getT(S,s2);
	c2=context->getPoint(S,T);
	getT(T,t2);
	getT(S,s1);
	c3=context->getPoint(S,T);
	getT(T,t2);
	getT(S,s2);
	c4=context->getPoint(S,T);
	//DBG cerr 
	//DBG 	<<(int)c1.x<<','<<c1.y<<"  "<<
	//DBG 	(int)c2.x<<','<<c1.y<<	"  "<<
	//DBG 	(int)c3.x<<','<<c1.y<<	"  "<<
	//DBG 	(int)c4.x<<','<<c1.y<<endl;	

	unsigned long color;
	if ( d>8 ||
			 ((int)c1.x==(int)c2.x && (int)c2.x==(int)c3.x && (int)c3.x==(int)c4.x &&
		(int)c1.y==(int)c2.y && (int)c2.y==(int)c3.y && (int)c3.y==(int)c4.y)) {
		color=pixelfromcolor(coloravg(&col,coloravg(&cola,col1,col2,s1),coloravg(&colb,col3,col4,s1),t1));
		dp->NewFG(color);
		dp->drawpoint(c1,1,1);//***
		//DBG cerr <<"---end color="<<col<<endl;
	} else {
		// s1,t1         (s1+s2)/2,t1          s2,t1
		// s1,(t1+t2)/2  (s1+s2)/2,(t1+t2)/2   s2,(t1+t2)/2
		// s1,t2         (s1+s2)/2,t2          s2,t2
//		if (abs((int)c1.x-(int)c4.x)>1 && abs((int)c1.y-(int)c4.y)>1) {
			patchpoint(context, s1,t1, (s1+s2)/2,(t1+t2)/2);
			patchpoint(context, (s1+s2)/2,t1, s2,(t1+t2)/2);
			patchpoint(context, s1,(t1+t2)/2, (s1+s2)/2,t2);
			patchpoint(context, (s1+s2)/2,(t1+t2)/2, s2,t2);
//		}
//		----------------
//		patchpoint(s1,t1, (s1+s2)/2,t1, s1,(t1+t2)/2, (s1+s2)/2,(t1+t2)/2);
//		patchpoint((s1+s2)/2,t1, s2,t2, (s1+s2)/2,(t1+t2)/2, (s1+s2)/2,t2);
//		patchpoint(s1,(t1+t2)/2, (s1+s2)/2,(t1+t2)/2, s1,t2, (s1+s2)/2,t2);
//		patchpoint((s1+s2)/2,(t1+t2)/2, s2,(t1+t2)/2, (s1+s2)/2,t2, s2,t2);
	}
	d--;
}






} // namespace Laxkit

