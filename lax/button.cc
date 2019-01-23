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




#include <lax/button.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/language.h>

#include <iostream>
using namespace std;
#define DBG 


using namespace LaxFiles;

namespace Laxkit {


	
/*! \class Button
 * \brief Simple class for a button that is an icon with optional label. Sends message when pressed.
 *
 * If the image is not valid, then the text is used. If neither are valid, then
 * no image is drawn, but the bevel will still be drawn.
 *
 * labelstyle can be
 *   LAX_ICON_ONLY,
 *   LAX_TEXT_ONLY,
 *   LAX_TEXT_ICON,
 *   LAX_ICON_TEXT,
 * You can specify for this with the corresponding styles:
 *  IBUT_ICON_ONLY,
 *  IBUT_TEXT_ONLY,
 *  IBUT_TEXT_ICON,
 *  IBUT_ICON_TEXT
 * 
 */
/*! \fn const char *Button::Label()
 * \brief Return what is the current label.
 */



/*! If img, then install img in preference to filename.
 *
 *  Does not call inc_count() on the image.
 */
Button::Button(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int nid,
						const char *nlabel,
						const char *filename,LaxImage *img,
						int npad,int ngap)
		: ButtonBase(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendmes,nid)
{
	font=NULL;

	pad = npad; if (pad<0) pad = app->theme->default_padx;
	gap = npad; if (gap<0) gap = app->theme->default_padx;
	state=oldstate=1;

	thing=THING_None;
	thingw=thingh=0;

	label=newstr(nlabel);
	labelstyle=LAX_ICON_TEXT;

	if (win_style&IBUT_TEXT_ONLY) labelstyle=LAX_TEXT_ONLY;
	else if (win_style&IBUT_ICON_ONLY) labelstyle=LAX_ICON_ONLY;
	else if (win_style&IBUT_ICON_TEXT) labelstyle=LAX_ICON_TEXT;
	else if (win_style&IBUT_TEXT_ICON) labelstyle=LAX_TEXT_ICON;


	if (!label && nid>=0) {
		const char *maybe=Label(nid);
		if (!isblank(maybe)) Label(maybe);
	}
	if (!label && (win_style&BUTTON_TEXT_MASK)) {
		const char *maybe=Label(win_style&BUTTON_TEXT_MASK);
		if (!isblank(maybe)) Label(maybe);
	}

	image=bwimage=NULL;
	if (img) SetIcon(img);
	else if (filename) SetIcon(filename);
	
	 // wrap window to extent
	if (ww<2 || hh<2) WrapToExtent((ww<2?1:0)|(hh<2?2:0));

	//DBG if (image) cerr <<WindowTitle()<<" make image succeeded"<<endl;
	//DBG else  cerr <<WindowTitle()<<" make image not succeeded"<<endl;
}

//! Calls dec_count() on images.
Button::~Button()
{
	if (font)    font   ->dec_count();
	if (image)   image  ->dec_count();
	if (bwimage) bwimage->dec_count();
}

//! Set win_w (if which&1) and win_h (if which&2)  to the extent of the icon/label.
void Button::WrapToExtent(int which)
{
	int w,h;
	if (image || thing==THING_None) get_placement(image,win_themestyle->normal,label,gap,labelstyle,&w,&h, NULL,NULL,NULL,NULL);
	else get_placement(thingw,thingh,win_themestyle->normal,label,gap,labelstyle,&w,&h, NULL,NULL,NULL,NULL);
	if (which&1) win_w=w+2*bevel+2*pad;
	if (which&2) win_h=h+2*bevel+2*pad;
}

/*! Use this font. NULL means use system default.
 * will inc count of font (when it's not already the font).
 */
int Button::Font(LaxFont *nfont)
{
	if (nfont != font) {
		if (font) font->dec_count();
		font = nfont;
		if (nfont) nfont->inc_count();
	}
	needtodraw=1;
	return 1;
}

//! Change the label to nlabel, and set needtodraw.
/*! Return what the label is made into.
 */
const char *Button::Label(const char *nlabel)
{
	makestr(label,nlabel);
	needtodraw=1;
	return label;
}

//! Pass in something like BUTTON_OK or BUTTON_CANCEL.
/*! Choices are: Ok, Cancel, Open, Save, Save As, Save All, 
 * Close, Close All, Quit, Quit Anyway, Print, Preview,
 * Yes, No, and Overwrite.
 */
const char *Button::Label(unsigned int which) 
{
	switch (which) {
		case BUTTON_OK         : return _("Ok"); 
		case BUTTON_CANCEL     : return _("Cancel"); 
		case BUTTON_OPEN       : return _("Open"); 
		case BUTTON_SAVE       : return _("Save"); 
		case BUTTON_SAVE_AS    : return _("Save As"); 
		case BUTTON_SAVE_ALL   : return _("Save All"); 
		case BUTTON_CLOSE      : return _("Close"); 
		case BUTTON_CLOSE_ALL  : return _("Close All"); 
		case BUTTON_QUIT       : return _("Quit"); 
		case BUTTON_QUIT_ANYWAY: return _("Quit Anyway"); 
		case BUTTON_PRINT      : return _("Print"); 
		case BUTTON_PREVIEW    : return _("Preview"); 
		case BUTTON_YES        : return _("Yes"); 
		case BUTTON_NO         : return _("No"); 
		case BUTTON_OVERWRITE  : return _("Overwrite");
	}
	return "";
}

//! Use a graphic instead of an image.
/*! This is something that can be drawn with draw_thing().
 *
 * This will dec_count the image if present.
 *
 * Return 0 for thing installed. Nonzero for no change.
 */
int Button::SetGraphic(int newthing, int newwidth, int newheight)
{
	if (image)   image  ->dec_count();
	if (bwimage) bwimage->dec_count();
	image=bwimage=NULL;

	thing=newthing;
	thingw=newwidth;
	thingh=newheight;
	if (thingw<=0) {
		if (label) {
			double th;
			GetDisplayer()->textextent(label,-1,NULL,&th);
			thingw=th/2;
		}
		else thingw=app->defaultlaxfont->textheight()/2;
	}
	if (thingh<=0) thingh=thingw;

	needtodraw=1;
	return 0;
}

//! Return 0 on success, else nonzero error.
/*! if makebw, then also make a black and white version of the icon.
 *
 * \todo *** make black and white is not currently implemented.
 */
int Button::SetIcon(const char *filename,int makebw) // makebw=0
{
	if (!filename) return 1;
	
	if (image)   image  ->dec_count();
	if (bwimage) bwimage->dec_count();
	image=bwimage=NULL;
	
	thing = THING_None;
	image = ImageLoader::LoadImage(filename);
	needtodraw=1;
	return image?0:1;
}

//! Return 0 on success, 1 otherwise. If makebw, then make a black and white copy too.
/*! Transfers pointer, does not make copy and does not call inc_count() on img.
 *
 * \todo makebw
 */
int Button::SetIcon(LaxImage *img,int makebw) // makebw=0
{
	if (!img || !img->imagestate()) return 1;
	
	if (image)   image  ->dec_count();
	if (bwimage) bwimage->dec_count();
	image=bwimage=NULL;
	
	image=img;
	
	needtodraw=1;
	return 0;
}

/*! \todo implement the b+w drawing.
 */
void Button::draw()
{
	Displayer *dp = GetDisplayer();
	dp->MakeCurrent(this);
	if (font) {
		dp->font(font, font->textheight());
	}
	dp->NewFG(mousein ? win_themestyle->bghover : win_themestyle->bg);
	dp->drawrectangle(0,0, win_w,win_h, 1);
	
	unsigned int what=labelstyle;
	
	const char *l=NULL;
	LaxImage *i=NULL;
	int usei=0;
	if (!(image || thing!=THING_None) || what==LAX_TEXT_ONLY || what==LAX_TEXT_ICON || what==LAX_ICON_TEXT) l=label;
	if ((image || thing!=THING_None) && (what==LAX_ICON_ONLY || what==LAX_TEXT_ICON || what==LAX_ICON_TEXT)) { usei=1; i=image; }
	
	 // set up placement
	double tx=0,ty,th=0,tw=0,ix=0,iy,iw=0,ih=0;
	if (l) dp->textextent(l,-1,&tw,&th);
	if (usei) {
		if (image) { iw=image->w(); ih=image->h(); }
		else { iw=thingw; ih=thingh; }
	}

	ty=win_h/2-th/2;
	iy=win_h/2-ih/2;
	if (l && usei) {
		if (what==LAX_TEXT_ICON) {
			tx=win_w/2-(tw+iw+gap)/2;
			ix=tx+tw+gap;
		} else {
			ix=win_w/2-(tw+iw+gap)/2;
			tx=ix+iw+gap;
		}
	} else if (l) { tx=win_w/2-tw/2;
	} else if (usei) { ix=win_w/2-iw/2;
	}

	//DBG cerr <<" ---------button: "<<whattype()<<"  ix="<<ix<<endl;
	//DBG cerr <<"            ty:"<<ty<<"  th:"<<th<<endl;
	
	 // draw the stuff
	flatpoint offset;
	if (state&LAX_ON) { offset.x=offset.y=bevel/2; }

	if (usei) {
		if (image) {
			if (Grayed()) dp->setSourceAlpha(.4);
			dp->imageout(i, ix,iy);
			if (Grayed()) dp->setSourceAlpha(1.);
			i->doneForNow();
		} else dp->drawthing(ix+iw/2,iy+iy/2, iw/2,ih/2, (DrawThingTypes)thing, win_themestyle->fg.Pixel(), win_themestyle->color1.Pixel());
	}

	if (l) {
		dp->NewFG((state==LAX_GRAY||Grayed()) ? win_themestyle->fggray : win_themestyle->fg);
		dp->textout(tx+offset.x,ty+offset.y, l,-1, LAX_LEFT|LAX_TOP);
	}
	
	if ((win_style&BUTTON_TOGGLE)
			|| !(win_style&IBUT_FLAT)
			|| ((win_style&IBUT_FLAT) && state!=LAX_OFF))
		dp->drawBevel(bevel,highlight,shadow,state, 0,0, win_w,win_h);

	if (font) {
		dp->font(app->defaultlaxfont, app->defaultlaxfont->textheight());
	}
}


/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *Button::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);

	if (what==-1) {
		att->push("text","string");
		att->push("align","one of: left center right");
		att->push("icon","(file) Icon to use");
		att->push("momentary","Or toggle");
		att->push("send","(string) Message to send when clicked");
		//Todo: att->pushsub("mods","Mix of shift,control,meta,alt, Send the message when these modifiers are pressed when clicking");
		return att;
	}

	att->push("win_name",win_name);
	att->push("text",Label());
	if (win_sendthis) att->push("send",win_sendthis);
	if (image) att->push("icon",image->filename);

	if (win_style&BUTTON_MOMENTARY) att->push("momentary");
	else att->push("toggle");


	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void Button::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"momentary")) {
			setWinStyle(BUTTON_MOMENTARY,1);
			setWinStyle(BUTTON_TOGGLE,0);

		} else if (!strcmp(name,"toggle")) {
			setWinStyle(BUTTON_MOMENTARY,0);
			setWinStyle(BUTTON_TOGGLE,1);

		} else if (!strcmp(name,"text")) {
			Label(value);

		} else if (!strcmp(name,"send")) {
			makestr(win_sendthis,value);

		} else if (!strcmp(name,"icon")) {
			SetIcon(value);
		}
	}
}

} // namespace Laxkit

