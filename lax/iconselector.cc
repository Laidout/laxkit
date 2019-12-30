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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//


#include <lax/iconselector.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//----------------------------- IconBox ---------------------------------

/*! \class IconBox
 * \brief Internal node type for IconSelector.
 *
 * Holds an icon, possibly a black and white version of the icon, and a label.
 */

	
/*!  Does NOT call inc_count() on img. Transfers pointer to image via SetBox().
 */
IconBox::IconBox(const char *nlabel,LaxImage *img,int nid)
	: SelBox(nid)
{ 
	id=nid;
	label=NULL;
	image=bwimage=NULL;
	SetBox(nlabel,img,NULL);
}

//! Destructor, deletes the label, and calls dec_count() on image and bwimage.
IconBox::~IconBox() 
{
	if (label) delete[] label;
	if (image)  image  ->dec_count();
	if (bwimage) bwimage->dec_count();
}

//! Set the label and images to the given label and images.
/*! Return 0 on success. This does not alter the metric info.
 *
 * Does NOT Call inc_count() on the new images, but DOES call dec_count()
 * on the old image and bwimage.
 */
int IconBox::SetBox(const char *nlabel,LaxImage *img,LaxImage *bw) 
{
	makestr(label,nlabel);
	if (image) image->dec_count(); 
	if (bwimage) bwimage->dec_count();
	image=bwimage=NULL;
	
	bwimage=bw;
	image=img;
	
	return 0;
}


//------------------------------ IconSelector --------------------------------

/*! \class IconSelector
 * \brief A selector using boxes with a label and/or an icon.
 *
 * Supports the same layout options as BoxSelector, using the same style defines
 * with the addition of this one:
 *
 * \code
 *  #define STRICON_STR_ICON   (1<<31)  <-- put the icon after the string, default is other way
 * \endcode
 *
 * \todo
 *   on off mover-on mover-off grayed 
 *   *** must have a make_gray_pixmap
 *   *** MUST coordinate all the states in various controls to use new LAX_* defines
 */
/*! \var int IconSelector::padg
 * \brief Pad to put around the label, whether or not an icon is present. (the icon is not padded)
 */



IconSelector::IconSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int npad,
						int npadg //!< npadg is the padding to put around a label. Icons are not padded.
						) //npadg=5
		: BoxSelector(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendmes,npad)
{
	// npad is the inset, padg is space between icon and text
	padg = npadg;

	if (win_style&STRICON_STR_ICON) labelstyle=LAX_TEXT_ICON;
	else labelstyle=LAX_ICON_TEXT;
}

//! Empty destructor.
IconSelector::~IconSelector()
{}

//! Loads image, then calls FillBox(IconBox *,const char *,LaxImage *,int).
void IconSelector::FillBox(IconBox *b,const char *nlabel,const char *filename,int nid)
{
	FillBox(b,nlabel, ImageLoader::LoadImage(filename),nid);
}

//! Fill the given box with the label, and the icon from filename.
/*! This function is separated from AddBox to make it very slightly easier for implementing
 *  TabFrame***???, which is different only in that it has an extra anXWindow pointer member
 *  
 *  Sets the preferred metrics to be exactly equal to the bounds of the icon+label.
 *  sync() must be called after AddBox sometime to layout everything. Note that the final
 *  defininte width and height may not be these ideal width and height.
 *
 *  b is supposed to be a new, blank box, whose members are NULL.
 */
void IconSelector::FillBox(IconBox *b,const char *nlabel,LaxImage *img,int nid)
{
	if (!b) return;

	 // get icons and set w,h
	int iw=0,ih=0;
	if (img) {
		b->image=img;
		iw=b->image->w();
		ih=b->image->h();
		//if (makebw) ; //*** make the black and white version
	}
	
	double tw=0,th=0;
	if (nlabel) tw = win_themestyle->normal->Extent(nlabel,-1,&tw,&th,NULL,NULL) + 2*padg;
	
	b->w(tw+iw);
	b->pw(tw+iw);
	b->h(th>ih?th:ih);
	b->ph(th>ih?th:ih);
	b->id=nid;
	makestr(b->label,nlabel);

	b->pad=bevel;
}

//! Just returns AddBox(NULL,load_image(filename),makebw).
int IconSelector::AddBox(const char *nlabel,const char *filename,int nid)
{
	return AddBox(NULL, ImageLoader::LoadImage(filename),nid);
}

//! Add box and return its index.
/*! This calls FillBox to set the new boxes elements.
 */
int IconSelector::AddBox(const char *nlabel,LaxImage *img,int nid)
{
	IconBox *newbox=new IconBox();
	FillBox(newbox,nlabel,img,nid);
	wholelist.push(newbox);
	needtodraw=1;
	
	DBG cerr <<"IconSelector AddBox(";
	DBG if (nlabel) cerr <<nlabel<<',';
	DBG if (img && img->filename) cerr <<img->filename; else cerr <<"(no image)";
	DBG cerr <<"): "<<wholelist.n<<endl;

	return wholelist.n-1;
}

//! Draw the icon box.
void IconSelector::drawbox(int which)
{
	if (which<0 || which>=wholelist.n || !win_on) return;
	IconBox *b=dynamic_cast<IconBox *>(wholelist.e[which]);
	if (!b) return;
	
	Displayer *dp = GetDisplayer();

	dp->NewFG(which==hoverbox ? win_themestyle->bghover : win_themestyle->bg);
	dp->drawrectangle(b->x() - b->pad,  b->y() - b->pad,    b->w() + 2*b->pad,  b->h() + 2*b->pad, 1);

	 // Set  tx,ty  px,py
	int w,h,tx,ty,ix,iy,dx,dy;
	LaxImage *i=b->image;
	const char *l=b->label;
	get_placement(i,win_themestyle->normal,l,padg,labelstyle,&w,&h,&tx,&ty,&ix,&iy);
	dx=b->x()+(b->w()-w)/2;
	dy=b->y()+(b->h()-h)/2;

	 // draw the stuff
	if (i && ix!=LAX_WAY_OFF) {
		ix+=dx;
		iy+=dy;
		dp->imageout(i,ix,iy);
		i->doneForNow();
	}
	if (l && tx>LAX_WAY_OFF) {
		tx+=dx;
		ty+=dy;
		dp->NewFG(win_themestyle->fg);
		dp->textout(tx,ty, l,-1, LAX_LEFT|LAX_TOP);
	}
	
	dp->drawBevel(b->pad,highlight,shadow,b->state, b->x()-b->pad,b->y()-b->pad, b->w()+2*b->pad,b->h()+2*b->pad);
}



} // namespace Laxkit


