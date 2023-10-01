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
//    Copyright (C) 2004-2006,2012,2015 by Tom Lechner
//
#ifndef _LAX_ANOBJECT_H
#define _LAX_ANOBJECT_H

#include <lax/refcounted.h>

#include <cstring>
#include <ctime>


namespace Laxkit {


class anObject : virtual public RefCounted
{
  protected:

  public:
	unsigned long object_id;
	char *object_idstr;
	std::clock_t modtime;

	anObject();
	virtual ~anObject();
	virtual const char *whattype() { return "anObject"; }
	virtual const char *IconKey() { return nullptr; }
	virtual bool istype(const char *type);
	virtual anObject *ObjectOwner() { return NULL; }
	virtual anObject *duplicate(anObject *ref) { return NULL; }
	virtual int inc_count();
	virtual int dec_count();
	virtual const char *Id();
	virtual const char *Id(const char *newid);
	virtual void touchContents();
};


} // namespace Laxkit

#endif


