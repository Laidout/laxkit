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
//    Copyright (C) 2004-2006,2010,2013 by Tom Lechner
//
#ifndef _LAX_QUICKFILEOPEN_H
#define _LAX_QUICKFILEOPEN_H

#include <lax/button.h>
#include <lax/filedialog.h>
#include <lax/inputdialog.h>
	
namespace Laxkit {

class QuickFileOpen : public Button
{
 public:
	int type;
	LineInput *path;
	char *dir;
	QuickFileOpen(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long  nowner,const char *nsendmes="file open",
						unsigned int ntype=FILES_OPEN_ONE,
						LineInput *npath=NULL,
						const char *ndir=NULL,
						const char *nlabel="...");
	virtual ~QuickFileOpen();
	virtual void SetDir(const char *ndir);
	virtual int send(int deviceid,int direction);
};

} // namespace Laxkit

#endif

