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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_FILLSTYLE_H
#define _LAX_FILLSTYLE_H

#include <X11/Xlib.h>
#include <cstdio>
#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/colors.h>
#include <lax/drawingdefs.h>

#define FillNone 100

namespace LaxInterfaces {

class FillStyle : virtual public Laxkit::anObject, virtual public Laxkit::DumpUtility
{
  public:
	Laxkit::Color *color2;
	Laxkit::ScreenColor color;
	int fillrule;
	int fillstyle;
	int function;
	unsigned long mask;

	FillStyle();
	FillStyle(int r,int g,int b, int a,int fr,int fs,int f);
	FillStyle(const FillStyle &f);
	FillStyle &operator=(FillStyle &f);
	virtual ~FillStyle();
	virtual const char *whattype() { return "FillStyle"; }
	virtual anObject *duplicate(anObject *ref);

	virtual void Color(int r,int g,int b,int a);
	virtual void Colorf(double r,double g,double b,double a);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);

	virtual int hasFill();
	virtual int FillRule(int newrule);
};


} // namespace LaxInterfaces

#endif

