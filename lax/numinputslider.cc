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


#include <lax/numinputslider.h>


namespace Laxkit {

/*! \class NumInputSlider
 * \brief Like NumSlider, but on shift-LBDown lets you type in a value.
 *
 * This is simliar to the control in the program Blender. The current value
 * is made the starting value of the LineEdit. If there is a parsing error,
 * then the original value is not changed. If a number is entered, but it is
 * out of bounds, it is clamped to the range [min,max]. 
 *
 * \todo Double click should also jump to edit. A single normal click will
 *   inc or dec the value, and the double click should revert to previous
 *   value before entering the edit.
 */

	
NumInputSlider::NumInputSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			   int xx,int yy,int ww,int hh,int brder,
			   anXWindow *prev,unsigned long nowner,const char *nsendthis,
			   const char *nlabel,int nmin,int nmax,int current)
	: NumSlider(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendthis, nlabel,nmin,nmax,current)
{
	le=NULL;
}


/*! \todo set focus on line edit with paired keyboard of d if d is mouse
 */
int NumInputSlider::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!(state&ShiftMask)) return NumSlider::LBDown(x,y,state,count,d);
	//  LineEdit(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
	//  //          int xx,int yy,int ww,int hh,unsigned int bordr,anXWindow *prev,
	//  //           const char *newtext=NULL,unsigned int ntstyle=0);
	  
	if (le) return 0;
	char num[20];
	sprintf(num,"%d",curitem);
	le=new LineEdit(this,"nums-le",NULL,win_style&(ANXWIN_HOVER_FOCUS),
		 0,0, win_w-2,win_h-2,1,
		 NULL,object_id,"lineedit",
		 num,0);
	le->SetCurpos(-1);
	le->padx=5;
	app->addwindow(le);
	if (!le->xlib_window) return 0;
	app->setfocus(le,0,NULL);//***
	return 0;
}

//! Catches when the lineedit receives enter..
int NumInputSlider::Event(const EventData *e,const char *mes)
{
	if (!le || strcmp(mes,"lineedit")) return anXWindow::Event(e,mes);

	char *blah=le->GetText(),*tmp;
	int ncuritem=strtol(blah,&tmp,10);
	if (ncuritem<min) ncuritem=min;
	else if (ncuritem>max) ncuritem=max;
	if (tmp!=blah) { 
		curitem=ncuritem; 
		needtodraw=1; 
		send();
	}
	app->destroywindow(le);
	le=NULL;
	delete[] blah;
	return 0;
}

} // namespace Laxkit


