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
#ifndef _LAX_PANPOPUP_H
#define _LAX_PANPOPUP_H

#include <lax/anxapp.h>
#include <lax/pancontroller.h>
#include <lax/panuser.h>
#include <lax/laximages.h>
	
namespace Laxkit {

 // Specifies that the window should try to center itself and the selbox
 // around the mouse.
#define PAN_IS_POPUP     (1<<16)
#define PAN_NEEDS_BDOWN  (1<<17)

//--------------------------------- PanPopup --------------------------------------
class PanPopup : public PanUser, public anXWindow
{
 protected:
 public:
	unsigned long bgcolor;
	int padx,pady;
	PanPopup(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
				int nx,int ny,int nw,int nh,int brder,
				anXWindow *prev,unsigned long nowner=0,const char *nsend=NULL,
				PanController *pan=NULL);
	virtual ~PanPopup();
	virtual void Refresh();
	virtual const char *whattype() { return "PanPopup"; } 
	virtual int LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int send();
};

//--------------------------------- PanWindow --------------------------------------
class PanWindow : public anXWindow, public PanUser
{
 protected:
	LaxImage *image; // contains the scaled down image
	XRectangle imagerect; // x,y of image in window
	int imagex,imagey; // x,y of image in window
	int xs,ys; // the x,y scaling
	int mx,my;
	unsigned long bgcolor;
	int buttondown;
	int bdowndevice;
 public:
	int pad,maxdim;
	PanWindow(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsend,
						int mouseid,
						PanController *pan,LaxImage *img,int nxs=0,int nys=0,int npad=0);
	virtual ~PanWindow();
	virtual void UseThisPanner(PanController *npanner);
	virtual void centerOnMouse(int mouseid);
	virtual void CenterImage();
	virtual int SetImage(LaxImage *imag);
	virtual int Event(const EventData *e,const char *mes);
	virtual int init();
	virtual void Refresh();
	virtual const char *whattype() { return "PanWindow"; } 
	virtual int LBDown(int x,int y,unsigned int state,int count, const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int send();
};

} // namespace Laxkit

#endif

