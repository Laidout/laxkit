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
//    Copyright (C) 2004-2010,2014 by Tom Lechner
//



#include <lax/numslider.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/lineedit.h>

#include <iostream>
using namespace std;

namespace Laxkit {

/*! \class NumSlider
 * \brief A slider control specifically for selecting integer numbers within a range.
 *
 * The range is [min,max]. If NUMSLIDER_WRAP is part of win_style, then sliding through
 * numbers wraps around, rather than stops at min or max.
 *
 * \todo  optional label:   [ Blah: 34 ]
 * \todo super shift/ shift-but4 to move faster*** put that in itemslider
 */


NumSlider::NumSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsendthis,const char *nlabel,int nmin,int nmax,int cur)
	: ItemSlider(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendthis)
{
	mode=0;
	nitems=0;
	min=nmin;
	max=nmax;
	curitem=cur;
	if (curitem<min) curitem=min;
	else if (curitem>max) curitem=max;
	lastitem=-1;
	movewidth=2;
	label=NULL;
	labelbase=NULL;
	makestr(label,nlabel);

	installColors(app->color_panel);

	if (win_w==0 || win_h==0) wraptoextent();
}

NumSlider::~NumSlider()
{
	delete[] label;
	delete[] labelbase;
}

/*! Use a printf style format string to create label from integer curitem and nlabelbase.
 * nlabelbase MUST have a signed integer format in it (such as "%d").
 *
 * nlabelbase==NULL means use label as normal, and delete the labelbase.
 */
const char *NumSlider::LabelBase(const char *nlabelbase)
{
	makestr(labelbase,nlabelbase);
	needtodraw=1;
	return labelbase;
}

//! Change the label. If nlabel==NULL, then remove the label. Returns label.
const char *NumSlider::Label(const char *nlabel)
{
	needtodraw=1;
	return makestr(label,nlabel);
}

//! Find the maximum extent of the items, and set win_w,win_h to them if they are 0.
void NumSlider::wraptoextent()
{
	char num[20+(label?strlen(label):0)];
	double x,y,x2,y2;
	if (label) sprintf(num,"%s%d",label,max); else sprintf(num,"%d",max);
	getextent(num,-1,&x,&y);
	if (label) sprintf(num,"%s%d",label,min); else sprintf(num,"%d",min);
	getextent(num,-1,&x2,&y2);
	if (x2>x) x=x2;
	if (y2>y) y=y2;
	if (win_w==0) win_w=x;
	if (win_h==0) win_h=y;
}

//int NumSlider::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
//{
//}

void NumSlider::Refresh()
{ 
	if (!win_on || !needtodraw) return;
	clear_window(this);


	 //draw arrows
	int ww=win_w/2;
	int hh=win_h/2;
	if (win_style&EDITABLE) ww=text_height();

	 // draw left arrow
	foreground_color(coloravg(win_colors->bg,win_colors->fg,.2));
	draw_thing(this, ww/2,win_h/2, ww/2,hh/2, hover==LAX_LEFT?1:0, THING_Triangle_Left);

	 // draw right arrow
	draw_thing(this, win_w-ww/2,win_h/2, ww/2,hh/2, hover==LAX_RIGHT?1:0, THING_Triangle_Right);


	 //draw number
	if (hover==LAX_CENTER) {
		foreground_color(coloravg(win_colors->bg,win_colors->fg,.1));
		hh=text_height()*1.1;
		fill_rectangle(this, ww,win_h/2-hh/2,win_w-2*ww,hh);
	}
	foreground_color(win_colors->fg);
	char num[20+((labelbase?labelbase:label)?strlen((labelbase?labelbase:label)):0)];
	if (labelbase) sprintf(num,labelbase,curitem);
	else if (label) sprintf(num,"%s%d",label,curitem);
	else sprintf(num,"%d",curitem);

	textout(this, num,-1,win_w/2,win_h/2,LAX_CENTER);
	needtodraw=0;
}

int NumSlider::SelectPrevious(double multiplier)
{ 
	curitem-=(int)multiplier;
	if (curitem<min) {
		if (win_style&WRAP) curitem=max;
		else curitem=min;
	}
	if (win_style & SENDALL) send();
	needtodraw=1;
	return curitem;
}

int NumSlider::SelectNext(double multiplier)
{
	curitem+=(int)multiplier;
	if (curitem>max) {
		if (win_style&WRAP) curitem=min;
		else curitem=max;
	}
	if (win_style & SENDALL) send();
	needtodraw=1;
	return curitem;
}

//! Select number nn. Does nothing if out of bounds. Returns current number.
int NumSlider::Select(int nn)
{
	if (curitem==nn || nn<min || nn>max) return curitem;
	curitem=nn;
	needtodraw=1;
	if (win_style & SENDALL) send();
	return curitem;
}

int NumSlider::Mode(int newmode)
{
	if (newmode==1 && mode!=1) {
		 //edit current number
		char num[20];
		sprintf(num,"%d",curitem);
		LineEdit *le=new LineEdit(this,"inputedit",NULL,
			 LINEEDIT_DESTROY_ON_ENTER,
			 0,0, win_w-4,win_h-4,2,
			 NULL,object_id,"lineedit",
			 num,0);
		le->SetSelection(0,-1);
		le->padx=5;
		app->addwindow(le);
		if (!le->xlib_window) return 0;
		app->setfocus(le,0,NULL);//***

		return 1;

	} else {
		mode=0;
		anXWindow *w=findChildWindowByName("inputedit");
		if (w) app->destroywindow(w);
	}

	needtodraw=1;
	return mode;
}

//! Catches when the lineedit receives enter..
int NumSlider::Event(const EventData *e,const char *mes)
{
	if (strcmp(mes,"lineedit")) return ItemSlider::Event(e,mes);

	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
	if (!s) return ItemSlider::Event(e,mes);

	const char *blah=s->str;
	char *tmp;

	int ncuritem=strtol(blah,&tmp,10);
	if (ncuritem<min) ncuritem=min;
	else if (ncuritem>max) ncuritem=max;
	if (tmp!=blah) { 
		curitem=ncuritem; 
		needtodraw=1; 
		send();
	}
	//app->destroywindow(le);

	return 0;
}


} // namespace Laxkit

