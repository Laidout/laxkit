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

LaxCompositeOp drawing_function(LaxCompositeOp mode);
void drawing_line_attributes(double width, int type, int cap, int join);
//ScreenColor forground_color(ScreenColor &color);
//ScreenColor background_color(ScreenColor &color);
void foreground_color(double r,double g,double b,double a=1);
void background_color(double r,double g,double b);
unsigned long foreground_color(unsigned long newcolor);
unsigned long background_color(unsigned long newcolor);

void clear_window(anXWindow *win);
void draw_rectangle(aDrawable *win, double x, double y, double w, double h);
void fill_rectangle(aDrawable *win, double x, double y, double w, double h);
void draw_line(aDrawable *win, double x1,double y1, double x2,double y2);
void draw_arc(aDrawable *win, double x,double y, double xradius, double yradius, double start_radians, double end_radians);
void draw_arc_wh(aDrawable *win, double x,double y, double width, double height, double start_radians, double end_radians);
void fill_arc(aDrawable *win, double x,double y, double xradius, double yradius, double start_radians, double end_radians);
void fill_arc_wh(aDrawable *win, double x,double y, double width, double height, double start_radians, double end_radians);
void draw_lines(aDrawable *win, flatpoint *p, int n, int isclosed);
void fill_polygon(aDrawable *win, flatpoint *p, int n);
void fill_faux_transparent(aDrawable *win, ScreenColor &color, int x, int y, int w, int h, int square);

int draw_thing(aDrawable *win,double x, double y, double rx, double ry, int fill, DrawThingTypes thing);
int draw_thing(aDrawable *win,double x, double y, double rx, double ry, DrawThingTypes thing,unsigned long fg,unsigned long bg,int lwidth=1);
flatpoint *draw_thing_coordinates(DrawThingTypes thing, flatpoint *buffer, int buffer_size, int *n_ret,double scale=1,DoubleBBox *bounds=NULL);
void draw_special_color(aDrawable *win, int which, double square, double x, double y, double w, double h);
void fill_with_transparency(aDrawable *win, ScreenColor &color, double square, double x,double y,double w,double h);

 //color utitilies
ScreenColor *coloravg(ScreenColor *result, ScreenColor *a, ScreenColor *b,float r=.5);
unsigned long pixelfromcolor(ScreenColor *col);
unsigned long standoutcolor(const Laxkit::ScreenColor &color, bool bw);

unsigned long coloravg(unsigned long a,unsigned long b,float r=.5);
void colorrgb(unsigned long col,int *r,int *g,int *b,int *a=NULL);
unsigned long rgbcolor(int r,int g,int b);
unsigned long rgbcolorf(double r,double g,double b);
void set_color_shift_info(unsigned int rm, unsigned int gm, unsigned int bm, unsigned int am);


 //mouse and coordinate utilities
int translate_window_coordinates(anXWindow *from, int x, int y, anXWindow *to, int *xx, int *yy, anXWindow **kid);
int mouseposition(int mouse_id, anXWindow *win, int *x, int *y, unsigned int *state,anXWindow **child,int *screen=NULL);
//Window xouseposition(XID mouse_id, int *x, int *y, unsigned int *state,anXWindow **child,Window *childw);
int mouseisin(XID mouse_id, anXWindow *win);
unsigned long screen_color_at_mouse(int mouse_id);


 //text utilities

#define LAX_ICON_ONLY        (0)
#define LAX_TEXT_ONLY        (1)
#define LAX_TEXT_ICON        (2)
#define LAX_ICON_TEXT        (3)
#define LAX_ICON_OVER_TEXT   (4)
#define LAX_TEXT_OVER_ICON   (5)
#define LAX_ICON_STYLE_MASK  (7)
#define LAX_WAY_OFF          (-1000000)

const char *flow_name(int direction);
const char *flow_name_translated(int direction);
int flow_id(const char *direction);

double text_height();
LaxFont *get_default_font();
double getextent(const char *str,int len, double *ex,double *ey,double *fasc=NULL,double *fdes=NULL,char r=0);
double getextent(LaxFont *font, const char *str,int len,double *ex,double *ey,double *fasc,double *fdes,char r=0);

double textout(aDrawable *win,const char *thetext,int len,double x,double y,unsigned long align);
double textout(aDrawable *win,LaxFont *font,const char *thetext,int len,double x,double y,unsigned long align);
double textout_rotated(aDrawable *win, double radians,const char *thetext,int len,double x,double y,unsigned long align);
double textout_rotated(aDrawable *win, LaxFont *font, double radians,const char *thetext,int len,double x,double y,unsigned long align);
double textout_matrix(aDrawable *win, double *m,const char *thetext,int len,double x,double y,unsigned long align);

double textout_multiline(aDrawable *win,const char *thetext,int len,double x,double y,unsigned long align);

void get_placement(LaxImage *image,const char *label,int gap,unsigned int how,
					int *w,int *h,int *tx,int *ty,int *ix,int *iy);
void get_placement(int thingw, int thingh,const char *label,int gap,unsigned int how,
					int *w,int *h,int *tx,int *ty,int *ix,int *iy);





} //namespace Laxkit

#endif

