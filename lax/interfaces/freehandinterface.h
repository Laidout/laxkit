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
#ifndef _LAX_FREEHANDINTERFACE_H
#define _LAX_FREEHANDINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/bezinterface.h>
#include <lax/interfaces/linestyle.h>

namespace LaxInterfaces { 

class FreehandInterface : public anInterface
{
  protected:
	char showdecs;
	int mx,my;
	LineStyle linestyle;
  public:
  int mindist;
	unsigned long deconcolor,decoffcolor;
	BezData *data;
	FreehandInterface(int nid,Laxkit::Displayer *ndp);
	~FreehandInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual int LBDown(int x,int y,unsigned int state,int count);
	virtual int LBUp(int x,int y,unsigned int state);
	virtual int MouseMove(int x,int y,unsigned int state);
//	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state);
//	virtual int CharRelease(unsigned int ch,unsigned int state);
	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d) { if ((!d && data) || (d && d==data)) { data->dec_count(); data=NULL; } }
	const char *whattype() { return "FreehandInterface"; }
	const char *whatdatatype() { return "none"; } // is creation only
	int Refresh();
};

} // namespace LaxInterfaces

#endif

