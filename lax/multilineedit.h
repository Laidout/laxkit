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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_GOODEDITWW_H
#define _LAX_GOODEDITWW_H


#include <lax/textxeditbase-utf8.h>
#include <lax/scroller.h>
#include <lax/buttondowninfo.h>

#define GOODEDITWW_Y_IS_CHARS   (1<<16)


namespace Laxkit {

class MultiLineEdit : public TextXEditBaseUtf8
{
  protected:
	char *blanktext;
	ButtonDownInfo buttondown;
	int cdir,scrollwidth,mostpixwide,mostcharswide,maxpixwide;
	int numlines,lpers,longestline;
	int mx,my;
	struct Linestat {
		long start;
		int pixlen,indent;
	} *linestats;
	Scroller *xscroller,*yscroller;
	int xscrollislocal,yscrollislocal;
	virtual int send() { return 0; }
 public:
	int padx,pady;
	MultiLineEdit(anXWindow *prnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner=0,const char *nsend=NULL,
						unsigned int ntstyle=0,const char *newtext=NULL);
	virtual ~MultiLineEdit();
	virtual void UseTheseScrollers(Scroller *xscroll,Scroller *yscroll);
	 // if goodeditww uses outside supplied scrollers, the must be passed on to 
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int LBDown(int x,int y, unsigned int state,int count,const LaxMouse *d);
	virtual int LBDblClick(int x,int y, unsigned int state,const LaxMouse *d);
	virtual int LBUp(int x,int y, unsigned int state,const LaxMouse *d);
	virtual int RBDown(int x,int y, unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int Idle(int tid=0);
	virtual int Resize(int nw,int nh);
	virtual int MoveResize(int nx,int ny,int nw,int nh);

	virtual int Find(char *str,int fromcurs=1); // fromcurs=1 
	virtual int Replace(const char *oldstr,const char *newstr,int all);
	virtual int SetText(const char *newtext);
	virtual long SetCurPos(long newcurpos);
	virtual int SetSelection(long newss,long newse);
	 // NOTE: the scrollers are always flush with right and bottom, and newyssize,newxssize modify textrect
	virtual int newyssize();
	virtual int newxssize(int p=1); //p=1, p is to prevent race with newyssize
	 // called anytime changes are made in thetext
	 // it changes linestats, needtodraw, dpos, nlines
	virtual void wwinspect(int fromline=0); //fromline=0, this and fineline are the linebreaking procs
	virtual int inschar(int ch);
	virtual int delchar(int bksp); // assumes not sellen 
	virtual int insstring(const char *blah,int after=0);	// after=0
	virtual int replacesel(int ch);
	virtual int delsel();
	virtual int getscreenline(long pos);
	virtual int replacesel(const char *newt,int after=0); //newt=NULL deletes, after=0
	 // return position most appropriate for x=pix,y=l max <= pi
	 // sets curpos to pos near (x,y)=(l,pix)
	virtual long findpos(int l,int pix,int updatecp=1,int conv=1);
	virtual long countout(int linenumber,int &pout);
	virtual int makeinwindow();  // assumes cx,cy already set accurately 
	virtual void findcaret();
	 // returns 1 if there is a change, 0 if not
	virtual int ShiftScreen(int x,long y);  // ylines 
	virtual int isword(long pos);
	 // returns pos of start of next potential screen line after ls, assumes ilsofar set right
	 // updates ilsofar which basically becomes pixlen of line with ls
	virtual long findline(long ls,int &ilsofar);
	 // find what should be the start of the screen line that contains pos
	virtual long findlinestart(long pos=-1); //pos=-1 means use curpos
	 // this remaps linestats, but does not findcaret
	 // it does find and set mostpixwide,longestline, dpos,nlines
	virtual int makelinestart(int startline,long ulwc,int godown,int ifdelete);
	 // does not recompute linestats.pixlens
	virtual int Getmostwide();  // returns which line 
	virtual long WhichLine(long pos);
	virtual void DrawText(int black=1); // black=1 
	virtual int SetupMetrics();
	virtual void SetupScreen();

    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *loadcontext);
 
};

} // namespace Laxkit

#endif
