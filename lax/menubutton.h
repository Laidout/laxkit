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
//    Copyright (C) 2004-2007,2010-2011,2013 by Tom Lechner
//
#ifndef _LAX_MENUBUTTON_H
#define _LAX_MENUBUTTON_H


#include <lax/button.h>
#include <lax/menuinfo.h>

#define MENUBUTTON_DOWNARROW         (1<<22)
#define MENUBUTTON_CLICK_CALLS_OWNER (1<<23)
#define MENUBUTTON_LEFT              (1<<24)
#define MENUBUTTON_RIGHT             (1<<25)
#define MENUBUTTON_SEND_STRINGS      (1<<26)

#define MENUBUTTON_ICON_ONLY         (1<<27)
#define MENUBUTTON_TEXT_ONLY         (1<<28)
#define MENUBUTTON_TEXT_ICON         (1<<29)
#define MENUBUTTON_ICON_TEXT         (1<<30)

namespace Laxkit {


class MenuButton : public Button
{ 
 protected:
	MenuInfo *menuinfo;
 public:	
	unsigned long menubutton_style;
	MenuButton(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,int nid=-1,
						MenuInfo *menu=NULL, int absorb=1,
						const char *nlabel=NULL,
						const char *filename=NULL,LaxImage *img=NULL,
						int npad=0,int ngap=0);

	virtual ~MenuButton();
	virtual const char *whattype() { return "MenuButton"; }
	virtual int LBDown(int x,int y,unsigned int wstate,int count,const LaxMouse *d);
	virtual int SetMenu(MenuInfo *menu, int absorb);
	virtual void makePopup(int mouseid);
};

} // namespace Laxkit

#endif

