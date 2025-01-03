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
//    Copyright (C) 2009,2010 by Tom Lechner
//
#ifndef _LAX_STACKFRAME_H
#define _LAX_STACKFRAME_H

#include <lax/boxarrange.h>
#include <lax/anxapp.h>
#include <lax/winframebox.h>
#include <lax/buttondowninfo.h>

namespace Laxkit {

//-------------------------------- StackFrame -------------------------

#define STACKF_VERTICAL      (1<<16)
#define STACKF_NOT_SIZEABLE  (1<<17)
#define STACKF_ALLOW_SWAP    (1<<18)
#define STACKF_BEVEL         (1<<19)

	
class StackFrame : public anXWindow, public ListBox
{
	double *pos;  //!< Fraction of window extent in range 0..1 for where position of bar after box
	bool *sticky; //!< For each box, whether to have the ... between it and next box

 protected:
	int lastx,lasty, whichbar;
	int gap;
	ButtonDownInfo buttondown;
	int curshape;
	bool needtoremap;  //!< Need to moveresize kids based on pos, assumes child preferred metrics are set as desired
	bool needtolayout; //!< There was a window add/remove, we need to recompute pos based on metrics

	virtual int RebuildPos(bool useactual = false);

 public:
	StackFrame(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend,
		int ngap);
	virtual ~StackFrame();
	virtual const char *whattype() { return "StackFrame"; }
	virtual int init();
	virtual void Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int Event(const EventData *e,const char *mes);

	virtual anXWindow *GetWindow(int index);
	virtual bool HideSubBox(int index);
	virtual bool ShowSubBox(int index);
	virtual void sync();
	virtual int SyncFromPos(int add);
	virtual int WrapToExtent();
	virtual void MarkForLayout() { needtolayout = true; }
	virtual int findWhichBar(int x,int y);
	virtual int MoveBar(int index, int pixelamount, int shift);
	virtual int Gap() { return gap; }
	virtual int Gap(int ngap);
	virtual int ReplaceWin(anXWindow *win, int absorbcount, int index);
	virtual int AddWin(WinFrameBox *box,char islocal=1,int where=-1);
	virtual int AddWin(anXWindow *win,int absorbcount,int where=-1); // adds with what is w/h in window, no stretch
	virtual int AddWin(anXWindow *win,int absorbcount,
					int npw,int nws,int nwg,int nhalign,int nhgap,
					int nph,int nhs,int nhg,int nvalign,int nvgap,
					int where=-1);
	virtual int Remove(int which);
	virtual void MarkForLayout() { needtolayout = true; }
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);

	static StackFrame *HBox(const char *name = "hbox");
	static StackFrame *VBox(const char *name = "vbox");
};





} //namespace Laxkit


#endif 


