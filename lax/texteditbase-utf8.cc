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
//    Copyright (C) 2004-2013 by Tom Lechner
//


#include <lax/texteditbase-utf8.h>
#include <lax/utf8utils.h>
#include <lax/strmanip.h>


#include <iostream>
#define DBG 
using namespace std;



 // thetext only reallocated when crossing block edges of size TEXTMEMBLOCK
#define TEXTMEMBLOCK 300


namespace Laxkit {

//-----------------------------------------------------------

/*! \class TextEditBaseUtf8
 * \brief A base class for text edits using utf8 encoded character arrays.
 *
 * \todo might be good idea to somehow define the encoding? Latin-1, utf8, ascii? 
 *   probably much more hassle than it's worth.. simpler to do all in utf8.
 * \todo *** thinks all utf8 chars are actually distinct chars... is this true?
 *   are some utf8 actually tags for vowel marks over a previous char?
 *   at times like these, pango is looking pretty good..
 *
 * This class provides the basic memory management functions for manipulating
 * a big character array of text. These include inserting and deleting single
 * characters and strings of characters, and finding words and lines near
 * a given point. Actual formating, displaying, and such are not handled in this class.
 * TextXEditBase and classes derived from in handle displaying.
 *
 * It can handle any of the 3 main systems of line delimiting commonly found
 * in text files: '\\n' for unix/linux, 
 * '\\r' for macs, and '\\r\\n' for windows text files. The cursor should
 * always be on the first byte.
 * 
 * Really the point of the Laxkit text edits setup with 
 * TextEditBaseUtf8--TextXEditBase--ActualEdit is to provide 
 * a common text manipulation scheme, most of which I wouldn't 
 * have to reprogram whenever I wanted to make a text edit
 * in other window systems, like ncurses for instance. In terms of code generated,
 * and speed of action, it adds a couple of function calls per action, but 
 * that probably slows things down only just slightly. If that
 * seems to really be noticable, then at some point I would probably not bother
 * with any *EditBase, and just reimplement the scheme for single LineEdits, and
 * then again for GoodEditWW, since they both have rather different needs for
 * text lookup.
 * 
 * The following defines are kind of an all-in-one source for text edit
 * related flags. Not all of them are actually accessed in TextEditBaseUtf8.
 * These are stored in textstyle rather than win_style.
 * \code
 * #define LEFT_TAB                 (1<<0)
 * #define CENTER_TAB               (1<<1)
 * #define RIGHT_TAB                (1<<2)
 * #define CHAR_TAB                 (1<<3)
 * #define NUMERIC_TAB              (1<<3)
 * 
 * #define TEXT_LEFT                (1<<0)
 * #define TEXT_CENTER              (1<<1)
 * #define TEXT_RIGHT               (1<<2)
 * #define TEXT_JUSTIFY             (1<<3)
 * #define TEXT_JUSTIFY_RIGHT       (1<<2 | 1<<3)
 * #define TEXT_JUSTIFY_CENTER      (1<<1 | 1<<3)
 * #define TEXT_FORCEJUSTIFY        (1<<4)
 * 
 * #define TEXT_WORDWRAP            (1<<5)
 * #define TEXT_READONLY            (1<<6)
 * 
 * #define TEXT_NLONLY              (1<<7)
 * #define TEXT_CRLF                (1<<9)
 * #define TEXT_LF                  (1<<7)
 * #define TEXT_CR                  (1<<8)
 * 
 *  // write non-chars as cntl char (such as space, '.','\' etc), cntlchar+hex, none
 * #define TEXT_CNTL_BANG           (1<<10)
 * #define TEXT_CNTL_HEX            (1<<11)
 * #define TEXT_CNTL_NONE           (1<<12)
 * 
 *  // SHOWTABLINE means have a line at top of edit showing the tab stops
 * #define TEXT_TABS_SPACES         (1<<13)
 * #define TEXT_TABS_EVEN           (1<<14)
 * #define TEXT_TABS_STOPS          (1<<15)
 * #define TEXT_TABS_NONE           (1<<16)
 * #define TEXT_SHOWTABLINE         (1<<17)
 * 
 *  // put little divits where space/newline/tabs are
 * #define TEXT_SHOW_WHITESPACE     (1<<18)
 * 
 *  // default scrollers is for transient scrollers, special check for ALWAYS must be made in classes
 * #define TEXT_NOSCROLLERS         (1<<19)
 * #define TEXT_SCROLLERS           (1<<20 | 1<<21)
 * #define TEXT_TRANSIENT_SCROLL    (1<<20 | 1<<21)
 * #define TEXT_TRANSIENT_X         (1<<20)
 * #define TEXT_TRANSIENT_Y         (1<<21)
 * #define TEXT_XSCROLL             (1<<20)
 * #define TEXT_YSCROLL             (1<<21)
 * #define TEXT_ALWAYS_SCROLLERS    (1<<20 | 1<<21 | 1<<22 | 1<<23)
 * #define TEXT_ALWAYS_X            (1<<22 | 1<<20)
 * #define TEXT_ALWAYS Y            (1<<23 | 1<<21)
 * 
 *  // Set this if you want to be able to move the cursor in regions where
 *  // there would normally not be anything, like after the newline of a short line.
 * #define TEXT_CURS_PAST_NL        (1<<24)
 * \endcode
 *
 * \todo ***finish putting in Modified() to whatever modifies!!!
 * \todo implement max/min textlen/numlines
 */


//------------------------ TextUndo ---------------------------------
//! Class to hold info for undos in text edit boxes.
class TextUndo : public UndoData
{
  public:
	int type; //add, remove
	char *text;
	long start,end;
	TextUndo(Undoable *editor, int t,const char *str,long l, long s,long e);
	virtual ~TextUndo();
	virtual const char *Description() { return NULL; }
};

TextUndo::TextUndo(Undoable *editor, int t,const char *str,long l, long s,long e)
{
	context=editor;
	text=newnstr(str,l);
	start=s;
	end=e;
	type=t;
}

TextUndo::~TextUndo()
{
	if (text) delete[] text;
}

enum TextUndoTypes {
	TEXTUNDO_Insert,
	TEXTUNDO_Delete,
	TEXTUNDO_MAX
};

//------------------------------- Back to TextEditBaseUtf8:

//! Constructor
/*! If there is no justification set, then TEXT_LEFT is applied. If no control character scheme
 * is set, then TEXT_CNTL_BANG is applied. The default delimiter is a newline ('\\n'==ascii 0x0A).
 * Default tabs is to allow them, and have them evenly spaced at tabwidth=4.
 * 
 * ncntlchar is the UCS code for the character that shows for character that is not printable.
 * For TEXT_CNTL_BANG, this is '!'.
 * 
 * newtext is copied, and is assumed to be utf8 encoded.
 */
TextEditBaseUtf8::TextEditBaseUtf8(const char *newtext,unsigned long nstyle,unsigned int ncntlchar)
{ 			// nstyle=0, newtext=NULL, ncntlchar=0; newtext copied
	textstyle=nstyle;
	if ((textstyle&(TEXT_LEFT|TEXT_RIGHT|TEXT_CENTER))==0) textstyle|=TEXT_LEFT;
	if ((textstyle&(TEXT_CNTL_BANG|TEXT_CNTL_HEX|TEXT_CNTL_NONE))==0) textstyle|=TEXT_CNTL_BANG;
	if (textstyle&TEXT_CRLF) SetDelimiter('\r','\n');
	else if (textstyle&TEXT_CR) { SetDelimiter('\r',0); textstyle|=TEXT_NLONLY; }
	else { SetDelimiter('\n',0); textstyle|=TEXT_NLONLY; }
	if (!(textstyle&(TEXT_TABS_NONE|TEXT_TABS_STOPS|TEXT_TABS_EVEN|TEXT_TABS_SPACES))) textstyle|=TEXT_TABS_EVEN;
	if ((textstyle&(TEXT_NOSCROLLERS|TEXT_SCROLLERS|TEXT_TRANSIENT_SCROLL))==0) textstyle|=TEXT_SCROLLERS;
	if (textstyle&TEXT_TABS_EVEN) textstyle|=TEXT_TABS_STOPS;
	
	modified=0; 
	tabwidth=4;
	cutbuffer=NULL;
	thetext=NULL;
	maxlines=maxtextlen=mintextlen=minlines=0;
	if (ncntlchar<33) if (textstyle&TEXT_CNTL_BANG) cntlchar=(unsigned int) '!'; else cntlchar=(unsigned int) '\\';
	else cntlchar=ncntlchar;
	cntlmovedist=10;

	curpos=0;
	selstart=sellen=0;

	undomode=0;
	SetText(newtext);
	undomode=1;
}

//! delete[] thetext and cutbuffer.
TextEditBaseUtf8::~TextEditBaseUtf8()
{
	DBG cerr <<" ---- in texteditbase dest"<<endl;
	delete[] thetext;
	delete[] cutbuffer;
	DBG cerr <<" ----   dest done."<<endl;
}

//! For CHAR_TABS, returns the character to center on, or 0 if no such tab there.
/*! The number returned is the UCS code for the character. 
 */
int TextEditBaseUtf8::GetTabChar(int atpix)
{
	return 0;
} 

//! Redefine for custom tabstops, default is even spacing with tabwidth pixels
/*! Returns the next tab stop which is at a pixel position greater than atpix.
 */
int TextEditBaseUtf8::GetNextTab(int atpix) 
{
	return (atpix/tabwidth+1)*tabwidth;
}
	
//! Redefine for custom tabstops, default is even spacing with tabwidth pixels.
/*! This is a more detailed version of GetNextTab. The return value is still the next tab
 *  stop at pixel position greater than atpix, and also sets tabtype to the type of
 *  tab returned. The default is LEFT_TAB, but the other defined types are RIGHT_TAB,
 *  CENTER_TAB, NUMERIC_TAB, and CHAR_TAB. Note that tab types are not relevant
 *  to TextEditBaseUtf8. They are more or less implemented in TextXEditBase, in conjunction
 *  with the function GetTabChar().
 */
int TextEditBaseUtf8::GetNextTab(int atpix,int &tabtype) 
{
	tabtype=LEFT_TAB;
	return GetNextTab(atpix);
}
	
//! Return pix wide of line from p to newline
/*! NOTE that this assumes that each character takes up a certain width, which is
 * added on to the summed widths of previous characters, which is a false assumption
 * for various non-english languages!
 */
int TextEditBaseUtf8::Getpixwide(long p) 
{
	int pix=0;
	while (p<textlen && !onlf(p)) {
		pix+=charwidth(p);
		p=nextpos(p);
	}
	return pix;
}

//! Cut and copy selection to cutbuffer
int TextEditBaseUtf8::Cut()
{
	if (!sellen) return 1;
	if (cutbuffer) delete[] cutbuffer;
	cutbuffer=CutSelText();
	return 0;
}

//! Copy selection to cutbuffer
int TextEditBaseUtf8::Copy()
{
	if (!sellen) return 1;
	if (cutbuffer) delete[] cutbuffer;
	cutbuffer=GetSelText();
	return 0;
}

//! Paste cutbuffer to curpos or selection location
int TextEditBaseUtf8::Paste()
{
	if (!cutbuffer) return 1;
	if (sellen) return replacesel(cutbuffer);
	return insstring(cutbuffer);
}

//! Find str in thetext, start search at fromcurs, wrap around
/*! \todo *** should optionally do case insensitive search
 */
int TextEditBaseUtf8::Find(const char *str,int fromcurs) // fromcurs==1
{ 						  
	if (!str) return 0;
	char *it=NULL;
	if (fromcurs){
		it=strstr(thetext+curpos,str);
		if (!it) it=strstr(thetext,str); // wraps around
	} else it=strstr(thetext,str);
	if (!it) return 0;

	selstart=it-thetext;
	curpos=it-thetext+strlen(str);
	sellen=curpos-selstart;
	return 1;
}

//! Replace a range of text with a new string.
int TextEditBaseUtf8::Replace(const char *newstr,int start,int end)
{
	SetSelection(start,end);
	return replacesel(newstr);
}

/*! \todo *** this is basically unimplemented currently
 */
int TextEditBaseUtf8::Replace(const char *newstr,const char *,int which)
{ //***		     /* which: 0=only next, 1=all, -1=all in selection */
	//if (readonly()) return 1;
	return replacesel(newstr);/*****???replace all, only in sel...*/
}

//! Return a const pointer to thetext.
/*! Useful for when you just want to scan the text for some reason, and
 * not copy the whole thing.
 */
const char *TextEditBaseUtf8::GetCText()
{
	return thetext;
}

//! Return a new char[] copy of thetext
char *TextEditBaseUtf8::GetText()
{
	char *txt=NULL;
	makestr(txt,thetext);
	return txt;
}

//! Set thetext equal to newtext (copies it).
int TextEditBaseUtf8::SetText(const char *newtext)
{
	DBG if (newtext) cerr <<endl<<"TextEditBaseUtf8:: NewText"<<endl<<newtext<<endl;
	DBG else cerr <<endl<<"TextEditBaseUtf8:: NewText=NULL"<<endl;

	if (textlen) AddUndo(TEXTUNDO_Delete, thetext,textlen, 0,0);
	if (newtext) AddUndo(TEXTUNDO_Insert, newtext,strlen(newtext), 0,0);

	delete[] thetext;
	if (!newtext) {
		thetext=new char[2+TEXTMEMBLOCK];
		thetext[0]='\0';
		textlen=0;
	} else {
		textlen=strlen(newtext);
		thetext=new char[textlen+TEXTMEMBLOCK+1];
		strcpy(thetext,newtext);
	}
	maxtextmem=textlen+TEXTMEMBLOCK-1;
	curpos=sellen=0;
	return 0;
}

//! Returns new curpos.
/*! If the requested position is on the second character of a two
 * character delimiter, then curpos is set to the first character.
 *
 * If newcurpos<0 or >textlen, then set to textlen.
 */
long TextEditBaseUtf8::SetCurpos(long newcurpos)
{
	if (newcurpos<0 || newcurpos>textlen) newcurpos=textlen;
	makevalidpos(newcurpos);
	curpos=newcurpos;
	if (sellen) sellen=curpos-selstart;
	if (onlf(curpos)==2) curpos--; // curpos had to be>0
	return curpos;
}

//! Sets selstart to be newss and curpos to newse
/*! Like SetCurpos, the positions are never set to the
 *  second character of a 2 character delimiter, nor to anything but
 *  the first byte of a utf8 char.
 *
 *  If newss or newse <0, then set them to textlen.
 *
 *  Returns sellen.
 */
int TextEditBaseUtf8::SetSelection(long newss,long newse) // newss=newse=-2
{ 
	if (newss<0 || newss>textlen) newss=textlen;
	if (newse<0 || newse>textlen) newse=textlen;

	selstart=newss;
	curpos=newse;
	makevalidpos(selstart);
	makevalidpos(curpos);

	sellen=curpos-selstart;
	return sellen;
}

//! Return the next utf8 character position after l.
/*! If l happens to be in the middle of a utf8 char, return the start
 * of the next char.
 */
long TextEditBaseUtf8::nextpos(long l)
{
	if (l>=textlen) return textlen;
	if (l<0) return 0;
	if ((thetext[l]&128)==0) { //cur char is ascii
		++l;
		if (onlf(l+1)==2) ++l;
		return l;
	}
	if ((thetext[l]&192)==192) { //cur char is start of multibyte char
		l++;
	}
	 //is in middle of a utf8 char
	while (l<textlen && (thetext[l]&192)==128) l++;
	return l;
}

//! Return the utf8 character position befor l.
/*! If l happens to be in the middle of a utf8 char, return the start
 * of the character it is in.
 */
long TextEditBaseUtf8::prevpos(long l)
{
	if (l<=0) return 0;
	if (l>textlen) return textlen;
	--l;
	if ((thetext[l]&128)==0) { //prev char is ascii
		if (onlf(l)==2) --l;
		return l;
	}
	 //l is within multibyte char, move to start of the char
	while (l>0 && (thetext[l]&192)==128) l--;

//---note: if char&192!=192, then thetext is corrupted utf8
//	if ((thetext[l]&192)==192) { //l is start of multibyte char
//		l--;
//	}

	return l;
}

//! Make l point only to 1st byte of a utf8 char or 2-char delimiter.
void TextEditBaseUtf8::makevalidpos(long &l)
{
	if (l<0) { l=0; return; }
	if (l>textlen) { l=textlen; return; }
	 // make point only to 1st byte of a utf8 char.
	while (l && (thetext[l]&196)==128) l--;
	if (!(textstyle&TEXT_NLONLY)) {
		if (onlf(l)==2) l--; // l had to be>0 for only==2
	}
}

//! Return a new char[] copy of the selected text.
char *TextEditBaseUtf8::GetSelText()
{
	if (!sellen) return NULL;
	char *blah=new char[(sellen>0?sellen:-sellen)+1];
	strncpy(blah,thetext+(selstart<curpos?selstart:curpos),(sellen>0?sellen:-sellen));
	return blah;
}

//! Cut out the selected text, and return a copy to a new char[] containing the text.
/*! The calling code is responsible for calling delete[] on the returned char[]. 
 */
char *TextEditBaseUtf8::CutSelText() // cuts to char *, not clipboard
{
	if (!sellen) return NULL;
	char *t=GetSelText();
	delsel();
	return t;
}

//! Get information about the selection positions
/*! sels gets assigned to the selstart. sele gets curpos.
 *  Returns the length of the selection. 
 *  Note that this return value is bytes long, not characters long.
 */
long TextEditBaseUtf8::GetSelection(long &sels,long &sele)
{
	if (sellen) { sels=selstart; sele=curpos; return sellen; }
	sels=sele=curpos;
	return 0;
}

//! Returns if the text is readonly at the given position.
/*! If pos<0, then curpos should be assumed.
 *
 * Default is to return (textstyle&TEXT_READONLY).
 */
int TextEditBaseUtf8::readonly(long) // pos==-1
{ 
//	if (pos<0) pos=-1;
	if (textstyle&TEXT_READONLY) return 1;
	return 0;
}

//! Extend the memory allocated for thetext.
int TextEditBaseUtf8::extendtext() 
{ 
	int imax=(int) maxtextmem;
	extendstr(thetext,imax,TEXTMEMBLOCK); //***
	maxtextmem=imax;
	if (thetext) return 0;
	return 1;
}

//! Insert character at curpos. Afterward, make curpos on it (a==0) or after (a==1) it. Ignores selection, always at curpos.
/*! if ch=='\\n' then put in the delimiter.
 *
 * Return nonzero if nothing changed, and 0 if something changed.
 */
int TextEditBaseUtf8::inschar(unsigned int ucs,char a) //a=1
{
	if (readonly()) return 1;
	if (textlen+5>maxtextmem) extendtext();
	if (ucs==(unsigned int)'\n' && !(textstyle&TEXT_NLONLY)) {
		memmove(thetext+curpos+2,thetext+curpos,textlen-curpos+1);
		textlen+=2;
		thetext[curpos]=newline; thetext[curpos+1]=newline2;
		
		AddUndo(TEXTUNDO_Insert, thetext+curpos,2, curpos,0);

		if (a) curpos+=2;
	} else {
		int clen=utf8bytes(ucs);
		memmove(thetext+curpos+clen,thetext+curpos,textlen-curpos+1);
		textlen+=clen;
		utf8encode(ucs,thetext+curpos);

		AddUndo(TEXTUNDO_Insert, thetext+curpos,clen, curpos,0);

		if (a) curpos+=clen;
	}
	Modified();
	return 0;
}

//! Delete:  bksp==1 means delete curpos-1, 0 del curpos
/*! Return nonzero if nothing changed, and 0 if something changed.
 *
 * Ignores the selection.
 */
int TextEditBaseUtf8::delchar(int bksp) 
{
	if (readonly()) return 1;
	 //position curpos to the start of the char being deleted
	if (bksp) {
		if (curpos==0) return 0;
		curpos=prevpos(curpos);
	} else {
		if (curpos==textlen) return 0;	
	}
	int clen=nextpos(curpos)-curpos;

	AddUndo(TEXTUNDO_Delete, thetext+curpos,clen, curpos,curpos+clen);

	memmove(thetext+curpos,thetext+curpos+clen,textlen-curpos-clen+1);
	textlen-=clen;
	Modified();
	return 0;
}

//! Insert a string of text. after!=0 makes cursor jump to after the insert.
/*! \todo should probably validate blah. currently it is assumed that blah
 *    is valid utf8.
 */
int TextEditBaseUtf8::insstring(const char *blah,int after) // after==0
{
	if (!blah) return 1;

	//**** validate blah

	long l=strlen(blah);
	while (textlen+l>=maxtextmem) extendtext();
	memmove(thetext+curpos+l,thetext+curpos,textlen-curpos+1);
	memcpy(thetext+curpos,blah,l);
	textlen+=l;

	AddUndo(TEXTUNDO_Insert, blah,l, curpos,0);

	if (after) curpos+=l;
	Modified();
	return 0;
}

//! Delete [selstart,curpos], but only if sellen>0
/*! Return nonzero for nothing changed, or 0 for selection deleted.
 */
int TextEditBaseUtf8::delsel()
{
	if (!sellen) return 1;
	long rbegin,rend; // delete [rbegin,rend)
	if (curpos<selstart) { rbegin=curpos; rend=selstart; }
	   else { rbegin=selstart; rend=curpos; curpos=rbegin; }
	if (sellen<0) sellen=-sellen;

	AddUndo(TEXTUNDO_Delete, thetext+rbegin,rend-rbegin, rbegin,rend);

	memmove(thetext+rbegin,thetext+rend, textlen-rend+1);

	textlen-=sellen;
	if (curpos>textlen) curpos=textlen;
	sellen=0;
	Modified();
	return 0;
}

//! Replace the selection with character ch.
int TextEditBaseUtf8::replacesel(unsigned int ucs)
{ 
	char ch[5];
	ch[utf8encode(ucs,ch)]='\0';
	return replacesel(ch,1);
}

//! Replace the selection with utf8 encoded newt, placing cursor at beginning of newt (after=0) or after it.
/*! Return 0 for thetext changed, else nonzero.
 */
int TextEditBaseUtf8::replacesel(const char *newt,int after) // after=0, newt=NULL deletes
{
	if (!newt) return delsel();
	if (!sellen) return insstring(newt,after);

	long l=strlen(newt),rbegin,rend; //replace [rstart,rend)
	if (curpos<selstart) { rbegin=curpos; rend=selstart; }
	   else { rbegin=selstart; rend=curpos; curpos=rbegin; }
	if (sellen<0) sellen=-sellen;
	while (textlen+l>=maxtextmem) extendtext();

	AddUndo(TEXTUNDO_Delete, thetext+rbegin,rend-rbegin+1, rbegin,rend);
	AddUndo(TEXTUNDO_Insert, newt,l, rbegin,0);

	memmove(thetext+rbegin+l,thetext+rend,textlen-rend+1);
	memcpy(thetext+rbegin,newt,l);
	textlen+=l-sellen;
	if (after) curpos=rbegin+l; 
	if (curpos>textlen) curpos=textlen;
	sellen=0;
	Modified();
	return 0;
}

//! Find the start and end of a word that includes position pos.
/*! Return 1 for word found, 0 for not found.
 */
int TextEditBaseUtf8::findword(long pos,long &start, long &end) //pos=-1
{  	// ' ab-' -> start='a' end='b' 
	if (pos<0) pos=curpos;
	if (!isword(pos)) return 0;
	start=end=pos;
	while (start>0 && isword(prevpos(start))) start=prevpos(start);
	while (end<textlen && isword(nextpos(end))) end=nextpos(end);
	return 1; 
}

//! Return if character at pos is alphanumeric character.
/*! This is used in cntl-left/right to hop over words. 
 */
int TextEditBaseUtf8::isword(long pos) //pos=-1
{ 
	if (pos<0) pos=curpos;
	int len;
	return isalnum(utf8decode(thetext+pos,thetext+textlen,&len)); 
}

//! Return first place after the first newline before pos
long TextEditBaseUtf8::findlinestart(long pos) // pos=-1
{
	if (pos<0) pos=curpos;
	if (pos>textlen) pos=textlen;
	while (pos>0 && !onlf(pos-1)) pos=prevpos(pos);
	return pos;
}

//! Return how many newlines before pos.
/*! defaults to count from beginning,first line==0, one line per newline 
 *
 * \todo *** perhaps remove this from this class, it is not used here, and a 'line'
 *   is perhaps too much a many splendored thing. besides as implemented here is
 *   very inefficient.
 */
long TextEditBaseUtf8::WhichLine(long pos)
{
	 //rather inefficient
	int l=0;
	while (pos>0) if (onlf(pos--)==1) l++;
	return l;
}

//! Returns most characters wide in all of lines of thetext.
long TextEditBaseUtf8::Getcharswide()  /* # chars, not actual width */
{
	long temp=0,temp2=0;
	long width=0,cw;
	while (temp<textlen) {
		 //count temp2 out to newline
		temp2=temp;
		while (!onlf(temp2) && thetext[temp2]!='\0') temp2=nextpos(temp2);
		cw=temp2-temp;
		if (cw>width) width=cw;
		if (temp2==textlen) break;
		temp=nextpos(temp2);
	}
	return width;
}

//! Return the number of newlines in the whole buffer.
long TextEditBaseUtf8::Getnumlines()
{ return Getnlines(0,textlen-1); }

//! Return the number of newlines in range [s,e]
/*! Only counts chars matching the first delimiter. */
long TextEditBaseUtf8::Getnlines(long s,long e) // s=e=-1
{
	if (textlen==0) return 0;
	if (sellen) {
		if (s==-1) s=selstart;
		if (e==-1) e=curpos;
	} else {
		if (s<0) s=0;
		if (s>textlen) s=textlen;
		if (e<0) e=0;
		if (e>textlen) e=textlen;
	}
	if (s<e) { long t=s; s=e; e=t; }
	long n=0;
	for (long c=s; c<=e; c=nextpos(c)) if (onlf(c)==1) n++;
	return n;
}

//! Set the newline delimiter to "n1n2". n2==0 means use single character newline.
/*! Return 0 for not changed, and 1 for changed.
 * 
 * The delimiter can be 1 or 2 consecutive ASCII characters.
 */
int TextEditBaseUtf8::SetDelimiter(char n1,char n2) // n2=0
{
	if (n1==0) return 0;
	if (n2==0) textstyle|=TEXT_NLONLY; else textstyle&=~TEXT_NLONLY;
	newline=n1;
	newline2=n2;
	return 1;
}

//! Return whether pos is on a delimiter. 0==no, 1==first delimiter, 2==second delimeter
int TextEditBaseUtf8::onlf(long pos) // pos=-1
{
	if (pos<0) pos=curpos;
	if (pos>=textlen) return 0;
	if (textstyle&TEXT_NLONLY) return thetext[pos]==newline?1:0;
	if (thetext[pos]==newline2) {
		if (pos>0 && thetext[pos-1]==newline) return 2;
		else return 0;
	}
	if (thetext[pos]==newline) {
		if (pos<textlen && thetext[pos+1]==newline2) return 1;
	}
	return 0;
}


//---------------Undo functions:

/*! Return 1 for added, or 0 for ignoring add, for instance was in middle of an undo.
 */
int TextEditBaseUtf8::AddUndo(int type, const char *str,long len, long start,long end)
{
	if (undomode==0) return 0;
	undomanager.AddUndo(new TextUndo(this, type,str,len,start,end));
	return 1;
}

int TextEditBaseUtf8::Undo()
{ return undomanager.Undo(); }

int TextEditBaseUtf8::Redo()
{ return undomanager.Redo(); }

int TextEditBaseUtf8::Undo(UndoData *data)
{
	if (!data) return 1;
	TextUndo *t=dynamic_cast<TextUndo*>(data);
	if (!t) return 2;

	if (t->type==TEXTUNDO_Insert) {
		 //need to delete
		selstart=t->start;
		sellen=strlen(t->text);
		curpos=selstart+sellen;
		undomode=0;
		delsel();
		undomode=1;
		return 0;

	} else if (t->type==TEXTUNDO_Delete) {
		 //need to insert
		curpos=t->start;
		sellen=0;
		undomode=0;
		insstring(t->text,1);
		undomode=1;
		return 0;
	}

	return 3;
}

int TextEditBaseUtf8::Redo(UndoData *data)
{
	if (!data) return 1;
	TextUndo *t=dynamic_cast<TextUndo*>(data);
	if (!t) return 2;

	if (t->type==TEXTUNDO_Insert) {
		if (sellen) sellen=0;
		undomode=0;
		insstring(t->text,1);
		undomode=1;
		return 0;

	} else if (t->type==TEXTUNDO_Delete) {
		selstart=t->start;
		sellen=t->end-t->start;
		curpos=selstart+sellen;
		undomode=0;
		delsel();
		undomode=1;
		return 0;
	}

	return 3;
}

} // namespace Laxkit

