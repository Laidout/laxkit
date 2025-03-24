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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_TEXTSTREAMINTERFACE_H
#define _LAX_TEXTSTREAMINTERFACE_H


#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/pathinterface.h>


namespace LaxInterfaces { 


enum TextStreamActions {
	TXT_None = 0,
	TXT_Hover_Outside,
	TXT_Hover_Outside_Stroke,
	TXT_Hover_Inside,
	TXT_Hover_Inside_Stroke,
	TXT_Hover_Stroke,
	TXT_Hover_Area,
	TXT_Hover_New,
	TXT_Offset,
	TXT_Position,

	TXT_Font,
	TXT_MAX
};

enum TextStreamToolFlags {
	TXT_On_Stroke      = (1<<0),
	TXT_In_Area        = (1<<1),
	TXT_Draggable_Area = (1<<2),
	TXT_FLAGS_MAX
};

class TextStreamInterface : public anInterface
{
  protected:
	int showdecs;
	ObjectContext *extrahover;
	int extra_hover; // what is hovered, see TXT_Hover_*
	Laxkit::flatpoint hover_point; // where hovered, snaps to path
	Laxkit::flatpoint hover_direction; // for text on path target
	double hover_baseline; // fraction of fontheight to offset text on path
	double hover_t; // t value along path for current path on text

	Laxkit::flatpoint flowdir; // default forward text direction for areas
	double fontheight; // use this hint when default_font is null
	Laxkit::LaxFont *default_font;
	Laxkit::Utf8String sample_text;

	PathsData outline;
	int outline_index;

	double close_dist;

	bool search_on_outline = true;
	bool search_in_object = true;

	Laxkit::ShortcutHandler *sc;

	virtual int send();
	virtual int scanInCurrent(int x,int y,unsigned int state, int &index, Laxkit::flatpoint &hovered, float &hovered_t);
	virtual int Track(ObjectContext *oc);
	virtual int DefineOutline(int which);

  public:
	unsigned int tstream_style;

	TextStreamInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~TextStreamInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "TextStream"; }
	const char *Name();
	const char *whattype() { return "TextStreamInterface"; }
	const char *whatdatatype();
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	//virtual int MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();

	virtual void FlowDir(Laxkit::flatpoint dir);
	virtual void FontHeight(double height);
};

} // namespace LaxInterfaces

#endif

