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
#ifndef _LAX_LABEL_H
#define _LAX_LABEL_H

#include <lax/anxapp.h>
#include <lax/buttondowninfo.h>

namespace Laxkit {

enum class LabelTypes {
	LEFT           =(1<<0),
	RIGHT          =(1<<1),
	CENTERX        =(1<<2),
	CENTERY        =(1<<3),
	CENTER         =(1<<2 | 1<<3),
	TOP            =(1<<4),
	BOTTOM         =(1<<5),
	INLINE_TOP,
	INLINE_CENTER,
	INLINE_BOTTOM,
	MOVE           =(1<<6),
	COPY           =(1<<7),
	WRAP           =(1<<8),
	LEAVE_DESTROYS =(1<<9),
	BINARY         =(1<<10),
	EXPAND_TO_SIZE =(1<<11)
	
	MAX_BIT        = 11
};


class LabelChunk : public DoubleBBox
{
  public:
  	enum ContentType { Text, Image, Graphic };
	ContentType               content_type = Text;
	int                       info    = 0;
	std::vector<unsigned int> text;             // ucs32 glyph indices
	std::vector<GlyphPlace>   glyphs;  // glyph placement, [x y x y x y ...]
	LaxFont *                 font  = nullptr;  // parent owns ref
	ScreenColor *             color = nullptr;  // assume parent owns this ref
	LaxImage *                img   = nullptr;

	LabelChunk();
	virtual ~LabelChunk();

	//static DecomposeText(const char *text, PtrStack<LabelChunk> &chunks);
};


class Label : public anXWindow
{
  protected:
  	unsigned long label_style;
	int firsttime;
	ButtonDownInfo buttondown;

	Utf8String rawtext;
	PtrStack<LabelChunk> thetext;
	int nlines;
	GradientStrip *colors; //if null, window colors are used
	PtrStack<LaxFont> fonts;
	PtrStack<LaxImage> images;

	double ex,ey,fasc,fdes,height; // ex,ey are extents
	int offset_x, offset_y;

	int current_color; // index in colors
	int current_font;  // index in fonts

  public:
	int padx,pady;

	Label();
	Label(anXWindow *pwindow,
				const char *nname,
				const char *ntitle,
				unsigned long window_flags, int nx,int ny,int nw,int nh,int brder,
				const char *newtext, unsigned long label_flags,
				int pad_x=0, int pad_y=0);
	virtual ~Label();

	virtual int SetText(const char *newtext);
	virtual int SetupMetrics();
	virtual char *GetText();
	virtual char *GetTaggedText();
	virtual const char *GetConstText();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual void Refresh();
	virtual const char *whattype() { return "Label"; } 
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);

	 //serializing aids
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
};

} // namespace Laxkit

#endif

