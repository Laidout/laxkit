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
#ifndef _LAX_TEXTTOPATH_H
#define _LAX_TEXTTOPATH_H


#include <harfbuzz/hb-ft.h>
#include FT_OUTLINE_H

namespace LaxInterfaces {

int pathsdata_ft_move_to(const FT_Vector* to, void* user);
int pathsdata_ft_line_to(const FT_Vector* to, void* user);
int pathsdata_ft_conic_to(const FT_Vector* control, const FT_Vector* to, void* user);
int pathsdata_ft_cubic_to(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector*  to, void* user);

} // namespace LaxInterfaces


#endif

