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
//    Copyright (C) 2011 by Tom Lechner
//
#ifndef _LAX_LAXTUIO_H
#define _LAX_LAXTUIO_H


#include <lax/laxdevices.h>
#include <lo/lo.h>

namespace Laxkit {



int SetupTUIOListener(const char *port); //default port is 3333

//-------------------------------------- TouchObject --------------------------------
class TouchObject : public LaxMouse
{
 public:
	int touch_id;
	double x,y; //scaled to screen
	double xv,yv,a;
	int active;
	char firsttime;
	anXWindow *focus;

	TouchObject(int touchid);
	virtual ~TouchObject();
	virtual int usesX() { return 0; }
	//virtual int grabDevice(anXWindow *win);
	//virtual int ungrabDevice();
	virtual int getInfo(anXWindow *win,
						int *screen, anXWindow **child,
						double *x, double *y, unsigned int *mods,
						double *pressure, double *tiltx, double *tilty, ScreenInformation **screenInfo);
	virtual int Set(double xx,double yy,double xxv,double yyv,double aa);
};

//------------------------------------- TUIOListener ----------------------------------
class TUIOListener : public LaxDevice
{
  protected:
	int filedescriptor;
  public:
	int current_frame;
	RefPtrStack<TouchObject> points;
	int firstinactive;
	time_t last_fseq;

	lo_server_thread  st;

	TUIOListener(const char *port);
	virtual ~TUIOListener();
	virtual int Start();
	virtual int Stop();
	virtual int tuioHandler( int argc, lo_arg **argv, const char *types);
	virtual int fd();
};

} //namespace Laxkit

#endif

