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
//    Copyright (C) 2004-2007 by Tom Lechner
//

#ifndef _LAX_LINKEDLIST_H
#include <lax/linkedlist.h>
#endif

#ifndef _LAX_LINKEDLIST_CC
#define _LAX_LINKEDLIST_CC


*************** need to add this to the install files *******************

namespace Laxkit {

	
//-------------------------- LinkedList -----------------------------

/*! \class LinkedList
 * \brief Independent node type for doubly linked lists.
 *
 * \todo ***** this needs testing
 */


template <class T>
LinkedList<T>::LinkedList()
{
	next_node=prev_node=NULL;
}

/*! Calls disconnectNode(), which results in prev_node and next_node 
 * being set to NULL.
 * It does NOT delete adjacent nodes. The containing code should do
 * that with judicious use of disconnectNode() and deletes.
 */
template <class T>
LinkedList<T>::~LinkedList()
{
	disconnectNode();
}

//! If this is part of a list, then remove it.
/*! Connects prev_node to next_node if possible. Returns prev_node
 * if it exists, or next_node otherwise. prev_node and next_node get
 * set to NULL.
 */
template <class T>
T *LinkedList<T>disconnectNode()
{
	if (prev_node) prev_node->next_node=next_node;
	if (next_node) next_node->prev_node=prev_node;

	T *n=prev_node;
	if (!n) n=next_node;
	prev_node=next_node=NULL;
	return n;
}


//! Insert a node or a node chain after or before this.
/*! If node is part of a list, not an independent node, then
 * the whole list is added. If node is part of a closed list,
 * then that loop is cut between node and node->prev_node.
 *
 * Return 0 for success, nonzero for error.
 */
template <class T>
int LinkedList<T>::connectNode(T *node, int after)
{
	if (!node) return 1;
	T *ns, *ne, *n;
	
	node->openNodeLoop(0);
	ne=ns=node;
	while (ne->next_node) ne=ne->next_node;
	while (ns->prev_node) ns=ns->prev_node;

	if (after) {
		if (next_node) next_node->prev_node=ne;
		ne->next_node=next_node;
		next_node=ns;
		ns->prev_node=this;
		return 0;
	}
	if (prev_node) prev_node->netx_node=ns;
	ns->prev_node=prev_node;
	prev_node=ne;
	ne->next_node=this;
	return 0;
}

//! Close the loop, if it was open.
/*! If it was already closed, return 1. Else return 0 for success.
 */
template <class T>
int LinkedList<T>::closeNodeLoop()
{
	T *s=this, *e=this;
	while (s->prev_node && s->prev_node!=this) s=s->prev_node;
	if (s->prev_node==this) return 1;
	while (e->next_node) e=e->next_node;
	s->prev_node=e;
	e->next_node=s;
	return 0;
}

//! Open a closed loop. If after!=0, then cut the loop after *this, else before.
/*! 
 * Return NULL if loop was already open and nothing was severed.
 * On success return pointer to the severed bit. That is, say you have a-this-b,
 * and you call this->openNodeLoop(0), then a is returned. Else b is returned.
 *
 * Note that if this is looped to itself, then this is still returned.
 */
template <class T>
T *LinkedList<T>::openNodeLoop(int after)
{
	 // first check that it is closed
	T *s=this;
	while (s->prev_node && s->prev_node!=this) s=s->prev_node;
	if (!s->prev_node) return 1;

	if (after) {
		s=this->next_node;
		s->prev_node=NULL;
		this->next_node=NULL;
		return s;
	} 
	s=this->prev_node;
	s->next_node=NULL;
	this->prev_node=NULL;
	return s;
}



//-------------------------------------- PtrList -----------------------------------
/*! \class PtrList
 * \ingroup templates
 * \brief A simple doubly linked list of pointers.
 *
 * An instance of this is really a linked list head, and the actual list is stored
 * internally. For a linked list whose nodes can be subclassed directly, see LinkedList.
 * 
 * ******PLEASE NOTE that this class is not really finished or tested, and is not used by anything
 * in the Laxkit at the moment.
 * 
 * \code
 * #include <lax/linkedlist.h>
 * \endcode
 *
 * If you define LAX_DONT_INCLUDE_LINKEDLIST_CC before including linkedlist.h, then linkedlist.cc is 
 * not included. Otherwise, linkedlist.cc is included (the default).
 * 
 * The member functions are the same as for PtrStack. The difference is the data
 * are stored in a doubly linked list rather than an array, which might be better in some
 * ways when dealing with huge amounts of data.
 *
 * Please note that there is no copy constructor or assignement operator defined.
 */
	

template <class T>
PtrList::node::~node()
{ 
	delete next; 
	if (data) 
		if (dataislocal==1) delete data; 
		else if (dataislocal==2) delete[] data;
}

//! List destructor, just call flush()
template <class T>
PtrList<T>::~PtrList<T>()
{ 
	flush(); 
}

//! Flush the list.
/*! If PtrList::arrays==1 then the elements are delete with <tt>delete[]</tt> 
 *  rather than just <tt>delete</tt>. If the islocal flag for the element is zero,
 *  then the element is not delete'd at all.
 */
template <class T>
void PtrList<T>::flush()
{
	delete first;
	first=NULL;
	num=0;
}

//! Find the index (in the range [0,n-1]) corresponding to the pointer t.
/*! If the element is not in the list, then -1 is returned.
 */
template <class T>
int PtrList<T>::findindex(T *t) // returns index of first element equal to t
{
	int c;
	node *nd=first;
	for (c=0; c<num; c++) {
		if (nd->data==t) return c;
		nd=nd->next;
	}
	return -1;
}


//! Push a pointer onto the list before index where. Transfers pointer, does not duplicate.
/*! If called without where, pointer is pushed onto the top (highest n) position.
 *  If local==1, then when the list flushes or the the element is removed,
 *  then it is delete'd. If local==2 then the pointer is delete[]'d. Note that it is
 *  ok to push NULL.
 *
 *  Returns the number of elements on the stack.
 */
template <class T>
int PtrList<T>::push(T *ne,char local,int where) // local=1, where=-1
{ 
	if (where<0 || where>=num) where=num-1;
	node *nd=new node(ne,local);
	if (num==0) {
		num=1;
		first=nd;
		return num;
	}

	node *f=first;
	if (where==0) {
		first=nd;
		first->next=f;
		f->prev=first;
		num++;
		return num;
	}

	where--;
	while (where) { // position where at the element before where new one goes
		where--;
		f=f->next;
	}
	if (f->next) f->next->prev=nd;
	nd->next=f->next;
	nd->prev=f;
	f->next=nd;
	num++;
	
	return num;
}

 //! Pushes an element only if it is not already in the list.
 /*! Please note that this checks for whether anything in the list points
  * to the same thing that the supplied pointer points to, 
  * and NOT whether the contents of whatever the pointer points to match
  * the contents of any element in the list. Always pushes on the end.
  *
  * Returns 1 if the item was pushed, otherwise 0 if the item was already there.
  */
template <class T>
int PtrList<T>::pushnodup(T *ne,char local) // push on end returns 1 if pushed
{
	node *f=first;
	while (f) {
		if (f->data==ne) break;
		f=f->next;
	}
	if (!f) { push(ne,local); return 1; }
	return 0;
}

//! Pop element with index which, or the top if which not specified
/*! \return Returns the popped pointer, or NULL.
 */
template <class T> 
T *PtrList<T>::pop(int which) // which==-1
{
	if (which<-1) return NULL;
	T *t=NULL;
	pop(t,which);
	return t;
}

//! Pop element with index which (defaults to top), and make popped point to it.
/*! If there is no element to return, popped is set to NULL. If which is out of
 *  bounds, then the top most element is popped.
 * 
 * Returns the number of elements still in the list.
 */
template <class T> 
int PtrList<T>::pop(T *&popped,int which,char *local) // which=-1, returns new n //***is that right?
{
	if (num==0) { popped=NULL; return 0; }
	if (which<0 || which>=num) which=num-1;
	node *f=first;
	if (which==0) {
		first=first->next;
		first->prev=NULL;
	} else {
		which--;
		while (which) { // place f at node before the one to be deleted
			f=f->next;
			which--;
		}
		node *ff=f;
		f=f->next; // now ff is before the one to delete, f is the one to delete
		ff->next=f->next;
		if (ff->next) ff->next->prev=ff;
	}
	
	popped=f->data;
	f->data=NULL;
	f->next=f->prev=NULL;
	if (local) *local=f->dataislocal;
	delete f;
	num--;

	return num;
}

//! Pop and delete (if islocal) the element at index which.
/*! This purges the element by popping and then (if islocal) deleting it.
 * Default if no index is specified is to remove the top element.
 */
template <class T>
int PtrList<T>::remove(int which) //which=-1
{
	if (which<0 || which>=num) which=num-1;
	char l;
	T *t=NULL;
	pop(t,which,&l);
	if (l && t) 
		if (l==1) delete t; 
		else if (l==2) delete[] t;
	
	return num;
}


//---------------------------------------- NumList (not implemented) ---------------------------

//template <class T>
//class NumList
//{
//  protected:
//	int max;
//	struct node
//	{	T *data;
//		char dataislocal;
//		node *next;
//		node() { next=NULL; data=NULL; dataislocal=0; }
//		node(T *nd, char istobelocal) {next=NULL; data=nd; dataislocal=istobelocal; }
//		~node() { delete next; if (data && dataislocal) delete data; }
//	} *first;
//   public:
//	NumList() { max=0; first=NULL; }
//	NumList(int n); 
//	~NumList() { delete first; }
//	T &operator[](int n);
//	void flush();
//	void push(T nd);
//	T pop();
//	int howmany() { return max; }
//};


} // namespace Laxkit;

#endif

