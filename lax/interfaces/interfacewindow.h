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
#ifndef _LAX_INTERFACEWINDOW_H
#define _LAX_INTERFACEWINDOW_H


//#include <lax/anxapp.h>
#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces {

	

class InterfaceWindow : public Laxkit::anXWindow
{
  private:
	char *_whattype;

  protected:
	anInterface *interface;
	Laxkit::DoubleBBox rect;
	double padouter;

  public:
	InterfaceWindow(Laxkit::anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						Laxkit::anXWindow *prev, unsigned long nowner, const char *nsend,
						anInterface *theinterface, int absorb);
	virtual ~InterfaceWindow();
	virtual const char *whattype();
	virtual int init();
	virtual void Refresh();
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *kb);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb);
	
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int Event(const Laxkit::EventData *e, const char *mes);

	// serializing aids
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att, int what, Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att, int flag, Laxkit::DumpContext *context);

	// Interface specific functions:
	virtual void SetupRect(); // the rect that the interface should operate in
	virtual int SetInterface(anInterface *newinterface);
	virtual anInterface *GetInterface() { return interface; }
};


} // namespace LaxInterfaces

#endif



