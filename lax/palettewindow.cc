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

#include <lax/palettewindow.h>
#include <lax/laxutils.h>
#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/colors.h>
#include <lax/language.h>
#include <lax/lists.cc>

#include <sys/stat.h>
#include <cmath>

#include <iostream>
using namespace std;
#define DBG 

using namespace LaxFiles;

namespace Laxkit {


//-------------------------------- PaletteWindow -----------------------------


/*! \class PaletteWindow 
 *  \brief A window to handle Palette instances.
 *
 *  \code
 *    //if left-double clicking calls up a FileDialog to load another Palette.
 *   #define PALW_DBCLK_TO_LOAD   (1<<16)
 *  \endcode
 */
/*! \var double PaletteWindow::dx
 * \brief Column width.
 */
/*! \var double PaletteWindow::dy
 * \brief Row height.
 */


PaletteWindow::PaletteWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend)
	: anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	palette=NULL;
	
	//***
	//LoadPalette("/usr/share/gimp/2.0/palettes/Plasma.gpl");
	palette=rainbowPalette(27,18,255, 1);

	curcolor=ccolor=-1;
	pad=app->default_padx;
	
	installColors(app->color_panel);

	findInrect();
}

PaletteWindow::~PaletteWindow()
{
	if (palette) palette->dec_count();
}
 
//! Send a SimpleColorEventData message to win_owner. 
int PaletteWindow::send()
{
	if (!win_owner || !win_sendthis || !palette || curcolor<0) return 0;

	SimpleColorEventData *e=new SimpleColorEventData;

	e->max=palette->defaultmaxcolor;
	e->numchannels=palette->colors.e[curcolor]->numcolors;
	e->channels=new int[e->numchannels];
	e->colorsystem=palette->colors.e[curcolor]->color_space;

	int c;
	for (c=0; c<palette->colors.e[curcolor]->numcolors; c++) 
		e->channels[c]=palette->colors.e[curcolor]->channels[c];
	
	app->SendMessage(e,win_owner,win_sendthis,object_id);
	return 1;
}

/*! 
 * Currently, this dec_counts the old, and installs a new one.
 * 
 * Return 0 for success, or nonzero error.
 */
int PaletteWindow::LoadPalette(const char *file)
{
	FILE *f=fopen(file,"r");
	if (!f) return 1;

	Palette *p=new Palette;
	p->dump_in(f,0,LAX_GIMP_PALETTE,NULL,NULL);
	fclose(f);

	if (p->colors.n) {
		if (palette) palette->dec_count();
		palette=p;
		findInrect();
		curcolor=ccolor=-1;
		needtodraw=1;
		return 0;
	} 

	delete p;
	return 1;
}
	
void PaletteWindow::Refresh()
{
	if (!win_on || needtodraw==0) return;
	needtodraw=0;

	Displayer *dp=MakeCurrent();
	dp->ClearWindow();


	 // draw head stuff
	const char *blah;
	dp->NewFG(win_colors->bg);
	dp->BlendMode(LAXOP_Over);
	dp->drawrectangle(0,0,win_w,app->defaultlaxfont->textheight(), 1);
			
	if (palette->name) blah=palette->name; else blah="(untitled)";
	dp->NewFG(win_colors->fg);
	int cc=dp->textout(pad,pad, blah,strlen(blah), LAX_LEFT|LAX_TOP);

	int r,g,b;
	if (curcolor>=0 || ccolor>=0) {
		 //write out current color name
		int color;
		if (ccolor>=0) color=ccolor; else color=curcolor;
		char *blah2=NULL;
		r=palette->colors.e[color]->channels[0]*255/palette->defaultmaxcolor;
		g=palette->colors.e[color]->channels[1]*255/palette->defaultmaxcolor;
		b=palette->colors.e[color]->channels[2]*255/palette->defaultmaxcolor;
		if (!palette->colors.e[color]->name || !strcmp(palette->colors.e[color]->name,"Untitled")) {
			blah2=new char[30];
			sprintf(blah2,"%02X%02X%02X",r,g,b);
		}
		int ccc=dp->textout(win_w-pad,pad, (blah2?blah2:palette->colors.e[color]->name),-1,LAX_TOP|LAX_RIGHT);
		if (blah2) delete[] blah2;

		 //draw current mouse over color
		ccc=win_w-cc-ccc-4*pad;
		if (ccc>0) {
			dp->NewFG(rgbcolor(r,g,b));
			dp->drawrectangle(cc+2*pad,0, ccc,app->defaultlaxfont->textheight()+2*pad,1);
		}
	}

	 // draw all the palette colors
	int i;
	double x=0,y;
	
	y=inrect.y-dy;
	for (i=0; i<palette->colors.n; i++) {
		if (i%xn==0) {
			x=inrect.x;
			y+=dy;
		}
		r=palette->colors.e[i]->channels[0]*255/palette->defaultmaxcolor;
		g=palette->colors.e[i]->channels[1]*255/palette->defaultmaxcolor;
		b=palette->colors.e[i]->channels[2]*255/palette->defaultmaxcolor;
		dp->NewFG(rgbcolor(r,g,b));
		dp->drawrectangle(x,y,dx+1,dy+1, 1);
		x+=dx;
//		if ((i+1)%xn==0) {
//			 //blank out to the right
//			dp->NewFG(win_colors->bg);
//			fill_rectangle(this, x,y,win_w-x,dy);
//		}
	}
//	 //blank out unused space
//	dp->NewFG(win_colors->bg);
//	fill_rectangle(this, x,y,win_w-x,dy);
//	fill_rectangle(this, 0,y+dy,win_w,win_h-y);
	
	 // draw box around curcolor
	if (curcolor>=0) {
		x=inrect.x + (curcolor%xn)*dx;
		y=inrect.y + (curcolor/xn)*dy;
		dp->NewFG((unsigned long) 0);
		dp->drawrectangle(x,y,dx,dy, 0);
		dp->NewFG(~0);
		dp->drawrectangle(x-1,y-1,dx+2,dy+2, 0);
	}
	
	SwapBuffers();
}

int PaletteWindow::findColorIndex(int x,int y)
{
	if (x<inrect.x || x>=inrect.x+inrect.width || y<inrect.y || y>=inrect.y+inrect.height) return -1;
	
	if (inrect.height<1) return -1; 

	int r,c;
	c=(x-inrect.x)/dx;
	r=(y-inrect.y)/dy;
	if (c>=xn) return -1;
	//DBG cerr<<"c,r: "<<c<<','<<r<<endl;
	c=c+r*xn;
	if (c<0 || c>=palette->colors.n) return -1;
	return c;
}

//! Make needtodraw=1 if LeaveNotify.
int PaletteWindow::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onMouseOut) {
		//DBG cerr <<" in PaletteWindow::event()..."<<endl;
		ccolor=-1;
		needtodraw=1;
		return 0;
		//DBG cerr <<"  Button::event:Leave:"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;

	} else if (!strcmp(mes,"loadpalette")) {
		 //sent from a FileDialog, selecting a new palette to load in.
		const StrEventData *s=dynamic_cast<const StrEventData *>(e);
		LoadPalette(s->str);
		return 0;
	}

	return anXWindow::Event(e,mes);
}

//! Return pointer to the directory to search for palettes from a FileDialog.
/*! Currently defaults to /usr/share/gimp/2.0/palettes if it exists, or NULL.
 */
const char *PaletteWindow::PaletteDir()
{
	if (file_exists("/usr/share/gimp/2.0/palettes",1,NULL)==S_IFDIR)
		return "/usr/share/gimp/2.0/palettes";
	return NULL;
}

/*! If count==2, then bring up a FileDialog.
 */
int PaletteWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (count==2 && (win_style&PALW_DBCLK_TO_LOAD)) {
		app->rundialog(new FileDialog(NULL,"Load Palette",_("Load Palette"),0,
									  0,0,400,500,0,
									  object_id,"loadpalette",
									  FILES_OPEN_ONE,
									  NULL,PaletteDir()));
	}
	buttondown.down(d->id,LEFTBUTTON, x,y);
	return 0;
}

int PaletteWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 0;
	buttondown.up(d->id,LEFTBUTTON);

	int cc=findColorIndex(x,y);

	if (cc<0 && y<inrect.y && y>0) {
		 //clicked down in header
		//  [Palette name]  [current color]  [color name]

	} else if (curcolor!=cc && cc>=0) {
		needtodraw|=2;
		curcolor=cc;
		send();
	}
	return 0;
}

//! Pop up a context menu to select a recent menu or other things.
int PaletteWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	//if (recentpalettes) ***;
	//if (palette_is_editable) { 
	//	Delete color
	//	New color
	//	Edit color
	//}
	//LoadPalette
	//SavePalette
	return 0;
}

//! Update showing name of curcolor.
/*! \todo this could be used to show a temp color? buttondown-nodrag-up actually changes curcolor?
 */
int PaletteWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	int cc=findColorIndex(x,y);
	if (cc<0) {
		if (ccolor!=curcolor) {
			needtodraw|=2;
			ccolor=curcolor;
		}
		return 0;
	}

	if (buttondown.isdown(0,LEFTBUTTON) && curcolor!=cc) {
		needtodraw|=2;
		curcolor=ccolor=cc;
		send();

	} else if (!buttondown.isdown(0,LEFTBUTTON) && ccolor!=cc) {
		needtodraw|=2;
		ccolor=cc;
	}
	//DBG cerr <<"palette curcolor: "<<curcolor<<"  ccolor: "<<ccolor<<endl;
	
	return 0;
}

//! Set up inrect to correspond the region the colors should be drawn in.
/*! Also finds xn, yn, dx, and dy.
 */
void PaletteWindow::findInrect()
{
	inrect.x=0;
	inrect.y=app->defaultlaxfont->textheight() + 2*pad;
	inrect.width=  win_w;
	inrect.height= win_h - app->defaultlaxfont->textheight() - 2*pad;
	if (inrect.width<1) inrect.width=1;
	if (inrect.height<1) inrect.height=1;
	
	double aspect=double(inrect.height)/inrect.width;

	if (palette->columns>0) {
		xn=palette->columns;
		yn=palette->colors.n/xn;
		if (palette->colors.n%xn!=0) yn++;
	} else {
		xn=int(ceil(sqrt(palette->colors.n/aspect)));
		yn=int(xn*aspect);
		while (xn*yn<palette->colors.n) yn++;
	}
	
	dx=double(inrect.width)/xn;
	dy=double(inrect.height)/yn;
	if (dx<=0) dx=1;
	if (dy<=0) dy=1;
}

//! Resize, then call findInrect().
int PaletteWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	int c=anXWindow::MoveResize(nx,ny,nw,nh);
	findInrect();
	return c;
}

//! Resize, then call findInrect().
int PaletteWindow::Resize(int nw,int nh)
{
	int c=anXWindow::Resize(nw,nh);
	findInrect();
	return c;
}

	
} //namespace Laxkit;


