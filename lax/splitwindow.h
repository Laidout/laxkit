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
#ifndef _LAX_SPLITWINDOW_H
#define _LAX_SPLITWINDOW_H

#include <lax/anxapp.h>
#include <lax/strmanip.h>
#include <lax/menuinfo.h>
#include <lax/buttondowninfo.h>
#include <lax/newwindowobject.h>


namespace Laxkit {

//---------------------------- PlainWinBox ---------------------------------
class PlainWinBox 
{ 
  protected:
	anXWindow *window, *prev,*next;

  public:
	int x1,y1,x2,y2;
	PlainWinBox(anXWindow *nwin,int xx1,int yy1,int xx2,int yy2);
	virtual ~PlainWinBox();
	virtual void NewWindow(anXWindow *nwin);
	virtual anXWindow *win() { return window; }
	virtual void sync(int inset,int mapit=-1);
};


//---------------------------- WinFuncNode ---------------------------------
typedef NewWindowObject WinFuncNode;

//---------------------------- SplitWindow ---------------------------------

#define SPLIT_WITH_SAME     (1<<16)
#define SPLIT_WITH_DEFAULT  (1<<17)
#define SPLIT_WITH_BLANK    (1<<18)
#define SPLIT_BEVEL         (1<<19)
#define SPLIT_DRAG_MAPPED   (1<<20)
#define SPLIT_STACKED_PANES (1<<21)
#define SPLIT_TABBED_PANES  (1<<22)
#define SPLIT_NO_MAXIMIZE   (1<<23)

//These are return codes for the SplitWindow menu:
#define SPLITW_Split          1
#define SPLITW_Join           2
#define SPLITW_ChangeTo_Start 101
#define SPLITW_ChangeTo_Blank 100
#define SPLITW_Mark           3
#define SPLITW_Swap_With_Mark 4
#define SPLITW_UnMaximize     5
#define SPLITW_Maximize       5


class SplitWindow : public anXWindow
{
	int cleanup_timer = 0;

  protected:
	ButtonDownInfo buttondown;
	int mousein;
	PtrStack<PlainWinBox> windows;
	PlainWinBox *curbox,*marked;
	int space; // width of space between windows
	int bevel; // amount to add per window for a bevel
	int minx,curx,maxx, miny,cury,maxy;
	int sminx,smaxx, sminy,smaxy;
	int mx,my;
	int mode;
	int lastbox;
	anXWindow *lastactivewindow;
	NumStack<int> laffected,raffected,taffected,baffected;
	PtrStack<WinFuncNode> winfuncs;
	int defaultwinfunc;
	virtual void scaleWindows();
	virtual void drawmovemarks(int on=1);
	virtual void drawsplitmarks();
	virtual PlainWinBox *findcurbox(int x,int y,const LaxMouse *mouse);
	virtual void setupsplit(int x,int y);
	virtual int splitthewindow(anXWindow *fillwindow=NULL,int whichside=0);
	virtual int joinwindow(int x,int y,char keepother);
	virtual void syncaffected();
	virtual anXWindow *NewWindow(const char *wtype,anXWindow *likethis=NULL);
	virtual MenuInfo *GetMenu();
	virtual void CheckHover();
	virtual void InitCleanupCheck();

  public:
	SplitWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long owner,const char *mes);
	virtual ~SplitWindow() {}
	virtual const char *whattype() { return "SplitWindow"; }
	virtual int init();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d); 
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d); 
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int FocusOn(const FocusChangeData *e);
	virtual int FocusOff(const FocusChangeData *e);
	virtual int Event(const EventData *e,const char *mes);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual void Refresh();
	virtual int  Idle(int tid, double delta);
	
	virtual int NumPanes() { return windows.n; }
	virtual PlainWinBox *GetPane(int which);
	virtual anXWindow *GetPaneWindow(int which);
	virtual int AddWindowType(const char *wtype,const char *ndesc,
			unsigned long style,NewWindowFunc winfunc,int settodefault=0);
	virtual void BoxNearPos(int x,int y,PlainWinBox *box, int *l,int *r,int *t,int *b);
	virtual void PosInBox(int x,int y,PlainWinBox *box, int *xx,int *yy);
	virtual void SetSpace(int spc);
	virtual int Split(int c,int whichside,anXWindow *fillwindow=NULL);
	virtual int Join(int c,int whichside,char keepother=0);
	virtual int Mark(int c);
	virtual int SwapWithMarked();
	virtual int Curbox(int c);
	virtual int FindBox(int x,int y);
	virtual int FindBox(anXWindow *newcurbox);
	virtual int FindBoxIndex(PlainWinBox *box);
	virtual int Change(anXWindow *towhat,int absorbcount, int which);
	virtual int Add(anXWindow *win,unsigned int whichside=LAX_BOTTOM,int absorbcount=0);
	virtual int Add(const char *type,unsigned int whichside=LAX_BOTTOM);
	virtual int RemoveCurbox();
	virtual int GetAffected(int x,int y,const LaxMouse *m);
	virtual int SetCursor(const char *curs,LaxMouse *d);
	virtual void validateX();
	virtual void validateY();
	virtual int Maximize(int which);
	virtual int Mode() { return mode; }
};

} // namespace Laxkit

#endif

