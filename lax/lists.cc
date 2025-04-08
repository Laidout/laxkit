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
//    Copyright (C) 2004-2010,2013 by Tom Lechner
//



#ifndef _LAX_LISTS_H
#include <lax/lists.h>
#endif

#ifndef _LAX_LISTS_CC
#define _LAX_LISTS_CC


// only for memcpy:
#include <cstring>


namespace Laxkit {

//--------------------------------------------- NumStack -----------------------------------

/*! \defgroup templates Laxkit Templates
 *
 * The templates defined by Laxkit are a basic stack (NumStack, by value, not pointers), a pointer stack (PtrStack),
 * a reference counting pointer stack (RefCounter).
 *
 * Please note that stacks replace its entire array when necessary, pop does not necessarily make new array.
 *
 * If you have a huge stack of objects, these classes are probably not the best things to use.
 *
 * \todo implement a doubly linked list of pointers (PtrList).
 */

/*! \class NumStack
 * \ingroup templates
 * \brief A generic stack for values (like int, double), not pointers.
 *
 * IMPORTANT: Do not use this to stack classes that have special allocations in them!! Internally,
 * this class uses memcpy and memmove to work on its elements. It does not trigger any assingment
 * operator.
 *
 * The internal array has max elements allocated. When an item is pushed, and the number of
 * actual elements is greater than max, then the array is reallocated with max+delta spaces.
 * If an element is popped, and the number of elements is less than max-2*delta, then the array
 * is reallocated with max-delta spaces.
 *
 * max is readonly, but you can set and get delta with Delta(int) and Delta().
 */
/*! \var int NumStack::delta
 * \brief Size of chunks of memory to add or remove from internal array.
 */
/*! \var int NumStack::max
 * \brief The number of spaces allocated in the internal array.
 */
/*! \var int NumStack::n
 * \brief The number of elements on the stack.
 */
/*! \var T *NumStack::e
 * \brief The elements of the stack.
 */
/*! \fn void NumStack::Delta(int ndelta)
 * \brief Set the delta, the size of a chunk of memory to allocate or deallocate.
 */
/*! \fn int NumStack::Delta()
 * \brief Get the delta, the size of a chunk of memory to allocate or deallocate.
 */
/*! \fn int NumStack::Allocated()
 * \brief Return number of elements currently allocated.
 */
/*! \fn int NumStack<T>::how_many()
 * \brief Returns how many things are on the stack.
 *
 * This function is not called from within NumStack, so derived classes can
 * make "how many things on the stack" mean something other than how many
 * elements are in e.
 */
/*! \fn NumStack<T>::NumStack()
 * \brief Constructor, creates null list, n=max=0, e=nullptr, delta=10.
 */


/*! \fn NumStack<T>::NumStack(const NumStack &numstack)
 * \brief Copy constructor, makes max=numstack.n+delta if numstack.e exists, else max=0.
 */
template <class T>
NumStack<T>::NumStack(const NumStack<T> &numstack)
	: delta(10), max(0), n(0),e(nullptr)
{
	delta = numstack.delta;
	if (numstack.e) {
		n   = numstack.n;
		max = n+delta;
		e   = new T[max];
		memcpy(e,numstack.e,n*sizeof(T));
		//for (int c=0; c<n; c++) e[c]=numstack.e[c];
	}
}

template <class T>
NumStack<T>::NumStack(NumStack<T> &&numstack)
{
	e     = nullptr;
	delta = 10;
	max   = 0;
	n     = 0;
	if (numstack.e) {
		max   = numstack.max;
		delta = numstack.delta;
		e     = numstack.extractArray(&n);
	}
}

/*! \fn NumStack<T> &NumStack<T>::operator=(NumStack &numstack)
 * \brief Preserves delta, adjust everything else.
 */
template <class T>
NumStack<T> &NumStack<T>::operator=(NumStack<T> &numstack)
{
	flush();
	if (numstack.e) {
		n   = numstack.n;
		max = n+delta;
		e   = new T[max];
		memcpy(e,numstack.e,n*sizeof(T));
	}
	return numstack;
}

/*! \fn NumStack<T> &NumStack<T>::operator=(NumStack &numstack)
 * \brief Preserves delta, adjust everything else.
 */
template <class T>
NumStack<T> &NumStack<T>::operator=(NumStack<T> &&numstack)
{
	flush();
	if (numstack.e) {
		max   = numstack.max;
		delta = numstack.delta;
		e     = numstack.ExtractArray(&n);
	}
	return *this;
}

/*! \fn const NumStack<T> &NumStack<T>::operator=(const NumStack &numstack)
 * \brief Preserves delta, adjust everything else.
 */
template <class T>
const NumStack<T> &NumStack<T>::operator=(const NumStack<T> &numstack)
{
	flush();
	if (numstack.e) {
		n=numstack.n;
		max=n+delta;
		e=new T[max];
		memcpy(e,numstack.e,n*sizeof(T));
	}
	return numstack;
}

//! Ensure that the number of allocated is at least newmax.
/*! If newmax<max, then nothing is done and max is returned.
 * The new (or old) number of allocated elements is returned.
 * The old elements are copied to the newly allocated array.
 */
template <class T>
int NumStack<T>::Allocate(int newmax)
{
	if (newmax < max) return max;
	T *newt = new T[newmax];
	//if (n) memcpy(newt,e,n*sizeof(T)); //copy over old data
	if (n) for (int c=0; c<n; c++) newt[c] = e[c]; //copy over old data
	delete[] e;
	e = newt;
	max = newmax;
	return max;
}

//! Return the e array, and set e to nullptr, max=n=0.
template <class T>
T* NumStack<T>::extractArray(int *nn)//n=nullptr
{
	T *ee=e;
	if (nn) *nn=n;
	e=nullptr;
	max=n=0;
	return ee;
}

//! Flush, then use a as the new array.
/*! Note that it is assumed that flush sets e=nullptr.
 * e is then set to a. No element copying is done, the actual
 * array a is used. Does not alter delta, sets max=nn.
 *
 * Returns 0 for success, nonzero for error.
 */
template <class T>
int NumStack<T>::insertArray(T *a,int nn)
{
	flush();
	e=a;
	max=n=nn;
	return 0;
}

//! Find the index (in the range [0,n-1]) corresponding to the value t.
/*! If the element is not in the stack, then -2 is returned. The -2 is used
 * rather than -1 so that one can call pop(findindex(t)) and not pop anything
 * if the element is not there. Otherwise, pop(-1) makes the top of the
 * stack popped.
 */
template <class T>
int NumStack<T>::findindex(T t) // returns index of first element equal to t
{
	int c;
	for (c=0; c<n; c++) if (e[c]==t) return c;
	return -2;
}

/*! Return positive number (the element index+1) if element is in list,
 * otherwise 0.
 */
template <class T>
int NumStack<T>::Contains(T t)
{
	int i = findindex(t);
	if (i >= 0) return i+1;
	return 0;
}

//! Swap the elements with indices i1 and i2.
/*! If i1 or i2 is out of bounds, then substitute the top of the stack for it.
 */
template <class T>
void NumStack<T>::swap(int i1,int i2)
{
	if (i1<0 || i1>=n) i1=n-1;
	if (i2<0 || i2>=n) i2=n-1;
	if (i1==i2) return;
	T t=e[i1];
	e[i1]=e[i2];
	e[i2]=t;
}

//! Should probably remove this function..
template <class T>
T &NumStack<T>::operator[](int i) // *** beware out of bounds values
{
	if (i<0 || i>=n) return e[0];
	return e[i];
}

//! Deletes e, sets e=nullptr, sets n and max to 0.
template <class T>
void NumStack<T>::flush()
{
	delete[] e;
	e = nullptr;
	n = 0;
	max = 0;
}

//! Just set n to 0. Unlike flush(), do not deallocate e.
template <class T>
void NumStack<T>::flush_n()
{
	n=0;
}

//! Pushes an element only if it is not already on the stack.
/*! Returns the index if the item was already on the stack. Otherwise -1, which
 * means that the item got pushed to the top of the stack and its index is n-1.
 */
template <class T>
int NumStack<T>::pushnodup(T ne) // push on end returns 1 if pushed
{
	int c;
	for (c=0; c<n; c++) if (e[c]==ne) break;
	if (c==n) { push(ne); return -1; }
	return c;
}

//! Push an element onto the stack before index where.
/*! If called push(whatever) default is to insert after the highest index.
 * Returns the index of the added item, or -1 for error.
 */
template <class T>
int NumStack<T>::push(T ne,int where) // where=-1, pushes before which
{
	if (where<0 || where>n) where=n;
	if (n==0) {
		n=1;
		if (max==0) { if (delta==0) max=1; else max=delta; e=new T[max]; }
		e[0]=ne;
		return 0;
	}
	if (n+1>max) {
		if (delta) max+=delta; else max++;
		T *temp=new T[max];
		//if (where>0) memcpy(temp,e,where*sizeof(T));
		//if (where<n) memcpy(temp+where+1,e+where,(n-where)*sizeof(T));
		//----
		if (where>0) for (int c=0; c<where; c++) temp[c] = e[c];
		if (where<n) for (int c=0; c<n-where; c++) temp[where+1+c] = e[where+c];
		temp[where]=ne;
		delete[] e;
		e=temp;
	} else {
		//if (where<n) memmove(e+where+1,e+where,(n-where)*sizeof(T));
		if (where<n) for (int c=n; c>where; c--) e[c] = e[c-1];
		e[where]=ne;
	}
	n++;
	return n-1;
}

//! Removes an element like pop(), but returns 0 if element found, else 1 if not found.
template <class T>
int NumStack<T>::remove(int which)
{
	if (which<0 || which>=n) return 1;
	pop(which);
	return 0;
}

//! Pop which.
/*! Shrink the array if the new n<max-2*delta.
 * Returns the number of elements on the stack.
 */
template <class T>
T NumStack<T>::pop(int which) // which=-1
{
	T popped=T();
	if (n==0) return popped;
	if (which<0 || which>=n) which=n-1;
	popped=e[which];
	n--;
	if (n<max-2*delta) { // shrink the allocated space
		if (n==0) { delete[] e; e=nullptr; max=0; }
		else {
			max -= delta;
			T *temp = new T[max];
			//if (which>0) memcpy(temp,e,which*sizeof(T));
			//if (which<n) memcpy(temp+which,e+which+1,(n-which)*sizeof(T));
			//---------
			if (which>0) for (int c=0; c<which; c++) temp[c] = e[c];
			if (which<n) for (int c=0; c<n-which; c++) temp[which+c] = e[which+1+c];
			delete[] e;
			e=temp;
		}
	//} else memmove(e+which,e+which+1,(n-which)*sizeof(T));
	} else for (int c=0; c<n-which; c++) e[which+c] = e[which+1+c];
	return popped;
}

//--------------------------------------------- PtrStack -----------------------------------

/*! \class PtrStack
 * \ingroup templates
 * \brief A generic stack for pointers (like anXWindow *, char *), not values.
 *
 * The internal array has max elements allocated. When an item is pushed, and the number of
 * actual elements is greater than max, then the array is reallocated with max+delta spaces.
 * If an element is popped, and the number of elements is less than max-2*delta, then the array
 * is reallocated with max-delta spaces.
 *
 * If created PtrStack(LISTS_DELETE_Array), then all the elements are assumed to be arrays, and
 * will be deleted with a call like <tt>delete[] element</tt> rather
 * than <tt>delete element</tt>. If PtrStack(LISTS_DELETE_Single) (the default constructor),
 * then default is delete item. If PtrStack(LISTS_DELETE_None),
 * then default is to not delete at all.
 *
 * When elements are pushed, there is the option of
 * specifying that the stack should not delete the element at all. Calling push(blah,LISTS_DELETE_Single) means
 * that "delete blah" will be called when removing the item or flushing the stack. Calling push(blah,LISTS_DELETE_Array)
 * means that "delete[] blah" will be called. When push(blah,-1) is called, the value of arrays
 * is used for its local tag. push(blah, any other number) will result in the element not being
 * delete'd at all.
 */
/*! \var int PtrStack::delta
 * \brief Size of chunks of memory to add or remove from internal array.
 */
/*! \var int PtrStack::max
 * \brief The number of spaces allocated in the internal array.
 */
/*! \var int PtrStack::n
 * \brief The number of elements on the stack.
 */
/*! \var T *PtrStack::e
 * \brief The elements of the stack.
 */

/*! \fn int PtrStack<T>::how_many()
 * \brief Returns how many things are on the stack.
 *
 * This function is not called from within PtrStack, so derived classes can
 * make "how many things on the stack" mean something other than how many
 * elements are in e.
 */
/*! \fn void PtrStack::Delta(int ndelta)
 * \brief Set the delta.
 */
/*! \fn int PtrStack::Delta()
 * \brief Get the delta.
 */
/*! \var char PtrStack::arrays
 * \brief The default local value for elements of the stack.
 *
 * Uses ListsDeleteType.
 * This should be LISTS_DELETE_Array to delete[] elements, rather than just delete (LISTS_DELETE_Single, the
 * default) or LIST_DELETE_None to not delete at all.
 */
/*! \fn PtrStack<T>::PtrStack(char nar)
 *
 * nar uses ListsDeleteType.
 * nar should be LISTS_DELETE_Array to delete[] elements, rather than just delete (LISTS_DELETE_Single, the
 * default) or LIST_DELETE_None to not delete at all.
 */
/*! \fn T *PtrStack<T>::operator[](int i)
 * \brief Return pointer to element i, or nullptr.
 */


template <class T>
PtrStack<T>::PtrStack(char nar)
	: max(0),
	  delta(10),
	  arrays(nar),
	  islocal(nullptr),
	  n(0),
	  e(nullptr)
{}

//! PtrStack Destructor, just calls flush().
template <class T>
PtrStack<T>::~PtrStack<T>()
{
	flush();
}

//! Swap the elements with indices i1 and i2.
/*! If i1 or i2 is out of bounds, then substitute the top of the stack for it.
 */
template <class T>
void PtrStack<T>::swap(int i1,int i2)
{
	if (i1<0 || i1>=n) i1=n-1;
	if (i2<0 || i2>=n) i2=n-1;
	if (i1==i2) return;
	T *t=e[i1];
	e[i1]=e[i2];
	e[i2]=t;
	char tl=islocal[i1];
	islocal[i1]=islocal[i2];
	islocal[i2]=tl;
}

/*! Take item i1 and put it at i2, sliding the ones in between.
 * Basically a pop and insert.
 */
template <class T>
void PtrStack<T>::slide(int i1,int i2)
{
	int local;
	T *popped=pop(i1, &local);
	push(popped, local, i2);
}

//! Flush, then use a as the new array.
/*! Note that it is assumed that flush sets e and islocal=nullptr.
 * e is then set to a. No element copying is done, the actual
 * arrays a and nl are used. Does not alter delta, sets max=nn.
 *
 * If nl==nullptr, the islocal array is filled with the
 * value of the arrays variable.
 *
 * Returns 0 for success, nonzero for error.
 *
 * Please note that if you are simply reaaranging the elements
 * currently on the stack, this function will not work, since
 * the current stack is flush()'d first, potentiall deleting
 * the elements.
 */
template <class T>
int PtrStack<T>::insertArrays(T **a,char *nl,int nn)
{
	flush();
	e = a;
	max = n = nn;
	if (nl) islocal = nl;
	else {
		islocal = new char[n];
		for (int c=0; c<n; c++) islocal[c] = arrays;
	}
	return 0;
}

//! Return the e and islocal arrays, and internally set them to nullptr, max=n=0.
/*! If local==nullptr, then just delete local, rather than return it.
 */
template <class T>
T **PtrStack<T>::extractArrays(char **local,int *nn)//local=nullptr, nn=nullptr
{
	T **ee = e;
	e = nullptr;
	if (local) *local = islocal;
	else delete[] islocal;
	islocal = nullptr;
	if (nn) *nn=n;
	max = n = 0;
	return ee;
}

//! Flush the stack. Deletes e and islocal and makes them nullptr.
/*! If the element's local==2 then the elements are delete with <tt>delete[]</tt>.
 *  If the local==1 it is just deleted with <tt>delete</tt>.
 *  If the islocal flag for the element is !=1 or 2,
 *  then the element is not delete'd at all.
 */
template <class T>
void PtrStack<T>::flush()
{
	//if (n == 0) return;
	for (int c=0; c<n; c++)
		if (e[c]) {
			if (islocal[c] == LISTS_DELETE_Array) delete[] e[c];
			else if (islocal[c] == LISTS_DELETE_Single) delete e[c];
		}
	delete[] e; e = nullptr;
	delete[] islocal; islocal = nullptr;
	n = 0;
	max = 0;
}

/*! Flush the stack WITHOUT deallocating internal arrays.
 *  This WILL delete individual elements and set them to null in the arrays, without reallocating the arrays,
 *  so this->max stays the same.
 *  If the element's local==2 then the elements are delete with <tt>delete[]</tt>.
 *  If the local==1 it is just deleted with <tt>delete</tt>.
 *  If the islocal flag for the element is !=1 or 2,
 *  then the element is not delete'd at all.
 */
template <class T>
void PtrStack<T>::flush_n()
{
	if (n == 0) return; // we assume all good
	for (int c = 0; c < n; c++)
		if (e[c]) {
			if (islocal[c] == LISTS_DELETE_Array) delete[] e[c];
			else if (islocal[c] == LISTS_DELETE_Single) delete e[c];
			e[c] = nullptr;
		}
	n = 0;
}

//! Find the index (in the range [0,n-1]) corresponding to the pointer t.
/*! If the element is not in the stack, then -2 is returned. The -2 is used
 * rather than -1 so that one can call pop(findindex(t)) and not pop anything
 * if the element is not there. Otherwise, pop(-1) makes the top of the
 * stack popped.
 */
template <class T>
int PtrStack<T>::findindex(T *t) // returns index of first element equal to t
{
	int c;
	for (c=0; c<n; c++) if (e[c] == t) return c;
	return -2;
}

/*! Return positive number (the element index+1) if element is in list,
 * otherwise 0.
 */
template <class T>
int PtrStack<T>::Contains(T *t)
{
	int i = findindex(t);
	if (i >= 0) return i+1;
	return 0;
}
//! Pop and delete (if islocal) the element at index which.
/*! This purges the element by popping and then (if islocal==1 or 2) deleting it.
 * Default if no index is specified is to remove the top element (which==-1).
 * If which==-2 then do nothing (see findindex()).
 *
 * Return 1 if an item is removed, else 0.
 */
template <class T>
int PtrStack<T>::remove(int which) //which=-1
{
	if (which==-2) return 0;
	if (which<0 || which>=n) which=n-1;
	if (which<0) return 0;

	char l = islocal[which];
	T *t = pop(which);
	if (t) {
		if (l == LISTS_DELETE_Array) delete[] t;
		else if (l == LISTS_DELETE_Single) delete t;
	}
	if (t) return 1; else return 0; //note t is deleted now, we just want to know if we had an address
}

//! Pop and delete (according to islocal) the element if it exists in the stack.
/*! This is basically a convenience function that just returns remove(findindex(t)).
 *
 * Return 1 if an item is removed, else 0. Passing in nullptr returns 0.
 */
template <class T>
int PtrStack<T>::remove(T *t)
{
	if (!t) return 0;
	return remove(findindex(t));
}

//! Push a pointer onto the stack before index where. Transfers pointer, does not duplicate.
/*! If called without where, pointer is pushed onto the top (highest n) position.
 *
 *  If local==-1, then use arrays for local.
 *  If local==1, then when the stack flushes or the the element is removed,
 *  then it is delete'd. If local==2, then the element will be delete[]'d.
 *  If local is any other value, then delete is not called on the element.
 *
 *  Please note that pushing nullptr objects is ok.
 *
 *  Returns the index of the new element on the stack, or -1 if the push failed.
 */
template <class T>
int PtrStack<T>::push(T *ne,char local,int where) // local=-1, where=-1
{
	if (where<0 || where>n) where=n;
	if (local == -1) local=arrays;
	if (n == 0) {
		n = 1;
		if (max == 0) {
			if (delta == 0) max = 1; else max = delta;
			delete[] e;       // shouldn't be necessary, but valgrind complains
			delete[] islocal; // shouldn't be necessary, but valgrind complains
			e = new T*[max]; //e and islocal should always have been null before here
			islocal = new char[max];
		}
		e[0] = ne;
		islocal[0] = local;
		return 0;
	}
	if (n+1 > max) {
		if (delta) max += delta; else max++;
		T **temp = new T*[max];
		char *templ = new char[max];
		if (where > 0) {
			memcpy(temp, e, where*sizeof(T*));
			memcpy(templ, islocal, where*sizeof(char));
		}
		if (where < n) {
			memcpy(temp+where+1,  e+where,       (n-where)*sizeof(T*));
			memcpy(templ+where+1, islocal+where, (n-where)*sizeof(char));
		}
		temp[where]  = ne;
		templ[where] = local;
		delete[] e; e = nullptr;
		delete[] islocal; islocal = nullptr;
		e = temp;
		islocal = templ;
	} else {
		if (where < n) {
			memmove(e+where+1,       e+where,       (n-where)*sizeof(T*));
			memmove(islocal+where+1, islocal+where, (n-where)*sizeof(char));
		}
		e[where] = ne;
		islocal[where] = local;
	}
	n++;
	return where;
}

//! Pushes an element only if it is not already on the stack.
/*! Please note that this checks for whether anything on the stack points
 * to the same address that the supplied pointer points to,
 * (address equality) NOT whether the contents of whatever the pointer points to match
 * the contents of any element on the stack (content equality).
 *
 * If item already exists, where is ignored... should it cause a swap instead?
 *
 * local is interpreted same way as in normal push(): -1 use default, 1 delete item,
 * 2 delete[] item. Other value, don't delete item.
 *
 * Returns -1 if the item was pushed, otherwise the index of the item in the stack.
 */
template <class T>
int PtrStack<T>::pushnodup(T *ne,char local,int where)//where=-1
{
	int c;
	for (c=0; c<n; c++) if (e[c] == ne) break;
	if (c == n) { push(ne,local,where); return -1; }
	return c;
}

//! Pop the first item that points where topop points.
/*! Return 1 if item popped, else 0.
 *
 * If local!=nullptr, then put whether the item was local there.
 */
template <class T>
int PtrStack<T>::popp(T *topop,int *local)
{
	int c;
	for (c=0; c<n; c++) if (e[c] == topop) break;
	if (c == n) return 0;
	pop(c,local);
	return 1;
}

//! Pop element with index which, or the top if which not specified
/*! If which==-1 (the default) then pop off the top (highest index) of the stack.
 * If which==-2, do nothing (see findindex()) and return nullptr.
 *
 * \return Returns the popped pointer, or nullptr on error.
 *
 * If local!=nullptr, then put whether the item was local there.
 */
template <class T>
T *PtrStack<T>::pop(int which,int *local) // which==-1,local=nullptr
{
	if (which < -1 || n == 0) return nullptr;
	if (which < 0 || which >= n) which = n-1;
	T *popped = e[which];
	if (local) *local = islocal[which];
	n--;
	if (n < max-2*delta) { // shrink the allocated space
		if (n == 0) {
			delete[] e; e=nullptr;
			delete[] islocal; islocal=nullptr;
			max = 0;
		} else {
			max -= delta;
			T **temp = new T*[max];
			char *templ = new char[max];
			if (which > 0) {
				memcpy(temp, e, which*sizeof(T*));
				memcpy(templ, islocal, which*sizeof(char));
			}
			if (which < n) {
				memcpy(temp+which, e+which+1, (n-which)*sizeof(T*));
				memcpy(templ+which, islocal+which+1, (n-which)*sizeof(char));
			}
			delete[] e; e = nullptr;
			delete[] islocal; islocal = nullptr;

			e = temp;
			islocal = templ;
		}
	} else {
		memmove(e+which, e+which+1, (n-which)*sizeof(T*));
		memmove(islocal+which, islocal+which+1, (n-which)*sizeof(char));
	}
	return popped;
}

//! Pop element with index which (defaults to top), and make popped point to it.
/*! If there is no element to return, popped is set to nullptr. If which is out of
 *  bounds, then the top most element is popped.
 *
 * If local!=nullptr, then put whether the item was local there.
 *
 * Returns the number of elements still in the stack.
 */
template <class T>
int PtrStack<T>::pop(T *&popped,int which,int *local) // which=-1,local=nullptr
{
	if (n == 0) { popped = nullptr; return 0; }
	popped = pop(which,local);
	return n;
}

/*! Return the number of item pointers currently allocated.
 * This is just this->max.
 */
template <class T>
int PtrStack<T>::Allocated()
{ return max; }

//! Ensure that the number of allocated is at least newmax.
/*! If newmax<max, then nothing is done and max is returned.
 * The new (or old) number of allocated elements is returned.
 * Elements beyond n are initialized to nullptr.
 */
template <class T>
int PtrStack<T>::Allocate(int newmax)
{
	if (newmax < max) return max;

	T **newt = new T*[newmax];
	if (n) memcpy(newt, e, n*sizeof(T*));
	delete[] e; e = nullptr;
	e = newt;

	char *templ = new char[max];
	if (n) memcpy(templ,islocal,n*sizeof(char));
	delete[] islocal;
	islocal = templ;

	max = newmax;
	for (int c=n; c<max; c++) e[c] = nullptr;

	return max;
}



} // namespace Laxkit;

#endif

