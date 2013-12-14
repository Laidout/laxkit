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
//    Copyright (c) 2004-2010,2012 Tom Lechner
//
#ifndef _LAX_FILEPREVIEWER_H
#define _LAX_FILEPREVIEWER_H

#include <lax/messagebar.h>

#ifdef LAX_USES_IMLIB
#include <Imlib2.h>
#endif

namespace Laxkit {


#define FILEPREV_SHOW_DIMS   (1<<28)

class FilePreviewer : public MessageBar
{
 protected:
	char *filename;
	long sizelimit;
	int state;
#ifdef LAX_USES_IMLIB
	Imlib_Image image;
#endif


 public:
	int pad;
	FilePreviewer(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
				int nx,int ny,int nw,int nh,int brder,
				const char *file=NULL,int sz=5000);
	virtual ~FilePreviewer();
	virtual const char *tooltip() { return filename; }
//	virtual int init();
	virtual void Refresh();
	virtual const char *whattype() { return "FilePreviewer"; } 
//	virtual int LBDown(int x,int y,unsigned int state,int count);
//	virtual int RBDown(int x,int y,unsigned int state,int count);
//	virtual int LBUp(int x,int y,unsigned int state);
//	virtual int RBUp(int x,int y,unsigned int state);
//	virtual int MouseMove(int x,int y,unsigned int state);
//	virtual int MoveResize(int nx,int ny,int nw,int nh);
//	virtual int Resize(int nw,int nh);
	virtual int SetText(const char *newtext) { return MessageBar::SetText(newtext); }
	virtual int Preview(const char *file);
	virtual const char *Preview() { return filename; }
};

} // namespace Laxkit

#endif

