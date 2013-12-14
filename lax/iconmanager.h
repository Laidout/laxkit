//
//	
// The Laxkit, a windowing toolkit
// Please consult http://laxkit.sourceforge.net about where to send any
// correspondence about this software.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Copyright (C) 2004-2007,2010 by Tom Lechner
//

#ifndef _LAX_ICONMANAGER_H
#define _LAX_ICONMANAGER_H

#include <lax/lists.h>
#include <lax/laximages.h>

namespace Laxkit {

//----------------------------- IconNode ---------------------------

class IconNode
{
 public:
	char *name;
	int id;
	Laxkit::LaxImage *image;
	IconNode(const char *nname, int nid, Laxkit::LaxImage *img);
	~IconNode();
};

//----------------------------- IconManager ---------------------------

class IconManager : public Laxkit::PtrStack<IconNode>
{
  protected:
	Laxkit::PtrStack<char> icon_path;
	Laxkit::LaxImage *findicon(const char *name);
  public:
	IconManager();
	int InstallIcon(const char *nname, int nid, const char *file);
	int InstallIcon(const char *nname, int nid, Laxkit::LaxImage *img);
	Laxkit::LaxImage *GetIconByIndex(int index);
	Laxkit::LaxImage *GetIcon(int id);
	Laxkit::LaxImage *GetIcon(const char *name);
	int HowMany();
	void addpath(const char *newpath);
};

} //namespace Laxkit


#endif


