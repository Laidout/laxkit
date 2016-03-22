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
//    Copyright (C) 2004-2006,2012,2015 by Tom Lechner
//
#ifndef _LAX_ANOBJECT_H
#define _LAX_ANOBJECT_H

#include <cstring>

namespace Laxkit {


class anObject 
{
  protected:
	int _count;

  public:
	int suppress_debug;
	unsigned long object_id;
	char *object_idstr;
	anObject();
	virtual ~anObject();
	virtual const char *whattype() { return "anObject"; }
	virtual anObject *ObjectOwner() { return NULL; }
	virtual anObject *duplicate(anObject *ref) { return NULL; }
	virtual int inc_count();
	virtual int dec_count();
	virtual int the_count() { return _count; }
	virtual const char *Id();
	virtual const char *Id(const char *newid);
	virtual void touchContents();
};


} // namespace Laxkit

#endif


