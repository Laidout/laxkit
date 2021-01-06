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
//    Copyright (C) 2004-2015 by Tom Lechner
//
#ifndef _LAX_FONTMANAGER_H
#define _LAX_FONTMANAGER_H

#include <cstdlib>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ERRORS_H

#include <lax/anobject.h>
#include <lax/errorlog.h>
#include <lax/tagged.h>
#include <lax/resources.h>
#include <lax/laximages.h>
#include <lax/lists.h>
#include <lax/palette.h>
#include <lax/resources.h>



namespace Laxkit {


//------------------------------ GlyphPlace -----------------------------------
struct GlyphPlace
{
	int index; //font glyph index
	unsigned int cluster; //in Laxkit, utf8 byte position in original string
	int numchars; //how many unicode chars this cluster covers, such as 1 glyph for 3 chars in "ffi"

	double x_advance;
	double y_advance;
	double x_offset; //shift from current position based on advances
	double y_offset;
	double x;
	double y;
 
	int styleid; //can be used to specify different font layer, or totally different font, depends on context
	int has_color; //1 for manual layered, 2 for COLR layers, 3 for full svg
	               //1 can still be used by normal font rendering.. 
	               //until freetype/cairo supports full multicolor, have to manually draw other glyphs


	//ScreenColor *glyph_color;
	//int color_id;
	//GlyphImage  *glyph_img;

	GlyphPlace();
	~GlyphPlace();
};

class ColrGlyphMap
{
  public:
	int initial_glyph; //base glyph, and also the fallback glyph
	int numglyphs;
	int *glyphs; //z order bottom to top
	int *colors; //index into a palette held elsewhere
	ColrGlyphMap(int initial, int n, int *g, int *cols);
	~ColrGlyphMap();
};

//---------------------------- LaxFont -------------------------------
class LaxFont : public Resourceable
{
  protected:
	char *family;
	char *style;
	char *fontfile;
	char *psname;
	int fontindex;
	anObject *color;//optional, preferred color, a Color for single layer, Palette for multicolor

  public:
	int id;
	unsigned long textstyle;
	char cntlchar;

	LaxFont *nextlayer; //for layered multicolor fonts

	LaxFont();
	virtual ~LaxFont();
	virtual LaxFont *duplicate();
	virtual anObject *duplicate(anObject *ref) { return duplicate(); }

	virtual int FontId() { return id; }

	virtual int SetFromFile(const char *nfile, const char *nfamily, const char *nstyle, double size) = 0;

	virtual double charwidth(unsigned long chr,int real,double *width=NULL,double *height=NULL) = 0;
	virtual double contextcharwidth(char *start,char *pos,int real,double *width=NULL,double *height=NULL) = 0;
	virtual double ascent() = 0;
	virtual double descent() = 0;
	virtual double textheight() = 0;
	virtual double Msize() = 0;
	virtual const char *Family();
	virtual const char *Style();
	virtual const char *FontFile();
	virtual const char *PostscriptName();
	virtual double Extent(const char *str,int len) = 0;
	virtual double Extent(const char *str,int len, double *w, double *h, double *asc, double *desc);
	virtual double Resize(double newsize) = 0;

	 //stuff for layered color fonts
	virtual int Layers();
	virtual LaxFont *Layer(int which);
	virtual LaxFont *NextLayer() { return nextlayer; }
	virtual LaxFont *AddLayer(int where, LaxFont *nfont);
	virtual LaxFont *RemoveLayer(int which, LaxFont **popped_ret);
	virtual LaxFont *MoveLayer(int which, int to);
	virtual void RemoveAllLayers();

	virtual int HasColors() { return 0; } //1 for is manual layered font, 2 for colr based, 3 for svg
	virtual anObject *GetColor() { return color; } //a Color or Palette
	virtual int SetColor(anObject *ncolor);

	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att, int what, LaxFiles::DumpContext *context);
	//virtual void dump_in_atts(LaxFiles::Attribute *att,int flag, LaxFiles::DumpContext *context);
};


//---------------------------- FontDialogFont -------------------------------
/*! \class FontDialogFont
 * \brief Describes a font as dealt with in a FontDialog.
 */
class FontDialogFont
{
  public:
    int id;
    char *name;
    char *family;
    char *style;
    char *psname;
    char *file;
	int format;
    int index; //index in file when more than one font in file

    LaxImage *preview;
    IntTagged tags;
	int favorite; //num is rank in fav list, starting with 1. 0 is not favorite

    FcPattern *fc_pattern; //owned by the master list

    FontDialogFont(int nid=-1, const char *nfile=NULL, const char *nfamily=NULL, const char *nstyle=NULL);
    virtual ~FontDialogFont();
    virtual bool Match(const char *mfamily, const char *mstyle);
    virtual int HasTag(int tag_id);
    virtual int AddTag(int tag_id);
    virtual void RemoveTag(int tag_id);
	
	virtual void Favorite(int nfav) { favorite=nfav; }
	virtual int Favorite() { return favorite; }

	virtual int UsePSName();
	virtual int UseFamilyStyleName();
};

class LayeredDialogFont : public FontDialogFont
{
  public:
	PtrStack<FontDialogFont> layers; //must be non-layered fonts, points to other existing fonts in fontmanager->fonts
	Palette *palette;

    LayeredDialogFont(int nid=-1);
    virtual ~LayeredDialogFont() { if (palette) palette->dec_count(); }
};

//--------------------------- FontTag ------------------------------------------
class FontTag
{
  public:
	enum FontTagType {
		TAG_None=0,
		TAG_Fontmatrix,
		TAG_Favorite,
		TAG_File_Type,
		TAG_Monospace,
		TAG_Format,
		TAG_Other,
		TAG_MAX
	};
    int id;
    int tagtype; //such as from TAG_Fontmatrix, TAG_Favorites, etc. See FontTag::FontTagType
    char *tag;

	FontTag(int nid, int ntagtype, const char *ntag);
	virtual ~FontTag();
};



//---------------------------- FontManager -------------------------------

class FontManager : public anObject
{
  protected:
	FcConfig *fcconfig;
	FT_Library *ft_library;
	PtrStack<FontDialogFont> fonts;
	ResourceDirs dirs; //extra places outside normal fontconfig to look for fonts
	
  public: 
	PtrStack<FontTag> tags;

	FontManager();
	virtual ~FontManager();

	virtual LaxFont *MakeFontFromFile(const char *file, const char *nfamily, const char *nstyle, double size, int nid) = 0;
	virtual LaxFont *MakeFontFromStr(const char *fcstr, int nid) = 0;
	virtual LaxFont *MakeFont(const char *family, const char *style, double size, int nid) = 0;
	virtual LaxFont *MakeFont(int nid) = 0;
	virtual LaxFont *Add(LaxFont *font,int nid) = 0;
	virtual LaxFont *CheckOut(int id) = 0;

	virtual FcConfig *GetConfig();
	virtual FT_Library *GetFreetypeLibrary();
	virtual PtrStack<FontDialogFont> *GetFontList();

	virtual int AddDir(const char *dir);
	virtual int DumpInFontList(const char *file, ErrorLog *log);
	virtual FontDialogFont *DumpInFontDialogFont(LaxFiles::Attribute *att);

	virtual const char *LanguageString(int id);
	virtual const char *ScriptString(int id);
	virtual int LanguageId(const char *str, bool create_if_not_found);
	virtual int ScriptId  (const char *str, bool create_if_not_found);

	virtual int RetrieveFontmatrixTags();
	virtual int GetTagId(const char *tag);
	virtual const char *GetTagName(int id);
	virtual FontDialogFont *FindFontFromFile(const char *file);
	 
	virtual LaxFont *dump_in_font(LaxFiles::Attribute *att, LaxFiles::DumpContext *context);
};


//--------------------------------------- Default Renderer Stuff ---------------------------

typedef FontManager *(*NewFontManagerFunc)();
int SetNewFontManagerFunc(const char *backend);

int SetDefaultFontManager(FontManager *manager);
FontManager *GetDefaultFontManager();



} // namespace Laxkit

#endif

