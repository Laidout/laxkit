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
//    Copyright (C) 2004-2011 by Tom Lechner
//

#include <lax/rulerwin.h>
#include <lax/laxutils.h>
#include <lax/menuinfo.h>
#include <lax/popupmenu.h>
#include <lax/units.h>
#include <lax/language.h>

#include <cstring>
#include <iostream>
#include <cmath>
using namespace std;
#define DBG 


using namespace LaxFiles;


namespace Laxkit {


//------------------------------- RulerWindow -------------------------------------
/*! \class RulerWindow
 *
 * \brief A ruler that tracks the mouse.
 *
 * You can use a standard ruler, which draws ticks on 12's and
 * halves, or metric which draws ticks by tens and 5's and 1's.
 * The ruler can be vertical or horizontal, with the ticks on
 * the left, center, or right, and top, center or bottom. 
 * 
 * This class uses GetUnitManager() to coordinate units.
 *
 * Ticks are displayed by order of magnitude grouping. That is, say you have ticks every
 * real unit, and base is 10****
 *
 * Screen Units == mag * (Real Units)\n
 * Current Units == umag * (Base Units)\n
 * Pixel length of the space between smallest drawn divisions: mag*unit/umag/subdiv/subsubdiv\n
 * Pixel length between drawn subdivs: mag*unit/umag/subdiv\n
 * Pixel length between drawn units: mag*unit/umag\n
 *
 * When magnification makes units grow so much that the subsubdivs have enough space
 * for subdiv*subsubdiv number of ticks, then set unit to that smallest space (==unit/subdiv/subsubdiv).
 *
 * \todo ***optionally make vertical writing on the vertical ruler
 * \todo could make ruler automatically track mouse with mouseposition() every 10ms or so
 *      or respond to sent mouse move messages, and mouseposition() to get location...
 * \todo should be able to move position when mouse is in this window!!!
 * \todo placeable range indicators
 * 
 * \code
 * 
 *  #define RULER_X			(1<<16)
 *  #define RULER_Y			(1<<17)
 *  #define RULER_TOPTICKS  (1<<18)
 *  #define RULER_LEFTTICKS (1<<18)
 *  
 *  #define RULER_BOTTOMTICKS (1<<19)
 *  #define RULER_RIGHTTICKS  (1<<19)
 *  
 *  #define RULER_CENTERTICKS (1<<20)
 *  #define RULER_NONUMBERS   (1<<21)
 *  
 *   // defaults to standard,
 *	 // Standard: one basic unit is 1 inch, divided in halves, base=12, 
 *   // Metric is 1 whatever, divided in 10 portions, with a slightly bigger tick for the middle
 *  #define RULER_STANDARD    (1<<22)
 *  #define RULER_METRIC      (1<<23)
 *  
 * 	 // send message when button down, and mouse leaves ruler
 *  #define RULER_SENDONLBD (1<<24)
 *  #define RULER_SENDONMBD (1<<25)
 *  #define RULER_SENDONRBD (1<<26)
 *
 *  #define RULER_UP_IS_POSITIVE  (1<<27)
 * \endcode
 */
/*! \var double RulerWindow::base
 * \brief The base of the numbering. 12 for standard, 10 for metric
 *
 * Units are grouped according to base in that there will be one chuck for every
 *
 * It's usually a good idea for the base to be such 
 * that base/subdiv/subsubdiv is an integer.
 */
/*! \var int RulerWindow::subdiv
 * \brief The main subdivision of the current unit.
 *
 * When subdiv is 2, for instance, then the base unit gets divided
 * into 2 sections, with a tick of middle size placed in the middle.
 *
 * Standard and metric both set this to 2.
 */
/*! \var int RulerWindow::subsubdiv
 * \brief The divisions of the subdivisions.
 *
 * Each subdivision is itself divided into this many sections. The smallest
 * ticks demarcate the sections.
 *
 * Standard has 4 subsubdivs, and metric has 5.
 */
/*! \var int RulerWindow::tf
 * \brief The fraction of the window taken up by unit ticks.
 *
 * Defaults to 1.0.
 */
/*! \var int RulerWindow::stf
 * \brief The fraction of the window taken up by sub-unit ticks.
 *
 * Defaults to .75.
 */
/*! \var int RulerWindow::sstf
 * \brief The fraction of the window taken up by sub-sub-unit ticks.
 *
 * Defaults to .5.
 */
/*! \var int RulerWindow::unit
 * \brief The current size of a single unit in real coordinates.
 *
 * This value can change on magnification. It only specifies what real distance
 * is required between the biggest ticks on the ruler.
 * Say mag==1, then a unit=100 means that every 100 pixels is a unit tick.
 * If mag==2, then a unit=100 means (at least) every 200 pixels is a unit tick
 */
/*! \var double RulerWindow::umag
 * \brief Scaling between baseunits and currentunits.
 *
 * currentunits = umag * baseunits
 *
 * So to get screen coordinates for a currentunit of p: s=(p/umag-start)*mag;
 */
/*! \var double RulerWindow::mag
 * \brief The current magnification (real*mag=screen).
 */
/*! \var double RulerWindow::start
 * \brief The real starting point in the window.
 */
/*! \var double RulerWindow::end
 * \brief The real ending point in the window.
 */
/*! \var double RulerWindow::curpos
 * \brief The current position in real coordinates ((real-start)*mag=screen).
 */
/*! \var anXWindow *RulerWindow::trackwindow
 * \brief The window in which mouse movements are tracked.
 */
/*! \var int RulerWindow::screenoffset
 * \brief Distance that the ruler is offset from the trackwindow. (ruler-tracked)
 *
 *	For horizontal rulers: screenoffset = rulerwindow_x - trackwindow_x;<br>
 *	For vertical rulers: screenoffset = rulerwindow_y - trackwindow_y;
 */


/*! If base_units!=NULL, then use those units. By default, known units 
 * are "Inches", "Feet", "cm", "mm", "Meters", "Points", "Pixels", and the default is inches.
 *
 * \todo If syncwithscreen, then based on the given units, try to set up so that units
 *   make sense on the given screen.
 */
RulerWindow::RulerWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsend,
						const char *base_units, int syncwithscreen)
	: anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	if (!(nstyle&(RULER_X|RULER_Y))) win_style|=RULER_X;

	DBG cerr <<"Ruler:";
	DBG if (win_style&RULER_X) cerr<<"X";
	DBG if (win_style&RULER_Y) cerr <<"Y";
	DBG cerr <<endl;

	curpos=0;
	mag=1;
	screenoffset=0;
	start=0;
	end=0;
	needtodraw=1;
	unit=1;
	smallnumbers=app->fontmanager->MakeFont("Sans","",app->defaultlaxfont->textheight()/2,0);
	DBG smallnumbers->suppress_debug=1;
	
		// set fractional width of ticks
//	tf=1.;
//	stf=.75;
//	sstf=.5;
	tf=.75;
	stf=.4;
	sstf=.3;
	
	installColors(app->color_panel);
	numcolor=win_colors->fg;
	tickcolor=win_colors->fg;
	subtickcolor=coloravg(win_colors->fg,win_colors->bg,.3333);
	subsubtickcolor=coloravg(win_colors->fg,win_colors->bg,.6666);
	curposcolor=rgbcolor(0,254,0);

	trackwindow=NULL;

	UnitManager *units=GetUnitManager();
	//baseunits=UNITS_None;
	baseunits=units->DefaultUnits();
	DBG cerr <<"baseunits on ruler creation:"<<baseunits<<endl;

	if (base_units) {
		units->UnitInfo(base_units,&baseunits,NULL,NULL,NULL,NULL);
	}
	if (win_style&RULER_STANDARD || baseunits==UNITS_Inches) {
		base=12;
		subdiv=2;
		subsubdiv=4;
		if (baseunits==UNITS_None) baseunits=UNITS_Inches;
	} else { // default is for metric
		base=10;
		subdiv=2;
		subsubdiv=5;
		if (baseunits==UNITS_None) baseunits=UNITS_CM;
	} 
	currentunits=baseunits;
	umag=1;

	if (syncwithscreen) {
		//***
	}
}

//! Destructor.
RulerWindow::~RulerWindow()
{
	if (smallnumbers) smallnumbers->dec_count();
}

//! Create a pixmap to hold what's behind the moving bar.
int RulerWindow::init()
{
	if (start>=end) end=start+(win_style&RULER_X?win_w:win_h)/mag;
	return 0;
}

//! Ensures that there are enough whole ticks on the screen.
void RulerWindow::adjustmetrics()
{
	while (unit/umag*mag<subdiv*subsubdiv*2) unit*=base;
	while (unit/base/umag*mag>base*2) unit/=base; 
	needtodraw=1;
}

//! Draw the number labels every unit, taking steps to not overlap labels
/*! textpos is the last screen coordinate drawn to by a drawtext() call. 
 *
 * \todo *** this could use some work, particularly, vertical rulers should employ vertical text
 */
void RulerWindow::drawtext(double n,int pos,int &textpos,int toff) // n is real, textpos=minimum screenpos rel to ruler win_w
{
	char thetext[20];
	if (fabs(n)<1e-7) n=0;
	sprintf(thetext,"%.4g",((win_style&RULER_UP_IS_POSITIVE)?-n:n));
	double adv,ex,ey,asc,des;
	adv=getextent(smallnumbers, thetext,strlen(thetext),&ex,&ey,&asc,&des,0);

	if (win_style&RULER_X) {
		//if (pos<textpos || pos+3+adv<textpos || pos>((floor(n/unit)*unit+unit)-ustart)*umag*mag) { 
					// text must fit between textpos and next unit start, else return
		if (pos-3<textpos) { 
			return;		
		}
		if (win_style&RULER_TOPTICKS) toff=win_h-des-2; else toff=2+asc;
		textout(this, smallnumbers, thetext,strlen(thetext), pos+3,toff, LAX_LEFT|LAX_BASELINE);
		textpos=pos+adv;
	} else {
		 //*** Draw vertically: needs work, gotta rotate the text!!!
		if (pos<textpos || pos+3+(des+asc)<textpos
				//|| pos>((floor(n/unit)*unit+unit)-ustart)*umag*mag) { 
				) { 
					// text must fit between textpos and next unit start, else return
			return;		
		}
		//textout_rotated(this, smallnumbers, 90, thetext,strlen(thetext), 0,pos+asc,LAX_LEFT|LAX_BASELINE);
		textout(this, smallnumbers, thetext,strlen(thetext), 0,pos+asc,LAX_LEFT|LAX_BASELINE);
		textpos+=des+asc;
	}
}

//! Refresh.
/*! If needtodraw&1, then do the whole shebang, otherwise if needtodraw&2
 *  the just need to reposition the bar.
 */
void RulerWindow::Refresh()
{
	if (!win_on || !needtodraw) return;

	MakeCurrent();

	drawing_function(LAXOP_Source);
	drawing_line_attributes(0,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);

	//DBG cerr << "*******************rulerwin, winw/h: "<<win_w<<','<<win_h<<endl;

	 // draw all
	if (needtodraw&1) {
		//clear_window(this);
		foreground_color(win_colors->bg);
		fill_rectangle(this,0,0,win_w,win_h);

		double ustart=start*umag; //start and end in current units, rather than base units
		double uend  =  end*umag;
		double uunit=1;

		double p=0,pos,pos2, x;
		int c,c2;
		int toff,stoff,sstoff,tw,stw,sstw,textpos=0;
		double t, // t is total height (parallel to ticks), 
			   w; // w is window length perpendicular to ticks in real coords
		if (win_style&RULER_X) { 
			t=(double)win_h;
			w=(double)win_w/mag*umag; //so w is in real current units
		} else {
			t=(double)win_w;
			w=(double)win_h/mag*umag;
		}

		 // Find the tick heights and offsets
		tw=(int)(t*tf);
		stw=(int)(t*stf);
		sstw=(int)(t*sstf);
		//DBG cerr << "*******************rulerwin sstf, t, sstw: "<<sstf<<','<<t<<','<<sstw<<endl;

		if (win_style&RULER_TOPTICKS) {
			toff=stoff=sstoff=0;
		} else if (win_style&RULER_CENTERTICKS) {
			toff=((int)t-tw)/2;
			stoff=((int)t-stw)/2;
			sstoff=((int)t-sstw)/2;
		} else { // bottomticks or rightticks
			toff=((int)t-tw);
			stoff=((int)t-stw);
			sstoff=((int)t-sstw);
		}
		
		 // If the ticks are too close or too far apart, then adjust them accordingly
		 //
		 //     (base*subdiv*subsubdiv)*2 == num ticks for unit group +all subticks
		 //
		if (unit/umag*mag<subdiv*subsubdiv*2 ||  // (group of units fits within window ||
				unit/umag*mag>w ||  //  whole unit is bigger than window ||
				unit/subdiv/subsubdiv/umag*mag>(base*subdiv*subsubdiv)*2) // space between smallest tick can
																	// contain full range of ticks
			adjustmetrics();

		uunit=unit;
		//DBG cerr <<"ustart:"<<ustart<<"  uend:"<<uend<<"  uunit: "<<uunit<<endl;
		//DBG cerr << " subtickwidth="<<(uunit/umag/(double)subdiv*mag)<<"  subdiv,subsubdiv="
		//DBG   <<","<<subdiv<<","<<subsubdiv<<"  mag="<<mag<<"  umag="<<umag<<endl;

		 // Draw the ticks and labels
		for (p=floor(ustart/uunit)*uunit; p<uend; p+=uunit) {

			if (uunit/subdiv/umag*mag>4) {
				//DBG cerr <<"|";
				 // Draw subdivisions
				pos=p;
				for (c=0; c<subdiv; c++) {
					if (uunit/subdiv/subsubdiv/umag*mag>4) { // make sure the divisions aren't too close together
						pos2=pos;
						foreground_color(subsubtickcolor);
						 
						 // Draw subsubdivisions (smallest ticks)
						for (c2=1; c2<subsubdiv; c2++) {
							pos2+=uunit/subdiv/subsubdiv;
							x=(pos2-ustart)/umag*mag;
							if (win_style&RULER_X) 
								draw_line(this, (int)x,sstoff, (int)x,sstoff+sstw);
							else draw_line(this, sstoff,(int)x, sstoff+sstw,(int)x);
							//DBG cerr <<"'";						
						}
					}
					foreground_color(subtickcolor);
					pos+=uunit/subdiv;

					 //draw medium ticks
					if (c<subdiv-1) {
						x=(pos-ustart)/umag*mag;
						if (win_style&RULER_X) 
							draw_line(this, (int)x,stoff, (int)x,stoff+stw);
						else draw_line(this, stoff,(int)x, stoff+stw,(int)x);
					}
					//DBG cerr <<"=";
				}
			}

			 //draw big tick
			foreground_color(tickcolor);
			x=(p-ustart)/umag*mag;

			//DBG cerr <<"--- big tick for "<<p<<" at screen pos "<<x<<endl;
			//DBG cerr << "*******************rulerwin, line: "<<x<<','<<toff<<','<<toff+tw<<endl;

			if (win_style&RULER_X) {
				draw_line(this, (int)x,toff, (int)x,toff+tw);
			} else {
				draw_line(this, toff,(int)x, toff+tw,(int)x);
			}

			 //draw numbers next to big ticks
			drawtext(p,x,textpos,toff); // big tick number
		}
	}
	
	 // Draw mouse indicator lines, one line per identified mouse
	foreground_color(curposcolor);
	int x,y;
	unsigned int mask;
	LaxMouse *mouse;
	for (int c=0; c<app->devicemanager->NumDevices(); c++) {
		mouse=dynamic_cast<LaxMouse*>(app->devicemanager->Device(c));
		if (!mouse) continue;
		if (mouseposition(mouse->id, this, &x,&y, &mask, NULL)==0) {
			if (win_style&RULER_X) {
				draw_line(this, x,0, x,win_h);
			} else {
				draw_line(this, 0,y, win_w,y);
			}
		}
	}

	SwapBuffers();
	needtodraw=0;
	//DBG cerr <<endl;
}

//! Automatically set screenoffset based on win.
/*! Note that this does not set magnification, start or end.
 */
void RulerWindow::TrackThisWindow(anXWindow *win)
{
	trackwindow=win; // Yes, this goes before checking for win==NULL
	if (!win) return;

	int x,y, xx,yy;
	translate_window_coordinates(this,0,0, NULL, &x, &y, NULL);
	translate_window_coordinates(win, 0,0, NULL,&xx,&yy, NULL);

	if (win_style&RULER_X) screenoffset=x - xx;
	else screenoffset=y - yy;
}

//! Set pos from where the mouse is semi-manually, by querying the pointer(s).
/*! \todo *** this is unimplemented
 */
void RulerWindow::Track()
{
	if (!trackwindow) return;
	int x,y;
	unsigned int mask;
	LaxMouse *mouse;
	for (int c=0; c<app->devicemanager->NumDevices(); c++) {
		mouse=dynamic_cast<LaxMouse*>(app->devicemanager->Device(c));
		if (!mouse) continue;
		if (mouseposition(mouse->id, trackwindow, &x,&y, &mask, NULL)==0) {
			//SetPos rx-win_x,ry-win_y???*****
		} else {
			// Pointer is not on same screen as trackwindow
		}
	}
}

//
//void RulerWindow::Zoom(double f,int x) // screen resolution x remains at same point
//{
//	double pos=***pos of x
//	mag*=f;
//	start=***computer from pos
//}
//
//void RulerWindow::Zoomr(double f,double x)
//{
//	mag*=f;
//	***
//}

//! mag*=f, keeps start at the same position.
void RulerWindow::Zoom(double f) 
{
	mag*=f;
	end=start+((win_style&RULER_X)?win_w:win_h)/mag;
	needtodraw=1;
}

//! Set magnification, (screen) = mag * (real)
/*! Start remains the same value after the new magnification.
 */
void RulerWindow::SetMag(double nmag) // keeps start at same pos
{
	mag=nmag;
	end=start+((win_style&RULER_X)?win_w:win_h)/mag;
	needtodraw=1;
}

//! Set position based on whole screen coordinate (x,y).
void RulerWindow::SetPosFromScreenCoord(int x,int y)
{
	int p;
	if (win_style&RULER_X) p=x-win_x+screenoffset;
	 else p=y-win_y+screenoffset;
	SetPos(start+p/mag);
}

//! Set position based on screen coordinate p, rulerwindow coordinates.
/*! See also screenoffset.
 */
void RulerWindow::SetPosFromScreen(int p)
{
	p+=screenoffset;
	SetPos(start+p/mag);
}

//! Set the ruler position to be real number crpops.
void RulerWindow::SetPos(double crpos)
{ // *** curpos is not currently used
	if (crpos==curpos) return;
	if (curpos>start && curpos<end) needtodraw=1;
	curpos=crpos;
	if (curpos>start && curpos<end) needtodraw=1;
}

//! Set start=str, and compute end based on new start and mag.
/*! Does not change screenoffset.
 */
void RulerWindow::Set(double strt) // adjusts for screenoffset
{
	start=strt-screenoffset/mag;
	end=start+((win_style&RULER_X)?win_w:win_h)/mag;
	needtodraw=1;
}

//! Set start=strt, and end=fin, mag is calculated from that.
/*! Does not change screenoffset.
 * \todo *** does not adjust for screenoffset ***needs work
 */
void RulerWindow::Set(double strt,double fin) 
{
	start=strt;
	end=fin;
	mag=((win_style&RULER_X)?win_w:win_h)/(fin-start);
	needtodraw=1;
}

//! Set the origin with position o, assuming o is in window coordinates of trackwindow.
void RulerWindow::SetOrigin(int o)
{
	o-=screenoffset;
	start=-o/mag;
	end=start+(win_style&RULER_X?win_w:win_h)/mag;
	needtodraw=1;
}

//! Add a range to indicate within the ruler. UNIMPLEMENTED!!
/*! \todo do me!
 *
 * AddRange() establishes, RemoveRange() removes.. maybe optionally mouse over to show
 * bounds in ruler... maybe also set range color..
 */
void RulerWindow::AddRange(int id, double start, double end)
{
	cerr <<" *** Must implement RulerWindow::AddRange()!!"<<endl;
}

int RulerWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	return 0; //0 indicates that we intend to do something with the RBUp
}

//! By default use GetUnitManager() to find a units manager.
int RulerWindow::NumberOfUnits()
{
	UnitManager *units=GetUnitManager();
	return units->NumberOfUnits();
}

//! Return positive for info found, or 0 for not found.
int RulerWindow::UnitInfo(int index, const char **name, int *id, double *scale, int *sdiv, int *ssdiv)
{
	if (index<0 || index>=NumberOfUnits()) return 0;

	int iid;
	char *nm;
	UnitManager *units=GetUnitManager();
	units->UnitInfoIndex(index, &iid, scale, NULL, NULL, &nm);

	if (name)   *name=nm;
	if (id)       *id=iid;
	if (sdiv)   *sdiv=2;
	if (ssdiv) *ssdiv=(iid==UNITS_Feet || iid==UNITS_Inches ? 4 : 5 );

	return 1;
}

//! Call up a menu to choose units from.
int RulerWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!(win_style&RULER_UNITS_MENU)) return 0;

	MenuInfo *menu=new MenuInfo(_("Units"));

	const char *name;
	int id;
	for (int c=0; c<NumberOfUnits(); c++) {
		UnitInfo(c, &name, &id, NULL,NULL,NULL);
		//menu->AddItem(name, id, id==units->defaultunits?LAX_CHECKED|LAX_ISTOGGLE:0);
		menu->AddItem(name, id, (id==currentunits?LAX_CHECKED:0)|LAX_ISTOGGLE, 0);
	}

	PopupMenu *popup;
	popup=new PopupMenu(menu->title,
						menu->title,
						0,
						0,0,0,0, 1, 
						object_id,"units", 
						d->id,
						menu,1,
						NULL,
						MENUSEL_LEFT|MENUSEL_GRAPHIC_ON_LEFT);
	popup->WrapToMouse(d->id);
	app->rundialog(popup);

	return 0;
}

int RulerWindow::Event(const EventData *e,const char *mes)
{
	if (strcmp(mes,"units")) return anXWindow::Event(e,mes);

	const SimpleMessage *m=dynamic_cast<const SimpleMessage*>(e);
	DBG cerr <<"----RulerWindow got units event, select:"<< m->info1<<endl;

	int item_index= m->info1;
	int item_id   = m->info2;

	if (item_index<0 && item_index>=NumberOfUnits()) return 0;


	const char *name;
	int id, sdiv,ssdiv;
	double scale;

	for (int c=0; c<NumberOfUnits(); c++) {
		UnitInfo(c, &name, &id, &scale, &sdiv, &ssdiv);
		if (item_id==id) { 
			SetCurrentUnits(name);
			if (win_owner) {
				SimpleMessage *ev=new SimpleMessage;
				ev->info1=RULER_Units;
				ev->info2=currentunits;
				app->SendMessage(ev,win_owner,win_sendthis,object_id);
			}
			break;
		}
	}

	/// *** need to send units change message to parent

	needtodraw=1;
	return 0;
}


//! Calls to the various SetPos() and Set() commands are assumed to be in base units, but are displayed in current units.
/*! Return 0 for success, nonzero for error.
 *
 * Current units are not changed.
 */
int RulerWindow::SetBaseUnits(int units)
{
	UnitManager *u=GetUnitManager();
	if (u->UnitInfoId(units,NULL,NULL,NULL,NULL)==0) {
		baseunits=units;
		umag=u->GetFactor(baseunits,currentunits);
		needtodraw=1;
		return 0;
	}
	return 1;
}

int RulerWindow::SetBaseUnits(const char *units)
{
	UnitManager *u=GetUnitManager();
	int id;
	if (u->UnitInfo(units,&id,NULL,NULL,NULL,NULL)==0) {
		baseunits=id;
		umag=u->GetFactor(baseunits,currentunits);
		needtodraw=1;
		return 0;
	}
	return 1;
}

//! Calls to the various SetPos() and Set() commands are assumed to be in base units, but are displayed in current units.
/*! Return 0 for success or nonzero for units not found.
 */
int RulerWindow::SetCurrentUnits(const char *units)
{
	UnitManager *u=GetUnitManager();
	int id;
	if (u->UnitInfo(units,&id,NULL,NULL,NULL,NULL)==0 && id!=currentunits)
		return SetCurrentUnits(id);
	return 1;
}

/*! Return 0 for success or nonzero for units not found.
 */
int RulerWindow::SetCurrentUnits(int id)
{
	if (id==currentunits) return 0;

	UnitManager *u=GetUnitManager();
	if (u->UnitInfoId(id,NULL,NULL,NULL,NULL)==0) {
		currentunits=id;
		if (id==UNITS_Feet || id==UNITS_Inches) {
			subdiv=2;
			subsubdiv=4;
			base=12;
		} else {
			subdiv=2;
			subsubdiv=5;
			base=10;
		}
		umag=GetUnitManager()->GetFactor(baseunits,currentunits);
		unit=1;
		adjustmetrics();
		DBG cerr <<" set current units to "<<currentunits<<", umag="<<umag<<endl;
		needtodraw=1;
		return 0;
	}
	return 1;
}

//! Set the number of subdivisions and subsubdivions for the current units.
int RulerWindow::SetDivisions(int sdiv, int ssdiv)
{
	subdiv=sdiv;
	subsubdiv=ssdiv;
	needtodraw=1;
	return 0;
}

int RulerWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{	needtodraw=1;
	return 0;
}

//int RulerWindow::LBUp()
//{***
//}

int RulerWindow::MouseMove (int x,int y,unsigned int state, const LaxMouse *m)
{
	DBG double ustart=start*umag; //start and end in current units, rather than base units
	DBG double uend=end*umag; //start and end in current units, rather than base units
	DBG double pos=((win_style&RULER_X)?x:y)/mag+start;
	DBG double upos=pos*umag;
	DBG cerr <<"move scr:"<<x<<","<<y<<"  default:"<<pos<<"  current:"<<upos<<"  start:"<<start<<"  ustart:"<<ustart<<"  uend:"<<uend<<endl;
	return 0;
}


int RulerWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	//*** should have an extra track here
	anXWindow::MoveResize(nx,ny,nw,nh);
	end=start+(win_style&RULER_X?win_w:win_h)/mag;
	return 0;
}

int RulerWindow::Resize(int nw,int nh)
{
	//*** should have an extra track here
	anXWindow::Resize(nw,nh);
	end=start+(win_style&RULER_X?win_w:win_h)/mag;
	return 0;
}

/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *RulerWindow::dump_out_atts(Attribute *att,int what,anObject *context)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	if (what==-1) {
		att->push("vertical","boolean");
		att->push("horizontal","boolean");
		att->push("base","Base of number system. 12 for standard, 10 for metric, for instance.");
		att->push("subdiv","Divide a unit into this many major divisions");
		att->push("subsubdiv","Divide major divisions of unit into this many minor divisions");
		att->push("units","Name of units, such as inch or mm");
		att->push("tickalign","left or top, right or bottom");
		return att;
	}

	if (win_style&RULER_X) att->push("vertical");
	else att->push("horizontal");

	char buf[30];

	sprintf(buf,"%.10g",base);
	att->push("base",buf);

	sprintf(buf,"%d",subdiv);
	att->push("subdiv",buf);

	sprintf(buf,"%d",subsubdiv);
	att->push("subsubdiv",buf);

	char *unitname=NULL;
	UnitManager *units=GetUnitManager();
	units->UnitInfoId(baseunits,NULL,&unitname,NULL,NULL);
	att->push("units",unitname);

	if (win_style&RULER_X) {
		if (win_style&RULER_TOPTICKS) att->push("tickalign","top");
		else att->push("tickalign","bottom");
	} else {
		if (win_style&RULER_TOPTICKS) att->push("tickalign","left");
		else att->push("tickalign","right");
	}

	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void RulerWindow::dump_in_atts(Attribute *att,int flag,anObject *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"vertical")) {
			setWinStyle(RULER_Y,1);
			setWinStyle(RULER_X,0);

		} else if (!strcmp(name,"horizontal")) {
			setWinStyle(RULER_X,1);
			setWinStyle(RULER_Y,0);

		} else if (!strcmp(name,"subdiv")) {
			IntAttribute(value,&subdiv);

		} else if (!strcmp(name,"subsubdiv")) {
			IntAttribute(value,&subsubdiv);

		} else if (!strcmp(name,"base")) {
			DoubleAttribute(value,&base);

		} else if (!strcmp(name,"units")) {
			SetBaseUnits(value);

		} else if (!strcmp(name,"tickalign")) {
			if (!strcmp(value,"top") || !strcmp(value,"left")) setWinStyle(RULER_TOPTICKS,1);
			else if (!strcmp(value,"bottom") || !strcmp(value,"right")) setWinStyle(RULER_BOTTOMTICKS,1);

		}
	}
}


} // namespace Laxkit

