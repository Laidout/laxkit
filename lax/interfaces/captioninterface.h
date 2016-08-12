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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_CAPTIONINTERFACE_H
#define _LAX_CAPTIONINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/colors.h>
#include <lax/laximages.h>
#include <lax/fontmanager.h>

#include <lax/refptrstack.h>


namespace LaxInterfaces {


//--------------------------------- CaptionData -------------------------------

class CaptionData : virtual public SomeData
{
  public:
	int state; //1 if lengths found

	char *fontfamily;
	char *fontstyle;
	char *fontfile;

	char *language; //id matching something in fontmanager
	char *script;
	int direction; //rtl, ltr, ttb, btt? uses LAX_LRTB stuff

	double fontsize; //font height, font's default distance between baselines
	double linespacing; //percentage of font's default
	double xcentering;
	double ycentering; //around origin
	int baseline_hint; //for baseline justify. -1 means normal ycentering

	Laxkit::Color *color;
	double red,green,blue,alpha; //[0..1]

	Laxkit::LaxFont *font;

	Laxkit::PtrStack<char> lines;
	Laxkit::NumStack<double> linelengths;
	
	bool needtorecache;
	class Linestat
	{
	  public:		
		int needtorecache;
		long start, len;
		double pixlen;
		double indent; //from minx, or miny if vertical writing
		double height;

		 // Get glyph information and positions out of the buffer.
		Laxkit::GlyphPlace *glyphs;
		int numglyphs;
	
		Linestat(long nstart, long n, double npixlen, double nindent, double nh)
			{ glyphs=NULL; numglyphs=0; needtorecache=1; Set(nstart,n,npixlen,nindent,nh); }
		virtual ~Linestat() { delete[] glyphs; }
		void Set(long nstart, long n, double npixlen, double nindent, double nh)
			{ start=nstart; len=n; pixlen=npixlen; indent=nindent; height=nh; }
	};
	Laxkit::PtrStack<Linestat> linestats;


	virtual const char *whattype() { return "CaptionData"; }
	CaptionData();
	CaptionData(const char *ntext, const char *nfontfamily, const char *nfontstyle, int fsize, double xcenter, double ycenter);
	virtual ~CaptionData();
	virtual SomeData *duplicate(SomeData *dup);

	virtual int SetText(const char *newtext);
	virtual char *GetText();
	virtual bool IsBlank();
	virtual int NumLines() { return lines.n; }
	virtual double XCenter(double xcenter);
	virtual double XCenter() { return xcentering; }
	virtual double YCenter(double ycenter);
	virtual double YCenter() { return ycentering; }
	virtual double BaselineJustify(int line);
	virtual double Size(double newsize);
	virtual double Size() { return fontsize; }
	virtual double MSize() { return font->Msize(); }
	virtual double LineSpacing(double newspacing);
	virtual double LineSpacing() { return linespacing; }
	virtual int FindPos(double y, double x, int *line, int *pos);
	virtual void FindBBox();
	virtual int RecacheLine(int line);


	virtual int Font(Laxkit::LaxFont *newfont);
	virtual int Font(const char *file, const char *family,const char *style,double size);

	virtual int CharLen(int line);
	virtual double ComputeLineLen(int line);
	virtual int DeleteChar(int line,int pos,int after, int *newline,int *newpos);
	virtual int InsertChar(unsigned int ch, int line,int pos, int *newline,int *newpos);
	virtual int DeleteSelection(int fline,int fpos, int tline,int topos, int *newline,int *newpos);
	virtual int InsertString(const char *txt,int len, int line,int pos, int *newline,int *newpos);

//	virtual SomeData *ConvertToPaths(bool use_clones, SomeData *clones_to_add_to);
	virtual SomeData *ConvertToPaths(bool use_clones, Laxkit::RefPtrStack<SomeData> *clones_to_add_to);
	
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


//--------------------------------- CaptionInterface -------------------------------

enum CaptionInterfaceActions {
	CAPTION_None=0,
	CAPTION_Move,
	CAPTION_Rotate,
	CAPTION_Text,
	CAPTION_HAlign,
	CAPTION_VAlign,
	CAPTION_Size,
	CAPTION_Line_Spacing,

	CAPTION_Copy,
	CAPTION_Paste,
	CAPTION_LeftJustify,
	CAPTION_CenterJustify,
	CAPTION_RightJustify,
	CAPTION_TopJustify,
	CAPTION_MiddleJustify,
	CAPTION_BaselineJustify,
	CAPTION_BottomJustify,
	CAPTION_Direction,
	CAPTION_Decorations,
	CAPTION_ShowBaselines,
	CAPTION_InsertChar,
	CAPTION_CombineChars,
	CAPTION_Convert_To_Path,
	CAPTION_Lorem_Ipsum,

	CAPT_MAX
};


class CaptionInterface : public anInterface
{
  protected:
	int mode,mousedragged;
	flatpoint leftp;
	int caretline,caretpos;
	int scaretline,scaretpos, sellen;
	flatpoint caret;
	int lasthover;
	Laxkit::ShortcutHandler *sc;

	 // *** temporary:
	void TextOutGlyphs(int line, double x,double y, bool show_caret);

  public:
	double defaultscale;
	double defaultsize;
	double defaultspacing;
	char *defaultfamily;
	char *defaultstyle;

	double grabpad;
	int showobj;
	int showdecs;
	int showbaselines;
	unsigned long baseline_color;
	CaptionData *data;
	ObjectContext *coc;
	ObjectContext *extrahover;

	CaptionInterface(int nid,Laxkit::Displayer *ndp);
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
	virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Event(const Laxkit::EventData *e_data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
    virtual int PerformAction(int action);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Paste(const char *txt,int len, Laxkit::anObject *obj, const char *formathint);

	virtual void FixCaret();
	virtual int scan(int x,int y,unsigned int state, int *line, int *pos);
	virtual CaptionData *newData();
};


} // namespace LaxInterfaces

#endif

