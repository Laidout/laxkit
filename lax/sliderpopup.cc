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
//    Copyright (C) 2004-2006,2010 by Tom Lechner
//



#include <lax/sliderpopup.h>
#include <lax/laxutils.h>
#include <lax/popupmenu.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {


//------------------------------ SliderPopup ------------------------------
/*! \class SliderPopup
 * \ingroup menuthings
 * \brief Basically an extended IconSlider to include a popup menu.
 *
 * \todo ******** needs work! update these docs 
 *
 * The user has the option of poping up the menu by clicking on the little arrow,
 * or clicking, like any other item slider, on the left to decrease one element,
 * or on the right to increase the element.
 * 
 * Uses a MenuInfo class to store the items, which may or may not be local to
 * this window. This enables easy reuse of menu lists. Only the top level of the
 * MenuInfo is used, not submenus.
 *
 * \todo Make optional tab-completing input similar to NumInputSlider, perhaps in a subclass.
 */

bool SliderPopup::default_icon_from_font_size = true;
double SliderPopup::default_icon_height = 1.2;

SliderPopup::SliderPopup(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *mes,
		MenuInfo *nitems, int absorb)
	: ItemSlider(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,mes)
{
	icon_from_font_size = default_icon_from_font_size;
	icon_height = default_icon_height;

	curitem=-1;

	if (nitems) {
		items = nitems;
		if (!absorb) items->inc_count();
	} else {
		items = new MenuInfo;
	}

	pad = gap = win_themestyle->normal->textheight()/3;
	if (win_h == 0) {
		win_h = 2*pad + win_themestyle->normal->textheight();
	}
	arrowwidth = win_themestyle->normal->textheight()*2/3;
}

//! Delete items if it is local.
SliderPopup::~SliderPopup()
{
	if (items) items->dec_count();
}

int SliderPopup::ThemeChange(Theme *theme)
{
	anXWindow::ThemeChange(theme);

	pad = gap = win_themestyle->normal->textheight()/3;
	if (win_h == 0) {
		win_h = 2*pad + win_themestyle->normal->textheight();
	}
	arrowwidth = win_themestyle->normal->textheight()*2/3;
	return 0;
}

//! Set the dimensions to the maximum bounds of the entries.
/*! \todo *** this only works before mapping because it sets win_w,win_h
 * directly, rather than through Resize().. should probably think if
 * this is good or not.
 */
void SliderPopup::WrapToExtent()
{
	char *label;
	MenuItem *item;
	LaxImage *img;
	
	int w,h,maxh=0,maxw=0;
	for (int c=0; c<items->menuitems.n; c++) {
		if (items->menuitems.e[c]->state & LAX_SEPARATOR) continue;
		if (items->menuitems.e[c]->state & SLIDER_IGNORE_ON_BROWSE) continue;

		label=items->menuitems.e[c]->name;
		item=items->menuitems.e[c];
		if (item) img=item->image; else img=NULL;
		
		get_placement(img,win_themestyle->normal,label,gap,(win_style&SLIDER_WHAT_MASK)>>21,
					  &w,&h,NULL,NULL,NULL,NULL, icon_from_font_size ? icon_height : 0);
		if (w>maxw) maxw=w;
		if (h>maxh) maxh=h;
	}
	maxw+=arrowwidth+2*pad;
	maxh+=2*pad;
	win_w=maxw;
	win_h=maxh;
}

void SliderPopup::GetPlacement(double *w,double *h,double *tx,double *ty,double *ix,double *iy, double *iw,double *ih)
{
	// char *label = curitem >= 0 ? items->menuitems.e[curitem]->name : nullptr;
	// MenuItem *item = curitem >= 0 ? items->menuitems.e[curitem] : nullptr;
	// LaxImage *image = item ? item->image : nullptr;

	// double ww=0, hh=0;

	// if (image) {
	// 	if (icon_size_type == Relative_To_Font) {
	// 		hh = (font ? font : win_themestyle->normal)->textheight() * icon_height;
	// 	} else {
	// 		hh = image->h();
	// 	}
	// 	ww = hh / image->h() * image->w();
	// } else {
	// 	ww = thingw;
	// 	hh = thingh;
	// }

	// if (iw) *iw = ww;
	// if (ih) *ih = hh;

	// get_placement(ww,hh, (font ? font : win_themestyle->normal), label,gap,labelstyle, w,h, tx,ty, ix,iy);
}


//! Draw the little arrow to indicate that there's something to be popped up.
void SliderPopup::drawarrow()
{
	Displayer *dp = GetDisplayer();
	dp->drawthing(
			  win_w-arrowwidth/2,win_h/2, arrowwidth/2,arrowwidth/2,
			  THING_Triangle_Down, win_themestyle->fg.Pixel(), win_themestyle->color1.Pixel());
}

//! Draw the item text, and the little popup arrow.
void SliderPopup::Refresh()
{
	if (!win_on || !needtodraw) return;

	Displayer *dp=MakeCurrent();
	double th = win_themestyle->normal->textheight();
	dp->font(win_themestyle->normal, th);

	dp->NewBG(win_themestyle->bg);
	if (hover) {
		dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.07));
		dp->drawrectangle(0,0, win_w,win_h, 1);
	} else dp->ClearWindow();
	dp->NewFG(win_themestyle->fg);

	if (curitem >= 0) {
		char *label=items->menuitems.e[curitem]->name;
		MenuItem *item=items->menuitems.e[curitem];
		LaxImage *img=NULL;
		if (item) img=item->image;
			
		 // draw item
		int tx,ty,ix,iy,w,h;
		get_placement(img,win_themestyle->normal,label,gap,(win_style&SLIDER_WHAT_MASK)>>21,
					  &w,&h,&tx,&ty,&ix,&iy, icon_from_font_size ? icon_height : 0);
		if (tx != LAX_WAY_OFF) dp->textout((win_w-arrowwidth-w)/2+tx,(win_h-h)/2+ty, label,-1, LAX_LEFT|LAX_TOP);
		if (ix != LAX_WAY_OFF) {
			if (icon_from_font_size) {
				dp->imageout(img, (win_w-arrowwidth-w)/2+ix,(win_h-h)/2+iy, img->w() / img->h() * th * icon_height, th * icon_height);
			} else {
				dp->imageout(img, (win_w-arrowwidth-w)/2+ix,(win_h-h)/2+iy);
			}
		}

		if (!(win_style & SLIDER_POP_ONLY)) {
			if (hover==LAX_LEFT) {
				dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.3)); 
				dp->drawthing(arrowwidth/2,win_h/2,arrowwidth/2,arrowwidth/2, 1, THING_Triangle_Left);
			} else if (hover==LAX_RIGHT) {
				dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.3)); 
				dp->drawthing(win_w-arrowwidth-arrowwidth/2,win_h/2,arrowwidth/2,arrowwidth/2, 1, THING_Triangle_Right);
			}
		}
	}

	 // draw popup arrow
	drawarrow();
	
	needtodraw=0;
}

//! Delete item with this id, and return the number of items left.
int SliderPopup::DeleteItem(int id)
{
	if (nitems==0) return 0;
	int c2=items->findIndex(id);
	if (c2>=0) items->menuitems.pop(c2);
	nitems=items->menuitems.n;
	needtodraw=1;
	return nitems;
}

//! Adds a bunch of text only items.
/*! Returns number of items added.
 */
int SliderPopup::AddItems(const char **i,int n,int startid)
{
	int oldn = items->menuitems.n;
	for (int c=0; c<n; c++) {
		items->AddItem(i[c], startid+c);
	}
	nitems = items->menuitems.n;
	return nitems - oldn;
}

//! Currently adds a grayed separator with name="", id=0, no submenu, (but it is still legal to add a submenu!!)
/*! Returns whatever AddItem returns.
 *
 * These separators will not show up when just clicking through items.
 */
int SliderPopup::AddSep(const char *name,int where)
{
	return items->AddSep(name,where);
}

//! Just return AddItem(newitem,NULL,nid).
/*! Use SLIDER_IGNORE_ON_BROWSE in extrastate as a menuitem state, to show in menu, but not prev/next.
 */
int SliderPopup::AddItem(const char *newitem,int nid)
{
	return AddItem(newitem,NULL,nid);
}

int SliderPopup::AddToggleItem(const char *newitem, int nid, int ninfo, bool on)
{
	items->AddToggleItem(newitem,nid,ninfo,on);
	needtodraw=1;
	nitems = items->menuitems.n;
	if (nitems && curitem<0) curitem = 0;
	return nitems;
}

//! Return the number of items. icon's count is not incremented.
/*! Use SLIDER_IGNORE_ON_BROWSE in extrastate as a menuitem state, to show in menu, but not prev/next.
 */
int SliderPopup::AddItem(const char *newitem,LaxImage *icon,int nid)
{
	MenuItem *item=new MenuItem(newitem, icon, nid, LAX_OFF, 0, NULL, 1);
	items->AddItem(item,1);
	needtodraw=1;
	nitems=items->menuitems.n;
	if (nitems && curitem<0) curitem=0;
	return nitems;
}

//! Add extra state flags to an item, or most recently added if which==-1.
/*! Return 0 for success or nonzero for error.
 *
 * If on==-1, then toggle. If on==0, then clear. If on==1, then set.
 *
 * \todo this is currently a little bit of a kludge to add SLIDER_IGNORE_ON_BROWSE.
 *   a more clean way would be to have it in AddItem, but due to the similarity mix of two AddItem()
 *   functions, lots of cleanup would have to happen to add an extra int for extrastate there.
 */
int SliderPopup::SetState(int which, int extrastate, int on)
{
	if (which==-1) which=items->menuitems.n-1;
	if (which<0 || which>=items->menuitems.n) return 1;

	if (on==-1) {
		if (((int)items->menuitems.e[which]->state & extrastate)==extrastate) on=0;
		else on=1;
	}
	if (on==0) items->menuitems.e[which]->state&=~extrastate;
	else items->menuitems.e[which]->state|=extrastate;

	return 0;
}

//! Return index in items of item with fromid, or -1 if not found.
int SliderPopup::GetItemIndex(int fromid)
{
	for (int i=0; i<items->menuitems.n; i++)
		if (items->menuitems.e[i]->id==fromid) return i;
	return -1;
}

/*! Returns items->menuitems.e[which]->state&extrastate.
 */
int SliderPopup::GetState(int which, int extrastate)
{
	if (which==-1) which=items->menuitems.n-1;
	if (which<0 || which>=items->menuitems.n) return 0;
	return items->menuitems.e[which]->state&extrastate;
}

//! Remove all items from sliderpopup.
/*! If completely!=0, then totally remove old items object, and establish a new one.
 */
int SliderPopup::Flush(int completely)
{
	if (completely) {
		if (items) items->dec_count();
		items = new MenuInfo;
	} else items->Flush();
	return 0;
}

//! Returns const pointer to the item name.
/*! Better use it quick, before items change!
 */
const char *SliderPopup::GetCurrentItem()
{
	if (curitem>=0) return (const char *)(items->menuitems.e[curitem]->name);
	return NULL;
}

int SliderPopup::GetCurrentItemIndex()
{ return curitem; }

//! Return the id corresponding to item with index i.
int SliderPopup::getid(int i)
{
	if (i<0 || i>=items->menuitems.n) return -1;
	return items->menuitems.e[i]->id;
}

//! Sends message to owner.
/*! Sends a SimpleMessage with <tt>message->info1=id of curitem</tt>.
 * Also fill in message->str with the current item's name if win_style&SLIDER_SEND_STRING).
 */
int SliderPopup::send()
{
	if (!win_owner || !win_sendthis || curitem<0) return 0;

	SimpleMessage *ievent=new SimpleMessage;
	if (win_style&SLIDER_SEND_STRING) makestr(ievent->str,items->menuitems.e[curitem]->name);
	ievent->info1=items->menuitems.e[curitem]->id;
	ievent->info2=items->menuitems.e[curitem]->id;
	app->SendMessage(ievent,win_owner,win_sendthis,object_id);
	needtodraw=1;
	return 1;
}

int SliderPopup::Event(const EventData *e,const char *mes)
{
	if (strcmp(mes,"popupselect")) return ItemSlider::Event(e,mes);

	DBG cerr <<"SliderPopup message received."<<endl;
	
	 // So now the button was released, and we must determine the
	 // ON elements in menuinfo
	int ncuritem = -1;
	const SimpleMessage *m=dynamic_cast<const SimpleMessage*>(e);
	
	DBG cerr <<"----SliderPopup got popup event curitem:"<< m->info1<<", id:"<<m->info2<<endl;
	//ncuritem= m->info1; // this is the curitem just before popup destroyed itself

	for (int c=0; c<items->menuitems.n; c++) {
		if (m->info2 == items->menuitems.e[c]->id) {
			ncuritem = c;
			break;
		}
	}

	if (ncuritem>=0 && ncuritem<items->menuitems.n && !(items->menuitems.e[ncuritem]->state&(LAX_GRAY|LAX_SEPARATOR))) {
		if (curitem>=0 && items->menuitems.e[curitem]->state&LAX_ON) {
			items->menuitems.e[curitem]->state=
				(items->menuitems.e[curitem]->state&~LAX_ON)|LAX_OFF;
		}
		if (items->menuitems.e[ncuritem]->state&LAX_OFF) {
			items->menuitems.e[ncuritem]->state=
				(items->menuitems.e[ncuritem]->state&~LAX_OFF)|LAX_ON;
		}
		int oldcuritem = curitem;
		curitem=ncuritem;
		send();
		if (items->menuitems.e[ncuritem]->state & SLIDER_IGNORE_ON_BROWSE) curitem = oldcuritem;
		needtodraw=1;
	}

	return 0;
}

//! Create the popup menu if there are more than 0 items. Called from LBDown().
void SliderPopup::makePopup(int mouseid)
{
	if (items->n() == 0) return;

	PopupMenu *popup;
	int justify=0;
	if (win_style&SLIDER_LEFT)   justify|=TREESEL_LEFT;
	//if (win_style&SLIDER_CENTER) justify|=TREESEL_CENTER; //center is default
	if (win_style&SLIDER_RIGHT)  justify|=TREESEL_RIGHT;

	items->ClearSearch();
	popup=new PopupMenu(items->title?items->title:"Item Popup",
						items->title?items->title:"Item Popup",
						0,
						0,0,0,0, 1, 
						object_id,"popupselect", 
						mouseid,
						items,0,
						NULL,
						justify);

	popup->pad=pad;
	popup->Select(curitem);
//	popup->SetFirst(curitem,x,y); 
	popup->WrapToMouse(mouseid);
	app->rundialog(popup);
	app->setfocus(popup,0,NULL);//***
}

#define SLIDER_POP_MENU 1000

int SliderPopup::scan(int x,int y,unsigned int state)
{
	int ww=win_w/2;
	if (win_style&EDITABLE) ww = win_themestyle->normal->textheight();

	if (x<ww) return LAX_LEFT;
	else if (x>win_w-arrowwidth) return SLIDER_POP_MENU;
	else if (x>win_w-ww) return LAX_RIGHT;
	else if (x>0 && x<win_w) return LAX_CENTER;

	return 0;
}

//! Pop up a menu with the items in it via popup() when click on arrow.
/*! \todo *** work out how transfer focus
 */
int SliderPopup::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!(win_style & SLIDER_POP_ONLY) && (x<win_w-arrowwidth || !items)) return ItemSlider::LBDown(x,y,state,count,d);
	
	makePopup(d->id);
	return 0;
}

//! copy curselection to clipboard
int SliderPopup::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	// copy current item clipboard
	if (curitem>=0 && curitem<items->menuitems.n) 
		app->CopytoBuffer(items->menuitems.e[curitem]->name,
						  strlen(items->menuitems.e[curitem]->name)); 
	DBG else cerr << "SliderPopup copy to clip: No item selected.\n";

	return 1;
}

/*! Left selects previous str, right selects next.
 * Enter pops up menu.
 */
int SliderPopup::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	switch(ch) {
		case LAX_Tab: return anXWindow::CharInput(ch,buffer,len,state,d);
		case LAX_Enter: {
				makePopup(d->paired_mouse?d->paired_mouse->id:0);
				return 0;
			}
		case LAX_Left: SelectPrevious(1); send(); return 0; 
		case LAX_Right: SelectNext(1); send(); return 0;
//		case LAX_Up: //*** up: bring up popup
//		case LAX_Down: //*** down
	}
	return anXWindow::CharInput(ch,buffer,len,state,d);
}

//! Select the previous item.
/*! Returns id of the new item.
 *
 * This skips over any separators.
 */
int SliderPopup::SelectPrevious(double multiplier)
{ 
	if (curitem==-1) return -1;
	int olditem=curitem;
	do {
		curitem--;
		if (curitem<0) curitem=numitems()-1;
		if (!(items->menuitems.e[curitem]->state&(LAX_SEPARATOR|SLIDER_IGNORE_ON_BROWSE))) break;
	} while (curitem!=olditem);

	if (win_style & SENDALL) send();
	DBG cerr <<" Previous Item:"<<curitem<<endl;
	needtodraw=1;
	return getid(curitem);
}

//! Select the next item.
/*! Returns id of the new item.
 *
 * This skips over any separators.
 */
int SliderPopup::SelectNext(double multiplier)
{
	if (curitem==-1) return -1;

	int olditem=curitem;
	do {
		curitem++;
		if (curitem==numitems()) curitem=0;
		if (!(items->menuitems.e[curitem]->state&(LAX_SEPARATOR|SLIDER_IGNORE_ON_BROWSE))) break;
	} while (curitem!=olditem);

	if (win_style & SENDALL) send();
	needtodraw=1;
	return getid(curitem);
}


} // namespace Laxkit


