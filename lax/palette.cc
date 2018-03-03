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

PaletteEntry::PaletteEntry(PaletteEntry *entry)
{
	maxcolor   =entry->maxcolor;
	color_space=entry->color_space;
	numcolors  =entry->numcolors;
	name       =newstr(entry->name);
	channels   =new int[numcolors];
	memcpy(channels,entry->channels,sizeof(int)*numcolors);
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
	readonly=false;
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

Palette *Palette::duplicate()
{
	Palette *pal=new Palette();
	for (int c=0; c<colors.n; c++) {
		pal->colors.push(new PaletteEntry(colors.e[c]));
	}

	pal->defaultmaxcolor=defaultmaxcolor;
	pal->default_colorspace=default_colorspace;
	makestr(pal->filename, filename);
	makestr(pal->name,name);
	pal->is_read_in=is_read_in;
	pal->readonly=readonly;
	pal->columns=columns;

	return pal;
}

//! Dump out the palette to an Attribute, in standard format (what is ignored).
Attribute *Palette::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
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
void Palette::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *loadcontext)
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
void Palette::dump_in(FILE *f,int indent,int what,LaxFiles::DumpContext *loadcontext,Attribute **Att)
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
 *
 * \todo translateable metadata fields?
 * \todo color management, specify color profile to go along with palette
 */
void Palette::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
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


	
} //namespace Laxkit;


