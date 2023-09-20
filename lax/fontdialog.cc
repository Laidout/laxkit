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
//    Copyright (C) 2015 by Tom Lechner
//

#include <lax/fontdialog.h>
#include <lax/strmanip.h>
#include <lax/lineinput.h>
#include <lax/button.h>
#include <lax/language.h>
#include <lax/stackframe.h>
#include <lax/colorbox.h>
#include <lax/colorevents.h>
#include <lax/laxutils.h>
#include <lax/quickfileopen.h>
#include <lax/menubutton.h>


#define DBG
#include <iostream>
using namespace std;



namespace Laxkit {



//-------------------------------------- FontDialog -------------------------------------

/*! \class FontLayersWindow
 */

FontLayersWindow::FontLayersWindow(anXWindow *parnt, anXWindow *prev, unsigned long nowner,const char *nsend, int nmode, int nlayers)
  : anXWindow(parnt,"layers","layers",0, 0,0,0,0,0, prev,nowner,nsend)
{
	mode=nmode; //0 for don't use layers, 1 for do use layers

	if (nlayers<1) nlayers=1;
	numlayers=nlayers;
	if (numlayers>1) mode=1;
	current_layer=1;
	lasthover=0;
	grabbed=0; //which layer to float during a mouse drag, if any
	glyph_mismatch=false;

	Displayer *dp=GetDisplayer();
	pad=dp->textheight()/2;

	win_w=100;
	win_h=2*pad + dp->textheight();
}

FontLayersWindow::~FontLayersWindow()
{}

int FontLayersWindow::init()
{
	Displayer *dp=GetDisplayer();
	boxwidth=dp->textheight()*2;
	return 0;
}

int FontLayersWindow::Event(const EventData *data,const char *mes)
{
	if (data->type==LAX_onMouseOut) {
		lasthover=0;
		needtodraw=1;
	}
	return anXWindow::Event(data,mes);
}

	
int FontLayersWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	return anXWindow::CharInput(ch,buffer,len,state,kb);
}

int FontLayersWindow::LBDown(int x,int y, unsigned int state,int count,const LaxMouse *d)
{
	int over=scan(x,y,state, &offset);
	buttondown.down(d->id, LEFTBUTTON, x,y, over);
	return 0;
}

int FontLayersWindow::send(int action, int which, int which2)
{
	SimpleMessage *data=new SimpleMessage(NULL, action, which, which2,0, win_sendthis);
    app->SendMessage(data, win_owner, win_sendthis, object_id);
	return 0;
}

int FontLayersWindow::LBUp(int x,int y, unsigned int state,const LaxMouse *d)
{
	int over=scan(x,y,state);
	int down=0;
	buttondown.up(d->id, LEFTBUTTON, &down);
	int grab=grabbed;
	grabbed=0;

	if (over==LAYERS_On && over==down) {
		mode=1;
		over=0;
		send(LAYERS_On, 0);
		needtodraw=1;
		return 0;

	} else if (over==LAYERS_Off && over==down) {
		mode=0;
		over=0;
		send(LAYERS_Off, 0);
		current_layer=0;
		numlayers=1;
		needtodraw=1;
		return 0;

	} else if (over==LAYERS_Trash && over==down) {
		if (numlayers>1) {
			numlayers--;
			needtodraw=1;
			send(LAYERS_Trash, current_layer);
			if (current_layer>=numlayers) current_layer--;
		}
		return 0;

	} else if (over==LAYERS_New && over==down) {
		numlayers++;
		send(LAYERS_New, current_layer);
		current_layer++;
		needtodraw=1;
		return 0;

	} else if (over>0 && over==down) {
		current_layer=over;
		send(LAYERS_Select, current_layer);
		needtodraw=1;
		return 0;

	} else if (grab) {
		 //done dragging a layer
		DBG cerr <<" place grabbed layer: "<<grabbed<<endl;

		if (over<=0) {
			 //dragged off somewhere
			send(LAYERS_Trash, grab);
			numlayers--;
			needtodraw=1;
		} else {
			send(LAYERS_Arrange, grab, over);
			current_layer=over;
		}

		needtodraw=1;
		return 0;
	}

	// *** 
	
	return 0;
}

int FontLayersWindow::scan(int x,int y,unsigned int state, flatpoint *off)
{
	if (mode==0) {
		Displayer *dp=GetDisplayer();
		if (x>win_w-2*pad-dp->textextent(_("Layers..."),-1, NULL,NULL)) {
			return LAYERS_On;
		}
		return 0;
	}

	// [glyph mismatch!] [1][2][3][4] [+] [trash]  [No layers]

	if (y<0 || y>=win_h) return LAYERS_None;

	int xx=win_w-boxwidth-pad;
	if (x>xx) return LAYERS_Off;
	xx-=1.5*boxwidth;
	if (x>xx && x<xx+boxwidth) return LAYERS_Trash;
	xx-=boxwidth;
	if (x>xx && x<xx+boxwidth) return LAYERS_New;

	for (int c=numlayers; c>0; c--) {
		xx-=boxwidth;
		if (x>xx && x<xx+boxwidth) {
			if (off) off->set(x-xx, y-(win_h/2-boxwidth/2));
			return c;
		}
	}

	return LAYERS_None;
}

int FontLayersWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.any()) {
		int over=scan(x,y,state);
		if (over!=lasthover) {
			DBG cerr <<"FontLayersWindow::MouseMove: "<<over<<endl;
			lasthover=over;
			needtodraw=1;
		}
		return 0; 
	}


	int over=scan(x,y,state);
	buttondown.move(d->id, x,y);

	if (over>0 && !grabbed && numlayers>1) {
		grabbed=over;
		current_layer=grabbed;
		send(LAYERS_Select, grabbed);
	}

	if (grabbed) {
		lasthover=over;
		needtodraw=1;

	} else if (over!=lasthover) {
		lasthover=over;
		needtodraw=1;
	}
	return 0;
}

void FontLayersWindow::Refresh()
{
	if (!needtodraw) return;
	needtodraw=0;

	Displayer *dp=MakeCurrent();
	dp->ClearWindow();
    dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->NewFG(win_themestyle->fg);

	if (mode==0) {
		DBG int over=0;
		//if (buttondown.isdown(0,LEFTBUTTON)){ 
		//	int device=buttondown.whichdown(0, LEFTBUTTON);
		//	buttondown.getextrainfo(device, LEFTBUTTON, &over);
		//}
		if (lasthover==LAYERS_On) {
			 //draw highlighted

			DBG cerr <<" over"<<over<<endl;
			dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.1));
			int w=dp->textextent(_("Layers..."),-1,NULL,NULL) + 2*pad;
			dp->drawrectangle(win_w-w,0, w,win_h, 1);
		}
		dp->NewFG(win_themestyle->fg);
		dp->textout(win_w-pad,win_h/2, _("Layers..."),-1, LAX_RIGHT|LAX_VCENTER);
		return;
	}

	 //else mode 1

	// [glyph mismatch!] [1][2][3][4] [+] [trash]  [No layers]

	 //draw "no layers" X icon
	int xx=win_w-pad-boxwidth;
	if (!grabbed && lasthover==LAYERS_Off) { dp->NewFG(1.0,0.0,0.0); dp->LineWidth(2); }
	dp->drawline(xx,win_h/2-boxwidth/2, xx+boxwidth,win_h/2+boxwidth/2);
	dp->drawline(xx,win_h/2+boxwidth/2, xx+boxwidth,win_h/2-boxwidth/2);
	dp->NewFG(win_themestyle->fg);
	dp->LineWidth(1);

	 //draw trash
	xx-=1.5*boxwidth;
	if ((!grabbed && lasthover==LAYERS_Trash) || (grabbed && lasthover<=0)) { dp->NewFG(1.0,0.0,0.0); dp->LineWidth(2); }
	dp->drawline(xx+boxwidth*.2,win_h/2-boxwidth*.3, xx+boxwidth*.8,win_h/2+boxwidth*.3);
	dp->drawline(xx+boxwidth*.2,win_h/2+boxwidth*.3, xx+boxwidth*.8,win_h/2-boxwidth*.3);
	dp->NewFG(win_themestyle->fg);
	dp->LineWidth(1);
	
	 //draw new layer
	xx-=boxwidth;
	if (!grabbed && lasthover==LAYERS_New) {
		dp->NewBG(coloravg(win_themestyle->bg,win_themestyle->fg,.1));
		dp->drawrectangle(xx+boxwidth*.1,win_h/2-boxwidth*.4, boxwidth*.8,boxwidth*.8, 2);
	} else {
		dp->drawrectangle(xx+boxwidth*.1,win_h/2-boxwidth*.4, boxwidth*.8,boxwidth*.8, 0);
	}
	dp->drawline(xx+boxwidth*.2,win_h/2, xx+boxwidth*.8,win_h/2);
	dp->drawline(xx+boxwidth/2,win_h/2-boxwidth*.3, xx+boxwidth/2,win_h/2+boxwidth*.3);

	 //draw layers
	char str[12];
	int i;
	for (int c=numlayers; c>0; c--) {
		xx-=boxwidth;
		i=c;

		if (grabbed==c && lasthover==grabbed) continue;
		if (grabbed && c==lasthover) continue; //empty space where the hovered one goes
		if (grabbed) {
			if (c<lasthover && c>=grabbed) i++;
			else if (c<=grabbed && c>lasthover) i--;
		}
		if (i<=0) break;

		if (!grabbed && c==current_layer) dp->LineWidth(4);
		if (!grabbed && lasthover==c) {
			dp->NewBG(coloravg(win_themestyle->bg,win_themestyle->fg,.1));
			dp->drawrectangle(xx,win_h/2-boxwidth/2, boxwidth,boxwidth, 2);
		} else {
			dp->drawrectangle(xx,win_h/2-boxwidth/2, boxwidth,boxwidth, 0);
		}

		if (!grabbed && c==current_layer) dp->LineWidth(1);
		sprintf(str,"%d",i);
		dp->textout(xx+boxwidth/2, win_h/2, str,-1, LAX_CENTER);
	}


	 //draw mismatch message
	if (glyph_mismatch) dp->textout(xx,win_h/2, _("Glyph mismatch!"), LAX_RIGHT|LAX_VCENTER);


	 //draw hovered layer
	if (grabbed && buttondown.isdown(0,LEFTBUTTON)) { 
		int device=buttondown.whichdown(0, LEFTBUTTON);

		int x,y;
		buttondown.getcurrent(device, LEFTBUTTON, &x, &y);

		dp->NewBG(win_themestyle->bg);
		dp->NewFG(win_themestyle->fg);
		dp->drawrectangle(x-offset.x,  y-offset.y,  boxwidth,boxwidth, 2);

		sprintf(str,"%d",grabbed);
		dp->textout(x-offset.x+boxwidth/2,  y-offset.y+boxwidth/2, str,-1, LAX_CENTER);
	}

}


//-------------------------------------- FontDialog -------------------------------------
/*! \class FontDialog
 * \brief Dialog to allow selecting fonts.
 *
 * This will sent a StrsEventData with fields: family, style, file, size.
 *
 * fam, style, and size are default values to open the dialog to. If NULL or 0 are supplied,
 * then the dialog will select defaults.
 *
 * nfont's count will be incremented if !work_on_dup. If work_on_dup, nfont is left alone,
 * and the dialog works on and sends back a duplicate of nfont.
 */
FontDialog::FontDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		unsigned long nowner,const char *nsend,
		unsigned long ndstyle,
		const char *fam, const char *style, double size, const char *nsample,
		LaxFont *nfont, bool work_on_dup)
	: RowFrame(parnt,nname,ntitle,(nstyle&0xffff)|ROWFRAME_ROWS|ROWFRAME_CENTER,
			   xx,yy,ww,hh,brder,
			   NULL,nowner,nsend,
			   5)
{
	palette=NULL;

	thefont=nfont;
	if (nfont) {
		if (work_on_dup) thefont=nfont->duplicate();
		else thefont->inc_count();

		if (!fam)   fam  =thefont->Family();
		if (!style) style=thefont->Style();
	} else {
		if (!fam) fam="sans";
		thefont=app->fontmanager->MakeFont(fam, style, size, 0);
	}

	fontlayer=thefont;
	palette=dynamic_cast<Palette*>(thefont->GetColor());
	if (palette) palette->inc_count();


	origfamily=newstr(fam);
	origstyle =newstr(style);

	if (!thefont && origfamily) {
		FindFont();
	}

	dialog_style=ndstyle;
	defaultsize=size;
	if (defaultsize<=0) defaultsize=12;
	currentfont=-1;

	if (!nsample) nsample=_("The quick brown fox etc");
	sampletext=newstr(nsample);
	mfonts=NULL;

	more=false;
	initted=false;

	tags=NULL;
}

FontDialog::~FontDialog()
{
	//fonts->flush(); <- lives in fontmanager
	if (mfonts) mfonts->dec_count();
	if (thefont) thefont->dec_count();
	if (palette) palette->dec_count();

	delete[] sampletext;
	delete[] origfamily;
	delete[] origstyle;
}

int FontDialog::init()
{

	FontManager *fmanager=GetDefaultFontManager();

	fonts=fmanager->GetFontList();


	 //-------------build windows
	//int textheight=app->defaultfont->max_bounds.ascent+app->defaultfont->max_bounds.descent;
	//int linpheight=textheight+6;

	anXWindow *last = NULL;
	double textheight = win_themestyle->normal->textheight();

	 //------font family
	 // *** type in box progressively limits what's displayed in list 
	 // *** should have selectors to group favorites or whatever
	last=fontfamily=new LineInput(this,"fontfamily","fontfamily",0, 0,0,0,0, 0, last,object_id,"fontfamily", 
							_("Family"),origfamily,0, 0,0,2,2,2,2);
	fontfamily->tooltip(_("Family name of the font"));
	AddWin(fontfamily,1, 400,200,1000,50,0, fontfamily->win_h,0,0,50,0, -1);


	 //------font style
	last=fontstyle=new LineInput(this,"fontstyle","fontstyle",0, 0,0,0,0, 0, last,object_id,"fontstyle", 
							_("Style"),origstyle,0, 0,0,2,2,2,2);
	fontstyle->tooltip(_("Style of the font"));
	AddWin(fontstyle,1, 200,100,1000,50,0, fontstyle->win_h,0,0,50,0, -1);

	last=new MenuButton(this,"styles","styles",MENUBUTTON_DOWNARROW, 0,0,0,0,0, last,object_id,"style",0, &styles,0, NULL,NULL,NULL,textheight/3);
	AddWin(last,1, last->win_w,0,0,50,0, last->win_h,0,0,50,0, -1);


	 //-----font size
	//last=fontsize=new LineInput(this,"size","size",0, //LINP_FLOAT,
	//						0,0,0,0, 0, last,object_id,"fontsize", 
	//						_("Size"),NULL,0, 0,0,2,2,2,2);
	last=fontsize=new NumSlider(this,"size","size",ItemSlider::SENDALL|ItemSlider::EDITABLE|NumSlider::DOUBLES,
							0,0,0,0, 1, last,object_id,"fontsize", 
							_("Size"), 0,1000000, defaultsize, .5);
	fontsize->tooltip(_("Size of the font"));
	AddWin(fontsize,1, 150,100,1000,50,0, fontsize->win_h,0,0,50,0, -1);

	AddNull();


	 //------font file
	last=fontfile=new LineInput(this,"fontfile",_("Font file"),LINP_FILE, 0,0,0,0, 0, last,object_id,"fontfile", 
							_("File"), thefont->FontFile(), 0, 0,0,2,2,2,2);
	fontfile->tooltip(_("File of the font"));
	AddWin(fontfile,1, 200,100,2000,50,0, fontstyle->win_h,0,0,50,0, -1);

	last=new QuickFileOpen(this,"new file","new file",ANXWIN_REMEMBER, 0,0,0,0, 1,
	                      last,object_id,"newfile",
						  FILES_OPEN_ONE,
						  fontfile);
	AddWin(last,1, last->win_w,0,0,50,0, last->win_h,0,0,50,0, -1);

	AddNull();


	AddVSpacer(textheight/2,0,0,0);
	AddNull();


	 //------search
	last=search=new LineInput(this,"search","search",0, 0,0,0,0, 0, last,object_id,"search", 
							_("Search"),NULL,0, 0,0,2,2,2,2);
	search->GetLineEdit()->SetWinStyle(LINEEDIT_SEND_ANY_CHANGE, 1);
	search->GetLineEdit()->SetWinStyle(LINEEDIT_CLEAR_X, 1);
	search->tooltip(_("Search among fonts"));
	AddWin(search,1, 20,10,5000,50,0, search->win_h,0,0,50,0, -1);

	AddHSpacer(textheight*2, 0,250,0);

	Button *tbut;
//	last=tbut=new Button(this,"more","more",IBUT_FLAT, 0,0,0,0, 0, 
//			last,object_id,"more", 0, _("More.."),NULL,NULL,3,3);
//	AddWin(tbut,1, tbut->win_w,0,tbut->win_w*2,50,0, tbut->win_h,0,0,50,0, -1);

	AddNull();

	//----- tags
	if (app->fontmanager->tags.n) {
		last = tags = new IconSelector(this, "tags","tags", 0, 0,0,0,0,0, last,object_id,"tags",textheight/2,textheight/2);

		for (int c=0; c<app->fontmanager->tags.n; c++) {
			tags->AddBox(app->fontmanager->tags.e[c]->tag, (LaxImage*)NULL, app->fontmanager->tags.e[c]->id);
		}

		AddWin(tags,1, tags->win_w,0,5000,50,0, textheight*1.5,0,2*textheight,50,0, -1);
		AddNull();
	}

	 //------font list
	int orig=-1;
	if (!mfonts) {
		mfonts=new MenuInfo("Fonts");
		//char str[1024];

		for (int c=0; c<fonts->n; c++) {
			if (orig<0 && fonts->e[c]->file && thefont->FontFile() && !strcmp(fonts->e[c]->file, thefont->FontFile()))
				orig=c;

			mfonts->AddItem(fonts->e[c]->name, c);
			//------
			//sprintf(str,"%s, %s",fonts->e[c]->family,fonts->e[c]->style);
			//mfonts->AddItem(fonts->e[c]->name ? fonts->e[c]->name : str, c);
			//------
			//mfonts->AddItem((fonts->e[c]->psname ? fonts->e[c]->psname : _("No ps name!")),c);
		}

		mfonts->SetCompareFunc(SORT_ABC|SORT_IGNORE_CASE);
		mfonts->Sort(0);
	}

	last=fontlist=new TreeSelector(this,"fonts","fonts", SW_RIGHT,
									0,0,0,0,1,
									last,object_id,"font",
									TREESEL_SEND_ON_UP
									 |TREESEL_CURSSENDS
									 //|TREESEL_TEXTCOLORS
									 //|TREESEL_SUB_ON_LEFT
									 |TREESEL_FOLLOW_MOUSE
									 |TREESEL_CURSSELECTS
									 |TREESEL_LEFT
									 |TREESEL_ONE_ONLY,
									mfonts);
	fontlist->InstallColors(THEME_Edit);
	//fontlist->tooltip(_("Select one of these"));
	AddWin(fontlist,1, 200,100,1000,50,0, 30,0,2000,50,0, -1);

	//***supposed to be handled automatically via ScrolledWindow:
	//last=scroller=new Scroller(this, "scr","scr", SC_YSCROLL|SC_ABOTTOM, 0,0,0,0,0, last,object_id,"scroll", panner, ****sizes..);

	AddNull();



	 //-----sample text
	last=text=new LineEdit(this,"sample","sample",0,
								0,0,0,0,0,
								last,object_id,"sample",
								sampletext,TEXT_CENTER);
	WindowStyle *ncolors = app->theme->GetStyle(THEME_Edit)->duplicate();
	text->InstallColors(ncolors);
	ncolors->dec_count();
//	if (orig>=0) {
//		LaxFont *newfont=app->fontmanager->MakeFont(fonts->e[orig]->family, fonts->e[orig]->style, defaultsize, 0);
//		if (newfont) {
//			text->UseThisFont(newfont);
//			newfont->dec_count();
//		}
//	}
	text->UseThisFont(thefont);
	AddWin(text,1, 200,100,1000,50,0, defaultsize*1.75,0,0,50,0, -1);
	AddNull();


	 // fg/bg boxes
	int r,g,b,a=255;
	ColorBox *colorbox;

	AddHSpacer(0, 0, 5000,0);

	if (!palette) palette=new Palette;

	 //bg
	colorrgb(text->win_themestyle->bg.Pixel(), &r,&g,&b);
	colorbox=new ColorBox(this,"bg","bg",COLORBOX_SEND_ALL, 0,0,textheight*2,textheight*2,1, NULL,object_id,"bg", 
							   LAX_COLOR_RGB,1./255, r/255.,g/255.,b/255.,a/255.,0);
	colorbox->tooltip(_("Sample background"));
	AddWin(colorbox, 1, textheight*2,0,0,50,0, textheight*2,0,0,50,0, -1);

	 //fg
	if (thefont) {
		char str[20];
		for (int c=0; c<thefont->Layers(); c++) {
			sprintf(str, "fg%d", c+1);

			 // *** replace all Palette stuff with GradientStrip/Color setup when done implementing it!!
			if (palette && c<palette->colors.n) {
				r=255*palette->colors.e[c]->channels[0]/(double)palette->colors.e[c]->maxcolor;
				g=255*palette->colors.e[c]->channels[1]/(double)palette->colors.e[c]->maxcolor;
				b=255*palette->colors.e[c]->channels[2]/(double)palette->colors.e[c]->maxcolor;
				a=255*palette->colors.e[c]->channels[3]/(double)palette->colors.e[c]->maxcolor;
			} else {
				a=255;
				colorrgb(text->win_themestyle->fg.Pixel(), &r,&g,&b);
			}

			colorbox=new ColorBox(this, str,str, COLORBOX_SEND_ALL, 0,0,textheight*2,textheight*2,1, NULL,object_id,str,
									   LAX_COLOR_RGB,1./255, r/255.,g/255.,b/255.,a/255.,0);
			colorbox->tooltip(_("Sample foreground"));
			AddWin(colorbox, 1, textheight*2,0,0,50,0, textheight*2,0,0,50,0, -1); 
		}

	} else {
		colorrgb(text->win_themestyle->fg.Pixel(), &r,&g,&b);
		colorbox=new ColorBox(this,"fg1","fg1",COLORBOX_SEND_ALL, 0,0,textheight*2,textheight*2,1, NULL,object_id,"fg1", 
								   LAX_COLOR_RGB,1./255, r/255.,g/255.,b/255.,a/255.,0);
		colorbox->tooltip(_("Sample foreground"));
		AddWin(colorbox, 1, textheight*2,0,0,50,0, textheight*2,0,0,50,0, -1); 
	}

	//AddHSpacer(0, 0, 5000,0);

	 //------multicolor layer selector
	last=layers=new FontLayersWindow(this, last, object_id, "layers", thefont->Layers()>1 ? 1 : 0, thefont->Layers());
	//last=tbut=new Button(this,"layers","layers",IBUT_FLAT, 0,0,0,0, 0, 
	//		last,object_id,"layers", 0, _("Layers.."),NULL,NULL,3,3);
	AddWin(last,1, textheight*5,0,5000,50,0, last->win_h,0,0,50,0, -1);


	AddNull();


//------old vertical stack of colors:
//	StackFrame *stack=new StackFrame(this, "vstack",NULL, STACKF_VERTICAL|STACKF_NOT_SIZEABLE, 0,0,0,0,0, NULL,0,NULL,0);
//	int r,g,b;
//	colorrgb(text->win_themestyle->fg, &r,&g,&b);
//	ColorBox *colorbox;
//	colorbox=new ColorBox(stack,"fg","fg",COLORBOX_SEND_ALL, 0,0,textheight*2,textheight*2,1, NULL,object_id,"fg", 
//							   LAX_COLOR_RGB,1./255, r/255.,g/255.,b/255.,1.0,0);
//	colorbox->tooltip(_("Sample foreground"));
//	stack->AddWin(colorbox, 1, textheight*2,0,0,50,0, textheight*2,0,100,50,0);
//
//	colorrgb(text->win_themestyle->bg, &r,&g,&b);
//	colorbox=new ColorBox(stack,"bg","bg",COLORBOX_SEND_ALL, 0,0,textheight*2,textheight*2,1, NULL,object_id,"bg", 
//							   LAX_COLOR_RGB,1./255, r/255.,g/255.,b/255.,1.0,0);
//	colorbox->tooltip(_("Sample background"));
//	stack->AddWin(colorbox, 1, textheight*2,0,0,50,0, textheight*2,0,100,50,0);
//	stack->WrapToExtent();
//	AddWin(stack,1, stack->win_w,0,0,50,0, stack->win_h,0,0,50,0, -1);
//------end old vertical stack of colors:

	AddNull();


	AddVSpacer(textheight/2,0,0,0);
	AddNull();


	 //--------final ok and cancel
	last=tbut=new Button(this,"ok","ok",BUTTON_OK, 0,0,0,0, 1, 
			last,object_id,"ok", 0, NULL,NULL,NULL,3,3);
	AddWin(tbut,1, 200,100,1000,50,0, tbut->win_h,0,0,50,0, -1);

	last=tbut=new Button(this,"cancel","cancel",BUTTON_CANCEL, 0,0,0,0, 1, 
			last,object_id,"cancel", 0, NULL,NULL,NULL,3,3);
	AddWin(tbut,1, 200,100,1000,50,0, tbut->win_h,0,0,50,0, -1);



	last->CloseControlLoop();
	Sync(1);

	if (orig<0) orig=FindFont(origfamily, origstyle, thefont->FontFile());
	if (orig>=0) fontlist->SelectId(orig);

	initted=true;

	return 0;
}

/*! Returns index in internal (fontmanager->GetFontList()) all font list of a font caselessly matching family
 * and style, or exactly matches file (with case).
 *
 * \todo option to search vaguely?
 */
int FontDialog::FindFont(const char *family, const char *style, const char *file)
{
	for (int c=0; c<fonts->n; c++) {
		if (file) {
			if (!strcmp(file, fonts->e[c]->file)) return c;
			continue;
		}

		if (family && strcasecmp(family, fonts->e[c]->family)) continue;
		if (style  && !strcasecmp(style, fonts->e[c]->style)) return c;
	}

	return -1;
}

int FontDialog::SampleText(const char *ntext)
{
	if (isblank(ntext)) return 1;
	makestr(sampletext, ntext);
	if (initted) UpdateSample();
	return 0;
}

void FontDialog::UpdateSample()
{
	double size=fontsize->Value();
	if (size<=0) size=1e-4;

	if (currentfont<0) FindFont();
	fontlayer->SetFromFile(fonts->e[currentfont]->file, fonts->e[currentfont]->family, fonts->e[currentfont]->style, size);
	thefont->Resize(size);

	double samplesize=size;
	LaxFont *samplefont = thefont;
	double th = win_themestyle->normal->textheight();
	if      (size < th / 2) samplesize = th / 2;
	else if (size > th * 5) samplesize = th * 5;

	if (samplesize != size) {
		samplefont = samplefont->duplicate();
		samplefont->Resize(samplesize);
	}

	text->UseThisFont(samplefont);
	if (samplefont != thefont) samplefont->dec_count();

	SquishyBox *box=findBox(text);
	if (samplesize<15) samplesize=15;
	samplesize=1.75*samplesize;
	if (box->ph() != samplesize) {
		box->ph(samplesize);
		Sync(0);
		//fontlist->makeinwindow();
	}
}

/*! Apply the search term if any, and any tag filters.
 */
void FontDialog::UpdateSearch()
{
	fontlist->ClearSearch();
	fontlist->UpdateSearch(search->GetCText(), 0);

	if (!app->fontmanager->tags.n) return;
	if (!tags) return;

	MenuInfo *menu=fontlist->Menu();
	MenuItem *item;
	SelBox *box;
	FontDialogFont *font;
	int tag;

	NumStack<int> active_tags;
	for (int c2=0; c2<tags->NumBoxes(); c2++) {
		box=dynamic_cast<SelBox*>(tags->GetBox(c2));
		if (!box) continue;
		if (box->state&LAX_ON) {
			active_tags.push(box->id);

			DBG cerr<<"active tag: "<<app->fontmanager->GetTagName(box->id)<<endl;
		}
	}

	if (active_tags.n) {
		for (int c=0; c<menu->n(); c++) { //each line in window font list
			item=menu->e(c);
			if (!item) continue;

			font = fonts->e[item->id]; //index into fontmanager->fonts[]

			int hit=0;
			for (int c2=0; c2<active_tags.n; c2++) {
				tag=active_tags.e[c2];
				if (font->HasTag(tag)) {
					item->state|=MENU_SEARCH_HIT;
					hit++;
				}
			}
			if (!hit) item->state|=MENU_SEARCH_HIDDEN;
		}
	}

	fontlist->Sync();
}

int FontDialog::Event(const EventData *data,const char *mes)
{
	DBG cerr <<"-----font dialog got: "<<mes<<endl;


	if (!strcmp(mes,"fontfamily")) { 
		// *** search for matching font
	
	} else if (!strcmp(mes,"fontstyle")) {
		// *** search for matching font
	
	} else if (!strcmp(mes,"style")) {
		//from the style drop down
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		int i=s->info2;
		if (i<0 || i>=fonts->n) return 0;
		fontfamily->SetText(fonts->e[i]->family);
		fontstyle ->SetText(fonts->e[i]->style);
		fontfile  ->SetText(fonts->e[i]->file);
		//fontlayer->SetFromFile(fonts->e[i]->file, fonts->e[i]->family, fonts->e[i]->style, defaultsize);
		currentfont=i;
		fontlist->ClearSearch();
		fontlist->SelectId(i);
		UpdateSample();
		return 0;
	
	} else if (!strcmp(mes,"fontsize")) {
		UpdateSample();
		return 0;

	} else if (!strcmp(mes,"font")) { 
		 //clicked in list
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		//int i=s->info1; //index
		int i=s->info2; //id
		if (i<0 || i>=fonts->n) return 0;
		fontfamily->SetText(fonts->e[i]->family);
		fontstyle ->SetText(fonts->e[i]->style);
		fontfile  ->SetText(fonts->e[i]->file);
		//fontlayer->SetFromFile(fonts->e[i]->file, fonts->e[i]->family, fonts->e[i]->style, defaultsize);
		currentfont=i;
		UpdateSample();
		UpdateStyles();
		return 0;

	} else if (!strcmp(mes,"search")) {
		fontlist->UpdateSearch(search->GetCText(), 0);
		return 0;

	} else if (!strcmp(mes,"tags")) {
		DBG cerr <<"===========================tags======================="<<endl;

		//const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		//int i=s->info1; //index
		//int tagid = s->info2; //id

		UpdateSearch();

		return 0;

	} else if (!strcmp(mes,"newfile")) {
		cerr <<" *** NEED TO IMPLEMENT FontDialog:: get new file"<<endl;

	} else if (strstr(mes,"fg")==mes || !strcmp(mes,"bg")) { 
		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(data);
        if (!ce) return 0;

		unsigned long color=rgbcolorf(ce->channels[0]/(double)ce->max, ce->channels[1]/(double)ce->max, ce->channels[2]/(double)ce->max);
		if (!strcmp(mes,"bg")) text->win_themestyle->bg=color;
		else {
			 //fg
			int which=strtol(mes+2,NULL,10);
			if (which>0 && which<=palette->colors.n) {
				DBG cerr <<" change color for font layer "<<which<<endl;

				palette->colors.e[which-1]->channels[0]=ce->channels[0];
				palette->colors.e[which-1]->channels[1]=ce->channels[1];
				palette->colors.e[which-1]->channels[2]=ce->channels[2];
				palette->colors.e[which-1]->channels[3]=ce->channels[3];
				palette->colors.e[which-1]->maxcolor=ce->max;
			}
			text->win_themestyle->fg=color;
		}
		text->Needtodraw(1);
		return 0;

	} else if (!strcmp(mes,"cancel")) {
		app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"ok")) {
		send();
		app->destroywindow(this);
		return 0;

	} else if (!strcmp(mes,"more")) {
		Button *button = dynamic_cast<Button *>(findChildWindowByName("more"));
		more=!more;
		if (more) button->Label(_("Less"));
		else button->Label(_("More.."));

		cerr <<" *** need to toggle more/less options!"<<endl;

		return 0;

	} else if (!strcmp(mes,"layers")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(data);
		int action=s->info1;
		int which =s->info2-1;
		int which2=s->info3-1;

		DBG cerr <<" layers: "<<action<<"  which: "<<which<<"  which2: "<<which2<<endl; 


		if (action==LAYERS_Off) {
			thefont->RemoveAllLayers();
			fontlayer=thefont;
			thefont->SetColor(NULL);
			UpdateColorBoxes();
			text->Needtodraw(1);

		} else if (action==LAYERS_On) {
			 //assumes there was only one layer before, need only ensure palette is used.
			LaxFont *f=thefont;
			while (f) {
				f->SetColor(palette);
				f=f->nextlayer;
			}

		} else if (action==LAYERS_New) {
			//put new one at which
			LaxFont *newfont=CreateFromCurrent();
			newfont->SetColor(palette);
			thefont = thefont->AddLayer(which+1, newfont);
			fontlayer=newfont;
			//palette->colors.push(new PaletteEntry(palette->colors.e[which]), 1);
			UpdateColorBoxes();
			text->UseThisFont(thefont);

		} else if (action==LAYERS_Trash) {
			//delete layer which
			if (which>=0 && which<thefont->Layers()) {
				thefont = thefont->RemoveLayer(which, NULL);
				if (which==thefont->Layers()) fontlayer=thefont->Layer(which-1);
				else fontlayer=thefont->Layer(which);
				UpdateColorBoxes();
				text->UseThisFont(thefont);
				palette->colors.remove(which);
			}

		} else if (action==LAYERS_Select) {
			if (which>=0 && which<thefont->Layers()) {
				fontlayer=thefont->Layer(which);

				int i=FindFont(fontlayer->Family(), fontlayer->Style(), fontlayer->FontFile());
				if (i>=0) {
					fontlist->SelectId(i);

					fontfamily->SetText(fonts->e[i]->family);
					fontstyle ->SetText(fonts->e[i]->style);
					fontfile  ->SetText(fonts->e[i]->file);
				}
				currentfont=i;
				UpdateStyles();
			}
		} else if (action==LAYERS_Arrange) {
			//take which and put it at place which2
			thefont = thefont->MoveLayer(which, which2);
			palette->colors.slide(which,which2);
			UpdateColorBoxes();
			text->UseThisFont(thefont);
		} 

		needtodraw=1;
		return 0;
	}

	return anXWindow::Event(data,mes);
}

void FontDialog::UpdateColorBoxes()
{
	ColorBox *box;
	int i=findWindowIndex("fg1");
	int numboxes=1;
	int numlayers = thefont->Layers();
	int needtosync=0;

	while (dynamic_cast<ColorBox *>(findWindowFromIndex(i+numboxes))) {
		numboxes++;
	}

	 //remove excess color boxes
	while (numboxes > numlayers) {
		Pop(i+numboxes-1);
		numboxes--;
		needtosync=1;
	}

	 //add new color boxes and palette entries as necessary
	double textheight = win_themestyle->normal->textheight();
	double r,g,b,a;
	if (numboxes>0 && numboxes-1<palette->colors.n) {
		r=palette->colors.e[numboxes-1]->channels[0]/(double)palette->colors.e[numboxes-1]->maxcolor;
		g=palette->colors.e[numboxes-1]->channels[1]/(double)palette->colors.e[numboxes-1]->maxcolor;
		b=palette->colors.e[numboxes-1]->channels[2]/(double)palette->colors.e[numboxes-1]->maxcolor;
		a=palette->colors.e[numboxes-1]->channels[3]/(double)palette->colors.e[numboxes-1]->maxcolor;
	} else {
		int rr,gg,bb;
		colorrgb(text->win_themestyle->fg.Pixel(), &rr,&gg,&bb);
		r=rr/255.;
		g=gg/255.;
		b=bb/255.;
		a=1.0;
	}

	char str[20];
	while (numboxes < numlayers) {
		sprintf(str, "fg%d", numboxes+1);

		box=new ColorBox(this, str,str, COLORBOX_SEND_ALL, 0,0,textheight*2,textheight*2,1, NULL,object_id,str,
								   LAX_COLOR_RGB,  1./255, r,g,b,a, 0);
		box->tooltip(_("Sample foreground"));
		AddWin(box, 1, textheight*2,0,0,50,0, textheight*2,0,0,50,0, i+numboxes); 

		numboxes++;
		needtosync=1;
	}

	for (int c=palette->colors.n+1; palette->colors.n < numboxes; c++) {
		sprintf(str, "fg%d", c);
		palette->AddRGBA(str, int(r*255),int(g*255),int(b*255),int(a*255), 255);
	}

	for (int c=i; c<i+numboxes; c++) {
		box=dynamic_cast<ColorBox *>(findWindowFromIndex(c));
		box->SetRGB(palette->colors.e[c-i]->channels[0]/(double)palette->colors.e[c-i]->maxcolor,
					palette->colors.e[c-i]->channels[1]/(double)palette->colors.e[c-i]->maxcolor,
					palette->colors.e[c-i]->channels[2]/(double)palette->colors.e[c-i]->maxcolor,
					palette->colors.e[c-i]->channels[3]/(double)palette->colors.e[c-i]->maxcolor
				);

	}

	SquishyBox *sbox=findBox(layers);
	sbox->pw(textheight*(5+2*(thefont->Layers()-1)));

	//DBG palette->dump_out(stderr, 0, 0, NULL);

	if (needtosync) Sync(1);
}

LaxFont *FontDialog::CreateFromCurrent()
{
	const char *file =fontfile  ->GetCText();
	const char *fam  =fontfamily->GetCText();
	const char *style=fontstyle ->GetCText();

	LaxFont *newfont=NULL;

	newfont = app->fontmanager->MakeFontFromFile(file, fam, style, defaultsize, 0);
	if (!newfont) newfont=app->fontmanager->MakeFont(fam, style, defaultsize, 0);

	return newfont;
}

/*! When currentfont<0, try to find a font that matches current text in family and style.
 *
 * If currentfont>=0, then do nothing and return.
 *
 * currentfont will always be >=0 after calling this.
 */
int FontDialog::FindFont()
{
	if (currentfont>=0) return 0;

	 //set up fontconfig pattern to match
	FcPattern *pattern=FcPatternCreate();

    FcValue value;

    value.type=FcTypeString; value.u.s = (FcChar8*)const_cast<char*>(fontfile->GetCText());
    FcPatternAdd(pattern, FC_FILE, value, FcTrue);

    value.type=FcTypeString; value.u.s = (FcChar8*)const_cast<char*>(fontfamily->GetCText());
    FcPatternAdd(pattern, FC_FAMILY, value, FcTrue);

    value.type=FcTypeString; value.u.s = (FcChar8*)const_cast<char*>(fontstyle->GetCText());
    FcPatternAdd(pattern, FC_STYLE, value, FcTrue);


	 //find best match
	FontManager *fmanager=GetDefaultFontManager();
	FcConfig *fcconfig = fmanager->GetConfig(); //stored in and destroyed by the font manager

	FcResult result;
	FcPattern *found = FcFontMatch(fcconfig, pattern, &result);
	if (result!=FcResultMatch) {
		currentfont=0;
		return 1;
	}

	result=FcPatternGet(found, FC_FAMILY,0,&value);
	if (result==FcResultMatch) {
		fontfamily->SetText((const char *)value.u.s);
	}

	result=FcPatternGet(found, FC_STYLE,0,&value);
	if (result==FcResultMatch) {
		fontstyle->SetText((const char *)value.u.s);
	}

	for (int c=0; c<fonts->n; c++) {
		if (fonts->e[c]->Match(fontfamily->GetCText(),fontstyle->GetCText())) {
			currentfont=c;
			break;
		}
	}


	 //clean up
    FcPatternDestroy(pattern);

	if (currentfont<0) currentfont=0;
	//else fonts->SelectId(currentfont);
	return 0;
}

//! Sends a StrsEvent Data.
/*! Fields are: family, style, size, file.
 */
int FontDialog::send()
{
	if (!win_owner) return 1;

	if (currentfont<0) FindFont();

	StrsEventData *s=new StrsEventData;
	s->send_message=newstr(win_sendthis);
	s->to=win_owner;
	s->from=object_id;

	s->strs=new char*[4];
	s->n=4;
	s->strs[0]=NULL;
	makestr(s->strs[0],fonts->e[currentfont]->family);

	s->strs[1]=NULL;
	makestr(s->strs[1],fonts->e[currentfont]->style);

	s->strs[2]=new char[30];
	sprintf(s->strs[2],"%.10g",fontsize->Valuef());

	s->strs[3]=NULL;
	makestr(s->strs[3],fonts->e[currentfont]->file);

	s->object=thefont;
	thefont->inc_count();

	app->SendMessage(s);
	return 0;
}

//! Called after a change to the font family, this updates the styles popup.
void FontDialog::UpdateStyles()
{
	if (currentfont<0) return;

	int start=currentfont;
	int end=currentfont;
	while (start>0 && !strcmp(fonts->e[start-1]->family, fonts->e[start]->family)) start--;
	while (end<fonts->n-1 && !strcmp(fonts->e[end+1]->family, fonts->e[end]->family)) end++;

	styles.Flush();
	for (int c=start; c<=end; c++) {
		styles.AddItem(fonts->e[c]->style, c);
	}

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

