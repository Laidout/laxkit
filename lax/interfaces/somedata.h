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
#include <lax/undo.h>

#include <cstdio>
#include <ctime>

#ifndef NULL
#define NULL 0
#endif

using namespace std;

namespace LaxInterfaces {

#define SOMEDATA_KEEP_ASPECT    (1<<0)
#define SOMEDATA_KEEP_1_TO_1    (1<<1)
#define SOMEDATA_LOCK_SHEAR     (1<<2)
#define SOMEDATA_LOCK_ROTATION  (1<<3)
#define SOMEDATA_LOCK_SCALEX    (1<<4)
#define SOMEDATA_LOCK_SCALEY    (1<<5)
#define SOMEDATA_LOCK_POSITION  (1<<6)
 //only object tool can touch it
#define SOMEDATA_LOCK_CONTENTS  (1<<7)
 //child objects are not selectable
#define SOMEDATA_LOCK_KIDS      (1<<8)
#define SOMEDATA_UNSELECTABLE   (1<<9)
#define SOMEDATA_UNEDITABLE     (1<<10)




class SomeData :  virtual public Laxkit::Resourceable,
				  virtual public Laxkit::Affine,
				  virtual public Laxkit::DoubleBBox,
				  virtual public Laxkit::Undoable
{
  protected:
  public:
	 //preview mechanism
	Laxkit::LaxImage *preview;
	int usepreview;

	std::time_t previewtime;
	std::time_t modtime;
	virtual void touchContents();

	virtual Laxkit::LaxImage *GetPreview();
	virtual void GeneratePreview(int w,int h);
	virtual int renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels);
	virtual int renderToBufferImage(Laxkit::LaxImage *image);

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
	
	virtual void FindBBox() {}
	virtual flatpoint BBoxPoint(double x,double y, bool transform_to_parent);
	virtual flatpoint ReferencePoint(int which, bool transform_to_parent);
	virtual SomeData *duplicate(SomeData *dup);
	virtual int pointin(flatpoint pp,int pin=1); // return in=1 | on=2 | out=0, default is pointin bbox
	virtual int fitto(double *boxm,DoubleBBox *box,double alignx,double aligny, int whentoscale=2);
	virtual SomeData *GetParent() { return NULL; }
	virtual anObject *ObjectOwner() { return GetParent(); }
	virtual Laxkit::Affine GetTransformToContext(bool invert, int partial);
	virtual int NestedDepth();

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

