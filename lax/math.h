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
#ifndef _LAX_LAXMATH_H
#define _LAX_LAXMATH_H


#include <cmath>

namespace Laxkit {

//-------------------------- Math utilities -------------------------

bool isNearZero(double x, double epsilon = 1e-7) { return fabs(x) < epsilon; }
bool isNearZero(float x,  float epsilon = 1e-7) { return fabs(x) < epsilon; }
bool isNearlyEqual(double x, double y, double epsilon = 1e-7) { return fabs(x-y) < epsilon; }
bool isNearlyEqual(float x,  float y,  float  epsilon = 1e-7) { return fabs(x-y) < epsilon; }

} // namespace Laxkit

#endif
