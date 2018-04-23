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
//    Copyright (C) 2018 by Tom Lechner
//
#ifndef _LAX_BOILERPLATEINTERFACE_H
#define _LAX_BOILERPLATEINTERFACE_H

#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 


//--------------------------- BoilerPlateData -------------------------------------

class BoilerPlateData : virtual public SomeData
{
  public:
	BoilerPlateData();
	virtual ~BoilerPlateData();
};


//--------------------------- BoilerPlateInterface -------------------------------------

class BoilerPlateInterface : public anInterface
{
  protected:
	int showdecs;

	Laxkit::ShortcutHandler *sc;

	BoilerPlateData *data; //points to dataoc->obj
	ObjectContext *dataoc;

	int hover;
	virtual int scan(int x, int y, unsigned int state);
	virtual int OtherObjectCheck(int x,int y,unsigned int state);

	virtual int send();

  public:
	enum BoilerPlateActions {
		BOILERPLATE_None=0,
		BOILERPLATE_Something,
		BOILERPLATE_MAX
	};

	unsigned int interface_flags;

	BoilerPlateInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~BoilerPlateInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "BoilerPlate"; }
	virtual const char *Name();
	virtual const char *whattype() { return "BoilerPlateInterface"; }
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
	virtual int MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();
};

} // namespace LaxInterfaces

#endif

