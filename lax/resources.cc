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
#include <cstdlib>
#include <lax/resources.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include <lax/refptrstack.cc>



#include <iostream>
#define DBG

using namespace std;

using namespace LaxFiles;



namespace Laxkit { 


//----------------------------- Resourceable -------------------------------

/*! \class Resourceable 
 *
 * Class to ease referencing other objects. Each user is meant to be a non-parent that
 * needs the object for some reason. The application must take care when reparenting to 
 * not delete the object while there are still users that need it.
 */

Resourceable::Resourceable()
{
	resource_owner=NULL;
}

Resourceable::~Resourceable()
{} 

/*! Redefine from anObject, default is just to return resource_owner.
 */
anObject *Resourceable::ObjectOwner()
{
	return resource_owner;
}

/*! Default is to return resource_owner.
 */
Laxkit::anObject *Resourceable::ResourceOwner()
{
	return resource_owner;
}

/*! Default is just to replace resource_owner with newowner.
 * It's assumed *this does not affect the count of resource_owner, that is, it's assumed
 * resource_owner will always outlive *this.
 */
void Resourceable::SetResourceOwner(anObject *newowner)
{
	resource_owner=newowner;
}

Laxkit::LaxImage *Resourceable::ResourceIcon()
{
	return NULL;
}


int Resourceable::dec_count()
{
	if (users.n && _count==users.n+1) {
		 //need to check all objects that can be connected to this one. If all have a count
		 //equal to their users.n, then the whole net is not connected to anything but itself
		 //and all those objects must be deleted
		cerr <<" *** need to implement isolated Resourceable::users net!"<<endl;

//		PtrStack<Resourceable> objs;
//		Resourceable *obj;
//		int c;
//		for (c=0; c<users.n; c++) {
//			obj=dynamic_cast<Resourceable*>(users.e[c]);
//			if (!obj) break;
//			if (obj->users.n!=obj->the_count()) break;
//		}
//		if (c==users.n) {
//			//***
//		}
	}

	return anObject::dec_count();
}

/*! Returns -1 if the item was pushed, otherwise the index of the item in the stack.
 *
 * This will inc_count() when object is not already a user.
 */
int Resourceable::AddUser(anObject *object)
{
	return users.pushnodup(object,0); //assume the users will always outlast this object
}

/*! Return 1 if an item is removed, else 0.
 *
 * This will dec_count() when object is a user.
 */
int Resourceable::RemoveUser(anObject *object)
{
	return users.remove(users.findindex(object));

}


//----------------------------- Resource -------------------------------

/*! \class Resource
 *
 * A case around an anObject to give extra meta and usage info about it.
 * Used in ResourceManager.
 */


Resource::Resource()
{
	object=NULL;
	owner=NULL;

	name=NULL;
	Name=NULL;
	description=NULL;
	icon=NULL;

	linkable=true; //if false, then checkouts must be duplicates

	ignore=false;
	favorite=0;
	source=NULL;
	source_type=0; //0 for object on its own,
				   //1 for object from file,
				   //3 for resource inserted during a directory scan
				   //2 for built in (do not dump out)
				   
	objecttype=NULL;
	config=NULL;
	creation_func=NULL;
}

Resource::Resource(anObject *obj, anObject *nowner, const char *nname, const char *nName, const char *ndesc, const char *nfile, LaxImage *nicon)
{
	object=obj;
	if (object) object->inc_count();
	owner=nowner;

	name=newstr(nname);
	Name=newstr(nName);
	description=newstr(description);
	icon=nicon;
	if (icon) icon->inc_count();
	source=newstr(nfile);
	if (nfile) source_type=1; else source_type=0;

	linkable=true;

	ignore=false;
	favorite=0;

	objecttype=NULL;
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
 * Stores meta about the type, as well as a list of Resource and ResourceType objects.
 * ResourceType in the resources list is just a shortcut for easy coding of nested trees of
 * resources.
 *
 * There will be only one head per resource type, and within that head, it is 
 * assumed all objects are of that type.
 *
 * Some notes about dirs. You are welcome to follow the XDG Base Directory Specification
 * (and see also ResourceManager::AddDirs_XDG()):
 *   http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * - Single dir: XDG_DATA_HOME: defaults to ~/.local/share
 * 
 * - Single dir: XDG_CONFIG_HOME: defaults to ~/.config 
 *  
 * - Preference ordered List of other dirs to search for data: XDG_DATA_DIRS, default to  /usr/local/share/:/usr/share/
 *
 * - Preference ordered List of other dirs to search for config: XDG_CONFIG_DIRS, default to  /etc/xdg
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
  : Resource(NULL,NULL,nname,nName,ndesc,NULL,nicon),
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

/*! Return 0 for added, -1 for already there and not added.
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

/*! Find some object that has str in it. Ignore case.
 *
 * Search by Name, then by name, then by description.
 */
anObject *ResourceType::Find(const char *str, Resource **resource_ret)
{
	if (!str) {
		if (resource_ret) *resource_ret=NULL;
		return NULL;
	}

	anObject *obj=NULL;
	ResourceType *rt;

	 //search in Name
	for (int c=0; c<resources.n; c++) {
		 //check sub trees
		rt=dynamic_cast<ResourceType*>(resources.e[c]);
		if (rt) {
			obj=rt->Find(str,resource_ret);
			if (obj) return obj;
		}

		if (strcasestr(resources.e[c]->name,str)) {
			if (resource_ret) *resource_ret=resources.e[c];
			return resources.e[c]->object;
		}
	}

	if (resource_ret) *resource_ret=NULL;
	return NULL;
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
int ResourceType::AddResource(anObject *nobject, anObject *nowner, const char *nname, const char *nName, const char *ndescription, const char *nfile, LaxImage *nicon)
{
	if (Find(object)) return -1;

	Resource *r=new Resource(nobject,nowner,nname,nName,ndescription,nfile,nicon);
	resources.push(r);

	return 0;
}


//----------------------------- ResourceManager -------------------------------

/*! \class ResourceManager
 *
 * Class to ease remembering various settings and styles.
 */


ResourceManager::ResourceManager()
{
	app_name=NULL;
	app_version=NULL;
}

ResourceManager::~ResourceManager()
{
	delete[] app_name;
	delete[] app_version;
}

void ResourceManager::SetAppName(const char *nname, const char *nversion)
{
	makestr(app_name,nname);
	makestr(app_version,nversion);
}

/*! Add the usual directories to search in, according to the XDG Base Directory Specification.
 * Resources will be searched in dir[]/app_name/app_version/resource_name.
 * If app_name or app_version are NULL, then that component is not used
 *
 * Some notes about dirs. The XDG Base Directory Specification is at:
 *   http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * - Single dir: XDG_DATA_HOME: defaults to ~/.local/share
 * - Single dir: XDG_CONFIG_HOME: defaults to ~/.config   
 * - Preference ordered List of other dirs to search for data: XDG_DATA_DIRS, default to  /usr/local/share/:/usr/share/
 * - Preference ordered List of other dirs to search for config: XDG_CONFIG_DIRS, default to  /etc/xdg
 *
 * which_type==-1 means add to all existing types.
 *
 * Returns number of directories added.
 */
int ResourceManager::AddDirs_XDG(int which_type)
{
	const char *home=getenv("XDG_DATA_HOME");
	if (!home) home="~";

	 //create "/app_name/app_version/"
	char *extra=NULL;
	if (app_name) {
		appendstr(extra,"/");
		appendstr(extra,app_name);
		appendstr(extra,"/");
		if (app_version) {
			appendstr(extra,app_version);
			appendstr(extra,"/");
		}
	}

	PtrStack<char> dirs(2);
	dirs.push(newstr(home));

	const char *xdg_data  =getenv("XDG_DATA_DIRS");
	if (!xdg_data) xdg_data="/usr/local/share/:/usr/share/";
	int nn=0;
	char **strs=split(xdg_data, ':', &nn);
	for (int c=0; c<nn; c++) {
		dirs.push(strs[c]);
	}
	delete[] strs; //dirs now owns the split pieces

	const char *xdg_config=getenv("XDG_CONFIG_DIRS");
	if (!xdg_config) xdg_config="/etc/xdg/";
	strs=split(xdg_config, ':', &nn);
	for (int c=0; c<nn; c++) {
		dirs.push(strs[c]);
	}
	delete[] strs; //dirs now owns the split pieces


	int start=which_type;
	int end=which_type;
	if (start<0) start=0;
	if (end<0 || end>=types.n) end=types.n;

	ResourceType *type;
	char *dir=NULL;

	int numadded=0;
	for (int c=start; c<=end; c++) {
		type=types.e[c];
		if (isblank(type->name)) continue;
	
		for (int c2=0; c2<dirs.n; c2++) {
			dir=newstr(dirs.e[c2]);
			appendstr(dir,"/");
			appendstr(dir,extra);
			appendstr(dir,type->name);
			expand_home_inplace(dir);
			simplify_path(dir,1);
			if (file_exists(dir,1,NULL)==S_IFDIR) {
				if (type->AddDir(dir,-1)==0) numadded++;
			}
			delete[] dir;
		} 
	}

	delete[] extra;
	return numadded;
}

anObject *ResourceManager::FindResource(const char *name, const char *type, Resource **resource_ret)
{
	ResourceType *rtype=FindType(type);

	if (!name || !rtype) {
		if (resource_ret) *resource_ret=NULL;
		return NULL;
	}

	anObject *obj=rtype->Find(name,resource_ret);
	if (obj) {
		return obj;
	}

	if (resource_ret) *resource_ret=NULL;
	return NULL;
}

/*!
 * object's count will be inc'd.
 *
 * Return 0 for successful add, or nonzero for not added.
 * -1 if object is already there for type.
 */
int ResourceManager::AddResource(const char *type, //! If NULL, then use object->whattype()
							anObject *object, anObject *nowner,
							const char *name, const char *Name, const char *description, const char *file, LaxImage *icon)
{
	if (!object) return 1;
	if (!type) type=object->whattype();

	ResourceType *t=FindType(type);
	if (!t) {
		 //resource type not found, add new type with NULL icon and description. Both names will be type
		t=AddResourceType(type,type,NULL,NULL);
	}

	t->AddResource(object,nowner, name,Name,description,file,icon);
	return 0;
}

ResourceType *ResourceManager::FindType(const char *name)
{
	if (!name) return NULL;

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


void ResourceManager::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext)
{
	//Attribute att;
	//dump_out_atts(&att,what,context);
	//att.dump_out(f,indent);

	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0';

	for (int c=0; c<types.n; c++) {
		if (types.e[c]->ignore) continue;

		fprintf(f,"%s%s\n",spc, types.e[c]->name);
		// ***
	}
}

LaxFiles::Attribute *ResourceManager::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	return NULL;

//	if (!att) att=new Attribute();
//
//	if (what==-1) {
//	}
//
//	return att;

}

void ResourceManager::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext)
{
	if (!att) return;

	ResourceType *type;
    char *name,*value;
    int c;

	cerr << " *** FINISH IMPLEMENTING ResourceManager::dump_in_atts()!!!"<<endl;

    for (c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

		//at top level, each name is the name of a ResourceType

		if (!strcmp(name,"type")) {
			type=FindType(value);
			if (!type) {
				type=AddResourceType(value,value,NULL,NULL);

			}
		}

	}
}


} //namespace Laxkit

