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

#include <lax/dump.h>
#include <lax/strmanip.h>
#include <lax/iobuffer.h>

//template implementation:
#include <lax/lists.cc>

using namespace Laxkit;

namespace LaxFiles {
	
//------------------------------------ DumpUtility ---------------------------------
/*! \class DumpUtility
 * \brief Class to provide do nothing place holders for dump_out, dump_in_atts, and
 * a standard dump_in.
 *
 * These functions are used with the style of file where an items related elements
 * are listed on following lines with a certain indentation, similar to how things
 * are grouped in the Python language. The amount of indentation for the classes 
 * elements is specified in \a indent.
 * 
 * It provides abstract place holders for dump_out() and dump_in_atts(), and provides
 * a dump_in() that reads in an Attribute, and passes it to dump_in_atts().
 * 
 * Derived classes would inherit like:\n
 * <tt>class Blah : virtual public DumpUtility, public WhateverOtherClass, ...</tt>
 *
 */

/*! \fn  void DumpUtility::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext)
 * \brief what==0 means write out an Attribute formatted file.
 *
 * what==-1 should mean write out a definition snippet that represents
 * what attributes can be passed in to this object.
 */
/*! \fn  Attribute *DumpUtility::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *savecontext)
 * \brief Default is return NULL. what==0 means write out normal Attribute formatted things.
 *
 * If att==NULL, then create and return a new Attribute. Otherwise just
 * append to att.
 * 
 * \todo what==-1 should mean write out a definition snippet that represents
 *   what attributes can be passed in to this object. this would make it a snap to
 *   produce the Attribute equivalent of dtd information..
 */
/*! \fn  void DumpUtility::dump_in_atts(Attribute *att,int what,LaxFiles::DumpContext *loadcontext)
 * \brief Read the Attribute and take away what it can.
 *
 * \todo in future might have a flag somehow to remove atts that are processed (flag=1)..
 *   this will have to play nice with dump_in().
 */
/*! \fn DumpUtility::~DumpUtility()
 * \brief Empty virtual destructor.
 */
	
//! Read in a file segment as an Attribute, and pass parsing duties to dump_in_atts.
/*! The default function here ignores what (assumes it is 0).
 * Creates a new Attribute, does newatt->dump_in(f,indent), then calls
 * dump_in_atts(newatt,loadcontext). Puts the plain att in Att if Att!=NULL. Otherwise deletes the nem att.
 *
 * what==0 means f is an Attribute formatted file. Other values of what can be used by
 * subclasses to read in from other file formats, like a PathsData reading in an SVG,
 * for instance.
 *
 * loadcontext, if not NULL, will typically be something like a Laxkit::DumpContext object.
 */
void DumpUtility::dump_in(FILE *f,int indent,int what,LaxFiles::DumpContext *loadcontext,Attribute **Att)
{
	Attribute *att=new Attribute;
	att->dump_in(f,indent);
	dump_in_atts(att,0,loadcontext);
	if (Att) *Att=att;
	else delete att;
}

/*! Read in string as an attribute,  and pass parsing duties to dump_in_atts().
 * Return the created Attribute in att if not null.
 */
void DumpUtility::dump_in_str(const char *str, int what, DumpContext *context, Attribute **Att)
{
	IOBuffer buffer;
	buffer.OpenCString(str);

	Attribute *att = new Attribute;
	att->dump_in(buffer, 0);

	dump_in_atts(att,0,context);

	if (Att) *Att = att;
	else delete att;
}


//------------------------------- DumpContext ---------------------------------
/*! \class DumpContext
 * \brief Class to pass to interface object dump out methods for particular behavior.
 *
 * If basedir!=NULL and saving is happening, then paths saved are relative to basedir.
 * If subs_only==0, then the saved path is always relative to basedir. If subs_only!=0,
 * then the saved path is relative to basedir ONLY if it is in basedir or a subdirectory 
 * of basedir. This makes managing project directories a little easier.
 *
 * If basedir!=NULL and loading is happening, then any relative paths encountered are
 * considered relative to basedir, or the current directory is basedir==NULL. subs_only
 * is ignored when loading.
 */

DumpContext::DumpContext()
	: basedir(NULL), subs_only(0)
{
	what=0;
	zone=0;

	initiator=0;
	extra=NULL;

	log=NULL;
}

DumpContext::DumpContext(const char *nbasedir,char nsubs_only, unsigned long initer)
{
	basedir=newstr(nbasedir);
	subs_only=nsubs_only;
	initiator=initer;
	extra=NULL;

	what=0;
	zone=0;
	log=NULL;
}

/*! Dec count on extra if not NULL.
 */
DumpContext::~DumpContext()
{
	if (basedir) delete[] basedir;
	if (extra) extra->dec_count();
}



} // namespace LaxFiles
