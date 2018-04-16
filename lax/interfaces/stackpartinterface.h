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
//    Copyright (C) 2018 by Tom Lechner
//
#ifndef _LAX_STACKPARTINTERFACE_H
#define _LAX_STACKPARTINTERFACE_H


#include <lax/interfaces/aninterface.h>
#include <lax/rectangles.h>

namespace LaxInterfaces { 


//--------------------------- StackParts -------------------------------------

class StackParts : public Laxkit::anObject, public Laxkit::DoubleRectangle
{
  public:
	class StackNode : public DoubleRectangle
	{
	  public:
		int id;
		char *text;
		Laxkit::LaxImage *icon;
		Laxkit::anObject *extra;

		StackNode(const char *ntext, Laxkit::LaxImage *nicon, int nid, Laxkit::anObject *nextra, int absorb);
		virtual ~StackNode();
	};

	enum StackPartsStyle { //default style is just text strings
		Rounded  = (1<<0), //show parts in rounded boxes
		Vertical = (1<<1), //default is horizontal layout
		Numbered = (1<<2), //prepend with numbers "(1) Blah"
		MAX
	};


	Laxkit::PtrStack<StackNode> stack;
	char *delimiter;

	int current;

	int style; //0 is rounded rects
	Laxkit::LaxFont *font;
	double padding;
	int display_type; //0 for screen, 1 for real
	int display_gravity; //LAX_TOP, ... hint for default positioning of block

	Laxkit::ScreenColor fg, bg;
	double mo_diff;
	double cur_diff;

	StackParts();
	virtual ~StackParts();

	virtual int SetPath(const char *path, const char *delim="/");
	virtual int SetPathPart(const char *text, int which);
	virtual char *GetPath(const char *delim="/");
	virtual int Add(const char *ntext, Laxkit::LaxImage *nicon, int nid, Laxkit::anObject *nextra, int absorb, int where);
	virtual int Remove(int index);
	virtual int Pop(int index, char **text_ret);

	virtual StackNode *Current();
	virtual int Current(int which);
	virtual int n();
	virtual StackNode *e(int which);
};


//--------------------------- StackPartInterface -------------------------------------

class StackPartInterface : public anInterface
{
  protected:
	int showdecs;

	Laxkit::ShortcutHandler *sc;

	StackParts *data;

	int hover;
	virtual int scan(int x, int y, unsigned int state);

	virtual int send();

  public:
	enum StackPartActions {
		STACK_None = 0,
		STACK_Rename,
		STACK_MAX
	};

	unsigned int interface_flags;

	StackPartInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~StackPartInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "StackParts"; }
	virtual const char *Name();
	virtual const char *whattype() { return "StackPartInterface"; }
	virtual const char *whatdatatype();
	virtual ObjectContext *Context(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int info);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();
};

} // namespace LaxInterfaces

#endif

