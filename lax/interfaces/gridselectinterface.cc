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
//    Copyright (C) 2025 by Tom Lechner
//



#include <lax/interfaces/gridselectinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/laxdefs.h>
#include <lax/language.h>

#include <lax/debug.h>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//--------------------------- GridSelectData -------------------------------------

// /*! \class GridSelectData
//  * \ingroup interfaces
//  * \brief Data that GridSelectInterface can use.
//  */

// GridSelectData::GridSelectData()
// {
// }

// GridSelectData::~GridSelectData()
// {
// }



//--------------------------- GridSelectInterface -------------------------------------

/*! \class GridSelectInterface
 * \ingroup interfaces
 * \brief Exciting interface that does stuff.
 */


GridSelectInterface::GridSelectInterface(anInterface *nowner, int nid, Displayer *ndp, unsigned int send_to_id, const char *msg)
 : anInterface(nowner,nid,ndp)
{
	interface_flags = 0;

	showdecs   = 1;
	needtodraw = 1;

	rows = columns = -1;

	color_normal.rgbf(0.,0.,0.);
	color_selected.rgbf(.1,0.,.1);
	hover_diff = .1;

	owner_id = send_to_id;
	makestr(owner_message, msg);

	sc = nullptr; //shortcut list, define as needed in GetShortcuts()
}

GridSelectInterface::~GridSelectInterface()
{
	if (items) items->dec_count();
	if (sc) sc->dec_count();
}

const char *GridSelectInterface::whatdatatype()
{
	return nullptr; // nullptr means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *GridSelectInterface::Name()
{ return _("Grid Select"); }


//! Return new GridSelectInterface.
/*! If dup!=nullptr and it cannot be cast to GridSelectInterface, then return nullptr.
 */
anInterface *GridSelectInterface::duplicateInterface(anInterface *dup)
{
	if (dup == nullptr) dup = new GridSelectInterface(nullptr,id,nullptr);
	else if (!dynamic_cast<GridSelectInterface *>(dup)) return nullptr;
	return anInterface::duplicateInterface(dup);
}

//! Use the object at oc if it is an GridSelectData.
int GridSelectInterface::UseThisObject(ObjectContext *oc)
{
	return 0;
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int GridSelectInterface::UseThis(anObject *nobj, unsigned int mask)
{
	// if (!nobj) return 1;
	// LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
	// if (ls!=nullptr) {
	// 	if (mask & (LINESTYLE_Color | LINESTYLE_Color2)) { 
	// 		linecolor=ls->color;
	// 	}
	// 	needtodraw=1;
	// 	return 1;
	// }
	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *GridSelectInterface::Context()
{
	return nullptr;
	// return dataoc;
}

/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int GridSelectInterface::InterfaceOn()
{
	showdecs = 1;
	needtodraw = 1;
	return 0;
}

/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int GridSelectInterface::InterfaceOff()
{
	Clear(nullptr);
	showdecs = 0;
	needtodraw = 1;
	return 0;
}

/*! Clear references to d within the interface.
 */
void GridSelectInterface::Clear(SomeData *d)
{
	selected.flush();
	almost_selected.flush();

	// if (dataoc) { delete dataoc; dataoc=nullptr; }
	// if (data) { data->dec_count(); data=nullptr; }
}

void GridSelectInterface::ViewportResized()
{
	DBGL("========================================= GridSelectInterface::ViewportResized() ");
	// if necessary, do stuff in response to the parent window size changed
	bounds.minx = 0;
	bounds.miny = 0;
	bounds.maxx = curwindow->win_w;
	bounds.maxy = curwindow->win_h;
	need_to_remap = true;
	// RemapItems();
	if (!lerp_timer) lerp_timer = app->addtimer(this, 15, 15, -1);
}

/*! Return a context specific menu, typically in response to a right click.
 */
Laxkit::MenuInfo *GridSelectInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	// if (no menu for x,y) return menu;

	// if (!menu) menu=new MenuInfo;
	// if (!menu->n()) menu->AddSep(_("Some new menu header"));

	// menu->AddToggleItem(_("New checkbox"), laximage_icon, YOUR_CHECKBOX_ID, checkbox_info, (istyle & STYLEFLAG) /*on*/, -1 /*where*/);
	// menu->AddItem(_("Some menu item"), YOUR_MENU_VALUE);
	// menu->AddSep(_("Some separator text"));
	// menu->AddItem(_("Et Cetera"), YOUR_OTHER_VALUE);
	// menp->AddItem(_("Item with info"), YOUR_ITEM_ID, LAX_OFF, items_info);

	//  //include <lax/iconmanager.h> if you want access to default icons
	// LaxImage icon = iconmanager->GetIcon("NewDirectory");
	// menp->AddItem(_("Item with icon"), icon, SOME_ITEM_ID, LAX_OFF, items_info);

	return menu;
}

int GridSelectInterface::Idle(int tid, double delta)
{
	if (tid != lerp_timer) return 1;
	return AdjustOffset();
}

int GridSelectInterface::AdjustOffset()
{
	if (!items) {
		offset.set(0,0);
		return 1;
	}
	if (need_to_remap) RemapItems();

	double scroll_speed = .6;

	if (fabs(offset_target.x - offset.x) > 1) {
		double diff = (offset.x - offset_target.x) * scroll_speed;
		offset.x -= diff;
		needtodraw = 1;
		curwindow->Needtodraw(1);
	} else offset.x = offset_target.x;

	double diff = (offset_target.y - offset.y);
	if (fabs(diff) < 1) {
		offset.y = offset_target.y;

	} else {
		diff *= scroll_speed;

		offset.y += diff;
		if (offset.y + items_bounds.miny <= bounds.miny && offset.y + items_bounds.maxy >= bounds.maxy) {
			// ok, all outside bounds
		
		} else if (items_bounds.maxy - items_bounds.miny <= bounds.maxy - bounds.miny) {
			// small items area, snap to top, min to min
			DBGL("------------------------------------ small, snap to top");
			offset_target.y = bounds.miny - items_bounds.miny;
		
		} else if (offset.y + items_bounds.maxy < bounds.miny) {
			// whole thing somehow is offscreen below min, snap max to max
			DBGL("------------------------------------ whole below min")
			offset_target.y = bounds.maxy - (items_bounds.maxy - items_bounds.miny);
		
		} else if (offset.y + items_bounds.miny > bounds.maxy) {
			// whole thing somehow is offscreen beyond max, snap min to min
			DBGL("------------------------------------ whole above max")
			offset_target.y = bounds.miny - items_bounds.miny;
		
		} else if (offset.y + items_bounds.miny > bounds.miny && offset.y + items_bounds.maxy > bounds.maxy) {
			// hanging beyond min, snap to top
			DBGL("------------------------------------ hang beyond min")
			offset_target.y = bounds.miny - items_bounds.miny;
		
		} else if (offset.y + items_bounds.miny < bounds.miny && offset.y + items_bounds.maxy < bounds.maxy) {
			// gap at bottom
			DBGL("------------------------------------ hang below max")
			offset_target.y = bounds.maxy - (items_bounds.maxy - items_bounds.miny);

		} else {
			DBGL("------------------------------------ other")
		}
		DBGL("        bounds xxyy: "<<bounds.minx<<" "<<bounds.maxx<<" "<<bounds.miny<<" "<<bounds.maxy)
		DBGL("  items_bounds xxyy: "<<items_bounds.minx<<" "<<items_bounds.maxx<<" "<<items_bounds.miny<<" "<<items_bounds.maxy)
		DBGL("        offset_target.y:"<<offset_target.y)
	
		needtodraw = 1;
		curwindow->Needtodraw(1);
	}

	if (offset.y != offset_target.y || offset.x != offset_target.x) return 0;
	lerp_timer = 0;
	return 1; // means remove timer
}


/*! Intercept events if necessary, such as from the ContextMenu().
 */
int GridSelectInterface::Event(const Laxkit::EventData *evdata, const char *mes)
{
	// if (!strcmp(mes,"menuevent")) {
	// 	 //these are sent by the ContextMenu popup
	// 	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(evdata);
	// 	int i	= s->info2; //id of menu item
	// 	int info = s->info4; //info of menu item

	// 	if ( i==SOME_MENU_VALUE) {
	// 		...
	// 	}

	// 	return 0; 
	// }

	return 1; //event not absorbed
}


/*! Draw some data other than the current data.
 * This is called during screen refreshes.
 */
int GridSelectInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	// if (!ndata || dynamic_cast<GridSelectData *>(ndata)==nullptr) return 1;

	// GridSelectData *bzd=data;
	// data=dynamic_cast<GridSelectData *>(ndata);

	//  // store any other state we need to remember
	//  // and update to draw just the temporary object, no decorations
	// int td=showdecs,ntd=needtodraw;
	// showdecs=0;
	// needtodraw=1;

	// Refresh();

	//  //now restore the old state
	// needtodraw=ntd;
	// showdecs=td;
	// data=bzd;
	return 1;
}


void GridSelectInterface::RemapItems()
{
	if (!items || !curwindow) return;

	DBGL("GridSelectInterface::RemapItems()")

	if (!bounds.validbounds()) {
		// bounds.minx = dp->Minx;
		// bounds.miny = dp->Miny;
		// bounds.maxx = dp->Maxx;
		// bounds.maxy = dp->Maxy;
		bounds.minx = 0;//curwindow->win_;
		bounds.miny = 0;//curwindow->win_;
		bounds.maxx = curwindow->win_w;
		bounds.maxy = curwindow->win_h;
	}

	double textheight = UIScale() * curwindow->win_themestyle->normal->textheight();
	if (cell_w <= 0) {
		cell_w = 6 * textheight;
	}
	if (cell_h <= 0) {
		cell_h = 6 * textheight;
	}

	if (gap < 0) { gap = textheight/2; }
	// double padx = 0;

	double innergap = gap;
	if (grid_type == GRIDSELECT_One_Row) {
		rows = 1;
		columns = items->how_many(-1);
	} else if (grid_type == GRIDSELECT_One_Column) {
		columns = 1;
		rows = items->how_many(-1);
	} else {
		columns = bounds.boxwidth() / cell_w;
		if (columns < 1) columns = 1;
		rows = items->how_many(-1) / columns;
		if (rows < 1) rows = 1;
		// padx = (bounds.boxwidth() - columns * cell_w)/2;
		
		if (columns == 1) innergap = 0;
		else innergap = (bounds.boxwidth() - columns * cell_w) / (columns-1);
		if (innergap < 0) innergap = 0;
	}
	DBGL("rows: "<<rows<<" cols: "<<columns<<"  cellw: "<<cell_w<<" cellh:"<<cell_h);
	DBGL("bounds: "<<bounds.minx<<" "<<bounds.maxx<<" "<<bounds.miny<<" "<<bounds.maxy)
	DBGL("  old items_bounds xxyy: "<<items_bounds.minx<<" "<<items_bounds.maxx<<" "<<items_bounds.miny<<" "<<items_bounds.maxy)

	items_bounds.ClearBBox();
	// DBGL("  items_bounds xxyy: "<<items_bounds.minx<<" "<<items_bounds.maxx<<" "<<items_bounds.miny<<" "<<items_bounds.maxy)
	double x = 0; //padx + bounds.minx + offset.x;
	double y = 0; //bounds.miny + offset.y;
	int i = 0, j = 0;
	for (int c = 0; c < items->how_many(0); c++) {
		MenuItem *item = items->findFromLine(c);
		item->x = x;
		item->y = y;
		item->w = cell_w;
		item->h = cell_h;
		items_bounds.addtobounds_wh(x,y,cell_w,cell_h);

		// DBGM(c<<": "<<item->x<<' '<<item->y<<' '<<item->w<<' '<<item->h);

		if (grid_type == GRIDSELECT_One_Row) {
			i++;
			x += cell_w;

		} else if (grid_type == GRIDSELECT_One_Column) {
			j++;
			y += cell_h;

		} else { // grid
			i++;
			if (i == columns) {
				i = 0;
				x = 0; //padx + bounds.minx + offset.x;
				y += cell_h;
			} else {
				x += cell_w + innergap;
			}
		}
	}
	if (grid_type == GRIDSELECT_One_Row) {
		offset.y = (bounds.boxheight() - items_bounds.boxheight())/2;
	} else if (grid_type == GRIDSELECT_One_Column) {
		offset.x = (bounds.boxwidth() - items_bounds.boxwidth())/2;
	} else { // full grid
		offset.x = (bounds.boxwidth() - items_bounds.boxwidth())/2;
	}

	DBGL("  new items_bounds xxyy: "<<items_bounds.minx<<" "<<items_bounds.maxx<<" "<<items_bounds.miny<<" "<<items_bounds.maxy)
	need_to_remap = false;
}

int GridSelectInterface::Refresh()
{ 
	if (needtodraw == 0) return 0;
	needtodraw = 0;

	if (items && need_to_remap) {
		RemapItems();
		AdjustOffset();
	}

	 //draw some text name
	dp->DrawScreen();
	dp->LineAttributes(ScreenLine(), 0, LAXCAP_Round, LAXJOIN_Round);
	dp->NewFG(curwindow->win_themestyle->fg);
	double th = dp->textheight();

	
	if (items) {
		for (int c = 0; c < items->how_many(0); c++) {
			MenuItem *item = items->findFromLine(c);

			if (bounds.intersect(offset.x+item->x,offset.x+item->x+item->w, offset.y+item->y,offset.y+item->y+item->h)) {
				if (selected.Contains(c)) {
					if (hover == c) dp->NewFG(color_selected.Hinted(hover_diff));
					else dp->NewFG(color_selected);
					dp->drawrectangle(offset.x+item->x,offset.y+item->y, item->w,item->h, 1);
				} else if (hover == c) {
					ScreenColor hc = color_normal.Hinted(hover_diff);
					dp->NewFG(hc);
					dp->drawrectangle(offset.x+item->x,offset.y+item->y, item->w,item->h, 1);
				} else {
					dp->NewFG(color_normal);
					dp->drawrectangle(offset.x+item->x+gap/2,offset.y+item->y+gap/2, item->w-gap,item->h-gap, 1);
				}
				dp->NewFG(curwindow->win_themestyle->fg);

				if (show_images && item->image) {
					if (show_strings && item->name)
						dp->imageout_within(item->image, offset.x+item->x+gap/2,offset.y+item->y+gap/2, item->w-gap,item->h-gap-th, nullptr, 0 /*flip*/);
					else
						dp->imageout_within(item->image, offset.x+item->x+gap/2,offset.y+item->y+gap/2, item->w-gap,item->h-gap, nullptr, 0 /*flip*/);
				}
				if (show_strings && item->name) {
					if (show_images && item->image)
						dp->textout(offset.x+item->x+item->w/2,offset.y+item->y+item->h-gap/2, item->name ? item->name : "?",-1, LAX_HCENTER | LAX_BOTTOM);
					else
						dp->textout(offset.x+item->x+item->w/2,offset.y+item->y+item->h/2, item->name ? item->name : "?",-1, LAX_CENTER);
				}
			}
		}
	}

	if (mode == GRIDSELECT_Drag_Box) {
		dp->NewFG(curwindow->win_themestyle->fg);
		int ix, iy, cx, cy;
		buttondown.getinitial(0,LEFTBUTTON, &ix,&iy);
		buttondown.getcurrent(0,LEFTBUTTON, &cx,&cy);
		dp->moveto(ix,iy);
		dp->lineto(cx,iy);
		dp->lineto(cx,cy);
		dp->lineto(ix,cy);
		dp->closed();
		dp->stroke(0);
	}

	dp->DrawReal();


	if (showdecs) {
		// draw interface decorations on top of interface data
	}

	return 0;
}


int GridSelectInterface::scan(int x, int y, unsigned int state)
{
	if (!items) {
		DBGW("GridSelectInterface::scan no items!!");
		return -1;
	}
	x -= offset.x;
	y -= offset.y;

	if (grid_type == GRIDSELECT_One_Row) {
		DBGW("IMP ME!")
	} else if (grid_type == GRIDSELECT_One_Column) {
		DBGW("IMP ME!")
	} else {
		for (int c=0; c<items->how_many(0); c++) {
			MenuItem *item = items->findFromLine(c);
			// if (y > item->y + item->h) {
			// 	c += columns-1;
			// 	continue;
			// }
			if (y >= item->y && y <= item->y+item->h && x >= item->x && x <= item->x + item->w) {
				DBGL("scan: "<<c);
				return c;
			}
		}
	}

	// DBGL("scan: "<<-1);
	return -1;
}

int GridSelectInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	int nhover = scan(x,y,state);
	hover = nhover;
	buttondown.down(d->id,LEFTBUTTON,x,y, nhover);
	needtodraw = 1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int GridSelectInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	if (!buttondown.isdown(d->id, LEFTBUTTON)) return 1;

	int nhover = scan(x,y,state);
	int old_hover = -1;
	int dragged = buttondown.up(d->id,LEFTBUTTON, &old_hover);

	DBGL(" ***************** dragged: "<<dragged<<" oldhover: "<<old_hover<<" new_hover: "<<nhover)

	if (dragged < 5 && nhover >= 0 && nhover == old_hover) {

		addselect(nhover, state);

		// if ((state & (ControlMask | ShiftMask)) == ControlMask) {
		// 	selected.pushnodup(nhover);

		// } else if ((state & (ControlMask | ShiftMask)) == ShiftMask) {
		// 	if (selected.n) {
		// 		int start = selected.e[selected.n-1];
		// 		int stop = nhover;
		// 		if (stop < start) { int t = stop; stop = start; start = t; }
		// 		for (int c = start; c <= stop; c++) {
		// 			selected.pushnodup(c);
		// 		}
		// 	}

		// } else {
		// 	selected.flush();
		// 	selected.pushnodup(nhover);
		// }
		send(0);
		needtodraw = 1;
		//items->findFromLine(nhover)->SetState(MENU_SELECTED, true);
	}

	return 0; //return 0 for absorbing event, or 1 for ignoring
}

void GridSelectInterface::addselect(int i,unsigned int state)
{
	if (!items) return;

	if (!(state & ShiftMask) || select_type == LAX_ONE_ONLY || select_type == LAX_ZERO_OR_ONE) {
		//shift not pressed, or select zero or one

		MenuItem *mitem = items->e(i);
		int oldstate = selected.Contains(i);

		if (!(state & ControlMask)) {
			// unselect others
			if (selected.n) oldstate = 0;
			selected.flush();
		}

		if (mitem) {
			if (select_type == LAX_ONE_ONLY || oldstate == 0) {
				//turn on
				if (select_type == LAX_ONE_ONLY) selected.flush();
				selected.pushnodup(i);
				
			} else if (oldstate) {
				//turn off
				selected.remove(selected.findindex(i));
			}
		}

		cur_item = i;
		
		needtodraw |= 2;

	} else if (state & ShiftMask) { // select range
		// MenuItem *curmenuitem = items->e(cur_item);
		int start,end;
		int nstate = selected.Contains(cur_item);
		if (i < cur_item) { start = i; end = cur_item; } else { start = cur_item; end = i; }

		for (int c = start; c <= end; c++) {
			//make each item's state the same as the old curitem state
			//or toggle the state if control is also pressed. toggle is only 
			//between ON and OFF, otherwise curitem->state is used.
			// MenuItem *titem = items->e(c);
			//if (titem->isGrayed()) continue;
			
			if (state & ControlMask) { 
				//toggle
				if (selected.Contains(c)) {
					if (selected.Contains(c)) {
						if (nstate) selected.pushnodup(c);
						else selected.remove(selected.findindex(c));
					}
				}
			} else if (!selected.Contains(c)) { 
				//turn on
				selected.pushnodup(c);
			}
		}
		cur_item = i;
		//curmenuitem = mitem;
		needtodraw |= 2;
	}
}

int GridSelectInterface::MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	//dragged is a rough gauge of the maximum distance the mouse was from the original point
	buttondown.down(d->id,MIDDLEBUTTON,x,y);

	needtodraw = 1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int GridSelectInterface::MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,MIDDLEBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//// NOTE! You probably only need to redefine ContextMenu(), instead of grabbing right button,
//// Default right button is to pop up a Context menu for the coordinate.
//
//int GridSelectInterface::RBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
//{ ***
//	buttondown.down(d->id,RIGHTBUTTON,x,y);
//	return 0; //return 0 for absorbing event, or 1 for ignoring
//}
//
//int GridSelectInterface::RBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
//{ ***
//	buttondown.up(d->id,RIGHTBUTTON);
//	return 0; //return 0 for absorbing event, or 1 for ignoring
//}


int GridSelectInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		// update any mouse over state
		int nhover = scan(x,y,state);
		if (nhover != hover) {
			hover = nhover;
			// DBGL("hover "<<hover);
			needtodraw = 1;
			return 0;
		}
		return 1;
	}

	//else deal with mouse dragging...

	int oldx, oldy;
    int action1, action2;
    buttondown.move(d->id,x,y, &oldx,&oldy);
    buttondown.getextrainfo(d->id,LEFTBUTTON, &action1, &action2);

	

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int GridSelectInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if (state & ControlMask) {
		double aspect = cell_h / cell_w;
		cell_w *= 1.1;
		cell_h *= 1.1;
		if (cell_w > bounds.boxwidth() - 2*gap) {
			cell_w = bounds.boxwidth() - 2*gap;
			cell_h = aspect * cell_w;
		}
		// if (cell_h > bounds.boxheight() - 2*gap)
		// 	cell_h = bounds.boxheight() - 2*gap;
		need_to_remap = true;
		needtodraw = 1;
		return 0;

	} else {
		offset_target.y += cell_h;
		if (!lerp_timer) {
			lerp_timer = app->addtimer(this, 15, 15, -1);
		}
		return 0;
	}
	return 1; //wheel up ignored
}

int GridSelectInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if (state & ControlMask) {
		double aspect = cell_h / cell_w;
		cell_w /= 1.1;
		cell_h /= 1.1;
		if (cell_w < 2 * UIScale() * curwindow->win_themestyle->normal->textheight()) {
			cell_w = 2 * UIScale() * curwindow->win_themestyle->normal->textheight();
			cell_h = aspect * cell_w;
		}
		// if (cell_h < UIScale() * curwindow->win_themestyle->normal->textheight())
		// 	cell_h = UIScale() * curwindow->win_themestyle->normal->textheight();
		need_to_remap = true;
		needtodraw = 1;
		return 0;
	
	} else {
		offset_target.y -= cell_h;
		if (!lerp_timer) {
			lerp_timer = app->addtimer(this, 50, 50, -1);
		}
		return 0;
	}
	return 1; //wheel down ignored
}


/*! Context: 0 for selection changed.
 *
 * Send SimpleMessage with:
 * - info1 = curitem index (which might be -1 for nothing)
 * - info2 = id of curitem
 * - info3 = menuitem->info
 * - info4 = number of selected
 * \endcode
 */
int GridSelectInterface::send(int context)
{
	if (owner || owner_id) {
		SimpleMessage *msg = new SimpleMessage();
		if (selected.n) {
			int i = selected.e[selected.n-1];
			MenuItem *item = items->findFromLine(i);
			makestr(msg->str, item->name);
			msg->info1 = i;
			msg->info2 = item->id;
			msg->info3 = item->info;
		} else {
			msg->info1 = -1;
			msg->info2 = 0;
			msg->info3 = 0;
		}
		msg->info4 = selected.n;
		app->SendMessage(msg, owner ? owner->object_id : owner_id, owner_message, object_id);
	}

	return 0;
}

int GridSelectInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if ((state & LAX_STATE_MASK) == (ControlMask|ShiftMask|AltMask|MetaMask)) {
		//deal with various modified keys...
	}

	if (ch == LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
		if (selected.n == 0) return 1;
		// if (nothing selected) return 1; //need to return on plain escape, so that default switching to Object tool happens
		
		if (select_type == LAX_ONE_ONLY) return 1;

		 //else..
		Clear(nullptr);
		needtodraw = 1;
		return 0;

	} else if (ch == LAX_Up) {
		//*** stuff
	} else if (ch == LAX_Down) {
		//*** stuff
	} else if (ch == LAX_Left) {
		//*** stuff
	} else if (ch == LAX_Right) {
		//*** stuff
	} else if (ch == LAX_Pgup) {
		//*** stuff
	} else if (ch == LAX_Pgdown) {
		//*** stuff

	} else {
		 //default shortcut processing

		if (!sc) GetShortcuts();
		if (sc) {
			int action = sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
			if (action >= 0) {
				return PerformAction(action);
			}
		}
	}

	return 1; //key not dealt with, propagate to next interface
}

int GridSelectInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	return 1; //key not dealt with
}

Laxkit::ShortcutHandler *GridSelectInterface::GetShortcuts()
{
	// if (sc) return sc;
    // ShortcutManager *manager=GetDefaultShortcutManager();
    // sc=manager->NewHandler(whattype());
    // if (sc) return sc;

    // //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    // sc=new ShortcutHandler(whattype());

	// //sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    // sc->Add(GRIDSELECT_Something,  'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),nullptr,0);
    // sc->Add(GRIDSELECT_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),nullptr,0);
    // sc->Add(GRIDSELECT_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),nullptr,0);
	// sc->Add(GRIDSELECT_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),nullptr,0);
	// sc->AddShortcut('=',0,0, GRIDSELECT_Something); //add key to existing action

    // manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int GridSelectInterface::PerformAction(int action)
{
	return 1;
}

bool GridSelectInterface::Select(int id)
{
	if (!items) return false;
	int index = -1;
	items->findid(id, &index);
	if (index < 0) return false;
	addselect(index, 0);
	return true;
}

bool GridSelectInterface::SelectIndex(int index)
{
	if (index < 0 || index >= items->n()) return false;
	addselect(index, 0);
	return true;
}

void GridSelectInterface::UseThisMenu(Laxkit::MenuInfo *newmenu)
{
	if (!newmenu) return;
	if (newmenu == items) return;
	if (items) items->dec_count();
	items = newmenu;
	items->inc_count();
	Clear(nullptr);
	RemapItems();
}

} // namespace LaxInterfaces

