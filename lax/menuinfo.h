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
//    Copyright (C) 2004-2007,2010,2012-2013 by Tom Lechner
//
#ifndef _LAX_MENUINFO_H
#define _LAX_MENUINFO_H

#include <lax/lists.h>
#include <lax/laxdefs.h>
#include <lax/strmanip.h>
#include <lax/anobject.h>
#include <lax/laximages.h>
#include <lax/refptrstack.h>

#ifndef NULL
#define NULL (0)
#endif

 //-----sorting styles
enum MenuSortStyles {
	 // These:
	SORT_NONE         =(1<<0),
	SORT_ABC          =(1<<1),
	SORT_CBA          =(1<<2),
	SORT_123          =(1<<3),
	SORT_321          =(1<<4),
	SORT_123kb        =(1<<5),
	SORT_321kb        =(1<<6),
	SORT_INFO         =(1<<7),
	SORT_INFO_REV     =(1<<8),

	 // Or'd with any of these:
	SORT_IGNORE_CASE  =(1<<9),
	SORT_DIRS_FIRST   =(1<<10),
	SORT_HIDE_HIDDEN  =(1<<11),
	SORT_DOT_FIRST    =(1<<12),
	SORT_BY_EXTENSION =(1<<13),

	SORT_MAX
};


//-------item state
//---from laxdefs.h:
//#define LAX_HIDDEN      (0)
//#define LAX_OFF         (1<<0)
//#define LAX_ON          (1<<1)
//#define LAX_GRAY        (1<<2) <- not selectable
//#define LAX_SEPARATOR   (1<<3)
//#define LAX_MSTATE_MASK (0xff)

 // note to programmer: make sure this can be combined with above item state
 // these must correspond to the equivalent LAX_*:
#define MENU_OPEN        (1<<8)
#define MENU_CCUR        (1<<9)
#define MENU_MOUSEIN     (1<<10)
#define MENU_ISLEAF      (1<<11)
#define MENU_HAS_SUBMENU (1<<12)
#define MENU_ISTOGGLE    (1<<13)
#define MENU_CHECKED     (1<<14)
#define MENU_SELECTED    (1<<15)
#define MENU_VISUAL_GRAY (1<<16) //any children inherit this grayness.. still selectable

#define MENU_SEARCH_HIDDEN (1<<17)
#define MENU_SEARCH_PARENT (1<<18)
#define MENU_SEARCH_HIT    (1<<19)

namespace Laxkit {

class MenuInfo;


//----------------------------------- MenuItem --------------------------------
class MenuItem : public anObject
{
  protected:
	void base_init();
	void base_init(const char *newitem,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal);

	MenuInfo *submenu; // menu is the menu that this item is in, submenu is assumed local
	int subislocal;

  public:
	char *name; //displayed text
	char *key; //internal key identifier (not keyboard key)
	LaxImage *image;
	int formathint;
	int id,info;
	unsigned int state;
	int x,y,w,h;
	MenuItem *nextdetail;
	MenuInfo *parent;
	anObject *extra;

	MenuItem();
	MenuItem(const char *newitem,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal);
	MenuItem(LaxImage *img=NULL);
	MenuItem(const char *newitem,const char *img,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal);
	MenuItem(const char *newitem,LaxImage *img,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal);
	virtual ~MenuItem();
	virtual MenuInfo *GetSubmenu(int create=0);
	virtual MenuInfo *CreateSubmenu(const char *ntitle);
	virtual int AddDetail(MenuItem *detail);
	virtual int AddDetail(const char *newitem,LaxImage *img=NULL,int nid=0,int ninfo=0);
	virtual MenuItem *GetDetail(int i);
	virtual int NumDetail();
	virtual void SetState(unsigned newstate, int on);
	virtual void SetExtra(anObject *obj, bool absorb);

	virtual const char *GetString(int detail);

	virtual int isSelected(int oron=1);
	virtual int hasSub() const;
	virtual int isOpen() const;
	virtual int Open();
	virtual int Close();
	virtual int hidden();
	virtual int Hide(int yes);
	virtual int hasParent(MenuInfo *menuinfo);
	virtual int hasParent(MenuItem *menuitem);
	virtual int pointIsIn(int xx,int yy) { return xx>=x && xx<x+w && yy>=y && yy<y+h; }
};


//----------------------------------- MenuInfo --------------------------------
//typedef int (*CompareFunc)(const char *s1,const char *s2);
typedef int (*CompareFunc)(MenuItem *i1,int detail1, MenuItem *i2,int detail2);

class MenuInfo : public anObject
{
  protected:
	CompareFunc Compare;

  public:
	int sortstyle;
	MenuInfo *curmenu; // where adds get put on to, could be a submenu
	char *title;
	MenuItem *parent;
	RefPtrStack<MenuItem> menuitems;

	MenuInfo(const char *ntitle=NULL);
	virtual ~MenuInfo();
	virtual void SetCompareFunc(CompareFunc func);
	virtual void SetCompareFunc(int newsortstyle);
	virtual void Sort(int detail=0,int newsortstyle=0);
	virtual void sort(int start,int end, int detail); // sort in 1,2,3..  flips elsewhere for 3,2,1
	virtual MenuInfo *findparent(MenuInfo *m,int *index=NULL);
	virtual MenuItem *findFromLine(int i); // find element with index i, counting all open submenus
	virtual int findLine(MenuItem *mi); // return index of mi counting all open submenus
	virtual int findIndex(MenuItem *mi); // return index of mi only if mi is in this menu, not a submenu
	virtual int findIndex(const char *name, int start_at=0); // return index if name is in this menu, not a submenu
	virtual int findIndex(int checkid); // return index of element with checkid in this menu, not a submenu
	virtual MenuItem *findid(int checkid); // find MenuItem with id==checkid
	virtual int idexists(int check,MenuInfo *look); // whether an id==check exists somewhere in the menu.
	virtual int getuniqueid(int trythis=-1); // find a unique id number to use
	virtual int how_many(int maxn,int all=0); // maxn=0 is count all visible, maxn>0 is only consider menuitems.e[c<maxn]

	virtual int AddItem(const char *newitem, int nid=0, int ninfo=0, LaxImage *img=nullptr, int where=-1, int state=0);
	virtual int AddDelimited(const char *newitem,char delimiter='/', int nid=0, int ninfo=0, LaxImage *img=nullptr, int where=-1, int state=0);
	virtual int AddToggleItem(const char *newitem, int nid=0, int ninfo=0, bool on=false, LaxImage *img=nullptr, int where=-1, int state=0);
	virtual int AddSep(const char *name=NULL,int where=-1);
	virtual int AddItemAsIs(MenuItem *mi,char islocal,int where=-1); //where=-1
	virtual int AddDetail(const char *newitem,LaxImage *img,int nid=0,int ninfo=0, int towhich=-1);
	virtual int AddItem(MenuItem *mi,char islocal,int where=-1); //where=-1

	virtual MenuItem *Top();
	virtual int Remove(int which=-1);
	virtual int SubMenu(const char *ntitle=NULL,int which=-1); // future adds go on a new submenu for current item
	virtual void EndSubMenu(); // stop adding to a submenu
	virtual void DoneSubMenus() { curmenu=this; }  // resets curmenu to this
	virtual void NewTitle(const char *ntitle); // set a new title for curmenu
	virtual void Flush();

	virtual int SetRecursively(unsigned long nstate, int on, int ignoreunmade=0);
	virtual int Search(const char *search, int isprogressive, int ignoreunmade=0);
	virtual void ClearSearch();

	virtual int n();
	virtual MenuItem *e(int i);
};


void menuinfoDump(MenuInfo *menu, int indent); //for debugging


} // namespace Laxkit
	
#endif

