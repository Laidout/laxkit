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
//    Copyright (C) 2009-2010,2012,2015 by Tom Lechner
//


#include <lax/misc.h>
#include <lax/colors.h>
#include <lax/refptrstack.cc>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <lax/singletonkeeper.h>



#define DBG
#include <iostream>
using namespace std;


using namespace LaxFiles;


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


//------------------------------- Color -------------------------------
/*! \class Color
 * \ingroup colors
 * \brief A base color, one value in range 0..1.0 for each channel of a ColorSystem.
 */


Color::Color()
{
    colorsystemid = 0;
    color_type    = COLOR_Normal;
    system        = nullptr;
    nvalues       = 0;
    alpha         = 1.0;
    values        = nullptr;
    name          = nullptr;
}

Color::~Color()
{
	if (name) delete[] name;
	if (system) system->dec_count();
	delete[] values;
}

Color::Color(const Color &c)
  : Color()
{
	if (system!=c.system) {
		if (system) system->dec_count();
		system=c.system;
		if (system) system->inc_count();
	}
	if (system) colorsystemid = system->SystemId();
	else colorsystemid = c.colorsystemid;
	color_type = c.color_type;
	screen = c.screen;
	alpha = c.alpha;
	name = newstr(c.name);

	nvalues = c.nvalues;
	if (nvalues) {
		values = new double[nvalues];
		memcpy(values, c.values, nvalues*sizeof(double));
	}
	else values = nullptr;
}

Color &Color::operator=(Color &c)
{
	if (system!=c.system) {
		if (system) system->dec_count();
		system=c.system;
		if (system) system->inc_count();
	}
	if (system) colorsystemid = system->SystemId();
	else colorsystemid = c.colorsystemid;
	color_type = c.color_type;
	alpha = c.alpha;
	screen = c.screen;
	makestr(name,c.name);
	if (nvalues>c.nvalues) {
		delete[] values;
		nvalues=c.nvalues;
		values=new double[nvalues];
	}
	memcpy(c.values,values,nvalues*sizeof(double));
	return c;
}

Color *Color::duplicate()
{
	Color *color = new Color(*this);
	return color;
}

/*! Return the alpha of the color. 1==totally opaque, 0==totally transparent.
 *
 * If there is no attached system, or the system has no alpha, then return this->alpha.
 * Else return values[system->NumChannels()-1], which by custom is where
 * alpha is stored for systems that use it.
 */
double Color::Alpha()
{
	//if (!system) return alpha;
	//int i=system->HasAlpha();
	//if (i<0) return alpha;
	//if (alpha<0 || alpha>1.0) return values[i];
	//return values[i]*alpha;
	//----
	if (!system || (system && !system->HasAlpha())) return alpha;
	return values[system->NumChannels()-1];
}

/*! Return something usually from SimpleColorId. 
 * In the normal course of things, this will usually be one of
 * 	COLOR_None, COLOR_Normal, COLOR_Registration, or COLOR_Knockout.
 */
int Color::ColorType()
{
	return color_type;
}

//! Return the value of channel.
/*! This function does no checking against system.
 * It only checks channel against this->nvalues, and returns values[channel].
 *
 * On error -1 is returned.
 */
double Color::ChannelValue(int channel)
{
	if (channel<0 || channel >= nvalues) return -1;
	return values[channel];
}

/*! Some systems use value ranges outside 0 to 1, so this returns a normalized value.
 */
double Color::ChannelValue0To1(int channel)
{
	if (channel<0 || channel >= nvalues) return -1;
	return (values[channel] - system->ChannelMinimum(channel)) / (system->ChannelMaximum(channel) - system->ChannelMinimum(channel));
}

/*! Set and return new value.
 * WARNING! This function does no validation checking against system.
 */
double Color::ChannelValue(int channel, double newvalue)
{
	if (channel<0 || channel >= nvalues) return -1;
	values[channel] = newvalue;
	return newvalue;
}

/*! Return name if not nullptr, otherwise return Id().
 */
const char *Color::Name()
{
	if (!name) return Id(); 
	return name;
}

/*! Return system->object_id or colorsytemid if system==nullptr;
 */
int Color::ColorSystemId()
{
	return system ? system->SystemId() : colorsystemid;
}

/*! Make this color have the same settings and number of channels as color.
 */
int Color::UpdateToSystem(Color *color)
{
	if (colorsystemid == color->ColorSystemId()) return 0;
	if (system!=color->system) {
		if (system) system->dec_count();
		system = color->system;
		if (system) system->inc_count();
		colorsystemid = (system ? system->systemid : color->ColorSystemId());

		if (nvalues != color->NumChannels()) {
			delete[] values;
			values=nullptr;
			nvalues=color->NumChannels();

			if (nvalues) {
				values = new double[nvalues];
				for (int c=0; c<nvalues; c++) values[0]=0;
			}
		}
	}

	return 0;
}

void Color::UpdateScreenColor()
{
	int systemid = ColorSystemId();
	if (systemid == LAX_COLOR_RGB) {
		screen.rgbf(values[0],values[1],values[2],values[3]);
	} else if (systemid == LAX_COLOR_GRAY) {
		screen.grayf(values[0],values[1]);
	}
}

/*! Make the color be valid for this system, and ensure there are the right number
 * of channels.
 */
void Color::InstallSystem(ColorSystem *newsystem)
{
	if (system!=newsystem) {
		if (system) system->dec_count();
		system=newsystem;
		if (system) system->inc_count();
	}
	colorsystemid = (system ? system->SystemId() : 0);

	if (nvalues != system->NumChannels()) {
		delete[] values;
		values=nullptr;
		nvalues = system->NumChannels();

		if (nvalues) {
			values = new double[nvalues];
			for (int c=0; c<nvalues; c++) values[c]=0;
		}
	}
}

char *Color::dump_out_simple_string()
{
	int n = 0;
	n = dump_out_simple_string(nullptr, n);
	char *str = new char[n];
	dump_out_simple_string(str, n);
	return str;
}

/*! For instance, an sRGB might be output as "rgbaf(1.0, 0.0, 0.0, .5)".
 *
 * If system!=nullptr, then use system->shortname as the base.
 * If system==nullptr, then assume rgb.
 *
 * If color==null or n is less than the number of chars needed, return the number needed.
 */
int Color::dump_out_simple_string(char *color, int n)
{
	int needed=0;
	if (color == nullptr) {
		if (color_type==COLOR_Normal) {
			if (nvalues==0) needed = 100;
			else needed = (nvalues+1) * 20;
		} else needed = 13;
	}
	if (needed > n) return needed;

	if (color_type==COLOR_Normal) {
		if (nvalues==0) {
			 //fallback to using ScreenColor
			sprintf(color, "rgbaf(%.10g,%.10g,%.10g,%.10g)",
				screen.Red(), screen.Green(), screen.Blue(), screen.Alpha());
			return strlen(color);
		}

		const char *base=nullptr;
		int hasalpha=1;

		if (system) {
			base = system->shortname;
			hasalpha = system->HasAlpha();
		} else {
			if      (colorsystemid==LAX_COLOR_RGB   ) base="rgb";
			else if (colorsystemid==LAX_COLOR_CMYK  ) base="cmyk";
			else if (colorsystemid==LAX_COLOR_GRAY  ) base="gray";
			else if (colorsystemid==LAX_COLOR_HSL   ) base="hsl";
			else if (colorsystemid==LAX_COLOR_HSV   ) base="hsv";
			else if (colorsystemid==LAX_COLOR_CieLAB) base="cielab";
			else if (colorsystemid==LAX_COLOR_XYZ   ) base="xyz";
			else if (colorsystemid==LAX_COLOR_N     ) base="n";
			else base="rgb";
		}

		sprintf(color, "%s%sf(", base, hasalpha ? "a" : "");

		char *ptr;
		for (int c=0; c<NumChannels(); c++) {
			ptr=color+strlen(color);
			if (NumChannels()==1 || c==NumChannels()-1) sprintf(ptr, "%.10g", ChannelValue(c));
			else sprintf(ptr, "%.10g, ", ChannelValue(c));
		}

		ptr=color+strlen(color);
		ptr[0]=')';
		ptr[1]='\0';

	} else {
		if (color_type==COLOR_None) sprintf(color, "none");
		else if (color_type==COLOR_Knockout) sprintf(color, "knockout");
		else if (color_type==COLOR_Registration) sprintf(color, "registration");
	}

	return strlen(color);
}

void Color::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

LaxFiles::Attribute *Color::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("name","Red #a human readable name for this color instance.");
		att->push("type","Normal # or none,registration,knockout,");
		att->push("system","sRGB #name of color system this belongs to");
		att->push("values","1.0 1.0 1.0 1.0 #floating point values of each channel");
	}

	att->push("name",Name());
	
	if (system) att->push("system",system->Name());
	else if (colorsystemid) att->push("system_id",colorsystemid);

	if (color_type==COLOR_Normal) {
		if ((system && !system->HasAlpha()) || !system) att->push("alpha",alpha);

		char str[nvalues*20];
		str[0]=0;
		for (int c=0; c<nvalues; c++) {
			sprintf(str+strlen(str),"%.10g ", values[c]);
		}
		att->push("values",str);

	} else {
		if (color_type==COLOR_Knockout) att->push("type","knockout");
		else if (color_type==COLOR_None) att->push("type","none");
		else if (color_type==COLOR_Registration) att->push("type","registration");
	}

	return att;
}

void Color::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext)
{
    char *name,*value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

		if (!strcmp(name,"name")) {
			if (!isblank(value)) makestr(name,value);

		} else if (!strcmp(name,"alpha")) {
			DoubleAttribute(value, &alpha);

		} else if (!strcmp(name,"system")) {
			ColorManager *colormanager = ColorManager::GetDefault();
			ColorSystem *sys = colormanager->FindSystem(value);
			if (system != sys) {
				if (system) system->dec_count();
				system=sys;
				if (system) system->inc_count();
			}

		} else if (!strcmp(name,"system_id")) {
			IntAttribute(value, &colorsystemid);

		} else if (!strcmp(name,"type")) {
			if (!value) continue;
			if (!strcmp(value,"none")) color_type=COLOR_None;
			else if (!strcmp(value,"knockout")) color_type=COLOR_Knockout;
			else if (!strcmp(value,"registration")) color_type=COLOR_Registration;
			else if (!strcmp(value,"normal")) color_type=COLOR_Normal;

		} else if (!strcmp(name,"values")) {
			int nn=0;
			double *list=nullptr;
			DoubleListAttribute(value,&list,&nn);
			if (nn) {
				nvalues=nn;
				delete[] values;
				values=list;
			}
		}
	}

	 //validate that values has correct number of channels
	if (system) {
		if (system->NumChannels()!=nvalues) {
			if (nvalues>system->NumChannels()) nvalues=system->NumChannels();
			else if (nvalues<system->NumChannels()){
				 //arbitrarily add 0s for any missing channels
				double *list=new double[system->NumChannels()];
				if (values && nvalues) memcpy(list, values, nvalues*sizeof(double));
				delete[] values;
				values=list;
				for ( ; nvalues<system->NumChannels(); nvalues++) {
					values[nvalues]=0;
				}
			}
		}
	}
}

//------------------------------- ColorRef -------------------------------
/*! \class ColorRef
 */

ColorRef::ColorRef(Color *newcolor)
{
	state=-1;
	color=nullptr;
	Reference(newcolor);
}

ColorRef::~ColorRef()
{
	if (color) color->dec_count();
}


Color *ColorRef::duplicate()
{
	ColorRef *col = new ColorRef(color);
	return col;
}

//const char *ColorRef::Name()
//{
//}

double ColorRef::Alpha()
{
	if (color) return color->Alpha();
	return 0;
}

int ColorRef::ColorType()
{
	if (color) return color->ColorType();
	return 0;
}

int ColorRef::ColorSystemId()
{
	if (color) return color->ColorSystemId();
	return 0;
} 

double ColorRef::ChannelValue(int channel)
{
	if (color) return color->ChannelValue(channel);
	return -1;
}

double ColorRef::ChannelValue(int channel, double newvalue)
{
	if (color) return color->ChannelValue(channel, newvalue);
	return -1;
}

/*! Change the color that this ColorRef references.
 * Incs newcolor. If newcolor is nullptr, nothing is done.
 * Return 0 for changed (or newcolor was already the color), 1 for not changed.
 */
int ColorRef::Reference(Color *newcolor)
{
	if (!newcolor) return 1;
	if (color==newcolor) return 0;
	if (color) color->dec_count();
	color=newcolor;
	if (color) color->inc_count();
	state=0;

	return 0;
}


LaxFiles::Attribute *ColorRef::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (color) {
		att=color->dump_out_atts(att,what,context);
		att->push("ref", color->Name());
		// *** needs fleshing out how and where colors are referenced
	}

	return att;
}

void ColorRef::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{ //***
	Attribute *att2 = att->find("ref");
	if (!att2) return;
	cerr << " *** NEED TO IMPLEMENT ColorRef::dump_in_atts()"<<endl;
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
	name=nullptr;
	minvalue=0;
	maxvalue=1;
}

ColorPrimary::ColorPrimary(const char *nname, double min, double max)
{
	name=newstr(nname);
	minvalue=min;
	maxvalue=max;
}

ColorPrimary::~ColorPrimary()
{
	if (name) delete[] name;
}


//------------------------------- ColorSystem -------------------------------

/*! \class ColorSystem
 * \ingroup colors
 * \brief Defines a color system, like RGB, CMYK, etc.
 *
 */


ColorSystem::ColorSystem()
{
	name = nullptr;
	shortname = nullptr;
	//iccprofile = nullptr;
	style = 0;
}

ColorSystem::~ColorSystem()
{
	delete[] name;
	delete[] shortname;
	//cmsCloseProfile(iccprofile);
	primaries.flush();
}

Color *ColorSystem::newColor(int nvalues, ...)
{
	va_list argptr;
	va_start(argptr, nvalues);
	Color *color = newColor(nvalues, argptr);
	va_end(argptr); 
	return color;
}

/*! Return a new Color instance with the given channel values.
 * There must be nvalues double values.
 */
Color *ColorSystem::newColor(int nvalues, va_list argptr)
{
	Color *color = new Color();
	color->InstallSystem(this);

	for (int c=0; c<nvalues && c<NumChannels(); c++) {
		double val = va_arg(argptr, double);
		DBG cerr << "add to color at "<<c<<": "<<val<<endl;
		color->ChannelValue(c, val);
	}
	color->UpdateScreenColor();

	return color;
}

/*! Return if it is ok to use alpha for this system. The channel is assumed
 * to be primaries.n, thus for a Color, you can get the alpha
 * with color->ChannelValue(system->primaries.n).
 */
bool ColorSystem::HasAlpha()
{
	return (style & COLOR_Has_Alpha) ? true : false;
}

/*! Returns the number of primaries, plus one if HasAlpha().
 */
int ColorSystem::NumChannels()
{
	return primaries.n + (HasAlpha() ? 1 : 0);
}

/*! Return 0 if channel number out of bounds.
 * Note some systems don't have constant max/min per channel .... ***WHAT DOES THIS MEAN??? UPDATE ME!
 */
double ColorSystem::ChannelMinimum(int channel)
{
	if (channel<0 || channel >= primaries.n) return 0;
	return primaries.e[channel]->minvalue;
}

/*! Return 0 if channel number out of bounds.
 */
double ColorSystem::ChannelMaximum(int channel)
{
	if (channel<0 || channel >= primaries.n) return 0;
	return primaries.e[channel]->maxvalue;
}

void ColorSystem::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

LaxFiles::Attribute *ColorSystem::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("name","Red #a human readable name.");
		//att->push("profile","/path/to/iccprofile");
	}

	att->push("name",Id());
	att->push("has_alpha", HasAlpha() ? "yes" : "no");
	
	cerr <<" *** need to finish implementing ColorSystem::dump_out_atts()!!"<<endl;
	for (int c=0; c<primaries.n; c++) {
		// ***

	}

	return att;
}

void ColorSystem::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	cerr <<" *** need to finish implementing ColorSystem::dump_in_atts()!!"<<endl;
}



//------------------------------- Built in ColorSystem creators-------------------------------

/*! Create a ColorSystem based on sRGB.
 */
ColorSystem *Create_sRGB_System(bool with_alpha)
{
	ColorSystem *rgb=new ColorSystem;
	makestr(rgb->name,_("sRGB"));
	makestr(rgb->shortname,"rgb");
	rgb->systemid = LAX_COLOR_RGB;
	if (with_alpha) rgb->style |= COLOR_Has_Alpha;

	//rgb->iccprofile=***;

	 //red
	ColorPrimary *primary=new ColorPrimary;
	makestr(primary->name,_("Red"));
	primary->screencolor.rgbf(1.0,0.0,0.0);
	rgb->primaries.push(primary);

	 //green
	primary=new ColorPrimary;
	makestr(primary->name,_("Green"));
	primary->screencolor.rgbf(0.0,1.0,0.0);
	rgb->primaries.push(primary);

	 //blue
	primary=new ColorPrimary;
	makestr(primary->name,_("Blue"));
	primary->screencolor.rgbf(0.0,0.0,1.0);
	rgb->primaries.push(primary);

	return rgb;
}

ColorSystem *Create_Gray_System(bool with_alpha)
{
	ColorSystem *gray=new ColorSystem;
	makestr(gray->name,_("Gray"));
	makestr(gray->shortname,"gray");
	gray->systemid = LAX_COLOR_GRAY;
	if (with_alpha) gray->style |= COLOR_Has_Alpha;

	//gray->iccprofile=***;

	 //black
	ColorPrimary *primary=new ColorPrimary;
	makestr(primary->name,_("Black"));
	primary->screencolor.rgbf(0.0,0.0,0.0);
	gray->primaries.push(primary);

	return gray;
}

/*! Create a ColorSystem based on naive cmyk.
 */
ColorSystem *Create_Generic_CMYK_System(bool with_alpha)
{
	ColorSystem *cmyk=new ColorSystem;
	makestr(cmyk->name,_("Generic CMYK"));
	makestr(cmyk->shortname,"cmyk");
	cmyk->systemid = LAX_COLOR_CMYK;
	if (with_alpha) cmyk->style |= COLOR_Has_Alpha;

	//cmyk->iccprofile=***;

	 //cyan
	ColorPrimary *primary=new ColorPrimary;
	makestr(primary->name,_("Cyan"));
	primary->screencolor.cmykf(1.0,0.0,0.0,0.0);
	cmyk->primaries.push(primary);

	 //Magenta
	primary=new ColorPrimary;
	makestr(primary->name,_("Magenta"));
	primary->screencolor.cmykf(0.0,1.0,0.0,0.0);
	cmyk->primaries.push(primary);

	 //yellow
	primary=new ColorPrimary;
	makestr(primary->name,_("Yellow"));
	primary->screencolor.cmykf(0.0,0.0,1.0,0.0);
	cmyk->primaries.push(primary);

	 //black
	primary=new ColorPrimary;
	makestr(primary->name,_("Black"));
	primary->screencolor.cmykf(0.0,0.0,0.0,1.0);
	cmyk->primaries.push(primary);

	return cmyk;
}


ColorSystem *Create_CieLab_System(bool with_alpha)
{
	ColorSystem *cielab=new ColorSystem;
	makestr(cielab->name,_("CieL*a*b*"));
	makestr(cielab->shortname,"cielab");
	cielab->systemid = LAX_COLOR_CieLAB;
	if (with_alpha) cielab->style |= COLOR_Has_Alpha;

	//cielab->iccprofile=***;

	 //L*
	ColorPrimary *primary=new ColorPrimary(_("L"), 0,100);
	primary->screencolor.rgbf(0.0,0.0,0.0);
	cielab->primaries.push(primary);

	 //a*
	primary=new ColorPrimary(_("a"), -128,127);
	primary->screencolor.rgbf(0.0,1.0,0.0);
	cielab->primaries.push(primary);

	 //b*
	primary=new ColorPrimary(_("b"), -128,127);
	primary->screencolor.rgbf(1.0,0.0,1.0);
	cielab->primaries.push(primary);


	return cielab;
}

ColorSystem *Create_XYZ_System(bool with_alpha)
{
	//todo: the screen color representation needs to make sense.. it doesn't currently
	

	ColorSystem *xyz=new ColorSystem;
	makestr(xyz->name,_("XYZ"));
	makestr(xyz->shortname,"xyz");
	xyz->systemid = LAX_COLOR_XYZ;
	if (with_alpha) xyz->style|=COLOR_Has_Alpha;

	//xyz->iccprofile=***;

	 //X
	ColorPrimary *primary=new ColorPrimary(_("X"), 0,100);
	primary->screencolor.rgbf(0.0,0.0,0.0);
	xyz->primaries.push(primary);

	 //Y
	primary=new ColorPrimary(_("Y"), 0,100);
	primary->screencolor.rgbf(0.0,1.0,0.0);
	xyz->primaries.push(primary);

	 //Z
	primary=new ColorPrimary(_("Z"), 0,100);
	primary->screencolor.rgbf(0.0,0.0,1.0);
	xyz->primaries.push(primary);


	return xyz;
}



////------------------------------- ColorManager -------------------------------
//
/*! \class ColorManager
 * \ingroup colors
 * \brief Keeps track of colors and color systems.
 *
 * *** it will be ColorManager's responsibility to read in all the icc profiles,
 * and possibly create color systems from them(? is that reasonable??).
 *
 * Basics are rgb, gray, cmy, cmyk, yuv, hsv.
 */


static SingletonKeeper cmKeeper;

ColorManager *ColorManager::GetDefault(bool create)
{
	DBG cerr << "ColorManager::GetDefault()"<<endl;
	ColorManager *default_manager = dynamic_cast<ColorManager*>(cmKeeper.GetObject());
	if (!default_manager && create) {
		default_manager = new ColorManager();
		cmKeeper.SetObject(default_manager, 1);
	}

	return default_manager;
}

/*! Increments count.
 */
void ColorManager::SetDefault(ColorManager *manager)
{
	DBG cerr << "ColorManager::SetDefault()"<<endl;

	ColorManager *default_manager = dynamic_cast<ColorManager*>(cmKeeper.GetObject());

	if (manager == default_manager) return;
	cmKeeper.SetObject(manager, 0);
}

/*! Static function to return a random color.
 * Finds the system with systemid in default ColorManager, then
 * returns whatever that system makes.
 */
Color *ColorManager::newColor(int systemid, int nvalues, ...)
{
	ColorManager *manager = GetDefault();

	va_list argptr;
	va_start(argptr, nvalues);

	Color *color=nullptr;
	for (int c=0; c<manager->systems.n; c++) {
		if (systemid == manager->systems.e[c]->SystemId()) {
			color=manager->systems.e[c]->newColor(nvalues, argptr);
			break;
		}
	}
	va_end(argptr);

	return color;
} 

/*! Static function to convert ScreenColor to a systemid (such as LAX_COLOR_RGB) Color.
 */
Color *ColorManager::newColor(int systemid, ScreenColor *color)
{
	if (!color) return nullptr;
	return newColor(systemid, 4, color->red/65535., color->green/65535., color->blue/65535., color->alpha/65535.);
}

Color *ColorManager::newColor(int systemid, const ScreenColor &color)
{
	return newColor(systemid, 4, color.red/65535., color.green/65535., color.blue/65535., color.alpha/65535.);	
}

Color *ColorManager::newColor(LaxFiles::Attribute *att)
{
	if (!att) return nullptr;

	//examine att->value, if it starts with an alnum string matching one of a system's shortnames,
	//then use that system, and assign values in parentheses.
	//Else search in att->attributes for system, and search based on that

	if (att->attributes.n) {
		cerr << " *** FINISH IMPLEMENTING ColorManager::newColor(Attribute)!!!!"<<endl;

	} else {
		if (isblank(att->value)) return nullptr;
		double v[5];

		if (SimpleColorAttribute(att->value, v, nullptr) != 0) return nullptr;
		return ColorManager::newColor(LAX_COLOR_RGB, 4, v[0], v[1], v[2], v[3]);
	}

	return nullptr;
}

ColorManager::ColorManager()
{
	Id("ColorManager");
	DBG cerr <<"ColorManager "<<(Id()?Id():"unnamed")<<" constructor"<<endl;
}

ColorManager::~ColorManager()
{
	DBG cerr <<"ColorManager "<<(Id()?Id():"unnamed")<<" destructor"<<endl;
}

/*! Return 0 for newly added. Return index+1 for system already in stack.
 *  Return -1 for could not add.
 */
int ColorManager::AddSystem(ColorSystem *system, bool absorb)
{
	if (!system) return -1;

	for (int c=0; c<systems.n; c++) {
		if (system==systems.e[c]) {
			if (absorb) system->dec_count();
			return c+1;
		}
	}

	systems.push(system);
	if (absorb) system->dec_count();

	return 0;
}

ColorSystem *ColorManager::FindSystem(const char *name)
{
	if (isblank(name)) return nullptr;

	//caselessly search unique names
	for (int c=0; c<systems.n; c++) {
		if (!strcasecmp(systems.e[c]->shortname, name)) return systems.e[c];
	}

	//search human readable names
	for (int c=0; c<systems.n; c++) {
		if (!strcasecmp(systems.e[c]->name, name)) return systems.e[c];
	}

	return nullptr;
}



} //namespace Laxkit



