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
//    Copyright (C) 2013 by Tom Lechner
//

#include <lax/refcounted.h>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//-------------------------- RefCounted ----------------------------
/*! \class RefCounted
 * \brief A minimal refcounted object.
 *
 * Provides inc_count() and dec_count() for reference counting.
 * Objects are created with a count of 1.
 */
	
/*! \var int RefCounted::_count
 * \brief The reference count of the object.
 *
 * Controlled with inc_count() and dec_count(). When the count
 * is less or equal to 0, then object has 'delete this' called. See dec_count() for more.
 */


RefCounted::RefCounted()
{
	suppress_debug=0;
	_count=1; 
}


//! Empty virtual destructor.
RefCounted::~RefCounted()
{
}

	

/*! \fn int RefCounted::inc_count()
 * \brief Increment the data's count by 1. Returns count.
 */
int RefCounted::inc_count()
{
	_count++;
	DBG if (!suppress_debug) {
	DBG   cerr <<"refcounted inc count, now: "<<_count<<endl;
	DBG }
	return _count; 
}

//! Decrement the count of the data, deleting if count is less than or equal to 0.
/*! Returns the count. If 0 is returned, the item is gone, and should
 * not be accessed any more.
 */
int RefCounted::dec_count()
{
	_count--;
	DBG if (!suppress_debug) {
	DBG   cerr <<"refcounted dec count, now: "<<_count<<(_count==0?", deleting":"")<<endl;
	DBG }

	if (_count<=0) {
		int c=_count;
		delete this;
		return c;
	}
	return _count; 
}


} // namespace Laxkit

