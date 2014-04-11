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
//    Copyright (C) 2004-2007,2010-2011 by Tom Lechner
//
#ifndef _LAX_PATCHINTERFACE_H
#define _LAX_PATCHINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/screencolor.h>




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
	double s0,ds,t0,dt;

	unsigned char *buffer;
	int bufferwidth,bufferheight;
	int numchannels;    //usually 4 (argb) and 8bits
	int bitsperchannel; //8 or 16
	int stride; //usually bufferwidth * numchannels * bitsperchannel/8

	flatpoint getPoint(double *S,double *T);
};

//------------------------------ PatchData ---------------------

#define PATCH_SMOOTH      (1<<0)


class PatchData : virtual public SomeData
{
  protected:
  	
  public:
	int renderdepth;
	int griddivisions;
	flatpoint *points;
	int xsize,ysize; // sizes%3 must == 1, numpoints=xsize*ysize
	unsigned int style;
	PatchControls controls;
	LineStyle linestyle;
	
	int npoints_boundary;
	flatpoint *boundary_outline; //cached for convenience, updated in FindBBox()

	PatchData(); 
	PatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual ~PatchData();
	virtual const char *whattype() { return "PatchData"; }
	virtual SomeData *duplicate(SomeData *dup);
	virtual void FindBBox();
	virtual int pointin(flatpoint pp,int pin=1);

	 /*! \name Informational functions */
	 //@{
	virtual int hasColorData();
	virtual flatpoint getControlPoint(int r,int c);
	virtual flatpoint getPoint(double t,double s);
	virtual flatpoint *bezAtEdge(flatpoint *p,int i,int row);
	virtual flatpoint *bezCrossSection(flatpoint *p,int i,double t,int row);
	virtual int bezOfPatch(flatpoint *p,int r,int rl,int c,int cl);
	virtual void resolveToSubpatch(double s,double t,int &c,double &ss,int &r,double &tt);
	virtual void resolveFromSubpatch(int c,double ss,int r,double tt,double &s,double &t);
	virtual void getGt(double *G,int roffset,int coffset,int isfory);
	virtual int inSubPatch(flatpoint p,int *r_ret,int *c_ret,double *t_ret,double *s_ret,double d);
	virtual int coordsInSubPatch(flatpoint p,int r,int c,double maxd, double *s_ret,double *t_ret);
	virtual int WhatColor(double s,double t,Laxkit::ScreenColor *color_ret);
	 //@}

	 /*! \name Data modifying functions */
	 //@{
	virtual void CopyMeshPoints(PatchData *patch);
	virtual void Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual void zap(flatpoint p,flatpoint x,flatpoint y);
	virtual void zap(); // zap to bbox
	virtual int subdivide(int r,double rt,int c,double ct);
	virtual int subdivide(int xn=2,int yn=2);
	virtual void grow(int where, double *tr);
	virtual void collapse(int rr,int cc);
	virtual void InterpolateControls(int whichcontrols);
	virtual int warpPatch(flatpoint center, double r1,double r2, double s,double e);
	 //@}

	 /*! \name I/O */
	 //@{
	 //rendering functions
	virtual int renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels);
	virtual void rpatchpoint(PatchRenderContext *context,
								flatpoint ul,flatpoint ur,flatpoint ll,flatpoint lr,
								double s1,double t1, double s2,double t2,int which);
	virtual void patchpoint(PatchRenderContext *context, double s0,double ds,double t0,double dt,int n);
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
	 //@}
};



//------------------------------ PatchInterface ---------------------

enum PatchInterfaceActions {
	PATCHA_RenderMode,
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
	PATCHA_DeleteSelected,
	PATCHA_MAX
};

class PatchInterface : public anInterface
{
 protected:
	double movetransform[6];
	flatpoint *movepts;
	flatpoint lbdown;
	flatpoint *cuth,*cutv;
	double cutatct, cutatrt;
	int overv,overh,overcv,overch,overstate;
	int dragmode;
	int hoverpoint;
	
	int bx,by,mx,my,constrain;
	Laxkit::NumStack<int> curpoints; 
	void getG(double *G,int roffset,int coffset,int isfory);
	virtual int selectablePoint(int i);
	int mousedragged;
	virtual int findNearHorizontal(flatpoint fp,double d,double *t_ret,int *i_ret);
	virtual int   findNearVertical(flatpoint fp,double d,double *t_ret,int *i_ret);
	virtual void drawControlPoints();
	virtual void drawControlPoint(int i);

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

	 // these are the state:
	LineStyle linestyle;
	unsigned long rimcolor,handlecolor,gridcolor;
	int xs,ys, rdiv,cdiv;
	unsigned long controlcolor;
	int showdecs, oldshowdecs; 
	char whichcontrols;
	int recurse;
	int rendermode;
	
	PatchData *data;
	ObjectContext *poc;

	PatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~PatchInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual const char *IconId() { return "Patch"; }
	virtual const char *Name();
	virtual const char *whattype() { return "PatchInterface"; }
	virtual const char *whatdatatype() { return "PatchData"; }
	virtual anInterface *duplicate(anInterface *dup);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid);
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
	virtual void deletedata();
	virtual int scan(int x,int y);
	virtual int SelectPoint(int c,unsigned int state);
};

} // namespace LaxInterfaces;

#endif

