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
#include <lax/interfaces/curvemapinterface.h>


namespace LaxInterfaces {

	

enum CurveWindowStyles {
	CURVE_Show_Ranges=(1<<15),
};

class CurveWindow : public Laxkit::anXWindow
{
 protected:
	CurveMapInterface interface;

 public:

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
	virtual int init();
	virtual void Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual void ChangeEditable(unsigned int which, int on);

	 //serializing aids
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);


	 //curve specific functions:
	virtual void SetupRect();
	//virtual int scaneditable(int x,int y);
	//virtual int scan(int x,int y);
	//virtual int scannear(int x,int y, flatpoint *p_ret, int *index);
	virtual int MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue);
	virtual double f(double x);
	virtual Laxkit::CurveInfo *GetInfo() { return interface.GetInfo(); }
	virtual int SetInfo(Laxkit::CurveInfo *info);
	virtual int CopyInfo(Laxkit::CurveInfo *info);
	virtual int AddPoint(double x,double y);
	virtual int MovePoint(int index, double x,double y);
	virtual void Reset();
};


} // namespace LaxInterfaces

#endif



