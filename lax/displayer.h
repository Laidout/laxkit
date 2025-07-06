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
#ifndef _LAX_DISPLAYER_H
#define _LAX_DISPLAYER_H

#include <lax/anxapp.h>
#include <lax/vectors.h>
#include <lax/pancontroller.h>
#include <lax/panuser.h>
#include <lax/colors.h>
#include <lax/doublebbox.h>
#include <lax/laximages.h>
#include <lax/screencolor.h>
#include <lax/drawingdefs.h>

#define DISPLAYER_NO_SHEAR (1<<0)

namespace Laxkit {
	

class GlyphPlace;
class LaxFont;

//----------------------------------- Displayer -----------------------------
enum DisplayerFeature {
	 //see Displayer::Capability()
	DRAWS_LinearGradient,
	DRAWS_RadialGradient,
	DRAWS_MeshGradient,

	 //other draw flags
	DRAWS_Screen,
	DRAWS_Hires,
	DRAWS_Preview,

	DRAWS_MAX
};

class Displayer : public PanUser, virtual public anObject
{
 protected:

	 //base displayer:
	anXWindow *xw;
	aDrawable *dr;

	int on; //is displayer valid for drawing
	char updatepanner;
	double spaceminx,spacemaxx,spaceminy,spacemaxy; // workspace bounds
	
	char draw_immediately;
	char real_coordinates;
	char decimal;
	int num_bez_div;
	bool default_righthanded;
	int render_target;

	Palette *palette;

 public:
	unsigned long displayer_style;

	double upperbound,lowerbound;
	int Minx,Maxx,Miny,Maxy;    // screen coords of viewport bounding box

	Displayer();
	Displayer(aDrawable *d);
	Displayer(anXWindow *w, PanController *pan = nullptr);
	virtual ~Displayer();
	virtual Displayer *duplicate() = 0;

	/*! \name Window specific helper functions: */
	//@{
	virtual aDrawable *GetDrawable() { return dr; }
	//@}

	/*! \name Buffer Management */
	//@{ 
	virtual void SwapBuffers() = 0;
	virtual void BackBuffer(int on) = 0;
	virtual void WrapWindow(anXWindow *nw);
	virtual int StartDrawing(aDrawable *buffer) = 0;
	virtual int MakeCurrent(aDrawable *buffer) = 0;
	virtual int MakeCurrent(LaxImage *buffer) = 0;
	virtual int CurrentResized(aDrawable *buffer, int nwidth,int nheight) = 0;
	virtual int ClearDrawable(aDrawable *drawable) = 0;
	virtual int CreateSurface(int width,int height, int type=-1) = 0;
	virtual LaxImage *GetSurface() = 0;
	virtual int ResizeSurface(int width, int height) = 0;
	virtual int EndDrawing() = 0;
	//@}

	/*! \name State */
	//@{
	virtual unsigned long NewFG(double r,double g,double b,double a=1.0) = 0;
	virtual unsigned long NewFG(unsigned long ncol) = 0;
	virtual unsigned long NewFG(const ScreenColor *col) = 0;
	virtual unsigned long NewFG(const ScreenColor &col);
	virtual unsigned long NewFG(Color *col) = 0;
	virtual unsigned long NewFG(int r,int g,int b,int a=255) = 0;
	virtual unsigned long NewBG(double r,double g,double b,double a=1.0) = 0;
	virtual unsigned long NewBG(int r,int g,int b,int a=255) = 0;
	virtual unsigned long NewBG(unsigned long nc) = 0;
	virtual unsigned long NewBG(const ScreenColor *col) = 0;
	virtual unsigned long NewBG(const ScreenColor &col);
	virtual unsigned long FG() = 0;
	virtual unsigned long BG() = 0;
	virtual double LineWidth() = 0;
	virtual double LineWidth(double newwidth) = 0; //return old
	virtual double LineWidthScreen(double newwidth) = 0; //return old
	virtual void LineAttributes(double width,int dash,int cap,int join) = 0;
	virtual void FillAttributes(int fillstyle, int fillrule) = 0;
	virtual void LineCap(int cap) = 0;
	virtual void LineJoin(int join) = 0;
	virtual void Dashes(double *dashes, int num, double offset) = 0;
	virtual void Dashes(double dashlength);
	virtual LaxCompositeOp BlendMode(LaxCompositeOp mode) = 0;
	virtual double setSourceAlpha(double alpha) = 0;
	virtual double setCurrentAlpha(double alpha) = 0;

	virtual bool Capability(DisplayerFeature what) = 0; //see DisplayerFeature
	virtual int RenderTarget();
	virtual int RenderTarget(int newtarget);
	virtual void setLinearGradient(int extend, double x1,double y1, double x2,double y2, double *offsets, ScreenColor *colors, int n) = 0;
	virtual void setRadialGradient(int extend, double x1,double y1, double r1, double x2,double y2, double r2, double *offsets, ScreenColor *colors, int n) = 0;
	virtual void setMesh(int numrows, int numcolumns, flatpoint *points, ScreenColor *colors) = 0;
	//@}


	/*! \name Main drawing functions: */
	//@{
	virtual void ClearWindow() = 0;
	virtual void ClearTransparent() = 0;
	virtual flatpoint realtoscreen(flatpoint p) = 0;
	virtual flatpoint realtoscreen(double x,double y) = 0;
	virtual flatpoint screentoreal(int x,int y) = 0;
	virtual flatpoint screentoreal(flatpoint p) = 0;
	virtual flatpoint screentorealv(flatvector v);
	virtual flatpoint realtoscreenv(flatvector v);

	virtual int Clip(flatpoint *p, int n, int append) = 0;//install a clip mask from a polyline (line is automatically closed)
	virtual int Clip(bool append) = 0;//install a clip mask from current path
	virtual void PushClip(int startfresh) = 0; //push the current clip mask onto a stack, make a new one maybe
	virtual void PopClip() = 0; //restore a previous mask
	virtual void ClearClip() = 0; //remove any mask
	virtual int activeMask() = 0; //return whether there is an active mask

	virtual void PushGroup() = 0; //start operations that will be converted to a paint source on PopGroup()
	virtual void PopGroup() = 0;

	//path drawing and filling
	virtual int DrawReal(); //any subsequent calls are using real coordinates
	virtual int DrawScreen(); //any subsequent calls are using screen coordinates
	virtual void Radians(); //interpret all angles as radians
	virtual void Degrees(); //interpret all angles as degrees
	virtual void DrawOnMask();
	virtual void DrawOnSrc();
	virtual int DrawImmediately(int yes); //do not append path operations, draw them with each call
	virtual int onscreen(double x,double y);

	virtual void show() = 0; //collapse source through mask onto surface
	virtual void fillAndStroke(int preserve);
	virtual void fill(int preserve) = 0;
	virtual void stroke(int preserve) = 0;
	virtual void moveto(double x,double y) { moveto(flatpoint(x,y)); }
	virtual void moveto(flatpoint p) = 0;
	virtual void lineto(double x,double y) { lineto(flatpoint(x,y)); }
	virtual void lineto(flatpoint p) = 0;
	virtual void curveto(flatpoint c1,flatpoint c2,flatpoint v) = 0;
	virtual void closed() = 0;
	virtual void closeopen() = 0;
	virtual void drawpixel(flatpoint p) = 0;
	virtual void drawpoint(double x,double y,double radius,int fill) = 0;  //draw circle screen radius r
	virtual void drawpoint(flatpoint p,double radius,int fill); //draw circle screen radius r
	virtual void drawlines(flatpoint *points,int npoints,char closed,char fill) = 0;
	virtual void drawline(flatpoint p1,flatpoint p2) = 0;
	virtual void drawline(double ax,double ay,double bx,double by);
	virtual void drawrectangle(double x,double y,double w,double h,int tofill);
	virtual void drawrectangle(const DoubleBBox &r,int tofill) { drawrectangle(r.minx,r.miny, r.maxx-r.minx,r.maxy-r.miny, tofill); }
	virtual void drawRoundedRect(double x,double y,double w,double h,
                                double vround, bool vispercent, double hround, bool hispercent,
                                int tofill, int whichcorners=0xf);
	virtual void drawbez(flatpoint *bpoints,int n,int isclosed=0,int tofill=0);
	virtual void drawFormattedPoints(flatpoint *pts, int n, int tofill);
	virtual int drawrealline(flatline &ln,int num);
	virtual void drawcircle(double x,double y,double radius,int fill);
	virtual void drawcircle(flatpoint p,double radius,int fill);
	virtual void drawellipseWH(double x,double y,double xradius,double yradius,double start_angle=0,double end_angle=0,int fill=0);
	virtual void drawellipse(double x,double y,double xradius,double yradius,double start_angle=0,double end_angle=0,int fill=0);
	virtual void drawellipse(flatpoint p,double xradius,double yradius,double start_angle=0,double end_angle=0,int fill=0);
	virtual void drawarc(flatpoint p,double xr,double yr,double start_angle=0,double end_angle=0);
	virtual void drawfocusellipse(flatpoint focus1,flatpoint focus2, double c,
								double start_angle=0,double end_angle=0,int fill=0, int wedge=2);
	virtual void drawCheckerboard(double x,double y,double w,double h, double square, double offsetx,double offsety);
	virtual void drawBevel(double bevel, ScreenColor *highlight, ScreenColor *shadow, int state,double x,double y,double w,double h);
	virtual void drawBevel(double bevel,unsigned long highlight,unsigned long shadow, int state,double x,double y,double w,double h);

	//draw things
	virtual void drawthing(flatpoint p, double rx, double ry, int tofill, DrawThingTypes thing, double angle_radians=0);
	virtual void drawthing(double x, double y, double rx, double ry, int tofill, DrawThingTypes thing, double angle_radians=0);
	virtual void drawthing(double x, double y, double rx, double ry, DrawThingTypes thing,unsigned long fg,unsigned long bg,int lwidth=1, double angle_radians=0);
	virtual void drawthing(flatpoint p, double rx, double ry, DrawThingTypes thing,unsigned long fg,unsigned long bg,int lwidth=1, double angle_radians=0);
	virtual void drawarrow(flatpoint p,flatpoint v,double rfromp=0,double len=10,char reallength=1,int portion=3,bool center=false);
	virtual void drawaxes(double len=1); //draw axes with real length at the origin
	virtual void drawnum(double x, double y, int num); //write out the text of a number at the given coordinates.

	//draw text
	virtual int SetPalette(Palette *npalette);
	virtual int font(LaxFont *nfont, double size=-1) = 0;
	virtual int font(const char *fontconfigpattern) = 0;
	virtual int font(const char *family,const char *style,double size) = 0;
	virtual int fontsize(double size) = 0;
	virtual int textheight() = 0;
	virtual double textextent(LaxFont *thisfont, const char *str,int len, double *width,double *height,double *ascent,double *descent,char real) = 0;
	virtual double textextent(const char *str,int len, double *width,double *height,double *ascent=NULL,double *descent=NULL,char real=0);
	virtual double textout_halo(double offset, double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double textout_bg(double pad, double x,double y,const char *str,int len = 0, unsigned long align = LAX_CENTER, bool rounded = true);
	virtual double textout(flatpoint p,const char *str,int len=0,unsigned long align=LAX_CENTER);
	virtual double textout(double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER) = 0;
	virtual double textout(double *matrix,double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER) = 0;
	virtual double textout(double angle, double x,double y,const char *str,int len=0,unsigned long align=LAX_CENTER) = 0;
	virtual double glyphsout(double x,double y, GlyphPlace *glyphs,GlyphPlace **glyphsp,unsigned int numglyphs, unsigned long align=LAX_CENTER) = 0;
	virtual double glyphsextent(GlyphPlace *glyphs,GlyphPlace **glyphsp,unsigned int numglyphs, double *width,double *height, bool real=false) = 0;

	//draw images
	virtual int  imageout_within(LaxImage *image, double x,double y, double w,double h, DoubleRectangle *rect=NULL, int flip=0);
	virtual int  imageout(LaxImage *image, double x,double y, double w,double h) = 0;
	virtual void imageout(LaxImage *img,double x,double y) = 0;
	virtual void imageout(LaxImage *img,double *matrix) = 0;
	virtual void imageout(LaxImage *img,double angle, double x,double y) = 0;
	virtual void imageout_rotated(LaxImage *img,double x,double y,double ulx,double uly) = 0;
	virtual void imageout_skewed(LaxImage *img,double x,double y,double ulx,double uly,double urx,double ury) = 0;
	 //@}


	/*! \name Viewport maintenance functions: */
	 //@{
	virtual const double *Getctm() = 0;
	virtual const double *Getictm() = 0;
	virtual int righthanded();
	virtual bool defaultRighthanded(bool right);
	virtual bool defaultRighthanded();

	virtual char Updates(char toupdatepanner);
	virtual void syncPanner(int all=0);
	virtual void syncFromPanner(int all=0);
	virtual void UseThisPanner(PanController *npanner);

	virtual void PushAxes() = 0;
	virtual void PopAxes() = 0;
	virtual void ResetTransform() = 0;
	virtual void NewTransform(double a,double b,double c,double d,double x0,double y0) = 0;
	virtual void NewTransform(const double *d);
	virtual void PushAndNewTransform(const double *m);
	virtual void PushAndNewAxes(flatpoint p,flatpoint x,flatpoint y);
	virtual void NewAxis(flatpoint o,flatpoint xtip);
	virtual void NewAxis(flatpoint o,flatvector x,flatvector y);
	virtual void ShiftScreen(double dx,double dy);
	virtual void ShiftReal(double dx,double dy);
	virtual void Center(double minx,double maxx,double miny,double maxy);
	virtual void Center(const double *m,DoubleBBox *bbox);
	virtual void Center(DoubleBBox *bbox);
	virtual void CenterPoint(flatpoint p);
	virtual void CenterReal();
	virtual void Newangle(double angle,int dir=0,int dec=-1);
	virtual void Rotate(double angle, double x,double y,int dec=-1);
	virtual void Zoomr(double m,flatpoint p);
	virtual void Zoom(double m,int x,int y);
	virtual void Zoom(double m);
	virtual flatpoint XAxis();
	virtual flatpoint XAxis(flatpoint xaxis);
	virtual flatpoint YAxis();
	virtual flatpoint YAxis(flatpoint yaxis);
	virtual flatpoint Origin();
	virtual flatpoint Origin(flatpoint origin);
	virtual double Getmag(int y=0);
	virtual double GetVMag(double x,double y);
	virtual void Newmag(double xs,double ys=-1);
	virtual void SetView(double minx,double maxx,double miny,double maxy);
	virtual int SetSpace(double minx,double maxx,double miny,double maxy);
	virtual void GetSpace(double *minx,double *maxx,double *miny, double *maxy);
	virtual void GetTransformedSpace(long *minx,long *maxx,long *miny,long *maxy);
	 //@}
};


//--------------------------------------- Default Renderer Stuff ---------------------------

typedef Displayer *(*NewDisplayerFunc)(aDrawable *w);
extern NewDisplayerFunc newDisplayer;
int SetNewDisplayerFunc(const char *backend);

int SetDefaultDisplayer(Displayer *displayer);
Displayer *GetDefaultDisplayer();

} // namespace Laxkit

#endif
