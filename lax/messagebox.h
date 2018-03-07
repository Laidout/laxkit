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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_MESSAGEBOX_H
#define _LAX_MESSAGEBOX_H

#include <lax/rowframe.h>
#include <lax/button.h>

namespace Laxkit {

#define MBOX_WRAP (1<<16)
	
class MessageBox : public RowFrame
{
 protected:
 public:
	
	MessageBox(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev, unsigned long nowner, const char *nsend,
						const char *mes);
	virtual ~MessageBox() {}
	virtual const char *whattype() { return "MessageBox"; }
	virtual int preinit();
	virtual int init();
	virtual Button *AddButton(const char *btext,int sendid,anXWindow *prev=NULL);
	virtual Button *AddButton(unsigned int buttontype,anXWindow *prev=NULL);
	virtual int Event(const EventData *e,const char *mes);
};


} // namespace Laxkit

#endif

