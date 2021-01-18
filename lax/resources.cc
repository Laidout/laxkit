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

#include <cstdlib>

#include <lax/resources.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/misc.h>
#include <lax/language.h>


//template implementation:
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
	object   = NULL;
	topowner = NULL;

	name        = NULL;
	Name        = NULL;
	description = NULL;
	icon        = NULL;

	linkable = true;  // if false, then checkouts must be duplicates

	ignore      = false;
	favorite    = 0;
	source      = NULL;
	source_type = Floating;

	objecttype    = NULL;
	config        = NULL;
	creation_func = NULL;

	meta=NULL;
}

Resource::Resource(anObject *obj, anObject *nowner, const char *nname, const char *nName, const char *ndesc, const char *nfile, LaxImage *nicon)
{
	object = obj;
	if (object) object->inc_count();
	topowner = nowner;

	name        = newstr(nname);
	Name        = newstr(nName);
	description = newstr(ndesc);

	icon = nicon;
	if (icon) icon->inc_count();

	source = newstr(nfile);
	if (nfile) source_type = FromFile;
	else source_type = Floating;

	linkable = true;

	ignore   = false;
	favorite = 0;

	objecttype    = NULL;
	config        = NULL;
	creation_func = NULL;

	if (dynamic_cast<Resourceable *>(obj)) {
		Resourceable *r = dynamic_cast<Resourceable *>(obj);
		r->SetResourceOwner(this);
	}

	meta = NULL;
}

Resource::~Resource()
{
	DBG cerr <<"--Resource destructor for "<<name<<", id="<<(object ? object->object_id : object_id)<<endl;

	if (object) object->dec_count();
	if (icon)   icon  ->dec_count();

	delete[] name;
	delete[] Name;
	delete[] description;
	delete[] source;

	if (config) delete config;
	if (meta)   delete meta;
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


//----------------------------- ResourceDirs -------------------------------
/*! \class ResourceDirs 
 * Node class for ResourceDirs.
 */

ResourceDir::ResourceDir()
{
	id        =getUniqueNumber();
	dir       =NULL;
	last_scan =0;
	ignore    =false;
	auto_added=false;
}

ResourceDir::ResourceDir(const char *ndir, bool nignore, bool isauto)
{
	id        =getUniqueNumber();
	dir       =newstr(ndir);
	last_scan =0;
	ignore    =nignore;
	auto_added=isauto;
}

ResourceDir::~ResourceDir()
{
	delete[] dir;
}


//----------------------------- ResourceDirs -------------------------------
/*! \class ResourceDirs 
 *
 * Class to simplify accessing files within a set of common directories.
 */


ResourceDirs::ResourceDirs()
{}

ResourceDirs::~ResourceDirs()
{}

/*! Return 0 for added, -1 for already there and not added.
 * where<0 means add at end of list.
 */
int ResourceDirs::AddDir(const char *ndir, int where)
{
    if (!ndir) return 1;

    char *dir = full_path_for_file(ndir, NULL);
    int status=1;

    if (S_ISDIR(file_exists(dir,1,NULL))) {
        int c2=0;
        for ( ; c2<n; c2++) {
            if (!strcmp(dir, e[c2]->dir)) break;
        }

        if (c2==n) {
            push(new ResourceDir(dir, false, false));
            status=0;
        } else status=-1; //already there
    }

    delete[] dir;
    return status;

}

/*! Return 0 for removed, or 1 for not there already.
 */
int ResourceDirs::RemoveDir(const char *dir)
{
	for (int c=0; c<n; c++) {
		if (!strcmp(e[c]->dir, dir)) {
			remove(c);
			return 0;
		}
	}

	return 1;
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
{
	default_icon=NULL;
	creation_func=NULL;
}

ResourceType::ResourceType(const char *nname, const char *nName, const char *ndesc, LaxImage *nicon)
  : Resource(NULL,NULL,nname,nName,ndesc,NULL,nicon)
{
	default_icon=NULL;
	creation_func=NULL;
}

ResourceType::~ResourceType()
{
	if (default_icon) default_icon->dec_count();
}

/*! Return the number of actual resources, excluding folders.
 */
int ResourceType::NumResources()
{
	int n = 0;
	for (int c=0; c<resources.n; c++) {
		if (dynamic_cast<ResourceType*>(resources.e[c])) {
			n += dynamic_cast<ResourceType*>(resources.e[c])->NumResources();
		} else {
			if (!dynamic_cast<ResourceType*>(resources.e[c])) n++;
		}
	}
	return n;
}

/*! Make thename a unique name. Return 1 if name is changed, else 0.
 */
int ResourceType::MakeNameUnique(char *&thename)
{
	int changed = 0;
	for (int c=0; c<resources.n; c++) {
		if (dynamic_cast<ResourceType*>(resources.e[c])) { //resource folder
			if (dynamic_cast<ResourceType*>(resources.e[c])->MakeNameUnique(thename)) {
				c = -1;
				changed = 1;
				continue;
			}
		} else { //normal resource
			if (!strcmp(resources.e[c]->name, thename)) {
				char *newname = increment_file(thename);
				delete[] thename;
				thename = newname;
				c = -1;
				changed = 1;
				continue;
			}
		}
	}
	return changed;
}

/*! Return number of all the resources that are not built in. This is so we don't have to output
 * sections in save files if we don't have to.
 */
int ResourceType::NumberNotBuiltIn()
{
	int n = 0;
	for (int c=0; c<resources.n; c++) {
		if (dynamic_cast<ResourceType*>(resources.e[c])) {
			n += dynamic_cast<ResourceType*>(resources.e[c])->NumberNotBuiltIn();
		} else {
			if (resources.e[c]->source_type != BuiltIn) n++;
		}
	}
	return n;
}

/*! Return 0 for added, -1 for already there and not added.
 * where<0 means add at end of list.
 */
int ResourceType::AddDir(const char *dir, int where)
{
	return dirs.AddDir(dir, where);
}

/*! Return 0 for removed, or 1 for not there already.
 */
int ResourceType::RemoveDir(const char *dir)
{
	return dirs.RemoveDir(dir);
}

/*! Return 0 for removed, 1 for not found. */
int ResourceType::Remove(anObject *obj)
{
	for (int c=0; c<resources.n; c++) {
		if (resources.e[c]->object == obj) {
			resources.remove(c);
			return 0;
		}
	}
	ResourceType *rtype;
	for (int c=0; c<resources.n; c++) {
		rtype = dynamic_cast<ResourceType*>(resources.e[c]);
		if (rtype->Remove(obj) == 0) return 0;
	}
	return 1;
}

Resource *ResourceType::FindFromRID(unsigned int id)
{
	ResourceType *rt;
	Resource *res = nullptr;

	for (int c=0; c<resources.n; c++) {
		 //check sub trees
		rt = dynamic_cast<ResourceType*>(resources.e[c]);
		if (rt) {
			res = rt->FindFromRID(id);
			if (res) return res;
		}

		if (resources.e[c]->object_id == id) return resources.e[c];
	}

	return nullptr;
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

/*! Return which Resource contains object, or null if none.
 */
Resource *ResourceType::Find(anObject *object)
{
	Resource *res = nullptr;
	for (int c=0; c<resources.n; c++) {
		if (resources.e[c]->object == object) return resources.e[c];

		 //check sub trees
		if (dynamic_cast<ResourceType*>(resources.e[c])) {
			res = dynamic_cast<ResourceType*>(resources.e[c])->Find(object);
			if (res) return res;
		}
	}

	return nullptr;
}

/*! Return -1 for already there. 0 for successfully added. Nonzero for error and not added.
 *
 * object's count will be incremented.
 */
int ResourceType::AddResource(anObject *nobject, anObject *ntopowner, const char *nname, const char *nName, const char *ndescription,
								const char *nfile, LaxImage *nicon, bool builtin)
{
	if (Find(object)) return -1;

	Resource *r=new Resource(nobject,ntopowner,nname,nName,ndescription,nfile,nicon);
	if (builtin) r->source_type = BuiltIn;
	resources.push(r);
	r->dec_count();

	return 0;
}

/*! \todo *** should have progressive loading of submenus when resource list is file based and large.
 *
 * The info field of normal items is -1. The info of favorite items is >=0 and represents placement in the list.
 */
MenuInfo *ResourceType::AppendMenu(MenuInfo *menu, bool do_favorites, int *numadded)
{
	if (!menu) menu=new MenuInfo(name);
	
	Resource *r;
	for (int c=0; c<resources.n; c++) {
		r=resources.e[c];
		if (r->ignore) continue;

		if (do_favorites && !r->favorite) continue;

		if (dynamic_cast<ResourceType*>(r)) {
			 //sub list...
			if (do_favorites) menu->SubMenu(r->Name);
			int oldn=menu->n();
			dynamic_cast<ResourceType*>(r)->AppendMenu(menu,do_favorites,numadded);
			oldn=menu->n()-oldn;
			if (numadded) *numadded += oldn;
			if (do_favorites) {
				if (oldn==0) menu->Remove(-1); //remove added submenu when no items added
				menu->EndSubMenu();
			}

		} else {
			 //normal resource
			// menu->AddItem(r->Name ? r->Name : (r->name ? r->name : _("(unnamed)")),
			// 			  r->icon,
			// 			  r->object_id, //id, later event->info2
			// 			  LAX_OFF,
			// 			  do_favorites ? r->favorite : -1, //later event->info4
			// 			  NULL);
			menu->AddItem(r->Name ? r->Name : (r->name ? r->name : _("(unnamed)")),
						  r->object_id, //id, later event->info2
						  do_favorites ? r->favorite : -1, //later event->info4
						  r->icon,
						  -1,
						  LAX_OFF
						  );
			if (numadded) *numadded += 1;
		}
	}

	return menu;
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

	objectfactory=NULL;
}

ResourceManager::~ResourceManager()
{
	delete[] app_name;
	delete[] app_version;
	if (objectfactory) objectfactory->dec_count();
}

void ResourceManager::SetAppName(const char *nname, const char *nversion)
{
	makestr(app_name,nname);
	makestr(app_version,nversion);
}

void ResourceManager::SetObjectFactory(ObjectFactory *factory)
{
	if (objectfactory!=factory) {
		if (objectfactory) objectfactory->dec_count();
		objectfactory=factory;
		if (objectfactory) objectfactory->inc_count();
	}
}

/*! If menu!=NULL, then append to it. Else return a new one.
 *
 * If type not found, return NULL.
 */
MenuInfo *ResourceManager::ResourceMenu(const char *type, bool include_recent, MenuInfo *menu)
{
	ResourceType *rtype=FindType(type);
	if (!rtype) return NULL;

	if (!menu) menu=new MenuInfo(type);

	 //first do a favorites menu
	int numadded=0;
	rtype->AppendMenu(menu, true, &numadded);
	if (numadded) menu->AddSep();

	 //then add full menu
	rtype->AppendMenu(menu, false, &numadded);

	return menu;
}

int ResourceManager::NumResources(const char *type)
{
	ResourceType *rtype=FindType(type);
	if (!rtype) return 0;
	return rtype->resources.n;
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

/*! Search for Resource->object_id, as might be returned from a ResourceMenu().
 */
Resource *ResourceManager::FindResourceFromRID(unsigned int id, const char *type)
{
	ResourceType *rtype=FindType(type);
	if (!rtype) return nullptr;
	return rtype->FindFromRID(id);
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

Resource *ResourceManager::FindResource(anObject *obj, const char *type)
{
	ResourceType *rtype = FindType(type);

	if (!rtype || !obj) return nullptr;

	Resource *res = rtype->Find(obj);
	return res;
}

/*!
 * object's count will be inc'd.
 *
 * Return 0 for successful add, or nonzero for not added.
 * -1 if object is already there for type.
 */
int ResourceManager::AddResource(const char *type, //! If NULL, then use object->whattype()
							anObject *object, anObject *ntopowner,
							const char *name, const char *Name, const char *description, const char *file, LaxImage *icon,
							bool builtin)
{
	DBG cerr <<"Add resource "<<(object->Id() ? object->Id():"(no id!!)" )<<"..."<<endl;
	if (!object) return 1;
	if (!type) type=object->whattype();

	ResourceType *t=FindType(type);
	if (!t) {
		 //resource type not found, add new type with NULL icon and description. Both names will be type
		t=AddResourceType(type,type,NULL,NULL);
	}

	t->AddResource(object,ntopowner, name,Name,description,file,icon, builtin);
	return 0;
}

/*! Return 1 for removed, 0 for not found.
 * Note if removal leaves parent ResourceType empty, that ResourceType is NOT removed.
 */
int ResourceManager::RemoveResource(anObject *obj, const char *type)
{
	ResourceType *rtype = FindType(type);
	if (!rtype) return 0;
	rtype->Remove(obj);
	Resource *res = FindResource(obj, type);
	if (!res) return 0;

	rtype->Remove(obj);
	return 1;
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
	for (c=0; c<types.n; c++) {
		if (strcmp(name,types.e[c]->name)<0) break;
	}

	types.push(t,LISTS_DELETE_Single,c);
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


void ResourceManager::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	//Attribute att;
	//dump_out_atts(&att,what,context);
	//att.dump_out(f,indent);

	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0';

	ResourceType *type=NULL;

	for (int c=0; c<types.n; c++) {
		if (types.e[c]->ignore) continue;

		type = types.e[c];
		if (type->NumberNotBuiltIn() == 0) continue;

		fprintf(f,"%stype %s\n",spc, type->name);
		if (type->Name)        fprintf(f,"%s  Name %s\n",spc, type->Name);
		if (type->description) fprintf(f,"%s  description %s\n",spc, type->description);

		if (type->dirs.n) {
			fprintf(f,"%s  dirs \\\n",spc);

			for (int c=0; c<type->dirs.n; c++) {
				fprintf(f,"%s    %s\n",spc, type->dirs.e[c]->dir);
			}
		}
		
		dump_out_list(type, f,indent+2,0,context);
	}
}

void ResourceManager::dump_out_list(ResourceType *type, FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	if (!type || !type->resources.n) return;

	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0';

	Resource *resource;
	for (int c=0; c<type->resources.n; c++) {
		resource=type->resources.e[c];
		if (resource->ignore) continue;

		if (dynamic_cast<ResourceType*>(resource)) {
			fprintf(f,"%ssublist %s\n",spc, resource->name ? resource->name : "");
			dump_out_list(dynamic_cast<ResourceType*>(resource), f,indent+2,what,context);
			continue;
		}

		fprintf(f,"%sresource\n",spc);
		fprintf(f,"%s  name %s\n",spc, resource->name);
		if (resource->Name) fprintf(f,"%s  Name %s\n",spc, resource->Name);
		if (resource->description) fprintf(f,"%s  description %s\n",spc, resource->description);
		fprintf(f,"%s  favorite %d\n",spc, resource->favorite);

		if (resource->source_type == Resource::Floating && resource->object) {
			fprintf(f,"%s  object %s\n",spc, resource->object->whattype());
			if (dynamic_cast<DumpUtility*>(resource->object))
				dynamic_cast<DumpUtility*>(resource->object)->dump_out(f,indent+4,what,context);

		} else if (resource->source_type == Resource::FromFile && !isblank(resource->source)) {
			fprintf(f,"%s  file %s\n",spc, resource->source);

		} else if (resource->source_type == Resource::FromConfig && resource->config) {
			fprintf(f,"%s  config %s\n",spc, resource->objecttype);
			resource->config->dump_out(f,indent+4);

		} else if (resource->source_type == Resource::BuiltIn) {
			fprintf(f,"%s  builtin\n",spc);
		} 
	}
}

LaxFiles::Attribute *ResourceManager::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	cerr << " *** need to implement ResourceManager::dump_out_atts()!!"<<endl;
	return NULL;

//	if (!att) att=new Attribute();
//
//	if (what==-1) {
//	}
//
//	return att;

}

void ResourceManager::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;

	ResourceType *type;
    const char *name,*value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

		//at top level, each name is the name of a ResourceType

		if (!strcmp(name,"type")) {
			type=FindType(value);
			if (!type) type=AddResourceType(value,value,NULL,NULL);

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"Name")) {
					makestr(type->Name,value);

				} else if (!strcmp(name,"description")) {
					makestr(type->description,value);

				} else if (!strcmp(name,"dirs")) {
					const char *end=value;
					char *dir;

					while (*value) {
						end=strchr(value,'\n');
						if (!end) end=value+strlen(value);
						dir=newnstr(value,end-value);
						type->AddDir(dir,-1);
						delete[] dir;
						if (*end) value=end+1; else value=end;
					}
				}
			}

			dump_in_list_atts(type, att->attributes.e[c],0,context);
		}

	}
}

/*! Separated from main dump_in_atts() so as to allow recursive resource tree input.
 */
void ResourceManager::dump_in_list_atts(ResourceType *type, LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
    const char *name,*value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

		if (!strcmp(name,"sublist")) {
			ResourceType *sub=new ResourceType(value,value,NULL,NULL);
			type->resources.push(sub);
			sub->dec_count();
			dump_in_list_atts(sub, att->attributes.e[c], flag,context);

		} else if (!strcmp(name,"resource")) {
			Resource *resource=new Resource;
			int resourceok=0;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"name")) {
					makestr(resource->name,value);

				} else if (!strcmp(name,"Name")) {
					makestr(resource->Name,value);

				} else if (!strcmp(name,"description")) {
					makestr(resource->description,value);

				} else if (!strcmp(name,"favorite")) {
					resource->favorite=BooleanAttribute(value);

				} else if (!strcmp(name,"object")) {
					resource->source_type = Resource::Floating;
					anObject *newobject=NewObjectFromType(value);

					if (dynamic_cast<DumpUtility*>(newobject)) {
						dynamic_cast<DumpUtility*>(newobject)->dump_in_atts(att->attributes.e[c]->attributes.e[c2], flag,context);
						resourceok=1;
						resource->object=newobject;
						if (!isblank(newobject->object_idstr)) makestr(resource->name,newobject->Id());

					} else if (newobject) {
						newobject->dec_count();
					}

				} else if (!strcmp(name,"file")) {
					resource->source_type = Resource::FromFile;
					makestr(resource->source, value);
					resourceok=1;

				} else if (!strcmp(name,"config")) {
					resource->source_type = Resource::FromConfig;
					makestr(resource->objecttype,value);
					resource->config=att->attributes.e[c]->attributes.e[c2]->duplicate();
					resourceok=1;

				} else if (!strcmp(name,"builtin")) {
					resource->source_type = Resource::BuiltIn;
					// *** skip

				}
			}

			if (resourceok) type->resources.push(resource);
			resource->dec_count();
		}
	}
}

anObject *ResourceManager::NewObjectFromType(const char *type)
{
	if (!objectfactory) return NULL;
	return objectfactory->NewObject(type);
}

} //namespace Laxkit


