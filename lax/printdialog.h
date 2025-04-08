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
#ifndef _LAX_PRINTDIALOG_H
#define _LAX_PRINTDIALOG_H

#include <lax/rowframe.h>
#include <lax/doublebbox.h>

#include <cups/cups.h>
// #include <cups/ppd.h>


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

class PrintDialog : public RowFrame
{
  protected:
	cups_dest_t *dests;
	int numdests, current_printer;
	int numoptions;
	char *filetoprint;
	char *to_file_path;
	PrintContext *printcontext;
	int optionsstart; //index in wholelist where options widgets start

	MenuInfo *cached_paper_list = nullptr;
	// cups_option_t *options;
	// ppd_file_t *ppd;
	
	virtual void addOptions();

  public:
  	enum DialogFlags {
  		PRINT_NO_TO_FILE = (1<<16)
  	};
  	unsigned int dialog_flags;

 	PrintDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			anXWindow *prev,unsigned long nowner,const char *nsend,
			unsigned int dialog_flags,
			const char *nfiletoprint=NULL, PrintContext *pt=NULL);
	virtual ~PrintDialog();
	virtual const char *whattype() { return "PrintDialog"; }
	virtual int preinit();
	virtual int init();
	virtual int send();
	virtual void setup();
	virtual MenuInfo *GetPaperSizeMenu();
	virtual int Event(const EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
};

} //namespace Laxkit

#endif


