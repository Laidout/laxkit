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
//    Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef _LAX_SOMEDATAFACTORY_H
#define _LAX_SOMEDATAFACTORY_H

#include <lax/interfaces/somedata.h>
#include <lax/lists.h>

enum LaxInterfaceDataTypes {
	LAX_BEZDATA,
	LAX_COLORPATCHDATA,
	LAX_ELLIPSEDATA,
	LAX_GRADIENTDATA,
	LAX_IMAGEDATA,
	LAX_IMAGEPATCHDATA,
	LAX_LINESDATA,
	LAX_PATCHDATA,
	LAX_PATHSDATA,
	LAX_RECTDATA,
	LAX_SOMEDATA,
	LAX_SOMEDATAREF,
	LAX_CAPTIONDATA,

	LAX_DATA_MAX
};


namespace LaxInterfaces {


typedef SomeData *(*NewSomeDataFunc)(SomeData *refobj);
typedef int (*DelSomeDataFunc)(SomeData *obj);

//---------------------------- SomeFacNode ---------------------------------
class SomeFacNode
{
 public:
	char *name;
	int info;
	int id;
	NewSomeDataFunc newfunc;
	DelSomeDataFunc delfunc;
	SomeFacNode() { info=id=0; name=NULL; }
	virtual ~SomeFacNode() { delete[] name; }
};

//---------------------------- SomeDataFactory ---------------------------------
class SomeDataFactory
{
 public:
	Laxkit::PtrStack<SomeFacNode> types;

	virtual ~SomeDataFactory() {}
	virtual int DefineNewObject(int newid, const char *newname, NewSomeDataFunc newfunc,DelSomeDataFunc delfunc);
	virtual SomeData *newObject(const char *objtype, SomeData *refobj=NULL);
	virtual SomeData *newObject(int objtype, SomeData *refobj=NULL);
	//virtual SomeData *newObject(int id, LaxFiles::Attribute *att);
	//virtual SomeData *newObject(LaxFiles::Attribute *att);
	virtual void delObject(SomeData *obj) {}

	virtual const char *TypeStr(int which);
	virtual int TypeId(int which);
	virtual int NumTypes() { return types.n; }
};

#ifndef SOMEDATAFACTORY_CC
extern LaxInterfaces::SomeDataFactory *somedatafactory;
#endif

} //namespace LaxInterfaces


#endif

