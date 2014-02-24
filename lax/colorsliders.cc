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


#include <lax/colorsliders.h>
#include <lax/laxutils.h>
#include <lax/misc.h>
#include <lax/language.h>
#include <lax/drawingdefs.h>
#include <lax/mouseshapes.h>
#include <lax/lineedit.h>

#include <lax/lists.cc>



#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {




//return values for GetPos()
#define ONPOS_Hex  -2
#define ONPOS_Old  -3
#define ONPOS_New  -4

//------------------------------------- ColorSliders -------------------------------
/*! \class ColorSliders
 * \brief Panel of sliders to control various color channels.
 *
 * \todo Can toggle on or off groups of rgb, hsv, cmyk.
 * Optional transparency.
 */


ColorBarInfo::ColorBarInfo(int nid,int ntype,double npos)
{
	id=nid;
	type=ntype;
	pos=npos;
	hidden=0;
}

ColorSliders::ColorSliders(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int ctype, int nmax, int nstep,
			 int c0,int c1,int c2,int c3,int c4)
  : anXWindow(parnt,nname,ntitle,ANXWIN_DOUBLEBUFFER|nstyle,nx,ny,nw,nh,brder,prev,owner,mes),
	ColorBase(ctype,nmax,c0,c1,c2,c3,c4)
{
	step=(double)nstep/max;
	gap=5;
	current=-1;
	currenthalf=0;

	int i=1;
	 //rgb
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Red, Redf()),1);
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Green, Greenf()),1);
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Blue, Bluef()),1);
	 //cmyk
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Cyan, Cyanf()),1);
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Magenta, Magentaf()),1);
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Yellow, Yellowf()),1);
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Black, Blackf()),1);
	 //hsv
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Hue,Huef()),1);
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Saturation,Saturationf()),1);
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Value,Valuef()),1);
	 //alpha
	bars.push(new ColorBarInfo(i++, COLORSLIDER_Transparency,Alphaf()),1);

	installColors(app->color_panel);

	mouseshape=0;
}

ColorSliders::~ColorSliders()
{
}

int ColorSliders::init()
{
	updateSliderRect();
	return 0;
}

void ColorSliders::Updated()
{
	curcolor.rgbf(Redf(),Greenf(),Bluef(),Alphaf());
	needtodraw=1;
}

int ColorSliders::send()
{
	if (!win_owner || !win_sendthis) return 0;
	DBG cerr <<" SEND "<<win_sendthis<<" to "<<win_owner<<endl;

    SimpleColorEventData *cevent=NULL;
	int currentid=0;
    if (colortype==LAX_COLOR_RGB) cevent=new SimpleColorEventData(max,Red(),Green(),Blue(),Alpha(),currentid);
    else if (colortype==LAX_COLOR_GRAY) cevent=new SimpleColorEventData(max,Gray(),Alpha(),currentid);
    else cevent=new SimpleColorEventData(max,Cyan(),Magenta(),Yellow(),Black(),Alpha(),currentid);

    app->SendMessage(cevent, win_owner,win_sendthis, object_id);

    return 1;
}


void ColorSliders::Refresh()
{
	if (!needtodraw || !win_on) return;
	needtodraw=0;

	clear_window(this);
	if (!bars.n) return;

	double pos;
	ScreenColor color1, color2;
	int x=sliders.x+gap,y=sliders.y+gap;
	double w=sliders.width-2*gap;
	double h=sliders.height-2*gap;


	if (win_style&COLORSLIDERS_Vertical) {
		w-=gap*(bars.n-1);
		w/=bars.n;
	} else {
		h-=gap*(bars.n-1);
		h/=bars.n;
	}

	 //paint highlighted current in gap
	if (current>=0) {
		foreground_color(coloravg(win_colors->fg,win_colors->bg,.8));
		int xx=x, yy=y;
		if (win_style&COLORSLIDERS_Vertical) {
			xx+=current*(w+gap);
		} else {
			yy+=current*(h+gap);
		}

		fill_rectangle(this, xx-gap,yy-gap, w+2*gap,h+2*gap);

	} else if (current==ONPOS_Hex) {
		foreground_color(coloravg(win_colors->fg,win_colors->bg,.8));
		fill_rectangle(this, hex.x-gap,hex.y-gap,hex.width+2*gap,hex.height+2*gap);

	} else if (current==ONPOS_Old || current==ONPOS_New) {
		foreground_color(coloravg(win_colors->fg,win_colors->bg,.8));
		fill_rectangle(this, oldnew.x-gap,oldnew.y-gap,oldnew.width+2*gap,oldnew.height+2*gap);
	}


	double rgb[3];  RGB(rgb);
	double cmyk[4]; CMYK(cmyk);
	double hsv[3];  HSV(hsv);
	double tt[4];
	double alpha=Alphaf();
	const char *text=NULL;
	for (int c=0; c<bars.n; c++) {
		
		if (win_style&COLORSLIDERS_Vertical) {
			x=gap+(w+gap)*c;
			y=gap;
		} else {
			x=gap;
			y=gap+(h+gap)*c;
		}

		 //need to set pos and colors
		if (bars.e[c]->type==COLORSLIDER_Red) {
			pos=Redf();
			color1.rgbf(0,rgb[1],rgb[2],alpha);
			color2.rgbf(1,rgb[1],rgb[2],alpha);
			text=_("Red");

		} else if (bars.e[c]->type==COLORSLIDER_Green) {
			pos=Greenf();
			color1.rgbf(rgb[0],0,rgb[2],alpha);
			color2.rgbf(rgb[0],1,rgb[2],alpha);
			text=_("Green");

		} else if (bars.e[c]->type==COLORSLIDER_Blue) {
			pos=Bluef();
			color1.rgbf(rgb[0],rgb[1],0,alpha);
			color2.rgbf(rgb[0],rgb[1],1,alpha);
			text=_("Blue");

		} else if (bars.e[c]->type==COLORSLIDER_Cyan) {
			pos=Cyanf();
			cmyk[0]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[0]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[0]=pos;
			text=_("Cyan");

		} else if (bars.e[c]->type==COLORSLIDER_Magenta) {
			pos=Magentaf();
			cmyk[1]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[1]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[1]=pos;
			text=_("Magenta");

		} else if (bars.e[c]->type==COLORSLIDER_Yellow) {
			pos=Yellowf();
			cmyk[2]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[2]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[2]=pos;
			text=_("Yellow");

		} else if (bars.e[c]->type==COLORSLIDER_Black) {
			pos=Blackf();
			cmyk[3]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[3]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[3]=pos;
			text=_("Black");

		} else if (bars.e[c]->type==COLORSLIDER_Saturation) {
			pos=Saturationf();
			hsv[1]=0; simple_hsv_to_rgb(hsv,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[1]=1; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[1]=pos;
			text=_("Saturation");

		} else if (bars.e[c]->type==COLORSLIDER_Value) {
			pos=Valuef();
			hsv[2]=0; simple_hsv_to_rgb(hsv,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[2]=1; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[2]=pos;
			text=_("Value");

		} else if (bars.e[c]->type==COLORSLIDER_Transparency) {
			pos=alpha;
			color1.rgbf(rgb[0],rgb[1],rgb[2],0.);
			color2.rgbf(rgb[0],rgb[1],rgb[2],1.);
			text=_("Alpha");

		} else if (bars.e[c]->type==COLORSLIDER_Hue) {
			 //hue is special in that it is in 3 parts
			text=_("Hue");
			if (c!=current) text=NULL;
			pos=Huef();

			 //segment 1
			hsv[0]=0; simple_hsv_to_rgb(hsv,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[0]=1./3; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y, w,h/3+1, pos,text);
			else DrawHorizontal(color1,color2, x,y, w/3+1,h, -1,text);

			 //segment 2
			color1=color2;
			hsv[0]=2./3; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y+h/3, w,h/3+1, pos,text);
			else DrawHorizontal(color1,color2, x+w/3,y, w/3+1,h, -1,text);

			 //segment 3
			color1=color2;
			hsv[0]=1.; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y+h*2./3, w,h/3, -1,text);
			else DrawHorizontal(color1,color2, x+w*2./3,y, w/3,h, -1,text);

			DBG cerr <<" Hue pos:"<<pos<<" xywh:"<<x<<" "<<y<<" "<<w<<" "<<h<<endl;
			DrawPos(x,y,w,h, pos);

			hsv[0]=pos;
			continue;
		}

		if (c!=current) text=NULL;
		if (win_style&COLORSLIDERS_Vertical) {
			DrawVertical(color1,color2, x,y, w,h, pos,text);
		} else {
			DrawHorizontal(color1,color2, x,y, w,h, pos,text);
		}
	}

	if (!(win_style&COLORSLIDERS_HideHex)) {
		char str[20];
		foreground_color(win_colors->fg);
		textout(this, HexValue(str),-1, hex.x+hex.width/2,hex.y+hex.height/2, LAX_CENTER);
	}

	if (!(win_style&COLORSLIDERS_HideOldNew)) {
		int *ccolor=colors;
		colors=oldcolor;
		foreground_color(Redf(),Greenf(),Bluef());
		fill_rectangle(this, oldnew.x,oldnew.y,oldnew.width/2,oldnew.height);
		colors=ccolor;
		foreground_color(Redf(),Greenf(),Bluef());
		fill_rectangle(this, oldnew.x+oldnew.width/2,oldnew.y,oldnew.width/2,oldnew.height);
	}
	SwapBuffers();
}

//! Each bar is drawn horizontally.
/*! pos is [0..1].
 */
void ColorSliders::DrawHorizontal(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h, double pos,const char *text)
{
//	if (color1.alpha!=65535 || color2.alpha!=65535) {
//		 //need to draw transparency backdrop
//		int tt=10;
//		for (int xx=x; xx<x+w; xx+=tt) {
//			for (int yy=y; yy<y+h; yy+=tt) {
//				if (xx*yy%2==0) foreground_color(.6,.6,.6); else foreground_color(.3,.3,.3);
//				fill_rectangle(this, xx,yy,tt,tt); // *** need to adjust for edges
//			}
//		}
//	}

	 //draw color
	double pp;
	ScreenColor color;
	for (int c=x; c<x+w; c++) {
		pp=(double)(c-x)/w;
		coloravg(&color, &color1,&color2,pp);
		foreground_color(pixelfromcolor(&color));
		draw_line(this, c,y, c,y+h);
	}

	 //draw pos
	DrawPos(x,y,w,h,pos);

	 //text
	if (text) {
		foreground_color(~0);
		drawing_function(LAXOP_Xor);
		textout(this, text,-1, gap,y, LAX_TOP|LAX_LEFT);
		drawing_function(LAXOP_Over);
	}
}

void ColorSliders::DrawPos(int x,int y,int w,int h, double pos)
{
	pos*=w;

	 //draw pos
	if (pos>=0) {
		foreground_color(0);
		draw_line(this, gap+pos,y, gap+pos,y+h);
		foreground_color(~0);
		draw_line(this, gap+pos+1,y, gap+pos+1,y+h);
	}
}

void ColorSliders::DrawOldNew(int x,int y,int w,int h, int horiz)
{
	int *ccolor=colors;

	colors=oldcolor;
	ScreenColor(Redf(),Greenf(),Bluef(),Alphaf());

	if (horiz) fill_rectangle(this, x,y,w/2,h);
	else fill_rectangle(this, x,y,w,h/2);

	colors=ccolor;
	ScreenColor(Redf(),Greenf(),Bluef(),Alphaf());

	if (horiz) fill_rectangle(this, x+w/2,y,w/2,h);
	else fill_rectangle(this, x,y+h/2,w,h/2);
}

void ColorSliders::DrawVertical(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h, double pos,const char *text)
{
//	if (color1.alpha!=65535 || color2.alpha!=65535) {
//		 //need to draw transparency backdrop
//		int tt=10;
//		for (int xx=x; xx<x+w; xx+=tt) {
//			for (int yy=y; yy<y+h; yy+=tt) {
//				if (xx*yy%2==0) foreground_color(.6,.6,.6); else foreground_color(.3,.3,.3);
//				fill_rectangle(this, xx,yy,tt,tt); // *** need to adjust for edges
//			}
//		}
//	}

	 //draw colors
	double pp;
	ScreenColor color;
	for (int c=y; c<y+h; y++) {
		coloravg(&color, &color1,&color2,pp);
		pp=(double)(c-y)/h;
		foreground_color(pixelfromcolor(&color));
		draw_line(this, x,c, x+w,c);
	}

	 //draw pos
	if (pos>=0) {
		foreground_color(0);
		draw_line(this, x,gap+pos, x+w,gap+pos);
		foreground_color(~0);
		draw_line(this, x,gap+pos, x+w,gap+pos+1);
	}
}


/*! Screen coordinates x,y.
 * half is 1 for upper half (absolute placement) or 0 for lower (for panning).
 * 
 * Return -1 for not on anything.
 * Return -2 for hex, -3 for old, or -4 for new.
 */
int ColorSliders::GetPos(int x,int y, double *pos, int *half)
{
	if (hex.pointIsIn(x,y)) { *half=-1; return ONPOS_Hex; }
	if (oldnew.pointIsIn(x,y)) {
		*half=-1;
		if (x<oldnew.x+oldnew.width/2) return ONPOS_Old;
		return ONPOS_New;
	}

	double bar=-1;
	x-=sliders.x;
	y-=sliders.y;

	if (win_style&COLORSLIDERS_Vertical) {
		bar=x/((double)(sliders.width/bars.n));
		*pos=(y-gap)/((double)sliders.height-2*gap);
	} else {
		bar=y/((double)sliders.height/bars.n);
		*pos=(x-gap)/((double)sliders.width-2*gap);
	}
	*half=((bar-int(bar))<.5);
	if (bar<0 || bar>=bars.n) { *half=-1; return -1; }
	return bar;
}

int ColorSliders::LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	if (_kids.n) app->destroywindow(_kids.e[0]); //we are assuming this is a number edit

	double pos;
	int half=0;
	int bar=GetPos(x,y, &pos,&half);
	buttondown.down(d->id,LEFTBUTTON, x,y, bar,half);

	if (half) {
		 //drag upper half to position absolutely
		DBG cerr << "slider lbd  bar:"<<bar<<"  pos:"<<pos<<endl;
		SetBar(bar,pos);
		send();
		needtodraw=1;
		return 0;
	}

	return 0;
}

//! Respond to "hex" color event.
int ColorSliders::Event(const EventData *e,const char *mes)
{
	if (!strcmp(mes,"hex")) {
		 // apply message as new current color, pass on to viewport
		const SimpleMessage *ce=dynamic_cast<const SimpleMessage *>(e);
		if (!ce) return 0;

		SetHexValue(ce->str);
		send();
		needtodraw=1;
		return 0;
	}
	return anXWindow::Event(e,mes);
}

//! Start little window to edit the hex value
int ColorSliders::EditHex()
{
	char str[20];
	int border=3;
	LineEdit *le=new LineEdit(this, "edithex",NULL,LINEEDIT_DESTROY_ON_ENTER|LINEEDIT_GRAB_ON_MAP|ANXWIN_ESCAPABLE,
							  hex.x-border,hex.y-border,hex.width,hex.height,border,
							  NULL,object_id,"hex",
							  HexValue(str),0);
	app->addwindow(le);
	return 0;
}

int ColorSliders::LBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	int bar,half;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &bar,&half);
	if (!dragged && (bar==ONPOS_Old || bar==ONPOS_Hex)) {
		if (bar==ONPOS_Old) {
			RestoreColor();
			send();
		} else EditHex();
	}
	return 0;
}

int ColorSliders::MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	if (_kids.n) app->destroywindow(_kids.e[0]); //we are assuming this is a number edit
	return 0;
}

int ColorSliders::MBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	return 0;
}

int ColorSliders::RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	if (_kids.n) app->destroywindow(_kids.e[0]); //we are assuming this is a number edit
	return 0;
}

int ColorSliders::RBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	return 0;
}

int ColorSliders::MouseMove(int mx,int my, unsigned int state, const LaxMouse *d)
{
	double pos;
	int half;
	int bar=GetPos(mx,my, &pos,&half);
	DBG cerr <<" sliders mm--- bar: "<<bar<<  "pos:"<<pos<<"  half:"<<half<<endl;

	if (!buttondown.any()) {
		if (bar!=current) {
			current=bar;
			needtodraw=1;
		}
		int shape=0;
		if (half!=currenthalf) {
			if (win_style&COLORSLIDERS_Vertical) {
				if (half) shape=LAX_MOUSE_Right;
				else shape=LAX_MOUSE_UpDown;
			} else {
				if (half) shape=LAX_MOUSE_Up;
				else shape=LAX_MOUSE_LeftRight;
			}
			currenthalf=half;
			if (half<0) shape=0;
			const_cast<LaxMouse*>(d)->setMouseShape(this,shape);
		}
		return 0;
	}

	//now deal with button drags



	int oldbar=-1, oldhalf=-1;
	if (buttondown.isdown(d->id,LEFTBUTTON, &oldbar, &oldhalf)) {
		int oldx,oldy;
		buttondown.move(d->id, mx,my, &oldx,&oldy);
		if (oldhalf) {
			 //drag upper half to position absolutely
			DBG cerr << "slider lbd  bar:"<<oldbar<<"  pos:"<<pos<<endl;
			SetBar(oldbar,pos);
			send();
			needtodraw=1;
			return 0;
		} else {
			 //drag lower half to drag position by step
			double oldpos=GetPosForBar(oldbar);
			if (win_style&COLORSLIDERS_Vertical) oldpos+=step*(my-oldy);
			else oldpos+=step*(mx-oldx);
			DBG cerr << "slider lbd  bar:"<<oldbar<<"  pos:"<<oldpos<<endl;
			SetBar(oldbar,oldpos);
			send();
			needtodraw=1;
			return 0;
		}
	}

	return 0;
}

int ColorSliders::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (_kids.n) app->destroywindow(_kids.e[0]); //we are assuming this is a number edit

	double s=step;
	if ((state&LAX_STATE_MASK)==ShiftMask || (state&LAX_STATE_MASK)==ControlMask) s*=10;
	else if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) s*=30;
	double pos=GetPosForBar(current);
	if (!pos<0) return 0;
	pos+=s;
	SetBar(current,pos);
	send();
	needtodraw=1;
	return 0;
}

int ColorSliders::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (_kids.n) app->destroywindow(_kids.e[0]); //we are assuming this is a number edit

	double s=-step;
	if ((state&LAX_STATE_MASK)==ShiftMask || (state&LAX_STATE_MASK)==ControlMask) s*=10;
	else if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) s*=30;
	double pos=GetPosForBar(current);
	if (!pos<0) return 0;
	pos+=s;
	SetBar(current,pos);
	send();
	needtodraw=1;
	return 0;
}

/*! pos is clamped to [0..1].
 */
void ColorSliders::SetBar(int whichbar, double pos)
{
	if (whichbar<0 || whichbar>=bars.n) return;

	if (pos<0) pos=0; else if (pos>1) pos=1;
	bars.e[whichbar]->pos=pos;

	if      (bars.e[whichbar]->type==COLORSLIDER_Red)  Redf(pos);
	else if (bars.e[whichbar]->type==COLORSLIDER_Green) Greenf(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Blue)   Bluef(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Cyan)   Cyanf(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Magenta) Magentaf(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Yellow)   Yellowf(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Black)     Blackf(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Hue)        Huef(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Saturation) Saturationf(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Value)       Valuef(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Transparency) Alphaf(pos); 
}

//! Return what the bar->pos should be, not what it actually contains.
/*! Returns -1 for no such bar.
 */
double ColorSliders::GetPosForBar(int whichbar)
{
	if (whichbar<0 || whichbar>=bars.n) return -1;

	if      (bars.e[whichbar]->type==COLORSLIDER_Red)  return Redf();
	else if (bars.e[whichbar]->type==COLORSLIDER_Green) return Greenf(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Blue)   return Bluef(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Cyan)   return Cyanf(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Magenta) return Magentaf(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Yellow)   return Yellowf(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Black)     return Blackf(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Hue)        return Huef(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Saturation) return Saturationf(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Value)       return Valuef(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Transparency) return Alphaf(); 
	return -1;
}

int ColorSliders::FindBar(int type)
{
	for (int c=0; c<bars.n; c++) if (bars.e[c]->type==type) return c;
	return -1;
}

int ColorSliders::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d)
{

	if (ch==LAX_Up) {
		current--;
		if (current<0) current=bars.n-1;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Down) {
		current++;
		if (current>=bars.n) current=0;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Left) {
		if (current<0) return 0;
		double pos=GetPosForBar(current);
		if (pos<0) return 0;
		if ((state&LAX_STATE_MASK)==ControlMask) pos-=.1;
		else pos-=.01;
		if (pos<0) pos=0;
		SetBar(current, pos);
		send();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Right) {
		if (current<0) return 0;
		double pos=GetPosForBar(current);
		if (pos<0) return 0;
		if ((state&LAX_STATE_MASK)==ControlMask) pos+=.1;
		else pos+=.01;
		if (pos>1) pos=1;
		SetBar(current, pos);
		send();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_End) {
		if (current<0) return 0;
		double pos=GetPosForBar(current);
		if (pos<0) return 0;
		pos=1;
		SetBar(current,pos);
		send();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Home) {
		if (current<0) return 0;
		double pos=GetPosForBar(current);
		if (pos<0) return 0;
		pos=0;
		SetBar(current,pos);
		send();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Bksp) {
		RestoreColor();
		return 0;
	}

	if        (ch=='r') { current=FindBar(COLORSLIDER_Red);     needtodraw=1; return 0;
	} else if (ch=='g') { current=FindBar(COLORSLIDER_Green);   needtodraw=1; return 0;
	} else if (ch=='b') { current=FindBar(COLORSLIDER_Blue);    needtodraw=1; return 0;
	} else if (ch=='c') { current=FindBar(COLORSLIDER_Cyan);    needtodraw=1; return 0;
	} else if (ch=='m') { current=FindBar(COLORSLIDER_Magenta);  needtodraw=1; return 0;
	} else if (ch=='y') { current=FindBar(COLORSLIDER_Yellow);   needtodraw=1; return 0;
	} else if (ch=='k') { current=FindBar(COLORSLIDER_Black);     needtodraw=1; return 0;
	} else if (ch=='h') { current=FindBar(COLORSLIDER_Hue);       needtodraw=1; return 0;
	} else if (ch=='s') { current=FindBar(COLORSLIDER_Saturation); needtodraw=1; return 0;
	} else if (ch=='v') { current=FindBar(COLORSLIDER_Value);       needtodraw=1; return 0;
	} else if (ch=='a') { current=FindBar(COLORSLIDER_Transparency); needtodraw=1; return 0;
	}

	return anXWindow::CharInput(ch,buffer,len,state,d);
}

//! Respond to window resizes by updating where sliders, hex and old new are drawn in.
/*! Meaning, change the IntRectangles sliders, hex, and oldnew.
 */
void ColorSliders::updateSliderRect()
{
	sliders.x=0; //do not include gap
	sliders.y=0;
	sliders.width=win_w;
	sliders.height=win_h;

	double hexh=text_height();
	double hexw=getextent("#00000000",-1, NULL,NULL)+2*gap;

	hex.width=oldnew.width=-1;
	if (win_style&COLORSLIDERS_Vertical) {
		sliders.width-=2*gap;
		//*** put in hex and oldnew

	} else {
		if ((win_style&(COLORSLIDERS_HideHex|COLORSLIDERS_HideOldNew))==0) 
			sliders.height-=1.5*gap+hexh;

		if (!(win_style&COLORSLIDERS_HideHex)) {
			hex.y     =sliders.y+sliders.height+gap/2;
			hex.height=hexh;
			if (win_style&COLORSLIDERS_HideOldNew) {
				hex.x     =sliders.x+gap;
				hex.width =sliders.width-2*gap;
			} else {
				hex.x=sliders.x+sliders.width-gap-hexw;
				hex.width=hexw;
			}
		}
		if (!(win_style&COLORSLIDERS_HideOldNew)) {
			oldnew.y     =sliders.y+sliders.height+gap/2;
			oldnew.height=hexh;

			if (win_style&COLORSLIDERS_HideHex) {
				 //no hex
				oldnew.x     =sliders.x+2*gap;
				oldnew.width =sliders.width-4*gap;
			} else {
				 // with hex
				oldnew.x     =sliders.x+gap;
				oldnew.width =sliders.width-3*gap-hexw;
			}

		}
//		------
//		sliders.height-=gap*(bars.n-1);
//		sliders.height-=oldnewheight+hexh;
//		if (hexh) {
//			sliders.height-=gap;
//			hex.x     =sliders.x;
//			hex.y     =sliders.y+sliders.height+gap;
//			hex.width =sliders.width;
//			hex.height=hexh;
//		}
//		if (oldnewheight) {
//			sliders.height-=gap;
//			oldnew.x     =sliders.x;
//			oldnew.y     =sliders.y+yh-oldnewheight;
//			oldnew.width =sliders.width;
//			oldnew.height=oldnewheight;
//		}
	}
}

//! Calls anXWindow::Resize, then Sync(0).
int ColorSliders::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	updateSliderRect();
	needtodraw=1;
	return 0;
}

//! Calls anXWindow::MoveResize, then Sync(0).
int ColorSliders::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	updateSliderRect();
	needtodraw=1;
	return 0;
}

} // namespace Laxkit




