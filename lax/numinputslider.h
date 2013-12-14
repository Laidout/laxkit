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
//    Copyright (C) 2004-2006 by Tom Lechner
//
#ifndef _LAX_NUMINPUTSLIDER_H
#define _LAX_NUMINPUTSLIDER_H

#include <lax/numslider.h>
#include <lax/lineedit.h>

namespace Laxkit {

class NumInputSlider : public NumSlider
{
  protected:
	LineEdit *le;
  public:
	NumInputSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
				   int xx,int yy,int ww,int hh,int brder,
				   anXWindow *prev,unsigned long nowner,const char *nsendthis,
				   const char *nlabel,int nmin,int nmax,int current=-10000);
//	~NumInputSlider();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int Event(const EventData *e,const char *mes);
};

} // namespace Laxkit

#endif

