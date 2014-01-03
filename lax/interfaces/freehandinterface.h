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
//    Copyright (C) 2004-2007,2011,2014 by Tom Lechner
//
#ifndef _LAX_FREEHANDINTERFACE_H
#define _LAX_FREEHANDINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/linestyle.h>

namespace LaxInterfaces { 


enum FreehandEditorStyles {
	FREEHAND_Till_Closed =(1<<0), //mouse down drag out a line, up and clicking adds points
	FREEHAND_Coordinates =(1<<1), //Construct Coordinate points
	FREEHAND_Flatpoints  =(1<<2), //Create a list of flatpoints
	FREEHAND_Path        =(1<<3),      //Create a PathsData
	FREEHAND_ScrollWindow=(1<<4), //scroll viewport if off screen
	FREEHAND_MAX
};

class RawPoint {
  public:
	flatpoint p;
	clock_t time;
	double pressure;
	double tiltx,tilty;
	RawPoint(flatpoint pp) { p=pp; time=0; pressure=0; tiltx=tilty=0; }
};

typedef Laxkit::PtrStack<RawPoint> RawPointLine;

class FreehandInterface : public anInterface
{
  protected:
	char showdecs;
	int mx,my;
	LineStyle linestyle;
	//Laxkit::ShortcutHandler *sc;

	int findLine(int id);

	virtual int send(int i);

  public:
	unsigned int freehand_style;
	double mindist;
	Laxkit::ScreenColor linecolor;
	Laxkit::ScreenColor pointcolor;

	Laxkit::PtrStack<RawPointLine> lines;
	Laxkit::NumStack<int> deviceids;

	FreehandInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~FreehandInterface();
	virtual anInterface *duplicate(anInterface *dup);
	const char *Name();
	const char *whattype() { return "FreehandInterface"; }
	const char *whatdatatype() { return NULL; } // is creation only
	//virtual Laxkit::ShortcutHandler *GetShortcuts();
	//virtual int PerformAction(int actionnumber);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	//virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	//virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

};

} // namespace LaxInterfaces

#endif

