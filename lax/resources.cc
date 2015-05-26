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
#include <lax/resources.h>
#include <lax/strmanip.h>

#include <lax/refptrstack.cc>


#include <iostream>
#define DBG



namespace Laxkit { 


//----------------------------- Resource -------------------------------

/*! \class Resource
 *
 * A case around an anObject to give extra meta and usage info about it.
 * Used in ResourceManager.
 */


Resource::Resource()
{
	object=NULL;

	name=NULL;
	Name=NULL;
	description=NULL;
	icon=NULL;

	ignore=false;
	favorite=0;
	source=NULL;
	source_type=0; //0 for object on its own,
				   //1 for object from file,
				   //3 for resource inserted during a directory scan
				   //2 for built in (do not dump out)

	config=NULL;
	creation_func=NULL;
}

Resource::Resource(anObject *obj, const char *nname, const char *nName, const char *ndesc, const char *nfile, LaxImage *nicon)
{
	object=obj;
	if (object) object->inc_count();

	name=newstr(nname);
	Name=newstr(nName);
	description=newstr(description);
	icon=nicon;
	if (icon) icon->inc_count();
	source=newstr(nfile);
	if (nfile) source_type=1; else source_type=0;

	ignore=false;
	favorite=0;

	config=NULL;
	creation_func=NULL;
}

Resource::~Resource()
{
	if (object) object->dec_count();

	delete[] name;
	delete[] Name;
	delete[] description;
	delete[] source;
	if (icon) icon->dec_count();

	if (config) delete config;
}

/*! Sometimes a resource takes up a lot of memory, so we can store basic config instead,
 * and generate it. If object!=NULL, just return this. Otherwise, create and return.
 */
anObject *Resource::Create()
{
	if (object) return object;
	if (!creation_func) return NULL;

	object=creation_func(config);
	return object;
}


//----------------------------- ResourceType -------------------------------

/*! \class ResourceType
 * 
 * Node type for use in ResourceManager.
 *
 * Stores meta about the type, as well as a list of Resource objects.
 * These can be nested in a tree, since ResourceType is also a Resource, though
 * it is not meant to be used outside the ResourceManager. There will be only
 * one head per resource type, and within that head, it is assumed all objects
 * are of that type.
 *
 * Some notes about dirs. You are welocme to follow the XDG Base Directory Specification:
 *   http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * - Single dir: XDG_DATA_HOME: defaults to ~/.local/share
 * 
 * - Single dir: XDG_CONFIG_HOME: defaults to ~/.config 
 *  
 * - Preference ordered List of other dirs to search for data: XDG_DATA_DIRS, default to  /usr/local/share/:/usr/share/

 * - Preference ordered List of other dirs to search for config: XDG_DATA_DIRS, default to  /etc/xdg
 *
 * - Where to store "user specific, non-essential" files: XDG_CACHE_HOME, default to ~/.cache
 *
 * - A temp dir that exists while a user is logged in with 0700 permissions: XDG_RUNTIME_DIR
 *   Files in it should have mod times no more than 6 hours old to avoid cleanup.
 *   Files should not be very big there, and are intended to be for communication and synchronization purposes, whatever that means.
 */


ResourceType::ResourceType()
	: dirs(LISTS_DELETE_Array)
{
	name=NULL;
	Name=NULL;
	description=NULL;
	icon=NULL;

	PtrStack<Resource> resources;

}

ResourceType::ResourceType(const char *nname, const char *nName, const char *ndesc, LaxImage *nicon)
  : Resource(NULL,nname,nName,ndesc,NULL,nicon),
	dirs(LISTS_DELETE_Array)
{
}

ResourceType::~ResourceType()
{
	delete[] name;
	delete[] Name;
	delete[] description;
	delete[] source;
	if (icon) icon->dec_count();
}

/*! Return 0 for added, -1 for already there.
 * where<0 means add at end of list.
 */
int ResourceType::AddDir(const char *dir, int where)
{
	 //check to see if already there
	for (int c=0; c<dirs.n; c++) {
		if (!strcmp(dirs.e[c], dir)) return -1;
	}

	dirs.push(newstr(dir), 2, where);
	return 0;
}

/*! Return 0 for removed, or 1 for not there already.
 */
int ResourceType::RemoveDir(const char *dir)
{
	for (int c=0; c<dirs.n; c++) {
		if (!strcmp(dirs.e[c], dir)) {
			dirs.remove(c);
			return 0;
		}
	}

	return 1;
}

/*! Return non zero if object found is resources somewhere. Else return 0.
 */
int ResourceType::Find(anObject *object)
{
	for (int c=0; c<resources.n; c++) {
		if (resources.e[c]->object==object) return c+1;

		 //check sub trees
		if (dynamic_cast<ResourceType*>(resources.e[c])) {
			if (dynamic_cast<ResourceType*>(resources.e[c])->Find(object))
				return 1; 
		}
	}

	return 0;
}

/*! Return -1 for already there. 0 for successfully added. Nonzero for error and not added.
 *
 * object's count will be incremented.
 */
int ResourceType::AddResource(anObject *nobject, const char *nname, const char *nName, const char *ndescription, const char *nfile, LaxImage *nicon)
{
	if (Find(object)) return -1;

	Resource *r=new Resource(nobject,nname,nName,ndescription,nfile,nicon);
	resources.push(r);

	return 0;
}


//----------------------------- ResourceManager -------------------------------

/*! \class ResourceManager
 *
 * Class to ease remembering various settings and styles.
 */


/*!
 * object's count will be inc'd.
 *
 * Return 0 for successful add, or nonzero for not added.
 * -1 if object is already there for type.
 */
int ResourceManager::AddResource(const char *type, //! If NULL, then use object->whattype()
							anObject *object, 
							const char *name, const char *Name, const char *description, const char *file, LaxImage *icon)
{
	if (!object) return 1;
	if (!type) type=object->whattype();

	ResourceType *t=FindType(type);
	if (!t) {
		 //resource type not found, add new type with NULL icon and description. Both names will be type
		t=AddResourceType(type,type,NULL,NULL);
	}

	t->AddResource(object, name,Name,description,file,icon);
	return 0;
}

ResourceType *ResourceManager::FindType(const char *name)
{
	for (int c=0; c<types.n; c++) {
		if (!strcmp(name,types.e[c]->name)) return types.e[c];
	}
	return NULL;
}

/*! If resource exists already, then just return that. Else create and add a new one.
 */
ResourceType *ResourceManager::AddResourceType(const char *name, const char *Name, const char *description, LaxImage *icon)
{
	ResourceType *t=FindType(name);
	if (t) return t;
	
	 //add sorted
	t=new ResourceType(name,Name,description,icon);
	int c;
	for (c=0; c<t->resources.n; c++) {
		if (strcmp(name,t->resources.e[c]->name)<0) break;
	}

	types.push(t,1,c);
	return t;
}

/*! Return 0 for added, -1 for already there, or >0 for resource not found.
 * where<0 means add at end of list.
 */
int ResourceManager::AddResourceDir(const char *type, const char *dir, int where)
{
	if (!type || !dir) return 1;
	ResourceType *t=FindType(type);
	if (!t) return 2;

	return t->AddDir(dir,where);
}

int ResourceManager::RemoveResourceDir(const char *type, const char *dir)
{
	if (!type || !dir) return 1;
	ResourceType *t=FindType(type);
	if (!t) return 2;

	return t->RemoveDir(dir);
}


} //namespace Laxkit


