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
//    Copyright (C) 2009 by Tom Lechner
//
#ifndef _LAX_MEASUREINTERFACE_H
#define _LAX_MEASUREINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/coordinate.h>
#include <lax/interfaces/somedata.h>

namespace LaxInterfaces {



//--------------------------------- MeasureInterface ----------------------------


class MeasureInterface : public anInterface
{
 protected:
	int mx,my,draggingmode;
	Coordinate *curpoint;
	Coordinate *points;
	double extra[6];
	LineStyle linestyle;
 public:
	unsigned long controlcolor;
	int target,mode,showdecs;

	MeasureInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~MeasureInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual int scan(int x,int y);
	virtual int LBDown(int x,int y,unsigned int state,int count);
	virtual int LBUp(int x,int y,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state);
	virtual int Refresh();
	//virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual void deletedata();
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual const char *whattype() { return "MeasureInterface"; }
	virtual const char *whatdatatype() { return "None"; }
};

} // namespace LaxInterfaces;

#endif

