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
//    Copyright (C) 2004-2010,2012 by Tom Lechner
//

#include <lax/panuser.h>

namespace Laxkit {

/*! \class PanUser
 * \brief Brief class to provide useful stuff for elements that use a PanController.
 *
 * Classes derived from this one inherit these panner goodies. See for instance,
 * Scroller, ScrolledWindow, PanPopup, PanWindow. For derived classes, make sure to 
 * inherit from the PanUser BEFORE a window class, otherwise the destructor will
 * not function properly.
 *
 * \todo ***If this is made over to be anObject derived, then must ensure that all derived
 *   classes do ClassBlah : virtual ... PanUser, in any case, make sure that all the 
 *   other classes derive properly (public vs. protected?)
 * \todo *** maybe make the panner protected, functions public?
 */


//! Create with a new panner.
/*! A PanUser will never leave the constructor without a valid panner. panner
 * will not be NULL. Calls UseThisPanner(npan).
 */
PanUser::PanUser(PanController *npan)//NULL
{
	panner=NULL;
	UseThisPanner(npan);
}

//! Removes this from panner tellstack. Delete panner if necessary. 
/*! This transparently handles PanUsers being deleted BEFORE the panner.
 *
 * IMPORTANT: For this destructing mechanism to work, in your class definition of
 * the PanUser-derived class,
 * you MUST declare the PanUser BEFORE the window part, otherwise the PanUser
 * will not pop itself from the panner->tellstack here, since the window portion
 * will have been destroyed, and it cannot cast itself to an anXWindow anymore.
 */
PanUser::~PanUser()
{
	if (dynamic_cast<anXWindow *>(this) && panner) {
		 // assumes non refcounted this in tellstack. this is a good assumption.
		panner->tellPop(dynamic_cast<anXWindow *>(this));
	}
	if (panner) panner->dec_count();
}

//! Replace the current panner with npanner, which can be NULL to mean make a new one.
/*! This will always cause panner to point to a PanController. panner will not be NULL afterwards.
 *
 * The old is dec_counted, the new inc_counted.
 *
 * If this can be cast to anXWindow, then call this->Needtodraw(1), and add
 * this to panner->tellstack.
 */
void PanUser::UseThisPanner(PanController *npanner)
{
	if (npanner==NULL) {
		PanController *temppan=panner;
		panner=NULL;
		createNewPanner(temppan); // creates a new panner with temppan's settings
		if (temppan) temppan->dec_count();

	} else {
		if (panner!=npanner) {
			if (panner) panner->dec_count();
			panner=npanner;
			if (panner) panner->inc_count();
		}
	}
	if (dynamic_cast<anXWindow *>(this)) {
		dynamic_cast<anXWindow *>(this)->Needtodraw(1);
		panner->tell(dynamic_cast<anXWindow *>(this));
	}
}

//! Create a new panner that is a copy of pan if given.
PanController *PanUser::createNewPanner(PanController *pan)//pan=NULL 
{
	if (panner) panner->dec_count();
	if (pan) panner=new PanController(*pan);
	else panner=new PanController();
	return panner;
}

} // namespace Laxkit

