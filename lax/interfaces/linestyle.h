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
//    Copyright (C) 2004-2007,2010-2012 by Tom Lechner
//
#ifndef _LAX_LINESTYLE_H
#define _LAX_LINESTYLE_H

#include <cstdio>

#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/screencolor.h>


namespace LaxInterfaces {

enum LineStyleMask {
	LINESTYLE_Width     =(1<<0),
	LINESTYLE_Color     =(1<<1),
	LINESTYLE_Dash      =(1<<2),
	LINESTYLE_Joinstyle =(1<<3),
	LINESTYLE_Miterlimit=(1<<4),
	LINESTYLE_Capstyle  =(1<<5),
	LINESTYLE_MAX
};

class LineStyle : public Laxkit::anObject, public LaxFiles::DumpUtility
{
  public:
  	double width;
	int widthtype;
	int capstyle, endcapstyle;
	int joinstyle;
	double miterlimit;
	int dotdash;
	int function;

	double *dashes;
	double dash_offset;
	int numdashes;

	Laxkit::ScreenColor color;

	unsigned long mask;

	LineStyle();
	LineStyle(int r,int g,int b, int a, double w,int cap,int join,int dot,int func);
	LineStyle(const LineStyle &l);
	LineStyle &operator=(LineStyle &l);
	virtual ~LineStyle();
	virtual void Color(int r,int g,int b,int a);
	virtual void Colorf(double r,double g,double b,double a);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);

	virtual int hasStroke();
};

} // namespace LaxInterfaces

#endif

