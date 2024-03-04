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
//    Copyright (C) 2015-2017 by Tom Lechner
//

#include <string.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>

#include FT_OUTLINE_H

#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/texttopath.h>
#include <lax/interfaces/characterinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/viewportwindow.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/somedataref.h>
#include <lax/interfaces/groupdata.h>
#include <lax/colorevents.h>
#include <lax/laxutils.h>
#include <lax/fontdialog.h>
#include <lax/fontmanager.h>
#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/utf8utils.h>
#include <lax/language.h>


//template implementation:
#include <lax/refptrstack.cc>
#include <lax/lists.cc>

using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

#define sgn(a) ((a)<0?-1:((a)>0?1:0))


namespace LaxInterfaces {


//static const char *loremipsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";


//line spacing handle size as percentage of bounding box height
static const double LSSIZE=.25;


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
	fontfamily  = newstr("sans");
	fontstyle   = newstr("normal");
	fontfile    = NULL;
	fontsize    = 24;
	linespacing = 1;
	font        = NULL;

	direction = -1;
	language  = NULL;
	script    = NULL;

	state     = 0;

	xcentering    = 0;
	ycentering    = 0;
	baseline_hint = -1;

	lines.push(newstr(""));
	linelengths.push(0);
	linestats.push(new Linestat(0,0,0,0,0));

	color = NULL;
	red = green = blue = .5;
	alpha = 1.;

	needtorecache = true;

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

	direction=-1;
	language=NULL;
	script=NULL;
	
	fontsize=fsize;
	linespacing=1;
	if (fontsize<=0) fontsize=24;
	font=NULL;

	state=0;  //0 means someone needs to remap extents


	int numlines=0;
	char **text=split(ntext,'\n',&numlines);
	for (int c=0; c<numlines; c++) {
		lines.push(text[c]);
		linelengths.push(0);
		linestats.push(new Linestat(0,0,0,0,0));
	}
	xcentering=xcenter;
	ycentering=ycenter;
	baseline_hint=-1;

	if (numlines==0) {
		lines.push(newstr(""));
		linelengths.push(0);
		linestats.push(new Linestat(0,0,0,0,0));
		numlines=1;
	}

	color=NULL;
	red=green=blue=.5;
	alpha=1.;

	needtorecache=true;
	Font(fontfile, fontfamily, fontstyle, fontsize);

	DBG if (ntext) cerr <<"CaptionData new text:"<<endl<<ntext<<endl;
	DBG cerr <<"..CaptionData end"<<endl;
}

CaptionData::~CaptionData()
{
	DBG cerr <<"in CaptionData destructor"<<endl;

	delete[] fontfamily;
	delete[] fontstyle;
	delete[] fontfile;

	delete[] language;
	delete[] script;

	if (color) color->dec_count();
	if (font)  font ->dec_count();

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

	i->direction   =direction;
	makestr(i->language, language);
	makestr(i->script,   script);
	//i->language    =language;
	//i->script      =script;

	if (color) {
		if (color->ObjectOwner() == this) {
			 //dup color
			i->color = color->duplicate();
		} else {
			 //link color
			i->color = color;
			color->inc_count();
		}
	}
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

void CaptionData::FindBBox()
{
	double width=fontsize;
	double height=fontsize + (lines.n-1)*fontsize*linespacing;
	if (linespacing<0) {
		height=fontsize - (lines.n-1)*fontsize*linespacing;
	}


	if (state==0 || NeedToRecache()) {
		ComputeLineLen(-1);
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

	 //check for effects of negative line spacing
//	if (linespacing<0) { 
//		maxy=fontsize;
//		miny=maxy-height;
//	}

}

int CaptionData::NeedToRecache()
{
	if (needtorecache) return 1;
	for (int c=0; c<linestats.n; c++) 
		if (linestats.e[c]->needtorecache) return 1;
	return 0;
} 

/*! Return the number of bytes (not characters) in the given line number.
 */
int CaptionData::CharLen(int line)
{
	if (line<0 || line>=lines.n) return 0;
	return strlen(lines.e[line]);
}

/*! If line<0, then do all lines.
 * If needtorecache==false, then skip the line.
 */
int CaptionData::RecacheLine(int linei)
{
	//int start=0, end=lines.n-1;

	bool ntrc=needtorecache;
	if (linei<0 || linei>=lines.n) { 
		for (int c=0; c<lines.n; c++) {
			if (needtorecache) linestats.e[c]->needtorecache=true;
			else if (linestats.e[c]->needtorecache) ntrc=true;
		}
	} else {
		ntrc=true;
		linestats.e[linei]->needtorecache=true;
	}

	if (!ntrc) return 0;



	 //set up freetype and hb fonts
	FontManager *fontmanager = InterfaceManager::GetDefault()->GetFontManager();

	FT_Face ft_face;
	FT_Library *ft_library = fontmanager->GetFreetypeLibrary();

	FT_New_Face (*ft_library, font->FontFile(), 0, &ft_face);
	FT_Set_Char_Size (ft_face, font->Msize()*64, font->Msize()*64, 0, 0);

	hb_font_t *hb_font;
	hb_font = hb_ft_font_create (ft_face, NULL);


	 //figure out direction, language, and script
	hb_segment_properties_t seg_properties;


	 //Figure out direction, if any
	hb_direction_t dir = HB_DIRECTION_INVALID;
	if (direction==LAX_LRTB || direction==LAX_LRBT) dir=HB_DIRECTION_LTR;
	else if (direction==LAX_RLTB || direction==LAX_RLBT) dir=HB_DIRECTION_RTL;
	else if (direction==LAX_BTLR || direction==LAX_BTRL) dir=HB_DIRECTION_BTT;
	else if (direction==LAX_TBLR || direction==LAX_TBRL) dir=HB_DIRECTION_TTB;
	// else need to guess direction


	 //Figure out language, if any
	hb_language_t hblang = NULL;
	//const char *str=fontmanager->LanguageString(language);
	//if (language>=0 && str) hblang = hb_language_from_string(str, strlen(str));
	if (language) hblang = hb_language_from_string(language, strlen(language));
	//hblang = hb_language_get_default; 


	 //Figure out script, if any
	hb_script_t hbscript = HB_SCRIPT_UNKNOWN;
	//str=fontmanager->ScriptString(language);
	//if (script>=0 && *str) hbscript = hb_script_from_string(str, strlen(str));
	if (script) hbscript = hb_script_from_string(script, strlen(script));

//	const char *nonblank=NULL;
//	for (int c=0; c<lines.n; c++) {
//		if (!isblank(lines.e[c])) nonblank=lines.e[c];
//	}


	 //now figure out lines as necessary
	double width;
	for (int c=0; c<lines.n; c++) {
		if (!linestats.e[c]->needtorecache) continue;

		if (strlen(lines.e[c])==0) { //ok to have just a bunch of spaces
			linelengths.e[c] = 0;

			linestats.e[c]->pixlen = 0;
			linestats.e[c]->needtorecache = false;
			continue;
		}

		 //create buffer and add text
		hb_buffer_t *hb_buffer;
		hb_buffer = hb_buffer_create ();
		hb_buffer_add_utf8 (hb_buffer, lines.e[c], -1, 0, -1);


		 //set direction, language, and script
		if (dir==HB_DIRECTION_INVALID || hblang==NULL || hbscript==HB_SCRIPT_UNKNOWN) {
			 //at least one of these things we know, so we have to manually set again
			hb_buffer_guess_segment_properties (hb_buffer); //guesses direction, script, language 
			hb_buffer_get_segment_properties (hb_buffer, &seg_properties);

			if (dir != HB_DIRECTION_INVALID)   seg_properties.direction = dir;      else dir      = seg_properties.direction;
			if (hblang != NULL)                seg_properties.language  = hblang;   else hblang   = seg_properties.language;
			if (hbscript != HB_SCRIPT_UNKNOWN) seg_properties.script    = hbscript; else hbscript = seg_properties.script;

			hb_buffer_set_segment_properties (hb_buffer, &seg_properties);

		} else {
			 //we know them all so...
			seg_properties.direction = dir;
			seg_properties.language  = hblang;
			seg_properties.script    = hbscript;
			hb_buffer_set_segment_properties (hb_buffer, &seg_properties);
		}


		 //set features
		hb_face_t *hb_face;
		hb_face = hb_ft_face_create_referenced(ft_face); 

		//*** foreach (feature in feature_string) set for hb_font
		//hb_ot_layout_language_get_feature_tags()
		//

		unsigned int lang_index=0;
		unsigned int script_index=0;

#ifdef HARFBUZZ_BELOW_2_0
		hb_tag_t script_tag1, script_tag2;
		hb_ot_tags_from_script(hbscript, &script_tag1, &script_tag2);

		hb_tag_t lang_tag = hb_ot_tag_from_language (hblang);
		hb_ot_layout_script_find_language (hb_face, HB_OT_TAG_GSUB, script_index, lang_tag, &lang_index);

#else
		hb_tag_t lang_tag;
		hb_tag_t script_tag1;
		unsigned int n_script_tags = 1;
		unsigned int n_lang_tags = 1;

		hb_ot_tags_from_script_and_language(hbscript, hblang, &n_script_tags, &script_tag1, &n_lang_tags, &lang_tag);

		hb_ot_layout_table_find_script (hb_face, HB_OT_TAG_GSUB, script_tag1, &script_index);

		hb_ot_layout_script_select_language (hb_face, HB_OT_TAG_GSUB, script_index, 1, &lang_tag, &lang_index);
#endif

		//------getting list of otf features:
		// unsigned int count = 80;
		// hb_tag_t myResult[count];
		// //hb_ot_layout_table_get_feature_tags(hb_font_get_face(hb_font), HB_OT_TAG_GSUB, 0, &count, myResult)
		// //hb_ot_layout_table_get_feature_tags(hb_font_get_face(hb_font), HB_OT_TAG_GPOS, 0, &count, myResult)
		// //
		// //
		// //- hb_ot_layout_table_get_script_tags() to get all scripts for this font;
		// //- for each script index get all languages supported for this script with 
		// //hb_ot_layout_script_get_language_tags();
		// //- at this point it's two dimensional - (script index, language index), 
		// //now get feature set for this pair with:

		// hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GSUB, script_index, lang_index, 0, &count, myResult);
		// DBG cerr <<"for font :"<<font->FontFile()<<":"<<endl;
		// DBG cerr <<" All the GSUB features for cur lang+script:  ";
		// char tagstr[5]; tagstr[4]='\0';
		// for (int cc=0; cc<(int)count; cc++) {
		// 	hb_tag_to_string(myResult[cc], tagstr);
		// 	DBG cerr <<tagstr<<"  ";
		// }
		// DBG cerr << endl;
		// hb_ot_layout_language_get_feature_tags(hb_face, HB_OT_TAG_GPOS, script_index, lang_index, 0, &count, myResult);
		// DBG cerr <<" All the GPOS features for cur lang+script:  ";
		// for (int cc=0; cc<(int)count; cc++) {
		// 	hb_tag_to_string(myResult[cc], tagstr);
		// 	DBG cerr <<tagstr<<"  ";
		// }
		// DBG cerr << endl;


		  //Shape it!
		hb_shape (hb_font, hb_buffer, NULL, 0);


		 // get positioning
		 //
		//struct hb_glyph_info_t {
		//  hb_codepoint_t codepoint;
		//  hb_mask_t      mask;
		//  uint32_t       cluster;
		//}
		//
		//struct hb_glyph_position_t {
		//  hb_position_t  x_advance;
		//  hb_position_t  y_advance;
		//  hb_position_t  x_offset; //shift from current position based on advances
		//  hb_position_t  y_offset;
		//}
		//
		//hb_segment_properties_t {
		//  hb_direction_t  direction;
		//  hb_script_t     script;
		//  hb_language_t   language;
		//}
		//
		//cairo_glyph_t {
		//  unsigned long        index;
		//  double               x;
		//  double               y;
		//} 
		//

		unsigned int numglyphs   = hb_buffer_get_length (hb_buffer); //glyphs wide
		hb_glyph_info_t *info    = hb_buffer_get_glyph_infos (hb_buffer, NULL);     //points to inside hb_buffer
		hb_glyph_position_t *pos = hb_buffer_get_glyph_positions (hb_buffer, NULL); //points to inside hb_buffer

		 // update cache info
		if (linestats.e[c]->numglyphs < (int)numglyphs) {
			delete[] linestats.e[c]->glyphs;
			linestats.e[c]->glyphs = new GlyphPlace[numglyphs];
		}
		linestats.e[c]->numglyphs = numglyphs;

		GlyphPlace *glyphs = linestats.e[c]->glyphs;
		double current_x = 0;
		double current_y = 0;
		width=0; 
		double widthy=0;

		DBG cerr <<" computing line: "<<lines.e[c]<<endl;
		for (unsigned int i = 0; i < numglyphs; i++)
		{
			glyphs[i].index     = info[i].codepoint;
			glyphs[i].cluster   = info[i].cluster;
			glyphs[i].x_advance = pos[i].x_advance / 64.;
			glyphs[i].y_advance = pos[i].y_advance / 64.;
			glyphs[i].x         =   current_x + pos[i].x_offset / 64.;
			glyphs[i].y         = -(current_y + pos[i].y_offset / 64.);
			glyphs[i].numchars  = get_num_chars(lines.e[c], -1, glyphs[i].cluster, i==numglyphs-1 ? strlen(lines.e[c]) : glyphs[i+1].cluster);

			width     += pos[i].x_advance / 64.;
			widthy    += pos[i].y_advance / 64.;

			current_x += pos[i].x_advance / 64.;
			current_y += pos[i].y_advance / 64.;

			DBG fprintf(stderr, "index: %4d   cluster: %2u   at: %f, %f\n", glyphs[i].index, glyphs[i].cluster, glyphs[i].x, glyphs[i].y);
			//DBG cerr <<"index: "<<glyphs[i].index<<"  cluster: "<<glyphs[i].cluster<<"  at: "<<glyphs[i].x<<","<<glyphs[i].y<<endl;
		}

		DBG cerr <<endl;

		linelengths.e[c] = width;

		linestats.e[c]->pixlen = width;
		linestats.e[c]->needtorecache = false;
		

		hb_buffer_destroy (hb_buffer);

	} //foreach line


	 //cleanup
	hb_font_destroy (hb_font);
	FT_Done_Face (ft_face);
	needtorecache = 0;

	return 0;
}

//! Update cached pixel length of line. Also return that value.
/*! If line<0 then compute for all lines, and returns maximum length.
 */
double CaptionData::ComputeLineLen(int line)
{
	if (line>=0 && line<lines.n) linestats.e[line]->needtorecache=true;
	else needtorecache=true;
	RecacheLine(line);

	if (line>=0 && line<lines.n) return linestats.e[line]->pixlen;
	return 0;
	//-----------------
//	if (line<0) {
//		int max=0,w;
//		for (int c=0; c<lines.n; c++) {
//			w=ComputeLineLen(c);
//			if (w>max) max=w;
//		}
//		return w;
//	}
//
//	if (!font || line>=lines.n) return 0;
//	return font->Extent(lines.e[line],strlen(lines.e[line]));
}

/*! Find line and byte position within.
 * Returns 1 if found, else 0 for out of bounds.
 */
int CaptionData::FindPos(double y, double x, int *line, int *pos)
{
	int l = (linespacing!=0 ? (y-miny)/fabs(fontsize*linespacing) : 0); 
	if (l<0 || l>=lines.n) return 0;
	if (linespacing<0) l=lines.n-1-l;

	x+=xcentering/100*(linelengths[l]);
	
	unsigned int p=0;

//	------------------
	int len = linestats.e[l]->numglyphs;
	int g=0;
	double current_x=0;
	//double current_y=y;

	GlyphPlace *glyph;
	while (g<len) {
		glyph = &linestats.e[l]->glyphs[g];

		if (glyph->numchars>1 && x<current_x+glyph->x_advance) {
			p=glyph->cluster; //the utf8 byte index

			double tick = glyph->x_advance/glyph->numchars;

			if (x<current_x+tick/2) break;

			if (x<current_x+glyph->x_advance - tick/2) {
				x -= current_x+tick/2;
				int n=1+x/tick;
				while (n) {
					p=utf8fwd_index(lines.e[l],p+1,strlen(lines.e[l]));
					n--;
				}
				break;
			}
		}

		if (x<current_x+glyph->x_advance/2) {

			// *** works for ltr only !!!
			p=glyph->cluster; //the utf8 byte index
			// *** watch for when glyph spans many clusters
			break;
		}

		current_x += glyph->x_advance;
		//current_y += glyph->y_advance;

		g++;
	}
	if (g==len) p=strlen(lines.e[l]);

//	------------------
//	double lastw=0;
//	double w;
//	const char *pp;
//	while (p<strlen(lines.e[l])) {
//		p++;
//		pp=utf8fwd(lines.e[l]+p, lines.e[l], lines.e[l]+strlen(lines.e[l]));
//		p=pp-lines.e[l];
//
//		w=font->Extent(lines.e[l], p);
//		if (x<(lastw+w)/2) { p=utf8back_index(lines.e[l],p-1,strlen(lines.e[l])); break; }
//		lastw=w;
//	}
//	------------------

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
 *  direction   lrtb
 *  text \
 *    Blah Blah.
 *    Blah, "blah blah"
 * </pre>
 *
 */
void CaptionData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%smatrix 1 0 0 1 0 0   #transform of the whole object\n",spc);
		fprintf(f,"%sfont                 #Font to use\n",spc);
		fprintf(f,"%sfontsize 12          #hopefully this is point size\n",spc);
		fprintf(f,"%slinespacing 1        #percentage different from font's default spacing\n",spc);
		fprintf(f,"%sdirection lrtb       #lrtb, lrbt, rltb, rlbt, tblr, tbrl, btlr, btrl, or guess\n",spc);
		fprintf(f,"%sxcentering 50        #0 is left, 50 is center, 100 is right, or any other number\n",spc);
		fprintf(f,"%sycentering 50        #0 is top, 50 is center, 100 is bottom, or any other number\n",spc);
		fprintf(f,"%scolor rgbaf(1,0,0,1)\n", spc);
		fprintf(f,"%stext \\  #The actual text\n%s  blah",spc,spc);
		return;
	}


	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				m(0),m(1),m(2),m(3),m(4),m(5));

	fprintf(f,"%sfont\n",spc);
	Attribute att;
	font->dump_out_atts(&att, what, context);
	att.dump_out(f, indent+2);

	fprintf(f,"%sfontsize %.10g\n",spc,fontsize);
	fprintf(f,"%slinespacing %.10g\n",spc,linespacing);

	const char *dir=flow_name(direction);
	if (!dir) dir="guess";
	fprintf(f,"%sdirection %s\n",spc, dir);
	if (language) fprintf(f,"%slanguage %s\n",spc, language);
	if (script)   fprintf(f,"%sscript %s\n",  spc, script);

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

Laxkit::Attribute *CaptionData::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("matrix", "1 0 0 1 0 0", "An affine matrix of 6 numbers");
		att->push("font", "...",           "Font to use");
		att->push("fontsize", "12",        "hopefully this is point size");
		att->push("linespacing", "1",      "percentage different from font's default spacing");
		att->push("direction", "lrtb",     "lrtb, lrbt, rltb, rlbt, tblr, tbrl, btlr, btrl, or guess");
		att->push("xcentering", "50",      "0 is left, 50 is center, 100 is right, or any other number");
		att->push("ycentering", "50",      "0 is top, 50 is center, 100 is bottom, or any other number");
		att->push("color", "rgbaf(1,0,0,1)");
		att->push("text", "The actual text\n%s  blah");
		return att;
	}

	char scratch[200];
	sprintf(scratch, "%.10g %.10g %.10g %.10g %.10g %.10g", m(0),m(1),m(2),m(3),m(4),m(5));
	att->push("matrix", scratch);

	
	if (font->ResourceOwner() != this) {
		att->pushStr("font", -1, "resource: %s", font->Id());
	} else {
		Attribute *att2=att->pushSubAtt("font");
		font->dump_out_atts(att2, what, context);
	}

	att->push("fontsize",fontsize);
	att->push("linespacing",linespacing);

	const char *dir=flow_name(direction);
	if (!dir) dir="guess";
	att->push("direction", dir);
	if (language) att->push("language", language);
	if (script)   att->push("script",   script);

	att->push("xcentering", xcentering);
	att->push("ycentering", ycentering);

	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)",
						red,green,blue,alpha);
	att->push("color", scratch);

	if (lines.n) {
		char *txt=GetText();
		att->push("text", txt);
		delete[] txt;
	}

	return att;
}

//! See dump_out().
void CaptionData::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
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

		} else if (!strcmp(name,"direction")) {
			direction=flow_id(value);
			if (direction<0) direction=-1; //for guess

		} else if (!strcmp(name,"language")) {
			makestr(language, value);

		} else if (!strcmp(name,"script")) {
			makestr(script, value);

		} else if (!strcmp(name,"xcentering")) {
			DoubleAttribute(value,&xcentering);

		} else if (!strcmp(name,"ycentering")) {
			DoubleAttribute(value,&ycentering);

		} else if (!strcmp(name,"text")) {
			SetText(value);

		} else if (!strcmp(name,"font")) {
			if (!strncmp_safe(value, "resource:", 9)) {
				value += 9;
				while (isspace(*value)) value++;
				InterfaceManager *imanager=InterfaceManager::GetDefault(true);
				ResourceManager *rm=imanager->GetResourceManager();
				LaxFont *nfont = dynamic_cast<LaxFont*>(rm->FindResource(value,"Font"));
				if (nfont) {
					if (newfont) newfont->dec_count();
					newfont = nfont;
				} else {
					DBG cerr << "Missing font resource! "<<value<<endl;
				}

			} else if (att->attributes.e[c]->attributes.n) {
				FontManager *fontmanager = InterfaceManager::GetDefault()->GetFontManager();
				if (newfont) newfont->dec_count();
				newfont=fontmanager->dump_in_font(att->attributes.e[c], context);
			}

		} else if (!strcmp(name,"fontfile")) {
			file=value;

		} else if (!strcmp(name,"fontfamily")) {
			family=value;

		} else if (!strcmp(name,"fontstyle")) {
			style=value;

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

	needtorecache=true;
}

/*! Adjust ycentering to align with the given line's baseline.
 *
 * Returns the new ycentering value.
 */
double CaptionData::BaselineJustify(int line)
{
	if (line<0 || line>=lines.n) line=lines.n-1;
	double height = maxy-miny;
	ycentering=100*(line*fontsize*linespacing + font->ascent())/height;
	baseline_hint=line;
	FindBBox();
	return ycentering;
}

/*! Value is percentage (1 == 100%) off from font's line spacing.
 */
double CaptionData::LineSpacing(double newspacing)
{
	if (newspacing!=0) linespacing=newspacing;
	FindBBox();
	touchContents();
	return linespacing;
}

double CaptionData::Size(double newsize)
{ 
	if (newsize<=0) newsize=1e-4;
	font->Resize(newsize);
	fontsize=newsize;
	state=0;
	needtorecache=true;
	FindBBox();
	touchContents();
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

bool CaptionData::IsBlank()
{
	for (int c=0; c<lines.n; c++) {
		if (!isblank(lines.e[c])) return false;
	}
	return true;
}

void CaptionData::ColorRGB(double r, double g, double b, double a)
{
	red = r;
	green = g;
	blue = b;
	if (a >= 0) alpha = a;
}

/*! Convert backslashed escapes to normal string.
 * This means "\\n" is newline, "\\t" tab, "\\U002F" and "\\u002f" are the
 * corresponding unicode character, "\\(any other char)" is (any other char).
 */
int CaptionData::SetTextEscaped(const char *newtext)
{
	const char *ptr = newtext ? strchr(newtext, '\\') : nullptr;
	if (!ptr) return SetText(newtext);

	int slen = strlen(newtext);
	int maxstr = slen;
	int spos = 0;
	char *newstr = new char[maxstr];
	char ch;
	newstr[0] = '\0';
	int from = 0;
	char utf8[10];

	 // read in the string, translating escaped things as we go
	while (from < slen) {
		ch = newtext[from];

		 // escape sequences
		if (ch == '\\') {
			ch = 0;
			if (newtext[from+1]=='n') ch='\n';
			else if (newtext[from+1]=='t') ch='\t';
			else if ((newtext[from+1]=='u' || newtext[from+1]=='U')
				 && isxdigit(newtext[from+2])
				 && isxdigit(newtext[from+3])
				 && isxdigit(newtext[from+4])
				 && isxdigit(newtext[from+5])) {
				//unicode character
				char *endptr = nullptr;
				unsigned int i = strtol(newtext+from+2, &endptr, 16);
				if (endptr != newtext+from+2) {
					//was valid number
					from = endptr - newtext;
					if (i >= 32 || i == 10 || i == 9) { //don't allow non-character ascii other than nl, tab
						int len = utf8encode(i, utf8);
						if (len + spos > maxstr) extendstr(newstr, maxstr, 20+len);
						strncpy(newstr+spos, utf8, len);
						spos += len;
					}
					continue;
				}
			}
			if (ch) from++; else ch='\\';
		}
		if (spos == maxstr) extendstr(newstr,maxstr,20);
		newstr[spos] = ch;
		spos++;
		from++;
	}
	newstr[spos] = '\0';

	int status = SetText(newstr);
	delete[] newstr;
	return status;
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
	linestats.flush();

	int numlines=0;
	char **text=split(newtext,'\n',&numlines);
	for (int c=0; c<numlines; c++) {
		lines.push(text[c]);
		linelengths.push(0);
		while (c>=linestats.n) linestats.push(new Linestat(0,0,0,0,0));
		//if (c>=linestats.Allocated()) linestats.push(new Linestat(0,0,0,0,0));
	}

	needtorecache=true;
	state=0;
	FindBBox();
	touchContents();
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
			linestats.remove(line+1);
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
			linestats.remove(line);
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
	needtorecache=true;
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
		linestats.push(new Linestat(0,0,0,0,0),1,line+1);

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

	needtorecache=true;
	state=0;
	FindBBox();

	return 0;
}

int CaptionData::DeleteSelection(int fline,int fpos, int tline,int tpos, int *newline,int *newpos)
{
	if (fline<0 || fline>=lines.n) fline=lines.n-1;
	if (tline<0 || tline>=lines.n) tline=lines.n-1;
	if (fpos<0 || fpos>=(int)strlen(lines.e[fline])) fpos=strlen(lines.e[fline]);
	if (tpos<0 || tpos>=(int)strlen(lines.e[tline])) tpos=strlen(lines.e[tline]);
	if (fline==tline && fpos==tpos) { *newline=fline; *newpos=fpos; return 0; }

	 //make from be before to
	if (tline<fline) {
		int tt=tline; tline=fline; fline=tt;
		tt=tpos; tpos=fpos; fpos=tt;
	}
	if (tline==fline && tpos<fpos) { int tt=tpos; tpos=fpos; fpos=tt; }

	 //remove any whole lines
	while (tline>fline+1) {
		lines.remove(tline-1);
		linelengths.remove(tline-1);
		linestats.remove(tline-1);
		tline--;
	}

	if (tline==fline) {
		 //remove text all within one line
		memmove(lines.e[fline]+fpos, lines.e[fline]+tpos, strlen(lines.e[fline])-tpos+1);

	} else {
		 //remove end of fline, and append tline+tpos to fline
		lines.e[fline][fpos]='\0';
		appendstr(lines.e[fline], lines.e[tline]+tpos);
		lines.remove(tline);
		linelengths.remove(tline);
		linestats.remove(tline);
	}

	ComputeLineLen(fline);
	needtorecache=true;
	FindBBox();
	*newline=fline;
	*newpos=fpos;
	return 0;
}

int CaptionData::InsertString(const char *txt,int len, int line,int pos, int *newline,int *newpos)
{
	if (line<0 || line>=lines.n) line=lines.n-1;
	if (pos<0 || pos>=(int)strlen(lines.e[line])) pos=strlen(lines.e[line]);
	if (len<0) len=strlen(txt);
	if (!txt || !len) { *newline=line; *newpos=pos; return 0; }
	
	insertnstr(lines.e[line], txt,len, pos);

	char *curline =lines.e[line];
	char *origline=lines.e[line];
	lines.e[line]=NULL;
	int first=1;

	while (*curline) {
		char *nl = lax_strchrnul(curline, '\n');
		if (*nl=='\0' && first) {
			lines.e[line]=origline;
			origline=NULL;
			break; //line is good as is
		}

		 //else we need to add it
		char *thisline=newnstr(curline, nl-curline);
		if (first) {
			lines.e[line]=thisline;
			first=0;
		} else {
			lines.push(thisline, line);
			linelengths.push(0, line);
			linestats.push(new Linestat(0,0,0,0,0),1,line);
		}
		ComputeLineLen(line);

		line++;
		curline=nl;
		if (*curline) curline++;
	}
	delete[] origline;

	needtorecache=true;
	FindBBox();

	*newline=line;
	*newpos =pos;
	return 0;
}

//! Set horizontal centering, and adjust bbox.
double CaptionData::XCenter(double xcenter)
{
	xcentering=xcenter;
	double w(maxx-minx);
	minx=-xcentering/100*w;
	maxx=minx+w;
	touchContents();

	return xcentering;
}

//! Set vertical centering, and adjust bbox.
double CaptionData::YCenter(double ycenter)
{
	ycentering=ycenter;
	double h(maxy-miny);
	miny=-ycentering/100*h;
	maxy=miny+h;
	touchContents();

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
	//linespacing=1;
	makestr(fontfamily,font->Family());
	makestr(fontstyle, font->Style());
	makestr(fontfile,  font->FontFile());

	DBG cerr <<"---- caption: linked new font size a,d,h, fs: "<<font->ascent()<<", "<<font->descent()<<", "<<font->textheight()<<"   "<<fontsize<<endl;

	needtorecache=true;
	state=0;
	FindBBox();
	touchContents();

	return 0;
}

int CaptionData::Font(const char *file, const char *family,const char *style,double size)
{
	if (font) { font->dec_count(); font=NULL; }

	if (file)  font=InterfaceManager::GetDefault()->GetFontManager()->MakeFontFromFile(file, family,style,size,-1);
	if (!font) font=InterfaceManager::GetDefault()->GetFontManager()->MakeFont(family,style,size,-1);
	if (!font) font=InterfaceManager::GetDefault()->GetFontManager()->MakeFont("sans",NULL,size,-1);

	fontsize=size;
	//linespacing=1;

	makestr(fontfile,  file   ? file   : font->FontFile());
	makestr(fontfamily,family ? family : font->Family());
	makestr(fontstyle, style  ? style  : font->Style());

	DBG cerr <<"------ caption: new font size a,d,h, fs: "<<font->ascent()<<", "<<font->descent()<<", "<<font->textheight()<<"   "<<fontsize<<endl;

	needtorecache=true;
	state=0;
	FindBBox();
	touchContents();

	return 0;
}


/*! If !use_clones, then return a single PathsData object with each glyph to independent paths.
 * 
 * If use_clones, then return a group of two objects.
 * The first object is a Group of SomeDataRef objects, and the second is another Group object
 * containing the glyphs. The SomeDataRef objects all point to the paths in the second object.
 *
 * NULL will be returned if there is no text to render.
 */
SomeData *CaptionData::ConvertToPaths(bool use_clones, RefPtrStack<SomeData> *clones_to_add_to)
{
	if (!font || (lines.n==1 && strlen(lines.e[0])==0)) return NULL;

	RecacheLine(-1);


	FT_Error ft_error;
	FT_Face ft_faces[font->Layers()];
	FontManager *fontmanager = GetDefaultFontManager();
	FT_Library *ft_library = fontmanager->GetFreetypeLibrary();

	LaxFont *ff;
	for (int c=0; c<font->Layers(); c++) {
		ff=font->Layer(c);
		FT_New_Face (*ft_library, ff->FontFile(), 0, &ft_faces[c]);
		FT_Set_Char_Size (ft_faces[c], font->Msize()*64, font->Msize()*64, 0, 0);
	}


	InterfaceManager *imanager = InterfaceManager::GetDefault();

	RefPtrStack<SomeData> glyphs[font->Layers()];

	char glyphname[100];
	GlyphPlace *glyph;
	PathsData *outline;
	
	RefPtrStack<PathsData> layers; //temp object for single color layers
	PathsData *pobject=NULL;

	RefPtrStack<SomeData> *object=NULL;

	ScreenColor color(red,green,blue,alpha);

	//bool is_all_paths=true;
	//if (use_clones) is_all_paths=false;

	//bool is_single_color=true;
	Palette *palette=dynamic_cast<Palette*>(font->GetColor());
	//if (font->Layers()>1) is_single_color=false;


	 //for layered fonts, need to extract glyphs from each font, then stack 'em
	for (int l=0; l<lines.n; l++) {
	  flatpoint loffset;
	  loffset.x = -xcentering/100*(linelengths.e[l]);
	  loffset.y = miny + font->ascent() + l*fabs(fontsize*linespacing);

	   //foreach glyph...
	  for (int g=0; g<linestats.e[l]->numglyphs; g++) {
		glyph = &linestats.e[l]->glyphs[g];

		 //assign a glyph name.
		 //use ONLY face[0] for name in layered fonts. All parts of the glyph ultimately get collapsed to single object.
		FT_Get_Glyph_Name(ft_faces[0], glyph->index, glyphname, 100);
		if (glyphname[0]=='\0') sprintf(glyphname,"glyph%d",glyph->index);

		for (int layer=0; layer<font->Layers(); layer++) {
		  outline=NULL;
		  for (int o=0; o<glyphs[layer].n; o++) {
			if (!strcmp(glyphs[layer].e[o]->Id(), glyphname)) { 
				outline=dynamic_cast<PathsData*>(glyphs[layer].e[o]);
				break;
			}
		  }

		  if (!outline) {
			 //make glyph
			//ft_error = FT_Load_Glyph(ft_faces[layer], glyph->index, FT_LOAD_NO_SCALE);
			ft_error = FT_Load_Glyph(ft_faces[layer], glyph->index, FT_LOAD_NO_BITMAP);
			if (ft_error != 0) continue;

			 //so maybe the glyph is a bitmap, maybe it is an svg
			 //we need to be on watch so that if glyph is COLR-able, we need to get the
			 //color glyphs, NOT the fallback ones!

			if (ft_faces[layer]->glyph->format != FT_GLYPH_FORMAT_OUTLINE) {
				 //not the simple case where outline is right there!!
				cerr << " *** Need to implement something meaningful for non-outline glyph to path! "<<endl;

			} else {
				 //we're in luck! just an ordinary path in one color to parse...

				if (palette) {
					color.rgbf(
						palette->colors.e[layer]->color->values[0],
						palette->colors.e[layer]->color->values[1],
						palette->colors.e[layer]->color->values[2],
						palette->colors.e[layer]->color->values[3]);
				}

				 //has to be first layer, so make new base object
				PathsData *glypho = dynamic_cast<PathsData*>(imanager->NewDataObject("PathsData"));
				glypho->Id(glyphname);

				glypho->fill(&color);
				glypho->line(0);
				outline=glypho;

				 //set up parsing functions for Path object
				int pathn = outline->paths.n;
				FT_Outline_Funcs outline_funcs;
				outline_funcs.move_to  = pathsdata_ft_move_to;
				outline_funcs.line_to  = pathsdata_ft_line_to;
				outline_funcs.conic_to = pathsdata_ft_conic_to;
				outline_funcs.cubic_to = pathsdata_ft_cubic_to;
				outline_funcs.shift    = 0;
				outline_funcs.delta    = 0;
				 // parse!!
				ft_error = FT_Outline_Decompose( &ft_faces[layer]->glyph->outline, &outline_funcs, outline ); 
				outline->close();

				for (int p=pathn; p<outline->paths.n; p++) {
					Coordinate *coord = outline->paths.e[p]->path;
					Coordinate *start = coord;

					 //remove double points
					do {
						if (coord->prev && coord->prev!=start
								&& coord->flags&POINT_VERTEX && coord->prev->flags&POINT_VERTEX
								&& coord->fp==coord->prev->fp) {
							Coordinate *d = coord->prev;
							d->detach();
							delete d;
						}
					} while (coord && coord!=start);

				}

			} //if ft glyph format is outline

			if (outline) {
				glyphs[layer].push(outline);
				outline->dec_count();
			}

		  } //if outline didn't exist
		  
		  if (use_clones) {
			  //*** //need to build up the glyph image
			      //then apply a ref to the image outside of the layers loop below
			cerr << " *** implement use_clones in convert text to paths!!"<<endl;

		  } else {
			   //add duplicate of glyph to existing overall object

			  if (layer>=layers.n) {
				  PathsData *nlayer = dynamic_cast<PathsData*>(imanager->NewDataObject("PathsData"));
				  layers.push(nlayer);
				  nlayer->dec_count();

				  pobject=layers.e[layer];
				  ScreenColor color;
				  if (palette) {
				  	color.rgbf(
				  		palette->colors.e[layer]->color->values[0],
				  		palette->colors.e[layer]->color->values[1],
				  		palette->colors.e[layer]->color->values[2],
				  		palette->colors.e[layer]->color->values[3]);
				  } else color.rgbf(red, green, blue, alpha);
				  pobject->fill(&color);
				  pobject->line(0);
				  pobject->linestyle->function=LAXOP_None;

			  } else pobject = layers.e[layer];

			   //provide proper offset
			  PathsData *newchar = dynamic_cast<PathsData*>(outline->duplicate(NULL));

			  for (int p=0; p<newchar->paths.n; p++) {
				  Coordinate *coord = newchar->paths.e[p]->path;
				  Coordinate *start = coord;

					start=coord;
					do {
						coord->fp += flatpoint(glyph->x+loffset.x, glyph->y+loffset.y);
						coord = coord->next;
					} while (coord && coord!=start);
			  }

			   //transfer all paths from newchar to pobject
			  newchar->FindBBox();
			  //newchar->ApplyTransform();
			  for ( ; newchar->paths.n>0; ) {
				  Path *path = newchar->paths.pop(0);
				  pobject->paths.push(path);
			  }

			  newchar->dec_count();
		  }

		} //foreach layer

		if (!outline) continue;

		if (use_clones) {
		   // add SomeDataRef to existing group object
		  //SomeDataRef *ref=new SomeDataRef(outline); // *** need to use generic object generator
		  SomeDataRef *ref = dynamic_cast<SomeDataRef*>(imanager->NewDataObject("SomeDataRef"));
		  if (!ref) ref=new SomeDataRef;
		  ref->Set(outline,false);

		  //ref->Scale(***);
		  ref->origin(flatpoint(glyph->x+loffset.x, glyph->y+loffset.y));

		  if (!object) object=new RefPtrStack<SomeData>();
		  object->push(ref);
		  ref->dec_count();
		  //is_all_paths=false;

		}
	  } //for each glyph
	} //for each line


	 //cleanup
	//if (glyphs != clones_to_add_to) glyphs->dec_count();
	for (int c=0; c<font->Layers(); c++) {
		FT_Done_Face (ft_faces[c]);
	}

	//if (is_all_paths) *** CollapsePaths(object);

	if (layers.n) {
		for (int l=0; l<layers.n; l++) {
			pobject = layers.e[l];
			pobject->m(m());
			pobject->FindBBox();
		}

		if (layers.n==1) {
			layers.e[0]->inc_count();
			return layers.e[0];
		}

		DBG cerr <<"Converted to path."<<endl;
		//DBG dump_out(stderr,2,0,NULL);

		GroupData *group = dynamic_cast<GroupData*>(imanager->NewDataObject("Group"));
		for (int l=0; l<layers.n; l++) {
			group->push(layers.e[l]);
		}
		group->FindBBox();

		return group;
	}

	if (object) {
		DBG cerr << "*** Warning!!! must implement GroupData for CaptionData::ConvertToPaths()!!!"<<endl;
	}
	return NULL;
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
	data          = NULL;
	coc           = NULL;
	extrahover    = NULL;
	showdecs      = 1;
	showbaselines = 0;
	showobj       = 1;
	mode          = 0;
	lasthover     = CAPTION_None;

	defaultsize    = 24;
	defaultspacing = 1;
	defaultscale   = -1;
	defaultfamily  = newstr("sans");
	defaultstyle   = newstr("");

	grabpad    = 20;
	caretline  = 0;  // line number, starting from 0
	caretpos   = 0;  // position of caret in caretline
	sellen     = 0;
	scaretline = scaretpos = 0;
	needtodraw = 1;

	baseline_color = rgbcolorf(1.,0.,1.);

//	if (newtext) {
//		if (!data) data=new CaptionData(newtext,
//						 "sans", //font name   //"/usr/X11R6/lib/X11/fonts/TTF/temp/hrtimes_.ttf",
//						 NULL, //font style
//						 36, //font size
//						 0,  //xcenter,
//						 0); //ycenter
//	}

	sc = NULL;
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
{ return _("Caption"); }

//! Sets showdecs=1, and needtodraw=1.
int CaptionInterface::InterfaceOn()
{
	DBG cerr <<"CaptionInterfaceOn()"<<endl;
	showdecs=1;
	needtodraw=1;
	mode=0;


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
			viewport->ChangeObject(coc, false, true);
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
	if (!menu) menu=new MenuInfo();

	//menu->AddItem(_("Lorem ipsum..."));
	//menu->AddItem(_("Show all controls"));
	
	if (data) {
		menu->AddItem(_("Insert character..."), CAPTION_InsertChar);
		menu->AddItem(_("Select font..."), CAPTION_Font_Dialog);
		menu->AddSep();
		menu->AddItem(_("Create path object"), CAPTION_Create_Path_Object);
		//menu->AddItem(_("Convert to path"), CAPTION_Convert_To_Path);
		//menu->AddItem(_("Convert to path clones"));
		//menu->AddItem(_(""));
	}

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

/*! Draw one line of text, and also draw out the caret and selection highlight.
 */
void CaptionInterface::TextOutGlyphs(int line, double x,double y, bool show_caret)
{
	unsigned int len=data->linestats.e[line]->numglyphs;

	double current_x = x;
	//double current_y = y;
	double current_y = y+data->font->ascent(); 
	GlyphPlace *glyph;
	GlyphPlace *nextglyph;

	dp->glyphsout(current_x,current_y, data->linestats.e[line]->glyphs, NULL, len, LAX_BASELINE|LAX_LEFT); 

	if (!show_caret) return;
	

	double caretx=x;

	for (unsigned int i = 0; i < len; i++)
	{
		glyph = &data->linestats.e[line]->glyphs[i];
		if (i<len-1) nextglyph = &data->linestats.e[line]->glyphs[i+1];
		else nextglyph = NULL;

		// *** ASSUMES CLUSTER VALUES MONOTONICALLY INCREASE
		//
		// Sometimes one cluster produces many glyphs.
		// Sometimes many clusters produce a single glyph.

		if (caretpos == (long)glyph->cluster) {
			caretx=current_x;
			break;

		} else if (nextglyph && caretpos>(long)glyph->cluster && caretpos<(long)nextglyph->cluster) {
			 //current glyph spans many clusters, interpolate
			caretx = current_x + glyph->x_advance*char_distance(caretpos, data->lines.e[line], -1, glyph->cluster, data->linestats.e[line]->glyphs[i+1].cluster);
			break;

		} else if (!nextglyph && caretpos>(long)glyph->cluster) {
			 //final glyph spans many clusters
			caretx = current_x + glyph->x_advance*char_distance(caretpos, data->lines.e[line], -1, glyph->cluster, strlen(data->lines.e[line]));
			break; 

		} else if (i==len-1) return; //caret place not found!

		//DBG PostMessage(data->lines.e[line]+caretpos);

		current_x += glyph->x_advance;
		current_y += glyph->y_advance;
	} 

	 //draw caret
	dp->NewFG(0.0, .5, 0.0, 1.0);
	//double ex=dp->textextent(data->font, data->lines.e[line],caretpos, NULL,NULL,NULL,NULL,0);
	double tick=data->fontsize/10;
	dp->drawline(caretx,y, caretx,y+data->fontsize);
	dp->drawline(caretx,y, caretx-tick,y-tick);
	dp->drawline(caretx,y, caretx+tick,y-tick);
	dp->drawline(caretx,y+data->fontsize, caretx-tick,y+data->fontsize+tick);
	dp->drawline(caretx,y+data->fontsize, caretx+tick,y+data->fontsize+tick);
	dp->NewFG(data->red, data->green, data->blue, data->alpha);

}

/*! Returns 1 if no data, -1 if thing was offscreen, or 0 if thing drawn.
 */
int CaptionInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	needtodraw=0;

	if (extrahover) {
		 //draw ouline of another caption
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

	if (!data) return 1;
	
	dp->font(data->font);
	if (data->NeedToRecache()) {
		 //need to find line lengths
		data->FindBBox();
	}

	 //find how large
	flatpoint pb =flatpoint(0, data->miny),
			  pt =flatpoint(0, data->miny+fabs(data->fontsize)),
			  ptt=flatpoint(0, data->maxy);
	int height=(int)norm(dp->realtoscreen(pb)-dp->realtoscreen(pt));
	double totalheight=norm(dp->realtoscreen(pb)-dp->realtoscreen(ptt));
	if (totalheight<.5) return 0;



	 // find the screen box to draw into
	 // these still need the 'dp->' because dp was transformed to space immediately holding data already
	 // The viewport is the base transform, usually different then dp coords...
	flatpoint ul=dp->realtoscreen(flatpoint(data->minx,data->miny)), 
			  ur=dp->realtoscreen(flatpoint(data->maxx,data->miny)), 
			  ll=dp->realtoscreen(flatpoint(data->minx,data->maxy)), 
			  lr=dp->realtoscreen(flatpoint(data->maxx,data->maxy));
	flatpoint v=ll-ul;
	v=v/norm(v);

	// DBG fprintf(stderr,"draw caption scr coords: %ld: ul:%g,%g ur:%g,%g ll:%g,%g lr:%g,%g\n",
	// DBG		data->object_id,ul.x,ul.y,ur.x,ur.y,ll.x,ll.y,lr.x,lr.y);
	// DBG fprintf(stderr,"     caption bounds:    w:%g  h:%g\n",
	// DBG		norm(ul-ur),norm(ul-ll));
	
	 // check for positive intersection of transformed image to dp view area
	DoubleBBox bbox(ul);
	bbox.addtobounds(ur);
	bbox.addtobounds(ll);
	bbox.addtobounds(lr);
	if (!bbox.intersect(dp->Minx,dp->Maxx,dp->Miny,dp->Maxy)) {
		DBG cerr <<"----------------CaptionData outside viewport"<<endl;
		//if (!coc) dp->PopAxes();
		return -1;
	}

	
	 // draw control points;
	if (showdecs) { 
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
		double thin = ScreenLine();
		double xs=grabpad*thin/dp->Getmag()/2;
		double ys=grabpad*thin/dp->Getmag(1)/2;

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
		dp->moveto(data->maxx+xs+xs/2, data->maxy - (data->lines.n==1 ? 0 : (data->maxy-data->miny)*LSSIZE));
		dp->lineto(data->maxx+xs, data->miny);
		dp->lineto(data->maxx+xs+xs, data->miny);
		dp->closed();
		if (lasthover==CAPTION_Size) dp->fill(0); else dp->stroke(0);

		 //draw line spacing handle
		if (data->lines.n>1) {
			dp->NewFG(.5,.5,.5,.5);
			dp->moveto(data->maxx+xs+xs/2, data->maxy);
			dp->lineto(data->maxx+xs,    data->miny + (data->maxy-data->miny)*(1-LSSIZE));
			dp->lineto(data->maxx+xs+xs, data->miny + (data->maxy-data->miny)*(1-LSSIZE));
			dp->closed();
			if (lasthover==CAPTION_Line_Spacing) dp->fill(0); else dp->stroke(0);
		}


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
		if (height<1) {
			texttoosmall=1;
		}

		 //draw the text
		if (!texttoosmall) {
			dp->font(data->font);

			 //draw the stuff
			double x,y;
			double baseline=data->font->ascent(), descent=data->font->descent();
			//y = -data->ycentering/100*data->fontsize*data->linespacing*data->lines.n;
			y = data->miny;

			for (int c=(data->LineSpacing()<0 ? data->lines.n-1 : 0);
					 (data->LineSpacing()<0 ? c>=0 : c<data->lines.n);
					 (data->LineSpacing()<0 ? c-- : c++),
					 y+=fabs(data->fontsize*data->linespacing)) {

				x=-data->xcentering/100*(data->linelengths.e[c]);

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
					TextOutGlyphs(c, x,y, c==caretline && showdecs);
					//dp->textout(x,y, data->lines.e[c],-1, LAX_TOP|LAX_LEFT);
				}

				// //draw caret
				//if (c==caretline && showdecs) {
				//	dp->NewFG(0.0, .5, 0.0, 1.0);
				//	double ex=dp->textextent(data->font, data->lines.e[c],caretpos, NULL,NULL,NULL,NULL,0);
				//	double tick=data->fontsize/10;
				//	dp->drawline(x+ex,y, x+ex,y+data->fontsize);
				//	dp->drawline(x+ex,y, x+ex-tick,y-tick);
				//	dp->drawline(x+ex,y, x+ex+tick,y-tick);
				//	dp->drawline(x+ex,y+data->fontsize, x+ex-tick,y+data->fontsize+tick);
				//	dp->drawline(x+ex,y+data->fontsize, x+ex+tick,y+data->fontsize+tick);
				//	dp->NewFG(data->red, data->green, data->blue, data->alpha);
				//}
			}

			//dp->font(app->defaultlaxfont);

		} else {
			//*** text is too small, draw little lines
			DBG cerr <<"small text, implement draw little gray lines instead..."<<endl;
		}
	}
	

	//DBG cerr<<"..Done drawing CaptionInterface"<<endl;
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
        DBG cerr <<"Caption new color stuff"<< endl;
        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(newdata);

        if (nlinestyle->mask & (LINESTYLE_Color | LINESTYLE_Color2)) {
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

	defaultsize = data->fontsize;
	defaultspacing = data->LineSpacing();
	makestr(defaultfamily,data->fontfamily);
	makestr(defaultstyle, data->fontstyle);
	defaultscale = data->xaxis().norm();

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
		app->rundialog(new FontDialog(NULL, "Font",_("Font"),ANXWIN_REMEMBER, 10,10,700,700,0, object_id,"newfont",0,
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
		
		if (viewport) viewport->ChangeObject(oc,0,true);
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
		if (!primary && c==-1 && viewport->ChangeObject(oc,1,true)) {
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

int CaptionInterface::Paste(const char *txt,int len, Laxkit::anObject *obj, const char *formathint)
{
	DBG if (txt) cerr <<"    pasting into captioninterface: "<<txt<<endl;

	if (!txt || !len) return 1;

	if (data) {
		data->InsertString(txt, len, caretline, caretpos, &caretline, &caretpos);
		PostMessage(_("Pasted."));
		needtodraw=1;
		return 0;

	} else {
		 //need to create and insert a new text object
		if (len<0) len=strlen(txt);
		char text[len+1];
		strncpy(text,txt,len);
		text[len]='\0';

		data=newData();
		data->SetText(text);

		int mx,my;
		mouseposition(0, curwindow, &mx,&my, NULL, NULL, NULL);
		flatpoint leftp = screentoreal(mx,my);
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

		PostMessage(_("Pasted."));
		needtodraw=1;
	}

	return 1;
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
		data->FindBBox();
	
		DBG cerr <<"------------ new font size a,d,h, fs: "<<data->font->ascent()<<", "<<data->font->descent()<<", "<<data->font->textheight()<<"   "<<data->fontsize<<endl;

		defaultsize=size;
		makestr(defaultfamily,data->fontfamily);
		makestr(defaultstyle, data->fontstyle);
		needtodraw=1;

		return 0;

	} else if (!strcmp(mes, "linespacing")) {
		const StrEventData *s=dynamic_cast<const StrEventData*>(e_data);
		if (!s) return 1;
		char *end=NULL;
		double spacing = strtod(s->str,&end);
		if (end!=s->str && spacing>0) {
			data->LineSpacing(spacing);
			defaultspacing=data->LineSpacing();
			needtodraw=1;
		}
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

	} else if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        int i =s->info2; //id of menu item

        if ( i == CAPTION_Convert_To_Path
          || i == CAPTION_Create_Path_Object
          || i == CAPTION_Font_Dialog
          || i == CAPTION_InsertChar
          ) {
			return PerformAction(i);
		}
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

			} else if (over==CAPTION_Line_Spacing) {
				what="linespacing";
				sprintf(str,"%.10g",data->linespacing);

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

	double xm = grabpad*ScreenLine()/xmag/2;
	double ym = grabpad*ScreenLine()/ymag/2;
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

	 //checking for clicking down just outside bounds...
	if (p.x>=data->minx-xm && p.x<=data->maxx+xm+xm
			&& p.y>=data->miny-ym && p.y<=data->maxy+ym) {

		flatpoint m((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);

		if (p.x>data->maxx+xm) {
			if (data->lines.n>1 && p.y > data->maxy - (data->maxy-data->miny)*LSSIZE) return CAPTION_Line_Spacing;
			return CAPTION_Size;
		}
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
					if (extrahover) delete extrahover;
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
			if (lasthover==CAPTION_Move)              PostMessage(_("Move"));
			else if (lasthover==CAPTION_HAlign)       PostMessage(_("Horizontal alignment"));
			else if (lasthover==CAPTION_VAlign)       PostMessage(_("Vertical alignment"));
			else if (lasthover==CAPTION_Rotate)       PostMessage(_("Rotate, shift to snap"));
			else if (lasthover==CAPTION_Size)         PostMessage(_("Drag for font size, click to input"));
			else if (lasthover==CAPTION_Line_Spacing) PostMessage(_("Drag for line spacing, click to input"));
			else if (lasthover==CAPTION_Text)         PostMessage(_("Text"));
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

		} else if (over==CAPTION_Line_Spacing) {
			double factor=.1;
			if (state&ShiftMask) factor*=.1;
			if (state&ControlMask) factor*=.1;
			double d=data->LineSpacing() + factor*dv.y;
			data->LineSpacing(d);
			char str[100];
			sprintf(str, _("Line spacing %f %%"), d);

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

Laxkit::ShortcutHandler *CaptionInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc=new ShortcutHandler(whattype());

    sc->Add(CAPTION_Copy,            'c',ControlMask,0, "Copy"           , _("Copy"            ),NULL,0);
    sc->Add(CAPTION_Paste,           'v',ControlMask,0, "Paste"          , _("Paste"           ),NULL,0);
    sc->Add(CAPTION_LeftJustify,     'l',ControlMask,0, "LeftJustify"    , _("Left Justify"    ),NULL,0);
    sc->Add(CAPTION_CenterJustify,   'e',ControlMask,0, "CenterJustify"  , _("Center Justify"  ),NULL,0);
    sc->Add(CAPTION_RightJustify,    'r',ControlMask,0, "RightJustify"   , _("Right Justify"   ),NULL,0);
    sc->Add(CAPTION_TopJustify,      't',ControlMask,0, "TopJustify"     , _("Top Justify"     ),NULL,0);
    sc->Add(CAPTION_MiddleJustify,   'm',ControlMask,0, "MiddleJustify"  , _("Middle Justify"  ),NULL,0);
    sc->Add(CAPTION_BaselineJustify, 'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),NULL,0);
    sc->Add(CAPTION_BottomJustify,   'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
    sc->Add(CAPTION_Direction,       'D',ShiftMask|ControlMask,0, "Direction", _("Toggle Direction"),NULL,0);
    sc->Add(CAPTION_Decorations,     'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
    sc->Add(CAPTION_ShowBaselines,   'D',ShiftMask|ControlMask,0, "ShowBaselines", _("Show Baselines"),NULL,0);
    sc->Add(CAPTION_InsertChar,      'i',ControlMask,0, "InsertChar"     , _("Insert Character"),NULL,0);
    sc->Add(CAPTION_CombineChars,    'j',ControlMask,0, "CombineChars"   , _("Join Characters if possible"),NULL,0);
    sc->Add(CAPTION_Create_Path_Object,'P',ShiftMask|ControlMask,0, "ConvertToPaths", _("Convert to path object, but keep the old object"),NULL,0);
    sc->Add(CAPTION_Font_Dialog,     'F',ShiftMask|ControlMask,0, "SelectFont", _("Select font"),NULL,0);
    //sc->Add(CAPTION_Convert_To_Path, 'P',ShiftMask|ControlMask,0, "ConvertToPaths", _("Convert to path object"),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

int CaptionInterface::PerformAction(int action)
{
	if (action==CAPTION_Paste) {
		 //pasting with no data should create a new data
		viewport->PasteRequest(this, NULL);
		return 0;
	}

	 //everything else needs a data
	if (!data) return 1;

	if (action==CAPTION_Copy) {
		PostMessage(" *** Need to implement copy!!!");
		return 0;

	} else if (action==CAPTION_Decorations) {
		showdecs=!showdecs;
		if (showdecs) PostMessage(_("Show controls"));
		else PostMessage(_("Hide controls"));
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_LeftJustify) {
		data->xcentering=0;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_CenterJustify) {
		data->xcentering=50;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_RightJustify) {
		data->xcentering=100;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_TopJustify) {
		data->ycentering=0;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_MiddleJustify) {
		data->ycentering=50;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_BaselineJustify) {
		int i=data->baseline_hint+1;
		if (i<0) i=0;
		if (i>=data->lines.n) i=0;
		data->BaselineJustify(i);
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_BottomJustify) {
		data->ycentering=100;
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_InsertChar) {
		if (data && !child) {
			CharacterInterface *chari = new CharacterInterface(this, 0, dp, data->font);
			child=chari;
			viewport->Push(chari, viewport->HasInterface(object_id)-1,0);
		}
		return 0;

	} else if (action==CAPTION_ShowBaselines) {
		showbaselines++;
		if (showbaselines>1) showbaselines=0;
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_Direction) {
		if (data->direction==LAX_LRTB) data->direction=LAX_RLTB;
		else if (data->direction==LAX_RLTB) data->direction=LAX_TBLR;
		else if (data->direction==LAX_TBLR) data->direction=LAX_TBRL;
		else if (data->direction==LAX_TBRL) data->direction=LAX_LRTB;

		PostMessage(" *** need to implement text flow direction change in CaptionInterface!!");

		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action==CAPTION_CombineChars) {
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

	} else if (action==CAPTION_Convert_To_Path) {
		// need to construct the path object, and remove the current object
		// ***
		return 0;

	} else if (action==CAPTION_Create_Path_Object) {
		if (!data) return 0;
		SomeData *newdata = data->ConvertToPaths(false, NULL);

		 //add data to viewport, and select tool for it
		ObjectContext *oc=NULL;
		viewport->NewData(newdata,&oc);//viewport adds only its own counts
		//viewport->ChangeObject(oc, 1,true);
		newdata->dec_count();
		return 0;

	} else if (action == CAPTION_Font_Dialog) {
		if (!data) return 0;
		app->rundialog(new FontDialog(NULL, "Font",_("Font"),ANXWIN_REMEMBER, 10,10,700,700,0, object_id,"newfont",0,
					data->fontfamily, data->fontstyle, data->fontsize,
					NULL, //sample text
					data->font, true
					));
		return 0;
	}

	return 1;
}

int CaptionInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d) 
{
	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}


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

	}

	return 1; 
}

int CaptionInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d) 
{
	return 1; 
}


} // namespace LaxInterfaces


