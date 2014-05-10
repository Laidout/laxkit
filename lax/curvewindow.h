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
//    Copyright (C) 2013-2014 by Tom Lechner
//
#ifndef _LAX_CURVEWINDOW_H
#define _LAX_CURVEWINDOW_H

#include <lax/anxapp.h>
#include <lax/rectangles.h>
#include <lax/buttondowninfo.h>
#include <lax/curveinfo.h>

namespace Laxkit {

	

enum CurveWindowStyles {
	CURVE_Show_Ranges=(1<<15),
};

class CurveWindow : public anXWindow
{
 protected:
	LaxFont *smallnumbers;
	IntRectangle rect;
	unsigned int curve_win_style;
	int firsttime;
	CurveInfo *curveinfo;
	ButtonDownInfo buttondown;

	int show_label_ranges;
	int show_labels;
	int always_refresh_lookup;
	int highlighteditable;

	flatpoint lastpoint;
	int draglimbo;
	flatpoint ClampPoint(flatpoint p, int pp);

	int *histogram; //in range [0..1000]
	int hist_n;

 public:
	enum CurveWindowEditable {
		YMax  =(1<<0),
		YMin  =(1<<1),
		XMax  =(1<<2),
		XMin  =(1<<3),
		YUnits=(1<<4),
		XUnits=(1<<5)
	};

	int padouter, padinner;
	unsigned int curve_color;
	unsigned int graph_color;
	unsigned int editable; //mask of enum curvewindoweditable
	
	CurveWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev, unsigned long nowner, const char *nsend,
						const char *nctitle=NULL,
						const char *xl=NULL, double nxmin=0, double nxmax=1,
						const char *yl=NULL, double nymin=0, double nymax=1);
	virtual ~CurveWindow();
	virtual const char *whattype() { return "CurveWindow"; }
	//virtual int init();
	virtual void Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	//virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int Event(const EventData *e,const char *mes);
	virtual void ChangeEditable(unsigned int which, int on);

	 //serializing aids
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,anObject *context);


	 //curve specific functions:
	virtual void SetupRect();
	virtual int scaneditable(int x,int y);
	virtual int scan(int x,int y);
	virtual int scannear(int x,int y, flatpoint *p_ret, int *index);
	virtual int MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue);
	virtual void send(int which=0);
	virtual double f(double x);
	virtual CurveInfo *GetInfo() { return curveinfo; }
	virtual int SetInfo(CurveInfo *info);
	virtual int CopyInfo(CurveInfo *info);
	virtual int AddPoint(double x,double y);
	virtual int MovePoint(int index, double x,double y);
	virtual void Reset();
};


} // namespace Laxkit

#endif



