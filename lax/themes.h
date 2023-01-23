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
//    Copyright (C) 2018 by Tom Lechner
//
#ifndef _LAX_THEMES_H
#define _LAX_THEMES_H

#include <lax/anobject.h>
#include <lax/screencolor.h>
#include <lax/dump.h>
#include <lax/fontmanager.h>
#include <lax/iconmanager.h>


namespace Laxkit {

//------------------------------- Themes ---------------------------------

enum ThemeThings {
	THEME_None = 0,

	 //builtin theme types
	THEME_Light = 1,
	THEME_Dark,
	THEME_Gray,

	 //categories
	THEME_Panel = 10000,
	THEME_Edit,
	THEME_Menu,
	THEME_Button,
	THEME_Tooltip,
	THEME_CATEGORY_MAX,

 	 //colors
	THEME_COLOR_FIRST = 20000,
	THEME_FG,
	THEME_FG_Hover,
	THEME_FG_Highlight,
	THEME_FG_Gray,
	THEME_BG,
	THEME_BG_Hover,
	THEME_BG_Highlight,
	THEME_BG_Gray,
	THEME_Color1,
	THEME_Color2,
	THEME_Activate,
	THEME_Deactivate,
	THEME_Border_Active,
	THEME_Border_Inactive,
	THEME_COLOR_LAST,

	 //values
	THEME_VALUES_FIRST = 30000,
	THEME_Diff_Hover_Add,
	THEME_Highlight_Add,
	THEME_Focus_Add,
	THEME_Alternate_Add,
	THEME_UI_Scale,

	THEME_Border_Width,
	THEME_Padx,
	THEME_Pady,
	THEME_Bevel,

	THEME_Firstclk,
	THEME_Dblclk,
	THEME_Idleclk,

	THEME_Diff_BG_Hover,
	THEME_Diff_BG_Highlight,
	THEME_Diff_BG_Focus,
	THEME_Diff_BG_Alternate,
	THEME_Diff_FG_Hover,
	THEME_Diff_FG_Highlight,
	THEME_Diff_FG_Focus,
	THEME_Diff_FG_Alternate,
	THEME_Diff_Shadow,
	THEME_Diff_Highlight,
	THEME_VALUES_LAST,

	 //fonts
	THEME_FONT_FIRST = 40000,
	THEME_Font_Normal,
	THEME_Font_Bold,
	THEME_Font_Italic,
	THEME_Font_Monospace,
	THEME_Font_Edit,
	THEME_FONT_MAX,

	 //assorted values

	THEME_MAX
};

//----------------------------- WindowStyle ---------------------------------------

class WindowStyle : public anObject
{
 public:
	int category; // usually one of: THEME_Panel, THEME_Edit, THEME_Menu, THEME_Button, THEME_Tooltip


	 //color adjustments for different states
	double diff_bg_hover;     //-1..1 to add to default color on mouse hovering
	double diff_bg_highlight; //-1..1 to add to default color on item highlighted, as in a menu
	double diff_bg_focus;     //-1..1 to add to default color on item keyboard focused
	double diff_bg_alternate; //-1..1 to add to default color on alternate rows in certain edits
	double diff_fg_hover;     //-1..1 to add to default color on mouse hovering
	double diff_fg_highlight; //-1..1 to add to default color on item highlighted, as in a menu
	double diff_fg_focus;     //-1..1 to add to default color on item keyboard focused
	double diff_fg_alternate; //-1..1 to add to default color on alternate rows in certain edits

	double diff_shadow;    //-1..1
	double diff_highlight; //-1..1

    ScreenColor fg;
    ScreenColor fghover; //if both hovered and highlighted, use highlighted first (and apply the diffs)
    ScreenColor fghl;
    ScreenColor fggray;
    ScreenColor bg;
    ScreenColor bghover;
    ScreenColor bghl;
    ScreenColor bggray;
    ScreenColor color1;
    ScreenColor color2;
    ScreenColor activate;  //usually green for go
    ScreenColor deactivate;//usually red for stop

    ScreenColor active_border;
    ScreenColor inactive_border;
	double border_width;

	//NumStack<ScreenColor> extra_colors;


	 //---fonts
	LaxFont *normal;
	LaxFont *bold;
	LaxFont *italic;
	LaxFont *monospace;
	//RefPtrStack<LaxFont> extra_fonts; //one font each for: normal, bold, italic, monospace


	 //---other
	//std::map<string, anObject*> extra_values;
	AttributeObject extra;


	WindowStyle(int ncategory = THEME_Panel);
    WindowStyle(const WindowStyle &l);
    WindowStyle &operator=(WindowStyle &l);
	virtual ~WindowStyle();
	virtual const char *whattype() { return "WindowStyle"; }

    WindowStyle *duplicate();

	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);

	virtual int SetDefaultColors(const char *type);
	virtual int SetFonts(LaxFont *nnormal, LaxFont *nbold, LaxFont *nitalic, LaxFont *nmonospace);
	virtual int NormalFont(LaxFont *nfont);
	virtual int BoldFont(LaxFont *nfont);
	virtual int ItalicFont(LaxFont *nfont);
	virtual int MonospaceFont(LaxFont *nfont);
	virtual double GetValue(const char *what);
	virtual double GetValue(ThemeThings what);
};


//----------------------------- Theme ---------------------------------------

class Theme : public anObject, public DumpUtility
{
  public:
	char *name; //localized. script name is usual Id()
	char *filename;

	double default_border_width;
	double default_padx;
	double default_pady;
	double default_bevel;
	double base_font_size;
	double ui_scale;
	double ui_default_ppi;

	unsigned int firstclk; //how long after first click to wait before idle "clicks", in ms
	unsigned int dblclk;   //time between clicks that counts as a double click, in ms
	unsigned int idleclk;  //time between idle fake clicks, in ms

	int tooltips;

	 //colors
	RefPtrStack<WindowStyle> styles; //panel, edit, menu, button

	 //icons
	IconManager *iconmanager;

	AttributeObject extra;

	Theme(const char *nname=NULL);
	virtual ~Theme();
	virtual const char *whattype() { return "Theme"; }

	static Theme *DefaultTheme(const char *themename);

	virtual WindowStyle *GetStyle(int category);
	virtual int GetInt(int what);
	virtual double GetDouble(int what);
	virtual ScreenColor *GetScreenColor(int category, int what);
	virtual unsigned long GetColor(int category, int what);
	virtual LaxFont *GetFont(int category, int what);
	virtual LaxImage *GetIcon(const char *key);
	virtual int AddDefaults(const char *which=NULL);
	virtual int UpdateFontSizes();

	virtual void       dump_out(FILE *f,int indent,int what,DumpContext *context);
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);

};

//----------------------------- helpers ---------------------------------------
const char *window_category_name(int category);


} //namespace Laxkit

#endif
