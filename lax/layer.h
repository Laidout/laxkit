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
//    Copyright (C) 2004-2010 by Tom Lechner
//
#ifndef _LAX_LAYER_H
#define _LAX_LAYER_H

#include <lax/anxapp.h>

namespace Laxkit {

class LayerPicker : public anXWindow
{
  protected:
	int nperwidth,first,drawme;
	int numwithinfo, *ninfo,*whichwithinfo;
	int onr,ong,onb,br,bg,bb;
	int w,h;
  public:
	LayerPicker(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder,int npw);
	int init();
	void DrawInfoButton(int x,int y,int i);
	void DrawButton(int x,int y,int ifn,int ifon);
	virtual void DrawNumber(int n);
	virtual void Setnperwidth(int npw);
	virtual void SetNumber(int n,int i);
	virtual int WhatsNumber(int n);
	virtual void Refresh();
	virtual int LBDown(int x,int y, unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
};

} // namespace Laxkit

#endif

