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
#ifndef _LAX_PATCHINTERFACE_H
#define _LAX_PATCHINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/selection.h>
#include <lax/screencolor.h>
#include <lax/rectangles.h>
#include <lax/pointset.h>




namespace LaxInterfaces {

typedef enum _PatchControls {
	Patch_Full_Bezier=0,
	Patch_Coons,
	Patch_Border_Only,
	Patch_Linear,
	Patch_MAX
} PatchControls;

	
 // matrix utilities
void getPolyT(double *N, double n, double t0);
void m_transpose(double *M);
void printG(const char *ch,double *G);
void getScaledI(double *I,double a,double b,double c,double d);
void getI(double *I);
void m_times_m(double *a,double *b,double *m);
void m_times_v(double *m,double *b,double *v);
double dot(double *a,double *b);
void getT(double *v,double t) ;


//------------------------------------- PatchRenderContext ------------------------
class PatchRenderContext
{
 public:
	double Cx[16],Cy[16];
	double V[4]; //temp space
	double TT[4], SS[4]; //temp space for getPoint(double,double)
	double s0,ds,t0,dt; //used primarily for PatchData::WhatColor() lookup

	unsigned char *buffer; //a temp, non-local buffer
	int bufferwidth,bufferheight;
	int numchannels;    //usually 4 (argb) and 8bits
	int bitsperchannel; //8 or 16
	int stride; //usually bufferwidth * numchannels * bitsperchannel/8

	PatchRenderContext() {
		buffer=NULL; bufferwidth=bufferheight=stride=0; numchannels=4; bitsperchannel=8;
		SS[0]=SS[1]=SS[2]=SS[3]=TT[0]=TT[1]=TT[2]=TT[3]=1;
	}
	Laxkit::flatpoint getPoint(double *S,double *T);

	Laxkit::flatpoint getPoint(double s,double t);
};


//------------------------------ PatchData ---------------------

//goes in PatchData::style:
enum PatchDataStyles {
	PATCH_SMOOTH    =(1<<0),
	PATCH_Units_BBox=(1<<1), //flag that can be used on import to correspond to svg gradientUnits
	PATCH_MAX = 1
};


class PatchData : virtual public SomeData, virtual public Laxkit::PointCollection
{
  protected:
  	
  public:
	int renderdepth;
	int griddivisions; //hint for display of interior of patch
	Laxkit::flatpoint *points;
	int xsize,ysize; // sizes%3 must == 1, numpoints=xsize*ysize
	unsigned int style;
	PatchControls controls;
	LineStyle linestyle;
	
	PathsData *base_path; //optionally restrict to a weighted path
	int pathdivisions; //number of mesh squares to install perpendicular to path
	int pathmethod; //0 for sampled, 1 for extrapolate

	 //cache transform matrices for faster getPoint()
	PatchRenderContext *cache;
	int ncache; //num allocated in cache
	Laxkit::IntRectangle needtorecache;
	virtual void NeedToUpdateCache(int mincol,int maxcol, int minrow,int maxrow);
	virtual void UpdateCache();

	 //cached outline for convenience, updated in FindBBox()
	int npoints_boundary;
	Laxkit::flatpoint *boundary_outline;


	PatchData(); 
	PatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual ~PatchData();
	virtual const char *whattype() { return "PatchData"; }
	virtual SomeData *duplicateData(SomeData *dup);
	virtual void FindBBox();
	virtual int pointin(Laxkit::flatpoint pp,int pin=1);

	 /*! \name Informational functions */
	 //@{
	virtual int hasColorData();
	virtual int MeshWidth()  { return xsize/3; }
	virtual int MeshHeight() { return ysize/3; }
	virtual Laxkit::flatpoint getControlPoint(int r,int c);
	virtual Laxkit::flatpoint getPoint(double s,double t, bool bysize);
	virtual Laxkit::flatpoint getPointReverse(double x,double y, int *error_ret);
	virtual double getScaling(double s,double t, bool bysize);
	virtual Laxkit::flatpoint *bezAtEdge(Laxkit::flatpoint *p,int i,int row);
	virtual Laxkit::flatpoint *bezCrossSection(Laxkit::flatpoint *p,int i,double t,int row);
	virtual int bezOfPatch(Laxkit::flatpoint *p,int r,int rl,int c,int cl);
	virtual void resolveToSubpatch(double s,double t,int &c,double &ss,int &r,double &tt);
	virtual void resolveFromSubpatch(int c,double ss,int r,double tt,double &s,double &t);
	virtual void getGt(double *G,int roffset,int coffset,int isfory);
	virtual int inSubPatch(Laxkit::flatpoint p,int *r_ret,int *c_ret,double *t_ret,double *s_ret,double d);
	virtual int coordsInSubPatch(Laxkit::flatpoint p,int r,int c,double maxd, double *s_ret,double *t_ret);
	virtual int WhatColor(double s,double t,Laxkit::ScreenColor *color_ret);
	 //@}

	 /*! \name Data modifying functions */
	 //@{
	virtual void CopyMeshPoints(PatchData *patch, bool usepath);
	virtual void Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual void zap(Laxkit::flatpoint p,Laxkit::flatpoint x,Laxkit::flatpoint y);
	virtual void zap(); // zap to bbox
	virtual int subdivide(int r,double rt,int c,double ct);
	virtual int subdivide(int xn=2,int yn=2);
	virtual void grow(int where, const double *tr, bool smooth);
	virtual void collapse(int rr,int cc);
	virtual void InterpolateControls(int whichcontrols);
	virtual int WarpPatch(Laxkit::flatpoint center, double A,double B,double inner, double s,double e, const double *extra);
	virtual int EstablishPath(int preferredaxis);
	virtual int RemovePath();
	virtual int InstallPath(PathsData *path);
	virtual int UpdateFromPath();
	virtual int UpdateFromPathSampled();
	virtual int UpdateFromPathExtrapolate();
	virtual int UsesPath();
	 //@}

	 /*! \name I/O */
	 //@{
	 //rendering functions
	virtual int renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels);
	virtual void rpatchpoint(PatchRenderContext *context,
								Laxkit::flatpoint ul,Laxkit::flatpoint ur,Laxkit::flatpoint ll,Laxkit::flatpoint lr,
								double s1,double t1, double s2,double t2,int which);
	virtual void patchpoint(PatchRenderContext *context, double s0,double ds,double t0,double dt,int n);
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	 //@}
	
	/*! \name From PointSet */
	//@{
	virtual int Depth() { return 2; } //how many dimensions wide are points stored
	virtual int NumPoints(int dim) { return dim == 0 ? ysize : xsize; } //0 rows, 1 columns
	virtual Laxkit::flatpoint Point(int row, ...); //retrieve point, as many numbers as Depth()
	virtual Laxkit::flatpoint Point(Laxkit::flatpoint newPos, int row, ...); //set point
	virtual int Map(std::function<int(const Laxkit::flatpoint &p, Laxkit::flatpoint &newp)> adjustFunc); //ret num changed
	//@}
};



//------------------------------ PatchInterface ---------------------

enum PatchInterfaceActions {
	PATCHA_RenderMode=PATHIA_MAX,
	PATCHA_RecurseInc,
	PATCHA_RecurseDec,
	PATCHA_SmoothEdit,
	PATCHA_MeshType,
	PATCHA_MeshTypeR,
	PATCHA_Select,
	PATCHA_ConstrainY,
	PATCHA_ConstrainX,
	PATCHA_RowDivInc,
	PATCHA_RowDivDec,
	PATCHA_ColDivInc,
	PATCHA_ColDivDec,
	PATCHA_Subdivide,
	PATCHA_SubdivideRows,
	PATCHA_SubdivideCols,
	PATCHA_Rectify,
	PATCHA_Decorations,
	PATCHA_CircleWarp,
	PATCHA_AutoSelectNear,
	PATCHA_SelectCorners,
	PATCHA_SelectMids,
	PATCHA_SelectEdgeMids,
	PATCHA_SelectVerticalEdgeMids,
	PATCHA_SelectHorizontalEdgeMids,
	PATCHA_SelectAllVertically,
	PATCHA_SelectMoreVertically,
	PATCHA_SelectAllHorizontally,
	PATCHA_SelectMoreHorizontally,
	PATCHA_SelectAround,
	PATCHA_SelectControls,
	PATCHA_DeleteSelected,
	PATCHA_BaseOnPath,
	PATCHA_DontBaseOnPath,

	 //other context menu items
	PATCHA_Full,   
	PATCHA_Coons,  
	PATCHA_Borders,
	PATCHA_Linear, 
	PATCHA_MAX
};

class PatchInterface : public anInterface
{
 protected:
	double movetransform[6];
	Laxkit::flatpoint *movepts;
	Laxkit::flatpoint lbdown;
	Laxkit::flatpoint *cuth,*cutv;
	double cutatct, cutatrt;
	int overv,overh,overcv,overch,overstate;
	int dragmode;
	int hoverpoint;

	Laxkit::flatpoint hovertemp;    //DBG!!
	Laxkit::flatpoint hovertemprev; //DBG!!
	
	int bx,by,mx,my,constrain;
	Laxkit::NumStack<int> curpoints; 
	void getG(double *G,int roffset,int coffset,int isfory);
	virtual int selectablePoint(int i);
	int mousedragged;
	virtual int findNearHorizontal(Laxkit::flatpoint fp,double d,double *t_ret,int *i_ret);
	virtual int   findNearVertical(Laxkit::flatpoint fp,double d,double *t_ret,int *i_ret);
	virtual void drawControls();
	virtual void drawControlPoints();
	virtual void drawControlPoint(int i, bool hovered);
	virtual int ActivatePathInterface();
	virtual int ActivateCircleInterface();

	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

 public:
	 //draw exterior and interior grid lines
	enum PatchDecorations {
		 SHOW_Grid   =(1<<0),
		 //draw indicators for the points
		 SHOW_Points =(1<<1),
		 //draw the exterior edges
		 SHOW_Edges  =(1<<2),
		 SHOW_Max    =(1<<3)
	};

	enum RenderMode {
		RENDER_None  = -1,
		RENDER_Grid,
		RENDER_Preview,
		RENDER_Color,
		RENDER_Controls_Only,
		RENDER_MAX   
	};

	 // these are the state:
	unsigned int style;
	LineStyle linestyle;
	unsigned long rimcolor,handlecolor,gridcolor;
	int xs,ys, rdiv,cdiv;
	unsigned long controlcolor;
	int showdecs, oldshowdecs; 
	bool smoothedit;
	char whichcontrols;
	bool auto_select_close;
	double auto_close_threshhold;
	int recurse;
	int rendermode; //see RenderMode
	int drawrendermode; //see RenderMode
	
	Selection *selection;
	PatchData *data;
	ObjectContext *poc;

	PatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~PatchInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual const char *IconId() { return "Patch"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PatchInterface"; }
	virtual const char *whatdatatype() { return "PatchData"; }
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e_data, const char *mes);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int UseThis(Laxkit::anObject *newdata,unsigned int mask=0);
	virtual int UseThis(int id,int ndata);
	virtual void Clear(SomeData *d=NULL);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual ObjectContext *Context() { return poc; }
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual void drawpatch(int roff,int coff);
	virtual void drawpatches();
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);

	virtual PatchData *newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual void deletedata(bool flush_selection);
	virtual int scan(int x,int y);
	virtual int SelectPoint(int c,unsigned int state);
	virtual int AddToSelection(ObjectContext *oc);
	virtual void UpdateFromCircle();
};

} // namespace LaxInterfaces;

#endif

