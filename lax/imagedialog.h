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
	LineInput *file,*preview,*titlee,*side;
	FilePreviewer *previewer;
	ImageInfo *imageinfo;
	unsigned int dialog_style;
	virtual FilePreviewer *newFilePreviewer();
	virtual char *reallyGeneratePreview();
	virtual int RegeneratePreview(int force);
	virtual void updateImageInfo();
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

	//virtual void dump_out(FILE *f,int indent,int what);
	//virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what);
	//virtual void dump_in_atts(LaxFiles::Attribute *att,int flag);
};

} //namespace Laxkit

#endif

