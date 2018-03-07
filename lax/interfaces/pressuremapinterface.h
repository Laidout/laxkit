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
//    Copyright (C) 2014 by Tom Lechner
//
#ifndef _LAX_PRESSUREMAPINTERFACE_H
#define _LAX_PRESSUREMAPINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/coordinate.h>
#include <lax/curveinfo.h>

namespace LaxInterfaces { 



class PressureMapInterface : public anInterface
{
  protected:
	char showdecs;
	Laxkit::CurveInfo curve;
	Laxkit::NumStack<flatpoint> points;
	Laxkit::NumStack<double> histogram;
	Laxkit::NumStack<double> pticker;
	int max_histogram_value, num_histogram_samples;
	int histogram_threshhold;
	int max_ticker, cur_ticker;

	int device;
	std::string device_name;

	//DoubleBBox scratch;
	//DoubleBBox curve;
	//DoubleBBox tickerbox;

	//Laxkit::ShortcutHandler *sc;

	virtual int send();

  public:
	unsigned int interface_style;
	double smooth_pixel_threshhold;
	Laxkit::ScreenColor linecolor;

	PressureMapInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~PressureMapInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "PressureMap"; }
	const char *Name();
	const char *whattype() { return "PressureMapInterface"; }
	const char *whatdatatype() { return NULL; } // is creation only
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	//virtual Laxkit::ShortcutHandler *GetShortcuts();
	//virtual int PerformAction(int action);

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
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	//virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

};

} // namespace LaxInterfaces

#endif

