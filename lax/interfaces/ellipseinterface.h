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
#include <lax/interfaces/pathinterface.h>


namespace LaxInterfaces {



//----------------------------- EllipseData -----------------------------

enum EllipsePoints {
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
	ELLP_StartAngle,
	ELLP_EndAngle,
	ELLP_WedgeToggle,
	ELLP_Focus1,
	ELLP_Focus2,
	ELLP_XRadius, //a
	ELLP_XRadiusN, //a on -x
	ELLP_YRadius, //b
	ELLP_YRadiusN, //b on -y
	ELLP_OuterRadius,
	ELLP_InnerRadius,
	ELLP_WildPoint, //point that changes c but preserves focal points
	ELLP_DragRect,
	ELLP_MAX
};

class EllipseData : virtual public SomeData
{
  protected:
  	
  public:

  	enum EllipseFlags {
		ELLIPSE_IsCircle = (1<<0),
		ELLIPSE_NoInner = (1<<1),
		ELLIPSE_Wedge = (1<<2),
		ELLIPSE_Chord = (1<<3),
		ELLIPSE_Open = (1<<4),
		ELLIPSE_Closed = (1<<5),
		ELLIPSEFLAGS_MAX
	};

	unsigned int style;
	double start,end;
	int wedge_type;
	double inner_r; //actual r = inner_r * (default), for rings
	//double inner_round[8], outer_round[8]; //for rounded corners
	double a,b; // x and y half height
	Laxkit::flatpoint center,x,y; //center, x and y axis (in addition to this->m())

	LineStyle *linestyle;
	FillStyle *fillstyle;

	EllipseData();
	virtual ~EllipseData();
	virtual SomeData *duplicate(SomeData *dup);
	virtual const char *whattype() { return "EllipseData"; }

	virtual void InstallDefaultLineStyle();
	virtual void SetFoci(Laxkit::flatpoint f1,Laxkit::flatpoint f2,double c=-1);
	virtual void GetFoci(Laxkit::flatpoint *f1,Laxkit::flatpoint *f2,double *c);
	virtual void FindBBox();
	virtual void SetStyle(unsigned int s, bool on);
	virtual bool GetStyle(unsigned int s);
	virtual Laxkit::flatpoint getpoint(EllipsePoints c, bool transform_to_parent);
	virtual bool IsCircle() { return a == b; }
	virtual bool UsesAngles();
	virtual void MakeAligned();

	virtual int GetPath(int which, Laxkit::flatpoint *pts_ret);
	virtual PathsData *ToPath();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
    virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


//----------------------------- EllipseInterface -----------------------------


class EllipseInterface : public anInterface
{
  public:
  	enum EllipseActions {
		//ELLP_Move,
		//ELLP_DragRect,
		//ELLP_DragRectCenter,

		ELLP_UsePaths, //create PathsData objects, else make EllipseData
		ELLP_Outline_Only,

		ELLP_ToggleDecs,
		ELLP_ToggleCircle,
		ELLP_ToggleFoci,
		ELLP_ToggleColorDest,
		ELLP_CloseGap,
		ELLP_FlipGap,
		ELLP_UseWedge,
		ELLP_UseChord,
		ELLP_UseOpen,
		ELLP_SetWidth,
		ELLP_ResetAlignment,
		
		ELLACTIONS_MAX
	};

  protected:
	int hover_x,hover_y;
	EllipsePoints hover_point;
	EllipsePoints curpoint;
	Laxkit::flatpoint ref_point;
	Laxkit::flatpoint ref_point2;
	int ref_wedge;
	Laxkit::flatpoint createp,createx,createy;
	int mode;

	bool allow_foci;
	bool allow_angles;
	bool allow_inner;
	bool allow_width;

	bool show_foci;
	bool show_width;

	bool color_stroke;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	
  public:
	Laxkit::ScreenColor controlcolor;
	Laxkit::ScreenColor controlcolor_rim;
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
	virtual ObjectContext *Context() { return eoc; }
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);

	virtual void deletedata();
	virtual EllipsePoints scan(double x,double y, unsigned int state);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb);
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,
			Laxkit::anObject *a2=NULL,int info=0);
	virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);
	virtual int UseThisObject(ObjectContext *oc, SomeData *other_object);
	virtual int UseThisObject(ObjectContext *oc) override;
	virtual Laxkit::flatpoint getpoint(EllipsePoints c, bool inparent);

	virtual bool ShowFoci(bool yes);
};

} // namespace LaxInterfaces

#endif

