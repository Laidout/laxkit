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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_DATAFACTORY_H
#define _LAX_DATAFACTORY_H

#include <cstdio>

#include <lax/anobject.h>
#include <lax/lists.h>



namespace Laxkit {


typedef anObject *(*NewObjectFunc)(int parameter, anObject *refobj);

typedef int (*DelObjectFunc)(anObject *obj);


//---------------------------- ObjectFactoryNode ---------------------------------
class ObjectFactoryNode
{
 public:
	char *name;
	int info;
	int id;
	int parameter;
	NewObjectFunc newfunc;
	DelObjectFunc delfunc;

	ObjectFactoryNode() { parameter=0; info=id=0; name=NULL; }
	virtual ~ObjectFactoryNode() { delete[] name; }
};


//---------------------------- ObjectFactory ---------------------------------
class ObjectFactory : public anObject
{
  protected:
	virtual ObjectFactoryNode *newObjectFactoryNode();

  public:
	PtrStack<ObjectFactoryNode> types;

	//static ObjectFactory *default_factory;
	//static ObjectFactory *GetDefault(bool create_if_null);
	//static void SetDefault(ObjectFactory *newfactory);

	ObjectFactory() {}
	virtual ~ObjectFactory() {}
	virtual const char *whattype() { return "ObjectFactory"; }
	virtual int findPosition(const char *name, int *exists);
	virtual int FindType(const char *name);
	virtual int DefineNewObject(int newid, const char *newname, NewObjectFunc newfunc,DelObjectFunc delfunc, int param=0);

	virtual anObject *NewObject(const char *objtype, anObject *refobj=NULL);
	virtual anObject *NewObject(int objtype, anObject *refobj=NULL);
	virtual void delObject(anObject *obj) {}

	virtual const char *TypeStr(int which);
	virtual int TypeId(int which);
	virtual int NumTypes() { return types.n; }

	virtual void dump_out(FILE *f, int indent);
};



} //namespace Laxkit


#endif

