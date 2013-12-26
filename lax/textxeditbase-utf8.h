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
#ifndef _LAX_TEXTXEDITBASE_UTF8_H
#define _LAX_TEXTXEDITBASE_UTF8_H



#include <lax/anxapp.h>
#include <lax/texteditbase-utf8.h>
#include <lax/rectangles.h>

#include <lax/fontmanager.h>


namespace Laxkit {


class TextXEditBaseUtf8 : public anXWindow, public TextEditBaseUtf8
{
 protected:
	int cx,cy,oldx,oldy,curlineoffset;
	int padx,pady;
 	char firsttime,con;
 	long dpos,nlines;
	long oldsellen,oldcp;
	int textascent,textheight,textdescent;
	unsigned long curtextcolor,textbgcolor;
	unsigned long curbkcolor,bkwrongcolor,wscolor;
 	LaxFont *thefont;
	int valid;
	DoubleRectangle textrect;
	virtual void docaret(int w=1);
	virtual void settextrect();
	virtual int selectionDropped(unsigned char *data,unsigned long len,Atom actual_type,Atom which);
	virtual char *getSelectionData(int *len,Atom property,Atom targettype,Atom selection);
 public:
	TextXEditBaseUtf8(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
							 int xx,int yy,int ww,int hh,int brder,
							 anXWindow *prev,unsigned long nowner,const char *nsend,
							 const char *newtext=NULL,unsigned long ntstyle=0,int ncntlchar=0);
	virtual ~TextXEditBaseUtf8();
	virtual int init();
	virtual void Refresh();
	virtual int MBUp(int x,int y,unsigned int state, const LaxMouse *d);
	virtual int FocusOn(const FocusChangeData *e);
	virtual int FocusOff(const FocusChangeData *e);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);

	virtual int charwidth(int ch,int r=0);
	virtual int Cut(); 
	virtual int Copy();
	virtual int Paste();
	virtual void Colors(int hl);
	virtual void Black(int x,int y,int w,int h);
	virtual int DrawTabLine();
	virtual void DrawCaret(int flag=0,int on=1); // flag=0,on=1 
	virtual void DrawText(int black=-1) = 0;
	virtual int DrawLineOfText(int x,int y,long pos,long len,char &check,long eof=-1);
	virtual int TextOut(int x,int y,char *str,long len,long eof);
	virtual int ExtentAndStr(char *str,long len,char *&blah,long &p);
	virtual int GetExtent(long pos,long end,int lsofar=0,long eof=-1); //lsofar=0,eof=-1
	virtual long GetPos(long pos,int pix,int lsofar=0,long eof=-1); //lsofar=0,eof=-1
	virtual int SetupMetrics();
	virtual int UseThisFont(LaxFont *newfont);
};

} // namespace Laxkit



#endif
