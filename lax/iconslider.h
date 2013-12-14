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
#ifndef _LAX_ICONSLIDER_H
#define _LAX_ICONSLIDER_H


#include <lax/itemslider.h>
#include <lax/menuinfo.h>
#include <sys/times.h>

#define ICONSLIDER_ICON_ONLY    (0<<20)
#define ICONSLIDER_TEXT_ONLY    (1<<20)
#define ICONSLIDER_TEXT_ICON    (2<<20)
#define ICONSLIDER_ICON_TEXT    (3<<20)
#define ICONSLIDER_WHAT_MASK    (3<<20)

#define ICONSLIDER_M_ICON_ONLY  (0<<22)
#define ICONSLIDER_M_TEXT_ONLY  (1<<22)
#define ICONSLIDER_M_TEXT_ICON  (2<<22)
#define ICONSLIDER_M_ICON_TEXT  (3<<22)
#define ICONSLIDER_M_WHAT_MASK  (3<<22)

namespace Laxkit {

class IconSlider : public ItemSlider
{
 protected:
	MenuInfo *items;
	char iislocal;
	int lastitem;
 public:
	int gap;
	IconSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsendthis,
		MenuInfo *nitems,int ilocal);
	virtual ~IconSlider();
	virtual int DeleteItem(int id);
	virtual int AddItem(const char *newitem,int nid);
	virtual int AddItem(const char *newitem,const char *filename,int nid);
	virtual void Refresh();
};


} // namespace Laxkit

#endif

