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
//    Copyright (C) 2004-2010 by Tom Lechner
//

#define STEPS 255

struct rgb_color {
	unsigned int r,g,b;
}

struct cmyk_color {
	unsigned int c,m,y,k;
}


-----------------------------
cmyk_color rgb_to_cmyk(rgb_color rgb)
{
	cmyk_color cmyk;
	cmyk.b=MIN (MIN (rgb.r,rgb.g), rgb.b);
	cmyk.c=(STEPS-rgb.r)*STEPS/(STEPS-b);
	cmyk.m=(STEPS-rgb.g)*STEPS/(STEPS-b);
	cmyk.y=(STEPS-rgb.b)*STEPS/(STEPS-b);
	return cmyk;
}

hsv_to_*
*_to_hsv

cmyk_to_rgb

bw_to_rgb
rgb_to_bw

bw_to_cmyk
cmyk_to_bw

rgb_to_cmy
cmy_to_rgb

hsv

yuv

COLSTRIP_X
COLSTRIP_Y

COLOR_CMYK
COLOR_RGB
COLOR_HSV
COLOR_YUV
COLOR_BW
COLOR_INDEXED

color bit depth
fg/bg/alpha

