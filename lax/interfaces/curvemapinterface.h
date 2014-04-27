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
#ifndef _LAX_CURVEMAPINTERFACE_H
#define _LAX_CURVEMAPINTERFACE_H

#include <lax/curvewindow.h>
#include <lax/rectangles.h>
#include <lax/interfaces/aninterface.h>

namespace LaxInterfaces {

	


class CurveMapInterface : public anInterface
{
 protected:
	Laxkit::LaxFont *smallnumbers;
	Laxkit::IntRectangle bounds; //total bounds for curve and labels
	Laxkit::IntRectangle rect; //curve goes in here
	unsigned int curve_win_style;
	int firsttime;
	Laxkit::CurveInfo *curveinfo;
	Laxkit::WindowColors *win_colors;

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
	enum CurveMapStyles {
		Escapable
	};
	enum CurveMapInterfaceEditable {
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
	
	CurveMapInterface(int nid, Laxkit::Displayer *ndp,
						const char *nctitle=NULL,
						const char *xl=NULL, double nxmin=0, double nxmax=1,
						const char *yl=NULL, double nymin=0, double nymax=1);
	virtual ~CurveMapInterface();

    virtual const char *IconId() { return "CurveMap"; }
    virtual const char *Name() { return "Curve Interface"; }
    virtual const char *whatdatatype() { return NULL; }
	virtual const char *whattype() { return "CurveMapInterface"; }
	virtual void Clear(SomeData *d);

	virtual int Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	 //serializing aids
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);


	 //curve specific functions:
	virtual void ChangeEditable(unsigned int which, int on);
	virtual void SetupRect(int x,int y,int w,int h);
	virtual int scaneditable(int x,int y);
	virtual int scan(int x,int y);
	virtual int scannear(int x,int y, flatpoint *p_ret, int *index);
	virtual int MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue);
	virtual void send(int which=0);
	virtual double f(double x);
	virtual Laxkit::CurveInfo *GetInfo() { return curveinfo; }
	virtual int SetInfo(Laxkit::CurveInfo *info);
	virtual int AddPoint(double x,double y);
	virtual int MovePoint(int index, double x,double y);
	virtual void Reset();
};


} // namespace Laxkit

#endif



