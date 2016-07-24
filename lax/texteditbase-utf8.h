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
//    Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef _LAX_TEXTEDITBASE_UTF8_H
#define _LAX_TEXTEDITBASE_UTF8_H


#include <lax/undo.h>
#include <fstream>

#define LEFT_TAB                 (1<<0)
#define CENTER_TAB               (1<<1)
#define RIGHT_TAB                (1<<2)
#define CHAR_TAB                 (1<<3)
#define NUMERIC_TAB              (1<<3)

#define TEXT_LEFT                (1<<0)
#define TEXT_CENTER              (1<<1)
#define TEXT_RIGHT               (1<<2)
#define TEXT_JUSTIFY             (1<<3)
#define TEXT_JUSTIFY_RIGHT       (1<<2 | 1<<3)
#define TEXT_JUSTIFY_CENTER      (1<<1 | 1<<3)
#define TEXT_FORCEJUSTIFY        (1<<4)

#define TEXT_WORDWRAP            (1<<5)
#define TEXT_READONLY            (1<<6)

#define TEXT_NLONLY              (1<<7)
#define TEXT_CRLF                (1<<9)
#define TEXT_LF                  (1<<7)
#define TEXT_CR                  (1<<8)

 // write non-chars as cntl char (such as space, '.','\' etc), cntlchar+hex, none
#define TEXT_CNTL_BANG           (1<<10)
#define TEXT_CNTL_HEX            (1<<11)
#define TEXT_CNTL_NONE           (1<<12)

 // SHOWTABLINE means have a line at top of edit showing the tab stops
 // TEXT_TABS_STOPS must be set for GetNextTab to be called
#define TEXT_TABS_SPACES         (1<<13)
#define TEXT_TABS_EVEN           (1<<14)
#define TEXT_TABS_STOPS          (1<<15)
#define TEXT_TABS_NONE           (1<<16)
#define TEXT_SHOWTABLINE         (1<<17)

 // put little divits where space/newline/tabs are
#define TEXT_SHOW_WHITESPACE     (1<<18)

 // default scrollers is for transient scrollers, special check for ALWAYS must be made in classes
#define TEXT_NOSCROLLERS         (1<<19)
#define TEXT_SCROLLERS           (1<<20 | 1<<21)
#define TEXT_TRANSIENT_SCROLL    (1<<20 | 1<<21)
#define TEXT_TRANSIENT_X         (1<<20)
#define TEXT_TRANSIENT_Y         (1<<21)
#define TEXT_XSCROLL             (1<<20)
#define TEXT_YSCROLL             (1<<21)
#define TEXT_ALWAYS_SCROLLERS    (1<<20 | 1<<21 | 1<<22 | 1<<23)
#define TEXT_ALWAYS_X            (1<<22 | 1<<20)
#define TEXT_ALWAYS_Y            (1<<23 | 1<<21)

#define TEXT_CURS_PAST_NL        (1<<24)
#define TEXT_LINE_NUMBERS        (1<<25)


namespace Laxkit {


class TextEditBaseUtf8 : virtual public Undoable
{
  protected:
	char *thetext;
	long textlen,maxtextmem;
	unsigned long textstyle;

	long curline,curpos,selstart,sellen;
	long modpos;
	int cntlmovedist;
	int tabwidth;
	char *cutbuffer;
	char newline,newline2;
	char cntlchar;
	char modified;
	long maxtextlen,mintextlen, maxcharswide,mincharswide, maxlines,minlines; // these must be implemented in derived classes
	virtual int extendtext(); // overkill?
	virtual long nextpos(long l);
	virtual long prevpos(long l);
	virtual void makevalidpos(long &l);

	friend class UndoManager;
	int undomode; //1 for add undos, or 0 for ignore undos
	UndoManager undomanager;
	virtual int Undo(UndoData *data); //called by undomanager
	virtual int Redo(UndoData *data);
	virtual int AddUndo(int type, const char *str,long len, long start,long end);

  public:
	TextEditBaseUtf8(const char *newtext=NULL,unsigned long nstyle=0,unsigned int ncntlchar=0);
	virtual ~TextEditBaseUtf8();
	virtual int charwidth(unsigned int usc,int actual=0) { return 1; }
	virtual int GetTabChar(int atpix);
	virtual int GetNextTab(int atpix); // redefine for custom tabstops, default is even spacing with tabwidth pixels
	virtual int GetNextTab(int atpix,int &tabtype); // defaults to LEFT_TAB, calls GetNextTab(atpix)

	virtual int inschar(unsigned int ucs,char a=1); // a==1 means put curpos after the insert
	virtual int insstring(const char *blah,int after=0);
	virtual int delsel();
	virtual int delchar(int bksp); // bksp=1 del curpos-1, 0 del curpos 
	virtual int replacesel(unsigned int ucs);
	virtual int replacesel(const char *newt=NULL,int after=0); // after=0, newt=NULL deletes
	virtual int onlf(long pos=-1);
	virtual int findword(long pos,long &start, long &end);
	virtual int isword(long pos=-1); 
	virtual long findlinestart(long pos=-1); 
	virtual int CombineChars();
	
	virtual int readonly(long pos=-1);
	virtual int SetText(const char *newtext);
	//virtual int SetText(std::ifstream &fromfile,long len); // file must be open, len must be accurate 
	virtual const char *GetCText();
	virtual char *GetText();
	virtual char *GetSelText();
	virtual char *CutSelText(); // cuts to char *, not clipboard
	virtual long GetSelection(long &sels,long &sele);
	virtual int SetSelection(long newss=-2,long newse=-2); // if e==-1, sets e=textlen,e==-2 sets sels=curpos, curpos+=s
	virtual long GetNumLines();
	virtual long Getnlines(long s=-1,long e=-1);
	virtual long Getcharswide();  // total # chars with letter,nonprint,tabs,etc, not actual width
	virtual int Getpixwide(long linenum); // return pix wide of line
	virtual int Modified(int m=1);

	virtual int Cut(); // default is for internal cutbuffer
	virtual int Copy();
	virtual int Paste();
	virtual int Undo(); //called by anyone
	virtual int Redo();

	virtual long WhichLine(long pos);
	virtual long GetCurLine() { return 0; }
	virtual long SetCurLine(long nline) { return 0; }
	virtual long GetCurpos() { return curpos; }
	virtual long SetCurpos(long ncurpos); // returns actual new curpos
	virtual int SetDelimiter(char n1,char n2=0);

	virtual int Find(const char *str,int fromcurs=1);
	virtual int Replace(const char *newstr,const char *,int which);
	virtual int Replace(const char *newstr,int start,int end);
};

} // namespace Laxkit


#endif

