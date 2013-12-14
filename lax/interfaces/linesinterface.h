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
#ifndef _LAX_LINESINTERFACE_H
#define _LAX_LINESINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/vectors.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>

#define LINESDATA_CLOSED   1
#define LINESDATA_LINES    2
#define LINESDATA_POLYGON  4
#define LINESDATA_SYMPOLY  8

 // most of these are for polygon creation, and where the corners point
#define LINESDATA_UP       16
#define LINESDATA_DOWN     32
#define LINESDATA_RIGHT    64
#define LINESDATA_LEFT     128
#define LINESDATA_PLUSHALF 256
#define LINESDATA_RECT     512
#define LINESDATA_MAXINBOX 1024

namespace LaxInterfaces {
 
class LinesData : public SomeData
{
//  protected:
  	
  public:
  	int numsides,numturns;
  	flatpoint *points;
	int npoints;
	unsigned int style;
	LineStyle linestyle;
	LinesData() { points=NULL; style=0; npoints=0; }
	~LinesData() { if (points) delete[] points; }
	const char *whattype() { return "LinesData"; }
	int AddAfter(int afterwhich,flatpoint p); //returns new curpoint
	int Delete(int which);
	void FindBBox();
};

//--------------------

class LinesInterface : public anInterface
{
  protected:
	int mx,my;
	int curpoint;
	int creating;
	flatpoint createp,createx,createy;
	double newa,newb;
	void Setupcreation(int sides,int turns);
	void movecontrol(int which,int x,int y);
	void movecontrol(int which,flatpoint d);
  public:
	unsigned long controlcolor;
	int creationstyle,creationsides,creationturns,creationgravity,showdecs;
	double creationangle;
	LinesData *data;
	LineStyle linestyle;
	LinesInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~LinesInterface() { deletedata(); }
	virtual void deletedata();
	virtual int scan(int x,int y);
	virtual int LBDown(int x,int y,unsigned int state,int count);
	virtual int LBUp(int x,int y,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state);
	virtual int CharRelease(unsigned int ch,unsigned int state);
	virtual int Refresh();
	virtual int UseThis(int id,int ndata);
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int mask=0);
	virtual int toggleclosed(int c=-1);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual const char *whattype() { return "LinesInterface"; }
	virtual const char *whatdatatype() { return "LinesData"; }
};

} // namespace LaxInterfaces

#endif

