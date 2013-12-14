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
#ifndef _LAX_LISTS_H
#define _LAX_LISTS_H

namespace Laxkit {

#ifndef NULL
#define NULL 0
#endif

template <class T>
class NumStack
{
 protected:
	int delta,max; // delta is size of chunk to add or remove, max is how many spaces allocated
 public:
	int n;
	T *e;
	NumStack() : delta(10), max(0), n(0),e(NULL) {}
	NumStack(const NumStack &numstack); // copy constructor
	NumStack &operator=(NumStack &numstack); // equals operator
	const NumStack &operator=(const NumStack &numstack); // equals operator
	virtual ~NumStack() { if (e) delete[] e; }
	virtual T &operator[](int i);
	virtual void flush();
	virtual int howmany() { return n; }
	virtual int findindex(T t);
	virtual void swap(int i1,int i2);
	virtual int push(T nd,int where=-1);
	virtual int pushnodup(T nd);
	virtual T pop(int which=-1);
	virtual int remove(int which=-1);
	virtual void Delta(int ndelta) { if (ndelta>=0) delta=ndelta; }
	virtual int Delta() { return delta; }
	virtual T *extractArray(int *nn=NULL);
	virtual int insertArray(T *a,int nn);
};


enum ListsDeleteType {
	LISTS_DELETE_None,
	LISTS_DELETE_Array,
	LISTS_DELETE_Refcount,
	LISTS_DELETE_MAX
};


template <class T>
class PtrStack
{
 protected:
	int max,delta;
	char arrays;
 public:
	char *islocal;
	int n;
	T **e;
	PtrStack(char nar=1);
	virtual ~PtrStack();
	virtual T *operator[](int i) { if (i>=0 && i<n) return e[i]; else return NULL; }
	virtual void flush();
	virtual int howmany() { return n; }
	virtual void swap(int i1,int i2);
	virtual int push(T *nd,char local=-1,int where=-1);
	virtual int popp(T *topop,int *local=NULL);
	virtual int pop(T *&popped,int which=-1,int *local=NULL);
	virtual T *pop(int which=-1,int *local=NULL); // -1 means from the end, <-1 means return NULL
	virtual int findindex(T *t);
	virtual int remove(int which=-1); // which is index
	virtual int pushnodup(T *nd,char local,int where=-1);
	virtual void Delta(int ndelta) { if (ndelta>=0) delta=ndelta; }
	virtual int Delta() { return delta; }
	virtual T **extractArrays(char **local=NULL,int *nn=NULL);
	virtual int insertArrays(T **a,char *nl,int nn);
};

} // namespace Laxkit;

#ifdef LAX_LISTS_SOURCE_TOO
#include <lax/lists.cc>
#endif


#endif

