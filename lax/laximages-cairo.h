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
//    Copyright (C) 2013 by Tom Lechner
//
#ifndef _LAX_LAXIMAGES_CAIRO_H
#define _LAX_LAXIMAGES_CAIRO_H


#include <lax/configured.h>


#ifdef LAX_USES_CAIRO

#include <lax/laximages.h>

#include <cairo/cairo-xlib.h>

namespace Laxkit {



//--------------------------- LaxCairoImage --------------------------------------
class LaxCairoImage : public LaxImage
{
 protected:
	char flag;
	int display_count;

 public:
	cairo_surface_t *image;
	int width,height;

	LaxCairoImage();
	LaxCairoImage(int w, int h);
	LaxCairoImage(const char *fname, cairo_surface_t *img=NULL);
	LaxCairoImage(const char *original, const char *fname, int maxw, int maxh);
	virtual ~LaxCairoImage();
	virtual cairo_surface_t *Image();
	virtual void doneForNow();
	virtual unsigned int imagestate();
	virtual int imagetype() { return LAX_IMAGE_CAIRO; }
	virtual int w() { return width; }
	virtual int h() { return height; }
	virtual void clear();

	virtual int createFromData_ARGB8(int width, int height, int stride, const unsigned char *data, bool not_premultiplied);
	virtual unsigned char *getImageBuffer();
	virtual int doneWithBuffer(unsigned char *buffer);

	virtual int Save(const char *tofile = nullptr, const char *format = nullptr); //format==null guess from extension
};


//----------------- LaxCairoImage utils

int laxcairo_image_type();

//LaxImage *load_cairo_image(const char *filename);
//LaxImage *load_cairo_image_with_preview(const char *filename,const char *previewfile,int maxx,int maxy,LaxImage **previewimage_ret);
//int laxcairo_generate_preview(const char *original, const char *preview, const char *format,int maxw, int maxh, int fit);
//
//LaxImage *image_from_buffer_cairo(unsigned char *buffer, int w, int h, int stride);
//LaxImage *create_new_cairo_image(int w, int h);
//
//int save_image_cairo(LaxImage *image, const char *filename, const char *format);


////--------------------------- CairoLoader --------------------------------------
class CairoImageLoader : public ImageLoader
{
  protected:

  public:
	CairoImageLoader();
	virtual ~CairoImageLoader();

	virtual bool CanLoadFile(const char *file);
	virtual bool CanLoadFormat(const char *format);
	virtual int PingFile(const char *file, int *width, int *height, long *filesize, int *subfiles);
	virtual int LoadToMemory(LaxImage *img);

	 //return a LaxImage in target_format.
	 //If must_be_that_format and target_format cannot be created, then return NULL.
	virtual LaxImage *load_image(const char *filename,
								 const char *previewfile, int maxx, int maxy, LaxImage **previewimage_ret,
								 int required_state, //any of metrics, or image data, or preview data
								 int target_format,
								 int *actual_format,
								 bool ping_only,
								 int index);
	virtual LaxImage *CreateImage(int width, int height, int format = LAX_IMAGE_DEFAULT);
	virtual LaxImage *CreateImageFromBuffer(unsigned char *data, int width, int height, int stride, int format = LAX_IMAGE_DEFAULT);
};


} //namespace Laxkit

#endif //uses cairo
#endif //_LAX_LAXIMAGES_CAIRO_H

