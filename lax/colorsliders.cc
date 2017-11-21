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
//    Copyright (C) 2013-2017 by Tom Lechner
//


#include <lax/colorsliders.h>
#include <lax/colorspace.h>
#include <lax/laxutils.h>
#include <lax/misc.h>
#include <lax/language.h>
#include <lax/drawingdefs.h>
#include <lax/mouseshapes.h>
#include <lax/lineedit.h>
#include <lax/strmanip.h>

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


//------------------------------------- ColorBlockInfo 

/*! \class ColorBlockInfo
 * 
 * *** At some point, this class will be be merged somehow with ColorSystem.
 * In the meantime, is in an info node for ColorSlider.
 */
ColorBlockInfo::ColorBlockInfo(int ntype, const char *nname, int isoutput, bool nhidden)
{
	type=ntype;
	name=newstr(nname);
	is_output_type=isoutput;
	hidden=nhidden;
}

ColorBlockInfo::~ColorBlockInfo()
{
	delete[] name;
}



//------------------------------------- ColorBarInfo 

/*! \class ColorBarInfo
 * Used in ColorSlider. npos should be normalized to range [0..1].
 */
ColorBarInfo::ColorBarInfo(int nid,int nsystem,int ntype,double npos, const char *ntext)
{
	id=nid;
	system=nsystem;
	type=ntype;
	pos=npos;
	hidden=0;
	text=newstr(ntext);
}

ColorBarInfo::~ColorBarInfo()
{
	delete[] text;
}


//------------------------------------- ColorSliders 

/*! nstep is what fraction out of range to move on a single shift, like a pan event or wheel movement.
 *
 * If nearx>0 and neary>0, then try to place the window near that screen postiion.
 */
ColorSliders::ColorSliders(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int ctype, double nstep,
			 double c0,double c1,double c2,double c3,double c4,
			 double nearx, double neary)
  : anXWindow(parnt,nname,ntitle,ANXWIN_DOUBLEBUFFER|nstyle,nx,ny,nw,nh,brder,prev,owner,mes),
	ColorBase(ctype, c0,c1,c2,c3,c4)
{
	sendtype=ctype; //sliding cmyk/rgb does odd things on conversion, making sliders jump terribly
						   //this lets one shift input style based on which sliders are last moved, 
						   //but sends color message based on sendtype.

	step=nstep;
	gap=5;
	current=-1;
	currenthalf=0;
	mouseshape=0;
	square=10;

	if (nw<=0) nw=200;
	if (nh<=0) nh=400;

	DefineSystems(COLORBLOCK_RGB|COLORBLOCK_CMYK|COLORBLOCK_HSL|COLORBLOCK_Alpha);
	DefineBars();

	installColors(app->color_panel);


	 //try to place window near (nearx,neary)
	if (nearx>=0 && neary>=0) {
		int sw,sh;
		app->ScreenInfo(0, NULL,NULL, &sw,&sh, NULL,NULL,NULL,NULL);

		nx=nearx-nw-10;
		if (nx<0) {
			nx=nearx+10;
		}
		if (nx+nw>=sw) {
			nx=sw-nw;
			if (nx<0) {
				nx=0;
				win_w=sw;
			}
		}

		ny=neary-nh/2;
		if (ny<0) {
			ny=0;
		}
		if (ny>=sh) {
			ny=sh-nh;
			if (ny<0) {
				ny=0;
				win_h=sh;
			}
		}

		win_x=nx;
		win_y=ny;
	}
}

ColorSliders::~ColorSliders()
{
}

/*! Convenience function to check if any of:
 *  COLORSLIDERS_Allow_None        
 *  COLORSLIDERS_Allow_Registration
 *  COLORSLIDERS_Allow_Knockout    
 *  are active.
 */

bool ColorSliders::useSpecialLine()
{
	return win_style&( COLORSLIDERS_Allow_None        
					| COLORSLIDERS_Allow_Registration
					| COLORSLIDERS_Allow_Knockout    
			);
}


/*! If systems.n==0, then install all color systems.
 * Else just update the hidden field of each system according to if it is flagged in which.
 * To actually define the color bar instances, use DefineBars. The bars stack is not touched here.
 *
 * which is an or'd list of ColorSliderBlockType values.
 */
int ColorSliders::DefineSystems(int which)
{
	if (!systems.n) {
		systems.push(new ColorBlockInfo(COLORBLOCK_RGB,    _("RGB"),      0, !(which & COLORBLOCK_RGB   )));
		systems.push(new ColorBlockInfo(COLORBLOCK_CMYK,   _("CMYK"),     0, !(which & COLORBLOCK_CMYK  ))); 
		systems.push(new ColorBlockInfo(COLORBLOCK_HSV,    _("HSV"),      0, !(which & COLORBLOCK_HSV   ))); 
		systems.push(new ColorBlockInfo(COLORBLOCK_HSL,    _("HSL"),      0, !(which & COLORBLOCK_HSL   ))); 
		systems.push(new ColorBlockInfo(COLORBLOCK_Gray,   _("Gray"),     0, !(which & COLORBLOCK_Gray  ))); 
		systems.push(new ColorBlockInfo(COLORBLOCK_CieLAB, _("CieL*a*b*"),0, !(which & COLORBLOCK_CieLAB))); 
		systems.push(new ColorBlockInfo(COLORBLOCK_XYZ,    _("XYZ"),      0, !(which & COLORBLOCK_XYZ   ))); 
		systems.push(new ColorBlockInfo(COLORBLOCK_Alpha,  _("Alpha"),    0, !(which & COLORBLOCK_Alpha )));

	} else {
		for (int c=0; c<systems.n; c++) {
			systems.e[c]->hidden = (which&systems.e[c]->type);
		}
	} 

	return 0;
}

/*! Flushes bars, and readds according to if the relevant color system is hidden or not.
 */
int ColorSliders::DefineBars()
{
	bars.flush();
	int i=1;

	 //maybe implement reordering someday...

	for (int c=0; c<systems.n; c++) {
		if (systems.e[c]->hidden) continue;

		if (systems.e[c]->type==COLORBLOCK_RGB) {
			 //rgb
			bars.push(new ColorBarInfo(i++, COLORBLOCK_RGB, COLORSLIDER_Red,   Red(),  _("Red")),  1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_RGB, COLORSLIDER_Green, Green(),_("Green")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_RGB, COLORSLIDER_Blue,  Blue(), _("Blue")), 1);

		} else if (systems.e[c]->type==COLORBLOCK_CMYK) {
			 //cmyk
			bars.push(new ColorBarInfo(i++, COLORBLOCK_CMYK, COLORSLIDER_Cyan,    Cyan(),   _("Cyan")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_CMYK, COLORSLIDER_Magenta, Magenta(),_("Magenta")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_CMYK, COLORSLIDER_Yellow,  Yellow(), _("Yellow")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_CMYK, COLORSLIDER_Black,   Black(),  _("Black")),1);

		} else if (systems.e[c]->type==COLORBLOCK_HSV) {
			 //hsv
			bars.push(new ColorBarInfo(i++, COLORBLOCK_HSV, COLORSLIDER_HSV_Hue,       Hue(),       _("Hue")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_HSV, COLORSLIDER_HSV_Saturation,HSV_Saturation(),_("Saturation")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_HSV, COLORSLIDER_HSV_Value,     Value(),     _("Value")),1);

		} else if (systems.e[c]->type==COLORBLOCK_HSL) {
			 //hsl
			bars.push(new ColorBarInfo(i++, COLORBLOCK_HSL, COLORSLIDER_HSL_Hue,       Hue(),       _("Hue")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_HSL, COLORSLIDER_HSL_Saturation,HSL_Saturation(),_("Saturation")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_HSL, COLORSLIDER_HSL_Lightness, Lightness(), _("Lightness")),1);

		} else if (systems.e[c]->type==COLORBLOCK_Gray) {
			bars.push(new ColorBarInfo(i++, COLORBLOCK_Gray, COLORSLIDER_Gray,     Gray(), _("Gray")),1);

		} else if (systems.e[c]->type==COLORBLOCK_CieLAB) {
			bars.push(new ColorBarInfo(i++, COLORBLOCK_CieLAB, COLORSLIDER_Cie_L,  Cie_L(), _("Lightness*")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_CieLAB, COLORSLIDER_Cie_a,  Cie_a(), _("a*")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_CieLAB, COLORSLIDER_Cie_b,  Cie_b(), _("b*")),1);

		} else if (systems.e[c]->type==COLORBLOCK_XYZ) {
			bars.push(new ColorBarInfo(i++, COLORBLOCK_XYZ, COLORSLIDER_X,  X(), _("X")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_XYZ, COLORSLIDER_Y,  Y(), _("Y")),1);
			bars.push(new ColorBarInfo(i++, COLORBLOCK_XYZ, COLORSLIDER_Z,  Z(), _("Z")),1);

		} else if (systems.e[c]->type==COLORBLOCK_Alpha) {
			 //alpha
			bars.push(new ColorBarInfo(i++, COLORBLOCK_Alpha, COLORSLIDER_Transparency,Alpha(),_("Alpha")),1);
		}
	}

	return 0;
}

int ColorSliders::init()
{
	DefineSystems(COLORBLOCK_RGB |COLORBLOCK_CMYK |COLORBLOCK_HSL |COLORBLOCK_Alpha);

	updateSliderRect();
	return 0;
}

void ColorSliders::Updated()
{
	curcolor.rgbf(Red(),Green(),Blue(),Alpha());
	needtodraw=1;
}

/*! If sending SimpleColorEventData, then all channels are normalized to be in range [0..max].
 */
int ColorSliders::send()
{
	if (!win_owner || !win_sendthis) return 0;
	DBG cerr <<" ColorSliders SEND "<<win_sendthis<<" to "<<win_owner<<endl;

    SimpleColorEventData *cevent=NULL;
	int currentid=0;

    if (sendtype==LAX_COLOR_RGB)
		cevent=new SimpleColorEventData(max,max*Red(),max*Green(),max*Blue(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_GRAY)
		cevent=new SimpleColorEventData(max,max*Gray(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_CMYK)
		cevent=new SimpleColorEventData(max,max*Cyan(),max*Magenta(),max*Yellow(),max*Black(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_HSV)
		cevent=new SimpleColorEventData(max,max*Hue()/360,max*HSV_Saturation(),max*Value(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_HSL)
		cevent=new SimpleColorEventData(max,max*Hue()/360,max*HSL_Saturation(),max*Lightness(),max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_CieLAB)
		cevent=new SimpleColorEventData(max,max*Cie_L()/100,max*(Cie_a()+108)/216,max*(Cie_b()+108)/216,max*Alpha(),currentid);

    else if (sendtype==LAX_COLOR_XYZ)
		cevent=new SimpleColorEventData(max,max*X(),max*Y(),max*Z(),max*Alpha(),currentid);

	
	if (cevent==NULL) {
		DBG cerr <<" WARNING! Unknown color type: "<<sendtype<<endl;

	} else {
		cevent->colorspecial=colorspecial;
		cevent->colorsystem=sendtype;
		app->SendMessage(cevent, win_owner,win_sendthis, object_id);
	} 

    return 1;
}


void ColorSliders::Refresh()
{
	if (!needtodraw || !win_on) return;
	needtodraw=0;


	Displayer *dp=MakeCurrent();
	dp->ClearWindow();

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
		dp->NewFG(coloravg(win_colors->fg,win_colors->bg,.8));
		int xx=x, yy=y;
		if (win_style&COLORSLIDERS_Vertical) {
			xx+=current*(w+gap);
		} else {
			yy+=current*(h+gap);
		}

		dp->drawrectangle(xx-gap,yy-gap, w+2*gap,h+2*gap, 1);

	} else if (current==ONPOS_Hex) {
		dp->NewFG(coloravg(win_colors->fg,win_colors->bg,.8));
		dp->drawrectangle(hex.x-gap,hex.y-gap,hex.width+2*gap,hex.height+2*gap, 1);

	} else if (current==ONPOS_Old || current==ONPOS_New) {
		dp->NewFG(coloravg(win_colors->fg,win_colors->bg,.8));
		dp->drawrectangle(oldnew.x-gap,oldnew.y-gap,oldnew.width+2*gap,oldnew.height+2*gap, 1);
	}


	double rgb[3];  RGB(rgb);
	double cmyk[4]; CMYK(cmyk);
	double hsv[3];  HSV(hsv);
	double hsl[3];  HSL(hsl);
	double lab[3];  CieLab(lab);
	double xyz[3];  XYZ(xyz);

	double tt[4];
	double alpha=Alpha();
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
			pos=Red();
			color1.rgbf(0,rgb[1],rgb[2],alpha);
			color2.rgbf(1,rgb[1],rgb[2],alpha);

		} else if (bars.e[c]->type==COLORSLIDER_Green) {
			pos=Green();
			color1.rgbf(rgb[0],0,rgb[2],alpha);
			color2.rgbf(rgb[0],1,rgb[2],alpha);

		} else if (bars.e[c]->type==COLORSLIDER_Blue) {
			pos=Blue();
			color1.rgbf(rgb[0],rgb[1],0,alpha);
			color2.rgbf(rgb[0],rgb[1],1,alpha);

		} else if (bars.e[c]->type==COLORSLIDER_Cyan) {
			pos=Cyan();
			cmyk[0]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[0]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[0]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_Magenta) {
			pos=Magenta();
			cmyk[1]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[1]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[1]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_Yellow) {
			pos=Yellow();
			cmyk[2]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[2]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[2]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_Black) {
			pos=Black();
			cmyk[3]=0; simple_cmyk_to_rgb(cmyk,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[3]=1; simple_cmyk_to_rgb(cmyk,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			cmyk[3]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_HSV_Hue || bars.e[c]->type==COLORSLIDER_HSL_Hue) {
			 //hue is special in that it is in 3 parts
			text=bars.e[c]->text;
			if (c!=current) text=NULL;
			pos=Hue()/360;

			 //segment 1
			hsv[0]=0; simple_hsv_to_rgb(hsv,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[0]=1./3; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y, w,h/3+1, -1,NULL, 0);
			else DrawHorizontal(color1,color2, x,y, w/3+1,h, -1,text, 0);

			 //segment 2
			color1=color2;
			hsv[0]=2./3; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y+h/3, w,h/3+1, -1,NULL, 0);
			else DrawHorizontal(color1,color2, x+w/3,y, w/3+1,h, -1,NULL, 0);

			 //segment 3
			color1=color2;
			hsv[0]=1.; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y+h*2./3, w,h/3, -1,NULL, 0);
			else DrawHorizontal(color1,color2, x+w*2./3,y, w/3,h, -1,NULL, 0);

			DBG cerr <<" Hue pos:"<<pos<<" xywh:"<<x<<" "<<y<<" "<<w<<" "<<h<<endl;
			DrawPos(x,y,w,h, pos);

			hsv[0]=pos;
			continue;

		} else if (bars.e[c]->type==COLORSLIDER_HSV_Saturation) {
			pos=HSV_Saturation();
			hsv[1]=0; simple_hsv_to_rgb(hsv,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[1]=1; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[1]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_HSV_Value) {
			pos=Value();
			hsv[2]=0; simple_hsv_to_rgb(hsv,tt);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[2]=1; simple_hsv_to_rgb(hsv,tt);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			hsv[2]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_HSL_Saturation) {
			pos=HSL_Saturation();
			hsl[1]=0;  ColorConvert::Hsl2Rgb(&tt[0],&tt[1],&tt[2], hsl[0],hsl[1],hsl[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsl[1]=1;  ColorConvert::Hsl2Rgb(&tt[0],&tt[1],&tt[2], hsl[0],hsl[1],hsl[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			hsl[1]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_HSL_Lightness) {
			pos=Lightness();

			hsl[2]=.5;  ColorConvert::Hsl2Rgb(&tt[0],&tt[1],&tt[2], hsl[0],hsl[1],hsl[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsl[2]=1;  ColorConvert::Hsl2Rgb(&tt[0],&tt[1],&tt[2], hsl[0],hsl[1],hsl[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			hsl[2]=pos;

			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y+h/2, w,h/2, -1,NULL, 0);
			else DrawHorizontal(color1,color2, x+w/2,y, w/2,h, -1,NULL, 0);

			hsl[2]=0;  ColorConvert::Hsl2Rgb(&tt[0],&tt[1],&tt[2], hsl[0],hsl[1],hsl[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			hsl[2]=.5;  ColorConvert::Hsl2Rgb(&tt[0],&tt[1],&tt[2], hsl[0],hsl[1],hsl[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			hsl[2]=pos;

			if (win_style&COLORSLIDERS_Vertical) DrawVertical(color1,color2, x,y, w,h/2, -1,NULL, 0);
			else DrawHorizontal(color1,color2, x,y, w/2,h, -1,NULL, 0);

			text = bars.e[c]->text;
			if (c != current) text=NULL; 
			if (text) {
				dp->NewFG(standoutcolor(color1,1));
				dp->textout(gap,y, text,-1, LAX_TOP|LAX_LEFT);
			}

			DrawPos(x,y,w,h, pos);
			continue;

		} else if (bars.e[c]->type==COLORSLIDER_Cie_L) {
			pos=Cie_L()/100;
			lab[0]=0;  ColorConvert::Lab2Rgb(&tt[0],&tt[1],&tt[2], lab[0],lab[1],lab[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			lab[0]=1;  ColorConvert::Lab2Rgb(&tt[0],&tt[1],&tt[2], lab[0],lab[1],lab[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			lab[0]=pos;
				   
		} else if (bars.e[c]->type==COLORSLIDER_Cie_a) {
			pos=(Cie_a()+108)/216;
			lab[1]=0;  ColorConvert::Lab2Rgb(&tt[0],&tt[1],&tt[2], lab[0],lab[1],lab[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			lab[1]=1;  ColorConvert::Lab2Rgb(&tt[0],&tt[1],&tt[2], lab[0],lab[1],lab[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			lab[1]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_Cie_b) {
			pos=(Cie_b()+108)/216;
			lab[2]=0;  ColorConvert::Lab2Rgb(&tt[0],&tt[1],&tt[2], lab[0],lab[1],lab[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			lab[2]=1;  ColorConvert::Lab2Rgb(&tt[0],&tt[1],&tt[2], lab[0],lab[1],lab[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			lab[2]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_X) {
			pos=X();
			xyz[0]=0;  ColorConvert::Xyz2Rgb(&tt[0],&tt[1],&tt[2], xyz[0],xyz[1],xyz[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			xyz[0]=1;  ColorConvert::Xyz2Rgb(&tt[0],&tt[1],&tt[2], xyz[0],xyz[1],xyz[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			xyz[0]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_Y) {
			pos=Y();
			xyz[1]=0;  ColorConvert::Xyz2Rgb(&tt[0],&tt[1],&tt[2], xyz[0],xyz[1],xyz[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			xyz[1]=1;  ColorConvert::Xyz2Rgb(&tt[0],&tt[1],&tt[2], xyz[0],xyz[1],xyz[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			xyz[1]=pos;

		} else if (bars.e[c]->type==COLORSLIDER_Z) {
			pos=Z();
			xyz[2]=0;  ColorConvert::Xyz2Rgb(&tt[0],&tt[1],&tt[2], xyz[0],xyz[1],xyz[2]);
			color1.rgbf(tt[0],tt[1],tt[2],alpha);
			xyz[2]=1;  ColorConvert::Xyz2Rgb(&tt[0],&tt[1],&tt[2], xyz[0],xyz[1],xyz[2]);
			color2.rgbf(tt[0],tt[1],tt[2],alpha);
			xyz[2]=pos;


		} else if (bars.e[c]->type==COLORSLIDER_Transparency) {
			pos=alpha;
			color1.rgbf(rgb[0],rgb[1],rgb[2],0.);
			color2.rgbf(rgb[0],rgb[1],rgb[2],1.);
		}

		if (c!=current) text=NULL;
		else text=bars.e[c]->text;

		if (win_style&COLORSLIDERS_Vertical) {
			DrawVertical(color1,color2, x,y, w,h, pos,text, bars.e[c]->type==COLORSLIDER_Transparency);
		} else {
			DrawHorizontal(color1,color2, x,y, w,h, pos,text, bars.e[c]->type==COLORSLIDER_Transparency);
		}
	}

	if (!(win_style&COLORSLIDERS_HideHex)) {
		char str[20];
		dp->NewFG(win_colors->fg);
		dp->textout(hex.x+hex.width/2,hex.y+hex.height/2, HexValue(str),-1, LAX_CENTER);
	}

	if (!(win_style&COLORSLIDERS_HideOldNew)) {
		ScreenColor color;
		double *ccolor=colors;
		int ccolortype=colortype;

		 //old color
		colortype=oldcolortype;
		colors=oldcolor;
		color.rgbf(Red(),Green(),Blue(), Alpha());
		FillWithTransparency(color, oldnew.x,oldnew.y,oldnew.width/2,oldnew.height);

		 //new color
		colortype=ccolortype;
		colors=ccolor;
		color.rgbf(Red(),Green(),Blue(), Alpha());
		FillWithTransparency(color, oldnew.x+oldnew.width/2,oldnew.y,oldnew.width/2,oldnew.height);
	}

	if (useSpecialLine()) {
		x=specials.x;
		if (win_style&COLORSLIDERS_Allow_None) { 
			DrawSpecial(COLORSLIDER_None, x,specials.y, specials.height,specials.height);
			x+=specials.height+gap;
		}
		if (win_style&COLORSLIDERS_Allow_Registration) {
			DrawSpecial(COLORSLIDER_Registration,  x,specials.y, specials.height,specials.height);
			x+=specials.height+gap;
		}
		if (win_style&COLORSLIDERS_Allow_Knockout) {
			DrawSpecial(COLORSLIDER_Knockout,  x,specials.y, specials.height,specials.height);
		}
	}

	SwapBuffers();
}

/*! Draw color faded by it's transparency, with the checkerbox behind it.
 */
void ColorSliders::FillWithTransparency(ScreenColor &color, int x,int y,int w,int h)
{
	Displayer *dp = GetDisplayer();

	unsigned int bg1=coloravg(rgbcolorf(.3,.3,.3),color.Pixel(), color.alpha/65535.);
	unsigned int bg2=coloravg(rgbcolorf(.6,.6,.6),color.Pixel(), color.alpha/65535.);
	int ww=square,hh;
	int a=0;

	for (int xx=x; xx<x+w; xx+=square) {
		a=(xx/square)%2;
		hh=square;
		if (xx+ww>x+w) ww=x+w-xx;
		for (int yy=y; yy<y+h; yy+=square) {
			if (yy+hh>y+h) hh=y+h-yy;
			dp->NewFG(a ? bg1 : bg2);
			dp->drawrectangle(xx,yy,ww,hh, 1);
			a=!a;
		}
		ww=square;
	}
}

/*! Draw special colors like knockout, none, and registration.
 */
void ColorSliders::DrawSpecial(int which, int x,int y,int w,int h)
{
	Displayer *dp = GetDisplayer();
	dp->MakeCurrent(this);//should have been done already

	if (which==COLORSLIDER_None) {
		dp->NewFG(1.,1.,1.);
		dp->drawrectangle(x,y,w,h, 1);

		int ll=3;
		x+=ll/2; y+=ll/2; w-=ll; h-=ll;
		dp->LineAttributes(ll,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->NewFG(0.,0.,0.);
		dp->drawline(x+ll/2,y, x+ll/2+w,y+h);
		dp->drawline(x+ll/2,y+h, x+ll/2+w,y);

		dp->NewFG(1.,0.,0.);
		dp->drawline(x,y, x+w,y+h);
		dp->drawline(x,y+h, x+w,y);

	} else if (which==COLORSLIDER_Registration) {
		dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->NewFG(1.,1.,1.);
		dp->drawrectangle(x,y,w,h, 1);
		int ww=w;
		if (h<ww) ww=h;
		dp->NewFG(0.,0.,0.);
		dp->drawline(x+w/2,      y+h/2-ww/2,  x+w/2     , y+h/2+ww/2);
		dp->drawline(x+w/2-ww/2, y+h/2     ,  x+w/2+ww/2, y+h/2     );
		dp->drawpoint(x+w/2,y+h/2, ww/4, 0);

	} else if (which==COLORSLIDER_Knockout) {
		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		ScreenColor color(0,0,0,0);
		FillWithTransparency(color, x,y,w,h);
		//dp->drawrectangle(x,y,w,h, 1);

		dp->NewFG(1.,1.,1.);
		dp->moveto(x,y+h*.5);
		dp->lineto(x,y);
		dp->lineto(x+w,y);
		dp->lineto(x+w,y+h*.25);
		dp->curveto(flatpoint(x+w*.6,y+h*.75), flatpoint(x+w*.6,y), flatpoint(x,y+h*.5));
		dp->closed();
		dp->fill(0);

		dp->moveto(x,y+h);
		dp->lineto(x+w,y+h);
		dp->lineto(x+w,y+h*.5);
		dp->curveto(flatpoint(x+w*.6,y+h), flatpoint(x+w*.6,y+h*.333), flatpoint(x,y+h));
		dp->closed();
		dp->fill(0);

	}

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	
	//Maybe other custom special colors or flags:
	//  name   image-representation  number-id
	//MenuInfo specials;
	//specials.AddItem(_("None"),         img, COLORSLIDER_None);
	//specials.AddItem(_("Knockout"),     img, COLORSLIDER_Knockout);
	//specials.AddItem(_("Registration"), img, COLORSLIDER_Registration);
}

//! Each bar is drawn horizontally.
/*! pos is [0..1] within 0..w. -1 means don't draw the pos.
 */
void ColorSliders::DrawHorizontal(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h, double pos,const char *text, int usealpha)
{
	Displayer *dp = GetDisplayer();

	 //draw color
	double pp;
	ScreenColor color;

	if (usealpha) {
		unsigned int abg1=rgbcolorf(.3,.3,.3);
		unsigned int abg2=rgbcolorf(.6,.6,.6);
		unsigned int bg1, bg2;
		unsigned int col;
		int a;
		int hh=square;
		for (int c=x; c<x+w; c++) {
			a=(c/square)%2;
			pp=(double)(c-x)/w;
			coloravg(&color, &color1,&color2,pp);

			bg1=coloravg(abg1,color.Pixel(), color.alpha/65535.);
			bg2=coloravg(abg2,color.Pixel(), color.alpha/65535.);

			hh=square;
			for (int yy=y; yy<y+h; yy+=square) {
				if (a) col=bg1; else col=bg2;
				dp->NewFG(col);
				if (yy+hh>y+h) hh=y+h-yy;
				dp->drawline(c,yy, c,yy+hh);
				a=!a;
			}
		}

	} else {
		double offsets[2];
		ScreenColor colors[2];
		offsets[0] = 0;
		offsets[1] = 1;
		colors [0] = color1;
		colors [1] = color2;
		dp->setLinearGradient(3, x,y, x+w,y, offsets, colors, 2);
		dp->drawrectangle(x,y,w,h,1);
		dp->NewFG(dp->FG());
	}

	 //draw pos
	DrawPos(x,y,w,h,pos);

	 //text
	if (text) {
		dp->NewFG(standoutcolor(color1,1));
		dp->textout(gap,y, text,-1, LAX_TOP|LAX_LEFT);
	}
}

void ColorSliders::DrawPos(int x,int y,int w,int h, double pos)
{
	pos*=w;

	 //draw vertical black+white bar at pos
	if (pos>=0) {
		Displayer *dp = GetDisplayer();
		dp->NewFG(0.,0.,0.);
		dp->drawline(gap+pos,y, gap+pos,y+h);
		dp->NewFG(1.0,1.0,1.0);
		dp->drawline(gap+pos+1,y, gap+pos+1,y+h);
	}
}

void ColorSliders::DrawVertical(ScreenColor &color1,ScreenColor &color2, int x,int y,int w,int h, double pos,const char *text, int usealpha)
{
	Displayer *dp = GetDisplayer();

	 //draw colors
	double offsets[2];
	ScreenColor colors[2];
	offsets[0] = 0;
	offsets[1] = 1;
	colors [0] = color1;
	colors [1] = color2;
	dp->setLinearGradient(3, x,y, x,y+h, offsets, colors, 2);
	dp->drawrectangle(x,y,w,h,1);
	dp->NewFG(dp->FG());

	 //draw pos
	if (pos>=0) {
		dp->NewFG(0.,0.,0.);
		dp->drawline(x,gap+pos, x+w,gap+pos);
		dp->NewFG(1.,1.,1.);
		dp->drawline(x,gap+pos, x+w,gap+pos+1);
	}

	// *** ignoring text
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
			double m=1;
			if (state&ShiftMask) m=4;
			else if (state&ControlMask) m=.3;

			if (win_style&COLORSLIDERS_Vertical) oldpos+=m*step*(my-oldy);
			else oldpos+=m*step*(mx-oldx);
			DBG cerr << "slider lbd  step:"<<step<<"  bar:"<<oldbar<<"  pos:"<<oldpos<<endl;
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
	if (pos<0) return 0;
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
	if (pos<0) return 0;
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

	if      (bars.e[whichbar]->type==COLORSLIDER_Red)  Red(pos);
	else if (bars.e[whichbar]->type==COLORSLIDER_Green) Green(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Blue)   Blue(pos); 

	else if (bars.e[whichbar]->type==COLORSLIDER_Cyan)   Cyan(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Magenta) Magenta(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Yellow)   Yellow(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Black)     Black(pos); 

	else if (bars.e[whichbar]->type==COLORSLIDER_HSV_Hue)        Hue(pos*360); 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSV_Saturation) HSV_Saturation(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSV_Value)      Value(pos); 

	else if (bars.e[whichbar]->type==COLORSLIDER_HSL_Hue)        Hue(pos*360); 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSL_Saturation) HSL_Saturation(pos); 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSL_Lightness)  Lightness(pos); 

	else if (bars.e[whichbar]->type==COLORSLIDER_Cie_L) Cie_L(pos*100);
	else if (bars.e[whichbar]->type==COLORSLIDER_Cie_a) Cie_a(pos*216-108);
	else if (bars.e[whichbar]->type==COLORSLIDER_Cie_b) Cie_b(pos*216-108);

	else if (bars.e[whichbar]->type==COLORSLIDER_X) X();
	else if (bars.e[whichbar]->type==COLORSLIDER_Y) Y();
	else if (bars.e[whichbar]->type==COLORSLIDER_Z) Z();

	else if (bars.e[whichbar]->type==COLORSLIDER_Transparency) Alpha(pos); 
}

//! Return what the bar->pos should be, not what it actually contains.
/*! Returns -1 for no such bar.
 */
double ColorSliders::GetPosForBar(int whichbar)
{
	if (whichbar<0 || whichbar>=bars.n) return -1;

	if      (bars.e[whichbar]->type==COLORSLIDER_Red)  return Red();
	else if (bars.e[whichbar]->type==COLORSLIDER_Green) return Green(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Blue)   return Blue(); 

	else if (bars.e[whichbar]->type==COLORSLIDER_Cyan)   return Cyan(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Magenta) return Magenta(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Yellow)   return Yellow(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_Black)     return Black(); 

	else if (bars.e[whichbar]->type==COLORSLIDER_HSV_Hue)        return Hue()/360; 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSV_Saturation) return HSV_Saturation(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSV_Value)       return Value(); 

	else if (bars.e[whichbar]->type==COLORSLIDER_HSL_Hue)        return Hue()/360; 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSL_Saturation) return HSL_Saturation(); 
	else if (bars.e[whichbar]->type==COLORSLIDER_HSL_Lightness)  return Lightness(); 

	else if (bars.e[whichbar]->type==COLORSLIDER_Cie_L) return Cie_L()/100;
	else if (bars.e[whichbar]->type==COLORSLIDER_Cie_a) return (Cie_a()+108)/216;
	else if (bars.e[whichbar]->type==COLORSLIDER_Cie_b) return (Cie_b()+108)/216;

	else if (bars.e[whichbar]->type==COLORSLIDER_X) return X();
	else if (bars.e[whichbar]->type==COLORSLIDER_Y) return Y();
	else if (bars.e[whichbar]->type==COLORSLIDER_Z) return Z();

	else if (bars.e[whichbar]->type==COLORSLIDER_Transparency) return Alpha(); 
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

	} else if (ch==LAX_Enter) {
		app->destroywindow(this);
		return 0;
	}

	int ncurrent=current;
	if        (ch=='r') { ncurrent=FindBar(COLORSLIDER_Red);     
	} else if (ch=='g') { ncurrent=FindBar(COLORSLIDER_Green);   
	} else if (ch=='b') { ncurrent=FindBar(COLORSLIDER_Blue);    
	} else if (ch=='c') { ncurrent=FindBar(COLORSLIDER_Cyan);    
	} else if (ch=='m') { ncurrent=FindBar(COLORSLIDER_Magenta);  
	} else if (ch=='y') { ncurrent=FindBar(COLORSLIDER_Yellow);   
	} else if (ch=='k') { ncurrent=FindBar(COLORSLIDER_Black);     
	} else if (ch=='h') { ncurrent=FindBar(COLORSLIDER_HSL_Hue);       
	} else if (ch=='s') { ncurrent=FindBar(COLORSLIDER_HSL_Saturation); 
	} else if (ch=='l') { ncurrent=FindBar(COLORSLIDER_HSL_Lightness);   
	} else if (ch=='a') { ncurrent=FindBar(COLORSLIDER_Transparency); 
	}
	if (current!=ncurrent) {
		current=ncurrent;
		needtodraw=1;
		return 0;
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
		cerr << " *** note that ColorSliders in vertical mode is really not implemented"<<endl;

	} else {
		if ((win_style&(COLORSLIDERS_HideHex|COLORSLIDERS_HideOldNew))==0) 
			 //make room for the old/new selector and/or the hex entry box
			sliders.height-=1.5*gap+hexh;

		if (!(win_style&COLORSLIDERS_HideHex)) {
			 //set up hex rectangle
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
			 //set up oldnew box
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

		if (useSpecialLine()) {
			sliders.height-=1.5*gap+hexh;

			specials.x=gap;
			specials.y=sliders.y+sliders.height+gap/2;
			specials.width=win_w-2*gap;
			specials.height=hexh;
		}
	}

	if (win_style&COLORSLIDERS_Vertical) step=1./sliders.height;
	else step=1./sliders.width;
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




