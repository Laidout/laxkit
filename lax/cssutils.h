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
//    Copyright (C) 2019-2020 by Tom Lechner
//
#ifndef _LAX_CSSUTILS_H
#define _LAX_CSSUTILS_H


#include <lax/screencolor.h>
#include <lax/gradientstrip.h>


namespace Laxkit {


enum CSSName {
	CSS_Unknown,
	CSS_Error,
	CSS_Inherit,

	CSS_em,    //Relative to the font-size of the element (2em means 2 times the size of the current font)
	CSS_ex,    //Relative to the x-height of the current font (rarely used)
	CSS_ch,    //Relative to width of the "0" (zero)
	CSS_rem,   //Relative to font-size of the root element
	CSS_vw,    //Relative to 1% of the width of the viewport
	CSS_vh,    //Relative to 1% of the height of the viewport
	CSS_vmin,  //Relative to 1% of viewport's smaller dimension
	CSS_vmax,  //Relative to 1% of viewport's larger dimension
	CSS_Percent, //Relative to the parent element
	CSS_GlobalSize, //relative to global list of sizes. For xx-small, x-small, medium, large, x-large, xx-large
	CSS_Physical, //units are absolute physical size (not strictly a css concept)

	MAX
};

int CSSNamedColor(const char *value, Laxkit::ScreenColor *scolor);
int CSSGradient(GradientStrip *gradient, const char *buffer, int len, const char **end_ptr_ret);

int CSSFontSize(const char *value, double *value_ret, CSSName *relative_ret, int *units_ret, const char **end_ptr);
int CSSFontWeight(const char *value, const char **endptr, bool *relative_ret);
int CSSFontStyle(const char *value, const char **endptr);
int CSSFontVariant(const char *value, const char **endptr);

int CSSParseAngle(const char *buffer, long len, double &angle_ret, const char *&endptr_ret);

} //namespace Laxkit

#endif

