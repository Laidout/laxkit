//
//	
//    The Laxkit, a windowing toolkit
//    Copyright (C) 2004-2006 by Tom Lechner
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
//    Please consult https://github.com/Laidout/laxkit about where to send any
//    correspondence about this software.
//
#ifndef _LAX_DUMP_H
#define _LAX_DUMP_H

#include <cstdio>
#include <lax/anobject.h>
#include <lax/attributes.h>
#include <lax/laxdefs.h>
#include <lax/errorlog.h>

namespace Laxkit {


//------------------------------- DumpContext ---------------------------------
class DumpContext : public Laxkit::anObject
{
 public:
	int what;
	int zone; //app dependent, like 0 for document, 1 for project, 2 for component (for instance)
	unsigned long initiator; //object_id of top initiating object

	char *basedir;
	bool subs_only;
	bool render_proxies = false; //when a group has a proxy_shape defined
	Laxkit::anObject *extra;

	Laxkit::ErrorLog *log;

	DumpContext();
	DumpContext(const char *nbasedir, bool nsubs_only, unsigned long initer);
	virtual ~DumpContext();
};


//------------------------------- DumpUtility ---------------------------------
class DumpUtility
{
 public:
	virtual void       dump_out(FILE *f,int indent,int what,DumpContext *context) =0;
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context) { return NULL; }

	virtual void dump_in(FILE *f, int indent, int what, DumpContext *context, Attribute **att);
	virtual void dump_in_str(const char *str, int what, DumpContext *context, Attribute **att);
	virtual void dump_in_atts(Attribute *att, int what, DumpContext *context) = 0;

	virtual ~DumpUtility() {}
};



} // namespace

#endif


