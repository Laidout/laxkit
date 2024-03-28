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
//    Copyright (C) 2004-2007,2010-2015 by Tom Lechner
//
#ifndef _LAX_PATHINTERFACE_H
#define _LAX_PATHINTERFACE_H


#include <lax/interfaces/viewportwindow.h>
#include <lax/interfaces/coordinate.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/fillstyle.h>
#include <lax/interfaces/lineprofile.h>
// #include <lax/interfaces/shapebrush.h>
#include <lax/curveinfo.h>
#include <lax/screencolor.h>
#include <lax/dump.h>

#include <functional>


namespace LaxInterfaces {


class PathsData;
class PathOperator;
class LineProfile;
class ShapeBrush;


//These are added to Coordinate::flags, and devs should ensure they do not conflict with normal flags
#define BEZ_MASK           (255<<20)
#define BEZ_STIFF_EQUAL    (1<<20)
#define BEZ_STIFF_NEQUAL   (1<<21)
#define BEZ_NSTIFF_EQUAL   (1<<22)
#define BEZ_NSTIFF_NEQUAL  (1<<23)


//-------------------- Path ---------------------------

class PathWeightNode
{
  public:
	int type; //symmetric width, fixed width with offset, width1/2 independent
	double width; //total width of stroke at this node
	double offset; //0 means weight symmetric about path
	double angle; //see Path::absoluteangle, it affects what this value is
	double t; //point along path, the bezier parameter: integer t corresponds to line points
	double center_t; //t position on cached centerline (todo)

	enum PathWeightNodeTypes {
		Default=0,
		Width      =(1<<0),
		Offset     =(1<<1),
		DualOffset =(1<<2),
		Angle      =(1<<3),
		AbsAngle   =(1<<4)
	};
	int nodetype; //or'd PathWeightNodeTypes

	PathWeightNode();
	PathWeightNode(double nt,double noffset, double nwidth, double nangle, int ntype=Default);
	double topOffset() { return offset+width/2; }
	double bottomOffset() { return offset-width/2; }
};

class Path : virtual public Laxkit::anObject,
			 virtual public Laxkit::DoubleBBox,
			 virtual public Laxkit::DumpUtility
{
  public:
 	static Laxkit::PtrStack<PathOperator> basepathops; 

	Coordinate *path; // path is not necessarily the first in a chain, but is a vertex
	Laxkit::PtrStack<PathWeightNode> pathweights;

	LineStyle *linestyle;
	LineProfile *profile;
	ShapeBrush *brush; //this will be the same as parent PathsData::brush (if it is contained there)
	double defaultwidth;
	bool absoluteangle; //1==absolute, or 0==relative to direction to path, wich angle==0 do default
	Laxkit::anObject *generator_data;

	//------ cache funcs ------
	int needtorecache;
	int cache_samples;
	int cache_types; // If 1, then also compute cache_top and cache_bottom
	bool save_cache; // Whether to save cache on file out
	//std::time_t cache_mod_time;
	Laxkit::CurveInfo cache_offset;
	Laxkit::CurveInfo cache_width;
	Laxkit::CurveInfo cache_angle;
	Laxkit::NumStack<Laxkit::flatpoint> outlinecache; //bezier c-v-c-...
	Laxkit::NumStack<Laxkit::flatpoint> centercache; //bezier c-v-c-...
	Laxkit::NumStack<Laxkit::flatpoint> cache_top; //bezier c-v-c-... like top half of outline cache without joins
	Laxkit::NumStack<Laxkit::flatpoint> cache_bottom; //bezier c-v-c-... like bottom half of outline cache without joins
	virtual void UpdateS(bool all, int resolution=30);
	virtual void UpdateCache();
	virtual void UpdateWidthCache();

	Path();
	Path(Coordinate *np,LineStyle *nls=NULL);
	virtual ~Path();
	virtual const char *whattype() { return "Path"; }
	virtual Laxkit::anObject *duplicate(Laxkit::anObject *ref); // redef from anObject
	virtual Path *duplicate();
	virtual void FindBBox();
	virtual void FindBBoxBase(DoubleBBox *ret);
	virtual void FindBBoxWithWidth(DoubleBBox *ret);
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);

	 //building functions
	virtual Coordinate *lastPoint(int v=0);
	virtual void append(double x,double y,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL);
	virtual void append(Laxkit::flatpoint p,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL);
	virtual void append(Coordinate *coord);
	virtual void append(Laxkit::flatpoint *pts, int n);
	virtual void appendBezFromStr(const char *value);
	virtual void AppendPath(Path *p, bool absorb_path, double merge_ends, int at = -1);
	virtual int MakeRoundedRect(double x, double y, double w, double h, Laxkit::flatpoint *sizes, int numsizes);
	virtual int MakeSquircleCubic(double x, double y, double w, double h, double *sizes, int numsizes);
	virtual int removePoint(Coordinate *p, bool deletetoo);
	virtual void moveTo(Laxkit::flatpoint p);
	virtual void lineTo(Laxkit::flatpoint p);
	virtual void curveTo(Laxkit::flatpoint c1, Laxkit::flatpoint c2, Laxkit::flatpoint p2);
	virtual int close();
	virtual int openAt(Coordinate *curvertex, int after);
	virtual int CutSegment(Coordinate *curvertex, int after, Path **remainder);
	virtual Coordinate *AddAt(double t);
	virtual int AddAt(double *t, int n, double *t_ret);
	virtual int AddAt(Coordinate *curvertex, Coordinate *np, int after);
	virtual int CutAt(double t, Path **new_path_ret);
	virtual int LerpSimple(Path *a, Path *b, double t);
	virtual void Transform(const double *mm);
	virtual void clear();

	virtual int Line(LineStyle *nlinestyle);
	virtual int LineColor(Laxkit::ScreenColor *ncolor);
	virtual int ApplyLineProfile(LineProfile *p, bool linked);
	virtual int ApplyLineProfile();
	virtual int UseShapeBrush(ShapeBrush *newbrush);
	virtual int RemoveDoubles(double threshhold);
	virtual int Reverse();

	 //weight node related
	virtual bool Weighted();
	virtual bool ConstantWidth();
	virtual bool HasOffset();
	virtual bool Angled();
	virtual void InsertWeightNode(double nt);
	virtual void AddWeightNode(double nt,double noffset,double nwidth,double nangle);
	virtual int RemoveWeightNode(int which);
	virtual int MoveWeight(int which, double nt);
	virtual int GetWeight(double t, double *width, double *offset, double *angle);
	virtual void SortWeights();
	virtual void ClearWeights();
	virtual int ApplyOffset();
	virtual int ApplyUpperStroke();
	virtual int ApplyLowerStroke();
	virtual int SetOffset(double towhat, bool diff=false);
	virtual int SetAngle(double towhat, int absolute);
	virtual int MakeStraight(Coordinate *from, Coordinate *to, bool asbez);

	 //info functions
	virtual int Intersect(Laxkit::flatpoint p1,Laxkit::flatpoint p2, int isline, double startt, Laxkit::flatpoint *pts,int ptsn, double *t,int tn);
	virtual Coordinate *GetCoordinate(double t);
	virtual int PointAlongPath(double t, int tisdistance, Laxkit::flatpoint *point, Laxkit::flatpoint *tangent, int resolution=50);
	virtual int PointInfo(double t, int tisdistance, Laxkit::flatpoint *point, Laxkit::flatpoint *tangentafter, Laxkit::flatpoint *tangentbefore,
						            Laxkit::flatpoint *ptop, Laxkit::flatpoint *pbottom,
									double *offset, double *width, double *angle);
	virtual Laxkit::flatpoint ClosestPoint(Laxkit::flatpoint point, double *disttopath, double *distalongpath, double *tdist, int resolution=50);
	virtual double Length(double tstart,double tend);
	virtual double distance_to_t(double distance, int *err, int resolution=50);
	virtual double t_to_distance(double t, int *err, int resolution=50);
	virtual int NumVertices(bool *isclosed_ret);
	virtual bool IsClosed();
	virtual int GetIndex(Coordinate *p, bool ignore_controls);
	virtual int Contains(Path *otherpath);
	virtual int FindExtrema(Laxkit::NumStack<Laxkit::flatpoint> *points_ret, Laxkit::NumStack<double> *t_ret);
	virtual void MinMax(Laxkit::flatvector direction, Laxkit::flatvector &min, Laxkit::flatvector &max);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};


//-------------------- PathsData ---------------------------
	


class PathsData : virtual public SomeData
{
 protected:
 public:
	enum PathsDataStyle { //these are or'd in this->style
		PATHS_Ignore_Weights=(1<<0),
		PATHS_Save_Cache    =(1<<1),
		PATHS_MAX           =(1<<2)
	};

	unsigned long style; // contains FILL_* for combining(?)
	Laxkit::RefPtrStack<Path> paths;
	LineStyle *linestyle; //!< This is the default line style for any paths that are added.
	FillStyle *fillstyle; //!< This is the fill style for the collection of paths
	ShapeBrush *brush;    //!< Default shape brush. Each Path can override default.

	int generator;
	Laxkit::anObject *generator_data;

	//PathsData(unsigned long ns=(unsigned long)PATHS_Ignore_Weights);
	PathsData(unsigned long ns=0);
	virtual ~PathsData();
	virtual const char *whattype() { return "PathsData"; }
	virtual void FindBBox();
	virtual void FindBBoxBase(DoubleBBox *ret);
	virtual void FindBBoxWithWidth(DoubleBBox *ret);
	virtual void ComputeAABB(const double *transform, DoubleBBox &box);
	virtual SomeData *duplicate(SomeData *dup);

	virtual int line(double width,int cap=-1,int join=-1,Laxkit::ScreenColor *color=NULL);
	virtual int fill(Laxkit::ScreenColor *color);
	virtual void InstallLineStyle(LineStyle *newlinestyle);
	virtual void InstallFillStyle(FillStyle *newfillstyle);

	virtual int UseShapeBrush(ShapeBrush *newbrush);

	virtual bool Weighted(int whichpath=-1);
	virtual bool HasOffset(int whichpath=-1);
	virtual bool Angled(int whichpath=-1);
	virtual void Recache(bool now=false);
	virtual int ApplyOffset(int whichpath);
	virtual int SetOffset(int whichpath, double towhat);
	virtual int SetAngle(int whichpath, double towhat, int absolute);
	virtual int MakeStraight(int whichpath, Coordinate *from, Coordinate *to, bool asbez);
	virtual bool IsEmpty();

	virtual int hasCoord(Coordinate *co);
	virtual int pathHasCoord(int pathindex,Coordinate *co);
	virtual void appendCoord(Coordinate *coord,int whichpath=-1);
	virtual void append(double x,double y,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void append(Laxkit::flatpoint p,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void appendRect(double x,double y,double w,double h,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual bool appendRect(DoubleBBox *box, int whichpath=-1);
	virtual int MakeRoundedRect(int pathi, double x, double y, double w, double h, Laxkit::flatpoint *sizes, int numsizes);
	virtual int MakeSquircleCubic(int pathi, double x, double y, double w, double h, double *sizes, int numsizes);
	virtual void appendEllipse(Laxkit::flatpoint center, double xradius, double yradius, double angle, double offset, int num_vertices, int closed);
	virtual void appendBezArc(Laxkit::flatpoint center, double angle, int num_vertices);
	virtual void appendSvg(const char *d);
	virtual void moveTo(Laxkit::flatpoint p,int whichpath=-1);
	virtual void lineTo(Laxkit::flatpoint p,int whichpath=-1);
	virtual void curveTo(Laxkit::flatpoint c1, Laxkit::flatpoint c2, Laxkit::flatpoint p2, int whichpath=-1);
	virtual void close(int whichpath=-1);
	virtual Coordinate *LastVertex();
	virtual void pushEmpty(int where=-1,LineStyle *nls=NULL);
//	virtual int AddAfter(Coordinate *afterwhich,flatpoint p); 
//	virtual int Delete(Coordinate *which); // returns num left in stack
	virtual void clear(int which=-1);
	virtual int RemovePath(int index, Path **popped_ret);
	virtual int CutSegment(Coordinate *coord, bool after, bool remove_dangling);
	virtual int CutAt(int pathindex, double t);
	virtual int CutAt(int n, int *paths, double *t);
	virtual int AddAtIntersections(bool segment_loops, bool self_path_only, int pathi=-1);
	virtual int AddAt(int pathindex, double t);
	virtual int AddAt(int n, int *paths, double *t);

	virtual int ConnectEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);
	virtual int MergeEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);
	virtual void ApplyTransform();
	virtual void MatchTransform(Affine &affine);
	virtual void MatchTransform(const double *mm);
	virtual void SetOriginToBBoxPoint(Laxkit::flatpoint p);

	virtual Laxkit::flatpoint ClosestPoint(Laxkit::flatpoint point, double *disttopath, double *distalongpath, double *tdist, int *pathi, int resolution=50);
	virtual int Intersect(int pathindex,Laxkit::flatpoint p1,Laxkit::flatpoint p2, int isline, double startt,Laxkit::flatpoint *pts,int ptsn, double *t,int tn);
	virtual int PointAlongPath(int pathindex, double t, int tisdistance, Laxkit::flatpoint *point, Laxkit::flatpoint *tangent, int resolution=50);
	// virtual int Sample(int pathindex, double step, bool step_is_distance, double from, double to, bool fit_step, Laxkit::PtrStack<Affine> &tr_ret);
	// virtual int SampleN(int pathindex, int n_points, bool step_is_distance, double from, double to, Laxkit::PtrStack<Affine> &tr_ret);
	virtual Coordinate *GetCoordinate(int pathi, double t);
	virtual int ReversePath(int pathindex);
	virtual double Length(int pathi, double tstart,double tend);
	virtual int FindExtrema(int pathindex, Laxkit::NumStack<Laxkit::flatpoint> *points_ret, Laxkit::NumStack<double> *t_ret);
	virtual int MinMax(int pathi, Laxkit::flatvector direction, Laxkit::flatvector &min, Laxkit::flatvector &max);
	
	virtual int NumPaths() { return paths.n; }
	virtual Path *GetPath(int index);
	virtual Path *GetOffsetPath(int index);

	virtual PathsData *MergeWith(PathsData *otherPath, double *transform_from_other, double endpoint_merge_threshhold, 
								 bool extract_paths_from_other, bool return_new);

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

	// typedef void (*PointMapFunc)(const flatpoint &p, flatpoint &newp);
 	virtual int Map(std::function<int(const Laxkit::flatpoint &p, Laxkit::flatpoint &newp)> adjustFunc);

 	virtual int Undo(Laxkit::UndoData *data); // return 0 for sucess, nonzero error
	virtual int Redo(Laxkit::UndoData *data); // return 0 for sucess, nonzero error
};


//-------------------- PathsData utility

PathsData *SvgToPathsData(PathsData *existingpath, const char *d,char **end_ptr, Laxkit::Attribute *powerstroke);
int SetClipFromPaths(Laxkit::Displayer *dp, LaxInterfaces::SomeData *outline, const double *extra_m, bool real=false);
int GetNumPathsData(SomeData *obj, int *num_other);

//-------------------- PathOperator ---------------------------
	

class PathInterface;

class PathOperator : public Laxkit::anObject
{
 public:
	int id;

	PathOperator(int nid=-1);
	virtual ~PathOperator();
	virtual void dumpOut(FILE *f, int indent, SegmentControls *controls, int what, Laxkit::DumpContext *context) = 0;
	virtual void dumpIn(Coordinate *attachto, Laxkit::Attribute *att, Laxkit::DumpContext *context) = 0;

	virtual void drawControls(Laxkit::Displayer *dp, int which, SegmentControls *controls,
							  int showdecs, PathInterface *pathi) = 0;

	 //return an interface that can manipulate relevant data
	virtual anInterface *getInterface(Laxkit::Displayer *dp,PathsData *data) = 0;

	 //return a thing id for yes, there is something at screen coordinate (x,y), or 0 for none
	virtual int scan(Laxkit::Displayer *dp,SegmentControls *ctl,double x,double y) = 0;

	 // Return a new point and associated points. It must return
	 // the most previous point in the group. This gets appended to whatever. 
	 // It must not return closed loops. By comparison, a new bez point would create 3
	 // points: control1--vertex--control2, and would return control1.
	 // The returned segment must start and end with a vertex.
	virtual Coordinate *newPoint(Laxkit::flatpoint p) = 0; // pop after/before, prefunit=0 means full unit

	virtual int ShiftPoint(Coordinate *pp,Laxkit::flatpoint d) = 0;
	virtual int Rotate(Laxkit::flatpoint o,double angle,Coordinate *pp) = 0;
	virtual int Scale(Laxkit::flatpoint o,double f,int constrain, Coordinate *pp) = 0;

	//virtual int UseThis(***);
};
	


	
//-------------------- PathInterface ---------------------------

enum PathInterfaceSettings {
	PATHI_One_Path_Only    =(1<<0),
	PATHI_Two_Point_Minimum=(1<<1),
	PATHI_Path_Is_Screen   =(1<<2),
	PATHI_Path_Is_M_Screen =(1<<3),
	PATHI_Path_Is_Real     =(1<<4),
	PATHI_Path_Is_M_Real   =(1<<5),
	PATHI_Path_Is_MM_Real  =(1<<6), //transform = obj->m * extram
	PATHI_Esc_Off_Sub      =(1<<7),
	PATHI_Plain_Click_Add  =(1<<8),
	PATHI_No_Weights       =(1<<9),
	PATHI_Single_Weight    =(1<<10),
	PATHI_No_Offset        =(1<<11),
	PATHI_No_Angle_Weight  =(1<<12),
	PATHI_Render_With_Cache=(1<<13),
	PATHI_Hide_Path        =(1<<14),
	PATHI_Send_On_Changes  =(1<<15),
	PATHI_Defer_Render     =(1<<16)
};

enum PathInterfaceActions {
	PATHIA_None = 0,
	PATHIA_CurpointOnHandle,
	PATHIA_CurpointOnHandleR,
	PATHIA_Pathop,
	PATHIA_ToggleAbsAngle,
	PATHIA_ToggleOutline,
	PATHIA_ToggleBaseline,
	PATHIA_ToggleFillRule,
	PATHIA_ToggleFill,
	PATHIA_ToggleStroke,
	PATHIA_ColorFillOrStroke,
	PATHIA_RollNext,
	PATHIA_RollPrev,
	PATHIA_ToggleAddAfter,
	PATHIA_TogglePointType,
	PATHIA_PointTypeSmooth,
	PATHIA_PointTypeSmoothUnequal,
	PATHIA_PointTypeCorner,
	PATHIA_Select,
	PATHIA_SelectInPath,
	PATHIA_SelectInvert,
	PATHIA_FlipVertically,
	PATHIA_FlipHorizontally,
	PATHIA_Close,
	PATHIA_CutSegment,
	PATHIA_Decorations,
	PATHIA_StartNewPath,
	PATHIA_StartNewSubpath,
	PATHIA_ToggleWeights,
	PATHIA_ToggleShowPoints,
	PATHIA_ToggleHideControls,
	PATHIA_ToggleShowExtrema,
	PATHIA_Wider,
	PATHIA_Thinner,
	PATHIA_WidthStep,
	PATHIA_WidthStepR,
	PATHIA_MakeCircle,
	PATHIA_MakeRect,
	PATHIA_MakeStraight,
	PATHIA_MakeBezStraight,
	PATHIA_ApplyOffset,
	PATHIA_ResetOffset,
	PATHIA_ResetAngle,
	PATHIA_Reverse,
	PATHIA_Delete,
	PATHIA_UseLineProfile,
	PATHIA_UseShapeBrush,
	PATHIA_ClearShapeBrush,
	PATHIA_SaveAsShapeBrush,
	PATHIA_Combine, //todo
	PATHIA_BreakApart, //todo
	PATHIA_BreakApartChunks, //todo
	PATHIA_Copy, //todo
	PATHIA_Cut, //todo
	PATHIA_Paste, //todo
	PATHIA_ShowNumbers, //todo
	PATHIA_NewFromStroke, //todo
	PATHIA_Subdivide, //todo
	PATHIA_SubdivideExtrema, //todo
	PATHIA_SubdivideExtremaH, //todo
	PATHIA_SubdivideExtremaV, //todo
	PATHIA_SubdivideInflection, //todo

	PATHIA_Bevel,
	PATHIA_Miter,
	PATHIA_Round,
	PATHIA_Extrapolate,
	PATHIA_CapRound,
	PATHIA_CapButt,
	PATHIA_CapZero,
	PATHIA_EndCapSame,
	PATHIA_EndCapRound,
	PATHIA_EndCapButt,
	PATHIA_EndCapZero,

	PATHIA_MAX
};

class PathInterface : public anInterface
{
	const double *datam();

 protected:
	class SelectedPoint //prep for actions on multiple pathsdata objects
	{
	  public:
		ObjectContext *context; //nonlocal ref to context in selection
		PathsData *paths;
		int pathindex;
		Coordinate *point; //if !null, then index is point (not vertex) index in the path
		PathWeightNode *weight; //if !null, then index is in weight stack
		int index;
	};
	//Laxkit::PtrStack<SelectedPoint> curpoints;

	 //PathInterface potentially non-local state
	int addmode;
	int editmode;
	LineStyle *linestyle,*defaultline;
	FillStyle *fillstyle,*defaultfill;

	PathWeightNode defaultweight;

	 //other state
	int constrain;
	int addafter;
	int colortofill; //whether generic color events should go to fill (1) or stroke (0)
	double widthstep;
	
	Path *curpath;
	Laxkit::flatpoint curdirv;
	Coordinate *curdirp;
	Coordinate  defaulthoverp;
	Coordinate *curvertex; // curvertex points to the relevant vertex point, not to the last selected point
	Coordinate *lbfound;
	int lbselected;
	int drawhover; //what hovered
	int drawpathi; //which subpath hovered
	int drawhoveri; //can be weight node index
	int hoverdevice;
	int hoverpointtype;
	int edit_pathi; //keep track for numerical input
	int edit_weighti;
	Laxkit::flatpoint hoverpoint;
	Laxkit::flatpoint hoverdir;
	Laxkit::flatpoint hoversegment[4];
	Laxkit::Affine extram;
	int lasth; //direction of toggling selected handle
	int last_action, recent_action;
	Laxkit::NumStack<Laxkit::flatvector> extrema;

	int show_addpoint; //0 no, 1 one bez segs, 2 two bez segs (adding within line)
	Laxkit::flatpoint add_point_hint[6];
	
	//Laxkit::PtrStack<PathOperator> pathops; 
	PathOperator *curpathop;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	virtual void Modified(int level=0);
	
	virtual void clearSelection();
	virtual PathOperator *getPathOpFromId(int iid);
	virtual PathOperator *getPathOp(Coordinate *p);
	virtual void selectPoint(Coordinate *p,char flag);
	virtual void removeSegment(Coordinate *c);
	virtual Coordinate *scannear(Coordinate *p,char u,double radius=5);
	virtual bool IsNearSelected(Coordinate *coord);
	virtual void SetCurvertex(Coordinate *p, int path=-1);
	virtual void UpdateAddHint();
	virtual void UpdateDir();
	virtual void UpdateExtrema();
	virtual int WeightNodePosition(Path *path, PathWeightNode *weight,
									Laxkit::flatpoint *pp_ret, Laxkit::flatpoint *po_ret, Laxkit::flatpoint *ptop_ret, Laxkit::flatpoint *pbottom_ret,
									Laxkit::flatpoint *vv_ret, Laxkit::flatpoint *vt_ret,
									int needtotransform);

	virtual int ConnectEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);
	virtual int MergeEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);

	virtual int shiftBezPoint(Coordinate *p,Laxkit::flatpoint d);
	virtual int shiftSelected(Laxkit::flatpoint d);
	virtual int scaleSelected(Laxkit::flatpoint center,double f,int constrain);
	virtual int rotateSelected(Laxkit::flatpoint center,double angle);

	virtual void hoverMessage();
	virtual void drawNewPathIndicator(Laxkit::flatpoint p,int which);
	virtual void drawWeightNode(Path *path, PathWeightNode *weight, int isfornew);
	virtual void DrawBaselines();
	virtual void DrawOutlines();

	virtual void UpdateViewportColor();

 public:
	// the following bunch comprise the default PathInterface settings.
	unsigned long controlcolor;
	unsigned long addcolor;
	unsigned long creationstyle;
	unsigned long pathi_style;
	bool show_weights;
	bool show_points; //0 no, 1 v + c of current, 2 all points
	bool show_baselines;
	bool show_outline;
	bool show_extrema;
	bool hide_other_controls;
	double arrow_size;
	double select_radius = 5; //!< selection distance
	double select_radius2; //!< convenince variable for the square of select_radius, must recompute when select_radius changes
	double dir_select_radius = 35; //!< selection distance for the direction indicator
	
	// selection
	Laxkit::PtrStack<Coordinate> curpoints;
	
	// current state
	int showdecs;
	int verbose;
	PathsData *data;
	ObjectContext *poc;

	PathInterface(int nid,Laxkit::Displayer *ndp, unsigned long nstyle=PATHI_Render_With_Cache);
	virtual ~PathInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup);
	virtual PathsData *newPathsData();
	virtual int InitializeResources();

	 // from anInterface:
	virtual const char *IconId() { return "Path"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PathInterface"; }
	virtual const char *whatdatatype() { return "PathsData"; }
	virtual bool Setting(unsigned int flag, bool on);
	virtual void Dp(Laxkit::Displayer *ndp);
	virtual int DrawDataDp(Laxkit::Displayer *ndp,SomeData *ndata,
			Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int DrawData(Laxkit::anObject *ndata, Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual ObjectContext *Context() { return poc; }
	virtual int Refresh();
	virtual int UseThisObject(ObjectContext *oc);
	virtual int UseThisObject(PathsData *pathobject, const double *extramatrix);
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d=NULL);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e_data, const char *mes);
	
	//virtual void RegisterOp(PathOperator *apathop);
	virtual int ChangeCurpathop(int newiid);

	virtual int VerticesSelected();
	virtual int DeleteCurpoints();
	virtual int DeletePoint(Coordinate *p);
	virtual void deletedata();
	virtual Coordinate *scan(int x,int y,int pmask=0, int *pathindex=NULL);
	virtual Coordinate *scanEndpoints(int x,int y,int *pathindex,Coordinate *exclude, bool *is_first);
	virtual int scanHover(int x,int y,unsigned int state, int *pathi);
	virtual int scanWeights(int x,int y,unsigned int state, int *pathindex, int *index);
	virtual Laxkit::flatpoint screentoreal(int x,int y);
	virtual Laxkit::flatpoint realtoscreen(Laxkit::flatpoint r);
	virtual int toggleclosed(int c=-1);
	virtual int AddPoint(Laxkit::flatpoint p);
	virtual void SetPointType(int newtype);
	virtual void SetPointType(Coordinate *v,int newtype);
	virtual void MakeCircle();
	virtual void MakeRect();
	virtual int CutNear(Laxkit::flatpoint hoverpoint);
	virtual int CutSegment();
};

//------------------------- SomeDataUndo -------------------------------

class PathUndo : public Laxkit::UndoData
{
  public:
	enum UndoTypes {
		MovePoints,
		DelPoints,
		AddPoints,
		AddPath,
		DelPath,
		Reorder,
		ReplacePath,
		WeightMove,
		WeightAdd,
		WeightDel,
		WeightValues,
		MAX
	};

	int type;

	Path *path;
	Laxkit::NumStack<int> path_indices;
	Laxkit::NumStack<int> indices;
	Laxkit::NumStack<Laxkit::flatpoint> points;

	PathUndo(PathsData *object, int ntype, int nisauto);
	~PathUndo();
	virtual const char *Description();
};

} // namespace LaxInterfaces


#endif

