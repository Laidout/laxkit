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



#include <lax/themes.h>
#include <lax/anxapp.h>
#include <lax/fileutils.h>
#include <lax/language.h>
#include <lax/version.h>


//template implementations
#include <lax/refptrstack.cc>


#include <iostream>

#define DBG
using namespace std;

using namespace LaxFiles;


namespace Laxkit {


//----------------------------- helpers ---------------------------------------

const char *window_category_name(int category) 
{
	if      (category == THEME_Panel)   return "panel";
	else if (category == THEME_Menu)    return "menu";
	else if (category == THEME_Edit)    return "edit";
	else if (category == THEME_Button)  return "button";
	else if (category == THEME_Tooltip) return "tooltip";
	return NULL;
}


//----------------------------- WindowStyle ---------------------------------------


/*! \class WindowStyle
 * Holds styles specific to one category of windows, like panel, edit, menu, or button.
 */


WindowStyle::WindowStyle(int ncategory)
{
	category = ncategory;

	normal    = NULL;
	bold      = NULL;
	italic    = NULL;
	monospace = NULL;

	diff_bg_hover     = .05;
	diff_bg_highlight = .05;
	diff_bg_focus     = .05;
	diff_bg_alternate = .05;
	diff_fg_hover     = 0.;
	diff_fg_highlight = 0.;
	diff_fg_focus     = 0.;
	diff_fg_alternate = 0.;

	diff_shadow      = -.1;
	diff_highlight   = .1;

	SetDefaultColors("Light");
}

WindowStyle::WindowStyle(const WindowStyle &l)
{
	category  = l.category;

	diff_bg_hover     = l.diff_bg_hover    ;
	diff_bg_highlight = l.diff_bg_highlight;
	diff_bg_focus     = l.diff_bg_focus    ;
	diff_bg_alternate = l.diff_bg_alternate;
	diff_fg_hover     = l.diff_fg_hover    ;
	diff_fg_highlight = l.diff_fg_highlight;
	diff_fg_focus     = l.diff_fg_focus    ;
	diff_fg_alternate = l.diff_fg_alternate;

	diff_shadow      = l.diff_shadow     ;
	diff_highlight   = l.diff_highlight  ;


    fg        = l.fg;
    fghl      = l.fghl;
    fghover   = l.fghover;
    fggray    = l.fggray;

    bg        = l.bg;
    bghl      = l.bghl;
    bghover   = l.bghover;
    bggray    = l.bggray;

    color1    = l.color1;
    color2    = l.color2;
    activate  = l.activate;
    deactivate= l.deactivate;

    active_border   = l.active_border;
    inactive_border = l.inactive_border; 

	normal    = NULL;
	bold      = NULL;
	italic    = NULL;
	monospace = NULL;
	SetFonts(l.normal, l.bold, l.italic, l.monospace);
}

WindowStyle &WindowStyle::operator=(WindowStyle &l)
{
	category  = l.category;

	diff_bg_hover     = l.diff_bg_hover    ;
	diff_bg_highlight = l.diff_bg_highlight;
	diff_bg_focus     = l.diff_bg_focus    ;
	diff_bg_alternate = l.diff_bg_alternate;
	diff_fg_hover     = l.diff_fg_hover    ;
	diff_fg_highlight = l.diff_fg_highlight;
	diff_fg_focus     = l.diff_fg_focus    ;
	diff_fg_alternate = l.diff_fg_alternate;

	diff_shadow      = l.diff_shadow     ;
	diff_highlight   = l.diff_highlight  ;

    fg        = l.fg;
    fghl      = l.fghl;
    fghover   = l.fghover;
    fggray    = l.fggray;

    bg        = l.bg;
    bghl      = l.bghl;
    bghover   = l.bghover;
    bggray    = l.bggray;

    color1    = l.color1;
    color2    = l.color2;
    activate  = l.activate;
    deactivate= l.deactivate;

    active_border   = l.active_border;
    inactive_border = l.inactive_border;

	SetFonts(l.normal, l.bold, l.italic, l.monospace);

	return l;
} 

WindowStyle::~WindowStyle()
{
	if (normal)    normal   ->dec_count();
	if (bold)      bold     ->dec_count();
	if (italic)    italic   ->dec_count();
	if (monospace) monospace->dec_count();
}

WindowStyle *WindowStyle::duplicate()
{
	WindowStyle *ncolor = new WindowStyle(*this);
	return ncolor;
}

Attribute *WindowStyle::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (what==-1) {
		return att;
	}

	char scratch[200];

	const char *cat = window_category_name(category);
	if (!cat) cat = Id();
	att->push("category", cat);


	att->push("diff_bg_hover"    , diff_bg_hover    );
	att->push("diff_bg_highlight", diff_bg_highlight);
	att->push("diff_bg_focus"    , diff_bg_focus    );
	att->push("diff_bg_alternate", diff_bg_alternate);
	att->push("diff_fg_hover"    , diff_fg_hover    );
	att->push("diff_fg_highlight", diff_fg_highlight);
	att->push("diff_fg_focus"    , diff_fg_focus    );
	att->push("diff_fg_alternate", diff_fg_alternate);
	att->push("diff_shadow"      , diff_shadow      );
	att->push("diff_highlight"   , diff_highlight   );

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", fg.Red(),fg.Green(),fg.Blue(),fg.Alpha());
	att->push("fg", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", fghover.Red(),fghover.Green(),fghover.Blue(),fghover.Alpha());
	att->push("fghover", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", fghl.Red(),fghl.Green(),fghl.Blue(),fghl.Alpha());
	att->push("fghl", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", fggray.Red(),fggray.Green(),fggray.Blue(),fggray.Alpha());
	att->push("fggray", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", bg.Red(),bg.Green(),bg.Blue(),bg.Alpha());
	att->push("bg", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", bghover.Red(),bghover.Green(),bghover.Blue(),bghover.Alpha());
	att->push("bghover", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", bghl.Red(),bghl.Green(),bghl.Blue(),bghl.Alpha());
	att->push("bghl", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", bggray.Red(),bggray.Green(),bggray.Blue(),bggray.Alpha());
	att->push("bggray", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", color1.Red(),color1.Green(),color1.Blue(),color1.Alpha());
	att->push("color1", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", color2.Red(),color2.Green(),color2.Blue(),color2.Alpha());
	att->push("color2", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", activate.Red(),activate.Green(),activate.Blue(),activate.Alpha());
	att->push("activate", scratch);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", deactivate.Red(),deactivate.Green(),deactivate.Blue(),deactivate.Alpha());
	att->push("deactivate", scratch);


	if (normal) {
		Attribute *att2 = att->pushSubAtt("font_normal");
		normal->dump_out_atts(att2, 0, context);
	}
	if (bold) {
		Attribute *att2 = att->pushSubAtt("font_bold");
		bold->dump_out_atts(att2, 0, context);
	}
	if (italic) {
		Attribute *att2 = att->pushSubAtt("font_italic");
		italic->dump_out_atts(att2, 0, context);
	}
	if (monospace) {
		Attribute *att2 = att->pushSubAtt("font_monospace");
		monospace->dump_out_atts(att2, 0, context);
	}

	return att;
}

void WindowStyle::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
    if (!att) return;
    char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

	    if (!strcmp(name,"category")) {
			Id(value);

			if (!strcmp(value, "panel")) {
				category = THEME_Panel;
			} else if (!strcmp(value, "menu")) {
				category = THEME_Menu;
			} else if (!strcmp(value, "edit")) {
				category = THEME_Edit;
			} else if (!strcmp(value, "button")) {
				category = THEME_Button;
			} else if (!strcmp(value, "tooltip")) {
				category = THEME_Tooltip;
			}

		} else if (!strcmp(name,"diff_bg_hover")) {
			DoubleAttribute(value, &diff_bg_hover);

		} else if (!strcmp(name,"diff_bg_highlight")) {
			DoubleAttribute(value, &diff_bg_highlight);

		} else if (!strcmp(name,"diff_bg_focus")) {
			DoubleAttribute(value, &diff_bg_focus);

		} else if (!strcmp(name,"diff_bg_alternate")) {
			DoubleAttribute(value, &diff_bg_alternate);

		} else if (!strcmp(name,"diff_fg_hover")) {
			DoubleAttribute(value, &diff_fg_hover);

		} else if (!strcmp(name,"diff_fg_highlight")) {
			DoubleAttribute(value, &diff_fg_highlight);

		} else if (!strcmp(name,"diff_fg_focus")) {
			DoubleAttribute(value, &diff_fg_focus);

		} else if (!strcmp(name,"diff_fg_alternate")) {
			DoubleAttribute(value, &diff_fg_alternate);

		} else if (!strcmp(name,"diff_shadow")) {
			DoubleAttribute(value, &diff_shadow);

		} else if (!strcmp(name,"diff_highlight")) {
			DoubleAttribute(value, &diff_highlight);


		} else if (!strcmp(name,"font_normal")) {
			LaxFont *newfont = anXApp::app->fontmanager->dump_in_font(att->attributes.e[c], context);
			if (newfont) {
				NormalFont(newfont);
				newfont->dec_count();
			}

		} else if (!strcmp(name,"font_bold")) {
			LaxFont *newfont = anXApp::app->fontmanager->dump_in_font(att->attributes.e[c], context);
			if (newfont) {
				BoldFont(newfont);
				newfont->dec_count();
			}

		} else if (!strcmp(name,"font_italic")) {
			LaxFont *newfont = anXApp::app->fontmanager->dump_in_font(att->attributes.e[c], context);
			if (newfont) {
				ItalicFont(newfont);
				newfont->dec_count();
			}

		} else if (!strcmp(name,"font_monospace")) {
			LaxFont *newfont = anXApp::app->fontmanager->dump_in_font(att->attributes.e[c], context);
			if (newfont) {
				MonospaceFont(newfont);
				newfont->dec_count();
			}



		} else {
 			 //try for a color
			ScreenColor color;
			if (SimpleColorAttribute(value,NULL,&color,NULL)==0) {

				if (!strcmp(name,"fg")) {
					fg = color;
				} else if (!strcmp(name,"fghl")) {
					fghl = color;
				} else if (!strcmp(name,"fghover")) {
					fghover = color;
				} else if (!strcmp(name,"fggray")) {
					fggray = color;

				} else if (!strcmp(name,"bg")) {
					bg = color;
				} else if (!strcmp(name,"bghl")) {
					bghl = color;
				} else if (!strcmp(name,"bghover")) {
					bghover = color;
				} else if (!strcmp(name,"bggray")) {
					bggray = color;

				} else if (!strcmp(name,"color1")) {
					color1 = color;
				} else if (!strcmp(name,"color2")) {
					color2 = color;
				} else if (!strcmp(name,"activate")) {
					activate = color;
				} else if (!strcmp(name,"deactivate")) {
					deactivate = color;
				}
			}
		}
	}
}

double WindowStyle::GetValue(const char *what)
{
	if (!strcmp(what, "diff_bg_hover"    )) return diff_bg_hover;
	if (!strcmp(what, "diff_bg_highlight")) return diff_bg_highlight;
	if (!strcmp(what, "diff_bg_focus"    )) return diff_bg_focus;
	if (!strcmp(what, "diff_bg_alternate")) return diff_bg_alternate;
	if (!strcmp(what, "diff_fg_hover"    )) return diff_fg_hover;
	if (!strcmp(what, "diff_fg_highlight")) return diff_fg_highlight;
	if (!strcmp(what, "diff_fg_focus"    )) return diff_fg_focus;
	if (!strcmp(what, "diff_fg_alternate")) return diff_fg_alternate;
	if (!strcmp(what, "diff_shadow"      )) return diff_shadow;
	if (!strcmp(what, "diff_highlight"   )) return diff_highlight;
	return 0;
}

double WindowStyle::GetValue(ThemeThings what)
{
	if (what == THEME_Diff_BG_Hover    ) return diff_bg_hover;
	if (what == THEME_Diff_BG_Highlight) return diff_bg_highlight;
	if (what == THEME_Diff_BG_Focus    ) return diff_bg_focus;
	if (what == THEME_Diff_BG_Alternate) return diff_bg_alternate;
	if (what == THEME_Diff_FG_Hover    ) return diff_fg_hover;
	if (what == THEME_Diff_FG_Highlight) return diff_fg_highlight;
	if (what == THEME_Diff_FG_Focus    ) return diff_fg_focus;
	if (what == THEME_Diff_FG_Alternate) return diff_fg_alternate;
	if (what == THEME_Diff_Shadow      ) return diff_shadow;
	if (what == THEME_Diff_Highlight   ) return diff_highlight;
	return 0;
}

/*! Set to nfont if nfont!=NULL.
 */
int WindowStyle::NormalFont(LaxFont *nfont)
{
	if (!nfont || nfont == normal) return 0;
	if (normal) normal->dec_count();
	normal = nfont;
	normal->inc_count();
	return 1;
}

/*! Set to nfont if nfont!=NULL.
 */
int WindowStyle::BoldFont(LaxFont *nfont)
{
	if (!nfont || nfont == bold) return 0;
	if (bold) bold->dec_count();
	bold = nfont;
	bold->inc_count();
	return 1;
}

/*! Set to nfont if nfont!=NULL.
 */
int WindowStyle::ItalicFont(LaxFont *nfont)
{
	if (!nfont || nfont == italic) return 0;
	if (italic) italic->dec_count();
	italic = nfont;
	italic->inc_count();
	return 1;
}

/*! Set to nfont if nfont!=NULL.
 */
int WindowStyle::MonospaceFont(LaxFont *nfont)
{
	if (!nfont || nfont == monospace) return 0;
	if (monospace) monospace->dec_count();
	monospace = nfont;
	monospace->inc_count();
	return 1;
}

/*! If not null, replace the old font with the new. If the new is null, then don't replace.
 * Incs counts. Returns number of fonts changed. If the font is the same as current, that doesn't count.
 */
int WindowStyle::SetFonts(LaxFont *nnormal, LaxFont *nbold, LaxFont *nitalic, LaxFont *nmonospace)
{
	int n=0;

	if (NormalFont   (nnormal)   ) n++;
	if (BoldFont     (nbold)     ) n++;
	if (ItalicFont   (nitalic)   ) n++;
	if (MonospaceFont(nmonospace)) n++;

	return n;
}

/*! Install default values for type considering category.
 * Current defaults are "Light", "Dark", and "Gray" when category is
 * THEME_Panel, THEME_Edit, THEME_Menu, THEME_Button.
 *
 * Return 0 for success, or 1 for unknown type+category.
 */
int WindowStyle::SetDefaultColors(const char *type)
{
	if (strcasecmp(type, "Light") && strcasecmp(type, "Dark") && strcasecmp(type, "Gray")) return 1;
	if (category != THEME_Panel && category != THEME_Edit && category != THEME_Menu
			&& category != THEME_Button && category != THEME_Tooltip) return 1;


	active_border  .rgbf(.35,.35,.35);
	inactive_border.rgbf( .5, .5, .5); 

	if (!strcasecmp(type, "Dark")) {
		if (category == THEME_Panel || category == THEME_Tooltip) {
			fg        .rgb8(255,0,0);
			bg        .rgb8(31,31,31);
			fghl      .rgb8(255,0,255);
			bghl      .rgb8(127,127,127);
			fghover   .rgb8(255,0,0);
			bghover   .rgb8(51,51,51);
			fggray    .rgb8(128,0,0);
			bggray    .rgb8(31,31,31);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Menu) {
			fg        .rgb8(255,0,0);
			bg        .rgb8(31,31,31);
			fghl      .rgb8(255,0,0);
			bghl      .rgb8(127,127,127);
			fghover   .rgb8(255,0,0);
			bghover   .rgb8(51,51,51);
			fggray    .rgb8(128,0,0);
			bggray    .rgb8(31,31,31);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Edit) {
			fg        .rgb8(255,0,0);
			bg        .rgb8(0,0,0);
			fghl      .rgb8(255,0,255);
			bghl      .rgb8(127,127,127);
			fghover   .rgb8(255,0,0);
			bghover   .rgb8(51,51,51);
			fggray    .rgb8(128,0,0);
			bggray    .rgb8(0,0,0);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Button) {
			fg        .rgb8(255,0,0);
			bg        .rgb8(41,41,41);
			fghl      .rgb8(255,0,255);
			bghl      .rgb8(127,127,127);
			fghover   .rgb8(255,0,0);
			bghover   .rgb8(144,144,144);
			fggray    .rgb8(128,0,0);
			bggray    .rgb8(41,41,41);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);
		}

	} else if (!strcasecmp(type, "Gray")) {
		if (category == THEME_Panel || category == THEME_Tooltip) {
			fg        .rgb8(  0,  0,  0);
			bg        .rgb8(100,100,100);
			fghl      .rgb8(255,255,255);
			bghl      .rgb8(50,50,50);
			fghover   .rgb8(0,0,0);
			bghover   .rgb8(150,150,150);
			fggray    .rgb8(200,200,200);
			bggray    .rgb8(100,100,100);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Menu) {
			fg        .rgb8(  0,  0,  0);
			bg        .rgb8(100,100,100);
			fghl      .rgb8(175,175,175);
			bghl      .rgb8(50,50,50);
			fghover   .rgb8(0,0,0);
			bghover   .rgb8(150,150,150);
			fggray    .rgb8(200,200,200);
			bggray    .rgb8(100,100,100);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Edit) {
			fg        .rgb8(0,0,0);
			bg        .rgb8(128,128,128);
			fghl      .rgb8(200,200,200);
			bghl      .rgb8( 64, 64, 64);
			fghover   .rgb8(0,0,0);
			bghover   .rgb8(51,51,51);
			fggray    .rgb8(128,0,0);
			bggray    .rgb8(128,128,128);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Button) {
			fg        .rgb8(0,0,0);
			bg        .rgb8(128,128,128);
			fghl      .rgb8(0,0,0);
			bghl      .rgb8(200,200,200);
			fghover   .rgb8(0,0,0);
			bghover   .rgb8(150,150,150);
			fggray    .rgb8(200,200,200);
			bggray    .rgb8(128,128,128);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);
		}

    } else { //Light
		if (category == THEME_Panel || category == THEME_Tooltip) {
			fg        .rgbf(.13,.13,.13);
			bg        .rgbf(.75,.75,.75);
			fghl      .rgbf( 0., 0., 0.);
			bghl      .rgbf( .5, .5, .5);
			fghover   .rgbf(.13,.13,.13);
			bghover   .rgbf(.64,.64,.64);
			fggray    .rgbf( .4, .4, .4);
			bggray    .rgbf(.75,.75,.75);
			color1    .rgbf( .5, .5, .5);
			color2    .rgbf( .5, .5, .5);
			activate  .rgbf(  0,.78,  0);
			deactivate.rgbf( 1., .4, .4);

		} else if (category == THEME_Menu) {
			fg        .rgb8(0,0,0);
			bg        .rgb8(192,192,192);
			fghl      .rgb8(255,0,0);
			bghl      .rgb8(127,127,127);
			fghover   .rgb8(16,16,16);
			bghover   .rgb8(150,150,150);
			fggray    .rgb8(100,100,100);
			bggray    .rgb8(192,192,192);
			color1    .rgb8(128,128,128);
			color2    .rgb8(128,128,128);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Edit) {
			fg        .rgb8(64,64,64);
			bg        .rgb8(255,255,255);
			fghl      .rgb8(0,0,0);
			bghl      .rgb8(192,192,192);
			fghover   .rgb8(64,64,64);
			bghover   .rgb8(255,255,255);
			fggray    .rgb8(100,100,100);
			bggray    .rgb8(255,255,255);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);

		} else if (category == THEME_Button) {
			fg        .rgb8(0,0,0);
			bg        .rgb8(192,192,192);
			fghl      .rgb8(0,0,0);
			bghl      .rgb8(127,127,127);
			fghover   .rgb8(0,0,0);
			bghover   .rgb8(164,164,164);
			fggray    .rgb8(100,100,100);
			bggray    .rgb8(192,192,192);
			color1    .rgb8(64,64,64);
			color2    .rgb8(64,64,64);
			activate  .rgb8(  0,200,  0);
			deactivate.rgb8(255,100,100);
		}
	}

	if (category == THEME_Tooltip) {
		 //piggy backs THEME_Panel
		fg.rgb8(0,0,0);
		bg.rgb8(255,255,128);
	}

	return 0;
}


//----------------------------- Theme ---------------------------------------
/*! \class Theme 
 * Simple class to store common window styling.
 */


/*! Create a blank theme with no defined styles.
 * nname is only assigned, it is not interpreted as anything.
 */
Theme::Theme(const char *nname)
{
	name = newstr(nname);
	filename = NULL;

	default_border_width = 1;
	default_padx = 5;
	default_pady = 5;
	default_bevel = 5;
	interface_scale = 1; //all sizes should be multiplied with this

	firstclk = 1000/7;
	dblclk   = 1000/5;
	idleclk  = 1000/15;

	tooltips = 1000;

	iconmanager = new IconManager();
}

Theme::~Theme()
{
	if (iconmanager) iconmanager->dec_count();
	delete[] name;
	delete[] filename;
}

/*! Return the default theme for "Light", "Gray", or "Dark".
 * If themename is not one of those, then return NULL.
 */
Theme *Theme::DefaultTheme(const char *themename) 
{
	if (strcasecmp(themename, "Light") && strcasecmp(themename, "Gray") && strcasecmp(themename, "Dark")) return NULL;
	Theme *theme = new Theme(themename);
	theme->AddDefaults(themename);
	return theme;
}

/*! Install default theme styles. Built in which are "Light" (the default), "Dark", and "Gray".
 * Return 0 for success or nonzero error.
 */
int Theme::AddDefaults(const char *which)
{
	if (!which) which = "Light";

	if (!strcasecmp(which, "Light") || !strcasecmp(which, "Dark") || !strcasecmp(which, "Gray")) {

		LaxFont *font = (anXApp::app ? anXApp::app->defaultlaxfont : NULL);
		WindowStyle *s = NULL;
		
		const int categories[] = { THEME_Panel, THEME_Edit, THEME_Menu, THEME_Button, THEME_Tooltip, 0 };

		for (int c=0; categories[c]; c++) {
			s = new WindowStyle(categories[c]);
			s->SetDefaultColors(which);
			if (font) s->SetFonts(font, font, font, font);
			styles.push(s);
			s->dec_count();
		}

		return 0;
	}

	return 1;
}

/*! Shortcut for int(GetDouble(what)+.5).
 */
int Theme::GetInt(int what)
{
	return GetDouble(what)+.5;
}

double Theme::GetDouble(int what)
{
	if (what == THEME_Border_Width) return default_border_width;
	if (what == THEME_Padx) return default_padx;
	if (what == THEME_Pady) return default_pady;
	if (what == THEME_Bevel) return default_bevel;

	if (what == THEME_Firstclk) return firstclk;
	if (what == THEME_Dblclk) return dblclk;
	if (what == THEME_Idleclk) return idleclk;

	return 0;
}

LaxImage *Theme::GetIcon(const char *key)
{
	// ***
	return NULL;
}

/*! Return the WindowStyle object for category, or NULL if not found.
 */
WindowStyle *Theme::GetStyle(int category)
{
	for (int c=0; c<styles.n; c++) {
		if (category == styles.e[c]->category) return styles.e[c];
	}
	return NULL;
}

/*! Returns pointer to the object in the style.
 * You must inc_count on the returned object if you want to keep it around.
 */
LaxFont *Theme::GetFont(int category, int what)
{
	for (int c=0; c<styles.n; c++) {
		if (category == styles.e[c]->category) {
			if      (what == THEME_Font_Normal)    return styles.e[c]->normal;
			else if (what == THEME_Font_Bold)      return styles.e[c]->bold;
			else if (what == THEME_Font_Italic)    return styles.e[c]->italic;
			else if (what == THEME_Font_Monospace) return styles.e[c]->monospace;
		}
	}

	return NULL;
}

/*! return 0 for not found.
 */
unsigned long Theme::GetColor(int category, int what)
{
	ScreenColor *color = GetScreenColor(category, what);
	if (color) return color->Pixel();
	return 0;
}

/*! Return NULL for not found.
 */
ScreenColor *Theme::GetScreenColor(int category, int what)
{
	for (int c=0; c<styles.n; c++) {
		if (category == styles.e[c]->category) {
			if      (what == THEME_FG)              return &styles.e[c]->fg;
			else if (what == THEME_FG_Hover)        return &styles.e[c]->fghover;
			else if (what == THEME_FG_Highlight)    return &styles.e[c]->fghl;
			else if (what == THEME_FG_Gray)         return &styles.e[c]->fggray;
			else if (what == THEME_BG)              return &styles.e[c]->bg;
			else if (what == THEME_BG_Hover)        return &styles.e[c]->bghover;
			else if (what == THEME_BG_Highlight)    return &styles.e[c]->bghl;
			else if (what == THEME_BG_Gray)         return &styles.e[c]->bggray;
			else if (what == THEME_Color1)          return &styles.e[c]->color1;
			else if (what == THEME_Color2)          return &styles.e[c]->color2;
			else if (what == THEME_Activate)        return &styles.e[c]->activate;
			else if (what == THEME_Deactivate)      return &styles.e[c]->deactivate;
			else if (what == THEME_Border_Active)   return &styles.e[c]->active_border;
			else if (what == THEME_Border_Inactive) return &styles.e[c]->inactive_border; 
		}
	}

	return NULL;
}

void Theme::dump_out(FILE *f,int indent,int what,DumpContext *context)
{
	Attribute att;
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	fprintf(f, "%s#Laxkit %s Theme\n", LAXKIT_VERSION, spc);
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *Theme::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (!att) att = new Attribute;

	if (what==-1) {
		//att->push("firstclk; //how long after first click to wait before idle "clicks", in ms
		//att->push("dblclk;   //time between clicks that counts as a double click, in ms
		//att->push("idleclk;  //time between idle fake clicks, in ms
		return att;
	}

	att->push("name", name);

	for (int c=0; c<iconmanager->NumPaths(); c++) {
		att->push("icon_dir", iconmanager->GetPath(c));
	} 

	att->push("default_border_width", default_border_width);
	att->push("default_padx", default_padx);
	att->push("default_pady", default_pady);
	att->push("default_bevel", default_bevel);
	att->push("interface_scale", interface_scale);

	att->push("first_click",  (int)firstclk);  att->Top()->Comment("milliseconds before idle clicking after first click");
	att->push("double_click", (int)dblclk);    att->Top()->Comment("millisecond limit for double click");
	att->push("idle_click",   (int)idleclk);   att->Top()->Comment("milliseconds between idle clicks"); 
	att->push("tooltips",     tooltips);       att->Top()->Comment("#millisecond delay before popping up, or 0 for never");

	for (int c=0; c<styles.n; c++) {
		WindowStyle *style = styles.e[c];
		Attribute *att2 = att->pushSubAtt("windowstyle");
		style->dump_out_atts(att2, what, context);
	}

	return att;
}

void Theme::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
    if (!att) return;
    char *name,*value;

    for (int c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;
		
        if (!strcmp(name,"name")) {
			if (!isblank(value)) makestr(this->name, value);

		} else if (!strcmp(name,"windowstyle")) {
			WindowStyle *style = new WindowStyle();
			style->dump_in_atts(att->attributes.e[c], flag, context);
			styles.push(style);
			style->dec_count();

        } else if (!strcmp(name,"first_click")) {
            UIntAttribute(value,&firstclk);

        } else if (!strcmp(name,"double_click")) {
            UIntAttribute(value,&dblclk);

        } else if (!strcmp(name,"idle_click")) {
            UIntAttribute(value,&idleclk);

        } else if (!strcmp(name,"tooltips")) {
            IntAttribute(value,&tooltips);

        } else if (!strcmp(name,"bevel")) {
            DoubleAttribute(value,&default_bevel);

        } else if (!strcmp(name,"default_border_width")) {
            DoubleAttribute(value,&default_border_width);

        } else if (!strcmp(name,"default_padx")) {
            DoubleAttribute(value,&default_padx);

        } else if (!strcmp(name,"default_pady")) {
            DoubleAttribute(value,&default_pady);

        } else if (!strcmp(name,"interface_scale")) {
            DoubleAttribute(value,&interface_scale);

        } else if (!strcmp(name,"default_pady")) {
			double d;
            if (DoubleAttribute(value,&d)) { default_padx = default_pady = d; }
 
        } else if (!strcmp(name,"icon_dir")) {
			if (file_exists(value,1,NULL) == S_IFDIR) iconmanager->AddPath(value);
		}
	}

}


} //namespace Laxkit

