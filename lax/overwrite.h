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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_OVERWRITE_H
#define _LAX_OVERWRITE_H

#include <lax/messagebox.h>

namespace Laxkit {

//-------------------------- Overwrite ------------------------------------
class Overwrite : public MessageBox
{
 public:
	int info1,info2,info3;
	char *file;
	Overwrite(unsigned long nowner,const char *mes, const char *nfile, int i1=-1,int i2=-1,int i3=-1);
	virtual ~Overwrite() { delete file; }
	virtual const char *whattype() { return "Overwrite"; }
	virtual int Event(const EventData *e,const char *mes);
};

} // namespace Laxkit

#endif

