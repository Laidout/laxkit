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
#ifndef _LAX_TABFRAME_H
#define _LAX_TABFRAME_H


#include <lax/iconselector.h>

namespace Laxkit {

//------------------------------ TabBox --------------------------------
class TabBox : public IconBox
{
 public:
	anXWindow *win;
	int winislocal; // *** just assume not? no to do floatables??-- what does that mean??*** assuming not local
	TabBox() { win=NULL; }
	TabBox(anXWindow *nwin,const char *nlabel,LaxImage *img,int nid);
	virtual ~TabBox();
	//virtual void sync();
};

//------------------------------ TabFrame --------------------------------

enum TabFrameStyle {
	TabFrame_Top    =(1<<16),
	TabFrame_Bottom =(1<<17),
	TabFrame_Right  =(1<<18),
	TabFrame_Left   =(1<<19),
	TabFrame_Stretch=(1<<20),
	TabFrame_Pile   =(1<<21)
};

class TabFrame : public IconSelector
{
 protected:
	int curtab;
	virtual int mapWindow(int which,int mapit=1); //mapit=1
	virtual void DrawTab(IconBox *b, int selected, int iscurbox);
	virtual void drawbox(int which);
	virtual void FillBox(IconBox *b,const char *nlabel,LaxImage *img, int nid);

 public:
	TabFrame(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
                      int xx,int yy,int ww,int hh,int brder,
                      anXWindow *prev,unsigned long nowner,const char *nsendmes,
                      int npad=0,int npadg=5);
	virtual ~TabFrame();
	virtual const char *whattype() { return "TabFrame"; }
	virtual int init();
	virtual void Refresh();

	virtual int AddWin(anXWindow *nwin,int absorbcount,const char *nlabel,const char *iconfilename, int makebw);
	virtual int SelectN(int which);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
};

} // namespace Laxkit

#endif

