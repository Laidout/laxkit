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
//    Copyright (C) 2004-2010 by Tom Lechner
//


#include <lax/lists.cc>
#include <lax/iconslider.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

//------------------------------------ IconSlider ----------------------------------
/*! \class IconSlider
 * \brief Flip through a number of icons, 1 shown at a time.
 *
 * Clicking on left decrements, right increments. Click and drag walks through the icons.
 *
 * \todo on to striconsliderpopup Gack!!
 * 
 * Which of icons or text is displayed depends on these window styles:
 * \code
 *  #define ICONSLIDER_ICON_ONLY    (0<<20)
 *  #define ICONSLIDER_TEXT_ONLY    (1<<20)
 *  #define ICONSLIDER_TEXT_ICON    (2<<20)
 *  #define ICONSLIDER_ICON_TEXT    (3<<20)
 *  #define ICONSLIDER_WHAT_MASK    (3<<20)
 * \endcode
 */


IconSlider::IconSlider(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
	int xx,int yy,int ww,int hh,int brder,
	anXWindow *prev,unsigned long nowner,const char *nsendthis,
	MenuInfo *nitems,int ilocal)
	: ItemSlider(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsendthis)
{
	lastitem=curitem;
	InstallColors(THEME_Panel);
	gap=3;

	items=nitems;
	iislocal=ilocal;
}

IconSlider::~IconSlider()
{
	if (iislocal) delete items;
}

void IconSlider::Refresh()
{
	if (!win_on || !needtodraw || curitem<0) return;

	Displayer *dp=GetDisplayer();
	dp->NewFG(win_themestyle->bg);
	dp->drawrectangle(0,0,win_w,win_h, 1);

	char *label=items->menuitems.e[curitem]->name;
	MenuItem *item=items->menuitems.e[curitem];
	LaxImage *img=NULL;
	if (item) img=item->image;
		
	int tx,ty,ix,iy;
	get_placement(img,label,gap,(win_style&ICONSLIDER_WHAT_MASK)>>20,
		NULL,NULL,&tx,&ty,&ix,&iy);
	if (tx!=LAX_WAY_OFF) textout(this, label,-1,tx,ty,LAX_LEFT|LAX_TOP);
	if (ix!=LAX_WAY_OFF) image_out(img,this,ix,iy);
	
	needtodraw=0;
}

/*! \todo *** imp me!
 */
int IconSlider::DeleteItem(int id)
{//***
	return 1;
}
	
int IconSlider::AddItem(const char *nlabel,const char *filename,int nid)
{
	if (!filename && !nlabel) return 1;

	if (!items) {
		items=new MenuInfo;
		iislocal=1;
	}
	MenuItem *newitem=new MenuItem(nlabel,filename,nid,LAX_OFF,0,NULL,0);
	items->AddItem(newitem,1);
	
	DBG cerr <<"Added "<<filename<<" to IconSlider "<<WindowTitle()<<endl;
	needtodraw=1;
	return items->menuitems.n-1;
}


} // namespace Laxkit

