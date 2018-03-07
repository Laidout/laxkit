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
//    Copyright (C) 2004-2006,2011,2014 by Tom Lechner
//
#ifndef _LAX_ELLIPSEINTERFACE_H
#define _LAX_ELLIPSEINTERFACE_H


#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>

//using namespace std;

namespace LaxInterfaces {



//----------------------------- EllipseData -----------------------------

enum EllipseFlags {
	ELLIPSE_ISCIRCLE = (1<<0),
	ELLIPSEFLAGS_MAX
};

enum EllipsePoints {
	 //dev note: interface code is highly dependent on order here:
	ELLP_None=0,
	ELLP_TopLeft,
	ELLP_Top,
	ELLP_TopRight,
	ELLP_Left,
	ELLP_Center,
	ELLP_Right,
	ELLP_BottomLeft,
	ELLP_Bottom,
	ELLP_BottomRight,
	ELLP_Focus1,
	ELLP_Focus2,
	ELLP_StartAngle,
	ELLP_EndAngle,
	ELLP_XRadius,
	ELLP_YRadius,
	ELLP_OuterRadius,
	ELLP_InnerRadius,
	ELLP_WildPoint,

	ELLP_DragRect,
	ELLP_DragRectCenter,

	ELLP_MAX
};

class EllipseData : public SomeData
{
  protected:
  	
  public:

	unsigned int style;
	double start,end;
	double outer_r, inner_r;
	double a,b;
	flatpoint center,x,y; //center, x and y axis (in addition to this->m())
	LineStyle linestyle;

	EllipseData();
	virtual ~EllipseData();
	virtual SomeData *duplicate(SomeData *dup);
	virtual const char *whattype() { return "EllipseData"; }
	virtual void usefocus(flatpoint f1,flatpoint f2,double c=-1);
	virtual void FindBBox();
	virtual void SetStyle(unsigned int s, bool on);
	virtual bool GetStyle(unsigned int s);
	virtual flatpoint getpoint(EllipsePoints c, bool transform_to_parent);


	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
    virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what, LaxFiles::DumpContext *context);
    virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//----------------------------- EllipseInterface -----------------------------


class EllipseInterface : public anInterface
{
  protected:
	int hover_x,hover_y;
	int hover_point;
	EllipsePoints curpoint;
	flatpoint createp,createx,createy;
	int mode;

	bool inrect;
	RectInterface rinterf;
	RectData rdata;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	
  public:
	Laxkit::ScreenColor controlcolor;
	int creationstyle,createfrompoint,createangle,showdecs; // cfp: 0 (nw), 1 y, 2 x, 3 xy
	EllipseData *data;
	ObjectContext *eoc;
	LineStyle linestyle;

	EllipseInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~EllipseInterface();
	virtual const char *IconId() { return "Ellipse"; }
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual const char *Name();
	virtual const char *whattype() { return "EllipseInterface"; }
	virtual const char *whatdatatype() { return "EllipseData"; }
	virtual anInterface *duplicate(anInterface *dup);
	virtual void Clear(SomeData *d);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual void Dp(Laxkit::Displayer *ndp);

	virtual void deletedata();
	virtual void rectify();
	virtual void erectify();
	virtual EllipsePoints scan(int x,int y);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb);
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,
			Laxkit::anObject *a2=NULL,int info=0);
	virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);
	virtual flatpoint getpoint(EllipsePoints c, bool inparent=true);
};

} // namespace LaxInterfaces

#endif

