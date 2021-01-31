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
//    Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef _LAX_ANINTERFACE_H
#define _LAX_ANINTERFACE_H

#include <lax/displayer.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/viewportwindow.h>
#include <lax/menuinfo.h>
#include <lax/buttondowninfo.h>
#include <lax/shortcuts.h>


namespace LaxInterfaces {

//! for anInterface::interface_type:
enum anInterFaceTypes {
	INTERFACE_Unknown=0,
	INTERFACE_Overlay,
	INTERFACE_Tool,
	INTERFACE_Child,
	INTERFACE_MAX
};

//! for anInterface::interface_style:
enum anInterFaceStyles {
	INTERFACE_DeferChildInput    = (1<<0),
	INTERFACE_DontSendOnModified = (1<<1),
	INTERFACE_Experimental       = (1<<2),
	INTERFACE_BITMAX
};

class ViewportWindow;
class ObjectContext;




//------------------------------ anInterface ------------------------------------------
class anInterface : virtual public Laxkit::EventReceiver,
					virtual public LaxFiles::DumpUtility
{
	char *last_message; //for PostMessage2(fmt,...)
	int last_message_n;

  protected:
	Laxkit::ButtonDownInfo buttondown;
	ViewportWindow *viewport;
	Laxkit::Displayer *dp;

	virtual void Modified(int level=0);

  public:
	char *name;
	int id;
	unsigned long interface_style;
	int interface_type;
	Laxkit::DoubleBBox bounds;

	Laxkit::anXApp *app;
	Laxkit::anXWindow *curwindow;
	anInterface *owner,*child;
	char *owner_message;

	int primary;
	int needtodraw; 

	anInterface();
	anInterface(int nid);
	anInterface(int nid,Laxkit::Displayer *ndp);
	anInterface(anInterface *nowner,int nid);
	anInterface(anInterface *nowner,int nid,Laxkit::Displayer *ndp);
	virtual ~anInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int actionnumber);

	virtual const char *IconId() { return ""; }
	virtual const char *Name() { return "An Interface"; }
	virtual const char *whattype() { return "anInterface"; }
	virtual const char *whatdatatype() = 0;
	virtual int draws(const char *atype) { return whatdatatype()!=NULL && !strcmp(whatdatatype(),atype); }
	
	virtual void Clear();
	virtual int InterfaceOn() { return 0; }
	virtual int InterfaceOff() { return 0; } 
	virtual int InitializeResources();
	virtual int RemoveChild();
	virtual int AddChild(LaxInterfaces::anInterface *ch, int absorbcount, int addbefore);

	virtual int Needtodraw() { return needtodraw; }
	virtual int Needtodraw(int n) { return needtodraw|=n; }
	
	 // return 0 if interface absorbs event, MouseMove never absorbs: must return 1;
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) { return 1; }
	virtual int MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) { return 1; }
	virtual int RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) { return 1; }
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) { return 1; }
	virtual int MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) { return 1; }
	virtual int RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) { return 1; }
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) { return 1; }
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) { return 1; }
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) { return 1; }
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d) { return 1; }
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d) { return 1; }
	virtual int DeviceChange(const Laxkit::DeviceEventData *e) { return 1; }

	virtual int Event(const Laxkit::EventData *e, const char *mes) { return 1; }
	virtual void ExposeChange(Laxkit::ScreenEventData *e) { Needtodraw(1); }
	virtual void ViewportResized() {}
	virtual void Mapped() {}
	virtual void Unmapped() {}
	virtual int PreRefresh() { return 0; } //todo, needs more thought: done for each active interface in viewport before any objects rendered
	virtual int Refresh() { needtodraw=0; return 0; }
	virtual void PostMessage(const char *message);
	virtual void PostMessage2(const char *fmt, ...);
	virtual Laxkit::UndoManager *GetUndoManager();

	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu) { return menu; }

	virtual int Paste(const char *txt,int len, Laxkit::anObject *obj, const char *formathint);
	virtual int GetForCopy(const char **txt,int *len, Laxkit::anObject **obj, const char *formathint);
	virtual int InitForCopy();

	virtual int UseThis(int id,int ndata) { return 0; }
	virtual int UseThis(Laxkit::anObject *ndata,unsigned int mask=0) { return 0; } 
	virtual int UseThisObject(ObjectContext *oc) { return 0; } 
	virtual int DrawData(Laxkit::anObject *ndata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0) { return 1; }

	virtual Laxkit::anXWindow *CurrentWindow(Laxkit::anXWindow *ncur);

	 //--- SomeData and Viewport oriented functions:
	virtual void Clear(SomeData *d) = 0;
	virtual ObjectContext *Context();
	virtual void Dp(Laxkit::Displayer *ndp);
	virtual int DrawDataDp(Laxkit::Displayer *tdp,SomeData *tdata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=1);

	 // if owning window can be cast to ViewportWindow, then these call corresponding funcs in it:
	virtual flatpoint realtoscreen(flatpoint r);
	virtual flatpoint screentoreal(int x,int y);
	virtual double Getmag(int c=0);
	virtual double GetVMag(int x,int y);

	virtual double UIScale();
	virtual void ThemeChange(Laxkit::Theme *theme);
	virtual void UIScaleChange();
	virtual double ScreenLine();
	virtual double NearThreshhold();
	virtual double NearThreshhold2();
	virtual double DraggedThreshhold();

	 //input and output
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);

};


} // namespace LaxInterfaces


#endif

