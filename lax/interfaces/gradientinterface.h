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
//    Copyright (C) 2004-2011 by Tom Lechner
//
#ifndef _LAX_GRADIENTINTERFACE_H
#define _LAX_GRADIENTINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/screencolor.h>
#include <lax/gradientstrip.h>


namespace LaxInterfaces {




//--------------------------------- GradientData ----------------------------
class GradientData : virtual public SomeData
{
  protected:
	Laxkit::flatpoint p1,p2;
	float r1, r2;
  	
  public:
	enum GradentDataStyle {
		GRADIENT_RADIAL      = (1<<0),
		GRADIENT_LINEAR      = (1<<1),
		GRADIENT_FILL_PARENT = (1<<2),
		GRADIENT_X_Only      = (1<<3)
	};

	unsigned int style;
	double hint_a;
	bool use_strip_points;
	bool fill_parent;
	int spread_method;

	Laxkit::GradientStrip *strip;

	GradientData();
	GradientData(Laxkit::flatpoint pp1,Laxkit::flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2,unsigned int stle);
	virtual ~GradientData();
	virtual const char *whattype() { return "GradientData"; }
	virtual SomeData *duplicateData(SomeData *dup);
	virtual void FindBBox();
	virtual int pointin(Laxkit::flatpoint pp,int pin=1);

	virtual double *GradientTransform(double *result, bool invert);
	virtual int IsRadial() { return style&GRADIENT_RADIAL; }
	virtual int IsLinear() { return style&GRADIENT_LINEAR; }
	virtual void SetRadial();
	virtual void SetLinear();
	virtual void SetRadial(Laxkit::flatpoint pp1,Laxkit::flatpoint pp2,double rr1,double rr2);
	virtual void SetLinear(Laxkit::flatpoint pp1,Laxkit::flatpoint pp2,double rr1,double rr2);
	virtual void SetRadial(Laxkit::flatpoint pp1,Laxkit::flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2);
	virtual void SetLinear(Laxkit::flatpoint pp1,Laxkit::flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2);
	virtual void Set(Laxkit::flatpoint pp1,Laxkit::flatpoint pp2,double rr1,double rr2,
			Laxkit::ScreenColor *col1,Laxkit::ScreenColor *col2, unsigned int stle);
	virtual int Set(Laxkit::GradientStrip *newstrip, int absorb, bool keep_placement, bool own_resource);
	virtual int ShiftPoint(int which, double dt, bool clamp);
	virtual double GetNormalizedT(int i);
	virtual int NumColors();
	virtual double AddColor(double t,double red,double green,double blue,double alpha);
	virtual int AddColor(double t,Laxkit::ScreenColor *col);
	virtual int AddColor(Laxkit::GradientStrip::GradientSpot *spot);
	virtual int WhatColor(Laxkit::flatpoint p, Laxkit::ScreenColor *col, bool is_normalized);
	virtual int WhatColor(double t,Laxkit::ScreenColor *col, bool is_normalized);
	virtual int WhatColor(double t,double *col, bool is_normalized);
	virtual void FlipColors();
	virtual float MinT();
	virtual float MaxT();

	virtual Laxkit::flatpoint P1(Laxkit::flatpoint p);
	virtual Laxkit::flatpoint P1() const;
	virtual Laxkit::flatpoint P2(Laxkit::flatpoint p);
	virtual Laxkit::flatpoint P2() const;
	virtual float R1(float r);
	virtual float R1() const;
	virtual float R2(float r);
	virtual float R2() const;

	virtual int renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
};


//--------------------------------- GradientInterface ----------------------------


class GradientInterface : public anInterface
{
  protected:
	int mx,my,draggingmode;
	int curpoint; //-1=whole thing, >=0 is color point, -2 is none
	Laxkit::NumStack<int> curpoints;
	virtual void newData(double x, double y);
	Laxkit::flatpoint leftp;
	Laxkit::flatpoint hoverdbg, screendbg;
	GradientData *strip_data;

	bool show_settings_icon;
	Laxkit::LaxImage *settings_icon;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

  public:
	enum DisplayStyles {
        Escapable      = (1<<0),
        RealSpace      = (1<<1),
        RealSpaceMouse = (1<<2),
        Expandable     = (1<<3),
        EditStrip      = (1<<4),
        SettingsIcon   = (1<<5)
    };

	enum GradientInterfaceActions {
		GRAD_SelectLeft,
		GRAD_SelectRight,
		GRAD_Decorations,
		GRAD_Flip,
		GRAD_MakeLinear,
		GRAD_MakeRadial,
		GRAD_Select,
		GRAD_Delete,
		GRAD_Spread_None,
		GRAD_Spread_Repeat,
		GRAD_Spread_Reflect,
		GRAD_Spread_Pad,
		GRAD_Spread_Next,
		GRAD_Spread_Prev,
		GRAD_Toggle_Fill_Parent,
		GRAD_Settings_Icon,
		GRAD_Drag_To_Sample,

		GRAD_MAX
	};

	enum GradientDecs {
		ShowControls = (1<<0),
		ShowColors = (1<<1)
	};


	 // these are gradient state:
	unsigned long controlcolor;
	int creationstyle,showdecs,usepreview;
	Laxkit::ScreenColor col1,col2;
	Laxkit::flatpoint createv;
	double creater1,creater2;
	int gradienttype;
    unsigned int style; //DisplayStyles


	Laxkit::GradientStrip *strip;
	GradientData *data;
	ObjectContext *goc;

	GradientInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~GradientInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual const char *IconId();
	virtual const char *Name();
	virtual const char *whattype() { return "GradientInterface"; }
	virtual const char *whatdatatype() { return "GradientData"; }
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int); // assumes not use local
	virtual int UseThisObject(ObjectContext *oc);
	virtual int UseThisObject(Laxkit::GradientStrip *nstrip);
	virtual void Clear(SomeData *d) { if ((!d && data) || (d && d==data)) { data->dec_count(); data=NULL; } }
	virtual void SetupRect(double x,double y,double w,double h);

    virtual Laxkit::flatpoint screentoreal(int x,int y);
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual int Refresh();
	virtual void DrawOutline(double width, double r,double g,double b);
	virtual void drawLinear();
	virtual void drawLinear2();
	virtual void drawRadial();
	virtual void drawRadial2();
	virtual void drawRadialLine(double t, bool thick);
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);

	virtual int scan(int x,int y);
	virtual int SelectPoint(int c);
	virtual void deletedata();
	virtual ObjectContext *Context();
	virtual Laxkit::flatpoint getpoint(int c, int trans);
	virtual int sendcolor(Laxkit::ScreenColor *col);
};

} // namespace LaxInterfaces;

#endif

