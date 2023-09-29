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
	UNITS_SvgPoints, // 90 svgpts == 1 inch
	UNITS_CSSPoints, // 96 csspts == 1 inch
	UNITS_Pixels,    // 96 == 1 inch in css, but tricky due to device size
	UNITS_Picas,     // 1 pc == 12 pt
	UNITS_em,        // 1 == context font size
	UNITS_ex,        // 1 == x-height
	UNITS_ch,        // 1 == width of the 0 glyph
	UNITS_rem,       // 1 == font size of root element
	UNITS_vw,        // 1 == 1% of the width of viewport
	UNITS_vh,        // 1 == 1% of the height of viewport
	UNITS_vmin,      // 1 == 1% of the min(width,height) of viewport
	UNITS_vmax,      // 1 == 1% of the max(width,height) of viewport
	UNITS_MAX
};


typedef int Unit;

//------------------------------------- SimpleUnit ----------------------------------------
class SimpleUnit : public anObject
{
  public:
	int id;
	double scaling;
	PtrStack<char> names; //0,1,2 are short name, singular, plural
	char *label;
	int relative_key = 0; //TODO: if non-zero, UnitManager uses this key to get correct relative scaling value
	SimpleUnit *next;

	SimpleUnit();
	virtual ~SimpleUnit();

	virtual SimpleUnit *find(const char *name,int len=-1);
	virtual SimpleUnit *find(int units);
};


class UnitManager : public anObject
{
  protected:
  	SimpleUnit *units = nullptr;

	int defaultunits = UNITS_None;

  public:
  	UnitManager(bool install_default = true);
  	virtual ~UnitManager();

  	virtual int UnitId(const char *name, int len=-1);
	virtual const char *UnitName(int uid);
	virtual int UnitInfo(const char *name, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret);
	virtual int UnitInfoIndex(int index, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret);
	virtual int UnitInfoId(int id, double *scale, char **shortname, char **singular,char **plural, const char **label_ret);
	virtual double GetFactor(int fromunits, int tounits);
	virtual int DefaultUnits();
	virtual int DefaultUnits(const char *units);
	virtual int DefaultUnits(int units);
	virtual int PixelSize(double pixelsize, int intheseunits);

  	virtual int NumberOfUnits();
	virtual const SimpleUnit *Find(int id);
	virtual int AddUnits(int nid, double scale, const char *shortname, const char *singular,const char *plural, const char *nlabel=NULL);
	virtual double Convert(double value, const char *from, const char *to, int *error_ret);
	virtual double Convert(double value, int from_id, int to_id, int *error_ret);
};


//------------------------------------- CreateDefaultUnits() ----------------------------------------
UnitManager *CreateDefaultUnits(UnitManager *units = nullptr);
UnitManager *GetUnitManager();
void SetUnitManager(UnitManager *manager);


} // namespace Laxkit

#endif
