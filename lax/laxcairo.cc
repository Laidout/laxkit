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
//    Copyright (C) 2013 by Tom Lechner
//


#include <lax/configured.h>
#include <lax/laxcairo.h>
#include <lax/laximages-cairo.h>
#include <lax/fontmanager.h>
#include <lax/displayer.h>

#include <iostream>
using namespace std;
#define DBG
	



#ifdef LAX_USES_CAIRO

namespace Laxkit {

//--------------------------- InitLaxCairo() --------------------------------------
//! Initialize Cairo for use in the Laxkit.
/*! \ingroup misc
 * This should be called after anXApp::init() has been called, and of course before using 
 * Cairo functions.
 *
 * This will set up the default graphics backend to use Cairo. This means all the functions
 * like load_image, image_out, etc, will be based on Cairo calls.
 *
 *
 * If with_backend, also sets the default image functions (load_image(), for instance)
 * to cairo based functions that expect LaxCairoImage objects.
 *
 */
void InitLaxCairo(bool with_backend)
{
	 //initialize settings within Cairo

	 //set various base functions
	if (with_backend) InitCairoBackend();
}

/*! This will set up the default graphics backend to cairo. This means all the functions
 * like load_image, image_out, etc, will be based on cairo calls.
 */
void InitCairoBackend()
{
	 //set up default image functions
	default_image_type = laxcairo_image_type;

	//save_image              = save_image_cairo;
	//load_image              = load_cairo_image;
	//load_image_with_preview = load_cairo_image_with_preview;
	//generate_preview_image  = laxcairo_generate_preview;

	//create_new_image  = create_new_cairo_image;
	//image_from_buffer = image_from_buffer_cairo;

	SetNewFontManagerFunc("cairo");
	SetNewDisplayerFunc("cairo");
}



} //namespace Laxkit




#else
//------------------------------------------------
//----------------  no LAX_USES_CAIRO  -----------
//------------------------------------------------


#include <lax/anxapp.h>
#include <lax/laximages-cairo.h>

namespace Laxkit {
	
//--------------------------- InitLaxCairo() when no Laxkit--------------------------------------

//! Cairo support not compiled in, this just prints a warning and returns.
/*! \ingroup misc
 */
void InitLaxCairo() 
{
	printf(" ** Warning! InitLaxCairo() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
}

void InitCairoBackend() 
{
	printf(" ** Warning! InitCairoBackend() was called, but "
			" Cairo support was not compiled into the Laxkit.\n");
}

} // namespace Laxkit

#endif //ifdef LAX_USES_CAIRO


