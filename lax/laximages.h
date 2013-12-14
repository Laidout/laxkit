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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_LAXIMAGES_H
#define _LAX_LAXIMAGES_H

#include <lax/anobject.h>
#include <sys/times.h>

#define LAX_IMAGE_NULL            0
#define LAX_IMAGE_BUFFER          1
#define LAX_IMAGE_XIMAGE          2
#define LAX_IMAGE_PIXMAP          3
#define LAX_IMAGE_IMLIB           4
#define LAX_IMAGE_CAIRO           5
#define LAX_IMAGE_ANTIGRAIN       6
#define LAX_IMAGE_GL              7
#define LAX_IMAGE_FIRST_USER_TYPE 1000

#define LAX_IMAGE_METRICS        (1<<0)
#define LAX_IMAGE_PREVIEW        (1<<1)
#define LAX_IMAGE_WHOLE          (1<<2)
#define LAX_IMAGE_HAS_FILE       (1<<3)
#define LAX_IMAGE_HAS_PREVIEW    (1<<4)

namespace Laxkit {
	

class aDrawable;


//--------------------------- LaxImage --------------------------------------
class LaxImage : public anObject
{
 public:
	char delpreview;
	char *filename;
	char *previewfile;
	clock_t lastaccesstime;
	LaxImage(const char *fname);
	virtual ~LaxImage();
	virtual int imagetype()=0;
	virtual unsigned int imagestate()=0;
	virtual void doneForNow() {}
	virtual int w()=0;
	virtual int h()=0;
	virtual int dataw()=0;
	virtual int datah()=0;
	virtual void clear()=0;

	virtual unsigned char *getImageBuffer() = 0;
	virtual int doneWithBuffer(unsigned char *buffer) = 0;

	//virtual int MaxMemoryUsed();
	//virtual int dec_count();
};


//--------------------------- LaxImage utils --------------------------------------

typedef int (*DefaultImageTypeFunc)();
extern DefaultImageTypeFunc default_image_type;

//---------------- image_out_* 

typedef void (*ImageOutFunc)(LaxImage *image, aDrawable *win, int ulx, int uly);
typedef void (*ImageOutRotatedFunc)(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury);
typedef void (*ImageOutSkewedFunc)(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury, int llx, int lly);
typedef void (*ImageOutMatrixFunc)(LaxImage *image, aDrawable *win, double *m);

extern ImageOutFunc        image_out;
extern ImageOutRotatedFunc image_out_rotated;
extern ImageOutSkewedFunc  image_out_skewed;
extern ImageOutMatrixFunc  image_out_matrix;


//---------------- base preview creation
typedef int  (*GeneratePreviewFunc)(const char *original_file, 
									const char *to_preview_file,
									const char *format,int width, int height, int fit); 
extern GeneratePreviewFunc generate_preview_image;


//---------------- load_image 

typedef LaxImage *(*CreateNewImageFunc)(int w, int h);
typedef LaxImage *(*ImageFromBufferFunc)(unsigned char *buffer, int w, int h, int stride);
typedef LaxImage *(*LoadImageFunc)(const char *filename);
typedef LaxImage *(*LoadImageWithPreviewFunc)(const char *filename,const char *previewfile,
											  int maxx,int maxy,char delpreview);

extern CreateNewImageFunc       create_new_image;
extern ImageFromBufferFunc      image_from_buffer;
extern LoadImageFunc            load_image;
extern LoadImageWithPreviewFunc load_image_with_preview;



} //namespace Laxkit

#endif

