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
//    Copyright (C) 2004-2011 by Tom Lechner
//
#ifndef _LAX_VIEWPORTWINDOW_H
#define _LAX_VIEWPORTWINDOW_H

#include <lax/anxapp.h>
#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/viewportwindow.h>
#include <lax/interfaces/selection.h>
#include <lax/rulerwin.h>
#include <lax/scroller.h>
#include <lax/panuser.h>
#include <lax/shortcuts.h>
#include <lax/buttondowninfo.h>


namespace LaxInterfaces {

//forward declarations
class anInterface;
//class Selection;


#define VIEWPORT_NO_XSCROLLER    (1<<16)
#define VIEWPORT_NO_YSCROLLER    (1<<17)
#define VIEWPORT_NO_SCROLLERS    ((1<<16)|(1<<17))
#define VIEWPORT_NO_XRULER       (1<<18)
#define VIEWPORT_NO_YRULER       (1<<19)
#define VIEWPORT_NO_RULERS       ((1<<18)|(1<<19))
#define VIEWPORT_ROTATABLE       (1<<20)
#define VIEWPORT_BACK_BUFFER     (1<<21)
#define VIEWPORT_RIGHT_HANDED    (1<<22)
#define VIEWPORT_NO_ZOOM_MENU    (1<<23)
#define VIEWPORT_STYLE_MASK      (0xffff0000)

//---------------------------- ObjectContext -----------------------
class ObjectContext
{
  public:
	int i;
	SomeData *obj;
	ObjectContext();
	ObjectContext(int ii, SomeData *o);
	virtual ~ObjectContext();
	virtual int isequal(const ObjectContext *oc) { return i==oc->i; }
	virtual void SetObject(SomeData *o);
	virtual int Set(ObjectContext *oc);
	virtual void set(int ii, SomeData *o);
	virtual void clear();
	virtual int Up() { return 0; } //select up, 0 for fail, 1 for ok
	virtual ObjectContext *duplicate();
};

class SomeDataNewDeleteUndo : public Laxkit::UndoData
{
  public:
  	bool for_new; // this class is used for both new and delete, according to this flag
  	ObjectContext *oc;

  	SomeDataNewDeleteUndo(ObjectContext *noc, bool is_for_new);
  	virtual ~SomeDataNewDeleteUndo();
  	virtual const char *Description();
};


//---------------------------- ViewportWindow ----------------------

enum SearchFlags {
	SEARCH_None       = 0,

	 //search mode
	SEARCH_Find       = (1<<0),
	SEARCH_Select     = (1<<1),

	 //searhc criteria
	SEARCH_Any        = (1<<2),
	SEARCH_Visible    = (1<<3),
	SEARCH_SameLevel  = (1<<4),
	SEARCH_SameOrUnderLevel = (1<<5),
	SEARCH_Children   = (1<<6),
	SEARCH_UnderMouse = (1<<7),

	SEARCH_MAX = 7
};


enum ViewportWindowActions {
	VIEWPORT_ZoomIn=1,
	VIEWPORT_ZoomOut,
	VIEWPORT_CenterReal,
	VIEWPORT_ResetView,

	VIEWPORT_Default_Zoom, 
	VIEWPORT_Set_Default_Zoom,
	VIEWPORT_Center_View,
	VIEWPORT_Center_Object,
	VIEWPORT_Zoom_To_Fit,
	VIEWPORT_Zoom_To_Object,
	VIEWPORT_Zoom_To_Width,
	VIEWPORT_Zoom_To_Height,
	
	VIEWPORT_Reset_Rotation,
	VIEWPORT_Rotate_90,
	VIEWPORT_Rotate_180,
	VIEWPORT_Rotate_270, 

	VIEWPORT_NextObject,
	VIEWPORT_PreviousObject,
	VIEWPORT_DeleteObj,

	VIEWPORT_ShiftLeft,
	VIEWPORT_ShiftRight,
	VIEWPORT_ShiftUp,
	VIEWPORT_ShiftDown,
	VIEWPORT_IncShift,
	VIEWPORT_DecShift,

	VIEWPORT_Undo,
	VIEWPORT_Redo,

	VIEWPORT_MAX
};

class ViewportWindow : public Laxkit::PanUser, public Laxkit::anXWindow
{
  private:
	char *last_message; //buffer for PostMessage2(fmt,...)
	int last_message_n;

  protected:
	int interfacemenu;
	char firsttime;
	double view_shift_amount;
	int last_mouse;

	 // Object searching utility:
	int searchx,searchy;
	const char *searchtype;

	Selection *selection;

	anInterface *copysource;
	anInterface *pastedest;

	Laxkit::anXWindow *temp_input;
	unsigned long temp_input_interface;
	char *temp_input_label;
	unsigned int temp_grab;

	Laxkit::ButtonDownInfo buttondown;
	Laxkit::ShortcutHandler *sc;

	Laxkit::RulerWindow *xruler,*yruler;
	Laxkit::Scroller *xscroller,*yscroller;
	virtual void syncrulers(int which=3);
	virtual void syncWithDp();

	virtual int deletekid(anXWindow *w);

  public:
	Laxkit::Displayer *dp;
	Laxkit::RefPtrStack<anInterface> interfaces;

 	ViewportWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder, Laxkit::Displayer *ndp=NULL);
	virtual ~ViewportWindow();
	virtual const char *whattype() { return "ViewportWindow"; }
	virtual int init();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual void Refresh();
	virtual void RefreshUnder();
	virtual void RefreshOver();
	virtual void DrawSomeData(Laxkit::Displayer *ddp,LaxInterfaces::SomeData *ndata,
			            Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0) {}
	virtual void DrawSomeData(LaxInterfaces::SomeData *ndata,
			            Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0) {}
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelRight(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelLeft(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int PerformAction(int action);
	virtual int Needtodraw();
	virtual void Needtodraw(int ntd) { needtodraw=ntd; }
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual Laxkit::Displayer *GetDisplayer() { return dp; }
	
	virtual int UseTheseScrollers(Laxkit::Scroller *x,Laxkit::Scroller *y);
	virtual int UseTheseRulers(Laxkit::RulerWindow *x,Laxkit::RulerWindow *y);
	virtual int SetSpace(double minx,double maxx,double miny, double maxy);
	virtual int Push(anInterface*i,int where, int absorbcount);
	virtual int HasInterface(int iid);
	virtual anInterface *HasInterface(const char *name, int *index_ret);
	virtual anInterface *Pop(anInterface *i, bool deletetoo = false);
	virtual anInterface *PopId(int iid, bool deletetoo = false);
	virtual void PostMessage(const char *mes);
	virtual void PostMessage2(const char *fmt, ...);
	virtual Laxkit::UndoManager *GetUndoManager();

	 //Object context adding, deleting, and adjusting
	virtual int NewData(SomeData *d, ObjectContext **oc_ret, bool clear_selection=true);
	virtual int DeleteObject();
	virtual ObjectContext *ObjectMoved(ObjectContext *oc, int modifyoc);
	virtual int ChangeContext(int x,int y,ObjectContext **oc);
	virtual int ChangeContext(ObjectContext *oc);
	virtual int ChangeObject(ObjectContext *oc, bool switchtool, bool replace_selection);
	virtual double *transformToContext(double *m,ObjectContext *oc,int invert,int full);
	virtual bool IsValidContext(ObjectContext *oc);
	virtual SomeData *GetObject(ObjectContext *oc);
	virtual int UpdateSelection(Selection *sel);
	virtual ObjectContext *CurrentContext();
	
	 //Object searching and selecting
	virtual int FindObject(int x,int y, const char *dtype, 
					SomeData *exclude, int start,ObjectContext **oc, int searcharea=0);
	virtual int FindObjects(Laxkit::DoubleBBox *box, char real, char ascurobj,
							SomeData ***data_ret, ObjectContext ***c_ret);
	virtual int SelectObject(int i);

	virtual Selection *GetSelection();
	virtual int SetSelection(Selection *nselection);

	 //copy and paste
	virtual int PasteRequest(anInterface *interf, const char *targettype);
	virtual int SetCopySource(anInterface *source);
	virtual int selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type, const char *which);

	 // coordinate helper functions
	virtual Laxkit::flatpoint realtoscreen(Laxkit::flatpoint r);
	virtual Laxkit::flatpoint screentoreal(int x,int y);
	virtual double Getmag(int c=0);
	virtual double GetVMag(int x,int y);

	 //helper for grabbing custom input for interfaces
	virtual Laxkit::anXWindow *SetupInputBox(unsigned long owner_id, const char *label, const char *text, const char *message,
											 const Laxkit::DoubleBBox &bounds, const char *ntooltip=NULL, bool send_controls=false);
	virtual Laxkit::anXWindow *GetInputBox() { return temp_input; }
	virtual void ClearInputBox();
};

} // namespace LaxInterfaces

#endif

