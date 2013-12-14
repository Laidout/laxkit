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

#include <lax/layer.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

/*! \class LayerPicker
 * \brief Pick layers by number.
 *
 * Arranged in 2 rows. The top row represents numbers sequentially.
 * The bottom row represents the second digit, so to speak. Say there are
 * 6 columns, then clicking each of the top buttons activates or deactivates
 * layers 1-6. Clicking the first button of the bottom row will make the
 * top bottoms represent layers 7-12, clicking the second button of the
 * bottom row makes the top row layers 13-18, and so on.
 *
 * \todo redo to use LAX_ON/LAX_OFF, and drawbevel()
 * \todo this is very old code. is this widget really useful at all??
 */



LayerPicker::LayerPicker(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
					int xx,int yy,int ww,int hh,int brder,int npw)
	: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder, NULL,0,NULL)
{
	numwithinfo=0;
	ninfo=whichwithinfo=NULL;
	nperwidth=(npw>0?npw:3);
	first=0;
	onr=200; 
	onb=ong=0;
	br=150; bg=bb=50;
	needtodraw=1;
	
	DBG cerr <<"layer const:"<<nperwidth<<endl;
}

int LayerPicker::init()
{
	w=win_w/nperwidth;
	h=win_h/2;
	return 0;
}

void LayerPicker::Setnperwidth(int npw) 
{
	if (npw<1 || npw==nperwidth) return;
	nperwidth=npw;
	needtodraw=1;
}

int LayerPicker::WhatsNumber(int n) // returns info, 
{
	int c=0;
	while (c<numwithinfo && whichwithinfo[c]>n) c++;
	if (c>=numwithinfo) return 0;
	if (whichwithinfo[c]==n) return ninfo[c];
	return 0;
}

void LayerPicker::SetNumber(int n,int i)
{
	DBG cerr <<" SetNumber:"<<numwithinfo<<' '<<n<<'='<<i<<endl;
	if (numwithinfo==0) {
		ninfo=new int[1];
		ninfo[0]=i;
		whichwithinfo=new int[1];
		whichwithinfo[0]=n;
		numwithinfo=0;
		drawme=0;
		numwithinfo++;
		needtodraw|=2;
		return;
	}
	 // find where n is in list
	int c=0,c2;
	while (c<numwithinfo && whichwithinfo[c]>n) c++;
	if (c<numwithinfo && whichwithinfo[c]==n) { ninfo[c]=i; return; }
	int *tempwwi,*tempni;
	tempwwi=new int[numwithinfo+1];
	tempni=new int[numwithinfo+1];
	for (c2=0; c2<c; c2++) { tempwwi[c2]=whichwithinfo[c2]; tempni[c2]=ninfo[c2]; }
	tempwwi[c]=n;
	tempni[c]=i;
	for (c2=c+1; c2<numwithinfo+1; c2++) { tempwwi[c2]=whichwithinfo[c2-1]; tempni[c2]=ninfo[c2-1]; }
	delete[] whichwithinfo;
	whichwithinfo=tempwwi;
	delete[] ninfo;
	ninfo=tempni;
	numwithinfo++;
	
	if (n>=first && n<first+nperwidth) { if (needtodraw&2) needtodraw|=1; else needtodraw|=2; }
	drawme=n;
}

	// standard appearance for on/off...
void LayerPicker::DrawButton(int x,int y,int ifn,int ifon)
{
	int r,g,b;
	if (ifn) { r=onr; g=ong; b=onb; } else { r=br; g=bg; b=bb; }
	foreground_color(rgbcolor(r,g,b));
	fill_rectangle(this, x,y,w-1,h-1);

	if (ifon) foreground_color(rgbcolor((r+255)/2,(g+255)/2,(b+255)/2));
	else foreground_color(rgbcolor(r*2/3,g*2/3,b*2/3));
	draw_line(this, x+w-1,y,x+w-1,y+h-1);
	draw_line(this, x,y+h-1,x+w-1,y+h-1);

	if (ifon) foreground_color(rgbcolor(r*2/3,g*2/3,b*2/3));
	else foreground_color(rgbcolor((r+255)/2,(g+255)/2,(b+255)/2));
	draw_line(this, x,y,x+w-1,y);
	draw_line(this, x,y,x,y+h-1);
}

void LayerPicker::DrawInfoButton(int x,int y,int i)
{
	if (i&1) DrawButton(x,y,1,1);
	else DrawButton(x,y,1,0);
}

void LayerPicker::DrawNumber(int n) // sees if n has info, calls DrawInfoButton for actual drawing
{
	if (n<first || n>=first+nperwidth) return;
	int c=0,x,y;
	x=(n-first)*w;
	y=0;
	while (c<numwithinfo && whichwithinfo[c]>n) c++;
	if (c<numwithinfo && whichwithinfo[c]==n) DrawInfoButton(x,y,ninfo[c]);
	else DrawButton(x,y,1,0);
}

void LayerPicker::Refresh()
{
	if (!win_on || !needtodraw) return;
	if (needtodraw&1) {
			// draw all on first row (info indicators), then second row (bank indicators)
		int c;
		for (c=first; c<first+nperwidth; c++) {
			DrawNumber(c);
			if (c/nperwidth==c-first) DrawButton((c-first)*win_w/nperwidth,win_h/2,0,1);
			else DrawButton((c-first)*win_w/nperwidth,win_h/2,0,0);
		}
		for (int c=0; c<numwithinfo; c++) DrawNumber(whichwithinfo[c]);
	}
	if ((needtodraw&3)==2) DrawNumber(drawme);
	needtodraw=0;
}

int LayerPicker::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	//DBG cerr <<"LayerLBd: "<<x<<','<<y;
	x/=w;
	y/=win_h/2;
	if (y==1) {
		first=x*nperwidth;
	} else if (y==0) {
		int c=0;
		DBG cerr <<"\nnumwithinfo="<<numwithinfo;
		while (c<numwithinfo && whichwithinfo[c]>first+x) c++;
		
		DBG cerr <<"  testing c="<<c;
		DBG if (c<numwithinfo && whichwithinfo[c]==first+x) cerr <<" before:"<<ninfo[c];
		DBG else cerr <<" new-info ";
		
		if (c<numwithinfo && whichwithinfo[c]==first+x) {
			ninfo[c]^=1;
		}
		else SetNumber(first+x,1);
		DBG cerr <<" after:"<<ninfo[c]<<" ";
	}
	needtodraw=1;	
	DBG cerr <<endl;
	return 0;
}

int LayerPicker::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	x/=w;
	y/=win_h/2;
	//DBG cerr <<"\nLayerM: "<<x<<','<<y;
	return 0;
}

} // namespace Laxkit

