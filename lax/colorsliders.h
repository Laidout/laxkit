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
	COLORBLOCK_Alpha  =(1<<0),
	COLORBLOCK_RGB    =(1<<1),
	COLORBLOCK_CMYK   =(1<<2),
	COLORBLOCK_HSV    =(1<<3),
	COLORBLOCK_HSL    =(1<<4),
	COLORBLOCK_Gray   =(1<<5),
	COLORBLOCK_CieLAB =(1<<6),
	COLORBLOCK_XYZ    =(1<<7),
	COLORBLOCK_MAXBIT = 7
};

enum ColorSliderType {
	COLORSLIDER_Unknown=0,
	COLORSLIDER_Red,
	COLORSLIDER_Green,
	COLORSLIDER_Blue,
	COLORSLIDER_Gray,
	COLORSLIDER_Cyan,
	COLORSLIDER_Magenta,
	COLORSLIDER_Yellow,
	COLORSLIDER_Black,
	COLORSLIDER_HSV_Hue,
	COLORSLIDER_HSV_Saturation,
	COLORSLIDER_HSV_Value,
	COLORSLIDER_HSL_Hue,
	COLORSLIDER_HSL_Saturation,
	COLORSLIDER_HSL_Lightness,
	COLORSLIDER_Cie_L,
	COLORSLIDER_Cie_a,
	COLORSLIDER_Cie_b,
	COLORSLIDER_X,
	COLORSLIDER_Y,
	COLORSLIDER_Z,
	COLORSLIDER_Transparency,

	COLORSLIDER_Knockout,
	COLORSLIDER_Registration,
	COLORSLIDER_None,
	COLORSLIDER_MAX
};

enum ColorSlidersStyle {
	COLORSLIDERS_Vertical          = (1<<16),
	COLORSLIDERS_HideOldNew        = (1<<17),
	COLORSLIDERS_VerticalOldNew    = (1<<18),
	COLORSLIDERS_HideHex           = (1<<19),
	COLORSLIDERS_Allow_None        = (1<<20),
	COLORSLIDERS_Allow_Registration= (1<<21),
	COLORSLIDERS_Allow_Knockout    = (1<<22),
	COLORSLIDERS_SpecialMask       = ((1<<20)|(1<<21)|(1<<22)), 
	COLORSLIDERS_Recent            = (1<<24), //unimplemented
	COLORSLIDERS_FG_and_BG         = (1<<25), //unimplemented
	COLORSLIDERS_Done_Button       = (1<<26), //unimplemented
	COLORSLIDERS_MAX
};

//----------------------------ColorBlockInfo
class ColorBlockInfo
{
  public:
	int type;
	char *name;
	bool hidden;
	int is_output_type; //-1 means output cannot be this type (like pure alpha)
	ColorBlockInfo(int type, const char *nname, int isoutput, bool nhidden);
	~ColorBlockInfo();
};

//----------------------------ColorBarInfo
class ColorBarInfo
{
  public:
	int id;
	int system;
	int type; //usually one of ColorSliderType
	int hidden;
	double pos;
	char *text;
	ColorBarInfo(int nid,int nsystem,int ntype,double npos=0, const char *ntext=NULL);
	~ColorBarInfo();
};

class ColorSliders : public anXWindow, public ColorBase
{
  protected:
	ButtonDownInfo buttondown;
	IntRectangle bwcolor, hue;
	IntRectangle sliders, hex, oldnew, specials;
	ScreenColor curcolor,original_color;
	int sendtype;

	PtrStack<ColorBlockInfo> systems;
	PtrStack<ColorBarInfo> bars;
	int current, currenthalf;
	int mouseshape;

	virtual int DefineSystems(int which);
	virtual int DefineBars();
	virtual int send();
	virtual bool useSpecialLine();
	virtual void DrawSpecial(int which, int x,int y,int w,int h);
	
  public:
	int square;
	int gap;
	double step;

	ColorSliders(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int ctype, double nstep,
			 double c0,double c1,double c2,double c3=-1,double c4=-1,
			 double nearx=-1, double neary=-1);
	virtual ~ColorSliders();
	virtual const char *whattype() { return "ColorSliders"; }
	virtual int init();
	virtual void updateSliderRect();
	virtual int Resize(int nw,int nh);
	virtual int MoveResize(int nx,int ny,int nw,int nh);

	virtual void Refresh();
	virtual void DrawVertical(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h,double pos,const char *text, int usealpha);
	virtual void DrawHorizontal(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h,double pos,const char *text, int usealpha);
	virtual void FillWithTransparency(ScreenColor &color, int x,int y,int w,int h);
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


