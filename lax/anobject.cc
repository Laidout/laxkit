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
//    Copyright (C) 2004-2007,2012 by Tom Lechner
//

#include <lax/anobject.h>
#include <lax/misc.h>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//-------------------------- anObject ----------------------------
/*! \class anObject
 * \brief Base class for all Laxkit objects
 * 
 * In the constructor, object_id gets assinged a number returned from getUniqueNumber().
 *
 * Also provides inc_count() and dec_count() for reference counting.
 * Objects are created with a count of 1.
 */
	
DBG static int numofanObject=0;

//! Set object_id=getUniqueNumber().
anObject::anObject()
{
	suppress_debug=0;
	DBG numofanObject++;
	
	object_id=getUniqueNumber(); 
	object_idstr=NULL;
	DBG cerr <<"anObject tracker "<<object_id<<"   created    num of anObjects: "<<numofanObject<<endl;

	deleteMe=NULL;
	_count=1; 
}

/*! \fn const char *anObject::whattype()
 * \brief Returns the type of anObject.
 *
 * Usually, this corresponds to the C++ class name. However, it is also intended
 * to be used to say what the object should be considered to be. For instance, if you
 * subclass ImagePatchInterface to behave better, the program should still think of it
 * as an ImagePatchInterface, which is something the C++ typeinfo features are not
 * really designed for.
 */


//! Empty virtual destructor.
anObject::~anObject()
{
	DBG numofanObject--;
	DBG cerr <<"anObject tracker "<<object_id<<"   destroyed  num of anObjects: "<<numofanObject<<endl;

	if (object_idstr) delete[] object_idstr;
}


//---------------- reference counting stuff


/*! \typedef int (*DeleteRefCountedFunc)(anObject *obj)
 * \brief The type of function in an object optionally called on count reaching 0.
 */


/*! 
 * If this function pointer is not NULL, then every time a anObject::dec_count()
 * results in a count less than or equal to 0, that function gets called with
 * the object address as the argument, unless the corresponding
 * anObject::deleteMe is not NULL, in which case that deleteMe is
 * called instead of this function.
 *
 * This could be used, for instance to remove the object from
 * some stack, when the object is no longer referenced.
 *
 * Should return 1 if the object is no longer needed, and the destructor should delete
 * it, and 0 if it might still exist, like if it gets thrown onto a hidden cache, and
 * should not yet be deleted.
 *
 * NOTE, currently, this is not used in Laxkit.
 */
DeleteRefCountedFunc defaultDeleteRefCountedFunc=NULL;



/*! \var int anObject::_count
 * \brief The reference count of the object.
 *
 * Controlled with inc_count() and dec_count(). When the count
 * is less or equal to 0, then deleteMe is called. See dec_count() for more.
 */
/*! \var DeleteRefCountedFunc anObject::deleteMe
 * \brief Called when the count is decremented to 0.
 *
 * See dec_count() for more.
 */

	

/*! \fn int anObject::inc_count()
 * \brief Increment the data's count by 1. Returns count.
 */
int anObject::inc_count()
{
	_count++;
	DBG if (!suppress_debug) {
	DBG   cerr <<"refcounted inc count, now: "<<_count<<endl;
	DBG   cerr<<whattype()<<" "<<object_id<<" inc counted: "<<_count<<endl;
	DBG }
	return _count; 
}

//! Decrement the count of the data, deleting if count is less than or equal to 0.
/*! If count gets decremented to 0, then do the following. 
 * If deleteMe!=NULL, then call that.
 * Then, if defaultDeleteRefCountedFunc!=NULL, then call that. 
 * Then call "delete this" when count==0.
 *
 * Returns the count. If 0 is returned, the item is gone, and should
 * not be accessed any more.
 */
int anObject::dec_count()
{
	_count--;
	DBG if (!suppress_debug) {
	DBG   cerr <<"refcounted dec count, now: "<<_count<<(_count==0?", deleting":"")<<endl;
	DBG   cerr<<whattype()<<" "<<object_id<<" dec counted: "<<_count<<endl;
	DBG }

	if (_count<=0) {
		int yesdelete=1;
		if (deleteMe) yesdelete=deleteMe(this);
		else if (defaultDeleteRefCountedFunc) yesdelete=defaultDeleteRefCountedFunc(this);
		int c=_count;
		if (yesdelete) delete this;
		return c;
	}
	return _count; 
}


} // namespace Laxkit

