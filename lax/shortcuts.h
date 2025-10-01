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
#ifndef _LAX_SHORTCUT_H
#define _LAX_SHORTCUT_H


#include <cstdlib>

#include <lax/lists.h>
#include <lax/anobject.h>
#include <lax/refptrstack.h>
#include <lax/tagged.h>
#include <lax/dump.h>


namespace Laxkit {


//----------------------------------- utils ------------------------------------

const char *key_name_from_value(int ch,char *buf);
int key_value_from_name(const char *k);


//----------------------------------- KeyInfo ------------------------------------
class KeyInfo
{
  public:
	unsigned int key;
	unsigned long state;
	clock_t time;
	KeyInfo *next;
	KeyInfo(unsigned int k, unsigned int mods, clock_t t) : key(k), state(mods), time(t), next(NULL) {}
	KeyInfo() : key(0), state(0), time(0), next(NULL) {}
	virtual ~KeyInfo() { if (next) delete next; }
};


//----------------------------------- ShortcutDef ------------------------------------
class ShortcutDef
{
  public:
	KeyInfo *keys;
	int mode; //this shortcut only activated when win_mode==mode
	int action;
	int info1;

	ShortcutDef(unsigned int key, unsigned int state, int a, int m=-1);
	virtual ~ShortcutDef();
	virtual int match(unsigned int key, unsigned int state);
};

//----------------------------------- ShortcutDefs ------------------------------------
class ShortcutDefs : public PtrStack<ShortcutDef>, public anObject
{
  public:
	char *name;
	char *description;
	ShortcutDefs();
	virtual ~ShortcutDefs();
	virtual int Add(unsigned int key, unsigned int state, int a, int m=-1);
	virtual ShortcutDef *FindShortcutFromAction(int action, int startingfrom);
};


//----------------------------------- WindowAction ------------------------------------
class WindowAction : public Tagged
{
  public:
	int id; // <- corresponds to shortcut->action
	char *name; //such as for a menu line
	char *description; //short description
	char *iconname;
	int mode; //win_mode window must be in for action to be acted on
	int assignable; //whether to not allow keys to directly bind to them, or be visible to user in menus
	char *customcode;
	int info1;

	WindowAction(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);
	virtual ~WindowAction();
};

//---------------------------------- WindowModeInfo ------------------------------------------
class WindowModeInfo
{
  public:
	int mode;
	char *mode_str;
	char *name;
	char *description;
	WindowModeInfo(int m, const char *modestr, const char *Name, const char *desc);
	virtual ~WindowModeInfo();
};

//----------------------------------- WindowActions ------------------------------------
class WindowActions : public anObject, public PtrStack<WindowAction>
{
  public:
	char *name;
	char *description;
	ShortcutDefs *default_shortcuts;
	PtrStack<WindowModeInfo> modes;
	WindowActions();
	virtual ~WindowActions();
	virtual const char *Name() { return object_idstr; }
	virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);
	virtual int AddMode(int mode, const char *modestr, const char *name, const char *desc);
	virtual WindowAction *FindAction(int action);
	virtual WindowAction *ActionAt(int index);
};


//----------------------------------- ShortcutHandler ------------------------------------
class ShortcutHandler : public Laxkit::anObject
{
  protected:
	ShortcutDefs *shortcuts;
	WindowActions *actions;

	int index; //which is the active shortcut
	int keyindex; //how far into a shortcut chain to look
	int deviceid; //id of the active keyboard
  public:
	char *area;

	ShortcutHandler(const char *areaname=NULL, ShortcutDefs *cuts=NULL, WindowActions *wactions=NULL);
	virtual ~ShortcutHandler(); 
	virtual ShortcutHandler *duplicate();
	virtual anObject *duplicate(anObject *ref) { return dynamic_cast<ShortcutHandler*>(duplicate()); }
	virtual const char *whattype() { return "ShortcutHandler"; }

	virtual int NumActions();
	virtual WindowAction *Action(int i);
	virtual WindowActions *Actions();
	virtual WindowAction *FindAction(unsigned int key, unsigned int state, int mode);
	virtual int FindActionNumber(unsigned int key, unsigned int state, int mode);
	virtual int FindActionNumber(const char *actionname,int len=-1);

	virtual int NumShortcuts();
	virtual ShortcutDef *Shortcut(int i);
	virtual ShortcutDefs *Shortcuts();
	virtual int FindShortcutIndex(unsigned int key, unsigned int state, int mode);
	virtual int FindShortcutFromAction(int action, int startingfrom);
	virtual int InstallShortcuts(ShortcutDefs *cuts, bool absorb);

	virtual int AddMode(int mode, const char *modestr, const char *name, const char *desc);
	virtual int AddShortcut(unsigned int key, unsigned int state, unsigned int mode, unsigned int action);
	//virtual int AddShortcutChain(const char *keys, unsigned int *states, int n, unsigned int action, unsigned int mode);
	virtual unsigned int AddAction(unsigned int action, const char *nname, const char *desc, const char *icon, unsigned int mode, int assign);
	virtual unsigned int Add(unsigned int action, unsigned int key, unsigned int state, unsigned int mode,
							const char *nname, const char *desc, const char *icon, int assign);
	virtual int RemoveAction(int action);
	virtual int ClearShortcut(unsigned int key, unsigned int state, int mode);
	virtual int ClearShortcut(int i);
	virtual int UpdateKey(unsigned int key, unsigned int state, unsigned int mode, unsigned int newaction);
	virtual int ReassignKey(unsigned int newkey, unsigned int newstate,
							unsigned int oldkey, unsigned int oldstate,
							unsigned int mode, unsigned int action);
};

//--------------------------------- ShortcutManager ------------------------------------
class ShortcutManager : public DumpUtility, public anObject
{
  protected:

  public:
	char *settitle;
	char *subtitle;
	char *setname;
	char *setfile;
	Attribute tree;
	RefPtrStack<ShortcutHandler> shortcuts;
	// map zone -- ShortcutHandler

	ShortcutManager();
	virtual ~ShortcutManager();

	virtual int AddArea(const char *area, ShortcutDefs *cuts, WindowActions *actions);
	virtual int AddArea(const char *area, ShortcutHandler *handler);
	virtual int AreaParent(const char *area, const char *parent);
	virtual ShortcutHandler *NewHandler(const char *area);
	virtual ShortcutHandler *FindHandler(const char *area);

	virtual int Load(const char *file=NULL);
	virtual int LoadKeysOnly(const char *file);
	virtual int Save(const char *file=NULL, const char *header=NULL);
	virtual int SaveHTML(const char *file=NULL);
	virtual void dump_out(FILE *f,int indent,int what,DumpContext *savecontext);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext);
	virtual char *ShortcutString(ShortcutDef *def, char *buffer, bool mods_as_words);
	virtual char *ShortcutString(unsigned int key, unsigned long state, char *buffer, bool mods_as_words);
	virtual int KeyAndState(const char *str, unsigned int *key,unsigned int *state);

	virtual int ClearKeys();
	virtual int UpdateKey(const char *area, unsigned int key, unsigned int state, unsigned int mode, unsigned int newaction);
	virtual int ReassignKey(const char *area, unsigned int newkey, unsigned int newstate,
											  unsigned int oldkey, unsigned int oldstate,
											  unsigned int mode, unsigned int action);

	virtual WindowAction *FindAction(const char *area, unsigned int key, unsigned int state, int mode);
};

void InstallShortcutManager(ShortcutManager *manager);
ShortcutManager *GetDefaultShortcutManager();


} //namespace Laxkit

#endif //_LAX_SHORTCUT_H

