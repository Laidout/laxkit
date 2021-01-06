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
//    Copyright (c) 2020 Tom Lechner
//
#ifndef _LAX_VALUECONSTRAINT_H
#define _LAX_VALUECONSTRAINT_H


namespace Laxkit {

//------------------------ ValueConstraint --------------------------------

class ValueConstraint
{
  public:
	enum Constraint {
		PARAM_None = 0,

		PARAM_No_Maximum,
		PARAM_No_Minimum,
		PARAM_Min_Loose_Clamp, //using the <, <=, >, >= should be hints, not hard clamp
		PARAM_Max_Loose_Clamp, //using the <, <=, >, >= should be hints, not hard clamp
		PARAM_Min_Clamp, //when numbers exceed bounds, force clamp
		PARAM_Max_Clamp, //when numbers exceed bounds, force clamp

		PARAM_Integer,
		PARAM_Double,

		//PARAM_Step_Adaptive_Mult,
		PARAM_Step_Adaptive_Add,
		PARAM_Step_Add,  //sliding does new = old + step, or new = old - step
		PARAM_Step_Mult, //sliding does new = old * step, or new = old / step

		PARAM_MAX
	};

	int value_type;
	Constraint mintype, maxtype, steptype;
	double min, max, step;
	double default_value;

	ValueConstraint();
	ValueConstraint(int defaultv, int min);
	ValueConstraint(int defaultv, int min, int max);
	ValueConstraint(double defaultv, double min);
	ValueConstraint(double defaultv, double min, double max);
	virtual ~ValueConstraint();

	virtual int Validity(double v, double *v_ret);
	virtual int Validity(int v, int *v_ret);
	virtual int SetBounds(const char *bounds); //a single range like "( .. 0]", "[0 .. 1]", "[.1 .. .9]", "{1..9]"
	virtual void SetBounds(double nmin, Constraint nmin_type, double nmax, Constraint nmax_type);
	virtual int SetStep(double nstep, Constraint nsteptype);

	virtual int SlideInt(int oldvalue, double numsteps);
	virtual double SlideDouble(double oldvalue, double numsteps);
};

} //namespace Laxkit

#endif
