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
#include <lax/treeselector.h>
#include <lax/numslider.h>
#include <lax/scroller.h>
#include <lax/palette.h>
#include <lax/iconselector.h>


namespace Laxkit {




//-------------------------------------- FontDialog -------------------------------------

enum LayersActions {
	LAYERS_None   = 0,
	LAYERS_Layer  =-1,
	LAYERS_Off    =-2,
	LAYERS_On     =-3,
	LAYERS_New    =-4,
	LAYERS_Trash  =-5,
	LAYERS_Select =-6,
	LAYERS_Arrange=-7,

	LAYERS_MAX    =-7
};

class FontLayersWindow : public anXWindow
{
  protected:
	int mode;
	int pad;
	int boxwidth;
	bool glyph_mismatch;

	int numlayers;
	int current_layer;
	int grabbed;
	int lasthover;
	flatpoint offset;

	ButtonDownInfo buttondown;

  public:
	FontLayersWindow(anXWindow *parnt, anXWindow *prev, unsigned long nowner,const char *nsend, int nmode, int nlayers);
	virtual ~FontLayersWindow();

	virtual const char *whattype() { return "FontLayersWindow"; }
	virtual int init();
	virtual int Event(const EventData *data,const char *mes);
	virtual void Refresh();
	
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int LBDown(int x,int y, unsigned int state,int count,const LaxMouse *d);
    virtual int LBUp(int x,int y, unsigned int state,const LaxMouse *d);
    virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int scan(int x,int y,unsigned int state, flatpoint *off=NULL);
    //virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
    //virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
    //virtual int Resize(int nw,int nh);
    //virtual int MoveResize(int nx,int ny,int nw,int nh);

	virtual int send(int action, int which, int which2=0);
	virtual int Current() { return current_layer; }

};

//-------------------------------------- FontDialog -------------------------------------

#define FONTD_NO_DEL_WIN (1<<16)

class FontDialog : public RowFrame
{
  protected:
	char *sampletext;
	double defaultsize;
	unsigned long dialog_style;
	PtrStack<FontDialogFont> *fonts; //stored in global fontmanager
	int currentfont; //index into fonts

	char *origfamily, *origstyle;

	bool more;
	bool initted;

	TreeSelector *fontlist;
	LineEdit *text;
	LineInput *fontfamily, *fontstyle, *fontfile;
	LineInput *search;
	NumSlider *fontsize;
	MenuInfo *mfonts;
	MenuInfo styles;
	FontLayersWindow *layers;
	IconSelector *tags;

	LaxFont *thefont, *fontlayer;
	Palette *palette;

	virtual void UpdateStyles();
	virtual void UpdateSample();
	virtual void UpdateColorBoxes();
	virtual void UpdateSearch();

  public:
	FontDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
				int xx,int yy,int ww,int hh,int brder,
				unsigned long nowner,const char *nsend,
				unsigned long ndstyle,
				const char *fam, const char *style, double size, const char *nsample,
				LaxFont *nfont, bool work_on_dup);
	virtual ~FontDialog();

	virtual const char *whattype() { return "FontDialog"; }
	virtual int init();
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int Event(const EventData *data,const char *mes);
	
	virtual LaxFont *CreateFromCurrent();
	virtual int FindFont();
	virtual int FindFont(const char *family, const char *style, const char *file);
	virtual int send();
	virtual int SampleText(const char *ntext);
};


} // namespace Laxkit

#endif

