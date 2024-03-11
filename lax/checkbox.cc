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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//


#include <lax/checkbox.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>

#include <cmath>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {


/*! \class CheckBox
 * \brief Class for a single item checkbox.
 *
 * Essentially, this is a toggle textbutton.
 *
 * <pre>
 *  state==LAX_ON selected, LAX_OFF==not selected
 *  circle button
 *  square x button
 *  square check button
 *  square but botton
 * 
 *  LEFT:   | O blah        |
 *  RIGHT:  |        blah O |
 *
 * </pre>
 * \code
 *  //circle and squares automatically set Toggle, overriding style passed to constructor
 * #define CHECK_SQUARE_CHECK  (1<<18)
 * #define CHECK_SQUARE_BUT    (1<<19)
 * #define CHECK_SQUARE_X      (1<<20)
 * #define CHECK_CIRCLE        (1<<21)
 * 
 * #define CHECK_LEFT          (1<<22)
 * #define CHECK_RIGHT         (1<<23)
 * #define CHECK_CENTER        (1<<24)
 * \endcode
 */



/*! \todo *** clear up setting initial size to be max of graphic height and the text height
 */
CheckBox::CheckBox(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						const char *nnme,int npad,int ngap)
		: Button(parnt,nname,ntitle,nstyle,
				 xx,yy,ww,hh,brder,
				 prev,nowner,nsendmes,
				 0,
				 nnme,
				 NULL,NULL,npad,ngap)
{
	win_style=(win_style&~BUTTON_MOMENTARY)|BUTTON_TOGGLE;

	InstallColors(THEME_Panel);

	 //*** maybe include bevel just for easier levelling with buttons??
	trect.width = (unsigned int) (UIScale() * win_themestyle->normal->Extent(label,-1));
	grect.y = trect.y = pad;
	grect.height = trect.height = win_h-2*pad;
	grect.width = UIScale() * win_themestyle->normal->textheight(); //grect.height;
	state = LAX_OFF;

	ScreenColor pit;
	coloravg(&pit, &win_themestyle->fg, &win_themestyle->bg, 1.2);
	pit.Clamp();
	pitcolor = pit.Pixel();
	
	if (hh<2) { // wrap window to v textextent
		win_h = UIScale() * (2 * pad + win_themestyle->normal->textheight());
	}
	if (ww<2) { // wrap window to h textextent
		win_w = grect.height + 3 * pad + trect.width;
	}
	setPlacement();


	DBG cerr << win_name<<": "<<(label?label:"(no label)")<<",  x,y:"<<win_x<<','<<win_y<<"  w,h:"<<win_w<<','<<win_h<<endl;
}

//! Empty virtual destructor
CheckBox::~CheckBox() {}

void CheckBox::UIScaleChanged()
{
	trect.width = (unsigned int) (UIScale() * win_themestyle->normal->Extent(label,-1));
	trect.height = grect.height = grect.width = UIScale() * win_themestyle->normal->textheight();
}

bool CheckBox::Checked()
{
	return State() == LAX_ON;
}

bool CheckBox::Checked(bool yes)
{
	State(yes ? LAX_ON : LAX_OFF);
	return State();
}

//! Sync up the placement of things.
void CheckBox::setPlacement()
{
	double scale = UIScale();

	grect.y = win_h/2 - grect.height/2;
	trect.y = win_h/2 - trect.height/2;

	if (win_style&CHECK_RIGHT) {
		grect.x = win_w   - scale * pad - grect.width;
		trect.x = grect.x - scale * pad - trect.width;
	} else { //if (win_style&CHECK_LEFT) {
		grect.x = scale * pad;
		trect.x = scale * pad + grect.width + scale * pad*.6;
	}

	DBG cerr <<"grect: "<<grect.x<<","<<grect.y<<", "<<grect.width<<","<<grect.height<<endl;
	DBG cerr <<"trect: "<<trect.x<<","<<trect.y<<", "<<trect.width<<","<<trect.height<<endl;
}

const char *CheckBox::Label(const char *nlabel)
{
	const char *ret = Button::Label(nlabel);
	setPlacement();
	return ret;
}


void CheckBox::drawgraphic()
{
	Displayer *dp=GetDisplayer();
	dp->LineWidthScreen(1);

	// outer circle
	dp->NewFG(pitcolor);
	dp->drawellipse(grect.x + grect.width/2, grect.y + grect.height/2, 
					grect.width/2, grect.height/2, 0,0, 1);
	
	// inner circle
	dp->NewFG(Grayed() ? coloravg(win_themestyle->fg,win_themestyle->bg) : win_themestyle->fg);
	if (state==LAX_ON) 
		dp->drawellipse(grect.x + grect.width/2, grect.y + grect.height/2,
						grect.width/4, grect.height/4, 0,0, 1);


	dp->NewFG(shadow);
	dp->drawellipse(grect.x + grect.width/2, grect.y + grect.height/2, 
					grect.width/2, grect.height/2, 0,0, 0);
}


void CheckBox::draw()
{ 
	Displayer *dp = MakeCurrent();
	dp->font(win_themestyle->normal, UIScale() * win_themestyle->normal->textheight());
	dp->NewFG(mousein ? win_themestyle->bghover : win_themestyle->bg);

	dp->drawrectangle(0,0, win_w,win_h, 1);
	drawgraphic();

	if (!label) return;
	
    dp->NewFG(Grayed() ? coloravg(win_themestyle->fg,win_themestyle->bg) : win_themestyle->fg);

	if (win_style & CHECK_RIGHT)
		dp->textout(trect.x + trect.width, trect.y+trect.height/2, label,strlen(label), LAX_RIGHT|LAX_VCENTER);
	else
		dp->textout(trect.x, trect.y+trect.height/2, label,strlen(label), LAX_LEFT|LAX_VCENTER);
}

int CheckBox::MoveResize(int nx,int ny,int nw,int nh)
{   
    anXWindow::MoveResize(nx,ny,nw,nh);
    setPlacement();
    needtodraw=1;
    return 0;
}   
    
int CheckBox::Resize(int nw,int nh)
{   
    anXWindow::Resize(nw,nh);
    setPlacement();
    needtodraw=1;
    return 0;
}   

} // namespace Laxkit

