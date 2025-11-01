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

#include <png.h>
#include <cstdio>
#include <errno.h>
#include <unistd.h>

#include <lax/laximages.h>
#include <lax/strmanip.h>
#include <lax/vectors.h>
#include <lax/transformmath.h>
#include <lax/singletonkeeper.h>
#include <lax/laxutils.h>
#include <lax/freedesktop.h>


#include <iostream>
using namespace std;
#define DBG 



namespace Laxkit {

/*! \defgroup laximages Images
 *
 * The class LaxImage abstracts various image related tasks. No actual image data
 * is stored in a base LaxImage. 
 *
 * The default image type can be Imlib based image manipulation
 * using LaxImlibImage classes, for instance.
 *
 * The following function pointers can be redefined if you prefer to use your own
 * kind of LaxImage. They function like the Imlib equivalents.
 * \code
 *   LoadImageFunc       load_image       
 *   *** update me!!
 * \endcode
 *
 * LaxImage is derived from anObject, so code should use the dec_count() and
 * inc_count() member functions when using LaxImages, rather than <tt>delete someimage</tt>
 *
 * \todo In the future, will probably want some kind of load_image_to_buffer, but that means
 *   I have to understand color management much more thoroughly.
 *
 *   @{
 */
	



//----------------------------Memory Cache------------------------
/*! **********hmmmmm THIS IS A WORK IN PROGRESS!!!
 *
 * image data can be: 
 *    1. in memory and needed
 *    2. in memory and not needed,     needtoregenerate==0
 *    3. not in memory and needed.     needtoregenerate==1
 *    4. not in memory and not needed. needtoregenerate==1
 * doneForNow() will put it in 2 if it was in 1, and do nothing if it was in 3
 * getImageData() will put it in 1 from any other state
 *
 * when it is in 2, there needs to be background cache management to free
 * the data if memory is low and it has the oldest access time, putting it into 3.
 *
 * in states 1 and 3, it is not managed by any cache manager
 *
 * In order for cache manager to accept an object, is must have able_to_regenerate==1
 * 
 * \todo ***** think about this!! Imlib2 does memory caching already... how I wonder?
 */
class MemCachedObject
{
 friend class CacheManager;
 private:
	int incache;
	unsigned int cache_count;
	MemCachedObject *prev_cache, *next_cache;

 public:
	int cachegroup; //images | fonts | object groups...
	clock_t lastaccesstime;
	int id;

	MemCachedObject();
	virtual ~MemCachedObject();

	virtual int CacheMemoryNeeded()=0;
	virtual int AbleToRegenerate()=0;
	virtual int CacheRegenerate()=0;
	virtual int CacheFree()=0; //return 0 for success, or nonzero can't free, like for in memory buffer with no regenerate available
	virtual int CacheState()=0;

	virtual int inc_cache();
	virtual int dec_cache();
};

/*! Default is to start with the object data not being needed, so the count is 0.
 */
MemCachedObject::MemCachedObject()
{
	prev_cache=next_cache=NULL;

	cache_count=0;
	lastaccesstime=0;
}

MemCachedObject::~MemCachedObject()
{}

//! Increment the cache count, returning the new current count.
int MemCachedObject::inc_cache()
{
	return ++cache_count;
}

//! Decrement the cache count, returning the new current count.
/*! If the cache count is zero, nothing is done. When it is zero,
 * whatever internal memory is allocated can be freed or disk cached
 * if the memory is needed.
 */
int MemCachedObject::dec_cache()
{
	if (cache_count>0) cache_count--;
	return cache_count;
}


//--------------------------- CacheManager --------------------------------------
class CacheManager
{
  protected:
	MemCachedObject *top, *bottom; //each mod touch moves object to bottom

  public:
	static CacheManager *GetDefault();
	static CacheManager *SetDefault(CacheManager *newmanager);

	CacheManager();
	virtual ~CacheManager();
	virtual int Add(MemCachedObject *obj);
	virtual int Remove(MemCachedObject *obj);
};

CacheManager::CacheManager()
{
	top=bottom=NULL;
}

CacheManager::~CacheManager()
{
}

int CacheManager::Add(MemCachedObject *obj)
{
	// ***
	return 1;
}

int CacheManager::Remove(MemCachedObject *obj)
{
	// ***
	return 1;
}




//--------------------------- LaxImage --------------------------------------
/*! \class LaxImage
 * \brief Abstraction around images.
 *
 * The purpose of this class is to provide a relatively simple abstraction of images
 * to make simple image loading easy, such as for icons. There is also a mechanism to
 * allow a preview image to shadow the actual image. See previewfile for more.
 * 
 * If filename exists, but w()<=0, then the filename is invalid. Otherwise, the w() and
 * h() refer to the actual width and height of the image.
 *
 */
/*! var char *LaxImage::filename
 * \brief The file that the image is supposed to correspond to.
 *
 * If w()<=0, then filename is invalid.
 */
/*! \fn void LaxImage::doneForNow()
 * \brief This might free the image to make room in a cache, for instance.
 */
/*! \fn void LaxImage::clear()
 * \brief Clear all data contained in this image.
 *
 * Derived classes should remember to delete[] filename and preview.
 */
/*! \fn LaxImage::LaxImage(const char *fname)
 * Constructor should get metric info on the image from file fname.
 * It need not read in the actual pixel data.
 */
/*! \fn int LaxImage::imagetype()
 * Return what sort of image this is. See LaxImageTypes.
 */
/*! \fn int LaxImage::imagestate()
 * \brief Return whether this is valid image, is whole image, etc.
 *
 * If the object does not hold a valid object, then 0 should be returned.
 * Otherwise, the image data should be able to be gotten or already be in memory,
 * and the return value should be an or'd combination of the following:
 * \code
 *   LAX_IMAGE_METRICS        <-- whether metric info is ready
 *   LAX_IMAGE_PREVIEW        <-- whether (maybe) smaller preview is in memory
 *   LAX_IMAGE_WHOLE          <-- whether the whole data is in memory
 *   LAX_IMAGE_HAS_FILE       <-- whether the image corresponds to a file
 *   LAX_IMAGE_HAS_TEMP_FILE  <-- whether the image is held in a temporary file
 * \endcode
 */
/*! \fn int LaxImage::w()
 * \brief Return the width of the actual image.
 */
/*! \fn int LaxImage::h()
 * \brief Return the height of the actual image.
 */
/*! \fn unsigned char *LaxImage::getImageBuffer()
 * This is mainly to assist auto cached preview images for some interface data classes.
 * More specialized low level data manipulation should be done by accessing the particular
 * implementation of LaxImage.
 *
 * This checks out data of the actual image (in filename) not the previewfile, 
 * in 8 bit ARGB format, with the row stride==4*width.
 * The bytes are arranged BGRABGRABGRA...
 *
 * It MUST be checked back in with doneWithBuffer() if you make changes in it.
 */
/*! \fn int LaxImage *doneWithBuffer(unsigned char *buffer)
 *
 * This puts back data that was checked out with getImageBuffer().
 */
/*! \fn LaxImage *Crop(int x, int y, int width, int height, bool return_new)
 *
 * Make the image be the specified rectangle.
 * If return_new then return a new LaxImage. Else modify current image.
 * 
 * Return nullptr on error, such as width or height being non-positive.
 */


/*! fname is file name, but the default base class only sets filename, it does no loading on its own.
 */
LaxImage::LaxImage(const char *fname)
{
	importer = NULL;
	importer_data = NULL;

	filename=newstr(fname);
	index = 1; //for things like multipage rasterized like pdf, or gif frames
}

/*! If *** delpreview and previewfile, then unlink(previewfile). Note that this is hazardous,
 * and great care must be taken to ensure that previewfile is always what it is supposed to be.
 * The Laxkit will never directly set delpreview to 1, except if you tell it to, via 
 * load_image_with_preview().
 *
 * \todo *** perhaps unlink also only if dataw()>0? not sure if that is accessible in
 *   destructor, might have to intercept dec_count().
 */
LaxImage::~LaxImage()
{
//	if (delpreview && previewfile) {
//		DBG cerr <<" = = = Deleting preview \""<<previewfile<<"\" for \""<<(filename?filename:"(unknown)")<<"\""<<endl;
//		unlink(previewfile);
//	}
	if (filename) delete[] filename; 
	if (importer) importer->dec_count();
	if (importer_data) importer_data->dec_count();
}

/*! Pass null as value to remove the attribute.
 */
int LaxImage::SetAttribute(const char *key, const char *value)
{
	int i = -1;
	Attribute *a = attributes.find(key, &i);
	if (a) {
		if (!value) {
			a->remove(i);
			return 1;
		}
		makestr(a->value, value);
		return 1;
	}
	attributes.push(key, value);
	return 1;
}

/*! Returned value must be delete'd.
 */
char *LaxImage::GetAttribute(const char *key)
{
	return newstr(attributes.findValue(key));
}


//--------------------------- LaxImage utils --------------------------------------
/*! \typedef int (*DefaultImageTypeFunc)()
 * \brief Returns what the default image format is.
 *
 * For the forseeable future, this will be LAX_IMAGE_IMLIB or LAX_IMAGE_CAIRO.
 */
DefaultImageTypeFunc default_image_type = NULL;



//---------------------- preview creation


/*! Load in original_file, and Save a resized version of image to to_preview_file.
 * If fit, maintain aspect ratio to found bounds that fit within a box width x height.
 * Return 0 for success or nonzero for error and not saved.
 */
int GeneratePreviewFile(const char *original_file,
						   const char *to_preview_file, 
						   const char *format, 
						   int width, int height, int fit,
						   LaxImage **main_ret, LaxImage **preview_ret)
{
	LaxImage *image = ImageLoader::LoadImage(original_file,
								 nullptr, 0,0, nullptr,
								 LAX_IMAGE_WHOLE,
								 LAX_IMAGE_DEFAULT, //!< LaxImageTypes, for starters. 0 and LAX_IMAGE_DEFAULT both mean use default. Non zero is return NULL for can't.
								 NULL,
								 false, //!< If possible, do not actually load the pixel data, just things like width and height
								 0);
	if (!image) return 1;
	int status = GeneratePreviewFile(image, to_preview_file, format, width, height, fit);
	if (main_ret) *main_ret = image;
	else image->dec_count();
	return status;
}


/*! Save a resized version of image to to_preview_file.
 * If fit, maintain aspect ratio to found bounds that fit within a box width x height.
 * Return 0 for success or nonzero for error and not saved.
 */
int GeneratePreviewFile(LaxImage *image,
						   const char *to_preview_file, 
						   const char *format, 
						   int width, int height, int fit, LaxImage **preview_ret)
{
	if (!image || !to_preview_file || to_preview_file[0] == '\0') return 1;

	if (width <= 0) width = height;
	if (height <= 0) height = width;
	if (width == 0) {
		width = height = freedesktop_guess_thumb_size(to_preview_file);
		if (width == 0) width = height = 256;
	}

	LaxImage *preview = GeneratePreview(image, width, height, fit);
	if (!preview) return 2;

	preview->Save(to_preview_file, format);
	if (preview_ret) *preview_ret = preview;
	else preview->dec_count();

	return 0;
}

/*! Save a resized version of image to to_preview_file.
 * If fit, maintain aspect ratio to found bounds that fit within a box width x height.
 * Please note this uses GetDefaultDisplayer().
 */
LaxImage *GeneratePreview(LaxImage *image, int width, int height, int fit)
{
	if (!image) return NULL;

	int owidth  = image->w(),
	    oheight = image->h();
	
	if (fit) {
		 //figure out dimensions of new preview
		double a = double(oheight)/owidth;
		int ww = width, hh = height;
		if (a*ww > hh) {
			height = hh;
			width = int(hh/a);
		} else {
			width = ww;
			height = int(ww*a);
		}
		if (height==0) height=1;
		if (width ==0) width =1;
	}

	if (width<=0 || height<=0) return NULL;

	LaxImage *preview = ImageLoader::NewImage(width,height);
	Displayer *dp = GetDefaultDisplayer();
	dp->MakeCurrent(preview);
	dp->imageout(image, 0,0, width, height);
	dp->EndDrawing();

	return preview;
}


//------------------------------- ImageLoader stuff -------------------------------------

/*! \class ImageLoader
 * \brief Class to facilitate getting an image from the disk into memory suitable for the current graphics backend.
 *
 * ImageLoader objects simplify loading images from a wider variety of sources than is
 * normally available for any one particular graphics backend. For instance, cairo only
 * reads in png, but if we add Imlib or GraphicsMagick loaders, then the sky's the limit.
 *
 * NOTES ON OTHER TOOLKIT IMPLEMENTATIONS:
 *
 * gtk has module image loaders in lib directory. these are activated on call only.
 *
 * implement: 
 *   graphicsmagick loader
 *   cairo loader
 */


ImageLoader::ImageLoader(const char *newname, int nformat)
  : name(newstr(newname)),
	format(nformat),
	next(NULL), prev(NULL)
{ }

ImageLoader::~ImageLoader()
{
	DBG cerr << "ImageLoader destructor "<<name<<", object_id: "<<object_id<<endl;

	 //assumes prev is deleted separately
	if (next) {
		next->prev=NULL;
		next->dec_count();
	}

	delete[] name;
}

/*! Static function, return number of known, installed loaders.
 */
int ImageLoader::NumLoaders()
{
	int n=0;
	ImageLoader *loader=loaders;
	while (loader) {
		n++;
		loader=loader->next;
	}
	return n;
}

ImageLoader *ImageLoader::GetLoaderByFormat(int format)
{
	if (!loaders) return NULL; 

	ImageLoader *loader = loaders;
	while (loader) {
		if (loader->format == format) return loader;
		loader = loader->next;
	}

	return NULL;
}

/*! Static function to retrieve a particular loader.
 */
ImageLoader *ImageLoader::GetLoaderById(unsigned long id)
{
	if (!loaders) return NULL; 

	ImageLoader *loader=loaders;
	while (loader) {
		if (loader->object_id == id) return loader;
		loader = loader->next;
	}

	return NULL;
}

/*! Static function to retrieve a particular loader.
 */
ImageLoader *ImageLoader::GetLoaderByIndex(int which)
{
	if (!loaders) return NULL; 
	if (which<0) return NULL;

	ImageLoader *loader = loaders;
	while (which>0 && loader) { loader = loader->next; which--; }

	return loader;
}


ImageLoader *ImageLoader::loaders = NULL;
static SingletonKeeper loaderKeeper;


/*! Static function to add loader to list of available loaders.
 *  This function takes possession of the loader. If it is later removed, loader->dec_count()
 * is called on it. If the loader cannot be installed for some reason, it's count is decremented also.
 *
 * Return 0 for success or nonzero for error. On error, dec_count() is still called on loader.
 */
int ImageLoader::AddLoader(ImageLoader *loader, int where)
{
	if (!loader) return 1;
	if (!loaders) {
		loaders = loader;
		loaderKeeper.SetObject(loader, true);
		return 0;
	}

	if (where < 0) where = NumLoaders()+1;

	if (where == 0) {
		loader->next = loaders;
		if (loaders) loaders->prev = loader;
		loaders = loader;
		loaderKeeper.SetObject(loaders, true);
		return 0;
	}

	 //else add after some loader
	ImageLoader *l;
	for (l = loaders; l->next && where>0; l=l->next) where--;

	loader->next = l->next;
	if (l->next) l->next->prev=loader;

	loader->prev=l;
	l->next=loader;

	return 0;
}

/*! Static function to remove a loader.
 * Return 0 for success, 1 for not found.
 */
int ImageLoader::RemoveLoader(int which)
{
	ImageLoader *loader = GetLoaderByIndex(which);
	if (!loader) return 1;

	if (which==0) loaders=loaders->next;

	if (loader->prev) loader->prev->next = loader->next;
	if (loader->next) loader->next->prev = loader->prev;
	loader->next = loader->prev = nullptr;
	if (loaders == nullptr) loaderKeeper.SetObject(nullptr, false);
	else loader->dec_count();

	return 0;
}

/*! Static function to flush all loaders, such as when the application is shutting down.
 */
int ImageLoader::FlushLoaders()
{
	if (loaders) {
		loaderKeeper.SetObject(nullptr, false);
		loaders = nullptr;
	}
	return 0;
}


/*! Static function to load ImageLoader dynamic module.
 *
 * Return 0 for success, or nonzero for could not load.
 */
int ImageLoader::LoadLoader(const char *file, int where)
{
	cerr << " *** need to implement ImageLoader::LoadLoader()"<<endl;
	return 1;
}

int ImageLoader::SetLoaderPriority(int where)
{
	cerr << " *** need to implement ImageLoader::SetLoaderPriority()"<<endl;
	return 1;
}


/*! Try to load file into the default image format.
 */
LaxImage *ImageLoader::LoadImage(const char *file)
{
	return LoadImage(file, NULL,0,0,NULL, 0, LAX_IMAGE_DEFAULT, NULL, true, 0);
}

LaxImage *ImageLoader::LoadImage(const char *file,
								 const char *previewfile, int maxw,int maxh, LaxImage **preview_ret,
								 int required_state, //!< Or'd combo of LaxImageState
								 int target_format, //!< LaxImageTypes, for starters. 0 means use default. Non zero is return NULL for can't.
								 int *actual_format,
								 bool ping_only, //!< If possible, do not actually load the pixel data, just things like width and height
								 int index)
{
	if (!file) return NULL;

	DBG cerr <<"ImageLoader::LoadImage()..."<<file<<endl;

	if (target_format<=0 || target_format == LAX_IMAGE_DEFAULT) target_format = default_image_type();

	ImageLoader *loader = ImageLoader::GetLoaderByIndex(0);
	if (!loader) {
		DBG cerr <<"ImageLoader::LoadImage() no loaders!"<<endl;
		return NULL;
	}

	LaxImage *image=NULL;
	while (loader) {
		image = loader->load_image(file,
								 previewfile,maxw,maxh,preview_ret,
								 required_state,
								 target_format,
								 actual_format,
								 ping_only,
								 index);
		if (image) {
			if (actual_format) *actual_format = loader->format;
			DBG cerr <<"load_image_with_loaders() done"<<endl;
			return image;
		}
		loader = loader->next;
	}

	DBG cerr <<"ImageLoader::LoadImage() couldn't load "<<file<<endl;
	return NULL;
}

/*! Ping an image file without fully loading it or instantiating a LaxImage.
 * Return 0 for success, or nonzero for can't ping. subfiles is number of "frames" in file.
 * This runs through the loader list until loader->PingFile() returns 0.
 */
int ImageLoader::Ping(const char *file, int *width, int *height, long *filesize, int *subfiles)
{
	ImageLoader *loader = ImageLoader::GetLoaderByIndex(0);
	if (!loader) {
		DBG cerr <<"ImageLoader::Ping() no loaders!"<<endl;
		return 1;
	}

	while (loader) {
		if (loader->PingFile(file, width, height, filesize, subfiles) == 0)
			return 0;
		loader = loader->next;
	}

	return 2;
}

LaxImage *ImageLoader::NewImage(int width, int height, int format)
{
	if (format == LAX_IMAGE_DEFAULT) format = default_image_type();
	ImageLoader *loader = GetLoaderByFormat(format);
	if (!loader) return NULL;
	return loader->CreateImage(width, height, format);
}

LaxImage *ImageLoader::NewImageFromBuffer(unsigned char *data, int width, int height, int stride, int format)
{
	if (format == LAX_IMAGE_DEFAULT) format = default_image_type();
	ImageLoader *loader = GetLoaderByFormat(format);
	if (!loader) return NULL;
	return loader->CreateImageFromBuffer(data, width, height, stride, format);
}



//---------------------------------------------------------------------------------------



//--------------------------- LaxBufferImage --------------------------------------

/* \class LaxBufferImage
 * \brief A fallback to only buffer data, no X component at all.
 */
class LaxBufferImage : public LaxImage
{
 protected:
	int color_system_id;
	//cmsHPROFILE iccprofile; //at some point this will be used, from lcms
	char *iccprofile;
	int num_channels;
	int alpha_channel; //index where alpha channel can be found (will be >=0 and <num_channels or -1)
	int bits_per_channel; //8 or 16, others not supported
	int stride;
	int width, height;

	unsigned char *pixel_data;

	anObject *config;

 public:
	LaxBufferImage();
	virtual ~LaxBufferImage();
	//virtual LaxBufferImage *Image();
	virtual int imagetype() { return LAX_IMAGE_BUFFER; }

	virtual int Stride();
	virtual int HasAlpha() { return alpha_channel>=0; }
	virtual int BitsPerChannel() { return bits_per_channel; }
};

LaxBufferImage::LaxBufferImage()
	: LaxImage(NULL)
{
	num_channels=bits_per_channel=stride=0;
	alpha_channel=-1;
	iccprofile=0;
	pixel_data=NULL;
	config = NULL;
}

LaxBufferImage::~LaxBufferImage()
{
	if (config) config->dec_count();
}

//! The number of bytes between rows.
/*! So say an image is 40 pixels wide, 16 bits per channel, and has 4 channels, and
 * has no padding. Then the stride is 40*(16/8)*4.
 */
int LaxBufferImage::Stride()
{
	//***
	return stride;
}


/*! @} */



} //namespace Laxkit


