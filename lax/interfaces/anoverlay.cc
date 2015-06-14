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
//    Copyright (C) 2012 by Tom Lechner
//


#include <lax/interfaces/aninterface.h>
#include <lax/rectangles.h>


namespace LaxInterfaces {


enum OverlayOptions {
	OVERLAYOPT_RESIZABLE = (1<<0),
	OVERLAYOPT_FIXEDPOS  = (1<<1),

	OVERLAYOPT_MAX
}

// int rect is a bounds hint, the actual shape of the overlay may be different pieces and strange shapes

class anOverlay : public anInterface
{
  protected:
	IntRectangle bounds;
  public:
	anOverlay();
	virtual ~anOverlay();

	virtual int X() { return bounds.x; }
	virtual int Y() { return bounds.y; }
	virtual int Width() { return bounds.width; }
	virtual int Height() { return bounds.height; }
};


// TO DO
//
// Is this class useful?
//
//"minimize" behavior when dragged to screen edge
//resize limiting
//adapt this to allow panel docking


/*! \class anOverlay 
 * \brief Class to simplify floating panels.
 */


} // namespace LaxInterfaces




