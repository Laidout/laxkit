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
//    Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef _LAX_LAXUTILS_H
#define _LAX_LAXUTILS_H


#include <lax/screencolor.h>
#include <lax/anxapp.h>
#include <lax/laximages.h>
#include <lax/vectors.h>
#include <lax/doublebbox.h>
#include <lax/displayer.h>
#include <lax/drawingdefs.h>


namespace Laxkit {


 //drawing basics
int StringToLaxop(const char *str);
char *LaxopToString(int function, char *str_ret, int len, int *len_ret);

void draw_special_color(Displayer *dp, int which, double square, double x, double y, double w, double h);
flatpoint *draw_thing_coordinates(DrawThingTypes thing, flatpoint *buffer, int buffer_size, int *n_ret,double scale=1,DoubleBBox *bounds=NULL);


 //color utitilies
ScreenColor *coloravg(ScreenColor *result, ScreenColor *a, ScreenColor *b,float r=.5);
unsigned long coloravg(const ScreenColor &a, const ScreenColor &b,float r=.5);
unsigned long pixelfromcolor(ScreenColor *col);
unsigned long standoutcolor(const Laxkit::ScreenColor &color, bool bw, ScreenColor *color_ret = nullptr);
void StandoutColor(const Laxkit::ScreenColor &color, bool bw, Laxkit::ScreenColor &color_ret);

unsigned long coloravg(unsigned long a,unsigned long b,float r=.5);
void colorrgb(unsigned long col,int *r,int *g,int *b,int *a=NULL);
unsigned long rgbcolor(int r,int g,int b,int a = 255);
unsigned long rgbcolorf(double r,double g,double b);
void set_color_shift_info(unsigned int rm, unsigned int gm, unsigned int bm, unsigned int am);


 //mouse and coordinate utilities
int translate_window_coordinates(anXWindow *from, int x, int y, anXWindow *to, int *xx, int *yy, anXWindow **kid);
void screen_coordinates(int x, int y, anXWindow *window, int *screen_x_ret, int *screen_y_ret);
int mouseposition(int mouse_id, anXWindow *win, int *x, int *y, unsigned int *state,anXWindow **child,int *screen=NULL, ScreenInformation **monitor=NULL);
//Window xouseposition(XID mouse_id, int *x, int *y, unsigned int *state,anXWindow **child,Window *childw);
int mouseisin(int mouse_id, anXWindow *win);
unsigned long screen_color_at_mouse(int mouse_id, int *error_ret = nullptr);


 //text utilities

#define LAX_ICON_ONLY        (0)
#define LAX_TEXT_ONLY        (1)
#define LAX_TEXT_ICON        (2)
#define LAX_ICON_TEXT        (3)
#define LAX_ICON_OVER_TEXT   (4)
#define LAX_TEXT_OVER_ICON   (5)
#define LAX_ICON_STYLE_MASK  (7)
#define LAX_WAY_OFF          (-1000000)

void get_placement(LaxImage *image, LaxFont *font, const char *label,int gap,unsigned int how,
					int *w,int *h,int *tx,int *ty,int *ix,int *iy, double icon_height=0, double ui_scale = 1.0);
void get_placement(int thingw, int thingh, LaxFont *font,const char *label,int gap,unsigned int how,
					int *w,int *h,int *tx,int *ty,int *ix,int *iy, double ui_scale = 1.0);
void get_placement(int thingw, int thingh, LaxFont *font,const char *label,int gap,unsigned int how,
					double *w,double *h,double *tx,double *ty,double *ix,double *iy, double ui_scale = 1.0);

const char *flow_name(int direction);
const char *flow_name_translated(int direction);
int flow_id(const char *direction);

double MaxExtent(LaxFont *font, ...);
double MaxExtent(const char **strs, int n, LaxFont *font);


} //namespace Laxkit

#endif

