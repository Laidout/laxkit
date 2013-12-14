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



#include <lax/numslider.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>

#include <iostream>
using namespace std;

namespace Laxkit {

/*! \class NumSlider
 * \brief A slider control specifically for selecting integer numbers within a range.
 *
 * The range is [min,max]. If NUMSLIDER_WRAP is part of win_style, then sliding through
 * numbers wraps around, rather than stops at min or max.
 *
 * See NumInputSlider for a class that adds the ability to type in a value in addition
 * to the sliding feature of this class.
 *
 * \todo  optional label:   [ Blah: 34 ]
 * \todo super shift/ shift-but4 to move faster*** put that in itemslider
 */


NumSlider::NumSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsendthis,const char *nlabel,int nmin,int nmax,int cur)
	: ItemSlider(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendthis)
{
	nitems=0;
	min=nmin;
	max=nmax;
	curitem=cur;
	if (curitem<min) curitem=min;
	else if (curitem>max) curitem=max;
	lastitem=-1;
	movewidth=2;
	label=NULL;
	makestr(label,nlabel);

	installColors(app->color_panel);

	if (win_w==0 || win_h==0) wraptoextent();
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

int NumSlider::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown && buttondowndevice==d->id && x>=0 && x<win_w && y>0 && y<win_h) needtodraw=1;
	return ItemSlider::MouseMove(x,y,state,d);
}

/*! \todo *** show a faint arrow to say what will happen when you click on
 * where mouse is hovering
 */
void NumSlider::Refresh()
{ 
	if (!win_on || !needtodraw) return;
	clear_window(this);
	int x,y;
	unsigned int state;
	if (buttondown) {
		mouseposition(buttondowndevice, this,&x,&y,&state,NULL);
		if (x>=0 && x<win_w && y>0 && y<win_h) {
			if (x<win_w/2) {
				 // draw left arrow
				foreground_color(coloravg(win_colors->bg,win_colors->fg,.1));
				draw_thing(this, win_w/4,win_h/2, win_w/4,win_h/2,1, THING_Triangle_Left);
			} else {
				 // draw right arrow
				foreground_color(coloravg(win_colors->bg,win_colors->fg,.1));
				draw_thing(this, win_w*3/4,win_h/2, win_w/4,win_h/2,1, THING_Triangle_Right);
			}
		}
	}
	foreground_color(win_colors->fg);
	char num[20+(label?strlen(label):0)];
	if (label) sprintf(num,"%s%d",label,curitem); else sprintf(num,"%d",curitem);
	textout(this, num,-1,win_w/2,win_h/2,LAX_CENTER);
	needtodraw=0;
}

int NumSlider::SelectPrevious()
{ 
	curitem--;
	if (curitem<min) {
		if (win_style&NUMSLIDER_WRAP) curitem=max;
		else curitem=min;
	}
	if (win_style & ITEMSLIDER_SENDALL) send();
	needtodraw=1;
	return curitem;
}

int NumSlider::SelectNext()
{
	curitem++;
	if (curitem>max) {
		if (win_style&NUMSLIDER_WRAP) curitem=min;
		else curitem=max;
	}
	if (win_style & ITEMSLIDER_SENDALL) send();
	needtodraw=1;
	return curitem;
}

//! Select number nn. Does nothing if out of bounds. Returns current number.
int NumSlider::Select(int nn)
{
	if (curitem==nn || nn<min || nn>max) return curitem;
	curitem=nn;
	needtodraw=1;
	if (win_style & ITEMSLIDER_SENDALL) send();
	return curitem;
}

} // namespace Laxkit

