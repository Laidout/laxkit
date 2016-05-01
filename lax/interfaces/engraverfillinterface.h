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
//    Copyright (C) 2014,2015 by Tom Lechner
//
#ifndef _LAX_ENGRAVERFILLINTERFACE_H
#define _LAX_ENGRAVERFILLINTERFACE_H

#include <lax/interfaces/engraverfilldata.h>
#include <lax/interfaces/patchinterface.h>
#include <lax/interfaces/lineprofile.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/screencolor.h>
#include <lax/curveinfo.h>
#include <lax/noise.h>


namespace LaxInterfaces {


//------------------------------ EngraverFillInterface -------------------------------

//class EngraverInterfaceSettings
//{
//  public:
//	EngraverInterfaceSettings();
//	virtual ~EngraverInterfaceSettings() {}
//
//	Laxkit::ScreenColor fgcolor,bgcolor;
//
//	int panelwidth;
//
//	double sensitive_thickness;
//	double sensitive_turbulence;
//	double sensitive_drag;
//	double sensitive_pushPull;
//	double sensitive_avoidToward;
//	double sensitive_twirl;
//};

class EngraverFillInterface : public PatchInterface
{
  private:
	unsigned int eventobject; //bit of a hack to remember which event when calling
	int eventgroup;          //helper windows from the group list

  protected:
	Laxkit::MenuInfo modes;
	EngraverFillData *edata;
	int mode;
	int controlmode;
	int submode;
	int current_group;

	 //general tool settings
	double brush_radius; //screen pixels
	Laxkit::CurveInfo thickness; //ramp of thickness brush
	double sensitive_thickness;
	double sensitive_turbulence;
	double sensitive_drag;
	double sensitive_pushpull;
	double sensitive_avoidtoward;
	double sensitive_twirl;

	double default_spacing;
	EngraverLineQuality   default_linequality;
	EngraverTraceSettings default_trace;
	//EngraverDirection     default_direction;
	//EngraverSpacing       default_spacing;
	NormalDirectionMap *directionmap;

	Laxkit::RefPtrStack<TraceObject> traceobjects;

	 //for turbulence tool
	double turbulence_size; //this*spacing
	bool turbulence_per_line;
	Laxkit::OpenSimplexNoise noise;

	 //decorations to show..
	Laxkit::MenuInfo panel;
	flatpoint list_offset;
	bool show_group_list;

	int show_points;
	bool show_direction;
	bool show_panel;
	bool show_trace_object;
	bool show_object;
	bool grow_lines;
	bool always_warp;
	bool auto_reline;
	//Laxkit::CurveInfo tracemap;
	Laxkit::MenuItem *tracebox;
	Laxkit::DoubleBBox panelbox;
	Laxkit::IntRectangle sensbox;
	

	 //general display state
	Laxkit::ScreenColor fgcolor,bgcolor;
	unsigned long activate_color, deactivate_color;

	int lasthover;
	int lasthovercategory;
	int lasthoverindex, lasthoverdetail;
	flatpoint hover;
	flatpoint hoverdir, hdir[10];
	//Selection *selection;

	CurveMapInterface curvemapi;

	virtual int ActivatePathInterface();
	virtual void ChangeMessage(int forwhich);
	virtual int scanPanel(int x,int y, int *category, int *index_ret, int *detail_ret);
	virtual int scanEngraving(int x,int y,unsigned int state, int *category, int *index_ret, int *detail_ret);
	virtual int PerformAction(int action);

	virtual void DrawOrientation(int over);
	virtual void DrawPanel();
	virtual void DrawPanelHeader(int open, int hover,const char *name, int x,int y,int w, int hh);
	virtual void DrawLineGradient(double minx,double maxx,double miny,double maxy, int groupnum, int horizontal);
	virtual void DrawSlider(double pos,int hovered, double x,double y,double w,double h, const char *text);
	virtual void DrawCheckBox(int on,  int hovered, double x,double y,double w,double h, const char *text);
	virtual void DrawNumInput(double pos,int type,int hovered, double x,double y,double w,double h, const char *text);
	virtual void DrawShadeGradient(double minx,double maxx,double miny,double maxy);

	virtual int IsSharing(int what, EngraverPointGroup *group, int curgroup); 
	virtual void UpdatePanelAreas();
	virtual Laxkit::MenuInfo *GetGroupMenu(int what);
	virtual int NumGroupLines();
	virtual EngraverFillData *GroupFromLineIndex(int i, int *gi);
	virtual int CurrentLineIndex(int *maxindex);

	virtual int PushToAll(int what, EngraverPointGroup *from,int fromi);
	virtual int PushSettings(int what, EngraverPointGroup *from,int fromi, EngraverPointGroup *to,int toi);

	virtual void UpdateDashCaches(EngraverLineQuality *dash);

  public:
	EngraverFillInterface(int nid, Laxkit::Displayer *ndp);
	virtual ~EngraverFillInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual const char *IconId() { return "Engraver"; }
	virtual const char *Name();
	virtual const char *whattype() { return "EngraverFillInterface"; }
	virtual const char *whatdatatype() { return "EngraverFillData"; }
	virtual anInterface *duplicate(anInterface *dup);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int UseThis(anObject *newdata,unsigned int mask=0);
	virtual int UseThis(int id,int ndata);
	virtual int DrawData(anObject *ndata,anObject *a1=NULL,anObject *a2=NULL,int info=0);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int InterfaceOff();
	virtual int InitializeResources();

	virtual void deletedata(bool flush_selection);
	virtual PatchData *newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	//virtual void drawpatch(int roff,int coff);
	//virtual void patchpoint(PatchRenderContext *context, double s0,double ds,double t0,double dt,int n);
	virtual int ChangeMode(int newmode);
	virtual const char *ModeTip(int mode);
	virtual int Trace(bool do_once=false);
	virtual int Reline(bool do_once=true, int which=3);
	virtual int Grow(bool alldir, bool allindata);

	virtual int AddToSelection(ObjectContext *oc);
};

} //namespace LaxInterfaces

#endif


