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
	UNITS_None = 0,
	UNITS_Any,
	UNITS_Default,
	//categories
	UNITS_Length,
	UNITS_Time,
	UNITS_Mass,
	UNITS_Current,
	UNITS_Temperature,
	UNITS_Amount,
	UNITS_Luminosity,
	UNITS_Angle,
	UNITS_SphericalAngle,
	// length
	UNITS_Inches,   // 1 inch = 2.54 cm by definition!
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
	// super length
	UNITS_Fermi,
	UNITS_Angstrom,
	UNITS_Micron,
	UNITS_Mil,     // 1000 mil = 1 inch
	UNITS_Miles,
	UNITS_NauticalMile,
	UNITS_League,  // 1 = 3 miles = 4800 m
	UNITS_Fathom,  // 1 = 6 feet
	UNITS_Furlong,  // 1 = 1/8 mile
	UNITS_Horse,    // about 8 feet
	UNITS_FootballField, // 100 yards
	UNITS_EarthRadius, // approx. 6,371 km
	UNITS_LunarDist,   // approx. 384402 km, center of earth to center of moon
	UNITS_AU,       // astronomical unit, 1 au = 149597870700 m
	UNITS_Lightsecond,
	UNITS_Lightyear,
	UNITS_Parsec,    // 30856775814671.9 km
	UNITS_Hubble,   // 14.4 billion ly
	UNITS_Cubit,   // elbow to tip of middle finger, very roughly 1/2 meter
	// area
	UNITS_SquareMeter,
	UNITS_SquareKM,
	UNITS_SquareFeet,
	UNITS_Hectare,
	UNITS_Acre,
	UNITS_Killa,   // (s asia) 1 = 1 acre
	UNITS_Ghumaon, // (s asia) 1 = 1 acre
	UNITS_Kanal,   // (s asia) 1 = .125 acre (1 acre = 8 kanal)
	UNITS_Decimal, // (s asia) 1 = 48.4 square yards
	UNITS_Chatak,  // (s asia) 1 = 180 square feet
	UNITS_Barn, // 1 barn = 10^-28 m^2
	// volume
	UNITS_Litre,      // 1 L = .001 m^3 == 1000 cm^3
	UNITS_Millilitre, // 1 ml = 1 m^3 = 1000000 cm^3
	UNITS_CubicInch,
	UNITS_CubicFoot,
	UNITS_CubicKM,
	UNITS_CubicMile,
	UNITS_Hogshead,  // 1 hogshead = 63 us gal
	UNITS_OilBarrel, // 1 bbl = 42 us gal
	UNITS_Barrel,   // 1 barrel = 31.5 us gal
	UNITS_Gallon,     // 231 cubic inches (US), or 4.54609 L UK
	UNITS_Quart,      // 4 quart = 1 gallon
	UNITS_Pint,       // 2 pint  = 1 quart
	UNITS_Cup,        // 2 cup   = 1 pint
	UNITS_Gill,       // 4 gill  = 1 pint, 2 gill = 1 cup
	UNITS_Jig,        // 1 jig   = 3 tbsp
	UNITS_Shot,       // 20 to 60 ml
	UNITS_PonyShot,   // 20 to 30 ml
	UNITS_FluidOunce, // 4 ounce = 1 gill, 1 oz = 2 tbsp, 8 oz = 1 cup
	UNITS_Tablespoon, // 1 tbsp  = 3 tsp, 16 tbsp = 1 cup
	UNITS_Teaspoon,   // 1 tsp   = 80 min
	UNITS_FluidDram,  // 1 dr    = 60 min
	UNITS_Minim,      // 1 drop of water, .95 grain water
	UNITS_Grain,      // 1 grain = 64.79891 mg
	UNITS_GrandCanyon, // 4.17 trillion cubic meters
	// angle
	UNITS_Radians,
	UNITS_Rotations,
	UNITS_Degrees,
	UNITS_Arcminute,
	UNITS_Arcsecond,
	UNITS_Steradian, // solid angle.. portion of unit sphere surface area / radius^2. whole sphere = 4 pi
	// temperature
	UNITS_K,  // K = C − 273.15
	UNITS_C,  // C = 5/9 * (F - 32)
	UNITS_F,  // F = 32 + 9/5 * C
	// mass
	UNITS_kg,
	UNITS_g,
	UNITS_lb,
	UNITS_Ounces,
	UNITS_Tonne,  // 1 t = 1 Mg = 1000 kg
	UNITS_Jupiter, // 1.9 ×1027 kg
	UNITS_SunMass, // 2.0×1030 kg
	// time
	UNITS_Seconds,
	UNITS_Minutes,
	UNITS_Hours,
	UNITS_Days,
	UNITS_Weeks,
	UNITS_Fortnight,
	UNITS_Months, // assume 30 days?
	UNITS_Years,
	UNITS_Decades,
	UNITS_Centuries,
	UNITS_SiderealDay, // 23 hours, 56 min, 4.0905 seconds
	// speed
	UNITS_MPH,
	UNITS_KPH,
	UNITS_MPS,
	UNITS_FPS,
	UNITS_Knots,  // 1.852 km/h
	UNITS_Mach,  // 340.3 m/s
	UNITS_LightSpeed, // in a vacuum, 299792458 m/s
	// rotational speed
	UNITS_RadiansPerSec,
	UNITS_DegreesPerSec,
	UNITS_RotationsPerSec,
	// current / electricity related
	UNITS_Ampere,
	UNITS_Coulomb, // s A
	UNITS_Hertz,   // 1/s
	UNITS_Ohms,    // kg m^2 / (s^3 * A^2)
	UNITS_Volt,    // kg m^2 / (s^3 * A)
	UNITS_Watt,    // kg m^2 / s^3
	UNITS_Farad,   // s^4 A^2 / kg / m^2
	UNITS_Siemens, // s^3 A^2 / kg / m^2
	UNITS_Weber,   // kg m^2 / s^2 / A
	UNITS_Tesla,   // kg / s^2 / A
	UNITS_Henry,   // kg m^2 / s^2 / A^2
	// amount
	UNITS_Mole, // 6.02214076 x 10^23
	// luminosity
	UNITS_Candela,
	UNITS_Lumen,   // cd sr (sr is a steradian)
	UNITS_Lux,     // cd sr / m^2
	// pressure
	UNITS_Pa,      // kg / m / s^2
	UNITS_kPa,
	UNITS_PSI,
	// force
	UNITS_Newton,   // kg m / s^2
	// Energy
	UNITS_Joule,     // kg m^2 / s^@
	UNITS_eV,        // 1 ev = 1.602176634 x 10^−19 J
	UNITS_Calorie,
	// radioactivity
	UNITS_Becquerel, // 1/s^2
	UNITS_Gray,      // m^2 / s^2
	UNITS_Sievert,   // m^2 / s^2
	// other
	UNITS_Katal,    // mol/s
	UNITS_dB,
	UNITS_Neper,

	// prefixes
	UNITS_Quetta, // 10^30
	UNITS_Ronna , // 10^27
	UNITS_Yotta,  // 10^24
	UNITS_Zetta,  // 10^21
	UNITS_Exa,    // 10^18
	UNITS_Peta,   // 10^15
	UNITS_Tera,   // 10^12
	UNITS_Giga,   // 10^9
	UNITS_Mega,   // 10^6
	UNITS_Kilo,   // 10^3
	UNITS_Hecto,  // 10^2
	UNITS_Deca,   // 10^1
	UNITS_Deci,   // 10^-1
	UNITS_Centi,  // 10^-2
	UNITS_Milli,  // 10^-3
	UNITS_Micro,  // 10^-6
	UNITS_Nano,   // 10^-9
	UNITS_Pico,   // 10^-12
	UNITS_Femto,  // 10^-15
	UNITS_Atto,   // 10^-18
	UNITS_Zepto,  // 10^-21
	UNITS_Yocto,  // 10^-24
	UNITS_Ronto,  // 10^-27
	UNITS_Quecto, // 10^-30
	
	UNITS_MAX
};


typedef int Unit;

//------------------------------------- SimpleUnit ----------------------------------------
class SimpleUnit
{
  public:
	int id;
	int category = 0; // ie, one of the 7 SI units
	char *label; // translated name for menus
	PtrStack<char> names; //0,1,2 are short name, singular, plural
	
	double scaling; // this value * scaling = base unit value
	double offset = 0.0;
	int relative_to = 0; //TODO: if non-zero, UnitManager uses this key for relative scaling, eg 1em = 12pt. else use default for category

	// SimpleUnit *next;

	SimpleUnit();
	virtual ~SimpleUnit();

	// virtual SimpleUnit *find(const char *name,int len=-1);
	// virtual SimpleUnit *find(int units);
};

class UnitCategory
{
  public:
  	PtrStack<SimpleUnit> units;

  	int default_units_index = -1;
	int category = UNITS_None;

  	UnitCategory() {}
  	~UnitCategory() {}
};

class UnitManager : public anObject
{
  protected:
	// int defaultunits = UNITS_None;
  	// SimpleUnit *units = nullptr;

  	int default_category = UNITS_Length;
  	PtrStack<UnitCategory> categories;

  	UnitCategory *FindCategory(int category);

  public:
  	UnitManager(bool install_default = true);
  	virtual ~UnitManager();

  	// default funcs for UNITS_Length:
  	virtual int NumberOfUnits(int category = UNITS_Default);
	virtual const SimpleUnit *Find(int id, int category = UNITS_Any);
	virtual const SimpleUnit *FindByName(const char *name, int len, int category = UNITS_Any);
  	virtual int UnitId(const char *name, int len=-1, int category = UNITS_Any);
	virtual const char *UnitName(int uid, int category = UNITS_Any);
	virtual int UnitInfo(const char *name, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret, double *offset, int category = UNITS_Any);
	virtual int UnitInfoId(int id, double *scale, char **shortname, char **singular,char **plural, const char **label_ret, double *offset, int category = UNITS_Any);
	virtual int UnitInfoIndex(int index, int *iid, double *scale, char **shortname, char **singular,char **plural, const char **label_ret, double *offset, int category = UNITS_Default);
	virtual double GetFactor(int fromunits, int tounits, double *offset_ret = nullptr, int category = UNITS_Any);
	virtual int DefaultUnits(int category = UNITS_Default);
	virtual int DefaultUnits(const char *units, int category = UNITS_Default);
	virtual int DefaultUnits(int units, int category = UNITS_Default);
	virtual double Convert(double value, const char *from, const char *to, int category = UNITS_Any, int *error_ret = nullptr);
	virtual double Convert(double value, int from_id, int to_id, int category = UNITS_Any, int *error_ret = nullptr);
	virtual int PixelSize(double pixelsize, int intheseunits); // for length only

	//------------ new 
	int BuildFor(int category);
	int AddUnits(int category, int nid, double scale, double offset, const char *shortname, const char *singular,const char *plural, const char *nlabel = nullptr);

	bool AddDerivedUnit(int nid, double scale, double offset,
						const char *shortname, const char *singular,const char *plural, const char *nlabel,
						int num_base_units, ...);
};


//------------------------------------- CreateDefaultUnits() ----------------------------------------
UnitManager *CreateDefaultUnits(UnitManager *units = nullptr);
UnitManager *CreateTemperatureUnits(UnitManager *units = nullptr);
UnitManager *GetUnitManager();
void SetUnitManager(UnitManager *manager);


} // namespace Laxkit

#endif
