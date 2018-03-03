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
//    Copyright (C) 2014 by Tom Lechner
//

#include <lax/interfaces/selection.h>
#include <lax/interfaces/viewportwindow.h>

#include <lax/lists.cc>

namespace LaxInterfaces {

//--------------------------- SelectedObject -------------------------

/*! \class SelectedObject
 */

/*! This will add a duplicate of noc.
 */
SelectedObject::SelectedObject(LaxInterfaces::ObjectContext *noc, int ninfo)
{
	info=ninfo;
	oc=noc->duplicate();
	//if (oc) obj=oc->obj;
}

SelectedObject::~SelectedObject()
{
	if (oc) delete oc;
}




//--------------------------- Selection -------------------------

/*! \class Selection
 */

Selection::Selection()
{
	currentobject=-1;
	base_object=NULL;
}

Selection::~Selection()
{
	if (base_object) base_object->dec_count();
}

//! Return a duplicate of this selection.
/*! Will not copy properties.
 */
Selection *Selection::duplicate()
{
	Selection *s=new Selection();
	for (int c=0; c<objects.n; c++) {
		s->Add(objects.e[c]->oc,-1,objects.e[c]->info);
	}
	return s;
}

LaxInterfaces::ObjectContext *Selection::CurrentObject()
{
	if (currentobject<0) return NULL;
	return objects.e[currentobject]->oc;
}

/*! Set the current object to be this one. Set to top if which out of bounds.
 */
void Selection::CurrentObject(int which)
{
	if (which<0 || which>=objects.n) which=objects.n-1;
	currentobject=which;
}

///*! Return the highest common ancestor of all the selected objects.
// * If oc==NULL, then return a new ObjectContext. Otherwise, modify oc
// * to contain the context.
// */
//ObjectContext *Selection::CommonAncestor(ObjectContext *oc)
//{
//}


/*! Return index of added oc in selection. Warning: does NOT check for previous 
 * existence of same path!
 * A duplicate of oc is added.
 */
int Selection::Add(LaxInterfaces::ObjectContext *oc, int where, int ninfo)
{
	if (!oc) return -1;
	if (where<0 || where>objects.n) where=objects.n;
	currentobject=where;
	return objects.push(new SelectedObject(oc,ninfo),-1,where);
}

/*! Like Add(), but do not add if is already in selection. In that case,
 * return -1. Return -2 if oc==NULL.
 */
int Selection::AddNoDup(LaxInterfaces::ObjectContext *oc, int where, int ninfo)
{
	if (!oc) return -2;
	if (where<0 || where>objects.n) where=objects.n;

	for (int c=0; c<objects.n; c++) {
		if (oc->isequal(objects.e[c]->oc)) return -1;
	}

	currentobject=where;
	return objects.push(new SelectedObject(oc,ninfo),-1,where);
}

/*! Remove item at index i.
 */
int Selection::Remove(int i)
{
	if (i<0 || i>=objects.n) return 1;
	int c=objects.remove(i);
	if (currentobject==i) currentobject=i-1;
	if (currentobject<0) currentobject=objects.n-1;
	return c;
}

/*! Pop and return item at index i.
 */
ObjectContext *Selection::Pop(int i)
{
	if (i<0 || i>=objects.n) return NULL;
	SelectedObject *soc = objects.pop(i);
	ObjectContext *oc = soc->oc;
	soc->oc=NULL;
	delete soc;
	if (currentobject==i) currentobject=i-1;
	if (currentobject<0) currentobject=objects.n-1;
	return oc;
}


void Selection::Flush()
{
	currentobject=-1;
	objects.flush();
}


int Selection::FindIndex(LaxInterfaces::ObjectContext *oc)
{
	for (int c=0; c<objects.n; c++) {
		if (oc->isequal(objects.e[c]->oc)) return c;
	}
	return -1;
}

LaxInterfaces::ObjectContext *Selection::e(int i)
{
	if (i<0 || i>=objects.n) return NULL;
	return objects.e[i]->oc;
}

//ValueHash *Selection::e_properties(int i)
//{
//	if (i<0 || i>=objects.n) return NULL;
//	return &objects.e[i]->properties;
//}

/*! Get the info value for the element.
 * Return is -1 if out of bounds.
 */
int Selection::e_info(int i)
{
	if (i<0 || i>=objects.n) return -1;
	return objects.e[i]->info;
}

/*! Set the object's info to newinfo.
 * Return is -1 if out of bounds, else just return newinfo.
 */
int Selection::e_info(int i, int newinfo)
{
	if (i<0 || i>=objects.n) return -1;
	objects.e[i]->info = newinfo;
	return objects.e[i]->info;
}

/*! Return the index corresponding to object, or -1 if object not in selection.
 */
int Selection::ObjectIndex(SomeData *object)
{
	for (int c=0; c<n(); c++) {
		if (object==e(c)->obj) return c;
	}

	return -1;
}

// *** complicated because currently objectcontext needs viewport to retrieve transforms
//void Selection::FindBBox()
//{
//}


} //namespace LaxInterfaces

