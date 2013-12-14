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
//    Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/anxapp.h>
#include <lax/configured.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>

#include <lax/lists.cc>

#include <sys/file.h>


#include <iostream>
using namespace std;
#define DBG 

using namespace LaxFiles;

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
	if (!color_panel)   color_panel  =new WindowColors;
	if (!color_menu)    color_menu   =new WindowColors;
	if (!color_edits)   color_edits  =new WindowColors;
	if (!color_buttons) color_buttons=new WindowColors;


	default_border_width= 1;
	default_padx        = 5;
	default_pady        = 5;
	default_bevel       = 2;

	color_tooltip_fg    = rgbcolor(0,0,0);
	color_tooltip_bg    = rgbcolor(255,255,128);
	color_inactiveborder= rgbcolor(128,128,128);
	color_activeborder  = rgbcolor(0,0,0);

	if (app_profile && !strcmp(app_profile,"Dark")) {
		color_panel->fg      =rgbcolor(255,0,0);
		color_panel->bg      =rgbcolor(31,31,31);
		color_panel->hfg     =rgbcolor(255,0,255);
		color_panel->hbg     =rgbcolor(127,127,127);
		color_panel->moverfg =rgbcolor(255,0,0);
		color_panel->moverbg =rgbcolor(51,51,51);
		color_panel->grayedfg=rgbcolor(128,0,0);
		color_panel->color1  =rgbcolor(64,64,64);
		color_panel->color2  =rgbcolor(64,64,64);

		color_menu->fg      =rgbcolor(255,0,0);
		color_menu->bg      =rgbcolor(31,31,31);
		color_menu->hfg     =rgbcolor(255,0,0);
		color_menu->hbg     =rgbcolor(127,127,127);
		color_menu->moverfg =rgbcolor(255,0,0);
		color_menu->moverbg =rgbcolor(51,51,51);
		color_menu->grayedfg=rgbcolor(128,0,0);
		color_menu->color1  =rgbcolor(64,64,64);
		color_menu->color2  =rgbcolor(64,64,64);

		color_edits->fg      =rgbcolor(255,0,0);
		color_edits->bg      =rgbcolor(0,0,0);
		color_edits->hfg     =rgbcolor(255,0,255);
		color_edits->hbg     =rgbcolor(127,127,127);
		color_edits->moverfg =rgbcolor(255,0,0);
		color_edits->moverbg =rgbcolor(51,51,51);
		color_edits->grayedfg=rgbcolor(128,0,0);
		color_edits->color1  =rgbcolor(64,64,64);
		color_edits->color2  =rgbcolor(64,64,64);

		color_buttons->fg      =rgbcolor(255,0,0);
		color_buttons->bg      =rgbcolor(41,41,41);
		color_buttons->hfg     =rgbcolor(255,0,255);
		color_buttons->hbg     =rgbcolor(127,127,127);
		color_buttons->moverfg =rgbcolor(255,0,0);
		color_buttons->moverbg =rgbcolor(144,144,144);
		color_buttons->grayedfg=rgbcolor(128,0,0);
		color_buttons->color1  =rgbcolor(64,64,64);
		color_buttons->color2  =rgbcolor(64,64,64);

    } else { //Light
		color_panel->fg      =rgbcolor(32,32,32);
		color_panel->bg      =rgbcolor(192,192,192);
		color_panel->hfg     =rgbcolor(0,0,0);
		color_panel->hbg     =rgbcolor(127,127,127);
		color_panel->moverfg =rgbcolor(16,16,16);
		color_panel->moverbg =rgbcolor(150,150,150);
		color_panel->grayedfg=rgbcolor(100,100,100);
		color_panel->color1  =rgbcolor(128,128,128);
		color_panel->color2  =rgbcolor(128,128,128);

		color_menu->fg      =rgbcolor(0,0,0);
		color_menu->bg      =rgbcolor(192,192,192);
		color_menu->hfg     =rgbcolor(255,0,0);
		color_menu->hbg     =rgbcolor(127,127,127);
		color_menu->moverfg =rgbcolor(16,16,16);
		color_menu->moverbg =rgbcolor(150,150,150);
		color_menu->grayedfg=rgbcolor(100,100,100);
		color_menu->color1  =rgbcolor(128,128,128);
		color_menu->color2  =rgbcolor(128,128,128);

		color_edits->fg      =rgbcolor(64,64,64);
		color_edits->bg      =rgbcolor(255,255,255);
		color_edits->hfg     =rgbcolor(0,0,0);
		color_edits->hbg     =rgbcolor(127,127,127);
		color_edits->moverfg =rgbcolor(64,64,64);
		color_edits->moverbg =rgbcolor(255,255,255);
		color_edits->grayedfg=rgbcolor(100,100,100);
		color_edits->color1  =rgbcolor(64,64,64);
		color_edits->color2  =rgbcolor(64,64,64);

		color_buttons->fg      =rgbcolor(0,0,0);
		color_buttons->bg      =rgbcolor(192,192,192);
		color_buttons->hfg     =rgbcolor(0,0,0);
		color_buttons->hbg     =rgbcolor(127,127,127);
		color_buttons->moverfg =rgbcolor(0,0,0);
		color_buttons->moverbg =rgbcolor(164,164,164);
		color_buttons->grayedfg=rgbcolor(100,100,100);
		color_buttons->color1  =rgbcolor(64,64,64);
		color_buttons->color2  =rgbcolor(64,64,64);
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

	if (profile) {
		fprintf(f,"%sprofile %s\n",spc,profile); 
		spc[indent]=' ';
		spc[indent+1]=' ';
		spc[indent+2]='\0';
	}

	fprintf(f,"%sfirstclk       %u  #in milliseconds, delay to repeating events for mouse downs\n",spc,firstclk);
	fprintf(f,"%sdblclk         %u  #in ms, time afterwhich clicks are single clicks\n",spc,dblclk);
	fprintf(f,"%sidleclk        %u  #in ms, time to send fake clicks when holding down a mouse button\n",spc,idleclk);
	fprintf(f,"%stooltips       %d  #in ms, time to wait before bringing up tooltips. 0 means no tooltips\n",spc,tooltips);
	fprintf(f,"%sbevel          %d  #default pixel bevel width\n",spc,default_bevel);
	fprintf(f,"%sdefault_border %d  #default pixel border width\n",spc,default_border_width);
	fprintf(f,"%sdefault_padx   %d  #default pixel horizontal padding\n",spc,default_padx);
	fprintf(f,"%sdefault_pady   %d  #default pixel vertical padding\n",spc,default_pady);
	fprintf(f,"%stextfont       %s  #a fontconfig string\n",spc,textfontstr?textfontstr:"none");
	fprintf(f,"%scontrolfont    %s  #a fontconfig string\n",spc,controlfontstr?controlfontstr:"none");

	WindowColors *cols;
	fprintf(f,"%scolors  #Assume 8 bit, rgb. Also specify with \"rgbf 1. 1. .4\", \"rgb16 65535 65535 65535\", etc\n",spc);
	for (int c=0; c<4; c++) {
		if (c==0)      { cols=color_panel;   fprintf(f,"%s  panel\n",  spc); }
		else if (c==1) { cols=color_menu;    fprintf(f,"%s  menu\n",   spc); }
		else if (c==2) { cols=color_edits;   fprintf(f,"%s  edits\n",  spc); }
		else           { cols=color_buttons; fprintf(f,"%s  buttons\n",spc); }

		fprintf(f,"%s    fg       %lu %lu %lu\n",spc, cols->fg&0xff, (cols->fg&0xff00)>>8, (cols->fg&0xff0000)>>16);
		fprintf(f,"%s    bg       %lu %lu %lu\n",spc, cols->bg&0xff, (cols->bg&0xff00)>>8, (cols->bg&0xff0000)>>16);
		fprintf(f,"%s    hfg      %lu %lu %lu  #highlighted fg\n",spc, cols->hfg&0xff, (cols->hfg&0xff00)>>8, (cols->hfg&0xff0000)>>16);
		fprintf(f,"%s    hbg      %lu %lu %lu  #highlighted bg\n",spc, cols->hbg&0xff, (cols->hbg&0xff00)>>8, (cols->hbg&0xff0000)>>16);
		fprintf(f,"%s    moverfg  %lu %lu %lu  #mouse over fg\n",spc, cols->moverfg&0xff, (cols->moverfg&0xff00)>>8, (cols->moverfg&0xff0000)>>16);
		fprintf(f,"%s    moverbg  %lu %lu %lu  #mouse over bg\n",spc, cols->moverbg&0xff, (cols->moverbg&0xff00)>>8, (cols->moverbg&0xff0000)>>16);
		fprintf(f,"%s    grayedfg %lu %lu %lu\n",spc, cols->grayedfg&0xff, (cols->grayedfg&0xff00)>>8, (cols->grayedfg&0xff0000)>>16);
		fprintf(f,"%s    color1   %lu %lu %lu  #arrow heads and scroller bar, for instance\n",spc,
							cols->color1&0xff, (cols->color1&0xff00)>>8, (cols->color1&0xff0000)>>16);
		fprintf(f,"%s    color2   %lu %lu %lu\n",spc,
							cols->color2&0xff, (cols->color2&0xff00)>>8, (cols->color2&0xff0000)>>16);
	}

	if (what==-1) {
		fprintf(f,"%s#profile profilename #In an rc, you can define multiple profiles.\n",spc);
		fprintf(f,"%s#    ...              #When read in, you specify which profile to read in.\n",spc);
	}
}

/*! Used to scan in laxrc values.
 */
void anXApp::dump_in_rc(Attribute *att, const char *profile)
{
	char *name,*value;
	unsigned long ul;
	int c;
	
	for (c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"firstclk")) {
			if (ULongAttribute(value,&ul)) firstclk=ul;
			
		} else if (!strcmp(name,"dblclk")) {
			if (ULongAttribute(value,&ul)) dblclk=ul;
			
		} else if (!strcmp(name,"idleclk")) {
			if (ULongAttribute(value,&ul)) idleclk=ul;
			
		} else if (!strcmp(name,"tooltips")) {
			IntAttribute(value,&tooltips);
			
		} else if (!strcmp(name,"bevel")) {
			IntAttribute(value,&default_bevel);
			
		} else if (!strcmp(name,"default_border")) {
			IntAttribute(value,&default_border_width);
			
		} else if (!strcmp(name,"default_padx")) {
			IntAttribute(value,&default_padx);
			
		} else if (!strcmp(name,"default_pady")) {
			IntAttribute(value,&default_pady);
			
		//} else if (!strcmp(name,"bookmarks")) {
		//	***
		//
		//} else if (!strcmp(name,"directories")) {
		//	***

		} else if (!strcmp(name,"textfont")) {
			if (!isblank(value)) makestr(textfontstr,value);
			DBG cerr<<"textfont: "<<(textfontstr?textfontstr:"(none)")<<endl;

		} else if (!strcmp(name,"controlfont")) {
			if (!isblank(value)) makestr(controlfontstr,value);
			DBG cerr<<"controlfont: "<<(controlfontstr?controlfontstr:"(none)")<<endl;

		} else if (!strcmp(name,"colors")) {
			//colors->dump_in_atts(resources.attributes.e[c]);
			dump_in_colors(att->attributes.e[c]);

		} else if (!strcmp(name,"profile")) {
			if (!value || !profile || strcmp(value,profile)) continue;
			dump_in_rc(att->attributes.e[c],NULL);

		}
	}
}

/*! Used to scan in laxrc color values.
 */
void anXApp::dump_in_colors(Attribute *att)
{
	if (!att) return;
	char *name,*value;
	int c,c2;
	unsigned long color;
	
	WindowColors *col=NULL;
	for (c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		col=NULL;
		if (!strcmp(name,"panel"))        col=color_panel;
		else if (!strcmp(name,"menu"))    col=color_menu;
		else if (!strcmp(name,"edits"))   col=color_edits;
		else if (!strcmp(name,"buttons")) col=color_buttons;

		if (col) {
			for (c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name=att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (SimpleColorAttribute(value,&color)==0) {
					if (!strcmp(name,"fg")) {
						col->fg=color;      
					} else if (!strcmp(name,"bg")) {
						col->bg=color;      
					} else if (!strcmp(name,"hfg")) {
						col->hfg=color;     
					} else if (!strcmp(name,"hbg")) {
						col->hbg=color;     
					} else if (!strcmp(name,"moverfg")) {
						col->moverfg=color; 
					} else if (!strcmp(name,"moverbg")) {
						col->moverbg=color; 
					} else if (!strcmp(name,"grayedfg")) {
						col->grayedfg=color;
					} else if (!strcmp(name,"color1")) {
						col->color1=color;
					} else if (!strcmp(name,"color2")) {
						col->color2=color;
					}
				}
			}
		}
	}
}

} // namespace Laxkit

