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
//    Copyright (C) 2004-2006,2010 by Tom Lechner
//

#include <lax/quickfileopen.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

/*! \class QuickFileOpen
 * \brief A button with "..." on it that pops up a FileDialog sporting a menu with FileMenuItem entries.
 *
 * A StrEventData with the chosen file is sent to the owner (which is
 * passed in to the constructor).
 *
 * 
 */

//! nowner is who gets sent the StrEventData of the file chosen.
QuickFileOpen::QuickFileOpen(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						unsigned int ntype,
						LineInput *npath,
						const char *ndir,
						const char *nlabel)
	: Button(parnt,nname,ntitle,(nstyle&(0xffff))|BUTTON_MOMENTARY,
			 xx,yy,ww,hh,brder,prev,nowner,nsendmes,
			 0,
			 nlabel?nlabel:"...",NULL,NULL)
{
	win_style=nstyle|BUTTON_MOMENTARY;
	type=ntype;
	path=npath;
	dir=newstr(ndir);
}

QuickFileOpen::~QuickFileOpen()
{
	if (dir) delete[] dir;
}

//! Set the current path to some non-null path. Ok to be "".
void QuickFileOpen::SetDir(const char *ndir)
{
	if (ndir) makestr(dir,ndir);
}

	
//! Called when mouse is up, the pops up the PopupMenu via app->rundialog(new PopupMenu).
int QuickFileOpen::send(int deviceid,int direction)
{
	
	app->rundialog(new FileDialog(NULL,"File Popup Menu","File Popup menu", 0,
								0,0,0,0,1, 
								win_owner,win_sendthis, 
								type,
								path?path->GetCText():NULL,
								dir),
					NULL,1);
	return 0;
}


} // namespace Laxkit

