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
//    Copyright (C) 2007,2010 by Tom Lechner
//
#ifndef _LAX_DUMPCONTEXT_H
#define _LAX_DUMPCONTEXT_H

#include <lax/anobject.h>

namespace LaxInterfaces {

//------------------------------- DumpContext ---------------------------------
class DumpContext : public Laxkit::anObject
{
 public:
	char *basedir;
	char subs_only;
	DumpContext();
	DumpContext(const char *b,char s);
	virtual ~DumpContext();
};

} // namespace LaxInterfaces

#endif

