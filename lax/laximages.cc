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

#include <png.h>
#include <cstdio>
#include <errno.h>
#include <unistd.h>

#include <lax/laximages.h>
#include <lax/strmanip.h>
#include <lax/vectors.h>
#include <lax/transformmath.h>


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
 *   ImageOutFunc        image_out        
 *   ImageOutRotatedFunc image_out_rotated
 *   ImageOutSkewedFunc  image_out_skewed 
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


/*! fname is file name, but the default base class only sets filename, it does no loading on its own.
 */
LaxImage::LaxImage(const char *fname)
{
	importer=NULL;

	filename=newstr(fname);
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
}

//--------------------------- LaxImage utils --------------------------------------
/*! \typedef int (*DefaultImageTypeFunc)()
 * \brief Returns what the default image format is.
 *
 * For the forseeable future, this will be LAX_IMAGE_IMLIB or LAX_IMAGE_CAIRO.
 */
DefaultImageTypeFunc default_image_type = NULL;


//------------------ image_out_* 

/*! \typedef void ImageOutFunc(LaxImage *image, aDrawable *win, int ulx, int uly)
 * \brief Simply oriented drawing of image to aDrawable.
 *
 * Draw an image with upper left corner at window coordinates (ulx,uly). Positive y is down.
 */
/*! \typedef void ImageOutRotatedFunc(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury)
 * \brief Drawing image to aDrawable with rotation and scaling.
 * 
 * Draw an image so that the upper left and right corners are at (ulx,uly) and (ulx+urx,uly+ury).
 */
/*! \typedef void ImageOutSkewedFunc(LaxImage *image, aDrawable *win, int ulx,int uly, int urx,int ury, int llx, int lly)
 * \brief Drawing image to aDrawable with rotation, scaling, and skewing.
 * 
 * Draw an image so that the upper left and right corners are at (ulx,uly) and (ulx+urx,uly+ury), and the
 * lower left corner is at (ulx+llx,uly+lly).
 */
/*! \typedef void (*ImageOutMatrixFunc)(LaxImage *image, aDrawable *win, double *m)
 * \brief Drawing an image to aDrawable with an affine matrix.
 */

//! The default image to window drawing function.
/*! \ingroup laximages
 */
ImageOutFunc        image_out         = NULL;

//! The default image to window drawing function with rotation and scaling.
/*! \ingroup laximages
 */
ImageOutRotatedFunc image_out_rotated = NULL;

//! The default image to window drawing function with skewing.
/*! \ingroup laximages
 */
ImageOutSkewedFunc  image_out_skewed  = NULL;

//! The default image to window drawing function with an affine matrix.
/*! \ingroup laximages
 */
ImageOutMatrixFunc  image_out_matrix  = NULL;



//------------------- load_image 

/*! \typedef LaxImage *(*ImageFromBufferFunc)(unsigned char *buffer, int w, int h)
 * \brief Create a LaxImage from ARGB 8 bit data.
 */
/*! \typedef LaxImage *LoadImageFunc(const char *filename)
 * \brief The type of function that loads a file to a LaxImage. Defines load_image().
 */
/*! \typedef LaxImage *(*LoadImageWithPreviewFunc)(const char *filename,const char *pfile,int maxx,int maxy, LaxImage **preview_ret)
 * \brief Loads an image, using a preview image.
 *
 */
/*! \typedef LaxImage *(*CreateNewImageFunc)(int w, int h)
 * \brief Type of function that creates a new image based on a certain width and height.
 */

//! The default image loading function.
/*! \ingroup laximages
 *
 * This must be initialized to some function. InitImlibBackend(), or InitCairoBackend() for instance.
 */
LoadImageFunc            load_image              = NULL;
LoadImageWithPreviewFunc load_image_with_preview = NULL;
ImageFromBufferFunc      image_from_buffer       = NULL;
CreateNewImageFunc       create_new_image        = NULL;



//------------------- save_image 

/*! \typedef int (*SaveImageFunc)(LaxImage *image, const char *filename, const char *format);
 *
 * Returns 0 for success, or nonzero for some kind of error, not saved.
 */

SaveImageFunc  save_image=NULL;


//---------------------- preview creation

/*! \typedef int (*GeneratePreviewImageFunc)(const char *original_file, const char *to_preview_file, int width, int height, int fit)
 * \brief Function pointer type to a preview image creator.
 *
 * This sort of function basically creates a copy of original into preview with the given
 * width and height. If fit!=0, then fit the image within a box that is width x height, but
 * has the same aspect as the original image.
 *
 * WARNING: this does no sanity checking on file names, and will force an overwrite.
 * It is the responsibility of the calling code to do those things, and to ensure
 * that the preview file path is writable.
 *
 * Returns 0 for preview created. Otherwise nonzero.
 */

/*! \brief The base preview creator.
 */
GeneratePreviewFunc generate_preview_image = NULL;




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
 *   imlib loader
 *   graphicsmagick loader
 *   cairo loader
 */


ImageLoader *ImageLoader::loaders = NULL;


ImageLoader::ImageLoader(const char *newname, int nformat)
  : name(newstr(newname)),
	format(nformat),
	next(NULL), prev(NULL)
{ }

ImageLoader::~ImageLoader()
{
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

/*! Static function to retrieve a particular loader.
 */
ImageLoader *ImageLoader::GetLoaderById(unsigned long id)
{
	if (!loaders) return NULL; 

	ImageLoader *loader=loaders;
	while (loader) {
		if (loader->object_id == id) return loader;
	}

	return NULL;
}

/*! Static function to retrieve a particular loader.
 */
ImageLoader *ImageLoader::GetLoaderByIndex(int which)
{
	if (!loaders) return NULL; 
	if (which<0) return NULL;

	ImageLoader *loader=loaders;
	while (which>0 && loader) { loader=loader->next; which--; }

	return loader;
}


/*! Static function to add loader to list of available loaders.
 *  This function takes possession of the loader. If it is later removed, loader->dec_count()
 * is called on it. If the loader cannot be installed for some reason, it's count is decremented also.
 *
 * Return 0 for success or nonzero for error. On error, dec_count() is still called on loader.
 */
int ImageLoader::AddLoader(ImageLoader *loader, int where)
{
	if (!loader) return 1;
	if (!loaders) { loaders=loader; return 0; }

	if (where<0) where=NumLoaders()+1;

	if (where==0) {
		loader->next=loaders;
		if (loaders) loaders->prev=loader;
		loaders=loader;
		return 0;
	}

	 //else add after some loader
	ImageLoader *l;
	for (l=loaders; l->next && where>0; l=l->next) where--;

	loader->next=l->next;
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
	ImageLoader *loader=GetLoaderByIndex(which);
	if (!loader) return 1;

	if (which==0) loaders=loaders->next;

	if (loader->prev) loader->prev->next = loader->next;
	if (loader->next) loader->next->prev = loader->prev;
	loader->next=loader->prev=NULL;
	loader->dec_count();

	return 0;
}

/*! Static function to flush all loaders, such as when the application is shutting down.
 */
int ImageLoader::FlushLoaders()
{
	if (loaders) {
		loaders->dec_count();
		loaders=NULL;
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



LaxImage *load_image_with_loaders(const char *file,
								  const char *previewfile, int maxw,int maxh, LaxImage **preview_ret,
								 int required_state,
								 int target_format,
								 int *actual_format,
								 bool ping_only)
{
	DBG cerr <<"load_image_with_loaders()..."<<endl;

	if (target_format<0) target_format = default_image_type();

	ImageLoader *loader = ImageLoader::GetLoaderByIndex(0);
	if (!loader) {
		DBG cerr <<"load_image_with_loaders() done with null"<<endl;
		return NULL;
	}

	LaxImage *image=NULL;
	while (loader) {
		image=loader->load_image(file,
								 previewfile,maxw,maxh,preview_ret,
								 required_state,
								 target_format,
								 actual_format,
								 ping_only);
		if (image) {
			if (actual_format) *actual_format = loader->format;
			DBG cerr <<"load_image_with_loaders() done"<<endl;
			return image;
		}
		loader=loader->next;
	}

	DBG cerr <<"load_image_with_loaders() done with null"<<endl;
	return NULL;
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


