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
//    Copyright (C) 2012 by Tom Lechner
//
#ifndef _LAX_DISPLAYER_XLIB_H
#define _LAX_DISPLAYER_XLIB_H

#include <lax/anxapp.h>
#include <lax/vectors.h>
#include <lax/pancontroller.h>
#include <lax/panuser.h>
#include <lax/laxutils.h>
#include <lax/doublebbox.h>
#include <lax/laximages.h>
#include <lax/screencolor.h>

#define DISPLAYER_NO_SHEAR (1<<0)

namespace Laxkit {
	
class DisplayerXlib : public Displayer
{
  private:
	void base_init();

  protected:
	 //not base displayer:
	Display *dpy;
	Window w;
	Visual *vis;
	GC gc;
	int isinternal;

	XftFont *xfont;
	XftFont *tempfont;
	LaxFont *laxfont;
    //XftColor textxftcolor;
    XftDraw *textxftdraw;

	Region clipregion;
	Pixmap clipmask;
	NumStack<Region> clipstack;
	virtual int Clip(Region newregion, int append);

	NumStack<flatpoint> curpoints;
	NumStack<int> multiplepaths;
	NumStack<int> multipleclosedpaths;
	XPoint *xpoints; //of current path(s)
	int needtobuildxpoints;
	int maxxpoints_allocated;
	int numxpoints;
	int num_bez_div;
	virtual void buildXPoints();

	double *ctm,ictm[6];
	PtrStack<double> axesstack;
	unsigned long fgcolor,bgcolor;
	ScreenColor fg,bg;
	LaxCompositeOp drawmode;
	double linewidth;
	int curcap;
	int curjoin;
	int curdash;
 
  public:
	DisplayerXlib(aDrawable *d);
	DisplayerXlib(anXWindow *w=NULL,PanController *pan=NULL);
	virtual ~DisplayerXlib();
	virtual Displayer *duplicate();

	/*! \name Window and Xlib specific helper functions: */
	//@{
	virtual Display *GetDpy() { return dpy; }
	virtual aDrawable *GetXw() { return xw; }
	virtual Drawable GetXDrawable() { return w; }
	virtual GC GetGC() { return gc; }
	XftDraw *textdraw(Window xlib_window);
	//@}

	/*! \name Buffer Management */
	//@{ 
	virtual void SwapBuffers();
	virtual void BackBuffer(int on);
	virtual int StartDrawing(aDrawable *buffer);
	virtual int MakeCurrent(aDrawable *buffer);
	virtual int MakeCurrent(LaxImage *buffer);
	virtual int CurrentResized(aDrawable *buffer, int nwidth,int nheight);
	virtual int ClearDrawable(aDrawable *drawable);
	virtual int CreateSurface(int width,int height, int type=-1);
	virtual LaxImage *GetSurface();
	virtual int ResizeSurface(int width, int height);
	virtual int EndDrawing();
	//@}

	/*! \name State */
	//@{
	virtual unsigned long NewFG(double r,double g,double b,double a=1.0);
	virtual unsigned long NewFG(unsigned long ncol);
	virtual unsigned long NewFG(const ScreenColor *col);
	virtual unsigned long NewFG(Color *col);
	virtual unsigned long NewFG(int r,int g,int b,int a=255);
	virtual unsigned long NewBG(double r,double g,double b,double a=1.0);
	virtual unsigned long NewBG(int r,int g,int b,int a=255);
	virtual unsigned long NewBG(unsigned long nc);
	virtual unsigned long NewBG(const ScreenColor *col);
	virtual unsigned long FG() { return fgcolor; }
	virtual unsigned long BG() { return bgcolor; }
	virtual double LineWidth(double newwidth);
	virtual double LineWidthScreen(double newwidth);
	virtual void LineAttributes(double width,int dash,int cap,int join);
	virtual void FillAttributes(int fillstyle, int fillrule);
	virtual LaxCompositeOp BlendMode(LaxCompositeOp mode);
	virtual double setSourceAlpha(double alpha);

	virtual bool Capability(DisplayerFeature what);
	virtual void setLinearGradient(int extend, double x1,double y1, double x2,double y2, double *offsets, ScreenColor *colors, int n);
	virtual void setRadialGradient(int extend, double x1,double y1, double r1, double x2,double y2, double r2, double *offsets, ScreenColor *colors, int n);
	virtual void setMesh(int numrows, int numcolumns, flatpoint *points, ScreenColor *colors);
	//@}


	/*! \name Main drawing functions: */
	//@{
	virtual void ClearWindow();
	virtual void ClearTransparent();
	virtual flatpoint realtoscreen(flatpoint p);
	virtual flatpoint realtoscreen(double x,double y);
	virtual flatpoint screentoreal(int x,int y);
	virtual flatpoint screentoreal(flatpoint p);

	virtual int Clip(flatpoint *p, int n, int append);//install a clip mask from a polyline (line is automatically closed)
	virtual int Clip(bool append);//install a clip mask from current path
	virtual void PushClip(int startfresh); //push the current clip mask onto a stack, make a new one maybe
	virtual void PopClip(); //restore a previous mask
	virtual void ClearClip(); //remove any mask
	virtual int activeMask(); //return whether there is an active mask

	//path drawing and filling
	virtual int DrawReal(); //any subsequent calls are using real coordinates
	virtual int DrawScreen(); //any subsequent calls are using screen coordinates
	virtual void DrawOnMask();
	virtual void DrawOnSrc();

	virtual void show(); //collapse source through mask onto surface
	virtual void fill(int preserve);
	virtual void stroke(int preserve);
	virtual void moveto(flatpoint p);
	virtual void lineto(flatpoint p);
	virtual void curveto(flatpoint c1,flatpoint c2,flatpoint v);
	virtual void closed();
	virtual void closeopen();
	virtual void drawpixel(flatpoint p);
	virtual void drawpoint(double x,double y,double radius,int fill);  //draw filled circle radius r
	virtual void drawlines(flatpoint *points,int npoints,char closed,char fill);
	virtual void drawline(flatpoint p1,flatpoint p2);
	virtual void drawline(double ax,double ay,double bx,double by);

	//draw text
	virtual int textheight();
	virtual int font(LaxFont *nfont, double size=-1);
	virtual int font(const char *fontconfigpattern);
	virtual int font(const char *family,const char *style,double pixelsize);
	virtual int fontsize(double size);
	virtual double textextent(LaxFont *thisfont, const char *str,int len, double *width,double *height,double *ascent,double *descent,char real);
	virtual double textout_line(double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double textout(double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double textout(double *matrix,double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double textout(double angle, double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double glyphsout(double x,double y, GlyphPlace *glyphs,GlyphPlace **glyphsp,unsigned int len, unsigned long align=LAX_CENTER);
	virtual double glyphsextent(GlyphPlace *glyphs,GlyphPlace **glyphsp,unsigned int len, double *width,double *height, bool real=false);

	//draw images
	virtual int  imageout(LaxImage *image, double x,double y, double w,double h);
	virtual void imageout(LaxImage *img,double x,double y);
	virtual void imageout(LaxImage *img,double *matrix);
	virtual void imageout(LaxImage *img,double angle, double x,double y);
	virtual void imageout_rotated(LaxImage *img,double x,double y,double ulx,double uly);
	virtual void imageout_skewed(LaxImage *img,double ulx,double uly,double urx,double ury,double llx,double lly);


	/*! \name Viewport maintenance functions: */
	 //@{
	virtual double *m() { return ctm; }
	virtual const double *Getctm() { return ctm; }
	virtual const double *Getictm() { return ictm; }
	virtual void findictm();

	virtual void ShiftScreen(double dx,double dyy);
	virtual void ShiftReal(double dx,double dy);
	virtual void ResetTransform();
	virtual void NewTransform(const double *d);
	virtual void NewTransform(double a,double b,double c,double d,double x0,double y0);
	virtual void PushAndNewTransform(const double *m);
	virtual void PushAndNewAxes(flatpoint p,flatpoint x,flatpoint y);
	virtual void PushAxes();
	virtual void PopAxes();
	virtual void Newangle(double angle,int dir=0,int dec=1);
	virtual void Rotate(double angle,double x,double y,int dec=1);
	virtual void Newmag(double xs,double ys=-1);
	virtual void Zoomr(double m,flatpoint p);
	virtual void Zoom(double m,int x,int y);
	virtual void Zoom(double m);
	virtual double Getmag(int c=0);
	virtual double GetVMag(int x,int y);
	 //@}
};

} // namespace Laxkit

#endif
