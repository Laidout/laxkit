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
//    Copyright (c) 2004-2007,2010-2011 Tom Lechner
//
#ifndef _LAX_VIEWERWINDOW_H
#define _LAX_VIEWERWINDOW_H


#include <lax/rowframe.h>
#include <lax/rulerwin.h>
#include <lax/scroller.h>
#include <lax/messagebar.h>
#include <lax/interfaces/viewportwindow.h>

namespace LaxInterfaces {


enum ViewerWindowActions {
	VIEWER_NextTool,
	VIEWER_PreviousTool,

	VIEWER_Default_Zoom,
	VIEWER_Zoom_In,
	VIEWER_Zoom_Out,
	VIEWER_Center_View,
	VIEWER_Center_Object,
	VIEWER_Zoom_To_Fit,
	VIEWER_Zoom_To_Object,
	VIEWER_Zoom_To_Width,
	VIEWER_Zoom_To_Height, 
	VIEWER_Reset_Rotation,
	VIEWER_Rotate_0,
	VIEWER_Rotate_90,
	VIEWER_Rotate_180,
	VIEWER_Rotate_270, 
	VIEWER_Set_Default_Zoom,

	VIEWER_MAX
};

class ViewerWindow : public Laxkit::RowFrame
{
	char *last_message; //for PostMessage2(fmt,...)
	int last_message_n;
	
  protected:
	Laxkit::Scroller *xscroller,*yscroller;
	Laxkit::RulerWindow *xruler,*yruler;
	Laxkit::MessageBar *mesbar;
	Laxkit::PtrStack<Laxkit::anXWindow> tonotify; // who to send note of where the mouse is in viewport
	Laxkit::RefPtrStack<anInterface> tools;
	Laxkit::RefPtrStack<anInterface> overlays;
	int rulerh,scrollerh;
	anInterface *curtool;
	int lazytool;
	unsigned long viewer_style;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	virtual Laxkit::MenuInfo *GetZoomMenu();

  public:
	ViewportWindow *viewport;
	ViewerWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						ViewportWindow *vw=NULL);
	virtual ~ViewerWindow();
	virtual const char *whattype() { return "ViewerWindow"; }
	virtual int init();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual void PostMessage(const char *mes);
	virtual void PostMessage2(const char *fmt, ...);
	virtual void SetRealUnits();
	
	virtual int RemoveTool(int id);
	virtual int AddTool(anInterface *i, bool selectalso, bool absorbcount);
	virtual int SelectTool(const char *type);
	virtual int SelectTool(int id);
	virtual int SelectToolFor(const char *datatype,ObjectContext *oc=NULL);
	virtual anInterface *CurrentTool() { return curtool; }
	virtual anInterface *FindInterface(const char *which);
	virtual anInterface *FindInterface(int id);
	virtual int SetAsCurrentTool(anInterface *interf);

	virtual int PushInterface(anInterface *i,int absorbcount);
	virtual int PopInterface(anInterface *i);

	virtual int tools_n() { return tools.n; }
	virtual anInterface *tools_e(int i) { if (i>=0 && i<tools.n) return tools.e[i]; return NULL; }

	friend class ViewportWindow;
};


} // namespace LaxInterfaces

#endif

