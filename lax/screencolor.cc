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
//    Copyright (C) 2011 by Tom Lechner
//


#include <lax/screencolor.h>
#include <lax/misc.h>
#include <lax/laxutils.h>


namespace Laxkit {

//------------------------------- ScreenColor -------------------------------
/*! \class ScreenColor
 * Fields are range [0..65535] and internally stored as integers.
 */


ScreenColor::ScreenColor(unsigned int color)
{
	int r,g,b;
	colorrgb(color,&r,&g,&b);
	red=r*256;
	green=g*256;
	blue=b*256;
	alpha=65535;
	info = 0; //user definable extra tag
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
unsigned long ScreenColor::Pixel() const
{
	return (blue>>8) | ((green>>8)<<8) | ((red>>8)<<16) | ((alpha>>8)<<24);
}

void ScreenColor::Set(unsigned int color)
{
	int r,g,b;
	colorrgb(color,&r,&g,&b);
	red=r*256;
	green=g*256;
	blue=b*256;
	alpha=(0xff000000&color)>>16;
}

//! Set colors to represent a gray, from range [0..255].
void ScreenColor::gray8(int g, int a)
{
	red = green = blue = (g<<8)|g;
	alpha = (a<<8)|a;
}

//! Set colors to represent rgb, from range [0..255].
void ScreenColor::rgb8(int r,int g,int b, int a)
{
	red   = (r<<8)|r;
	green = (g<<8)|g;
	blue  = (b<<8)|b;
	alpha = (a<<8)|a;
}

//! Set colors to represent cmyk, from range [0..255].
void ScreenColor::cmyk8(int c, int m, int y, int k, int a)
{
	c = (c<<8)|c;
	m = (m<<8)|m;
	y = (y<<8)|y;
	k = (k<<8)|k;
	a = (a<<8)|a;

	simple_cmyk_to_rgb(c,m,y,k, &red,&green,&blue, 65535);
	alpha=a;
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

/*! Change the colors by given amounts, which should be in scale [0..1].
 * If addition will overflow, then subtract instead.
 * If subtracting will underflow, then use 0. You shouldn't use such big diffs anyway.
 */
void ScreenColor::AddDiff(double r, double g, double b)
{
	int rr = r*65535;
	int gg = g*65535;
	int bb = b*65535;

	red += rr;
	if (red>65535) {
		red -= 2*rr;
		if (red<0) red=0;
	} else if (red<0) {
		red -= 2*rr;
		if (red>65535) red=65535;
	}

	green += gg;
	if (green>65535) {
		green -= 2*gg;
		if (green<0) green=0;
	} else if (green<0) {
		green -= 2*gg;
		if (green>65535) green=65535;
	}

	blue += bb;
	if (blue>65535) {
		blue -= 2*bb;
		if (blue<0) blue=0;
	} else if (blue<0) {
		blue -= 2*bb;
		if (blue>65535) blue=65535;
	}
}

/*! Put the average color with this in result. It works if result==this.
 * r==0 is just this, r==1 is color.
 *
 * result can't be NULL.
 */
void ScreenColor::Average(ScreenColor *result, const ScreenColor &color, double r)
{
	result->red   = red  *(1-r) + color.red  *r;
	result->green = green*(1-r) + color.green*r;
	result->blue  = blue *(1-r) + color.blue *r;
	result->alpha = alpha*(1-r) + color.alpha*r;
}

/*! Return a new color lerping between this and color. */
ScreenColor ScreenColor::Lerp(const ScreenColor &color, double r) const
{
	ScreenColor col = *this;
	col.Average(&col, color, r);
	return col;
}

/*! Make sure the channel values are [0..65535].
 */
void ScreenColor::Clamp()
{
	if (red<0) red=0;
	else if (red>65535) red=65535;

	if (green<0) green=0;
	else if (green>65535) green=65535;

	if (blue<0) blue=0;
	else if (blue>65535) blue=65535;

	if (alpha<0) alpha=0;
	else if (alpha>65535) alpha=65535;
}

/*! Write out a hex version to buffer. If depth == 8, then output 6 hex digits. Else output 3 hex digits as
 * "rrggbb". If use_alpha, then output 8 or 4 digits with alpha in the first place.
 */
void ScreenColor::hex(char *buffer, bool use_alpha, int depth)
{
	if (depth == 8) {
		if (use_alpha) 
			 sprintf(buffer, "%02x%02x%02x%02x", alpha>>8, red>>8, green>>8, blue>>8);
		else sprintf(buffer, "%02x%02x%02x", red>>8, green>>8, blue>>8);
	}

	if (use_alpha) 
		 sprintf(buffer, "%x%x%x%x", alpha>>12, red>>12, green>>12, blue>>12);
	else sprintf(buffer, "%x%x%x", red>>12, green>>12, blue>>12);
}

/*! Return a screen color that is slightly different than the current.
 * Each channel is added by abs(hint)*65535, unless
 * the channel is overflowed, in which case, subtract the hint instead.
 */
ScreenColor ScreenColor::Hinted(double hint) const
{
	hint = fabs(hint); //to protect against jerks
	ScreenColor ret;
	int diff = hint * 65535;

	if (red + diff > 65535) ret.red = red - diff;
	else ret.red = red + diff;
	if (ret.red < 0) ret.red = 0;
	if (green + diff > 65535) ret.green = green - diff;
	else ret.green = green + diff;
	if (ret.green < 0) ret.green = 0;
	if (blue + diff > 65535) ret.blue = blue - diff;
	else ret.blue = blue + diff;
	ret.alpha = alpha;
	if (ret.blue < 0) ret.blue = 0;

	return ret;
}

} // namespace Laxkit

