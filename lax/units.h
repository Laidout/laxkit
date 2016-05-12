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
//    Copyright (C) 2011 by Tom Lechner
//
#ifndef _LAX_UNITS_H
#define _LAX_UNITS_H


#include <lax/anobject.h>
#include <lax/lists.h>

namespace Laxkit {

enum UnitTypes {
	UNITS_None=0,
	UNITS_Inches,
	UNITS_Feet,
	UNITS_Yards,
	UNITS_CM,
	UNITS_MM,
	UNITS_Meters,
	UNITS_Points,
	UNITS_SvgPoints,
	UNITS_Pixels,
	UNITS_Em,
	UNITS_MAX
};

//------------------------------------- SimpleUnit ----------------------------------------
class SimpleUnit : public anObject
{
  protected:
	virtual SimpleUnit *find(const char *name,int len=-1);
	virtual SimpleUnit *find(int units);

  public:
	int id;
	double scaling;
	PtrStack<char> names; //0,1,2 are short name, singular, plural
	SimpleUnit *next;

	int defaultunits;

	SimpleUnit();
	virtual ~SimpleUnit();

	virtual int UnitId(const char *name,int len=-1);
	virtual const char *UnitName(int uid);
	virtual int UnitInfo(const char *name, int *iid, double *scale, char **shortname, char **singular,char **plural);
	virtual int UnitInfoIndex(int index, int *iid, double *scale, char **shortname, char **singular,char **plural);
	virtual int UnitInfoId(int id, double *scale, char **shortname, char **singular,char **plural);
	virtual double GetFactor(int fromunits, int tounits);
	virtual int DefaultUnits();
	virtual int DefaultUnits(const char *units);
	virtual int DefaultUnits(int units);
	virtual int PixelSize(double pixelsize, int intheseunits);

	virtual int NumberOfUnits();
	virtual const SimpleUnit *Find(int id);
	virtual int AddUnits(int nid, double scale, const char *shortname, const char *singular,const char *plural);
	virtual double Convert(double value, const char *from, const char *to, int *error_ret);
	virtual double Convert(double value, int from_id, int to_id, int *error_ret);
};

typedef SimpleUnit UnitManager;

//------------------------------------- CreateDefaultUnits() ----------------------------------------
SimpleUnit *CreateDefaultUnits(SimpleUnit *units=NULL);
UnitManager *GetUnitManager();
void SetUnitManager(UnitManager *manager);


} // namespace Laxkit

#endif
