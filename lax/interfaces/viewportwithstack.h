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
//    Copyright (C) 2004-2011 by Tom Lechner
//
#ifndef _LAX_VIEWPORTWITHSTACK_H
#define _LAX_VIEWPORTWITHSTACK_H

#include <lax/interfaces/viewportwindow.h>

namespace LaxInterfaces {



//---------------------------- ViewportWithStack ----------------------
class ViewportWithStack : public ViewportWindow
{
 protected:
	flatpoint lastm;
	int vpwsfirsttime;
	ObjectContext *foundobj,*foundtypeobj,*firstobj; //obj just before firstobj should be the last one searched.
	ObjectContext *curobj;
	virtual void ClearSearch();

 public:
	bool draw_axes;
	bool draw_bounding_boxes;

	Laxkit::RefPtrStack<SomeData> datastack;
 	ViewportWithStack(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder,Laxkit::Displayer *ndp=NULL);
	virtual ~ViewportWithStack();
	virtual void Refresh();
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	
	virtual int NewData(SomeData *d,ObjectContext **oc_ret, bool clear_selection=true);
	virtual int DropObject(SomeData *d, double x,double y);
	virtual int DeleteObject();
	
	virtual int FindObject(int x,int y, const char *dtype, 
						   SomeData *exclude, int start,
						   ObjectContext **oc, int searcharea);
	virtual int ChangeObject(ObjectContext *oc, int switchtool);
	virtual int ChangeContext(int x,int y,ObjectContext **oc);
	virtual int SelectObject(int i);
	virtual double *transformToContext(double *m,ObjectContext *oc,int invert,int full);
};

} // namespace laxkit

#endif

