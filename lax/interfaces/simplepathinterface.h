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
//    Copyright (C) 2019 by Tom Lechner
//
#ifndef _LAX_SIMPLEPATHINTERFACE_H
#define _LAX_SIMPLEPATHINTERFACE_H


#include <lax/spiro/spiro.h>
#include <lax/spiro/rspline.h>
#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 


//--------------------------- SimplePathData -------------------------------------



class SimplePathData : virtual public SomeData
{
  public:
	enum Interpolation {
		Linear,
		Quadratic,
		Cubic,
		Spiro,
		NewSpiro
	};

	enum PointType {
		Default,
		Smooth,
		Corner,
		Spiro_G4,
		Spiro_G2,
		Spiro_Left,
		Spiro_Right,
		Spiro_Anchor,
		Spiro_Handle
	};

	static const char *PathTypeName(Interpolation interpolation);
	static const char *PointTypeName(PointType pointtype);

	class Point
	{
	  public:
		int type;
		flatvector p;
		double ltheta;
		double rtheta;
		int ltype;
		int rtype;

		Point()
		{
			type = Smooth;
			ltheta = rtheta = 0;
			ltype = rtype = Default;
		}
		Point(flatvector p, int type, int ltype, double ltheta, int rtype, double rtheta)
		{
			this->p      = p;
			this->type   = type;
			this->ltheta = ltheta;
			this->rtheta = rtheta;
			this->ltype  = ltype;
			this->rtype  = rtype;
		}
	};

	Laxkit::ScreenColor color;
	Interpolation interpolation;
	Laxkit::PtrStack<Point> mpoints;
	//Laxkit::NumStack<flatvector> points;
	Laxkit::NumStack<flatvector> pointcache;
	double linewidth;
	bool closed;

	bool needtoupdate;
	Laxkit::anObject *cache;

	//old spiro data
	int spiro_cache_n;
	Laxkit::spiro_cp *spiro_points;
	
	//new spiro data
	NewSpiro::Spline newspiro;


	SimplePathData();
	virtual ~SimplePathData();
	virtual const char *whattype() { return "SimplePath"; }
    virtual void FindBBox();
    virtual SomeData *duplicate(SomeData *dup);

	virtual void UpdateInterpolation();
	virtual void BuildSpiro();
	virtual void BuildNewSpiro();
	virtual void Interpolate(Interpolation newInterpolation);

	virtual void TangentAtIndex(int index, flatpoint &prev, flatpoint &next);

	virtual int AddPoint(flatvector p, int where);
	virtual int AddPoint(flatvector p, int type, int ltype, double ltheta, int rtype, double rtheta, int where);
	virtual void RemoveAt(int index);

	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual void dump_out(FILE *f, int indent, int what, LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
	virtual char *GetSvgPath();
};


//--------------------------- SimplePathInterface -------------------------------------

class SimplePathInterface : public anInterface
{
  protected:
	Laxkit::DoubleBBox selection;
	Laxkit::NumStack<int> curpoints;
	int curpoint;
	int curcontrol;

	int timerid;
	double theta_radius;

	Laxkit::ShortcutHandler *sc;

	SimplePathData *data; //points to dataoc->obj
	ObjectContext *dataoc;

	int hover;
	int hover_handle;

	virtual int scan(double x, double y, unsigned int state, int *handle_ret, int *closest_i_ret = nullptr, double *closest_dist_ret = nullptr);
	virtual int OtherObjectCheck(int x,int y,unsigned int state);
	virtual void deletedata();
	virtual SimplePathData *newData();
	virtual bool IsSelected(int i);

	virtual int send();

  public:
	enum SimplePathActions {
		SIMPLEPATH_None=0,

		SIMPLEPATH_Linear,
		SIMPLEPATH_Quadratic,
		SIMPLEPATH_Cubic,
		SIMPLEPATH_Spiro,
		SIMPLEPATH_NewSpiro,

		SIMPLEPATH_Handle,
		SIMPLEPATH_Left_Handle,
		SIMPLEPATH_Right_Handle,

		SIMPLEPATH_Spiro_Corner,
		SIMPLEPATH_Spiro_Smooth,
		SIMPLEPATH_Spiro_Auto,
		SIMPLEPATH_Spiro_G4,
		SIMPLEPATH_Spiro_G2,
		SIMPLEPATH_Spiro_Left,
		SIMPLEPATH_Spiro_Right,
		SIMPLEPATH_Spiro_Anchor,
		SIMPLEPATH_Spiro_Handle,

		SIMPLEPATH_Load,
		SIMPLEPATH_Save,
		SIMPLEPATH_Save_Bezier,

		SIMPLEPATH_ShowBez,
		SIMPLEPATH_ToggleClosed,

		SIMPLEPATH_MAX
	};

	unsigned int interface_flags;

	//adjustable settings:
	int showdecs;
	bool showbez;
	double select_radius;
	double max_theta_radius;
	double min_theta_radius;
	double point_radius;


	SimplePathInterface(anInterface *nowner, int nid, Laxkit::Displayer *ndp);
	virtual ~SimplePathInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "SimplePath"; }
	virtual const char *Name();
	virtual const char *whattype() { return "SimplePathInterface"; }
	virtual const char *whatdatatype();
	virtual int Idle(int tid, double delta);
	virtual ObjectContext *Context(); 
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
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
	//virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	//virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();

	virtual void ClearSelection();
};

} // namespace LaxInterfaces

#endif

