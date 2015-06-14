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
#ifndef RESOURCES_H
#define RESOURCES_H

#include <lax/anobject.h>
#include <lax/dump.h>
#include <lax/tagged.h>
#include <lax/laximages.h>
#include <lax/refptrstack.h>

#include <ctime>


namespace Laxkit {


//----------------------------- Resourceable -------------------------------

class Resourceable : virtual public anObject
{
  protected:
	RefPtrStack<anObject> users;
	anObject *resource_owner;

  public:
	Resourceable();
	virtual ~Resourceable(); 

	virtual anObject *ObjectOwner();
	virtual anObject *ResourceOwner();
	virtual void SetResourceOwner(anObject *newowner);

	virtual LaxImage *ResourceIcon();
	virtual const char *WhatResourceType() { return whattype(); }

	virtual int AddUser(anObject *object);
	virtual int RemoveUser(anObject *object);
	virtual int NumUsers() { return users.n; }
	virtual anObject *GetUser(int which) { if (which<0 || which>=users.n) return NULL; return users.e[which]; }

	virtual int dec_count();
};


//----------------------------- Resource -------------------------------

typedef anObject *(*ResourceCreateFunc)(LaxFiles::Attribute *att);

class Resource : virtual public anObject, virtual public Tagged
{
  public:
	anObject *object;
	anObject *owner; //such as a document or a project, not usually direct owner of object?

	char *name;
	char *Name;
	char *description;
	LaxImage *icon;
	bool ignore;
	bool linkable;

	int favorite; //0 for not fav, positive for order in a favorites list
	int source_type; //0 for object on its own, 1 for object from file, 2 for built in (do not dump out)
	 //stand alone resource
	 //temp resource: in use by a random object
	 //resource scanned in from directory
	char *source;

	Resource();
	Resource(anObject *obj, anObject *nowner, const char *nname, const char *nName, const char *ndesc,  const char *nfile,LaxImage *nicon);
	virtual ~Resource();
	virtual const char *whattype() { return "Resource"; }

	char *objecttype;
	LaxFiles::Attribute *config; //when we are creating, not storing.
	ResourceCreateFunc creation_func;
	virtual anObject *Create();
};


//----------------------------- ResourceType -------------------------------

class ResourceType : public Resource
{
  protected: 
  public: 
	class ResourceDir
	{
	  public:
		unsigned long id;
		char *dir;
		std::time_t last_scan;
		bool ignore;
		bool auto_added;
	};
	PtrStack<char> dirs;
	 //dir last scan time

	RefPtrStack<Resource> resources;
	RefPtrStack<anObject> recent;

	LaxImage *default_icon;

	ResourceType();
	ResourceType(const char *nname, const char *nName, const char *ndesc, LaxImage *nicon);
	virtual ~ResourceType();
	virtual const char *whattype() { return "ResourceType"; }

	virtual int AddDir(const char *dir, int where);
	virtual int RemoveDir(const char *dir);
	virtual int Find(anObject *object);
	virtual anObject *Find(const char *str, Resource **resource_ret);
	virtual int AddResource(anObject *object, anObject *nowner, const char *name, const char *Name, const char *description, const char *file, LaxImage *icon);
};


//----------------------------- ResourceManager -------------------------------


class ResourceManager : public anObject, public LaxFiles::DumpUtility
{
  public:
	char *app_name; //where app specific resources are located, so search in */app_name/app_version/resource_name/*
	char *app_version; //only used if app_name!=NULL
	PtrStack<ResourceType> types;

	ResourceManager();
	virtual ~ResourceManager();
	virtual const char *whattype() { return "ResourceManager"; }

	virtual void SetAppName(const char *nname, const char *nversion);
	virtual int AddResource(const char *type, anObject *object, anObject *nowner, const char *name, const char *Name, const char *description, const char *file, LaxImage *icon);
	virtual anObject *FindResource(const char *name, const char *type, Resource **resource_ret=NULL);
	virtual ResourceType *AddResourceType(const char *name, const char *Name, const char *description, LaxImage *icon);

	virtual int AddResourceDir(const char *type, const char *dir, int where);
	virtual int RemoveResourceDir(const char *type, const char *dir);
	virtual ResourceType *FindType(const char *name);
	virtual int NumTypes() { return types.n; }
	virtual ResourceType *GetType(int which) { if (which>=0 && which<types.n) return types.e[which]; return NULL; }
	virtual int AddDirs_XDG(int which_type);

	virtual void       dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext);


};




} //namespace Laxkit

#endif


