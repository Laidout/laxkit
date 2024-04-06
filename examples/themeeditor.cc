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
//    Copyright (C) 2024 by Tom Lechner
//


#include <lax/scroller.h>
#include <lax/menuinfo.h>
#include <lax/menubutton.h>
#include <lax/popupmenu.h>
#include <lax/rulerwin.h>
#include <lax/dateselector.h>
#include <lax/treeselector.h>
#include <lax/colorbox.h>
#include <lax/checkbox.h>
#include <lax/messagebar.h>
#include <lax/messagebox.h>
#include <lax/laxutils.h>
#include <lax/buttondowninfo.h>
#include <lax/colorsliders.h>
#include <lax/lineinput.h>
#include <lax/button.h>
#include <lax/fileutils.h>
#include <lax/sliderpopup.h>
#include <lax/fontdialog.h>
#include <lax/filedialog.h>
#include <lax/scrolledwindow.h>
#include <lax/tabframe.h>
#include <lax/themes.h>
#include <lax/utf8string.h>
#include <lax/version.h>
#include <lax/language.h>

#include <lax/debug.h>


#include <string>

using namespace std;
using namespace Laxkit;


// -------- TO DO ----------
//
// Fonts
// undo
// dnd colors
//
//  [Name __________]  Save.. Load..
//  [colors           ]
//  [strips][ mockup  ]
//  [icons]
//
// Export to other systems, like Gimp, Krita, etc
// Import from other systems
//
// Base Colors | Color Diffs | Fonts | Metrics | Icons
//
// 
// click drag to select multiples
// click shift drag to add to current selection
// click shift to add/remove under mouse to current selection
//
// click to activate color/value/font/icon_dir editor
//


//#define DBG


const int default_colors[] = {
		THEME_FG,
		THEME_BG,
		THEME_FG_Hover,
		THEME_BG_Hover,
		THEME_FG_Highlight,
		THEME_BG_Highlight,
		THEME_FG_Gray,
		THEME_BG_Gray,
		THEME_Color1,
		THEME_Color2,
		THEME_Activate,
		THEME_Deactivate,
		THEME_Border_Active,
		THEME_Border_Inactive,
		0
	};
const char *default_color_names[] = {
		"Foreground",
		"Background",
		"Foreground Hover",
		"Background Hover",
		"Foreground Highlight",
		"Background Highlight",
		"Foreground Gray",
		"Background Gray",
		"Color1",
		"Color2",
		"Activate",
		"Deactivate",
		"Border Active",
		"Border Inactive",
		nullptr
	};
const char *short_color_names[] = {
		"fg",
		"bg",
		"fgmo",
		"bgmo",
		"fgh",
		"bgh",
		"fggray",
		"bggray",
		"color1",
		"color2",
		"yes",
		"no",
		"active",
		"inactive",
		nullptr
	};

const char *diffs[] = {
		"diff_bg_hover"    ,
		"diff_bg_highlight",
		"diff_bg_focus"    ,
		"diff_bg_alternate",
		"diff_fg_hover"    ,
		"diff_fg_highlight",
		"diff_fg_focus"    ,
		"diff_fg_alternate",
		"diff_shadow"      ,
		"diff_highlight"   ,
		nullptr
	};


//------------------------------- ThemeEditor ---------------------------------

//redefine ColorBox to intercept onMouseIn
class ColorBox2 : public ColorBox
{
  public:
	ColorBox2(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
             int nx,int ny,int nw,int nh,int brder,
             anXWindow *prev,unsigned long owner,const char *mes,
             int ctype, double nstep,
             double c0,double c1,double c2,double c3=-1,double c4=-1);
	virtual int Event(const EventData *e,const char *mes);
};

ColorBox2::ColorBox2(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
             int nx,int ny,int nw,int nh,int brder,
             anXWindow *prev,unsigned long owner,const char *mes,
             int ctype, double nstep,
             double c0,double c1,double c2,double c3,double c4)
	: ColorBox(parnt,nname,ntitle,nstyle, nx,ny,nw,nh,brder, prev,owner,mes, ctype,nstep,c0,c1,c2,c3,c4,nullptr)
{
}

int ColorBox2::Event(const EventData *e,const char *mes)
{   
    switch (e->type) {
        case LAX_onMouseIn: {
            //const EnterExitData *ee=dynamic_cast<const EnterExitData *>(e);

			SimpleMessage *eee=new SimpleMessage(win_title, 0,0,0,0, "mo");
			DBG cerr <<"ColorBox2: "<<WindowTitle()<<"  send to "<<win_owner<<endl;
			app->SendMessage(eee,win_owner,"mo",object_id);

        } break;
	}

	return ColorBox::Event(e,mes);
}

class YesNo : public anXWindow
{
  public:
	YesNo(anXWindow *parent) : anXWindow(parent, "yesno","yesno", 0, 0,0,0,0,1, nullptr,0,nullptr)
	{
		InstallColors(THEME_Panel);
	}

	virtual void Refresh() {
		if (!needtodraw) return;
		needtodraw = 0;

		Displayer *dp = MakeCurrent();
		dp->ClearWindow();
		dp->font(win_themestyle->normal, UIScale() * win_themestyle->normal->textheight());

		dp->NewFG(win_themestyle->activate);
		dp->drawcircle(win_w/4,win_h/2, win_w/7, 1);
		dp->NewFG(win_themestyle->fg);
		dp->textout(win_w/4,win_h/2, "Yes",-1, LAX_CENTER);

		dp->NewFG(win_themestyle->deactivate);
		dp->drawcircle(win_w*3/4,win_h/2, win_w/7, 1);
		dp->NewFG(win_themestyle->fg);
		dp->textout(win_w*3/4,win_h/2, "No",-1, LAX_CENTER);
	}
};

//---------------------------------------- ColorArea -----------------------------------

class ColorArea : public SquishyBox
{
  public:
  	Utf8String name; //for tooltip
	ScreenColor color;
	int category;
	int which;
	bool selected;

	ColorArea(ScreenColor *col, int cat, int nwhich) {
		color = *col;
		category = cat;
		which = nwhich;
		selected = false;
	}
};

//---------------------------------------- TableEditor -----------------------------------

class TableEditor : public RowFrame
{
  public:
  	PtrStack<char> rowlabels;
  	PtrStack<char> columnlabels;
  	PtrStack<ColorArea> colorareas;
  	Theme *theme;
  	int hover_row, hover_col;

  	TableEditor(Theme *theme);
  	virtual ~TableEditor();

  	// virtual void ColumnLabels(...); //null terminated cchar
  	// virtual void RowLabels(...); //null terminated cchar
  	virtual void AddColumn(const char *name);
  	virtual void AddRow(const char *name);
	virtual int init();

  	virtual void Refresh();
	virtual int Event(const EventData *e,const char *mes);
	virtual int UseTheme(Theme *theme);
	virtual void SyncFromTheme(Theme *theme);

	virtual SquishyBox *ContentBox(int r, int c, double w,double h);
	virtual int Scan(int x,int y, int *row, int *col);
};

TableEditor::TableEditor(Theme *theme)
  : columnlabels(LISTS_DELETE_Array), rowlabels(LISTS_DELETE_Array)
{
	this->theme = theme;
	theme->inc_count();
	hover_row = -1;
	hover_col = -1;
}

TableEditor::~TableEditor()
{
	theme->dec_count();
}

/*! Return index in colorareas that contains point x,y.
 */
int TableEditor::Scan(int x,int y, int *row, int *col)
{
	int i = 0;
	for (int r = 0; r<rowlabels.n; r++) {
		for (int c=0; c<columnlabels.n; c++) {
			if (x >= colorareas.e[i]->x() && x <= colorareas.e[i]->x()+colorareas.e[i]->w() &&
				y >= colorareas.e[i]->y() && y <= colorareas.e[i]->y()+colorareas.e[i]->h()) {
				*row = r;
				*col = c;
				return i;
			}
			i++;
		}
	}
	return -1;
}

void TableEditor::AddColumn(const char *name)
{
	columnlabels.push(newstr(name));
}

void TableEditor::AddRow(const char *name)
{
	rowlabels.push(newstr(name));
}

// void TableEditor::ColumnLabels(bool flush_first, ...) //null terminated cchar
// {
//     if (flush_first) columnlabels.flush();
//     
//     va_list arg;
//     va_start(arg, flush_first);
//     const char *str = va_arg(arg, const char *);
//     while (str) {
//     	columnlabels.push(newstr(str));
// 	    str = va_arg(arg, const char *);
// 	}
//     va_end(arg);
// }

// void TableEditor::RowLabels(bool flush_first, const char *s1, ...) //null terminated cchar
// {
//     if (flush_first) columnlabels.flush();
//     
//     va_list arg;
//     va_start(arg, flush_first);
//     const char *str = va_arg(arg, const char *);
//     while (str) {
//     	rowlabels.push(newstr(str));
// 	    str = va_arg(arg, const char *);
// 	}
//     va_end(arg);
// }

SquishyBox *TableEditor::ContentBox(int r, int c, double w,double h)
{
	WindowStyle *style = theme->styles.e[r];
	ScreenColor *color = theme->GetScreenColor(style->category, default_colors[c]);
	ColorArea *box = new ColorArea(color, style->category, default_colors[c]);
	colorareas.push(box,1);
	return box;
}

int TableEditor::init()
{
	double th = win_themestyle->normal->textheight();

	double colextent = MaxExtent((const char **)columnlabels.e, columnlabels.n, win_themestyle->normal);
	double rowextent = MaxExtent((const char **)rowlabels.e, rowlabels.n, win_themestyle->normal);

	for (int r = 0; r<rowlabels.n+1; r++) {
		for (int c=0; c<columnlabels.n+1; c++) {
			if (r==0 && c==0) { // blank upper left corner
				AddWin(new MessageBar(this,"ul","ul",MB_CENTER, 0,100,0,0, 0, " "),1,
					rowextent+th,0,0,50,0,  1.5*th,0,0,50,0, -1);

			} else if (r == 0) { //doing column headers
				double ex = win_themestyle->normal->Extent(columnlabels.e[c-1], -1);
				AddWin(new MessageBar(this,"c",nullptr,MB_CENTER, 0,100,0,0, 0, columnlabels.e[c-1]),1,
					colextent,colextent-ex,100,50,0,  1.5*th,0,0,50,0, -1);

			} else if (c == 0) { //row label
				AddWin(new MessageBar(this,"r",nullptr,MB_CENTER, 0,100,0,0, 0, rowlabels.e[r-1]),1,
					colextent+th,0,0,50,0,  1.5*th,0,0,50,0, -1);

			} else { //content
				double ex = win_themestyle->normal->Extent(columnlabels.e[c-1], -1);
				SquishyBox *box = ContentBox(r-1, c-1, colextent, th*1.5);
				box->SetPreferred(colextent,colextent-ex,100,50,0,  1.5 * th,0,0,50,0);
				Push(box, 0, -1);
				
			}
		}
		AddNull();
	}
	
	Sync(1);
	SyncFromTheme(nullptr);
	return 0;
}

void TableEditor::Refresh()
{
	if (!needtodraw) return;
	RowFrame::Refresh();
	needtodraw=0;

	Displayer *dp = MakeCurrent();
	dp->ClearWindow();
	double border = 1 * UIScale();
	for (int c=0; c<colorareas.n; c++) {
		ColorArea *box = colorareas.e[c];
		dp->NewFG(box->color);
		dp->drawrectangle(box->x()+border, box->y()+border, box->w()-border*2, box->h()-border*2, 1);
	}
}

int TableEditor::Event(const EventData *e,const char *mes)
{
	return anXWindow::Event(e,mes);
}

void TableEditor::SyncFromTheme(Theme *theme)
{
	if (theme && theme != this->theme) {
		this->theme->dec_count();
		this->theme = theme;
		theme->inc_count();
	} else theme = this->theme;

	 //colors
	ScreenColor *color;
	int i=0;
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];
		for (int c2=0; default_colors[c2]; c2++) {
			color = theme->GetScreenColor(style->category, default_colors[c2]);
			colorareas[i]->color = *color;
			i++;
		}
	}
	needtodraw = 1;
}

int TableEditor::UseTheme(Theme *theme)
{
	if (theme != this->theme) {
		this->theme->dec_count();
		this->theme = theme;
		theme->inc_count();
	}

	SyncFromTheme(nullptr);
	return 0;
}


//---------------------------------------- ThemeControls -----------------------------------


class ThemeControls : public RowFrame
{
  public:
	ButtonDownInfo buttondown;

	RefPtrStack<Theme> themes;
	PtrStack<ColorArea> colorareas;
	Theme *theme;
	//WindowStyle *panel, *edit, *menu, *button;
	TableEditor *colortable;
	
	LineInput *test_lineedit;
	CheckBox *test_checkbox;
	MenuButton *test_menubutton;
	Button *test_button;
	TreeSelector *test_treeselector;
	NumSlider *test_numslider;

	MessageBar *uiscalemessage;
	MessageBar *status;
	LineInput *border, *bevel, *pad, *tooltips, *firstclk, *dblclk, *idleclk;
	NumSlider *uiscale;

	int firsttime;

	RefPtrStack<anXWindow> testWindows;

	ThemeControls(anXWindow *prnt);
	virtual ~ThemeControls();
	virtual const char *whattype() { return "ThemeControls"; }

	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual void Refresh();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual int UseTheme(int which);
	virtual void UpdateWindows();
	// virtual int MoveResize(int nx,int ny,int nw,int nh);
	// virtual int Resize(int nw,int nh);
	virtual void UIScaleChanged();

	void SyncFromTheme();
};


// int ThemeControls::MoveResize(int nx,int ny,int nw,int nh)
// {
// 	int status = RowFrame::MoveResize(nx,ny,nw,nh);
// 	char str[100];
// 	win_cur_uiscale = -1;
// 	sprintf(str, "current window scale: %f", UIScale());
// 	if (uiscalemessage) uiscalemessage->SetText(str);
// 	return status;
// }
// int ThemeControls::Resize(int nw,int nh)
// {
// 	int status = RowFrame::Resize(nw,nh);
// 	char str[100];
// 	win_cur_uiscale = -1;
// 	sprintf(str, "current window scale: %f", UIScale());
// 	if (uiscalemessage) uiscalemessage->SetText(str);
// 	return status;
// }

ThemeControls::ThemeControls(anXWindow *prnt)
  : RowFrame(prnt, "Theme Controls", "Theme Controls",
		    ROWFRAME_ROWS, 
			//ROWFRAME_STRETCH_IN_COL,// |
			//ROWFRAME_STRETCH_IN_ROW,
			0,0,1200,800,0,
			nullptr,0,nullptr)
{
	firsttime = 1;

	pw(BOX_SHOULD_WRAP);
	ph(BOX_SHOULD_WRAP);

	theme = new Theme("Light");
	theme->AddDefaults("Light");
	themes.push(theme);
	theme->dec_count();
	
	theme = new Theme("Dark");
	theme->AddDefaults("Dark");
	themes.push(theme);
	theme->dec_count();

	theme = new Theme("Gray");
	theme->AddDefaults("Gray");
	themes.push(theme);
	theme->dec_count();

	theme = themes.e[0];
	theme->inc_count();

	uiscalemessage = nullptr;

	//theme = new Theme("Gray");
	//theme = new Theme("Dark");
}

ThemeControls::~ThemeControls()
{
	if (theme) theme->dec_count();
}


int ThemeControls::Event(const EventData *e,const char *mes)
{
	DBG cerr <<"themecontrols event.."<<(mes?mes:"(something)")<<endl;

	if (!strcmp(mes,"mo")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		DBG cerr <<" got message mo: "<<s->str<<endl;
		status->SetText(s->str);
		return 0;

	} else if (!strcmp(mes,"selectTheme")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int which = s->info1;
		if (which == -1) {
			// *** new from current

		} else if (which > 0 && which <= themes.n) {
			UseTheme(which-1);
		}
		return 0;

	} else if  (!strncmp(mes,"normal", 6)
			 || !strncmp(mes,"bold", 4)
			 || !strncmp(mes,"italic", 6)
			 || !strncmp(mes,"monospace", 9)) {
		WindowStyle *style = nullptr;
		int i=-1;
		int which = 0;
		LaxFont *font = nullptr;
		char scratch[200];

		if (!strncmp(mes,"normal", 6))    {
			i = strtod(mes + 6, nullptr); which = THEME_Font_Normal;    if (i>0) font = theme->styles.e[i-1]->normal;

		} else if (!strncmp(mes,"bold", 4)) {
			i = strtod(mes + 4, nullptr); which = THEME_Font_Bold;      if (i>0) font = theme->styles.e[i-1]->bold;

		} else if (!strncmp(mes,"italic", 6)) {
			i = strtod(mes + 6, nullptr); which = THEME_Font_Italic;    if (i>0) font = theme->styles.e[i-1]->italic;

		} else if (!strncmp(mes,"monospace", 9)) {
			i = strtod(mes + 9, nullptr); which = THEME_Font_Monospace; if (i>0) font = theme->styles.e[i-1]->monospace;
		}

		if (i>0) style = theme->styles.e[i-1];
		if (!style) return 0;

		string sendmes = "set"+string(mes);
		FontDialog *d = new FontDialog(nullptr, "Select Font","Select Font", ANXWIN_REMEMBER, 0,0,800,600,0, 
								object_id, sendmes.c_str(),
								0,
								font->Family(), font->Style(), font->Msize(), "Characters", font, true);
		app->rundialog(d);
		return 0;

	} else if (!strncmp(mes,"setnormal", 9)
			|| !strncmp(mes,"setbold", 7)
			|| !strncmp(mes,"setitalic", 9)
			|| !strncmp(mes,"setmonospace", 12)) {
		mes += 3;

		const StrsEventData *m = dynamic_cast<const StrsEventData*>(e);
		const char *family = m->strs[0];
		const char *sstyle = m->strs[1];
		double size        = strtod(m->strs[2], nullptr);
		const char *file   = m->strs[3];

		WindowStyle *style = nullptr;
		int i=-1;
		int which = 0;
		if       (!strncmp(mes,"normal", 6))    { i = strtod(mes + 6, nullptr); which = THEME_Font_Normal;    }
		else if  (!strncmp(mes,"bold", 4))      { i = strtod(mes + 4, nullptr); which = THEME_Font_Bold;      }
		else if  (!strncmp(mes,"italic", 6))    { i = strtod(mes + 6, nullptr); which = THEME_Font_Italic;    }
		else if  (!strncmp(mes,"monospace", 9)) { i = strtod(mes + 9, nullptr); which = THEME_Font_Monospace; }
		if (i>0) style = theme->styles.e[i-1];
		if (!style) return 0;

		LaxFont *font = app->fontmanager->MakeFont(family, sstyle, size, 0);

		if      (which = THEME_Font_Normal)    style->SetFonts(font,nullptr,nullptr,nullptr);
		else if (which = THEME_Font_Bold)      style->SetFonts(nullptr,font,nullptr,nullptr);
		else if (which = THEME_Font_Italic)    style->SetFonts(nullptr,nullptr,font,nullptr);
		else if (which = THEME_Font_Monospace) style->SetFonts(nullptr,nullptr,nullptr,font);

		Button *button = dynamic_cast<Button*>(findChildWindowByName(mes, true));
		if (button) button->Font(font);

		return 0;

	} else if (!strcmp(mes,"loadtheme")) {
		FileDialog *f = new FileDialog(nullptr, "Load", _("Load"), ANXWIN_REMEMBER, 0,0,0,0,0,
									object_id, "loadthemefile", FILES_OPEN_ONE, theme->filename);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"loadthemefile")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		FILE *f = fopen(s->str, "r");
		if (f) {
			DBG cerr << " *** need to implement verify that it is a theme file"<<endl;
			Theme *ntheme = new Theme;
			ntheme->dump_in(f, 0, 0, nullptr, nullptr);
			themes.push(ntheme);
			ntheme->dec_count();
			UseTheme(themes.n-1);
			fclose(f);
		}
		return 0;

	} else if (!strcmp(mes,"savetheme")) {
		FileDialog *f = new FileDialog(nullptr, "Save", _("Save"), ANXWIN_REMEMBER, 0,0,0,0,0,
									object_id, "savethemefile", FILES_SAVE, theme->filename);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"savethemefile")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		FILE *f = fopen(s->str, "w");
		if (f) {
			makestr(theme->filename, s->str);
			theme->dump_out(f, 0, 0, nullptr);
			fclose(f);
			DBG cerr <<" Theme saved to "<<s->str<<endl;
		} else {
			DBG cerr <<" *** Could not save!"<<endl;
		}
		return 0;

	} else if (!strcmp(mes, "uiscaleslide")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e);
		double val = -2;
		if (strEquals(s->str, "default", true)) val = -1;
		if (val == -1 || DoubleAttribute(s->str, &val)) {
			if (val <= 0) val = -1;
			theme->ui_scale = val;
			DBGL("new theme->ui_scale: "<<val<<endl);
			UpdateWindows();
		}
		return 0;
	}

	return anXWindow::Event(e,mes);
}

int ThemeControls::UseTheme(int which)
{
	if (which < 0 || which >= themes.n) return 1;
	if (theme == themes.e[which]) return 0;
	theme->dec_count();
	theme = themes.e[which];
	theme->inc_count();
	SyncFromTheme();
	return 0;
}


void ThemeControls::UpdateWindows()
{
	for (int c=0; c<testWindows.n; c++) {
		testWindows.e[c]->CustomTheme(theme);
	}

	//--- resize windows
	double scale = test_lineedit->UIScale();
	double scaled_th = test_lineedit->UIScale() * test_lineedit->win_themestyle->normal->textheight();

	//LineInput
	GetBox(0)->pw(scaled_th * 7);
	GetBox(0)->ph(2*scaled_th);
	test_lineedit->SetPlacement();

	//TestButton
	GetBox(1)->pw(2*scaled_th + scale * test_button->win_themestyle->normal->Extent(test_button->Label(),-1));
	GetBox(1)->ph(2*scaled_th);

	//Test Checkbox
	GetBox(2)->pw(2*scaled_th + scale * test_checkbox->win_themestyle->normal->Extent(test_checkbox->Label(),-1));
	GetBox(2)->ph(2*scaled_th);

	//Test MenuButton
	GetBox(6)->pw(2*scaled_th + scale * test_menubutton->win_themestyle->normal->Extent(test_menubutton->Label(),-1));
	GetBox(6)->ph(2*scaled_th);

	arrangedstate = 0;
	needtodraw = 1;
	//Sync(0);
}


void ThemeControls::UIScaleChanged()
{
	char str[100];
	win_cur_uiscale = -1;
	sprintf(str, "current window scale: %f", testWindows.e[0]->UIScale());
	if (uiscalemessage) uiscalemessage->SetText(str);

	anXWindow::UIScaleChanged();

	Sync(0);
}


int ThemeControls::init()
{
	double th = UIScale() * win_themestyle->normal->textheight();

	anXWindow *last = nullptr;
	Button *button  = nullptr;
	MessageBar *mes = nullptr;
	RowFrame *rows  = nullptr;


	//---------add mockup
	// 0
	last = test_lineedit = new LineInput(this,"lineinput","lineinput",0, 0,0,200,50,0, last,0,nullptr, "Input", "stuff stuff stuff");
	test_lineedit->GetLineEdit()->SetSelection(5,10);
	AddWin(test_lineedit,1, -1);
	testWindows.push(test_lineedit);

	// 1
	LaxImage *img = IconManager::GetDefault()->GetIcon("ImagePatch");
	if (img) { // test with image
		last = test_button = button = new Button(this,"Test button","Test button",BUTTON_OK, 5,300, 0,2*th,0,
								last,0,nullptr, 0, "Test Button", nullptr, img);
		img->dec_count();
		AddWin(button,1, -1);
		testWindows.push(button);

	} else { // test with graphic instead
		last = test_button = button = new Button(this,"Test button","Test button",BUTTON_OK, 5,300, 0,2*th,0,
								last,0,nullptr, 0, "Test Button", nullptr, nullptr);
		test_button->SetGraphic(THING_Wrench, th, th);
		AddWin(button,1, -1);
		testWindows.push(button);
	}

	// 2
	last = test_checkbox = new CheckBox(this, "checkbox", "checkbox", CHECK_CIRCLE|CHECK_LEFT,
                             0,0,0,0,0,
                             last,object_id,nullptr,
                             "Check this", th/2,th/2);
	testWindows.push(test_checkbox);
	AddWin(test_checkbox,1, -1);

	// 3
	MenuInfo *menu = new MenuInfo;
	for (int c=0; default_color_names[c]; c++) {
		menu->AddItem(default_color_names[c]);
		cerr << c<<": c%3 " << (c%3) <<endl;
		if (c%3==1) {
			menu->SubMenu("Submenu");
			menu->AddItem("1");
			menu->AddItem("2");
			menu->AddItem("3");
			if (c%6==1) {
				menu->SubMenu("Submenu");
				menu->AddItem("4");
				menu->AddItem("5");
				menu->AddItem("6");
				menu->EndSubMenu();
			}
			menu->EndSubMenu();
		}
	}

	last = test_treeselector = new TreeSelector(this,"Test tree","Test tree",0, 0,0, 150,150,1, last,0,nullptr,
			TREESEL_FOLLOW_MOUSE | TREESEL_SCROLLERS, menu);
	menu->dec_count();
	AddWin(test_treeselector,1, -1);
	testWindows.push(test_treeselector);


	// 4
	Scroller *scroller;
	last = scroller = new Scroller(this,"test","test",SC_YSCROLL, 0,0,0,0,1, last,0,nullptr, nullptr, 0,1000, 200, 10, 400, 600);
	AddWin(scroller,1,  10,0,10,50,0,    150,0,0,50,0, -1);
	testWindows.push(scroller);

	// 5
	anXWindow *awindow = new YesNo(this);
	AddWin(awindow,1, 200,50,50,50,0, 100,50,50,50,0, -1);
	testWindows.push(awindow);

	// 6
	MenuInfo *pmenu = new MenuInfo;
	pmenu->AddItem("First");
	for (int c=0; default_color_names[c]; c++) {
		pmenu->AddItem(default_color_names[c]);
		if (c%3==1) {
			pmenu->SubMenu("Subpmenu");
			pmenu->AddItem("1");
			pmenu->AddItem("2");
			pmenu->AddItem("3");
			if (c%6==1) {
				pmenu->SubMenu("Subpmenu");
				pmenu->AddItem("4");
				pmenu->AddItem("5");
				pmenu->AddItem("6");
				pmenu->EndSubMenu();
			}
			pmenu->EndSubMenu();
		}
	}
	pmenu->AddItem("Last");
	last = test_menubutton = new MenuButton(this,"TestMenu","menu",MENUBUTTON_DOWNARROW, 0,0,0,0,1, last,0,nullptr, 0, pmenu,1, "Popup Menu");
	AddWin(test_menubutton,1, -1);
	testWindows.push(test_menubutton);

	// 7
	test_numslider = new NumSlider(rows,"UIScaleSlider", "UI Scale Slider",
		NumSlider::DOUBLES | ItemSlider::EDITABLE | ItemSlider::SENDALL,
		0,0,win_themestyle->normal->textheight()*6,0,0, last,object_id,nullptr, nullptr, 0, 100, 1);
	test_numslider->SetFloatRange(0,100,.1);
	AddWin(test_numslider,1, -1);
	testWindows.push(test_numslider);

	AddNull();
	AddSpacer(10,0,100000,50, 20,0,0,50);
	AddNull();


	//AddVSpacer();

	 //-----status bar
	last = status = mes = new MessageBar(this,"message2","message2",MB_MOVE|MB_CENTER, 0,100,0,0,0, "Stuff!");
	AddWin(mes,1,  100,0,10000,50,0,    th*1.2,0,th,50,0, -1);
	AddNull();


	TabFrame *tabs = new TabFrame();
	double running_height = 0;

	//---------------- base colors
	
	colortable = new TableEditor(theme);
	for (int c2=0; short_color_names[c2]; c2++) {
		colortable->AddColumn(short_color_names[c2]);
	}
	double h = 1;
	for (int c=0; c<theme->styles.n; c++) {
		h++;
		colortable->AddRow(window_category_name(theme->styles.e[c]->category));
	}
	colortable->WrapToExtent();

	running_height = MAX(running_height, (1+h*1.5)*th);
	tabs->AddWin(colortable,1, "Colors", nullptr, 0); 



	//------------diffs
	rows = new RowFrame();
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, ""        ),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);

	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+bg_hover    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+bg_highlight"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+bg_focus    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+bg_alternate"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+fg_hover    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+fg_highlight"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+fg_focus    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+fg_alternate"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+shadow      "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, "+highlight   "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	rows->AddNull();
	
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];
		rows->AddWin(new MessageBar(rows,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, window_category_name(style->category)),1,
					50,40,100,50,0,    1.5*th,0,0,50,0, -1);

		for (int c2=0; diffs[c2]; c2++) {
			string name;
			name = name + window_category_name(style->category) + "/" + diffs[c2];

			last = new LineEdit(rows,name.c_str(),name.c_str(),0, 0,0, 100,0,1, last,object_id,diffs[c2], ".....");
			rows->AddWin(last,1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);                                              
		}
		rows->AddNull();
	}

	running_height = MAX(running_height, (3+6*1.5)*th);
	tabs->AddWin(rows,1, "Color Diffs", nullptr, 0); 


	 //--------------fonts
	rows = new RowFrame();
	Utf8String str;
	char sstr[200];
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];
		str = window_category_name(style->category);
		str += " fonts";
		rows->AddWin(new MessageBar(rows,"l","l",MB_CENTER, 0,0,0,0,0, str.c_str()),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);

		sprintf(sstr, "normal %d", c+1);
		last = button = new Button(rows,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Normal Font", nullptr,nullptr, 3);
		button->Font(style->normal);
		rows->AddWin(button,1,-1);
		sprintf(sstr, "bold %d", c+1);
		last = button = new Button(rows,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Bold Font", nullptr,nullptr, 3);
		button->Font(style->bold);
		rows->AddWin(button,1,-1);
		sprintf(sstr, "italic %d", c+1);
		last = button = new Button(rows,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Italic Font", nullptr,nullptr, 3);
		button->Font(style->italic);
		rows->AddWin(button,1,-1);
		sprintf(sstr, "monospace %d", c+1);
		last = button = new Button(rows,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Monospace Font", nullptr,nullptr, 3);
		button->Font(style->monospace);
		rows->AddWin(button,1,-1);

		rows->AddNull();
	}

	running_height = MAX(running_height, 5*1.5*th);
	tabs->AddWin(rows,1, "Fonts",       nullptr, 0); 


	 //---------------theme wide values
	rows = new RowFrame();

	//last = uiscale = new LineInput(rows,"UI Scale","UI Scale",LINP_FLOAT, 5,415, 100,0,0, last,object_id,"uiscale", "UI Scale",".....");
	//uiscale->tooltip(_("If -1, then use environment variable GTK_SCALE\nor QT_SCALE_FACTOR, whichever exists"));
	//rows->AddWin(last,1,-1);
	
	mes = new MessageBar(rows,"uimes",nullptr,MB_CENTER, 0,0,0,0,0, _("UI Scale"), th*.25,th*.25);
	rows->AddWin(mes,1,-1);
	uiscale = new NumSlider(rows,"UIScaleSlider", "UI Scale Slider", 
		NumSlider::DOUBLES | ItemSlider::EDITABLE | ItemSlider::SENDALL,
		0,0,win_themestyle->normal->textheight()*5,0,0, last,object_id,"uiscaleslide", nullptr, 0, 100, 1);
	uiscale->SetFloatRange(0,100,.1);
	uiscale->tooltip(_("If <= 0, then use environment variable GTK_SCALE\nor QT_SCALE_FACTOR, whichever exists"));
	last = uiscale;
	rows->AddWin(last,1,-1);

	str.Sprintf("current window scale: %f", UIScale());
	mes = uiscalemessage = new MessageBar(rows,"uimes",nullptr,MB_CENTER, 0,0,0,0,0, str.c_str());
	rows->AddWin(mes,1,  mes->win_w,0,100,50,0,    1.5*th,0,0,50,0, -1);

	rows->AddNull();
	last = border = new LineInput(rows,"Border","Border",LINP_FLOAT, 5,415, 100,0,0, last,object_id,"border", "Border",".....");
	rows->AddWin(last,1,-1);
	last = bevel = new LineInput(rows,"Bevel","Bevel",LINP_FLOAT, 5,415, 100,0,0, last,object_id,"bevel", "Bevel",".....");
	rows->AddWin(last,1,-1);
	last = pad = new LineInput(rows,"Pad","Pad",LINP_FLOAT, 5,415, 100,0,0, last,object_id,"pad", "Pad",".....");
	rows->AddWin(last,1,-1);
	rows->AddNull();
	last = tooltips = new LineInput(rows,"Tooltips","Tooltips",LINP_INT, 5,415, 150,0,0, last,object_id,"tooltips", "Tooltips (ms)",".....");
	last->tooltip(_("Milliseconds before popping tooltips, or 0 for never"));
	rows->AddWin(last,1,-1);
	rows->AddNull();
	last = firstclk = new LineInput(rows,"firstclk","firstclk",LINP_INT, 5,415, 100,0,0, last,object_id,"firstclk", "Firstclk",".....");
	last->tooltip(_("Milliseconds after first click before idle clicking"));
	rows->AddWin(last,1,-1);
	last = dblclk = new LineInput(rows,"dblclk","dblclk",LINP_INT, 5,415, 100,0,0, last,object_id,"dblclk", "dblclk",".....");
	last->tooltip(_("Millisecond threshhold for double clicks"));
	rows->AddWin(last,1,-1);
	last = idleclk = new LineInput(rows,"idleclk","idleclk",LINP_INT, 5,415, 100,0,0, last,object_id,"idleclk", "idleclk",".....");
	last->tooltip(_("Milliseconds between idle clicks"));
	rows->AddWin(last,1,-1);
	rows->AddNull();

	running_height = MAX(running_height, 3*1.5*th);
	tabs->AddWin(rows,1, "Metrics",     nullptr, 0); 


	//---------------------------- Icons
	// Default dirs:
	//   dir 1
	//   dir 2
	// per category:
	//   override dir
	//
	//
	tabs->AddWin(new MessageBar(tabs,"l","l",MB_CENTER, 0,100,0,0,0, "TODO!!"),1, "Icons",       nullptr, 0); 



	//---------------- add the tab window
	//tabs->WrapToExtent();
	//AddWin(tabs,1, 1000,500,3000,50,0, tabs->win_h,0,0,50,0, -1);
	AddWin(tabs,1, 1000,500,3000,50,0, running_height,0,0,50,0, -1);
	AddNull();




	 //---------------------load, save, select theme
	AddSpacer(10,0,100000,50, 10,0,0,50);
	AddNull();

	last = button = new Button(this,"Load","Load",BUTTON_OK, 5,300, 0,0,0, last,object_id,"loadtheme", 0, "Load...", nullptr,nullptr, 3);
	AddWin(button,1,-1);
	last = button = new Button(this,"Save","Save",BUTTON_OK, 5,300, 0,0,0, last,object_id,"savetheme", 0, "Save...", nullptr,nullptr, 3);
	AddWin(button,1,-1);

	LineInput *linp = new LineInput(this,"lineinput","lineinput",0, 0,0,400,th*1.5, 0, nullptr,0,nullptr, "Theme name", _("default"));
	AddWin(linp,1, -1);


	SliderPopup *p=new SliderPopup(this,"view type",nullptr,SLIDER_POP_ONLY, 0,0,0,0,1, nullptr,object_id,"selectTheme");
	for (int c=0; c<themes.n; c++) {
		p->AddItem(themes.e[c]->name,c+1);
	}
	p->AddSep();
	p->AddItem(_("New from current"), -1);
	p->SetState(-1,SLIDER_IGNORE_ON_BROWSE,1);
    p->Select(0);
    p->WrapToExtent();
    p->tooltip(_("Select theme"));
    AddWin(p,1, p->win_w,0,50,50,0, p->win_h,0,0,50,0, -1);


	AddNull();
	

	//ColorSliders *sliders;
	//last = sliders = new ColorSliders(this, "colors", "colors", 0, 0,0,0,0,0, last,object_id,nullptr, LAX_RGB,1, 1.,1.,1.);
	//AddWin(sliders, 1,-1);




	Sync(1);
	SyncFromTheme();

	return 0;
}

void ThemeControls::SyncFromTheme()
{
	 //colors
	colortable->SyncFromTheme(theme);

	 //diffs	
	LineEdit *lin;
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];

		for (int c2=0; diffs[c2]; c2++) {
			string name;
			name = name + window_category_name(style->category) + "/" + diffs[c2];
			lin = dynamic_cast<LineEdit*>(findChildWindowByName(name.c_str(), true));
			lin->SetText(style->GetValue(diffs[c2]));

		}
		AddNull();
	}

	 //themewide
	uiscale ->Select(theme->ui_scale);
	border  ->SetText(theme->default_border_width);
	pad     ->SetText(theme->default_padx);
	bevel   ->SetText(theme->default_bevel);
	tooltips->SetText(theme->tooltips);
	firstclk->SetText((int)theme->firstclk);
	dblclk  ->SetText((int)theme->dblclk  );
	idleclk ->SetText((int)theme->idleclk );
	

	UpdateWindows();

	needtodraw = 1;
}

void ThemeControls::Refresh()
{
	if (!needtodraw) return;

	if (firsttime) {
		if (win_w != w() || win_h != h()) Resize(w(), h());
		//Sync(0);
		DBG cerr <<"ThemeControls w,h: "<<win_w<<','<<win_h<<"  "<<w()<<','<<h()<<endl;
		firsttime = 0;
	}

	RowFrame::Refresh();
	needtodraw=0;

//	Displayer *dp = MakeCurrent();
//
//	for (int c=0; c<colorareas.n; c++) {
//		ColorArea *area = colorareas.e[c];
//		dp->NewFG(&area->color);
//		dp->drawrectangle(area->x()+1, area->y()+1, area->w()-2, area->h()-2, 1);
//	}
}

int ThemeControls::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	if (ch==LAX_Left) {
	} else if (ch==LAX_Right) {
	} else if (ch==LAX_Up) {
	} else if (ch==LAX_Down) {
	}

	return anXWindow::CharInput(ch,buffer,len,state,kb);
}

int ThemeControls::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d) 
{
	//buttondown.down(d->id,LEFTBUTTON, x,y);
	return 1;
}

int ThemeControls::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	return 1;
}

int ThemeControls::MouseMove(int x,int y,unsigned int state, const LaxMouse *m)
{
//	if (!buttondown.any()) return 0;
//	int lx,ly;
//	buttondown.move(m->id, x,y, &lx,&ly);
//	offset+=flatpoint(x-lx,y-ly);
//	angle=(2*x/(double)win_w-1) * M_PI/2;
//	points.push(flatpoint(x,y));
//	needtodraw=1;
	return 0;
}


//---------------------------------------- main -----------------------------------

int main(int argc,char **argv)
{

	//------------initialize 
	anXApp app;
	app.Backend("cairo");
	makestr(app.textfontstr,"sans-20");
	makestr(app.controlfontstr,"sans-15");
	app.init(argc,argv);

	IconManager::GetDefault()->AddPath("../lax/icons");

	//------------add windows

	int ww = 1024;
	int wh = 600;
	int mx,my;
	ScreenInformation *monitor = nullptr;
	mouseposition(0, nullptr, &mx, &my, nullptr, nullptr, nullptr, &monitor);
	int x = monitor->x + monitor->width/2 - ww/2;
	int y = monitor->y;

	
	ScrolledWindow *container = new ScrolledWindow(nullptr, "Theme", _("Theme"), ANXWIN_ESCAPABLE | SW_RIGHT | SW_MOVE_WINDOW,
			x,y,ww,wh,0, nullptr);
	ThemeControls *controls = new ThemeControls(nullptr);
	container->UseThisWindow(controls);
	//controls->dec_count();
	app.addwindow(container);


	//-----------now run!

	cout <<"------Done adding initial windows in main() -------\n";
	app.run();


	cout <<"------ App Close:  -------\n";
	app.close();
	
	cout <<"------ Bye! -------\n";
	return 0;
}

