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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_ICONSELECTOR_H
#define _LAX_ICONSELECTOR_H


#define STRICON_STR_ICON   (1<<31)



#include <lax/boxselector.h>
#include <lax/laximages.h>

namespace Laxkit {


//------------------------------- IconBox --------------------------------
class IconBox : public SelBox
{
  public:
	char *label;
	LaxImage *image;
	LaxImage *bwimage;

	IconBox(const char *nlabel,LaxImage *img,int nid);
	IconBox() { image=bwimage=NULL; label=NULL; state=LAX_OFF; }
	virtual ~IconBox();
	virtual int SetBox(const char *nlabel,LaxImage *img,LaxImage *bw);
};


//------------------------------- IconSelector --------------------------------
class IconSelector : public BoxSelector
{ 
  protected:
  public:
	int padg,labelstyle;
	IconSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int npadx=0,int npady=0);
	~IconSelector();
	virtual void drawbox(int which);
	virtual void FillBox(IconBox *b,const char *nlabel,LaxImage *img, int nid);
	virtual void FillBox(IconBox *b,const char *nlabel,const char *filename, int nid);
	virtual int AddBox(const char *nlabel,LaxImage *img,int nid);
	virtual int AddBox(const char *nlabel,const char *filename,int nid);
};


} // namespace Laxkit

#endif

