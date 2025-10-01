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
//    Copyright (C) 2016 by Tom Lechner
//


#include <lax/interfaces/groupdata.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/transformmath.h>
#include <lax/utf8string.h>
#include <lax/language.h>

#include <iostream>
#define DBG
using namespace std;


using namespace Laxkit;
using namespace LaxInterfaces;


namespace LaxInterfaces {



//----------------------------- GroupData ---------------------------------
/*! \class GroupData
 * \brief Holds a collection of other SomeData objects.
 */


GroupData::GroupData()
{
	//clip=NULL;
	//clip_path=wrap_path=inset_path=NULL;
	//autowrap=autoinset=0;

	parent = nullptr;
	proxy_shape = nullptr;

	//Id(); //makes this->nameid (of SomeData) be something like `whattype()`12343
}

/*! Will detach this object from any object chain. It is assumed that other objects in
 * the chain are referenced elsewhere, so the other objects in the chain are NOT deleted here.
 */
GroupData::~GroupData()
{
	if (proxy_shape) proxy_shape->dec_count();
	
	//if (clip) clip->dec_count();
	//if (clip_path) clip_path->dec_count();
	//if (wrap_path) wrap_path->dec_count();
	//if (inset_path) inset_path->dec_count();

}

/*! Set parent only if it is a GroupData. Returns new parent.
 */
SomeData *GroupData::SetParent(SomeData *newparent)
{
	parent=dynamic_cast<GroupData*>(newparent);
	return parent;
}

//! Simply return this->parent.
LaxInterfaces::SomeData *GroupData::GetParent()
{ return parent; }

/*! Returns copy with copy of kids.
 */
LaxInterfaces::SomeData *GroupData::duplicateData(LaxInterfaces::SomeData *dup)
{
	GroupData *d=dynamic_cast<GroupData*>(dup);
	if (dup && !d) return nullptr; //not a drawableobject!
	if (!dup) {
		InterfaceManager *imanager = InterfaceManager::GetDefault(true);
		dup = imanager->NewDataObject("Group");
		d = dynamic_cast<GroupData*>(dup);
	}


	 //kids
	SomeData *obj;
	GroupData *dobj;
	for (int c=0; c<kids.n; c++) {
		obj = kids.e[c]->duplicateData(nullptr);
		dobj=dynamic_cast<GroupData*>(obj);
		d->push(obj);
		dobj->parent=d;
		obj->dec_count();
	}

	return dup;
}

//! Push obj onto the stack. It's count will be incremented. (new objects only!)
/*! 
 * No previous existence
 * check is done here. For that, use pushnodup().
 */
int GroupData::push(LaxInterfaces::SomeData *obj, int where)
{
	if (!obj) return -1;
	obj->SetParent(this);
	touchContents();
	return kids.push(obj,-1,where);
}

//! Push obj onto the stack only if it is not already there.
/*! If the item is already on the stack, then its count is not
 * incremented.
 *
 * Returns -1 if the item was pushed, otherwise the index of the item in the stack.
 */
int GroupData::pushnodup(LaxInterfaces::SomeData *obj)
{
	if (!obj) return -1;
	obj->SetParent(this);
	int c=kids.pushnodup(obj,-1);
	touchContents();
	return c;
}

//! Pop d, do not decrement its count.
/*! Returns 1 for item popped, 0 for not, such as when d is not a child.
 * Sets its parent to NULL.
 */
int GroupData::popp(LaxInterfaces::SomeData *d)
{
	int c=kids.popp(d);
	if (c>=0) {
		d->SetParent(NULL);
	}
	touchContents();
	return c;
}

/*! Return the popped item. Does not change its count, except to change its parent to NULL.
 * If which out of bounds, pop the top. Else return NULL when nothing to pop.
 */
LaxInterfaces::SomeData *GroupData::pop(int which)
{
	if (which<0 || which>=kids.n) which=kids.n-1;
	if (which<0) return NULL;
	kids.e[which]->SetParent(NULL);
	return kids.pop(which);
}

//! Remove item with index i. Return 1 for item removed, 0 for not.
int GroupData::remove(int i)
{
	touchContents();
	if (i<0 || i>=kids.n) i=kids.n-1;
	kids.e[i]->SetParent(NULL);
	return kids.remove(i);
}

//! Pops item i1, then pushes it so that it is in position i2. 
/*! Return 1 for slide happened, else 0.
 *
 * Does not tinker with the object's count.
 */
int GroupData::slide(int i1,int i2)
{
	if (i1<0 || i1>=kids.n || i2<0 || i2>=kids.n) return 0;
	LaxInterfaces::SomeData *obj;
	kids.pop(obj,i1); //does nothing to count 
	kids.push(obj,-1,i2); //incs count
	obj->dec_count(); //remove the additional count
	touchContents();
	return 1;
}

//! Decrements all objects in kids vie kids.flush().
void GroupData::flush()
{
	for (int c=0; c<kids.n; c++) {
		kids.e[c]->SetParent(NULL);
	}
	kids.flush();
	touchContents();
}


//! Append all the bboxes of the objects.
void GroupData::FindBBox()
{
	maxx=minx-1; maxy=miny-1;
	if (!kids.n) return;

	for (int c=0; c<kids.n; c++) {
		addtobounds(kids.e[c]->m(),kids.e[c]);
	}
}

//! Check the point against all objs.
int GroupData::pointin(flatpoint pp,int pin)
{ 
	if (!Selectable()) return 0;
	if (!kids.n) return SomeData::pointin(pp,pin);

	flatpoint p(((pp-origin())*xaxis())/(xaxis()*xaxis()), 
		        ((pp-origin())*yaxis())/(yaxis()*yaxis()));
	for (int c=0; c<kids.n; c++) {
		if (kids.e[c]->pointin(p,pin)) return 1;
	}
	return 0;
}


//! Normally return kids.n, but return 0 if the object has locked kids.
int GroupData::NumKids()
{
	//if (SomeData::flags&(SOMEDATA_LOCK_KIDS|SOMEDATA_LOCK_CONTENTS)) return 0;
	return kids.n;
}

//! Return object with index i in stack.
/*! Note that the object's count is not changed. If the calling code wants to hang
 * on to the object they should quickly inc_count the object.
 */
LaxInterfaces::SomeData *GroupData::Child(int i)
{
	if (i<0 || i>=kids.n) return NULL;
	return kids.e[i];
}

/*! Find exact match to id. Does NOT compare to self.
 */
LaxInterfaces::SomeData *GroupData::FindChild(const char *id)
{
	GroupData *o;
	SomeData *s;
	for (int c=0; c<NumKids(); c++) {
		s=Child(c);
		if (!strcmp(s->Id(),id)) return s;

		o=dynamic_cast<GroupData*>(Child(c));
		if (!o) continue;

		if (o->kids.n) {
			s=o->FindChild(id);
			if (s) return s;
		}
	}
	return NULL;
} 

//! Find d somewhere within this (it can be kids also). Searches in kids too.
/*! if n!=NULL, then increment n each time findobj is called. So say an object
 * is in group->group->group->kids, then n gets incremented 3 times. If object
 * is in this group, then do not increment n at all.
 *
 * Return the object if it is found, otherwise NULL.
 */
LaxInterfaces::SomeData *GroupData::findobj(LaxInterfaces::SomeData *d,int *n)
{
	if (d==this) return d;
	int c;
	for (c=0; c<kids.n; c++) {
		if (kids.e[c]==d) return d;
	}
	SomeData *s;
	GroupData *g;
	for (c=0; c<kids.n; c++) {
		s=kids.e[c];
		g=dynamic_cast<GroupData*>(s);
		if (!g) continue;
		if (n) (*n)++;
		if (g->findobj(d,n)) {
			return d;
		}
		if (n) (*n)--;
	}
	return NULL;
}

//! Take all the elements in the list which, and put them in a new group at the smallest index.
/*! If any of which are not in kids, then nothing is changed. If ne<=0 then the which list
 * is assumed to be terminated by a -1.
 *
 * Return 0 for success, or nonzero error. Returns the index of the newly created group.
 */
int GroupData::GroupObjs(int ne, int *which, int *newgroupindex)
{
	if (ne<0) {
		ne=0;
		while (which[ne]>=0) ne++;
	}
	
	 // first verify that all in which are in kids
	int c;
	for (c=0; c<ne; c++) if (which[c]<0 || which[c]>=NumKids()) return 1;
	
	GroupData *g=dynamic_cast<GroupData *>(somedatafactory()->NewObject(LAX_GROUPDATA));
	if (!g) g=new GroupData;

	//g->flags|=SOMEDATA_LOCK_CONTENTS;// *** <-make this optional? makes group contents inaccessible

	int where,w[ne];
	memcpy(w,which,ne*sizeof(int));
	where=w[0];
	for (int c=1; c<ne; c++) if (where>w[c]) where=w[c]; //select lowest index

	LaxInterfaces::SomeData *d;
	while (ne) {
		d=pop(w[ne-1]); //doesn't change count
		g->push(d); //incs count
		d->dec_count();  //remove extra count
		ne--;
		for (int c2=0; c2<ne; c2++)
			if (w[c2]>w[ne]) w[c2]--;
	}

	g->FindBBox();
	g->SetParent(this);
	int index = kids.push(g,-1,where); //incs g
	g->dec_count(); //remove initial count
	FindBBox();

	if (newgroupindex) *newgroupindex = index;
	touchContents();
	return 0;
}

//! If element which is a Group, then make its elements direct elements of this, and remove the group.
/*! Return 0 for success, or nonzero error.
 */
int GroupData::UnGroup(int which)
{
	if (which<0 || which>=NumKids()) return 1;

	GroupData *g=dynamic_cast<GroupData*>(Child(which)); //assumes "Group" is a GroupData here. does not change count of g
	if (!g) return 1;

	kids.pop(which); //note does not change count of g
	
	SomeData *d;
	double mm[6];
	while (g->NumKids()) {
		d=g->pop(0); //count stays same on d
		transform_mult(mm,d->m(),g->m());
		d->m(mm);
		d->SetParent(this);
		kids.push(d,-1,which++); //incs d
		d->dec_count(); //remove extra count
	}

	g->dec_count(); //dec count of now empty group
	FindBBox();
	touchContents();
	return 0;
}

//! Ungroup some descendent of this Group.
/*! which is list of indices of subgroup. So say which=={1,3,6},
 * then ungroup the element this->1->3->6, which is a great
 * grandchild of this. All intervening elements must be Group objects
 * as must be the final object. Then the other UnGroup() is called.
 *
 * If n<=0, then which is assumed to be a -1 terminated list.
 *
 *  Return 0 for success, or nonzero error.
 */
int GroupData::UnGroup(int n,const int *which)
{
	if (n<=0) {
		n=0;
		while (which[n]!=-1) n++;
	}
	if (n==0) return 1;
	if (*which<0 || *which>=kids.n) return 2;

	SomeData *d=kids.e[*which];
	GroupData *g=dynamic_cast<GroupData*>(d);
	if (!g) return 3;

	if (n>1) return g->UnGroup(n-1,which+1);
	
	double mm[6];
	while (d=g->pop(0), d) {
		transform_mult(mm,d->m(),g->m());
		d->m(mm);
		d->SetParent(this);
		kids.push(d,-1,*which+1);
		d->dec_count();
	}
	remove(*which);
	FindBBox();
	touchContents();
	return 0;
}

const char *GroupData::Id()
{ return SomeData::Id(); }

const char *GroupData::Id(const char *newid)
{ return SomeData::Id(newid); }

//! Dump out iohints and metadata, if any.
void GroupData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	if (what==-1) {
		fprintf(f,"%sid nameofobject\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #affine transform to apply to the whole group\n",spc);
		fprintf(f,"%slocks      #combination of: contents position rotation scale shear kids selectable\n",spc); 
		fprintf(f,"%svisible    #no indicates that this group cannot be seen on screen\n",spc);
		fprintf(f,"%sselectable #whether users can select this object\n",spc);
		fprintf(f,"%stags tag1 \"tag 2\" #(optional) list of string tags\n",spc);
		fprintf(f,"%skids          #child object list...\n",spc);
		//fprintf(f,"%s    ...\n",spc);
		return;
	}

	fprintf(f,"%sid %s\n",spc,Id());

	if (NumberOfTags()) {
		char *str=GetAllTags();
		fprintf(f,"%stags %s\n",spc, str);
		delete[] str;
	}

	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				m((int)0),m(1),m(2),m(3),m(4),m(5));

	if (visible)    fprintf(f,"%svisible\n",spc);
	if (selectable) fprintf(f,"%sselectable\n",spc);
	fprintf(f,"%slocks ",spc);
	if (locks & OBJLOCK_Contents  ) fprintf(f,"contents ");
	if (locks & OBJLOCK_Position  ) fprintf(f,"position ");
	if (locks & OBJLOCK_Rotation  ) fprintf(f,"rotation ");
	if (locks & OBJLOCK_Scale     ) fprintf(f,"scale ");
	if (locks & OBJLOCK_Shear     ) fprintf(f,"shear ");
	if (locks & OBJLOCK_Kids      ) fprintf(f,"kids ");
	if (locks & OBJLOCK_Selectable) fprintf(f,"selectable ");
	fprintf(f,"\n");


	if (kids.n) {
		fprintf(f,"%skids\n",spc);
		dump_out_group(f,indent+2,what,context, true);
	}
}

//! Write out the objects.
/*! If what==-1, dump out pseudocode of file format for a group.
 *
 * if kidsonly, then only dump kid objects, not matrix, id, locks, visible.
 *
 * \todo automate object management, necessary here for what==-1
 */
void GroupData::dump_out_group(FILE *f,int indent,int what,Laxkit::DumpContext *context, bool kidsonly)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sid         #the name of a group. There can be no whitespace in the id\n",spc);
		fprintf(f,"\n%s#Groups contain any number of drawable objects. Here are all the possible such\n",spc);
		fprintf(f,"%s#objects currently installed:\n",spc);
		fprintf(f,"\n%sobject 1 Group\n%s  #...a subgroup...\n",spc,spc);
		SomeData *obj;

		InterfaceManager *imanager = InterfaceManager::GetDefault(true);
	    ResourceManager *itools = imanager->GetTools();
		ResourceType *tools = itools->FindType("tools");

		for (int c=0; tools->resources.n; c++) {
			if (!strcmp(tools->resources.e[c]->name,"Group")) continue;
			fprintf(f,"\n%sobject %s\n",spc, tools->resources.e[c]->name);
			obj = imanager->NewDataObject(tools->resources.e[c]->name);
			obj->dump_out(f,indent+2,-1,NULL);
			delete obj;
		}
		return;
	}

	if (!kidsonly) {
		fprintf(f,"%sid %s\n",spc,Id());
		fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
					m((int)0),m(1),m(2),m(3),m(4),m(5));

		if (visible)    fprintf(f,"%svisible\n",spc);
		if (selectable) fprintf(f,"%sselectable\n",spc);
		fprintf(f,"%slocks ",spc);
		if (locks & OBJLOCK_Contents  ) fprintf(f,"contents ");
		if (locks & OBJLOCK_Position  ) fprintf(f,"position ");
		if (locks & OBJLOCK_Rotation  ) fprintf(f,"rotation ");
		if (locks & OBJLOCK_Scale     ) fprintf(f,"scale ");
		if (locks & OBJLOCK_Shear     ) fprintf(f,"shear ");
		if (locks & OBJLOCK_Kids      ) fprintf(f,"kids ");
		if (locks & OBJLOCK_Selectable) fprintf(f,"selectable ");
		fprintf(f,"\n");
	}

	for (int c=0; c<kids.n; c++) {
		fprintf(f,"%sobject %d %s\n",spc,c,kids.e[c]->whattype());
		kids.e[c]->dump_out(f,indent+2,0,context);
	}
}

Laxkit::Attribute *GroupData::dump_out_group_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context, bool kidsonly)
{
	if (!att) att = new Attribute();

	Attribute *att2;

	if (what==-1) {
		att->push("id","name", "the name of a group. There can be no whitespace in the id");
		att->push(nullptr, nullptr, "Groups contain any number of drawable objects. Here are all the possible such");
		att->push(nullptr, nullptr, "objects currently installed:");
		att->push("object", "1 Group","a subgroup...");

		InterfaceManager *imanager = InterfaceManager::GetDefault(true);
	    ResourceManager *itools = imanager->GetTools();
		ResourceType *tools = itools->FindType("tools");

		SomeData *obj;
		for (int c=0; tools->resources.n; c++) {
			if (!strcmp(tools->resources.e[c]->name,"Group")) continue;
			att2 = att->pushSubAtt("object", tools->resources.e[c]->name);
			obj = imanager->NewDataObject(tools->resources.e[c]->name);
			obj->dump_out_atts(att2,-1,NULL);
			delete obj;
		}
		return att;
	}

	Utf8String s;
	if (!kidsonly) {
		att->push("id", Id());
		s.Sprintf("%.10g %.10g %.10g %.10g %.10g %.10g", m((int)0),m(1),m(2),m(3),m(4),m(5));
		att->push("matrix", s.c_str());

		if (visible) att->push("visible");
		if (selectable) att->push("selectable");

		if (locks) {
			s.SetToNone();

			if (locks & OBJLOCK_Contents  ) s.Append("contents ");
			if (locks & OBJLOCK_Position  ) s.Append("position ");
			if (locks & OBJLOCK_Rotation  ) s.Append("rotation ");
			if (locks & OBJLOCK_Scale     ) s.Append("scale ");
			if (locks & OBJLOCK_Shear     ) s.Append("shear ");
			if (locks & OBJLOCK_Kids      ) s.Append("kids ");
			if (locks & OBJLOCK_Selectable) s.Append("selectable "); 

			att->push("locks", s.c_str());
		}
	}

	for (int c=0; c<kids.n; c++) {
		s.Sprintf("%d %s", c, kids.e[c]->whattype());
		att2 = att->pushSubAtt("object", s.c_str());
		kids.e[c]->dump_out_atts(att2, 0, context);
	}

	return att;
}

void GroupData::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	 //reads in locks, visible, selectable, min/max
	SomeData::dump_in_atts(att,flag,context);


	char *name,*value; 
	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			SomeData::Id(value);

		} else if (!strcmp(name,"tags")) {
			InsertTags(value,0);

		} else if (!strcmp(name,"kids")) {
			dump_in_group_atts(att->attributes.e[c], flag,context, false);

		}
	}

	touchContents();
}

/*! Reads in kids.
 * If checksomedata, then also SomeData::dump_in_atts(att,flag,context).
 * The kids should have been flushed before coming here.
 */
void GroupData::dump_in_group_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context, bool checksomedata)
{
	if (checksomedata) SomeData::dump_in_atts(att,flag,context);

	char *name,*value;

	for (int c=0; c<att->attributes.n; c++)  {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"object")) {
			int n;
			char **strs = splitspace(value,&n);
			if (strs) {
				// could use the number as some sort of object id?
				// currently out put was like: "object 2 ImageData"
				//***strs[0]==that id
				InterfaceManager *imanager = InterfaceManager::GetDefault(true);
				SomeData *data = imanager->NewDataObject(n>1 ? strs[1] : (n==1 ? strs[0] : NULL)); //objs have 1 count

				if (data) {
					push(data);
					data->dump_in_atts(att->attributes[c],flag,context);
					DBG if (!dynamic_cast<GroupData*>(data)) cerr <<" --- WARNING! newObject returned a non-GroupData"<<endl;
					data->dec_count();
				}
				deletestrs(strs,n);

			} else {
				DBG cerr <<"*** readin blank object for Group..."<<endl;
			}

		} else { 
			//DBG cerr <<"Group dump_in:*** unknown attribute!!"<<endl;
		}
	}

	FindBBox();
}




} //namespace LaxInterfaces


