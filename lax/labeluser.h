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
#ifndef _LAX_LABELUSER_H
#define _LAX_LABELUSER_H

namespace Laxkit {

/*! Classes that incorporate a label should use this so that labels can easily be aligned.
 */
class LabelUser
{
  public:
	LabelUser() {}
	virtual ~LabelUser() {}

	virtual double LabelWidth() = 0;
	virtual double LabelWidth(double newwidth) = 0;

	virtual void Label(const char *newLabel) = 0;
	virtual const char *Label() = 0;

	virtual bool LabelOnLeft() = 0; //true: (label) (stuff),  false: (stuff) (label)
	virtual bool LabelOnLeft(bool onLeft) = 0;

	virtual int LabelAlignment() = 0; //how to align label with the label width.  LAX_LEFT, LAX_RIGHT, or LAX_CENTER
	virtual int LabelAlignment(int newAlignment) = 0; // LAX_LEFT, LAX_RIGHT, or LAX_CENTER
};

} //namespace Laxkit

#endif

