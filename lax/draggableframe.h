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
//    Copyright (C) 2020 by Tom Lechner
//
#ifndef _LAX_DRAGGABLEFRAME_H
#define _LAX_DRAGGABLEFRAME_H

#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

namespace Laxkit {


class DraggableFrame : public anXWindow
{
  protected:
	anXWindow *child;
	ButtonDownInfo buttondown;
	int what, hover;

	virtual void SyncChild();
	virtual int scan(int x, int y, unsigned int state);

 public:
	double pad;
	double minx, miny;
	bool allow_drag;
	bool allow_x_resize;
	bool allow_y_resize;
	bool keep_in_parent;

	DraggableFrame(anXWindow *pwindow,
				const char *nname,
				const char *ntitle,
				unsigned long nstyle,
				int nx,int ny,int nw,int nh,int brder);
	virtual ~DraggableFrame();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual void Refresh();
	virtual const char *whattype() { return "DraggableFrame"; } 
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);

	virtual int SetChild(anXWindow *win); //absorbs

	 //serializing aids
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
};

} // namespace Laxkit

#endif

