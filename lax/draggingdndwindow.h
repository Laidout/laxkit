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
//    Copyright (C) 2019 by Tom Lechner
//
#ifndef _LAX_DRAGGINGDNDWINDOW_H
#define _LAX_DRAGGINGDNDWINDOW_H


#include <lax/anxapp.h>

#include <functional>


namespace Laxkit {

class DraggingDNDWindow : public anXWindow
{
  protected:
	std::function<void(anXWindow *, Displayer *)> renderer;
	int mouse;
	flatpoint offset;

  public:
	DraggingDNDWindow(const char *nname, const char *ntitle,
			unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			unsigned long nowner,const char *nsend,
			int mouseid=-1, double noffsetx=0, double noffsety=0);
	virtual ~DraggingDNDWindow();
	virtual const char *whattype() { return "DraggingDNDWindow"; }
	virtual void Refresh();
	virtual int  Idle(int tid, double delta);

	virtual void SetRenderer(std::function<void(anXWindow *, Displayer *dp)> newrenderer);
	virtual void Done();
};


} //namespace Laxkit

#endif
