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
//    Copyright (C) 2015 by Tom Lechner
//

#include <lax/bitmaputils.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//---------------------------- ImageProcessor --------------------------------------
/*! class ImageProcessor
 *
 * Basically a container to hold various common useful image processing utilities.
 *
 * Get a default one with GetDefaultImageProcessor() and 
 * set a new default one with SetDefaultImageProcessor(ImageProcessor *new_processor).
 */

void ImageProcessor::MakeValueMap(unsigned char *img, int mapwidth, int mapheight, int blur, const DoubleBBox &bounds, flatpoint *points, int numpoints, bool flipy)
{
	return Laxkit::MakeValueMap(img,mapwidth,mapheight,blur,bounds,points,numpoints,flipy);
}

int ImageProcessor::GaussianBlur(int radius, char which, unsigned char *img, int orig_width, int orig_height,
					unsigned char *blurred, bool expand, int depth, int numchannels, int channel_mask )
{
	return Laxkit::GaussianBlur(radius,which,img,orig_width,orig_height, blurred,expand,depth,numchannels,channel_mask);
}




//--------------------------- default ImageProcessor

ImageProcessor *ImageProcessor::default_processor=NULL;


/*! This will always return non-null
 */
ImageProcessor *ImageProcessor::GetDefault(bool create_if_null)
{
	if (!default_processor && create_if_null) {
		default_processor = new ImageProcessor;
	}

	return default_processor;
}

/*! If you pass in NULL, it will dec_count the old one.
 * The count on new_processor will be incremented.
 */
void ImageProcessor::SetDefault(ImageProcessor *new_processor)
{
	if (new_processor==default_processor) return;

	if (default_processor) default_processor->dec_count();
	default_processor = new_processor;
	if (default_processor) default_processor->inc_count();
}






//---------------------------- default functions --------------------------------------

/*! From a collection of points that have values, create a one channel, 8 bit value map that is an approximation of the
 * spread out point values.
 *
 * This is quick and dirty and not terribly accurate at the pixel level. It will be faster for high point numbers, and
 * obnoxiously slow for low point numbers.
 *
 * This is done by drawing the initial points on a pixmap, and spreading those values
 * one pixel at a time outward, with the result being a kind of voronoi pattern.
 * This pattern is then blurred by blur pixels vertically and horizontally.
 */
void MakeValueMap(unsigned char *img, int mapwidth, int mapheight, int blur, const DoubleBBox &bounds, flatpoint *points, int numpoints, bool flipy)
{
	 //initialize
	unsigned char map1[mapwidth*mapheight];
	unsigned char map2[mapwidth*mapheight]; //used as status bits
	memset(map1, 0, mapwidth*mapheight);
	memset(map2, 0, mapwidth*mapheight);

	int v, i;
	int x,y;
	for (int c=0; c<numpoints; c++) {
		x=(points[c].x-bounds.minx)/(bounds.maxx-bounds.minx)*mapwidth;
		if (flipy) y=(bounds.maxy-points[c].y)/(bounds.maxy-bounds.miny)*mapheight;
		else y=(points[c].y-bounds.miny)/(bounds.maxy-bounds.miny)*mapheight;

		v=points[c].info+1;
		if (v>255) v=255; else if (v<0) v=0;
		
		if (x<0 || x>=mapwidth || y<0 || y>=mapheight) continue; 

		i=x + y*mapwidth;
		map1[i] = v;
		map2[i] = v;
	}


	 //now grow. 
	 //From top to bottom, for any blank pixel with a filled neighbor, fill that pixel with average
	 //of the filled neighbors' colors. Repeat until there are no blank pixels left!
	int startline=0, lastline=mapheight; //used to not check regions already filled
	unsigned char *m1=map1, *m2=map2;
	int firstzero=0, lastzero=1, nfilled;
	int stillzero=0, firstnonzero=mapwidth;
	int ii, sum;
	//int curstep=2;

	DBG int numiter=0;
	DBG int numfilled=0;


	while (!stillzero) {
		DBG numiter++;
		DBG if (numiter>40) break;

		firstzero=mapheight*mapwidth;
		lastzero=0;

		DBG numfilled=0;

		stillzero=1;
		for (int y=startline; y<lastline && y<mapheight; y++) {
			i=y*mapwidth;

			for (int x=0; x<mapwidth; x++, i++) {
				if (m1[i]!=0) {
					DBG numfilled++;
					m2[i]=m1[i];
					continue; //pixel filled, go to next one
				}

				if (stillzero) {
					stillzero=0;
					firstnonzero=i;
				}

				 //check for filled neighbors, average from any filled
				 //If no filled neighbors, go to next pixel
				nfilled=0;
				ii=i-1-mapwidth;
				sum=0;

				for (int yy=y-1; yy<y+2; yy++, ii+=mapwidth-3) {
					if (yy<0 || yy>=mapheight) { ii+=3; continue; }

					for (int xx=x-1; xx<x+2; xx++, ii++) {
						if (xx<0 || xx>=mapwidth || (xx==x && yy==y)) continue;

						if (m1[ii]==0) continue; //blank pixel, skip
						sum+=m1[ii];
						nfilled++;
					}
				}


				if (nfilled==0) {
					if (i<firstzero) firstzero=i;
					if (i>lastzero)  lastzero=i;
				} else {
					m2[i]=sum/nfilled;

					DBG numfilled++;
				}
				
			}
		}

		if (m1==map1) { m1=map2; m2=map1; }
		else { m1=map1; m2=map2; }

		startline=firstnonzero/mapwidth;
		//lastline=lastzero/mapwidth+1;

	}


	 //now blur
	if (blur>0) {
		GaussianBlur(blur,'x', m2,mapwidth,mapheight, m1, false, 8, 1,1);
		GaussianBlur(blur,'y', m1,mapwidth,mapheight, m2, false, 8, 1,1);
		m1=m2;

	} else m1=m2;

	 //we had values+1, so remove the +1, we need that 0...
	i=0;
	for (y=0; y<mapheight; y++) {
		for (x=0; x<mapheight; x++) {
			if (m1[i]>0) m1[i++]--;
		}
	}

	 //finally blit the correct data to img
	memcpy(img, m1, mapwidth*mapheight);


}




/*! Gaussian: 1/(2*pi*sigma^2) * exp(-(x^2 + y^2)/(2*sigma^2)
 * in one dim: 1/sqrt(2*pi*sigma^2) * exp(-x^2/2/sigma^2)
 * 
 * This is usually below 1 pixel when d=3*sigma.
 *
 * If expand, then the blurred image needs to be sized (orig_width + 2*xradius, orig_height + 2*yradius).
 * In this case, the boundary outside the original image is taken to be transparent black.
 * If !expand, blurred needs to be the same size as img.
 *
 * Return 0 for success, or nonzero for error (like bad inputs).
 */
int GaussianBlur(int radius, //!< Pixels to blur from to left and right of a given pixel. 0 for no blur on x
					char which, //!< 'x' or 'y'
					unsigned char *img    , int orig_width, int orig_height,
					unsigned char *blurred,
					bool expand, //!< true to have new image
					int depth,  //<8 or 16, per channel
					int numchannels, //!< Number of channels to be blurred independent of each other
					int channel_mask //!< Bit 0 is for channel 1, bit 1 for channel 2, etc. A mask of 0 means do all.
					)
{
	if (depth!=8) {
		cerr << " *** WARNING!!! GaussianBlur depth greater than 8 needs to be implemented!!!"<<endl;
	}

	if (!img || !blurred || orig_width<1 || orig_height<1) return 1;

	if (depth==16) depth=2;
	else depth=1;
	if (channel_mask==0) channel_mask=~0;
		
	int xradius = (which=='x' ? radius : 0);
	int yradius = (which=='y' ? radius : 0);

	int new_width  = orig_width  + (expand ? xradius*2 : 0);
	//int new_height = orig_height + (expand ? yradius*2 : 0);

	int xoff=0;
	int yoff=0;
	if (expand){
		xoff+=xradius;
		yoff+=yradius;
	}

	 //create the bell shaped blur kernel
	int n=radius*2+1;
	double kernel[n];
	double sigma=radius/3.;

	for (int c=0; c<=radius; c++) {
		kernel[radius-c-1] = kernel[radius+c] = 1/sqrt(2*M_PI*sigma*sigma) * exp(-c*c/2/sigma/sigma);
	}

	 //normalize the kernel, so all values add to 1.0
	double summ=0;
	for (int c=0; c<=2*radius; c++) {
		summ+=kernel[c];
	}
	if (summ!=1.0) for (int c=0; c<=2*radius; c++) {
		kernel[c]/=summ;
	}

	//int i;
	int v, ii, dopartial;
	double sum,tsum;
	int stride = orig_width*numchannels*depth;
	int pixwidth = numchannels*depth;


	 //blur horizontal
	if (xradius>0) { 
		for (int x=0; x<orig_width; x++) {
		  for (int y=0; y<orig_height; y++) {

			for (int ch=0; ch<numchannels; ch++) {
			  if ((channel_mask&(1<<ch))==0) continue;

				//i=y*stride + x*pixwidth; // *** <- this and below can be optimized

				sum=tsum=0;
				if (!expand && (x<radius || x>=orig_width-radius)) dopartial=1;
				else dopartial=0;
				for (int c=-radius; c<=radius; c++) {
					ii=x + c;

					if (ii>=0 && ii<orig_width) {
						v=img[y*stride + ii*pixwidth + ch*depth];
						sum  += v * kernel[c+radius];
						if (dopartial) tsum += kernel[c+radius]; //for when part of kernel is out of bounds
					} else { v=0; }


				}
				if (dopartial) blurred[((y+yoff)*new_width + (x+xoff))*pixwidth + ch*depth]=sum/tsum;
				else blurred[((y+yoff)*new_width + (x+xoff))*pixwidth + ch*depth]=sum;
			}
		  }
		}
	}

	if (yradius>0) { 
		for (int x=0; x<orig_width; x++) {
		  for (int y=0; y<orig_height; y++) {

			for (int ch=0; ch<numchannels; ch++) {
			  if ((channel_mask&(1<<ch))==0) continue;

				//i=y*stride + x*pixwidth; // *** <- this and below can be optimized

				sum=tsum=0;
				if (!expand && (y<radius || y>orig_height-radius)) dopartial=1;
				else dopartial=0;
				for (int c=-radius; c<=radius; c++) {
					ii=y + c;

					if (ii>=0 && ii<orig_height) {
						v=img[ii*stride + x*pixwidth + ch*depth];
						sum  += v * kernel[c+radius];
						if (dopartial) tsum += kernel[c+radius]; //for when part of kernel is out of bounds
					} else v=0;

				}
				if (dopartial) blurred[((y+yoff)*new_width + (x+xoff))*pixwidth + ch*depth]=sum/tsum;
				else blurred[((y+yoff)*new_width + (x+xoff))*pixwidth + ch*depth]=sum;
			}
		  }
		}
	}


	return 0;
}

// //------------------------------------ Skeleton Thinning ----------------------------

// /*! From 8 bit grayscale image, perform threshhold with 0 being on, 255 being off.
//  * Then shrink until single pixel widths.
//  * Assumes in and out are char[width * height].
//  *
//  * ```
//  *  for each pixel, examine vertically and horizontally:
//  *    all image edges go to 0
//  *    if black on all sides:
//  *      keep
//  *    else:
//  *      top if 0:
//  *        if y+2 white keep
//  *        if y+1 white keep
//  *        else loose
//  *      left if 0:
//  *        if x+2 white keep
//  *        if x+1 white keep
//  *        else loose
//  *      right if 0:
//  *        if x-1 white keep
//  *      bottom if 0:
//  *        if y-1 white keep
//  *      lose
//  * ```
//  */
// int MakeSkeleton(unsigned char *image_in, int width, int height,
// 				 unsigned char *image_out,
// 				 unsigned char threshhold)
// {
// 	int n = 0; //number of changed pixels

// 	unsigned char tmp[width * height];
// 	memcpy(tmp, image_in, width*height);

// #define SKIP = 255
// #define KEEP = 0

// 	//perform threshhold
// 	n = width * height;
// 	unsigned char *ptr = tmp;
// 	for (int ii=0; ii<n; ii++) {
// 		ptr[ii] = (image_in[ii] >= threshhold ? SKIP : KEEP);
// 	}

// 	unsigned char *cur  = tmp;
// 	unsigned char *next = image_out;

// 	//blank out edges
// 	int i = (height-1) * width;
// 	for (int c=0; c<width; c++) {
// 		next[c] = SKIP;
// 		next[i+c] = SKIP;
// 	}
// 	i = width;
// 	for (int c=1; c<height-1; c++) {
// 		next[i] = SKIP;
// 		next[i+width-1] = SKIP;
// 		i += width;
// 	}

// 	//iterate
// 	unsigned char *nextptr = next;
// 	unsigned char *top, *bottom, *bottom2, *left, *right, *right2;
// 	bool keep;
// 	do {
// 		n = 0;
// 		for (int y=1; y<height-1; y++) {
// 			ptr = cur + width * y + 1;
// 			nextptr = next + width * y + 1;

// 			for (int x=1; x<width-1; x++) {
// 				if (*ptr == SKIP) {
// 					*nextptr = SKIP;

// 				} else {
// 					keep = false;
// 					top = ptr - width;
// 					bottom = ptr + width;
// 					bottom2 = bottom + width;
// 					left = ptr - 1;
// 					right = ptr + 1;
// 					right2 = right + 1;

// 					//if black on all sides: keep
// 					if (*top == KEEP && *left == KEEP && *right == KEEP && *bottom == KEEP) {
// 						keep = true;
// 					} else {
// 						if (*top == SKIP) {
// 							if (*bottom2 == SKIP) keep = true;
// 							else if (*bottom == SKIP) keep = true;
// 							//else keep = false;
// 						}
// 						if (*left == SKIP) {
// 							if (*right2 == SKIP) keep = true;
// 							else if (*right == SKIP) keep = true;
// 							//else keep = false
// 						}
// 						if (*right == SKIP) {
// 							if (*left == SKIP) keep = true;
// 						}
// 						if (*bottom == SKIP) {
// 							if (*top == SKIP) keep = true;
// 						}
// 					}
// 					if (keep) {
// 						*nextptr = KEEP;
// 						n++;
// 					} else *nextptr = SKIP;
// 				}

// 				ptr++;
// 				nextptr++;
// 			}
// 		}

// 		//swap buffers
// 		ptr = cur;
// 		cur = next;
// 		next = ptr;
// 	} while (n);

// 	if (cur != image_out) {
// 		memcpy(image_out, cur, width*height);
// 	}
// }


} //namespace Laxkit

