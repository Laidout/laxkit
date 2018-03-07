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
//    Copyright (C) 2007 by Tom Lechner
//


#include <lax/inputdialog.h>
#include <lax/lineedit.h>
#include <lax/messagebar.h>
#include <lax/strmanip.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


/*! \class InputDialog
 * \brief Dialog to enter a single line of text, with optional 3 buttons.
 *
 *  \code #include <lax/inputdialog.h> \endcode
 */


/*! For the optional added buttons, each has an id value. If that id is 0 or BUTTON_CANCEL, then
 * that is equivalent to "cancel". A value of 1 must mean the same as hitting "ok".
 * This is so that when somewhen hits enter in the edit, it will trigger an event with an info
 * value of 1. Any other id value causes a StrEventData to be
 * sent with that number as the info.
 *
 * InputDialog objects are always ANXWIN_ESCAPABLE.
 */
InputDialog::InputDialog(anXWindow *parnt, const char *nname,const char *ntitle, unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev, unsigned long nowner, const char *nsend,
						const char *starttext,
						const char *label,
						const char *button1, int id1,
						const char *button2, int id2,
						const char *button3, int id3)
	: MessageBox(parnt, nname, ntitle, ANXWIN_ESCAPABLE|nstyle, xx,yy,ww,hh,brder,
						prev, nowner, nsend, label)
{

//	LineEdit(anXWindow *parnt,const char *nname,const char *ntitle,unsigned int nstyle,
//			int xx,int yy,int ww,int hh,int brder,
//			anXWindow *prev,unsigned long nowner=None,const char *nsend=NULL,
//			const char *newtext=NULL,unsigned int ntstyle=0);

	LineEdit *le=new LineEdit(this,"input-edit",NULL,0,
              0,0,0,0, 1,
			  NULL,object_id,"mes input",
			  starttext);
	le->padx=3;
	AddWin(le,1, 250,0,1000,50,0, app->defaultlaxfont->textheight()+6,0,0,50,0, -1);
	AddNull();
	anXWindow *w=le;
	if (button1) w=AddButton(button1,id1,w);
	if (button2) w=AddButton(button2,id2,w);
	if (button3) w=AddButton(button3,id3,w);	
}

LineEdit *InputDialog::GetLineEdit()
{
	return dynamic_cast<LineEdit*>(findChildWindowByName("input-edit"));
}

//! Send a StrEventData with the input, and destroy the dialog.
/*! \todo *** enter from the lineedit makes focus jump to ok button?
 */
int InputDialog::Event(const EventData *e,const char *mes)
{
	DBG cerr <<WindowTitle()<<" -- Event \""<<(mes?mes:"(nostr)")<<"\"  idnum="<<e->type<<endl;

	if (e->type==LAX_ButtonEvent && !strcmp(mes,"mbox-mes")) {

		const SimpleMessage *ev=dynamic_cast<const SimpleMessage*>(e);
		if (!strcmp(mes,"mbox-mes") && (ev->info2==0 || ev->info2==BUTTON_CANCEL)) {
			app->destroywindow(this);
			return 1;
		}
		
		if (win_owner) {
			StrEventData *s=new StrEventData(NULL,0,0,0,0);
			if (!strcmp(mes,"mbox->mes")) s->info1=ev->info2;
			else if (!strcmp(mes,"mes input")) s->info1=1;

			LineEdit *le=(LineEdit *)findChildWindowByName("input-edit");
			makestr(s->str,le->GetCText());

			app->SendMessage(s,win_owner,win_sendthis,object_id);
		}
		
		app->destroywindow(this);
		return 0;
	}
	return anXWindow::Event(e,mes);
}

//! Cancel if ESC.
int InputDialog::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		app->destroywindow(this);
		return 0;

	} else if (ch=='\t') {
		LineEdit *le=(LineEdit *)findChildWindowByName("input-edit");
		const LaxKeyboard *k=dynamic_cast<const LaxKeyboard *>(d);
		app->setfocus(le,0,k);
		return 0;
	}

	return anXWindow::CharInput(ch,buffer,len,state,d);

}





} // namespace Laxkit

