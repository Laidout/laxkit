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
 * \brief A slider control specifically for selecting integer or double numbers within a range.
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
	mode = MODE_Normal;
    nitems = 0;

    min  = nmin;
    max  = nmax;
    step = 1;

    curnum = cur;
    if      (!(win_style & NO_MINIMUM) && curnum < min) curnum = min;
    else if (!(win_style & NO_MAXIMUM) && curnum > max) curnum = max;

    curitem = curnum;

    lastitem  = -1;
    movewidth = 2;
    label     = NULL;
    labelbase = NULL;
    makestr(label, nlabel);

    InstallColors(THEME_Panel);

    if (win_w <= 0 || win_h <= 0)
      WrapToExtent();
}

NumSlider::NumSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsendthis,const char *nlabel,double nmin,double nmax,double cur, double nstep)
	: ItemSlider(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendthis)
{
	mode   = MODE_Normal;
	nitems = 0;

	win_style |= DOUBLES;
	min  = nmin;
	max  = nmax;
	step = nstep;

	curnum = cur;
	if      (!(win_style & NO_MINIMUM) && curnum < min) curnum = min;
	else if (!(win_style & NO_MAXIMUM) && curnum > max) curnum = max;

	curitem = curnum;

	lastitem  = -1;
	movewidth = 2;
	label     = NULL;
	labelbase = NULL;
	makestr(label, nlabel);

	InstallColors(THEME_Panel);

	if (win_w == 0 || win_h == 0) WrapToExtent();
}

NumSlider::~NumSlider()
{
	delete[] label;
	delete[] labelbase;
}

/*! Use a printf style format string to create label from integer curitem and nlabelbase.
 * nlabelbase MUST have a signed integer format in it (such as "%d"), unless
 * you are using DOUBLES style, in which case, it MUST have a double capable formatter,
 * such as "%f" or "%g".
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
void NumSlider::WrapToExtent()
{
	char num[30+(label?strlen(label):0)];
	double ex,ey,ex2,ey2;

	if (label) sprintf(num,"%s%dMM",label,(int)max); else sprintf(num,"%dMM",(int)max);
	win_themestyle->normal->Extent(num,-1,&ex,&ey,NULL,NULL);
	if (label) sprintf(num,"%s%dMM",label,(int)min); else sprintf(num,"%dMM",(int)min);
	win_themestyle->normal->Extent(num,-1,&ex2,&ey2,NULL,NULL);

	if (ex2 > ex) ex = ex2;
	if (ey2 > ey) ey = ey2;
	ex *= UIScale();
	ey *= UIScale();
	double pad = ey * .1;
	if (win_w == 0) win_w = ex + 2*pad;
	if (win_h == 0) win_h = ey + 2*pad;
}

//int NumSlider::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
//{
//}

void NumSlider::Refresh()
{ 
	if (!win_on || !needtodraw) return;


	Displayer *dp = MakeCurrent();
	if (hover) {
		dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.07));
		dp->drawrectangle(0,0,win_w,win_h, 1);
	} else dp->ClearWindow();

	dp->font(win_themestyle->normal, UIScale() * win_themestyle->normal->textheight());

	 //draw arrows
	int ww = win_w/2;
	int hh = win_h/2;
	if (win_style & EDITABLE) ww = UIScale() * win_themestyle->normal->textheight();

	 // draw left arrow
	dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.2));
	dp->drawthing(hh/2,win_h/2, hh/2,hh/2, hover==LAX_LEFT?1:0, THING_Triangle_Left);

	 // draw right arrow
	dp->drawthing(win_w-hh/2,win_h/2, hh/2,hh/2, hover==LAX_RIGHT?1:0, THING_Triangle_Right);


	 //draw number
	if (hover==LAX_CENTER) {
		dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.2));
		hh = UIScale() * win_themestyle->normal->textheight() * 1.1;
		dp->drawrectangle(ww,win_h/2-hh/2,win_w-2*ww,hh, 1);
	}

	dp->NewFG(win_themestyle->fg);
	char num[31+((labelbase?labelbase:label)?strlen((labelbase?labelbase:label)):0)];
	if (labelbase) sprintf(num,labelbase,curnum);
	else {
		if (win_style&DOUBLES) {
			if (label) sprintf(num,"%s %g",label,curnum);
			else sprintf(num,"%g",curnum);
		} else {
			if (label) sprintf(num,"%s %d",label,int(curnum+.5));
			else sprintf(num,"%d",int(.5+curnum));
		}
	}

	dp->textout(win_w/2,win_h/2, num,-1, LAX_CENTER);
	needtodraw=0;
}

int NumSlider::SelectPrevious(double multiplier)
{ 
	curnum-=multiplier*step;
	if (!(win_style&DOUBLES)) curnum=int(curnum+.5);

	if (!(win_style&NO_MINIMUM) && curnum<min) {
		if (win_style&WRAP) curnum=max;
		else curnum=min;
	}

	curitem=curnum;

	if (win_style & SENDALL) send();
	needtodraw=1;
	return curitem;
}

int NumSlider::SelectNext(double multiplier)
{
	curnum+=multiplier*step;
	if (!(win_style&DOUBLES)) curnum=int(curnum+.5);

	if (!(win_style&NO_MAXIMUM) && curnum>max) {
		if (win_style&WRAP) curnum=min;
		else curnum=max;
	}

	curitem=curnum;

	if (win_style & SENDALL) send();
	needtodraw=1;
	return curitem;
}

//! Select number nn. Does nothing if out of bounds. Returns current number.
int NumSlider::Select(int nn)
{
	if (curitem==nn || nn<min || nn>max) return curitem;
	curnum=nn;
	curitem=curnum;

	needtodraw=1;
	if (win_style & SENDALL) send();
	return curitem;
}

//! Select number nn. Does nothing if out of bounds. Returns current number.
int NumSlider::Select(double nn)
{
	if (!(win_style&NO_MINIMUM) && nn<min) return curitem;
	else if (!(win_style&NO_MAXIMUM) && nn>max) return curitem;
	if (curnum==nn) return curitem;

	curnum=nn;
	curitem=nn;

	needtodraw=1;
	if (win_style & SENDALL) send();
	return curitem;
}

/*! Mode can be 1 for editing number in a box, or 0 for normal.
 */
int NumSlider::Mode(int newmode)
{
	if (newmode == MODE_Editing && mode != MODE_Editing) {
		// activate editor for current number
		char num[30];
		if (win_style & DOUBLES) sprintf(num,"%.8g", curnum);
		else sprintf(num, "%d", (int)curnum);

		double th = UIScale() * win_themestyle->normal->textheight();

		LineEdit *le = new LineEdit(this, "inputedit", nullptr,
			 ANXWIN_OUT_CLICK_DESTROYS|LINEEDIT_DESTROY_ON_ENTER|((win_style&DOUBLES) ? LINEEDIT_FLOAT : LINEEDIT_INT),
			 th/2,0, win_w - th, win_h - th*.15, 2,
			 nullptr,object_id,"lineedit",
			 num,0);

		le->SetSelection(0,-1);
		le->padx = .2;
		app->addwindow(le);
		app->setfocus(le, 0, nullptr);

		return MODE_Editing;

	} else {
		mode = MODE_Normal;
		anXWindow *w = findChildWindowByName("inputedit");
		if (w) app->destroywindow(w);
	}

	needtodraw = 1;
	return mode;
}

void NumSlider::SetFloatRange(double nmin, double nmax, double nstep)
{
	win_style |= DOUBLES;
	min = nmin;
	max = nmax;
	step = nstep;
	needtodraw = 1;
}

//! Catches when the lineedit receives enter..
int NumSlider::Event(const EventData *e,const char *mes)
{
	if (strcmp(mes,"lineedit")) return ItemSlider::Event(e,mes);

	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
	if (!s) return ItemSlider::Event(e,mes);

	const char *blah=s->str;
	char *tmp;

	double ncurnum=strtod(blah,&tmp);
	if (!(win_style&DOUBLES)) ncurnum=int(ncurnum+.5);

	if (!(win_style&NO_MINIMUM) && ncurnum<min) ncurnum=min;
	else if (!(win_style&NO_MAXIMUM) && ncurnum>max) ncurnum=max;

	if (tmp!=blah) { 
		curnum=ncurnum;
		curitem=ncurnum; 
		needtodraw=1; 
		send();
	}
	//app->destroywindow(le);

	return 0;
}

int NumSlider::send()
{
	cerr << "sending NumSlider"<<endl;

	if (!win_owner || !win_sendthis) return 0;

	SimpleMessage *ievent = new SimpleMessage(NULL, getid(curitem),0,0,0);
	if (win_style & DOUBLES) ievent->str = newprintfstr("%f", curnum);
	else ievent->str = newprintfstr("%d", (int)(curnum+.5));

	app->SendMessage(ievent, win_owner, win_sendthis, object_id);
	needtodraw = 1;
	return 1;
}


int NumSlider::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	if (ch == LAX_Esc) {
		anXWindow *inputedit = findChildWindowByName("inputedit");
		if (inputedit) {
			Mode(MODE_Normal);
			return 0;
		}
	}

	return ItemSlider::CharInput(ch, buffer, len, state, kb);
}

} // namespace Laxkit

