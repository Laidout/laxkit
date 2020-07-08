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
//    Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef _LAX_SOMEDATA_H
#define _LAX_SOMEDATA_H

#include <lax/anobject.h>
#include <lax/doublebbox.h>
#include <lax/vectors.h>
#include <lax/lists.h>
#include <lax/attributes.h>
#include <lax/dump.h>
#include <lax/laximages.h>
#include <lax/transformmath.h>
#include <lax/resources.h>
#include <lax/previewable.h>
#include <lax/undo.h>

#include <cstdio>
#include <ctime>

#ifndef NULL
#define NULL 0
#endif

using namespace std;

namespace LaxInterfaces {


//for SomeData::flags:
enum SomeDataFlags {
	SOMEDATA_KEEP_ASPECT    = (1<<0),
	SOMEDATA_KEEP_1_TO_1    = (1<<1),
	SOMEDATA_LOCK_SHEAR     = (1<<2),
	SOMEDATA_LOCK_ROTATION  = (1<<3),
	SOMEDATA_LOCK_SCALEX    = (1<<4),
	SOMEDATA_LOCK_SCALEY    = (1<<5),
	SOMEDATA_LOCK_POSITION  = (1<<6),
 //only object tool can touch it
	SOMEDATA_LOCK_CONTENTS  = (1<<7),
 //child objects are not selectable
	SOMEDATA_LOCK_KIDS      = (1<<8),
	SOMEDATA_UNSELECTABLE   = (1<<9),
	SOMEDATA_UNEDITABLE     = (1<<10),
	SOMEDATA_FLAGS_MAX      = 10
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


class SomeData :  virtual public Laxkit::Resourceable,
				  virtual public Laxkit::Affine,
				  virtual public Laxkit::DoubleBBox,
				  virtual public Laxkit::Undoable,
				  virtual public Laxkit::Previewable
{
  protected:

  public:
	 //preview mechanism
	int usepreview;

	virtual Laxkit::LaxImage *GetPreview();
	virtual int GeneratePreview(int w,int h);
	virtual int renderToBufferImage(Laxkit::LaxImage *image);
	virtual int renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels);

	int modified; //hint for what has been modified
	virtual void touchContents();
	virtual int IsModified()     { return modified; }
	virtual void Modified()      { modified |= 1; touchContents(); }
	virtual void ChildModified() { modified |= 2; touchContents(); }
	virtual void ClearModified() { modified = 0; }

	int locks; //lock object contents|matrix|position|rotation|shear|scale|kids|selectable
	bool visible;
	bool selectable;
	int bboxstyle; //useparent
	flatpoint centerpoint; //used as a passive center by ObjectInterface
	unsigned int flags;
	int iid; // interface id

	char *nameid;
	virtual const char *Id();
	virtual const char *Id(const char *newid);

	SomeData();
	SomeData(double nminx,double nmaxx,double nminy,double nmaxy);
	virtual ~SomeData();
	virtual const char *whattype() { return "SomeData"; }
	
	virtual int Selectable();
	virtual int Visible();
	virtual int IsLocked(int which);
	virtual void Lock(int which);
	virtual void Unlock(int which);

	virtual void FindBBox() {}
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual flatpoint BBoxPoint(double x,double y, bool transform_to_parent);
	virtual flatpoint ReferencePoint(int which, bool transform_to_parent);
	virtual SomeData *duplicate(SomeData *dup);
	virtual int pointin(flatpoint pp,int pin=1); // return in=1 | on=2 | out=0, default is pointin bbox
	virtual int fitto(double *boxm,DoubleBBox *box,double alignx,double aligny, int whentoscale=2);
	virtual SomeData *GetParent() { return NULL; }
	virtual SomeData *SetParent(SomeData *newparent);
	virtual SomeData *FindCommonParent(SomeData *other);
	virtual anObject *ObjectOwner() { return GetParent(); }
	virtual Laxkit::Affine GetTransformToContext(bool invert, int partial);
	virtual int NestedDepth();

	virtual void FlipH();
	virtual void FlipV();

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in(FILE *f,int indent,LaxFiles::DumpContext *context,LaxFiles::Attribute **Att=NULL);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);

    virtual int Undo(Laxkit::UndoData *data);
    virtual int Redo(Laxkit::UndoData *data);

	virtual int NumResources();
	virtual int ResourceInfo(int index, Laxkit::anObject **resource_ret);
};


   
//------------------------- SomeDataUndo -------------------------------

class SomeDataUndo : public Laxkit::UndoData
{
  public:
	enum SDUndoTypes {
		SDUNDO_Bounds   =(1<<0),
		SDUNDO_Transform=(1<<1),
		SDUNDO_Shift    =(1<<2),
		SDUNDO_Rotation =(1<<3),
		SDUNDO_Scale    =(1<<4),
		SDUNDO_Shear    =(1<<5),
		SDUNDO_Flip     =(1<<6),
		SDUNDO_MAX
	};

	int type;
	Laxkit::Affine m, m_orig;
	Laxkit::DoubleBBox box, box_orig;

	SomeDataUndo(SomeData *object,
			     Laxkit::Affine *mo, Laxkit::DoubleBBox *boxo,
			     Laxkit::Affine *nm, Laxkit::DoubleBBox *nbox,
			     int ntype, int nisauto);
	virtual const char *Description();
};



} // namespace LaxInterfaces

#endif

