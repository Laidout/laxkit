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
#ifndef _LAX_COLORBASE_H
#define _LAX_COLORBASE_H


#include <lax/colors.h>

namespace Laxkit {



//------------------------------- ColorBase ------------------------------
class ColorBase
{
  protected:
	
  public:
	int colorspecial;
	int colortype;
	double channel_min[5],channel_max[5];

	int max;
	int oldcolortype;
	int oldcolorspecial;
	double oldcolor[5], color1[5],color2[5];
	double *colors;

	ColorBase();
	ColorBase(int ctype, double c0,double c1,double c2,double c3=-1,double c4=-1, double nmax=65535);
	virtual ~ColorBase();

	virtual int SetColorSystem(int newsystem);
	virtual int ColorChanged();
	virtual void Updated();
	virtual void Clamp();
	virtual void RestoreColor(int swap=1);

	virtual void SetMax(int newmax);

	virtual int SetSpecial(int newspecial);
	virtual int Set(const ScreenColor &color);
	virtual int Set(unsigned long color);
	virtual int Set(Color *color);
	virtual int Set(int newtype, double c0,double c1=-1,double c2=-1,double c3=-1,double c4=-1); 
	virtual void SetRGB(double r,double g,double b,double a=-1);
	virtual void SetGray(double g,double a=-1);
	virtual void SetCMYK(double c,double m,double y,double k, double a=-1);
	virtual void SetHSV(double h,double s,double v, double a=-1);
	virtual void SetHSL(double h,double s,double l, double a=-1);
	virtual void SetLab(double l,double a,double b, double alpha=-1);
	virtual void SetXYZ(double x,double y,double z, double a=-1);

	virtual int Get(int newtype, double *c0,double *c1,double *c2,double *c3,double *c4); 
	virtual void CMYK(double *cmyk);
	virtual void RGB(double *rgb);
	virtual void HSV(double *hsv);
	virtual void HSL(double *hsl);
	virtual void CieLab(double *lab);
	virtual void XYZ(double *xyz);

	virtual bool Parse(const char *buffer, int len, const char **endptr);
	virtual char *HexValue(char *buffer);
	virtual int SetHexValue(const char *hex);


	 //alpha
	virtual double Alpha(double a);
	virtual double Alpha();

	 //rgb
	virtual double Red(double r);
	virtual double Red();
	virtual double Green(double g);
	virtual double Green();
	virtual double Blue(double b);
	virtual double Blue();

	 //gray
	virtual double Gray(double g);
	virtual double Gray();

	 //cmyk
	virtual double Cyan(double c);
	virtual double Cyan();
	virtual double Magenta(double m);
	virtual double Magenta();
	virtual double Yellow(double y);
	virtual double Yellow();
	virtual double Black(double k);
	virtual double Black();

	 //hsv
	virtual double Hue(double h);
	virtual double Hue();
	virtual double HSV_Saturation(double s);
	virtual double HSV_Saturation();
	virtual double Value(double v);
	virtual double Value();

	 //hsl
	//virtual double Hue(double h);
	//virtual double Hue();
	virtual double HSL_Saturation(double s);
	virtual double HSL_Saturation();
	virtual double Lightness(double v);
	virtual double Lightness();

	 //cielab
	virtual double Cie_L(double v);
	virtual double Cie_L();
	virtual double Cie_a(double v);
	virtual double Cie_a();
	virtual double Cie_b(double v);
	virtual double Cie_b();

	 //xyz
	virtual double X(double v);
	virtual double X();
	virtual double Y(double v);
	virtual double Y();
	virtual double Z(double v);
	virtual double Z();
};



} // namespace Laxkit

#endif 

