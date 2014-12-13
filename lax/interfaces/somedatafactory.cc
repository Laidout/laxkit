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
//    Copyright (C) 2004-2011 by Tom Lechner
//

#include <lax/strmanip.h>
#include <lax/lists.cc>

#define SOMEDATAFACTORY_CC
#include <lax/interfaces/somedatafactory.h>

namespace LaxInterfaces {


/*! Static pointer to a SomeDataFactory, and interfaces and their data would 
 *  get new instances of themselves through it if it exists, else do normal new WhateverData.
 */
SomeDataFactory *somedatafactory=NULL;


typedef SomeData *(*NewSomeDataFunc)(SomeData *refobj);
typedef int (*DelSomeDataFunc)(SomeData *obj);

//---------------------------- SomeFacNode ---------------------------------
/*! \class SomeFacNode
 * \brief Internal node for SomeDataFactory.
 */


//---------------------------- SomeDataFactory ---------------------------------
/*! \class SomeDataFactory
 * \brief Class to get instances of interface data.
 *
 * This class makes it unnecessary to have a complicated system of function pointers
 * to add, remove, and save objects.
 */
/*! \fn int SomeDataFactory::delObject(T *obj)
 * \brief Currently does nothing and is not called from any Lax classes...
 */
/*! \fn SomeDataFactory::~SomeDataFactory()
 * \brief Empty virtual destructor.
 */

//! Add ability to make a new type of object.
/*! Returns whatever PtrStack::push() returns.
 */
int SomeDataFactory::DefineNewObject(int newid, const char *newname, NewSomeDataFunc newfunc,DelSomeDataFunc delfunc)
{
	SomeFacNode *node=new SomeFacNode();
	node->name=newstr(newname);
	node->id=newid;
	node->newfunc=newfunc;
	node->delfunc=delfunc;
	
	return types.push(node,1);
}

//! Return object based on id number objtype.
SomeData *SomeDataFactory::newObject(int objtype, SomeData *refobj)
{
	for (int c=0; c<types.n; c++) {
		if (objtype==types.e[c]->id) {
			return types.e[c]->newfunc(refobj);
		}
	}
	return NULL;
}

//! Return object based on name, which should be the same as object->whattype().
SomeData *SomeDataFactory::newObject(const char *objtype, SomeData *refobj)
{
	for (int c=0; c<types.n; c++) {
		if (!strcmp(objtype,types.e[c]->name)) {
			return types.e[c]->newfunc(refobj);
		}
	}
	return NULL;
}

//SomeData *newObject(int id, LaxFiles::Attribute *att)
//{
//}
//
//SomeData *newObject(LaxFiles::Attribute *att)
//{
//}

const char *SomeDataFactory::TypeStr(int which)
{
	if (which>=0 && which<types.n) return types.e[which]->name;
	return NULL;
}

int SomeDataFactory::TypeId(int which)
{
	if (which>=0 && which<types.n) return types.e[which]->id;
	return 0;
}




//------------------------------ drawing helper ------------------------------------------

/*! \typedef void (*DrawSomeDataFunc)(Laxkit::Displayer *dp,LaxInterfaces::SomeData *data);
 *
 * On rare occasions, it is necessary to do random drawing of objects directly onto Displayer objects,
 * bypassing windows entirely. Do that with this, and redefine DrawSomeData.
 */

/*! Stub for LaxInterfaces::DrawSomeData(). Does nothing here, applications must make it point elsewhere.
 */
void LaxDrawSomeData(Laxkit::Displayer *dp,LaxInterfaces::SomeData *data)
{}

DrawSomeDataFunc DrawSomeData = LaxDrawSomeData;






} //namespace LaxInterfaces
