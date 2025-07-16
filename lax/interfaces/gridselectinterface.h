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
//    Copyright (C) 2025 by Tom Lechner
//
#ifndef _LAX_GRIDSELECTINTERFACE_H
#define _LAX_GRIDSELECTINTERFACE_H

#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 



//--------------------------- GridSelectInterface -------------------------------------

class GridSelectInterface : public anInterface
{
  protected:
	int showdecs;

	Laxkit::ShortcutHandler *sc;

	Laxkit::MenuInfo *items = nullptr;
	int rows = 0, columns = 0;
	Laxkit::DoubleBBox items_bounds;
	double cell_w = 0, cell_h = 0;
	double gap = -1;

	Laxkit::NumStack<int> selected;
	Laxkit::NumStack<int> almost_selected; // for drag box selecting, preview what will be selected

	int hover = -1;
	Laxkit::flatpoint offset;
	Laxkit::flatpoint offset_target;

	bool need_to_remap = true;
	int lerp_timer = 0;


	virtual int scan(int x, int y, unsigned int state);
	virtual int send(int context);
	virtual void RemapItems();
	virtual int AdjustOffset();

  public:
	enum GridSelectActions {
		GRIDSELECT_None = 0,
		GRIDSELECT_One_Row,
		GRIDSELECT_One_Column,
		GRIDSELECT_Grid,
		GRIDSELECT_Maybe_Drag,
		GRIDSELECT_Drag_Box,
		// GRIDSELECT_,
		GRIDSELECT_MAX
	};
	int mode = GRIDSELECT_None;

	int grid_type = GRIDSELECT_Grid;

	unsigned int interface_flags = 0;

	double hover_diff;
	Laxkit::ScreenColor color_normal;
	Laxkit::ScreenColor color_selected;

	bool show_images  = true;
	bool show_strings = true;

	GridSelectInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~GridSelectInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "GridSelect"; }
	virtual const char *Name();
	virtual const char *whattype() { return "GridSelectInterface"; }
	virtual const char *whatdatatype();
	virtual ObjectContext *Context(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual int  Idle(int tid, double delta);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

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
	virtual int MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	//virtual int RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();


	virtual void UseThisMenu(Laxkit::MenuInfo *newmenu);
};

} // namespace LaxInterfaces

#endif

