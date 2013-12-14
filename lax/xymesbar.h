//
//	
//    The Laxkit, a windowing toolkit
//    Copyright (C) 2004-2006 by Tom Lechner
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
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
#ifndef _LAX_XYMESBAR_H
#define _LAX_XYMESBAR_H

#include <lax/messagebar.h>

namespace Laxkit {

#define XYMB_COMMA		(1)
#define XYMB_X			(2)

#define XYMB_BOTHUNITS	(1<<28)
#define XYMB_UNITS		(1<<29)
#define XYMB_TRAILZEROS	(1<<30)
#define XYMB_SCIENTIFIC	(1<<31)

class XYMesBar : public MessageBar
{
  protected:
  	char *prestuff,*middlestuff,*poststuff,*xunits,*yunits;
	char *curx,*cury;
	int precision;
  public:
	XYMesBar(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nwstyle,
			 int nx,int ny,int nw,int nh,int brder,
			 int initialx,int initialy,int formathints);
	virtual ~XYMesBar();
	virtual void SetFormat(const char *pres,const char *mids,const char *posts,const char *xun=NULL,const char *yun=NULL);
	virtual void SetXY(const char *x,const char *y);
	virtual void SetXY(int x,int y);
	virtual void SetXY(double x,double y);
	virtual int SetPrecision(int p);	
	virtual const char *whattype() { return "XYMesBar"; }
};

} // namespace Laxkit

#endif

