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
//    Copyright (C) 2004-2007,2010-2012 by Tom Lechner
//
#ifndef _LAX_COLORBOX_H
#define _LAX_COLORBOX_H

#include <lax/anxapp.h>
#include <lax/colorbase.h>
#include <lax/buttondowninfo.h>
#include <lax/newwindowobject.h>
#include <lax/colors.h>

namespace Laxkit {



#define COLORBOX_FG          (1<<20)
#define COLORBOX_FGBG        (1<<21)
#define COLORBOX_STROKEFILL  (1<<22)

#define COLORBOX_ALLOW_NONE         (1<<23)
#define COLORBOX_ALLOW_KNOCKOUT     (1<<24)
#define COLORBOX_ALLOW_REGISTRATION (1<<25)

#define COLORBOX_SEND_ALL    (1<<26)


//------------------------------- ColorBox ------------------------------
class ColorBox : public anXWindow, virtual public ColorBase
{
  protected:
	int colormap[9];
	ButtonDownInfo buttondown;
	NewWindowObject *colorselector;

	virtual int send(int which=-1);
	virtual void SetCurrentColor(int x,int y);

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	
  public:
	enum ColorBoxActions {
		COLORBOXA_SelectNone,        
		COLORBOXA_SelectRegistration,
		COLORBOXA_SelectKnockout,    
		COLORBOXA_SelectNormal,    
		COLORBOXA_SwapColors,    
		COLORBOXA_ToggleNone,    
		COLORBOXA_MAX
	};

	int currentid;
	double *topcolor; //for fg/bg mode with one box drawn on the other
	double step;
	int sendtype; //LAX_COLOR_*

	double strokew, strokeh;

	//Color *color1, *color2;

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
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int init();
	virtual int SetSpecial(int newspecial);
	virtual int SetIndex(int index);
	virtual int SetMode(int mode);
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

	virtual int PopupColorSelector(int x,int y);
	virtual void Updated();
};

} // namespace Laxkit

#endif 

