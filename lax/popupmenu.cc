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


#include <lax/popupmenu.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//------------------------------------------------

/*! \class PopupMenu
 * \ingroup menuthings
 * \brief Class for, of all things, popup menus.
 *
 * \code #include <lax/popupmenu.h> \endcode
 *
 *  This is specifically for the popup part of popup menus. This is separate from
 *  the (usually horizontal) MenuBar from which you pull down popups, as is so common
 *  in applications.
 *
 *  Almost all the real work is done in MenuSelector. This class adds features to
 *  keep track of which menu the mouse enters, and modify which submenus are actually
 *  popped up up accordingly. This is done basically by keeping a linked lists of popped up
 *  menus, and checks where the mouse is with each mouse movement. The pointer is grabbed
 *  by each menu it enters, so this is relatively easy.
 *
 *  It is worth noting that each PopupMenu is created from scratch each time it is needed,
 *  and deleted when not needed. This menu system does not use persistent windows that are 
 *  just mapped on and off. That might be a useful feature for complex menus, but for
 *  now, that is just a remote TODO.
 * 
 *  \todo ***** mondo needs work!!!!!!!!!! *** problem when menu is popped up from a running dialog,
 *    crossing over between the popped menus makes all events be blocked!!!
 *  \todo add support for tearoff menus
 *  \todo POPMENU_LEAVE_DESTROYS should abandon the windows if the mouse leaves a menu, and does not enter another one.
 *
 *  <pre>
 *  ***TODO:
 *  // *** clicking on the item leading to an open submenu should simply close the submenu, not select said item
 *  // **** must process leavenotify/enternotify to make sure all ok
 *  // *** focus must revert back to the calling window!!
 *  // ***** mondo needs work!!!!!!!!!!
 *  // *** put left/right to go to sub/parent menu
 *  // 
 *  //	if you intend to modify menu states, each id must be unique (see Modify(...))
 *  // ids should be non-negative
 *  // ---
 *  //Read in from file or **char:???
 *  // MenuTitle:
 *  // name idnumber g[rayed]
 *  // **** need to iron out having window checked as well as highlighted!!!!
 *  //  "This is the menu title":
 *  //	blah1 1
 *  //	blah2 2 g
 *  //	blah3d 3
 *  //		a 4
 *  //		b 5
 *  //		c 6
 *  //		d 7
 *  //			1 8
 *  //			2 9
 *  //			3 10
 *  //	blah4 11
 *  //	----
 *  //  "blah5 blah" 12
 *  //  
 *  //
 *  // menu.Modify(int id,const char *newitemname,int nid,*newsubmenu)
 *  //
 *  </pre>
 */
/*! \var PopupMenu *PopupMenu::parentmenu
 * \brief The popup that is the parent of this.
 */
/*! \var PopupMenu *PopupMenu::submenu
 * \brief The popup that is the (open) child of this.
 */

	
//! Constructor. Sets win_style to have ANXWIN_BARE|ANXWIN_HOVER_FOCUS.
/*! This creates a MenuSelector that has no parent, and is set with\n
 *   menustyle= MENUSEL_ZERO_OR_ONE | MENUSEL_CURSSELECTS | MENUSEL_FOLLOW_MOUSE | MENUSEL_SEND_ON_UP |
 *   MENUSEL_GRAB_ON_ENTER | MENUSEL_OUT_CLICK_DESTROYS | MENUSEL_CLICK_UP_DESTROYS. 
 *
 *   If nparentmenu is NULL, then MENUSEL_GRAB_ON_MAP is also set.
 *
 *   Ignores parnt and nstyle, and uses NULL and (ANXWIN_BARE|ANXWIN_HOVER_FOCUS) instead.
 *
 *	 This always tries to popup near the mouse, next to nparentmenu.
 */
PopupMenu::PopupMenu(const char *nname, const char *ntitle, unsigned long long style,
				int xx,int yy,int ww,int hh,int brder,
				unsigned long  nowner,const char *mes,
				int mouseid,
				MenuInfo *usethismenu,  //!< Pass in a MenuInfo class, if not NULL is assumed to not be local
				char absorb_count,      //!< Whether to absorb the count of usethismenu 
				PopupMenu *nparentmenu, //!< This is the calling menu, if any
				unsigned long long extrastyle //!< Extra MenuSelector styles to pass along
			) //nowner=0,atom=0,  nminfo=NULL,nmemislocal=1
					: MenuSelector(NULL,nname,(ntitle?ntitle:"Menu"),
							ANXWIN_BARE|ANXWIN_HOVER_FOCUS|ANXWIN_OUT_CLICK_DESTROYS,
							xx,yy,ww,hh,brder,
							nparentmenu,nowner,mes,
							MENUSEL_ZERO_OR_ONE|MENUSEL_CURSSELECTS|MENUSEL_FOLLOW_MOUSE|MENUSEL_SEND_ON_UP
								|MENUSEL_GRAB_ON_ENTER|MENUSEL_OUT_CLICK_DESTROYS
								|MENUSEL_CLICK_UP_DESTROYS|MENUSEL_SUB_ON_RIGHT,
							usethismenu,absorb_count)
{
	menustyle|=(extrastyle &
					(MENUSEL_LEFT|MENUSEL_RIGHT|MENUSEL_CENTER|MENUSEL_CHECK_ON_LEFT|MENUSEL_CHECK_ON_RIGHT
					 |MENUSEL_GRAPHIC_ON_RIGHT|MENUSEL_GRAPHIC_ON_LEFT|MENUSEL_SEND_STRINGS|MENUSEL_DESTROY_ON_LEAVE)
			   );
	if (menustyle&MENUSEL_OUT_CLICK_DESTROYS) win_style|=ANXWIN_OUT_CLICK_DESTROYS;
	parentmenu=nparentmenu;
	submenu=NULL;
	if (!nparentmenu) menustyle|=MENUSEL_GRAB_ON_MAP;
	if (menu) Select(0); //*** if 0 has a menu then popit up!! also should wrap near parentmenu->selected item!!
	if (mouseid) WrapToMouse(mouseid,nparentmenu);
	else WrapToPosition(xx,yy,0,nparentmenu);

	pad=app->defaultlaxfont->textheight()/3;
}

//! Destructor.
/*! Calls app->destroywindow(submenu) if submenu is not NULL,
 * and also app->destroywindow(parentmenu) if parentmenu is not NULL.
 * Care must be taken in addselect and any other place that parts of the popup
 * tree are destroywindow'd!
 */
PopupMenu::~PopupMenu()
{
	if (submenu) {
		submenu->parentmenu=NULL;
		app->destroywindow(submenu);
	}
	if (parentmenu) { // if done with it, should have been set to NULL elsewhere
		parentmenu->submenu=NULL;
		app->destroywindow(parentmenu);
	}

}

////! Focus off maybe destroys the menus.
///*! Also, if MENUSEL_FOCUS_OFF_DESTROYS, then an off focus destroys this window.
// */
//int PopupMenu::FocusOff(const FocusChangeData *e)
//{
//  app->addtimer(***.5 second) ... if after that time, the mouse is not in a popup menu, then destroy the menu setup
//---------------
//	DBG cerr <<"PopupMenu "<<WindowTitle()<<" FocusOff..."<<endl;
//	anXWindow::FocusOff(e);
//	if (!win_active) {
//		if (menustyle&MENUSEL_DESTROY_ON_FOCUS_OFF) {
//			app->destroywindow(this);
//			return 0;
//		}
//		drawitem(ccuritem);
//	}
//	return 0;
//}

//! Must capture before and after selecting, to control what popups should be up.
void PopupMenu::addselect(int i,unsigned int state)
{
	int oldcuritem=curitem;
	MenuSelector::addselect(i,state);
	if (curitem!=oldcuritem) { // must potentially tinker with the popped up.
		 // must remove the old popped up submenu(s)
		if (oldcuritem>=0 && menu->menuitems.e[oldcuritem]->state&LAX_HAS_SUBMENU && submenu) { 
			submenu->parentmenu=NULL; // Important: detach the submenu(s) before destroying.
			app->destroywindow(submenu);
			submenu=NULL;
		}
		 // must establish a new popped up submenu
		 //*** this needs work, maybe need a MenuItem::Open/CloseSubmenu()??
		 //*** when wrapping to mouse is always Selecting 0, and so makes 
		 //almost the whole menu down below the screen...
		if (curitem>=0 && menu->menuitems.e[curitem]->state&LAX_HAS_SUBMENU) { 
			 //***using Atoms to retrieve the message name of the menu to pass to the submenu
			MenuInfo *minfo=menu->menuitems.e[curitem]->GetSubmenu(1);
			char *blah=numtostr((int)getUniqueNumber());
			IntRectangle rect;
			findRect(curitem,&rect);
			anXWindow *win=this;
			while (win) {
				rect.x+=win->win_x;
				rect.y+=win->win_y;
				win=win->win_parent;
			}
			submenu=new PopupMenu(blah, blah, menustyle,
									rect.x+rect.width/2,rect.y+rect.height/2,0,0,win_border,win_owner,win_sendthis,
									0,
									minfo,0,this);
			delete[] blah;
			app->rundialog(submenu,this);

			//DBG if (submenu->window) app->setfocus(submenu);
			//DBG else cerr <<"============Set focus fail!"<<endl;
		}
	}
}

//! Right button currently just escapes out of the window.
/*! TODO: might want to think about some mechanism to have it
 * easy to popup another popup window to modify something
 * about the item that the mouse is over, something akin to
 * "create a link to this on the desktop".
 */
int PopupMenu::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{ 
	return MenuSelector::RBDown(x,y,state,count,d);
}

//! RBUp escapes out of the window entirely.
int PopupMenu::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	int c=MenuSelector::RBUp(x,y,state,d);
	app->destroywindow(this);
	return c;
}

//! Return whether the given coordinate is in the window or not.
char PopupMenu::MouseIn(int x,int y) 
{ 
	return x>=0 && x<win_w && y>=0 && y<win_h; 
}

//! Implements PopupMenu specific mouse move traits.
/*! Since PopupMenus grab the mouse, it is easy to keep track of where the mouse is.
 * It is especially easy since the menu keeps track of the parent and sub menus that
 * are open. Basically this function just shifts the pointer grab to whichever
 * popped up menu it is in.
 */
int PopupMenu::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	//DBG cerr <<"PopupMenu "<<WindowTitle()<<"mx,my:"<<mx<<','<<my<<" x,y:"<<x<<','<<y<<endl;

	 // do default if the mouse is within the window
	if (MouseIn(x,y)) {
		DBG cerr <<"-----Mouse in orig window, do default"<<endl;
		return MenuSelector::MouseMove(x,y,state,d);
	}

	 // else must check to see if it is in another one of the chain
	 // Not necessary to destroy or add windows here, that is handled when addselect is performed.
	 // scan submenus starting from the most submenu:
	PopupMenu *check=submenu;
	if (check) while (check->submenu) check=check->submenu;
	else check=this;

	if (check) {
		while (check && !check->MouseIn(x+win_x-check->win_x,y+win_y-check->win_y)) check=check->parentmenu;
	}
	if (check) { // is in one of the related windows, just shift the pointer grab
		//XUngrabPointer(....
		DBG cerr <<"---==< Grabbing: "<<check->WindowTitle()<<" ...";
		//app->rundialog(check);
		app->setfocus(check,0,d->paired_keyboard);
		const_cast<LaxMouse*>(d)->grabDevice(check);

	} else {
		DBG cerr <<"------< Mouse is not in a popup menu."<<endl;

		 //make this window idle after half a second to check that the mouse is still in a related menu
		app->addtimer(this, 500, 500, 600);
											
	}
	return 0;
}

/*! If there are no mice in any of the related menus, then destroy all the related menus.
 */
int PopupMenu::Idle(int tid)
{
	if (tid==0) return 1;
	app->removetimer(this,tid);

	LaxMouse *mouse;
	anXWindow *win;
	int num=0;
	for (int c=0; c<app->devicemanager->NumDevices(); c++) {
		mouse=dynamic_cast<LaxMouse*>(app->devicemanager->Device(c));
		if (!mouse) continue;
		int x,y;
		unsigned int mask;
		if (mouseposition(mouse->id, NULL, &x,&y, &mask, &win)!=0) {
			if (!win) continue;

			PopupMenu *check=submenu;
			if (check) while (check->submenu) check=check->submenu;
			else check=this;

			if (check) {
				while (check && check!=win) check=check->parentmenu;
			}
			if (check) { num++; continue; }
		} else {
			// Pointer is probably not on same screen as trackwindow
		}
	}

	 //no mice in any of the windows, so get rid of all the menus
	//if (num==0) app->destroywindow(this);
	// **** this does not seem to be working

	return 0;
}

//! In addition to the MenuSelector::CharInput, left goes up a menu, right goes down, ESC escapes.
int PopupMenu::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Enter) {
		MenuSelector::CharInput(ch,buffer,len,state,d);
		app->destroywindow(this);
		return 0;
	}

	if (ch==LAX_Left) { //left, go up a menu
		if (!parentmenu) return 0;
//		app->rundialog(parentmenu);
		app->setfocus(parentmenu,0,dynamic_cast<const LaxKeyboard*>(d));
		return 0;

	} else if (ch==LAX_Right) { //right
		if (!submenu) return 0;
//		app->rundialog(submenu);
		app->setfocus(submenu,0,dynamic_cast<const LaxKeyboard*>(d));
		return 0;

	} else if (ch==LAX_Esc) { //escape
		app->destroywindow(this);
		return 0;
	}
	return MenuSelector::CharInput(ch,buffer,len,state,d);
}


} // namespace Laxkit

