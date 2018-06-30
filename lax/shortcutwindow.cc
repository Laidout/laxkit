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
//    Copyright (C) 2012-2013 by Tom Lechner
//

#include <lax/shortcutwindow.h>
#include <lax/menuinfo.h>
#include <lax/treeselector.h>
#include <lax/laxutils.h>
#include <lax/lineinput.h>
#include <lax/filedialog.h>
#include <lax/button.h>
#include <lax/language.h>


#include <iostream>
using namespace std;
#define DBG


using namespace LaxFiles;

namespace Laxkit {


#define SEARCH_KEYS 1
#define SEARCH_TEXT 0

//--------------------------------------- ShortcutTreeSelector -----------------------------------------------

/*! \class ShortcutTreeSelector
 * Redefine to capture key input...
 * \todo there should probably be a better way for parents to intercept input to kids...
 */
class ShortcutTreeSelector : public TreeSelector
{
  public:
	MenuItem *wait_for;
	int skipswap;

	ShortcutTreeSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			             int xx,int yy,int ww,int hh,int brder,
						 anXWindow *prev,unsigned long nowner=0,const char *mes=0,
						 unsigned long long nmstyle=0,MenuInfo *minfo=NULL);
	~ShortcutTreeSelector();
	virtual void Refresh();
	virtual void SwapBuffers();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int isPressableKey(unsigned int ch);
	virtual void UpdateSearch(MenuInfo *m,const char *str, int search_type);
};

ShortcutTreeSelector::ShortcutTreeSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			             int xx,int yy,int ww,int hh,int brder,
						 anXWindow *prev,unsigned long nowner,const char *mes,
						 unsigned long long nmstyle,MenuInfo *minfo)
  : TreeSelector(parnt,nname,ntitle,nstyle|ANXWIN_HOVER_FOCUS,xx,yy,ww,hh,brder,prev,nowner,mes,nmstyle|TREESEL_FOLLOW_MOUSE,minfo)
{
	wait_for=NULL;
	skipswap=0;
	DBG cerr <<"in ShortcutTreeSelector(), id="<<object_id<<endl;
}

ShortcutTreeSelector::~ShortcutTreeSelector()
{
	DBG cerr <<"in ~ShortcutTreeSelector(), id="<<object_id<<endl;
}

int ShortcutTreeSelector::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (wait_for) {
		wait_for=NULL;
		needtodraw=1;
		return 0;
	}
	return TreeSelector::LBDown(x,y,state,count, d);
}

int ShortcutTreeSelector::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (wait_for) {
		return 0;
	}
	return TreeSelector::MouseMove(x,y,state,d);
}

int ShortcutTreeSelector::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
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
	ShortcutManager *manager=GetDefaultShortcutManager();
	char keyb[50]; //new key

	ShortcutDefs  *s=handler->Shortcuts();
	WindowActions *a=handler->Actions();
	WindowAction  *aa;


	 //output all bound keys
	if (s) {
		char buffer[1000];
		for (int c2=0; c2<s->n; c2++) {
			buffer[0]='\0';
			manager->ShortcutString(s->e[c2], keyb);
			aream->AddItem(keyb);

			if (a) aa=a->FindAction(s->e[c2]->action); else aa=NULL;
			if (aa) {
				 //print out string id and commented out description
				aream->AddDetail(aa->name,NULL,aa->mode,aa->id);
				if (!isblank(aa->description)) 
					aream->AddDetail(aa->description,NULL);
			} else {
				sprintf(buffer,"%d",s->e[c2]->action); //print out number only
				aream->AddDetail(buffer,NULL);
			}
		}
	}

	 //output any unbound actions
	if (a) {
		int c2=0;
		for (c2=0; c2<a->n; c2++) {
			if (s && s->FindShortcutFromAction(a->e[c2]->id,0)) continue; //action is bound!
			aream->AddItem("");
			aream->AddDetail(a->e[c2]->name,NULL,a->e[c2]->mode,a->e[c2]->id);
			if (!isblank(a->e[c2]->description)) aream->AddDetail(a->e[c2]->description,NULL);
		}
	}

	aream->Sort(1);
}


/*! -1 for Escape or other chars that are supposed to break out of key waiting.
 * shift, control, alt, or windows/meta key return 0.
 * return 1 for key ok.
 */
int ShortcutTreeSelector::isPressableKey(unsigned int ch)
{
	if (ch==LAX_Shift || ch==LAX_Control || ch==LAX_Alt || ch==LAX_Meta) return 0;
	if (ch==LAX_Esc) return -1;
	return 1;
}

int ShortcutTreeSelector::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{

	if (wait_for==NULL) {
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
		case -1: { //escape key, never available as a shortcut key
			wait_for=NULL;
			needtodraw=1;
			return 0;
		 }
		case 0: return 0; //usually mod keys ignored
	}

	state&=ShiftMask|ControlMask|MetaMask|AltMask;

	char keyb[50]; //new key
	ShortcutManager *manager=GetDefaultShortcutManager();
	manager->ShortcutString(ch,state, keyb);

	if (!strcmp(keyb,wait_for->name)) {
		//it's the same key
		wait_for=NULL;
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
	wait_for=NULL;
	needtodraw=1;

	return 0;
}





/*! Call TreeSelector::Refresh(), then if we are waiting for a key,
 * overwrite that cell to prompt for input.
 */
void ShortcutTreeSelector::Refresh()
{
	if (!win_on || !needtodraw) return;
	skipswap=1;
	TreeSelector::Refresh();
	skipswap=0;

	if (wait_for) {
		foreground_color(0.,1.,0.);
		textout(this, _("<<press>>"), -1, wait_for->x+offsetx,wait_for->y+offsety,LAX_LEFT|LAX_TOP);
	}

	SwapBuffers();
}

/*! Hack to overlay something during a Refresh.
 */
void ShortcutTreeSelector::SwapBuffers()
{
	if (skipswap) return;
	anXWindow::SwapBuffers();
}

/*! Recursively do the heavy lifting of updating the menu to search for str.
 * Uses search_type for the search type.
 *
 * If setinput!=0, then update the search input box.
 */
void ShortcutTreeSelector::UpdateSearch(MenuInfo *m,const char *str, int search_type)
{
	if (m==NULL) m=menu;
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

//--------------------------------------- ShortcutWindow -----------------------------------------------

/*! \class ShortcutWindow
 * Hold a TreeSelector to show and edit shortcuts.
 */


ShortcutWindow::ShortcutWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,const char *place)
		: RowFrame(parnt,nname,ntitle,(nstyle&ANXWIN_MASK)|ROWFRAME_ROWS|ROWFRAME_STRETCH_IN_COL|ROWFRAME_STRETCH_IN_ROW,
					xx,yy,ww,hh,brder, NULL,0,NULL,
					0)
{
	swin_style=(nstyle&~ANXWIN_MASK);
	search_type=SEARCH_TEXT; //0 is search text, 1 is search keys

	initialarea=newstr(place);
	sc=NULL;
	textheader=NULL;

	ShortcutTreeSelector *tree=new ShortcutTreeSelector(this, "tree","tree",SW_RIGHT, 0,0,0,0,0, NULL,this->object_id,"tree", 0,NULL);
	tree->AddColumn(_("Key"),NULL,-1);
	tree->AddColumn(_("Action"),NULL,-1);
	tree->AddColumn(_("Description"),NULL,-1);
	app->reparent(tree,this);
	tree->dec_count(); // *** why does uncommenting this make it crash on window close!?!?

	DBG cerr <<"in ShortcutWindow(), id="<<object_id<<endl;
}

ShortcutWindow::~ShortcutWindow()
{
	delete[] textheader;

	if (sc) sc->dec_count();
	if (initialarea) delete[] initialarea;

	DBG cerr <<"in ~ShortcutWindow(), id="<<object_id<<endl;
}


int ShortcutWindow::init()
{
	//[display type] [search]_____________________x [expandall][contractall]
	//[current set:]_____________[save set][load set]
	//
	//display types:
	//  as input tree
	//  area, sorted by key
	//  area, sorted by action
	//  area, sorted by default
	//  keyboard

	ShortcutManager *manager=GetDefaultShortcutManager();


	int textheight=text_height();

	anXWindow *last=NULL;

	int addspacer=0;

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
//								NULL);
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


	if (swin_style&SHORTCUTW_Load_Save) {
		//Name_______ [load][save][export]

		LineInput *linp;
		last=linp=new LineInput(this, "setname","set",0, 0,0,0,0,1, last,object_id,"setname",
								_("Key set name"),manager->setname);
		linp->tooltip(_("Name of the shortcut set"));
		AddWin(linp,1, 200,0,10000,50,0, textheight*1.25,0,0,50,0, -1);

		Button *but;
		last=but=new Button(this,"save","save",BUTTON_SAVE, 0,0,0,0,1, last,object_id,"save");
		but->tooltip(_("Save the shortcut set to a file"));
		AddWin(but,1, but->win_w,0,0,50,0, textheight*1.25,0,0,50,0, -1);

		last=but=new Button(this,"load","load",BUTTON_OPEN, 0,0,0,0,1, last,object_id,"load");
		but->tooltip(_("Load the shortcut set from a file"));
		AddWin(but,1, but->win_w,0,0,50,0, textheight*1.25,0,0,50,0, -1);

		AddNull();
		addspacer=1;
	}

	if (addspacer) { AddVSpacer(textheight/2,0,0,50); AddNull(); }

	
	 //define the shortcuts tree
	MenuInfo *menu=new MenuInfo;

	const char *place=initialarea;
	if (isblank(place)) place=NULL;

	for (int c=0; c<manager->shortcuts.n; c++) {
		if (place && !strcmp(place,manager->shortcuts.e[c]->area))
			menu->AddItem(manager->shortcuts.e[c]->area,0,MENU_OPEN,0,NULL,-1,1);
		else menu->AddItem(manager->shortcuts.e[c]->area);

		menu->SubMenu();
		AddAreaToMenu(menu->curmenu, manager->shortcuts.e[c]);
		menu->EndSubMenu();
	}

	ShortcutTreeSelector *tree=dynamic_cast<ShortcutTreeSelector*>(findChildWindowByName("tree"));
	tree->InstallMenu(menu);
	menu->dec_count();
	AddWin(tree,0, 200,0,10000,50,0, 200,0,10000,50,0, -1);


	Sync(1);

	return 0;
}

int ShortcutWindow::MouseMove (int x,int y,unsigned int state, const LaxMouse *m)
{
	return 1;
}

int ShortcutWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	return 1;
}

int ShortcutWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	return 1;
}

int ShortcutWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	return 1;
}

int ShortcutWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	return 1;
}

int ShortcutWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	return anXWindow::CharInput(ch,buffer,len,state,kb);
}

LaxFiles::Attribute *ShortcutWindow::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	att=anXWindow::dump_out_atts(att,what,context);
	TreeSelector *tree=dynamic_cast<TreeSelector*>(findChildWindowByName("tree"));

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

void ShortcutWindow::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	TreeSelector *tree=dynamic_cast<TreeSelector*>(findChildWindowByName("tree"));
	if (!tree) return;

	char *name,*value;
	int column=0;
	int i;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"column")) {
			IntAttribute(value,&i,NULL);
			if (column>=tree->columns.n) {
				tree->AddColumn(NULL,NULL, i,0, 0,-1);
				DBG cerr << "*** warning: adding unexpected column in ShortcutWindow!"<<endl;

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
void ShortcutWindow::UpdateSearch()
{
	LineInput *b=(LineInput*)findChildWindowByName("searchterm");
	if (!b) return;
	ShortcutTreeSelector *t=(ShortcutTreeSelector*)findChildWindowByName("tree");
	
	t->Menu()->ClearSearch();
	t->UpdateSearch(NULL, b->GetCText(), search_type);
}

/*! Set the search string to str AND set it in the edit box.
 * searchtype==-1 means use current. 0 is search text. 1 is search keys.
 */
int ShortcutWindow::SetSearch(const char *str, int searchtype)
{
	if (searchtype>=0) search_type=searchtype;

	LineInput *b=(LineInput*)findChildWindowByName("searchterm");
	if (b) b->SetText(str);

	UpdateSearch();
	needtodraw=1;

	return 0;
}

int ShortcutWindow::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"searchterm")) {
		// *** 
		LineInput *b=(LineInput*)findChildWindowByName("searchterm");
		if (!b) return 0;
		UpdateSearch();

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
		SetSearch(NULL);
		return 0;

	} else if (!strcmp(mes,"expandall")) {
		// ***

	} else if (!strcmp(mes,"collapseall")) {
		// ***

	} else if (!strcmp(mes,"save")) {
		ShortcutManager *manager=GetDefaultShortcutManager();
		FileDialog *f=new FileDialog(NULL,"save",_("Save shortcuts"),0, 0,0,500,500,0,object_id,"savethis",
									FILES_SAVE|FILES_ASK_TO_OVERWRITE,
									manager->setfile);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"savethis")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 1;
		if (isblank(s->str)) return 1;
		ShortcutManager *manager=GetDefaultShortcutManager();
		manager->Save(s->str, textheader);
		if (isblank(manager->setname)) {
			makestr(manager->setname,lax_basename(s->str));
			LineInput *i=dynamic_cast<LineInput*>(findChildWindowByName("setname"));
			i->SetText(manager->setname);
		}
		return 0;

	} else if (!strcmp(mes,"load")) {
		ShortcutManager *manager=GetDefaultShortcutManager();
		FileDialog *f=new FileDialog(NULL,"load",_("Load"),0, 0,0,500,500,0,object_id,"loadthis",FILES_OPEN_ONE,
									manager->setfile);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"loadthis")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		if (!s) return 1;
		if (isblank(s->str)) return 1;
		ShortcutManager *manager=GetDefaultShortcutManager();
		manager->Load(s->str); 
		//*** must merge with existing, currently deletes all old, OR reset actions, just load keys
	}

	return anXWindow::Event(e,mes);
}



} //namespace Laxkit


