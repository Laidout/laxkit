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
//    Copyright (C) 2011 by Tom Lechner
//
#ifndef _LAX_SCREENCOLOR_H
#define _LAX_SCREENCOLOR_H




namespace Laxkit {

//------------------------------- ScreenColor -------------------------------
class ScreenColor
{
 public:
	int red, green, blue, alpha;
	char pixel_is_synced;
	long pixel;
	int info;

	ScreenColor() { red=green=blue=0; alpha=65535; pixel_is_synced=0; pixel=0; info=0; }
	ScreenColor(unsigned int color);
	ScreenColor(int r, int g, int b, int a)
	  : red(r), green(g), blue(b), alpha(a), pixel_is_synced(0), pixel(0), info(0) {}
	ScreenColor(double r, double g, double b, double a)
	  : red(r*65535), green(g*65535), blue(b*65535), alpha(a*65535), pixel_is_synced(0), pixel(0), info(0) {}

	void gray(int g, int a=0xffff);
	void rgb(int r,int g,int b, int a=0xffff);
	void cmyk(int c, int m, int y, int k, int a=0xffff);

	void gray8(int g, int a=0xff);
	void rgb8(int r,int g,int b, int a=0xff);
	void cmyk8(int c, int m, int y, int k, int a=0xff);

	void grayf(double g, double a=1.0);
	void rgbf(double r,double g,double b, double a=1.0);
	void cmykf(double c, double m, double y, double k, double a=1.0);

	void Set(unsigned int color);

	double Red()   { return red  /65535.; }
	double Green() { return green/65535.; }
	double Blue()  { return blue /65535.; }
	double Alpha() { return alpha/65535.; }

	void AddDiff(double r, double g, double b);
	void Average(ScreenColor *result, const ScreenColor &color, double r);
	void Clamp();

	unsigned long Pixel();
	int equals(ScreenColor &color);
	int equals(double r,double g,double b, double a);
};

} //namespace Laxkit


#endif

