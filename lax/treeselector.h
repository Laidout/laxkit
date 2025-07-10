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
#ifndef _LAX_TREESELECTOR_H
#define _LAX_TREESELECTOR_H

#include <lax/anxapp.h>
#include <lax/menuinfo.h>
#include <lax/scrolledwindow.h>
#include <lax/buttondowninfo.h>
#include <lax/vectors.h>

#include <cstring>


namespace Laxkit {


 // Item placement and display flags
#define TREESEL_LEFT                 (1LL<<1 )
#define TREESEL_RIGHT                (1LL<<2 )
#define TREESEL_SCROLLERS            (1LL<<3 )
#define TREESEL_USE_TITLE            (1LL<<4 )
#define TREESEL_LIVE_SEARCH          (1LL<<6 )
#define TREESEL_STATIC_SEARCH        (1LL<<7 )

#define TREESEL_SORT_NUMBERS         (1LL<<8 )
#define TREESEL_SORT_LETTERS         (1LL<<9 )
#define TREESEL_SORT_REVERSE         (1LL<<10)
#define TREESEL_SORT_IGNORE_CASE     (1LL<<11)
#define TREESEL_SORT_BY_EXTENSIONS   (1LL<<12)

#define TREESEL_USE_DOT              (1LL<<13)
#define TREESEL_USE_DOT_DOT          (1LL<<14)

#define TREESEL_ONE_ONLY             (1LL<<15)
#define TREESEL_ZERO_OR_ONE          (1LL<<16)
#define TREESEL_SELECT_ANY           (1LL<<17)
#define TREESEL_SELECT_LEAF_ONLY     (1LL<<18)
#define TREESEL_SELECT_SUB_ONLY      (1LL<<19)
#define TREESEL_CURSSELECTS          (1LL<<20)
#define TREESEL_CURSSENDS            (1LL<<21)
#define TREESEL_FOLLOW_MOUSE         (1LL<<22)
#define TREESEL_GRAB_ON_MAP          (1LL<<23)
#define TREESEL_GRAB_ON_ENTER        (1LL<<24)

#define TREESEL_REARRANGEABLE        (1LL<<25)
#define TREESEL_EDIT_IN_PLACE        (1LL<<26)
//*** add items?? (more than just editing names)
//*** remove items??

#define TREESEL_SEND_ON_UP           (1LL<<27)
#define TREESEL_SEND_ON_ENTER        (1LL<<28)
#define TREESEL_SEND_IDS             (1LL<<29)
#define TREESEL_SEND_STRINGS         (1LL<<30)
#define TREESEL_SEND_PATH            (1LL<<31)
#define TREESEL_SEND_DETAIL          (1LL<<32)
#define TREESEL_SEND_HOVERED         (1LL<<33)
#define TREESEL_DESTROY_ON_UP        (1LL<<34)

#define TREESEL_GRAPHIC_ON_RIGHT     (1LL<<35) //else on left
#define TREESEL_NO_LINES             (1LL<<36)
#define TREESEL_FLAT_COLOR           (1LL<<37)
#define TREESEL_DONT_EXPAND          (1LL<<38)

#define TREESEL_SUB_FOLDER           (1LL<<39) //arrow graphic is a little folder
#define TREESEL_SUB_ON_RIGHT         (1LL<<40) //draw the submenu indicator on far right side

//... remember that the buck stops with (1<<63)





class TreeSelector : public ScrolledWindow
{
  private:
	void base_init();

	int firsttime;

  protected:

	ButtonDownInfo buttondown;
	int mousedragmode; 
	int offsetx,offsety;
	int firstinw;
	int textheight,lineheight,pagesize; //these have ui scale applied
	int timerid;
	int senddetail;
	int dragflag;
	char *flagdef;

	MenuInfo *menu; 
	MenuItem *curmenuitem;
	int curitem,ccuritem;
	int ccurdetail, ccurflag;
	PtrStack<MenuItem> selection;

	char *searchfilter;
	int showsearch;

	int needtobuildcache;
	MenuInfo visibleitems;

	virtual void adjustinrect();
	virtual void findoutrect();
 	virtual double getitemextent(MenuItem *mitem,double *w,double *h,double *gx,double *tx);
 	virtual double getgraphicextent(MenuItem *mitem,double *w,double *h);
//	virtual void drawitem(int c);
	virtual void drawsep(const char *name,IntRectangle *rect);
	virtual void drawSubIndicator(MenuItem *mitem,int x,int y, int selected);
	virtual void drawitemname(MenuItem *mitem,IntRectangle *rect);
	virtual void drawflags(MenuItem *mitem,IntRectangle *rect);
	virtual int  drawFlagGraphic(char flag, int x,int y,int w,int h);
	virtual void drawtitle(int &y);
	virtual int findmaxwidth(int s,int e, int *h_ret);
	virtual int findColumnWidth(int which);

	virtual int send(int deviceid, int action = 0, int actiondetail=0, int which = -1);
	virtual void addselect(int i,unsigned int state);
	virtual int findItem(int x,int y, int *onsub, int *ondetail = nullptr, int *onflag = nullptr);
	virtual int findColumn(int x);
	virtual int findRect(int c,IntRectangle *itemspot);
	virtual void arrangeItems();
	virtual void syncWindows();
	virtual int makeinwindow();
	virtual int numItems();
	virtual MenuItem *item(int i,char skipcache=0);

	virtual int ToggleFlag(int itemi, int detail, int flag);
	virtual char ToggleChar(char current);

	virtual int addToCache(int indent,MenuInfo *menu, int cury);
	virtual int DrawItems(int indent, MenuInfo *item, int &n, flatpoint offset);
	virtual void drawItemContents(MenuItem *i,int offset_x,int suboffset,int offset_y, int fill, int indent);
	virtual void drawarrow(int x,int y,int r,int type);

	virtual void editInPlace(int which);
  
  public:
	class ColumnInfo
	{
	  public:
		enum ColumnInfoType {
			ColumnString,
			ColumnInt,
			ColumnNumber, //double or int
			ColumnBytes, //like "23kb" or "5 MB"
			ColumnDate,
			ColumnFlags,
			ColumnMAX
		};
		char *title;
		int pos;
		int width;
		int width_type; //0=absolute, 1=fill
		int detail; //for remapping order of columns
		int type; //uses ColumnInfoType
		int sort; //0 don't sort, 1 sort ascending, -1 sort descending
		int sort_type; //overrides default sort for menu
		CompareFunc compareFunc;

		ColumnInfo(const char *ntitle, int ntype, int whichdetail, int nwidth, int nwtype);
		~ColumnInfo();
	};
	PtrStack<ColumnInfo> columns;
	float flag_width; // actual width = flag_width * cell height
	int tree_column;
	int sort_detail;
	int sort_descending;
	bool sort_dirs_first;

	unsigned long highlight,shadow;
	unsigned long long menustyle;
	int column_gap;
	int padg,pad,leading;
	int iwidth; //tree arrow width
	double iconwidth; //1.0 == textheight

	TreeSelector(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
				int xx,int yy,int ww,int hh,int brder,
				anXWindow *prev, unsigned long nowner=0, const char *mes=0,
				unsigned long long nmstyle=0, MenuInfo *minfo = nullptr); 
	virtual ~TreeSelector();
	virtual const char *whattype() { return "TreeSelector"; }
	virtual int init();
	virtual void Refresh();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int Idle(int tid, double delta);
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual int FocusOn(const FocusChangeData *e);
	virtual int FocusOff(const FocusChangeData *e);
	virtual int Event(const EventData *e,const char *mes);
	virtual void UIScaleChanged();

	virtual int WrapToMouse(int mouseid, anXWindow *onedgeofthis = nullptr);
	virtual int WrapToPosition(int screen_x, int screen_y, int screen, anXWindow *onedgeofthis = nullptr);

	virtual void       dump_out(FILE *f,int indent,int what,DumpContext *savecontext);
    virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *savecontext); 
    virtual void dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext);

    virtual bool SetStyle(unsigned long long style, int newvalue);
	virtual bool HasStyle(unsigned long long style);


	virtual int movescreen(int dx,int dy);
	virtual int SetFirst(int which,int x,int y);
	virtual int Curitem() { return curitem; }
	virtual const MenuItem *Item(int c) { return item(c); }
	virtual MenuInfo *Menu() { return menu; }
	virtual int InstallMenu(MenuInfo *nmenu);
	virtual void Wrap();

	virtual int Expand(int which);
	virtual int Collapse(int which);
	virtual int Select(int which, bool replace_selection = true);
	virtual int Select(const char *name, bool replace_selection);
	virtual int SelectId(int which);
	virtual int SelectAtPath(const char **path, int n, bool replace_selection);
	virtual int Deselect(int which);
	virtual int DeselectAll();
	virtual int RebuildCache();
	virtual int ShowSearch(bool on=true);
	virtual int ClearSearch();
	virtual int UpdateSearch(const char *searchterm, bool isprogressive);

	virtual void SendDetail(int which);
	virtual int GetSelectedIndex(int i);
	virtual int GetSelectedId(int i);
	virtual MenuItem *GetSelected(int i);
	virtual MenuItem *GetItem(int i);
	virtual int NumSelected();

	//virtual int WhichSelected(unsigned int state);
	virtual void SetLineHeight(int ntotalheight,int newleading, char forcearrange);
	virtual void Sync();

	//virtual int RemoveItem(int whichid);
	//virtual int RemoveItem(const char *i);
	virtual void Sort(int t, int detail);
	virtual void Sort();
	virtual int AddItems(const char **i,int n,int startid); // assume ids sequential, state=0
	virtual int AddItem(const char *i,LaxImage *img,int nid,int newstate);

	virtual int AddColumn(const char *i,LaxImage *img, int width,int width_type=0,
							int ntype=ColumnInfo::ColumnString, int whichdetail=-1,
							bool nodup=true, int sort_override=-1);
	virtual void ClearColumns();
	virtual void RemapColumns();
};


} // namespace Laxkit

#endif


