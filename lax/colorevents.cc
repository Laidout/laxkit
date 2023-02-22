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
//    Copyright (C) 2009-present by Tom Lechner
//

#include <lax/colorevents.h>


namespace Laxkit {


//------------------------------- SimpleColorEventData ------------------------------
/*! \class SimpleColorEventData
 * \ingroup colors
 * \brief Class to pass simple rgb, gray, or cmyk colors.
 *
 * You can use simple_cmyk_to_rgb() and simple_rgb_to_cmyk() to do some simple conversions.
 *
 * By default, constructors will set colorsystem to be one of LAX_COLOR_RGB, LAX_COLOR_CMYK, or LAX_COLOR_GRAY.
 * You can use other values, but you will have to set it manually.
 *
 * All values are normalized to be from [0..max].
 */

SimpleColorEventData::SimpleColorEventData()
{
	numchannels  = 0;
	channels     = nullptr;
	colorindex   = 0;
	colorsystem  = 0;
	colorspecial = 0;
	colormode    = 0;
	max          = 0;
	id           = 0;

	type = LAX_ColorEvent;
}

/*! Construct with 4 channel rgba color.
 */
SimpleColorEventData::SimpleColorEventData(int nmax, int r,int g, int b, int a, int nid)
	: max(nmax), numchannels(4)
{
	channels     = new int[4];
	channels[0]  = r;
	channels[1]  = g;
	channels[2]  = b;
	channels[3]  = a;
	colorindex   = 0;
	colorsystem  = LAX_COLOR_RGB;
	colorspecial = 0;
	colormode    = 0;
	type         = LAX_ColorEvent;
	id           = nid;
}

/*! Construct with 2 channel gray-alpha color.
 */
SimpleColorEventData::SimpleColorEventData(int nmax, int gray, int a, int nid)
	: max(nmax), numchannels(2)
{
	channels     = new int[2];
	channels[0]  = gray;
	channels[1]  = a;
	colorindex   = 0;
	colorsystem  = LAX_COLOR_GRAY;
	colorspecial = 0;
	colormode    = 0;
	type         = LAX_ColorEvent;
	id           = nid;
}

/*! Construct with 5 channel cmyka color.
 */
SimpleColorEventData::SimpleColorEventData(int nmax, int c,int m, int y, int k, int a, int nid)
	: max(nmax), numchannels(5)
{
	channels     = new int[5];
	channels[0]  = c;
	channels[1]  = m;
	channels[2]  = y;
	channels[3]  = k;
	channels[4]  = a;
	colorindex   = 0;
	colorsystem  = LAX_COLOR_CMYK;
	colorspecial = 0;
	colormode    = 0;
	type         = LAX_ColorEvent;
	id           = nid;
}

SimpleColorEventData::~SimpleColorEventData()
{
	if (channels) delete[] channels;
}

double SimpleColorEventData::Valuef(int i) const
{
	if (i < 0 || i >= numchannels) return 0;
	return channels[i] / (double)max;
}


//------------------------------- ColorEventData -------------------------------

/*! \class ColorEventData
 * \ingroup colors
 * \brief Class to pass Color objects in an event.
 */

ColorEventData::ColorEventData()
{
	id = info  = 0;
	color      = nullptr;
	type       = LAX_ColorEvent;
	colorindex = 0;
	colormode  = 0;
}

ColorEventData::ColorEventData(Color *ncolor, int absorbcount, int nid, int ninfo, int ninfo2)
{
	color = ncolor;
	if (color && !absorbcount) color->inc_count();
	id         = nid;
	info       = ninfo;
	info2      = ninfo2;
	colorindex = 0;
	colormode  = 0;
	type       = LAX_ColorEvent;
}

ColorEventData::~ColorEventData()
{
	if (color) color->dec_count();
}

} // namespace Laxkit
