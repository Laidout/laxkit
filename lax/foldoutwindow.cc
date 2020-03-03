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

#include <lax/FoldoutWindow.h>

#include <iostream>
#define DBG
using namespace std;


namespace Laxkit {


/*! \class FoldoutWindow
 * \brief Class to contain one subwindow that can be expanded or collapsed.
 */


FoldoutWindow::FoldoutWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend,
		const char *nlabel, anXWindow *subwindow)
	: anXWindow (parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	thewindow = NULL;
	expanded = false;
	InstallColors(THEME_Panel);

	double th = win_themestyle->normal->textheight();
	pad = th/2;
	labelheight = th*1.2;
	label = newstr(nlabel);

	lbdown = false;
}

FoldoutWindow::~FoldoutWindow()
{
	delete[] label;
}

//! Send on a user induced expand or collapse.
int FoldoutWindow::send()
{
	if (!win_owner || isblank(win_sendthis)) return 1;
	SimpleMessage *message = new SimpleMessage();
	message->info1 = expanded;
	app->SendMessage(message, win_owner, win_sendthis, object_id);
	return 0;
}

int FoldoutWindow::init()
{
	return 0;
}

//! Use nwindow as the nested window.
/*! An old window must be disposed of, and nwindow installed, which
 * means reparenting to this.
 *
 * Returns 0 on success. If nwindow is NULL, nothing is done and 1 is returned.
 * 
 * The old window is app->destroywindow'd.
 */
int FoldoutWindow::UseThisWindow(anXWindow *nwindow)
{
	if (!nwindow) return 1;
	if (nwindow != thewindow) {
		if (thewindow) {
			//*** remove from stack
			app->destroywindow(thewindow);
		}
	}

	thewindow = nwindow;
	panner->tell(thewindow);
	app->reparent(thewindow,this); // app takes care of whether kid/parent->windows exist yet.
	SyncWindows();
	return 0;
}

int FoldoutWindow::Event(const EventData *e,const char *mes)
{
	return anXWindow::Event(e,mes);
}


//! Just calls anXWindow::MoveResize, then syncWindows().
int FoldoutWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	needtosync = true;
	//SyncWindow();
	return 0;
}

//! Just calls anXWindow::Resize, then syncWindows().
int FoldoutWindow::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	needtosync = true;
	//SyncWindow();
	return 0;
}

void FoldoutWindow::SyncWindows()
{
	if (!expanded) {
		ph(labelheight);
		return;
	}
	if (!thewindow) return;

	ph(labelheight + thewindow->win_h);
	thewindow->MoveResize(0,labelheight, win_w,win_h - labelheight);
}

int FoldoutWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	lbdown = false;
	if (y >= 0 && y < labelheight) {
		lbdown = true;
		needtodraw = 1;
	}

	return 0;
}

int FoldoutWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (lbdown && y >= 0 && y < labelheight) {
		if (expanded) Collapse();
		else Expand();
		send();
	}

	lbdown = false;
	return 0;
}

void FoldoutWindow::Expand()
{
	expanded = true;
	needtosync = true;
}

void FoldoutWindow::Collapse()
{
	expanded = false;
	needtosync = true;
}

void FoldoutWindow::Refresh()
{
	if (needtosync) SyncWindows();

	if (!needtodraw) return;
	needtodraw = 0;

	double th = win_themestyle->normal->textheight();
	Displayer *dp = MakeCurrent();
	dp->ClearWindow();
	dp->NewFG(win_themestyle->fg);

	//draw arrow
	if (expanded) dp->drawthing(labelheight/2,labelheight/2, labelheight/2,labelheight/2, 1, THING_Triangle_Down);
	else dp->drawthing(labelheight/2,labelheight/2, labelheight/2,labelheight/2, 1, THING_Triangle_Right);

	//draw label
	if (label) dp->textout(labelheight,labehheight/2, label,-1, LAX_LEFT | LAX_VCENTER);
}


} // namespace Laxkit

