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


Button::IconSizeType Button::default_icon_size_type = Button::Relative_To_Font;
double Button::default_icon_height = 1.2;


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
	font = nullptr;
	icon_size_type = default_icon_size_type;
	icon_height = default_icon_height;

	pad = npad; if (pad < 0) pad = UIScale() * app->theme->default_padx;
	gap = npad; if (gap < 0) gap = UIScale() * app->theme->default_padx;
	state = oldstate = 1;

	thing  = THING_None;
	thingw = thingh = 0;
	thing_on        = THING_Undefined; //means default to thing
	thing_fill      = 2;
	thing_on_fill   = 2;

	label = newstr(nlabel);
	labelstyle = LAX_ICON_TEXT;

	if      (win_style & IBUT_TEXT_ONLY) labelstyle = LAX_TEXT_ONLY;
	else if (win_style & IBUT_ICON_ONLY) labelstyle = LAX_ICON_ONLY;
	else if (win_style & IBUT_ICON_TEXT) labelstyle = LAX_ICON_TEXT;
	else if (win_style & IBUT_TEXT_ICON) labelstyle = LAX_TEXT_ICON;


	if (!label && nid >= 0) {
		const char *maybe = Label(nid);
		if (!isblank(maybe)) Label(maybe);
	}
	if (!label && (win_style & BUTTON_TEXT_MASK)) {
		const char *maybe = Label(win_style&BUTTON_TEXT_MASK);
		if (!isblank(maybe)) Label(maybe);
	}

	image = bwimage = nullptr;
	if (img) SetIcon(img);
	else if (filename) SetIcon(filename);

	 // wrap window to extent
	if (ww<2 || hh<2) WrapToExtent((ww<2?1:0)|(hh<2?2:0));
}

//! Calls dec_count() on images.
Button::~Button()
{
	delete[] label;
	if (font)    font   ->dec_count();
	if (image)   image  ->dec_count();
	if (bwimage) bwimage->dec_count();
}

void Button::ThemeChanged()
{
	anXWindow::ThemeChanged();
}

void Button::GetPlacement(double *w,double *h,double *tx,double *ty,double *ix,double *iy, double *iw,double *ih)
{
	double ww=0, hh=0;

	if (image) {
		if (icon_size_type == Relative_To_Font) {
			hh = UIScale() * (font ? font : win_themestyle->normal)->textheight() * icon_height;
		} else {
			hh = image->h();
		}
		ww = hh / image->h() * image->w();
	} else {
		if (icon_size_type == Relative_To_Font) {
			ww = UIScale() * thingw;
			hh = UIScale() * thingh;
		} else {
			ww = thingw;
			hh = thingh;
		}
	}

	if (iw) *iw = ww;
	if (ih) *ih = hh;

	get_placement(ww,hh, (font ? font : win_themestyle->normal), label,gap,labelstyle, w,h, tx,ty, ix,iy, UIScale());
}

//! Set win_w (if which&1) and win_h (if which&2)  to the extent of the icon/label.
void Button::WrapToExtent(int which)
{
	double w=0, h=0;

	GetPlacement(&w,&h, nullptr,nullptr, nullptr,nullptr, nullptr,nullptr);

	if (which&1) win_w = w+2*bevel+2*pad;
	if (which&2) win_h = h+2*bevel+2*pad;
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
 * Note this ONLY returns a string, it does not change internal state.
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
	image = bwimage = NULL;

	thing  = newthing;
	thing_on = newthing;
	thing_on_fill = thing_fill = 2;

	thingw = newwidth;
	thingh = newheight;
	if (thingw <= 0) {
		thingw = win_themestyle->normal->textheight() / 2;
		// if (label) {
		// 	double th;
		// 	GetDisplayer()->textextent(label, -1, NULL, &th);
		// 	thingw = th / 2;
		// } else
		// 	thingw = win_themestyle->normal->textheight() / 2;
	}
	if (thingh <= 0) thingh = thingw;

	needtodraw = 1;
	return 0;
}

//! Use one graphic for on, another graphic for off.
/*! This is something that can be drawn with draw_thing(). See THING_*.
 * Fill values are 0 for stroke only. 1 for fill only, 2 for fill and stroke, which strokes with fg, and fills with color1.
 *
 * This will dec_count the image if present.
 *
 * Return 0 for thing installed. Nonzero for no change.
 */
int Button::SetGraphicOnOff(int newthing_on, int on_fill, int newthing_off, int off_fill, int newwidth, int newheight)
{
	if (image)   image  ->dec_count();
	if (bwimage) bwimage->dec_count();
	image = bwimage = nullptr;

	thing         = newthing_off;
	thing_fill    = off_fill;
	thing_on      = newthing_on;
	thing_on_fill = on_fill;

	if (newwidth > 0 || newheight > 0) {
		thingw = newwidth;
		thingh = newheight;
	}

	if (thingw <= 0) {
		thingw = win_themestyle->normal->textheight() / 2;
	}
	if (thingh <= 0) thingh = thingw;

	needtodraw = 1;
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
	double th = 0;
	if (font) {
		th = UIScale() * font->textheight();
		dp->font(font, th);
	} else {
		th = UIScale() * win_themestyle->normal->textheight();
		dp->font(win_themestyle->normal, th);
	}
	dp->NewFG(mousein ? win_themestyle->bghover : win_themestyle->bg);
	dp->drawrectangle(0,0, win_w,win_h, 1);

	unsigned int what=labelstyle;

	 // set up placement
	double w,h, tx=0,ty,tw=0, ix=0,iy,iw=0,ih=0;
	GetPlacement(&w,&h, &tx,&ty, &ix,&iy, &iw,&ih);
	
	const char *l = label;
	LaxImage *i = image;
	int usei = 0;

	if (tx == LAX_WAY_OFF) l = nullptr; else tw = UIScale() * (font ? font : win_themestyle->normal)->Extent(l,-1);
	if (ix == LAX_WAY_OFF) i = nullptr; else usei = 1;
	
	ty = win_h/2-th/2;
	iy = win_h/2-ih/2;
	if (l && usei) {
		if (what == LAX_TEXT_ICON) {
			tx = win_w/2-(tw+iw+gap)/2;
			ix = tx+tw+gap;
		} else {
			ix = win_w/2-(tw+iw+gap)/2;
			tx = ix+iw+gap;
		}

	} else if (l)    {
		tx = win_w/2 - tw/2;

	} else if (usei) {
		ix = win_w/2 - iw/2;
	}

	 // draw the stuff
	flatpoint offset;
	if (state&LAX_ON) { offset.x = offset.y = bevel/2; }

	if (usei) {
		if (image) {
			if (Grayed()) dp->setSourceAlpha(.4);
			dp->imageout(i, ix,iy, iw,ih);
			if (Grayed()) dp->setSourceAlpha(1.);
			i->doneForNow();
		} else {
			dp->LineWidthScreen(1); //TODO: *** this should be something like theme->ScreenLine()
			if (state & LAX_ON) {
				if (thing_on_fill == 2)
					dp->drawthing(ix+iw/2,iy+ih/2, iw/2,ih/2, (DrawThingTypes)thing_on, win_themestyle->fg.Pixel(), win_themestyle->color1.Pixel());
				else {
					dp->NewFG(win_themestyle->fg.Pixel());
					dp->drawthing(ix+iw/2,iy+ih/2, iw/2,ih/2, thing_on_fill, (DrawThingTypes) thing_on);
				}
			} else {
				if (thing_fill == 2)
					dp->drawthing(ix+iw/2,iy+ih/2, iw/2,ih/2, (DrawThingTypes)thing, win_themestyle->fg.Pixel(), win_themestyle->color1.Pixel());
				else {
					dp->NewFG(win_themestyle->fg.Pixel());
					dp->drawthing(ix+iw/2,iy+ih/2, iw/2,ih/2, thing_fill, (DrawThingTypes) thing);
				}
			}
		}
	}

	if (l) {
		dp->NewFG((state==LAX_GRAY||Grayed()) ? win_themestyle->fggray : win_themestyle->fg);
		dp->textout(tx+offset.x,ty+offset.y, l,-1, LAX_LEFT|LAX_TOP);
	}

	if (((win_style&BUTTON_TOGGLE) && (state != LAX_OFF || (state == LAX_ON && !(win_style&IBUT_FLAT))))
			|| !(win_style&IBUT_FLAT)
			|| ((win_style&IBUT_FLAT) && state!=LAX_OFF)) {
		dp->drawBevel(bevel,highlight,shadow,state, 0,0, win_w,win_h);
	}
}


/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *Button::dump_out_atts(Attribute *att,int what,DumpContext *context)
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
void Button::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"momentary")) {
			SetWinStyle(BUTTON_MOMENTARY,1);
			SetWinStyle(BUTTON_TOGGLE,0);

		} else if (!strcmp(name,"toggle")) {
			SetWinStyle(BUTTON_MOMENTARY,0);
			SetWinStyle(BUTTON_TOGGLE,1);

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
