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
//    Copyright (C) 2004-2012,2015 by Tom Lechner
//
#ifndef _LAX_SOMEDATAFACTORY_H
#define _LAX_SOMEDATAFACTORY_H

#include <lax/objectfactory.h>



namespace LaxInterfaces {

enum LaxInterfaceDataTypes {
	LAX_BEZDATA,
	LAX_BEZNETDATA,
	LAX_COLORPATCHDATA,
	LAX_ELLIPSEDATA,
	LAX_ENGRAVERFILLDATA,
	LAX_GRADIENTDATA,
	LAX_IMAGEDATA,
	LAX_IMAGEPATCHDATA,
	LAX_LINESDATA,
	LAX_PATCHDATA,
	LAX_PATHSDATA,
	LAX_RECTDATA,
	LAX_SOMEDATA,
	LAX_SOMEDATAREF,
	LAX_CAPTIONDATA,
	LAX_TEXTONPATH,
	LAX_GROUPDATA,
	LAX_VORONOIDATA,

	LAX_DATA_MAX
};

Laxkit::ObjectFactory *somedatafactory();




} //namespace LaxInterfaces


#endif

