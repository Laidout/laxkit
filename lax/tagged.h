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
//   Copyright (c) 2010 Tom Lechner
//
#ifndef _LAXKIT_TAGGED_H
#define _LAXKIT_TAGGED_H

#include <lax/lists.h>


namespace Laxkit {


//-------------------------------- Tagged ------------------------------------------

class Tagged
{
 protected:
	PtrStack<char> list_of_tags;

 public:
	int sorttags;
	Tagged();
	virtual ~Tagged();

	virtual bool HasTag(const char *tag, int casematters, int *tag_index = nullptr);
	virtual int NumberOfTags();
	virtual const char *GetTag(int i);
	virtual char *GetAllTags();
	virtual int InsertTags(const char *tags, int casematters);
	virtual int InsertTag(const char *tag, int casematters);
	virtual int RemoveTag(const char *tag);
	virtual int RemoveTag(int i);
	virtual void FlushTags();
};


//-------------------------------- IntTagged ------------------------------------------

class IntTagged
{
 protected:
	NumStack<int> list_of_tags;

 public:
	int sorttags;
	IntTagged();
	virtual ~IntTagged();

	virtual bool HasTag(int tag, int *tag_index = nullptr);
	virtual int NumberOfTags();
	virtual int GetTag(int index);
	virtual int *GetAllTags(int *n);
	virtual int InsertTags(int *tags, int n);
	virtual int InsertTag(int tag);
	virtual int RemoveTag(int tag);
	virtual int RemoveTagIndex(int i);
	virtual void FlushTags();
};


//-------------------------------- TagCloudInfo ------------------------------------------

class TagCloudInfo
{
  public:
	int info;
	char *tag;
	PtrStack<Tagged> objs;

	TagCloudInfo(const char *t,int i=0);
	virtual ~TagCloudInfo();
};


//-------------------------------- TagCloud ------------------------------------------

class TagCloud : public Tagged
{
  protected:
	PtrStack<TagCloudInfo> taginfo;

  public:
	int keep_empty_tags;

	TagCloud();
	//virtual int InsertTags(const char *tags, int casematters);
	//virtual int InsertTag(const char *tag, int casematters);
	virtual int RemoveTag(const char *tag);
	virtual int RemoveTag(int i);
	virtual void FlushTags();

	virtual int AddObject(Tagged *obj);
	virtual int RemoveObject(Tagged *obj);
};



} //namespace Laxkit

#endif
