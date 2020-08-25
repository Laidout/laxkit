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
//    Copyright (C) 2014 by Tom Lechner
//
#ifndef _LAX_INTERFACES_SELECTION_H
#define _LAX_INTERFACES_SELECTION_H


#include <lax/anobject.h>
#include <lax/doublebbox.h>
#include <lax/lists.h>
//#include <lax/values.h>


namespace LaxInterfaces {


class ObjectContext; //forward declaration
class SomeData;


//--------------------------- SelectedObject -------------------------

/*! \class SelectedObject
 */
class SelectedObject
{
  public:
	int info;
	//LaxInterfaces::SomeData *obj;
	LaxInterfaces::ObjectContext *oc;
	//ValueHash properties;

	SelectedObject(LaxInterfaces::ObjectContext *noc, int ninfo);
	virtual ~SelectedObject();
};


//--------------------------- Selection -------------------------

class Selection : public Laxkit::anObject, public Laxkit::DoubleBBox
{
	Laxkit::PtrStack<SelectedObject> objects;
	int currentobject;
	anObject *base_object;

  public:
	Selection();
	virtual ~Selection();

	virtual Selection *duplicate();
	virtual int FindIndex(LaxInterfaces::ObjectContext *oc);
	virtual int FindIndex(LaxInterfaces::SomeData *obj);
	virtual int Add(LaxInterfaces::ObjectContext *oc, int where, int ninfo=-1);
	virtual int AddNoDup(LaxInterfaces::ObjectContext *oc, int where, int ninfo=-1);
	virtual int Remove(int i);
	virtual ObjectContext *Pop(int i);
	virtual void Flush();
	virtual LaxInterfaces::ObjectContext *CurrentObject();
	virtual int CurrentObjectIndex() { return currentobject; }
	virtual void CurrentObject(int which);
	virtual int ObjectIndex(SomeData *object);

	//virtual ObjectContext *CommonAncestor(ObjectContext *oc);

	virtual int n() { return objects.n; }
	virtual LaxInterfaces::ObjectContext *e(int i);
	virtual int e_info(int i);
	virtual int e_info(int i, int newinfo);
	//virtual ValueHash *e_properties(int i);

};


} //namespace LaxInterfaces

#endif

