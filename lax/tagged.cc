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


#include <lax/tagged.h>
#include <lax/strmanip.h>
#include <lax/attributes.h>

#include <lax/lists.cc>

#include <cctype>

using namespace std;

namespace Laxkit {

//-------------------------------- Tagged ------------------------------------------

/*! \class Tagged
 * \brief class to help implement simple tagging and tag querying system.
 *
 * Store a list of tags in char[] strings.
 *
 * \todo make some sort of tag manager, with special functions to build a tag
 *    cloud database, links to the tagged objects, be able to search by tag, and
 *    substitute tag aliases..
 */


Tagged::Tagged()
	: list_of_tags(2),
	  sorttags(1)
{
}

Tagged::~Tagged()
{
}

//! Return the number of tags, strangely enough.
int Tagged::NumberOfTags()
{
	return list_of_tags.n;
}

//! Return const pointer to the tag text for tag number i, where i==0 is the first tag.
/*! i must be in range [0..NumberOfTags()-1]. If i is out of bounds, then return NULL.
 */
const char *Tagged::GetTag(int i)
{
	if (i<0 || i>=list_of_tags.n) return NULL;
	return list_of_tags.e[i];
}

//! Return a new char[] with a space separated list of all the tags.
/*! If any tag has whitespace in it, it is put in quotes and the white space
 * is escaped, if necessary.
 */
char *Tagged::GetAllTags()
{
	if (!NumberOfTags()) return NULL;
	char *tags=NULL,*str;
	const char *tag;

	int n, nq, l, i;
	for (int c=0; c<list_of_tags.n; c++) {
		tag=GetTag(c);
		l=strlen(tag);
		n=nq=0;
		for (int c2=0; c2<l; c2++) {
			if (isspace(tag[c2])) n++;
			else if (tag[c2]=='"') nq++;
		}
		str=new char[l+n+nq+1];
		i=0;
		if (n) { str[i++]='"'; } //if tag has spaces, need to quote it!
		for (int c2=0; c2<l; c2++) {
			if (isspace(tag[c2])) {
				 //only accept spaces
				if (tag[c2]==' ') str[i++]=' ';
			} else if (tag[c2]=='"') {
				str[i++]='\\';
				str[i++]='"';
			} else str[i++]=tag[c2];
		}
		if (n) { str[i++]='"'; } //if tag has spaces, need to quote it!
		str[i]='\0';
		if (tags) appendstr(tags," ");
		appendstr(tags,str);
	}
	return tags;
}

//! Return whether the tag exists.
/*! casematters== -1 for might not care, 0=it does not matter, 1=it does matter.
 *
 * If the tag is not found, then return 0. 
 *
 * If casematters==1, then return the index+1 of the tag when the tag matches exactly, otherwise 0.
 * If casematters==0, then return the index+1 of the tag when the tag matches ignoring case, otherwise 0.
 * If casematters==-1, then if there is an exact match, return the index+1 of the tag. 
 * If there is a match only ignoring case, then return the index+1 of the tag times -1, otherwise 0.
 *
 */
int Tagged::HasTag(const char *tag, int casematters)
{
	if (!list_of_tags.n) return 0;

	for (int c=0; c<list_of_tags.n; c++) {
		if (casematters && !strcmp(tag,list_of_tags.e[c])) return c+1;
		if (casematters<1 && !strcasecmp(tag,list_of_tags.e[c]))
			return casematters==-1 ? -(c+1) : c+1;
	}
	return 0;
}

//! Remove tag number i. i must be in range [0..NumberOfTags()-1].
/*! Return 0 for tag removed. -1 for tag not found, 1 for other error and tag not removed.
 */
int Tagged::RemoveTag(int i)
{
	if (i<0 || i>=NumberOfTags()) return -1;

	list_of_tags.remove(i);
	return 0;
}

//! The tag must be an exact match.
/*! Return 0 for tag removed. -1 for tag not found, 1 for other error and tag not removed.
 */
int Tagged::RemoveTag(const char *tag)
{
	int c=HasTag(tag,1);
	if (!c) return -1;
	list_of_tags.remove(c-1);
	return 0;
}

//! Insert tags from a string such as 'tag1 tag2 tag3 "tag with spaces" tag4'
/*! Returns the number of tags inserted.
 */
int Tagged::InsertTags(const char *tags, int casematters)
{
	char *e;
	char *tag;
	int n=0;
	while (tags) {
		tag=QuotedAttribute(tags,&e);
		if (!tag || e==tags) break;
		if (!isblank(tag)) InsertTag(tag,casematters);
		delete[] tag;
		tags=e;
	}
	return n;
}

//! Insert tag if it doesn't exist already.
/*! Will insert ONLY if the tag does not exist there .......
 *
 * Returns the index of the tag.
 * 
 * If tag didn't exist, it is added, and its index is returned.
 */
int Tagged::InsertTag(const char *tag, int casematters)
{
	if (!tag) return 1;

	int c=HasTag(tag,-1);
	if (c) return c>0?c-1:-c+1;

	char *t=newstr(tag);
	int pos=list_of_tags.n;
	if (sorttags) {
		for (pos=0; pos<list_of_tags.n; pos++) {
			if (casematters && strcmp(list_of_tags.e[pos],tag)>0) break;
			if (!casematters && strcasecmp(list_of_tags.e[pos],tag)>0) break;
		}
	}
	list_of_tags.push(t,-1,pos);
	return 0;
}

void Tagged::FlushTags()
{
	list_of_tags.flush();
}


//-------------------------------- IntTagged ------------------------------------------

/*! \class IntTagged
 * \brief class to help implement simple tagging and tag querying system.
 *
 * Store a list of tags in char[] strings.
 *
 * \todo make some sort of tag manager, with special functions to build a tag
 *    cloud database, links to the tagged objects, be able to search by tag, and
 *    substitute tag aliases..
 */


IntTagged::IntTagged()
{
	sorttags=0;
}

IntTagged::~IntTagged()
{
}

//! Return the number of tags, strangely enough.
int IntTagged::NumberOfTags()
{
	return list_of_tags.n;
}

//! Return the tag at index of internal list.
/*! i must be in range [0..NumberOfTags()-1]. If i is out of bounds, then return -1.
 */
int IntTagged::GetTag(int index)
{
	if (index<0 || index>=list_of_tags.n) return -1;
	return list_of_tags.e[index];
}

//! Return a new int[] with the tags. Return the number of them in n.
int *IntTagged::GetAllTags(int *n)
{
	*n=list_of_tags.n;
	if (!NumberOfTags()) return NULL;

	int *tags = new int[list_of_tags.n];
	memcpy(tags, list_of_tags.e, list_of_tags.n*sizeof(int));
	return tags;
}

//! Return whether the tag exists.
/*! If the tag is not found, then return 0. 
 *
 * Return the index+1 of the tag when the tag matches exactly, otherwise 0.
 */
int IntTagged::HasTag(int tag)
{
	if (!list_of_tags.n) return 0;

	for (int c=0; c<list_of_tags.n; c++) {
		if (tag == list_of_tags.e[c]) return c+1;
	}
	return 0;
}

//! Remove tag number i. i must be in range [0..NumberOfTags()-1].
/*! Return 0 for tag removed. -1 for tag not found, 1 for other error and tag not removed.
 */
int IntTagged::RemoveTagIndex(int i)
{
	if (i<0 || i>=NumberOfTags()) return -1;

	list_of_tags.remove(i);
	return 0;
}

//! The tag must be an exact match.
/*! Return 0 for tag removed. -1 for tag not found, 1 for other error and tag not removed.
 */
int IntTagged::RemoveTag(int tag)
{
	int c=HasTag(tag);
	if (!c) return -1;
	list_of_tags.remove(c-1);
	return 0;
}

//! Insert tags.
/*! Returns the number of tags actually inserted. Tags already in are not included.
 */
int IntTagged::InsertTags(int *tags, int n)
{
	int nn=0;

	for (int c=0; c<n; c++) {
		int tag=tags[c];
		if (HasTag(tag)) continue;

		InsertTag(tag);
		nn++;
	}

	return nn;
}

//! Insert tag if it doesn't exist already.
/*! Will insert ONLY if the tag does not exist there.
 *
 * Returns the index of the tag.
 * 
 * If tag didn't exist, it is added, and its index is returned.
 */
int IntTagged::InsertTag(int tag)
{
	if (!tag) return 1;

	int c=HasTag(tag);
	if (c) return c-1;

	int pos=list_of_tags.n;
	if (sorttags) {
		for (pos=0; pos<list_of_tags.n; pos++) {
			if (list_of_tags.e[pos] > tag) break;
		}
	}

	list_of_tags.push(tag,pos);
	return 0;
}

void IntTagged::FlushTags()
{
	list_of_tags.flush();
}


//-------------------------------- TagCloudInfo ------------------------------------------

/*! \class TagCloudInfo
 * \brief TagCloud node to keep track of which objects have a particular tag.
 */

TagCloudInfo::TagCloudInfo(const char *t, int i)
{
	info=i;
	tag=newstr(t);
}

TagCloudInfo::~TagCloudInfo()
{
	if (tag) delete[] tag;
}


//-------------------------------- TagCloud ------------------------------------------

/*! \class TagCloud
 * \brief A tagged object manager, to make searching for sets of objects with specified tags easier.
 */

TagCloud::TagCloud()
{
	sorttags=1;
	keep_empty_tags=0;
}

/*! Return 0 for success, nonzero for error.
 */
int TagCloud::AddObject(Tagged *obj)
{
	if (!obj) return 1;
	const char *tag;
	int i;
	for (int c=0; c<obj->NumberOfTags(); c++) {
		tag=obj->GetTag(c);

		i=HasTag(tag,1);
		if (i<0) i=-i;
		if (i) i--;
		else {
			 //brand new tag
			i=InsertTag(tag,1);
			taginfo.push(new TagCloudInfo(tag),0,i);
		}

		taginfo.e[i]->objs.push(obj,0);
	}
	return 0;
}

//! Remove object from being referenced by taginfo.
/*! Return 1 for object removed, no change in tag list.
 * Return 2 for object removed, and some tags are gone now.
 * Return a negative number for some error.
 */
int TagCloud::RemoveObject(Tagged *obj)
{
	if (!obj) return -1;
	int tagsremoved=0;
	for (int c=0; c<list_of_tags.n; c++) {
		if (obj->HasTag(list_of_tags.e[c],1)) {
			taginfo.e[c]->objs.remove(taginfo.e[c]->objs.findindex(obj));
			if (taginfo.e[c]->objs.n==0 && !keep_empty_tags) {
				tagsremoved++;
				RemoveTag(c);
				taginfo.remove(c);
				c--;
			}
		}
	}
	return tagsremoved?2:1;
}

void TagCloud::FlushTags()
{
	Tagged::FlushTags();
	taginfo.flush();
}

int TagCloud::RemoveTag(const char *tag)
{
	int c=HasTag(tag,1);
	if (!c) return -1;
	Tagged::RemoveTag(tag);
	if (c<0) c=-c;
	c--;
	taginfo.remove(c);
	return 0;
}

//! Remove tag index i.
int TagCloud::RemoveTag(int i)
{
	int r;
	if ((r=Tagged::RemoveTag(i))==0) taginfo.remove(i);
	return r;
}



} //namespace Laxkit



