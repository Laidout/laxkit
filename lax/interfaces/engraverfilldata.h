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
//    Copyright (C) 2014,2015 by Tom Lechner
//
#ifndef _LAX_ENGRAVERFILLDATA_H
#define _LAX_ENGRAVERFILLDATA_H

#include <lax/interfaces/patchinterface.h>
#include <lax/interfaces/lineprofile.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/imageinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/colors.h>
#include <lax/screencolor.h>
#include <lax/curveinfo.h>
#include <lax/noise.h>

// *** for PointCollection callback:
#include <functional>


namespace LaxInterfaces {


//------------------------------------- EngraverFillData helper stuff ------------------------

class EngraverFillData;

//NOTE!! these values must be coordinated with LaxInterfaceDataTypes, and 
//any other Laxkit object types meant to be used with an ObjectFactory.
enum EngraverObjectTypes {
	ENGTYPE_TraceObject = LAX_DATA_MAX,
    ENGTYPE_EngraverLineQuality,
    ENGTYPE_EngraverTraceSettings,
    ENGTYPE_EngraverTraceStack,
    ENGTYPE_NormalDirectionMap,
    ENGTYPE_EngraverDirection,
    ENGTYPE_EngraverSpacing,
    ENGTYPE_EngraverFillStyle,

	ENGTYPE_MAX
};

void InstallEngraverObjectTypes(Laxkit::ObjectFactory *factory);

const char *PGroupTypeName(int type);
const char *PGroupTypeNameUntrans(int type);
int PGroupTypeId(const char *value);


//------------------------------------- Engraver control numbers ------------------------
enum EngraveControls {
	ENGRAVE_None=0,

	 //------------ panel ids...
	ENGRAVE_Panel           = PATCHA_MAX,
	ENGRAVE_Mode_Selection,
	ENGRAVE_Groups,

	 //--------------- point group
	ENGRAVE_Toggle_Group_List,
	ENGRAVE_Previous_Group,
	ENGRAVE_Next_Group,
	ENGRAVE_Group_Name,
	ENGRAVE_Group_List,
	ENGRAVE_Group_Linked,
	ENGRAVE_Group_Active,
	ENGRAVE_Group_Color,
	ENGRAVE_Delete_Group,
	ENGRAVE_New_Group,
	ENGRAVE_Merge_Group,
	ENGRAVE_Dup_Group, // <- is same as new?
	ENGRAVE_Group_Up,
	ENGRAVE_Group_Down,

	 //--------------- tracing  
	ENGRAVE_Tracing,
	ENGRAVE_Trace_Box,
	ENGRAVE_Trace_Name,
	ENGRAVE_Trace_Menu,
	ENGRAVE_Trace_Once,
	ENGRAVE_Trace_Load,
	ENGRAVE_Trace_Clear,
	ENGRAVE_Trace_Snapshot,
	ENGRAVE_Trace_Current,
	ENGRAVE_Trace_Linear_Gradient,
	ENGRAVE_Trace_Radial_Gradient,
	ENGRAVE_Trace_Save,           
	ENGRAVE_Trace_Continuous,
	ENGRAVE_Trace_Object,
	ENGRAVE_Trace_Object_Name,
	ENGRAVE_Trace_Object_Menu,
	ENGRAVE_Trace_Opacity,
	ENGRAVE_Trace_Curve,
	ENGRAVE_Trace_Curve_Line_Bar,
	ENGRAVE_Trace_Curve_Value_Bar,
	ENGRAVE_Trace_Move_Mesh,
	ENGRAVE_Trace_Same_As,
	ENGRAVE_Trace_Thicken,
	ENGRAVE_Trace_Thin,
	ENGRAVE_Trace_Set,
	ENGRAVE_Trace_Using_type,
	ENGRAVE_Trace_Using,
	ENGRAVE_Trace_Apply,
	ENGRAVE_Trace_Remove,

	 //--------------- Dashes  
	ENGRAVE_Dashes,
	ENGRAVE_Dash_Same_As,
	ENGRAVE_Dash_Menu,
	ENGRAVE_Dash_Name,
	ENGRAVE_Dash_Stipple,
	ENGRAVE_Dash_Dashes,
	ENGRAVE_Dash_Zero_Threshhold,
	ENGRAVE_Dash_Broken_Threshhold,
	ENGRAVE_Dash_Length,
	ENGRAVE_Dash_Seed,
	ENGRAVE_Dash_Random,
	ENGRAVE_Dash_Taper,
	ENGRAVE_Dash_Density,
	ENGRAVE_Dash_Caps,
	ENGRAVE_Dash_Join,

	 //--------------- Direction  
	ENGRAVE_Direction,
	ENGRAVE_Direction_Same_As,
	ENGRAVE_Direction_Menu,
	ENGRAVE_Direction_Name,
	ENGRAVE_Direction_Type,
	ENGRAVE_Direction_Reline,
	ENGRAVE_Direction_Show_Dir,
	ENGRAVE_Direction_Profile,
	ENGRAVE_Direction_Profile_Menu,
	ENGRAVE_Direction_Profile_Start,
	ENGRAVE_Direction_Profile_Start_Random,
	ENGRAVE_Direction_Profile_End,
	ENGRAVE_Direction_Profile_End_Random,
	ENGRAVE_Direction_Profile_Scale,
	ENGRAVE_Direction_Profile_Max_Height,
	ENGRAVE_Direction_Line_Offset,
	ENGRAVE_Direction_Point_Offset,
	ENGRAVE_Direction_Point_Off_Size,
	ENGRAVE_Direction_Grow,
	ENGRAVE_Direction_Fill,
	ENGRAVE_Direction_Merge,
	ENGRAVE_Direction_Spread,
	ENGRAVE_Direction_Spread_Depth,
	ENGRAVE_Direction_Spread_Angle,
	ENGRAVE_Direction_Seed,
	
	ENGRAVE_Direction_Current,
	ENGRAVE_Direction_Paint,
	ENGRAVE_Direction_Create_From_Cur,
	ENGRAVE_Direction_From_Trace,
	ENGRAVE_Direction_Load_Normal,
	ENGRAVE_Direction_Load_Image,

	 //--------------- Spacing  
	ENGRAVE_Spacing,
	ENGRAVE_Spacing_Same_As,
	ENGRAVE_Spacing_Menu,
	ENGRAVE_Spacing_Name,
	ENGRAVE_Spacing_Default,
	ENGRAVE_Spacing_Use_Map,
	ENGRAVE_Spacing_Map_File,
	ENGRAVE_Spacing_Map,
	ENGRAVE_Spacing_Map_Menu,
	ENGRAVE_Spacing_Preview,
	ENGRAVE_Spacing_Create_From_Current,
	ENGRAVE_Spacing_Load,
	ENGRAVE_Spacing_Save,
	ENGRAVE_Spacing_Paint,

	ENGRAVE_Panel_MAX,

	 //--------misc menu items
	ENGRAVE_Make_Local,
	ENGRAVE_Make_Shared,

	 //------------on canvas tool controls:
	ENGRAVE_Orient,
	ENGRAVE_Orient_Spacing,
	ENGRAVE_Orient_Position,
	ENGRAVE_Orient_Direction,
	ENGRAVE_Orient_Type,
	ENGRAVE_Orient_Quick_Adjust,
	ENGRAVE_Orient_Keep_Old,
	ENGRAVE_Orient_Grow,
	ENGRAVE_Orient_Parameter,

	ENGRAVE_Sensitivity,

	 //------modes:
	EMODE_Render_Only,
	EMODE_Controls,
	EMODE_Mesh, //dev note: EMODE_Mesh MUST be first in mode list for proper mouse over stuff
	EMODE_Thickness,
	EMODE_Orientation,
	EMODE_Freehand,
	EMODE_Blockout,
	EMODE_Drag, 
	EMODE_PushPull,
	EMODE_AvoidToward,
	EMODE_Twirl,
	EMODE_Turbulence,
	EMODE_Trace,
	EMODE_Direction,
	EMODE_Resolution, //change sample point distribution
	EMODE_MAX

};



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

	Laxkit::flatpoint p;
	double weight;
	int on; //see EngraveLinePointCacheTypes
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
	int on; //see EngraveLinePointCacheTypes

	Laxkit::flatpoint p; //(s,t) transformed by the mesh 
	Laxkit::flatpoint bez_before, bez_after;
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

class EngraverLine
{
  public:
	int startcap;
	int endcap;
	double startspread;
	double endspread;
	double startangle; //always relative to default angle
	double endangle;
	Laxkit::Color *color;
	LinePoint *line;

	EngraverLine();
	virtual ~EngraverLine();
};


//--------------------------- ValueMap -----------------------------
class ValueMap : public Laxkit::Resourceable
{
  public:
	ValueMap() {}
	virtual ~ValueMap() {}
	virtual double GetValue(double x,double y) = 0;
	virtual double GetValue(Laxkit::flatpoint p) { return GetValue(p.x,p.y); }
};


//---------------------------------------------- EngraverTraceSettings 

class TraceObject : public Laxkit::Resourceable, public Laxkit::DumpUtility
{
  public:
	enum TraceObjectType {
		TRACE_None,
		TRACE_Current,
		TRACE_Snapshot,
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
	virtual const char *whattype() { return "TraceObject"; }
	virtual Laxkit::anObject *duplicate();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	
	double GetValue(LinePoint *p, double *transform);
	void ClearCache(bool obj_too);
	int UpdateCache();
	int NeedsUpdating();

	void Install(TraceObjectType ntype, SomeData *obj);
};

class EngraverTraceSettings : public Laxkit::Resourceable, public Laxkit::DumpUtility
{
  public:
    enum TraceType {
	  TRACE_Set,
	  TRACE_Multiply,
	  TRACE_Add,
	  TRACE_Subtract,
	  TRACE_MAX
	} tracetype;

	int group;
	Laxkit::CurveInfo *value_to_weight;
	double traceobj_opacity;
	bool continuous_trace;
	bool show_trace;

	TraceObject *traceobject;
	bool lock_ref_to_obj;
	EngraverTraceSettings *next;

	EngraverTraceSettings();
	virtual ~EngraverTraceSettings();
	virtual const char *Id();
	virtual const char *Id(const char *str);
	virtual const char *whattype() { return "EngraverTraceSettings"; }
	virtual Laxkit::anObject *duplicate();
	void ClearCache(bool obj_too);
	void Install(TraceObject::TraceObjectType ntype, SomeData *obj);
	void Install(TraceObject *nobject);
	virtual const char *Identifier();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
};

class EngraverTraceStack : public Laxkit::Resourceable, public Laxkit::DumpUtility
{
  public:
    EngraverTraceStack();
    virtual ~EngraverTraceStack();

	struct TraceNode
	{
		EngraverTraceSettings *settings;
		double amount;
		bool visible;
		TraceNode(EngraverTraceSettings *nsettings, double namount, bool nvis);
		~TraceNode();
	};

	Laxkit::PtrStack<TraceNode> nodes;

	virtual int Num() { return nodes.n; }
	virtual TraceNode *Settings(int which);
	virtual double Amount(int which);
	virtual double Amount(int which, double newamount);
	virtual bool Visible(int which);
	virtual bool Visible(int which, bool newvisible);
	virtual int PushSettings(EngraverTraceSettings *settings, double amount, bool nvisible, int where);
	virtual int Move(int which, int towhere); //stack slide
	virtual int Remove(int which);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
};


//----------------------------------------------- EngraverLineQuality

class EngraverLineQuality : public Laxkit::Resourceable, public Laxkit::DumpUtility
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
	virtual Laxkit::anObject *duplicate();
	virtual const char *Id();
	virtual const char *Id(const char *id);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);

	int GetNewWeight(double weight, double *weight_ret);
};


//--------------------------- DirectionMap -----------------------------
class DirectionMap : public Laxkit::Resourceable
{
  public:
	DirectionMap() {}
	virtual ~DirectionMap() {}
	virtual Laxkit::flatpoint Direction(double x,double y) = 0;
	virtual Laxkit::flatpoint Direction(Laxkit::flatpoint p) { return Direction(p.x,p.y); }
};


//--------------------------- NormalDirectionMap -----------------------------
class NormalDirectionMap : public DirectionMap
{   
  public:
    Laxkit::Affine m; //transform input points to image space
    Laxkit::LaxImage *normal_map;
    bool has_transparency; //if byte 3 is transparency
    bool has_origin_value; //if byte 2 is a grayscale version of source
	double angle; //angle to add to default normals
    
    unsigned char *data; //for 4*w*h
    int width, height;
    Laxkit::DoubleBBox limits;
    

    NormalDirectionMap();
    NormalDirectionMap(const char *file);
    virtual ~NormalDirectionMap();
    virtual Laxkit::flatpoint Direction(double x,double y);
	virtual int Load(const char *file);
	virtual void Clear();
};  
    

//--------------------------- GrowPointInfo -----------------------------
class GrowPointInfo
{
  public:
	Laxkit::flatpoint p;
	Laxkit::flatpoint v;
	int godir;
	GrowPointInfo(Laxkit::flatpoint pp,Laxkit::flatpoint vv, int gdir) { p=pp; v=vv; godir=gdir; }
	GrowPointInfo(Laxkit::flatpoint pp, int gdir) { p=pp; godir=gdir; }
};


//------------------------- class StarterPoint, used for growing lines --------------------
class StarterPoint
{
  public:
	int iteration;
	int piteration;

	Laxkit::flatpoint p;
	Laxkit::flatpoint dir;
	int dodir; //1 for add to +direction, 2 for add to -direction, 3 for both

	int lineref;
	LinePoint *line;
	LinePoint *first, *last; //is part of line, will be either the most next or most prev

	StarterPoint (Laxkit::flatpoint p, int indir, double weight,int groupid, int nlineref);
};


//------------------------ GrowContext --------------------------
class GrowContext
{
  public:
  	struct ScratchData
  	{
  		int group;
  		int line;
  		int point;
  		Laxkit::flatpoint dir;
  	};

  	bool active = true;
  	ScratchData *scratch_data = nullptr;

  	GrowContext();
  	~GrowContext();

  	Laxkit::PtrStack<StarterPoint> generators;

  	//EngraverDirection *direction;
  	//EngraverSpacing *spacing;
  	//TraceObject *tracing;

  	char *error = nullptr;

  	int iteration = 0;
	int iteration_limit;

  	EngraverFillData *data = nullptr;
	double resolution = -1; 

	double defaultspace = 0;
	ValueMap *spacingmap = nullptr;

	double defaultweight = 0;
	ValueMap *weightmap = nullptr;

	Laxkit::flatpoint directionv;
	DirectionMap *directionmap = nullptr;
};


//--------------------------- EngraverDirection -----------------------------
enum PointGroupType {
	PGROUP_Unknown=0,
	PGROUP_Linear,
	PGROUP_Radial,
	PGROUP_Circular,
	PGROUP_Spiral,
	PGROUP_Shell,
	PGROUP_S,
	PGROUP_Contour,
	PGROUP_Map,
	PGROUP_Manual,
	PGROUP_Function,
	//PGROUP_Voronoi,
	//PGROUP_Maze,
	PGROUP_MAX
};

class EngraverDirection : public Laxkit::Resourceable, public Laxkit::DumpUtility
{
  public:
	class Parameter
	{
	  public:
		char *name; //scripting name
		char *Name; //human name
		char type; //initial of: boolean, int, real
		int dtype; //which direction->type this corresponds to
		double min, max;
		int min_type, max_type; //0=not active, 1=fixed
		double mingap; //min size the slider shows
		double value;
		Parameter();
		Parameter(const char *nname, const char *nName, int ndtype, char ntype, double nmin,
				int nmint, double nmax,int nmaxt, double nmingap, double nvalue);
		~Parameter();
	};

	int type; //what manner of lines: linear, radial, circular. See PointGroupType
	DirectionMap *map;

	double spacing;  //default
	double resolution; //default samples per spacing unit, default is 1
	double default_weight; //a fraction of spacing 
	Laxkit::flatpoint position, direction; //default
	Laxkit::PtrStack<Parameter> parameters; //extras beyond position, spacing, rotation

	 //line generation tinkering settings
	int seed; //for any randomness
	double line_offset; //0..1 for random offset per line
	double point_offset;
	double noise_scale; //applied per sample point, but offset per random line, not random at each point

	LineProfile *default_profile;
	int start_type, end_type; //0=normal, 1=random
	double start_rand_width, end_rand_width; //zone around start and end to randomize, fraction of total
	double profile_start, profile_end; // [0..1]
	double max_height;
	bool scale_profile;

	bool grow_lines;
	bool fill;
	bool merge;
	double spread;
	double spread_depth;
	double merge_angle;


	EngraverDirection();
	virtual ~EngraverDirection();
	virtual const char *whattype() { return "EngraverDirection"; }
	virtual Laxkit::anObject *duplicate();
	virtual const char *Id();
	virtual const char *Id(const char *id);

	virtual const char *TypeName();
	virtual int SetType(int newtype);
	virtual Parameter *FindParameter(const char *name);
	virtual int ValidateParameter(EngraverDirection::Parameter *p);
	virtual Parameter *GetParameter(int index);
	virtual int NumParameters();

	virtual int GetStartEnd(double *start_ret, double *end_ret, bool keep_increasing);
	virtual int InstallProfile(LineProfile *profile, int absorbcount);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
};


//--------------------------- EngraverSpacing -----------------------------
class EngraverSpacing : public Laxkit::Resourceable, public Laxkit::DumpUtility
{
  public:
	int type; //how to get spacing: use default, use grabbed current map, use custom map
	double spacing;  //default

	ValueMap *map;
	bool map_multiply; //multiply with default spacing

	EngraverSpacing();
	virtual ~EngraverSpacing();
	virtual const char *whattype() { return "EngraverSpacing"; }
	virtual Laxkit::anObject *duplicate();
	virtual const char *Id();
	virtual const char *Id(const char *id);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
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
	EngraverFillData *owner;
	virtual Laxkit::anObject *ObjectOwner();

	int id;
	char *name;
	bool active;
	bool linked; //if distortion tool should adjust points even when not current group
	Laxkit::ScreenColor color;

	bool needtotrace;
	bool needtoreline;
	bool needtodash;

	double default_weight; //a fraction of spacing
	Laxkit::flatpoint position,directionv; //note NOT the same properties as in EngravingDirection, this overrides those

	Laxkit::PtrStack<GrowPointInfo> growpoints;
	GrowContext *grow_cache = nullptr;

	EngraverTraceSettings *trace; 
	EngraverLineQuality *dashes;
	EngraverDirection *direction;
	EngraverSpacing *spacing;

	char *iorefs; //tags of unresolved references to dashes, traces, etc

	Laxkit::PtrStack<LinePoint> lines;

	EngraverPointGroup(EngraverFillData *nowner);
	EngraverPointGroup(EngraverFillData *nowner,int nid,const char *nname, int ntype, Laxkit::flatpoint npos, Laxkit::flatpoint ndir,
						EngraverTraceSettings *newtrace,
						EngraverLineQuality   *newdash,
						EngraverDirection     *newdir,
						EngraverSpacing       *newspacing);
	virtual ~EngraverPointGroup();
	virtual void CopyFrom(EngraverPointGroup *orig, bool keep_name, bool link_trace, bool link_dash, bool link_dir, bool link_spacing);
	virtual void Modified(int what);

	virtual void InstallTraceGradient(char type, GradientData *ngradient, int absorbcount);
		
	virtual void InstallTraceSettings(EngraverTraceSettings *newtrace, int absorbcount);
	virtual void InstallDashes(EngraverLineQuality *newdash, int absorbcount);
	virtual void InstallDirection(EngraverDirection *newdir, int absorbcount);
	virtual void InstallSpacing(EngraverSpacing *newspace, int absorbcount);

	virtual int PointOn(LinePoint *p);
	virtual int PointOnDash (LinePointCache *p);
	virtual int CachePointOn(LinePointCache *p);
	virtual Laxkit::flatpoint Direction(double s,double t);
	virtual LinePoint *LineFrom(double s,double t);

	virtual int Trace(Laxkit::Affine *aa);
	virtual void Fill(EngraverFillData *data, double nweight); //fill in x,y = 0..1,0..1
	virtual void FillRegularLines(EngraverFillData *data, double nweight);
	virtual void FillRadial(EngraverFillData *data, double nweight);
	virtual void FillCircular(EngraverFillData *data, double nweight);
	virtual void FillSpiral(EngraverFillData *data, double nweight, int numarms, double r0,double b, int spin);
	virtual void QuickAdjust(double factor);
	virtual ImageData *CreateFromSnapshot();
	virtual ImageData *SpacingSnapshot();
	virtual int TraceFromSnapshot();

	virtual void GrowLines_OLD(EngraverFillData *data,
									double resolution, 
									double defaultspace,  	ValueMap *spacingmap,
									double defaultweight,   ValueMap *weightmap, 
									Laxkit::flatpoint direction,    DirectionMap *directionmap,
									Laxkit::PtrStack<GrowPointInfo> *growpoint_ret,
									int iteration_limit);
	virtual GrowContext *GrowLines_Init(EngraverFillData *data,
								double resolution, 
								double defaultspace,  	ValueMap *spacingmap,
								double defaultweight,   ValueMap *weightmap, 
								Laxkit::flatpoint direction,    DirectionMap *directionmap,
								int iteration_limit,
								Laxkit::PtrStack<GrowPointInfo> *custom_starters
								);
	virtual bool GrowLines_Iterate();
	virtual void GrowLines_Finish();

	virtual void UpdateBezCache();
	virtual void UpdatePositionCache();
	virtual int UpdateDashCache();
	virtual void StripDashes();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


//---------------------------------------- EngraverFillData

class EngraverFillStyle : public Laxkit::Resourceable
{
  public:

	EngraverFillStyle *next_group_style; //define stacks of style for when there are multiple groups

	EngraverLineQuality *dashes;
	EngraverTraceSettings *trace;
	EngraverDirection *direction;
	EngraverSpacing *spacing;

	EngraverFillStyle()
	{
		dashes=NULL;
		trace=NULL;
		direction=NULL;
		spacing=NULL;
		next_group_style=NULL;
	}
	virtual ~EngraverFillStyle()
	{
		if (dashes) dashes->dec_count();
		if (trace) trace->dec_count();
		if (direction) direction->dec_count();
		if (spacing) spacing->dec_count();
		if (next_group_style) next_group_style->dec_count();
	} 
	virtual const char *whattype() { return "EngraverFillStyle"; }
};

class EngraverFillData : virtual public PatchData
{
 protected:
  	
 public:
	Laxkit::PtrStack<EngraverPointGroup> groups; 

	EngraverFillData();
	//EngraverFillData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual ~EngraverFillData(); 
	virtual const char *whattype() { return "EngraverFillData"; }
	virtual const char *Id();
	virtual const char *Id(const char *id);
	virtual SomeData *duplicateData(SomeData *dup);
	virtual int renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels);
	virtual int renderToBufferImage(Laxkit::LaxImage *image);

	virtual double DefaultSpacing(double nspacing);
	virtual void MakeDefaultGroup();
	virtual int MakeGroupNameUnique(int which);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);
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
	virtual void BezApproximate(Laxkit::NumStack<Laxkit::flatpoint> &fauxpoints, Laxkit::NumStack<Laxkit::flatpoint> &points);
	virtual void MorePoints(int curgroup);
	virtual EngraverPointGroup *FindGroup(int id, int *err_ret=NULL);
	virtual EngraverPointGroup *GroupFromIndex(int index, int *err_ret=NULL);
	virtual int MergeDown(int which_group);

	virtual int IsSharing(int what, EngraverPointGroup *group, int curgroup); 

	virtual void Update();
};





} //namespace LaxInterfaces

#endif


