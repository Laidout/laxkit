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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_SIMPLEPRINT_H
#define _LAX_SIMPLEPRINT_H

#include <lax/lineedit.h>
#include <lax/checkbox.h>
#include <lax/messagebox.h>

namespace Laxkit {

#define SIMPP_SEND_TO_PRINTER  (1<<16)
#define SIMPP_DEL_PRINTTHIS    (1<<17)
#define SIMPP_PRINTRANGE       (1<<18)
	
class SimplePrint : public MessageBox
{
 protected:
	char *printthis;
	int tofile;
	LineEdit *fileedit,*commandedit,*printstart,*printend;
	CheckBox *filecheck,*commandcheck;
	CheckBox *printall,*printcurrent,*printrange;
	int min,max,cur;
	virtual void changeTofile(int t);
 public:
	SimplePrint(unsigned long nstyle,unsigned long nowner,const char *nsend,
						 const char *file="output.ps", const char *command="lp",
						 const char *thisfile=NULL,
						 int ntof=1,int pmin=-1,int pmax=-1,int pcur=-1);
	virtual ~SimplePrint() { delete[] printthis; }
	virtual int init();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int Event(const EventData *e,const char *mes);
	virtual int Print();
};

} // namespace Laxkit


#endif

