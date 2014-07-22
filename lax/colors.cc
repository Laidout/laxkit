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
//    Copyright (C) 2009-2010,2012 by Tom Lechner
//


#include <lax/misc.h>
#include <lax/colors.h>
#include <lax/refptrstack.cc>
#include <lax/strmanip.h>


#define LAXCOLOR_ARGB
#define LAXCOLOR_ABGR
#define LAXCOLOR_RGBA
#define LAXCOLOR_BGRA

namespace Laxkit {

/*! \defgroup colors Color Management
 *
 * These are functions and classes related to color management...
 *
 *
 * \todo
 *   this must be flexible enough to allow sets of colors (like the X11 color names),
 *   spot colors that cannot be scaled, Registration color, None color, and knockout, which is like
 *   an erase as opposed to simply being paper color...
 */



//------------------------------- SimpleColorEventData ------------------------------
/*! \class SimpleColorEventData
 * \ingroup colors
 * \brief Class to pass simple rgb, gray, or cmyk colors.
 *
 * You can use simple_cmyk_to_rgb() and simple_rgb_to_cmyk() to do some simple conversions.
 *
 * By default, constructors will set colortype to be one of LAX_COLOR_RGB, LAX_COLOR_CMYK, or LAX_COLOR_GRAY.
 * You can use other values, but you will have to set it manually.
 *
 * All values are normalized to be from [0..max].
 */

SimpleColorEventData::SimpleColorEventData()
{
	numchannels=0;
	channels=NULL;
	colortype=0;
	max=0;
	id=0;

	type=LAX_ColorEvent; 
}

/*! Construct with 4 channel rgba color.
 */
SimpleColorEventData::SimpleColorEventData(int nmax, int r,int g, int b, int a, int nid)
	: max(nmax), numchannels(4)
{
	channels=new int[4];
	channels[0]=r;
	channels[1]=g;
	channels[2]=b;
	channels[3]=a;
	colortype=LAX_COLOR_RGB;
	type=LAX_ColorEvent; 
	id=nid;
}

/*! Construct with 2 channel gray-alpha color.
 */
SimpleColorEventData::SimpleColorEventData(int nmax, int gray, int a, int nid)
	: max(nmax), numchannels(2)
{
	channels=new int[2];
	channels[0]=gray;
	channels[1]=a;
	colortype=LAX_COLOR_GRAY;
	type=LAX_ColorEvent;
	id=nid;
}

/*! Construct with 5 channel cmyka color.
 */
SimpleColorEventData::SimpleColorEventData(int nmax, int c,int m, int y, int k, int a, int nid)
	: max(nmax), numchannels(5)
{
	channels=new int[5];
	channels[0]=c;
	channels[1]=m;
	channels[2]=y;
	channels[3]=k;
	channels[4]=a;
	colortype=LAX_COLOR_CMYK;
	type=LAX_ColorEvent;
	id=nid;
}

SimpleColorEventData::~SimpleColorEventData()
{
	if (channels) delete[] channels;
}


//------------------------------- ColorEventData -------------------------------
ColorEventData::ColorEventData()
{
	id=info=0;
	color=NULL;
}

ColorEventData::ColorEventData(Color *ncolor, int absorbcount, int nid, int ninfo, int ninfo2)
{
	color=ncolor;
	if (color && !absorbcount) color->inc_count();
	id=nid;
	info=ninfo;
	info2=ninfo2;
}

ColorEventData::~ColorEventData()
{
	if (color) color->dec_count();
}


//------------------------------- ColorPrimary -------------------------------
/*! \class ColorPrimary
 * \ingroup colors
 * \brief Defines a primary color of a ColorSystem.
 *
 * For instance, for an RGB red, name="red" or "R", minvalue, maxvalue might be 0,255.
 * The screencolor is a representation of how the color should appear on the computer screen.
 * Red, for instance would be (255,0,0) for TrueColor visuals in X.
 *
 * ***Attributes would tell extra information about how to display the colors. For instance,
 * to simulate differences between transparent and opaque inks, you might specify 
 * reflectance/absorption values, or maybe bumpiness. These are separate from any alpha or tint values defined
 * elsewhere.
 * 
 * <pre>
 * ***
 *  // sample attributes would be:
 *  // 	Reflectance: 128,128,0 (these would be based on the min/max for system, so for [0..255], 0=0%, 255=100%
 *  // 	Absorption: 0,0,0
 * </pre> 
 */


ColorPrimary::ColorPrimary()
{
	name=NULL;
	maxvalue=minvalue=0;
}

ColorPrimary::~ColorPrimary()
{
	if (name) delete[] name;
}


//------------------------------- ColorSystem -------------------------------

#define COLOR_ADDITIVE    (1<<0)
#define COLOR_SUBTRACTIVE (1<<1)
#define COLOR_SPOT        (1<<2)
#define COLOR_ALPHAOK     (1<<3)
#define COLOR_SPECIAL_INK (1<<4)


/*! \class ColorSystem
 * \ingroup colors
 * \brief Defines a color system, like RGB, CMYK, etc.
 *
 * \code
 *  //ColorSystem::style:
 *  #define COLOR_ADDITIVE    (1<<0)
 *  #define COLOR_SUBTRACTIVE (1<<1)
 *  #define COLOR_SPOT        (1<<2)
 *  #define COLOR_ALPHAOK     (1<<3)
 *  #define COLOR_SPECIAL_INK (1<<4)
 * \endcode
 */
/*! \fn int ColorSystem::AlphaChannel()
 * \brief Return the channel index of the alpha channel, or -1 if none.
 */


ColorSystem::ColorSystem()
{
	name=NULL;
	//iccprofile=NULL;
	systemid=getUniqueNumber();
	style=0;
}

ColorSystem::~ColorSystem()
{
	if (name) delete[] name;
	//cmsCloseProfile(iccprofile);
	primaries.flush();
}


//! Return a new Color instance with the given channel values.
Color *ColorSystem::newColor(int n,...)
{// ***
	return NULL;
}

//------------------------------- Color -------------------------------
/*! \class Color
 * \ingroup colors
 * \brief A base color, one value in range 0..1.0 for each channel of a ColorSystem.
 */


Color::Color()
{
	colorsystemid=0;
	system=NULL;
	n=0;
	alpha=1.0;
	values=NULL;
	name=NULL;
}

Color::~Color()
{
	if (name) delete[] name;
	if (system) system->dec_count();
}

Color::Color(const Color &c)
{
	if (system!=c.system) {
		if (system) system->dec_count();
		system=c.system;
		if (system) system->inc_count();
	}
	if (system) colorsystemid=system->systemid;
	else colorsystemid=c.colorsystemid;
	alpha=c.alpha;
	makestr(name,c.name);
	id=c.id;
	if (n>c.n) {
		delete[] values; values=NULL;
		n=c.n;
		if (n) values=new double[n];
	}
	if (n) memcpy(c.values,values,n*sizeof(double));
}

Color &Color::operator=(Color &c)
{
	if (system!=c.system) {
		if (system) system->dec_count();
		system=c.system;
		if (system) system->inc_count();
	}
	if (system) colorsystemid=system->systemid;
	else colorsystemid=c.colorsystemid;
	alpha=c.alpha;
	makestr(name,c.name);
	id=c.id;
	if (n>c.n) {
		delete[] values;
		n=c.n;
		values=new double[n];
	}
	memcpy(c.values,values,n*sizeof(double));
	return c;
}

//! Return the alpha of the color. 1==totally opaque, 0==totally transparent, -1==no alpha.
/*! If the ColorSystem defines alpha within the primaries, then
 * either that alpha is used in place of this->alpha, but only if
 * this->alpha is not within range 0..1.0. If alpha is in range,
 * then the returned alpha is that multiplied by the system channel.
 * If the system has no built in alpha, then this->alpha is returned.
 */
double Color::Alpha()
{
	if (!system) return alpha;
	int i=system->AlphaChannel();
	if (i<0) return alpha;
	if (alpha<0 || alpha>1.0) return values[i];
	return values[i]*alpha;
}

//! Return the value of channel.
/*! This function does no checking against system.
 * It only checks channel against this->n, and returns values[channel].
 *
 * On error -1 is returned.
 */
double Color::ChannelValue(int channel)
{
	if (channel<0 || channel>=n || channel>=system->primaries.n) return -1;
	return values[channel];
}

//! Return the value of channel as an int in the range of the primary of the ColorSystem.
/*! On error, -1 is returned, and error_ret set to a nonzero number:
 * 1 means no system specified, 2 means channel out of range, 3 means
 * values[channel] is not in range 0 to 1.0.
 * If channel doesn't exist. This assumes that system exists to
 * be queried about primary limits.
 */
int Color::ChannelValueInt(int channel, int *error_ret)
{
	try {
		if (!system) throw 1;
		if (channel<0 || channel>=n || channel>=system->primaries.n) throw 2;
		if (values[channel]<0 || values[channel]>1.0) throw 3;
	} catch (int error) {
		if (error_ret) *error_ret=error;
		return -1;
	}
	if (error_ret) *error_ret=0;
	return (int)(system->primaries.e[channel]->minvalue
		+ values[channel]*(system->primaries.e[channel]->maxvalue-system->primaries.e[channel]->minvalue)
		+ .5);
}



//------------------------------ ColorSet ----------------------------------
/*! \class ColorSet
 * \brief Holds a collection of colors.
 *
 * This can be used for palettes, for instance.
 */

ColorSet::ColorSet()
{
	setstyle=0;
	name=NULL;
}

ColorSet::~ColorSet()
{
	if (name) delete[] name;
}


////------------------------------- ColorManager -------------------------------
//
///*! \class ColorManager
// * \ingroup colors
// * \brief Keeps track of colors and color systems.
// *
// * *** it will be ColorManager's responsibility to read in all the icc profiles,
// * and possibly create color systems from them(? is that reasonable??).
// *
// * Basics are rgb, cmy, cmyk, yuv, hsv.
// */
//class ColorManager
//{
// protected:
// public:
//	PtrStack<char> icc_paths;
//	RefPtrStack<ColorSystem> colorsystems;
//};


} //namespace Laxkit


const char *css_color_names[]={
		"white",   "#ffffff",
		"silver",  "#808080",
		"gray",    "#c0c0c0",

		NULL	
	};

