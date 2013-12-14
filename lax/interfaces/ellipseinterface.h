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
//    Copyright (C) 2004-2006,2011 by Tom Lechner
//
#ifndef _LAX_ELLIPSEINTERFACE_H
#define _LAX_ELLIPSEINTERFACE_H

#define ELLIPSES_ISCIRCLE 1

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>

//using namespace std;

namespace LaxInterfaces {

class EllipseData : public SomeData
{
  protected:
  	
  public:
	unsigned int style;
	double s,e,a,b;
	flatpoint center,x,y;
	LineStyle linestyle;
	EllipseData() { center=flatpoint(0,0); x=flatpoint(1,0); y=flatpoint(0,1); a=b=0; style=0; s=0; e=2*3.14159265358979; }
	virtual void usefocus(flatpoint f1,flatpoint f2,double c=-1);
	const char *whattype() { return "EllipseData"; }
	void FindBBox();
};


//-----------------------------


class EllipseInterface : public anInterface
{
  protected:
	int mx,my;
	char dataislocal; //*** for creation, deletes easy?? waits to pass on (see Usethis)
	int curpoint; //0=none 1=f1 2=f2 3=t 4=r 5=b 6=l 7=wildpoint 8=start 9=end
	flatpoint createp,createx,createy;
  public:
	unsigned long controlcolor;
	int creationstyle,createfrompoint,createangle,showdecs; // cfp: 0 (nw), 1 y, 2 x, 3 xy
	EllipseData *data;
	RectInterface *rinterf;
	RectData rdata;
	LineStyle linestyle;
	EllipseInterface(int nid,Laxkit::Displayer *ndp);
	~EllipseInterface();
	virtual anInterface *duplicate(anInterface *dup);
	void deletedata();
	void rectify();
	void erectify();
	virtual int scan(int x,int y);
	virtual int LBDown(int x,int y,unsigned int state,int count);
	virtual int LBUp(int x,int y,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state);
	virtual int CharRelease(unsigned int ch,unsigned int state);
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,
			Laxkit::anObject *a2=NULL,int info=0);
	virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	const char *whattype() { return "EllipseInterface"; }
	const char *whatdatatype() { return "EllipseData"; }
	flatpoint getpoint(int c);
};

} // namespace LaxInterfaces

#endif

