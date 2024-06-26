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
	double height_over_M;
	void UpdateVariations();
	void RebuildHBFont();

  public:
	cairo_font_extents_t extents;
	cairo_font_face_t *font;
	cairo_scaled_font_t *scaledfont;
	cairo_font_options_t *options;

	LaxFontCairo();
	LaxFontCairo(const char *fontconfigstr,int nid);
	LaxFontCairo(const char *nfamily, const char *nstyle, double size, int nid);
	virtual ~LaxFontCairo();

	virtual const char *PostscriptName();
	virtual int SetFromFile(const char *nfile, const char *nfamily, const char *nstyle, double size);

	virtual double charwidth(unsigned long chr,int real,double *width=NULL,double *height=NULL);
	virtual double contextcharwidth(char *start,char *pos,int real,double *width=NULL,double *height=NULL);
	virtual double Msize();
	virtual double textheight();
	virtual double ascent();
	virtual double descent();
	virtual double Extent(const char *str,int len);
	virtual double Resize(double newsize);

	// OpenType variations
	virtual const char *AxisName(int index) const;
	virtual int AxisIndex(const char *name) const;
	virtual bool SetAxis(int index, double value);
	virtual double GetAxis(int index) const;
	virtual bool SetFeature(const char *feature, bool active);
};


//---------------------------- FontManager -------------------------------
class FontManagerCairo : public FontManager, protected RefPtrStack<LaxFont>
{
  protected:
	cairo_t *ref_cr;
	cairo_surface_t *ref_surface;

  public:
	FontManagerCairo();
	virtual ~FontManagerCairo();

	virtual LaxFont *MakeFontFromFile(const char *file, const char *nfamily, const char *nstyle, double size, int nid);
	virtual LaxFont *MakeFontFromStr(const char *fcstr, int nid);
	virtual LaxFont *MakeFont(const char *family, const char *style, double size, int nid);
	virtual LaxFont *MakeFont(int nid);
	virtual LaxFont *Add(LaxFont *font,int nid);
	virtual LaxFont *CheckOut(int id);

	virtual cairo_t *ReferenceCairo();
};


//--------------------------- FontManagerCairo ------------------------------------------
FontManager *newFontManager_cairo();



} //namespace Laxkit

#endif //uses cairo
#endif

