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
//    Copyright (C) 2020 by Tom Lechner
//

#include <lax/interfaces/shapebrush.h>


namespace LaxInterfaces {


/*! \class ShapeBrush
 * A PathsData that is used together, possibly with a LineProfile to stroke another PathsData.
 */

ShapeBrush::ShapeBrush()
{
	needtoremap = true;
}

ShapeBrush::~ShapeBrush()
{
}

/*! Remap points so that bounding box is centered around origin, and points fill a bbox 2 x 2.
 */
void ShapeBrush::Normalize()
{
	flatpoint o((maxx+minx)/2, (maxy+miny)/2);
	double w = boxwidth(), h = boxheight();
	double scale = 1;
	if (w > h) scale = 1/w; else scale = 1/h;

	***
}

void ShapeBrush::CopyFrom(PathsData *pathsdata)
{
	clear();
	for (int c=0; c<pathsdata->n; c++) {
		Path *path = paths.e[c]->duplicate();
		paths.push(path);
	}
	Normalize();
}

void ShapeBrush::MinMax(int pathi, flatpoint direction, flatpoint &min, flatpoint &max)
{
	if (direction != last_dir) needtoremap = true;
	if (needtoremap) Remap();

	
}



} // namespace LaxInterfaces
