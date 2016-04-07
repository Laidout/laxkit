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
//    Copyright (C) 2009-2010,2012,2015 by Tom Lechner
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


enum BasicColorTypes {
	LAX_COLOR_NONE   =0,
	LAX_COLOR_RGB    , //note to devs: this should always be the first actual type
	LAX_COLOR_CMYK   ,
	LAX_COLOR_GRAY   ,
	LAX_COLOR_HSL    ,
	LAX_COLOR_HSV    ,
	LAX_COLOR_CieLAB ,
	LAX_COLOR_XYZ    ,
	LAX_COLOR_N      ,
	LAX_COLOR_MAX
};

enum SimpleColorId {
	COLOR_No_Color=0, //means undefined, not the same as COLOR_None
	COLOR_Normal,
	COLOR_Foreground,
	COLOR_Background,
	COLOR_Stroke,
	COLOR_Fill,
	COLOR_Controls,
	COLOR_None,
	COLOR_Registration,
	COLOR_Knockout,
	COLOR_MAX
};


//------------------------------- SimpleColorEventData ------------------------------
class SimpleColorEventData : public EventData
{
 public:
	int id;
	int colortype; //one of BasicColorTypes
	int colorspecial;
	int max;
	int numchannels;
	int *channels;

	SimpleColorEventData();
	SimpleColorEventData(int nmax, int gray,int a, int nid);
	SimpleColorEventData(int nmax, int r,   int g, int b, int a, int nid);
	SimpleColorEventData(int nmax, int c,   int m, int y, int k, int a,  int nid);
	virtual ~SimpleColorEventData();
};


//------------------------------- ColorEventData ------------------------------
class Color;
class ColorSystem;

class ColorEventData : public EventData
{
 public:
	Color *color;
	int id;
	int info, info2;

	ColorEventData();
	ColorEventData(Color *ncolor,int absorbcount, int nid, int ninfo, int ninfo2);
	virtual ~ColorEventData();
};


//------------------------------- Color -------------------------------
class Color : public Laxkit::anObject, public LaxFiles::DumpUtility
{
 public:
	char *name; //note this is different than object_idstr which is supposed to be unique
	double alpha; //additional to any alpha defined in ColorSystem itself
	int color_type; //such as none, knockout, or registration, from SimpleColorId

	ColorSystem *system;
	int colorsystemid; //usually same as system->object_id;
	int n; // num values, put here so you don't have to always look them up in system definition
	double *values; // the values for each primary

	ScreenColor screen;

	Color();
	Color(const Color &l);
	Color &operator=(Color &l);
	virtual ~Color();
	virtual Color *duplicate();

	virtual const char *Name();
	virtual double Alpha();
	virtual int ColorType();
	virtual int ColorSystemId();

	virtual double ChannelValue(int channel);
	virtual int ChannelValueInt(int channel, int *error_ret=NULL);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//------------------------------- ColorRef -------------------------------
class ColorRef : virtual public Laxkit::Color
{
  public:
	Color *color;
	int state; //0==good ref. 1==values set, but need to find ref'd obj, -1==undefined

	ColorRef(Color *newcolor);
	virtual ~ColorRef();

	virtual Color *duplicate();
	//virtual const char *Name();
	virtual double Alpha();
	virtual int ColorType();
	virtual int ColorSystemId();

	virtual double ChannelValue(int channel);
	virtual int ChannelValueInt(int channel, int *error_ret=NULL);

    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

	virtual int Reference(Color *newcolor);
};


//------------------------------- ColorPrimary -------------------------------
class ColorPrimary
{
 public:
	char *name;
	double maxvalue;
	double minvalue;
	ScreenColor screencolor;
	//LaxImage *pattern; //tilable image for instance for speckled paint in a ColorN space
	
	LaxFiles::Attribute atts; //*** this could be a ColorAttribute class, to allow ridiculously adaptable color systems
					//    like being able to define a sparkle or metal speck fill pattern 
	               // for each i'th attribute
	ColorPrimary();
	ColorPrimary(const char *nname, double min, double max);
	virtual ~ColorPrimary();
};


//------------------------------- ColorSystem -------------------------------

enum ColorSystemStyles {
	COLOR_Additive    =(1<<0),
	COLOR_Subtractive =(1<<1),
	COLOR_Spot        =(1<<2),
	COLOR_Has_Alpha   =(1<<3),
	COLOR_Special_Ink =(1<<4),
	COLOR_SYSTEM_MAX
};

class Color;

class ColorSystem: public Laxkit::anObject, public LaxFiles::DumpUtility 
{
 public:
	char *name;
	unsigned long style;
	
	//cmsHPROFILE iccprofile;
	PtrStack<ColorPrimary> primaries;

	ColorSystem();
	virtual ~ColorSystem();
	virtual const char *Name() { return name; }

	virtual Color *newColor(int n,...);
	virtual int HasAlpha() { return style & COLOR_Has_Alpha; } //return if it is ok to use alpha for this system
	virtual double ChannelMinimum(int channel); //some systems don't have constant max/min per channel
	virtual double ChannelMaximum(int channel);
	virtual int NumChannels() { return primaries.n + HasAlpha(); }

	 //return an image tile representing the color, speckled inks, for instance
	//virtual LaxImage *PaintPattern(Color *color); 

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext);
};



ColorSystem *Create_sRGB(bool with_alpha);
ColorSystem *Create_Generic_CMYK(bool with_alpha);
ColorSystem *Create_CieLab(bool with_alpha);
ColorSystem *Create_XYZ(bool with_alpha);


////------------------------------- class ColorManager -------------------------------
//
//    todo!


} //namespace Laxkit


#endif


