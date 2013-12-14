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
//    Copyright (C) 2012-2013 by Tom Lechner
//

#include <lax/colorbase.h>
#include <lax/misc.h>
#include <lax/colors.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//------------------------------- ColorBase ------------------------------
/*! \class ColorBase
 * \brief Defines various simple conversions between rgb, cmyk, gray, and hsv.
 */


ColorBase::ColorBase(int ctype, int nmax, int c0,int c1,int c2,int c3,int c4)
{
	colortype=ctype;
	max=nmax;
	if (max<=0) max=65535;

	 //fill in transparency if necessary
	if (colortype==LAX_COLOR_RGB) { if (c3<0) c3=nmax; }
	else if (colortype==LAX_COLOR_GRAY)  { if (c1<0) c1=nmax; }
	else if (colortype==LAX_COLOR_CMYK)  { if (c4<0) c4=nmax; }

	colors=color1;
	colors[0]=color2[0]=c0;
	colors[1]=color2[1]=c1;
	colors[2]=color2[2]=c2;
	colors[3]=color2[3]=c3;
	colors[4]=color2[4]=c4;
	Clamp();

	oldcolor[0]=colors[0];
	oldcolor[1]=colors[1];
	oldcolor[2]=colors[2];
	oldcolor[3]=colors[3];
	oldcolor[4]=colors[4];
}

ColorBase::~ColorBase()
{
}


void ColorBase::SetMax(int newmax)
{ max=newmax; }

//! Fill cmyk[4] with double values.
void ColorBase::CMYK(double *cmyk)
{ cmyk[0]=Cyanf(); cmyk[1]=Magentaf(); cmyk[2]=Yellowf(); cmyk[3]=Blackf(); }

//! Fill rgb[3] with double values.
void ColorBase::RGB(double *rgb)
{ rgb[0]=Redf(); rgb[1]=Greenf(); rgb[2]=Bluef(); }

//! Fill hsv[3] with double values.
void ColorBase::HSV(double *hsv)
{ hsv[0]=Huef(); hsv[1]=Saturationf(); hsv[2]=Valuef(); }

//! buffer must be at least 10 characters long, or you will segfault.
/*! Puts something like "#ff00abff" (without the quotes) in buffer. Returns buffer.
 */
char *ColorBase::HexValue(char *buffer)
{
	sprintf(buffer,"#%02x%02x%02x%02x", (unsigned int)(Redf()*255),(unsigned int)(Greenf()*255),(unsigned int)(Bluef()*255),(unsigned int)(Alphaf()*255));
	return buffer;
}

/*! Return 0 for success, or 1 for parse error.
 * hex must be something like "#00ff00ff", with 8 digits. The '#' is optional.
 */
int ColorBase::SetHexValue(const char *hex)
{
	if (!hex) return 1;
	if (*hex=='#') hex++;
	unsigned long hh;
	hh=strtol(hex,NULL,16);
	double r=(hh&0xff000000)>>24;
	double g=(hh&0x00ff0000)>>16;
	double b=(hh&0x0000ff00)>> 8;
	double a=(hh&0x000000ff)    ;

	a/=255;
	r/=255;
	g/=255;
	b/=255;

	Alpha(a);
	Red  (r);
	Green(g);
	Blue (b);
	return 0;
}

//! Set the channel when the value is >= 0.
/*! Any values over max are clamped.
 *
 * Any call to this function will set the color mode to LAX_COLOR_RGB.
 */
void ColorBase::SetRGB(int r,int g,int b,int a)
{
	colortype=LAX_COLOR_RGB;

	if (r>=0) { colors[0]=r; if (colors[0]>max) colors[0]=max; }
	if (g>=0) { colors[1]=g; if (colors[1]>max) colors[1]=max; }
	if (b>=0) { colors[2]=b; if (colors[2]>max) colors[2]=max; }
	if (a>=0) { colors[3]=a; if (colors[3]>max) colors[3]=max; }
	colors[4]=0;

	Updated();

	DBG cerr <<" ColorBase set new color:"<<Red()<<','<<Green()<<','<<Blue()<<" a:"<<Alpha()<<endl;
}

/*! Values in [0..1].
 */
void ColorBase::SetRGB(double rr,double gg,double bb,double aa)
{
	colortype=LAX_COLOR_RGB;
	int r=(rr)*max+.5;
	int g=(gg)*max+.5;
	int b=(bb)*max+.5;
	int a=(aa)*max+.5;
	SetRGB(r,g,b,a);

	Updated();
}

//! Set the channel when the value is >= 0.
/*! Any values over max are clamped.
 *
 * Any call to this function will set the color mode to LAX_COLOR_CMYK.
 */
void ColorBase::SetCMYK(int c,int m,int y,int k,int a)
{
	colortype=LAX_COLOR_CMYK;

	if (c>=0) { colors[0]=c; if (colors[0]>max) colors[0]=max; }
	if (m>=0) { colors[1]=m; if (colors[1]>max) colors[1]=max; }
	if (y>=0) { colors[2]=y; if (colors[2]>max) colors[2]=max; }
	if (k>=0) { colors[3]=k; if (colors[3]>max) colors[3]=max; }
	if (a>=0) { colors[4]=a; if (colors[4]>max) colors[4]=max; }

	Updated();

	DBG cerr <<" ColorBase set new color:"<<Red()<<','<<Green()<<','<<Blue()<<" a:"<<Alpha()<<endl;
}

/*! Values in [0..1].
 */
void ColorBase::SetCMYK(double c,double m,double y,double k,double a)
{
	int cc=(c)*max+.5;
	int mm=(m)*max+.5;
	int yy=(y)*max+.5;
	int kk=(k)*max+.5;
	int aa=(a)*max+.5;
	SetCMYK(cc,mm,yy,kk,aa);
}

//! Set the channel when the value is >= 0.
/*! Any values over max are clamped.
 *
 * Any call to this function will set the color mode to LAX_COLOR_CMYK.
 */
void ColorBase::SetGray(int g,int a)
{
	colortype=LAX_COLOR_GRAY;

	if (g>=0) { colors[0]=g; if (colors[0]>max) colors[0]=max; }
	if (a>=0) { colors[1]=a; if (colors[1]>max) colors[1]=max; }
	colors[2]=colors[3]=colors[4]=0;

	Updated();

	DBG cerr <<" ColorBase set new color:"<<Red()<<','<<Green()<<','<<Blue()<<" a:"<<Alpha()<<endl;
}

void ColorBase::SetGray(double g,double a)
{
	int gg=(g+.5)*max;
	int aa=(a+.5)*max;
	SetGray(gg,aa);
}

void ColorBase::SetHSV(int h,int s,int v, int a)
{
	if (a>=0) Alpha(a);
	int r,g,b;
	simple_hsv_to_rgb(h,s,v,&r,&g,&b,max);
	Red(r);
	Green(g);
	Blue(b);
}

void ColorBase::SetHSV(double h,double s,double v, double a)
{
	int hh=(h)*max+.5;
	int ss=(s)*max+.5;
	int vv=(v)*max+.5;
	int aa=(a)*max+.5;
	SetHSV(hh,ss,vv,aa);
}


double ColorBase::Redf(double r)
{
	int rr=(r)*max+.5;
	return Red(rr)/(double)max;
}

int ColorBase::Red(int r)
{
	if (colortype==LAX_COLOR_RGB) colors[0]=r;
	else if (colortype==LAX_COLOR_GRAY) colors[0]=0.3*r + 0.59*Green() + 0.11*Blue();
	else { //cmyk
		int rgb[3];
		simple_cmyk_to_rgb(colors,rgb,max);
		rgb[0]=r;
		simple_rgb_to_cmyk(rgb,colors,max);
	}

	Updated();
	return Red();
}

double ColorBase::Redf()
{ return Red()/(double)max; }

int ColorBase::Red()
{
	if (colortype==LAX_COLOR_RGB) return colors[0];
	if (colortype==LAX_COLOR_GRAY) return colors[0];
	int rgb[3];
	simple_cmyk_to_rgb(colors,rgb,max);
	return rgb[0];
}

double ColorBase::Greenf()
{ return Green()/(double)max; }

double ColorBase::Greenf(double o)
{ return Green(int((o)*max+.5))/(double)max; }

int ColorBase::Green(int g)
{
	if (colortype==LAX_COLOR_RGB) colors[1]=g;
	else if (colortype==LAX_COLOR_GRAY) colors[0]=0.3*Red() + 0.59*g + 0.11*Blue();
	else {
		int rgb[3];
		simple_cmyk_to_rgb(colors,rgb,max);
		rgb[1]=g;
		simple_rgb_to_cmyk(rgb,colors,max);
	}

	Updated();
	return Green();
}

int ColorBase::Green()
{
	if (colortype==LAX_COLOR_RGB) return colors[1];
	if (colortype==LAX_COLOR_GRAY) return colors[0];
	int rgb[3];
	simple_cmyk_to_rgb(colors,rgb,max);
	return rgb[1];
}

double ColorBase::Bluef()
{ return Blue()/(double)max; }

double ColorBase::Bluef(double o)
{ return Blue(int((o)*max+.5))/(double)max; }

int ColorBase::Blue(int b)
{
	if (colortype==LAX_COLOR_RGB) colors[2]=b;
	else if (colortype==LAX_COLOR_GRAY) colors[0]=0.3*Red() + 0.59*Green() + 0.11*b;
	else {
		int rgb[3];
		simple_cmyk_to_rgb(colors,rgb,max);
		rgb[2]=b;
		simple_rgb_to_cmyk(rgb,colors,max);
	}

	Updated();
	return Blue();
}

int ColorBase::Blue()
{
	if (colortype==LAX_COLOR_RGB) return colors[2];
	if (colortype==LAX_COLOR_GRAY) return colors[0];
	int rgb[3];
	simple_cmyk_to_rgb(colors,rgb,max);
	return rgb[2];
}

double ColorBase::Grayf()
{ return Gray()/(double)max; }

double ColorBase::Grayf(double o)
{ return Gray(int((o)*max+.5))/(double)max; }

int ColorBase::Gray(int g)
{
	if (colortype==LAX_COLOR_GRAY) colors[0]=g;
	else if (colortype==LAX_COLOR_RGB) {
		colors[0]=colors[1]=colors[2]=g;
	} else {
		int rgb[3];
		rgb[0]=rgb[1]=rgb[2]=g;
		simple_rgb_to_cmyk(rgb,colors,max);
	}

	Updated();
	return Gray();
}

int ColorBase::Gray()
{
	if (colortype==LAX_COLOR_RGB) return 0.3*colors[0] + 0.59*colors[1] + 0.11*colors[2];
	if (colortype==LAX_COLOR_GRAY) return colors[0];
	int rgb[3];
	simple_cmyk_to_rgb(colors,rgb,max);
	return 0.3*rgb[0] + 0.59*rgb[1] + 0.11*rgb[2];

}

double ColorBase::Cyanf()
{ return Cyan()/(double)max; }

double ColorBase::Cyanf(double o)
{ return Cyan(int((o)*max+.5))/(double)max; }

int ColorBase::Cyan(int c)
{
	if (colortype==LAX_COLOR_CMYK) colors[0]=c;
	else if (colortype==LAX_COLOR_GRAY) colors[0]=0.3*(max-c) + 0.59*Green() + 0.11*Blue();
	else {
		int cmyk[4];
		simple_rgb_to_cmyk(colors,cmyk,max);
		cmyk[0]=c;
		simple_cmyk_to_rgb(cmyk,colors,max);
	}

	Updated();
	return Cyan();
}

int ColorBase::Cyan()
{
	if (colortype==LAX_COLOR_CMYK) return colors[0];
	if (colortype==LAX_COLOR_GRAY) return 0;

	int cmyk[4];
	simple_rgb_to_cmyk(colors,cmyk,max);
	return cmyk[0];
}

double ColorBase::Magentaf()
{ return Magenta()/(double)max; }

double ColorBase::Magentaf(double o)
{ return Magenta(int((o)*max+.5))/(double)max; }

int ColorBase::Magenta(int m)
{
	if (colortype==LAX_COLOR_CMYK) colors[1]=m;
	else if (colortype==LAX_COLOR_GRAY) colors[0]=0.3*Red() + 0.59*(max-m) + 0.11*Blue();
	else {
		int cmyk[4];
		simple_rgb_to_cmyk(colors,cmyk,max);
		cmyk[1]=m;
		simple_cmyk_to_rgb(cmyk,colors,max);
	}

	Updated();
	return Magenta();
}

int ColorBase::Magenta()
{
	if (colortype==LAX_COLOR_CMYK) return colors[1];
	if (colortype==LAX_COLOR_GRAY) return 0;

	int cmyk[4];
	simple_rgb_to_cmyk(colors,cmyk,max);
	return cmyk[1];
}

double ColorBase::Yellowf()
{ return Yellow()/(double)max; }

double ColorBase::Yellowf(double o)
{ return Yellow(int((o)*max+.5))/(double)max; }

int ColorBase::Yellow(int y)
{
	if (colortype==LAX_COLOR_CMYK) colors[2]=y;
	else if (colortype==LAX_COLOR_GRAY) colors[0]=0.3*Red() + 0.59*Green() + 0.11*(max-y);
	else {
		int cmyk[4];
		simple_rgb_to_cmyk(colors,cmyk,max);
		cmyk[2]=y;
		simple_cmyk_to_rgb(cmyk,colors,max);
	}

	Updated();
	return Yellow();
}

int ColorBase::Yellow()
{
	if (colortype==LAX_COLOR_CMYK) return colors[2];
	if (colortype==LAX_COLOR_GRAY) return 0;

	int cmyk[4];
	simple_rgb_to_cmyk(colors,cmyk,max);
	return cmyk[2];
}

double ColorBase::Blackf()
{ return Black()/(double)max; }

double ColorBase::Blackf(double b)
{ return Black(int((b)*max+.5))/(double)max; }

int ColorBase::Black(int k)
{
	if (colortype==LAX_COLOR_CMYK) colors[3]=k;
	else if (colortype==LAX_COLOR_GRAY) colors[0]=k;
	else {
		//colors[0]=colors[1]=colors[2]=k;
		int cmyk[4];
		simple_rgb_to_cmyk(colors,cmyk,max);
		cmyk[3]=k;
		simple_cmyk_to_rgb(cmyk,colors,max);
	}

	Updated();
	return Black();
}

int ColorBase::Black()
{
	if (colortype==LAX_COLOR_CMYK) return colors[3];
	if (colortype==LAX_COLOR_GRAY) return colors[0];

	int cmyk[4];
	simple_rgb_to_cmyk(colors,cmyk,max);
	return cmyk[3];
}

double ColorBase::Alphaf()
{ return Alpha()/(double)max; }

double ColorBase::Alphaf(double o)
{ return Alpha(int((o)*max+.5))/(double)max; }

int ColorBase::Alpha(int a)
{
	if (colortype==LAX_COLOR_RGB) colors[3]=a;
	else if (colortype==LAX_COLOR_GRAY) colors[1]=a;
	else colors[4]=a;
	Updated();
	return Alpha();
}

int ColorBase::Alpha()
{
	if (colortype==LAX_COLOR_RGB) return colors[3];
	if (colortype==LAX_COLOR_GRAY) return colors[1];
	return colors[4];
}

int ColorBase::Hue()
{
	int r=Red(),g=Green(),b=Blue();
	int h,s,v;
	simple_rgb_to_hsv(r,g,b,&h,&s,&v, max);
	return h;
}

double ColorBase::Huef()
{ return Hue()/(double)max; }

double ColorBase::Huef(double o)
{ return Hue(int((o)*max+.5))/(double)max; }

int ColorBase::Hue(int h)
{
	int r=Red(),g=Green(),b=Blue();
	int hh,s,v;
	simple_rgb_to_hsv(r,g,b,&hh,&s,&v, max);
	simple_hsv_to_rgb(h,s,v,&r,&g,&b, max);
	Red(r);
	Green(g);
	Blue(b);
	return h;
}

int ColorBase::Saturation()
{
	int r=Red(),g=Green(),b=Blue();
	int h,s,v;
	simple_rgb_to_hsv(r,g,b,&h,&s,&v, max);
	return s;
}

double ColorBase::Saturationf()
{ return Saturation()/(double)max; }

double ColorBase::Saturationf(double o)
{ return Saturation(int((o)*max+.5))/(double)max; }

int ColorBase::Saturation(int s)
{
	int r=Red(),g=Green(),b=Blue();
	int h,ss,v;
	simple_rgb_to_hsv(r,g,b,&h,&ss,&v, max);
	simple_hsv_to_rgb(h,s,v,&r,&g,&b, max);
	Red(r);
	Green(g);
	Blue(b);
	return s;
}

int ColorBase::Value()
{
	int r=Red(),g=Green(),b=Blue();
	int h,s,v;
	simple_rgb_to_hsv(r,g,b,&h,&s,&v, max);
	return v;
}

double ColorBase::Valuef()
{ return Value()/(double)max; }

double ColorBase::Valuef(double o)
{ return Value(int((o)*max+.5))/(double)max; }

int ColorBase::Value(int v)
{
	if (colortype==LAX_COLOR_GRAY) {
		return Black(v);
	} 
	int r=Red(),g=Green(),b=Blue();
	int h,s,vv;
	simple_rgb_to_hsv(r,g,b,&h,&s,&vv, max);
	simple_hsv_to_rgb(h,s,v,&r,&g,&b, max);
	Red(r);
	Green(g);
	Blue(b);
	return v;
}

//! For each field, make sure it is in range [0..max].
void ColorBase::Clamp()
{
	for (int c=0; c<5; c++) {
		if (color1[c]<0) color1[c]=0;
		else if (color1[c]>max) color1[c]=max;

		if (color2[c]<0) color2[c]=0;
		else if (color2[c]>max) color2[c]=max;
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
	return 0;
}

//! Revert colors to oldcolor.
void ColorBase::RestoreColor()
{
	for (int c=0; c<5; c++) colors[c]=oldcolor[c];
	Updated();
}


} // namespace Laxkit



