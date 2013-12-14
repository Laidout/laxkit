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
//    Copyright (C) 2004-2010,2012-2013 by Tom Lechner
//

#include <lax/misc.h>
#include <lax/laxdefs.h>
#include <lax/strmanip.h>

#include <cstdio>
using namespace std;


#define Min(a,b,c) (((a)<(b)?(a):(b))<(c)?((a)<(b)?(a):(b)):(c))
#define Max(a,b,c) (((a)>(b)?(a):(b))>(c)?((a)>(b)?(a):(b)):(c))

namespace Laxkit {
	
//! Return a unique unsigned long.
/*! \ingroup misc
 * Keeps a static unsigned long, and increments it by 1 on each call. Starts at 0.
 *
 * \#include <lax/misc.h>
 */ 
unsigned long getUniqueNumber() 
{ 
	static unsigned long uniquenumber=0;
	return uniquenumber++; 
}

//! Return a roughly unique id. Uniqueness is not guaranteed!
/*! Say base=="blah" then something like "blah12" will be returned.
 *
 * This currently uses Laxkit::getUniqueNumber(), and simply appends it to base.
 * Please note that if you load in something, either a laidout document or a resource,
 * it is possible to have name collision.
 */ 
char *make_id(const char *base)
{
    if (!base) base="id";
    char *str=new char[strlen(base)+30];
    sprintf(str,"%s%ld",base,getUniqueNumber());
    return str;
}


//----------------------------- Simple, non color managed color conversions ----------------------------

//-------------- rgb to cmyk

//! Convert an rgb color to cmyk.
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_rgb_to_cmyk(double r,double g,double b,double *c,double *m,double *y,double *k)
{
	*k=Min(r,g,b);
	if (1.-*k) {
		 //partial black, scale other components
		double d=1./(1.-*k);
		*c=(1.-*k-r)*d;
		*m=(1.-*k-g)*d;
		*y=(1.-*k-b)*d;
	} else {
		 //full black
		*c=1.-r;
		*m=1.-g;
		*y=1.-b;
	}
	if (*c<0) *c=0; else if (*c>1.) *c=1.;
	if (*m<0) *m=0; else if (*m>1.) *m=1.;
	if (*y<0) *y=0; else if (*y>1.) *y=1.;
	if (*k<0) *k=0; else if (*k>1.) *k=1.;
}

//! Convert an rgb color to cmyk. Fields in range 0..1.
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_rgb_to_cmyk(double *rgb, double *cmyk)
{ simple_rgb_to_cmyk(rgb[0],rgb[1],rgb[2], cmyk,cmyk+1,cmyk+2,cmyk+3); }

//! Convert an rgb color to cmyk. Max is maximum value of the field, ie 255 for 8 bit, or 65535 for 16 bit.
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_rgb_to_cmyk(int r,int g,int b,int *c,int *m,int *y,int *k,int max)
{
	*k=MIN (MIN (r,g), b);
	if (max-*k) {
		double d=((double)max)/(max-*k);
		*c=(max-*k-r)*d;
		*m=(max-*k-g)*d;
		*y=(max-*k-b)*d;
	} else {
		*c=max-r;
		*m=max-g;
		*y=max-b;
	}
	if (*c<0) *c=0; else if (*c>max) *c=max;
	if (*m<0) *m=0; else if (*m>max) *m=max;
	if (*y<0) *y=0; else if (*y>max) *y=max;
	if (*k<0) *k=0; else if (*k>max) *k=max;
}

//! Convert an rgb color to cmyk. Max is maximum value of the field, ie 255 for 8 bit, or 65535 for 16 bit.
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_rgb_to_cmyk(int *rgb, int*cmyk, int max)
{ simple_rgb_to_cmyk(rgb[0],rgb[1],rgb[2], cmyk,cmyk+1,cmyk+2,cmyk+3, max); }


//-------------- cmyk to rgb

//! Convert a cmyk color to rgb with components [0..1].
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_cmyk_to_rgb(double c,double m,double y,double k,double *r,double *g,double *b)
{
	*r=(1.-k)*(1.-c);
	*g=(1.-k)*(1.-m);
	*b=(1.-k)*(1.-y);

	if (*r<0) *r=0; else if (*r>1.) *r=1.;
	if (*g<0) *g=0; else if (*g>1.) *g=1.;
	if (*b<0) *b=0; else if (*b>1.) *b=1.;
}

//! Convert a cmyk color to rgb. Max is maximum value of the field, ie 255 for 8 bit, or 65535 for 16 bit.
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_cmyk_to_rgb(int c,int m,int y,int k,int *r,int *g,int *b,int max)
{
	*r=(max-k)*(max-c)/(double)max;
	*g=(max-k)*(max-m)/(double)max;
	*b=(max-k)*(max-y)/(double)max;

	if (*r<0) *r=0; else if (*r>max) *r=max;
	if (*g<0) *g=0; else if (*g>max) *g=max;
	if (*b<0) *b=0; else if (*b>max) *b=max;
}

//! Convert a cmyk color to rgb. Max is maximum value of the field, ie 255 for 8 bit, or 65535 for 16 bit.
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_cmyk_to_rgb(int *cmyk, int *rgb, int max)
{ simple_cmyk_to_rgb(cmyk[0],cmyk[1],cmyk[2],cmyk[3], rgb,rgb+1,rgb+2, max); }

//! Convert a cmyk color to rgb.
/*! \ingroup misc
 * Warning, does no NULL pointer check, or bounds check.
 */
void simple_cmyk_to_rgb(double *cmyk, double *rgb)
{ simple_cmyk_to_rgb(cmyk[0],cmyk[1],cmyk[2],cmyk[3], rgb,rgb+1,rgb+2); }



//--------------------- rgb to hsv

/*! \ingroup misc
 * rgb all range [0..1].
 */
void simple_rgb_to_hsv(double r,double g,double b,double *h,double *s,double *v)
{
	double min=Min(r,g,b);
	double max=Max(r,g,b);
	double d=max-min;

	*v=max;

	if (d==0) {
		//pure gray
		*h=0;
		*s=0;
	} else {
	   *s=d/max;

		double dr=(((max-r) / 6) + (max/2)) / max;
		double dg=(((max-g) / 6) + (max/2)) / max;
		double db=(((max-b) / 6) + (max/2)) / max;

		if      (r==max) *h=db - dg;
		else if (g==max) *h=(1./3) + dr - db;
		else if (b==max) *h=(2./3) + dg - dr;

		if (*h<0) *h+=1;
		if (*h>1) *h-=1;
	}

	if (*h<0) *h=0; else if (*h>1) *h=1;
	if (*s<0) *s=0; else if (*s>1) *s=1;
	if (*v<0) *v=0; else if (*v>1) *v=1;
}

/*! \ingroup misc
 * rgb all range [0..max].
 */
void simple_rgb_to_hsv(int r,int g,int b,int *h,int *s,int *v,int max)
{
	double hh,ss,vv;
	simple_rgb_to_hsv(r/(double)max,g/(double)max,b/(double)max, &hh,&ss,&vv);
	*h=hh*max;
	*s=ss*max;
	*v=vv*max;

	if (*h<0) *h=0; else if (*h>max) *h=max;
	if (*s<0) *s=0; else if (*s>max) *s=max;
	if (*v<0) *v=0; else if (*v>max) *v=max;
}

/*! \ingroup misc
 * rgb all range [0..1].
 */
void simple_rgb_to_hsv(int *rgb, int *hsv, int max)
{ simple_rgb_to_hsv(rgb[0],rgb[1],rgb[2], hsv,hsv+1,hsv+2, max); }

/*! \ingroup misc
 * rgb all range [0..1].
 */
void simple_rgb_to_hsv(double *rgb, double *hsv)
{ simple_rgb_to_hsv(rgb[0],rgb[1],rgb[2], hsv,hsv+1,hsv+2); }




//--------------------- hsv to rgb


/*! \ingroup misc
 * hsv all range [0..1].
 */
void simple_hsv_to_rgb(double h,double s,double v,double *r,double *g,double *b)
{
	if (s==0) {
		*r=v;
		*g=v;
		*b=v;

	} else {
		double hh=h*6;
		if (hh==6) hh=0;
		double hint = int(hh);
		double v1 = v*(1-s);
		double v2 = v*(1-s * (hh-hint));
		double v3 = v*(1-s * (1 -(hh-hint)));

		if      ( hint == 0 ) { *r = v ; *g = v3; *b = v1; }
		else if ( hint == 1 ) { *r = v2; *g = v ; *b = v1; }
		else if ( hint == 2 ) { *r = v1; *g = v ; *b = v3; }
		else if ( hint == 3 ) { *r = v1; *g = v2; *b = v ; }
		else if ( hint == 4 ) { *r = v3; *g = v1; *b = v ; }
		else                  { *r = v ; *g = v1; *b = v2; }
	}
}

/*! \ingroup misc
 * hsv all range [0..1].
 */
void simple_hsv_to_rgb(int h,int s,int v,int *r,int *g,int *b,int max)
{
	double rr,gg,bb;
	simple_hsv_to_rgb(h/(double)max,s/(double)max,v/(double)max, &rr,&gg,&bb);
	*r=rr*max;
	*g=gg*max;
	*b=bb*max;
}

/*! \ingroup misc
 * hsv all range [0..1].
 */
void simple_hsv_to_rgb(int *hsv, int *rgb, int max)
{ simple_hsv_to_rgb(hsv[0],hsv[1],hsv[2], rgb,rgb+1,rgb+2,max);
}

/*! \ingroup misc
 * hsv all range [0..1].
 */
void simple_hsv_to_rgb(double *hsv, double *rgb)
{ simple_hsv_to_rgb(hsv[0],hsv[1],hsv[2], rgb,rgb+1,rgb+2); }




} //namespace Laxkit

