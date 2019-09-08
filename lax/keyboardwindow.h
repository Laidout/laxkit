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

#ifndef _LAX_KEYBOARDWINDOW_H
#define _LAX_KEYBOARDWINDOW_H



#include <lax/lists.h>
#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>
#include <lax/rectangles.h>

#include <string>



namespace Laxkit {


//----------------------------------- helper funcs ----------------------------------
const char *XlibToNormal(const char *str);


//----------------------------------- Keymap ----------------------------------
class Keymap
{
  public:
	std::string name;
	unsigned int ch;    //unicode or LAX_* of the character produced
	unsigned int mods; //modifiers that produce this key: ShiftMask|ControlMask|MetaMask|AltMask
	int keygroup, keylevel; //0 normal, 1 keypad, 2 escape+function keys, 4 custom
	ScreenColor fill;
	anObject *extra;

	Keymap(const char *str, unsigned int ch,unsigned int mods);
	~Keymap();
	void AddExtra(anObject *nextra, int absorb);
};


//----------------------------------- Key ----------------------------------
//Key::keysection:
enum KeySectionType {
	KEYSECTION_Done     =0,
	KEYSECTION_Normal   ,
	KEYSECTION_Keypad   ,
	KEYSECTION_Fkeys    ,
	KEYSECTION_Controls ,
	KEYSECTION_MAX
};

enum KeyTypes {
	KEYTYPE_Label           =0,
	KEYTYPE_Normal          ,
	KEYTYPE_ToggleModifier  ,
	KEYTYPE_Modifier        ,
	KEYTYPE_MAX
};

class KeyShape : public RefCounted, public DoubleRectangle
{
  public:
	std::string name;
	double round; //how much to round corners
	NumStack<flatpoint> points;
	ScreenColor fill, stroke;

	KeyShape() { round = 0; }
};

class KeySection
{
  public:
	std::string name;
	ScreenColor fill, stroke;
	flatpoint origin;
};

class KeyboardLight : public DoubleRectangle
{
  public:
	ScreenColor on;
	ScreenColor off;
	flatpoint *pts;
	NumStack<flatpoint> points;
};

class Key
{
  public:
	int keysection; //hint for physical grouping
	int keycode; //key scan code
	int keytype;
	int mod; //if is modifier key, this is the modifier mask. otherwise 0
	int down; //1 if button is down, else 0
	int finger; //which finger is expected to be hitting this key (0-7, left-right)
	int tag;

	IntRectangle position;
	PtrStack<Keymap> keymaps; //alternate keys when there are modifiers
	KeySection *section;

	flatpoint origin;
	KeyShape *shape;

	Key(int x, int y, int w, int h, int thefinger, int group, int ktype, int nmod,
		const char *name, unsigned int ch, unsigned int upper, unsigned int other, int nkeycode);
	Keymap *MatchMods(int mods);
	Keymap *HasKey(unsigned int unicode);
};

class KeyModStyle
{
  public:
	ScreenColor fill;
	int mod;
	std::string name;
	std::string short_name;

	KeyModStyle();
	KeyModStyle(int nmod, double r,double g,double b, const char *modName, const char *modShortName);
};


//----------------------------------- Keyboard ----------------------------------

class Keyboard : public Laxkit::DoubleRectangle
{
  public:
	double basewidth,baseheight; //key coords are in basewidth x baseheight
	std::string name;
	std::string description;
	std::string language;

	PtrStack<Key> keys;
	PtrStack<KeyShape> shapes;
	PtrStack<KeySection> sections;
	PtrStack<KeyboardLight> lights;

	PtrStack<KeyModStyle> modstyles;

	int homekeys[8]; //from left to right, indices of home keys for fingers (not thumbs)

	int curgroup;

	Keyboard(const char *nme, const char *lang, const char *desc);
	virtual ~Keyboard();
	virtual int AddInGroup(int g);
	virtual int AddKey(int x, int y, int w, int h, int finger, int keytype,
					   const char *name, unsigned int ch, unsigned int upper, unsigned int other,
					   int mod=0, int keycode=0);

	virtual Key *key(int i) { return i >= 0 && i < keys.n ? keys.e[i] : nullptr; }
    virtual Key *FindKey(int keycode, unsigned int ch, unsigned int state, Keymap **kmap=nullptr, int *index_ret=nullptr);
	virtual Key *FindKeyFromKeycode(int keycode, unsigned int state, Keymap **kmap);
	virtual const char *Name();
	virtual KeyModStyle *ModStyle(int mod);

    virtual int ImportXkb(const char *file, int index);
	virtual int ExportSVG(const char *file, bool with_list, bool with_labels);

	virtual void ApplyCurrentLocale();
	virtual void ApplyDefaultKeycodes();
};


//----------------------------------- KeyboardWindow ----------------------------------

enum KeyboardWindowEnum {
	KBWIN_RESIZEABLE      = (1<<16),
	KBWIN_Send_Fake_Keys  = (1<<17),
    KBWIN_Show_Double_Row = (1<<18)
};

enum KWinActions {
	KMENU_Force_Toggles,
	KMENU_Select_Keyboard,
	KMENU_Load_Keyboard,
	KMENU_Save_Keyboard,
	KMENU_Reset_Keyboard,
	KMENU_Export_SVG,
	KMENU_Export_Html_List,
	KMENU_Export_ShortcutMapper,
	KMENU_MAX
};

class KeyboardWindow : public anXWindow
{
  protected:
	Keyboard *keyboard;
	unsigned int currentmods;
	int showallmaps;
	ButtonDownInfo buttondown;
	LaxKeyboard dummydevice;

    ScreenColor defaultKeyBG;
    ScreenColor defaultKeyOutline;
    ScreenColor defaultKeyText;

	virtual unsigned int CurrentChar(Key *key,int mods);
    virtual int sendFakeKeyEvent(unsigned int key, unsigned int mods);
	virtual void send(bool down, unsigned int key, unsigned int mods);
	virtual int hasHover(int c);
    virtual void DrawKeyText(Key *key, int mods, double x,double y,double w,double h);

  public:
	int send_type;
    double gap;
    double margin;
	bool force_toggles; //force modifier keys to always be toggles (like caps lock)
    bool keys_to_move; //temp: whether you can use keys to reposition window

	KeyboardWindow(anXWindow *parnt, const char *nname, const char *ntitle,
			unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsend,
			const char *kb);
	virtual ~KeyboardWindow();
	virtual const char *whattype() { return "KeyboardWindow"; }
	virtual int init();

	virtual int Event(const EventData *data,const char *mes);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int KeyUp(unsigned int ch,unsigned int state, const LaxKeyboard *kb);
	//virtual int KeyUp(unsigned int ch,unsigned int state, const LaxKeyboard *kb);
	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	//virtual int ButtonDown(int button, int x,int y,unsigned int state,int count, const LaxMouse *m);
	//virtual int ButtonUp  (int button, int x,int y,unsigned int state, const LaxMouse *m);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	//virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	//virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual void Refresh();
	virtual void PostRefresh(Displayer *dp);
	virtual void CurrentModsColor(ScreenColor &curModsColor, int withmods = -1);
	
	virtual int scan(int x,int y);
	virtual int ShiftWindow(int dx,int dy);
	virtual Keyboard *GetKeyboard() { return keyboard; }
	virtual unsigned int CurrentMods() { return currentmods; }
	virtual void UpdateMouseOver(int i) {} //called whenever the mouse hover changes
	virtual void DrawMouseOverTip(Key *key, double x, double y, double w, double h) {} //called each refresh for hovered key
};


} // namespace Laxkit


#endif

