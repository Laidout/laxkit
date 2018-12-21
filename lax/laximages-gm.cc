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




#include <cstdio>
#include <errno.h>

#include <lax/laximages-gm.h>
#include <lax/strmanip.h>
#include <lax/vectors.h>
#include <lax/transformmath.h>
#include <lax/anxapp.h>
#include <lax/laxutils.h>
#include <lax/fileutils.h>


#include <iostream>
using namespace std;
#define DBG 



#ifdef LAX_USES_GraphicsMagick




namespace Laxkit {


//------------------- LaxGMImage utils ---------------------------------



int laxgm_image_type()
{ return LAX_IMAGE_GRAPHICSMAGICK; }





//--------------------------- LaxGMImage --------------------------------------
/*! \class LaxGMImage
 * \brief A LaxImage based on GraphicsMagick Image objects.
 */
/*! \var int LaxGMImage::width
 * \brief The actual width of the full image.
 */
/*! \var int LaxGMImage::height
 * \brief The actual height of the full image.
 */


LaxGMImage::LaxGMImage()
  : LaxImage(NULL)
{ 
	display_count = 0;

	has_image = false;
	width = height = 0;
	pixel_packet =NULL;
}

/*! If fname and img, then assume that img corresponds to fname, read dimensions from img,
 * then free it.
 * If fname and !img, then get the dims from gm by loading fname and reading off dims.
 *
 * Note that GraphicsMagick docs say loading, reading off things, then freeing is a good thing.
 *
 * 
 * \todo *** needs to be some error checking when generating new previews
 * \todo scaling to maxx OR maxy if either 0 not implemented. both must currently be nonzero.
 * \todo when generating preview, might be wise to have check for freedesktop thumb locations to enforce
 *    proper sizing
 */
LaxGMImage::LaxGMImage(const char *fname, Magick::Image *img)
	: LaxImage(fname)
{
	display_count = 0; 
	has_image = false;
	width = height = 0;
	pixel_packet =NULL;

	if (!img) {
		if (fname) {
			try {
				image.read(fname);
				has_image = true;
			} catch (Exception &error_ ) {
				DBG cerr <<"GraphicsMagick Error loading "<<file<<endl;
			}
		}
	} else {
		has_image = true;
		image = *img;
	}

	if (has_image) {
		width  = image.baseColumns();
		height = image.baseRows();
	}
}

LaxImage::LaxImage(int width, int height)
{
}

LaxImage::LaxImage(unsigned char *buffer, int w, int h, int stride)
{
}

//! Free image if it happens to exist.
LaxGMImage::~LaxGMImage()
{
}

/*! MUST be followed up with call to doneWithBuffer().
 * The bytes are arranged BGRABGRABGRA...
 *
 * This returns a new uchar[] array.
 */
unsigned char *LaxGMImage::getImageBuffer()
{
	if (!has_image) Image();
	if (!has_image) return nullptr;

	DBG cerr <<" LaxGMImage::getImageBuffer() "<<w()<<" x "<<h()<<endl;

	image.modifyImage();
	// Set the image type to TrueColor DirectClass representation.
	image.type(TrueColorType);

	// Request pixel region with size 60x40, and top origin at 20x30
	if (pixel_cache == nullptr) pixel_cache = image.getPixels(0,0,width,height);

	// Set pixel at column 5, and row 10 in the pixel cache to red.
	//PixelPacket *pixel = pixel_cache+row*columns+column;
	//*pixel = Color("red");

	unsigned char *buffer = new unsigned char[width*height*4];

	int shift = QuantumDepth - 8;

	unsigned char *p = buffer;
	PixelPacket *pixel = pixel_cache;
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			p[0] = pixel.blue >> shift;
			p[1] = pixel.green >> shift;
			p[2] = pixel.red >> shift;
			p[3] = 256 - (pixel.opacity >> shift);

			p  += 4;
			pixel += 1;
		}
	}

	return buffer;
}

/*! Check in data that was just checked out with getImageBuffer().
 * bbuffer gets delete'd.
 *
 * Assumes 8 bit ARGB.
 */
int LaxGMImage::doneWithBuffer(unsigned char *bbuffer)
{
	if (has_image) {

		//copy back buffer
		unsigned char *p = buffer;
		PixelPacket *pixel = pixel_cache;
		int shift = QuantumDepth - 8;

		if (shift == 0) {
			for (int y=0; y<height; y++) {
				for (int x=0; x<width; x++) {
					pixel.blue    = p[0];
					pixel.green   = p[1];
					pixel.red     = p[2];
					pixel.opacity = 256-p[3];

					p  += 4;
					pixel += 1;
				}
			}

		} else if (shift == 8) {
			unsigned int o;
			for (int y=0; y<height; y++) {
				for (int x=0; x<width; x++) {
					pixel.blue    = (p[0] << 8) | p[0];
					pixel.green   = (p[1] << 8) | p[1];
					pixel.red     = (p[2] << 8) | p[2];
					o = 256-p[3];
					pixel.opacity = (o << 8) | o;

					p  += 4;
					pixel += 1;
				}
			}

		} else if (shift == 24) {
			unsigned int o;
			for (int y=0; y<height; y++) {
				for (int x=0; x<width; x++) {
					pixel.blue    = (((p[0] << 8) | p[0]) << 8) | p[0];
					pixel.green   = (((p[1] << 8) | p[1]) << 8) | p[1];
					pixel.red     = (((p[2] << 8) | p[2]) << 8) | p[2];
					o = 256-p[3];
					pixel.opacity = (((o << 8) | o) << 8) | o;

					p  += 4;
					pixel += 1;
				}
			}
		}

		image.syncPixels();
	}

	pixel_packet = nullptr;
	delete[] bbuffer;
	return 0;
}

//! Create a LaxGMImage from raw 8 bit data, arranged ARGB.
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
int LaxGMImage::createFromData_ARGB8(int nwidth, int nheight, int stride, const unsigned char *data)
{ ***
	if (!data) return 1;
	clear();

	image=gm_image_surface_create(GraphicsMagick_FORMAT_ARGB32, nwidth,nheight);
	width =nwidth;
	height=nheight;
	gm_t *cr=gm_create(image);

	gm_surface_t *image;
	image=gm_image_surface_create_for_data(const_cast<unsigned char*>(data),GraphicsMagick_FORMAT_ARGB32, nwidth,nheight, stride);
	//if (!image) return NULL;
	gm_set_source_surface(cr,image, 0,0);
	gm_paint(cr);
	gm_surface_destroy(image);
	gm_destroy(cr);



	return 0;
}

int LaxGMImage::Load(const char *file)
{
	***
}

int LaxGMImage::Save(const char *file)
{
	***
}

void LaxGMImage::clear()
{	
	 //free any image data
	if (filename) { delete[] filename; filename=NULL; }
	width = height = 0;

	has_image = false;
	image = Magick::Image();
}

unsigned int LaxGMImage::imagestate()
{
	return (filename ? LAX_IMAGE_HAS_FILE : 0) |
		   (width>0 ? LAX_IMAGE_METRICS : 0) |
		   (has_image ? LAX_IMAGE_WHOLE : 0);
}


/*! Calling this is supposed to make it easier on the memory cache, by allowing
 * other code to free the gm_surface from memory. Calling Image() will place
 * it back in memory.
 */
void LaxGMImage::doneForNow()
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
Magick::Image LaxGMImage::Image()
{
	display_count++;
	return image;
}


//--------------------------- GraphicsMagickLoader --------------------------------------
/*! \class GraphicsMagickLoader
 *
 * Loads image via GraphicsMagick2.
 */

GraphicsMagickLoader::GraphicsMagickLoader()
  : ImageLoader("GraphicsMagick", LAX_IMAGE_GraphicsMagick)
{}

GraphicsMagickLoader::~GraphicsMagickLoader()
{}


bool GraphicsMagickLoader::CanLoadFile(const char *file)
{
    try {
        image.read(file);
    } catch (Exception &error_ ) {
        DBG cerr <<"GraphicsMagick Error loading "<<file<<endl;
        return false;
    }
	return true;
}

bool GraphicsMagickLoader::CanLoadFormat(const char *format)
{
	return true; //hope for the best!
}

/*! Return 0 for successful ping, else nonzero.
 *
 * filesize returns size in bytes.
 */
int GraphicsMagickLoader::PingFile(const char *file, int *width, int *height, long *filesize, int *subfiles)
{
	Magick::Image img;
    try {
        img.ping(file);
    } catch (Exception &error_ ) {
        DBG cerr <<"GraphicsMagick Error loading "<<file<<endl;
        return 1;
    }

	if (width || height) {
		if (width)  *width  = img.baseColumns();
		if (height) *height = img.baseRows();
	}

	if (filesize) {
		//*filesize = LaxFiles::file_size(file, 1, NULL);
		*filesize = img.fileSize();
	}

    if (subfiles) {
         //find number of readable frames
        list<Magick::image> imagelist;
        Magick::readImages( &imagelist, file );
        *subfiles = imagelist.size();
    }

	return 0;
}

/*! Return 0 for success, nonzero for could not load to memory.
 */
int GraphicsMagickLoader::LoadToMemory(LaxImage *img)
{
	if (dynamic_cast<LaxGMImage*>(img)) {
		 //nothing really to do, GraphicsMagick does caching automatically
		dynamic_cast<LaxGMImage*>(img)->Image();
		return 0;
	}

	return 2;
}


#ifdef LAX_USES_CAIRO
/*! Note, does not set importer field.
 */
LaxCairoImage *MakeCairoFromGraphicsMagick(LaxGMImage *iimg, bool ping_only)
{
	LaxCairoImage *cimage = new LaxCairoImage();
	makestr(cimage->filename, iimg->filename);

	if (ping_only) {
		 //only set dimensions. The importer will have to install actual image on call later
		cimage->width =iimg->w();
		cimage->height=iimg->h();

	} else {
		 //copy over buffer from GraphicsMagick image
		unsigned char *data = iimg->getImageBuffer();
		cimage->createFromData_ARGB8(iimg->w(), iimg->h(), 4*iimg->w(), data);
		iimg->doneWithBuffer(data);
	}

	return cimage;
}
#endif


 //return a LaxImage in target_format.
 //If must_be_that_format and target_format cannot be created, then return NULL.
LaxImage *GraphicsMagickLoader::load_image(const char *filename, 
								 const char *previewfile, int maxx, int maxy, LaxImage **previewimage_ret,
								 int required_state, //any of metrics, or image data, or preview data
								 int target_format,
								 int *actual_format,
								 bool ping_only,
								 int index)
{
	LaxGMImage *iimg = dynamic_cast<LaxGMImage*>(load_gm_image_with_preview(filename, previewfile, maxx,maxy, previewimage_ret));
	if (!iimg) return NULL;
	
	if (target_format == 0 || target_format == LAX_IMAGE_GRAPHICSMAGICK) {
		 //GraphicsMagick loads by ping only anyway, so ok to ignore ping_only
		if (actual_format) *actual_format = LAX_IMAGE_GRAPHICSMAGICK;
		iimg->importer = this;
		iimg->importer->inc_count();
		return iimg;
	}


#ifdef LAX_USES_CAIRO
	if (target_format == LAX_IMAGE_CAIRO) {
		 //convert GraphicsMagick image to a cairo image
		LaxCairoImage *cimage = MakeCairoFromGraphicsMagick(iimg, ping_only);
		cimage->importer = this;
		cimage->importer->inc_count();
		makestr(cimage->filename, filename);

		iimg->dec_count();

		if (actual_format) *actual_format = LAX_IMAGE_CAIRO;

		if (previewimage_ret && *previewimage_ret) {
			 //convert preview image, if any, to cairo
			iimg = dynamic_cast<LaxGraphicsMagickImage*>(*previewimage_ret);
			
			LaxCairoImage *pimage = MakeCairoFromGraphicsMagick(iimg, true); //assume previews ok to load straight in
			iimg->dec_count();

			pimage->importer = this;
			pimage->importer->inc_count();
			makestr(pimage->filename, previewfile);
			*previewimage_ret = pimage; 
		}

		return cimage;
	} 
#endif //uses cairo

	return NULL;
}

LaxImage *GraphicsMagickLoader::CreateImage(int width, int height, int format)
{
	if (format != LAX_IMAGE_GRAPHICSMAGICK) return NULL;
	return LaxGMImage::NewImage(width, height);
}

LaxImage *GraphicsMagickLoader::CreateImageFromBuffer(unsigned char *data, int width, int height, int stride, int format)
{
	if (format != LAX_IMAGE_GRAPHICSMAGICK) return NULL;
	return LaxGMImage::NewImage(data, width, height, stride);
}


} //namespace Laxkit



#else
//-------------------------if no gm support:-------------------------------------



namespace Laxkit {



int lax_gm_image_type()
{ return LAX_IMAGE_GRAPHICSMAGICK; }





} //namespace Laxkit

#endif

