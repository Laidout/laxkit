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
//    Copyright (C) 2013 by Tom Lechner
//
#ifndef _LAX_COLORSLIDERS_H
#define _LAX_COLORSLIDERS_H


#include <lax/anxapp.h>
#include <lax/colorbase.h>
#include <lax/buttondowninfo.h>
#include <lax/rectangles.h>
#include <lax/lists.h>


namespace Laxkit {


//------------------------------------------- ColorSliders ----------------------------------

enum ColorSliderBlockType {
	COLORBLOCK_RGB,
	COLORBLOCK_CMYK,
	COLORBLOCK_HSV,
	COLORBLOCK_HSL,
	COLORBLOCK_Alpha,
	COLORBLOCK_Gray,
	COLORBLOCK_Hue,
	COLORBLOCK_CieLAB, //unimplemented
	COLORBLOCK_XYZ,   //unimplemented
	COLORBLOCK_MAX
};

enum ColorSliderType {
	COLORSLIDER_Red,
	COLORSLIDER_Green,
	COLORSLIDER_Blue,
	COLORSLIDER_Cyan,
	COLORSLIDER_Magenta,
	COLORSLIDER_Yellow,
	COLORSLIDER_Black,
	COLORSLIDER_Hue,
	COLORSLIDER_Saturation,
	COLORSLIDER_Value,
	COLORSLIDER_Lightness,
	COLORSLIDER_Transparency,
	COLORSLIDER_MAX
};

enum ColorSlidersStyle {
	COLORSLIDERS_Vertical       = (1<<16),
	COLORSLIDERS_HideOldNew     = (1<<17),
	COLORSLIDERS_VerticalOldNew = (1<<18),
	COLORSLIDERS_HideHex        = (1<<19),
	COLORSLIDERS_MAX
};

class ColorBarInfo
{
  public:
	int id;
	int type; //usually one of ColorSliderType
	int hidden;
	double pos;
	ColorBarInfo(int nid,int ntype,double npos=0);
};

class ColorSliders : public anXWindow, public ColorBase
{
  protected:
	ButtonDownInfo buttondown;
	IntRectangle bwcolor, hue;
	IntRectangle sliders, hex, oldnew;
	ScreenColor curcolor,original_color;
	int inputpreference;

	PtrStack<ColorBarInfo> bars;
	int current, currenthalf;
	int mouseshape;

	virtual int send();
	
  public:
	int gap;
	double step;

	ColorSliders(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int ctype, int nmax, int nstep,
			 int c0,int c1,int c2,int c3=-1,int c4=-1);
	virtual ~ColorSliders();
	virtual const char *whattype() { return "ColorSliders"; }
	virtual int init();
	virtual void updateSliderRect();
	virtual int Resize(int nw,int nh);
	virtual int MoveResize(int nx,int ny,int nw,int nh);

	virtual void Refresh();
	virtual void DrawVertical(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h,double pos,const char *text, int usealpha);
	virtual void DrawHorizontal(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h,double pos,const char *text, int usealpha);
	virtual void DrawPos(int x,int y,int w,int h, double pos);
	virtual void DrawOldNew(int x,int y,int w,int h, int horiz);

	virtual int GetPos(int x,int y, double *pos, int *half);
	virtual int LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MouseMove(int mx,int my, unsigned int state, const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d);
	virtual int Event(const EventData *e,const char *mes);

	virtual void Updated();
	virtual void SetBar(int whichbar, double pos);
	virtual double GetPosForBar(int whichbar);
	virtual int FindBar(int type);
	virtual int EditHex();
};




} // namespace Laxkit

#endif 


