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
//    Copyright (C) 2015 by Tom Lechner
//

#include <lax/interfaces/interfacemanager.h>

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;

namespace LaxInterfaces {


//-------------------------- InterfaceManager --------------------------

/*! \class InterfaceManager
 *
 * Class to simplify create and maintenance of interfaces.
 *
 * Contains: 
 *
 *  -DrawData(), general purpose function to be able to draw any known object onto a Displayer
 *  -a list of possible tools to use. These are typically copied in for each ViewerWindow.
 *  -a ResourceManager to keep resources available for objects to draw from
 *  -a settings manager to ease coordination of interface settings across interface instances
 */


InterfaceManager *InterfaceManager::default_manager=NULL;

InterfaceManager::InterfaceManager()
{
	tools=NULL;
	resources=NULL;
	datafactory=NULL;
}

InterfaceManager::~InterfaceManager()
{
	DBG cerr <<"----InterfaceManager destructor begin"<<endl;

	if (tools)       { tools->dec_count();       tools=NULL; }
	if (resources)   { resources->dec_count();   resources=NULL; }
	if (datafactory) { datafactory->dec_count(); datafactory=NULL; }

	DBG cerr <<"----InterfaceManager destructor end"<<endl;
}


ResourceManager *InterfaceManager::GetSettingsManager()
{
	if (!tools) {
		tools=new ResourceManager();
	}
	return tools;
}

ResourceManager *InterfaceManager::GetResourceManager()
{
	if (!resources) {
		resources=new ResourceManager();
		resources->SetObjectFactory(GetObjectFactory());
	}
	return resources;
}

IconManager *InterfaceManager::GetIconManager()
{
	return IconManager::GetDefault();
}

UndoManager *InterfaceManager::GetUndoManager()
{
	return Laxkit::GetUndoManager();
}

Laxkit::ObjectFactory *InterfaceManager::GetObjectFactory()
{
	if (!datafactory) {
		datafactory=new ObjectFactory();
	}
	return datafactory;
}


anObject *InterfaceManager::NewObject(const char *type)
{
	if (!datafactory) GetObjectFactory();
	anObject *obj=datafactory->NewObject(type);
	return obj;
}

anObject *InterfaceManager::NewObject(int type)
{
	if (!datafactory) GetObjectFactory();
	anObject *obj=datafactory->NewObject(type);
	return obj;
}

SomeData *InterfaceManager::NewDataObject(const char *type)
{
	if (!datafactory) GetObjectFactory();
	anObject *obj=datafactory->NewObject(type);
	SomeData *sd=dynamic_cast<SomeData*>(obj);
	if (sd) return sd;
	if (obj) obj->dec_count();
	return NULL;
}

SomeData *InterfaceManager::NewDataObject(int type)
{
	if (!datafactory) GetObjectFactory();
	anObject *obj=datafactory->NewObject(type);
	SomeData *sd=dynamic_cast<SomeData*>(obj);
	if (sd) return sd;
	if (obj) obj->dec_count();
	return NULL;
}

/*! For an object that is NOT currently a resource, make it one.
 * No check is done to ensure it's not currently a resource.
 */
int InterfaceManager::Resourcify(Laxkit::anObject *resource, const char *type)
{
	if (!type) type=resource->whattype();

	ResourceManager *rm=GetResourceManager();
	rm->AddResource(resource->whattype(), resource, NULL,
                    resource->Id(), resource->Id(), NULL, NULL, NULL);
			
	return 0;
}


void InterfaceManager::DrawSomeData(Laxkit::Displayer *ddp,LaxInterfaces::SomeData *ndata,
							Laxkit::anObject *a1,Laxkit::anObject *a2,int info)
{
	DBG cerr << " Warning! Default InterfaceManager::DrawSomeData() doesn't do anything!!"<<endl;
}


//---------------------default setup 


/*! Return the current default interface manager.
 * If one does not exist and create, then create a new one and set it to current.
 */
InterfaceManager *InterfaceManager::GetDefault(bool create)
{
	if (!default_manager) {
		default_manager=new InterfaceManager;
	}
	return default_manager;
}

/*! Passing in NULL will clear the current.
 */
void InterfaceManager::SetDefault(InterfaceManager *nmanager, int absorb_count)
{
	if (nmanager!=default_manager) {
		if (default_manager) default_manager->dec_count();
		default_manager=nmanager;
		if (!absorb_count) if (default_manager) default_manager->inc_count();
	}
}


} //namespace LaxInterfaces



