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
#ifndef _LAX_RESOURCES_H
#define _LAX_RESOURCES_H

#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/objectfactory.h>
#include <lax/tagged.h>
#include <lax/laximages.h>
#include <lax/menuinfo.h>
#include <lax/refptrstack.h>

#include <ctime>


namespace Laxkit {


//----------------------------- Resourceable -------------------------------

class Resourceable : virtual public anObject
{
  protected:
	RefPtrStack<anObject> users;
	anObject *resource_owner; // this will be either a local owner or a Resource in app->resourcemanager

  public:
	Resourceable();
	virtual ~Resourceable(); 

	virtual anObject *ObjectOwner(); //convenience redirect from anObject
	virtual anObject *ResourceOwner();
	virtual void SetResourceOwner(anObject *newowner);
	virtual bool IsResourced();

	virtual LaxImage *ResourceIcon();
	virtual const char *WhatResourceType() { return whattype(); }

	virtual int AddUser(anObject *object);
	virtual int RemoveUser(anObject *object);
	virtual int NumUsers() { return users.n; }
	virtual anObject *GetUser(int which) { if (which<0 || which>=users.n) return NULL; return users.e[which]; }

	virtual int dec_count();
};


//----------------------------- Resource -------------------------------

typedef anObject *(*ResourceCreateFunc)(Attribute *config);
typedef anObject *(*ResourceFromFileFunc)(const char *file, Attribute *config);

class Resource : virtual public anObject, virtual public Tagged
{
  public:
	anObject *object;
	anObject *topowner; //such as a document or a project, not usually direct owner of object?

	char *name;
	char *Name;
	char *description;
	LaxImage *icon;
	bool ignore;
	bool linkable;
	Attribute *meta;

	int favorite; //0 for not fav, positive for order in a favorites list
	enum SourceType {
		BuiltIn = -1,
		Floating, // defined by what's in memory
		FromFile, // defined by what's in file
		FromConfig// defined by this->config
	};
	SourceType source_type;
	 //stand alone resource
	 //temp resource: in use by a random object
	 //resource scanned in from directory
	char *source;

	char *objecttype;
	Attribute *config; //when we are creating, not storing.
	ResourceCreateFunc creation_func = nullptr;
	ResourceFromFileFunc from_file_func = nullptr;
	virtual anObject *Create();

	Resource();
	Resource(anObject *obj, anObject *nowner, const char *nname, const char *nName, const char *ndesc,  const char *nfile,LaxImage *nicon);
	virtual ~Resource();
	virtual const char *whattype() { return "Resource"; }
	virtual anObject *GetObject();
};


//----------------------------- ResourceDir -------------------------------

class ResourceDir
{
  public:
	unsigned long id;
	char *dir;
	std::time_t last_scan;
	bool ignore;
	bool auto_added;

	ResourceDir();
	ResourceDir(const char *ndir, bool ignore, bool auto_added);
	virtual ~ResourceDir();
};

//----------------------------- ResourceDirs -------------------------------

class ResourceDirs : public PtrStack<ResourceDir>
{
  public:
	ResourceDirs();
	virtual ~ResourceDirs();
	virtual int AddDir(const char *dir, int where);
	virtual int RemoveDir(const char *dir);
};


//----------------------------- ResourceType -------------------------------

class ResourceType : public Resource
{
  protected: 
  public: 
	ResourceDirs dirs;
	 //dir last scan time

	RefPtrStack<Resource> resources;
	RefPtrStack<anObject> recent;

	ResourceCreateFunc creation_func = nullptr; //the default one, may be overridden for particular Resource objects
	ResourceFromFileFunc from_file_func = nullptr;

	LaxImage *default_icon;

	ResourceType();
	ResourceType(const char *nname, const char *nName, const char *ndesc, LaxImage *nicon);
	virtual ~ResourceType();
	virtual const char *whattype() { return "ResourceType"; }

	virtual int AddDir(const char *dir, int where);
	virtual int RemoveDir(const char *dir);
	virtual Resource *Find(anObject *object);
	virtual anObject *Find(const char *str, Resource **resource_ret);
	virtual Resource *FindFromRID(unsigned int id);
	virtual int Remove(anObject *obj);
	virtual int AddResource(anObject *object, anObject *ntopowner, const char *name, const char *Name, const char *description,
							const char *file, LaxImage *icon,bool builtin=false, const char *menu=nullptr);
	virtual int NumberNotBuiltIn();
	virtual int NumResources();
	virtual int MakeNameUnique(char *&thename);

	virtual MenuInfo *AppendMenu(MenuInfo *menu, bool do_favorites, int *numadded, int id_offset, int info, anObject *current=nullptr);
	virtual MenuInfo *AppendResourceMenu(MenuInfo *menu, const char *separator_label, const char *select_label,
										const char *select_favorite_label,
										int item_offset, int item_favorite_category, int item_category,
										const char *make_resource_label, int make_resource_id,
										const char *make_local_label, int make_local_id,
										Resourceable *current);
};


//----------------------------- ResourceManager -------------------------------


class ResourceManager : public anObject, public DumpUtility
{
  public:
	char *app_name; //where app specific resources are located, so search in */app_name/app_version/resource_name/*
	char *app_version; //only used if app_name!=NULL

	PtrStack<ResourceType> types;

	ObjectFactory *objectfactory;
	virtual ObjectFactory *GetObjectFactory() { return objectfactory; }
	virtual void SetObjectFactory(ObjectFactory *factory);

	ResourceManager();
	virtual ~ResourceManager();
	virtual const char *whattype() { return "ResourceManager"; }

	virtual void SetAppName(const char *nname, const char *nversion);

	 //resource management
	virtual int AddResource(const char *type, anObject *object, anObject *ntopowner,
							const char *name, const char *Name, const char *description, const char *file, LaxImage *icon,
							bool builtin=false, const char *menu=nullptr);
	virtual int RemoveResource(anObject *obj, const char *type);
	virtual anObject *FindResource(const char *name, const char *type, Resource **resource_ret=NULL);
	virtual Resource *FindResource(anObject *obj, const char *type);
	virtual Resource *FindResourceFromRID(unsigned int id, const char *type);

	virtual anObject *NewObjectFromType(const char *type);


	 //type management
	virtual MenuInfo *ResourceMenu(const char *type, bool include_recent, MenuInfo *menu, int id_offset, int info, anObject *current=nullptr);
	virtual int NumResources(const char *type);
	virtual ResourceType *AddResourceType(const char *name, const char *Name, const char *description, LaxImage *icon,
				ResourceCreateFunc create_func = nullptr, ResourceFromFileFunc from_file = nullptr);
	virtual ResourceType *FindType(const char *name);
	virtual ResourceType *GetTypeFromIndex(int which) { if (which>=0 && which<types.n) return types.e[which]; return NULL; }
	virtual int NumTypes() { return types.n; }

	virtual int AddResourceDir(const char *type, const char *dir, int where);
	virtual int RemoveResourceDir(const char *type, const char *dir);
	virtual int AddDirs_XDG(int which_type);


	 //io
	virtual void dump_out(FILE *f,int indent,int what,DumpContext *context);
	virtual void dump_out_list(ResourceType *type, FILE *f,int indent,int what,DumpContext *context);
    virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
    virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
	virtual void dump_in_list_atts(ResourceType *type, Attribute *att,int flag,DumpContext *context);

};


} //namespace Laxkit

#endif


