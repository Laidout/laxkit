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
//    Copyright (C) 2007 by Tom Lechner
//



#ifndef LANGUAGE_H
#define LANGUAGE_H


#if 1

 //yes gettext
#include <libintl.h>

#define _(str) gettext(str)
#define gettext_noop(str) str
#define N_(str) gettext_noop(str)
#else

 //no gettext
#define _(str) str
#define gettext_noop(str) str
#define N_(str) gettext_noop(str)

#endif


#endif



