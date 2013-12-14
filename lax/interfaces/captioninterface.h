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
//    Copyright (C) 2013 by Tom Lechner
//
#ifndef _LAX_CAPTIONINTERFACE_H
#define _LAX_CAPTIONINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/laximages.h>



namespace LaxInterfaces {

//--------------------------------- CaptionData -------------------------------
class CaptionData : public SomeData
{
 public:
	int state; //1 if lengths found

	char *fontname, *fontstyle, *fontfile;
	double fontsize;
	Laxkit::LaxFont *font;
	double red,green,blue,alpha; //[0..1]
	double xcentering,ycentering; //around origin

	Laxkit::PtrStack<char> lines;
	Laxkit::NumStack<double> linelengths;

	virtual const char *whattype() { return "CaptionData"; }
	CaptionData(const char *ntext, const char *nfontname, const char *nfontstyle, int fsize, double xcenter, double ycenter);
	virtual ~CaptionData();
	virtual int SetText(const char *newtext);
	virtual double XCenter(double xcenter);
	virtual double XCenter() { return xcentering; }
	virtual double YCenter(double ycenter);
	virtual double YCenter() { return ycentering; }
	virtual void FindBBox();

	virtual int CharLen(int line);
	virtual int ComputeLineLen(int line);
	virtual int DeleteChar(int line,int pos,int after, int *newline,int *newpos);
	virtual int InsertChar(unsigned int ch, int line,int pos, int *newline,int *newpos);
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::anObject *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,Laxkit::anObject *context);
};

//--------------------------------- CaptionInterface -------------------------------
class CaptionInterface : public anInterface
{
 protected:
	int mode,mousedragged;
	flatpoint leftp;
	int mx,my,lx,ly;
	int caretline,caretpos;
 public:
	int showdecs;
	CaptionData *data;
	ObjectContext *coc;

	CaptionInterface(int nid,Laxkit::Displayer *ndp, const char *newtext);
	virtual ~CaptionInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Caption"; }
	virtual const char *Name();
	virtual const char *whattype() { return "CaptionInterface"; }
	virtual const char *whatdatatype() { return "CaptionData"; }
	virtual void deletedata();
	virtual ObjectContext *Context();
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);
	virtual CaptionData *newData();
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
};


} // namespace LaxInterfaces

#endif

