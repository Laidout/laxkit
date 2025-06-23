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
#ifndef _LAX_UNDO_H
#define _LAX_UNDO_H


#include <lax/anobject.h>
#include <lax/attributes.h>
#include <cstdlib>

namespace Laxkit {

class UndoData;

//--------------------------------------------- Undoable ------------------------------------------
class Undoable
{
  public:
	virtual int Undo(UndoData *data) = 0; // return 0 for sucess, nonzero error
	virtual int Redo(UndoData *data) = 0; // return 0 for sucess, nonzero error
};


//--------------------------------------------- UndoData ------------------------------------------
class UndoData
{
  public:
    Undoable *context;
	unsigned long undogroup; // automatically assigned. 0 for continues previous
	unsigned long undo_id;   // automatically assigned
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
	virtual int GetUndoPosition();
	virtual int Size(); //in bytes of this whole undo instance
};


//--------------------------------------------- MetaUndoData ------------------------------------------
class MetaUndoData : public UndoData
{
  public:
    int type;
    Attribute meta;
    char *description;

    MetaUndoData(Undoable *context, int ntype, int nisauto, const char *desc);
    virtual ~MetaUndoData();
    virtual const char *Description();
    virtual int Size(); //in bytes of this whole undo instance
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
bool EnableUndo(bool yes); //by default it is enabled
UndoManager *GetUndoManager();
UndoManager *SetUndoManager(UndoManager *newmanager);


} //namespace Laxkit


#endif

