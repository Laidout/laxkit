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

#include <lax/palette.h>
#include <lax/laxutils.h>
#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/colors.h>
#include <lax/language.h>
#include <lax/lists.cc>

#include <sys/stat.h>
#include <cmath>

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxFiles;

namespace Laxkit {

//-------------------------------- PaletteEntry/Palette ---------------------------------


/*! \class PaletteEntry
 *  \brief Color instance type for Palette.
 *
 * color_space: LAX_COLOR_RGB=rgb, 1=rgba, LAX_COLOR_CMYK=cmyk, 3=cmyka, LAX_COLOR_GRAY=gray, 5=grayA
 */
	

//! Copies v, assumed to have n entries.
PaletteEntry::PaletteEntry(const char *nname,int n,int *v,int space,int max)//max=255
{
	maxcolor=max;
	color_space=space;
	numcolors=n;
	name=newstr(nname);
	channels=new int[n];
	memcpy(channels,v,sizeof(int)*n);
}

PaletteEntry::~PaletteEntry()
{
	if (name) delete[] name;
	if (channels) delete[] channels;
}

//--------------------------------- Palette ----------------------------------------
/*! \class Palette 
 * \brief A color palette. You can use a PaletteWindow to handle these.
 *
 * Can read and write Gimp palettes, as well as an Attribute ready format.
 *
 * \todo *** needs better color management, have color space field, etc.... right now assumes ONLY rgb.
 */

	
Palette::Palette()
{
	columns=0;
	is_read_in=0;
	name=filename=NULL;
	defaultmaxcolor=255;
}

Palette::~Palette()
{
	if (name) delete[] name;
	if (filename) delete[] filename;
}

//! Dump out the palette to an Attribute, in standard format (what is ignored).
Attribute *Palette::dump_out_atts(Attribute *att,int what,anObject *savecontext)
{
	if (!att) att=new Attribute;

	 //name
	att->push("name",name);
	
	 //columns
	char num[12],*blah=NULL;
	if (columns>0) {
		sprintf(num,"%d",columns);
		att->push("columns",num);
	}
	
	 //maxcolor
	sprintf(num,"%d",defaultmaxcolor);
	att->push("maxcolor",num);
	
	 //the colors
	if (colors.n) {
		char *color=NULL;
		for (int c=0; c<colors.n; c++) {
			for (int c2=0; c2<colors.e[c]->numcolors; c2++) {
				sprintf(num,"%-3d ",colors.e[c]->channels[c2]);
				appendstr(color,num);
			}
			if (colors.e[c]->name) appendstr(color,colors.e[c]->name);
			appendstr(blah,color);
			if (color) { delete[] color; color=NULL; }
		}
		att->push("colors",blah);
	}
	if (blah) delete[] blah;
	return att;
}

/*! Expects value for colors to be like "1 2 3 name\n1 2 3 name\n...".
 * The final value is always assumed to be the name. This will read in any
 * number of initial numbers, then the first non-number character signals the start
 * of the name of the color.
 */
void Palette::dump_in_atts(Attribute *att,int flag,anObject *loadcontext)
{
	int c;
	char *value,*name;
	for (c=0; c<att->attributes.n; c++) { 
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"name")) {
			makestr(name,value);
		} else if (!strcmp(name,"columns")) {
			IntAttribute(value,&columns);
		} else if (!strcmp(name,"maxcolor")) {
			IntAttribute(value,&defaultmaxcolor);
		} else if (!strcmp(name,"colors")) {
			char *e,*v=value, *nl;
			int n=0;
			int *col=NULL;
			while (1) {
				 // each loop iteration reads one color composed of the channels, then
				 // an optional name until '\n'
				nl=strchr(v,'\n');
				if (nl) *nl='\0';
				IntListAttribute(v,&col,&n,&e);
				if (e!=v) {
					while (isspace(*e)) e++;
					 //*** the 0 in next line is for color space...
					colors.push(new PaletteEntry(e,n,col,0,defaultmaxcolor),1);
					delete[] col; col=NULL; //the new copies col
				}
				if (!nl) break;
				*nl='\n';
				v=nl+1;
			}
			if (col) delete[] col;
		}
	}
}

/*! what==0 normal Attribute format..
 *
 * or LAX_GIMP_PALETTE
 */
void Palette::dump_in(FILE *f,int indent,int what,anObject *loadcontext,Attribute **Att)
{
	if (what==LAX_GIMP_PALETTE) {
		 // Read in
		if (feof(f)) return;
		
		defaultmaxcolor=255;
		
		char *line=NULL;
		int c;
		size_t n=0;

		c=getline(&line,&n,f);
		if (c>0 && strncmp(line,"GIMP Palette",12)) c=0;
		
		if (c>0) c=getline(&line,&n,f);
		if (c>0 && !strncmp(line,"Name: ",6)) {
			makestr(name,line+6);
			name[strlen(name)-1]='\0';
		} else c=0;

		if (c>0) c=getline(&line,&n,f);
		if (c>0) if (!strncmp(line,"Columns: ",9)) IntAttribute(line+9,&columns);

		int rgb[3],n2;
		char *e;
		while (c>0 && !feof(f)) {
			c=getline(&line,&n,f);
			if (c<=0) break;
			e=NULL;
			n2=IntListAttribute(line,rgb,3,&e);
			if (n2!=3) continue;
			while (e && isspace(*e)) e++;
			if (e[strlen(e)-1]=='\n') e[strlen(e)-1]='\0';
			colors.push(new PaletteEntry(e,3,rgb,0),1,255);
		} 
		if (line) free(line);
		if (Att) {
			if (!Att) *Att=new Attribute;
			dump_out_atts(*Att,0,NULL);//***should the context be NULL here?
		}
	} else {
		DumpUtility::dump_in(f,indent,0,loadcontext,Att); //ultimately calls dump_in_atts()
	}
}

/*! Assumes the palette is already read in. filename is ignored.
 *
 * If what==LAX_GIMP_PALETTE and the number of channels for the color
 * does not equal 3, then the palette will not necessarily be readable
 * by the Gimp, for instance, which assumes "r g b Name".
 *
 * Out:
 * <pre>
 *  name Blah Palette
 *  columns 10
 *  maxcolor 255 #if you say 1.0 for maxcolor, then channels are floating point in range [0..1]
 *  colors \
 *    0   0   0   Black
 *    255 255 255 White
 *    255 0   0   Red
 *    #r  g   b   Name
 * </pre>
 *
 * If what==-1, the dump out a pseudocode mockup of the default file format.
 */
void Palette::dump_out(FILE *f,int indent,int what,anObject *context)
{
	if (what==LAX_GIMP_PALETTE) {
		fprintf(f,"GIMP Palette\n");
		fprintf(f,"Name: %s\n",(name?name:"Untitled"));
		if (columns>0) fprintf(f,"Columns: %d\n",columns);
		fprintf(f,"#\n");
		int c,c2;
		for (c=0; c<colors.n; c++) {
			for (c2=0; c<colors.e[c]->numcolors; c++) {
				fprintf (f,"%d ",colors.e[c]->channels[c2]);
			}
			if (colors.e[c]->name) fprintf (f,"%s\n",colors.e[c]->name);
			else fprintf (f,"%x%x%x\n",colors.e[c]->channels[0],colors.e[c]->channels[1],colors.e[c]->channels[2]);	
		}
	} else if (what==-1) {
		char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
		
		fprintf(f,"%sname Name of the palette\n",spc);
		fprintf(f,"%scolumns  5    #how many columns to display the palette in. 0 means it doesn't matter\n",spc);
		fprintf(f,"%smaxcolor 255  #the maximum value of a color component, range is [0..maxcolor]\n",spc);
		fprintf(f,"%scolors \\     #the backslash is important!\n",spc);
		fprintf(f,"%s  #what follows is a list of the colors, one per line,\n",spc);
		fprintf(f,"%s  #color component list, then the color name if any\n",spc);
		fprintf(f,"%s  0 255 0 255     Green\n",spc);
		fprintf(f,"%s  255 255 255 255 White\n",spc);
		fprintf(f,"%s  255 255 255 128 Half-transparent White\n",spc);
		return;
	} else if (what==0) {
		char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
		
		if (name) fprintf(f,"%sname %s\n",spc,name);
		if (columns>0) fprintf(f,"%scolumns %d\n",spc,columns);
		
		fprintf(f,"%smaxcolor %d\n",spc,defaultmaxcolor);
		
		if (colors.n) {
			fprintf(f,"%scolors \\\n",spc);
			int c,c2;
			for (c=0; c<colors.n; c++) {
				fprintf(f,"%s  ",spc);
				for (c2=0; c2<colors.e[c]->numcolors; c2++) {
					fprintf (f,"%-3d ",colors.e[c]->channels[c2]);
				}
				fprintf (f,"%s\n",colors.e[c]->name);
			}
		}
	}
}

int Palette::AddRGB(const char *name, int r, int g, int b, int max)
{
	int v[3];
	v[0]=r;
	v[1]=g;
	v[2]=b;
	return colors.push(new PaletteEntry(name, 3,v, LAX_COLOR_RGB, max));
}

int Palette::AddRGBA(const char *name, int r, int g, int b, int a, int max)
{
	int v[4];
	v[0]=r;
	v[1]=g;
	v[2]=b;
	v[3]=a;
	return colors.push(new PaletteEntry(name, 4,v, LAX_COLOR_RGB, max));
}

int Palette::AddCMYK(const char *name, int c, int m, int y, int k, int max)
{
	int v[4];
	v[0]=c;
	v[1]=m;
	v[2]=y;
	v[3]=k;
	return colors.push(new PaletteEntry(name, 4,v, LAX_COLOR_RGB, max));
}

int Palette::AddCMYKA(const char *name, int c, int m, int y, int k, int a, int max)
{
	int v[5];
	v[0]=c;
	v[1]=m;
	v[2]=y;
	v[3]=k;
	v[4]=a;
	return colors.push(new PaletteEntry(name, 5,v, LAX_COLOR_RGB, max));
}

int Palette::AddGray(const char *name, int g,int max)
{
	int v[1];
	v[0]=g;
	return colors.push(new PaletteEntry(name, 1,v, LAX_COLOR_RGB, max));
}

int Palette::AddGrayA(const char *name, int g,int a,int max)
{
	int v[2];
	v[0]=g;
	v[1]=a;
	return colors.push(new PaletteEntry(name, 2,v, LAX_COLOR_RGB, max));
}



//--------------------------------------- rainbowPalette -----------------------------------

/* \ingroup misc
 *
 * Make a palette with RYGCBM horizontally and white to black vertically.
 *
 * If include_gray_strip, then the final row is a transition from full white to full black.
 */
Palette *rainbowPalette(int w,int h,int max,int include_gray_strip)
{
	Palette *p=new Palette;
	p->columns=w;
	int x,y,rgb[3];
	float fmax=max;
	if (include_gray_strip) h--;
	for (y=0; y<h; y++) {
		for (x=0; x<w; x++) {
			if (x<w/6) {
				rgb[0]=max;
				rgb[1]=int(fmax*x*6/w);
				rgb[2]=0;
			} else if (x<w*2/6) {
				rgb[0]=max-int(fmax*(6*x-w)/w);
				rgb[1]=max;
				rgb[2]=0;
			} else if (x<w*3/6) {
				rgb[0]=0;
				rgb[1]=max;
				rgb[2]=int(fmax*(6*x-2*w)/w);
			} else if (x<w*4/6) {
				rgb[0]=0;
				rgb[1]=max-int(fmax*(6*x-3*w)/w);
				rgb[2]=max;
			} else if (x<w*5/6) {
				rgb[0]=int(fmax*(6*x-4*w)/w);
				rgb[1]=0;
				rgb[2]=max;
			} else {
				rgb[0]=max;
				rgb[1]=0;
				rgb[2]=max-int(fmax*(6*x-5*w)/w);
			}
			
			if (y<h/2) {
				rgb[0]=rgb[0]*y/(h/2);
				rgb[1]=rgb[1]*y/(h/2);
				rgb[2]=rgb[2]*y/(h/2);
			} else {
				rgb[0]=max*(y-(h-1)/2)/(h/2)+(rgb[0])*(h-1-y)/(h/2);
				rgb[1]=max*(y-(h-1)/2)/(h/2)+(rgb[1])*(h-1-y)/(h/2);
				rgb[2]=max*(y-(h-1)/2)/(h/2)+(rgb[2])*(h-1-y)/(h/2);
			}
			if (rgb[0]<0) rgb[0]=0;
			else if (rgb[0]>max) rgb[0]=max;
			if (rgb[1]<0) rgb[1]=0;
			else if (rgb[1]>max) rgb[1]=max;
			if (rgb[2]<0) rgb[2]=0;
			else if (rgb[2]>max) rgb[2]=max;
			p->colors.push(new PaletteEntry(NULL,3,rgb,LAX_COLOR_RGB,max),1);
		}
	}
	if (include_gray_strip) {
		for (x=0; x<w; x++) {
			rgb[0]=rgb[1]=rgb[2]=int(fmax*x/(w-1) + .5);
			p->colors.push(new PaletteEntry(NULL,3,rgb,0),1);
		}

	}
	return p;
}

//-------------------------------- PaletteWindow -----------------------------


/*! \class PaletteWindow 
 *  \brief A window to handle Palette instances.
 *
 *  \code
 *    //if left-double clicking calls up a FileDialog to load another Palette.
 *   #define PALW_DBCLK_TO_LOAD   (1<<16)
 *  \endcode
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
	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	palette=NULL;
	
	//***
	//LoadPalette("/usr/share/gimp/2.0/palettes/Plasma.gpl");
	palette=rainbowPalette(27,18,255, 1);

	curcolor=ccolor=-1;
	
	installColors(app->color_panel);

	findInrect();
}

PaletteWindow::~PaletteWindow()
{
	if (palette) palette->dec_count();
}
 
//! Send a SimpleColorEventData message to win_owner. 
int PaletteWindow::send()
{
	if (!win_owner || !win_sendthis || !palette || curcolor<0) return 0;

	SimpleColorEventData *e=new SimpleColorEventData;

	e->max=palette->defaultmaxcolor;
	e->numchannels=palette->colors.e[curcolor]->numcolors;
	e->channels=new int[e->numchannels];
	e->colortype=palette->colors.e[curcolor]->color_space;

	int c;
	for (c=0; c<palette->colors.e[curcolor]->numcolors; c++) 
		e->channels[c]=palette->colors.e[curcolor]->channels[c];
	
	app->SendMessage(e,win_owner,win_sendthis,object_id);
	return 1;
}

/*! 
 * Currently, this dec_counts the old, and installs a new one.
 * 
 * Return 0 for success, or nonzero error.
 * 
 * \todo *** check readability of the file...
 * \todo *** make a default palette for file==NULL 32x8 rainbow
 */
int PaletteWindow::LoadPalette(const char *file)
{
	FILE *f=fopen(file,"r");
	if (!f) return 1;
	Palette *p=new Palette;
	p->dump_in(f,0,LAX_GIMP_PALETTE,NULL,NULL);
	fclose(f);
	if (p->colors.n) {
		if (palette) palette->dec_count();
		palette=p;
		findInrect();
		curcolor=ccolor=-1;
		needtodraw=1;
		return 0;
	} 
	delete p;
	return 1;
}
	
void PaletteWindow::Refresh()
{
	if (!win_on || needtodraw==0) return;
	needtodraw=0;


	 // draw head stuff
	const char *blah;
	foreground_color(win_colors->bg);
	drawing_function(LAXOP_Source);
	fill_rectangle(this, 0,0,win_w,app->defaultlaxfont->textheight());
			
	if (palette->name) blah=palette->name; else blah="(untitled)";
	foreground_color(win_colors->fg);
	int cc=textout(this, blah,strlen(blah),0,0,LAX_LEFT|LAX_TOP);

	int r,g,b;
	if (curcolor>=0 || ccolor>=0) {
		int color;
		if (ccolor>=0) color=ccolor; else color=curcolor;
		char *blah2=NULL;
		r=palette->colors.e[color]->channels[0]*255/palette->defaultmaxcolor;
		g=palette->colors.e[color]->channels[1]*255/palette->defaultmaxcolor;
		b=palette->colors.e[color]->channels[2]*255/palette->defaultmaxcolor;
		if (!palette->colors.e[color]->name || !strcmp(palette->colors.e[color]->name,"Untitled")) {
			blah2=new char[30];
			sprintf(blah2,"%02X%02X%02X",r,g,b);
		}
		int ccc=textout(this, (blah2?blah2:palette->colors.e[color]->name),-1,win_w,0,LAX_TOP|LAX_RIGHT);
		if (blah2) delete[] blah2;
		ccc=win_w-cc-ccc-10;
		if (ccc>0) {
			foreground_color(rgbcolor(r,g,b));
			fill_rectangle(this, cc+5,0, ccc,app->defaultlaxfont->textheight());
		}
	}

	 // draw all the palette colors
	int i;
	double x=0,y;
	
	y=inrect.y-dy;
	for (i=0; i<palette->colors.n; i++) {
		if (i%xn==0) {
			x=inrect.x;
			y+=dy;
		}
		r=palette->colors.e[i]->channels[0]*255/palette->defaultmaxcolor;
		g=palette->colors.e[i]->channels[1]*255/palette->defaultmaxcolor;
		b=palette->colors.e[i]->channels[2]*255/palette->defaultmaxcolor;
		foreground_color(rgbcolor(r,g,b));
		fill_rectangle(this, x,y,dx+1,dy+1);
		x+=dx;
//		if ((i+1)%xn==0) {
//			 //blank out to the right
//			foreground_color(win_colors->bg);
//			fill_rectangle(this, x,y,win_w-x,dy);
//		}
	}
//	 //blank out unused space
//	foreground_color(win_colors->bg);
//	fill_rectangle(this, x,y,win_w-x,dy);
//	fill_rectangle(this, 0,y+dy,win_w,win_h-y);
	
	 // draw box around curcolor
	if (curcolor>=0) {
		x=inrect.x + (curcolor%xn)*dx;
		y=inrect.y + (curcolor/xn)*dy;
		foreground_color(0);
		draw_rectangle(this, x,y,dx,dy);
		foreground_color(~0);
		draw_rectangle(this, x-1,y-1,dx+2,dy+2);
	}
	
}

int PaletteWindow::findColorIndex(int x,int y)
{
	if (x<inrect.x || x>=inrect.x+inrect.width || y<inrect.y || y>=inrect.y+inrect.height) return -1;
	
	if (inrect.height<1) return -1; 

	int r,c;
	c=(x-inrect.x)/dx;
	r=(y-inrect.y)/dy;
	if (c>=xn) return -1;
	//DBG cerr<<"c,r: "<<c<<','<<r<<endl;
	c=c+r*xn;
	if (c<0 || c>=palette->colors.n) return -1;
	return c;
}

//! Make needtodraw=1 if LeaveNotify.
int PaletteWindow::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onMouseOut) {
		//DBG cerr <<" in PaletteWindow::event()..."<<endl;
		ccolor=-1;
		needtodraw=1;
		return 0;
		//DBG cerr <<"  Button::event:Leave:"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;

	} else if (!strcmp(mes,"loadpalette")) {
		 //sent from a FileDialog, selecting a new palette to load in.
		const StrEventData *s=dynamic_cast<const StrEventData *>(e);
		LoadPalette(s->str);
		return 0;
	}

	return anXWindow::Event(e,mes);
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

/*! If count==2, then bring up a FileDialog.
 */
int PaletteWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (count==2 && (win_style&PALW_DBCLK_TO_LOAD)) {
		app->rundialog(new FileDialog(NULL,"Load Palette",_("Load Palette"),0,
									  0,0,400,500,0,
									  object_id,"loadpalette",
									  FILES_OPEN_ONE,
									  NULL,PaletteDir()));
	}
	buttondown.down(d->id,LEFTBUTTON, x,y);
	return 0;
}

int PaletteWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 0;
	buttondown.up(d->id,LEFTBUTTON);
	int cc=findColorIndex(x,y);
	if (curcolor!=cc) {
		needtodraw|=2;
		curcolor=cc;
		send();
	}
	return 0;
}

//! Pop up a context menu to select a recent menu or other things.
int PaletteWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	//if (recentpalettes) ***;
	//if (palette_is_editable) { 
	//	Delete color
	//	New color
	//	Edit color
	//}
	//LoadPalette
	//SavePalette
	return 0;
}

//! Update showing name of curcolor.
/*! \todo this could be used to show a temp color? buttondown-nodrag-up actually changes curcolor?
 */
int PaletteWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	int cc=findColorIndex(x,y);

	if (buttondown.isdown(0,LEFTBUTTON) && curcolor!=cc) {
		needtodraw|=2;
		curcolor=ccolor=cc;
		send();

	} else if (!buttondown.isdown(0,LEFTBUTTON) && ccolor!=cc) {
		needtodraw|=2;
		ccolor=cc;
	}
	//DBG cerr <<"palette curcolor: "<<curcolor<<"  ccolor: "<<ccolor<<endl;
	
	return 0;
}

//! Set up inrect to correspond the region the colors should be drawn in.
/*! Also finds xn, yn, dx, and dy.
 */
void PaletteWindow::findInrect()
{
	inrect.x=0;
	inrect.y=app->defaultlaxfont->textheight();
	inrect.width= win_w;
	inrect.height=win_h-app->defaultlaxfont->textheight();
	if (inrect.width<1) inrect.width=1;
	if (inrect.height<1) inrect.height=1;
	
	double aspect=double(inrect.height)/inrect.width;

	if (palette->columns>0) {
		xn=palette->columns;
		yn=palette->colors.n/xn;
		if (palette->colors.n%xn!=0) yn++;
	} else {
		xn=int(ceil(sqrt(palette->colors.n/aspect)));
		yn=int(xn*aspect);
		while (xn*yn<palette->colors.n) yn++;
	}
	
	dx=double(inrect.width)/xn;
	dy=double(inrect.height)/yn;
	if (dx<=0) dx=1;
	if (dy<=0) dy=1;
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

	
} //namespace Laxkit;


