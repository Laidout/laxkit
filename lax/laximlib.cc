//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Copyright (C) 2009-2010 by Tom Lechner
//

#include <X11/Xlib.h>

#include <lax/configured.h>
#include <lax/laximlib.h>

#include <iostream>
using namespace std;
#define DBG
	
#ifndef LAX_USES_IMLIB

//--------------------------- InitLaxImlib() when no Laxkit--------------------------------------

namespace Laxkit {

//! Imlib2 support not compiled in, this just prints a warning and returns.
/*! \ingroup misc
 */
void InitLaxImlib() 
{
	printf(" ** Warning! InitLaxImlib() was called, but "
			" Imlib2 support was not compiled into the Laxkit.\n");
}

void InitImlib2Backend() 
{
	printf(" ** Warning! InitImlib2Backend() was called, but "
			" Imlib2 support was not compiled into the Laxkit.\n");
}

} // namespace Laxkit




#else  //LAX_USES_IMLIB is defined...

#include <Imlib2.h>

#include <lax/anxapp.h>
#include <lax/laximlib.h>
#include <lax/laximages-imlib.h>

namespace Laxkit {
	
//--------------------------- InitLaxImlib() --------------------------------------
//! Initialize Imlib2 using settings in anXApp::app.
/*! \ingroup misc
 * This should be called after anXApp::init() has been called, and of course before using 
 * Imlib2 functions.
 *
 * This will set up the default graphics backend to Imlib. This means all the functions
 * like load_image, image_out, etc, will be based on Imlib calls.
 *
 * It sets display, visual, and colormap the the defaults of anXApp::app.
 * Also calls imlib_set_cache_size(megabytes * 1024 * 1024), which sets the image cache size to 10megs.
 * You may call this function whenever you want to expand the cache, for instance.
 *
 * Also sets the default image functions (load_image(), for instance)
 * to imlib based functions that expect LaxImlibImage objects.
 *
 */
void InitLaxImlib(int megabytes)
{
	 //initialize settings within Imlib
	imlib_context_set_display(anXApp::app->dpy);
	imlib_context_set_visual(anXApp::app->vis);
	imlib_context_set_colormap(DefaultColormap(anXApp::app->dpy, DefaultScreen(anXApp::app->dpy)));
	imlib_set_cache_size(megabytes * 1024 * 1024); // in bytes

	 //set various base functions
	InitImlib2Backend();
}

/*! This will set up the default graphics backend to Imlib. This means all the functions
 * like load_image, image_out, etc, will be based on Imlib calls.
 */
void InitImlib2Backend()
{
	 //set up default image functions
	default_image_type = laximlib_image_type;

	image_out         =laximlib_image_out;
	image_out_rotated =laximlib_image_out_rotated;
	image_out_skewed  =laximlib_image_out_skewed;
	image_out_matrix  =laximlib_image_out_matrix;

	load_image              =load_imlib_image;
	load_image_with_preview =load_imlib_image_with_preview;
	generate_preview_image  =laximlib_generate_preview;

	create_new_image  = create_new_imlib_image;
	image_from_buffer = image_from_buffer_imlib;
}



} //namespace Laxkit

#endif //ifdef LAX_USES_IMLIB


