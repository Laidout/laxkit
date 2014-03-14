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
#ifndef _LAX_FONTMANAGER_CAIRO_H
#define _LAX_FONTMANAGER_CAIRO_H


#include <lax/configured.h>

#ifdef LAX_USES_CAIRO


#include <lax/lists.h>
#include <lax/refptrstack.h>
#include <lax/fontmanager.h>

#include <cairo/cairo-xlib.h>


namespace Laxkit {


//---------------------------- LaxFontCairo -------------------------------
class LaxFontCairo : public LaxFont
{
  protected:
  public:
	cairo_font_extents_t extents;
	cairo_font_face_t *font;
	cairo_scaled_font_t *scaledfont;
	cairo_font_options_t *options;

	LaxFontCairo();
	LaxFontCairo(const char *fontconfigstr,int nid);
	LaxFontCairo(const char *family, const char *style, double size, int nid);
	virtual ~LaxFontCairo();

	virtual double charwidth(unsigned long chr,int real,double *width=NULL,double *height=NULL);
	virtual double contextcharwidth(char *start,char *pos,int real,double *width=NULL,double *height=NULL);
	virtual double textheight();
	virtual double ascent();
	virtual double descent();
	virtual double Resize(double newsize);
};


//---------------------------- FontManager -------------------------------
class FontManagerCairo : public FontManager, protected RefPtrStack<LaxFont>
{
 public:
	FontManagerCairo();
	virtual ~FontManagerCairo() {}

	virtual LaxFont *MakeFontFromFile(const char *file, double size, int nid);
	virtual LaxFont *MakeFontFromStr(const char *fcstr, int nid);
	virtual LaxFont *MakeFont(const char *family, const char *style, double size, int nid);
	virtual LaxFont *Add(LaxFont *font,int nid);
	virtual LaxFont *CheckOut(int id);
};


//--------------------------- FontManagerCairo ------------------------------------------
FontManager *newFontManager_cairo();



} //namespace Laxkit

#endif //uses cairo
#endif

