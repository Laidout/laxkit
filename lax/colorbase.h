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
#ifndef _LAX_COLORBASE_H
#define _LAX_COLORBASE_H


#include <lax/colors.h>

namespace Laxkit {



//------------------------------- ColorBase ------------------------------
class ColorBase
{
  protected:
	
  public:
	int colortype;
	int max;
	int oldcolor[5], color1[5],color2[5];
	int *colors;

	ColorBase(int ctype, int nmax, int c0,int c1,int c2,int c3=-1,int c4=-1);
	virtual ~ColorBase();

	virtual int ColorChanged();
	virtual void Updated();
	virtual void Clamp();
	virtual void RestoreColor();

	virtual void SetMax(int newmax);
	virtual void SetRGB(int r,int g,int b,int a=-1);
	virtual void SetGray(int g,int a=-1);
	virtual void SetCMYK(int c,int m,int y,int k, int a=-1);
	virtual void SetHSV(int h,int s,int v, int a=-1);

	virtual void SetRGB(double r,double g,double b,double a=-1);
	virtual void SetGray(double g,double a=-1);
	virtual void SetCMYK(double c,double m,double y,double k, double a=-1);
	virtual void SetHSV(double h,double s,double v, double a=-1);
	virtual void CMYK(double *cmyk);
	virtual void RGB(double *rgb);
	virtual void HSV(double *hsv);
	virtual char *HexValue(char *buffer);
	virtual int SetHexValue(const char *hex);

	virtual int Red();
	virtual int Red(int r);
	virtual double Redf(double r);
	virtual double Redf();
	virtual int Green();
	virtual int Green(int g);
	virtual double Greenf(double g);
	virtual double Greenf();
	virtual int Blue();
	virtual int Blue(int b);
	virtual double Bluef(double b);
	virtual double Bluef();
	virtual int Gray();
	virtual int Gray(int g);
	virtual double Grayf(double g);
	virtual double Grayf();
	virtual int Cyan();
	virtual int Cyan(int c);
	virtual double Cyanf(double c);
	virtual double Cyanf();
	virtual int Magenta();
	virtual int Magenta(int m);
	virtual double Magentaf(double m);
	virtual double Magentaf();
	virtual int Yellow();
	virtual int Yellow(int y);
	virtual double Yellowf(double y);
	virtual double Yellowf();
	virtual int Black();
	virtual int Black(int k);
	virtual double Blackf(double k);
	virtual double Blackf();
	virtual int Alpha();
	virtual int Alpha(int a);
	virtual double Alphaf(double a);
	virtual double Alphaf();
	virtual int Hue();
	virtual int Hue(int h);
	virtual double Huef(double h);
	virtual double Huef();
	virtual int Saturation();
	virtual int Saturation(int s);
	virtual double Saturationf(double s);
	virtual double Saturationf();
	virtual int Value();
	virtual int Value(int v);
	virtual double Valuef(double v);
	virtual double Valuef();
};



} // namespace Laxkit

#endif 

