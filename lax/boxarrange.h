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
#ifndef _LAX_BOXARRANGE_H
#define _LAX_BOXARRANGE_H


#ifndef NULL
#define NULL 0
#endif

//#define LAX_LISTS_H_ONLY
#include <lax/lists.h>
//#undef  LAX_LISTS_H_ONLY


#define BOX_SHOULD_WRAP   100000
#define BOX_NO_BREAK      1000000
#define BOX_MUST_BREAK    -1000000

 // For laying out, for instance LRTB == Left to Right, Top to Bottom
 // Note that only LRTB and TBLR are implemented in ArrangeRowCol().
 // These correspond to LAX_LRTB, etc. note that they take up 
 // only the leftmost 3 bits of flags.
#define BOX_LRTB      (0)
#define BOX_LRBT      (1)
#define BOX_RLTB      (2)
#define BOX_RLBT      (3)
#define BOX_TBLR      (4)
#define BOX_TBRL      (5)
#define BOX_BTLR      (6)
#define BOX_BTRL      (7)
#define BOX_FLOW_MASK (0x7)

#define BOX_HORIZONTAL         (1<<3)
#define BOX_VERTICAL           (1<<4)
#define BOX_WRAP_TO_X_EXTENT   (1<<5)
#define BOX_WRAP_TO_Y_EXTENT   (1<<6)
#define BOX_WRAP_TO_EXTENT     (3<<5)

 // if laying out x, make y,h value sync with target->h
#define BOX_SET_Y_TOO   (1<<7)
 // if laying out y, make x,w value sync with target->w
#define BOX_SET_X_TOO   (1<<8)
#define BOX_SET_METRICS (1<<7|1<<8)
        
 // |11223344| <-- with any of the left/center/right/justify, always acts as justify
#define BOX_STRETCH_TO_FILL_X  (1<<9)
#define BOX_STRETCH_TO_FILL_Y  (1<<10)
#define BOX_STRETCH_MASK       (1<<9|1<<10)

 // Space with left/right/center/justify
 // |1 2 3 4  |  
 // | 1 2 3 4 |  
 // |  1 2 3 4| 
 // |1 2  3  4|
#define BOX_SPACE_TO_FILL_X    (1<<11)
#define BOX_SPACE_TO_FILL_Y    (1<<12)
#define BOX_SPACE_JUSTIFY      (1<<13)
#define BOX_SPACE_MASK         (1<<11|1<<12|1<<13)

 // overall alignment of the laid out boxes
 //        |1 2 3 4  |
 //        | 1 2 3 4 |
 //        |  1 2 3 4|
#define BOX_ALIGN_MASK         (63<<14)
#define BOX_HALIGN_SHIFT       14
#define BOX_HALIGN_MASK        (7<<14)
#define BOX_LEFT               (1<<14)
#define BOX_HCENTER            (1<<15)    
#define BOX_RIGHT              (1<<16)

#define BOX_VALIGN_SHIFT       17
#define BOX_VALIGN_MASK        (7<<17)
#define BOX_TOP                (1<<17)
#define BOX_VCENTER            (1<<18)    
#define BOX_BOTTOM             (1<<19)
#define BOX_CENTER             (1<<15|1<<18)    
        
#define BOX_STRETCH_IN_ROW     (1<<20)
#define BOX_STRETCH_IN_COL     (1<<21)

 // Windows (rather than boxes) set positions according to their own origin,
 // so don't add on the x and y positions.
#define BOX_DONT_PROPAGATE_POS (1<<22)

 //whether to use this box in calculations or not
#define BOX_HIDDEN             (1<<23)


namespace Laxkit {


	
//----------------------------- SquishyBox -------------------------------

class SquishyBox 
{
 public:
	int m[14]; // metrics: x,w,pw,ws,wg,halign,hgap, y,h,ph,hs,hg,valign,vgap
	int pad; // the bevel would go around this box, and should be added to the width in width calculations.
	int padinset; // additional pad inside a box. "pad" is outside the box
	int fpen,lpen; //flow penalty, line penalty
	unsigned long flags; // LAX_LRTB, BOX_CENTER, for instance <-- these refer to arrangement of children
	SquishyBox();
	SquishyBox(unsigned int nflags,
				int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
				int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap);
	virtual ~SquishyBox() {}
	
	virtual int hideBox(int yeshide);
	virtual int hidden();

	virtual int WrapToExtent();
	virtual void sync(int xx,int yy,int ww,int hh);
	virtual void sync();
	
	 //sizing functions
	virtual int x()      { return m[0]; }
	virtual int w()      { return m[1]; }
	virtual int pw()     { return m[2]; }
	virtual int ws()     { return m[3]; }
	virtual int wg()     { return m[4]; }
	virtual int halign() { return m[5]; }
	virtual int hgap()   { return m[6]; }

	virtual int y()      { return m[7]; }
	virtual int h()      { return m[8]; }
	virtual int ph()     { return m[9]; }
	virtual int hs()     { return m[10]; }
	virtual int hg()     { return m[11]; }
	virtual int valign() { return m[12]; }
	virtual int vgap()   { return m[13]; }

	virtual int x(int val)      { return m[0]=val; } 
	virtual int w(int val)      { return m[1]=val; }
	virtual int pw(int val)     { return m[2]=val; }
	virtual int ws(int val)     { return m[3]=val; }
	virtual int wg(int val)     { return m[4]=val; }
	virtual int halign(int val) { return m[5]=val; }
	virtual int hgap(int val)   { return m[6]=val; }

	virtual int y(int val)      { return m[7]=val; }
	virtual int h(int val)      { return m[8]=val; }
	virtual int ph(int val)     { return m[9]=val; }
	virtual int hs(int val)     { return m[10]=val; }
	virtual int hg(int val)     { return m[11]=val; }
	virtual int valign(int val) { return m[12]=val; }
	virtual int vgap(int val)   { return m[13]=val; }

	virtual int fpenalty() { return fpen; } //0=nothing, <0 force break, >0 never break
	virtual int fpenalty(int val) { return fpen=val; }
	virtual int lpenalty() { return lpen; }
	virtual int lpenalty(int val) { return lpen=val; }
};

//----------------------------- ListBox ------------------------------
class ListBox : public SquishyBox
{
 public:
	PtrStack<SquishyBox> list;
	
	ListBox(unsigned int flag=0);//BOX_VERTICAL or BOX_HORIZONTAL
	ListBox(unsigned int nflags,
			int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
			int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap);

	 //list management functions
	virtual void Push(SquishyBox *box,char islocal=0,int where=-1);
	virtual void AddSpacer(int npw,int nws,int nwg,int nhalign, int where=-1);
	virtual int Pop(int which=-1);
	virtual void Flush(); 

	 //syncing and distributing
	virtual int WrapToExtent();
	virtual void sync();
	virtual int arrangeBoxes(int distributetoo=0);
	virtual int distributeBoxes(int setmetrics=0);
	virtual int figureDimensions(ListBox *target,int *nextrow=NULL,SquishyBox **boxes=NULL, int n=0,
								double *squishx=NULL,double *squishy=NULL); 
};

//----------------------------- RowColBox ------------------------------

class RowColBox : public ListBox
{
 protected:
	PtrStack<SquishyBox> wholelist; // the master list of boxes
	int arrangedstate;
	virtual ListBox *newSubBox();
	virtual void filterflags();
 public:
	unsigned int elementflags;
	RowColBox();
	RowColBox(unsigned int nflags,
			  int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
			  int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap);
	virtual void Push(SquishyBox *box,char islocal=0,int where=-1);
	virtual int Pop(int which=-1);
	virtual void Flush(); 
	
//	virtual int figureDimensions(); // finds and sets the pw,s,g, ph,s,g based on the children
	virtual int arrangeBoxes(int distributetoo=0);
	virtual int distributeBoxes(int setmetrics=0);
};



//-------------------------------------- tables ------------------------

class TableData 
{ 
 public: 
	int i,j,rowspan,colspan; 
	SquishyBox *box; 
};

class TableBox : public SquishyBox
{
 public:
	int i,j; // row i, column j == coordinates of upper left corner
	int rowspan,colspan;
	//int cellpadding; <-- use SquishyBox::pad
};

} // namespace Laxkit

#endif 

