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
//    Copyright (C) 2012-2013 by Tom Lechner
//
#ifndef _LAX_COLORSELECTOR_H
#define _LAX_COLORSELECTOR_H


#include <lax/anxapp.h>
#include <lax/colors.h>
#include <lax/screencolor.h>
#include <lax/rectangles.h>

namespace Laxkit {

//------------------------------- ColorSelector ------------------------------
class ColorSelector : public anXWindow, public ColorBase
{
  protected:
	ButtonDownInfo buttondown;
	IntRectangle bwcolor, hue;
	ScreenColor current,old;

	virtual int send();
	
  public:
	ColorSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int nmax, int nstep, int ctype,
			 int c0,int c1,int c2,int c3=0,int c4=0);
	virtual ~ColorSelector();
	virtual const char *whattype() { return "ColorSelector"; }
	virtual int init();
	virtual void Refresh();
	virtual void DrawMixer();
	virtual int LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MouseMove(int mx,int my, unsigned int state, const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d);
};

} //namespace Laxkit


#endif


