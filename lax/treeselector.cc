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
//    Copyright (C) 2012-2014 by Tom Lechner
//


#include <lax/treeselector.h>
#include <lax/laxutils.h>
#include <lax/language.h>

#include <lax/lists.cc>
#include <lax/refptrstack.cc>

#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {
	


//---------------------------------------- TreeSelector::ColumnInfo ------------------------
/*! \class ColumnInfo
 * \brief Column info of detail columns in a TreeSelector.
 *
 * Strategy is to have three lists.
 * - menu: One is the base MenuInfo that lists every accessible item in the tree. This
 *   is not shown directly.
 * - visibleitems: a psuedo-flat list of all items that you can actually see. It contains pointers
 *   to the actual items in menu, but is interpreted as flat when refreshing
 * - selection: a subset of visibleitems that are currently selected.
 */


TreeSelector::ColumnInfo::ColumnInfo(const char *ntitle, int nwidth, int ntype, int whichdetail)
{
	type=ntype; //uses ColumnInfoType
	if (type==0) type=ColumnString;
	title=newstr(ntitle);
	pos=0;
	detail=whichdetail;
	width=nwidth;
}

TreeSelector::ColumnInfo::~ColumnInfo()
{
	if (title) delete[] title;
}


//---------------------------------------- TreeSelector ------------------------

/*! \class TreeSelector
 * \ingroup menuthings
 * \brief Collapsible tree view.
 *
 * \code
 * #include <lax/treeselector.h>
 * \endcode
 * 
 * There is a separate highlighting corresponding to a current item on which the next action will
 * be performed. For instance, say you press up or down, you are not necessarily selecting
 * the items that are up or down, but if you then press space, then that curitem
 * is selected. This 'user focused' item is stored in ccuritem. The last item
 * to have its state actually changed is stored in curitem.
 *
 * 
 * This class uses a PanController such that
 * the panner wholebox (min and max) is the bounding box of all the items,
 * and the selbox (start and end) is inrect. However, any shifting changes 
 * the panner's selbox, but this does not trigger any change in inrect.
 * 
 * <pre>
 *   TODO
 *   *** RemoveItem
 *   *** when mouseover up arrow or autoscroll up, moving down should not shiftscreen
 *   *** scrollbar (x/y) versus up/down arrows and right click dragging??
 *   		  
 *   *** neat from fltk:
 *    		Add("File")
 *    		Add("File/Save")
 *    		Add("File/Save As")....
 * </pre>
 */
/*! \var MenuInfo *TreeSelector::menu
 * \brief Stores the actual menu items.
 */
/*! \var int TreeSelector::iwidth
 * \brief The width of the sub menu indicator graphic of a menu item. (only one width, not per item)
 */
/*! \var int TreeSelector::pad
 * \brief The pad around the inside border of the window.
 */
/*! \var int TreeSelector::padg
 * \brief The pad to place between text and other graphic elements.
 */
/*! \var int TreeSelector::textheight
 * \brief The height of lines are leading+(text height).
 *
 * Any highlighting only includes the textheight pixels.
 */
/*! \var int TreeSelector::leading
 * \brief The height of lines are leading+(text height).
 *
 * Any highlighting does not include the leading pixels.
 */
/*! \var MenuItem *TreeSelector::curmenuitem
 * \brief The MenuItem corresponding to curitem.
 */
/*! \var int TreeSelector::curitem
 * \brief The last selected or deselected item.
 */
/*! \var int TreeSelector::ccuritem
 * \brief The item which one has cursored to.
 */
/*! \var int TreeSelector::pagesize
 * \brief 3/4 of inrect.height.
 */
/*! \var int TreeSelector::mousedragmode
 * \brief Flag for whether the mouse has been dragged since a button was down.
 *
 * If it is a simple move, then mousedragmode==1. If the menuselector is REARRANGEABLE,
 * and items are in the process of being moved, then mousedragmode==2.
 */
/*! \var unsigned long TreeSelector::highlight
 * \brief Highlight color of beveled graphics.
 */
/*! \var unsigned long TreeSelector::shadow
 * \brief Shadow color of beveled graphics.
 */

/*! \fn virtual int TreeSelector::Curitem()
 * \brief Return curitem, which is the last item whose state was toggled.
 *
 * Note that this is not necessarily ccuritem which is what one might go to
 * with the arrow keys without actually selecting.
 */
/*! \fn virtual const MenuItem *Item(int c)
 * \brief Publically accessible func to do: return item(c); 
 */


//! Constructor.
/*! If usethismenu is NULL, then a new MenuInfo is created. If usethismenu is
 * not NULL, then its count is incremented.
 *
 * Sets up colors, and pushes 0 onto the columns stack.
 */
TreeSelector::TreeSelector(anXWindow *parnt,const char *nname,const char *ntitle,
				unsigned long nstyle,  //!< Holds the usual anXWindow style flags
				int xx,int yy,int ww,int hh,int brder,
				anXWindow *prev,unsigned long nowner,const char *mes,
				unsigned long long nmstyle, //!< Holds the menu style defines
				MenuInfo *usethismenu //!< Pass in a MenuInfo class, increment its count
			)
  : ScrolledWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder,prev,nowner,mes,NULL)
{
	base_init();

	menustyle=nmstyle;
	if (usethismenu) {
		menu=usethismenu;
		menu->inc_count();
	} else {
		menu=new MenuInfo;
	}

	gap=3;
}

void TreeSelector::base_init()
{
	offsetx=offsety=0;
	mousedragmode=0;
	pad=3;
	textheight=0;
	lineheight=0;
	pagesize=0;
	ccuritem=curitem=0;
	curmenuitem=NULL;
	timerid=0;
	leading=0;
	menustyle=0;
	needtobuildcache=1;
	iwidth=10;

	searchfilter=NULL;
	showsearch=0;

	tree_column=0; //the column in which to position the tree lines
	sort_detail=-1;
	sort_descending=0;

	//installColors(app->color_buttons);
	//installColors(app->color_edits);
	installColors(app->color_panel);

	menu=NULL;
}

//! Destructor, increment count on menu.
TreeSelector::~TreeSelector()
{
	if (menu) menu->dec_count();
	if (searchfilter) delete[] searchfilter;

	DBG visibleitems.Flush();
}

int TreeSelector::InstallMenu(MenuInfo *nmenu)
{
	if (menu!=nmenu) {
		if (menu) menu->dec_count();
		menu=nmenu;
		nmenu->inc_count();
	}
	needtobuildcache=1;
	selection.flush();
	ccuritem=curitem=0;
	RebuildCache();
	RemapColumns();
	return 0;
}


//! Focus on draws the char over item.
int TreeSelector::FocusOn(const FocusChangeData *e)
{
	int c=anXWindow::FocusOn(e);
	//if (win_active) drawitem(ccuritem);
	return c;
}

//! Focus off draws the char over item.
/*! Also, if TREESEL_FOCUS_OFF_DESTROYS, then an off focus destroys this window.
 */
int TreeSelector::FocusOff(const FocusChangeData *e)
{
	DBG cerr <<"TreeSelector "<<WindowTitle()<<" FocusOff..."<<endl;

	int c=anXWindow::FocusOff(e);
//	if (!win_active) {
//		if (menustyle&TREESEL_DESTROY_ON_FOCUS_OFF) {
//			app->destroywindow(this);
//			return 0;
//		}
//		drawitem(ccuritem);
//	}
	return c;
}

//! Return the ith selelected item. i must be in range [0..NumSelected()-1].
MenuItem *TreeSelector::GetSelected(int i)
{
	if (i<0 || i>=selection.n) return NULL;
	return selection.e[i];
}

//! Return how many items are currently selected
int TreeSelector::NumSelected()
{
	return selection.n;
}

//! Default is to return the number of items in menu->menuitems stack.
/*! A tree class, for instance would return the number of items plus the items
 * from any open submenus.
 */
int TreeSelector::numItems()
{
	return visibleitems.menuitems.n;
}

//! Return the item corresponding to screen index i.
/*! This is not necessarily a direct index into the menu->menuitems stack, though
 * that is the default behavior of this function.
 * 
 * If open menus are displayed with this menu, like in a tree,
 * then this function must be redefined to access the thing associated with
 * screen index i.
 *
 * This function does not implement a search cache, but provides the tag
 * for subclasses to use if they want.
 * 
 * \todo maybe have option to force remapping==bypassing any search cache??
 */
MenuItem *TreeSelector::item(int i,char skipcache)
{
	if (i<0 || i>=visibleitems.menuitems.n) return NULL;
	return visibleitems.menuitems.e[i];
}

//! Programs call this to select index which of visible items.
/*! This has the same effect as left button down on it.
 *
 * Returns index of the new curitem.
 */
int TreeSelector::Select(int which)
{
	if (which<0 || which>=numItems()) return curitem;
	if (item(which)->state&LAX_ON) item(which)->state^=(LAX_ON|LAX_OFF);
	addselect(which,0);
	return curitem;
}

/*! Return 0 for success, or nonzero error.
 */
int TreeSelector::Deselect(int which)
{
	if (which<0 || which>=visibleitems.howmany(0)) return 1;
	MenuItem *i=item(which);
	i->state&=~(MENU_SELECTED|LAX_ON|LAX_OFF);
	i->state|=LAX_OFF;
	selection.remove(selection.findindex(i));
	needtodraw=1;
	return 0;
}

//! Expand which visible item. If already expanded, then do nothing.
/*! Return 1 for expanded, 0 for can't or error.
 */
int TreeSelector::Expand(int which)
{
	if (which<0 || which>=visibleitems.howmany(0)) return 0;
	visibleitems.menuitems.e[which]->state|=MENU_OPEN;
	needtobuildcache=1;
	needtodraw=1;
	return 1;
}

/*! Deselects any that are in submenus.
 */
int TreeSelector::Collapse(int which)
{
	if (which<0 || which>=visibleitems.howmany(0)) return 0;

	MenuItem *parent=item(which);
	for (int c=selection.n-1; c>=0; c--) {
		if (selection.e[c]->hasParent(parent)) {
			selection.e[c]->state&=~(LAX_ON|LAX_OFF|MENU_SELECTED);
			selection.e[c]->state|=LAX_OFF;
			selection.remove(c);
		}
	}

	parent->state&=~MENU_OPEN;
	needtobuildcache=1;
	needtodraw=1;
	return 1;
}


//! Make visible item with index which be near window coordinate x,y
/*! Returns 0 for success, or 1 for bad item.
 */
int TreeSelector::SetFirst(int which,int x,int y)
{
	if (which<0 || which>=visibleitems.howmany(0)) return 1;

	MenuItem *i=item(which);
	movescreen(0, y - (i->y+i->w/2));
	needtodraw=1;
	return 0;
}

//! Set some values that are derived from other values (pagesize, highlight, shadow, ...).
/*! Sets leading to have a little bit extra if TREESEL_CHECKBOXES and not otherwise specified.
 * Also sets the shadow and highlight colors, and determines the pagesize.
 */
int TreeSelector::init()
{
	ScrolledWindow::init(); //this calls syncWindows, causing inrect and pagesize to get set
    highlight=coloravg(win_colors->bg,rgbcolor(255,255,255));
	shadow   =coloravg(win_colors->bg,rgbcolor(0,0,0));
	
	leading=1;
	if (textheight==0) textheight=get_default_font()->textheight()+leading;
	iwidth=textheight;
	
	DBG cerr <<"--"<<WindowTitle()<<": textheight, leading="<<textheight<<','<<leading<<endl;
	
	if (menustyle&TREESEL_USE_TITLE) offsety+=textheight;


	RemapColumns();

	padg=textheight/2;
	pagesize=inrect.height*.75;
	arrangeItems();
	return 0;
}


/*! Add a new column for details of items.
 *
 * If whichdetail<0, then use columns.n (before pushing).
 * If ntype<=0, use ColumnString.
 */
int TreeSelector::AddColumn(const char *i,LaxImage *img,int width, int ntype, int whichdetail)
{
	if (whichdetail<0) whichdetail=columns.n;
	if (ntype<=0) ntype=ColumnInfo::ColumnString;
	columns.push(new ColumnInfo(i,width, ntype, whichdetail),1);
	return 0;
}

/*! Fill in a default width for any column that has width<=0.
 * Adjusts each column->pos to be after the width of previous columns.
 */
void TreeSelector::RemapColumns()
{
	if (columns.n) {
		offsety+=textheight;
		double pos=0;
		for (int c=0; c<columns.n; c++) {
			if (columns.e[c]->width<=0) {
				columns.e[c]->width=findColumnWidth(c);
				//if (columns.e[c]->width<=0) columns.e[c]->width=100;
			}
			columns.e[c]->pos=pos;
			pos+=columns.e[c]->width;
		}
	}
	needtodraw=1;
}

void TreeSelector::ClearColumns()
{
	columns.flush();
	needtodraw=1;
}


//! Sort the items alphabetically by the name.
/*! t is passed on to MenuInfo::sortstyle.
 * t==0 means do the default sorting.
 *
 * \todo *** should be able to sort only a subset, for instance all items between separators.
 */
void TreeSelector::Sort(int t, int detail) 
{
	if (menu) menu->sort(0,-1,detail);//*** this is now broken potentially cause numItems!=menuitems->n
}

//! Add a bunch of items all at once.
/*! Returns number of items added
 * \todo ***this is cheap, should be optimized for large arrays?? add in one lump, then sort??
 */
int TreeSelector::AddItems(const char **i,int n,int startid) // assume ids sequential, state=0
{
	if (i==NULL || n==0) return 0;
	for (int c=0; c<n; c++) AddItem(i[c],NULL, startid++,LAX_OFF);
	needtobuildcache=1;
	return n;
}

//! Add item with text i to the top index of menuitems.
/*! You can specify an initial state, such as LAX_GRAY, etc.
 *  Automatically sorts if sort is the style.
 *
 *  Returns the number of items in the stack, or -1 if error.
 *
 *  Does not synchronize the display. You must do that manually later on,
 *  unless init() hasn't been called yet. init() synchronizes the display.
 */
int TreeSelector::AddItem(const char *i,LaxImage *img,int nid,int newstate)
{
	if (!i && !img) return -1;
	if ((newstate&LAX_MSTATE_MASK)==0) newstate|=LAX_OFF;
	if (numItems()==0 && menustyle&TREESEL_ONE_ONLY && (newstate&LAX_OFF)) // if only one item, make it on
		newstate=(newstate^LAX_OFF)|LAX_ON;
	menu->AddItem(i,img,nid,newstate,0,NULL,-1,0); 
	needtobuildcache=1;
	return numItems();
}

double TreeSelector::getgraphicextent(MenuItem *mitem,double *w,double *h)
{
    if (!mitem || !mitem->image) {
        *w=0;
        *h=0;
        return 0;
    }
    *w=mitem->image->w();
    *h=mitem->image->h();
	return *w;
}

//! Find the maximum width of (text+ padg+ graphic subw) of items in range [s,e]
/*! Clamp s and e to [0,numItems()-1]. Return -1 if e<s.
 *
 * If the corresponding style (like for submenu indicator for instance) is not
 * set in menustyle, then that width is not included.
 *
 * Return the maximum height in h_ret.
 */
int TreeSelector::findmaxwidth(int s,int e, int *h_ret)
{
	if (e>=0 && e<s) return -1;
	if (s<0) s=0;
	if (s>=numItems()) s=numItems()-1;
	if (e<0 || e>=numItems()) e=numItems()-1;
	if (s<0 || e<0) return 0;
	double w=0,h=0,ww=0,hh=0,t;
	MenuItem *mitem;

	for (int c=s; c<=e; c++) {
		mitem=item(c);
		getgraphicextent(mitem,&ww,&hh);
		if (hh>h) h=hh;
		if (ww) t=ww+padg; else t=0;
		t+=getextent(mitem->name,-1,NULL,NULL);
		if (t>w) w=t;
	}
	w+=iwidth;

	if (h_ret) *h_ret=h;
	return w;
}

int TreeSelector::findColumnWidth(int which)
{
	int s=0;
	int e=numItems()-1;
	if (s<0 || e<0) return 0;

	double w=0,h=0,ww=0,hh=0,t;
	MenuItem *mitem;

	int col;
	for (int c=s; c<=e; c++) {
		mitem=item(c);
		col=which;
		while (mitem && col) {
			mitem=mitem->nextdetail;
			col--;
		}
		if (!mitem) continue;

		getgraphicextent(mitem,&ww,&hh);
		if (hh>h) h=hh;
		if (ww) t=ww+padg; else t=0;
		t+=getextent(mitem->name,-1,NULL,NULL);
		if (t>w) w=t;
	}
	w+=iwidth;

	return w;
}

//! Basically RebuildCache(), then update the panner.
void TreeSelector::arrangeItems()
{
	RebuildCache();

	IntRectangle wholerect;
	wholerect.x=wholerect.y=0;
	wholerect.width=findmaxwidth(0,-1,NULL);

	if (visibleitems.menuitems.n==0) return;

	MenuItem *item=visibleitems.menuitems.e[visibleitems.menuitems.n-1];
	wholerect.height=item->y+item->h;
	IntRectangle selbox=inrect;
	selbox.x+=offsetx;
	selbox.y+=offsety;

	if (selbox.x<wholerect.x) selbox.x=wholerect.x;
	if (selbox.y<wholerect.y) selbox.y=wholerect.y;
	if (selbox.x+selbox.width>wholerect.x+wholerect.width) selbox.width=wholerect.x+wholerect.width-selbox.x;
	if (selbox.y+selbox.height+gap>wholerect.y+wholerect.height) selbox.height=wholerect.y+wholerect.height-selbox.y-gap;

	panner->SetWholebox(wholerect.x,wholerect.x+wholerect.width-1,wholerect.y,wholerect.y+wholerect.height-1);
	panner->SetCurPos(1,selbox.x,selbox.x+selbox.width-1);
	panner->SetCurPos(2,selbox.y,selbox.y+selbox.height-1);
}

/*! Update visibleitems.
 * Called from Refresh() if needtobuildcache!=0.
 */
int TreeSelector::RebuildCache()
{
	visibleitems.Flush();
	addToCache(0, menu, 0);
	needtobuildcache=0;
	return 0;
}

//! Add items to visibleitems stack. Called from RebuildCache.
/*! Note the first item essentially has x,y at 0,0.
 */
int TreeSelector::addToCache(int indent,MenuInfo *mmenu, int cury)
{
	double xx=(indent+1)*iwidth;
	double ww,hh, hhh;
	MenuItem *i, *ii;

	for (int c=0; c<mmenu->n(); c++) {
		i=mmenu->e(c);
		if (i->hidden()==1) continue;
	
		hh=0;
		ii=i;
		while (ii) {
			getitemextent(ii, &ww,&hhh, NULL,NULL);
			ii->x=xx; //note: x will not be accurate for details
			ii->y=cury;
			ii->w=ww;
			ii->h=hhh;

			if (hhh>hh) hh=hhh; // *** really should only compute for visible, used columns
			ii=ii->nextdetail;
		}
		i->h=hh;
		visibleitems.AddItemAsIs(i,0);

		cury+=hh+1;

		 //draw any subitems and connecting lines
		if (i->hasSub()) {
			if (i->isOpen()) {
				 //item is open submenu
				cury=addToCache(indent+1,i->GetSubmenu(),cury);
			}
		}
	}

	return cury;
}

//! Find extent of text+graphic+(pad between graphic and text).
/*! If the graphic extent is 0, then the pad is not included.
 * w and h must not be NULL! Note that the y extent is the actual
 * y extent of the graphic+text. The actual space on the window that the
 * line takes up is still textheight+leading.
 * 
 * Returns the x extent.
 */
double TreeSelector::getitemextent(MenuItem *mitem, //!< the index, MUST already be in bounds
								double *w, //!< Put the x extent here
								double *h, //!< Put the y extent here
								double *gx, //!< Where the graphic would start, relative to whole item
								double *tx  //!< Where the text would start, relative to whole item
							)
{
	double gw,gh;
	getgraphicextent(mitem,&gw,&gh);
	double ww=getextent(mitem->name,-1,w,h,NULL,NULL);
	if (menustyle&TREESEL_GRAPHIC_ON_RIGHT) {
		if (tx) *tx=0;
		if (gx) *gx=(ww?ww+padg:0);
	} else {
		if (gx) *gx=0;
		if (tx) *tx=(gw?gw+padg:0);
	}
	if (h && gh>*h) *h=gh;
	if (gw) ww+=padg+gw;
	return ww;
}


//! Find screen rectangle item c goes in
/*! Return 1 for rectangle found, otherwise 0 for unable to find (for instance when called before
 * necessary initialization done).
 */
int TreeSelector::findRect(int c,IntRectangle *itemspot)
{
	MenuItem *i=item(c);
	if (!i) return 0;

	itemspot->x=i->x+offsetx;
	itemspot->y=i->y+offsety;
	itemspot->width=i->w;
	itemspot->height=i->h;

	return 1;
}

/*! Draw the window.
 * Use DrawItems(), then draw column info over that.
 *
 * <pre>
 *  Refresh()
 *   drawtitle();            -> overall title of whole menu
 *   draw column info-      <-  part of Refresh() currently
 *   DrawItems()             -> draws tree lines, determines x and y offset of item
 *     drawSubIndicator()    -> figure out if and how to draw the triangle to indicate submenus, and if open or not
 *       drawarrow()         -> actually draw arrow
 *     drawItemContents()    -> check if onscreen, determine specific screen rectangle draw item in, draw background
 *       drawsep()           -> draw a separator within specific rectangle
 *       drawCellsInLine()   -> *** todo: draw each cell for each detail as specified in columns
 *         drawitemname()     -> *** todo: draw icon+text
 *         drawflags()        -> *** todo: draw flag checkmarks
 *           drawFlagGraphic() -> draw an individual flag
 * </pre>
 */
void TreeSelector::Refresh()
{
	if (!win_on || !needtodraw) return;
	clear_window(this);

	 // Draw title if necessary
	if (menustyle&TREESEL_USE_TITLE) drawtitle();


	flatpoint p;
	NumStack<int> pos;

	if (needtobuildcache) RebuildCache();


	int indent=0;
	flatpoint offset(offsetx,offsety);
	int n=0;
	DrawItems(indent,menu,n,offset);
	
	 
	 //draw column info
	if (columns.n) {
		foreground_color(win_colors->bg);
		fill_rectangle(this, 0,0,win_w,textheight);
		foreground_color(win_colors->fg);
		for (int c=0; c<columns.n; c++) {
			if (c<columns.n-1) {
				draw_line(this, columns.e[c+1]->pos,0, columns.e[c+1]->pos,textheight);
			}

			if (isblank(columns.e[c]->title)) continue;
			textout(this, columns.e[c]->title,-1, gap+columns.e[c]->pos,0, LAX_LEFT|LAX_TOP);
		}
	}
		
	SwapBuffers();
	needtodraw=0;
}

/*! Recursively draw all unhidden items in mmenu.
 * Note that items drawn will be in order of visibleitems, but it uses menu directly, not visibleitems,
 * in order to determine the pattern of nesting lines to draw.
 *
 * Essentially draws a background, then drawItemContents() for that item.
 *
 */
int TreeSelector::DrawItems(int indent, MenuInfo *mmenu, int &n, flatpoint offset)
{
	MenuItem *i;
	int yy;
	unsigned long oddcolor=coloravg(win_colors->bg,win_colors->fg, .05);
	unsigned long linecolor=win_colors->fg;
	unsigned long col;
	int tree_offset=0;
	if (columns.n && tree_column!=0 && tree_column<columns.n) {
		tree_offset=columns.e[tree_column]->pos;
	}

	for (int c=0; c<mmenu->n(); c++) {
		i=mmenu->e(c);
		if (i->hidden()==1) continue;

		n++;
		yy=i->y + i->h/2; //so yy in center of row

		 //draw background
		if (i->isSelected()) {
			col=win_colors->hbg;

		} else {
			linecolor=win_colors->fg;
			if (n%2==0) {
				 //draw bg slightly darker
				col=oddcolor;
			} else {
				 //draw bg as bg
				col=win_colors->bg;
			}
		}
		 //highlight more if is focused item
		if (i==item(ccuritem)) col=coloravg(col,win_colors->fg,.05);
		foreground_color(col);
		fill_rectangle(this, offset.x+0,offset.y+i->y, win_w,i->h);


		 //draw small horizontal dash for current item
		foreground_color(linecolor);
		draw_line(this, tree_offset+offset.x+(indent-.5)*iwidth,offset.y+yy, tree_offset+offset.x+indent*iwidth,offset.y+yy);

		 //draw any subitems and connecting lines
		if (i->hasSub()) {
			if (i->isOpen()) {
				 //item is open submenu
				int newy=DrawItems(indent+1,i->GetSubmenu(),n,offset);
				draw_line(this, tree_offset+offset.x+(.5+indent)*iwidth, offset.y+yy,
								tree_offset+offset.x+(.5+indent)*iwidth, offset.y+newy); //vertical line
				drawSubIndicator(i, tree_offset+offset.x+indent*iwidth,offset.y+yy, i->isSelected());
				yy=newy;
			} else {
				 //item is closed submenu
				drawSubIndicator(i, tree_offset+offset.x+indent*iwidth,offset.y+yy, i->isSelected());
			}
		}
		 
		 //finally draw actual item contents in cached area
		drawItemContents(i,offset.x,offset.y, 0, (indent+1)*iwidth);
	}

	return yy;
}

//! Draws a submenu indicator centered on x,y, and width=iwidth, height=textheight+leading.
void TreeSelector::drawSubIndicator(MenuItem *mitem,int x,int y, int selected)
{
	if (!mitem->hasSub()) return;

	int arrowtype=0;
	if (mitem->isOpen()) arrowtype=THING_Triangle_Down;
	else arrowtype=THING_Triangle_Right;

	int subw=iwidth;
	x+=subw/2;
	if (subw>(textheight+leading)) subw=(textheight+leading)/3;
	else subw/=3;

	if (menustyle&TREESEL_SUB_FOLDER) { arrowtype=THING_Folder; }

	drawarrow(x,y, subw, arrowtype);
}

//! Draw the arrows for menus, really just THING_Triangle_Up, Down, Left, Right for submenus.
/*! Centers around x,y within a box of side 2*r.
 * This function is also used for the up and down arrows when the menu contents 
 * extend beyond the screen.
 */
void TreeSelector::drawarrow(int x,int y,int r,int type)
{
	foreground_color(win_colors->color1); // inside the arrow
	draw_thing(this, x,y,r,r,1,(DrawThingTypes)type);
	foreground_color(win_colors->fg); // border of the arrow
	draw_thing(this, x,y,r,r,0,(DrawThingTypes)type);
}

//! Draw the menu title if present. Default is print it out at top of window (not inrect).
void TreeSelector::drawtitle()
{
	if (!menu || !menu->title) return;
	foreground_color(win_colors->fg); // background of title
	fill_rectangle(this, 0,pad, win_w,textheight+leading);
	foreground_color(win_colors->fg); // foreground of title
	textout(this,menu->title,-1,win_w/2,pad,LAX_CENTER|LAX_TOP);
}

/*! i is the head MenuItem of visibleitems. Note that i might not itself be shown,
 * if columns maps to only other details.
 *
 * Item contents are drawn starting at specified offset,
 * which means this function draws inside a rectangle with upper left corner
 * at (i->x+offsetx, i->y+offsety), width and height  to be determined by columns
 * and width, height of relevant menu details.
 *
 * Checks bounds, so no drawing if off screen.
 *
 * If bounds ok, and fill!=0, draw background.
 *
 * Finally, call either drawsep() or drawitemname() on relevant cells.
 *
 * indent is how much to indent a cell's contents when its column is the tree_column.
 */
void TreeSelector::drawItemContents(MenuItem *i,int offsetx,int offsety, int fill, int indent)
{
	IntRectangle itemspot;
	itemspot.x=i->x+offsetx;
	itemspot.y=i->y+offsety;
	itemspot.width=i->w;
	itemspot.height=i->h;

	 // If itemspot not onscreen, return
	if (itemspot.x+itemspot.width <=inrect.x || itemspot.x>=inrect.x+inrect.width ||
		itemspot.y+itemspot.height<=inrect.y || itemspot.y>=inrect.y+inrect.height) {
		//DBG cerr <<"Item "<<c<<" not on screen...."<<endl;
		return;
	}

	if (fill) {
		// ****** not used anymore?? wipes out tree lines...
		 //draw background
		unsigned long color;

		if (i->isSelected()) {
			 //draw bg selected
			color=win_colors->hbg;
		} else {
			int n=visibleitems.menuitems.findindex(i);

			 //draw bg slightly darker on odd lines
			if (n%2==1) color=coloravg(win_colors->bg,win_colors->fg, .05);
			else color=win_colors->bg;
		}

		int isccuritem=(i==item(ccuritem));
		if (isccuritem) color=coloravg(color,win_colors->fg,.05);
		foreground_color(color);
		fill_rectangle(this, offsetx+i->x,offsety+i->y, win_w,i->h);
	}


	//now draw actual content of item
	
	if (i->state&LAX_SEPARATOR) {
		 //draw a separator between edge of tree lines, filling rest of line
		int tree_offset=0;
		if (columns.n && tree_column!=0 && tree_column<columns.n) {
			tree_offset=columns.e[tree_column]->pos;
		}
		itemspot.x=offsetx+tree_offset+indent;
		itemspot.width=inrect.x+inrect.width-itemspot.x;
		drawsep(i->name,&itemspot);
		return;
	}

	 // Draw the item icon and text for each detail
	int start=0;
	if (columns.n==0) start=-1;
	MenuItem *ii;
	for (int c=start; c<columns.n; c++) {
		ii=i;
		if (c>=0) {
			itemspot.x=offsetx+columns.e[c]->pos;
			itemspot.width=columns.e[c]->width;
			ii=ii->GetDetail(columns.e[c]->detail);
			if (c==tree_column) {
				itemspot.width-=indent;
				itemspot.x+=indent;
			}
			if (columns.e[c]->type==ColumnInfo::ColumnFlags) {
				drawflags(ii, &itemspot);
				ii=NULL;
			}
		} else {
			itemspot.width-=indent;
			itemspot.x+=indent;
		}
		if (ii) drawitemname(ii,&itemspot);
	}
}

// *** if removing, update drawItemContents() to not use the "fill" part...
//
////! Draw the item with the index value c. ***** no longer used? remove?
///*!  All the things that go on an item line:\n
// * [status graphic=whether highlighted][check toggle][graphic icon][text][submenu indicator]
// *
// * First, determine if this item is actually on screen, and find the area to draw it in. Return if not on screen.
// * If it is found, then find the item and call drawitem(theitem,thebounds).
// */
//void TreeSelector::drawitem(int c)
//{
//	if (!ValidDrawable()) return;
//
//	if (c<0 || c>=numItems()) return;
//	drawItemContents(item(c), offsetx,offsety,1);
//}


/*! flags are parsed from mitem->name. There is one flag per byte of mitem->name.
 *
 * Then drawFlagGraphic() is used to draw actual flag.
 */
void TreeSelector::drawflags(MenuItem *mitem,IntRectangle *rect)
{
	const char *f=mitem->name;
	if (!f) return;

	int x=rect->x;
	for (unsigned int c=0; c<strlen(f); c++) {
		x=drawFlagGraphic(f[c], x+rect->height/2,rect->y, rect->height);
	}
}

/*! A couple of special cases are implemented.
 * If flag=='l', then an unlocked lock is drawn.
 * If flag=='L', then a locked lock is drawn.
 * if flag=='e', then a closed eye is drawn.
 * if flag=='E', then an open eye is drawn.
 * 
 * Otherwise, if a character is ' ', then by default, it is off, and nothing is drawn.
 * If not ' ', then a check mark is drawn.
 *
 * Return the new x size.
 */
int TreeSelector::drawFlagGraphic(char flag, int x,int y,int h)
{
	if (flag==' ') return x+h;

	DrawThingTypes thing=THING_Check;
	if (flag=='l') thing=THING_Unlocked;
	else if (flag=='L') thing=THING_Locked;
	else if (flag=='e') thing=THING_Closed_Eye;
	else if (flag=='E') thing=THING_Open_Eye;

	foreground_color(0);
	if (flag=='e' || flag=='E') background_color(~0);
	else background_color(.7,.7,.7);
	drawing_line_attributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	draw_thing(this, x,y+h/2, h*.4,-h*.4, 2, thing);
	return x+h;
}

/*! Draw the item icon and name in rect.
 * Assumes mitem is the relevant detail within rect, and that background colors have been drawn already.
 */
void TreeSelector::drawitemname(MenuItem *mitem,IntRectangle *rect)
{
	unsigned long f,g;
	double fasc,tx,gx,iw;
	fasc=get_default_font()->ascent();

	getitemextent(mitem,&iw,NULL,&gx,&tx); // status graphic and name x coordinate

	gx+=rect->x; tx+=rect->x; // |blah    |
	//else if (menustyle&TREESEL_RIGHT) { gx=rect->x+rect->width-iw+gx; tx=rect->x+rect->width-iw+tx; } // |   blah|
	//else { gx=rect->x+(rect->width-iw+gx)/2; tx=rect->x+(rect->width-iw+tx)/2; } // |   blah   |

	
	 // set proper foreground and background colors
	//DBG cerr<<"menu "<<(menu->title?menu->title:"untitled")<<" item "<<c<<": "<<(mitem->state&LAX_HAS_SUBMENU?1:0)<<endl;
	if ((mitem->state&LAX_MSTATE_MASK)>LAX_ON) { // grayed, hidden=0, off=1, on=2, so this is same as>2
		//DBG cerr <<"item "<<(mitem->state&LAX_ON?1:0)<<" "<<mitem->name<<endl;
		g=win_colors->bg;
		f=win_colors->grayedfg;
	} else {
		//DBG cerr <<"item "<<(mitem->state&LAX_ON?1:0)<<" "<<mitem->name<<endl;
		g=win_colors->bg;
		f=win_colors->fg;
	}

	 // do the actual drawing
	if (mitem->state&LAX_ON) { // draw on
		g=win_colors->hbg;
		f=win_colors->hfg;
	} 
	 // add a little extra hightlight if item is ccuritem
	//if (!(menustyle&TREESEL_ZERO_OR_ONE)) if (mitem==item(ccuritem)) g=coloravg(f,g,.85);
	
	//foreground_color(g); // only draw highlighted if not checkboxes
	//fill_rectangle(this, rect->x,rect->y,rect->width,rect->height);


	foreground_color(f);
	background_color(g);
	if (mitem->name) textout(this, mitem->name,strlen(mitem->name), tx,rect->y+fasc+leading/2, LAX_LEFT|LAX_BASELINE);
	if (mitem && mitem->image) {
		image_out(mitem->image, this, gx, rect->y);
	}

}

//! Draw the item icon and name in rect. *** keeping for reference
void TreeSelector::drawitemnameOLD(MenuItem *mitem,IntRectangle *rect)
{
	unsigned long f,g;
	double fasc,tx,gx,iw;
	fasc=get_default_font()->ascent();

	// ****** ToDo: use custom ordering of columns, maybe have 
	//          drawcells() which determines cell area, then calls drawitemname() for drawing within each cell
	//          according to column info
	//
	getitemextent(mitem,&iw,NULL,&gx,&tx); // status graphic and name x coordinate

	gx+=rect->x; tx+=rect->x; // |blah    |
	//else if (menustyle&TREESEL_RIGHT) { gx=rect->x+rect->width-iw+gx; tx=rect->x+rect->width-iw+tx; } // |   blah|
	//else { gx=rect->x+(rect->width-iw+gx)/2; tx=rect->x+(rect->width-iw+tx)/2; } // |   blah   |

	
	 // set proper foreground and background colors
	//DBG cerr<<"menu "<<(menu->title?menu->title:"untitled")<<" item "<<c<<": "<<(mitem->state&LAX_HAS_SUBMENU?1:0)<<endl;
	if ((mitem->state&LAX_MSTATE_MASK)>LAX_ON) { // grayed, hidden=0, off=1, on=2, so this is same as>2
		//DBG cerr <<"item "<<(mitem->state&LAX_ON?1:0)<<" "<<mitem->name<<endl;
		g=win_colors->bg;
		f=win_colors->grayedfg;
	} else {
		//DBG cerr <<"item "<<(mitem->state&LAX_ON?1:0)<<" "<<mitem->name<<endl;
		g=win_colors->bg;
		f=win_colors->fg;
	}

	 // do the actual drawing
	if (mitem->state&LAX_ON) { // draw on
		g=win_colors->hbg;
		f=win_colors->hfg;
	} 
	 // add a little extra hightlight if item is ccuritem
	if (!(menustyle&TREESEL_ZERO_OR_ONE)) if (mitem==item(ccuritem)) g=coloravg(f,g,.85);
	
	//foreground_color(g); // only draw highlighted if not checkboxes
	//fill_rectangle(this, rect->x,rect->y,rect->width,rect->height);


	foreground_color(f);
	background_color(g);
	if (mitem->name) textout(this, mitem->name,strlen(mitem->name), tx,rect->y+fasc+leading/2, LAX_LEFT|LAX_BASELINE);
	MenuItem *im=mitem;
	if (im && im->image) {
		image_out(im->image, this, gx, rect->y);
	}
	MenuItem *detail=mitem->nextdetail;
	int column=1;
	while (detail) {
		textout(this, detail->name,strlen(detail->name), offsetx+columns.e[column]->pos,rect->y+fasc+leading/2, LAX_LEFT|LAX_BASELINE);
		detail=detail->nextdetail;
		column++;
	}

}

//! Draw a separator (default is just a win_colors->grayedfg colored line) across rect widthwise.
void TreeSelector::drawsep(const char *name,IntRectangle *rect)
{
	foreground_color(win_colors->grayedfg); 
	draw_line(this, rect->x,rect->y+rect->height/2, rect->x+rect->width-1,rect->y+rect->height/2);
	if (!isblank(name)) {
		int extent=getextent(name, -1, NULL, NULL, NULL, NULL, 0);
		 //blank out area
		foreground_color(win_colors->bg); 
		fill_rectangle(this, rect->x+rect->width/2-extent/2-2,rect->y,extent+4,rect->height);
		 //draw name
		foreground_color(win_colors->grayedfg); 
		textout(this, name,-1,rect->x+rect->width/2,rect->y+rect->height/2,LAX_CENTER);
	}
}




//! Send message to owner.
/*! If !(menustyle&TREESEL_SEND_STRINGS) Sends SimpleMessage with:
 * \code
 * info1 = curitem (which might be -1 for nothing)
 * info2 = id of curitem
 * info3 = nitems selected
 * info4 = menuitem->info
 * \endcode
 *
 * Otherwise, sends a list of the selected strings in a StrsEventData,
 * with info=curitem.
 *
 * \todo *** there needs to be option to send id or list of ids..
 * \todo maybe send device id of the device that triggered the send
 */
int TreeSelector::send(int deviceid)
{
	DBG cerr <<WindowTitle()<<" send"<<endl;
	if (!win_owner || !win_sendthis) return 0;

	 // find how many selected
	if (selection.n==0 && !(menustyle&TREESEL_SEND_ON_UP)) return 0;

	if (menustyle&TREESEL_SEND_STRINGS) {
		StrsEventData *strs=new StrsEventData(NULL,win_sendthis,object_id,win_owner);
		strs->n=selection.n;
		strs->strs=new char*[selection.n+1];
		strs->info=curitem;
		int i=0;
		for (int c=0; c<selection.n; c++) strs->strs[i++]=newstr(selection.e[c]->name);
		strs->strs[i]=NULL;
		app->SendMessage(strs,win_owner,win_sendthis,object_id);

	} else {
		SimpleMessage *ievent=new SimpleMessage;
		ievent->info1=curitem; 
		ievent->info2=(curitem>=0 && curitem<numItems() ? item(curitem)->id : curitem);
		ievent->info3=selection.n;
		ievent->info4=(curitem>=0 && curitem<numItems() ? item(curitem)->info : 0);
		app->SendMessage(ievent,win_owner,win_sendthis,object_id);
	}

	return 0;
}

//------ non-input refreshing and arranging is above ^^^
//------       mostly user input functions are below vvv


//! Character input.
/*! Currently: 
 * <pre>
 *  enter      ???send/select currently selected items
 *  space      addselect(ccuritem)
 *  pgup/down, Jump ccuritem 1 page up or down
 *  up/down,   Move ccuritem 1 item up or down
 *  'a'        If none selected, select all, else deselect any selected.
 *  '/'        (***imp me!) start search mode?-- have either sep search mode
 *                   or make all selecting keys above be control-key, not just key
 *                   so can use normal typing as progressive search
 *  *** need to have type to search, or hotkeys
 * </pre>
 */
int TreeSelector::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
	if (ch==0) return 0; // null
	if (ch==LAX_Tab) return anXWindow::CharInput(ch,buffer,len,state,d); // tab
	if (ch==LAX_Enter) { // enter
		send(d->id);
		return 0;

	} else if (ch==LAX_Home) { // jump to beginning
		int c=ccuritem;
		ccuritem=0;
		if (ccuritem!=c) {
			//drawitem(c);
			c=makeinwindow();
			if (menustyle&TREESEL_CURSSELECTS) {
				addselect(ccuritem,state);
			} else if (!c) {
				//drawitem(ccuritem);
			}
			needtodraw|=2;
		}
		return 0;

	} else if (ch==LAX_End) { // jump to end
		int c=ccuritem;
		ccuritem=numItems()-1;
		if (ccuritem!=c) {
			//drawitem(c);
			c=makeinwindow();
			if (menustyle&TREESEL_CURSSELECTS) {
				addselect(ccuritem,state);
			} else if (!c) {
				//drawitem(ccuritem);
			}
			needtodraw|=2;
		}
		return 0;

//	} else if (ch==LAX_Esc) { //escape
//		if ((menustyle&TREESEL_DESTROY_ON_LEAVE) || (menustyle&TREESEL_DESTROY_ON_FOCUS_OFF)) {
//			app->destroywindow(this);
//			return 0;
//		}
//		return 1;

	} else if (ch==LAX_Pgup) { // pgup
		DBG cerr <<"pgup "<<pagesize<<endl;
		
		movescreen(0,-pagesize);
		return 0;

	} else if (ch==LAX_Pgdown) { // pgdown
		DBG cerr <<"pgdown "<<pagesize<<endl;
		
		movescreen(0,pagesize);
		return 0;

	} else if (ch==LAX_Up) { // up
		int c=ccuritem;
		ccuritem--;
		if (ccuritem<0) ccuritem=0;
		if (ccuritem!=c) {
			//drawitem(c);
			c=makeinwindow();
			if (menustyle&TREESEL_CURSSELECTS) {
				addselect(ccuritem,state);
				//send(d->id);
			} else if (!c) {
				//drawitem(ccuritem);
			}
			needtodraw|=2;
		}
		return 0;

	} else if (ch==LAX_Down) { // down
		int c=ccuritem;
		ccuritem++;
		if (ccuritem>=numItems()) ccuritem=numItems()-1;
		if (ccuritem!=c) {
			//DBG cerr <<"ccuritem: "<<ccuritem<<"  c="<<c<<endl;
			//drawitem(c);
			c=makeinwindow();
			if (menustyle&TREESEL_CURSSELECTS) {
				addselect(ccuritem,state);
				//send(d->id);
			} else if (!c) {
				//drawitem(ccuritem);
			}
			needtodraw|=2;
		}
		return 0;

	} else if (ch=='-') {  // Collapse current item, or all if control
		if (state&ControlMask) {
			for (int c=visibleitems.n()-1; c>=0; c--) {
				if (c>=visibleitems.n()) continue;
				if (visibleitems.e(c)->isOpen()) Collapse(c);
			}
			arrangeItems();
			return 0;
		}
		MenuItem *i=item(ccuritem);
		if (!i) return 0;
		if (i->isOpen()) {
			Collapse(ccuritem);
			arrangeItems();
		}
		return 0;

	} else if (ch=='=' || ch=='+') {  // Expand current item, or all if control
		if (state&ControlMask) {
			for (int c=0; c<visibleitems.n(); c++) {
				if (visibleitems.e(c)->hasSub()) Expand(c);
			}
			arrangeItems();
			return 0;
		}
		MenuItem *i=item(ccuritem);
		if (!i) return 0;
		if (!i->isOpen()) {
			Expand(ccuritem);
			arrangeItems();
		}
		return 0;

	} else if (ch==' ') {  // same as an LBDown on ccuritem
		addselect(ccuritem,state);
		if (menustyle&TREESEL_SEND_ON_UP) send(d->id);
		return 0;

	} else if (ch=='a') {
		if (selection.n) {
			 //deselect
			for (int c=0; c<selection.n; c++) {
				selection.e[c]->state&=~(LAX_ON|LAX_OFF|MENU_SELECTED);
				selection.e[c]->state|=LAX_OFF;
			}
			selection.flush();
		} else {
			 //select all
			for (int c=0; c<numItems(); c++) {
				selection.push(item(c),0);
				item(c)->state&=~(LAX_ON|LAX_OFF|MENU_SELECTED); 
				item(c)->state|=LAX_ON|MENU_SELECTED;
			}
			if (menustyle&(TREESEL_ONE_ONLY|TREESEL_ZERO_OR_ONE)) addselect(ccuritem,0);
		}
		needtodraw=1;
		return 0;
	}

	return anXWindow::CharInput(ch,buffer,len,state,d);
}

//! This is called on a mouse down or a space press.
/*! Sets the state of the affected items, and also draws the
 * results on the window. A plain select will deselect all,
 * and select just i. A control
 * select will toggle whether an item is on or off, without
 * deselecting all the others. A shift select will turn on any items
 * in the range that are off. A control-shift select will make all in the
 * range [curitem,i] have the same state as curitem. 
 *
 * An item's state is only modified if it is already LAX_ON or LAX_OFF.
 *
 * Does not send the control message from here.
 */
void TreeSelector::addselect(int i,unsigned int state)
{
	DBG cerr <<(WindowTitle())<<" addselect:"<<i<<" state="<<state<<endl;
	int c;
	MenuItem *mitem=item(i),*titem;
	if (!mitem) return;

	if (!(state&ShiftMask) || menustyle&(TREESEL_ZERO_OR_ONE|TREESEL_ONE_ONLY)) { // select individual
		int oldstate=mitem->state;
		if (!(state&ControlMask)) {
			 // unselect others
			if (selection.n) oldstate=LAX_OFF;
			for (c=selection.n-1; c>=0; c--) { // turn off all the ones that are on
				titem=selection.e[c];
				selection.remove(c);
				titem->state&=~(LAX_ON|LAX_OFF|MENU_SELECTED); 
				titem->state|=LAX_OFF; 
				//drawitem(visibleitems.menuitems.findindex(titem));
			}
		}

		if (menustyle&TREESEL_ONE_ONLY || oldstate==0 || (oldstate&LAX_MSTATE_MASK)==LAX_OFF) {
			mitem->state=(mitem->state&~(LAX_ON|LAX_OFF|MENU_SELECTED))|(LAX_ON|MENU_SELECTED);
			selection.pushnodup(mitem,0);
		} else if ((oldstate&(LAX_ON|LAX_OFF|MENU_SELECTED))==LAX_ON) {
			mitem->state=(mitem->state&~(LAX_ON|LAX_OFF|MENU_SELECTED))|LAX_OFF;
			selection.remove(selection.findindex(mitem));
		}
		c=ccuritem;
		ccuritem=curitem=i;
		curmenuitem=mitem;
		//drawitem(c);       // draw off old ccuritem
		//drawitem(curitem); // draw on curitem==ccuritem
		needtodraw|=2;

	} else if (state&ShiftMask) { // select range
		int start,end;
		unsigned int nstate=curmenuitem->state&(LAX_ON|LAX_OFF|MENU_SELECTED);
		if (!(nstate&(LAX_ON|LAX_OFF))) nstate=LAX_ON|MENU_SELECTED;
		if (i<curitem) { start=i; end=curitem; } else { start=curitem; end=i; }

		for (c=start; c<=end; c++) {
			 //make each item's state the same as the old curitem state
			 //or toggle the state if control is also pressed. toggle is only 
			 //between ON and OFF, otherwise curitem->state is used.
			 //***if curitem was gray?
			 //If the item to change is LAX_GRAY, then do not change it
			titem=item(c);
			if ((menustyle&TREESEL_SELECT_LEAF_ONLY) && (titem->hasSub())) continue;
			if ((menustyle&TREESEL_SELECT_SUB_ONLY) && !(titem->hasSub())) continue;
			if ((titem->state&LAX_MSTATE_MASK)==LAX_GRAY) continue;
			if (titem->state&LAX_SEPARATOR) continue;

			if (state&ControlMask) { 
				 //toggle
				if ((titem->state&(LAX_OFF|LAX_ON|MENU_SELECTED))!=0) {
					if ((titem->state&(LAX_ON|LAX_OFF|MENU_SELECTED))!=nstate) {
						titem->state=(titem->state&~(LAX_ON|LAX_OFF|MENU_SELECTED))|nstate; 
						if (nstate&LAX_ON) selection.pushnodup(mitem,0);
						else selection.remove(selection.findindex(titem));
						//drawitem(c);
					}
				}
			} else if ((titem->state&(LAX_ON|LAX_OFF|MENU_SELECTED))==LAX_OFF) { 
				 //turn on
				titem->state=(titem->state&~(LAX_ON|LAX_OFF|MENU_SELECTED))|(LAX_ON|MENU_SELECTED); 
				selection.pushnodup(titem,0);
				//drawitem(c); 
			}
		}
		c=ccuritem;
		ccuritem=curitem=i;
		curmenuitem=mitem;
		//drawitem(c);        // draw off old ccuritem
		//drawitem(curitem);  // draw on curitem==ccuritem
		needtodraw|=2;
	}
}

//! Left button down.
/*! If TREESEL_OUT_CLICK_DESTROYS is set, then app->destroywindow(this) when clicking
 * down outside the window.
 */
int TreeSelector::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
//	if (x<0 || x>=win_w || y<0 || y>=win_h) {
//		if (menustyle&TREESEL_OUT_CLICK_DESTROYS) {
//			curitem=-1;
//			curmenuitem=NULL;
//			send(d->id);
//			app->destroywindow(this);
//			return 0;
//		}
//	}
	int detailhover=-1;
	if (columns.n && y<textheight) {
		 //check for clicking on column dividers to resize
		 // *** what about title?
		for (int c=columns.n-1; c>0; c--) {
			if (x>columns.e[c]->pos-10 && x<columns.e[c]->pos+10) {
				detailhover=c;
				break;
			}
		}
		
	}
	DBG cerr <<"-----detailhover: "<<detailhover<<endl;
	int onsub=0;
	int item=findItem(x,y, &onsub, NULL);
	buttondown.down(d->id,LEFTBUTTON,x,y,item,detailhover);
	mousedragmode=0;
	return 0;
}

//! Left button up.
/*! If clicked on an arrow, then shift screen. If on item, then addselect(that item).
 * Send the control message if TREESEL_SEND_ON_UP is set.
 * 
 * \todo *** lbdown/up no drag on an already selected item should initiate EDIT_IN_PLACE
 *
 * \todo *** mousedragmode==2 means a rearrangement might be required
 */
int TreeSelector::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int hovered=-1, item=-1;
	int dragamount=buttondown.up(d->id,LEFTBUTTON, &item,&hovered);
	
	if (mousedragmode==2) { //*** ==2 means was attempting to rearrange..
		//***turn off any dragging mode
		mousedragmode=0;
		return 0;
	}
	
	 // do nothing if mouse is outside window.. This is necessary because of grabs.
	if (x<0 || x>=win_w || y<0 || y>=win_h) {
		return 0;
	}

	int onsub=0;
	int i=findItem(x,y,&onsub, NULL);
	if (i<0 || i>=numItems()) return 0; //if not on any item or arrow
	
	if (mousedragmode==2) {
		// *** dragging to rearrange
		return 0;
	}

	 // clicked on already selected item
	if (dragamount<7) {
		const MenuItem *ii=Item(i);

		if (onsub && ii->hasSub()) {
			if (ii->isOpen()) Collapse(i);
			else Expand(i);
			arrangeItems();
			return 0;

		} else if (menustyle&TREESEL_EDIT_IN_PLACE) {
			editInPlace(i);
			return 0; 
		}
	}

	addselect(i,state);
	if (menustyle&TREESEL_SEND_ON_UP) send(d->id);
	
	return 0;
}

//! Find the index of the item at window coordinates (x,y).
/*! The value returned is not necessarily an actual element index. It could be -1, or >=menuitems.n.
 */
int TreeSelector::findItem(int x,int y, int *onsub, int *column)
{
	x-=offsetx;
	y-=offsety;
	int s=0,e=visibleitems.n()-1, m;
	
	MenuItem *i;
	int which=-1;
	while (e>=s) {
		i=item(s);
		if (y<i->y) return -1;
		if (y>=i->y && y<=i->y+i->h) { which=s; break; }

		i=item(e);
		if (y>=i->y+i->h) return e+1;
		if (y>=i->y && y<=i->y+i->h) { which=e; break; }

		m=(e+s)/2;
		if (m==e || m==s) return -1;

		i=item(m);
		if (y>=i->y && y<=i->y+i->h) { which=m; break; }

		if (y<i->y) { s++; e=m-1; }
		else { s=m+1; e--; }
	}

	if (which<0) return -1;
	i=item(which);

	int indent=0;
	MenuItem *ii=i;
	while (ii && ii->parent) {
		ii=ii->parent->parent;
		indent++;
		//DBG cerr <<"for item "<<which<<" indent:"<<indent<<endl;
	}
	indent--;

	int col=0;
	if (columns.n) {
		for (int c=0; c<columns.n; c++) {
			if (x>=columns.e[c]->pos && x<columns.e[c]->pos+columns.e[c]->width) {
				col=c;
				break;
			}
		}	
		if (col==tree_column) {
			if (x-columns.e[col]->pos > indent*iwidth && x-columns.e[col]->pos < (indent+1)*iwidth) *onsub=1;
			else *onsub=0;
		}
	} else {
		if (x<i->x) *onsub=1; else *onsub=0;
		if (x>indent*iwidth && x<(indent+1)*iwidth) *onsub=1;
	}


	if (column) {
		 //search for position on column position sliders
		if (columns.n) {
			 // *** what about title?
			for (int c=0; c<columns.n; c++) {
				if (x>columns.e[c]->pos-10 && x<columns.e[c]->pos+10) {
					*column=c;
					break;
				}
			}
			
		} else *column=0;
	}

	return which;
}

//! Set up the edit in place mode.
/*! \todo *** imp me!
 */
void TreeSelector::editInPlace(int which)
{
	cerr <<" *** editInPlace not implemented!"<<endl;
}

//! Right button and drag drags the screen around (with potential autoscrolling)
/*! 
 * \todo perhaps someday have a hook so that right click on an item calls up some
 *   menu? like copy to, delete item, etc..
 */
int TreeSelector::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id,RIGHTBUTTON, x,y);
	//if (buttondown==RIGHTBUTTON) timerid=app->addmousetimer(this);
	
	return 0;
}

//! Nothing but remove tag from buttondown.
int TreeSelector::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,RIGHTBUTTON)) return 1;
	buttondown.up(d->id,RIGHTBUTTON);
	//if (!buttondown) app->removetimer(timerid);
	return 0;
}

//! Left might select depending on style. Right button drags. +R fast drags.
/*! \todo *** should implement speedy shifting with shift/cntl-RB drag
 *
 * \todo *** also, if right-click shifting, and is off screen, do a sort of
 * auto scrolling, like if mouse above, then move if mouse moving up,
 * but not if mouse is moving down?? or on timer?
 */
int TreeSelector::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	int mx,my;
	int hover=-1;
	buttondown.getextrainfo(d->id,LEFTBUTTON, NULL, &hover);
	buttondown.move(d->id, x,y, &mx,&my);
	//DBG cerr <<"mx,my:"<<mx<<','<<my<<" x,y:"<<x<<','<<y<<endl;

	int onsub;
	int i=findItem(x,y, &onsub, NULL);
	DBG cerr <<"tree found item "<<i<<", onsub:"<<onsub<<endl;

	if (!buttondown.any()) {
		 // do nothing if mouse is outside window..
		 // This is a necessary check because of grabs.
		if (x<0 || x>=win_w || y<0 || y>=win_h) return 0;
			
		if (i<0) i=0;
		if (i>=numItems()) i=numItems()-1;
		if (i!=ccuritem) {
			//*** must imp. move screen only if mouse move is right direction

			if (menustyle&(TREESEL_CURSSELECTS|TREESEL_FOLLOW_MOUSE)) addselect(i,state);
			else {
				//int tm=ccuritem;
				ccuritem=i;
				//drawitem(tm); //drawitem needs the current ccuritem, not old
				//drawitem(ccuritem);
				needtodraw|=2;
			}
		}
		return 0;
	}
	if (buttondown.isdown(d->id,LEFTBUTTON)) {
		if (hover>=0) {
			for (int c=hover; c<columns.n; c++) {
				columns.e[c]->pos+=x-mx;
			}
			for (int c=0; c<columns.n-1; c++) {
				columns.e[c]->width=columns.e[c+1]->pos-columns.e[c]->pos;
			}
			needtodraw=1;
			return 0;
		}
		if (mousedragmode==0) { 
			if (menustyle&TREESEL_REARRANGEABLE) {
				//***if any items selected, then drag them...
				mousedragmode=2;
			} else if (x!=mx || y!=my) mousedragmode=1;
		} else if (mousedragmode==2) {
			//*** dragging some items is already inprogress...
		}
		return 0;
	}

	if (!buttondown.isdown(d->id,RIGHTBUTTON)) return 1;
	 //so right button is down, need to scroll
	int m=1;
	if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) m=20;
	else if ((state&LAX_STATE_MASK)==ControlMask || (state&LAX_STATE_MASK)==ShiftMask) m=10;
	
	if (y-my>0) { 
		movescreen(0,-m*(y-my));
		if (y>0 && y<win_h) { mx=x; my=y; }
	} else if (my-y>0) {
		movescreen(0,m*(my-y));
		if (y>0 && y<win_h) { mx=x; my=y; }
	}
	return 0;
}

//! Autoscroll if necessary**** todo
/*! \todo *** must autoscroll when mouse over arrow and FOLLOW_MOUSE
 */
int TreeSelector::Idle(int tid)
{ // ***
//	if (tid!=timerid) return 1;
//	
//	 // autoscroll
//	if (buttondown==RIGHTBUTTON) {
//		if (my<0) movescreen(0,my));
//		else if (my>win_h) movescreen(0,my-win_h);
//	}
	return 0;
}

//! Make sure that ccuritem is visible by shifting screen so it is.
/*! Returns 1 if x shifted, 2 if y, 3 if both.
 */
int TreeSelector::makeinwindow()
{
	IntRectangle itemrect;
	if (!findRect(ccuritem,&itemrect)) return 0;

	int dx,dy;
	dx=dy=0;
	if (itemrect.x<inrect.x) dx=inrect.x-itemrect.x;
	else if (itemrect.x+itemrect.width>inrect.x+inrect.width)
		dx=(inrect.x+inrect.width)-(itemrect.x+itemrect.width);
	if (itemrect.y<inrect.y) dy=itemrect.y-inrect.y;
	else if (itemrect.y+itemrect.height>inrect.y+inrect.height)
		dy=(itemrect.y+itemrect.height)-(inrect.y+inrect.height);
	needtodraw=1;

	return panner->Shift(1,dx)|panner->Shift(2,dy);
}

//! Try to move the screen by dx pixels and dy pixels. 
/*! Also resets ccuritem so that it refers to an item that is
 * actually visible.
 *
 * Returns whether shifting occured. 1=x, 2=y, 3=x and y
 */
int TreeSelector::movescreen(int dx,int dy)
{
	DBG cerr <<" ccuritem before: "<<ccuritem<<endl;

	long c=panner->Shift(1,dx);
	c|=panner->Shift(2,dy);
	if (!c) return 0; // no shift occured

	offsetx=inrect.x-panner->GetCurPos(1);
	offsety=inrect.y-panner->GetCurPos(2);

	if (menustyle&TREESEL_USE_TITLE) offsety+=textheight;
	if (columns.n) offsety+=textheight;
	
	 // make ccuritem on screen somewhere
	 // by modifying ccuritem (not the same as makeinwindow!)
	IntRectangle itemrect;
	if (!findRect(ccuritem,&itemrect)) return 0;

	 // find an item that is on screen
	MenuItem *i=NULL;
	if (itemrect.y<inrect.y) { 
		 //search below
		for (int ii=ccuritem; ii<numItems(); ii++) {
			i=item(ii);
			if (offsety+i->y >= inrect.y) { 
				ccuritem=ii;
				break;
			}
		}
	} else if (itemrect.y+itemrect.height>inrect.y+inrect.height) {
		 //search above
		for (int ii=ccuritem; ii>=0; ii--) {
			i=item(ii);
			if (offsety+i->y+i->h <= inrect.y+inrect.height) { 
				ccuritem=ii;
				break;
			}
		}
	}
	
	if (ccuritem>=numItems()) ccuritem=numItems();
	needtodraw=1;
	DBG cerr <<" ccuritem after: "<<ccuritem<<endl;
	return c;
}

//! Scroll screen down.
/*! Control OR Shift mask shifts by whole page lengths.
 * Shift AND Control shifts by 3 times whole page lengths.
 */
int TreeSelector::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&ControlMask && state&ShiftMask) movescreen(0,-3*pagesize*(textheight+leading));
	else if (state&ControlMask || state&ShiftMask) movescreen(0,-pagesize*(textheight+leading)); 
	else movescreen(0,-(textheight+leading));
	needtodraw=1;
	return 0;
}

//! Scroll screen up.
/*! Control OR shift mask shifts by whole page lengths.
 * Shift AND Control shifts by 3 times whole page lengths.
 */
int TreeSelector::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&ControlMask && state&ShiftMask) movescreen(0,3*pagesize*(textheight+leading));
	else if (state&ControlMask || state&ShiftMask) movescreen(0,pagesize*(textheight+leading)); 
	else movescreen(0,(textheight+leading));
	needtodraw=1;
	return 0;
}

//! Called same for ScrolledWindow, then arrangeItems.
/*! Has the effect of resizing scrollers, and remapping inrect and outrect.
 */
void TreeSelector::syncWindows(int useinrect)//useinrect=0
{
	ScrolledWindow::syncWindows(useinrect);
	arrangeItems();
	needtodraw=1;
}

//! Set the new line height and leading.
/*! textheight is set to ntotalheight-newleading.
 * If forcearrange==1 then also syncWindows() which also causes adjustinrect() 
 * and arrangeItems() to be called.
 * If forcearrange==2, then do not
 * call syncWindows, instead call arrangeItems(0).
 * If forcearrange==3, then call arrangeItems(1). This last one is when you are
 * seeking setup in preparation for wrapping a window's boundaries to the items
 * extent.
 *
 * Usually from outside the class,
 * a program would just call SetLineHeight(h,l), and this would call arrangeItems().
 * init() needs to set some other stuff before arranging, so it calls with forcearrange==0.
 */
void TreeSelector::SetLineHeight(int ntotalheight,int newleading, char forcearrange)
{
	 // **** change font size???
	if (ntotalheight<=0 || newleading<0) return;
	textheight=ntotalheight-newleading;
	iwidth=textheight;
	leading=newleading;
	padg=textheight/2;
	if (forcearrange==1) syncWindows(); //syncwindows calls arrangeItems...
	else if (forcearrange==2) arrangeItems();
	else if (forcearrange==3) arrangeItems();
	needtodraw=1;
	//*** flush any selected items?
}
	
//! Set outrect to be the window minus space for title.
void TreeSelector::findoutrect()
{
	outrect.x=outrect.y=0;
	if (menustyle&TREESEL_USE_TITLE && menu->title) outrect.y=textheight+pad;
	if (columns.n) outrect.y=textheight+pad;
	outrect.width= win_w-outrect.x;
	outrect.height=win_h-outrect.y;

	if (searchfilter) {
		outrect.y+=textheight+pad;
		outrect.height-=textheight+pad;
	}
}

//! Remove the pads from inrect.
/*! This is called last thing from ScrolledWindow::syncWindows(), where inrect is set to be within scrollers.
 *  Also sets pagesize=(inrect.height+leading)/(textheight+leading)
 *
 *  This and arrangeItems are the only places where pagesize gets set.
 */
void TreeSelector::adjustinrect()
{
	inrect.x+=pad; 
	inrect.width-=2*pad;
	inrect.y+=pad;
	inrect.height-=2*pad;

	if (textheight+leading) pagesize=inrect.height*.75;
}

//! This is meant to be called when the window is going, but you just added or removed a bunch of stuff.
void TreeSelector::Sync()
{
	arrangeItems();
	needtodraw=1;
}


//! Calls ScrolledWindow::MoveResize(nx,ny,nw,nh).
int TreeSelector::MoveResize(int nx,int ny,int nw,int nh)
{
	ScrolledWindow::MoveResize(nx,ny,nw,nh);
	return 0;
}

//! Calls ScrolledWindow::Resize(nw,nh).
int TreeSelector::Resize(int nw,int nh)
{
	ScrolledWindow::Resize(nw,nh);
	return 0;
}

} // namespace Laxkit

