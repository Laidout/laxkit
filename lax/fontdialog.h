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
//    Copyright (C) 2015 by Tom Lechner
//

#ifndef _LAX_FONTDIALOG_H
#define _LAX_FONTDIALOG_H

#include <lax/rowframe.h>
#include <lax/lineinput.h>
#include <lax/menuselector.h>
#include <lax/numslider.h>


namespace Laxkit {


//-------------------------------------- FontDialogFont -------------------------------------
/*! \class FontDialogFont
 * \brief Describes a font as dealt with in a FontDialog.
 */
class FontDialogFont
{
 public:
	int id;
	char *family;
	char *style;
	char *file;
	double size;
	LaxImage *preview;

	FontDialogFont(int nid);
	virtual ~FontDialogFont();
	virtual bool Match(const char *mfamily, const char *mstyle);
};


//-------------------------------------- FontDialog -------------------------------------

#define FONTD_NO_DEL_WIN (1<<16)

class FontDialog : public RowFrame
{
 protected:
	char *sampletext;
	FcConfig *fcconfig;
	double defaultsize;
	unsigned long dialog_style;
	PtrStack<FontDialogFont> fonts;
	int currentfont;

	char *origfamily, *origstyle;

	MenuSelector *fontlist;
	LineEdit *text;
	LineInput *fontfamily, *fontstyle, *fontfile;
	LineInput *search;
	NumSlider *fontsize;

 public:
	FontDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
				int xx,int yy,int ww,int hh,int brder,
				unsigned long nowner,const char *nsend,
				unsigned long ndstyle,
				const char *fam, const char *style, double size);
	virtual ~FontDialog();

	virtual const char *whattype() { return "FontDialog"; }
	virtual int init();
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int Event(const EventData *data,const char *mes);
	
	virtual int FindFont();
	virtual int send();
	virtual void UpdateStyles();
	virtual void UpdateSample();
	virtual int SampleText(const char *ntext);
};


} // namespace Laxkit

#endif

