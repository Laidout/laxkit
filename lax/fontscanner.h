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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_FONTSCANNER_H
#define _LAX_FONTSCANNER_H


#include <cstdio>

#include <lax/fontmanager.h>

namespace Laxkit {


class FontScanner
{
  protected:
	FILE *ff;

  public:
	char *file;

	unsigned int svg_offset;
	unsigned int svg_complen;
	unsigned int svg_origlen;

	unsigned int cpal_offset;
	unsigned int cpal_complen;
	unsigned int cpal_origlen;
	Palette *palette;

	unsigned int colr_offset;
	unsigned int colr_complen;
	unsigned int colr_origlen;
	PtrStack<ColrGlyphMap> colr_maps;

	FontScanner(const char *nfile=NULL);
	virtual ~FontScanner();
	virtual bool isWoffFile(const char *maybefile);
	virtual int Scan(int which=0, const char *nfile=NULL);
	virtual bool Use(const char *nfile);

	virtual bool HasCpal() { return cpal_offset>0; }
	virtual bool HasColr() { return colr_offset>0; }
	virtual bool HasSvg()  { return  svg_offset>0; }

	virtual int ScanCpal();
	virtual int ScanColr();
	virtual int ScanSvg ();
};


} // namespace Laxkit


#endif

