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
//    Copyright (C) 2021 by Tom Lechner
//
#ifndef _LAX_MIRRORINTERFACE_H
#define _LAX_MIRRORINTERFACE_H

#include <lax/utf8string.h>
#include <lax/singletonkeeper.h>
#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 


//--------------------------- MirrorData -------------------------------------

class MirrorData : virtual public SomeData
{
  public:
	Laxkit::flatpoint p1, p2;
	bool merge;
	double merge_threshhold;
	bool cut_at_mirror; //like Blender's bisect
	bool flip_cut; //like Blender's flip
	Laxkit::Utf8String label;
	Laxkit::ScreenColor color;

	MirrorData();
	virtual ~MirrorData();

	virtual const char *whattype() { return "MirrorData"; }
	virtual void FindBBox();
	virtual SomeData *duplicate(SomeData *dup);

};


//--------------------------- MirrorInterface -------------------------------------

class MirrorToolSettings : public Laxkit::anObject
{
  public:
	Laxkit::ScreenColor knob;
	Laxkit::ScreenColor line;
	double line_width;
	double knob_size;
	bool show_labels;

	MirrorToolSettings();
};

class MirrorInterface : public anInterface
{
	static Laxkit::SingletonKeeper settingsObject;

  protected:
	int showdecs;

	Laxkit::ShortcutHandler *sc;

	SomeData *data; //points to dataoc->obj
	ObjectContext *dataoc; //reference object for mirror coordinate space

	MirrorData *mirrordata;
	MirrorToolSettings *settings;

	Laxkit::flatpoint drag_p1, drag_p2; //cache for snapping ref

	int hover;
	virtual int scan(int x, int y, unsigned int state);
	virtual int OtherObjectCheck(int x,int y,unsigned int state);

	virtual int send();

  public:
	enum MirrorActions {
		MIRROR_None=0,

		MIRROR_P1,
		MIRROR_P2,
		MIRROR_Line,
		MIRROR_Label,
		MIRROR_X,
		MIRROR_Y,
		MIRROR_Left,
		MIRROR_Top,
		MIRROR_Right,
		MIRROR_Bottom,
		MIRROR_45,
		MIRROR_135,
		MIRROR_Rotate,
		MIRROR_Merge,
		MIRROR_Merge_Threshhold,
		MIRROR_Move_Line,
		MIRROR_Move_Point,
		MIRROR_MAX
	};

  	enum MirrorSettings {
		MIRRORI_Mirror_Is_M_Real,
		MIRRORI_MAX
  	};
	unsigned int interface_flags; //see MirrorSettings

	MirrorInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~MirrorInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Mirror"; }
	virtual const char *Name();
	virtual const char *whattype() { return "MirrorInterface"; }
	virtual const char *whatdatatype();
	virtual ObjectContext *Context(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);
	virtual void deletedata();
	virtual MirrorData *newData();

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int DrawData(anObject *ndata,anObject *a1,anObject *a2,int info);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();

	virtual int UpdateData();
};

} // namespace LaxInterfaces

#endif

