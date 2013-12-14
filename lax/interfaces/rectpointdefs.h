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
//    Copyright (C) 2010 by Tom Lechner
//
#ifndef _LAX_RECTPOINTDEFS_H
#define _LAX_RECTPOINTDEFS_H


//This file has a bunch of defines that are used by the code in RectInterface and ObjectInterface.


// These are returned for various draggable points and areas
#define RP_None        0
#define RP_Move        1

#define RP_Center1     2
#define RP_Center2     3
#define RP_Shearpoint  4

#define RP_Rotate_NE   5
#define RP_Rotate_NW   6
#define RP_Rotate_SE   7
#define RP_Rotate_SW   8
#define RP_Rotate_Any  9

#define RP_Persp_NE    10
#define RP_Persp_NW    11
#define RP_Persp_SE    12
#define RP_Persp_SW    13

#define RP_Shear_N     14
#define RP_Shear_W     15
#define RP_Shear_S     16
#define RP_Shear_E     17

#define RP_Scale_N     18
#define RP_Scale_NW    19
#define RP_Scale_W     20
#define RP_Scale_SW    21
#define RP_Scale_S     22
#define RP_Scale_SE    23
#define RP_Scale_E     24
#define RP_Scale_NE    25

#define RP_Flip_H      26
#define RP_Flip_V      27
#define RP_Flip1       28
#define RP_Flip2       29
#define RP_Flip_Go     30

// These are used for convenience when computing handle dragging
#define RP_SW          101
#define RP_W           102
#define RP_NW          103
#define RP_N           104
#define RP_NE          105
#define RP_E           106
#define RP_SE          107
#define RP_S           108
#define RP_Middle      109

#define RP_MAX         109



#define HAS_CENTER1     1
#define HAS_CENTER2     2
#define HAS_SHEARPOINT  4

// For RectInterface decorations
#define SHOW_OUTER_HANDLES  (1<<0)
#define SHOW_INNER_HANDLES  (1<<1)
#define SHOW_TARGET         (1<<2)
#define SHOW_LINK_BALL      (1<<3)
#define SHOW_CONSTRAINTS    (1<<4)



#endif



