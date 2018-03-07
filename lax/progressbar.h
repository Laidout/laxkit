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
#ifndef _LAX_PROGRESSBAR_H
#define _LAX_PROGRESSBAR_H

#include <lax/anxapp.h>

#define PROGRESS_CANCEL (1<<16)
#define PROGRESS_PAUSE  (1<<17)
#define PROGRESS_OVAL   (1<<18)

namespace Laxkit {

class ProgressBar : public anXWindow
{
  protected:
  public:
	long oldprogress,progress,max;
	unsigned long pcolor,bgcolor;
	ProgressBar(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder);
	virtual void Refresh();
	virtual void Drawbar();
	virtual int Set(long p, long nmax=0);
	virtual const char *whattype() { return "ProgressBar"; }
};

} // namespace Laxkit

#endif
