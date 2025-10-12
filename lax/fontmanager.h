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
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>

#include <vector>

#include <lax/anobject.h>
#include <lax/errorlog.h>
#include <lax/tagged.h>
#include <lax/resources.h>
#include <lax/laximages.h>
#include <lax/lists.h>
#include <lax/gradientstrip.h>
#include <lax/utf8string.h>


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
	char *psname;
	char *fontfile;
	int fontindex;   //!< Index of font within fontfile.
	anObject *color; //!< optional, preferred color, a Color for single layer, Palette for multicolor

	char *cached_blob_file;
	hb_blob_t *hb_blob;
	hb_face_t *hb_face;
	hb_font_t *hb_font; //!< Cached harfbuzz font needed for glyph positioning.

	std::vector<hb_ot_var_axis_info_t> axes_array;
	std::vector<hb_variation_t> variation_data; // current values for variable type axes of a font
	std::vector<hb_feature_t> user_features;    // passed into hb_shape, which figures out which glyphs occur
	//todo: figure out why numstack fails with hb_*:
	//NumStack<hb_ot_var_axis_info_t> axes_array; // has axis min/max/default values for variable type axes
	//NumStack<hb_variation_t> variation_data; // current values for variable type axes
	//NumStack<hb_feature_t> userfeatures; // passed into hp_shape

  public:
	int id;
	unsigned long textstyle;
	char cntlchar;

	LaxFont *nextlayer; //for layered multicolor fonts

	LaxFont();
	virtual ~LaxFont();
	virtual const char *whattype() { return "Font"; }
	virtual LaxFont *duplicateFont();
	virtual anObject *duplicate() { return duplicateFont(); }

	virtual int FontId() { return id; }

	virtual int SetFromFile(const char *nfile, const char *nfamily, const char *nstyle, double size) = 0;

	virtual double charwidth(unsigned long chr,int real,double *width=NULL,double *height=NULL) = 0;
	// virtual double contextcharwidth(char *start,char *pos,int real,double *width=NULL,double *height=NULL) = 0;
	virtual double ascent() = 0;
	virtual double descent() = 0;
	virtual double textheight() = 0;
	virtual double Msize() = 0;
	virtual const char *Family();
	virtual const char *Style();
	virtual const char *FontFile();
	virtual int FontIndex() { return fontindex; }
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

	// OpenType variations
	virtual const char *AxisName(int index) const = 0;
	virtual int AxisIndex(const char *tag) const = 0;
	virtual int NumAxes() { return (int)variation_data.size(); }
	virtual bool SetAxis(int index, double value) = 0;
	virtual int SetAxes(const char *str) = 0;
	virtual double GetAxis(int index) const = 0;
	virtual bool SetFeature(const char *feature, bool active) = 0;
	virtual int CopyVariations(LaxFont *from_this);

	virtual int HasColors() { return 0; } //1 for is manual layered font, 2 for colr based, 3 for svg
	virtual anObject *GetColor() { return color; } //a Color or Palette
	virtual int SetColor(anObject *ncolor);

	virtual Attribute *dump_out_atts(Attribute *att, int what, DumpContext *context);
	//virtual void dump_in_atts(Attribute *att,int flag, DumpContext *context); <- see FontManager::dump_in_font()
};


//---------------------------- FontDialogFont -------------------------------
/*! \class FontDialogFont
 * Describes a font as dealt with in a FontDialog.
 * A list of these is maintained in FontManager::fonts.
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
    int index; //index in file when more than one font in file
	int format; // Currently -1 for default, or id for "Layered" tag (see also FontManager::tags).
    bool has_color;
    FontDialogFont *based_on = nullptr; //!< Extra style variations may modify based_on to create a new entry

    LaxImage *preview;
    IntTagged tags;
	int favorite; //num is rank in fav list, starting with 1. 0 is not favorite

	FcPattern *fc_pattern; //owned by the master list

	int variations_state = -1; //-1 for unknown, 0 for none, 1 for already queried
	std::vector<Utf8String> feature_tags;
	std::vector<Utf8String> axes_names;
	std::vector<hb_ot_var_axis_info_t> axes_array;
	
    FontDialogFont(int nid = -1, const char *nfile = nullptr, const char *nfamily = nullptr, const char *nstyle = nullptr);
    virtual ~FontDialogFont();

    virtual bool Match(const char *mfamily, const char *mstyle);
    virtual bool HasTag(int tag_id);
    virtual int AddTag(int tag_id);
    virtual void RemoveTag(int tag_id);
	
	virtual void Favorite(int nfav) { favorite = nfav; }
	virtual int Favorite() { return favorite; }

	virtual int UsePSName();
	virtual int UseFamilyStyleName();
	virtual bool UpdateVariations();

	virtual int FindAxisIndex(const char *name);
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
		TAG_Color,
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
	PtrStack<FontDialogFont> favorites;
	PtrStack<FontDialogFont> recent;

	ResourceDirs dirs; //extra places outside normal fontconfig to look for fonts
	NumStack<Utf8String> favorites_files;
	
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
	virtual LaxFont *dump_in_font(Attribute *att, DumpContext *context); // workaround for LaxFont subclass dump in complications

	virtual FcConfig *GetConfig();
	virtual FT_Library *GetFreetypeLibrary();
	virtual PtrStack<FontDialogFont> *GetFontList();

	virtual int AddDir(const char *dir);
	virtual int DumpInFontList(const char *file, ErrorLog *log);
	virtual FontDialogFont *DumpInFontDialogFont(Attribute *att);

	virtual const char *LanguageString(int id);
	virtual const char *ScriptString(int id);
	virtual int LanguageId(const char *str, bool create_if_not_found);
	virtual int ScriptId  (const char *str, bool create_if_not_found);

	virtual int RetrieveFontmatrixTags();
	virtual int GetTagId(const char *tag);
	virtual int GetTagCategory(int id);
	virtual const char *GetTagName(int id);
	virtual FontDialogFont *FindFontFromFile(const char *file);

	virtual int AddFavoritesFile(const char *file);
	virtual int RemoveFavoritesFile(const char *file);

	static void InstallFontResourceType(ObjectFactory *factory, ResourceManager *resources);
};


//--------------------------------------- Default Renderer Stuff ---------------------------

typedef FontManager *(*NewFontManagerFunc)();
int SetNewFontManagerFunc(const char *backend);

int SetDefaultFontManager(FontManager *manager);
FontManager *GetDefaultFontManager();



} // namespace Laxkit

#endif

