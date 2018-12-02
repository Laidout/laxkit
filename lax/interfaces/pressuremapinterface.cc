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
//    Copyright (C) 2014 by Tom Lechner
//



#include <lax/interfaces/pressuremapinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/laxutils.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class PressureMapInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


PressureMapInterface::PressureMapInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_style=0;

	linecolor.rgbf(0,0,.5);

	smooth_pixel_threshhold=2;

	showdecs=1;
	needtodraw=1;
	device=0;

	max_ticker=300;
	cur_ticker=0;
	histogram_threshhold=100;
	max_histogram_value=0;
	num_histogram_samples=100;

	for (int c=0; c<num_histogram_samples; c++) { histogram.push(0); }
}

PressureMapInterface::~PressureMapInterface()
{
}

const char *PressureMapInterface::Name()
{ return _("Pressure Mapper"); }


//! Return new PressureMapInterface.
/*! If dup!=NULL and it cannot be cast to ImageInterface, then return NULL.
 */
anInterface *PressureMapInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new PressureMapInterface(NULL,id,NULL);
	else if (!dynamic_cast<PressureMapInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

int PressureMapInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;
	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
	if (ls!=NULL) {
		if (mask&GCForeground) { 
			linecolor=ls->color;
		}
//		if (mask&GCLineWidth) {
//			linecolor.width=ls->width;
//		}
		needtodraw=1;
		return 1;
	}
	return 0;
}

int PressureMapInterface::InterfaceOn()
{ 
	showdecs=1;
	needtodraw=1;
	return 0;
}

int PressureMapInterface::InterfaceOff()
{ 
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void PressureMapInterface::Clear(SomeData *d)
{
	points.flush();
}

Laxkit::MenuInfo *PressureMapInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	return menu;
	//MenuInfo *menu=new MenuInfo;
	//menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path, (freehand_style&FREEHAND_Raw_Path)?LAX_CHECKED:0);
	//menu->AddItem(_("Create simplified polyline"), FREEHAND_Poly_Path, (freehand_style&FREEHAND_Poly_Path)?LAX_CHECKED:0);
	//menu->AddItem(_("Create bezier line"), FREEHAND_Bez_Path, (freehand_style&FREEHAND_Bez_Path)?LAX_CHECKED:0);
	//menu->AddItem(_("Create bezier outline"), FREEHAND_Bez_Outline, (freehand_style&FREEHAND_Bez_Outline)?LAX_CHECKED:0);
	//menu->AddItem(_("Create mesh"), FREEHAND_Mesh, (freehand_style&FREEHAND_Mesh)?LAX_CHECKED:0);
	//return menu;
}



int PressureMapInterface::Refresh()
{ 
	//DBG cerr <<"  PressureMap trying to startdrawing"<<endl;

	if (needtodraw==0) return 0;
	needtodraw=0;

	if (!device) return 0;

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->DrawScreen();

	double x,y,w,h;
	flatpoint p;


	 //draw device name
	//dp->NewFG(curwindow->win_colors->fg);
	//dp->textout((dp->Maxx+dp->Minx)/2,5, device_name.c_str(),-1, LAX_TOP|LAX_HCENTER);

	 //draw scratch area
	//***

	 //draw histogram
	w=(dp->Maxx-dp->Minx)*.8;
	h=(dp->Maxy-dp->Miny)/2;
	if (w<h) h=w; else w=h;
	x=(dp->Maxx+dp->Minx)/2-w/2;
	y=(dp->Maxy+dp->Miny)/2+h/2;
	double sw=w/num_histogram_samples;
	double sh;
	int i;

	dp->NewFG(coloravg(rgbcolorf(0.,0.,1.),curwindow->win_colors->bg));
	dp->drawrectangle(x,y,w,-h,0);

	dp->NewFG(coloravg(curwindow->win_colors->fg,curwindow->win_colors->bg));
	for (int c=0; c<num_histogram_samples; c++) {
		i=histogram.e[c];

		if (max_histogram_value>histogram_threshhold) {
			sh=h*i/(double)max_histogram_value;
		} else {
			sh=h*i/histogram_threshhold;
		}

		dp->drawrectangle(x+c/(double)num_histogram_samples*w,y, sw,-sh, 1);
	}

	 //draw curve
	//***

	 //draw ticker
	w=(dp->Maxx-dp->Minx)*.8;
	h=(dp->Maxy-dp->Miny)/4;
	x=(dp->Maxx+dp->Minx)/2-w/2;
	y=dp->Miny+(dp->Maxy-dp->Miny)*.75;
	
	dp->NewFG(coloravg(rgbcolorf(0.,0.,1.),curwindow->win_colors->bg));
	dp->drawrectangle(x,y,w,h,0);

	dp->NewFG(0.,0.,1.);

	for (int c=0; c<pticker.n; c++) {
		p.x = x + w*c/(double)max_ticker;
		p.y = y + h - pticker.e[c]*h;

		dp->lineto(p);
	}
	dp->stroke(0);


	dp->DrawReal();

	//DBG cerr <<"   PressureMap all done drawing.\n";
	return 0;
}

//! Start a new freehand line.
int PressureMapInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	buttondown.down(d->id,LEFTBUTTON,x,y);
	device=d->subid; //normal id is the core mouse, not the controlling sub device
	DBG cerr <<"device: "<<d->id<<"  subdevice: "<<d->subid<<endl;
	//LaxDevice *dv=app->devicemanager->findDevice(device);
	//device_name=dv->name;
	needtodraw=1;
	return 0;
}

//! Finish a new freehand line by calling newData with it.
int PressureMapInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}

/*! \todo *** this isn't very sophisticated, for elegance, should use some kind of 
 * bez curve fitting to cut down on unnecessary points should use a timer so 
 * stopping makes sharp corners and closer spaced points?
 */
int PressureMapInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) return 1;

	double pressure, tiltx,tilty;
	const_cast<LaxMouse*>(d)->getInfo(NULL,NULL,NULL,NULL,NULL,NULL,&pressure,&tiltx,&tilty,NULL);

	int i=pressure*100;
	if (i<0) i=0;
	else if (i>=100) i=99;
	histogram.e[i]++;
	if (histogram.e[i]>max_histogram_value) max_histogram_value=histogram.e[i];

	if (pticker.n==max_ticker) {
		pticker.e[cur_ticker]=pressure;
		cur_ticker++;
		if (cur_ticker>max_ticker) cur_ticker=0;
	} else {
		pticker.push(pressure);
		cur_ticker++;
		if (cur_ticker==max_ticker) cur_ticker=0;
	}

	DBG cerr <<"pressure:" <<pressure<<endl;

	needtodraw=1;
	return 0;
}


int PressureMapInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"PressureMapInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int PressureMapInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if (ch==' ') {
		for (int c=0; c<histogram.n; c++) histogram.e[c]=0;
		max_histogram_value=0;
		needtodraw=1;
		return 0;
	}

	return 1;
}

} // namespace LaxInterfaces

