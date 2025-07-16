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
//   Copyright (c) 2007-2011 Tom Lechner
//

#include <lax/imageinfo.h>
#include <lax/strmanip.h>


namespace Laxkit {
	
//-------------------------------- ImageInfo ------------------------------

/*! \class ImageInfo
 * \brief Kind of a shell to hold various information about an image.
 *
 * Currently, stores:
 *  - file name
 *  - subimage index
 *  - preview file name
 *  - title
 *  - description
 *
 * \todo come to think of it, this could be mostly just a hash, which would
 *   make integration of exif info easier...
 */


ImageInfo::ImageInfo()
{
	filename = previewfile = title = description = nullptr;
	previewflags = 0;
	next         = nullptr;
	index        = 0;
}

ImageInfo::ImageInfo(const char *f,const char *p,const char *t,const char *d,int pf, int i)
{
	filename     = newstr(f);
	previewfile  = newstr(p);
	title        = newstr(t);
	description  = newstr(d);
	previewflags = pf;
	next         = nullptr;
	index        = i;
}

/*! Delete the strings and next.
 */
ImageInfo::~ImageInfo()
{
	delete[] title;
	delete[] description;
	delete[] filename;
	delete[] previewfile;

	if (next) delete next;
	next = nullptr;
}

ImageInfo &ImageInfo::operator=(ImageInfo &f)
{
	makestr(filename, f.filename);
	makestr(description, f.description);
	makestr(previewfile, f.previewfile);
	makestr(title, f.title);
	previewflags = f.previewflags;
	mask         = f.mask;
	index        = f.index;
	return f;
}

/*! Return 0 for success, nonzero failure to set.
 */
int ImageInfo::SetInfo(ImageInfo *f)
{
	*this = *f;
	return 0;
}


} //namespace Laxkit
	
