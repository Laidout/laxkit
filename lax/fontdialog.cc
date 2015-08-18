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
//    Copyright (C) 2007 by Tom Lechner
//

#include <lax/fontdialog.h>
#include <lax/strmanip.h>
#include <lax/lineinput.h>
#include <lax/button.h>
#include <lax/language.h>

#include <lax/lists.cc>

#define DBG
#include <iostream>
using namespace std;


//Select Font
//filter: bold italic size scalable favorites
//font family: __Times/preview___[v]    font style:___________  [optionalcolor]
//size:_____v
//effects:________  (centering, line spacing, etc)
//
//sample text  ->  choose fg and bg color
//multiline sample

namespace Laxkit {


//-------------------------------------- FontDialogFont -------------------------------------
/*! \class FontDialogFont
 * \brief Describes a font as dealt with in a FontDialog.
 */

FontDialogFont::FontDialogFont()
{
	family=style=file=NULL;
	size=12;
	preview=NULL;
}

FontDialogFont::~FontDialogFont()
{
	delete[] family;
	delete[] style;
	delete[] file;
	if (preview) preview->dec_count();
}


//-------------------------------------- FontDialog -------------------------------------
/*! \class FontDialog
 * \brief Dialog to allow selecting fonts.
 *
 * This will sent a StrsEventData with fields: family, style, file, size.
 *
 * fam, style, and size are default values to open the dialog to. If NULL or 0 are supplied,
 * then the dialog will select defaults.
 */
FontDialog::FontDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		unsigned long nowner,const char *nsend,
		unsigned long ndstyle,
		const char *fam, const char *style, int size)
	: RowFrame(parnt,nname,ntitle,(nstyle&0xffff)|ROWFRAME_ROWS|ROWFRAME_CENTER,
			   xx,yy,ww,hh,brder,
			   NULL,nowner,nsend,
			   5)
{
	dialog_style=ndstyle;
	defaultsize=12;
	currentfont=-1;

	sampletext=newstr("The quick brown fox etc");
}

FontDialog::~FontDialog()
{
	fonts.flush();
	FcConfigDestroy(fcconfig);

	if (sampletext) delete[] sampletext;
}

int FontDialog::init()
{

	 // Initialize FontConfig library, and read in all the font family names.
	 // The style and file names are found separately.
	FcResult result;
	FcValue v;
	
	FcInit(); //does nothing if FcInit() already called somewhere else
	fcconfig=FcInitLoadConfigAndFonts();
	FontDialogFont *f=NULL;
	FcFontSet *fontset=FcConfigGetFonts(fcconfig, FcSetSystem);

	for (int c=0; c<fontset->nfont; c++) {
		 // Usually, for each font family, there are several styles
		 // like bold, italic, etc.
		result=FcPatternGet(fontset->fonts[c],FC_FAMILY,0,&v);
		if (result!=FcResultMatch) continue;
		
		f=new FontDialogFont();
		makestr(f->family, (const char *)v.u.s);

		result=FcPatternGet(fontset->fonts[c],FC_STYLE,0,&v);
		if (result==FcResultMatch) makestr(f->style, (const char *)v.u.s);

		result=FcPatternGet(fontset->fonts[c],FC_FILE,0,&v);
		if (result==FcResultMatch) makestr(f->file, (const char *)v.u.s);

		DBG cout <<c<<", found font: Family,style,file: "<<f->family<<", "<<f->style<<", "<<f->file<<endl;

		fonts.push(f);
	}

	 //-------------build windows
	//int textheight=app->defaultfont->max_bounds.ascent+app->defaultfont->max_bounds.descent;
	//int linpheight=textheight+6;

	anXWindow *last=NULL;


	 //------search
	last=search=new LineInput(this,"search","search",0, 0,0,0,0, 1, last,object_id,"search", 
							_("Search"),NULL,0, 0,0,2,2,2,2);
	search->tooltip(_("Search among fonts"));
	AddWin(search,1, 200,100,1000,50,0, search->win_h,0,0,50,0, -1);
	AddNull();

	
	 //------font family
	 // *** type in box progressively limits what's displayed in list 
	 // *** should have selectors to group favorites or whatever
	last=fontfamily=new LineInput(this,"fontfamily","fontfamily",0, 0,0,0,0, 1, last,object_id,"fontfamily", 
							_("Family"),NULL,0, 0,0,2,2,2,2);
	fontfamily->tooltip(_("Family name of the font"));
	AddWin(fontfamily,1, 200,100,1000,50,0, fontfamily->win_h,0,0,50,0, -1);


	 //------font style
	last=fontstyle=new LineInput(this,"fontstyle","fontstyle",0, 0,0,0,0, 1, last,object_id,"fontstyle", 
							_("Style"),NULL,0, 0,0,2,2,2,2);
	fontstyle->tooltip(_("Style of the font"));
	AddWin(fontstyle,1, 200,100,1000,50,0, fontstyle->win_h,0,0,50,0, -1);


	 //-----font size
	//last=fontsize=new LineInput(this,"size","size",0, //LINP_FLOAT,
	//						0,0,0,0, 1, last,object_id,"fontsize", 
	//						_("Size"),NULL,0, 0,0,2,2,2,2);
	last=fontsize=new NumSlider(this,"size","size",0,
							0,0,0,0, 1, last,object_id,"fontsize", 
							_("Size"),0,1000000, 15);
	fontsize->tooltip(_("Size of the font"));
	//fontsize->SetText(app->defaultlaxfont->textheight());
	AddWin(fontsize,1, 200,100,1000,50,0, fontsize->win_h,0,0,50,0, -1);

	AddNull();
	 //------font file
	last=fontfile=new LineInput(this,"fontfile","fontstyle",LINP_FILE, 0,0,0,0, 1, last,object_id,"fontfile", 
							_("File"),NULL,0, 0,0,2,2,2,2);
	fontfile->tooltip(_("File of the font"));
	AddWin(fontfile,1, 200,100,2000,50,0, fontstyle->win_h,0,0,50,0, -1);
	AddNull();




	 //------font list
	MenuInfo *mfonts=new MenuInfo("Fonts");
	char str[1024];
	for (int c=0; c<fonts.n; c++) {
		sprintf(str,"%s, %s",fonts.e[c]->family,fonts.e[c]->style);
		mfonts->AddItem(str,c);
	}
	last=fontlist=new MenuSelector(this,"fonts","fonts",0,
									0,0,0,0,1,
									last,object_id,"font",
									MENUSEL_SEND_ON_UP
									 |MENUSEL_CURSSELECTS
									 |MENUSEL_TEXTCOLORS
									 |MENUSEL_LEFT
									 |MENUSEL_SUB_ON_LEFT
									 |MENUSEL_ONE_ONLY,
									mfonts,1);
	//fontlist->tooltip(_("Select one of these"));
	AddWin(fontlist,1, 200,100,1000,50,0, 30,0,1000,50,0, -1);

	AddNull();



	 //-----sample text
	// *** todo be able to change fg/bg
	last=text=new LineEdit(this,"sample","sample",0,
								0,0,0,0,0,
								last,object_id,"sample",
								sampletext,TEXT_CENTER);
	AddWin(text,1, 200,100,1000,50,0, app->defaultlaxfont->textheight()*2,0,0,50,0, -1);
	AddNull();



	 //--------final ok and cancel
	Button *tbut;
	last=tbut=new Button(this,"ok","ok",BUTTON_OK, 0,0,0,0, 1, 
			last,object_id,"ok", 0, NULL,NULL,NULL,3,3);
	AddWin(tbut,1, 200,100,1000,50,0, tbut->win_h,0,0,50,0, -1);

	last=tbut=new Button(this,"cancel","cancel",BUTTON_CANCEL, 0,0,0,0, 1, 
			last,object_id,"cancel", 0, NULL,NULL,NULL,3,3);
	AddWin(tbut,1, 200,100,1000,50,0, tbut->win_h,0,0,50,0, -1);




	last->CloseControlLoop();
	Sync(1);

	return 0;
}

void FontDialog::UpdateSample()
{
	LaxFont *newfont=app->fontmanager->MakeFont(fontfamily->GetCText(),fontstyle->GetCText(),fontsize->Value(), 0);
	if (!newfont) return;
	text->UseThisFont(newfont);
	newfont->dec_count();
}

int FontDialog::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"-----font dialog got: "<<mes<<endl;

	if (!strcmp(mes,"fontfamily")) { 
	} else if (!strcmp(mes,"fontstyle")) {
	} else if (!strcmp(mes,"fontsize")) {
		UpdateSample();
		return 0;

	} else if (!strcmp(mes,"font")) { 
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		int i=s->info1;
		if (i<0 || i>=fonts.n) return 0;
		fontfamily->SetText(fonts.e[i]->family);
		fontstyle ->SetText(fonts.e[i]->style);
		fontfile  ->SetText(fonts.e[i]->file);
		currentfont=i;
		UpdateSample();
		return 0;

	} else if (!strcmp(mes,"cancel")) {
		app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"ok")) {
		send();
		app->destroywindow(this);
		return 0;
	}

	return 0;
}

//! Sends a StrsEvent Data.
/*! Fields are: family, style, file, size.
 */
int FontDialog::send()
{
	if (currentfont<0 || !win_owner) return 1;

	StrsEventData *s=new StrsEventData;
	s->send_message=win_sendthis;
	s->to=win_owner;
	s->from=object_id;

	s->strs=new char*[4];
	s->n=4;
	s->strs[0]=NULL;
	makestr(s->strs[0],fonts.e[currentfont]->family);
	s->strs[1]=NULL;
	makestr(s->strs[1],fonts.e[currentfont]->style);
	s->strs[2]=NULL;
	makestr(s->strs[2],fonts.e[currentfont]->file);
	s->strs[3]=new char[30];
	sprintf(s->strs[3],"%f",fontsize->Valuef());

	app->SendMessage(s);
	return 0;
}

//! Called after a change to the font family, this updates the styles popup.
void FontDialog::UpdateStyles()
{
//	fontstylespopup.Clear();
//	FcPattern *pattern, *retpattern;
//	FcResult result;
//	
//	retpattern=FcFontMatch(fcconfig,pattern,&result);
//	for (int c=0; c<****; c++) {
//		fontstylepopup.Add(******);
//	}
}

int FontDialog::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	if (ch==LAX_Esc && !(win_style&FONTD_NO_DEL_WIN)) {
		app->destroywindow(this);
		return 0;
	}
	return RowFrame::CharInput(ch,buffer,len,state,kb);
}


} // namespace Laxkit

