//
//	
//    The Laxkit, a windowing toolkit
//    Copyright (C) 2004-2006 by Tom Lechner
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
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//

// ***make transform a double[6] in manner of libart/cairo/postscript/etc?
// 		and have:  
// 			int GetAxesN();
// 			const double *GetAxesE(int index); 
// 			double *GetTransform() (return a copy); 
// 			SetTransform(const double *d);
// *** Minx,... are not actually set anywhere
// *** have its own gc? uses defaultgc



#include <lax/lists.cc>
#include <lax/displayer.h>
#include <lax/laxutils.h>
#include <lax/doublebbox.h>
#include <lax/transformmath.h>

#include <cstring>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {



//-------------------------------- DoublePanner --------------------------
/*! \class DoublePanner
 * \brief A PanController that uses doubles as a base.
 *
 * **** Please note that this class is totally unusable currently.
 *
 * There are the screen bounds, which are in screen window coordinates, defined
 * by Minx, Maxx, Miny, and Maxy. Also there are workspace bounds in real coordinates,
 * defined by spaceminx, spacemaxx, spaceminy, and spacemaxy. If the axes are 
 * rotated, then the actual contents of a window may show portions not
 * in the workspace. The bounding rectangle of this rotated space (in screen window coords)
 * is stored in the panner so that scrollers and such have access to something meaningful.
 *
 * *** have to coordinate panner->minsel/maxsel and upperbound/lowerbound.\n
 *
 * <pre>
 * About the Current Transformation matrix (ctm) and its inverse (ictm):
 *
 *  Postscript:
 *      [ a  b  0 ]
 *  CTM=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 * 
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *
 *  Say you want to rotate real, then translate real, then screenp = realp*R*T*CTM.
 *  
 *  Note that in libart, they are presented basically as the transpose of the above,
 *  but the elements of the 6 value array mean essentially the same as both systems.
 *  In the Cairo library, cairo_matrix_t={ double xx,xy, yx,yy, x0,y0 } corresponds
 *  basically to [xx yx xy yy x0 y0].
 * </pre>
 */
/*! \var int DoublePanner::Minx
 * \brief Minimum screen x coordinate.
 */
/*! \var int DoublePanner::Maxx
 * \brief Maximum screen x coordinate.
 */
/*! \var int DoublePanner::Miny
 * \brief Minimum screen y coordinate (near top of screen).
 */
/*! \var int DoublePanner::Maxy
 * \brief Maximum screen y coordinate (near bottom of screen).
 */
/*! \var double DoublePanner::spaceminx
 * \brief Minimum real workspace x coordinate.
 */
/*! \var double DoublePanner::spacemaxx
 * \brief Maximum real workspace x coordinate.
 */
/*! \var double DoublePanner::spaceminy
 * \brief Minimum real workspace y coordinate.
 */
/*! \var double DoublePanner::spacemaxy
 * \brief Maximum real workspace y coordinate.
 */
/*! \var double DoublePanner::upperbound
 * \brief The maximum screen length of an axis and other items.
 */
/*! \var double DoublePanner::lowerbound
 * \brief The minimum screen length of an axis and other items.
 */
/*! \var NumStack<flatpoint> DoublePanner::axesstack
 * \brief The stack of axes. See PushAxes() and PopAxes().
 */
/*! \var double DoublePanner::ctm[]
 * \brief Current Transformation Matrix.
 * 
 * This is a six valued array, see intro for discussion.
 * This matrix is not dynamic like ctm. Is computed on the fly as necessary.
 * Only the ctm is pushed and popped.
 */
/*! \var double DoublePanner::ictm[6]
 * \brief Inverse Current Transformation Matrix.
 * 
 * This is a six valued array, see intro for discussion.
 */
/*! \fn char DoublePanner::onscreen(int x,int y)
 * \brief Return whether screen coordinate (x,y) is within (Minx..Maxx,Miny..Maxy).
 */
//class DoublePanner : public PanController
//{
// protected:
//	double *ctm,ictm[6];
//	
//	int Minx,Maxx,Miny,Maxy;    // screen coords of viewport bounding box
//	double spaceminx,spacemaxx,spaceminy,spacemaxy; // workspace bounds
//	PtrStack<double> axesstack;
//	
// public:
//	double upperbound,lowerbound;
//	DoublePanner(anXWindow *w=NULL,PanController *pan=NULL,char npanislocal=0);
//	~DoublePanner();
//
//	virtual flatpoint realtoscreen(double x,double y);
//	virtual flatpoint realtoscreen(flatpoint p);
//	virtual flatpoint screentoreal(int x,int y);
//	virtual flatpoint screentoreal(flatpoint p);
//
//	 //essentially panner functions:
//	virtual char Updates(char toupdatepanner);
//	virtual void syncPanner(int all=0);
//	virtual void syncFromPanner(int all=0);
//	virtual char onscreen(int x,int y) { return x>=Minx && x<=Maxx && y>=Miny && y<=Maxy; }
//	virtual void ShiftScreen(int dx,int dy);
//	virtual void ShiftReal(double dx,double dy);
//	virtual void Center(double minx,double maxx,double miny,double maxy);
//	virtual void CenterPoint(flatpoint p);
//	virtual void CenterReal();
//	virtual int NewTransform(double *d);
//	virtual int NewTransform(double a,double b,double c,double d,double x0,double y0);
//	virtual void PushAndNewTransform(double *m);
//	virtual void PushAndNewAxes(flatpoint p,flatpoint x,flatpoint y);
//	virtual void PushAxes();
//	virtual void PopAxes();
//	virtual void NewAxis(flatpoint o,flatpoint xtip);
//	virtual void NewAxis(flatpoint o,flatvector x,flatvector y);
//	virtual void Newangle(double angle,int dir=0,int dec=1);
//	virtual void Rotate(double angle,int x,int y,int dec=1);
//	virtual void Newmag(double xs,double ys=-1)/*ys=-1*/;
//	virtual void Zoomr(double m,flatpoint p);
//	virtual void Zoom(double m,int x,int y);
//	virtual void Zoom(double m);
//	virtual double GetMag(int c=0);
//	virtual double GetVMag(int x,int y);
//	virtual void SetView(double minx,double maxx,double miny,double maxy);
//	virtual int RealSpace(double minx,double maxx,double miny,double maxy);
//	virtual void RealSpace(double *minx,double *maxx,double *miny, double *maxy);
//	virtual void getTransformedSpace(long *minx,long *maxx,long *miny,long *maxy);
//	virtual void WrapWindow(anXWindow *nw);
//};
//---------------------------------------------------------

//! Constructor, set everything to nothing or identity.
/*! upperbound=1000, lowerbound=.00001, ctm/ictm=identity,
 * Set Minx=Min=0, Maxx=w,Maxy=h
 */
DoublePanner::DoublePanner(double minx,double maxx,double miny,double maxy, int w,int h) 
	: axesstack(1), PanController()
{ 
	 // init ctm and ictm to identity
	ctm=new double[6];
	ctm[1]=ctm[2]=ctm[3]=ctm[5]=0.0;
	ctm[0]=ctm[3]=1.0;
	ictm[1]=ictm[2]=ictm[3]=ictm[5]=0.0; // ictm is not dynamic created like ctm
	ictm[0]=ictm[3]=1.0;
	
	upperbound=1e+3;
	lowerbound=1e-5;

	Minx=Miny=0;
	Maxx=w;
	Maxy=h;

	spaceminx=minx;
	spaceminy=miny;
	spacemaxx=maxx;
	spacemaxy=maxy;

	boxaspect[0]=w;
	boxaspect[1]=h;
	pixelaspect=1.0;

	 // space is set, must set wholebox, selbox, bounds to meaningful stuff
	setWholebox();
	set selbox to something meaningful... ***
	SetSelBounds(1,1,maxx-minx>maxy-miny?maxx-minx:maxy-miny);*** correspond to upper/lowerbounds
	SetSelBounds(2,1,maxx-minx>maxy-miny?maxx-minx:maxy-miny);
}

//! Destructor, delete ctm.
DoublePanner::~DoublePanner()
{ 
	delete[] ctm;
}

//! Using displayer viewable portion settings, set the panner settings.
/*! If all is nonzero, then set curpos and space in panner. Otherwise
 * just set the curpos.
 */
void DoublePanner::syncPanner(int all)//all=0
{***
	//if (!updatepanner) return;
DBG cerr<<"---====DoublePanner syncPanner"<<endl;
	// set the panner selbox values from the current displayer settings.
	// Maxx-Minx  is portion of transformed workspace, this is what to set in Panner
	// screentoreal(Minx,Miny)
	// realtoscreen(spaceminx,spaceminy) etc
	// 
	// If the current displayer settings do not conform to what the
	// panner allows, then call syncFromPanner();
	
	long minx,maxx,miny,maxy;
	//**** maybe (&double, &double...) rather than long? check for too big (>LONG_MAX, <LONG_MIN) from limits.h
	getTransformedSpace(&minx,&maxx,&miny,&maxy);
	
	SetWholebox(minx,maxx,miny,maxy);
	*** should set selbounds from upperbound/lowerbound
		
	SetCurPos(1,Minx,Maxx);
	SetCurPos(2,Miny,Maxy);
}

//! Convert real point p to screen coordinates.
/*! <pre>
 *  screen x= ax + cy + tx  --> screen = [x',y',1] = [x,y,1] * CTM  = real * CTM
 *  screen y= bx + dy + ty
 *  </pre>
 */
flatpoint DoublePanner::realtoscreen(flatpoint p)
{
	return flatpoint(ctm[4] + ctm[0]*p.x + ctm[2]*p.y, ctm[5]+ctm[1]*p.x+ctm[3]*p.y); 
} 

//! Convert real point (x,y) to screen coordinates.
flatpoint DoublePanner::realtoscreen(double x,double y)
{
	return flatpoint(ctm[4] + ctm[0]*x + ctm[2]*y, ctm[5]+ctm[1]*x+ctm[3]*y); 
} 

//! Convert screen point (x,y) to real coordinates.
flatpoint DoublePanner::screentoreal(int x,int y)
{
	return flatpoint(ictm[4] + ictm[0]*(double)x + ictm[2]*(double)y, ictm[5]+ictm[1]*(double)x+ictm[3]*(double)y); 
}

//! Convert screen point p to real coordinates.
flatpoint DoublePanner::screentoreal(flatpoint p)
{
	return flatpoint(ictm[4] + ictm[0]*p.x + ictm[2]*p.y, ictm[5]+ictm[1]*p.x+ictm[3]*p.y); 
}


//! Centers the view at real point p
void DoublePanner::CenterPoint(flatpoint p)
{
	flatpoint s;
	s= flatpoint((Minx+Maxx)/2,(Miny+Maxy)/2)-realtoscreen(p);
	ShiftScreen((int)s.x,(int)s.y);
}

//! Make the center of Minx,Maxx... correspond to the real origin.
/*! Min/Max are screen coordinates, so ctm[4],y is screen position of real origin
 */
void DoublePanner::CenterReal()
{
//cout <<"displayer Centerreal:x,y,rx,ry:["<<Minx<<","<<Maxx<<"] ["<<Miny<<","<<Maxy<<"] "<<ctm[4]<<","<<ctm[5]<<endl;
	ctm[4]=(Minx+Maxx)/2; ctm[5]=(Miny+Maxy)/2; 
	transform_invert(ictm,ctm);
	syncPanner();
DBG cerr <<"--displayer Centerrreal:x,y,rx,ry:["<<Minx<<","<<Maxx<<"] ["<<Miny<<","<<Maxy<<"] "<<ctm[4]<<","<<ctm[5]<<endl;
}

//! Move the viewable portion by dx,dy screen units.
void DoublePanner::ShiftScreen(int dx,int dy)
{
	ictm[4]-=dx;
	ictm[5]-=dy;
	ctm[4]+=dx;
	ctm[5]+=dy;
	transform_invert(ictm,ctm);
	syncPanner();
}

//! Move the screen by real dx,dy along the real axes.
void DoublePanner::ShiftReal(double dx,double dy)
{
	ctm[4]+=dx*ctm[0]+dy*ctm[2];
	ctm[5]+=dx*ctm[1]+dy*ctm[3];
	transform_invert(ictm,ctm);
	syncPanner();
}

//! Make the transform correspond to the values.
int DoublePanner::NewTransform(double a,double b,double c,double d,double x0,double y0)
{
	ctm[0]=a;
	ctm[1]=b;
	ctm[2]=c;
	ctm[3]=d;
	ctm[4]=x0;
	ctm[5]=y0;
	transform_invert(ictm,ctm);
	syncPanner();
}

//! Push axes, and multiply ctm by a new transform.**** implement!!
void DoublePanner::PushAndNewTransform(double *m)
{
	PushAxes();
	double *tctm=transform_mult(NULL,ctm,m);
	delete[] ctm;
	ctm=tctm;
	transform_invert(ictm,ctm);
}

//! Push axes, and multiply ctm by a new basis, p,x,y are all in real units.
/*! *** implement!! *** */
void DoublePanner::PushAndNewAxes(flatpoint p,flatpoint x,flatpoint y)
{
	PushAxes();
	NewAxis(p,x,y);
}

//! Push the current axes on the axessstack.
void DoublePanner::PushAxes()
{
	axesstack.push(ctm,1);
	ctm=new double[6];
	for (int c=0; c<6; c++) ctm[c]=axesstack.e[axesstack.n-1][c];
	transform_invert(ictm,ctm);
}

//! Recover the last pushed axes.
void DoublePanner::PopAxes()
{
	if (axesstack.n==0) return;
	delete[] ctm;
	ctm=axesstack.pop();
	transform_invert(ictm,ctm);
}

//! Set the ctm to these 6 numbers.
int DoublePanner::NewTransform(double *d)
{
	for (int c=0; c<6; c++) ctm[c]=d[c];
	transform_invert(ictm,ctm);
	syncPanner();
}

//! Replace current transform wth a new orthogonal basis with origin o, xaxis unit corresponds to (xtip-o).
void DoublePanner::NewAxis(flatpoint o,flatpoint xtip)
{
	NewAxis(o,xtip-o,transpose(xtip-o));
}

//! Replace current transform with origin o, with x and y as the new axes.
void DoublePanner::NewAxis(flatpoint o,flatvector x,flatvector y)
{
	if (x.y*y.x!=0 && x.y*y.x==x.x*y.y) return; //if x || y
	ctm[0]=x.x;
	ctm[1]=x.y;
	ctm[2]=y.x;
	ctm[3]=y.y;
	ctm[4]=o.x;
	ctm[5]=o.y;
	transform_invert(ictm,ctm);
}

//! Rotate by angle, about screen coordinate (x,y).
/*! dec nonzero means angle is degrees, otherwise radians.
 */
void DoublePanner::Rotate(double angle,int x,int y,int dec) // dec=1
{
	flatpoint p=screentoreal(x,y);
	Newangle(angle,1,dec);
	p=realtoscreen(p);
	ctm[4]+=x-p.x;
	ctm[5]+=y-p.y;
	transform_invert(ictm,ctm);
	syncPanner(1);
}

//! Rotate around real origin so that the x axis has angle between it and the screen horizontal.
/*! TODO: this could certainly be optimized..
 */
void DoublePanner::Newangle(double angle,int dir,int dec) // dir=0,dec=1
{
	if (dec) angle*=M_PI/180;
	flatpoint rx(ctm[0],ctm[1]),ry(ctm[2],ctm[3]);
	if (dir==0) { /* absolute slant */
      	double angle2=atan2(ctm[1],ctm[0]); // current angle of rx to screen x
		rx=rotate(rx,-angle2+angle);
		ry=rotate(ry,-angle2+angle);
	} else if (dir<0) { /* rotate CW */
		rx=rotate(rx,-angle);
		ry=rotate(ry,-angle);
	} else { /* rotate CCW */
		rx=rotate(rx,angle);
		ry=rotate(ry,angle);
	}
	ctm[0]=rx.x;
	ctm[1]=rx.y;
	ctm[2]=ry.x;
	ctm[3]=ry.y;
	transform_invert(ictm,ctm);
	syncPanner(1);
}
//  [xs] = [ctm[4]] + [X*Xr X*Yr][xr]
//  [ys]   [ctm[5]]   [Y*Xr Y*Yr][yr]

//! Zoom around real point p.
void DoublePanner::Zoomr(double m,flatpoint p)
{
	flatpoint po=realtoscreen(p);
	Zoom(m);
	po=realtoscreen(p)-po;
	ctm[4]+=po.x;
	ctm[5]+=po.y;
	transform_invert(ictm,ctm);
	syncPanner();
}

//! Zoom around screen point x,y.
void DoublePanner::Zoom(double m,int x,int y)
{
	flatpoint po=screentoreal(x,y);
	char udp=updatepanner;
	updatepanner=0;
DBG cerr <<"\nZoom:"<<m<<"   around real="<<po.x<<","<<po.y<<"  =s:"<<x<<','<<y<<"  ctm4,5="<<ctm[4]<<','<<ctm[5]<<endl;
	Zoom(m);
	po=realtoscreen(po);
DBG cerr <<"   shift:"<<po.x-x<<","<<po.y-y<<endl;
	ctm[4]+=x-po.x;
	ctm[5]+=y-po.y;
	transform_invert(ictm,ctm);
DBG po=screentoreal(x,y);
DBG cerr <<"   new realfromscreen:"<<po.x<<","<<po.y<<"  ctm4,5="<<ctm[4]<<','<<ctm[5]<<endl;
	updatepanner=udp;
	syncPanner();
}

//! Zoom around the real origin.
void DoublePanner::Zoom(double m) // zooms with origin constant point
{
//cout <<"DoublePanner zoom: "<<m<<" rx:"<<(ctm[0]*ctm[0]+ctm[1]*ctm[1])<<"  ry:"<<ctm[2]*ctm[2]+ctm[3]*ctm[3]<<endl;
	if (m<=0) return;

	if (m<1 && (m*sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1])<lowerbound || 
				m*sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3])<lowerbound)) return;
	if (m>1 && (m*sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1])>upperbound || 
				m*sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3])>upperbound)) return;
//cout <<"  adjusting mag..."<<endl;
	ctm[0]*=m;
	ctm[1]*=m;
	ctm[2]*=m;
	ctm[3]*=m;
	transform_invert(ictm,ctm);
	syncPanner();
}

//! Return the magnification along the screen vector (x,y). (screen=mag*real)
/*! This is useful when the axes are of different lengths or are not
 * orthogonal, or are rotated.
 */
double DoublePanner::GetVMag(int x,int y)
{
	flatpoint v=screentoreal(x,y),v2=screentoreal(0,0);
	return sqrt((x*x+y*y)/((v-v2)*(v-v2)));
}

//! Return the current magnification, screen=GetMag*real
/*! If c!=0 then return the y scale, default is to return the x scale.
 *
 * Note that this only returns basically length of axis in screen units divided
 * by length of axis in real coords. If the axes are rotated, they do not
 * correspond necessarily to the screen horizontal or vertical magnification.
 * For that, call GetVMag.
 */
double DoublePanner::GetMag(int c) // c=0
{ 
	if (c) return sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]);
	return sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]);
}

//! Set the inverse of the current transformation matrix, assuming ctm is already set.
/*! *** this function is pre-empted by transform_invert.... should remove transform_invert(ictm,ctm)?
 * <pre>
 *      [ a  b  0 ]
 *  CTM=[ c  d  0 ]  --> [a b c d tx ty]
 *      [ tx ty 1 ]
 *
 * {{d/(a*d-b*c),           (-b)/(a*d-b*c),        0},
 *  {(-c)/(a*d-b*c),        a/(a*d-b*c),           0},
 *  {(c*ty-d*tx)/(a*d-b*c), (b*tx-a*ty)/(a*d-b*c), 1}}
 * </pre>
 */
void DoublePanner::transform_invert(ictm,ctm)
{ transform_invert(ictm,ctm); }

//! Set the xscale and the yscale.
/*! If ys<=0, then the y scale is set the same as the xscale.
 *
 * TODO:this could certainly be optimized.
 */
void DoublePanner::Newmag(double xs,double ys)//ys=-1
{
	if (xs<=0) return;
	flatpoint rx(ctm[0],ctm[1]),ry(ctm[2],ctm[3]);
	rx=(xs/sqrt(ctm[0]*ctm[0]+ctm[1]*ctm[1]))*rx;
	if (ys<=0) ys=xs;
	ry=(ys/sqrt(ctm[2]*ctm[2]+ctm[3]*ctm[3]))*ry;
	ctm[0]=rx.x;
	ctm[1]=rx.y;
	ctm[2]=ry.x;
	ctm[3]=ry.y;
	transform_invert(ictm,ctm);
	syncPanner();
}

//!*** Call Updates(0) to disable updating the panner, Updates(1) to reenable it.
/*! This is used during refreshing, when there is constant
 * pushing and popping of the transform. Just sets a flag so that syncPanner
 * returns before doing anything.
 *
 * The programmer is responsible for balancing the push/pop of axes so that 
 * when updates are activated, the displayer and panner are still synced.
 *
 * Returns the old value of updatepanner.
 */
char DoublePanner::Updates(char toupdatepanner)
{
	updatepanner=toupdatepanner;
}

//! Using the settings in the panner, try to set the DoublePanner settings.
/*! This would occur, for instance, on a "pan change" event resulting from
 * a scroller move. The panner does not implement rotating, so any changes would
 * be from a page size change or a straight move in screen x or y directions.
 * Thus the panner wholespace does not change its values, but the panner selbox
 * has. However, the displayer "selbox" in Minx,Maxx.. 
 * 
 * panner contains the transformed workspace coords. Real space=ictm*twc.
 * So we must find a new transform so that the space fits the panner settings.
 *
 * This function is not called from within the DoublePanner. A holding window
 * (such as ViewportWindow) would call it upon receipt of a "pan change"
 * **** but must ensure it is correct time!!
 */
void DoublePanner::syncFromPanner(int all)
{***
DBG cerr <<"-=-= syncFromPanner"<<endl;
	//***if (!on) return;
	
	// panner->[start, end] must correspond to this->[Minx,Maxx]
	// transformed spaceminx/y still map to panner->min/max, but
	// they will not map after syncing, though they should have the same w and h.
	//
	// screen = real * ctm
	// real ---> transformed
	
	long xs,xe,ys,ye;
	panner->GetCurPos(1,&xs,&xe);
	panner->GetCurPos(2,&ys,&ye);

	// the transform is a rotation/scale/shear/translation. The rotation has stayed
	// the same. Using the old inverse transform, find the real points corresponding
	// to the new panner selbox:
	//*** this is really the long way around, and probably propogates panner rounding errors..
DBG dumpctm(ctm);
	flatpoint r1,r2,r3, s1,s2,s3;
	s1=flatpoint(xs,ys);
	s2=flatpoint(xe,ys);
	s3=flatpoint(xe,ye);
	r1=screentoreal(s1); // --> this must map back to (Minx,Miny)
	r2=screentoreal(s2); // --> this must map back to (Maxx,Miny)
	r3=screentoreal(s3); // --> this must map back to (Maxx,Maxy)
	s1=flatpoint(Minx,Miny);
	s2=flatpoint(Maxx,Miny);
	s3=flatpoint(Maxx,Maxy);
	 // Those 3 real points are transformed by [A B C D X Y] to 3 screen points.
	 // 6 linear equations, 6 unknowns, viola! (Really 2 sets of 3 eq/3 unks)
	 // {{(e*i-f*h)/(--),(c*h-b*i)/(--),(b*f-c*e)/(--)
	 // ,{(f*g-d*i)/(--),(a*i-c*g)/(--),(c*d-a*f)/(--)
	 // ,{(d*h-e*g)/(--),(b*g-a*h)/(--),(a*e-b*d)/(--)
	 //
	 // i,f,c==1
	 // (--)=a*e*i-a*f*h+c*d*h-b*d*i+b*f*g-c*e*g
	 //           (a*e-a*h +         d*h-b*d +          b*g-e*g)
	 //          a*(e   -h) +       d*(h   -b) +       g*(b   -e)
	double dd=r1.x*(r2.y-r3.y) + r1.y*(r3.x-r2.x) + r2.x*r3.y-r2.y*r3.x;
DBG cerr <<"dd="<<dd<<endl;

	//**** something screwed here.
	ctm[0]=(s1.x*(r2.y-r3.y) +           s2.x*(r3.y-r1.y) +           s3.x*(r1.y-r2.y))/dd; // s1.x  s2.x  s3.x
	ctm[2]=(s1.x*(r3.x-r2.x) +           s2.x*(r1.x-r3.x) +           s3.x*(r2.x-r1.x))/dd;
	ctm[4]=(s1.x*(r2.x*r3.y-r2.y*r3.x) + s2.x*(r1.y*r3.x-r1.x*r3.y) + s3.x*(r1.x*r2.y-r1.y*r2.x))/dd;
	
	ctm[1]=(s1.y*(r2.y-r3.y) +           s2.y*(r3.y-r1.y) +           s3.y*(r1.y-r2.y))/dd; // s1.y  s2.y  s3.y
	ctm[3]=(s1.y*(r3.x-r2.x) +           s2.y*(r1.x-r3.x) +           s3.y*(r2.x-r1.x))/dd;
	ctm[5]=(s1.y*(r2.x*r3.y-r2.y*r3.x) + s2.y*(r1.y*r3.x-r1.x*r3.y) + s3.y*(r1.x*r2.y-r1.y*r2.x))/dd;

DBG dumpctm(ctm);
	transform_invert(ictm,ctm);
	//*** should do final check to cut off any rounding errors; must preserve relative
	//scaling from before adjusting transform...
	syncPanner(); // This is necessary because the selbox in the panner always corresponds to 
				//   Minx,Miny..Maxx,Maxy, which is not what is in the panner before this line.
				//   ***IF the wholebox and selbox were reversed, ..... more thought here!!
				//   this is too complicated!
}

//! Zoom and center the view on the given real bounds.
/*! This will not rotate the display, but will shift and scale to view
 * properly. The axes aspect is maintained. A gap of about 5% is placed
 * around the bounds.
 *
 * ****** muts implement this!!!
 */
void DoublePanner::Center(double minx,double maxx,double miny,double maxy)
{
	DoubleBBox  bb(realtoscreen(flatpoint(minx,miny)));
	bb.addtobounds(realtoscreen(flatpoint(maxx,miny)));
	bb.addtobounds(realtoscreen(flatpoint(maxx,maxy)));
	bb.addtobounds(realtoscreen(flatpoint(minx,maxy)));
//	char udp=Updates(0);
	if (maxx-minx) Newmag((Maxx-Minx)/(maxx-minx));
	CenterPoint(flatpoint((minx+maxx)/2,(miny+maxy)/2));
//	Updates(udp);
	syncPanner();
}

//! Set the view area to these real bounds.
/*! These bounds will fit over the transformed workspace, and are
 * to be in screen coordinates. That is, after a workspace
 * is transformed into screen coordinates, the new coordinates are called transformed
 * workspace coordinates. This is relevant since a workspace that is rotated takes
 * up more in screen extent than an unrotated one.
 *
 * Min/Maxx/y will not be changed, but 
 * afterwards they will correspond to these bounds.
 * This is essentially a more complicated form of zooming.
 *
 * *** this needs LOTS o work, right now just shifts (minx,miny) to 
 * correspond to (Minx,Miny) .. must be sure to preserve axes aspect
 */
void DoublePanner::SetView(double minx,double maxx,double miny,double maxy)
{
	if (minx>maxx || miny>maxy) return;
	 //Min* are in window coords, not transformed workspace??
	 //make (Minx,Miny)--> (minx,miny), do not change Min*
	flatpoint minpoint=realtoscreen(flatpoint(minx,miny));
	ShiftScreen((int)(Minx-minpoint.x),(int)(Miny-minpoint.y));
}

//! Set the workspace bounds, return nonzero if successful.
/*! Returns 1 if successful and viewable area not changed.\n
 * Returns 2 if successful and viewable are is changed.
 */
int DoublePanner::RealSpace(double minx,double maxx,double miny,double maxy)
{
	if (minx>maxx) { double t=minx; minx=maxx; maxx=t; }
	if (miny>maxy) { double t=miny; miny=maxy; maxy=t; }
	spaceminx=minx;
	spacemaxx=maxx;
	spaceminy=miny;
	spacemaxy=maxy;
DBG cerr <<"--displayer space: ["<<spaceminx<<','<<spacemaxx<<"] ["<<spaceminy<<','<<spacemaxy<<']'<<endl;
DBG cerr <<"--displayer win:   ["<<Minx<<','<<Maxx<<"] ["<<Miny<<','<<Maxy<<']'<<endl;

	syncPanner(1);
	return 2;
}

//! Get the workspace bounds
void DoublePanner::RealSpace(double *minx,double *maxx,double *miny, double *maxy)
{
	if (minx) *minx=spaceminx;
	if (maxx) *maxx=spacemaxx;
	if (miny) *miny=spaceminy;
	if (maxy) *maxy=spacemaxy;
}

//! Find the mins and maxes of the transformed workspace.
/*! It is ok to have some of the pointers NULL.
 */
void DoublePanner::getTransformedSpace(long *minx,long *maxx,long *miny,long *maxy)
{
	flatvector  v1=realtoscreen(flatpoint(spaceminx,spaceminy)),
				v2=realtoscreen(flatpoint(spacemaxx,spaceminy)),
				v3=realtoscreen(flatpoint(spacemaxx,spacemaxy)),
				v4=realtoscreen(flatpoint(spaceminx,spacemaxy)),
				spanmin,spanmax;
	spanmin=spanmax=v1;
	if (v2.x<spanmin.x) spanmin.x=v2.x;
	if (v3.x<spanmin.x) spanmin.x=v3.x;
	if (v4.x<spanmin.x) spanmin.x=v4.x;
	
	if (v2.y<spanmin.y) spanmin.y=v2.y;
	if (v3.y<spanmin.y) spanmin.y=v3.y;
	if (v4.y<spanmin.y) spanmin.y=v4.y;
	
	if (v2.x>spanmax.x) spanmax.x=v2.x;
	if (v3.x>spanmax.x) spanmax.x=v3.x;
	if (v4.x>spanmax.x) spanmax.x=v4.x;
	
	if (v2.y>spanmax.y) spanmax.y=v2.y;
	if (v3.y>spanmax.y) spanmax.y=v3.y;
	if (v4.y>spanmax.y) spanmax.y=v4.y;

	if (minx) *minx=(long) spanmin.x;
	if (maxx) *maxx=(long) spanmax.x;
	if (miny) *miny=(long) spanmin.y;
	if (maxy) *maxy=(long) spanmax.y;
}

//! Set the viewable portion of the displayer to correspond with the window's win_w and win_h
/*! So Minx,Maxx,Miny,Maxy==0,win_w, 0,win_h
 * Also sets the panner boxaspect to correspond to the window proportions.
 *  If the spacemin/max seem not to be set, then set things so that the whole space is
 *  10 times the window size, and the viewable portion is in the middle.
 *
 *  *** this does not reassign dpy,gc,w, etc. That is done in StartDrawing. seems silly
 *  to wrap to this window, but not make all the other properties correspond to nw.
 *  needs more thought here.
 */
void DoublePanner::WrapWindow(anXWindow *nw)
{
	if (!nw || !nw->window) return;
	Minx=0;
	Miny=0;
	Window rootwin;
	int x,y;
	unsigned int width,h,bwidth,depth;
	XGetGeometry(nw->app->dpy,nw->window,&rootwin,&x,&y,&width,&h,&bwidth,&depth);
	Maxx=width;
	Maxy=h;
	if (Maxx<Minx) Maxx=Minx;
	if (Maxy<Miny) Maxy=Miny;
	if (spaceminx==spacemaxx) {
		spaceminx=-(Maxx-Minx)*5;
		spacemaxx=(Maxx-Minx)*5;
	}
	if (spaceminy==spacemaxy) {
		spaceminy=-(Maxy-Miny)*5;
		spacemaxy=(Maxy-Miny)*5;
	}
	panner->dontTell(xw);
	panner->SetBoxAspect(Maxx-Minx+1,Maxy-Miny+1);//***
	panner->dontTell(NULL);
	syncPanner();
DBG cerr <<"-----displayer WrapWindow:MinMaxx,:["<<Minx<<","<<Maxx<<"] MinMaxy:["<<Miny<<","<<Maxy<<"] x0,y0:"<<ctm[4]<<","<<ctm[5]<<endl;
}


} // namespace Laxkit

