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
#ifndef INDEXRANGE_H
#define INDEXRANGE_H


#include <lax/utf8string.h>
#include <lax/lists.h>


namespace Laxkit {


class IndexRange
{
    int curi, curi2; //range, index in that range
	int cur; //current actual index
	//int min;
	int max;
	char *range_marker;
	Laxkit::Utf8String str;
	Laxkit::NumStack<int> indices;

  public:
	bool parse_from_one; //whether int range when parsing/tostring starts at 1 (true) or 0 (false, default)

	IndexRange();
	virtual ~IndexRange();

	int NumRanges() { return indices.n/2; }
	int NumInRanges();
	int Start(); //note: loop funcs ignore parse_from_one
	int Next();
	int Previous();
	int Current();
	int End(); //note: this will set cur, don't use it to check current against end
	virtual int Max(int nmax) { max = nmax; return max; }
	virtual int Max() { return max; }
	//virtual int Min(int nmin) { min = nmin; }
	//virtual int Min() { return min; }
	virtual bool GetRange(int which, int *start, int *end);
	virtual bool SetRange(int which, int start, int end);
	virtual bool RemoveRange(int which);
	virtual int AddRange(int start, int end, int where=-1);
	virtual const char *RangeMarker() { return range_marker; }
	virtual const char *RangeMarker(const char *marker);

	virtual void Clear();
	virtual int Parse(const char *range, const char **end_ptr, bool use_labels);
	virtual const char *ToString(bool absolute, bool use_labels, bool use_cache);

	virtual void IndexToLabel(int i, Laxkit::Utf8String &str, bool absolute);
	virtual int LabelToIndex(const char *label, const char **end_ptr);
};

} //namespace Laxkit



#endif

