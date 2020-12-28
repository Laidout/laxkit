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

#include <lax/units.h>
#include <lax/language.h>
#include <lax/strmanip.h>
#include <lax/singletonkeeper.h>


//template implementation:
#include <lax/lists.cc>

#include <iostream>
using namespace std;
#define DBG

namespace Laxkit {


//------------------------------------- UnitManager ----------------------------------------

class UnitManager2
{
  public:
	UnitManager2();
	virtual ~UnitManager2();
};


//------------------------------------- CreateDefaultUnits() ----------------------------------------

//! Create a SimpleUnit collection of some common units.
/*! This will add "pixel" units with a scaling of 1. If you want it to be meaningful, you will
 * have to manually adjust this value. All other units assume that meters is 1.
 *
 * Defined are inches, feet, centimeters, millimeters, meters, points, and pixels
 *
 * If units==NULL, return a new SimpleUnit. Else add to units.
 */
SimpleUnit *CreateDefaultUnits(SimpleUnit *units, bool include_px, bool include_em)
{
	if (!units) units=new SimpleUnit();
	units->AddUnits(UNITS_Inches,    .0254,    _("in"), _("inch"),      _("inches"));
	units->AddUnits(UNITS_Feet,      12*.0254, _("ft"), _("foot"),      _("feet"));
	units->AddUnits(UNITS_Yards,     36*.0254, _("yd"), _("yard"),      _("yards"));
	units->AddUnits(UNITS_CM,        .01,      _("cm"), _("centimeter"),_("centimeters"));
	units->AddUnits(UNITS_MM,        .001,     _("mm"), _("millimeter"),_("millimeters"));
	units->AddUnits(UNITS_Meters,    1,        _("m"),   _("meter"),    _("meters"));
	units->AddUnits(UNITS_Points,    .0254/72, _("pt"),   _("point"),   _("points"), _("72 ppi"));
	units->AddUnits(UNITS_SvgPoints, .0254/90, _("svgpt"),_("svgpoint"),_("svgpoints"), _("Legacy 90 ppi"));
	units->AddUnits(UNITS_CSSPoints, .0254/96, _("csspt"),_("csspoint"),_("csspoints"), _("96 ppi"));

	if (include_px) units->AddUnits(UNITS_Pixels, .0254/96, _("px"), _("pixel"), _("pixels"));
	if (include_em) units->AddUnits(UNITS_Em,     1, _("em"), _("em"),    _("em"));

	return units;
}


//------------------------------------- GetUnitManager() ----------------------------------------
//! A general repository for units.
/*! This starts out as NULL. You would initialize it and retrieve it with GetUnitManager().
 */
static SingletonKeeper unit_manager;

//! Return unit_manager, initializing it with CreateDefaultUnits() if it was NULL, includes px and em units.
UnitManager *GetUnitManager()
{
	UnitManager *um = dynamic_cast<UnitManager*>(unit_manager.GetObject());
	if (!um) {
		um = CreateDefaultUnits(NULL, true, true);
		unit_manager.SetObject(um,1);
	}
	return um;
}

/*! If NULL, then clear.
 * If not NULL, then set manager as default. Note: does NOT inc count, simply takes possession.
 */
void SetUnitManager(UnitManager *manager)
{
	unit_manager.SetObject(manager, 1);
}


//------------------------------------- SimpleUnit ----------------------------------------

/*! \class SimpleUnit
 * \brief Small class to help with the conversion of simple units.
 *
 * Units are defined with a some id number, a scaling to a base unit, and possibly many names.
 * The base unit can be what ever you want, but by default in the Laxkit via GetUnitManager()
 * and CreateDefaultUnits(), meters is assumed to be 1.
 */

SimpleUnit::SimpleUnit()
  : names(2)
{
	id = 0;
	scaling = 0;
	next = NULL;
	defaultunits = 0;
	label = NULL;
}

SimpleUnit::~SimpleUnit()
{
	if (next) delete next;
	delete[] label;
}

int SimpleUnit::NumberOfUnits()
{
	SimpleUnit *f=this;
	int n=0;
	if (!f->scaling) return 0;
	while (f) {
		n++;
		f=f->next;
	}
	return n;
}

//! Return the unit id corresponding to name, or UNITS_None if not found.
int SimpleUnit::UnitId(const char *name,int len)
{
	SimpleUnit *f=find(name,len);
	if (!f) return UNITS_None;
	return f->id;
}

/*! Return name corresponding to uid. If not found, return NULL.
 *
 * Returns first in names list.
 */
const char *SimpleUnit::UnitName(int uid)
{
	SimpleUnit *f=find(uid);
	if (!f || !f->names.n) return NULL;

	return f->names[0];
}

//! Retrieve some information about a unit, using id value as a key (not index #).
/*! Return 0 for success or nonzero for error.
 */
int SimpleUnit::UnitInfoId(int id, double *scale, char **shortname, char **singular,char **plural, const char **label_ret)
{
	SimpleUnit *f=find(id);
	if (!f) return 1;

	if (scale)     *scale     = f->scaling;
	if (shortname) *shortname = f->names.e[0];
	if (singular)  *singular  = f->names.e[1];
	if (plural)    *plural    = f->names.e[2];
	if (label_ret) *label_ret = f->label;
	return 0;
}

//! Retrieve some information about a unit, using index value as a key (not id #).
/*! Return 0 for success or nonzero for error.
 */
int SimpleUnit::UnitInfoIndex(int index, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret)
{
	SimpleUnit *f=this;
	if (!f->scaling) return 1;
	while (f && index) { f=f->next; index--; }
	if (!f) return 1;

	if (iid)       *iid       = f->id;
	if (scale)     *scale     = f->scaling;
	if (shortname) *shortname = f->names.e[0];
	if (singular)  *singular  = f->names.e[1];
	if (plural)    *plural    = f->names.e[2];
	if (label_ret) *label_ret = f->label;
	return 0;
}

//! Retrieve some information about a unit.
/*! Return 0 for success or nonzero for error.
 */
int SimpleUnit::UnitInfo(const char *name, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret)
{
	SimpleUnit *f = find(name);
	if (!f) return 1;

	if (iid)       *iid       = f->id;
	if (scale)     *scale     = f->scaling;
	if (shortname) *shortname = names.e[0];
	if (singular)  *singular  = names.e[1];
	if (plural)    *plural    = names.e[2];
	if (label_ret) *label_ret = label;
	return 0;
}

//! Set the size of a pixel in the specified units, or the default units if intheseunits==0.
/*! Return 0 for success or nonzero for error and nothing done.
 */
int SimpleUnit::PixelSize(double pixelsize, int intheseunits)
{
	if (!intheseunits) intheseunits=defaultunits;
	SimpleUnit *p=this, *i=this;
	while (p && p->id!=UNITS_Pixels) p=p->next;
	while (i && i->id!=intheseunits) i=i->next;
	if (!p || !i) return 1;
	p->scaling=pixelsize*i->scaling;
	return 0;
}

//! Return the id of whatever are the default units, as set by DefaultUnits(const char *).
int SimpleUnit::DefaultUnits()
{
	return defaultunits;
}

//! Set default units, and return id of the current units.
int SimpleUnit::DefaultUnits(const char *units)
{
	SimpleUnit *f=find(units);
	if (!f) return defaultunits;
	defaultunits=f->id;
	return defaultunits;
}

int SimpleUnit::DefaultUnits(int units)
{
	SimpleUnit *f=this;
	while (f && f->id!=units) f=f->next;
	if (!f) return 0;
	defaultunits=f->id;
	return units;
}

//! Return a value you would multiply numbers in fromunits when you want tounits.
double SimpleUnit::GetFactor(int fromunits, int tounits)
{
	if (fromunits==tounits) return 1; //to ward off any rounding errors for the direct case.
	SimpleUnit *f=find(fromunits);
	SimpleUnit *t=find(tounits);
	if (!f || !t) return 1;
	return f->scaling/t->scaling;
}


/*! You should always define shortname. You may pass NULL for singular or plural.
 */
int SimpleUnit::AddUnits(int nid, double scale, const char *shortname, const char *singular,const char *plural, const char *nlabel)
{
	SimpleUnit *u=this;
	if (scaling) { //this unit already taken, need to advance to a new one
		u=this;
		while (u->next) u=u->next;
		u->next=new SimpleUnit;
		u=u->next;
	}
	u->id=nid;
	u->scaling=scale;
	u->names.push(newstr(shortname));
	if (singular) u->names.push(newstr(singular));
	if (plural)   u->names.push(newstr(plural));
	if (nlabel)   makestr(u->label, nlabel);
	return 0;
}

const SimpleUnit *SimpleUnit::Find(int id)
{
	return find(id);
}

SimpleUnit *SimpleUnit::find(int units)
{
	SimpleUnit *f;
	f=this;
	while (f && f->id!=units) f=f->next;
	return f;
}

SimpleUnit *SimpleUnit::find(const char *name,int len)
{
	if (len<0) len=strlen(name);
	SimpleUnit *f;
	f=this;
	while (f) {
		for (int c=0; c<f->names.n; c++)
			if (len==(int)strlen(f->names.e[c]) && !strncasecmp(f->names.e[c],name,len)) return f;
		f=f->next;
	}
	return NULL;
}

/*! From and to are matched ignoring case to all the names for each unit, until a match is found.
 *
 *  If the units were not found, ther error_ret gets set to a nonzero value. Else it is 0.
 */
double SimpleUnit::Convert(double value, const char *from, const char *to, int *error_ret)
{
	if (!from || !to) {
		if (error_ret) *error_ret=-1;
		return 0;
	}

	if (!strcmp(from,to)) return value;

	SimpleUnit *f,*t;
	int c;

	f=this;
	while (f) {
		for (c=0; c<f->names.n; c++) if (!strcasecmp(f->names.e[c],from)) break;
		if (f->names.n && c!=f->names.n) break;
		f=f->next;
	}
	if (!f) {
		if (error_ret) *error_ret=1;
		return 0;
	}

	t=this;
	while (t) {
		for (c=0; c<t->names.n; c++) if (!strcasecmp(t->names.e[c],to)) break;
		if (t->names.n && c!=t->names.n) break;
		t=t->next;
	}
	if (!t) {
		if (error_ret) *error_ret=2;
		return 0;
	}

	return value*f->scaling/t->scaling;
}

/*! If the units were not found, ther error_ret gets set to a nonzero value. Else it is 0.
 */
double SimpleUnit::Convert(double value, int from_id, int to_id, int *error_ret)
{
	if (from_id==to_id) return value; //avoid rounding errors!

	SimpleUnit *f,*t;

	f=this;
	while (f && f->id!=from_id) f=f->next;
	if (!f) {
		if (error_ret) *error_ret=1;
		return 0;
	}

	t=this;
	while (t && t->id!=to_id) t=t->next;
	if (!t) {
		if (error_ret) *error_ret=2;
		return 0;
	}

	return value*f->scaling/t->scaling;
}

} //namespace Laxkit


