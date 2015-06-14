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
//    Copyright (C) 2004-2006,2010,2012 by Tom Lechner
//
#ifndef _LAX_SOMEDATAREF_H
#define _LAX_SOMEDATAREF_H

#include <lax/interfaces/somedata.h>

namespace LaxInterfaces {

class SomeDataRef : virtual public SomeData
{
  public:
	SomeData *thedata;
	char *thedata_id; //can be used as a placeholder until actual object found
	int clone_group;

	SomeDataRef();
	SomeDataRef(SomeData *d);
	virtual ~SomeDataRef();
	virtual const char *whattype() { return "SomeDataRef"; }

	virtual SomeData *duplicate(SomeData *dup);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);

	virtual int Set(SomeData *d, int ignore_matrix);
	virtual SomeData *GetFinalObject();
	virtual SomeData *GetObject() { return thedata; }
	virtual void FindBBox();
	virtual int pointin(flatpoint pp,int pin=1);
};

} //namespace LaxInterfaces;

#endif

