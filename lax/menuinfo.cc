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
//    Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/menuinfo.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>

#include <cstdlib>
#include <iostream>
using namespace std;
#define DBG 





namespace Laxkit {


/*! \defgroup menuthings Menu Related Things
 * These are classes related to building and accessing menus
 * in various forms. The main menu structure is MenuInfo, which contains 
 * a stack of MenuItem instances, which in turn can contain a submenu.
 * One can derive classes from MenuItem to add special fields. 
 *
 * This MenuInfo/MenuItem
 * system is basically a tree that can be traversed only downward. 
 * This makes building of composite menus fairly easy, such as for context 
 * sensitve popup menus. Also, the whole tree does not have to be
 * created beforehand. Each item can be tagged with MENU_HAS_SUBMENU to indicate
 * that a submenu is available if necessary.
 * 
 * The Laxkit window classes that use this structure are 
 * \link Laxkit::MenuSelector MenuSelector \endlink, 
 * \link Laxkit::PopupMenu PopupMenu\endlink, and 
 * \link Laxkit::StrSliderPopup\endlink.
 *
 * *** fltk has neat add like \n
 * add("File")\n
 * add("File/1st file itme")\n
 * add("File/2nd...")
 * <pre>
 *  menu.Add("blah1",id=1,grayed)
 *  menu.Add("blah2",2)
 *  menu.SubMenu([title])
 *   menu.Add("a")
 *   menu.Add("b")
 *    menu.SubMenu([title],position==last added)
 *    menu.Add("1")
 *    menu.Add("2")
 *    menu.EndSubmenu()
 *   menu.EndSubmenu()
 *  menu.Add("blah4");
 *  menu.AddSep();
 *  menu.Add("blah5");
 * </pre>
 */

//--------------------------------------------------------

/*! \typedef int (*CompareFunc)(const char *s1,const char *s1)
 * \ingroup menuthings
 *
 * A character string comparison function used in MenuInfo/MenuIem.
 */
/*! \class MenuItem
 * \ingroup menuthings
 * \brief Node type for MenuInfo
 *
 * Holds an item name, an id, extra info, state, and sub MenuInfo.
 * 
 * The state variable is some combination of the following (defined in lax/laxdefs.h):
 * \code
 *  #define LAX_HIDDEN     (0)
 *  #define LAX_OFF        (1<<0)
 *  #define LAX_ON         (1<<1)
 *  #define LAX_GRAY       (1<<2)
 *  #define LAX_SEPARATOR  (1<<3)
 *  #define LAX_MSTATE_MASK (0xff)
 *  
 *   // note to programmer: make sure this can be combined with above state
 *  #define LAX_OPEN        (1<<8)
 *  #define LAX_CCUR        (1<<9)
 *  #define MENU_HAS_SUBMENU (1<<12)
 *  #define LAX_ISTOGGLE    (1<<13)
 *  #define LAX_CHECKED     (1<<14)
 * \endcode
 */
/*! \var int MenuItem::info
 * \brief Extra info, which is unused by either MenuItem or MenuInfo themselves.
 */
/*! \var unsigned int MenuItem::state
 * \brief Holds various state of the item, like if a menu is open, etc.
 *
 * This is used by various control mechanisms to keep track of what to
 * highlight, and what is currently selected (***check this last point!).
 * 
 * If an item is a leaf, then it does not have MENU_HAS_SUBMENU set.
 * A toggle item is a leaf that is allowed to be checked. This is to distinguish
 * items that are leaves, but which provoke some action by themselves, and items
 * that are things that are either on or off.
 */
/*! \var int MenuItem::subislocal
 * \brief Nonzero if submenu should be deleted from the destructor.
 */


MenuItem::MenuItem()
{ 
	base_init();
}

void MenuItem::base_init()
{ 
	parent=NULL;
	id=0; 
	name=NULL;
	image=NULL;
	submenu=NULL;
	state=LAX_OFF; 
	subislocal=0;
	info=0;
	x=y=w=h=0;

	formathint=0;
	nextdetail=NULL;
}

//! Item constructor, straight copy of all fields.
MenuItem::MenuItem(const char *newitem,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal)
{ 
	base_init(newitem,nid,nstate,ninfo,nsub,sublocal);
}

void MenuItem::base_init(const char *newitem,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal)
{ 
	parent=NULL;
	id=nid; 
	state=nstate; 
	submenu=nsub; 
	subislocal=sublocal;
	name=NULL; 
	image=NULL;
	makestr(name,newitem); 
	info=ninfo; 
	x=y=w=h=0;

	formathint=0;
	nextdetail=NULL;
}


/*! img's count is incremented.
 */
MenuItem::MenuItem(LaxImage *img)
{
	base_init();
	image=img;
	if (image) image->inc_count();
}

//! Image file img is loaded, so it has a count of 1 after the constructor.
MenuItem::MenuItem(const char *newitem,const char *img,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal)
{
	base_init(newitem,nid,nstate,ninfo,nsub,sublocal);
	image=load_image(img);
}

//! img pointer is transfered. Its count is not incremented.
MenuItem::MenuItem(const char *newitem,LaxImage *img,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal)
{
	base_init(newitem,nid,nstate,ninfo,nsub,sublocal);
	image=img;
}

//! Destructor, deletes name and if subislocal!=0, also deletes submenu.
MenuItem::~MenuItem()
{
	if (name) delete[] name; 
	if (image) image->dec_count();
	if (subislocal && submenu) delete submenu; 

	if (nextdetail) delete nextdetail;
}

//! Create a new submenu if one does not exist already.
/*! Return the current submenu if it does exist.
 * If it does exist already, ntitle is ignored.
 *
 * The state variable is or'd with MENU_HAS_SUBMENU.
 *
 * The default here is to create a new submenu that is local, that is, it will
 * be deleted in the destructor. FileMenuItem, for instance, creates a submenu
 * with that directory's contents only when it really needs to.
 */
MenuInfo *MenuItem::CreateSubmenu(const char *ntitle)
{
	if (!submenu) {
		submenu=new MenuInfo(ntitle);
		submenu->parent=this;
		subislocal=1;
	}
	state|=MENU_HAS_SUBMENU;
	return submenu;
}

//! Return the submenu, if it exists.
/*! Default is to call CreateSubmenu(NULL) if the item is tagged as having
 * a submenu and create!=0. 
 *
 * Derived classes would redefine CreateSubmenu() so that if an item is tagged as
 * having a submenu, but the submenu is not yet defined, then it is created
 * here only if create!=0. It should not have to always create, since that might
 * slow down various searching mechanisms in MenuInfo, for instance ***.
 * 
 * If there is a potential submenu available, then state must have
 * MENU_HAS_SUBMENU set.
 */
MenuInfo *MenuItem::GetSubmenu(int create) // create=0
{
	if (state&MENU_HAS_SUBMENU && !submenu && create) CreateSubmenu(NULL);
	return submenu;
}
	
int MenuItem::isSelected(int oron)
{
	if (oron) return state&(MENU_SELECTED|LAX_ON);
	return state&MENU_SELECTED;
}

void MenuItem::SetState(unsigned newstate, int on)
{
	if (on) state|=newstate;
	else state&=~newstate;
}

int MenuItem::hasSub() const 
{ return state&MENU_HAS_SUBMENU; }

int MenuItem::isOpen() const
{ return state&MENU_OPEN; }

//! Return 1 for definitely hidden. 2 for hidden, except a child is a search hit.
int MenuItem::hidden()
{
	int h=state&(LAX_HIDDEN|MENU_SEARCH_HIDDEN);
	if (h && (state&MENU_SEARCH_PARENT)) return 2;
	if (h) return 1;
	return 0;
}

int MenuItem::hasParent(MenuInfo *menuinfo)
{
	int n=1;
	MenuInfo *info=parent;
	MenuItem *item=this;
	while (info) {
		if (info==menuinfo) return n;
		item=parent->parent;
		info=item->parent;
		n++;
	}
	return 0;
}

int MenuItem::hasParent(MenuItem *menuitem)
{
	int n=1;
	MenuInfo *info=parent;
	MenuItem *item=(parent?parent->parent:NULL);
	while (item) {
		if (item==menuitem) return n;
		info=item->parent;
		item=(info?info->parent:NULL);
		n++;
	}
	return 0;
}

//! Install detail to end of nextdetail list.
int MenuItem::AddDetail(MenuItem *detail)
{
	MenuItem *next=this;
	while (next->nextdetail) next=next->nextdetail;

	next->nextdetail=detail;
	return 0;
}

const char *MenuItem::GetString(int detail)
{
	if (detail==0) return name;

	MenuItem *m=this;
	while (m && detail) { m=m->nextdetail; detail--; }
	if (m) return m->name;
	return "";
}


//---------------------------- MenuInfo -----------------------------
/*! \class MenuInfo
 * \ingroup menuthings
 * \brief General container for menu types of lists.
 *
 * This class stores lists that are used by MenuSelector, StrSliderPopup, and PopupMenu.
 * It is basically a stack of MenuItem objects, and an optional title.
 *
 *
 *  \todo *** RemoveItem
 *  \todo hot keys
 *
 * \code
 *   const char menustuff[][**] = {
 *   		"g,id#,blah1",
 *   		"2,blah2",
 *   		"g,-34,blah3d",
 *   		"=:Submenu Title",
 *   		"=a",
 *   		"=b",
 *   		"=c",
 *   		"=d",
 *   		"==1",
 *   		"==2",
 *   		"==3",
 *   		"--",    <-- separator(??? not imp?)
 *   		NULL,    <-- or separator! or perhaps NULL means end of list?
 *   		"blah4",
 *   		"--",
 *   		"blah5", }
 *   		
 *   menu.AddItem("blah1",id=1,LAX_GRAY);
 *   menu.AddItem("blah2",2);
 *   menu.SubMenu([title]);
 *    menu.AddItem("a");
 *    menu.AddItem("b");
 *     menu.SubMenu([title],position==last added);
 *     menu.AddItem("1");
 *     menu.AddItem("2");
 *     menu.EndSubmenu();
 *    menu.EndSubmenu();
 *   menu.AddItem("blah4");
 *   menu.AddSep();
 *   menu.AddItem("blah5");
 *   menu.AddItems(menustuff,n,startid);
 *  \endcode
 *  
 */
/*! \var MenuInfo *MenuInfo::curmenu
 * \brief Holds a temporary pointer to the (sub)menu that should be added to or modified.
 *
 * curmenu is set to this in the constructor. At no time should it be NULL.
 */
/*! \var char *MenuInfo::title
 * \brief The title of the menu.
 */
/*! \var PtrStack<MenuItem> MenuInfo::menuitems
 * \brief Holds the individual menu items.
 */
/*! \var int MenuInfo::sortstyle
 * \brief Indicates how to sort.
 *
 * \code
 *   //These:
 *  #define SORT_NONE         (1<<0)
 *  #define SORT_ABC          (1<<1)
 *  #define SORT_CBA          (1<<2)
 *  #define SORT_123          (1<<3)
 *  #define SORT_321          (1<<4)
 *   // Or'd with any of these:
 *   // DOT_FIRST means place "." and ".." at the beginning always. (if they are included)
 *   // HIDE_HIDDEN means don't show any entries that begin with a . (except "." or "..")
 *  #define SORT_IGNORE_CASE  (1<<8)
 *  #define SORT_DIRS_FIRST   (1<<9)
 *  #define SORT_HIDE_HIDDEN  (1<<10)
 *  #define SORT_DOT_FIRST    (1<<11)
 *  #define SORT_BY_EXTENSION (1<<12)
 * 
 * \endcode
 */

void menuinfoDump(MenuInfo *menu, int indent)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	for (int c=0; c<menu->menuitems.n; c++) {
		cerr <<spc<<"Item "<<c<<": "<<(menu->menuitems.e[c]->name?menu->menuitems.e[c]->name:"(no name)")<<endl;
		if (menu->menuitems.e[c]->state&MENU_HAS_SUBMENU)
			menuinfoDump(menu->menuitems.e[c]->GetSubmenu(),indent+2);
	}
}

/*! \fn void MenuInfo::DoneSubMenus()
 * \brief Just resets curmenu to this.
 */

//! Constructor, just make empty menu with title set to ntitle.
MenuInfo::MenuInfo(const char *ntitle) // ntitle
{
	title=NULL;
	makestr(title,ntitle);
	curmenu=this;
	Compare=strcmp;
	parent=NULL;
}

//! Destructor, just frees the title. The stack is flushed automatically.
MenuInfo::~MenuInfo()
{
	delete[] title;
	 // menuitems flush automatically
}

//! Currently adds a grayed separator with name="", id=0, no submenu, (but it is still legal to add a submenu!!)
/*! Returns whatever AddItem returns. */
int MenuInfo::AddSep(const char *name,int where)
{
	return AddItem((name?name:""),NULL,-1,LAX_GRAY|LAX_SEPARATOR,0,NULL,-1,0);
}

//! Future Add*() go on a new submenu for item which, or top of the stack.
/*! This prompts the MenuItem to establish a new MenuInfo
 *
 * If which is out of bounds, it is set to the final item on the stack
 *
 * On success, 0 is returned.
 * If curmenu has no items then nothing is done, and 1 is returned.
 * If there is a problem creating a new submenu, then curmenu is not changed and 2 is returned.
 */
int MenuInfo::SubMenu(const char *ntitle,int which)
{
	if (curmenu->menuitems.n==0) return 1;
	if (which<0 || which>=curmenu->menuitems.n) which=curmenu->menuitems.n-1;
	curmenu->menuitems.e[which]->state|=MENU_HAS_SUBMENU;
	MenuInfo *ncurmenu=curmenu->menuitems.e[which]->GetSubmenu(1); // 1 says create if does not exist
	if (ncurmenu) {
		ncurmenu->parent=curmenu->menuitems.e[which];
		curmenu=ncurmenu;
	}
	else return 1;
	return 0;
}

//! Stop adding to a submenu, revert curmenu to the parent of that submenu.
void MenuInfo::EndSubMenu()
{
	if (curmenu && curmenu!=this) {
		curmenu=findparent(curmenu,NULL);
	}
	DBG menuinfoDump(this,0);
}

//! Find the parent menu (MenuInfo that has the item, not the MenuItem that has m) of m.
/*! If index is not null, then also put the index of the item within the found
 *  parent menu whose submenu is equal to m. If there is no such m, then
 *  -1 is put in index. If m==this, index is set to -2 and NULL is returned.
 *
 *  If no parent menu is found, which should only happen when m is nowhere
 *  in the tree, then index is set to -1, and NULL is returned.
 *
 *  Please note that this basically assumes that all the relevant submenus
 *  are already defined. That is, any potential submenus that are not
 *  currently defined do not get defined in the process of searching.
 */
MenuInfo *MenuInfo::findparent(MenuInfo *m,int *index) //index=NULL
{
	if (m==this) { 
		if (index) *index=-2;
		return NULL;
	}
	
	int c;
	 // scan first level first
	for (c=0; c<menuitems.n; c++) {
		if (menuitems.e[c]->GetSubmenu(0)==m) {
			if (index) *index=c;
			return this;
		}
	}
	
	 // scan within submenus
	MenuInfo *mm;
	for (c=0; c<menuitems.n; c++) {
		mm=menuitems.e[c]->GetSubmenu(0);
		if (!mm) continue;
		mm=mm->findparent(m,index);
		if (mm) {
			return mm;
		}
	}

	 // not found
	if (index) *index=-1;
	return NULL;
}

//! Recursive quick sort routine.
/*! This can be called to only sort a subset of the menu. If end<0, then sort all from start to the final item.
 *
 * Currently only sorts in a,b,c order. 
 * Only the top is sorted, not submenus.
 *
 * \todo *** Really, there should be some other MenuItem based function pointer for
 *   the sorting, to accommodate sorting as text, or as numbers,
 *   or as some other format, without having to derive a new class???
 */
void MenuInfo::sort(int start,int end, int detail) // sort in 1,2,3..  
{
	//DBG cerr <<"--quick sort:"<<start<<" "<<end<<endl;
	if (end<0) end=menuitems.n-1;
	if (start>=end) return;
	int s,e;
	const char *mid=menuitems.e[end]->GetString(detail);
	MenuItem *mi;
	s=start;
	e=end;
	while (s<=e) {
		if (Compare(menuitems.e[s]->GetString(detail),mid)<0) { s++; continue; }
		if (Compare(menuitems.e[e]->GetString(detail),mid)>0) { e--; continue; }

		//DBG cerr <<"=== sort swap: "<<s<<','<<e<<endl;
		mi=menuitems.e[s];
		menuitems.e[s]=menuitems.e[e];
		menuitems.e[e]=mi;
		s++;
		e--;
	}
	 //now s==e+1
	sort(start,e, detail);
	sort(s,end, detail);
}

//! Sort the items according to sortstyle.
/*!  
 * *** should be able to sort only a subset, for instance all 
 * items between separators.
 */
void MenuInfo::Sort(int detail,int newsortstyle) 
{
	if (newsortstyle) SetCompareFunc(newsortstyle);
	sort(0,menuitems.n-1, detail);
}

//! Set the compare function to some "int (*func)(const char *,const char *).
/*! If func==NULL, then reset Compare to strcmp.
 */
void MenuInfo::SetCompareFunc(CompareFunc func)
{
	if (func) Compare=func; else Compare=strcmp;
	Sort();
}

int reversestrcmp(const char *s1,const char *s2)
{ return -strcmp(s1,s2); }

int reversestrcasecmp(const char *s1,const char *s2)
{ return -strcasecmp(s1,s2); }

int strcmp123(const char *s1,const char *s2)
{ return atof(s1)<atof(s2)?-1:(atof(s1)>atof(s2)?1:0); }

int strcmp321(const char *s1,const char *s2)
{ return atof(s1)>atof(s2)?-1:(atof(s1)<atof(s2)?1:0); }

//! Set Compare to the appropriate function for newsortstyle
void MenuInfo::SetCompareFunc(int newsortstyle)
{
	sortstyle=newsortstyle;
	if (sortstyle&SORT_ABC) 
		if (sortstyle&(SORT_IGNORE_CASE)) Compare=strcasecmp;
		else Compare=strcmp;
	else if (sortstyle&SORT_CBA) 
		if (sortstyle&(SORT_IGNORE_CASE)) Compare=reversestrcasecmp;
		else Compare=reversestrcmp;
	else if (sortstyle&SORT_123) Compare=strcmp123;
	else if (sortstyle&SORT_321) Compare=strcmp321;
	else Compare=strcmp; //**** default??
}

//! Add a whole bunch of items at the same time.
/*! Assigns ids sequentially starting with startid.
 * Does not check for multiple occurences of the ids used.
 * 
 *  Returns number of items added.
 * ***this is cheap, should be optimized for large arrays?? add in one lump, then sort??
 */
int MenuInfo::AddItems(const char **i,int n,int startid) // assume ids sequential, state=0
{
	if (i==NULL || n==0) return 0;
	int where=curmenu->menuitems.n;
	for (int c=0; c<n; c++) 
		if (!isblank(i[c])) AddItem(i[c],startid++,LAX_OFF,0,NULL,where);
	return n;
}

//! Add an already made MenuItem.
/*! Note that mi->parent is set to curmenu.
 * Returns the number of menuitems.
 *
 * If islocal is nonzero then this menuitem is not deleted when the menuitems stack is flushed.
 */
int MenuInfo::AddItem(MenuItem *mi,char islocal,int where)//where=-1
{
	curmenu->menuitems.push(mi,islocal,where);
	mi->parent=curmenu;
	return curmenu->menuitems.n;
}

//! Add an already made MenuItem, but DO NOT reparent. This is for temporary lists refering to the main one.
/*! Returns the number of menuitems.
 *
 * If islocal is nonzero then this menuitem is not deleted when the menuitems stack is flushed.
 */
int MenuInfo::AddItemAsIs(MenuItem *mi,char islocal,int where)//where=-1
{
	curmenu->menuitems.push(mi,islocal,where);
	return curmenu->menuitems.n;
}

//! Add item at position where, or to end of menu if where<0.
/*! Please note that the MenuItem that is created is local to the menuitems stack (it will be deleted
 *  when the item stack is flushed), while the passed in submenu is local according to subislocal.
 */
int MenuInfo::AddItem(const char *newitem,LaxImage *img,int nid)
{
	MenuItem *mi=new MenuItem(newitem,img,nid,0,0,NULL,0);
	curmenu->menuitems.push(mi,1,-1);
	mi->parent=curmenu;
	return curmenu->menuitems.n;
}

//! Add item at position where, or to end of menu if where<0.
/*! Please note that the MenuItem that is created is local to the menuitems stack (it will be deleted
 *  when the item stack is flushed), while the passed in submenu is local according to subislocal.
 */
int MenuInfo::AddItem(const char *newitem,LaxImage *img,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int where,char subislocal)
{
	MenuItem *mi=new MenuItem(newitem,img,nid,nstate,ninfo,nsub,subislocal);
	curmenu->menuitems.push(mi,1,where);
	mi->parent=curmenu;
	return curmenu->menuitems.n;
}

//! Add item at position where, or to end of menu if where<0.
/*! Please note that the MenuItem that is created is local to the menuitems stack (it will be deleted
 *  when the item stack is flushed), while the passed in submenu is local according to subislocal.
 */
int MenuInfo::AddItem(const char *newitem,int nid)
{
	MenuItem *mi=new MenuItem(newitem,nid,LAX_OFF,0,NULL,0);
	curmenu->menuitems.push(mi,1,-1);
	mi->parent=curmenu;
	return curmenu->menuitems.n;
}

//! Add item at position where, or to end of menu if where<0.
/*! Please note that the MenuItem that is created is local to the menuitems stack (it will be deleted
 *  when the item stack is flushed), while the passed in submenu is local according to subislocal.
 */
int MenuInfo::AddItem(const char *newitem,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int where,char subislocal)
{
	MenuItem *mi=new MenuItem(newitem,nid,nstate,ninfo,nsub,subislocal);
	curmenu->menuitems.push(mi,1,where);
	mi->parent=curmenu;
	return curmenu->menuitems.n;
}

/*! A shortcut to add something like "group 1/subgroup/item" to submenu.
 * Adds any non-existing intermediate groups (with id==0). Afterwards, curmenu is unchanged.
 */
int MenuInfo::AddDelimited(const char *newitem,char delimiter, int nid)
{
	if (!newitem || !*newitem) return 1;
	
	const char *st=strchr(newitem,delimiter);
	if (!st) return AddItem(newitem,nid);

	char *group=newnstr(newitem,st-newitem);
	int i=curmenu->findIndex(group);
	if (i<0) {
		AddItem(group);
		SubMenu();
	} else {
		SubMenu(NULL,i);
	}
	AddDelimited(st+1,delimiter,nid);
	EndSubMenu();

	return 0;
}

//! Add item detail to the item at the top of menuitems, or to towhich.
/*! If there is no recent item, then return 1. On success, return 0.
 */
int MenuInfo::AddDetail(const char *newitem,LaxImage *img,int nid,int ninfo, int towhich)
{
	if (towhich<0 || towhich>=curmenu->menuitems.n) towhich=curmenu->menuitems.n-1;
	if (towhich<0) return 0;

	MenuItem *mi=new MenuItem(newitem,img,nid,LAX_OFF,ninfo,NULL,0);
	curmenu->menuitems.e[towhich]->AddDetail(mi);

	return 0;
}

//! Flush menuitems and set curmenu=this.
void MenuInfo::Flush()
{
	curmenu=this;
	menuitems.flush();
}

//! Counts how many items are visible (including separators, but not hidden). 
/*! If all is 1, then include all the items from submenus, whether they are open or not. 
 *  If all is 0, then include all items from only the submenus that are open.
 *  If all is -1, then include only the items from this menu.
 *  
 *  Any potential menus that do not currently exist are skipped.
 * 
 *  Only include items whose stack indices i in this top menu where i<maxn. If maxn
 *  is out of bounds, then consider all the items in this menu.
 *
 *  ***TODO: Should decide whether to include a submenu title as a line for open menus!!
 *  right now it does not. note the title of a submenu is not the same as the name
 *  of the item that has the submenu.
 */
int MenuInfo::howmany(int maxn,int all) // maxn=0, maxn is only consider menuitems.e[c<maxn]
{
	int n=0;
	if (maxn<=0 || maxn>menuitems.n) maxn=menuitems.n;
	if (all==-1) return menuitems.n>maxn?maxn:menuitems.n;
	for (int c=0; c<maxn; c++) {
		if (menuitems.e[c]->state&LAX_HIDDEN) continue;
		n++;
		if (menuitems.e[c]->state&MENU_HAS_SUBMENU)
			if (all || menuitems.e[c]->state&LAX_OPEN) {
				MenuInfo *minfo=menuitems.e[c]->GetSubmenu();
				if (minfo) n+=minfo->howmany(-1,all);
			}
	}
	return n;
}

//! Find the line number of mi (starting at 0), counting all open submenus, thus as it would appear onscreen in a tree.
/*! This includes separators but not hidden items, thus if mi is in the tree but hidden,
 * then no valid index is returned for it.
 *
 * If mi is not found, then -1 is returned.
 */
int MenuInfo::findLine(MenuItem *mi)
{
	int i=0,i2;
	for (int c=0; c<menuitems.n; c++) {
		if (menuitems.e[c]->state&LAX_HIDDEN) continue;
		if (menuitems.e[c]==mi) return i;
		i++;
		if (menuitems.e[c]->state&MENU_HAS_SUBMENU && menuitems.e[c]->state&LAX_OPEN) {
			MenuInfo *minfo=menuitems.e[c]->GetSubmenu(0);
			if (minfo) {
				i2=minfo->findLine(mi);
				if (i2>=0) return i+i2;
				i+=minfo->howmany(-1,0);
			}
		}
	}
	return -1;
}

/*! Return index if name is in this menu, not a submenu.
 * Note that name is not necessarily unique. If start_at>0, then start the search at that item.
 *
 * Return -1 if not found.
 */
int MenuInfo::findIndex(const char *name, int start_at)
{
	if (!name || start_at<0) return -1;
	for (int c=start_at; c<menuitems.n; c++) {
		if (!menuitems.e[c]->name) continue;
		if (!strcmp(name,menuitems.e[c]->name)) return c;
	}
	return -1;
}

//! Returns index in menuitems.e of element with id==checkid in this menu, not a submenu.
int MenuInfo::findIndex(int checkid)
{
	for (int c=0; c<menuitems.n; c++) if (menuitems.e[c]->id==checkid) return c;
	return -1;
}

//! returns index in menuitems.e of mi only if mi is in this menu, not a submenu.
int MenuInfo::findIndex(MenuItem *mi)
{
	for (int c=0; c<menuitems.n; c++) if (menuitems.e[c]==mi) return c;
	return -1;
}

// //! Get the element that is plusorminus total line numbers from line i
// /*! 
//  */
//MenuItem *MenuInfo::getnext(int i,int plusorminus)
//{
//*** redo to not depend on parent/menu!
//	int i=menu->findLine(this);
//	return menu->findFromLine(i+plusorminus);
//}

//! Get the element at line number i, including all open submenus and separators.
MenuItem *MenuInfo::findFromLine(int i)
{
	if (i<0) return NULL;
	MenuItem *m=NULL;
	for (int c=0; c<menuitems.n; c++) {
		if (menuitems.e[c]->state&LAX_HIDDEN) continue;
		if (i==0) return menuitems.e[c];
		i--;
		if (menuitems.e[c]->state&MENU_HAS_SUBMENU && menuitems.e[c]->state&LAX_OPEN) {
			MenuInfo *minfo=menuitems.e[c]->GetSubmenu();
			if (minfo) {
				if (m=minfo->findFromLine(i), m!=NULL)  return m;
				else i-=minfo->howmany(-1,0);
			}
		}
	}
	// i should still be >0 here
	return NULL;
}

//! Find the MenuItem anywhere that has checkid as its id.
/*! Searches recursively through all submenus. When changing states, labels, ordering, etc.
 * from the outside, this is handy.
 *
 * Any potential submenus that do not currently exist are skipped.
 */
MenuItem *MenuInfo::findid(int checkid)
{
	MenuItem *m=NULL;
	for (int c=0; c<menuitems.n; c++) {
		if (menuitems.e[c]->id==checkid) return menuitems.e[c];
		if (menuitems.e[c]->state&MENU_HAS_SUBMENU) {
			MenuInfo *minfo=menuitems.e[c]->GetSubmenu(0);
			if (minfo) 
				if (m=minfo->findid(checkid), m!=NULL) return m;
		}
	}
	return NULL;
}

int MenuInfo::n()
{
	return menuitems.n;
}

MenuItem *MenuInfo::e(int i)
{
	if (i<0 || i>=menuitems.n) return NULL;
	return menuitems.e[i];
}

//! Check to see if an id==check exists somewhere in the menu hierarchy.
/*! if look is NULL, then default is to start checking in this->menu.
 */
int MenuInfo::idexists(int check,MenuInfo *look) //look=NULL
{
	if (look==NULL) look=this;
	for (int c=0; c<look->menuitems.n; c++) {
		if (look->menuitems.e[c]->id==check) return 1;
		if (look->menuitems.e[c]->state&MENU_HAS_SUBMENU) {
			MenuInfo *minfo=look->menuitems.e[c]->GetSubmenu(0);
			if (minfo) if (idexists(check,minfo)) return 1;
		}
	}
	return 0;
}

//! Return a unique id number, starting with trythis.
/*! Cheap, inefficient function to simply return a number 1 larger than the current largest id.
 *
 * If trythis<=0, then start looking with 1.
 */
int MenuInfo::getuniqueid(int trythis) //trythis=-1
{
	if (trythis==-1) trythis=1;
	while (idexists(trythis,this)) trythis++;
	return trythis;
}

//! Set the title if the curmenu to ntitle.
void MenuInfo::NewTitle(const char *ntitle)
{ 
	if (curmenu->title!=NULL) delete[] curmenu->title;
	curmenu->title=new char[strlen(ntitle)+1];
	strcpy(curmenu->title,ntitle);
}

/*! If ignoreunmade!=0, then do not use GetSubmenu(), which will generate unmade submenus.
 * You would want to do this, for instance, to avoid FileMenuItem generation of the entire harddrive.
 *
 * Returns number of items affected.
 */
int MenuInfo::SetRecursively(unsigned long nstate, int on, int ignoreunmade)
{
	MenuInfo *sub;
	int n=0;
	for (int c=0; c<menuitems.n; c++) {
		n++;
		if (on) menuitems.e[c]->state|=nstate;
		else menuitems.e[c]->state&=~nstate;
		sub=menuitems.e[c]->GetSubmenu(ignoreunmade ? 0 : 1);
		if (sub) n+=sub->SetRecursively(nstate,on,ignoreunmade);
	}
	return n;
}

void MenuInfo::ClearSearch()
{ SetRecursively(MENU_SEARCH_HIT|MENU_SEARCH_PARENT|MENU_SEARCH_HIDDEN, 0, 1); }

//! Progressively search for items, caseless+partial.
/*! Sets MENU_SEARCH_HIT on matching items.
 *  Sets MENU_SEARCH_PARENT on items that don't match, but who has descendents.
 *
 *  If isprogressive, then assume that search is a string that contains the previous search.
 *
 *  Return number of hits.
 */
int MenuInfo::Search(const char *search, int isprogressive, int ignoreunmade)
{
	if (!isprogressive) {
		 //clear old search results
		SetRecursively(MENU_SEARCH_HIT|MENU_SEARCH_PARENT|MENU_SEARCH_HIDDEN, 0, 1);
	}

	MenuInfo *sub;
	MenuItem *p;
	int n=0;
	for (int c=0; c<menuitems.n; c++) {
		if (menuitems.e[c]->name && strcasestr(menuitems.e[c]->name, search)) {
			 //got one!
			n++;
			menuitems.e[c]->state|=MENU_SEARCH_HIT;
			DBG cerr <<" SEARCH \""<<search<<"\" hit: "<<menuitems.e[c]->name<<endl;

			 //set parents to force visible
			p=parent;
			while (p) {
				p->state|=MENU_SEARCH_PARENT;
				if (p->parent) p=p->parent->parent;
				else p=NULL;
			}
		} else {
			menuitems.e[c]->state|=MENU_SEARCH_HIDDEN;
			menuitems.e[c]->state&=~MENU_SEARCH_HIT;
		}

		 //check subitems
		sub=menuitems.e[c]->GetSubmenu(ignoreunmade ? 0 : 1);
		if (sub) n+=sub->Search(search,isprogressive,ignoreunmade);
	}
	return n;
}


} // namespace Laxkit

