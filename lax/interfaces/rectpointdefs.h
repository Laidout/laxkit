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
//    Copyright (C) 2010 by Tom Lechner
//
#ifndef _LAX_RECTPOINTDEFS_H
#define _LAX_RECTPOINTDEFS_H


//This file has a bunch of definitions that are used by RectInterface and ObjectInterface.


// These are returned for various draggable points and areas
enum RectHandles {
	RP_None = 0,
	RP_Move,

	RP_Center1,
	RP_Center2,
	RP_Shearpoint,
	RP_Drag_Rect,

	RP_Rotate_NE,
	RP_Rotate_NW,
	RP_Rotate_SE,
	RP_Rotate_SW,
	RP_Rotate_Any,
	RP_Rotate_Num,
	RP_Rotate_Diff,

	RP_Persp_NE,
	RP_Persp_NW,
	RP_Persp_SE,
	RP_Persp_SW,

	RP_Shear_N,
	RP_Shear_W,
	RP_Shear_S,
	RP_Shear_E,

	RP_Scale_N,
	RP_Scale_NW,
	RP_Scale_W,
	RP_Scale_SW,
	RP_Scale_S,
	RP_Scale_SE,
	RP_Scale_E,
	RP_Scale_NE,
	RP_Scale_Num,

	RP_Flip_H,
	RP_Flip_V,
	RP_Flip1,
	RP_Flip2,
	RP_Flip_Go,

	// These are used for convenience when computing handle dragging
	RP_SW = 101,
	RP_W,
	RP_NW,
	RP_N,
	RP_NE,
	RP_E,
	RP_SE,
	RP_S,
	RP_Middle,

	RP_MAX
};


#define HAS_CENTER1     1
#define HAS_CENTER2     2
#define HAS_SHEARPOINT  4

// For RectInterface::showdecs
#define SHOW_OUTER_HANDLES  (1<<0)
#define SHOW_INNER_HANDLES  (1<<1)
#define SHOW_TARGET         (1<<2)
#define SHOW_LINK_BALL      (1<<3)
#define SHOW_CONSTRAINTS    (1<<4)
#define SHOW_FLIP_CONTROLS  (1<<5)



#endif



