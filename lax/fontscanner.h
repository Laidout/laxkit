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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_FONTSCANNER_H
#define _LAX_FONTSCANNER_H


#include <cstdio>

#include <lax/fontmanager.h>
#include <lax/gradientstrip.h>

namespace Laxkit {

enum class FontType : int {
	Unknown = 0,
	WOFF,
	OTF,
	TTF,
	CPAL = (1<<0),
	COLR = (1<<1),
	SVG  = (1<<2)
};

class FontScanner
{
  protected:
	FILE *ff;

  public:
	char *file;

	 //SVG components
	struct SvgEntry {
		unsigned int startglyph; //range of glyphs in specified svg
		unsigned int endglyph;  //..it is possible to have > 1 glyph range in same svg document
		unsigned long offset;  //in svgtable
		unsigned long len;    //from offset in svgtable
	};
	unsigned char *svgtable;
	PtrStack<SvgEntry> svgentries;
	unsigned int svg_offset;
	unsigned int svg_complen;
	unsigned int svg_origlen;

	 //CPAL components
	unsigned int cpal_offset;
	unsigned int cpal_complen;
	unsigned int cpal_origlen;
	GradientStrip *palette;

	 //COLR components
	unsigned int colr_offset;
	unsigned int colr_complen;
	unsigned int colr_origlen;
	PtrStack<ColrGlyphMap> colr_maps;


	FontScanner(const char *nfile = nullptr);
	virtual ~FontScanner();
	virtual bool isWoffFile(const char *maybefile);
	virtual int Scan(int which=0, const char *nfile = nullptr);
	virtual bool Use(const char *nfile);

	virtual bool HasCpal() { return cpal_offset > 0; }
	virtual bool HasColr() { return colr_offset > 0; }
	virtual bool HasSvg()  { return  svg_offset > 0; }

	virtual int ScanCpal();
	virtual int ScanColr();
	virtual int ScanSvg ();

	virtual void SvgDump(const char *directory);
};


} // namespace Laxkit


#endif

