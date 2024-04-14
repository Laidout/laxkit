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
//    Copyright (C) 2015 by Tom Lechner
//



#include <lax/interfaces/characterinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>
#include <lax/utf8utils.h>
#include <lax/debug.h>


using namespace Laxkit;


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class CharacterInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


// static variable
SingletonKeeper CharacterInterface::settingsObject;


/*! Works on a duplicate of nfont.
 */
CharacterInterface::CharacterInterface(anInterface *nowner, int nid, Displayer *ndp, LaxFont *nfont)
 : anInterface(nowner,nid,ndp)
{
	character_interface_style = 0;

	settings = dynamic_cast<Settings*>(settingsObject.GetObject());
	if (!settings) {
		settings = new Settings();
		settingsObject.SetObject(settings, true);
		settings->boxwidth = 30;
	}
	settings->inc_count();

	showdecs   = 1;
	needtodraw = 1;

	font = nullptr;
	if (nfont) {
		font=nfont->duplicate();
	}

	firsttime      = 1;
	needtosetchars = true;
	numwide        = 0;
	numtall        = 0;
	insertpoint.set(-1,-1);

	current     = -1;
	curcategory = 0;

	sc = nullptr;  // shortcut list, define as needed in GetShortcuts()
}

CharacterInterface::~CharacterInterface()
{
	if (sc)       sc->dec_count();
	if (font)     font->dec_count();
	if (settings) settings->dec_count();
}

const char *CharacterInterface::whatdatatype()
{
	return nullptr; // nullptr means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *CharacterInterface::Name()
{ return _("Insert Character"); }


//! Return new CharacterInterface.
/*! If dup!=nullptr and it cannot be cast to CharacterInterface, then return nullptr.
 */
anInterface *CharacterInterface::duplicate(anInterface *dup)
{
	if (dup==nullptr) dup=new CharacterInterface(nullptr,id,nullptr,nullptr);
	else if (!dynamic_cast<CharacterInterface *>(dup)) return nullptr;
	return anInterface::duplicate(dup);
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int CharacterInterface::UseThis(anObject *nobj, unsigned int mask)
{
	return 0;
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int CharacterInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int CharacterInterface::InterfaceOff()
{
	Clear(nullptr);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void CharacterInterface::Clear(SomeData *d)
{
}

void CharacterInterface::ViewportResized()
{
	SetBoxes();
}

/*! Fill chars with the characters defined in the font.
 */
void CharacterInterface::SetupChars()
{
	if (!font) return;

	chars.flush_n();

	FT_Library *ft_library = app->fontmanager->GetFreetypeLibrary();
	if (!ft_library) return; //this shouldn't happen!


	 //scan in from a freetype face
	FT_Face ft_face = nullptr;
	FT_Error ft_error = FT_New_Face(*ft_library, font->FontFile(), 0, &ft_face);
	if (ft_error) { 
		DBGE(" ERROR loading "<<(font->FontFile()?font->FontFile():"(no font file!!)")<<" with FT_New_Face");
		return;
	}

	 //find defined characters for font, distill in to ranges
	int code;
	FT_UInt gindex = 0;

	code = FT_Get_First_Char( ft_face, &gindex );

	while (gindex != 0) {
		chars.push(code); 
		code = FT_Get_Next_Char( ft_face, code, &gindex );
	} 


	if (ft_face) {
		FT_Done_Face(ft_face);
		ft_face = nullptr;
	}

	needtosetchars = false;
}

/*! Initialize or reset the various boxes to fit on screen.
 *
 * Default is to put a blank box around insertpoint. 
 * Recent chars are above.
 * Suggestions are below.
 * All the glyphs are to the right.
 *
 */
void CharacterInterface::SetBoxes()
{
	double boxwidth = settings->boxwidth;

	recentbox.miny = -boxwidth*NumRecent()/2;
	recentbox.maxy =  boxwidth*NumRecent()/2;
	recentbox.minx = 0; //-boxwidth/2;
	recentbox.maxx = boxwidth; // boxwidth/2;

	suggestionbox.miny =  boxwidth/2;
	suggestionbox.maxy =  boxwidth/2 + boxwidth*(NumSuggestions()+1);
	suggestionbox.minx = -boxwidth/2;
	suggestionbox.maxx =  boxwidth/2;


	bigbox.minx = boxwidth;
	bigbox.maxx = bigbox.minx + curwindow->win_w - 2*boxwidth;

	numwide = int(bigbox.boxwidth()/boxwidth);
	if (numwide < 1) numwide = 1;
	if (needtosetchars) SetupChars();
	if (chars.n == 0) numtall = 5; 
	else numtall = chars.n/numwide+1;

	bigbox.minx = curwindow->win_w/2 - boxwidth*numwide/2;
	bigbox.maxx = curwindow->win_w/2 + boxwidth*numwide/2;
	bigbox.miny = -boxwidth*numtall/2;
	bigbox.maxy =  boxwidth*numtall/2;

	if (insertpoint.x<0 || insertpoint.y<0) {
		insertpoint.x = 0;
		insertpoint.y = curwindow->win_h/2;
	}

	offset = insertpoint;

	needtodraw = 1;
}

Laxkit::MenuInfo *CharacterInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
//	if (no menu for x,y) return nullptr;
//
//	if (!menu) menu=new MenuInfo;
//	if (!menu->n()) menu->AddSep(_("Some new menu header"));
//
//	menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path, LAX_ISTOGGLE|(istyle&FREEHAND_Raw_Path)?LAX_CHECKED:0);
//	menu->AddItem(_("Some menu item"), SOME_MENU_VALUE);
//	menu->AddSep(_("Some separator text"));
//	menu->AddItem(_("Et Cetera"), SOME_OTHER_VALUE);
//	return menu;

	return nullptr;
}

int CharacterInterface::Event(const Laxkit::EventData *data, const char *mes)
{
//    if (!strcmp(mes,"menuevent")) {
//        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
//        int i =s->info2; //id of menu item
//
//        if ( i==SOME_MENU_VALUE) {
//			...
//		}
//
//		return 0; 
//	}

	return 1; //event not absorbed
}



int CharacterInterface::Refresh()
{

	if (needtodraw==0) return 0;

	if (firsttime) {
		if (!curwindow || curwindow->win_h<=0) return 0;
		SetBoxes();
		firsttime=0;
	}

	needtodraw=0;
	if (chars.n==0 && owner) {
		PostMessage(_("Could not scan font for characters!"));
		owner->RemoveChild();
		return 0;
	}

	dp->DrawScreen();
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	double boxwidth = settings->boxwidth;

	 //draw recent box
	unsigned int linecolor=coloravg(curwindow->win_themestyle->fg,curwindow->win_themestyle->bg, .5);
	unsigned int textcolor=coloravg(curwindow->win_themestyle->fg,curwindow->win_themestyle->bg, .2);
	unsigned int bgcolor  =coloravg(curwindow->win_themestyle->fg,curwindow->win_themestyle->bg, .9);
	char utf8[20];
	int len;

	dp->NewBG(bgcolor);
	dp->font(font, boxwidth*.8);

	double x = offset.x + recentbox.minx;
	double y = offset.y + recentbox.miny;
	for (int c=0; c<NumRecent(); c++) {
		dp->NewFG(linecolor);
		if (c == current && curcategory == INSCHAR_Recent) {
			// draw highlighted box
			dp->NewFG(curwindow->win_themestyle->fg);
			dp->NewBG(curwindow->win_themestyle->bg);
			dp->LineWidthScreen(3);
			dp->drawrectangle(x+.5, y+.5, boxwidth,boxwidth, 2);
			dp->LineWidthScreen(1);
			dp->NewFG(linecolor);
			dp->NewBG(bgcolor);

		} else {
			// draw normal box
			dp->drawrectangle(x+.5, y+.5, boxwidth,boxwidth, 2);
		}
		//dp->drawrectangle(x+.5, y+.5-boxwidth, boxwidth,boxwidth, 2);

		dp->NewFG(textcolor);
		len = utf8encode(settings->recent.e[c], utf8);
		utf8[len] = '\0';
		dp->textout(x+boxwidth/2,y+boxwidth/2, utf8,len, LAX_CENTER);

		y += boxwidth;
		//if (y<0) break;
	}


	 //draw suggestion boxes
	x = offset.x+suggestionbox.minx;
	y = offset.y+suggestionbox.miny;
	for (int c=0; c<NumSuggestions(); c++) {
		dp->drawrectangle(x+.5, y+.5, boxwidth,boxwidth, 2);
		y += boxwidth;
		if (y > curwindow->win_h) break;
	}


	 //draw all glpyh boxes
	int i=0;
	
	for (int rr=0; rr<numtall; rr++) {
	  x = offset.x + bigbox.minx;
	  y = offset.y - numtall*boxwidth/2 + rr*boxwidth;

	  if (y+boxwidth < 0) {
		  continue;
	  }
	  if (y > curwindow->win_h) break;

	  i = rr*numwide;

	  for (int cc=0; cc<numwide; cc++) {
		if (i >= chars.n) break;

		if (i==current && curcategory==INSCHAR_MainBox) {
			// draw highlighted box
			dp->NewFG(curwindow->win_themestyle->fg);
			dp->NewBG(curwindow->win_themestyle->bg);
			dp->LineWidthScreen(3);
			dp->drawrectangle(x+.5, y+.5, boxwidth,boxwidth, 2);
			dp->LineWidthScreen(1);
			dp->NewFG(linecolor);
			dp->NewBG(bgcolor);

		} else {
			// draw normal box
			dp->drawrectangle(x+.5, y+.5, boxwidth,boxwidth, 2);
		}

		dp->NewFG(textcolor);
		len = utf8encode(chars.e[i], utf8);
		utf8[len] = '\0';
		dp->textout(x+boxwidth/2,y+boxwidth/2 - (settings->show_unicode ? boxwidth*.1 : 0), utf8,len, LAX_CENTER);

		dp->NewFG(linecolor);

		x += boxwidth;
		i++;

		if (x > curwindow->win_w) break;
	  }
	}

	if (settings->show_unicode) {
		dp->font(font, boxwidth*.2);
		dp->NewFG(textcolor);
		i = 0;
	
		for (int rr=0; rr<numtall; rr++) {
			x = offset.x + bigbox.minx;
			y = offset.y - numtall*boxwidth/2 + rr*boxwidth;

			if (y+boxwidth < 0) continue;
			if (y > curwindow->win_h) break;

			i = rr*numwide;

			for (int cc=0; cc<numwide; cc++) {
				if (i >= chars.n) break;

				sprintf(utf8, "U+%04x", chars.e[i]);
				dp->textout(x+boxwidth/2,y+boxwidth, utf8,-1, LAX_HCENTER | LAX_BOTTOM);

				x += boxwidth;
				i++;
				if (x > curwindow->win_w) break;
			}
		}
	}

	if (chars.n == 0) {
		dp->NewFG(curwindow->win_themestyle->fg);
		dp->textout(curwindow->win_w/2,curwindow->win_h/2, _("Could not scan font for characters!"),-1, LAX_CENTER);
	}


	//dp->textout((dp->Maxx+dp->Minx)/2,(dp->Maxy+dp->Miny)/2, "Blah!",,-1, LAX_CENTER);

	dp->DrawReal();
	dp->font(app->defaultlaxfont);

	return 0;
}

int CharacterInterface::NumRecent()
{
	return settings->recent.n;
}

int CharacterInterface::NumSuggestions()
{
	return 0;
}

/*! Give some context to guide suggestions for characters to insert.
 *
 * Return 0 for success, or nonzero for context not understood for some reason.
 */
int CharacterInterface::Context(const char *str, long pos, long len)
{
	//***
	return 0;
}

/*! nfont is the font to base the displayed font on. We will not actually use nfont, but
 * a copy of it set to the right size for boxwidth.
 */
int CharacterInterface::Font(Laxkit::LaxFont *nfont)
{ // ***
	DBGE("IMPLEMENT ME!");
	return 0;
}

int CharacterInterface::Font(const char *family, const char *style, double size, Laxkit::ScreenColor *color)
{ // ***
	DBGE("IMPLEMENT ME!");
	return 0;
}

/*! A hint for where on screen the character is to be inserted.
 */
void CharacterInterface::ScreenPosition(flatpoint p)
{
	insertpoint=p;
	if (insertpoint.x<0) insertpoint.x=0;
	else if (insertpoint.x>=curwindow->win_w) insertpoint.x=curwindow->win_w-1;
	if (insertpoint.y<0) insertpoint.y=0;
	else if (insertpoint.y>=curwindow->win_h) insertpoint.y=curwindow->win_h-1;
}

int CharacterInterface::scan(int x, int y, unsigned int state, int *category)
{
	x -= offset.x;
	y -= offset.y;

	int ix,iy;
	double boxwidth = settings->boxwidth;

	if (recentbox.boxcontains(x,y)) {
		ix = (x-recentbox.minx)/boxwidth;
		iy = (y-recentbox.miny)/boxwidth;
		if (iy >= settings->recent.n) iy = -1;
		*category = INSCHAR_Recent;

		return iy; // recent box is single column
	}

	if (suggestionbox.boxcontains(x,y)) {
		ix = (x-suggestionbox.minx)/boxwidth;
		iy = (y-suggestionbox.miny)/boxwidth;
		*category = INSCHAR_Suggestions;
		return iy*(suggestionbox.maxx-suggestionbox.miny)/boxwidth + ix;
	}

	if (bigbox.boxcontains(x,y)) {
		ix = (x-bigbox.minx)/boxwidth;
		iy = (y-bigbox.miny)/boxwidth;
		*category =INSCHAR_MainBox;
		if (ix >= 0 && ix < numwide) {
			return iy*numwide + ix;
		}
	}

	*category = 0;
	return -1;
}

//! Start a new freehand line.
int CharacterInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	int cat = 0;
	int i = scan(x,y, state, &cat);
	buttondown.down(d->id,LEFTBUTTON,x,y, i,cat);
	needtodraw = 1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

//! Finish a new freehand line by calling newData with it.
int CharacterInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	int cat=0;
	int i=scan(x,y, state, &cat);
	int i1,i2;
	buttondown.up(d->id,LEFTBUTTON, &i1,&i2);

	if (i1==i && i2==cat) {
		if (i>=0) send();
		if (owner) owner->RemoveChild();
	}

	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int CharacterInterface::MBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	buttondown.down(d->id,MIDDLEBUTTON,x,y);
	return 0;
}

int CharacterInterface::MBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	buttondown.up(d->id,MIDDLEBUTTON);
	return 0;
}


int CharacterInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		int cat=0;
		int i=scan(x,y, state, &cat);

		if (i>=0 && i<chars.n && (i!=current || cat!=curcategory)) {
			current = i;
			curcategory = cat;
			needtodraw = 1;
			if (cat == INSCHAR_MainBox)
				PostMessage2(_("U+%04x"), chars.e[current]);
			else if (cat == INSCHAR_Recent)
				PostMessage2(_("U+%04x"), settings->recent.e[current]);
		}
		return 1;
	}

	int lx,ly;
	buttondown.move(d->id, x,y, &lx,&ly);

	//offset.x += x-lx;
	int dy= y-ly;
	//if ((dy>0 && offset.y+dy+bigbox.miny < boxwidth) || (dy<0 && offset.y+dy+bigbox.maxy+boxwidth > dp->Maxy)) {
		offset.y += dy; 
		if (offset.y + bigbox.miny > dp->Maxy || offset.y+bigbox.maxy<0) {
			if (owner) owner->RemoveChild();
		}
		needtodraw=1;
	//}

	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

int CharacterInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if ((state&LAX_STATE_MASK)==ControlMask) {
		settings->boxwidth *= 1.2;
		firsttime = 1; //triggers a SetBoxes() on next refresh
		insertpoint.set(-1,-1);
		needtodraw = 1;
		return 0;

	} else {
		int dy = settings->boxwidth;
		if (state & ShiftMask)   dy *= 10;
		if (state & ControlMask) dy *= 10;
		offset.y += dy;

		if (offset.y + bigbox.maxy > curwindow->win_h && offset.y + bigbox.miny - settings->boxwidth > 0) 
			offset.y = -bigbox.miny+settings->boxwidth;
		needtodraw=1;
		return 0;
	}

	return 1; //wheel up ignored
}

int CharacterInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{
	if ((state&LAX_STATE_MASK)==ControlMask) {
		settings->boxwidth *= .9;
		if (settings->boxwidth < 5) settings->boxwidth = 5; //arbitrary lower pixel limit
		insertpoint.set(-1,-1);
		firsttime=1; //triggers a SetBoxes() on next refresh
		needtodraw=1;
		return 0;

	} else {
		int dy = settings->boxwidth;
		if (state & ShiftMask)   dy *= 10;
		if (state & ControlMask) dy *= 10;
		offset.y -= dy;

		if (offset.y+bigbox.miny<0 && offset.y+bigbox.maxy+settings->boxwidth<curwindow->win_h) 
			offset.y = curwindow->win_h - (bigbox.maxy + settings->boxwidth);
		needtodraw=1;
		return 0;
	}

	return 1; //wheel down ignored
}


/*! Sends SimpleMessage, with
 * string ==(utf8 of current char),
 * info1  == current char
 * info2  == current category
 */
int CharacterInterface::send()
{
	if (!owner || current < 0) return 1;

	int code = 0;
	if (curcategory == INSCHAR_MainBox && current < chars.n) code = chars.e[current];
	else if (curcategory == INSCHAR_Recent) code = settings->recent.e[current];

	settings->recent.pushnodup(code);
	char utf8[10];
	int len = utf8encode(code, utf8);
	utf8[len] = '\0';
	SimpleMessage *data = new SimpleMessage(utf8, code, curcategory, 0, 0);
	app->SendMessage(data,owner->object_id,"insert char", object_id);

	return 0;
}

int CharacterInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
//	if (ch==LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
//		needtodraw=1;
//		return 0;
//	}

	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action >= 0) {
		return PerformAction(action);
	}

	if (ch==LAX_Pgdown) {
		int mult=1;
		if (state&ControlMask) mult=5;
		if (state&ShiftMask) mult*=5;
		offset.y -= settings->boxwidth*10*mult;

		if (offset.y+bigbox.miny<0 && offset.y+bigbox.maxy+settings->boxwidth<curwindow->win_h) 
			offset.y = curwindow->win_h - (bigbox.maxy + settings->boxwidth);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Pgup) {
		int mult=1;
		if (state&ControlMask) mult=5;
		if (state&ShiftMask) mult*=5;
		offset.y += settings->boxwidth*10*mult;

		if (offset.y+bigbox.maxy>curwindow->win_h && offset.y+bigbox.miny-settings->boxwidth > 0) 
			offset.y = -bigbox.miny+settings->boxwidth;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_End) {
		offset.y = curwindow->win_h - (bigbox.maxy + settings->boxwidth);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Home) {
		offset.y = -bigbox.miny+settings->boxwidth;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Down) {
		int i=current;
		if (current<0) { i=0; curcategory=INSCHAR_MainBox; }
		else if (current+numwide < chars.n) i+=numwide;
		if (i!=current) { current=i; needtodraw=1; }
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Up) {
		int i=current;
		if (current<0) { i=0; curcategory=INSCHAR_MainBox; }
		else if (current-numwide >=0) i-=numwide;
		if (i!=current) { current=i; needtodraw=1; }
		return 0;

	} else if (ch==LAX_Right) {
		int i=current;
		if (current<0) { i=0; curcategory=INSCHAR_MainBox; }
		else if (current%numwide < numwide-1 && current<chars.n-1) i++;
		if (i!=current) { current=i; needtodraw=1; }
		return 0;

	} else if (ch==LAX_Left) {
		int i=current;
		if (current<0) { i=0; curcategory=INSCHAR_MainBox; }
		else if (current%numwide > 0) i--;
		if (i!=current) { current=i; needtodraw=1; }
		return 0;

	} else if (ch==LAX_Enter) {
		send();
		if (owner) owner->RemoveChild();
		return 0;
	}

	return 1; //key not dealt with, propagate to next interface
}

Laxkit::ShortcutHandler *CharacterInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc = manager->NewHandler(whattype());
    if (sc) return sc;

    sc = new ShortcutHandler(whattype());

    sc->Add(CHARI_ToggleUnicode,  'u',ControlMask,0, "ToggleUnicode"  , _("ToggleUnicode"),NULL,0);
    sc->Add(CHARI_Paste,          'v',ControlMask,0, "Paste"          , _("Paste"),NULL,0);
    sc->Add(CHARI_Copy,           'c',ControlMask,0, "Copy"           , _("Copy"),NULL,0);
    sc->Add(CHARI_Search,         '/',0,0, "Search"          , _("Search"),NULL,0);
    //sc->Add(CAPTION_Convert_To_Path, 'P',ShiftMask|ControlMask,0, "ConvertToPaths", _("Convert to path object"),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

int CharacterInterface::PerformAction(int action)
{ 
	if (action == CHARI_ToggleUnicode) {
		settings->show_unicode = !settings->show_unicode;
		needtodraw = 1;
		return 0;

	} else if (action == CHARI_Paste) {
	} else if (action == CHARI_Copy) {
	} else if (action == CHARI_Search) {
	}
	return 1;
}

//! Placeholder for dumping in settings. Currently does nothing.
void CharacterInterface::dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext)
{
	for (int c=0; c<att->attributes.n; c++) {
		if (strEquals(att->attributes.e[c]->name, "boxwidth")) {
			DoubleAttribute(att->attributes.e[c]->value, &(settings->boxwidth));

		} else if (strEquals(att->attributes.e[c]->name, "show_unicode")) {
			settings->show_unicode = BooleanAttribute(att->attributes.e[c]->value);
		}
	}
}

/*! Placeholder for dumping out settings. Default just returns att.
 * Subclasses that do output need to create a new Attribute if att==null, and fill with
 * appropriate settings that will be read back in with dump_in_atts().
 */
Attribute *CharacterInterface::dump_out_atts(Attribute *att,int what,DumpContext *savecontext)
{
	if (!att) att = new Attribute();
	att->push("boxwidth", settings->boxwidth);
	att->push("show_unicode", settings->show_unicode ? "yes": "no");
	return att;
}

} // namespace LaxInterfaces

