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
//    Copyright (C) 2012-2013 by Tom Lechner
//

#include <lax/colorbase.h>
#include <lax/colorspace.h>
#include <lax/misc.h>
#include <lax/colors.h>
#include <lax/strmanip.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//------------------------------- ColorBase ------------------------------
/*! \class ColorBase
 * Defines various simple conversions between rgb, cmyk, gray, and hsv, hsl, cielab, and xyz.
 *
 * Whenever you set a particular channel, the colortype will change to whatever
 * system the channel belongs in.
 *
 * Currently, these are non-icc based conversions. They are used more
 * as an interface hint than actual color mapping.
 *
 * colortype is from BasicColorTypes.
 *
 * \todo current channel normalizing is quite a hack. Running an actual scan of the whole rgb
 *    combinations yields the following range, based on the conversion functions currently in use..
 * <pre>  
 *  L:    0.000 ..   99.910
 *  a:  -86.117 ..   98.184
 *  b: -107.786 ..   94.410
 *  
 *  x:    0.000 ..    0.948
 *  y:    0.000 ..    0.998
 *  z:    0.000 ..    1.087
 * </pre>
 */

/*! By default, max=65535.
 */
ColorBase::ColorBase()
{
	max=65535;

	colorspecial=COLOR_Normal;
	colortype=LAX_COLOR_RGB;
	SetColorSystem(colortype);

	colors=color1;
	oldcolor[0]=colors[0]=color2[0]=0;
	oldcolor[1]=colors[1]=color2[1]=0;
	oldcolor[2]=colors[2]=color2[2]=0;
	oldcolor[3]=colors[3]=color2[3]=1;
	oldcolor[4]=colors[4]=color2[4]=0;

	oldcolorspecial=colorspecial;
	oldcolortype=colortype;
}


ColorBase::ColorBase(int ctype, double c0,double c1,double c2,double c3,double c4, double nmax)
{
	max = nmax;

	colorspecial=COLOR_Normal; //a SimpleColorId
	colortype=ctype;
	SetColorSystem(ctype);

	 //fill in transparency if necessary
	if (colortype==LAX_COLOR_GRAY)  { if (c1<0) c1=1; }
	else if (colortype==LAX_COLOR_CMYK)  { if (c4<0) c4=1; }
	else { //the rest have 3 fields normally
		if (c3<0) c3=1;
	}

	colors=color1;
	colors[0]=color2[0]=c0;
	colors[1]=color2[1]=c1;
	colors[2]=color2[2]=c2;
	colors[3]=color2[3]=c3;
	colors[4]=color2[4]=c4;
	Clamp();

	oldcolorspecial=colorspecial;
	oldcolortype=colortype;
	oldcolor[0]=colors[0];
	oldcolor[1]=colors[1];
	oldcolor[2]=colors[2];
	oldcolor[3]=colors[3];
	oldcolor[4]=colors[4];
}

ColorBase::~ColorBase()
{
}


/*! Change the maximum integer value of color channels.
 * This only sets this->max. Does not convert anything.
 */
void ColorBase::SetMax(int newmax)
{  max=newmax; }

/*! Sets colortype, and updates channel_min and channel_max.
 *
 * HSV and HSL use hue in range 0..360, but other two in 0..1.
 * CieLab uses L 0..100, and a,b range from I don't know what all. Arbitrarily choosing [-108..108].
 * x,y,z arbitrarily cut off at 0..1.
 *
 * All others channels are all 0..1.
 *
 * Return 0 for success, or 1 for system not found.
 */
int ColorBase::SetColorSystem(int newsystem)
{
	if (newsystem<LAX_COLOR_RGB || newsystem>=LAX_COLOR_MAX) return 1;

	colortype=newsystem;

	for (int c=0; c<5; c++) {
		channel_min[c]=0;
		channel_max[c]=1;
	}

	if (colortype==LAX_COLOR_HSL || colortype==LAX_COLOR_HSV) {
		channel_max[0]=360;

	} else if (colortype==LAX_COLOR_CieLAB ) {
		channel_max[0]=100;
		channel_min[1]=-108;
		channel_max[1]=108;
		channel_min[2]=-108;
		channel_max[2]=108; 
	}

	return 0;
}

//! For each field, make sure it is in range [0..max].
void ColorBase::Clamp()
{
	for (int c=0; c<5; c++) {
		if (color1[c]<channel_min[c]) color1[c]=channel_min[c];
		else if (color1[c]>channel_max[c]) color1[c]=channel_max[c];

		if (color2[c]<channel_min[c]) color2[c]=channel_min[c];
		else if (color2[c]>channel_max[c]) color2[c]=channel_max[c];
	}
}

//! Called when a value is changed through any of the various color setting functions.
/*! Default is do nothing.
 */
void ColorBase::Updated()
{
}

//! Return 1 if the color array does not match the oldcolor array. Usually called after button up, compared to button down.
int ColorBase::ColorChanged()
{
	for (int c=0; c<5; c++) if (oldcolor[c]!=colors[c]) return 1;
	if (oldcolortype!=colortype) return 1;
	return 0;
}

//! Revert colors to oldcolor.
void ColorBase::RestoreColor(int swap)
{
	if (swap) {
		double t;
		for (int c=0; c<5; c++) {
			t=colors[c];
			colors[c]=oldcolor[c];
			oldcolor[c]=t;
		}
		int tt=colortype;
		colortype=oldcolortype;
		oldcolortype=tt;

		tt=colorspecial;
		colorspecial=oldcolorspecial;
		oldcolorspecial=tt;

	} else {
		colorspecial=oldcolorspecial;
		colortype=oldcolortype;
		for (int c=0; c<5; c++) colors[c]=oldcolor[c];
	}

	Updated();
}



/*! Return 1 for color returned, or 0 for unable to convert.
 */
int ColorBase::Get(int newtype, double *c0,double *c1,double *c2,double *c3,double *c4)
{
	double cc[5];

    if (newtype==LAX_COLOR_RGB) {
        cc[0]=Red(); cc[1]=Green(); cc[2]=Blue(); cc[3]=Alpha(); cc[4]=0;
    
    } else if (newtype==LAX_COLOR_GRAY) {
        cc[0]=Gray(); cc[1]=Alpha(); cc[2]=0; cc[3]=0; cc[4]=0;
    
    } else if (newtype==LAX_COLOR_CMYK) {
        cc[0]=Cyan(); cc[1]=Magenta(); cc[2]=Yellow(); cc[3]=Black(); cc[4]=Alpha();

    } else if (newtype==LAX_COLOR_HSV) {
        cc[0]=Hue(); cc[1]=HSV_Saturation(); cc[2]=Value(); cc[3]=Alpha(); cc[4]=0; 
  
    } else if (newtype==LAX_COLOR_HSL) {
        cc[0]=Hue(); cc[1]=HSL_Saturation(); cc[2]=Lightness(); cc[3]=Alpha(); cc[4]=0;
    
    } else if (newtype==LAX_COLOR_CieLAB) {
        cc[0]=Cie_L(); cc[1]=Cie_a(); cc[2]=Cie_b(); cc[3]=Alpha(); cc[4]=0;

    } else if (newtype==LAX_COLOR_XYZ) {
        cc[0]=X(); cc[1]=Y(); cc[2]=Z(); cc[3]=Alpha(); cc[4]=0;

	} else return 0;


	if (c0) *c0=cc[0];
	if (c1) *c1=cc[1];
	if (c2) *c2=cc[2];
	if (c3) *c3=cc[3];
	if (c4) *c4=cc[4];

	return 1;
}


//! Fill cmyk[4] with current double values.
void ColorBase::CMYK(double *cmyk)
{  cmyk[0]=Cyan(); cmyk[1]=Magenta(); cmyk[2]=Yellow(); cmyk[3]=Black(); }


//! Fill rgb[3] with current double values.
void ColorBase::RGB(double *rgb)
{  rgb[0]=Red(); rgb[1]=Green(); rgb[2]=Blue(); }


//! Fill hsv[3] with current double values.
void ColorBase::HSV(double *hsv)
{  hsv[0]=Hue(); hsv[1]=HSV_Saturation(); hsv[2]=Value(); }


//! Fill hsl[3] with current double values.
void ColorBase::HSL(double *hsl)
{  hsl[0]=Hue(); hsl[1]=HSL_Saturation(); hsl[2]=Lightness(); }

//! Fill lab[3] with current double values.
void ColorBase::CieLab(double *lab)
{  lab[0]=Cie_L(); lab[1]=Cie_a(); lab[2]=Cie_b(); }

void ColorBase::XYZ(double *xyz)
{  xyz[0]=X(); xyz[1]=Y(); xyz[2]=Z(); }


bool ColorBase::Parse(const char *buffer, int len, const char **endptr)
{
	ScreenColor scolor;
	double d[5];
	if (len < 0) len = strlen(buffer);
	if (len == 0) return false;
	char *buf = newnstr(buffer,len);
	bool status = (LaxFiles::SimpleColorAttribute(buf, d, endptr) == 0);
	if (status) {
		//success!
		SetRGB(d[0], d[1], d[2], d[3]);
	}
	delete[] buf;
	return status;
}

/*! Return an 8 bit per channel hex argb value in format "#AARRGGBB".
 *
 *  buffer must be at least 10 characters long, or you will segfault.
 *  Puts something like "#ff00abff" (without the quotes) in buffer. Returns buffer.
 */
char *ColorBase::HexValue(char *buffer)
{
	sprintf(buffer,"#%02x%02x%02x%02x", (unsigned int)(Alpha()*255), (unsigned int)(Red()*255),(unsigned int)(Green()*255),(unsigned int)(Blue()*255));
	return buffer;
}

/*! Set rgba color from a hex string.
 *
 * Return 0 for success, or 1 for parse error.
 * hex like "#00ff00ff" with 8 digits is "#aarrggbb".
 * hex like "#00ff00" with 6 digits is "#rrggbb", fully opaque
 * hex like "#123" is "#rgb", same as "#rrggbb".
 *
 * The '#' is optional.
 *
 * Uses LaxFiles::HexColorAttributeRGB().
 */
int ColorBase::SetHexValue(const char *hex)
{
	if (!hex) return 1;

	ScreenColor color;
	LaxFiles::HexColorAttributeRGB(hex, &color, NULL);

	double a=color.alpha/65535.;
	double r=color.red  /65535.;
	double g=color.green/65535.;
	double b=color.blue /65535.;

	SetRGB(r,g,b,a);
	return 0;
}

/*! Should be, but doesn't have to be, one of SimpleColorId.
 *
 * If it's not one of SimpleColorId, the value should be greater than COLOR_CATEGORY_MAX.
 *
 * Returns old value.
 */
int ColorBase::SetSpecial(int newspecial)
{
	int old=colorspecial;
	colorspecial=newspecial;
	return old;
}

int ColorBase::Set(Color *color)
{
	if (!color) return 1;

	if (color->color_type==COLOR_Normal) {
		return Set(color->colorsystemid, color->ChannelValue(0),
										 color->ChannelValue(1),
										 color->ChannelValue(2),
										 color->ChannelValue(3),
										 color->ChannelValue(4));

	} else if (color->color_type==COLOR_Knockout
				|| color->color_type==COLOR_Registration
				|| color->color_type==COLOR_None) {
		return SetSpecial(color->color_type);
	}

	return 0;
}

int ColorBase::Set(const ScreenColor &color)
{
	return Set(LAX_COLOR_RGB, color.Red(), color.Green(), color.Blue(), color.Alpha());
}

//! Assume rgb, no a
int ColorBase::Set(unsigned long color)
{
	return Set(LAX_COLOR_RGB,
			((color&0xff0000)  >>16) /255.,
			((color&0xff00)    >> 8)/255.,
			((color&0xff)      >>0)/255.,
			1.0, //((color&0xff000000)>>24)/255.,
			1.0
		 );
}

/*! Return 0 for success or 1 for error. newtype is one of BasicColorTypes.
 */
int ColorBase::Set(int newtype, double c0,double c1,double c2,double c3,double c4)
{
	if (SetColorSystem(newtype)!=0) return 1;

	colorspecial=COLOR_Normal;

	colors[0]=c0;
	colors[1]=c1;
	colors[2]=c2;
	colors[3]=c3;
	colors[4]=c4;

	Clamp();
	Updated();

	DBG cerr <<" ColorBase set new color:"<<Red()<<','<<Green()<<','<<Blue()<<" a:"<<Alpha()<<endl;

	return 0;
}

//! Set the color.
/*! Any values out of [0..1] are clamped with Clamp().
 *
 * Any call to this function will set the color mode to LAX_COLOR_RGB.
 */
void ColorBase::SetRGB(double rr,double gg,double bb,double aa)
{
	Set(LAX_COLOR_RGB, rr,gg,bb,aa >= 0 ? aa : 1, 0);
	DBG cerr <<" ColorBase set new rgb color:"<<Red()<<','<<Green()<<','<<Blue()<<" a:"<<Alpha()<<endl;
}


//! Set the channel when the value is >= 0.
/*! Any values over max are clamped.
 *
 * Any call to this function will set the color mode to LAX_COLOR_CMYK.
 */
void ColorBase::SetCMYK(double c,double m,double y,double k,double a)
{
	Set(LAX_COLOR_CMYK, c,m,y,k,a);
	DBG cerr <<" ColorBase set new cmyk color:"<<Cyan()<<','<<Magenta()<<','<<Yellow()<<" b:"<<Black()<<" a:"<<Alpha()<<endl;
}


//! Set the channel when the value is >= 0.
/*! Any values over max are clamped.
 *
 * Any call to this function will set the color mode to LAX_COLOR_CMYK.
 */
void ColorBase::SetGray(double g,double a)
{
	Set(LAX_COLOR_CMYK, g,a, 0,0,0);
	DBG cerr <<" ColorBase set new gray color:"<<Gray()<<','<<Alpha()<<endl;
}


void ColorBase::SetHSV(double h,double s,double v, double a)
{
	Set(LAX_COLOR_HSV, h,s,v,a, 0);
}

void ColorBase::SetHSL(double h,double s,double l, double a)
{
	Set(LAX_COLOR_HSL, h,s,l,a, 0);
}

void ColorBase::SetLab(double l,double a,double b, double alpha)
{
	Set(LAX_COLOR_CieLAB, l,a,b,alpha, 0);
}

void ColorBase::SetXYZ(double x,double y,double z, double a)
{
	Set(LAX_COLOR_XYZ, x,y,z,a, 0);
}


//----------------Alpha
double ColorBase::Alpha(double a)
{
	if (colortype==LAX_COLOR_CMYK) colors[4]=a;
	else if (colortype==LAX_COLOR_GRAY) colors[1]=a;
	else colors[3]=a;

	Updated();
	return Alpha();
}

double ColorBase::Alpha()
{
	if (colortype==LAX_COLOR_CMYK) return colors[4];
	if (colortype==LAX_COLOR_GRAY) return colors[1];
	return colors[3];
}


//-------------Red
double ColorBase::Red(double r)
{
	if (colortype==LAX_COLOR_RGB    ) colors[0]=r;
	else {
		 //not already rgb, so we need to switch
		double a=Alpha();
		double rgb[3];

		if (colortype==LAX_COLOR_GRAY   )
			{ rgb[0]=rgb[1]=rgb[2]=colors[0]; }

		else if (colortype==LAX_COLOR_CMYK   ) 
			simple_cmyk_to_rgb(colors,rgb);

		else if (colortype==LAX_COLOR_HSL    ) 
			ColorConvert::Hsl2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSL_Saturation(),Lightness());

		else if (colortype==LAX_COLOR_HSV    ) 
			ColorConvert::Hsv2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSV_Saturation(),Value());

		else if (colortype==LAX_COLOR_CieLAB ) 
			ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		else if (colortype==LAX_COLOR_XYZ    )   
			ColorConvert::Xyz2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		rgb[0]=r;
		SetRGB(rgb[0],rgb[1],rgb[2],a);
	}

	Clamp();
	Updated();
	return colors[0];
}

double ColorBase::Red()
{ 
	if (colortype==LAX_COLOR_RGB    ) return colors[0];
	if (colortype==LAX_COLOR_GRAY   ) return colors[0];

	double rgb[3];
	rgb[0]=0;

	if (colortype==LAX_COLOR_CMYK   ) simple_cmyk_to_rgb(colors,rgb);
	else if (colortype==LAX_COLOR_HSL    ) {
		ColorConvert::Hsl2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSL_Saturation(),Lightness());

	} else if (colortype==LAX_COLOR_HSV    ) {
		ColorConvert::Hsv2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSV_Saturation(),Value());

	} else if (colortype==LAX_COLOR_CieLAB ) {
		ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], Cie_L(),Cie_a(),Cie_b());

	} else if (colortype==LAX_COLOR_XYZ    ) {
		ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], X(),Y(),Z());
	}

	return rgb[0];
}


//----------------Green
double ColorBase::Green(double r)
{
	if (colortype==LAX_COLOR_RGB    ) colors[1]=r;
	else {
		 //not already rgb, so we need to switch
		double a=Alpha();
		double rgb[3];

		if (colortype==LAX_COLOR_GRAY   )
			{ rgb[0]=rgb[1]=rgb[2]=colors[0]; }

		else if (colortype==LAX_COLOR_CMYK   ) 
			simple_cmyk_to_rgb(colors,rgb);

		else if (colortype==LAX_COLOR_HSL    ) 
			ColorConvert::Hsl2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSL_Saturation(),Lightness());

		else if (colortype==LAX_COLOR_HSV    ) 
			ColorConvert::Hsv2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSV_Saturation(),Value());

		else if (colortype==LAX_COLOR_CieLAB ) 
			ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		else if (colortype==LAX_COLOR_XYZ    )   
			ColorConvert::Xyz2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		rgb[1]=r;
		SetRGB(rgb[0],rgb[1],rgb[2],a);
	}

	Clamp();
	Updated();
	return colors[1];
}

double ColorBase::Green()
{ 
	if (colortype==LAX_COLOR_RGB    ) return colors[1];
	if (colortype==LAX_COLOR_GRAY   ) return colors[1];

	double rgb[3];
	rgb[1]=0;

	if (colortype==LAX_COLOR_CMYK   ) simple_cmyk_to_rgb(colors,rgb);
	else if (colortype==LAX_COLOR_HSL    ) {
		ColorConvert::Hsl2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSL_Saturation(),Lightness());

	} else if (colortype==LAX_COLOR_HSV    ) {
		ColorConvert::Hsv2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSV_Saturation(),Value());

	} else if (colortype==LAX_COLOR_CieLAB ) {
		ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], Cie_L(),Cie_a(),Cie_b());

	} else if (colortype==LAX_COLOR_XYZ    ) {
		ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], X(),Y(),Z());
	}

	return rgb[1];
}


//----------------Blue
double ColorBase::Blue(double r)
{
	if (colortype==LAX_COLOR_RGB    ) colors[2]=r;
	else {
		 //not already rgb, so we need to switch
		double a=Alpha();
		double rgb[3];

		if (colortype==LAX_COLOR_GRAY   )
			{ rgb[0]=rgb[1]=rgb[2]=colors[0]; }

		else if (colortype==LAX_COLOR_CMYK   ) 
			simple_cmyk_to_rgb(colors,rgb);

		else if (colortype==LAX_COLOR_HSL    ) 
			ColorConvert::Hsl2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSL_Saturation(),Lightness());

		else if (colortype==LAX_COLOR_HSV    ) 
			ColorConvert::Hsv2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSV_Saturation(),Value());

		else if (colortype==LAX_COLOR_CieLAB ) 
			ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		else if (colortype==LAX_COLOR_XYZ    )   
			ColorConvert::Xyz2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		rgb[2]=r;
		SetRGB(rgb[0],rgb[1],rgb[2],a);
	}

	Clamp();
	Updated();
	return colors[2];
}

double ColorBase::Blue()
{ 
	if (colortype==LAX_COLOR_RGB    ) return colors[2];
	if (colortype==LAX_COLOR_GRAY   ) return colors[2];

	double rgb[3];
	rgb[1]=0;

	if (colortype==LAX_COLOR_CMYK   ) {
		simple_cmyk_to_rgb(colors,rgb);

	} else if (colortype==LAX_COLOR_HSL    ) {
		ColorConvert::Hsl2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSL_Saturation(),Lightness());

	} else if (colortype==LAX_COLOR_HSV    ) {
		ColorConvert::Hsv2Rgb(&rgb[0],&rgb[1],&rgb[2], Hue(),HSV_Saturation(),Value());

	} else if (colortype==LAX_COLOR_CieLAB ) {
		ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], Cie_L(),Cie_a(),Cie_b());

	} else if (colortype==LAX_COLOR_XYZ    ) {
		ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], X(),Y(),Z());
	}

	return rgb[2];
}


//----------------Gray
double ColorBase::Gray(double r)
{
	SetGray(r,Alpha());
	return colors[0];
}

double ColorBase::Gray()
{ 
	if (colortype==LAX_COLOR_GRAY   ) return colors[0];

	return simple_rgb_to_grayf(Red(),Green(),Blue());
}


//----------------Cyan
double ColorBase::Cyan(double r)
{
	if (colortype==LAX_COLOR_CMYK) {
		colors[0]=r;
		Clamp();
		Updated();
		return colors[0];

	}

	 //not already cmyk, so we need to switch
	double a=Alpha();
	double cmyk[4];

	if (colortype==LAX_COLOR_GRAY   )
		{ cmyk[0]=cmyk[1]=cmyk[2]=0;  cmyk[3]=colors[0]; }

	else {
		 //note: this is faster to write, but 3 times slower than it needs to be:
		simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);
	}

	cmyk[0]=r;
	SetCMYK(cmyk[0],cmyk[1],cmyk[2],cmyk[3],a);

	return colors[0];
}

double ColorBase::Cyan()
{ 
	if (colortype==LAX_COLOR_CMYK   ) return colors[0];


	double cmyk[4];
	cmyk[1]=0;

	 //note: this is faster to write, but 3 times slower than it needs to be:
	simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);

	return cmyk[0];
}


//----------------Magenta
double ColorBase::Magenta(double r)
{
	if (colortype==LAX_COLOR_CMYK) {
		colors[1]=r;
		Clamp();
		Updated();
		return colors[1];

	}

	 //not already cmyk, so we need to switch
	double a=Alpha();
	double cmyk[4];

	if (colortype==LAX_COLOR_GRAY   )
		{ cmyk[0]=cmyk[1]=cmyk[2]=0;  cmyk[3]=colors[0]; }

	else {
		 //note: this is faster to write, but 3 times slower than it needs to be:
		simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);
	}

	cmyk[1]=r;
	SetCMYK(cmyk[0],cmyk[1],cmyk[2],cmyk[3],a);

	return colors[1];
}

double ColorBase::Magenta()
{ 
	if (colortype==LAX_COLOR_CMYK   ) return colors[1];


	double cmyk[4];

	 //note: this is faster to write, but 3 times slower than it needs to be:
	simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);

	return cmyk[2];
}


//----------------Yellow
double ColorBase::Yellow(double r)
{
	if (colortype==LAX_COLOR_CMYK) {
		colors[2]=r;
		Clamp();
		Updated();
		return colors[2];

	}

	 //not already cmyk, so we need to switch
	double a=Alpha();
	double cmyk[4];

	if (colortype==LAX_COLOR_GRAY   )
		{ cmyk[0]=cmyk[1]=cmyk[2]=0;  cmyk[3]=colors[0]; }

	else {
		 //note: this is faster to write, but 3 times slower than it needs to be:
		simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);
	}

	cmyk[2]=r;
	SetCMYK(cmyk[0],cmyk[1],cmyk[2],cmyk[3],a);

	return colors[2];
}

double ColorBase::Yellow()
{ 
	if (colortype==LAX_COLOR_CMYK   ) return colors[2];


	double cmyk[4];

	 //note: this is faster to write, but 3 times slower than it needs to be:
	simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);

	return cmyk[2];
}


//----------------Black
double ColorBase::Black(double r)
{
	if (colortype==LAX_COLOR_CMYK) {
		colors[3]=r;
		Clamp();
		Updated();
		return colors[3];

	}

	 //not already cmyk, so we need to switch
	double a=Alpha();
	double cmyk[4];

	if (colortype==LAX_COLOR_GRAY   )
		{ cmyk[0]=cmyk[1]=cmyk[2]=0;  cmyk[3]=colors[0]; }

	else {
		 //note: this is faster to write, but 3 times slower than it needs to be:
		simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);
	}

	cmyk[3]=r;
	SetCMYK(cmyk[0],cmyk[1],cmyk[2],cmyk[3],a);

	return colors[3];
}

double ColorBase::Black()
{ 
	if (colortype==LAX_COLOR_CMYK   ) return colors[3];

	double cmyk[4];

	 //note: this is faster to write, but 3 times slower than it needs to be:
	simple_rgb_to_cmyk(Red(),Green(),Blue(), &cmyk[0],&cmyk[1],&cmyk[2],&cmyk[3]);

	return cmyk[3];
}


//----------------Hue
double ColorBase::Hue(double r)
{
	if (colortype==LAX_COLOR_HSV || colortype==LAX_COLOR_HSL) {
		colors[0]=r;
		Clamp();
		Updated();
		return colors[0];
	}

	 //not already hsv, so we need to switch
	double a=Alpha();
	double hsv[3];

	if (colortype==LAX_COLOR_GRAY   )
		ColorConvert::Rgb2Hsv(&hsv[0],&hsv[1],&hsv[2], colors[0],colors[0],colors[0]);

	else {
		 //need to convert to rgb, then to hsv
		double rgb[3];

		if (colortype==LAX_COLOR_CMYK   ) 
			simple_cmyk_to_rgb(colors,rgb);

		else if (colortype==LAX_COLOR_RGB    ) 
			{ rgb[0]=colors[0]; rgb[1]=colors[1]; rgb[2]=colors[2]; }

		else if (colortype==LAX_COLOR_CieLAB ) 
			ColorConvert::Lab2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		else if (colortype==LAX_COLOR_XYZ    )   
			ColorConvert::Xyz2Rgb(&rgb[0],&rgb[1],&rgb[2], colors[0],colors[1],colors[2]);

		ColorConvert::Rgb2Hsv(&hsv[0],&hsv[1],&hsv[2], rgb[0],rgb[1],rgb[2]);
	}

	hsv[0]=r;
	SetHSV(hsv[0],hsv[1],hsv[2],a);

	return colors[0];
}

double ColorBase::Hue()
{
	if (colortype==LAX_COLOR_HSV || colortype==LAX_COLOR_HSL) return colors[0];

	double hsv[3];
	ColorConvert::Rgb2Hsv(&hsv[0],&hsv[1],&hsv[2], Red(),Green(),Blue());
	return hsv[0];
}


//----------------HSV Saturation
double ColorBase::HSV_Saturation(double r)
{
	if (colortype==LAX_COLOR_HSV) {
		colors[1]=r;
		Clamp();
		Updated();
		return colors[1];
	}

	 //not already hsv, so we need to switch
	double a=Alpha();
	double hsv[3];

	ColorConvert::Rgb2Hsv(&hsv[0],&hsv[1],&hsv[2], Red(),Green(),Blue());
	hsv[1]=r;
	SetHSV(hsv[0],hsv[1],hsv[2],a);

	return colors[1];
}

double ColorBase::HSV_Saturation()
{
	if (colortype==LAX_COLOR_HSV) return colors[1];

	double hsv[3];
	ColorConvert::Rgb2Hsv(&hsv[0],&hsv[1],&hsv[2], Red(),Green(),Blue());
	return hsv[1];
}


//----------------Value
/*! Of HSV.
 */
double ColorBase::Value(double r)
{
	if (colortype==LAX_COLOR_HSV) {
		colors[2]=r;
		Clamp();
		Updated();
		return colors[2];
	}

	 //not already hsv, so we need to switch
	double a=Alpha();
	double hsv[3];

	ColorConvert::Rgb2Hsv(&hsv[0],&hsv[1],&hsv[2], Red(),Green(),Blue());
	hsv[2]=r;
	SetHSV(hsv[0],hsv[1],hsv[2],a);

	return colors[2];
}

double ColorBase::Value()
{
	if (colortype==LAX_COLOR_HSV) return colors[2];

	double hsv[3];
	ColorConvert::Rgb2Hsv(&hsv[0],&hsv[1],&hsv[2], Red(),Green(),Blue());
	return hsv[2];
}


//----------------HSL Saturation
double ColorBase::HSL_Saturation(double r)
{
	if (colortype==LAX_COLOR_HSL) {
		colors[1]=r;
		Clamp();
		Updated();
		return colors[1];
	}

	 //not already hsl, so we need to switch
	double a=Alpha();
	double hsl[3];

	ColorConvert::Rgb2Hsl(&hsl[0],&hsl[1],&hsl[2], Red(),Green(),Blue());
	hsl[1]=r;
	SetHSL(hsl[0],hsl[1],hsl[2],a);

	return colors[1];
}

double ColorBase::HSL_Saturation()
{
	if (colortype==LAX_COLOR_HSL) return colors[1];

	double hsl[3];
	ColorConvert::Rgb2Hsl(&hsl[0],&hsl[1],&hsl[2], Red(),Green(),Blue());
	return hsl[1];
}


//----------------Lightness
/*! Of HSL.
 */
double ColorBase::Lightness(double r)
{
	if (colortype==LAX_COLOR_HSL) {
		colors[2]=r;
		Clamp();
		Updated();
		return colors[2];
	}

	 //not already hsl, so we need to switch
	double a=Alpha();
	double hsl[3];

	ColorConvert::Rgb2Hsl(&hsl[0],&hsl[1],&hsl[2], Red(),Green(),Blue());
	hsl[2]=r;
	SetHSL(hsl[0],hsl[1],hsl[2],a);

	return colors[2];
}

double ColorBase::Lightness()
{
	if (colortype==LAX_COLOR_HSL) return colors[2];

	double hsl[3];
	ColorConvert::Rgb2Hsl(&hsl[0],&hsl[1],&hsl[2], Red(),Green(),Blue());
	return hsl[2];
}


//----------------Cie_L
double ColorBase::Cie_L(double r)
{
	if (colortype==LAX_COLOR_CieLAB) {
		colors[0]=r;
		Clamp();
		Updated();
		return colors[0];
	}

	 //not already lab, so we need to switch
	double a=Alpha();
	double lab[3];

	if (colortype==LAX_COLOR_XYZ)
		ColorConvert::Xyz2Lab(&lab[0],&lab[1],&lab[2], X(),Y(),Z());
	else 
		ColorConvert::Rgb2Lab(&lab[0],&lab[1],&lab[2], Red(),Green(),Blue());

	lab[0]=r;
	SetLab(lab[0],lab[1],lab[2],a);

	return colors[0];
}

double ColorBase::Cie_L()
{
	if (colortype==LAX_COLOR_CieLAB) return colors[0];

	double lab[3];
	if (colortype==LAX_COLOR_XYZ)
		ColorConvert::Xyz2Lab(&lab[0],&lab[1],&lab[2], X(),Y(),Z());
	else 
		ColorConvert::Rgb2Lab(&lab[0],&lab[1],&lab[2], Red(),Green(),Blue());

	return lab[0];
}


//----------------Cie_a
double ColorBase::Cie_a(double r)
{
	if (colortype==LAX_COLOR_CieLAB) {
		colors[1]=r;
		Clamp();
		Updated();
		return colors[1];
	}

	 //not already lab, so we need to switch
	double a=Alpha();
	double lab[3];

	if (colortype==LAX_COLOR_XYZ)
		ColorConvert::Xyz2Lab(&lab[0],&lab[1],&lab[2], X(),Y(),Z());
	else 
		ColorConvert::Rgb2Lab(&lab[0],&lab[1],&lab[2], Red(),Green(),Blue());

	lab[1]=r;
	SetLab(lab[0],lab[1],lab[2],a);

	return colors[1];
}

double ColorBase::Cie_a()
{
	if (colortype==LAX_COLOR_CieLAB) return colors[1];

	double lab[3];
	if (colortype==LAX_COLOR_XYZ)
		ColorConvert::Xyz2Lab(&lab[0],&lab[1],&lab[2], X(),Y(),Z());
	else 
		ColorConvert::Rgb2Lab(&lab[0],&lab[1],&lab[2], Red(),Green(),Blue());

	return lab[1];
}


//----------------Cie_b
double ColorBase::Cie_b(double r)
{
	if (colortype==LAX_COLOR_CieLAB) {
		colors[2]=r;
		Clamp();
		Updated();
		return colors[2];
	}

	 //not already lab, so we need to switch
	double a=Alpha();
	double lab[3];

	if (colortype==LAX_COLOR_XYZ)
		ColorConvert::Xyz2Lab(&lab[0],&lab[1],&lab[2], X(),Y(),Z());
	else 
		ColorConvert::Rgb2Lab(&lab[0],&lab[1],&lab[2], Red(),Green(),Blue());

	lab[2]=r;
	SetLab(lab[0],lab[1],lab[2],a);

	return colors[2];
}

double ColorBase::Cie_b()
{
	if (colortype==LAX_COLOR_CieLAB) return colors[2];

	double lab[3];
	if (colortype==LAX_COLOR_XYZ)
		ColorConvert::Xyz2Lab(&lab[0],&lab[1],&lab[2], X(),Y(),Z());
	else 
		ColorConvert::Rgb2Lab(&lab[0],&lab[1],&lab[2], Red(),Green(),Blue());

	return lab[2];
}


//----------------X
double ColorBase::X(double r)
{
	if (colortype==LAX_COLOR_XYZ) {
		colors[0]=r;
		Clamp();
		Updated();
		return colors[0];
	}

	 //not already lab, so we need to switch
	double a=Alpha();
	double xyz[3];

	if (colortype==LAX_COLOR_CieLAB)
		ColorConvert::Lab2Xyz(&xyz[0],&xyz[1],&xyz[2], Cie_L(),Cie_a(),Cie_b());
	else 
		ColorConvert::Rgb2Xyz(&xyz[0],&xyz[1],&xyz[2], Red(),Green(),Blue());

	xyz[0]=r;
	SetXYZ(xyz[0],xyz[1],xyz[2],a);

	return colors[0];
}

double ColorBase::X()
{
	if (colortype==LAX_COLOR_XYZ) return colors[0];

	double xyz[3];
	if (colortype==LAX_COLOR_CieLAB)
		ColorConvert::Lab2Xyz(&xyz[0],&xyz[1],&xyz[2], Cie_L(),Cie_a(),Cie_b());
	else 
		ColorConvert::Rgb2Xyz(&xyz[0],&xyz[1],&xyz[2], Red(),Green(),Blue());

	return xyz[0];
}


//----------------Y
double ColorBase::Y(double r)
{
	if (colortype==LAX_COLOR_XYZ) {
		colors[1]=r;
		Clamp();
		Updated();
		return colors[1];
	}

	 //not already lab, so we need to switch
	double a=Alpha();
	double xyz[3];

	if (colortype==LAX_COLOR_CieLAB)
		ColorConvert::Lab2Xyz(&xyz[0],&xyz[1],&xyz[2], Cie_L(),Cie_a(),Cie_b());
	else 
		ColorConvert::Rgb2Xyz(&xyz[0],&xyz[1],&xyz[2], Red(),Green(),Blue());

	xyz[1]=r;
	SetXYZ(xyz[0],xyz[1],xyz[2],a);

	return colors[1];
}

double ColorBase::Y()
{
	if (colortype==LAX_COLOR_XYZ) return colors[1];

	double xyz[3];
	if (colortype==LAX_COLOR_CieLAB)
		ColorConvert::Lab2Xyz(&xyz[0],&xyz[1],&xyz[2], Cie_L(),Cie_a(),Cie_b());
	else 
		ColorConvert::Rgb2Xyz(&xyz[0],&xyz[1],&xyz[2], Red(),Green(),Blue());

	return xyz[1];
}


//----------------Z
double ColorBase::Z(double r)
{
	if (colortype==LAX_COLOR_XYZ) {
		colors[2]=r;
		Clamp();
		Updated();
		return colors[2];
	}

	 //not already lab, so we need to switch
	double a=Alpha();
	double xyz[3];

	if (colortype==LAX_COLOR_CieLAB)
		ColorConvert::Lab2Xyz(&xyz[0],&xyz[1],&xyz[2], Cie_L(),Cie_a(),Cie_b());
	else 
		ColorConvert::Rgb2Xyz(&xyz[0],&xyz[1],&xyz[2], Red(),Green(),Blue());

	xyz[2]=r;
	SetXYZ(xyz[0],xyz[1],xyz[2],a);

	return colors[2];
}

double ColorBase::Z()
{
	if (colortype==LAX_COLOR_XYZ) return colors[2];

	double xyz[3];
	if (colortype==LAX_COLOR_CieLAB)
		ColorConvert::Lab2Xyz(&xyz[0],&xyz[1],&xyz[2], Cie_L(),Cie_a(),Cie_b());
	else 
		ColorConvert::Rgb2Xyz(&xyz[0],&xyz[1],&xyz[2], Red(),Green(),Blue());

	return xyz[2];
}



} // namespace Laxkit


