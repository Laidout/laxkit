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
//    Copyright (C) 2004-2015 by Tom Lechner
//


#include <lax/fontmanager.h>
#include <cstdio>

#include <lax/misc.h>
#include <lax/anxapp.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include <iostream>
using namespace std;
#define DBG 


//include various backends...
#ifdef LAX_USES_CAIRO
#include <lax/fontmanager-cairo.h>
#endif 

#ifdef LAX_USES_XLIB
#include <lax/fontmanager-xlib.h>
#endif

#ifdef LAX_USES_GL
//#include <lax/fontmanager-gl.h>
#endif

#ifdef LAX_USES_SQLITE
#include <sqlite3.h>
#endif

#include <lax/lists.cc>

using namespace LaxFiles;


namespace Laxkit {


//-------------------------------------- LaxFont -------------------------------------------------
/*! \class LaxFont
 * \brief A wrapper for fonts that contains various metric information.
 *
 * LaxFont contains easy to get to common font information the text height,
 * ascent, and descent, and the widths of the 
 * characters.
 *
 * These objects are reference counted via dec_count() and inc_count(). 
 */


LaxFont::LaxFont()
{
	id=getUniqueNumber();
	textstyle=0;
	cntlchar='\\';

	family=NULL;
	style=NULL;
	psname=NULL;
	fontfile=NULL;
	fontindex=0;

	color=NULL;
	nextlayer=NULL;
}

LaxFont::~LaxFont()
{
	delete[] family;
	delete[] style;
	delete[] fontfile;
	delete[] psname;

	if (color) color->dec_count();
	if (nextlayer) nextlayer->dec_count();
}

/*! \fn LaxFont::LaxFont(const char *fontconfigstr,int nid)
 * \brief Constructor. Uses a fontconfig description.
 */

/*! \fn int LaxFont::charwidth(unsigned long chr,int real,double *width=NULL,double *height=NULL)
 * \brief Return the character width of ch.
 *
 * If the real character width is 0, and r==0, this should return width of "\9f" or "?" or some default
 * replacement character.
 */


/*! \fn int contextcharwidth(char *start,char *pos,int real,double *width=NULL,double *height=NULL)
 * \brief Like charwidth(), but return width of character in context of a whole string. In some languages, it's different.
 */


/*! \fn  double textheight()
 * \brief Usually, but NOT ALWAYS,  ascent plus descent. It is the default distance between baselines.
 */

/*! \fn  double ascent()
 * \brief Distance from baseline to top of the font. Note this is usually higher than the visual top.
 */

/*! \fn  double descent()
 * \brief Distance from baseline to bottom of the font. Note this is usually lower than the visual top.
 */

/*! \fn  double Resize(double newsize)
 * \brief Change the size of the cached font, keeping the font type and style. Returns newsize on success, or 0 on error.
 */


LaxFont *LaxFont::duplicate()
{
	FontManager *fontmanager=GetDefaultFontManager();

	LaxFont *old=this, *newlayer, *newfont=NULL;
	while (old) {
		newlayer = fontmanager->MakeFontFromFile(old->FontFile(), old->Family(), old->Style(), textheight(), -1);
		if (!newlayer) break;

		if (!newfont) newfont=newlayer;
		else newfont->AddLayer(-1, newlayer);

		old=old->nextlayer;
	}

	if (color) {
		anObject *ncolor=color->duplicate(NULL);
		if (ncolor) {
			newfont->SetColor(ncolor);
			ncolor->dec_count();
		}
	}

	return newfont;
}


/*! Default just return this->family.
 */
const char *LaxFont::Family()
{
	return family;
}

/*! Default just return this->style.
 */
const char *LaxFont::Style()
{
	return style;
}

/*! Default just return this->fontfile.
 */
const char *LaxFont::FontFile()
{
	return fontfile;
}

const char *LaxFont::PostscriptName()
{
	return psname;
}

/*! For multicolor font layers, the number of different fonts to layer up. Default is to return
 * 1, for normal single layer fonts.
 */
int LaxFont::Layers()
{
	int n=1;
	LaxFont *f=nextlayer;
	while (f) { n++; f=f->nextlayer; }
	return n;
}

/*! For multicolor font layers. which must be in range [0..Layers()], or NULL is returned.
 */
LaxFont *LaxFont::Layer(int which)
{
	if (which<0 || which>=Layers()) return NULL;

	LaxFont *f=this;
	do {
		if (which==0) return f;
		f=f->nextlayer;
		which--;
	} while (f);

	return NULL;
}

/*! Put layer at position where. 
 * Returns the new head. Normally this will be *this, but if where==0, then nfont becomes
 * the new front, which points to *this. If the head is replaced, this->count is not changed,
 * so that calling code can simply reassign without having to mess with dec/inc counts.
 *
 * Count of nfont will be absorbed.
 */
LaxFont *LaxFont::AddLayer(int where, LaxFont *nfont)
{
	if (where==0) {
		nfont->nextlayer=this;
		return nfont;
	}

	if (where<0 || where>=Layers()) where=Layers();

	LaxFont *f=this;
	where--;
	while (where>0 && f->nextlayer) { f=f->nextlayer; where--; }
	if (f->nextlayer) nfont->nextlayer=f->nextlayer;
	f->nextlayer=nfont;

	return this;
}

/*! Returns the new head. Normally this will be *this, but if which==0, then this->nextlayer becomes
 * the new head and that is returned.
 *
 * If popped_ret!=NULL, then do not dec_count the removed layer, but return it. Otherwise, the popped
 * layer is dec_counted, even if it was the head.
 *
 * This lets you easily delete the first layer with something like:
 *  font = font->RemoveLayer(0, NULL), and not have to delete it yourself.
 *
 * If only one layer or which is out of bounds, nothing is done and this is returned. popped_ret gets
 * set to NULL in this case.
 */
LaxFont *LaxFont::RemoveLayer(int which, LaxFont **popped_ret)
{
	if (!nextlayer) { return this; if (popped_ret) *popped_ret=NULL; }
	if (which<0 || which>=Layers()) { return this; if (popped_ret) *popped_ret=NULL; }

	if (which==0) {
		LaxFont *f=nextlayer;
		nextlayer=NULL;
		 if (popped_ret) {
			 *popped_ret=this;
		 } else this->dec_count();
		return f;
	}

	LaxFont *f=this;
	which--;
	while (which>0 && f->nextlayer) { f=f->nextlayer; which--; }
	LaxFont *of=f->nextlayer;
	f->nextlayer = of->nextlayer;
	of->nextlayer=NULL;

	if (popped_ret) *popped_ret=of;
	else of->dec_count();

	return this;
}

/*! Take layer which and put it at position to
 *
 * If the first layer changes, the new head is returned. Old head is returned if not, or
 * if there is an out of bounds error.
 */
LaxFont *LaxFont::MoveLayer(int which, int to)
{
	if (which==to) return this;
	if (which<0 || which>=Layers()) return this;
	if (to<0 || to>=Layers()) return this;

	LaxFont *head=this;
	LaxFont *popped=NULL;

	head=RemoveLayer(which, &popped);
	//if (to>which) to--;
	head=head->AddLayer(to, popped);

	return head;
}

/*! Basically do nextlayer->dec_count() and set to null.
 */
void LaxFont::RemoveAllLayers()
{
	if (!nextlayer) return;
	nextlayer->dec_count();
	nextlayer=NULL;
}

int LaxFont::SetColor(anObject *ncolor)
{
	if (color) color->dec_count();
	color=ncolor;
	if (color) color->inc_count();
	return 0;
}


//-------------------------------------- FontDialogFont -------------------------------------
/*! \class FontDialogFont
 * \brief Describes a font as dealt with in a FontDialog.
 */

FontDialogFont::FontDialogFont(int nid)
{
    id=nid;

    name=NULL;
    psname=NULL;
    family=NULL;
    style=NULL;
	format=NULL;
    file=NULL;
    index=0;
    preview=NULL;

    numtags=0;
    tags=NULL;

	fc_pattern=NULL; //assume fontmanager->fontlist gets destroyed AFTER this
}

FontDialogFont::~FontDialogFont()
{
    delete[] name;
    delete[] psname;
    delete[] family;
    delete[] style;
    delete[] file;
    delete[] format;
    if (preview) preview->dec_count();
    delete[] tags;
}

bool FontDialogFont::Match(const char *mfamily, const char *mstyle)
{
    if (mfamily && family && !strcasecmp(mfamily,family)) {
        if (mstyle && style && !strcasecmp(mstyle,style)) return true;
    }

    return false;
}

/*! Return value is (index in tags array)+1, or 0 if not there.
 */
int FontDialogFont::HasTag(int tag_id)
{
    for (int c=0; c<numtags; c++) {
        if (tags[c]==tag_id) return c+1;
    }
    return 0;
}

/*! Returns the new number of tags.
 */
int FontDialogFont::AddTag(int tag_id)
{
    if (HasTag(tag_id)) return numtags;

    int *newtags=new int[numtags+1];
    if (numtags) memcpy(newtags, tags, numtags*sizeof(int));
    newtags[numtags]=tag_id;
	delete[] tags;
	tags=newtags;
    numtags++;
    return numtags;
}

void FontDialogFont::RemoveTag(int tag_id)
{
    for (int c=0; c<numtags; c++) {
        if (tags[c]==tag_id) {
            for (int c2=c; c2<numtags-1; c2++) tags[c2]=tags[c2+1];
            numtags--;
            if (numtags==0) { delete[] tags; tags=NULL; }
        }
    }
}

int FontDialogFont::UseFamilyStyleName()
{
	char *nname=new char[(family ? strlen(family) : 0) + (style ? strlen(style) : 0) + 3];

	sprintf(nname,"%s, %s", family?family:"", style?style:"");
	delete[] name;
	name=nname;

	return 0;
}

/*! Parse the psname and use that instead of family+style.
 * This should result in an almost unique name, barring the same font file being loaded
 * in from different spots on the drive.
 */
int FontDialogFont::UsePSName()
{
	if (!psname) return 1;

	int n=0;
	for (char *p=psname; *p; p++) {
		if (isupper(*p)) { n++; }
	}

	char *nname=new char[strlen(psname)+n+10];
	int i=0;
	for (char *p=psname; *p; p++) {
		if (p!=psname && isupper(*p)) nname[i++]=' ';
		if (!isalnum(*p)) nname[i++]=' ';
		else nname[i++]=*p;
	}
	nname[i]='\0';

	delete[] name;
	name=nname;
	return 0;
}


//--------------------------- FontTag ------------------------------------------
/*! \class FontTag
 * Tag definitions for use in FontDialogFont
 */

FontTag::FontTag(int nid, int ntagtype, const char *ntag)
{
	id=nid;
	if (id<=0) id=getUniqueNumber();
	tagtype=ntagtype;
	tag=newstr(ntag);
}

FontTag::~FontTag()
{
	delete[] tag;
}


//--------------------------- FontManager ------------------------------------------


/*! \class FontManager
 * \brief The font manager used by anXApp to simplify keeping track of what fonts are loaded.
 */

FontManager::FontManager()
{
	fcconfig=NULL;
	ft_library=NULL;
}

FontManager::~FontManager()
{
	if (fcconfig) FcConfigDestroy(fcconfig);
	if (ft_library) FT_Done_FreeType(*ft_library);
}

FT_Library *FontManager::GetFreetypeLibrary()
{
	if (!ft_library) {
		ft_library = new FT_Library;
		FT_Error ft_error = FT_Init_FreeType (ft_library);

		if (ft_error) {
			cerr <<"Could not initialize freetype!!"<<endl;
			return NULL;
		}

	}
	
	return ft_library;
}

FcConfig *FontManager::GetConfig()
{
	if (fcconfig) return fcconfig;

     // Initialize FontConfig library.
	FcInit(); //does nothing if FcInit() already called somewhere else
	fcconfig=FcInitLoadConfigAndFonts();

	return fcconfig;
}

/*! for a qsort()
 */
int cmp_fontinfo_name(const void *f1p, const void *f2p)
{
	FontDialogFont *f1=*((FontDialogFont**)f1p);
	FontDialogFont *f2=*((FontDialogFont**)f2p);

	if (!f1->name) return -1;
	if (!f2->name) return 1;
	return strcmp(f1->name, f2->name);
}

/*! for a qsort()
 */
int cmp_fontinfo_psname(const void *f1p, const void *f2p)
{
	FontDialogFont *f1=*((FontDialogFont**)f1p);
	FontDialogFont *f2=*((FontDialogFont**)f2p);

	if (!f1->psname) return -1;
	if (!f2->psname) return 1;
	return strcmp(f1->psname, f2->psname);
}

/*! for a qsort()
 */
static int cmp_fontinfo_file(const void *f1p, const void *f2p)
{
	FontDialogFont *f1=*((FontDialogFont**)f1p);
	FontDialogFont *f2=*((FontDialogFont**)f2p);

	if (!f1->file) return -1;
	if (!f2->file) return 1;
	return strcmp(f1->file, f2->file);
}


/*! If this->fonts.n>0, then just return &this->fonts. Else populate then return it.
 */
PtrStack<FontDialogFont> *FontManager::GetFontList()
{
	if (fonts.n) return &fonts;
	
	if (!fcconfig) GetConfig(); //inits fontconfig


	DBG cerr <<"Scanning for installed fonts..."<<endl;

     // Read in all the fonts, and arrange to make using the list a little more ui friendly
    FcResult result;
    FcValue v;

    FontDialogFont *f=NULL;
    FcFontSet *fontset=FcConfigGetFonts(fcconfig, FcSetSystem); //"This font set is owned by the library and must not be modified or freed"

    for (int c=0; c<fontset->nfont; c++) {
         // Usually, for each font family, there are several styles
         // like bold, italic, etc.
        result=FcPatternGet(fontset->fonts[c],FC_FAMILY,0,&v);
        if (result!=FcResultMatch) continue;

        f=new FontDialogFont(c);
        f->fc_pattern=fontset->fonts[c];

        makestr(f->family, (const char *)v.u.s);

        result=FcPatternGet(fontset->fonts[c],FC_STYLE,0,&v);
        if (result==FcResultMatch) makestr(f->style, (const char *)v.u.s);

        result=FcPatternGet(fontset->fonts[c],FC_POSTSCRIPT_NAME,0,&v);
        if (result==FcResultMatch) makestr(f->psname, (const char *)v.u.s);

        result=FcPatternGet(fontset->fonts[c],FC_FILE,0,&v);
        if (result==FcResultMatch) makestr(f->file, (const char *)v.u.s);

        result=FcPatternGet(fontset->fonts[c],FC_INDEX,0,&v);
        if (result==FcResultMatch) f->index = v.u.i;

        result=FcPatternGet(fontset->fonts[c],FC_FONTFORMAT,0,&v);
        if (result==FcResultMatch) makestr(f->format, (const char *)v.u.s);

		f->UseFamilyStyleName();
		//f->UsePSName();

        //FC_OUTLINE
        //FC_SCALABLE
        //FC_LANG -> string of languages like "en|es|bs|ch"
        //FC_CAPABILITY -> has "otlayout:" stuff
        //FC_FONTFORMAT
        //FC_FONT_FEATURES

        DBG cerr <<c<<", found font: Family,style,file: "<<f->family<<", "<<f->style<<", "<<f->file<<endl;

        fonts.push(f);

    }

	 //sort by name, and try to rename duplicates
	qsort(fonts.e, fonts.n, sizeof(FontDialogFont*), cmp_fontinfo_name); 
	int start=0,end=-1;
	for (int c=1; c<=fonts.n; c++) {
		if (c<fonts.n && start!=c && !strcmp(fonts.e[start]->name, fonts.e[c]->name)) {
			end=c;
			continue;
		}
		if (end>0) {
			 //doubles found
			for (int c2=start+1; c2<=end; c2++) {
				fonts.e[c2]->UsePSName();
			}
			//qsort(fonts.e+start, end-start+1, sizeof(FontDialogFont*), cmp_fontinfo_name); 

			// *** check for doubles still, if found, modify based on filename diff??
			
			end=-1;
		}
		start=c;
	}


	 //sort by file name
	qsort(fonts.e, fonts.n, sizeof(FontDialogFont*), cmp_fontinfo_file); 

 
	 //need to differentiate names where family and style are equal to other fonts
	// *** - 1st try could take apart postscript name: turn something like 
	//        DroidSansEthiopic-Bold  ->  Droid Sans Ethiopic Bold  (a bit unreliable perhaps)
	//     - maybe diff the files' basenames. if still the same, just say "(file 1)", "(file 2)", etc.
	

	DBG cerr <<"Done scanning for installed fonts."<<endl;

	RetrieveFontmatrixTags();

	return &fonts;
}


/*! \fn LaxFont *FontManager::Add(LaxFont *font,int nid)
 * \brief Add an existing font to the fontmananger.
 * 
 * If nid<=0 then the id is from getUniqueNumber().
 * 
 * Returns the new font object, or NULL on failure.
 */


/*! \fn LaxFont *FontManager::CheckOut(int id)
 *  \brief Add a count of 1 to the LaxFont with the given id, and return the corresponding font object.
 *  Returns the LaxFont object, or NULL if object not found.
 *
 * Please remember that when you are all done with the font, you must decrement its count.
 */


/*! \fn LaxFont *FontManager::MakeFontFromFile(const char *file, double size, int nid)
 * \brief Create and return a LaxFont, but do not store it within the fontmanager.
 *
 * If nid<=0, then a random id is given to the font.
 */


/*! \fn LaxFont *FontManager::MakeFontFromStr(const char *str, int nid)
 * \brief Create and return a LaxFont, but do not store it within the fontmanager.
 *
 * str is a FontConfig string.
 * If nid<=0, then a random id is given to the font.
 */


/*! \fn LaxFont *FontManager::MakeFont(const char *family, const char *style, double size, int nid)
 * \brief Create and return a LaxFont, but do not store it within the fontmanager.
 *
 * If nid<=0, then a random id is given to the font.
 */

const char *FontManager::GetTagName(int id)
{
	for (int c=0; c<tags.n; c++) {
		if (tags.e[c]->id==id) return tags.e[c]->tag;
	}
	return NULL;
}

/*! Return index in tags corresponding to tag, or -1 if not found.
 */
int FontManager::GetTagId(const char *tag)
{
	if (!tag) return -1;
	for (int c=0; c<tags.n; c++) {
		if (!strcasecmp(tag,tags.e[c]->tag)) return tags.e[c]->id;
	}
	return -1;
}

FontDialogFont *FontManager::FindFontFromFile(const char *file)
{
	if (!fonts.n || !file) return NULL;

	 //binary search, assume fonts is sorted by file
	int s=0, e=fonts.n-1, mid, cmp; 

	if (!strcmp(file, fonts.e[s]->file)) return fonts.e[s];
	if (!strcmp(file, fonts.e[e]->file)) return fonts.e[e];;

	do {
		mid=(s+e)/2;
		if (mid==e || mid==s) return NULL; //already checked

		cmp=strcmp(file, fonts.e[mid]->file);
		if (cmp==0) return fonts.e[mid];
		if (cmp<0) {
			e=mid;
		} else {
			s=mid;
		}
	} while (s!=e);

	return NULL;
}

/*! Returns number of tags retrieved.
 *
 * If sqlite is enabled for the Laxkit, then this will try to get the tags from the Fontmatrix
 * database (at ~/.Fontmatrix/Data.sql), and apply to fonts, which should have been populated already.
 */
int FontManager::RetrieveFontmatrixTags()
{
	int n=0;

#ifdef LAX_USES_SQLITE

	char *dbfile=newstr("~/.Fontmatrix/Data.sql");
	expand_home_inplace(dbfile); 
	int status=0;

	sqlite3 *db=NULL;

	try {
		if (!S_ISREG(file_exists(dbfile,1,NULL))) throw(1);

		int error = sqlite3_open_v2(dbfile, &db, SQLITE_OPEN_READONLY, NULL);
		if (error != SQLITE_OK) {
			cerr <<"Couldn't open database "<<dbfile<<": "<<sqlite3_errmsg(db)<<endl;
			throw (2);
		}


		 //first read in the tag names
		DBG cerr <<"Get tag names..."<<endl;

		const char *sqlstr="SELECT tag FROM fontmatrix_tags GROUP BY tag";
		sqlite3_stmt *stmt=NULL;
		const char *strremain=NULL;

		status = sqlite3_prepare_v2(db, sqlstr, strlen(sqlstr)+1, &stmt, &strremain);
		if (status !=SQLITE_OK) {
			DBG cerr <<"Could not prepare statement! "<<sqlite3_errmsg(db)<<endl;
			throw (30);
		}

		int rownum=0;
		do {
			status = sqlite3_step(stmt);

			if (status == SQLITE_ROW) {

				DBG int id           = sqlite3_column_int(stmt, 0);
				//const unsigned char *text = sqlite3_column_text(stmt, 1);
				const char *tag = (const char *)sqlite3_column_text(stmt, 0);

				DBG cerr <<rownum<<".  id:"<<id<<"   tag: "<<tag<<endl;

				if (!isblank(tag)) {
					int cmp;

					if (tags.n==0) {
						tags.push(new FontTag(-1, 2, tag)); //puts at end 

					} else for (int c=0; c<tags.n; c++) {
						cmp=strcasecmp(tag, tags.e[c]->tag);

						if (cmp<0) {
							tags.push(new FontTag(-1, 2, tag));
							break;
						} else if (cmp==0) break; //tag exists already!

						if (cmp>0 && c==tags.n-1) tags.push(new FontTag(-1, 2, tag)); //puts at end 
					}
				}

				rownum++;

			} else if (status!=SQLITE_DONE) {
				cout <<"  *** step error!!" << sqlite3_errmsg(db) <<endl;
				sqlite3_finalize(stmt);
				throw (40);
			}
		} while (status !=SQLITE_DONE);

		sqlite3_finalize(stmt);

		DBG cerr <<"Tags:"<<endl;
		DBG for (int c=0; c<tags.n; c++) {
		DBG 	cerr <<c<<". ("<<tags.e[c]->id<<")  \""<<tags.e[c]->tag<<"\""<<endl;
		DBG }
		DBG cerr <<"...Get tag names done!"<<endl;
		


		//then apply tags to font list, which is sorted by file

		DBG cerr <<endl<<" matching tags to font files..."<<endl;

		 //for each font in fontmatrix_id, get all tags for that font..
		sqlstr="SELECT fontident,digitident FROM fontmatrix_id";
		stmt=NULL;
		strremain=NULL;
		error = sqlite3_prepare_v2(db, sqlstr, strlen(sqlstr)+1, &stmt, &strremain);
		FontDialogFont *font=NULL;

		if (error !=SQLITE_OK) {
			cerr <<"Could not prepare statement! "<<sqlite3_errmsg(db)<<endl;
			throw (50);
		}

		rownum=0;
		do {
			error = sqlite3_step(stmt);

			if (error == SQLITE_ROW) {

				int id                    = sqlite3_column_int (stmt, 1);
				const char *file = (const char *)sqlite3_column_text(stmt, 0);

				font=FindFontFromFile(file);
				if (font) {

					//DBG cerr <<"row "<<rownum<<": id:"<<id<<"   file: "<<file<<endl;
					//DBG cerr <<"    tags: ";
					rownum++;

					const char *sqlstr2="SELECT digitident,tag FROM fontmatrix_tags WHERE digitident=?";
					sqlite3_stmt *stmt2=NULL;
					int tagid;

					int error2 = sqlite3_prepare_v2(db, sqlstr2, strlen(sqlstr2)+1, &stmt2, &strremain);
					if (error2 !=SQLITE_OK) {
						cerr <<"Could not prepare statement for fontmatrix_tags! "<<sqlite3_errmsg(db)<<endl;
						throw (51);
					}

					sqlite3_bind_int(stmt2, 1, id);

					do {
						error2 = sqlite3_step(stmt2);
						if (error2 == SQLITE_ROW) {
							const char *tag = (const char *)sqlite3_column_text(stmt2,1);
							//DBG cerr <<"\""<<tag<<"\""<<"  ";

							tagid = GetTagId(tag);
							if (tagid>=0) {
								font->AddTag(tagid);
							}
						}
					} while (error2==SQLITE_ROW);

					DBG cerr <<endl;
					sqlite3_finalize(stmt2);
				}


			} else if (error!=SQLITE_DONE) {
				DBG cerr <<"  *** step error!!" << sqlite3_errmsg(db) <<endl;
				sqlite3_finalize(stmt);
				throw (52);
			}

		} while (error !=SQLITE_DONE);


	} catch (int error) {
		DBG cerr <<" Error processing "<<dbfile<<"! err number: "<<error<<endl;
	}

	delete[] dbfile;


	//connect to db
	//db  has table fontmatrix_tags with [digitident==int, tag==text]
	//another table fontmatrix_id    has [fontident==text, digitident==int]
	//the fontident is the file path of the font, *** not sure if it does indexed fonts properly

#else
	cerr << "Not using Fontmatrix database. Enable sqlite and install fontmatrix to use!"<<endl;
#endif

	return n;
}



//--------------------------------------- Default FontManager Stuff ---------------------------

//! There can be only one (default displayer).
static FontManager *fontmanager = NULL;

/*! \typedef FontManager *NewFontManagerFunc(aDrawable *w);
 * \brief Function type to create new FontManager objects.
 */

//! The default FontManager "constructor".
NewFontManagerFunc newFontManager = NULL;



//! Set the default "constructor" for FontManager objects.
/*! If func==NULL, then use the default, checks for cairo, then for xlib.
 * Return 0 for success, or 1 for unable to set to non-null.
 */
int SetNewFontManagerFunc(const char *backend)
{
	NewFontManagerFunc func=NULL;
	if (!backend) backend=LAX_DEFAULT_BACKEND;

	if (!func) {
		if (!strcmp(backend,"cairo")) {
#ifdef LAX_USES_CAIRO
			if (!func) func=newFontManager_cairo;
#endif 
			if (!func) {
				cerr <<" Ack! Trying to initialize cairo font manager, but no cairo in Laxkit!!"<<endl;
				return 1;
			}


		} else if (!strcmp(backend,"xlib")) {

#ifdef LAX_USES_XLIB
			if (!func) func=newFontManager_xlib;
#endif
			if (!func) {
				cerr <<" Ack! Trying to initialize xlib font manager, but no xlib in Laxkit!!"<<endl;
				return 1;
			}
		}
	}

    newFontManager=func;
    return 0;
}

FontManager *GetDefaultFontManager()
{
    if (!newFontManager) SetNewFontManagerFunc(NULL);
    if (!fontmanager && newFontManager) fontmanager=newFontManager();
    return fontmanager;
}






} // namespace Laxkit

