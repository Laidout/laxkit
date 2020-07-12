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
//    Copyright (C) 2004-2007,2010-2012 by Tom Lechner
//
#ifndef _LAX_OBJECTINTERFACE_H
#define _LAX_OBJECTINTERFACE_H

#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/selection.h>
#include <lax/lists.h>


namespace LaxInterfaces {


//-----------------------------

enum ObjecInterfaceStyles {
	OBJECTI_Select_Touching   = (1<<0),
	OBJECTI_Private_Selection = (1<<1),
	OBJECTI_MAX
};

enum ObjectInterfaceActions {
	OIA_Group = RIA_MAX,
	OIA_Ungroup,
	OIA_ToggleGroup,
	OIA_Delete,
	OIA_MAX
};

class ObjectInterface : public RectInterface
{
  protected:
	 //remember dragmode&1 means don't show arrow handles
	enum MoreDragMode {
		DRAG_NEW_SELECTION = DRAG_MAX,
		DRAG_ADD_SELECTION,
		DRAG_SUBTRACT_SELECTION,
		DRAG_OBJI_MAX
	};

	Selection *selection;
	int dontclear;
	virtual void Flip(int type);
	virtual void Rotate(double angle);
	virtual int PerformAction(int action);

	 //undo related:
	Laxkit::PtrStack<SomeData> initial;
	virtual void UpdateInitial();
	virtual int InstallTransformUndo();

  public:
	ObjectInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~ObjectInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual ObjectContext *Context() { return NULL; }
	virtual const char *IconId() { return "Object"; }
	virtual const char *Name();
	virtual const char *whattype() { return "ObjectInterface"; }
	virtual const char *whatdatatype() { return "Selection"; }
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual anInterface *duplicate(anInterface *dup);
	virtual void deletedata();
	virtual void Clear(SomeData *d=NULL);
	virtual int UseThis(anObject *newdata,unsigned int);
    virtual int UseThisObject(ObjectContext *oc);
	virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	virtual void TransformSelection(const double *N, int s=-1, int e=-1);
	virtual int PointInSelection(int x,int y);
	virtual int AddToSelection(ObjectContext *oc);
	virtual int AddToSelection(Laxkit::PtrStack<ObjectContext> &nselection);
	virtual int AddToSelection(Selection *nselection);
	virtual int FreeSelection();
	virtual void RemapBounds();
	virtual int GrabSelection(unsigned int state);
	virtual int ToggleGroup();
	virtual int GroupObjects();
	virtual int UngroupObjects();
	//virtual void RedoBounds();
};

} // namespace LaxInterfaces

#endif

