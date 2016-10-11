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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_MENUSELECTOR_H
#define _LAX_MENUSELECTOR_H

#include <lax/anxapp.h>
#include <lax/menuinfo.h>
#include <lax/scrolledwindow.h>
#include <lax/buttondowninfo.h>

#include <cstring>



 // Item placement and display flags
#define MENUSEL_CHECK_ON_RIGHT       (1LL<<0)
#define MENUSEL_CHECK_ON_LEFT        (1LL<<1)
#define MENUSEL_SUB_ON_RIGHT         (1LL<<2)
#define MENUSEL_SUB_ON_LEFT          (1LL<<3)
#define MENUSEL_STATUS_ON_RIGHT      (1LL<<4)
#define MENUSEL_STATUS_ON_LEFT       (1LL<<5)
#define MENUSEL_GRAPHIC_ON_RIGHT     (1LL<<6)
#define MENUSEL_GRAPHIC_ON_LEFT      (1LL<<7)
#define MENUSEL_SUB_FOLDER           (1LL<<8)
#define MENUSEL_SUB_ARROW            (1LL<<9)
#define MENUSEL_CENTER               (1LL<<10)
#define MENUSEL_LEFT                 (1LL<<11)
#define MENUSEL_RIGHT                (1LL<<12)
#define MENUSEL_TAB_JUSTIFY          (1LL<<13)
#define MENUSEL_CHECKS               (1LL<<14)
#define MENUSEL_CHECKBOXES           (1LL<<15)
#define MENUSEL_SHOW_LEAF_ONLY       (1LL<<16)
#define MENUSEL_SHOW_SUB_ONLY        (1LL<<17)
#define MENUSEL_SCROLLERS            (1LL<<18)
#define MENUSEL_USE_TITLE            (1LL<<19)
#define MENUSEL_BUTTONCOLORS         (1LL<<20)
#define MENUSEL_TEXTCOLORS           (1LL<<21)
#define MENUSEL_FIT_TO_WIDTH         (1LL<<22)
#define MENUSEL_FIT_TO_HEIGHT        (1LL<<23)
#define MENUSEL_TREE                 (1LL<<24)
#define MENUSEL_COMPOUND_TREE        (1LL<<25)

#define MENUSEL_SORT_NUMBERS         (1LL<<26)
#define MENUSEL_SORT_LETTERS         (1LL<<27)
#define MENUSEL_SORT_REVERSE         (1LL<<28)
#define MENUSEL_SORT_SUBS_FIRST      (1LL<<29)
#define MENUSEL_SORT_IGNORE_CASE     (1LL<<30)
#define MENUSEL_SORT_BY_EXTENSIONS   (1LL<<31)

#define MENUSEL_USE_DOT              (1LL<<32)
#define MENUSEL_USE_DOT_DOT          (1LL<<33)
#define MENUSEL_USE_UP               (1LL<<34)
#define MENUSEL_USE_BACK             (1LL<<35)

#define MENUSEL_ONE_ONLY             (1LL<<36)
#define MENUSEL_ZERO_OR_ONE          (1LL<<37)
#define MENUSEL_SELECT_ANY           (1LL<<38)
#define MENUSEL_SELECT_LEAF_ONLY     (1LL<<39)
#define MENUSEL_SELECT_SUB_ONLY      (1LL<<40)
#define MENUSEL_CURSSELECTS          (1LL<<41)
#define MENUSEL_CURSSENDS            (1LL<<42)
#define MENUSEL_FOLLOW_MOUSE         (1LL<<43)
#define MENUSEL_GRAB_ON_MAP          (1LL<<44)
#define MENUSEL_GRAB_ON_ENTER        (1LL<<45)
#define MENUSEL_OUT_CLICK_DESTROYS   (1LL<<46)
#define MENUSEL_CLICK_UP_DESTROYS    (1LL<<47)
#define MENUSEL_DESTROY_ON_LEAVE     (1LL<<48)
#define MENUSEL_DESTROY_ON_FOCUS_OFF (1LL<<49)

#define MENUSEL_REARRANGEABLE        (1LL<<50)
#define MENUSEL_EDIT_IN_PLACE        (1LL<<51)
//*** add items?? (more than just editing names)
//*** remove items??

#define MENUSEL_SEND_ON_UP           (1LL<<52)
#define MENUSEL_SEND_ON_ENTER        (1LL<<53)
#define MENUSEL_SEND_IDS             (1LL<<54)
#define MENUSEL_SEND_STRINGS         (1LL<<55)

//... remember that the buck stops with (1<<63)


namespace Laxkit {

class MenuSelector : public ScrolledWindow
{
 protected:
	ButtonDownInfo buttondown;
	int mousedragged;
	NumStack<int> columns;
	MenuInfo *menu;
	int mx,my;
	int sgw,checkw,subw;
	int firstinw;
	int textheight,lineheight,pagesize,columnsize;
	MenuItem *curmenuitem;
	int curitem,ccuritem;
	int timerid;
	virtual void adjustinrect();
	virtual void findoutrect();
 	virtual double getitemextent(MenuItem *mitem,double *w,double *h,double *gx,double *tx);
 	virtual double getgraphicextent(MenuItem *mitem,double *x,double *y);
	virtual void drawStatusGraphic(int x,int y,int state);
	virtual void drawitem(MenuItem *mitem,IntRectangle *itemspot);
	virtual void drawitem(int c);
	virtual void drawsep(const char *name,IntRectangle *rect);
	virtual void drawarrow(int x,int y,int r,int type);
	virtual void drawcheck(int on, int x,int y);
	virtual void drawsubindicator(MenuItem *mitem,int x,int y);
	virtual void drawitemname(MenuItem *mitem,IntRectangle *rect);
	virtual void drawtitle();
	virtual int send(int deviceid);
	virtual void addselect(int i,unsigned int state);
	virtual int findmaxwidth(int s,int e, int *h_ret);
	virtual int findItem(int x,int y);
	virtual int findRect(int c,IntRectangle *itemspot);
	virtual void editInPlace();
	virtual int numItems();
	virtual MenuItem *item(int i,char skipcache=0);
	virtual void arrangeItems(int forwrapping=0);
	virtual void setcuritem(int i);
	virtual void syncWindows(int useinrect=0);
	virtual int makeinwindow();
  
  public:
	unsigned long highlight,shadow;
	unsigned long long menustyle;
	int padg,pad,leading;
	MenuSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
				int xx,int yy,int ww,int hh,int brder,
				anXWindow *prev,unsigned long nowner=0,const char *mes=0,
				unsigned long long nmstyle=0,MenuInfo *minfo=NULL,char absorb_count=0); 
	virtual ~MenuSelector();
	virtual int init();
	virtual void Refresh();
	virtual int Event(const EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int Idle(int tid);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int FocusOn(const FocusChangeData *e);
	virtual int FocusOff(const FocusChangeData *e);

	virtual int WrapToMouse(int mouseid, anXWindow *onedgeofthis=NULL);
	virtual int WrapToPosition(int screen_x, int screen_y, int screen, anXWindow *onedgeofthis=NULL);
	virtual int movescreen(int dx,int dy);
	virtual int SetFirst(int which,int x,int y);
	virtual int Curitem() { return curitem; }
	virtual const MenuItem *Item(int c) { return item(c); }
	virtual int Select(int which);
	virtual int NumSelected();
	virtual int *WhichSelected(unsigned int state);
	virtual void SetLineHeight(int ntotalheight,int newleading,char forcearrange=1);
	virtual void Sync();
	
//	virtual int RemoveItem(int whichid);
//	virtual int RemoveItem(const char *i);
	virtual void Sort(int t);
	virtual int AddItems(const char **i,int n,int startid); // assume ids sequential, state=0
	virtual int AddItem(const char *i,int nid,int nst);
};


} // namespace Laxkit

#endif


