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
#ifndef _LAX_PRINTDIALOG_H
#define _LAX_PRINTDIALOG_H

#include <lax/rowframe.h>
#include <lax/doublebbox.h>

#include <cups/cups.h>
#include <cups/ppd.h>

namespace Laxkit {

//--------------------------------- PrintContext ------------------------------
class PrintContext : public EventData
{
 public:
	char *papername;
	float paperx;
	float papery;
	DoubleBBox pageregion;
	unsigned long flags; //landscape/portrait
	int copies;
	int pagestart,pageend; //-1==auto
	PrintContext();
	virtual ~PrintContext();
};

//--------------------------------- PrintDialog ------------------------------
#define PRINT_USE_PREVIEW      (1<<16)
#define PRINT_USE_PS_FILE      (1<<17)
#define PRINT_USE_EPS_FILE     (1<<18)
#define PRINT_USE_PDF_FILE     (1<<19)
#define PRINT_USE_XPRINT       (1<<20)
#define PRINT_USE_SVG          (1<<21)
#define PRINT_USE_PNG          (1<<22)
#define PRINT_USE_JPG          (1<<23)
#define PRINT_NO_CANCEL_DIALOG (1<<24)


class PrintDialog : public RowFrame
{
 protected:
	cups_dest_t *dests;
	int numdests,dest;
	ppd_file_t *ppd;
	cups_option_t *options;
	int numoptions;
	char *filetoprint;
	PrintContext *printcontext;
	int optionsstart;
	virtual void addOptions();
 public:
 	PrintDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			anXWindow *prev,unsigned long nowner,const char *nsend,
			const char *nfiletoprint=NULL, PrintContext *pt=NULL);
	virtual ~PrintDialog();
	virtual const char *whattype() { return "PrintDialog"; }
	virtual int preinit();
	virtual int init();
	virtual int send();
	virtual void setup();
	virtual int Event(const EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
};

} //namespace Laxkit

#endif


