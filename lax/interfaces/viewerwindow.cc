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
//    Copyright (c) 2004-2007,2010-2011 Tom Lechner
//


#include <lax/interfaces/viewerwindow.h>
#include <lax/colorevents.h>
#include <lax/colorbox.h>
#include <lax/units.h>
#include <lax/menubutton.h>
#include <lax/language.h>


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;

namespace LaxInterfaces {

/*! \class ViewerWindow
 * \brief Class for providing a ViewportWindow together with rulers, scrollers, etc.
 *
 * Currently, this ONLY has the above + messagebar, it is not set up to easily add other
 * little controls for the viewport portion. 
 * 
 * ViewerWindow maintains a stack of tools in the form of anInterface instances.
 * Only one tool can be active on the viewport->interfaces stack
 * at one time. Use SelectTool(), AddTool(), and RemoveTool() to adjust the tool stack.
 * Users would call viewer->AddTool(ImageInterface, 1, 1) to add a locally owned ImageInterface tool
 * and make it the current tool. In addition to that, ViewerWindow can pass along interfaces to
 * the viewport->interfaces stack so you can have several interfaces active at the same time in addition
 * to the single tool. See PushInterface() and PopInterface().
 *
 * 
 * \todo It would be nice to have 
 *   the scrollers/rulers/other in some sort of separate container, independent of 
 *   the interface system... ScrolledWindow just has scrollers, not rulers.
 * \todo rulers should always extend over whole viewport. Scrollers don't really have to.
 * \todo have AddToTop/Bottom(other window, etc), AddToLeft/Right, which would
 *   sqeeze in other little windows where the scrollers are.
 * \todo might be useful to have function to change the viewport at any time.
 *
 * Possible window styles:
 * \code
 * #define VIEWER_BACK_BUFFER  (1<<16)
 * \endcode
 */


	// the default border for the scrollers and rulers...
#define BORDER 1

//! Constructor
/*! When creating a ViewerWindow, a valid ViewportWindow must be passed in.
 * If NULL is passed in, you will surely crash your program later on, as most
 * of ViewerWindow assumes a valid ViewportWindow exists in the viewport variable.
 * The viewport need not have been app->addwindow()'d. That is done in init().
 * 
 * The count of vw is incremented, not absorbed.
 *
 * These flags are passed to the RowFrame base class:
 *  ROWFRAME_ROWS|ROWFRAME_STRETCH_IN_COL|ROWFRAME_STRETCH_IN_ROW
 */
ViewerWindow::ViewerWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						ViewportWindow *vw)//vw=NULL
		: RowFrame(parnt,nname,ntitle,(nstyle&ANXWIN_MASK)|ROWFRAME_ROWS|ROWFRAME_STRETCH_IN_COL|ROWFRAME_STRETCH_IN_ROW,
					xx,yy,ww,hh,brder, NULL,0,NULL,
					0)
{
	viewer_style = nstyle & VIEWPORT_STYLE_MASK;

	xscroller = yscroller = NULL;
	xruler = yruler = NULL;
	mesbar          = NULL;
	last_message    = nullptr;
	last_message_n  = 0;

	viewport=vw;
	if (viewport) {
		app->reparent(vw,this); //adds to kids stack
	} else {
		//viewport=new ViewportWindow(this,"viewport",ANXWIN_HOVER_FOCUS|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,0,0,0,0,0);
	}

	curtool = NULL;
	//lazytool=1; //forces change of primary
	lazytool=0;

	sc=NULL;
}

ViewerWindow::~ViewerWindow()
{
	if (sc) sc->dec_count();
}

//! Set the text of mesbar if it exists, or call app->postmessage(mes).
void ViewerWindow::PostMessage(const char *mes)
{
	if (!mesbar) { if (win_on) app->postmessage(mes); }
	else mesbar->SetText(mes);
}

/*! Printf style message.
 */
void ViewerWindow::PostMessage2(const char *fmt, ...)
{
	va_list arg;

    va_start(arg, fmt);
    int c = vsnprintf(last_message, last_message_n, fmt, arg);
    va_end(arg);

    if (c >= last_message_n) {
        delete[] last_message;
        last_message_n = c+100;
        last_message = new char[last_message_n];
        va_start(arg, fmt);
        vsnprintf(last_message, last_message_n, fmt, arg);
        va_end(arg);
    }

    PostMessage(last_message);
}

//! Try to ensure that the units in the rulers reflect some measure of reality for pixel size.
/*! This uses anXApp::ScreenInfo() to try to figure out how big pixels are on your monitor,
 * and configure the rulers accordingly.
 *
 * \todo **** IMPLEMENT ME!! should somehow use a syncwithscreen option in RulerWindow
 *    this is troublesome because the values returned by X are not very reliable
 */
void ViewerWindow::SetRealUnits()
{
	 //for the rulers, figure out the pixel resolution
	 //note that this will fail if x and y resolution differ
	int w,h,mmw,mmh;
	app->ScreenInfo(0, NULL,NULL,&w,&h,&mmw,&mmh,NULL, NULL);
	GetUnitManager()->PixelSize(((double)mmw)/w,UNITS_MM);
}

/*! Adds rulers, scrollers, and a messagebar.
 *
 * \todo *** app->addwindow(viewport) is called here, which might be a little silly.
 */
int ViewerWindow::init()
{
	 //create any rulers or scrollers that we still need to create
	if (!xscroller && !(viewer_style&VIEWPORT_NO_XSCROLLER))
		xscroller=new Scroller(this,"vw X Scroller",NULL,SC_ABOTTOM|SC_XSCROLL|SC_ZOOM, 
						0,0,0,0, 0, NULL,viewport->object_id,"xscroller", 
						viewport->panner,0);
	if (!yscroller && !(viewer_style&VIEWPORT_NO_YSCROLLER))
		yscroller=new Scroller(this,"vw Y Scroller",NULL,SC_ABOTTOM|SC_YSCROLL|SC_ZOOM, 
						0,0,0,0, 0, NULL,viewport->object_id,"yscroller", 
						viewport->panner,0);
	if (!xruler && !(viewer_style&VIEWPORT_NO_XRULER))
		xruler=new RulerWindow(this,"vw X Ruler",NULL,RULER_X|RULER_UNITS_MENU, 0,0,0,0,BORDER, NULL,viewport->object_id,"ruler");
	if (!yruler && !(viewer_style&VIEWPORT_NO_YRULER))
		yruler=new RulerWindow(this,"vw Y Ruler",NULL,RULER_Y|RULER_UNITS_MENU, 0,0,0,0,BORDER, NULL,viewport->object_id,"ruler");

	if (!mesbar) mesbar=new MessageBar(this,"vw mesbar",NULL,MB_MOVE, 0,0,0,0,BORDER, curtool ? curtool->Name() : "Blah Blah Blah");

	if (xscroller) viewport->panner->tell(xscroller);//these might be redundant
	if (yscroller) viewport->panner->tell(yscroller);

	 //now actually add the windows

	double th = win_themestyle->normal->textheight();
	rulerh    = th * 1.4;
	scrollerh = th * 1.2;

	if (yruler) AddWin(NULL,0,    rulerh,0,0,50,0,    rulerh,0,0,50,0, -1); // spacer square between rulers
	if (xruler) AddWin(xruler,1,  10000,9990,0,50,0,  rulerh,0,0,50,0, -1);
	if (yruler) AddWin(yruler,1,  rulerh,0,0,50,0,    20,0,10000,50,0, -1);

	AddWin(viewport,0,  10,0,10000,50,0,    20,0,10000,50,0, -1); //do not absorb count, since count is _kids count!

	if (yscroller) AddWin(yscroller,1, scrollerh,0,0,50,0, 20,0,10000,50,0, -1);

	AddNull();
	
	if (xscroller) AddWin(xscroller,1, 100,50,10000,50,0, scrollerh,0,0,50,0, -1);
	//if (xscroller && yscroller) AddWin(panpopup,1, scrollerh,0,0,50,0, scrollerh,0,0,50,0, -1);

	if (!(viewer_style&VIEWPORT_NO_ZOOM_MENU)) {
		 // add viewer zoom/reset menu
		MenuInfo *zoommenu=GetZoomMenu();
  		MenuButton *menub=new MenuButton(this,"zoommenu",NULL,
                         MENUBUTTON_ICON_ONLY|MENUBUTTON_LEFT,
                         //MENUBUTTON_CLICK_CALLS_OWNER|MENUBUTTON_ICON_ONLY|MENUBUTTON_LEFT,
                         0,0,0,0,0,
                         NULL,object_id,"zoommenu",
                         0,
                         zoommenu,1, //menu
                         "o", //label
                         NULL,NULL,
                         win_themestyle->normal->textheight()/4);
		menub->SetGraphic(THING_Magnifying_Glass, 0,0);
		AddWin(menub,1, scrollerh,0,scrollerh,50,0, scrollerh,0,scrollerh,50,0, -1); 
		 //note: this will be put next to mesbar if no xscroller
	}

	if (xscroller) AddNull();
	
	AddWin(mesbar,1, 30,0,10000,50,0, rulerh,0,0,50,0, -1);

	//add a final corner config button with zoom options:
	//  zoom 1:1                1
	//  zoom in                 =
	//  zoom out                -
	//  clear zoom rotation     '\'
	//  rotate view +/-90 deg   [  ]
	//  reset view              +' '
	//  center                  ' '

	Sync(1);
	viewport->UseTheseRulers(xruler,yruler);
	viewport->UseTheseScrollers(xscroller,yscroller);
	//app->addwindow(viewport); //*** clunky here!

	return 0;
	//return RowFrame::init(); //<- this just calls Sync(1), so no need to call it
}

/*! If there is a child window named "colorbox", then set with the color event.
 */
int ViewerWindow::Event(const Laxkit::EventData *e,const char *mes)
{
	if (e->type==LAX_ColorEvent && !strcmp(mes,"make curcolor")) { 
		ColorBox *colorbox=dynamic_cast<ColorBox*>(findChildWindowByName("colorbox"));
		if (!colorbox) return 0;

		const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(e);
		if (!ce) {
			const ColorEventData *cce=dynamic_cast<const ColorEventData *>(e);
			if (!cce) return 0;

			Color *color = cce->color;
			colorbox->SetIndex(cce->colorindex);
			colorbox->Set(color->colorsystemid, color->ChannelValue(0), 
												color->ChannelValue(1), 
												color->ChannelValue(2), 
												color->ChannelValue(3), 
												color->ChannelValue(4)); 
			if (cce->colormode == 0) colorbox->SetMode(COLORBOX_FG);
			else if (cce->colormode == COLOR_FGBG) colorbox->SetMode(COLORBOX_FGBG);
			else if (cce->colormode == COLOR_StrokeFill) colorbox->SetMode(COLORBOX_STROKEFILL);
			return 0;
		}

		colorbox->SetIndex(ce->colorindex);
		if (ce->colorsystem==LAX_COLOR_GRAY)
			colorbox->SetGray(ce->channels[0]/(double)ce->max,ce->channels[1]/(double)ce->max);

		else if (ce->colorsystem==LAX_COLOR_RGB)
			colorbox->SetRGB(ce->channels[0]/(double)ce->max,
							 ce->channels[1]/(double)ce->max,
						  	 ce->channels[2]/(double)ce->max,
						 	 ce->channels[3]/(double)ce->max);

		else if (ce->colorsystem==LAX_COLOR_CMYK)
			colorbox->SetCMYK(ce->channels[0]/(double)ce->max,
							  ce->channels[1]/(double)ce->max,
							  ce->channels[2]/(double)ce->max,
							  ce->channels[3]/(double)ce->max,
							  ce->channels[4]/(double)ce->max);

		if (ce->colormode == 0) colorbox->SetMode(COLORBOX_FG);
		else if (ce->colormode == COLOR_FGBG) colorbox->SetMode(COLORBOX_FGBG);
		else if (ce->colormode == COLOR_StrokeFill) colorbox->SetMode(COLORBOX_STROKEFILL);

		return 0;

    } else if (!strcmp(mes,"zoommenu")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage *>(e);
        if (!s) return 1;
        int action=s->info2;
        viewport->PerformAction(action);
        return 0; 
	}

	return anXWindow::Event(e,mes);
}

/*! Default is to return a menu with various zoom and canvas rotation VIEWPORT_* ids.
 */
MenuInfo *ViewerWindow::GetZoomMenu()
{
	MenuInfo *menu=new MenuInfo;

	menu->AddItem(_("Default zoom"),     VIEWPORT_Default_Zoom);
	menu->AddItem(_("Zoom in"),          VIEWPORT_ZoomIn);
	menu->AddItem(_("Zoom out"),         VIEWPORT_ZoomOut);
	menu->AddItem(_("Center view"),      VIEWPORT_Center_View);
	menu->AddItem(_("Zoom to fit"),      VIEWPORT_Zoom_To_Fit);
	menu->AddItem(_("Center on object"), VIEWPORT_Center_Object);
	menu->AddItem(_("Zoom to object"),   VIEWPORT_Zoom_To_Object);
	//menu->AddItem(_("Zoom to width"),    VIEWPORT_Zoom_To_Width);
	//menu->AddItem(_("Zoom to height"),   VIEWPORT_Zoom_To_Height);
	menu->AddSep();
	menu->AddItem(_("Reset rotation"),   VIEWPORT_Reset_Rotation);
	//menu->AddItem(_("Rotate 0"),         VIEWPORT_Rotate_0);
	menu->AddItem(_("Rotate 90"),        VIEWPORT_Rotate_90);
	menu->AddItem(_("Rotate 180"),       VIEWPORT_Rotate_180);
	menu->AddItem(_("Rotate 270"),       VIEWPORT_Rotate_270);
	//menu->AddSep();
	//menu->AddItem(_("Set default zoom"), VIEWPORT_Set_Default_Zoom);

	return menu;
}

//! Return the interface in tools with whattype equal to which.
anInterface *ViewerWindow::FindInterface(const char *which)
{
	for (int c=0; c<tools.n; c++) {
		if (!strcmp(tools.e[c]->whattype(),which)) return tools.e[c];
	}
	return NULL;
}

//! Remove tool with id from the list of tools.
int ViewerWindow::RemoveTool(int id)
{
	int c;
	for (c=0; c<tools.n; c++) if (id==tools.e[c]->id) break;
	if (c<tools.n) return tools.remove(c); 
	return 1;
}

//! Add i to tools, and make it the current tool if selectalso with call to SelectTool(i->id).
/*! i->Dp(&viewport->dp) is called.
 *
 * The way I do it, this interface will be local to ViewerWindow, and should be a copy of
 * an interface from a pool somewhere. This way, the interface can maintain its own state variables,
 * and settings its dp to viewportwindow->dp is normal and expected.
 *
 * So when adding tools, say you keep your base interfaces in pool stack, then step
 * through the stack doing AddTool(pool[c]->duplicate(NULL),1,0) will add local copies of the interfaces,
 * and set their dp to the right thing. Works for me...
 */
int ViewerWindow::AddTool(anInterface *i, char selectalso, int absorbcount)
{
	if (!i) return 1;
	DBG cerr <<"-----AddTool: "<<i->whattype()<<" with id="<<i->id<<endl;
	tools.pushnodup(i,3);
	i->Dp(viewport->dp);
	if (selectalso) SelectTool(i->id);
	if (absorbcount) i->dec_count();
	return 0;
}

//! Select a tool that can handle datatype, where datatype==data->whattype==interface->whatdatatype().
/*! Return 0 if curtool has been adjusted and there is a tool change, 
 * 2 for unknown datatype, 1 for successful tool change, and the tool cannot use data.
 */
int ViewerWindow::SelectToolFor(const char *datatype,ObjectContext *oc)//data=NULL
{
	if (!datatype && !oc) return 2;
	if (oc && !oc->obj) return 2;
	if (!datatype) datatype=oc->obj->whattype();
	if (!datatype) return 2;

	int c;
	for (c=0; c<tools.n; c++) {
		//if (!strcmp(tools.e[c]->whatdatatype(),datatype)) {
		if (tools.e[c]->draws(datatype)) {
			SelectTool(tools.e[c]->id);
			break;
		}
	}
	if (c == tools.n) return 2;
	return !curtool->UseThisObject(oc); // UseThis returns 1 for used..
}

/*! Make the tool with the given whattype() the current tool.
 * Basically, find that tool in this->tools, then call SelectTool(id).
 */
int ViewerWindow::SelectTool(const char *type)
{
	for (int ti=0; ti<tools.n; ti++)
		if (!strcmp(type,tools.e[ti]->whattype())) return SelectTool(tools.e[ti]->id);
	return 1;
}

/*! Set visual display of current tool to look like interf.
 * Note this DOES NOT update tool stack, but WILL set curtool to interf.
 * If interf is not in the current interface stack, nothing is done and 0 is returned, else 1 is returned.
 */
int ViewerWindow::SetAsCurrentTool(anInterface *interf)
{
	if (curtool == interf) return 1;
	int i = -1;
	for (int c=0; c<tools.n; c++) {
		if (!strcmp(tools.e[c]->Name(), interf->Name())) {
			i = c;
			break;
		}
	}
	if (i<0) return 0;

	curtool = interf;

	return 1;
}

//! Make the tool with this id the current tool, or previous (id==-2), next (id==-1), or first (id==0). 
/*! Return 1 for error, 0 for tool selected, -1 for overlay toggled.
 *
 * Calls ViewportWindow::Push and ViewportWindow::Pop to put on and off the tools.
 * Remember that Pop also removes any children of the interface.
 *
 * If the tool is an overlay, then calling this will toggle whether it is on or off.
 */
int ViewerWindow::SelectTool(int id)
{
	if (tools.n==0) return 1;
	if (curtool && curtool->id==id) return 0;

	int ti; //index of tool to select in tools stack
	if (id==-2 || id==-1) { // select previous or next
		if (!curtool) ti=0;
		else {
			ti=tools.findindex(curtool);
			if (ti>=0) {
				if (id==-2) {
					 //previous tool, skip overlays
					ti--;
					if (ti<0) ti=tools.n-1;
					while (ti>=0 && tools.e[ti]->interface_type==INTERFACE_Overlay) ti--;
					if (ti<0) ti=0;
				} else {
					 //next tool, skip overlays
					ti++; if (ti==tools.n) ti=0;
					while (ti<tools.n && tools.e[ti]->interface_type==INTERFACE_Overlay) ti++;
					if (ti==tools.n) ti=0;
				}
			} else ti=0;
		}
	} else if (id==0) ti=0;
	else for (ti=0; ti<tools.n; ti++) if (id==tools.e[ti]->id) break;

	if (ti==tools.n || ti<0) { 
		DBG cerr <<"********* no tool found for id "<<id<<endl; 
		return 1;
	}

	if (tools.e[ti]->interface_type==INTERFACE_Overlay) {
		int i=viewport->HasInterface(tools.e[ti]->id);
		if (i) {
			 //pop overlay
			viewport->Pop(tools.e[ti],1);
		} else {
			 //push overlay
			viewport->Push(tools.e[ti],-1,0); // this also calls the InterfaceOn
			if (mesbar) mesbar->SetText(tools.e[ti]->Name());
		}
		return -1;
	}

	 //else assume is tool like interface, not overlay
	if (curtool) viewport->Pop(curtool,1); //pop old tool
	curtool = tools.e[ti];

	DBG cerr <<"ViewerWindow::SelectTool id="<<id<<" ti="<<ti<<": "<<curtool->whattype()<<endl;
	if (lazytool) curtool->primary=0; else curtool->primary=1;
	viewport->Push(tools.e[ti],-1,0); // this also calls the InterfaceOn
	if (mesbar) mesbar->SetText(curtool->Name());
	return 0;
}

//! Just calls viewportwindow->Push(i,local)
/*! This is meant for interfaces that are not exactly tools, such as global shortcuts.
 * The interface is not put on the tools stack.
 */
int ViewerWindow::PushInterface(anInterface *i,int absorbcount)
{
	return viewport->Push(i,-1,absorbcount);
}

/*! Remove interface from viewport's interfaces stack.
 */
int ViewerWindow::PopInterface(anInterface *i)
{
	viewport->Pop(i,1); //the 1 means dec_count on the popped object
	if (i == curtool) curtool = nullptr;
	return 0;
}

Laxkit::ShortcutHandler *ViewerWindow::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(VIEWER_NextTool,     't',0,0,           _("NextTool"),     _("Next tool"),NULL,0);
	sc->Add(VIEWER_PreviousTool, 'T',ShiftMask,0,   _("PreviousTool"), _("Previous tool"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

int ViewerWindow::PerformAction(int action)
{
	DBG cerr <<"ViewerWindow::PerformAction("<<action<<")"<<endl;
	if (action==VIEWER_NextTool || action==VIEWER_PreviousTool) {
		SelectTool(action==VIEWER_NextTool?-1:-2);
		return 0;
	}
	return 1; //***
}

//! Key presses.
/*!
 * \todo what about: +^'t'    toggle lazy tool (whether the tool should stay same if click on object of another type)
 */
int ViewerWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	DBG cerr <<"ViewerWindow::CharInput: "<<ch<<endl;

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
	    return PerformAction(action);
	}

	return anXWindow::CharInput(ch,buffer,len,state,d);
}

} // namespace LaxInterfaces


