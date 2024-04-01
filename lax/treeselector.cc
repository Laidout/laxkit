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
//    Copyright (C) 2012-2014 by Tom Lechner
//


#include <lax/treeselector.h>
#include <lax/laxutils.h>
#include <lax/language.h>
#include <lax/utf8utils.h>

#include <cctype>


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


TreeSelector::ColumnInfo::ColumnInfo(const char *ntitle, int ntype, int whichdetail, int nwidth, int nwtype)
{
	type = ntype;  // uses ColumnInfoType
	if (type == 0) type = ColumnString;
	title      = newstr(ntitle);
	pos        = 0;
	detail     = whichdetail;
	width      = nwidth;
	width_type = nwtype;
	sort       = 0;
	sort_type  = 0;
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
 * and the selbox (start and end) is inrect minus the pads. However, any shifting changes 
 * the panner's selbox, but this does not trigger any change in inrect.
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
 * If it is a simple move, then mousedragmode==1. If the tree is REARRANGEABLE,
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

	menustyle = nmstyle;
	if (usethismenu) {
		menu = usethismenu;
		menu->inc_count();
	} else {
		menu = new MenuInfo;
	}

	menustyle |= TREESEL_LIVE_SEARCH;
	if (menustyle & TREESEL_STATIC_SEARCH) showsearch = true;
}

void TreeSelector::base_init()
{
	mousedragmode = 0;
	column_gap    = -1;
	pad           = -1;
	padg          = -1;
	offsetx = offsety = 0; //offset of everything from inrect.x,inrect.y,  so (screen item).y = item.y + offsety + inrect.y
	textheight   = 0;
	lineheight   = 0;
	pagesize     = 0;
	ccuritem = curitem = 0;
	ccurdetail   = -1;
	ccurflag     = -1;
	curmenuitem  = NULL;
	timerid      = 0;
	senddetail   = -1;
	leading      = 0;
	menustyle    = 0;
	needtobuildcache = 1;
	iwidth       = 10;
	iconwidth    = 0; //default width of icon, even if icon missing, as fraction of textheight, so 1 == textheight
	firsttime    = 1;
	flag_width   = 1;
	dragflag     = -1;
	flagdef      = nullptr; //string of byte pairs, like "eElL", checked in ToggleFlag()

	searchfilter = NULL;
	showsearch   = 0;

	tree_column  = 0; //the column in which to position the tree lines
	sort_detail  = -1;
	sort_descending = 0;
	sort_dirs_first = false;

	InstallColors(THEME_Panel);

	menu = NULL;
}

//! Destructor, increment count on menu.
TreeSelector::~TreeSelector()
{
	if (menu) menu->dec_count();
	if (searchfilter) delete[] searchfilter;
	delete[] flagdef;

	DBG visibleitems.Flush();
}

/*! newvalue==0 is off, anything else is on.
 * Returns the state afterwards.
 */
bool TreeSelector::SetStyle(unsigned long long style, int newvalue)
{
	if (newvalue) menustyle |= style;
	else menustyle &= ~style;
	return menustyle & style;
}

bool TreeSelector::HasStyle(unsigned long long style)
{
	return menustyle & style;
}

int TreeSelector::InstallMenu(MenuInfo *nmenu)
{
	if (menu != nmenu) {
		if (menu) menu->dec_count();
		menu = nmenu;
		if (nmenu) nmenu->inc_count();
		else menu = new MenuInfo;
	}

	needtobuildcache = 1;
	selection.flush();
	ccuritem = curitem = 0;
	ccurdetail = -1;
	ccurflag   = -1;
	arrangeItems();
	RemapColumns();
	needtodraw = 1;
	return 0;
}

void TreeSelector::dump_out(FILE *f,int indent,int what,DumpContext *savecontext)
{
    Attribute att;
    dump_out_atts(&att,what,savecontext);
    att.dump_out(f,indent);
}

Attribute *TreeSelector::dump_out_atts(Attribute *att,int what,DumpContext *savecontext)
{
	if (!att) att = new Attribute();

	anXWindow::dump_out_atts(att,what,savecontext);

    if (what == -1) {
		att->push("columns", "#A list of positions of the columns. Each entry is the name and displayed width of the column");
        return att;
    }

	if (columns.n) {
		Attribute *col=att->pushSubAtt("columns");
		for (int c=0; c<columns.n; c++) {
			col->push(columns.e[c]->title,  columns.e[c]->width);
		}
	}

	return att;
}

void TreeSelector::dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext)
{
	anXWindow::dump_in_atts(att,flag,loadcontext);

	char *name;
	char *value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"columns")) {
			 //each line is (column name) (column width)
			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name  = att->attributes.e[c]->attributes.e[c2]->name;
				value = att->attributes.e[c]->attributes.e[c2]->value;

				for (int c3=0; c3<columns.n; c3++) {
					if (!strcmp(columns.e[c3]->title,name)) {
						columns.e[c3]->width = strtol(value, NULL, 10);
						break;
					}
				}
			}
		}
	}
}


//! Focus on draws the char over item.
int TreeSelector::FocusOn(const FocusChangeData *e)
{
	int c = anXWindow::FocusOn(e);
	return c;
}

int TreeSelector::Event(const EventData *e,const char *mes)
{
	DBG cerr << "TreeSelector::Event "<<(mes?mes:"noname")<<endl;

    if (e->type == LAX_onMapped && (menustyle & TREESEL_GRAB_ON_MAP)) {
        app->setfocus(this,0,NULL);

	} else if (!strcmp(mes, "pan change")) {
		offsetx = -panner->GetCurPos(1);
		offsety = -panner->GetCurPos(2);
		needtodraw = 1;
		return 0;
	}

	return ScrolledWindow::Event(e,mes);
}

//! Focus off draws the char over item.
/*! Also, if TREESEL_FOCUS_OFF_DESTROYS, then an off focus destroys this window.
 */
int TreeSelector::FocusOff(const FocusChangeData *e)
{
	DBG cerr <<"TreeSelector "<<WindowTitle()<<" FocusOff..."<<endl;

	if (HasStyle(TREESEL_FOLLOW_MOUSE)) {
		addselect(-1, 0);
	}

	int c = anXWindow::FocusOff(e);

//	if (!win_active) {
//		if (menustyle&TREESEL_DESTROY_ON_FOCUS_OFF) {
//			app->destroywindow(this);
//			return 0;
//		}
//		drawitem(ccuritem);
//	}
	return c;
}

/*! Return the number in selection [0..NumSelected] corresponding to
 * the screen index i, or -1 if not found.
 */
int TreeSelector::GetSelectedIndex(int i)
{
	if (i<0 || i>=visibleitems.n()) return -1;

	MenuItem *item=visibleitems.e(i);
	for (int c=0; c<selection.n; c++) {
		if (item==selection.e[c]) return c;
	}
	return -1;
}

//! Return the ith selelected item. i must be in range [0..NumSelected()-1].
MenuItem *TreeSelector::GetSelected(int i)
{
	if (i<0 || i>=selection.n) return NULL;
	return selection.e[i];
}

//! Return the ith selelected item. i must be in range [0..NumSelected()-1].
MenuItem *TreeSelector::GetItem(int i)
{
	if (i<0 || i>=visibleitems.n()) return NULL;
	return visibleitems.e(i);
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
int TreeSelector::Select(int which, bool replace_selection)
{
	if (which<0 || which>=numItems()) return curitem;
	if (item(which)->state&LAX_ON) item(which)->state^=(LAX_ON|LAX_OFF);
	addselect(which, replace_selection ? 0 : ControlMask);
	makeinwindow();
	return curitem;
}

int TreeSelector::Select(const char *name, bool replace_selection)
{
	int which = visibleitems.findIndex(name);
	return Select(which, replace_selection);
}

int TreeSelector::SelectId(int id)
{
	int which=visibleitems.findIndex(id);
	if (which<0 || which>=numItems()) return curitem;

	if (item(which)->state&LAX_ON) item(which)->state^=(LAX_ON|LAX_OFF);
	addselect(which,0);
	makeinwindow();
	return curitem;
}

int TreeSelector::SelectAtPath(const char **path, int n, bool replace_selection)
{
	if (n <= 0) return 1;
	if (replace_selection) DeselectAll();

	MenuInfo *info = Menu();
	for (int c=0; c<n; c++) {
		// *** 1. expand at this level
		int i = info->findIndex(path[c]);
		MenuItem *item = info->e(i);
		if (!item) {
			DBG cerr << " *** can't find "<<path[c]<<"!!! this shouldn't happen"<<endl;
			return 1;
		}

		if (item->hasSub()) item->Open();
		
		// *** 2. if at final, select item
		if (c == n-1) {
			item->SetState(LAX_ON|MENU_SELECTED, true);
			selection.pushnodup(item, 0);
		} else {
			info = item->GetSubmenu(0);
		}
	}

	RebuildCache();
	return 0;
}


/*! Return 0 for success, or nonzero error.
 */
int TreeSelector::Deselect(int which)
{
	if (which < 0 || which >= visibleitems.how_many(0)) return 1;
	MenuItem *i = item(which);
	i->state &= ~(MENU_SELECTED|LAX_ON|LAX_OFF);
	i->state |= LAX_OFF;
	selection.remove(selection.findindex(i));
	needtodraw = 1;
	return 0;
}

int TreeSelector::DeselectAll()
{
	int n = selection.n;
	for (int c = selection.n-1; c>=0; c--) { // turn off all the ones that are on
		MenuItem *item = selection.e[c];
		item->state &= ~(LAX_ON|LAX_OFF|MENU_SELECTED); 
		item->state |= LAX_OFF; 
		selection.remove(c);
	}
	needtodraw = 1;
	return n;
}

//! Expand which visible item. If already expanded, then do nothing.
/*! Return 1 for expanded, 0 for can't or error.
 */
int TreeSelector::Expand(int which)
{
	if (which<0 || which>=visibleitems.how_many(0)) return 0;
	visibleitems.menuitems.e[which]->state|=MENU_OPEN;
	needtobuildcache=1;
	needtodraw=1;
	return 1;
}

/*! Deselects any that are in submenus.
 */
int TreeSelector::Collapse(int which)
{
	if (which<0 || which>=visibleitems.how_many(0)) return 0;

	MenuItem *parent=item(which);
	for (int c=selection.n-1; c>=0; c--) {
		if (selection.e[c]->hasParent(parent)) {
			selection.e[c]->state &= ~(LAX_ON|LAX_OFF|MENU_SELECTED);
			selection.e[c]->state |= LAX_OFF;
			selection.remove(c);
		}
	}

	parent->state &= ~MENU_OPEN;
	needtobuildcache = 1;
	needtodraw = 1;
	return 1;
}


//! Make visible item with index which be near window coordinate x,y
/*! Returns 0 for success, or 1 for bad item.
 */
int TreeSelector::SetFirst(int which,int x,int y)
{
	if (which<0 || which>=visibleitems.how_many(0)) return 1;

	MenuItem *i = item(which);
	movescreen(0, y - (i->y+i->w/2));
	needtodraw = 1;
	return 0;
}


void TreeSelector::UIScaleChanged()
{
	anXWindow::UIScaleChanged(); //this sends same to children
	needtodraw = 1;

	double scale = UIScale();
	double scale_factor = scale * win_themestyle->normal->textheight() / textheight;
	leading = 1 * scale;
	textheight = scale * win_themestyle->normal->textheight();
	iwidth = textheight;
	padg = textheight/2;
	pad *= scale_factor;

	RebuildCache();
}


//! Set some values that are derived from other values (pagesize, highlight, shadow, ...).
/*! Sets the shadow and highlight colors, and determines the pagesize.
 */
int TreeSelector::init()
{
	ScrolledWindow::init(); //this calls syncWindows, causing inrect and pagesize to get set
    highlight = coloravg(win_themestyle->bg,rgbcolor(255,255,255));
	shadow    = coloravg(win_themestyle->bg,rgbcolor(0,0,0));
	
	double scale = UIScale();
	leading = 1 * scale;
	if (textheight <= 0) textheight = scale * win_themestyle->normal->textheight() + leading;
	iwidth = textheight;
	
	DBG cerr <<"--"<<WindowTitle()<<": textheight, leading="<<textheight<<','<<leading<<endl;
	
	RemapColumns();

	padg = textheight/2;
	if (pad < 0) pad = textheight/2;
	pagesize = inrect.height*.75;
	arrangeItems();
	return 0;
}


/*! Add a new column for details of items.
 *
 * If whichdetail<0, then use columns.n (before pushing).
 * If ntype<=0, use ColumnString.
 *
 * If nodup, then if the named column exists already, replace its old details with those provided.
 */
int TreeSelector::AddColumn(const char *i,LaxImage *img,int width,int width_type, int ntype, int whichdetail, bool nodup, int sort_override)
{
	if (whichdetail < 0) whichdetail = columns.n;
	if (ntype <= 0) ntype = ColumnInfo::ColumnString;

	int c;
	for (c=0; c<columns.n; c++) {
		if (!strcmp(columns.e[c]->title,i) && nodup) {
			 //column existed
			columns.e[c]->width  = width;
			columns.e[c]->detail = whichdetail;
			columns.e[c]->type   = ntype;
			if (sort_override >= 0) columns.e[c]->sort_type = sort_override;
			break;
		}
	}
	
	if (c == columns.n) {
		columns.push(new ColumnInfo(i, ntype, whichdetail, width,width_type),1);
		if (sort_override >= 0) columns.e[c]->sort_type = sort_override;
	}

	return 0;
}

/*! Fill in a default width for any column that has width<=0.
 * Adjusts each column->pos to be after the width of previous columns.
 */
void TreeSelector::RemapColumns()
{
	if (!columns.n) return;

	int    totalwidth = 0;
	double pos        = 0;
	int    nfills     = 0;
	
	for (int c=0; c<columns.n; c++) {
		if (columns.e[c]->width <= 0) {
			columns.e[c]->width = findColumnWidth(c);
			if (columns.e[c]->width_type == 1) nfills++;
		}
		columns.e[c]->pos = pos;
		pos += columns.e[c]->width;
		totalwidth += columns.e[c]->width;
	}

	if (nfills) {
		pos = 0;
		double fill = (inrect.width-totalwidth)/(float)nfills;
		if (fill < 0) fill = 0;

		if (fill) for (int c=0; c<columns.n; c++) {
			columns.e[c]->pos = pos;
			if (columns.e[c]->width_type == 1) {
				columns.e[c]->width += fill;
			}

			pos += columns.e[c]->width;
		}
	}

	needtodraw = 1;
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

/*! Sort with current settings.
 */
void TreeSelector::Sort()
{
	if (!menu) return;

	menu->SetCompareFunc(menu->sortstyle);
	// visibleitems.SetCompareFunc(menu->sortstyle);
	menu->Sort(sort_detail);
	RebuildCache();
	needtodraw=1;
}

int TreeSelector::ShowSearch(bool on)
{
	showsearch = on;
	needtodraw=1;
	return showsearch;
}

int TreeSelector::ClearSearch()
{
	if (!menu) return 1;
	menu->ClearSearch();
	makestr(searchfilter, NULL);
	needtobuildcache=1;
	needtodraw=1; 
	return 0; 
}

/*! Update the search term. If isprogressive, then update assuming that
 * we are merely narrowing the search of existing visible items.
 * If searchterm is blank, then remove searchfilter.
 */
int TreeSelector::UpdateSearch(const char *searchterm, bool isprogressive)
{
	if (!menu) return 1;
	if (searchterm && !strcmp(searchterm, " ")) return 1; //don't allow starting a search with a space

	makestr(searchfilter, searchterm);

	if (!isprogressive) menu->ClearSearch();
	menu->Search(searchterm, isprogressive, 1);
	makestr(searchfilter, searchterm);

	if (HasStyle(TREESEL_LIVE_SEARCH)) {
		if (isblank(searchfilter)) { //maybe it was "   "
			delete[] searchfilter;
			searchfilter=NULL;
			if (!HasStyle(TREESEL_STATIC_SEARCH)) showsearch = false;

		} else showsearch = true;
		findoutrect();
		inrect = outrect;
		adjustinrect();
	}

	 //update curitem
	MenuItem *sitem = item(curitem);
	curitem    = -1;
	ccuritem   = 0;
	ccurdetail = -1;
	ccurflag   = -1;

	if (sitem && !sitem->hidden()) {
		for (int c=0; c<visibleitems.n(); c++) {
			if (visibleitems.e(c)->hidden()) continue;
			if (sitem == visibleitems.e(c)) {
				curitem = c;
				break;
			}
		}
	}

	if (curitem < 0 && HasStyle(TREESEL_FOLLOW_MOUSE) && visibleitems.n()>0) {
		ccuritem = 0;
		addselect(ccuritem,0);
	}

	needtobuildcache = 1;
	needtodraw = 1;
	return 0;
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
	menu->AddItem(i,nid,0, img, -1, newstate);
	needtobuildcache=1;
	return numItems();
}

double TreeSelector::getgraphicextent(MenuItem *mitem, double *w, double *h)
{
    if (!mitem || !mitem->image) {
    	if (iconwidth > 0) {
    		*w = *h = iconwidth * UIScale() * win_themestyle->normal->textheight();
    	} else {
	        *w = 0;
	        *h = 0;
	    }
        return 0;
    }
    *w = mitem->image->w();
    *h = mitem->image->h();
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
	if (visibleitems.n() == 0) RebuildCache();

	if (e >= 0 && e < s) return -1;
	if (s < 0) s = 0;
	if (s >= numItems()) s = numItems()-1;
	if (e < 0 || e >= numItems()) e = numItems()-1;
	if (s < 0 || e < 0) return 0;
	double w=0, h=0, ww=0, hh=0,t;
	double scale = UIScale();
	MenuItem *mitem;

	if (pad < 0) pad = textheight * .2;

	for (int c = s; c <= e; c++) {
		mitem = item(c);
		getgraphicextent(mitem, &ww, &hh);
		if (hh > h) h = hh;
		if (ww) t = ww+padg; else t = 0;
		t += 2*pad + scale * win_themestyle->normal->Extent(mitem->name,-1);
		if (t > w) w = t;
	}
	w += iwidth;

	if (h_ret) *h_ret = h;
	return w;
}

void TreeSelector::Wrap()
{
	int h = 0;
	int w = findmaxwidth(0,-1,&h);

	win_w = w;
	win_h = h;
}

/*! Return maximum actual width for which column.
 */
int TreeSelector::findColumnWidth(int which)
{
	int s = 0;
	int e = numItems()-1;
	if (s<0 || e<0) return 0;

	double w=0,h=0,ww=0,hh=0,t;
	double scale = UIScale();
	MenuItem *mitem;

	if (pad < 0) pad = textheight * .2;

	if (columns.e[which]->title) w = 2*pad + scale * win_themestyle->normal->Extent(columns.e[which]->title,-1);

	int col;
	for (int c=s; c<=e; c++) {
		mitem = item(c);
		col = which;
		while (mitem && col) {
			mitem = mitem->nextdetail;
			col--;
		}
		if (!mitem) continue;

		getgraphicextent(mitem,&ww,&hh);
		if (hh > h) h = hh;
		if (ww) t = ww + padg; else t = 0;
		t += 2*pad + scale * win_themestyle->normal->Extent(mitem->name,-1);
		if (t > w) w = t;
	}
	w += iwidth;

	return w;
}

//! Basically RebuildCache(), then update the panner.
void TreeSelector::arrangeItems()
{
	RebuildCache();

	IntRectangle wholerect; //rectangle around all items. 0,0 is 0,0 for first item
	wholerect.x = wholerect.y = 0;
	wholerect.width = findmaxwidth(0,-1,NULL);

	if (visibleitems.menuitems.n == 0) return;

	MenuItem *item = visibleitems.menuitems.e[visibleitems.menuitems.n-1];
	wholerect.height = item->y + item->h;
	IntRectangle selbox = inrect; //inrect has window coordinates. selbox is screen area mapped to wholerect space
	selbox.x -= inrect.x;     //selbox width and height should be same w and h as inrect
	selbox.y -= inrect.y;

	if (selbox.x < wholerect.x) selbox.x = wholerect.x;
	if (selbox.y < wholerect.y) selbox.y = wholerect.y;
	if (selbox.x + selbox.width  > wholerect.x + wholerect.width)  selbox.width  = wholerect.x + wholerect.width -selbox.x;
	if (selbox.y + selbox.height > wholerect.y + wholerect.height) selbox.height = wholerect.y + wholerect.height-selbox.y;

	panner->SetWholebox(wholerect.x,wholerect.x+wholerect.width-1,wholerect.y,wholerect.y+wholerect.height-1);
	panner->SetCurPos(1,selbox.x,selbox.x+selbox.width-1);
	panner->SetCurPos(2,selbox.y,selbox.y+selbox.height-1);

	offsetx = -panner->GetCurPos(1);
	offsety = -panner->GetCurPos(2);
}

/*! Update visibleitems.
 * Called from Refresh() if needtobuildcache!=0.
 * Note that this does not update the panner.
 * Use arrangeItems() to both RebuildCache() and update panner.
 */
int TreeSelector::RebuildCache()
{
	visibleitems.Flush();
	addToCache(0, menu, 0);
	needtobuildcache = 0;

	if (ccuritem >= numItems()) {
		ccuritem    = curitem  = numItems()-1;
		ccurdetail  = ccurflag = -1;
		curmenuitem = item(ccuritem);
		makeinwindow();
	}

	return 0;
}

//! Add items to visibleitems stack. Called from RebuildCache.
/*! Note the first item gets assigned x,y at 0,0, NOT actual window coordinates.
 * Items get coordinates relative to the top left of a rectangle that encloses all the items.
 * Items get width and height corresponding to extents according to getitemextent(),
 * which by default is the extent of the text+icon.
 */
int TreeSelector::addToCache(int indent,MenuInfo *mmenu, int cury)
{
	double xx = 0;
	double ww,hh, hhh;
	MenuItem *i, *ii;

	for (int c=0; c<mmenu->n(); c++) {
		i = mmenu->e(c);
		if (i->hidden() == 1) continue;
	
		hh = 0;
		ii = i;
		while (ii) {
			getitemextent(ii, &ww,&hhh, NULL,NULL);
			if (hhh == 0) hhh = textheight;
			ii->x = xx; //note: x will not be accurate for details
			ii->y = cury;
			ii->w = ww;
			ii->h = hhh;

			if (hhh > hh) hh = hhh; // *** really should only compute for visible, used columns
			ii = ii->nextdetail;
		}
		i->h = hh;
		visibleitems.AddItemAsIs(i,0);

		cury += hh + 1;

		 //draw any subitems and connecting lines
		if (i->hasSub()) {
			if (i->isOpen()) {
				 //item is open submenu
				cury = addToCache(indent+1,i->GetSubmenu(),cury);
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
	double gw, gh;
	getgraphicextent(mitem, &gw, &gh);
	double ww = UIScale() * win_themestyle->normal->Extent(mitem->name,-1,w,h,NULL,NULL);
	if (h) *h *= UIScale();
	if (w) *w *= UIScale();
	if (menustyle & TREESEL_GRAPHIC_ON_RIGHT) {
		if (tx) *tx = 0;
		if (gx) *gx = (ww ? ww + padg : 0);
	} else {
		if (gx) *gx = 0;
		if (tx) *tx = (gw ? gw + padg : 0);
	}
	if (h && gh > *h) *h = gh;
	if (gw) {
		if (w) *w += padg + gw;
		ww += padg + gw;
	}
	return ww;
}


//! Find screen rectangle item c goes in. Includes pad.
/*! Return 1 for rectangle found, otherwise 0 for unable to find (for instance when called before
 * necessary initialization done).
 */
int TreeSelector::findRect(int c,IntRectangle *itemspot)
{
	MenuItem *i = item(c);
	if (!i) return 0;

	itemspot->x      = i->x + offsetx + inrect.x;
	itemspot->y      = i->y + offsety + inrect.y;
	itemspot->width  = i->w;
	itemspot->height = i->h;

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

	if (firsttime) {
		makeinwindow();
		firsttime = 0;
	}

	//if (needtobuildcache) RebuildCache();
	if (needtobuildcache) arrangeItems();

	if (pad < 0) pad = textheight * .2;
	if (column_gap < 0) column_gap = textheight * .2;
	double scale = UIScale();

	Displayer *dp = MakeCurrent();
	dp->font(win_themestyle->normal, scale * win_themestyle->normal->textheight());
	dp->ClearWindow();
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

	double th = textheight;
	int indent = 0;
	flatpoint offset(offsetx + inrect.x, offsety + inrect.y); //from window 0,0
	int n = 0;
	DrawItems(indent,menu,n,offset);


	 // Draw title if necessary
	int y = 0;
	if (menustyle & TREESEL_USE_TITLE) drawtitle(y);

	 //draw column info
	if (columns.n) {
		dp->NewFG(coloravg(win_themestyle->fg,win_themestyle->bg,.8));
		dp->drawrectangle(0,y, win_w,textheight, 1);
		dp->NewFG(win_themestyle->fg);
		dp->drawline(0,y+textheight, win_w,y+textheight);

		for (int c = 0; c < columns.n; c++) {
			if (c < columns.n-1) {
				dp->drawline(columns.e[c+1]->pos,y, columns.e[c+1]->pos,y+textheight);
			}

			if (isblank(columns.e[c]->title)) continue;
			dp->textout(column_gap+columns.e[c]->pos,y, columns.e[c]->title,-1, LAX_LEFT|LAX_TOP);

			 //draw sort indicator
			if (columns.e[c]->sort>0)      dp->drawthing(column_gap+columns.e[c]->pos+columns.e[c]->width-th, y+th/2, th/4,th/4, 1, THING_Triangle_Down);
			else if (columns.e[c]->sort<0) dp->drawthing(column_gap+columns.e[c]->pos+columns.e[c]->width-th, y+th/2, th/4,th/4, 1, THING_Triangle_Up);
		}
		y += th;
	}

	if (showsearch) {
		dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg, .25));
		dp->drawRoundedRect(inrect.x,y+pad, inrect.width,textheight, textheight/2,0, textheight/2,0, 1, 15);

		dp->NewFG(win_themestyle->fg);
		dp->textout(textheight,y+pad, searchfilter,-1, LAX_LEFT|LAX_TOP);
	}

	SwapBuffers();
	needtodraw = 0;
}

/*! Recursively draw all unhidden items in mmenu.
 * Note that items drawn will be in order of visibleitems, but it uses menu directly, not visibleitems,
 * in order to determine the pattern of nesting lines to draw.
 *
 * Essentially draws a background, then drawItemContents() for that item.
 *
 * offset is screen offset.
 */
int TreeSelector::DrawItems(int indent, MenuInfo *mmenu, int &n, flatpoint offset)
{
	Displayer *dp = GetDisplayer();

	MenuItem *i;
	int yy = 0;
	unsigned long oddcolor = coloravg(win_themestyle->bg,win_themestyle->fg, .05);
	unsigned long linecolor = win_themestyle->fg.Pixel();
	unsigned long col;

	int tree_offset=0; //offset of tree lines due to being in a particular column
	if (columns.n && tree_column!=0 && tree_column<columns.n) {
		tree_offset = columns.e[tree_column]->pos;
	}

	for (int c = 0; c < mmenu->n(); c++) {
		i = mmenu->e(c);
		if (i->hidden() == 1) continue;

		n++;
		yy = i->y + i->h/2; //so yy in center of row

		 //draw background
		if (i->isSelected()) {
			col = win_themestyle->bghl.Pixel();
			if (n%2 == 0) col = coloravg(col, win_themestyle->fg, .05); //slightly darker for odd lines

		} else {
			if (n%2 == 0 && !HasStyle(TREESEL_FLAT_COLOR)) {
				 //draw bg slightly darker
				col = oddcolor;
			} else {
				 //draw bg as bg
				col = win_themestyle->bg.Pixel();
			}
		}

		 //highlight more if is focused item
		if (i == item(ccuritem)) {
			DBG cerr <<" Item "<<n<<" is focused item, making darker"<<endl;
			col = coloravg(col,win_themestyle->fg,.1);
		}
		dp->NewFG(col);
		dp->drawrectangle(offset.x+0,offset.y+i->y, win_w,i->h, 1);


		 //draw small horizontal dash for current item
		dp->NewFG(linecolor);
		if (!HasStyle(TREESEL_NO_LINES) && indent>0) {
			dp->drawline(tree_offset+offset.x+(indent-.5)*iwidth,offset.y+yy, tree_offset+offset.x+indent*iwidth,offset.y+yy);
		}

		 //draw any subitems and connecting lines
		//double suboffset = 0;
		double suboffset = (HasStyle(TREESEL_SUB_ON_RIGHT) ? 0 : iwidth);
		if (i->hasSub()) {
			suboffset = iwidth;
			int suby = yy;

			if (i->isOpen()) {
				 //item is open submenu
				int newy = DrawItems(indent+1,i->GetSubmenu(),n,offset);
				if (!HasStyle(TREESEL_NO_LINES)) {
					dp->drawline(tree_offset+offset.x+(.5+indent)*iwidth, offset.y+yy,
								 tree_offset+offset.x+(.5+indent)*iwidth, offset.y+newy); //vertical line
				}
				yy = newy;
			} else {
				 //item is closed submenu
				//drawSubIndicator(i, tree_offset+offset.x+indent*iwidth,offset.y+yy, i->isSelected());
			}

			if (HasStyle(TREESEL_SUB_ON_RIGHT)) {
				suboffset = 0;
				drawSubIndicator(i, offset.x+inrect.width - iwidth,offset.y+suby, i->isSelected());
				//drawSubIndicator(i, tree_offset+offset.x+indent*iwidth + i->width - iwidth,offset.y+yy, i->isSelected());
			} else {
				drawSubIndicator(i, tree_offset+offset.x+indent*iwidth,offset.y+suby, i->isSelected());
			}
		} else suboffset = iwidth;

		 //finally draw actual item contents in cached area
		drawItemContents(i, offset.x,suboffset,offset.y, 0, (indent+(HasStyle(TREESEL_NO_LINES) ? 0 : 1))*iwidth);
	}

	return yy;
}

//! Draws a submenu indicator centered on x,y, and width=iwidth, height=textheight+leading.
void TreeSelector::drawSubIndicator(MenuItem *mitem,int x,int y, int selected)
{
	if (!mitem->hasSub()) return;

	int arrowtype = 0;
	if (mitem->isOpen()) arrowtype = THING_Triangle_Down;
	else arrowtype = THING_Triangle_Right;

	int subw = iwidth;
	x += subw/2;
	if (subw > (textheight+leading)) subw = (textheight+leading)/3;
	else subw /= 3;

	if (menustyle & TREESEL_SUB_FOLDER) { arrowtype = THING_Folder; }

	drawarrow(x,y, subw, arrowtype);
}

//! Draw the arrows for menus, really just THING_Triangle_Up, Down, Left, Right for submenus.
/*! Centers around x,y within a box of side 2*r.
 * This function is also used for the up and down arrows when the menu contents 
 * extend beyond the screen.
 */
void TreeSelector::drawarrow(int x,int y,int r,int type)
{
	Displayer *dp = GetDisplayer();
	dp->NewFG(win_themestyle->color1); // inside the arrow
	dp->drawthing(x,y,r,r,1,(DrawThingTypes)type);
	dp->NewFG(win_themestyle->fg); // border of the arrow
	dp->drawthing(x,y,r,r,0,(DrawThingTypes)type);
}

//! Draw the menu title if present. Default is print it out at top of window (not inrect).
void TreeSelector::drawtitle(int &y)
{
	if (!menu || !menu->title) return;
	Displayer *dp = GetDisplayer();
	dp->NewFG(win_themestyle->fg); // background of title
	dp->drawrectangle(0,y+pad, win_w,textheight+leading, 1);
	dp->NewFG(win_themestyle->fg); // foreground of title
	dp->textout(win_w/2,y+pad, menu->title,-1, LAX_CENTER|LAX_TOP);
	y += textheight+leading;
}

/*! i is the head MenuItem (not a detail) of an item in visibleitems. Note that i might not itself be shown,
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
void TreeSelector::drawItemContents(MenuItem *i,int offset_x,int suboffset,int offset_y, int fill, int indent)
{
	IntRectangle itemspot;
	itemspot.x      = i->x+offset_x;
	itemspot.y      = i->y+offset_y;
	itemspot.width  = i->w;
	itemspot.height = i->h;

	 // If itemspot not onscreen, return
	if (itemspot.x+itemspot.width <inrect.x || itemspot.x>inrect.x+inrect.width ||
		itemspot.y+itemspot.height<inrect.y || itemspot.y>inrect.y+inrect.height) {
		//DBG cerr <<"Item "<<c<<" not on screen...."<<endl;
		return;
	}

	Displayer *dp = GetDisplayer();

	if (fill) {
		// ****** not used anymore?? wipes out tree lines...
		 //draw background
		unsigned long color;

		if (i->isSelected()) {
			 //draw bg selected
			color = win_themestyle->bghl.Pixel();
		} else {
			int n = visibleitems.menuitems.findindex(i);

			 //draw bg slightly darker on odd lines
			if (n%2 == 1) color = coloravg(win_themestyle->bg,win_themestyle->fg, .05);
			else color = win_themestyle->bg.Pixel();
		}

		int isccuritem = (i == item(ccuritem));
		if (isccuritem) color = coloravg(color,win_themestyle->fg,.05);
		dp->NewFG(color);
		dp->drawrectangle(offset_x+i->x,offset_y+i->y, inrect.x+inrect.width-(offset_x+i->x),i->h, 1);
	}

	//DBG dp->NewFG(win_themestyle->fg.Pixel());
	//DBG dp->drawrectangle(offset_x+i->x,offset_y+i->y, inrect.x+inrect.width-(offset_x+i->x),i->h, 0);

	//now draw actual content of item
	
	if (i->state & LAX_SEPARATOR) {
		 //draw a separator between edge of tree lines, filling rest of line
		int tree_offset = 0;
		if (columns.n && tree_column!=0 && tree_column<columns.n) {
			tree_offset = columns.e[tree_column]->pos;
		}
		itemspot.x = offset_x+tree_offset+indent;
		itemspot.width = inrect.x+inrect.width-itemspot.x;
		drawsep(i->name,&itemspot);
		return;
	}

	 // Draw the item icon and text for each detail
	int start = 0;
	if (columns.n == 0) start = -1;
	MenuItem *ii;
	for (int c = start; c < columns.n; c++) {
		ii = i;
		if (c >= 0) { //we have columns
			itemspot.x = offset_x + columns.e[c]->pos + (c == tree_column ? suboffset : 0);
			itemspot.width = columns.e[c]->width;
			ii = ii->GetDetail(columns.e[c]->detail);
			if (c == tree_column) {
				itemspot.width -= indent;
				itemspot.x += indent;
			}
			if (columns.e[c]->type == ColumnInfo::ColumnFlags) {
				drawflags(ii, &itemspot);
				ii = NULL;
			}
		} else {
			 //c<0 happens when there are no columns, always remove the line space
			itemspot.width-=indent;
			itemspot.x+=indent;
		}

		if (ii) {
			if (ii->state & LAX_ISTOGGLE) {
				//draw checkmark if ii is a toggle
				double th = dp->textheight();
				if (ii->state & LAX_CHECKED) {
					dp->drawthing(itemspot.x+th/2,itemspot.y+itemspot.height/2, th/3,-th/3, 1, THING_Check);
				}
				itemspot.width -= th;
				itemspot.x += th;
			}
			drawitemname(ii,&itemspot);
		}
	}
}


/*! flags are parsed from mitem->name. There is one flag per byte of mitem->name.
 *
 * Then drawFlagGraphic() is used to draw actual flag.
 */
void TreeSelector::drawflags(MenuItem *mitem,IntRectangle *rect)
{
	const char *f=mitem->name;
	if (!f) return;

	int w = flag_width * rect->height;
	for (unsigned int c=0; c<strlen(f); c++) {
		drawFlagGraphic(f[c], rect->x + c*w, rect->y, w,rect->height);
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
 * Return the new x position.
 */
int TreeSelector::drawFlagGraphic(char flag, int x,int y,int w,int h)
{
	if (flag==' ') return x+h;

	Displayer *dp = GetDisplayer();

	DrawThingTypes thing=THING_Check;
	if      (flag=='l') thing = THING_Unlocked;
	else if (flag=='L') thing = THING_Locked;
	else if (flag=='e') thing = THING_Closed_Eye;
	else if (flag=='E') thing = THING_Open_Eye;

	dp->NewFG(0,0,0);
	if (flag=='e' || flag=='E') dp->NewBG(1.,1.,1.);
	else dp->NewBG(.7,.7,.7);
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->drawthing(x+w/2, y+h/2, w*.4,-w*.4, 2, thing); //draw centered on x, with width h/2

	return x+h;
}

/*! Draw the item icon and name in rect.
 * Assumes mitem is the relevant detail within rect, and that background colors have been drawn already.
 */
void TreeSelector::drawitemname(MenuItem *mitem,IntRectangle *rect)
{
	Displayer *dp = GetDisplayer();

	unsigned long f,g;
	double fasc,tx,gx,iw;
	fasc = UIScale() * win_themestyle->normal->ascent();

	getitemextent(mitem,&iw,NULL,&gx,&tx); // status graphic and name x coordinate

	gx += rect->x; tx+=rect->x; // |blah    |
	
	 // set proper foreground and background colors
	if ((mitem->state & LAX_MSTATE_MASK) > LAX_ON) { // grayed, hidden=0, off=1, on=2, so this is same as>2
		g = win_themestyle->bg.Pixel();
		f = win_themestyle->fggray.Pixel();
	} else {
		g = win_themestyle->bg.Pixel();
		f = win_themestyle->fg.Pixel();
	}

	 // do the actual drawing
	if (mitem->state & LAX_ON) { // draw on
		g = win_themestyle->bghl.Pixel();
		f = win_themestyle->fghl.Pixel();
	} 
	 // add a little extra hightlight if item is ccuritem
	//if (!(menustyle&TREESEL_ZERO_OR_ONE)) if (mitem==item(ccuritem)) g=coloravg(f,g,.85);
	
	//foreground_color(g); // only draw highlighted if not checkboxes
	//fill_rectangle(this, rect->x,rect->y,rect->width,rect->height);


	dp->NewFG(f);
	dp->NewBG(g);
	if (mitem->name) dp->textout(tx,rect->y+fasc+leading/2, mitem->name,strlen(mitem->name), LAX_LEFT|LAX_BASELINE);
	if (mitem && mitem->image) {
		if (app->theme->ui_scale_on_icons) {
			double th = UIScale() * win_themestyle->normal->textheight();
			double w = 1. * th / mitem->image->h() * mitem->image->w();
			double h = 1. * th;
			dp->imageout(mitem->image, gx, rect->y + rect->height/2 - h/2, w, h);
		} else {
			dp->imageout(mitem->image, gx, rect->y + rect->height/2 - mitem->image->h()/2);
		}
	}

}

//! Draw a separator (default is just a win_themestyle->grayedfg colored line) across rect widthwise.
void TreeSelector::drawsep(const char *name,IntRectangle *rect)
{
	Displayer *dp = GetDisplayer();

	dp->NewFG(win_themestyle->fggray); 
	dp->drawline(rect->x,int(rect->y+rect->height/2)+.5, rect->x+rect->width-1,int(rect->y+rect->height/2)+.5);

	if (!isblank(name)) {
		int extent = dp->textextent(name, -1, NULL, NULL);
		 //blank out area
		dp->NewFG(win_themestyle->bg); 
		dp->drawrectangle(rect->x+rect->width/2-extent/2-2,rect->y,extent+4,rect->height, 1);
		 //draw name
		dp->NewFG(win_themestyle->fggray); 
		dp->textout(rect->x+rect->width/2,rect->y+rect->height/2, name,-1, LAX_CENTER);
	}
}


/*! When TREESEL_SEND_STRINGS, then send strings with this detail.
 *  This will set TREESEL_SEND_DETAIL in menustyle.
 */
void TreeSelector::SendDetail(int which)
{
	senddetail = which;
	menustyle |= TREESEL_SEND_DETAIL;
}

//! Send message to owner.
/*! If `!(menustyle&TREESEL_SEND_STRINGS)` Sends SimpleMessage with:
 * \code
 * info1 = curitem index (which might be -1 for nothing)
 * info2 = id of curitem
 * info3 = hover changed
 * info4 = menuitem->info
 * \endcode
 *
 * action == 1 means hover changed.
 * 2 == flag on, 3 == flag off. 2 or 3 implies info4 is the flag character
 * as specified by actiondetail.
 *
 * Otherwise, if `menustyle&TREESEL_SEND_STRINGS`, sends a list of the selected strings in a StrsEventData,
 * with info=curitem.
 *
 * \todo *** there needs to be option to send id or list of ids..
 * \todo maybe send device id of the device that triggered the send
 */
int TreeSelector::send(int deviceid, int action, int actiondetail, int which)
{
	DBG cerr <<WindowTitle()<<" send"<<endl;
	if (!win_owner || !win_sendthis) return 0;


	if (which < 0) which = curitem;

	 // find how many selected
	if (selection.n==0 && !(menustyle&TREESEL_SEND_ON_UP)) return 0;

	if (menustyle&TREESEL_SEND_STRINGS) {
		StrsEventData *strs=new StrsEventData(NULL,win_sendthis,object_id,win_owner);
		strs->n=selection.n;
		strs->strs=new char*[selection.n+1];
		strs->info =which;
		strs->info2=(which>=0 && which<numItems() ? item(which)->id : which);
		strs->info3=(which>=0 && which<numItems() ? item(which)->info : 0);

		int i=0;
		MenuItem *item;
		for (int c=0; c<selection.n; c++) {
			if (senddetail>=0 && (menustyle&TREESEL_SEND_DETAIL)) {
				item=selection.e[c]->GetDetail(senddetail);
				if (item) strs->strs[i++]=newstr(item->name);
				else strs->strs[i++]=newstr(selection.e[c]->name);
			} else strs->strs[i++]=newstr(selection.e[c]->name);
		}
		strs->strs[i]=NULL;
		app->SendMessage(strs,win_owner,win_sendthis,object_id);

	} else {
		 //send SimpleMessage
		MenuItem *itm = item(which);

		SimpleMessage *ievent=new SimpleMessage;
		ievent->info1 = which; 
		ievent->info2 = (which>=0 && which<numItems() ? itm->id : which);
		ievent->info3 = action; //old: selection.n;
		ievent->info4 = (which>=0 && which<numItems() ? itm->info : 0);

		if (action == 2 || action == 3) {
			ievent->info4 = actiondetail;
		}

        if (menustyle & TREESEL_SEND_PATH) {
             //we need to construct something like "Parent/parent/name"
			makestr(ievent->str, itm ? itm->name : NULL);
            while (itm && itm->parent && itm->parent->parent) {
                itm = itm->parent->parent;
                prependstr(ievent->str, "/");
                prependstr(ievent->str, itm->name);
            }

        } else {
			makestr(ievent->str, itm->name);
		}
		 

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

	if (ch==LAX_Esc && searchfilter) { //escape
		UpdateSearch(NULL,0);
		return 0;

	} else if (ch==LAX_Bksp && searchfilter) {
		char *old = newstr(searchfilter);
		const char *ef = old + strlen(old);
		if (ef>old) ef--;
		ef = utf8back(ef, old, ef+1);
		old[ef-old] = '\0';
		UpdateSearch(old, 0);
		return 0;

	} else if (ch==LAX_Enter) { // enter
		MenuItem *ii = item(curitem);
		if (ii && !(ii->hasSub() && HasStyle(TREESEL_SELECT_LEAF_ONLY))) {
			send(d->id);
		}
		return 0;

	} else if (ch==LAX_Home) { // jump to beginning
		int c=ccuritem;
		ccuritem=0;
		if (ccuritem!=c) {
			//drawitem(c);
			c=makeinwindow();
			if (menustyle&TREESEL_CURSSELECTS) {
				addselect(ccuritem,state);
				if (menustyle&TREESEL_CURSSENDS) send(d->id);
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
				if (menustyle&TREESEL_CURSSENDS) send(d->id);
			} else if (!c) {
				//drawitem(ccuritem);
			}
			needtodraw|=2;
		}
		return 0;

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
				if (menustyle&TREESEL_CURSSENDS) send(d->id);
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
				if (menustyle&TREESEL_CURSSENDS) send(d->id);
			} else if (!c) {
				//drawitem(ccuritem);
			}
			needtodraw|=2;
		}
		return 0;

	// } else if (ch==LAX_Left) {   ...maybe do something else when multiple columns??
	// 	MenuItem *item = item(ccuritem);
	// 	return 0;

	// } else if (ch==LAX_Right) {
	// 	return 0;

	} else if (ch == LAX_Left || (ch=='-' && !HasStyle(TREESEL_LIVE_SEARCH))) {  // Collapse current item, or all if control
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

	} else if (ch == LAX_Right || ((ch=='=' || ch=='+') && !HasStyle(TREESEL_LIVE_SEARCH))) {  // Expand current item, or all if control
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

	} else if (ch==' ' && !HasStyle(TREESEL_LIVE_SEARCH)) {  // same as an LBDown on ccuritem
		addselect(ccuritem,state);
		if (menustyle&TREESEL_SEND_ON_UP) send(d->id);
		return 0;

	} else if (ch=='a' && !HasStyle(TREESEL_LIVE_SEARCH)) {
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

	} else if (((state&LAX_STATE_MASK)|ShiftMask)==ShiftMask && ch < 0x1000000 && HasStyle(TREESEL_LIVE_SEARCH)) {
		 // update searchterm
		char *old = newstr(searchfilter);
		char s[10];
		s[utf8encode(ch, s)] = '\0';
		appendstr(old, s);
		UpdateSearch(old, 0);
		delete[] old;

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
	MenuItem *mitem = item(i),*titem;
	//if (!mitem) return;
	//if (mitem->state&LAX_SEPARATOR) return;

	if (!(state&ShiftMask) || menustyle&(TREESEL_ZERO_OR_ONE|TREESEL_ONE_ONLY)) {
		 //shift not pressed, or select zero or one

		int oldstate = mitem ? mitem->state : 0;
		if (!(state&ControlMask)) {
			 // unselect others
			if (selection.n) oldstate = LAX_OFF;
			for (c=selection.n-1; c>=0; c--) { // turn off all the ones that are on
				titem = selection.e[c];
				selection.remove(c);
				titem->state &= ~(LAX_ON|LAX_OFF|MENU_SELECTED); 
				titem->state |= LAX_OFF; 
			}
		}

		if (mitem) {
			if ((menustyle&TREESEL_ONE_ONLY) || oldstate==0 || (oldstate&LAX_MSTATE_MASK)==LAX_OFF) {
				 //turn on
				mitem->state = (mitem->state & ~(LAX_ON|LAX_OFF|MENU_SELECTED)) | (LAX_ON|MENU_SELECTED);
				selection.pushnodup(mitem,0);
				
			//} else if ((oldstate&(LAX_ON|LAX_OFF|MENU_SELECTED))==LAX_ON) {
			} else if ((oldstate & (LAX_ON|LAX_OFF))==LAX_ON) {
				 //turn off
				mitem->state=(mitem->state & ~(LAX_ON|LAX_OFF|MENU_SELECTED)) | LAX_OFF;
				selection.remove(selection.findindex(mitem));
			}
		}

		ccuritem = curitem = i;
		curmenuitem = mitem;

		needtodraw |= 2;

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
			if (HasStyle(TREESEL_SELECT_LEAF_ONLY) && (titem->hasSub())) continue;
			if (HasStyle(TREESEL_SELECT_SUB_ONLY) && !(titem->hasSub())) continue;
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

	int detailhover = -1;
	int flaghover = -1;
	int item = -1;

	int onsub=0;
	item = findItem(x,y, &onsub, &detailhover, &flaghover);
	ccurflag = flaghover;
	ccurdetail = detailhover;

	DBG cerr <<"-----click on "<<item<<" detailhover: "<<detailhover<<endl;

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
	int dragamount = buttondown.up(d->id,LEFTBUTTON, &item,&hovered);
	
	if (item == -2 && hovered >= 0) {
		 //click down and up on column header. Adjust sort.
		if (dragamount<7) {
			int column = findColumn(x);

			if (columns.n && column>=0) {
				if (columns.e[column]->sort==0) {
					 //set column to sort column
					for (int c=0; c<columns.n; c++) columns.e[c]->sort=0;
					columns.e[column]->sort=1;

				} else {
					 //toggle sort direction
					columns.e[column]->sort = (columns.e[column]->sort==1 ? -1 : 1);
				}

				if (columns.e[column]->type == ColumnInfo::ColumnBytes) {
					 //has to be SORT_123kb or SORT_321kb
					menu->sortstyle &= ~(SORT_ABC|SORT_CBA|SORT_123|SORT_321|SORT_123kb|SORT_321kb);
					if (columns.e[column]->sort==1) menu->sortstyle|=SORT_123kb;
					else menu->sortstyle|=SORT_321kb;

				} else {
					menu->sortstyle &= ~(SORT_123kb|SORT_321kb);

					if ((menu->sortstyle&(SORT_ABC|SORT_CBA|SORT_123|SORT_321))==0) menu->sortstyle|=SORT_ABC;

					if ((menu->sortstyle&(SORT_ABC|SORT_CBA))) {
						menu->sortstyle &= ~(SORT_ABC|SORT_CBA);
						if (columns.e[column]->sort==1) menu->sortstyle|=SORT_ABC;
						else menu->sortstyle|=SORT_CBA;

					} else if ((menu->sortstyle&(SORT_123|SORT_321))) {
						menu->sortstyle &= ~(SORT_123|SORT_321);
						if (columns.e[column]->sort==1) menu->sortstyle|=SORT_123;
						else menu->sortstyle|=SORT_321;
					}
				}

				sort_detail=columns.e[column]->detail;
				menu->SetCompareFunc(menu->sortstyle);
				visibleitems.SetCompareFunc(menu->sortstyle);
				//visibleitems.Sort(sort_detail);
				menu->Sort(sort_detail);

				RebuildCache();
				needtodraw=1;
			}
		}
		return 0;
	}

	// if (hovered>=0) return 0; //was dragging on column divider
	if (item < 0) return 0; //was dragging on something column header related

	 // do nothing if mouse is outside window.. This is necessary because of grabs.
	if (x<0 || x>=win_w || y<0 || y>=win_h) { return 0; }

	int curdetailhover = -1;
	int curflaghover = -1;
	int onsub=0;
	int i = findItem(x,y,&onsub, &curdetailhover, &curflaghover);

	if (i != item || curdetailhover != hovered) return 0; //up on something not down on
	if (i<0 || i>=numItems()) return 0; //if not on any item or arrow
	
	if (mousedragmode==2) {
		// *** dragging to rearrange
		mousedragmode=0;
		return 0;
	}

	 // clicked on already selected item
	const MenuItem *ii=Item(i);
	if (dragamount<7) {

		if (onsub && ii->hasSub() && !HasStyle(TREESEL_DONT_EXPAND)) { 
			if (ii->isOpen()) Collapse(i);
			else Expand(i);
			arrangeItems();
			return 0;

		} else if (curflaghover >= 0) {
			ToggleFlag(i, curdetailhover, curflaghover);
			return 0;

		} else if (HasStyle(TREESEL_EDIT_IN_PLACE)) {
			editInPlace(i);
			return 0; 
		}
	}

	addselect(i,state);

	if (!(ii->hasSub() && HasStyle(TREESEL_SELECT_LEAF_ONLY))) {
		if (HasStyle(TREESEL_SEND_ON_UP))    send(d->id);
		if (HasStyle(TREESEL_DESTROY_ON_UP)) app->destroywindow(this);
	}
	
	return 0;
}

/*! Return the character that is the opposite of the given character.
 * By default, check flagdef first. If not found, swap upper and lowercase chars.
 * If not upper/lower, non-space returns space, space returns '.'.
 */
char TreeSelector::ToggleChar(char current)
{
	if (flagdef) {
		char *ptr = strchr(flagdef, current);
		if (!ptr) return current;

		if ((ptr - flagdef)%2 == 0) return ptr[1];
		return *(ptr-1);
	}

	if (islower(current)) return toupper(current);
	if (isupper(current)) return tolower(current);
	if (current != ' ') return ' ';
	if (current == ' ') return '.';
	return current;
}

int TreeSelector::ToggleFlag(int itemi, int detail, int flag)
{
	if (detail < 0 || detail >= columns.n) return -1;

	MenuItem *ii = item(itemi);
	if (!ii) return -1;

	MenuItem *idetail = ii->GetDetail(columns.e[detail]->detail);
	if (!idetail) return -1;

	if (flag < 0 || flag >= (int)strlen(idetail->name)) return -1;

	idetail->name[flag] = ToggleChar(idetail->name[flag]);

	send(0, 2, idetail->name[flag], itemi);
	needtodraw = 1;
	return 1;
}

//! Find the index of the item at window coordinates (x,y).
/*! The value returned is not necessarily an actual element index. It could be -1, or >=menuitems.n.
 */
int TreeSelector::findItem(int x,int y, int *onsub, int *ondetail, int *onflag)
{
	if (onflag) *onflag = -1;
	if (ondetail) {
		*ondetail = -1;

		if (columns.n && y <= inrect.y) {
			 //check for clicking on column dividers to resize
			for (int c=columns.n-1; c>0; c--) {
				if (x>columns.e[c]->pos-10 && x<columns.e[c]->pos+10) {
					*ondetail = c;
					return -1;
				}
			}

			 // check for click on any column header
			for (int c=columns.n-1; c>=0; c--) {
				if (x>columns.e[c]->pos && x<columns.e[c]->pos+columns.e[c]->width) {
					*ondetail = c;
					return -2;
				}
			}
		}
	}
	
	x -= offsetx + inrect.x;
	y -= offsety + inrect.y;
	int s=0, e=visibleitems.n()-1, m;
	
	MenuItem *i;
	int which = -1;
	int itemgap = 1;
	while (e >= s) {
		i = item(s);

		if (y < i->y) return -1;
		if (y >= i->y && y <= i->y + i->h + itemgap) { which=s; break; }

		i = item(e);
		if (y >= i->y+i->h) return e+1;
		if (y >= i->y && y <= i->y + i->h + itemgap) { which=e; break; }

		m = (e+s)/2;
		if (m==e || m==s) return -1;

		i = item(m);
		if (y >= i->y && y <= i->y + i->h + itemgap) { which=m; break; }

		if (y < i->y) { s++; e=m-1; }
		else { s=m+1; e--; }
	}

	if (which<0) return -1;
	i = item(which);

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
				if (ondetail) *ondetail = c;

				if (onflag && columns.e[c]->type == ColumnInfo::ColumnFlags) {
					*onflag = (x-columns.e[c]->pos) / (flag_width * i->h);
					//if (*onflag > numflags) *onflag = -1;
				}
				break;
			}
		}	
		if (col==tree_column) { //the column with the layer lines
			if (x-columns.e[col]->pos > indent*iwidth && x-columns.e[col]->pos < (indent+1)*iwidth) *onsub=1;
			else *onsub=0;
		}
	} else {
		if (x<i->x) *onsub=1; else *onsub=0;
		if (x>indent*iwidth && x<(indent+1)*iwidth) *onsub=1;
	}


	return which;
}

/*! Find column mouse is in. Note this is NOT whether it's on a boundary.
 *
 * If columns.n==0, then always return 0.
 * If columns.n>0, then only return a valid column when mouse is actually within
 * pos+data for that column. -1 is returned if x is outside.
 */
int TreeSelector::findColumn(int x)
{
	if (columns.n) {
		for (int c=0; c<columns.n; c++) {
			if (x>=columns.e[c]->pos && x<columns.e[c]->pos+columns.e[c]->width) {
				return c;
			}
		}
		return -1;
	}
	return 0;
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
	return 0;
}

//! Nothing but remove tag from buttondown.
int TreeSelector::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,RIGHTBUTTON)) return 1;
	buttondown.up(d->id,RIGHTBUTTON);
	return 0;
}

//! Middle button and drag drags the screen around (with potential autoscrolling)111
int TreeSelector::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id,MIDDLEBUTTON, x,y);
	return 0;
}

//! Nothing but remove tag from buttondown.
int TreeSelector::MBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	if (!buttondown.isdown(d->id,MIDDLEBUTTON)) return 1;
	buttondown.up(d->id,MIDDLEBUTTON);
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
	int hover = -1;
	int which = -1;
	buttondown.getextrainfo(d->id,LEFTBUTTON, &which, &hover);
	buttondown.move(d->id, x,y, &mx,&my);
	
	int onsub=0;
	int ondetail=0, onflag=0;
	int i=findItem(x,y, &onsub, &ondetail, &onflag);
	DBG cerr <<"tree found item "<<i<<", onsub:"<<onsub<<", ondetail: "<<ondetail<<", onflag: "<<onflag<<endl;

	if (!buttondown.any()) {
		 // do nothing if mouse is outside window..
		 // This is a necessary check because of grabs.
		if (x<0 || x>=win_w || y<0 || y>=win_h) return 0;
			
		if (i<0) i=0;
		if (i>=numItems()) i=numItems()-1;
		//if (i!=ccuritem) {
			if (HasStyle(TREESEL_FOLLOW_MOUSE)) {
				//if (menustyle&(TREESEL_CURSSELECTS)) addselect(i,state);
				//else {
					//curitem  = i;
					ccuritem = i;
					ccurdetail = ondetail;
					ccurflag = onflag;
					addselect(ccuritem,0);
					if (HasStyle(TREESEL_SEND_HOVERED)) send(0, true);
					needtodraw|=2;
				//}
			}
		//}
		return 0;
	}

	if (buttondown.isdown(d->id,LEFTBUTTON)) {
		if (which < 0 && hover >= 0) {
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

	if (!buttondown.isdown(d->id,RIGHTBUTTON) && !buttondown.isdown(d->id,MIDDLEBUTTON)) return 1;

	 //so right or middle button is down, need to scroll
	int m=1;
	if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) m=20;
	else if ((state&LAX_STATE_MASK)==ControlMask || (state&LAX_STATE_MASK)==ShiftMask) m=10;
	
	int dy=y-my;
	if (y<0) dy=y;
	else if (y>win_h) dy=y-win_h;

	if (dy>0) { 
		movescreen(0,-m*dy);
		if (y>0 && y<win_h) { mx=x; my=y; }

	} else if (dy<0) {
		movescreen(0,-m*dy);
		if (y>0 && y<win_h) { mx=x; my=y; }
	}
	return 0;
}

//! Autoscroll if necessary**** todo
/*! \todo *** must autoscroll when mouse over arrow and FOLLOW_MOUSE
 */
int TreeSelector::Idle(int tid, double delta)
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
	if (textheight <= 0) return 0;

	IntRectangle itemrect;
	if (!findRect(ccuritem,&itemrect)) return 0;

	int dx,dy;
	dx=dy=0;

	if (itemrect.x<inrect.x) dx=inrect.x-itemrect.x;
	else if (itemrect.x+itemrect.width>inrect.x+inrect.width)
		dx=(inrect.x+inrect.width)-(itemrect.x+itemrect.width);

	if (itemrect.y<inrect.y) dy=itemrect.y-inrect.y;
	else if (itemrect.y+itemrect.height > inrect.y+inrect.height)
		dy=(itemrect.y+itemrect.height)-(inrect.y+inrect.height);

	needtodraw=1;

	if (dx==0 && dy==0) return 0;
	int ret=panner->Shift(1,dx)|panner->Shift(2,dy);
	//offsetx = inrect.x-panner->GetCurPos(1);
	//offsety = inrect.y-panner->GetCurPos(2);
	offsetx = -panner->GetCurPos(1);
	offsety = -panner->GetCurPos(2);
	return ret;
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

	long c=(dx!=0 ? panner->Shift(1,dx) : 0);
	c|=(dy!=0 ? panner->Shift(2,dy) : 0);
	if (!c) return 0; // no shift occured

	//offsetx = inrect.x-panner->GetCurPos(1);
	//offsety = inrect.y-panner->GetCurPos(2);
	offsetx = -panner->GetCurPos(1);
	offsety = -panner->GetCurPos(2);

	//if (menustyle&TREESEL_USE_TITLE) offsety+=textheight;
	//if (columns.n) offsety+=textheight;
	
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
	//if (state&ControlMask && state&ShiftMask) movescreen(0,-3*pagesize*(textheight+leading));
	//else if (state&ControlMask || state&ShiftMask) movescreen(0,-pagesize*(textheight+leading)); 
	if (state&ControlMask && state&ShiftMask) movescreen(0,-3*pagesize);
	else if (state&ControlMask || state&ShiftMask) movescreen(0,-pagesize); 
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
	if (state&ControlMask && state&ShiftMask) movescreen(0,3*pagesize);
	else if (state&ControlMask || state&ShiftMask) movescreen(0,pagesize); 
	else movescreen(0,(textheight+leading));
	needtodraw=1;
	return 0;
}

//! Called same for ScrolledWindow, then arrangeItems.
/*! Has the effect of resizing scrollers, and remapping inrect and outrect.
 */
void TreeSelector::syncWindows()
{
	ScrolledWindow::syncWindows();
	arrangeItems();
	makeinwindow();
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
	 if (ntotalheight <= 0 || newleading < 0) return;
	 textheight = ntotalheight - newleading;
	 iwidth     = textheight;
	 leading    = newleading;
	 padg       = textheight / 2;
	 if (forcearrange == 1) syncWindows();  // syncwindows calls arrangeItems...
	 else if (forcearrange == 2) arrangeItems();
	 else if (forcearrange == 3) arrangeItems();
	 needtodraw = 1;
	 //*** flush any selected items?
}
	
//! Set outrect to be the window minus space for title.
void TreeSelector::findoutrect()
{
	outrect.x = outrect.y = 0;
	if (pad < 0) pad = textheight * .2;

	if (menustyle&TREESEL_USE_TITLE && menu->title) outrect.y += textheight+pad; //one line for title
	if (columns.n)  outrect.y += textheight+pad; //one line for column heads
	if (showsearch) outrect.y += textheight+pad; //one line for search term

	outrect.width  = win_w-outrect.x;
	outrect.height = win_h-outrect.y;
}

//! Remove the pads from inrect.
/*! This is called last thing from ScrolledWindow::syncWindows(), where inrect is set to be within scrollers.
 *  Also sets pagesize=(inrect.height+leading)/(textheight+leading)
 *
 *  This and arrangeItems are the only places where pagesize gets set.
 */
void TreeSelector::adjustinrect()
{
	if (pad < 0) pad = textheight * .2;
	inrect.x      +=   pad;
	inrect.width  -= 2*pad;
	inrect.y      +=   pad;
	inrect.height -= 2*pad;

//	if (menustyle&TREESEL_USE_TITLE) { <- subtracted in findoutrect
//		inrect.y+=textheight+leading;
//		inrect.height-=textheight+leading;
//	}

//	if (columns.n) { <- subtracted in findoutrect
//		inrect.y+=textheight+leading;
//		inrect.height-=textheight+leading;
//	}


//	if (showsearch) { <- subtracted in findoutrect
//		inrect.y += textheight;
//		inrect.height -= textheight;
//	}

	if (textheight+leading) pagesize=inrect.height*.75;
}

//! This is meant to be called when the window is going, but you just added or removed a bunch of stuff.
void TreeSelector::Sync()
{
	arrangeItems();
	needtodraw=1;
}

//! Resize the window to the extent of the items, and reposition near the mouse.
int TreeSelector::WrapToMouse(int mouseid, anXWindow *onedgeofthis) //onedgeofthis=0
{
    int x=0,y=0,screen=0;
    mouseposition(mouseid, NULL, &x, &y, NULL, NULL, &screen);
    return WrapToPosition(x,y,screen,onedgeofthis);
}


/*! Orient the window's size and position to be near the given screen coordinates.
 *
 *  If onedgeofthis is NULL, it prefers to put the window to the left of the mouse. 
 *  If it does not fit then it tries to put the window to the right of x,y. Also, 
 *  the currently selected item is made to be at the same
 *  height of y.
 *
 *  If onedgeofthis is not NULL, then it also tries to place 
 *  itself on a nearby right edge of onedgeofthis.
 *
 *  Currently, it assumes that this window is a top level window, such as a popup menu,
 *  and that it is not a tree, that is, it does not consider submenus.
 * 
 *  This also calls anXWindow::MoveResize().
 */
int TreeSelector::WrapToPosition(int screen_x, int screen_y, int screen, anXWindow *onedgeofthis) //onedgeofthis=NULL
{
	 //---------- Setup the Popup TreeSelector -----------------------
	 // Find the extent of the items, and lay it out with the current item
	 // as close to and to the right of the mouse as possible

	 // get screen geometry
	int px, py;     // what will be the window x,y
	int ew=0, eh=0; // extent of wrapped window
	int screen_width, screen_height;
	int x = screen_x, y = screen_y, scrx, scry;
	ScreenInformation *scr = app->FindNearestMonitor(screen, screen_x, screen_y);
	scrx = scr->x;
	scry = scr->y;
	screen_width = scr->width;
	screen_height= scr->height;

	if (textheight <= 0) textheight = UIScale() * win_themestyle->normal->textheight();
	if (pad < 0) pad = textheight * .2;

	 // -----find extent: ew,eh *** this only finds the text extent
	if (leading == 0) {
		leading = 1 * UIScale();
	}

	win_w = screen_width;
	win_h = screen_height;
	findoutrect();
	inrect = outrect;
	adjustinrect();
	SetLineHeight(textheight+leading,leading,0);
	arrangeItems();

	if (columns.n) {
		for (int c=0; c<columns.n; c++) ew += columns.e[c]->width;
	} else {
		for (int c=0; c<visibleitems.menuitems.n; c++) {
			MenuItem *im = visibleitems.menuitems.e[c];
			if (im->hidden() == 1) continue;
			if (im->w > ew) ew = im->w;
		}
	}

	double fullheight = 0;
	for (int c=0; c<visibleitems.menuitems.n; c++) {
		MenuItem *im = visibleitems.menuitems.e[c];
		if (im->hidden() == 1) continue;
		fullheight = im->y + im->h; //full height is just bottom edge of last visible item
	}

	eh = fullheight;
	eh += 2*pad;
	ew += 2*pad + 2.5*iwidth; // *** not quite sure why this works.. big pads in front

	y -= textheight/2;

	 // ------find placement of popup px,py based on ew,eh, x,y
	int arrowwidth = 15; // the amount aside from the pointer to shift to
	 // first set horizontal:
	 // Try to put popup to left of x,y, which are now in screen coords
	 // set x:
	if (x-arrowwidth-ew > scrx) { // popup fits to the left
		px = x-arrowwidth-ew;
	} else { //put popup at right
		px = x+arrowwidth;
		if (px+ew > scrx + screen_width) px = scrx + screen_width - ew; //put up against right side of screen
	}

	 // then set vertical: hopefully with item centered at y
	 // also find out offset necessary to put the currently selected item near the mouse
	int ypref;
	//ypref=(curitem%columnsize)*(textheight+leading)+pad;
	MenuItem *curi = item(curitem);
	ypref = (curi ? curi->y : 0);

	if (eh > (int)screen_height) eh = screen_height;
	if (y-ypref+eh > scry + (int)screen_height-2) { // window goes offscreen below when centered on curitem
		py = scry + screen_height-eh-2;
		//extrapad=y-py-ypref;
	} else if (y-ypref < scry) { // window goes offscreen top when centered on curitem
		py = scry;
		//extrapad=y-ypref-py;
	} else { // popup fits vertically just fine
		py = y-ypref;
	}

	 // Move window horizontally to be near right edge of window if possible
	if (onedgeofthis) {
		int wx = onedgeofthis->win_x;
		//int wy = onedgeofthis->win_y;
		int bd = onedgeofthis->win_border;
		unsigned int ww = onedgeofthis->win_w;
		//unsigned int wh = onedgeofthis->win_h;

		px = wx+ww+bd;
		if (px+ew > (int)screen_width) {
			px = wx-bd-ew;
			if (px<0) px = 0;
		}
	}
	
	MoveResize(px,py,ew,eh);
	
	SetFirst(curitem,x,y);

	needtodraw=1;
	return 0;
}

//! Calls ScrolledWindow::MoveResize(nx,ny,nw,nh).
int TreeSelector::MoveResize(int nx,int ny,int nw,int nh)
{
	ScrolledWindow::MoveResize(nx,ny,nw,nh);

	for (int c=0; c<columns.n; c++) {
		if (columns.e[c]->width_type==1) {
			RemapColumns();
			break;
		}
	}
	return 0;
}

//! Calls ScrolledWindow::Resize(nw,nh).
int TreeSelector::Resize(int nw,int nh)
{ 
	ScrolledWindow::Resize(nw,nh);

	for (int c=0; c<columns.n; c++) {
		if (columns.e[c]->width_type==1) {
			RemapColumns();
			break;
		}
	}

	return 0;
}

} // namespace Laxkit

