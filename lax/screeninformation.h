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
//    Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef _LAX_SCREENINFORMATION_H
#define _LAX_SCREENINFORMATION_H


namespace Laxkit {

//--------------------------- ScreenInformation -------------------------------
class ScreenInformation
{
  public:
    char *id; //ideally such as DP-0, DP-5
	char *name; //ideally such as "Dell Something"
	int primary;
	int screen; //screen number, can contain many monitors
	int monitor; //id of monitor within larger virtual screen space
	int x,y, width,height;
	int mmwidth,mmheight;
	int depth;

	ScreenInformation *next;

	ScreenInformation() { next = nullptr; name = nullptr; id = nullptr; }
	~ScreenInformation();
	int HowMany() { return 1 + (next ? next->HowMany() : 0); }
	ScreenInformation *Get(int i) { if (i==0) return this; else { if (next && i>0) return next->Get(i-1); else return nullptr; } }
	ScreenInformation *Get(char *which);
};

} //namespace Laxkit


#endif
