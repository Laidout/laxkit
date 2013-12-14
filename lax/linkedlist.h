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
//    Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef _LAX_LINKEDLIST_H
#define _LAX_LINKEDLIST_H

namespace Laxkit {

#ifndef NULL
#define NULL 0
#endif


//-------------------------- LinkedList -----------------------------
template <class T>
class LinkedList
{
 protected:
	T *next_node,*prev_node;
 public:
	LinkedList();
	virtual ~LinkedList();
	virtual T *disconnectNode();
	virtual int connectNode(T *node);
	virtual int closeNodeLoop();
	virtual T *openNodeLoop(int before);
};

//-------------------------------------- PtrList -----------------------------------
template <class T>
class PtrList
{
 protected:
	int num;
	struct node
	{	T *data;
		char dataislocal;
		node *next,*prev;
		node() { next=prev=NULL; data=NULL; dataislocal=0; }
		node(T *nd, char istobelocal) {next=prev=NULL; data=nd; dataislocal=istobelocal; }
		~node();
	} *first;
 public:
	PtrList() { num=0; first=NULL; }
	virtual ~PtrList();
//	T &operator[](int n);
	virtual void flush();
	virtual int howmany() { return num; }
	virtual int push(T *nd,char local=1,int where=-1);
	virtual int pop(T *&popped,int which=-1,char *local=NULL);
	virtual T *pop(int which=-1); // -1 means from the end, <-1 means return NULL
	virtual int findindex(T *t);
	virtual int remove(int which=-1); // which is index
	virtual int pushnodup(T *nd,char local); // push on end, returns 1 if pushed
};




#ifndef LAX_DONT_INCLUDE_LINKEDLIST_CC
#include <lax/linkedlist.cc>
#endif

} // namespace Laxkit;

#endif

