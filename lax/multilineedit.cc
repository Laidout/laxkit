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
//    Copyright (C) 2004-2007,2010,2015 by Tom Lechner
//


#include <lax/multilineedit.h>
#include <lax/utf8utils.h>
#include <lax/mouseshapes.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>

#include <iostream>
using namespace std;
#define DBG 


using namespace LaxFiles;

namespace Laxkit {



//----------------------------------------------- MultiLineEdit ----------------------------------------
/*! \class MultiLineEdit
 * \brief *** please note this class is currently seriously broken *** Multiline edit with optional word wrap.
 * 
 * Optional scrollers, handles l/r/c/char tabs, simple l/r/c justification.
 *
 * 
 * <pre>
 * NOTES:
 * *** means not yet implemented..
 *  Usual char codes:
 *  ESC         get out of any control modes, deselect selection ***
 *  ^l          put in/out select line mode ***
 *  ^left,^right  jump word left or right
 *  ^up,^down   jump to first/last line on screen
 *  ^a          select all
 *  ^c/C, ^ins  copy
 *  ^x/X, +del  cut
 *  ^v/V  +ins  paste
 *  ^w          toggle word wrap
 *  ^+w         toggle show whitespace
 *  ^.          increase tab width
 *  ^,          decrease tab width
 *  ^t ???      set l/r/c/n tab *** slide around a guide??? ***
 *              rm tab***
 *  ^\          toggle show hex chars
 *  
 *  lbdbl       select word***
 *  lbtrp       select line***
 *  rb-drag     ShiftScreen***
 *  wheel       move y
 *  ^wheel      move page y
 *  +wheel      move x
 *  ^+wheel     move page x
 *  </pre>
 *  
 * \todo *** lots of stuff, particularly adding scrollers, see class description....
 * \code
 * #define GOODEDITWW_FILL_LR_BOX   <-- make x,y scrollers cover up the little box in lower right
 * #define GOODEDITWW_ABS_SCROLLERS <-- use some definite rectangle, and don't unmap??? huh?
 * #define GOODEDITWW_Y_IS_LINES    <-- if scrollers show line span or character span
 * #define GOODEDITWW_Y_IS_CHARS
 *
 * 
 * <pre> 
 *  TODO
 *  *** smart refreshing when moving screen XMoveArea.. make sure little windows on top dealt with ok?? how??
 *  *** make a window for tab position editing??
 *  
 *  *** might be useful to have a charwidth(long pos) for strange fonts??
 *  *** center and right justified
 *  
 *  *** wheel up doing weird
 *  *** not blacking out when add lines at eof
 *  *** not drawing final box for eof if eof on its own line
 *  *** deleting lines no good
 *  *** makeinwindow? not dealing with right edge after textrect change
 *  *** select, then focus out, then click on, and old selection is not removed
 *  *** end on wrapped lines now not going to end (this is actually a feature!! though should make cursor at fake-eol
 *  		equivalent to cursor at beg of next line for wrapped lines
 *  *** wwinspect doesn't seem to be working well, inschars allowed to go beyond maxpixlen
 *
 *  FOR FANCY
 *  put this in a derived class?:
 *  ^t then arrows to position tab, l/r/c/n selects type, ^arrows selects current tabs, ESC escapes
 * </pre>
 *    
 * \endcode
 */
/*! \var int MultiLineEdit::lpers
 * \brief The number of lines per screen.
 *
 * Line 0 is the first line of the screen and line lpers-1 is the last full line
 * on the screen. Typically lines do not fit exactly on the screen, but it is useful to
 * have a couple extra. So line lpers is usually the half on screen line, and line lpers+1
 * is the next line after that. The linestats array will have lpers+2 elements.
 * 
 * This is set only in SetupScreen(), newxsize() and newysize().
 */
/*! \var MultiLineEdit::Linestat *MultiLineEdit::linestats
 * \brief Holds the character start position, pixel length, and pixel indentation of a screen line.
 */
//-------------- MultiLineEdit::Linestat

/*! \struct MultiLineEdit::Linestat
 * \brief Holds information about the screen lines.
 *
 * Has how many pixels to indent, the starting character index into thetext, and the length of the line.
 * This stuff helps speed up refreshing. MultiLineEdit::linestats holds an array of these things,
 * to make movement easier.
 */


//TextXEditBaseUtf8::TextXEditBaseUtf8(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
//                             int xx,int yy,int ww,int hh,int brder,anXWindow *prev,
//                             const char *newtext,unsigned long ntstyle,char ncntlchar) // newtext=NULL, ntstyle=0 ncntlchar=0

//! Constructor.
MultiLineEdit::MultiLineEdit(anXWindow *prnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder,
					anXWindow *prev,unsigned long nowner,const char *nsend,
					unsigned int ntstyle,const char *newtext)
				: TextXEditBaseUtf8(prnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend,newtext,ntstyle,'\\')
{ 			// nstyle=0, newtext=NULL, cntlchar=0; newtext copied
	modified=0;

	installColors(app->color_edits);
	wscolor=rgbcolor(0,64,255);
	con=cx=cy=cdir=0;
	oldsellen=0;
	scrollwidth=15;

	padx=pady=0;
	textheight=0;
	firsttime=1;
	needtodraw=1;
	tabwidth=30;
	blanktext=NULL;
			
	linestats=NULL;
	numlines=0;

//	mostcharswide=Getcharswide();
//	numlines=GetNumLines();
	curlineoffset=-(padx + textrect.x);
	curline=0;
	dpos=0; nlines=0;
	mostpixwide=mostcharswide*5;
	if (textstyle&TEXT_WORDWRAP) maxpixwide=textrect.width-2*padx;
		else maxpixwide=30000; 

	xscroller=yscroller=NULL;
	xscrollislocal=yscrollislocal=1;

	win_pointer_shape=LAX_MOUSE_Text;
}

//! Destructor, delete linestats.
MultiLineEdit::~MultiLineEdit()
{
	DBG cerr <<"----multilineedit dest"<<endl;

	if (blanktext) delete[] blanktext;
	if (linestats) delete[] linestats;
	
//***	if (xscroller && xscrollislocal) app->destroywindow(xscroller);// is this strictly necessary? done in contructor anyway
//		//***if (!scrollislocal) should remove this from being parent of scroller!
//	if (yscroller && yscrollislocal) app->destroywindow(yscroller);

	DBG cerr <<"--------multilineedit dest finished"<<endl;
}

//	Scroller(anXWindow *parnt,const char *ntitle, unsigned long nstyle,
//			int nx,int ny,int nw,int nh,unsigned int nbrder,
//			anXWindow *prev,Window nowner,const char *mes,
//			long nmins,long nmaxs,long nes,long nps,long ncp=-1,long ncpe=-1); // ncp=ncpe=-1

//! Use these external scrollers, rather than dynamically add and remove them.
/*! \todo *** this needs work..
 */
void MultiLineEdit::UseTheseScrollers(Scroller *xscroll,Scroller *yscroll)
{
	if (xscroll) {
		if (xscroller) {
			if (xscrollislocal) {
				if (xscroller->win_on) textrect.height+=scrollwidth;
				firsttime=1;
				//SetupScreen();
				app->destroywindow(xscroller);
			}
		}
		xscrollislocal=0;
		xscroller=xscroll;
		newxssize();
	}
	if (yscroll) {
		if (yscroller) {
			if (yscrollislocal) {
				if (yscroller->win_on) textrect.width+=scrollwidth;
				firsttime=1;
				//SetupScreen();
				app->destroywindow(yscroller);
			}
		}
		yscrollislocal=0;
		yscroller=yscroll;
		newyssize();
	}
}

void MultiLineEdit::SetStyle(unsigned long style, int on)
{
	if (style&TEXT_LINE_NUMBERS) {
		if (on) textstyle|=TEXT_LINE_NUMBERS;
		else textstyle&=~TEXT_LINE_NUMBERS;
	}
}

//	Scroller(anXWindow *parnt,const char *ntitle, unsigned long nstyle,
//			int nx,int ny,int nw,int nh,unsigned int nbrder,
//			anXWindow *prev,Window nowner,const char *mes,
//			long nmins,long nmaxs,long nes,long nps,long ncp=-1,long ncpe=-1); // ncp=ncpe=-1

/*! If multilineedit uses outside supplied scrollers, the must be passed into this
 *  before init is called. Otherwise new scrollers are created here.
 *
 *  Init sets an initial textrect size, and sets curlineoffset to be at padx.
 */
int MultiLineEdit::init()
{
	DBG cerr <<"multilineedit init.."<<endl;
//	if (textstyle&TEXT_XSCROLL && !xscroller && !(textstyle&TEXT_WORDWRAP)) {
//		xscroller=new Scroller(this,"ww-xscroller",SC_ABOTTOM|SC_XSCROLL, 
//				0,win_h-scrollwidth, win_w,scrollwidth, 0, 
//				NULL, window, "xscroller", 
//				NULL,
//				0,mostpixwide,1,win_w*3/4,0);
//		xscrollislocal=1;
//		app->addwindow(xscroller,0);
//	}
//	if (textstyle&TEXT_YSCROLL && !yscroller) {
//		yscroller=new Scroller(this,"ww-yscroller",SC_ABOTTOM|SC_YSCROLL, 
//				win_w-scrollwidth,0, scrollwidth,win_h, 0, 
//				NULL, window, "yscroller", 
//				NULL,
//				0,textlen,1,100,0);
//		yscrollislocal=1;
//		app->addwindow(yscroller,0);
//	}
	settextrect();
	curlineoffset=-(padx + textrect.x);
	DBG cerr <<"multilineedit init done."<<endl;
	return 0;
}

//! Find occurence of str, starting from position curpos if fromcurs==1.
int MultiLineEdit::Find(char *str,int fromcurs) // fromcurs=1 
{
	if (!TextEditBaseUtf8::Find(str,fromcurs)) return 0;
			
	findcaret();
	needtodraw|=(makeinwindow()?1:4);
	return 1;
}

/*! \todo *** this, like the others in base classes are wholly inadequate
 */
int MultiLineEdit::Replace(const char *oldstr,const char *newstr,int all)
{ //***			 // all=0: 0=only next, 1=all, -1=all in selection 
	if (TextEditBaseUtf8::Replace(oldstr,newstr,all)) return 1;
	findcaret();
	needtodraw|=(makeinwindow()?1:4);
	return 0;
}

//! Set the text, and remap the screen.
int MultiLineEdit::SetText(const char *newtext)
{
	TextEditBaseUtf8::SetText(newtext);

	DBG cerr <<"MultiLineEdit newtext:"<<endl<<thetext<<endl;
	numlines=GetNumLines();
	mostcharswide=Getcharswide();

	if (!firsttime) {
		makelinestart(0,0,1,1);
		Getmostwide();
		findcaret();
		newxssize();
		newyssize();
	}

	Modified(0);
	return 0;
}

//! Jump to this new position.
long MultiLineEdit::SetCurpos(long newcurpos)
{
	TextEditBaseUtf8::SetCurpos(newcurpos);
	needtodraw=1;

	if (!firsttime) {
		findcaret();
		needtodraw|=(makeinwindow()?1:4);
	}
	return curpos;
}

//! Set the selection to these bounds.
int MultiLineEdit::SetSelection(long newss,long newse)
{
	TextEditBaseUtf8::SetSelection(newss,newse);
	findcaret();
	needtodraw|=(makeinwindow()?1:4);
	return 0;
}

/*! Responds to 'xscroller' and 'yscroller'.
 *
 * \todo *** not sure if this is ok since scrollers use the panner now... 
 */
int MultiLineEdit::Event(const EventData *e,const char *mes)
{
	if (!strcmp(mes,"yscroller")) {
		const SimpleMessage *ee=dynamic_cast<const SimpleMessage*>(e);

		if (ee->info1==1 || ee->info1==-1 || !(textstyle&GOODEDITWW_Y_IS_CHARS)) ShiftScreen(0,ee->info1);
		else { // superduper Y_IS_CHARS
			if (ee->info1<0) makelinestart(0,findlinestart(yscroller->GetCurPos()),1,0);
			else makelinestart(lpers-2,findlinestart(yscroller->GetCurPosEnd()),0,0);
			newyssize();
			newxssize();
		}
		return 0;

	} else if (!strcmp(mes,"xscroller")) {
		const SimpleMessage *ee=dynamic_cast<const SimpleMessage*>(e);
		ShiftScreen(0,ee->info1);
		return 0;
	}
	return anXWindow::Event(e,mes);
}

//	virtual void SetSize(long nms,long nstart,long nend,long nps,long nes); nps=-1 means newpagesize=nend-nstart
//	virtual long SetSize(long nms,long nps=0,long nes=0);


/*! NOTE: the scrollers are always flush with right and bottom, and newyssize,newxssize modify textrect
 */
int MultiLineEdit::newyssize()
{//***
	return 0;
//	if (!(textstyle&TEXT_YSCROLL)) return 0;
//	if (linestats[0].start>0 || linestats[lpers].start<textlen || textstyle&TEXT_ALWAYS_Y) { //turn on yscroller or newdata 
//		if (yscroller && yscroller->win_on) { // scroller exists already
//			yscroller->SetSize(0,textlen, linestats[0].start,linestats[lpers].start, -1,-1);
//		} else { // scroller doesn't exist or isn't on
//			 // must resize textwindow, position scroller(s), makelinestart
//			if (!yscroller) { //*** when would this happen?? if not done in init, just never do it?? no so apps can have/not have
//				yscroller=new Scroller(this,"ww-yscroller",SC_ABOTTOM|SC_YSCROLL, 
//						win_w-scrollwidth,0, scrollwidth,win_h, 0, 
//						NULL, window, "yscroller", 
//						NULL,0,textlen,1,-1,linestats[0].start,linestats[lpers].start);
//				app->addwindow(yscroller,0);
//				yscrollislocal=1;
//			}
//			
//			 // now scroller exists, but is not on, so place it, resize text, but wait till resetting before mapping
//			if (yscrollislocal) { // turn on/resize text and scroller only if set for local sizing
//				textrect.width-=scrollwidth; // resizing the textwindow
//				
//				yscroller->Resize(scrollwidth,win_h); 
//				if (textstyle&TEXT_WORDWRAP) {
//					maxpixwide=textrect.width-2*padx;
//					if (mostpixwide>maxpixwide) mostpixwide=maxpixwide; // newxssize needs mostpixwide set
//				}
//				
//				 // resize/setsize xscroller if necessary
//				if (xscroller && xscrollislocal && xscroller->win_on) {
//					xscroller->Resize(win_w-scrollwidth,scrollwidth);
//					xscroller->SetSize(mostpixwide+5,textrect.width-2*padx);
//					makelinestart(0,-1,1,0);
//				} else if (!newxssize(0)) makelinestart(0,-1,1,0);
//				
//			}
//
//
//			 // finally make y settings current
//			yscroller->SetSize(0,textlen, linestats[0].start,linestats[lpers].start, -1,0);
//			if (!yscroller->win_on && yscrollislocal) app->mapwindow(yscroller);
//			
//		}
//	} else { // turn off scroller because all text is onscreen
//		if (!yscroller || !yscroller->win_on) return 0; // is already off
//		if (textstyle&TEXT_ALWAYS_Y || !yscrollislocal) {
//			yscroller->SetSize(0,textlen, 0,textlen, -1,0);
//		} else if (yscroller && yscroller->win_on)  {  // turn off yscroller 
//			 // to be here, must be local
//			app->unmapwindow(yscroller);
//			textrect.width+=scrollwidth;
//			if (textstyle&TEXT_WORDWRAP) maxpixwide=textrect.width-2*padx;
//			makelinestart(0,0,1,0);
//			if (xscroller && xscroller->win_on && xscrollislocal) {
//				xscroller->Resize(win_w,scrollwidth);
//				xscroller->SetSize(mostpixwide+2*padx,textrect.width-2*padx);
//			}
//		} // else islocal, and is already off
//	}
//	return 1;
}

//	Scroller(anXWindow *parnt,const char *ntitle, unsigned long nstyle,
//			int nx,int ny,int nw,int nh,unsigned int nbrder,
//			anXWindow *prev,Window nowner,const char *mes,
//			long nms,long nes,long nps,long ncp=-1,long ncpe=-1); // ncp=ncpe=-1
//! Make sure the scrollers are correct for current line widths.
int MultiLineEdit::newxssize(int p)//p=1, p is to prevent race with newyssize
{ //***
	return 0;
//	if (!(textstyle&(TEXT_XSCROLL|TEXT_WORDWRAP))) return 0;
//	if (mostpixwide>textrect.width-2*padx || textstyle&TEXT_ALWAYS_X || !xscrollislocal) { // turn on xscroller or new data 
//		if (xscroller && xscroller->win_on) { 
//			xscroller->SetSize(mostpixwide,textrect.width-2*padx);
//		} else { // xscroller doesn't exist or isn't on
//			if (!xscroller) {
//				xscroller=new Scroller(this,"ww-xscroller",SC_XSCROLL|SC_ABOTTOM,
//					0,win_h-scrollwidth, win_w-(yscroller&&yscroller->win_on&&yscrollislocal?scrollwidth:0),scrollwidth, 0,
//					NULL,window,"xscroller",
//					NULL,
//					0,mostpixwide,1,textrect.width-2*padx);
//				xscrollislocal=1;
//				app->addwindow(xscroller,0);
//			}
//			
//			textrect.height-=scrollwidth;
//			lpers=(textrect.height-2*pady)/textheight;
//			makelinestart(0,-1,1,1);
//			
//			if (p)
//				if (yscroller) {
//					yscroller->Resize(scrollwidth,win_h-scrollwidth);
//					yscroller->SetSize(0,textlen, linestats[0].start,linestats[lpers].start, -1,0);
//				} else newyssize();
//		}				
//	} else {			
//		if (xscroller && xscroller->win_on) {  // turn off xscroller 
//			app->unmapwindow(xscroller);
//			curlineoffset=-(padx+textrect.x);
//			textrect.height+=scrollwidth; 
//			lpers=(textrect.height-2*pady)/textheight;
//			makelinestart(0,-1,1,1);
//			findcaret();
//			if (p && yscroller) {
//				yscroller->Resize(scrollwidth,win_h);
//				yscroller->SetSize(0,textlen, linestats[0].start,linestats[lpers].start, -1,0);
//			}
//		}
//	}
//	return 1;
}

//! Try to figure out if screen still laid out correctly based on word wrapping.
/*!  Called anytime changes are made in thetext.
 *   It changes linestats, needtodraw, dpos, nlines.
 *   
 *   This inspects lines starting at fromline to ensure that they are.
 *   correct given current wrapping setup. It was checking each line only
 *   until the next line is correct, but that doesn't catch all changes, for
 *   instance adding a character to one line may change the previous line (if it
 *   is a space in a wrapped word for instance) or it may only cause the line
 *   it was typed on wrap differently. So you must scan from fromline to eo-screen.
 *  
 *   \todo *** perhaps an optimization would be wwinspect(fromline,scanAtLeastThisManyLines)
 */
void MultiLineEdit::wwinspect(int fromline)//fromline=0
{ 
	long pos;
	int  len=0,nl=0;
	if (fromline>lpers) fromline=lpers;
	if (fromline<0) { // line is offscreen, so just jump so that it is onscreen
		makelinestart(0,findlinestart(linestats[0].start),0,0);
		newyssize(); 
		newxssize();
		needtodraw=1;
	} else { // scan through lines starting at fromline
			// 
		pos=linestats[fromline].start;
		do {
			len=0;
			pos=findline(pos,len);
			if (pos!=linestats[fromline+1].start) { 
				linestats[fromline].pixlen=len;
				linestats[fromline+1].start=pos;
			} 
			fromline++;
			nl++;
		} while (fromline<lpers+1);
		if (nl) needtodraw|=2;
	}
	if (nlines) nlines=nlines>nl?nlines:nl;
	else nlines=nl;
	if (nlines) {
		pos=linestats[fromline>=0?fromline:0].start;
		dpos=dpos<pos?dpos:pos;
	}
	findcaret();
}


//! Insert character ch at curpos.
int MultiLineEdit::inschar(int ch)
{
	long ocp=curpos;
	if (TextEditBaseUtf8::inschar(ch)) return 1;
	dpos=ocp; nlines=1;
	cdir=0;
	int f=curpos-ocp;
	if (curline<0 || curline>=lpers) makeinwindow();
	if (ch==newline) {
		curlineoffset=-(padx+textrect.x);
		curline++;
		for (int d=lpers+1; d>curline; d--)
			{ linestats[d]=linestats[d-1]; linestats[d].start+=f; }
		linestats[curline].start=curpos;
		linestats[curline].pixlen=GetExtent(linestats[curline].start,linestats[curline+1].start,0,linestats[curline+1].start);
		linestats[curline].indent=0;
		linestats[curline-1].pixlen=GetExtent(linestats[curline-1].start,linestats[curline].start,0,linestats[curline+1].start);
		cx=padx;
		cy=textrect.y+(pady)+curline*textheight+textascent;
		Getmostwide();
		wwinspect(curline-2);
		if (textstyle&TEXT_XSCROLL) newxssize();
		if (textstyle&TEXT_YSCROLL) newyssize();
		if (makeinwindow()) needtodraw=1;
		else { nlines=0; needtodraw|=2; }
	} else {
		for (int d=curline+1; d<lpers+2; d++) linestats[d].start+=f;  // shift down in memory 
		linestats[curline].pixlen=GetExtent(curpos,linestats[curline+1].start,cx-padx+curlineoffset,linestats[curline+1].start);
		if (linestats[curline].pixlen>maxpixwide || !isword(curpos-1))
			wwinspect(curline-1);
		if (linestats[curline].pixlen>mostpixwide) {
			mostpixwide=linestats[curline].pixlen;
			longestline=curline;
			if (textstyle&TEXT_XSCROLL) newxssize();
		}
		findcaret();
		needtodraw|=(makeinwindow()?1:2);
	}
	Modified();
	return 0;
}

//! Delete character at curpos. bksp==0 means curpos, else curpos-1.
/*! *** this is rather convoluted, rethink it */
int MultiLineEdit::delchar(int bksp) // assumes not sellen 
{
	cdir=0;
	long f=0;
	char ch=0;
	if (bksp) {
		if (curpos>0) f=onlf(curpos-1); 
		else f=onlf(curpos);
	}
	if (f) ch='\n'; else f=1; // f is the number of chars slated for deletion
	
	if (TextEditBaseUtf8::delchar(bksp)) return 1;
	if (curline<0 || curline>lpers) { 
		needtodraw|=makeinwindow();
	} else if (ch==newline || (curpos!=textlen && curpos==linestats[curline+1].start-f)) {
		makelinestart(curline,-1,0,0);
		newyssize();
		newxssize();
		findcaret();
		makeinwindow();
		needtodraw=1;
	} else {
		for (int c=curline+1; c<lpers+2; c++) linestats[c].start-=f;
		if (longestline==curline) {
			Getmostwide();
			newxssize();
		}
		dpos=curpos; nlines=1;
		wwinspect(curline-1);
		needtodraw|=(makeinwindow()?1:2);
	}
	Modified();
	return 0;
}

//! Insert a string. If after!=0 then place curpos after the string.
int MultiLineEdit::insstring(const char *blah,int after)	// after=0
{
	long oldpos=curpos;
	if (TextEditBaseUtf8::insstring(blah,after)) return 1;
	dpos=oldpos;
	cdir=0;
	if (dpos>=linestats[lpers].start)
		makelinestart(lpers-1,-2,0,0);
	else if (dpos>=linestats[0].start)
		makelinestart(curline-1,-1,1,0);
	else makelinestart(0,-2,1,0);
	findcaret();
	newyssize();
	newxssize();
	Modified();
	needtodraw|=(makeinwindow()?1:2);
	return 0;
}

//! Replace the selection with this character.
int MultiLineEdit::replacesel(int ch)
{
	char c[6];
	int l=utf8encode(ch,c);
	c[l]='\0';
	return replacesel(c,1);
}

//! Delete selection.
/*! *** a little messy here
 */
int MultiLineEdit::delsel()
{
	long n,c;
	cdir=0;
	n=curline;
	c=getscreenline(selstart)-n;
	if (c<0) { n+=c; c=-c; }
	if (TextEditBaseUtf8::delsel()) return 1;

	dpos=curpos;
	nlines=c;

	makelinestart(n-1,-1,1,0);
	findcaret();
	makeinwindow(); 

	if (textstyle&TEXT_YSCROLL) newyssize();
	if (textstyle&TEXT_XSCROLL) newxssize();

	needtodraw|=2;
	return 0;
}

//! Return the screen line corresponding to character pos.
/*! If the position is before the screen, then -1 is returned.
 * If the position is after the screen, then lpers+2 is returned.
 *
 * *** see also WhichLine()..
 */
int MultiLineEdit::getscreenline(long pos)
{
	if (pos<linestats[0].start) return -1;
	int c;
	for (c=0; c<lpers; c++) if (pos<linestats[c+1].start) return c;
	return c+2;
}

int MultiLineEdit::replacesel(const char *newt,int after) //newt=NULL deletes, after=0
{
	if (!newt) return delsel();
	delsel();
	return insstring(newt,after);
}


int MultiLineEdit::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d) 
{
	int c=0,pout;

	if (!(state&ControlMask) && ((ch>=32 && ch<255) || ch=='\t' || ch==LAX_Enter) && !readonly()) {
		 //normal key press, insert character
		if (ch==LAX_Enter) ch='\n';
		if (sellen) return replacesel(ch);
		else return inschar(ch);
	}

	if (state&ControlMask && ch>=32) // scan for ^char (<32 handled below)
	  switch(ch) { // cntl+? 
		//case 'q': app->quit(); return 0;
		case 'a': cdir=0; selstart=0; curpos=sellen=textlen; return needtodraw=1; 
		case 'x':
		case 'X': ch=LAX_Del; state=ShiftMask; break;//cut 
		case 'c':
		case 'C': ch=LAX_Ins; state=ControlMask; break;//copy
		case 'v':
		case 'V': ch=LAX_Ins; state=ShiftMask; break;//paste
		//case 'f': return GetFont();

        case 'j': { //try to join selected (ascii) characters into appropriate accented characters
                if (CombineChars()==0) needtodraw=1;
                return 0; 
            } 
 
		case 'l':
			textstyle^=TEXT_LINE_NUMBERS;
			settextrect();
			makelinestart(0,-1,0,0);
			newxssize();
			newyssize();
			findcaret();
			needtodraw=1;
			return 0;
	
		case 'n': // toggle NLONLY 
			textstyle^=TEXT_NLONLY;
			makelinestart(curline,-2,0,0);
			newxssize();
			newyssize();
			findcaret();
			needtodraw=1;
			return 0;

		case 't':
			tabwidth++;
			makelinestart(0,-2,1,0);
			newyssize(); newxssize();
			findcaret();
			needtodraw=1;
			return 0;

		case 'T':
			tabwidth--; if (tabwidth<1) tabwidth=1;
			makelinestart(0,-2,1,0);
			newyssize(); newxssize();
			findcaret();
			needtodraw=1;
			return 0;

		case '.':
			 //jump to last modification point
			SetCurpos(modpos);
			return 0;

		case ' ': 
			if (state & ControlMask) { textstyle^=TEXT_SHOW_WHITESPACE; needtodraw=1; return 0; }
			if (textstyle&TEXT_WORDWRAP) {
				textstyle^=TEXT_WORDWRAP;
				maxpixwide=30000;
			} else {
				textstyle|=TEXT_WORDWRAP;
				maxpixwide=textrect.width-2*padx;
			}
			makelinestart(0,-2,0,0);
			newyssize();
			newxssize();
			findcaret();
			needtodraw=1;
			return 0;

		case '\\': // toggle how to display control chars 
			if (textstyle&TEXT_CNTL_BANG) {
				textstyle=(textstyle&~(TEXT_CNTL_BANG|TEXT_CNTL_NONE|TEXT_CNTL_HEX))|TEXT_CNTL_HEX;
				cntlchar='\\'; //***
			} else if (textstyle&TEXT_CNTL_HEX) {
				textstyle=(textstyle&~(TEXT_CNTL_BANG|TEXT_CNTL_NONE|TEXT_CNTL_HEX))|TEXT_CNTL_NONE;
			} else {
				textstyle=(textstyle&~(TEXT_CNTL_BANG|TEXT_CNTL_NONE|TEXT_CNTL_HEX))|TEXT_CNTL_BANG;
				cntlchar='\\';
			}
			firsttime=1;
			//SetupMetrics();
			findcaret();
			needtodraw=1;
			return 0;

		case 'z': //undo?
			if (state & ShiftMask) Redo();
			else Undo();
			return 0;

	} //switch

	switch(ch) { // handle any other various control characters that don't need control key pressed
		case 0: return 0; // null 
		case LAX_Tab:  // ^tab nextcontrol, ^+tab for prev, if was not ^, tab was handled above
			if (state&ControlMask) {
				if (state&ShiftMask) SelectPrevControl(d);
				else SelectNextControl(d);
			}
			return 0;

		case LAX_Ins:
			if (state&ControlMask) { //cntl+ins: copy
				if (!sellen) return 0;
				Copy();
				return 0;
			}
			if (state&ShiftMask) { //shift+ins: paste	
				if (readonly()) return 0;
				//Paste();
				selectionPaste(0,None);
				return 0;
			}		
			return 0;//ins

		case LAX_Del:
			if (readonly()) return 0; //del
			if (state&ShiftMask) {	//shift+del: cut
				if (!sellen) return 0;
				Copy();
				return delsel();
			}
			if (sellen) return delsel(); //***cn+del=del to endl?
			if (curpos==textlen) return 0;
			delchar(0);
			return 0;

		case LAX_Bksp:
			if (readonly(prevpos(curpos))) return 0; //bksp, ***sh+bk=del line,? 
			if (sellen) return delsel();//cn+bk=del to begl?
			delchar(1);
			return 0;

		case LAX_Home: // home 
			if (state&ShiftMask && !sellen) selstart=curpos;
			if (state&ControlMask) { curpos=curline=0; } //ct+home
			else {
				if (curline<0 || curline>=lpers) needtodraw|=makeinwindow();
				curpos=linestats[curline].start;
			}
			cdir=0;
			findcaret();
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			needtodraw|=(makeinwindow()?1:4);
			return 0;

		case LAX_End: // end
			if (state&ShiftMask && !sellen) selstart=curpos;
			if (curpos==textlen) { needtodraw|=makeinwindow(); return 0; }
			cdir=0;
			if (state&ControlMask) { // ct+end=eof 
				curpos=textlen; 
				makelinestart(lpers+1,textlen,-1,0);
				newyssize(); newxssize();
				c=makeinwindow();
				findcaret();
			} else {
				if (curline<0 || curline>=lpers) c=makeinwindow();
				curpos=linestats[curline+1].start;
				if (curpos!=textlen) curpos=prevpos(curpos);
				if (onlf(curpos)==2) curpos--;
				findcaret();
			}
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			needtodraw|=c|(makeinwindow()?1:4);
			return 0;

		case LAX_Pgup: //pgup
			if (state&ShiftMask && !sellen) selstart=curpos;
			pout=curlineoffset+cx;
			c=0;

			if (curline<0 || curline>=lpers) makeinwindow();	
			if (linestats[0].start==0) curline=0;
			else {
				linestats[lpers-1]=linestats[0];
				makelinestart(lpers-1,-1,0,0);
				c=curline;
				newyssize(); 
				newxssize();
				curline=c;
			}
			if (cdir) pout=cdir; else cdir=pout;
			curpos=countout(curline,pout);
			cx=pout-curlineoffset;
			cy=textrect.y+(pady)+curline*textheight+textascent;
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			return needtodraw|=4;

		case LAX_Pgdown: //pgdn
			if (state&ShiftMask && !sellen) selstart=curpos;
			pout=curlineoffset+cx;

			if (curline<0 || curline>=lpers) makeinwindow();
			if (linestats[lpers].start==textlen) {
				c=lpers;	
				while (linestats[c].start==textlen) { c--; }
				if (onlf(linestats[c].start-1)) c++;
				curline=c;
			} else {
				linestats[0]=linestats[lpers-1];
				makelinestart(0,-1,1,0);
				c=curline;
				newyssize(); 
				newxssize();
				curline=c;
			}
			if (cdir) pout=cdir; else cdir=pout;
			curpos=countout(curline,pout);
			cx=pout-curlineoffset;
			cy=textrect.y+(pady)+curline*textheight+textascent;
			if (curpos==textlen) findcaret();
			if (state&ShiftMask) sellen=curpos-selstart;
			else if (sellen) {
				sellen=0;
				dpos=(selstart<curpos?selstart:curpos);
				if (needtodraw!=1) needtodraw|=2;
			}
			return needtodraw|=4;

		case LAX_Up: //up
			if (state&ShiftMask && !sellen) selstart=curpos;
			makeinwindow();
			if (linestats[curline].start==0) return c;
			pout=curlineoffset+cx;
			 // set cy,curline first
			if (state&ControlMask) { // ct+up = top of screen 
				if (curline==0) return 0;
				curline=0;
				cy=textrect.y+(pady)+textascent;
			} else {
				if (curline==0) {
					for (int c2=lpers+1; c2>0; c2--) linestats[c2]=linestats[c2-1];
					makelinestart(1,-1,-1,0);
					newyssize(); 
					newxssize();
				} else { curline--;  cy-=textheight; }
			}

			 // find pos, cx
			if (cdir) pout=cdir; else cdir=pout;
			curpos=countout(curline,pout);
			cx=pout-curlineoffset;
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			needtodraw|=makeinwindow()|4;
			return 0;

		case LAX_Down: //down
			if (state&ShiftMask && !sellen) selstart=curpos;
			makeinwindow();
			pout=curlineoffset+cx;
			if (linestats[curline].start==textlen 
					|| (linestats[curline+1].start==textlen 
					 && linestats[curline].start!=textlen 
					 &&	!onlf(textlen-1)))
				return needtodraw;
			if (state&ControlMask) { // ct+dn = screen bottom  
				if (curline==lpers-1) return 0;
				curline=lpers-1;
				cy=textrect.y+(pady)+curline*textheight+textascent;
			} else {
				curline++;
				cy+=textheight;
				if (curline==lpers) {
					for (int c2=0; c2<lpers+1; c2++) linestats[c2]=linestats[c2+1];
					curline--;
					makelinestart(lpers,-1,1,0);
					newyssize();
					c=curline;	newxssize(); curline=c; //***???
					cy=textrect.y+(pady)+textheight*curline+textascent;
					needtodraw=1;
				}
			}
			if (cdir) pout=cdir; else cdir=pout;
			curpos=countout(curline,pout);
			cx=pout-curlineoffset;
			if (curpos==textlen) findcaret();
			if (state&ShiftMask) sellen=curpos-selstart;
				else sellen=0;
			needtodraw|=makeinwindow()|4;
			return 0;

		case LAX_Left: //left
			DBG cerr <<endl<<endl<<" --left from "<<curpos<<endl;
			if (state&ShiftMask && !sellen) selstart=curpos;
			cdir=0;
			makeinwindow();
			if (curpos==0) { 
				DBG cerr <<" --left: curpos==0"<<endl; 
				return 0; 
			}
			if (state&ControlMask) {  // skip words 
				int c2=0;
				curpos=prevpos(curpos);
				while (curpos>0 && c2<10 && !isword(curpos))
					{ curpos=prevpos(curpos); c2++; }
				while (curpos>0 && isword(prevpos(curpos))) curpos=prevpos(curpos);
				findcaret();
				if (curline<0) makeinwindow();
			} else {
				curpos=prevpos(curpos);
				DBG cerr <<" --go left to "<<curpos<<" char:"<<thetext[curpos]<<endl;
				if (curpos<linestats[curline].start) {
					DBG cerr <<" --left go up a line"<<endl;
					curline--;
					if (onlf(curpos)==2) curpos--;
					findcaret();
				} else {
					//if (onlf(curpos)==2) curpos--; ***this never happens here?
					if (thetext[curpos]=='\t' && textstyle&TEXT_TABS_STOPS)
						cx=GetExtent(linestats[curline].start,curpos,0,linestats[curline+1].start)-curlineoffset;
					else cx-=charwidth(thetext[curpos]);
				}
			}
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			needtodraw|=makeinwindow()|4;
			return 0;

		case LAX_Right: //right
			c=cdir=0;
			if (state&ShiftMask && !sellen) selstart=curpos;
			if (state&ControlMask) {
				while (curpos<textlen && isword(curpos)) curpos=nextpos(curpos);
				while (curpos<textlen && c<10 && !isword(curpos))
					{ curpos=nextpos(curpos); c++; }
				findcaret();
			} else {
				makeinwindow();
				if (curpos==textlen) return 0;
				curpos=nextpos(curpos);
				if (onlf(curpos)==2) curpos++;
				if (curpos==linestats[curline+1].start && curpos!=textlen) {
					curline++;
					cy+=textheight;
					cx=-curlineoffset;
				} else {
					if (thetext[curpos-1]=='\t' && textstyle&TEXT_TABS_STOPS)
						cx=GetExtent(linestats[curline].start,curpos,0,linestats[curline+1].start)-curlineoffset;
					else cx+=charwidth(thetext[curpos-1]);
				}
			}
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			needtodraw|=(makeinwindow()?1:4);
			return 0;

		default: return anXWindow::CharInput(ch,buffer,len,state,d);
	}

	return anXWindow::CharInput(ch,buffer,len,state,d);
}


//! Returns position most appropriate for x=pix, y=screen line l
/*! Sets curpos to pos near (x,y)=(pix,l)
 *
 * If conv==1 then l is a pixel value.
 */
long MultiLineEdit::findpos(int l,int pix,int updatecp,int conv) //updatecp==1,conv==1
{
	if (conv) {
		l=(l-pady-textrect.y)/textheight;
		pix=pix+curlineoffset;
	}
	
	if (pix<0) pix=0;
	long newpos=0;
	DBG cerr <<"\nFind pos: pix:"<<pix<<" line:"<<l;

	if (l<0) { // pos is somewhere above
		newpos=linestats[0].start-20*l;
		makevalidpos(newpos);
		if (newpos<0) newpos=0; //*** what about selstart
		if (updatecp && curpos!=newpos) { curpos=newpos; findcaret(); }

	} else if (l>lpers) { // pos is somewhere below
		newpos=linestats[lpers].start+20*l;
		makevalidpos(newpos);
		if (newpos>textlen) newpos=textlen;
		if (updatecp && curpos!=newpos) { curpos=newpos; findcaret(); }

	} else { // pos is somewhere on screen
		if (linestats[l].start==textlen) {
			while (l>0 && linestats[l].start==textlen) l=prevpos(l);
			if (l>0) if (onlf(linestats[l+1].start-1)) l=nextpos(l);
		}
		newpos=countout(l,pix);
		if (updatecp && curpos!=newpos) {
			curpos=newpos;
			curline=l;
			cy=textrect.y+(pady)+l*textheight+textascent;
			cx=-curlineoffset+pix;
		}
	}

	return newpos;
}

//! Find character position for screen line number linenumber and horizontal pixel pout.
long MultiLineEdit::countout(int linenumber,int &pout)
{ //*** maybe change GetPos to return the lsofar?
	long pos=GetPos(linestats[linenumber].start,pout,0); //*** must check how newlines are handled, should put pos at the newline if fits
	pout=GetExtent(linestats[linenumber].start,pos,0,linestats[linenumber+1].start);
	return pos;
}

//! Make sure curpos is in the window somewhere (shifts screen, does not change curpos.
/*! Assumes the caret's cx,cy already set accurately.
 */
int MultiLineEdit::makeinwindow()
{
	if (cx>=padx && cx<=win_w-padx &&
		cy-textascent>=pady  && cy+textdescent<=win_h-pady) return 0;

	if (cy-textascent<pady) { // cursor is above screen, go up 
		makelinestart(0,-2,1,0);
		findcaret(); 
		newyssize();
	} else if (cy+textdescent>=pady+lpers*textheight) { // curs is below screen, go down 
		makelinestart(lpers>0?lpers-1:0,-2,0,0);
		findcaret(); 
		newyssize();
	}
	 // get horizontal position  
	if (cx<padx+textrect.x) {
		curlineoffset+=cx-(padx+textrect.x);
		if (curlineoffset<-(padx+textrect.x)) curlineoffset=-(padx+textrect.x);
		if (xscroller) xscroller->SetCurPos(curlineoffset);
		needtodraw=1;
	} else if (cx>textrect.x+textrect.width-padx) {
		curlineoffset+=cx-(textrect.x+textrect.width)+padx;
		if (cx+curlineoffset>mostpixwide) {
			Getmostwide();
			longestline=curline;
			if (textstyle&TEXT_XSCROLL) newxssize(); 
		}
		if (xscroller) xscroller->SetCurPos(curlineoffset);
		needtodraw=1;
	}
	findcaret();
	return 1;
}

/*! this assumes curlineoffset set correctly
 */
void MultiLineEdit::findcaret()
{
	if (curpos>=linestats[lpers].start) { // is below screen
		if (linestats[lpers].start==textlen) { // curpos=textlen 
			curline=lpers;
			while (curline>0 && linestats[curline].start==textlen) curline--;
			if (textlen>0 && onlf(linestats[curline+1].start-1)) curline++;
			cy=textrect.y+(pady)+textheight*curline+textascent;
			cx=GetExtent(linestats[curline].start,curpos,0,linestats[curline+1].start)-curlineoffset;		 
		} else {
			cy=(win_h-pady)+1+textascent;
			curline=lpers+2;
		}

	} else if (curpos<linestats[0].start) { // is above screen
		cy=-1;
		curline=-1;

	} else { // is on screen
		curline=lpers-1;
		while (curpos<linestats[curline].start) curline--;
		cy=textrect.y+(pady)+textheight*curline+textascent;
		cx=GetExtent(linestats[curline].start,curpos,0,linestats[curline+1].start)-curlineoffset;		 
	}
}

//! Shift the view by x pixels and y lines.
/*! 
 * \todo *** need smart ShiftScreen XCopyArea
 * 
 * Returns 1 if there is a change, 0 if not
 */
int MultiLineEdit::ShiftScreen(int x,long y)  // ylines 
{
	DBG cerr <<"ShiftScreen:"<<x<<','<<y<<endl;
	if (!x && !y) return 0;
	if (x && !(textstyle&TEXT_WORDWRAP) && mostpixwide>win_w-2*padx) {
		//DBG cerr <<':'<<x<<','<<xscroller->GetCurPos();
		if (x>0) { // shift screen right
			if (curlineoffset-x<-padx) x=curlineoffset+padx;
		} else if (x<0) {
			if (mostpixwide-(curlineoffset-x)<win_w-padx) x=win_w-padx-mostpixwide+curlineoffset;
		}
		cx+=x;
		curlineoffset-=x;
		if (xscroller) xscroller->SetCurPos(curlineoffset);

		DBG cerr <<" "<<x<<','<<(xscroller?xscroller->GetCurPos():-10000)<<" mpw="<<mostpixwide;
		needtodraw=1;
		return 1;
	}
	if (!y) return 0;
	while (y) {
		if (y>0) {	// shift screen upward
			if (linestats[lpers-1].start==textlen) break;
			if (y>0 && y<lpers+1) {
				makelinestart(0,linestats[y].start,1,0);
				break;
			}
			 //y>lpers
			makelinestart(0,linestats[lpers].start,1,0);
			y-=lpers;
		} else if (y<0) { // shift screen downward
			if (linestats[0].start==0) break;
			if (y<0 && -y<lpers+1) {
				makelinestart(-y,linestats[0].start,0,0);
				break;
			}
			 // y<-lpers 
			makelinestart(lpers,linestats[0].start,0,0);
			y+=lpers;
			if (y>0) y=0;
		}
	}
	//XCopyArea(app->dpy,window,window,app->gc(), 0,0,win_w,win_h, x,y); ***
	newyssize();
	newxssize();
	findcaret();
	needtodraw=1;
	return 1; 
}

/*! \todo *** should put in drag and drop when lbdown on a selection
 */
int MultiLineEdit::LBDown(int x,int y, unsigned int state,int count,const LaxMouse *d)
{
	//DBG cerr<<"-gewwld-";
	buttondown.down(d->id, LEFTBUTTON, x,y);
	
	//*** check for trpl click?
	if (count==2) return LBDblClick(x,y,state,d);

	// desel, addtosel, dragdrop?, repos
	if (state&ShiftMask) { if (sellen==0) selstart=curpos; }
	else { sellen=0; }
	findpos(y,x); // this sets curpos and findscaret
	DBG cerr <<' '<<thetext[curpos]<<':'<<(int)thetext[curpos]<<' ';
	if (state&ShiftMask) sellen=curpos-selstart;
	needtodraw|=makeinwindow()|4;
	return 0;
}

//int MultiLineEdit::LBTrplClick(int x,int y, unsigned int state) // select line or paragraph??
//! Selects whole word.
int MultiLineEdit::LBDblClick(int x,int y, unsigned int state,const LaxMouse *d)
{
	//DBG cerr<<"-gewwLDClk-";

	//select word*
	int c=0;
	findpos(y,x);
	if (sellen) {
		sellen=0;
		needtodraw|=4;
		return 0;
	}
	selstart=curpos;

	if (isalnum(thetext[selstart])) { // find word
		while (selstart>0 && isword(selstart-1)) selstart=prevpos(selstart);
		while (curpos<textlen && isword(curpos)) curpos=nextpos(curpos);

	} else { // find block of whitespace
		while (selstart>0 && c<10 && !isword(selstart-1) && !onlf(selstart-1))
			{ selstart=prevpos(selstart); c++; }
		while (curpos<textlen && c<10 && !isword(thetext[curpos]) && !onlf(curpos))
			{ curpos=nextpos(curpos); c++; }
	}
	findcaret();
	dpos=selstart; nlines=1;
	sellen=curpos-selstart;
	needtodraw|=2;
	return 0;
}

//! Button up, nothing special.
int MultiLineEdit::LBUp(int x,int y, unsigned int state,const LaxMouse *d)
{ 
	//DBG cerr<<"-gewwlu-";
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 0;
	buttondown.up(d->id, LEFTBUTTON);
	return 0;
}

//! Button down, nothing special.
int MultiLineEdit::RBDown(int x,int y, unsigned int state,int count,const LaxMouse *d)
{
	//DBG cerr<<"-gewwrd-";
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	return 0;
}

//! Button up, nothing special.
int MultiLineEdit::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	//DBG cerr<<"-gewwru-";
	if (!buttondown.isdown(d->id,RIGHTBUTTON)) return 0;
	buttondown.up(d->id, RIGHTBUTTON);
	mx=x; my=y;
	return 0;
}

//! Roll screen.
int MultiLineEdit::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&ControlMask) { // shift x
		if (state&ShiftMask) ShiftScreen((win_w-2*padx)*4/5,0);
		else ShiftScreen(1,0);
	} else { // shift y
		if (state&ShiftMask) ShiftScreen(0,lpers-1);
		else ShiftScreen(0,1);
	}
	return 0;
}

//! Roll screen.
int MultiLineEdit::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&ControlMask) { // shift x
		if (state&ShiftMask) ShiftScreen(-(win_w-2*padx)*4/5,0);
		else ShiftScreen(-1,0);
	} else { // shift y
		if (state&ShiftMask) ShiftScreen(0,-lpers+1);
		else ShiftScreen(0,-1);
	}
	return 0;
}

/*! \todo institute drag and drop for selections
 */
int MultiLineEdit::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{ 
	//DBG cerr << "\ngeMove: ";
	if (buttondown.isdown(d->id,LEFTBUTTON)) {
		if (x>=padx && x<(win_w-padx) &&
			y>=(pady) && y<(pady)+lpers*textheight) {
			if (!sellen) selstart=curpos;
			findpos(y,x);
			sellen=curpos-selstart;
			needtodraw|=4;
			return 0;
		}

	} else if (buttondown.isdown(d->id,RIGHTBUTTON)) { // ***move screen
		buttondown.move(d->id, x,y);
		if (y-my>textheight) { ShiftScreen(x-mx,-1); if (y>0 && y<win_h) { my=y; } }
		else if (my-y>textheight) { ShiftScreen(x-mx,1); if (y>0 && y<win_h) { my=y; } }
		mx=x;
		return 0;
	}

	return 1;
}

/*! currently empty...
 */
int MultiLineEdit::Idle(int tid, double delta)
{ 
	return 0;
}

//! Return whether position pos is part of a word. Default is alphanumeric or a period.
int MultiLineEdit::isword(long pos)
{  	return isalnum(thetext[pos]) || thetext[pos]=='.'; }

/*! Returns pos of start of next potential screen line after ls, assumes ilsofar set right
 * updates ilsofar which basically becomes pixlen of line with ls.
 */
long MultiLineEdit::findline(long ls,int &ilsofar)
{
	int lsofar=ilsofar;
	long pos=ls;

	 // first put pos at eol, use the lsofar+=thing to have rough estimate of 
	 //  how much is too much. that lsofar is guaranteed to be <= actual lsofar
	if (onlf(pos)) {
		if (textstyle&TEXT_NLONLY) return pos+1;
		else return pos+2;
	}

	while (pos<textlen && !onlf(nextpos(pos))) {
		pos=nextpos(pos);
	}
	lsofar=GetExtent(ls,nextpos(pos),ilsofar,nextpos(pos)); //range [ls,nextpos(pos))
	if (lsofar>=maxpixwide) { // wrap 
		while (pos>ls && lsofar>=maxpixwide) { // puts pos inside maxpixwide 
			lsofar=GetExtent(ls,pos,ilsofar,pos);
			pos=prevpos(pos);
		}
		long tpos=pos;
		int tlsofar=lsofar;
		while (pos>ls && isword(prevpos(pos))) pos=prevpos(pos); // puts pos at beginning of word
		//tlsofar=GetExtent(ls,pos,ilsofar);***whats this

		if (pos==ls) { 
			 // word is too long to wrap or there are no chars in line
			ilsofar=tlsofar;
			return nextpos(tpos);
		}
	} else if (pos!=textlen) { // pos must be \n 
		pos=nextpos(pos);
		if (onlf(pos)) pos=nextpos(pos);
	}
	ilsofar=lsofar;
	return pos;
}

//! Find what should be the start of the screen line that contains pos
long MultiLineEdit::findlinestart(long pos)//pos=-1 means use curpos
{
	if (pos<0) pos=curpos;
	if (pos>textlen) pos=textlen;
	long npos=pos,onpos=pos;
	int l=0;
	while (npos>0 && !onlf(npos-1))	npos=prevpos(npos);
	do { onpos=npos; npos=findline(npos,l); } while (npos<pos);
	return (npos==pos?npos:onpos);
}

//! Remaps linestats, but does not findcaret.
/*! It does find and set mostpixwide,longestline, dpos,nlines
 * 
 * \todo *****put newyssize in mls?
 */
int MultiLineEdit::makelinestart(int startline, //! Screen line to begin remapping from
								 long ulwc,    //! Character to place at beginning of screen line startline
								 int godown,   //! If godown<=0, then remap backwards from startline, else forwards from startline
								 int ifdelete //! 1 to reallocate linestats, else do not reallocate
								)
{ 	 // ulwc=-1 use current ulwc, -2 means use cls, ulwc=-1,godown=1, ifdelete=0
	if (firsttime) return 0;
	
	//DBG cerr <<" m";

	if (startline<0) startline=0;
	if (startline>lpers+1) startline=lpers+1;
	if (!linestats) startline=0;

	long leftwchar;
	if (ulwc==-2) leftwchar=findlinestart();
	else if (ulwc<0) {
		if (linestats) leftwchar=linestats[startline].start;
		else leftwchar=0;
	} else if (ulwc>textlen) leftwchar=textlen;
	else leftwchar=ulwc;

	if (godown==1 && linestats && startline!=0) dpos=linestats[startline].start;
		else dpos=0;
	nlines=0;
	
	if (ifdelete) {
		Linestat *temp=linestats;
		linestats=new Linestat[lpers+2];
		if (temp) {
			for (int c=0; c<lpers+2; c++) linestats[c]=temp[c];
			delete[] temp;
		}
	}

	linestats[startline].start=leftwchar;
	int lsofar,c=startline;
	if (godown<=0) { // proceed backwards until line starts == linestats.start
		int c2,lfroml;
		long pos,temp,atline;
		if (leftwchar==0) { // there are only blank lines beneath (< startline)
			if (startline!=0) { startline=0; godown=1; }
			linestats[0].start=0;
		}
		c=startline;
		 // find mapping for linestats.start, going -1 from startline
		while (c>0) {
			atline=linestats[c].start;
			if (atline>textlen) { atline=linestats[c].start=textlen; }
			leftwchar=prevpos(atline);
			if (atline==0) { // reached beginning, so copy what's gotten already, and jump to godown>0
				for (int c2=0; c2<=startline-c; c2++) linestats[c2]=linestats[c2+c];
				startline=startline-c;
				godown=0;
				break;
			}
			 // put leftwchar at paragraph start of line containing line number c-1
			while (leftwchar>0 && !onlf(leftwchar-1)) leftwchar=prevpos(leftwchar);
			pos=leftwchar;
			lfroml=0; // lfroml counts how many lines are in the paragraph being scanned
			while (pos<atline) { // scan forward in line from lwc 
				lsofar=0;
				temp=pos;
				pos=findline(pos,lsofar);
				linestats[c-1].start=temp;
				linestats[c-1].pixlen=lsofar;
				lfroml++;
				if (pos<atline) // must squeeze more sublines in due to wrapping
					for (c2=c-1-lfroml; c2<c-1; c2++)
						if (c2>=0) linestats[c2]=linestats[c2+1];
				if (c==startline && pos>atline) 
					{ godown=1; linestats[c].start=pos; }
			}
			c-=lfroml;
			leftwchar=prevpos(leftwchar);
		}
	}
	leftwchar=linestats[startline].start;
	if (godown>=0) {
		for (c=startline; c<lpers+2; c++) {
			 //***the following if:
			 //you are at eof, this ensures there are no empty lines
			 //at start of linestats.. does this actually work
			 //as intended???
			if (leftwchar==textlen && c<lpers) {
				int c3;
				for (c3=c; c3<lpers+2; c3++) { linestats[c3].start=textlen; linestats[c3].pixlen=0; }
				c3=lpers;
				linestats[c3].start=linestats[lpers+1].start=textlen;
				linestats[c3].pixlen=linestats[lpers+1].pixlen=0;	
				if (onlf(textlen-1)) {
					c3--;
					linestats[c3].start=textlen;
					linestats[c3].pixlen=0;
				}
				while (linestats[0].start!=0 && c<c3) { // shift down text: minimize empty lines shown
					for (int c2=c; c2>0; c2--) {
						linestats[c2]=linestats[c2-1];
					}
					makelinestart(1,-1,-1,0); // updates linestats[0]
					c++;
				}
				break;	
			}
			linestats[c].start=leftwchar;
			lsofar=0;
			leftwchar=findline(leftwchar,lsofar);
			linestats[c].pixlen=lsofar;
		}
	}
	mostpixwide=linestats[0].pixlen;
	longestline=0;
	for (c=1; c<lpers; c++) {
		if (mostpixwide<linestats[c].pixlen) {
			mostpixwide=linestats[c].pixlen;
			longestline=c;
		}
	}
//	findcaret();
//	if (textstyle&TEXT_YSCROLL) newyssize();

	//DBG cerr<<"mdone"<<endl;
	//DBG cerr <<"makelinestart: lpers="<<lpers<<endl;
	//DBG char ch[100];
	//DBG int ll;
	//DBG for (c=0; c<lpers+1; c++) {
	//DBG		if (linestats[c].start<textlen && linestats[c+1].start-linestats[c].start>0) {
	//DBG			ll=linestats[c+1].start-linestats[c].start;
	//DBG			if (ll>95) ll=95;
	//DBG 		strncpy(ch,thetext+linestats[c].start,ll);
	//DBG 		ch[ll]='\0';
	//DBG		} else strcpy(ch,"\n");
	//DBG 	cerr <<"  "<<c<<","<<linestats[c].start<<": "<< ch;
	//DBG     if (strlen(ch)==0 || ch[strlen(ch)]!='\n') cerr <<endl;
	//DBG }
	//DBG cerr <<endl<<endl;

	return needtodraw|=dpos?2:1;
}

// // does not recompute pixlen
//long MultiLineEdit::Getlinewide(long which)	// which=-1 => curline
//{
//	if (which>=lpers) which=lpers-1;
//	if (which==-1) { which=curline; }
//	return linestats[which].pixlen;
//}

//! Returns which line is the most pixels wide.
/*! Does not recompute linestats.pixlens
 */
int MultiLineEdit::Getmostwide()
{
	if (!linestats) return 0;
	mostpixwide=linestats[0].pixlen;
	longestline=0;
	int c;
	for (c=1; c<lpers; c++) 
		if (linestats[c].pixlen>mostpixwide) {
			mostpixwide=linestats[c].pixlen;
			longestline=c;
		}
	if (xscroller) xscroller->SetSize(mostpixwide,textrect.width-2*padx);
	return c;
}

//! Return which screen line pos is in.
/*! If the position is before the screen, then -1 is returned.
 * If the position is after the screen, then lpers is returned (*** is that really oK??).
 *
 * ***  see getscreenline()... this is reimp from TextEditBaseUtf8, i think its really supposed
 * to be the absolute line..
 *
 * This function is used in TextXEditBaseUtf8::DrawCaret() to figure out what lines
 * to either highlight or de-highlight.
 */
long MultiLineEdit::WhichLine(long pos)
{
	int l=0;
	while (pos>=linestats[l].start && l<lpers+1) l++;
	return l-1;
}

void MultiLineEdit::Refresh()
{
	TextXEditBaseUtf8::Refresh();

	if (textstyle & TEXT_LINE_NUMBERS) {
		// *** need to respond to partial refreshing
		double th=thefont->textheight();
		Displayer *dp=GetDisplayer();
		dp->BlendMode(LAXOP_Over);
		dp->NewFG(coloravg(win_colors->bg,win_colors->fg));

		int nl=numlines;
		if (!onlf(thetext[textlen-1])) nl++;

		for (int c=0; c<nl; c++) {
			dp->drawnum(textrect.x/2, textrect.y + pady + c*th + th/2, c+1);
		}
	}
}

//! Draw the screen.
void MultiLineEdit::DrawText(int black) // black=1 
{
	char *posinline=thetext+linestats[0].start;
	int cline=0,spy;
	char check=1;
	//if (black) XClearWindow();
	background_color(win_colors->bg);
	foreground_color(win_colors->fg);
	if (curline+nlines<lpers+1 && linestats[curline+nlines].start==textlen) nlines=0;
	if (nlines==0) nlines=lpers+1;
	if (dpos==0) dpos=-1;
	spy=textrect.y+pady+textascent;

	 // posinline already==linestats[0].start 
	 // draw from [cline,cline+nlines), cline is the screen line containing dpos
	if (dpos!=textlen && dpos>=linestats[lpers+1].start) { spy=(win_h-pady); } // dpos definitely offscreen 
	else if (dpos>linestats[0].start) { 
		if (dpos==textlen) {
			cline=lpers;
			while (cline>0 && linestats[cline].start==textlen) cline--;
			if (textlen>0 && onlf(textlen-1)) cline++; 
		} else {	
			cline=0;
			while (cline<lpers+1 && dpos>=linestats[cline+1].start) cline++; 
		}
		spy=(pady)+cline*textheight+textascent;
		posinline=thetext+dpos; //*** this won't work when dpos is in a non-left tab segment
		//lsofar=GetExtent(linestats[cline].start,posinline-thetext,0,linestats[cline+1].start);
		GetExtent(linestats[cline].start,posinline-thetext,0,linestats[cline+1].start);
	}  

	if (textlen==0) nlines=0;
	while (spy<(win_h-pady) && nlines) {
 		 // *** this ignores small updates, shouldn't redraw the whole line???
		 //  	would need special check for non-left tabs when setting dpos
		DrawLineOfText(-curlineoffset,spy, linestats[cline].start,linestats[cline+1].start-linestats[cline].start,check);
		if (linestats[cline].start==textlen && !onlf(linestats[cline].start-1)) break;
		cline++;
		nlines--;
		spy+=textheight;
	}

	if (black && nlines && spy<(win_h-pady)) { // black out the remainder of the screen
		int x=padx,y=spy, w=win_w,h=win_h-y;
		if (w>0 && h>0) Black(x,y,w,h);
	}
	dpos=0; nlines=0;
	return;
}
	 
int MultiLineEdit::UseThisFont(LaxFont *newfont)
{
    TextXEditBaseUtf8::UseThisFont(newfont);
    return 1;
}

//! Call TextXEditBaseUtf8::SetupMetrics(), set firsttime=0, and SetupScreen().
int MultiLineEdit::SetupMetrics()
{
	if (TextXEditBaseUtf8::SetupMetrics()) return 1;
	SetupScreen();
	makeinwindow();
	return 0;
}

/*! Sets up lpers, does Getmostwide(), and moves scrollers (if any) to correct positions.
 * Then findcaret().
 * 
 * \todo *** if (xscroller && xscroller->win_on) xscroller->MoveResize(0,win_h-scrollwidth, win_w,scrollwidth);
 * \todo *** if (yscroller && yscroller->win_on) yscroller->MoveResize(win_w-scrollwidth,0, scrollwidth,win_h);
 */
void MultiLineEdit::SetupScreen()
{ 
	if (textheight==0) return;
	if (textrect.height==0) return;

	lpers=(textrect.height-2*pady)/textheight;
	DBG cerr <<"lpers=="<<lpers<<endl;

	makelinestart(0,-1,1,1);
	Getmostwide();

	if (textstyle&TEXT_WORDWRAP) {
		maxpixwide=textrect.width-2*padx;
	}

	if (textstyle&TEXT_XSCROLL) {
		if (xscroller && xscrollislocal) {
			xscroller->MoveResize(0,win_h-scrollwidth, win_w,scrollwidth);
		}
		newxssize();
	}

	if (textstyle&TEXT_YSCROLL) {
		if (yscroller && yscrollislocal) {
			yscroller->MoveResize(win_w-scrollwidth,0,scrollwidth,win_h);
		}
		newyssize(); // sets curpos 
	}

	findcaret();
	needtodraw=1;
}

void MultiLineEdit::settextrect()
{
	TextXEditBaseUtf8::settextrect();

	if (textstyle & TEXT_LINE_NUMBERS) {
		int nlines = GetNumLines();
		if (nlines<=0) nlines=1;
		int sz = int(log10(nlines)) + 1;
		char nn[sz+1];
		memset(nn, '0', sz);
		nn[sz]='\0';
		double w=thefont->extent(nn,sz);

		textrect.x += w+padx;
		textrect.width -= w+padx;
	}
}

/*! *** probably should cache resize events, so don't have to constantly
 * SetupScreen which drag-resizing a window.
 */
int MultiLineEdit::Resize(int nw,int nh)
{ 
	anXWindow::Resize(nw,nh);
	settextrect();
	firsttime=1;
	return 0;
}

/*! *** probably should cache resize events, so don't have to constantly
 * SetupScreen which drag-resizing a window.
 */
int MultiLineEdit::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	settextrect();
	firsttime=1;
	return 0;
}

Attribute *MultiLineEdit::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,savecontext);

	if (what==-1) {
		att->push("win_name","string");
		att->push("text","string");
		att->push("blanktext","(string) When actual text is nothing, show this, but remove when anything else typed.");
		att->push("align","one of: left center right");
		return att;
	}

	att->push("win_name",win_name);
	att->push("text",GetCText());
	if (!isblank(blanktext)) att->push("blanktext",blanktext);

	//if (textstyle&TEXT_LEFT) att->push("align","left");
	//else if (textstyle&TEXT_CENTER) att->push("align","center");
	//else if (textstyle&TEXT_RIGHT) att->push("align","right");

	return att;
}

void MultiLineEdit::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *loadcontext)
{
	anXWindow::dump_in_atts(att,flag,loadcontext);

	const char *name, *value;
	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp("win_name",name)) makestr(win_name,value);
		else if (!strcmp("text",name)) SetText(value);
		else if (!strcmp("blanktext",name)) makestr(blanktext,value);

//		} else if (!strcmp(name,"align")) {
//			textstyle&=~TEXT_LEFT|TEXT_CENTER|TEXT_RIGHT;
//			if (!strcmp(value,"left")) textstyle|=TEXT_LEFT;
//			else if (!strcmp(value,"center")) textstyle|=TEXT_CENTER;
//			else if (!strcmp(value,"right")) textstyle|=TEXT_RIGHT;
//		}

		//else if (!strcmp("ispassword",name)) ***;
		//else if (!strcmp("font",name)) ***;
	}
}


} // namespace Laxkit

