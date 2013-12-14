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
#ifndef _LAX_COLORS_H
#define _LAX_COLORS_H


#include <lax/refptrstack.h>
#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/screencolor.h>
#include <lax/events.h>

//#include <lcms.h>


namespace Laxkit {

#define LAX_COLOR_RGB   1
#define LAX_COLOR_CMYK  2
#define LAX_COLOR_GRAY  3

enum SimpleColorId {
	COLOR_Foreground,
	COLOR_Background,
	COLOR_Stroke,
	COLOR_Fill,
	COLOR_Controls,
	COLOR_MAX
};

//------------------------------- SimpleColorEventData ------------------------------
class SimpleColorEventData : public EventData
{
 public:
	int numchannels;
	int *channels;
	int max;
	int colortype; //LAX_COLOR_RGB, LAX_COLOR_CMYK, or LAX_COLOR_GRAY
	int id;
	SimpleColorEventData();
	SimpleColorEventData(int nmax, int gray,int a, int nid);
	SimpleColorEventData(int nmax, int r,   int g, int b, int a, int nid);
	SimpleColorEventData(int nmax, int c,   int m, int y, int k, int a,  int nid);
	virtual ~SimpleColorEventData();
};



//------------------------------- ColorPrimary -------------------------------
class ColorPrimary
{
 public:
	char *name;
	double maxvalue;
	double minvalue;
	ScreenColor screencolor;
	
	LaxFiles::Attribute atts; //*** this could be a ColorAttribute class, to allow ridiculously adaptable color systems
					//    like being able to define a sparkle or metal speck fill pattern 
	               // for each i'th attribute
	ColorPrimary();
	virtual ~ColorPrimary();
};

//------------------------------- ColorSystem -------------------------------

#define COLOR_ADDITIVE    (1<<0)
#define COLOR_SUBTRACTIVE (1<<1)
#define COLOR_SPOT        (1<<2)
#define COLOR_ALPHAOK     (1<<3)
#define COLOR_SPECIAL_INK (1<<4)

class Color;

class ColorSystem: public Laxkit::anObject, public LaxFiles::DumpUtility 
{
 public:
	char *name;
	unsigned int systemid;
	unsigned long style;
	
	//cmsHPROFILE iccprofile;
	PtrStack<ColorPrimary> primaries;

	ColorSystem();
	virtual ~ColorSystem();

	virtual Color *newColor(int n,...);
	virtual int AlphaChannel() = 0;

	 //return an image tile representing the color, speckled inks, for instance
	//virtual LaxImage *PaintPattern(Color *color); 
};



//------------------------------- Color -------------------------------
class Color : public Laxkit::anObject, public LaxFiles::DumpUtility
{
 public:
	int id;
	char *name;  //optional color instance name
	double alpha; //additional to any alpha defined in ColorSystem itself

	ColorSystem *system;
	int colorsystemid; //usually same as system->systemid;
	int n; // num values, put here so you don't have to always look them up in system definition
	double *values; // the values for each primary

	Color();
	virtual ~Color();
	Color(const Color &l);
	Color &operator=(Color &l);
	virtual int ColorSystedId() { return system ? system->systemid : colorsystemid; }
	virtual double ChannelValue(int channel);
	virtual int ChannelValueInt(int channel, int *error_ret=NULL);
	virtual double Alpha();
};



//------------------------------ ColorSet ----------------------------------
class ColorSet : public Laxkit::anObject, public LaxFiles::DumpUtility
{
 public:
	unsigned int setstyle;
	char *name;
	RefPtrStack<Color> colors;
	ColorSet();
	virtual ~ColorSet();
};

////------------------------------- class ColorManager -------------------------------
//
//    todo!


} //namespace Laxkit


#endif


