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

//template implementation:
#include <lax/lists.cc>


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
	flatpoint pp;
	double w = boxwidth(), h = boxheight();
	double scale = 1;
	if (w > h) scale = 1/w; else scale = 1/h;

	o *= scale;

	Coordinate *p, *start;
	for (int c=0; c<paths.n; c++) {
		Path *path = paths.e[c];
		p = start = path->path;
		if (!path) { paths.remove(c); c--; continue; }

		do {
			pp = p->p() * scale - o;
			p->p(pp);
			p = p->next;
		} while (p && p != start);
	}
}

/*! Copy over all non-empty paths, ignoring holes.
 */
void ShapeBrush::CopyFrom(PathsData *pathsdata)
{
	clear();
	bool skip;
	for (int c=0; c<pathsdata->paths.n; c++) {
		if (!pathsdata->paths.e[c]->path) continue;
		skip = false;

		for (int c2=0; c2<pathsdata->paths.n; c2++) {
			if (c == c2) continue;
			if (paths.e[c2]->Contains(paths.e[c]) == 1) {
				skip = true;
				break;
			}
		}

		if (skip) continue;
		Path *path = paths.e[c]->duplicate();
		paths.push(path);
	}

	FindBBox();
	Normalize();
}

void ShapeBrush::Remap()
{
	// ***
}

void ShapeBrush::MinMax(int pathi, flatpoint direction, flatpoint &min, flatpoint &max)
{
	if (direction != last_dir) needtoremap = true;
	if (needtoremap) Remap();

	
}



} // namespace LaxInterfaces
