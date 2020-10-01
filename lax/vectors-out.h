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
#ifndef _LAX_VECTORS_OUT_H
#define _LAX_VECTORS_OUT_H

#include <lax/vectors.h>
#include <iostream>


namespace Laxkit {

// Functions to output various vector classes to streams

std::ostream &operator<<(std::ostream &os, const flatvector &v);
std::ostream &operator<<(std::ostream &os, const spacevector &v);
std::ostream &operator<<(std::ostream &os, const Quaternion &v);
std::ostream &operator<<(std::ostream &os, const flatline &v);
std::ostream &operator<<(std::ostream &os, const spaceline &v);
std::ostream &operator<<(std::ostream &os, const Plane &v);
std::ostream &operator<<(std::ostream &os, const Basis &v);

} //namespace Laxkit

#endif
