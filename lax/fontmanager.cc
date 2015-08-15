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
//    Copyright (C) 2004-2012 by Tom Lechner
//


#include <lax/fontmanager.h>
#include <cstdio>

#include <lax/misc.h>
#include <lax/anxapp.h>

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


//--------------------------- FontManager ------------------------------------------


/*! \class FontManager
 * \brief The font manager used by anXApp to simplify keeping track of what fonts are loaded.
 */

FontManager::FontManager()
{}


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

