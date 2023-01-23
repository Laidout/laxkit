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
#ifndef _LAX_COLORS_H
#define _LAX_COLORS_H


#include <lax/refptrstack.h>
#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/screencolor.h>
#include <lax/events.h>

//#include <lcms.h>


namespace Laxkit {


enum BasicColorSystems {
	LAX_COLOR_NONE   =0,
	LAX_COLOR_RGB    , //note to devs: rgb should always be the first actual type
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
	COLOR_Undefined=0, //not the same as COLOR_None
	COLOR_Normal,
	COLOR_None,
	COLOR_Registration,
	COLOR_Knockout,

	COLOR_Foreground,
	COLOR_Background,
	COLOR_Stroke,
	COLOR_Fill,
	COLOR_Controls,
	COLOR_FGBG,
	COLOR_StrokeFill,

	COLOR_CATEGORY_MAX
};


//------------------------------- SimpleColorEventData ------------------------------
class SimpleColorEventData : public EventData
{
 public:
	int id;
	int colorindex;
	int colorsystem; //one of BasicColorSystems
	int colorspecial; //one of SimpleColorId
	int colormode; //0 for single color, or COLOR_FGBG, or COLOR_StrokeFill
	int max;
	int numchannels;
	int *channels;

	SimpleColorEventData();
	SimpleColorEventData(int nmax, int gray,int a, int nid);
	SimpleColorEventData(int nmax, int r,   int g, int b, int a, int nid);
	SimpleColorEventData(int nmax, int c,   int m, int y, int k, int a,  int nid);
	virtual ~SimpleColorEventData();
	virtual double Valuef(int i) const;
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
	int colorindex;
	int colormode; //0 for single color, or COLOR_FGBG, or COLOR_StrokeFill

	ColorEventData();
	ColorEventData(Color *ncolor,int absorbcount, int nid, int ninfo, int ninfo2);
	virtual ~ColorEventData();
};


//------------------------------- Color -------------------------------
class Color : public Laxkit::anObject, public DumpUtility
{
 public:
	char *name; //note this is different than object_idstr which is supposed to be unique
	double alpha; //additional to any alpha defined in ColorSystem itself
	int color_type; //such as none, normal, knockout, or registration. See SimpleColorId

	ColorSystem *system;
	int colorsystemid; //usually same as system->systemid, see BasicColorSystems
	int nvalues; // num values, usually numprimaries+alpha. put here so you don't have to always look them up in system definition
	double *values; // the values for each primary plus alpha at the end (if any)

	ScreenColor screen;

	Color();
	Color(const Color &l);
	Color &operator=(Color &l);
	virtual ~Color();
	virtual const char *whattype() { return "Color"; }
	virtual Color *duplicate();

	virtual const char *Name();
	virtual double Alpha();
	virtual int ColorType();
	virtual int ColorSystemId();
	virtual int UpdateToSystem(Color *color);
	virtual void InstallSystem(ColorSystem *newsystem);
	virtual void UpdateScreenColor();

	virtual double NumChannels() { return nvalues; }
	virtual double ChannelValue(int channel);
	virtual double ChannelValue0To1(int channel);
	virtual double ChannelValue(int channel, double newvalue);

	virtual char *dump_out_simple_string();
	virtual int dump_out_simple_string(char *color, int n);
	virtual void dump_out_simple_string(Attribute *att, const char *prop, const char *comment=nullptr);
	virtual void dump_out(FILE *f,int indent,int what,DumpContext *context);
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
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
	virtual double ChannelValue(int channel, double newvalue);

    virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
    virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);

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
	
	Attribute atts; //*** this could be a ColorAttribute class, to allow ridiculously adaptable color systems
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

class ColorSystem: public Laxkit::anObject, public DumpUtility 
{
 public:
	char *name; //human readable
	char *shortname; //unique short name for this system for use in file i/o
	unsigned long style;
	int systemid;
	
	//cmsHPROFILE iccprofile;
	PtrStack<ColorPrimary> primaries;

	ColorSystem();
	virtual ~ColorSystem();
	virtual const char *whattype() { return "ColorSystem"; }
	virtual const char *Name() { return name; }
	virtual int SystemId() { return systemid; }

	virtual Color *newColor(int nvalues, ...);
	virtual Color *newColor(int nvalues, va_list argptr);
	virtual bool HasAlpha();
	virtual int NumChannels();
	virtual double ChannelMinimum(int channel); //some systems don't have constant max/min per channel
	virtual double ChannelMaximum(int channel);

	 //return an image tile representing the color, speckled inks, for instance
	//virtual LaxImage *PaintPattern(Color *color); 

	virtual void dump_out(FILE *f,int indent,int what,DumpContext *savecontext);
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *savecontext);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext);
};



ColorSystem *Create_sRGB_System(bool with_alpha);
ColorSystem *Create_Gray_System(bool with_alpha);
ColorSystem *Create_Generic_CMYK_System(bool with_alpha);
ColorSystem *Create_CieLab_System(bool with_alpha);
ColorSystem *Create_XYZ_System(bool with_alpha);


//------------------------------- class ColorManager -------------------------------

class ColorManager : public anObject
{
  protected: 
	RefPtrStack<ColorSystem> systems;

  public:
	static ColorManager *GetDefault(bool create=true);
	static void SetDefault(ColorManager *manager);

	static Color *newColor(int systemid, int nvalues, ...);
	static Color *newColor(int systemid, ScreenColor *color);
	static Color *newColor(int systemid, const ScreenColor &color);
	static Color *newColor(Attribute *att);
	static Color *newRGBA(double r, double g, double b, double a);
	static Color *newRGBA(const ScreenColor &color);
		
	ColorManager();
	virtual ~ColorManager();
	virtual const char *whattype() { return "ColorManager"; }
	virtual int AddSystem(ColorSystem *system, bool absorb);
	virtual ColorSystem *FindSystem(const char *name);
};



} //namespace Laxkit


#endif


