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
//    Copyright (C) 2009-present by Tom Lechner
//
#ifndef _LAX_COLOREVENTSS_H
#define _LAX_COLOREVENTSS_H


#include <lax/colors.h>
#include <lax/events.h>


namespace Laxkit {


//------------------------------- SimpleColorEventData ------------------------------
class SimpleColorEventData : public EventData
{
 public:
	int id;
	int colorindex;
	int colorsystem; //one of BasicColorSystems
	int colorspecial; //one of SimpleColorId
	int colormode; //0 for single color, or COLOR_FGBG, or COLOR_StrokeFill
	int max;
	int numchannels;
	int *channels;

	SimpleColorEventData();
	SimpleColorEventData(int nmax, int gray,int a, int nid);
	SimpleColorEventData(int nmax, int r,   int g, int b, int a, int nid);
	SimpleColorEventData(int nmax, int c,   int m, int y, int k, int a,  int nid);
	virtual ~SimpleColorEventData();
	virtual double Valuef(int i) const;
};


//------------------------------- ColorEventData ------------------------------

class ColorEventData : public EventData
{
 public:
	Color *color;
	int id;
	int info, info2;
	int colorindex;
	int colormode; //0 for single color, or COLOR_FGBG, or COLOR_StrokeFill

	ColorEventData();
	ColorEventData(Color *ncolor,int absorbcount, int nid, int ninfo, int ninfo2);
	virtual ~ColorEventData();
};


} // namespace Laxkit

#endif
