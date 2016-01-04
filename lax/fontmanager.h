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
#ifndef _LAX_FONTMANAGER_H
#define _LAX_FONTMANAGER_H

#include <cstdlib>
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ERRORS_H

#include <lax/anobject.h>
#include <lax/laximages.h>
#include <lax/lists.h>



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
 
	int has_color; //1 for manual layered, 2 for COLR layers, 3 for full svg
	               //1 can still be used by normal font rendering.. 
	               //until freetype/cairo supports full multicolor, have to manually draw other glyphs


	//ScreenColor *glyph_color;
	//int color_id;
	//GlyphImage  *glyph_img;

	GlyphPlace();
	~GlyphPlace();
};

//---------------------------- LaxFont -------------------------------
class LaxFont : public anObject
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
	virtual double extent(const char *str,int len) = 0;
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
	char *format;
    int index; //index in file when more than one font in file

    LaxImage *preview;
    int numtags, *tags;

    FcPattern *fc_pattern; //owned by the master list

    FontDialogFont(int nid);
    virtual ~FontDialogFont();
    virtual bool Match(const char *mfamily, const char *mstyle);
    virtual int HasTag(int tag_id);
    virtual int AddTag(int tag_id);
    virtual void RemoveTag(int tag_id);

	virtual int UsePSName();
	virtual int UseFamilyStyleName();
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
		TAG_Other,
		TAG_MAX
	};
    int id;
    int tagtype; //such as from Fontmatrix==2, user favorites==1, document defined, etc
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

	virtual const char *LanguageString(int id);
	virtual const char *ScriptString(int id);
	virtual int LanguageId(const char *str, bool create_if_not_found);
	virtual int ScriptId  (const char *str, bool create_if_not_found);

	virtual int RetrieveFontmatrixTags();
	virtual int GetTagId(const char *tag);
	virtual const char *GetTagName(int id);
	virtual FontDialogFont *FindFontFromFile(const char *file);
};


//--------------------------------------- Default Renderer Stuff ---------------------------

typedef FontManager *(*NewFontManagerFunc)();
int SetNewFontManagerFunc(const char *backend);

int SetDefaultFontManager(FontManager *manager);
FontManager *GetDefaultFontManager();



} // namespace Laxkit

#endif

