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
//    Copyright (C) 2004-2007,2010-2015 by Tom Lechner
//
#ifndef _LAX_PATHINTERFACE_H
#define _LAX_PATHINTERFACE_H


#include <lax/interfaces/viewportwindow.h>
#include <lax/interfaces/coordinate.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/fillstyle.h>
#include <lax/curveinfo.h>
#include <lax/screencolor.h>
#include <lax/dump.h>



namespace LaxInterfaces {


class PathOperator;

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
	double center_t; //t position on cached centerline

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

class Path : public LaxFiles::DumpUtility, public Laxkit::DoubleBBox
{
 protected:
 public:
	static Laxkit::PtrStack<PathOperator> basepathops; 

	Coordinate *path; // path is not necessarily the first in a chain, but is a vertex
	Laxkit::PtrStack<PathWeightNode> pathweights;

	LineStyle *linestyle; 
	double defaultwidth;
	bool absoluteangle; //1==absolute, or 0==relative to direction to path, wich angle==0 do default

	int needtorecache;
	int cache_samples;
	int cache_types;
	bool save_cache;
	//std::time_t cache_mod_time;
	Laxkit::CurveInfo cache_offset;
	Laxkit::CurveInfo cache_width;
	Laxkit::CurveInfo cache_angle;
	Laxkit::NumStack<flatpoint> outlinecache; //bezier c-v-c-...
	Laxkit::NumStack<flatpoint> centercache; //bezier c-v-c-...
	Laxkit::NumStack<flatpoint> cache_top; //bezier c-v-c-...
	Laxkit::NumStack<flatpoint> cache_bottom; //bezier c-v-c-...
	virtual void UpdateS(bool all, int resolution=16);
	virtual void UpdateCache();
	virtual void UpdateWidthCache();

	Path();
	Path(Coordinate *np,LineStyle *nls=NULL);
	virtual ~Path();
	virtual Path *duplicate();
	virtual void FindBBox();

	 //building functions
	virtual Coordinate *lastPoint(int v=0);
	virtual void append(double x,double y,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL);
	virtual void append(flatpoint p,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL);
	virtual void append(Coordinate *coord);
	virtual void appendBezFromStr(const char *value);
	virtual int removePoint(Coordinate *p, bool deletetoo);
	virtual void moveTo(flatpoint p);
	virtual void lineTo(flatpoint p);
	virtual void curveTo(flatpoint c1, flatpoint c2, flatpoint p2);
	virtual int close();
	virtual int openAt(Coordinate *curvertex, int after);
	virtual Coordinate *addAt(double t);
	virtual int addAt(Coordinate *curvertex, Coordinate *np, int after);
	virtual void clear();
	virtual int Line(LineStyle *nlinestyle);
	virtual int LineColor(Laxkit::ScreenColor *ncolor);
	virtual int RemoveDoubles(double threshhold);

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
	virtual int ApplyOffset();
	virtual int SetOffset(double towhat, bool diff=false);
	virtual int SetAngle(double towhat, int absolute);
	virtual int MakeStraight(Coordinate *from, Coordinate *to, bool asbez);

	 //info functions
	virtual int Intersect(flatpoint p1,flatpoint p2, int isline, double startt, flatpoint *pts,int ptsn, double *t,int tn);
	virtual Coordinate *GetCoordinate(double t);
	virtual int PointAlongPath(double t, int tisdistance, flatpoint *point, flatpoint *tangent);
	virtual int PointInfo(double t, int tisdistance, flatpoint *point, flatpoint *tangentafter, flatpoint *tangentbefore,
						            flatpoint *ptop, flatpoint *pbottom,
									double *offset, double *width, double *angle);
	virtual flatpoint ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist);
	virtual int Reverse();
	virtual double Length(double tstart,double tend);
	virtual double distance_to_t(double distance, int *err);
	virtual double t_to_distance(double t, int *err);
	virtual int NumVertices(bool *isclosed_ret);
	virtual bool IsClosed();
	virtual int GetIndex(Coordinate *p, bool ignore_controls);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what, LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
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
	Laxkit::PtrStack<Path> paths;
	LineStyle *linestyle; //!< This is the default line style for any paths that are added.
	FillStyle *fillstyle; //!< This is the fill style for the collection of paths

	//PathsData(unsigned long ns=(unsigned long)PATHS_Ignore_Weights);
	PathsData(unsigned long ns=0);
	virtual ~PathsData();
	virtual const char *whattype() { return "PathsData"; }
	virtual void FindBBox();
	virtual SomeData *duplicate(SomeData *dup);

	virtual int line(double width,int cap=-1,int join=-1,Laxkit::ScreenColor *color=NULL);
	virtual int fill(Laxkit::ScreenColor *color);
	virtual void InstallLineStyle(LineStyle *newlinestyle);
	virtual void InstallFillStyle(FillStyle *newfillstyle);

	virtual bool Weighted(int whichpath=-1);
	virtual bool HasOffset(int whichpath=-1);
	virtual bool Angled(int whichpath=-1);
	virtual void Recache(bool now=false);
	virtual int ApplyOffset(int whichpath);
	virtual int SetOffset(int whichpath, double towhat);
	virtual int SetAngle(int whichpath, double towhat, int absolute);
	virtual int MakeStraight(int whichpath, Coordinate *from, Coordinate *to, bool asbez);

	virtual int hasCoord(Coordinate *co);
	virtual int pathHasCoord(int pathindex,Coordinate *co);
	virtual void appendCoord(Coordinate *coord,int whichpath=-1);
	virtual void append(double x,double y,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void append(flatpoint p,unsigned long flags=POINT_VERTEX,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void appendRect(double x,double y,double w,double h,SegmentControls *ctl=NULL,int whichpath=-1);
	virtual void appendEllipse(flatpoint center, double xradius, double yradius, double angle, int num_vertices, bool closed);
	virtual void appendBezArc(flatpoint center, double angle, int num_vertices);
	virtual void moveTo(flatpoint p,int whichpath=-1);
	virtual void lineTo(flatpoint p,int whichpath=-1);
	virtual void curveTo(flatpoint c1, flatpoint c2, flatpoint p2, int whichpath=-1);
	virtual void close(int whichpath=-1);
	virtual Coordinate *LastVertex();
	virtual void pushEmpty(int where=-1,LineStyle *nls=NULL);
//	virtual int AddAfter(Coordinate *afterwhich,flatpoint p); 
//	virtual int Delete(Coordinate *which); // returns num left in stack
	virtual void clear(int which=-1);

	virtual int ConnectEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);
	virtual void ApplyTransform();
	virtual void MatchTransform(Affine &affine);
	virtual void MatchTransform(const double *mm);

	virtual flatpoint ClosestPoint(flatpoint point, double *disttopath, double *distalongpath, double *tdist, int *pathi);
	virtual int Intersect(int pathindex,flatpoint p1,flatpoint p2, int isline, double startt,flatpoint *pts,int ptsn, double *t,int tn);
	virtual int PointAlongPath(int pathindex, double t, int tisdistance, flatpoint *point, flatpoint *tangent);
	virtual Coordinate *GetCoordinate(int pathi, double t);
	virtual int ReversePath(int pathindex);
	virtual double Length(int pathi, double tstart,double tend);
	//virtual int NumPoints(int vertices=1);

	virtual int NumPaths() { return paths.n; }
	virtual Path *GetPath(int index);
	virtual Path *GetOffsetPath(int index);

	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what, LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//-------------------- PathsData utility

PathsData *SvgToPathsData(PathsData *existingpath, const char *d,char **end_ptr, LaxFiles::Attribute *powerstroke);
int SetClipFromPaths(Laxkit::Displayer *dp, LaxInterfaces::SomeData *outline, const double *extra_m, bool real=false);


//-------------------- PathOperator ---------------------------
	

class PathInterface;

class PathOperator : public Laxkit::anObject
{
 public:
	int id;

	PathOperator(int nid=-1);
	virtual ~PathOperator();
	virtual void dumpOut(FILE *f, int indent, SegmentControls *controls, int what, LaxFiles::DumpContext *context) = 0;
	virtual void dumpIn(Coordinate *attachto, LaxFiles::Attribute *att, LaxFiles::DumpContext *context) = 0;

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
	virtual Coordinate *newPoint(flatpoint p) = 0; // pop after/before, prefunit=0 means full unit

	virtual int ShiftPoint(Coordinate *pp,flatpoint d) = 0;
	virtual int Rotate(flatpoint o,double angle,Coordinate *pp) = 0;
	virtual int Scale(flatpoint o,double f,int contsrain, Coordinate *pp) = 0;

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
	PATHI_Esc_Off_Sub      =(1<<6),
	PATHI_Plain_Click_Add  =(1<<7),
	PATHI_No_Weights       =(1<<8),
	PATHI_Single_Weight    =(1<<9),
	PATHI_No_Offset        =(1<<10),
	PATHI_No_Angle_Weight  =(1<<11),
	PATHI_Render_With_Cache=(1<<12),
	PATHI_Hide_Path        =(1<<13),
	PATHI_Send_On_Changes  =(1<<14),
	PATHI_Defer_Render     =(1<<15)
};

enum PathInterfaceActions {
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
	PATHIA_FlipVertically,
	PATHIA_FlipHorizontally,
	PATHIA_Close,
	PATHIA_Decorations,
	PATHIA_StartNewPath,
	PATHIA_StartNewSubpath,
	PATHIA_ToggleWeights,
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
	PATHIA_Combine,
	PATHIA_ExtractPath,
	PATHIA_ExtractAll,
	PATHIA_Copy,
	PATHIA_Cut,
	PATHIA_Paste,
	PATHIA_ShowNumbers,
	PATHIA_NewFromStroke,
	PATHIA_BreakApart,
	PATHIA_BreakApartChunks,

	PATHIA_Bevel,
	PATHIA_Miter,
	PATHIA_Round,
	PATHIA_Extrapolate,
	PATHIA_CapRound,
	PATHIA_CapButt,
	PATHIA_CapZero,

	PATHIA_MAX
};

class PathInterface : public anInterface
{
 protected:
	class SelectedPoint //prep for actions on multiple pathsdata objects
	{
	  public:
		ObjectContext *context; //nonlocal ref to context in selection
		PathsData *paths;
		int pathindex;
		Coordinate *point; //if null, then index is which weight node
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
	int colortofill;
	double widthstep;
	
	Path *curpath;
	flatpoint curdirv;
	Coordinate *curdirp;
	Coordinate  defaulthoverp;
	Coordinate *curvertex; // curvertex points to the relevant vertex point, not to the last selected point
	Coordinate *lbfound;
	int lbselected;
	int drawhover;
	int drawpathi;
	int drawhoveri;
	int hoverdevice;
	int hoverpointtype;
	flatpoint hoverpoint;
	flatpoint hoverdir;
	flatpoint hoversegment[4];
	int lasth; //direction of toggling selected handle
	
	//Laxkit::PtrStack<PathOperator> pathops; 
	PathOperator *curpathop;

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);
	
	virtual void clearSelection();
	virtual PathOperator *getPathOpFromId(int iid);
	virtual PathOperator *getPathOp(Coordinate *p);
	virtual void selectPoint(Coordinate *p,char flag);
	virtual void removeSegment(Coordinate *c);
	virtual Coordinate *scannear(Coordinate *p,char u,double radius=5);
	virtual void SetCurvertex(Coordinate *p, int path=-1);
	virtual void UpdateDir();
	virtual int WeightNodePosition(Path *path, PathWeightNode *weight,
									flatpoint *pp_ret, flatpoint *po_ret, flatpoint *ptop_ret, flatpoint *pbottom_ret,
									flatpoint *vv_ret, flatpoint *vt_ret,
									int needtotransform);

	virtual int ConnectEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);
	virtual int MergeEndpoints(Coordinate *from,int fromi, Coordinate *to,int toi);

	virtual int shiftBezPoint(Coordinate *p,flatpoint d);
	virtual int shiftSelected(flatpoint d);
	virtual int scaleSelected(flatpoint center,double f,int constrain);
	virtual int rotateSelected(flatpoint center,double angle);

	virtual void hoverMessage();
	virtual void drawNewPathIndicator(flatpoint p,int which);
	virtual void drawWeightNode(Path *path, PathWeightNode *weight, int isfornew);
	virtual void DrawBaselines();
	virtual void DrawOutlines();

 public:
	 // the following three comprise the default PathInterface settings.
	unsigned long controlcolor;
	unsigned long creationstyle;
	unsigned long pathi_style;
	bool show_weights;
	bool show_baselines;
	bool show_outline;
	
	Laxkit::PtrStack<Coordinate> curpoints;
	
	int showdecs;
	int verbose;
	PathsData *data;
	ObjectContext *poc;

	PathInterface(int nid,Laxkit::Displayer *ndp, unsigned long nstyle=PATHI_Render_With_Cache);
	virtual ~PathInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicate(anInterface *dup);
	virtual PathsData *newPathsData();

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
	virtual Coordinate *scanEndpoints(int x,int y,int *pathindex,Coordinate *exclude);
	virtual int scanHover(int x,int y,unsigned int state, int *pathi);
	virtual int scanWeights(int x,int y,unsigned int state, int *pathindex, int *index);
	virtual flatpoint screentoreal(int x,int y);
	virtual flatpoint realtoscreen(flatpoint r);
	virtual int toggleclosed(int c=-1);
	virtual int AddPoint(flatpoint p);
	virtual void SetPointType(int newtype);
	virtual void SetPointType(Coordinate *v,int newtype);
	virtual void MakeCircle();
	virtual void MakeRect();
	virtual int CutNear(flatpoint hoverpoint);
};


} // namespace LaxInterfaces


#endif

