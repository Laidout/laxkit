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
//    Copyright (C) 2012-2013 by Tom Lechner
//
#ifndef _LAX_SHORTCUTWINDOW_H
#define _LAX_SHORTCUTWINDOW_H


#include <lax/rowframe.h>
#include <lax/menuinfo.h>

namespace Laxkit {

enum ShortcutWindowStyles
{
	SHORTCUTW_Show_Search = (1<<16),
	SHORTCUTW_Load_Save   = (1<<17),
	SHORTCUTW_MAX
};

class ShortcutWindow : public Laxkit::RowFrame
{
 protected:

	char *initialarea;
	Laxkit::ShortcutHandler *sc;
	//virtual int PerformAction(int action);

	unsigned long swin_style;
	int search_type;
	void UpdateSearch();
 public:
	char *textheader;

	ShortcutWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						const char *place=NULL);
	virtual ~ShortcutWindow();
	virtual const char *whattype() { return "ShortcutWindow"; }
	virtual int init();
	//virtual Laxkit::ShortcutHandler *GetShortcuts();
	//virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	//virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int SetSearch(const char *str, int searchtype=-1);

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	
};

} //namespace Laxkit

#endif

