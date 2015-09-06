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
//    Copyright (C) 2013-2014 by Tom Lechner
//

#include <lax/interfaces/curvewindow.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/bezutils.h>

#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxFiles;


namespace LaxInterfaces {


//-------------------------------- CurveWindow ----------------------------------
/*! \class CurveWindow
 * Window to edit curves that map one range, one-to-one, to another range.
 * 
 * This class just wraps a CurveMapInterface to a window.
 */

CurveWindow::CurveWindow(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder, 
						anXWindow *prev, unsigned long nowner, const char *nsend,
						const char *nctitle,
						const char *xl, double nxmin, double nxmax,
						const char *yl, double nymin, double nymax)
  : anXWindow(pwindow,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER, nx,ny, nw,nh,brder, prev,nowner,nsend),
	interface(-1, GetDefaultDisplayer(), nctitle, xl, nxmin, nxmax, yl, nymin, nymax)
{
}

CurveWindow::~CurveWindow()
{
}

int CurveWindow::init()
{
	SetupRect();
	return 0;
}

//! Set values in rect (the rectangle that the actual curve is drawn in) to be reasonable.
void CurveWindow::SetupRect()
{
	Displayer *dp=GetDefaultDisplayer();
	int th=dp->textheight();

	interface.SetupRect(th/2,th/2,win_w-th,win_h-th);
	needtodraw=1;
}

double CurveWindow::f(double x)
{
	return interface.f(x);
}

int CurveWindow::Event(const EventData *e,const char *mes)
{
    if (e->type==LAX_onMouseOut) {
		interface.Event(e,mes);
		needtodraw=1;
	}

	return anXWindow::Event(e,mes);
}

/*! Adds reference to info.
 */
int CurveWindow::SetInfo(CurveInfo *info)
{
	return interface.SetInfo(info);
}

/*! This basically does *curveinfo=*info, making sure curveinfo!=NULL first.
 */
int CurveWindow::CopyInfo(CurveInfo *info)
{
	return interface.CopyInfo(info);
}

//! Make any CurveWindowEditable masked in editable editable (on==1) or not (on==0).
void CurveWindow::ChangeEditable(unsigned int which, int on)
{
	interface.ChangeEditable(which,on);
}

void CurveWindow::Reset()
{
	interface.Reset();
	needtodraw=1;
}


int CurveWindow::MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue)
{
	return interface.MakeLookupTable(table,numentries,minvalue,maxvalue);
}

int CurveWindow::AddPoint(double x,double y)
{
	interface.AddPoint(x,y);
	needtodraw=1;
	return 0;
}

int CurveWindow::MovePoint(int index, double x,double y)
{
	interface.MovePoint(index,x,y);
	needtodraw=1;
	return 0;
}


/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *CurveWindow::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{ //***
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void CurveWindow::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{ //***
	anXWindow::dump_in_atts(att,flag,context);
}

void CurveWindow::Refresh()
{
	if (!win_on) return;
	if (!needtodraw && !interface.needtodraw) return;

	MakeCurrent();
	interface.Needtodraw(1);
	interface.Refresh();
	needtodraw=0;
	SwapBuffers();
	return;
}

int CurveWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	int status=interface.LBDown(x,y,state,count,d);
	needtodraw|=interface.needtodraw;
	return status;
}

int CurveWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	int status=interface.LBUp(x,y,state,d);
	needtodraw|=interface.needtodraw;
	return status;
}

int CurveWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *m)
{
	int status=interface.MouseMove(x,y,state,m);
	needtodraw|=interface.needtodraw;
	return status;
}

int CurveWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	int status=interface.WheelUp(x,y,state,count,d);
	needtodraw|=interface.needtodraw;
	return status;
}

int CurveWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	int status=interface.WheelDown(x,y,state,count,d);
	needtodraw|=interface.needtodraw;
	return status;
}

int CurveWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	int status=interface.CharInput(ch,buffer,len,state,d);
	needtodraw|=interface.needtodraw;
	if (status!=0) return anXWindow::CharInput(ch,buffer,len,state,d);
	return status;

}

int CurveWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	SetupRect();
	return 0;
}

int CurveWindow::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	SetupRect();
	return 0;
}


} // namespace LaxInterfaces

