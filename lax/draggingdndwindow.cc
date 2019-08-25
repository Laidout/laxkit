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


#include <lax/draggingdndwindow.h>
#include <lax/laxutils.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

/*! \class DraggingDNDWindow
 * Window that follows the mouse around and has a custom render.
 */

DraggingDNDWindow::DraggingDNDWindow(const char *nname, const char *ntitle,
			unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			unsigned long nowner,const char *nsend,
			int mouseid, double noffsetx, double noffsety)
  //: anXWindow(nullptr, nname, ntitle, ANXWIN_BARE | nstyle, xx,yy,ww,hh,brder, nullptr,nowner,nsend)
  : anXWindow(nullptr, nname, ntitle,  nstyle, xx,yy,ww,hh,brder, nullptr,nowner,nsend)
{
	mouse = mouseid;
	offset.x = noffsetx;
	offset.y = noffsety;

	app->addtimer(this, 50,50,-1);
	//virtual int addtimer(EventReceiver *win,int strt,int next,int duration);

	if (mouse <= 0) {
		LaxMouse *m = app->devicemanager->findMouse(0);
		if (m) mouse = m->id;
	}
}

DraggingDNDWindow::~DraggingDNDWindow()
{
}

int DraggingDNDWindow::Idle(int tid, double delta)
{
	 // update position
	int mx,my;
	mouseposition(mouse, nullptr, &mx, &my, nullptr, nullptr);
	MoveResize(mx + offset.x, my + offset.y, win_w,win_h);
	DBG cerr << "Dragging dnd window at pos "<<mx<<','<<my<<"  w,h: "<<win_w<<','<<win_h<<endl;
	return 0;
}

void DraggingDNDWindow::Refresh()
{
	if (!win_on || !needtodraw) return;
	needtodraw = 0;
	if (!renderer) return;

	Displayer *dp = MakeCurrent();
	renderer(this, dp);
}


void DraggingDNDWindow::SetRenderer(std::function<void(anXWindow *, Displayer *dp)> newrenderer)
{
	renderer = newrenderer;
}

void DraggingDNDWindow::Done()
{
	app->destroywindow(this);
}


} //namespace Laxkit

