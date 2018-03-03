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
//    Copyright (C) 2012-2013 by Tom Lechner
//

//
//4 point color select square, choose your own 4 colors..
//n primaries? 
//


#include <lax/colorselector.h>


namespace Laxkit {

ColorSelector::ColorSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 anXWindow *prev,unsigned long owner,const char *mes,
			 int nmax, int nstep, int ctype,
			 int c0,int c1,int c2,int c3=0,int c4=0)
{
}

ColorSelector::~ColorSelector()
{
}

int ColorSelector::init()
{
	return 0;
}

void ColorSelector::Refresh()
{
	if (!win_on || !needtodraw) return;
	needtodraw=0;

	clear_window(this);

	DrawMixer();
}

void ColorSelector::DrawMixer()
{
	double r,g,b;
	double p,x,y;

	 //draw hue box r..g..b
	for (int c=0; c<hue.height; c++) {
		p=(double)c/hue.height;
		if (p<1./3) {
			 //r1..0, g0..1
			p*=3;
			r=1-p;
			g=p;
			b=0;
		} else if (p<2./3) {
			 //g1..0, b0..1
			p=3*p-1;
			r=0;
			g=1-p;
			b=p;
		} else {
			 //b1..0, r0..1
			p=3*p-2;
			r=p;
			g=0;
			b=1-p;
		}

		foreground_color(rgbcolorf(r,g,b));
		draw_line(hue.x,hue.y+c, hue.width,1);
	}

	 //draw rectangular color mixer box
	unsigned long left,right;
	unsigned long currenthue=****;
	unsigned long ul=rgbcolorf(1.,1.,1.);
	unsigned long ll=rgbcolorf(0.,0.,0.);
	unsigned long ur=currenthue;
	unsigned long lr=currenthue;

	for (int c=0; c<bwcolor.height; c++) {
		y=(double)c/bwcolor.height;
		left =coloravg(ul,ll,y);
		right=coloravg(ur,lr,y);
		for (int c2=0; c2<bwcolor.width; c2++) {
			x=(double)c/bwcolor.width;
			foreground_color(coloravg(left,right, x));
			fill_rectangle(this,bwcolor.x+c,bwcolor.y+c2, 1,1);
		}
	}

	 //highlight current spot in bwcolor
	***

	 //highlight current spot in hue
	***
}

int ColorSelector::LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	return 0;
}

int ColorSelector::LBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	return 0;
}

int ColorSelector::MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	return 0;
}

int ColorSelector::MBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	return 0;
}

int ColorSelector::RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	return 0;
}

int ColorSelector::RBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	return 0;
}

int ColorSelector::MouseMove(int mx,int my, unsigned int state, const LaxMouse *d)
{
	return 0;
}

int ColorSelector::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d)
{
	return 0;
}


} //namespace Laxkit


