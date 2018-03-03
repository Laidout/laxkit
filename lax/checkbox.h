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
#ifndef _LAX_CHECKBOX_H
#define _LAX_CHECKBOX_H

#include <lax/button.h>
#include <lax/rectangles.h>

namespace Laxkit {

 // circle and squares automatically set Toggle, overriding style passed to constructor
#define CHECK_SQUARE_CHECK  (1<<18)
#define CHECK_SQUARE_BUT    (1<<19)
#define CHECK_SQUARE_X      (1<<20)
#define CHECK_CIRCLE        (1<<21)

#define CHECK_LEFT          (1<<22)
#define CHECK_RIGHT         (1<<23)
#define CHECK_CENTER        (1<<24)
#define CHECK_CENTERL       (1<<25)
#define CHECK_CENTERR       (1<<26)


class CheckBox : public Button
{ 
  protected:
	IntRectangle grect,trect;
	unsigned long pitcolor;
  public:	
	CheckBox(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
				int xx,int yy,int ww,int hh,int brder,
				anXWindow *prev,unsigned long nowner,const char *nsendmes,
				const char *nnme=NULL,int npadx=0,int npady=0);
	virtual ~CheckBox();
	virtual const char *Label(const char *nlabel);
	virtual void draw();
	virtual void setPlacement();
	virtual void drawgraphic();
	virtual int MoveResize(int nx,int ny,int nw,int nh);
    virtual int Resize(int nw,int nh);
};

} // namespace Laxkit

#endif

