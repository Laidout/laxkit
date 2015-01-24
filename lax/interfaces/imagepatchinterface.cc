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
//    Copyright (C) 2004-2011 by Tom Lechner
//

#include <Imlib2.h>

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




//------------------------------------- ImagePatchData ------------------------


/*! \class ImagePatchData
 * \ingroup interfaces
 * \brief Cubic tensor product patch with an image as the color source.
 *
 * See ImagePatchInterface.
 *
 * \todo *** should have option to offset the color data within the patch!!!
 * \todo should not depend on Imlib
 */
/*! \var char *ImagePatchData::filename
 * \brief The file (if any) that idata corresponds to.
 */
/*! \var unsigned long *ImagePatchData::idata
 * \brief The color data in ARGB format for sampling from.  
 */
/*! \var int ImagePatchData::idataislocal
 * \brief Whether idata should be delete[]'d in the destructor.
 */
/*! \var int ImagePatchData::iwidth
 * \brief The width of the image color data.
 */
/*! \var int ImagePatchData::iheight
 * \brief The width of the image color data.
 */
/*! \var double ImagePatchData::im[6]
 * \brief Offset of the image within color space.
 *
 * Normally, images will fill the entire patch, with (s,t)=(1,1) corresponding
 * to point (width,height) in the image. im is a transform to apply to the image
 * data before retrieving the color. 
 *
 * This allows easily offseting patched data without creating a whole new image.
 * Please note that this may make some areas of the image invisible, as they are
 * outside the range of the patch.
 *
 * \todo *** implement me!!! also implement auto expand patch to include areas off the edges
 */


//! Create a patch with the provided color data, set in unit rectangle x:[0,1] and y:[0,1].
ImagePatchData::ImagePatchData(int iwidth, int iheight, unsigned long *ndata,int disl,
							   double nscale,unsigned int stle)
	: PatchData(0,0,1,1,1,1,0)
{
	usepreview=1;

	filename=NULL;
	idata=ndata;
	idataislocal=disl;
	iwidth=iwidth;
	iheight=iheight;
	transform_identity(im);
}

//! Plain constructor. Calls SetImage(file). Sets in unit rectangle x:[0,1] and y:[0,1].
ImagePatchData::ImagePatchData(const char *file)//file=NULL
	: PatchData(0,0,1,1,1,1,0)
{
	usepreview=1;
	renderdepth=0;

	filename=NULL;
	idata=NULL;
	idataislocal=0;
	iwidth=iheight=0;
	transform_identity(im);

	SetImage(file);
}

/*! delete[]'s idata if idataislocal==1 || idataislocal==2.
 */
ImagePatchData::~ImagePatchData()
{
	if (filename) delete[] filename;
	if (idata && (idataislocal==1 || idataislocal==2)) delete[] idata; 
}

SomeData *ImagePatchData::duplicate(SomeData *dup)
{
	ImagePatchData *p=dynamic_cast<ImagePatchData*>(dup);
	if (!p && !dup) return NULL; //was not ImagePatchData!

	char set=1;
	if (!dup && somedatafactory) {
		dup=somedatafactory->newObject(LAX_IMAGEPATCHDATA,this);
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			set=0;
		}
		p=dynamic_cast<ImagePatchData*>(dup);
	} 
	if (!p) {
		p=new ImagePatchData();
		dup=p;
	}
	if (set) {
		p->SetImage(filename);
		transform_copy(p->im, im);
	}

	return PatchData::duplicate(dup);
}

//! Return the color at (s,t) where (1,1) corresponds to (iwidth,iheight) in the image.
/*! This returns an element from idata, which is ARGB.
 */
int ImagePatchData::WhatColor(double s,double t,ScreenColor *color_ret)
{
	if (!idata) return 1;
	//if (s<0. || s>1. || t<0. || t>1.) return 0;
	int x,y,i;
	x=(int)(s*iwidth);
	y=(int)(t*iheight);
	if (x<0) x=0; else if (x>=iwidth) x=iwidth-1;
	if (y<0) y=0; else if (y>=iheight) y=iheight-1;
	i=y*iwidth+x;
	color_ret->alpha=((idata[i]&0xff000000)>>16) | 0xff;
	color_ret->red  =((idata[i]&0xff0000)>>8) | 0xff;
	color_ret->green= (idata[i]&0xff00) | 0xff;
	color_ret->blue =((idata[i]&0xff)<<8) | 0xff;

	return 0;
}

//! Return the color at (s,t) where (1,1) corresponds to (iwidth,iheight) in the image.
/*! This returns an unsigned long element from idata, which is ARGB.
 */
unsigned long ImagePatchData::WhatColorLong(double s,double t)
{
	if (!idata) return 0;
	//if (s<0. || s>1. || t<0. || t>1.) return 0;
	int x,y,i;
	x=(int)(s*iwidth);
	y=(int)(t*iheight);
	if (x<0) x=0; else if (x>=iwidth) x=iwidth-1;
	if (y<0) y=0; else if (y>=iheight) y=iheight-1;
	i=y*iwidth+x;
	return idata[i];
}

/*! \ingroup interfaces
 * Dump out an ImagePatchData.
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
void ImagePatchData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	PatchData::dump_out(f,indent,what,context);
	
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 
	if (what==-1) {
		fprintf(f,"%sfilename whicheverfile.jpg  #name of the image used\n", spc);
		fprintf(f,"%siwidth  100  #width of the image in pixels for a preview sampling\n",spc);
		fprintf(f,"%siheight 100  #iheight of the image in pixels for a preview sampling\n",spc);
		return;
	}
	DumpContext *dump=dynamic_cast<DumpContext *>(context);
	if (dump && dump->basedir) {
		char *tmp=NULL;
		if (filename) {
			if (!dump->subs_only || (dump->subs_only && is_in_subdir(filename,dump->basedir)))
				tmp=relative_file(filename,dump->basedir,1);
			fprintf(f,"%sfilename \"%s\"\n",spc,tmp?tmp:filename);
			if (tmp) { delete[] tmp; tmp=NULL; }
		}
	} else fprintf(f,"%sfilename \"%s\"\n",spc,filename);
	fprintf(f,"%siwidth %d\n",spc,iwidth);
	fprintf(f,"%siheight %d\n",spc,iheight);
}

//! Reverse of dump_out.
void ImagePatchData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	int c;

	PatchData::dump_in_atts(att,flag,context);

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"filename")) {
			DumpContext *dump=dynamic_cast<DumpContext *>(context);
			if (value && *value!='/' && dump && dump->basedir) {
				if (filename) delete[] filename;
				filename=full_path_for_file(value,dump->basedir);
			} else makestr(filename,value);

		} else if (!strcmp(name,"iwidth")) {
			IntAttribute(value,&iwidth);

		} else if (!strcmp(name,"iheight")) {
			IntAttribute(value,&iheight);
		}
	}

	SetImage(filename);
	FindBBox();
}

//! Return 0 for success.
/*! *** on error, the old stuff is kept.
 *
 * This makes idata hold 8bit ARGB samples, not necessarily ready for display on the screen.
 * Note that this is the same as is returned in Imlib's
 * imlib_image_get_data_for_reading_only().
 */
int ImagePatchData::SetImage(const char *fname)
{
	if (!fname) return -1;
	Imlib_Image t=imlib_load_image(fname);
	if (t) {
		makestr(filename,fname);
		imlib_context_set_image(t);
		DATA32 *data32=imlib_image_get_data_for_reading_only(); //ARGB
		iwidth= imlib_image_get_width();
		iheight=imlib_image_get_height();
		if (idata && (idataislocal==1 || idataislocal==2)) delete[] idata; 
		idata=new unsigned long[iwidth*iheight];
		int r,c,i,i2;
		int flip=1;
		for (r=0; r<iheight; r++) 
			for (c=0; c<iwidth; c++) {
				i=r*iwidth+c;
				if (flip) i2=(iheight-r-1)*iwidth+c;
				else i2=i;
				idata[i]=data32[i2];

//				idata[i]=anXApp::app->rgbcolor(
//						(data32[i2]&0xff0000)>>16,
//						(data32[i2]&0xff00)>>8,
//						(data32[i2]&0xff)
//					);
			}
		imlib_free_image();
		idataislocal=1;
		touchContents();
		
		DBG cerr <<"ImagePatchData "<<object_id<<" Set to "<<filename<<endl;
		DBG dump_out(stderr,2,0,NULL);
		return 0;
	} else {
		cout <<"** warning ImageData couldn't load "<<(fname?fname:"(unknown)")<<" do something!"<<endl;
	}
	return 1;
}

//! Make patch be rectangular with 1:1 pixel aspect in image.
void ImagePatchData::zap()
{
	PatchData::zap();
	if (iheight>0) {
		double scale=(maxx-minx)/iwidth*iheight/(maxy-miny);
		//double scale=iheight/(maxy-miny);
		for (int c=0; c<xsize*ysize; c++) {
			points[c].y=miny+(points[c].y-miny)*scale;
		}
	}
	FindBBox();
}

//! Return whether idata has any info.
int ImagePatchData::hasColorData()
{ 
	return idata?1:0;
}


////------------------------------ ImagePatchInterface -------------------------------


/*! \class ImagePatchInterface
 * \ingroup interfaces
 * \brief Interface for dealing with ImagePatchData objects.
 *
 * \todo ***when inserting new image, should have option to preserve the scale, so if aspect of
 *    old image was 2, and new image is 1, then do the same for the aspect of the patch as a whole,
 *    or have the image space map to a subset of the patch space, rather than always [0..1]
 */


ImagePatchInterface::ImagePatchInterface(int nid,Displayer *ndp) : PatchInterface(nid,ndp)
{
	rendermode=1;
	recurse=0;
}

//! Empty destructor.
ImagePatchInterface::~ImagePatchInterface() 
{
	DBG cerr<<"-------"<<whattype()<<","<<" destructor"<<endl;
}

const char *ImagePatchInterface::Name()
{ return _("Image Patch Tool"); }

anInterface *ImagePatchInterface::duplicate(anInterface *dup)//dup=NULL;
{
	ImagePatchInterface *dupp;
	if (dup==NULL) dupp=new ImagePatchInterface(id,NULL);
	else dupp=dynamic_cast<ImagePatchInterface *>(dup);
	if (!dupp) return NULL;

	dupp->recurse=recurse;
	dupp->rendermode=rendermode;
	return PatchInterface::duplicate(dupp);
}

//! Return new local ImagePatchData
PatchData *ImagePatchInterface::newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	ImagePatchData *cpd=NULL;
	if (somedatafactory) {
		cpd=dynamic_cast<ImagePatchData *>(somedatafactory->newObject(LAX_IMAGEPATCHDATA));
	} 
	if (!cpd) cpd=new ImagePatchData();//creates 1 count

	//cpd->renderdepth=-recurse;
	//cpd->xaxis(3*cpd->xaxis());
	//cpd->yaxis(3*cpd->yaxis());
	cpd->origin(flatpoint(xx,yy));
	cpd->xaxis(flatpoint(1,0)/Getmag()*100);
	cpd->yaxis(flatpoint(0,1)/Getmag()*100);
	cpd->FindBBox();
	return cpd;
}

//! id==4 means make recurse=ndata.
int ImagePatchInterface::UseThis(int id,int ndata)
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

int ImagePatchInterface::UseThisObject(ObjectContext *oc)
{
	int c=PatchInterface::UseThisObject(oc);
	ipdata=dynamic_cast<ImagePatchData *>(data);
	return c;
}

/*! Accepts ImagePatchData, or ImageInfo.
 *
 * Returns 1 to used it, 0 didn't
 *
 * If nobj is an ImageInfo, then change the file, cacheimage, description of the current data..
 */
int ImagePatchInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask) // assumes not use local
{
    if (!nobj) return 0;

	if (dynamic_cast<ImagePatchData *>(nobj)) { 
		return PatchInterface::UseThis(nobj,mask);

	} else if (data && dynamic_cast<ImageInfo *>(nobj)) {
		ImageInfo *imageinfo=dynamic_cast<ImageInfo *>(nobj);
		ipdata=dynamic_cast<ImagePatchData *>(data);
		if (!ipdata) return 0;

		ipdata->SetImage(imageinfo->filename);
		return 1;
	}
	return 0;
}

/*! This should result in an image properties dialog of some kind being run.
 * This function is called in response to an image patch being double clicked on.
 *
 * It is assumed that data and curwindow both exist.
 *
 * \todo *** after laxkit event mechanism gets rewritten, the return event would get
 *   sent back to this ImageInterface instance, but right now, curwindow has to
 *   somehow relay the event to the object in question.
 */
void ImagePatchInterface::runImageDialog()
{
	 //after Laxkit event system is rewritten, this will be very different:
	ImagePatchData *p=dynamic_cast<ImagePatchData *>(data);
	if (!p) return;
	ImageInfo *inf=new ImageInfo(p->filename,NULL,NULL,NULL,0);
	curwindow->app->rundialog(new ImageDialog(NULL,NULL,"Image Properties",
					0,
					0,0,400,400,0,
					NULL,curwindow->object_id,"image properties",
					IMGD_NO_TITLE|IMGD_NO_DESCRIPTION,
					inf));
	inf->dec_count();
}

//! Catch a double click to pop up an ImageDialog.
int ImagePatchInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (count==2 && (style&IMGPATCHI_POPUP_INFO) && curwindow) {
		runImageDialog();
		//buttondown.down(d->id,LEFTBUTTON);
		return 0;
	}
	return PatchInterface::LBDown(x,y,state,count,d);
}
	
int ImagePatchInterface::PerformAction(int action)
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

Laxkit::ShortcutHandler *ImagePatchInterface::GetShortcuts()
{ return PatchInterface::GetShortcuts(); }

int ImagePatchInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"in ImagePatchInterface::CharInput"<<endl;
	return PatchInterface::CharInput(ch,buffer,len,state,d);
}

//! Checks for ImagePatchData, then calls PatchInterface::DrawData(ndata,a1,a2,info).
int ImagePatchInterface::DrawData(Laxkit::anObject *ndata,anObject *a1,anObject *a2,int info) // info=0
{
	if (!ndata || dynamic_cast<ImagePatchData *>(ndata)==NULL) return 1;
	return PatchInterface::DrawData(ndata,a1,a2,info);
}

//! Draws one patch to the screen. Called from PatchInterface::Refresh().
/*! The whole patch is made of potentially a whole lot of adjacent
 * patches. If rendermode==0, then just draw the wire outline. Otherwise
 * do the whole color thing.
 *
 * This function prepares Cx and Cy matrices for patchpoint(). A point in the
 * patch is px=St*Cx*T, py=St*Cy*T.
 *
 * roff,coff is which patch, point start is == off*3
 *
 * \todo *** when there is no image should do something clever to indicate there is none, like
 *   drawing a bezier X across the patch.
 */
void ImagePatchInterface::drawpatch(int roff,int coff)
{
	//DBG cerr <<"Draw Color Patch: roff:"<<roff<<"  coff:"<<coff<<"   mode:"<<rendermode<<endl;
	if (rendermode==0) { PatchInterface::drawpatch(roff,coff); return; }
	
	DBG cerr <<" - - - ImagePatchInterface::drawpatch()"<<endl;

	int r,c,i;
	flatpoint fp;
	double C[16],Gty[16],Gtx[16];
	double s0,ds,t0,dt;
	data->getGt(Gtx,roff*3,coff*3,0);
	data->getGt(Gty,roff*3,coff*3,1);
	DoubleBBox bbox;
	for (r=0; r<4; r++) {
		for (c=0; c<4; c++) {
			i=c*4+r;
			fp=flatpoint(Gtx[i],Gty[i]);
			fp=dp->realtoscreen(fp);
			Gtx[i]=fp.x;
			Gty[i]=fp.y;
		}
	}
	bbox.addtobounds(flatpoint(Gtx[0],Gty[0]));
	bez_bbox(flatpoint(Gtx[0],Gty[0]), flatpoint(Gtx[1],Gty[1]), flatpoint(Gtx[2],Gty[2]), flatpoint(Gtx[3],Gty[3]), &bbox, NULL);
	bez_bbox(flatpoint(Gtx[0],Gty[0]), flatpoint(Gtx[4],Gty[4]), flatpoint(Gtx[8],Gty[8]), flatpoint(Gtx[12],Gty[12]), &bbox, NULL);
	bez_bbox(flatpoint(Gtx[3],Gty[3]), flatpoint(Gtx[7],Gty[7]), flatpoint(Gtx[11],Gty[11]), flatpoint(Gtx[15],Gty[15]), &bbox, NULL);
	bez_bbox(flatpoint(Gtx[12],Gty[12]), flatpoint(Gtx[13],Gty[13]), flatpoint(Gtx[14],Gty[14]), flatpoint(Gtx[15],Gty[15]), &bbox, NULL);

	 //make sure is onscreen
	if (bbox.maxx<dp->Minx || bbox.minx>dp->Maxx ||
		bbox.maxy<dp->Miny || bbox.miny>dp->Maxy) return;
	s0=bbox.maxx-bbox.minx;
	t0=bbox.maxy-bbox.miny;
	if (t0>s0) s0=t0;
	r=(int)(s0/recurse);
	
	PatchRenderContext context;
	m_times_m(B,Gty,C);
	m_times_m(C,B, context.Cy);
	m_times_m(B,Gtx,C);
	m_times_m(C,B, context.Cx);  //Cx = B Gtx B

	s0=coff*3./(data->xsize-1);
	ds=3./(data->xsize-1);
	t0=roff*3./(data->ysize-1);
	dt=3./(data->ysize-1);
	DBG cerr <<" draw patch s:"<<s0<<','<<ds<<"  t:"<<t0<<','<<dt<<endl;
		
	patchpoint(&context, s0,ds,t0,dt,r);
}

//! Called from drawpatch(). No recursion, just draw a bunch of rects for patch.
/*! The parameters refer to the color space. They are not coordinates. 
 *
 * n is the number of areas to divide the patch into. s0, t0, ds, dt all refer
 * to the color space.
 *
 * Finds coords via getSPoint().
 *
 * \todo *** optimize this baby! If going beyond resolution of what is in the color source, should be able
 *   back up and draw larger rect...
 */
void ImagePatchInterface::patchpoint(PatchRenderContext *context,double s0,double ds,double t0,double dt,int n)
{
	int r,c;
	int i;
	flatpoint c1,c2,c3,c4;
	double T[4],S[4],s,t, // s and t vary [0..1] and are for patch 
		   ss,tt;        // ss and tt refer to the image space
	ImagePatchData *idata=dynamic_cast<ImagePatchData *>(data);
	
	if (ds*idata->iwidth<n) {
		n=int(ds*idata->iwidth);
		if (n==0) n=1;
	}
	if (dt*idata->iheight<n) {
		n=int(dt*idata->iheight);
		if (n==0) n=1;
	}
	ds/=n;
	dt/=n;
	
	getT(T,0);
	double d=1.0/n;
	flatpoint pp[(n+1)*2];

	//double dt2,dt3,ds2,ds3,t1,t2,t3;
	//dt2=dt*dt;
	//dt3=dt2*dt;
	//ds2=ds*ds;
	//ds3=ds2*ds;
	
	ScreenColor col[2][n+1];
	int a=1;
	for (c=0,ss=s0; c<=n; c++,ss+=ds) idata->WhatColor(ss,0, &col[0][c]);
		
	idata->WhatColor(0,0, &col[a][0]);
	int olddm=dp->DrawScreen();
	for (r=0,t=0,tt=t0; r<=n; r++,t+=d,tt+=dt) {
		getT(T,t);
		for (c=0,s=0,ss=s0; c<=n; c++,s+=d,ss+=ds) {
			idata->WhatColor(ss+ds,tt+dt, &col[a][c+1]);
			
			getT(S,s);
			if (r%2==0) 
				if (c%2==0) i=2*c;    // row even, column even
				else i=2*c+1;         // row even, column odd
			else if (c%2==0) i=2*c+1; // row odd,  column even
				else i=2*c;           // row odd,  column odd
			pp[i]=context->getPoint(S,T); // computes (S Cx T,S Cy T), is already in screen 
										  //   coords, thus so is returned point

			if (r>0 && c>0) {
				dp->NewFG(&col[a][c]);
				dp->drawlines(pp+(c-1)*2,4,1,1); //draw a filled quadrilateral
				//XFillPolygon(app->dpy,dp->GetWindow(),dp->GetGC(),pp+(c-1)*2,4,Convex,CoordModeOrigin);
			}
		}
		a^=1;
		idata->WhatColor(0,tt+dt, &col[a][0]);
	}
	if (olddm) dp->DrawReal();
}




} // namespace LaxInterfaces

