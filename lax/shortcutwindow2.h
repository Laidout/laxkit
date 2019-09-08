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
#ifndef _LAX_SHORTCUTWINDOW2_H
#define _LAX_SHORTCUTWINDOW2_H


#include "keyboardwindow.h"

#include <lax/rowframe.h>
#include <lax/menuinfo.h>
#include <lax/stackframe.h>
#include <lax/messagebar.h>

#include <string>


namespace Laxkit {

enum ShortcutWindow2Styles
{
	SHORTCUTW2_Show_Search = (1<<16),
	SHORTCUTW2_Load_Save   = (1<<17),
	SHORTCUTW2_MAX
};

class ShortcutKBWindow : public KeyboardWindow
{
  protected:
	int dnd_hover_key;

  public:
	int rbdown; //key index
	unsigned int rbdown_ch; //key char
	unsigned int rbdown_mods;
	int rbdown_mode;
	int current_mode;

	std::string current_area;
	//MessageBar *ttip;

	ShortcutKBWindow(anXWindow *parnt, const char *nname, const char *ntitle,
            unsigned long nstyle,
            int xx,int yy,int ww,int hh,int brder,
            anXWindow *prev,unsigned long nowner,const char *nsend,
            const char *kb);
	virtual ~ShortcutKBWindow() {}
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual void UpdateCurrent(bool update_mods=false);
    virtual void send(bool down, unsigned int key, unsigned int mods);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual void UpdateMouseOver(int i);
	virtual void DrawMouseOverTip(Key *key, double x, double y, double w, double h);
	virtual int DropHover(double x, double y);
	virtual void PostRefresh(Displayer *dp);
	virtual int HighlightKey(int which, unsigned int mods);
};

class ShortcutWindow2 : public Laxkit::StackFrame
{
  protected:
	char *initialarea;
	Laxkit::ShortcutHandler *sc;
	//virtual int PerformAction(int action);

	ShortcutKBWindow *keyboard;
	unsigned long swin_style;
	int search_type;
	bool use_locale;

	//std::string current_area;

	void UpdateSearch();
	virtual MenuInfo *GetSettingsMenu();

	//virtual void UpdateCurrent();

  public:
	std::string platform; //Windows, Linux, Mac
	char *textheader; //saved as first part of keys file

	ShortcutWindow2(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						const char *place=NULL);
	virtual ~ShortcutWindow2();
	virtual const char *whattype() { return "ShortcutWindow2"; }
	virtual int init();
	//virtual Laxkit::ShortcutHandler *GetShortcuts();
	//virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	//virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	//virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	//virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int SetSearch(const char *str, int searchtype=-1);
	virtual int SelectArea(const char *area);
	virtual int ApplyCurrentLocale();
	virtual int ExportSVG(const char *file, bool with_list, bool with_labels, int mods);

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	
};

} //namespace Laxkit

#endif

