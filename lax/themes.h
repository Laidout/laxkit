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
//    Copyright (C) 2018 by Tom Lechner
//
#ifndef _LAX_THEMES_H
#define _LAX_THEMES_H

#include <lax/anobject.h>
#include <lax/screencolor.h>
#include <lax/dump.h>
#include <lax/fontmanager.h>


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
	THEME_Hover_Add = 30000,
	THEME_Highlight_Add,
	THEME_Focus_Add,
	THEME_Alternate_Add,

	THEME_Border_Width,
	THEME_Padx,
	THEME_Pady,
	THEME_Bevel,

	THEME_Firstclk,
	THEME_Dblclk,
	THEME_Idleclk,

	 //fonts
	THEME_Font_Normal = 40000,
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

	//NumStack<ScreenColor> extra_colors;


	 //---fonts
	LaxFont *normal;
	LaxFont *bold;
	LaxFont *italic;
	LaxFont *monospace;
	//RefPtrStack<LaxFont> extra_fonts; //one font each for: normal, bold, italic, monospace


	 //---other
	//std::map<string, string> extra_values;


	WindowStyle(int ncategory = THEME_Panel);
    WindowStyle(const WindowStyle &l);
    WindowStyle &operator=(WindowStyle &l);
	virtual ~WindowStyle();

    WindowStyle *duplicate();

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

	virtual int SetDefaultColors(const char *type);
	virtual int SetFonts(LaxFont *nnormal, LaxFont *nbold, LaxFont *nitalic, LaxFont *nmonospace);
	virtual int NormalFont(LaxFont *nfont);
	virtual int BoldFont(LaxFont *nfont);
	virtual int ItalicFont(LaxFont *nfont);
	virtual int MonospaceFont(LaxFont *nfont);
	virtual double GetValue(const char *what);
};


//----------------------------- Theme ---------------------------------------

class Theme : public anObject, public LaxFiles::DumpUtility
{
  public:
	char *name; //localized. script name is usual Id()
	char *filename;

	double default_border_width;
	double default_padx;
	double default_pady;
	double default_bevel;

	unsigned int firstclk; //how long after first click to wait before idle "clicks", in ms
	unsigned int dblclk;   //time between clicks that counts as a double click, in ms
	unsigned int idleclk;  //time between idle fake clicks, in ms

	int tooltips;

	 //colors
	RefPtrStack<WindowStyle> styles; //panel, edit, menu, button

	 //icons
	PtrStack<char> icon_dirs;


	Theme(const char *nname=NULL);
	virtual ~Theme();

	virtual WindowStyle *GetStyle(int category);
	virtual int GetInt(int what);
	virtual double GetDouble(int what);
	virtual ScreenColor *GetScreenColor(int category, int what);
	virtual unsigned long GetColor(int category, int what);
	virtual LaxFont *GetFont(int category, int what);
	virtual LaxImage *GetIcon(const char *key);
	virtual int AddDefaults(const char *which=NULL);

	virtual void       dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

};

//----------------------------- helpers ---------------------------------------
const char *window_category_name(int category);


} //namespace Laxkit

#endif

