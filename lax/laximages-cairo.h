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
#ifndef _LAX_LAXIMAGES_CAIRO_H
#define _LAX_LAXIMAGES_CAIRO_H



#include <lax/laximages.h>



#ifdef LAX_USES_CAIRO

#include <cairo/cairo-xlib.h>

namespace Laxkit {
	


//--------------------------- LaxCairoImage --------------------------------------
class LaxCairoImage : public LaxImage
{
 protected:
	char flag,whichimage;
	int display_count;
 public:
	cairo_surface_t *image;
	int width,height;
	int dwidth,dheight;
	LaxCairoImage(const char *fname,cairo_surface_t *img=NULL,const char *npreviewfile=NULL,
				  int maxx=0,int maxy=0,char del=0);
	virtual ~LaxCairoImage();
	virtual cairo_surface_t *Image(int which=0);
	virtual void doneForNow();
	virtual unsigned int imagestate();
	virtual int imagetype() { return LAX_IMAGE_CAIRO; }
	virtual int w() { return width; }
	virtual int h() { return height; }
	virtual int dataw() { return dwidth; } //often smaller image used for preview purposes..
	virtual int datah() { return dheight; }
	virtual void clear();

	virtual int createFromData_ARGB8(int width, int height, int stride, const unsigned char *data);
	virtual unsigned char *getImageBuffer();
	virtual int doneWithBuffer(unsigned char *buffer);
};

//----------------- LaxCairoImage utils

int laxcairo_image_type();

void laxcairo_image_out(LaxImage *image, aDrawable *win, int ulx, int uly);
void laxcairo_image_out_rotated(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury);
void laxcairo_image_out_skewed(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury, int llx, int lly);
void laxcairo_image_out_matrix(LaxImage *image, aDrawable *win, double *m);

LaxImage *load_cairo_image(const char *filename);
LaxImage *load_cairo_image_with_preview(const char *filename,const char *previewfile,int maxx,int maxy,char del);
int laxcairo_generate_preview(const char *original, const char *preview, const char *format,int maxw, int maxh, int fit);

LaxImage *image_from_buffer_cairo(unsigned char *buffer, int w, int h, int stride);
LaxImage *create_new_cairo_image(int w, int h);



} //namespace Laxkit

#endif //uses cairo
#endif

