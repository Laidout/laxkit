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

#include <lax/simpleprint.h>
#include <lax/messagebar.h>
#include <lax/language.h>
#include <lax/strmanip.h>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//---------------------------- SimplePrint -------------------------
/*! \class SimplePrint
 * \brief Simple dialog with option to print to file or via command.
 *
 * For a super-duper print dialog, see PrintDialog.
 *
 * \code
 *  #include <lax/simpleprint.h> 
 * 
 *  #define SIMPP_SEND_TO_PRINTER  (1<<16) <-- unimp!
 *  #define SIMPP_DEL_PRINTTHIS    (1<<17)
 *  #define SIMPP_PRINTRANGE       (1<<18)
 * \endcode
 * 
 * SIMPP_PRINTRANGE will include From: ___ To: ___ controls.
 *  
 * SIMPP_DEL_PRINTTHIS causes what you pass in as thisfile in the constructor
 * to be deleted after it is sent to the printer or printed to some other file.
 *
 * 
 */


/*! Controls are added here, rather than waiting for init().
 *
 * pcur, pmin, and pmax are only used when SIMPP_PRINTRANGE is set.
 */
SimplePrint::SimplePrint(unsigned long nstyle,unsigned long nowner,const char *nsend,
						 const char *file, const char *command,
						 const char *thisfile,
						 int ntof, //!< Whether to start assuming print to file selected
						 int pmin, //!< The minimum page of the range
						 int pmax, //!< The maximum page of the range
						 int pcur) //!< The current page
	: MessageBox(NULL,"Print","Print",nstyle, 0,0,300,200,0,
			NULL,nowner,nsend, _("Print:"))
{
	printthis=newstr(thisfile);
	tofile=ntof;
	anXWindow *last=NULL;
	if (!file) file="output.ps";
	if (!command) command="lp";
	
	//	CheckBox(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
	//						int xx,int yy,int ww,int hh,int brder,
	//						anXWindow *prev,Window nowner,const char *nsendmes,
	//						const char *nname=NULL,int npadx=0,int npady=0);
	last=filecheck=new CheckBox(this,"ps-tofile",NULL,CHECK_CIRCLE|CHECK_LEFT, 
						 0,0,0,0,0, 
	 					 last,object_id,"ps-tofile-check",
						 _("To File: "), 0,5);
	filecheck->State(LAX_ON);
	AddWin(filecheck,1,-1);

	//	LineEdit(anXWindow *parnt,const char *ntitle,unsigned int nstyle,
	//			int xx,int yy,int ww,int hh,int brder,
	//			anXWindow *prev,Window nowner=None,const char *nsend=NULL,
	//			const char *newtext=NULL,unsigned int ntstyle=0);
	last=fileedit=new LineEdit(this,"ps-tofile-le",NULL,0, 
						 0,0,100,20, 1,
						 last,object_id,"ps-tofile-le",
						 file,0);
	fileedit->padx=5;
	AddWin(fileedit,1, fileedit->win_w,0,1000,50,0, fileedit->win_h,0,0,50,0, -1);
	AddNull();
	
	last=commandcheck=new CheckBox(this,"ps-command",NULL,CHECK_CIRCLE|CHECK_LEFT,
						 0,0,0,0,0, 
						 last,object_id,"ps-command-check",
						 _("By Command: "), 0,5);
	commandcheck->State(LAX_OFF);
	AddWin(commandcheck,1,-1);

	last=commandedit=new LineEdit(this,"ps-command-le",NULL,0, 
						 0,0,100,20, 1,
						 last,object_id,"ps-command-le",
						 command,0);
	commandedit->padx=5;
	AddWin(commandedit,1, commandedit->win_w,0,1000,50,0, commandedit->win_h,0,0,50,0, -1);
	AddNull();
	
	AddSpacer(0,0,9999,50, 12,0,0,50);
	AddNull();
	min=pmin;
	max=pmax;
	cur=pcur;
	if (win_style&SIMPP_PRINTRANGE) {

		last=printall=new CheckBox(this,"ps-printall",NULL,CHECK_CIRCLE|CHECK_LEFT,
							 0,0,0,0,0, 
							 last,object_id,"ps-printall",
							 _("Print All"), 0,5);
		printall->State(LAX_ON);
		AddWin(printall,1, win_w,0,2000,0,0, 20,0,0,50,0, -1);
		AddNull();

		last=printcurrent=new CheckBox(this,"ps-printcurrent",NULL,CHECK_CIRCLE|CHECK_LEFT,
							 0,0,0,0,0, 
							 last,object_id,"ps-printcurrent",
							 _("Print Current"), 0,5);
		printcurrent->State(LAX_OFF);
		AddWin(printcurrent,1, win_w,0,2000,0,0, 20,0,0,50,0, -1);
		AddNull();

		last=printrange=new CheckBox(this,"ps-printrange",NULL,CHECK_CIRCLE|CHECK_LEFT,
							 0,0,0,0,0, 
							 last,object_id,"ps-printrange",
							 _("Print From:"), 0,5);
		printrange->State(LAX_OFF);
		AddWin(printrange,1,-1);

		char blah[15];
		sprintf(blah,"%d",pmin);
		last=printstart=new LineEdit(this,"ps-printstart",NULL,0, 
							 0,0,100,20, 1,
							 last,object_id,"ps-printstart",
							 blah,0);
		printstart->padx=5;
		AddWin(printstart,1, printstart->win_w,0,1000,50,0, printstart->win_h,0,0,50,0, -1);
			
		AddWin(new MessageBar(this,"ps-to",NULL,0, 0,0,0,0,0, "To:"),1,-1);
		sprintf(blah,"%d",pmax);
		last=printend=new LineEdit(this,"ps-printend",NULL,0, 
							 0,0,100,20, 1,
							 last,object_id,"ps-printend",
							 blah,0);
		printend->padx=5;
		AddWin(printend,1, printend->win_w,0,1000,50,0, printend->win_h,0,0,50,0, -1);
		AddNull();
		AddWin(NULL,0, 0,0,9999,50,0, 12,0,0,50,0, -1);
		AddNull();
	} else {
		printall=printcurrent=printrange=NULL;
		printstart=printend=NULL;
	}

	last=AddButton(BUTTON_PRINT,last);
	last=AddButton(BUTTON_CANCEL,last);
	last->CloseControlLoop();
}

//! Make sure the kid windows have this as owner.
int SimplePrint::init()
{
	WinFrameBox *wb;
	for (int c=0; c<wholelist.n; c++) {
		wb=dynamic_cast<WinFrameBox *>(wholelist.e[c]);
		if (wb && wb->win()) wb->win()->SetOwner(this);
	}
	return MessageBox::init();
}

//! Keep the controls straight.
/*! \todo Changes to printstart and end use the c function atoi()
 * and do not check for things like "1ouaoeao" which it sees as 1.
 */
int SimplePrint::Event(const EventData *e,const char *mes)
{
	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);

	int d=1;
	if (!strcmp(mes,"ps-tofile-check")) {
		if (s->info1==LAX_OFF) {
			 // turn it back on
			filecheck->State(LAX_ON);
			return 0;
		}
		 //else turn off other
		commandcheck->State(LAX_OFF);
		changeTofile(1);
		return 0;

	} else if (!strcmp(mes,"ps-command-check")) {
		if (s->info1==LAX_OFF) {
			 // turn it back on
			commandcheck->State(LAX_ON);
			return 0;
		}
		 //else turn off other
		filecheck->State(LAX_OFF);
		changeTofile(0);
		return 0;

	} else if (!strcmp(mes,"ps-printall")) {
		printrange->State(LAX_OFF);
		printcurrent->State(LAX_OFF);
		printall->State(LAX_ON);
		return 0;

	} else if (!strcmp(mes,"ps-printcurrent")) {
		printrange->State(LAX_OFF);
		printcurrent->State(LAX_ON);
		printall->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"ps-printrange")) {
		printrange->State(LAX_ON);
		printcurrent->State(LAX_OFF);
		printall->State(LAX_OFF);
		return 0;

	} else if (!strcmp(mes,"ps-printstart")) {
		int a=atoi(printstart->GetCText()),
			b=atoi(printend->GetCText());
		char blah[15];
		if (a<min) {
			sprintf(blah,"%d",min);
			printstart->SetText(blah);
		} else {
			if (a>max) {
				a=b=max;
				sprintf(blah,"%d",a);
				printstart->SetText(blah);
				printend->SetText(blah);
			}
			if (a>b) {
				sprintf(blah,"%d",a);
				printend->SetText(blah);
			}
		}
		return 0;

	} else if (!strcmp(mes,"ps-printend")) {
		int a=atoi(printstart->GetCText()),
			b=atoi(printend->GetCText());
		char blah[15];
		if (b>max) {
			sprintf(blah,"%d",max);
			printend->SetText(blah);
		} else {
			if (b<min) {
				a=b=min;
				sprintf(blah,"%d",a);
				printstart->SetText(blah);
				printend->SetText(blah);
			}
			if (b<a) {
				sprintf(blah,"%d",b);
				printstart->SetText(blah);
			}
		}
		return 0;

	} else if (!strcmp(mes,"ps-tofile-le")) {
		Print();

	} else if (!strcmp(mes,"ps-command-le")) {
		Print();

	} else if (s && s->info2==BUTTON_PRINT) {
		Print();

	} else if (s && s->info2==BUTTON_CANCEL) {
		//make sure d stays 1
	} else d=0;

	if (d) app->destroywindow(this);

	return anXWindow::Event(e,mes);
}

//! This allows special things to happen when a different printing target is selected.
void SimplePrint::changeTofile(int t)
{
	tofile=t;
}

//! Run the command, or do to file, shut this box down, and send notification to owner.
/*! If printthis==NULL then send a StrEventData with 
 * data->info=tofile and the str=command or file. 
 *
 * Otherwise, run the command or copy printthis to the chosen file. WARNING: the
 * command is not currently checked for offensive commands. Whatever the command in
 * the window is, "lp" say, then the command executed is "lp [printthis]".
 *
 * For printthis!=NULL and tofile, copy printthis to the chosen file.
 * 
 * If win_style&SIMPP_DEL_PRINTTHIS then delete printthis after it's done,
 * the calling window must respond to the StrEventData.
 * The file is not copied here to try to enforce an overwrite check....?
 * 
 * \todo *** printthis!=NULL needs testing/debugging
 */
int SimplePrint::Print()
{
	if (printthis) {
		if (tofile) {
			cout <<"*** must implement copy printthis to chosen file"<<endl;
			//if (win_style&SIMPP_DEL_PRINTTHIS) {
				//if (link(printthis)==0) {//link does not overwrite
					 //unlink(printthis);
				//} else ***send error
			//} else {
				//*** do copy
			//}
		} else {
			char *blah=newstr(commandedit->GetCText());
			appendstr(blah," ");
			appendstr(blah,printthis);
			system(blah);
			DBG cerr << "------ print with this: \""<<blah<<"\""<<endl;
			delete[] blah;
			if (win_style&SIMPP_DEL_PRINTTHIS) unlink(printthis);
			return 0;
		}
	}
	
	StrEventData *data=new StrEventData;
	data->info1=data->info2=data->info3=-1;
	if (win_style&SIMPP_PRINTRANGE) {
		if (printcurrent->State()==LAX_ON) {
			data->info2=data->info3=cur;
		} else if (printall->State()==LAX_ON) {
			data->info2=min;
			data->info3=max;
		} else {
			data->info2=atoi(printstart->GetCText());
			data->info3=atoi(  printend->GetCText());
		}
	}

	if (tofile) {
		DBG cerr << " print to file: \""<<fileedit->GetCText()<<"\""<<endl;
		data->info1=1;
		data->str=newstr(fileedit->GetCText());
		app->SendMessage(data,win_owner,win_sendthis,object_id);
		return 0;
	} else {
		DBG cerr << " print with: \""<<commandedit->GetCText()<<"\""<<endl;
		data->info1=0;
		data->str=newstr(commandedit->GetCText());
		app->SendMessage(data,win_owner,win_sendthis,object_id);
		return 0;
	}
}

//! Character input.
/*! ESC  cancel this dialog, and don't print.
 */
int SimplePrint::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		app->destroywindow(this);
		return 0;
	}
	return 1;
}

} // namespace Laxkit

