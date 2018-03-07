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
#ifndef _LAX_POPUPMENU_H
#define _LAX_POPUPMENU_H


#include <lax/treeselector.h>


namespace Laxkit {


class PopupMenu : public TreeSelector
{
  protected:
	PopupMenu *parentmenu, *submenu;
	int outtimer;
	PopupMenu *Top();
	virtual void RemoveSubmenu();

  public:
	PopupMenu(const char *nname, const char *ntitle, unsigned long long style,
				int xx,int yy,int ww,int hh,int brder,
				unsigned long nowner,const char *mes,
				int mouseid,
				MenuInfo *usethismenu, char absorb_count, PopupMenu *nparentmenu=NULL,
				unsigned long long extrastyle=0
				);
	virtual ~PopupMenu();
	virtual const char *whattype() { return "PopupMenu"; }
	virtual int Idle(int tid, double delta);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int UpdateSearch(const char *searchterm, bool isprogressive);

	virtual char MouseIn(int x,int y); // return whether the mouse is in the window or not
	virtual void addselect(int i,unsigned int state);

};


} // namespace Laxkit

#endif


