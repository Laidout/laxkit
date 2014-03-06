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
//    Copyright (C) 2004-2011 by Tom Lechner
//
#ifndef _LAX_GRADIENTINTERFACE_H
#define _LAX_GRADIENTINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/screencolor.h>

namespace LaxInterfaces {


#define GRADIENT_RADIAL      (1<<0)
#define GRADIENT_LINEAR      (1<<1)
#define GRADIENT_FILL_PARENT (1<<2)

//--------------------------------- GradientDataSpot ----------------------------

class GradientDataSpot
{
 public:
	double t;
	Laxkit::ScreenColor color;
	GradientDataSpot(double tt,Laxkit::ScreenColor *col);
	GradientDataSpot(double tt,int rr,int gg,int bb,int aa);
	virtual ~GradientDataSpot() {}
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in(FILE *f,int indent,Laxkit::anObject *context,LaxFiles::Attribute **Att=NULL);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//--------------------------------- GradientData ----------------------------
class GradientData : virtual public SomeData
{
 protected:
  	
 public:
	unsigned int style;
	double p1, p2, r1, r2, a;
	Laxkit::PtrStack<GradientDataSpot> colors;
	GradientData();
	GradientData(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2,unsigned int stle);
	virtual ~GradientData() {}
	virtual void Set(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2,unsigned int stle);
	virtual const char *whattype() { return "GradientData"; }
	virtual SomeData *duplicate(SomeData *dup);
	virtual void FindBBox();
	virtual int pointin(flatpoint pp,int pin=1);
	virtual int ShiftPoint(int which,double dt);
	virtual double GetNormalizedT(int i);
	virtual int AddColor(double t,int red,int green,int blue,int alpha);
	virtual int AddColor(double t,Laxkit::ScreenColor *col);
	virtual int WhatColor(double t,Laxkit::ScreenColor *col);
	virtual int WhatColor(double t,double *col);
	virtual int AddColor(GradientDataSpot *spot);
	virtual void FlipColors();

	virtual int renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


//--------------------------------- GradientInterface ----------------------------


class GradientInterface : public anInterface
{
 protected:
	int mx,my,draggingmode;
	int curpoint; //-1=whole thing, >=0 is color point, -2 is none
	Laxkit::NumStack<int> curpoints;
	virtual void newData(int x,int y);
	flatpoint leftp;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
 public:
	 // these are gradient state:
	unsigned long controlcolor;
	int creationstyle,showdecs,usepreview;
	Laxkit::ScreenColor col1,col2;
	flatpoint createv;
	double creater1,creater2;
	int gradienttype;

	GradientData *data;
	ObjectContext *goc;

	GradientInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~GradientInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId();
	virtual const char *Name();
	virtual const char *whattype() { return "GradientInterface"; }
	virtual const char *whatdatatype() { return "GradientData"; }
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int); // assumes not use local
	virtual int UseThisObject(ObjectContext *oc);
	virtual void Clear(SomeData *d) { if ((!d && data) || (d && d==data)) { data->dec_count(); data=NULL; } }

	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual int Refresh();
	virtual void drawLinear();
	virtual void drawRadial();
	virtual void drawRadialLine(double t);
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);

	virtual int scan(int x,int y);
	virtual int SelectPoint(int c);
	virtual void deletedata();
	virtual ObjectContext *Context() { return goc; }
	virtual flatpoint getpoint(int c, int trans);
	virtual int sendcolor(Laxkit::ScreenColor *col);
};

} // namespace LaxInterfaces;

#endif

