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
//   Copyright (c) 2005-2009 Tom Lechner
//
#include <lax/imagedialog.h>
#include <lax/filedialog.h>
#include <lax/fileutils.h>
#include <lax/button.h>
#include <lax/multilineedit.h>
#include <lax/laximages.h>
#include <lax/overwrite.h>
#include <lax/language.h>

#include <sys/stat.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//-------------------------------- ImageDialog ------------------------------

/*! \class ImageDialog 
 *  \brief Class to allow viewing and changing info related to an image.
 *
 *  This includes the actual file (which need not actually be an image file), a
 *  separate preview file for the main file, a description for the file, a title,
 *  and a button to generate or regenerate a smaller preview file.
 *
 *  \code
 *   #define IMGD_NO_FINAL_BUTTONS   (1<<16)
 *   #define IMGD_NO_TITLE           (1<<17)
 *   #define IMGD_NO_DESCRIPTION     (1<<18)
 *   #define IMGD_NO_DESTROY         (1<<19) <-- no app->destroywindow() in closewindow()
 *   #define IMGD_SEND_STRS          (1<<20) <-- send StrsEventData, not RefCountedEventData
 *  \endcode
 *
 *  \todo implement tag editing, must switch ImageInfo to a Tagged subclass
 */


//! Constructor.
/*! 
 *  Note that the previewer is not created, and the windows other than file, path and mask
 *  are not created until init(). All initial window layout is done in init().
 *
 * The count of inf will be incremented if not nullptr. If it is nullptr, then imageinfo gets
 * a new ImageInfo with a count of 1.
 */
ImageDialog::ImageDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend,
		unsigned long ndstyle,
		ImageInfo *inf)
	: RowFrame(parnt,nname,ntitle,(nstyle&0xffff)|ROWFRAME_ROWS|ROWFRAME_CENTER,
			   xx,yy,ww,hh,brder, prev,nowner,nsend,
			   5)
{
	dialog_style = ndstyle;
	
	imageinfo = inf;
	if (imageinfo) imageinfo->inc_count(); else imageinfo = new ImageInfo();

	titlee = file = preview = side = nullptr;
	desc = nullptr;
}

/*! Dec count of imageinfo.
 */
ImageDialog::~ImageDialog() 
{
	if (imageinfo) imageinfo->dec_count();
	imageinfo=nullptr;
}

//! Return a new FilePreviewer instance.
/*! This is called in init(). Subclasses can redefine this to provide their
 * own file previewer.
 */
FilePreviewer *ImageDialog::newFilePreviewer()
{
	return new FilePreviewer(this,"previewer",nullptr,
									 MB_MOVE|FILEPREV_SHOW_DIMS,
									 0,0,0,0, 1, imageinfo->filename);
}

//! Set win_w and win_h to sane values if necessary.
int ImageDialog::preinit()
{
	anXWindow::preinit();
	if (win_w==0) win_w=500;
	if (win_h==0) {
		int textheight = win_themestyle->normal->textheight();
		win_h = 12 * (textheight + 7) + 20;
	}
	return 0;
}

/*! Add dialogs in this order: 
 *    FilePreviewer, 
 *    file, preview,
 *    preview generate button,
 *    title,
 *    description,
 *    final ok and cancel.
 */
int ImageDialog::init()
{
	if (!xlib_window) return 1;

	int        textheight = win_themestyle->normal->textheight();
	int        linpheight = textheight + 6;
	anXWindow *last       = nullptr;
	Button *   tbut       = nullptr;

	//--------------- FilePreviewer
	last=previewer=newFilePreviewer();
	//***warning, hack!:
	previewer->Preview(imageinfo->filename);
	AddWin(previewer,1, 100,50,1600,50,0, 30,0,1600,50,0, -1);
	AddNull();

	 //---------- File
	last=tbut=new Button(this,"preview file",nullptr,0, 0,0,0,0, 1, 
						last,object_id,"preview file",
						0,_("File"),nullptr,nullptr,
						3,3);
	tbut->tooltip(_("Display file above"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	//-------
	last=file   =new LineInput(this,"file",_("File"),LINP_FILE | LINP_SEND_ANY,
							    0,0,0,0, 1, last,object_id,"new file", " ",imageinfo->filename);
	file->GetLineEdit()->SetCurpos(-1);
	file->tooltip("Filename to use");
	AddWin(file,1, 200,100,1000,50,0, file->win_h,0,0,50,0, -1);
	AddNull();
	
	 //------------ Preview
	last=tbut=new Button(this,"preview preview",nullptr,0, 0,0,0,0, 1, 
						last,object_id,"preview preview",
						0,_("Preview"),nullptr,nullptr,
						3,3);
	tbut->tooltip(_("Display preview file above"));
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	//----
	last=preview=new LineInput(this,"preview",_("Preview"),LINP_FILE, 0,0,0,0, 1, last,object_id,"new preview", " ",imageinfo->previewfile);
	preview->GetLineEdit()->SetCurpos(-1);
	preview->tooltip("The image's preview file, if any");
	AddWin(preview,1, 200,100,1000,50,0, preview->win_h,0,0,50,0, -1);
	AddNull();

	 //------------ [Re]Generate button
	int p=file_exists(imageinfo->previewfile,1,nullptr);
	last=tbut=new Button(this,"generate",nullptr,0, 0,0,0,0, 1, 
						last,object_id,"generate",
						0,
						p?_("Regenerate preview:"):_("Generate preview:"), nullptr,nullptr,
						3,3);
	if (p && p!=S_IFREG) tbut->State(LAX_GRAY);
	AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	 // add field for max preview dimension
	last=side=new LineInput(this,"side",nullptr,0,    0,0,0,0, 1, last,object_id,"new max side", "Fit to:","200");
	last->tooltip("Generate a preview inside a square this wide in pixels");
	AddWin(last,1, linpheight*3,linpheight,10,50,0, last->win_h,0,0,50,0, -1);
	//last=new LineInput(this,"height",0,    0,0,0,0, 1, last,object_id,"new max height", "Height:","200");
	//last->tooltip("The maximum pixel height for the preview");
	//AddWin(last,1, linpheight*3,linpheight,10,50, last->win_h,0,0,50, -1);
	AddNull();
	
	
	 //------------ Title
	if (!(dialog_style&IMGD_NO_TITLE)) {
		last=titlee =new LineInput(this,"preview",nullptr,0, 0,0,0,0, 1, last,object_id,"new title", "Title",imageinfo->title);
		titlee->tooltip("The image's title");
		AddWin(titlee,1, 200,100,1000,50,0, titlee->win_h,0,0,50,0, -1);
		AddNull();
	}
	
	 //------------ Description
	if (!(dialog_style&IMGD_NO_DESCRIPTION)) {
		AddWin(new MessageBar(this,"desc mesbar",nullptr,MB_MOVE, 0,0,0,0,1, "Description:"),1,-1);
		AddNull();
		last=desc   =new MultiLineEdit(this,"desc",nullptr,0,    0,0,0,0, 1, last,object_id,"new desc", 0,imageinfo->description);
		desc->tooltip("The image's description");
		AddWin(desc,1, 200,100,1000,50,0, 2*linpheight,0,0,50,0, -1);
		AddNull();
	}
	


	

	 //--------- final Ok/Cancel

	if (!(dialog_style&IMGD_NO_FINAL_BUTTONS)) {
		last=tbut=new Button(this,"fd-Ok",nullptr,BUTTON_OK, 0,0,0,0, 1, 
							last,object_id,"ok", 0,nullptr,nullptr,nullptr,3,3);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
		last=tbut=new Button(this,"fd-cancel",nullptr,BUTTON_CANCEL, 0,0,0,0, 1, 
						last,object_id,"cancel", 0,nullptr,nullptr,nullptr,3,3);
		AddWin(tbut,1, tbut->win_w,0,50,50,0, linpheight,0,0,50,0, -1);
	}

	last->CloseControlLoop();
	Sync(1);

	return 0;
}

//! Send an event to owner.
/*! If dialog_style&IMGD_SEND_STRS, sends a StrsEventData, 
 * with 4 strings: file, preview, title, and description.
 * Even if any of those four are optioned out, there is still a placeholder. That is,
 * file is still the first string, and description is always the fourth.
 *
 * Otherwise, send a RefCountedEventData with imageinfo as the object.
 *
 * Note that closeWindow() is not called from here. It is a separate action.
 *
 * Returns 0 if nothing sent, else nonzero.
 */
int ImageDialog::send()
{
	updateImageInfo();

	if (dialog_style&IMGD_SEND_STRS) {
		StrsEventData *e=new StrsEventData();
		char **strs;
		strs=new char*[5];
		strs[0]=newstr(imageinfo->filename);
		strs[1]=newstr(imageinfo->previewfile);
		strs[2]=newstr(imageinfo->title);
		strs[3]=newstr(imageinfo->description);
		strs[4]=nullptr;
		e->strs=strs;
		e->n=4;
		anXApp::app->SendMessage(e,win_owner,win_sendthis,0);

	} else {
		imageinfo->mask=~0;
		RefCountedEventData *event=new RefCountedEventData(imageinfo);
		anXApp::app->SendMessage(event,win_owner,win_sendthis,0);
	}
	
	return 0;
}

/*! Empty placeholder for subclasses to do something special when the window is due
 *  to shut down.
 */
void ImageDialog::closeWindow()
{
	if (!(dialog_style&IMGD_NO_DESTROY)) app->destroywindow(this);
}

//! Respond to the various controls
/*! \todo should probably make the imageinfo fields set to nullptr if
 *    the corresponding edits return blank strings or strings with only whitespace.
 */
int ImageDialog::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"-----image dialog got: "<<mes<<endl;

	if (!strcmp(mes,"reallyoverwrite")) {
		RegeneratePreview(1);
		return 0;

	} else if (!strcmp(mes,"install new file")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		makestr(imageinfo->filename,s->str);
		file->SetText(imageinfo->filename);
		previewer->Preview(imageinfo->filename);
		return 0;

	} else if (!strcmp(mes,"install new preview")) {
		const StrEventData *s=dynamic_cast<const StrEventData *>(data);
		if (!s) return 1;
		makestr(imageinfo->previewfile,s->str);
		preview->SetText(imageinfo->previewfile);
		previewer->Preview(imageinfo->previewfile);
		return 0;

	} else if (!strcmp(mes,"ok")) {
		send();
		closeWindow();
		return 0;

	} else if (!strcmp(mes,"cancel")) {
		closeWindow(); 
		return 0;

	} else if (!strcmp(mes,"new file")) {
		const char *f = file->GetCText();
		makestr(imageinfo->filename, f);
		previewer->Preview(imageinfo->filename);
		return 0;

	} else if (!strcmp(mes,"new preview")) {
		const char *prev=preview->GetCText();
		makestr(imageinfo->previewfile,prev);
		previewer->Preview(imageinfo->previewfile);
		return 0;

	} else if (!strcmp(mes,"new title")) {
		const char *t=titlee->GetCText();
		makestr(imageinfo->title,t);
		return 0;

	} else if (!strcmp(mes,"new desc")) {
		const char *t=desc->GetCText();
		makestr(imageinfo->description,t);
		return 0;

	} else if (!strcmp(mes,"preview file")) {
		const char *prev=file->GetCText();
		makestr(imageinfo->filename,prev);
		previewer->Preview(imageinfo->filename);
		return 0;

	} else if (!strcmp(mes,"preview preview")) {
		const char *prev=preview->GetCText();
		makestr(imageinfo->previewfile,prev);
		previewer->Preview(imageinfo->previewfile);
		return 0;

	} else if (!strcmp(mes,"generate")) {
		const char *prev=preview->GetCText();
		makestr(imageinfo->previewfile,prev);
		RegeneratePreview(0);
		return 0;

	} else if (!strcmp(mes,"get new file")) {
		app->rundialog(new FileDialog(nullptr,"get new file",nullptr,ANXWIN_REMEMBER,
									  0,0,400,500,0,
									  object_id,"install new file",
									  FILES_OPEN_ONE|FILES_PREVIEW,
									  file->GetCText()));
		return 0;

	} else if (!strcmp(mes,"get new preview")) {
		app->rundialog(new FileDialog(nullptr,"install new preview",nullptr,ANXWIN_REMEMBER,
									  0,0,400,500,0,object_id,"install new preview",
									  FILES_OPEN_ONE|FILES_PREVIEW,
									  preview->GetCText()));
		return 0;
	}

	return anXWindow::Event(data,mes);
}

//! Sync imageinfo to the window controls' contents.
void ImageDialog::updateImageInfo()
{
	const char *s=file->GetCText();
	makestr(imageinfo->filename,s);
	
	s=preview->GetCText();
	makestr(imageinfo->previewfile,s);
	
	if (titlee) {
		s=titlee->GetCText();
		makestr(imageinfo->previewfile,s);
	}
	
	if (desc) {
		s=desc->GetCText();
		makestr(imageinfo->description,s);
	}
}

//! Really make the preview, return an error string if something went wrong.
/*! The returned string, if any, is a new char[] and must be delete[]'d.
 *
 * Note that this will force an overwrite. For checking against that sort of thing,
 * use RegeneratePreview().
 *
 * \todo *** need check for fd thumb locations to enforce proper sizing
 */
char *ImageDialog::reallyGeneratePreview()
{
	long width = side->GetLineEdit()->GetLong(nullptr);
	if (width <= 10) return newstr("Too small to fit preview inside.");

	if (GeneratePreviewFile(imageinfo->filename,imageinfo->previewfile,"jpg",width,width,1))
		return newstr("Error making preview.");

	LaxImage *image = ImageLoader::LoadImage(imageinfo->previewfile);
	if (image) {
		image->dec_count();
	}

	previewer->Preview(imageinfo->previewfile);
	return nullptr;
}

//! Regenerate the preview. Ask to overwrite if force!=0 and imageinfo->previewfile exists already.
/*! This does checking to make sure it is ok to generate preview.
 * Return 0 for preview regenerated, 1 for overwrite dialog initiated, 
 *  and 2 for error in regeneration and pop up message box initiated.
 *
 *  If everything looks good, then call reallyGeneratePreview() to actually make the preview.
 */
int ImageDialog::RegeneratePreview(int force)
{
	int c=file_exists(imageinfo->previewfile,1,nullptr);
	if (c==S_IFREG && !force) {
		 //ask to overwrite
		anXWindow *ob=new Overwrite(object_id,"reallyoverwrite", imageinfo->previewfile, 0,0,0);
		app->rundialog(ob);
		return 1;
	}
	char *error=nullptr;
	if (c && c!=S_IFREG) {
		error=newstr("Cannot generate preview in:\n");
		appendstr(error,imageinfo->previewfile);
	}
	if (!error) {
		error=reallyGeneratePreview();
	}
	if (error) {
		previewer->SetText(error);
		delete[] error;
		return 2;
	}
	previewer->Preview(imageinfo->previewfile);
	return 0;
}

//! Cancel if ESC.
int ImageDialog::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==LAX_Esc) {
		closeWindow();
		return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state,d);
}

} //namespace Laxkit


