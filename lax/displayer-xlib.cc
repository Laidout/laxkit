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


#include <lax/lists.cc>
#include <lax/displayer-xlib.h>
#include <lax/fontmanager-xlib.h>
#include <lax/doublebbox.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>

#include <lax/laximages-imlib.h>

#include <cstring>

#include <iostream>
#define DBG 

using namespace std;

namespace Laxkit {


DBG void DANGER() { cerr <<"DANGER!!"<<endl; }

//-------------------------------- DisplayerXlib --------------------------
/*! \class DisplayerXlib
 * \brief Somewhat of a graphics wrapper for Xlib graphics functions.
 *
 * Handles drawing lines, bezier curves, ellipses (in any orientation,
 * not just elongated horizontally and vertically), and viewport scaling, rotating, shifting.
 *
 * DisplayerXlib classes are meant to draw onto random buffers, but also have ability to
 * draw right onto an Xlib drawable. 
 * 
 *
 */
/*! \var NumStack<flatpoint> DisplayerXlib::axesstack
 * \brief The stack of axes. See PushAxes() and PopAxes().
 */
/*! \var double DisplayerXlib::ctm[]
 * \brief Current Transformation Matrix.
 * 
 * This is a six valued array, see intro for discussion.
 * This matrix is not dynamic like ctm. Is computed on the fly as necessary.
 * Only the ctm is pushed and popped.
 */
/*! \var double DisplayerXlib::ictm[6]
 * \brief Inverse Current Transformation Matrix.
 * 
 * This is a six valued array, see intro for discussion.
 */
/*! \fn char DisplayerXlib::onscreen(int x,int y)
 * \brief Return whether screen coordinate (x,y) is within (Minx..Maxx,Miny..Maxy).
 */
/*! \fn double *DisplayerXlib::m()
 * \brief Return pointer to the displayer's transformation matrix.
 */
/*! fn  const double *DisplayerXlib::Getctm()
 * \brief Return a constant pointer to the current transformation matrix.
 */
/*! fn  const double *DisplayerXlib::Getictm()
 * \brief Return a constant pointer to the inverse of the current transformation matrix.
 */

DisplayerXlib::DisplayerXlib(aDrawable *d)
  : Displayer(d)
{
	base_init();
}
	
//! Constructor, set everything to nothing or identity.
/*! upperbound=1000, lowerbound=.00001, ctm/ictm=identity,
 * fgcolor=white, bg=black
 */
DisplayerXlib::DisplayerXlib(anXWindow *nxw,PanController *pan) 
	: Displayer(nxw,pan)
{
	base_init();
}

void DisplayerXlib::base_init()
{
	displayer_style=0;

	isinternal=0;
	updatepanner=1;
	if (xw) {
		dpy=xw->app->dpy;
		vis=xw->app->vis;
		w=xw->xlib_window;
	} else {
		dpy=anXApp::app->dpy;
		vis=anXApp::app->vis;
		w=0;
	}
	fgcolor=~0;
	bgcolor=0;
	drawmode=LAXOP_Source;
	on=0;
	gc=DefaultGC(dpy,0); //dpy,screen
//    defaultgcv=new XGCValues;
//    XSetBackground(dpy,defaultgc,color_panel->bg);
//    XSetForeground(dpy,defaultgc,color_panel->fg);
//    XGetGCValues(dpy,defaultgc,GCFunction|GCPlaneMask|GCForeground|GCBackground|GCLineWidth|
//                GCLineStyle|GCCapStyle|GCJoinStyle|GCFillStyle|GCFillRule|GCTile|GCStipple|GCTileStipXOrigin|
//                GCTileStipYOrigin|GCFont|GCSubwindowMode|GCGraphicsExposures|GCClipXOrigin|GCClipYOrigin|
//                GCDashOffset|GCArcMode,
//                defaultgcv);

	if (anXApp::app) xfont=XftFontOpenName(dpy,0,anXApp::app->controlfontstr);
	else xfont=XftFontOpenName(dpy,0,"sans-12");
	tempfont=NULL;
	laxfont=NULL;
	textxftdraw=NULL;

	ctm=new double[6];
	ctm[1]=ctm[2]=ctm[4]=ctm[5]=0.0;
	ctm[0]=ctm[3]=1.0;
	
	ictm[1]=ictm[2]=ictm[4]=ictm[5]=0.0; // ictm is not dynamic created like ctm
	ictm[0]=ictm[3]=1.0;
	
	clipmask=0;
	clipregion=NULL;

	xpoints=NULL;
	maxxpoints_allocated=0;
	numxpoints=0;
	num_bez_div=30;

	draw_immediately=1;
	real_coordinates=1;
	decimal=0;
}

//! Destructor, delete ctm.
DisplayerXlib::~DisplayerXlib()
{
	delete[] ctm;
	
	if (xpoints) delete[] xpoints;
	if (laxfont) {
		if (dynamic_cast<LaxFontXlib*>(laxfont)->font==xfont) xfont=NULL;
		laxfont->dec_count();
	}
	if (xfont && !tempfont && anXApp::app->dpy) XftFontClose(anXApp::app->dpy, xfont);

	if (w && isinternal) XFreePixmap(anXApp::app->dpy,w);
	if (textxftdraw) XftDrawDestroy(textxftdraw);
}

Displayer *DisplayerXlib::duplicate()
{
	return new DisplayerXlib();
}

//! Flush waiting composite operation.
void DisplayerXlib::show()
{ cout <<"*** imp DisplayerXlib::show()"<<endl; }

void DisplayerXlib::SwapBuffers()
{ cout <<"*** imp DisplayerXlib::swapbuffers()"<<endl; }

//! Turn on or off the usage of double buffering.
void DisplayerXlib::BackBuffer(int on)
{ cout <<"*** imp DisplayerXlib::backbuffer()"<<endl; }

//! Return whether there is an active clipping area.
int DisplayerXlib::activeMask()
{
	return clipregion || clipmask;
}

//! Push the current clip mask onto a stack, make a new one maybe
void DisplayerXlib::PushClip(int startfresh)
{
	clipstack.push(clipregion);
	if (startfresh) {
		ClearClip();
	} else if (clipregion) {
		Region empty=XCreateRegion();
		Region newregion=XCreateRegion();
		XUnionRegion(clipregion,empty,newregion);
		Clip(newregion,0);
		XDestroyRegion(newregion);
	}
}

//! Restore a previous mask
void DisplayerXlib::PopClip()
{
	if (clipregion) { XDestroyRegion(clipregion); clipregion=0; }
	ClearClip();
	Region newregion=clipstack.pop();
	Clip(newregion,0);
	if (newregion) XDestroyRegion(newregion);
}

/*! Calling code must XDestroyRegion(newregion).
 */
int DisplayerXlib::Clip(Region newregion, int append)
{
	if (!append) ClearClip();

	if (newregion) {
		if (!clipregion) clipregion=XCreateRegion();
		Region region=XCreateRegion();
		XUnionRegion(newregion,clipregion,region);
		XDestroyRegion(clipregion);
		clipregion=region;
	}

	if (clipregion) XSetRegion(GetDpy(),GetGC(),clipregion);
	else XSetClipMask(GetDpy(),GetGC(),None);

	clipmask=XCreatePixmap(GetDpy(),GetXDrawable(), Maxx-Minx+1,Maxy-Miny+1, 1);
	XSetForeground(dpy,GetGC(),0);
	//XCreateGC(Display *display, Drawable d, unsigned long valuemask, XGCValues *values);

	GC ggc=XCreateGC(GetDpy(),clipmask, 0,NULL);
	XFillRectangle(GetDpy(),clipmask,ggc, Minx,Miny, Maxx-Minx+1,Maxy-Miny+1);
	if (clipregion) XSetRegion(GetDpy(),ggc,clipregion);
	XSetForeground(dpy,ggc,~0);
	XFillRectangle(GetDpy(),clipmask,ggc, Minx,Miny, Maxx-Minx+1,Maxy-Miny+1);
	XFreeGC(GetDpy(),ggc);

	return 0;
}


/*! Copies the p array. If append!=0, then add the (closed) polyline to
 * the clip area. If append==0, then replace any current clip area with
 * the polygon.
 *
 * All the points p are assumed to be screen coordinates.
 *
 * Return 0 for success, nonzero for error.
 */
int DisplayerXlib::Clip(flatpoint *p,int n, int append)
{
	if (!append) ClearClip();
	if (!n) return 0;

	XPoint pts[n];
	for (int c=0; c<n; c++) {
		pts[c].x=(int)(p[c].x+.5);
		pts[c].y=(int)(p[c].y+.5);
	}
	Region newregion=XPolygonRegion(pts,n,WindingRule);
	Region oldregion=clipregion;
	if (oldregion) {
		clipregion=XCreateRegion();
		XUnionRegion(newregion,oldregion,clipregion);
		XDestroyRegion(oldregion);
		XDestroyRegion(newregion);
	} else clipregion=newregion;

	XSetRegion(GetDpy(),GetGC(),clipregion);

	clipmask=XCreatePixmap(GetDpy(),GetXDrawable(), Maxx-Minx+1,Maxy-Miny+1, 1);
	XSetForeground(dpy,GetGC(),0);
	//XCreateGC(Display *display, Drawable d, unsigned long valuemask, XGCValues *values);
	GC ggc=XCreateGC(GetDpy(),clipmask, 0,NULL);
	XFillRectangle(GetDpy(),clipmask,ggc, Minx,Miny, Maxx-Minx+1,Maxy-Miny+1);
	XSetRegion(GetDpy(),ggc,clipregion);
	XSetForeground(dpy,ggc,~0);
	XFillRectangle(GetDpy(),clipmask,ggc, Minx,Miny, Maxx-Minx+1,Maxy-Miny+1);
	XFreeGC(GetDpy(),ggc);

	return 0;
}

//! Clears clipregion, clipmask, and calls XSetClipMask(GetDpy(),GetGC(),None).
void DisplayerXlib::ClearClip()
{
	if (clipregion) { XDestroyRegion(clipregion); clipregion=NULL; }
	if (clipmask) { XFreePixmap(anXApp::app->dpy,clipmask); clipmask=0; }
	XSetClipMask(GetDpy(),GetGC(),None);
}

//! Convert real point p to screen coordinates.
/*! <pre>
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *  </pre>
 */
flatpoint DisplayerXlib::realtoscreen(flatpoint p)
{
	if (ctm[4]>1e+100 || ctm[5]>1e+100) {
		dumpctm(ctm);
		cerr <<" *****"<<endl;
		cerr <<" *****"<<endl;
		cerr <<" *****"<<endl;
		cerr <<" *****"<<endl;
		cerr <<" ***** BLAM ******"<<endl;
		cerr <<" ***** This is a mystery bug, very intermittent DisplayerXlib transform corruption ******"<<endl;
		cerr <<" ***** aborting program..."<<endl;
		cerr <<" *****"<<endl;
		cerr <<" *****"<<endl;
		cerr <<" *****"<<endl;
		cerr <<" *****"<<endl;

		transform_identity(ctm);
		transform_identity(ictm);
		//exit(1);
	}
	return flatpoint(ctm[4] + ctm[0]*p.x + ctm[2]*p.y, ctm[5]+ctm[1]*p.x+ctm[3]*p.y); 
} 

//! Convert real point (x,y) to screen coordinates.
/*! <pre>
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *  </pre>
 */
flatpoint DisplayerXlib::realtoscreen(double x,double y)
{
	return flatpoint(ctm[4] + ctm[0]*x + ctm[2]*y, ctm[5]+ctm[1]*x+ctm[3]*y); 
} 

//! Convert screen point (x,y) to real coordinates.
flatpoint DisplayerXlib::screentoreal(int x,int y)
{
	return flatpoint(ictm[4] + ictm[0]*(double)x + ictm[2]*(double)y, ictm[5]+ictm[1]*(double)x+ictm[3]*(double)y); 
}

//! Convert screen point p to real coordinates.
flatpoint DisplayerXlib::screentoreal(flatpoint p)
{
	return flatpoint(ictm[4] + ictm[0]*p.x + ictm[2]*p.y, ictm[5]+ictm[1]*p.x+ictm[3]*p.y); 
}

//! Clear the window to bgcolor between Min* and Max*. 
void DisplayerXlib::ClearWindow()
{
	//DBG cerr <<"--displayer ClearWindow:MinMaxx,:["<<Minx<<","<<Maxx
	//DBG		<<"] MinMaxy:["<<Miny<<","<<Maxy<<"] x0,y0:"<<ctm[4]<<","<<ctm[5]<<endl;

	//if (xw && dr->xlibDrawable(-1)==dr->xlibDrawable(1)) {
		 //if using double buffer, XClearWindow will crash your program, so clear manually
		if (xw) XSetForeground(dpy,gc, (xw->win_colors?xw->win_colors->bg:0));
		else XSetForeground(dpy,gc, bgcolor);
		XSetFunction(dpy,gc, GXcopy);
		XFillRectangle(dpy,w,gc, Minx,Miny,Maxx-Minx,Maxy-Miny);
		XSetForeground(dpy,gc,fgcolor);
	//} else XClearWindow(dpy, w);

	 //if using double buffer, XClearWindow will crash your program, so clear manually
	//XSetForeground(dpy,gc, bgcolor);
	//XSetFunction(dpy,gc, GXcopy);
	//XFillRectangle(dpy,w,gc, 0,0,Maxx,Maxy);
	//XSetForeground(dpy,gc,fgcolor);
}



//! Return the text height of the current font.
int DisplayerXlib::textheight()
{
	return xfont->height;
}

int DisplayerXlib::fontsize(double size)
{
	cerr <<"  *** need to implement DisplayerXlib::fontsize()!!! "<<endl;
	return 1;
}

/*! Return 0 for success, 1 for fail.
 */
int DisplayerXlib::font(LaxFont *nfont, double size)
{
	LaxFontXlib *xlibfont=dynamic_cast<LaxFontXlib*>(nfont);
	if (!xlibfont) return 1;

	if (xfont && !tempfont) XftFontClose(anXApp::app->dpy, xfont); //xfont was a temp Xft font
	nfont->inc_count();
	if (laxfont) laxfont->dec_count();
	laxfont=nfont;

	tempfont=xlibfont->font;
	xfont=xlibfont->font;
	return 0;
}

/*! Return 0 for success, 1 for fail.
 */
int DisplayerXlib::font(const char *fontconfigpattern)
{
	cerr <<"  *** need to implement DisplayerXlib::font()!!! "<<endl;
	return 1;
}

/*! Return 0 for success, 1 for fail.
 */
int DisplayerXlib::font(const char *family,const char *style,double size)
{
    if (size<=0) size=anXApp::app->defaultlaxfont->textheight();
	LaxFont *newfont=anXApp::app->fontmanager->MakeFont(family,style,size,0);
	if (!newfont) return 1;
	int status=font(newfont,size);
	newfont->dec_count();
	return status;
}

double DisplayerXlib::textextent(LaxFont *thisfont, const char *str,int len, double *width,double *height,double *ascent,double *descent,char real)
{
	XftFont *font=xfont;
	if (thisfont) {
		if (!dynamic_cast<LaxFontXlib*>(thisfont)) font=NULL;
		else font=dynamic_cast<LaxFontXlib*>(thisfont)->font;
	}

    if (str==NULL || !font) {
        if (width) *width=0;
        if (height) *height=0;
		if (ascent) *ascent=0;
		if (descent) *descent=0;
        return 0;
    }
	if (len<0) len=strlen(str);

    int fascent,fdescent;
    XGlyphInfo info;
    XftTextExtentsUtf8(anXApp::app->dpy, font, (XftChar8 *)(str), len, &info);
    fascent =font->ascent;
    fdescent=font->descent;

    double eex,eey;
    eex=info.xOff;
	eey=fascent+fdescent;

    if (width)   *width=eex;
    if (height)  *height=eey;
	if (ascent)  *ascent=fascent;
	if (descent) *descent=fdescent;

	return eex;
}

//! Return the application's default Xft text drawing context.
/*! Return a new one if none exists currently. Set to window if it does.
 *
 * If window==None, then destroy the existing textxftdraw. This is necessary to
 * do before the window is destroyed or else an X error pops up at strange times.
 */
XftDraw *DisplayerXlib::textdraw(Window xlib_window)
{
    if (!xlib_window && textxftdraw) {
        XftDrawDestroy(textxftdraw);
        textxftdraw=NULL;
    } else if (!textxftdraw) textxftdraw=XftDrawCreate(dpy, xlib_window, vis, DefaultColormap(dpy,0));
    else XftDrawChange(textxftdraw,xlib_window);
    return textxftdraw;
}

/*! Write out a single line.
 * Returns distance advanced. 
 */
double DisplayerXlib::textout_line(double x,double y,const char *str,int len,unsigned long align)
{
    if (len==0 || str==NULL) return 0;
    if (len<0) len=strlen(str);
    if (!xfont && dynamic_cast<LaxFontXlib*>(anXApp::app->defaultlaxfont)) { 
		if (xfont && !tempfont) XftFontClose(dpy, xfont);
		xfont=dynamic_cast<LaxFontXlib*>(anXApp::app->defaultlaxfont)->font;
		tempfont=xfont;
	}
	if (!xfont) return 0;

    //int dir=0; //dir is direction of text
    int ascent,descent;
    XGlyphInfo info;
    XftTextExtentsUtf8(dpy, xfont, (XftChar8 *)(str), len, &info);
    ascent= xfont->ascent;
    descent=xfont->descent;

    int ox,oy;
    if (align&LAX_RIGHT) ox=x-info.xOff;
    else if (align&LAX_LEFT) ox=x;
    else ox=x-info.xOff/2; //center

    if (align&LAX_TOP) oy=y+ascent;
    else if (align&LAX_BOTTOM) oy=y-descent;
    else if (align&LAX_BASELINE) oy=y;
    else oy=y-(ascent+descent)/2+ascent; //center

    //----------------
    int r,g,b;
    colorrgb(fgcolor,&r,&g,&b);
    XftColor color;
    color.pixel=fgcolor;
    color.color.alpha=65535;
    color.color.red  =r<<8;
    color.color.green=g<<8;
    color.color.blue =b<<8;
    //---------
    XftDrawStringUtf8(textdraw(w),
                      &color,
                      xfont,
                      ox,oy,
                      (XftChar8 *)str, len);
    textdraw(None);
    return info.xOff;
}

//! Draw multiple lines of text around screen x,y.
double DisplayerXlib::textout(double x,double y,const char *str,int len,unsigned long align)
{
	if (!w) return 0;
	XSetForeground(dpy,gc,fgcolor);

    int n=0;
    const char *nl=str;
    do {
        nl=strchr(nl,'\n');
        if (nl) nl++;
        n++;
    } while (nl);
    if (n==1) {
		flatpoint p;
		if (real_coordinates) {
			p=realtoscreen(x,y);
		} else {
			p.x=x; p.y=y;
		}
		return textout_line(p.x,p.y,str,len,align);
	}

    const char *text=str;
    int ret=0;
    int h=n*textheight();
	flatpoint p;

    int valign= align&(LAX_TOP|LAX_BOTTOM|LAX_VCENTER|LAX_BASELINE);
    align=align&(LAX_LEFT|LAX_HCENTER|LAX_RIGHT);
    if (valign==LAX_VCENTER) y-=h/2;
    else if (valign==LAX_BOTTOM) y-=h;

    do {
        nl=strchr(text,'\n');
        if (!nl) nl=text+strlen(text);

		if (real_coordinates) {
			p=realtoscreen(x,y);
		} else {
			p.x=x; p.y=y;
		}

        ret=textout_line(p.x,p.y, text,nl-text, align|LAX_TOP);
        if (*nl) {
            y+=textheight();
            text=nl+1;
            if (!*text) nl=text;
        }
    } while (*nl);
    return ret;
}

double DisplayerXlib::textout(double *matrix,double x,double y,const char *str,int len,unsigned long align)
{
	cerr << " *** implement DisplayerXlib::textout(double *matrix)!!"<<endl;

	if (!w) return 0;
	XSetForeground(dpy,gc,fgcolor);
	return textout(x,y, str,len, align);
}

double DisplayerXlib::textout(double angle, double x,double y,const char *str,int len,unsigned long align)
{
	cerr << " *** implement DisplayerXlib::textout(double angle)!!"<<endl;

	if (!w) return 0;
	XSetForeground(dpy,gc,fgcolor);
	return textout(x,y, str,len, align);
}


//! Set the width, whether solid, line cap and join.
/*! This currently uses Xlib's names which are as follows.
 * 
 * width is only in screen pixels for now.
 * See LaxJoinStyle and LaxCapStyle for values for cap and join.
 *
 * \todo should be able to set dash pattern somehow, and set line width based
 *   on current transform...
 */
void DisplayerXlib::LineAttributes(double width,int dash,int cap,int join)
{
	if (cap>0) {
		if (cap==LAXCAP_Butt) cap=CapButt;
		else if (cap==LAXCAP_Round) cap=CapRound;
		else if (cap==LAXCAP_Projecting) cap=CapProjecting;
		else cap=CapRound;
	}

	if (join>0) {
		if (join==LAXJOIN_Miter) join=JoinMiter;
		else if (join==LAXJOIN_Round) join=JoinRound;
		else if (join==LAXJOIN_Bevel) join=JoinBevel;
		else join=JoinRound;
	}

	XSetLineAttributes(GetDpy(),GetGC(), (int)width,dash,cap,join);
}

/*! fillrule can be EvenOddRule or WindingRule.
 * fillstyle can be FillSolid, FillTiled, FillStippled, or FillOpaqueStippled.
 */
void DisplayerXlib::FillAttributes(int fillstyle, int fillrule)
{
	XSetFillRule(GetDpy(),GetGC(), fillrule);
	XSetFillStyle(GetDpy(),GetGC(), fillstyle);
}

//! Set how to combine drawing elements to the target surface.
/*! Currently, there are clear correspondences with only a coulpe Xlib modes,
 * such as GXcopy, GXxor, and GXclear. All others map to GXcopy.
 *
 * I leave it as an exercise for the reader to properly map all these
 * to hopefully equivalent values:
 * </pre>
 *  GXclear        0 
 *  GXand          src AND dst 
 *  GXandReverse   src AND NOT dst 
 *  GXcopy         src 
 *  GXandInverted  NOT src AND dst 
 *  GXnoop         dst 
 *  GXxor          src XOR dst 
 *  GXor           src OR dst 
 *  GXnor          NOT src AND NOT dst 
 *  GXequiv        NOT src XOR dst 
 *  GXinvert       NOT dst 
 *  GXorReverse    src OR NOT dst 
 *  GXcopyInverted NOT src 
 *  GXorInverted   NOT src OR dst 
 *  GXnand         NOT src OR NOT dst 
 *  GXset          1
 * </pre>
 */
LaxCompositeOp DisplayerXlib::BlendMode(LaxCompositeOp mode)
{
	LaxCompositeOp old=drawmode;
	int xmode=GXcopy;
	if (mode==LAXOP_Clear) xmode=GXclear;
	else if (mode==LAXOP_Xor) xmode=GXxor;
	XSetFunction(dpy,gc,xmode);
	drawmode=old;
	return old;
}

double DisplayerXlib::setSourceAlpha(double alpha)
{
	cerr <<" *** need to implement DisplayerXlib::setSourceAlpha()!"<<endl;
	return 1;
}

//! Any subsequent calls are using real coordinates
/*! Returns old real_coordinates.
 */
int DisplayerXlib::DrawReal()
{
	int r=real_coordinates;
	real_coordinates=1;
	return r;
}

//! Any subsequent calls are using screen coordinates
/*! Returns old real_coordinates.
 */
int DisplayerXlib::DrawScreen()
{
	int r=real_coordinates;
	real_coordinates=0;
	return r;
}

void DisplayerXlib::DrawOnMask()
{
	cout <<" *** DisplayerXlib::DrawOnMask doesn't do anything!"<<endl;
}

void DisplayerXlib::DrawOnSrc()
{
	cout <<" *** DisplayerXlib::DrawOnSrc doesn't do anything!"<<endl;
}

void DisplayerXlib::moveto(flatpoint p)
{
	 //ignore single point paths...
	if (curpoints.n && (curpoints.e[curpoints.n-1].info&LINE_Start)) curpoints.e[curpoints.n-1]=p;
	else curpoints.push(p);
	curpoints.e[curpoints.n-1].info=LINE_Start|LINE_Vertex;

	if (curpoints.n>1 && !(curpoints.e[curpoints.n-2].info&LINE_End)) {
		 //previous point had no end marker, so must tag previous path
		curpoints.e[curpoints.n-2].info|=LINE_End|LINE_Open;
		int c;
		for (c=curpoints.n-2; !(curpoints.e[c].info&LINE_Start); c--) ;
		curpoints.e[c].info|=LINE_Open;
	}
	needtobuildxpoints=1;
}

/*! If there was no previous point, then this is the same as a moveto(p).
 */
void DisplayerXlib::lineto(flatpoint p)
{
	if (!curpoints.n || (curpoints.e[curpoints.n-1].info&(LINE_End))) {
		 //previous point was the end of a path, so we need to start a new one
		moveto(p);
		return;
	}

	curpoints.push(p);
	curpoints.e[curpoints.n-1].info=LINE_Vertex;
	needtobuildxpoints=1;
}

void DisplayerXlib::curveto(flatpoint c1,flatpoint c2,flatpoint v)
{
	if (!curpoints.n || (curpoints.e[curpoints.n-1].info&LINE_End)) moveto(c1);

	curpoints.push(c1);
	curpoints.e[curpoints.n-1].info=LINE_Bez;
	curpoints.push(c2);
	curpoints.e[curpoints.n-1].info=LINE_Bez;
	curpoints.push(v);
	curpoints.e[curpoints.n-1].info=LINE_Vertex;
	needtobuildxpoints=1;
}

//! Call if current path should be closed, and close at the last added point.
void DisplayerXlib::closed()
{
	if (!curpoints.n) return;
	curpoints.e[curpoints.n-1].info|=LINE_Closed|LINE_End;
	if (curpoints.e[curpoints.n-1].info&LINE_Start) curpoints.pop();
	else {
		int c;
		for (c=curpoints.n-1; !(curpoints.e[c].info&LINE_Start); c--) ;
		curpoints.e[c].info|=LINE_Closed;
	}
	needtobuildxpoints=1;
}

//! Call if current path should be ended, but not closed.
void DisplayerXlib::closeopen()
{
	if (!curpoints.n) return;
	curpoints.e[curpoints.n-1].info|=LINE_Open|LINE_End;
	if (curpoints.e[curpoints.n-1].info&LINE_Start) curpoints.pop();
	else {
		int c;
		for (c=curpoints.n-1; !(curpoints.e[c].info&LINE_Start); c--) ;
		curpoints.e[c].info|=LINE_Open;
	}
	needtobuildxpoints=1;
}

//! Rebuild xpoints to correspond to the current path, if any.
/*! \todo shouldn't ignore multiple paths..
 */
void DisplayerXlib::buildXPoints()
{
	if (!needtobuildxpoints) return;
	if (xpoints) { numxpoints=0; } //keep allocated array

	if (!curpoints.n) return;
	int numneeded=0;
	if (!(curpoints.e[curpoints.n-1].info&LINE_End)) closeopen();
	needtobuildxpoints=0;


	//starting endpoints have LINE_Start.
	//interior line points have LINE_Bez or LINE_Vertex
	//ending points will typically have LINE_Vertex, maybe or'd with LINE_Closed

	multiplepaths.flush();//this will hold the start in xpoints of each open path
	multipleclosedpaths.flush();//this will hold the start in xpoints of each closed path

	 //first find the number of x points needed
	for (int c=0; c<curpoints.n; c++) {
		numneeded++;

		if (curpoints.e[c].info&LINE_Start) {
			//we have encountered the start of a new line, and previous line was open
			numneeded+=3; //1 for closing off, 1 for anchor
		}
		if (curpoints.e[c].info&LINE_Vertex) continue; //is vertex
		numneeded+=num_bez_div+1; //else is 2 bezier control points
		c+=2; //skip control points
	}

	//DBG cerr <<"build path from "<<curpoints.n<<endl;
	//DBG if (curpoints.n==4) {
	//DBG 	cerr <<"blah"<<endl;
	//DBG }
	numxpoints=0;

	if (numneeded>maxxpoints_allocated) {
		if (xpoints) { delete[] xpoints; xpoints=NULL; }
		xpoints=new XPoint[numneeded];
		maxxpoints_allocated=numneeded;
	}
	numxpoints=0;

	int i=0;
	int anchor=-1;
	flatpoint p;
	flatpoint b[num_bez_div];

	 //first pass, closed paths, attach all together
	for (int c=0; c<curpoints.n; c++) {
		if (curpoints.e[c].info&LINE_Start) {
			if (curpoints.e[c].info&LINE_Open) {
				 //skip open lines
				while (c<curpoints.n && !(curpoints.e[c].info&LINE_End)) c++;
				continue;						
			}
			multipleclosedpaths.push(i);
		}

		if (curpoints.e[c].info&LINE_Vertex) {
			 //is vertex
			p= real_coordinates ? realtoscreen(curpoints.e[c]) : curpoints.e[c];
			xpoints[i].x=(int) p.x;
			xpoints[i].y=(int) p.y;
			i++;
			DBG if (i>maxxpoints_allocated) DANGER();

		} else {
			bez_points(b, curpoints.e+(c-1), num_bez_div, 1);
			for (int c2=1; c2<num_bez_div; c2++) { //skipping 1st point
				p= real_coordinates ? realtoscreen(b[c2]) : b[c2];
				xpoints[i].x=(int) p.x;
				xpoints[i].y=(int) p.y;
				i++;
				DBG if (i>maxxpoints_allocated) DANGER();
			}
			c+=2;
		}

		if (curpoints.e[c].info&LINE_End) {
			xpoints[i++]=xpoints[multipleclosedpaths.e[multipleclosedpaths.n-1]]; //close off path in xlib way. ug!
			multipleclosedpaths.push(i);

			if (anchor<0) anchor=i-1;
			else {
				xpoints[i]=xpoints[anchor];
				i++;
				DBG if (i>maxxpoints_allocated) DANGER();
			}
		}
	}

	 //second pass, open paths
	for (int c=0; c<curpoints.n; c++) {
		if (curpoints.e[c].info&LINE_Start) {
			if (curpoints.e[c].info&LINE_Closed) {
				 //skip closed lines
				while (c<curpoints.n && !(curpoints.e[c].info&LINE_End)) c++;
				continue;
			}
			multiplepaths.push(i);
		}

		if (curpoints.e[c].info&LINE_Vertex) {
			 //is vertex
			p= real_coordinates ? realtoscreen(curpoints.e[c]) : curpoints.e[c];
			xpoints[i].x=(int) p.x;
			xpoints[i].y=(int) p.y;
			i++;
				DBG if (i>maxxpoints_allocated) DANGER();

		} else {
			bez_points(b, curpoints.e+(c-1), num_bez_div, 1);
			for (int c2=1; c2<num_bez_div; c2++) { //skipping 1st point
				p= real_coordinates ? realtoscreen(b[c2]) : b[c2];
				xpoints[i].x=(int) p.x;
				xpoints[i].y=(int) p.y;
				i++;
				DBG if (i>maxxpoints_allocated) DANGER();
			}
			c+=2;
		}
	}


	numxpoints=i;
}

void DisplayerXlib::fill(int preserve)
{
	if (!curpoints.n) return;

	buildXPoints();
	if (!numxpoints) return;

	int start=0,end;

	if (!multipleclosedpaths.n) return; //no closed paths to fill!

	if (multiplepaths.n) end=multiplepaths.e[0];
	else end=numxpoints;

	XFillPolygon(dpy,w,gc, xpoints+start,end-start, Complex,CoordModeOrigin);

	if (!preserve) {
		curpoints.flush();
		multiplepaths.flush();
		multipleclosedpaths.flush();
	}
}

void DisplayerXlib::stroke(int preserve)
{
	if (!curpoints.n) return;
	
	buildXPoints();
	if (!numxpoints) return;

	int start=0,end;

	 //closed paths
	for (int c=0; c<multipleclosedpaths.n; c+=2) {
		start=multipleclosedpaths.e[c];
		end=multipleclosedpaths.e[c+1];
		XDrawLines(dpy,w,gc, xpoints+start,end-start, CoordModeOrigin);
	}

	 //open paths
	for (int c=0; c<multiplepaths.n; c++) {
		start=multiplepaths.e[c];
		if (c!=multiplepaths.n-1) end=multiplepaths.e[c+1];
		else end=numxpoints;

		XDrawLines(dpy,w,gc, xpoints+start,end-start, CoordModeOrigin);
	}

	if (!preserve) {
		curpoints.flush();
		multiplepaths.flush();
		multipleclosedpaths.flush();
	}
}

//! Draw a polygon, optionally closed, optionally fill, optionally transform.
/*! If fill==1 then fill with FG and have no border. If fill==2,
 * then fill with BG and border with FG.
 */
void DisplayerXlib::drawlines(flatpoint *points,int npoints,char closed,char fill)
{
	XPoint xpoints[npoints+(closed?1:0)];
	int c;
	flatpoint p;
	for (c=0; c<npoints; c++) {
		if (real_coordinates) {
			p=realtoscreen(points[c]);
			xpoints[c].x=(int)p.x;
			xpoints[c].y=(int)p.y;
		} else {
			xpoints[c].x=(int)points[c].x;
			xpoints[c].y=(int)points[c].y;
		}
	}
	if (closed) xpoints[c]=xpoints[0];

	if (fill) {
		if (fill==2) XSetForeground(dpy,gc,bgcolor); 
		XFillPolygon(dpy,w,gc,xpoints,npoints+(closed?1:0),Complex,CoordModeOrigin);
		if (fill==2) XSetForeground(dpy,gc,fgcolor); 
	}
	if (fill!=1) XDrawLines(dpy,w,gc,xpoints,npoints+(closed?1:0),CoordModeOrigin);	
}

//! Draw a line between real coordinates p1 and p2.
void DisplayerXlib::drawline(flatpoint p1,flatpoint p2)
{
	//DBG cerr <<" **************Displayer::drawline before "<<p1.x<<','<<p1.y<<" to "<<p2.x<<','<<p2.y<<endl;
	if (real_coordinates) {
		p1=realtoscreen(p1); 
		p2=realtoscreen(p2);
	}
	//DBG cerr <<" **************Displayer::drawline after "<<p1.x<<','<<p1.y<<" to "<<p2.x<<','<<p2.y<<endl;
	XDrawLine(dpy,w,gc, (int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y);
}

//! Draw a line between screen coordinates (ax,ay) and (bx,by).
void DisplayerXlib::drawline(double ax,double ay,double bx,double by)
{  drawline(flatpoint(ax,ay),flatpoint(bx,by)); }

//! Draw one pixel at coord p.
void DisplayerXlib::drawpixel(flatpoint p)
{
	if (real_coordinates) p=realtoscreen(p);
	XDrawPoint(dpy,w,gc,  (int)p.x, (int)p.y);
}

//! Draw a little circle at screen coord (x,y) with radius r.
void DisplayerXlib::drawpoint(double x, double y, double radius, int fill)
{
	if (real_coordinates) {
		flatpoint fp=realtoscreen(flatpoint(x,y));
		x=fp.x;
		y=fp.y;
	}
	if (fill) {
		if (fill==2) XSetForeground(dpy,gc,bgcolor); 
		XFillArc(dpy,w,gc,  (int)x-radius, (int)y-radius,2*radius,2*radius, 0, 23040);
		if (fill==2) {
			XSetForeground(dpy,gc,fgcolor); 
			XDrawArc(dpy,w,gc,  (int)x-radius, (int)y-radius,2*radius,2*radius, 0, 23040);
		}
	} else XDrawArc(dpy,w,gc,  (int)x-radius, (int)y-radius,2*radius,2*radius, 0, 23040);
}

//! Output an image into a real space rectangle.
/*! This will obey any clipping in place.
 *
 * (x,y) is an additional translation to use. If w>0 OR h>0, then fit the image into
 * a rectangle with those real width and height. If w<=0 AND h<=0, then use the
 * image's pixel width and height as bounds.
 *
 * Return 0 for image drawn with no complications.
 *
 * When there is an error and nothing is drawn, a negative value is returned.
 * Return -1 for image==NULL.
 * Return -2 for unknown image type.
 * Return -3 for image not available.
 *
 * When drawing effectively succeeds, but there are extenuating circumstances, a positive value is returned.
 * Return 1 for image not in viewport.
 * Return 2 for image would be drawn smaller than a pixel, so it's not actually drawn.
 *
 * \todo should be able to do partial drawing when an image is huge, and only a tiny
 *   portion of it actually should be rendered.
 */
int DisplayerXlib::imageout(LaxImage *image, double x,double y, double w,double h)
{
	if (image==NULL) return -1;
	LaxImlibImage *imlibimage=dynamic_cast<LaxImlibImage *>(image);
	if (!imlibimage) return -2;
	if (!imlibimage->Image()) return -3;

	if (w<=0 && h<=0) { w=image->w(); h=image->h(); }

	flatpoint ul=flatpoint(x  ,y  ),
			  ur=flatpoint(x+w,y  ),
			  ll=flatpoint(x  ,y+h),
			  lr=flatpoint(x+w,y+h);
	if (real_coordinates) {
		ul=realtoscreen(ul), 
		ur=realtoscreen(ur), 
		ll=realtoscreen(ll), 
		lr=realtoscreen(lr);
	} else {
	}

	DoubleBBox bbox(ul);
	bbox.addtobounds(ur);
	bbox.addtobounds(ll);
	bbox.addtobounds(lr);
	if (!bbox.intersect(Minx,Maxx,Miny,Maxy)) {
		DBG cerr <<"----------------ImageData outside viewport"<<endl;
		return 1;
	}
	if ((int)bbox.maxx<=(int)bbox.minx || (int)bbox.maxy<=(int)bbox.miny) {
		 //image would render smaller than a pixel, so skip it
		DBG cerr <<" ImageData too small: w,h="<<bbox.maxx-bbox.minx<<" x "<<bbox.maxy-bbox.miny<<endl;
		return 2;
	}

	imlib_context_set_drawable(GetXDrawable());
	
	 // ***note that this is a terrible hack:
	 // imlib has unfortunate defficiencies for clipping, ie it seems to use its own
	 // graphics context, so you cannot use the normal clipping area from an Xlib GC,
	 // hence the following ugly workaround, which assumes that the clip area for 
	 // dp is accessible in dp->clipmask:
	 //  strategy is to get a copy of the destination area of the drawable with the 
	 //  appropriate clip mask defined on it, then render the image onto that,
	 //  then copy that back to the drawable, where its existing clip mask blocks
	 //  what it has to.
	 //  ******Please note that this is ridiculously slow.
	Imlib_Image tempimage=0;
	
	int imagew,imageh;
	imlib_context_set_image(imlibimage->Image());
	imagew=imlib_image_get_width();
	imageh=imlib_image_get_height();
		
	if (activeMask()) {
		DBG cerr <<"DisplayerXlib thinks it should clip"<<endl;

		 // create image with the destination screen area, containing this->clipmask
		imlib_context_set_drawable(GetXDrawable());
		tempimage=imlib_create_image_from_drawable(clipmask,
						(int)bbox.minx,(int)bbox.miny,
						(int)bbox.maxx-(int)bbox.minx,(int)bbox.maxy-(int)bbox.miny,
						1); 
		if (!tempimage) {
			DBG cerr <<"WARING!! null image in DisplayerXlib::imageOut() for clipping"<<endl;
		} else {
			imlib_context_set_image(tempimage);
			DBG cerr <<"image "<<(imlib_image_has_alpha()?"doesnt have":"has")<<" alpha"<<endl;
			
			 //lay the actual image onto the destination area image
			imlib_blend_image_onto_image_skewed(imlibimage->Image(),
					0, //char merge_alpha=0 means keep the destination alpha channel
					//0,0, (int)data->maxx,(int)data->maxy,         //src x,y,w,h
					0,0, imagew,imageh,     //src x,y,w,h
					(int)(ll.x-bbox.minx),(int)(ll.y-bbox.miny), //dest x,y
					(int)(lr.x-ll.x),(int)(lr.y-ll.y),   //offset for upper right corner from destx,y
					(int)(ul.x-ll.x),(int)(ul.y-ll.y)   //offset for lower left corner from destx,y
				);
			 //create X pixmaps for the resulting image data, and resulting alpha channel
			Pixmap pixmap,nclipmask;
			imlib_render_pixmaps_for_whole_image(&pixmap, &nclipmask);

			 //finally, copy that back to the screen
			XCopyArea(GetDpy(),pixmap,GetXDrawable(),GetGC(),
					0,0,
					(int)bbox.maxx-(int)bbox.minx,(int)bbox.maxy-(int)bbox.miny,
					(int)bbox.minx,(int)bbox.miny);

			 //cleanup
			imlib_free_pixmap_and_mask(pixmap);
			imlib_free_image();
		}

	} else { // no clipping
		imlib_context_set_image(imlibimage->Image());
		imlib_render_image_on_drawable_skewed(
				//0,0, (int)data->maxx,(int)data->maxy, //src x,y,w,h
				0,0, imagew,imageh, //src x,y,w,h
				(int)ll.x,(int)ll.y,                     //dest x,y
				(int)(lr.x-ll.x),(int)(lr.y-ll.y),   //offset for upper right corner from destx,y
				(int)(ul.x-ll.x),(int)(ul.y-ll.y)   //offset for lower left corner from destx,y
			);
	}

	return 0;
}

//***
void DisplayerXlib::imageout(LaxImage *image, double x,double y)
{ imageout(image,x,y,0,0); }

//***
void DisplayerXlib::imageout(LaxImage *img,double *matrix)
{ imageout(img, matrix[4],matrix[5],0,0); }

//***
void DisplayerXlib::imageout(LaxImage *img, double angle, double x,double y)
{ imageout(img,x,y,0,0); }

//***
void DisplayerXlib::imageout_rotated(LaxImage *img,double x,double y,double ulx,double uly)
{ imageout(img,x,y,0,0); }

//***
void DisplayerXlib::imageout_skewed(LaxImage *img,double ulx,double uly,double urx,double ury,double llx,double lly)
{ imageout(img,ulx,uly,0,0); }



//! Move the viewable portion by dx,dy screen units.
void DisplayerXlib::ShiftScreen(int dx,int dy)
{
	ictm[4]-=dx;
	ictm[5]-=dy;
	ctm[4]+=dx;
	ctm[5]+=dy;
	findictm();
	syncPanner();
}

//! Move the screen by real dx,dy along the real axes.
void DisplayerXlib::ShiftReal(double dx,double dy)
{
	ctm[4]+=dx*ctm[0]+dy*ctm[2];
	ctm[5]+=dx*ctm[1]+dy*ctm[3];
	findictm();
	syncPanner();
}

//! Make the transform correspond to the values.
void DisplayerXlib::NewTransform(double a,double b,double c,double d,double x0,double y0)
{
	ctm[0]=a;
	ctm[1]=b;
	ctm[2]=c;
	ctm[3]=d;
	ctm[4]=x0;
	ctm[5]=y0;
	findictm();
	syncPanner();
}

//! Push axes, and multiply ctm by a new transform.
void DisplayerXlib::PushAndNewTransform(const double *m)
{
	PushAxes();
	//DBG cerr <<"before: ";
	//dumpctm(ctm);
	double *tctm=transform_mult(NULL,m,ctm);
	delete[] ctm;
	ctm=tctm;
	findictm();
	//DBG cerr <<"after:  ";
	//dumpctm(ctm);
}

//! Push axes, and multiply ctm by a new basis, p,x,y are all in real units.
void DisplayerXlib::PushAndNewAxes(flatpoint p,flatpoint x,flatpoint y)
{
	PushAxes();
	NewAxis(p,x,y);
}

//! Push the current axes on the axessstack.
void DisplayerXlib::PushAxes()
{
	axesstack.push(ctm,2);
	ctm=new double[6];
	for (int c=0; c<6; c++) ctm[c]=axesstack.e[axesstack.n-1][c];
	findictm();
}

//! Recover the last pushed axes.
void DisplayerXlib::PopAxes()
{
	if (axesstack.n==0) return;
	delete[] ctm;
	ctm=axesstack.pop();
	findictm();
}

//! Set the ctm to these 6 numbers.
void DisplayerXlib::NewTransform(const double *d)
{
	for (int c=0; c<6; c++) ctm[c]=d[c];
	findictm();
	syncPanner();
}

//! Rotate by angle, about screen coordinate (x,y).
/*! dec nonzero means angle is degrees, otherwise radians.
 */
void DisplayerXlib::Rotate(double angle,int x,int y,int dec) // dec=1
{
	flatpoint p=screentoreal(x,y);
	Newangle(angle,1,dec);
	p=realtoscreen(p);
	ctm[4]+=x-p.x;
	ctm[5]+=y-p.y;
	findictm();
	syncPanner(1);
}

//! Rotate around real origin so that the x axis has angle between it and the screen horizontal.
/*! TODO: this could certainly be optimized..
 */
void DisplayerXlib::Newangle(double angle,int dir,int dec) // dir=0,dec=1
{
	if (dec) angle*=M_PI/180;
	flatpoint rx(ctm[0],ctm[1]),ry(ctm[2],ctm[3]);
	if (dir==0) { /* absolute slant */
      	double angle2=atan2(ctm[1],ctm[0]); // current angle of rx to screen x
		rx=rotate(rx,-angle2+angle);
		ry=rotate(ry,-angle2+angle);
	} else if (dir<0) { /* rotate CW */
		rx=rotate(rx,-angle);
		ry=rotate(ry,-angle);
	} else { /* rotate CCW */
		rx=rotate(rx,angle);
		ry=rotate(ry,angle);
	}
	ctm[0]=rx.x;
	ctm[1]=rx.y;
	ctm[2]=ry.x;
	ctm[3]=ry.y;
	findictm();
	syncPanner(1);
}
//  [xs] = [ctm[4]] + [X*Xr X*Yr][xr]
//  [ys]   [ctm[5]]   [Y*Xr Y*Yr][yr]

//! Zoom around real point p.
void DisplayerXlib::Zoomr(double m,flatpoint p)
{
	flatpoint po=realtoscreen(p);
	Zoom(m);
	po=realtoscreen(p)-po;
	ctm[4]+=po.x;
	ctm[5]+=po.y;
	findictm();
	syncPanner();
}

//! Zoom around screen point x,y.
void DisplayerXlib::Zoom(double m,int x,int y)
{
	flatpoint po=screentoreal(x,y);
	char udp=updatepanner;
	updatepanner=0;
	DBG cerr <<"\nZoom:"<<m<<"   around real="<<po.x<<","<<po.y<<"  =s:"<<x<<','<<y<<"  ctm4,5="<<ctm[4]<<','<<ctm[5]<<endl;
	Zoom(m);
	po=realtoscreen(po);
	DBG cerr <<"   shift:"<<po.x-x<<","<<po.y-y<<endl;
	ctm[4]+=x-po.x;
	ctm[5]+=y-po.y;
	findictm();
	
	DBG po=screentoreal(x,y);
	DBG cerr <<"   new realfromscreen:"<<po.x<<","<<po.y<<"  ctm4,5="<<ctm[4]<<','<<ctm[5]<<endl;
	updatepanner=udp;
	syncPanner();
}

//! Zoom around the real origin.
void DisplayerXlib::Zoom(double m) // zooms with origin constant point
{
	//DBG cerr <<"DisplayerXlib zoom: "<<m<<" rx:"<<(ctm[0]*ctm[0]+ctm[1]*ctm[1])<<"  ry:"<<ctm[2]*ctm[2]+ctm[3]*ctm[3]<<endl;
	if (m<=0) return;

	if (m<1 && (m*sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1])<lowerbound || 
				m*sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3])<lowerbound)) return;
	if (m>1 && (m*sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1])>upperbound || 
				m*sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3])>upperbound)) return;
	//DBG cerr <<"  adjusting mag..."<<endl;
	ctm[0]*=m;
	ctm[1]*=m;
	ctm[2]*=m;
	ctm[3]*=m;
	findictm();
	syncPanner();
}

//! Return the magnification along the screen vector (x,y). (screen=mag*real)
/*! This is useful when the axes are of different lengths or are not
 * orthogonal, or are rotated.
 */
double DisplayerXlib::GetVMag(int x,int y)
{
	flatpoint v=screentoreal(x,y),v2=screentoreal(0,0);
	return sqrt((x*x+y*y)/((v-v2)*(v-v2)));
}

//! Return the current magnification, screen=Getmag*real
/*! If c!=0 then return the y scale, default is to return the x scale.
 *
 * Note that this only returns basically length of axis in screen units divided
 * by length of axis in real coords. If the axes are rotated, they do not
 * correspond necessarily to the screen horizontal or vertical magnification.
 * For that, call GetVMag.
 */
double DisplayerXlib::Getmag(int c) // c=0
{ 
	if (c) return sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]);
	return sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]);
}

//! Set the inverse of the current transformation matrix, assuming ctm is already set.
void DisplayerXlib::findictm()
{
	transform_invert(ictm,ctm);
}

//! Set new foreground color. Typically usage is NewFG(app->rgbcolor(.5,.8,0,1.0)).
/*! Component range is [0..1.0].
 */
unsigned long DisplayerXlib::NewFG(double r,double g,double b,double a)
{
	unsigned long old=fgcolor;
	fgcolor=rgbcolor((int)(r*255),(int)(g*255),(int)(b*255));
	if (gc) XSetForeground(dpy,gc,fgcolor); 
	return old;
}

//! Set new foreground. Typically usage is NewFG(app->rgbcolor(23,34,234)).
/*! Component range is currently 0..255.... subject to change.
 */
unsigned long DisplayerXlib::NewFG(int r,int g,int b,int a)
{
	unsigned long old=fgcolor;
	fgcolor=rgbcolor(r,g,b);
	if (gc) XSetForeground(dpy,gc,fgcolor); 
	return old;
}

//! Set new foreground. Color components are 0..0xffff.
unsigned long DisplayerXlib::NewFG(ScreenColor *col)
{
	unsigned long old=fgcolor;
	fgcolor=rgbcolor(col->red>>8,col->green>>8,col->blue>>8);
	if (gc) XSetForeground(dpy,gc,fgcolor); 
	return old;
}

//! Set new foreground. Typically usage is NewFG(app->rgbcolor(23,34,234)).
unsigned long DisplayerXlib::NewFG(unsigned long ncol)
{
	unsigned long old=fgcolor;
	fgcolor=ncol;
	if (gc) XSetForeground(dpy,gc,ncol); 
	return old;
}

//! Set new foreground. Color components are 0..0xffff.
unsigned long DisplayerXlib::NewBG(ScreenColor *col)
{
	unsigned long old=bgcolor;
	bgcolor=rgbcolor(col->red>>8,col->green>>8,col->blue>>8);
	if (gc) XSetBackground(dpy,gc,bgcolor); 
	return old;
}

//! Set new background color. Typically usage is NewFG(app->rgbcolor(.5,.8,0)).
/*! Component range is [0..1.0].
 */
unsigned long DisplayerXlib::NewBG(double r,double g,double b)
{
	unsigned long old=bgcolor;
	bgcolor=rgbcolor((int)(r*255),(int)(g*255),(int)(b*255));
	if (gc) XSetBackground(dpy,gc,bgcolor); 
	return old;
}

//! Set new background. Typically usage is NewBG(app->rgbcolor(23,34,234)).
unsigned long DisplayerXlib::NewBG(int r,int g,int b)
{
	unsigned long old=bgcolor;
	bgcolor=rgbcolor(r,g,b);
	if (gc) XSetBackground(dpy,gc,bgcolor); 
	return old;
}
	
//! Set new background. Typically usage is NewBG(app->rgbcolor(23,34,234)).
unsigned long DisplayerXlib::NewBG(unsigned long nc)
{
	unsigned long old=bgcolor;
	bgcolor=nc;
	if (gc) XSetBackground(dpy,gc,nc); 
	return old;
}

//! Set the xscale and the yscale.
/*! If ys<=0, then the y scale is set the same as the xscale.
 *
 * \todo this could certainly be optimized.
 */
void DisplayerXlib::Newmag(double xs,double ys)//ys=-1
{
	if (xs<=0) return;
	flatpoint rx(ctm[0],ctm[1]),ry(ctm[2],ctm[3]);
	rx=(xs/sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]))*rx;
	if (ys<=0) ys=xs;
	ry=(ys/sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]))*ry;
	ctm[0]=rx.x;
	ctm[1]=rx.y;
	ctm[2]=ry.x;
	ctm[3]=ry.y;
	DBG cerr <<"=====Newmag()="<<xs<<" x "<<ys<<endl;
	findictm();
	syncPanner();
}

//! Set up to be drawing on a buffer.
/*! An important trait of starting drawing with this is that Updates(0) is always in effect until 
 * EndDrawing is called. ***Calling this start is when you want to draw a bunch of stuff
 * on any old pixmap, and not fuss with the panner things.
 *
 * Sets Minx=Miny=0, Maxx=width, Maxy=height of drawable.
 * 
 * If xw==NULL when EndDrawing() is called, then Updates(1) is called. Please remember, however,
 * that the Minx,Maxx,... are screwed up by then, and you must sync those yourself.
 */
int DisplayerXlib::StartDrawing(aDrawable *buffer)//buffer=0
{
	DBG cerr<<"----DisplayerXlib Start Drawing with drawable"<<endl;
	dpy=anXApp::app->dpy;
	vis=anXApp::app->vis;
	gc=anXApp::app->gc(); //*** should allow different gc's?
	w=buffer->xlibDrawable();
	xw=dynamic_cast<anXWindow*>(buffer);
	dr=buffer;

	XSetForeground(dpy,gc,fgcolor);
	XSetBackground(dpy,gc,bgcolor);
	//*** set other default graphics state?
	//if (Minx>=Maxx || Miny>=Maxy) {
		Window rootwin;
		int x,y;
		unsigned int width,height,bwidth,depth;
		XGetGeometry(dpy,w,&rootwin,&x,&y,&width,&height,&bwidth,&depth);
		Minx=Miny=0;
		Maxx=width;
		Maxy=height;
	//}
	on=1;
	Updates(0);
	return 0;
}

int DisplayerXlib::MakeCurrent(aDrawable *buffer)
{
	dr=buffer;
	xw=dynamic_cast<anXWindow*>(buffer);
	w=buffer->xlibDrawable();

	Window rootwin;
	int x,y;
	unsigned int width,height,bwidth,depth;
	XGetGeometry(dpy,w,&rootwin,&x,&y,&width,&height,&bwidth,&depth);
	Minx=Miny=0;
	Maxx=width;
	Maxy=height;

	return 1;
}

//! Free any data associated with drawable.
/*! Return 1 for drawable not currently used, or 0 for cleared.
 */
int DisplayerXlib::ClearDrawable(aDrawable *drawable)
{
	if (!textxftdraw || drawable!=dr) return 1;
	if (anXApp::app->findsubwindow_xlib(dynamic_cast<anXWindow*>(drawable),XftDrawDrawable(textxftdraw))) {
		XftDrawDestroy(textxftdraw);
		textxftdraw=NULL;
		return 0;
	}
	return 1;
}


//! Return a new LaxImage that is copied from the current buffer.
/*! Returned image has count of 1.
 */
LaxImage *DisplayerXlib::GetSurface()
{
	if (!w) return NULL;

	imlib_context_set_drawable(w);
	Imlib_Image tnail=imlib_create_image_from_drawable(0,0,0,Maxx-Minx,Maxy-Miny,1);
	LaxImlibImage *img=new LaxImlibImage(NULL,tnail);
	return img;
}

//! Create a fresh surface to perform drawing operations on.
/*! Return 0 for successful creation.
 */
int DisplayerXlib::CreateSurface(int width,int height, int type)
{
	if (w && isinternal) {
		XFreePixmap(anXApp::app->dpy,w);
	}
	w=XCreatePixmap(anXApp::app->dpy,DefaultRootWindow(anXApp::app->dpy),
								width,height,XDefaultDepth(anXApp::app->dpy,0));
	if (!dr) dr=new aDrawable(w);
	else dr->xlib_window=w;
	xw=NULL;
	isinternal=1;
	Minx=Miny=0;
	Maxx=width;
	Maxy=height;

	gc=anXApp::app->gc();

	return 0;
}

//! Resize an internal drawing surface.
/*! If you had previously called CreateSurface(), this will resize that
 * surface. If the target surface is an external surface, then nothing is done.
 *
 * Return 0 for success, or 1 if not using an internal surface, and nothing done.
 */
int DisplayerXlib::ResizeSurface(int width, int height)
{
	if (!isinternal) return 1;
	if (width!=Maxx || height!=Maxy) return CreateSurface(width,height);
	return 0;
}


//! Sets gc,w,on all to 0.
/*! This resets all the drawing bits to 0 to prevent calls made from non-window elements
 * trying to use windows that might have been destroyed. Unfortunately, such error checking for
 * drawing bits set to 0 is hardly implemented in DisplayerXlib drawing functions.. DisplayerXlib needs
 * more work...
 *
 * If xw==NULL, then call Updates(1);
 */
int DisplayerXlib::EndDrawing()
{
	if (xw==NULL) Updates(1);
	gc=0;
	if (!isinternal) w=0;
	on=0;
	return 0;
}

} // namespace Laxkit

