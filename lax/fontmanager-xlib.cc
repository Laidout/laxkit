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
//    Copyright (C) 2012 by Tom Lechner
//


#include <lax/fontmanager-xlib.h>
#include <lax/anxapp.h>
#include <cstdio>

#include <lax/lists.cc> // this is necessary to instantiate templates
#include <lax/refptrstack.cc>

static const char hexdigits[17]="0123456789ABCDEF";

#include <iostream>
#define DBG
using namespace std;

namespace Laxkit {


//--------------------------- FontManagerCairo ------------------------------------------
FontManager *newFontManager_xlib()
{ return new FontManagerXlib(); }


//---------------------------- LaxFontXlib -------------------------------

/*! \class LaxFontXlib
 * \brief A LaxFont using Xft.
 *
 * In addition to an XftFont holding the font, LaxFont also contains easy to
 * get to info about the range of characters defined in the font (one continuous range
 * is allowed per LaxFont), the text height, ascent, and descent, and the widths of the 
 * characters. The width is actually separated into 2 width arrays. One has the real
 * character width, which could be 0, and one has the charwidths based on converted the
 * 0 width characters into a hex representation. For instance, the character '\\n' has
 * ascii value 10, and it might be converted to "\0a". This is to aid text editors such
 * as LineEdit.
 *
 * These objects are reference counted via dec_count() and inc_count(). If a dec_count() 
 * results in a count of 0, then <tt>delete this</tt> is called.
 * 
 * \todo read up on Xft, have ability to maintain charwidths[] for multiple ranges,
 *   not just the basic Latin1/small [firstcharinfont + numcharsinfont]. Must be able to
 *   facilitate font subsetting
 */
LaxFontXlib::LaxFontXlib()
{
	charwidths=realcharwidths=NULL;
	textstyle=0;
	numcharsinfont=firstchar=0; 
	font=NULL;
}

//! Constructor. Uses an X Logical Font Description.
LaxFontXlib::LaxFontXlib(Display *dpy,const char *xlfd,int nid)
{
	id=nid;
	if (!id) id=getUniqueNumber();
	charwidths=realcharwidths=NULL;
	textstyle=0;
	cntlchar=0;
	font=NULL;

	font=XftFontOpenName(dpy,0,xlfd);
	//font=XftFontOpenXlfd(dpy,0,xlfd);
	SetupMetrics();
}

/*! Takes over xfs. Calling code must not free it.
 */
LaxFontXlib::LaxFontXlib(XftFont *f,int nid)
{
	font=f;
	id=nid;
	if (!id) id=getUniqueNumber();
	charwidths=realcharwidths=NULL;
	textstyle=0;
	cntlchar=0;
	SetupMetrics();
}

LaxFontXlib::~LaxFontXlib()
{
	if (font && anXApp::app->dpy) XftFontClose(anXApp::app->dpy, font);

	if (charwidths)     delete[] charwidths;
	if (realcharwidths) delete[] realcharwidths;
}

double LaxFontXlib::textheight()
{
	if (font) return font->height;
	return 0;
}

double LaxFontXlib::ascent()
{
	if (font) return font->ascent;
	else return 0;
}

double LaxFontXlib::descent()
{ 
	if (font) return font->descent;
	else return 0;
}

double LaxFontXlib::Resize(double newsize)
{
	cerr << " *** WARNING!! FontManagerXlib::Resize() not implemented!!!"<<endl;
	return textheight();
}


//! Return the character width of ch.
/*! This assumes the charwidth array exists already.
 * If the real character width is 0, and r==0, this returns width of "\9f"
 * or some such. Of course, this function is for latin-1 only.
 *
 * \todo *** this is broken
 */
double LaxFontXlib::charwidth(unsigned long chr,int real,double *width,double *height)
{
	if (!charwidths || !realcharwidths) return 0;

	unsigned char ch=(unsigned char)chr;//***

	if (ch<firstchar || ch>=firstchar+numcharsinfont) {
		if (real) return 0;
		 // return the hexified val of ch (just the number)
		return charwidth(cntlchar,1)+charwidth(hexdigits[ch&15],1)+charwidth(hexdigits[ch>>4&15],1);
	}
	if (real && realcharwidths) return realcharwidths[ch-firstchar];
	return charwidths[ch-firstchar];
}

//! Return 0 success else nonzero error
/*! This fills up charwidths and realcharwidths with appropriate values.
 *
 * \todo *** this is old and basically broken. is all core xlib
 */
int LaxFontXlib::SetupMetrics()
{
	return 1;
//*********this stuff is all old. Is it in any way useful????
//
//	if (charwidths) { delete[] charwidths; charwidths=NULL; }
//	if (realcharwidths) { delete[] realcharwidths; realcharwidths=NULL; }
//
//	*** //is this stuff used anymore??
//	
//	 // ***assumes linear character index, starting close to 0
//	firstchar= fontstruct->min_char_or_byte2;
//	numcharsinfont= fontstruct->max_char_or_byte2 - fontstruct->min_char_or_byte2 + 1;
//	if (fontstruct->min_byte1!=0 || fontstruct->max_byte1!=0) {
//		 // byte1/byte2 setup.. what the fuck?
//		DBG cerr <<"Only Ascii or Latin1 fonts supported.\n";
//		exit(1); //***kind of drastic! just return 1 if fail?
//	}
//
//	//*** should have separate width array for hex chars??
//	charwidths=new int[numcharsinfont];
//	realcharwidths=new int[numcharsinfont];
//	if ((unsigned char)cntlchar<firstchar || (unsigned char) cntlchar>=firstchar+numcharsinfont) cntlchar=firstchar;
//
//	int c;
//	for (c=0; c<numcharsinfont; c++) {
//		charwidths[c]=realcharwidths[c]=0;
//		if (fontstruct->per_char) {
//			//realcharwidths[c]=fontstruct->per_char[c].rbearing-fontstruct->per_char[c].lbearing;
//			realcharwidths[c]=fontstruct->per_char[c].width;
//			if (realcharwidths[c]==0) 
//				realcharwidths[c]= //***check:is this right?
//					fontstruct->per_char[fontstruct->default_char-firstchar].rbearing - 
//					fontstruct->per_char[fontstruct->default_char-firstchar].lbearing;
//		} else { // monospaced
//			realcharwidths[c]=fontstruct->max_bounds.rbearing-fontstruct->max_bounds.lbearing;
//		}
//		charwidths[c]=realcharwidths[c];
//	}
//	if (textstyle&TEXT_CNTL_BANG) {
//		for (c=0; c<numcharsinfont; c++) {
//			if (realcharwidths[c]==0) charwidths[c]=charwidths[(unsigned char)cntlchar-firstchar];
//		}
//	} else if (textstyle&TEXT_CNTL_HEX) {
//		for (c=0; c<numcharsinfont; c++) {
//			if (realcharwidths[c]==0) {
//				charwidths[c]= realcharwidths[(unsigned char)cntlchar-firstchar] + 
//								realcharwidths[(unsigned char)hexdigits[(c&15)]-firstchar] +
//								realcharwidths[(unsigned char)hexdigits[((c>>4)&15)]-firstchar];
//			}
//		}
//	}
//
//	return 0;
}

/*! Some languages change the character based on where letter is in word.
 * \todo This function as it stands is totally worthless and meaningless.
 */
double LaxFontXlib::contextcharwidth(char *start,char *pos,int real,double *width,double *height)
{ 
	return charwidth((unsigned long)(*pos),real, width,height); 
}


//--------------------------- FontManagerXlib ------------------------------------------


/*! \class FontManagerXlib
 * \brief The font manager used by anXApp to simplify keeping track of what fonts are loaded.
 */

FontManagerXlib::FontManagerXlib()
	: RefPtrStack<LaxFont>()
{}



//! Add a font to the fontmananger for which an XftFont is already made.
/*! The manager takes possession of xfs, it should not be deleted by anything else.
 * 
 * If nid<0 then the id is from getUniqueNumber().
 * 
 * Returns the new font object, or NULL on failure.
 */
LaxFont *FontManagerXlib::Add(XftFont *xftfont,int nid)
{
	if (nid<0) nid=getUniqueNumber();
	LaxFont *f=new LaxFontXlib(xftfont,nid);
	push(f);//incs count of f to 2
	f->dec_count();//remove creation count
	return e[n-1];
}

//! Create and return a font based on fcstr, a fontconfig string.
/*! If nid<0 then the id is from getUniqueNumber().
 */
LaxFont *FontManagerXlib::MakeFontFromStr(const char *fcstr, int nid)
{
	if (nid<0) nid=getUniqueNumber();
	LaxFont *f=new LaxFontXlib(anXApp::app->dpy,fcstr,nid);
	return f;
}

//! Add an already created LaxFont to the manager.
/*! If nid<0 then the id is from getUniqueNumber().
 * 
 *
 * If the font is already on the stack, nothing is done.
 *
 * The font's count is incremented.
 * Returns the font object or NULL on error.
 */
LaxFont *FontManagerXlib::Add(LaxFont *font,int nid)
{
	if (!font) return NULL;
	if (nid<0) nid=getUniqueNumber();
	pushnodup(font); //incs count if not already on stack
	return font;
}


//! Add a count of 1 to the LaxFont with the given id, and return the corresponding font object.
/*! Returns the LaxFont object, or NULL if object not found.
 *
 * Please remember that when you are all done with the font, you must decrement its count.
 */
LaxFont *FontManagerXlib::CheckOut(int id)
{
	int c;
	for (c=0; c<n; c++) {
		if (e[c]->id==id) {
			e[c]->inc_count();
			return e[c];
		}
	}
	return NULL;
}

//! Create and return a LaxFont, but do not store it within the fontmanager.
/*! This file and size are passed along to fontconfig.
 * If fontconfig cannot do anything with it, NULL is returned.
 */
LaxFont *FontManagerXlib::MakeFontFromFile(const char *file, double size, int nid)
{
	int screen=0;
	if (size<=0) size=anXApp::app->defaultlaxfont->textheight();
	XftFont *xfont=XftFontOpen(anXApp::app->dpy, screen,
								XFT_FILE, XftTypeString, file,
								XFT_PIXEL_SIZE, XftTypeDouble, size,
								NULL); 
	if (!xfont) return NULL;
	LaxFont *laxfont=new LaxFontXlib(xfont,nid);

	return laxfont;
}

//! Create and return a LaxFont, but do not store it within the fontmanager.
/*! This family, style, and size are passed along to fontconfig.
 * If fontconfig cannot do anything with it, NULL is returned.
 */
LaxFont *FontManagerXlib::MakeFont(const char *family, const char *style, double size, int nid)
{
	if (!family) return NULL;
	int screen=0;
	if (size<=0) size=anXApp::app->defaultlaxfont->textheight();
	XftFont *xfont;
	if (style) {
		xfont=XftFontOpen(anXApp::app->dpy, screen,
								XFT_FAMILY, XftTypeString, family,
								XFT_STYLE,  XftTypeString, style,
								XFT_PIXEL_SIZE,   XftTypeDouble, size,
								NULL); 
	} else {
		xfont=XftFontOpen(anXApp::app->dpy, screen,
								XFT_FAMILY, XftTypeString, family,
								XFT_PIXEL_SIZE,   XftTypeDouble, size,
								NULL); 
	}
	if (!xfont) return NULL;
	LaxFont *laxfont=new LaxFontXlib(xfont,nid);

	return laxfont;
}


} // namespace Laxkit
