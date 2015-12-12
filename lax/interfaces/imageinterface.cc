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
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/viewportwindow.h>
#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/imagedialog.h>
#include <lax/fileutils.h>
#include <lax/laxutils.h>
#include <lax/language.h>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

#define sgn(a) ((a)<0?-1:((a)>0?1:0))

namespace LaxInterfaces {


//--------------------------------- ImageData -------------------------------
/*! \class ImageData
 * \brief Holds an image and a transform.
 *
 * The image is held from (0,0) to (image_width,image_height) such that, by convention,
 * minx=miny=0, maxx=image_width, and maxy=image_height. However, please note that
 * in class ImageInterface, when an image's width and height need to be found, maxx-minx and
 * maxy-miny are used instead.
 *
 * Please note that these are not really designed with the idea of editting the contained image,
 * but are more just a container to move around and scale images. 
 *
 * Be forewarned that if previewflag==1, then the preview image will be unlinked (deleted from
 * the harddrive) in the LaxImage destructor. The default for ImageData objects is to not
 * delete (previewflag==0).
 * 
 * \todo In addition to have filename ande previewimage, perhaps make allowances for
 *   a transformedImageCache to facilitate use of ImagePatchData, for instance.....
 *   this is relevant because the ImageInterface has lots of drawing code, and it is easier
 *   to use a temp ImageData than an ImagePatchInterface, for instance, figuring out how to draw it itself?
 * \todo should probably have an option for deferred loading? (not needed if using imlib, which basically
 *   does that automatically)
 * \todo have flag: Don't display image|Use original|Use preview?
 */
/*! \var char ImageData::previewflag
 * 
 * If previewflag&1, then the preview file path is saved in a dump_out, and is used
 * when loading from a dump_in.  On a dump_in, if a preview file is given, then the
 * 1 bit of previewflag gets set to 1.
 */

	
//! Constructor. Configures based on the image in nfilename.
/*! If nfilename is not a valid image, then image is NULL, and maxx==maxy==0.
 *
 * If npreview is passed in, then potentially generate a preview for the image. These previews will
 * not exceed dimensions maxpx by maxpy. Note that If there is already an image at npreview, and it exceeds these
 * bounds, then it is not forced down into the lower bounds. A new preview is generated only when
 * the file doesn't exist.
 *
 * If delpreview!=0, then for previews that are newly created, they should be deleted when
 * the associated LaxImage is destroyed. See LaxImage for more about that convention.
 *
 * If npreview, load_image_with_preview() is used, otherwise load_image() is used.
 */
ImageData::ImageData(const char *nfilename, const char *npreview, int maxpx, int maxpy, char delpreview)
	: ImageInfo(nfilename,npreview,NULL,NULL,0)
{
	DBG cerr <<"in ImageData constructor"<<endl;
	previewflag=(delpreview?0:1);
	image=NULL;
	previewimage=NULL;
	flags|=SOMEDATA_KEEP_1_TO_1;
	
	if (!filename) return;
	LoadImage(nfilename, npreview, maxpx, maxpy, delpreview,0);
}

ImageData::~ImageData()
{
	DBG cerr <<"in ImageData destructor"<<endl;

	if (image) { image->dec_count(); image=NULL; }
	if (previewimage) { previewimage->dec_count(); previewimage=NULL; }

	DBG cerr <<"-- ImageData dest. end"<<endl;
}
	
//! Return a new ImageData, based on this.
SomeData *ImageData::duplicate(SomeData *dup)
{
	ImageData *i=dynamic_cast<ImageData*>(dup);
	if (!i && dup) return NULL; //was not an ImageData!

	char set=1;
	if (!dup) {
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_IMAGEDATA));
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
		}
		i=dynamic_cast<ImageData*>(dup);
	} 
	if (!i) {
		i=new ImageData();
		dup=i;
	}
	if (set && i) {
		if (image) i->LoadImage(image->filename, previewimage ? previewimage->filename : NULL, 0,0,0,0);
	}

	 //somedata elements:
	dup->bboxstyle=bboxstyle;
	dup->m(m());
	return dup;
}

ImageData &ImageData::operator=(ImageData &i)
{
	SetInfo(&i);
	if (image) { image->dec_count(); image=NULL; }
	minx=i.minx;
	maxx=i.maxx;
	miny=i.miny;
	maxy=i.maxy;
	if (i.image) LoadImage(i.image->filename, previewimage ? previewimage->filename : NULL,0,0,0,1);
	return i;
}

/*! \ingroup interfaces
 * Default dump for an ImageData. The width and height are saved, but beware that the actual
 * width and height of the thing at filename may have different dimensions. This helps compensate
 * for broken images when source files are moved around: you still save the working dimensions.
 * On a dump in, if the image exists, then the dimensions at time of load are used,
 * rather than what was saved in the file.
 *
 * Note that ImageData::image is not consulted for any values.
 * 
 * Dumps:
 * <pre>
 *  width 34
 *  height 234
 *  filename filename.jpg
 *  previewfile .filename-preview.jpg
 *  description "Blah blah blah"
 * </pre>
 * width and height are the integer pixel dimensions of the image.\n
 * If previewfile is not an absolute path, then it is relative to filename.
 *
 * If what==-1, then dump out a psuedocode mockup of what gets dumped. This makes it very easy
 * for programs to keep track of their file formats, that is, when the programmers remember to
 * update this code as change happens.
 * Otherwise dumps out in indented data format as above.
 */
void ImageData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%sfilename /path/to/file\n",spc);
		fprintf(f,"%spreviewfile /path/to/preview/file  #if not absolute, is relative to filename\n",spc);
		fprintf(f,"%swidth 100           #in pixels, overriden by the actual dimensions of the image when read in.\n",spc);
		fprintf(f,"%sheight 100          #If the file is not found or broken, then these dimensions are used.\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #affine transform to apply to the image\n",spc);
		fprintf(f,"%sdescription \"Text description, such as for captions\"\n",spc);
		
		//schema:?
		//fprintf(f,"%sfilename file\n",spc);
		//fprintf(f,"%spreviewfile file    #if not absolute, is relative to filename\n",spc);
		//fprintf(f,"%swidth int:[0..inf)  #in pixels, overriden by the actual dimensions of the image when read in.\n",spc);
		//fprintf(f,"%sheight int:[0..inf) #If the file is not found or broken, then these dimensions are used.\n",spc);
		//fprintf(f,"%smatrix affine       #affine transform to apply to the image\n",spc);
		//fprintf(f,"%sdescription string  #Text description, such as for captions\n",spc);

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
		if (previewfile) {
			if (!dump->subs_only || (dump->subs_only && is_in_subdir(previewfile,dump->basedir)))
				tmp=relative_file(previewfile,dump->basedir,1);
			fprintf(f,"%spreviewfile \"%s\"\n",spc, tmp ? tmp : previewfile);
			if (tmp) delete[] tmp;
		}
			
	} else {
		if (filename) fprintf(f,"%sfilename \"%s\"\n",spc,filename);
		if (previewfile && previewflag&1)
			fprintf(f,"%spreviewfile \"%s\"\n",spc, previewfile);
	}
	fprintf(f,"%swidth %.10g\n",spc,maxx-minx);
	fprintf(f,"%sheight %.10g\n",spc,maxy-miny);
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				m(0),m(1),m(2),m(3),m(4),m(5));
	if (description) {
		fprintf(f,"%sdescription",spc);
		dump_out_value(f,indent+2,description);
	}
}

LaxFiles::Attribute *ImageData::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
   if (!att) att=new Attribute(whattype(),NULL);

	if (what==-1) {
		att->push("filename /path/to/file");
		att->push("previewfile /path/to/preview/file","#if not absolute, is relative to filename");
		att->push("width 100","#in pixels, overriden by the actual dimensions of the image when read in.");
		att->push("height 100","#If the file is not found or broken, then these dimensions are used.");
		att->push("matrix 1 0 0 1 0 0","#affine transform to apply to the image");
		att->push("description \"Text description, such as for captions\"");

		return att;
	}
	
	DumpContext *dump=dynamic_cast<DumpContext *>(savecontext);
	if (dump && dump->basedir) {
		char *tmp=NULL;
		if (filename) {
			if (!dump->subs_only || (dump->subs_only && is_in_subdir(filename,dump->basedir)))
				tmp=relative_file(filename,dump->basedir,1);
			att->push("filename",tmp?tmp:filename);
			if (tmp) { delete[] tmp; tmp=NULL; }
		}
		if (previewfile) {
			if (!dump->subs_only || (dump->subs_only && is_in_subdir(previewfile,dump->basedir)))
				tmp=relative_file(previewfile,dump->basedir,1);
			att->push("previewfile", tmp ? tmp : previewfile);
			if (tmp) delete[] tmp;
		}
			
	} else {
		if (filename) att->push("filename",filename);
		if (previewfile && previewflag&1) att->push("previewfile",previewfile);
	}

	att->push("width", maxx-minx);
	att->push("height",maxy-miny);

	char s[120];
    sprintf(s,"%.10g %.10g %.10g %.10g %.10g %.10g",
				m(0),m(1),m(2),m(3),m(4),m(5));
	att->push("matrix",s);

	if (description) att->push("description",description);
	

	return att;
}

/*! When the image listed in the attribute cannot be loaded,
 * image is set to NULL, and the width and height attributes
 * are used if present. If the image can be loaded, then width and
 * height as given in the file are curretly ignored, and the actual pixel 
 * width and height of the image are used instead.
 *
 * If context is a DumpContext, then expand relative paths accordingly.
 */
void ImageData::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;
	char *name,*value;
	minx=miny=0;
	char *fname=NULL,*pname=NULL;
	double w=0,h=0;
	previewflag=(previewflag&~1);

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"matrix")) {
			DoubleListAttribute(value,const_cast<double*>(m()),6);
		} else if (!strcmp(name,"width")) {
			DoubleAttribute(value,&w);
		} else if (!strcmp(name,"height")) {
			DoubleAttribute(value,&h);
		} else if (!strcmp(name,"description")) {
			makestr(description,value);
		} else if (!strcmp(name,"filename")) {
			fname=value;
		} else if (!strcmp(name,"previewfile")) {
			pname=value;
		}
	}

	char *checkname=NULL;
	DumpContext *dump=dynamic_cast<DumpContext *>(context);
	if (fname && fname[0]!='/' && dump && dump->basedir) {
		fname=checkname=full_path_for_file(fname,dump->basedir);
	}

	if (w>0 && h>0) {
		maxx=w;
		maxy=h;
	}

	 // if filename is given, and old file is NULL, or is different... 
	 //  ... meaning don't load in the image if it is the same image
	if (fname && (!filename || (filename && strcmp(fname,filename)))) {
		if (pname) previewflag|=1;
		 // load an image with existing preview, do not destroy that preview when
		 // image is destroyed:
		if (LoadImage(fname,pname,0,0,0,1)) {
			 // error loading image, so use the above w,h
			minx=miny=0;
			maxx=w;
			maxy=h;
		} else {
			 //loading did fine, now sync up maxx,maxy with w,h if necessary
		}
	} else {
		 //filename borked, use stored width and height
		minx=miny=0;
		maxx=w;
		maxy=h;
	}
	if (checkname) delete[] checkname;

}

//! Flip an image vertically or horizontally, but keep the same bounding box.
/*! This does not flip the image data, only the transform.
 * Especially useful when a Displayer has unexpected handedness.
 * Horizontal flipping is provided for completeness (horiz!=0).
 *
 * \todo perhaps this should be in SomeData, since it is not dependent on content
 */
void ImageData::Flip(int horiz)
{
	if (horiz) {
		origin(origin()+xaxis()*(maxx+minx));
		xaxis(-xaxis());
	} else {
		origin(origin()+yaxis()*(maxy+miny));
		yaxis(-yaxis());
	}
}

void ImageData::Flip(flatpoint f1,flatpoint f2)
{
	Affine::Flip(f1,f2);
}

/*! Return the image's filename, if any. If none, return NULL.
 */
const char *ImageData::Filename()
{
	if (image==NULL || isblank(image->filename)) return NULL;
	return image->filename;
}

//! Set the image to the image in fname, if possible. Sets filename regardless.
/*! Return 0 for success, other for error.
 *
 * If fit!=0, then if the object's bounds are valid and nonzero, fit the image by 
 * centering into those bounds, change the object's transform, and update the bounds.
 *
 * Be forewarned that dump_in_atts() calls this function.
 * 
 * If fname cannot be opened as an image, then maxx and maxy are
 * set to 0, but filename is set
 * to fname, and the previous image is freed. 1 is returned.
 *
 * If npreview is NULL, then use whatever was in previewfile...?
 *
 * Pass 1 for del if the preview should be deleted when image is destroyed.
 */
int ImageData::LoadImage(const char *fname, const char *npreview, int maxpx, int maxpy, char del,char fit)
{
	makestr(filename,fname);
	makestr(previewfile,npreview);
	if (image) { image->dec_count(); image=NULL; }
	if (previewimage) { previewimage->dec_count(); previewimage=NULL; }
	
	LaxImage *t, *p=NULL;
	//--------
	//if (npreview) t=load_image_with_preview(fname,npreview,maxpx,maxpy, &p);
	//else t=load_image(fname);
	//--------
	t=load_image_with_loaders(fname, npreview,maxpx,maxpy,&p, 0,-1, NULL);

	if (t) {
		image=t;
		previewimage=p;

		if (fit && maxx>minx && maxy>miny) {
			 //if you want to fit the new image within the bounds of the old image
			DoubleBBox box;
			box.setbounds(this);
			minx=0; miny=0;
			maxx=image->w();
			maxy=image->h();
			fitto(NULL,&box,50.,50.);

		} else {
			minx=0; miny=0;
			maxx=image->w();
			maxy=image->h();
		}
		return 0;

	} else {
		DBG cerr <<"** warning ImageData couldn't load "<<(fname?fname:"(unknown)")<<endl;
		minx=miny=maxx=maxy=0;
	}

	return 1;
}

//! Use a different preview.
/*! This simply calls LoadImage() with the old filename, and the new preview info.
 *
 * \todo Check that this works as intended.
 */
int ImageData::UsePreview(const char *npreview, int maxpx, int maxpy, char del)
{
	cout <<"*** must check that this works: ImageData::UsePreview()"<<endl;
	
	//***should pop out old LaxImage::previewfile, and drop in new, generating if necessary...	
	//***check that this works
	LoadImage(filename,npreview,maxpx,maxpy,del);
	
	return 0;
}

//! Set a new description for the data.
void ImageData::SetDescription(const char *ndesc)
{
	makestr(description,ndesc);
}

//! Set the image to this one, dec counting the old one.
/*! 
 * Sets minx=miny=0, and maxx=image width and maxy=image height.
 * Clears filename.
 *
 * Increments count on newimage.
 * 
 * Returns 0 for success, 1 for error.
 *
 * \todo *** implement fit in here too
 */
int ImageData::SetImage(LaxImage *newimage, LaxImage *newpreview)
{
	if (image!=newimage) {
		if (image) image->dec_count();
		image=newimage;
		if (image) newimage->inc_count(); 
	}

	if (previewimage!=newpreview) {
		if (previewimage) previewimage->dec_count();
		previewimage=newpreview;
		if (previewimage) previewimage->inc_count();
	}
	
	if (newimage) {
		makestr(filename,newimage->filename);
		if (newpreview) makestr(previewfile, newpreview->filename);

		if (!image && maxx!=0 && maxy!=0) {
			 // there was no valid image, but there were bounds, so fill
			 // those bounds with new image
			double s,
				   oldaspect=maxy/maxx,
				   newaspect=newimage->h()/newimage->w();
			if (newaspect<oldaspect) s=maxx/newimage->w();
			else s=maxy/newimage->h();
			xaxis(s*xaxis());
			yaxis(s*yaxis());
		}
		minx=0; miny=0;
		maxx=image->w();
		maxy=image->h();

	} else {
		image=NULL;
		makestr(filename,NULL);
	}
	return 0;
}


//------------------------------- ImageInterface ---------------------------------
/*! \class ImageInterface
 * \brief Interface for manipulating ImageData objects.
 *
 * If (style&IMAGEI_POPUP_INFO) then when double clicking on an image, pop up an ImageDialog
 * to allow editing of file, preview, description, and title. Because of the unsatisfactory
 * limitations of the current Laxkit
 * event system, the interface cannot receive the results of the dialog directly. It has to
 * be dispatched by the viewportwindow.
 * 
 */


ImageInterface::ImageInterface(int nid,Displayer *ndp,int nstyle) : anInterface(nid,ndp)
{
	style=nstyle;

	data=NULL;
	ioc=NULL;

	showdecs=1;
	showfile=0;
	mode=0;
	controlcolor=rgbcolor(128,128,128);

	needtodraw=1;

	sc=NULL;
}

ImageInterface::~ImageInterface()
{ 	
	deletedata();
	if (sc) sc->dec_count();
	DBG cerr <<"----in ImageInterface destructor"<<endl;
}

const char *ImageInterface::Name()
{ return _("Image Tool"); }

//! Return new ImageInterface.
/*! If dup!=NULL and it cannot be cast to ImageInterface, then return NULL.
 *
 * \todo dup max_preview dims, and make it one dim, not x and y?
 */
anInterface *ImageInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new ImageInterface(id,NULL);
	else if (!dynamic_cast<ImageInterface *>(dup)) return NULL;
	
	return anInterface::duplicate(dup);
}

//! Use the object at oc if it is an ImageData.
int ImageInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	ImageData *ndata=dynamic_cast<ImageData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) deletedata();
	if (ioc) delete ioc;
	ioc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}
	needtodraw=1;
	return 1;
}

//! Check out nobj and use it as the data if possible.
/* If nobj is an ImageData, then make data be that. ViewportWindow::NewData() is called in here..
 *
 * If nobj is an ImageInfo, then change the file, preview, description of the current data..
 *
 * \todo need a little cleanup to take into account ImageInfo::mask.
 */
int ImageInterface::UseThis(anObject *nobj,unsigned int mask)
{
	if (!nobj) return 0;
	if (nobj==data) return 1;

	if (data && dynamic_cast<ImageInfo *>(nobj)) {
		ImageInfo *imageinfo=dynamic_cast<ImageInfo *>(nobj);
		data->SetDescription(imageinfo->description);
		 //load new image if one is provided, and auto fit to old image area
		data->LoadImage(imageinfo->filename,imageinfo->previewfile, 0,0,0, 1);
		return 1;

//	} else if (dynamic_cast<ImageData *>(nobj)) {
//		if (data) deletedata();
//		ImageData *ndata=dynamic_cast<ImageData *>(nobj);
//		if (viewport) {
//			ObjectContext *oc=NULL;
//			viewport->NewData(ndata,&oc);
//			if (oc) ioc=oc->duplicate();
//		}
//		data=ndata;
//		data->inc_count();
//		needtodraw=1;
//		return 1;

	} 
	return 0;
}

//! Sets showdecs=1, and needtodraw=1.
int ImageInterface::InterfaceOn()
{
	DBG cerr <<"imageinterfaceOn()"<<endl;
	showdecs=1;
	needtodraw=1;
	mode=0;
	return 0;
}

//! Calls Clear(), sets showdecs=0, and needtodraw=1.
int ImageInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	DBG cerr <<"imageinterfaceOff()"<<endl;
	return 0;
}

void ImageInterface::Clear(SomeData *d)
{
	if ((d && d==data) || (!d && data)) {
		data->dec_count(); 
		data=NULL; 
	} 
}

//! Draw ndata, but remember that data should still be the resident data.
int ImageInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<ImageData *>(ndata)==NULL) return 1;

	ImageData *bzd=data;
	data=dynamic_cast<ImageData *>(ndata);
	int td=showdecs,ntd=needtodraw;
	showdecs=2;//***
	needtodraw=1;

	Refresh();

	needtodraw=ntd;
	showdecs=td;
	data=bzd;
	return 1;
}

/*! Returns 1 if no data, -1 if thing was offscreen, or 0 if thing drawn.
 *
 * \todo **** must have check for very expanded images to only draw the section of the
 *   image that is actually onscreen, rather than massive image, which nearly freezes X
 */
int ImageInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	needtodraw=0;
	if (!data) return 1;

	//DBG cerr <<"  ImageRefresh";

	dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
	dp->BlendMode(LAXOP_Over);
		
	 // find the screen box to draw into
	 // these still need the 'dp->' because dp was transformed to space immediately holding data already
	 // The viewport is the base transform, usually different then dp coords...
	flatpoint ul=dp->realtoscreen(flatpoint(data->minx,data->miny)), 
			  ur=dp->realtoscreen(flatpoint(data->maxx,data->miny)), 
			  ll=dp->realtoscreen(flatpoint(data->minx,data->maxy)), 
			  lr=dp->realtoscreen(flatpoint(data->maxx,data->maxy));
//***	flatpoint ul=dp->realtoscreen(data->origin()+data->xaxis()*data->minx+data->yaxis()*data->miny), 
//			  ur=dp->realtoscreen(data->origin()+data->xaxis()*data->maxx+data->yaxis()*data->miny), 
//			  ll=dp->realtoscreen(data->origin()+data->xaxis()*data->minx+data->yaxis()*data->maxy), 
//			  lr=dp->realtoscreen(data->origin()+data->xaxis()*data->maxx+data->yaxis()*data->maxy);
	
	DBG cerr <<"imageinterf Refresh: "<<(data->filename ? data->filename : "(no file)")<<endl;
	DBG fprintf(stderr,"draw image scr coords: %ld: ul:%g,%g ur:%g,%g ll:%g,%g lr:%g,%g\n",
	DBG		data->object_id,ul.x,ul.y,ur.x,ur.y,ll.x,ll.y,lr.x,lr.y);
	
	 // check for positive intersection of transformed image to dp view area
	DoubleBBox bbox(ul);
	bbox.addtobounds(ur);
	bbox.addtobounds(ll);
	bbox.addtobounds(lr);
	if (!bbox.intersect(dp->Minx,dp->Maxx,dp->Miny,dp->Maxy)) {
		DBG cerr <<"----------------ImageData outside viewport"<<endl;
		return -1;
	}
	//---or---
	//flatpoint pts[4]={dp->screentoreal(ul,ur,ll,lr)}
	//bbox.bounds(pts,4);
	//bbox.intersect(data->minx,data->maxx,data->miny,data->maxy, settointersection);
	
	dp->NewFG(controlcolor);

	//if (0) {
	if (showdecs&2) {
		//draw the image
		
		int status=dp->imageout(data->image, data->minx,data->miny, data->maxx-data->minx,data->maxy-data->miny);
		if (status<0) {
			 // There is either no image or a broken image

			dp->DrawScreen();
			 
			 //draw box around it
			dp->drawline(ul,ur);
			dp->drawline(ur,lr);
			dp->drawline(lr,ll);
			dp->drawline(ll,ul);

			int up=-1;
			if (dp->defaultRighthanded()) up=1; //flip if dp is +y==up
			//if (dp->righthanded()) up=1; //flip if dp is +y==up
			
			flatpoint p=(ll+lr+ul+ur)/4; //center of image
			flatpoint tip=p+up*((ul+ur)/2-p)*2/3; //tip of an arrow from center, 2/3 toward up direction

			if (status==-1) {
				 // undefined image, draw big x
				dp->drawline(ul,lr);
				dp->drawline(ll,ur);
				 // draw arrow pointing up from center
				dp->drawline(p,tip);
				dp->drawline(tip,tip+((ll+ul)/2-tip)/5);
				dp->drawline(tip,tip+((lr+ur)/2-tip)/5);

			} else {
				 // broken image
				dp->NewFG((unsigned long)0);
				dp->NewBG(rgbcolor(128,128,128));
				flatpoint points[6];
				points[0]=p+flatpoint(-10,-10);
				points[1]=p+flatpoint( 10,-10);
				points[2]=p+flatpoint( 10, -6);
				points[3]=p+flatpoint(  5, -2);
				points[4]=p+flatpoint( -1, -5);
				points[5]=p+flatpoint( -10, 0);
				dp->drawlines(points,6, 1,2);
				
				points[0]=p+flatpoint(-10, 10);
				points[1]=p+flatpoint( 10, 10);
				points[2]=p+flatpoint( 10, -3);
				points[3]=p+flatpoint(  5, 1);
				points[4]=p+flatpoint( -1, -2);
				points[5]=p+flatpoint( -10, 3);
				dp->drawlines(points,6, 1,2);

				DBG cerr <<"******************************broken image: "<<data->filename<<endl;
			}

			dp->DrawReal();
		} //dp->imageOut returned negative
	} //showdecs==2
	
	 // draw control points, just draws an outline
	if (showdecs&1) { 
		 // for a box outline? control decorations..
		dp->DrawScreen();
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
		dp->NewFG(255,0,0);
		dp->drawline(ul,ur);
		dp->drawline(ur,lr);
		dp->drawline(lr,ll);
		dp->drawline(ll,ul);
	
		dp->NewFG(controlcolor);
		dp->DrawReal();
	}

	 // show filename
	if (showfile && data->filename) {
		dp->DrawScreen();
		dp->NewFG(rgbcolor(255,0,0));
		flatpoint p=(lr+ll+ur+ul)/4;
		char *f=NULL;
		if (showfile==2) f=strrchr(data->filename,'/');
		if (!f) f=data->filename; else f++;
		dp->textout((int)p.x,(int)p.y,f,-1);
		dp->DrawReal();
	}

	//DBG cerr<<"..Done drawing ImageInterface"<<endl;
	return 0;
}

//! Decs count of data, and Sets to NULL.
void ImageInterface::deletedata()
{
	if (data) { data->dec_count(); data=NULL; }
	if (ioc) { delete ioc; ioc=NULL; }
}

//! Create and return new data, also calls viewport->NewData(newdata).
/*! Please note that this function is not a redefinition of anything. It is used
 * internally to get a new instance of ImageData and also tell the viewport about it.
 *
 * This function causes a new ImageData to be created and to have a count of 1 for
 * the interface, and whatever the viewport tacks on.
 * Currently, new ImageData makes the maxx/maxy be the image pixel width/height..
 *
 * \todo need some flag somewhere to auto flip vertically (for +y is up rather than down)
 */
ImageData *ImageInterface::newData()
{
	ImageData *ndata=NULL;
	ndata=dynamic_cast<ImageData *>(somedatafactory()->NewObject(LAX_IMAGEDATA));
	if (ndata) {
		ndata->LoadImage(NULL);
	} 
	if (!ndata) ndata=new ImageData(NULL);//creates 1 count
	ndata->flags|=SOMEDATA_KEEP_1_TO_1; //so that when scaling, default is to maintain proportions

//	if (sgn(dp->m()[0])!=sgn(dp->m()[3])) ndata->m(3,-ndata->m(3)); //flip if dp is +y==up

	if (viewport) {
		ObjectContext *oc=NULL;
		viewport->NewData(ndata,&oc);//viewport adds only its own counts
		if (ioc) delete ioc;
		if (oc) ioc=oc->duplicate();
	}
	return ndata;
}

//! If !data on LBDown, then make a new one...
int ImageInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	DBG cerr << "  in imageinterface lbd..";
	if (buttondown.isdown(d->id,LEFTBUTTON)) return 0;
	buttondown.down(d->id,LEFTBUTTON);
	mousedragged=0;

	lx=mx=x;
	ly=my=y;
	
	 // Get rid of old data if not clicking in it.
	if (data && !data->pointin(screentoreal(x,y)) && (state&LAX_STATE_MASK)==0) {
		deletedata();
	}
	
	 // Was clicked outside current image or on blank space, make new one or find other one.
//	if (!data) {
//		if (primary && viewport->clickToChange(x,y)) return 1; //transfers control to another object
//		-----
//		viewport:
//		if (interface.e[c]->claimsClick(button, x,y,state,count,mouse,event))
//			interface.e[c]->LBDown(x,y,state,count,d);
//		else { do something with click in viewport } 
//	}
//	---------------------------
	if (!data) {
		ImageData *obj=NULL;
		ObjectContext *oc=NULL;
		int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
		if (c>0) obj=dynamic_cast<ImageData *>(oc->obj);

	 	if (obj) { 
			 // found another ImageData to work on.
			 // If this is primary, then it is ok to work on other images, but not click onto
			 // other types of objects.
			data=obj;
			data->inc_count();
			if (ioc) delete ioc;
			ioc=oc->duplicate();

			if (viewport) viewport->ChangeObject(oc,0);
			needtodraw=1;
			return 0;

		} else if (c<0) {
			 // If there is some other non-image data underneath (x,y) and
			 // this is not primary, then switch objects, and switch tools to deal
			 // with that object.
			//******* need some way to transfer the LBDown to the new tool
			if (!primary && c==-1 && viewport->ChangeObject(oc,1)) {
				buttondown.up(d->id,LEFTBUTTON);
				return 1;
			}
		}

		 // To be here, must want brand new data plopped into the viewport context
		if (viewport) viewport->ChangeContext(x,y,NULL);
		mode=1;
		data=newData(); //data has count of at least one. viewport->NewData() was called here.
		needtodraw=1;
		if (!data) return 0;

		leftp=screentoreal(x,y);
		data->origin(leftp);
		data->xaxis(flatpoint(1,0)/Getmag()/2);
		data->yaxis(flatpoint(0,1)/Getmag()/2);
		DBG data->dump_out(stderr,6,0,NULL);
		return 0;
	}

	if (count==2 && (style&IMAGEI_POPUP_INFO) && curwindow) {
		runImageDialog();
		buttondown.up(d->id,LEFTBUTTON);
		return 0;
	}

	 //to be here, must have clicked down somewhere in image

	 // Set leftp to the point in the image that the mouse was clicked down on.
	flatpoint oo=(screentoreal(x,y)-data->origin()); // real vector from data->origin() to mouse move to 
	leftp.x=(oo*data->xaxis())/(data->xaxis()*data->xaxis());
	leftp.y=(oo*data->yaxis())/(data->yaxis()*data->yaxis()); // leftp is in data coords now
	

//	if (state&ControlMask && state&ShiftMask && data) { // +^lb move around wildpoint
//		return 0;
//	} else if (state&ControlMask && data) { // ^lb focus or end angle
//		return 0;
//	} else if (state&ShiftMask && data) { // +lb start angle
//		return 0;
//	} else { // straight click
//		return 0;
//	}

	needtodraw=1;
	return 0;
	DBG cerr <<"..imageinterfacelbd done   ";
}

/*! This should result in an image properties dialog of some kind being run.
 * This function is called in response to an image being double clicked on.
 * It calls new ImageDialog with ANXWIN_REMEMBER.
 *
 * It is assumed that data and curwindow both exist.
 *
 * \todo after laxkit event mechanism gets rewritten, the return event would get
 *   sent back to this ImageInterface instance, but right now, curwindow has to
 *   somehow relay the event to the object in question.
 *
 * \todo *** at some point it might be worth laxkit's while to have a fairly abstracted
 *   general object factory, something to be able to say 
 *   newObject("ImageDialog", (anObject that wants it), other config info)
 */
void ImageInterface::runImageDialog()
{
	 //after Laxkit event system is rewritten, this will be very different:
	ImageInfo *inf=new ImageInfo(data->filename,data->previewfile,NULL,data->description,0);
	curwindow->app->rundialog(new ImageDialog(NULL,"imagedialog for imageinterface","Image Properties",
					ANXWIN_REMEMBER,
					0,0,400,400,0,
					NULL,object_id,"image properties",
					IMGD_NO_TITLE,inf));
	inf->dec_count();
}

/*! Intercept "image properties" events, passing in ImageInfo objects.
 */
int ImageInterface::Event(const Laxkit::EventData *data, const char *mes)
{
	if (!strcmp(mes,"image properties")) {
		if (!data) return 0;

		 //pass on to the first active interface that wants it, if any
		const RefCountedEventData *e=dynamic_cast<const RefCountedEventData *>(data);
		if (!e) return 1;
		ImageInfo *imageinfo=dynamic_cast<ImageInfo *>(e->object);
		if (!imageinfo) return 1;

		UseThis(imageinfo,imageinfo->mask);
		needtodraw=1;
		return 0;
	}

	return 1;
}

//! If data, then call viewport->ObjectMoved(data).
int ImageInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	if (mode==1 && !mousedragged && data) {
		DBG cerr <<"**ImageInterface Clear() for no mouse move on new imagedata"<<endl;
		if (viewport) viewport->DeleteObject();
		Clear(NULL);
	} else if (data && viewport) viewport->ObjectMoved(ioc,1);
	mode=0;
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}

int ImageInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse) 
{
	if (!buttondown.any() || !data) return 0;
	if (x!=mx || y!=my) mousedragged=1;

	 // Section out a simple rectangular area to later drop image into if mode==1
	if (mode==1) {
		double m[6];
		transform_invert(m,data->m());
		flatpoint p =screentoreal(x,y);
		flatpoint oo=transform_point(m, leftp);
		p=transform_point(m,p);

		flatpoint o;
		double dx=p.x-oo.x,
			   dy=p.y-oo.y;

		if (dx>0) {
			o.x=oo.x;
			data->maxx=dx;
		} else {
			o.x=p.x;
			data->maxx=-dx;
		}
		if (dy>0) {
			o.y=oo.y;
			data->maxy=dy;
		} else {
			o.y=p.y;
			data->maxy=-dy;
		}
		data->origin(transform_point(data->m(),o));
		mx=x;
		my=y;
		needtodraw=1;
		return 0;
	}
	
	 // If mode!=1, then do normal rotate and scale
	flatpoint d; // amount to shift the image origin.
	flatpoint p=screentoreal(x,y),    //real point where mouse moved to
				//real point where mouse clicked:
				//should be the same as screentoreal(lx,ly)
			  lp=data->origin() + leftp.x*data->xaxis() + leftp.y*data->yaxis(); 
	flatpoint oo,
			  o; // the point in image coords the mouse is over
	oo=(p-data->origin()); // real vector from data->origin() to mouse move to 
	o.x=(oo*data->xaxis())/(data->xaxis()*data->xaxis());
	o.y=(oo*data->yaxis())/(data->yaxis()*data->yaxis()); // o is in data coords now
	
	//DBG cerr <<"x,y="<<x<<','<<y<<"  p="<<p.x<<","<<p.y<<"  o="<<o.x<<','<<o.y;

	if (!buttondown.isdown(mouse->id,LEFTBUTTON) || !data || !dp) return 1;
	if (x==mx && y==my) return 0;
	if (state&ControlMask && state&ShiftMask) { //rotate
		double angle=x-mx;
		data->xaxis(rotate(data->xaxis(),angle,1));
		data->yaxis(rotate(data->yaxis(),angle,1));
		d=lp-(data->origin()+data->xaxis()*leftp.x+data->yaxis()*leftp.y);
		data->origin(data->origin()+d);
	} else if (state&ControlMask) { // scale
		if (x>mx) {
			if (data->xaxis()*data->xaxis()<dp->upperbound*dp->upperbound) {
				data->xaxis(data->xaxis()*1.05);
				data->yaxis(data->yaxis()*1.05);
			}
		} else if (x<mx) {
			if (data->xaxis()*data->xaxis()>dp->lowerbound*dp->lowerbound) {
				data->xaxis(data->xaxis()/1.05);
				data->yaxis(data->yaxis()/1.05);
			}
		}
		oo=data->origin() + leftp.x*data->xaxis() + leftp.y*data->yaxis(); // where the point clicked down on is now
		//DBG cerr <<"  oo="<<oo.x<<','<<oo.y<<endl;
		d=lp-oo;
		data->origin(data->origin()+d);
	} else { //translate
		d=screentoreal(x,y)-screentoreal(mx,my);
		data->origin(data->origin()+d);
	}
	//DBG cerr <<"  d="<<d.x<<','<<d.y<<endl;
	mx=x; my=y;
	needtodraw|=2;
	return 0;
}

enum ImageInterfaceActions {
	II_Normalize,
	II_Rectify,
	II_Decorations,
	II_ToggleLabels,
	II_FlipH,
	II_FlipV,
	II_Image_Info,
	II_MAX
};

Laxkit::ShortcutHandler *ImageInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(II_Normalize,    'n',0,0,         "Normalize",     _("Normalize"),NULL,0);
	sc->Add(II_Rectify,      'N',ShiftMask,0, "Rectify",       _("Clear aspect and rotation"),NULL,0);
	sc->Add(II_Decorations,  'd',0,0,         "Decorations",   _("Toggle decorations"),NULL,0);
	sc->Add(II_ToggleLabels, 'f',0,0,         "Labels",        _("Toggle labels"),NULL,0);
	sc->Add(II_FlipH,        'h',0,0,         "FlipHorizontal",_("Flip horizontally"),NULL,0);
	sc->Add(II_FlipV,        'v',0,0,         "FlipVertical",  _("Flip vertically"),NULL,0);
	sc->Add(II_Image_Info,   LAX_Enter,0,0,   "ImageInfo",     _("Edit image info"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

int ImageInterface::PerformAction(int action)
{
	if (action==II_Normalize || action==II_Rectify) {
		if (!data) return 0;
		if (action==II_Rectify) {
			double x=norm(data->xaxis());
			data->xaxis(flatpoint(x,0));
		}
		data->yaxis(transpose(data->xaxis()));
		needtodraw=1;
		return 0;

	} else if (action==II_Image_Info) {
		runImageDialog();
		return 0;

	} else if (action==II_FlipH) {
		if (!data) return 0;
		data->Flip(1);
		//data->Flip(data->transformPoint(flatpoint(data->minx, (data->miny+data->maxy)/2)),
				   //data->transformPoint(flatpoint(data->maxx, (data->miny+data->maxy)/2)));
		needtodraw=1;
		return 0;

	} else if (action==II_FlipV) {
		if (!data) return 0;
		data->Flip(0);
		//data->Flip(data->transformPoint(flatpoint((data->minx+data->maxx)/2, data->miny)),
				   //data->transformPoint(flatpoint((data->minx+data->maxx)/2, data->maxy)));
		needtodraw=1;
		return 0;

	} else if (action==II_Decorations) {
		if (--showdecs<0) showdecs=3;
		needtodraw=1;
		return 0;

	} else if (action==II_ToggleLabels) {
		showfile++;
		if (showfile==3) showfile=0;
		needtodraw=1;
		DBG cerr <<"ImageInterface showfile: "<<(int)showfile<<endl;
		return 0;
	}

	return 1;
}

int ImageInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1; 
}



} // namespace LaxInterfaces


