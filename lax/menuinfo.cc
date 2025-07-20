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
//    Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/menuinfo.h>

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
 * Usage:
 * <pre>
 *  menu.Add("blah1",id=1,grayed)
 *  menu.Add("blah2",2)
 *  menu.SubMenu([title])
 *   menu.Add("a")
 *   menu.AddToggleItem("text", icon);
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


//-------------------------- Comparison functions

int menu_strcmp(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return strcmp(i1->GetString(detail1), i2->GetString(detail1)); }

int menu_strcasecmp(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return strcasecmp(i1->GetString(detail1), i2->GetString(detail1)); }

int menu_reversestrcmp(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return -strcmp(i1->GetString(detail1), i2->GetString(detail1)); }

int menu_reversestrcasecmp(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return -strcasecmp(i1->GetString(detail1), i2->GetString(detail1)); }

int menu_strcmp123(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return atof(i1->GetString(detail1))<atof(i2->GetString(detail2))?-1:(atof(i1->GetString(detail1))>atof(i2->GetString(detail2))?1:0); }

int menu_strcmp321(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return atof(i1->GetString(detail1))>atof(i2->GetString(detail2))?-1:(atof(i1->GetString(detail1))<atof(i2->GetString(detail2))?1:0); }


int strcmpInfo(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return i1->GetDetail(detail1)->info < i2->GetDetail(detail2)->info ? -1 : ((i1->GetDetail(detail1)->info > i2->GetDetail(detail2)->info) ? 1 : 0); }

int strcmpInfoRev(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return i1->GetDetail(detail1)->info > i2->GetDetail(detail2)->info ? -1 : ((i1->GetDetail(detail1)->info < i2->GetDetail(detail2)->info) ? 1 : 0); }


/*! Compare strings that hold some byte size string, like "1 byte", "2.5G", "25 mb".
 * This is a very simple check, only looks for 'b', 'm', 'g', or 't' as first char after number.
 *
 * Return 0 for equal, -1 for a<b, 1 for a>b.
 *
 * \todo this should probably have a localized variation.
 */
int cmpKB(const char *a, const char *b)
{
	if (!a || !b) return 0;

	double ad, bd;
	char *endptr=NULL;

	ad=strtod(a, &endptr);
	if (endptr!=a) {
		a=endptr;
		while (isspace(*a)) a++;

		if      (*a=='k') ad*=1024;
		else if (*a=='m') ad*=1024*1024;
		else if (*a=='g') ad*=1e+9; //ok, i know this is fudging a little
		else if (*a=='t') ad*=1e+12;
	}

	bd=strtod(b, &endptr);
	if (endptr!=b) {
		b=endptr;
		while (isspace(*b)) b++;

		if      (*b=='k') bd*=1024;
		else if (*b=='m') bd*=1024*1024;
		else if (*b=='g') bd*=1e+9;
		else if (*b=='t') bd*=1e+12;
	}

	if (ad==bd) return 0;
	if (ad<bd) return -1;
	return 1;
}

int menu_strcmp321kb(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return cmpKB(i1->GetString(detail1), i2->GetString(detail2)); }

int menu_strcmp123kb(MenuItem *i1,int detail1, MenuItem *i2,int detail2)
{ return -cmpKB(i1->GetString(detail1), i2->GetString(detail2)); }


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
	parent     = nullptr;
	id         = 0;
	name       = nullptr;
	key        = nullptr;
	image      = nullptr;
	submenu    = nullptr;
	state      = LAX_OFF;
	subislocal = 0;
	info       = 0;
	extra      = nullptr;
	x = y = w = h = 0;

	formathint = 0;
	nextdetail = nullptr;
}

//! Item constructor, straight copy of all fields.
MenuItem::MenuItem(const char *newitem,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal)
{ 
	base_init(newitem,nid,nstate,ninfo,nsub,sublocal);
}

void MenuItem::base_init(const char *newitem,int nid,unsigned int nstate,int ninfo,MenuInfo *nsub,int sublocal)
{
	parent     = nullptr;
	id         = nid;
	state      = nstate;
	submenu    = nsub;
	subislocal = sublocal;
	name       = nullptr;
	key        = nullptr;
	image      = nullptr;
	extra      = nullptr;
	makestr(name, newitem);
	info = ninfo;
	x = y = w = h = 0;

	formathint = 0;
	nextdetail = nullptr;
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
	image = ImageLoader::LoadImage(img);
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
	delete[] key;
	delete[] name;
	if (image) image->dec_count();
	if (extra) extra->dec_count();
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

void MenuItem::SetExtra(anObject *obj, bool absorb)
{
	if (obj != extra) {
		if (extra) extra->dec_count();
		extra = obj;
	}
	if (!absorb && extra) extra->inc_count();
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

int MenuItem::Open()
{
	int old=state&LAX_OPEN;
	state|=LAX_OPEN;
	return old;
}

/*! Return old state.
 */
int MenuItem::Close()
{
	int old=state&LAX_OPEN;
	state&=~LAX_OPEN;
	return old;
}

//! Return 1 for definitely hidden. 2 for hidden, except a child is a search hit.
int MenuItem::hidden()
{
	int h=state&(LAX_HIDDEN|MENU_SEARCH_HIDDEN);
	if (h && (state&MENU_SEARCH_PARENT)) return 2;
	if (h) return 1;
	return 0;
}

/*! yes==-1 means toggle LAX_HIDDEN in state.
 * Otherwise, 0 clears hide, 1 installs hide.
 * Returns new current hidden.
 */
int MenuItem::Hide(int yes)
{
	if (yes<0) state^=LAX_HIDDEN;
	else if (yes) state|=LAX_HIDDEN;
	else state&=~LAX_HIDDEN;
	if (state&LAX_HIDDEN) return 1;
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

/*! Return the ith detail of *this. i==0 corresponds to *this.
 * Return NULL for i not found.
 */
MenuItem *MenuItem::GetDetail(int i)
{
	MenuItem *ii=this;
	do {
		if (i==0) return ii;
		ii=ii->nextdetail;
		i--;
	} while (ii);
	return NULL;
}

/*! Return the number of subdetails (not including *this).
 */
int MenuItem::NumDetail()
{
	int n = 0;
	MenuItem *i = this;
	while (i->nextdetail) { n++; i=i->nextdetail; }
	return n;
}

//! Install detail to end of nextdetail list.
int MenuItem::AddDetail(MenuItem *detail)
{
	MenuItem *next=this;
	while (next->nextdetail) next=next->nextdetail;

	next->nextdetail=detail;
	return 0;
}

//! Add item detail to the item at the top of menuitems, or to towhich.
/*! If there is no recent item, then return 1. On success, return 0.
 */
int MenuItem::AddDetail(const char *newitem,LaxImage *img,int nid,int ninfo)
{
	MenuItem *mi=new MenuItem(newitem,img,nid,LAX_OFF,ninfo,NULL,0);
	AddDetail(mi);
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
 * This class stores lists that are used by TreeSelector, StrSliderPopup, and PopupMenu for instance.
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
 * See MenuSortStyles.
 */

void menuinfoDump(MenuInfo *menu, int indent)
{
	if (!menu) return;

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	MenuItem *detail;
	for (int c=0; c<menu->menuitems.n; c++) {
		detail = menu->menuitems.e[c];
		cerr <<spc<<"Item "<<c<<": ";

		while (detail) {
			cerr <<(detail->name ? detail->name : "(no name)");
			detail=detail->nextdetail;
			if (detail) cerr <<", ";
		}
		cerr<<endl;

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
	Compare=menu_strcmp;
	parent=NULL;
	sortstyle=0;
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
	return AddItem((name?name:""), 0, 0, nullptr, -1, LAX_GRAY|LAX_SEPARATOR);
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
	if (end < 0) end = curmenu->menuitems.n-1;
	if (start >= end) return;

	int s,e;
	MenuItem *mid = curmenu->menuitems.e[end];
	MenuItem *mi;
	s = start;
	e = end;

	while (s <= e) {
		if (Compare(curmenu->menuitems.e[s],detail, mid,detail)<0) { s++; continue; }
		if (Compare(curmenu->menuitems.e[e],detail, mid,detail)>0) { e--; continue; }

		//DBG cerr <<"=== sort swap: "<<s<<','<<e<<endl;
		mi = curmenu->menuitems.e[s];
		curmenu->menuitems.e[s] = curmenu->menuitems.e[e];
		curmenu->menuitems.e[e] = mi;
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
	sort(0, curmenu->menuitems.n-1, detail);

	if (sortstyle & SORT_DIRS_FIRST) {
		for (int c=0; c<curmenu->menuitems.n; c++) {
			if (curmenu->menuitems.e[c]->hasSub()) continue;
			//now we are on a non-sub
			for (int c2=c+1; c2<curmenu->menuitems.n; c2++) {
				if (curmenu->menuitems.e[c2]->hasSub()) {
					curmenu->menuitems.slide(c2, c);
					break;
				}
			}
		}
	}
}

//! Set the compare function to some "int (*func)(const char *,const char *).
/*! If func==NULL, then reset Compare to strcmp.
 */
void MenuInfo::SetCompareFunc(CompareFunc func)
{
	if (func) Compare=func; else Compare=menu_strcmp;
	//Sort();
}


//! Set Compare to the appropriate function for newsortstyle
void MenuInfo::SetCompareFunc(int newsortstyle)
{
	sortstyle=newsortstyle;

	if (sortstyle&SORT_ABC) {
		if (sortstyle&(SORT_IGNORE_CASE))
			 Compare=menu_strcasecmp;
		else Compare=menu_strcmp;

	} else if (sortstyle&SORT_CBA)  {
		if (sortstyle&(SORT_IGNORE_CASE))
			 Compare=menu_reversestrcasecmp;
		else Compare=menu_reversestrcmp;

	} else if (sortstyle&SORT_INFO)     Compare=strcmpInfo;
	else   if (sortstyle&SORT_INFO_REV) Compare=strcmpInfoRev;
	else   if (sortstyle&SORT_123)      Compare=menu_strcmp123;
	else   if (sortstyle&SORT_321)      Compare=menu_strcmp321;
	else   if (sortstyle&SORT_123kb)    Compare=menu_strcmp123kb;
	else   if (sortstyle&SORT_321kb)    Compare=menu_strcmp321kb;

	else Compare=menu_strcmp; //**** default??
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

int MenuInfo::AddToggleItem(const char *newitem, int nid, int ninfo, bool on, LaxImage *img, int where, int state)
{
	return AddItem(newitem, nid, ninfo, img, where, state | LAX_OFF | LAX_ISTOGGLE | (on ? LAX_CHECKED : 0));
}

//! Add item at position where, or to end of menu if where<0.
/*! Please note that the MenuItem that is created is local to the menuitems stack (it will be deleted
 *  when the item stack is flushed), while the passed in submenu is local according to subislocal.
 *
 * \todo why is this img not incremented but other constructor is??
 */
int MenuInfo::AddItem(const char *newitem, int nid, int info, LaxImage *img, int where, int state)
{
	MenuItem *mi = new MenuItem(newitem,img,nid, state, info, NULL,0);
	curmenu->menuitems.push(mi,1,where);
	mi->parent = curmenu;
	return curmenu->menuitems.n;
}

/*! A shortcut to add something like "group 1/subgroup/item" to submenu.
 * Adds any non-existing intermediate groups (with id==0). Afterwards, curmenu is unchanged.
 *
 * where is an index in the final level (-1 for at end).
 */
int MenuInfo::AddDelimited(const char *newitem, char delimiter, int nid, int ninfo, LaxImage *img, int where, int state)
{
	if (!newitem || !*newitem) return 1;
	
	const char *st = strchr(newitem,delimiter);
	if (!st) return AddItem(newitem, nid, ninfo, img, where, state);
	
	char *group=newnstr(newitem,st-newitem);
	int i = curmenu->findIndex(group);
	if (i<0) {
		AddItem(group,where);
		SubMenu();
	} else {
		SubMenu(NULL,i);
	}
	AddDelimited(st+1, delimiter, nid, ninfo, img, where, state);
	EndSubMenu();

	return 0;
}

/*! Return the top item of curmenu. This is typically the last item you added.
 */
MenuItem *MenuInfo::Top()
{
	if (!curmenu) return NULL;
	if (!curmenu->menuitems.n) return NULL;
	return curmenu->menuitems.e[curmenu->menuitems.n-1];
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

/*! Returns 0 if element found, else 1 if not found.
 */
int MenuInfo::Remove(int which)
{
	return curmenu->menuitems.remove(which);
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
int MenuInfo::how_many(int maxn,int all) // maxn=0, maxn is only consider menuitems.e[c<maxn]
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
				if (minfo) n+=minfo->how_many(-1,all);
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
				i+=minfo->how_many(-1,0);
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

//! Get the element at line number i, including all open submenus and separators, and skipping hidden.
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
				else i-=minfo->how_many(-1,0);
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
MenuItem *MenuInfo::findid(int checkid, int *index_ret)
{
	MenuItem *m = nullptr;
	for (int c = 0; c < menuitems.n; c++) {
		if (menuitems.e[c]->id == checkid) {
			if (index_ret) *index_ret = c;
			return menuitems.e[c];
		}
		if (menuitems.e[c]->state & MENU_HAS_SUBMENU) {
			MenuInfo *minfo = menuitems.e[c]->GetSubmenu(0);
			if (minfo) 
				if (m = minfo->findid(checkid, index_ret), m != nullptr) return m;
		}
	}
	if (index_ret) *index_ret = -1;
	return nullptr;
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

//! Progressively search for items, caseless+partial. search==NULL matches all.
/*! Sets MENU_SEARCH_HIT on matching items.
 *  Sets MENU_SEARCH_PARENT on items that don't match, but who has matching descendents.
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
		if (!search || (menuitems.e[c]->name && strcasestr(menuitems.e[c]->name, search))) {
			 //got one!
			n++;
			menuitems.e[c]->state|=MENU_SEARCH_HIT;
			DBG cerr <<" SEARCH \""<<(search?search:"(null)")<<"\" hit: "<<menuitems.e[c]->name<<endl;

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

