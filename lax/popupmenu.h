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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_POPUPMENU_H
#define _LAX_POPUPMENU_H

#include <lax/menuselector.h>

namespace Laxkit {

class PopupMenu : public MenuSelector
{
 protected:
	PopupMenu *parentmenu,*submenu;
//	int createsubwindow();
 public:
	PopupMenu(const char *nname, const char *ntitle, unsigned long long style,
				int xx,int yy,int ww,int hh,int brder,
				unsigned long nowner,const char *mes,
				int mouseid,
				MenuInfo *usethismenu, char mislocal, PopupMenu *nparentmenu=NULL,
				unsigned long long extrastyle=0
				);
	virtual ~PopupMenu();
	virtual const char *whattype() { return "PopupMenu"; }
	virtual int Idle(int tid=0);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);

	virtual char MouseIn(int x,int y); // return whether the mouse is in the window or not
	virtual void addselect(int i,unsigned int state);

//	virtual int Add(char *name,int nid,int nstate=1);
//	virutal int AddSep();
//	virtual void SubMenu();
//	virtual void EndSubMenu();
//	virtual void Refresh();
//	virtual MenuItem *finditem(int idnumber);
//	virtual int gray(int idnumber,int on);
//	virtual remove(int idnumber);
//	virtual void UpdateMenu();

};

} // namespace Laxkit

#endif


