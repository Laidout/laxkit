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
	virtual int InstallShortcuts(ShortcutDefs *cuts);

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
class ShortcutManager : public LaxFiles::DumpUtility, public anObject
{
  protected:

  public:
	char *settitle, *setname, *setfile;
	LaxFiles::Attribute tree;
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
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *savecontext);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext);
	virtual char *ShortcutString(ShortcutDef *def, char *buffer);
	virtual char *ShortcutString(unsigned int key, unsigned long state, char *buffer);
	virtual int KeyAndState(const char *str, unsigned int *key,unsigned int *state);

	virtual int ClearKeys();
	virtual int UpdateKey(const char *area, unsigned int key, unsigned int state, unsigned int mode, unsigned int newaction);
	virtual int ReassignKey(const char *area, unsigned int newkey, unsigned int newstate,
											  unsigned int oldkey, unsigned int oldstate,
											  unsigned int mode, unsigned int action);
};

void InstallShortcutManager(ShortcutManager *manager);
ShortcutManager *GetDefaultShortcutManager();
void FinalizeShortcutManager();


} //namespace Laxkit

#endif //_LAX_SHORTCUT_H

