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


#include <lax/interfaces/texttopath.h>
#include <lax/interfaces/pathinterface.h>


using namespace Laxkit;

namespace LaxInterfaces {


#define FACTOR (64.)

int pathsdata_ft_move_to(const FT_Vector* to, void* user)
{
	PathsData *paths = (PathsData*)user;
	if (paths->paths.n>0 && paths->paths.e[paths->paths.n-1]->path==NULL)
		paths->lineTo(flatpoint(to->x/FACTOR, -to->y/FACTOR));
	else paths->moveTo(flatpoint(to->x/FACTOR, -to->y/FACTOR));
	return 0;
}

int pathsdata_ft_line_to(const FT_Vector* to, void* user)
{
	PathsData *paths = (PathsData*)user;
	paths->lineTo(flatpoint(to->x/FACTOR, -to->y/FACTOR));
	return 0;
}

int pathsdata_ft_conic_to(const FT_Vector* control, const FT_Vector* to, void* user)
{
	PathsData *paths = (PathsData*)user;
	flatpoint lastp = paths->LastVertex()->p();
	flatpoint cc(control->x/FACTOR, -control->y/FACTOR), pto(to->x/FACTOR,-to->y/FACTOR);
	paths->curveTo(lastp + (cc-lastp)*2./3, pto + (cc-pto)*2./3, pto);
	return 0;
}

int pathsdata_ft_cubic_to(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector*  to, void* user)
{
	PathsData *paths = (PathsData*)user;
	paths->curveTo( flatpoint(control1->x/FACTOR, -control1->y/FACTOR),
					flatpoint(control2->x/FACTOR, -control2->y/FACTOR),
					flatpoint(to->x/FACTOR, -to->y/FACTOR));
	return 0;
}


} // namespace LaxInterfaces


