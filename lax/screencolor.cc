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
//    Copyright (C) 2011 by Tom Lechner
//


#include <lax/screencolor.h>
#include <lax/misc.h>
#include <lax/laxutils.h>


namespace Laxkit {

//------------------------------- ScreenColor -------------------------------
/*! \class ScreenColor
 * Fields are range [0..65535].
 */


ScreenColor::ScreenColor(unsigned int color)
{
	int r,g,b;
	colorrgb(color,&r,&g,&b);
	red=r*256;
	green=g*256;
	blue=b*256;
	alpha=65535;
}

int ScreenColor::equals(ScreenColor &color)
{
	return red==color.red && green==color.green && blue==color.blue && alpha==color.alpha;
}

int ScreenColor::equals(double r,double g,double b, double a)
{
	return red==int(.5+r*65535) && green==int(.5+g*65535) && blue==int(.5+b*65535) && alpha==int(.5+a*65535);
}

/*! Return 0xAARRGGBB. Always recomputes, does not rely on this->pixel.
 */
unsigned long ScreenColor::Pixel()
{
	return (blue>>8) | ((green>>8)<<8) | ((red>>8)<<16) | ((alpha>>8)<<24);
}

//! Set colors to represent a gray, range [0..65535].
void ScreenColor::gray(int g, int a)
{
	red=green=blue=g;
	alpha=a;
}

//! Set colors to represent an rgb color. Channels in range [0..65535].
void ScreenColor::rgb(int r,int g,int b, int a)
{
	red=r;
	green=g;
	blue=b;
	alpha=a;
}

//! Set colors to represent a cmyk color.
void ScreenColor::cmyk(int c, int m, int y, int k, int a)
{
	simple_cmyk_to_rgb(c,m,y,k, &red,&green,&blue, 65535);
	alpha=a;
}

//! Set colors to represent a gray, range [0..65535].
void ScreenColor::grayf(double g, double a)
{
	red=green=blue=(g*65535+.5);
	alpha=(a*65535+.5);
}

//! Set colors to represent an rgb color.
void ScreenColor::rgbf(double r,double g,double b, double a)
{
	red=(r*65535+.5);
	green=(g*65535+.5);
	blue=(b*65535+.5);
	alpha=(a*65535+.5);
}

//! Set colors to represent a cmyk color.
void ScreenColor::cmykf(double c, double m, double y, double k, double a)
{
	simple_cmyk_to_rgb((c*65535+.5),(m*65535+.5),(y*65535+.5),(k*65535+.5), &red,&green,&blue, 65535);
	alpha=(a*65535+.5);
}

} // namespace Laxkit

