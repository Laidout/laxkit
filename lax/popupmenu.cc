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
 *  Almost all the real work is done in TreeSelector. This class adds features to
 *  keep track of which menu the mouse enters, and modify which submenus are actually
 *  popped up up accordingly. This is done basically by keeping a linked lists of popped up
 *  menus, and checks where the mouse is with each mouse movement. The pointer is grabbed
 *  by each menu it enters, so this is relatively easy.
 *
 *  It is worth noting that each PopupMenu is created from scratch each time it is needed,
 *  and deleted when not needed. This menu system does not use persistent windows that are 
 *  just mapped on and off.
 * 
 *  \todo add support for tearoff menus
 *  \todo POPMENU_LEAVE_DESTROYS should abandon the windows if the mouse leaves a menu, and does not enter another one.
 *
 *  <pre>
 *  ***TODO:
 *  // *** clicking on the item leading to an open submenu should simply close the submenu, not select said item
 *  // **** must process leavenotify/enternotify to make sure all ok
 *  // *** focus must revert back to the calling window!!
 *  // 
 *  //	if you intend to modify menu states, each id must be unique (see Modify(...))
 *  // ids should be non-negative
 *  // ---
 *  </pre>
 */

/*! \var PopupMenu *PopupMenu::parentmenu
 * \brief The popup that is the parent of this.
 */
/*! \var PopupMenu *PopupMenu::submenu
 * \brief The popup that is the (open) child of this.
 */

	
//! Constructor. Sets win_style to have ANXWIN_BARE|ANXWIN_HOVER_FOCUS.
/*! This creates a TreeSelector that has no parent, and is set with\n
 *   menustyle= TREESEL_ZERO_OR_ONE | TREESEL_CURSSELECTS | TREESEL_FOLLOW_MOUSE | TREESEL_SEND_ON_UP |
 *   TREESEL_GRAB_ON_ENTER | TREESEL_OUT_CLICK_DESTROYS | TREESEL_CLICK_UP_DESTROYS. 
 *
 *   If nparentmenu is NULL, then TREESEL_GRAB_ON_MAP is also set.
 *
 *   Ignores parnt and nstyle, and uses NULL and (ANXWIN_BARE|ANXWIN_HOVER_FOCUS) instead.
 *
 *	 This always tries to popup near the mouse, next to nparentmenu.
 */
PopupMenu::PopupMenu(const char *nname, const char *ntitle, unsigned long long style,
				int xx,int yy,int ww,int hh,int brder,
				unsigned long  nowner,const char *mes,
				int mouseid,
				MenuInfo *usethismenu,        //!< Pass in a MenuInfo object
				char absorb_count,            //!< Whether to absorb the count of usethismenu 
				PopupMenu *nparentmenu,      //!< This is the calling menu, if any
				unsigned long long extrastyle //!< Extra TreeSelector styles to pass along
			)
	//TreeSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			                //int xx,int yy,int ww,int hh,int brder,
							//anXWindow *prev,unsigned long nowner=0,const char *mes=0,
							//unsigned long long nmstyle=0, MenuInfo *minfo=NULL);
//
					: TreeSelector(NULL,nname,(ntitle?ntitle:"Menu"),
							ANXWIN_BARE
							|ANXWIN_HOVER_FOCUS
							|ANXWIN_OUT_CLICK_DESTROYS,
							xx,yy,ww,hh,brder,
							nparentmenu, nowner, mes,
							TREESEL_ZERO_OR_ONE
						   |TREESEL_CURSSELECTS
						   |TREESEL_FOLLOW_MOUSE
						   |TREESEL_SEND_ON_UP
						   |TREESEL_DESTROY_ON_UP
						   |TREESEL_NO_LINES
						   |TREESEL_FLAT_COLOR
						   |TREESEL_SUB_ON_RIGHT
						   |TREESEL_SELECT_LEAF_ONLY,
							usethismenu)
{
	menustyle |= (extrastyle &
					( TREESEL_LEFT
					 |TREESEL_RIGHT
					 |TREESEL_GRAPHIC_ON_RIGHT
					 |TREESEL_SEND_STRINGS
					 |TREESEL_SEND_PATH
					 |TREESEL_SEND_HOVERED
					 |TREESEL_LIVE_SEARCH 
					)
			   );
	parentmenu = nparentmenu;
	submenu = NULL;
	if (usethismenu && absorb_count) usethismenu->dec_count(); //was inc'd in TreeSelector

	if (!nparentmenu) menustyle |= TREESEL_GRAB_ON_MAP;
	//if (menu) Select(0); //*** if 0 has a menu then popit up!! also should wrap near parentmenu->selected item!!
	if (mouseid) WrapToMouse(mouseid,nparentmenu);
	else WrapToPosition(xx,yy,0,nparentmenu);

	pad = win_themestyle->normal->textheight()/3;
	win_border = 1;

	outtimer = 0;

	if (menu) {
		menu->SetRecursively(MENU_SELECTED, 0);
		menu->SetRecursively(LAX_ON, 0);
	}
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
		submenu->parentmenu = NULL;
		app->destroywindow(submenu);
	}
	if (parentmenu) { // if done with it, should have been set to NULL elsewhere
		parentmenu->submenu = NULL;
		app->destroywindow(parentmenu);
	}

}

////! Focus off maybe destroys the menus.
///*! Also, if TREESEL_FOCUS_OFF_DESTROYS, then an off focus destroys this window.
// */
//int PopupMenu::FocusOff(const FocusChangeData *e)
//{
//  app->addtimer(***.5 second) ... if after that time, the mouse is not in a popup menu, then destroy the menu setup
//---------------
//	DBG cerr <<"PopupMenu "<<WindowTitle()<<" FocusOff..."<<endl;
//	anXWindow::FocusOff(e);
//	if (!win_active) {
//		if (menustyle&TREESEL_DESTROY_ON_FOCUS_OFF) {
//			app->destroywindow(this);
//			return 0;
//		}
//		drawitem(ccuritem);
//	}
//	return 0;
//}

void PopupMenu::RemoveSubmenu()
{
	if (!submenu) return;
	submenu->parentmenu=NULL; // Important: detach the submenu(s) before destroying.
	app->destroywindow(submenu);
	submenu=NULL;
}

//! Must capture before and after selecting, to control what popups should be up.
void PopupMenu::addselect(int i,unsigned int state)
{
	//int oldcuritem = curitem;
	TreeSelector::addselect(i,state);

	 // must remove the old popped up submenu(s)
	//MenuItem *ii = item(oldcuritem);

//	if (curitem >= 0 && ii && ii->hasSub() && submenu) { 
//		submenu->parentmenu=NULL; // Important: detach the submenu(s) before destroying.
//		app->destroywindow(submenu);
//		submenu=NULL;
//	}

	 // must establish a new popped up submenu
	 //*** this needs work, maybe need a MenuItem::Open/CloseSubmenu()??
	 //*** when wrapping to mouse is always Selecting 0, and so makes 
	 //almost the whole menu down below the screen...
	MenuItem *ii = item(curitem);

	if (ii && ii->hasSub()) { 
		MenuInfo *minfo = ii->GetSubmenu(1);
		//if (minfo->NumVisible() == 0) minfo = NULL;
		
		if (minfo && (!submenu || submenu->Menu() != minfo)) {
			RemoveSubmenu();

			char *blah = numtostr((int)getUniqueNumber()); //make a unique window name for debugging purposes
			IntRectangle rect;
			findRect(curitem,&rect);
			anXWindow *win=this;
			while (win) {
				rect.x+=win->win_x;
				rect.y+=win->win_y;
				win = win->win_parent;
			}
			submenu = new PopupMenu(blah, blah, 0,
									rect.x+rect.width/2,rect.y+rect.height/2,0,0,win_border,win_owner,win_sendthis,
									0,
									minfo,0,this,
									menustyle);
			delete[] blah;
			app->rundialog(submenu,this);

			//DBG if (submenu->window) app->setfocus(submenu);
			//DBG else cerr <<"============Set focus fail!"<<endl;
		}
	} else if (ii) RemoveSubmenu();
}

//! Right button currently just escapes out of the window.
/*! TODO: might want to think about some mechanism to have it
 * easy to popup another popup window to modify something
 * about the item that the mouse is over, something akin to
 * "create a link to this on the desktop".
 */
int PopupMenu::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	return TreeSelector::RBDown(x,y,state,count,d);
}

//! RBUp escapes out of the window entirely.
int PopupMenu::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	int c = TreeSelector::RBUp(x,y,state,d);
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
		return TreeSelector::MouseMove(x,y,state,d);
	}

	 // else must check to see if it is in another one of the chain
	 // Not necessary to destroy or add windows here, that is handled when addselect is performed.
	 // scan submenus starting from the most submenu:
	PopupMenu *check = this;
	while (check->submenu) check = check->submenu;

	if (check) {
		while (check && !check->MouseIn(x+win_x-check->win_x,y+win_y-check->win_y)) check = check->parentmenu;
	}


	if (check) { // is in one of the related windows, just shift the pointer grab
		DBG cerr <<"---==< Grabbing: "<<check->WindowTitle()<<" ...";
		//app->rundialog(check);
		if (outtimer) {
			app->removetimer(this, outtimer);
			outtimer = 0;
		}
		app->setfocus(check,0,d->paired_keyboard);
		const_cast<LaxMouse*>(d)->grabDevice(check);

	} else {
		DBG cerr <<"------< Mouse is not in a popup menu, setting timer to check"<<endl;

		 //make this window idle after a second to check that the mouse is still in a related menu
		if (outtimer) {
			// *** there should be a modify timer
			app->removetimer(this, outtimer);
			outtimer = 0;
		}
		outtimer = app->addtimer(this, 1000, 1000, 1000);
											
	}
	return 0;
}

/*! If there are no mice in any of the related menus, then destroy all the related menus.
 */
int PopupMenu::Idle(int tid, double delta)
{
	if (tid != outtimer) return TreeSelector::Idle(tid,delta);
	app->removetimer(this,tid);
	outtimer = 0;

	LaxMouse *mouse;
	anXWindow *win;
	int num = 0;

	for (int c=0; c<app->devicemanager->NumDevices(); c++) {
		mouse = dynamic_cast<LaxMouse*>(app->devicemanager->Device(c));
		if (!mouse) continue;
		int x,y;
		unsigned int mask;

		if (mouseposition(mouse->id, NULL, &x,&y, &mask, &win)!=0) {
			if (!win) continue;

			PopupMenu *check = this;
			while (check->parentmenu) check = check->parentmenu;
			while (check) {
				if (check == win) break;
				check = check->submenu;
			}

			if (check) { num++; continue; }
		} else {
			// Pointer is probably not on same screen as trackwindow
		}
	}

	DBG cerr <<"PopupMenu::Idle outtimer check done: "<<num<<endl;

	 //no mice in any of the windows, so get rid of all the menus
	//if (num==0) app->destroywindow(this);
	// **** this does not seem to be working

	return 0;
}

//! In addition to the TreeSelector::CharInput, left goes up a menu, right goes down, ESC escapes.
int PopupMenu::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Enter) {
		TreeSelector::CharInput(ch,buffer,len,state,d);
		app->destroywindow(this);
		return 0;
	}

	if (ch==LAX_Left) { //left, go up a menu
		if (!parentmenu) return 0;
		app->setfocus(parentmenu,0,dynamic_cast<const LaxKeyboard*>(d));
		//curitem = -1;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Right) { //right, go down into submenu
		if (!submenu) return 0;
		app->setfocus(submenu,0,dynamic_cast<const LaxKeyboard*>(d));
		//curitem = -1;
		needtodraw=1;
		return 0;

	}

	//search only happens in top parent
	PopupMenu *top = this;
	while (top->parentmenu) top = top->parentmenu;

	if ((ch==LAX_Esc && top->searchfilter)
		|| (ch==LAX_Bksp && top->searchfilter) 
		|| (((state&LAX_STATE_MASK)|ShiftMask)==ShiftMask && ch < 0x1000000 && HasStyle(TREESEL_LIVE_SEARCH))
	   ) {
		//search related things.. this needs to be updated when TreeSelector ifs get updated
		// pass to top menu
		if (top != this) {
        	app->setfocus(top,0,NULL);
			top->CharInput(ch, buffer, len, state, d);
			return 0;
		}
	}
     
	
	int status = TreeSelector::CharInput(ch,buffer,len,state,d);

	if (status==1 && ch==LAX_Esc) { //escape to destroy menu
		app->destroywindow(this);
		return 0;
	}

	return status;
}

int PopupMenu::UpdateSearch(const char *searchterm, bool isprogressive)
{
	PopupMenu *top = Top();
	if (this != top) return top->UpdateSearch(searchterm, isprogressive);

	if (submenu) {
		submenu->parentmenu = NULL;
		app->destroywindow(submenu);
		submenu = NULL;
	}
	return TreeSelector::UpdateSearch(searchterm, isprogressive);
}

PopupMenu *PopupMenu::Top()
{
	PopupMenu *top = this;
	while (top->parentmenu) top = top->parentmenu;
	return top;
}

} // namespace Laxkit

