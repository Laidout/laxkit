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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_PALETTE_H
#define _LAX_PALETTE_H

#include <lax/dump.h>
#include <lax/anxapp.h>
#include <lax/anobject.h>
#include <lax/rectangles.h>
#include <lax/lists.h>
#include <lax/buttondowninfo.h>

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
	

//-------------------------------- PaletteWindow -----------------------------

#define PALW_DBCLK_TO_LOAD   (1<<16)
#define PALW_READONLY        (1<<17)


class PaletteWindow : public anXWindow
{
 protected:
	int xn,yn;
	double dx,dy;
	ButtonDownInfo buttondown;

 public:
	Palette *palette;
	int pad;
	int curcolor,ccolor;
	IntRectangle inrect;

	PaletteWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend);
	virtual ~PaletteWindow();
	virtual const char *whattype() { return "PaletteWindow"; }
	virtual int Event(const EventData *e,const char *mes);
	virtual int send();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual void Refresh();
	virtual void findInrect();
	virtual int findColorIndex(int x,int y);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);

	virtual const char *PaletteDir();
	virtual int LoadPalette(const char *file);
};

} //namespace Laxkit;

#endif

