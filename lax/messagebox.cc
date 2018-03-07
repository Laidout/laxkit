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


#include <lax/messagebox.h>
#include <lax/messagebar.h>

#include <iostream>
using namespace std;
#define DBG 

//#define MBOX_WRAP (1<<16)
//#define MBOX_OK_CANCEL
//#define MBOX_OK
//#define MBOX_QUIT_ANYWAY_CANCEL

namespace Laxkit {


/*! \class MessageBox
 * \brief Window that puts a message and buttons like OK or Cancel.
 * 
 * The strategy is to create a new MessageBox, then immediately add
 * buttons (with AddButton()) or other windows to it
 * (through the base class RowFrame::AddWin()), before calling
 * app->addwindow().
 * 
 * \todo  standard buttons like OK or Cancel! or whole style like \n
 * Overwrite? Yes No Cancel\n
 * Save "blah"? Yes No Cancel\n
 * Save "blah" before quitting? Yes No\n
 * "blah" unsaved, quit anyway? Yes No\n
 * 
 *  \todo *** incorporate automatic Control Tab Loop thingy
 *  ***first button is always ok??
 *  *** must have a constructor specifically for blocking popups that auto-wrap to message extent, and send a message back
 *
 *  \code #include <messagebox.h> \endcode
 */


//! Default is to add a new MessageBar if mes!=NULL.
/*! The nstyle should be the same things you would pass to RowFrame.
 * Currently what actually does get passed to RowFrame is nstyle|ROWFRAME_CENTER|ROWFRAME_ROWS.
 *
 * \todo prev is ignored
 */
MessageBox::MessageBox( anXWindow *parnt, const char *nname,const char *ntitle, unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev, unsigned long nowner, const char *nsend,
						const char *mes)
		: RowFrame(parnt,nname,ntitle,nstyle|ROWFRAME_CENTER|ROWFRAME_ROWS,
				   xx,yy,ww,hh,brder, prev,nowner,nsend,
				   anXApp::app->defaultlaxfont->textheight()/2)
{
	if (win_w<=1) w(BOX_SHOULD_WRAP); else { w(win_w); pw(win_w); }
	if (win_h<=1) h(BOX_SHOULD_WRAP); else { h(win_h); ph(win_h); }
	//m[1]=m[7]=BOX_SHOULD_WRAP; //***<-- this triggers a wrap in rowcol-figureDims

	padinset=5;
	if (mes) {
		AddWin(new MessageBar(this,"mbox-mes",NULL,MB_CENTER|MB_MOVE|MB_COPY, 0,0,0,0, 0, mes), 1,-1);
		AddNull();
	}
}

//! Destroy on any ClientMessage.
/*! If the message is a 'mbox-mes', then relay that message to the owner.
 *
 * The data from the relayed event is not changed, but the message_type
 * becomes this->sendthis.
 */
int MessageBox::Event(const EventData *e,const char *mes)
{
	DBG cerr <<WindowTitle()<<" -- MessageBox::Event \""<<mes<<"\""<<endl;
	if (e->type!=LAX_ButtonEvent || strcmp(mes,"mbox-mes")) return anXWindow::Event(e,mes);

	if (win_owner) {
		const SimpleMessage *ev=dynamic_cast<const SimpleMessage*>(e);

		SimpleMessage *ee=new SimpleMessage;
		ee->type=LAX_ButtonEvent;
		ee->info1=ev->info1;
		ee->info2=ev->info2;

		app->SendMessage(ee,win_owner,win_sendthis,object_id);
	}
	
	app->destroywindow(this);
	return 0;
}

/*! This sets w and h to BOX_SHOULD_WRAP, which is supposed to trigger a wrap to extent.
 * Then arrangeBoxes(1) is called. If the window sizes are then larger than the screen,
 * the window is shrunk to be within the screen.
 * \todo ***check this!!
 *
 * If a window has a stretch of 9000, say, then that window is stretched
 * to that amount, which is silly, so right now, the only way to prevent that 
 * is to subclass MessageBox and intercept preinit().... 
 *
 * \todo this needs a little more thought....
 */
int MessageBox::preinit()
{
	//m[1]=m[7]=BOX_SHOULD_WRAP; //***<-- this triggers a wrap in rowcol-figureDims
	if (win_w<=1) w(BOX_SHOULD_WRAP);
	if (win_h<=1) h(BOX_SHOULD_WRAP);

	//WrapToExtent: 
	arrangeBoxes(1);
	win_w=w();
	win_h=h();

	Screen *screen=DefaultScreenOfDisplay(app->dpy);
	if (win_h+2*(int)win_border>(int)screen->height) { 
		win_h=screen->height-2*win_border;
	}
	if (win_w+2*(int)win_border>(int)screen->width) { 
		win_w=screen->width-2*win_border;
	}
	return 0;
}

int MessageBox::init()
{
	DBG cerr <<"------MessageBox::init"<<endl;
	RowFrame::init(); // the Sync(1) there addwindow's the lot
	
//	if (!win_sizehints) {
//		win_sizehints=XAllocSizeHints();
//	}
//	win_sizehints->x=win_x;
//	win_sizehints->y=win_y;
//	win_sizehints->width=win_w;
//	win_sizehints->height=win_h;
//	win_sizehints->flags=USPosition|USSize;
		
	 // make the owner of all the direct kids this->window
	anXWindow *cc;
	for (int c=0; c<_kids.n; c++) {
		cc=dynamic_cast<anXWindow *>(_kids.e[c]);
		if (cc && !cc->win_owner) cc->SetOwner(this);
	}
	DBG cerr <<"--------win w,h:"<<win_w<<','<<win_h<<endl;
	DBG cerr <<"------done MessageBox::init"<<endl;
	return 0;
}


//! Add a text button with btext as the label.
/*! Clicking on buttons installed with this will cause the
 * messagebox to be destroyed and message sent to the owner.
 *
 * prev is passed to the Button constructor for the previous contron of
 * a tab loop.
 * Returns a pointer to the new Button. This can be used to close a tab
 * loop, for instance.
 */
Button *MessageBox::AddButton(const char *btext,int sendid,anXWindow *prev)
{
	 // textbutton will determine the extent of the text, which RowFrame uses 
	 // with the following form of AddWin.
	Button *tbut=new Button(this,"button",NULL,0,
							0,0,0,0, 1, 
							prev, object_id, "mbox-mes",
							sendid,
							btext,NULL,NULL,pad/2,pad/2);
	if (AddWin(tbut,1,-1)) return NULL;
	return tbut;
}

//! Adds a button with a standard Button style define in buttontype
/*! Clicking on buttons installed with this will cause the
 * messagebox to be destroyed and message sent to the owner.
 *
 * prev is passed to the Button constructor for the previous contron of
 * a tab loop.
 * Returns a pointer to the new Button. This can be used to close a tab
 * loop, for instance.
 */
Button *MessageBox::AddButton(unsigned int buttontype,anXWindow *prev)
{
	Button *tbut=new Button(this,"button",NULL,buttontype,
							0,0,0,0, 1, 
							prev, object_id, "mbox-mes",
							buttontype,
							NULL,NULL,NULL,pad/2,pad/2);
	if (AddWin(tbut,1,-1)) return NULL;
	return tbut;
}




} // namespace Laxkit

