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
//    Copyright (C) 2013-2014 by Tom Lechner
//
#ifndef _LAX_CURVEMAPINTERFACE_H
#define _LAX_CURVEMAPINTERFACE_H

#include <lax/curveinfo.h>
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
	Laxkit::WindowStyle *win_themestyle;

	int show_label_ranges;
	int show_labels;
	int show_hover;
	int always_refresh_lookup;
	int highlighteditable;

	Laxkit::flatpoint lastpoint; //in real space
	int lasthover;
	Laxkit::flatpoint hoverpoint;
	int draglimbo;
	Laxkit::flatpoint ClampPoint(Laxkit::flatpoint p, int pp);

	int *histogram; //in range [0..1000]
	int hist_n;


	Laxkit::ShortcutHandler *sc;
    virtual int PerformAction(int action);

 public:
	enum CurveMapStyles {
		Escapable      = (1<<0),
		RealSpace      = (1<<1),
		RealSpaceMouse = (1<<2),
		Expandable     = (1<<3)
	};
	enum CurveMapInterfaceEditable {
		YMax  =(1<<0),
		YMin  =(1<<1),
		XMax  =(1<<2),
		XMin  =(1<<3),
		YUnits=(1<<4),
		XUnits=(1<<5),
		ExpandUL = -2,
		ExpandUR = -3,
		ExpandLL = -4,
		ExpandLR = -5,
		AddNear  = -6
	};

	unsigned int style;
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
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int InitializeResources();

	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata, Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);

	 //serializing aids
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);


	 //curve specific functions:
	virtual void ChangeEditable(unsigned int which, int on);
	virtual void SetupRect(double x,double y,double w,double h);
	virtual int scaneditable(double x,double y);
	virtual int scanExpandable(double x,double y);
	virtual int scan(double x,double y);
	virtual int scannear(double x,double y, Laxkit::flatpoint *p_ret, int *index);
	virtual int MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue);
	virtual void send(int which=0);
	virtual double f(double x);
	virtual Laxkit::CurveInfo *GetInfo() { return curveinfo; }
	virtual int CopyInfo(Laxkit::CurveInfo *info);
	virtual int SetInfo(Laxkit::CurveInfo *info);
	virtual int AddPoint(double x,double y);
	virtual int MovePoint(int index, double x,double y);
	virtual void Reset();
};


} // namespace Laxkit

#endif



