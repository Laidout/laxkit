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
//    Copyright (C) 2004-2009,2012-2013 by Tom Lechner
//
#ifndef _LAX_VECTORS_OUT_H
#define _LAX_VECTORS_OUT_H

#include <lax/vectors.h>
#include <iostream>


namespace Laxkit {

// Functions to output to streams various vector classes

std::ostream &operator<<(std::ostream &os, const flatvector &v) { 
    return os << '('<<v.x<<','<<v.y<<')';
}


std::ostream &operator<<(std::ostream &os, const spacevector &v) { 
    return os << '('<<v.x<<','<<v.y<<','<<v.z<<')';
}

std::ostream &operator<<(std::ostream &os, const Quaternion &v) { 
    return os << '('<<v.x<<','<<v.y<<','<<v.z<<','<<v.w')';
}

std::ostream &operator<<(std::ostream &os, const flatline &v) { 
    return os << "{ p:("<<v.p.x<<','<<v.p.y<<"), v:("<<v.v.x<<','<<v.v.y<<") }";
}

std::ostream &operator<<(std::ostream &os, const spaceline &v) { 
    return os << "{ p:("<<v.p.x<<','<<v.p.y<<','<<v.p.z<<"), v:("<<v.v.x<<','<<v.v.y<<','<<v.v.z<<") }";
}

std::ostream &operator<<(std::ostream &os, const Plane &v) { 
    return os << "{ p:("<<v.p.x<<','<<v.p.y<<','<<v.p.z<<"), v:("<<v.n.x<<','<<v.n.y<<','<<v.n.z<<") }";
}

std::ostream &operator<<(std::ostream &os, const Basis &v) { 
    return os << "{ p:("<<v.p.x<<','<<v.p.y<<','<<v.p.z
              <<"), x:("<<v.x.x<<','<<v.x.y<<','<<v.x.z
              <<"), x:("<<v.y.x<<','<<v.y.y<<','<<v.y.z
 			  <<"), x:("<<v.z.x<<','<<v.z.y<<','<<v.z.z
              <<") }";
}

} //namespace Laxkit

#endif
