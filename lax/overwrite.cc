//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Copyright (C) 2004-2007,2010 by Tom Lechner
//


#include <lax/overwrite.h>
#include <lax/messagebar.h>
#include <lax/strmanip.h>
#include <lax/language.h>


namespace Laxkit {

//-------------------------- Overwrite ------------------------------------
/*! \class Overwrite
 * \brief Dialog to ask whether to overwrite something.
 *
 * On a button click, a StrEventData with file is sent to the owner.
 *
 * Will make a dialog something like:
 * <pre>
 *     %Overwrite
 *  blahblahblah.blah?
 *
 *   [%Overwrite][No]
 * </pre>
 */


//! Constructor.
/*! Often this dialog is used to verify the continuation of a previous
 * StrEventData. In that case you can pass the info variables from the
 * previous event, which become the info for the event sent from this dialog.
 */
Overwrite::Overwrite(unsigned long nowner, //!< Who to notify
					 const char *mes,      //!< Message type to send
					 const char *nfile,   //!< File to prompt about
					 int i1,             //!< Gets put in StrEventData::info1
					 int i2,             //!< Gets put in StrEventData::info2
					 int i3)             //!< Gets put in StrEventData::info3
	: MessageBox(NULL, "overwrite", "Overwrite?", ANXWIN_CENTER, 
				 0,0,0,0,0, NULL,nowner,mes, _("Overwrite?"))
{
	info1=i1;
	info2=i2;
	info3=i3;
	file=newstr(nfile);
	char *blah=newstr("Overwrite\n");
	appendstr(blah,nfile);
	appendstr(blah,"?");
	WinFrameBox *box=dynamic_cast<WinFrameBox *>(wholelist.e[0]);
	MessageBar *mesbar=dynamic_cast<MessageBar *>(box->win());
	if (mesbar) {
		mesbar->win_w=0;
		mesbar->win_h=0;
		mesbar->SetText(blah);
		mesbar->SetupMetrics();
		box->pw(mesbar->win_w);
		box->ph(mesbar->win_h);
	}
	AddButton(BUTTON_OVERWRITE);
	AddButton(BUTTON_NO);
}

//! Sends a SimpleMessage to owner with file in it.
int Overwrite::Event(const EventData *e,const char *mes)
{
	if (e->type!=LAX_ButtonEvent || strcmp(mes,"mbox-mes")) return anXWindow::Event(e,mes);

	const SimpleMessage *ev=dynamic_cast<const SimpleMessage*>(e);
	if (ev->info2==BUTTON_OVERWRITE) {
		SimpleMessage *data=new SimpleMessage(file,info1,info2,info3,0);
		app->SendMessage(data,win_owner,win_sendthis,object_id);
	}
	
	app->destroywindow(this);
	return 0;
}



} // namespace Laxkit

