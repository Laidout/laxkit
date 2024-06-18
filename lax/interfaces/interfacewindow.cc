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
//    Copyright (C) 2024 by Tom Lechner
//

#include <lax/interfaces/interfacewindow.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>

#include <lax/debug.h>

using namespace Laxkit;


namespace LaxInterfaces {


//-------------------------------- InterfaceWindow ----------------------------------
/*! \class InterfaceWindow
 * Window based on top of a single interface, that has no SomeData needs.
 * Also, there is no interface stack, so the interface must manage its own
 * subinterfaces, if any.
 */

InterfaceWindow::InterfaceWindow(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder, 
						anXWindow *prev, unsigned long nowner, const char *nsend,
						anInterface *theinterface, int absorb)
				: anXWindow(pwindow,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER, nx,ny, nw,nh,brder, prev,nowner,nsend) 
{
	_whattype = nullptr;
	interface = nullptr;
	padouter = -1;

	InstallColors(THEME_Panel);

	if (theinterface) {
		interface = theinterface;
		interface->CurrentWindow(this);
		if (!absorb) interface->inc_count();
	}
}

InterfaceWindow::~InterfaceWindow()
{
	delete [] _whattype;
	if (interface) interface->dec_count();
}

int InterfaceWindow::init()
{
	if (interface) {
		Displayer *dp = MakeCurrent();
		interface->Dp(dp);
	}
	SetupRect();
	return anXWindow::init();
}

/*! Override to return the interface's whattype() instead.
 */
const char *InterfaceWindow::whattype()
{
	if (!_whattype) {
		if (!interface) return "InterfaceWindow";
		_whattype = newstr(interface->whattype());
		char *i = strstr(_whattype, "Interface");
		if (i) *i = '\0';
		appendstr(_whattype,"Window");
	}

	return _whattype;
}

//! Set values in rect to be reasonable.
void InterfaceWindow::SetupRect()
{
	if (padouter < 0) padouter = win_themestyle->normal->textheight()/3;
	// rect.x = rect.y = padouter;
	// rect.width  = win_w - 2 * padouter;
	// rect.height = win_h - 2 * padouter;
	rect.minx = rect.miny = padouter;
	rect.maxx = win_w - padouter;
	rect.maxy = win_h - padouter;

	if (interface) {
		interface->bounds = rect;
		interface->ViewportResized();
		needtodraw = 1;
	}
}


/*! Absorbs count of new interface-> */
int InterfaceWindow::SetInterface(anInterface *new_interface)
{
	if (new_interface == interface) interface->dec_count();
	else {
		delete[] _whattype;
		_whattype = nullptr;
		if (interface) interface->dec_count();
		interface = new_interface;
	}
	return 0;
}


void InterfaceWindow::Refresh()
{
	if (!interface) { needtodraw = 0; return; }
	if (!win_on || !interface) return;
	if (!(needtodraw || interface->needtodraw)) return;
	
	Displayer *dp = MakeCurrent();
	dp->ClearWindow();

	interface->Needtodraw(1);
	interface->Dp(dp);
	interface->Refresh();

	SwapBuffers();
	needtodraw = 0;
	return;
}


int InterfaceWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (interface) {
		int status = interface->LBDown(x,y,state,count,d);
		needtodraw |= interface->needtodraw;
		if (status == 0) return 0;
	}
	return anXWindow::LBDown(x,y,state,count,d);
}

int InterfaceWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->LBUp(x,y,state,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->MBDown(x,y,state,count,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::MBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->MBUp(x,y,state,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->RBDown(x,y,state,count,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->RBUp(x,y,state,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->MouseMove(x,y,state,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->WheelUp(x,y,state,count,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (!interface) return 1;
	int status = interface->WheelDown(x,y,state,count,d);
	needtodraw |= interface->needtodraw;
	return status;
}

int InterfaceWindow::CharInput(unsigned int ch, const char *buffer, int len, unsigned int state, const LaxKeyboard *kb)
{
	if (interface) {
		int status = interface->CharInput(ch, buffer, len, state, kb);
		needtodraw |= interface->needtodraw;
		if (status == 0) return 0;
	}
	return anXWindow::CharInput(ch,buffer,len,state,kb);
}

int InterfaceWindow::KeyUp(unsigned int ch, unsigned int state, const LaxKeyboard *kb)
{
	if (interface) {
		int status = interface->KeyUp(ch, state, kb);
		needtodraw |= interface->needtodraw;
		if (status == 0) return 0;
	}
	return anXWindow::KeyUp(ch,state,kb);
}


int InterfaceWindow::Event(const EventData *data,const char *mes)
{
	if (interface) {
		DBGM("ev: "<<(int)data->type);
		if (data->type == LAX_onMouseIn) {
			interface->MouseIn();
			needtodraw |= interface->needtodraw;
		} else if (data->type == LAX_onMouseOut) {
			interface->MouseOut();
			needtodraw |= interface->needtodraw;
		}
	}
	return anXWindow::Event(data, mes);
}


/*! Append to att if att!=nullptr, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *InterfaceWindow::dump_out_atts(Attribute *att, int what, DumpContext *context)
{
	if (!att) att=new Attribute(whattype(),nullptr);
	anXWindow::dump_out_atts(att,what,context);
	if (interface) {
		Attribute *a = att->pushSubAtt("interface", whattype());
		interface->dump_out_atts(a,what,context);
	}
	return att;
}

void InterfaceWindow::dump_in_atts(Laxkit::Attribute *att, int flag, DumpContext *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	const char *name;
	// const char *value;

	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		// value = att->attributes.e[c]->value;

		if (!strcmp(name,"interface")) {
			interface->dump_in_atts(att->attributes.e[c], flag, context);
		}
	}
}

int InterfaceWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	SetupRect();
	return 0;
}

int InterfaceWindow::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	SetupRect();
	return 0;
}


} // namespace LaxInterfaces

