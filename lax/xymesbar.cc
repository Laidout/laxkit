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

#include <cstring>
#include <iostream>
#include <lax/xymesbar.h>

using namespace std;

#include <lax/strmanip.h>

namespace Laxkit {

/*! \class XYMesBar
 * \brief Designed to display 2 numbers: "32,45" or "32x45"
 *
 * This allows printing of any two strings or numbers with optional
 * initial (prestuff), in-between (middlestuff), and ending text (poststuff), 
 * with optional units (xunits,yunits). The values for x and y are
 * stored with strings, not numbers, but you can set as either int, double,
 * or character strings.
 *
 * 
 * The following styles are in addition to the MessageBar styles:
 * \code
 *  // these can be passed in via formathints in constructor
 * #define XYMB_COMMA		(1)   <-- put a comma between the values
 * #define XYMB_X			(2)   <-- put an 'x' between the values
 * 
 *  //these go in win_style
 * #define XYMB_BOTHUNITS	(1<<28)   <-- "3in x 4in"
 * #define XYMB_UNITS		(1<<29)   <-- "3 x 4 in"
 * #define XYMB_TRAILZEROS	(1<<30)   <-- **** not implemented
 * #define XYMB_SCIENTIFIC	(1<<31)   <-- **** not implemented
 * 	// for big numbers: scientific=1x10^2, default is 1e+2
 * \endcode
 */


XYMesBar::XYMesBar(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nwstyle,
					int nx,int ny,int nw,int nh,int brder,
					int initialx,int initialy,int formathints) 
	: MessageBar(pwindow,nname,ntitle,nwstyle, nx,ny, nw,nh,brder,NULL) 
{ 
	prestuff=middlestuff=poststuff=xunits=yunits=NULL;
	makestr(prestuff,"");
	makestr(poststuff,"");
	makestr(xunits,"");
	makestr(yunits,"");
	if (formathints==XYMB_COMMA) makestr(middlestuff,",");
	else if (formathints==XYMB_X) makestr(middlestuff,"x");
	else makestr(middlestuff," ");
	precision=9;
	curx=cury=NULL;
	SetXY(initialx,initialy);
}

XYMesBar::~XYMesBar()
{
	delete[] curx;
	delete[] cury;
	delete[] prestuff;
	delete[] middlestuff;
	delete[] poststuff;
	delete[] xunits;
	delete[] yunits;
}

//! Set the format of the display. 
void XYMesBar::SetFormat(const char *pres, //!< Set the initial text
						const char *mids,  //!< Set the middle text
						const char *posts, //!< Set the ending text
						const char *xun,   //!< Set the xunits
						const char *yun    //!< Set the yunits
						)
{								// xun=yun=NULL
	makestr(prestuff,pres);
	makestr(middlestuff,mids);
	makestr(poststuff,posts);
	makestr(xunits,(xun?xun:""));
	makestr(yunits,(yun?yun:""));
	SetXY(curx,cury);
}

//! Set the x and y values to the strings.
void XYMesBar::SetXY(const char *xbit,const char *ybit)
{
	char *temp=new char[strlen(prestuff)
						+strlen(xbit)
						+strlen(middlestuff)
						+strlen(ybit)
						+strlen(poststuff)
						+strlen(xunits)
						+strlen(yunits)
						+1];
	if (win_style&XYMB_BOTHUNITS) sprintf(temp,"%s%s%s%s%s%s%s",prestuff,xbit,xunits,middlestuff,ybit,yunits,poststuff);
	else if (win_style&XYMB_UNITS) sprintf(temp,"%s%s%s%s%s%s",prestuff,xbit,middlestuff,ybit,xunits,poststuff);
	else sprintf(temp,"%s%s%s%s%s",prestuff,xbit,middlestuff,ybit,poststuff);
	SetText(temp);
	delete[] temp;
	makestr(curx,xbit);
	makestr(cury,ybit);
	needtodraw|=1;
}

//! Set the x and y values to the given integers.
void XYMesBar::SetXY(int x,int y)
{	
	char *xbit=numtostr(x),
		 *ybit=numtostr(y);
	SetXY(xbit,ybit);
	delete[] xbit;
	delete[] ybit;
}

//! Set the x and y values to the give doubles.
/*! TODO: put in precision and scientific */
void XYMesBar::SetXY(double x,double y)
{
	char *xbit=numtostr(x),  //zeros|10^blah|e+-blah)
		 *ybit=numtostr(y); // definable exponent string: "e" "x10^" " x 10^"
	//*** pull down to precision, numtostr uses 13 precision
	
	SetXY(xbit,ybit);
	delete[] xbit;
	delete[] ybit;
}

//! Set how many decimal places to write in double values.
int XYMesBar::SetPrecision(int p)
{
	if (p==precision || p<0 || p>20) return precision;
	int oldp=precision;
	precision=p;
	SetXY(curx,cury);
	return oldp;
}

} // namespace Laxkit

