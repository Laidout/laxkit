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


#include <lax/lineinput.h>
#include <lax/quickfileopen.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {

/*! \class LineInput
 * \brief A LineEdit with a label.
 *
 * This can be used as a simple one line input popup. Just pass in the style LINP_POPUP
 * with a NULL parent. When enter is hit, the window sends a StrEventData with the text in it to
 * owner, and then the window deletes itself.
 * 
 * <img src="images/lineinput-flat.png">\n
 *
 * <pre>
 *  lx,ly  Coordinates of the label
 *  lew    Preferred width of the edit (<=0 means use remainder after label)
 *  leh    Preferred height (0==use textheight+padlx, <0 is remainder)
 *  padx   Horizontal pad around both the label and edit
 *  pady   Vertical pad around both the label and edit
 *  padlx  Horizontal inset passed to the LineEdit
 *  padly  Vertical inset passed to the LineEdit
 * </pre>
 *
 * \code
 * #define LINP_ONLEFT    (1<<16) <-- put label on the left
 * #define LINP_ONRIGHT   (1<<17) <-- put label on the right 
 * #define LINP_ONTOP     (1<<18) <-- put label on the top 
 * #define LINP_ONBOTTOM  (1<<19) <-- put label on the bottom
 * #define LINP_LEFT      (1<<20) <-- left justify the whole label/LineEdit
 * #define LINP_RIGHT     (1<<21) <-- right justify the whole label/LineEdit
 * #define LINP_CENTER    (1<<22) <-- center justify the whole label/LineEdit ***imp me!
 * #define LINP_POPUP     (1<<23) <-- make SendMessage with a StrEventData, and app->destroywindow(this) when it sends
 * #define LINP_STYLEMASK (1<<16|1<<17|1<<18|1<<19|1<<20|1<<21|1<<22|1<<23)
 * \endcode
 */
/*! \var int LineInput::lx
 * \brief X coordinate of the label.
 */
/*! \var int LineInput::ly
 * \brief Y coordinate of the label.
 */
/*! \var int LineInput::lew
 * \brief The preferred width of the LineEdit portion of the window. Nonpositive means use the remainder.
 */
/*! \var int LineInput::leh
 * \brief The preferred height of the LineEdit portion of the window. (see below)
 *
 *   leh<0 means use remainder, \n
 *	 leh=0 means use textheight+padlx\n
 *	 leh>0 is absolute\n
 */
/*! \var double LineInput::padx
 * \brief Horizontal inset for the whole window. Pixel value is padx*scaled_textheight.
 */
/*! \var double LineInput::pady
 * \brief Vertical inset for the whole window. Pixel value is pady*scaled_textheight.
 */
/*! \var double LineInput::padlx
 * \brief Horizontal inset for the internal LineEdit.
 */
/*! \var double LineInput::padly
 * \brief Vertical inset for the internal LineEdit.
 */
/*! \fn int LineInput::CloseControlLoop()
 * \brief Does: if (le) le->CloseControlLoop();
 */
/*! \fn char *LineInput::GetText()
 * \brief Return a new'd copy of the LineEdit's text.
 */
/*! \fn const char *LineInput::GetCText()
 * \brief Return a const pointer to the LineEdit's text.
 *
 * Use this quickly, before the LineEdit changes it.
 */


//! Constructor, creates the internal LineEdit.
LineInput::LineInput(anXWindow *parnt,const char *nname,const char *ntitle,unsigned int nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsend,
			const char *newlabel,const char *newtext,unsigned int ntstyle,
			int nlew,   //!< Width of the edit box
			int nleh,   //!< Height of the edit box
			double npadx,  //!< Horizontal pad around the whole window
			double npady,  //!< Vertical pad around the whole window
			double npadlx, //!< Horizontal pad to inset text in the edit box
			double npadly) //!< Horizontal pad to inset text in the edit box
					// all after and inc newtext==0
		: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,nowner,nsend)
{
	if ((win_style&(LINP_ONTOP|LINP_ONBOTTOM|LINP_ONLEFT|LINP_ONRIGHT))==0) win_style|=LINP_ONLEFT;
	InstallColors(THEME_Panel);

	padx  = npadx;
	pady  = npady;
	padly = npadly;
	padlx = npadlx;
	label = NULL;
	makestr(label, newlabel);
	auto_labelw = true;
	labelw = 0;
	lew    = nlew;
	leh    = nleh;
	helper = nullptr;

	double scale = UIScale();
	double lw = 0, lh = 0, fasc = 0, fdes = 0, textheight;
	if (label) {
		win_themestyle->normal->Extent(label,-1,&lw,&lh,&fasc,&fdes);
		lw *= scale;
		lh *= scale;
	} else {
		fasc = win_themestyle->normal->ascent();
		fdes = win_themestyle->normal->descent();
	}
	fasc *= scale;
	fdes *= scale;
	textheight = fasc + fdes;

	if (padx  < 0) padx  = .15;
	if (pady  < 0) pady  = .15;
	if (padly < 0) padly = .15;
	if (padlx < 0) padlx = .15;

	char *letitle = nullptr;
	makestr(letitle,win_name);
	appendstr(letitle,"-le");
	unsigned long extrastyle = 0;

	if      (win_style & LINP_FILE)      extrastyle |= LINEEDIT_FILE;
	else if (win_style & LINP_FILESAVE)  extrastyle |= LINEEDIT_FILESAVE;
	else if (win_style & LINP_DIRECTORY) extrastyle |= LINEEDIT_DIRECTORY;
	else if (win_style & LINP_INT)       extrastyle |= LINEEDIT_INT;
	else if (win_style & LINP_FLOAT)     extrastyle |= LINEEDIT_FLOAT;

	if (win_style & LINP_SEND_ANY)  extrastyle |= LINEEDIT_SEND_ANY_CHANGE;
	
	le = new LineEdit(this,letitle,NULL,extrastyle|(nstyle&~(ANXWIN_ESCAPABLE|ANXWIN_CENTER|LINP_STYLEMASK)), 0,0, 0,0, 1, 
			prev,win_owner,nsend,
			newtext,ntstyle);
	le->padx = padlx;
	le->pady = padly;
	delete[] letitle;


	if (extrastyle & (LINEEDIT_FILE | LINEEDIT_FILESAVE | LINEEDIT_DIRECTORY)) {
		//DBG cerr << "Adding helper for file LineInput"<<endl;

		helper = new QuickFileOpen(this,"file helper",nullptr,0, 0,0,0,0, 1,
                          nullptr,object_id,"helper",
                          (extrastyle & LINEEDIT_FILE) ? FILES_OPEN_ONE : FILES_SAVE,
                          this);
	}

	if (leh == 0) leh = 2*padly*textheight + textheight; // set to textheight always, not remainder
	
	 // set win_w and win_h for this window
	if (win_h <= 1) { // wrap height to textheight+pads
		int nh = textheight + 2 * padly * textheight;
		if (nleh > nh) nh = nleh;
		win_h = nh + 2 * brder + 2 * pady * textheight;
		if (win_style & (LINP_ONTOP | LINP_ONBOTTOM)) win_h += padly * textheight + textheight;
	} 
	if (win_w <= 1) {
		win_w = 2*padlx*textheight + 2*padx*textheight + 2*brder; // set win_w equal to the pads
		if (win_style & (LINP_ONLEFT|LINP_ONRIGHT)) { // to win_w add lew and lw
			if (lew > 0) win_w += padx*textheight + lew + lw;
			else win_w += padlx*textheight + lw + lw; // make edit width same as label width
		} else { // to win_w add the greater of lw or nlew
			if (nlew > lw) win_w += nlew; else win_w += lw;
		}
		if (helper) win_w += helper->win_w;
	}

	SetPlacement(); // sets lx,ly, le size and place
	needtodraw = 1;
}

LineInput::~LineInput()
{
	if (label) delete[] label;
}

/*! Set win_w and win_h to just enough to contain contents.
 */
void LineInput::WrapToExtent()
{
	double scale = UIScale();
	double lw = 0, lh = 0, fasc = 0, fdes = 0, textheight;
	if (label) {
		win_themestyle->normal->Extent(label,-1,&lw,&lh,&fasc,&fdes);
		lw *= scale;
		lh *= scale;
	} else {
		fasc = win_themestyle->normal->ascent();
		fdes = win_themestyle->normal->descent();
	}
	fasc *= scale;
	fdes *= scale;
	textheight = fasc + fdes;

	if (padx  < 0) padx  = .15;
	if (pady  < 0) pady  = .15;
	if (padly < 0) padly = .15;
	if (padlx < 0) padlx = .15;
	
	if (!auto_labelw) lw = labelw;
	else if (Label()) {
		win_themestyle->normal->Extent(Label(),-1,&lw,nullptr,nullptr,nullptr); //&lh,&fasc,&fdes);
		lw *= UIScale();
	}

	 // set win_w and win_h for this window
	int brder = le->win_border;
	if (brder < 0) brder = 0;
	if (win_h <= 1) { // wrap height to textheight+pads
		int nh = textheight + 2 * padly * textheight;
		if (le->win_h > nh) nh = le->win_h;
		win_h = nh + 2 * brder + 2 * pady * textheight;
		if (win_style & (LINP_ONTOP | LINP_ONBOTTOM)) win_h += padly * textheight + textheight;
	} 
	if (win_w <= 1) {
		win_w = 2*padlx*textheight + 2*padx*textheight + 2*brder; // set win_w equal to the pads
		if (win_style & (LINP_ONLEFT|LINP_ONRIGHT)) { // to win_w add lew and lw
			if (lew > 0) win_w += padx*textheight + lew + lw;
			else win_w += padlx*textheight + lw + lw; // make edit width same as label width
		} else { // to win_w add the greater of lw or nlew
			int nlew = le->win_w;
			if (nlew > lw) win_w += nlew; else win_w += lw;
		}
		if (helper) win_w += helper->win_w;
	}	
}

void LineInput::Qualifier(const char *nqualifier)
{
	if (le) le->Qualifier(nqualifier);
}

void LineInput::SetType(unsigned long what)
{
	if (le) le->SetType(what);
}

//! Set same tip for le.
const char *LineInput::tooltip(const char *ntip)
{
	anXWindow::tooltip(ntip);
	if (le) le->tooltip(ntip);
	return anXWindow::tooltip();
}

//! Take any StrEventData and set thetext from it.
int LineInput::Event(const EventData *data,const char *mes)
{
	if (dynamic_cast<const StrEventData *>(data)) {
		if (!strcmp(mes,"lineinput-popup-style")) {
			if (!win_owner || !(win_style&LINP_POPUP)) return 1;
			
			 //send the text, then destroy this window
			char *str=GetText();
			if (!str) return 0;
			StrEventData *sendd=new StrEventData();
			sendd->str=str;
			app->SendMessage(sendd,win_owner,win_sendthis,object_id);
			app->destroywindow(this);
			return 0;

		} else if (!strcmp(mes,"helper")) {
			const StrEventData *sdata = dynamic_cast<const StrEventData *>(data);
			SetText(sdata->str);
			le->Modified(1);
			return 0;
		}
	}

	return anXWindow::Event(data, mes);
}

//! Return pointer to the internal LineEdit.
/*! Beware using this! Should really be used only to modify things like initial
 * cursor position and colors.
 */
LineEdit *LineInput::GetLineEdit()
{ 
	return le;
}

//! Just returns le->GetLong(error_ret).
long LineInput::GetLong(int *error_ret)
{ return le->GetLong(error_ret); }

//! Just returns le->GetDouble(error_ret).
double LineInput::GetDouble(int *error_ret)
{ return le->GetDouble(error_ret); }


double LineInput::LabelWidth()
{
	return labelw;
}

/*! Pass in -1 to use auto width.
 */
double LineInput::LabelWidth(double newwidth)
{
	labelw = newwidth;
	auto_labelw = (labelw < 0);
	SetPlacement();
	return labelw;
}

//! Set the label (not the internal edit text).
void LineInput::Label(const char *newlabel)
{
	if (newlabel) {
		makestr(label,newlabel);
		SetPlacement();
	}
}

const char *LineInput::Label()
{
	return label;
}

bool LineInput::LabelOnLeft() //true: (label) (stuff),  false: (stuff) (label
{
	return (win_style & LINP_ONLEFT) != 0;
}

bool LineInput::LabelOnLeft(bool onLeft)
{
	win_style = (win_style & ~(LINP_ONLEFT | LINP_ONRIGHT)) | (onLeft ? LINP_ONLEFT : LINP_ONRIGHT);
	return (win_style & LINP_ONLEFT) != 0;
}

int LineInput::LabelAlignment() //how to align label with the label width. LAX_LEFT, LAX_RIGHT, or LAX_CENTER
{
	if (win_style & LINP_LEFT)   return LAX_LEFT;
	if (win_style & LINP_CENTER) return LAX_CENTER;
	if (win_style & LINP_RIGHT)  return LAX_RIGHT;
	return LAX_CENTER;
}

int LineInput::LabelAlignment(int newAlignment)
{
	win_style &= ~(LAX_LEFT | LAX_RIGHT | LAX_CENTER);
	if (newAlignment == LAX_LEFT) win_style |= LAX_LEFT;
	else if (newAlignment == LAX_RIGHT) win_style |= LAX_RIGHT;
	else win_style |= LAX_CENTER;

	return win_style & (LAX_LEFT | LAX_RIGHT | LAX_CENTER);
}


//! Set the text according to the given integer.
void LineInput::SetText(int newtext)
{
	char num[15];
	sprintf(num,"%d",newtext);
	SetText(num);
}

//! Set the text according to the given floating point number.
void LineInput::SetText(double newtext)
{
	char num[30];
	sprintf(num,"%.10g",newtext);
	SetText(num);
}

//! Set the text of the internal LineEdit.
void LineInput::SetText(const char *newtext)
{
	le->SetText(newtext);
}

//! Sets the placement of the label and LineEdit.
/*! Called when the window is resized.
 *
 * assumes that win_w, win_h, lew,leh set, and le exists\n
 * sets lx,ly, le->x,y,w,h
 */
void LineInput::SetPlacement()
{
	int    lex = 0, ley = 0, nlew = 0, nleh = 0;
	double lw = 0,  lh = 0,  fasc = 0, fdes = 0, textheight;
	double scale = UIScale();
	if (label) win_themestyle->normal->Extent(label,-1,&lw,&lh,&fasc,&fdes);
	lw   *= scale;
	lh   *= scale;
	fasc *= scale;
	fdes *= scale;

	if (auto_labelw) labelw = lw;
	textheight = fasc+fdes;
	

	if (win_style&(LINP_ONTOP|LINP_ONBOTTOM)) { // assume h centered
		if (lew>0 && !auto_labelw) nlew=lew;
		else nlew = win_w - 2*padlx*textheight - 2*le->WindowBorder();
		if (nlew+2*(int)le->WindowBorder() > win_w-2*padlx*textheight) nlew=win_w-2*padlx*textheight-2*le->WindowBorder();
			
		if (leh==0) nleh = 2*pady*textheight+textheight;
		else if (leh<0) nleh = win_h-2*le->WindowBorder()-3*padly*textheight-lh;
		else nleh = leh;
				
		lex=win_w/2-nlew/2;
		lx=win_w/2-lw/2;
		if (win_style&LINP_ONTOP) {
			ley=win_h-padly*textheight-nleh;
			ly=ley-padly*textheight-fdes;
		} else { // ONBOTTOM
			ley=padly*textheight;
			ly=ley+nleh+padly*textheight+fasc;
		}
		int oldley=ley;
		ley=(win_h-nleh-lh-padly*textheight)/2+lh+padly*textheight;
		ly+=(ley-oldley);

	} else if (win_style & (LINP_ONLEFT|LINP_ONRIGHT)) {
		if (lew > 0 && !auto_labelw) nlew = lew;
		else nlew = win_w - 3*padlx*textheight - 2*le->WindowBorder() - labelw;
		if  (nlew > win_w - 3*padlx*textheight - 2*le->WindowBorder() - labelw)
			 nlew = win_w - 3*padlx*textheight - 2*le->WindowBorder() - labelw;
			
		if (leh == 0) nleh = 2*pady*textheight + textheight;
		else if (leh < 0) nleh = win_h - 2*le->WindowBorder() - 3*padly*textheight;
		else nleh = leh;
		
		ley = padly*textheight;
		ly = le->WindowBorder() + nleh/2 - textheight/2 + fasc;

		if (win_style & LINP_ONRIGHT) { // [line edit] label
			lex = padlx*textheight;
			lx  = (padlx*textheight + nlew) + 2 * le->WindowBorder() + padlx*textheight;
			if (win_style & LINP_RIGHT) lx = win_w - padlx*textheight - lw;
			else if (win_style & LINP_CENTER) lx = win_w - padlx*textheight - labelw/2 - lw/2;

		} else { // ONLEFT   label [line edit]
			lex = win_w - padlx*textheight - 2 * le->WindowBorder() - nlew;

			if (win_style & LINP_RIGHT) lx = lex - padlx*textheight - lw;
			else if (win_style & LINP_CENTER) lx = lex - padlx*textheight - labelw/2 - lw/2;
			else lx  = padlx*textheight;
		}

		//int oldley = ley;
		ley = (win_h - nleh) / 2;
		ly += ley;
	}

	if (helper) {
		nlew -= helper->win_w;
		helper->MoveResize(lex + nlew,ley, helper->win_w,nleh);
	}

	le->MoveResize(lex,ley,nlew,nleh);
	
	DBG cerr <<"SetPlacement "<<WindowTitle()<<": le of LineInput: x,y:"<<le->win_x<<","<<le->win_y
	DBG 	<<"  lex,ley;"<<lex<<","<<ley<<"  w,h:"<<nlew<<","<<nleh<<endl;
	DBG cerr <<"-SetPlacement: lx,ly: "<<lx<<','<<ly<<"  lw,lh:"<<lw<<','<<lh<<endl;
	DBG cerr <<"-SetPlacement: win_w,win_h: "<<win_w<<','<<win_h<<endl;

	needtodraw=1;
}

//! Adds the internal LineEdit.
/*! Also for LINP_POPUPs, the owner and message of the internal edit
 * are set to this->window, and "lineinput-popup-style". In this case, when the LineInput
 * gets the corresponding ClientMessage event, it then sends a StrEventData with a
 * copy of le->thetext to the real owning window.
 */
int LineInput::init()
{
	app->addwindow(le);
	if (win_style&LINP_POPUP) { // sends StrEventData instead of simple atom
		le->SetOwner(this,"lineinput-popup-style");
	}
	app->addwindow(helper);
	return 0;
}

/*! \todo *** transfer focus to le
 */
int LineInput::FocusOn(const FocusChangeData *e)
{
	return anXWindow::FocusOn(e);
}

//! Just return anXWindow::FocusOff(e);
int LineInput::FocusOff(const FocusChangeData *e)
{
	return anXWindow::FocusOff(e);
}

//! Redraw label.
void LineInput::Refresh()
{
	if (!win_on || !needtodraw) { needtodraw=1; return; }
	
	DBG cerr <<"LineInput "<<WindowTitle()<<": le of LineInput: x,y:"<<le->win_x<<","<<le->win_y<<", font size: "
		<<win_themestyle->normal->textheight()<<" uiscale: "<<UIScale()<<endl;
	
	if (label) {
		Displayer *dp = MakeCurrent();
		dp->ClearWindow();
		dp->NewFG(win_themestyle->fg);
		dp->font(win_themestyle->normal, UIScale() * win_themestyle->normal->textheight());
		dp->textout(lx,ly, label,strlen(label), LAX_LEFT|LAX_BASELINE);
	}
	needtodraw=0;
}

//! Calls anXWindow::MoveResize(nx,ny,nw,nh) then SetPlacement().
int LineInput::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	SetPlacement();
	return 0;
}

//! Calls anXWindow::Resize(nw,nh) then SetPlacement().
int LineInput::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	SetPlacement();
	return 0;
}


void LineInput::UIScaleChanged()
{
	anXWindow::UIScaleChanged(); //this sends same to children
	leh = -1;
	SetPlacement();
	needtodraw = 1;
}


Attribute *LineInput::dump_out_atts(Attribute *att,int what,DumpContext *savecontext)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,savecontext);

	if (what==-1) {
		le->dump_out_atts(att,-1,savecontext);
		att->push("label","string");
		att->push("labelplacement","one of: left top bottom right");
		return att;
	}

	att->push("win_name",win_name);
	att->push("label",label);

	if      (win_style&LINP_ONLEFT)   att->push("labelplacement","left");
	else if (win_style&LINP_ONTOP)    att->push("labelplacement","top");
	else if (win_style&LINP_ONBOTTOM) att->push("labelplacement","bottom");
	else if (win_style&LINP_ONRIGHT)  att->push("labelplacement","right");

	le->dump_out_atts(att,what,savecontext);

	return att;
}

void LineInput::dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext)
{
	anXWindow::dump_in_atts(att,flag,loadcontext);
	le->dump_in_atts(att,flag,loadcontext);

	const char *name, *value;
	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp("win_name",name)) {
			makestr(win_name,value);

		} else if (!strcmp("label",name)) {
			makestr(label,value);

		} else if (!strcmp("labelplacement",name)) {
			win_style &= ~(LINP_ONLEFT | LINP_ONTOP | LINP_ONBOTTOM | LINP_ONRIGHT);
			if (!strcmp(value,"left"))        win_style |= LINP_ONLEFT;
			else if (!strcmp(value,"top"))    win_style |= LINP_ONTOP;
			else if (!strcmp(value,"bottom")) win_style |= LINP_ONBOTTOM;
			else if (!strcmp(value,"right"))  win_style |= LINP_ONRIGHT;

		//else if (!strcmp("font",name)) ***;
		}
	}
}

} // namespace Laxkit

