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
//    Copyright (C) 2004-2010 by Tom Lechner
//


#include <lax/printdialog.h>
#include <lax/lineinput.h>
#include <lax/sliderpopup.h>
#include <lax/messagebar.h>
#include <lax/checkbox.h>
#include <lax/language.h>

#include <iostream>
using namespace std;

#define DBG


namespace Laxkit {

//*** get ppd options, modifying as necessary
//select which printer to use, name/instance
//ppd options
// group 1
//  options...
// group 2 
//  options...
//move/rotate/scale on page
//__ All
//__ Current page
//__ from ___ to ___
//# of copies
//dpi
//medium type
//paper size
//gray/line/rgb/cmyk
//printer profile


//--------------------------------- Print Something ------------------------------

/*! Return 0 for success or nonzero error.
 */
int Print(const char *filename, //!< Currently must be a pdf.
		  cups_dest_t *dest,
		  cups_dinfo_t *dest_info,
		  const char *title, //!< Such as "My Document"
		  int num_copies
		 )
{
	//first create a cups job
	char scratch[100];
	int job_id = 0;
	int num_options = 0;
	cups_option_t *options = NULL;
	sprintf(scratch, "%d", num_copies);

	num_options = cupsAddOption(CUPS_COPIES, scratch,
	                            num_options, &options); //if num_options==0, creates a new array
	num_options = cupsAddOption(CUPS_MEDIA, CUPS_MEDIA_LETTER,
	                            num_options, &options);
	num_options = cupsAddOption(CUPS_SIDES,
	                            CUPS_SIDES_TWO_SIDED_PORTRAIT,
	                            num_options, &options);

	ipp_status_t status = cupsCreateDestJob(CUPS_HTTP_DEFAULT,
				  dest,
                  dest_info,
                  &job_id, //gets assigned during call
                  title,
                  num_options,
                  options //see cupsAddOption()
                  );

	if (status != IPP_STATUS_OK) {
		//no!!!!
		// *** make error message
		return 1;
	}

	//------now send the file data to the job
	const char *format = nullptr;
	format = CUPS_FORMAT_PDF;
	// format = CUPS_FORMAT_POSTSCRIPT;
	// format = CUPS_FORMAT_JPEG;
	// format = CUPS_FORMAT_TEXT;

	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		// *** Error!!
		return 2;
	}

	size_t bytes;
	char buffer[65536];
	int errored = 0;

	try {
		http_status_t pstatus = cupsStartDestDocument(CUPS_HTTP_DEFAULT,
							  dest,
		                      dest_info,
		                      job_id,
		                      filename, //name of doc, "typically the original filename"
		                      format, //mime type of original document.. ok for null
		                      num_options,
		                      options,
		                      1); // last_document: 1 if this request is the last document for job

		if (pstatus == HTTP_STATUS_CONTINUE) {
			while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
			    if (cupsWriteRequestData(CUPS_HTTP_DEFAULT, buffer, bytes) != HTTP_STATUS_CONTINUE)
			    	break;
			}

			if (cupsFinishDestDocument(CUPS_HTTP_DEFAULT, dest, dest_info) == IPP_STATUS_OK) {
				DBG cerr << "Print job sent to printer"<<endl;
			} else {
				throw cupsLastErrorString();
			}

		} else {
			// could not start dest! fail!
			throw _("Could not start print job!");
		}

	} catch (const char *msg) {
		DBG cerr << "Print error: "<<(msg?msg:"")<<endl;
		errored = 2;

	} catch (exception& e) {
		// *** some other error
		DBG cerr << "Some kind of print error: "<<e.what() <<endl;
		errored = 1;
	}

	fclose(fp);
	return errored;
}


//--------------------------------- PrintDialog ------------------------------
/*! \class PrintDialog
 * \brief Dialog to get some sort of context for printing.
 *
 * Default is to use Cups to figure out destinations and options.
 * 
 * If no file to print is passed in, then the control message sent is a 
 * PrintContext object, and must be received in a DataEvent() function.
 *
 * ***Currently, everything is dumped into a single window. When there are a million
 * driver options, not all get displayed on the screen. Need to finish implementing
 * ScrolledWindow. An alternate layout could be a tabbed window with "General"
 * on one tab, and "Driver Options" on the other, or button to popup driver settings,
 * like Scribus has.
 * 
 * \todo Need a CancelPrint dialog class, initiated when printing commences...
 */

	
PrintDialog::PrintDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						 int xx,int yy,int ww,int hh,int brder,
						 anXWindow *prev, unsigned long  nowner,const char *nsend,
						 unsigned int dialog_flags,
						 const char *nfiletoprint, PrintContext *pt)//nfiletoprint=NULL,pt=NULL
	: RowFrame(parnt,nname,ntitle,(nstyle&ANXWIN_MASK)|ROWFRAME_ROWS|ROWFRAME_CENTER,
			   xx,yy,ww,hh,brder, prev,nowner,nsend,
			   5)
{
	// ppd=NULL;

	dests      = NULL;
	// options    = NULL;
	numoptions = 0;
	numdests   = 0;
	current_printer = 0;
	optionsstart    = 0;
	this->dialog_flags = dialog_flags;

	to_file_path = nullptr;
	filetoprint  = newstr(nfiletoprint);
	printcontext = pt;

	setup();
}

PrintDialog::~PrintDialog()
{
	delete[] to_file_path;
	delete[] filetoprint;
	
	cupsFreeDests(numdests,dests);
	// cupsFreeOptions(numoptions,options);

	// if (ppd) ppdClose(ppd);
}

//! Get all possible printer destinations...
void PrintDialog::setup()
{
	//----find available printers:
	numdests = cupsGetDests2(CUPS_HTTP_DEFAULT, &dests);

	//-----just keeping this block around for reference:
	DBG cerr << "-----------Printer Info--------------"<<endl;
	DBG if (numdests) {
	DBG 	for (int c=0; c<numdests; c++) {
	DBG 		cerr << "dest "<<c<<": "<<dests[c].name;
	DBG 		if (dests[c].instance) cerr<<"/"<<dests[c].instance;
	DBG 		cerr<<endl;
	DBG 	}
	DBG } else {
	DBG 	cerr << "No dests!"<<endl;
	DBG }
	
	DBG cerr << "options:"<<endl;
	DBG for (int c=0; c<dests[current_printer].num_options; c++) {
	DBG 	cerr << "  "<<dests[current_printer].options[c].name<<"="<<dests[current_printer].options[c].value<<endl;
	DBG }
	DBG cerr << "-----------specific options:--------------"<<endl;

	cups_dinfo_t *dest_info = cupsCopyDestInfo(CUPS_HTTP_DEFAULT, &(dests[0])); // **** does this need to be freed?

	if (cupsCheckDestSupported(CUPS_HTTP_DEFAULT, &(dests[0]), dest_info,
	                           CUPS_FINISHINGS, NULL))
	{
		ipp_attribute_t *finishings = cupsFindDestSupported(CUPS_HTTP_DEFAULT, &dests[0], dest_info, CUPS_FINISHINGS);
		int count = ippGetCount(finishings);

		cerr << "  finishings supported: "<<count<<endl;
		for (int i = 0; i < count; i ++)
			cerr << "    "<< ippGetInteger(finishings, i)<<endl;
	}
	else cerr<< "  finishings not supported."<<endl;

	//types of "standard" options... what about weird custom printer options??? used to be able to query with ppd!
	//  CUPS_COPIES
	//  CUPS_FINISHING -> comma separated list of mystery ints
	//  CUPS_MEDIA
	//  CUPS_MEDIA_SOURCE
	//  CUPS_MEDIA_TYPE
	//  CUPS_NUMBER_UP
	//  CUPS_ORIENTATION
	//  CUPS_PRINT_COLOR_MODE
	//  CUPS_PRINT_QUALITY
	//  CUPS_SIDES
	//    CUPS_SIDES_ONE_SIDED
	//    CUPS_SIDES_TWO_SIDED_PORTRAIT
	//    CUPS_SIDES_TWO_SIDED_LANDSCAPE
	//  

	//list all supported options
	ipp_attribute_t *attrs = cupsFindDestSupported(CUPS_HTTP_DEFAULT, &dests[0], dest_info, "job-creation-attributes");
	int count = ippGetCount(attrs);

	cerr << "  job-creation-attributes: "<<count<<endl;
	for (int i = 0; i < count; i ++) cerr << "    "<<ippGetString(attrs, i, NULL)<<endl;

	cupsFreeDestInfo(dest_info);
	DBG cerr << "-----------End Printer Info--------------"<<endl;

	//----find options
	// ppd=ppdOpenFile(cupsGetPPD(dests[current_printer].name));
	// if (!ppd) {
	// 	DBG cerr <<"--Error! You need to have cups installed for this to work."<<endl;
	// 	return;
	// }
	// ppdMarkDefaults(ppd);
	// cupsMarkOptions(ppd,dests[current_printer].num_options,dests[current_printer].options);

	
	//int job=cupsPrintFile(dests[current_printer].name,filetoprint,"filename job title",dests[current_printer].num_options,&dests[current_printer].options);
	//int job=cupsPrintFiles(dests[current_printer].name,numfiles,files,"filename job title",dests[current_printer].num_options,&dests[current_printer].options);
	//cupsCancelJob(dests[current_printer].name,job);
	
}

/*! \todo maybe send note says sent to printer with job number?
 */
int PrintDialog::send()
{
	if (filetoprint==NULL) {
		//***send PrintContext
		printcontext=NULL;
		return 1;
	}
	return 1;
}

//! Set win_w and win_h to sane values if necessary.
int PrintDialog::preinit()
{
	anXWindow::preinit();
	if (win_w==0) win_w=500;
	if (win_h==0) {
		int textheight = win_themestyle->normal->textheight();
		win_h = 12 * (textheight + 7) + 20;
	}
	return 0;
}

int PrintDialog::init()
{
	LineInput *linp,*paperx,*papery;
	SliderPopup *strpop;
	Button *tbut;
	anXWindow *last=NULL;
	int c,c2=-1;
	double textheight = UIScale() * win_themestyle->normal->textheight();
	double linpheight = 1.2 * textheight;
	double CHECKGAP = textheight/4;

	// -----Print To
	AddWin(new MessageBar(this,"printtomes",NULL,MB_MOVE, 0,0,0,0,0, "Print To:"),1,-1);

	last = strpop = new SliderPopup(this,"printto",NULL,SLIDER_POP_ONLY, 0,0,0,0,1, NULL,object_id,"printto",NULL,0);
	char blah[300];//***check max name..
	for (c=0; c<numdests; c++) {
		if (dests[c].instance) sprintf(blah,"%s/%s",dests[c].name,dests[c].instance);
		else sprintf(blah,"%s",dests[c].name);

		strpop->AddItem(blah,c); //item,id,info
	}
	if ((dialog_flags & PRINT_NO_TO_FILE) == 0) {
		strpop->AddItem(_("To file"), -1);
	}
	//strpop->AddSep(); <-- implement!!
	//strpop->AddItem("Poscript file",NULL,c++);
	//strpop->AddItem("EPS file",NULL,c++);
	//strpop->AddItem("PDF file",NULL,c++);
	//strpop->AddItem("XPrint",NULL,c++);
	AddWin(strpop,1, 200,50,1000,50,0, linpheight,0,10,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak

	AddVSpacer(textheight/2,0,0,50);
	AddNull();

	 // -----Paper Size Y
	last=paperx=new LineInput(this,"paper x",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"paper x",
			            "Paper Size  x:","8.5?",0,
			            100,0);
	// const char *newlabel=NULL,const char *newtext=NULL,unsigned int ntstyle=0,
	// int nlew=0,int nleh=0, double npadx=-1,double npady=-1,double npadlx=-1,double npadly=-1);
	AddWin(paperx,1, paperx->win_w,0,50,50,0, linpheight,0,10,50,0, -1);
	
	 // -----Paper Size Y
	last=papery=new LineInput(this,"paper y",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"paper y",
			            "y:","11?",0,
			           100,0);
	AddWin(papery,1, papery->win_w,0,50,50,0, linpheight,0,10,50,0, -1);
	
	 // -----Paper Name
	last=strpop=new SliderPopup(this,"paperName",NULL,0, 0,0, 0,0, 1, last,object_id,"paper name",NULL,0);
	// ppd_option_t *option=ppdFindOption(ppd,"PageSize");
	// for (int c=0; c<option->num_choices; c++) {
	// 	if (option->choices[c].marked) c2=c;
	// 	strpop->AddItem(option->choices[c].text?option->choices[c].text:option->choices[c].choice,NULL,c);
	// }
	strpop->Select(c2);
	AddWin(strpop,1, 200,100,50,50,0, linpheight,0,10,50,0, -1);
	
	 // -----Paper Orientation
	last=strpop=new SliderPopup(this,"paperOrientation",NULL,0, 0,0, 0,0, 1, last,object_id,"orientation",NULL,0);
	strpop->AddItem("Portrait",NULL,0);
	strpop->AddItem("Landscape",NULL,1);
	strpop->Select(0);
	AddWin(strpop,1, 200,100,50,50,0, linpheight,0,10,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak


	 //-------------------- Number of copies
	last=linp=new LineInput(this,"copies",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"copies",
			            "Copies:","1",0,
			            100,0,1,1,3,3);
	AddWin(linp,1, linp->win_w,0,50,50,0, linpheight,0,10,50,0, -1);
	AddWin(NULL,0, 10000,10000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak
	
	
	 //-------------------- Current/All/From-To
	CheckBox *check;
	AddNull();
	last=check=new CheckBox(this,"CurrentPage",NULL,CHECK_LEFT, 0,0,0,0,1, 
							last,object_id,"CurrentPage","Current Page", CHECKGAP);
	AddWin(check,1, check->win_w,0,0,50,0, linpheight,0,10,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak

	last=check=new CheckBox(this,"AllPages",NULL,CHECK_LEFT, 0,0,0,0,1, last,object_id,"AllPages","All Pages",CHECKGAP);
	AddWin(check,1, check->win_w,0,0,50,0, linpheight,0,10,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak

	last=check=new CheckBox(this,"CurrentPage",NULL,CHECK_LEFT, 0,0,0,0,1, last,object_id,"PageRange","", CHECKGAP);
	AddWin(check,1, check->win_w,0,0,50,0, linpheight,0,10,50,0, -1);
	last=linp=new LineInput(this,"frompage",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"frompage",
			            "From:","1",0,
			            100,0,1,1,3,3);
	AddWin(linp,1, linp->win_w,0,50,50,0, linpheight,0,10,50,0, -1);
	last=linp=new LineInput(this,"topage",NULL,LINP_ONLEFT, 0,0,0,0, 0, 
						last,object_id,"topage",
			            "To:","10000",0,
			            100,0,1,1,3,3);
	AddWin(linp,1, linp->win_w,0,50,50,0, linpheight,0,10,50,0, -1);
	AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, -1);//*** forced linebreak
	

	 //add specific printer options
	optionsstart=wholelist.n;
	addOptions();


	 //-------------------- Final Print/Cancel ------------------------
	AddNull();
	last=tbut=new Button(this,"Print",NULL,BUTTON_PRINT,0,0,0,0,1,
						 last,object_id,"Print");
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,10,50,0, -1);
	AddWin(NULL,0, 20,0,0,50,0, 5,0,0,50,0, -1); // add space of 20 pixels
	last=tbut=new Button(this,"cancel",NULL,BUTTON_CANCEL,0,0,0,0,1,
						 last,object_id,"Cancel");
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,10,50,0, -1);

	
	last->CloseControlLoop();
	Sync(1);
//	wrapextent();
	return 0;
}


void PrintDialog::addOptions()
{
	cerr << " IMP ME!!! PrintDialog::addOptions"<<endl;
}


// ************** ppd funcs are deprecated in cups now:
// //! Add windows for all the driver options starting at position optionsstart.
// /*! This function does not sync the squishybox. It pops any boxes between index
//  * optionsstart and the final &quot;Print&quot; and &quot;Cancel&quot; buttons,
//  * replacing them with what are the current options.
//  * 
//  * These boxes are expected to be the options for the printer driver as
//  * returned by Cups.
//  */
// void PrintDialog::addOptions()
// {
// 	 // Pop old
// 	WinFrameBox *b;
// 	if (optionsstart>wholelist.n) optionsstart=wholelist.n;
// 	while (optionsstart!=wholelist.n) {
// 		b=dynamic_cast<WinFrameBox *>(wholelist.e[wholelist.n-1]);
		
// 		 // if box is the Print button, then break
// 		if (b && b->win() && b->win()->win_title && !strcmp("Print",b->win()->win_title)) break;
// 		wholelist.pop(optionsstart);
// 	}
	
// 	 // Push new
// 	int s=optionsstart,d,c,c2,c3;
// 	SliderPopup *strpop=NULL;
// 	const char *str;
// 	char place[100];
// 	if (ppd) {
// 		//DBG cerr << ppd->num_groups << " groups"<<endl;
// 		for (c=0; c<ppd->num_groups; c++) {
// 			//---ignoring group names for now...
// 			DBG cerr << "print options Group "<< (ppd->groups[c].name?ppd->groups[c].name:"(no name)")<<
// 			DBG 	"=\""<<(ppd->groups[c].text?ppd->groups[c].text:"(no text)")<<"\""<<endl;
			
// 			for (c2=0; c2<ppd->groups[c].num_options; c2++) {
// 				 // add option name label
// 				sprintf(place,"%d %d",c,c2);
// 				str=ppd->groups[c].options[c2].text;
// 				if (!str) str=ppd->groups[c].options[c2].keyword;
// 				if (!str) str="(error-no text)";
// 				AddWin(new MessageBar(this,"label",NULL,MB_MOVE, 0,0,0,0,0, str), 1, s++);
						
// 				 // add slider for the possible options values
// 				strpop=new SliderPopup(this,place,NULL,0, 0,0,0,0, 1,
// 									NULL,object_id,place, NULL,0);
// 				d=-1; //default option
// 				for (c3=0; c3<ppd->groups[c].options[c2].num_choices; c3++) {
// 					if (ppd->groups[c].options[c2].choices[c3].marked) d=c3;
// 					str=ppd->groups[c].options[c2].choices[c3].text;
// 					if (!str) str=ppd->groups[c].options[c2].choices[c3].choice;
// 					if (!str) str="(error-no text)";
// 					strpop->AddItem(str,NULL,c3);
// 				}
// 				strpop->Select(d);
// 				AddWin(strpop,1, 75,10,100,50,0, strpop->win_h,10,100,50,0, s++);
// 				//AddWin(strpop,1, strpop->win_w,10,100,50,0, strpop->win_h,10,100,50,0, s++);
// 				AddWin(NULL,0, 2000,2000,0,50,0, 0,0,0,0,0, s++); //forced linebreak

// 				if (c2>10) break;
// 			}
// 		}
// 	}
// }

/*! *** must respond to various controls to update any ppd settings..
 */
int PrintDialog::Event(const EventData *e,const char *mes)
{// ***
	if (!strcmp(mes,"Cancel")) {
		DBG cerr << "Cancel print dialog!"<<endl;
		app->destroywindow(this);
		return 0;
	}
	return anXWindow::Event(e,mes);
}

//! Character input.
int PrintDialog::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
//	if (ch==LAX_Esc) {
//		app->destroywindow(this);
//		return 0;
//	}
	return anXWindow::CharInput(ch,buffer,len,state,d);
}

} //namespace Laxkit

