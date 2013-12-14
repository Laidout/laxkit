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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_FILEMENUINFO_H
#define _LAX_FILEMENUINFO_H

#include <lax/menuinfo.h>
#include <sys/stat.h>

namespace Laxkit {

class FileMenuItem : public MenuItem
{
 public:
	struct stat statbuf;
	FileMenuItem(const char *newitem,unsigned int nstate,int followlinks=1);
	virtual MenuInfo *GetSubmenu(int create=0);
	virtual MenuInfo *CreateSubmenu(const char *pathtodir);
	virtual MenuInfo *ExtractMenu();
};

} // namespace Laxkit

#endif

