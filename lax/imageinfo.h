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
//   Copyright (c) 2007-2011 Tom Lechner
//
#ifndef _LAX_IMAGE_INFO_H
#define _LAX_IMAGE_INFO_H

#include <lax/anobject.h>
#include <lax/tagged.h>

namespace Laxkit {

//------------------------------ ImageInfo ------------------------------------
#define IMAGEINFO_FILE        (1<<0)
#define IMAGEINFO_PREVIEW     (1<<1)
#define IMAGEINFO_TITLE       (1<<2)
#define IMAGEINFO_DESCRIPTION (1<<3)
#define IMAGEINFO_TAGS        (1<<4)

class ImageInfo : virtual public anObject, virtual public Tagged
{
 public:
	char *filename;
	char *previewfile;
	char *title;
	char *description;
	int previewflags;
	unsigned int mask;
	ImageInfo *next;

	ImageInfo();
	ImageInfo(const char *f,const char *p,const char *t,const char *d,int pf);
	virtual ~ImageInfo();
	ImageInfo &operator=(ImageInfo &f);
	virtual int SetInfo(ImageInfo *f);
};

} //namespace Laxkit

#endif


