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
#ifndef _LAX_SCROLLER_H
#define _LAX_SCROLLER_H

#include <lax/anxapp.h>
#include <lax/pancontroller.h>
#include <lax/panuser.h>
#include <lax/buttondowninfo.h>

namespace Laxkit {

//    00000100  SC_ASPLIT    Split arrows
//    00001000  SC_ATOP	   Arrows at top
//    00010000  SC_ABOTTOM   Arrows at bottom


#define SC_XSCROLL             (1<<16)
#define SC_YSCROLL             (1<<17)
#define SC_ASPLIT              (1<<18)
#define SC_ATOP                (1<<19)
#define SC_ABOTTOM             (1<<20)
#define SC_NOARROWS            (1<<21)
#define SC_ZOOM                (1<<22)
#define SC_PAGE_IS_PERCENT     (1<<23)
#define SC_ELEMENT_IS_PERCENT  (1<<24)
#define SC_ALLOW_SMALL         (1<<25)



class Scroller : public PanUser, public anXWindow
{
 protected:
	ButtonDownInfo buttondown;
	int zh,ah,minboxlen;
	int toff,a1off,a2off,omx,omy;
	int tid;
	int idlemx,idlemy;
	virtual void redoarrows();
 public:
	 // colors:
	unsigned long bordercolor,wholecolor,trackcolor;
	Scroller(anXWindow *parnt,const char *nname, const char *ntitle, unsigned long nstyle,
			int nx,int ny,int nw,int nh,int brder,
			anXWindow *prev,unsigned long nowner,const char *mes,
			PanController *npan,
			long nmins=0,long nmaxs=0,long nps=0,long nes=0,long ncp=-1,long ncpe=-1); // ncp=ncpe=-1
	virtual ~Scroller();
	virtual int Event(const EventData *e,const char *mes);
	virtual void Refresh();
	virtual int Idle(int id=0);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int RBDown(int mx,int my,unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int mx,int my, unsigned int state,const LaxMouse *d);
	virtual int LBDown(int mx,int my,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int mx,int my, unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int mx,int my, unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);

	virtual void drawarrows();
	virtual void drawtrack();
	virtual void drawtrackbox();
	virtual long SetPageSize(long nps);
	virtual long GetPageSize();
	virtual long GetCurPos(long *poss=NULL,long *pose=NULL);
	virtual long GetCurPosEnd();
	virtual long SetCurPos(long newcurpos);
	virtual long SetCurPos(long start, long end);
	virtual long SetSize(long nmins,long nmaxs,long ncurpos,long ncurposend,long nps,long nes);
	virtual long SetSize(long nmins,long nmaxs,long nps=0);
	virtual int getpos(int mx,int my);
	virtual int send();
	virtual void send(long change,int pagesizechange=0);

	virtual long PageDown(int numpages=1);
	virtual long PageUp(int numpages=1);
	virtual long OneDown();
	virtual long OneUp();

	 //serializing aids
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

} // namespace Laxkit

#endif
