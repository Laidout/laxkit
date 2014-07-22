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
//    Copyright (C) 2004-2007,2010-2012 by Tom Lechner
//
#ifndef _LAX_COLORBOX_H
#define _LAX_COLORBOX_H

#include <lax/anxapp.h>
#include <lax/colorbase.h>
#include <lax/buttondowninfo.h>
#include <lax/newwindowobject.h>

namespace Laxkit {


#define COLORBOX_DRAW_NUMBER (1<<16)
#define COLORBOX_CMYK        (1<<17)
#define COLORBOX_RGB         (1<<18)
#define COLORBOX_GRAY        (1<<19)

#define COLORBOX_FG          (1<<20)
#define COLORBOX_FGBG        (1<<21)
#define COLORBOX_STROKEFILL  (1<<22)


//------------------------------- ColorBox ------------------------------
class ColorBox : public anXWindow, virtual public ColorBase
{
  protected:
	int colormap[9];
	ButtonDownInfo buttondown;
	NewWindowObject *colorselector;

	virtual int send();
	
  public:
	int currentid;
	double *topcolor;
	double step;
	int sendtype;

	ColorBox(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int ctype, double nstep,
			 double c0,double c1,double c2,double c3=-1,double c4=-1,
			 NewWindowObject *newcolorselector=NULL);
	virtual ~ColorBox();
	virtual const char *whattype() { return "ColorBox"; }
	virtual const char *tooltip(const char *newtip) { return anXWindow::tooltip(newtip); }
	virtual const char *tooltip(int mouseid=0);
	virtual int init();
	virtual void Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MouseMove(int mx,int my, unsigned int state, const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d);
	virtual int Event(const EventData *e,const char *mes);

	virtual int PopupColorSelector();
	virtual void Updated();
};

} // namespace Laxkit

#endif 

