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
//    Copyright (C) 2020 by Tom Lechner
//


#include <cstring>
#include <lax/draggableframe.h>
#include <lax/laxutils.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

/*! \class DraggableFrame
 * Resizeable frame with a single child.
 */
/*! \var int DraggableFrame::ex
 * \brief The x extent of the text.
 */

enum WHERE {
	WHERE_None,
	WHERE_TopLeft,
	WHERE_Top,
	WHERE_TopRight,
	WHERE_Right,
	WHERE_BottomRight,
	WHERE_Bottom,
	WHERE_BottomLeft,
	WHERE_Left,
	WHERE_Middle
};

DraggableFrame::DraggableFrame(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder) 
				: anXWindow(pwindow,nname,ntitle,nstyle, nx,ny, nw,nh,brder, NULL,0,NULL) 
{
	allow_drag = true;
	allow_x_resize = true;
	allow_y_resize = true;
	keep_in_parent = true;
	what = hover = WHERE_None;

	InstallColors(THEME_Panel);
	double th = win_themestyle->normal->textheight();
	pad = th/2;
	minx = miny = 2*th;

	child = nullptr;
}

DraggableFrame::~DraggableFrame()
{
}

int DraggableFrame::Event(const EventData *e,const char *mes)
{
	//if (e->type==LAX_onMouseOut && (win_style&MB_LEAVE_DESTROYS)) {

	if (e->type==LAX_onMouseOut) {
		cout << " --out-- "<<endl;
		if (!buttondown.any()) { hover = WHERE_None; needtodraw = 1; }
	}
	return anXWindow::Event(e,mes);
}

int DraggableFrame::init()
{
	return 0;
}

void DraggableFrame::Refresh()
{
	if (!win_on || !needtodraw) return;

	Displayer *dp = MakeCurrent();
	dp->NewFG(win_themestyle->bg);
	dp->ClearWindow();

	dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg));

	if (hover == WHERE_Middle) {
		dp->drawrectangle(0,0,win_w,win_h, 1);

	} else if (hover == WHERE_Top)         {
		dp->drawrectangle(0,0,win_w,pad, 1);

	} else if (hover == WHERE_Bottom)      {
		dp->drawrectangle(0,win_h-pad,win_w,pad, 1);

	} else if (hover == WHERE_Left)        {
		dp->drawrectangle(0,0,pad,win_h, 1);

	} else if (hover == WHERE_Right)       {
		dp->drawrectangle(win_w-pad,0, pad,win_h, 1);
	 
	} else if (hover == WHERE_TopLeft)     {
		dp->drawrectangle(0,0,pad,win_h, 1);
		dp->drawrectangle(0,0,win_w,pad, 1);

	} else if (hover == WHERE_TopRight)    {
		dp->drawrectangle(0,0,win_w,pad, 1);
		dp->drawrectangle(win_w-pad,0, pad,win_h, 1);

	} else if (hover == WHERE_BottomLeft)  {
		dp->drawrectangle(0,0,pad,win_h, 1);
		dp->drawrectangle(0,win_h-pad,win_w,pad, 1);

	} else if (hover == WHERE_BottomRight) {
		dp->drawrectangle(win_w-pad,0, pad,win_h, 1);
		dp->drawrectangle(0,win_h-pad,win_w,pad, 1);
	}


	needtodraw=0;
	return;
}

int DraggableFrame::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!allow_drag) return 0;

	what = scan(x,y,state);
	hover = what;

	if (what != WHERE_None) {
		//translate_window_coordinates(this, x, y, nullptr, &x, &y, nullptr); //get screen coordinates
		mouseposition(d->id, nullptr, &x, &y, &state,nullptr);
		buttondown.down(d->id, LEFTBUTTON, x,y, what);
	}
	needtodraw = 1;
	return 0;
}

int DraggableFrame::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, LEFTBUTTON);
	what = hover = WHERE_None;
	needtodraw = 1;
	return 0;
}

/*! If allowed with MB_MOVE, this shifts around text that is smaller
 * than the window only within the window boundaries, and shifts 
 * around larger text only just enough so that you can see it.
 */
int DraggableFrame::MouseMove(int xx,int yy,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.any(d->id)) {
		int nhover = scan(xx,yy,state);
		if (nhover != hover) { hover = nhover; needtodraw = 1; }
		return 1;
	}

	//translate_window_coordinates(this, xx, yy, nullptr, &xx, &yy, nullptr); //get screen coordinates
	mouseposition(d->id, nullptr, &xx, &yy, &state,nullptr);

	int what;
	int dx, dy;
	buttondown.move(d->id, xx,yy, &dx, &dy);
	buttondown.getextrainfo(d->id, LEFTBUTTON, &what);
	if (what == WHERE_None) return 0;
	if (!(state & ShiftMask)) what = WHERE_Middle;

	dx = xx - dx;
	if (!allow_x_resize) dx = 0;
	dy = yy - dy;
	if (!allow_y_resize) dy = 0;
	DBG cerr << "DraggableFrame window x,y: "<<xx<<','<<yy<<"   dx,dy: "<<dx<<','<<dy<<endl;

	int x=win_x, y=win_y, w=win_w, h=win_h;

	if (what == WHERE_Middle)           { x += dx; y += dy; }

	else {
		if (what == WHERE_Top || what == WHERE_TopLeft || what == WHERE_TopRight) {
			if (h-dy > miny) { y += dy; h -= dy; }

		} else if (what == WHERE_Bottom || what == WHERE_BottomLeft || what == WHERE_BottomRight) {
			h += dy;
		}

		if (what == WHERE_Left || what == WHERE_TopLeft || what == WHERE_BottomLeft) {
			if (w-dx > minx) { x += dx; w -= dx; }

		} else if (what == WHERE_Right || what == WHERE_BottomRight || what == WHERE_TopRight) {
			w += dx;
		}
	}

	if (w < minx) w = minx;
	if (h < miny) h = miny;

	if (win_parent) {
		if (x + w > win_parent->win_w) x = win_parent->win_w - w;
		else if (x < 0) x = 0;
		if (y + h > win_parent->win_h) y = win_parent->win_h - h;
		else if (y < 0) y = 0;
	}

	//int oldx = win_x, oldy = win_y;
	MoveResize(x,y,w,h);
	//buttondown.move(d->id, xx - (win_x - oldx), yy - (win_y - oldy));

	needtodraw=1;
	return 0;
}

int DraggableFrame::MoveResize(int nx,int ny,int nw,int nh)
{
	DBG int oldx = win_x, oldy = win_y;
	anXWindow::MoveResize(nx,ny,nw,nh);
	DBG cerr << "DraggableFrame ---------------------------------MoveResize after anxw::mr x,y: "
	DBG 	<<win_x<<','<<win_y<<"   w,h: "<<win_w<<','<<win_h<<"   dx,dy: "<<(win_x - oldx)<<','<<(win_y - oldy)<<endl;
	SyncChild();
	needtodraw=1;
	return 0;
}

int DraggableFrame::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	DBG cerr << "DraggableFrame ------------------------------------Resize after anxw::mr x,y: "<<win_x<<','<<win_y<<"   w,h: "<<win_w<<','<<win_h<<endl;
	SyncChild();
	needtodraw=1;
	return 0;
}

void DraggableFrame::SyncChild()
{
	if (!child) return;
	child->MoveResize(pad,pad, win_w - 2*pad, win_h - 2*pad);
}

int DraggableFrame::SetChild(anXWindow *win)
{
	if (child) {
		app->destroywindow(child);
	}
	child = win;
	if (child) app->reparent(child, this);

	return 0;
}

int DraggableFrame::scan(int x, int y, unsigned int state)
{
	//if (!(state & ShiftMask)) return WHERE_Middle;

	int xpad = win_w / 4;
	int ypad = win_h / 4;
	if (x < xpad) {
		if (y < ypad) return WHERE_TopLeft;
		if (y > win_h - ypad) return WHERE_BottomLeft;
		if (x >= 0) return WHERE_Left;

	} else if (x > win_w - xpad) {
		if (y < xpad) return WHERE_TopRight;
		if (y > win_h - ypad) return WHERE_BottomRight;
		if (x < win_w) return WHERE_Right;

	} else if (y >= 0 && y < ypad) return WHERE_Top;
	else if (y >= win_h-ypad && y < win_h) return WHERE_Bottom;
	else if (x >= 0 && x < win_w && y >= 0 && y < win_h) return WHERE_Middle;

	return WHERE_None;
}


/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *DraggableFrame::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	if (what==-1) {
		att->push("pad","int");
		att->push("halign","one of: left center right");
		att->push("valign","one of: left center right");
		return att;
	}

	att->push("pad",pad);
	att->push("minx",minx);
	att->push("miny",miny);
	if (allow_drag) att->push("allow_drag");
	if (allow_x_resize) att->push("allow_x_resize");
	if (allow_y_resize) att->push("allow_y_resize");

	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void DraggableFrame::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"pad")) {
			DoubleAttribute(value, &pad);

		} else if (!strcmp(name,"minx")) {
			DoubleAttribute(value, &minx);

		} else if (!strcmp(name,"miny")) {
			DoubleAttribute(value, &miny);

		} else if (!strcmp(name,"allow_drag")) {
			allow_drag = BooleanAttribute(value);

		} else if (!strcmp(name,"allow_x_resize")) {
			allow_x_resize = BooleanAttribute(value);

		} else if (!strcmp(name,"allow_y_resize")) {
			allow_y_resize = BooleanAttribute(value);
		}
	}
}


} // namespace Laxkit

