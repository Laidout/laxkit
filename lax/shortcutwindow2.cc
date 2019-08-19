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
//    Copyright (C) 2019 by Tom Lechner
//

#include <lax/shortcutwindow2.h>

#include <lax/menuinfo.h>
#include <lax/treeselector.h>
#include <lax/laxutils.h>
#include <lax/lineinput.h>
#include <lax/filedialog.h>
#include <lax/popupmenu.h>
#include <lax/button.h>
#include <lax/language.h>


#include <iostream>
using namespace std;
#define DBG


using namespace LaxFiles;

namespace Laxkit {


#define SEARCH_KEYS 1
#define SEARCH_TEXT 0


//--------------------------------------- ShortcutKBWindow -----------------------------------------------

/*! \class ShortcutKBWindow
 * Subclass of KeyboardWindow to more easily handle drag and drop and key events.
 */


ShortcutKBWindow::ShortcutKBWindow(anXWindow *parnt, const char *nname, const char *ntitle,
									unsigned long nstyle,
									int xx,int yy,int ww,int hh,int brder,
									anXWindow *prev,unsigned long nowner,const char *nsend,
									const char *kb)
  : KeyboardWindow(parnt, nname, ntitle, nstyle, xx,yy,ww,hh,brder, prev,nowner,nsend, kb)
{
	ttip = nullptr;
	rbdown = -1;
	rbdown_ch = 0;
	rbdown_mods = 0;
	rbdown_mode = 0;

	current_mode = 0;
}

void ShortcutKBWindow::DrawMouseOverTip(Key *key, double x, double y, double w, double h)
{
	if (!key->tag) return;

	Keymap *keymap = key->MatchMods(currentmods);
	ShortcutManager *manager = GetDefaultShortcutManager();
	WindowAction *action = manager->FindAction(current_area.c_str(), keymap->ch, currentmods, current_mode);
	if (!action && (currentmods & ShiftMask)) {
		if (key->keymaps.n > 1) keymap = key->keymaps.e[1];
		action = manager->FindAction(current_area.c_str(), keymap->ch, currentmods, current_mode);
	}
	if (!action) return;

	Displayer *dp = GetDisplayer();
	double th = dp->textheight();
	dp->NewBG(win_themestyle->bg);
	dp->NewFG(win_themestyle->fg);

	double ww = dp->textextent(action->description,-1, nullptr,nullptr);
	x += w/2;
	y += h + th;
	dp->drawrectangle(x-ww/2-th, y, ww+2*th, 2*th, 2);
	dp->textout(x,y+th, action->description,-1, LAX_CENTER);
}

void ShortcutKBWindow::UpdateMouseOver(int i)
{
//	if (!ttip) {
//		ttip = new MessageBar(nullptr, "keyhover",nullptr, ANXWIN_BARE,
//						0,0,0,0,1, "      ");
//		app->addwindow(ttip, 0);
//	}
//
//	if (i<0) {
//		app->unmapwindow(ttip);
//
//	} else {
//		Key *key = keyboard->keys[i];
//		//ttip->SetText(key->ch);
//		ttip->SetText("BLAH");
//		app->mapwindow(ttip);
//		//ttip->MoveResize();
//
//		//ShortcutManager *manager = GetDefaultShortcutManager();
//		//ShortcutHandler *shortcuts = manager->FindHandler(current_area.c_str());
//		//if (!shortcuts) return;
//	}
}

enum SCKBActions {
	SCKB_Clear_Key = KMENU_MAX + 1,
	SCKB_Assign_Action,
	SCKB_New_Action = 9999
};

int ShortcutKBWindow::Event(const Laxkit::EventData *e,const char *mes)
{
	return KeyboardWindow::Event(e,mes);
}

/*! Pop up context menu.
 */
int ShortcutKBWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	// Add New Action
	// Clear Key
	// Assign existing action

	MenuInfo *menu = new MenuInfo(_("Actions"));
	//menu->AddSep();

	ShortcutManager *manager = GetDefaultShortcutManager();
	ShortcutHandler *handler = manager->FindHandler(current_area.c_str());

	int i = scan(x,y);
	if (i >= 0) {
		Key *key = keyboard->keys[i];
		Keymap *keymap = key->MatchMods(currentmods);
		rbdown = i;
		rbdown_ch = keymap->ch;
		rbdown_mods = currentmods;

		//exclude ESC and mod keys
		if (!key->mod && key->keymaps.e[0]->ch != LAX_Esc) {
			if (key->tag) {
				menu->AddItem("Clear key", SCKB_Clear_Key,0, 0);
				menu->AddItem("Reassign action...", SCKB_Assign_Action,0, 0);
			}
			else menu->AddItem("Assign action...", SCKB_Assign_Action,0, 0);

			menu->SubMenu();
			WindowActions *actions = handler->Actions();
			for (int c=0; c<actions->n; c++) {
				menu->AddItem(actions->e[c]->name, SCKB_New_Action + c);//actions->e[c]->id);
			}
			menu->Sort();
			menu->AddSep();
			menu->AddItem(_("New action..."), SCKB_New_Action);
			menu->EndSubMenu();
			menu->AddSep();
		}
	}

	menu->AddItem("en_qwerty", KMENU_Select_Keyboard,0, 0);
	menu->AddItem("en_dvorak", KMENU_Select_Keyboard,0, 1);
	menu->AddItem(_("Load keyboard..."), KMENU_Load_Keyboard);


	app->rundialog(new PopupMenu("Shortcuts",_("Shortcuts"), 0,
							 0,0,0,0,1,
							 win_owner,"kbpopup",
							 d->id,
							 menu,1,NULL,
							 TREESEL_LEFT));

	return 0;
}

void ShortcutKBWindow::send(bool down, unsigned int key, unsigned int mods)
{
	UpdateCurrent();
}

void ShortcutKBWindow::UpdateCurrent()
{
	if (isblank(current_area.c_str())) {
		DBG cerr << "Warning! ShortcutWindow2::UpdateCurrent() missing current area!"<<endl;
		return;
	}

	ShortcutManager *manager = GetDefaultShortcutManager();
	ShortcutHandler *shortcuts = manager->FindHandler(current_area.c_str());
	if (!shortcuts) return;

	Keyboard *kb = GetKeyboard();

	//update key colors to reflect current mods
	WindowAction *action;
	for (int c=0; c<kb->keys.n; c++) {
		//associate keys with particular shortcut definitions (key+mods <-> action)
		Key *key = kb->keys.e[c];
		if (key->mod) continue;

		action = nullptr;
		for (int c2=0; c2<key->keymaps.n; c2++) {
			action = shortcuts->FindAction(key->keymaps.e[c2]->ch, currentmods, 0 /**** current_mode*/);
			if (action) break;
		}
		key->tag = action ? 1 : 0;
	}

	needtodraw = 1;
	//keyboard->Needtodraw(1);
}

//--------------------------------------- ShortcutTreeSelector2 -----------------------------------------------

/*! \class ShortcutTreeSelector2
 * Redefine to capture key input...
 * \todo there should probably be a better way for parents to intercept input to kids...
 */
class ShortcutTreeSelector2 : public TreeSelector
{
	double shiftext, controlext, metaext, altext;

  public:
	MenuItem *wait_for;
	int skipswap;

	ShortcutTreeSelector2(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			             int xx,int yy,int ww,int hh,int brder,
						 anXWindow *prev,unsigned long nowner=0,const char *mes=0,
						 unsigned long long nmstyle=0,MenuInfo *minfo=nullptr);
	~ShortcutTreeSelector2();
	virtual void Refresh();
	virtual void SwapBuffers();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int isPressableKey(unsigned int ch);
	virtual void UpdateSearch(MenuInfo *m,const char *str, int search_type);

	virtual void drawItemContents(MenuItem *i,int offset_x,int offset_y, int fill, int indent);
	virtual double getitemextent(MenuItem *mitem, double *w, double *h, double *gx, double *tx);
	virtual double drawMod(Displayer *dp,double x,double y, int mod);
};

ShortcutTreeSelector2::ShortcutTreeSelector2(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			             int xx,int yy,int ww,int hh,int brder,
						 anXWindow *prev,unsigned long nowner,const char *mes,
						 unsigned long long nmstyle,MenuInfo *minfo)
  : TreeSelector(parnt,nname,ntitle,nstyle|ANXWIN_HOVER_FOCUS,xx,yy,ww,hh,brder,prev,nowner,mes,nmstyle/*|TREESEL_FOLLOW_MOUSE*/,minfo)
{
	wait_for=nullptr;
	skipswap=0;
	DBG cerr <<"in ShortcutTreeSelector2(), id="<<object_id<<endl;
}

ShortcutTreeSelector2::~ShortcutTreeSelector2()
{
	DBG cerr <<"in ~ShortcutTreeSelector2(), id="<<object_id<<endl;
}

int ShortcutTreeSelector2::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (wait_for) {
		wait_for=nullptr;
		needtodraw=1;
		return 0;
	}
	return TreeSelector::LBDown(x,y,state,count, d);
}

int ShortcutTreeSelector2::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (wait_for) {
		return 0;
	}
	return TreeSelector::MouseMove(x,y,state,d);
}

int ShortcutTreeSelector2::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	if (mousedragmode!=0) return TreeSelector::LBUp(x,y,state,d);

	int onsub=0;
    int i=findItem(x,y,&onsub);

	if (onsub) return TreeSelector::LBUp(x,y,state,d);
	MenuItem *mi=item(i);
	if (mi && !mi->isSelected()) addselect(i,0);

	int detailhovered=-1, item=-1;
	buttondown.getextrainfo(d->id,LEFTBUTTON, &item,&detailhovered);
	if (detailhovered>0 || item<0 || item>=numItems()) return TreeSelector::LBUp(x,y,state,d);

	int isdragged=buttondown.isdragged(d->id,LEFTBUTTON);
	buttondown.up(d->id,LEFTBUTTON);
	if (isdragged) return TreeSelector::LBUp(x,y,state,d);;

	MenuItem *mitem=visibleitems.e(item);
	if (!mitem->parent || !mitem->parent->parent) return TreeSelector::LBUp(x,y,state,d);;
	// *** note this assumes having no parent item means this is a key-action instance

	 //now we know there was no dragging, and we click down on first detail block, so expect new key...
	wait_for=mitem;
	needtodraw=1;

	// *** assumes the flat view, each area is top level, keys are subs of that


	return 0;
}


static void AddAreaToMenu(MenuInfo *aream, ShortcutHandler *handler)
{
	char keyb[50]; //new key

	ShortcutDefs  *s=handler->Shortcuts();
	WindowActions *a=handler->Actions();
	WindowAction  *aa;


	 //output all bound keys
	if (s) {
		char buffer[1000];
		for (int c2=0; c2<s->n; c2++) {
			buffer[0]='\0';
			//manager->ShortcutString(s->e[c2], keyb, true);
			key_name_from_value(s->e[c2]->keys->key, keyb);

			aream->AddItem(keyb, s->e[c2]->keys->key, LAX_OFF, s->e[c2]->keys[0].state, nullptr);

			if (a) aa = a->FindAction(s->e[c2]->action); else aa=nullptr;
			if (aa) {
				 //print out string id and commented out description
				aream->AddDetail(aa->name,nullptr,aa->mode,aa->id);
				if (!isblank(aa->description)) 
					aream->AddDetail(aa->description,nullptr);
			} else {
				sprintf(buffer,"%d",s->e[c2]->action); //print out number only
				aream->AddDetail(buffer,nullptr);
			}
		}
	}

	 //output any unbound actions
	if (a) {
		int c2=0;
		for (c2=0; c2<a->n; c2++) {
			if (s && s->FindShortcutFromAction(a->e[c2]->id,0)) continue; //action is bound!
			aream->AddItem("");
			aream->AddDetail(a->e[c2]->name,nullptr,a->e[c2]->mode,a->e[c2]->id);
			if (!isblank(a->e[c2]->description)) aream->AddDetail(a->e[c2]->description,nullptr);
		}
	}

	aream->Sort(1);
}


/*! -1 for Escape or other chars that are supposed to break out of key waiting.
 * shift, control, alt, or windows/meta key return 0.
 * return 1 for key ok.
 */
int ShortcutTreeSelector2::isPressableKey(unsigned int ch)
{
	if (ch==LAX_Shift || ch==LAX_Control || ch==LAX_Alt || ch==LAX_Meta) return 0;
	if (ch==LAX_Esc) return -1;
	return 1;
}

int ShortcutTreeSelector2::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{

	if (wait_for==nullptr) {
		if (isblank(searchfilter) && (ch==LAX_Bksp || ch==LAX_Del)) {
			 // clear key from item being hovered over
			unsigned int oldkey, oldstate;
			MenuItem *i=item(ccuritem);
			if (!i || !i->parent || !i->parent->parent || !i->nextdetail) return 0;

			ShortcutManager *manager=GetDefaultShortcutManager();
			ShortcutHandler *handler=manager->FindHandler(i->parent->parent->name);
			if (!handler) return 0;
			manager->KeyAndState(i->name, &oldkey, &oldstate);
			handler->ClearShortcut(oldkey,oldstate,i->nextdetail->id);
			makestr(i->name,"");

			i->parent->menuitems.flush();
			AddAreaToMenu(i->parent, handler);

			needtobuildcache=1;
			needtodraw=1;
			return 0;
		}
		return TreeSelector::CharInput(ch,buffer,len,state,d);
	}

	//try to change shortcut.
	//Strategy here is to update inside shortcutmanager, delete and recreate all menu items for that area

	switch (isPressableKey(ch)) {
		case -1: { //escape key, never available as a shortcut key. used to exit grab key mode
			wait_for=nullptr;
			needtodraw=1;
			return 0;
		 }
		case 0: return 0; //usually mod keys ignored
	}

	state &= ShiftMask | ControlMask | MetaMask | AltMask;

	char keyb[50]; //new key
	ShortcutManager *manager = GetDefaultShortcutManager();
	manager->ShortcutString(ch,state, keyb, false);

	if (!strcmp(keyb,wait_for->name)) {
		//it's the same key
		wait_for=nullptr;
		needtodraw=1;
		return 0;
	}

	const char *area=wait_for->parent->parent->name;
	ShortcutHandler *handler=manager->FindHandler(area);

	unsigned int oldkey, oldstate;
	manager->KeyAndState(wait_for->name, &oldkey, &oldstate);

	//handler->ReassignKey(newkey, newstate, oldkey, oldstate, mode, action);
	handler->ReassignKey(ch, state, oldkey, oldstate, wait_for->nextdetail->id, wait_for->nextdetail->info);



	// update menu

	MenuInfo *aream=wait_for->parent;
	aream->menuitems.flush();
	AddAreaToMenu(aream, handler);


	needtobuildcache=1;
	wait_for=nullptr;
	needtodraw=1;

	return 0;
}





/*! Call TreeSelector::Refresh(), then if we are waiting for a key,
 * overwrite that cell to prompt for input.
 */
void ShortcutTreeSelector2::Refresh()
{
	if (!win_on || !needtodraw) return;

	Displayer *dp = MakeCurrent();

	shiftext   = dp->textextent(_("Shift"),-1, nullptr, nullptr);
	controlext = dp->textextent(_("Control"),-1, nullptr, nullptr);
	metaext    = dp->textextent(_("Meta"),-1, nullptr, nullptr);
	altext     = dp->textextent(_("Alt"),-1, nullptr, nullptr);


	skipswap=1;
	TreeSelector::Refresh();
	skipswap=0;

	if (wait_for) {
		dp->NewFG(0.,1.,0.);
		dp->textout(wait_for->x+offsetx,wait_for->y+offsety, _("<<press>>"), -1,  LAX_LEFT|LAX_TOP);
	}

	SwapBuffers();
}

void ShortcutTreeSelector2::drawItemContents(MenuItem *i,int offset_x,int offset_y, int fill, int indent)
{
	if (i->NumDetail() == 0) {
		TreeSelector::drawItemContents(i, offset_x,offset_y, fill, indent);
		return;
	}

	double x = i->x + offset_x;
	double y = i->y + offset_y;

	Displayer *dp = GetDisplayer();
	double thh = .5 * dp->textheight();

	x += shiftext + controlext + metaext + altext;
	y += i->h/2;
	dp->textout(x+thh*2, y, i->GetDetail(2)->name,-1, LAX_LEFT);
	x -= thh + dp->textout(x,y, i->name,-1, LAX_RIGHT);

	dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .25));
	if (i->info & ShiftMask)   x -= thh + drawMod(dp, x,y, ShiftMask);
	if (i->info & ControlMask) x -= thh + drawMod(dp, x,y, ControlMask);
	if (i->info & MetaMask)    x -= thh + drawMod(dp, x,y, MetaMask);
	if (i->info & AltMask)     x -= thh + drawMod(dp, x,y, AltMask);
	dp->NewFG(win_themestyle->fg);

}

double ShortcutTreeSelector2::getitemextent(MenuItem *mitem, //!< the index, MUST already be in bounds
                                double *w, //!< Put the x extent here
                                double *h, //!< Put the y extent here
                                double *gx, //!< Where the graphic would start, relative to whole item
                                double *tx  //!< Where the text would start, relative to whole item
                            )
{
	double xx = TreeSelector::getitemextent(mitem, w,h,gx,tx);
	if (h) *h *= 1.5;
	return xx;
}

double ShortcutTreeSelector2::drawMod(Displayer *dp,double x,double y, int mod)
{
	const char *text = nullptr;
	if (mod == ShiftMask)        text = _("Shift");
	else if (mod == ControlMask) text = _("Control");
	else if (mod == MetaMask)    text = _("Meta");
	else if (mod == AltMask)     text = _("Alt");

	double ext = dp->textout(x,y, text,-1, LAX_RIGHT);
	return ext;
}

/*! Hack to overlay something during a Refresh.
 */
void ShortcutTreeSelector2::SwapBuffers()
{
	if (skipswap) return;
	anXWindow::SwapBuffers();
}

/*! Recursively do the heavy lifting of updating the menu to search for str.
 * Uses search_type for the search type.
 *
 * If setinput!=0, then update the search input box.
 */
void ShortcutTreeSelector2::UpdateSearch(MenuInfo *m,const char *str, int search_type)
{
	if (m==nullptr) m=menu;
	needtobuildcache=1;
	needtodraw=1;

	MenuItem *mi, *mmi, *md;

	for (int c=0; c<m->menuitems.n; c++) {
		mi=m->menuitems.e[c];
		if (mi->hasSub()) {
			UpdateSearch(mi->GetSubmenu(0), str, search_type);
			continue; //assume only leaves have key info
		}

		if (search_type==SEARCH_KEYS) {
			 //search keys
			if (strstr(str,mi->name)) {
				 //hit!
				mi->SetState(MENU_SEARCH_HIT,1);
				while (mi->parent && mi->parent->parent) {
					mi=mi->parent->parent;
					mi->SetState(MENU_SEARCH_PARENT,1);
				}
			} else mi->SetState(MENU_SEARCH_HIDDEN,1);
			continue;
		} 

		md=mi->nextdetail;
		while (md) {
			if (strcasestr(str,md->name)) {
				 //hit!
				mi->SetState(MENU_SEARCH_HIT,1);
				mmi=mi;
				while (mmi && mmi->parent && mmi->parent->parent) {
					mmi=mmi->parent->parent;
					mmi->SetState(MENU_SEARCH_PARENT,1);
				}
				break;
			}

			md=md->nextdetail;
		}
		if (!md) mi->SetState(MENU_SEARCH_HIDDEN,1); //no detail match found
	}
}

//--------------------------------------- ShortcutWindow2 -----------------------------------------------

/*! \class ShortcutWindow2
 * Interactive keyboard to display and edit shortcuts.
 */


ShortcutWindow2::ShortcutWindow2(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,const char *place)
		: StackFrame(parnt,nname,ntitle,(nstyle&ANXWIN_MASK) | STACKF_VERTICAL,
					xx,yy,ww,hh,brder, nullptr,0,nullptr,
					0)
//		: RowFrame(parnt,nname,ntitle,(nstyle&ANXWIN_MASK)|ROWFRAME_ROWS|ROWFRAME_STRETCH_IN_COL|ROWFRAME_STRETCH_IN_ROW,
//					xx,yy,ww,hh,brder, nullptr,0,nullptr,
//					0)
{
	gap = win_themestyle->normal->textheight()*.6;

	swin_style  = (nstyle & ~ANXWIN_MASK);
	search_type = SEARCH_TEXT;  // 0 is search text, 1 is search keys

	initialarea = newstr(place);
	sc          = nullptr;
	textheader  = nullptr;
	keyboard    = nullptr;

	ShortcutTreeSelector2 *tree = new ShortcutTreeSelector2(this, "tree","tree",SW_RIGHT, 0,0,0,0,0, 
						nullptr,this->object_id,"tree", TREESEL_NO_LINES,nullptr);
	//tree->AddColumn(_("Key"),nullptr,-1);
	//tree->AddColumn(_("Action"),nullptr,-1);
	//tree->AddColumn(_("Description"),nullptr,-1);
	app->reparent(tree,this);
	tree->dec_count(); // *** why does uncommenting this make it crash on window close!?!?

	DBG cerr <<"in ShortcutWindow2(), id="<<object_id<<endl;
}

ShortcutWindow2::~ShortcutWindow2()
{
	delete[] textheader;

	if (sc) sc->dec_count();
	if (keyboard) keyboard->dec_count();
	if (initialarea) delete[] initialarea;

	DBG cerr <<"in ~ShortcutWindow2(), id="<<object_id<<endl;
}


int ShortcutWindow2::init()
{
	//keyboard 
	//
	//[display type] [search]_____________________x [expandall][contractall]
	//[current set:]_____________[save set][load set]
	//
	//display types:
	//  as input tree
	//  area, sorted by key
	//  area, sorted by action
	//  area, sorted by default
	//  keyboard

	ShortcutManager *manager = GetDefaultShortcutManager();


	int textheight = win_themestyle->normal->textheight();

	anXWindow *last=nullptr;

	//int addspacer=0;


	//--------------------------Name_______ [load][save][export]
	RowFrame *rowframe = new RowFrame(this, "rf",nullptr, ROWFRAME_ROWS, 0,0,0,0,0, nullptr,0,nullptr);

	LineInput *linp;
	last = linp = new LineInput(this, "setname","set",0, 0,0,0,0,1, last,object_id,"setname",
							_("Key set name"),manager->setname);
	linp->tooltip(_("Name of the shortcut set"));
	rowframe->AddWin(linp,1, 10*textheight,0,20*textheight,50,0, textheight*1.5,0,0,50,0, -1);

	Button *but;
	last = but = new Button(this,"save","save",BUTTON_SAVE, 0,0,0,0,1, last,object_id,"save");
	but->tooltip(_("Save the shortcut set to a file"));
	rowframe->AddWin(but,1, but->win_w,0,0,50,0, textheight*1.5,0,0,50,0, -1);

	last = but = new Button(this,"load","load",BUTTON_OPEN, 0,0,0,0,1, last,object_id,"load");
	but->tooltip(_("Load the shortcut set from a file"));
	rowframe->AddWin(but,1, but->win_w,0,0,50,0, textheight*1.5,0,0,50,0, -1);

	rowframe->AddHSpacer(textheight);

	last = but = new Button(this,"settings",nullptr,IBUT_FLAT, 0,0,1.5*textheight,1.5*textheight,0, last,object_id,"settings");
	but->tooltip(_("Keyboard settings"));
	but->SetGraphic(THING_Wrench, textheight, textheight);
	rowframe->AddWin(but,1, but->win_w,0,0,50,0, textheight*1.5,0,0,50,0, -1);

	AddWin(rowframe,1, 400,200,10000,50,0, 2*textheight,0,0,50,0, -1);


	//---------------------------- Keyboard
	if (!keyboard) {
		keyboard = new ShortcutKBWindow(this, "Keyboard","Keyboard", ANXWIN_HOVER_FOCUS,
				0,0,0,0,0, nullptr,object_id,"keyboard", nullptr);
	}

	AddWin(keyboard,1, keyboard->win_w,keyboard->win_w/2,10000,50,0, keyboard->win_h,0,0,50,0, -1);
	//AddNull();


//	if (swin_style&SHORTCUTW_Show_Search) {
//		// [search keys]Search____________[x][search actions] [collapse all][expand all]
//
//		Button *but;
//		last=but=new Button(this,"searchkeys","searchkeys",BUTTON_OK, 0,0,0,0,1, last,object_id,"searchkeys",
//							1,_("Search keys"));
//		but->tooltip(_("Clear search term"));
//		AddWin(but,1, but->win_w,0,0,50,0, textheight*1.25,0,0,50,0, -1);
//
//
//		last=but=new Button(this,"searchaction","searchaction",BUTTON_OK, 0,0,0,0,1, last,object_id,"searchaction",
//							3,_("Search"));
//		but->tooltip(_("Search actions"));
//		AddWin(but,1, but->win_w,0,0,50,0, textheight*1.25,0,0,50,0, -1);
//
//
//		LineInput *linp;
//		last=linp=new LineInput(this, "searchterm","searchterm",0, 0,0,0,0,1, last,object_id,"searchterm",
//								nullptr);
//		linp->tooltip(_("Search term. Type over keys to search by key press."));
//		AddWin(linp,1, 200,0,10000,50,0, textheight*1.25,0,0,50,0, -1);
//		// *** constant search, not activated search?
//
//
//		last=but=new Button(this,"clearsearch","clearsearch",BUTTON_CANCEL, 0,0,0,0,1, last,object_id,"clearsearch",
//							2,"x");
//		but->tooltip(_("Clear search term"));
//		AddWin(but,1, but->win_w,0,0,50,0, textheight*1.25,0,0,50,0, -1);
//
//
//		AddNull();
//		addspacer=1;
//	}


	//AddNull();
	//addspacer=1;


//	if (addspacer) { AddVSpacer(textheight/2,0,0,50); AddNull(); }

	
	 //define the shortcuts tree
	MenuInfo *menu = new MenuInfo;
	MenuInfo *areas = new MenuInfo;

	const char *place = initialarea;
	//int initial = -1;
	if (isblank(place)) place = nullptr;

	for (int c=0; c<manager->shortcuts.n; c++) {
		if (place && !strcmp(place,manager->shortcuts.e[c]->area)) {
			keyboard->current_area = manager->shortcuts.e[c]->area;
			//initial = c;

			menu->AddItem(manager->shortcuts.e[c]->area,0,MENU_OPEN,0,nullptr,-1,1); //add open
			menu->SubMenu();
			AddAreaToMenu(menu->curmenu, manager->shortcuts.e[c]);
			menu->EndSubMenu();
		} //else menu->AddItem(manager->shortcuts.e[c]->area); //add closed

		areas->AddItem(manager->shortcuts.e[c]->area);

	}

	StackFrame *hframe = new StackFrame(this, nullptr,nullptr,0, 0,0,0,0,0, nullptr,0,nullptr, -1);

	//left side, all the known areas
	TreeSelector *areawindow = new TreeSelector(hframe, "areas","areas",SW_RIGHT,
						0,0,0,0,0, nullptr,this->object_id,"areas",
						TREESEL_SEND_ON_UP | TREESEL_NO_LINES,
						nullptr);
	areawindow->InstallMenu(areas);
	areawindow->Select(0);
	areas->dec_count();
	hframe->AddWin(areawindow,1);

	//right side, shortcuts for currently selected areas
	ShortcutTreeSelector2 *tree = dynamic_cast<ShortcutTreeSelector2*>(findChildWindowByName("tree", true));
	tree->InstallMenu(menu);
	menu->dec_count();

	hframe->AddWin(tree,0);


	AddWin(hframe,1, 200,0,10000,50,0, 200,0,10000,50,0, -1);


	keyboard->UpdateCurrent();
	Sync(1);

	return 0;
}


int ShortcutWindow2::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	return anXWindow::CharInput(ch,buffer,len,state,kb);
}

LaxFiles::Attribute *ShortcutWindow2::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	att=anXWindow::dump_out_atts(att,what,context);
	TreeSelector *tree = dynamic_cast<TreeSelector*>(findChildWindowByName("tree", true));

	if (tree && tree->columns.n) {
		char scratch[30];
		int w;
		for (int c=0; c<tree->columns.n; c++) {
			w=tree->columns.e[c]->width;
			sprintf(scratch,"%d",w>0?w:10);
			//sprintf(scratch,"%d",tree->columns.e[c]->pos);
			att->push("column",scratch);
		}
	}
	return att;
}

void ShortcutWindow2::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	TreeSelector *tree=dynamic_cast<TreeSelector*>(findChildWindowByName("tree", true));
	if (!tree) return;

	char *name,*value;
	int column=0;
	int i;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"column")) {
			IntAttribute(value,&i,nullptr);
			if (column>=tree->columns.n) {
				tree->AddColumn(nullptr,nullptr, i,0, 0,-1);
				DBG cerr << "*** warning: adding unexpected column in ShortcutWindow2!"<<endl;

			} else tree->columns.e[column]->width=i;
			column++;
		}
	}
	int pos=0;
	for (int c=0; c<tree->columns.n; c++) {
		tree->columns.e[c]->pos=pos;
		pos+=tree->columns.e[c]->width;
	}
}


//! Get search term from "searchterm" window, and update in the tree.
void ShortcutWindow2::UpdateSearch()
{
	LineInput *b=(LineInput*)findChildWindowByName("searchterm");
	if (!b) return;
	ShortcutTreeSelector2 *t=(ShortcutTreeSelector2*)findChildWindowByName("tree", true);
	
	t->Menu()->ClearSearch();
	t->UpdateSearch(nullptr, b->GetCText(), search_type);
}

/*! Set the search string to str AND set it in the edit box.
 * searchtype==-1 means use current. 0 is search text. 1 is search keys.
 */
int ShortcutWindow2::SetSearch(const char *str, int searchtype)
{
	if (searchtype>=0) search_type=searchtype;

	LineInput *b=(LineInput*)findChildWindowByName("searchterm");
	if (b) b->SetText(str);

	UpdateSearch();
	needtodraw=1;

	return 0;
}

int ShortcutWindow2::SelectArea(const char *area)
{
	if (isblank(area)) return 1;

	ShortcutManager *manager = GetDefaultShortcutManager();

	MenuInfo *menu = new MenuInfo;
	for (int c=0; c<manager->shortcuts.n; c++) {
		if (!strcmp(area, manager->shortcuts.e[c]->area)) {
			keyboard->current_area = manager->shortcuts.e[c]->area;

			menu->AddItem(manager->shortcuts.e[c]->area,0,MENU_OPEN,0,nullptr,-1,1); //add open
			menu->SubMenu();
			AddAreaToMenu(menu->curmenu, manager->shortcuts.e[c]);
			menu->Sort();
			menu->EndSubMenu();
		} //else menu->AddItem(manager->shortcuts.e[c]->area); //add closed
	}

	ShortcutTreeSelector2 *tree = dynamic_cast<ShortcutTreeSelector2*>(findChildWindowByName("tree", true));
	tree->InstallMenu(menu);
	menu->dec_count();
	keyboard->UpdateCurrent();

	return 0;
}

int ShortcutWindow2::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"keyboard")) {
		//const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		//DBG cerr << "Shortcutwindow2 key event: "<< s->info1<<" "<<s->info2<<" "<<s->info3<<endl;
		////if (s->info3) UpdateCurrent(); //was down on a mod key
		//UpdateCurrent();
		return 0;

	} else if (!strcmp(mes,"areas")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		DBG cerr << "areas change: " <<(s->str ? s->str : "none" )<<endl; //the item
		SelectArea(s->str);
		return 0;

	} else if (!strcmp(mes, "kbpopup")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);

		if (s->info2 == SCKB_Clear_Key && keyboard->rbdown >= 0) {
			Key *key = keyboard->GetKeyboard()->keys[keyboard->rbdown];

			ShortcutManager *manager = GetDefaultShortcutManager();
			ShortcutHandler *handler = manager->FindHandler(keyboard->current_area.c_str());
			if (!handler) return 0;

			//Update shortcut manager
			handler->ClearShortcut(keyboard->rbdown_ch, keyboard->rbdown_mods, 0 /* ***fixme: mode */);

			//update keyboard
			key->tag = 0;
			 
			//update action list
			//ShortcutTreeSelector2 *tree = dynamic_cast<ShortcutTreeSelector2*>(findChildWindowByName("tree", true));
			SelectArea(keyboard->current_area.c_str());

			needtodraw=1;
			return 0;

		} else if (s->info2 >= SCKB_New_Action) {
			Key *key = keyboard->GetKeyboard()->keys[keyboard->rbdown];

			ShortcutManager *manager = GetDefaultShortcutManager();
			ShortcutHandler *handler = manager->FindHandler(keyboard->current_area.c_str());

			//update shortcut manager
			WindowActions *actions = handler->Actions();
			WindowAction *newaction = actions->ActionAt(s->info2 - SCKB_New_Action);
			if (newaction) {
				handler->UpdateKey(keyboard->rbdown_ch, keyboard->rbdown_mods, keyboard->rbdown_mode, newaction->id);
			}

			//update keyboard
			key->tag = 1;

			//update action list
			SelectArea(keyboard->current_area.c_str());

			needtodraw=1;
			return 0;
		}

		return 0;

	} else if (!strcmp(mes,"searchterm")) {
		// *** 
		LineInput *b=(LineInput*)findChildWindowByName("searchterm");
		if (!b) return 0;
		UpdateSearch();
		return 0;

	} else if (!strcmp(mes,"searchkeys")) {
		if (search_type==SEARCH_KEYS) return 0;
		search_type=SEARCH_KEYS;
		UpdateSearch();
		return 0;

	} else if (!strcmp(mes,"searchaction")) {
		if (search_type==SEARCH_TEXT) return 0;
		search_type=SEARCH_TEXT;
		UpdateSearch();
		return 0;

	} else if (!strcmp(mes,"clearsearch")) {
		SetSearch(nullptr);
		return 0;

	} else if (!strcmp(mes,"expandall")) {
		// ***

	} else if (!strcmp(mes,"collapseall")) {
		// ***

	} else if (!strcmp(mes,"save")) {
		ShortcutManager *manager=GetDefaultShortcutManager();
		FileDialog *f=new FileDialog(nullptr,"save",_("Save shortcuts"),0, 0,0,500,500,0,object_id,"savethis",
									FILES_SAVE|FILES_ASK_TO_OVERWRITE,
									manager->setfile);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"savethis")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 1;
		if (isblank(s->str)) return 1;
		ShortcutManager *manager = GetDefaultShortcutManager();

		LineInput *setnameinput = dynamic_cast<LineInput*>(findChildWindowByName("setname", true));
		if (!isblank(setnameinput->GetCText())) { //use displayed name as manager name
			makestr(manager->setname, setnameinput->GetCText());
		}
		if (isblank(manager->setname)) { //set name to filename if we hadn't entered one into the input
			makestr(manager->setname, lax_basename(s->str));
		}
		if (isblank(setnameinput->GetCText())) setnameinput->SetText(manager->setname);
		manager->Save(s->str, textheader);
		return 0;

	} else if (!strcmp(mes,"load")) {
		ShortcutManager *manager=GetDefaultShortcutManager();
		FileDialog *f = new FileDialog(nullptr,"load",_("Load"),0, 0,0,500,500,0,object_id,"loadthis",FILES_OPEN_ONE,
									manager->setfile);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"loadthis")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 1;
		if (isblank(s->str)) return 1;
		ShortcutManager *manager = GetDefaultShortcutManager();
		manager->Load(s->str); 
		LineInput *setnameinput = dynamic_cast<LineInput*>(findChildWindowByName("setname", true));
		setnameinput->SetText(manager->setname);
		SelectArea(keyboard->current_area.c_str());

		//*** must merge with existing? currently deletes all old, OR reset actions, just load keys

	} else if (!strcmp(mes,"settings")) {
		MenuInfo *menu = new MenuInfo(_("Settings"));
		menu->AddSep(_("Keyboards"));
		menu->AddItem("en_qwerty", KMENU_Select_Keyboard,0, 0);
		menu->AddItem("en_dvorak", KMENU_Select_Keyboard,0, 1);
		menu->AddItem(_("Load keyboard..."), KMENU_Load_Keyboard);
		menu->AddItem(_("Reset Keymap"), KMENU_Reset_Keyboard);

		app->rundialog(new PopupMenu("Settings",_("Settings"), 0,
								 0,0,0,0,1,
								 object_id,"settingsm",
								 -1,
								 menu,1,nullptr,
								 TREESEL_LEFT));

	} else if (!strcmp(mes,"settingsm")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 1;

		if (s->info2 == KMENU_Reset_Keyboard) {
			DBG cerr << "Reset keyboard to locale..."<<endl;
			keyboard->GetKeyboard()->ApplyCurrentLocale();
			SelectArea(keyboard->current_area.c_str());
			return 0;
		}

		return 0;
	}

	return StackFrame::Event(e,mes);
	//return anXWindow::Event(e,mes);
}



} //namespace Laxkit


