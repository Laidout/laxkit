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
//    Copyright (C) 2004-2007,2011,2014 by Tom Lechner
//
#ifndef _LAX_FREEHANDINTERFACE_H
#define _LAX_FREEHANDINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/fillstyle.h>
#include <lax/interfaces/shapebrush.h>
#include <lax/interfaces/lineprofile.h>
#include <lax/gradientstrip.h>
#include <lax/interfaces/coordinate.h>


namespace LaxInterfaces { 


enum FreehandEditorStyles {
	FREEHAND_Coordinates     =(1<<0), //Construct Coordinate points (not implemented)
	FREEHAND_Flatpoints      =(1<<1), //Create a list of flatpoints (not implemented)
	FREEHAND_Raw_Path        =(1<<2), //Create a straight polyline PathsData with all input points
	FREEHAND_Poly_Path       =(1<<3), //Create a simplified polyline PathsData, approximated within a threshhold
	FREEHAND_Bez_Path        =(1<<4), //Create a bezier PathsData based on a simplified polyline
	FREEHAND_Bez_Outline     =(1<<5), //Create a bezier PathsData that is the outline of pressure sensitive line
	FREEHAND_Bez_Weighted    =(1<<6), //Create a bezier PathsData with PathWeightNode markers
	FREEHAND_Shape_Brush     =(1<<7), //Create a PathsData built from a shape brush
	
	FREEHAND_Color_Mesh      =(1<<8), //Create a ColorPatchData using pressure and a gradient
	FREEHAND_Double_Mesh     =(1<<9), //Create a ColorPatchData where the gradient is mirrored about the middle of the line
	FREEHAND_Grid_Mesh       =(1<<10), //Create a PatchData
	FREEHAND_Path_Mesh       =(1<<11), //Create a PatchData based on a bezier weighted path


	FREEHAND_Mesh            =((1<<8)|(1<<9)|(1<<10)|(1<<11)), //mask for any mesh
	FREEHAND_All_Types       =((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<9)|(1<<10)|(1<<11)),

	FREEHAND_Till_Closed     =(1<<12), //mouse down drag out a line, up and clicking adds points
	FREEHAND_Notify_All_Moves=(1<<13), //send events to owner upon every move
	FREEHAND_Lock_Type       =(1<<14),
	FREEHAND_Remove_On_Up    =(1<<15), //remove the interface from viewport when button up

	FREEHAND_MAX
};

class RawPoint {
  public:
	flatpoint p;
	int flag; //used by point simplfier
	clock_t time;
	double pressure;
	double tiltx,tilty;
	RawPoint() { time=0; pressure=0; tiltx=tilty=0; flag=0; }
	RawPoint(flatpoint pp) { p=pp; time=0; pressure=0; tiltx=tilty=0; flag=0; }
};

typedef Laxkit::PtrStack<RawPoint> RawPointLine;

class FreehandInterface : public anInterface
{
  protected:
	enum EditMode { MODE_Normal, MODE_Settings, MODE_BrushSize };
	EditMode mode;

	char showdecs;
	clock_t ignore_clock_t; // ignore_tip_time in clock_t ticks
	Laxkit::ShortcutHandler *sc;

	Laxkit::PtrStack<RawPointLine> lines; //one line per mouse id
	Laxkit::NumStack<int> deviceids;

	//----user settings
	double brush_size; //real size
	double close_threshhold;
	double smooth_pixel_threshhold;
	int ignore_tip_time; //milliseconds of final input to ignore, as pressure produces ugly ends
	
	LineStyle default_linestyle;
	FillStyle default_fillstyle;

	LineStyle *linestyle; //maybe resources.. overrides default
	FillStyle *fillstyle;

	ShapeBrush *shape_brush;
	LineProfile *line_profile;

	Laxkit::GradientStrip default_gradient;
	Laxkit::NumStack<double> gradient_pos;
	Laxkit::RefPtrStack<Laxkit::GradientStrip> gradients;

	//display settings:
	Laxkit::ScreenColor linecolor;
	Laxkit::ScreenColor pointcolor;
	//----end user settings

	//mode brush size adjust
	flatpoint size_center;
	flatpoint last_screen_pos;
	bool size_dragged;


	void SetupSettings();
	void RefreshSettings();

	int findLine(int id);

	virtual int send(int i);
	virtual void sendObject(LaxInterfaces::SomeData *tosend, int i);
	virtual void RecurseReduce(RawPointLine *l, int start, int end, double epsilon);
	virtual void RecurseReducePressure(RawPointLine *l, int start, int end, double epsilon);
	virtual RawPointLine *Reduce(int i, double epsilon);
	virtual RawPointLine *ReducePressure(int i, double epsilon);
	virtual Coordinate *BezApproximate(RawPointLine *l);

  public:
  	enum Actions {
  		FH_None = 0,
  		FH_Settings,
  		FH_MAX
  	};

	unsigned int freehand_style;


	FreehandInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~FreehandInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Freehand"; }
	virtual const char *Name();
	virtual const char *whattype() { return "FreehandInterface"; }
	virtual const char *whatdatatype() { return NULL; } // is creation only
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e,const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int IgnoreTipTime(int milliseconds);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	//virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);

	virtual unsigned int SendType(unsigned int type);
	virtual int Mode(EditMode newmode);

	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *loadcontext);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
};

} // namespace LaxInterfaces

#endif

