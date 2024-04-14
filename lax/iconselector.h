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
	int thing;
	bool use_thing;
	bool manual_size;
	int img_w; //sep from w(),h() since those are for displayed box, not possible sizes
	int img_h;
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
  	int tlabelstyle;
  	int display_type; //0 for normal rows or columns, 1 for vertical list with text

  public:
	int labelstyle; //!< One of: LAX_TEXT_ONLY, LAX_ICON_ONLY, LAX_TEXT_ICON, LAX_ICON_TEXT
	int padg;       //!< Pad between text and icon
	int boxinset;   //!< pad inside each box
	double default_boxw; //!< If -1, use extent of pads+icon+text, else use this absolute value.
	double default_boxh; //!< If -1, use extent of pads+icon+text, else use this absolute value.

	IconSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsendmes,
						int npadx=0,int npady=0, int nboxinset=0);
	~IconSelector();
	virtual void drawbox(int which);
	virtual void FillBox(IconBox *b,const char *nlabel,LaxImage *img, int nid);
	virtual void FillBox(IconBox *b,const char *nlabel,const char *filename, int nid);
	virtual int AddBox(const char *nlabel, int nid);
	virtual int AddBox(const char *nlabel, LaxImage *img, int nid);
	virtual int AddBox(const char *nlabel, const char *filename, int nid);
	virtual int send();
	virtual int DisplayAsList(bool yes);
	virtual int DisplayType() { return display_type; }
};


} // namespace Laxkit

#endif

