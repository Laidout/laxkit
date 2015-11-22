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
//    Copyright (C) 2015 by Tom Lechner
//


#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/characterinterface.h>
#include <lax/interfaces/viewportwindow.h>
#include <lax/interfaces/linestyle.h>
#include <lax/colors.h>
#include <lax/laxutils.h>
#include <lax/fontdialog.h>
#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/utf8utils.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

#define sgn(a) ((a)<0?-1:((a)>0?1:0))


namespace LaxInterfaces {


//--------------------------------- CaptionData -------------------------------
/*! \class CaptionData
 * \brief Holds a little bit of text.
 * 
 * This is for very simple text labels and such. There can be multiple lines
 * of text with variable centering. One CaptionData unit should be considered one "point"
 * when the object is scaled same as paper.
 */
/*! \var double CaptionData::xcentering
 * \brief 0 is left, 100 is right, 50 is center, and other number is partial centering.
 *
 * All centering is around the origin.
 */
/*! \var double CaptionData::ycentering
 * \brief 0 is top, 100 is bottom, 50 is center, and other number is partial centering.
 *
 * All centering is around the origin.
 */
/*! \var char *CaptionData::font
 * \brief The filename of the font to use.
 */
/*! \var double CaptionData::fontsize
 * \brief The point size of the font.
 *
 * This size is relative to the object's coordinate space which are assumed to be inches.
 * Thus a fontsize of 1pt will be 1/72nd of 1 unit in the object's space.
 */

CaptionData::CaptionData()
  : lines(2)
{
	fontfamily=newstr("sans");
	fontstyle=newstr("normal");
	fontfile=NULL;
	fontsize=12;
	linespacing=1;
	font=NULL;
	righttoleft=false;

	state=0;

	xcentering=0;
	ycentering=0;

	lines.push(newstr(""));
	linelengths.push(0);

	red=green=blue=.5;
	alpha=1.;

	Font(fontfile, fontfamily, fontstyle, fontsize);
}

	
CaptionData::CaptionData(const char *ntext, const char *nfontfamily, const char *nfontstyle,  int fsize, double xcenter, double ycenter)
  : lines(2)
{
	DBG cerr <<"in CaptionData constructor"<<endl;
	
	if (isblank(nfontfamily)) nfontfamily="sans";
	if (isblank(nfontstyle)) nfontstyle="normal";

	fontfamily=newstr(nfontfamily);
	fontstyle=newstr(nfontstyle);
	fontfile=NULL;
	//font=NULL;
	fontsize=fsize;
	linespacing=1;
	if (fontsize<=0) fontsize=10;
	font=NULL;
	righttoleft=false;

	state=0;  //0 means someone needs to remap extents

	

	int numlines=0;
	char **text=split(ntext,'\n',&numlines);
	for (int c=0; c<numlines; c++) {
		lines.push(text[c]);
		linelengths.push(0);
	}
	xcentering=xcenter;
	ycentering=ycenter;
	//centerline=NULL;

	if (numlines==0) {
		lines.push(newstr(""));
		linelengths.push(0);
		numlines=1;
	}

	red=green=blue=.5;
	alpha=1.;

	Font(fontfile, fontfamily, fontstyle, fontsize);

	DBG if (ntext) cerr <<"CaptionData new text:"<<endl<<ntext<<endl;
	DBG cerr <<"..CaptionData end"<<endl;
}

CaptionData::~CaptionData()
{
	DBG cerr <<"in CaptionData destructor"<<endl;

	if (fontfamily) delete[] fontfamily;
	if (fontstyle) delete[] fontstyle;
	if (fontfile) delete[] fontfile;
	if (font) font->dec_count();

	DBG cerr <<"-- CaptionData dest. end"<<endl;
}

SomeData *CaptionData::duplicate(SomeData *dup)
{
	CaptionData *i=dynamic_cast<CaptionData*>(dup);
    if (!i && dup) return NULL; //was not an ImageData!

    if (!dup) {
        dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_CAPTIONDATA));
        if (dup) {
            dup->setbounds(minx,maxx,miny,maxy);
        }
        i=dynamic_cast<CaptionData*>(dup);
    }
    if (!i) {
        i=new CaptionData();
        dup=i;
    }

     //somedata elements:
    dup->bboxstyle=bboxstyle;
    dup->m(m());

	i->linespacing =linespacing;
	i->xcentering  =xcentering;
	i->ycentering  =ycentering;
	i->righttoleft =righttoleft;

	i->red   =red;
	i->green =green;
	i->blue  =blue;
	i->alpha=alpha;

	i->Font(fontfile, fontfamily, fontstyle, fontsize);

	char *txt=GetText();
	i->SetText(txt);
	delete[] txt;
	

    return dup;

}


/*! Return the number of characters in the given line number.
 */
int CaptionData::CharLen(int line)
{
	if (line<0 || line>=lines.n) return 0;
	return strlen(lines.e[line]);
}

//! Update cached pixel length of line. Also return that value.
/*! If line<0 then compute for all lines, and returns maximum length.
 */
int CaptionData::ComputeLineLen(int line)
{
	if (line<0) {
		int max=0,w;
		for (int c=0; c<lines.n; c++) {
			w=ComputeLineLen(c);
			if (w>max) max=w;
		}
		return w;
	}

	if (!font || line>=lines.n) return 0;
	return font->extent(lines.e[line],strlen(lines.e[line]));
}

/*! Returns 1 if found, else 0 for out of bounds.
 */
int CaptionData::FindPos(double y, double x, int *line, int *pos)
{
	int l = (y-miny)/(fontsize*linespacing);
	if (l<0 || l>=lines.n) return 0;
	
	unsigned int p=0;
	double lastw=0;
	double w;
	const char *pp;
	while (p<strlen(lines.e[l])) {
		p++;
		pp=utf8fwd(lines.e[l]+p, lines.e[l], lines.e[l]+strlen(lines.e[l]));
		p=pp-lines.e[l];

		w=font->extent(lines.e[l], p);
		if (x<(lastw+w)/2) { p=utf8back_index(lines.e[l],p-1,strlen(lines.e[l])); break; }
		lastw=w;
	}

	*line=l;
	*pos=p;
	return 0;
}

/*! 
 * Default dump for a CaptionData. 
 *
 * Dumps:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  fontfamily sans
 *  fontstyle normal
 *  fontsize 10
 *  xcentering  50
 *  ycentering  50
 *  righttoleft no
 *  text \
 *    Blah Blah.
 *    Blah, "blah blah"
 * </pre>
 *
 */
void CaptionData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%smatrix 1 0 0 1 0 0   #transform of the whole object\n",spc);
		fprintf(f,"%sfontfamily sans      #Family name of font\n",spc);
		fprintf(f,"%sfontstyle normal     #Style name of font\n",spc);
		fprintf(f,"%sfontfile /path/to/it #File on disk to use. Overrides whatever is in family and style.\n",spc);
		fprintf(f,"%sfontsize 12          #hopefully this is point size\n",spc);
		fprintf(f,"%slinespacing 1        #percentage different from font's default spacing\n",spc);
		fprintf(f,"%srighttoleft no       #yes or no. ***todo: vertical options\n",spc);
		fprintf(f,"%sxcentering 50        #0 is left, 50 is center, 100 is right, or any other number\n",spc);
		fprintf(f,"%sycentering 50        #0 is top, 50 is center, 100 is bottom, or any other number\n",spc);
		fprintf(f,"%scolor rgbaf(1,0,0,1)\n", spc);
		fprintf(f,"%stext \\  #The actual text\n%s  blah",spc,spc);
		return;
	}


	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				m(0),m(1),m(2),m(3),m(4),m(5));

	if (font->Layers()>1) {
		fprintf(f,"%sfont\n",spc);
		Palette *palette=dynamic_cast<Palette*>(font->GetColor());

		int i=0;
		for (LaxFont *ff=font; ff; ff=ff->nextlayer, i++) {
			fprintf(f,"%s  layer\n",spc);
			fprintf(f,"%s    fontfamily \"%s\"\n", spc,ff->Family());
			fprintf(f,"%s    fontstyle  \"%s\"\n", spc,ff->Style());
			fprintf(f,"%s    fontfile   \"%s\"\n", spc,ff->FontFile());
			if (palette && i<palette->colors.n) {
				fprintf(f,"%s    color rgbaf(%.10g, %.10g, %.10g, %.10g)\n", spc,
						palette->colors.e[i]->channels[0]/(double)palette->colors.e[i]->maxcolor,
						palette->colors.e[i]->channels[1]/(double)palette->colors.e[i]->maxcolor,
						palette->colors.e[i]->channels[2]/(double)palette->colors.e[i]->maxcolor,
						palette->colors.e[i]->channels[3]/(double)palette->colors.e[i]->maxcolor
					   ); 
			}
		}

	} else {
		if (font->FontFile()) fprintf(f,"%sfontfile   \"%s\"\n", spc,font->FontFile());
		if (fontfamily)       fprintf(f,"%sfontfamily \"%s\"\n",spc,fontfamily);
		if (fontstyle)        fprintf(f,"%sfontstyle  \"%s\"\n",spc,fontstyle);
	}
	fprintf(f,"%sfontsize %.10g\n",spc,fontsize);
	fprintf(f,"%slinespacing %.10g\n",spc,linespacing);
	fprintf(f,"%srighttoleft %s\n",spc, righttoleft ? "yes" : "no");
	fprintf(f,"%sxcentering %.10g\n",spc,xcentering);
	fprintf(f,"%sycentering %.10g\n",spc,ycentering);
	fprintf(f,"%scolor rgbaf(%.10g, %.10g, %.10g, %.10g)\n",
				spc, red,green,blue,alpha);

	if (lines.n) {
		fprintf(f,"%stext \\\n",spc);
		for (int c=0; c<lines.n; c++) {
			fprintf(f,"%s  %s\n",spc,lines.e[c]); // *** this destroys spaces!!
		}
	}
}

//! See dump_out().
void CaptionData::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;

	char *name,*value;
	minx=miny=0;
	maxx=maxy=-1;

	const char *family=NULL, *style=NULL, *file=NULL;
	LaxFont *newfont=NULL;
	Palette *palette=NULL;

	const char *sz=att->findValue("fontsize");
	if (sz) DoubleAttribute(sz,&fontsize);

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"matrix")) {
			double mm[6];
			DoubleListAttribute(value,mm,6);
			m(mm);

		} else if (!strcmp(name,"righttoleft")) {
			righttoleft=BooleanAttribute(value);

		} else if (!strcmp(name,"xcentering")) {
			DoubleAttribute(value,&xcentering);

		} else if (!strcmp(name,"ycentering")) {
			DoubleAttribute(value,&ycentering);

		} else if (!strcmp(name,"text")) {
			SetText(value);

		} else if (!strcmp(name,"font")) {
			int layer=0;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"layer")) {
					family=style=file=NULL;
					layer++;
					char cname[10];
					sprintf(cname,"fg%d",layer);

					for (int c3=0; c3<att->attributes.e[c]->attributes.e[c2]->attributes.n; c3++) {
						name= att->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->name;
						value=att->attributes.e[c]->attributes.e[c2]->attributes.e[c3]->value;

						if (!strcmp(name,"fontfile")) {
							file=value;
						} else if (!strcmp(name,"fontfamily")) {
							family=value;
						} else if (!strcmp(name,"fontstyle")) {
							style=value;
						} else if (!strcmp(name,"color")) {
							double co[5];
							if (SimpleColorAttribute(value, co, NULL)==0) {
								if (!palette) palette=new Palette;
								palette->AddRGBA(cname, co[0]*255, co[1]*255, co[2]*255, co[3]*255, 255);
							}
						}
					}

					LaxFont *newlayer = InterfaceManager::GetDefault()->GetFontManager()->MakeFontFromFile(file, family,style,fontsize,-1);
					if (!newfont) newfont=newlayer;
					else newfont->AddLayer(newfont->Layers(), newlayer);
				}
			}

		} else if (!strcmp(name,"fontfile")) {
			file=value;
			//makestr(fontfile, value);

		} else if (!strcmp(name,"fontfamily")) {
			family=value;
			//makestr(fontfamily, value);

		} else if (!strcmp(name,"fontstyle")) {
			style=value;
			//makestr(fontstyle, value);

		} else if (!strcmp(name,"fontsize")) {
			DoubleAttribute(value,&fontsize);

		} else if (!strcmp(name,"linespacing")) {
			DoubleAttribute(value,&linespacing);

		} else if (!strcmp(name,"color")) {
			double co[5];
			if (SimpleColorAttribute(value, co, NULL)==0) {
				red  =co[0];
				green=co[1];
				blue =co[2];
				alpha=co[3]; 
			}
		}
	}

	if (newfont) {
		if (palette) {
			for (LaxFont *f=newfont; f; f=f->nextlayer) {
				f->SetColor(palette);
			}
			palette->dec_count();
		}
		Font(newfont);
		newfont->dec_count();
	} else Font(file, family, style, fontsize);
}

/*! Value is percentage (1 == 100%) off from font's line spacing.
 */
double CaptionData::LineSpacing(double newspacing)
{
	if (newspacing!=0) linespacing=newspacing;
	return linespacing;
}

double CaptionData::Size(double newsize)
{ 
	if (newsize<=0) newsize=1e-4;
	font->Resize(newsize);
	fontsize=newsize;
	state=0;
	FindBBox();
	return fontsize;
}

/*! Return a new char[] of the (utf8) text.
 */
char *CaptionData::GetText()
{
	char *text=NULL;
	for (int c=0; c<lines.n; c++) {
		appendstr(text, lines.e[c]);
		if (c<lines.n-1) appendstr(text,"\n");
	}
	return text;
}

//! Set new text.
/*! Accepts multi line text, where lines are delineated with '\n'.
 * 
 * Returns 0 for success, 1 for error.
 */
int CaptionData::SetText(const char *newtext)
{
	lines.flush();
	linelengths.flush();

	int numlines=0;
	char **text=split(newtext,'\n',&numlines);
	for (int c=0; c<numlines; c++) {
		lines.push(text[c]);
		linelengths.push(0);
	}
	state=0;
	FindBBox();
	return 0;
}

int CaptionData::DeleteChar(int line,int pos,int after, int *newline,int *newpos)
{
	if (after) { //del
		if (pos==(int)strlen(lines.e[line]) && line<lines.n-1) {
			 //combine lines
			appendstr(lines.e[line],lines.e[line+1]);
			lines.remove(line+1);
			linelengths.remove(line+1);
			ComputeLineLen(line);

		} else if (pos < (int)strlen(lines.e[line])) {
			 //delete char within a line
			const char *p=utf8fwd(lines.e[line]+pos+1, lines.e[line], lines.e[line]+strlen(lines.e[line])); 
			int cl=p-(lines.e[line]+pos);
			memmove(lines.e[line]+pos, lines.e[line]+pos+cl, strlen(lines.e[line])-(cl+pos)+1);
			ComputeLineLen(line);
		}

	} else { //bksp
		if (pos==0 && line>0) {
			 //combine lines
			appendstr(lines.e[line-1],lines.e[line]);
			lines.remove(line);
			linelengths.remove(line);
			line--;
			pos=strlen(lines.e[line]);
			ComputeLineLen(line);

		} else if (pos>0) {
			const char *p=utf8back(lines.e[line]+pos-1, lines.e[line], lines.e[line]+strlen(lines.e[line]));
			int cl=(lines.e[line]+pos)-p;
			memmove(lines.e[line]+pos-cl, lines.e[line]+pos, strlen(lines.e[line])-pos+1);
			pos-=cl;
			ComputeLineLen(line);
		}
	}

	state=0;
	if (newline) *newline=line;
	if (newpos) *newpos=pos;
	return 0;
}

int CaptionData::InsertChar(unsigned int ch, int line,int pos, int *newline,int *newpos)
{
	if (pos<0 || pos>=(int)strlen(lines.e[line])) pos=strlen(lines.e[line]);

	if (ch=='\n') {
		 //add new line
		char *str=newstr(lines.e[line]+pos);
		lines.e[line][pos]='\0';

		lines.push(str, 2, line+1);
		linelengths.push(0, line+1);

		ComputeLineLen(line);
		line++;
		ComputeLineLen(line);
		pos=0;
		*newpos=pos;
		*newline=line;

	} else {
		char utf8[10];
		int cl=utf8encode(ch,utf8);
		utf8[cl]='\0';

		char *nline=new char[strlen(lines.e[line])+cl+1];
		*nline='\0';
		if (pos) {
			strncpy(nline,lines.e[line],pos);
			nline[pos]='\0';
		}
		strcat(nline,utf8);
		strcat(nline,lines.e[line]+pos);

		delete[] lines.e[line];
		lines.e[line]=nline;
		ComputeLineLen(line);
		pos+=cl;
		*newpos=pos;
	}

	state=0;
	FindBBox();

	return 0;
}

/*! Assumes font is accurate????
 *
 * \todo *** must be able to find the actual extent of the text!
 *   right now, the interface must handle bbox finding.
 */
void CaptionData::FindBBox()
{//***
	double height=lines.n*fontsize*linespacing,
	       width=height;

	if (state==0) {
		Displayer *dp=GetDefaultDisplayer();
		for (int c=0; c<lines.n; c++) {
			linelengths.e[c]=dp->textextent(font, lines.e[c],-1, NULL,NULL,NULL,NULL,0);
		}
		state=1;
	}

	if (lines.n) {
		width=0;
		for (int c=0; c<lines.n; c++) {
			if (linelengths.e[c]>width) width=linelengths.e[c];
		}
	}

	minx=-xcentering/100*width;
	maxx=minx+width;
	if (maxx==minx) maxx=minx+fontsize/3;

	miny=-ycentering/100*height;
	maxy=miny+height;
	if (maxy==miny) maxy=miny+fontsize;
}

//! Set horizontal centering, and adjust bbox.
double CaptionData::XCenter(double xcenter)
{
	xcentering=xcenter;
	double w(maxx-minx);
	minx=-xcentering/100*w;
	maxx=minx+w;

	return xcentering;
}

//! Set vertical centering, and adjust bbox.
double CaptionData::YCenter(double ycenter)
{
	ycentering=ycenter;
	double h(maxy-miny);
	miny=-ycentering/100*h;
	maxy=miny+h;

	return ycentering;
}

/*! Incs count of newfont, unless newfont already equals font.
 *
 * By default, will link to the font, not create a duplicate.
 */
int CaptionData::Font(LaxFont *newfont)
{
	if (!newfont) return 1;

	if (font!=newfont) {
		if (font) font->dec_count();
		font=newfont;
		font->inc_count();
	}

	fontsize=font->textheight();
	linespacing=1;
	makestr(fontfamily,font->Family());
	makestr(fontstyle, font->Style());
	makestr(fontfile,  font->FontFile());

	DBG cerr <<"------------ new font size a,d,h, fs: "<<font->ascent()<<", "<<font->descent()<<", "<<font->textheight()<<"   "<<fontsize<<endl;

	state=0;
	FindBBox();

	return 0;
}

int CaptionData::Font(const char *file, const char *family,const char *style,double size)
{
	if (font) font->dec_count();

	if (file) font=InterfaceManager::GetDefault()->GetFontManager()->MakeFontFromFile(file, family,style,size,-1);
	else font=InterfaceManager::GetDefault()->GetFontManager()->MakeFont(family,style,size,-1);

	fontsize=size;
	linespacing=1;

	makestr(fontfile,  file   ? file   : font->FontFile());
	makestr(fontfamily,family ? family : font->Family());
	makestr(fontstyle, style  ? style  : font->Style());

	DBG cerr <<"------------ new font size a,d,h, fs: "<<font->ascent()<<", "<<font->descent()<<", "<<font->textheight()<<"   "<<fontsize<<endl;

	state=0;
	FindBBox();

	return 0;
}


//--------------------------------- CaptionInterface ---------------------------------
/*! \class CaptionInterface
 * \brief Interface for manipulating CaptionData objects.
 *
 * \todo this could also be used for little sticky notes in the viewport, these would
 *   be text blocks that are not transformed with the viewport's matrix..
 */
/*! \var int CaptionInterface::mode
 * 
 * If mode==0, then edit existing text normally. If mode==1, then we are
 * working on a brand new object. If the interface is called off without actually
 * writing any text, then do not install the object.
 */


CaptionInterface::CaptionInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	data=NULL;
	coc=NULL;
	extrahover=NULL;
	showdecs=3;
	showbaselines=0;
	showobj=1;
	mode=0;
	lasthover=CAPTION_None;

	defaultsize=20;
	defaultscale=-1;
	defaultfamily=newstr("sans");
	defaultstyle=newstr("");

	grabpad=20;
	caretline=0;//line number, starting from 0
	caretpos=0; //position of caret in caretline
	needtodraw=1;

	baseline_color=rgbcolorf(1.,0.,1.);

//	if (newtext) {
//		if (!data) data=new CaptionData(newtext,
//						 "sans", //font name   //"/usr/X11R6/lib/X11/fonts/TTF/temp/hrtimes_.ttf",
//						 NULL, //font style
//						 36, //font size
//						 0,  //xcenter,
//						 0); //ycenter
//	}

	sc=NULL;
}

CaptionInterface::~CaptionInterface()
{
	DBG cerr <<"----in CaptionInterface destructor"<<endl;
	deletedata();

	if (extrahover) { delete extrahover; extrahover=NULL; }
	if (sc) sc->dec_count();
	delete[] defaultfamily;
	delete[] defaultstyle;
}

//! Return new CaptionInterface.
/*! If dup!=NULL and it cannot be cast to CaptionInterface, then return NULL.
 */
anInterface *CaptionInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new CaptionInterface(id,NULL);
	else if (!dynamic_cast<CaptionInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

const char *CaptionInterface::Name()
{ return _("Caption Tool"); }

//! Sets showdecs=3, and needtodraw=1.
int CaptionInterface::InterfaceOn()
{
	DBG cerr <<"CaptionInterfaceOn()"<<endl;
	showdecs=3;
	needtodraw=1;
	mode=0;//***


//	if (!data) data=new CaptionData("\n0123\n  spaced line 3", 
//						 "sans", //font name   //"/usr/X11R6/lib/X11/fonts/TTF/temp/hrtimes_.ttf",
//						 NULL, //font style
//						 36, //font size
//						 0,  //xcenter,
//						 0); //ycenter
	return 0;
}

//! Calls Clear(), sets showdecs=0, and needtodraw=1.
int CaptionInterface::InterfaceOff()
{
	//Clear(NULL);
	deletedata(); 
	if (extrahover) { delete extrahover; extrahover=NULL; }
	showdecs=0;
	needtodraw=1;
	DBG cerr <<"CaptionInterfaceOff()"<<endl;
	return 0;
}

void CaptionInterface::Clear(SomeData *d)
{
	if ((d && d==data) || (!d && data)) {
		data->dec_count(); 
		data=NULL; 
	} 
}

//! Sets data=NULL.
/*! That results in data->dec_count() being called somewhere along the line.
 */
void CaptionInterface::deletedata()
{
	if (data) {
		if (data->lines.n==1 && data->lines.e[0][0]=='\0') {
			 //is a blank object, need to remove it
			data->dec_count();
			data=NULL;
			viewport->ChangeObject(coc, false);
			viewport->DeleteObject(); //this will also result in deletedata

		} else {
			data->dec_count();
			data=NULL;
		}
	}
	if (coc) { delete coc; coc=NULL; }
}

Laxkit::MenuInfo *CaptionInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	//if (!menu) menu=new MenuInfo();

	//menu->AddItem(_("Lorem ipsum..."));
	//menu->AddItem(_("Show all controls"));

	//if (data) {
		//menu->AddItem(_("Convert to "));
		//menu->AddItem(_(""));
		//menu->AddItem(_(""));
	//}

	return menu;
}

//! Draw ndata, but remember that data should still be the resident data afterward.
int CaptionInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<CaptionData *>(ndata)==NULL) return 1;

	CaptionData *d=data;
	data=dynamic_cast<CaptionData *>(ndata);
	int td=showdecs, ntd=needtodraw, oldshowobj=showobj;
	showdecs=0;
	showobj=1;
	needtodraw=1;

	Refresh();

	needtodraw=ntd;
	showobj=oldshowobj;
	showdecs=td;
	data=d;
	return 1;
}

/*! Note that this calls  imlib_context_set_drawable(dp->GetWindow()).
 * Please note that Imlib does not naturally handle skewed fonts, and this function
 * doesn't either.
 *
 * Returns 1 if no data, -1 if thing was offscreen, or 0 if thing drawn.
 *
 * \todo should work this to not need imlib, allowing a redefinable backend to render the text.
 */
int CaptionInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	needtodraw=0;
	if (!data) return 1;

	dp->font(data->fontfamily,data->fontstyle,data->fontsize);//1 for real size, not screen size
	if (!data->state) {
		 //need to find line lengths
		for (int c=0; c<data->lines.n; c++) {
			data->linelengths.e[c]=dp->textextent(data->font, data->lines.e[c],-1, NULL,NULL,NULL,NULL,0);
		}
		data->state=1;
		data->FindBBox();
	}

	if (extrahover) {
		dp->NewFG(255,0,255);
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
		
		double m[6];
		if (data) {
			viewport->transformToContext(m,coc,1,-1);
			dp->PushAndNewTransform(m);
		}

		viewport->transformToContext(m,extrahover,0,-1);
		dp->PushAndNewTransform(m);
		dp->moveto(extrahover->obj->minx, extrahover->obj->miny);
		dp->lineto(extrahover->obj->maxx, extrahover->obj->miny);
		dp->lineto(extrahover->obj->maxx, extrahover->obj->maxy);
		dp->lineto(extrahover->obj->minx, extrahover->obj->maxy);
		dp->closed();
		dp->stroke(0);
		dp->PopAxes(); 

		if (data) dp->PopAxes();

	}
		
	 //find how large
	flatpoint pb =flatpoint(0,0),
			  pt =flatpoint(0, data->fontsize*data->linespacing),
			  ptt=flatpoint(0, pt.y * data->lines.n);
	int height=(int)norm(dp->realtoscreen(pb)-dp->realtoscreen(pt));
	double totalheight=norm(dp->realtoscreen(pb)-dp->realtoscreen(ptt));
	if (totalheight<.5) return 0;


	//if (!coc) dp->PushAndNewTransform(data->m());


	 // find the screen box to draw into
	 // these still need the 'dp->' because dp was transformed to space immediately holding data already
	 // The viewport is the base transform, usually different then dp coords...
	flatpoint ul=dp->realtoscreen(flatpoint(data->minx,data->miny)), 
			  ur=dp->realtoscreen(flatpoint(data->maxx,data->miny)), 
			  ll=dp->realtoscreen(flatpoint(data->minx,data->maxy)), 
			  lr=dp->realtoscreen(flatpoint(data->maxx,data->maxy));
	//flatpoint cursor=ul;
	//flatpoint advance=ur-ul;
	flatpoint v=ll-ul;
	v=v/norm(v);
	//double boxtotalheight=norm(ul-ll);
	//double lineheight=boxtotalheight/data->lines.n;

	DBG fprintf(stderr,"draw caption scr coords: %ld: ul:%g,%g ur:%g,%g ll:%g,%g lr:%g,%g\n",
	DBG		data->object_id,ul.x,ul.y,ur.x,ur.y,ll.x,ll.y,lr.x,lr.y);
	DBG fprintf(stderr,"     caption bounds:    w:%g  h:%g\n",
	DBG		norm(ul-ur),norm(ul-ll));
	
	 // check for positive intersection of transformed image to dp view area
	DoubleBBox bbox(ul);
	bbox.addtobounds(ur);
	bbox.addtobounds(ll);
	bbox.addtobounds(lr);
	if (!bbox.intersect(dp->Minx,dp->Maxx,dp->Miny,dp->Maxy)) {
		DBG cerr <<"----------------CaptionData outside viewport"<<endl;
		if (!coc) dp->PopAxes();
		return -1;
	}
	//---or---
	//flatpoint pts[4]={dp->screentoreal(ul,ur,ll,lr)}
	//bbox.bounds(pts,4);
	//bbox.intersect(data->minx,data->maxx,data->miny,data->maxy, settointersection);

	
	 // draw control points;
	if (showdecs&1) { 
		dp->DrawScreen();
		 // dashed outline around text..
		dp->LineAttributes(1,LineOnOffDash,CapRound,JoinRound);
		dp->NewFG(255,0,0);
		dp->drawline(ul,ur);
		dp->drawline(ur,lr);
		dp->drawline(lr,ll);
		dp->drawline(ll,ul);
//		dp->NewFG(controlcolor);


		 //draw little circle around origin
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
		flatpoint p=dp->realtoscreen(0,0);
		dp->drawpoint(p, 4,0);


		 //draw alignment knobs
		dp->NewFG(0.0,0.0,1.0,.25);
		double xs=grabpad/dp->Getmag()/2;
		double ys=grabpad/dp->Getmag(1)/2;

		dp->DrawReal();
		dp->LineAttributes(-1,LineSolid,CapRound,JoinRound);
		dp->LineWidthScreen(1);
		
		if (xs<(data->maxx-data->minx)/5) {
			p=flatpoint((data->maxx+data->minx)/2,data->miny-ys*.7);
			dp->drawellipse(p, xs,ys*.35, 0,0, lasthover==CAPTION_HAlign ? 1 : 0);
			p=flatpoint((data->maxx+data->minx)/2,data->maxy+ys*.7);
			dp->drawellipse(p, xs,ys*.35, 0,0, lasthover==CAPTION_HAlign ? 1 : 0);
		}
		if (ys<(data->maxy-data->miny)/5) {
			p=flatpoint(data->minx-xs*.7, (data->maxy+data->miny)/2);
			dp->drawellipse(p, xs*.35,ys, 0,0, lasthover==CAPTION_VAlign ? 1 : 0);
			p=flatpoint(data->maxx+xs*.7, (data->maxy+data->miny)/2);
			dp->drawellipse(p, xs*.35,ys, 0,0, lasthover==CAPTION_VAlign ? 1 : 0);
		}


		 //draw size handle
		dp->NewFG(.5,.5,.5,.5);
		dp->moveto(data->maxx+xs+xs/2, data->maxy);
		dp->lineto(data->maxx+xs, data->miny);
		dp->lineto(data->maxx+xs+xs, data->miny);
		dp->closed();
		if (lasthover==CAPTION_Size) dp->fill(0); else dp->stroke(0);


		 //draw move and rotate indicators
		if (lasthover==CAPTION_Move) {
			dp->NewFG(.5,.5,.5,.5);
			dp->drawrectangle(data->minx-xs,data->miny-ys, data->maxx-data->minx+2*xs,ys, 1);
			dp->drawrectangle(data->minx-xs,data->maxy, data->maxx-data->minx+2*xs,ys, 1);
			dp->drawrectangle(data->minx-xs,data->miny, xs,data->maxy-data->miny, 1);
			dp->drawrectangle(data->maxx,data->miny, xs,data->maxy-data->miny, 1);

		} else if (lasthover==CAPTION_Rotate) {
			dp->NewFG(.5,.5,.5,.5);
			xs*=2;
			ys*=2;

			dp->drawellipse(flatpoint(data->minx,data->miny), xs/2,ys/2, M_PI/2, 2*M_PI, 1);
			dp->drawellipse(flatpoint(data->maxx,data->miny), xs/2,ys/2, -M_PI, M_PI/2, 1);
			dp->drawellipse(flatpoint(data->maxx,data->maxy), xs/2,ys/2, -M_PI/2, M_PI, 1);
			dp->drawellipse(flatpoint(data->minx,data->maxy), xs/2,ys/2, 0, 3*M_PI/2, 1);

			//dp->drawrectangle(data->minx,data->miny-2*ys, xs,ys, 1);
		}

	}

	dp->NewFG(data->red, data->green, data->blue, data->alpha);

	if (showobj) {
		int texttoosmall=0;
		if (height==0) {
			texttoosmall=1;
			//*** just draw little lines
			//return 0;
		}

		 //draw the text
		if (!texttoosmall) {
			dp->font(data->font);

			 //draw the stuff
			double x,y;
			double baseline=data->font->ascent(), descent=data->font->descent();
			//double width=data->maxx-data->minx;
			y=-data->ycentering/100*data->fontsize*data->linespacing*data->lines.n;
			for (int c=0; c<data->lines.n; c++, y+=data->fontsize*data->linespacing) {
				x=-data->xcentering/100*(data->linelengths[c]);

				if (showbaselines) {
					dp->NewFG(baseline_color);
					dp->LineWidthScreen(1);

					 //baseline
					dp->drawline(x,y+baseline, x+data->linelengths.e[c],y+baseline);
					 //ascent/descent
					dp->LineAttributes(-1, LineDoubleDash, LAXCAP_Round, LAXJOIN_Round);
					dp->drawline(x,y, x+data->linelengths.e[c],y);
					dp->drawline(x,y+baseline+descent, x+data->linelengths.e[c],y+baseline+descent);
					dp->LineAttributes(-1, LineSolid, LAXCAP_Round, LAXJOIN_Round);

					dp->NewFG(data->red, data->green, data->blue, data->alpha);
				}

				if (!isblank(data->lines.e[c])) {
					dp->textout(x,y, data->lines.e[c],-1, LAX_TOP|LAX_LEFT);
				}

				 //draw caret
				if (c==caretline && showdecs) {
					dp->NewFG(0.0, .5, 0.0, 1.0);
					double ex=dp->textextent(data->font, data->lines.e[c],caretpos, NULL,NULL,NULL,NULL,0);
					double tick=data->fontsize/10;
					dp->drawline(x+ex,y, x+ex,y+data->fontsize);
					dp->drawline(x+ex,y, x+ex-tick,y-tick);
					dp->drawline(x+ex,y, x+ex+tick,y-tick);
					dp->drawline(x+ex,y+data->fontsize, x+ex-tick,y+data->fontsize+tick);
					dp->drawline(x+ex,y+data->fontsize, x+ex+tick,y+data->fontsize+tick);
					dp->NewFG(data->red, data->green, data->blue, data->alpha);
				}
			}

			dp->font(app->defaultlaxfont);

		} else {
			//*** text is too small, draw little lines
			DBG cerr <<"small text, draw little gray lines instead..."<<endl;
		}
	}
	

	//DBG cerr<<"..Done drawing CaptionInterface"<<endl;
	//if (!coc) dp->PopAxes();
	return 0;
}

ObjectContext *CaptionInterface::Context()
{
	return coc;
}

//! Create and return new data, also calls viewport->newData(newdata,0).
/*! Please note that this function is not a redefinition of anything. It is used
 * internally to get a new instance of CaptionData and also tell the viewport about it.
 *
 * This function causes a new CaptionData to be created and to have a count of 2.
 * Currently, new CaptionData makes the maxx/maxy be the image pixel width/height..
 *
 * \todo need some flag somewhere to auto flip vertically (for +y is up rather than down)
 */
CaptionData *CaptionInterface::newData()
{
	CaptionData *ndata=NULL;

	ndata=dynamic_cast<CaptionData *>(somedatafactory()->NewObject(LAX_CAPTIONDATA));
	//if (ndata) ndata->SetText("\nline 2\nthird line");

	if (!ndata) ndata=new CaptionData();
	
	ndata->Font(NULL, defaultfamily, defaultstyle, defaultsize);

	return ndata;
}

/*! Apply color from LineStyle.
 */
int CaptionInterface::UseThis(anObject *newdata,unsigned int) // assumes not use local
{   
    if (!newdata || !data) return 0;
    
    if (data && dynamic_cast<LineStyle *>(newdata)) { // make all selected points have this color
        DBG cerr <<"Grad new color stuff"<< endl;
        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(newdata);

        if (nlinestyle->mask&GCForeground) {
			data->red  =nlinestyle->color.red/65535.;
			data->green=nlinestyle->color.green/65535.;
			data->blue =nlinestyle->color.blue/65535.;
			data->alpha=nlinestyle->color.alpha/65535.;
            
            data->touchContents();
            needtodraw=1;
        } 

        needtodraw=1;
        return 1;
    }

    return 0;
}

/*! Make sure caretline and caretpos are within bounds for data.
 */
void CaptionInterface::FixCaret()
{
	caretline=0;
	caretpos=0;
}

//! Use the object at oc if it is an ImageData.
int CaptionInterface::UseThisObject(ObjectContext *oc)
{   
    if (!oc) return 0;
    
    CaptionData *ndata=dynamic_cast<CaptionData *>(oc->obj);
    if (!ndata) return 0;
    
    if (data && data!=ndata) deletedata();
    if (coc) delete coc;
    coc=oc->duplicate();
    
    if (data!=ndata) {
        data=ndata;
        data->inc_count();
    }

	defaultsize=data->fontsize;
	makestr(defaultfamily,data->fontfamily);
	makestr(defaultstyle, data->fontstyle);
	defaultscale=data->xaxis().norm();

	SimpleColorEventData *e=new SimpleColorEventData( 65535, 0xffff*data->red, 0xffff*data->green, 0xffff*data->blue, 0xffff*data->alpha, 0);
	app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);

	FixCaret();

    needtodraw=1;
    return 1;
}   

//! If !data on LBDown, then make a new one...
int CaptionInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if (child) return 1;

	DBG cerr << "  in captioninterface lbd..";
	int mline,mpos;
	int over=scan(x,y,state, &mline, &mpos);
	buttondown.down(d->id,LEFTBUTTON,x,y, over);
	mousedragged=0;
	if (extrahover) { delete extrahover; extrahover=NULL; }

	if (data && count==2) {
		app->addwindow(new FontDialog(NULL, "Font",_("Font"),ANXWIN_REMEMBER, 10,10,700,700,0, object_id,"newfont",0,
					data->fontfamily, data->fontstyle, data->fontsize,
					NULL, //sample text
					data->font, true
					));
		buttondown.up(d->id,LEFTBUTTON);
		return 0;
	}

	if (over!=CAPTION_None) {
		if (over==CAPTION_Text) {
			caretline=mline;
			caretpos=mpos;
			needtodraw=1;
		}
		return 0; //clicked down on something for current data
	}


	 // make new one or find other one.
	CaptionData *obj=NULL;
	ObjectContext *oc=NULL;
	int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
	if (c>0) obj=dynamic_cast<CaptionData *>(oc->obj); //***actually don't need the cast, if c>0 then obj is CaptionData

	if (obj) { 
		 // found another CaptionData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		if (data) deletedata();

		data=obj;
		data->inc_count();
		if (coc) delete coc;
		coc=oc->duplicate();
		
		if (viewport) viewport->ChangeObject(oc,0);
		buttondown.moveinfo(d->id,LEFTBUTTON, CAPTION_Move);

		defaultsize=data->fontsize;
		makestr(defaultfamily,data->fontfamily);
		makestr(defaultstyle, data->fontstyle);
		defaultscale=data->xaxis().norm();

		SimpleColorEventData *e=new SimpleColorEventData( 65535, 0xffff*data->red, 0xffff*data->green, 0xffff*data->blue, 0xffff*data->alpha, 0);
		app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);

		FixCaret();

		needtodraw=1;
		return 0;

	} else if (c<0) {
		 // If there is some other non-image data underneath (x,y) and
		 // this is not primary, then switch objects, and switch tools to deal
		 // with that object.
		//******* this will have to be ChangeObject(oc,transfer lbdown) or some such
		if (!primary && c==-1 && viewport->ChangeObject(oc,1)) {
			buttondown.up(d->id,LEFTBUTTON);
			return 0;
		}
	}

	 // To be here, must want brand new data plopped into the viewport context
	if (viewport) viewport->ChangeContext(x,y,NULL);
	//mode=1; //drag out text area
	mode=0;
	deletedata();
	data=newData(); 
	needtodraw=1;

	if (!data) return 0;
	if (data->maxx>data->minx && data->maxy>data->miny) mode=0;
	caretline=0;
	caretpos=0;

	leftp=screentoreal(x,y);
	data->origin(leftp);
	if (defaultscale<=0) defaultscale=1./Getmag()/2;
	data->xaxis(flatpoint(defaultscale,0));
	data->yaxis(flatpoint(0,defaultscale));
	if (dp->defaultRighthanded()) {
		data->yaxis(-data->yaxis());
	}
	DBG data->dump_out(stderr,6,0,NULL);

	SimpleColorEventData *e=new SimpleColorEventData( 65535, 0xffff*data->red, 0xffff*data->green, 0xffff*data->blue, 0xffff*data->alpha, 0);
	app->SendMessage(e, curwindow->win_parent->object_id, "make curcolor", object_id);
	
	if (viewport) {
		ObjectContext *oc=NULL;
		viewport->NewData(data,&oc);//viewport adds only its own counts
		if (coc) { delete coc; coc=NULL; }
		if (oc) coc=oc->duplicate();
	}

	return 0; 
}

int CaptionInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{
	if (!data) return 1;

	if (!strcmp(mes, "newfont")) {
		const StrsEventData *s=dynamic_cast<const StrsEventData*>(e_data);
		if (!s) return 1;

		if (!data) return 0;

		double size=strtod(s->strs[2], NULL);
		if (size<=0) size=1e-4;

		LaxFont *newfont=dynamic_cast<LaxFont*>(s->object);
		if (newfont) data->Font(newfont);
		else data->Font(s->strs[3], s->strs[0], s->strs[1], size); //file, family, style, size
		data->state=0;
		data->FindBBox();
	
		DBG cerr <<"------------ new font size a,d,h, fs: "<<data->font->ascent()<<", "<<data->font->descent()<<", "<<data->font->textheight()<<"   "<<data->fontsize<<endl;

		defaultsize=size;
		makestr(defaultfamily,data->fontfamily);
		makestr(defaultstyle, data->fontstyle);
		needtodraw=1;

		return 0;

	} else if (!strcmp(mes, "size")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(e_data);
		if (!s) return 1;
		char *end=NULL;
		double size=strtod(s->str,&end);
		if (end!=s->str && size>0) {
			data->Size(size);
			defaultsize=data->fontsize;
			needtodraw=1;
		}

		DBG cerr <<"------------ new font size a,d,h, fs: "<<data->font->ascent()<<", "<<data->font->descent()<<", "<<data->font->textheight()<<"   "<<data->fontsize<<endl;
		return 0;

	} else if (!strcmp(mes, "angle")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(e_data);
		if (!s) return 1;
		char *end=NULL;
		double angle=strtod(s->str,&end)/180.*M_PI;
		if (end!=s->str) {
			double oldang=angle_full(data->xaxis(),data->yaxis());
			double oldy=norm(data->yaxis());
			data->xaxis(norm(data->xaxis())*flatpoint(cos(angle),sin(angle)));
			data->yaxis(oldy*flatpoint(cos(angle+oldang),sin(angle+oldang)));
			needtodraw=1;
		}
		return 0;

	} else if (!strcmp(mes, "insert char")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(e_data);
		if (!s) return 1;

		int ch=s->info1;

		if (caretline<0) caretline=0;
		if (caretpos<0) caretpos=0;

		if (data) data->InsertChar(ch,caretline,caretpos,&caretline,&caretpos);
		needtodraw=1;

		return 0;
	}

	return 1;
}

//! If data, then call viewport->ObjectMoved(data).
int CaptionInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	if (child) return 1;

	if (mode==1 && !mousedragged && data) {
		DBG cerr <<"**CaptionInterface Clear() for no mouse move on new data"<<endl;
		if (viewport) viewport->DeleteObject();
		anInterface::Clear();

	} else {
		if (!mousedragged) {
			int over;
			buttondown.getextrainfo(d->id,LEFTBUTTON, &over);

			const char *what=NULL;
			char str[30];

			if (over==CAPTION_Size) {
				what="size";
				sprintf(str,"%.10g",data->fontsize);

			} else if (over==CAPTION_Rotate) {
				what="angle";
				flatpoint x=data->xaxis();
				sprintf(str,"%.10g",x.angle()*180/M_PI);
			}

			if (what) {
				double th=app->defaultlaxfont->textheight();
				DoubleBBox bounds(x+th, x+20*th, y-th/2, y+th/2);
				viewport->SetupInputBox(object_id, NULL, str, what, bounds);
			}

		} else if (data && viewport) viewport->ObjectMoved(coc,1);
	}

	mode=0;
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}

int CaptionInterface::scan(int x,int y,unsigned int state, int *line, int *pos)
{
	if (!data) return CAPTION_None;

	double xmag=norm(dp->realtoscreen(transform_point(data->m(),flatpoint(1,0)))
                    -dp->realtoscreen(transform_point(data->m(),flatpoint(0,0))));
    double ymag=norm(dp->realtoscreen(transform_point(data->m(),flatpoint(0,1)))
                    -dp->realtoscreen(transform_point(data->m(),flatpoint(0,0))));

	double xm=grabpad/xmag/2;
	double ym=grabpad/ymag/2;
	//double xm=grabpad/Getmag();
	//double ym=grabpad/Getmag(1);
	DBG cerr <<"caption scan xm: "<<xm<<"  ym: "<<ym<<endl;

	flatpoint p=screentoreal(x,y);
	p=data->transformPointInverse(p);

	//--------------
	//flatpoint ul=realtoscreen(flatpoint(data->minx,data->miny));
    //flatpoint ur=realtoscreen(flatpoint(data->maxx,data->miny));
    //flatpoint ll=realtoscreen(flatpoint(data->minx,data->maxy));
    //flatpoint lr=realtoscreen(flatpoint(data->maxx,data->maxy));


	if (p.x>=data->minx && p.x<=data->maxx && p.y>=data->miny && p.y<=data->maxy) {
		if (line) {
			data->FindPos(p.y, p.x, line,pos);
		}
		return CAPTION_Text;
	}

	if (p.x>=data->minx-xm && p.x<=data->maxx+xm+xm
			&& p.y>=data->miny-ym && p.y<=data->maxy+ym) {

		flatpoint m((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);

		if (p.x>data->maxx+xm) return CAPTION_Size;
		if (xm<(data->maxx-data->minx)/5 && p.x>m.x-xm && p.x<m.x+xm) return CAPTION_HAlign;
		if (ym<(data->maxy-data->miny)/5 && p.y>m.y-ym && p.y<m.y+ym) return CAPTION_VAlign;
		if (p.x<data->minx+xm && p.y<data->miny+ym) return CAPTION_Rotate;
		if (p.x<data->minx+xm && p.y>data->maxy-ym) return CAPTION_Rotate;
		if (p.x>data->maxx-xm && p.y<data->miny+ym) return CAPTION_Rotate;
		if (p.x>data->maxx-xm && p.y>data->maxy-ym) return CAPTION_Rotate;
		return CAPTION_Move;
	}


	return CAPTION_None;
}

int CaptionInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *mouse) 
{
	if (child) return 0;

	if (!buttondown.any() || !data) {
		int hover=scan(x,y,state, NULL,NULL);

		if (hover==CAPTION_None) {
			 //set up to outline potentially editable other captiondata
			ObjectContext *oc=NULL;
			int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
			if (c>0) {
				DBG cerr <<"caption mouse over: "<<oc->obj->Id()<<endl;

				if (!oc->isequal(extrahover)) {
					extrahover=oc->duplicate();
					needtodraw=1;
				}
			} else if (extrahover) {
				delete extrahover;
				extrahover=NULL;
				needtodraw=1;
			}
		}

		if (hover!=CAPTION_None && extrahover) {
			delete extrahover;
			extrahover=NULL;
			needtodraw=1;
		}

		if (hover!=lasthover) {
			lasthover=hover;
			needtodraw=1;
			if (lasthover==CAPTION_Move) PostMessage(_("Move"));
			else if (lasthover==CAPTION_HAlign) PostMessage(_("Horizontal alignment"));
			else if (lasthover==CAPTION_VAlign) PostMessage(_("Vertical alignment"));
			else if (lasthover==CAPTION_Rotate) PostMessage(_("Rotate, shift to snap"));
			else if (lasthover==CAPTION_Size) PostMessage(_("Drag for font size, click to input"));
			else if (lasthover==CAPTION_Text) PostMessage(_("Text"));
			else PostMessage(" ");
			return 0;
		}

		return 0;
	}

	int mx,my;
	buttondown.move(mouse->id, x,y, &mx,&my);
	if (!data) return 0;

	int over=0;
	buttondown.getextrainfo(mouse->id, LEFTBUTTON, &over);
	if (x!=mx || y!=my) mousedragged=1;
	else return 0; 

//	double xmag=norm(dp->realtoscreen(transform_point(data->m(),flatpoint(1,0)))
//                    -dp->realtoscreen(transform_point(data->m(),flatpoint(0,0))));
//	double ymag=norm(dp->realtoscreen(transform_point(data->m(),flatpoint(0,1)))
//                    -dp->realtoscreen(transform_point(data->m(),flatpoint(0,0))));

	flatpoint dv= data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(mx,my));

	if (mode==0) {
		 //normal editing

		if (over==CAPTION_Move) {
			flatpoint d=screentoreal(x,y)-screentoreal(mx,my); // real vector from data->origin() to mouse move to 
			data->origin(data->origin() + d); 

		} else if (over==CAPTION_Size) {
			double factor=2;
			if (state&ShiftMask) factor*=.1;
			if (state&ControlMask) factor*=.1;
			double d=data->fontsize - factor*dv.y;
			if (d<=0) d=1e-3;
			data->Size(d);
			char str[100];
			sprintf(str, _("Size %f pt"), d);

			DBG cerr <<"------------ new font size a,d,h, fs: "<<data->font->ascent()<<", "<<data->font->descent()<<", "<<data->font->textheight()<<"   "<<data->fontsize<<endl;
			PostMessage(str);
			needtodraw=1;
			return 0;

		} else if (over==CAPTION_Rotate) {
			if (state&ShiftMask) {
				 //snap to 0/90/180/270
				int ox,oy;
				buttondown.getinitial(mouse->id, LEFTBUTTON, &ox,&oy);
				double angle=-angle_full(screentoreal(ox,oy)-data->origin(), screentoreal(x,y)-data->origin());

				double snapdiv = M_PI/2/6;
				angle = (angle+snapdiv/2)/snapdiv;
				angle = snapdiv*int(angle);

				data->setRotation(angle);

			} else data->RotatePointed(data->origin(), screentoreal(mx,my),screentoreal(x,y));
			needtodraw=1;
			return 0;

		} else if (over==CAPTION_HAlign || over==CAPTION_VAlign) {
			int which=0;
			if (over==CAPTION_HAlign) which|=1; else which|=2;
			if (state&ControlMask) which|=3;

			if ((state&ShiftMask)==0) {
				flatpoint p=data->transformPointInverse(screentoreal(x,y));
				p.x=-50*floor(p.x/(data->maxx-data->minx)*2-.5);
				if (p.x<0) p.x=0; else if (p.x>100) p.x=100;
				p.y=-50*floor(p.y/(data->maxy-data->miny)*2-.5);
				if (p.y<0) p.y=0; else if (p.y>100) p.y=100;

				if (which&1) data->xcentering=p.x;
				if (which&2) data->ycentering=p.y;

			} else {
				double dx=100*dv.x/(data->maxx-data->minx);
				double dy=100*dv.y/(data->maxy-data->miny);

				if (which&1) data->xcentering-=dx;
				if (which&2) data->ycentering-=dy;
			}

			data->FindBBox();
			char str[100];
			sprintf(str,"Align: %f, %f",data->xcentering,data->ycentering);
			PostMessage(str);
			needtodraw=1;
			return 0;

		}

	}

	needtodraw|=2;
	return 0;
}

enum CaptionInterfaceActions {
	CAPT_None=0,
	CAPT_Copy,
	CAPT_Paste,
	CAPT_LeftJustify,
	CAPT_CenterJustify,
	CAPT_RightJustify,
	CAPT_TopJustify,
	CAPT_MiddleJustify,
	CAPT_BaselineJustify,
	CAPT_BottomJustify,
	CAPT_Direction,
	CAPT_Decorations,
	CAPT_ShowBaselines,
	CAPT_InsertChar,
	CAPT_CombineChars,
	CAPT_MAX
};

Laxkit::ShortcutHandler *CaptionInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc=new ShortcutHandler(whattype());

    sc->Add(CAPT_Copy,            'c',ControlMask,0, "Copy"           , _("Copy"            ),NULL,0);
    sc->Add(CAPT_Paste,           'v',ControlMask,0, "Paste"          , _("Paste"           ),NULL,0);
    sc->Add(CAPT_LeftJustify,     'l',ControlMask,0, "LeftJustify"    , _("Left Justify"    ),NULL,0);
    sc->Add(CAPT_CenterJustify,   'e',ControlMask,0, "CenterJustify"  , _("Center Justify"  ),NULL,0);
    sc->Add(CAPT_RightJustify,    'r',ControlMask,0, "RightJustify"   , _("Right Justify"   ),NULL,0);
    sc->Add(CAPT_TopJustify,      't',ControlMask,0, "TopJustify"     , _("Top Justify"     ),NULL,0);
    sc->Add(CAPT_MiddleJustify,   'm',ControlMask,0, "MiddleJustify"  , _("Middle Justify"  ),NULL,0);
    sc->Add(CAPT_BaselineJustify, 'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),NULL,0);
    sc->Add(CAPT_BottomJustify,   'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
    sc->Add(CAPT_Direction,       'D',ShiftMask|ControlMask,0, "Direction", _("Toggle Direction"),NULL,0);
    sc->Add(CAPT_Decorations,     'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
    sc->Add(CAPT_ShowBaselines,   'D',ShiftMask|ControlMask,0, "ShowBaselines", _("Show Baselines"),NULL,0);
    sc->Add(CAPT_InsertChar,      'i',ControlMask,0, "InsertChar"     , _("Insert Character"),NULL,0);
    sc->Add(CAPT_CombineChars,    'j',ControlMask,0, "CombineChars"   , _("Join Characters if possible"),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

int CaptionInterface::PerformAction(int action)
{
	if (action==CAPT_Paste) {
		 //pasting with no data should create a new data
		PostMessage(" *** Need to implement paste!!!");
		return 0;
	}

	 //everything else needs a data
	if (!data) return 1;

	if (action==CAPT_Copy) {
		PostMessage(" *** Need to implement copy!!!");
		return 0;

	} else if (action==CAPT_Decorations) {
		showdecs=!showdecs;
		if (showdecs) PostMessage(_("Show controls"));
		else PostMessage(_("Hide controls"));
		needtodraw=1;
		return 0;

	} else if (action==CAPT_LeftJustify) {
		data->xcentering=0;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_CenterJustify) {
		data->xcentering=50;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_RightJustify) {
		data->xcentering=100;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_TopJustify) {
		data->ycentering=0;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_MiddleJustify) {
		data->ycentering=50;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_BaselineJustify) {
		PostMessage(" *** need to implement baseline justify action in CaptionInterface!!");
		//data->ycentering=50;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_BottomJustify) {
		data->ycentering=100;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_InsertChar) {
		if (data && !child) {
			CharacterInterface *chari = new CharacterInterface(this, 0, dp, data->font);
			child=chari;
			viewport->Push(chari, viewport->HasInterface(object_id)-1,0);
		}
		return 0;

	} else if (action==CAPT_ShowBaselines) {
		showbaselines++;
		if (showbaselines>1) showbaselines=0;
		needtodraw=1;
		return 0;

	} else if (action==CAPT_Direction) {
		data->righttoleft=!data->righttoleft;
		PostMessage(" *** need to implement rtl/ltr toggle action in CaptionInterface!!");
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPT_CombineChars) {
		if (!data) return 1;

		if (caretpos==0) caretpos++;
		if (caretpos>data->CharLen(caretline)) caretpos=data->CharLen(caretline);

		long i1=caretpos-2;
		long i2=caretpos-1;
		if (i1<0 || i2<0) {
			PostMessage(_("Must have 2 characters to combine!"));
			return 0;
		}

		int newch = composekey(data->lines.e[caretline][i1],data->lines.e[caretline][i2]);
		if (newch==0 || newch==data->lines.e[caretline][i2]) {
			PostMessage(_("Could not combine"));
			return 0;
		}

		data->DeleteChar(caretline,caretpos,0, &caretline,&caretpos);
		data->DeleteChar(caretline,caretpos,0, &caretline,&caretpos);
		data->InsertChar(newch,caretline,caretpos,&caretline,&caretpos);
		needtodraw=1;
		return 0;
	}

	return 1;
}

int CaptionInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d) 
{

	if (!data) return 1;


	if (ch==LAX_Esc && child) {
		RemoveChild();
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Del) { // delete
		data->DeleteChar(caretline,caretpos,1, &caretline,&caretpos);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Bksp) { // backspace
		data->DeleteChar(caretline,caretpos,0, &caretline,&caretpos);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Home) {
		caretpos=0;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_End) {
		caretpos=data->CharLen(caretline);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Left) { // left
		if (caretpos<0) caretpos=0;
		else if (caretpos>0) {
			caretpos--;
			caretpos=utf8back_index(data->lines.e[caretline], caretpos, strlen(data->lines.e[caretline]));
			needtodraw=1;
		}
		return 0;

	} else if (ch==LAX_Right) { // right
		const char *line = data->lines.e[caretline];
		if (line[caretpos]) {
			caretpos++;
			const char *pos = utf8fwd(line+caretpos, line, line+strlen(line));
			caretpos = pos-line;
			if (caretpos>data->CharLen(caretline)) caretpos=data->CharLen(caretline);
			needtodraw=1;
		}
		return 0;

	} else if (ch==LAX_Up) { // up
		caretline--;
		if (caretline<0) caretline=0;
		if (caretpos>(int)strlen(data->lines.e[caretline]))
			caretpos=strlen(data->lines.e[caretline]);
		caretpos=utf8back_index(data->lines.e[caretline], caretpos, strlen(data->lines.e[caretline]));
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Down) { // down
		caretline++;
		if (caretline>=data->lines.n) caretline=data->lines.n-1;
		if (caretpos>(int)strlen(data->lines.e[caretline]))
			caretpos=strlen(data->lines.e[caretline]);
		caretpos=utf8back_index(data->lines.e[caretline], caretpos, strlen(data->lines.e[caretline]));
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Enter) {
		data->InsertChar('\n',caretline,caretpos,&caretline,&caretpos);
		needtodraw=1;
		return 0;

	} else if (ch>=32 && ch<0xff00 && (state&(ControlMask|AltMask|MetaMask))==0) {
		 // add character to text
		if (caretline<0) caretline=0;
		if (caretpos<0) caretpos=0;

		data->InsertChar(ch,caretline,caretpos,&caretline,&caretpos);
		needtodraw=1;
		return 0;

	} else {

		if (!sc) GetShortcuts();
		int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
		if (action>=0) {
			return PerformAction(action);
		}
	}

	return 1; 
}

int CaptionInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d) 
{
	return 1; 
}


} // namespace LaxInterfaces


