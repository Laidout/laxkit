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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_BITMAPUTILS_H
#define _LAX_BITMAPUTILS_H



#include <lax/anobject.h>
#include <lax/doublebbox.h>


#include <iostream>
using namespace std;
#define DBG 


//
// ***
// Ultimately, this stuff here should be encapsulated in some kind of object,
// so that it becomes very easy to swap out in favor of methods that are more
// efficient than the default ones.. 
// An object is better than needing to redefine a ton of otherwise unconnected
// function pointers.
// ***
//

namespace Laxkit {


void MakeValueMap(unsigned char *img, int mapwidth, int mapheight, int blur, const DoubleBBox &bounds, flatpoint *points, int numpoints, bool flipy);
int GaussianBlur(int radius, char which, unsigned char *img, int orig_width, int orig_height,
					unsigned char *blurred, bool expand, int depth, int numchannels, int channel_mask );

//---------------------------- ImageProcessor --------------------------------------
class ImageProcessor : public anObject
{
  public:
	ImageProcessor() {}
	virtual ~ImageProcessor() {}

	virtual void MakeValueMap(unsigned char *img, int mapwidth, int mapheight, int blur, const DoubleBBox &bounds, flatpoint *points, int numpoints, bool flipy);
	virtual int GaussianBlur(int radius, char which, unsigned char *img, int orig_width, int orig_height,
					unsigned char *blurred, bool expand, int depth, int numchannels, int channel_mask );
};


ImageProcessor *GetDefaultImageProcessor(bool create_if_null=true);
void SetDefaultImageProcessor(ImageProcessor *new_processor);


} //namespace Laxkit

#endif

