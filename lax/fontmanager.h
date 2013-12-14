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
//    Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef _LAX_FONTMANAGER_H
#define _LAX_FONTMANAGER_H

#include <cstdlib>
#include <lax/anobject.h>

namespace Laxkit {
	
//---------------------------- LaxFont -------------------------------
class LaxFont : public anObject
{
 protected:
 public:
	int id;
	unsigned long textstyle;
	char cntlchar;

	LaxFont();
	LaxFont(const char *fontconfigstr,int nid);
	LaxFont(const char *family, const char *style, double size, int nid);
	virtual ~LaxFont() {}
	virtual int laxfid() { return id; }

	virtual double charwidth(unsigned long chr,int real,double *width=NULL,double *height=NULL) = 0;
	virtual double contextcharwidth(char *start,char *pos,int real,double *width=NULL,double *height=NULL) = 0;
	virtual double ascent() = 0;
	virtual double descent() = 0;
	virtual double textheight() = 0;
	virtual double Resize(double newsize) = 0;
};


//---------------------------- FontManager -------------------------------
class FontManager : public anObject
{
 public:
	FontManager();
	virtual ~FontManager() {}

	virtual LaxFont *MakeFontFromFile(const char *file, double size, int nid) = 0;
	virtual LaxFont *MakeFontFromStr(const char *fcstr, int nid) = 0;
	virtual LaxFont *MakeFont(const char *family, const char *style, double size, int nid) = 0;
	virtual LaxFont *Add(LaxFont *font,int nid) = 0;
	virtual LaxFont *CheckOut(int id) = 0;
};


//--------------------------------------- Default Renderer Stuff ---------------------------

typedef FontManager *(*NewFontManagerFunc)();
int SetNewFontManagerFunc(const char *backend);

int SetDefaultFontManager(FontManager *manager);
FontManager *GetDefaultFontManager();



} // namespace Laxkit

#endif

