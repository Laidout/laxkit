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
	id    = nid;
	label = nullptr;
	image = bwimage = nullptr;
	use_thing = false;
	manual_size = false; // else use actual icon size. things always use manual size
	img_w = img ? img->w() : 10;
	img_h = img ? img->h() : 10;
	SetBox(nlabel, img, nullptr);
}

//! Destructor, deletes the label, and calls dec_count() on image and bwimage.
IconBox::~IconBox() 
{
	if (label) delete[] label;
	if (image)   image  ->dec_count();
	if (bwimage) bwimage->dec_count();
}

//! Set the label and images to the given label and images.
/*! Return 0 on success. This does not alter the metric info.
 *
 * Takes img and bw, so does NOT Call inc_count() on the new images, but DOES call dec_count()
 * on the old image and bwimage.
 */
int IconBox::SetBox(const char *nlabel,LaxImage *img,LaxImage *bw) 
{
	makestr(label, nlabel);
	if (image) image->dec_count();
	if (bwimage) bwimage->dec_count();
	image = bwimage = nullptr;

	bwimage = bw;
	image   = img;

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
 */
/*! \var int IconSelector::padg
 * \brief Pad to put around the label, whether or not an icon is present. (the icon can be padded with boxinset)
 */


#define DTYPE_NORMAL 0
#define DTYPE_LIST   1

IconSelector::IconSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int npad,
						int npadg, //!< npadg is the padding to put around a label. Icons are not padded.
						int nboxinset
						) //npadg=5
		: BoxSelector(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder,prev,nowner,nsendmes,npad)
{
	// npad is the inset, padg is space between icon and text
	padg = npadg;
	boxinset = nboxinset;
	display_type = DTYPE_NORMAL;

	if (win_style&STRICON_STR_ICON) labelstyle=LAX_TEXT_ICON;
	else labelstyle=LAX_ICON_TEXT;
	tlabelstyle = labelstyle;
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
	int iw = 0, ih = 0;
	if (img) {
		if (labelstyle != LAX_TEXT_ONLY) {
			iw = img->w() + boxinset;
			ih = img->h() + boxinset;
		}
		// if (makebw) ; //*** make the black and white version
	}

	double tw=0,th=0;
	if (nlabel) tw = win_themestyle->normal->Extent(nlabel,-1,&tw,&th,NULL,NULL) + 2*padg;
	if (labelstyle == LAX_ICON_ONLY && img) tw = 0;

	tw *= UIScale();
	th *= UIScale();

	b->w (tw + iw);
	b->pw(tw + iw);
	b->h (th > ih ? th : ih);
	b->ph(th > ih ? th : ih);
	b->id = nid;

	b->SetBox(nlabel, img, nullptr);

	b->pad = bevel;
}

//! Just returns AddBox(NULL,load_image(filename),makebw).
int IconSelector::AddBox(const char *nlabel,const char *filename,int nid)
{
	return AddBox(NULL, ImageLoader::LoadImage(filename),nid);
}

//! Add box and return its index. img is taken, not inc_counted.
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
	if (which < 0 || which >= wholelist.n || !win_on) return;
	IconBox *b = dynamic_cast<IconBox *>(wholelist.e[which]);
	if (!b) return;
	
	Displayer *dp = GetDisplayer();

	if (display_style == BOXES_Highlighted)
		 dp->NewFG(which==hoverbox ? win_themestyle->bghover : (b->state == LAX_ON ? win_themestyle->bghl : win_themestyle->bg));
	else dp->NewFG(which==hoverbox ? win_themestyle->bghover : win_themestyle->bg);

	dp->drawrectangle(b->x() - b->pad,  b->y() - b->pad,    b->w() + 2*b->pad,  b->h() + 2*b->pad, 1);

	 // Set  tx,ty  px,py
	int w,h,tx,ty,ix,iy,dx,dy;
	LaxImage *i = b->image;
	const char *l = b->label;
	get_placement(i,win_themestyle->normal,l,padg,labelstyle,&w,&h,&tx,&ty,&ix,&iy, UIScale());
	dx = b->x() + (b->w() - w)/2;
	dy = b->y() + (b->h() - h)/2;

	 // draw the stuff
	if (i && ix!=LAX_WAY_OFF) {
		ix += dx;
		iy += dy;
		dp->imageout(i, ix,iy+b->h()-boxinset, i->w(),-i->h());
		i->doneForNow();
	}
	if (l && tx>LAX_WAY_OFF) {
		tx += dx;
		ty += dy;
		dp->NewFG(win_themestyle->fg);
		dp->textout(tx,ty, l,-1, LAX_LEFT|LAX_TOP);
	}
	
	if (display_style == BOXES_Flat) {
		if (b->state == LAX_ON)
			dp->drawBevel(b->pad,highlight,shadow,b->state, b->x()-b->pad,b->y()-b->pad, b->w()+2*b->pad,b->h()+2*b->pad);
	} else if (display_style == BOXES_Beveled) {
		dp->drawBevel(b->pad,highlight,shadow,b->state, b->x()-b->pad,b->y()-b->pad, b->w()+2*b->pad,b->h()+2*b->pad);
	}
}

int IconSelector::send()
{
	DBG cerr << "IconSelector::send(), curbox: "<<curbox<<" "<<(curbox >=0 && (wholelist.e[curbox]) ? ((IconBox *)(wholelist.e[curbox]))->label : "null")<<endl;
	if (display_type == DTYPE_NORMAL) return BoxSelector::send();

	SimpleMessage *mevent=new SimpleMessage;

	int n=0;
	for (int c=0; c<wholelist.n; c++) {
		if (wholelist.e[c] && ((SelBox *)(wholelist.e[c]))->state&LAX_ON) n++;
	}
	mevent->info1 = curbox/2; // current box*** if LAX_ON only?? or first on box?
	mevent->info2 = (curbox >= 0?((SelBox *)wholelist.e[curbox])->id:0); // id of curbox
	mevent->info3 = n; // num that are on
	mevent->info4 = 0; // ***num that are newly changed

	app->SendMessage(mevent,win_owner,win_sendthis,object_id);
	return 1;
}

/*! If yes, inserts null boxes to force line breaks, and uses text only display style.
 */
int IconSelector::DisplayAsList(bool yes)
{
	if ((yes && display_type == DTYPE_LIST) || (!yes && display_type == DTYPE_NORMAL)) return display_type;

	if (yes) { //was not-list, insert nulls and sync
		display_type = DTYPE_LIST;
		tlabelstyle = labelstyle;
		labelstyle = LAX_TEXT_ONLY;
		for (int c = wholelist.n; c > 0; c--) {
			Push(NULL,0,c); //AddNull(c);
			IconBox *b = dynamic_cast<IconBox*>(wholelist.e[c-1]);
			if (b->image) b->image->inc_count();
			FillBox(b, b->label, b->image, b->id);
		}

	} else { //was list, remove nulls and sync
		display_type = DTYPE_NORMAL;
		labelstyle = tlabelstyle;
		for (int c = wholelist.n-1; c >= 0; c-=2) {
			Pop(c);
			IconBox *b = dynamic_cast<IconBox*>(wholelist.e[c-1]);
			if (b->image) b->image->inc_count();
			FillBox(b, b->label, b->image, b->id);
		}
	}
	sync();
	return display_type;
}

} // namespace Laxkit


