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


#include <lax/menuselector.h>
#include <lax/laxutils.h>

#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {
	
/*! \class MenuSelector
 * \ingroup menuthings
 * \brief General list and menu controller. Base class for PopupMenu.
 *
 * \code
 * #include <lax/menuselector.h>
 * \endcode
 * 
 * \todo *** need style USE_ALL_OPEN, where all open submenus are used, should also
 * be able to toggle wether the actual item text is displayed when the sub stuff is used:
 * or put that in a TreeSelector??
 * item
 * -item-  <--item has open sub menu, the submenu's title is ignored
 * sub1
 * -sub2-   <--sub2 has open sub menu, the submenu's title is ignored
 * subsub1
 * subsub1
 *
 * For speed, all lines have the same height. This is really a little bit of a
 * cop-out for not wanting to program for lines that have differing heights.
 * 
 * Please note that the ridiculously large number of style flags are passed in through a dedicated 
 * long long in the constructor, rather than the normal or'ing with the anXWindow::win_style flags. These
 * menu flags are stored in menustyle.
 * 
 * This class handles only one level of a menu. Multiple levels are 
 * handled in the more specialized derived classes TreeSelector (***todo!) and PopupMenu.
 * All items can be selected. They are indicated by being highlighted (or having a
 * status graphic checked).
 * There is a separate highlighting corresponding to a current item on which the next action will
 * be performed. For instance, say you press up or down, you are not necessarily selecting
 * the items that are up or down, but if you then press space, then that curitem
 * is selected. This 'user focused' item is stored in ccuritem. The last item
 * to have its state actually changed is stored in curitem.
 *
 * Items can be displayed in 1 long vertical list. Alternately, 
 * the items fill columns the size of the window, and columns extend off to the 
 * left and right. A third (*** not implemented) style has how ever many columns
 * fit across the width of the window, and the contents extend off the top and bottom.
 * Items fill downward in the first column, then fill the second column, etc.
 * 
 * MenuItems that have LAX_ISTOGGLE are allowed to be checked on and off. This is 
 * different then being selected/highlighted. Items
 * can also be hidden (***not implemented), but still be stored in the stack.
 *
 * \todo *** implement hidden items
 *
 * This class uses a PanController such that
 * the panner wholebox (min and max) is the bounding box of all the items,
 * and the selbox (start and end) is inrect. However, any shifting changes 
 * the panner's selbox, but this does not trigger any change in inrect.
 * 
 * There are three standard color schemes to choose from. First, there are the standard
 * button colors for text and background (MENUSEL_BUTTONCOLORS). Second are the 
 * standard text edit colors (MENUSEL_TEXTCOLORS). And last
 * and default are the standard menu panel colors.
 * 
 * \code
 *   // Item placement and display flags
 *  #define MENUSEL_CHECK_ON_RIGHT       (1LL<<0)
 *  #define MENUSEL_CHECK_ON_LEFT        (1LL<<1)
 *  #define MENUSEL_SUB_ON_RIGHT         (1LL<<2)
 *  #define MENUSEL_SUB_ON_LEFT          (1LL<<3)
 *  #define MENUSEL_STATUS_ON_RIGHT      (1LL<<4)
 *  #define MENUSEL_STATUS_ON_LEFT       (1LL<<5)
 *  #define MENUSEL_GRAPHIC_ON_RIGHT     (1LL<<6)
 *  #define MENUSEL_GRAPHIC_ON_LEFT      (1LL<<7)
 *  #define MENUSEL_SUB_FOLDER           (1LL<<8)
 *  #define MENUSEL_SUB_ARROW            (1LL<<9)
 *  #define MENUSEL_CENTER               (1LL<<10)
 *  #define MENUSEL_LEFT                 (1LL<<11)
 *  #define MENUSEL_RIGHT                (1LL<<12)
 *  #define MENUSEL_TAB_JUSTIFY          (1LL<<13)
 *  #define MENUSEL_CHECKS               (1LL<<14)
 *  #define MENUSEL_CHECKBOXES           (1LL<<15)
 *  #define MENUSEL_SHOW_LEAF_ONLY       (1LL<<16)
 *  #define MENUSEL_SHOW_SUB_ONLY        (1LL<<17)
 *  #define MENUSEL_SCROLLERS            (1LL<<18)
 *  #define MENUSEL_USE_TITLE            (1LL<<19)
 *  #define MENUSEL_BUTTONCOLORS         (1LL<<20)
 *  #define MENUSEL_TEXTCOLORS           (1LL<<21)
 *  #define MENUSEL_FIT_TO_WIDTH         (1LL<<22)
 *  #define MENUSEL_FIT_TO_HEIGHT        (1LL<<23)
 *  #define MENUSEL_TREE                 (1LL<<24)
 *  #define MENUSEL_COMPOUND_TREE        (1LL<<25)
 *  
 *  #define MENUSEL_SORT_NUMBERS         (1LL<<26)
 *  #define MENUSEL_SORT_LETTERS         (1LL<<27)
 *  #define MENUSEL_SORT_REVERSE         (1LL<<28)
 *  #define MENUSEL_SORT_SUBS_FIRST      (1LL<<29)
 *  #define MENUSEL_SORT_IGNORE_CASE     (1LL<<30)
 *  #define MENUSEL_SORT_BY_EXTENSIONS   (1LL<<31)
 *  
 *  #define MENUSEL_USE_DOT              (1LL<<32)
 *  #define MENUSEL_USE_DOT_DOT          (1LL<<33)
 *  #define MENUSEL_USE_UP               (1LL<<34)
 *  #define MENUSEL_USE_BACK             (1LL<<35)
 *  
 *  #define MENUSEL_ONE_ONLY             (1LL<<36)
 *  #define MENUSEL_ZERO_OR_ONE          (1LL<<37)
 *  #define MENUSEL_SELECT_ANY           (1LL<<38)
 *  #define MENUSEL_SELECT_LEAF_ONLY     (1LL<<39)
 *  #define MENUSEL_SELECT_SUB_ONLY      (1LL<<40)
 *  #define MENUSEL_CURSSELECTS          (1LL<<41)
 *  #define MENUSEL_FOLLOW_MOUSE         (1LL<<42)
 *  #define MENUSEL_GRAB_ON_MAP          (1LL<<43)
 *  #define MENUSEL_GRAB_ON_ENTER        (1LL<<44)
 *  #define MENUSEL_OUT_CLICK_DESTROYS   (1LL<<45)
 *  #define MENUSEL_CLICK_UP_DESTROYS    (1LL<<46)
 *  #define MENUSEL_DESTROY_ON_LEAVE     (1LL<<47)
 *  #define MENUSEL_DESTROY_ON_FOCUS_OFF (1LL<<48)
 *  
 *  #define MENUSEL_REARRANGEABLE        (1LL<<49)
 *  #define MENUSEL_EDIT_IN_PLACE        (1LL<<50)
 *  // *** add items?? (more than just editing names)
 *  // *** remove items??
 *  
 *  #define MENUSEL_SEND_ON_UP           (1LL<<51)
 *  #define MENUSEL_SEND_ON_ENTER        (1LL<<52)
 *  #define MENUSEL_SEND_IDS             (1LL<<53)
 *  #define MENUSEL_SEND_STRINGS         (1LL<<54)
 * \endcode
 * <pre>
 *  //... remember that the buck stops with (1<<63)
 *
 * 
 * TODO: label in 2 parts: "Blah\t+t" --> 'Blah    +t' (2 part labels, also 
 * underline character for hot keys??), rearrange, edit in place
 *   TODO
 *   *** RemoveItem
 *   *** put in a newstate(int newstate,int drawnow)/toggle(item,-1)
 *   *** when mouseover up arrow or autoscroll up, moving down should not shiftscreen
 *   *** scrollbar (x/y) versus up/down arrows and right click dragging??
 *   *** Menu
 *   		| Blah        ^x | <-- parse item in two parts, "Blah\t^x" -> "Blah" ---> "^x"??
 *   		| Yack       +^y |
 *   		| Pshaww      ^p |
 *   	Shift:   + // 	Control: ^ // 	Alt:     * // 	Mod4:    ~
 *   		  -  <-- key menu shortcut (on P) No!! use the progressive scanning? both?
 *   		  
 *   *** non-constant item height, easier facility for graphic+text??
 *   *** menu groups, so you can push groups of things onto a single menu, without
 *   		the need of poping up a submenu!! Quite useful for many of the potential
 *   		menus in laidout ==== this is basically a tree, without all the tree lines...
 *   *** neat from fltk:
 *    		Add("File")
 *    		Add("File/Save")
 *    		Add("File/Save As")....
 * </pre>
 * \todo  a lot of stuff, see class MenuSelector
 */
/*! \var MenuInfo *MenuSelector::menu
 * \brief Stores the actual menu items.
 *
 * The MenuInfo may be owned by MenuSelector or not, based on whether menuislocal is 
 * nonzero or not. If it is non-zero, then the destructor calls 'delete menu'.
 *
 */
/*! \var char MenuSelector::menuislocal
 * \brief Whether menu is local to this MenuSelector.
 */
/*! \var int MenuSelector::checkw
 * \brief The width of the check mark graphic of a menu item. (only one width, not per item)
 */
/*! \var int MenuSelector::subw
 * \brief The width of the sub menu indicator graphic of a menu item. (only one width, not per item)
 */
/*! \var int MenuSelector::sgw
 * \brief The width of the status graphic element of a menu item. (only one width, not per item)
 */
/*! \var int MenuSelector::firstinw
 * \brief The equivalent index of the first line in the window.
 *
 * This could be a negative value, and so should not be interpreted as
 * an index into the menuitems stack. It is useful for instance when popping up menus
 * and you want the current selection to be near the mouse, no matter where the actual
 * bounds of the window are.
 *
 * \todo *** should remove this variable, i don't think it is needed
 */
/*! \var int MenuSelector::padg
 * \brief The pad to place between text and other graphic elements.
 */
/*! \var int MenuSelector::textheight
 * \brief The height of lines are leading+(text height).
 *
 * Any highlighting only includes the textheight pixels.
 */
/*! \var int MenuSelector::leading
 * \brief The height of lines are leading+(text height).
 *
 * Any highlighting does not include the leading pixels.
 */
/*! \var MenuItem *MenuSelector::curmenuitem
 * \brief The MenuItem corresponding to curitem.
 */
/*! \var int MenuSelector::curitem
 * \brief The last selected or deselected item.
 */
/*! \var int MenuSelector::ccuritem
 * \brief The item which one has cursored to.
 */
/*! \var int MenuSelector::pagesize
 * \brief The number of lines that can be comfortably fit in inrect.
 *
 * That is, pagesize=(inrect.height+leading)/(textheight+leading)
 */
/*! \var int MenuSelector::columnsize
 * \brief The maximum number of actual elements in any the columns.
 *
 * When the style is wrapping to the height, columnsize should be equal to pagesize.
 */
/*! \var NumStack<int> MenuSelector::columns;
 * \brief Each element of the stack holds the width of that column.
 *
 * So of course, columns.n is the number of columns. The height of a full
 * column is just pagesize*(textheight+leading).
 */
/*! \var int MenuSelector::mousedragged
 * \brief Flag for whether the mouse has been dragged since a button was down.
 *
 * If it is a simple move, then mousedragged==1. If the menuselector is REARRANGEABLE,
 * and items are in the process of being moved, then mousedragged==2.
 */
/*! \fn virtual int MenuSelector::Curitem()
 * \brief Return curitem, which is the last item whose state was toggled.
 *
 * Note that this is not necessarily ccuritem which is what one might go to
 * with the arrow keys without actually selecting.
 */
/*! \fn virtual const MenuItem *Item(int c)
 * \brief Publically accessible func to do: return item(c); 
 */
/*! \var unsigned long MenuSelector::highlight
 * \brief Highlight color of beveled graphics.
 */
/*! \var unsigned long MenuSelector::shadow
 * \brief Shadow color of beveled graphics.
 */


//! Constructor.
/*! If you pass in a MenuInfo, you can also pass in whether the MenuSelector should
 * be resposible for deleting it (menuislocal==1) or not. If usethismenu is
 * NULL, then a new MenuInfo is created and menuislocal is set to 1. If usethismenu is
 * not NULL, then the value of nmenuislocal is used. The default is 1: for the MenuSelector
 * to take possession of the MenuInfo. This allows programs to create a menuinfo themselves,
 * and then not worry about deleting it, because the selector will delete it.
 *
 * Sets up colors, and pushes 0 onto the columns stack.
 */
//	ScrolledWindow(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//		int xx,int yy,int ww,int hh,int brder,
//		anXWindow *prev,Window nowner=None,const char *nsend=NULL,
//		PanController *pan=NULL,int npislocal=0);
MenuSelector::MenuSelector(anXWindow *parnt,const char *nname,const char *ntitle,
				unsigned long nstyle,  //!< Holds the usual anXWindow style flags
				int xx,int yy,int ww,int hh,int brder,
				anXWindow *prev,unsigned long nowner,const char *atom,
				unsigned long long nmstyle, //!< Holds the menu style defines
				MenuInfo *usethismenu, //!< Pass in a MenuInfo class, if not NULL is assumed to not be local
				char absorb_count      //!< Whether the passed in menu's count should be absorbed. Default is 1
			) //nowner=0,atom=0, ncid=0, nminfo=NULL,nmemislocal=1
					: ScrolledWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,atom,NULL)
{
	mousedragged=0;
	firstinw=0;
	pad=3;
	textheight=0;
	lineheight=0;
	pagesize=columnsize=0;
	ccuritem=curitem=0;
	curmenuitem=NULL;
	timerid=0;
	leading=0;
	menustyle=nmstyle;
//	panner->pan_style|=PANC_ALLOW_SMALL;

	if ((menustyle&(MENUSEL_CHECK_ON_LEFT|MENUSEL_CHECK_ON_RIGHT))==0)
		menustyle|=MENUSEL_CHECK_ON_LEFT;

	if (menustyle&MENUSEL_BUTTONCOLORS) {
		installColors(app->color_buttons);
	} else if (menustyle&MENUSEL_TEXTCOLORS) {
		installColors(app->color_edits);
	} else { // just use regular panel colors
		installColors(app->color_panel);
	}

	if (usethismenu) {
		menu=usethismenu;
		if (!absorb_count) menu->inc_count();
	} else {
		menu=new MenuInfo;
	}
	
	columns.push(0);
}

//! Destructor, just does if (menu && menuislocal) delete menu.
MenuSelector::~MenuSelector()
{
	if (menu) menu->dec_count();
}

//! Return a new'd int array of which items are in the given state, if there are any, else NULL.
/*! Same state means MenuItem::state&LAX_MSTATE_MASK == state.
 *
 * The first element is the number of selected items.
 * The calling code is responsible for deleting the array.
 */
int *MenuSelector::WhichSelected(unsigned int state)
{
	int c,n=numItems();
	NumStack<int> w;
	w.push(-1);
	for (c=0; c<n; c++) {
		if ((item(c)->state&LAX_MSTATE_MASK)==state) w.push(c);
	}
	w.e[0]=w.n-1;
	if (w.n>1) return w.extractArray();
	return NULL;
}

//! Return how many items are currently selected
int MenuSelector::NumSelected()
{
	int c,n=numItems();
	int nn=0;
	for (c=0; c<n; c++) {
		if ((item(c)->state&LAX_MSTATE_MASK)==LAX_ON) nn++;
	}
	return nn;
}


//! Find the maximum width of (text+ pad+ graphic+ statusgraphicw+ checkwidth+ subw) of items in range [s,e]
/*! Clamp s and e to [0,numItems()-1]. Return -1 if e<s.
 *
 * If the corresponding style (like for submenu indicator for instance) is not
 * set in menustyle, then that width is not included.
 *
 * Return the maximum height in h_ret.
 */
int MenuSelector::findmaxwidth(int s,int e, int *h_ret)
{
	if (e<s) return -1;
	if (s<0) s=0;
	if (e<0) e=0;
	if (s>=numItems()) s=numItems()-1;
	if (e>=numItems()) e=numItems()-1;
	if (s<0 || e<0) return 0;
	double w=0,h=0,x=0,y=0,t;
	MenuItem *mitem;

	for (int c=s; c<=e; c++) {
		mitem=item(c);
		getgraphicextent(mitem,&x,&y);
		if (y>h) h=y;
		if (x) t=x+padg; else t=0;
		t+=getextent(mitem->name,-1,NULL,NULL);
		if (t>w) w=t;
	}
	if (menustyle&(MENUSEL_CHECK_ON_LEFT|MENUSEL_CHECK_ON_RIGHT)) w+=checkw;
	if (menustyle&(MENUSEL_SUB_ON_LEFT|MENUSEL_SUB_ON_RIGHT)) w+=subw;
	if (menustyle&(MENUSEL_STATUS_ON_LEFT|MENUSEL_STATUS_ON_RIGHT)) w+=sgw;

	if (h_ret) *h_ret=h;
	return w;
}

//! Make item which be near screen coordinate x,y
/*! Returns ???***.
 *
 * \todo *** no bounds check, should at least make sure something is in window?
 *
 * \todo *** fix me!
 */
int MenuSelector::SetFirst(int which,int x,int y)
{//***
//	firstinw=which;
//	needtodraw=1;
//	return firstinw;
	return 0;
}

//! Set some values that are derived from other values (pagesize, hightlight, shadow, ...).
/*! Sets leading to have a little bit extra if MENUSEL_CHECKBOXES and not otherwise specified.
 * Also sets the shadow and hightlight colors, and determines the pagesize.
 */
int MenuSelector::init()
{ 
	ScrolledWindow::init(); //this calls syncWindows, causing inrect and pagesize to get set
    highlight=coloravg(win_colors->bg,rgbcolor(255,255,255));
	shadow   =coloravg(win_colors->bg,rgbcolor(0,0,0));

	if (leading==0) { if (menustyle&MENUSEL_CHECKBOXES) leading=pad; else leading=1; }
	if (textheight==0) SetLineHeight(app->defaultlaxfont->textheight()+leading,leading,0);
	
	DBG cerr <<"--"<<WindowTitle()<<": textheight, leading="<<textheight<<','<<leading<<endl;
	
	padg=textheight/2;
	if (menustyle&(MENUSEL_CHECKBOXES|MENUSEL_CHECKS)) sgw=textheight; else sgw=0;
	arrangeItems();
	return 0;
}

//! Setup columns based on inrect, then sync up the panner.
/*! Default is for single column. FIT_TO_HEIGHT causes multiple columns that
 * continue off horizontally. Assumes inrect has been found and arranges
 * items to fit in that. Sets columnsize to something appropriate.
 *
 * This sets pagesize if necessary.
 *
 * Syncs the panner such that the wholebox of the panner is the newly found wholerect,
 * and the selbox is inrect. However, any shifting changes the panner's selbox, but this
 * does not trigger any change in inrect.
 * 
 * If forwrapping!=0 then do not try to expand wholerect to fill up inrect, and do not
 * update the panner. This is when you are arranging items first, and then wrapping the
 * window to the extent of the items.
 * 
 * \todo ***Need to implement FIT_TO_WIDTH, or remove is as possibility...
 */
void MenuSelector::arrangeItems(int forwrapping)//forwrapping=0
{
	if (!pagesize && !textheight) return;
	if (!pagesize) pagesize=(inrect.height+leading)/(textheight+leading);
	columns.flush();
	IntRectangle wholerect;
	wholerect.x=inrect.x;
	wholerect.y=inrect.y;
	wholerect.width=wholerect.height=0;
	if (menustyle&MENUSEL_FIT_TO_WIDTH) { // variable height
		cout <<"***MENUSEL_FIT_TO_WIDTH not implemented!"<<endl;
	} // else if (menustyle&MENUSEL_FIT_TO_HEIGHT) {
	if (menustyle&MENUSEL_FIT_TO_HEIGHT) { // variable width, fixed height
		int numcols=numItems()/pagesize+1;
		int index;
		wholerect.height=pagesize*(textheight+leading);
		columnsize=pagesize;
		for (int c=0; c<numcols; c++) {
			index=c*pagesize;
			int w=findmaxwidth(index,index+pagesize-1,NULL);
			wholerect.width+=w;
			columns.push(w);
		}
	} else { // default single column with arrows, not scrollers..
		columns.push(findmaxwidth(0,numItems(),NULL));
		wholerect.width=columns.e[0];
		wholerect.height=numItems()*(textheight+leading);
		columnsize=numItems();
		if (columnsize==0) columnsize=pagesize;
	}

	if (!forwrapping) {
		 // distribute gap among the columns
		int gap;
		if (wholerect.width<inrect.width) {
			gap=inrect.width-wholerect.width;
			for (int c=0; c<columns.n; c++) {
				columns.e[c]+=gap/(columns.n-c);
				gap-=gap/(columns.n-c);
				if (gap<=0) break;
			}
			wholerect.width=inrect.width;
		}

		IntRectangle selbox=inrect;
		if (selbox.x<wholerect.x) selbox.x=wholerect.x;
		if (selbox.y<wholerect.y) selbox.y=wholerect.y;
		if (selbox.x+selbox.width>wholerect.x+wholerect.width) selbox.width=wholerect.x+wholerect.width-selbox.x;
		if (selbox.y+selbox.height>wholerect.y+wholerect.height) selbox.height=wholerect.y+wholerect.height-selbox.y;
		
		panner->SetWholebox(wholerect.x,wholerect.x+wholerect.width-1,wholerect.y,wholerect.y+wholerect.height-1);
		panner->SetCurPos(1,selbox.x,selbox.x+selbox.width-1);
		panner->SetCurPos(2,selbox.y,selbox.y+selbox.height-1);
	}
}

//! Default is to return the number of items in menu->menuitems stack.
/*! A tree class, for instance would return the number of items plus the items
 * from any open submenus.
 */
int MenuSelector::numItems()
{
	return menu->menuitems.n;
}

//! Return the item corresponding to screen index i.
/*! This is not necessarily a direct index into the menu->menuitems stack, though
 * that is the default behavior of this function.
 * 
 * If open menus are displayed with this menu, like in a tree,
 * then this function must be redefined to access the thing associated with
 * screen index i.
 *
 * This function does not implement a search cache, but provides the tag
 * for subclasses to use if they want.
 * 
 * \todo maybe have option to force remapping==bypassing any search cache??
 */
MenuItem *MenuSelector::item(int i,char skipcache)
{
//	if (!skipcache) {
//		if (i==curitem && curmenuitem) return curmenuitem;
//		// if (i==lastfoundi && lastfound) return lastfound;
//		//keep:
//		//	lastfound
//		//	lastfoundi, then can do some optimizing if you select 1 up or 1 down
//	}
	if (i<0 || i>menu->menuitems.n) return NULL;
	//lastfoundi=i;
	//return lastfound=menu->menuitems[i];
	return menu->menuitems.e[i];
}

//! Focus on draws the char over item.
int MenuSelector::FocusOn(const FocusChangeData *e)
{
	anXWindow::FocusOn(e);
	if (win_active) drawitem(ccuritem);
	return 0;
}

//! Focus off draws the char over item.
/*! Also, if MENUSEL_FOCUS_OFF_DESTROYS, then an off focus destroys this window.
 */
int MenuSelector::FocusOff(const FocusChangeData *e)
{
	DBG cerr <<"MenuSelector "<<WindowTitle()<<" FocusOff..."<<endl;
	anXWindow::FocusOff(e);
	if (!win_active) {
		if (menustyle&MENUSEL_DESTROY_ON_FOCUS_OFF) {
			app->destroywindow(this);
			return 0;
		}
		drawitem(ccuritem);
	}
	return 0;
}

//! Intercept LAX_onMapped in case MENUSEL_GRAB_ON_MAP is set.
/*! Grabs the pointer if that style is set.
 * In either case, it then properly calls 
 * anXWindow::Event().
 *
 * \todo *** is just selecting a default keyboard, which might be unacceptible
 */
int MenuSelector::Event(const EventData *e,const char *mes)
{
	DBG cerr <<"***********************menusel message:"<<e->type<<endl;
	if (e->type==LAX_onMapped && (menustyle&MENUSEL_GRAB_ON_MAP)) {
		app->setfocus(this,0,NULL);
		//***
//****turned off for debugging purposes!! Damnation! what's going on here?
//****need actually LEARN how to do focus handling properly!!!
//		if (XGrabPointer(app->dpy,window,False,ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
//				GrabModeAsync,GrabModeAsync,
//				None,None,CurrentTime)==GrabSuccess)
//			cout <<"MenuSelector GrabSuccess!"<<endl; 
//		else cout <<"MenuSelector GrabFailure!"<<endl; 

	} else if (e->type==LAX_onMouseOut && (menustyle&MENUSEL_DESTROY_ON_LEAVE)) {
		app->destroywindow(this);
	}

	return anXWindow::Event(e,mes);
}

//! Sort the items alphabetically by the name.
/*! t is passed on to MenuInfo::sortstyle.
 * t==0 means do the default sorting.
 *
 * \todo *** should be able to sort only a subset, for instance all 
 * items between separators.
 */
void MenuSelector::Sort(int t) 
{
	if (menu) menu->sort(0,numItems()-1,0);//*** this is now broken potentially cause numItems!=menuitems->n
}

//! Add a bunch of items all at once.
/*! Returns number of items added
 * \todo ***this is cheap, should be optimized for large arrays?? add in one lump, then sort??
 */
int MenuSelector::AddItems(const char **i,int n,int startid) // assume ids sequential, state=0
{
	if (i==NULL || n==0) return 0;
	for (int c=0; c<n; c++) AddItem(i[c],startid++,LAX_OFF);
	return n;
}

//! Add item with text i to the top index of menuitems.
/*! You can specify an initial state, such as LAX_GRAY, etc.
 *  Automatically sorts if sort is the style.
 *
 *  Returns the number of items in the stack, or -1 if error.
 *
 *  Does not synchronize the display. You must do that manually later on,
 *  unless init() hasn't been called yet. init() synchronizes the display.
 */
int MenuSelector::AddItem(const char *i,int nid,int newstate)
{ 
	if (!i) return -1;
	if ((newstate&LAX_MSTATE_MASK)==0) newstate|=LAX_OFF;
	int c=numItems();
	if (!(menustyle&MENUSEL_SORT_REVERSE)) {
		c=0;
		while (c<numItems() && strcmp(i,item(c)->name)>0) c++;
	} else if (menustyle&MENUSEL_SORT_REVERSE) {
		c=0;
		while (c<numItems() && strcmp(i,item(c)->name)<0) c++;
	}
	if (numItems()==0 && menustyle&MENUSEL_ONE_ONLY && (newstate&LAX_OFF)) // if only one item, make it on
		newstate=(newstate^LAX_OFF)|LAX_ON;
	menu->AddItem(i,nid,newstate,0,NULL,c,0); 
	return numItems();
}

//! Base MenuSelector currently just does text, so points w and h to 0.
/*! 
 * \todo *** incorporate icons/XImages/Pixmaps.. wait to do this until laxkit
 * has more definite image handling setup
 * 
 * Returns the width.
 */
double MenuSelector::getgraphicextent(MenuItem *mitem,//!< the item
								double *w, //!< Put the x extent here
								double *h //!< Put the y extent here
							)
{
	MenuItem *im=mitem;
	if (!im || !im->image) {
		*w=0;
		*h=0;
		return 0;
	}
	*w=im->image->w();
	*h=im->image->h();
	return 0;

}

//! Find extent of text+graphic+(pad between graphic and text).
/*! If the graphic extent is 0, then the pad is not included.
 * w and h must not be NULL! Note that the y extent is the actual
 * y extent of the graphic+text. The actual space on the window that the
 * line takes up is still textheight+leading.
 * 
 * Returns the x extent.
 */
double MenuSelector::getitemextent(MenuItem *mitem, //!< the index, MUST already be in bounds
								double *w, //!< Put the x extent here
								double *h, //!< Put the y extent here
								double *gx, //!< Where the graphic would start, relative to whole item
								double *tx  //!< Where the text would start, relative to whole item
							)
{
	double gw,gh;
	getgraphicextent(mitem,&gw,&gh);
	double ww=getextent(mitem->name,-1,w,h,NULL,NULL);
	if (menustyle&MENUSEL_GRAPHIC_ON_RIGHT) {
		if (tx) *tx=0;
		if (gx) *gx=(ww?ww+padg:0);
	} else {
		if (gx) *gx=0;
		if (tx) *tx=(gw?gw+padg:0);
	}
	if (h && gh>*h) *h=gh;
	if (gw) ww+=padg+gw;
	return ww;
}

//! Draw the status graphic at window coordinates x,y,width=sgw,height=textheight+leading, with state.
/*!  This is not an item's icon. It is the menu style in which the items are not
 *  highlighted, but act like several checkboxes. The boxes are marked instead of 
 *  the whole item being hightlighted.
 *
 *  Of this scheme, MENUSEL_CHECKBOXES causes the graphic to be a circle with the mark
 *  inside the circle. Otherwise, the mark is drawn without a surrounding circle.
 */
void MenuSelector::drawStatusGraphic(int x,int y,int state)
{
	if (menustyle&MENUSEL_CHECKBOXES) { // draw the circle with textbg first
		foreground_color(win_colors->bg);
		fill_arc_wh(this, x,y, sgw,textheight, 0,2*M_PI);
	}
	
	foreground_color(win_colors->fg);
	if (state&LAX_ON) fill_arc_wh(this, x+sgw/4,y+sgw/4, sgw/2,sgw/2, 0,2*M_PI);

	foreground_color(highlight);
	if (menustyle&MENUSEL_CHECKBOXES) 
		draw_arc_wh(this, x,y, sgw,textheight, 200*M_PI/180,(360+20)*M_PI/180);
	if (state&LAX_ON) draw_arc_wh(this, x+sgw/4,y+sgw/4, sgw/2,sgw/2, (360+20)*M_PI/180,200*M_PI/180);

	foreground_color(shadow);
	if (menustyle&MENUSEL_CHECKBOXES) 
		draw_arc_wh(this, x,y, sgw,sgw, (360+20)*M_PI/180,200*M_PI/180);
	if (state&LAX_ON) draw_arc_wh(this, x+sgw/4,y+sgw/4, sgw/2,sgw/2, 200*M_PI/180,(360+20)*M_PI/180);
}


//! Draw a separator (default is just a win_colors->grayedfg colored line) across rect widthwise.
void MenuSelector::drawsep(const char *name,IntRectangle *rect)
{
	foreground_color(win_colors->grayedfg); 
	draw_line(this, rect->x,rect->y+rect->height/2, rect->x+rect->width-1,rect->y+rect->height/2);
	if (!isblank(name)) {
		int extent=getextent(name, -1, NULL, NULL, NULL, NULL, 0);
		 //blank out area
		foreground_color(win_colors->bg); 
		fill_rectangle(this, rect->x+rect->width/2-extent/2-2,rect->y,extent+4,rect->height);
		 //draw name
		foreground_color(win_colors->grayedfg); 
		textout(this, name,-1,rect->x+rect->width/2,rect->y+rect->height/2,LAX_CENTER);
	}
}

//! Draws a submenu indicator in rectangle x,y,width=subw, height=textheight+leading.
/*! \todo for MENUSEL_SUB_FOLDER, draw an open or closed folder.
 */
void MenuSelector::drawsubindicator(MenuItem *mitem,int x,int y)
{
	if (!(mitem->state&LAX_HAS_SUBMENU)) return;
	if (menustyle&MENUSEL_SUB_FOLDER) {
		 //draw a little folder
		int w=subw,h=textheight+leading;
		flatpoint p[8]={
						 flatpoint(x,       y+h*2/3),
						 flatpoint(x+w*3/4, y+h*2/3),
						 flatpoint(x+w*3/4, y+h/3),
						 flatpoint(x+w/3,   y+h/3),
						 flatpoint(x+w/3,   y+h/6),
						 flatpoint(x+w/8,   y+h/6),
						 flatpoint(x,       y+h/3),
						 flatpoint(x,       y+h*2/3)
						};
		foreground_color(win_colors->color1); // only draw highlighted if not checkboxes
		fill_polygon(this, p,8);
		foreground_color(win_colors->fg); // only draw highlighted if not checkboxes
		draw_lines(this, p,8,1);
//***	if (mitem->state&LAX_OPEN) { // draw open folder
//		} else { // draw closed folder
//		}
	} else { // draw the arrows
		if (mitem->state&LAX_OPEN) { // draw arrow down
			drawarrow(x+subw/2,y+(textheight+leading)/2,(subw>(textheight+leading)?(textheight+leading):subw)/3,THING_Triangle_Down);
		} else { // draw arrow right
			drawarrow(x+subw/2,y+(textheight+leading)/2,(subw>(textheight+leading)?(textheight+leading):subw)/3,THING_Triangle_Right);
		}
	}
}

//! Draw a check mark in rectangle x,y,width=checkw, height=textheight+leading.
void MenuSelector::drawcheck(int on, int x,int y)
{
	if (!on) return;

	foreground_color(win_colors->fg);
	draw_thing(this, x+checkw/2,y+(textheight+leading)/2, .7*checkw,-.7*(textheight+leading/2), 1, THING_Check);
}

//! Draw the item icon and name in rect.
void MenuSelector::drawitemname(MenuItem *mitem,IntRectangle *rect)
{
	unsigned long f,g;
	double fasc,tx,gx,iw;
	fasc=app->defaultlaxfont->ascent();

	getitemextent(mitem,&iw,NULL,&gx,&tx); // status graphic and name x coordinate
	if (menustyle&MENUSEL_LEFT || menustyle&MENUSEL_TAB_JUSTIFY) { gx+=rect->x; tx+=rect->x; } // |blah    |
	else if (menustyle&MENUSEL_RIGHT) { gx=rect->x+rect->width-iw+gx; tx=rect->x+rect->width-iw+tx; } // |   blah|
	else { gx=rect->x+(rect->width-iw+gx)/2; tx=rect->x+(rect->width-iw+tx)/2; } // |   blah   |
	
	 // set proper foreground and background colors
	//DBG cerr<<"menu "<<(menu->title?menu->title:"untitled")<<" item "<<c<<": "<<(mitem->state&LAX_HAS_SUBMENU?1:0)<<endl;
	if ((mitem->state&LAX_MSTATE_MASK)>LAX_ON) { // grayed, hidden=0, off=1, on=2, so this is same as>2
		//DBG cerr <<"item "<<(mitem->state&LAX_ON?1:0)<<" "<<mitem->name<<endl;
		g=win_colors->bg;
		f=win_colors->grayedfg;
	} else {
		//DBG cerr <<"item "<<(mitem->state&LAX_ON?1:0)<<" "<<mitem->name<<endl;
		g=win_colors->bg;
		f=win_colors->fg;
	}

	 // do the actual drawing
	if (!(menustyle&(MENUSEL_CHECKBOXES|MENUSEL_CHECKS))) { // draw as menu, check boxes don't hightlight
		if (mitem->state&LAX_ON) { // draw on
			g=win_colors->hbg;
			f=win_colors->hfg;
		} 
		 // add a little extra hightlight if item is ccuritem
		if (!(menustyle&MENUSEL_ZERO_OR_ONE)) if (mitem==item(ccuritem)) g=coloravg(f,g,.85);
		
		foreground_color(g); // only draw highlighted if not checkboxes
		fill_rectangle(this, rect->x,rect->y,rect->width,rect->height);
	}
	foreground_color(f);
	background_color(g);
	textout(this, mitem->name,strlen(mitem->name), tx,rect->y+fasc+leading/2, LAX_LEFT|LAX_BASELINE);
	MenuItem *im=mitem;
	if (im && im->image) {
		image_out(im->image, this, gx, rect->y);
	}

	 // draw little markers for ccuritem
	if (mitem==item(ccuritem) && win_active) {
		foreground_color(win_colors->fg);
		draw_arc_wh(this, rect->x,rect->y+textheight/3, textheight/3,textheight/3, 0,2*M_PI);
		draw_arc_wh(this, rect->x+rect->width-1-textheight/3,rect->y+textheight/3,textheight/3,textheight/3, 0,2*M_PI);
	}
}

//! Draw the item in the provided area..
/*!  All the things that go on an item line:\n
 * [status graphic=whether highlighted][check toggle][graphic icon][text][submenu indicator]
 * First XClearArea, then put it in..
 * 
 * 1. If the item is a separator, call drawsep() and return. 
 * 
 * 2. Otherwise, determine whether there is a status graphic, submenu indicator, and checkmark. 
 *    If so, from the original line, the status graphic is either put flush left or right.
 * 
 * 3. Then a checkmark is placed either flush left or right in the remaining space. 
 * 
 * 4. Then the submenu indicator is put flush left or right of the remaining space.
 * 
 * 5. Finally, the actual item text and icon fill the remaining space.
 * 
 *  Normally, drawitem would use drawStatusGraphic,****is this right
 *  and getitemextent to determine these things.
 *
 *  \todo *** Highlighting should go right across the item, behind any other doodads
 *  put up, currently just goes across text...
 *
 *	\todo *** set clip to intersection of itemspot and inrect??
 */
void MenuSelector::drawitem(MenuItem *mitem,IntRectangle *itemspot)
{
	foreground_color(win_colors->bg);
	if (!ValidDrawable()) return;
	fill_rectangle(this, itemspot->x,itemspot->y,itemspot->width,itemspot->height);

	if (mitem->state&LAX_SEPARATOR) {
		drawsep(mitem->name,itemspot);
		return;
	}
	
	 // Status graphic, which is in lieu of highlighting
	//***if (menustyle&(MENUSEL_CHECKBOXES|MENUSEL_CHECKS)) { // draw as list of checkboxes
	if (menustyle&MENUSEL_STATUS_ON_LEFT) {
		drawStatusGraphic(itemspot->x,itemspot->y,mitem->state);
		itemspot->x+=sgw;
		itemspot->width-=sgw;
	} else if (menustyle&MENUSEL_STATUS_ON_LEFT) {
		drawStatusGraphic(itemspot->x+itemspot->width-sgw,itemspot->y,mitem->state);
		itemspot->width-=sgw;
	}
	
	 // Draw checkmark if necessary, item must have LAX_ISTOGGLE in state.
	if (mitem->state&LAX_ISTOGGLE) {
		if (menustyle&MENUSEL_CHECK_ON_LEFT) {
			drawcheck(mitem->state&LAX_CHECKED,itemspot->x,itemspot->y);
			itemspot->x+=checkw;
			itemspot->width-=checkw;
		} else if (menustyle&MENUSEL_CHECK_ON_RIGHT) {
			drawcheck(mitem->state&LAX_CHECKED,itemspot->x+itemspot->width-checkw,itemspot->y);
			itemspot->width-=checkw;
		}
	}
		
	 // Draw sub menu indicator
	if (menustyle&MENUSEL_SUB_ON_LEFT) {
		drawsubindicator(mitem,itemspot->x,itemspot->y);
		itemspot->x+=subw;
		itemspot->width-=subw;
	} else if (menustyle&MENUSEL_SUB_ON_RIGHT) {
		drawsubindicator(mitem,itemspot->x+itemspot->width-subw,itemspot->y);
		itemspot->width-=subw;
	}

	 // Draw the item icon and text
	drawitemname(mitem,itemspot);
}

//! Draw the item with the index value c.
/*!  All the things that go on an item line:\n
 * [status graphic=whether highlighted][check toggle][graphic icon][text][submenu indicator]
 *
 * First, determine if this item is actually on screen, and find the area to draw it in. Return if not on screen.
 * If it is found, then find the item and call drawitem(theitem,thebounds).
 */
void MenuSelector::drawitem(int c)
{
//DBG cerr <<"draw "<<c<<endl;
	if (c<0 || c>=numItems()) return;

	 // find item and item bounds
	IntRectangle itemspot;
	findRect(c,&itemspot);

	 // If itemspot not onscreen, return
	if (itemspot.x+itemspot.width <=inrect.x || itemspot.x>=inrect.x+inrect.width ||
		itemspot.y+itemspot.height<=inrect.y || itemspot.y>=inrect.y+inrect.height) {
		//DBG cerr <<"Item "<<c<<" not on screen...."<<endl;
		return;
	}
	
	drawitem(item(c),&itemspot);
}

//! Find screen rectangle item c goes in
/*! Return 0 for rectangle found, otherwise 1 for unable to find (for instance when called before
 * necessary initialization done).
 */
int MenuSelector::findRect(int c,IntRectangle *itemspot)
{
	if (columnsize==0) return 1;
	int col=c/columnsize;
	long x,y;
	panner->GetCurPos(1,&x);
	panner->GetCurPos(2,&y);
	
	itemspot->x=inrect.x-(x-inrect.x);
	itemspot->y=inrect.y-(y-inrect.y);
	for (int cc=0; cc<col; cc++) itemspot->x+=columns.e[cc];
	itemspot->y+=(c-col*columnsize)*(textheight+leading);
	itemspot->width=columns.e[col];
	itemspot->height=textheight+leading;

	return 0;
}

//! Draw the arrows for menus, really just THING_Triangle_Up, Down, Left, Right for submenus.
/*! Centers around x,y within a box of side 2*r.
 * This function is also used for the up and down arrows when the menu contents 
 * extend beyond the screen.
 */
void MenuSelector::drawarrow(int x,int y,int r,int type)
{
	foreground_color(win_colors->color1); // inside the arrow
	draw_thing(this, x,y,r,r,1,(DrawThingTypes)type);
	foreground_color(win_colors->fg); // border of the arrow
	draw_thing(this, x,y,r,r,0,(DrawThingTypes)type);
}

//! Draw the menu title if present. Default is print it out at top of window (not inrect).
void MenuSelector::drawtitle()
{
	if (!menu || !menu->title) return;
	foreground_color(win_colors->fg); // background of title
	fill_rectangle(this, 0,pad, win_w,textheight+leading);
	foreground_color(win_colors->fg); // foreground of title
	textout(this,menu->title,-1,win_w/2,pad,LAX_CENTER|LAX_TOP);
}

//! Draw the window.
/*! Draws the items by calling drawitem(). Then draws the up arrow and down arrow
 *  if they are needed. Draws the title along top of window
 *
 * // *** will need a smart refreshing to XCopyArea instead of redrawing the whole thing\n
 * // *** need itemrect????\n
 *
 * <pre>
 * Goes like:
 *  drawtitle()
 *  drawitem(int) for each item
 *    then with that item and rect on screen where it goes, call:
 *    drawitem(MenuItem *mitem,IntRectangle *itemspot)
 *      which might call as appropriate:
 *      drawsep(const char *name,IntRectangle *rect) <-- separator
 *      drawStatusGraphic(int x,int y,int state) <-- in lieu of highlighting
 *      drawcheck(int on, int x,int y)          <-- togglable check mark
 *      drawsubindicator(MenuItem *mitem,int x,int y)  <-- whether there's a submenu
 *      drawitemname(MenuItem *mitem,IntRectangle *rect) <-- item name and/or icon
 * </pre>
 */
void MenuSelector::Refresh()
{
	if (!win_on || !needtodraw) return;
	clear_window(this);

	int c;
	
	 // Draw title if necessary
	if (menustyle&MENUSEL_USE_TITLE) drawtitle();
	
	 // Draw items, *** this could be made more efficient...
	for (c=0; c<numItems(); c++) drawitem(c);
	
	// //*** for debugging, draw box around inrect
	//foreground_color(coloravg(win_colors->bg,win_colors->fg,.5));
	//draw_rectangle(this,inrect.x,inrect.y,inrect.width,inrect.height);
	
	 // draw More arrows if necessary
	// if (panner->min[0]<inrect.x) ***draw left
	// if (panner->min[1]<inrect.y) ***draw up
	// if (panner->max[0]>inrect.x+inrect.width-1) ***draw right
	// if (panner->max[1]<inrect.y+inrect.height-1) ***draw down
//	 // draw up tick *** this is broken for multiple column layouts 
//	if (firstinw>0) drawarrow(win_w/2,y+textheight/2,textheight/4,3);
//	 // draw down tick	 *** this is broken for multiple column layouts 
//	if (firstinw+(columns.n-1)*pagesize<numItems()-1) 
//		drawarrow(win_w/2,(pagesize-1)*(textheight+leading)+textheight/2,textheight/4,4);
		
	needtodraw=0;
}

//! Send message to owner.
/*! If !(menustyle&MENUSEL_SEND_STRINGS) Sends SimpleMessage with:
 * \code
 * info1 = curitem (which might be -1 for nothing)
 * info2 = id of curitem
 * info3 = nitems selected
 * info4 = curitem->info
 * str   = curitem->name
 * \endcode
 *
 * Otherwise, sends a list of the selected strings in a StrsEventData,
 * with info=curitem.
 *
 * \todo there needs to be option to send id or list of ids..
 * \todo maybe send device id of the device that triggered the send
 */
int MenuSelector::send(int deviceid)
{
	DBG cerr <<WindowTitle()<<" send"<<endl;
	if (!win_owner || !win_sendthis) return 0;

	 // find how many selected
	int n=0;
	for (int c=0; c<numItems(); c++) if (item(c)->state&LAX_ON) n++;
	if (n==0 && !(menustyle&MENUSEL_SEND_ON_UP)) return 0;

	if (menustyle&MENUSEL_SEND_STRINGS) {
		StrsEventData *strs=new StrsEventData(NULL,win_sendthis,object_id,win_owner);
		strs->n=n;
		strs->strs=new char*[n+1];
		strs->info=curitem;
		int i=0;
		for (int c=0; c<numItems(); c++) 
			if (item(c)->state&LAX_ON) strs->strs[i++]=newstr(item(c)->name);
		strs->strs[i]=NULL;
		app->SendMessage(strs,win_owner,win_sendthis,object_id);

	} else {
		SimpleMessage *ievent=new SimpleMessage;
		ievent->info1=curitem; 
		ievent->info2=(curitem>=0 && curitem<numItems() ? item(curitem)->id : curitem);
		ievent->info3=n; // n is the number of items selected
		ievent->info4=(curitem>=0 && curitem<numItems() ? item(curitem)->info : 0);
		makestr(ievent->str, (curitem>=0 && curitem<numItems() ? item(curitem)->name : NULL));
		app->SendMessage(ievent,win_owner,win_sendthis,object_id);
	}

	 // reset cache on each send event just in 
	 // case event causes cache to get out of sync
	//curmenuitem=NULL;

	return 0;
}

//------ non-input refreshing and arranging is above ^^^
//------       mostly user input functions are below vvv


//! Character input.
/*! Currently: 
 * <pre>
 *  enter      ???send/select currently selected items
 *  space      addselect(ccuritem)
 *  pgup/down, Jump ccuritem 1 page up or down
 *  up/down,   Move ccuritem 1 item up or down
 *  'a'        If none selected, select all, else deselect any selected.
 *  ^'m'       (***imp me!) move selected items to ccuritem
 *  '/'        (***imp me!) start search mode?-- have either sep search mode
 *                   or make all selecting keys above be control-key, not just key
 *                   so can use normal typing as progressive search
 *  *** need to have type to search, or hotkeys
 * </pre>
 */
int MenuSelector::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
//	switch(ch) {
//		case LAX_Shift: return 0;// shift
//		case LAX_Control: return 0;// cntl
//		case LAX_Esc: return 0;// esc
//		case LAX_Del: // del
//		case LAX_Bksp: // bksp
//		case LAX_Ins: // ins
//		case 'm':
//		case 'v': {
//			*** move selected things to where ccuritem is located.. like drag and drop
//		} return 0;

	if (ch==0) return 0; // null
	if (ch==LAX_Tab) return anXWindow::CharInput(ch,buffer,len,state,d); // tab
	if (ch==LAX_Enter) { // enter
		send(d->id);
		return 0;

	} else if (ch==LAX_Home) { // jump to beginning
		int c=ccuritem;
		ccuritem=0;
		if (ccuritem!=c) {
			drawitem(c);
			c=makeinwindow();
			if (menustyle&MENUSEL_CURSSELECTS) {
				addselect(ccuritem,state);
				if (menustyle&MENUSEL_CURSSENDS) send(d->id);
			} else if (!c) {
				drawitem(ccuritem);
			}
		}
		return 0;

	} else if (ch==LAX_End) { // jump to end
		int c=ccuritem;
		ccuritem=numItems()-1;
		if (ccuritem!=c) {
			drawitem(c);
			c=makeinwindow();
			if (menustyle&MENUSEL_CURSSELECTS) {
				addselect(ccuritem,state);
				if (menustyle&MENUSEL_CURSSENDS) send(d->id);
			} else if (!c) {
				drawitem(ccuritem);
			}
		}
		return 0;

	} else if (ch==LAX_Esc) { //escape
		if ((menustyle&MENUSEL_DESTROY_ON_LEAVE) || (menustyle&MENUSEL_DESTROY_ON_FOCUS_OFF)) {
			app->destroywindow(this);
			return 0;
		}
		return 1;

	} else if (ch==LAX_Pgup) { // pgup
		DBG cerr <<"pgup "<<pagesize<<endl;
		
		movescreen(0,(-pagesize+1)*(textheight+leading));
		return 0;

	} else if (ch==LAX_Pgdown) { // pgdown
		DBG cerr <<"pgdown "<<pagesize<<endl;
		
		movescreen(0,(pagesize-1)*(textheight+leading));
		return 0;

	} else if (ch==LAX_Left) { // left
		 // scroll through columns
		int c=ccuritem/columnsize;
		if (c==0) return 0;
		c=ccuritem;
		ccuritem-=columnsize;
		if (ccuritem<0) ccuritem=0;
		if (ccuritem!=c) {
			drawitem(c);
			c=makeinwindow();
			if (menustyle&MENUSEL_CURSSELECTS) {
				addselect(ccuritem,state);
				if (menustyle&MENUSEL_CURSSENDS) send(d->id);
			} else if (!c) {
				drawitem(ccuritem);
			}
		}
		return 0;

	} else if (ch==LAX_Right)  { // right
		 // scroll through columns
		int c=ccuritem/columnsize;
		if (c==columns.n-1) return 0;
		c=ccuritem;
		ccuritem+=columnsize;
		if (ccuritem<0) ccuritem=0;
		if (ccuritem!=c) {
			drawitem(c);
			c=makeinwindow();
			if (menustyle&MENUSEL_CURSSELECTS) {
				addselect(ccuritem,state);
				if (menustyle&MENUSEL_CURSSENDS) send(d->id);
			} else if (!c) {
				drawitem(ccuritem);
			}
		}
		return 0;

	} else if (ch==LAX_Up) { // up
		int c=ccuritem;
		ccuritem--;
		if (ccuritem<0) ccuritem=0;
		if (ccuritem!=c) {
			drawitem(c);
			c=makeinwindow();
			if (menustyle&MENUSEL_CURSSELECTS) {
				addselect(ccuritem,state);
				if (menustyle&MENUSEL_CURSSENDS) send(d->id);				
			} else if (!c) {
				drawitem(ccuritem);
			}
		}
		return 0;

	} else if (ch==LAX_Down) { // down
		int c=ccuritem;
		ccuritem++;
		if (ccuritem>=numItems()) ccuritem=numItems()-1;
		if (ccuritem!=c) {
			//DBG cerr <<"ccuritem: "<<ccuritem<<"  c="<<c<<endl;
			drawitem(c);
			c=makeinwindow();
			if (menustyle&MENUSEL_CURSSELECTS) {
				addselect(ccuritem,state);
				if (menustyle&MENUSEL_CURSSENDS) send(d->id);
			} else if (!c) {
				drawitem(ccuritem);
			}
		}
		return 0;

	} else if (ch==' ') {  // same as an LBDown on ccuritem
		addselect(ccuritem,state);
		if (menustyle&MENUSEL_SEND_ON_UP) send(d->id);
		return 0;

	} else if (ch=='a') {
		int n=0,c,c2;
		for (c=0; c<numItems(); c++) if (item(c)->state&LAX_ON) break;
		if (c!=numItems()) n=LAX_OFF; else n=LAX_ON;
		for (int c=0; c<numItems(); c++) {
			if (c2=(item(c)->state&(LAX_OFF|LAX_ON)), c2>0 && c2!=n) {
				item(c)->state&=~LAX_MSTATE_MASK; 
				item(c)->state|=n;
			}
		}
		if (menustyle&(MENUSEL_ONE_ONLY|MENUSEL_ZERO_OR_ONE)) addselect(ccuritem,0);
		needtodraw=1;
		return 0;
	}
	return 1;
}

//! Programs call this to select index which.
/*! This has the same effect as left button down on it.
 *
 * \todo *** add state to it, for more control... that is be able to modify state of
 * any element...
 *
 * Returns index of the new curitem.
 */
int MenuSelector::Select(int which)
{
	if (which<0 || which>=numItems()) return curitem;
	if (item(which)->state&LAX_ON) item(which)->state^=(LAX_ON|LAX_OFF);
	addselect(which,0);
	return curitem;
}

//**** make me!!
//void makestate(MenuItem *m,unsigned int nstate)
//{
//	m->state=m->state&~LAX_MSTATE_MASK|nstate;
//}

//***void MenuSelector::selectrange(int end, int start) // start==-1 => start=curitem
//***void MenuSelector::unselect(int end, int start) // start==-1 => start=curitem

//! This is called on a mouse down or a space press.
/*! Sets the state of the affected items, and also draws the
 * results on the window. A plain select will deselect all,
 * and select just i. A control
 * select will toggle whether an item is on or off, without
 * deselecting all the others. A shift select will turn on any items
 * in the range that are off. A control-shift select will make all in the
 * range [curitem,i] have the same state as curitem. 
 *
 * An item's state is only modified if it is already LAX_ON or LAX_OFF.
 *
 * Does not send the control message from here.
 */
void MenuSelector::addselect(int i,unsigned int state)
{
	DBG cerr <<(WindowTitle())<<" addselect:"<<i<<" state="<<state<<endl;
	int c;
	MenuItem *mitem=item(i),*titem;
	if (!mitem) return;
	
	if (!(state&ShiftMask) || menustyle&(MENUSEL_ZERO_OR_ONE|MENUSEL_ONE_ONLY)) { // select individual
		int oldstate=mitem->state;
		if (!(state&ControlMask)) { // unselect others
			int n=0;
			for (c=0; c<numItems(); c++) { // turn off all the ones that are on
				titem=item(c);
				if (titem->state&LAX_ON) { 
					titem->state&=~LAX_MSTATE_MASK; 
					titem->state|=LAX_OFF; 
					drawitem(c);
					n++; 
				}
			}
			if (n>1) oldstate=LAX_OFF;
		}
		if (menustyle&MENUSEL_ONE_ONLY || (oldstate&LAX_MSTATE_MASK)==LAX_OFF) 
			mitem->state=(mitem->state&~LAX_MSTATE_MASK)|LAX_ON;
		else if ((oldstate&LAX_MSTATE_MASK)==LAX_ON) mitem->state=(mitem->state&~LAX_MSTATE_MASK)|LAX_OFF;
		c=ccuritem;
		ccuritem=curitem=i;
		curmenuitem=mitem;
		drawitem(c);       // draw off old ccuritem
		drawitem(curitem); // draw on curitem==ccuritem

	} else if (state&ShiftMask) { // select range
		int start,end;
		unsigned int nstate=curmenuitem->state&LAX_MSTATE_MASK;
		if (!(nstate&(LAX_ON|LAX_OFF))) nstate=LAX_ON;
		if (i<curitem) { start=i; end=curitem; } else { start=curitem; end=i; }
		for (c=start; c<=end; c++) {
			 //make each item's state the same as the old curitem state
			 //or toggle the state if control is also pressed. toggle is only 
			 //between ON and OFF, otherwise curitem->state is used.
			 //***if curitem was gray?
			 //If the item to change is LAX_GRAY, then do not change it
			titem=item(c);
			if ((menustyle&MENUSEL_SELECT_LEAF_ONLY) && (titem->state&MENU_HAS_SUBMENU)) continue;
			if ((menustyle&MENUSEL_SELECT_SUB_ONLY) && !(titem->state&MENU_HAS_SUBMENU)) continue;
			if ((titem->state&LAX_MSTATE_MASK)==LAX_GRAY) continue;
			if (state&ControlMask) { 
				if ((titem->state&(LAX_OFF|LAX_ON))!=0) {
					if ((titem->state&LAX_MSTATE_MASK)!=nstate) {
						titem->state=(titem->state&~LAX_MSTATE_MASK)|nstate; 
						drawitem(c);
					}
				}
			} else if ((titem->state&LAX_MSTATE_MASK)==LAX_OFF) { 
				titem->state=(titem->state&~LAX_MSTATE_MASK)|LAX_ON; 
				drawitem(c); 
			}
		}
		c=ccuritem;
		ccuritem=curitem=i;
		curmenuitem=mitem;
		drawitem(c);        // draw off old ccuritem
		drawitem(curitem);  // draw on curitem==ccuritem
	}
}

//! Sets curitem and curmenuitem. Internal function.
/*! *** doesn't seem to be used anywhere.. get rid of it? */
void MenuSelector::setcuritem(int i)
{
	curitem=i;
	curmenuitem=item(i,0); // the 0 means skip any caching stuff
}

//! Left button down.
/*! If MENUSEL_OUT_CLICK_DESTROYS is set, then app->destroywindow(this) when clicking
 * down outside the window.
 */
int MenuSelector::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (x<0 || x>=win_w || y<0 || y>=win_h) {
		if (menustyle&MENUSEL_OUT_CLICK_DESTROYS) {
			curitem=-1;
			curmenuitem=NULL;
			send(d->id);
			app->destroywindow(this);
			return 0;
		}
	}
	buttondown.down(d->id,LEFTBUTTON,x,y);
	mousedragged=0;
	return 0;
}

//! Left button up.
/*! If clicked on an arrow, then shift screen. If on item, then addselect(that item).
 * Send the control message if MENUSEL_SEND_ON_UP is set.
 * Destroy this window if MENUSEL_CLICK_UP_DESTROYS is set.
 * 
 * \todo *** lbdown/up no drag on an already selected item should initiate EDIT_IN_PLACE
 *
 * \todo *** mousedragged==2 means a rearrangement might be required
 */
int MenuSelector::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);
	
	if (mousedragged==2) { //*** ==2 means was attempting to rearrange..
		//***turn off any dragging mode
		mousedragged=0;
		return 0;
	}
	
	 // do nothing if mouse is outside window.. This is necessary because of grabs.
	if (x<0 || x>=win_w || y<0 || y>=win_h) {
		return 0;
	}
			
	int i=findItem(x,y);
	//if (i==firstinw && firstinw) { movescreen(0,-(textheight+leading)); return 0; } //if on up arrow
	//if (i==firstinw+pagesize-1 && firstinw+pagesize!=numItems()) 
	//	{ movescreen(0,(textheight+leading)); return 0; } //if on down arrow
	if (i<0 || i>=numItems()) return 0; //if not on any item or arrow
	
	 // clicked on already selected item
	if (dragged<20 && menustyle&MENUSEL_EDIT_IN_PLACE) {
		editInPlace();
		return 0; 
	}
	
	if (dragged<20) {
		if (!(menustyle&MENUSEL_CLICK_UP_DESTROYS)) addselect(i,state);
		if (menustyle&MENUSEL_SEND_ON_UP) send(d->id);
		if (menustyle&MENUSEL_CLICK_UP_DESTROYS) app->destroywindow(this);
	}
	
	return 0;
}

//! Find the would be item at window coordinates (x,y).
/*! The value returned is not necessarily an actual element index. It could be -1, or >=menuitems.n.
 */
int MenuSelector::findItem(int x,int y)
{
	long xx,yy;
	panner->GetCurPos(1,&xx);
	panner->GetCurPos(2,&yy);
	x+=(xx-inrect.x);
	y+=(yy-inrect.y);
	int c=0;
	while (c<columns.n && x>columns.e[c]) x-=columns.e[c++];
	if (c==columns.n) c--;
	return c*columnsize + y/(textheight+leading);
}

//! Set up the edit in place mode. ****TODO!!
/*! \todo *** imp me!
 */
void MenuSelector::editInPlace()
{//***
	DBG cerr <<" *** editInPlace not implemented!"<<endl;
}

//! Right button and drag drags the screen around (with potential autoscrolling)
/*! 
 * \todo ***perhaps someday have a hook so that right click on an item calls up some
 * menu? like copy to, delete item, etc..
 */
int MenuSelector::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id,RIGHTBUTTON, x,y);
	mx=x; my=y;
	//if (buttondown==RIGHTBUTTON) timerid=app->addmousetimer(this);
	
	return 0;
}

//! Nothing but remove tag from buttondown.
int MenuSelector::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,RIGHTBUTTON)) return 1;
	buttondown.up(d->id,RIGHTBUTTON);
	//if (!buttondown) app->removetimer(timerid);
	return 0;
}

//! Left might select depending on style. Right button drags. +R fast drags.
/*! \todo *** should implement speedy shifting with shift/cntl-RB drag
 *
 * \todo *** also, if right-click shifting, and is off screen, do a sort of
 * auto scrolling, like if mouse above, then move if mouse moving up,
 * but not if mouse is moving down?? or on timer?
 */
int MenuSelector::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	//DBG cerr <<"mx,my:"<<mx<<','<<my<<" x,y:"<<x<<','<<y<<endl;
	int i=findItem(x,y);
	if (!buttondown.any() && menustyle&MENUSEL_FOLLOW_MOUSE) {
		 // do nothing if mouse is outside window..
		 // This is a necessary check because of grabs.
		if (x<0 || x>=win_w || y<0 || y>=win_h) return 0;
			
		if (i<0) i=0;
		if (i>=numItems()) i=numItems()-1;
		if (i!=ccuritem) {
			//*** must imp. move screen only if mouse move is right direction
			//if (firstinw>0 && i==firstinw) movescreen(0,-(textheight+leading));
			//else if (i==firstinw+pagesize-1 && i!=numItems()-1) movescreen(0,(textheight+leading));

			if (menustyle&MENUSEL_CURSSELECTS) {
				addselect(i,state);
				if (menustyle&MENUSEL_CURSSENDS) send(d->id);
			} else {
				int tm=ccuritem;
				ccuritem=i;
				drawitem(tm); //drawitem needs the current ccuritem, not old
				drawitem(ccuritem);
			}
		}
		return 0;
	}
	if (buttondown.isdown(d->id,LEFTBUTTON)) {
		if (mousedragged==0) { 
			if (menustyle&MENUSEL_REARRANGEABLE) {
				//***if any items selected, then drag them...
				mousedragged=2;
			} else mousedragged=1;
		} else if (mousedragged==2) {
			//*** dragging some items is already inprogress...
		}
		return 0;
	}
	if (!buttondown.isdown(d->id,RIGHTBUTTON)) return 1;
	int m=1;
	if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) m=20;
	else if ((state&LAX_STATE_MASK)==ControlMask || (state&LAX_STATE_MASK)==ShiftMask) m=10;
	
	if (y-my>0) { 
		movescreen(0,-m*(y-my));
		if (y>0 && y<win_h) { mx=x; my=y; }
	} else if (my-y>0) {
		movescreen(0,m*(my-y));
		if (y>0 && y<win_h) { mx=x; my=y; }
	}
	return 0;
}

//! Autoscroll if necessary****TODO
/*! \todo *** must autoscroll when mouse over arrow and FOLLOW_MOUSE
 */
int MenuSelector::Idle(int tid)
{//***
//	if (tid!=timerid) return 1;
//	
//	 // autoscroll
//	if (buttondown==RIGHTBUTTON) {
//		if (my<0) movescreen(0,my));
//		else if (my>win_h) movescreen(0,my-win_h);
//	}
	return 0;
}

//! Make sure that ccuritem is visible by shifting screen so it is.
/*! Returns 1 if x shifted, 2 if y, 3 if both.
 */
int MenuSelector::makeinwindow()
{
	IntRectangle itemrect;
	findRect(ccuritem,&itemrect);
	int dx,dy;
	dx=dy=0;
	if (itemrect.x<inrect.x) dx=inrect.x-itemrect.x;
	else if (itemrect.x+itemrect.width>inrect.x+inrect.width)
		dx=(inrect.x+inrect.width)-(itemrect.x+itemrect.width);
	if (itemrect.y<inrect.y) dy=itemrect.y-inrect.y;
	else if (itemrect.y+itemrect.height>inrect.y+inrect.height)
		dy=(itemrect.y+itemrect.height)-(inrect.y+inrect.height);
	needtodraw=1;
	return panner->Shift(1,dx)|panner->Shift(2,dy);
}

//! Try to move the screen by dx pixels and dy pixels. 
/*! Also resets ccuritem so that it refers to an item that is
 * actually visible.
 *
 * Returns whether shifting occured. 1=x, 2=y, 3=x and y
 */
int MenuSelector::movescreen(int dx,int dy)
{
	DBG cerr <<" ccuritem before: "<<ccuritem<<endl;
	long c=panner->Shift(1,dx);
	c|=panner->Shift(2,dy);
	if (!c) return 0; // no shift occured
	
	 // make ccuritem on screen somewhere
	 // by modifying ccuritem (not the same as makeinwindow!)
	IntRectangle itemrect;
	findRect(ccuritem,&itemrect);
	int col=ccuritem/columnsize,
		row=ccuritem%columnsize,
		w;
	
	 // find a good column
	if (itemrect.x<inrect.x) { 
		w=col=0;
		while (col<columns.n && w<inrect.x) { w+=columns.e[col++]; }
		if (col==columns.n) col--;
	} else if (itemrect.x+itemrect.width>inrect.x+itemrect.width) {
		w=itemrect.x;
		while (col>=0 && w>inrect.x+inrect.width) { col--; w-=columns.e[col]; }
		if (col==columns.n) col--;
	}
	
	 // find a good row
	if (itemrect.y<inrect.y) { 
		row+=(inrect.y-itemrect.y)/(textheight+leading)+1;
		if (row>=columnsize) row=columnsize-1;
	} else if (itemrect.y+itemrect.height>inrect.y+inrect.height) {
		row-=(inrect.y+inrect.height-itemrect.y)/(textheight+leading)+1;
		if (row<0) row=0;
	}
	
	ccuritem=col*columnsize+row;
	if (ccuritem>=numItems()) ccuritem=numItems();
	needtodraw=1;
	DBG cerr <<" ccuritem after: "<<ccuritem<<endl;
	return c;
}

//! Scroll screen down.
/*! Control OR Shift mask shifts by whole page lengths.
 * Shift AND Control shifts by 3 times whole page lengths.
 */
int MenuSelector::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&ControlMask && state&ShiftMask) movescreen(0,-3*pagesize*(textheight+leading));
	else if (state&ControlMask || state&ShiftMask) movescreen(0,-pagesize*(textheight+leading)); 
	else movescreen(0,-(textheight+leading));
	return 0;
}

//! Scroll screen up.
/*! Control OR shift mask shifts by whole page lengths.
 * Shift AND Control shifts by 3 times whole page lengths.
 */
int MenuSelector::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&ControlMask && state&ShiftMask) movescreen(0,3*pagesize*(textheight+leading));
	else if (state&ControlMask || state&ShiftMask) movescreen(0,pagesize*(textheight+leading)); 
	else movescreen(0,(textheight+leading));
	return 0;
}

//! Resize the window to the extent of the items, and reposition near the mouse.
int MenuSelector::WrapToMouse(int mouseid, anXWindow *onedgeofthis) //onedgeofthis=0
{
	int x,y,screen=0;
	mouseposition(mouseid, NULL, &x, &y, NULL, NULL, &screen);
	return WrapToPosition(x,y,screen,onedgeofthis);
}

//! Orient the window's size and position to be near the given screen coordinates.
/*! This function tries to place the window 
 *  near screen coordinates of the mouse.
 *
 *  If onedgeofthis is None, it prefers to put the window to the left of the mouse. 
 *  If it does not fit then it tries to put the window to the right of x,y. Also, 
 *  the currently selected item is made to be at the same
 *  height of y.
 *
 *  If onedgeofthis is not NULL, then it also tries to place 
 *  itself on a nearby right edge of onedgeofthis.
 *
 *  Currently, it assumes that this window is a top level window, such as a popup menu,
 *  and that it is not a tree, that is it does not consider submenus.
 * 
 *  This also calls anXWindow::MoveResize().
 *
 *  \todo *** BROKEN! fix me!
 */
int MenuSelector::WrapToPosition(int screen_x, int screen_y, int screen, anXWindow *onedgeofthis) //onedgeofthis=0
{
	 //---------- Setup the Popup MenuSelector -----------------------
	 // Find the extent of the items, and lay it out with the current item
	 // as close to and to the right of the mouse as possible
	 
	 // get screen geometry
	int px,py,     // what will be the window x,y
		ew=0,eh=0; // extent of wrapped window
	int rw,rh;
	int x=screen_x, y=screen_y;
	app->ScreenInfo(screen, NULL,NULL, &rw, &rh, NULL,NULL,NULL, NULL);

	 // -----find extent: ew,eh *** this only finds the text extent
	if (textheight==0) textheight=app->defaultlaxfont->textheight();
	if (leading==0) {
		if (menustyle&MENUSEL_CHECKBOXES) leading=pad;
		else leading=1;
		MenuItem *im;
		int h=textheight;
		for (int c=0; c<menu->menuitems.n; c++) {
			im=menu->menuitems.e[c];
			if (!im || !im->image) continue;
			if (im->image->h()>h) h=im->image->h();
			if (h>textheight+leading) leading=h-textheight;
		}
	}
	win_w=rw;
	win_h=rh;
	findoutrect();
	inrect=outrect;
	adjustinrect();
	SetLineHeight(textheight+leading,leading,0);
	arrangeItems(1);

	for (int c=0; c<columns.n; c++) ew+=columns.e[c];
	
	eh=columnsize*(textheight+leading)+2*pad;
	ew+=2*pad+4;
	
	y-=textheight/2;
	
	 // ------find placement of popup px,py based on ew,eh, x,y
	int arrowwidth=15; // the amount aside from the pointer to shift to
	 // first set horizontal:
	 // Try to put popup to left of x,y, which are now in screen coords
	 // set x:
	if (x-arrowwidth-ew>0) { // popup fits to the left
		px=x-arrowwidth-ew;
	} else { //put popup at right
		px=x+arrowwidth;
		if (px+ew>(int)rw) px=rw-ew;
	}
	 // then set vertical: hopefully with item centered at y
	 // also find out offset necessary to put the currently selected item near the mouse
	int ypref;
	ypref=(curitem%columnsize)*(textheight+leading)+pad;
	if (eh>(int)rh) eh=rh;
	if (y-ypref+eh>(int)rh-2) { // window goes offscreen below when centered on curitem
		py=rh-eh-2;
		//extrapad=y-py-ypref;
	} else if (y-ypref<0) { // window goes offscreen top when centered on curitem
		py=0;
		//extrapad=y-ypref-py;
	} else { // popup fits vertically just fine
		py=y-ypref;
	}
	
	 // Move window horizontally to be near right edge of window if possible
	if (onedgeofthis) {
		int wx=onedgeofthis->win_x;
		//int wy=onedgeofthis->win_y;
		int bd=onedgeofthis->win_border;
		unsigned int ww=onedgeofthis->win_w;
		//unsigned int wh=onedgeofthis->win_h;

		px=wx+ww+bd;
		if (px+ew>(int)rw) {
			px=wx-bd-ew;
			if (px<0) px=0;
		}
	}
	
	MoveResize(px,py,ew,eh);
	
	SetFirst(curitem,x,y);

	needtodraw=1;
	return 0;
}

//! Called same for ScrolledWindow, then arrangeItems.
/*! Has the effect of resizing scrollers, and remapping inrect and outrect.
 */
void MenuSelector::syncWindows(int useinrect)//useinrect=0
{
	ScrolledWindow::syncWindows(useinrect);
	arrangeItems();
	needtodraw=1;
}

//! Set the new line height and leading.
/*! textheight is set to ntotalheight-newleading.
 * If forcearrange==1 then also syncWindows() which also causes adjustinrect() 
 * and arrangeItems() to be called.
 * If forcearrange==2, then do not
 * call syncWindows, instead call arrangeItems(0).
 * If forcearrange==3, then call arrangeItems(1). This last one is when you are
 * seeking setup in preparation for wrapping a window's boundaries to the items
 * extent.
 *
 * Usually from outside the class,
 * a program would just call SetLineHeight(h,l), and this would call arrangeItems().
 * init() needs to set some other stuff before arranging, so it calls with forcearrange==0.
 */
void MenuSelector::SetLineHeight(int ntotalheight,int newleading, char forcearrange)
{
	if (ntotalheight<=0 || newleading<0) return;
	textheight=ntotalheight-newleading;
	leading=newleading;
	padg=textheight/2;
	sgw=textheight;
	subw=textheight;
	checkw=textheight*2/3;
	if (forcearrange==1) syncWindows(); //syncwindows calls arrangeItems...
	else if (forcearrange==2) arrangeItems();
	else if (forcearrange==3) arrangeItems(1);
	needtodraw=1;
	//*** flush any selected items?
}
	
//! Set outrect to be the window minus space for title.
void MenuSelector::findoutrect()
{
	outrect.x=outrect.y=0;
	if (menustyle&MENUSEL_USE_TITLE && menu->title) outrect.y=textheight+pad;
	outrect.width= win_w-outrect.x;
	outrect.height=win_h-outrect.y;
}

//! Remove the pads from inrect.
/*! This is called last thing from ScrolledWindow::syncWindows().
 *  Also sets pagesize=(inrect.height+leading)/(textheight+leading)
 *
 *  This and arrangeItems are the only places that pagesize gets set.
 */
void MenuSelector::adjustinrect()
{
	inrect.x+=pad; 
	inrect.width-=2*pad;
	inrect.y+=pad;
	inrect.height-=2*pad;

	if (textheight+leading) pagesize=(inrect.height+leading)/(textheight+leading); 
}

//! This is meant to be called when the window is going, but you just added or removed a bunch of stuff.
void MenuSelector::Sync()
{
	arrangeItems();
	needtodraw=1;
}


//! Calls ScrolledWindow::MoveResize(nx,ny,nw,nh).
int MenuSelector::MoveResize(int nx,int ny,int nw,int nh)
{
	ScrolledWindow::MoveResize(nx,ny,nw,nh);
	return 0;
}

//! Calls ScrolledWindow::Resize(nw,nh).
int MenuSelector::Resize(int nw,int nh)
{
	ScrolledWindow::Resize(nw,nh);
	return 0;
}

} // namespace Laxkit

