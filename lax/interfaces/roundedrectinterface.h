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
//    Copyright (C) 2025 by Tom Lechner
//
#ifndef _LAX_ROUNDEDRECTINTERFACE_H
#define _LAX_ROUNDEDRECTINTERFACE_H


#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/pathinterface.h>


namespace LaxInterfaces {



//----------------------------- RoundedRectData -----------------------------

enum RRectPoints {
	RRECT_None=0,
	RRECT_TopLeft, //do not change order of these, lest you mess up the hacky point checking
	RRECT_Top,
	RRECT_TopRight,
	RRECT_Left,
	RRECT_Center,
	RRECT_Right,
	RRECT_BottomLeft,
	RRECT_Bottom,
	RRECT_BottomRight,
	RRECT_Width,
	RRECT_Height,
	RRECT_DragRect,

	RRECT_TopLeftIn,
	RRECT_TopLeftOut,
	RRECT_TopRightIn,
	RRECT_TopRightOut,
	RRECT_BottomRightIn,
	RRECT_BottomRightOut,
	RRECT_BottomLeftIn,
	RRECT_BottomLeftOut,

	RRECT_MAX
};

class RoundedRectData : virtual public SomeData
{
  protected:
  	
  public:
	bool is_square = false; // hint so the interface keeps width and height the same
	int round_style; // 0 = ellipsoidal, 1 = squircle, 2 = custom bevel
	double x_align = 50.0;
	double y_align = 50.0;
	double width = 1.0;
	double height = 1.0;
	
	double round[8]; // clockwise from upper left: in,out, upper-right-in,out, ...
	enum RoundType { Symmetric=0, Asymmetric=1, Independent=2 };
	int round_type; // symmetric for all, different x/y but same for all, different for all

	LineStyle *linestyle;
	FillStyle *fillstyle;

	RoundedRectData();
	virtual ~RoundedRectData();
	virtual SomeData *duplicateData(SomeData *dup);
	virtual const char *whattype() { return "RoundedRectData"; }

	virtual void InstallLineStyle(LineStyle *newlinestyle);
	virtual void InstallFillStyle(FillStyle *newfillstyle);
	virtual int  fill(Laxkit::ScreenColor *color);

	virtual void FindBBox();
	virtual Laxkit::flatpoint getpoint(RRectPoints c, bool transform_to_parent);
	virtual bool IsSquare() { return width == height; }
	
	virtual int GetPath(Laxkit::flatpoint *pts_ret);
	virtual PathsData *ToPath();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
    virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


//----------------------------- RoundedRectInterface -----------------------------


class RoundedRectInterface : public anInterface
{
  public:
  	enum RRectActions {
		//RRECT_Move,
		//RRECT_DragRect,
		//RRECT_DragRectCenter,

		RRECT_UsePaths, //create PathsData objects, else make RoundedRectData
		RRECT_Outline_Only, // ignore fill/stroke styles

		RRECT_Squircle,
		RRECT_Ellipsoidal,
		RRECT_RemoveRound,
		RRECT_ToggleRectControls,
		RRECT_EditBevel,

		RRECT_UseLineStyle,
		RRECT_MakeLineResource,
		RRECT_MakeLineLocal,
		RRECT_UseFillStyle,
		RRECT_MakeFillResource,
		RRECT_MakeFillLocal,

		RRECT_ToggleDecs,
		RRECT_ToggleSquare,
		RRECT_SetWidth,
		RRECT_ResetAlignment,
		RRECT_ApplyTransform,
		
		RRECT_ACTIONS_MAX
	};

  protected:
	int hover_x,hover_y;
	RRectPoints hover_point;
	RRectPoints curpoint;
	Laxkit::flatpoint ref_point;
	Laxkit::flatpoint ref_point2;
	Laxkit::flatpoint createp,createx,createy;
	int mode;

	bool allow_width;
	bool show_width;

	LineStyle *default_linestyle;
	FillStyle *default_fillstyle;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	
  public:
	Laxkit::ScreenColor controlcolor;
	Laxkit::ScreenColor controlcolor_rim;
	int creationstyle,createfrompoint,createangle,showdecs; // cfp: 0 (nw), 1 y, 2 x, 3 xy

	RoundedRectData *data;
	ObjectContext *roc;


	RoundedRectInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~RoundedRectInterface();
	virtual const char *IconId() { return "RoundedRectangle"; }
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual const char *Name();
	virtual const char *whattype() { return "RoundedRectInterface"; }
	virtual const char *whatdatatype() { return "RoundedRectData"; }
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual void Clear(SomeData *d);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual void Dp(Laxkit::Displayer *ndp);
	virtual ObjectContext *Context() { return roc; }
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);

	virtual LineStyle *DefaultLineStyle();
	virtual FillStyle *DefaultFillStyle();

	virtual void deletedata();
	virtual RRectPoints scan(double x,double y, unsigned int state);
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
	virtual void UpdateViewportColor();
	virtual Laxkit::flatpoint getpoint(RRectPoints c, bool inparent);
	virtual bool getBar(RRectPoints which, Laxkit::flatpoint &p1_ret, Laxkit::flatpoint &p2_ret, bool transform_to_parent, double gap, double threshhold);
};

} // namespace LaxInterfaces

#endif

