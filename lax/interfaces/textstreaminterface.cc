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
//    Copyright (C) 2015 by Tom Lechner
//



#include <lax/interfaces/textstreaminterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/fontdialog.h>
#include <lax/laxutils.h>
#include <lax/language.h>


#include <lax/debug.h>


using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class TextStreamInterface
 * \ingroup interfaces
 * Interface to select an object to stream text upon.
 * It can scan for both inside an object area, or on the path of an object.
 */


TextStreamInterface::TextStreamInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	tstream_style = TXT_On_Stroke | TXT_In_Area | TXT_Draggable_Area;

    showdecs   = 1;
    needtodraw = 1;

    sc = nullptr; // shortcut list, define as needed in GetShortcuts()

    extrahover    = nullptr;
    extra_hover   = 0;
    outline_index = -1;

    flowdir.x    = 1;
    close_dist   = 20;
    fontheight   = 36. / 72;
    default_font = nullptr;

    sample_text = _("Aa");

    outline.style |= PathsData::PATHS_Ignore_Weights;
	ScreenColor color(1.,0.,1.,1.);
	outline.line(2, -1, -1, &color);
	outline.linestyle->widthtype = 0;//screen width

	hover_t = 0;
	hover_baseline = 0;
}

TextStreamInterface::~TextStreamInterface()
{
	if (sc) sc->dec_count();
	if (extrahover) { delete extrahover; extrahover = nullptr; }
	if (default_font) default_font->dec_count();
}

const char *TextStreamInterface::whatdatatype()
{ 
	return nullptr; // nullptr means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *TextStreamInterface::Name()
{ return _("Text Stream"); }


//! Return new TextStreamInterface.
/*! If dup!=nullptr and it cannot be cast to TextStreamInterface, then return nullptr.
 */
anInterface *TextStreamInterface::duplicate(anInterface *dup)
{
	if (dup==nullptr) dup=new TextStreamInterface(nullptr,id,nullptr);
	else if (!dynamic_cast<TextStreamInterface *>(dup)) return nullptr;
	return anInterface::duplicate(dup);
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int TextStreamInterface::UseThis(anObject *nobj, unsigned int mask)
{
//	if (!nobj) return 1;
//	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
//	if (ls!=nullptr) {
//      ***
//		needtodraw=1;
//		return 1;
//	}
	return 0;
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int TextStreamInterface::InterfaceOn()
{
	showdecs = 1;
	needtodraw = 1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int TextStreamInterface::InterfaceOff()
{ 
	Clear(nullptr);
	if (extrahover) { delete extrahover; extrahover = nullptr; }
	showdecs = 0;
	needtodraw = 1;
	return 0;
}

void TextStreamInterface::Clear(SomeData *d)
{
}

void TextStreamInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

Laxkit::MenuInfo *TextStreamInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{ 
	if (!menu) menu = new MenuInfo;

	if (!menu->n()) menu->AddSep(_("Text stream"));

	menu->AddItem(_("Default font..."), TXT_Font);

	// stream resource menu
	menu->AddSep(_("Text"));
	menu->AddItem(_("New text"),          TXT_NewText);
	// menu->AddItem(_("Text from file..."), TXT_ImportText);
	menu->AddItem(_("Lorem ipsum"),       TXT_LoremIpsum);
	menu->AddItem(_("Cervantes ipsum"),   TXT_Cervantes);
	
	return menu;
}

int TextStreamInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{
   if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s = dynamic_cast<const SimpleMessage*>(e_data);
		int i = s->info2; //id of menu item

		if ( i == TXT_Font) {
			FontDialog *dialog = new FontDialog(nullptr, "Font",_("Font"),ANXWIN_REMEMBER, 10,10,700,700,0, object_id,"newfont",0,
							nullptr, nullptr, default_font ? default_font->textheight() : fontheight, //fontfamily, fontstyle, fontsize,
							nullptr, //sample text
							default_font, true
							);
			app->rundialog(dialog);

		}

		return 0;

	} else if (!strcmp(mes, "newfont")) {
		DBGM("IMPLEMENT ME!!");
		return 0;
	}

	return 1; //event not absorbed
}



int TextStreamInterface::Refresh()
{

	if (needtodraw == 0) return 0;
	needtodraw = 0;

	DBG cerr <<"--------------TextStreamInterface::Refresh()-------------"<<endl;

	if (extrahover) {
		DBG cerr <<"--------------drawing extrahover-------------"<<extra_hover<<endl;
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
		
		double m[6];

		viewport->transformToContext(m,extrahover,0,-1);
		dp->PushAndNewTransform(m);

		//draw path around object
		dp->LineWidthScreen(3*ScreenLine());
		dp->NewFG(255,0,255);
		outline.linestyle->width = 2*ScreenLine() * (extra_hover == TXT_Hover_Stroke ? 2 : 1);

		InterfaceManager *imanager = InterfaceManager::GetDefault();
		imanager->DrawDataStraight(dp, &outline);
		outline.linestyle->width = 2*ScreenLine();
		
		//---------- Draw baselines
		if (extra_hover == TXT_Hover_Area) {
			dp->PushClip(0);
			SetClipFromPaths(dp, &outline, nullptr, true);

			DoubleBBox box;
			box.addtobounds(extrahover->obj->minx, extrahover->obj->miny);
			box.addtobounds(extrahover->obj->maxx, extrahover->obj->miny);
			box.addtobounds(extrahover->obj->maxx, extrahover->obj->maxy);
			box.addtobounds(extrahover->obj->minx, extrahover->obj->maxy);


			flatpoint pp, p1, p2, mid;
			flatpoint ydir = -transpose(flowdir);
			pp = flatpoint(extrahover->obj->minx,extrahover->obj->maxy);
			//double mag = get_imagnification(m, 1,0);
			// double mm[6];
			// transform_invert(mm, extrahover->obj->m());

			for (int c = 0; ; c++) {
				DBGM("drawing baseline "<<c);
				flatline l(pp, pp + flowdir);
				if (!extrahover->obj->IntersectWithLine(l, &p1, &p2, nullptr, nullptr)) break;
				// mid = (p1 + p2)/2 + fontheight*.5*ydir;
				// if (!extrahover->obj->pointin(extrahover->obj->transformPoint(mid))) break;

				dp->drawline(p1, p2);

				pp += fontheight * ydir;
			}

			dp->PopClip();

		} else if (extra_hover == TXT_Hover_Stroke) {
			dp->NewFG(100,100,255);
			dp->drawarrow(hover_point, hover_direction, 0, 20*ScreenLine(), 0);
			dp->fontsize(fontheight);
			flatpoint p = hover_direction.transpose().normalized();
			// DBGM("******************************************************************* baseline: "<<hover_baseline<<"  fh: "<<fontheight<<"  tp: "<<p.x<<","<<p.y);
			p = hover_baseline * fontheight * p + hover_point;
			double angle = atan2(-hover_direction.y,hover_direction.x);
			if (hover_reverse_dir) angle += M_PI;
			dp->textout(angle, p.x,p.y, sample_text.c_str(),-1, LAX_LEFT|LAX_BOTTOM|LAX_FLIP);
		}

		dp->PopAxes(); 
	}

	return 0;
}

/*! Stub to allow subclasses to do something in response to a click down on a streamable area
 * without having to refedine LBDown,etc.
 *
 * This is called on LBDown when there is an extrahover.
 */
bool TextStreamInterface::AttachStream()
{
	return false;
}

int TextStreamInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	buttondown.down(d->id,LEFTBUTTON,x,y);

	if (extrahover) {
		AttachStream();
		delete extrahover;
		extrahover = nullptr;
	}

	//int device=d->subid; //normal id is the core mouse, not the controlling sub device
	//DBG cerr <<"device: "<<d->id<<"  subdevice: "<<d->subid<<endl;
	//LaxDevice *dv=app->devicemanager->findDevice(device);
	//device_name=dv->name;

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int TextStreamInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}


/*! Return what (x,y) is most near, plus the index of the path in extrahover.
 * Note: does not change the currently tracked object! Need to use Track() for that.
 */
int TextStreamInterface::scanInCurrent(int x,int y,unsigned int state, int &index, flatpoint &hovered, float &hovered_t, bool &outside, bool &reversed)
{
	if (!extrahover) return TXT_None;

	double m[6];
	flatpoint p = dp->screentoreal(x,y);
	viewport->transformToContext(m,extrahover,0,1);
	p = transform_point_inverse(m, p); //p should now be in object space

	PathsData *pathsobj = dynamic_cast<PathsData*>(extrahover->obj);
	if (pathsobj && search_on_outline) {

		double dist = 0, distalong = 0, tdist = 0;
		int pathi;
		flatpoint pp = pathsobj->ClosestPoint(p, &dist, &distalong, &tdist, &pathi);

		// *** check distance within bounds
		DBGM("--- dist: "<<dist<<"  distalong: "<<distalong<<"  tdist: "<<tdist<<"  fontsize: "<<fontheight)
	
		// *** check direction compared to path

		if (pathi >= 0 && dist < fontheight*.5) {
			index = pathi;
			hovered = pp;
			hovered_t = tdist;
			if (state & ControlMask) reversed = true; else reversed = false;
			pathsobj->PointAlongPath(index, tdist, 0, nullptr, &hover_direction);
			flatvector v = p - pp;
			flatvector vt = hover_direction.transpose();
			outside = ((v*vt) >= 0);
			DBGM("******************************************************************************"<< (v*vt) <<"  "<<reversed);

			return TXT_Hover_Stroke;
		}
	}
	// else 
	{ // can't resolve to a path, so just use object default in/out
		if (extrahover->obj->pointin(extrahover->obj->transformPoint(p), 1)) {
			index = 0;
			return TXT_Hover_Area;
		}
	}

	index = -1;
	return TXT_None;
}

/*! Track the object defined by oc (copies oc). 
 * Does not select proper subline, nor scan for point closeness.
 * Only installs this object as most relevant for scanning.
 */
int TextStreamInterface::Track(ObjectContext *oc)
{
	if (!oc) {
		DBG cerr <<" -- tracking nothing"<<endl;
		if (extrahover) { delete extrahover; extrahover=nullptr; }
		needtodraw=1;
		DBG PostMessage("");
		return 0;
	}

	DBG cerr <<" -- track "<<oc->obj->Id()<<endl;
	DBG PostMessage(oc->obj->Id());

	if (extrahover && oc->isequal(extrahover)) return 0; //already tracked!

	if (extrahover) delete extrahover; 
	extrahover = oc->duplicate();

	//determine inset path in which to lay text
	//determine outline path.. depending on mode, we need:
	//    base path
	//    outer stroke line
	//    inner stroke line
	//
	//hover move changes insertion point
	//hover in area activates rotation handles
	//drag in area change baseline offset
	//click and drag to change offset off line


	 //cache the path for mouseover use

	DefineOutline(0);


	needtodraw=1;

	return 0;
}

/*! Define the path to show a hover indication.
 */
int TextStreamInterface::DefineOutline(int which)
{
	outline.clear();

	if (!extrahover) return -1;

	if (dynamic_cast<PathsData*>(extrahover->obj)) {
		PathsData *paths = dynamic_cast<PathsData*>(extrahover->obj);

		if (outline_index < 0) outline_index = 0;
		Path *npath = paths->GetOffsetPath(outline_index);
		if (npath) outline.paths.push(npath); 
		return 0;
	}
	
	//create basic bounding box outline as default:
	Affine m = extrahover->obj->GetTransformToContext(false, 0);
	outline.moveTo(flatpoint(extrahover->obj->minx,extrahover->obj->miny));
	// DBG Coordinate *cc=outline.LastVertex();
	// DBG cerr <<"--hover\n  moveto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;

	outline.lineTo(flatpoint(extrahover->obj->maxx,extrahover->obj->miny));
	// DBG cc=outline.LastVertex(); cerr <<"  lineto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;

	outline.lineTo(flatpoint(extrahover->obj->maxx,extrahover->obj->maxy));
	// DBG cc=outline.LastVertex(); cerr <<"  lineto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;

	outline.lineTo(flatpoint(extrahover->obj->minx,extrahover->obj->maxy));
	// DBG cc=outline.LastVertex(); cerr <<"  lineto: "<<cc->fp.x<<", "<<cc->fp.y<<endl;


	outline.close();
	
	return 0;
}

int TextStreamInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		int index = -1;
		flatpoint hovered;
		float hovert;
		bool outside = false;
		bool reversed = false;
		int hover = scanInCurrent(x,y,state, index,hovered,hovert, outside, reversed); //searches for hits on last known object
		DBGM("----textstream hover: "<<hover)

		if (hover == TXT_None) {
			// did not hover over part of current extrahover.
			// search for new data to use.
			ObjectContext *oc = nullptr;
			int c = viewport->FindObject(x,y, nullptr, nullptr,1,&oc);

			if (c > 0) {
				//found object, so set up with it
				DBG cerr <<"textstream mouse over: "<<oc->obj->Id()<<endl;

				needtodraw = 1;
				Track(oc);
				hover = scanInCurrent(x,y,state, index,hovered,hovert, outside, reversed);
				DefineOutline(index);

				if (hover == TXT_None) hover = TXT_Hover_New;
			} else Track(nullptr); 
		}

		if (extrahover && hover == TXT_None) {
			delete extrahover;
			extrahover = nullptr;
			needtodraw = 1;
		}

		if (extrahover && hover != TXT_Hover_New && outline_index != index) {
			DefineOutline(index);
			needtodraw = 1;
		}

		if (outline_index != index) {
			outline_index = index;
			needtodraw = 1;
		}
		if (extra_hover != hover) {
			extra_hover = hover;
			needtodraw = 1;
		}
		if (extra_hover == TXT_Hover_Stroke) {
			if (hovert != hover_t || hovered != hover_point || outside != hover_outside || reversed != hover_reverse_dir) {
				hover_point = hovered;
				hover_t = hovert;
				hover_reverse_dir = reversed;
				hover_outside = outside;
				hover_baseline = hover_reverse_dir ? (hover_outside ? 1 : 0) : (hover_outside ? 0 : -1);
				needtodraw = 1;
			}
		}
		return 0;
	}

	//else deal with mouse dragging...
	

	//needtodraw = 1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int TextStreamInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1; //wheel up ignored
}

int TextStreamInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	return 1; //wheel down ignored
}


int TextStreamInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"TextStreamInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,nullptr);
//	}

	return 0;
}

int TextStreamInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{

	if (!sc) GetShortcuts();
	int action=(sc ? sc->FindActionNumber(ch,state&LAX_STATE_MASK,0) : 0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1; //key not dealt with, propagate to next interface
}

int TextStreamInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	return 1; //key not dealt with
}

Laxkit::ShortcutHandler *TextStreamInterface::GetShortcuts()
{
//	if (sc) return sc;
//    ShortcutManager *manager=GetDefaultShortcutManager();
//    sc=manager->NewHandler(whattype());
//    if (sc) return sc;
//
//    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);
//
//    sc=new ShortcutHandler(whattype());
//
//	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
//    sc->Add(CAPT_BaselineJustify, 'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),nullptr,0);
//    sc->Add(CAPT_BottomJustify,   'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),nullptr,0);
//    sc->Add(CAPT_Decorations,     'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),nullptr,0);
//	sc->Add(VIEWPORT_ZoomIn,      '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),nullptr,0);
//	sc->AddShortcut('=',0,0, VIEWPORT_ZoomIn); //add key to existing action
//
//    manager->AddArea(whattype(),sc);
    return sc;
}

int TextStreamInterface::PerformAction(int action)
{
	if (action == TXT_Font) {
		FontDialog *dialog = new FontDialog(nullptr, "Font",_("Font"),ANXWIN_REMEMBER, 10,10,700,700,0, object_id,"newfont",0,
							nullptr, nullptr, default_font ? default_font->textheight() : fontheight, //fontfamily, fontstyle, fontsize,
							nullptr, //sample text
							default_font, true
							);
		app->rundialog(dialog);
		return 0;
	
	} else if (action == TXT_NewText) {
		text_to_place_hint = TXT_NewText;
		needtodraw = 1;
		return 0;

	} else if (action == TXT_LoremIpsum) {
		text_to_place_hint = TXT_LoremIpsum;
		text_to_place = LoremIpsum();
		needtodraw = 1;
		return 0;

	} else if (action == TXT_Cervantes) {
		text_to_place_hint = TXT_Cervantes;
		text_to_place = CervantesIpsum();
		needtodraw = 1;
		return 0;
	}

	return 1;
}

void TextStreamInterface::FlowDir(flatpoint dir)
{
	flowdir = dir;
}

void TextStreamInterface::FontHeight(double height)
{
	fontheight = height;
}

} // namespace LaxInterfaces

