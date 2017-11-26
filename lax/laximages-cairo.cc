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




#include <cstdio>
#include <errno.h>

#include <lax/laximages-cairo.h>
#include <lax/strmanip.h>
#include <lax/vectors.h>
#include <lax/transformmath.h>
#include <lax/anxapp.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 



#ifdef LAX_USES_CAIRO




namespace Laxkit {


//--------------------------- LaxCairoImage --------------------------------------
/*! \class LaxCairoImage
 * \brief A LaxImage based on cairo image surfaces in a cairo_surface_t.
 */
/*! \var char LaxCairoImage::flag
 * \brief 1 if the image should be free'd when not in use (assumes filename is good). else 0.
 */
/*! \var int LaxCairoImage::width
 * \brief The actual width of the full image.
 */
/*! \var int LaxCairoImage::height
 * \brief The actual height of the full image.
 */


LaxCairoImage::LaxCairoImage()
  : LaxImage(NULL)
{ 
	flag=0;
	display_count=0;

	image=NULL;
	width=height=0;
}

/*! If fname and img, then assume that img corresponds to fname, read dimensions from img,
 * then free it.
 * If fname and !img, then get the dims from cairo by loading fname and reading off dims.
 * If !fname and img, then set flag=0 (do not free image during LaxImage lifetime).
 *
 * Note that Cairo docs say loading, reading off things, then freeing is a good thing.
 *
 * 
 * \todo *** needs to be some error checking when generating new previews
 * \todo scaling to maxx OR maxy if either 0 not implemented. both must currently be nonzero.
 * \todo when generating preview, might be wise to have check for freedesktop thumb locations to enforce
 *    proper sizing
 */
LaxCairoImage::LaxCairoImage(const char *fname, cairo_surface_t *img)
	: LaxImage(fname)
{
	display_count=0; 
	flag=0;
	image=NULL;

	if (!img) {
		if (fname) {
			image=cairo_image_surface_create_from_png(fname); //***need real image loaders...
			if (cairo_surface_status(image)!=CAIRO_STATUS_SUCCESS) {
				 //is nil surface
				cairo_surface_destroy(image);
				image=NULL;
			}
		}
	} else image=img;

	if (image) {
		width= cairo_image_surface_get_width(image),
	    height=cairo_image_surface_get_height(image);
		if (!img || (img && fname)) {
			 //free the just loaded image
			cairo_surface_destroy(image);
			image=NULL;

		} else if (fname) flag=1;

	} else {
		width=height=0;
	}

}

/*! Load image, but rescale to fit within a box maxw by maxy.
 */
LaxCairoImage::LaxCairoImage(const char *original, const char *fname, int maxw, int maxh)
	: LaxImage(fname)
{
	display_count=0;
	flag=0;
	image=NULL;

	if (maxh==0) maxh=maxw;

	// ***
/* The preview image is set up only if the main image exists, and only if the image's
 * dimensions exceed maxx or maxy. Also if the main image does not exist, previewfile
 * is set to NULL, and npfile is ignored. Whether the main image exists is by checking 
 * whether width>0, not by whether filename exists, since there might be no associated
 * filename for the image.
 *
 * If npfile is given, but maxx<=0 or maxy<=0, then set up the preview file only if
 * it existed already. A new preview is not generated. A new preview IS generated whenever
 * maxx>0 or maxy>0 AND the main image's dimensions are too big for the specified bounds, 
 * AND npfile cannot be opened as an image by Cairo.
 * If only one of maxx or maxy is greater than 0, then only fit to be within that bound.
 */


	 // only set up the preview image if the main image exists and is readable
	cairo_surface_t *pimage = cairo_image_surface_create_from_png(fname);

	if (cairo_surface_status(pimage)!=CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(pimage);
		pimage=NULL;
		width=height=0;
	}

	if (pimage) {
		width= cairo_image_surface_get_width(pimage);
		height=cairo_image_surface_get_height(pimage);

		if (maxw>0 && maxh>0 && (width>maxw || height>maxh)) {
		
			 //****make sure previewfile is writable
			 
			 //figure out dimensions of new preview
			double a=double(height)/width;
			int nwidth=0, nheight=0;

			if (a*maxw>maxh) {
				nheight=maxh;
				nwidth=int(maxh/a);
			} else {
				nwidth=maxw;
				nheight=int(maxw*a);
			}

			 //need to resize to a smaller surface
			cairo_surface_t *nimage=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, nwidth,nheight);
			cairo_surface_destroy(pimage);

			DBG cerr << " *** FINISH IMPLEMENTING LaxCairoImage::LaxCairoImage(const char *fname, int maxw, int maxh)!!"<<endl;

			//generate_preview_image(filename,previewfile,"jpg",dwidth,dheight,0);
			image=nimage;

		} else {
			cairo_surface_destroy(pimage);
		}
	}
}

//! Free image if it happens to exist.
LaxCairoImage::~LaxCairoImage()
{
	if (image) {
		cairo_surface_destroy(image);
		image=NULL;
	}
}

/*! MUST be followed up with call to doneWithBuffer().
 *
 * This returns a new uchar[] array.
 */
unsigned char *LaxCairoImage::getImageBuffer()
{
	if (!image) image=Image();
	DBG cerr <<" LaxCairoImage::getImageBuffer()"<<endl;

	 //cairo buffers have premultiplied alpha
	cairo_surface_flush(image);
	unsigned char *buffer=cairo_image_surface_get_data(image);

	int width =cairo_image_surface_get_width(image);
	int height=cairo_image_surface_get_height(image);
	int stride=cairo_image_surface_get_stride(image);
	cairo_format_t format=cairo_image_surface_get_format(image);

	unsigned char *bbuffer=new unsigned char[width*height*4];
	if (format==CAIRO_FORMAT_ARGB32 || format==CAIRO_FORMAT_RGB24) {

		//DBG cerr <<"*** need to correctly apply premultiplied in cairo image / buffer exchange"<<endl;

		unsigned char *p=buffer, *pb=bbuffer;
		for (int c=0; c<height; c++) {
			memcpy(pb,p, width*4);
			p+=stride;
			pb+=width*4;
		}
	}

	return bbuffer;
}

/*! Check in data that was just checked out with getImageBuffer().
 * bbuffer gets delete'd.
 *
 * Assumes 8 bit ARGB.
 */
int LaxCairoImage::doneWithBuffer(unsigned char *bbuffer)
{
	 //need to translate from rgba to premultiplied rgba
	if (!image) image=Image();
	if (!image) return 1;

	DBG cerr <<" LaxCairoImage::doneWithBuffer()"<<endl;

	 //cairo buffers have premultiplied alpha
	unsigned char *buffer=cairo_image_surface_get_data(image);
	int width =cairo_image_surface_get_width(image);
	int height=cairo_image_surface_get_height(image);
	int stride=cairo_image_surface_get_stride(image);
	cairo_format_t format=cairo_image_surface_get_format(image);

	if (format==CAIRO_FORMAT_ARGB32 || format==CAIRO_FORMAT_RGB24) {
		cerr <<"*** need to correctly apply premultiplied in cairo image / buffer exchange"<<endl;

		unsigned char *p=buffer, *pb=bbuffer;
		for (int c=0; c<height; c++) {
			memcpy(p,pb, width*4);
			p+=stride;
			pb+=width*4;
		}
	}



	delete[] bbuffer;
	cairo_surface_mark_dirty (image);

	return 0;
}

//! Create a LaxCairoImage from raw 8 bit data, arranged ARGB.
/*! If stride!=0, then each row takes up this many bytes. If stride==0, then
 * assume 4*width.
 *
 * 8 bit ARGB happens to be compatible with both Imlib2 and cairo.
 *
 * The data is copied to a new buffer, which is stored within the object.
 *
 * Returns 0 for success, or 1 for error in processing, for instance, if the stride
 * is not a reasonable value.
 */
int LaxCairoImage::createFromData_ARGB8(int nwidth, int nheight, int stride, const unsigned char *data)
{
	if (!data) return 1;
	clear();

	image=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, nwidth,nheight);
	width =nwidth;
	height=nheight;
	cairo_t *cr=cairo_create(image);

	cairo_surface_t *image;
	image=cairo_image_surface_create_for_data(const_cast<unsigned char*>(data),CAIRO_FORMAT_ARGB32, nwidth,nheight, stride);
	//if (!image) return NULL;
	cairo_set_source_surface(cr,image, 0,0);
	cairo_paint(cr);
	cairo_surface_destroy(image);
	cairo_destroy(cr);


	 //---alternate method: keep external buffer around:
//	if (stride==0) stride=4*width;
//	int cstride=cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,width);
//	 //the cairo format can be: CAIRO_FORMAT_ARGB32
//	 //						    CAIRO_FORMAT_RGB24  (no alpha)
//	 //						    CAIRO_FORMAT_A8  (8 bit gray scale)
//	 //						    CAIRO_FORMAT_A1  (bitmap)
//	if (stride>cstride) return 1;
//	buffer=new unsigned char[cstride*height];
//	if (cstride==stride) {
//		memcpy(buffer, data, cstride*height);
//	} else {
//		DBG cerr <<" strange stride value for buffer->cairo! copying row by row"<<endl;
//		int i=0,i2;
//		for (int c=0; c<height; c++, i+=cstride, i2+=stride) {
//			memcpy(buffer+i, data+i2, stride);
//		}
//	}




	return 0;
}

/*! \todo shouldn't always free the buffer if any, just to save on allocation time..
 *         resizing down is ok if buffer is already big enough, and the resize isn't drastic
 */
void LaxCairoImage::clear()
{	
	 //free any image data
	if (filename) { delete[] filename; filename=NULL; }
	width=height=0;

	cairo_surface_destroy(image);
	image=NULL;
}

unsigned int LaxCairoImage::imagestate()
{
	return (filename ? LAX_IMAGE_HAS_FILE : 0) |
		   (width>0 ? LAX_IMAGE_METRICS : 0) |
		   (image!=NULL ? LAX_IMAGE_WHOLE : 0);
}


/*! Calling this is supposed to make it easier on the memory cache, by allowing
 * other code to free the cairo_surface from memory. Calling Image() will place
 * it back in memory.
 */
void LaxCairoImage::doneForNow()
{
	if (!image || flag) return;

	if (display_count>0) display_count--;
}

//! Return the image. Loads from filename if !image.
/*! Attempts to load the previewfile. If that fails, then
 * attempts to load the real file.
 *
 * which==0 is get default (whichever is already loaded, or if none loaded, then 
 * preview if avaible, else main image), 1 is main, 2 is preview.
 * If 1 and main is unavailable, or 2 and the preview is unavailable, then NULL is
 * returned.
 *
 * \todo note that if image is already loaded, this function does not yet switch to
 *   the proper one..
 */
cairo_surface_t *LaxCairoImage::Image()
{
	if (!image) {
		if (importer && filename) {
			importer->LoadToMemory(this);

		} else {
			image=cairo_image_surface_create_from_png(filename);
			if (cairo_surface_status(image)!=CAIRO_STATUS_SUCCESS) {
				cairo_surface_destroy(image);
				image=NULL;

			} else { //success loading
				if (width<=0 || height<=0) {
					width= cairo_image_surface_get_width(image),	
					height=cairo_image_surface_get_height(image);
				}
			}
		}
	} 

	display_count++;
	return image;
}

//------------------- LaxCairoImage utils

//! Generate a preview image. Return 0 for success.
/*! WARNING: this does no sanity checking on file names, and will force an overwrite.
 * It is the responsibility of the calling code to do those things, and to
 * ensure that preview is in fact a writable path.
 * 
 * \todo *** afterwards, make sure preview was actually written
 */
int laxcairo_generate_preview(const char *original,
						   const char *preview, 
						   const char *format, 
						   int width, int height, int fit)
{
	DBG cerr <<" *** need to use loaders to load original for preview generation in laxcairo_generate_preview()!!"<<endl;

	cairo_surface_t *image=NULL,*pimage;

	if (!image) {
		image=cairo_image_surface_create_from_png(original);
		if (cairo_surface_status(image)!=CAIRO_STATUS_SUCCESS) {
			cairo_surface_destroy(image);
			image=NULL;
		}
	}

	if (!image) return 1;

	int owidth= cairo_image_surface_get_width(image),
	    oheight=cairo_image_surface_get_height(image);
	
	if (fit) {
		 //figure out dimensions of new preview
		double a=double(oheight)/owidth;
		int ww=width, hh=height;
		if (a*ww>hh) {
			height=hh;
			width=int(hh/a);
		} else {
			width=ww;
			height=int(ww*a);
		}
	}

	if (width>0 && height>0) {
		//pimage=cairo_create_cropped_scaled_image(0,0, owidth,oheight, width,height);
		pimage=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width,height);
		cairo_t *cr=cairo_create(pimage);
		cairo_set_source_surface(cr,image, 0,0);
		cairo_scale(cr,(double)width/owidth,(double)height/oheight);
		cairo_paint(cr);
		cairo_destroy(cr);
	}
	cairo_surface_destroy(image);
	image=NULL;
	if (width<=0 || height<=0) return 2;

	cairo_status_t status=cairo_surface_write_to_png(pimage,preview);
	if (status!=CAIRO_STATUS_SUCCESS) {
		DBG cerr <<"Error saving cairo preview: "<<cairo_status_to_string(status)<<endl;
	}
	cairo_surface_destroy(pimage);

	return 0;
}




/*! \ingroup laximages
 * Set image, set drawable, then uses cairo_render_image_on_drawable().
 */
void laxcairo_image_out(LaxImage *image, aDrawable *win, int ulx, int uly)
{
	if (!dynamic_cast<LaxCairoImage *>(image)) return;

//	cairo_surface_t *surface=cairo_xlib_surface_create(win->app->dpy, win->xlibDrawable(), win->app->vis, win->win_w,win->win_h);
//	cairo_t *cr=cairo_create(surface);
//
//	cairo_set_source_surface(cr,dynamic_cast<LaxCairoImage *>(image)->Image());
//	cairo_paint();
//	cairo_destroy(cr);
//	cairo_dostroy_surface(surface);
//	----
	Displayer *dp=GetDefaultDisplayer();
	dp->MakeCurrent(win);
	dp->imageout(image,ulx,uly);
}

/*! \ingroup laximages
 * Set image, set drawable, then uses Displayer::imageout_rotated().
 */
void laxcairo_image_out_rotated(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury)
{
	if (!dynamic_cast<LaxCairoImage *>(image)) return;
	Displayer *dp=GetDefaultDisplayer();
	dp->MakeCurrent(win);
	dp->imageout_rotated(image, ulx,uly, urx,ury);
}

/*! \ingroup laximages
 * Set image, set drawable, then uses cairo_render_image_on_drawable_skewed().
 */
void laxcairo_image_out_skewed(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury, int llx, int lly)
{
	if (!dynamic_cast<LaxCairoImage *>(image)) return;
	Displayer *dp=GetDefaultDisplayer();
	dp->MakeCurrent(win);
	dp->imageout_skewed(image, ulx,uly, urx,ury, llx,lly);
}

/*! \ingroup laximages
 * Set image, set drawable, then uses cairo_render_image_on_drawable_skewed() according to 
 * the affine matrix m.
 */
void laxcairo_image_out_matrix(LaxImage *image, aDrawable *win, double *m)
{
	if (!dynamic_cast<LaxCairoImage *>(image)) return;
	Displayer *dp=GetDefaultDisplayer();
	dp->MakeCurrent(win);
	dp->imageout(image, m);
}


//! Function that returns a new LaxCairoImage.
/*! \ingroup laximages
 *  This loads the image, grabs the dimensions and does LaxCairoImage::doneForNow().
 *
 * To use a preview image, see _load_cairo_image_with_preview().
 */
LaxImage *load_cairo_image(const char *filename)
{
	if (!filename) return NULL;

	cairo_surface_t *image;
	image=cairo_image_surface_create_from_png(filename);
	if (cairo_surface_status(image)!=CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(image);
		image=NULL;
	}

	if (!image) { 
		 //cairo load failed, try other loaders
		LaxImage *img = load_image_with_loaders(filename, NULL,0,0,NULL, 0, LAX_IMAGE_CAIRO, NULL, false);
		return img;
	}

	LaxCairoImage *img=new LaxCairoImage(filename,image);
	img->doneForNow();
	return img;
}

//! Function that returns a new LaxCairoImage with preview.
/*! \ingroup laximages
 *  This loads the images, grabs the dimensions. If the preview path does not exist,
 *  then a new preview is generated that is within the bounds of maxx and maxy,
 *  and saved to the path if possible. 
 *
 *  If the preview already exists, then use it, without regenerating. If the preview path
 *  is not a valid image, then previewfile is set to NULL, but filename is still used as possible.
 *
 *  LaxCairoImage::doneForNow() is finally called.
 */
LaxImage *load_cairo_image_with_preview(const char *filename,
										 const char *previewfile,
										 int maxx,int maxy,
										 LaxImage **previewimage_ret)
{
	cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
	if (cairo_surface_status(image)!=CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(image);
		image=NULL;
	}
	if (!image) return NULL;

	LaxCairoImage *img=new LaxCairoImage(filename, image);
	img->doneForNow();

	if (previewimage_ret) {
		LaxCairoImage *pimg=new LaxCairoImage(filename, previewfile, maxx,maxy);
		pimg->doneForNow();

		*previewimage_ret = pimg;
	}

	return img;
}

//! Simply return a new imlib image.
/*! Note that this image will not be cached, since it is not associated
 * with a file.
 */
LaxImage *create_new_cairo_image(int w, int h)
{
	cairo_surface_t *image;
	image=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w,h);
	if (!image) return NULL;
	LaxCairoImage *img=new LaxCairoImage(NULL,image);
	img->doneForNow();
	return img;
}


//! Basically create a nem Imlib_Image, and copy buffer to its data.
/*! buffer is assumed to be ARGB 8 bit data.
 *
 * Note that this image will not be cached, since it is not associated
 * with a file.
 */
LaxImage *image_from_buffer_cairo(unsigned char *buffer, int w, int h, int stride)
{
	cairo_surface_t *ii=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w,h);
	cairo_t *cr=cairo_create(ii);

	cairo_surface_t *image;
	image=cairo_image_surface_create_for_data(buffer,CAIRO_FORMAT_ARGB32, w,h,stride);
	//if (!image) return NULL;
	cairo_set_source_surface(cr,image, 0,0);
	cairo_paint(cr);
	cairo_surface_destroy(image);

	LaxCairoImage *img=new LaxCairoImage(NULL,ii);
	img->doneForNow();
	return img;
}

int laxcairo_image_type()
{ return LAX_IMAGE_CAIRO; }


int save_image_cairo(LaxImage *image, const char *filename, const char *format)
{
	LaxCairoImage *img = dynamic_cast<LaxCairoImage *>(image);
	if (!img) return 1;

	cairo_surface_t *surface=img->image;
	if (!surface) return 2;

	cairo_status_t status = cairo_surface_write_to_png(surface, filename);
	if (status!=CAIRO_STATUS_SUCCESS) {
		DBG cerr <<"Error saving cairo preview: "<<cairo_status_to_string(status)<<endl;
		return 1;
	}
	return 0;
}


} //namespace Laxkit




#else
//-------------------------if no cairo support:-------------------------------------



namespace Laxkit {



int laxcairo_image_type()
{ return LAX_IMAGE_CAIRO; }


void laxcairo_image_out(LaxImage *image, anXWindow *win, int ulx, int uly)
{
	printf(" ** Warning! laxcairo_image_out() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
}

void laxcairo_image_out_rotated(LaxImage *image, anXWindow *win, int ulx,int uly, int urx,int ury)
{
	printf(" ** Warning! laxcairo_image_out_rotated() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
}

void laxcairo_image_out_skewed(LaxImage *image, anXWindow *win, int ulx,int uly, int urx,int ury, int llx, int lly)
{
	printf(" ** Warning! laxcairo_image_out_skewed() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
}

void laxcairo_image_out_matrix(LaxImage *image, anXWindow *win, double *m)
{
	printf(" ** Warning! laxcairo_image_out_matrix() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
}

LaxImage *load_cairo_image(const char *filename)
{
	printf(" ** Warning! load_cairo_image() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
	return NULL;
}

LaxImage *load_cairo_image_with_preview(const char *filename,const char *previewfile,int maxx,int maxy,char del)
{
	printf(" ** Warning! load_cairo_image_with_preview() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
	return NULL;
}

int laxcairo_generate_preview(const char *original, const char *preview, const char *format,int maxw, int maxh, int fit)
{
	printf(" ** Warning! laxcairo_generate_preview() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
	return 1;
}

LaxImage *create_new_cairo_image(int w, int h)
{
	printf(" ** Warning! create_new_cairo_image() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
	return NULL;
}

LaxImage *image_from_buffer_cairo(unsigned char *buffer, int w, int h)
{
	printf(" ** Warning! image_from_buffer_cairo() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
	return NULL;
}





} //namespace Laxkit

#endif

