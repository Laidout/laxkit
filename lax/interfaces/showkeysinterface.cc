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



#include <lax/interfaces/showkeysinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/laxutils.h>
#include <lax/language.h>


using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class ShowKeysInterface
 * \ingroup interfaces
 * \brief Interface to show what keys and mouse buttons are being pressed
 *
 * Note this is pretty primitive, and only works in viewports.
 * A better solution is to use a program like key-mon.
 */


ShowKeysInterface::ShowKeysInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	placement=LAX_BOTTOM|LAX_RIGHT;
	showkeys_style=0;

	needtodraw=1;
}

ShowKeysInterface::~ShowKeysInterface()
{
}


/*! Name as displayed in menus, for instance.
 */
const char *ShowKeysInterface::Name()
{ return _("Show Keys"); }


//! Return new ShowKeysInterface.
/*! If dup!=NULL and it cannot be cast to ShowKeysInterface, then return NULL.
 */
anInterface *ShowKeysInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new ShowKeysInterface(NULL,id,NULL);
	else if (!dynamic_cast<ShowKeysInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int ShowKeysInterface::InterfaceOn()
{ 
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int ShowKeysInterface::InterfaceOff()
{ 
	Clear(NULL);
	needtodraw=1;
	return 0;
}

void ShowKeysInterface::Clear(SomeData *d)
{
}


const char *KeyIntToStr(unsigned int ch)
{
    if (ch==' '       ) return "Space"; 
    if (ch==LAX_Esc   ) return "Esc";   
    if (ch==LAX_Menu  ) return "Menu";  
    if (ch==LAX_Pause ) return "Pause"; 
    if (ch==LAX_Del   ) return "Del";   
    if (ch==LAX_Bksp  ) return "Bksp";  
    if (ch==LAX_Tab   ) return "Tab";   
    if (ch==LAX_Ins   ) return "Ins";   
    if (ch==LAX_Home  ) return "Home";  
    if (ch==LAX_End   ) return "End";   
    if (ch==LAX_Enter ) return "Enter"; 
    if (ch==LAX_Pgup  ) return "PgUp";  
    if (ch==LAX_Pgdown) return "PgDn";  
    if (ch==LAX_F1    ) return "F1";    
    if (ch==LAX_F2    ) return "F2";    
    if (ch==LAX_F3    ) return "F3";    
    if (ch==LAX_F4    ) return "F4";    
    if (ch==LAX_F5    ) return "F5";    
    if (ch==LAX_F6    ) return "F6";    
    if (ch==LAX_F7    ) return "F7";    
    if (ch==LAX_F8    ) return "F8";    
    if (ch==LAX_F9    ) return "F9";    
    if (ch==LAX_F10   ) return "F10";   
    if (ch==LAX_F11   ) return "F11";   
    if (ch==LAX_F12   ) return "F12";   
    if (ch==LAX_Left  ) return "Left";  
    if (ch==LAX_Up    ) return "Up";    
    if (ch==LAX_Down  ) return "Down";  
    if (ch==LAX_Right ) return "Right"; 

	return NULL;
}

int ShowKeysInterface::Refresh()
{
	if (needtodraw==0) return 0;
	needtodraw=0;

	if (currentkey==0 && (currentstate&LAX_STATE_MASK)==0) return 0;

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->NewFG(curwindow->win_themestyle->fg);

	dp->DrawScreen();


	char cc[20]; cc[0]='\0';
	const char *ss=cc;
	if (currentkey>32 && currentkey<128) {
		cc[0]=currentkey;
		cc[1]='\0';
	} else if (currentkey>0) {
		ss=KeyIntToStr(currentkey);
		if (!ss) {
			ss=cc;
			sprintf(cc,"0x%x",currentkey);
		}
	}

	int xx=dp->Maxx-1;
	if (currentkey!=0) xx=DrawBox(ss,dp->Maxx-1,dp->Maxy-1);

	xx-=dp->textheight()/2;
	if (currentstate&MetaMask)    xx=DrawBox("Meta",   xx,dp->Maxy-1);
	if (currentstate&AltMask)     xx=DrawBox("Alt",    xx,dp->Maxy-1);
	if (currentstate&ControlMask) xx=DrawBox("Control",xx,dp->Maxy-1);
	if (currentstate&ShiftMask)   xx=DrawBox("Shift",  xx,dp->Maxy-1);


	dp->DrawReal();

	return 0;
}

/*! x,y is the lower right corner. Draw boxes starting at right side. Return new right edge.
 */
int ShowKeysInterface::DrawBox(const char *str,int x,int y)
{
	double th=dp->textheight();
	double pad=th/4;

	double ww=dp->textextent(str,-1,NULL,NULL);
	dp->NewFG(coloravg(curwindow->win_themestyle->fg, curwindow->win_themestyle->bg, .5));
	dp->NewBG(coloravg(curwindow->win_themestyle->fg, curwindow->win_themestyle->bg, .9));
	dp->drawRoundedRect(x-ww-2*pad, y-th-2*pad, ww+2*pad,th+2*pad, pad,false, pad,false, 2);

	dp->NewFG(curwindow->win_themestyle->fg);
	dp->textout(x-pad,y-pad, str,-1, LAX_BOTTOM|LAX_RIGHT);

	return x-ww-2*pad;
}

int ShowKeysInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (state!=currentstate) {
		currentstate=state;
		needtodraw=1;
	}
	return 0;
}

//! Start a new freehand line.
int ShowKeysInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	buttondown.down(d->id,LEFTBUTTON,x,y);
	needtodraw=1;
	return 1;
}

//! Finish a new freehand line by calling newData with it.
int ShowKeysInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	needtodraw=1;
	return 1;
}

//! Start a new freehand line.
int ShowKeysInterface::MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	buttondown.down(d->id,MIDDLEBUTTON,x,y);
	needtodraw=1;
	return 1;
}

//! Finish a new freehand line by calling newData with it.
int ShowKeysInterface::MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,MIDDLEBUTTON);
	needtodraw=1;
	return 1;
}

int ShowKeysInterface::RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	buttondown.down(d->id,RIGHTBUTTON,x,y);
	needtodraw=1;
	return 1;
}

int ShowKeysInterface::RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,RIGHTBUTTON);
	needtodraw=1;
	return 1;
}

int ShowKeysInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1;
}

int ShowKeysInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1;
}


int ShowKeysInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	currentstate=state;
	if (ch==LAX_Shift)        currentstate|=ShiftMask;
	else if (ch==LAX_Control) currentstate|=ControlMask;
	else if (ch==LAX_Alt)     currentstate|=AltMask;
	else if (ch==LAX_Meta)    currentstate|=MetaMask;
	needtodraw=1;

	if (!(ch==LAX_Shift || ch==LAX_Control || ch==LAX_Meta || ch==LAX_Alt)) currentkey=ch;

	needtodraw=1;
	return 1;
}

int ShowKeysInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	currentstate=state;
	if (ch==LAX_Shift)        currentstate&=~ShiftMask;
	else if (ch==LAX_Control) currentstate&=~ControlMask;
	else if (ch==LAX_Alt)     currentstate&=~AltMask;
	else if (ch==LAX_Meta)    currentstate&=~MetaMask;

	currentkey=0;
	needtodraw=1;
	return 1;
}


} // namespace LaxInterfaces

