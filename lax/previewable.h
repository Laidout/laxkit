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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_PREVIEWABLE_H
#define _LAX_PREVIEWABLE_H


#include <lax/laximages.h>
#include <lax/doublebbox.h>

#include <ctime>

namespace Laxkit {

//--------------------------------- Previewable ----------------------------

class Previewable : virtual public DoubleBBox
{
  public:
	Previewable();
	virtual ~Previewable();

	LaxImage *preview;
	std::time_t previewtime;
    std::time_t modtime;     

	virtual void touchContents();
	virtual LaxImage *GetPreview();
	virtual int GeneratePreview(int width, int height);

	virtual int renderToBufferImage(LaxImage *image) = 0;
	virtual int maxPreviewSize() = 0;
};


} //namespace Laxkit


#endif

