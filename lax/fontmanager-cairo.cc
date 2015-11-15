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
#include <lax/strmanip.h>



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
	height_over_M=0;
	font=NULL;
	scaledfont=NULL;
	options=cairo_font_options_create();
	//cairo_matrix_t matrix;
	//cairo_matrix_init_identity(&matrix);
}

//! Constructor.
LaxFontCairo::LaxFontCairo(const char *fontconfigstr,int nid)
{
	font=NULL;
	scaledfont=NULL;
	options=cairo_font_options_create();

	FcPattern *pattern=FcNameParse((FcChar8*)fontconfigstr);

	DBG cerr <<"LaxFontCairo constructor: pattern from string: "<<endl;
	//DBG FcPatternPrint(pattern); *** use stdout, not stderr!!!

	font=cairo_ft_font_face_create_for_pattern(pattern);
	double height=1;
	FcPatternGetDouble(pattern, FC_SIZE, 0, &height);

	FcValue v;
	FcResult result;

	result=FcPatternGet(pattern,FC_FAMILY,0,&v);
	if (result==FcResultMatch) makestr(family, (const char *)v.u.s);

	result=FcPatternGet(pattern,FC_STYLE,0,&v);
	if (result==FcResultMatch) makestr(style, (const char *)v.u.s);

	result=FcPatternGet(pattern,FC_FILE,0,&v);
	if (result==FcResultMatch) makestr(fontfile, (const char *)v.u.s);

	FcPatternDestroy(pattern);

	cairo_matrix_t matrix;
	cairo_matrix_init_scale(&matrix, height, height);
	cairo_matrix_t identity;
	cairo_matrix_init_identity(&identity);
	options=cairo_font_options_create();
	scaledfont=cairo_scaled_font_create(font, &matrix, &identity, options);

	//const cairo_matrix_t *font_matrix,
    //const cairo_matrix_t *ctm,
    //const cairo_font_options_t *options);
	//cairo_font_options_set_antialias(&options, cairo_antialias_t);

	cairo_scaled_font_extents(scaledfont, &extents);

    height_over_M=extents.height/height; // *** note: unreliable!  

	if (nid>0) id=nid;
}

LaxFontCairo::LaxFontCairo(const char *nfamily, const char *nstyle, double size, int nid)
{
	DBG cerr <<"LaxFontCairo constructor family/style/size..."<<endl;

	id=nid;
	if (!id) id=getUniqueNumber();

	font=NULL;
	scaledfont=NULL;
	options=cairo_font_options_create();

	height_over_M=0; 
	SetFromFile(NULL, nfamily, nstyle, size);
}

LaxFontCairo::~LaxFontCairo()
{
	DBG cerr <<"LaxFontCairo destructor..."<<endl;

	if (scaledfont) cairo_scaled_font_destroy(scaledfont);
	if (font) cairo_font_face_destroy(font);
	if (options) cairo_font_options_destroy(options);
}

/*! Return 0 for success, nonzero for error.
 *
 * If nfile is the same as current file, nothing is done and 0 is returned, regardless of other fields.
 */
int LaxFontCairo::SetFromFile(const char *nfile, const char *nfamily, const char *nstyle, double size)
{
	if (fontfile && nfile && !strcmp(nfile,fontfile) && size==extents.height) { 
		return 0;
	}

	 //find new!
	FcPattern *pattern=FcPatternCreate();

	FcValue value;
	if (nfile) {
		value.type=FcTypeString; value.u.s=(FcChar8*)nfile;
		FcPatternAdd(pattern, FC_FILE, value, FcTrue);
	}

	if (nfamily) {
		value.type=FcTypeString; value.u.s=(FcChar8*)nfamily;
		FcPatternAdd(pattern, FC_FAMILY, value, FcTrue);
	}

	if (nstyle) {
		value.type=FcTypeString; value.u.s=(FcChar8*)nstyle;
		FcPatternAdd(pattern, FC_STYLE, value, FcTrue);
	}

	cairo_font_face_t *newfont=cairo_ft_font_face_create_for_pattern(pattern);
	FcPatternDestroy(pattern);


	if (cairo_font_face_status(newfont)!=CAIRO_STATUS_SUCCESS) {
		cairo_font_face_destroy(newfont);
		return 1;
	}

	makestr(fontfile, nfile);
	makestr(family, nfamily ? nfamily : NULL);// *** look up from fontconfig??
	makestr(style, nstyle ? nstyle : NULL); // ***


	 //delete old info if any
	if (scaledfont) cairo_scaled_font_destroy(scaledfont);
	if (font) cairo_font_face_destroy(font);
	if (options) cairo_font_options_destroy(options);

	font=newfont;

	cairo_matrix_t m, ctm;
	cairo_matrix_init_scale(&m,size,size);
	cairo_matrix_init_identity(&ctm);

	options=cairo_font_options_create();
	scaledfont=cairo_scaled_font_create(font, &m, &ctm, options);
	cairo_scaled_font_extents(scaledfont, &extents);

    height_over_M=extents.height/size;

	cairo_matrix_init_scale(&m,size/height_over_M,size/height_over_M);
	cairo_scaled_font_destroy(scaledfont);
	scaledfont=cairo_scaled_font_create(font, &m, &ctm, options);
    cairo_scaled_font_extents(scaledfont, &extents); 
	return 0;
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

double LaxFontCairo::extent(const char *str,int len)
{
	cerr << " *** need to implement LaxFontCairo::extent()"<<endl;
	return 0;
}

double LaxFontCairo::Resize(double newsize)
{
	if (scaledfont) { cairo_scaled_font_destroy(scaledfont); scaledfont=NULL; }
	if (!font) return 0;


	cairo_matrix_t m, ctm;
	cairo_matrix_init_identity(&ctm);
	if (height_over_M<=0) {
		cairo_matrix_init_scale(&m,newsize,newsize);
		scaledfont=cairo_scaled_font_create(font, &m, &ctm, options);
		cairo_scaled_font_extents(scaledfont, &extents); 

		height_over_M=extents.height/newsize;

		cairo_matrix_init_scale(&m,newsize/height_over_M,newsize/height_over_M);

		cairo_scaled_font_destroy(scaledfont);
	}

	cairo_matrix_init_scale(&m,newsize/height_over_M,newsize/height_over_M);
	if (!options) options=cairo_font_options_create();
	scaledfont=cairo_scaled_font_create(font, &m, &ctm, options);
	cairo_scaled_font_extents(scaledfont, &extents);

	if (nextlayer) nextlayer->Resize(newsize);

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
{
	ref_cr=NULL;
	ref_surface=NULL;
}

FontManagerCairo::~FontManagerCairo()
{
	if (ref_cr) cairo_destroy(ref_cr);
	if (ref_surface) cairo_surface_destroy(ref_surface);
}


//! Create and return a LaxFont, but do not store it within the fontmanager.
/*! str is a FontConfig string.
 * If nid<0 then the id is from getUniqueNumber().
 */
LaxFont *FontManagerCairo::MakeFontFromStr(const char *fcstr, int nid)
{
	if (nid<0) nid=getUniqueNumber();
	LaxFont *f=new LaxFontCairo(fcstr,nid);
	//push(f);//incs count of f to 2
	//f->dec_count();//remove creation count
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
LaxFont *FontManagerCairo::MakeFontFromFile(const char *file, const char *nfamily, const char *nstyle, double size, int nid)
{
	LaxFontCairo *newfont=new LaxFontCairo();
	if (newfont->SetFromFile(file, nfamily, nstyle, size)!=0) {
		delete newfont;
		return NULL;
	}

	return newfont;
}

//! Create and return a LaxFont, but do not store it within the fontmanager.
/*! This family, style, and size are passed along to fontconfig.
 * If fontconfig cannot do anything with it, NULL is returned.
 */
LaxFont *FontManagerCairo::MakeFont(const char *family, const char *style, double size, int nid)
{
	return new LaxFontCairo(family,style,size,nid);
}

cairo_t *FontManagerCairo::ReferenceCairo()
{
	if (ref_cr) return ref_cr;

	if (!ref_surface) ref_surface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1,1);
	ref_cr=cairo_create(ref_surface);

	return ref_cr;
}


} // namespace Laxkit

#endif


