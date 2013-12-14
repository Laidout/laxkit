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


#include <lax/language.h>
#include <lax/colorbox.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/misc.h>
#include <lax/colors.h>
#include <lax/colorsliders.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {



//------------------------------- ColorBox ------------------------------
/*! \class ColorBox
 * \ingroup colors
 * \brief A control to select an RGBA color, with each mouse button corresponding to r, g, and b.
 *
 * By default, pressing and dragging the left mouse button changes the red value, 
 * the middle button does the green, and the right button the blue. The message is sent
 * when a button is released.
 *
 * While pressing a mouse button down and hitting r,g,b,c,m,y, or k, will cause that button
 * to shift the corresponding color. Internally, the colors are all rgb, and the cmyk transformations
 * are through the simple non-icc rgb<->cmyk conversion.
 *
 * This window sends SimpleColorEventData.
 *
 * The box can be one of LAX_COLOR_CMYK, LAX_COLOR_RGB, LAX_COLOR_GRAY.
 *
 *
 * \todo would be neat to maintain a stack of previous colors?
 * \todo might be nice to display number in window while dragging..
 * \todo someday do a color managed version of this window
 * \todo incorporate wheel movements to scroll the last accessed color
 */



#define RED     0
#define GREEN   1
#define BLUE    2
#define CYAN    3
#define MAGENTA 4
#define YELLOW  5
#define BLACK   6
#define ALPHA   7
#define MENU    8

#define SHIFT   (1<<9)
#define CONTROL (1<<10)
#define META    (1<<11)
#define ALT     (1<<12)

/*! Will delete newcolorselector in destructor.
 */
ColorBox::ColorBox(anXWindow *parnt,const char *nname,const char *ntitle, unsigned long nstyle,
				   int nx,int ny,int nw,int nh,int brder,
				   anXWindow *prev,unsigned long nowner,const char *mes,
			 	   int ctype, int nmax, int nstep,
				   int c0,int c1,int c2,int c3,int c4,
			 	   NewWindowObject *newcolorselector)
  : ColorBase(ctype,nmax,c0,c1,c2,c3,c4),
	anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,nx,ny,nw,nh,brder,prev,nowner,mes)
{
	colorselector=newcolorselector;

	topcolor=colors;
	step=nstep;
	currentid=0;

	 //set up which buttons control which color
	if (colortype==LAX_COLOR_CMYK) {
		colormap[RED    ]=1|CONTROL;
		colormap[GREEN  ]=2|CONTROL;
		colormap[BLUE   ]=3|CONTROL;
		colormap[CYAN   ]=1;
		colormap[MAGENTA]=2;
		colormap[YELLOW ]=3;
		colormap[BLACK  ]=1|CONTROL|SHIFT;
		colormap[ALPHA  ]=1|SHIFT;
		colormap[MENU   ]=3|CONTROL|SHIFT;
	} else if (colortype==LAX_COLOR_GRAY) {
		colormap[RED    ]=-1;
		colormap[GREEN  ]=-1;
		colormap[BLUE   ]=-1;
		colormap[CYAN   ]=-1;
		colormap[MAGENTA]=-1;
		colormap[YELLOW ]=-1;
		colormap[BLACK  ]=1;
		colormap[ALPHA  ]=3;
		colormap[MENU   ]=3|CONTROL;
	} else { //default to rgb
		colortype=LAX_COLOR_RGB;
		colormap[RED    ]=1;
		colormap[GREEN  ]=2;
		colormap[BLUE   ]=3;
		colormap[CYAN   ]=-1;
		colormap[MAGENTA]=-1;
		colormap[YELLOW ]=-1;
		colormap[BLACK  ]=-1;
		colormap[ALPHA  ]=1|SHIFT;
		colormap[MENU   ]=3|CONTROL;
	}

	
	needtodraw=1;

	 //create our own copy of default colors..
	win_colors=new WindowColors;
	*win_colors=*app->color_panel;
	win_colors->bg=rgbcolor(Red()*255/max, Green()*255/max, Blue()*255/max);
}

ColorBox::~ColorBox()
{
	if (colorselector) delete colorselector;
}

/*! \todo be ablte use control, meta, and alt?
 */
void mapname(char *buf,int m)
{
	buf[0]=0;
	if (m&SHIFT) sprintf(buf,_("Shift-"));
	if (m&CONTROL) sprintf(buf+strlen(buf),_("Control-"));
	if (m&META) sprintf(buf+strlen(buf),_("Meta-"));
	if (m&ALT) sprintf(buf+strlen(buf),_("Alt-"));
	if ((m&3)==1) sprintf(buf+strlen(buf),_("Left"));
	else if ((m&3)==2) sprintf(buf+strlen(buf),_("Middle"));
	else if ((m&3)==3) sprintf(buf+strlen(buf),_("Right"));
}

//! Return a tip saying what the current mouse maps are.
const char *ColorBox::tooltip(int mouseid)
{
	if (win_tooltip) { delete[] win_tooltip; win_tooltip=NULL; }

	char buf[100];
	if (colormap[RED]>0) {
		appendstr(win_tooltip,_("For red, drag "));
		mapname(buf,colormap[RED]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[GREEN]>0) {
		appendstr(win_tooltip,_("For green, drag "));
		mapname(buf,colormap[GREEN]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[BLUE]>0) {
		appendstr(win_tooltip,_("For blue, drag "));
		mapname(buf,colormap[BLUE]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[CYAN]>0) {
		appendstr(win_tooltip,_("For cyan, drag "));
		mapname(buf,colormap[CYAN]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[MAGENTA]>0) {
		appendstr(win_tooltip,_("For magenta, drag "));
		mapname(buf,colormap[MAGENTA]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[YELLOW]>0) {
		appendstr(win_tooltip,_("For yellow, drag "));
		mapname(buf,colormap[YELLOW]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[BLACK]>0) {
		appendstr(win_tooltip,_("For black, drag "));
		mapname(buf,colormap[BLACK]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}
	if (colormap[ALPHA]>0) {
		appendstr(win_tooltip,_("For alpha, drag "));
		mapname(buf,colormap[ALPHA]);
		buf[strlen(buf)+1]=0;
		buf[strlen(buf)]='\n';
		appendstr(win_tooltip,buf);
	}

	return win_tooltip;
}

int ColorBox::init()
{
	return 0;
}

void ColorBox::Updated()
{
	win_colors->bg=rgbcolor(Red()*255/max, Green()*255/max, Blue()*255/max);
	needtodraw=1;
}




/*! Puts coloreventdata->RGBA_max(red,green,blue,alpha, max).
 */
int ColorBox::send()
{
	if (!win_owner || !win_sendthis) return 0;

	SimpleColorEventData *cevent=NULL;
	if (colortype==LAX_COLOR_RGB) cevent=new SimpleColorEventData(max,Red(),Green(),Blue(),Alpha(),currentid);
	else if (colortype==LAX_COLOR_GRAY) cevent=new SimpleColorEventData(max,Gray(),Alpha(),currentid);
	else cevent=new SimpleColorEventData(max,Cyan(),Magenta(),Yellow(),Black(),Alpha(),currentid);

	app->SendMessage(cevent, win_owner,win_sendthis, object_id);

	return 1;
}


//! Change blue by default.
int ColorBox::RBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	if (!buttondown.any()) {
		memcpy(oldcolor,colors,5*sizeof(int));
	}
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	return 0;
}

int ColorBox::RBUp(int x,int y, unsigned int state, const LaxMouse *d)
{
	buttondown.up(d->id, RIGHTBUTTON);
	if (!buttondown.any(d->id) && ColorChanged()) send();
	return 0;
}

//! Change green by default.
int ColorBox::MBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	buttondown.down(d->id, MIDDLEBUTTON, x,y);
	if (!buttondown.any(d->id)) {
		memcpy(oldcolor,colors,5*sizeof(int));
	}
	return 0;
}

int ColorBox::MBUp(int x,int y, unsigned int state, const LaxMouse *d)
{
	buttondown.up(d->id, MIDDLEBUTTON);
	if (!buttondown.any(d->id) && ColorChanged()) send();
	return 0;
}

//! Change red by default.
/*! \todo Double click to raise a Color Selection Dialog.
 */
int ColorBox::LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	if (!buttondown.any()) {
		memcpy(oldcolor,colors,5*sizeof(int));
	}
	buttondown.down(d->id, LEFTBUTTON, x,y);

	if (count>1) {
		PopupColorSelector();
		buttondown.up(d->id,LEFTBUTTON);
		return 0;
	}
	return 0;
}

/*! Return 1 for unable. 0 for success.
 */
int ColorBox::PopupColorSelector()
{
	anXWindow *w=NULL;
	if (!colorselector) {
		w=new ColorSliders(NULL,"New Color","New Color",ANXWIN_ESCAPABLE|ANXWIN_REMEMBER, 0,0,200,400,0,
						   NULL,object_id,"newcolor",
						   colortype,max,step,
						   colors[0],colors[1],colors[2],colors[3],colors[4]);
	} else {
		w=colorselector->function(NULL,"New Color",colorselector->style,this);
	}

	if (!w) return 1;

	app->addwindow(w);
	return 0;
}


int ColorBox::Event(const EventData *e,const char *mes)
{
	if (!strcmp(mes,"newcolor")) {
		 // apply message as new current color, pass on to viewport
		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(e);
		if (!ce) return 0;

		for (int c=0; c<ce->numchannels && c<5; c++) {
			colors[c]=ce->channels[c];
		}
		win_colors->bg=rgbcolor(Red()*255/max, Green()*255/max, Blue()*255/max);
		send();
		needtodraw=1;
		return 0;
	}
	return anXWindow::Event(e,mes);
}

int ColorBox::LBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	buttondown.up(d->id, LEFTBUTTON);
	if (!buttondown.any(d->id) && ColorChanged()) send();
	return 0;
}

/*! Drag left, middle, right click for red, green, blue. 
 */
int ColorBox::MouseMove(int x,int y,unsigned int state, const LaxMouse *d)
{
	if (!buttondown.any(d->id)) return 1;

	int mods=0;
	if (state&ShiftMask)   mods|=SHIFT;
	if (state&ControlMask) mods|=CONTROL;
	if (state&MetaMask)    mods|=META;
	if (state&AltMask)     mods|=ALT;

	int b[3];
	b[0]=b[1]=b[2]=-1;
	if (buttondown.isdown(d->id,LEFTBUTTON))   b[0]=1|mods;
	if (buttondown.isdown(d->id,MIDDLEBUTTON)) b[1]=2|mods;
	if (buttondown.isdown(d->id,RIGHTBUTTON))  b[2]=3|mods;

	buttondown.move(d->id,x,y);
	int mx,my;

	if (buttondown.getinfo(d->id,LEFTBUTTON, NULL,NULL,&mx,&my,NULL,NULL) 
		 && buttondown.getinfo(d->id,RIGHTBUTTON, NULL,NULL,&mx,&my,NULL,NULL)
		 && buttondown.getinfo(d->id,MIDDLEBUTTON, NULL,NULL,&mx,&my,NULL,NULL)) {
		return 0; //couldn't find down info!
	}

	for (int c=0; c<3; c++) {
		if (b[c]<0) continue;
		int col;
		for (col=0; col<8; col++) if (colormap[col]==b[c]) break;
		if (col==8) continue; //mapping not found

		if (col==ALPHA) {
			int alpha=Alpha();
			alpha+=(x-mx)*((state&ControlMask)?1:step);
			if (alpha<0) alpha=0;
			if (alpha>max) alpha=max;
			Alpha(alpha);
			needtodraw=1;

		} else if (col==RED) {
			int red=Red();
			red+=(x-mx)*((state&ControlMask)?1:step);
			if (red<0) red=0;
			if (red>max) red=max;
			Red(red);
			needtodraw=1;

		} else if (col==GREEN) {
			int green=Green();
			green+=(x-mx)*((state&ControlMask)?1:step);
			if (green<0) green=0;
			if (green>max) green=max;
			Green(green);
			needtodraw=1;

		} else if (col==BLUE) {
			int blue=Blue();
			blue+=(x-mx)*((state&ControlMask)?1:step);
			if (blue<0) blue=0;
			if (blue>max) blue=max;
			Blue(blue);
			needtodraw=1;

		} else if (col==CYAN) {
			int cyan=Cyan();
			c+=(x-mx)*((state&ControlMask)?1:step);
			if (c<0) c=0;
			if (c>max) c=max;
			Cyan(cyan);
			needtodraw=1;

		} else if (col==MAGENTA) {
			int m=Magenta();
			m+=(x-mx)*((state&ControlMask)?1:step);
			if (m<0) m=0;
			if (m>max) m=max;
			Magenta(m);
			needtodraw=1;

		} else if (col==YELLOW) {
			int y=Yellow();
			y+=(x-mx)*((state&ControlMask)?1:step);
			if (y<0) y=0;
			if (y>max) y=max;
			Yellow(y);
			needtodraw=1;

		} else if (col==BLACK) {
			int k=Black();
			k+=(x-mx)*((state&ControlMask)?1:step);
			if (k<0) k=0;
			if (k>max) k=max;
			Black(k);
			needtodraw=1;
		}
	}

	win_colors->bg=rgbcolor(Red()*255/max, Green()*255/max, Blue()*255/max);
	needtodraw=1;
	
	char blah[100];
	if (colortype==LAX_COLOR_RGB) sprintf(blah,"%d,%d,%d,%d",Red(),Green(),Blue(),Alpha());
	else if (colortype==LAX_COLOR_CMYK) sprintf(blah,"%d,%d,%d,%d,%d",Cyan(),Magenta(),Yellow(),Black(),Alpha());
	else sprintf(blah,"%d,%d",Gray(),Alpha());
	app->postmessage(blah);
	return 0;
}

void ColorBox::Refresh()
{
	if (!win_on || !needtodraw) return;

	clear_window(this);

	if (win_style&(COLORBOX_FGBG|COLORBOX_STROKEFILL)) {
		 //two color mode, draw one color over another
		int *cc=colors;
		int offx,offy;

		if (topcolor==color1) { colors=color2; offx=win_w*.2; offy=win_h*.2; }
		else { colors=color1; offx=0; offy=0; }
		foreground_color(rgbcolor(Red()*255/max, Green()*255/max, Blue()*255/max));
		fill_rectangle(this, offx,offy, win_w*.8,win_h*.8);

		if (topcolor==color1) { colors=color1; offx=0; offy=0; }
		else { colors=color2; offx=win_w*.2; offy=win_h*.2; }
		foreground_color(rgbcolor(Red()*255/max, Green()*255/max, Blue()*255/max));
		fill_rectangle(this, win_w*.2,win_h*.2, win_w*.8,win_h*.8);

		colors=cc;

	} else {
		 //single color
		foreground_color(win_colors->bg);
		fill_rectangle(this, 0,0,win_w,win_h);
	}

	if (Alpha()<max) {
		win_colors->bg=rgbcolor(Red()*255/max, Green()*255/max, Blue()*255/max);
		foreground_color(coloravg(0,win_colors->bg, double(Alpha())/(max+1)));
		draw_thing(this, win_w/2,win_h/2,win_w/2,win_h/2,1,THING_Diamond);
	}
	
	SwapBuffers();
	needtodraw=0;
	return;
}

/*! While a button is pressed, pushing r,g,b,c,m,y,k will make that button shift
 * the corresponding color.
 */
int ColorBox::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxKeyboard *d)
{
	if (ch=='\t') return anXWindow::CharInput(ch,buffer,len,state,d);

	if (!d->paired_mouse) return anXWindow::CharInput(ch,buffer,len,state,d);
	if (!buttondown.any(d->paired_mouse->id)) return anXWindow::CharInput(ch,buffer,len,state,d);

	ch=tolower(ch);
	int b;
	if (buttondown.isdown(d->paired_mouse->id,LEFTBUTTON)) b=1;
	else if (buttondown.isdown(d->paired_mouse->id,MIDDLEBUTTON)) b=2;
	else  b=3;

	if (state&ShiftMask)   b|=SHIFT;
	if (state&ControlMask) b|=CONTROL;
	if (state&MetaMask)    b|=META;
	if (state&AltMask)     b|=ALT;

	 //clear old mapping
	for (int c=0; c<8; c++) if (colormap[c]==b) colormap[c]=-1;

	 //set new mapping
	if (ch=='a') {
		colormap[ALPHA]=b;
	} else if (ch=='r') {
		colormap[RED]=b;
	} else if (ch=='g') {
		colormap[GREEN]=b;
	} else if (ch=='b') {
		colormap[BLUE]=b;
	} else if (ch=='c') {
		colormap[CYAN]=b;
	} else if (ch=='m') {
		colormap[MAGENTA]=b;
	} else if (ch=='y') {
		colormap[YELLOW]=b;
	} else if (ch=='k') {
		colormap[BLACK]=b;
	} else return 1;

	//char blah[100];
	//sprintf(blah,"%d,%d,%d a:%d",red,green,blue,alpha);
	//app->postmessage(blah);
	//needtodraw=1;

	return anXWindow::CharInput(ch,buffer,len,state,d);
}



} // namespace Laxkit

