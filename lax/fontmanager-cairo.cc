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
//    Copyright (C) 2013 by Tom Lechner
//

#include <lax/fontmanager-cairo.h>



#ifdef LAX_USES_CAIRO


#include <lax/anxapp.h>

#include <lax/lists.cc> // this is necessary to instantiate templates
#include <lax/refptrstack.cc>

#include <cstdio>
#include <cairo/cairo-ft.h>

#include <iostream>
#define DBG
using namespace std;

namespace Laxkit {



//--------------------------- FontManagerCairo ------------------------------------------
FontManager *newFontManager_cairo()
{ return new FontManagerCairo(); }


//---------------------------- LaxFontCairo -------------------------------

/*! \class LaxFontCairo
 * \brief A LaxFont using cairo.
 *
 * Fonts in cairo have a base font (cairo_font_face_t) and a font at a particular size or
 * transform (cairo_scaled_font_t), held here by font and scaledfont respectively.
 */
LaxFontCairo::LaxFontCairo()
{
	font=NULL;
	scaledfont=NULL;
	options=cairo_font_options_create();
	cairo_matrix_t matrix;
	cairo_matrix_init_identity(&matrix);
}

//! Constructor.
LaxFontCairo::LaxFontCairo(const char *fontconfigstr,int nid)
{
	//DisplayerCairo *dd=dynamic_cast<DisplayerCairo*>(GetDefaultDisplayer);
	//if (!dd) return NULL;

	FcPattern *pattern=FcNameParse((FcChar8*)fontconfigstr);
	DBG cerr <<"LaxFontCairo constructor pattern from string: "<<endl;
	DBG FcPatternPrint(pattern);

	font=cairo_ft_font_face_create_for_pattern(pattern);
	double height=1;
	FcPatternGetDouble(pattern, FC_SIZE, 0, &height);
	FcPatternDestroy(pattern);

	cairo_matrix_t matrix;
	cairo_matrix_init_scale(&matrix, height, height);
	options=cairo_font_options_create();
	scaledfont=cairo_scaled_font_create(font, &matrix, &matrix, options);

	//const cairo_matrix_t *font_matrix,
    //const cairo_matrix_t *ctm,
    //const cairo_font_options_t *options);
	//cairo_font_options_set_antialias(&options, cairo_antialias_t);

	cairo_scaled_font_extents(scaledfont, &extents);
}

LaxFontCairo::LaxFontCairo(const char *family, const char *style, double size, int nid)
{
	id=nid;
	if (!id) id=getUniqueNumber();

	//DisplayerCairo *dd=dynamic_cast<DisplayerCairo*>(GetDefaultDisplayer);
	//if (!dd) return NULL;

	FcPattern *pattern=FcPatternCreate();

	FcValue value;
	value.type=FcTypeString; value.u.s=(FcChar8*)family;
	FcPatternAdd(pattern, FC_FAMILY, value, FcTrue);

	value.type=FcTypeString; value.u.s=(FcChar8*)style;
	FcPatternAdd(pattern, FC_STYLE, value, FcTrue);

	value.type=FcTypeDouble; value.u.d=size;
	FcPatternAdd(pattern, FC_SIZE, value, size);

	font=cairo_ft_font_face_create_for_pattern(pattern);
	FcPatternDestroy(pattern);


	cairo_matrix_t m, ctm;
	cairo_matrix_init_scale(&m,size,size);
	cairo_matrix_init_identity(&m);
	options=cairo_font_options_create();
	scaledfont=cairo_scaled_font_create(font, &ctm, &ctm, options);
	cairo_scaled_font_extents(scaledfont, &extents);
}

LaxFontCairo::~LaxFontCairo()
{
	if (scaledfont) cairo_scaled_font_destroy(scaledfont);
	if (font) cairo_font_face_destroy(font);
	if (options) cairo_font_options_destroy(options);
}

double LaxFontCairo::textheight()
{
	return extents.height;
}

double LaxFontCairo::ascent()
{
	return extents.ascent;
}

double LaxFontCairo::descent()
{
	return extents.descent;
}

double LaxFontCairo::Resize(double newsize)
{
	if (scaledfont) { cairo_scaled_font_destroy(scaledfont); scaledfont=NULL; }
	if (!font) return 0;

	cairo_matrix_t m, ctm;
	cairo_matrix_init_scale(&m,newsize,newsize);
	cairo_matrix_init_identity(&ctm);
	if (!options) options=cairo_font_options_create();
	scaledfont=cairo_scaled_font_create(font, &m, &ctm, options);
	cairo_scaled_font_extents(scaledfont, &extents);

	return textheight();
}


//! Return the character width of ch.
/*! This assumes the charwidth array exists already.
 * If the real character width is 0, and r==0, this returns width of "\9f"
 * or some such. Of course, this function is for latin-1 only.
 *
 * \todo *** this is broken
 */
double LaxFontCairo::charwidth(unsigned long chr,int real,double *width,double *height)
{
	DBG cerr <<" font::charwidth don't use!!!"<<endl;
	return 0;
}


/*! Some languages change the character based on where letter is in word.
 * \todo This function as it stands is totally worthless and meaningless.
 */
double LaxFontCairo::contextcharwidth(char *start,char *pos,int real,double *width,double *height)
{ 
	DBG cerr <<" font::charwidth don't use!!!"<<endl;
	return charwidth((unsigned long)(*pos),real, width,height); 
}


//--------------------------- FontManagerCairo ------------------------------------------


/*! \class FontManagerCairo
 * \brief The font manager used by anXApp to simplify keeping track of what fonts are loaded.
 */

FontManagerCairo::FontManagerCairo()
	: RefPtrStack<LaxFont>()
{ }


//! Add a font to the manager, based on an X Logical Font Description string.
/*! If nid<0 then the id is from getUniqueNumber().
 */
LaxFont *FontManagerCairo::MakeFontFromStr(const char *fcstr, int nid)
{
	if (nid<0) nid=getUniqueNumber();
	LaxFont *f=new LaxFontCairo(fcstr,nid);
	push(f);//incs count of f to 2
	f->dec_count();//remove creation count
	return f;
}

//! Add an already created LaxFont to the manager.
/*! If nid<0 then the id is from getUniqueNumber().
 * 
 *
 * If the font is already on the stack, nothing is done.
 *
 * The font's count is incremented.
 * Returns the font object or NULL on error.
 */
LaxFont *FontManagerCairo::Add(LaxFont *font,int nid)
{
	if (!font) return NULL;
	if (nid<0) nid=getUniqueNumber();
	pushnodup(font); //incs count if not already on stack
	return font;
}


//! Add a count of 1 to the LaxFont with the given id, and return the corresponding font object.
/*! Returns the LaxFont object, or NULL if object not found.
 *
 * Please remember that when you are all done with the font, you must decrement its count.
 */
LaxFont *FontManagerCairo::CheckOut(int id)
{
	int c;
	for (c=0; c<n; c++) {
		if (e[c]->id==id) {
			e[c]->inc_count();
			return e[c];
		}
	}
	return NULL;
}

//! Create and return a LaxFont, but do not store it within the fontmanager.
/*! This file and size are passed along to fontconfig.
 * If fontconfig cannot do anything with it, NULL is returned.
 */
LaxFont *FontManagerCairo::MakeFontFromFile(const char *file, double size, int nid)
{
	cerr <<" *** must implement FontManagerCairo::MakeFontFromFile"<<endl;
	return NULL;
//	int screen=0;
//	if (size<=0) size=anXApp::app->defaultlaxfont->textheight();
//	XftFont *xfont=XftFontOpen(anXApp::app->dpy, screen,
//								XFT_FILE, XftTypeString, file,
//								XFT_PIXEL_SIZE, XftTypeDouble, size,
//								NULL); 
//	if (!xfont) return NULL;
//	LaxFont *laxfont=new LaxFontCairo(xfont,nid);
//
//	return laxfont;
}

//! Create and return a LaxFont, but do not store it within the fontmanager.
/*! This family, style, and size are passed along to fontconfig.
 * If fontconfig cannot do anything with it, NULL is returned.
 */
LaxFont *FontManagerCairo::MakeFont(const char *family, const char *style, double size, int nid)
{
	return new LaxFontCairo(family,style,size,nid);
}


} // namespace Laxkit

#endif


