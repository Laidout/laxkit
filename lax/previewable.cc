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
//    Copyright (C) 2016 by Tom Lechner
//

#include <lax/previewable.h>

#include <iostream>
using namespace std;

#define DBG


namespace Laxkit {

//--------------------------------- Previewable ----------------------------

/*! \class Previewable
 *
 * Standardizes getting little preview images from various kinds of objects.
 */

Previewable::Previewable()
{
	preview=NULL;
	previewtime=0;
    modtime=0;
}

Previewable::~Previewable()
{
	if (preview) preview->dec_count();
}

void Previewable::touchContents()
{
    previewtime=0; //time() doesn't change often enough, so we have to force this to 0..
    modtime=time(NULL);
}

LaxImage *Previewable::GetPreview()
{
	if (previewtime<modtime || !preview) GeneratePreview(-1,-1);
    return preview;
}

/*! Set up a LaxImage to hold a preview, then call renderToBufferImage() to 
 * actually render the preview.
 */
int Previewable::GeneratePreview(int w, int h)
{
   if (preview) {
        if (w<=0 && h<=0) {
            w=preview->w();
            h=preview->h();
        }
         //make the desired preview dimensions approximately sympatico with
         //the actual dimensions of the object
        if ((w>h && (maxx-minx)<(maxy-miny))) h=0;
        else if ((w<h && (maxx-minx)>(maxy-miny))) w=0;
    }

    int maxdim = maxPreviewSize();

    if (w<=0 && h<=0) {
        if (maxx-minx>maxy-miny) w=maxdim;
		else h=maxdim;
    }
    if (w<=0 && h>0) w=(maxx-minx)*h/(maxy-miny);
    else if (w>0 && h<=0) h=(maxy-miny)*w/(maxx-minx);

    if (w<=0) w=1;
    if (h<=0) h=1;

     //protect against growing sizes...
    if (w>maxdim) {
        double aspect=(double)h/w;
        w=maxdim;
        h=maxdim*aspect;
        if (h<=0) h=1;
    }
    if (h>maxdim) {
        double aspect=(double)w/h;
        h=maxdim;
        w=maxdim*aspect;
        if (w<=0) w=1;
    }

 	//if (preview && (w!=preview->w() || h!=preview->h())) {
    if (preview && ((float)w/preview->w()>1.05 || (float)w/preview->w()<.95 ||
                    (float)h/preview->h()>1.05 || (float)h/preview->h()<.95)) {
         //delete old preview and make new only when changing size of preview more that 5%-ish in x or y
        preview->dec_count();
        preview=NULL;
        DBG cerr <<"removed old preview..."<<endl;
    }

    if (!preview) {
        DBG cerr <<"old preview didn't exist, so creating new one..."<<endl;
        preview=create_new_image(w,h);
    }

    if (renderToBufferImage(preview)==0) {
		 previewtime=time(NULL);

	} else {
         //render direct to image didn't work, so try the old style render to char[] buffer...
		previewtime=0;
        //unsigned char *buffer = preview->getImageBuffer();
        //renderToBuffer(buffer,w,h,w*4,8,4);
        //preview->doneWithBuffer(buffer);
    	//previewtime=time(NULL);

    }

	return (previewtime>=modtime); 
}


} //namespace Laxkit


