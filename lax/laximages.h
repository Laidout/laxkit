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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_LAXIMAGES_H
#define _LAX_LAXIMAGES_H

#include <lax/anobject.h>
#include <sys/times.h>


namespace Laxkit {
	

class aDrawable;


//--------------------------- LaxImage --------------------------------------
enum LaxImageTypes {
	LAX_IMAGE_NULL = 0,
	LAX_IMAGE_DEFAULT,
	LAX_IMAGE_BUFFER, //not implemented
	LAX_IMAGE_XIMAGE, //not implemented
	LAX_IMAGE_PIXMAP, //not implemented
	LAX_IMAGE_IMLIB,
	LAX_IMAGE_CAIRO,
	LAX_IMAGE_ANTIGRAIN, //not implemented
	LAX_IMAGE_GRAPHICSMAGICK,
	LAX_IMAGE_GL, //not implemented
	LAX_IMAGE_ANIMATED, //not implemented
	LAX_IMAGE_FIRST_USER_TYPE=1000
};

enum LaxImageState {
	LAX_IMAGE_METRICS       = (1<<0),
	LAX_IMAGE_PREVIEW       = (1<<1),
	LAX_IMAGE_WHOLE         = (1<<2),
	LAX_IMAGE_HAS_FILE      = (1<<3)
};


class ImageLoader;

class LaxImage : public anObject
{
  public:
	ImageLoader *importer;
	anObject *importer_data;

	char *filename;
	int index;
	clock_t lastaccesstime;

	LaxImage(const char *fname);
	virtual ~LaxImage();
	virtual int imagetype()=0;
	virtual unsigned int imagestate()=0;
	virtual void doneForNow() {}
	virtual int w()=0;
	virtual int h()=0;
	virtual void clear()=0;

	virtual unsigned char *getImageBuffer() = 0;
	virtual int doneWithBuffer(unsigned char *buffer) = 0;
	virtual LaxImage *Crop(int x, int y, int width, int height, bool return_new) = 0;

	//virtual int MaxMemoryUsed();
	//virtual int dec_count();

	virtual void Set(double r, double g, double b, double a) = 0;
	virtual int Save(const char *tofile = nullptr, const char *format = nullptr) = 0; //format==null guess from extension
};


//--------------------------- LaxImage utils --------------------------------------

typedef int (*DefaultImageTypeFunc)();
extern DefaultImageTypeFunc default_image_type;



//---------------- File or Image preview creation

LaxImage *GeneratePreview(LaxImage *image, int width, int height, int fit);
int GeneratePreviewFile(LaxImage *image, const char *to_preview_file, const char *format, int width, int height, int fit, LaxImage **preview_ret=nullptr);
int GeneratePreviewFile(const char *original_file, const char *to_preview_file, const char *format, int width, int height, int fit,
						   LaxImage **main_ret=nullptr, LaxImage **preview_ret=nullptr);



//------------------------------- ImageLoader stuff -------------------------------------

class ImageLoader : public anObject
{
  protected:
	static ImageLoader *loaders;

  public:
	const char *name;
	int format;

	ImageLoader *next, *prev; 

	ImageLoader(const char *newname, int nformat);
	virtual ~ImageLoader();

	//-------------- static funcs
	static int NumLoaders();
	static ImageLoader *GetLoaderByIndex(int which);
	static ImageLoader *GetLoaderById(unsigned long id);
	static ImageLoader *GetLoaderByFormat(int format);
	static int AddLoader(ImageLoader *loader, int where);
	static int LoadLoader(const char *file, int where); //load dynamic module
	static int RemoveLoader(int which);
	static int FlushLoaders();

	static LaxImage *LoadImage(const char *file,
                               const char *previewfile, int maxw,int maxh, LaxImage **preview_ret,
                               int required_state,
                               int target_format,
                               int *actual_format,
                               bool ping_only,
                               int index);
	static LaxImage *LoadImage(const char *file);
	static int Ping(const char *file, int *width, int *height, long *filesize, int *subfiles); //return 0 for success. subfiles is number of "frames" in file
	static LaxImage *NewImage(int width, int height, int format = LAX_IMAGE_DEFAULT);
	static LaxImage *NewImageFromBuffer(unsigned char *data, int width, int height, int stride, int format = LAX_IMAGE_DEFAULT);


	//-------------- Per loader functions:
	int SetLoaderPriority(int where);

	virtual bool CanLoadFile(const char *file) = 0;
	virtual bool CanLoadFormat(const char *format) = 0; 
	virtual int PingFile(const char *file, int *width, int *height, long *filesize, int *subfiles) = 0; //return 0 for success. subfiles is number of "frames" in file
	virtual int LoadToMemory(LaxImage *img) = 0;


	 //return a LaxImage in target_format.
	 //If target_format==0, then return any format.
	virtual LaxImage *load_image(const char *filename, 
								 const char *previewfile, int maxw,int maxh, LaxImage **preview_ret,
								 int required_state, //any of metrics, or image data
								 int target_format,
								 int *actual_format,
								 bool ping_only,
								 int index) = 0;
	virtual LaxImage *CreateImage(int width, int height, int format = LAX_IMAGE_DEFAULT) = 0;
	virtual LaxImage *CreateImageFromBuffer(unsigned char *data, int width, int height, int stride, int format = LAX_IMAGE_DEFAULT) = 0;
};

//LaxImage *load_image_with_loaders(const char *file,
//								  const char *previewfile, int maxw,int maxh, LaxImage **preview_ret,
//								  int required_state,
//								  int target_format,
//								  int *actual_format,
//								  bool ping_only,
//								  int index);


} //namespace Laxkit

#endif

