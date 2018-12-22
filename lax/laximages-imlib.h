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
//    Copyright (C) 2010 by Tom Lechner
//

#ifndef _LAX_LAXIMAGES_IMLIB_H
#define _LAX_LAXIMAGES_IMLIB_H


#include <lax/configured.h>

#ifdef LAX_USES_IMLIB


#include <lax/laximages.h>
#include <Imlib2.h>

namespace Laxkit {


//--------------------------- LaxImlibImage --------------------------------------
class LaxImlibImage : public LaxImage
{
 protected:
	char flag, whichimage;

 public:
	Imlib_Image image;
	int width,height;

	LaxImlibImage(const char *fname,Imlib_Image img=0);
	LaxImlibImage(const char *original, const char *npreviewfile, int maxx,int maxy);
	virtual ~LaxImlibImage();

	virtual Imlib_Image Image();
	virtual void doneForNow();
	virtual unsigned int imagestate();
	virtual int imagetype() { return LAX_IMAGE_IMLIB; }
	virtual int w() { return width; }
	virtual int h() { return height; }
	virtual void clear();

	virtual unsigned char *getImageBuffer();
	virtual int doneWithBuffer(unsigned char *buffer);

	virtual int Save(const char *tofile = nullptr, const char *format = nullptr); //format==null guess from extension
};


//----------------- LaxImlibImage utils

 //-----extra utility functions
void laximlib_usealpha(int yes);
void laximlib_update_alpha(int alpha);
void laximlib_alternate_drawable(Drawable drawable);

 //----------default image function replacements
int laximlib_image_type();


int save_imlib_image(LaxImage *image, const char *filename, const char *format);

LaxImage *load_imlib_image(const char *filename);
LaxImage *load_imlib_image_with_preview(const char *filename,const char *previewfile,int maxx,int maxy,LaxImage **previewimage_ret);
int laximlib_generate_preview(const char *original_file, const char *to_preview_file, const char *format,int maxw, int maxh, int fit);
LaxImage *image_from_buffer_imlib(unsigned char *buffer, int w, int h, int stride);
LaxImage *create_new_imlib_image(int w, int h);



//--------------------------- ImlibLoader --------------------------------------
class ImlibLoader : public ImageLoader
{
  protected:

  public:
	ImlibLoader();
	virtual ~ImlibLoader();

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

#endif //LAX_USES_IMLIB
#endif

