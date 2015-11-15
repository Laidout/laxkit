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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_CHARACTERINTERFACE_H
#define _LAX_CHARACTERINTERFACE_H

#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 



class CharacterInterface : public anInterface
{
  protected:
	int showdecs;
	Laxkit::LaxFont *font;

	char *recent;
	char *suggestions;

	Laxkit::ScreenColor fg, bg;

	int current, curcategory;

	int firsttime;
	int displaytype; //0: character only,  1: char + U0123,  2: char + U0123 + glyphname
	double boxwidth;
	int numwide, numtall;
	flatpoint offset;
	flatpoint insertpoint;
	Laxkit::DoubleBBox recentbox;
	Laxkit::DoubleBBox suggestionbox;
	Laxkit::DoubleBBox bigbox;

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
	virtual void ScreenPosition(flatpoint p);
	virtual int scan(int x, int y, unsigned int state, int *category);
	virtual void SetBoxes();

	virtual int NumRecent();
	virtual int NumSuggestions();
};

} // namespace LaxInterfaces

#endif

