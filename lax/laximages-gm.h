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
//    Copyright (C) 2018 by Tom Lechner
//
#ifndef _LAX_LAXIMAGES_GRAPHICSMAGICK_H
#define _LAX_LAXIMAGES_GRAPHICSMAGICK_H


#include <lax/configured.h>


#ifdef LAX_USES_GRAPHICSMAGICK

#include <lax/laximages.h>

#include <GraphicsMagick/Magick++.h>


namespace Laxkit {



//--------------------------- LaxGMImage --------------------------------------
class LaxGMImage : public LaxImage
{
 protected:
	char flag;
	int display_count;

 public:
	static int GeneratePreview();
	static int GMImageType() { return LAX_IMAGE_GRAPHICSMAGICK; }
	static LaxImage *LoadImage(const char *file_name);

	bool has_image;
	Magick::Image image;
	Magick::PixelPacket *pixel_packet;
	int width,height;

	LaxGMImage();
	LaxGMImage(const char *fname, Magick::Image img);
	LaxGMImage(const char *original, const char *fname, int maxw, int maxh);
	LaxImage(int width, int height);
	LaxImage(unsigned char *buffer, int w, int h, int stride);
	virtual ~LaxGMImage();

	virtual Magick::Image Image();
	virtual void doneForNow();
	virtual unsigned int imagestate();
	virtual int imagetype() { return LAX_IMAGE_GRAPHICSMAGICK; }
	virtual int w() { return width; }
	virtual int h() { return height; }
	virtual void clear();

	virtual int createFromData_ARGB8(int width, int height, int stride, const unsigned char *data);
	virtual unsigned char *getImageBuffer();
	virtual int doneWithBuffer(unsigned char *buffer);

	virtual int Load(const char *file);
	virtual int Save(const char *file=NULL);
};


//----------------- LaxGMImage utils

int lax_gm_image_type();



//--------------------------- GraphicsMagickLoader --------------------------------------
class GraphicsMagickLoader : public ImageLoader
{
  protected:

  public:
	GraphicsMagickLoader();
	virtual ~GraphicsMagickLoader();

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

#endif //LAX_USES_GRAPHICSMAGICK
#endif //_LAX_LAXIMAGES_GRAPHICSMAGICK_H

