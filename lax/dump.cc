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

#include <lax/dump.h>

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

/*! \fn  void DumpUtility::dump_out(FILE *f,int indent,int what,anObject *savecontext)
 * \brief what==0 means write out an Attribute formatted file.
 *
 * what==-1 should mean write out a definition snippet that represents
 * what attributes can be passed in to this object.
 */
/*! \fn  Attribute *DumpUtility::dump_out_atts(Attribute *att,int what,anObject *savecontext)
 * \brief Default is return NULL. what==0 means write out normal Attribute formatted things.
 *
 * If att==NULL, then create and return a new Attribute. Otherwise just
 * append to att.
 * 
 * \todo what==-1 should mean write out a definition snippet that represents
 *   what attributes can be passed in to this object. this would make it a snap to
 *   produce the Attribute equivalent of dtd information..
 */
/*! \fn  void DumpUtility::dump_in_atts(Attribute *att,int flag,anObject *loadcontext)
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
 * loadcontext, if not NULL, will typically be something like a LaxInterfaces::LoadContext object.
 */
void DumpUtility::dump_in(FILE *f,int indent,int what,anObject *loadcontext,Attribute **Att)
{
	Attribute *att=new Attribute;
	att->dump_in(f,indent);
	dump_in_atts(att,0,loadcontext);
	if (Att) *Att=att;
	else delete att;
}

} // namespace LaxFiles
