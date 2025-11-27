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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_PREVIEWABLE_H
#define _LAX_PREVIEWABLE_H


#include <lax/laximages.h>
#include <lax/doublebbox.h>

#include <ctime>

namespace Laxkit {

//--------------------------------- Previewable ----------------------------

class Previewable : virtual public anObject, virtual public DoubleBBox
{
  public:
	Previewable();
	virtual ~Previewable();

	LaxImage *preview;
	std::clock_t previewtime;

	virtual void touchContents();
	virtual bool HasOldPreview() { return modtime > previewtime; }
	virtual bool CanRenderPreview() { return false; } // whether renderToBufferImage() should work
	virtual LaxImage *GetPreview();
	virtual int GeneratePreview(int width, int height);

	virtual int renderToBufferImage(LaxImage *image) = 0; //return 0 for success
	virtual int maxPreviewSize();
};


} //namespace Laxkit


#endif

