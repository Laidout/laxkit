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
//    Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef _LAX_DRAWINGDEFS_H
#define _LAX_DRAWINGDEFS_H


namespace Laxkit {

//! Compositing operators for Displayer drawing functions.
/*!
 * These are basically one to one with Cairo's operators.
 * Used by LineStyle::function and FillStyle::function.
 */
enum LaxCompositeOp
{
	LAXOP_None=0,
	LAXOP_Undefined, //!< - different than None!
	LAXOP_Clear,     //!< - Block out parts of dest that are in src
	LAXOP_Source,    //!< - Lay on src totally
	LAXOP_Over,      //!< - Lay on src with transparency
	LAXOP_In,        //!< - Remove dest, but only put src where dest was
	LAXOP_Out,       //!< - Use all src, but mix with parts that overlap dest
	LAXOP_Atop,      //!< - Use all dest, but mix where src was
	LAXOP_Dest,      //!< - Use only dest, ignore src, this is a no-op
	LAXOP_Dest_over, //!< - Use dest as src, and src as dest
	LAXOP_Dest_in,   //!< - Like In, but swap dest and src
	LAXOP_Dest_out,  //!< - Like Out, but swap dest and src
	LAXOP_Dest_atop, //!< - Like Atop, but swap dest and src
	LAXOP_Xor,       //!< - Perform xor
	LAXOP_Add,       //!< - Add color values, clamping to max
	LAXOP_Saturate,  //!< - Pump it up
	LAXOP_Multiply,    //!< - At least as dark as darkest
	LAXOP_Screen,      //!< - At least as light as lightest
	LAXOP_Overlay,     //!< - Darkens or lightens depending on color
	LAXOP_Darken,      //!< -
	LAXOP_Lighten,      //!< -
	LAXOP_Color_dodge,   //!< -
	LAXOP_Color_burn,     //!< -
	LAXOP_Hard_light,     //!< -
	LAXOP_Soft_light,     //!< -
	LAXOP_Difference,     //!< -
	LAXOP_Exclusion,      //!< -
	LAXOP_Hsl_hue,        //!< -
	LAXOP_Hsl_saturation, //!< -
	LAXOP_Hsl_color,      //!< -
	LAXOP_Hsl_luminosity, //!< -

	LAXOP_MAX
};

enum LaxSpreadStyle
{
	LAXSPREAD_None    = 0,
	LAXSPREAD_Repeat  = 1,
	LAXSPREAD_Reflect = 2,
	LAXSPREAD_Pad     = 3,
	LAXSPREAD_MAX
};

enum LaxCapStyle
{
	LAXCAP_Butt = 1,
	LAXCAP_Round,
	LAXCAP_Peak,
	LAXCAP_Square,
	LAXCAP_Projecting,
	LAXCAP_Zero_Width,
	LAXCAP_From_Brush,
	LAXCAP_Custom,

	LAXCAP_MAX
};

enum LaxJoinStyle
{
	LAXJOIN_Miter = 1,
	LAXJOIN_Round,
	LAXJOIN_Bevel,
	LAXJOIN_Extrapolate,

	LAXJOIN_MAX
};

enum LaxFillRule
{
	LAXFILL_Nonzero = 1,
	LAXFILL_EvenOdd,
	LAXFILL_MAX
};

enum LaxFillStyle
{
	LAXFILL_None = 0,
	LAXFILL_Solid = LAXFILL_MAX,
	LAXFILL_Pattern,
	LAXFILLSTYLE_MAX
};

enum DrawThingTypes {
	THING_Undefined = -1,
	THING_None = 0,
	THING_Circle,
	THING_Circle_X,    //!< Circle with an x in it
	THING_Circle_Plus, //!< Circle with a plus in it
	THING_Square,
	THING_Diamond,     //!< Square rotated 45 degrees
	THING_Triangle_Up,
	THING_Triangle_Down,
	THING_Triangle_Left,
	THING_Triangle_Right,
	THING_To_Left,
	THING_To_Right,
	THING_To_Top,
	THING_To_Bottom,
	THING_Plus,
	THING_X,
	THING_Asterix,
	THING_Folder,
	THING_Pause,
	THING_Eject,         //!< Triangle up with a line underneath it
	THING_Double_Triangle_Up,
	THING_Double_Triangle_Down,
	THING_Double_Triangle_Left, //!< Like a fast forward button
	THING_Double_Triangle_Right,
	THING_Arrow_Left,
	THING_Arrow_Right,
	THING_Arrow_Up,
	THING_Arrow_Down,
	THING_Double_Arrow_Horizontal, //!< Arrow that points left and right
	THING_Double_Arrow_Vertical,
	THING_Pan_Arrows,            //!< Arrow that points left, right, up, and down
	THING_Check,
	THING_Cancel,
	THING_Locked,
	THING_Unlocked,
	THING_Open_Eye,
	THING_Closed_Eye,
	THING_Octagon,
	THING_Magnifying_Glass,
	THING_Wrench,
	THING_Star,
	THING_Gear,
	THING_Pin,
	THING_PinCentered,

	THING_MAX
};


} //namespace Laxkit


#endif

