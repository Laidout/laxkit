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
//    Copyright (C) 2010 by Tom Lechner
//
#include <lax/tooltip.h>

#include <iostream>
using namespace std;
#define DBG 

#include <lax/laxutils.h>
#include <lax/screeninformation.h>

namespace Laxkit {

//------------------------------ ToolTip ---------------------------------
/*! \class ToolTip
 * \brief The ToolTip class is used within anXApp.
 *
 * There can be one tooltip per mouse.
 *
 * To give a window a tooltip, simply call win->tooltip("blah new tip").
 *  Make sure that anXApp::tooltips is set to an adequate number of milliseconds,
 *  and anXApp will hover the tooltip near the current mouse position until the
 *  mouse moves again. 
 *
 *  Multiline tooltips are simply one char array with the standard
 *  '\\n' characters to delimit the lines.
 *
 *  \todo This class is basically a MessageBar without the input handling, maybe
 *    use that one instead for automatic text wrapping?
 */

int ToolTip::numtips = 0;

int ToolTip::NumTips()
{
	return numtips;
}

/*! Constructor, initiates a timer that lasts for a maximum
 * of 5 seconds + 1sec per every 20 characters.
 */
ToolTip::ToolTip(const char *newtext,int mouse)
	: anXWindow(NULL,"tooltip","tooltip",ANXWIN_BARE,0,0,1,1,0,NULL,0,NULL)
{
	DBG cerr <<"Creating Tooltip: \""<<(newtext?newtext:"(missing)")<<"\"..."<<endl;

	mouse_id=mouse;

	numtips++;
	needtodraw=1;
	win_border=1;
	InstallColors(THEME_Tooltip);
	
	if (!newtext) newtext="TOOLTIP MISSING";
	thetext=new char[strlen(newtext)+1];
	strcpy(thetext,newtext);

	 // Automatically wrap win_w,win_h to text extents
	int nl=0,t;
	win_w=0;
	int c=0,c2=0;
	while (thetext[c]!='\0') {
		c2=c;
		while (thetext[c]!='\0' && thetext[c]!='\n') c++;
		nl++;
		if (c==c2) continue;
	    t = win_themestyle->normal->Extent(thetext+c2,c-c2);
		if (t>win_w) win_w=t;
		if (thetext[c]!='\0') c++;
	}
	DBG cerr <<"Tooltip:  nl="<<nl<<endl;
	textheight = app->defaultlaxfont->textheight();
	win_w += app->default_padx*2;
	win_h  = app->default_pady*2 + nl*textheight;

	 // Automatically place so it is near but not on mouse, and contained in a monitor
	int rx = 0, ry = 0, scx = 0, scy = 0;
	ScreenInformation *scr = NULL;
	if (mouse_id > 0) {
		mouseposition(mouse_id, NULL, &rx,&ry, NULL,NULL, NULL, &scr);
		if (scr != NULL) { scx = scr->x; scy = scr->y; }
	}
	if (rx-win_w < scx) win_x = rx + textheight; else win_x = rx-win_w;
	if (ry-(1+nl)*textheight < scy) win_y = ry+textheight; else win_y = ry-(1+nl)*textheight;
	
	c=strlen(thetext)-20;
	if (c<0) c=0;
	app->addtimer(this,5000+c*50,5000+c*50,5001+c*50); // last for max of 5 seconds + 1sec / 20 chars
	//DBG cerr <<"Done Creating Tooltip..."<<endl;

}

ToolTip::~ToolTip()
{
	if (thetext) delete[] thetext;
	numtips--;
}

//! Just dump out the text, left justified.
void ToolTip::Refresh()
{
	if (!needtodraw || !win_on || !thetext) return;
	
	Displayer *dp = MakeCurrent();

	dp->NewFG(win_themestyle->fg);
	dp->NewBG(win_themestyle->bg);
	dp->ClearWindow();

	dp->textout(app->theme->default_padx,app->theme->default_pady, thetext,-1, LAX_LEFT|LAX_TOP);
	needtodraw=0;
}

//! The tooltip destroys itself when any key or mouse events occur.
int ToolTip::Event(const EventData *e,const char *mes)
{
	if (       e->type==LAX_onKeyDown
			|| e->type==LAX_onKeyUp
			|| e->type==LAX_onMouseOut
			|| e->type==LAX_onButtonDown
			|| e->type==LAX_onButtonUp) {
		app->destroywindow(this);
	}
	return 0;
}

//! Destroy itself if time up.
/*! Also, the tooltip destroys itself by checking for any key or mouse event in the X event queue,
 * for any window.
 */
int ToolTip::Idle(int tid, double delta)
{
	DBG cerr <<"ToolTip \""<<thetext<<"\" idle"<<endl;
	if (tid) app->destroywindow(this);
	return 0;
}

} //namespace Laxkit

