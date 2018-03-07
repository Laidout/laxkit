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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_PALETTE_H
#define _LAX_PALETTE_H

#include <lax/dump.h>
#include <lax/anobject.h>
#include <lax/lists.h>

namespace Laxkit {


//-------------------------------- Palette/PaletteEntry -----------------------------

#define LAX_GIMP_PALETTE 1

class PaletteEntry
{
 public:
	int *channels;
	int numcolors;
	int color_space;
	int maxcolor;
	char *name;

	PaletteEntry(PaletteEntry *entry);
	PaletteEntry(const char *nname,int n,int *v,int space,int max=255);
	virtual ~PaletteEntry();
};

class Palette : public LaxFiles::DumpUtility, public anObject
{
 public:
	int defaultmaxcolor;
	int default_colorspace;
	char *filename;
	char *name;
	char is_read_in;
	bool readonly;
	int columns;
	PtrStack<PaletteEntry> colors;

	Palette();
	virtual ~Palette();

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext);
	virtual void dump_in (FILE *f,int indent,int what,LaxFiles::DumpContext *loadcontext,LaxFiles::Attribute **att);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext);
	virtual Palette *duplicate();
	virtual anObject *duplicate(anObject *ref) { return duplicate(); }

	virtual int AddRGB(const char *name, int r, int g, int b, int max);
	virtual int AddRGBA(const char *name, int r, int g, int b, int a, int max);
	virtual int AddCMYK(const char *name, int c, int m, int y, int k, int max);
	virtual int AddCMYKA(const char *name, int c, int m, int y, int k, int a, int max);
	virtual int AddGray(const char *name, int g,int max);
	virtual int AddGrayA(const char *name, int g,int a,int max);
};

Palette *rainbowPalette(int w,int h,int max,int include_gray_strip);
	


} //namespace Laxkit;

#endif

