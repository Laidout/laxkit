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
//    Copyright (C) 2024 by Tom Lechner
//



#include <lax/interfaces/sliderinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>

#include <lax/debug.h>


using namespace Laxkit;


namespace LaxInterfaces {


//--------------------------- SliderInfo -------------------------------------

/*! \class SliderInfo
 * \ingroup interfaces
 * \brief Data that SliderInterface can use.
 */

SliderInfo::SliderInfo()
{
	to.x          = 1.0;
	graphic       = THING_Circle;
	graphic_fill_type  = 1;
	img           = nullptr;
	line_width    = 1;
	outline_width = .1;
	graphic_size  = -1;

	graphic_fill   .rgbf(1,0,0);
  	graphic_stroke .rgbf(.5,0,0);
  	line_fill      .rgbf(.6,0,0);
  	line_stroke   .rgbf(.2,0,0);
}


SliderInfo::~SliderInfo()
{}


void SliderInfo::FindBBox()
{
	ClearBBox();
	addtobounds(from);
	addtobounds(to);
	if (graphic_size > 0) ExpandBounds(graphic_size/2);
}


Laxkit::anObject *SliderInfo::duplicate()
{
	SliderInfo *dup = new SliderInfo();
	*dup = *this;
	return dup;
}


//--------------------------- SliderToolSettings -------------------------------------

SliderToolSettings::~SliderToolSettings()
{
	if (default_style) default_style->dec_count();
}


//--------------------------- SliderInterface -------------------------------------

/*! \class SliderInterface
 * \ingroup interfaces
 * \brief An interface to move a ball along a rail that corresponds to a value between a min and max.
 */

/*! Static singleton keeper. static so that it is easily shared between all tool instances. */
Laxkit::SingletonKeeper SliderInterface::settingsObject;

SliderInterface::SliderInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_flags = 0;

	showdecs   = 1;
	needtodraw = 1;
	hover      = 0;

	//dataoc     = nullptr;
	info       = nullptr;

	sc = nullptr; //shortcut list, define as needed in GetShortcuts()

	settings = dynamic_cast<SliderToolSettings*>(settingsObject.GetObject());
	if (!settings) {
		settings = new SliderToolSettings();
		settingsObject.SetObject(settings, false);
		settings->dec_count();
	}
}


SliderInterface::~SliderInterface()
{
	//if (dataoc) delete dataoc;
	if (info) { info->dec_count(); }
	if (sc) sc->dec_count();
}


const char *SliderInterface::whatdatatype()
{ 
	//return "SliderInfo";
	return nullptr; // nullptr means this tool is creation only, it cannot edit existing data automatically
}


/*! Name as displayed in menus, for instance.
 */
const char *SliderInterface::Name()
{ return _("Slider Interface"); }


//! Return new SliderInterface.
/*! If dup!=nullptr and it cannot be cast to SliderInterface, then return nullptr.
 */
anInterface *SliderInterface::duplicateInterface(anInterface *dup)
{
	if (dup == nullptr) dup = new SliderInterface(nullptr,id,nullptr);
	else if (!dynamic_cast<SliderInterface *>(dup)) return nullptr;
	return anInterface::duplicateInterface(dup);
}


// //! Use the object at oc if it is an SliderInfo.
// int SliderInterface::UseThisObject(ObjectContext *oc)
// {
// 	if (!oc) return 0;

// 	SliderInfo *ndata = dynamic_cast<SliderInfo *>(oc->obj);
// 	if (!ndata) return 0;

// 	if (data && data != ndata) deletedata();
// 	if (dataoc) delete dataoc;
// 	dataoc = oc->duplicate();

// 	if (data != ndata) {
// 		data = ndata;
// 		data->inc_count();
// 	}
// 	needtodraw = 1;
// 	return 1;
// }


int SliderInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;

	SliderInfo *ninfo = dynamic_cast<SliderInfo*>(nobj);
	if (ninfo) {
		deletedata();
		info = ninfo;
		info->inc_count();
		return 1;
	}

// 	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
// 	if (ls!=nullptr) {
// 		if (mask & (LINESTYLE_Color | LINESTYLE_Color2)) { 
// 			linecolor=ls->color;
// 		}
// //		if (mask & LINESTYLE_Width) {
// //			linecolor.width=ls->width;
// //		}
// 		needtodraw=1;
// 		return 1;
// 	}
	return 0;
}


// /*! Return the object's ObjectContext to make sure that the proper context is already installed
//  * before Refresh() is called.
//  */
// ObjectContext *SliderInterface::Context()
// {
// 	return dataoc;
// }


/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int SliderInterface::InterfaceOn()
{
	showdecs = 1;
	needtodraw = 1;
	return 0;
}


/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int SliderInterface::InterfaceOff()
{
	Clear(nullptr);
	showdecs = 0;
	needtodraw = 1;
	return 0;
}


/*! Clear references to d within the interface.
 */
void SliderInterface::Clear(SomeData *d)
{
	//if (dataoc) { delete dataoc; dataoc=nullptr; }
	//if (data) { data->dec_count(); data=nullptr; }
}


void SliderInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
	if (bounds.boxwidth() > bounds.boxheight()) {
		info->from = flatvector(bounds.minx + info->graphic_size, (bounds.maxy+bounds.miny)/2);
		info->to   = flatvector(bounds.maxx - info->graphic_size, (bounds.maxy+bounds.miny)/2);
	} else {
		info->from = flatvector((bounds.maxx+bounds.minx)/2, bounds.miny + info->graphic_size);
		info->to   = flatvector((bounds.maxx+bounds.minx)/2, bounds.maxy - info->graphic_size);
	}

	needtodraw = 1;
}


/*! Draw some data other than the current data.
 * This is called during screen refreshes.
 */
int SliderInterface::DrawData(anObject *ndata, anObject *a1, anObject *a2, int ninfo)
{
	if (!ndata || dynamic_cast<SliderInfo *>(ndata) == nullptr) return 1;

	SliderInfo *bzd = info;
	info = dynamic_cast<SliderInfo *>(ndata);

	 // store any other state we need to remember
	 // and update to draw just the temporary object, no decorations
	int td = showdecs, ntd = needtodraw;
	showdecs = 0;
	needtodraw = 1;

	Refresh();

	 //now restore the old state
	needtodraw = ntd;
	showdecs = td;
	info = bzd;
	return 1;
}


void SliderInterface::DrawLine()
{
	dp->LineCap(LAXCAP_Round);
	ScreenColor col; 
	if (info->outline_width > 0) {
		col = info->line_stroke;
		if (hover == SLIDER_Rail) col = col.Hinted(.05);
		dp->NewFG(col);
		dp->LineWidth(info->line_width + info->outline_width);
		dp->drawline(info->from, info->to);
	}
	col = info->line_fill;
	if (hover == SLIDER_Rail) col = col.Hinted(.05);
	dp->NewFG(col);
	dp->LineWidth(info->line_width);
	dp->drawline(info->from, info->to);
}

void SliderInterface::DrawBall()
{
	double pos = (info->current - info->min) / (info->max - info->min); //so 0..1
	flatvector p = info->from + pos * (info->to - info->from);

	if (info->img) {
		HEYYOU(" *** IMPLEMENT ME!! img slider ball");
	} else {
		ScreenColor col;
		if (info->graphic_fill_type == 1 || info->graphic_fill_type == 0) {
			col = info->graphic_fill_type == 1 ? info->graphic_fill : info->graphic_stroke;
			if (hover == SLIDER_Ball) col = col.Hinted();
			dp->NewFG(col);
			dp->drawthing(p, info->graphic_size/2, info->graphic_size/2, info->graphic_fill_type, (DrawThingTypes)info->graphic);
		} else {
			col = info->graphic_stroke;
			ScreenColor col2 = info->graphic_fill;
			if (hover == SLIDER_Ball) { col = col.Hinted(); col2 = col2.Hinted(); }
			dp->drawthing(p, info->graphic_size/2, info->graphic_size/2, (DrawThingTypes)info->graphic,
				col.Pixel(), col2.Pixel(), info->outline_width/2);
		}
	}
}

void SliderInterface::DrawLabel()
{
	if (!info->label.IsEmpty()) {
		dp->NewFG(info->graphic_fill);
		dp->font(curwindow->win_themestyle->normal, UIScale()*curwindow->win_themestyle->normal->textheight());
		Utf8String str("%s: %g", info->label.c_str(), info->current);
		
		// if (info->current > (info->max + info->min)/2)
		// 	 dp->textout(bounds.minx,bounds.miny, info->label.c_str(),-1, LAX_TOP|LAX_LEFT);
		// else dp->textout(bounds.maxx,bounds.miny, info->label.c_str(),-1, LAX_TOP|LAX_RIGHT);
		//------
		dp->textout(bounds.minx,bounds.miny, str.c_str(),-1, LAX_TOP|LAX_LEFT);
	}

	DBG dp->LineWidthScreen(1);
	DBG dp->drawrectangle(bounds, 0);
}

int SliderInterface::Refresh()
{
	if (needtodraw == 0) return 0;
	needtodraw = 0;
	if (!info) return 0;

	if (info->is_screen_space) dp->DrawScreen();


	DrawLine();	
	DrawLabel();
	DrawBall();
	
	if (info->is_screen_space) dp->DrawReal();
	return 0;
}


void SliderInterface::deletedata()
{
	if (info) { info->dec_count(); info = nullptr; }
    //if (dataoc) { delete dataoc; dataoc=nullptr; }
}


SliderInfo *SliderInterface::newData()
{
	return new SliderInfo();
}

/*! Based on curwindow->win_themestyle, set up colors in the current info.
 * Does nothing if no curwindow or no info.
 */
void SliderInterface::SetDefaultStyle()
{
	if (!curwindow || !info) return;

	WindowStyle *style = curwindow->win_themestyle;
	info->graphic_fill   = style->fg;
	info->graphic_stroke = style->fg.Lerp(style->bg, .4);
	info->line_fill      = style->fg.Lerp(style->bg, .8);
	info->line_stroke    = style->fg.Lerp(style->bg, .7);
	double th = style->normal->textheight();
	// if (info->graphic_size <= 0) info->graphic_size = th;
	// if (info->line_width <= 0) info->line_width = th/2;
	info->graphic_size = th;
	info->line_width = th*.4;
	info->outline_width = th/8;
}


int SliderInterface::scan(int x, int y, unsigned int state, double *pos_ret)
{
	if (!info) return SLIDER_None;

	flatvector p;
	if (info->is_screen_space) p.set(x,y);
	else p = screentoreal(x,y);

	if (!bounds.boxcontains(p.x, p.y)) return SLIDER_None;

	flatvector v = info->to - info->from;
	double vlen = v.norm();
	double pp = ((p - info->from) * v) / vlen; // should be 0..vlen
	if (pp < 0) pp = 0;
	else if (pp > vlen) pp = vlen;

	int ret = SLIDER_None;
	double cur = (info->current - info->min) / (info->max - info->min) * vlen;
	// DBGM("scan: "<< cur<<"  vlen: "<<vlen);
	if (pp >= cur - info->graphic_size/2 && pp <= cur + info->graphic_size/2) ret = SLIDER_Ball;
	else ret = SLIDER_Rail;

	if (pos_ret) *pos_ret = info->min + (pp / vlen) * (info->max - info->min);
	
	return ret;
}


int SliderInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	double new_pos;
	int nhover = scan(x,y,state, &new_pos);
	if (nhover != hover) {
		hover = nhover;
	}

	if (hover != SLIDER_None) {
		if (hover == SLIDER_Rail) {
			info->current = new_pos;
			Modified();
			hover = SLIDER_Ball;
			needtodraw = 1;
		}
		int offset = 10000 * (new_pos - info->current);
		buttondown.down(d->id,LEFTBUTTON,x,y, hover, offset);
		return 0;
	}

	needtodraw = 1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}


int SliderInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}


void SliderInterface::MouseOut()
{
	if (!buttondown.any()) hover = SLIDER_None;
	needtodraw = 1;
}


int SliderInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		// update any mouse over state
		double pos;
		int nhover = scan(x,y,state, &pos);
		// DBGM(hover<<"  "<<pos);
		// std::cerr << hover<<"  "<<pos<<std::endl;

		if (nhover != hover) {
			hover = nhover;
			//buttondown.down(d->id,LEFTBUTTON,x,y, nhover);

			if (hover == SLIDER_Ball) PostMessage(_("Drag to change"));
			else if (hover == SLIDER_Rail) PostMessage(_("Press to set"));
			needtodraw = 1;
			return 0;
		}
		return 1;
	}

	//else deal with mouse dragging...

	int oldx, oldy;
    int mhover;
    int ioffset;
    buttondown.move(d->id,x,y, &oldx,&oldy);
    buttondown.getextrainfo(d->id,LEFTBUTTON, &mhover, &ioffset);
    double offset = ioffset / 10000.;

    if (mhover == SLIDER_Ball) {
		flatvector pold;
		buttondown.getinitial(d->id,LEFTBUTTON, &oldx,&oldy);
		if (info->is_screen_space) pold.set(oldx,oldy);
		else pold = screentoreal(oldx,oldy);

		flatvector p;
		if (info->is_screen_space) p.set(x,y);
		else p = screentoreal(x,y);

		flatvector v = (info->to - info->from);
		double new_pos = info->min + ((p - info->from) * v / (v*v)) * (info->max - info->min);
		new_pos -= offset;
		info->current = new_pos;

		if (info->max > info->min) {
			if (info->current > info->max) info->current = info->max;
			else if (info->current < info->min) info->current = info->min;
		} else {
			if (info->current > info->min) info->current = info->min;
			else if (info->current < info->max) info->current = info->max;
		}

		Modified();
		needtodraw = 1;
    }


	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}


EventData *SliderInterface::BuildModifiedMessage(int level)
{
	SimpleMessage *msg = nullptr;
	if (info) {
		msg = new SimpleMessage();
		msg->d1 = info->current;
		msg->usertype = level;
		if (!info->id.IsEmpty()) makestr(msg->str, info->id.c_str());
		// app->SendMessage(msg, owner->object_id, owner_message ? owner_message : "Slider", object_id);
	}

	return msg;
}


int SliderInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	// if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask|AltMask|MetaMask)) {
	// 	//deal with various modified keys being pressed...
	// }

	// if (ch == LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
	// 	if (nothing selected) return 1; //need to return on plain escape, so that default switching to Object tool happens
		
	// 	 //else..
	// 	ClearSelection();
	// 	needtodraw=1;
	// 	return 0;

	// } else if (ch==LAX_Up) {
	// 	//*** stuff

	// } else {
	// 	 //default shortcut processing

	// 	if (!sc) GetShortcuts();
	// 	int action = sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	// 	if (action >= 0) {
	// 		return PerformAction(action);
	// 	}
	// }

	return 1; //key not dealt with, propagate to next interface
}


// int SliderInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
// { ***
// 	if ((state&LAX_STATE_MASK) == (ControlMask|ShiftMask|AltMask|MetaMask)) {
// 		//deal with various modified keys being released...
// 	}

// 	return 1; //key not dealt with
// }


Laxkit::ShortcutHandler *SliderInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager = GetDefaultShortcutManager();
    sc = manager->FindHandler(whattype());
    if (sc) return sc;

    sc = new ShortcutHandler(whattype());

	// //sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    // sc->Add(SLIDER_Something,  'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),nullptr,0);
    // sc->Add(SLIDER_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),nullptr,0);
    // sc->Add(SLIDER_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),nullptr,0);
	// sc->Add(SLIDER_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),nullptr,0);
	// sc->AddShortcut('=',0,0, SLIDER_Something); //add key to existing action

    manager->AddArea(whattype(),sc);
    return sc;
}


// /*! Return a context specific menu, typically in response to a right click.
//  */
// Laxkit::MenuInfo *SliderInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
// { ***
// 	if (no menu for x,y) return menu;

// 	if (!menu) menu = new MenuInfo;
// 	if (!menu->n()) menu->AddSep(_("Some new menu header"));

// 	menu->AddToggleItem(_("New checkbox"), YOUR_CHECKBOX_ID, checkbox_info, (istyle & STYLEFLAG) /*on*/, nullptr /*icon*/, -1 /*where*/, 0 /*state*/);
// 	menu->AddItem(_("Some menu item"), YOUR_MENU_VALUE);
// 	menu->AddSep(_("Some separator text")); // a line seperator with some text
// 	menu->AddItem(_("Et Cetera"), YOUR_OTHER_VALUE);
// 	menu->AddSep(); // a line seperator with no text on it

// 	 //include <lax/iconmanager.h> if you want access to default icons
// 	LaxImage *icon = iconmanager->GetIcon("NewDirectory");
// 	menp->AddItem(_("Item with icon"), SOME_ITEM_ID, item_info, icon, -1 /*where*/, 0 /*state*/); //state might be LAX_GRAY for instance

// 	return menu;
// }


// /*! Intercept events if necessary, such as from the ContextMenu().
//  */
// int SliderInterface::Event(const Laxkit::EventData *evdata, const char *mes)
// {
// 	if (!strcmp(mes,"menuevent")) {
// 		 //these are sent by the ContextMenu popup
// 		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(evdata);
// 		int i    = s->info2; //id of menu item
// 		int info = s->info4; //info of menu item

// 		// check actions first
// 		if ( i == SOME_MENU_VALUE
// 		  || i == SOME_OTHER_VALUE
// 		  ) {
// 			PerformAction(i);
// 			return 0;
// 		}

// 		return 0; 
// 	}

// 	return 1; //event not absorbed
// }


/*! Return 0 for action performed, or nonzero for unknown action.
 */
int SliderInterface::PerformAction(int action)
{
	return 1;
}

} // namespace LaxInterfaces

