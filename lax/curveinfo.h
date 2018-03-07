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
//    Copyright (C) 2013-2015 by Tom Lechner
//

#ifndef _LAX_CURVEINFO_H
#define _LAX_CURVEINFO_H

#include <lax/anobject.h>
#include <lax/resources.h>
#include <lax/dump.h>


namespace Laxkit {


//------------------------------- CurveInfo ------------------------------------

class CurveInfoGuide
{
  public:
	int id;
	char *name;
	double value; //real coords
	bool vertical;
	bool editable;
	bool active;
	//ScreenColor color;
	int style; //dashed 1, dotted 2, or solid 0

	CurveInfoGuide *next;

	CurveInfoGuide() { next=NULL; id=0; name=NULL; value=0; editable=true; vertical=false; active=false; }
	virtual ~CurveInfoGuide() { delete[] name; if (next) delete next; }
};

class CurveInfo : public Resourceable, public LaxFiles::DumpUtility
{
  private:
	void base_init();
	
  public:
	enum CurveTypes {
		Linear,
		Autosmooth,
		Bezier
	};

	enum CurveDefaults {
		CURVE_Rising,
		CURVE_Falling,
		CURVE_Flat_Low,
		CURVE_Flat_Middle,
		CURVE_Flat_High,
		CURVE_Sine_Rising,
		CURVE_Sine_Falling,
		CURVE_Sine_Bump,
		CURVE_Sine_Valley,
		CURVEMAX
	};
	void SetDefault(CurveDefaults type, bool set_title);

	CurveInfoGuide *guides;

	double xmin, xmax;
	double ymin, ymax;
	char *xlabel, *ylabel;
	char *title;
	CurveTypes curvetype;
	bool wrap;

	NumStack<flatpoint> points;
	NumStack<flatpoint> fauxpoints;

	int numsamples, lookup_min, lookup_max;
	int *lookup;

	CurveInfo();
	CurveInfo(const char *ntitle,
			  const char *xl, double nxmin, double nxmax,
			  const char *yl, double nymin, double nymax);
	virtual ~CurveInfo();
	virtual const char *whattype() { return "CurveInfo"; }
	CurveInfo &operator=(CurveInfo &l);
	virtual void SetXBounds(double nxmin, double nxmax, const char *nxlabel, bool remap);
	virtual void SetYBounds(double nymin, double nymax, const char *nylabel, bool remap);
	virtual void ComputeYBounds(double buffer);
	virtual void SetTitle(const char *ntitle);
	virtual flatpoint tangent(double x);
	virtual double f(double x);
	virtual double f_linear(double x);
	virtual double f_autosmooth(double x);
	virtual double f_bezier(double x);
	virtual flatpoint MapUnitPoint(flatpoint p);
	virtual flatpoint MapToUnitPoint(flatpoint p);
	virtual int AddRawYPoint(double x,double y);
	virtual int AddPoint(double x,double y);
	virtual int MovePoint(int index, double x,double y);
	virtual void SetSinusoidal(int samples, int variant=0);
	virtual void SetFlat(double y);
	virtual void Reset(bool leaveblank);
	virtual void SetData(flatpoint *p, int n);
	virtual void SetDataRaw(flatpoint *p, int n);
	virtual void Wrap(bool wrapx);
	virtual void InvertY();
	virtual void InvertX();

	virtual void MakeFakeCurve();
	virtual int MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue);
	virtual void RefreshLookup();
	virtual void RefreshLookup(int nsamples, int nmin, int nmax);
	virtual void LookupDump(const char *label,FILE *f);

	 //serializing aids
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};


} // namespace Laxkit

#endif

