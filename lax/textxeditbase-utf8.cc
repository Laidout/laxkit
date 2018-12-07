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
//    Copyright (C) 2004-2010,2015 by Tom Lechner
//



#include <lax/textxeditbase-utf8.h>
#include <lax/utf8utils.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>

// DBG !!!!!
#include <lax/displayer-cairo.h>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {


//----------------------------------------------------------------------------
/*! \class TextXEditBaseUtf8
 * \brief The window base class for text edits.
 *
 * This class provides basic text drawing utilities for subclasses to use. Mainly this means
 * providing textrect to say what portion of the actual window is to be used for drawing text in,
 * and functions to aid drawing single lines of text which take into account 
 * left, right, center, and numeric tabs.
 * 
 * Subclasses must define their own DrawText(). DrawLineOfText() can be called from DrawText(), once
 * the subclass determines what part of thetext is supposed to be shown.
 * a default DrawCaret done here, docaret done here, subclasses must upkeep oldx,oldy and cx,cy.
 * Cursor events must be handled in subclasses.
 *   
 * \todo how best to standardize text controls? 
 * \todo *** snafus with really long lines
 * \todo *** makeinwindow if it can fit, it should fit
 * \todo *** map space to tab for TABS_SPACES in charwidth(), tabwidth=' '*4
 * \todo *** color in the pad space, put little bevel, etc
 * \todo *** NO_TAB
 * \todo *** paste nl conversion
 * \todo *** DrawLineOfText should use a maxpix and a maxpos for refreshing and word wrapping convenience respectively
 */
/*! \var int TextXEditBaseUtf8::curlineoffset
 * \brief The pixel horizontal offset the text is shifted.
 *
 * Say it is 10, then the 10th pixel of the text from the left is located at the edge of textrect.x+pad.
 */
/*! \var int TextXEditBaseUtf8::padx
 * \brief Horizontal inset within textrect to place text.
 *
 * The horizontal area text can fill before scrolling is necessary is textrect.width-2*padx.
 */
/*! \var int TextXEditBaseUtf8::pady
 * \brief Vertical inset within textrect to place text.
 *
 * The vertical area text can fill before scrolling is necessary is textrect.height-2*pady.
 */
/*! \var int TextXEditBaseUtf8::cx
 * \brief The x position of the current position, measured from the top left corner of the window.
 */
/*! \var int TextXEditBaseUtf8::cy
 * \brief The baseline y position of the current line, measured from the top left corner of the window.
 */
/*! \var int TextXEditBaseUtf8::oldx
 * \brief The old value of cx for a caret that might need to be blanked out.
 */
/*! \var int TextXEditBaseUtf8::oldy
 * \brief The old value of cy for a caret that might need to be blanked out.
 */
/*! \var int TextXEditBaseUtf8::dpos
 * \brief The beginning position in thetext that needs to be redrawn.
 */
/*! \var int TextXEditBaseUtf8::nlines
 * \brief The number of lines starting from the line of dpos that must be redrawn.
 *
 * If nlines==0, then assume all the lines should be redrawn.
 */
/*! \var int TextXEditBaseUtf8::oldsellen
 * \brief The previous selection length to help narrow down what on screen must be redrawn.
 */
/*! \var int TextXEditBaseUtf8::oldcp
 * \brief The previous curpos to help narrow down what on screen must be redrawn.
 */
/*! \var int TextXEditBaseUtf8::valid
 * \brief An extra modifier to control what colors are used.
 *
 * This is used, for instance, to make the text background red when a file is invalid.
 * bkwrongcolor will be used in this case, instead of bkcolor.
 */
/*! \var int TextXEditBaseUtf8::con
 * \brief Nonzero if the caret is supposed to be on, and is drawn on, on the screen.
 */
/*! \var DoubleRectangle TextXEditBaseUtf8::textrect
 * \brief The rectangle inside of which text can be drawn.
 */


/*! newtext is assumed to be utf8 encoded. ncntlchar is a UCS code.
 *
 * Subclasses will want to initialize the various colors to something appropriate.
 */
TextXEditBaseUtf8::TextXEditBaseUtf8(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							 int xx,int yy,int ww,int hh,int brder,
							 anXWindow *prev,unsigned long nowner,const char *nsend,
							 const char *newtext,unsigned long ntstyle,int ncntlchar) // newtext=NULL, ntstyle=0 ncntlchar=0
	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend),
 	  TextEditBaseUtf8(newtext,ntstyle,ncntlchar)
{
	thefont=NULL;
	cx=cy=oldx=oldy=0;
	tabwidth=30;//30pixels, rather than 4 chars by default
	con=0;
	valid=1;
	curlineoffset=0;

	firsttime=1; //used as a flag to trigger SetupMetrics() at the start of next Refresh()
	dp=NULL;

	bkwrongcolor=~0;
	bkwrongcolor2=~0;

	padx=0;
	pady=0;
}

/*! Decrements count of thefont.
 */
TextXEditBaseUtf8::~TextXEditBaseUtf8()
{
	if (thefont) thefont->dec_count();
	DBG cerr <<"---textxedit dest"<<endl;
}

//! Draws the caret on.
int TextXEditBaseUtf8::FocusOn(const FocusChangeData *e)
{
	anXWindow::FocusOn(e); // this handles active, and border color change
	//DBG cerr <<"txb: focuson, active:"<<(int)win_active<<"  con:"<<(int)con<<endl;
	if (firsttime) return 0;
	
	if (win_active) {
		dp=MakeCurrent();
		DrawCaret(0,1);
		dp=NULL;
	}
	//DBG cerr <<"txb: focuson, active:"<<(int)win_active<<"  con:"<<(int)con<<endl;
	return 0;
}

//! Draws the caret off.
/*! \todo perhaps should also color the selection differently.
 */
int TextXEditBaseUtf8::FocusOff(const FocusChangeData *e)
{
	anXWindow::FocusOff(e);
	//DBG cerr <<"txb: focusoff, active:"<<(int)win_active<<"  con:"<<(int)con<<endl;
	if (!win_active) {
		if (!con) return needtodraw;
		dp=MakeCurrent();
		DrawCaret(1,0);
		dp=NULL;
	}
	//DBG cerr <<"txb: focusoff, active:"<<(int)win_active<<"  con:"<<(int)con<<endl;
	return 0;
}


const char hexdigits[17]="0123456789abcdef";

//! Put a hex string of i into str.
int hexify(char *str, int i)
{
	sprintf(str,"%x",i); //investigate snprintf to progressively provide enough buffer space...
	return strlen(str);
}

//! Returns real char width with r==1, else returns the custom hexified if r==0.
/*! WARNING: this is found from individual characters in the font, not from the context of the character.
 */
int TextXEditBaseUtf8::charwidth(int ch,int r) //r=0
{
	if (ch==32) return getextent(thefont, " ",-1, NULL,NULL,NULL,NULL, 0);

	char c[20];
	int l;
	l=utf8encode(ch,c);
	c[l]='\0';

	if (r==1) return getextent(thefont, c,-1, NULL,NULL,NULL,NULL, 1);

	int w=getextent(thefont, c,-1, NULL,NULL,NULL,NULL, 0);
	if (w) return w;
	hexify(c,ch);
	w=getextent(thefont, c,-1, NULL,NULL,NULL,NULL, 0);
	return w;
}

//! Just calls settextrect(), and returns 0.
int TextXEditBaseUtf8::init()
{
	settextrect();
	return 0;
}

//! Copy() then delsel().
int TextXEditBaseUtf8::Cut() 
{
	if (!sellen) return 0;
	Copy();
	delsel();
	return 0;
}

//! Calls anXApp::CopytoBuffer() with the current selection.
/*! 
 * \todo *** figure out what's wrong with the copy/paste stuff.. should
 *   be able to copy right from the terminal, but that's not working..
 */
int TextXEditBaseUtf8::Copy() 
{
	if (!sellen) return 0;
	int sl=sellen,ss;
	if (sl<0) sl=-sl;
	if (selstart<curpos) ss=selstart; else ss=curpos;

	TextEditBaseUtf8::Copy(); //updates cutbuffer
	selectionCopy(0);
	app->CopytoBuffer(thetext+ss,sl);

	return 0;
}

//! Utilizes anXApp::GetBuffer().
int TextXEditBaseUtf8::Paste() 
{
	char *c=app->GetBuffer();
	if (!c) return 0;
	insstring(c,1);
	delete[] c;
	return 0;
}

/*! Xlib specific. Return the selected text, or NULL if no selection.
 * Should probably check to make sure targettype is string.
 */
char *TextXEditBaseUtf8::getSelectionData(int *len,const char *property,const char *targettype,const char *selection)
{
	DBG cerr << "TextXEditBaseUtf8::getSelectionData():"<<endl;
	DBG cerr << "  with type "<<(targettype?targettype:("no type"))<<" on selection "<<selection<<endl;

	if (targettype && strcmp(targettype,"UTF8_STRING") && strcmp(targettype,"STRING")) {
		DBG cerr << " WARNING! strange type in TextXEditBaseUtf8::getSelectionData()"<<endl;
		*len=0;
		return NULL;
	}

	if (!sellen) {
		if (cutbuffer) {
			*len=strlen(cutbuffer);
			DBG cerr <<"  returned data("<<(*len)<<")"<<endl;
			return newstr(cutbuffer);
		}
		*len=0;
		return NULL;
	}

	*len= sellen>0?sellen:-sellen;
	DBG cerr <<"  returned data("<<(*len)<<")"<<endl;
	return GetSelText();
}

/*! Paste in text.
 */
int TextXEditBaseUtf8::selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type,const char *which)
{
	DBG cerr << "TextXEditBaseUtf8::selectionDropped():"<<endl;
	DBG cerr << "  with type "<<(actual_type?actual_type:("no type"))<<" on selection "<<which<<endl;
	DBG cerr << "  dropping data("<<len<<"): "<< (data ? data : (const unsigned char*)"(no data)") <<endl;

	if (!data || !*data) return 0;
	char txt[len+1];
	strncpy(txt, (char*)data, len);
	txt[len]='\0';
	insstring(txt,1);
	return 0;
}

//! Initiate a middle button paste via selectionPaste(1,0).
int TextXEditBaseUtf8::MBUp(int x,int y,unsigned int state, const LaxMouse *d)
{
	selectionPaste(1,"UTF8_STRING");
	return 0;
}

//! Draw a marker at top of text area that shows where and what type of tabs exist for the line.
/*! assumes cy set right, draws just above current line
 */
int TextXEditBaseUtf8::DrawTabLine()
{
	Displayer *dp=GetDisplayer();

	 //blank out where the line will be printed
	dp->NewFG(win_themestyle->bg);
	dp->drawrectangle(0,cy-textascent-10, win_w,10, 1);
	dp->NewFG(wscolor);
	
	if (textstyle&(TEXT_RIGHT|TEXT_CENTER)) return 0;
	int c=-curlineoffset,t;
	flatpoint p[3]; //***
	p[0].y=p[1].y=cy-textascent-10;
	p[2].y=cy-textascent;
	DBG cerr <<"   --Drawtabline:"<<tabwidth<<" ";
	while (c<win_w) { 
		c=GetNextTab(c+curlineoffset,t)-curlineoffset;
		//DBG cerr <<c+curlineoffset;
		
		p[1].y=cy-textascent-10;
		switch (t) {
			case LEFT_TAB:
				p[0].x=c-4;
				p[1].x=c;
				p[2].x=c;
				//DBG cerr <<"l ";
				break;
			case RIGHT_TAB:
				p[0].x=c;
				p[1].x=c+4;
				p[2].x=c;
				//DBG cerr <<"r ";
				break;
			case CENTER_TAB:
				p[0].x=c-4;
				p[1].x=c+4;
				p[2].x=c;
				//DBG cerr <<"c ";
				break;
			case NUMERIC_TAB:
				p[0].x=c;
				p[1].x=c+4;
				p[1].y+=5;
				p[2].x=c;
				//DBG cerr <<"c ";
				break;
		}

		 //draw the marker for the tab
		dp->drawlines(p,3, 1, 1);
	}
	DBG cerr <<endl;
	return 0;
}

/*! If needtodraw&1, draw all. Calls DrawText().
 * Else if needtodraw&2, then assumed to be updating either just mouse move,
 * or mouse move plus only a part of the window needs updating.
 *
 * Refresh potentially consists of
 *  optionally drawing the tab markers (done by DrawTabLine()),
 *  drawing the text with the selection shown (done by DrawText()),
 *  and drawing the caret (done by DrawCaret()).
 *
 * This function also calls textdraw(NULL) right at the end.
 */
void TextXEditBaseUtf8::Refresh() 
{
	if (!win_on) return;
	//DBG cerr << "\nEditor painting";
	
	if (firsttime) { 
		firsttime=0; 
		if (SetupMetrics()) { //couldn't set up yet for some reason
			firsttime=1;
			return;
		}
	}

	dp=MakeCurrent();
	double oldheight=dp->textheight();
	dp->font(thefont, thefont->textheight());
	
	DBG DisplayerCairo *ddp=dynamic_cast<DisplayerCairo*>(dp);
	DBG if (ddp && ddp->GetCairo()) cerr <<" TextXEditBaseUtf8 refresh, cairo status:  "<<cairo_status_to_string(cairo_status(ddp->GetCairo())) <<endl;

	if (needtodraw&1) { // draw all
		if (textstyle&TEXT_SHOWTABLINE) DrawTabLine();
		dpos=0; nlines=0;
		DrawCaret(0,0);
		//Black(textrect.x,textrect.y,textrect.width,textrect.height);
		dp->ClearWindow();
		DrawText(); // defaults to black out
		if (win_active) DrawCaret(0,1); 

	} else if (needtodraw&2) { // redraw from dpos to nlines
		DrawCaret(0,0);
		DrawText();
		if (win_active) DrawCaret(0,1);

	} else if (needtodraw&4) { // curs move only 
		DrawCaret(1,1);
	}

	dp->font(app->defaultlaxfont, oldheight);
	dp=NULL;
	
	needtodraw=0;
}

//! Update the caret and if necessary the change in the selection.
/*! Determines what to update based on the earliest and latest highlighted character,
 * and the difference between WhichLine(earliest) and WhichLine(latest). dpos gets
 * the earliest position, and nlines gets the line difference and DrawText(0) is
 * called in this case.
 *
 * If there is no selection and no old selection to redraw, then the caret is merely
 * turned off or on.
 *
 * removeold=1 means cursor movement, draw the caret and selection.
 * removeold==0 assume cursor not there, so that area does not have to be redrawn.
 * 
 * If on is nonzero, then the cursor should be drawn on.
 */
void TextXEditBaseUtf8::DrawCaret(int removeold,int on) // removeold=0,on=1 
{
	if (removeold) { // from Refresh this is only when needtodraw&4
		if (con) { // must remove old caret
			con=0; 
			docaret(0);
		}
			
		if (oldsellen && !sellen) {  // ***dehl old sel
			int t,t2;
			t=dpos=(oldsellen<0?oldcp:selstart);
			t2=(oldsellen<0?selstart:oldcp);
			nlines=WhichLine(t)-WhichLine(t2)+1;
			DrawText(0);
		}
		if (sellen)	{ // rehl around new cp, implies continuous curs move ***should use oldselstart?
		   if (curpos<oldcp) {
				dpos=curpos; nlines=WhichLine(oldcp)-curline+1;
			} else {
				dpos=oldcp; nlines=curline-WhichLine(oldcp)+1;
			}
			DrawText(0);
		}
	}
	
			 // draw cursor on
	if (on && !sellen && cx>=0 && cx<win_w && cy>=0 && cy-textascent+textheight<=win_h) { 
		if (!sellen) docaret(1);
		oldx=cx; oldy=cy;
		con=1;
	} else con=0;

	oldcp=curpos;
	oldsellen=sellen; //***oldselstart??
}

//! Inverts the caret area. DrawCaret is responsible for maintaining con.
/*! w=1, 1 is new caret, 0 is old caret, assumes cx,cy set right
 */
void TextXEditBaseUtf8::docaret(int w)
{ 
	dp->BlendMode(LAXOP_Difference);
	dp->NewFG(~0);
	if (w) dp->drawrectangle(cx,cy-textascent,2,textheight, 1);
	else dp->drawrectangle(oldx,oldy-textascent,2,textheight, 1);
	dp->BlendMode(LAXOP_Over);
}

//! Set curtextcolor and curbkcolor according to whether highlighting is on or not.
void TextXEditBaseUtf8::Colors(int hl)
{
	if (hl) {
		curtextcolor = win_themestyle->fghl.Pixel();
		curbkcolor   = win_themestyle->bghl.Pixel();
	} else {
		curtextcolor = win_themestyle->fg.Pixel();
		curbkcolor = ValidColor(valid);
	}

	dp->NewFG(curtextcolor);
	dp->NewBG(curbkcolor);
}

unsigned long TextXEditBaseUtf8::ValidColor(int which)
{
	if (valid==1) return win_themestyle->bg.Pixel();
	if (valid==0) return bkwrongcolor;
	if (valid==2) return bkwrongcolor2;
	return win_themestyle->bg.Pixel();
}

//! Fill x,y,w,h with curbkcolor.
void TextXEditBaseUtf8::Black(int x,int y,int w,int h)
{
	dp->NewFG(curbkcolor);
	dp->drawrectangle(x,y,w,h, 1);
	dp->NewFG(curtextcolor);
}

//! Draw a single line of text.
/*! y is the baseline of the text.
 *
 * Assumes that x corresponds to pixel 0 of the text that needs drawing.
 * Thus, doesn't inherently do right/center justify, but does map tabs (this conversion
 * is done in TextOut()) to chars if right/center.
 * Handles lines with highlighting, len==1 prints 1 char. Returns updated x.
 *
 * If len==-1, then draw until a newline or '\\0'.
 * If check!=0, then check to see if the selection starts or stops within the line.
 *  
 * \todo  *** automatically blacks out x to eol, is this ok?
 * \todo is clipping really necessary? Sure it leaks when text out of bounds.. only a problem
 *   when textrect is very different from window and nothing else covers up overflow..
 */
int TextXEditBaseUtf8::DrawLineOfText(int x,int y,long pos,long len,char &check,long eof) 
{
	if (eof<0) eof=textlen;

	//DBG cerr <<endl<<"DrawLineOfText in "<<WindowTitle()<<endl;
	//DBG cerr <<" Lot: pos:"<<pos<<"  textlen(or eof):"<<eof<<"  sellen:"<<sellen<<"  selstart:"<<selstart<<"  curpos:"<<curpos<<endl;
	//DBG cerr <<" Draw in rect:"<<textrect.x<<','<<textrect.y<<" "<<textrect.width<<'x'<<textrect.height<<endl;
	 
	 //return if text to draw is out of bounds
	if (x>textrect.x+textrect.width || y-textascent>textrect.y+textrect.height) {
		//XSetClipMask(app->dpy,app->gc(),None);
		return x;
	}

	 //set len to appropriate value: eol or eof if len was -1
	if (len==-1) {
		len=0;
		while (pos+len<eof && !onlf(pos+len)) len++;
		if (pos+len!=eof && !(textstyle&TEXT_NLONLY)) len++; //add 1 for 2-char newlines
		len++; // tacks on the \n or \0 
	}

	long temp=pos+len;
	//DBG cerr <<" len="<<len<<endl;
	long selbegin=0,selend=0;
	char hl;
	if (check && sellen) {
		if (curpos<selstart) { selbegin=curpos; selend=selstart; }
		else { selbegin=selstart; selend=curpos; }
	}
	//DBG cerr <<" Lot: selbegin:"<<selbegin<<"  selend:"<<selend<<endl;
	if (len<1 && x<textrect.x) {
		//XSetClipMask(app->dpy,app->gc(),None);
		return x;
	}

	 //start out assuming not highlighted, blank out the rest of the line
	Colors(0);
	Black(x,y-textascent,textrect.x+textrect.width-x,textheight); //*** bad for multilines? do this from DrawText?
	
	 // to draw: [pos,temp);
	if (check) {
		if (pos>=selend) {
			hl=0; check=0;
			Colors(0);
			x=TextOut(x,y,thetext+pos,temp-pos,eof);
		} else {
			if (pos>=selbegin) { Colors(1); hl=1; }
			else { hl=0; Colors(0); }

			 // 1st non-highlighted segment
			if (!hl && selbegin<temp) {
				//DBG cerr <<"1st no hi--";
				x=TextOut(x,y,thetext+pos,selbegin-pos,eof);
				Colors(1);
				hl=1;
				pos=selbegin;
			}
			 // print highlighted segment
			if (hl) {
				//DBG cerr <<"2nd hi--pos="<<pos<<"  ";
				if (selend<temp) {
					x=TextOut(x,y,thetext+pos,selend-pos,eof);
					pos=selend;
					Colors(0);
					hl=0;
					check=0;
				} else {
					x=TextOut(x,y,thetext+pos,temp-pos,eof);
				}
			}
			 // print final non-highlighted segment
			if (!hl) {
				//DBG cerr <<"3rd no hi--pos="<<pos<<"  ";
				x=TextOut(x,y,thetext+pos,temp-pos,eof);
			}
		}
	} else {
		 // assume whole line is not highlighted 
		//DBG cerr <<"no hi--pos="<<pos;
		x=TextOut(x,y,thetext+pos,len,eof);
	}
	//DBG cerr <<endl;
	
	//XSetClipMask(app->dpy,app->gc(),None);
	return x;
}

//! Print out len bytes of str, which is a pointer to somewhere inside thetext.
/*! Handles tabs, treats '\\n' as normal char, except when it is at position len. Returns updated x.
 *  y is baseline. Handles any number of tabs until a newline or len, checks for initial tab, does one segment,
 *  and if more tabs found, calls itself with rest of string.
 *   	
 *   Does not handle highlighting. Highlightind is accounted for in DrawLineOfText().
 *   
 *  If eof<0, then use textlen-(str-thetext) for eof. str points to somewhere inside thetext. Do not use
 *  an arbitrary string. eof is a length, with eof==0 corresponding to *str.
 *
 *   IMPORTANT: this depends on thetext and textlen to do bounds checking while working out tab placement, 
 *   	do not call with arbitrary strings, else will seg fault on tab placement sometimes.
 *   	If you must, you can temporarily redefine thetext, textlen, curpos, and curlineoffset.
 *   	Only other thing it calls is onlf(), which uses curpos if the pos is out of bounds.
 *
 * \todo break off the actual string output to laxutils::textout()?
 */
int TextXEditBaseUtf8::TextOut(int x,int y,char *str,long len,long eof) // len=1 prints 1 char,
{
	if (len<1 || x>textrect.x+textrect.width) return 0; // DrawLineOfText checks for y bounds
	if (eof<0) eof=textlen-(str-thetext);
	
	//DBG cerr <<"  GText: x,y:"<<x<<","<<y<<" str:";
	//DBG for (int c=0; c<len; c++) cerr <<str[c];
	//DBG cerr<<" len:"<<len<<"  eof:"<<eof;

	dp->LineWidth(1);
	
	long p;
	int c2;
	char *blah=NULL;
	long c=0,newstart=0,newlen=0;
	int pix;
	char tabfound=0,tabatfirst=0,nlf=0;
	 
	 // scan for initial tab, and next tab
	 // note that a tab character cannot be part of a multibyte utf8 character, so
	 //  the byte by byte searching here is ok..
	if ((textstyle&TEXT_TABS_STOPS) && !(textstyle&(TEXT_RIGHT|TEXT_CENTER))) {
		 // if tab found at first point in str, make a note of it
		if (str[0]=='\t') { 
			str++;
			tabatfirst=1;
			len--; 
			eof--;
		}
	
		 // redefine len, etc so that tabfound==1 means there are tabs remaining in str,
		 // len=length from str to the next tab (a tab segment) that gets printed here
		 // newstart=points to position in str of the next tab
		 // newlen=new len for rest of str, from newstart to the the previous len
		for (c2=0; c2<len; c2++)
			if (str[c2]=='\t') {
				newstart=c2;
				newlen=len-newstart;
				len=c2;
				tabfound=1;
				break; 
			}
	}
	
	 // scan for newline at position str+len-1==(end of segment to print)
	nlf=onlf(str-thetext+len-1); 

	 //Find the actual text to print out:
	 // pix is to len, which is only till the end of the text segment, ending at a tab, nl, or eof
	 // blah is set here to the actual utf8 string that must be printed, with char substitutions
	 //  for dead chars, and extraneous newlines and such
	 // p is the length of blah
	pix=ExtentAndStr(str,len-nlf,blah,p);
	if (nlf) {
		nlf=1;
		blah[p++]=' '; //add a space for a final newline
		blah[p]='\0'; 
	}
	
	if (tabatfirst==1) { // there is a tab at first pos, so draw triangle then rest of text
		int tabtype;
		int tabchar=0;
		char tabutf[5]; //temp buffer for utf8 char from ucs

		c=GetNextTab(x+curlineoffset,tabtype); // make c==tabbedto point
		if (tabtype==CHAR_TAB) {
			tabchar=GetTabChar(c);
			if (tabchar=='\0') tabtype=CENTER_TAB; 
			tabutf[utf8encode(tabchar,tabutf)]='\0';
		}

		 // blackout from current to the tab stop
		c-=curlineoffset; // make c a screen pos
		Black(x,y-textascent, c-x,textheight);

		int chunkpix=-1;
		
		 //For char tabs, find extent until the char
		if (tabtype==CHAR_TAB) {
			char ch=str[len];
			str[len]='\0';//we don't want to waste time scanning a huge str!
			char *charpos=strstr(str,tabutf);
			str[len]=ch;
			if (charpos) {
				chunkpix=GetExtent(str-thetext,charpos-thetext,0,eof+(str-thetext));
				//DBG cerr <<endl;
			} //else find full extent below
		} 
		if (chunkpix==-1) {
			chunkpix=pix;
		}
		
		 // update c to point to start of current segment after positioning according to tab
		if (tabtype==RIGHT_TAB)       { if (c-chunkpix>x) c-=chunkpix; else c=x; }
		else if (tabtype==CENTER_TAB) { if (c-chunkpix/2>x) c-=chunkpix/2; else c=x; }
		else if (tabtype==CHAR_TAB)   { if (c-chunkpix>x) c-=chunkpix; else c=x; }
		//else LEFT_TAB, nothing needs to be changed
		
		if (textstyle&TEXT_SHOW_WHITESPACE && x<textrect.x+textrect.width) {
			// draw triangle in tab space
			dp->NewFG(wscolor);
			dp->drawline(x,y, c,y-textascent+textheight/3);
			dp->drawline(c,y-textascent+textheight/3, c,y+textdescent-textheight/3);
			dp->drawline(c,y+textdescent-textheight/3, x,y);
			dp->NewFG(curtextcolor);
		}
		x=c; //now x points to where the text actually gets printed
	}

	c=x;

	 // At this point, c is assumed to point to the pixel start of current segment, after it has 
	 // been put into position according to tabtype
	//DBG cerr <<"  --p="<<p;
	if (p || nlf || len>=eof) { // if there are chars to draw or newlen
		if (curbkcolor == win_themestyle->bghl.Pixel()) Black(x,y-textascent,pix,textheight);

		//int r,g,b;
		//colorrgb(curtextcolor,&r,&g,&b);
		dp->NewFG(curtextcolor);
		dp->textout(x,y, blah,p, LAX_LEFT|LAX_BASELINE);

		if (textstyle&TEXT_SHOW_WHITESPACE) { // draw ticks for spaces
			 // different color for ws
			dp->NewFG(wscolor);
			int spwidth = charwidth(' ');

			for (c2=0; c2<p-nlf; c2++) {
				if (c>textrect.x+textrect.width) break;
				if (blah[c2]==' ') {
					dp->drawline(c+spwidth/2,y, c+spwidth/2,y-2);
				}
				c+=charwidth(blah[c2]);
			}
			if (nlf  && c<win_w) // draw slash for eol
				dp->drawline(c,y-(int)(textascent*.6), c+charwidth(' '),y);
			else if (str[len]=='\0')  // draw divet for eof
				dp->drawrectangle(c+1,y-textascent/2, 5,textascent/2, 0);
			dp->NewFG(curtextcolor);
		}
	}

	//DBG cerr <<" --print:"<<'"'<<blah<<'"'<<endl;
	delete[] blah;
	x+=pix;

	if (tabfound) x=TextOut(x,y,str+newstart,newlen,eof-newstart);
	return x;
}

//! Find a printable string, and its pixel extent.
/*! Assumes no tabs. str points to somewhere in thetext. That segment of thetext gets converted
 * into a string that is to be printed, and blah is made to point to that. Adds a pad of 6 bytes to blah, 
 * 
 * Returns pixel extent, p is len of blah.
 *
 * The previos contents of blah, if any, are ignored. They are not delete[]'d if blah!=NULL.
 */
int TextXEditBaseUtf8::ExtentAndStr(char *str,long len,char *&blah,long &p)
{
	 //*** some other way to check for ridiculously huge single lines? just let them be?
	if (len>2000) { len=2000; }
	 
	 //find the number of non printing chars (the ones that have 0 width)
	long c;
	int b,cb;
	char cntlutf[10];
	cb=utf8encode(cntlchar,cntlutf);
	cntlutf[cb]='\0';
	for (c=0,p=0; p<len && (str-thetext)+p<textlen; p=nextpos(p+(str-thetext))-(str-thetext)) 
		if (charwidth(utf8decode(str+p,str+len,&b),1)==0) c++; //*******
	p=(cb+4)*c+len+7;
	blah=new char[(int)p];
	p=0;
	int ucs;
	for (c=0; c<len; c=nextpos(c+(str-thetext))-(str-thetext)) {
		if (charwidth(utf8decode(str+c,str+len,&b),1)==0) {
			if (textstyle&TEXT_CNTL_BANG) {
				 //write out single character for missing chars
				memcpy(blah+p,cntlutf,cb*sizeof(char));
				p+=cb;
			} else if (textstyle&TEXT_CNTL_HEX) {
				 //write cntl+hex for missing chars
				ucs=utf8decode(str+c,str+len,&b);
				p+=b;
				b=sprintf(blah+p,"%s%x",cntlutf,ucs)-b;
				while (b) {
					b--;
				}
			}
		} else { 
			memcpy(blah+p,str+c,b*sizeof(char));
			p+=b;
		}
	}
	int pix;
	double ww;
	getextent(thefont, blah,p, &ww,NULL, NULL,NULL, 0);
	pix=ww;

	//DBG cerr <<"  e&s:="<<p<<" strlen="<<strlen(blah);
	blah[p]='\0';
	return pix;
}

//! Find the extent of the string in range [pos,end). Must be no newlines within the range.
/*! This can be used to find where the caret should be. The function deals with tabs.
 * Assumes lsofar starts at an accurate place. It must be the horizontal pixel location 
 * corresponding to pos. Also assumes curlineoffset set appropriately.
 *
 * As always, pos and end point to the first byte of a multibyte character, or the first of a 
 * 2-character newline.
 *   
 * Returns the new lsofar.
 *
 * \todo *** must fix the not taking into account missing chars..
 */
int TextXEditBaseUtf8::GetExtent(long pos,long end,int lsofar,long eof) //lsofar=0, eof==-1
{
	if (end<=pos) return 0;
	if (eof<0) eof=textlen;

	 //for right or center justified, or no tabs, return normal extents
	double ww;
	if ((textstyle&(TEXT_RIGHT|TEXT_CENTER)) || !(textstyle&TEXT_TABS_STOPS)) {
		getextent(thefont, thetext+pos,end-pos, &ww,NULL, NULL,NULL, 0);
		lsofar+=ww;
		 //*** note that this is wrong: it does not take into account mapping of missing chars
		 //    and probably not tabs
		return lsofar;
	}

	int ppos=pos, // temporary position pointer
		slen=0,   // Segment LENgth in pixels
		pslen=0,  // Partial Segment LENgth in pixels
		tlen=0;   // Tabchar LENgth in pixels, length from segment start to the beginning of tabchar
	long eot=end; // the byte position right after end of chars in the current tab segment
	int tabbedto=lsofar,tabtype=LEFT_TAB;
	int tabchar=0;
	char tabutf[6];
	
	 // put eot on next eol, eof, or at first tab after end
	while (eot<eof && thetext[eot]!='\t' && !onlf(eot)) eot++;
	
	//DBG cerr <<"pos<=eot: ";
	while (pos<=eot) { 
		//DBG cerr <<".";
		 //find the char extent of the current tab segment,
		 // makes ppos at end of current tab segment
		while (ppos<eot && thetext[ppos]!='\t' && !onlf(ppos)) ppos++;
		getextent(thefont, thetext+pos,ppos-pos, &ww,NULL, NULL,NULL, 0);
		slen=ww;

		if (ppos>=end) {
			getextent(thefont, thetext+pos,end-pos, &ww,NULL, NULL,NULL, 0);
			pslen=ww;
		}
		
		 //find out the length in the segment until the char for a char_tab
		if (tabtype==CHAR_TAB && tabchar) {
			tabutf[utf8encode(tabchar,tabutf)]='\0';

			char ch=thetext[ppos];
			thetext[ppos]='\0';//we don't want to waste time scanning a huge str!
			char *charpos=strstr(thetext+pos,tabutf);
			thetext[ppos]=ch;

			if (charpos) {
				getextent(thetext+pos,charpos-thetext, &ww,NULL, NULL,NULL, 0);
				tlen=ww;
			} else {
				tabtype=RIGHT_TAB;
				tlen=0;
			}
		}

		if (slen==0 || tabtype==LEFT_TAB) {
			if (ppos>=end) return tabbedto+pslen; //return partial length
			lsofar=tabbedto+slen; //add on segment length and continue

		} else if (tabtype==RIGHT_TAB) {
			if (tabbedto-slen<lsofar) 
				if (ppos>=end) return lsofar+pslen; 
				else lsofar+=slen;
			else if (ppos>=end) return tabbedto-slen+pslen;
			else lsofar=tabbedto;

		} else if (tabtype==CENTER_TAB) {
			if (tabbedto-slen/2<lsofar)
				if (ppos>=end) return lsofar+pslen; else lsofar+=slen;
			else if (ppos>=end) return tabbedto-slen/2+pslen; else lsofar=tabbedto+slen/2;

		} else if (tabtype==CHAR_TAB) {
			if (tabbedto-tlen<lsofar) 
				if (ppos>=end) return lsofar+pslen; else lsofar+=slen;
			else if (ppos>=end) return tabbedto-tlen+pslen; else lsofar=tabbedto-tlen+slen;
		}

		ppos++;
		pos=ppos;

		 // then get info for next tab
		if (thetext[pos]=='\t') {
			tabbedto=GetNextTab(lsofar,tabtype);
			if (tabtype==CHAR_TAB) {
				tabchar=GetTabChar(tabbedto);
				tabutf[utf8encode(tabchar,tabutf)]='\0';
			}
		}
	} //while (pos<=eot)

	//DBG cerr <<"getextent error-- shouldn't be here"<<endl;
	return lsofar; // note: this should never be reached
}
	
//! Returns maximum pos less than or equal to an arbitrary window pixel position.
/*! This is basically used to find a position underneath the mouse.
 * Assumes lsofar set to something reasonable for pos, as if it was on a lefttab.
 * Handles tabs.
 *
 * Scan from pos to an end of line, or to eof. If eof<0, then use eof=textlen.
 *
 * \todo mishandles missing chars
 */
long TextXEditBaseUtf8::GetPos(long pos,int pix,int lsofar,long eof) //lsofar=0, eof=-1
{
	DBG cerr <<endl;
	long end=pos;
	if (eof<0) eof=textlen;
	while (end<eof && !onlf(end)) end=nextpos(end); // puts end on newline-1
	DBG cerr <<"textx-GetPos: pos="<<pos<<" end="<<end<<" pix="<<pix<<endl;

	double ww,hh;
	if ((textstyle&(TEXT_RIGHT|TEXT_CENTER)) || !(textstyle&TEXT_TABS_STOPS)) {
		int tlsofar;
		int lastpix=lsofar, mid;
		long pos2=pos;
		long pos3;

		while (pos2<end) { 
			 //*** note that this is wrong: it does not take into account mapping of missing chars
			pos3=nextpos(pos2);
			getextent(thefont, thetext+pos,pos3-pos, &ww,&hh, NULL,NULL, 0);
			tlsofar=lsofar+ww;
			mid=(tlsofar+lastpix)/2;
			DBG cerr <<pix<<"  "<<lsofar<<"  "<<"  mid:"<<mid<<"  "<<tlsofar<<endl;
			if (mid>pix) break;
			if (tlsofar>pix) { pos2=nextpos(pos2); break; }
			lastpix=tlsofar;
			pos2=nextpos(pos2);
		}
		DBG cerr <<"GetPos: "<<pos2;
		return pos2;
	}
 
	 // get extent for next segment, determine tabtype, scan for lsofar in it.
	int tabbedto=lsofar,tabtype=LEFT_TAB;
	int tabchar='\0';
	char tabutf[6];
	int pos2,last,mid,lsofar2;
	int eotabseg, //char pos right after the end of the current tab segment
		seg,      //pixel length of the current tab segment
		topos;    //char pos to center a tab around, or the end of the tab segment

	while (pos<end) {
		 // pos is assumed to be placed after the previous tab
		eotabseg=pos; // eotabseg is set to point at next tab/eol/eof
		seg=0; 		// seg is pixel length of current tab segment
		if (tabtype==CHAR_TAB) {
			tabchar=GetTabChar(tabbedto); 
			if (tabchar=='\0') tabtype=CENTER_TAB; 
		}

		while (eotabseg<eof && !onlf(eotabseg) && thetext[eotabseg]!='\t') eotabseg++;

		if (tabtype==CHAR_TAB) {
			char ch=thetext[eotabseg];
			thetext[eotabseg]='\0';//we don't want to waste time scanning a huge str!
			tabutf[utf8encode(tabchar,tabutf)]='\0';
			char *charpos=strstr(thetext+pos,tabutf);
			thetext[eotabseg]=ch;
			if (charpos) topos=charpos-thetext;
				else topos=eotabseg;
		} else topos=eotabseg;

		getextent(thefont, thetext+pos,topos-pos, &ww,&hh, NULL,NULL, 0);
		seg=ww;

		switch (tabtype) {
			case LEFT_TAB:
				lsofar=tabbedto;
				break;
			case RIGHT_TAB:
				if (tabbedto-seg>lsofar) lsofar=tabbedto-seg;
				break;
			case CENTER_TAB:
				if (tabbedto-seg/2>lsofar) lsofar=tabbedto-seg/2;
				break;
			case CHAR_TAB: // really should be able to make right do double time, just with a partial seg
				if (tabbedto-seg>lsofar) lsofar=tabbedto-seg; 
				break;
		}
		DBG cerr <<"-pos="<<pos<<"  lsofar="<<lsofar<<"  eotabseg="<<eotabseg<<"  seg="<<seg<<endl;
		if (pix<lsofar) return pos-1; // pix is in the tab previous to seg
		pos2=pos;
		last=lsofar;

		long pos3;
		while (pos2<eotabseg) {
			 //*** probably not quite right:
			pos3=nextpos(pos2);
			getextent(thefont, thetext+pos,pos3-pos, &ww,&hh, NULL,NULL, 0);
			lsofar2=lsofar+ww;
			mid=(last+lsofar2)/2;

			if (pix<mid) return pos2;
			if (pix<lsofar2) return nextpos(pos2);
			last=lsofar2;
			pos2=nextpos(pos2);
		}
		lsofar=lsofar;
		pos=pos2;
		if (thetext[pos]=='\t') { tabbedto=GetNextTab(lsofar,tabtype); pos++; }
	}
	return pos;
}

/*! Dec counts old font and Increments newfont's count, unless it is the same as thefont.
 * Sets firsttime=1, which will trigger a SetupMetrics() at the beginning
 * of the next Refresh().
 */
int TextXEditBaseUtf8::UseThisFont(LaxFont *newfont)
{
	if (!newfont) return 1;
	if (newfont!=thefont) {
		if (thefont) thefont->dec_count();
		thefont=newfont;
		thefont->inc_count();
	}
	firsttime=1; //triggers SetupMetrics() in next Refresh
	needtodraw=1;
	return 0;
}

/*! Just return a link to the current font. Count is not incremented.
 */
LaxFont *TextXEditBaseUtf8::GetFont()
{
	return thefont;
}

/*! Sets thefont=app->defaultlaxfont, and assigns textheight, textascent, and textdescent accordingly.
 * Increments the count of app->defaultlaxfont.
 */
int TextXEditBaseUtf8::SetupMetrics()
{
	if (thefont==NULL) {
		thefont=app->defaultlaxfont;
		thefont->inc_count();
	}
	//textheight=thefont->textheight();
	//textascent=thefont->ascent();
	//textdescent=thefont->descent();
	textheight =thefont->textheight();
	textascent =thefont->ascent();
	textdescent=thefont->descent();
	
	return 0;
}

//! This updates textrect based on new window dimensions.
/*! Normally called from MoveResize() or Resize(). 
 * Subclasses would want to remap cx, cy, and curlineoffset when 
 * resizing the window makes the textrect origin change.
 */
void TextXEditBaseUtf8::settextrect()
{
	textrect.x=0;
	textrect.y=0;
	textrect.width=win_w;
	textrect.height=win_h;
}

/*! Default is to set textrect equal to the new window bounds.
 */
int TextXEditBaseUtf8::MoveResize(int nx,int ny,int nw,int nh)
{
	int c=anXWindow::MoveResize(nx,ny,nw,nh);
	settextrect();
	return c;
}

/*! Default is to set textrect equal to the new window bounds.
 */
int TextXEditBaseUtf8::Resize(int nw,int nh)
{
	int c=anXWindow::Resize(nw,nh);
	settextrect();
	return c;
}


} // namespace Laxkit

