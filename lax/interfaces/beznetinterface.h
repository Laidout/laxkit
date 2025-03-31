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
//    Copyright (C) 2023-present by Tom Lechner
//
#ifndef _LAX_BEZNETINTERFACE_H
#define _LAX_BEZNETINTERFACE_H


#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/beznet.h>
#include <lax/singletonkeeper.h>


namespace LaxInterfaces { 


//--------------------------- BezNetInterface -------------------------------------

class BezNetToolSettings : public Laxkit::Resourceable
{
  public:
	double default_linewidth;
	double max_arrow_length;
	Laxkit::ScreenColor default_vertex_color;
	Laxkit::ScreenColor default_edge_color;
	Laxkit::ScreenColor default_face_color;
	int curpoint;

	BezNetToolSettings();
	virtual ~BezNetToolSettings();
	virtual const char *whattype() { return "BezNetToolSettings"; }
};


class BezNetInterface : public anInterface
{
	static Laxkit::SingletonKeeper settingsObject;

  protected:
  	struct CurPointInfo
  	{
  		BezNetData *data;
  		int what; //vertex, point in edge, segment, edge, face
  		int index;
  	};

	Laxkit::PtrStack<CurPointInfo> selected;

	int showdecs;

	Laxkit::ShortcutHandler *sc;

	BezNetData *data; //points to dataoc->obj
	ObjectContext *dataoc;

	BezNetToolSettings *settings;

	int hover = -1;
	int hover_face = -1;
	int hover_edge = -1;
	int hover_type = 0;

	virtual int scan(double x, double y, unsigned int state, int *type_ret);
	virtual int scanFaces(double x, double y, unsigned int state);
	virtual int scanEdges(double x, double y, unsigned int state);
	virtual int OtherObjectCheck(int x,int y,unsigned int state);

	virtual int send();

  public:
	enum BezNetActions {
		BEZNET_None = 0,
		BEZNET_RemoveEdge,
		BEZNET_SplitEdge,
		BEZNET_MergeFaces,
		BEZNET_BuildArea,
		BEZNET_Vertex,
		BEZNET_Edge,
		BEZNET_Face,
		BEZNET_MAX
	};

	unsigned int interface_flags;

	BezNetInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~BezNetInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "BezNet"; }
	virtual const char *Name();
	virtual const char *whattype() { return "BezNetInterface"; }
	virtual const char *whatdatatype();
	virtual ObjectContext *Context(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);
	virtual void deletedata();
	virtual BezNetData *newData();

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
	//virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	//virtual void ViewportResized();
	
	virtual void ClearSelection();
	virtual void DrawEdgeArrow(HalfEdge *edge, int which, double gap);
};

} // namespace LaxInterfaces

#endif
