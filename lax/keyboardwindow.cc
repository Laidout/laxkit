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

#include <lax/language.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include <cstring>
#include <X11/Xlib.h>

#include <lax/keyboardwindow.h>


//template implementation:
#include <lax/lists.cc>


#include <iostream>
using namespace std;
#define DBG 

		  
#define HOVER (-1)


using namespace LaxFiles;


namespace Laxkit {



//----------------------------------- default keyboard layout ----------------------------------

struct KeycodeSymMap {
	int keysym;
	unsigned int code;
};

static const KeycodeSymMap keycodemap[] = {
	{ 8, },
	{ 0,0 }
};

struct KeyboardSimpleMap {
	unsigned int keycode;
	const char *normal;
	const char *shift;
	int finger;
};

const KeyboardSimpleMap en_dvorak[]={
		//"lower", "upper", "finger 0-7"
		{ 49,  "`","~", 0 },
		{ 10,  "1","!", 0 },
		{ 11,  "2","@", 1 },
		{ 12,  "3","#", 2 },
		{ 13,  "4","$", 3 },
		{ 14,  "5","%", 4 },
		{ 15,  "6","^", 4 },
		{ 16,  "7","&", 4 },
		{ 17,  "8","*", 5 },
		{ 18,  "9","(", 6 },
		{ 19,  "0",")", 7 },
		{ 20,  "[","{", 7 },
		{ 21,  "]","}", 7 },
		{ 22,  "Bksp","Bksp", 7 },
		{ 23,  "Tab","Tab", 0 },
		{ 24,  "'","\"", 0 },
		{ 25,  ",","<", 1 },
		{ 26,  ".",">", 2 },
		{ 27,  "p","P", 3 },
		{ 28,  "y","Y", 3 },
		{ 29,  "f","F", 4 },
		{ 30,  "g","G", 5 },
		{ 31,  "c","C", 6 },
		{ 32,  "r","R", 7 },
		{ 33,  "l","L", 7 },
		{ 34,  "/","?", 7 },
		{ 35,  "=","+", 7 },
		{ 51,  "\\","|", 7 },
		{ 66,  "Caps","Caps", 0 },
		{ 38,  "a","A", 0 },
		{ 39,  "o","O", 1 },
		{ 40,  "e","E", 2 },
		{ 41,  "u","U", 3 },
		{ 42,  "i","I", 3 },
		{ 43,  "d","D", 4 },
		{ 44,  "h","H", 4 },
		{ 45,  "t","T", 5 },
		{ 46,  "n","N", 6 },
		{ 47,  "s","S", 7 },
		{ 48,  "-","_", 7 },
		{ 36,  "Enter","Enter", 7 },
		{ 50,  "Shift","Shift", 0 },
		{ 52,  ";",":", 0 },
		{ 53,  "q","Q", 1 },
		{ 54,  "j","J", 2 },
		{ 55,  "k","K", 3 },
		{ 56,  "x","X", 3 },
		{ 57,  "b","B", 4 },
		{ 58,  "m","M", 4 },
		{ 59,  "w","W", 5 },
		{ 60,  "v","V", 6 },
		{ 61,  "z","Z", 7 },
		{ 62,  "Shift","Shift", 7 },
		{ 37,  "Cntl","Cntl", 0 },
		{ 133, "Meta","Meta", 0 },
		{ 64,  "Alt","Alt", 1 },
		{ 65,  "Space","Space",  },
		{ 108, "Alt","Alt", 6 },
		{ 134, "Meta","Meta", 7 },
		{ 135, "Menu","Menu", 70 },
		{ 105, "Cntl","Cntl", 7 },

        { 0, nullptr,nullptr, 0 }
	};

const KeyboardSimpleMap en_qwerty[]={
		{ 49,  "`","~", 0 },
		{ 10,  "1","!", 0 },
		{ 11,  "2","@", 1 },
		{ 12,  "3","#", 2 },
		{ 13,  "4","$", 3 },
		{ 14,  "5","%", 4 },
		{ 15,  "6","^", 4 },
		{ 16,  "7","&", 4 },
		{ 17,  "8","*", 5 },
		{ 18,  "9","(", 6 },
		{ 19,  "0",")", 7 },
		{ 20,  "-","_", 7 },
		{ 21,  "=","+", 7 },
		{ 22,  "Bksp","Bksp", 7 },
		{ 23,  "Tab","Tab", 0 },
		{ 24,  "q","Q", 0 },
		{ 25,  "w","W", 1 },
		{ 26,  "e","E", 2 },
		{ 27,  "r","R", 3 },
		{ 28,  "t","T", 3 },
		{ 29,  "y","Y", 4 },
		{ 30,  "u","U", 5 },
		{ 31,  "i","I", 6 },
		{ 32,  "o","O", 7 },
		{ 33,  "p","P", 7 },
		{ 34,  "[","{", 7 },
		{ 35,  "]","}", 7 },
		{ 51,  "\\","|", 7 },
		{ 66,  "Caps","Caps", 0 },
		{ 38,  "a","A", 0 },
		{ 39,  "s","S", 1 },
		{ 40,  "d","D", 2 },
		{ 41,  "f","F", 3 },
		{ 42,  "g","G", 3 },
		{ 43,  "h","H", 4 },
		{ 44,  "j","J", 4 },
		{ 45,  "k","K", 5 },
		{ 46,  "l","L", 6 },
		{ 47,  ";",":", 7 },
		{ 48,  "'","\"", 7 },
		{ 36,  "Enter","Enter", 7 },
		{ 50,  "Shift","Shift", 0 },
		{ 52,  "z","Z", 0 },
		{ 53,  "x","X", 1 },
		{ 54,  "c","C", 2 },
		{ 55,  "v","V", 3 },
		{ 56,  "b","B", 3 },
		{ 57,  "n","N", 4 },
		{ 58,  "m","M", 4 },
		{ 59,  ",","<", 5 },
		{ 60,  ".",">", 6 },
		{ 61,  "/","?", 7 },
		{ 62,  "Shift","Shift", 7 },
		{ 37,  "Cntl","Cntl", 0 },
		{ 133, "Meta","Meta", 0 },
		{ 64,  "Alt","Alt", 1 },
		{ 65,  "Space","Space",  },
		{ 108, "Alt","Alt", 6 },
		{ 134, "Meta","Meta", 7 },
		{ 135, "Menu","Menu", 70 },
		{ 105, "Cntl","Cntl", 7 },

        { 0, nullptr,nullptr, 0 }
	};

//----------------------------------- forward decs ----------------------------------

static unsigned int strtoChar(const char *s, unsigned int *other, int *keytype, int *mod);



//----------------------------------- Keymap ----------------------------------
/*! \class Keymap
 * \brief Description of what characters should be displayed on a key, and what modifiers produce it.
 *
 * Particular Key objects would have multiple Keymap objects to describe the various
 * things that can result from pressing.
 */


Keymap::Keymap(const char *str, unsigned int c,unsigned int mod)
{
	name = str?str:"";
	ch   = c;
	mods = mod;
	keygroup = keylevel = 0;
	extra = nullptr;
}

Keymap::~Keymap()
{
	if (extra) extra->dec_count();
}

void Keymap::AddExtra(anObject *nextra, int absorb)
{
	if (nextra == extra) {
		if (extra && absorb) extra->dec_count();
		return;
	}

	if (extra) extra->dec_count();
	extra = nextra;
	if (extra && !absorb) extra->inc_count();
}


//----------------------------------- Key ----------------------------------
/*! \class Key
 * \brief Describes one physical key position, with possibly multiple characters mapped to it.
 */


/*! Usually supply ch+upper, OR other.
 */
Key::Key(int x, int y, int w, int h, int thefinger, int group, int ktype, int nmod,
			const char *name, unsigned int ch, unsigned int upper, unsigned int other, int nkeycode)
{
	keysection = group;
	keytype = ktype;
	mod     = nmod;
	keycode = nkeycode;
	down    = 0;
	tag     = 0;

	shape   = nullptr;
	section = nullptr;

	position.x      = x;
	position.y      = y;
	position.width  = w;
	position.height = h;
	finger          = thefinger;

	if (ch) {
		keymaps.push(new Keymap(name, ch, 0));
		if (upper) {
			char str[10];
			sprintf(str,"%c",upper);
			keymaps.push(new Keymap(str, upper, ShiftMask));
		}

	} else if (other) {
		keymaps.push(new Keymap(name, other, 0));
	}
}

/*! Return a keymap that most closely matches mods.
 * This should always return a non-null object. If no exact match
 * is found, returns map 0.
 */
Keymap *Key::MatchMods(int mods)
{
	unsigned int basemods = mods & (ShiftMask | ControlMask | AltMask | MetaMask);
	if (mods & CapsLockMask) basemods ^= ShiftMask;

	int c;
	for (c=1; c<keymaps.n; c++) {
		if (keymaps.e[c]->mods != basemods) continue;
		return keymaps.e[c];
	}

	return keymaps.e[0];
}

/*! Return the first in keymaps that has unicode as the key.
 * If not found, return nullptr.
 */
Keymap *Key::HasKey(unsigned int unicode)
{
	int c;
	for (c=0; c<keymaps.n; c++) {
		if (keymaps.e[c]->ch == unicode) return keymaps.e[c];
	}

	return nullptr;
}


//----------------------------------- KeyModStyle ----------------------------------

KeyModStyle::KeyModStyle()
{
	fill.rgbf(1,1,1);
	mod = 0;
}

KeyModStyle::KeyModStyle(int nmod, double r,double g,double b, const char *modName, const char *modShortName)
{
	mod = nmod;
	fill.rgbf(r,g,b);
	name = modName;
	short_name = modShortName;
}


//----------------------------------- Keyboard ----------------------------------

/*! \class Keyboard
 * \brief Describes a keyboard layout.
 */


Keyboard::Keyboard(const char *nme, const char *lang, const char *desc)
{
    name        = nme ? nme : _("Keyboard");
    language    = lang ? lang : "";
    description = desc ? desc : "";

    x = y = width = height = 0;
    basewidth = baseheight = 0;

    curgroup = 0;

	//default mod styles:
	modstyles.push(new KeyModStyle(ShiftMask,    .6,1.,.6, "Shift",   "+"));
	modstyles.push(new KeyModStyle(ControlMask,  1.,.6,.6, "Control", "^"));
	modstyles.push(new KeyModStyle(AltMask,      .6,.6,1., "Alt",     "&"));
	modstyles.push(new KeyModStyle(MetaMask,     .6,1.,1., "Meta",    "%"));
	//modstyles.push(new KeyModStyle(CapsLockMask, .6,1.,.6, "CapsLock","+"));
	//modstyles.push(new KeyModStyle(NumLockMask,  .6,1.,.6, "NumLock",""));
}

Keyboard::~Keyboard()
{}

KeyModStyle *Keyboard::ModStyle(int mod)
{
	for (int c=0; c<modstyles.n; c++) {
		if (modstyles.e[c]->mod == mod) return modstyles.e[c];
	}
	return nullptr;
}

//! When using AddKey(), will attach to this key group.
int Keyboard::AddInGroup(int g)
{ return curgroup=g; }

//! Add a key position to the list.
/*! Returns the index in keys of the new key.
 */
int Keyboard::AddKey(int x, int y, int w, int h, int finger, int keytype,
				   const char *name, unsigned int ch, unsigned int upper, unsigned int other,int mod, int keycode)
{
	return keys.push(new Key(x,y,w,h,finger,curgroup, keytype,mod, name,ch,upper,other, keycode));
}

const char *Keyboard::Name()
{
	if (isblank(name.c_str())) return _("Keyboard");
	return name.c_str();
}

/*! Find the key that has the given character, optionally return the Keymap it is in.
 * Search first with keycode if keycode > 0.
 * If no match, search for ch.
 * Returns nullptr if not found.
 */
Key *Keyboard::FindKey(int keycode, unsigned int ch, unsigned int state, Keymap **kmap, int *index_ret)
{
    if (keycode > 0) {
        for (int c = 0; c < keys.n; c++) {
            if (keycode == keys.e[c]->keycode) {
                for (int c2 = 0; c2 < keys.e[c]->keymaps.n; c2++) {
                    if (ch == keys.e[c]->keymaps.e[c2]->ch && state == keys.e[c]->keymaps.e[c2]->mods) {
                        if (kmap) *kmap = keys.e[c]->keymaps.e[c2];
						if (index_ret) *index_ret = c;
                        return keys.e[c];
                    }
                }
                if (kmap) *kmap = keys.e[c]->keymaps.e[0];
				if (index_ret) *index_ret = c;
                return keys.e[c];
            }
        }
    }

	//try to match ch, ignore state and keycode
    for (int c = 0; c < keys.n; c++) {
        for (int c2 = 0; c2 < keys.e[c]->keymaps.n; c2++) {
            if (ch == keys.e[c]->keymaps.e[c2]->ch) {
                if (kmap) *kmap = keys.e[c]->keymaps.e[c2];
				if (index_ret) *index_ret = c;
                return keys.e[c];
            }
        }
    }

    if (kmap) *kmap = nullptr;
	if (index_ret) *index_ret = -1;
    return nullptr;
}

/*! Match keycode and state.
 * If state does not match a keymap for the key, kmap gets nullptr, but key is still returned.
 */
Key *Keyboard::FindKeyFromKeycode(int keycode, unsigned int state, Keymap **kmap)
{
    if (keycode <= 0) return nullptr;

	for (int c = 0; c < keys.n; c++) {
		if (keycode == keys.e[c]->keycode) {
			for (int c2 = 0; c2 < keys.e[c]->keymaps.n; c2++) {
				if (state == keys.e[c]->keymaps.e[c2]->mods) {
					if (kmap) *kmap = keys.e[c]->keymaps.e[c2];
					return keys.e[c];
				}
			}
			if (kmap) *kmap = nullptr;
			return keys.e[c];
		}
	}

	if (kmap) *kmap = nullptr;
	return nullptr;
}

const char *XlibToNormal(const char *str)
{
	if (!str) return nullptr;

	if (!strcmp(str, "Escape")) return "Esc";
	if (!strcmp(str, "bracketleft")) return "[";
	if (!strcmp(str, "bracketright")) return "]";
	if (!strcmp(str, "grave")) return "`";
	if (!strcmp(str, "apostrophe")) return "'";
	if (!strcmp(str, "comma")) return ",";
	if (!strcmp(str, "period")) return ".";
	if (!strcmp(str, "slash")) return "/";
	if (!strcmp(str, "equal")) return "=";
	if (!strcmp(str, "backslash")) return "\\";
	if (!strcmp(str, "Caps_Lock")) return "Caps";
	if (!strcmp(str, "minus")) return "-";
	if (!strcmp(str, "Shift_L")) return "Shift";
	if (!strcmp(str, "Shift_R")) return "Shift";
	if (!strcmp(str, "Control_L")) return "Control";
	if (!strcmp(str, "Control_R")) return "Control";
	if (!strcmp(str, "Alt_L")) return "Alt";
	if (!strcmp(str, "Alt_R")) return "Alt";
	if (!strcmp(str, "Super_L")) return "Meta";
	if (!strcmp(str, "Super_R")) return "Meta";
	if (!strcmp(str, "semicolon")) return ";";
	if (!strcmp(str, "bar")) return "|";
	if (!strcmp(str, "Prior")) return "PgUp";
	if (!strcmp(str, "Next")) return "PgDown";
	if (!strcmp(str, "Return")) return "Enter";
	
	if (!strcmp(str, "Num_Lock")) return "Num";
	if (!strcmp(str, "KP_Divide")) return "/";
	if (!strcmp(str, "KP_Multiply")) return "*";
	if (!strcmp(str, "KP_Add")) return "+";
	if (!strcmp(str, "KP_Subtract")) return "-";
	if (!strcmp(str, "KP_Enter")) return "Enter";

	if (!strcmp(str, "KP_Home")) return "7";
	if (!strcmp(str, "KP_Up")) return "8";
	if (!strcmp(str, "KP_Left")) return "4";
	if (!strcmp(str, "KP_Right")) return "6";
	if (!strcmp(str, "KP_Down")) return "2";
	if (!strcmp(str, "KP_End")) return "1";
	if (!strcmp(str, "KP_Prior")) return "9";
	if (!strcmp(str, "KP_Next")) return "3";
	if (!strcmp(str, "KP_Insert")) return "0";
	if (!strcmp(str, "KP_Delete")) return ".";
	if (!strcmp(str, "KP_Begin")) return "5";
	//-----
	//if (!strcmp(str, "KP_Home")) return "Home";
	//if (!strcmp(str, "KP_Up")) return "Up";
	//if (!strcmp(str, "KP_Left")) return "Left";
	//if (!strcmp(str, "KP_Right")) return "Right";
	//if (!strcmp(str, "KP_Down")) return "Down";
	//if (!strcmp(str, "KP_End")) return "End";
	//if (!strcmp(str, "KP_Prior")) return "PgUp";
	//if (!strcmp(str, "KP_Next")) return "PgDown";
	//if (!strcmp(str, "KP_Insert")) return "Ins";
	//if (!strcmp(str, "KP_Delete")) return "Del";
	//if (!strcmp(str, "KP_Begin")) return "Begin";

	if (!strcmp(str, "asciitilde")) return "~";
	if (!strcmp(str, "exclam")) return "!";
	if (!strcmp(str, "at")) return "@";
	if (!strcmp(str, "numbersign")) return "#";
	if (!strcmp(str, "dollar")) return "$";
	if (!strcmp(str, "percent")) return "%";
	if (!strcmp(str, "asciicircum")) return "^";
	if (!strcmp(str, "ampersand")) return "&";
	if (!strcmp(str, "asterisk")) return "*";
	if (!strcmp(str, "parenleft")) return "(";
	if (!strcmp(str, "parenright")) return ")";
	if (!strcmp(str, "braceleft")) return "{";
	if (!strcmp(str, "braceright")) return "}";
	if (!strcmp(str, "question")) return "?";
	if (!strcmp(str, "plus")) return "+";
	if (!strcmp(str, "underscore")) return "_";
	if (!strcmp(str, "colon")) return ":";
	if (!strcmp(str, "greater")) return ">";
	if (!strcmp(str, "less")) return "<";
	if (!strcmp(str, "quotedbl")) return "\"";

	//if (!strcmp(str, "")) return "";
	//if (!strcmp(str, "")) return "";

	return str;
}

/*! Main keyboard layout must already be set with keycodes.
 * Current map is loaded from xlib, and attempt is made to apply it to current
 * physical layout.
 */
void Keyboard::ApplyCurrentLocale()
{
	int keysyms_per_keycode = -1;
	int first_keycode = 0, last_keycode = 0;

	XDisplayKeycodes(anXApp::app->dpy, &first_keycode, &last_keycode);
	int num_keycodes = last_keycode - first_keycode + 1;

	KeySym *list = XGetKeyboardMapping(anXApp::app->dpy, first_keycode, num_keycodes, &keysyms_per_keycode);

	for (int c=0; c<num_keycodes; c++) {
		//printf("%02x/%d: ", c + first_keycode, c + first_keycode);

		Key *key = FindKeyFromKeycode(first_keycode + c, 0, nullptr);
		if (!key) continue;

		for (int c2=0; c2<keysyms_per_keycode; c2++) {

			const char *str = XlibToNormal(XKeysymToString(list[c*keysyms_per_keycode + c2]));
			if (!str) continue;
			unsigned int state = 0;
			if (c2 == 0) {
				key->keymaps.e[0]->name = str;
				key->keymaps.e[0]->ch = filterkeysym(list[c*keysyms_per_keycode + c2], &state);
			}
			if (c2 == 1 && key->keymaps.n>1) {
				key->keymaps.e[1]->name = str;
				key->keymaps.e[1]->ch = filterkeysym(list[c*keysyms_per_keycode + c2], &state);
			}
			//printf("%d(%-4x): %-10s ",
			//		c2,
			//		(unsigned int)list[c*keysyms_per_keycode + c2],
			//		str ? str : "(none)");
		}
		//printf("\n");
	}

	XFree(list);
}


/*! Apply default key mappings from en_qwerty.
 */
void Keyboard::ApplyDefaultKeycodes()
{
	for (int c=0; c<keys.n; c++) {
		Key *key = keys.e[c];
		int ii = -1;

		for (int c2=0; en_qwerty[c2].keycode != 0; c2++) {
			if ((int)en_qwerty[c2].keycode == keys.e[c]->keycode) {
				ii = c2;
				break;
			}
		}

		if (ii == -1) continue; //couldn't find key!
		key->keymaps.e[0]->name = en_qwerty[ii].normal;
		if (key->keymaps.n > 1) key->keymaps.e[1]->name = en_qwerty[ii].shift;
	}
}


//-------------------------------------- Export SVG --------------------------------

/*! Export the keyboard with simple key labels to an svg.
 */
int Keyboard::ExportSVG(const char *file, bool with_list, bool with_labels)
{
	FILE *f = fopen(file, "w");
	if (!f) return 1;

	double linewidth  = keys.e[0]->position.height*.02;
	double textheight = keys.e[0]->position.height*.2;
	double round = keys.e[0]->position.height * .15;
	double margin = textheight * 1;
	string str;
	double x,y;

	fprintf(f, "<svg width=\"%.10gpx\" height=\"%.10gpx\">\n<g>\n",
			basewidth+2*margin, baseheight+2*margin);


	//draw whole keyboard background
	fprintf(f, "  <rect id=\"board\" style=\"stroke-width:%.10gpx; fill:#aaa; stroke:#333;\" "
			   " rx=\"%.10g\" x=\"%.10g\" y=\"%.10g\" width=\"%.10g\" height=\"%.10g\" />\n",
			   linewidth, textheight, 0.,0., basewidth+2*margin, baseheight+2*margin
		   );

	//draw each key
	for (int c=0; c<keys.n; c++) {
		Key *k = key(c);

		fprintf(f, "  <rect style=\"stroke-width:%.10gpx; fill:#fff; stroke:#888;\" "
				   " rx=\"%.10g\" x=\"%.10g\" y=\"%.10g\" width=\"%.10g\" height=\"%.10g\" />\n",
				    linewidth,
					round,
					margin + k->position.x + round * .3,
					margin + k->position.y + round * .3,
					k->position.width - round * .6,
					k->position.height - round * .6
				);
		const char *keytext = k->keymaps.n>0 ? k->keymaps.e[0]->name.c_str() : nullptr;
		if (keytext) {
			if (keytext[1] == '\0' && keytext[0] >= 'a' && keytext[0] <= 'z') {
				str = keytext;
				str[0] = toupper(str[0]);
				keytext = str.c_str();
			}

			if (with_labels) {
				//make key label upper left
				x = margin + k->position.x + textheight/2.;
				y = margin + k->position.y + textheight;
			} else {
				//center key label
				// *** need special treatment to try to make fit within key
				x = margin + k->position.x + k->position.width/2.;
				y = margin + k->position.y + k->position.height/2. + textheight/2;
			}

			fprintf(f, "  <text style=\"fill:#000; text-anchor:middle; text-align:center; font-size:%.10gpx;\""
					   " x=\"%.10g\" y=\"%.10g\"><tspan>%s</tspan></text>\n",
					textheight,
					x, y,
					keytext
				);
		}
	}

	fprintf(f, "</g>\n</svg>\n");
	fclose(f);
    return 0;
}


//-------------------------------------- Import Xkb --------------------------------

/*! Doesn't return nullptr.
 */
static void skipWSComment(char *&str)
{
	while (isspace(*str)) str++;
    char *p;
	if (str[0]=='/' && str[1]=='/') {
		 //found comment
		p = strstr(str, "\n");
		if (!p) {
            str = str + strlen(str); //the rest was comment!
            return;
		}
		str = p+1;
		skipWSComment(str);
	}
}

/*! Scan for "<BLAH>". Keeps the <>.
 * Updates str.
 */
static char *scanKeycode(char *&str)
{
	skipWSComment(str);
	if (*str != '<') throw(_("Expected <"));
	char *s = str;
	s = strchr(str, '>');
	if (!s) throw (_("Expected >"));
	char *ss = newnstr(str, s-str+1);
	str = s + 1;
	return ss;
}

static double scanDouble(char *&str)
{
    char *e=nullptr;
	double d = strtod(str, &e);
	if (e == str) throw(_("Expected number"));
	str = e;
	return d;
}

/*! Scan for "blah" or "\"blah\"".
 */
static char *scanString(char *&str)
{
	skipWSComment(str);

	char *e = str;
	if (*str == '"') {
		str++;
        e = strchr(str, '"');
		if (!e) throw(_("Expected \""));
        char *s = newnstr(str, (int)(e-str));
		str = e+1;
		return s;
	}

	while (*e && !isspace(*e)) e++;
    if (e == str) return nullptr;
    char *s = newnstr(str, e-str);
	str = e;
	return s;
}

static ScreenColor scanColor(const char *str)
{
    ScreenColor color;
    if (!strcmp(str, "black")) color.rgbf(0,0,0);
    else if (!strcmp(str, "white")) color.rgbf(1,1,1);
    else if (!strcmp(str, "red")) color.rgbf(1,0,0);
    else if (!strcmp(str, "green")) color.rgbf(0,1,0);
    else if (!strcmp(str, "blue")) color.rgbf(0,0,1);
    else if (!strcmp(str, "orange")) color.rgbf(0,1,1);
    else if (!strcmp(str, "yellow")) color.rgbf(1,1,0);
    else if (!strcmp(str, "purple")) color.rgbf(1,0,1);
    else color.rgbf(0,0,0,1);

    return color;
}

int Keyboard::ImportXkb(const char *file, int index)
{
    char *contents = read_in_whole_file(file, nullptr, 0);
	if (!contents) return 1;

	// structured like:
	//  //.... Comment
	// [default] thing string:label { (stuff) } ;
	// thing.property = (string, number, or block) ;
	// property = (string, number, or block) ;
	//
	// stuff in a shape block is:
	//   { [2,3] },  //a rectangle from (0,0) to (2,3)
	//   { [2,3], [4,5] }  //rectangle from (2,3) to (4,5)
	//   { [2,3], [4,5], [6,7], ... }  //polygon of these points
	//
	// xkb_geometry
	//   shape
	//   solid
	//   indicator
	//   text
	//   section
	//     row
	//       keys

    char *str = contents;
    char *p, *str2;
    //int   nextisdefault = false;
    int   curindex      = -1;

    Keyboard *keyboard = nullptr;
    //KeyShape *shape    = nullptr;
    KeyShape  shape_default;
    //Key *     key = nullptr;
    //	Key key_defaults;
    KeyboardLight  light_default;
    //KeyboardLight *light;
    double         cornerRadius = 0;
    //double         top = 0, left = 0;
    //double         gap = 0;
    double         d;
    ScreenColor    baseColor, labelColor, color;
    //int            err = 0;
    char *         start;

    PtrStack<char> aliases(LISTS_DELETE_Array);

    try {

        start = nullptr;

		while (str && *str) {
			if (start && start == str) {
				throw(_("Couldn't parse more!"));
				break;
			}

			skipWSComment(str);

			if (!strncasecmp(str, "default", 7)) {
				//nextisdefault = true;
				str += 7;
				continue;
			}

			if (!strncasecmp(str, "xkb_geometry", 12)) {
				str += 12;
				skipWSComment(str);
				if (!str) break;

                keyboard = new Keyboard(nullptr, nullptr, nullptr);

				curindex++;

				 //read in keyboard id tag
				if (*str=='"') {
					str2 = scanString(str);
					keyboard->name = str2;
					delete[] str2;
				}
				skipWSComment(str);

				if (*str=='{') {
					str++;
				}

				while (str && *str && *str !='}') {
					skipWSComment(str);

					if (isalpha(*str)) {
						p = str;
						while (isalnum(*p)) p++;
						str = p;

						//----------properties
						if (!strncasecmp(str, "description", 11)) {
							str += 11;
							str2 = scanString(str);
							if (!str2) throw(_("Expected string"));
							keyboard->description = str2;
							delete[] str2;
							
						} else if (!strncasecmp(str, "width", 5)) {
							str += 5;
                            d = scanDouble(str);
                            keyboard->width = d;

						} else if (!strncasecmp(str, "height", 6)) {
							str += 6;
                            d = scanDouble(str);
                            keyboard->height = d;

						} else if (!strncasecmp(str, "top", 3)) {
							str += 3;
                            d = scanDouble(str);
                            shape_default.y = d;

						} else if (!strncasecmp(str, "left", 4)) {
							str += 4;
                            d = scanDouble(str);
                            shape_default.x = d;

						} else if (!strncasecmp(str, "color", 5)) {
							str += 5;
							color = scanColor(str);

						} else if (!strncasecmp(str, "alias", 5)) {
							str += 5; //alias <AE01> = <CAPS>;
							skipWSComment(str);

							str2 = scanKeycode(str);
							aliases.push(str2);

							skipWSComment(str);
							if (*str != '=') throw(_("Expecting ="));
							str++;

							skipWSComment(str);

							str2 = scanKeycode(str);
							aliases.push(str2);


						//----------or objects
						} else if (!strncasecmp(str, "shape", 5)) {
							 //a KeyShape
							str += 5;
							skipWSComment(str);

							if (*str == '.') {
								 //seems like the only one used is "cornerRadius"

								skipWSComment(str);
								if (strncmp(str, "cornerRadius", 12)) throw _("Unknown property");
								str += 12;
								skipWSComment(str);
								if (*str == '=') str++;
								skipWSComment(str);

								cornerRadius = scanDouble(str);
								DBG cerr <<"conrnerRadius: "<<cornerRadius<<endl;

							} else {
								 //shape "Name" {
                                 //    { [ 18,18] },
                                 //    { [2,1], [ 16,16] }
								 //  };
								 //}
								KeyShape *nshape = new KeyShape;
								nshape->x = shape_default.x;
                                nshape->y = shape_default.y;
								nshape->width = shape_default.width;
								nshape->height = shape_default.height;
								nshape->round = shape_default.round;

								//scanShape(str, shape_default);
								skipWSComment(str);
								
								char *id = scanString(str);
								if (id) {
									nshape->name = id;
									delete[] id;
								}

								skipWSComment(str);

                                if (*str == '{') {
                                    //***
                                }
							}


						} else if (!strncasecmp(str, "solid", 5)) {
							 //solid decoration areas:
							 // solid "Name" {
							 //   shape = "LEDS";
							 //   top= 22;
							 //   left= 377;
							 //   color = "grey10"
							 // };
							str += 5;
							skipWSComment(str);

							//char *id =
								scanString(str);

							skipWSComment(str);

                            // ***

						} else if (!strncasecmp(str, "indicator", 9)) {
							 //lights
							str += 9;
							if (*str=='.') {
								str++;
                                //if (!scanNameValue(str, str2, str3)) {
                                //}
							}

                            // ***

						} else if (!strncasecmp(str, "text", 4)) {
							 //text decoration
							str += 4;
                            //***

						} else if (!strncasecmp(str, "row", 3)) {
							 //here should always be props: row.top, row.left, row.vertical
							str += 3;
							if (*str != '.') throw(_("Expected default property"));
                            //***

							skipWSComment(str);

						} else if (!strncasecmp(str, "section", 7)) {
							 //a collection of rows
							str += 7;

                            //***

							if (!strncasecmp(str, "row", 3)) {
							 	//a collection of keys
								str += 3;
                                //***

								if (!strncasecmp(str, "keys", 4)) {
							 		//a collection of keycodes and custom keys
									str += 4;
                                    //***
								}
							}


						}
					}
				}

				if (!str) break;

			} //if xkb_geometry

			//DBG cerr <<" ----kbd eof!"<<endl;
	
		} //while str not done
		
	 } catch (const char *err) {
		 DBG cerr <<" keyboard parsing error: "<<err<<"!"<<endl;
		 return 1;
	 }
	

	delete[] contents;
	return 0;
}



//---------------------------- StandardKeyboard() ----------------------------


//! Define a standard keyboard arrangement.
/*! You can currently pass in "en_dvorak" or "en_qwerty".
 *
 * keystrings is an array of the keyboard characters starting upper left,
 * going left and so on, first the lower case thing, then upper case. For instance,
 * on typical us: "`","~", "1","!", "2","@",...
 *
 * \verbatim
 *
 *   `~   1!  2@  3#  4$  5%  6^  7&  8*  9(  0)  -_  =+   bksp    0..13
 *   tab    q   w   e   r   t   y  u   i   o   p   [{  ]}  \|     14..27
 *   caps    a   s   d   f   g   h   j   k   l   ;:  '"   Enter   28..40
 *   shift    z   x   c   v   b   n   m   ,<  .>  /?      Shift   41..52
 *   cntl meta alt  --------Space--------- alt  meta  menu cntl   53..60
 *
 *   Numlock / * -       61 62 63 64
 *         7 8 9 +       65 66 67 68
 *         4 5 6         69 70 71
 *         1 2 3 Enter   72 73 74 75
 *          0  .         76 77
 * \endverbatim
 */
Keyboard *StandardKeyboard(const char *type)
{
	Keyboard *keyboard;
	const KeyboardSimpleMap *keystrings;

	if (!strcmp(type,"en_dvorak")) {
		keystrings = en_dvorak;
        keyboard   = new Keyboard("English Dvorak","en",nullptr);

	} else if (!strcmp(type,"en_qwerty")) {
		keystrings = en_qwerty;
        keyboard   = new Keyboard("English Qwerty","en",nullptr);

    } else return nullptr;

	int x=0,y=0,w;
	int height=3, singlewidth=3; // note that 1 and 3 could be useful if this implemented in curses
	double maxheight=0, maxwidth=0;
	//int i;
	int ci=0;
	unsigned int ch, upper, other;
	const char *s;
    //Key *key=nullptr;
	int keytype, keytype2, mod;


	 //----------------------- main keyboard
	keyboard->AddInGroup(KEYSECTION_Normal);
	for (int c=0; c<61; c++) {
		 // pre modifications
         if (c == 14 || c == 28 || c == 41 || c == 53) {
             x = 0;
             y += height;
         }

         w = singlewidth;

         if (c == 13) w += singlewidth * 4 / 3;  // on bksp

         if (c == 14) w += singlewidth * 2 / 3;  // on tab key
         if (c == 27) w += singlewidth * 2 / 3;  // on \|

         if (c == 28) w += singlewidth;          // on caps lock
         if (c == 40) w += singlewidth * 4 / 3;  // on enter

         if (c == 41) w += singlewidth * 4 / 3;  // on left shift
         if (c == 52) w += singlewidth * 2;      // on right shift

         if (c == 53) w += singlewidth / 3;  // on last row, l-cntl
         if (c == 54) w += singlewidth / 3;  // on last row, l-meta
         if (c == 55) w += singlewidth / 3;  // on last row, l-alt
         if (c == 56) w += singlewidth * 5;  // on last row, space
         if (c == 57) w += singlewidth / 3;  // on last row, r-alt
         if (c == 58) w += singlewidth / 3;  // on last row, r-meta
         if (c == 59) w += singlewidth / 3;  // on last row, menu
         if (c == 60) w += singlewidth / 3;  // on last row, r-cntl

         ch = upper = other = mod = 0;

         s = keystrings[c].normal;
         if (s && s[0]) ch = strtoChar(s, &other, &keytype, &mod);

         s = keystrings[c].shift;
         if (s && s[0]) upper = strtoChar(s, &other, &keytype2, &mod);

         //i = 
		keyboard->AddKey(x, y, w, height, keystrings[c].finger, keytype,
                              keystrings[c].normal,  // key name string
                              ch, upper, other, mod, keystrings[c].keycode);
         //key = keyboard->key(i);

         x += w;
         ci += 3;

         if (c == 13 || c == 27 || c == 40 || c == 52 || c == 60) {
             if (x > maxwidth) maxwidth            = x;
             if (y + height > maxheight) maxheight = y + height;
		}
	}


	 //-----------------extra control keys
	keyboard->AddInGroup(KEYSECTION_Controls);
	y = -1.5*height;
    w = singlewidth;
	x = maxwidth+singlewidth/2;
//	virtual int AddKey(int x, int y, int w, int h, int finger, int keytype,
//					   const char *name, unsigned int ch, unsigned int upper, unsigned int other,
//					   int mod=0, int keycode=0);
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Print",  0,0,'*', 0,107); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Scroll", 0,0,'*', 0,78);  x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Pause",  0,0,'*', 0,127); x+=singlewidth;
	x=maxwidth+singlewidth/2;
	y=0;
	y+=height;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Ins",  0,0,LAX_Ins,  0,118); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Home", 0,0,LAX_Home, 0,110); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "PgUp", 0,0,LAX_Pgup, 0,112); x+=singlewidth;
	x=maxwidth+singlewidth/2;
	y+=height;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Del",  0,0,LAX_Del,    0,119); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "End",  0,0,LAX_End,    0,115); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "PgDn", 0,0,LAX_Pgdown, 0,117); x+=singlewidth;
	x=maxwidth+singlewidth/2+singlewidth;
	y+=height;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Up",   0,0,LAX_Up,     0,111); x+=singlewidth;
	x=maxwidth+singlewidth/2;
	y+=height;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Left",  0,0,LAX_Left,  0,113); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Down",  0,0,LAX_Down,  0,116); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Right", 0,0,LAX_Right, 0,114); x+=singlewidth;

	maxwidth += 3*singlewidth + singlewidth/2;


	 //-------------------------add on keypad
	keyboard->AddInGroup(KEYSECTION_Keypad);
    x = maxwidth+singlewidth/2;
    y = 0;
    w = singlewidth;
	 //numlock / * -
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_ToggleModifier, "NumLk", 0,0,LAX_Numlock, 0,77); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "/", '/',0,0, 0,106); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "*", '*',0,0, 0,63); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "-", '-',0,0, 0,82);
    y += height;
    x = maxwidth + singlewidth/2;
    // 7 8 9 +
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "7", '7',0,0, 0,79); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "8", '8',0,0, 0,80); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "9", '9',0,0, 0,81); x+=singlewidth;
	keyboard->AddKey(x,y,w,2*height, 4, KEYTYPE_Normal, "+", '+',0,0, 0,86);
	y+=height;
	x=maxwidth+singlewidth/2;
	 //4 5 6
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "4", '4',0,0, 0,83); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "5", '5',0,0, 0,84); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "6", '6',0,0, 0,85); x+=singlewidth;
	y+=height;
	x=maxwidth+singlewidth/2;
	 //1 2 3 Enter
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "1", '1',0,0, 0,87); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "2", '2',0,0, 0,88); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "3", '3',0,0, 0,89); x+=singlewidth;
	keyboard->AddKey(x,y,w,2*height, 4, KEYTYPE_Normal, "Enter", LAX_Enter,0,0, 0,104);
	y+=height;
	x=maxwidth+singlewidth/2;
	 //0 . 
	keyboard->AddKey(x,y,2*w,height, 4, KEYTYPE_Normal, "0", '0',0,0, 0,90); x+=2*singlewidth;
	keyboard->AddKey(x,y,w,height,   4, KEYTYPE_Normal, ".", '.',0,0, 0,91);

	maxwidth+=5*singlewidth;


	 //-----------------esc and function keys
	keyboard->AddInGroup(KEYSECTION_Fkeys);
	y=-1.5*height;
	x=0;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "Esc", 0,0,LAX_Esc, 0,9);  x+=singlewidth;
	x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F1", 0,0,LAX_F1, 0,67); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F2", 0,0,LAX_F2, 0,68); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F3", 0,0,LAX_F3, 0,69); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F4", 0,0,LAX_F4, 0,70); x+=singlewidth;
	x+=singlewidth*2/3;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F5", 0,0,LAX_F5, 0,71); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F6", 0,0,LAX_F6, 0,72); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F7", 0,0,LAX_F7, 0,73); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F8", 0,0,LAX_F8, 0,74); x+=singlewidth;
	x+=singlewidth*2/3;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F9",  0,0,LAX_F9, 0,75);  x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F10", 0,0,LAX_F10, 0,76); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F11", 0,0,LAX_F11, 0,95); x+=singlewidth;
	keyboard->AddKey(x,y,w,height, 4, KEYTYPE_Normal, "F12", 0,0,LAX_F12, 0,96); x+=singlewidth;


	 //define where fingers normally sit
	keyboard->homekeys[0]=29;
	keyboard->homekeys[1]=30;
	keyboard->homekeys[2]=31;
	keyboard->homekeys[3]=32;
	keyboard->homekeys[4]=35;
	keyboard->homekeys[5]=36;
	keyboard->homekeys[6]=37;
	keyboard->homekeys[7]=38;


	 //finalize metrics
	int minx=0,maxx=0,miny=0,maxy=0;
	for (int c=0; c<keyboard->keys.n; c++) {
		if (keyboard->keys.e[c]->position.x<minx) minx=keyboard->keys.e[c]->position.x;
		if (keyboard->keys.e[c]->position.y<miny) miny=keyboard->keys.e[c]->position.y;
		if (keyboard->keys.e[c]->position.x+keyboard->keys.e[c]->position.width>maxx)
			maxx=keyboard->keys.e[c]->position.x+keyboard->keys.e[c]->position.width;
		if (keyboard->keys.e[c]->position.y+keyboard->keys.e[c]->position.height>maxy)
			maxy=keyboard->keys.e[c]->position.y+keyboard->keys.e[c]->position.height;
	}
	for (int c=0; c<keyboard->keys.n; c++) {
		keyboard->keys.e[c]->position.x-=minx;
		keyboard->keys.e[c]->position.y-=miny;
	}
	DBG cerr <<"====keyboard dims x:"<<minx<<':'<<maxx<<"  y:"<<miny<<':'<<maxy<<endl;
	keyboard->basewidth  =keyboard->width  =maxx-minx;
	keyboard->baseheight =keyboard->height =maxy-miny;

	return keyboard;
}

//! Return simple ascii from s, or a LAX_* control key in other (and return 0).
unsigned int strtoChar(const char *s, unsigned int *other, int *keytype, int *mod)
{
	if (!s || !s[0]) { *other=0; return 0; }
	*mod=0;

	if (!strcmp(s,"Shift"))   { *other=LAX_Shift   ; *keytype=KEYTYPE_Modifier;       *mod=ShiftMask;    return 0; }
	if (!strcmp(s,"Control")) { *other=LAX_Control ; *keytype=KEYTYPE_Modifier;       *mod=ControlMask;  return 0; }
	if (!strcmp(s,"Cntl"))    { *other=LAX_Control ; *keytype=KEYTYPE_Modifier;       *mod=ControlMask;  return 0; }
	if (!strcmp(s,"Alt"))     { *other=LAX_Alt     ; *keytype=KEYTYPE_Modifier;       *mod=AltMask;      return 0; }
	if (!strcmp(s,"Meta"))    { *other=LAX_Meta    ; *keytype=KEYTYPE_Modifier;       *mod=MetaMask;     return 0; }
    if (!strcmp(s,"NumLk"))   { *other=LAX_Numlock ; *keytype=KEYTYPE_ToggleModifier; *mod=NumLockMask;  return 0; }
    if (!strcmp(s,"Caps"))    { *other=LAX_Capslock; *keytype=KEYTYPE_ToggleModifier; *mod=CapsLockMask; return 0; }

	if (!strcmp(s,"Space"))   { *other=0          ; *keytype=KEYTYPE_Normal; return (unsigned int)' '; }
	if (!strcmp(s,"Esc"))     { *other=LAX_Esc    ; *keytype=KEYTYPE_Normal; return 0; }
	if (!strcmp(s,"Menu"))    { *other=LAX_Menu   ; *keytype=KEYTYPE_Normal; return 0; }
	if (!strcmp(s,"Pause"))   { *other=LAX_Pause  ; *keytype=KEYTYPE_Normal; return 0; }
	if (!strcmp(s,"Del"))     { *other=LAX_Del    ; *keytype=KEYTYPE_Normal; return 0; }
	if (!strcmp(s,"Bksp"))    { *other=LAX_Bksp   ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Tab"))     { *other=LAX_Tab    ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Ins"))     { *other=LAX_Ins    ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Home"))    { *other=LAX_Home   ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"End"))     { *other=LAX_End    ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Enter"))   { *other=LAX_Enter  ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"PgUp"))    { *other=LAX_Pgup   ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"PgDn"))    { *other=LAX_Pgdown ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F1"))      { *other=LAX_F1     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F2"))      { *other=LAX_F2     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F3"))      { *other=LAX_F3     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F4"))      { *other=LAX_F4     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F5"))      { *other=LAX_F5     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F6"))      { *other=LAX_F6     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F7"))      { *other=LAX_F7     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F8"))      { *other=LAX_F8     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F9"))      { *other=LAX_F9     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F10"))     { *other=LAX_F10    ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F11"))     { *other=LAX_F11    ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"F12"))     { *other=LAX_F12    ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Left"))    { *other=LAX_Left   ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Up"))      { *other=LAX_Up     ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Down"))    { *other=LAX_Down   ; *keytype=KEYTYPE_Normal; return 0; }
    if (!strcmp(s,"Right"))   { *other=LAX_Right  ; *keytype=KEYTYPE_Normal; return 0; }


	*other = 0;
	*keytype = KEYTYPE_Normal;
	return (unsigned int)s[0];
}



//----------------------------- KeyboardWindow ----------------------------------
/*! \class KeyboardWindow
 * \brief Class for a clickable window
 */

KeyboardWindow::KeyboardWindow(anXWindow *parnt, const char *nname, const char *ntitle,
			unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
            anXWindow *prev,unsigned long nowner,const char *nsend,
            const char *kb)
  : anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	send_type = win_style & KBWIN_Send_Fake_Keys;

    keyboard = StandardKeyboard(kb?kb:"en_qwerty");
    //DBG keyboard->ExportSVG("TEST-EXPORT.svg");

    InstallColors(THEME_Panel);

    double th = win_themestyle->normal->textheight();
    gap = th/5;
    margin = th/2;

	WindowTitle(keyboard->Name());

    if (hh == 0) {
        keyboard->height = th * keyboard->baseheight;
        win_h = keyboard->height + 2 * margin;
	}
    if (ww == 0) {
		// *** assumes that one columns column unit is the textheight
        keyboard->width = th *keyboard->basewidth;
        win_w = keyboard->width + 2 * margin;
	}

	currentmods=0;
	showallmaps=1;
    keys_to_move = false;
    force_toggles = true;

    defaultKeyBG.rgbf(1.,1.,1.);
    coloravg(&defaultKeyOutline, &(win_themestyle->bg), &(win_themestyle->fg));
    defaultKeyText = win_themestyle->fg;
}

KeyboardWindow::~KeyboardWindow()
{
	if (keyboard) delete keyboard;
}

int KeyboardWindow::init()
{
    if (keyboard->width == 0) {
        keyboard->width = win_w - 2*margin;
        keyboard->height = win_h - 2*margin;
	}
    return 0;
}

//! Return whether that box has a hover over it according to buttondown.
int KeyboardWindow::hasHover(int c)
{
	int device=0;
	int hover=-1;
	
	do {
		device = buttondown.whichdown(device,HOVER);
		if (!device) return 0;
		hover = -1;
		buttondown.getextrainfo(device,HOVER,&hover);
		if (hover == c) return 1;
	} while (device>0);
	
	return 0;
}

/*! If !currentmods, don't change curModsColor.
 */
void KeyboardWindow::CurrentModsColor(ScreenColor &curModsColor)
{
	if (!currentmods) {
		coloravg(&curModsColor, &defaultKeyBG, &win_themestyle->fg);
		return;
	}

	int l=0;
	double r=0,g=0,b=0;
	KeyModStyle *ms = nullptr;
	const unsigned int mods[] = { ShiftMask, ControlMask, AltMask, MetaMask };
	for (int c=0; c<4; c++) {
		if (currentmods & mods[c])   {
			ms = keyboard->ModStyle(mods[c]);
			if (ms) {
				r += ms->fill.Red();
				g += ms->fill.Green();
				b += ms->fill.Blue();
				l++;
			}
		}
	}
	if (!l) {
		coloravg(&curModsColor, &defaultKeyBG, &defaultKeyText);
		return;
	}
	curModsColor.rgbf(r/l, g/l, b/l);
}

void KeyboardWindow::Refresh()
{
	if (!win_on || !needtodraw) return;
	needtodraw=0;
	if (!keyboard) return;

	Displayer *dp = MakeCurrent();
	dp->font(win_themestyle->normal, win_themestyle->normal->textheight());

	dp->ClearWindow();
	dp->NewFG(win_themestyle->fg);
	dp->LineWidth(1);

	double xs = keyboard->width /keyboard->basewidth;
	double ys = keyboard->height/keyboard->baseheight;
	double th = win_themestyle->normal->textheight();
	double round = th/5;
	ScreenColor bg, fg;
	ScreenColor curModsColor;
	CurrentModsColor(curModsColor);

	Key *key;
	double x,y,x2,y2,w,h;
//	int n, yp14,yp23,yp12;
	int hovered = -1;
	DoubleRectangle hoverfrom;

	for (int c=0; c<keyboard->keys.n; c++) {
		key = keyboard->key(c);
		bg = defaultKeyBG;
		fg = win_themestyle->fg;
		if (key->mod && key->down) {
			KeyModStyle *ms = keyboard->ModStyle(key->mod);
			if (ms) bg = ms->fill;
		}

		x  = margin + keyboard->x+key->position.x*xs;
		y  = margin + keyboard->y+key->position.y*ys;
		x2 = margin + (keyboard->x+key->position.x+key->position.width)*xs;
		y2 = margin + (keyboard->y+key->position.y+key->position.height)*ys;
		w  = x2-x;
		h  = y2-y;

		if (w > gap) w -= gap;
		if (h > gap) h -= gap;

        if (round > h/5) round = h/5;

		if (key->tag) {
			bg = curModsColor;
		} else if (key->down && !key->mod) {
			//default key down color
            coloravg(&bg, &bg, &win_themestyle->fg);
		}
		if (hasHover(c)) {
			hovered = c;
			hoverfrom.set(x,y,w,h);
			//add a bit of color when hovered
			bg.AddDiff(.1,.1,.1);
            //coloravg(&bg, &bg, &win_themestyle->fg, .3);
        }
		dp->NewBG(bg);

		dp->NewFG(coloravg(win_themestyle->bg, win_themestyle->fg));
		//dp->drawrectangle(x,y,w,h, 0);
		dp->drawRoundedRect(x,y,w,h, round,false, round,false, 2);

		StandoutColor(bg, true, fg);
		dp->NewFG(fg);
		DrawKeyText(key, currentmods, x,y,w,h);

	}

	if (hovered >= 0) DrawMouseOverTip(keyboard->keys[hovered], hoverfrom.x, hoverfrom.y, hoverfrom.width, hoverfrom.height);
	PostRefresh(dp);

	DBG cerr <<endl;
	SwapBuffers();
}

void KeyboardWindow::PostRefresh(Displayer *dp) {}

//void KeyboardWindow::DrawKey(Key *key, int mods, double x,double y,double w,double h)
//{
//	dp->NewBG(bg);
//
//	dp->NewFG(coloravg(win_themestyle->bg, win_themestyle->fg));
//	//dp->drawrectangle(x,y,w,h, 0);
//	dp->drawRoundedRect(x,y,w,h, round,false, round,false, 2);
//
//	StandoutColor(bg, true, fg);
//	dp->NewFG(fg);
//	DrawKeyText(key, currentmods, x,y,w,h);
//}

static void DrawKeyTextSquished(Displayer *dp, const char *str, double x, double y, double w)
{
	double ww = dp->textextent(str, -1, nullptr,nullptr);
	w *= .8;
	if (ww >= w) {
		dp->PushAxes();
		dp->Zoomr(w/ww, flatpoint(x,y));
	}
	dp->textout(x,y, str,-1, LAX_CENTER);
	if (ww >= w) dp->PopAxes();
}


/*! Write out the key text.
 */
void KeyboardWindow::DrawKeyText(Key *key, int mods, double x,double y,double w,double h)
{
	Displayer *dp = GetDisplayer();

	if (mods == 0) {
		int n = key->keymaps.n;
		char buffer[20];

		if ((win_style & KBWIN_Show_Double_Row) && n>1) {
			//sloppy here, only works when there are only two keymaps
			for (int c2=0; c2<n; c2++) {
				sprintf(buffer,"%c",key->keymaps.e[c2]->ch);

				if (key->keymaps.e[c2]->mods&ShiftMask)
					dp->textout(x+w/2,y+h*1/4, buffer,-1, LAX_CENTER);
				else
					dp->textout(x+w/2,y+h*3/4, buffer,-1, LAX_CENTER);
			}
		} else {
			//DBG cerr << " mods"<<mods;
			const char *str = key->keymaps.e[0]->name.c_str();
			//-----------------
			DrawKeyTextSquished(dp, str, x+w/2,y+h/2, w);
			//-----------------
//			double ww = dp->textextent(str, -1, nullptr,nullptr);
//			if (ww >= w) {
//				dp->PushAxes();
//				dp->Zoomr(w/ww, flatpoint(x,y));
//			}
//			dp->textout(x+w/2,y+h/2, str,-1, LAX_CENTER);
//			if (ww >= w) dp->PopAxes();
		}

	} else { //we are pressing down modifier keys
		//DBG cerr << " mods"<<mods;
		Keymap *map = key->MatchMods(mods);
		//dp->textout(x+w/2,y+h/2, map->name.c_str(),-1, LAX_CENTER);
		DrawKeyTextSquished(dp, map->name.c_str(), x+w/2,y+h/2, w);
	}
}

/*! Return the index in keyboard->keys of the key under (x,y), or -1 for none.
 */
int KeyboardWindow::scan(int xx,int yy)
{
	if (!keyboard) return -1;

	double xs = keyboard->width /keyboard->basewidth;
	double ys = keyboard->height/keyboard->baseheight;
	double x,y;
	x = (xx-keyboard->x)/xs;
	y = (yy-keyboard->y)/ys;
	//DBG cerr <<"scan "<<x<<','<<y<<"  bh:"<<keyboard->baseheight<<"  h:"<<keyboard->height<<endl;

	Key *key;
	for (int c=0; c<keyboard->keys.n; c++) {
		key=keyboard->key(c);
		//DBG if (c<3) cerr <<"bounds: " <<key->position.x<<',' << key->position.y<<"  "<< key->position.x+key->position.width<<" x "<< key->position.y+key->position.height<<endl;
		if (x>=key->position.x
			 && y>=key->position.y
			 && x<key->position.x+key->position.width
			 && y<key->position.y+key->position.height)
		  return c;
	}

	return -1;
}

//! Return which (normal) character should be sent with the given mods.
/*! Note that ONLY normal keys send characters, not modifier keys.
 */
unsigned int KeyboardWindow::CurrentChar(Key *key,int mods)
{
	if (key->keytype!=KEYTYPE_Normal) return 0;
	for (int c2=0; c2<key->keymaps.n; c2++) {
		if (key->keymaps.n>1 && key->keymaps.e[c2]->mods!=currentmods) continue;

		return key->keymaps.e[c2]->ch;
	}

	 // if no mods match, return the first key mapping
	if (key->keymaps.n) return key->keymaps.e[0]->ch;

	return 0;
}

/*! Send SimpleMessage with info 1,2,3: ifdown, key, mods.
 */
void KeyboardWindow::send(bool down, unsigned int key, unsigned int mods)
{
	if (!win_owner) return;
	SimpleMessage *ev = new SimpleMessage(nullptr, down,key,mods,0, win_sendthis, object_id, win_owner);
	app->SendMessage(ev);
}

int KeyboardWindow::sendFakeKeyEvent(unsigned int key, unsigned int mods)
{
	if (!win_owner) return 1;

	anXWindow *target=app->findwindow_by_id(win_owner);
	if (!target) return 2;

	KeyEventData *k = new KeyEventData(LAX_onKeyDown);
	k->propagate = 0;
	k->to        = win_owner;
	k->from      = object_id;
	k->target    = target;
	k->device    = &dummydevice;
    k->buffer    = nullptr;
	k->len       = 0;
	k->key       = key;
	k->modifiers = mods;
	app->SendMessage(k);

	k = new KeyEventData(LAX_onKeyUp);
	k->propagate = 0;
	k->to        = win_owner;
	k->from      = object_id;
	k->target    = target;
	k->device    = &dummydevice;
    k->buffer    = nullptr;
	k->len       = 0;
	k->key       = key;
	k->modifiers = mods;
	app->SendMessage(k);

	return 0;
}

int KeyboardWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	return 1;
}

int KeyboardWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	MenuInfo *menu = new MenuInfo();
    menu->AddToggleItem(_("Force toggles"), nullptr, 0,0, KMENU_Force_Toggles);
	menu->AddSep();

	menu->AddItem("en_qwerty", KMENU_Select_Keyboard,0, 0);
	menu->AddItem("en_dvorak", KMENU_Select_Keyboard,0, 1);
	menu->AddItem(_("Load keyboard..."), KMENU_Load_Keyboard);

	return 1;
}

int KeyboardWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	int i = scan(x,y);
	buttondown.down(d->id,LEFTBUTTON, x,y, i);

	if (i>=0) {
		Key *key = keyboard->keys.e[i];
		//bool wasdown = (key->down == 0);

		if (key->keytype == KEYTYPE_ToggleModifier
		    || (key->keytype == KEYTYPE_Modifier && force_toggles)
				) { //like caps lock

			if (key->down > 0) key->down = 0;
			else key->down++;

		} else key->down++;

		if (key->mod) {
			if (key->down)
				 currentmods |=  key->mod;
			else currentmods &= ~key->mod;
			DBG cerr << "currentmods: "<<currentmods<<endl;
		}

		Keymap *keymap = key->MatchMods(currentmods);
		if (keymap) send(key->down > 0, keymap->ch, currentmods);
	}
	needtodraw=1;
	return 0;
}

int KeyboardWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 0;

	int i;
	int dragged = buttondown.up(d->id,LEFTBUTTON, &i);
	DBG cerr <<"********dragged:"<<dragged<<endl;
	int ii = scan(x,y);

	if (i<0 || i != ii) return 0;


	if (send_type == KBWIN_Send_Fake_Keys && win_owner && keyboard->keys.e[i]->keytype == KEYTYPE_Normal) {
		sendFakeKeyEvent(CurrentChar(keyboard->keys.e[i],currentmods),currentmods);
	}

	Key *key = keyboard->keys.e[i];

	if (!(key->keytype == KEYTYPE_ToggleModifier
		    || (key->keytype == KEYTYPE_Modifier && force_toggles)
		 )) {
		 //only key up on not toggles
		key->down--;

		if (key->down <= 0) {
			key->down = 0;
			if (key->mod) {
				currentmods &= ~key->mod;
				DBG cerr <<"=====newmod:"<<currentmods<<endl;
			}
		}
	}

	Keymap *keymap = key->MatchMods(currentmods);
	if (keymap) send(key->down > 0, keymap->ch, currentmods);

//	if (!dragged) {
//		if (keyboard->keys.e[i]->togglemod)
//	}

	needtodraw=1;
	return 0;
}

int KeyboardWindow::ShiftWindow(int dx,int dy)
{
	if (!dx && !dy) return 0;

	 //Try 2: XReconfigureWMWindow
	XWindowChanges changes;

	//int offx,offy; //offset from decorations
    //translate_window_coordinates(this,0,0, nullptr,&offx,&offy, nullptr);
	//DBG cerr <<"old origin: "<<offx<<","<<offy<<endl;

	//changes.x=offx + 1;
	//changes.y=offy + 1;
	//changes.x=offx + x-oldx;
	//changes.y=offy + y-oldy;
	changes.x=win_x + dx - 3; //how to automatically find window decoration dimensions!?!??!?!?!?
	changes.y=win_y + dy - 25;
	changes.width=win_w;
	changes.height=win_h;

	int mask=CWX|CWY;
	//int mask=CWX|CWY|CWHeight|CWWidth;
	
	XReconfigureWMWindow(app->dpy,xlib_window,0,mask,&changes);

//	--------------------------
//  //Try 1: _NET_WM_MOVERESIZE
//  window = window to be moved or resized
//  message_type = _NET_WM_MOVERESIZE
//  format = 32
//  data.l[0] = x_root 
//  data.l[1] = y_root
//  data.l[2] = direction
//  data.l[3] = button
//  data.l[4] = source indication
//
//	XClientMessage mes;
//	mes.window=xlib_window;
//	mes.message_type=XInternAtom("_NET_WM_MOVERESIZE");
//	mes.format=32;
//	mes.data.l[0] = 
//	mes.data.l[1] = 
//	mes.data.l[2] = 3;
//	mes.data.l[3] = 0;
//	mes.data.l[4] = 1;
//	------------------

	 //Try 3: _NET_MOVERESIZE_WINDOW
	//  window = window to be moved or resized
	//  message_type = _NET_MOVERESIZE_WINDOW
	//  format = 32
	//  data.l[0] = gravity and flags 
	//  data.l[1] = x 
	//  data.l[2] = y
	//  data.l[3] = width
	//  data.l[4] = height

//	XEvent mes;
//	mes.xclient.type=ClientMessage;
//	mes.xclient.display=app->dpy;
//	mes.xclient.window = xlib_window;
//	mes.xclient.message_type = XInternAtom(app->dpy,"_NET_MOVERESIZE_WINDOW",False);
//	mes.xclient.format = 32;
//	mes.xclient.data.l[0] = 10 | (1<<12) | ((1+2)<<8); //static gravity | app sent | x and y present
//	mes.xclient.data.l[1] = win_x+dx;
//	mes.xclient.data.l[2] = win_y+dy;
//	mes.xclient.data.l[3] = win_w;
//	mes.xclient.data.l[4] = win_h;
//
//	XSendEvent(app->dpy,DefaultRootWindow(app->dpy),False,0,&mes);
//	-----------------------------

	return 0;
}

int KeyboardWindow::MouseMove(int x,int y,unsigned int state, const LaxMouse *m)
{
	int oldx, oldy;
	buttondown.move(m->id, x,y, &oldx,&oldy);
	DBG cerr <<" old:"<<oldx<<','<<oldy<<" new:"<<x<<','<<y<<endl;

	DBG if (buttondown.isdown(m->id,LEFTBUTTON)) {
	DBG 	cerr <<"dragged: "<<buttondown.isdragged(m->id,LEFTBUTTON)<<endl;
	DBG }
	//DBG int offx,offy; //offset from decorations
    //DBG translate_window_coordinates(this,0,0, nullptr,&offx,&offy, nullptr);
	//DBG cerr <<"old origin: "<<offx<<","<<offy<<"  actual:"<<win_x<<','<<win_y<<endl;

	if ((win_style&KBWIN_RESIZEABLE) && buttondown.isdown(0,LEFTBUTTON)==1 && buttondown.isdown(m->id,LEFTBUTTON)) {
		 //Move on single mouse drag
		 //**** need to do a configure request to move top level windows from the inside
		int key=-1;
		buttondown.getextrainfo(m->id,LEFTBUTTON, &key);
		if (key>=0) return 0;

		buttondown.getinitial(m->id, LEFTBUTTON, &oldx,&oldy);
		buttondown.move(m->id, oldx,oldy);

		if (win_parent) {
			MoveResize(win_x+x-oldx, win_y+y-oldy, win_w,win_h);

		} else {
			buttondown.move(m->id, -999999,-999999); //to force a dragged value
			buttondown.move(m->id, oldx,oldy);
			ShiftWindow(x-oldx,y-oldy);
		}

		needtodraw=1;
		return 0;
	}

	if (buttondown.isdown(0,LEFTBUTTON)==2 && buttondown.isdown(m->id,LEFTBUTTON)) {
		 //2 buttons down, we are scaling the keyboard
		int dev1=buttondown.whichdown(0),
			dev2=buttondown.whichdown(dev1);
//		int dev1=buttondown.whichdown(0,LEFTBUTTON),
//			dev2=buttondown.whichdown(dev1,LEFTBUTTON);
		int oldx1, oldy1;
		if (dev1==m->id) { int t=dev1; dev1=dev2; dev2=t; }
		if (dev2!=m->id) return 0;
		buttondown.getcurrent(dev1, LEFTBUTTON, &oldx1, &oldy1); //<- a constant point

		double scalex=1, scaley=1;
		if (oldx!=oldx1) scalex=abs((x-oldx1)/(double)(oldx-oldx1));
		if (oldy!=oldy1) scaley=abs((y-oldy1)/(double)(oldy-oldy1));

		int screenw, screenh, neww, newh;
        app->ScreenInfo(0, nullptr,nullptr, &screenw,&screenh, nullptr,nullptr, nullptr,nullptr);
		neww=win_w*scalex;
		if (neww>screenw) neww=screenw;
		newh=win_h*scaley;
		if (newh>screenh) newh=screenh;

//		if (!win_parent) PleaseResize(win_x,win_y,neww,newh);
//		else Resize(neww,newh); //*** fail! need to scale AND reposition
		return 0;
	}

	int i = scan(x,y);
	DBG cerr << "scan: " << i << endl;
	int oldhover = -1;
	buttondown.getextrainfo(m->id, HOVER, &oldhover);
	if (i != oldhover) {
		buttondown.down(m->id, HOVER, x, y, i);
		UpdateMouseOver(i);
		needtodraw = 1;
		DBG cerr << "  HOVER over " << i << endl;
	}
	return 0;
}

int KeyboardWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	int c=anXWindow::MoveResize(nx,ny,nw,nh);
    keyboard->width  = win_w - 2*margin;
    keyboard->height = win_h - 2*margin;
	return c;
}

int KeyboardWindow::Resize(int nw,int nh)
{
	int c=anXWindow::Resize(nw,nh);
    keyboard->width  = win_w - 2*margin;
    keyboard->height = win_h - 2*margin;
	return c;
}

int KeyboardWindow::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"************event: "<<data->type<<endl;

	if (data->type == LAX_onMouseOut) {
		const EnterExitData *e=dynamic_cast<const EnterExitData *>(data);
		buttondown.up(e->device->id,HOVER);
		needtodraw=1;

	} else if (data->type == LAX_onKeyDown) {
		const KeyEventData *kev = dynamic_cast<const KeyEventData*>(data);
		currentmods = kev->modifiers & (ShiftMask|ControlMask|MetaMask|AltMask);

		Keymap *keymap = nullptr;
		unsigned int mods = currentmods;
		if (kev->key == LAX_Shift || kev->key == LAX_Control || kev->key == LAX_Meta || kev->key == LAX_Alt) mods = 0;
		DBG cerr <<"    keyboardwindow keycode: "<<kev->keycode<<" for "<<kev->key<<"  curmods: "<<currentmods<<" mods: "<<mods<<endl;

		Key *key = keyboard->FindKey(kev->keycode, kev->key, mods, &keymap);
		if (key) {
			DBG cerr <<"     found key: "<<keymap->ch<<" mod: " <<key->mod<<endl;
			if (key->mod) currentmods |= key->mod;
			key->down++;
			needtodraw=1;
		}
		DBG else cerr <<"    didn't find keycode "<<kev->keycode<<endl;

		//Keymap *keymap = key->MatchMods(currentmods);
		if (keymap) send(key->down > 0, keymap->ch, currentmods);

	} else if (data->type == LAX_onKeyUp) {
		const KeyEventData *kev = dynamic_cast<const KeyEventData*>(data);
		currentmods = kev->modifiers & (ShiftMask|ControlMask|MetaMask|AltMask);
		DBG cerr <<"    keyboardwindow keycode: "<<kev->keycode<<" for "<<kev->key<<"  mods: "<<currentmods<<endl;

		Keymap *keymap = nullptr;
		unsigned int mods = currentmods;
		if (kev->key == LAX_Shift || kev->key == LAX_Control || kev->key == LAX_Meta || kev->key == LAX_Alt) mods = 0;
		Key *key = keyboard->FindKey(kev->keycode, kev->key, mods, &keymap);
		if (key) {
			//key->down--;
			if (key->mod) currentmods &= ~key->mod;
			key->down = 0;
			needtodraw=1;
		}

		//Keymap *keymap = key->MatchMods(currentmods);
		if (keymap) send(key->down > 0, keymap->ch, currentmods);
	}

	return anXWindow::Event(data,mes);
}

int KeyboardWindow::KeyUp(unsigned int ch,unsigned int state, const LaxKeyboard *kb)
{

	return 1;
}

int KeyboardWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{

//	if (keys_to_move) {
//		if (ch==LAX_Up) {
//			ShiftWindow(0,-2);
//			return 0;
//		} else if (ch==LAX_Down) {
//			ShiftWindow(0,2);
//			return 0;
//		} else if (ch==LAX_Left) {
//			ShiftWindow(-2,0);
//			return 0;
//		} else if (ch==LAX_Right) {
//			ShiftWindow(2,0);
//			return 0;
//		}
//
//	} else if (ch=='t') {
//		force_toggles = !force_toggles;
//	}

	return anXWindow::CharInput(ch,buffer,len,state,kb);
}




} // namespace Laxkit



