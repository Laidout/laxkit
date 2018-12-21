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
//    Copyright (C) 2018 by Tom Lechner
//


#include <lax/configured.h>
#include <lax/laxgm.h>

#include <iostream>
using namespace std;
#define DBG
	



#ifdef LAX_USES_GRAPHICSMAGICK

//-----------------------------------------------------------------------------------------------
//--------------------------- InitLaxGraphicsMagick() when YES GraphicsMagick ----------------------------
//-----------------------------------------------------------------------------------------------



#include <lax/anxapp.h>
#include <lax/laximages-gm.h>


namespace Laxkit {


//--------------------------- InitLaxGraphicsMagick() --------------------------------------
//! Initialize GraphicsMagick2 using settings in anXApp::app.
/*! \ingroup misc
 * This should be called after anXApp::init() has been called, and of course before using 
 * GraphicsMagick2 functions.
 *
 * This will set up the default graphics backend to GraphicsMagick. This means all the functions
 * like load_image, image_out, etc, will be based on GraphicsMagick calls.
 *
 * It sets display, visual, and colormap the the defaults of anXApp::app.
 * Also calls GraphicsMagick_set_cache_size(megabytes * 1024 * 1024), which sets the image cache size to 10megs.
 * You may call this function whenever you want to expand the cache, for instance.
 *
 * Also sets the default image functions (load_image(), for instance)
 * to GraphicsMagick based functions that expect LaxGraphicsMagickImage objects.
 *
 */
void InitLaxGraphicsMagick(bool with_backend)
{
	 //initialize settings within GraphicsMagick
	InitializeMagick(NULL);

	 //install GraphicsMagick loader
	GraphicsMagickLoader *loader=new GraphicsMagickLoader();
	ImageLoader::AddLoader(loader,-1);

	 //set various base functions
	if (with_backend) InitGraphicsMagick2Backend();
}

/*! This will set up the default graphics backend to GraphicsMagick. This means all the functions
 * like load_image, image_out, etc, will be based on GraphicsMagick calls.
 */
void InitGraphicsMagick2Backend()
{
	 //set up default image functions
	default_image_type = laxGraphicsMagick_image_type;
}



} //namespace Laxkit




#else
//-----------------------------------------------------------------------------------------------
//--------------------------- InitLaxGraphicsMagick() when NO GraphicsMagick --------------------------------------
//-----------------------------------------------------------------------------------------------

namespace Laxkit {

//! GraphicsMagick2 support not compiled in, this just prints a warning and returns.
/*! \ingroup misc
 */
void InitLaxGraphicsMagick(int megabytes, bool with_backend) 
{
	printf(" ** Warning! InitLaxGraphicsMagick() was called, but "
			" GraphicsMagick support was not compiled into the Laxkit.\n");
}

void InitGraphicsMagick2Backend() 
{
	printf(" ** Warning! InitGraphicsMagick2Backend() was called, but "
			" GraphicsMagick2 support was not compiled into the Laxkit.\n");
}

} // namespace Laxkit



#endif //ifdef LAX_USES_GraphicsMagick



