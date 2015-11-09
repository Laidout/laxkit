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
#ifndef _LAX_FONTMANAGER_XLIB_H
#define _LAX_FONTMANAGER_XLIB_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <lax/lists.h>
#include <lax/refptrstack.h>
#include <lax/fontmanager.h>


namespace Laxkit {


//---------------------------- LaxFontXlib -------------------------------
class LaxFontXlib : public LaxFont
{
 protected:

 public:
	int numcharsinfont,firstchar;
	unsigned long textstyle;
	int *charwidths,*realcharwidths;
	char cntlchar;

	XftFont *font;

	LaxFontXlib();
	LaxFontXlib(XftFont *f,int nid);
	LaxFontXlib(Display *dpy,const char *xlfd,int nid);
	virtual ~LaxFontXlib();

	virtual int SetFromFile(const char *nfile, const char *nfamily, const char *nstyle, double size);

	virtual int SetupMetrics();
	virtual double charwidth(unsigned long chr,int real,double *width=NULL,double *height=NULL);
	virtual double contextcharwidth(char *start,char *pos,int real,double *width=NULL,double *height=NULL);
	virtual double textheight();
	virtual double ascent();
	virtual double descent();
	virtual double extent(const char *str,int len);
	virtual double Resize(double newsize);

	virtual void ResetFamily(const char *nfamily);
	virtual void ResetStyle(const char *nstyle);
};


//---------------------------- FontManager -------------------------------
class FontManagerXlib : public FontManager, protected RefPtrStack<LaxFont>
{
 public:
	FontManagerXlib();
	virtual ~FontManagerXlib() {}

	virtual LaxFont *Add(XftFont *xftfont,int nid);

	virtual LaxFont *MakeFontFromFile(const char *file, const char *nfamily, const char *nstyle, double size, int nid);
	virtual LaxFont *MakeFontFromStr(const char *fcstr, int nid);
	virtual LaxFont *MakeFont(const char *family, const char *style, double size, int nid);
	virtual LaxFont *Add(LaxFont *font,int nid);
	virtual LaxFont *CheckOut(int id);
};


//--------------------------- FontManagerCairo ------------------------------------------
FontManager *newFontManager_xlib();



} //namespace Laxkit

#endif

