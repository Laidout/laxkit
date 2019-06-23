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
//    Copyright (C) 2017 by Tom Lechner
//

#ifndef _LAX_SINGLETONKEEPER_H
#define _LAX_SINGLETONKEEPER_H


#include <lax/anobject.h>


namespace Laxkit {

/*! \class SingletonKeeper
 *
 * Automate singleton management.. not sure if this is a good idea, but damned if it doesn't make
 * resource management easier.
 *
 * Meant to be a value object, so it finally deletes when it goes out of scope at program end.
 */
class SingletonKeeper
{
  protected:
	anObject *object;

  public:
	SingletonKeeper(anObject *obj = nullptr, bool absorb = false) { object = obj; if (object && !absorb) object->inc_count(); }
	~SingletonKeeper() { if (object) object->dec_count(); } //this happens on going out of scope
	anObject *GetObject() { return object; }
	void SetObject(anObject *nobj, bool absorb) {
		if (object) object->dec_count();
		object = nobj;
		if (object && !absorb) object->inc_count();
	}
};

} // namespace Laxkit


#endif

