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

#include <lax/interfaces/dumpcontext.h>
#include <lax/strmanip.h>

namespace LaxInterfaces {


//------------------------------- DumpContext ---------------------------------
/*! \class DumpContext
 * \brief Class to pass to interface object dump out methods for particular behavior.
 *
 * If basedir!=NULL and saving is happening, then paths saved are relative to basedir.
 * If subs_only==0, then the saved path is always relative to basedir. If subs_only!=0,
 * then the saved path is relative to basedir ONLY if it is in basedir or a subdirectory 
 * of basedir. This makes managing project directories a little easier.
 *
 * If basedir!=NUL and loading is happening, then any relative paths encountered are
 * considered relative to basedir, or the current directory is basedir==NULL. subs_only
 * is ignored when loading.
 */

DumpContext::DumpContext()
	: basedir(NULL), subs_only(0)
{}

DumpContext::DumpContext(const char *b,char s)
{
	basedir=newstr(b);
	subs_only=s;
}

DumpContext::~DumpContext()
{
	if (basedir) delete[] basedir;
}


} //namespace LaxInterfaces
