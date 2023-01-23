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
//    Copyright (C) 2022 by Tom Lechner
//


#include <cstring>
#include <lax/label.h>
#include <lax/laxutils.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

/*! \class Label
 * \brief A label capable of minimally formatted text (color, size, and font:normal, bold, italic, or monospace).
 * Pass text and this window will display it.
 * You can have multiple lines by putting '\\n' characters in 
 * the text.
 *
 * If there is too much text to fit in the window, you can optionally
 * allow the user to drag the text around. Simply define LabelTypes::MOVE, and
 * right clicking will shift around the text. Shift and/or Control 
 * right drag speeds up the shifting.
 *
 * Left click to copy to clipboard if LabelTypes::COPY is set.
 *
 */
/*! \var int Label::ex
 * \brief The x extent of the text.
 */
/*! \var int Label::ey
 * \brief The y extent of the text.
 */
/*! \var int Label::ox
 * \brief The x coordinate of the left edge of the text extent.
 */
/*! \var int Label::oy
 * \brief The y coordinate of the baseline of the top line of text.
 */


// Convenience contructor, which just calls the main constructor with empty values.
Label::Label()
 : Label(nullptr, nullptr, nullptr, 0, 0,0,0,0,0, 0, nullptr, 0, 0)
{}


/*! Pass in the new text after the regular window parameters.
 * Multi-line text is simply a char[] with '\\n' characters in it.
 *  The text is then copied and stored by line in thetext.
 *
 *  If win_w or win_h are 0, and newtext is supplied here, then win_w and
 *  win_h are set to the extent of the message plus pads.
 *
 * Text can be formatted with some basic html-like syntax:
 *     "<b>Bold Text</b> <i>italic text</i> <code>Monospaced</code>"
 *     "<color index=\"3\">Specific color from a palette</color>"
 *     "<size pt=\"3\">Specific absolute size</size>"
 *     "<size em=\"1.5\">Size relative to current</size>"
 *     "<b><i>Nested</i>styles</b>"
 */
Label::Label(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long window_flags,
						int nx,int ny,int nw,int nh,int brder,
						unsigned long label_flags,
						const char *newtext, int pad_x, int pad_y) 
				: anXWindow(pwindow,nname,ntitle,nstyle, nx,ny, nw,nh,brder, nullptr,0,nullptr) 
{ ***
	InstallColors(THEME_Panel);

	needtodraw = 1;
	firsttime  = 1;
	nlines     = 0;
	indents    = nullptr;
	colors     = nullptr;

	DBG if (!newtext) cerr << WindowTitle() << ": no new text" << endl;
	DBG else cerr << "New Mesbartext:" << newtext << endl;

	SetText(newtext);

	offset_x = 0;
	offset_y = 0;
	msx = msy = ex = ey = 0;
	padx = pad_x;
	pady = pad_y;

	if ((label_style & (LabelTypes::CENTER | LabelTypes::LEFT | LabelTypes::RIGHT | LabelTypes::TOP | LabelTypes::BOTTOM)) == 0) {  // default center
		label_style |= LabelTypes::CENTER;
	}

	SetupMetrics();  // sets win_w and win_h if nh==0
}


Label::~Label()
{
	if (colors) colors->dec_count();
}


int Label::Event(const EventData *e,const char *mes)
{
	if (e->type == LAX_onMouseOut && (label_style & LabelTypes::LEAVE_DESTROYS)) {
		app->destroywindow(this);
	}
	return anXWindow::Event(e,mes);
}

int Label::init()
{
	return 0;
}


//! Get a char[] copy of the text, with '\\n' for the linebreaks.
char *Label::GetText()
{
	if (modified) RebuildRaw();
	if (rawtext.IsEmpty()) return nullptr;
	return newstr(rawtext.c_str());
}

//! Get a char[] copy of the text, marked up with style tags as necessary. Uses '\\n' for the linebreaks.
char *Label::GetTaggedText()
{
	if (modified) RebuildRaw();
	if (rawtext.IsEmpty()) return nullptr;
	return newstr(rawtext.c_str());
}

//! Get a char[] copy of the message text, with '\\n' for the linebreaks.
char *Label::GetConstText()
{
	if (modified) RebuildRaw();
	if (rawtext.IsEmpty()) return nullptr;
	return rawtext.c_str();
}


//! Use this color for future AddText() calls.
int Label::UseColor(ScreenColor *color)
{***}

//! Use this font for future AddText() calls.
int Label::UseFont(LaxFont *font)
{***}

int Label::AddText(const char *text)
{***}

int Label::AddImage(LaxImage *img, int pad_x, int pad_y)
{***}

int Label::AddGraphic(DrawThingTypes graphic)
{***}


//! Redefine the message text.
/*! \todo this can use split()
 * \todo *** implement LabelTypes::WRAP
 */
int Label::SetText(const char *newtext, int type)
{ ***
	if (!newtext) return 1;
	//DBG cerr <<WindowTitle()<<" SetText: "<<newtext<<endl;
	for (int c=0; c<nlines; c++) delete[] thetext[c];
	if (nlines) {
		delete[] indents;
		delete[] thetext;
	} 

//	*** lines ending with \n are actually newlines. otherwise, they are
//		inserted breaks, due to wrapping

	if (!newtext) {
		thetext=new char*[1];
		thetext[0]=new char[2];
		thetext[0][0]=' '; thetext[0][1]='\0';
		indents=new double[1];
		nlines=1;
		firsttime=1;
		needtodraw=1;
		return 0;
	}
	nlines=1;
	int nl=0;
	int c,p;
	int tl=strlen(newtext);
	for (c=0; c<tl; c++) if (newtext[c]=='\n') nlines++;
	indents=new double[nlines];
	thetext=new char*[nlines];
	
	 // split text by \n
	c=0;
	for (nl=0; nl<nlines; nl++) {
		p=c;
		while (p<tl && newtext[p]!='\n') p++;
		if (p==c) { // blank line
			thetext[nl]=new char[2];
			thetext[nl][0]=' ';
			thetext[nl][1]='\0';
			c++;
			continue;
		}
		thetext[nl]=new char[p-c+1];
		strncpy(thetext[nl],newtext+c,p-c);
		thetext[nl][p-c]='\0';
		//DBG cerr <<"line "<<nl<<"("<<strlen(thetext[nl])<<"): "<<thetext[nl]<<endl;
		c=p+1;
	}
		
	offset_x = offset_y = 0;
	firsttime = 1;
	needtodraw = 1;
	return 0;
}

//!  If window dimensions are initially == 0 this sets them to extent of the message plus pads
int Label::SetupMetrics()
{ ***
	if (!thetext || !app) return 1;
	int n;
	ex=0;
	for (n=0; n<nlines; n++) {
		win_themestyle->normal->Extent(thetext[n],strlen(thetext[n]), &indents[n], &height, &fasc, &fdes);
		if (indents[n]>ex) ex=indents[n];
	}
	ey=nlines*height;

	if (win_w==0) win_w=ex+2*padx;
	if (win_h==0) win_h=ey+2*pady;

	if (label_style&LabelTypes::RIGHT) ox=win_w-ex-2*padx;
	else if (label_style&LabelTypes::LEFT) ox=padx; 
	else ox=(win_w-ex)/2; //center

	for (n=0; n<nlines; n++) {
		if (label_style&LabelTypes::RIGHT) indents[n]=win_w-indents[n]-2*padx-ox;
		else if (label_style&LabelTypes::LEFT) indents[n]=padx-ox;
		else indents[n]=(win_w-indents[n])/2-ox; //center
	}
	
	if (label_style&LabelTypes::TOP) oy=fasc+pady; 
	else if (label_style&LabelTypes::BOTTOM) oy=win_h-ey+fasc-2*pady;
	else oy=(win_h-ey)/2+fasc; //center
	
	//firsttime=0;
	return 0;
}
	
void Label::Refresh()
{
	if (!win_on || !needtodraw) return;

	if (firsttime) {
		if (SetupMetrics()) {
			if (rawtext.IsEmpty()) needtodraw = 0;
			return;
		}
		firsttime = 0;
	}

	//DBG cerr <<"mesbar("<<WindowTitle()<<")drawing at: "<<ox<<','<<oy<<endl;
	
	Displayer *dp = MakeCurrent();
	dp->NewBG(win_themestyle->bg.Pixel());
	dp->NewFG(win_themestyle->fg);
	dp->ClearWindow();


	LaxFont *cur_font = win_themestyle->normal;
	LaxFont *cur_color = &win_themestyle->fg;
	dp->font(cur_font->normal);
	dp->NewFG(cur_color);

	for (int c=0; c<thetext.n; c++) {
		LabelChuck *chunk = thetext.e[c];

		if (chunk->content == LabelChunk::Text) {
			if (chunk->font)  { dp->font(chunk->font);   cur_font  = chunk->font; }
			if (chunk->color) { dp->NewFG(chunk->color); cur_color = chunk->color }

			//dp->textout(ox+indents[c],oy+c*height, thetext[c],l, LAX_LEFT|LAX_BASELINE);
			dp->glyphsout(offset_x + chunk->minx, offset_y + chunk->miny, nullptr, chunk->glyphs.e, chunk->glyphs.n, LAX_LEFT);

		} else if (chunk->content == LabelChunk::Image) {
			if (chunk->img == nullptr) continue;
			float width  = chunk->img->w();
			float height = chunk->img->h();
			dp->imageout(chunk->img, offset_x+chunk->minx, offset_x+chunk->miny, chunk->boxwidth(), chunk->boxheight());

		} else if (chunk->content == LabelChunk::Graphic) {
			flatvector v = chunk->BBoxPoint(.5,.5);
			dp->drawthing(offset_x + v.x, offset_y + v.y, chunk->boxwidth()/2, chunk->boxheight()/2, chunk->info, fg,bg);
		}
	}

	if (win_style & ANXWIN_DOUBLEBUFFER) SwapBuffers();
	needtodraw=0;
	return;
}


int Label::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, LEFTBUTTON, x,y);
	return 1;
}

int Label::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, LEFTBUTTON);

	if (!(label_style & LabelTypes::COPY)) return 1;

	 //	copy to clipboard
	const char *str = GetConstText();
	if (str) {
		app->CopytoBuffer(str,strlen(str)); 
		//DBG cerr <<"(mb copy: "<<str<<")\n";
	}
	return 0;
}


int Label::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, MIDDLEBUTTON, x,y);
	return 1;
}

int Label::MBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, MIDDLEBUTTON);
	return 0;
}


int Label::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	return 0;
}

int Label::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, RIGHTBUTTON);
	return 0;
}


/*! If allowed with LabelTypes::MOVE, this shifts around text that is smaller
 * than the window only within the window boundaries, and shifts 
 * around larger text only just enough so that you can see it.
 */
int Label::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{ ***
	if (!buttondown.any(d->id) || !(label_style&LabelTypes::MOVE)) return 1;

	//DBG cerr<<"\nMB move:";

	int msx, msy;
	buttondown.move(d->id, x,y, &msx, &msy);
	buttondown.average(-1, &x,&y);

	int mult=1;
	if ((state&LAX_STATE_MASK)==ShiftMask || (state&LAX_STATE_MASK)==ControlMask) mult=4;
	else if ((state & LAX_STATE_MASK)==(ShiftMask|ControlMask)) mult=8;

	//DBG cerr <<"mb try move "<<ox<<','<<oy;
	if (label_style & LabelTypes::MOVE) {
		ox += mult * (x - msx);
		oy += mult * (y - msy);
	}
	//DBG cerr <<"  -->  "<<ox<<','<<oy;

	 //clamp if necessary
	if (x - msx > 0) {  // moving right
		if (ox - padx > 0 && ox + padx + ex > win_w) {
			if (ex + 2 * padx < win_w)
				ox = win_w - ex - padx;
			else
				ox = padx;
		}
	} else if (x - msx < 0) {  // moving left
		if (ox - padx < 0 && ox + padx + ex < win_w) {
			if (ex + 2 * padx < win_w)
				ox = padx;
			else
				ox = win_w - ex - padx;
		}
	}

	if (y - msy > 0) {  // move down
		// DBG cerr <<" ============ ey="<<ey<<" oy-fasc="<<oy-fasc<<" pady="<<pady<<" win_h="<<win_h<<endl;

		if (oy - fasc - pady > 0 && pady + oy - fasc + ey + pady > win_h) {  // box is below upper pad, lower edge is below lower pad
			if (ey + 2 * pady < win_h)
				oy = win_h - ey - pady + fasc;  // box fits in window
			else
				oy = fasc + pady;  // box does not fit in window
		}
	} else if (y - msy < 0) {  // move up
		if (oy - fasc - pady <= 0 && oy - fasc + pady + ey < win_h) {
			if (ey + 2 * pady < win_h)
				oy = fasc + pady;
			else
				oy = win_h - ey - pady + fasc;
		}
	}

	//DBG cerr <<"  end at  "<<ox<<','<<oy<<endl;

	needtodraw = 1;
	return 0;
}

//! Scroll contents.
int Label::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{ ***
	if (!(label_style & LabelTypes::MOVE)) return 0;

	if (state & (ShiftMask|ControlMask)) offset_y += win_h*3/4; else offset_y += 20;
	if (offset_y - fasc-pady>0 && pady+offset_y-fasc+ey+pady>win_h) { // box is below upper pad, lower edge is below lower pad
		if (ey+2*pady<win_h) offset_y = win_h-ey-pady+fasc; // box fits in window
		else offset_y = fasc+pady; // box does not fit in window
	}
	needtodraw=1;
	return 0;
}

//! Scroll contents.
int Label::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{ ***
	if (!(label_style & LabelTypes::MOVE)) return 0;
	
	if (state&(ShiftMask|ControlMask)) offset_y -= win_h*3/4; else offset_y -= 20;
	if (offset_y-fasc-pady<=0 && offset_y-fasc+pady+ey<win_h) {
		if (ey+2*pady<win_h) offset_y = fasc+pady;
		else offset_y = win_h-ey-pady+fasc;
	}
	needtodraw=1;
	return 0;
}

int Label::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	SetupMetrics();
	needtodraw = 1;
	return 0;
}

int Label::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	SetupMetrics();
	needtodraw = 1;
	return 0;
}


/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *Label::dump_out_atts(Attribute *att,int what,DumpContext *context)
{ ***
	if (!att) att = new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	if (what == -1) {
		att->push("text","string");
		att->push("halign","one of: left center right");
		att->push("valign","one of: left center right");
		return att;
	}

	char *txt = GetTaggedText();
	att->push("text",txt);
	delete[] txt;

	*** dump out color, fonts

	char align[20];
	if (label_style&LabelTypes::LEFT)         strcpy(align,"left");
	else if (label_style&LabelTypes::CENTERX) strcpy(align,"center");
	else if (label_style&LabelTypes::RIGHT)   strcpy(align,"right");
	att->push("halign",align);

	if (label_style&LabelTypes::TOP)          strcpy(align,"top");
	else if (label_style&LabelTypes::CENTERY) strcpy(align,"center");
	else if (label_style&LabelTypes::BOTTOM)  strcpy(align,"bottom");
	att->push("valign",align);

	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void Label::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{ ***
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"text")) {
			SetTaggedText(value);

		} else if (!strcmp(name, "halign")) {
			label_style &= ~(LabelTypes::LEFT | LabelTypes::CENTERX | LabelTypes::RIGHT);
			if (!strcmp(value, "left"))        label_style |= LabelTypes::LEFT;
			else if (!strcmp(value, "center")) label_style |= LabelTypes::CENTERX;
			else if (!strcmp(value, "right"))  label_style |= LabelTypes::RIGHT;
			if ((label_style & (LabelTypes::LEFT | LabelTypes::CENTERX | LabelTypes::RIGHT)) == 0)
				label_style |= LabelTypes::CENTERX;

		} else if (!strcmp(name, "valign")) {
			label_style &= ~(LabelTypes::TOP | LabelTypes::CENTERY | LabelTypes::BOTTOM);
			if (!strcmp(value, "left"))        label_style |= LabelTypes::TOP;
			else if (!strcmp(value, "center")) label_style |= LabelTypes::CENTERY;
			else if (!strcmp(value, "right"))  label_style |= LabelTypes::BOTTOM;
			if ((label_style & (LabelTypes::TOP | LabelTypes::CENTERY | LabelTypes::BOTTOM)) == 0)
				label_style |= LabelTypes::CENTERY;

		} else {
			*** dump in color, fonts
		}
	}
}


} // namespace Laxkit

