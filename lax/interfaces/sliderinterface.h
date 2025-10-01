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
//    Copyright (C) 2024 by Tom Lechner
//
#ifndef _LAX_SLIDERINTERFACE_H
#define _LAX_SLIDERINTERFACE_H


#include <lax/singletonkeeper.h>
#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 


//--------------------------- SliderData -------------------------------------

class SliderInfo : virtual public Laxkit::anObject, virtual public Laxkit::DoubleBBox
{
  public:
  	double min = 0;
  	double max = 1;
  	double default_v = 0;
  	double current = 0;
  	Laxkit::Utf8String label;
  	Laxkit::Utf8String id; // this is send in Modified() messages as event->str

  	bool is_screen_space = true;
  	Laxkit::flatvector from, to;

  	double line_width;
  	double outline_width;
  	double graphic_size;

  	Laxkit::LaxImage *img;
  	int graphic;
  	int graphic_fill_type; // 0=stroke, 1=fill, 2=stoke+fill
  	Laxkit::ScreenColor graphic_fill;
  	Laxkit::ScreenColor graphic_stroke;
  	Laxkit::ScreenColor line_fill;
  	Laxkit::ScreenColor line_stroke;

	SliderInfo();
	virtual ~SliderInfo();

	virtual const char *whattype() { return "SliderInfo"; }
	virtual void FindBBox();
	virtual Laxkit::anObject *duplicate();

	void CopyStyle(const SliderInfo *info);
};


//--------------------------- SliderInterface -------------------------------------

class SliderToolSettings : public Laxkit::anObject
{
  public:
	SliderInfo *default_style = nullptr;
	virtual ~SliderToolSettings();
};

class SliderInterface : public anInterface
{
	static Laxkit::SingletonKeeper settingsObject; // static so that it is easily shared between all tool instances

  protected:
	int showdecs;

	Laxkit::ShortcutHandler *sc;

	SliderInfo *info;

	SliderToolSettings *settings;

	int hover;
	virtual int scan(int x, int y, unsigned int state, double *pos_ret);

	virtual Laxkit::EventData *BuildModifiedMessage(int level);

  public:
	enum SliderActions {
		SLIDER_None = 0,
		SLIDER_Rail,
		SLIDER_Ball,
		SLIDER_MAX
	};

	unsigned int interface_flags;

	SliderInterface(anInterface *nowner = nullptr, int nid = -1, Laxkit::Displayer *ndp = nullptr);
	virtual ~SliderInterface();
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual const char *IconId() { return "Slider"; }
	virtual const char *Name();
	virtual const char *whattype() { return "SliderInterface"; }
	virtual const char *whatdatatype();
	//virtual ObjectContext *Context(); 
	//virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	//virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);
	virtual void deletedata();
	virtual SliderInfo *newData();
	virtual SliderInfo *GetInfo() { return info; }

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	//virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int info);
	virtual void MouseOut();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	// virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	// virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	//virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();
	
	virtual int Refresh();
	virtual void DrawLine();
	virtual void DrawLabel();
	virtual void DrawBall();
	virtual void SetDefaultStyle();
};

} // namespace LaxInterfaces

#endif

