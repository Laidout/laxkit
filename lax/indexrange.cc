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
//    Copyright (C) 2020 by Tom Lechner
//


#include "indexrange.h"

#include <lax/strmanip.h>


using namespace Laxkit;


namespace Laxkit {


/*! Class to simplify parsing and access to noncontinuous rangess
 * defined by ints, such as "1-4, 10-20".
 *
 * IndexRange defines sequences of elements in the range [0,max).
 * Ranges can potentially overlap, or go backwards.
 *
 * Syntax being: s* [s* i s* [ rangechar s* i s* ] ]*
 * where s is whitespace and i is a positive or negative integer.
 * Negative integers are from the end, so -1 is the final index (max-1),
 * rangechar is "-" or ":", but you can also use a custom one
 * by setting range_marker, such as "..".
 *
 * Ranges are stored by integer pairs in indices.
 */

IndexRange::IndexRange()
{
	parse_from_one = false;
	curi = 0;
	cur = 0;
	max = -1;
	//min = 0;
	range_marker = newstr("-"); //used in ToString, and ALSO checked in Parse(), along with '-' and ':'
}

IndexRange::IndexRange(const IndexRange &r)
  : IndexRange()
{
	*this = r;
}

IndexRange::~IndexRange()
{
	delete[] range_marker;
}

IndexRange &IndexRange::operator=(const IndexRange &r)
{
	Clear();
	parse_from_one = r.parse_from_one;
	max = r.Max();
	makestr(range_marker, r.RangeMarker());
	for (int c=0; c<r.NumRanges(); c++) {
		int i1,i2;
		r.GetRange(c, &i1, &i2);
		AddRange(i1,i2);
	}

	return *this;
}

/*! Returns true if any values had to be clamped. If !clamp_existing_to_max, then always return false.
 */
bool IndexRange::Max(int nmax, bool clamp_existing_to_max) 
{
	max = nmax;
	bool changed = false;
	if (clamp_existing_to_max) {
		for (int c=0; c< indices.n; c++) {
			if      (indices.e[c] >= max) { changed = true; indices.e[c] = max-1; }
			else if (indices.e[c] < -max) { changed = true; indices.e[c] = -max; }
		}
	}
	return changed; 
}

const char *IndexRange::RangeMarker(const char *marker)
{
	if (isblank(marker)) return range_marker;
	makestr(range_marker, marker);
	return range_marker;
}

/*! Return whether index is found within any range.
 */
bool IndexRange::Contains(int index, int *range_ret)
{
	for (int c=0; c<indices.n; c+=2) {
		int i1 = indices.e[c];
		int i2 = indices.e[c+1];
		if (i1 < 0) i1 = max + i1;
		if (i2 < 0) i2 = max + i2;

		if (i2 >= i1) {
			if (index >= i1 && index <= i2) {
				if (range_ret) *range_ret = c/2;
				return true;
			}
		} else {
			if (index >= i2 && index <= i1) {
				if (range_ret) *range_ret = c/2;
				return true;
			}
		}
	}
	return false;
}

//for qsort
static int cmp_int(const void *i1, const void *i2)
{
	if (*((int*)i1) > *((int*)i2)) return 1;
	if (*((int*)i1) < *((int*)i2)) return -1;
	return 0;
}

/*! Return a new int[] with the page indices (count from 0), sorted least to greatest, with no duplicates.
 * Warning, this currently uses Start() and Next().
 */
int *IndexRange::GetSortedList(int *n_ret)
{
	if (indices.n == 0) {
		*n_ret = 0;
		return nullptr;
	}
	NumStack<int> nums;
	for (int c = Start(); c >= 0; c = Next()) {
		nums.pushnodup(c);
	}
	int *ii = nums.extractArray(n_ret);

	if (*n_ret != 0) qsort(ii, *n_ret, sizeof(int), cmp_int);

	return ii;
}

/*! Return true for succes, or false for which not found.
 */
bool IndexRange::GetRange(int which, int *start, int *end) const
{
	if (which*2 >= indices.n) return false;
	*start = indices.e[which*2];
	*end   = indices.e[which*2+1];
	return true;
}

bool IndexRange::SetRange(int which, int start, int end)
{
	if (which < 0 || which >= indices.n/2) return false;
	indices.e[which*2] = start;
	indices.e[which*2+1] = end;
	return true;
}

bool IndexRange::RemoveRange(int which)
{
	if (which < 0 || which >= indices.n/2) return false;
	indices.remove(which*2);
	indices.remove(which*2);
	str.Clear();
	return true;
}

/*! Add a range from start to end (counting from 0).
 * If where<0, then add to the end, else insert at that position (at where*2 in indices).
 * Warning: does not bounds checking against max.
 * Returns the actual index of the range after inserting.
 */
int IndexRange::AddRange(int start, int end, int where)
{
	if (where < 0 || where >= indices.n) where = indices.n;
	indices.push(start, where*2);
	indices.push(end, where*2+1);
	return where;
}

/*! Sum of the number of indices in each range.
 * Note that if ranges overlap, the overlapping elements are counted more than once.
 */
int IndexRange::NumInRanges() const
{
	int n=0;
	for (int c=0; c<indices.n; c+=2) {
		int i1 = indices.e[c];
		int i2 = indices.e[c+1];
		if (i1 < 0) i1 = max + i1;
		if (i2 < 0) i2 = max + i2;

		if (i2 >= i1)
			n += i2 - i1 + 1;
		else
			n += i1 - i2 + 1;
	}

	return n;
}

/*! Return the number of odd indices. Warning: this uses Start() and Next().
 */
int IndexRange::NumOdd()
{
	int n = 0;
	for (int c = Start(); c >= 0; c = Next())
		if (c % 2 == 1) n++;
	return n;
}

/*! Return the number of even indices. Warning: this uses Start() and Next().
 */
int IndexRange::NumEven()
{
	int n = 0;
	for (int c = Start(); c >= 0; c = Next())
		if (c % 2 == 0) n++;
	return n;
}

int IndexRange::Start()
{
	if (indices.n == 0) {
		curi = -1;
		cur = -1;
	} else {
		curi = 0;
		curi2 = 0;
		cur = indices[0];
		if (cur < 0) cur = max + cur;
	}
	return cur;
}

/*! Return -1 if all done.
 */
int IndexRange::Next()
{
	if (curi < 0) return -1;
	int i1 = indices[curi];
	int i2 = indices[curi+1];
	if (i1 < 0) i1 = max + i1;
	if (i2 < 0) i2 = max + i2;

	if (cur == i2) {
		//advance to next section
		curi += 2;
		curi2 = 0;
		if (curi >= indices.n) return -1;
		cur = indices[curi];
		if (cur < 0) cur = max + cur;

	} else {
		//next in current range
		if (i2 > i1) {
			cur++;
			curi2++;

		} else {
			cur--;
			curi2++;
		}
	}

	return cur;
}

/*! Return -1 if all done.
 */
int IndexRange::Previous()
{
	if (curi < 0) return -1;
	int i1 = indices[curi];
	int i2 = indices[curi+1];
	if (i1 < 0) i1 = max + i1;
	if (i2 < 0) i2 = max + i2;

	if (cur == i1) {
		//advance to next section
		curi -= 2;
		curi2 = 0;
		if (curi < 0) return -1;
		cur = indices[curi+1];
		if (cur < 0) cur = max + cur;

	} else {
		//next in current range
		if (i2 > i1) {
			cur--;
			curi2++;

		} else {
			cur++;
			curi2++;
		}
	}

	return cur;
}

int IndexRange::Current()
{
	return cur;
}

/*! Set the internal counter to the end.
 */
int IndexRange::End()
{
	if (indices.n == 0) return -1;
	int i = indices[indices.n-1];
	if (i < 0) i = max + i;
	cur = i;
	curi = indices.n-2;
	curi2 = 0;
	return i;
}

void IndexRange::Clear()
{
	cur = -1;
	curi = curi2 = 0;
	indices.flush();
	str.Clear();
}

/*! Clears current ranges before parsing. Return 0 for success, or nonzero for error.
 * Expects string like "0-5, 9-7, 34". Any whitespace ok. Commas are ignored.
 * If second number is below first, then that range will count down instead of up.
 * Negative numbers mean count from end, so -1 means (max-1).
 *
 * If parse_from_one, then a number 1 gets translated to index 0.
 *
 * On fail, ranges are not changed.
 *
 * Return 0 for success, nonzero for parse error.
 */
int IndexRange::Parse(const char *range, const char **end_ptr, bool use_labels)
{
	const char *p = range;
	char *endptr;
	const char *cendptr;
	int i,i2;
	NumStack<int> newi;

	while (*p) {
		while (isspace(*p)) p++;
		if (!(*p)) break;

		if (use_labels) {
			int ii = LabelToIndex(p, &cendptr);
			if (cendptr == p) { //none found!
				break;
			}
			p = cendptr;
			i = ii;
		} else {
			i = strtol(p, &endptr, 10);
			if (endptr == p) { //number not found
				//if (*p == '-' && max >= 0) i = -1;  <-- should we allow like "-10" or "10-" ??
				//else break;
				break;
			}
			p = endptr;
			if (i > 0 && parse_from_one) i--;
		}

		while (isspace(*p)) p++;

		if (!strncmp(range_marker, p, strlen(range_marker))) {
			p += strlen(range_marker);
		} else if (*p == '-' || *p == ':') {
			p++;
		} else if (*p == '.') {
			while (*p == '.') p++; //skip ".." (or any number of . because why not)
		} else {
			//no range marker found!
			newi.push(i);
			newi.push(i);
			while (isspace(*p) || *p == ',') p++;
			continue; //no range marker found, assume end of current range
		}

		while (isspace(*p)) p++;

		//there is a second number
		if (use_labels) {
			int ii = LabelToIndex(p, &cendptr);
			if (cendptr == p) { //none found!
				break;
			}
			p = cendptr;
			i2 = ii;
		} else {
			i2 = strtol(p, &endptr, 10);
			if (endptr == p) { //number not found, this is an error!
				newi.flush();
				break;
			}
			p = endptr;
			if (i2 > 0 && parse_from_one) i2--;
		}

		newi.push(i);
		newi.push(i2);

		while (isspace(*p) || *p == ',') p++;
		//if (*p != ',') break; //no more ranges
	}

	while (isspace(*p)) p++;
	if (*p != '\0') {
		//error! string contains something that couldn't be parsed
		newi.flush();
	}
	if (newi.n == 0) { //parse problem!
		if (end_ptr) *end_ptr = range;
		return -1; 
	}

	str.Clear();
	indices = newi;
	return 0;
}

/*! If absolute, translate negative numbers i to (max+i).
 * If use_labels, negative numbers will be output as the same negative number.
 * If use_cache and str is not empty, then return that. Otherwise rebuilds to str.
 */
const char *IndexRange::ToString(bool absolute, bool use_labels, bool use_cache)
{
	if (use_cache && !str.IsEmpty()) return str.c_str();
	str.Clear();

	Utf8String scratch;

	if (use_labels) {
		Utf8String l1, l2;
		int i1, i2;

		for (int c=0; c<indices.n; c+=2) {
			i1 = indices[c];
			i2 = indices[c+1];
			IndexToLabel(i1, l1, absolute);
			IndexToLabel(i2, l2, absolute);

			if (i1 == i2) scratch.Sprintf("%s", l1.c_str());
			else scratch.Sprintf("%s%s%s", l1.c_str(), range_marker, l2.c_str());
			str.Append(scratch);
			if (c < indices.n-2) str.Append(", ");
		}

	} else { //output as ints
		int i1,i2;
		for (int c=0; c<indices.n; c+=2) {
			i1 = indices[c];
			i2 = indices[c+1];

			if (absolute) {
				if (i1 < 0) i1 = max + i1;
				if (i2 < 0) i2 = max + i2;
			}
			if (parse_from_one) {
				if (i1 >= 0) i1++;
				if (i2 >= 0) i2++;
			}

			if (i1 == i2) scratch.Sprintf("%d", i1);
			else scratch.Sprintf("%d%s%d", i1, range_marker, i2);
			str.Append(scratch);
			if (c < indices.n-2) str.Append(", ");
		}
	}

	return str.c_str() != nullptr ? str.c_str() : "";
}

/*! From absolute index i (starting from 0), return corresponding string.
 * Default is same as i, or i+1 if parse_from_one.
 * Subclasses might redefine to some other label translation.
 *
 * If absolute and i<0, then make string max-i, or max-i if parse_from_one.
 */
void IndexRange::IndexToLabel(int i, Utf8String &str, bool absolute)
{
	if (i<0 && absolute) i = max+i;
	if (parse_from_one && i >= 0) i++;
	str.Sprintf("%d", i);
}

/*! From label string, return index counting from 0.
 * Default is same as i, or i+1 if parse_from_one. Negative numbers passed as is.
 * Subclasses might redefine to some other label translation.
 *
 * If parse_from_one, subtract 1 from number read.
 */
int IndexRange::LabelToIndex(const char *label, const char **end_ptr)
{
	char *endptr;
	int i = strtol(label, &endptr, 10);
	*end_ptr = endptr;
	if (parse_from_one && i > 0) i--;
	return i;
}

} //namespace Laxkit

