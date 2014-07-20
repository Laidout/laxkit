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
//    Copyright (C) 2012 by Tom Lechner
//
#ifndef _LAX_DISPLAYER_CAIRO_H
#define _LAX_DISPLAYER_CAIRO_H



#include <lax/configured.h>

#ifdef LAX_USES_CAIRO


#include <cairo/cairo-xlib.h>
#include <cairo/cairo-ft.h>

#include <lax/displayer.h>


namespace Laxkit {

class DisplayerCairo : public Displayer
{
 protected:
	char *buffer;
	int bufferlen;
	virtual int reallocBuffer(int len);

	Display *dpy;  //if any
	Visual *vis;
	Window w;

	LaxCompositeOp blendmode;
	int isinternal;
	cairo_t *cr;
	cairo_surface_t *target;
	cairo_surface_t *surface;
	cairo_surface_t *mask;
	cairo_surface_t *source;
	//PtrStack<cairo_surface_t> groups;

	double ctm[6],ictm[6];
	PtrStack<double> axesstack;

	double fgRed, fgGreen, fgBlue, fgAlpha;
	double bgRed, bgGreen, bgBlue, bgAlpha;

	cairo_font_face_t *curfont;
	cairo_scaled_font_t *curscaledfont;
	double _textheight;
	double _ascent;
	double _descent;

	void base_init();

 public:
	DisplayerCairo(anXWindow *nxw=NULL,PanController *pan=NULL);
	virtual ~DisplayerCairo();
	virtual Displayer *duplicate();

	 /*! \name Window and Xlib specific helper functions: */
	 //@{
	//virtual Display *GetDpy();
	 //@}

	 /*! \name Cairo specific helper functions: */
	 //@{
	virtual cairo_t *GetCairo();
	virtual void setCairoBlendMode(cairo_operator_t mode);
	 //@}

	 /*! \name Buffer Management */
	 //@{ 
	virtual void SwapBuffers();
	virtual void BackBuffer(int on);
	//virtual void WrapWindow(anXWindow *nw);
	virtual int StartDrawing(aDrawable *buffer);
	virtual int MakeCurrent(aDrawable *buffer);
	virtual int ClearDrawable(aDrawable *drawable);
	virtual int CreateSurface(int w,int h, int type=-1);
	virtual LaxImage *GetSurface();
	virtual int ResizeSurface(int width, int height);
	virtual int EndDrawing();
	 //@}

	 /*! \name State */
	 //@{
	virtual unsigned long NewFG(double r,double g,double b,double a=1.0);
	virtual unsigned long NewFG(unsigned long ncol);
	virtual unsigned long NewFG(ScreenColor *col);
	virtual unsigned long NewFG(int r,int g,int b,int a=255);
	virtual unsigned long NewBG(double r,double g,double b);
	virtual unsigned long NewBG(int r,int g,int b);
	virtual unsigned long NewBG(unsigned long nc);
	virtual unsigned long NewBG(ScreenColor *col);
	virtual unsigned long FG();
	virtual unsigned long BG();
	virtual void LineAttributes(double width,int dash,int cap,int join);
	virtual void FillAttributes(int fillstyle, int fillrule);
	virtual LaxCompositeOp BlendMode(LaxCompositeOp mode);
	 //@}


	 /*! \name Main drawing functions: */
	 //@{
	virtual void ClearWindow();
	virtual flatpoint realtoscreen(flatpoint p);
	virtual flatpoint realtoscreen(double x,double y);
	virtual flatpoint screentoreal(int x,int y);
	virtual flatpoint screentoreal(flatpoint p);

	virtual int Clip(flatpoint *p, int n, int append);//install a clip mask from a polyline (line is automatically closed)
	virtual void PushClip(int startfresh); //push the current clip mask onto a stack, make a new one maybe
	virtual void PopClip(); //restore a previous mask
	virtual void ClearClip(); //remove any mask
	virtual int activeMask(); //return whether there is an active mask

	 //path drawing and filling
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
	virtual void drawpoint(double x,double y,double radius,int tofill);  //draw filled circle radius r
	virtual void drawlines(flatpoint *points,int npoints,char isclosed,char tofill);
	virtual void drawline(flatpoint p1,flatpoint p2);
	virtual void drawline(double ax,double ay,double bx,double by);

	 //gradients
	 // ********** Need to standardize this!!!
	virtual void fillgradient();

	 //draw text
	virtual void initFont(); //not from Displayer
	virtual int textheight();
	virtual int font(LaxFont *nfont, double size=-1);
	virtual int font(const char *fontconfigpattern);
	virtual int font(const char *family,const char *style,double pixelsize);
	virtual int fontsize(double size);
	virtual double textextent(LaxFont *thisfont, const char *str,int len, double *width,double *height,double *ascent,double *descent,char real);
	virtual double textout(double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double textout(double *matrix,double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double textout(double angle, double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);

	 //draw images
	virtual void imageout(LaxImage *img,double x,double y);
	virtual int  imageout(LaxImage *img, double x,double y, double w,double h);
	virtual void imageout(LaxImage *img,double *matrix);
	virtual void imageout(LaxImage *img,double angle, double x,double y);
	virtual void imageout_rotated(LaxImage *img,double x,double y,double ulx,double uly);
	virtual void imageout_skewed(LaxImage *img,double x,double y,double ulx,double uly,double urx,double ury);


	 /*! \name Viewport maintenance functions: */
	 //@{
	virtual const double *Getctm();
	virtual const double *Getictm();

	virtual void ShiftScreen(int dx,int dy);
	virtual void NewTransform(const double *d);
	virtual void NewTransform(double a,double b,double c,double d,double x0,double y0);
	virtual void PushAxes();
	virtual void PopAxes();
	virtual int DrawReal(); //any subsequent calls are using real coordinates
	virtual int DrawScreen(); //any subsequent calls are using screen coordinates
	 //@}
};

} // namespace Laxkit

#endif //cairo
#endif


