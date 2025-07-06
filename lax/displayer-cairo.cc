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
//    Copyright (C) 2012-2015 by Tom Lechner
//


#include <lax/displayer-cairo.h>
#include <lax/fontmanager-cairo.h>
#include <lax/laximages-cairo.h>



#ifdef LAX_USES_CAIRO



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



void dump_transforms(cairo_t *cr, double *d)
{
	if (!cr) return;

	cairo_matrix_t m;
	cairo_get_matrix(cr, &m);

	cerr <<
		m.xx<<"   "<<d[0]<<endl<<
		m.yx<<"   "<<d[1]<<endl<<
		m.xy<<"   "<<d[2]<<endl<<
		m.yy<<"   "<<d[3]<<endl<<
		m.x0<<"   "<<d[4]<<endl<<
		m.y0<<"   "<<d[5]<<endl;
}



/*! set cr=nullptr, and surface=nullptr.
 */
DisplayerCairo::DisplayerCairo(anXWindow *nxw,PanController *pan) 
	: Displayer(nxw,pan)
{
	base_init();
}

void DisplayerCairo::base_init()
{
	tbuffer = nullptr;
	tbufferlen = 0;

	isinternal = 0;
	imagebuffer = nullptr;

#ifdef _LAX_PLATFORM_XLIB
	if (xw) {
		dpy = xw->app->dpy;
		vis = xw->app->vis;
		w   = xw->xlib_window;
	} else {
		dpy = anXApp::app->dpy;
		vis = anXApp::app->vis;
		w   = 0;
	}
#endif

	blendmode = LAXOP_Over;
	on = 0;

	cr = nullptr;
	//cr_source = nullptr;
	laxfont = nullptr;
	curfont = nullptr;
	curscaledfont = nullptr; // *** not currently used
	_textheight = 0;
	height_over_M = 0; // = (M square height: cairo_set_font_size parameter) / (actual font height) 

	target  = nullptr;
	surface = nullptr;
	ref_surface = nullptr;
	mask = nullptr;
	mask_pattern = nullptr;
	source = nullptr;

	cairo_glyphs = nullptr;
	numalloc_glyphs = 0;

	fgRed = fgGreen = fgBlue = fgAlpha = 1.0;
	bgRed = bgGreen = bgBlue = 0; bgAlpha = 1;
	current_alpha = -1; // 0..1 means paint source with this additional alpha. otherwise do nothing special on paint

	transform_identity(ctm);
	transform_identity(ictm);
}

//! Dec count on cr and surface if they are not null.
DisplayerCairo::~DisplayerCairo()
{
	if (cr)           cairo_destroy(cr);
	//if (cr_source)    cairo_destroy(cr_source);

	if (surface)      cairo_surface_destroy(surface);
	if (ref_surface)  cairo_surface_destroy(ref_surface);
	if (mask)         cairo_surface_destroy(mask);
	if (mask_pattern) cairo_pattern_destroy(mask_pattern);
	if (source)       cairo_surface_destroy(source);

	if (laxfont)       laxfont->dec_count();
	if (curfont)       cairo_font_face_destroy(curfont);
	if (curscaledfont) cairo_scaled_font_destroy(curscaledfont);

	if (imagebuffer) imagebuffer->dec_count();

	delete[] cairo_glyphs;
	delete[] tbuffer;
}

Displayer *DisplayerCairo::duplicate()
{
	return new DisplayerCairo();
}


cairo_t *DisplayerCairo::GetCairo() { return cr; }



//------------------------- Surface prep functions ------------------------

void DisplayerCairo::SwapBuffers()
{ cout <<"*** imp DisplayerCairo::swapbuffers()"<<endl; }

void DisplayerCairo::BackBuffer(int on)
{ cout <<"*** imp DisplayerCairo::backbuffer()"<<endl; }

//! This sets up internals for drawing onto buffer, and wraps window if the min/max seem to not be set.
int DisplayerCairo::StartDrawing(aDrawable *buffer)
{
	DBG cerr<<"----DisplayerCairo Start Drawing"<<endl;

	MakeCurrent(buffer);
	Updates(0);
	NewFG(fgRed, fgGreen, fgBlue, fgAlpha);
	return 0;
}

/*! Free any resources allocated for drawing in this object created during StartDrawing() or MakeCurrent()..
 *
 *  This resets all the drawing bits to 0. Be warned that most of the functions in
 * DisplayerCairo do not check for a nullptr context or nullptr surface!!
 *
 * If xw==nullptr, then also call Updates(1);
 */
int DisplayerCairo::EndDrawing()
{
	if (xw == nullptr) Updates(1);

	if (cr)           { cairo_destroy(cr);                   cr = nullptr;           }
	if (surface)      { cairo_surface_destroy(surface);      surface = nullptr;      }
	if (mask)         { cairo_surface_destroy(mask);         mask = nullptr;         }
	if (mask_pattern) { cairo_pattern_destroy(mask_pattern); mask_pattern = nullptr; }
	if (source)       { cairo_surface_destroy(source);       source = nullptr;       }
	return 0;
}

/*! Called in response to a window resize, this must update if the current surface is buffer.
 * If current surface is not buffer, then nothing is done.
 */
int DisplayerCairo::CurrentResized(aDrawable *buffer, int nwidth,int nheight)
{
	if (buffer != dr) return 1;

	if (buffer->xlibDrawable(1)==0
			|| buffer->xlibDrawable(0)==buffer->xlibDrawable(1)) {
		 //not double buffered, easy..
		DBG cerr <<"cairo_xlib_surface_set_size("<<nwidth<<nheight<<")"<<endl;
		if (surface) cairo_xlib_surface_set_size(surface, nwidth,nheight);
		//cairo_xlib_surface_set_drawable(surface,w, nwidth,nheight);
		Maxx = nwidth;
		Maxy = nheight;
		return 0;
	}

	EndDrawing();
	//MakeCurrent(buffer);
	//w=buffer->xlibDrawable();

	return 0;
}

int DisplayerCairo::MakeCurrent(LaxImage *buffer)
{
	if (!buffer) return 2;
	LaxCairoImage *img=dynamic_cast<LaxCairoImage*>(buffer);
	if (!img) return 3;

	if (cr && imagebuffer==buffer) return 0;

	if (imagebuffer!=buffer) {
		if (imagebuffer) imagebuffer->dec_count();
		imagebuffer=buffer;
		imagebuffer->inc_count();

		if (cr) { cairo_destroy(cr); cr=nullptr; }
		if (surface) { cairo_surface_destroy(surface); surface=nullptr; }
	}

	dr=nullptr;
	xw=nullptr;
	w=0;

	Minx=Miny=0;
	Maxx=buffer->w();
	Maxy=buffer->h();

	if (isinternal) {
		 //we need to destroy internal, as we are now using external buffer
		if (cr) { cairo_destroy(cr); cr=nullptr; }
		if (surface) cairo_surface_destroy(surface);
		surface=nullptr;
		isinternal=0;
	}

	if (surface != img->image) {
		if (cr) { cairo_destroy(cr); cr = nullptr; }
		if (surface) cairo_surface_destroy(surface);
		surface = img->image;
		cairo_surface_reference(surface);
	}


	if (!cr) {
		cr = cairo_create(surface);

		if (!curfont) initFont();
		cairo_set_font_face(cr,curfont);
		if (_textheight>0) cairo_set_font_size(cr, _textheight/height_over_M);
		cairo_font_extents(cr, &curfont_extents);
	}

	cairo_matrix_t m;
	if (real_coordinates) cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
	else cairo_matrix_init(&m, 1,0,0,1,0,0);
	cairo_set_matrix(cr, &m);
	transform_invert(ictm,ctm);

	return 0;
}

//! Make sure we are drawing on the proper surface.
int DisplayerCairo::MakeCurrent(aDrawable *buffer)
{
	if (!buffer) { EndDrawing(); return -1; }

	if (cr && cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cerr << " *** WARNING!!! cairo in error status: "<<cairo_status_to_string(cairo_status(cr))<<"!! recreating cr..."<<endl;
		cairo_destroy(cr);
		cr = nullptr;
	}

	if (cr && surface && buffer == dr && w == buffer->xlibDrawable()) return 0; //already current!

	dr = buffer;
	xw = dynamic_cast<anXWindow*>(buffer);
	w = buffer->xlibDrawable();
	if (imagebuffer) { imagebuffer->dec_count(); imagebuffer = nullptr; }


	if (!xw) {
#ifdef _LAX_PLATFORM_XLIB
		 //buffer was not an anXWindow, buffer is probably an xlib Pixmap or something:
		Window rootwin;
		int x,y;
		unsigned int width,height,bwidth,depth;
		XGetGeometry(dpy,w,&rootwin,&x,&y,&width,&height,&bwidth,&depth);
		Minx=Miny=0;
		Maxx=width;
		Maxy=height;
#endif
	} else {
		Minx=Miny=0;
		Maxx=xw->win_w;
		Maxy=xw->win_h;
	}

	if (isinternal) {
		 //we need to destroy internal, as we are now using external buffer
		if (cr) { cairo_destroy(cr); cr = nullptr; }
		if (surface) { cairo_surface_destroy(surface); surface = nullptr; }
		isinternal = 0;
	}

#ifdef _LAX_PLATFORM_XLIB
	if (!surface) {
		 //no existing surface, need to remap to an xlib_surface
		if (cr) { cairo_destroy(cr); cr = nullptr; }
		surface = cairo_xlib_surface_create(dpy,w,vis,Maxx,Maxy);

	} else if (cairo_xlib_surface_get_drawable(surface)!=w) {
		 //we already have an xlib surface, just need to point to current xlib drawable
		cairo_xlib_surface_set_drawable(surface,w, Maxx,Maxy);
	}
#endif

	if (!cr) {
		cr = cairo_create(surface);

		if (!curfont) initFont();
		cairo_set_font_face(cr,curfont);
		if (_textheight>0) cairo_set_font_size(cr, _textheight/height_over_M);
		cairo_font_extents(cr, &curfont_extents);
	}


	cairo_matrix_t m;
	//transform_identity(ctm);
	if (real_coordinates) cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
	else cairo_matrix_init(&m, 1,0,0,1,0,0);
	cairo_set_matrix(cr, &m);
	transform_invert(ictm,ctm);

	return 0;
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

	if (surface) { cairo_surface_destroy(surface); surface = nullptr; }
	if (cr) { cairo_destroy(cr); cr = nullptr; }
	isinternal = 0;

	dr = nullptr;

	DBG cerr <<"DisplayerCairo::ClearDrawable()"<<endl;
	return 0;
}

/*! If surface was set with MakeCurrent(LaxImage*), then return that image, with
 * its count incremented. Else, create and return a new LaxImage.
 */
LaxImage *DisplayerCairo::GetSurface()
{
	if (!surface) return nullptr;
	if (imagebuffer) {
		imagebuffer->inc_count();
		return imagebuffer;
	}

	cairo_surface_t *s=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, Maxx-Minx,Maxy-Miny);
	cairo_t *cc=cairo_create(s);
	cairo_set_source_surface(cc,surface, 0,0);
	cairo_paint(cc);
	LaxCairoImage *img=new LaxCairoImage(nullptr,s);
	cairo_destroy(cc);
	return img;
}

//! Remove old surface, and create a fresh surface to perform drawing operations on.
int DisplayerCairo::CreateSurface(int width,int height, int type)
{
	xw=nullptr;
	dr=nullptr;
	w=0;
	if (imagebuffer) { imagebuffer->dec_count(); imagebuffer=nullptr; }

	if (surface) cairo_surface_destroy(surface);
	if (cr) cairo_destroy(cr);

	isinternal = 1;
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width,height);
	cr = cairo_create(surface);

	cairo_matrix_t m;
	if (real_coordinates) cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
	else cairo_matrix_init(&m, 1,0,0,1,0,0);
	cairo_set_matrix(cr, &m);
	transform_invert(ictm,ctm);

	if (!curfont) initFont();
	cairo_set_font_face(cr,curfont);
	if (_textheight>0) cairo_set_font_size(cr, _textheight/height_over_M);
	cairo_font_extents(cr, &curfont_extents);

	Minx=Miny=0;
	Maxx=width;
	Maxy=height;

	return 0;
}

/*! Resize an internal drawing surface.
 *
 *  If you had previously called CreateSurface(), this will resize that
 * surface. If the target surface is an external surface or an image surface,
 * then nothing is done.
 *
 * Return 0 for success, or 1 if not using an internal surface, and nothing done.
 */
int DisplayerCairo::ResizeSurface(int width, int height)
{
	if (!isinternal) return 1;
	if (width!=Maxx || height!=Maxy) return CreateSurface(width,height);
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

	//DBG  cerr <<"New fg: "<<fgRed<<"  "<<fgGreen<<"  "<<fgBlue<<"  "<<fgAlpha<<endl;
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
unsigned long DisplayerCairo::NewFG(const ScreenColor *col)
{
	return NewFG((double)col->red/65535,(double)col->green/65535,(double)col->blue/65535,(double)col->alpha/65535);
}

//! Set new foreground.
unsigned long DisplayerCairo::NewFG(Color *col)
{
	if (col->colorsystemid == LAX_COLOR_RGB)
		return NewFG(col->ChannelValue(0), col->ChannelValue(1), col->ChannelValue(2), col->Alpha());
	return NewFG(&col->screen);
}

//! Set new background. Color components are 0..0xffff.
unsigned long DisplayerCairo::NewBG(const ScreenColor *col)
{
	return NewBG((double)col->red/65535,(double)col->green/65535,(double)col->blue/65535,(double)col->alpha/65535);
}

//! Set new foreground. ncol is 0xaabbggrr.
unsigned long DisplayerCairo::NewFG(unsigned long ncol)
{
	int r,g,b,a;
	colorrgb(ncol,&r,&g,&b,&a);
	return NewFG((double)r/255,(double)g/255,(double)b/255,(double)a/255);
}

//! Set new background color. Typically usage is NewFG(app->rgbcolor(.5,.8,0)).
/*! Component range is [0..1.0].
 */
unsigned long DisplayerCairo::NewBG(double r,double g,double b,double a)
{
	unsigned long old=BG();
	if (r<0) r=0; else if (r>1.0) r=1.0;
	if (g<0) g=0; else if (g>1.0) g=1.0;
	if (b<0) b=0; else if (b>1.0) b=1.0;
	if (a<0) a=0; else if (a>1.0) a=1.0;
	bgRed   = r; 
	bgGreen = g;
	bgBlue  = b;
	bgAlpha = a;
	return old;
}

//! Set new background. Typically usage is NewBG(app->rgbcolor(23,34,234)).
unsigned long DisplayerCairo::NewBG(int r,int g,int b,int a)
{
	return NewBG((double)r/255,(double)g/255,(double)b/255,(double)a/255);
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

//int DisplayerCairo::FGLinearGradient(flatpoint p1, flatpoint p2, int extend, GradientStrip *strip)
//{
//}
//int DisplayerCairo::FGRadialGradient(flatpoint p1, flatpoint p2, int extend, GradientStrip *strip)
//{
//}
//int DisplayerCairo::DrawMesh(double *extra, ColorPatchData *mesh)
//{
//}

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

	if (mode==LAXOP_Source         ) m=CAIRO_OPERATOR_SOURCE;
	else if (mode==LAXOP_Over      ) m=CAIRO_OPERATOR_OVER;
	else if (mode==LAXOP_Xor       ) m=CAIRO_OPERATOR_XOR;
	else if (mode==LAXOP_In        ) m=CAIRO_OPERATOR_IN;
	else if (mode==LAXOP_Out       ) m=CAIRO_OPERATOR_OUT;
	else if (mode==LAXOP_Atop      ) m=CAIRO_OPERATOR_ATOP;
	else if (mode==LAXOP_Dest      ) m=CAIRO_OPERATOR_DEST;
	else if (mode==LAXOP_Dest_over ) m=CAIRO_OPERATOR_DEST_OVER;
	else if (mode==LAXOP_Dest_in   ) m=CAIRO_OPERATOR_DEST_IN;
	else if (mode==LAXOP_Dest_out  ) m=CAIRO_OPERATOR_DEST_OUT;
	else if (mode==LAXOP_Dest_atop ) m=CAIRO_OPERATOR_DEST_ATOP;
	else if (mode==LAXOP_Add       ) m=CAIRO_OPERATOR_ADD;
	else if (mode==LAXOP_Saturate  ) m=CAIRO_OPERATOR_SATURATE;
	else if (mode==LAXOP_Multiply  ) m=CAIRO_OPERATOR_MULTIPLY;
	else if (mode==LAXOP_Difference) m=CAIRO_OPERATOR_DIFFERENCE;
	else mode=LAXOP_None;

	if (mode!=LAXOP_None) {
		if (cr) cairo_set_operator(cr,m);
		blendmode=mode;
	}

	return old;
}

 /*! Start operations that will be converted to a source on PopGroup().
  */
void DisplayerCairo::PushGroup()
{
	if (!cr) return;
	cairo_push_group(cr);
}

/*! Stop operations since the last PushGroup(), and convert them to
 * the current source. You might then set BlendMode() or setSourceAlpha()
 * before a show() command to flush operations.//start operations that will be converted to a paint source on PopGroup()
 */
void DisplayerCairo::PopGroup()
{
	if (!cr) return;
	cairo_pop_group_to_source(cr);
}

/*! sets current_alpha, which is used when users call show() If neither mask nor mask_pattern exists.
 * Returns old value.
 */
double DisplayerCairo::setCurrentAlpha(double alpha)
{
	double old = current_alpha;
	current_alpha = alpha;
	return old;
}

double DisplayerCairo::setSourceAlpha(double alpha)
{
	if (mask) { cairo_surface_destroy(mask); mask=nullptr; }
	if (mask_pattern) { cairo_pattern_destroy(mask_pattern); mask_pattern=nullptr; }

	if (alpha<0) alpha=0;

	if (alpha>=1.0) alpha=1.0;
	else mask_pattern = cairo_pattern_create_rgba(0.,0.,0.,alpha);

	return 1;
}

double DisplayerCairo::LineWidth()
{
	if (!cr) return 0;
	return cairo_get_line_width(cr);
}

double DisplayerCairo::LineWidth(double newwidth)
{
	if (!cr) return 0;

	double old=cairo_get_line_width(cr);
	cairo_set_line_width(cr,newwidth);
	return old;
}

double DisplayerCairo::LineWidthScreen(double newwidth)
{
	if (!cr) return 0;

	double old=cairo_get_line_width(cr);
	if (real_coordinates) {
		newwidth/=Getmag();
	}
	cairo_set_line_width(cr,newwidth);
	return old;
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
		if (dash == 0) cairo_set_dash(cr,nullptr,0,0);
		else {
			if (width < 0) width = cairo_get_line_width(cr);
			double l=width*5;
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

void DisplayerCairo::LineJoin(int join)
{
	if (join >= 0) {
		if      (join == LAXJOIN_Miter) cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
		else if (join == LAXJOIN_Round) cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
		else if (join == LAXJOIN_Bevel) cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	}
}

void DisplayerCairo::LineCap(int cap)
{
	if (cap >= 0) {
		if      (cap == LAXCAP_Butt)       cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
		else if (cap == LAXCAP_Round)      cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		else if (cap == LAXCAP_Projecting) cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	}
}

void DisplayerCairo::FillAttributes(int fillstyle, int fillrule)
{
	if (!cr) return;

	if (fillrule == LAXFILL_Nonzero) cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
	else cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
	// *** what about fillstyle?? solid/pattern/none FINISH ME!!
}

/*! If num is 0, turn of dashes. If num is 1, use on/off same length.
 * Segments are capped with current path cap style.
 */
void DisplayerCairo::Dashes(double *dashes, int num, double offset)
{
	if (!cr) return;
	cairo_set_dash(cr, dashes,num,offset);
}

/*! Clears to bgcolor with 0 opacity.
 */
void DisplayerCairo::ClearTransparent()
{
	cairo_save(cr);
	cairo_identity_matrix(cr);
	cairo_operator_t oldmode=cairo_get_operator(cr);
	cairo_set_operator(cr,CAIRO_OPERATOR_SOURCE);

	if (xw) cairo_set_source_rgba(cr,
					((xw->win_themestyle->bg.Pixel()&0xff0000)>>16)/255.,
					((xw->win_themestyle->bg.Pixel()&0xff00)>>8)/255.,
					 (xw->win_themestyle->bg.Pixel()&0xff)/255.,
					0.);
	else cairo_set_source_rgba(cr, bgRed, bgGreen, bgBlue, 0.0);

	cairo_rectangle(cr, Minx,Miny,Maxx-Minx+1,Maxy-Miny+1);
	cairo_fill(cr);
	cairo_set_source_rgba(cr, fgRed, fgGreen, fgBlue, fgAlpha);
	cairo_set_operator(cr,oldmode);
	cairo_restore(cr);
}

//! Clear the window to bgcolor between Min* and Max*. 
void DisplayerCairo::ClearWindow()
{
	//DBG cerr <<"--displayer ClearWindow:MinMaxx,:["<<Minx<<","<<Maxx
	//DBG		<<"] MinMaxy:["<<Miny<<","<<Maxy<<"] x0,y0:"<<ctm[4]<<","<<ctm[5]<<endl;

	cairo_save(cr);
	cairo_identity_matrix(cr);
	cairo_operator_t oldmode=cairo_get_operator(cr);
	cairo_set_operator(cr,CAIRO_OPERATOR_OVER);

	if (xw) cairo_set_source_rgba(cr,
					((xw->win_themestyle->bg.Pixel()&0xff0000)>>16)/255.,
					((xw->win_themestyle->bg.Pixel()&0xff00)>>8)/255.,
					 (xw->win_themestyle->bg.Pixel()&0xff)/255.,
					1.);
	else cairo_set_source_rgba(cr, bgRed, bgGreen, bgBlue, 1.0);

	cairo_rectangle(cr, Minx,Miny,Maxx-Minx+1,Maxy-Miny+1);
	cairo_fill(cr);
	cairo_set_source_rgba(cr, fgRed, fgGreen, fgBlue, fgAlpha);
	cairo_set_operator(cr,oldmode);
	cairo_restore(cr);
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

/*! Clip using current path (consumes path).
 */
int DisplayerCairo::Clip(bool append)
{
	if (!append) cairo_reset_clip(cr);
	cairo_clip(cr); //clears its current path

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
	if (mask) { cairo_surface_destroy(mask); mask=nullptr; }
	if (mask_pattern) { cairo_pattern_destroy(mask_pattern); mask_pattern=nullptr; }
	cairo_reset_clip(cr);
}

//! Return whether there is an active mask.
int DisplayerCairo::activeMask()
{
	return mask!=nullptr || mask_pattern!=nullptr;
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
	else if (mask_pattern) cairo_mask(cr,mask_pattern);
	else if (current_alpha >= 0) cairo_paint_with_alpha(cr, current_alpha);
	else cairo_paint(cr);
}


//------------------source patterns

bool DisplayerCairo::Capability(DisplayerFeature what)
{
	if (what==DRAWS_LinearGradient ||
		what==DRAWS_RadialGradient ||
		what==DRAWS_MeshGradient) return true;
	
	return false;
}

/*! Set the foreground "color" to be a linear gradient.
 * See LaxSpreadStyle for values of extend.
 * extend is 0..3 for none, repeat, reflect, pad.
 */
void DisplayerCairo::setLinearGradient(int extend, double x1,double y1, double x2,double y2, double *offsets, ScreenColor *colors, int n)
{
	if (!cr) return;

	//set up a cairo_pattern_t to use as drawing source
	// *** see cairo_pattern_set_matrix() 

//	if (real_coordinates) {
//		flatpoint p;
//		p=realtoscreen(x1,y1);
//		x1=p.x;
//		y1=p.y;
//		p=realtoscreen(x2,y2);
//		x2=p.x;
//		y2=p.y;
//	}

	cairo_pattern_t *gradient=cairo_pattern_create_linear(x1,y1, x2,y2);
	//cairo_pattern_t *gradient=cairo_pattern_create_linear(0,0, 300,300);


	 // can be CAIRO_EXTEND_NONE, CAIRO_EXTEND_REPEAT, CAIRO_EXTEND_REFLECT, or CAIRO_EXTEND_PAD
	 // pad is default for gradients, none default for surfaces
	if      (extend == LAXSPREAD_None)    cairo_pattern_set_extend(gradient, CAIRO_EXTEND_NONE);
	else if (extend == LAXSPREAD_Repeat)  cairo_pattern_set_extend(gradient, CAIRO_EXTEND_REPEAT);
	else if (extend == LAXSPREAD_Reflect) cairo_pattern_set_extend(gradient, CAIRO_EXTEND_REFLECT);
	else if (extend == LAXSPREAD_Pad)     cairo_pattern_set_extend(gradient, CAIRO_EXTEND_PAD);

	double r,g,b,a;
	for (int c=0; c<n; c++) {
		r=colors[c].red  /65535.;
		g=colors[c].green/65535.;
		b=colors[c].blue /65535.;
		a=colors[c].alpha/65535.;
		cairo_pattern_add_color_stop_rgba(gradient, offsets[c], r,g,b,a); //offset in [0..1]
	} 
//	for (int c=0; c<colors->colors.n; c++) {
//		r=colors->colors.e[c]->screen.Red();
//		g=colors->colors.e[c]->screen.Green();
//		b=colors->colors.e[c]->screen.Blue();
//		a=colors->colors.e[c]->screen.Alpha();
//		cairo_pattern_add_color_stop_rgba(gradient, colors->colors.e[c]->nt, r,g,b,a); //offset in [0..1]
//	}

	cairo_set_source(cr, gradient);
	cairo_pattern_destroy(gradient);
}

/*! Set the foreground "color" to be a radial gradient.
 * extend is 0..3 for none, repeat, reflect, pad.
 */
//void DisplayerCairo::setRadialGradient(int extend, double x1,double y1, double r1, double x2,double y2, double r2, GradientStrip *colors)
void DisplayerCairo::setRadialGradient(int extend, double x1,double y1, double r1, double x2,double y2, double r2, double *offsets, ScreenColor *colors, int n)
{
	if (!cr) return;

	//set up a cairo_pattern_t to use as drawing source
	// *** see cairo_pattern_set_matrix() 

	cairo_pattern_t *gradient=cairo_pattern_create_radial(x1,y1,r1, x2,y2,r2); 

	 // can be CAIRO_EXTEND_NONE, CAIRO_EXTEND_REPEAT, CAIRO_EXTEND_REFLECT, or CAIRO_EXTEND_PAD
	 // pad is default for gradients, none default for surfaces
	if      (extend == LAXSPREAD_None)    cairo_pattern_set_extend(gradient, CAIRO_EXTEND_NONE);
	else if (extend == LAXSPREAD_Repeat)  cairo_pattern_set_extend(gradient, CAIRO_EXTEND_REPEAT);
	else if (extend == LAXSPREAD_Reflect) cairo_pattern_set_extend(gradient, CAIRO_EXTEND_REFLECT);
	else if (extend == LAXSPREAD_Pad)     cairo_pattern_set_extend(gradient, CAIRO_EXTEND_PAD);

	double r,g,b,a;
	for (int c=0; c<n; c++) {
		r=colors[c].red  /65535.;
		g=colors[c].green/65535.;
		b=colors[c].blue /65535.;
		a=colors[c].alpha/65535.;
		cairo_pattern_add_color_stop_rgba(gradient, offsets[c], r,g,b,a); //offset in [0..1]
	} 
//	for (int c=0; c<colors->colors.n; c++) {
//		r=colors->colors.e[c]->screen.Red();
//		g=colors->colors.e[c]->screen.Green();
//		b=colors->colors.e[c]->screen.Blue();
//		a=colors->colors.e[c]->screen.Alpha();
//		cairo_pattern_add_color_stop_rgba(gradient, colors->colors.e[c]->nt, r,g,b,a); //offset in [0..1]
//	}

	cairo_set_source(cr, gradient);
	cairo_pattern_destroy(gradient);
}

/*! Make a mesh as current color.
 *
 * numrows and numcolumns refer to the number of mesh squares, not points.
 * There should be (numrows*3+1)*(numcolumns*3+1) points in the points array,
 * and (numrows+1)*(numcolumns+1) in colors.
 */
void DisplayerCairo::setMesh(int numrows, int numcolumns, flatpoint *points, ScreenColor *colors)
{
	if (!cr) return;

	cairo_pattern_t *mesh=cairo_pattern_create_mesh();
	int xs=numcolumns*3+1;

	for (int r=0; r<numrows; r++) {
		for (int c=0; c<numcolumns; c++) {

			//     C1     Side 1       C2
			//       +---------------+
			//       |               |
			//       |  P1       P2  |
			//       |               |
			//Side 0 |               | Side 2
			//       |               |
			//       |               |
			//       |  P0       P3  |
			//       |               |
			//       +---------------+
			//     C0     Side 3        C3



			 //any number of begin/end pairs
			cairo_mesh_pattern_begin_patch(mesh);

			int i=c*3 + r*3*xs;
			cairo_mesh_pattern_move_to(mesh,  points[i   + 3*xs].x,points[i   + 3*xs].y);

			cairo_mesh_pattern_curve_to(mesh, points[i   + 2*xs].x,points[i   + 2*xs].y,
											  points[i   +   xs].x,points[i   +   xs].y,
											  points[i         ].x,points[i         ].y
					);
			cairo_mesh_pattern_curve_to(mesh, points[i+1       ].x,points[i+1       ].y,
											  points[i+2       ].x,points[i+2       ].y,
											  points[i+3       ].x,points[i+3       ].y
					);                                                             
			cairo_mesh_pattern_curve_to(mesh, points[i+3 +   xs].x,points[i+3 +   xs].y,
											  points[i+3 + 2*xs].x,points[i+3 + 2*xs].y,
											  points[i+3 + 3*xs].x,points[i+3 + 3*xs].y
					);                                                             
			cairo_mesh_pattern_curve_to(mesh, points[i+2 + 3*xs].x,points[i+2 + 3*xs].y,
											  points[i+1 + 3*xs].x,points[i+1 + 3*xs].y,
											  points[i   + 3*xs].x,points[i   + 3*xs].y
					);                                                             

			cairo_mesh_pattern_set_control_point(mesh, 0, points[i+1 + 2*xs].x, points[i+1 + 2*xs].y);
			cairo_mesh_pattern_set_control_point(mesh, 1, points[i+1 +   xs].x, points[i+1 +   xs].y);
			cairo_mesh_pattern_set_control_point(mesh, 2, points[i+2 +   xs].x, points[i+2 +   xs].y);
			cairo_mesh_pattern_set_control_point(mesh, 3, points[i+2 + 2*xs].x, points[i+2 + 2*xs].y);

			int cxs=numcolumns+1;
			cairo_mesh_pattern_set_corner_color_rgba(mesh, 0, colors[c + (r+1)*cxs].Red(),
															  colors[c + (r+1)*cxs].Green(),
															  colors[c + (r+1)*cxs].Blue(),
															  colors[c + (r+1)*cxs].Alpha()
															  );
			cairo_mesh_pattern_set_corner_color_rgba(mesh, 1, colors[c + r*cxs].Red(),
															  colors[c + r*cxs].Green(),
															  colors[c + r*cxs].Blue(),
															  colors[c + r*cxs].Alpha()
															  );
			cairo_mesh_pattern_set_corner_color_rgba(mesh, 2, colors[c+1 + r*cxs].Red(),
															  colors[c+1 + r*cxs].Green(),
															  colors[c+1 + r*cxs].Blue(),
															  colors[c+1 + r*cxs].Alpha()
															  );
			cairo_mesh_pattern_set_corner_color_rgba(mesh, 3, colors[c+1 + (r+1)*cxs].Red(),
															  colors[c+1 + (r+1)*cxs].Green(),
															  colors[c+1 + (r+1)*cxs].Blue(),
															  colors[c+1 + (r+1)*cxs].Alpha()
															  );

			cairo_mesh_pattern_end_patch(mesh); 
		}
	}

	cairo_set_source(cr, mesh);
	cairo_pattern_destroy(mesh);
}


//------------------path functions

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
	// DBG cerr << "moveto "<<p.x<<", "<<p.y<<endl;
	cairo_move_to(cr,p.x,p.y);
}

void DisplayerCairo::lineto(flatpoint p)
{
	// DBG cerr << "lineto "<<p.x<<", "<<p.y<<endl;
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
	int oldreal=real_coordinates;
	double oldwidth=cairo_get_line_width(cr);
	
	if (real_coordinates) {
		cairo_set_line_width(cr,oldwidth*Getmag());
	}

	DrawScreen();
	drawellipse(p,radius,radius,0,2*M_PI, tofill);
	if (oldreal) {
		DrawReal();
		cairo_set_line_width(cr,oldwidth);
	}
}

//! Draw a polygon, optionally fill.
/*! If fill==1 then fill with FG and have no border. If fill==2,
 * then fill with BG and border wich FG.
 */
void DisplayerCairo::drawlines(flatpoint *points,int npoints,char ifclosed,char tofill)
{
	if (!npoints) return;

	if (!cairo_has_current_point(cr)) cairo_move_to(cr, points[0].x,points[0].y);

	for (int c=0; c<npoints; c++) {
		cairo_line_to(cr, points[c].x,points[c].y);
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


int DisplayerCairo::textheight()
{
	if (!curfont) initFont();

	return _textheight;
}

int DisplayerCairo::font(LaxFont *nfont, double size)
{
	LaxFontCairo *cairofont=dynamic_cast<LaxFontCairo*>(nfont);
	if (!cairofont) return 1;

	if (size<0) size=nfont->textheight();

	//DBG cerr <<" font(LaxFont), count: "<<cairo_font_face_get_reference_count(cairofont->font) <<endl;

	//if (curfont!=cairofont->font) {
	if (laxfont != cairofont) {
		cairofont->inc_count();
		if (laxfont) laxfont->dec_count();
		laxfont=cairofont;

		if (curfont) cairo_font_face_destroy(curfont);//really just a melodramatic dec count
		curfont=cairofont->font;
		cairo_font_face_reference(curfont);
	}

	if (curscaledfont) { cairo_scaled_font_destroy(curscaledfont); curscaledfont=nullptr; }

	if (cr) {
		cairo_set_font_face(cr, curfont);
		if (cairofont->options) cairo_set_font_options(cr, cairofont->options);
		//DBG cerr <<" font(LaxFont), cairo status set font face:  "<<cairo_status_to_string(cairo_status(cr)) <<endl;
	}

	fontsize(size);
	//DBG cerr <<" (eo)font(LaxFont), count: "<<cairo_font_face_get_reference_count(cairofont->font) <<endl;
	return 0;
}

/*! Return 0 for success, 1 for fail.
 */
int DisplayerCairo::font(const char *fontconfigpattern)
{
	if (!fontconfigpattern) return 1;

	FontManager *fontmanager=GetDefaultFontManager();
	LaxFont *nfont=fontmanager->MakeFontFromStr(fontconfigpattern, -1);
	int ret=font(nfont, nfont->textheight());
	nfont->dec_count();

	return ret;
}

int DisplayerCairo::font(const char *family,const char *style,double pixelsize)
{
	FontManager *fontmanager=GetDefaultFontManager();
	LaxFont *nfont=fontmanager->MakeFont(family, style, pixelsize, -1);
	int ret=font(nfont, pixelsize);
	nfont->dec_count();

	return ret;
}

int DisplayerCairo::fontsize(double size)
{
	DBG cerr <<"---fontsize: "<<size<<endl;

	int tempcr=0;
	if (!cr) {
		 //use ref_surface for reference
		if (!surface && !ref_surface) {
			ref_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 10,10);
		}

		cr = cairo_create(surface ? surface : ref_surface);
		if (!curfont) initFont();

		//DBG cerr <<" fontsize, temp cr, status before set font face:  "<<cairo_status_to_string(cairo_status(cr)) <<endl;
		cairo_set_font_face(cr,curfont);
		//DBG cerr <<" fontsize, temp cr, cairo status set font face:  "<<cairo_status_to_string(cairo_status(cr)) <<endl;
		tempcr=1;

		cairo_matrix_t m;
		if (real_coordinates) cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
		else cairo_matrix_init(&m, 1,0,0,1,0,0);
		cairo_set_matrix(cr, &m);
	}

	 //need to do some double checking, since font extents height is NOT the same as size
	 //as set in cairo_set_font_size(). That size is the size of the M square.
	cairo_set_font_size(cr, size);
	//DBG cerr <<" fontsize, cairo status font size:  "<<cairo_status_to_string(cairo_status(cr)) <<endl;

	cairo_font_extents_t fextents;
	cairo_font_extents(cr, &fextents);

	//DBG cerr <<" fontsize, cairo status font extent:  "<<cairo_status_to_string(cairo_status(cr)) <<endl;

	height_over_M=fextents.height/size;

	cairo_set_font_size(cr, size/height_over_M);
	cairo_font_extents(cr, &curfont_extents);

	//DBG cerr <<" fontsize, cairo status font extent resized:  "<<cairo_status_to_string(cairo_status(cr)) <<endl;

	_textheight=size;

	if (tempcr) { cairo_destroy(cr); cr=nullptr; }

	//DBG cerr <<"---fontsize end"<<endl;
	return 0;
}

/*! str should be utf8, and len should be number of bytes into str.
 *
 * Note that height might not equal ascent+descent. There might be an additional line spacing defined by the font.
 * Also, cairo_set_font_size() sets the M square height NOT the font height.
 */
double DisplayerCairo::textextent(LaxFont *thisfont, const char *str,int len, double *width,double *height,double *ascent,double *descent,char real)
{ 
	//DBG cerr <<"-------cairo textextent-------"<<endl;

	LaxFont *oldfont=nullptr;
	//double oldheight=0;

	LaxFontCairo *cfont=dynamic_cast<LaxFontCairo*>(thisfont);

	if (!curfont) initFont();

	if (len<0) len=(str ? strlen(str) : 0);
	if (str==nullptr || len==0 || (!curfont && !cfont)) {
		if (width) *width=0;
		if (height) *height=0;
		if (ascent) *ascent=0;
		if (descent) *descent=0;
		//DBG cerr <<"-------cairo textextent (0)-------"<<endl;
		return 0;
	}

	//DBG cerr <<" font curfont start: "<<cairo_font_face_get_reference_count(curfont) <<endl;
	//DBG if (cfont) cerr <<" temp font count start: "<<cairo_font_face_get_reference_count(cfont->font) <<endl;

	if (cfont && laxfont!=thisfont) {
		oldfont=laxfont;
		oldfont->inc_count();
		//oldheight=_textheight;
		font(cfont,cfont->textheight()); //dec count old, inc new font if the cairo font not same as new one
		//font(thisfont,thisfont->textheight()); 
	}
	//DBG cerr <<" font curfont 2: "<<cairo_font_face_get_reference_count(oldfont ? oldfont : curfont) <<endl;


	if (len > tbufferlen) reallocBuffer(len);

	int tempcr=0;
	if (!cr) {
		 //use ref_surface for reference
		if (!surface && !ref_surface) {
			ref_surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 10,10);
		}

		cr = cairo_create(surface ? surface : ref_surface);
		cairo_matrix_t m;
		if (real_coordinates) cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
		else cairo_matrix_init(&m, 1,0,0,1,0,0);
		cairo_set_matrix(cr, &m);

		cairo_set_font_face(cr,curfont);
		cairo_set_font_size(cr, _textheight/height_over_M);
		tempcr=1;
	}

	cairo_text_extents_t extents;
	memcpy(tbuffer,str,len);
	tbuffer[len]='\0';
	cairo_text_extents(cr, tbuffer, &extents);

	cairo_font_extents_t fextents;
	cairo_font_extents(cr, &fextents);

	if (ascent)  *ascent =fextents.ascent;
	if (descent) *descent=fextents.descent;
	if (height)  { if (real) *height=extents.height; else *height=fextents.height; }
	if (width)   { if (real) *width =extents.width;  else *width=extents.x_advance; }

	if (tempcr) { cairo_destroy(cr); cr=nullptr; }

	//DBG if (oldfont) cerr <<" curfont count: "<<cairo_font_face_get_reference_count(oldfont) <<endl;
	//DBG if (cfont) cerr <<" temp font: "<<cairo_font_face_get_reference_count(cfont->font) <<endl;

	if (oldfont) {
		 //reinstall the old font
		font(oldfont,oldfont->textheight()); //dec count old, inc new font if the cairo font not same as new one
		oldfont->dec_count();
	}

	//DBG if (cfont) cerr <<" temp font count end: "<<cairo_font_face_get_reference_count(cfont->font) <<endl;
	//DBG cerr <<" font curfont end: "<<cairo_font_face_get_reference_count(curfont) <<endl;
	//DBG cerr <<" found extent: adv="<<extents.x_advance<<"  width="<<extents.width<<endl;
	//DBG cerr <<"-------end cairo textextent-------"<<endl;

	if (real) return extents.width;
	return extents.x_advance;
}

void DisplayerCairo::initFont()
{
	if (curfont) return;

	//LaxFont *def=anXApp::app->defaultlaxfont;
	//font("sans","normal",def->textheight());
	font(anXApp::app->defaultlaxfont, anXApp::app->defaultlaxfont->textheight());
}

/*! Reallocate text scratch buffer, only when necessary.
 */
int DisplayerCairo::reallocBuffer(int len)
{
	if (len < tbufferlen) return tbufferlen;

	if (tbuffer) delete[] tbuffer;
	tbufferlen = len+30;
	tbuffer = new char[tbufferlen];

	return tbufferlen;
}

//! Draw a single line of text at x,y.
/*! str is utf8.
 */
double DisplayerCairo::textout_line(double x,double y,const char *str,int len,unsigned long align)
{
	if (!str) return 0;
	if (len < 0) len = strlen(str);
	if (len == 0) return 0;
	if (len > tbufferlen) reallocBuffer(len);
	strncpy(tbuffer,str,len);
	tbuffer[len]='\0';

	if (!curfont) initFont();

	cairo_text_extents_t extents;
	cairo_text_extents(cr, tbuffer, &extents);

	double ox,oy;
	if (align & LAX_LEFT) ox = x;
	else if (align & LAX_RIGHT) ox = x-extents.width;
	else ox = x - extents.width/2; //center

	if (align & LAX_TOP) oy = y + curfont_extents.ascent;
	else if (align & LAX_BOTTOM) oy = y - (curfont_extents.height-curfont_extents.ascent);
	else if (align & LAX_BASELINE) oy = y;
	else oy = y - (curfont_extents.height)/2 + curfont_extents.ascent; //center


	cairo_move_to(cr, ox,oy);
	if (len == 0) return 0;

	if (laxfont->Layers() == 1) cairo_show_text(cr, tbuffer);
	else {
		 //layered color font...
		LaxFontCairo *f = laxfont;
		int l = 0;
		cairo_save(cr);
		Palette *fpalette=dynamic_cast<Palette*>(f->GetColor());
		if (!fpalette) fpalette=palette;

		while (f) {
			// *** set color!!! palette must be rgba currently
			if (fpalette) {
				if (l<fpalette->colors.n) {
					cairo_set_source_rgba(cr,
							fpalette->colors.e[l]->color->values[0],
							fpalette->colors.e[l]->color->values[1],
							fpalette->colors.e[l]->color->values[2],
							fpalette->colors.e[l]->color->values[3]);
				}
			}
			cairo_move_to(cr, ox,oy);
			cairo_set_font_face(cr,f->font);
			cairo_show_text(cr, tbuffer);
			f = dynamic_cast<LaxFontCairo*>(f->NextLayer());
			l++;
		}
		cairo_restore(cr);
	}
	cairo_fill(cr);

	return extents.x_advance;
}

/*! len must specify how many glyphs in glyphs.
 *
 * x,y will be added to the x,y in the glyphs, and those new coordinates will be drawn
 * from the origin of the current transform in cr.
 *
 * One of glyphs or glyphsp is assumed to be non null. If both nonnull, glyphs is used.
 *
 * glyphs and glyphsp both hold a list of GlyphPlace, but glpyhsp is just an array of pointers to GlyphPlace
 * instead of a simple array of GlyphPlace. They are lumped here instead of different functions
 * because I'm too lazy to duplicate code.
 */
double DisplayerCairo::glyphsout(double x,double y, GlyphPlace *glyphs,GlyphPlace **glyphsp, unsigned int numglyphs, unsigned long align)
{
	if (numglyphs<=0) return 0;

	if (numglyphs>numalloc_glyphs) {
		delete[] cairo_glyphs;
		cairo_glyphs = new cairo_glyph_t[numglyphs+10];
		numalloc_glyphs=numglyphs+10;
	}

	GlyphPlace *glyph;
	double current_x=0;
	double current_y=0;

	for (unsigned int i = 0; i < numglyphs; i++)
	{
		if (glyphs) glyph = &glyphs[i];
		else glyph = glyphsp[i];

		cairo_glyphs[i].index = glyph->index;
		cairo_glyphs[i].x =   current_x + glyph->x_offset;
		cairo_glyphs[i].y =   current_y + glyph->y_offset;

		current_x += glyph->x_advance;
		current_y += glyph->y_advance;
	}


	cairo_text_extents_t extents;
	cairo_glyph_extents(cr, cairo_glyphs, numglyphs, &extents);

	double ox,oy;
	if (align&LAX_LEFT) ox=x;
	else if (align&LAX_RIGHT) ox=x-extents.width;
	else ox=x-extents.width/2; //center

	if (align&LAX_TOP) oy=y+curfont_extents.ascent;
	else if (align&LAX_BOTTOM) oy=y-(curfont_extents.height-curfont_extents.ascent);
	else if (align&LAX_BASELINE) oy=y;
	else oy=y - (curfont_extents.height)/2 + curfont_extents.ascent; //center

	//cairo_move_to(cr, ox,oy); <- seems to have no effect on glyph placements
	for (unsigned int i = 0; i < numglyphs; i++)
	{
		cairo_glyphs[i].x += ox;
		cairo_glyphs[i].y += oy;
	}

	if (laxfont->Layers()==1) {
		cairo_show_glyphs(cr, cairo_glyphs, numglyphs);
	} else {
		 //layered color font...
		LaxFontCairo *f = laxfont;
		int l=0;
		cairo_save(cr);
		Palette *fpalette=dynamic_cast<Palette*>(f->GetColor());
		if (!fpalette) fpalette=palette;

		while (f) {
			// set color!!! palette must be rgba currently
			if (fpalette) {
				if (l<fpalette->colors.n) {
					cairo_set_source_rgba(cr,
							fpalette->colors.e[l]->color->values[0],
							fpalette->colors.e[l]->color->values[1],
							fpalette->colors.e[l]->color->values[2],
							fpalette->colors.e[l]->color->values[3]);
				}
			}
			//cairo_move_to(cr, ox,oy);
			cairo_set_font_face(cr,f->font);
			cairo_show_glyphs(cr, cairo_glyphs, numglyphs);
			f=dynamic_cast<LaxFontCairo*>(f->NextLayer());
			l++;
		}
		cairo_restore(cr);
	}

	return current_x;
}

/*! Returns advance value.
 * One of glyphs or glyphsp is assumed to be non null. If both nonnull, glyphs is used.
 */
double DisplayerCairo::glyphsextent(GlyphPlace *glyphs,GlyphPlace **glyphsp, unsigned int numglyphs, double *width,double *height, bool real)
{
	if (numglyphs<=0) return 0;

	if (numglyphs>numalloc_glyphs) {
		delete[] cairo_glyphs;
		cairo_glyphs = new cairo_glyph_t[numglyphs+10];
		numalloc_glyphs=numglyphs+10;
	}

	GlyphPlace *glyph;
	double current_x=0;
	double current_y=0;
	for (unsigned int i = 0; i < numglyphs; i++)
	{
		if (glyphs) glyph = &glyphs[i];
		else glyph = glyphsp[i];

		cairo_glyphs[i].index = glyph->index;
		cairo_glyphs[i].x =   current_x + glyph->x_offset;
		cairo_glyphs[i].y =   current_y + glyph->y_offset;

		current_x += glyph->x_advance;
		current_y += glyph->y_advance;
	}

	cairo_text_extents_t extents;
	cairo_glyph_extents(cr, cairo_glyphs, numglyphs, &extents);

	if (height)  { if (real) *height=extents.height; else *height=curfont_extents.height; }
	if (width)   { if (real) *width =extents.width;  else *width=extents.x_advance; }

	if (real) return extents.width;
	return extents.x_advance;
}

//! Draw possibly multiple lines of text at x,y according to align.
/*! str is utf8.
 */
double DisplayerCairo::textout(double x,double y,const char *str,int len,unsigned long align)
{
	if (!cr || !str) return 0;
	if (len < 0) len = strlen(str);
	if (!len) return 0;

	int n=0;
	const char *nl=str;
	do {
		nl = strchr(nl,'\n');
		if (nl-str >= len) break;
		if (nl) nl++;
		n++;
	} while (nl);

	if (n==1 || n == 0) {
		return textout_line(x,y, str,len, align);
	}

	const char *text = str;
	int ret = 0;
	int h = n*textheight();
	flatpoint p;

	int valign = align&(LAX_TOP|LAX_BOTTOM|LAX_VCENTER|LAX_BASELINE);
	align = align&(LAX_LEFT|LAX_HCENTER|LAX_RIGHT);
	if (valign == LAX_VCENTER) y-=h/2;
	else if (valign == LAX_BOTTOM) y-=h;

	int llen;
	do {
		nl = strchr(text, '\n');
		if (!nl) {
			llen = strlen(text);
			nl = text + llen;
		}
		llen = nl - text;
		if (llen > len) llen = len;

		ret = textout_line(x, y, text, llen, align | LAX_TOP);

		if (*nl && llen < len) { //there's more text
			y += textheight();
			text = nl + 1;
			len -= llen + 1;
			if (!*text) nl = text;
		} else len -= llen;
	} while (*nl && len > 0);
	return ret;
}

double DisplayerCairo::textout(double *matrix,double x,double y,const char *str,int len,unsigned long align)
{
	cairo_save(cr);

	cairo_matrix_t m;
	cairo_matrix_init(&m,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	cairo_set_font_matrix(cr,&m);
	double d=textout(x,y, str,len, align);

	cairo_restore(cr);
	return d;
}

double DisplayerCairo::textout(double angle,double x,double y,const char *str,int len,unsigned long align)
{ 
	double mm[6];
	transform_identity(mm);
	mm[4] -= x;
	mm[5] -= y;

	 //do a rotation
	double r[6], s[6];
	r[4] = r[5] = 0;
	r[0] = cos(angle);
	r[1] = -sin(angle);
	r[2] = sin(angle);
	r[3] = cos(angle);
	if (align & LAX_FLIP) {
		r[2] = -r[2];
		r[3] = -r[3];
	}
	transform_mult(s,mm,r);
	// transform_copy(mm,s);

	s[4] += x;
	s[5] += y;

	if (real_coordinates) transform_mult(mm, s,Getctm());
	else transform_copy(mm, s);

	cairo_save(cr);
	cairo_matrix_t m;
	cairo_matrix_init(&m,mm[0],mm[1],mm[2],mm[3],mm[4],mm[5]);
	cairo_set_matrix(cr,&m);
	//cairo_translate(cr,-x,-y); <- why doesn't this work!?!?!
	//cairo_rotate(cr,angle);
	//cairo_translate(cr,x,y);
	double d = textout(x,y,str,len,align);
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
	if (!t) return;
	
	cairo_save(cr);

	if (!real_coordinates || !defaultRighthanded()) {
		cairo_translate(cr,x,y);
	} else {
		cairo_translate(cr,x,y);
		cairo_scale(cr,1,-1);
		cairo_translate(cr,0,-img->h());
	}



	cairo_set_source_surface(cr, t, 0,0);
	if (mask) cairo_mask_surface(cr,mask,0,0);
	else if (mask_pattern) cairo_mask(cr,mask_pattern);
	else cairo_paint(cr);

	img->doneForNow();
	cairo_restore(cr);
}

int DisplayerCairo::imageout(LaxImage *image, double x,double y, double w,double h)
{
	if (!image) return -1; 
	if (image->imagetype()!=LAX_IMAGE_CAIRO) return -2; 
	LaxCairoImage *i=dynamic_cast<LaxCairoImage*>(image);
	if (!i) return -1;

	if (w==0 && h==0) { w=image->w(); h=image->h(); }
	if (w==0 && h==0) return 0;
	if (w==0) { w=h*image->w()/image->h(); }
	if (h==0) { h=w*image->h()/image->w(); }

	double sx=w/image->w();
	double sy=h/image->h();

	double m[6];
	transform_set(m, sx,0,0,sy, x,y);

	if (real_coordinates) {
		PushAndNewTransform(m);
		imageout(image,0,0);
		PopAxes();

	} else {
		cairo_save(cr);
		cairo_matrix_t cm;
		cairo_matrix_init(&cm,m[0],m[1],m[2],m[3],m[4],m[5]);
		cairo_set_matrix(cr,&cm);
		imageout(image,0,0);
		cairo_restore(cr);
	}

	return 0;
}

void DisplayerCairo::imageout(LaxImage *img,double *matrix)
{
	if (!img || img->imagetype()!=LAX_IMAGE_CAIRO) return; 

	cerr <<" *** need to properly implement  DisplayerCairo::imageout(img, matrix) for drawscreen!!"<<endl;
	PushAndNewTransform(matrix);
	imageout(img,0,0);
	PopAxes();
}

/*! Render such that the upper left corner is at (x,y) and upper right is at (ulx+urx,uly+ury).
 * The height is adjusted to preserve aspect.
 */
void DisplayerCairo::imageout_rotated(LaxImage *img,double x,double y,double ulx,double uly)
{
	//if (!img || img->imagetype()!=LAX_IMAGE_CAIRO) return; 
	//LaxCairoImage *i=dynamic_cast<LaxCairoImage*>(img);
	//if (!i) return;

	cerr <<" *** need to implement  DisplayerCairo::imageout_rotated()!!"<<endl;

	return imageout(img,x,y); // ***
}

void DisplayerCairo::imageout_skewed(LaxImage *img,double x,double y,double ulx,double uly,double urx,double ury)
{
	//if (!img || img->imagetype()!=LAX_IMAGE_CAIRO) return; 
	//LaxCairoImage *i=dynamic_cast<LaxCairoImage*>(img);
	//if (!i) return;

	cerr <<" *** need to implement  DisplayerCairo::imageout_skewed()!!"<<endl;

	return imageout(img,x,y); // ***
}

void DisplayerCairo::imageout(LaxImage *img,double angle, double x,double y)
{
	if (real_coordinates) {
		PushAxes();
		Rotate(angle,x,y);
		imageout(img,0,0);
		PopAxes();

	} else {
		cerr << " *** need to test drawscreen with DisplayerCairo::imageout"<<endl;
		cairo_save(cr);
		cairo_translate(cr,x,y);
		cairo_rotate(cr,angle);
		cairo_translate(cr,-x,-y);
		imageout(img,0,0);
		cairo_restore(cr);
	}
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
	return transform_point(ctm,flatpoint(x,y));
}

//! Convert screen point (x,y) to real coordinates.
flatpoint DisplayerCairo::screentoreal(int x,int y)
{
	return transform_point(ictm,flatpoint(x,y));
}

//! Convert screen point to real coordinates.
flatpoint DisplayerCairo::screentoreal(flatpoint p)
{
	return transform_point(ictm,p);
}

//! Return a pointer to the current transformation matrix.
const double *DisplayerCairo::Getctm()
{
	return ctm;
}

//! Return a pointer to the inverse of the current transformation matrix.
const double *DisplayerCairo::Getictm()
{
	transform_invert(ictm,ctm);
	return ictm;
}


/*! Stand out function to aid debug breaking at specific program states.
 */
//static void BUG_CATCHER(const char *mes, double dist)
//{
//	//DBG flatpoint oldp=transform_point(ctm, flatpoint(0,0));
//	//DBG flatpoint newp=transform_point(ctm, flatpoint(0,0));
//	//DBG double ctmdist=norm(newp-oldp);
//	//DBG if (ctmdist>200) BUG_CATCHER("ShiftScreen", ctmdist);
//
//	if (dist<1000) return;
//
//	cerr <<"===================================================="<<endl;
//	cerr <<"====BOOM!==== "<<dist<<":  "<<mes<<endl;
//	cerr <<"===================================================="<<endl;
//}


//! Move the viewable portion by dx,dy screen units.
void DisplayerCairo::ShiftScreen(double dx,double dy)
{
	//if (real_coordinates) {
		//dx/=Getmag();
		//dy/=Getmag();
	//}

	//DBG flatpoint oldp=transform_point(ctm, flatpoint(0,0));

	ctm[4]+=dx;
	ctm[5]+=dy;
	//ictm[4]-=dx;
	//ictm[5]-=dy;
	transform_invert(ictm,ctm);

	if (cr && real_coordinates) {
		//cairo_translate(cr,dx,dy);

		cairo_matrix_t m;
		if (real_coordinates) cairo_matrix_init(&m, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]);
		else cairo_matrix_init(&m, 1,0,0,1,0,0);
		cairo_set_matrix(cr, &m);
	}

	//DBG flatpoint newp=transform_point(ctm, flatpoint(0,0));
	//DBG double ctmdist=norm(newp-oldp);
	//DBG if (ctmdist>200) BUG_CATCHER("ShiftScreen", ctmdist);

	syncPanner();

	//DBG dump_transforms(cr, ctm);
}

bool CairoErrorCheck(cairo_t *cr, bool say_if_ok)
{
	if (cr && cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
		cerr << " *** WARNING!!! cairo in error status: "<<cairo_status_to_string(cairo_status(cr))<< endl;
		return true;
	} else if (say_if_ok) {
		cerr << " --- cairo no error" <<endl;
	}
	return false;
}

//! Set the ctm to these 6 numbers.
void DisplayerCairo::NewTransform(const double *d)
{
	//DBG flatpoint oldp=transform_point(ctm, flatpoint(0,0));

	//DBG cerr << __FILE__<<" NewTransform: ";
	//DBG dumpctm(d);
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
	//CairoErrorCheck(cr, true);

	transform_copy(ctm,d);
	transform_invert(ictm,ctm);

	//DBG flatpoint newp=transform_point(ctm, flatpoint(0,0));
	//DBG double ctmdist=norm(newp-oldp);
	//DBG if (ctmdist>200) BUG_CATCHER("NewTransform1", ctmdist);

	syncPanner();

	//DBG dump_transforms(cr, ctm);
}

//! Make the transform correspond to the values.
void DisplayerCairo::NewTransform(double a,double b,double c,double d,double x0,double y0)
{
	//DBG flatpoint oldp=transform_point(ctm, flatpoint(0,0));

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

	//DBG flatpoint newp=transform_point(ctm, flatpoint(0,0));
	//DBG double ctmdist=norm(newp-oldp);
	//DBG if (ctmdist>200) BUG_CATCHER("NewTransform2", ctmdist);

	syncPanner();

	//DBG dump_transforms(cr, ctm);
}

void DisplayerCairo::ResetTransform()
{
	double *m;
	while (axesstack.n) {
		m=axesstack.pop();
		delete[] m;
		if (cr) cairo_restore(cr);
	}

	if (cr) {
		cairo_matrix_t m;
		cairo_matrix_init(&m, 1, 0, 0, defaultRighthanded() ? -1 : 1, 0, 0);
		cairo_set_matrix(cr, &m);
	}

	transform_set(ctm, 1, 0, 0, (defaultRighthanded() ? -1 : 1), 0, 0); 
	transform_invert(ictm, ctm);
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

	if (cr) {
		cairo_restore(cr);
	}

	double *tctm=axesstack.pop();
	if (tctm) {
		transform_copy(ctm,tctm);
		delete[] tctm;
	}

	transform_invert(ictm,ctm); 
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
	int r=Displayer::DrawScreen();

	if (cr) {
		cairo_matrix_t m;
		cairo_matrix_init(&m, 1,0,0,1,0,0);
		cairo_set_matrix(cr, &m);
	}

	return r;
}


} // namespace Laxkit

#endif //cairo



