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
#include <lax/interfaces/aninterface.h>

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
	preview_size=300;
}

InterfaceManager::~InterfaceManager()
{
	DBG cerr <<"----InterfaceManager destructor begin"<<endl;

	if (tools)       { tools->dec_count();       tools=NULL; }
	if (resources)   { resources->dec_count();   resources=NULL; }
	if (datafactory) { datafactory->dec_count(); datafactory=NULL; }

	DBG cerr <<"----InterfaceManager destructor end"<<endl;
}

/*! The tools ResourceManager object by default has two ResourceTypes.
 *
 * One is "tools", each resource of which is an instance of any interface usuable. This is
 * a flat list, no nested Resource menus here, or favorites.
 *
 * The other is "settings", each resource of which has id the same the tool it is settings
 * for. Again, no nesting.
 */
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

Laxkit::FontManager *InterfaceManager::GetFontManager()
{
	return anXApp::app->fontmanager;
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


//! Draw data using the transform of the data....
/*! \ingroup objects
 * Assumes dp.Updates(0) has already been called, and the transform
 * before data has been set appropriately. This steps through any groups, and looks
 * up an appropriate interface from laidout->interfacepool to draw the data.
 *
 * Note that for groups, a1 and a2 are passed along to all the group members..
 *
 * \todo currently this looks up which interface to draw an object with in LaidoutApp,
 *   but it should first check for suitable one in the relevant viewport.
 */
int InterfaceManager::DrawData(Displayer *dp,SomeData *data,anObject *a1,anObject *a2,unsigned int flags)
{
    dp->PushAndNewTransform(data->m()); // insert transform first
    int status=DrawDataStraight(dp,data,a1,a2,flags);
    dp->PopAxes();
	return status;
}

/*! Return -1 for unknown data type, or 0 for drawn.
 */
int InterfaceManager::DrawDataStraight(Laxkit::Displayer *dp,LaxInterfaces::SomeData *data,
							Laxkit::anObject *a1,Laxkit::anObject *a2,unsigned int info)
{
	DBG cerr << " Warning! Default InterfaceManager::DrawSomeData() doesn't do anything!!"<<endl;

	if (!tools) return -1;

	ResourceType *interfs = tools->FindType("tools");
	anInterface *interf=NULL;

	for (int c=0; c<interfs->resources.n; c++) {
		interf=dynamic_cast<anInterface*>(interfs->resources.e[c]->object);
		if (interf && interf->draws(data->whattype())) break;
	}

	if (interf) {
		interf->DrawDataDp(dp,data,a1,a2);
		return 0;
	}

	return -1;
}

/*! Return the square edge length in pixels to use as the default preview size for SomeData objects
 * that use the SomeData::GetPreview() mechanism.
 *
 * Default is 300.
 */
int InterfaceManager::PreviewSize()
{
	return preview_size;
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



