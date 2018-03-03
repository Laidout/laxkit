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
#ifndef _LAX_INPUTDIALOG_H
#define _LAX_INPUTDIALOG_H

#include <lax/messagebox.h>
#include <lax/lineedit.h>

namespace Laxkit {

//#define MBOX_WRAP (1<<16)
	
class InputDialog : public MessageBox
{
 protected:
 public:
	
	InputDialog(anXWindow *parnt, const char *nname,const char *ntitle, unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev, unsigned long nowner, const char *nsend,
						const char *starttext,
						const char *label,
						const char *button1=NULL, int id1=0,
						const char *button2=NULL, int id2=0,
						const char *button3=NULL, int id3=0);
	virtual ~InputDialog() {}
	virtual const char *whattype() { return "InputDialog"; }
	virtual int Event(const EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual LineEdit *GetLineEdit();
};


} // namespace Laxkit

#endif

