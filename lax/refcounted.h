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
//    Copyright (C) 2013 by Tom Lechner
//
#ifndef _LAX_REFCOUNTED_H
#define _LAX_REFCOUNTED_H


namespace Laxkit {


class RefCounted
{
 protected:
	int _count;
  public:
	int suppress_debug;
	RefCounted();
	virtual ~RefCounted();
	virtual int inc_count();
	virtual int dec_count();
	virtual int the_count() { return _count; }
};

} // namespace Laxkit

#endif


