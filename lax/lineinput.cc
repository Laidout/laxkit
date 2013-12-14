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


#include <lax/lineinput.h>
#include <lax/laxutils.h>


#include <iostream>
using namespace std;
#define DBG 


using namespace LaxFiles;

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
 *
 * \todo *** fix crash on NULL label...
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
/*! \var int LineInput::padx
 * \brief Horizontal inset for the whole window.
 */
/*! \var int LineInput::pady
 * \brief Vertical inset for the whole window.
 */
/*! \var int LineInput::padlx
 * \brief Horizontal inset for the internal LineEdit.
 */
/*! \var int LineInput::padly
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
			int npadx,  //!< Horizontal pad around the whole window
			int npady,  //!< Vertical pad around the whole window
			int npadlx, //!< Horizontal pad to inset text in the edit box
			int npadly) //!< Horizontal pad to inset text in the edit box
					// all after and inc newtext==0
		: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,NULL,nowner,nsend)
{
	if ((win_style&(LINP_ONTOP|LINP_ONBOTTOM|LINP_ONLEFT|LINP_ONRIGHT))==0) win_style|=LINP_ONLEFT;
	padx=npadx;
	pady=npady;
	padly=npadly;
	padlx=npadlx;
	label=NULL;
	makestr(label,newlabel);
	lew=nlew;
	leh=nleh;

	double lw=0,lh=0,fasc=0,fdes=0,textheight;
	if (label) getextent(label,-1,&lw,&lh,&fasc,&fdes);
	else {
		fasc=app->defaultlaxfont->ascent();
		fdes=app->defaultlaxfont->descent();
	}
	textheight=fasc+fdes;

	if (padx==-1)  padx =textheight*.15;
	if (pady==-1)  pady =textheight*.15;
	if (padly==-1) padly=textheight*.15;
	if (padlx==-1) padlx=textheight*.15;
	
	char *letitle=NULL;
	makestr(letitle,win_name);
	appendstr(letitle,"-le");
	unsigned long extrastyle=0;

	if (win_style&LINP_FILE) extrastyle|=LINEEDIT_FILE;
	else if (win_style&LINP_INT) extrastyle|=LINEEDIT_INT;
	else if (win_style&LINP_FLOAT) extrastyle|=LINEEDIT_FLOAT;
	
	le=new LineEdit(this,letitle,NULL,extrastyle|(nstyle&~(ANXWIN_CENTER|LINP_STYLEMASK)), 0,0, 0,0, brder?brder:1, 
			prev,win_owner,nsend,
			newtext,ntstyle);
	DBG cerr <<"lineinput new LineEdit style: "<<le->win_style<<endl;
	le->padx=padlx;
	le->pady=padly;
	delete[] letitle;

	if (leh==0) leh=2*padly+textheight; // set to textheight always, not remainder
	
	 // set win_w and win_h for this window
	if (win_h<=1) { // wrap height to textheight+pads
		int nh=textheight+2*padly;
		if (nleh>nh) nh=nleh;
		win_h=nh+2*brder+2*pady;
		if (win_style&(LINP_ONTOP|LINP_ONBOTTOM)) win_h+=padly+textheight;
	} 
	if (win_w<=1) {
		win_w=2*padlx+2*padx+2*brder; // set win_w equal to the pads
		if (win_style&(LINP_ONLEFT|LINP_ONRIGHT)) { // to win_w add lew and lw
			if (lew>0) win_w+=padx+lew+lw;
			else win_w+=padlx+lw+lw; // make edit width same as label width
		} else { // to win_w add the greater of lw or nlew
			if (nlew>lw) win_w+=nlew; else win_w+=lw;
		}
	}

	installColors(app->color_panel);

	SetPlacement(); // sets lx,ly, le size and place
	
	needtodraw=1;
}

LineInput::~LineInput()
{ if (label) delete[] label; }

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
	if (!dynamic_cast<const StrEventData *>(data)) return 1;

	if (strcmp(mes,"lineinput-popup-style")) {
		if (!win_owner || !(win_style&LINP_POPUP)) return 1;
		
		 //send the text, then destroy this window
		char *str=GetText();
		if (!str) return 0;
		StrEventData *sendd=new StrEventData();
		sendd->str=str;
		app->SendMessage(sendd,win_owner,win_sendthis,object_id);
		app->destroywindow(this);
		return 0;
	}

	SetText(dynamic_cast<const StrEventData *>(data)->str);
	return 0;
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


//! Set the label (not the internal edit text).
void LineInput::SetLabel(const char *newlabel)
{
	if (newlabel) {
		makestr(label,newlabel);
		SetPlacement();
	}
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
	int lex=0,ley=0,nlew=0,nleh=0;
	double lw=0,lh=0,fasc=0,fdes=0,textheight;
	if (label) getextent(label,-1,&lw,&lh,&fasc,&fdes);
	textheight=fasc+fdes;
	
	if (win_style&(LINP_ONTOP|LINP_ONBOTTOM)) { // assume h centered
		if (lew>0) nlew=lew;
		else nlew=win_w-2*padlx-2*le->win_border;
		if (nlew+2*(int)le->win_border>win_w-2*padlx) nlew=win_w-2*padlx-2*le->win_border;
			
		if (leh==0) nleh=2*pady+textheight;
		else if (leh<0) nleh=win_h-2*le->win_border-3*padly-lh;
		else nleh=leh;
				
		lex=win_w/2-nlew/2;
		lx=win_w/2-lw/2;
		if (win_style&LINP_ONTOP) {
			ley=win_h-padly-nleh;
			ly=ley-padly-fdes;
		} else { // ONBOTTOM
			ley=padly;
			ly=ley+nleh+padly+fasc;
		}
		int oldley=ley;
		ley=(win_h-nleh-lh-padly)/2+lh+padly;
		ly+=(ley-oldley);
	} else if (win_style&(LINP_ONLEFT|LINP_ONRIGHT)) {
		if (lew>0) nlew=lew;
		else nlew=win_w-3*padlx-2*le->win_border-lw;
		if (nlew>win_w-3*padlx-2*(int)le->win_border-lw) nlew=win_w-3*padlx-2*le->win_border-lw;
			
		if (leh==0) nleh=2*pady+textheight;
		else if (leh<0) nleh=win_h-2*le->win_border-3*padly;
		else nleh=leh;
		
		ley=padly;
		ly=padly+padx+le->win_border+fasc;
		if (win_style&LINP_ONRIGHT) {
			lex=padlx;
			lx=padlx+nlew+2*le->win_border+padlx;
		} else { // ONLEFT
			lex=win_w-padlx-2*le->win_border-nlew;
			lx=lex-padlx-lw;
		}
		int oldley=ley;
		ley=(win_h-nleh)/2;
		ly+=(ley-oldley);
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
	
	DBG cerr <<"LineInput "<<WindowTitle()<<": le of LineInput: x,y:"<<le->win_x<<","<<le->win_y<<endl;
	
	if (label) {
		clear_window(this);
		foreground_color(win_colors->fg);
		textout(this, label,strlen(label),lx,ly,LAX_LEFT|LAX_BASELINE);
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

Attribute *LineInput::dump_out_atts(Attribute *att,int what,Laxkit::anObject *savecontext)
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

	if (win_style&LINP_ONLEFT) att->push("labelplacement","left");
	else if (win_style&LINP_ONTOP) att->push("labelplacement","top");
	else if (win_style&LINP_ONBOTTOM) att->push("labelplacement","bottom");
	else if (win_style&LINP_ONRIGHT) att->push("labelplacement","right");

	le->dump_out_atts(att,what,savecontext);

	return att;
}

void LineInput::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *loadcontext)
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
			win_style&=~LINP_ONLEFT|LINP_ONTOP|LINP_ONBOTTOM|LINP_ONRIGHT;
			if (!strcmp(value,"left")) win_style|=LINP_ONLEFT;
			else if (!strcmp(value,"top")) win_style|=LINP_ONTOP;
			else if (!strcmp(value,"bottom")) win_style|=LINP_ONBOTTOM;
			else if (!strcmp(value,"right")) win_style|=LINP_ONRIGHT;

		//else if (!strcmp("font",name)) ***;
		}
	}
}

} // namespace Laxkit

