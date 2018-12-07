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
//    Copyright (C) 2004-2011 by Tom Lechner
//
#ifndef _LAX_RULERWINDOW_H
#define _LAX_RULERWINDOW_H

#include <lax/anxapp.h>

namespace Laxkit {

#define RULER_X			(1<<16)
#define RULER_Y			(1<<17)
#define RULER_TOPTICKS  (1<<18)
#define RULER_LEFTTICKS (1<<18)

#define RULER_BOTTOMTICKS (1<<19)
#define RULER_RIGHTTICKS  (1<<19)

#define RULER_CENTERTICKS (1<<20)
#define RULER_NONUMBERS   (1<<21)

 // defaults standard :inches, base=12, metric=cm
#define RULER_STANDARD    (1<<22)
#define RULER_METRIC      (1<<23)

 // send message when button down, and mouse leaves ruler
#define RULER_SENDONLBD (1<<24)
#define RULER_SENDONMBD (1<<25)
#define RULER_SENDONRBD (1<<26)

#define RULER_UP_IS_POSITIVE  (1<<27)

#define RULER_UNITS_MENU        (1<<28)
#define RULER_UNITS_MENU_ALWAYS (1<<29)

 //sent in ruler messages when currentunits are changed
#define RULER_Units          1
#define RULER_AlwaysCurrent  (-2)


class RulerWindow : public anXWindow
{
  protected:
	double start,end,mag,umag; // real*mag=screen
	double tf,stf,sstf;
	anXWindow *trackwindow;
	LaxFont *smallnumbers;
	int baseunits, currentunits;
	virtual void adjustmetrics();
	virtual void drawtext(double n,int pos,int &textpos,int toff);

	virtual int NumberOfUnits();
	virtual int UnitInfo(int index, const char **name, int *id, double *scale, int *sdiv, int *ssdiv, const char **label);

  public:
	unsigned long bgcolor,numcolor,tickcolor,curposcolor,subtickcolor,subsubtickcolor;
	double base,unit;
	double curpos;
	int subdiv,subsubdiv,screenoffset;

	RulerWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev=NULL,unsigned long nowner=0,const char *nsend=NULL,
						const char *base_units=NULL, int syncwithscreen=1);
	virtual ~RulerWindow();
    virtual int ThemeChange(Theme *theme);
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual void Refresh();
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);

	virtual int SetBaseUnits(const char *units);
	virtual int SetBaseUnits(int units);
	virtual int SetCurrentUnits(const char *units);
	virtual int SetCurrentUnits(int units);
	virtual int SetDivisions(int sdiv, int ssdiv);

	virtual void TrackThisWindow(anXWindow *win);
	virtual void Track();
	virtual void SetPos(double crpos);
	virtual void SetPosFromScreen(int p);
	virtual void SetPosFromScreenCoord(int x,int y);
	virtual void SetMag(double nmag); 
	virtual void Zoom(double f);
	virtual void Set(double strt);
	virtual void Set(double strt,double fin);
	virtual void SetOrigin(int o);

	virtual void AddRange(int id, double start, double end);

	 //serializing aids
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

} // namespace Laxkit

#endif
