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

#include <lax/fontscanner.h>

#include <iostream> 
#include <cstring>
#include <zlib.h> 
#include <cstdio>

#include <lax/language.h>
#include <lax/strmanip.h>


//template implementation:
#include <lax/lists.cc>


#define DBG



using namespace std;


namespace Laxkit {

//---------------------------- FontScanner ------------------------------------

/*! \class FontScanner
 * Object that can scan font files for SVG, colr, and cpal tables.
 * Note that FreeType since 2.10 can natively handle CPAL/COLR, so scanning for
 * those might be obsolete here as time goes on. However SVG still can use CPAL,
 * and so far it appears unlikely svg parsing will become a part of FreeType.
 *
 * The CPAL table is converted into a GradientStrip.
 *
 * The COLR table is turned into a stack of ColrGlyphMap.
 *
 * The SVG table is turned into a stack of FontScanner::SvgEntry objects, which
 * detail ranges of glyphs encoded within svg documents embedded in svgtable.
 * Each SvgEntry specifies one range. Note that there may be multiple ranges,
 * thus multiple SvgEntry objects, that refer to the same svg document
 * in svgtable. It is up to other code to convert these char strings into
 * usable graphics objects.
 */


FontScanner::FontScanner(const char *nfile)
{
    svgtable    = nullptr;
    svg_offset  = 0;
    svg_complen = 0; //compressed length of svg table in the file
    svg_origlen = 0; //length of SVG table as reported by the file, this many + 1 bytes stored in buffer svgtable,

    cpal_offset  = 0;
    cpal_complen = 0;
    cpal_origlen = 0;

    colr_offset  = 0;
    colr_complen = 0;
    colr_origlen = 0;

    palette = nullptr;
    file    = newstr(nfile);
    ff      = nullptr;
}

FontScanner::~FontScanner()
{
	if (palette) palette->dec_count();
	if (ff) fclose(ff);
	delete[] svgtable;
	delete[] file;
}

/*! Set current file to nfile, and return isWoffFile(nfile).
 */
bool FontScanner::Use(const char *nfile)
{
	makestr(file, nfile);
	return isWoffFile(file);
}

/*! Checks, but does not make file the current file.
 */
bool FontScanner::isWoffFile(const char *maybefile)
{
	//should check the wOFF start, and file size.. should be good indicator..

    FILE *f = fopen(maybefile, "r");
    if (!f) return false;

    char buffer[45];
    buffer[44] = '\0';
    fread(buffer, 1, 44, f);  // reads in whole header
    fclose(f);

    if (!strncmp(buffer, "wOFF", 4)) return true;
    return false;
}

//WOFFHeader	    File header with basic font type and version, along with offsets to metadata and private data blocks.
//  0 UInt32  signature       0x774F4646 'wOFF'
//  4 UInt32  flavor          The "sfnt version" of the input font. usually 0x0100 or "OTTO", but "true" sometimes used
//  8 UInt32  length          Total size of the WOFF file.
// 12 UInt16  numTables       Number of entries in directory of font tables.
// 14 UInt16  reserved        Reserved; set to zero.
// 16 UInt32  totalSfntSize   Total size needed for the uncompressed font data, including the sfnt header, directory, and font tables (including padding).
// 20 UInt16  majorVersion    Major version of the WOFF file.
// 22 UInt16  minorVersion    Minor version of the WOFF file.
// 24 UInt32  metaOffset      Offset to metadata block, from beginning of WOFF file.
// 28 UInt32  metaLength      Length of compressed metadata block.
// 32 UInt32  metaOrigLength  Uncompressed size of metadata block.
// 26 UInt32  privOffset      Offset to private data block, from beginning of WOFF file.
// 40 UInt32  privLength      Length of private data block.
//
//TableDirectory    Directory of font tables, indicating the original size, compressed size and location of each table within the WOFF file.
//                  each entry is 20 bytes long.
//    UInt32    tag           4-byte sfnt table identifier.
//    UInt32    offset        Offset to the data, from beginning of WOFF file.
//    UInt32    compLength    Length of the compressed data, excluding padding. If compressed, must have been done by compress2 of zlib
//    UInt32    origLength    Length of the uncompressed table, excluding padding.
//    UInt32    origChecksum  Checksum of the uncompressed table.
//
//FontTables        The font data tables from the input sfnt font, compressed to reduce bandwidth requirements.
//ExtendedMetadata  An optional block of extended metadata, represented in XML format and compressed for storage in the WOFF file.
//PrivateData       An optional block of private data for the font designer, foundry, or vendor to use.

/*! Scan for CPAL, COLR, or SVG tables. If which==0, check for existence only.
 * If which!=0, then: 
 * whole&1 for CPAL, whole&2 for COLR, whole&4 for SVG.
 */
int FontScanner::Scan(int which, const char *nfile)
{
	if (nfile) makestr(file, nfile);

	FILE *f = fopen(file, "r");
	if (!f) return 1;

	ff = f;

	int err = 0;
	unsigned char buffer[45], tag[5];
	buffer[44] = '\0';
	tag[4] = '\0';
	fread(buffer,1,44, f); //reads in whole header

	try {
        svg_offset  = 0;
        svg_complen = 0;
        svg_origlen = 0;

        cpal_offset  = 0;
        cpal_complen = 0;
        cpal_origlen = 0;

        colr_offset  = 0;
        colr_complen = 0;
        colr_origlen = 0;

		//ttf: 00 01 00 00 00  <-  "OpenType fonts that contain TrueType outlines should use the value of 0x00010000 for the sfntVersion."
		//--or--   74 72 75 65 00 ("true")  in the Apple specification. Don't use that for otf
		//
		//otf: 4f 54 54 f4 00 "OTTO"

        //if (strncmp((char*)buffer, "wOFF", 4)) throw _("Not a wOFF");
		//-----
        if (strncmp((char*)buffer, "wOFF", 4) && strncmp((char*)buffer, "OTTO", 4))
			throw _("Not a wOFF or an otf font!");

		int numtables = (buffer[12]<<8) | buffer[13];
		if (numtables > 200) {
			throw (_("Way too many tables in font file. Something's probably gone wrong!"));
		}

		DBG int filesize=(((((buffer[8]<<8)|buffer[9])<<8)|buffer[10])<<8)|buffer[11];
		DBG cerr << "file: "<<file<<endl;
		DBG cerr << "file size: "<<filesize<<endl;
		DBG cerr << "Num tables: "<<numtables<<endl;

		for (int c=0; c<numtables; c++) {
			fseek(f, 44+c*20, SEEK_SET);
			fread(buffer,1,20, f);

			strncpy((char*)tag, (char*)buffer, 4);
			if (tag[0]<32) tag[0]=32;
			if (tag[1]<32) tag[1]=32;
			if (tag[2]<32) tag[2]=32;
			if (tag[3]<32) tag[3]=32;

			unsigned int offset      =(((((buffer[4]<<8)|buffer[5])<<8)|buffer[6])<<8)|buffer[7];
			unsigned int compLength  =(((((buffer[8]<<8)|buffer[9])<<8)|buffer[10])<<8)|buffer[11];
			unsigned int origLength  =(((((buffer[12]<<8)|buffer[13])<<8)|buffer[14])<<8)|buffer[15];
			unsigned int origChecksum=(((((buffer[16]<<8)|buffer[17])<<8)|buffer[18])<<8)|buffer[19];

            if (!strcmp((char *)tag, "SVG ")) {
                svg_offset  = offset;
                svg_complen = compLength;
                svg_origlen = origLength;

            } else if (!strcmp((char *)tag, "CPAL")) {
                cpal_offset  = offset;
                cpal_complen = compLength;
                cpal_origlen = origLength;

            } else if (!strcmp((char *)tag, "COLR")) {
                colr_offset  = offset;
                colr_complen = compLength;
                colr_origlen = origLength;
            }

            DBG printf("Found table %s   offset: %6d   compressed len: %6d   origl: %6d   origchecksum: %u\n", 
			DBG		tag, offset, compLength, origLength, origChecksum);
		}

	} catch (int e) {
	} catch (const char *msg) {
		DBG cerr << msg <<endl;
		err = 100;
	}

	DBG if (svg_offset > 0) cerr << "Found svg table"<<endl;
	DBG if (cpal_offset > 0) cerr << "Found cpal table"<<endl;
	DBG if (colr_offset > 0) cerr << "Found colr table"<<endl;

    if (cpal_offset > 0 && which & (int)FontType::CPAL) err |= ScanCpal();
    if (colr_offset > 0 && which & (int)FontType::COLR) err |= ScanColr();
    if ( svg_offset > 0 && which & (int)FontType::SVG)  err |= ScanSvg();

    ff = nullptr;
    fclose(f);
    return err;
}

/*! Return 0 for succes, nonzero for no cpal.
 *
 * If this->ff!=nullptr, then use that. else open this->file.
 */
int FontScanner::ScanCpal()
{
	if (!cpal_offset) return 1;

    FILE *f = ff;
    if (!f) f = fopen(file, "r");
    if (!f) return 1;

	DBG cerr <<"\nCPAL table found, attempting to read..."<<endl;

	//------------------------------- CPAL table: ---------------------------------
	//USHORT    version                 Table version number (=0 or 1).
	//USHORT    numPalettesEntries      Number of palette entries in each palette.
	//USHORT    numPalette              Number of palettes in the table.
	//USHORT    numColorRecords         Total number of color records, combined for all palettes.
	//ULONG     offsetFirstColorRecord  Offset from the beginning of CPAL table to the first ColorRecord.
	//USHORT    colorRecordIndices[numPalettes]   Index of each palette’s first color record in the combined color record array.
	//  CPAL version 1 has these also:
	//ULONG    offsetPaletteTypeArray        Offset from the beginning of CPAL table to the Palette Type Array. Set to 0 if no array is provided.
	//ULONG    offsetPaletteLabelArray       Offset from the beginning of CPAL table to the Palette Labels Array. Set to 0 if no array is provided.
	//ULONG    offsetPaletteEntryLabelArray  Offset from the beginning of CPAL table to the Palette Entry Label Array. Set to 0 if no array is provided.
	//
	//  CPAL palette table:
	//ColorRecord 	colorRecords[numColorRecords] 	Color records for all palettes
	//
	//  ColorRecord:
	//BYTE 	blue 	Blue value (B0).
	//BYTE 	green 	Green value (B1).
	//BYTE 	red 	Red value (B2).
	//BYTE 	alpha 	Alpha value (B3).
	//
	//  Palette Type Array:
	//ULONG 	paletteType [numPalettes] 	Array of 32-bit flag fields that describe properties of each palette.
	//                                      Set a particular ULONG to 0 if no properties are specified for that palette. Property flags:
	//                                      Bit 0: Palette is appropriate to use when displaying the font on a light background such as white.
	//                                      Bit 1: Palette is appropriate to use when displaying the font on a dark background such as
	//                                      black. Bits 0 and 1 are not mutually exclusive: they may both be set.
	//  Palette Labels Array:
	//USHORT 	paletteLabel [numPalettes] 	Array of name table IDs (typically in the font-specific name ID range) that specify user interface
	//                                      strings associated with each palette. Use 0xFFFF if no name ID is provided for a particular palette.
	//
	//  Palette Entry Label Array:
	//USHORT 	paletteEntryLabel [numPaletteEntries] 	Array of name table IDs (typically in the font-specific name ID range)
	//                                                  that specify user interface strings associated with each palette entry,
	//                                                  e.g. “Outline”, “Fill”. This set of palette entry labels applies to all palettes in the
	//                                                  font. Use 0xFFFF if no name ID is provided for a particular palette entry.

	DBG cerr <<"cpal compressed len: "<<cpal_complen<<endl;
	DBG cerr <<"cpal original len:   "<<cpal_origlen<<endl;

	unsigned char cpalcomptable[cpal_complen];
	unsigned char cpalorigtable[cpal_origlen+1];

	fseek(f, cpal_offset, SEEK_SET);
	fread(cpalcomptable, 1,cpal_complen, f);

	if (cpal_complen == cpal_origlen) {
		 //it wasn't compressed
		DBG cerr <<"cpal was not compressed.."<<endl;
		strncpy((char*)cpalorigtable, (char*)cpalcomptable, cpal_complen);
		cpalorigtable[cpal_complen]='\0';

	} else {
		uLongf actuallen=cpal_origlen;
		int status = uncompress((Bytef*)cpalorigtable, &actuallen,  (Bytef*)cpalcomptable, cpal_complen);

		if (status == Z_OK) {
			DBG cerr <<"Uncompress success!"<<endl;
		} else {
			DBG if (status == Z_MEM_ERROR)  cerr <<"Uncompress memory error!"<<endl;
			DBG else if (status == Z_BUF_ERROR)  cerr <<"Uncompress buffer error!"<<endl;
			DBG else if (status == Z_DATA_ERROR) cerr <<"Uncompress data error!"<<endl;
			return 2;
		}

			
		cpal_origlen = actuallen;
		cpalorigtable[cpal_origlen]='\0';
	}

	 //now cpal stored in cpalorigtable[cpal_origlen]
	unsigned char *ptr = cpalorigtable;

	DBG int cpal_table_version = (ptr[0]<<8)|ptr[1];
	ptr+=2;

	int num_palette_entries = (ptr[0]<<8)|ptr[1]; //per palette
	ptr+=2;

	int num_palettes = (ptr[0]<<8)|ptr[1];
	ptr+=2;

	DBG int num_colors = (ptr[0]<<8)|ptr[1];
	ptr+=2;

	long first_color_offset = (((((ptr[0]<<8)|ptr[1])<<8)|ptr[2])<<8)|ptr[3]; //from cpal start
	ptr+=4; 

	int palcolor_starts[num_palettes];

    DBG cerr << "CPAL table version " << cpal_table_version << endl;
    DBG cerr << "  number of colors: " << num_colors << endl;
    DBG cerr << "  number of palettes: " << num_palettes << endl;
    DBG cerr << "  number of palette entries: " << num_palette_entries << endl;
	DBG unsigned int red, green, blue, alpha;

	if (palette == nullptr) palette = new GradientStrip;

    for (int c=0; c<num_palettes; c++) {
		palcolor_starts[c] = (ptr[0]<<8)|ptr[1];
        ptr += 2;

        DBG cerr << "  Palette "<<c<<", colors start at: "<<palcolor_starts[c]<<endl;
		
		unsigned char *colors = cpalorigtable + first_color_offset + 4*palcolor_starts[c];
        for (int c2 = 0; c2 < num_palette_entries; c2++) {  // each color is in order  b g r a
            DBG blue  = colors[0];
			DBG green = colors[1];
			DBG red   = colors[2];
			DBG alpha = colors[3];
			DBG cerr << "    color "<<c2<<", rgba: "<<red<<" "<<green<<" "<<blue<<" "<<alpha<<endl;

			palette->AddColor(c2, colors[2]/255., colors[1]/255., colors[0]/255., colors[3]/255.);

			colors += 4;
		}
	}

    if (f != ff) fclose(f);

    return 0;
}

/*! Return 0 for succes, nonzero for no colr.
 */
int FontScanner::ScanColr()
{
	if (!colr_offset) return 1;

	FILE *f=ff;
	if (!f) f=fopen(file, "r");
	if (!f) return 1;

	DBG cerr <<"\nCOLR table found, attempting to read..."<<endl;

	//------------------------------- COLR table: -----------------------------------------
	//USHORT     version                Table version number (starts at 0).
	//USHORT     numBaseGlyphRecords    Number of Base Glyph Records.
	//ULONG      offsetBaseGlyphRecord  Offset (from beginning of COLR table) to Base Glyph records.
	//ULONG      offsetLayerRecord      Offset (from beginning of COLR table) to Layer Records.
	//USHORT     numLayerRecords        Number of Layer Records.
	//
	//  Base Glyph record:
	//USHORT     GID              Glyph ID of reference glyph. This glyph is for reference only and is not rendered for color.
	//USHORT     firstLayerIndex  Index (from beginning of the Layer Records) to the layer record. There will be numLayers consecutive entries for this base glyph.
	//USHORT     numLayers        Number of color layers associated with this glyph.
	//
	//  Layer Record:
	//USHORT     GID           Glyph ID of layer glyph (must be in z-order from bottom to top).
	//USHORT     paletteIndex  Index value to use with a selected color palette. This value must be less than numPaletteEntries
	//                           in the CPAL table. A palette entry index value of 0xFFFF is a special case indicating that the
	//                           text foreground color (defined by a higher-level client) should be used and shall not be treated
	//                           as actual index into CPAL ColorRecord array.

	unsigned char colrcomptable[colr_complen];
	unsigned char colrorigtable[colr_origlen+1];

	fseek(f, colr_offset, SEEK_SET);
	fread(colrcomptable, 1,colr_complen, f);

	if (colr_complen == colr_origlen) {
		 //it wasn't compressed
		DBG cerr <<"colr was not compressed.."<<endl;
		strncpy((char*)colrorigtable, (char*)colrcomptable, colr_complen);
		colrorigtable[colr_complen]='\0';

	} else {
		 //it was compressed
		uLongf actuallen=colr_origlen;
		int status = uncompress((Bytef*)colrorigtable, &actuallen,  (Bytef*)colrcomptable, colr_complen);

		if (status == Z_OK) {
			DBG cerr <<"Uncompress success!"<<endl;
		} else {
			DBG if (status == Z_MEM_ERROR)  cerr <<"Uncompress memory error!"<<endl;
			DBG else if (status == Z_BUF_ERROR)  cerr <<"Uncompress buffer error!"<<endl;
			DBG else if (status == Z_DATA_ERROR) cerr <<"Uncompress data error!"<<endl;
			return 2;
		}

			
		colr_origlen = actuallen;
		colrorigtable[colr_origlen]='\0';
	} 

	unsigned char *ptr = colrorigtable;
	DBG int colr_table_version = (ptr[0]<<8)|ptr[1];
	DBG cerr <<" colr table version: "<<colr_table_version<<endl;
	ptr+=2;

	int num_base_glyphs = (ptr[0]<<8)|ptr[1];
	ptr+=2;

	unsigned int offsetToBaseGlyphs = (((((ptr[0]<<8)|ptr[1])<<8)|ptr[2])<<8)|ptr[3];
	ptr+=4;
	unsigned int offsetToLayers = (((((ptr[0]<<8)|ptr[1])<<8)|ptr[2])<<8)|ptr[3];
	ptr+=4;
	//int num_layer_records = (ptr[0]<<8)|ptr[1];
	//ptr+=2;

	colr_maps.flush();
	unsigned char *base_glyph_ptr  = colrorigtable + offsetToBaseGlyphs;

	int allocated=10;
	int *map=new int[allocated];
	int *col=new int[allocated];

	for (int c=0; c<num_base_glyphs; c++) {
		ptr = base_glyph_ptr;

		int base_glyph  = (ptr[0]<<8)|ptr[1];
		int first_index = (ptr[2]<<8)|ptr[3];
		int num_layers  = (ptr[4]<<8)|ptr[5];

		if (num_layers>allocated) {
			delete[] map;
			delete[] col;
			allocated+=10;
			map=new int[allocated];
			col=new int[allocated];
		}

		ptr = colrorigtable + offsetToLayers + first_index*4;
		for (int c2=0; c2<num_layers; c2++) {
			map[c2] = (ptr[0]<<8)|ptr[1];
			col[c2] = (ptr[2]<<8)|ptr[3];
		}

		colr_maps.push(new ColrGlyphMap(base_glyph, num_layers, map, col));

		base_glyph_ptr += 6;
	}

	delete[] map;



	if (f!=ff) fclose(f);

	return 0;
}

/*! Return 0 for succes, nonzero for no svg.
 * This will populate svgtable and svgentries.
 */
int FontScanner::ScanSvg()
{
	if (!svg_offset) return 1;

    FILE *f = ff;
    if (!f) f = fopen(file, "r");
    if (!f) return 1;

    DBG cerr <<"\nSVG table found, attempting to read..."<<endl;

	//----------------------------- SVG table: -----------------------------------
	// USHORT 	version 	Table version (starting at 0). Set to 0.
	// ULONG 	offsetToSVGDocIndex 	Offset (relative to the start of the SVG table) to the SVG Documents Index. Must be non-zero.
	// ULONG 	reserved 	Set to 0.
	//   
	//   SVG Document Index:
	// USHORT 	numEntries 	Number of SVG Document Index Entries. Must be non-zero.
	// SVG Document Index Entry 	entries[numEntries] 	Array of SVG Document Index Entries.
	//
	//   SVG Document Index Entry:
	// USHORT  startGlyphID    The first glyph ID in the range described by this index entry.
	// USHORT  endGlyphID      The last  glyph ID in the range described by this index entry. Must be >= startGlyphID.
	// ULONG   svgDocOffset    Offset from the beginning of the SVG Document Index to an SVG document. Must be non-zero.
	// ULONG   svgDocLength    Length of the SVG document. Must be non-zero.
	//
	// can have colors in the CPAL table, specify with var(--color12)
	// as in <path fill="var(--color0)" d="..."/>
	//
	//from http://www.w3.org/TR/svg-integration/:
	//  "SVG documents processed with the font document referencing mode must use the secure animated processing mode."


	int err = 0;

	unsigned char svgcomptable[svg_complen];
	svgtable = new unsigned char[svg_origlen+1];
	unsigned char *svgorigtable = svgtable;

	fseek(f, svg_offset, SEEK_SET);
	fread(svgcomptable, 1,svg_complen, f);

	try {
		if (svg_complen == svg_origlen) {
			 //it wasn't compressed, we can just copy over directly
			strncpy((char*)svgorigtable, (char*)svgcomptable, svg_complen);
			svgorigtable[svg_complen]='\0';

		} else {
            uLongf actuallen = svg_origlen;
            int status = uncompress((Bytef*)svgorigtable, &actuallen,  (Bytef*)svgcomptable, svg_complen);

			if (status != Z_OK) {
				DBG if (status == Z_MEM_ERROR)  cerr <<"Uncompress memory error!"<<endl;
				DBG else if (status == Z_BUF_ERROR)  cerr <<"Uncompress buffer error!"<<endl;
				DBG else if (status == Z_DATA_ERROR) cerr <<"Uncompress data error!"<<endl;

				throw(1);
			} 
				
			svg_origlen = actuallen;
			svgorigtable[svg_origlen]='\0';
		}
		

		 //now svg stored in char[origlen] svgorigtable
		unsigned char *ptr = svgorigtable;

		DBG int svg_table_version = (ptr[0]<<8)|ptr[1];
		ptr+=2;

		DBG cerr <<" svg table version: "<<svg_table_version<<endl;

		unsigned int offsetToDocIndex = (((((ptr[0]<<8)|ptr[1])<<8)|ptr[2])<<8)|ptr[3];
		unsigned char *docIndexStart = svgorigtable + offsetToDocIndex;
		ptr = docIndexStart;

		
		unsigned int numentries  = (ptr[0]<<8)|ptr[1]; //in doc index
		ptr+=2;

		 //now ptr points at start of array of SVG Document Index Entries

		 //for each svg entry...
		//   SVG Document Index Entry:
		// USHORT  startGlyphID    The first glyph ID in the range described by this index entry.
		// USHORT  endGlyphID      The last  glyph ID in the range described by this index entry. Must be >= startGlyphID.
		// ULONG   svgDocOffset    Offset from the beginning of the SVG Document Index to an SVG document. Must be non-zero.
		// ULONG   svgDocLength    Length of the SVG document. Must be non-zero.

		SvgEntry *entry;
		for (unsigned int c=0; c<numentries; c++) {
			entry = new SvgEntry;
			entry->startglyph = (ptr[0]<<8)|ptr[1];  ptr+=2;
			entry->endglyph   = (ptr[0]<<8)|ptr[1];  ptr+=2;
			entry->offset     = offsetToDocIndex + ((((((ptr[0]<<8)|ptr[1])<<8)|ptr[2])<<8)|ptr[3]);  ptr+=4; 
			entry->len        = (((((ptr[0]<<8)|ptr[1])<<8)|ptr[2])<<8)|ptr[3];  ptr+=4;
			svgentries.push(entry,1);

			//svgentries[c].svg = new char[svgentries[c].len+1];
			//strncpy(svgentries[c].svg, (char*)(docIndexStart + svgentries[c].offset), svgentries[c].len);
			//svgentries[c].svg[svgentries[c].len] = '\0';
		}


		//DBG //dump out whole svg table for debugging purposes:
		//DBG for (unsigned int c=0; c<svg_origlen; c++) {
		//DBG 	if (svgorigtable[c]<32) svgorigtable[c]=' ';
		//DBG } 
		//DBG cerr <<"SVG table:\n"<<svgorigtable<<endl;


	} catch (int errr) {
		DBG cerr <<"...Error "<<errr<<"reading in svg table, pretending svg table is not there."<<endl;
		err = errr;

        delete[] svgtable;
        svgtable = nullptr;
        svg_offset = 0;
    }

    if (f != ff) fclose(f);

    return err;
}

/*! Output one file for each entry in svgentries.
 * Files will be of the form "glyph-%d.svg" if there is only one glpyh for that, or
 * "glyph-%d-$d.svg" when there is a range.
 *
 * Please note that there might be duplicate files created, only with different ranges defined.
 * \todo do something about potentially duplicated files.
 */
void FontScanner::SvgDump(const char *directory)
{
	if (!svgentries.n) return;
	if (!directory) directory = "./";

	for (int c=0; c<svgentries.n; c++) {
		SvgEntry *entry = svgentries.e[c];

		char file[strlen(directory) + 200];
		if (entry->startglyph == entry->endglyph)
			 sprintf(file, "%s/glyph-%d.svg",    directory, entry->startglyph);
		else sprintf(file, "%s/glyph-%d-%d.svg", directory, entry->startglyph, entry->endglyph);

		FILE *fout = fopen(file, "w");
		if (!fout) {
			DBG cerr << "WARNING! Could not open "<<file<< " for writing glyph. Aborting!"<<endl;
			break;
		}
		DBG cerr << "Writing "<<file<<endl;
		fwrite(svgtable + entry->offset, 1, entry->len, fout);
		fclose(fout);
	}
}



} // namespace Laxkit



