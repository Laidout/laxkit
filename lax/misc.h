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
//    Copyright (C) 2004-2010,2012-2013 by Tom Lechner
//
#ifndef _LAX_MISC_H
#define _LAX_MISC_H

namespace Laxkit {


//--------------------------- Unique things ----------------------------------------
unsigned long getUniqueNumber();
char *make_id(const char *base);


//----------------------------- Simple, non color managed color conversions ----------------------------

//-------gray
double simple_rgb_to_grayf(double r,double g,double b);
int simple_rgb_to_gray(int r,int g,int b, int max);


//---------rgb/cmyk 
void simple_rgb_to_cmyk(int r,int g,int b,int *c,int *m,int *y,int *k,int max);
void simple_rgb_to_cmyk(int *rgb, int *cmyk, int max);
void simple_rgb_to_cmyk(double r,double g,double b,double *c,double *m,double *y,double *k);
void simple_rgb_to_cmyk(double *rgb, double *cmyk);

void simple_cmyk_to_rgb(int c,int m,int y,int k,int *r,int *g,int *b,int max);
void simple_cmyk_to_rgb(int *cmyk, int *rgb, int max);
void simple_cmyk_to_rgb(double c,double m,double y,double k,double *r,double *g,double *b);
void simple_cmyk_to_rgb(double *cmyk, double *rgb);


//---------rgb/hsv
void simple_hsv_to_rgb(int h,int s,int v,int *r,int *g,int *b,int max);
void simple_hsv_to_rgb(int *hsv, int *rgb, int max);
void simple_hsv_to_rgb(double h,double s,double v,double *r,double *g,double *b);
void simple_hsv_to_rgb(double *hsv, double *rgb);

void simple_rgb_to_hsv(int r,int g,int b,int *h,int *s,int *v,int max);
void simple_rgb_to_hsv(int *rgb, int *hsv, int max);
void simple_rgb_to_hsv(double r,double g,double b,double *h,double *s,double *v);
void simple_rgb_to_hsv(double *rgb, double *hsv);


//---------rgb/hsl
void simple_hsl_to_rgb(double h, double s, double l, double *r,double *g,double *b);


//---------------------- some debugging helpers ------------------------------
void dump_flags(unsigned int f);

} //namespace Laxkit

#endif

