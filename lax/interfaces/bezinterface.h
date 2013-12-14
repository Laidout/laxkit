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
//    Copyright (C) 2004-2007,2011 by Tom Lechner
//
#ifndef _LAX_BEZINTERFACE_H
#define _LAX_BEZINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/vectors.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>

namespace LaxInterfaces {


#define BEZS_CLOSED         1
#define BEZS_STIFF_EQUAL    0
#define BEZS_STIFF_NEQUAL   1
#define BEZS_NSTIFF_EQUAL   2
#define BEZS_NSTIFF_NEQUAL  3

class BezData : public SomeData
{
  public:
	int style;
	int npoints; // is all points, not just vertices
	flatpoint *points;
	int *pointstyles; // length is n/3+1
	LineStyle linestyle;
	BezData(int cstyle=0);
	~BezData();
	flatpoint beztangent(int sv,int tov,double t); // useful for arrows when control points are too close to vertex
	flatpoint bezpoint(int s,double t);
	int AddPoint(int where,flatpoint c1,flatpoint v,flatpoint c2,int pointstyle);
	int DeletePoint(int index);
	void FindBBox();
	const char *whattype() { return "BezData"; }
};

class BezInterface : public anInterface
{
  protected:
	flatpoint tmppoint;
	int curpoint;
	Laxkit::NumStack<int> curpoints;
	int creationstyle;
	int curpointstyle;
	char showdecs;
	int mx,my,lx,ly;
	LineStyle linestyle;
  public:
	unsigned long linecolor,deconcolor,decoffcolor;
	BezData *data;
	BezInterface(int nid,Laxkit::Displayer *ndp);
	~BezInterface();
	int scan(int x,int y,int startat=0,int pref=-1); //startat=0  pref=-1;
	virtual int LBDown(int x,int y,unsigned int state,int count);
	virtual int LBUp(int x,int y,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state);
	virtual int CharRelease(unsigned int ch,unsigned int state);
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int mask=0);
	virtual void toggleclosed();
	virtual void deletedata();
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual const char *whattype() { return "BezInterface"; }
	virtual const char *whatdatatype() { return "BezData"; }
	virtual void Clear(SomeData *d) { if ((!d && data) || (d && d==data)) { data->dec_count(); data=NULL; } }
	virtual int DrawData(anObject *ndata,anObject *a1=NULL,anObject *a2=NULL,int info=0);
	int Refresh();
	void MakeCircle();
};

} // namespace LaxInterfaces

#endif

