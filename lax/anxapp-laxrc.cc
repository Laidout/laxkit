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

#include <lax/anxapp.h>
#include <lax/configured.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>

#include <sys/file.h>


#include <iostream>
using namespace std;
#define DBG


namespace Laxkit {

//--------------------------------laxrc docs------------------------------------
/*! \page laxrc The Laxrc File
 *
 *   Here is a sample laxrc file. Its location will have been compiled in to the Laxkit.
 *   The default location is ~/.lax/(version)/laxrc. The laxrc file will not by default
 *   be written to by Laxkit programs, only read from.
 *
 *   There are a number of settings to define, after which you may include basic profiles,
 *   which will overwrite any previously defined setting, as long as the application wants
 *   to use that profile.
 *
 *   \todo directories and fonts are still basically unimplemented..
 *   \todo many windows still need to pay attention to some of these settings, like pad and bevel
 * <pre>
 *
 * # this is a comment
 *  # these times are in milliseconds
 * firstclk       142  # <-- 1/7 of second,  for idling, time after button down to send messages
 * idleclk        67   # <-- 1/15 of second, for idling, time between idle clicks after the first one
 * dblclk         200  # <-- 1/5 of second,  time within which button must be clicked to be a double click
 * tooltips       500  #                     hover time until a tooltip pops up
 * default_padx   5    #                     default pixel pad around text horizontally
 * default_pady   5    #                     default pixel pad around text vertically
 * default_border 1    #                     default border width
 * bevel          2    #                     default bevel width
 *
 *
 * textfont    Courier-10  #font used for text edits, this is a fontconfig pattern string
 * controlfont Sans-9      #font used for menus, buttons, massage boxes, etc.
 *
 *
 * directories
 *   colorprofiles /usr/local/colorprofiles:/more/profiles
 *   extrafontdir  /in/addition/to/font/config/dirs:/another/dir
 *   imagedir      /blah/images
 *
 * colors  #anything at this base level will be the new default colors for everything
 *   panel
 *     fg         255 0 0
 *     bg         31 31 31
 *     hfg        255 0 255
 *     hbg        127 127 127
 *     moverfg    255 128 128
 *     moverbg    50 50 50
 *     grayedfg   128 0 0
 *     color1     64 64 64  #an extra color, for scroller slider, for instance
 *     color2     64 64 64
 *   menu
 *     fg         255 0 0
 *     bg         31 31 31
 *     hfg        255 0 0
 *     hbg        127 127 127
 *     moverfg    255 128 128
 *     moverbg    50 50 50
 *     grayedfg   128 0 0
 *     color1     64 64 64
 *     color2     64 64 64
 *   edits
 *     fg         255 0 0
 *     bg         0 0 0
 *     hfg        255 0 255
 *     hbg        127 127 127
 *     moverfg    255 0 0
 *     moverbg    30 30 30
 *     grayedfg   128 0 0
 *     color1     64 64 64
 *     color2     64 64 64
 *   buttons
 *     fg         255 0 0
 *     bg         31 31 31
 *     hfg        255 0 255
 *     hbg        127 127 127
 *     moverfg    255 128 128
 *     moverbg    50 50 50
 *     grayedfg   128 0 0
 *     color1     64 64 64
 *     color2     64 64 64
 *
 * profile light #You can define any profile you want, rather than colors for everything
 *   firstclk 142
 *   dblclk   200
 *   colors
 *     (...all the color categories, panel, menu, etc...)
 *
 * </pre>
 *
*/


//--------------------------------laxrc functions ------------------------------------
//! Set the builtin Laxkit colors.
/*! If app_profile is "Dark" then set a dark color scheme. If app_profile is NULL or "Light",
 * then set a more usual gray/white scheme.
 */
void anXApp::setupdefaultcolors()
{
	//default_border_width= 1;
	//default_padx        = 5;
	//default_pady        = 5;
	//default_bevel       = 2;

	//color_tooltip_fg    = rgbcolor(0,0,0);
	//color_tooltip_bg    = rgbcolor(255,255,128);
	//color_inactiveborder= rgbcolor(128,128,128);
	//color_activeborder  = rgbcolor(90,90,90);

	if (!theme) {
		theme = new Theme(app_profile ? app_profile : "Light");
		theme->AddDefaults(app_profile ? app_profile : "Light");
	}
}

//! Read in the given rc file, or the default if filename==NULL.
/*! This does not reset all values, but does overwrite any values found.
 *
 * The default is LAX_CONFIG_DIRECTORY/laxrc, or ./laxrc if that fails.
 *
 * Return 0 for success, nonzero for error, such as file not readable.
 *
 * \todo should probably flock() the file while reading in...
 */
int anXApp::getlaxrc(const char *filename, const char *profile)
{
	FILE *f=NULL;
	if (filename==NULL) {
		char ff[strlen(LAX_CONFIG_DIRECTORY)+10];
		sprintf(ff,"%s/laxrc",LAX_CONFIG_DIRECTORY);
		f=fopen(ff,"r");
		if (!f) f=fopen("laxrc","r");
	} else {
		f=fopen(filename,"r");
	}
	if (!f) {
		DBG cerr << "laxrc not found: "<<(filename?filename:"default location")<<endl;
		return 1;
	}

	//resources.dump_in(f,0);*** resources dealt with separately..
	//  to configured.h-->CONFIG_DIRECTORY/autolaxkitrc

	Attribute att;
	att.dump_in(f,0,NULL);
	dump_in_rc(&att,profile);

	fclose(f);
	return 0;
}

//! Output an rc file based on current colors.
/*! If what==-1, then dump out with comments.
 *
 * If profile!=NULL, then write out "profile name" where name is the profile string,
 * and everything is indented a further 2 spaces. Else everything is dumped out
 * with current indent.
 */
void anXApp::dump_out_rc(FILE *f, const char *profile, int indent, int what)
{
	if (!f) return;
	char spc[indent+3]; memset(spc,' ',indent+2); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sprofile profilename #Which theme to use. Should be before defs of any actual themes.\n",spc);
		fprintf(f,"%stheme ThemeName     #You can have any number of themes, profile chooses which one to actually use.\n",spc);
		if (theme) theme->dump_out(f,indent+2, -1, NULL);
		else {
			Theme t;
			t.dump_out(f,indent+2, -1, NULL);
		}
		return;
	}

	if (profile) {
		fprintf(f,"%sprofile %s\n",spc, theme && theme->name ? theme->name : "Light");
		spc[indent]=' ';
		spc[indent+1]=' ';
		spc[indent+2]='\0';
	}

	if (theme) {
		fprintf(f,"%stheme\n", spc);
		theme->dump_out(f, indent+2, what, NULL); //null is context
	}
}


/*! Used to scan in laxrc values.
 */
void anXApp::dump_in_rc(Attribute *att, const char *profile)
{
	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"profile")) {
			if (!value || !profile || strcmp(value,profile)) continue;
			profile = value;

		} else if (!strcmp(name,"theme")) {
			if (isblank(profile) || (!isblank(profile) && value && !strcmp(profile, value))) {
				//Theme *thme = new Theme(profile);
				//thme->dump_in_atts(att->attributes.e[c], 0, NULL);
				//if (theme) theme->dec_count();
				//theme = thme;
				//----
				if (!theme) theme = new Theme(profile);
				theme->dump_in_atts(att->attributes.e[c], 0, NULL);
			}
		}
	}
}


} // namespace Laxkit
