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

#include <lax/debug.h>


namespace Laxkit {


//------------------------------------- TODO: work in progress ----------------------------------------

/*! TODO: Class to store full units such as `kg*m/s/s`, should this ever be implemented!!
 * 
 * For reference, SI units:
 * 
 *     m    meters     length
 *     s    seconds    time
 *     kg   kilograms  mass
 *     A    ampere     current
 *     K    kelvin     temperature
 *     mol  mole       amount of something
 *     cd   candela    luminous intensity
 * 
 * Common derived units:
 * 
 *     hertz 	Hz 	frequency 	s−1 	
 *     newton 	N 	force, weight 	kg⋅m⋅s−2 	
 *     pascal 	Pa 	pressure, stress 	kg⋅m−1⋅s−2 	N/m2 = J/m3
 *     joule 	J 	energy, work, heat 	kg⋅m2⋅s−2 	N⋅m = Pa⋅m3
 *     watt 	W 	power, radiant flux 	kg⋅m2⋅s−3 	J/s
 *     coulomb 	C 	electric charge 	s⋅A 	
 *     volt 	V 	electric potential, voltage, emf 	kg⋅m2⋅s−3⋅A−1 	W/A = J/C
 *     farad 	F 	capacitance 	kg−1⋅m−2⋅s4⋅A2 	C/V = C2/J
 *     ohm 	Ω 	resistance, impedance, reactance 	kg⋅m2⋅s−3⋅A−2 	V/A = J⋅s/C2
 *     siemens 	S 	electrical conductance 	kg−1⋅m−2⋅s3⋅A2 	Ω−1
 *     weber 	Wb 	magnetic flux 	kg⋅m2⋅s−2⋅A−1 	V⋅s
 *     tesla 	T 	magnetic flux density 	kg⋅s−2⋅A−1 	Wb/m2
 *     henry 	H 	inductance 	kg⋅m2⋅s−2⋅A−2 	Wb/A
 *     degree Celsius 	°C 	temperature relative to 273.15 K 	K 	
 *     lumen 	lm 	luminous flux 	cd⋅m2/m2 	cd⋅sr
 *     lux 	lx 	illuminance 	cd⋅m2/m4 	lm/m2 = cd⋅sr⋅m−2
 *     becquerel 	Bq 	activity referred to a radionuclide (decays per unit time) 	s−1 	
 *     gray 	Gy 	absorbed dose (of ionising radiation) 	m2⋅s−2 	J/kg
 *     sievert 	Sv 	equivalent dose (of ionising radiation) 	m2⋅s−2 	J/kg
 *     katal 	kat 	catalytic activity 	mol⋅s−1
 */
class CompoundUnit
{
  public:
  	struct Piece {
  		int unit;
  		double power; //double to help interim calculations, but in end should resolve to integers generally
  		Piece() { unit = 0; power = 1; }
  		Piece(int u, double p) { unit = u; power = p; }
  	};
  	NumStack<Piece> units; // base units only?
  	int category_hint;

  	CompoundUnit *express_as = nullptr;

  	// Utf8String str; //cached write out of the units

  	void MultiplyUnits(const CompoundUnit &units);
  	void DivideUnits(const CompoundUnit &units);
  	int FindIndex(int unit);
};

int CompoundUnit::FindIndex(int unit)
{
	for (int c = 0; c < units.n; c++)
		if (units.e[c].unit == unit) return c;
	return -1;
}

void CompoundUnit::MultiplyUnits(const CompoundUnit &u)
{
	for (int c = 0; c < u.units.n; c++) {
		int i = FindIndex(u.units.e[c].unit);
		if (i >= 0) units.e[i].power += u.units.e[c].power;
		else units.push(Piece(u.units.e[c].unit, u.units.e[c].power));
	}
}

void CompoundUnit::DivideUnits(const CompoundUnit &u)
{
	for (int c = 0; c < u.units.n; c++) {
		int i = FindIndex(u.units.e[c].unit);
		if (i >= 0) units.e[i].power -= u.units.e[c].power;
		else units.push(Piece(u.units.e[c].unit, -u.units.e[c].power));
	}
}

//------------------------------------- CreateDefaultUnits() ----------------------------------------

//! Add a collection of some common units to a UnitManager.
/*! This will add "pixel" units with a scaling of 1. If you want it to be meaningful, you will
 * have to manually adjust this value. All other units assume that meters is 1.
 *
 * Defined are inches, feet, centimeters, millimeters, meters, points, and pixels.
 *
 * If units == nullptr, return a new SimpleUnit. Else add to units.
 */
UnitManager *CreateDefaultUnits(UnitManager *units, bool include_px, bool include_em)
{
	if (!units) units = new UnitManager(false);
	units->AddUnits(UNITS_Length, UNITS_Inches,    .0254,    0, _("in"),     _("inch"),      _("inches"));
	units->AddUnits(UNITS_Length, UNITS_Feet,      12*.0254, 0, _("ft"),     _("foot"),      _("feet"));
	units->AddUnits(UNITS_Length, UNITS_Yards,     36*.0254, 0, _("yd"),     _("yard"),      _("yards"));
	units->AddUnits(UNITS_Length, UNITS_CM,        .01,      0, _("cm"),     _("centimeter"),_("centimeters"));
	units->AddUnits(UNITS_Length, UNITS_MM,        .001,     0, _("mm"),     _("millimeter"),_("millimeters"));
	units->AddUnits(UNITS_Length, UNITS_Meters,    1,        0, _("m"),      _("meter"),     _("meters"));
	units->AddUnits(UNITS_Length, UNITS_Mil,       2.54e-5,  0, _("mil"),    _("mil"),       _("mils"));
	units->AddUnits(UNITS_Length, UNITS_Micron,    1e-6,     0, _("micron"), _("micron"),    _("micron"));

	units->AddUnits(UNITS_Length, UNITS_Kilometer, 1000,     0,  _("km"),    _("kilometer"), _("kilometers"));
	units->AddUnits(UNITS_Length, UNITS_Miles,     1609.34,  0,  _("mile"),  _("mile"),      _("miles"));

	units->AddUnits(UNITS_Length, UNITS_Points,    .0254/72, 0, _("pt"),     _("point"),     _("points"),    _("72 ppi"));
	units->AddUnits(UNITS_Length, UNITS_SvgPoints, .0254/90, 0, _("svgpt"),  _("svgpoint"),  _("svgpoints"), _("Legacy 90 ppi"));
	units->AddUnits(UNITS_Length, UNITS_CSSPoints, .0254/96, 0, _("csspt"),  _("csspoint"),  _("csspoints"), _("96 ppi"));

	if (include_px) units->AddUnits(UNITS_Length, UNITS_Pixels, .0254/96, 0, _("px"),        _("pixel"),     _("pixels"));
	if (include_em) units->AddUnits(UNITS_Length, UNITS_em,     1,        0, _("em"),        _("em"),        _("em"));

	return units;
}

UnitManager *CreateTemperatureUnits(UnitManager *units)
{
	if (!units) units = new UnitManager(false);

	// K = C − 273.15
	// C = 5/9 * (F - 32)
	// F = 32 + 9/5 * C
	units->AddUnits(UNITS_Temperature, UNITS_K,  1, 273.15,       _("K"), _("kelvin"),     _("kelvin"));
	units->AddUnits(UNITS_Temperature, UNITS_C,  1, 0,            _("C"), _("celcius"),    _("celcius"));
	units->AddUnits(UNITS_Temperature, UNITS_F,  5./9, -32.*5/9,  _("F"), _("fahrenheit"), _("fahrenheit"));
	units->DefaultUnits(UNITS_C, UNITS_Temperature);
	return units;
}

// UnitManager *CreateExtraLengthUnits(UnitManager *units)
// {
// 	if (!units) units = new UnitManager(false);

// 	units->AddUnits(UNITS_Length, UNITS_Fermi,          , 0, _(""), _("Fermi"),         _("Fermi"));           
// 	units->AddUnits(UNITS_Length, UNITS_Angstrom,       , 0, _("ang"), _("Angstrom"),      _("Angstrom"));              
// 	units->AddUnits(UNITS_Length, UNITS_Micron,         , 0, _("micron"), _("Micron"),        _("Micron"));            
// 	units->AddUnits(UNITS_Length, UNITS_Mil,            , 0, _("mil"), _("Mil"),           _("Mil"));              // 1000 mil = 1 inch
// 	units->AddUnits(UNITS_Length, UNITS_Miles,          , 0, _("mile"), _("Miles"),         _("Miles"));           
// 	units->AddUnits(UNITS_Length, UNITS_NauticalMile,   , 0, _("nmile"), _("NauticalMile"),  _("NauticalMile"));                  
// 	units->AddUnits(UNITS_Length, UNITS_League,         , 0, _(""), _("League"),        _("League"));              // 1 = 3 miles = 4800 m
// 	units->AddUnits(UNITS_Length, UNITS_Fathom,         , 0, _(""), _("Fathom"),        _("Fathom"));              // 1 = 6 feet
// 	units->AddUnits(UNITS_Length, UNITS_Furlong,        , 0, _(""), _("Furlong"),       _("Furlong"));               // 1 = 1/8 mile
// 	units->AddUnits(UNITS_Length, UNITS_Horse,          , 0, _(""), _("Horse"),         _("Horse"));               // about 8 feet
// 	units->AddUnits(UNITS_Length, UNITS_FootballField,  , 0, _(""), _("FootballField"), _("FootballField"));                    // 100 yards
// 	units->AddUnits(UNITS_Length, UNITS_EarthRadius,    , 0, _(""), _("EarthRadius"),   _("EarthRadius"));                  // approx. 6,371 km
// 	units->AddUnits(UNITS_Length, UNITS_LunarDist,      , 0, _(""), _("LunarDist"),     _("LunarDist"));                  // approx. 384402 km, center of earth to center of moon
// 	units->AddUnits(UNITS_Length, UNITS_AU,             , 0, _(""), _("AU"),            _("AU"));               // astronomical unit, 1 au = 149597870700 m
// 	units->AddUnits(UNITS_Length, UNITS_Lightsecond,    , 0, _(""), _("Lightsecond"),   _("Lightsecond"));                 
// 	units->AddUnits(UNITS_Length, UNITS_Lightyear,      , 0, _(""), _("Lightyear"),     _("Lightyear"));               
// 	units->AddUnits(UNITS_Length, UNITS_Parsec,         , 0, _(""), _("Parsec"),        _("Parsec"));                // 30856775814671.9 km
// 	units->AddUnits(UNITS_Length, UNITS_Hubble,         , 0, _(""), _("Hubble"),        _("Hubble"));               // 14.4 billion ly
// 	units->AddUnits(UNITS_Length, UNITS_Cubit,          .5, 0, _(""), _("Cubit"),         _("Cubit"));              // elbow to tip of middle finger, very roughly 1/2 meter

// 	return units;
// }

//------------------------------------- GetUnitManager() ----------------------------------------
//! A general repository for units.
/*! This starts out as nullptr. You would initialize it and retrieve it with GetUnitManager().
 */
static SingletonKeeper unit_manager;

//! Return unit_manager, initializing it with CreateDefaultUnits() if it was nullptr, includes px and em units.
UnitManager *GetUnitManager()
{
	UnitManager *um = dynamic_cast<UnitManager*>(unit_manager.GetObject());
	if (!um) {
		um = CreateDefaultUnits(nullptr, true, true);
		unit_manager.SetObject(um,1);
	}
	return um;
}

/*! If nullptr, then clear.
 * If not nullptr, then set manager as default. Note: does NOT inc count, simply takes possession.
 */
void SetUnitManager(UnitManager *manager)
{
	unit_manager.SetObject(manager, 1);
}


//------------------------------------- SimpleUnit ----------------------------------------

/*! \class SimpleUnit
 * \brief Definition of a single unit.
 * Used by UnitManager.
 *
 * Units are defined with a some id number, a scaling to a base unit, and possibly many names.
 * The base unit can be what ever you want, but by default in the Laxkit via GetUnitManager()
 * and CreateDefaultUnits(), meters is a base unit so has scaling 1.
 */

SimpleUnit::SimpleUnit()
  : names(2)
{
	id = 0;
	scaling = 0;
	label = nullptr;
}

SimpleUnit::~SimpleUnit()
{
	delete[] label;
}


//------------------------------------- UnitManager ----------------------------------------


UnitManager::UnitManager(bool install_default)
{
	if (install_default) CreateDefaultUnits(this, true, true);
}


UnitManager::~UnitManager()
{
}


int UnitManager::NumberOfUnits(int category)
{
	if (!categories.n) return 0;

	if (category == UNITS_Default) category = default_category;
	for (int c = 0; c < categories.n; c++) {
		if (categories.e[c]->category == category)
			return categories.e[c]->units.n;
	}

	return 0;
}

UnitCategory *UnitManager::FindCategory(int category)
{
	if (category == UNITS_Default) category = default_category;
	for (int c = 0; c < categories.n; c++) {
		if (categories.e[c]->category == category)
			return categories.e[c];		
	}
	return nullptr;
}

const SimpleUnit *UnitManager::Find(int id, int category)
{
	for (int c = 0; c < categories.n; c++) {
		if (category != UNITS_Any && categories.e[c]->category != category) continue;
		for (int c2 = 0; c2 < categories.e[c]->units.n; c2++) {
			if (categories.e[c]->units.e[c2]->id == id)
				return categories.e[c]->units.e[c2];
		}
	}

	return nullptr;
}

const SimpleUnit *UnitManager::FindByName(const char *name, int len, int category)
{
	if (len < 0) len = strlen(name);
	
	for (int c = 0; c < categories.n; c++) {
		if (category != UNITS_Any && categories.e[c]->category != category) continue;

		for (int c2 = 0; c2 < categories.e[c]->units.n; c2++) {
			SimpleUnit *f = categories.e[c]->units.e[c2];
			for (int c3 = 0; c3 < f->names.n; c3++)
			if (len == (int)strlen(f->names.e[c3]) && !strncasecmp(f->names.e[c3],name,len))
				return f;
		}
	}

	return nullptr;
}

//! Return the unit id corresponding to name, or UNITS_None if not found.
int UnitManager::UnitId(const char *name, int len, int category)
{
	const SimpleUnit *f = FindByName(name,len, category);
	if (!f) return UNITS_None;
	return f->id;
}

/*! Return name corresponding to uid. If not found, return nullptr.
 *
 * Returns first in names list.
 */
const char *UnitManager::UnitName(int uid, int category)
{
	const SimpleUnit *f = Find(uid, category);
	if (!f || !f->names.n) return nullptr;
	return f->names.e[0];
}

//! Retrieve some information about a unit, using id value as a key (not index #).
/*! Return 0 for success or nonzero for error.
 */
int UnitManager::UnitInfoId(int id, double *scale, char **shortname, char **singular,char **plural, const char **label_ret, double *offset, int category)
{
	const SimpleUnit *f = Find(id, category);
	if (!f) return 1;

	if (scale)     *scale     = f->scaling;
	if (offset)    *offset    = f->offset;
	if (shortname) *shortname = f->names.e[0];
	if (singular)  *singular  = f->names.e[1];
	if (plural)    *plural    = f->names.e[2];
	if (label_ret) *label_ret = f->label;
	return 0;
}

//! Retrieve some information about a unit, using index value as a key (not id #).
/*! Return 0 for success or nonzero for error.
 */
int UnitManager::UnitInfoIndex(int index, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret, double *offset, int category)
{
	UnitCategory *cat = FindCategory(category);
	if (!cat || index < 0 || index >= cat->units.n) return 1;

	SimpleUnit *f = cat->units.e[index];
	
	if (iid)       *iid       = f->id;
	if (scale)     *scale     = f->scaling;
	if (offset)    *offset    = f->offset;
	if (shortname) *shortname = f->names.e[0];
	if (singular)  *singular  = f->names.e[1];
	if (plural)    *plural    = f->names.e[2];
	if (label_ret) *label_ret = f->label;
	return 0;
}

//! Retrieve some information about a unit.
/*! Return 0 for success or nonzero for error.
 */
int UnitManager::UnitInfo(const char *name, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret, double *offset, int category)
{
	const SimpleUnit *f = FindByName(name,-1, category);
	if (!f) return 1;

	if (iid)       *iid       = f->id;
	if (scale)     *scale     = f->scaling;
	if (offset)    *offset    = f->offset;
	if (shortname) *shortname = f->names.e[0];
	if (singular)  *singular  = f->names.e[1];
	if (plural)    *plural    = f->names.e[2];
	if (label_ret) *label_ret = f->label;
	return 0;
}

//! Set the size of a pixel in the specified units, or the default units if intheseunits==0.
/*! Return 0 for success or nonzero for error and nothing done.
 * This is only for category UNITS_Length.
 */
int UnitManager::PixelSize(double pixelsize, int intheseunits)
{
	UnitCategory *cat = FindCategory(UNITS_Length);
	if (!cat) return 1;

	if (!intheseunits) intheseunits = cat->default_units_index >= 0 ? cat->units.e[cat->default_units_index]->id : UNITS_Meters;
	SimpleUnit *p = nullptr, *i = nullptr;
	for (int c = 0; c < cat->units.n; c++) {
		if (cat->units.e[c]->id == UNITS_Pixels) p = cat->units.e[c];
		if (cat->units.e[c]->id == intheseunits) i = cat->units.e[c];
		if (p && i) break;
	}
	if (!p || !i) return 1;
	p->scaling = pixelsize * i->scaling;
	return 0;
}

//! Return the id of whatever are the default units, as set by DefaultUnits(const char *, category).
int UnitManager::DefaultUnits(int category)
{
	UnitCategory *cat = FindCategory(category);
	if (cat && cat->default_units_index >= 0) return cat->units.e[cat->default_units_index]->id;
	return UNITS_None;
}

//! Set default units, and return id of the current default units after setting. On not found, return UNITS_None.
int UnitManager::DefaultUnits(const char *units, int category)
{
	if (category == UNITS_Default) category = default_category;
	const SimpleUnit *f = FindByName(units,-1, category);
	if (!f) return UNITS_None;

	UnitCategory *cat = FindCategory(category);
	if (!cat) return UNITS_None;

	for (int c = 0; c < cat->units.n; c++) {
		if (cat->units.e[c] == f) {
			cat->default_units_index = c;
			break;
		}
	}
	return f->id;
}

//! Set default units to the units with this id.
/*! Returns UNITS_None on error, else units_id. */
int UnitManager::DefaultUnits(int units_id, int category)
{
	if (category == UNITS_Default) category = default_category;
	const SimpleUnit *f = Find(units_id, category);
	if (!f) return UNITS_None;

	UnitCategory *cat = FindCategory(category);
	if (!cat) return UNITS_None;

	for (int c = 0; c < cat->units.n; c++) {
		if (cat->units.e[c] == f) {
			cat->default_units_index = c;
			break;
		}
	}
	return f->id;
}

//! Return a value you would multiply numbers in fromunits when you want tounits.
double UnitManager::GetFactor(int fromunits, int tounits, double *offset_ret, int category)
{
	if (fromunits == tounits) {
		if (offset_ret) *offset_ret = 0;
		return 1; //to ward off any rounding errors for the direct case.
	}
	const SimpleUnit *f = Find(fromunits, category);
	const SimpleUnit *t = Find(tounits, category);
	if (!f || !t) {
		if (offset_ret) *offset_ret = 0;
		return 1;
	}
	if (offset_ret) *offset_ret = (f->offset - t->offset) / t->scaling;
	return f->scaling / t->scaling;
}


/*! You should always define shortname. You may pass nullptr for singular, plural, and label.
 * Does NOT check for already exists.
 * Return 0 for success.
 */
int UnitManager::AddUnits(int category, int nid, double scale, double offset, const char *shortname, const char *singular,const char *plural, const char *nlabel)
{
	if (category == UNITS_Default) category = default_category;

	SimpleUnit *u = new SimpleUnit;
	UnitCategory *cat = FindCategory(category);
	if (!cat) {
		cat = new UnitCategory();
		cat->default_units_index = 0;
		cat->category = category;
		categories.push(cat);
	}
	cat->units.push(u);

	u->category = category;
	u->id      = nid;
	u->scaling = scale;
	u->offset  = offset;
	u->names.push(newstr(shortname));
	if (singular) u->names.push(newstr(singular));
	if (plural)   u->names.push(newstr(plural));
	if (nlabel)   makestr(u->label, nlabel);
	return 0;
}


/*! From and to are matched ignoring case to all the names for each unit, until a match is found.
 *
 *  If the units were not found, ther error_ret gets set to a nonzero value. Else it is 0.
 */
double UnitManager::Convert(double value, const char *from, const char *to, int category, int *error_ret)
{
	if (!from || !to) {
		if (error_ret) *error_ret = -1;
		return 0;
	}

	if (!strcmp(from,to)) return value;

	const SimpleUnit *f,*t;
	
	f = FindByName(from,-1, category);
	if (!f) {
		if (error_ret) *error_ret = 1;
		return 0;
	}

	t = FindByName(to,-1, category);
	if (!t) {
		if (error_ret) *error_ret = 2;
		return 0;
	}

	//return value * f->scaling / t->scaling;  without offset
	return (value * f->scaling + f->offset - t->offset) / t->scaling; // with offset
}

/*! If the units were not found, then error_ret gets set to a nonzero value. Else it is 0.
 */
double UnitManager::Convert(double value, int from_id, int to_id, int category, int *error_ret)
{
	if (from_id == to_id) return value; //avoid rounding errors!

	const SimpleUnit *f,*t;

	f = Find(from_id, category);
	if (!f) {
		if (error_ret) *error_ret=1;
		return 0;
	}

	t = Find(to_id, category);
	if (!t) {
		if (error_ret) *error_ret=2;
		return 0;
	}

	//return value * f->scaling / t->scaling;  without offset
	return (value * f->scaling + f->offset - t->offset) / t->scaling; // with offset
}

double UnitManager::ParseWithUnit(const char *str, int *units_ret, int *unit_category_ret, const char **unit_str_ret, const char **end_ptr)
{
	char *endptr = nullptr;
	double d = strtod(str, &endptr);
	if (end_ptr) *end_ptr = endptr;
	if (endptr == str) {
		if (units_ret) *units_ret = UNITS_Error;
		return 0;
	}

	const char *p = endptr;
	while(isspace(*p)) p++;

	int unit = UNITS_None;
	int cat = UNITS_None;
	if (unit_str_ret) *unit_str_ret = p;
	if (*p) {
		const SimpleUnit *sunit = FindByName(p,-1);
		if (sunit) {
			unit = sunit->id;
			cat = sunit->category;
		}
	}

	if (units_ret) *units_ret = unit;
	if (unit_category_ret) *unit_category_ret = cat;
	return d;
}

int UnitManager::BuildFor(int category)
{
	default_category = category;
	return default_category;
}


/*! For instance, to add meters/second:
 * ```
 *   AddDerivedUnit(UNITS_MPS, 1, 0, "m_per_s", "Meter per second", "Meters per second", "Meters per second",
 *   				2,
 *   				UNITS_Meters,   1.0,
 *   				UNITS_Seconds, -1.0)
 * ```
 */
bool UnitManager::AddDerivedUnit(int nid, double scale, double offset,
						const char *shortname, const char *singular,const char *plural, const char *nlabel,
						int num_base_units, ...)
{
	DBGE("IMPLEMENT ME!!")
	return false;
}


} //namespace Laxkit


