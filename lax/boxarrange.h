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
#ifndef _LAX_BOXARRANGE_H
#define _LAX_BOXARRANGE_H


#include <lax/refptrstack.h>


namespace Laxkit {


// These three are special flags, meaningless numbers that users should not use
// as actual numbers in SquishyBox::m. They are to only be used as flags.
#define BOX_SHOULD_WRAP   100000
#define BOX_NO_BREAK      1000000
#define BOX_MUST_BREAK    -1000000


// The following go in SqishyBox::flags:

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


enum SquishyBoxMetrics {
	BOX_H_pos    = 0,
	BOX_H_len    = 1,
	BOX_H_pref   = 2,
	BOX_H_shrink = 3,
	BOX_H_grow   = 4,
	BOX_H_align  = 5,
	BOX_H_gap    = 6,

	BOX_V_pos    = 7,
	BOX_V_len    = 8,
	BOX_V_pref   = 9,
	BOX_V_shrink = 10,
	BOX_V_grow   = 11,
	BOX_V_align  = 12,
	BOX_V_gap    = 13
};

	
//----------------------------- SquishyBox -------------------------------

class SquishyBox 
{
  public:
	double m[14]; // metrics: x,w,pw,ws,wg,halign,hgap, y,h,ph,hs,hg,valign,vgap
	double pad; // the bevel would go around this box, and should be added to the width in width calculations.
	double padinset; // additional pad inside a box. "pad" is outside the box
	double fpen,lpen; //flow penalty, line penalty
	unsigned long flags; // LAX_LRTB, BOX_CENTER, for instance <-- these refer to arrangement of children
	double last_scale; // ui scale with which current values were computed

	SquishyBox();
	SquishyBox(unsigned int nflags,
				double nx,double nw,double npw,double nws,double nwg,double nhalign,double nhgap,
				double ny,double nh,double nph,double nhs,double nhg,double nvalign,double nvgap);
	virtual ~SquishyBox() {}
	
	virtual int hideBox(int yeshide);
	virtual int hidden();

	virtual int WrapToExtent();
	virtual void sync(int xx,int yy,int ww,int hh);
	virtual void sync();
	virtual void UpdateScale(double new_scale);

	 //sizing functions
	virtual void SetPreferred(double npw,double nws,double nwg,double nhalign,double nhgap, 
							  double nph,double nhs,double nhg,double nvalign,double nvgap);
	virtual double x()      { return m[BOX_H_pos   ]; }
	virtual double w()      { return m[BOX_H_len   ]; }
	virtual double pw()     { return m[BOX_H_pref  ]; }
	virtual double ws()     { return m[BOX_H_shrink]; }
	virtual double wg()     { return m[BOX_H_grow  ]; }
	virtual double halign() { return m[BOX_H_align ]; }
	virtual double hgap()   { return m[BOX_H_gap   ]; }

	virtual double y()      { return m[BOX_V_pos   ]; }
	virtual double h()      { return m[BOX_V_len   ]; }
	virtual double ph()     { return m[BOX_V_pref  ]; }
	virtual double hs()     { return m[BOX_V_shrink]; }
	virtual double hg()     { return m[BOX_V_grow  ]; }
	virtual double valign() { return m[BOX_V_align ]; }
	virtual double vgap()   { return m[BOX_V_gap   ]; }

	virtual double x(int val)      { return m[BOX_H_pos   ]=val; } 
	virtual double w(int val)      { return m[BOX_H_len   ]=val; }
	virtual double pw(int val)     { return m[BOX_H_pref  ]=val; }
	virtual double ws(int val)     { return m[BOX_H_shrink]=val; }
	virtual double wg(int val)     { return m[BOX_H_grow  ]=val; }
	virtual double halign(int val) { return m[BOX_H_align ]=val; } //usually in [0..100]
	virtual double hgap(int val)   { return m[BOX_H_gap   ]=val; }

	virtual double y(int val)      { return m[BOX_V_pos   ]=val; }
	virtual double h(int val)      { return m[BOX_V_len   ]=val; }
	virtual double ph(int val)     { return m[BOX_V_pref  ]=val; }
	virtual double hs(int val)     { return m[BOX_V_shrink]=val; }
	virtual double hg(int val)     { return m[BOX_V_grow  ]=val; }
	virtual double valign(int val) { return m[BOX_V_align ]=val; }
	virtual double vgap(int val)   { return m[BOX_V_gap   ]=val; }

	virtual double fpenalty() { return fpen; } //0=nothing, <0 force break, >0 never break
	virtual double fpenalty(int val) { return fpen=val; }
	virtual double lpenalty() { return lpen; }
	virtual double lpenalty(int val) { return lpen=val; }
};


//----------------------------- ListBox ------------------------------
class ListBox : public SquishyBox
{
  public:
	RefPtrStack<SquishyBox> list;
	
	ListBox(unsigned int flag=0);//BOX_VERTICAL or BOX_HORIZONTAL
	ListBox(unsigned int nflags,
			double nx,double nw,double npw,double nws,double nwg,double nhalign,double nhgap, 
			double ny,double nh,double nph,double nhs,double nhg,double nvalign,double nvgap);

	// list management functions
	virtual void Push(SquishyBox *box,char islocal=0,int where=-1);
	virtual void AddSpacer(double npw,double nws,double nwg,double nhalign, int where=-1);
	virtual int Remove(int which=-1);
	virtual void Flush();
	virtual SquishyBox *GetBox(int which);
	virtual bool HideSubBox(int index);
	virtual bool ShowSubBox(int index);
	virtual int NumBoxes() { return list.n; }
	virtual int NumVisibleBoxes();

	// syncing and distributing
	virtual void UpdateScale(double new_scale);
	virtual int WrapToExtent();
	virtual void sync();
	virtual int arrangeBoxes(int distributetoo=0);
	virtual int distributeBoxes(int setmetrics=0);
	virtual int figureDimensions(ListBox *target,int *nextrow = nullptr,SquishyBox **boxes = nullptr, int n = 0,
								double *squishx = nullptr, double *squishy = nullptr); 
};


//----------------------------- RowColBox ------------------------------

class RowColBox : public ListBox
{
  protected:
	RefPtrStack<SquishyBox> wholelist; // the master list of boxes
	int arrangedstate; // 0 for list needs updating, 1 for no need to recompute
	virtual ListBox *newSubBox();
	virtual void filterflags();

  public:
	unsigned int elementflags;
	RowColBox();
	RowColBox(unsigned int nflags,
			  double nx,double nw,double npw,double nws,double nwg,double nhalign,double nhgap, 
			  double ny,double nh,double nph,double nhs,double nhg,double nvalign,double nvgap);
	virtual void Push(SquishyBox *box,char islocal=0,int where=-1);
	virtual int Remove(int which=-1);
	virtual SquishyBox *GetBox(int which);
	virtual int NumBoxes() { return wholelist.n; }
	virtual int NumVisibleBoxes();
	virtual void Flush();
	
	virtual void UpdateScale(double new_scale);
//	virtual int figureDimensions(); // finds and sets the pw,s,g, ph,s,g based on the children
	virtual int arrangeBoxes(int distributetoo=0);
	virtual int distributeBoxes(int setmetrics=0);
	virtual void MarkSizeDirty() { arrangedstate = 0; };
};


//-------------------------------------- TableLayout ------------------------

class TableLayout : public ListBox 
{ 
  public: 
	TableLayout();
	virtual ~TableLayout();
	NumStack<int> rowspan, colspan;
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

