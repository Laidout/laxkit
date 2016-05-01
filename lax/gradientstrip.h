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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_GRADIENTSTRIP_H
#define _LAX_GRADIENTSTRIP_H


#include <lax/anobject.h>
#include <lax/previewable.h>
#include <lax/dump.h>
#include <lax/colors.h>


namespace Laxkit {



//--------------------------------- GradientStrip ----------------------------
class GradientStrip : virtual public Resourceable, virtual public LaxFiles::DumpUtility, virtual public Previewable
{
 protected:
  	
 public:
	class GradientSpot
	{
	 public:
		double t, nt; //user t, normalized t
		double s, ns; //user s, normalized s, for mixer style display
		Color *color;

         //for compatibility with gimp gradients:
        double midpostition; //0..1, is along segment of this point to next
        int interpolation; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
        int transition; //how to vary the color, a line in rgb or in hsv


		GradientSpot(double tt,double ss,Laxkit::Color *col, bool dup);
		GradientSpot(double tt,double ss,Laxkit::ScreenColor *col);
		GradientSpot(double tt,double ss, double rr,double gg,double bb,double aa);
		virtual ~GradientSpot() {}

		virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
		virtual void dump_in(FILE *f,int indent,LaxFiles::DumpContext *context,LaxFiles::Attribute **Att=NULL);
		virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	};

	char *name;
	char *file;

	int num_columns_hint; //for palette display
	PtrStack<GradientSpot> colors;
	double width, height; //range for t,s of the colors

	GradientStrip();
	GradientData(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2,unsigned int stle);
	virtual ~GradientStrip();
	virtual const char *whattype() { return "GradientStrip"; }
	//virtual Value *duplicate();

	virtual void Set(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2);

	virtual void Set(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			Color *col1,Color *col2);

	virtual int NumColors();
	virtual GradientSpot *GetColor(int index);
	virtual int ShiftPoint(int which,double dt);
	virtual void UpdateNormalized(bool set=false);
	virtual double GetNormalizedT(int i);
	virtual int AddColorI(int index, double red,double green,double blue,double alpha);
	virtual int AddColorI(int index, Laxkit::ScreenColor *col);
	virtual int AddColorI(int index, Color *col);
	virtual int AddColor(double t, double red,double green,double blue,double alpha);
	virtual int AddColor(double t,Laxkit::ScreenColor *col);
	virtual int AddColor(double t,Color *col);
	virtual int RemoveColor(int index);
	virtual int WhatColor(double t,Laxkit::ScreenColor *col);
	virtual int WhatColor(double t,Color *col);
	virtual int WhatColor(double t,double *col);
	virtual int AddColor(GradientSpot *spot);
	virtual void FlipColors();

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} //namespace Laxkit


#endif

