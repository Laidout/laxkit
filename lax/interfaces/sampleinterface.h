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
#ifndef _LAX_SAMPLEINTERFACE_H
#define _LAX_SAMPLEINTERFACE_H

#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 


//--------------------------- SampleData -------------------------------------

//class SampleData : virtual public SomeData
//{
//  public:
//	SampleData();
//	virtual ~SampleData();
//
//	virtual const char *whattype() { return "SampleData"; }
//	virtual void FindBBox();
//	virtual SomeData *duplicate(SomeData *dup);
//
//};


//--------------------------- SampleInterface -------------------------------------

class SampleToolSettings : public Laxkit::anObject
{
  public:
  	int num_sample_points = 5;
  	int cur_mode = 0; // 0 == points, 1 == line
  	int cur_target = 0; // 0 == palette, 1 == gradient
	double default_linewidth = 2;
	double default_point_size = 30;

	int curpoint;
	Laxkit::NumStack<int> curpoints;
};

class SampleInterface : public anInterface
{
	static SingletonKeeper settingsObject;

  protected:
	int showdecs;

	class Spot {
	  public:
	  	flatpoint position;
	  	DoubleRect box;
	  	Color *color = nullptr;
	};

	GradientStrip *strip = nullptr;
	PtrStack<Spot> spots;
	int selected = -1; // index into strip

	Laxkit::ShortcutHandler *sc;

	// object to sample from, EITHER dataoc/data OR image
	SomeData *data; // points to dataoc->obj
	ObjectContext *dataoc; // optional viewport object to sample from. Note this is more a ref than the usual usage for dataoc.
	ImageData *image; // sample from this image


	SampleToolSettings *settings; // also stored in settingsObject

	int hover;
	virtual int scan(int x, int y, unsigned int state);
	virtual int OtherObjectCheck(int x,int y,unsigned int state);

	virtual int send();

  public:
 	enum Actions {
 		ACTION_Export = 1,
 		ACTION_Import,
 		ACTION_RenamePalette,
 		ACTION_DuplicatePalette,
 		ACTION_DeletePalette,
 		ACTION_SaveAsResource,
 		ACTION_RenameColor,
 		ACTION_NewColor,
 		ACTION_RemoveColor,
 		ACTION_EditColor,
 		ACTION_DuplicateColor,
 		ACTION_NewPalette,
 		ACTION_MAX,
 		ACTION_PaletteResource = 10000 //this needs to be > all the above
 	};

 	enum Hover {
 		HOVER_None = -1,
 		HOVER_Lock = -1000,
 		HOVER_PaletteName,
 		HOVER_ColorName,
 		HOVER_Color,
 		HOVER_ColorSelect,
 		HOVER_MAX
 	};

	unsigned int interface_flags;

	SampleInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~SampleInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "Sample"; }
	virtual const char *Name();
	virtual const char *whattype() { return "SampleInterface"; }
	virtual const char *whatdatatype();
	virtual ObjectContext *Context(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);
	virtual void deletedata();
	virtual SampleData *newData();

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
};

} // namespace LaxInterfaces

#endif

