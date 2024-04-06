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

#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/aninterface.h>
#include <lax/language.h>
#include <lax/singletonkeeper.h>


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


InterfaceManager::InterfaceManager()
{
	tools            = NULL;
	resources        = NULL;
	datafactory      = NULL;
	thin_line        = 1;
	near_threshhold  = 5;
	near_threshhold2 = 10;
	dragged_threshhold = 5;
	preview_size     = 300;
	previewer        = NULL;
}

InterfaceManager::~InterfaceManager()
{
	DBG cerr <<"----InterfaceManager destructor begin"<<endl;

	if (tools)       { tools->dec_count();       tools=NULL; }
	if (resources)   { resources->dec_count();   resources=NULL; }
	if (datafactory) { datafactory->dec_count(); datafactory=NULL; }
	if (previewer)   { previewer->dec_count();   previewer=NULL; }

	DBG cerr <<"----InterfaceManager destructor end"<<endl;
}

/*! Get a Displayer object suitable for rendering object previews.
 * This is kind of a cached Displayer object to minimize scratch buffer reallocation.
 */
Laxkit::Displayer *InterfaceManager::GetPreviewDisplayer()
{
	if (!previewer) {
		previewer=newDisplayer(NULL);
	}
	return previewer;
}

/*! Return a new Displayer object for some specific purpose.
 * By convention, purpose==DRAWS_Screen means a displayer fit for screen display which
 * may choose speed over quality, and DRAWS_Hires means a displayer meant for print,
 * that is, for high resolution rendering but perhaps slower.
 *
 * Note that unlike GetPreviewDisplayer(), this returns a new object, and thus must
 * be dec_counted when done.
 *
 * Default here is to just return newDisplayer(NULL).
 */
Laxkit::Displayer *InterfaceManager::GetDisplayer(int purpose)
{
	return newDisplayer(NULL);
}

/*! The tools ResourceManager object by default has two ResourceTypes.
 *
 * One is "tools", each resource of which is an instance of any usable interface. This is
 * a flat list, no nested Resource menus here, or favorites.
 *
 * The other is "settings", each resource of which has id the same the tool it is settings
 * for. Again, no nesting.
 */
ResourceManager *InterfaceManager::GetTools()
{
	if (!tools) {
		tools=new ResourceManager();
		tools->AddResourceType("tools",    _("Tools"),         _("Tools"),         NULL);
		tools->AddResourceType("settings", _("Tool settings"), _("Tool settings"), NULL); 
	}
	return tools;
}

anInterface *InterfaceManager::GetTool(const char *tool)
{
	if (!tools) GetTools();
	if (!tools) return NULL;

	anInterface *interf = dynamic_cast<anInterface*>(tools->FindResource(tool, "tools"));
	return interf;
}

anObject *InterfaceManager::GetSettings(const char *for_tool)
{
	if (!tools) GetTools();
	if (!tools) return NULL;

	anObject *obj = tools->FindResource(for_tool, "settings");
	return obj;
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

/*! Draw random data by finding an interface that can draw it, then draw it WITHOUT
 * first pushing its transform to the Displayer current transfrom stack.
 * 
 * Return -1 for unknown data type, or 0 for drawn.
 */
int InterfaceManager::DrawDataStraight(Laxkit::Displayer *dp,LaxInterfaces::SomeData *data,
							Laxkit::anObject *a1,Laxkit::anObject *a2,unsigned int info)
{
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

static SingletonKeeper default_manager;
// InterfaceManager *InterfaceManager::default_manager=NULL;


/*! Return the current default interface manager.
 * If one does not exist and create, then create a new one and set it to current.
 */
InterfaceManager *InterfaceManager::GetDefault(bool create)
{
	InterfaceManager *im = dynamic_cast<InterfaceManager*>(default_manager.GetObject());
	if (!im) {
		im = new InterfaceManager;
		default_manager.SetObject(im,1);
	}
	return im;
}

/*! Passing in NULL will clear the current.
 */
void InterfaceManager::SetDefault(InterfaceManager *nmanager, int absorb_count)
{
	default_manager.SetObject(nmanager, absorb_count);
}


} //namespace LaxInterfaces



