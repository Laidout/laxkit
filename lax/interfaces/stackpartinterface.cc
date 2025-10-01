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
//    Copyright (C) 2018 by Tom Lechner
//



#include <lax/interfaces/stackpartinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>


#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;


namespace LaxInterfaces {


//--------------------------- StackParts -------------------------------------

/*! \class StackParts
 * \ingroup interfaces
 * \brief Data that StackPartInterface can use.
 */


//---------StackParts::StackNode

/*! Incs count of icon. absorb is only for count on nextra.
 */
StackParts::StackNode::StackNode(const char *ntext, LaxImage *nicon, int nid, anObject *nextra, int absorb)
{
	text = newstr(ntext);
	id = nid;
	extra = nextra;
	icon = nicon;
	if (icon) icon->inc_count();
	if (extra && !absorb) extra->inc_count();
}

StackParts::StackNode::~StackNode()
{
	delete[] text;
	if (extra) extra->dec_count();
	if (icon) icon->dec_count();
}



//---------StackParts

StackParts::StackParts()
{
	style = 0;
	current = 0;
	delimiter = NULL;
	font = NULL;
	padding = .25; //fraction of text height
	display_type = 0;
	display_gravity = LAX_TOP | LAX_LEFT;
	mo_diff = .05;
	cur_diff = .05;
}

StackParts::~StackParts()
{
	delete[] delimiter;
	if (font) font->dec_count();
}

int StackParts::n()
{
	return stack.n;
}

StackParts::StackNode *StackParts::e(int which)
{
	if (which>=0 && which<stack.n) return stack.e[which];
	return NULL;
}

/*! Set from a string like "1/2/3".
 */
int StackParts::SetPath(const char *path, const char *delim)
{
	stack.flush();

	if (!delim) delim = "/";

	int c=0;
	const char *str = path;
	const char *part;

	while (str) {
		part = strstr(str, delim);
		if (part) {
			char *ss = newnstr(str, part-str);
			Add(ss, NULL, c, NULL,0, -1);
			c++;
			delete[] ss;
		}
		str = part + strlen(delim);
	}

	return 0;
}

/*! Return a concatenation up through current.
 */
char *StackParts::GetPath(const char *delim)
{
	char *path = NULL;
	for (int c=0; c <= current; c++) {
		appendstr(path, stack.e[c]->text);
		if (c != stack.n-1) appendstr(path, delim);
	}
	return path;
}

int StackParts::SetPathPart(const char *text, int which)
{
	StackNode *part = e(which);
	if (!part) return -1;

	makestr(part->text, text);
	return 0;
}

int StackParts::Add(const char *ntext, Laxkit::LaxImage *nicon, int nid, Laxkit::anObject *nextra, int absorb, int where)
{
	return stack.push(new StackNode(ntext, nicon, nid, nextra, absorb), 1, where);
}

int StackParts::Remove(int index)
{
	return stack.remove(index);
}

int StackParts::Current(int which)
{
	current = which;
	if (current > stack.n) current = -1;
	return current;
}

StackParts::StackNode *StackParts::Current()
{
	if (current>=0 && current < stack.n) return stack.e[current];
	return NULL;
}



//--------------------------- StackPartInterface -------------------------------------

/*! \class StackPartInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


StackPartInterface::StackPartInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_flags=0;
	interface_type = INTERFACE_Overlay;

	showdecs=1;
	needtodraw=1;

	data=NULL;

	sc=NULL; //shortcut list, define as needed in GetShortcuts()
}

StackPartInterface::~StackPartInterface()
{
	if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
}

const char *StackPartInterface::whatdatatype()
{ 
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *StackPartInterface::Name()
{ return _("Stack Selector"); }


//! Return new StackPartInterface.
/*! If dup!=NULL and it cannot be cast to StackPartInterface, then return NULL.
 */
anInterface *StackPartInterface::duplicateInterface(anInterface *dup)
{
	if (dup==NULL) dup=new StackPartInterface(NULL,id,NULL);
	else if (!dynamic_cast<StackPartInterface *>(dup)) return NULL;
	return anInterface::duplicateInterface(dup);
}

////! Use the object at oc if it is an StackParts.
//int StackPartInterface::UseThisObject(ObjectContext *oc)
//{
//	return 0;
//}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int StackPartInterface::UseThis(anObject *nobj, unsigned int mask)
{
//	if (!nobj) return 1;
//	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
//	if (ls!=NULL) {
//		if (mask & (LINESTYLE_Color | LINESTYLE_Color2)) { 
//			linecolor=ls->color;
//		}
////		if (mask & LINESTYLE_Width) {
////			linecolor.width=ls->width;
////		}
//		needtodraw=1;
//		return 1;
//	}
	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *StackPartInterface::Context()
{ return NULL; }

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int StackPartInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int StackPartInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void StackPartInterface::Clear(SomeData *d)
{
	if (data) { data->dec_count(); data=NULL; }
}

void StackPartInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

Laxkit::MenuInfo *StackPartInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{ return NULL; }

int StackPartInterface::Event(const Laxkit::EventData *data, const char *mes)
{ return 1; }


/*! Draw some data other than the current data.
 * This is called during screen refreshes.
 */
int StackPartInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<StackParts *>(ndata)==NULL) return 1;

	StackParts *bzd = data;
	data = dynamic_cast<StackParts *>(ndata);

	 // store any other state we need to remember
	 // and update to draw just the temporary object, no decorations
	int td = showdecs, ntd = needtodraw;
	showdecs = 0;
	needtodraw = 1;

	Refresh();

	 //now restore the old state
	needtodraw = ntd;
	showdecs = td;
	data = bzd;
	return 1;
}


int StackPartInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;


	 //draw some text name
	dp->DrawScreen();
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

	dp->NewFG(curwindow->win_themestyle->fg);
	dp->NewBG(curwindow->win_themestyle->fg);

	StackParts::StackNode *node;
	for (int c=0; c<data->n(); c++) {
		node = data->e(c);

		dp->textout(node->x+node->width/2, node->y+node->height/2, node->text,-1, LAX_VCENTER|LAX_LEFT);
	}

	dp->DrawReal();

	return 0;
}

int StackPartInterface::scan(int x, int y, unsigned int state)
{
	if (!data->pointIsIn(x,y)) return -1;

	x -= data->x;
	y -= data->y;

	for (int c=0; c<data->n(); c++) {
		if (data->e(c)->pointIsIn(x,y)) return c;
	}

	return -1;
}

int StackPartInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	int nhover = scan(x,y,state);
	if (nhover >= 0) {
		hover=nhover;
		buttondown.down(d->id,LEFTBUTTON,x,y, nhover);
		needtodraw=1;
	}



	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int StackPartInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int StackPartInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		// update any mouse over state
		int nhover = scan(x,y,state);
		if (nhover != hover) {
			hover = nhover;
			buttondown.down(d->id,LEFTBUTTON,x,y, nhover);

			PostMessage(_("Something based on new hover value"));
			needtodraw=1;
			return 0;
		}
		return 1;
	}

	//else deal with mouse dragging...
	

	//needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}



int StackPartInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"StackPartInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int StackPartInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	 //default shortcut processing

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1; //key not dealt with, propagate to next interface
}

Laxkit::ShortcutHandler *StackPartInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc = new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    //sc->Add(STACKPART_Something,  'B',ShiftMask|ControlMask,0, "BaselineJustify", _("Baseline Justify"),NULL,0);
    //sc->Add(STACKPART_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
    //sc->Add(STACKPART_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
	//sc->Add(STACKPART_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),NULL,0);
	//sc->AddShortcut('=',0,0, STACKPART_Something); //add key to existing action

    manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int StackPartInterface::PerformAction(int action)
{
	if (action == STACK_Rename) {
	}
	return 1;
}

} // namespace LaxInterfaces

