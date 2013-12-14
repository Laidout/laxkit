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
#ifndef _LAX_DOUBLEPANNER_H
#define _LAX_DOUBLEPANNER_H

#include <lax/anxapp.h>
#include <lax/vectors.h>
#include <lax/pancontroller.h>


namespace Laxkit {

class DoublePanner : public PanController
{
 protected:
	double *ctm,ictm[6];
	
	int Minx,Maxx,Miny,Maxy;    // screen coords of viewport bounding box
	double spaceminx,spacemaxx,spaceminy,spacemaxy; // workspace bounds
	PtrStack<double> axesstack;
	
 public:
	double upperbound,lowerbound;
	DoublePanner(anXWindow *w=NULL,PanController *pan=NULL,char npanislocal=0);
	~DoublePanner();

	virtual flatpoint realtoscreen(double x,double y);
	virtual flatpoint realtoscreen(flatpoint p);
	virtual flatpoint screentoreal(int x,int y);
	virtual flatpoint screentoreal(flatpoint p);

	 //essentially panner functions:
	virtual char Updates(char toupdatepanner);
	virtual void syncPanner(int all=0);
	virtual void syncFromPanner(int all=0);
	virtual char onscreen(int x,int y) { return x>=Minx && x<=Maxx && y>=Miny && y<=Maxy; }
	virtual void ShiftScreen(int dx,int dy);
	virtual void ShiftReal(double dx,double dy);
	virtual void Center(double minx,double maxx,double miny,double maxy);
	virtual void CenterPoint(flatpoint p);
	virtual void CenterReal();
	virtual int NewTransform(double *d);
	virtual int NewTransform(double a,double b,double c,double d,double x0,double y0);
	virtual void PushAndNewTransform(double *m);
	virtual void PushAndNewAxes(flatpoint p,flatpoint x,flatpoint y);
	virtual void PushAxes();
	virtual void PopAxes();
	virtual void NewAxis(flatpoint o,flatpoint xtip);
	virtual void NewAxis(flatpoint o,flatvector x,flatvector y);
	virtual void Newangle(double angle,int dir=0,int dec=1);
	virtual void Rotate(double angle,int x,int y,int dec=1);
	virtual void Newmag(double xs,double ys=-1)/*ys=-1*/;
	virtual void Zoomr(double m,flatpoint p);
	virtual void Zoom(double m,int x,int y);
	virtual void Zoom(double m);
	virtual double GetMag(int c=0);
	virtual double GetVMag(int x,int y);
	virtual void SetView(double minx,double maxx,double miny,double maxy);
	virtual int RealSpace(double minx,double maxx,double miny,double maxy);
	virtual void RealSpace(double *minx,double *maxx,double *miny, double *maxy);
	virtual void getTransformedSpace(long *minx,long *maxx,long *miny,long *maxy);
	virtual void WrapWindow(anXWindow *nw);
};

} // namespace Laxkit

#endif
