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
//    Copyright (C) 2014 by Tom Lechner
//
#ifndef _LAX_ENGRAVERFILLINTERFACE_H
#define _LAX_ENGRAVERFILLINTERFACE_H

#include <lax/interfaces/patchinterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/screencolor.h>
#include <lax/curveinfo.h>

//#include <lax/interfaces/selection.h>

namespace LaxInterfaces {


//------------------------------------- EngraverFillData ------------------------

class EngraverFillData;

//--------------------------------------------- LinePoint
class LinePoint
{
  public:
	double s,t;
	int row,col;
	double weight;
	double spacing; //visual measure, to be used when remapping
	int group;
	bool on;

	int needtosync; //0 no, 1: s,t -> p, 2: p->s,t
	flatpoint p; //(s,t) transformed by the mesh

	LinePoint *next, *prev;

	LinePoint();
	LinePoint(double ss, double tt, double ww, int ngroup=0);
	~LinePoint();

	void Set(double ss,double tt, double nweight) { s=ss; t=tt; needtosync=1; if (nweight>=0) weight=nweight; }
	void Set(LinePoint *pp);
	void Clear();
	void Add(LinePoint *np);
	void AddBefore(LinePoint *np);
};

//---------------------------------------------- EngraverTraceSettings 
class EngraverTraceSettings : public Laxkit::anObject
{
  public:
	int group;
	Laxkit::CurveInfo value_to_weight;
	double traceobj_opacity;
	bool continuous_trace;

	ObjectContext *tracecontext;
	SomeData *traceobject;
	char *identifier;
	unsigned char *trace_sample_cache;
	int samplew, sampleh;

	 //black and white cache:
	int tw,th; //dims of trace_ref_bw
	unsigned char *trace_ref_bw;

	EngraverTraceSettings();
	virtual ~EngraverTraceSettings();
	virtual const char *whattype() { return "EngraverTraceSettings"; }
	void ClearCache(bool obj_too);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);
};

//----------------------------------------------- EngraverLineQuality

class EngraverLineQuality : public Laxkit::anObject
{
  public:
	double dash_length;
	double dash_randomness;
	double zero_threshhold;
	double broken_threshhold;
	double dash_taper; //0 means taper all the way, 1 means no taper
	int indashcaps, outdashcaps, startcaps, endcaps;
	//Laxkit::CurveInfo weighttodist;

	EngraverLineQuality();
	virtual ~EngraverLineQuality();
	virtual const char *whattype() { return "EngraverLineQuality"; }
};


//----------------------------------------------- EngraverPointGroup

class ValueMap
{
  public:
	ValueMap() {}
	virtual ~ValueMap() {}
	virtual double GetValue(double x,double y) = 0;
	virtual double GetValue(flatpoint p) { return GetValue(p.x,p.y); }
};

class DirectionMap
{
  public:
	DirectionMap() {}
	virtual ~DirectionMap() {}
	virtual flatpoint Direction(double x,double y) = 0;
	virtual flatpoint Direction(flatpoint p) { return Direction(p.x,p.y); }
};

class GrowPointInfo
{
  public:
	flatpoint p;
	flatpoint v;
	int godir;
	GrowPointInfo(flatpoint pp,flatpoint vv, int gdir) { p=pp; v=vv; godir=gdir; }
	GrowPointInfo(flatpoint pp, int gdir) { p=pp; godir=gdir; }
};

class EngraverPointGroup : public DirectionMap
{
  public:
	enum PointGroupType {
		PGROUP_Linear,
		PGROUP_Radial,
		PGROUP_Spiral,
		PGROUP_Circular,
		PGROUP_MAX
	};

	int id; //the group number in LinePoint
	char *name;
	EngraverTraceSettings *trace;

	int type; //what manner of lines
	double type_d;   //parameter for type, for instance, an angle for spirals
	double spacing;  //default
	flatpoint position,direction; //default

	EngraverLineQuality *dashes;

	EngraverPointGroup();
	EngraverPointGroup(int nid,const char *nname, int ntype, flatpoint npos, flatpoint ndir, double ntype_d, EngraverTraceSettings *newtrace);
	virtual ~EngraverPointGroup();

	virtual void SetTraceSettings(EngraverTraceSettings *newtrace);

	virtual flatpoint Direction(double s,double t);
	virtual LinePoint *LineFrom(double s,double t);

	virtual void Fill(EngraverFillData *data, double nweight); //fill in x,y = 0..1,0..1
	virtual void FillRegularLines(EngraverFillData *data, double nweight);
	virtual void FillRadial(EngraverFillData *data, double nweight);
	virtual void FillCircular(EngraverFillData *data, double nweight);

	virtual void GrowLines(EngraverFillData *data,
									double resolution, 
									double defaultspace,  	ValueMap *spacingmap,
									double defaultweight,   ValueMap *weightmap, 
									flatpoint direction,    DirectionMap *directionmap,
									Laxkit::PtrStack<GrowPointInfo> *growpoint_ret,
									int iteration_limit);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


//---------------------------------------- EngraverFillData

class EngraverFillData : public PatchData
{
 protected:
  	
 public:
	Laxkit::PtrStack<LinePoint> lines;
	flatvector direction;
	FillStyle fillstyle;

	EngraverPointGroup defaultgroup;
	Laxkit::PtrStack<EngraverPointGroup> groups;


	EngraverFillData();
	//EngraverFillData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual ~EngraverFillData(); 
	virtual const char *whattype() { return "EngraverFillData"; }
	virtual SomeData *duplicate(SomeData *dup);
	virtual double DefaultSpacing(double nspacing);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void dump_out_svg(const char *file);
	virtual PathsData *MakePathsData();

	virtual void Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	//virtual unsigned long WhatColorLong(double s,double t);
	//virtual int WhatColor(double s,double t,Laxkit::ScreenColor *color_ret);
	//virtual int hasColorData();

	//virtual void zap();

	virtual int PointOn(LinePoint *p);
	virtual void FillRegularLines(double weight, double spacing);
	virtual void Sync(bool asneeded);
	virtual void ReverseSync(bool asneeded);
	virtual void BezApproximate(Laxkit::NumStack<flatvector> &fauxpoints, Laxkit::NumStack<flatvector> &points);
	virtual void MorePoints();
};




//------------------------------ EngraverFillInterface -------------------------------

class EngraverFillInterface : public PatchInterface
{
  protected:
	Laxkit::MenuInfo modes;
	EngraverFillData *edata;
	int mode;
	int controlmode;
	int submode;
	int show_points;
	int current_group;

	 //general tool settings
	double brush_radius; //screen pixels
	Laxkit::CurveInfo thickness; //ramp of thickness brush

	double default_spacing;
	EngraverLineQuality default_linequality;

	 //for turbulence tool
	double turbulence_size; //this*spacing
	bool turbulence_per_line;

	 //trace settings..
	bool show_trace;
	bool continuous_trace;
	bool grow_lines;
	bool always_warp;
	//Laxkit::CurveInfo tracemap;
	Laxkit::DoubleBBox tracebox;
	EngraverTraceSettings trace;

	 //grow related
	Laxkit::PtrStack<GrowPointInfo> growpoints;
	

	 //general display state
	Laxkit::ScreenColor fgcolor,bgcolor;

	int lasthover;
	flatpoint hover;
	flatpoint hoverdir, hdir[10];
	//Selection *selection;

	CurveMapInterface curvemapi;


	virtual void ChangeMessage(int forwhich);
	virtual int scanEngraving(int x,int y, int *category);
	virtual int PerformAction(int action);

	virtual void DrawOrientation(int over);
	virtual void DrawTracingTools();
	virtual void DrawLineGradient(double minx,double maxx,double miny,double maxy);
	virtual void DrawShadeGradient(double minx,double maxx,double miny,double maxy);

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
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);

	virtual void deletedata();
	virtual PatchData *newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	//virtual void drawpatch(int roff,int coff);
	//virtual void patchpoint(PatchRenderContext *context, double s0,double ds,double t0,double dt,int n);
	virtual int ChangeMode(int newmode);
	virtual int Trace();
};

} //namespace LaxInterfaces

#endif


