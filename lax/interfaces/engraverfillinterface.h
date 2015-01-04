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


namespace LaxInterfaces {


//------------------------------------- EngraverFillData ------------------------

class EngraverFillData;


//--------------------------------------------- LinePoint

#define  MAX_LINEPOINT_CACHE 10

enum EngraveLinePointCacheTypes {
	ENGRAVE_Off=0,
	ENGRAVE_On=1,
	ENGRAVE_EndPoint=-1,
	ENGRAVE_StartPoint=-2,

	ENGRAVE_Original=100,//an actual sample point, these persist
	ENGRAVE_Sample,     
	ENGRAVE_BlockStart,      //added by blockout
	ENGRAVE_BlockEnd,     //added by blockout
	ENGRAVE_VisualCache, //extra point for on screen display purposes
	ENGRAVE_EndDash,   //added by dash computations
	ENGRAVE_StartDash  //added by dash computations
};

class LinePoint;

class LinePointCache
{
  public:
	int type; //position, end border, start border, original

	flatpoint p;
	double weight;
	int on; //off if zero, 1 on, -1 end point, -2 start point
	int dashon;
	double bt; //bez t coord between LinePoints of this one
	LinePoint *original;

	LinePointCache *prev,*next; //must NOT be part of a loop

	LinePointCache(int ntype);
	LinePointCache(LinePointCache *prev);
	~LinePointCache();
	void Add(LinePointCache *np);
	void AddBefore(LinePointCache *np);
	LinePointCache *InsertAfter(LinePointCache *np);
	LinePointCache *Detach();

	LinePoint *PrevOriginal();
};


class LinePoint
{
  public:
	int type; //position, end border, start border
	double bt; //bez t coord between LinePoints of this one

	double weight;
	double weight_orig;
	int on; //off if zero, 1 on, -1 end point, -2 start point

	flatpoint p; //(s,t) transformed by the mesh 
	flatpoint bez_before, bez_after;
	double length;

	int needtosync; //0 no, 1: s,t -> p, 2: p->s,t
	double s,t;
	int row,col;
	double spacing; //visual measure, to be used when remapping

	LinePointCache *cache;
	LinePoint *next, *prev;

	LinePoint();
	LinePoint(double ss, double tt, double ww);
	~LinePoint();

	void Set(double ss,double tt, double nweight) { s=ss; t=tt; needtosync=1; if (nweight>=0) weight=nweight; }
	void Set(LinePoint *pp);
	void Clear();
	void Add(LinePoint *np);
	void AddBefore(LinePoint *np);
	void BaselineCache();

	void UpdateBezHandles();
	//void ReCache(int num, double dashleftover, EngraverLineQuality *dashes);
};


//--------------------------- ValueMap -----------------------------
class ValueMap
{
  public:
	ValueMap() {}
	virtual ~ValueMap() {}
	virtual double GetValue(double x,double y) = 0;
	virtual double GetValue(flatpoint p) { return GetValue(p.x,p.y); }
};


//---------------------------------------------- EngraverTraceSettings 

class TraceObject : public Laxkit::anObject
{
  public:
	enum TraceObjectType {
		TRACE_None,
		TRACE_Current,
		TRACE_ImageFile,
		TRACE_Object,
		TRACE_LinearGradient,
		TRACE_RadialGradient
	};
	TraceObjectType type;

	LaxInterfaces::SomeData *object; //transform is to maximum parent of owning object
	char *image_file;

	int samplew, sampleh;
	unsigned char *trace_sample_cache;
	std::time_t cachetime;

	 //black and white cache:
	int tw,th; //dims of trace_ref_bw
	unsigned char *trace_ref_bw;

	TraceObject();
	virtual ~TraceObject();
	double GetValue(LinePoint *p, double *transform);
	void ClearCache(bool obj_too);
	int UpdateCache(ViewportWindow *viewport);
	int NeedsUpdating();

	void Install(TraceObjectType ntype, SomeData *obj);
};

class EngraverTraceSettings : public Laxkit::anObject
{
  public:
	int group;
	Laxkit::CurveInfo value_to_weight;
	double traceobj_opacity;
	int tracetype; //0==absolute, 1=multiply
	bool continuous_trace;

	char *identifier;
	TraceObject *traceobject;
	EngraverTraceSettings *next;

	EngraverTraceSettings();
	virtual ~EngraverTraceSettings();
	virtual const char *whattype() { return "EngraverTraceSettings"; }
	void ClearCache(bool obj_too);
	virtual EngraverTraceSettings *duplicate();
	void Install(TraceObject::TraceObjectType ntype, SomeData *obj);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);
};

//----------------------------------------------- EngraverLineQuality

class EngraverLineQuality : public Laxkit::anObject
{
  public:
	double dash_length;
	double dash_density;
	double dash_randomness;
	int randomseed;
	double zero_threshhold;
	double broken_threshhold;
	double dash_taper; //0 means taper all the way, 1 means no taper

	int indashcaps, outdashcaps, startcaps, endcaps; //0 for butt, 1 for round
	//Laxkit::CurveInfo weighttodist;

	EngraverLineQuality();
	virtual ~EngraverLineQuality();
	virtual const char *whattype() { return "EngraverLineQuality"; }
	virtual EngraverLineQuality *duplicate();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);

	int GetNewWeight(double weight, double *weight_ret);
};


//--------------------------- DirectionMap -----------------------------
class DirectionMap
{
  public:
	DirectionMap() {}
	virtual ~DirectionMap() {}
	virtual flatpoint Direction(double x,double y) = 0;
	virtual flatpoint Direction(flatpoint p) { return Direction(p.x,p.y); }
};

//--------------------------- NormalDirectionMap -----------------------------
class NormalDirectionMap : public DirectionMap
{   
  public:
    Laxkit::Affine m; //transform input points to image space
    Laxkit::LaxImage *normal_map;
    bool has_transparency; //if byte 3 is transparency
    bool has_origin_value; //if byte 2 is a grayscale version of source
    
    unsigned char *data; //for 4*w*h
    int width, height;
    Laxkit::DoubleBBox limits;
    

    NormalDirectionMap();
    NormalDirectionMap(const char *file);
    virtual ~NormalDirectionMap();
    virtual flatpoint Direction(double x,double y);
	virtual int Load(const char *file);
	virtual void Clear();
};  
    

//--------------------------- EngraverDirection -----------------------------
class EngraverDirection : public Laxkit::anObject
{
  public:
	int type; //what manner of lines: linear, radial, circular
	double type_d;   //parameter for type, for instance, an angle for spirals
	double spacing;  //default
	double resolution; //samples per spacing unit, default is 1
	flatpoint position,direction; //default

	DirectionMap *map;

	EngraverDirection();
	virtual ~EngraverDirection();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);
};


//--------------------------- EngraverSpacing -----------------------------
class EngraverSpacing : public Laxkit::anObject
{
  public:
	int type; //how to get spacing: use default, use grabbed current map, use custom map
	double spacing;  //default

	ValueMap *map;

	EngraverSpacing();
	virtual ~EngraverSpacing();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,Laxkit::anObject *savecontext);
};


//--------------------------- GrowPointInfo -----------------------------
class GrowPointInfo
{
  public:
	flatpoint p;
	flatpoint v;
	int godir;
	GrowPointInfo(flatpoint pp,flatpoint vv, int gdir) { p=pp; v=vv; godir=gdir; }
	GrowPointInfo(flatpoint pp, int gdir) { p=pp; godir=gdir; }
};

//----------------------------------------------- EngraverPointGroup


class EngraverPointGroup : public DirectionMap
{
  protected:
	void EstablishDashMetrics(double s,double weight, double *next,int *nexton,int &nextmax,  LinePoint *l,LinePointCache *&lc,
							 int &laston, double &lasts, double &dashweight, double &dashlen, Laxkit::PtrStack<LinePointCache> &unused);
	LinePointCache *AddPoint(double s, int on, int type, double dashweight,double dashlen,
						LinePoint *l,LinePointCache *&lc, Laxkit::PtrStack<LinePointCache> &unused);
	int ApplyBlockout(LinePoint *l);

  public:
	enum PointGroupType {
		PGROUP_Linear,
		PGROUP_Radial,
		PGROUP_Spiral,
		PGROUP_Circular,
		PGROUP_Custom,
		PGROUP_MAX
	};

	int id;
	char *name;
	bool active;
	bool linked; //if distortion tool should adjust points even when not current group
	int type; //what manner of lines
	double type_d;   //parameter for type, for instance, an angle for spirals
	double spacing;  //default
	flatpoint position,direction; //default
	Laxkit::ScreenColor color;

	EngraverTraceSettings *trace; 
	EngraverLineQuality *dashes;
	int numdashes;

	char *iorefs; //tags of unresolved references to dashes, traces, etc

	Laxkit::PtrStack<LinePoint> lines;

	EngraverPointGroup();
	EngraverPointGroup(int nid,const char *nname, int ntype, flatpoint npos, flatpoint ndir, double ntype_d, EngraverTraceSettings *newtrace);
	virtual ~EngraverPointGroup();
	virtual void CopyFrom(EngraverPointGroup *orig, bool keep_name, bool link_trace, bool link_dash);

	virtual void InstallTraceSettings(EngraverTraceSettings *newtrace, int absorbcount);
	virtual void InstallDashes(EngraverLineQuality *newdash, int absorbcount);

	virtual int PointOn(LinePoint *p);
	virtual int PointOnDash (LinePointCache *p);
	virtual int CachePointOn(LinePointCache *p);
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

	virtual void UpdateBezCache();
	virtual void UpdatePositionCache();
	virtual int UpdateDashCache();
	virtual void StripDashes();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context,const char *sharetrace, const char *sharedash);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};


//---------------------------------------- EngraverFillData

class EngraverFillData : public PatchData
{
 protected:
  	
 public:
	Laxkit::PtrStack<EngraverPointGroup> groups; 

	EngraverFillData();
	//EngraverFillData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual ~EngraverFillData(); 
	virtual const char *whattype() { return "EngraverFillData"; }
	virtual SomeData *duplicate(SomeData *dup);
	virtual double DefaultSpacing(double nspacing);
	virtual void MakeDefaultGroup();
	virtual int MakeGroupNameUnique(int which);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	virtual void dump_out_svg(const char *file);
	virtual PathsData *MakePathsData(int whichgroup);

	virtual void Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	//virtual unsigned long WhatColorLong(double s,double t);
	//virtual int WhatColor(double s,double t,Laxkit::ScreenColor *color_ret);
	//virtual int hasColorData();

	//virtual void zap();

	virtual int PointOn(LinePoint *p,EngraverPointGroup *group);
	virtual void FillRegularLines(double weight, double spacing);
	virtual void Sync(bool asneeded);
	virtual void ReverseSync(bool asneeded);
	virtual void UpdatePositionCache();
	virtual void BezApproximate(Laxkit::NumStack<flatvector> &fauxpoints, Laxkit::NumStack<flatvector> &points);
	virtual void MorePoints(int curgroup);
	virtual EngraverPointGroup *FindGroup(int id, int *err_ret=NULL);
	virtual EngraverPointGroup *GroupFromIndex(int index, int *err_ret=NULL);

	virtual int IsSharing(int what, int curgroup); 
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
	int current_group;

	 //general tool settings
	double brush_radius; //screen pixels
	Laxkit::CurveInfo thickness; //ramp of thickness brush

	double default_spacing;
	EngraverLineQuality   default_linequality;
	EngraverTraceSettings default_trace;
	NormalDirectionMap *directionmap;

	Laxkit::RefPtrStack<TraceObject> traceobjects;

	 //for turbulence tool
	double turbulence_size; //this*spacing
	bool turbulence_per_line;

	 //decorations to show..
	Laxkit::MenuInfo panel;

	int show_points;
	bool show_direction;
	bool show_panel;
	bool show_trace;
	bool continuous_trace;
	bool grow_lines;
	bool always_warp;
	//Laxkit::CurveInfo tracemap;
	Laxkit::MenuItem *tracebox;
	Laxkit::DoubleBBox panelbox;

	 //grow related
	Laxkit::PtrStack<GrowPointInfo> growpoints;
	

	 //general display state
	Laxkit::ScreenColor fgcolor,bgcolor;

	int lasthover;
	int lasthovercategory;
	flatpoint hover;
	flatpoint hoverdir, hdir[10];
	//Selection *selection;

	CurveMapInterface curvemapi;


	virtual void ChangeMessage(int forwhich);
	virtual int scanPanel(int x,int y, int *category);
	virtual int scanEngraving(int x,int y, int *category);
	virtual int PerformAction(int action);

	virtual void DrawOrientation(int over);
	virtual void DrawPanel();
	virtual void DrawPanelHeader(int open, int hover,const char *name, int x,int y,int w, int hh);
	virtual void DrawTracingTools(Laxkit::MenuItem *item);
	virtual void DrawLineGradient(double minx,double maxx,double miny,double maxy, int groupnum, int horizontal);
	virtual void DrawSlider(double pos,int hovered, double x,double y,double w,double h, const char *text);
	virtual void DrawNumInput(double pos,int type,int hovered, double x,double y,double w,double h, const char *text);
	virtual void DrawShadeGradient(double minx,double maxx,double miny,double maxy);

	virtual int IsSharing(int what, int curgroup); 
	virtual void UpdatePanelAreas();
	virtual Laxkit::MenuInfo *GetGroupMenu(int what, int current);

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
	virtual const char *ModeTip(int mode);
	virtual int Trace();

	virtual int AddToSelection(ObjectContext *oc);
};

} //namespace LaxInterfaces

#endif


