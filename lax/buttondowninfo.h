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
//    Copyright (C) 2010 by Tom Lechner
//
#ifndef _LAX_BUTTONDOWNINFO_H
#define _LAX_BUTTONDOWNINFO_H

#include <cstdlib>
#include <lax/anobject.h>

namespace Laxkit {

//-------------------------- ButtonDownInfo ------------------------------
class ButtonDownInfo
{
	class ButtonDownInfoSpecific
	{
	 public:
		int device;
		int button;
		int initial_x, initial_y;
		int last_x, last_y;
		int current_x, current_y;
		int dragged;
		int info1,info2;
		anObject *extra;
		ButtonDownInfoSpecific *next;
		ButtonDownInfoSpecific(int d, int b, int x,int y, int i1,int i2,anObject *e,int absorbcount);
		~ButtonDownInfoSpecific();
	};
	ButtonDownInfoSpecific *info;
	ButtonDownInfoSpecific *exists(int d, int b);

 public:
	ButtonDownInfo() : info(NULL) {}
	~ButtonDownInfo() { if (info) delete info; }
	void clear();
	void down(int device_id, int button_id, int x=0, int y=0, int i1=0, int i2=0,anObject *e=NULL,int absorb=0);
	int move(int device_id, int x=0, int y=0, int *lastx=NULL, int *lasty=NULL);
	void moveinfo(int device_id, int button_id, int i1,int i2=0,int *oldi1=NULL,int *oldi2=NULL);
	int up(int device_id, int button_id, int *i1=NULL, int *i2=NULL);
	int any(int device_id=0,int button_id=0,int *device=NULL); //1 if any button of device_id is logged as down
	int whichdown(int afterthis, int button_id=0);
	int isdown(int device_id, int button_id, int *i1=NULL,int *i2=NULL);
	int isdragged(int device_id, int button_id);
	int average(int button_id, int *xavg, int *yavg);
	int getextrainfo(int device_id, int button_id, int *i1=NULL,int *i2=NULL);
	int getinitial(int device_id, int button_id, int *x0,int *y0);
	int getlast(int device_id, int button_id, int *xp,int *yp);
	int getcurrent(int device_id, int button_id, int *xp,int *yp);
	int getinfo(int device_id, int button_id, int *x0,int *y0, int *xp,int *yp, int *xc,int *yc,
				int *i1=NULL,int *i2=NULL);
	anObject *getextra(int device_id, int button_id);
	int replaceextra(int device_id, int button_id,anObject *e,int absorbcount);
};

} // namespace Laxkit

#endif

