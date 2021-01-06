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


#include <lax/valueconstraint.h>
#include <lax/strmanip.h>

#include <cstring>
#include <cctype>
#include <cmath>

namespace Laxkit {

//------------------------ ValueConstraint --------------------------------

/*! \class ValueConstraint
 * Define a constraint that can be used to clamp values, or otherwise help make Value
 * objects fit constraints. Base class is built for integers and doubles. More specialized
 * behavior needs to be subclassed.
 */

ValueConstraint::ValueConstraint()
{
	value_type = PARAM_Double;
	default_value = min = max = 0;
	step = 1;
	mintype = PARAM_No_Maximum;
	maxtype = PARAM_No_Minimum;
	steptype = PARAM_Step_Mult;
}

ValueConstraint::ValueConstraint(int defaultv, int min)
 : ValueConstraint()
{
	default_value = defaultv;
	value_type = PARAM_Integer;
	mintype = PARAM_Min_Clamp;
	steptype = PARAM_Step_Add;
	this->min = min;
}

ValueConstraint::ValueConstraint(int defaultv, int min, int max)
 : ValueConstraint()
{
	default_value = defaultv;
	mintype = PARAM_Min_Clamp;
	maxtype = PARAM_Max_Clamp;
	steptype = PARAM_Step_Add;
	this->min = min;
	this->max = max;
	step = (max-min)/255;
	if (fabs(step)<1) step = 1;
}

ValueConstraint::ValueConstraint(double defaultv, double min)
 : ValueConstraint()
{
	default_value = defaultv;
	value_type = PARAM_Integer;
	mintype = PARAM_Min_Clamp;
	steptype = PARAM_Step_Add;
	this->min = min;
}

ValueConstraint::ValueConstraint(double defaultv, double min, double max)
 : ValueConstraint()
{
	default_value = defaultv;
	mintype = PARAM_Min_Clamp;
	maxtype = PARAM_Max_Clamp;
	steptype = PARAM_Step_Add;
	this->min = min;
	this->max = max;
}


void ValueConstraint::SetBounds(double nmin, Constraint nmin_type, double nmax, Constraint nmax_type)
{
	min = nmin;
	mintype = nmin_type;
	max = nmax;
	maxtype = nmax_type;
}

/*! A single range like:
 * <pre>
 *   "(0]"      unbounded minimum, hard max of 0
 *   "[0)"      hard min of 0, unbounded maximum
 *   "()"       no bounds, same as "" or NULL.
 *   "(1)"      same as (), but min and max set to 1 instead of 0
 *   "[0, 1]"   hard minimum 0, hard max of 1
 *   "[.1, .9)" hard minimum .1, soft max of .9
 *   "(1,9]"    soft minimum 1 (just an interface hint, will not accually clamp), hard max of 9.
 * </pre>
 *
 * Return 0 for successfull set, or nonzero for parsing error.
 */
int ValueConstraint::SetBounds(const char *bounds)
{
	while (isspace(*bounds)) bounds++;

	if (!isblank(bounds) || !strncmp(bounds, "()", 2)) {
		mintype = PARAM_No_Minimum;
		maxtype = PARAM_No_Maximum;
		return 0;
	}

	if (*bounds == '(') mintype = PARAM_No_Minimum;
	else if (*bounds == '[') mintype = PARAM_Min_Clamp;
	else return 1;
	bounds++;

	char *end=NULL;
	double d = strtod(bounds, &end), d2 = 0;
	int n = 0;
	if (end != bounds) {
		n++;
		bounds = end;
		while (isspace(*bounds)) bounds++;
		if (*bounds == ',') {
			bounds++;
			d2 = strtod(bounds, &end), d2 = 0;
			if (bounds == end) {
				return 2;
			}
			n++;
		}
	}

	while (isspace(*bounds)) bounds++;

	if (*bounds == ')') maxtype = PARAM_No_Maximum;
	else if (*bounds == ']') maxtype = PARAM_Max_Clamp;

	if (n==0) {
		if (mintype != PARAM_No_Minimum || maxtype != PARAM_No_Maximum) return 3;
		min = max = 0;

	} else if (n==1) {
		min = max = d;

	} else {
		min = d;
		max = d2;
		if (mintype == PARAM_No_Minimum && maxtype == PARAM_No_Maximum) {
			mintype = PARAM_Min_Loose_Clamp;
			maxtype = PARAM_Max_Loose_Clamp;
		} else if (mintype == PARAM_No_Minimum && maxtype == PARAM_Max_Clamp) {
			mintype = PARAM_Min_Loose_Clamp;
		} else if (mintype == PARAM_Min_Clamp && maxtype == PARAM_No_Maximum) {
			maxtype = PARAM_Max_Loose_Clamp;
		}
	}

	return 0;
}

/*! Return 0 for ok, -1 for below min, 1 for above max.
 */
int ValueConstraint::Validity(double v, double *v_ret)
{
	if (mintype == PARAM_Min_Clamp && v < min) {
		if (v_ret) *v_ret = min;
		return -1;
	}
	if (maxtype == PARAM_Max_Clamp && v > max) {
		if (v_ret) *v_ret = max;
		return 1;
	}
	if (v_ret) *v_ret = v;
	return 0;
}

/*! Return 0 for ok, -1 for below min, 1 for above max.
 */
int ValueConstraint::Validity(int v, int *v_ret)
{
	if (mintype == PARAM_Min_Clamp && v < min) {
		if (v_ret) *v_ret = min;
		return -1;
	}
	if (maxtype == PARAM_Max_Clamp && v > max) {
		if (v_ret) *v_ret = max;
		return 1;
	}
	if (v_ret) *v_ret = v;
	return 0;
}

int ValueConstraint::SlideInt(int oldvalue, double numsteps)
{
	int newvalue = oldvalue;

	if (numsteps) {
		if (steptype == PARAM_Step_Add) newvalue += numsteps * step;
		else if (steptype == PARAM_Step_Adaptive_Add) newvalue += numsteps * step * (fabs(newvalue)>1 ? int(log10(fabs(newvalue)))*10+1 : 1);
		else if (steptype == PARAM_Step_Mult) {
			int vv = newvalue;
			if (newvalue == 0) {
				if (numsteps>0) newvalue = step-1; else newvalue = 1-step; //assumes step is like 1.1
			} else {
				if (numsteps>0) newvalue *= numsteps * step;
				else newvalue /= -numsteps * step;
			}
			if (vv == newvalue) {
				 //make sure it always moves by at least one
				if (numsteps>0) newvalue = vv+1;
				else newvalue = vv-1;
			}
		}
	}

	if (mintype == PARAM_Min_Clamp && newvalue < min) newvalue = min;
	else if (maxtype == PARAM_Max_Clamp && newvalue > max) newvalue = max;

	return newvalue;
}

double ValueConstraint::SlideDouble(double oldvalue, double numsteps)
{
	double newvalue = oldvalue;

	if (numsteps) {
		if (steptype == PARAM_Step_Add) newvalue += numsteps * step;
		else if (steptype == PARAM_Step_Adaptive_Add) newvalue += numsteps * step * (fabs(newvalue)>1 ? int(log10(fabs(newvalue)))*10+1 : 1);
		else if (steptype == PARAM_Step_Mult) {
			double vv = newvalue;
			if (newvalue == 0) {
				if (numsteps>0) newvalue = step-1; else newvalue = 1-step; //assumes step is like 1.1
			} else {
				if (numsteps>0) newvalue *= numsteps * step;
				else newvalue /= -numsteps * step;
			}
			if (vv == newvalue) {
				 //make sure it always moves by at least one
				if (numsteps>0) newvalue = vv+step;
				else newvalue = vv-step;
			}
		}
	}

	if (mintype == PARAM_Min_Clamp && newvalue < min) newvalue = min;
	else if (maxtype == PARAM_Max_Clamp && newvalue > max) newvalue = max;

	return newvalue;
}

} //namespace Laxkit
