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
//    Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/buttonbase.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

/*! \class ButtonBase
 * \brief Abstract base class for single buttons.
 *
 *  Most derived classes would only redefine the following, everything else should work ok.
 *  Most would only need to redefine draw() based on values of mousein and state.
 *  Separating drawon/off/other/grayed is for convenience, when a separate thing is drawn in each case.
 *
 *  They are currently the following. Refresh() does not call draw() directly, but does call
 *  the other ones directly according to state:
 *  <pre>
 * 	virtual void draw() = 0;
 * 	virtual void drawon() { draw(); }    //<- for LAX_ON
 * 	virtual void drawoff() { draw(); }  //<- for LAX_OFF
 * 	virtual void drawgrayed() { drawoff(); }  //<- for LAX_GRAY
 * 	virtual void drawother() {}        //<- for any other state
 * 	</pre>
 * 
 * To use the drawother, derived classes must implement state values other than LAX_ON/OFF/GRAY, and redefine:
 * 	virtual int toggle() which should then toggle between all the values, not just LAX_ON/LAX_OFF.
 * 	Toggle is called from LBDown().
 *  Also redefine drawother(), which is called for state >2 from Refresh.
 * 
 * State(newstate).. state holds only an info value, not mouseover or anything like that. mousein
 * holds whether the mouse is in the window or not, based on Enter/Leave events.
 *
 *  Style:
 * \code 
 *    // only when buttondown and mouseon, off when mouseoff,sends on
 *    // LBUp and mousein.\n
 *   #define BUTTON_MOMENTARY (1<<16)
 *    // click toggles on/off, sends on any toggle and LBUp***check that
 *   #define BUTTON_TOGGLE (1<<17)
 * \endcode
 * <pre>
 *  TODO
 *  ***test grayed
 *   class Thing {
 *   	int state;
 *   	unsigned int flags;??
 *   }
 *  
 *  perhaps *** use a statemap: map to looks raised, flat, looks pressed
 * style 1   2
 *  off   nomo  ccur    flat  up
 *  off   nomo  noccur  flat  up
 *  off     mo  ccur    up    up
 *  off     mo  noccur  up    up
 *  on    nomo  ccur    down  down
 *  on    nomo  noccur  down  down
 *  on      mo  ccur    down  down
 *  on      mo  noccur  down  down
 *  gray  nomo  ccur    flat  up
 *  gray  nomo  noccur  flat  up
 *  gray    mo  ccur    flat  up
 *  gray    mo  noccur  flat  up
 * </pre>
 *
 * \todo could combine all the buttons: text button, iconbutton, checkbox into one thing...
 */
/*! \fn int ButtonBase::State()
 * \brief Return the state.
 */
/*! \var int ButtonBase::mousein
 * \brief The number of mice in the button, so 0 if none.
 */
	
//---------------------------------------------------------

ButtonBase::ButtonBase(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
				int xx,int yy,int ww,int hh,int brder,
				anXWindow *prev,unsigned long nowner,const char *nsendmes,
				int nid)
		: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendmes)
{
	prevbutton=nextbutton=NULL;
	
	state = oldstate = LAX_OFF; 
	mousein = 0;
	bevel = app->theme->default_bevel;
	id = nid;
	button_style = win_style&(~ANXWIN_MASK);

	InstallColors(THEME_Button);

    highlight = coloravg(win_themestyle->bg.Pixel(),rgbcolor(255,255,255));
	shadow = coloravg(win_themestyle->bg.Pixel(),rgbcolor(0,0,0));
}

//! Detaches button from a button group, if any.
ButtonBase::~ButtonBase()
{
	if (prevbutton) prevbutton->nextbutton=nextbutton;
	if (nextbutton) nextbutton->prevbutton=prevbutton;
	prevbutton=nextbutton=NULL;
}

//! Return anXWindow::Grayed()|(state==LAX_GRAY).
int ButtonBase::Grayed()
{
	return anXWindow::Grayed()|(state==LAX_GRAY);
}

/*! For some REALLY REALLY STUPID REASON, g++ is not recognizing anXWindow::Grayed(int)
 * as the proper Grayed(int) when ButtonBase::Grayed() exists, so relaying here.
 * To that I say $&#^@$&^&$^&!!!!
 */
int ButtonBase::Grayed(int g)
{
	return anXWindow::Grayed(g);
}


//! Set the state to newstate (if a button is not currently pressed). Typically LAX_ON or LAX_OFF.
int ButtonBase::State(int newstate)
{
	if (!buttondown.any()) { state=newstate; needtodraw=1; } 
	return state; 
}

/*! Does this:
 * \code
 *   switch (state) {
 *   	case LAX_GRAY: drawgrayed(); return;
 *   	case LAX_OFF: drawoff(); return;
 *   	case LAX_ON: drawon(); return;
 *   	default: drawother(); return;
 *   }
 * \endcode
 */
void ButtonBase::Refresh()
{
	if (!win_on || !needtodraw) return;

	//mousein=mouseisin(this,0); ***this was here because window was not getting all the enter/exit events it was supposed to..

	//DBG cerr <<"******************** but draw: "<<state<<" old:"<<oldstate<<endl;
	needtodraw=0; 
	if (Grayed()) drawgrayed();
	else switch (state) {
		case LAX_GRAY: drawgrayed(); return;
		case LAX_OFF: drawoff(); return;
		case LAX_ON: drawon(); return;
		default: drawother(); return;
	}
	return;
}

//! Send a SimpleMessage with type==LAX_ButtonEvent.
/*! data.info1=state,
 *  data.info2=id,
 *  data.info3=device id of what triggered the send.
 *  data.info4=1 or -1, direction according to wheel clicks
 */
int ButtonBase::send(int deviceid, int direction)
{
	//DBG cerr <<"trying to send from "<<WindowTitle()<<" to "<<owner<<" with "<<sendthis<<endl;
	if (!win_owner || !win_sendthis) return 1;

	SimpleMessage *e=new SimpleMessage(win_owner,object_id, LAX_ButtonEvent, win_sendthis);
	e->info1=state;
	e->info2=id;
	e->info3=deviceid;
	e->info4=direction;
	//DBG cerr <<"ButtonBase:"<<WindowTitle()<<" send to "<<owner<<": "<<owner<<"  id="<<id<<endl;
	app->SendMessage(e,win_owner,win_sendthis,object_id);
	return 0;
}

//! On enter or leave, swap oldstate and newstate.
int ButtonBase::Event(const EventData *e,const char *mes)
{
	DBG cerr <<"ButtonBase::Event "<<WindowTitle()<<":"<<mes<<endl;
	switch (e->type) {
		case LAX_onMouseIn: {
			const EnterExitData *ee=dynamic_cast<const EnterExitData *>(e);
			mousein++;
			DBG cerr <<"  Enter (mousein="<<mousein<<"):"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;
			needtodraw=1;
			if (buttondown.isdown(ee->device->id,LEFTBUTTON)) {
				int c=state;
				state=oldstate;
				oldstate=c;
			}
			//DBG cerr <<"  Enter:"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;
		} break;

		case LAX_onMouseOut: {
			const EnterExitData *ee=dynamic_cast<const EnterExitData *>(e);
			mousein--;
			DBG cerr <<" Leave (mousein="<<mousein<<"):"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;
			if (mousein<0) mousein=0;
			needtodraw=1; 
			if (buttondown.isdown(ee->device->id,LEFTBUTTON)) {
				int c=state;
				state=oldstate;
				oldstate=c;
			}
			//DBG cerr <<"  ButtonBase::event:Leave:"<<WindowTitle()<<": state:"<<state<<"  oldstate:"<<oldstate<<endl;
		} break;

		default: break;
	}
	return anXWindow::Event(e,mes);
}

//! If state==LAX_ON, make LAX_OFF and vice versa, and set needtodraw=1. Returns state. Does net send.
int ButtonBase::toggle()
{
	if (state==LAX_ON) state=LAX_OFF;
	else if (state==LAX_OFF) state=LAX_ON;
	else return state;
	needtodraw=1;
	return state;
}

/*! If mouse crosses borders, make sure that mousein is the correct value.
 * Seems the window is not receiving all the Enter/LeaveNotify events that it
 * is supposed to.
 */
int ButtonBase::MouseMove(int x,int y,unsigned int mstate,const LaxMouse *d)
{
//	int nmi=mouseisin(this,0);
//	if ((nmi && !mousein) || (!nmi && mousein)) {
//		needtodraw=1;
//		mousein=nmi;
//	}
	return anXWindow::MouseMove(x,y,mstate, d);
}

/*! Set oldstate to the current state and call toggle.
 */
int ButtonBase::LBDown(int x,int y,unsigned int wstate,int count,const LaxMouse *d)
{
	if (buttondown.isdown(0,LEFTBUTTON)) return 0;
	buttondown.down(d->id,LEFTBUTTON, x,y);
	if (Grayed()) return 0;
	oldstate=state;
	toggle();
	return 0;
}

/*! If !mousein or left button is not down, return.
 *
 * If MOMENTARY, then set state=oldstate.
 *
 * send().
 */
int ButtonBase::LBUp(int x,int y,unsigned int wstate,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 0;
	buttondown.up(d->id,LEFTBUTTON);

	if (Grayed()) return 0;
	if (win_style&BUTTON_MOMENTARY || (!(win_style&BUTTON_TOGGLE))) {
		//if (state==LAX_ON) state=LAX_OFF;
		//if (mousein) state=oldstate;
		state=LAX_OFF;
		mousein=0;
		needtodraw=1;
	}
	if (x>=0 && x<win_w && y>=0 && y<win_h) send(d->id,0);
	return 0;
}

//! For Momentary buttons, send() with direction=1.
int ButtonBase::WheelDown(int x,int y,unsigned int wstate,int count,const LaxMouse *d)
{
	if (Grayed()) return 0;
	if (win_style&BUTTON_MOMENTARY || (!(win_style&BUTTON_TOGGLE))) {
		//if (state==LAX_ON) state=LAX_OFF;
		//if (mousein) state=oldstate;
		state=LAX_OFF;
		mousein=0;
		needtodraw=1;
	}
	send(d->id,1);
	return 0;
}

//! For Momentary buttons, send() with direction=-1.
int ButtonBase::WheelUp(int x,int y,unsigned int wstate,int count,const LaxMouse *d)
{
	if (Grayed()) return 0;
	if (win_style&BUTTON_MOMENTARY || (!(win_style&BUTTON_TOGGLE))) {
		//if (state==LAX_ON) state=LAX_OFF;
		//if (mousein) state=oldstate;
		state=LAX_OFF;
		mousein=0;
		needtodraw=1;
	}
	send(d->id,-1);
	return 0;
}

//! Sends on enter.
int ButtonBase::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Enter) {
		send(d->id,0);
		return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state, d);
}


} // namespace Laxkit

