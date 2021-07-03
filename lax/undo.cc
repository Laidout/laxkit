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
//    Copyright (C) 2012 by Tom Lechner
//

#include <lax/undo.h>
#include <lax/strmanip.h>

#include <sys/times.h>


#include <iostream>
using namespace std;
#define DBG


namespace Laxkit {


#define REDOABLE 2
#define UNDOABLE 1


//--------------------------------------------- Undoable ------------------------------------------
/*! \class Undoable
 * \brief Subclass from this if you want an object that can act as an agent of undoing.
 */


//--------------------------------------------- UndoData ------------------------------------------
/*! \class UndoData
 * \brief Undo node for UndoManager.
 *
 * Objects classed from this will be passed to Undoable objects to either undo or redo.
 * If direction is 1, then the item is undoable, else 2 is redoable.
 *
 * Subclasses must provide enough information to either redo OR undo, otherwise modify
 * the data to reflect what must be done after Undoable::Undo() or Redo() is called.
 */

static unsigned long getUniqueUndoId()
{
    static unsigned long uniquenumber=1;
    return uniquenumber++;
}


UndoData::UndoData(int nisauto)
{
	isauto=nisauto; //whether this undo element must exist connected to the previous undo element
	if (nisauto) undogroup=getUniqueUndoId();
	else undogroup=0;

	description=NULL;
	data=NULL;
	time=0;
	context=NULL;
	direction=UNDOABLE;
	prev=next=NULL;
}

/*! If prev==NULL, then delete next, else just remove *this from chain.
 *
 * If context is an anObject, then dec_count it.
 */
UndoData::~UndoData()
{
	if (prev) {
		 //is just a link in chain, remove from chain
		if (next) next->prev=prev;
		prev->next=next;
		prev=next=NULL;
	} else {
		 //is head of chain, remove whole chain
		if (next) {
			next->prev=NULL;
			delete next;
			next=NULL;
		}
	}

	if (dynamic_cast<anObject*>(context)) dynamic_cast<anObject*>(context)->dec_count();
	delete[] description;
}

int UndoData::isUndoable()
{ return direction==UNDOABLE; }

int UndoData::isRedoable()
{ return direction==REDOABLE; }

/*! in bytes of this whole undo instance,
 * helps keep track of size of undo stack, which if left unwatched can get quite large.
 *
 * Default is to return size of base elements. Subclasses should add on size of anything they add on.
 */
int UndoData::Size()
{
	return 4*sizeof(Undoable*) + 2*sizeof(int) + 2*sizeof(long) + (description ? strlen(description) : 0);
}



//--------------------------------------------- MetaUndoData ------------------------------------------
/*! \class MetaUndoData
 * Generic class to hold data that can be stored in an LaxFiles::Attribute.
 */

/*! Incs count of context. */
MetaUndoData::MetaUndoData(Undoable *context, int ntype, int nisauto, const char *desc)
  : UndoData(nisauto)
{
	type = ntype;
	makestr(description, desc);
	this->context = context;
	if (context && dynamic_cast<anObject*>(context)) {
		dynamic_cast<anObject*>(context)->inc_count();
	}
}

MetaUndoData::~MetaUndoData()
{
	delete[] description;
}

const char *MetaUndoData::Description()
{
	return description;
}

int MetaUndoData::Size() //in bytes of this whole undo instanc
{
	int n = (description ? strlen(description) : 0) + 4*meta.attributes.n;

	//note this is a very naive approximation.. checks only stringlen, not allocated.
	for (int c=0; c<meta.attributes.n; c++) {
		LaxFiles::Attribute *att = meta.attributes.e[c];
		n += (att->name ? strlen(att->name) : 0) + (att->value ? strlen(att->value) : 0) + (att->comment ? strlen(att->comment) : 0);
	}
	return n;
}




//--------------------------------------------- UndoManager ------------------------------------------
/*! \class UndoManager
 * \brief Simple class to keep track of undoes.
 */

	
UndoManager::UndoManager()
{
	head=current=NULL;
}

UndoManager::~UndoManager()
{
	if (head) delete head;
}

/*! Takes possession of data, and will delete it when done.
 */
int UndoManager::AddUndo(UndoData *data)
{
	//current points to an UndoData that is ready to be undone. It must have isauto==0.
	//If there are no UndoData nodes, then
	//current is NULL. In this case, if head is not NULL, then it is a list of redoables.

	data->time=times(NULL);

     //if any after current, remove them
	if (current) {
		while (current->next && current->next->isRedoable()) {
			delete current->next;
		}
	} else if (head) { delete head; head=NULL; }

    if (current) {
		current->next=data;
		data->prev=current;
		current=current->next;
	} else { 
		if (head) {
			 //head would be pointing to a redoable, but there are no undoables
			data->next=head;
			head->prev=data;
			head=data;
		} else head=current=data;
    }

	return 0;
}

//! Default is to call current->context->Undo(), and move current.
int UndoManager::Undo()
{
	if (!current) return 1;
	if (!current->context) {
		cerr <<" *** missing undo context!"<<endl;
		return 2;
	}

	int isauto;
	do {
		isauto = current->isauto;
		if (current->context->Undo(current)==0) {
			current->direction = REDOABLE;
			current=current->prev;
		} else {
			return 3;
		}
	} while (isauto);

	return 0;
}

//! Default is to call current->context->Redo(), and move current.
int UndoManager::Redo()
{
	if (!head) return 1;
	if (current && !current->next) return 2;

	if (current) current=current->next;
	else current=head;

	if (!current->context) {
		cerr <<" *** missing undo context!"<<endl;
		return 3;
	}

	if (current->context->Redo(current)==0) {
		current->direction=UNDOABLE;
		return 0;
	}
	return 4; //redo failed
}


//--------------------------------------------- UndoManager manager ------------------------------------------
static UndoManager *default_undo_manager=NULL;
static int ok_to_use_undo=true;

/*! Returns whether it was ok to undo before calling this.
 * By default undo is enabled.
 */
bool EnableUndo(bool yes)
{
	bool old=ok_to_use_undo;
	ok_to_use_undo=yes;
	return old;
}

UndoManager *GetUndoManager()
{
	if (!ok_to_use_undo) return NULL;
    if (!default_undo_manager) default_undo_manager=new UndoManager;
    return default_undo_manager;
}

/*! Any old manager will be deleted, and the newmanager pointer taken.
 */
UndoManager *SetUndoManager(UndoManager *newmanager)
{
	if (default_undo_manager) default_undo_manager->dec_count();
	default_undo_manager=newmanager;
	return default_undo_manager;
}


} //namespace Laxkit

