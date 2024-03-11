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
//    Copyright (C) 2004-2007,2010-2011,2013 by Tom Lechner
//
#ifndef _LAX_MENUBUTTON_H
#define _LAX_MENUBUTTON_H


#include <lax/button.h>
#include <lax/menuinfo.h>



 //these must not collide with Button defines
#define MENUBUTTON_DOWNARROW         (1<<23)
#define MENUBUTTON_CLICK_CALLS_OWNER (1<<24)
#define MENUBUTTON_LEFT              (1<<25)
#define MENUBUTTON_RIGHT             (1<<26)
#define MENUBUTTON_SEND_STRINGS      (1<<27)
#define MENUBUTTON_HOVER_FILL        (1<<28)

 //these are just duplicates of same in Button
#define MENUBUTTON_ICON_ONLY         (1<<18)
#define MENUBUTTON_TEXT_ONLY         (1<<19)
#define MENUBUTTON_TEXT_ICON         (1<<20)
#define MENUBUTTON_ICON_TEXT         (1<<21)
#define MENUBUTTON_FLAT              (1<<22)


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
						int npad=-1,int ngap=-1);

	virtual ~MenuButton();
	virtual const char *whattype() { return "MenuButton"; }
	virtual int LBDown(int x,int y,unsigned int wstate,int count,const LaxMouse *d);
	virtual int SetMenu(MenuInfo *menu, int absorb);
	virtual void makePopup(int mouseid);
};

} // namespace Laxkit

#endif

