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
//    Copyright (C) 2004-2010 by Tom Lechner
//


#include <lax/panpopup.h>
#include <lax/laxutils.h>

#include <cstring>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//------------------------------- PanPopup -------------------------------

/*! \class PanPopup
 * \brief Press this, and a little PanWindow pops up.
 * 
 * This is just a box with a plus with arrows on it, and
 * summons up a PanWindow.
 */


PanPopup::PanPopup(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder, 
						anXWindow *prev,unsigned long nowner,const char *nsend,
						PanController *pan)
				: PanUser(pan), 
				  anXWindow(pwindow,nname,ntitle,nstyle, nx,ny, nw,nh,brder,prev,nowner,nsend)
				  
{
	needtodraw=1;
	padx=pady=0;

	installColors(app->color_panel);
}

//! Empty default destructor
PanPopup::~PanPopup()
{ }

//! *** send message when a window is popped up? right now does nothing
int PanPopup::send()
{ return 0; }

//! Draw a plus with arrows on it.
void PanPopup::Refresh()
{
	if (!win_on || !needtodraw || win_w<5 || win_h<5) return;

	Displayer *dp=GetDisplayer();
	dp->MakeCurrent(this);
	dp->ClearWindow();
	dp->BlendMode(LAXOP_Over);
	dp->LineAttributes(1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);

	 // draw move arrows
	flatpoint p[25]={flatpoint(6,1),
					flatpoint(8,3),
					flatpoint(7,3),
					flatpoint(7,5),
					flatpoint(9,5),
					flatpoint(9,4),
					flatpoint(11,6),
					flatpoint(9,8),
					flatpoint(9,7),
					flatpoint(7,7),
					flatpoint(7,9),
					flatpoint(8,9),
					flatpoint(6,11),
					flatpoint(4,9),
					flatpoint(5,9),
					flatpoint(5,7),
					flatpoint(3,7),
					flatpoint(3,8),
					flatpoint(1,6),
					flatpoint(3,4),
					flatpoint(3,5),
					flatpoint(5,5),
					flatpoint(5,3),
					flatpoint(4,3),
					flatpoint(6,1) 
				};
	for (int c=0; c<25; c++) {
		p[c].x=p[c].x*win_w/12;
		p[c].y=p[c].y*win_h/12;
	}

	dp->NewFG(win_colors->color1);
	dp->drawlines(p,25, 1, 1);

	dp->NewFG(win_colors->fg);
	dp->drawlines(p,25, 1, 0);

	needtodraw=0;
}

//! For any ButtonPress event, pop up a PanWindow.
int PanPopup::LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{
	//***popup new PanWindow, position it right, make it grab mouse
	DBG cerr <<"***Popup the panwindow"<<endl;
	PanWindow *popup=new PanWindow(NULL,"pan-popedup",NULL,ANXWIN_BARE|PAN_IS_POPUP, 0,0,0,0, 1,
			NULL,object_id,"panwindow to panpopup",
			d->id, panner,NULL);
	app->rundialog(popup);
	return 0;
}



//------------------------------- PanWindow -------------------------------

/*! \class PanWindow
 * \brief Lets you drag a little rectangle around an image to select viewable area.
 *  
 * \todo ****** must be able to handle when the wholebox is less than the selbox...
 *   right now is screwed up...
 *  
 * \code
 *   // Specifies that the window should try to center itself and the selbox
 *   // around the mouse.
 *  #define PAN_IS_POPUP     (1<<16)
 *  #define PAN_NEEDS_BDOWN  (1<<17)
 * \endcode
 */

	

PanWindow::PanWindow(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsend,
						int mouseid,
						PanController *pan,LaxImage *img,int nxs,int nys,int npad
					)
				: anXWindow(pwindow,nname,ntitle,nstyle|ANXWIN_ESCAPABLE, nx,ny, nw,nh,brder, prev,nowner,nsend),
				  PanUser(pan)
{
	needtodraw=1;
	pad=npad;
	xs=nxs;
	ys=nys;
	imagerect.x=imagerect.y=0;
	imagerect.width=imagerect.height=0;
	maxdim=150; // maybe make this default to 1/5 of root window height?
	mx=my=-50000;
	buttondown=0;
	bdowndevice=0;

	image=NULL;
	SetImage(img); // this wraps win_w/h to image w/h if win_w/h==0
	if (win_w==0) win_w=imagerect.x;
	if (win_h==0) win_h=imagerect.y;
	
	if (win_style&PAN_IS_POPUP) centerOnMouse(mouseid);
}

PanWindow::~PanWindow()
{
	if (image) image->dec_count();
}

//! Move the window so that the selection is as near to centered on the mouse as possible.
void PanWindow::centerOnMouse(int mouseid)
{
	int x,y;
	int screen;
	mouseposition(mouseid, NULL, &x, &y, NULL,NULL,&screen);

	int xx,yy,ww,hh;
	long s,e;
	panner->GetMagToWhole(1,imagerect.width-1,&s,&e);
	xx=s;
	ww=e-s+1;
	panner->GetMagToWhole(2,imagerect.height-1,&s,&e);
	yy=s;
	hh=e-s+1;
	int width,height;
	 //***
	app->ScreenInfo(screen,NULL,NULL,&width,&height,NULL,NULL,NULL,NULL);
	x-=(xx+ww/2);
	y-=(yy+hh/2);
	width--;
	height--;
		
	 // these would make the mouse warp, perhaps?
	if (x+win_w+(int)win_border>width) x=width-win_w-win_border;
	if (x<(int)win_border) x=win_border;
	if (y+win_h+(int)win_border>height) y=height-win_h-win_border;
	if (y<(int)win_border) y=win_border;
	
	MoveResize(x,y,win_w,win_h);
}

//! Calls same of PanUser, then sets needtodraw=1
/*! *** this is perhaps not necessary? assumes that a new panner is set 
 * assumes needtodraw=1 already?
 */
void PanWindow::UseThisPanner(PanController *npanner)
{
	PanUser::UseThisPanner(npanner);
	needtodraw=1;
}

int PanWindow::send()
{
	//*** this might be better handled from the panner.
	if (!win_owner || !image) return 0;

	EventData *e=new EventData(win_sendthis);
	app->SendMessage(e, win_owner, win_sendthis, object_id);
	needtodraw=1;
	return 1;
}

//! Intercept MapNotify in case PAN_IS_POPUP is set.
/*! Grabs the pointer if that style is set.
 * In either case, it then properly calls 
 * anXWindow::Event().
 *
 * \todo *** this is broken!! Must reimplement for mpx!!
 */
int PanWindow::Event(const EventData *e,const char *mes)
{//***
//	if (e->type==MapNotify && win_style&PAN_IS_POPUP) {
//		app->setfocus(this);
//		if (XGrabPointer(app->dpy,window,False,ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
//				GrabModeAsync,GrabModeAsync,
//				None,None,CurrentTime)==GrabSuccess) {
//			DBG cerr <<"PanWindow GrabSuccess!"<<endl; 
//			unsigned int bmask;
//			mouseposition(this,NULL,NULL,&bmask,NULL);
//			if (bmask&Button1Mask) buttondown|=LEFTBUTTON;
//			if (bmask&Button2Mask) buttondown|=MIDDLEBUTTON;
//			if (bmask&Button3Mask) buttondown|=RIGHTBUTTON;
//		} else cout <<"PanWindow GrabFailure!"<<endl; 
//	}
	return anXWindow::Event(e,mes);
}

int PanWindow::init()
{
	CenterImage();
	return 0;
}

//! Set the image to imag or to panner settings. Sets imagerect to proper values.
/*! PanWindow assumes responsiblity for deleting the image via XDestroyImage.
 *
 * This designates the selectable area proportional to the image size, or
 * porportional to the panner wholespace if there is no image. It does not
 * overwrite any of the panner settings, unless the panner seems to have 0's
 * for everything.
 *
 * If imag==NULL, then the imagerect will correspond to panner stuff
 * scaled to fit within a square with side length maxdim, else imagerect
 * will correspond to the image. If this->window is None and win_w==0 or win_h==0,
 * then set that element to imagerect.width or height.
 */
int PanWindow::SetImage(LaxImage *imag)
{
	if (image) image->dec_count();
	image=imag;
	if (!image) { // use panner data scaled to fit square maxdim
		long w=panner->max[0]-panner->min[0],
		     h=panner->max[1]-panner->min[1];
		if (!h) { // watch out for null panner!
			imagerect.width=0;
			imagerect.height=0;
		} else { //*** this isn't quite right, should scale to window, if window dims are reasonable
			double d=(double)w/h;
			if (d>1) { 
				imagerect.width=maxdim;
				imagerect.height=(int)(maxdim/d);
			} else {
				imagerect.width=(int)(maxdim*d);
				imagerect.height=maxdim;
			}
		}
	} else {
		image->inc_count();
		imagerect.width= imag->w();
		imagerect.height=imag->h();
	}

	 //wrap window to the image if no width or height has been specified yet
	if (win_w==0) win_w=imagerect.width;
	if (win_h==0) win_h=imagerect.height;

	CenterImage();
	return 0;
}

//! Draw the image and the selection rectangle.
void PanWindow::Refresh()
{
	if (!win_on || !needtodraw) return;

	Displayer *dp=GetDisplayer();
	dp->MakeCurrent(this);
	dp->NewFG(rgbcolor(128,128,128));

//*** need smart refresh
//	if (needtodraw&1) {
		dp->ClearWindow();
		if (image) {
			dp->imageout(image,imagerect.x,imagerect.y);
		} else {
			// draw without the image
		}
		needtodraw=2;
//	}
	
	int drawarrow=0;
	 // draw the selbox
	dp->BlendMode(LAXOP_Xor);
	dp->LineAttributes(2,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);

	int xx,yy,ww,hh;
	long s,e;
	panner->GetMagToWhole(1,imagerect.width-1,&s,&e);
	xx=s;
	ww=e-s+1;
	if (e-s+1>imagerect.width) drawarrow=1;
	panner->GetMagToWhole(2,imagerect.height-1,&s,&e);
	yy=s;
	hh=e-s+1;
	if (e-s+1>imagerect.height) drawarrow|=2;

	dp->drawrectangle(xx,yy,ww,hh, 0);

	dp->LineAttributes(0,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
	if (drawarrow&1) { // selbox is larger than whole
		s=win_w/4;
		dp->drawline(win_w/2-s/2,win_h/2, win_w/2+s/2,win_h/2);
		dp->drawline(win_w/2-s/2,win_h/2, win_w/2-s/2+s/4,win_h/2-s/4);
		dp->drawline(win_w/2-s/2,win_h/2, win_w/2-s/2+s/4,win_h/2+s/4);
		dp->drawline(win_w/2+s/2,win_h/2, win_w/2+s/2-s/4,win_h/2-s/4);
		dp->drawline(win_w/2+s/2,win_h/2, win_w/2+s/2-s/4,win_h/2+s/4);
	}
	if (drawarrow&2) { // selbox is larger than whole
		s=win_h/4;
		dp->drawline(win_w/2,win_h/2-s/2, win_w/2,win_h/2+s/2);
		dp->drawline(win_w/2,win_h/2-s/2, win_w/2-s/4,win_h/2-s/2+s/4);
		dp->drawline(win_w/2,win_h/2-s/2, win_w/2+s/4,win_h/2-s/2+s/4);
		dp->drawline(win_w/2,win_h/2+s/2, win_w/2-s/4,win_h/2+s/2-s/4);
		dp->drawline(win_w/2,win_h/2+s/2, win_w/2+s/4,win_h/2+s/2-s/4);
	}
	
	
	needtodraw=0;
}

int PanWindow::LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d)
{// ***
	if (bdowndevice) return 0;
	bdowndevice=d->id;

	buttondown|=LEFTBUTTON;
	mx=x; my=y;

	//*** center sel around mouse if the mouse is not in the sel
	
	return 0;
}

int PanWindow::LBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	if (bdowndevice) return 0;
	bdowndevice=0;

	if (!(buttondown&LEFTBUTTON)) return 0;
	buttondown&=~LEFTBUTTON;
	if (win_style&PAN_IS_POPUP) app->destroywindow(this);
	return 0;
}

int PanWindow::MouseMove(int x,int y,unsigned int state, const LaxMouse *d)
{
//cout<<"\nPanWindow:";
	if (!bdowndevice || !(buttondown&LEFTBUTTON)) return 1;

	if (mx==-50000) mx=x;
	if (my==-50000) my=y;

	int dx,dy;
	dx=x-mx;
	dy=y-my;
	
	//dx=(x>=0 && x<win_w?x-mx:0);
	//dy=(y>=0 && y<win_h?y-my:0);

//	if (x<0) dx=mx>0?-mx:0;
//	else if (x>=win_w) dx=mx<=win_w?win_w-mx-1:0;
//	else dx=x-mx;
//	if (y<0) dy=my>0?-my:0;
//	else if (y>=win_h) dy=my<=win_h?win_h-my-1:0;
//	else dy=y-my;

	if (panner->Shift(1,dx,imagerect.width)) needtodraw|=2;
	if (panner->Shift(2,dy,imagerect.height)) needtodraw|=2;
	
	mx=x; my=y;
	return 0;
}

//! Adjust imagerect so that its width and height are centered in the window.
void PanWindow::CenterImage()
{	
	imagerect.x=(win_w-imagerect.width)/2;
	imagerect.y=(win_h-imagerect.height)/2;
}

//! Resizes and centers the image.
int PanWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	CenterImage();
	needtodraw=1;
	return 0;
}

//! Resizes and centers the image.
int PanWindow::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	CenterImage();
	needtodraw=1;
	return 0;
}

} // namespace Laxkit

