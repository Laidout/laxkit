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
//   Copyright (c) 2004-2009 Tom Lechner
//
#ifndef _LAX_IMAGE_DIALOG_H
#define _LAX_IMAGE_DIALOG_H

#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include <lax/multilineedit.h>
#include <lax/filepreviewer.h>
#include <lax/imageinfo.h>
#include <lax/dump.h>

namespace Laxkit {


//------------------------------ ImageDialog ------------------------------------

#define IMGD_NO_FINAL_BUTTONS   (1<<16)
#define IMGD_NO_TITLE           (1<<17)
#define IMGD_NO_DESCRIPTION     (1<<18)
#define IMGD_NO_DESTROY         (1<<19)
#define IMGD_SEND_STRS          (1<<20)

class ImageDialog : public RowFrame
{
 protected:
	//FileLineInput *file,*preview;
	MultiLineEdit *desc;
	LineInput *file, *preview, *titlee, *side, *index;
	FilePreviewer *previewer;
	ImageInfo *imageinfo;
	unsigned int dialog_style;
	virtual FilePreviewer *newFilePreviewer();
	virtual char *reallyGeneratePreview();
	virtual int RegeneratePreview(int force);
	virtual void updateImageInfo();
	bool SetImageIndex(int i);

	virtual char **GetPossiblePreviewFiles();
	
 public:
	ImageDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend,
		unsigned long ndstyle, ImageInfo *inf);
	virtual ~ImageDialog();
	virtual const char *whattype() { return "ImageDialog"; }
	virtual int preinit();
	virtual int init();
	virtual int send();
	virtual int Event(const EventData *data,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual void closeWindow();
};

} //namespace Laxkit

#endif

