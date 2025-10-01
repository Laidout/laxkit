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
//    Copyright (C) 2004-2007,2010-2011 by Tom Lechner
//
#ifndef _LAX_RECTINTERFACE_H
#define _LAX_RECTINTERFACE_H


#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/rectpointdefs.h>

namespace LaxInterfaces {

//----------------------------- RectData ----------------------------------
enum RectDataStyle {
	RECT_ISSQUARE        =(1<<0),
	RECT_OFF             =(1<<1),
	RECT_DOTTED          =(1<<2),
	RECT_SOLID           =(1<<3),
	RECT_INVISIBLECENTER =(1<<4),
	RECT_CANTCREATE      =(1<<5),
	RECT_AFFINE          =(1<<6),
	RECT_ALLOW_SHEAR     =(1<<7),
	RECT_NO_SHEAR        =(1<<8),
	RECT_HIDE_CONTROLS   =(1<<9),
	RECT_OBJECT_SHUNT    =(1<<10),
	RECT_FLIP_AT_SIDES   =(1<<11),
	RECT_FLIP_LINE       =(1<<12),
	RECT_LINK_BALL       =(1<<13),

	RECT_STYLE_MAX
};

class RectData : public SomeData
{
  protected:
  	
  public:
	unsigned int style;
	int griddivisions;
	int centertype; // like rectinterface numbering, <1 means no center (center=p), >9 means stationary
	Laxkit::flatpoint center,center2,shearpoint;
	LineStyle *linestyle;

	RectData();
	RectData(Laxkit::flatpoint pp,double ww,double hh,int ct,unsigned int stle);
	virtual ~RectData();
	virtual void centercenter();
	virtual const char *whattype() { return "RectData"; }
};


//----------------------------- RectInterface ----------------------------------

enum RectInterfaceActions {
	RIA_Decorations = RP_MAX,
	RIA_Normalize,
	RIA_Rectify,
	RIA_Constrain,
	RIA_MoveCenter,
	RIA_ExpandHandle,
	RIA_ContractHandle,
	RIA_FlipHorizontal,
	RIA_FlipVertical,
	RIA_ToggleFlipControls,
	RIA_RotateCW,
	RIA_RotateCCW,
	RIA_MAX
};


class RectInterface : public anInterface
{
  protected:
	int lastpoint;
	Laxkit::flatpoint createp,createx,createy;
	Laxkit::flatpoint center1,center2,shearpoint,leftp;
	Laxkit::flatpoint flip1,flip2;
	double rotatestep;
	Laxkit::flatpoint drag_rotate_center;
	double drag_rotate_radius;
	double drag_rotate_min_radius; //inside of which you can mouse up on number input
	double drag_scale_width; //drag vertically outside this lets you select number to edit
	double snap_running_angle;
	enum DragMode {
		DRAG_None,
		DRAG_Move,
		DRAG_Scale,
		DRAG_Rotate,
		DRAG_MAX
	};
	int drag_mode; //See DragMode
	Laxkit::Affine drag_tr_on_down;
	int hover;
	char constrain_to; //0, 'x', or 'y'
	int mousetarget;
	int shiftmode;
	Laxkit::flatpoint hoverpoint;
	double rw,rh;
	Laxkit::flatpoint origin,xdir,ydir;
	double xaxislen,yaxislen;
	double extra_context[6];
	bool use_extra;
	Laxkit::LaxFont *font;

	virtual void syncToData();
	virtual void syncFromData(int first);
	virtual const char *hoverMessage(int p);
	virtual void Flip(int type);
	virtual void Rotate(double angle);

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	virtual int GetMode();
	virtual void Modified(int level=0);

	virtual Laxkit::flatpoint ObjectToScreen(Laxkit::flatpoint p);
	virtual Laxkit::flatpoint ScreenToObject(double x,double y);
	virtual Laxkit::flatpoint ScreenToObjectParent(double x,double y);

  public:
	int maxtouchlen;
	int extrapoints;
	int griddivisions;
	unsigned int style;
	Laxkit::ScreenColor controlcolor, controltransp;
	int creationstyle,createfrompoint,showdecs;

	SomeData *somedata;
	RectData *data;

	RectInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~RectInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual const char *IconId() { return ""; }
	virtual const char *Name();
	virtual const char *whattype() { return "RectInterface"; }
	virtual const char *whatdatatype() { return "RectData"; }
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Mapped();
	virtual void Unmapped();
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int FakeLBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d, int dragaction = RP_Move);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int); // assumes not use local
	virtual void ExtraContext(const double *mm);
	virtual void Clear(SomeData *d=NULL);
	virtual int Event(const Laxkit::EventData *e,const char *mes);

	virtual int AlternateScan(Laxkit::flatpoint sp, Laxkit::flatpoint p, double xmag,double ymag, double onepix);
	virtual int scan(int x,int y);
	virtual int SelectPoint(int c);
	virtual void deletedata();
	virtual Laxkit::flatpoint getpoint(int c,int trans);
	virtual void GetOuterRect(Laxkit::DoubleBBox *box, double *mm);
};

} // namespace LaxInterfaces

#endif

