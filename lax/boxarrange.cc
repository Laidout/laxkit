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



/*! \file
 *
 * \todo *** implement all the LRTB, ...
 */

#include <lax/refptrstack.cc>

#include <lax/boxarrange.h>

#include <iostream>
using namespace std;
#define DBG 



// these defines are to make indexing the SquishyBox::m[] meaningful
#define pos    0
#define len    1
#define pref   2
#define shrink 3
#define grow   4
#define align  5
#define gap    6
#define vert   7



#include <iostream>
using namespace std;

namespace Laxkit {

/*! \defgroup boxarrangers Box Arrangers
 * 
 * These are functions and classes that can be used to build tables and more
 * complicated nested row and column arrangements. Inside the Laxkit, they are
 * used by RowFrame, the BoxSelector, and TableFrame.
 * 
 * The general setup is a simplification of the box-glue-penalty format of 
 * <a href="http://www.tug.org">TeX</a>, but the glue and box are roughly
 * combined into a SquishyBox class.
 */
	
	
//--------------------------------------------

//--------------------------------- Classes --------------------------------------------------

//----------------------- SquishyBox --------------------------------------------------

/*! \class SquishyBox
 * \ingroup boxarrangers
 * \brief A box with x,y,w,h, prefered w/h, and allowable shrink and expand values for w/h.
 *
 * Usually, you will set the preferred width, shrink, grow, align, and gap, and the box
 * sync() process will automatically compute the x,y, and actual width and height.
 *
 * Say a box's preferred width is 5, and it has a shrink of 3 and a grow of 4, then the
 * squish zone, or the acceptable actual width, of the box is somewhere in the range [2,9] 
 * (from 2 to 9 inclusive).
 * 
 * Also keeps track of preferred per-box vertical and horizontal alignment within any extra
 * space that results when the box must fit in a space that is outside its
 * squish zone. This is particularly meant for 
 * aligning in extra space in a table cell, or when laying out rows valign is used, when laying
 * out columns, halign is used. The typical values for v/halign range 0 to 100. 100 means top
 * align, 0 means bottom, 50 centers, everything else does the proportional thing. A super-duper
 * feature that is not implemented would be to have aligning to a baseline.
 *
 * The base SquishyBox only has metric information. 
 * ListBox is a box, with a list of internal boxes. This list can be either a horizontal
 * or vertical list (flags has BOX_HORIZONTAL (the default) or BOX_VERTICAL), with descriptive options
 * held in flags. More complicated arrangements are made
 * with other derived classes, notably RowColBox and TableBox.
 * 
 * This is a moderately adaptible structure to hold info to lay out windows and icons nicely. 
 * It is simplification of the box-glue-penalty format of 
 * <a href="http://www.tug.org">TeX</a>. Penalties are restricted to nothing special (==0),
 * force line break (<0), and never line break (>0). The glue and box are also roughly combined into this class,
 * such that one would always have box(w1)-(no break)-glue(w2,s,g), and the glue's width, shrink 
 * and grow are always used. In TeX, the glue acts like the space between words, but here, the 
 * boxes all kind of but up against each other. So anyway.
 *
 * The horizontal and vertical metric information is stored in the m array, with the
 * horizontal values first  (x,w,pw,ws,wg,halign,hgap), followed by the vertical values 
 * (y,h,ph,hs,hg,valign,vgap). The penalty is stored in fpen (penalty in the flow direction)
 * and lpen (penalty in the line direction).
 *
 *
 * To sum up:
 * <pre>
 *  m, holds horizontal and vertical metric information
 *  pad  = analogous to a window border: space around the box. say pad==5, and the box's x,y,w,h are
 *         (0,0,10,20), then the box+pad fills a rectangle (x,y,w,h)=(-5,-5,20,30)
 *  padinset = space within the box to inset child boxes
 *  fpen = penalty in the flow direction: 0=nothing, <0 force break, >0 never break
 *  lpen = would be penalty in direction of the next line
 * </pre>
 * 
 * The following all go in SquishyBox::flags:
 * \code
 *   // For laying out, for instance LRTB == Left to Right, Top to Bottom
 *   // Note that only LRTB and TBLR are implemented in ArrangeRowCol().
 *   // These correspond to LAX_LRTB, etc. note that they take up 
 *   // only the leftmost 3 bits of flags.
 *  #define BOX_LRTB      (0)
 *  #define BOX_LRBT      (1)
 *  #define BOX_RLTB      (2)
 *  #define BOX_RLBT      (3)
 *  #define BOX_TBLR      (4)
 *  #define BOX_TBRL      (5)
 *  #define BOX_BTLR      (6)
 *  #define BOX_BTRL      (7)
 *  #define BOX_FLOW_MASK (0x7)
 *  
 *  #define BOX_HORIZONTAL         (1<<3)
 *  #define BOX_VERTICAL           (1<<4)
 *   ***what are these:
 *  #define BOX_WRAP_TO_X_EXTENT   (1<<5)
 *  #define BOX_WRAP_TO_Y_EXTENT   (1<<6)
 *  #define BOX_WRAP_TO_EXTENT     (3<<5)
 *  
 *   // if laying out x, make y,h value sync with target->h
 *  #define BOX_SET_Y_TOO   (1<<7)
 *   // if laying out y, make x,w value sync with target->w
 *  #define BOX_SET_X_TOO   (1<<8)
 *  #define BOX_SET_METRICS (1<<7|1<<8)
 *          
 *   // |11223344| <-- with any of the left/center/right/justify, always acts as justify
 *  #define BOX_STRETCH_TO_FILL_X  (1<<9)
 *  #define BOX_STRETCH_TO_FILL_Y  (1<<10)
 *  #define BOX_STRETCH_MASK       (1<<9|1<<10)
 *  
 *   // Space with left/right/center/justify
 *   // |1 2 3 4  |  
 *   // | 1 2 3 4 |  
 *   // |  1 2 3 4| 
 *   // |1 2  3  4|
 *  #define BOX_SPACE_TO_FILL_X    (1<<11)
 *  #define BOX_SPACE_TO_FILL_Y    (1<<12)
 *  #define BOX_SPACE_JUSTIFY      (1<<13)
 *  #define BOX_SPACE_MASK         (1<<11|1<<12|1<<13)
 *  
 *   // overall alignment of the laid out boxes
 *   //        |1 2 3 4  |
 *   //        | 1 2 3 4 |
 *   //        |  1 2 3 4|
 *  #define BOX_ALIGN_MASK         (63<<14)
 *  #define BOX_HALIGN_SHIFT       14
 *  #define BOX_HALIGN_MASK        (7<<14)
 *  #define BOX_LEFT               (1<<14)
 *  #define BOX_HCENTER            (1<<15)    
 *  #define BOX_RIGHT              (1<<16)
 *  
 *  #define BOX_VALIGN_SHIFT       17
 *  #define BOX_VALIGN_MASK        (7<<17)
 *  #define BOX_TOP                (1<<17)
 *  #define BOX_VCENTER            (1<<18)    
 *  #define BOX_BOTTOM             (1<<19)
 *  #define BOX_CENTER             (1<<15|1<<18)    
 *          
 *  #define BOX_STRETCH_IN_ROW     (1<<20)
 *  #define BOX_STRETCH_IN_COL     (1<<21)
 *  
 *   // Windows (rather than boxes) set positions according to their own origin,
 *   // so don't add on the x and y positions.
 *  #define BOX_DONT_PROPAGATE_POS (1<<22)
 *
 *   //whether to use this box in calculations or not
 *  #define BOX_HIDDEN             (1<<23)
 * \endcode
 */
/*! \var int SquishyBox::pad
 * \brief Pad placed around a box, which can be where a bevel or window border would go.
 *
 * This is ignored within the box, but containing boxes can use it to inset things properly.
 */
/*! \var int SquishyBox::padinset
 * \brief Pad to inset the child boxes.
 *
 * If a box is 100 wide, and padinset=5, then the boxes are put in a
 * space 90 wide.
 */
/*! \var int SquishyBox::fpen
 * \brief The penalty in the flow direction.
 *
 * Currently: 
 * 0 = nothing special.\n
 * >0 = never break a line after this box\n
 * <0 = always break a line after this box
 *
 * Though in the future, this may change to be a truer response to penalties.
 * You can use BOX_NO_BREAK (currently==1000000) and
 * BOX_MUST_BREAK (currently==-1000000) to be safer against such future changes.
 *
 * \todo ***implement me!!
 */
/*! \var int SquishyBox::lpen
 * \brief The penalty perpendicular to the flow direction. 
 *
 * This value is currently ignored in the Laxkit.
 */
 

SquishyBox::SquishyBox()
{ 
	memset(m,0,14*sizeof(int));
	m[align]=m[vert+align]=50; // default is to center boxes
	fpen=lpen=0;
	flags=0;
	pad=padinset=0;
}

//! SquishyBox Constructor.
SquishyBox::SquishyBox(unsigned int nflags, //!< See above for BOX_* flags
					int nx,         //!< New x position
					int nw,         //!< New width
					int npw,        //!< New preferred width
					int nws,        //!< New horizontal shrink
					int nwg,        //!< New horizontal grow
					int nhalign,    //!< New horizontal align, 0=bottom/right, 100=top/left, 50=center, etc.
					int nhgap,      //!< New horizontal gap, ignored at edges
					int ny,         //!< New y position
					int nh,         //!< New height
					int nph,        //!< New preferred height
					int nhs,        //!< New vertical shrink
					int nhg,        //!< New vertical grow
					int nvalign,    //!< New vertical align, 0=bottom/right, 100=top/left, 50=center, etc.
					int nvgap      //!< New vertical gap, ignored at edges
					)
{ 
	m[0]=nx; m[1]=nw; m[2]=npw;  m[3]=nws;  m[4]=nwg;  m[5]=nhalign;  m[6]=nhgap;
	m[7]=ny; m[8]=nh; m[9]=nph; m[10]=nhs; m[11]=nhg; m[12]=nvalign; m[13]=nvgap;
	fpen=lpen=0;
	flags=nflags;
	pad=padinset=0;
}

void SquishyBox::SetPreferred(int npw,int nws,int nwg,int nhalign,int nhgap, 
							  int nph,int nhs,int nhg,int nvalign,int nvgap)
{
	m[BOX_H_pref  ] = npw;
	m[BOX_H_shrink] = nws;
	m[BOX_H_grow  ] = nwg;
	m[BOX_H_align ] = nhalign;
	m[BOX_H_gap   ] = nhgap;

	m[BOX_V_pref  ] = nph;
	m[BOX_V_shrink] = nhs;
	m[BOX_V_grow  ] = nhg;
	m[BOX_V_align ] = nvalign;
	m[BOX_V_gap   ] = nvgap;
}

//! hideBox(0) makes the box visible, otherwise, make it hidden. Returns state after call.
/*! \todo must sync or tag needs to sync if necessary...
 */
int SquishyBox::hideBox(int yeshide)
{
	 //***sync if necessary
	if (yeshide) {
		if (!(flags&BOX_HIDDEN)) {
			flags|=BOX_HIDDEN;
			//needtosync=1; //***
		}
	} else {
		if (flags&BOX_HIDDEN) {
			flags&=~BOX_HIDDEN;
			//needtosync=1; //***
		}
	}

	return flags&BOX_HIDDEN; 
}

//! Return whether the box is hidden or not.
int SquishyBox::hidden()
{
	return flags&BOX_HIDDEN; 
}

//! Sets this->pw,s,g/ph,s,g to be the sum of the metrics of the boxes in list.
/*! Note that it does NOT set this->x,y,w,h.
 *
 * Default here is simply to return 0, since plain SquishyBox has no contents.
 * Returns 0 success, nonzero error.
 */
int SquishyBox::WrapToExtent()
{ return 0; }

//! Sync the box to x,y,w,h.
/*! 
 * Currently, this always simply assigns x,y,w,h straight away,
 * then calls sync(). This is just a shortcut to set the x,y,w,h. The end result
 * must always be that it then calls sync().
 *
 * When a window derived from a SquishyBox is resized, it would then call
 * sync(0,0,win_w,win_h).
 */
void SquishyBox::sync(int xx,int yy,int ww,int hh)
{
	x(xx);
	w(ww);
	y(yy);
	h(hh);
	sync();
}
	
//! Sync the box to the already set values of x,y,w,h.
/*! This is meant to be called only when a definite x,y,w,h have already
 *  been assigned for this SquishyBox, such as by its parent box, and *this  
 *  must rearrange its contents according to the new dimensions.
 *
 * Default here is to do nothing, since a plain SquishyBox has no contents.
 */
void SquishyBox::sync()
{}

//-------------------------------------- ListBox ----------------------------------------
/*! \class ListBox
 * \ingroup boxarrangers
 * \brief A SquishyBox containing either a horizontal or vertical list of boxes.
 */


//! Create box as either BOX_VERTICAL or BOX_HORIZONTAL. Other flag values are ignored.
ListBox::ListBox(unsigned int flag)
{
	if (flag&BOX_VERTICAL) flags|=BOX_VERTICAL;
	else if (flag&BOX_HORIZONTAL) flags|=BOX_HORIZONTAL;
}

ListBox::ListBox(unsigned int nflags,
			int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
			int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap)
  : SquishyBox(nflags,nx,nw,npw,nws,nwg,nhalign,nhgap,
		              ny,nh,nph,nhs,nhg,nvalign,nvgap)
{}

//! Remove item with index which from list.
/*! Default is to pop the top of the stack.
 *
 * Returns 1 for item removed, or 0 for which out of bounds.
 */
int ListBox::Pop(int which) //which=-1
{ 
	return list.remove(which); 
}

//! Flush list.
void ListBox::Flush()
{ 
	list.flush(); 
}

//! Push box onto list with islocal. Create list if it doesn't exist.
/*! If box==NULL, then this is later on interpreted as a line break.
 *
 * If islocal==0 then ListBox will not delete the box when Pop() is called.
 * If islocal==1, then do call delete on the box when popped.
 *
 * where is the index to push onto the list stack.
 */
void ListBox::Push(SquishyBox *box,char islocal,int where) // islocal=0,where=-1
{
	list.push(box,islocal,where);
}

void ListBox::AddSpacer(int npw,int nws,int nwg,int nhalign, int where)
{
	SquishyBox *box;
	if (flags&BOX_VERTICAL) box=new SquishyBox(BOX_VERTICAL, 0,1,1,0,0,50,0, 0,npw,npw,nws,nwg,nhalign,0);
	else box=new SquishyBox(BOX_VERTICAL, 0,npw,npw,nws,nwg,nhalign, 0,1,1,0,0,50,0, 0);

	list.push(box,1,where);
}


//! Sets this->pw,s,g/ph,s,g to be the sum of the metrics of the boxes in list.
/*! Note that it does NOT set this->x,y,w,h.
 *
 * Simply calls figureDimensions(this).
 *
 * Returns 0 success, nonzero error.
 */
int ListBox::WrapToExtent()
{ 
	figureDimensions(this); 
	return 0;
}

//! Sync the box to the already set values of x,y,w,h.
/*! This is meant to be called only when a definite x,y,w,h have already
 *  been assigned for this box, such as by its parent box, and *this  
 *  must set the x,y,w,h of the child boxes, then tell all child boxes to sync their
 *  child boxes, etc.
 *
 *  All this default function does is call arrangeBoxes(1). The 1 tells arrangeBoxes()
 *  that the contained boxes should be distributed too. 
 *
 *  Normally,
 *  derived classes need only redefine arrangeBoxes() and/or distributeBoxes(), and not sync().
 *
 *  It goes:
 *  <pre>
 *   sync(0,0,w,h) //typically after a window resize event
 *    -> sync()
 *       -> arrangeboxes(1)
 *          -> distributeBoxes(0)
 *  </pre>
 */
void ListBox::sync()
{
	 // Arrange list to be some appropriate configuration.
	 // The 1 says to also set x,y,w,h of the child boxes, and tell them to sync(),
	 // which it does by calling distributeBoxes().
	arrangeBoxes(1); 
}

//! Arrange the list stack as appropriate.
/*! Any derived classes must make this function configure list to be something
 *  appropriate, meaning list should contain the boxes that make a horizontal (or vertical) list. 
 *  This function is typically called from sync(), which itself is normally
 *  called from a resize event.
 *
 *  The default ListBox is just a plain list, and therefore is inherently already
 *  arranged properly, and nothing is done to list.
 *
 *  Often, it is much more efficient to assign actual x,y,w,h values to boxes at the
 *  same time that the list stack is organized, so you may optionally assign those values
 *  when distributetoo is nonzero, which calls distributeBoxes(0).
 */
int ListBox::arrangeBoxes(int distributetoo) //distributetoo=0
{
	if (distributetoo) distributeBoxes(0);
	return 0;
}

//! Sets x,y,w,h of the child boxes in list, and tells them to sync().
/*! The default for ListBox is to layout ALL the boxes in list, based
 *  on the definite, and already set x,y,w,h of *this. It lays out ALL the boxes
 *  in list, whether they squish nicely or not.
 *
 *  If setmetrics!=0 then this->pw,s,g/ph,s,g are set to the pw,s,g/ph,s,g 
 *  found from the child boxes (via figureDimensions(this,...,&squishx,&squishy)).  
 *  This would be done to wrap this box around the extents of its children, for instance.
 *  Otherwise, leave this->pw,s,g/ph,s,g alone, and the 
 *  metrics derived from the child boxes are computed but not 
 *  stored (figureDimensions(NULL,...,&squishx,&squishy)). 
 *  They are just used to determine the squish values.
 *  
 *  This function assumes that the children already have their pw,s,g/ph,s,g (but not
 *  the x,y,w,h) set at desired values.
 *  First the squish factor is computed based on the preferred numbers, then
 *  the children's x,y,w,h are set, then each child's sync() is called.
 *
 *  If BOX_DONT_PROPAGATE_POS is not set, then this->x,y is
 *  added onto the newly found box->x,y. This means that when laying out massive
 *  complicated arrangements in a single window, say, then each box would have coordinates
 *  of the overall window, not just relative to its immediate parent box. This is important
 *  because the actual boxes the user uses might be contained in intermediary boxes,
 *  but this would mean window boxes offset their children incorrectly, since they use their own upper left
 *  corner as the origin, rather than a parent's origin. Thus, SquishyBoxes that are
 *  also windows should have BOX_DONT_PROPAGATE_POS set.
 *
 *  If there is a NULL in list, then the distributing stops at that NULL.
 *
 *  Returns 0 success, 1 error.
 *
 *  \todo ****** implement pad and penalty!!
 */
int ListBox::distributeBoxes(int setmetrics) //setmetrics=0
{
	if (list.n==0) return 0; // nothing to distribute!
	
	
	 // Find the squishx and squishy values.
	double squishx,squishy;
	 // Must find squish zones for the child boxes, NOT use the target or this->metrics, 
	 // because those are likely to be unrelated!!
	//int SquishyBox::figureDimensions(SquishyBox *target,int *nextrow,SquishyBox **boxes, int n,double *squishx,double *squishy) 
		
		// both of these calls to figureDimensions wraps to extent if this->m[1/7] are BOX_SHOULD_WRAP
		// However, only the first one actually changes the preferred metrics.
	if (setmetrics!=0) {
		figureDimensions(this,NULL,NULL,0,&squishx,&squishy);
	} else figureDimensions(NULL,NULL,NULL,0,&squishx,&squishy);
	
	 // clamp squish to [-1,1] because the extra pad is distributed in various ways other 
	 // than simple stretch
	if (squishx<-1) squishx=-1;
	else if (squishx>1) squishx=1;
	if (squishy<-1) squishy=-1;
	else if (squishy>1) squishy=1;

	
	 // Set offset into the metrics array.
	 // Later code looks as if it is adding to a row (horizontal list), but the offsets (row/col)
	 // can simply swap it all to make it actually add to a column (vertical list).
	 // Right here, we figure out what row and col should be, and how to deal with excess space.
	int row,col;
	int docol,stretchrow,stretchcol; // stretch=0 normal, 1=stretch, 2=space
	unsigned int centertype;
	if (flags&BOX_VERTICAL) { // is vertical list
		row=vert; col=0; 
		centertype=((flags&BOX_VALIGN_MASK)>>BOX_VALIGN_SHIFT)<<BOX_HALIGN_SHIFT;
		if (flags&BOX_STRETCH_TO_FILL_Y) stretchrow=1;
		else if (flags&BOX_SPACE_TO_FILL_Y) stretchrow=2;
		else stretchrow=0;
		if (flags&BOX_STRETCH_TO_FILL_X) stretchcol=1;
		else if (flags&BOX_SPACE_TO_FILL_X) stretchcol=2;
		else stretchcol=0;
		docol=1; //flags&BOX_SET_X_TOO;
		double tt=squishx; // swap the squishes.. Really, this could be done swapily with a squish[2], but what they hey
		squishx=squishy;
		squishy=tt;
	} else { // is horizontal list
		row=0; col=vert;
		centertype=flags&BOX_HALIGN_MASK;
		if (flags&BOX_STRETCH_TO_FILL_Y) stretchcol=1;
		else if (flags&BOX_SPACE_TO_FILL_Y) stretchcol=2;
		else stretchcol=0;
		if (flags&BOX_STRETCH_TO_FILL_X) stretchrow=1;
		else if (flags&BOX_SPACE_TO_FILL_X) stretchrow=2;
		else stretchrow=0;
		docol=1; //flags&BOX_SET_Y_TOO;
	} 

	
	SquishyBox **boxes=list.e;
	int rw=2*padinset; // the new calculated width of the boxes put next to each other.
			  // do not include target pad here; that goes outside the box
	
	 //Now distribute the boxes by setting their x,y,w,h
	 // First pass, apply the straight squishx, and set box->w, and also y,h (using squishy)
	int c;
	int n=list.n;
	for (c=0; c<n; c++) {
		if (!boxes[c]) break; // *** NULL is a forced line break, shouldn't really happen in list, but just in case
		if (boxes[c]->hidden()) continue;

		if (squishx>0 && boxes[c]->m[row+grow])  // must grow the box
			boxes[c]->m[row+len] = (int)(boxes[c]->m[row+pref] + boxes[c]->m[row+grow]*squishx + .5);
		else if (squishx<0 && boxes[c]->m[row+shrink])  // must shrink the box
			boxes[c]->m[row+len]  = (int)(boxes[c]->m[row+pref] + boxes[c]->m[row+shrink]*squishx + .5);
		else boxes[c]->m[row+len] = boxes[c]->m[row+pref]; // box is just right
		rw+=boxes[c]->m[row+len] + 2*boxes[c]->pad; //*** what about a padi?

		 // Must sync box y,h to the target
		if (docol) { // assumes m[col+len] set
			if (m[col+len]-2*boxes[c]->pad - 2*padinset >= boxes[c]->m[col+pref]-boxes[c]->m[col+shrink]  &&  
					m[col+len]-2*boxes[c]->pad <= boxes[c]->m[col+pref]+boxes[c]->m[col+grow]) // given this height is ok
				boxes[c]->m[col+len]=m[col+len] - 2*boxes[c]->pad - 2*padinset;
			else if (m[col+len]-2*boxes[c]->pad - 2*padinset< boxes[c]->m[col+pref]-boxes[c]->m[col+shrink]) {
				 // box much too big
				if (stretchcol==1) boxes[c]->m[col+len]=m[col+len] - 2*boxes[c]->pad - 2*padinset; //stretch
				else boxes[c]->m[col+len]=boxes[c]->m[col+pref]-boxes[c]->m[col+shrink]; //make smallest squishx
			} else {
				 // box much too small
				if (stretchcol==1) boxes[c]->m[col+len]=m[col+len] - 2*boxes[c]->pad - 2*padinset; //stretch(well, compress)
				else boxes[c]->m[col+len]=boxes[c]->m[col+pref]+boxes[c]->m[col+grow]; //make largest squishx
			}
			 // must also set boxes.y, this just centers in line.. 
			 // and adds on whatever this->m[col+pos] is
			boxes[c]->m[col+pos]=padinset + boxes[c]->pad +
								  (flags&BOX_DONT_PROPAGATE_POS?0:m[col+pos]) +
								  (m[col+len] -2*padinset - (boxes[c]->m[col+len] + 2*boxes[c]->pad))
								  		*(100-boxes[c]->m[col+align])/100;
		}
	}
	int extra=m[row+len]-rw; //*** what happens when extra is negative????
	//if (extra<0) extra=0; //***???

	 //Final pass must set box->x, and if necessary reset box->w to include extra stretch.
	 // find starting x point
	int x; // x does not include padinset
	if (stretchrow==1) x=0; //stretch
	else if (stretchrow==2) { //space
		if (centertype&BOX_HCENTER) x=extra/(n+1);
		else if (centertype&BOX_RIGHT) x=extra/n; // BOX_RIGHT==BOX_BOTTOM
		else x=0; // defaults to left justify
	} else { // normal l/r/c
		if (centertype&BOX_HCENTER) x=extra/2;
		else if (centertype&BOX_RIGHT) x=extra; // BOX_RIGHT==BOX_BOTTOM
		else x=0; //defaults to left justify
	} 
	 // Final pass: The following stuff with the wacky extra is to somewhat prevent heinous
	 // gaps at the end of the row because of rounding errors.  Otherwise, say the extra
	 // gap is 105, and you have 10 items. Simply distributing the gap amoung the items
	 // gives each 10 pixels of gap, but there are 5 pixels left over!
	for (c=0; c<n; c++) {
		if (!boxes[c]) break;
		if (boxes[c]->hidden()) continue;

		boxes[c]->m[row+pos]=padinset + 
							 x + 
							boxes[c]->pad + 
							(flags&BOX_DONT_PROPAGATE_POS?0:m[row+pos]); // makes the x this->x plus the running x
		DBG cerr <<boxes[c]->m[row+pos]<<endl;
		 // readjust boxes->w if necessary, and advance x
		if (stretchrow==1) { //stretch
			boxes[c]->m[row+len]+=extra/(n - c);
			x+=boxes[c]->m[row+len] + 2*boxes[c]->pad;
			extra-=extra/(n - c);
		} else if (stretchrow==2) { //space
			if (centertype&BOX_HCENTER) {
				x+=boxes[c]->m[row+len] + extra/(n+1-c) + 2*boxes[c]->pad;
				extra-=extra/(n+1-c);
			} else if (centertype&BOX_RIGHT) { 
				x+=boxes[c]->m[row+len] + extra/(n-c) + 2*boxes[c]->pad;
				extra-=extra/(n-c);
			} else if (flags&BOX_SPACE_JUSTIFY) {
				if (c!=n-1) {
					x+=boxes[c]->m[row+len] + extra/(n-1-c) + 2*boxes[c]->pad;
					extra-=extra/(n-1-c);
				}
			} else { // default for space is left justify
				x+=boxes[c]->m[row+len] + extra/(n-c) + 2*boxes[c]->pad;
				extra-=extra/(n-c);
			}
		} else { // no more width adjustment made and extra ignored
			x+=boxes[c]->m[row+len] + 2*boxes[c]->pad; //*** what about a padinset???
		} 
		
		 // Now box->x,y,w,h are all set, so call sync!
		DBG cerr <<"before sync"<<c<<": "<<m[pos]<<','<<m[vert+pos]<<"  "<<m[len]<<"x"<<m[vert+len]
		DBG		<<" pref:("<<m[pref]<<','<<m[shrink]<<','<<m[grow]
		DBG 	<<" x "<<m[vert+pref]<<','<<m[vert+shrink]<<','<<m[vert+grow]<<")"<<endl;
		
		boxes[c]->sync();

		DBG cerr <<"before sync"<<c<<": "<<m[pos]<<','<<m[vert+pos]<<"  "<<m[len]<<"x"<<m[vert+len]
		DBG		<<" pref:("<<m[pref]<<','<<m[shrink]<<','<<m[grow]
		DBG 	<<" x "<<m[vert+pref]<<','<<m[vert+shrink]<<','<<m[vert+grow]<<")"<<endl;
	}

	return 0;
}

//! Finds pw,s,g/ph,s,g based on the boxes in list.
/*! 
 *  To wrap *this to the metrics found from the boxes, call with target=this.
 *  Note that this only overwrites pw,s,g/ph,s,g. It does NOT change actual w,h, UNLESS
 *  target->w or h is equal to BOX_SHOULD_WRAP or flags\&BOX_WRAP_TO_X_EXTENT (or Y_EXTENT), in 
 *  which case they are set to the computed preferred w and/or h.
 *  
 *  If squish(x or y) is not NULL, then it is set to the value that would convert the newly found
 *  pw,s,g/ph,s,g to the actual w,h. Of course, this needs w,h to already be
 *  set to desired values. If they are not wrapped as above, then the calling code
 *  must have set these values. Please note that the newly found pw,s,g/ph,s,g do not necessarily
 *  have anything to do with those of this, or those previously held in target->m. They refer
 *  only to the sums of elements of boxes.
 *  
 *  The squish values computed are what is multiplied to boxes' metrics so that
 *  the list fits to the actual target->w,h.
 *
 *  If target is NULL, the pw,s,g/ph,s,g are not stored anywhere, and this->flags are
 *  used. In that case, this function might still be useful just to get the squish values, 
 *  which get based on fitting into this->w,h.
 *
 *  If target is not NULL, then target->flags are used, and the pw,s,g/ph,s,g are put
 *  into target, and the squish values are used to fit to target->w,h.
 *  
 *  If boxes is not null, then use that for the list of boxes assumed to have n boxes.
 *  Otherwise, use target->list.e/n, or if target is NULL, use this->list.e/n.
 *  Interprets boxes as a either a vertical or horizontal list, 
 *  according to whether BOX_VERTICAL is in flags or not.
 *  
 *  If nextrow is NULL, this uses ALL the boxes in list. It does not stop where a 
 *  line is filled. If nextrow is not NULL, then it does stop when a line is full, which
 *  requires that the target->w or h is set to a desired value already,
 *  and nextrow is set to what would be the starting box of the next line.
 *
 * 	If an element of boxes is NULL, then the figuring stops right there, and nextrow is set to point
 * 	to the first non-NULL box after that NULL, or n if there aren't any.
 *
 * 	\todo *** this must eventually be modified to use all the LRTB, LRBT, ...
 *  \todo ****** implement pad and penalty!!
 */
int ListBox::figureDimensions(ListBox *target,int *nextrow,SquishyBox **boxes, int n,double *squishx,double *squishy) 
{					//boxes=NULL, n=0

	 // Setup boxes to point to target->list, or this->list, or return
	if (boxes==NULL) {
		if (target) {
			boxes=target->list.e;
			n=target->list.n;
		} else {
			boxes=list.e;
			n=list.n;
		}
	}
	//*** if (!n || boxes[0]==NULL) punt?
		
	 // Set up the target. No boxes are added to target->list.
	int targetislocal=0;
	if (!target) {
		target=new ListBox();
		targetislocal=1;
		 // set up default target w/h to be this->w/h
		target->w(this->w()); //w
		target->h(this->h()); //h
		target->flags=flags;
	}
		
	 // Set offset into the metrics array.
	 // The code below looks as if it is adding to a row (horizontal list), but the offsets
	 // can simply swap it all to make it actually add to a column (vertical list).
	int row,col;
	if (target->flags&BOX_VERTICAL) { row=vert; col=0; } else { row=0; col=vert; } 

	int c,cs,tmp,padi=0,br=0; //***padi?? add a cellspacing/padi to SquishyBox?
	int rm[2*vert]; // the row metrics, laid out the same as the  SquishyBox::m[].
	for (c=0; c<2*vert; c++) rm[c]=0;

	c=0;
	cs=c; // cs=index of start of row
	
	 // Add up all the pw,s,g/ph,s,g of the boxes.
	 // Any NULL box means a forced line break.
	 // Fit them to a row with width target->[row+len]
	 // At end of loop, c must be at start of next line or n or at first NULL
	for (c=0; c<n && boxes[c]; c++) {
		if (boxes[c]->hidden()) continue;

		br=0;
		 // incorporate the width metrics
		tmp=(c>cs?padi:0) + boxes[c]->m[row+pref] + 2*boxes[c]->pad;
		rm[row+pref]  +=tmp; //pad is basically same as bevel
		rm[row+shrink]+=boxes[c]->m[row+shrink];
		rm[row+grow]  +=boxes[c]->m[row+grow];

		 // if nextrow==NULL, then use all the first non-NULL boxes
		 // otherwise, stop when the line is full.
		if (nextrow) {
			 // Check if sum of boxes exceeds the target width, 
			 // if so, try to shrink. If shrink is uncomfortable, remove the last added, 
			 // and break from adding more boxes.
			if (rm[row+pref]>target->m[row+len]-2*target->padinset) {
				br=1;
				if (rm[row+pref]-rm[row+shrink]>target->m[row+len]-2*target->padinset) { 
					 // If cannot shrink with these boxes, remove last added if possible
					if (c!=cs) { // remove last added
						rm[row+pref]  -=tmp; 
						rm[row+shrink]-=boxes[c]->m[row+shrink];
						rm[row+grow]  -=boxes[c]->m[row+grow];
					} else { // is on the first box, so include it, even though it is too honkin' big
						rm[col+pref]  =boxes[c]->m[col+pref]+2*boxes[c]->pad;
						rm[col+shrink]=boxes[c]->m[col+shrink];
						rm[col+grow]  =boxes[c]->m[col+grow];
						c++;
					}
					break; // end of line reached and no need to do the [col+*] below
				} //else c++; <-- Need the same c for [col+*]!!
			} 
		}
		
		 // incorporate the height metrics
		 // rs is not a true shrink here, it is temporarily length=max min
		 // rg is not a true shrink here, it is temporarily length=max max
		if (boxes[c]->m[col+pref]+2*boxes[c]->pad>rm[col+pref]) rm[col+pref]=boxes[c]->m[col+pref]+2*boxes[c]->pad;
		if (boxes[c]->m[col+pref]+2*boxes[c]->pad - boxes[c]->m[col+shrink]>rm[col+shrink]) //rs
			rm[col+shrink]=boxes[c]->m[col+pref]+2*boxes[c]->pad-boxes[c]->m[col+shrink]; 
		if (boxes[c]->m[col+pref]+2*boxes[c]->pad + boxes[c]->m[col+grow]>rm[col+grow]) //rg
			rm[col+grow]=boxes[c]->m[col+pref]+2*boxes[c]->pad + boxes[c]->m[col+grow]; 
		
		if (br) { c++; break; } // eol reached
	} 
	//if (cs==c) *** // no boxes were dealt with!
	while (c<n && !boxes[c]) c++; // skip over any NULL boxes
	if (nextrow) *nextrow=c; // this value is passed back
	
	 // At this point the metrics are added up, but no shrinking/growing has been done
	
	 // c now points to the first box of what ought to be the next line,
	 //  and either  rpw-rs<w<rpw  or  rpw<w
	 //  so now we have  rs,rpw,rg  and rpw may or may not be after w.
	 // Convert col s,g to real shrink and grow values (they were maximum min point, and max max point):
	rm[col+shrink]=rm[col+pref]-rm[col+shrink];
	if (rm[col+shrink]<0) rm[col+shrink]=0;
	rm[col+grow]  =rm[col+grow]-rm[col+pref];
	if (rm[col+grow]<0) rm[col+grow]=0;

	 // Set target->pw,s,g and maybe ->w
	int wraprow=(flags&BOX_VERTICAL && flags&BOX_WRAP_TO_Y_EXTENT) || (flags&BOX_HORIZONTAL && flags&BOX_WRAP_TO_X_EXTENT);
	target->m[row+pref]  =rm[row+pref] + 2*target->padinset;
	target->m[row+shrink]=rm[row+shrink];
	target->m[row+grow]  =rm[row+grow];
	if (target->m[row+len]==BOX_SHOULD_WRAP || wraprow) target->m[row+len]=target->m[row+pref]; // wrap to extent
	 // Set target->ph,s,g and maybe ->h
	wraprow=(flags&BOX_VERTICAL && flags&BOX_WRAP_TO_X_EXTENT) || (flags&BOX_HORIZONTAL && flags&BOX_WRAP_TO_Y_EXTENT);
	target->m[col+pref]  =rm[col+pref] + 2*target->padinset;
	target->m[col+shrink]=rm[col+shrink];
	target->m[col+grow]  =rm[col+grow];
	if (target->m[col+len]==BOX_SHOULD_WRAP || wraprow) target->m[col+len]=target->m[col+pref]; // wrap to extent

	
	 // Now we must determine the squish factor necessary to fit the rm[pref*] to target->w/h,
	 // which are assumed to already have been set to desirable values.
	 // Note that if the boxes can be adjusted within their
	 // specified squish zones, then abs(squish)<=1. Otherwise they must be stretched or compressed
	 // abnormally to fill the line.
	 //
	 // This is only done if the squish values are requested!
	double squishX=0,squishY=0;
	
	 // deal with squishX
	if ((target->flags&BOX_VERTICAL && squishy) || squishx) {
		if (rm[row+pref] < target->m[row+len]-2*target->padinset) { // must grow, squishX>0
			if (rm[row+grow]) squishX=(double) (target->m[row+len]-2*target->padinset-rm[row+pref])/rm[row+grow];
			else squishX=1000000; // simliar to infinity(?!)
		} else if (rm[row+pref] > target->m[row+len]-2*target->padinset) { // must shrink, squishX<0
			if (rm[row+shrink]) squishX=(double) (target->m[row+len]-2*target->padinset-rm[row+pref]) / rm[row+shrink];
			else squishX=-1000000; // simliar to infinity(?!)
		} else squishX=0;
	}
	 // deal with squishY
	if ((target->flags&BOX_VERTICAL && squishx) || squishy) {
		if (rm[col+pref] < target->m[col+len]) { // must grow, squishY>0 //**** before len and pref were switched!! which is right?
			if (rm[col+grow]) squishY=(double) (target->m[col+len]-2*target->padinset-rm[col+pref]) / rm[col+grow];
			else squishY=1000000; // simliar to infinity(?!)
		} else if (rm[col+pref] > target->m[col+len]) { // must shrink, squishY<0
			if (rm[col+shrink]) squishY=(double) (target->m[col+len]-2*target->padinset-rm[col+pref]) / rm[col+shrink];
			else squishY=-1000000; // simliar to infinity(?!)
		} else squishY=0;
	}

	if (flags&BOX_VERTICAL) {
		if (squishx) *squishx=squishY;
		if (squishy) *squishy=squishX;
	} else {
		if (squishx) *squishx=squishX;
		if (squishy) *squishy=squishY;
	}

	if (targetislocal) delete target;
	return 0;
}

//----------------------------------------- RowColBox ---------------------------------------
/*! \class RowColBox
 * \ingroup boxarrangers
 * \brief Divvies up the child boxes into multiple rows or columns.
 *
 * This class redefines ListBox's storage and arrange mechanism to separate a list of boxes
 * into multiple rows and columns by fitting boxes until the total preferred width is greater than
 * the actual width. If a sequence of boxes can be compressed to fit on 
 * the line and remain in its squish zone, then it is,
 * otherwise the boxes up to but not including the one causing the overflow are designated for that row,
 * and the next row gets configured.
 *
 * \todo *** need to debug laying out where boxes put in fixed width, but the whole height wraps
 *   to the vertical extent of the child boxes
 * \todo *** would be nice to auto layout in multiple columns if the boxes allow it.
 *\todo *** need to incorporate internal creation of sub-rows and columns... maybe that should
 *   only be done in subclasses? it is easy enough to push a new SquishyBox, but classes like
 *   RowFrame have special needs..
 */
/*! \var RefPtrStack<SquishyBox> RowColBox::wholelist
 * \brief The master list of boxes.
 *
 * this->list contains the arranged rows of the RowColBox. Each element of list is a list
 * of box pointers that point to the elements of wholelist. When adding, popping and flushing,
 * the actions are performend on wholelist rather than list.
 */


RowColBox::RowColBox()
{ 
//	if (flags&BOX_VERTICAL) flags|=BOX_STRETCH_TO_FILL_X;
//	else flags|=BOX_STRETCH_TO_FILL_Y;
	arrangedstate=0; 
	elementflags=0;
	filterflags();
}

//! Main RowColBox constructor.
/*! This screens nflags and changes it to values appropriate to a RowColBox.
 * To rephrase that, the flags you pass in are likely not exactly what is in
 * flags after the constructor finishes. See filterflags().
 */
RowColBox::RowColBox(unsigned int nflags,
			int nx,int nw,int npw,int nws,int nwg,int nhalign,int nhgap, 
			int ny,int nh,int nph,int nhs,int nhg,int nvalign,int nvgap)
  : ListBox(nflags,nx,nw,npw,nws,nwg,nhalign,nhgap,
			ny,nh,nph,nhs,nhg,nvalign,nvgap)
{
	arrangedstate=0; 
	elementflags=0;
	filterflags();
}

//! Take what is in flags, and remake flags to be reasonable for a RowColBox.
/*! Default for laying rows is for the main list to always stretch row->w (but not necessarily
 * to stretch the actual elements),  and h when laying columns.
 * If STRETCH_TO_FILL* was set in flags before calling filterflags(), then those
 * bits are copied to elementflags, and also stay in flags of this.
 *
 * elementflags are used in 
 * newSubBox() to help set the flags for boxes that contain the rows
 */
void RowColBox::filterflags()
{
	if (flags&BOX_VERTICAL) { // is laying out rows
		flags|=BOX_STRETCH_TO_FILL_X;
		if (flags&(BOX_STRETCH_IN_ROW|BOX_STRETCH_IN_COL)) elementflags|=BOX_STRETCH_TO_FILL_Y;
	} else { // laying out columns
		flags|=BOX_STRETCH_TO_FILL_Y;
		if (flags&(BOX_STRETCH_IN_ROW|BOX_STRETCH_IN_COL)) elementflags|=BOX_STRETCH_TO_FILL_X;
	}
	
	elementflags|=flags&(BOX_SPACE_MASK|BOX_STRETCH_MASK);
}

//! Remove box with index which from wholelist.
/*! Default (when which==-1) is to remove the top of the wholelist.
 */
int RowColBox::Pop(int which) //which=-1
{ 
	int er=wholelist.remove(which); 
	arrangedstate=0;
	return er;
}

SquishyBox *RowColBox::GetBox(int which)
{
	if (which>=0 && which<wholelist.n) return wholelist.e[which];
	return NULL;
}

//! Flush list, and if list does not exist, then create a new list.
void RowColBox::Flush()
{ 
	wholelist.flush(); 
	ListBox::Flush();
	arrangedstate=0;
}

//! Push box onto wholelist with islocal.
/*! If box==NULL, then this is later on interpreted as a line break.
 *
 * Sets arrangedstate=0. Nothing is added to list here. list is maintained
 * in arrangeBoxes().
 *
 * If islocal==0, then do not delete the box when removed. Otherwise, this RowColBox takes
 * possession of the box, and will delete it when no longer needed.
 */
void RowColBox::Push(SquishyBox *box,char islocal,int where) // islocal=0, where=-1
{
	wholelist.push(box,islocal,where);
	arrangedstate=0;
}


//! Set up SquishyBox::list to contain rows and columns based on boxes in wholelist.
/*! 
 * If BOX_VERTICAL, it lays out rows then columns, otherwise it lays out columns
 * then rows. This preserves the meaning of that from a normal ListBox.
 *
 * Assumes that w/h are already set.
 *
 * On success arrangedstate is set to 1 and 0 is returned, otherwise 
 * arrangedstate is set to 0 and 1 is returned.
 *
 * \todo *** should implement all 8 LRTB, LRBT, ... Currently horizontal is only LRTB,
 * and vertical is only TBLR.
 */
int RowColBox::arrangeBoxes(int distributetoo) // distributetoo=0
{
	 // Setup list to be a clean slate. All its elements should be nonlocal, so flushing is just fine.
	list.flush();
	
	ListBox *tbox;
	int c=0,cc,c2,cs=-1;
	
	 // find preliminary distribution of boxes, but do not set definite metrics yet.
	for (c=0; c<wholelist.n && cs!=c; ) {
		cs=c;
		 
		 // setup the new row (or col) box
		tbox=newSubBox();

		 // find dimensions of row, and add it to list
			//	virtual int figureDimensions(SquishyBox *target,int *nextrow=NULL,SquishyBox **boxes=NULL, int n=0,
			//								double *squishx=NULL,double *squishy=NULL); 
		cc=0;
		figureDimensions(tbox,&cc,wholelist.e+cs,wholelist.n-cs,NULL,NULL);
		for (c2=cs; c2<c+cc; c2++) tbox->Push(wholelist.e[c2],0);
		c+=cc;
		list.push(tbox);
	}

	arrangedstate=1;
	if (distributetoo) distributeBoxes(1);
	if (arrangedstate==1) return 0;
	return 1;
}

//! Return a new box that holds a whole row or column.
/*! For RowCols that have rows, this returns a box that wraps to y, and stretches x,
 * and vice versa for a columns box.
 *
 * Consults elementflags to see whether individual boxes should be stretched or spaced
 * within an extra space. For instance when laying out rows, a row might have many different
 * heights of boxes, element flags would indicate whether to stretch the boxes to fill the
 * height. Width stretching in this case is determined by the usual bits of flags, not 
 * elementflags. Vice versa for laying out columns.
 */
ListBox *RowColBox::newSubBox()
{
	ListBox *tbox=new ListBox();

	if (flags&BOX_VERTICAL) { // implies arrange TBLR (rowcol has rows), extend to the right, wrap to h
		 // ***note that TOP==LEFT, BOTTOM==RIGHT
		tbox->flags|= BOX_HORIZONTAL
					| BOX_WRAP_TO_Y_EXTENT 
			        | (flags&BOX_ALIGN_MASK) 
					| (elementflags&(BOX_STRETCH_MASK|BOX_SPACE_MASK)); 

		tbox-> w(w()-2*padinset);
		tbox->pw(w()-2*padinset); // make width map to fill this width
		tbox-> h(BOX_SHOULD_WRAP);

	} else { // implies arrange LRTB (rowcol has columns), extend off bottom, wrap to w
		tbox->flags|= BOX_VERTICAL
					| BOX_WRAP_TO_X_EXTENT
					| (flags&BOX_ALIGN_MASK)
					| (elementflags&(BOX_STRETCH_MASK|BOX_SPACE_MASK)); 
		tbox-> h(h()-2*padinset);
		tbox->ph(h()-2*padinset); // make width map to fill this width
		tbox-> w(BOX_SHOULD_WRAP);
	}
	return tbox;
}

//! Assign the x,y,w,h of the child boxes.
/*! This->x,y,w,h should all already be set.
 *
 * If arrangedstate!=1, then arrangeBoxes is called, which on success does set
 * arrangedstate to 1. Otherwise this simply calls SquishyBox::distributeBoxes().
 */
int RowColBox::distributeBoxes(int setmetrics)
{
//------*** distributeBoxes is called from arrangeBoxes! clean this up!!
//	if (arrangedstate!=1) arrangeBoxes();
//	if (arrangedstate!=1) return 1;
	
	 // The default SquishyBox::distributeBoxes does just fine, since the rows are properly arranged.
	return ListBox::distributeBoxes(setmetrics); 
}





//------------------------------------------------------------------------------------------
//------------------------------------ Tables ----------------------------------------------

/*! \class TableData
 * \ingroup boxarrangers
 * \brief Class used internally for the data of tables, oddly enough.
 *
 * Contains i,j,rowspan,colspan, and a pointer to the actual SquishyBox.
 *
 *  \todo ***please note that this class is totally unimplemented.
 */


/*! \class TableBox 
 * \ingroup boxarrangers
 * extra table options, not for cells:
 * 		border, cellspacing
 *
 *  ***For proper adding, the box MUST be a TableBox or SquishBox2. how  to 
 *  	have table data that is any kind of SquishyBox?
 *  	class TableData { public: int i,j,rowspan,colspan; SquishBox2 *box; }
 *
 *  \todo ***please note that this class is totally unimplemented.
 */
//class TableBox : public SquishyBox
//{
// public:
//	int i,j; // row i, column j == coordinates of upper left corner
//	int rowspan,colspan;
//	//int cellpadding; <-- use SquishyBox::pad
//};
//--OR--
//class TableLayout : public SquishyBox
//{
// public:
//	PtrStack<TableBox> boxes; <--*** can just use SquishyBox::list
//	int cellspacing;
//	*** is this really necessary? just make sure that the tableboxes are in the proper order?
//	void Init();
//	void NewRow();
//	void Push(TableData *box);
//	void Push(SquishBox2 *box); // constructs a TableData first...*** how exactly??
//	void Push(SquishyBox *box,int ii,int jj,int nrowspan,ncolspan);
//};
//
////! Arrange a bunch of TableBoxes into a table.
///*! The cellspacing becomes the pad of the returned SquishyBox.
// */
//SquishyBox *ArrangeTable(int options,int cellspacing,TableBox **boxes,int n,SquishyBox *tablebox_ret)
//{***
//}




} // namespace Laxkit

