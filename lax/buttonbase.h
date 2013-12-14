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
#ifndef _LAX_BUTTONBASE_H
#define _LAX_BUTTONBASE_H


#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

 // sends on LBUp up and mousein
 // it is assumes that there is only standard grayed, on, off
#define BUTTON_MOMENTARY (1<<16)

 // sends on any toggle and LBUp
#define BUTTON_TOGGLE    (1<<17)

#define BUTTON_OK           (0x1<<24)
#define BUTTON_CANCEL       (0x2<<24)
#define BUTTON_OPEN         (0x3<<24)
#define BUTTON_SAVE         (0x4<<24)
#define BUTTON_SAVE_AS      (0x5<<24)
#define BUTTON_SAVE_ALL     (0x6<<24)
#define BUTTON_CLOSE        (0x7<<24)
#define BUTTON_CLOSE_ALL    (0x8<<24)
#define BUTTON_QUIT         (0x9<<24)
#define BUTTON_QUIT_ANYWAY  (0xa<<24)
#define BUTTON_PRINT        (0xb<<24)
#define BUTTON_PREVIEW      (0xc<<24)
#define BUTTON_YES          (0xd<<24)
#define BUTTON_NO           (0xe<<24)
#define BUTTON_OVERWRITE    (0xf<<24)

#define BUTTON_TEXT_MASK         (0xff<<24)

namespace Laxkit {

class ButtonBase : public anXWindow
{
 protected:
	int mousein,state,oldstate, id;
	ButtonBase *nextbutton,*prevbutton;
	ButtonDownInfo buttondown;
	unsigned int button_style;
 public:
	unsigned int highlight,shadow;
	int bevel;
	ButtonBase(anXWindow *parnt,const char *nname,const char *ntitle,
		   unsigned long nstyle,int xx,int yy,int ww,int hh,int brder,
		   anXWindow *prev,unsigned long nowner,const char *nsendmes,
		   int nid=0); 
	virtual ~ButtonBase();
	virtual int Grayed();
	virtual int Grayed(int g);
	virtual int LBDown(int x,int y,unsigned int wstate,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int wstate,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int wstate,int count,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int wstate,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int Event(const EventData *e,const char *mes);
	virtual int send(int deviceid,int direction);

	virtual void Refresh();
	virtual void draw() = 0;
	virtual void drawon() { draw(); }
	virtual void drawoff() { draw(); }
	virtual void drawgrayed() { drawoff(); }
	virtual void drawother() {}

	virtual int toggle();
	virtual int State(int newstate);
	virtual int State() { return state; }
};

} // namespace Laxkit


#endif 

