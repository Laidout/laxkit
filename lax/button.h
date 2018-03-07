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
#ifndef _LAX_BUTTON_H
#define _LAX_BUTTON_H


#include <lax/buttonbase.h>
#include <lax/laximages.h>

namespace Laxkit {

#define IBUT_ICON_ONLY  (1<<18)
#define IBUT_TEXT_ONLY  (1<<19)
#define IBUT_TEXT_ICON  (1<<20)
#define IBUT_ICON_TEXT  (1<<21)
#define IBUT_FLAT       (1<<22)

class Button : public ButtonBase
{ 
  protected:
	LaxImage *image,*bwimage;
	int thing,thingw,thingh;
	char *label;
	LaxFont *font;

  public:	
	int pad,gap,labelstyle;

	Button(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int nid=-1,
						const char *nlabel=NULL,
						const char *filename=NULL,LaxImage *img=NULL,
						int npad=-1,int ngap=-1);
	virtual ~Button();
	virtual const char *whattype() { return "Button"; }
	virtual int SetGraphic(int newthing, int newwidth, int newheight);
	virtual int SetIcon(const char *filename,int makebw=0);
	virtual int SetIcon(LaxImage *img,int makebw=0);
	virtual const char *Label(unsigned int which);
	virtual const char *Label(const char *nlabel);
	virtual const char *Label() { return label; }
	virtual int Font(LaxFont *nfont);
	virtual void draw();
	virtual void WrapToExtent(int which);

	 //serializing aids
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
};

} // namespace Laxkit

#endif

