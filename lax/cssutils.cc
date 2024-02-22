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

#include <cctype>
#include <cstdlib>

#include <lax/units.h>
#include <lax/strmanip.h>
#include <lax/attributes.h>

#include "cssutils.h"


namespace Laxkit {


//--------------------------- Parsing helpers ----------------------------------


/*! Do caseless comparison to "left", "right", "top", or "bottom".
 * Return LAX_LEFT, etc.
 */
int GetLRTB(const char *buffer, long len, const char *&endptr_ret)
{
	endptr_ret = buffer;
	if (len >= 4 && !strncasecmp(buffer, "left",   4)) { endptr_ret = buffer + 4; return LAX_LEFT;   }
	if (len >= 5 && !strncasecmp(buffer, "right",  5)) { endptr_ret = buffer + 5; return LAX_RIGHT;  }
	if (len >= 3 && !strncasecmp(buffer, "top",    3)) { endptr_ret = buffer + 3; return LAX_TOP;    }
	if (len >= 6 && !strncasecmp(buffer, "bottom", 6)) { endptr_ret = buffer + 6; return LAX_BOTTOM; }
	return 0;
}

/*! Parse things like "23rad", "-23deg". Converts and returns in radians in angle_ret.
 * If there is a parse error, angle_ret is not changed.
 * Return 0 for success, or nonzero for error.
 */
int CSSParseAngle(const char *buffer, long len, double &angle_ret, const char *&endptr_ret)
{
	char *endptr = nullptr;
	double angle = strtod(buffer, &endptr);
	if (endptr - buffer > len || endptr == buffer) return 1;

	//flatvector v;
	len -= endptr - buffer;
	buffer = endptr;

	// convert angle to radians
	bool doit = false;
	if      (len > 4 && !strncasecmp(buffer, "turn", 4)) { angle *= 2*M_PI;   buffer += 4; doit = true; }
	else if (len > 3 && !strncasecmp(buffer, "deg",  3)) { angle *= M_PI/180; buffer += 3; doit = true; }
	else if (len > 3 && !strncasecmp(buffer, "rad",  3)) {                    buffer += 3; doit = true; }
	else if (len > 4 && !strncasecmp(buffer, "grad", 4)) { angle *= M_PI/200; buffer += 4; doit = true; }

	if (!doit && isalpha(*buffer)) return 2; //seems to be an unknown unit?

	angle_ret = angle;
	endptr_ret = buffer;

	return 0;
}


//------------------------------ CSS Funcs --------------------------------------

static int num_css_colors = -1;

struct CssColorSpec {
	const char *name;
	const char *color;
};

static const CssColorSpec css_colors[] = {
	{"aliceblue", "#f0f8ff"},
	{"antiquewhite", "#faebd7"},
	{"aqua", "#00ffff"},
	{"aquamarine", "#7fffd4"},
	{"azure", "#f0ffff"},
	{"beige", "#f5f5dc"},
	{"bisque", "#ffe4c4"},
	{"black", "#000000"},
	{"blanchedalmond", "#ffebcd"},
	{"blue", "#0000ff"},
	{"blueviolet", "#8a2be2"},
	{"brown", "#a52a2a"},
	{"burlywood", "#deb887"},
	{"cadetblue", "#5f9ea0"},
	{"chartreuse", "#7fff00"},
	{"chocolate", "#d2691e"},
	{"coral", "#ff7f50"},
	{"cornflowerblue", "#6495ed"},
	{"cornsilk", "#fff8dc"},
	{"crimson", "#dc143c"},
	{"cyan", "#00ffff"},
	{"darkblue", "#00008b"},
	{"darkcyan", "#008b8b"},
	{"darkgoldenrod", "#b8860b"},
	{"darkgray", "#a9a9a9"},
	{"darkgreen", "#006400"},
	{"darkgrey", "#a9a9a9"},
	{"darkkhaki", "#bdb76b"},
	{"darkmagenta", "#8b008b"},
	{"darkolivegreen", "#556b2f"},
	{"darkorange", "#ff8c00"},
	{"darkorchid", "#9932cc"},
	{"darkred", "#8b0000"},
	{"darksalmon", "#e9967a"},
	{"darkseagreen", "#8fbc8f"},
	{"darkslateblue", "#483d8b"},
	{"darkslategray", "#2f4f4f"},
	{"darkslategrey", "#2f4f4f"},
	{"darkturquoise", "#00ced1"},
	{"darkviolet", "#9400d3"},
	{"deeppink", "#ff1493"},
	{"deepskyblue", "#00bfff"},
	{"dimgray", "#696969"},
	{"dimgrey", "#696969"},
	{"dodgerblue", "#1e90ff"},
	{"firebrick", "#b22222"},
	{"floralwhite", "#fffaf0"},
	{"forestgreen", "#228b22"},
	{"fuchsia", "#ff00ff"},
	{"gainsboro", "#dcdcdc"},
	{"ghostwhite", "#f8f8ff"},
	{"gold", "#ffd700"},
	{"goldenrod", "#daa520"},
	{"gray", "#808080"},
	{"green", "#008000"},
	{"greenyellow", "#adff2f"},
	{"grey", "#808080"},
	{"honeydew", "#f0fff0"},
	{"hotpink", "#ff69b4"},
	{"indianred", "#cd5c5c"},
	{"indigo", "#4b0082"},
	{"ivory", "#fffff0"},
	{"khaki", "#f0e68c"},
	{"lavender", "#e6e6fa"},
	{"lavenderblush", "#fff0f5"},
	{"lawngreen", "#7cfc00"},
	{"lemonchiffon", "#fffacd"},
	{"lightblue", "#add8e6"},
	{"lightcoral", "#f08080"},
	{"lightcyan", "#e0ffff"},
	{"lightgoldenrodyellow", "#fafad2"},
	{"lightgray", "#d3d3d3"},
	{"lightgreen", "#90ee90"},
	{"lightgrey", "#d3d3d3"},
	{"lightpink", "#ffb6c1"},
	{"lightsalmon", "#ffa07a"},
	{"lightseagreen", "#20b2aa"},
	{"lightskyblue", "#87cefa"},
	{"lightslategray", "#778899"},
	{"lightslategrey", "#778899"},
	{"lightsteelblue", "#b0c4de"},
	{"lightyellow", "#ffffe0"},
	{"lime", "#00ff00"},
	{"limegreen", "#32cd32"},
	{"linen", "#faf0e6"},
	{"magenta", "#ff00ff"},
	{"maroon", "#800000"},
	{"mediumaquamarine", "#66cdaa"},
	{"mediumblue", "#0000cd"},
	{"mediumorchid", "#ba55d3"},
	{"mediumpurple", "#9370db"},
	{"mediumseagreen", "#3cb371"},
	{"mediumslateblue", "#7b68ee"},
	{"mediumspringgreen", "#00fa9a"},
	{"mediumturquoise", "#48d1cc"},
	{"mediumvioletred", "#c71585"},
	{"midnightblue", "#191970"},
	{"mintcream", "#f5fffa"},
	{"mistyrose", "#ffe4e1"},
	{"moccasin", "#ffe4b5"},
	{"navajowhite", "#ffdead"},
	{"navy", "#000080"},
	{"oldlace", "#fdf5e6"},
	{"olive", "#808000"},
	{"olivedrab", "#6b8e23"},
	{"orange", "#ffa500"},
	{"orangered", "#ff4500"},
	{"orchid", "#da70d6"},
	{"palegoldenrod", "#eee8aa"},
	{"palegreen", "#98fb98"},
	{"paleturquoise", "#afeeee"},
	{"palevioletred", "#db7093"},
	{"papayawhip", "#ffefd5"},
	{"peachpuff", "#ffdab9"},
	{"peru", "#cd853f"},
	{"pink", "#ffc0cb"},
	{"plum", "#dda0dd"},
	{"powderblue", "#b0e0e6"},
	{"purple", "#800080"},
	{"red", "#ff0000"},
	{"rosybrown", "#bc8f8f"},
	{"royalblue", "#4169e1"},
	{"saddlebrown", "#8b4513"},
	{"salmon", "#fa8072"},
	{"sandybrown", "#f4a460"},
	{"seagreen", "#2e8b57"},
	{"seashell", "#fff5ee"},
	{"sienna", "#a0522d"},
	{"silver", "#c0c0c0"},
	{"skyblue", "#87ceeb"},
	{"slateblue", "#6a5acd"},
	{"slategray", "#708090"},
	{"slategrey", "#708090"},
	{"snow", "#fffafa"},
	{"springgreen", "#00ff7f"},
	{"steelblue", "#4682b4"},
	{"tan", "#d2b48c"},
	{"teal", "#008080"},
	{"thistle", "#d8bfd8"},
	{"tomato", "#ff6347"},
	{"turquoise", "#40e0d0"},
	{"violet", "#ee82ee"},
	{"wheat", "#f5deb3"},
	{"white", "#ffffff"},
	{"whitesmoke", "#f5f5f5"},
	{"yellow", "#ffff00"},
	{"yellowgreen", "#9acd32"},
	{nullptr, nullptr}
};

/*! Check against the 147 common css named colors.
 * Return nonzero for found, else 0 for not found.
 */
int CSSNamedColor(const char *value, Laxkit::ScreenColor *scolor)
{
	if (num_css_colors <= 0) {
		num_css_colors = 0;
		while (css_colors[num_css_colors].name) num_css_colors++;
	}

	//binary search
	int match = -1;
	int s = 0, e = num_css_colors-1, m, c;
	if (!strcasecmp(value, css_colors[s].name)) match = s;
	else if (!strcasecmp(value, css_colors[e].name)) match = e;

	while (match == -1) {
		m = (s+e)/2;
		c = strcasecmp(value, css_colors[m].name);
		if (c==0) { match = m; break; }

		if (m==s || m==e) break;

		if (c<0) {
			e = m-1;
			if (e<=s) break;
			c = strcasecmp(value, css_colors[e].name);
			if (c == 0) { match = e; break; }

		} else { // c>0
			s = m+1;
			if (s>=e) break;
			c = strcasecmp(value, css_colors[s].name);
			if (c == 0) { match = s; break; }
		}
	}

	if (match == -1) return 0;

	return HexColorAttributeRGB(css_colors[match].color, scolor, nullptr);
}


/*! Append colors to gradient.
 * Return 0 for success, or nonzero for parsing error.
 */
int CSSGradient(GradientStrip *gradient, const char *buffer, int len, const char **end_ptr_ret)
{
	if (end_ptr_ret) *end_ptr_ret = buffer;
	if (!buffer || len <= 0) return 1;
	if (!gradient) return 2;

	// linear-gradient(to left top, blue, red)
	// linear-gradient(36deg, blue, green 30%, red)
	// linear-gradient(2.5turn, red 0 50%, blue 50% 100%)
	// linear-gradient(90deg in hsl shorter hue, red, blue);
	// linear-gradient(blue, 10%, pink); <- no color for the 10% means at the .1 mark make (blue + pink)/2
	// repeating-linear-gradient
	// radial-gradient
	// repeatin-radial-gradient
	
	while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
	if (len <= 0) return 3;

	char *endptr = nullptr;
	int type = 0;
	#define LINEAR 1
	#define LINEAR_REPEAT 2
	#define RADIAL 3
	#define RADIAL_REPEAT 4

	if (len >= 15 && !strncmp(buffer, "linear-gradient", 15)) {
		buffer += 15; len -= 15; type = LINEAR;
	} else if (len >= 25 && !strncmp(buffer, "repeating-linear-gradient", 25)) {
		buffer += 25; len -= 25; type = LINEAR_REPEAT;
	} else if (len >= 15 && !strncmp(buffer, "radial-gradient", 15)) {
		buffer += 15; len -= 15; type = RADIAL;
	} else if (len >= 25 && !strncmp(buffer, "repeating-radial-gradient", 25)) {
		buffer += 25; len -= 25; type = RADIAL_REPEAT;
	} else return 4;

	while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
	
	if (*buffer != '(' || len == 0) return 5;
	buffer += 1; len -= 1;
	while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }

	// read first parameter
	if (type == LINEAR || type == LINEAR_REPEAT) {
		double angle = strtod(buffer, &endptr);
		flatvector v;
		bool doit = false;
		bool skip_comma = false;

		if (endptr != buffer) {
			if (endptr - buffer > len) return 6;
			buffer = endptr;

			// convert angle to radians
			if (len > 4 && !strncasecmp(buffer, "turn", 4))      { angle *= 2*M_PI;   buffer += 4; len -= 4; doit = true; }
			else if (len > 3 && !strncasecmp(buffer, "deg", 3))  { angle *= M_PI/180; buffer += 3; len -= 3; doit = true; }
			else if (len > 3 && !strncasecmp(buffer, "rad", 3))  {                    buffer += 3; len -= 3; doit = true; }
			else if (len > 4 && !strncasecmp(buffer, "grad", 4)) { angle *= M_PI/200; buffer += 4; len -= 4; doit = true; }

			if (doit) { v.x = cos(angle); v.y = sin(angle); }
			skip_comma = true;

		} else if (len > 2 && !strncasecmp(buffer, "to", 2)) { //not a number, try alternate angle spec
			// [ left | right ]  || [ top | bottom ]
			buffer += 2; len -= 2;
			while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }

			const char *ptr = nullptr;
			int lrtb = GetLRTB(buffer, len, ptr);
			if (lrtb) {
				switch (lrtb) {
					case LAX_LEFT:   v.x = -1; break;
					case LAX_RIGHT:  v.x =  1; break;
					case LAX_TOP:    v.y = -1; break;
					case LAX_BOTTOM: v.y =  1; break;
				}
				len -= ptr - buffer;
				buffer = ptr;
			}
			while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
			lrtb = GetLRTB(buffer, len, ptr);
			if (lrtb) {
				switch (lrtb) {
					case LAX_LEFT:   v.x = -1; break;
					case LAX_RIGHT:  v.x =  1; break;
					case LAX_TOP:    v.y = -1; break;
					case LAX_BOTTOM: v.y =  1; break;
				}
				len -= ptr - buffer;
				buffer = ptr;
			}
			
			while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
			v.normalize();
			skip_comma = true;
		}

		if (skip_comma) {
			while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
			if (len == 0 || *buffer != ',') return false;
			buffer++; len--;
			while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
		}
		
	} else { //RADIAL || RADIAL_REPEAT
			//radial-gradient(ellipse at top, #e66465, transparent),
			//radial-gradient(circle at bottom right, #e66465, transparent),
			//radial-gradient(at bottom right, #e66465, transparent),
			//radial-gradient(#e66465, transparent),
		return 5; //TODO! implement me!
	}

	// read spot list, same format for linear and radial
	// Each spot will be something like: color [pos1 [pos2] ]
	RefPtrStack<Color> colors;
	NumStack<double> pos1;
	NumStack<double> pos2;

	while (len > 0) {
		double v[5];
		double from = -1, to = -1;

		const char *cendptr = nullptr;
		SimpleColorAttribute(buffer, v, &cendptr); //todo: note this does not respect len
		if (cendptr == buffer) break;

		len -= cendptr - buffer;
		buffer = cendptr;

		Color *color = ColorManager::newRGBA(v[0], v[1], v[2], v[3]);
		colors.push(color);
		color->dec_count();

		while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
		if (isdigit(*buffer) || *buffer == '.') {
			from = strtod(buffer, &endptr);
			if (endptr == buffer) return 5;
			if (*endptr == '%') { from /= 100; endptr++; }
			len -= endptr - buffer;
			buffer = endptr;
			
			while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
			if (isdigit(*buffer) || *buffer == '.') {
				to = strtod(buffer, &endptr);
				if (endptr == buffer) return 5;
				if (*endptr == '%') { to /= 100; endptr++; }
				len -= endptr - buffer;
				buffer = endptr;
			}

			pos1.push(from);
			pos2.push(to);
			while (isspace(*buffer) && *buffer && len > 0) { buffer++; len--; }
		}

		if (*buffer == ')') break;
		if (*buffer != ',') return 7; // expected end of gradient function!
		len--;
		buffer++;
	}

	if (len < 1 || *buffer != ')') return 8;
	len--;
	buffer++;

	if (end_ptr_ret) *end_ptr_ret = buffer;

	// Now build the actual gradient
	for (int c=0; c<colors.n; c++) {
		gradient->AddColor(pos1.e[c], colors.e[c], false);
		//TODO: implement stepped gradients, using pos2
	}
	
	return 0;
}


//------------------------------------ CSS Font Functions -----------------------------------


/*! Parse a css font size value.
 * relative_ret gets what a relative value is relative to. Percentage returns values where 1 is 100% and
 * applies to things like "100%", "larger" and "smaller".
 *
 * If there are units, then value is converted to css pixels (96 per inch),
 * EXCEPT when units_ret != nullptr. Then, don't convert, and return the units there.
 *
 * Returns 1 for success or 0 for parse error.
 */
int CSSFontSize(const char *value, double *value_ret, CSSName *relative_ret, int *units_ret, const char **end_ptr) 
{
	 //  font-size: 	<absolute-size> | <relative-size> | <length> | <percentage> | inherit
	 //     absolute-size == xx-small | x-small | small | medium | large | x-large | xx-large 
	 //     relative-size == larger | smaller
	double v=-1;
	CSSName relative = CSS_Unknown;
	int units = UNITS_None;

	if (!strncmp(value,"inherit", 7)) ; //do nothing special

	 //named absolute sizes, these are relative to some platform specific table of numbers:
	else if (!strncmp(value,"xx-small", 8)) { relative = CSS_GlobalSize; v =.5;  value += 8; }
	else if (!strncmp(value,"x-small" , 7)) { relative = CSS_GlobalSize; v =.75; value += 7; }
	else if (!strncmp(value,"small"   , 5)) { relative = CSS_GlobalSize; v =.8;  value += 5; }
	else if (!strncmp(value,"medium"  , 6)) { relative = CSS_GlobalSize; v =1;   value += 6; }
	else if (!strncmp(value,"large"   , 5)) { relative = CSS_GlobalSize; v =1.2; value += 5; }
	else if (!strncmp(value,"x-large" , 7)) { relative = CSS_GlobalSize; v =1.4; value += 7; }
	else if (!strncmp(value,"xx-large", 8)) { relative = CSS_GlobalSize; v =1.7; value += 8; }

   	 //for relative size
	else if (!strncmp(value,"larger", 6)) { relative = CSS_Percent; v=1.2; value += 6; }
	else if (!strncmp(value,"smaller",7)) { relative = CSS_Percent; v=.8;  value += 7; }

	 //percentage, named relative units, or absolute length with units
	else {
		char *endptr = nullptr;
		DoubleAttribute(value,&v,&endptr); //relative size

		if (endptr && endptr != value) { //parse units
			value = endptr;
			while (isspace(*value)) value++;
			if (*value == '%') { relative = CSS_Percent; v /= 100; value++; }
			else if (!strncmp(value, "em" , 2)) { relative = CSS_em;   value += 2; }
			else if (!strncmp(value, "ex" , 2)) { relative = CSS_ex;   value += 2; }
			else if (!strncmp(value, "ch" , 2)) { relative = CSS_ch;   value += 2; }
			else if (!strncmp(value, "rem", 3)) { relative = CSS_rem;  value += 3; }
			else if (!strncmp(value, "vw" , 2)) { relative = CSS_vw;   value += 2; }
			else if (!strncmp(value, "vh" , 2)) { relative = CSS_vh;   value += 2; }
			else if (!strncmp(value, "vmin",4)) { relative = CSS_vmin; value += 4; }
			else if (!strncmp(value, "vmax",4)) { relative = CSS_vmax; value += 4; }
			else {
			    UnitManager *unitm = GetUnitManager();

				endptr = const_cast<char*>(value);
				while (isalnum(*endptr)) endptr++;
				units = unitm->UnitId(value, endptr - value);
				if (units != UNITS_None && !units_ret) {
					v = unitm->Convert(v, units, UNITS_CSSPoints, nullptr);
				}
				value = endptr;
			}
		}
	}

	*value_ret = v;
	if (relative_ret) *relative_ret = relative;
	if (units_ret) *units_ret = units;
	if (end_ptr) *end_ptr = value;

	return 1;
}


/*! Return integer weight, or -1 for error.
 *
 * <pre>
 *  normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit
 * </pre>
 *
 * "bolder" and "lighter" are here arbitrarily mapped to 700 and 200 respectively and relative_ret is set to true.
 * Otherwise relative_ret is set to false.
 *
 */
int CSSFontWeight(const char *value, const char **endptr, bool *relative_ret)
{
	int weight=-1;
	if (relative_ret) *relative_ret = false;

	if (!strncmp(value,"inherit",7))       { *endptr = value+7; } //do nothing special
	else if (!strncmp(value,"normal",6))   { *endptr = value+6; weight=400; }
	else if (!strncmp(value,"bold",4))     { *endptr = value+4; weight=700; }
	else if (!strncmp(value,"bolder", 6))  { *endptr = value+6; weight=700; if (relative_ret) *relative_ret = true; } //120% ?
	else if (!strncmp(value,"lighter", 7)) { *endptr = value+7; weight=200; if (relative_ret) *relative_ret = true; } //80% ? 
	else if (value[0] >= '1' && value[0] <= '9' &&
			 value[1] >= '0' && value[1] <= '9' &&
			 value[2] >= '0' && value[2] <= '9') {
		//scan in any 3 digit integer between 100 and 999 inclusive... not really css compliant, but what the hay
		char *end_ptr;
		weight = strtol(value,&end_ptr,10);
		*endptr = end_ptr;
	} else *endptr = value;

	return weight;
}


/*! Return 0 for "normal", 1 for "italic", 2 for "oblique", and set endptr to just after the word.
 * Else return -1, and endptr will == value.
 */
int CSSFontStyle(const char *value, const char **endptr)
{
	while (isspace(*value)) value++;

	int italic = -1;
	*endptr = value;
	
    if      (!strncmp(value,"normal", 6)  && !isalnum(value[6])) { italic = 0; *endptr = value + 6; }
    else if (!strncmp(value,"italic", 6)  && !isalnum(value[6])) { italic = 1; *endptr = value + 6; }
    else if (!strncmp(value,"oblique", 7) && !isalnum(value[7])) { italic = 2; *endptr = value + 7; }
    //technically oblique is distorted normal, italic is actual new glyphs

    return italic;
}


int CSSFontVariant(const char *value, const char **endptr)
{
	// CSS 2.1 it can be only "normal" or "small-caps".
	// More recent CSS is more complicated, see: 
	//    https://developer.mozilla.org/en-US/docs/Web/CSS/font-variant

	while (isspace(*value)) value++;

	int variant = -1;
	*endptr = value;
	
	if      (!strncmp(value,"normal",     6)  && !isalnum(value[6]))  { variant = 0; *endptr = value + 6; }
    else if (!strncmp(value,"small-caps", 10) && !isalnum(value[10])) { variant = 1; *endptr = value + 10; }

    return variant;
}


} //namespace Laxkit


