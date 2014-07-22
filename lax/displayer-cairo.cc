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


#include <lax/displayer-cairo.h>
#include <lax/fontmanager-cairo.h>
#include <lax/laximages-cairo.h>



#ifdef LAX_USES_CAIRO



#include <lax/lists.cc>
#include <lax/laximages-cairo.h>
#include <lax/laxutils.h>
#include <lax/doublebbox.h>
#include <lax/transformmath.h>

#include <cstring>

#include <iostream>
#define DBG 

using namespace std;


namespace Laxkit {

//-------------------------------- DisplayerCairo --------------------------
/*! \class DisplayerCairo
 * \brief Displayer based on Cairo.
 * <pre>
 * About the Current Transformation matrix (ctm) and its inverse (ictm):
 *
 *  Postscript:
 *      [ a  b  0 ]
 *  CTM=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * 
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *
 *  Say you want to rotate real, then translate real, then screenp = realp*R*T*CTM.
 *  
 *  In the Cairo library, cairo_matrix_t={ double xx,xy, yx,yy, x0,y0 } corresponds
 *  to a Laxkit transform [xx yx xy yy x0 y0].
 * </pre>
 */
/*! \var cairo_t *DisplayerCairo::cr
 * \brief Cairo context that the displayer uses.
 */
/*! \var cairo_surface_t *DisplayerCairo::surface
 * \brief Cairo destination surface.
 */
/*! \var cairo_surface_t *DisplayerCairo::mask
 * \brief Cairo mask surface, if any.
 */
/*! \var cairo_surface_t *DisplayerCairo::source
 * \brief Cairo source surface, if any.
 */





/*! set cr=NULL, and surface=NULL.
 */
DisplayerCairo::DisplayerCairo(anXWindow *nxw,PanController *pan) 
	: Displayer(nxw,pan)
{
	base_init();
}

void DisplayerCairo::base_init()
{
	buffer=NULL;
	bufferlen=0;

	isinternal=0;

	if (xw) {
		dpy=xw->app->dpy;
		vis=xw->app->vis;
		w=xw->xlib_window;
	} else {
		dpy=anXApp::app->dpy;
		vis=anXApp::app->vis;
		w=0;
	}

	blendmode=LAXOP_Source;
	on=0;

	cr=NULL;
	curfont=NULL;
	curscaledfont=NULL;
	_textheight=_ascent=_descent=0;

	target=NULL;
	surface=NULL;
	mask=NULL;
	source=NULL;

	fgRed=fgGreen=fgBlue=fgAlpha=1.0;
	bgRed=bgGreen=bgBlue=0; bgAlpha=1;

	transform_identity(ctm);
	transform_identity(ictm);
}

//! Dec count on cr and surface if they are not null.
DisplayerCairo::~DisplayerCairo()
{
	if (cr) cairo_destroy(cr);

	if (surface) cairo_surface_destroy(surface);
	if (mask) cairo_surface_destroy(mask);
	if (source) cairo_surface_destroy(source);

	if (curfont) cairo_font_face_destroy(curfont);
	if (curscaledfont) cairo_scaled_font_destroy(curscaledfont);
}

Displayer *DisplayerCairo::duplicate()
{
	return new DisplayerCairo();
}


//Display *DisplayerCairo::GetDpy() { return dpy; }
cairo_t *DisplayerCairo::GetCairo() { return cr; }



//------------------------- Surface prep functions ------------------------

void DisplayerCairo::SwapBuffers()
{ cout <<"*** imp DisplayerCairo::swapbuffers()"<<endl; }

void DisplayerCairo::BackBuffer(int on)
{ cout <<"*** imp DisplayerCairo::backbuffer()"<<endl; }

//void DisplayerCairo::WrapWindow(anXWindow *nw)
//{
//	Displayer::WrapWindow(nw);
//	MakeCurrent(nw);
//}

//! This sets up internals for drawing onto buffer, and wraps window if the min/max seem to not be set.
int DisplayerCairo::StartDrawing(aDrawable *buffer)
{
	DBG cerr<<"----DisplayerCairo Start Drawing"<<endl;

	MakeCurrent(buffer);
	return 0;
}

//! Make sure we are drawing on the proper surface.
int DisplayerCairo::MakeCurrent(aDrawable *buffer)
{
	if (cr && surface && buffer==dr) return 1; //already current!

	dr=buffer;
	xw=dynamic_cast<anXWindow*>(buffer);
	w=buffer->xlibDrawable();

	if (!xw) {
		Window rootwin;
		int x,y;
		unsigned int width,height,bwidth,depth;
		XGetGeometry(dpy,w,&rootwin,&x,&y,&width,&height,&bwidth,&depth);
		Minx=Miny=0;
		Maxx=width;
		Maxy=height;
	} else {
		Minx=Miny=0;
		Maxx=xw->win_w;
		Maxy=xw->win_h;
	}

	if (isinternal) {
		if (surface) cairo_surface_destroy(surface);
		surface=NULL;
		if (cr) { cairo_destroy(cr); cr=NULL; }
		isinternal=0;
	}

	if (!surface) {
		 //no existing surface, need to remap to an xlib_surface
		if (cr) { cairo_destroy(cr); cr=NULL; }
		surface=cairo_xlib_surface_create(dpy,w,vis,Maxx,Maxy);

	} else if (cairo_xlib_surface_get_drawable(surface)!=w) {
		 //we already have an xlib surface, just need to point to current xlib drawable
		cairo_xlib_surface_set_drawable(surface,w, Maxx,Maxy);
	}

	if (!cr) {
		cr=cairo_create(surface);
		if (!curfont) initFont();
		cairo_set_font_face(cr,curfont);
		if (_textheight>0) cairo_set_font_size(cr, _textheight);
		cairo_font_extents_t fextents;
		cairo_font_extents(cr, &fextents);
		_ascent =fextents.ascent;
		_descent=fextents.descent;
		_textheight=_ascent+_descent;
	}

	cairo_matrix_t m;
	if (real_coordinates) cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
	else cairo_matrix_init(&m, 1,0,0,1,0,0);
	cairo_set_matrix(cr, &m);

	return 1;
}

////! Find out what type of surface is currently set to be drawn on.
///*! 1 for internal surface, 2 for aDrawable, 3 for a back buffer of aDrawable.
// * 4 for mask.
// *
// * 0 is none.
// */
//int DisplayerCairo::CurrentSurface()
//{***
//}
//

//! Free any data associated with drawable.
/*! Return 1 for drawable not currently used, or 0 for cleared.
 */
int DisplayerCairo::ClearDrawable(aDrawable *drawable)
{
	if (!cr || !surface || drawable!=dr) return 1; //already cleared!

	if (surface) cairo_surface_destroy(surface);
	surface=NULL;
	if (cr) { cairo_destroy(cr); cr=NULL; }
	isinternal=0;

	dr=NULL;

	DBG cerr <<"DisplayerCairo::ClearDrawable()"<<endl;
	return 0;
}

LaxImage *DisplayerCairo::GetSurface()
{
	if (!surface) return NULL;

	cairo_surface_t *s=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, Maxx-Minx,Maxy-Miny);
	cairo_t *cc=cairo_create(s);
	cairo_set_source_surface(cc,surface, 0,0);
	cairo_paint(cc);
	LaxCairoImage *img=new LaxCairoImage(NULL,s);
	cairo_destroy(cc);
	return img;
}

//! Remove old surface, and create a fresh surface to perform drawing operations on.
int DisplayerCairo::CreateSurface(int w,int h, int type)
{
	xw=NULL;
	dr=NULL;
	w=0;
	cairo_surface_destroy(surface);
	if (cr) cairo_destroy(cr);

	isinternal=1;
	surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w,h);
	cr=cairo_create(surface);
	if (!curfont) initFont();
	cairo_set_font_face(cr,curfont);
	if (_textheight>0) cairo_set_font_size(cr, _textheight);
	cairo_font_extents_t fextents;
	cairo_font_extents(cr, &fextents);
	_ascent =fextents.ascent;
	_descent=fextents.descent;
	_textheight=_ascent+_descent;

	Minx=Miny=0;
	Maxx=w;
	Maxy=h;

	return 0;
}

//! Resize an internal drawing surface.
/*! If you had previously called CreateSurface(), this will resize that
 * surface. If the target surface is an external surface, then nothing is done.
 *
 * Return 0 for success, or 1 if not using an internal surface, and nothing done.
 */
int DisplayerCairo::ResizeSurface(int width, int height)
{
	if (!isinternal) return 1;
	if (width!=Maxx || height!=Maxy) return CreateSurface(width,height);
	return 0;
}

//! Free any resources allocated for drawing in this object.
/*! This resets all the drawing bits to 0. Be warned that most of the functions in
 * DisplayerCairo do not check for a NULL context or NULL surface!!
 *
 * If xw==NULL, then also call Updates(1);
 */
int DisplayerCairo::EndDrawing()
{
	if (xw==NULL) Updates(1);
	//if (xw) { xw->dec_count(); xw=NULL; }

	if (cr)      { cairo_destroy(cr); cr=NULL; }
	if (surface) { cairo_surface_destroy(surface); surface=NULL; }
	if (mask)    { cairo_surface_destroy(mask);    mask=NULL; }
	if (source)  { cairo_surface_destroy(source);  source=NULL; }
	return 0;
}


//------------------------ State -----------------------------


//! Set new foreground color. Typically usage is NewFG(.5,.8,0,1.0).
/*! Component range is clamped to [0..1.0].
 */
unsigned long DisplayerCairo::NewFG(double r,double g,double b,double a)
{
	unsigned long old=FG();
	if (r<0) r=0; else if (r>1.0) r=1.0;
	if (g<0) g=0; else if (g>1.0) g=1.0;
	if (b<0) b=0; else if (b>1.0) b=1.0;
	if (a<0) a=0; else if (a>1.0) a=1.0;
	fgRed=r; 
	fgGreen=g;
	fgBlue=b;
	fgAlpha=a;
	if (cr) cairo_set_source_rgba(cr, fgRed, fgGreen, fgBlue, fgAlpha);
	return old;
}

//! Set new foreground. Typically usage is NewFG(app->rgbcolor(23,34,234)).
/*! Component range is currently 0..255.... subject to change.
 */
unsigned long DisplayerCairo::NewFG(int r,int g,int b,int a)
{
	return NewFG((double)r/255,(double)g/255,(double)b/255,(double)a/255);
}

//! Set new foreground. Color components are 0..0xffff.
unsigned long DisplayerCairo::NewFG(ScreenColor *col)
{
	return NewFG((double)col->red/65535,(double)col->green/65535,(double)col->blue/65535,(double)col->alpha/65535);
}

//! Set new background. Color components are 0..0xffff.
unsigned long DisplayerCairo::NewBG(ScreenColor *col)
{
	return NewBG((double)col->red/65535,(double)col->green/65535,(double)col->blue/65535);
}

//! Set new foreground. Typically usage is NewFG(app->rgbcolor(23,34,234)).
unsigned long DisplayerCairo::NewFG(unsigned long ncol)
{
	int r,g,b;
	colorrgb(ncol,&r,&g,&b);
	return NewFG((double)r/255,(double)g/255,(double)b/255,1.0);
}

//! Set new background color. Typically usage is NewFG(app->rgbcolor(.5,.8,0)).
/*! Component range is [0..1.0].
 */
unsigned long DisplayerCairo::NewBG(double r,double g,double b)
{
	unsigned long old=BG();
	if (r<0) r=0; else if (r>1.0) r=1.0;
	if (g<0) g=0; else if (g>1.0) g=1.0;
	if (b<0) b=0; else if (b>1.0) b=1.0;
	bgRed=r; 
	bgGreen=g;
	bgBlue=b;
	return old;
}

//! Set new background. Typically usage is NewBG(app->rgbcolor(23,34,234)).
unsigned long DisplayerCairo::NewBG(int r,int g,int b)
{
	return NewBG((double)r/255,(double)g/255,(double)b/255);
}
	
//! Set new background. Typically usage is NewBG(app->rgbcolor(23,34,234)).
unsigned long DisplayerCairo::NewBG(unsigned long ncol)
{
	int r,g,b;
	colorrgb(ncol,&r,&g,&b);
	return NewBG((double)r/255,(double)g/255,(double)b/255);
}

unsigned long DisplayerCairo::FG() { return ((int)(fgAlpha*255)<<24) + ((int)(fgRed*255)<<16) + ((int)(fgGreen*255)<<8) + (fgBlue*255); }
unsigned long DisplayerCairo::BG() { return ((int)(bgAlpha*255)<<24) + ((int)(bgRed*255)<<16) + ((int)(bgGreen*255)<<8) + (bgBlue*255); }

//! Set how the source is blended onto the destination.
/*! The cairo modes are cairo_operator_t:
 * <pre>
 *  CAIRO_OPERATOR_CLEAR, 
 *  CAIRO_OPERATOR_SOURCE,
 *  CAIRO_OPERATOR_OVER,
 *  CAIRO_OPERATOR_IN,
 *  CAIRO_OPERATOR_OUT,
 *  CAIRO_OPERATOR_ATOP, 
 *  CAIRO_OPERATOR_DEST,
 *  CAIRO_OPERATOR_DEST_OVER,
 *  CAIRO_OPERATOR_DEST_IN,
 *  CAIRO_OPERATOR_DEST_OUT,
 *  CAIRO_OPERATOR_DEST_ATOP, 
 *  CAIRO_OPERATOR_XOR,
 *  CAIRO_OPERATOR_ADD,
 *  CAIRO_OPERATOR_SATURATE
 * </pre>
 */
void DisplayerCairo::setCairoBlendMode(cairo_operator_t mode)
{
	cairo_set_operator(cr, mode); 
}

LaxCompositeOp DisplayerCairo::BlendMode(LaxCompositeOp mode)
{
	LaxCompositeOp old=blendmode;
	cairo_operator_t m=CAIRO_OPERATOR_OVER;

	if (mode==LAXOP_Source        ) m=CAIRO_OPERATOR_SOURCE;
	else if (mode==LAXOP_Over     ) m=CAIRO_OPERATOR_OVER;
	else if (mode==LAXOP_Xor      ) m=CAIRO_OPERATOR_XOR;
	else if (mode==LAXOP_In       ) m=CAIRO_OPERATOR_IN;
	else if (mode==LAXOP_Out      ) m=CAIRO_OPERATOR_OUT;
	else if (mode==LAXOP_Atop     ) m=CAIRO_OPERATOR_ATOP;
	else if (mode==LAXOP_Dest     ) m=CAIRO_OPERATOR_DEST;
	else if (mode==LAXOP_Dest_over) m=CAIRO_OPERATOR_DEST_OVER;
	else if (mode==LAXOP_Dest_in  ) m=CAIRO_OPERATOR_DEST_IN;
	else if (mode==LAXOP_Dest_out ) m=CAIRO_OPERATOR_DEST_OUT;
	else if (mode==LAXOP_Dest_atop) m=CAIRO_OPERATOR_DEST_ATOP;
	else if (mode==LAXOP_Xor      ) m=CAIRO_OPERATOR_XOR;
	else if (mode==LAXOP_Add      ) m=CAIRO_OPERATOR_ADD;
	else if (mode==LAXOP_Saturate ) m=CAIRO_OPERATOR_SATURATE;
	else mode=LAXOP_None;
	
	if (mode!=LAXOP_None) {
		cairo_set_operator(cr,m);
		blendmode=mode;
	}

	return old;
}

double DisplayerCairo::setSourceAlpha(double alpha)
{
	cerr <<" *** need to implement DisplayerCairo::setSourceAlpha()!"<<endl;
	return 1;
}

//! Set the width, whether solid, line cap and join.
/*! This currently uses Xlib's names which are as follows.
 * 
 * width is only in screen pixels for now.
 * dash can be LineSolid, LineOnOffDash, or LineDoubleDash.
 * width is only in screen pixels for now.
 * See LaxJoinStyle and LaxCapStyle for values for cap and join.
 *
 * \todo should be able to set dash pattern somehow, and set line width based
 *   on current transform...
 */
void DisplayerCairo::LineAttributes(double width,int dash,int cap,int join)
{
	if (width>=0) cairo_set_line_width(cr,width);

	if (dash>=0) {
		if (dash==LineSolid) cairo_set_dash(cr,NULL,0,0);
		else {
			double l=width;
			if (l<=0) l=1;
			cairo_set_dash(cr, &l,1, 0);
			//cairo_set_dash(cr, *double, num_dashes, offset);
		}
	}

	if (cap>=0) {
		if (cap==LAXCAP_Butt) cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
		else if (cap==LAXCAP_Round) cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		else if (cap==LAXCAP_Projecting) cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	}

	if (join>=0) {
		if (join==LAXJOIN_Miter) cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
		else if (join==LAXJOIN_Round) cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
		else if (join==LAXJOIN_Bevel) cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	}

}

void DisplayerCairo::FillAttributes(int fillstyle, int fillrule)
{
	cerr <<"*** implement DisplayerCairo::FillAttributes"<<endl;
	//XSetFillRule(GetDpy(),GetGC(), fillrule);
	//XSetFillStyle(GetDpy(),GetGC(), fillstyle);
}

//! Clear the window to bgcolor between Min* and Max*. 
void DisplayerCairo::ClearWindow()
{
	DBG cerr <<"--displayer ClearWindow:MinMaxx,:["<<Minx<<","<<Maxx
	DBG		<<"] MinMaxy:["<<Miny<<","<<Maxy<<"] x0,y0:"<<ctm[4]<<","<<ctm[5]<<endl;

	if (xw==NULL || (xw && dr->xlibDrawable(-1)==dr->xlibDrawable(1))) {
		 //if using double buffer, XClearWindow will crash your program, so clear manually
		cairo_save(cr);
		cairo_identity_matrix(cr);
		cairo_operator_t oldmode=cairo_get_operator(cr);
		cairo_set_operator(cr,CAIRO_OPERATOR_OVER);
		if (xw) cairo_set_source_rgba(cr,
						(xw->win_colors->bg&0xff)/255.,
						((xw->win_colors->bg&0xff00)>>8)/255.,
						((xw->win_colors->bg&0xff0000)>>16)/255.,
						1.);
		else cairo_set_source_rgba(cr, bgRed, bgGreen, bgBlue, bgAlpha);
		cairo_rectangle(cr, Minx,Miny,Maxx-Minx+1,Maxy-Miny+1);
		cairo_fill(cr);
		cairo_set_source_rgba(cr, fgRed, fgGreen, fgBlue, fgAlpha);
		cairo_set_operator(cr,oldmode);
		cairo_restore(cr);
	} else XClearWindow(dpy, w);
}

//! Install a clip mask from a polyline (line is automatically closed)
/*! If append!=0, then add this path to the clipping path. Otherwise, remove any existing clip information,
 * and install this one path.
 */
int DisplayerCairo::Clip(flatpoint *p, int n, int append)
{
	int o=DrawImmediately(0);

	cairo_path_t *curpath=cairo_copy_path(cr);
	cairo_new_path(cr);
	drawlines(p,n, 1,0);
	if (!append) cairo_reset_clip(cr);
	cairo_clip(cr); //clears its current path
	cairo_append_path(cr,curpath);

	DrawImmediately(o);
	return 0;
}

//! Push the current clip mask onto a stack, make a new one maybe
void DisplayerCairo::PushClip(int startfresh)
{
	cairo_save(cr);
}

//! Restore a previous mask
void DisplayerCairo::PopClip()
{
	cairo_restore(cr);
}

//! Remove any mask
void DisplayerCairo::ClearClip()
{
	if (mask) cairo_surface_destroy(mask);
	mask=NULL;
	cairo_reset_clip(cr);
}

//! Return whether there is an active mask.
int DisplayerCairo::activeMask()
{
	return mask!=NULL;
}

//! Drawing operations following a call here will operate on the mask surface.
void DisplayerCairo::DrawOnMask()
{
	//if (!mask) ***create new mask;
	cerr <<" *** must implement DisplayerCairo::DrawOnMask()!"<<endl;
}

//! Drawing operations following a call here will operate on the main surface.
void DisplayerCairo::DrawOnSrc()
{
	cerr <<" *** must implement DisplayerCairo::DrawOnSrc()!"<<endl;
}


//! Flush any waiting composite operation, ie collapse source through mask onto surface.
void DisplayerCairo::show()
{
	if (source) cairo_set_source_surface(cr, source, 0,0); //else assume source has been set with something like NewFG()
	if (mask) cairo_mask_surface(cr,mask,0,0);
	else cairo_paint(cr);
}


//------------------path functions

/*! Using current path, fill with a gradient.
 */
void DisplayerCairo::fillgradient()
{
	//set up a cairo_pattern_t to use as drawing source

}

//! Draw out current path if any.
/*! If preserve!=0, then the path is not cleared.
 */
void DisplayerCairo::fill(int preserve)
{
	if (preserve) cairo_fill_preserve(cr);
	else cairo_fill(cr);
}

//! Draw out current path if any.
/*! If preserve!=0, then the path is not cleared.
 */
void DisplayerCairo::stroke(int preserve)
{
	if (preserve) cairo_stroke_preserve(cr);
	else cairo_stroke(cr);
}

void DisplayerCairo::moveto(flatpoint p)
{
	cairo_move_to(cr,p.x,p.y);
}

void DisplayerCairo::lineto(flatpoint p)
{
	cairo_line_to(cr,p.x,p.y);
}

//! Add a cubic bezier segment to the current path.
void DisplayerCairo::curveto(flatpoint c1,flatpoint c2,flatpoint v)
{
	cairo_curve_to(cr, c1.x,c1.y, c2.x,c2.y, v.x,v.y);
}

//! Call if current path should be closed, and close at the last added point.
void DisplayerCairo::closed()
{
	cairo_close_path(cr);
}

//! Call if current path should be ended, but not closed.
void DisplayerCairo::closeopen()
{
	cairo_new_sub_path(cr);
}


//! Draw one pixel at coord p.
void DisplayerCairo::drawpixel(flatpoint p)
{
	if (real_coordinates) p=realtoscreen(p);
	cairo_rectangle(cr, p.x,p.y, 1,1);
	cairo_fill(cr);
}


//! Draw optionally filled circle with screen radius r, at (x,y).
/*! (x,y) is real or screen according to DrawScreen() or DrawReal(),
 * but radius is still screen units.
 */
void DisplayerCairo::drawpoint(double x,double y,double radius,int tofill)
{
	flatpoint p(x,y);
	if (real_coordinates) p=realtoscreen(p);
	int old=real_coordinates;
	DrawScreen();
	drawarc(p,radius,radius,0,2*M_PI);
	if (old) DrawReal();
}

//! Draw a polygon, optionally fill.
/*! If fill==1 then fill with FG and have no border. If fill==2,
 * then fill with BG and border wich FG.
 */
void DisplayerCairo::drawlines(flatpoint *points,int npoints,char ifclosed,char tofill)
{
	flatpoint p=(real_coordinates ? realtoscreen(points[0]) : points[0]);
	if (!cairo_has_current_point(cr)) cairo_move_to(cr, p.x,p.y);
	int c;
	for (c=0; c<npoints; c++) {
		p=(real_coordinates ? realtoscreen(points[c]) : points[c]);
		cairo_line_to(cr, p.x,p.y);
	}
	if (ifclosed) cairo_close_path(cr);

	if (!draw_immediately) return;

	if (tofill==1) {
		cairo_fill(cr);
	} else if (tofill==2) {
		cairo_set_source_rgba(cr, bgRed, bgGreen, bgBlue, bgAlpha);
		cairo_fill_preserve(cr);
		cairo_set_source_rgba(cr, fgRed, fgGreen, fgBlue, fgAlpha); 
	}
	if (tofill!=1) cairo_stroke(cr);
}

void DisplayerCairo::drawline(flatpoint p1,flatpoint p2)
{
	cairo_move_to(cr, p1.x,p1.y);
	cairo_line_to(cr, p2.x,p2.y);
	if (draw_immediately) stroke(0);
}

void DisplayerCairo::drawline(double ax,double ay,double bx,double by)
{
	cairo_move_to(cr, ax,ay);
	cairo_line_to(cr, bx,by);
	if (draw_immediately) stroke(0);
}


//-------------------------text functions


int DisplayerCairo::font(LaxFont *nfont, double size)
{
	LaxFontCairo *cairofont=dynamic_cast<LaxFontCairo*>(nfont);
    if (!cairofont) return 1;

	if (curfont!=cairofont->font) {
		if (curfont) cairo_font_face_destroy(curfont);//really just a melodramatic dec count
		curfont=cairofont->font;
		cairo_font_face_reference(curfont);
	}

	if (curscaledfont) { cairo_scaled_font_destroy(curscaledfont); curscaledfont=NULL; }

	cairo_set_font_face(cr,curfont);
	fontsize(size);
	return 1;
}

int DisplayerCairo::textheight()
{
	if (!curfont) initFont();

	return _textheight;
}

/*! Return 0 for success, 1 for fail.
 */
int DisplayerCairo::font(const char *fontconfigpattern)
{
	if (!fontconfigpattern) return 1;

	FcPattern *pattern=FcNameParse((FcChar8*)fontconfigpattern);
	cairo_font_face_t *newfont;
	newfont=cairo_ft_font_face_create_for_pattern(pattern);
	FcPatternDestroy(pattern);
	if (curfont) cairo_font_face_destroy(curfont);//really just a melodramatic dec count
	curfont=newfont;

	if (curscaledfont) { cairo_scaled_font_destroy(curscaledfont); curscaledfont=NULL; }
	FcPatternDestroy(pattern);

	cairo_set_font_face(cr,curfont);
	return 0;
}

int DisplayerCairo::font(const char *family,const char *style,double pixelsize)
{
	FcValue v;
	FcPattern *pattern=FcPatternCreate();
	v.type=FcTypeString; v.u.s=(FcChar8*)family;
	FcPatternAdd(pattern,FC_FAMILY,v,FcTrue);
	_textheight=pixelsize;

	if (style) {
		v.type=FcTypeString; v.u.s=(FcChar8*)style;
		FcPatternAdd(pattern,FC_STYLE,v,FcTrue);
	}
	v.type=FcTypeDouble; v.u.d=pixelsize;
	FcPatternAdd(pattern,FC_SIZE,v,FcTrue);

	cairo_font_face_t *newfont;
	newfont=cairo_ft_font_face_create_for_pattern(pattern);
	if (curfont) cairo_font_face_destroy(curfont);
	curfont=newfont;

	 //curscaledfont is used for extent finding... *** need to investigate if necessary!! extents are screen extents?
	if (curscaledfont) { cairo_scaled_font_destroy(curscaledfont); curscaledfont=NULL; }
	//cairo_matrix_t matrix;
	//cairo_matrix_init_scale(&matrix, pixelsize, pixelsize);
	//cairo_font_options_t *options=cairo_font_options_create();
	//curscaledfont=cairo_scaled_font_create(curfont, &matrix, ****ctm, options);
	//cairo_font_options_destroy(options);

	FcPatternDestroy(pattern);

	if (cr) {
		cairo_set_font_face(cr, curfont);
		cairo_set_font_size(cr, pixelsize);

		cairo_font_extents_t fextents;
		cairo_font_extents(cr, &fextents);
		_ascent =fextents.ascent;
		_descent=fextents.descent;
	}

	return 0;
}

int DisplayerCairo::fontsize(double size)
{
	cairo_set_font_size(cr, size);
	_textheight=size;

	cairo_font_extents_t fextents;
	cairo_font_extents(cr, &fextents);
	_ascent =fextents.ascent;
	_descent=fextents.descent;
	return 0;
}

/*! str should be utf8, and len should be number of bytes into str.
 */
double DisplayerCairo::textextent(LaxFont *thisfont, const char *str,int len, double *width,double *height,double *ascent,double *descent,char real)
{
	if (!curfont) initFont();

   if (str==NULL || !curfont) {
        if (width) *width=0;
        if (height) *height=0;
        if (ascent) *ascent=0;
        if (descent) *descent=0;
        return 0;
    }

	if (len<0) len=strlen(str);
	if (len>bufferlen) reallocBuffer(len);

	cairo_text_extents_t extents;
	memcpy(buffer,str,len);
	buffer[len]='\0';
	cairo_text_extents(cr, buffer, &extents);

	cairo_font_extents_t fextents;
	cairo_font_extents(cr, &fextents);

	if (ascent)  *ascent =fextents.ascent;
	if (descent) *descent=fextents.descent;
	if (height)  { if (real) *height=extents.height; else *height=fextents.ascent+fextents.descent; }
	if (width)   *width  =extents.width;

	return extents.width;
}

void DisplayerCairo::initFont()
{
	if (curfont) return;
	LaxFont *def=anXApp::app->defaultlaxfont;
	font("sans","normal",def->textheight());

	if (!cr) {
		if (!surface) CreateSurface(50,50,0);
	}

//	if (cr) {
//		cairo_set_font_face(cr, curfont);
//		cairo_set_font_size(cr, pixelsize);
//	}
}

/*! Reallocate only when necessary.
 */
int DisplayerCairo::reallocBuffer(int len)
{
	if (len<bufferlen) return bufferlen;

	if (buffer) delete[] buffer;
	bufferlen=len+30;
	buffer=new char[bufferlen];

	return bufferlen;
}

//! Draw a single line of text at x,y.
/*! str is utf8.
 */
double DisplayerCairo::textout(double x,double y,const char *str,int len,unsigned long align)
{
	if (len<0) len=strlen(str);
	if (len>bufferlen) reallocBuffer(len);
	strncpy(buffer,str,len);
	buffer[len]='\0';

	if (!curfont) initFont();


    int ox,oy;
	if (align&LAX_LEFT) ox=x;
	else {
		cairo_text_extents_t extents;
		cairo_text_extents(cr, buffer, &extents);

		if (align&LAX_RIGHT) ox=x-extents.width;
		else ox=x-extents.width/2; //center
	}

    if (align&LAX_TOP) oy=y+_ascent;
    else if (align&LAX_BOTTOM) oy=y-_descent;
    else if (align&LAX_BASELINE) oy=y;
    else oy=y-(_ascent+_descent)/2+_ascent; //center


	cairo_move_to(cr, ox,oy);
	if (len==0) return 0;

	cairo_show_text(cr, buffer);
	cairo_fill(cr);

	//DBG drawline(ox,oy-_ascent, ox+50,oy-_ascent);
	//DBG drawline(ox,oy, ox+50,oy);
	//DBG drawline(ox,oy+_descent, ox+50,oy+_descent);
	//DBG drawpoint(x,y, 5,0);


	return 0; // ***
}

double DisplayerCairo::textout(double *matrix,double x,double y,const char *str,int len,unsigned long align)
{
	cairo_save(cr);

	cairo_matrix_t m;
	cairo_matrix_init(&m,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	cairo_set_font_matrix(cr,&m);
	double d=textout(x,y,str,len,align);

	cairo_restore(cr);
	return d;
}

double DisplayerCairo::textout(double angle,double x,double y,const char *str,int len,unsigned long align)
{
	cairo_save(cr);
	cairo_rotate(cr,angle);
	double d=textout(x,y,str,len,align);
	cairo_restore(cr);
	return d;
}


//-------------------------image functions

////! Export a portion of the current surface to a LaxCairoImage.
//void DisplayerCairo::imagePartialOut(LaxImage *image, int dx,int dy, int sx,int sy,int sw,int sh)
//{***}

void DisplayerCairo::imageout(LaxImage *img,double x,double y)
{
	if (!img || img->imagetype()!=LAX_IMAGE_CAIRO) return; 
	LaxCairoImage *i=dynamic_cast<LaxCairoImage*>(img);

	cairo_surface_t *t=i->Image();
	cairo_set_source_surface(cr, t, 0,0);
	if (mask) cairo_mask_surface(cr,mask,0,0);
	else cairo_paint(cr);
	img->doneForNow();
}

void DisplayerCairo::imageout(LaxImage *img,double *matrix)
{
	return;
//***
//	if (!img || img->imagetype()!=LAX_IMAGE_CAIRO) return; 
//	LaxCairoImage *i=dynamic_cast<LaxCairoImage*>(img);
//	PushAndNewTransform(matrix);
//	imageout(img,0,0);
//	PopAxes();
}

int DisplayerCairo::imageout(LaxImage *image, double x,double y, double w,double h)
{
	double sx=w/image->w();
	double sy=h/image->h();

	double m[6];
	m[0]=sx;
	m[3]=sy;
	PushAndNewTransform(m);
	imageout(image,0,0);
	PopAxes();

	return 0;
}

void DisplayerCairo::imageout_rotated(LaxImage *img,double x,double y,double ulx,double uly)
{
	if (!img || img->imagetype()!=LAX_IMAGE_CAIRO) return; 
	LaxCairoImage *i=dynamic_cast<LaxCairoImage*>(img);
	if (!i) return;

	return imageout(img,x,y); // ***
}

void DisplayerCairo::imageout_skewed(LaxImage *img,double x,double y,double ulx,double uly,double urx,double ury)
{
	if (!img || img->imagetype()!=LAX_IMAGE_CAIRO) return; 
	LaxCairoImage *i=dynamic_cast<LaxCairoImage*>(img);
	if (!i) return;

	return imageout(img,x,y); // ***
}

void DisplayerCairo::imageout(LaxImage *img,double angle, double x,double y)
{
	PushAxes();
	Rotate(angle,x,y);
	imageout(img,0,0);
	PopAxes();
}


//---------------------viewport management

//! Convert real point p to screen coordinates.
/*! <pre>
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *  </pre>
 */
flatpoint DisplayerCairo::realtoscreen(flatpoint p)
{
//	if (!cr) MakeCurrent(dr);
//
//	if (real_coordinates) {
//		double x=p.x,y=p.y;
//		cairo_user_to_device(cr, &x,&y);
//		return flatpoint(x,y);
//	}
	return transform_point(ctm,p);
} 

//! Convert real point (x,y) to screen coordinates.
/*! <pre>
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *  </pre>
 */
flatpoint DisplayerCairo::realtoscreen(double x,double y)
{
//	if (!cr) MakeCurrent(dr);
//
//	if (real_coordinates) {
//		double xx=x,yy=y;
//		cairo_user_to_device(cr, &xx,&yy);
//		return flatpoint(xx,yy);
//	}
	return transform_point(ctm,flatpoint(x,y));
}

//! Convert screen point (x,y) to real coordinates.
flatpoint DisplayerCairo::screentoreal(int x,int y)
{
//	if (!cr) MakeCurrent(dr);
//
//	if (real_coordinates) {
//		double xx=x,yy=y;
//		cairo_device_to_user(cr, &xx,&yy);
//		return flatpoint(xx,yy);
//	}
	return transform_point(ictm,flatpoint(x,y));
}

//! Convert screen point to real coordinates.
flatpoint DisplayerCairo::screentoreal(flatpoint p)
{
//	if (!cr) MakeCurrent(dr);

//	if (real_coordinates) {
//		double xx=p.x,yy=p.y;
//		cairo_device_to_user(cr, &xx,&yy);
//		return flatpoint(xx,yy);
//	}
	return transform_point(ictm,p);
}

//! Return a pointer to the current transformation matrix.
const double *DisplayerCairo::Getctm()
{
	//--- bad to rely on cr:
	//cairo_matrix_t m;
	//cairo_get_matrix(cr, &m);
	//transform_set(ctm, m.xx,m.yx, m.xy,m.yy, m.x0,m.y0);
	return ctm;
}

//! Return a pointer to the inverse of the current transformation matrix.
const double *DisplayerCairo::Getictm()
{
	//cairo_matrix_t m;
	//cairo_get_matrix(cr, &m);
	//cairo_matrix_invert(&m);
	//transform_set(ictm, m.xx,m.yx, m.xy,m.yy, m.x0,m.y0);
	//return ictm;
	//----
	transform_invert(ictm,ctm);
	return ictm;
}


//! Move the viewable portion by dx,dy screen units.
void DisplayerCairo::ShiftScreen(int dx,int dy)
{
	if (cr && real_coordinates) cairo_translate(cr,dx,dy);

	ictm[4]-=dx;
	ictm[5]-=dy;
	ctm[4]+=dx;
	ctm[5]+=dy;

	syncPanner();
}

//! Set the ctm to these 6 numbers.
void DisplayerCairo::NewTransform(const double *d)
{
	if (cr && real_coordinates) {
		cairo_matrix_t m;
		m.xx=d[0];
		m.yx=d[1];
		m.xy=d[2];
		m.yy=d[3];
		m.x0=d[4];
		m.y0=d[5];
		cairo_set_matrix(cr, &m);
	}

	transform_copy(ctm,d);
	transform_invert(ictm,ctm);

	syncPanner();
}

//! Make the transform correspond to the values.
void DisplayerCairo::NewTransform(double a,double b,double c,double d,double x0,double y0)
{
	if (cr && real_coordinates) {
		cairo_matrix_t m;
		m.xx=a;
		m.yx=b;
		m.xy=c;
		m.yy=d;
		m.x0=x0;
		m.y0=y0;
		cairo_set_matrix(cr, &m);
	}

	ctm[0]=a;
	ctm[1]=b;
	ctm[2]=c;
	ctm[3]=d;
	ctm[4]=x0;
	ctm[5]=y0;
	transform_invert(ictm,ctm);

	syncPanner();
}

//! Push the current axes on the axessstack. Relying on cr existing is problematic.
void DisplayerCairo::PushAxes()
{
	if (cr) cairo_save(cr);

	double *tctm=new double[6];
	transform_copy(tctm,ctm);
	axesstack.push(tctm,2);
}

//! Recover the last pushed axes. Relying on cr existing is problematic.
void DisplayerCairo::PopAxes()
{
	if (axesstack.n==0) return;
	double *tctm=axesstack.pop();
	transform_copy(ctm,tctm);
	transform_invert(ictm,ctm);
	delete[] tctm;
	
	if (cr) {
		cairo_restore(cr);

		cairo_matrix_t m;
		cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
		cairo_set_matrix(cr, &m);
	}
}

int DisplayerCairo::DrawReal()
{
	int r=Displayer::DrawReal();

	if (cr) {
		cairo_matrix_t m;
		cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
		cairo_set_matrix(cr, &m);
	}

	return r;
}

int DisplayerCairo::DrawScreen()
{
	int r=Displayer::DrawReal();

	if (cr) {
		cairo_matrix_t m;
		cairo_matrix_init(&m, 1,0,0,1,0,0);
		cairo_set_matrix(cr, &m);
	}

	return r;
}


} // namespace Laxkit

#endif //cairo



