/**************** themes.cc *****************/



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
#include <lax/themes.h>
#include <lax/version.h>

#include <lax/language.h>


//template implementation:
#include <lax/refptrstack.cc>


#include <string>
#include <iostream>
using namespace std;
using namespace Laxkit;
using namespace LaxFiles;


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


#define DBG


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
		NULL
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
		NULL
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
		NULL
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
	: ColorBox(parnt,nname,ntitle,nstyle, nx,ny,nw,nh,brder, prev,owner,mes, ctype,nstep,c0,c1,c2,c3,c4,NULL)
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
	YesNo(anXWindow *parent) : anXWindow(parent, "yesno","yesno", 0, 0,0,0,0,1, NULL,0,NULL) {}

	virtual void Refresh() {
		if (!needtodraw) return;
		needtodraw=0;

		Displayer *dp = MakeCurrent();
		dp->ClearWindow();

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


//---------------------------------------- ThemeControls -----------------------------------

class ColorArea : public SquishyBox
{
  public:
	ScreenColor color;
	int category;
	int which;

	ColorArea(ScreenColor *col, int cat, int nwhich) {
		color = *col;
		category = cat;
		which = nwhich;
	}
};

class ThemeControls : public RowFrame
{
  public:
	ButtonDownInfo buttondown;

	RefPtrStack<Theme> themes;
	PtrStack<ColorArea> colorareas;
	Theme *theme;
	WindowStyle *panel, *edit, *menu, *button;
	
	MessageBar *status;
	LineInput *border, *bevel, *pad, *tooltips, *firstclk, *dblclk, *idleclk;

	int firsttime;

	RefPtrStack<anXWindow> testWindows;

	ThemeControls();
	virtual ~ThemeControls();

	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual void Refresh();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual int UseTheme(int which);
	virtual void UpdateWindows();

	void SyncFromTheme();
};

ThemeControls::ThemeControls()
  : RowFrame(NULL, "Theme", "Theme", ANXWIN_ESCAPABLE | ROWFRAME_ROWS|ROWFRAME_STRETCH_IN_COL|ROWFRAME_STRETCH_IN_ROW,
  //: RowFrame(NULL, "Theme", "Theme", ANXWIN_ESCAPABLE | ROWFRAME_ROWS|ROWFRAME_STRETCH_IN_COL,
			0,0,1200,800,0,
			NULL,0,NULL)
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
		WindowStyle *style = NULL;
		int i=-1;
		int which = 0;
		LaxFont *font = NULL;
		char scratch[200];

		if (!strncmp(mes,"normal", 6))    {
			i = strtod(mes + 6, NULL); which = THEME_Font_Normal;    if (i>0) font = theme->styles.e[i-1]->normal;

		} else if (!strncmp(mes,"bold", 4)) {
			i = strtod(mes + 4, NULL); which = THEME_Font_Bold;      if (i>0) font = theme->styles.e[i-1]->bold;

		} else if (!strncmp(mes,"italic", 6)) {
			i = strtod(mes + 6, NULL); which = THEME_Font_Italic;    if (i>0) font = theme->styles.e[i-1]->italic;

		} else if (!strncmp(mes,"monospace", 9)) {
			i = strtod(mes + 9, NULL); which = THEME_Font_Monospace; if (i>0) font = theme->styles.e[i-1]->monospace;
		}

		if (i>0) style = theme->styles.e[i-1];
		if (!style) return 0;

		string sendmes = "set"+string(mes);
		FontDialog *d = new FontDialog(NULL, "Select Font","Select Font", ANXWIN_REMEMBER, 0,0,800,600,0, 
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
		double size        = strtod(m->strs[2], NULL);
		const char *file   = m->strs[3];

		WindowStyle *style = NULL;
		int i=-1;
		int which = 0;
		if       (!strncmp(mes,"normal", 6))    { i = strtod(mes + 6, NULL); which = THEME_Font_Normal;    }
		else if  (!strncmp(mes,"bold", 4))      { i = strtod(mes + 4, NULL); which = THEME_Font_Bold;      }
		else if  (!strncmp(mes,"italic", 6))    { i = strtod(mes + 6, NULL); which = THEME_Font_Italic;    }
		else if  (!strncmp(mes,"monospace", 9)) { i = strtod(mes + 9, NULL); which = THEME_Font_Monospace; }
		if (i>0) style = theme->styles.e[i-1];
		if (!style) return 0;

		LaxFont *font = app->fontmanager->MakeFont(family, sstyle, size, 0);

		if      (which = THEME_Font_Normal)    style->SetFonts(font,NULL,NULL,NULL);
		else if (which = THEME_Font_Bold)      style->SetFonts(NULL,font,NULL,NULL);
		else if (which = THEME_Font_Italic)    style->SetFonts(NULL,NULL,font,NULL);
		else if (which = THEME_Font_Monospace) style->SetFonts(NULL,NULL,NULL,font);

		Button *button = dynamic_cast<Button*>(findChildWindowByName(mes));
		if (button) button->Font(font);

		return 0;

	} else if (!strcmp(mes,"loadtheme")) {
		FileDialog *f = new FileDialog(NULL, "Load", _("Load"), ANXWIN_REMEMBER, 0,0,0,0,0,
									object_id, "loadthemefile", FILES_OPEN_ONE, theme->filename);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"loadthemefile")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		FILE *f = fopen(s->str, "r");
		if (f) {
			DBG cerr << " *** need to implement verify that it is a theme file"<<endl;
			Theme *ntheme = new Theme;
			ntheme->dump_in(f, 0, 0, NULL, NULL);
			themes.push(ntheme);
			ntheme->dec_count();
			UseTheme(themes.n-1);
			fclose(f);
		}
		return 0;

	} else if (!strcmp(mes,"savetheme")) {
		FileDialog *f = new FileDialog(NULL, "Save", _("Save"), ANXWIN_REMEMBER, 0,0,0,0,0,
									object_id, "savethemefile", FILES_SAVE, theme->filename);
		app->rundialog(f);
		return 0;

	} else if (!strcmp(mes,"savethemefile")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		FILE *f = fopen(s->str, "w");
		if (f) {
			makestr(theme->filename, s->str);
			theme->dump_out(f, 0, 0, NULL);
			fclose(f);
			DBG cerr <<" Theme saved to "<<s->str<<endl;
		} else {
			DBG cerr <<" *** Could not save!"<<endl;
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

int ThemeControls::init()
{
	double th=app->defaultlaxfont->textheight();

	anXWindow *last=NULL;
	Button *button=NULL;
	MessageBar *mes=NULL;


	//---------add mockup

	LineInput *test_lineedit = new LineInput(this,"lineinput","lineinput",0, 0,0,200,50,0, NULL,0,NULL, "Input", "stuff stuff stuff");
	test_lineedit->GetLineEdit()->SetSelection(5,10);
	AddWin(test_lineedit,1, -1);
	testWindows.push(test_lineedit);

	button = new Button(this,"Test button","Test button",BUTTON_OK, 5,300, 0,0,0, NULL,0,NULL, 0, "Test Button", NULL,NULL, 3);
	AddWin(button,1, -1);
	testWindows.push(button);

	MenuInfo *menu = new MenuInfo;
	for (int c=0; default_color_names[c]; c++) {
		menu->AddItem(default_color_names[c]);
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
	TreeSelector *tree = new TreeSelector(this,"Test tree","Test tree",0, 0,0, 150,150,1, NULL,0,NULL, TREESEL_FOLLOW_MOUSE, menu);
	menu->dec_count();
	AddWin(tree,1, -1);
	testWindows.push(tree);

	Scroller *scroller = new Scroller(this,"test","test",SC_YSCROLL, 0,0,0,0,1, NULL,0,NULL, NULL, 0,1000, 200, 10, 400, 600);
	AddWin(scroller,1,  10,0,10,50,0,    150,0,10000,50,0, -1);
	testWindows.push(scroller);

	anXWindow *awindow = new YesNo(this);
	AddWin(awindow,1, 200,50,50,50,0, 100,50,50,50,0, -1);
	testWindows.push(awindow);

	MenuInfo *pmenu = new MenuInfo;
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
	MenuButton *menubutton = new MenuButton(this,"menu","menu",MENUBUTTON_DOWNARROW, 0,0,0,0,1, NULL,0,NULL, 0, pmenu,1, "Popup Menu");
	AddWin(menubutton,1, -1);
	testWindows.push(menubutton);

	AddNull();
	AddSpacer(10,0,100000,50, 10,0,0,50);
	AddNull();



	 //-----status bar
	last = status = mes = new MessageBar(this,"message2","message2",MB_MOVE|MB_CENTER, 0,100,0,0,0, "Stuff!");
	AddWin(mes,1,  100,0,10000,50,0,    th*1.2,0,10000,50,0, -1);
	AddNull();


	 //column labels
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,100,0,0,0, ""        ),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	for (int c2=0; short_color_names[c2]; c2++) {
		AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,100,0,0,0, short_color_names[c2]),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	}
	AddNull();

	 //color rows
	ScreenColor *color;
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];
		AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,100,0,0,0, window_category_name(style->category)),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);

		for (int c2=0; default_colors[c2]; c2++) {
			color = theme->GetScreenColor(style->category, default_colors[c2]);
			ColorArea *box = new ColorArea(color, style->category, default_colors[c2]);
			box->SetPreferred(50,40,100,50,0,  50,40,100,50,0);
			Push(box, 0, -1);
			colorareas.push(box,1);
		}
		AddNull();
	}


	//diffs
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, ""        ),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);

	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+bg_hover    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+bg_highlight"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+bg_focus    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+bg_alternate"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+fg_hover    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+fg_highlight"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+fg_focus    "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+fg_alternate"),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+shadow      "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, "+highlight   "),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);
	AddNull();
	
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];
		AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, window_category_name(style->category)),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);

		for (int c2=0; diffs[c2]; c2++) {
			string name;
			name = name + window_category_name(style->category) + "/" + diffs[c2];

			last = new LineEdit(this,name.c_str(),name.c_str(),0, 0,0, 100,0,0, last,object_id,diffs[c2], ".....");
			AddWin(last,1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);                                              
		}
		AddNull();
	}

	AddSpacer(10,0,100000,50, 10,0,0,50);
	AddNull();

	 //theme wide values
	last = border = new LineInput(this,"Border","Border",0, 5,415, 100,0,0, last,object_id,"border", "Border",".....");
	AddWin(last,1,-1);
	last = bevel = new LineInput(this,"Bevel","Bevel",0, 5,415, 100,0,0, last,object_id,"bevel", "Bevel",".....");
	AddWin(last,1,-1);
	last = pad = new LineInput(this,"Pad","Pad",0, 5,415, 100,0,0, last,object_id,"pad", "Pad",".....");
	AddWin(last,1,-1);
	last = tooltips = new LineInput(this,"Tooltips","Tooltips",0, 5,415, 150,0,0, last,object_id,"tooltips", "Tooltips",".....");
	last->tooltip(_("Milliseconds before popping tooltips, or 0 for never"));
	AddWin(last,1,-1);
	last = firstclk = new LineInput(this,"firstclk","firstclk",0, 5,415, 100,0,0, last,object_id,"firstclk", "Firstclk",".....");
	last->tooltip(_("Milliseconds after first click before idle clicking"));
	AddWin(last,1,-1);
	last = dblclk = new LineInput(this,"dblclk","dblclk",0, 5,415, 100,0,0, last,object_id,"dblclk", "dblclk",".....");
	last->tooltip(_("Millisecond threshhold for double clicks"));
	AddWin(last,1,-1);
	last = idleclk = new LineInput(this,"idleclk","idleclk",0, 5,415, 100,0,0, last,object_id,"idleclk", "idleclk",".....");
	last->tooltip(_("Milliseconds between idle clicks"));
	AddWin(last,1,-1);
	AddNull();
	AddSpacer(10,0,100000,50, 10,0,0,50);
	AddNull();

	 //fonts
	string str;
	char sstr[200];
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];
		str = window_category_name(style->category);
		str += " fonts";
		AddWin(new MessageBar(this,"l","l",MB_MOVE|MB_CENTER, 0,0,0,0,0, str.c_str()),1,  50,40,100,50,0,    1.5*th,0,0,50,0, -1);

		sprintf(sstr, "normal %d", c+1);
		last = button = new Button(this,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Normal Font", NULL,NULL, 3);
		button->Font(style->normal);
		AddWin(button,1,-1);
		sprintf(sstr, "bold %d", c+1);
		last = button = new Button(this,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Bold Font", NULL,NULL, 3);
		button->Font(style->bold);
		AddWin(button,1,-1);
		sprintf(sstr, "italic %d", c+1);
		last = button = new Button(this,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Italic Font", NULL,NULL, 3);
		button->Font(style->italic);
		AddWin(button,1,-1);
		sprintf(sstr, "monospace %d", c+1);
		last = button = new Button(this,sstr,"b",BUTTON_OK, 5,300, 0,0,0, last,object_id,sstr, 0, "Monospace Font", NULL,NULL, 3);
		button->Font(style->monospace);
		AddWin(button,1,-1);

		AddNull();
	}

	AddSpacer(10,0,100000,50, 10,0,0,50);
	AddNull();

	 //load, save, select theme
	last = button = new Button(this,"Load","Load",BUTTON_OK, 5,300, 0,0,0, last,object_id,"loadtheme", 0, "Load...", NULL,NULL, 3);
	AddWin(button,1,-1);
	last = button = new Button(this,"Save","Save",BUTTON_OK, 5,300, 0,0,0, last,object_id,"savetheme", 0, "Save...", NULL,NULL, 3);
	AddWin(button,1,-1);

	LineInput *linp = new LineInput(this,"lineinput","lineinput",0, 0,0,400,50,0, NULL,0,NULL, "Theme name", _("default"));
	AddWin(linp,1, -1);


	SliderPopup *p=new SliderPopup(this,"view type",NULL,0, 0,0,0,0,1, NULL,object_id,"selectTheme");
	for (int c=0; c<themes.n; c++) {
		p->AddItem(themes.e[c]->name,c+1);
	}
	p->AddSep();
	p->AddItem(_("New from current"), -1);
    p->Select(0);
    p->WrapToExtent();
    p->tooltip(_("Select theme"));
    AddWin(p,1, p->win_w,0,50,50,0, p->win_h,0,50,50,0, -1);


	AddNull();
	

	//ColorSliders *sliders;
	//last = sliders = new ColorSliders(this, "colors", "colors", 0, 0,0,0,0,0, last,object_id,NULL, LAX_RGB,1, 1.,1.,1.);
	//AddWin(sliders, 1,-1);




	Sync(1);
	SyncFromTheme();

	return 0;
}

void ThemeControls::UpdateWindows()
{
	for (int c=0; c<testWindows.n; c++) {
		testWindows.e[c]->ThemeChange(theme);
	}
}

void ThemeControls::SyncFromTheme()
{
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

	 //diffs	
	LineEdit *lin;
	for (int c=0; c<theme->styles.n; c++) {
		WindowStyle *style = theme->styles.e[c];

		for (int c2=0; diffs[c2]; c2++) {
			string name;
			name = name + window_category_name(style->category) + "/" + diffs[c2];
			lin = dynamic_cast<LineEdit*>(findChildWindowByName(name.c_str()));
			lin->SetText(style->GetValue(diffs[c2]));

		}
		AddNull();
	}

	 //themewide
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

	Displayer *dp = MakeCurrent();

	for (int c=0; c<colorareas.n; c++) {
		ColorArea *area = colorareas.e[c];
		dp->NewFG(&area->color);
		dp->drawrectangle(area->x()+1, area->y()+1, area->w()-2, area->h()-2, 1);

	}
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

	//Theme theme("Test theme!");
	//theme.AddDefaults("Light");
	//theme.dump_out(stdout, 0, 0, NULL);

	 //------------initialize 
	anXApp app;
	app.Backend("cairo");
	makestr(app.textfontstr,"sans-20");
	makestr(app.controlfontstr,"sans-15");
	app.init(argc,argv);


	 //------------add windows

	app.addwindow(new ThemeControls());


	 //-----------now run!

	cout <<"------Done adding initial windows in main() -------\n";
	app.run();


	cout <<"------ App Close:  -------\n";
	app.close();
	
	cout <<"------ Bye! -------\n";
	return 0;
}

