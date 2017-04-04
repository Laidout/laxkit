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
//    Copyright (C) 2017 by Tom Lechner
//

#ifndef _LAX_SINGLETONKEEPER_H
#define _LAX_SINGLETONKEEPER_H


#include <lax/anobject.h>


namespace Laxkit {

/*! \class SingletonKeeper
 *
 * Automate singleton removal.. not sure if this is a good idea, but damned if it doesn't make
 * resource management easier.
 *
 * Meant to be a value object, so it finally deletes when it goes out of context at program end.
 */
class SingletonKeeper
{
  protected:
	anObject *object;

  public:
	SingletonKeeper(anObject *obj=NULL, bool absorb=false) { object=obj; if (object && !absorb) object->inc_count(); }
	~SingletonKeeper() { if (object) object->dec_count(); } //this happens on going out of scope
	anObject *GetObject() { return object; }
	void SetObject(anObject *nobj, bool absorb) {
		if (object) object->dec_count();
		object=nobj;
		if (object && !absorb) object->inc_count();
	}
};

//SingletonDestroyer colormanagerkeeper;
//... then the static GetDefault()/SetDefault() will update colormanagerkeeper
//... so ColorManager not created initially, but once it is created somewhere else,
//... it will be removed when colormanagerkeeper goes out of scope, ie at program termination

} // namespace Laxkit

#endif

