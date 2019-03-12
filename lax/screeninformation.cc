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
//    Copyright (C) 2004-2013 by Tom Lechner
//

#include <lax/screeninformation.h>
#include <lax/strmanip.h>



namespace Laxkit {


//-------------------------- ScreenInformation ----------------------------------------
/*! \class ScreenInformation
 * \brief Information about screens and monitors.
 */

/*! Return monitor matching this name.
 */
ScreenInformation *ScreenInformation::Get(char *which)
{
	if (which == nullptr || which[0]=='\0') return nullptr;
	if (!strcmp(which, name)) return this;
	if (!next) return nullptr;
	return next->Get(which);
}

ScreenInformation::~ScreenInformation()
{
	delete name;
	if (next) delete next;
}


} //namespace Laxkit

