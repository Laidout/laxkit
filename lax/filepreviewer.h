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
//    Copyright (c) 2004-2010,2012 Tom Lechner
//
#ifndef _LAX_FILEPREVIEWER_H
#define _LAX_FILEPREVIEWER_H

#include <lax/messagebar.h>
#include <lax/laximages.h>


namespace Laxkit {


#define FILEPREV_SHOW_DIMS   (1<<28)

class FilePreviewer : public MessageBar
{
 protected:
	char *filename;
	long sizelimit;
	int state;
	LaxImage *image;


 public:
	int pad;
	FilePreviewer(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
				int nx,int ny,int nw,int nh,int brder,
				const char *file = nullptr, int index = 0);
	virtual ~FilePreviewer();
	virtual const char *tooltip() { return filename; }
	virtual int init();
	virtual void Refresh();
	virtual const char *whattype() { return "FilePreviewer"; } 
//	virtual int LBDown(int x,int y,unsigned int state,int count);
//	virtual int RBDown(int x,int y,unsigned int state,int count);
//	virtual int LBUp(int x,int y,unsigned int state);
//	virtual int RBUp(int x,int y,unsigned int state);
//	virtual int MouseMove(int x,int y,unsigned int state);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
//	virtual int Resize(int nw,int nh);
	virtual int SetText(const char *newtext) { return MessageBar::SetText(newtext); }
	virtual int Preview(const char *file, int index = 0);
	virtual const char *Preview() { return filename; }
};

} // namespace Laxkit

#endif

