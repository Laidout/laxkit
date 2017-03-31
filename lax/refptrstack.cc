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
//    Copyright (C) 2006-2007 by Tom Lechner
//

#include <lax/refptrstack.h>

#ifndef _LAX_REFPTRSTACK_CC
#define _LAX_REFPTRSTACK_CC

#include <lax/anobject.h>
#include <lax/lists.cc>

#include <cstring>




namespace Laxkit {

//--------------------------------------------- RefPtrStack -----------------------------------

/*! \class RefPtrStack
 * \ingroup templates
 * \brief A RefPtrStack with refcounting elements.
 *
 * In addition to 0 (no special delete behavior), 1 (delete), and 2 (delete[]),
 * there is here also 3 for call dec_count() on the element if it can be cast to
 * anObject. If not, \a delete is called on it.
 */

template <class T>
RefPtrStack<T>::RefPtrStack(char nar)
	: PtrStack<T>(nar)
{}

//! RefPtrStack Destructor, empty.
template <class T>
RefPtrStack<T>::~RefPtrStack<T>()
{
	flush();
}

//! Flush the stack. Makes e==NULL.
/*! If the element's local==2 then the elements are delete with <tt>delete[]</tt>.
 *  If the local==1 it is just deleted with <tt>delete</tt>. 
 *  If the local==3, then if it can be cast to a anObject, then it's dec_count()
 *  is called. If it cannot be so cast, then it is simply deleted as if local==1.
 *  If the islocal flag for the element is !=1,2, or 3,
 *  then the element is not delete'd or decremented at all.
 */
template <class T>
void RefPtrStack<T>::flush()
{	
	if (PtrStack<T>::n==0) return;
	for (int c=0; c<PtrStack<T>::n; c++) {
		if (PtrStack<T>::e[c]) {
			if (PtrStack<T>::islocal[c]==LISTS_DELETE_Array) 
				delete[] PtrStack<T>::e[c]; 
			else if (PtrStack<T>::islocal[c]==LISTS_DELETE_Single) delete PtrStack<T>::e[c];
			else if (PtrStack<T>::islocal[c]==LISTS_DELETE_Refcount) {
				anObject *ref=dynamic_cast<anObject *>(PtrStack<T>::e[c]);
				if (ref) ref->dec_count();
				else delete PtrStack<T>::e[c];
			}
		}
	}
	delete[] PtrStack<T>::e;       PtrStack<T>::e=NULL;
	delete[] PtrStack<T>::islocal; PtrStack<T>::islocal=NULL;
	PtrStack<T>::n=0;
	PtrStack<T>::max=0;
}

//! Pop and delete (if islocal) the element at index which.
/*! This purges the element by popping and then (if islocal==1 or 2) deleting it, or
 * dec_count() on it if islocal==3.
 *
 * Default if no index is specified is to remove the top element (which==-1).
 * If which==-2 then do nothing (see findindex()).
 *
 * Return 1 if an item is removed, else 0.
 */
template <class T>
int RefPtrStack<T>::remove(int which) //which=-1
{
	if (which==-2) return 0;
	if (which<0 || which>=PtrStack<T>::n) which=PtrStack<T>::n-1;
	char l=PtrStack<T>::islocal[which];
	T *t=PtrStack<T>::pop(which);
	if (t) {
		if (l==LISTS_DELETE_Array) delete[] t;
		else if (l==LISTS_DELETE_Single) delete t;
		else if (l==LISTS_DELETE_Refcount) {
			anObject *ref=dynamic_cast<anObject *>(t);
			if (ref) ref->dec_count();
			else {
				delete t;
				//DBG cerr <<" *** Uh oh! RefPtrStack trying to dec_count something not dec_countable!"<<endl;
			}
		}
	}
	if (t) return 1; else return 0;
}

//! Push a pointer onto the stack before index where. Transfers pointer, does not duplicate.
/*! If called without where, pointer is pushed onto the top (highest n) position.
 *
 *  If local==-1, then use arrays for local.
 *  If local==LISTS_DELETE_Single, then when the stack flushes or the the element is removed,
 *  then it is delete'd. If local==LISTS_DELETE_Array, then the element will be delete[]'d.
 *  
 *  If local==LISTS_DELETE_Refcount then call dec_count() when the stack would otherwise delete it.
 *  That assumes the element can be cast to anObject. ne's count is incremented
 *  when pushed here.
 *
 *  If local is any other value, then delete is not called on the element.
 *
 *  If the item has LISTS_DELETE_Refcount for its local and it can be cast to anObject, then 
 *  inc_count() is called on it.
 * 
 *  Returns the index of the new element on the stack, or -1 if the push failed.
 */
template <class T>
int RefPtrStack<T>::push(T *ne,char local,int where) // local=-1, where=-1
{
	int i=PtrStack<T>::push(ne,local,where);
	if (i<0) return i;
	if (PtrStack<T>::islocal[i]==LISTS_DELETE_Refcount) {
		anObject *ref=dynamic_cast<anObject *>(ne);
		if (ref) ref->inc_count();
	}
	return i;
}

template <class T>
int RefPtrStack<T>::pushnodup(T *nd,char local,int where)
{
	return PtrStack<T>::pushnodup(nd,local,where);
}



} // namespace Laxkit;

#endif

