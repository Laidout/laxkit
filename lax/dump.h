//
//	
//    The Laxkit, a windowing toolkit
//    Copyright (C) 2004-2006 by Tom Lechner
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
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
#ifndef _LAX_DUMP_H
#define _LAX_DUMP_H

#include <cstdio>
#include <lax/anobject.h>
#include <lax/attributes.h>
#include <lax/laxdefs.h>

namespace LaxFiles {

//------------------------------- DumpUtility ---------------------------------
class DumpUtility
{
 public:
	virtual void       dump_out(FILE *f,int indent,int what,Laxkit::anObject *savecontext) =0;
	virtual Attribute *dump_out_atts(Attribute *att,int what,Laxkit::anObject *savecontext) { return NULL; }

	virtual void dump_in (FILE *f,int indent,int what,Laxkit::anObject *loadcontext,Attribute **att);
	virtual void dump_in_atts(Attribute *att,int flag,Laxkit::anObject *loadcontext) =0;

	virtual ~DumpUtility() {}
};


//------------------------------- DumpContext ---------------------------------
class DumpContext : public Laxkit::anObject
{
 public:
	char *basedir;
	char subs_only;
	unsigned long initiator;

	DumpContext();
	DumpContext(const char *b,char s, unsigned long initer);
	virtual ~DumpContext();
};


} // namespace LaxFiles

#endif


