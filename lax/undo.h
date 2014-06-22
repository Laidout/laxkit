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
//    Copyright (C) 2012 by Tom Lechner
//
#ifndef _LAX_UNDO_H
#define _LAX_UNDO_H


#include <lax/anobject.h>
#include <cstdlib>

namespace Laxkit {

class UndoData;

//--------------------------------------------- Undoable ------------------------------------------
class Undoable
{
  public:
	virtual int Undo(UndoData *data) = 0;
	virtual int Redo(UndoData *data) = 0;
};


//--------------------------------------------- UndoData ------------------------------------------
class UndoData
{
  public:
    Undoable *context;
	unsigned long undogroup;
	char *description;
    clock_t time;
    int direction;
    int isauto;
    UndoData *prev, *next;

	anObject *data;

    UndoData(int nisauto=0);
    virtual ~UndoData();
    virtual int isUndoable();
    virtual int isRedoable();
    virtual const char *Description() = 0;
	virtual const char *Script() { return NULL; }
};

//--------------------------------------------- UndoManager ------------------------------------------
class UndoManager : public anObject
{
  protected:
    UndoData *head;
    UndoData *current; //points to either the current undoable, or NULL. There may be redoable ones in head!
  public:
	UndoManager();
	virtual ~UndoManager();
	virtual const char *whattype() { return "UndoManager"; }
    virtual int AddUndo(UndoData *data);

	virtual int Undo();
	virtual int Redo();
};



//--------------------------------------------- Default UndoManager ------------------------------------------
UndoManager *GetUndoManager();
UndoManager *SetUndoManager(UndoManager *newmanager);


} //namespace Laxkit


#endif

