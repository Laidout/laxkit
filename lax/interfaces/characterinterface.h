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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_CHARACTERINTERFACE_H
#define _LAX_CHARACTERINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/singletonkeeper.h>


namespace LaxInterfaces { 



class CharacterInterface : public anInterface
{
  protected:
  	class Settings : public Laxkit::anObject
  	{
  	  public:
  	  	double boxwidth = 30.0;
  	  	bool show_unicode = false;
  	  	Laxkit::ScreenColor fg;
  	  	Laxkit::ScreenColor bg;
		Laxkit::NumStack<int> recent;
  	};

	static Laxkit::SingletonKeeper settingsObject;
	Settings *settings;

	int showdecs;
	Laxkit::LaxFont *font;

	Laxkit::NumStack<int> suggestions;

	int current, curcategory;

	int firsttime;
	int numwide, numtall;
	Laxkit::flatpoint offset;
	Laxkit::flatpoint insertpoint;
	Laxkit::DoubleBBox recentbox;
	Laxkit::DoubleBBox suggestionbox;
	Laxkit::DoubleBBox bigbox; //box that contains main list of characters

	//Laxkit::NumStack<int> ranges;
	//Laxkit::NumStack<int> starts;
	Laxkit::NumStack<int> chars;
	bool needtosetchars;
	virtual void SetupChars();

	Laxkit::ShortcutHandler *sc;

	virtual int send();


  public:
	enum SelectCategory {
		INSCHAR_None=0,
		INSCHAR_Recent,
		INSCHAR_Suggestions,
		INSCHAR_MainBox,
		INSCHAR_MAX,
	};

	enum Actions {
		CHARI_ToggleUnicode,
		CHARI_Copy,  //copy current to clipboard
		CHARI_Paste, //search for first characeter of pasted text
		CHARI_Search,
		CHARI_MAX
	};

	unsigned int character_interface_style;

	CharacterInterface(anInterface *nowner, int nid, Laxkit::Displayer *ndp, Laxkit::LaxFont *nfont);
	virtual ~CharacterInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Character"; }
	const char *Name();
	const char *whattype() { return "CharacterInterface"; }
	const char *whatdatatype();
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *loadcontext);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();

	virtual int Font(Laxkit::LaxFont *font);
	virtual int Font(const char *family, const char *style, double size, Laxkit::ScreenColor *color);
	virtual int Context(const char *str, long pos, long len);
	virtual void ScreenPosition(Laxkit::flatpoint p);
	virtual int scan(int x, int y, unsigned int state, int *category);
	virtual void SetBoxes();

	virtual int NumRecent();
	virtual int NumSuggestions();
};

} // namespace LaxInterfaces

#endif

