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


//---------------------------- LaxFont -------------------------------
class LaxFont : public anObject
{
  protected:
	char *family;
	char *style;
	char *fontfile;
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
	virtual const char *Family();
	virtual const char *Style();
	virtual const char *FontFile();
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
	virtual anObject *GetColor() { return color; }
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
};

//--------------------------- FontTag ------------------------------------------
class FontTag
{
  public:
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
	virtual LaxFont *Add(LaxFont *font,int nid) = 0;
	virtual LaxFont *CheckOut(int id) = 0;
	virtual FcConfig *GetConfig();
	virtual FT_Library *GetFreetypeLibrary();
	virtual PtrStack<FontDialogFont> *GetFontList();

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

