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
//    Copyright (C) 2004-2006,2010 by Tom Lechner
//
#ifndef _LAX_SLIDERPOPUP_H
#define _LAX_SLIDERPOPUP_H

#include <lax/itemslider.h>
#include <lax/menuinfo.h>

namespace Laxkit {

#define SLIDER_ICON_ONLY    (0<<21)
#define SLIDER_TEXT_ONLY    (1<<21)
#define SLIDER_TEXT_ICON    (2<<21)
#define SLIDER_ICON_TEXT    (3<<21)
#define SLIDER_WHAT_MASK    (3<<21)
#define SLIDER_ALLOW_EDIT   (1<<23)
#define SLIDER_SEND_STRING  (1<<24)
#define SLIDER_LEFT         (1<<25)
#define SLIDER_RIGHT        (1<<26)
#define SLIDER_CENTER       (1<<27)
#define SLIDER_SKIP_TOGGLES (1<<28)
#define SLIDER_POP_ONLY     (1<<29)
	
 //used as a menuitem state, show in menu, but not prev/next
 //note to programmer: this define must cooperate with MENU_* defines in menuinfo.h
#define SLIDER_IGNORE_ON_BROWSE (1<<19)

class SliderPopup : public ItemSlider
{
  protected:
	MenuInfo *items;
	int itemsislocal;
	virtual void drawarrow();
	virtual int send();
	virtual int getid(int i);
	virtual int numitems() { return items->menuitems.n; }
	virtual void makePopup(int mouseid);

  public:
	int arrowwidth,pad,gap;
	SliderPopup(anXWindow *parnt,const char *nname,const char *ntitle,
				unsigned long nstyle, int xx,int yy,int ww,int hh,int brder,
				anXWindow *prev,unsigned long nowner,const char *nsendthis,
				MenuInfo *nitems=NULL,int ilocal=1);
	virtual ~SliderPopup();
	virtual void Refresh();
	virtual int Event(const EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int scan(int x,int y,unsigned int state);

	virtual int SelectPrevious(double multiplier);
	virtual int SelectNext(double multiplier);
	virtual const char *GetCurrentItem();
	virtual int GetCurrentItemIndex();
	virtual int DeleteItem(int id);
	virtual int AddSep(const char *name=NULL,int where=-1);
	virtual int AddItem(const char *newitem,int nid);
	virtual int AddItem(const char *newitem,LaxImage *icon,int nid);
	virtual int AddItems(const char **newitems,int n,int startid);
	virtual int SetState(int which, int extrastate, int on);
	virtual int GetState(int which, int extrastate);
	virtual int GetItemIndex(int fromid);
	virtual int Flush(int completely=0);
	virtual void WrapToExtent();

	//virtual int MoveResize(int nx,int ny,int nw,int nh);
	//virtual int Resize(int nw,int nh);
};

} // namespace Laxkit

#endif

