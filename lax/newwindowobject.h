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
//    Copyright (C) 2013 by Tom Lechner
//
#ifndef _LAX_NEWWINDOWOBJECT_H
#define _LAX_NEWWINDOWOBJECT_H

#include <lax/anxapp.h>

namespace Laxkit {

//---------------------------- NewWindowObject ---------------------------------
typedef anXWindow *(* NewWindowFunc)(anXWindow *parnt,const char *ntitle,unsigned long style,anXWindow *owner);

class NewWindowObject
{ 
 public:
	char *name,*desc;
	unsigned long style;
	NewWindowFunc function;
	NewWindowObject(const char *nname,const char *ndesc,unsigned long nstyle,NewWindowFunc f);
	virtual ~NewWindowObject();
};

} //namespace Laxkit


#endif


