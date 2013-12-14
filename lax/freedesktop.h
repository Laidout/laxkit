//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Copyright (C) 2007 by Tom Lechner
//
#ifndef _LAX_FREEDESKTOP_H
#define _LAX_FREEDESKTOP_H

#include <lax/attributes.h>

namespace LaxFiles {


int touch_recently_used(const char *file, const char *mime, const char *group, const char *timestamp);
Attribute *recently_used_att(const char *file="~/.recently-used");
char *recently_used(const char *mimetype,const char *group, int includewhat);

char *get_bookmarks(const char *file,const char *filetype);
int add_bookmark(const char *directory, int where);

char *freedesktop_thumbnail(const char *file, char which='n');



} //namespace LaxFiles

#endif


