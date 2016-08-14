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


#include <lax/tabframe.h>
#include <lax/displayer.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 


// // buttons uses bevel(), and does not rearrange rows
// // tabs is default, rearranges rows
//#define TABFRAME_BUTTONS (1<<22)
//#define TABFRAME_TABS    (1<<23)


namespace Laxkit {
	
//------------------------------ TabBox --------------------------------
/*! \class TabBox
 * \brief Holds the icon and a pointer to the window for elements of a TabFrame.
 *
 * This is just like StrIconBox, but with additional member anXWindow win.
 */


//	StrIconBox(const char *nlabel,Pixmap img,Pixmap nclip,Pixmap bw,int nid);
TabBox::TabBox(anXWindow *nwin,const char *nlabel,LaxImage *img,int nid)
	: IconBox(nlabel,img,nid)
{
	win=nwin;
	winislocal=0;
}

//! Empty destructor.
TabBox::~TabBox()
{}

////! Resize the window to have the current settings for x,y,w,h.
///*! The SquishyBox x,y,w,h should have been set already, this
// * function just resizes the window to those values.
// */
//void TabBox::sync()
//{
//	//DBG if (win) cerr <<"-=-=-=-==< WFresize<sync "<<win->WindowTitle()<<": ";
//	//DBG else cerr <<"-=-=-=-==< WFresize NULL"<<": ";
//	//DBG cerr <<m[0]<<','<<m[6]<<"  "<<m[1]<<"x"<<m[7]<<" pref:("<<m[2]<<','<<m[3]<<','
//	//DBG 	<<m[4]<<" x "<<m[8]<<','<<m[9]<<','<<m[10]<<")"<<endl;
//	
//	SquishyBox::sync();
//	
//	//DBG if (win) cerr <<"-=-=-=-==< WFresize>sync "<<win->WindowTitle()<<": ";
//	//DBG else cerr <<"-=-=-=-==< WFresize NULL"<<": ";
//	//DBG cerr <<m[0]<<','<<m[6]<<"  "<<m[1]<<"x"<<m[7]<<" pref:("<<m[2]<<','<<m[3]<<','
//	//DBG 	<<m[4]<<" x "<<m[8]<<','<<m[9]<<','<<m[10]<<")"<<endl;
//
//	if (!win) return;
//
//	 // some basic sanity checking, make windows smaller than 2000, clamps w,h, not pref w,h
//	if (m[1]>2000) m[1]=2000;
//	else if (m[1]<0) m[1]=0;
//	if (m[7]>2000) m[7]=2000;
//	else if (m[7]<0) m[7]=0;
//
//	win->MoveResize(m[0]-win->win_border,m[6]-win->win_border,m[1],m[7]);
//	//if (!win->window) anXApp::app->addwindow(win);
//}


//------------------------------ TabFrame --------------------------------
/*! \class TabFrame
 * \brief A frame to hold tabbed windows via TabBox objects.
 *
 * You can have a style where each tab is just a button, so the whole thing looks like a 
 * normal StrIconSelector, or you can have the usual tabbed window format, where the 
 * elements are arranged like tabbed folders, and clicking on an icon, makes that row and
 * icon come to the front. In either case, the tabs are stretched to fill across the window.
 * (*** most of that is still unimplemented)
 *
 * All the tabs must be clustered at the top, bottom, left or right, and any remaining space
 * is for the selected window.
 *
 * Pass in BOXSEL_* styles fond for BoxSelector.
 */


TabFrame::TabFrame(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
                      int xx,int yy,int ww,int hh,int brder,
					  anXWindow *prev,unsigned long nowner,const char *nsendmes,
                      int npad,int npadg)
	: IconSelector(parnt,nname,ntitle,
				   nstyle | BOXSEL_ONE_ONLY,
				   xx,yy,ww,hh,brder,prev,nowner,nsendmes,npad,npadg)
{
	//***
	curtab=-1;
}


TabFrame::~TabFrame()
{
}

//! Calls BoxSelector::init(), then SyncWindow(1).
int TabFrame::init()
{
	BoxSelector::init();

	for (int c=0; c<wholelist.n; c++) {
		mapWindow(c,0); // turn off initially
	}

	SelectN(0);
	return 0;
}

void TabFrame::FillBox(IconBox *b,const char *nlabel,LaxImage *img, int nid)
{
	IconSelector::FillBox(b,nlabel,img,nid);
	double pad = app->defaultlaxfont->textheight()/3;
	b->h (b->h()  + pad);
	b->w (b->w()  + pad);
	b->ph(b->ph() + pad);
	b->pw(b->pw() + pad);
}

void TabFrame::drawbox(int which)
{
	needtodraw=1;
}

void TabFrame::DrawTab(IconBox *b, int selected, int iscurbox)
{
	if (!b) return;

	Displayer *dp=GetDisplayer();

	 //draw the outline

	//dp->drawrectangle(b->x() - b->pad,  b->y() - b->pad,    b->w() + 2*b->pad,  b->h() + 2*b->pad, 0);

	double xx=b->x(), yy=b->y(), ww=b->w(), hh=b->h();

	dp->moveto(xx-hh/3, yy+hh);
	dp->curveto(flatpoint(xx,yy+hh), flatpoint(xx,yy), flatpoint(xx+hh/3,yy));
	dp->lineto(xx+ww-hh/3, yy);
	dp->curveto(flatpoint(xx+ww,yy), flatpoint(xx+ww,yy+hh), flatpoint(xx+ww+hh/3,yy+hh));
	dp->closed();

	dp->NewFG(selected ? win_colors->moverbg : win_colors->bg);
	dp->fill(0);

	if (iscurbox) { //draw extra line to window border
		dp->moveto(0, yy+hh-1); 
		dp->lineto(xx-hh/3, yy+hh-1); 
	} else {
		dp->moveto(xx-hh/3, yy+hh-1);
	}
	dp->curveto(flatpoint(xx,yy+hh-1), flatpoint(xx,yy), flatpoint(xx+hh/3,yy));
	dp->lineto(xx+ww-hh/3, yy);
	dp->curveto(flatpoint(xx+ww,yy), flatpoint(xx+ww,yy+hh-1), flatpoint(xx+ww+hh/3,yy+hh-1));
	if (iscurbox) { //draw extra line to window border
		dp->lineto(win_w, yy+hh-1); 
	}

	dp->NewFG(coloravg(win_colors->fg, win_colors->bg));
	dp->stroke(0);

	//dp->NewFG(win_colors->bg);
	//dp->drawline(xx-hh/3,yy+hh, xx+ww+hh/3,yy+hh);


	 // Set  tx,ty  px,py
	int w,h,tx,ty,ix,iy,dx,dy;
	LaxImage *i=b->image;
	const char *l=b->label;
	get_placement(i,l,padg,labelstyle,&w,&h,&tx,&ty,&ix,&iy);
	dx=b->x()+(b->w()-w)/2;
	dy=b->y()+(b->h()-h)/2;

	 // draw the info
	if (i && ix!=LAX_WAY_OFF) {
		ix+=dx;
		iy+=dy;
		dp->imageout(i,ix,iy);
		i->doneForNow();
	}

	if (l && tx>LAX_WAY_OFF) {
		tx+=dx;
		ty+=dy;
		dp->NewFG(win_colors->fg);
		dp->textout(tx,ty, l,-1, LAX_LEFT|LAX_TOP);
	}
}

void TabFrame::Refresh()
{
	if (!needtodraw) return;
	if (arrangedstate==0) {
		sync();
		mapWindow(curtab,1);
	}

	Displayer *dp=MakeCurrent();
	dp->ClearWindow();
	dp->LineWidth(1);
	needtodraw=0;

	IconBox *b, *cbox = NULL, *hbox=NULL;
	if (hoverbox>=0) hbox=dynamic_cast<IconBox *>(wholelist.e[hoverbox]);
	if (curtab>=0)   cbox=dynamic_cast<IconBox *>(wholelist.e[curtab]);

	for (int c=0; c<list.n; c++) {
		ListBox *row = dynamic_cast<ListBox*>(list.e[c]);

		dp->NewFG(coloravg(win_colors->fg,win_colors->bg,.8));
		dp->drawrectangle(0,row->y()-padinset, win_w,row->h()+padinset-1, 1);

		int i = row->list.findindex(cbox);
		if (i<0) i=-1;

		for (int c2=row->list.n-1; c2>i; c2--) {
			b=dynamic_cast<IconBox *>(row->list.e[c2]); 
			DrawTab(b, b==hbox, b==cbox);
		}

		for (int c2=0; c2<i; c2++) {
			b=dynamic_cast<IconBox *>(row->list.e[c2]); 
			DrawTab(b, b==hbox, b==cbox);
		}

		if (i>=0) DrawTab(cbox, hbox==cbox, true); 
	} 

}

//! Add a new tab with the given window, label, and icon.
/*! If makebw is not 0, then automatically create a black and white icon from the specified icon (this
 * is unimplemented).
 *
 * If absorbcount, then the reference count of nwin is absorbed, and the calling code need not
 * dec_count(). This lets you call AddWin(new SomeWindow, absorbcount=1) and you don't have to worry
 * about decrementing the window any more.
 */
int TabFrame::AddWin(anXWindow *nwin,int absorbcount, const char *nlabel,const char *iconfilename,int makebw)
{
	TabBox *newbox=new TabBox();
	newbox->win=nwin;
	if (nwin) {
		nwin->win_border=0;
		app->reparent(nwin,this); //this will increment the window's count
		if (absorbcount) nwin->dec_count();
	}

	IconSelector::FillBox(newbox,nlabel,iconfilename,makebw);
	wholelist.push(newbox);
	mapWindow(wholelist.n-1,0);
	needtodraw=1;
	return wholelist.n-1;
}

//! Calls BoxSelector::SelectN(), then unmaps old tab, and maps new tab.
/*! \todo rearrange rows when a tab has to be brought forward!!
 *
 * Returns the index of the current box after the change.
 */
int TabFrame::SelectN(int which)
{
	which=BoxSelector::SelectN(which);
	if (which<0 || which>=wholelist.n || which==curtab) return curbox;
	TabBox *b;
	b=dynamic_cast<TabBox *>(wholelist.e[which]);
	if (!b) return curbox;
	mapWindow(curtab,0); // turn off old tab
	curtab=which;
	mapWindow(curtab,1); // turn on new tab
	return curbox;
}

//! Sync and map the current tab window.
/*! Assumes that sync() on this has already been called. That is, this should be
 * the correct size before calling this function.
 * Turns on if mapit!=0 and off if mapit==0.
 *
 * If turning on, then the which window also gets resized to the space left over from
 * the boxes.
 */
int TabFrame::mapWindow(int which,int mapit) //mapit=1
{
	if (which<0 || which>=wholelist.n) return 1;
	TabBox *b;
	b=dynamic_cast<TabBox *>(wholelist.e[which]);
	if (!b || !b->win) return 1;
	
	DBG cerr<<"----- TabFrame map "<<b->win->WindowTitle()<<": "<<which<<' '<<mapit;

	 // find the window area
	int x,y,w,h;
	if (flags&BOX_VERTICAL) { // is rows, so height=win_h-bbox.h, width=win_w
		x=0;
		w=win_w-1;
		h=win_h-ph();
		if (win_style&BOXSEL_TOP) y=win_h-h;
		else y=0;
	} else {
		w=win_w-pw();
		if (win_style&BOXSEL_RIGHT) x=0; else x=win_w-w;
		y=0;
		h=win_h-1;
	} 

	DBG cerr<<"  "<<x<<','<<y<<' '<<w<<'x'<<h<<endl;
	if (mapit) {
		//&& w>0 && h>0) {
		if (b->win->xlib_window==0) app->addwindow(b->win,1); // add but don't map
		else {
			DBG cerr << "------tabframe calling app map on"<<endl;
			app->mapwindow((anXWindow *)b->win); // ***don't know why but this is necessary
		}
		if (b->win->win_w!=w || b->win->win_h!=h || b->win->win_x!=x || b->win->win_y!=y) 
			b->win->MoveResize(x,y,w,h); // beware strange things when resizing unmapped windows
	} else { 
		DBG cerr <<"------tabframe calling app map off"<<endl;
		app->unmapwindow((anXWindow *)b->win); // ***don't know why but this is necessary
	}
	return 0;
}

//! Select previous tab.
int TabFrame::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	SelectN(curtab>0?curtab-1:wholelist.n-1);
	return 0;
}

//! Select next tab.
int TabFrame::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	SelectN(curtab<wholelist.n-1?curtab+1:0);
	return 0;
}

int TabFrame::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Pgup && (state&LAX_STATE_MASK)==ControlMask) {
		SelectN(curtab>0?curtab-1:wholelist.n-1);
		return 0;

	} else if (ch==LAX_Pgdown && (state&LAX_STATE_MASK)==ControlMask) {
		SelectN(curtab<wholelist.n-1?curtab+1:0);
		return 0;
	}

	return anXWindow::CharInput(ch,buffer,len,state,d);
}



} // namespace Laxkit

