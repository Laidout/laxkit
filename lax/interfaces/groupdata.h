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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_GROUPDATA_H
#define _LAX_GROUPDATA_H


#include <lax/tagged.h>
#include <lax/refptrstack.h>
#include <lax/interfaces/somedata.h>



namespace LaxInterfaces {



//----------------------------- GroupData ---------------------------------

/*! Used in GroupData::GetAnchor().
 * Custom anchors must have a value of ANCHOR_MAX or greater.
 */
enum BBoxAnchorTypes {
	BBOXANCHOR_ul=1, //devs: do not change this value!!
	BBOXANCHOR_um,
	BBOXANCHOR_ur,
	BBOXANCHOR_ml,
	BBOXANCHOR_mm,
	BBOXANCHOR_mr,
	BBOXANCHOR_ll,
	BBOXANCHOR_lm,
	BBOXANCHOR_lr,
	BBOXANCHOR_MAX
};

/*! for GroupData::locks */
enum GroupDataLockTypes {
	OBJLOCK_Contents   = (1<<0),
	OBJLOCK_Position   = (1<<1),
	OBJLOCK_Rotation   = (1<<2),
	OBJLOCK_Scale      = (1<<3),
	OBJLOCK_Shear      = (1<<4),
	OBJLOCK_Kids       = (1<<5),
	OBJLOCK_Selectable = (1<<6)
};

class GroupData :   virtual public Laxkit::Tagged,
					virtual public LaxInterfaces::SomeData
{
 protected:
 public:
	SomeData *parent;

	int locks; //lock object contents|matrix|rotation|shear|scale|kids|selectable
	char locked, visible, prints, selectable;

	//SomeData *clip; //If not a PathsData, then is an object for a softmask
	//LaxInterfaces::PathsData *clip_path;
	//LaxInterfaces::PathsData *wrap_path;
	//LaxInterfaces::PathsData *inset_path;
	//double autowrap, autoinset; //distance away from default to put the paths when auto generated
	//int wraptype;


	//GroupData(LaxInterfaces::SomeData *refobj=NULL);
	GroupData();
	virtual ~GroupData();
	virtual const char *whattype() { return "Group"; }
	virtual LaxInterfaces::SomeData *duplicate(LaxInterfaces::SomeData *dup);
	virtual const char *Id();
	virtual const char *Id(const char *newid);

	 //sub classes MUST redefine pointin() and FindBBox() to point to the proper things.
	 //default is point to things particular to Groups.
	virtual int pointin(flatpoint pp,int pin=1);
	virtual void FindBBox();
	virtual LaxInterfaces::SomeData *GetParent();
	virtual SomeData *SetParent(SomeData *newparent);

	virtual int Selectable();
	virtual int Visible();
	virtual int IsLocked(int which);
	virtual void Lock(int which);
	virtual void Unlock(int which);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_out_group(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual void dump_in_group_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	
	 //new functions for GroupData
	//virtual LaxInterfaces::SomeData *EquivalentObject();
	//virtual LaxInterfaces::PathsData *GetAreaPath();
	//virtual LaxInterfaces::PathsData *GetInsetPath(); //return an inset path, may or may not be inset_path, where streams are laid into
	//virtual LaxInterfaces::PathsData *GetWrapPath(); //path inside which external streams can't go

	 //Group specific functions:
	Laxkit::RefPtrStack<LaxInterfaces::SomeData> kids;

	virtual int NumKids();
	virtual SomeData *Child(int which);
	virtual LaxInterfaces::SomeData *FindChild(const char *id);
	virtual LaxInterfaces::SomeData *findobj(LaxInterfaces::SomeData *d,int *n=NULL);
	virtual int findindex(LaxInterfaces::SomeData *d) { return kids.findindex(d); }
	virtual int push(LaxInterfaces::SomeData *obj);
	virtual int pushnodup(LaxInterfaces::SomeData *obj);
	virtual int remove(int i);
	virtual LaxInterfaces::SomeData *pop(int which);
	virtual int popp(LaxInterfaces::SomeData *d);
	virtual void flush();
	virtual void swap(int i1,int i2) { kids.swap(i1,i2); }
	virtual int slide(int i1,int i2);

	virtual int GroupObjs(int n, int *which);
	virtual int UnGroup(int which);
	virtual int UnGroup(int n,const int *which);
	
};


} //namespace LaxInterfaces

#endif

