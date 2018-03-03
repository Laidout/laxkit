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
//    Copyright (C) 2004-2012 by Tom Lechner
//
#ifndef _LAX_MESSAGEBAR_H
#define _LAX_MESSAGEBAR_H

#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

namespace Laxkit {

enum MessageBarTypes {
//***these must jive with xymesbar and filepreviewer defines
	MB_LEFT           =(1<<16),
	MB_RIGHT          =(1<<17),
	MB_CENTERX        =(1<<18),
	MB_CENTERY        =(1<<19),
	MB_CENTER         =(1<<18 | 1<<19), 
	MB_TOP            =(1<<20),
	MB_BOTTOM         =(1<<21),
	MB_MOVE           =(1<<22),
	MB_COPY           =(1<<23),
	MB_WRAP           =(1<<24),
	MB_LEAVE_DESTROYS =(1<<25),
	MB_BINARY         =(1<<26),
	
	MB_MAX
};

class MessageBar : public anXWindow
{
   protected:
	int lbdown,firsttime;
	int msx,msy,ox,oy;
	double ex,ey,fasc,fdes,height; // ex,ey are extents
	char **thetext;
	int nlines;
	double *indents;
	unsigned long textcolor,bkcolor,bordercolor;
	ButtonDownInfo buttondown;
   public:
	int padx,pady;
	MessageBar(anXWindow *pwindow,
				const char *nname,
				const char *ntitle,
				unsigned long nstyle,
				int nx,int ny,int nw,int nh,int brder,const char *newtext);
	virtual ~MessageBar();
	virtual int SetText(const char *newtext);
	virtual int SetupMetrics();
	virtual char *GetText();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual void Refresh();
	virtual const char *whattype() { return "MessageBar"; } 
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);

	 //serializing aids
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

} // namespace Laxkit

#endif

