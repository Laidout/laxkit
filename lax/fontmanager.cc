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


#include <lax/fontmanager.h>
#include <cstdio>

#include <lax/misc.h>
#include <lax/anxapp.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/language.h>
#include <lax/colors.h>

#include <iostream>
using namespace std;
#define DBG 


//include various backends...
#ifdef LAX_USES_CAIRO
#include <lax/fontmanager-cairo.h>
#endif 

#ifdef LAX_USES_XLIB_RENDER
#include <lax/fontmanager-xlib.h>
#endif

#ifdef LAX_USES_GL
//#include <lax/fontmanager-gl.h>
#endif

#ifdef LAX_USES_SQLITE
#include <sqlite3.h>
#endif


namespace Laxkit {


//------------------------------ GlyphPlace -----------------------------------
/*! \struct GlyphPlace
 *
 * Class to hold info about specific glyphs for a string of text.
 * See also Displayer::glyphsout().
 */

GlyphPlace::GlyphPlace()
{
	index=0;
	cluster=0;
	numchars=1;

	x_advance=y_advance=x_offset=y_offset=x=y=0;

	styleid=0;
	has_color=0;
}

GlyphPlace::~GlyphPlace()
{
}


//----------------------------- Extra color font support things -------------------------------------

/*! \class ColrGlyphMap
 * Aids workaround for rendering CPAL/COLR based opentype fonts.
 */

ColrGlyphMap::ColrGlyphMap(int initial, int n, int *g, int *cols)
{
	numglyphs = n;
	if (n) {
		glyphs = new int[n];
		memcpy(glyphs, g, n*sizeof(int));

		colors = new int[n];
		memcpy(colors, cols, n*sizeof(int));
	} else {
		glyphs = nullptr;
		colors = nullptr;
	}
}

ColrGlyphMap::~ColrGlyphMap()
{
	delete[] glyphs;
	delete[] colors;
}

//class ColrTable
//{
//  public:
//	NumStack<ColrGlyphMap> maps;
//};

/*! \class SvgMap
 * One per glyph, contains original svg and cached render.
 */
class SvgMap
{
  public:
	int glyph;
	//VectorImage *svg;
	double img_fsize;
	LaxImage *img;
};


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
	id        = getUniqueNumber();
	textstyle = 0;
	cntlchar  = '\\';

	family    = nullptr;
	style     = nullptr;
	psname    = nullptr;
	fontfile  = nullptr;
	fontindex = 0;

	cached_blob_file = nullptr;
	hb_font   = nullptr;
	hb_face   = nullptr;
	hb_blob   = nullptr;

	color     = nullptr;
	nextlayer = nullptr;
}

LaxFont::~LaxFont()
{
	delete[] family;
	delete[] style;
	delete[] fontfile;
	delete[] psname;

	if (color)     color->dec_count();
	if (nextlayer) nextlayer->dec_count();

	delete[] cached_blob_file;
	if (hb_font) hb_font_destroy(hb_font);
	if (hb_face) hb_face_destroy(hb_face);
	if (hb_blob) hb_blob_destroy(hb_blob);
}


/*! \fn int LaxFont::charwidth(unsigned long chr,int real,double *width=nullptr,double *height=nullptr)
 * \brief Return the character width of ch.
 *
 * If the real character width is 0, and r==0, this should return width of "\9f" or "?" or some default
 * replacement character.
 */


/*! \fn int contextcharwidth(char *start,char *pos,int real,double *width=nullptr,double *height=nullptr)
 * \brief Like charwidth(), but return width of character in context of a whole string. In some languages, it's different.
 */


/*! \fn  double textheight()
 * \brief Usually, but NOT ALWAYS, ascent plus descent. It is the default distance between baselines.
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

/*! Convenince function to return several characteristics at once.
 */
double LaxFont::Extent(const char *str,int len, double *w, double *h, double *asc, double *desc)
{
	double ww = Extent(str,len);
	if (w) *w = ww;
	if (h) *h = textheight();
	if (asc) *asc = ascent();
	if (desc) *desc = descent();
	return ww;
}


LaxFont *LaxFont::duplicate()
{
	FontManager *fontmanager = GetDefaultFontManager();

	LaxFont *old = this, *newlayer, *newfont = nullptr;
	while (old) {
		newlayer = fontmanager->MakeFontFromFile(old->FontFile(), old->Family(), old->Style(), textheight(), -1);
		if (!newlayer) break;
		newlayer->CopyVariations(old);

		if (!newfont) newfont = newlayer;
		else newfont->AddLayer(-1, newlayer);

		old = old->nextlayer;
	}

	if (color) {
		anObject *ncolor = color->duplicate(nullptr);
		if (ncolor) {
			newfont->SetColor(ncolor);
			ncolor->dec_count();
		}
	}

	return newfont;
}

int LaxFont::CopyVariations(LaxFont *from_this)
{
	if (!from_this) return 0;
	user_features = from_this->user_features;
	variation_data = from_this->variation_data;
	return user_features.size() + variation_data.size();
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

/*! For multicolor font layers. which must be in range [0..Layers()], or nullptr is returned.
 */
LaxFont *LaxFont::Layer(int which)
{
	if (which<0 || which>=Layers()) return nullptr;

	LaxFont *f=this;
	do {
		if (which==0) return f;
		f=f->nextlayer;
		which--;
	} while (f);

	return nullptr;
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
 * If popped_ret!=nullptr, then do not dec_count the removed layer, but return it. Otherwise, the popped
 * layer is dec_counted, even if it was the head.
 *
 * This lets you easily delete the first layer with something like:
 *  font = font->RemoveLayer(0, nullptr), and not have to delete it yourself.
 *
 * If only one layer or which is out of bounds, nothing is done and this is returned. popped_ret gets
 * set to nullptr in this case.
 */
LaxFont *LaxFont::RemoveLayer(int which, LaxFont **popped_ret)
{
	if (!nextlayer) { return this; if (popped_ret) *popped_ret=nullptr; }
	if (which<0 || which>=Layers()) { return this; if (popped_ret) *popped_ret=nullptr; }

	if (which==0) {
		LaxFont *f=nextlayer;
		nextlayer=nullptr;
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
	of->nextlayer=nullptr;

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
	LaxFont *popped=nullptr;

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
	nextlayer=nullptr;
}

int LaxFont::SetColor(anObject *ncolor)
{
	if (color) color->dec_count();
	color=ncolor;
	if (color) color->inc_count();
	return 0;
}

Attribute *LaxFont::dump_out_atts(Attribute *att, int what, DumpContext *context)
{
	if (!att) att=new Attribute;

	att->push("fontsize", textheight());
	att->push("Msize", Msize());

	Attribute *att2;

	Palette *palette=dynamic_cast<Palette*>(GetColor());
	Color   *color  =dynamic_cast<Color*>  (GetColor()); 

	if (palette) {
		att2=att->pushSubAtt("palette");
		palette->dump_out_atts(att2, what, context);
	}
	if (color) {
		att2=att->pushSubAtt("color");
		color->dump_out_atts(att2, what, context);
	}

	for (int c=0; c<Layers(); c++) {
		LaxFont *ff = Layer(c);

		if (Layers()==1) att2=att;
		else {
			att2=att->pushSubAtt("layer");
		}

		att2->push("fontfile"  ,ff->FontFile()); 
		att2->push("fontfamily",ff->Family());
		att2->push("fontstyle" ,ff->Style());
    }

	return att;
}


//-------------------------------------- FontDialogFont -------------------------------------
/*! \class FontDialogFont
 * \brief Describes a font as dealt with in a FontDialog.
 */

FontDialogFont::FontDialogFont(int nid, const char *nfile, const char *nfamily, const char *nstyle)
{
	id = nid;
	if (id <= 0) id = getUniqueNumber();

	name    = nullptr;
	psname  = nullptr;
	format  = -1;
	preview = nullptr;

	family    = newstr(nfamily);
	style     = newstr(nstyle);
	file      = newstr(nfile);
	index     = 0;
	has_color = false;

	favorite  = 0;

	fc_pattern = nullptr;  // assume fontmanager->fontlist gets destroyed AFTER this
}

FontDialogFont::~FontDialogFont()
{
    delete[] name;
    delete[] psname;
    delete[] family;
    delete[] style;
    delete[] file;
    if (preview) preview->dec_count();
}

/*! Return whether both family and style caselessly match.
 */
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
	return tags.HasTag(tag_id);
}

/*! Returns the new number of tags.
 */
int FontDialogFont::AddTag(int tag_id)
{
	tags.InsertTag(tag_id);
	return tags.NumberOfTags();
}

void FontDialogFont::RemoveTag(int tag_id)
{
	tags.RemoveTag(tag_id);
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

	if (strcasestr(psname, "Villa")) {
		cerr <<" BLAH"<<endl;
	}

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

/*! When variations_state == -1, then scan the font with Harfbuzz to determine
 * variable type features and axes. This requires that file be a valid font file.
 *
 * Returns true for success and any features/axes found, otherwise return false
 * and features/axes still not determined.
 */
bool FontDialogFont::UpdateVariations()
{
	if (variations_state >= 0) return true;
	if (variations_state != -1) return false; // other than -1, we don't know what to do by default!

	if (isblank(file)) {
		DBG cerr << " *** missing file for font!! " <<(family ? family : "null") << endl;
		return false;
	}


	hb_blob_t *blob = hb_blob_create_from_file(file);
	hb_face_t *face = hb_face_create(blob, index);
	
	// determine adjustable axes
	unsigned int num_axes = hb_ot_var_get_axis_count(face);
	axes_array.resize(num_axes);
	if (num_axes) {
		hb_ot_var_axis_info_t *arr = axes_array.data();
		hb_ot_var_get_axis_infos(face, 0, &num_axes, arr);
		for (unsigned int c = 0; c < num_axes; c++) {
			unsigned int text_len = hb_ot_name_get_utf8(face, axes_array[c].name_id, 0, 0, nullptr);
			if (text_len) {
				text_len++;
				char text[text_len];
				hb_ot_name_get_utf8(face, axes_array[c].name_id, 0, &text_len, text);
				axes_names.push_back(text);

				DBG cerr << "named axis: "<<text<<endl;
			}
		}
	}


	// determine all features
	unsigned int num_features = 0;
	unsigned int num_scripts = hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, nullptr, nullptr);
	std::vector<hb_tag_t> scripts(num_scripts + 1);
	
	hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, 0, &num_scripts, scripts.data());

	char tag[5];
	tag[4] = '\0';

	for (unsigned int c = 0; c < num_scripts; c++) {
        unsigned int num_languages = hb_ot_layout_script_get_language_tags(face, HB_OT_TAG_GSUB, c, 0, nullptr, nullptr);
        for (unsigned int l = 0; l < num_languages; l++) {
			unsigned int nfeatures = hb_ot_layout_language_get_feature_tags(face, HB_OT_TAG_GSUB, c, l, 0, nullptr, nullptr);
			hb_tag_t features[nfeatures + 1];
			hb_ot_layout_language_get_feature_tags(face, HB_OT_TAG_GSUB, c, l, 0, &nfeatures, features);

			for (unsigned int f = 0; f < nfeatures; f++) {
				tag[0] = (features[f]>>24)&0xff;
				tag[1] = (features[f]>>16)&0xff;
				tag[2] = (features[f]>>8 )&0xff;
				tag[3] = (features[f]    )&0xff;
				// each language might repeat a tag. check to not add dups..
				// at this point we don't care about tag-language correspondence
				unsigned int s = 0;
				for (s = 0; s < feature_tags.size(); s++) {
					if (feature_tags[s] == tag) break;
				}
				if (s == feature_tags.size()) {
					feature_tags.push_back(Utf8String(tag));
					num_features++;
				}
			}
        }
	}

	// retrieve named axis instances
	unsigned int num_named = hb_ot_var_get_named_instance_count(face);
	if (num_named) {
		for (unsigned int c = 0; c < num_named; c++) {
			hb_ot_name_id_t subfamily_name_id = hb_ot_var_named_instance_get_subfamily_name_id(face, c);
			hb_ot_name_id_t ps_name_id = hb_ot_var_named_instance_get_postscript_name_id(face, c);
            unsigned int coords_length = 0;
			unsigned int axes_count = hb_ot_var_named_instance_get_design_coords(face, c,
                                &coords_length,
                                nullptr);

			unsigned int text_len = hb_ot_name_get_utf8(face, subfamily_name_id, 0, 0, nullptr);
			if (text_len) {
				text_len++;
				char subfamily_text[text_len];
				hb_ot_name_get_utf8(face, subfamily_name_id, 0, &text_len, subfamily_text);
				DBG cerr << "named instance subfamily: "<<subfamily_text<<endl;
			}
			text_len = hb_ot_name_get_utf8(face, ps_name_id, 0, 0, nullptr);
			if (text_len) {
				text_len++;
				char psname_text[text_len];
				hb_ot_name_get_utf8(face, ps_name_id, 0, &text_len, psname_text);
				DBG cerr << "named instance ps name: "<<psname_text<<endl;
			}

			if (axes_count) {
				float coords[axes_count];
				coords_length = axes_count;
				unsigned int ret = hb_ot_var_named_instance_get_design_coords(face, c,
                                &coords_length,
                                coords);
				DBG cerr << "returned axis coords: "<<coords_length<<" "<<axes_count<<" "<<ret<<endl;
			}
			//*** do something with these!!
		}
	}

	// cleanup
	hb_face_destroy(face);
	hb_blob_destroy(blob);

	if (num_axes == 0 && num_features == 0) variations_state = 0;
	else variations_state = 1;
	return true;
}

/*! Return index, or -1 if not found. */
int FontDialogFont::FindAxisIndex(const char *name)
{
	for (unsigned int c = 0; c < axes_names.size(); c++) {
		if (axes_names[c].Equals(name)) return c;
	}
	return -1;
}


//-------------------------------------- FontDialogFont -------------------------------------
LayeredDialogFont::LayeredDialogFont(int nid)
  : FontDialogFont(nid)
{ 
	palette = nullptr;
}


//--------------------------- FontTag ------------------------------------------
/*! \class FontTag
 * Tag definitions for use in FontDialogFont
 */

FontTag::FontTag(int nid, int ntagtype, const char *ntag)
{
	id = nid;
	if (id <= 0) id = getUniqueNumber();
	tagtype = ntagtype;
	tag = newstr(ntag);
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
	fcconfig   = nullptr;
	ft_library = nullptr;
}

FontManager::~FontManager()
{
	if (fcconfig)   FcConfigDestroy(fcconfig);
	if (ft_library) FT_Done_FreeType(*ft_library);
}

FT_Library *FontManager::GetFreetypeLibrary()
{
	if (!ft_library) {
		ft_library = new FT_Library;
		FT_Error ft_error = FT_Init_FreeType (ft_library);

		if (ft_error) {
			cerr <<"Could not initialize freetype!!"<<endl;
			return nullptr;
		}

	}
	
	return ft_library;
}

FcConfig *FontManager::GetConfig()
{
	if (fcconfig) return fcconfig;

     // Initialize FontConfig library.
	FcInit(); //does nothing if FcInit() already called somewhere else
	fcconfig = FcInitLoadConfigAndFonts();

	return fcconfig;
}

/*! for a qsort()
 */
int cmp_fontinfo_name(const void *f1p, const void *f2p)
{
	FontDialogFont *f1 = *((FontDialogFont**)f1p);
	FontDialogFont *f2 = *((FontDialogFont**)f2p);

	if (!f1->name) return -1;
	if (!f2->name) return 1;
	return strcmp(f1->name, f2->name);
}

/*! for a qsort()
 */
int cmp_fontinfo_psname(const void *f1p, const void *f2p)
{
	FontDialogFont *f1 = *((FontDialogFont**)f1p);
	FontDialogFont *f2 = *((FontDialogFont**)f2p);

	if (!f1->psname) return -1;
	if (!f2->psname) return 1;
	return strcmp(f1->psname, f2->psname);
}

/*! for a qsort()
 */
static int cmp_fontinfo_file(const void *f1p, const void *f2p)
{
	FontDialogFont *f1 = *((FontDialogFont**)f1p);
	FontDialogFont *f2 = *((FontDialogFont**)f2p);

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

	const char *format = nullptr;
    FontDialogFont *f = nullptr;
    FcFontSet *fontset = FcConfigGetFonts(fcconfig, FcSetSystem); //"This font set is owned by the library and must not be modified or freed"

    tags.push(new FontTag(-1, FontTag::TAG_Favorite, _("Favorites")));

    for (int c = 0; c < fontset->nfont; c++) {
         // Usually, for each font family, there are several styles
         // like bold, italic, etc.
        result = FcPatternGet(fontset->fonts[c],FC_FAMILY,0,&v);
        if (result != FcResultMatch) continue;

        f = new FontDialogFont(c);
        f->fc_pattern = fontset->fonts[c];

        makestr(f->family, (const char *)v.u.s);

        result = FcPatternGet(fontset->fonts[c],FC_STYLE,0,&v);
        if (result == FcResultMatch) makestr(f->style, (const char *)v.u.s);

        result = FcPatternGet(fontset->fonts[c],FC_POSTSCRIPT_NAME,0,&v);
        if (result == FcResultMatch) makestr(f->psname, (const char *)v.u.s);

        result = FcPatternGet(fontset->fonts[c],FC_FILE,0,&v);
        if (result == FcResultMatch) makestr(f->file, (const char *)v.u.s);

        result = FcPatternGet(fontset->fonts[c],FC_INDEX,0,&v);
        if (result == FcResultMatch) f->index = v.u.i;

        result = FcPatternGet(fontset->fonts[c],FC_FONTFORMAT,0,&v);
        if (result == FcResultMatch) format = (const char *)v.u.s; else format = nullptr;
		if (format != nullptr) {
			int id = GetTagId(format);
			if (id == -1) {
				tags.push(new FontTag(-1, FontTag::TAG_Format, format)); //puts at end 
				f->id = tags.e[tags.n-1]->id;
			} else f->id = id;
			if (id > -1) f->AddTag(id);
		}

		result = FcPatternGet(fontset->fonts[c], FC_COLOR, 0, &v);
		if (result == FcResultMatch) {
			f->has_color = v.u.b;

			if (f->has_color) {
				int tag_id = GetTagId(_("Color"));
				if (tag_id == -1) {
					tags.push(new FontTag(-1, FontTag::TAG_Color, _("Color"))); //puts at end 
					tag_id = tags.e[tags.n-1]->id;
				}
				if (tag_id > -1) f->AddTag(tag_id);
			}
		}

		result = FcPatternGet(fontset->fonts[c], FC_VARIABLE, 0, &v);
		if (result == FcResultMatch) {
			//DBG if (v.u.b) cerr << "TODO! implement FC_VARIABLE retrieval with fontconfig: " << v.u.b << endl;
			if (v.u.b) {
				int tag_id = GetTagId(_("Variable"));
				if (tag_id == -1) {
					tags.push(new FontTag(-1, FontTag::TAG_Other, _("Variable"))); //puts at end 
					tag_id = tags.e[tags.n-1]->id;
				}
				if (tag_id > -1) f->AddTag(tag_id);
			}
		}

		result = FcPatternGet(fontset->fonts[c], FC_FONT_VARIATIONS, 0, &v);
		if (result == FcResultMatch) {
			// *** this is apparently for setting query patterns somewhere else? never returns anything
			DBG cerr << "TODO! implement FC_FONT_VARIATIONS retrieval with fontconfig: " << (const char *)v.u.s << endl;
		}

		result = FcPatternGet(fontset->fonts[c], FC_FONT_FEATURES, 0, &v);
		if (result == FcResultMatch) {
			// *** this is apparently for setting query patterns somewhere else? never returns anything
			DBG cerr << "TODO! implement FC_FONT_FEATURES retrieval with fontconfig: " << (const char *)v.u.s << endl;
		}

		f->UseFamilyStyleName();
		//f->UsePSName();

        //FC_OUTLINE
        //FC_SCALABLE
        //FC_LANG -> string of languages like "en|es|bs|ch"
        //FC_CAPABILITY -> has "otlayout:" stuff
        //FC_FONTFORMAT
        //FC_FONT_FEATURES

        DBG cerr <<c<<", found font: Family,style,file: "
        DBG      <<(f->family ? f->family : "null") << ", "
        DBG      <<(f->style ? f->style : "null") << ", "
        DBG      <<(f->file ? f->file : "null") << endl;

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
 * Returns the new font object, or nullptr on failure.
 */


/*! \fn LaxFont *FontManager::CheckOut(int id)
 *  \brief Add a count of 1 to the LaxFont with the given id, and return the corresponding font object.
 *  Returns the LaxFont object, or nullptr if object not found.
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
	return nullptr;
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

/*! nullptr for unknown id. 0 is default language.
 */
const char *FontManager::LanguageString(int id)
{
	cerr <<" *** need to implement FontManager language+script stuff!!"<<endl;
	return nullptr;
}

/*! nullptr for unknown id. 0 is default.
 */
const char *FontManager::ScriptString(int id)
{
	cerr <<" *** need to implement FontManager language+script stuff!!"<<endl;
	return nullptr;
}

int FontManager::LanguageId(const char *str, bool create_if_not_found)
{
	cerr <<" *** need to implement FontManager language+script stuff!!"<<endl;
	return -1;
}

int FontManager::ScriptId  (const char *str, bool create_if_not_found)
{
	cerr <<" *** need to implement FontManager language+script stuff!!"<<endl;
	return -1;
}

/*! Does binary search in fonts, which should be sorted by file already.
 */
FontDialogFont *FontManager::FindFontFromFile(const char *file)
{
	if (!fonts.n || !file) return nullptr;

	 //binary search, assume fonts is sorted by file
	int s=0, e=fonts.n-1, mid, cmp; 

	if (!strcmp(file, fonts.e[s]->file)) return fonts.e[s];
	if (!strcmp(file, fonts.e[e]->file)) return fonts.e[e];;

	do {
		mid=(s+e)/2;
		if (mid==e || mid==s) return nullptr; //already checked

		cmp=strcmp(file, fonts.e[mid]->file);
		if (cmp==0) return fonts.e[mid];
		if (cmp<0) {
			e=mid;
		} else {
			s=mid;
		}
	} while (s!=e);

	return nullptr;
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

	sqlite3 *db=nullptr;

	try {
		if (!S_ISREG(file_exists(dbfile,1,nullptr))) throw(1);

		int error = sqlite3_open_v2(dbfile, &db, SQLITE_OPEN_READONLY, nullptr);
		if (error != SQLITE_OK) {
			cerr <<"Couldn't open database "<<dbfile<<": "<<sqlite3_errmsg(db)<<endl;
			throw (2);
		}


		 //first read in the tag names
		DBG cerr <<"Get tag names..."<<endl;

		const char *sqlstr="SELECT tag FROM fontmatrix_tags GROUP BY tag";
		sqlite3_stmt *stmt=nullptr;
		const char *strremain=nullptr;

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
						tags.push(new FontTag(-1, FontTag::TAG_Fontmatrix, tag)); //puts at end 

					} else for (int c=0; c<tags.n; c++) {
						cmp=strcasecmp(tag, tags.e[c]->tag);

						if (cmp<0) {
							tags.push(new FontTag(-1, FontTag::TAG_Fontmatrix, tag));
							break;
						} else if (cmp==0) break; //tag exists already!

						if (cmp>0 && c==tags.n-1) tags.push(new FontTag(-1, FontTag::TAG_Fontmatrix, tag)); //puts at end 
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
		stmt=nullptr;
		strremain=nullptr;
		error = sqlite3_prepare_v2(db, sqlstr, strlen(sqlstr)+1, &stmt, &strremain);
		FontDialogFont *font=nullptr;

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
					sqlite3_stmt *stmt2=nullptr;
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

					//DBG cerr <<endl;
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

/*! Return 0 if added. Return 1 if cannot add for some reason. Return -1 for already there.
 */
int FontManager::AddDir(const char *ndir)
{
	return dirs.AddDir(ndir, -1);
}

/*! Read in extra config for fonts. This extra config consists of defining layered fonts, and adding meta on top
 * of what is known by fontconfig.
 *
 * <pre>
 *   font_dir  /path/to/custom/font/dir/not/known/to/fontconfig  #directories to add to base fontconfig on initialization
 *
 *   font_alias "My Goto Font" file://path/to/it
 *   font_alias "My Goto Font" "font string id"
 *
 *   font_substitute
 *     replace "sans"
 *     with /path/to/actual/font/to/use
 *
 *   font file://file/to/grab/a/font/definition/from  #could be an svg font, lax formatted layered font, truetype, etc
 *
 *   font
 *     id "string id" //for layered fonts, they are listed with filename "layered:[fontid]"
 *     tags "Blah blah" blah
 *     favorite 4
 *
 *     layer
 *       file /path/to/it
 *       index 0 #index within file for this particular font, when file is a collection
 *       family "Font Family"   #family, style, and psname should be considered hints when actual file exists
 *       style  "Font Style"
 *       psname "Font Postscript Name"
 *     layer
 *       ...
 *  meta
 *    "font id"
 *      tags "extra user tags"
 *      favorite 2
 *      alias "some other name"
 *      exclusive_alias "Use this name instead of the default"
 * </pre>
 */ 
int FontManager::DumpInFontList(const char *file, ErrorLog *log)
{
	FILE *f=fopen(file, "r");
	if (!f) return 1;

	const char *name, *value;


	Attribute att;
	att.dump_in(file);

	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;

		if (!strcmp(name, "font_dir")) {
			AddDir(value);

		} else if (!strcmp(name, "font")) {
			FontDialogFont *f=DumpInFontDialogFont(att.attributes.e[c]);
			if (f) fonts.push(f);

		} else if (!strcmp(name, "meta")) {
			// ***

		} else if (!strcmp(name, "font_substitute")) {
			// ***

		} else if (!strcmp(name, "font_alias")) {
			// ***

		}
	}

	return 0;
}

FontDialogFont *FontManager::DumpInFontDialogFont(Attribute *att)
{
	int layer=0;
	const char *file=nullptr, *family=nullptr, *style=nullptr, *id=nullptr;
	//const char *tagstr=nullptr;

	char *name=nullptr;
	char *value=nullptr;

	FontDialogFont *font=nullptr;
	LayeredDialogFont *lfont=nullptr;

	for (int c2=0; c2<att->attributes.n; c2++) {
		name= att->attributes.e[c2]->name;
		value=att->attributes.e[c2]->value;

        if (!strcmp(name,"id")) {
			id=value;

		} else if (!strcmp(name,"tags")) {
			//tagstr = value;

		} else if (!strcmp(name,"layer")) {
			if (!lfont) {
				lfont = new LayeredDialogFont();

				int lid=GetTagId(_("Layered"));
				if (lid==-1) {
					tags.push(new FontTag(-1, FontTag::TAG_Format, _("Layered"))); //puts at end 
					lid=tags.e[tags.n-1]->id;
				}
				lfont->format = lid;
			}

			layer++;
			family=style=file=nullptr;
			char colorname[20];
			sprintf(colorname,"fg%d",layer);

			for (int c3=0; c3<att->attributes.e[c2]->attributes.n; c3++) {
				name= att->attributes.e[c2]->attributes.e[c3]->name;
				value=att->attributes.e[c2]->attributes.e[c3]->value;

				if (!strcmp(name,"fontfile")) {
					file=value;

				} else if (!strcmp(name,"fontfamily")) {
					family=value;

				} else if (!strcmp(name,"fontstyle")) {
					style=value;

				} else if (!strcmp(name,"color")) {
					double co[5];
					if (SimpleColorAttribute(value, co, nullptr)==0) {
						if (!lfont->palette) lfont->palette = GradientStrip::newPalette();
						//lfont->palette->AddRGBA(colorname, co[0]*255, co[1]*255, co[2]*255, co[3]*255, 255);
						lfont->palette->AddColor(0, co[0], co[1], co[2], co[3], colorname);
					}
				}
			}
			
			FontDialogFont *fnt = FindFontFromFile(file);
			if (fnt) lfont->layers.push(fnt,0);
			else lfont->layers.push(new FontDialogFont(-1, file, family, style)); 

       } else if (!strcmp(name,"fontfile")) {
            file=value;

        } else if (!strcmp(name,"fontfamily")) {
            family=value;

        } else if (!strcmp(name,"fontstyle")) {
            style=value;
		}
	}

	if (lfont) {
		if (id) makestr(lfont->name, id);
		char *ff=newstr(".layered:"); //<- filename like this so it alphabetizes to one side of regular files
		appendstr(ff, lfont->name);
		delete[] lfont->file;
		lfont->file=ff;
		return lfont;
	}

	if (!file) return nullptr;

	font = FindFontFromFile(file);
	if (!font) {
		font = new FontDialogFont();
		makestr(font->file, file);
		makestr(font->family, family);
		makestr(font->style, style);
	}

	return font;
}

LaxFont *FontManager::dump_in_font(Attribute *att, DumpContext *context)
{
	LaxFont *newfont=nullptr;
	const char *name, *value;
	const char *file=nullptr, *family=nullptr, *style=nullptr;
	Palette *palette=nullptr;
	int layer=0;
	double fontsize=1;

	value = att->value;
	if (value && !strncmp(value, "resource:", 9)) {
		value += 9;
		while (isspace(*value)) value++;

		LaxFont *font = dynamic_cast<LaxFont*>(anXApp::app->resourcemanager->FindResource(value, "Font"));
		if (!font) {
			if (context && context->log) context->log->AddError(_("Missing font resource!"));
		}
		return font;
	}

	for (int c2=0; c2<att->attributes.n; c2++) {
		name= att->attributes.e[c2]->name;
		value=att->attributes.e[c2]->value;

		if (!strcmp(name,"layer")) {
			family=style=file=nullptr;
			layer++;
			char cname[20];
			sprintf(cname,"fg%d",layer);

			for (int c3=0; c3<att->attributes.e[c2]->attributes.n; c3++) {
				name= att->attributes.e[c2]->attributes.e[c3]->name;
				value=att->attributes.e[c2]->attributes.e[c3]->value;

				if (!strcmp(name,"fontfile")) {
					file=value;

				} else if (!strcmp(name,"fontfamily")) {
					family=value;

				} else if (!strcmp(name,"fontstyle")) {
					style=value;

				} else if (!strcmp(name,"color")) {
					double co[5];
					if (SimpleColorAttribute(value, co, nullptr)==0) {
						if (!palette) palette = GradientStrip::newPalette();
						//palette->AddRGBA(cname, co[0]*255, co[1]*255, co[2]*255, co[3]*255, 255);
						palette->AddColor(0, co[0], co[1], co[2], co[3], cname);
					}
				}
			}

			LaxFont *newlayer = MakeFontFromFile(file, family,style,fontsize,-1);
			if (!newfont) newfont=newlayer;
			else newfont->AddLayer(newfont->Layers(), newlayer);

		} else if (!strcmp(name,"fontfile")) {
			file=value;

		} else if (!strcmp(name,"fontfamily")) {
			family=value;

		} else if (!strcmp(name,"fontstyle")) {
			style=value;

		} else if (!strcmp(name,"fontsize")) {
			DoubleAttribute(value,&fontsize); 
		} 
	} 

	if (!newfont) newfont=MakeFontFromFile(file, family, style, fontsize, -1);
	if (newfont && palette) {
		newfont->SetColor(palette);
		palette->dec_count();
	} else if (palette) palette->dec_count();

	return newfont;
}


/*! Return 1 for added, 2 for already there, else 0 for instance not found or null file. */
int FontManager::AddFavoritesFile(const char *file)
{
	if (isblank(file)) return 0;

	for (int c = 0; c < favorites_files.n; c++) {
		if (favorites_files.e[c].Equals(file)) {
			return 2;
		}
	}

	favorites_files.push(Utf8String(file));
	cerr << " *** MUST IMPLEMENT REBUILD FAVORITE FONTS" <<endl;
	return 1;
}


/*! Return 1 for success, else 0 for instance not found or null file. */
int FontManager::RemoveFavoritesFile(const char *file)
{
	if (isblank(file)) return 0;

	for (int c = 0; c < favorites_files.n; c++) {
		if (favorites_files.e[c].Equals(file)) {
			favorites_files.remove(c);
			cerr << " *** MUST IMPLEMENT REBUILD FAVORITE FONTS" <<endl;
			return 1;
		}
	}

	return 0;
}



//--------------------------------------- Default FontManager Stuff ---------------------------

//! There can be only one (default displayer).
static FontManager *fontmanager = nullptr;

/*! \typedef FontManager *NewFontManagerFunc(aDrawable *w);
 * \brief Function type to create new FontManager objects.
 */

//! The default FontManager "constructor".
NewFontManagerFunc newFontManager = nullptr;



//! Set the default "constructor" for FontManager objects.
/*! If func==nullptr, then use the default, checks for cairo, then for xlib.
 * Return 0 for success, or 1 for unable to set to non-null.
 */
int SetNewFontManagerFunc(const char *backend)
{
	NewFontManagerFunc func=nullptr;
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

#ifdef LAX_USES_XLIB_RENDER
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
    if (!newFontManager) SetNewFontManagerFunc(nullptr);
    if (!fontmanager && newFontManager) fontmanager=newFontManager();
    return fontmanager;
}






} // namespace Laxkit

