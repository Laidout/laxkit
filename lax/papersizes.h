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
const char *BuiltinPaperSizes[61*5]=
	{
		"Letter"   ,"8.5" ,"11"   ,"in", "p", // 0
		"Legal"    ,"8.5" ,"14"   ,"in", "p", // 1
		"Tabloid"  ,"11"  ,"17"   ,"in", "p", // 2
		"A4"       ,"210" ,"297"  ,"mm", "p", // 3
		"A3"       ,"297" ,"420"  ,"mm", "p", // 4
		"A2"       ,"420" ,"594"  ,"mm", "p", // 5
		"A1"       ,"594" ,"841"  ,"mm", "p", // 6
		"A0"       ,"841" ,"1189" ,"mm", "p", // 7
		"A5"       ,"148" ,"210"  ,"mm", "p", // 8
		"A6"       ,"105" ,"148"  ,"mm", "p", // 9
		"A7"       ,"74"  ,"105"  ,"mm", "p", // 10
		"A8"       ,"52"  ,"74"   ,"mm", "p", // 11
		"A9"       ,"37"  ,"52"   ,"mm", "p", // 12
		"A10"      ,"26"  ,"37"   ,"mm", "p", // 13
		"B0"       ,"1000","1414" ,"mm", "p", // 14
		"B1"       ,"707" ,"1000" ,"mm", "p", // 15
		"B2"       ,"500" ,"707"  ,"mm", "p", // 16
		"B3"       ,"353" ,"500"  ,"mm", "p", // 17
		"B4"       ,"250" ,"353"  ,"mm", "p", // 18
		"B5"       ,"176" ,"250"  ,"mm", "p", // 19
		"B6"       ,"125" ,"176"  ,"mm", "p", // 20
		"B7"       ,"88"  ,"125"  ,"mm", "p", // 21
		"B8"       ,"62"  ,"88"   ,"mm", "p", // 22
		"B9"       ,"44"  ,"62"   ,"mm", "p", // 23
		"B10"      ,"31"  ,"44"   ,"mm", "p", // 24
		"C0"       ,"917" ,"1297" ,"mm", "p", // 25
		"C1"       ,"648" ,"917"  ,"mm", "p", // 26
		"C2"       ,"458" ,"648"  ,"mm", "p", // 27
		"C3"       ,"324" ,"458"  ,"mm", "p", // 28
		"C4"       ,"229" ,"324"  ,"mm", "p", // 29
		"C5"       ,"162" ,"229"  ,"mm", "p", // 30
		"C6"       ,"114" ,"162"  ,"mm", "p", // 31
		"C7"       ,"81"  ,"114"  ,"mm", "p", // 32
		"C8"       ,"57"  ,"81"   ,"mm", "p", // 33
		"C9"       ,"40"  ,"57"   ,"mm", "p", // 34
		"C10"      ,"28"  ,"40"   ,"mm", "p", // 35
		"ArchA"    ,"9"   ,"12"   ,"in", "p", // 36
		"ArchB"    ,"12"  ,"18"   ,"in", "p", // 37
		"ArchC"    ,"18"  ,"24"   ,"in", "p", // 38
		"ArchD"    ,"24"  ,"36"   ,"in", "p", // 39
		"ArchE"    ,"36"  ,"48"   ,"in", "p", // 40
		"Flsa"     ,"8.5" ,"13"   ,"in", "p", // 41
		"Flse"     ,"8.5" ,"13"   ,"in", "p", // 42
		"Index"    ,"3"   ,"5"    ,"in", "p", // 43
		"Executive","7.25","10.5" ,"in", "p", // 44
		"Ledger"   ,"17"  ,"11"   ,"in", "p", // 45
		"Halfletter","5.5","8.5"  ,"in", "p", // 46
		"Note"      ,"7.5","10"   ,"in", "p", // 47
		"1:1"       ,"1"  ,"1"    ,"in", "p", // 48
		"4:3"       ,"4"  ,"3"    ,"in", "p", // 49
		"16:9"      ,"16" ,"9"    ,"in", "p", // 50
		"640x480"   ,"640","480"  ,"px", "s", // 51
		"800x600"   ,"800","600"  ,"px", "s", // 52
		"1024x768"  ,"1024","768" ,"px", "s", // 53
		"1280x1024" ,"1280","1024","px", "s", // 54
		"1600x1200" ,"1600","1200","px", "s", // 55
		"1920x1080" ,"1920","1080","px", "s", // 56
		"1920x1200" ,"1920","1200","px", "s", // 57
		"4096x2160" ,"4096","2160","px", "s", // 58
		"3840x2160" ,"3840","2160","px", "s", // 59
		NULL,NULL,NULL,NULL,NULL // 60
	};

} //namespace Laxkit;




#endif

