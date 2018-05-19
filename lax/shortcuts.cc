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
//    Copyright (C) 2010-2012 by Tom Lechner
//

#include <lax/shortcuts.h>
#include <lax/strmanip.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>


#include <iostream>
using namespace std;
#define DBG


namespace Laxkit {

/*! \defgroup shortcuts Shortcuts
 *
 * When windows get key events, search a list of key mappings to get an associated action.
 *
 * A window would define a list of actions, and store a list of shortcuts pointing to those actions.
 * In the window's CharInput() function, the action number is found from its (redefinable) shortcuts
 * and something like thewindow->action(shortcut->action) is called.
 *
 * Shortcuts may be processed by ShortcutHandler and ShortcutManager objects.
 *
 *
 * 
 *
 * \todo The trick here is to have this accessible enough, that you can dump out a list of key
 *   commands at runtime. In the interest of saving space, maybe windows could just have pointer
 *   to a list, or ActionList() to return a list of available actions, that might have any key
 *   set to them, as well as ShortcutList() for actual bindings.
 * \todo perhaps key chaining can be done with a little popup up in the corner of the affected window?
 *   that would greatly relieve the particular window from having a special key chain mode.. it would
 *   just respond to a shortcut-command message..
 * \todo there could be a global ShortcutManager hash, where key is something like "ImageInterface"
 *   or "ViewwerWindow", and the value is a ShortcutHandler object. That way, shortcuts are synchronized
 *   across all interfaces of the same type
 */


int key_value_from_name(const char *k);
const char *key_name_from_value(int ch,char *buf);

//----------------------------------- KeyInfo ------------------------------------
/*! \class KeyInfo
 * \brief Info about a key, which can be part of a list in ShortCutDef
 */
/*! \var unsigned long KeyInfo::state
 * \brief the or'd state mask of for the shortcut.
 *
 * This can be any of ShiftMask, ControlMask, AltMask, or MetaMask.
 *
 * Note that for shortcuts, LockMask, Mod3Mask (caps lock), Mod2Mask (num lock), and Mod5Mask (scroll lock)
 * are not used.
 *
 */


//----------------------------------- ShortcutDef ------------------------------------
/*! \class ShortCutDef
 * \ingroup shortcuts
 * \brief Holds info about a keyboard shortcut.
 *
 * This is only info about specific key bindings. action is a kind of link to the actual action
 * that is performed. You may use WindowAction objects to keep various info about the
 * actually available actions that may be mapped to keys.
 */
/*! \var int ShortCutDef::mode
 * \brief The mode the window must be in for the shortcut to be active.
 *
 * The actual meaning of the value of mode is up to the particular anXWindow holding the shortcut,
 * but must be a positive number.
 */
/*! \var KeyInfo *ShortCutDef::keys
 * \brief A string of keys, perhaps a shortcut chain.
 */


/*! If key!=0, then define a single key shortcut.
 */
ShortcutDef::ShortcutDef(unsigned int key, unsigned int state, int m, int a)
	: keys(NULL), mode(m), action(a)
{
	 //else define a single key shortcut
	keys=new KeyInfo(key,state,0);
}

ShortcutDef::~ShortcutDef()
{
	if (keys) delete keys;
}

//! Return 0 for no match, or 1 or more for level the key and state matches.
/*! If keys is a sequence, scan down the sequence. 1 is first key in sequence, 2 is second, etc.
 */
int ShortcutDef::match(unsigned int key, unsigned int state)
{
	if (!keys) return 0;
	KeyInfo *k=keys;
	int l=1;
	while (k) {
		if (key==k->key && state==k->state) return l;
		l++;
		k=k->next;
	}
	return 0;
}


//----------------------------------- ShortcutDef ------------------------------------
/* \class ShortcutDefs
 * \brief Stack of ShortcutDef.
 */

ShortcutDefs::ShortcutDefs()
{
	name=description=NULL;
}

ShortcutDefs::~ShortcutDefs()
{
	if (name) delete[] name;
	if (description) delete[] description;
}

int ShortcutDefs::Add(unsigned int key, unsigned int state, int a, int m)
{
	 //sort on add:
	int c=0;
	for (c=0; c<n; c++) {
		if (e[c]->keys && key<=e[c]->keys->key) break;
	}
	return push(new ShortcutDef(key,state,m),1, c);
}

//! Return the shortcut with the given action.
/*! If startingfrom>=0, then start from that index, rather than 0.
 */
ShortcutDef *ShortcutDefs::FindShortcutFromAction(int action, int startingfrom)
{
	if (startingfrom<0 || startingfrom>=n) startingfrom=0;
	for (int c=startingfrom; c<n; c++) {
		if (e[c]->action==action) return e[c];
	}
	return NULL;
}

//----------------------------------- WindowAction ------------------------------------
/*! \class WindowAction
 * A window would have a list of possible actions, which does not depend on being
 * tied to particular keys.
 */
/*! \var char *WindowAction::customcode
 *   String that either is, or can be interpreted to be runnable code. These are optional
 *   "macros", or some other custom shortcuts that cannot be represented
 *   just with a name.
 */


WindowAction::WindowAction(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign)
{
	id=nid;
	name=newstr(nname);
	description=newstr(desc);
	iconname=newstr(icon);
	mode=nmode;
	assignable=assign;
	customcode=NULL;
}

WindowAction::~WindowAction()
{
	delete[] name;
	delete[] description;
	delete[] iconname;
	delete[] customcode;
}

//---------------------------------- WindowModeInfo ------------------------------------------
/*! \class WindowModeInfo
 * \brief Class to hold info about different modes a window can have. Used in WindowActions.
 */

WindowModeInfo::WindowModeInfo(int m, const char *modestr, const char *Name, const char *desc)
{
	mode=m;
	mode_str=newstr(modestr);
	name=newstr(Name);
	description=newstr(desc);
}

WindowModeInfo::~WindowModeInfo()
{
	if (mode_str) delete[] mode_str;
	if (name) delete[] name;
	if (description) delete[] description;
}

//----------------------------------- WindowActions ------------------------------------
/*! \class WindowActions
 * \brief RefCount derived stack for WindowAction objects.
 */
WindowActions::WindowActions()
{
	name=description=NULL;
	default_shortcuts=NULL;
}

WindowActions::~WindowActions()
{
	if (default_shortcuts) default_shortcuts->dec_count();
	if (name) delete[] name;
	if (description) delete[] description;
}

//! Return the WindowAction associated with action.
WindowAction *WindowActions::FindAction(int action)
{
	for (int c=0; c<n; c++) {
		if (action==e[c]->id) return e[c];
	}
	return NULL;
}

int WindowActions::Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign)
{
	 //sort by nname on add:
	int c=0;
	for (c=0; c<n; c++) {
		if (strcmp(nname,e[c]->name)<=0) break;
	}
	return push(new WindowAction(nid,nname,desc,icon,nmode,assign), 1, c);
}

int WindowActions::AddMode(int mode, const char *modestr, const char *name, const char *desc)
{
	modes.push(new WindowModeInfo(mode,modestr,name,desc));
	return 0;
}



//----------------------------------- ShortcutHandler ------------------------------------
/*! \class ShortcutHandler
 * \brief A kind of event filter windows can use to help process keyboard shortcuts.
 *
 * Windows and interfaces would have one ShortcutHandler object, that points to action and
 * shortcut stacks elsewhere. ShortcutHandler objects can keep info related to tracking shortcut
 * chain input.
 *
 * This class stores lists of actions and shortcut definitions, and can return an action
 * based on sequences of key presses.
 *
 * \todo maybe have name/file tags, to help loading and saving sets of shortcuts?
 *    for loading and saving, the actions stay the same, but key sets change. How
 *    to clarify what saves? At some point, would need access to mode names too.
 *    either that or have on ShortcutHandler per window mode? then shortcut manager
 *    keys could be "name-mode"
 */


ShortcutHandler::ShortcutHandler(const char *areaname,ShortcutDefs *cuts, WindowActions *wactions)
  :	index(-1),
	keyindex(0),
	deviceid(0)
{
	area=newstr(areaname);
	shortcuts=cuts;   if (cuts)    cuts->inc_count();
	actions=wactions; if (actions) actions->inc_count();
}

ShortcutHandler::~ShortcutHandler()
{
	if (area) delete[] area;
	if (actions) actions->dec_count();
	if (shortcuts) shortcuts->dec_count();
}

//! Return a new ShortcutHandler object. The action and shortcut lists are refcounted, not duplicated.
ShortcutHandler *ShortcutHandler::duplicate()
{
	ShortcutHandler *h=new ShortcutHandler;
	h->shortcuts=shortcuts;
	if (shortcuts) shortcuts->inc_count();
	h->actions=actions;
	if (actions) actions->inc_count();
	makestr(h->area,area);
	return h;
}


//----------action stack functions:

int ShortcutHandler::NumActions()
{
	if (actions) return actions->n;
	return 0;
}

//! Return the WindowAction object at index i.
WindowAction *ShortcutHandler::Action(int i)
{
	if (!actions) return NULL;
	if (i<0 || i>=actions->n) return NULL;
	return actions->e[i];
}

WindowActions *ShortcutHandler::Actions()
{ return actions; }

//! Return the WindowAction object corresponding to a matching key, state, and mode if mode>=0.
WindowAction *ShortcutHandler::FindAction(unsigned int key, unsigned int state, int mode)
{
	if (!shortcuts || !actions) return NULL;

	int i=FindShortcutIndex(key,state,mode);
	if (i<0) return NULL;
	if (mode>0 && shortcuts->e[i]->mode!=mode) return NULL;
	for (int c=0; c<actions->n; c++) {
		if (shortcuts->e[i]->action==actions->e[c]->id) return actions->e[c];
	}
	return NULL;
}

//! From the actions stack, return the action number corresponding to actionname.
/*! -1 is returned if not found.
 *
 * If len<0 then use strlen(actionname). Otherwise, use len number of characters of actionname.
 */
int ShortcutHandler::FindActionNumber(const char *actionname,int len)
{
	if (!actions) return -1;
	if (len<0) len=strlen(actionname);
	for (int c=0; c<actions->n; c++) {
		if (len==(int)strlen(actions->e[c]->name) && !strncmp(actionname,actions->e[c]->name,len)) return actions->e[c]->id;
	}
	return -1;
}


//----------shortcut stack functions:

int ShortcutHandler::NumShortcuts()
{
	if (shortcuts) return shortcuts->n;
	return 0;
}

//! Return the ShortcutDef object at index i.
ShortcutDef *ShortcutHandler::Shortcut(int i)
{
	if (i<0 || i>=shortcuts->n) return NULL;
	return shortcuts->e[i];
}

ShortcutDefs *ShortcutHandler::Shortcuts()
{ return shortcuts; }

int ShortcutHandler::ClearShortcut(unsigned int key, unsigned int state, int mode)
{ return ClearShortcut(FindShortcutIndex(key,state,mode)); }

/*! Return 0 for success, nonzero for error.
 */
int ShortcutHandler::ClearShortcut(int index)
{
	if (!shortcuts) return 1;
	if (index<0 || index>=shortcuts->n) return 2;
	shortcuts->remove(index);
	return 0;
}

//! Return action number listed in shortcuts stack. Does not read from actions stack.
/*! If mode>0, then the key must have that mode.
 * Return -1 for not found, else the action number.
 */
int ShortcutHandler::FindActionNumber(unsigned int key, unsigned int state, int mode)
{
	int i=FindShortcutIndex(key,state,mode);
	if (i<0) return -1;
	if (shortcuts->e[i]->mode > 0 && shortcuts->e[i]->mode != mode) return -1;
	return shortcuts->e[i]->action;
}

//! Return the index of a shortcut (in shortcuts stack) matching key, state, and mode if mode>=0.
int ShortcutHandler::FindShortcutIndex(unsigned int key, unsigned int state, int mode)
{
	if (!shortcuts) return -1;
	for (int c=0; c<shortcuts->n; c++) {
		if (shortcuts->e[c]->match(key,state)>0
				&& ((shortcuts->e[c]->mode>0 && mode==shortcuts->e[c]->mode) || shortcuts->e[c]->mode<=0)
			) return c;
	}
	return -1;
}

int ShortcutHandler::FindShortcutFromAction(int action, int startingfrom)
{
	if (!shortcuts) return -1;
	ShortcutDef *def=shortcuts->FindShortcutFromAction(action,startingfrom);
	return shortcuts->findindex(def);
}

int ShortcutHandler::InstallShortcuts(ShortcutDefs *cuts)
{
	if (shortcuts) shortcuts->dec_count();
	shortcuts=cuts;
	if (shortcuts) shortcuts->inc_count();
	return 0;
}


//-----------------other misc

int ShortcutHandler::AddMode(int mode, const char *modestr, const char *name, const char *desc)
{
	if (!actions) actions=new WindowActions();
	actions->modes.push(new WindowModeInfo(mode,modestr,name,desc));
	return 0;
}

//! Add an action with a default shortcut.
unsigned int ShortcutHandler::Add(unsigned int action, unsigned int key, unsigned int state, unsigned int mode,
							const char *nname, const char *desc, const char *icon, int assign)
{
	AddAction(action, nname,desc,icon,mode,assign);
	AddShortcut(key,state,mode,action);
	return 0;
}

//! Add a window action.
unsigned int ShortcutHandler::AddAction(unsigned int action, const char *nname, const char *desc, const char *icon, unsigned int mode, int assign)
{
	if (!actions) actions=new WindowActions();
	actions->push(new WindowAction(action, nname,desc,icon,mode,assign));
	return 0;
}

//! Remove action from actions stack, and remove any shortcuts bound to that action number.
int ShortcutHandler::RemoveAction(int action)
{
	 //remove from actions stack
	if (actions) {
		for (int c=0; c<actions->n; c++) {
			if (actions->e[c]->id==action) {
				actions->remove(c);
				break;
			}
		}
	}

	 //remove any keys bound to the action
	if (shortcuts) {
		for (int c=0; c<shortcuts->n; c++) {
			if (shortcuts->e[c]->action==action) {
				shortcuts->remove(c);
				c--;
				continue;
			}
		}
	}

	return 0;
}

//! Add a shortcut. Assumes action is in actions, does not check. Return index of shortcut.
/*! If shortcut key+state+mode existed already, then assign the action to that one.
 */
int ShortcutHandler::AddShortcut(unsigned int key, unsigned int state, unsigned int mode, unsigned int action)
{
	if (!shortcuts) shortcuts=new ShortcutDefs();

	 //if existing shortcut for key,state,mode, remap to action
	int i=FindShortcutIndex(key,state,mode);
	if (i>=0) shortcuts->e[i]->action=action;
	else {
		shortcuts->push(new ShortcutDef(key,state,mode, action));
		i=shortcuts->n-1;
	}
	return i;
}


//! Update which action is triggered by the specified key.
/*! If key didn't exist, then install new shortcut.
 *
 * Return 0 for sucess, or nonzero for error.
 */
int ShortcutHandler::UpdateKey(unsigned int key, unsigned int state, unsigned int mode, unsigned int newaction)
{
	int keyi=FindShortcutIndex(key, state, mode);

	if (keyi<0) { //key not found
		AddShortcut(key,state,mode,newaction);
	} else {
		shortcuts->e[keyi]->action=newaction;
	}

	return 0;
}

/*! When you overwrite a key in a ShortcutWindow, you need to syncronize
 * which keys are mapped to which actions. This will try to preserve the order
 * of the shortcuts.
 *
 * Return -1 for old equals new, so nothing done.
 */
int ShortcutHandler::ReassignKey(unsigned int newkey, unsigned int newstate,
								 unsigned int oldkey, unsigned int oldstate,
								 unsigned int mode, unsigned int action)
{
	if (newkey==oldkey && newstate==oldstate) return -1;

	int oldkeyi=FindShortcutIndex(oldkey, oldstate, mode);
	if (oldkeyi>=0) {
		 //remove old association
		shortcuts->remove(oldkeyi);
	}

	int newkeyi=FindShortcutIndex(newkey, newstate, mode);
	if (newkeyi>=0 && shortcuts->e[newkeyi]->action==(int)action) return 0; //new association already existed!
	if (newkeyi>=0) { shortcuts->e[newkeyi]->action=action; return 0; }
	
	if (newkey==0) return 1; //don't add null keys!

	newkeyi=AddShortcut(newkey,newstate,mode,action);
//	if (oldkeyi>=0 && newkeyi!=oldkeyi) {
//		 //reposition
//		int local;
//		ShortcutDef *def=shortcuts->pop(newkeyi,&local);
//		shortcuts->push(def,local,oldkeyi);
//	}

	return 0;
}


//--------------------------------- ShortcutManager ------------------------------------
/*! \class ShortcutManager
 * 
 * Class to store collections of shortcuts and actions in a sort of input tree.
 * This is basically a fancy stack of ShortcutHandler objects. Sometimes, there will be
 * WindowAction things defined, but no bound shortcuts, or shortcuts that only are
 * bound to numbers.
 */

ShortcutManager::ShortcutManager()
{
	settitle=newstr("Shortcuts");
	setname=NULL;
	setfile=NULL;
}

ShortcutManager::~ShortcutManager()
{
	delete[] settitle;
	delete[] setname;
	delete[] setfile;
}


/*! Counts of cuts and actions incremented.
 */
int ShortcutManager::AddArea(const char *area, ShortcutDefs *cuts, WindowActions *actions)
{
	ShortcutHandler *h=new ShortcutHandler(area,cuts,actions);
	shortcuts.push(h);
	h->dec_count();
	
	return 0;
}

int ShortcutManager::AddArea(const char *area, ShortcutHandler *handler)
{
	ShortcutHandler *h=handler->duplicate();
	makestr(h->area,area);
	shortcuts.push(h);
	h->dec_count();
	
	return 0;
}

int ShortcutManager::AreaParent(const char *area, const char *parent)
{
	cerr << " *** need to adequately implement AreaParent()!"<<endl;

	if (!parent) {
		 //simple case, just add area, no parent
		if (tree.find(area)) return 0;
		tree.push(area);
		return 0;
	}
	LaxFiles::Attribute *att=tree.find(parent);
	if (att) {
		 //add area to existing parent
		if (att->find(area)) return 0;
		att->push(area);
	} else {
		 //add parent and area
		att=att->pushSubAtt(parent);
		att->flags=1;
		att->push(area);
	}
	return 0;
}

//! Return a new ShortcutHandler object. The action and shortcut lists are refcounted, not duplicated.
ShortcutHandler *ShortcutManager::NewHandler(const char *area)
{
	for (int c=0; c<shortcuts.n; c++) {
		if (!strcmp(shortcuts.e[c]->area, area)) return shortcuts.e[c]->duplicate();
	}

	return NULL;
}

//! Load from file if possible. If file==NULL, use setfile.
/*! Set setfile to this file, if the load succeeds, else don't change setfile.
 *
 *  Return 0 for success, or nonzero for save fail.
 */
int ShortcutManager::Load(const char *file)
{
	if (!file) file=setfile;
	if (!file) return 1;
	FILE *f=fopen(file,"r");
	if (!f) return 2;

	makestr(setname,NULL);
	dump_in(f,0,0,NULL,NULL);

	fclose(f);
	if (file!=setfile) makestr(setfile,file);
	if (isblank(setname)) makestr(setname, lax_basename(setfile));
	return 0;
}

/*! If file==NULL, use setfile.
 */
int ShortcutManager::LoadKeysOnly(const char *file)
{
	cerr <<" *** need to implement int ShortcutManager::LoadKeysOnly(const char *file)"<<endl;
	return 0;
}

//! Save to file, or to setfile if file==NULL.
/*! Return 0 for success, or nonzero for save fail.
 * If header is nonzero then first output that string, a newline, then dump_out().
 */
int ShortcutManager::Save(const char *file, const char *header)
{
	if (!file) file=setfile;
	if (!file) return 1;

	FILE *f=fopen(file,"w");
	if (!f) return 2;

	if (header) fprintf(f,"%s\n",header);
	dump_out(f,0,0,NULL);

	fclose(f);
	if (file!=setfile) makestr(setfile,file);
	return 0;
}

/*! Convert "'&<> to &quot;&apos;&amp;&lt;&gt;
 *
 * \todo PUT THIS SOMEWHERE BETTER
 */
char *XMLCharsToEntities(const char *str, char *&nstr, int &bufferlen)
{
	const char *p=str;
	int n=0;
	do {
		p=strpbrk(p,"\"'&<>");
		if (p!=NULL) { n++; p++; }
	} while (p && *p);

	int len=strlen(str) + 6*n + 1;
	if (len>bufferlen) {
		delete[] nstr;
		nstr=new char[len];
		bufferlen=len;
	}

	int i=0;
	p=str;
	while (*p) {
		if (*p=='\'')     { nstr[i++]='&'; nstr[i++]='a'; nstr[i++]='p'; nstr[i++]='o'; nstr[i++]='s'; nstr[i++]=';'; }
		else if (*p=='"') { nstr[i++]='&'; nstr[i++]='q'; nstr[i++]='u'; nstr[i++]='o'; nstr[i++]='t'; nstr[i++]=';'; }
		else if (*p=='&') { nstr[i++]='&'; nstr[i++]='a'; nstr[i++]='m'; nstr[i++]='p'; nstr[i++]=';'; }
		else if (*p=='<') { nstr[i++]='&'; nstr[i++]='l'; nstr[i++]='t'; nstr[i++]=';'; }
		else if (*p=='>') { nstr[i++]='&'; nstr[i++]='g'; nstr[i++]='t';nstr[i++]=';'; }
		else nstr[i++]=*p;
		p++;
	}
	nstr[i]='\0';

	return nstr;
}

int ShortcutManager::SaveHTML(const char *file)
{
	if (!file) file="-";

	FILE *f;
	if (!strcmp(file,"-")) f=stdout;
	else f=fopen(file,"w");
	if (!f) return 2;

	char *buffer2=NULL;
	int bufferlen=0;

	if (!isblank(setname)) fprintf(f,"%s<br/><br/>\n\n",XMLCharsToEntities(setname,buffer2,bufferlen));

	//const char *shift  ="+";
	//const char *control="^";
	//const char *alt    ="&amp;";
	//const char *meta   ="~";

	fprintf(f,"<!DOCTYPE html>\n"
				"<html>\n"
				"<head>\n"
				"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
				"<meta charset=\"utf-8\">\n"
				"<title>%s</title>\n"
				"\n"
				"<style type=\"text/css\">\n"
				"body { background-color: #eee; color: #000; }\n"
				".cntlkey, .metakey, .altkey, .shiftkey {\n"
				"	border: 1px solid #bbb;\n"
				"	background-color: #ddd;\n"
				"	color: #444;\n"
				"	padding: .25em;\n"
				"	margin: .1em;\n"
				"	border-radius: .4em;\n"
				"	-moz-border-radius: .4em;\n"
				"	font-family: sans-serif, courier;\n"
				"	font-size: 85%%;\n"
				"	display: inline-block;\n"
				"   box-shadow: 0px .1em .15em #555;\n"
				"}\n"
				"\n"
				".cntlkey::before  { content: \"control\"; }\n"
				".altkey::before   { content: \"alt\"; }\n"
				".metakey::before  { content: \"meta\"; }\n"
				".shiftkey::before { content: \"shift\"; }\n"
				"\n"
				".keystyle {\n"
				"   border: 1px solid #bbb;\n"
				"   padding: .25em .25em .15em .25em;\n"
				"   margin: .1em;\n"
				"   border-radius: .4em;\n"
				"   display: inline-block;\n"
				"   box-shadow: 0px .1em .2em #555;\n"
				"}\n"
				"\n"
				"table.formattable {\n"
				"	border-width: 1px 1px 1px 1px;\n"
				"	border-spacing: 0px;\n"
				"	border-style: outset outset outset outset;\n"
				"	border-color: #888;\n"
				"	border-collapse: collapse;\n"
				"}\n"
				"table.formattable th {\n"
				"	border-width: 1px 1px 1px 1px;\n"
				"	padding: 1px 1px 1px 1px;\n"
				"	border-style: inset inset inset inset;\n"
				"	border-color: #ccc;\n"
				"	-moz-border-radius: 0px;\n"
				"}\n"
				"table.formattable td {\n"
				"	border-width: 1px 1px 1px 1px;\n"
				"	border-style: inset inset inset inset;\n"
				"	border-color: #ccc;\n"
				"	padding: .2em .5em .2em .5em;\n"
				"	-moz-border-radius: 0px;\n"
				"}\n"
				"td.area { background-color: #cccccc; }\n"
				"td.key { background-color: #ffffff; text-align: right; }\n"
				"td.id { background-color: #dddddd; }\n"
				"td.description { background-color: #ffffff; }\n"
				"</style>\n"
				"\n"
				"</head>\n"
				"<body>\n\n"
				"<h1>%s</h1>\n",
				settitle?XMLCharsToEntities(settitle,buffer2,bufferlen):"Shortcuts",
				settitle?XMLCharsToEntities(settitle,buffer2,bufferlen):"Shortcuts");
	
	 //output available modifier aliases
	//fprintf(f,"Modifiers<br/>\n");
	//fprintf(f,"<table>\n");
	//fprintf(f,"  <tr><td>Shift   </td><td>%s</td></tr>\n",shift);
	//fprintf(f,"  <tr><td>Control </td><td>%s</td></tr>\n",control);
	//fprintf(f,"  <tr><td>Alt     </td><td>%s</td></tr>\n",alt);
	//fprintf(f,"  <tr><td>Meta    </td><td>%s</td></tr>\n",meta);
	//fprintf(f,"</table>\n");

	 //output the shortcuts
	ShortcutDefs *s;
	WindowActions *a;
	WindowAction *aa;
	char buffer[100];


	fprintf(f,"<table class=\"formattable\">\n");
	for (int c=0; c<shortcuts.n; c++) {
		fprintf(f,"<tr><td colspan=\"3\" class=\"area\">%s</td></tr>\n",shortcuts.e[c]->area);
		s = shortcuts.e[c]->Shortcuts();
		a = shortcuts.e[c]->Actions();

		 //output all bound keys
		if (s) {
			for (int c2=0; c2<s->n; c2++) {
				fputs("  <tr>", f);
				if (a) aa=a->FindAction(s->e[c2]->action); else aa=NULL;

				 //key
				fputs("<td class=\"key\">", f);
				if (s->e[c2]->keys->state&ShiftMask)   fputs("<span class=\"shiftkey\"></span>",f);
				if (s->e[c2]->keys->state&ControlMask) fputs("<span class=\"cntlkey\"></span>" ,f);
				if (s->e[c2]->keys->state&AltMask)     fputs("<span class=\"altkey\"></span>"  ,f); 
				if (s->e[c2]->keys->state&MetaMask)    fputs("<span class=\"metakey\"></span>" ,f);

				key_name_from_value(s->e[c2]->keys->key, buffer);
				fprintf(f, "<span class=\"keystyle\">%s</span></td>", buffer);

				 //description
				if (aa) {
					fputs("<td class=\"description\">", f);
					if (!isblank(aa->description)) fprintf(f,"%s",XMLCharsToEntities(aa->description, buffer2, bufferlen));
					fprintf(f,"</td>");
				} else fputs("<td class=\"description\"></td>\n", f);

				 //name
				if (aa) {
					 //print out string id
					fprintf(f,"<td class=\"id\">%s</td>", XMLCharsToEntities(aa->name, buffer2, bufferlen));
				} else fprintf(f,"<td class=\"id\">%d</td>",s->e[c2]->action); //print out number only

				fputs("</tr>\n", f);
			}
		}

		 //output any unbound actions
		if (a) {
			int c2=0;
			for (c2=0; c2<a->n; c2++) {
				if (s && s->FindShortcutFromAction(a->e[c2]->id,0)) continue; //action is bound!

				fputs("  <tr>", f);

				//key
				fputs("<td class=\"key\">none</td>",f);
				
				 //description
				fputs("<td class=\"description\">", f);
				if (!isblank(a->e[c2]->description)) {
					fprintf(f,"%s", XMLCharsToEntities(a->e[c2]->description, buffer2, bufferlen));
				} else fputs("</td>", f);

				 //action
				fprintf(f, "<td class=\"id\">%s</td>", XMLCharsToEntities(a->e[c2]->name, buffer2, bufferlen));


				fputs("</tr>\n",f);
			}
		}
	}
	fprintf(f,"</table>\n");
	fprintf(f,"</body>\n");

	if (f!=stdout) fclose(f);

	delete[] buffer2;
	return 0;
}

/*!
 * \verbatim
 * modifiers
 *    +^a,c,h,a,i,n   action 5               
 *    shift-alt-'c',h,a,i,n
 *    '+',''',','  someotheraction    # <-- a chain of '+' then apostrophe, then comma
 * \endverbatim
 *
 * \todo custom modifiers?
 */
void ShortcutManager::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sname \"Name of shortcut set\"\n",spc);
		fprintf(f,"%smodifiers          #Modifier aliases, note that this is only a hint, it is currently ignored on read in\n",spc);
		fprintf(f,"%s  Shift   +\n",spc);
		fprintf(f,"%s  Control ^\n",spc);
		fprintf(f,"%s  Alt     &\n",spc);
		fprintf(f,"%s  Meta    ~\n",spc);
		fprintf(f,"%sarea SomeInputArea  #Shortcuts are defined per area, and areas can be stacked on each other\n",spc);
		fprintf(f,"%s  1   SomeAction     #a shortcut for the '1' key\n",spc);
		fprintf(f,"%s  +A  SomeAction      #a shortcut for the shift-'a' key\n",spc);
		fprintf(f,"%s  ^+^ SomeAction       #a shortcut for the control-shift-'^' key\n",spc);
		fprintf(f,"%s  alt-shift-F1 action   #a shortcut for the alt-shift-f1 key\n",spc);
		fprintf(f,"%s  space  SomeAction         #special indicator for the space key\n",spc);
		fprintf(f,"%s  tab    SomeAction         #special indicator for the tab key\n",spc);
		fprintf(f,"%s  enter  SomeAction         #special indicator for the enter key\n",spc);
		fprintf(f,"%s  none SomeUnboundAction  #an unbound window action, output for reference\n",spc);
		fprintf(f,"%sinputtree  #If defined, output a hierarchy for how input areas are available\n",spc);
		fprintf(f,"%s  area1    #Different areas might be accessible from multiple other areas.\n",spc);
		fprintf(f,"%s    area3\n",spc);
		fprintf(f,"%s    area4\n",spc);
		fprintf(f,"%s  area2\n",spc);
		fprintf(f,"%s    area3\n",spc);
		return; 
	}


	if (!isblank(setname)) fprintf(f,"%sname %s\n",spc,setname);

	char shift  ='+';
	char control='^';
	char alt    ='&';
	char meta   ='~';

	 //output available modifier aliases
	fprintf(f,"%smodifiers #warning: for now modifiers block is a hint only! will not affect parsing!\n",spc);
	fprintf(f,"%s  Shift   %c\n",spc,shift);
	fprintf(f,"%s  Control %c\n",spc,control);
	fprintf(f,"%s  Alt     %c\n",spc,alt);
	fprintf(f,"%s  Meta    %c\n",spc,meta);

	 //output the shortcuts
	ShortcutDefs *s;
	WindowActions *a;
	WindowAction *aa;
	char buffer[100];
	for (int c=0; c<shortcuts.n; c++) {
		fprintf(f,"%sarea %s\n",spc,shortcuts.e[c]->area);
		s=shortcuts.e[c]->Shortcuts();
		a=shortcuts.e[c]->Actions();

		 //output all bound keys
		if (s) {
			for (int c2=0; c2<s->n; c2++) {
				fprintf(f,"%s  %-10s ",spc,ShortcutString(s->e[c2], buffer));
				if (a) aa=a->FindAction(s->e[c2]->action); else aa=NULL;
				if (aa) {
					 //print out string id and commented out description
					fprintf(f,"%-20s",aa->name);
					if (!isblank(aa->description)) fprintf(f,"#%s",aa->description);
					fprintf(f,"\n");
				} else fprintf(f,"%d\n",s->e[c2]->action); //print out number only
			}
		}

		 //output any unbound actions
		if (a) {
			int c2=0;
			for (c2=0; c2<a->n; c2++) {
				if (s && s->FindShortcutFromAction(a->e[c2]->id,0)) continue; //action is bound!
				fprintf(f,"%s  none       %-20s",spc,a->e[c2]->name);
				if (!isblank(a->e[c2]->description)) fprintf(f,"#%s",a->e[c2]->description);
				fprintf(f,"\n");
			}
		}
	}

	 //output input tree if available
	if (tree.attributes.n) {
		fprintf(f,"%sinputtree\n",spc);
		tree.dump_out(f,indent+2);
	}
}

//! Just call the other ShortcutString() with key and state.
char *ShortcutManager::ShortcutString(ShortcutDef *def, char *buffer)
{
	return ShortcutString(def->keys->key,def->keys->state,buffer);
}

/*! It would be good to have buffer at least 20 bytes long.
 * buffer should be null terminated. The key string is strcat'd to it.
 *
 * See KeyAndState() for the reverse of this.
 */
char *ShortcutManager::ShortcutString(unsigned int key, unsigned long state, char *buffer)
{
	int l=0;
	*buffer=0;

	 //add mods
	if (state&ShiftMask)   { strcat(buffer,"+"); l++; }
	if (state&ControlMask) { strcat(buffer,"^"); l++; }
	if (state&AltMask)     { strcat(buffer,"&"); l++; }
	if (state&MetaMask)    { strcat(buffer,"~"); l++; }

	key_name_from_value(key,buffer+l);

	return buffer;
}

//! Return the installed handler for the given area.
/*! If you are wanting to install a handler object in a window or interface, use
 * NewHandler() instead, and that make a duplicate. The object returned here is
 * merely for informational purposes.
 */
ShortcutHandler *ShortcutManager::FindHandler(const char *area)
{
	for (int c=0; c<shortcuts.n; c++) {
		if (!strcmp(area,shortcuts.e[c]->area)) return shortcuts.e[c];
	}
	return NULL;
}

/*! If no handler is found for any particular area, then a new one is created.
 * 
 * If one IS found, then any old shortcut bindings are flushed in favor of the new ones.
 * Care must be taken to ensure that there are WindowActions defined for the area BEFORE
 * calling this! Otherwise, the non-number actions defined for the key will not map to anything meaningful.
 */
void ShortcutManager::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext)
{
    if (!att) return;
    char *name,*value,*kstr;
	unsigned int key,state;
	int actionnum;
	ShortcutHandler *handler;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

		if (!strcmp(name,"modifiers")) {
			//skip for now

		} else if (!strcmp(name,"name")) {
			if (isblank(value)) continue;
			makestr(setname,value);

		} else if (!strcmp(name,"area")) {
			if (isblank(value)) continue; //ruthlessly ignore unlabeled areas

			handler=FindHandler(value);
			if (!handler) {
				handler=new ShortcutHandler(value);
				shortcuts.push(handler);
				handler->dec_count();
			}
			handler->InstallShortcuts(new ShortcutDefs); //warning! totaly removes old keys

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				kstr= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (KeyAndState(kstr, &key,&state)==0) continue;
				actionnum=handler->FindActionNumber(value);
				if (actionnum<0) {
					char *after;
					actionnum=strtol(value,&after,10);
					if (*after) continue; //was not a valid number, and action name not found
				}
				handler->AddShortcut(key,state,0, actionnum);
			}

		} else if (!strcmp(name,"inputtree")) {
		}
	}
}

/*! Return 1 for success or 0 for fail. This is the reverse of ShortcutString().
 */
int ShortcutManager::KeyAndState(const char *str, unsigned int *key,unsigned int *state)
{
	int mods=0, k;
	while (str && *str) {
		if (!strncasecmp(str,"shift-",6)) { mods|=ShiftMask; str+=6; continue; }
		if (!strncasecmp(str,"alt-",4)) { mods|=AltMask; str+=4; continue; }
		if (!strncasecmp(str,"Control-",8)) { mods|=ControlMask; str+=8; continue; }
		if (!strncasecmp(str,"meta-",5)) { mods|=MetaMask; str+=5; continue; }
		if (str[1]!='\0') {
			if (str[0]=='+') { mods|=ShiftMask;   str++; continue; }
			if (str[0]=='&') { mods|=AltMask;     str++; continue; }
			if (str[0]=='^') { mods|=ControlMask; str++; continue; }
			if (str[0]=='~') { mods|=MetaMask;    str++; continue; }
		}
		break;
	}
	k=key_value_from_name(str);
	*key=k;
	*state=mods;
	return 1;
}

/*! If replace!=0, then replace the action of the key and mode in area with the new action.
 * Otherwise add it as an alternative. Return 0 for sucess, or nonzero for error, could not process.
 */
int ShortcutManager::UpdateKey(const char *area, unsigned int key, unsigned int state, unsigned int mode, unsigned int newaction)
{
	if (!area) return 1;

	ShortcutHandler *areah=FindHandler(area);
	if (!areah) return 2;

	return areah->UpdateKey(key,state,mode,newaction);
}

int ShortcutManager::ReassignKey(const char *area, unsigned int newkey, unsigned int newstate,
								  unsigned int oldkey, unsigned int oldstate,
								  unsigned int mode, unsigned int action)
{
	if (!area) return 1; 
	ShortcutHandler *areah=FindHandler(area);
	if (!areah) return 2; 
	return areah->ReassignKey(newkey,newstate,oldkey,oldstate,mode, action);
}

/*! Flush all key shortcuts, but keep all actions.
 * This is used, for instance, with LoadKeysOnly(), which matches all keys to existing actions,
 * but does not load actions. You would want to do this to ensure your application's
 * current actions still exist after loading in a new key set.
 */
int ShortcutManager::ClearKeys()
{
	ShortcutDefs *s;
	for (int c=0; c<shortcuts.n; c++) {
		s=shortcuts.e[c]->Shortcuts();
		s->flush();
	}
	return 0;
}


//--------------------------- ShortcutManager singleton helper functions ---------------------------

/*! Retrieve with GetDefaultShortcutManager() and set with InstallShortCutManager().
 */
static ShortcutManager *default_shortcutmanager=NULL;

/*! If manager==NULL, then remove the old default. This will result in
 * NO manager being defined. To reallocate one, simply call GetDefaultShortcutManager(),
 * and a new one will be generated. You might want to pass in NULL at the end of a program,
 * when memory is all being freed.
 *
 * Takes posession of the manager count, meaning if it had a count of 1, it will still
 * have a count of 1, and it will be dec_count()'d when removed.
 */
void InstallShortcutManager(ShortcutManager *manager)
{
	if (default_shortcutmanager) default_shortcutmanager->dec_count();
	default_shortcutmanager=manager;
	if (manager) manager->inc_count();
}

//! Return a default shortcut manager.
/*! Defines a default manager if none exists.
 */
ShortcutManager *GetDefaultShortcutManager()
{
	if (default_shortcutmanager==NULL) { default_shortcutmanager=new ShortcutManager; } 
	return default_shortcutmanager;
}

//! Dec count on the default shortcut manager, and set to null.
void FinalizeShortcutManager()
{
	if (default_shortcutmanager) default_shortcutmanager->dec_count();
	default_shortcutmanager=NULL;
}

//-------------------misc helpers, probably should be in laxutils perhaps?

//! Return a string identifier for non-printing keys known to the Laxkit.
/*! Returns a string like "Esc", "Bksp", or "F1".
 */
const char *lax_nonprinting_key_name(int ch)
{
	switch (ch) {
		//case LAX_Shift: return "Shift"; 
		//case LAX_Control return "Control";
		//case LAX_Alt: return "Alt"; 
		//case LAX_Meta: return "Meta"; 

		case LAX_Esc: return "Esc"; 
		case LAX_Menu: return "Menu"; 
		case LAX_Pause: return "Pause"; 
		case LAX_Del: return "Del"; 
		case LAX_Bksp: return "Bksp"; 
		case LAX_Tab: return "Tab"; 
		case LAX_Ins: return "Ins"; 
		case LAX_Home: return "Home"; 
		case LAX_End: return "End"; 
		case LAX_Enter: return "Enter"; 
		case LAX_Pgup: return "PageUp"; 
		case LAX_Pgdown: return "PageDown"; 
		case LAX_F1: return "F1"; 
		case LAX_F2: return "F2"; 
		case LAX_F3: return "F3"; 
		case LAX_F4: return "F4"; 
		case LAX_F5: return "F5"; 
		case LAX_F6: return "F6"; 
		case LAX_F7: return "F7"; 
		case LAX_F8: return "F8"; 
		case LAX_F9: return "F9"; 
		case LAX_F10: return "F10"; 
		case LAX_F11: return "F11"; 
		case LAX_F12: return "F12"; 
		case LAX_Left: return "Left"; 
		case LAX_Up: return "Up"; 
		case LAX_Down: return "Down"; 
		case LAX_Right: return "Right"; 
	}
	return NULL;
}
//! Return a string name (printed into buf) for a non-modifier key.
/*! If the value is not translatable, return NULL, and nothing is written to buf.
 * Buf should be 10 bytes or longer.
 */
const char *key_name_from_value(int ch,char *buf)
{
	const char *k=lax_nonprinting_key_name(ch);
	if (k) { strcpy(buf,k); return buf; }

	if (ch=='#') { sprintf(buf,"\\#"); return buf; }
	if (ch<128 && ch>32) { buf[0]=ch; buf[1]='\0'; return buf; }
	if (ch==32) sprintf(buf,"space");
	else sprintf(buf,"u%04x",ch);
	return buf;
}

int key_value_from_name(const char *k)
{
	if (*k==0) return 0;
	if (!strcasecmp(k, "space")) return ' '; 
	if (!strcasecmp(k, "Esc")) return LAX_Esc; 
	if (!strcasecmp(k, "Menu")) return LAX_Menu; 
	if (!strcasecmp(k, "Pause")) return LAX_Pause; 
	if (!strcasecmp(k, "Del")) return LAX_Del; 
	if (!strcasecmp(k, "Bksp")) return LAX_Bksp; 
	if (!strcasecmp(k, "Tab")) return LAX_Tab; 
	if (!strcasecmp(k, "Ins")) return LAX_Ins; 
	if (!strcasecmp(k, "Home")) return LAX_Home; 
	if (!strcasecmp(k, "End")) return LAX_End; 
	if (!strcasecmp(k, "Enter")) return LAX_Enter; 
	if (!strcasecmp(k, "PageUp")) return LAX_Pgup; 
	if (!strcasecmp(k, "PageDown")) return LAX_Pgdown; 
	if (!strcasecmp(k, "F1")) return LAX_F1; 
	if (!strcasecmp(k, "F2")) return LAX_F2; 
	if (!strcasecmp(k, "F3")) return LAX_F3; 
	if (!strcasecmp(k, "F4")) return LAX_F4; 
	if (!strcasecmp(k, "F5")) return LAX_F5; 
	if (!strcasecmp(k, "F6")) return LAX_F6; 
	if (!strcasecmp(k, "F7")) return LAX_F7; 
	if (!strcasecmp(k, "F8")) return LAX_F8; 
	if (!strcasecmp(k, "F9")) return LAX_F9; 
	if (!strcasecmp(k, "F10")) return LAX_F10; 
	if (!strcasecmp(k, "F11")) return LAX_F11; 
	if (!strcasecmp(k, "F12")) return LAX_F12; 
	if (!strcasecmp(k, "Left")) return LAX_Left; 
	if (!strcasecmp(k, "Up")) return LAX_Up; 
	if (!strcasecmp(k, "Down")) return LAX_Down; 
	if (!strcasecmp(k, "Right")) return LAX_Right; 
	if (!strcasecmp(k, "none")) return 0;

	if (k[1]=='\0' && k[0]>=32 && k[0]<128) return (int)k[0]; //ascii printable
	if (k[0]!='u' && k[0]!='U') return 0;
	return strtol(k+1,NULL,16);
}



} //namespace Laxkit


