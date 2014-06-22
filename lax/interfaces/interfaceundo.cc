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
//    Copyright (C) 2014 by Tom Lechner
//

#include <lax/interfaces/interfaceundo.h>



#include <iostream>
using namespace std;
#define DBG 



//! Namespace for the Laxkit interfaces, surprisingly enough.
namespace LaxInterfaces {


/*! In case you want to have a custom undo manager..
 */
Laxkit::UndoManager *defaultNewInterfaceUndoer() { return new Laxkit::UndoManager(); }
static NewInterfaceUndoManagerFunc newInterfaceUndoer=defaultNewInterfaceUndoer;

/*! If func==NULL, then set to defaultNewInterfaceUndoer, which just returns
 * an ordinary Laxkit::UndoManager.
 */
void SetInterfaceUndoManagerFunc(NewInterfaceUndoManagerFunc func)
{
	newInterfaceUndoer=func;
	if (func==NULL) newInterfaceUndoer=defaultNewInterfaceUndoer;
}


/*! Whether to allow undo for the interfaces.
 */
static bool use_interface_undo=true;
static Laxkit::UndoManager *default_interface_undoer=NULL;



/*! This should be called before
 * any calls to GetIntefaceUndoManager() whenever you want to disable
 * undoing. Undoing is enabled by default.
 */
void UseInterfaceUndoManager(bool useit)
{
	use_interface_undo=useit;
}

Laxkit::UndoManager *GetInterfaceUndoManager()
{
	if (!use_interface_undo) return NULL;

	if (default_interface_undoer==NULL) default_interface_undoer = newInterfaceUndoer();
	return default_interface_undoer;
}

/*! incs count if !absorbcount.
 * If newmanager==NULL, then dec_count the old one, and set the default to null. This
 * triggers creation of a new undo manager during the next call to GetInterfaceUndoManager(),
 * and can also be used to close when exiting program.
 */
void SetInterfaceUndoManager(Laxkit::UndoManager *newmanager, bool absorbcount)
{
	if (default_interface_undoer) default_interface_undoer->dec_count();
	default_interface_undoer=newmanager;
	if (newmanager && !absorbcount) newmanager->inc_count();
}




} // namespace Laxkit

