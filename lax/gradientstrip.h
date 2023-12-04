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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_GRADIENTSTRIP_H
#define _LAX_GRADIENTSTRIP_H


#include <lax/anobject.h>
#include <lax/previewable.h>
#include <lax/dump.h>
#include <lax/colors.h>
#include <lax/resources.h>


namespace Laxkit {



//--------------------------------- GradientStrip ----------------------------


class GradientStrip : virtual public Resourceable, virtual public DumpUtility, virtual public Previewable
{
 protected:
	unsigned int gradient_flags; //readonly, radial gradient, linear gradient, palette 
  	
 public:
 	enum GradientTypes {
 		Default = 0,
 		GimpGPL,
 		Swatchbooker,
 		CSS
 	};
	enum GradientStripFlags { //for gradient_flags
		StripOnly  = (1<<0),
		Linear     = (1<<1),
		Radial     = (1<<2),
		AsPalette  = (1<<3),
		Gimp_Spots = (1<<4),
		Continue   = (1<<6),
		Repeat     = (1<<5),
		FlipRepeat = (1<<7),
		Read_only  = (1<<8),
		Fill       = (1<<9),
		MAX
	};

	char *name;
	char *file;
	virtual void SetFlags(unsigned int flag, bool on);
	bool IsRadial();
	bool IsLinear();
	bool IsPalette(); 

	enum GradInterpType {
		Constant,
		LinearInterp,
		Quadratic,
		Cubic
	};

	class GradientSpot
	{
	  public:
		unsigned int flags; //maybe Gimp_Spots
		
		char *name;
		double t, nt; //user t, normalized t
		double s, ns; //user s, normalized s, for mixer style display
		//flatvector in,out; //when curve interpolation
		Color *color;

         //for compatibility with gimp gradients:
        double midposition; //0..1, is along segment of this point to next
        int interpolation; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec, 5=constant? 6=curves?
        int transition; //how to vary the color, a line in rgb or in hsv


		GradientSpot(GradientSpot *spot=NULL, bool dup_color=true);
		GradientSpot(double tt,double ss,Laxkit::Color *col, bool dup);
		GradientSpot(double tt,double ss,Laxkit::ScreenColor *col);
		GradientSpot(double tt,double ss, double rr,double gg,double bb,double aa);
		virtual ~GradientSpot();

		virtual void dump_out(FILE *f,int indent,int what,DumpContext *context);
		virtual void dump_in(FILE *f,int indent,DumpContext *context,Attribute **Att=NULL);
		virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
		virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	};

	enum GradStockType {
		WhiteToBlack,
		BlackToWhite,
		TranspToWhite,
		WhiteToTransp
	};

	flatpoint p1, p2;
	double r1, r2; // radii for radial gradients, whose centers possibly go from p1 to p2

	int num_columns_hint; //for palette display
	double tmin, tmax; //range for t in colors
	double smin, smax; //range for s in colors, hint for mixer display

	PtrStack<GradientSpot> colors;
	//LaxImage *strip_render; //for non-cairo interpolations, possibly higher res than preview image

	int continue_left, continue_right; //0 for stop, 1 for continue with closest color, 2 for flip, 3 for repeat

	GradientStrip(int init=0);
	GradientStrip(Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2);
	GradientStrip(Color *col1, int dup1, Color *col2, int dup2); 
	GradientStrip(flatpoint from, flatpoint to, double rr1, double rr2, Color *col1, int dup1, Color *col2, int dup2); 
	GradientStrip(flatpoint from, flatpoint to, double rr1, double rr2, Laxkit::ScreenColor *col1, Laxkit::ScreenColor *col2); 
	virtual ~GradientStrip();
	virtual const char *whattype() { return IsPalette() ? "Palette" : "GradientStrip"; }
	virtual anObject *duplicate(anObject *ref);
	virtual GradientStrip *newGradientStrip();
	virtual void touchContents();
	virtual int maxPreviewSize();


	virtual void SetRadial(flatpoint pp1, flatpoint pp2, double rr1, double rr2);
	virtual void SetLinear(flatpoint pp1, flatpoint pp2, double rr1, double rr2);

	virtual int NumColors();
	virtual double TRange();
	virtual double MinT();
    virtual double MaxT();
	virtual Color *GetColor(int index);
	virtual GradientSpot *GetColorSpot(int index);
	virtual int FlushColors(bool reset);
	virtual int ShiftPoint(int which, double dt, bool clamp);
	virtual void FlipColors();
	virtual void UpdateNormalized(bool set=false);
	virtual double GetNormalizedT(int index);

	virtual int AddColor(GradientSpot *spot);
	virtual int AddColor(double t, double red,double green,double blue,double alpha, const char *nname=nullptr);
	virtual int AddColor(double t, Laxkit::ScreenColor *col, const char *nname=nullptr);
	virtual int AddColor(double t, Color *col, bool dup, const char *nname=nullptr);
	virtual int RemoveColor(int index);

	virtual void Set(Color *col1, bool dup1, Color *col2, bool dup2, bool reset_bounds);
	virtual void Set(Laxkit::ScreenColor *col1, Laxkit::ScreenColor *col2, bool reset_bounds); 
	virtual int SetColor(int index, double red,double green,double blue,double alpha);
	virtual int SetColor(int index, Laxkit::ScreenColor *col);
	virtual int SetColor(int index, Color *col, bool dup);
	virtual int SetStock(GradStockType which);

	virtual Color *WhatColor(double t, bool is_normalized) { return WhatColor(t, (Color*)nullptr, is_normalized); }
	virtual Color *WhatColor(double t, Color *col, bool is_normalized);
	virtual int WhatColor(double t, Laxkit::ScreenColor *col, bool is_normalized);

	virtual void dump_out(FILE *f,int indent,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
	virtual void dump_in (FILE *f,int indent,int what,DumpContext *context,Attribute **att);
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);

	virtual bool ImportGimpPalette(IOBuffer &f);

	virtual int renderToBufferImage(LaxImage *image);
	virtual int RenderPalette(LaxImage *image);
	virtual int RenderRadial(LaxImage *image);
	virtual int RenderLinear(LaxImage *image);

	static GradientStrip *newPalette();
	static GradientStrip *rainbowPalette(int w, int h, bool include_gray_strip);

};

typedef GradientStrip Palette;


} //namespace Laxkit


#endif

