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
//    Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/palettewindow.h>
#include <lax/laxutils.h>
#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/colorevents.h>
#include <lax/colorsliders.h>
#include <lax/popupmenu.h>
#include <lax/inputdialog.h>
#include <lax/language.h>

#include <sys/stat.h>
#include <cmath>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//-------------------------------- PaletteWindow -----------------------------


/*! \class PaletteWindow 
 *  \brief A window to handle Palette instances.
 */
/*! \var double PaletteWindow::dx
 * \brief Column width.
 */
/*! \var double PaletteWindow::dy
 * \brief Row height.
 */


PaletteWindow::PaletteWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend)
	: anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	palette = GradientStrip::rainbowPalette(27,18, true);
	makestr(palette->name, _("Default"));
	palette->SetFlags(GradientStrip::Read_only | GradientStrip::Built_in, true);

	curcolor = -1;
	hover = HOVER_None;
	//color_popup = nullptr;
	
	InstallColors(THEME_Panel);
	pad = app->theme->default_padx;

	findInrect();
}

PaletteWindow::~PaletteWindow()
{
	if (palette) palette->dec_count();
	if (sc) sc->dec_count();
}


//! Send a SimpleColorEventData message to win_owner. 
int PaletteWindow::send()
{
	if (!win_owner || !win_sendthis || !palette || curcolor<0) {
		return 0;
	}

	ColorEventData *e = new ColorEventData(palette->colors.e[curcolor]->color, 0, 0,0,0);
	e->colorindex = -1;
	
	app->SendMessage(e,win_owner,win_sendthis,object_id);
	return 1;
}


void PaletteWindow::LaunchNewColor()
{
	double white[4] { 1., 1., 1., 1. };
	double *cc = curcolor >= 0 ? palette->colors.e[curcolor]->color->values : white;
	//waiting_for_new = true;
	ColorSliders *win = new ColorSliders(NULL,"New Color","New Color",
							ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS|COLORSLIDERS_Done_Button|COLORSLIDERS_Send_Only_Done,
							0,0,200,400,0,
							NULL,object_id,"newcolor",
							LAX_COLOR_RGB, 1./255,
							cc[0],cc[1],cc[2],cc[3],0.,
							0,0);
	app->rundialog(win);
}


void PaletteWindow::LaunchEditColor()
{
	if (curcolor < 0) return;
	double *cc = palette->colors.e[curcolor]->color->values;
	ColorSliders *win = new ColorSliders(NULL,"Edit Color","Edit Color",
							ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS|COLORSLIDERS_Done_Button,
							0,0,200,400,0,
							NULL,object_id,"editcolor",
							LAX_COLOR_RGB, 1./255,
							cc[0],cc[1],cc[2],cc[3],0.,
							0,0);
	app->rundialog(win);
}


void PaletteWindow::LaunchImportPaletteDialog()
{
	app->rundialog(new FileDialog(NULL,"ImportPalette",_("Import Palette"), ANXWIN_REMEMBER,
								  0,0,400,500,0,
								  object_id,"importpalette",
								  FILES_OPEN_ONE | FILES_PREVIEW,
								  NULL, PaletteDir()));
}


/*! Currently, this dec_counts the old, and installs a new one.
 * 
 * Return 0 for success, or nonzero error.
 */
int PaletteWindow::LoadPalette(const char *file)
{
	ErrorLog log;
	Palette *p = new Palette;
	p->SetFlags(GradientStrip::AsPalette, true);

	if (p->Import(file, &log)) {
		if (palette) palette->dec_count();
		palette = p;
		app->resourcemanager->AddResource("Palette", p, nullptr, isblank(p->name)?p->Id():p->name, nullptr, nullptr, nullptr, nullptr);
	
		findInrect();
		curcolor = hover = -1;
		needtodraw = 1;
		return 0;
	}

	//TODO: should do toast on error
	DBG dumperrorlog("PaletteWindow::LoadPalette() fail!", log);

	delete p;
	return 1;
}


void PaletteWindow::LaunchExportDialog()
{
	FileDialog *dialog = new FileDialog(NULL,"ExportPalette",_("Export Palette"), ANXWIN_REMEMBER,
								  0,0,400,500,0,
								  object_id,"exportpalette",
								  FILES_SAVE | FILES_PREVIEW,
								  palette->name, PaletteDir());
	dialog->OkButton(_("Export"), nullptr);

	MenuInfo *file_types = new MenuInfo();
	file_types->AddItem(_("Laidout palette: lop"), GradientStrip::Default);          makestr(file_types->Top()->key, "lop");
	file_types->AddItem(_("Inkscape or Gimp palette: gpl"), GradientStrip::GimpGPL); makestr(file_types->Top()->key, "gpl");
	file_types->AddItem(_("Swatchbooker: sbz"),    GradientStrip::SwatchBooker);     makestr(file_types->Top()->key, "sbz");
	file_types->AddItem(_("Krita palette: kpl"),   GradientStrip::KritaKPL);         makestr(file_types->Top()->key, "kpl");
	file_types->AddItem(_("Scribus xml"),          GradientStrip::ScribusXML);       makestr(file_types->Top()->key, "sla");
	file_types->AddItem(_("SVG grid"),             GradientStrip::SVGGrid);          makestr(file_types->Top()->key, "svg");
	file_types->AddItem(_("CSS code"),             GradientStrip::CSSColors);        makestr(file_types->Top()->key, "css");
	dialog->UseFileTypes(file_types, GradientStrip::GimpGPL);

	app->rundialog(dialog);
}


/*! Return 0 for success, or nonzero error.
 */
int PaletteWindow::ExportPalette(const char *file, int type)
{
	// intercept the ones that output zip files:
	if (type == GradientStrip::KritaKPL) {
		return palette->ExportKritaKPL(file) == false;

	} else if (type == GradientStrip::SwatchBooker) {
		return palette->ExportSwatchBookerSBZ(file) == false;

	// everything else are just plain single files:
	} else {
		FILE *f = fopen(file,"w");
		if (!f) return 1;
		palette->dump_out(f, 0, type, NULL);
		fclose(f);
	}
	
	return 0;
}

bool PaletteWindow::UseThis(Palette *new_palette, bool absorb_count)
{
	if (palette != new_palette) {
		palette->dec_count();
		if (!absorb_count) new_palette->inc_count();
	} else {
		if (absorb_count) new_palette->dec_count();
	}
	palette = new_palette;

	findInrect();
	curcolor = hover = -1;
	needtodraw = 1;
	return true;
}
	
void PaletteWindow::Refresh()
{
	if (!win_on || needtodraw==0) return;
	needtodraw=0;

	Displayer *dp = MakeCurrent();
	dp->ClearWindow();
	dp->NewBG(win_themestyle->bg);
 
	double th = UIScale() * win_themestyle->normal->textheight();

	 // draw head stuff
	double pname_width = win_themestyle->normal->Extent(isblank(palette->name) ? _("untitled") : palette->name, -1);
	const char *blah;
	dp->font(win_themestyle->normal, th);
	dp->NewFG(win_themestyle->bg);
	dp->BlendMode(LAXOP_Over);
	dp->drawrectangle(0,0,win_w, th, 1);

	// write palette name
	if (palette->name) blah = palette->name; else blah = _("(untitled)");
	if (hover == HOVER_PaletteName) {
		dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .9));
		dp->drawrectangle(pad+th,pad, pname_width,th, 1);
	}
	dp->NewFG(win_themestyle->fg);
	int cc = dp->textout(pad+th,pad, blah,strlen(blah), LAX_LEFT|LAX_TOP);

	// draw lock
	if (hover == HOVER_Lock || show_locked_hint) {
		if (show_locked_hint) { show_locked_hint = false; dp->NewFG(1.,0.,0.); }
		else dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .9));
		dp->drawrectangle(pad,pad, th,th, 1);
	}
	dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, hover == HOVER_Lock ? .0 : .5));
	if (palette->IsLocked()) {
		dp->drawthing(pad+th/2,pad+th/2, th/2,-th/2, 1, THING_Locked);
	} else {
		dp->drawthing(pad+th/2,pad+th/2, th/2,-th/2, 1, THING_Unlocked);
	}
	cc += th;


	// write color name
	int r,g,b,a;
	if (curcolor >= 0 || hover >= 0) {
		 //write out current color name
		int color;
		if (hover >= 0) color = hover; else color = curcolor;
		r = palette->colors.e[color]->color->values[0]*255;
		g = palette->colors.e[color]->color->values[1]*255;
		b = palette->colors.e[color]->color->values[2]*255;
		a = palette->colors.e[color]->color->values[3]*255;

		char *cname = nullptr;
		if (isblank(palette->colors.e[color]->name)) {
			cname = new char[30];
			sprintf(cname,"%02X%02X%02X",r,g,b);
		} else cname = palette->colors.e[color]->name;

		double cname_width = win_themestyle->normal->Extent(cname, -1);
	
		if (hover == HOVER_ColorName) {
			dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .9));
			dp->drawrectangle(win_w - pad - cname_width, pad, cname_width, th, 1);
		}
		dp->NewFG(win_themestyle->fg);
		int ccc = dp->textout(win_w-pad,pad, (cname),-1,LAX_TOP|LAX_RIGHT);
		if (cname != palette->colors.e[color]->name) delete[] cname;

		 //draw current mouse over color
		ccc = win_w-cc-ccc-4*pad; // width of top bar color part
		if (ccc > 0) {
			if (color >= 0 && curcolor < 0) {
				// draw one solid bar
				dp->NewFG(rgbcolor(r,g,b,a));
				if (a < 255) dp->drawCheckerboard(cc+2*pad,0, ccc, th+2*pad, th/2, 0,0);
				dp->drawrectangle(cc+2*pad,0, ccc, th+2*pad,1);
			} else {
				// draw split color bar:  curcolor / hovered color
				if (a < 255) {
					dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .75));
					dp->drawCheckerboard(cc+2*pad + ccc/2,0, ccc/2, th+2*pad, th/2, 0,0);
				}
				dp->NewFG(rgbcolor(r,g,b,a));
				dp->drawrectangle(cc+2*pad + ccc/2,0, ccc/2, th+2*pad,1);

				if (palette->colors.e[curcolor]->color->values[3] < 1.0) {
					dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .75));
					dp->drawCheckerboard(cc+2*pad,0, ccc/2, th+2*pad, th/2, 0,0);
				}
				dp->NewFG(palette->colors.e[curcolor]->color);
				dp->drawrectangle(cc+2*pad,0, ccc/2, th+2*pad,1);
			}
		}
	}

	 // draw all the palette colors
	int i;
	double x=0,y;
	
	y = inrect.y-dy;
	for (i=0; i<palette->colors.n; i++) {
		if (i%xn==0) {
			x=inrect.x;
			y+=dy;
		}
		r = palette->colors.e[i]->color->values[0] * 255;
		g = palette->colors.e[i]->color->values[1] * 255;
		b = palette->colors.e[i]->color->values[2] * 255;
		a = palette->colors.e[i]->color->values[3] * 255;
		if (a < 255) {
			dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .75));
			dp->drawCheckerboard(x,y,dx+1,dy+1, th/2, 0,0);
		}
		dp->NewFG(rgbcolor(r,g,b,a));
		dp->drawrectangle(x,y,dx+1,dy+1, 1);
		x += dx;
	}
	
	 // draw box around curcolor
	if (curcolor>=0) {
		x=inrect.x + (curcolor%xn)*dx;
		y=inrect.y + (curcolor/xn)*dy;
		dp->NewFG(0., 0., 0.);
		dp->drawrectangle(x,y,dx,dy, 0);
		dp->NewFG(1., 1., 1.);
		dp->drawrectangle(x-1,y-1,dx+2,dy+2, 0);
	}
	
	SwapBuffers();
}


/*! Return either color index or HOVER_*. */
int PaletteWindow::findColorIndex(int x,int y)
{
	double th = UIScale() * win_themestyle->normal->textheight();

	// scan top bar
	if (y < th+pad) {
		if (x < pad + th) return HOVER_Lock;
		double pname_width = palette->name ? win_themestyle->normal->Extent(palette->name, -1) : 0;
		double cname_width = win_themestyle->normal->Extent(curcolor >= 0 && !isblank(palette->colors.e[curcolor]->name)
					? palette->colors.e[curcolor]->name : "xxxxxx", -1);
		if (x < pad + th + pname_width) return HOVER_PaletteName;
		if (x >= win_w - pad - cname_width) return HOVER_ColorName;
		return HOVER_Color;
	}

	if (x<inrect.x || x>=inrect.x+inrect.width || y<inrect.y || y>=inrect.y+inrect.height) return HOVER_None;
	
	if (inrect.height < 1) return HOVER_None;

	int r,c;
	c = (x-inrect.x)/dx;
	r = (y-inrect.y)/dy;
	if (c >= xn) return HOVER_None;
	//DBG cerr<<"c,r: "<<c<<','<<r<<endl;
	c = c+r*xn;
	if (c < 0 || c >= palette->colors.n) return HOVER_None;
	return c;
}


//! Return pointer to the directory to search for palettes from a FileDialog.
/*! Currently defaults to /usr/share/gimp/2.0/palettes if it exists, or NULL.
 */
const char *PaletteWindow::PaletteDir()
{
	if (file_exists("/usr/share/gimp/2.0/palettes",1,NULL)==S_IFDIR)
		return "/usr/share/gimp/2.0/palettes";
	return NULL;
}

int PaletteWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (count == 2) {
		if (hover >= 0 || hover == HOVER_Color) LaunchEditColor();
		else if (hover == HOVER_ColorName)      LaunchRenameColor();
		else if (hover == HOVER_PaletteName)    LaunchRenamePalette();
		return 0;
	}

	buttondown.down(d->id,LEFTBUTTON, x,y);
	MouseMove(x,y,state,d); // force select color
	return 0;
}

int PaletteWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 0;
	buttondown.up(d->id,LEFTBUTTON);

	int cc = findColorIndex(x,y);

	if (cc == hover && hover < 0) {
		// handle actions, not color selection
		if (hover == HOVER_Lock) {
			if (!palette->IsBuiltin()) {
				palette->SetFlags(GradientStrip::Read_only, !palette->IsLocked());				
				needtodraw = 1;
			}

		} else if (hover == HOVER_PaletteName) {
			if (palette->IsLocked()) {
				show_locked_hint = true;
				needtodraw = 1;
			} else {
				LaunchRenamePalette();
			}

		} else if (hover == HOVER_ColorName) {
			if (palette->IsLocked()) {
				show_locked_hint = true;
				needtodraw = 1;
			} else {
				LaunchRenameColor();
			}

		} else if (hover == HOVER_Color) {
			if (palette->IsLocked()) {
				show_locked_hint = true;
				needtodraw = 1;
			} else {
				LaunchEditColor();
			}
		}

	} else if (curcolor != cc && cc >= 0) {
		needtodraw |= 2;
		curcolor = cc;
		send();
	}

	return 0;
}


//! Update showing name of curcolor.
/*! \todo this could be used to show a temp color? buttondown-nodrag-up actually changes curcolor?
 */
int PaletteWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	int cc = findColorIndex(x,y);

	if (buttondown.isdown(0,LEFTBUTTON) && curcolor != cc) {
		// hovered on something new while dragging around
		needtodraw |= 2;
		hover = cc;
		if (cc >= 0 && curcolor != cc) {
			curcolor = cc;
			send();
		}
		return 0;
	}

	// else mouse not down, check for hover:
	if (hover != cc) {
		needtodraw |= 2;
		hover = cc;
	}
	
	return 0;
}

int PaletteWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	// capture so it doesn't leak through to lower windows
	return 0;
}

//! Pop up a context menu to select a recent menu or other things.
int PaletteWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	MenuInfo *menu = new MenuInfo();

	// color options
	menu->AddItem(_("Rename color"),      ACTION_RenameColor);
	menu->AddItem(_("New color"),         ACTION_NewColor);
	menu->AddItem(_("Remove color"),      ACTION_RemoveColor);
	menu->AddItem(_("Edit color"),        ACTION_EditColor);
	menu->AddItem(_("Duplicate color"),   ACTION_DuplicateColor);

	// palette options
	menu->AddSep();
	menu->AddItem(_("Rename palette"),    ACTION_RenamePalette);
	menu->AddItem(_("Duplicate palette"), ACTION_DuplicatePalette);
	//menu->AddItem(_("Delete palette"), ACTION_DeletePalette);

	// resource management
	//menu->AddSep();
	ResourceManager *manager = app->resourcemanager;
	//if (dynamic_cast<Resource*>(palette->ResourceOwner()))
	//	menu->AddItem(_("Make local"),      ENGRAVE_Make_Local,           -2);
	//else
	//	menu->AddItem(_("Make shared resource"),ENGRAVE_Make_Shared,      -2);
	if (manager->NumResources("Palette") > 0) {
		menu->AddSep();
		manager->ResourceMenu("Palette", true, menu, 0, ACTION_PaletteResource, palette);
	}

	// switch palette, and io
	menu->AddSep();
	menu->AddItem(_("New"));
	menu->SubMenu(_("New"));
	menu->AddItem(_("Blank"),           ACTION_NewPalette, 0);
	menu->AddItem(_("Basic"),           ACTION_NewPalette, 1);
	menu->AddItem(_("Laidout Rainbow"), ACTION_NewPalette, 2);
	menu->EndSubMenu();
	menu->AddItem(_("Import..."), ACTION_Import);
	menu->AddItem(_("Export..."), ACTION_Export);

	app->rundialog(new PopupMenu("Palette Menu","Palette Menu", ANXWIN_REMEMBER,
									 0,0,0,0,1,
									 object_id,"palettemenu",
									 d->id,
									 menu,1,NULL,
									 TREESEL_LEFT));
	return 0;
}


//! Make needtodraw=1 if LeaveNotify.
int PaletteWindow::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onMouseOut) {
		//DBG cerr <<" in PaletteWindow::event()..."<<endl;
		hover = -1;
		needtodraw = 1;
		return 0;
		//DBG cerr <<"  Button::event:Leave:"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;

	} else if (!strcmp(mes,"importpalette")) {
		 //sent from a FileDialog, selecting a new palette to load in.
		const StrEventData *s=dynamic_cast<const StrEventData *>(e);
		LoadPalette(s->str);
		return 0;

	} else if (!strcmp(mes,"exportpalette")) {
		 //sent from a FileDialog, selecting a file to export palette to.
		const StrEventData *s = dynamic_cast<const StrEventData *>(e);
		int status = ExportPalette(s->str, s->info2);
		if (status == 0) app->PostMessage2("Export success to: %s", s->str);
		else app->PostMessage2("Error exporting to %s", s->str);
		return 0;

	} else if (!strcmp(mes, "palettemenu")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int id   = s->info2; //id of menu item
		int info = s->info4; //is menuitem info

		if (info == ACTION_PaletteResource) {
			// select the resource
			int rid = id;// - ACTION_PaletteResource;
			Resource *r = app->resourcemanager->FindResourceFromRID(rid, "Palette");
			if (r) {
				Palette *p = dynamic_cast<Palette*>(r->object);
				if (p) {
					UseThis(p, 0);
				}
			}
			return 0;

		} else {
			if (id == ACTION_Export) {
				LaunchExportDialog();

			} else if (id == ACTION_Import) {
				LaunchImportPaletteDialog();

	 		} else if (id == ACTION_RenamePalette) {
	 			if (palette->IsLocked()) {
	 				show_locked_hint = true;
	 				needtodraw = 1;
	 				return 0;
	 			}
	 			LaunchRenamePalette();
				return 0;

	 		} else if (id == ACTION_DuplicatePalette) {
	 			Palette *dup = dynamic_cast<Palette*>(palette->duplicate());
	 			char *newname = increment_file(palette->name);
	 			palette->dec_count();
	 			delete[] dup->name;
	 			dup->name = newname;
	 			palette = dup;
	 			palette->SetFlags(GradientStrip::Read_only, false);
	 			needtodraw = 1;
	 			return 0;

	 		} else if (id == ACTION_EditColor) {
	 			LaunchEditColor();
	 			return 0;

	 		} else if (id == ACTION_RenameColor) {
	 			LaunchRenameColor();
	 			return 0;

	 		} else if (id == ACTION_RemoveColor) {
	 			RemoveColor();
	 			return 0;

	 		} else if (id == ACTION_DuplicateColor) {
	 			if (curcolor < 0) return 0;
	 			GradientStrip::GradientSpot *cur = palette->colors[curcolor];
	 			GradientStrip::GradientSpot *spot = new GradientStrip::GradientSpot(cur->t, cur->s,
	 								cur->color->values[0], cur->color->values[1], cur->color->values[2], cur->color->values[3]);
	 			palette->colors.push(spot, 1, curcolor);
	 			findInrect();
	 			needtodraw = 1;
	 			return 0;

	 		} else if (id == ACTION_NewPalette) {
	 			Palette *p = nullptr;

	 			if (info == 0) {
	 				p = new Palette();
	 				makestr(p->name, _("Blank"));

	 			} else if (info == 1) {
	 				p = new Palette();
	 				makestr(p->name, _("Basic"));
	 				p->AddColor(0, 1.,0.,0.,1., _("Red"));
	 				p->AddColor(1, 1.,1.,0.,1., _("Yellow"));
	 				p->AddColor(2, 0.,1.,0.,1., _("Green"));
	 				p->AddColor(3, 0.,1.,1.,1., _("Cyan"));
	 				p->AddColor(4, 0.,0.,1.,1., _("Blue"));
	 				p->AddColor(5, 1.,0.,1.,1., _("Magenta"));
	 				p->AddColor(6, 1.,1.,1.,1., _("White"));
	 				p->AddColor(7, .5,.5,.5,1., _("Gray"));
	 				p->AddColor(8, 0.,0.,0.,1., _("Black"));

	 			} else if (info == 2) {
	 				p = GradientStrip::rainbowPalette(27,18, true);
	 			}
 				if (p) {
 					p->SetFlags(GradientStrip::AsPalette, true);
 					app->resourcemanager->AddResource("Palette", p, nullptr, isblank(p->name)?p->Id():p->name, nullptr, nullptr, nullptr, nullptr);
 					UseThis(p, true);
 				}
 				return 0;

	 		} else if (id == ACTION_NewColor) {
	 			LaunchNewColor();
	 			return 0;
	 			
	 		} else if (id == ACTION_SaveAsResource) {
	 			// ***
	 		}
		}

		return 0;

	} else if (!strcmp(mes, "renamepalette")) {
		const StrEventData *s = dynamic_cast<const StrEventData *>(e);
		makestr(palette->name, s->str);
		needtodraw = 1;
		return 0;

	} else if (!strcmp(mes, "renamecolor")) {
		const StrEventData *s = dynamic_cast<const StrEventData *>(e);
		makestr(palette->colors.e[curcolor]->name, s->str);
		needtodraw = 1;
		return 0;

	} else if (!strcmp(mes, "editcolor")) {
		const SimpleColorEventData *ev = dynamic_cast<const SimpleColorEventData *>(e);
		Color *col = palette->colors.e[curcolor]->color;
		col->values[0] = ev->channels[0] / (double)ev->max;
		col->values[1] = ev->channels[1] / (double)ev->max;
		col->values[2] = ev->channels[2] / (double)ev->max;
		col->values[3] = ev->channels[3] / (double)ev->max;
		needtodraw = 1;
		return 0;

	} else if (!strcmp(mes, "newcolor")) {
		//waiting_for_new = false;
		const SimpleColorEventData *ev = dynamic_cast<const SimpleColorEventData *>(e);
		palette->AddPaletteColor(
				ev->channels[0] / (double)ev->max,
				ev->channels[1] / (double)ev->max,
				ev->channels[2] / (double)ev->max,
				ev->channels[3] / (double)ev->max,
				nullptr, curcolor);
		findInrect();
		needtodraw = 1;
		return 0;
	}

	return anXWindow::Event(e,mes);
}


void PaletteWindow::RemoveColor()
{
	if (curcolor < 0) return;
	palette->RemoveColor(curcolor);
	if (curcolor == palette->colors.n) curcolor--;
	hover = curcolor;
	findInrect();
	needtodraw = 1;
}


void PaletteWindow::LaunchRenamePalette()
{
	InputDialog *i = new InputDialog(nullptr,"Rename",_("Rename palette"), ANXWIN_CENTER|ANXWIN_OUT_CLICK_DESTROYS,
			 20,20,.75*win_w,0, 4,
			 NULL,object_id,"renamepalette",
			 palette->name,	  //start text
			 _("New name?"), //label
			 _("Rename"), 1,
			 _("Cancel"), 0);
	app->rundialog(i);
}

void PaletteWindow::LaunchRenameColor()
{
	if (curcolor < 0) return;

	InputDialog *i = new InputDialog(nullptr,"Rename",_("Rename color"), ANXWIN_CENTER|ANXWIN_OUT_CLICK_DESTROYS,
			 20,20,.75*win_w,0, 4,
			 NULL,object_id,"renamecolor",
			 palette->colors.e[curcolor]->name, //start text
			 _("New name?"), //label
			 _("Rename"), 1,
			 _("Cancel"), 0);
	app->rundialog(i);
}



int PaletteWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	// check shortcuts
	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch, state&LAX_STATE_MASK, 0);
	if (action >= 0) {
		return PerformAction(action);
	}

	if (ch == LAX_Left) {
		if (curcolor > 0) {
			int newpos = curcolor - 1;
			if ((state & (ShiftMask|ControlMask)) == ShiftMask && !palette->IsLocked()) {
				// move color
				palette->colors.swap(curcolor, newpos);
			} 
			curcolor = newpos;
			needtodraw = 1;
			if ((state & (ShiftMask|ControlMask)) == 0) send();
		}
		return 0;

	} else if (ch == LAX_Right) {
		if (curcolor < palette->colors.n-1) {
			int newpos = curcolor + 1;
			if ((state & (ShiftMask|ControlMask)) == ShiftMask && !palette->IsLocked()) {
				// move color
				palette->colors.swap(curcolor, newpos);
			}
			curcolor = newpos;
			needtodraw = 1;
			if ((state & (ShiftMask|ControlMask)) == 0) send();
		}
		return 0;

	} else if (ch == LAX_Up) {
		if (curcolor >= xn) {
			int newpos = curcolor - xn;
			if ((state & (ShiftMask|ControlMask)) == ShiftMask && !palette->IsLocked()) {
				// move color
				palette->colors.swap(curcolor, newpos);
			}
			curcolor = newpos;
			needtodraw = 1;
			if ((state & (ShiftMask|ControlMask)) == 0) send();
		}
		return 0;

	} else if (ch == LAX_Down) {
		if (curcolor+xn < palette->colors.n) {
			int newpos = curcolor + xn;
			if ((state & (ShiftMask|ControlMask)) == ShiftMask && !palette->IsLocked()) {
				// move color
				palette->colors.swap(curcolor, newpos);
			}
			curcolor = newpos;
			needtodraw = 1;
			if ((state & (ShiftMask|ControlMask)) == 0) send();
		}
		return 0;

	} else if (ch == LAX_Enter) {
		LaunchEditColor();
		return 0;
	}

	return anXWindow::CharInput(ch, buffer, len, state, kb);
}


//! Set up inrect to correspond the region the colors should be drawn in.
/*! Also finds xn, yn, dx, and dy.
 */
void PaletteWindow::findInrect()
{
	double th = UIScale() * win_themestyle->normal->textheight();
	inrect.x = 0;
	inrect.y = th + 2*pad;
	inrect.width=  win_w;
	inrect.height= win_h - th - 2*pad;
	if (inrect.width<1) inrect.width=1;
	if (inrect.height<1) inrect.height=1;
	
	double aspect = double(inrect.height)/inrect.width;

	if (palette->num_columns_hint > 0) {
		xn = palette->num_columns_hint;
	} else {
		xn = int(ceil(sqrt(palette->colors.n / aspect)));
		if (xn == 0) xn = 1;
	}
	yn = (palette->colors.n-1) / xn + 1;
	if (yn <= 0) yn = 1;
	
	dx = double(inrect.width)/xn;
	dy = double(inrect.height)/yn;
	if (dx <= 0) dx = 1;
	if (dy <= 0) dy = 1;
}

//! Resize, then call findInrect().
int PaletteWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	int c=anXWindow::MoveResize(nx,ny,nw,nh);
	findInrect();
	return c;
}

//! Resize, then call findInrect().
int PaletteWindow::Resize(int nw,int nh)
{
	int c=anXWindow::Resize(nw,nh);
	findInrect();
	return c;
}


Laxkit::ShortcutHandler *PaletteWindow::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager = GetDefaultShortcutManager();
	sc = manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc = new ShortcutHandler(whattype());

	sc->Add(ACTION_NewColor,   'n',ControlMask,0,          "NewColor",   _("Add a new color to the palette"),NULL,0);
	sc->Add(ACTION_NewPalette, 'n',ControlMask|ShiftMask,0,"NewPalette", _("Create a new blank palette"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}


int PaletteWindow::PerformAction(int action)
{
	if (action == ACTION_NewColor) {
		if (palette->IsLocked()) {
			show_locked_hint = true;
			needtodraw = 1;
			return 0;
		}
		
		LaunchNewColor();
		return 0;

	} else if (action == ACTION_NewPalette) {
		Palette *p = new Palette();
		makestr(p->name, _("Colors"));
		p->SetFlags(GradientStrip::AsPalette, true);
		app->resourcemanager->AddResource("Palette", p, nullptr, isblank(p->name)?p->Id():p->name, nullptr, nullptr, nullptr, nullptr);
		UseThis(p, true);
		return 0;
	}

	return 1;
}

	
} //namespace Laxkit;


