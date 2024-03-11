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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
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

class IconManager : public Laxkit::anObject, public Laxkit::PtrStack<IconNode>
{
  protected:
	Laxkit::PtrStack<char> icon_path;
	Laxkit::PtrStack<char> broken; //stack of string ids

	virtual Laxkit::LaxImage *findicon(const char *name, bool save_broken);

  public:
	static IconManager* GetDefault();
	static IconManager* SetDefault(IconManager *newmanager);

	bool remember_broken;

	IconManager();
	virtual ~IconManager();
	virtual const char *whattype() { return "IconManager"; }

	virtual int InstallIcon(const char *nname, int nid, const char *file);
	virtual int InstallIcon(const char *nname, int nid, Laxkit::LaxImage *img);
	virtual Laxkit::LaxImage *GetIconByIndex(int index);
	virtual Laxkit::LaxImage *GetIcon(int id);
	virtual Laxkit::LaxImage *GetIcon(const char *name);
	virtual int HowMany();
	virtual void AddPath(const char *newpath);
	virtual int RemovePath(const char *oldpath);
	virtual int NumPaths() { return icon_path.n; }
	virtual const char *GetPath(int index);
	virtual int PreloadAll();

	virtual int NumBroken() { return broken.n; }
	virtual const char *Broken(int i);
	virtual int ScanForBroken();
};

} //namespace Laxkit


#endif


