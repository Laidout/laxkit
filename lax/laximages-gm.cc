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


#include <lax/laximages-gm.h>


#ifdef LAX_USES_GRAPHICSMAGICK

#include <cstdio>
#include <errno.h>

#include <lax/strmanip.h>
#include <lax/vectors.h>
#include <lax/transformmath.h>
#include <lax/anxapp.h>
#include <lax/laxutils.h>
#include <lax/fileutils.h>

#ifdef LAX_USES_CAIRO
#include <lax/laximages-cairo.h>
#endif


#include <iostream>
using namespace std;
#define DBG 





namespace Laxkit {


//------------------- LaxGMImage utils ---------------------------------



int lax_gm_image_type()
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

	has_image = NoImage;
	width = height = 0;
	pixel_cache =NULL;
}

/*! Image is pinged, not read.
 */
LaxGMImage::LaxGMImage(const char *fname, Magick::Image *img, int nindex)
	: LaxImage(fname)
{
	index = nindex;
	display_count = 0; 
	has_image = NoImage;
	width = height = 0;
	pixel_cache =NULL;

	if (img) image = *img;

	if (fname) {
		try {
			image.ping(fname);
			has_image = HasPing;
		} catch (Magick::Exception &error_ ) {
			DBG cerr <<"GraphicsMagick Error loading "<<fname<<endl;
		}
	}

	if (has_image != NoImage) {
		width  = image.baseColumns();
		height = image.baseRows();
	}
}

LaxGMImage::LaxGMImage(int w, int h)
  : LaxImage(nullptr)
{
	if (w < 0) w = 0;
	if (h < 0) h = 0;
	if (w == 0 || h == 0) w=h=0;

	display_count = 0; 
	has_image = NoImage;
	pixel_cache =NULL;
	width = w;
	height = h;

	if (width > 0 && height > 0) {
		char scratch[100];
		sprintf(scratch,"%dx%d", width, height);
		image.size(scratch);
		image.read("xc:transparent");
		has_image = HasData;
	}
}

/*! Create new LaxGMImage from an 8 bit BGRABGRA... buffer.
 * buffer is not deleted, calling code must dispose of it.
 */
LaxGMImage::LaxGMImage(unsigned char *buffer, int w, int h, int stride)
  : LaxImage(nullptr)
{
	if (w < 0) w = 0;
	if (h < 0) h = 0;
	if (w == 0 || h == 0) w=h=0;

	display_count = 0; 
	has_image = NoImage;
	pixel_cache =NULL;
	width = w;
	height = h;

	if (buffer != nullptr && width > 0 && height > 0) {
		char scratch[100];
		sprintf(scratch,"%dx%d", width,height);
		image.size(scratch);
		//image.read("xc:transparent");

		pixel_cache = image.getPixels(0,0,width,height);

		CopyBufferToPixels(buffer);
		has_image = HasData;
	}
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
	if (has_image != HasData) Image(NULL);
	if (has_image != HasData) return nullptr;

	DBG cerr <<" LaxGMImage::getImageBuffer() "<<w()<<" x "<<h()<<endl;

	image.modifyImage();
	// Set the image type to TrueColor DirectClass representation.
	image.type(Magick::TrueColorType);

	// Request pixel region with size 60x40, and top origin at 20x30
	if (pixel_cache == nullptr) pixel_cache = image.getPixels(0,0,width,height);

	// Set pixel at column 5, and row 10 in the pixel cache to red.
	//PixelPacket *pixel = pixel_cache+row*columns+column;
	//*pixel = Color("red");

	unsigned char *buffer = new unsigned char[width*height*4];

	int shift = QuantumDepth - 8;

	unsigned char *p = buffer;
	Magick::PixelPacket *pixel = pixel_cache;
	for (int y=0; y<height; y++) {
		for (int x=0; x<width; x++) {
			p[0] = pixel->blue >> shift;
			p[1] = pixel->green >> shift;
			p[2] = pixel->red >> shift;
			p[3] = 255 - (pixel->opacity >> shift);

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
	if (has_image == HasData) {

		//copy back buffer
		CopyBufferToPixels(bbuffer);
	}

	pixel_cache = nullptr;
	delete[] bbuffer;
	return 0;
}

/*! Assumes pixel_cache and width and height are set to expected things.
 */
void LaxGMImage::CopyBufferToPixels(unsigned char *buffer)
{
	//copy back buffer
	unsigned char *p = buffer;
	Magick::PixelPacket *pixel = pixel_cache;
	int shift = QuantumDepth - 8;

	if (shift == 0) {
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				pixel->blue	= p[0];
				pixel->green   = p[1];
				pixel->red	 = p[2];
				pixel->opacity = 255-p[3];

				p  += 4;
				pixel += 1;
			}
		}

	} else if (shift == 8) {
		unsigned int o;
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				pixel->blue	= (p[0] << 8) | p[0];
				pixel->green   = (p[1] << 8) | p[1];
				pixel->red	 = (p[2] << 8) | p[2];
				o = 255-p[3];
				pixel->opacity = (o << 8) | o;

				p  += 4;
				pixel += 1;
			}
		}

	} else if (shift == 24) {
		unsigned int o;
		for (int y=0; y<height; y++) {
			for (int x=0; x<width; x++) {
				pixel->blue	= (((p[0] << 8) | p[0]) << 8) | p[0];
				pixel->green   = (((p[1] << 8) | p[1]) << 8) | p[1];
				pixel->red	 = (((p[2] << 8) | p[2]) << 8) | p[2];
				o = 255-p[3];
				pixel->opacity = (((o << 8) | o) << 8) | o;

				p  += 4;
				pixel += 1;
			}
		}
	}

	image.syncPixels();
}


/*! Query file for width and height only. Does not actually load full image.
 * Return 0 for success, nonzero failure.
 */
int LaxGMImage::Ping(const char *file)
{
	if (!file) file = filename;
	if (!file) return 1;

	try {
		image.ping(file);
		width  = image.baseColumns();
		height = image.baseRows();
		if (file != filename) makestr(filename, file);
	} catch (Magick::Exception &error_ ) {
		DBG cerr <<"GraphicsMagick Error pinging "<<file<<endl;
		return 2;
	}
	return 1;
}

/*! Return 0 for success, nonzero failure.
 * If atindex >= 0, then use that as index of a subimage. If not available, default to 0.
 * If atindex <0, then use current index.
 */
int LaxGMImage::Load(const char *file, int atindex)
{
	if (!file) file = filename;
	if (!file) return 1;

	if (atindex >= 0) index = atindex;
	if (file != filename) makestr(filename, file);
	has_image = NoImage;
	return Image(nullptr);

//	try {
//		image.read(file);
//		width  = image.baseColumns();
//		height = image.baseRows();
//		has_image = HasData;
//	} catch (Magick::Exception &error_ ) {
//		has_image = NoImage;
//		DBG cerr <<"GraphicsMagick Error loading "<<file<<endl;
//		return 2;
//	}
	return 1;
}

/*! If format==null guess from extension.
 * Return 0 for success, nonzero failure.
 */
int LaxGMImage::Save(const char *tofile, const char *format)
{
	if (!tofile) tofile = filename;
	if (!tofile) return 1;

	if (format) image.magick(format);
	image.write(tofile);

	return 0;
}

void LaxGMImage::Set(double r, double g, double b, double a)
{
	if (!has_image) return;

	//image.fillColor(ColorRGB(r,g,b));
	DBG cerr << " *** NEED TO IMPLEMENT LaxGMImage::Set"<<endl;
}

void LaxGMImage::clear()
{	
	 //free any image data
	if (filename) { delete[] filename; filename=NULL; }
	width = height = 0;

	has_image = NoImage;
	image = Magick::Image();
}

unsigned int LaxGMImage::imagestate()
{
	return (filename ? LAX_IMAGE_HAS_FILE : 0) |
		   (width>0 ? LAX_IMAGE_METRICS : 0) |
		   (has_image==HasData ? LAX_IMAGE_WHOLE : 0);
}


/*! Calling this is supposed to make it easier on the memory cache, by allowing
 * other code to free the gm_surface from memory. Calling Image() will place
 * it back in memory.
 */
void LaxGMImage::doneForNow()
{
	if (has_image == NoImage || flag) return;

	if (display_count>0) display_count--;
}

/*! Return true on success.
 */
bool LoadFrame(const char *filename, int index, Magick::Image *image_ret)
{
	if (image_ret) *image_ret = nullptr;
	Magick::Image gif;
	try {
		gif.read(filename);
	} catch (Magick::Exception &error) {
		DBG cerr << "Error reading multiframe image in "<<filename<<": "<<error.what() <<endl;
		return false;
	}

	//int delay     = gif.animationDelay(); // hundredths of seconds per frame
	int disposal  = gif.gifDisposeMethod();
	//int width     = gif.baseColumns();
    //int height    = gif.baseRows();
	Magick::Color bg  = gif.backgroundColor();

	try {
		list<Magick::Image> imagelist;
		Magick::readImages( &imagelist, filename );
		int numframes = imagelist.size();
		if (numframes == 0) return false;
		if (index < 0 || index >= numframes) index = 0;

		Magick::Image image;
			//stack frames up if necessary..
		int c=0;
		for (list<Magick::Image>::iterator gmimg = imagelist.begin(); gmimg != imagelist.end(); gmimg++) {
			if (disposal == 1) {
				if (c == 0) image = *gmimg;
				else image.composite(*gmimg, 0,0, Magick::OverCompositeOp);
			} else {
				if (c == index) image = *gmimg;
			}
			c++;
			if (c>index) break;
		}

		if (image_ret) *image_ret = image;

	} catch (Magick::Exception &error) {
		DBG cerr << "Error reading multiframe image in "<<filename<<": "<<error.what() <<endl;
		return false;
	}

	return true;
}

/*! Return the Image in img_ret. Loads from filename if !image.
 * Returns true if data exists, else false if we have no pixel data.
 */
bool LaxGMImage::Image(Magick::Image *img_ret)
{
	if (has_image == HasData) {
		if (img_ret) *img_ret = image;
		return true;
	}
	if (!filename) return false;

	try {
		if (index > 0) {
			if (LoadFrame(filename, index, &image))
				has_image = HasData;
		} else {
			image.read(filename);
			has_image = HasData;
		}
	} catch (Magick::Exception &error_ ) {
		DBG cerr <<"GraphicsMagick Error loading "<<filename<<endl;
		return false;
	}

	display_count++;
	if (img_ret) *img_ret = image;
	return true;
}


//--------------------------- GraphicsMagickLoader --------------------------------------
/*! \class GraphicsMagickLoader
 *
 * Loads image via GraphicsMagick2.
 */

GraphicsMagickLoader::GraphicsMagickLoader()
  : ImageLoader("GraphicsMagick", LAX_IMAGE_GRAPHICSMAGICK)
{}

GraphicsMagickLoader::~GraphicsMagickLoader()
{}


bool GraphicsMagickLoader::CanLoadFile(const char *file)
{
	try {
		Magick::Image image;
		image.ping(file);
	} catch (Magick::Exception &error_ ) {
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
	} catch (Magick::Exception &error_ ) {
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
		 // *** NOTE!! this might read in all frames? not ping?
		list<Magick::Image> imagelist;
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
		 //nothing really to do
		dynamic_cast<LaxGMImage*>(img)->Image(NULL);
		return 0;
	}

#ifdef LAX_USES_CAIRO
	if (dynamic_cast<LaxCairoImage*>(img)) {
		LaxCairoImage *cimg = dynamic_cast<LaxCairoImage*>(img);

		Magick::Image image;
		try {
			if (img->index == 0) image.read(cimg->filename);
			else {
				LoadFrame(cimg->filename, cimg->index, &image);
			}

			image.type(Magick::TrueColorType);

			// Request pixel region with size 60x40, and top origin at 20x30
			int width = img->w();
			int height = img->h();
			const Magick::PixelPacket *pixel_cache = image.getConstPixels(0,0,width,height);

			unsigned char buffer[width*height*4];

			int shift = QuantumDepth - 8;

			unsigned char *p = buffer;
			const Magick::PixelPacket *pixel = pixel_cache;
			for (int y=0; y<height; y++) {
				for (int x=0; x<width; x++) {
					p[0] = pixel->blue >> shift;
					p[1] = pixel->green >> shift;
					p[2] = pixel->red >> shift;
					//p[3] = (pixel->opacity >> shift);
					p[3] = 255 - (pixel->opacity >> shift);

					p  += 4;
					pixel += 1;
				}
			}

			cimg->createFromData_ARGB8(width, height, width*4, buffer, true);
		} catch (Magick::Exception &error_ ) {
			return 1;
		}
		return 0;
	}
#endif

	return 2;
}


#ifdef LAX_USES_CAIRO
/*! Note, does not set importer field.
 */
LaxCairoImage *MakeCairoFromGraphicsMagick(LaxGMImage *iimg, bool ping_only)
{
	LaxCairoImage *cimage = new LaxCairoImage();
	makestr(cimage->filename, iimg->filename);
	cimage->index =  iimg->index;

	if (ping_only) {
		 //only set dimensions. The importer will have to install actual image on call later
		cimage->width =iimg->w();
		cimage->height=iimg->h();

	} else {
		 //copy over buffer from GraphicsMagick image
		unsigned char *data = iimg->getImageBuffer();
		cimage->createFromData_ARGB8(iimg->w(), iimg->h(), 4*iimg->w(), data, true);
		iimg->doneWithBuffer(data);
	}

	return cimage;
}
#endif

LaxGMImage *load_gm_image_with_preview(const char *filename, const char *previewfile, int maxx, int maxy, LaxImage **previewimage_ret, int index)
{
	Magick::Image image;
	try {
		image.ping(filename);
	} catch (Magick::Exception &error_ ) {
		DBG cerr <<"GraphicsMagick Error loading "<<filename<<endl;
		return NULL;
	}

	LaxGMImage *img = new LaxGMImage(filename, &image, index);
	img->doneForNow();

	if (previewimage_ret) {
	  if (!isblank(previewfile) && previewimage_ret) {
		 //this will create previewfile on disk if it doesn't already exist:
		LaxImage *pimg = NULL;
		GeneratePreviewFile(img, previewfile, "png", maxx, maxy, 1, &pimg);
		if (pimg) pimg->doneForNow();

		*previewimage_ret = dynamic_cast<LaxImage*>(pimg);

	  } else {
		*previewimage_ret = NULL;
	  }
	}

	return img;
}

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
	LaxGMImage *iimg = dynamic_cast<LaxGMImage*>(load_gm_image_with_preview(filename, previewfile, maxx,maxy, previewimage_ret, index));
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
			iimg = dynamic_cast<LaxGMImage*>(*previewimage_ret);
			
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
	return dynamic_cast<LaxImage*>(new LaxGMImage(width, height));
}

LaxImage *GraphicsMagickLoader::CreateImageFromBuffer(unsigned char *data, int width, int height, int stride, int format)
{
	if (format != LAX_IMAGE_GRAPHICSMAGICK) return NULL;
	return dynamic_cast<LaxImage*>(new LaxGMImage(data, width, height, stride));
}


} //namespace Laxkit





#else
//-------------------------if no gm support:-------------------------------------

#include <lax/laximages.h> 

namespace Laxkit {


int lax_gm_image_type()
{ return LAX_IMAGE_GRAPHICSMAGICK; }


} //namespace Laxkit

#endif //LAX_USES_GRAPHICSMAGICK

