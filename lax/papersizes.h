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
//    Copyright (C) 2013,2015 by Tom Lechner
//


#ifndef _LAX_PAPERSIZES_H
#define _LAX_PAPERSIZES_H


namespace Laxkit {


//-------------------------------- GetBuiltinPaperSizes ------------------

//
// The target is p for paper, c for custom, s for screen, and * for special "whatever" paper.
//
//      PAPERSIZE   Width  Height  Units  Target
//      ------------------------------------------
const char *BuiltinPaperSizes[65*5]=
	{
		"Letter"   ,"8.5" ,"11"  ,"in", "US",
		"Legal"    ,"8.5" ,"14"  ,"in", "US",
		"Tabloid"  ,"11"  ,"17"  ,"in", "US",
		"A4"       ,"210" ,"297" ,"mm", "A",
		"A3"       ,"297" ,"420" ,"mm", "A",
		"A2"       ,"420" ,"594" ,"mm", "A",
		"A1"       ,"594" ,"841" ,"mm", "A",
		"A0"       ,"841" ,"1189","mm", "A",
		"A5"       ,"148" ,"210" ,"mm", "A",
		"A6"       ,"105" ,"148" ,"mm", "A",
		"A7"       ,"74"  ,"105" ,"mm", "A",
		"A8"       ,"52"  ,"74"  ,"mm", "A",
		"A9"       ,"37"  ,"52"  ,"mm", "A",
		"A10"      ,"26"  ,"37"  ,"mm", "A",
		"B0"       ,"1000","1414","mm", "B",
		"B1"       ,"707" ,"1000","mm", "B",
		"B2"       ,"500" ,"707" ,"mm", "B",
		"B3"       ,"353" ,"500" ,"mm", "B",
		"B4"       ,"250" ,"353" ,"mm", "B",
		"B5"       ,"176" ,"250" ,"mm", "B",
		"B6"       ,"125" ,"176" ,"mm", "B",
		"B7"       ,"88"  ,"125" ,"mm", "B",
		"B8"       ,"62"  ,"88"  ,"mm", "B",
		"B9"       ,"44"  ,"62"  ,"mm", "B",
		"B10"      ,"31"  ,"44"  ,"mm", "B",
		"C0"       ,"917" ,"1297","mm", "C",
		"C1"       ,"648" ,"917" ,"mm", "C",
		"C2"       ,"458" ,"648" ,"mm", "C",
		"C3"       ,"324" ,"458" ,"mm", "C",
		"C4"       ,"229" ,"324" ,"mm", "C",
		"C5"       ,"162" ,"229" ,"mm", "C",
		"C6"       ,"114" ,"162" ,"mm", "C",
		"C7"       ,"81"  ,"114" ,"mm", "C",
		"C8"       ,"57"  ,"81"  ,"mm", "C",
		"C9"       ,"40"  ,"57"  ,"mm", "C",
		"C10"      ,"28"  ,"40"  ,"mm", "C",
		"ArchA"    ,"9"   ,"12"  ,"in", "Arch",
		"ArchB"    ,"12"  ,"18"  ,"in", "Arch",
		"ArchC"    ,"18"  ,"24"  ,"in", "Arch",
		"ArchD"    ,"24"  ,"36"  ,"in", "Arch",
		"ArchE"    ,"36"  ,"48"  ,"in", "Arch",
		"Flsa"     ,"8.5" ,"13"  ,"in", "US",
		"Flse"     ,"8.5" ,"13"  ,"in", "US",
		"Index"    ,"3"   ,"5"   ,"in", "US",
		"Executive","7.25","10.5","in", "US",
		"Ledger"   ,"17"  ,"11"  ,"in", "US",
		"Halfletter","5.5","8.5" ,"in", "US",
		"Note"      ,"7.5","10"  ,"in", "US",
		"1:1"       ,"1"  ,"1"   ,"in", "Aspect",
		"1:2"       ,"1"  ,"2"   ,"in", "Aspect",
		"4:3"       ,"4"  ,"3"   ,"in", "Aspect",
		"16:9"      ,"16" ,"9"   ,"in", "Aspect",
		"640x480"   ,"640","480" ,      "px", "Screen",
		"800x600"   ,"800","600" ,      "px", "Screen",
		"1024x768"  ,"1024","768",      "px", "Screen",
		"1280x1024" ,"1280","1024",     "px", "Screen",
		"1600x1200" ,"1600","1200",     "px", "Screen",
		"1920x1080" ,"1920","1080",     "px", "Screen",
		"1920x1200" ,"1920","1200",     "px", "Screen",
		"2560x1440 (2k)", "2560","1440","px", "Screen",
		"3840x2160 (4k)", "3840","2160","px", "Screen",
		"7680x4320 (8k)", "7680","4320","px", "Screen",
		//"Custom"    ,"8.5","11"   ,"in", "", /* NOTE!!! these two must be last!! */
		//"Whatever"  ,"8.5","11"   ,"in", "", /* NOTE!!! these two must be last!! */
		NULL,NULL,NULL,NULL
	};

} //namespace Laxkit;




#endif

