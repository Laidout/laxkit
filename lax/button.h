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

	void GetPlacement(double *w,double *h,double *tx,double *ty,double *ix,double *iy, double *iw,double *ih);

  public:	
	int pad,gap,labelstyle;
	enum IconSizeType {
		Default,
		Relative_To_Font,
		Absolute_Pixels,
		Image_pixels
	};

	static IconSizeType default_icon_size_type;
	static double default_icon_height;

	IconSizeType icon_size_type;
	double icon_height;

	Button(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int nid=-1,
						const char *nlabel=NULL,
						const char *filename=NULL,LaxImage *img=NULL,
						int npad=-1,int ngap=-1);
	virtual ~Button();
	virtual const char *whattype() { return "Button"; }
	virtual void ThemeChanged();
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
	virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
	virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
};

} // namespace Laxkit

#endif

