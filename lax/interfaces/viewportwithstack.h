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
	int vpwsfirsttime;
	ObjectContext *foundobj,*foundtypeobj,*firstobj; //obj just before firstobj should be the last one searched.
	ObjectContext *curobj;
	virtual void ClearSearch();
 public:
	Laxkit::RefPtrStack<SomeData> datastack;
 	ViewportWithStack(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder,Laxkit::Displayer *ndp=NULL);
	virtual ~ViewportWithStack();
	virtual void Refresh();
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	
	virtual int NewData(SomeData *d,ObjectContext **oc_ret);
	virtual int DropObject(SomeData *d, double x,double y);
	virtual int DeleteObject();
	
	virtual int FindObject(int x,int y, const char *dtype, 
						   SomeData *exclude, int start,
						   ObjectContext **oc);
	virtual int ChangeObject(ObjectContext *oc, int switchtool);
	virtual int ChangeContext(int x,int y,ObjectContext **oc);
	virtual int SelectObject(int i);
	virtual double *transformToContext(double *m,ObjectContext *oc,int invert,int full);
};

} // namespace laxkit

#endif

