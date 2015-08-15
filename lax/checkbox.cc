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
 * To have a list of possibly mutually exclusive check boxes use MenuSelector.
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
 *  CENTERL:|    blah O     |
 *  CENTERR:|    O blah     |
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
 * #define CHECK_CENTERL       (1<<25)
 * #define CHECK_CENTERR       (1<<26)
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

	 //*** maybe include bevel just for easier levelling with buttons??
	trect.width=(unsigned int) getextent(label,-1, NULL,NULL);
	grect.y=trect.y=pad;
	grect.height=trect.height=win_h-2*pad;
	grect.width=grect.height;
	state=LAX_OFF;

	pitcolor=app->color_edits->bg;
	
	if (hh<2) { // wrap window to v textextent
		win_h=2*pad+app->defaultlaxfont->textheight();
	}
	if (ww<2) { // wrap window to h textextent
		win_w=grect.height + 3*pad + trect.width;
	}
	setPlacement();

	if (win_colors) win_colors->dec_count();
	win_colors=app->color_panel;
	win_colors->inc_count();


	DBG cerr << win_name<<": "<<(label?label:"(no label)")<<",  x,y:"<<win_x<<','<<win_y<<"  w,h:"<<win_w<<','<<win_h<<endl;
}

//! Empty virtual destructor
CheckBox::~CheckBox() {}

//! Sync up the placement of things.
void CheckBox::setPlacement()
{
	if (win_style&CHECK_LEFT) {
		grect.x=pad;
		trect.x=pad+grect.width+pad;

	} else if (win_style&CHECK_RIGHT) { 
		grect.x=win_w-pad-grect.width;
		trect.x=grect.x-pad-trect.width;

	} else if (win_style&CHECK_CENTERR) { 
		grect.x=win_w/2-(grect.width+pad+trect.width)/2;
		trect.x=grect.x+grect.width+pad;

	} else { // default CENTERL
		trect.x=win_w/2-(grect.width+pad+trect.width)/2;
		grect.x=trect.x+trect.width+pad;
	}

	DBG cerr <<"grect: "<<grect.x<<","<<grect.y<<", "<<grect.width<<","<<grect.height<<endl;
	DBG cerr <<"trect: "<<trect.x<<","<<trect.y<<", "<<trect.width<<","<<trect.height<<endl;
}

const char *CheckBox::Label(const char *nlabel)
{//***must determine new grect
	return Button::Label(nlabel);	
}

/*! \todo implement other than circle
 */
void CheckBox::drawgraphic()
{//***
//	if (win_style&CHECK_CIRCLE) {
		 //whole graphic area
		foreground_color(pitcolor);
		fill_arc_wh(this, grect.x,grect.y, grect.width,grect.height, 0,0);
		
		 //inner circle
		foreground_color(win_colors->fg);
		if (state==LAX_ON) fill_arc_wh(this, grect.x+grect.width/4,grect.y+grect.height/4, grect.width/2,grect.height/2, 0,0);

		foreground_color(highlight);
		//draw_arc_wh(this, grect.x,grect.y, grect.width,grect.height, 0,M_PI);
		//if (state==LAX_ON) draw_arc_wh(this, grect.x+grect.width/4,grect.y+grect.height/4, grect.width/2,grect.height/2,
									//(360+20)*M_PI/180,200*M_PI/180);

		foreground_color(shadow);
		draw_arc_wh(this, grect.x,grect.y, grect.width,grect.height, 0,0);
		//draw_arc_wh(this, grect.x,grect.y, grect.width,grect.height, M_PI,2*M_PI);
		//if (state==LAX_ON) draw_arc_wh(this, grect.x+grect.width/4,grect.y+grect.height/4, grect.width/2,grect.height/2,
									//200*M_PI/180,(360+20)*M_PI/180);
//	} else if (***) {
//	}
}

void CheckBox::draw()
{ 
	Displayer *dp=MakeCurrent();
    dp->NewFG(mousein ? win_colors->moverbg : win_colors->bg);

	dp->drawrectangle(0,0, win_w,win_h, 1);
	drawgraphic();

	if (!label) return;
	
	//double ex,ey,fasc,fdes;
	//dp->textextent(label,-1,&ex,&ey,&fasc,&fdes);
	//getextent(label,-1,&ex,&ey,&fasc,&fdes);
	
    dp->NewFG(win_colors->fg);
	dp->textout(trect.x+trect.width/2,trect.y+trect.height/2, label,strlen(label), LAX_CENTER);

	//drawbevel(0);
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

