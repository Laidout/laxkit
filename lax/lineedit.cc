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
//    Copyright (c) 2004-2015 Tom Lechner
//


#include <lax/lineedit.h>
#include <lax/fileutils.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/mouseshapes.h>

#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//--------------------------------------------------------------------------------
/*! \class LineEdit
 * \brief A generic single line text edit.
 *
 * If there happen to be multiple keyboards focused on the edit, they all change the
 * same point and the same selection. Nor are multiple mice allowed different selection areas.
 *
 * Can use l/r/c/char tabs at arbitrary spacing, and l/r/c justification.
 * The right and center justified styles replace the tabs with spaces when
 * the text is displayed (not in the actual text).
 * 
 * \code
 *  // this is to require that cntl-tab be pressed to go to next control
 *  // otherwise just tab does it
 * LINEEDIT_CNTLTAB_NEXT
 * 
 *  // Because it is so convenient, the following require that thetext be
 *  // equivalent to an int/float. When the focus goes off, or enter is
 *  // pressed, it does a check to ensure this.***TODO!!!
 * LINEEDIT_INT
 * LINEEDIT_FLOAT
 *  
 *  // The text is a file name, and the background color should change depending on
 *  // if the file exist, and what sort of file it is..
 * LINEEDIT_FILE
 * LINEEDIT_DIRECTORY
 *
 *  //do a send(0) whenever the text changes at all, not just on enter
 *  //sends whenever Modified(parameter!=0) is called
 * LINEEDIT_SEND_ANY_CHANGE
 * \endcode
 *
 * \todo *** non actual moves must deselect selection
 * \todo ***undo??? or be able to do 'esc' to revert to what edit was before focus?
 * \todo *** RBDown should shift screen
 * \todo *** maybe put in margins+pad
 * \todo you know, this could be abstracted so the underlying text functions of TextEditBase
 *   and TextXEditBase could be an object rather than a parent class.. would make swapping out
 *   the back end easier, so wouldn't have to rewrite the interface from scratch each time..
 *   thinking of using pango, also what about interface tools that use text? have to rewrite
 *   the whole character input mechanism again?!?
 *
 */
/*! \var char *LineEdit::qualifier
 * \brief Can be used during thetext validation.
 *
 * For LINEEDIT_FILE, this is an extra path to prepend to thetext before 
 * checking for file existence.
 */
/*! \var char *LineEdit::label
 * \brief Optional label for the edit.
 *
 * \todo *** combine LineEdit and LineInput??
 */
/*! \var char *LineEdit::revertto
 * \brief Kind of an undo buffer. On escape, revert thetext to this.
 *
 * \todo *** implement me!!
 */



//! Constructor. newtext should be Utf8 text.
LineEdit::LineEdit(anXWindow *parnt,const char *nname,const char *ntitle,unsigned int nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsend,
			const char *newtext,
			unsigned int ntstyle) //!< See TextEditBaseUtf8 for what goes in this.
			 : TextXEditBaseUtf8(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend,newtext,ntstyle,'\\')
{ 			// ntstyle=0, newtext=NULL, cntlchar=0; newtext copied
	qualifier = NULL;
	blanktext = NULL;
	lasthover = 0;
	
	InstallColors(THEME_Edit);

	bkwrongcolor  = coloravg(rgbcolor(255,0,0),win_themestyle->bg.Pixel(),.7);
	bkwrongcolor2 = coloravg(rgbcolor(255,255,0),win_themestyle->bg.Pixel(),.7);
	wscolor       = coloravg(coloravg(win_themestyle->fg.Pixel(),win_themestyle->bg.Pixel()), rgbcolor(0,0,255));
	
	con = cx = cy = 0;
	newline = newline2 = 0;

	textheight = win_themestyle->normal->textheight();
	padx = textheight * .2;
	pady = textheight * .2;
	firsttime = 1;
	needtodraw = 1;

	modified = 0;
	//thetext=NULL;
	//SetText(newtext);
	//maxtextlen=0;

	sellen = curpos = selstart = 0;
	oldsellen = 0;
	curlineoffset = -padx;
	dpos = 0;
	mostpixwide = 10;

	valid = 1;

	win_pointer_shape=LAX_MOUSE_Text;
}

//! Empty destructor.
LineEdit::~LineEdit()
{
	delete[] blanktext;
	delete[] qualifier;
}

int LineEdit::init()
{
	TextXEditBaseUtf8::init();
	curlineoffset = -padx;
	if (!win_owner) SetOwner(win_parent);
	return 0;
}

Attribute *LineEdit::dump_out_atts(Attribute *att,int what,DumpContext *savecontext)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,savecontext);

	if (what==-1) {
		att->push("win_name","string");
		att->push("text","string");
		att->push("blanktext","(string) When actual text is nothing, show this, but remove when anything else typed.");
		att->push("type","one of: int real file");
		att->push("align","one of: left center right");
		return att;
	}

	att->push("win_name",win_name);
	att->push("text",GetCText());
	if (!isblank(blanktext)) att->push("blanktext",blanktext);

	if (win_style&LINEEDIT_INT) att->push("type","int");
	else if (win_style&LINEEDIT_FLOAT) att->push("type","real");
	else if (win_style&LINEEDIT_FILE) att->push("type","file");
	else if (win_style&LINEEDIT_DIRECTORY) att->push("type","directory");
	else if (win_style&LINEEDIT_PASSWORD) att->push("type","password");
	else if (win_style&LINEEDIT_DATE) att->push("type","date");
	else if (win_style&LINEEDIT_TIME) att->push("type","time");

	if (textstyle&TEXT_LEFT) att->push("align","left");
	else if (textstyle&TEXT_CENTER) att->push("align","center");
	else if (textstyle&TEXT_RIGHT) att->push("align","right");

	return att;
}

void LineEdit::dump_in_atts(Attribute *att,int flag,DumpContext *loadcontext)
{
	anXWindow::dump_in_atts(att,flag,loadcontext);

	const char *name, *value;
	for (int c=0; c<att->attributes.n; c++) {
		name =att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp("win_name",name)) makestr(win_name,value);
		else if (!strcmp("text",name)) SetText(value);
		else if (!strcmp("blanktext",name)) makestr(blanktext,value);
		else if (!strcmp(name,"type")) {
			win_style&=~(LINEEDIT_INT|LINEEDIT_FLOAT|LINEEDIT_FILE|LINEEDIT_DIRECTORY|LINEEDIT_PASSWORD|LINEEDIT_DATE|LINEEDIT_TIME);
			if (!strcmp(value,"int")) win_style|=LINEEDIT_INT;
			else if (!strcmp(value,"real")) win_style|=LINEEDIT_FLOAT;
			else if (!strcmp(value,"file")) win_style|=LINEEDIT_FILE;
			else if (!strcmp(value,"directory")) win_style|=LINEEDIT_DIRECTORY;
			else if (!strcmp(value,"password")) win_style|=LINEEDIT_PASSWORD;
			else if (!strcmp(value,"date")) win_style|=LINEEDIT_DATE;
			else if (!strcmp(value,"time")) win_style|=LINEEDIT_TIME;

		} else if (!strcmp(name,"align")) {
			textstyle&=~TEXT_LEFT|TEXT_CENTER|TEXT_RIGHT;
			if (!strcmp(value,"left")) textstyle|=TEXT_LEFT;
			else if (!strcmp(value,"center")) textstyle|=TEXT_CENTER;
			else if (!strcmp(value,"right")) textstyle|=TEXT_RIGHT;
		}

		//else if (!strcmp("ispassword",name)) ***;
		//else if (!strcmp("font",name)) ***;
	}
}

void LineEdit::Refresh()
{
	if (!needtodraw) return;
	TextXEditBaseUtf8::Refresh();

	if ((win_style & LINEEDIT_CLEAR_X) && textlen > 0) {
		Displayer *dp=GetDisplayer();
		dp->LineWidth(2);
		if (lasthover==0) {
			dp->NewFG(coloravg(win_themestyle->fg.Pixel(),win_themestyle->bg.Pixel()));
		} else {
			dp->NewFG(1.0,0.,0.);
		}
		dp->drawline(win_w-padx-.75*textheight,win_h/2-textheight/4, 
					 win_w-padx-.25*textheight,win_h/2+textheight/4);
		dp->drawline(win_w-padx-.75*textheight,win_h/2+textheight/4, 
					 win_w-padx-.25*textheight,win_h/2-textheight/4);
		dp->LineWidth(1);
	}

}

int LineEdit::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onMapped && win_style&LINEEDIT_GRAB_ON_MAP) {
		app->setfocus(this,0,NULL);
		//do not return here

	} else if (e->type==LAX_onMouseOut && lasthover!=0) {
		lasthover=0;
		needtodraw=1;
		//do not return here

	}

	return anXWindow::Event(e,mes);
}

//! Change whether the text is valid or not.
void LineEdit::Valid(int v)
{
	if (valid!=v) {
		valid=v;
		needtodraw=1;
	}
}

//! Change whether the text is valid or not, and install a new background color for invalid.
void LineEdit::Valid(int v,unsigned long col)
{
	if (bkwrongcolor!=col) {
		bkwrongcolor=col;
		needtodraw=1;
	}
	if (valid!=v) {
		valid=v;
		needtodraw=1;
	}
}

const unsigned int ValueTypeMask = 
	  LINEEDIT_INT       
	| LINEEDIT_FLOAT     
	| LINEEDIT_FILE      
	| LINEEDIT_FILESAVE  
	| LINEEDIT_DIRECTORY 
	| LINEEDIT_PASSWORD  
	| LINEEDIT_DATE      
	| LINEEDIT_TIME
	;

void LineEdit::SetType(unsigned long what)
{
	win_style &= ~ValueTypeMask;
	if ((what & ValueTypeMask) != 0) SetWinStyle(what, 1);
	Modified(0);
}

void LineEdit::Qualifier(const char *nqualifier)
{
	makestr(qualifier,nqualifier);
	Modified(0);
}

/*! If LINEEDIT_FILE, then check for existence of qualifier/thetext, or
 * just thetext if it seems like an absolute path.
 *
 * Update valid if LINEEDIT_INT or LINEEDIT_FLOAT.
 *
 * This is called whenever text is changed.
 *
 * \todo finish implementing date and time..
 */
int LineEdit::Modified(int m)//m=1
{	
	int v=valid;
	if (thetext && thetext[0]!='\0') {
		if (win_style&LINEEDIT_INT) {
			char *e=NULL;
			strtol(GetCText(),&e,10);
			while (e && isspace(*e)) e++;
			if (*e!='\0') v=0; else v=1;
			
		} else if (win_style&LINEEDIT_FLOAT) {
			char *e=NULL;
			strtod(GetCText(),&e);
			while (e && isspace(*e)) e++;
			if (*e!='\0') v=0; else v=1;
			
		} else if (win_style&(LINEEDIT_FILESAVE|LINEEDIT_FILE|LINEEDIT_DIRECTORY)) {
			char tmp[(thetext?strlen(thetext):0) + 1 + (qualifier?strlen(qualifier):0) + 1];
			if (thetext[0]=='/') sprintf(tmp,"%s",thetext);
			else if (qualifier) sprintf(tmp,"%s/%s",qualifier,thetext);
			else sprintf(tmp,"%s",thetext);

			int type=file_exists(tmp,1,NULL);
			if ((win_style&LINEEDIT_FILE)!=0 && S_ISREG(type)) v=1;
			else if ((win_style&LINEEDIT_FILESAVE)!=0) {
				if (S_ISREG(type)) v=2;
				else if (type==0) v=1;
				else v=0; //is some other kind of file, assume no good
			} else if ((win_style&LINEEDIT_DIRECTORY)!=0 && S_ISDIR(type)) v=1;
			else v=0;
		}
		if (v!=valid) { valid=v; needtodraw=1; }
	} else valid=1;
	if (m && (win_style&LINEEDIT_SEND_ANY_CHANGE)) send(0);
	return modified=m;
}

//! Sends a SimpleMessage event to win_owner.
/*! Puts info1=i and info2=GetLong().
 *
 *  If i==0, then the text was modified.
 *  If i==1, then enter was pressed.
 *  If i==2, then the edit got the focus.
 *  If i==3, then the edit lost the focus.
 *  If i==-1, then this is control notification, and info2==char, info3==state.
 */
int LineEdit::send(int i)
{
	if (!win_owner || !win_sendthis) return 1;

     //set the number value
	int e;
	long v=GetLong(&e);
	SimpleMessage *data=new SimpleMessage(GetText(), i, e?0:v , 0,0, win_sendthis);
	app->SendMessage(data, win_owner, win_sendthis, object_id);
	return 0;
}

/*! Sends a message when focus on if LINEEDIT_SEND_FOCUS_ON.
 * Also select all text.
 */
int LineEdit::FocusOn(const FocusChangeData *e)
{
	int c=TextXEditBaseUtf8::FocusOn(e);
	if (win_active && (win_style&LINEEDIT_SEND_FOCUS_ON)) send(2);
	return c;
}

/*! Sends a message when focus off if LINEEDIT_SEND_FOCUS_OFF.
 * Also deselect any selected text
 */
int LineEdit::FocusOff(const FocusChangeData *e)
{
	int c=TextXEditBaseUtf8::FocusOff(e);
	if (!win_active && (win_style&LINEEDIT_SEND_FOCUS_OFF)) send(3);
	if (lasthover!=0) { lasthover=0; needtodraw=1; }
	return c;
}

void LineEdit::ControlActivation(int on)
{
	if (on) SetSelection(0,textlen); //select all text on focus on
	else if (sellen!=0) sellen=0;
	needtodraw=1;
}

//! Return double value of the text, or 0 if error.
/*! If error_ret is not NULL, then it is set to 0 if all of thetext represents
 * an single double. Nonzero otherwise.
 */
double LineEdit::GetDouble(int *error_ret)
{ 
	if (thetext) { 
		char *endptr;
		double d=strtod(thetext,&endptr);
		if (endptr!=thetext) {
			while (isspace(*endptr)) endptr++;
			if (*endptr=='\0') {
				if (error_ret) *error_ret=0;
				return d;
			}
		}
	}
	
	if (error_ret) *error_ret=1;
	return 0;
}

//! Return a long value of the text or 0 if error.
/*! If error_ret is not NULL, then it is set to 0 if all of thetext represents
 * an single (long) integer. Nonzero otherwise.
 */
long LineEdit::GetLong(int *error_ret)
{
	if (thetext) { 
		char *endptr;
		long d=strtol(thetext,&endptr,10);
		if (endptr!=thetext) {
			while (isspace(*endptr)) endptr++;
			if (*endptr=='\0') {
				if (error_ret) *error_ret=0;
				return d;
			}
		}
	}
	
	if (error_ret) *error_ret=1;
	return 0;
}

//! Replace occurence(s) of what with newstr.
/*! If all==0, then replace only next. If all==1, then replace all
 * occurences of what in thetext. If all==-1, the replace all what
 * only within the selection if any.
 * 
 * \todo finish me!
 */
int LineEdit::Replace(char *newstr,char *what,int all)
{	//***calls replacesel(newstr)...
	needtodraw=1;
	return TextEditBaseUtf8::Replace(newstr,what,all);
}

//! Set the text according to the given integer.
int LineEdit::SetText(int newtext)
{
	char num[15];
	sprintf(num,"%d",newtext);
	return SetText(num);
}

//! Set the text according to the given floating point number.
int LineEdit::SetText(double newtext)
{
	char num[30];
	sprintf(num,"%g",newtext);
	return SetText(num);
}

int LineEdit::SetText(const char *newtext)
{ 
	if (TextEditBaseUtf8::SetText(newtext)) return 1;
	curlineoffset = -padx;
	findcaret();
	Modified(0);
	needtodraw=1;
	return 0;
}

long LineEdit::SetCurpos(long newcurpos) // removes selection
{
	TextEditBaseUtf8::SetCurpos(newcurpos);
	if (firsttime) return curpos;
	findcaret();
	return (makeinwindow()?1:-1);
}

int LineEdit::SetSelection(long newss,long newse)
{ 
	TextEditBaseUtf8::SetSelection(newss,newse);
	findcaret();
	return (makeinwindow()?1:-1);
}

int LineEdit::inschar(unsigned int ch,char a)
{
	dpos=curpos; 
	if (TextEditBaseUtf8::inschar(ch,a)) return 1;
	Modified();
	Setpixwide();
	findcaret();
	return needtodraw|=(makeinwindow()?1:2);
}

int LineEdit::delchar(int bksp)
{
	if (TextEditBaseUtf8::delchar(bksp)) return 1;
	dpos=curpos; 
	Modified();
	Setpixwide();
	findcaret();
	return needtodraw|=(makeinwindow()?1:2);
}

int LineEdit::insstring(const char *blah,int after) //after=0
{
	dpos=curpos; 
	if (TextEditBaseUtf8::insstring(blah,after)) return 1;
	Modified();
	Setpixwide();
	findcaret();
	return needtodraw|=(makeinwindow()?1:2);
}

int LineEdit::delsel()
{
	if (TextEditBaseUtf8::delsel()) return 1;
	Modified();
	dpos=curpos;
	Setpixwide();
	findcaret();
	return needtodraw|=(makeinwindow()?1:2);
}

//int LineEdit::replacesel(char ch) // after=0, newt=NULL
//{***
//}

int LineEdit::replacesel(const char *newt,int after) // after=0, newt=NULL
{
	if (sellen) { if (selstart<curpos) dpos=selstart; else dpos=curpos; }
	if (TextEditBaseUtf8::replacesel(newt,after)) return 1;
	Setpixwide();
	findcaret();
	return needtodraw|=(makeinwindow()?1:2);
}


/*! ^b     ***for debugging, allows setting breakpoint accessible by ^b
 *  ^j     toggle justification between left, center, and right
 *  ^t     toggle textstyle&TEXT_TABS_STOPS
 *  ^.     increase tabwidth
 *  ^,     decrease tabwidth
 *  ^a     select all
 *  ^x     cut
 *  ^X     cut
 *  +del   cut
 *  ^v     paste
 *  ^V     paste
 *  +ins   paste
 *  ^c     copy
 *  ^C     copy
 *  ^ins   copy
 *  ^w     toggle showing of whitespace
 *  ^tab   next control
 *  ^+tab  prev control
 *  del    delete current character
 *  bksp   delete previous character
 *  ^left  back a word or group of whitespace
 *  ^right forward a word or group of whitespace
 *
 *  \todo ESC for reset to a copy of initial buffer
 */
int LineEdit::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d) 
{
	DBG cerr <<"########LineEdit::CharInput"<<endl;

	if (!thefont) {
		cerr <<"No font in "<<WindowTitle()<<"! This shouldn't happen."<<endl;
		return anXWindow::CharInput(ch,buffer,len,state,d);;
	}

	if ((win_style & LINEEDIT_SEND_CONTROLS)
			&& (ch == '\t' || ch == LAX_Up || ch == LAX_Down)) {
		
		SimpleMessage *data=new SimpleMessage(NULL, -1, ch,state, 0, win_sendthis);
		app->SendMessage(data, win_owner, win_sendthis, object_id);

		return 0;
	}

	int c=0;
	if ((ch==LAX_Tab && !(win_style&LINEEDIT_CNTLTAB_NEXT)) 
			|| ((win_style&LINEEDIT_CNTLTAB_NEXT) && (state&ControlMask)))
		return anXWindow::CharInput('\t',NULL,0,state,d);

	if (dp == nullptr) {
		dp = MakeCurrent();
		dp->font(thefont, thefont->textheight());
	}

	if (!(state&ControlMask) && (ch=='\t' || (ch>=32 && ch<255)) && !readonly()) {
		 //insert the character
		if (sellen) replacesel(ch);
		else inschar(ch);
		DBG cerr <<"text: "<<thetext<<endl;
		return 0;
	}
	DBG if (state&ControlMask && ch>=32) cerr <<"cntl-"<<ch<<endl;


	if (ch=='z' && (state&LAX_STATE_MASK)==ControlMask) {
		Undo();
		return 0;

	} else if ((ch=='y' && (state&LAX_STATE_MASK)==(ControlMask))
				|| (ch=='z' && (state&LAX_STATE_MASK)==(ShiftMask|ControlMask))) {
		Redo();
		return 0;
	}


	 // catch control-(char) keys
	if ((state&LAX_STATE_MASK)==ControlMask && ch>=32) switch(ch) { 
		case 'b': 
			DBG cerr <<"breakpoint"<<endl;
			return 1;

		case 'j': { //try to join selected (ascii) characters into appropriate accented characters
				if (CombineChars()==0) needtodraw=1;
				return 0; 
			}
		case 'J': //justification
			DBG cerr <<" justify before:"<<(textstyle&(TEXT_LEFT|TEXT_RIGHT|TEXT_CENTER))<<endl;
			switch (textstyle&(TEXT_LEFT|TEXT_RIGHT|TEXT_CENTER)) {
					  case TEXT_LEFT: textstyle=(textstyle&~TEXT_LEFT)|TEXT_CENTER; break;
					  case TEXT_CENTER: textstyle=(textstyle&~TEXT_CENTER)|TEXT_RIGHT; break;
					  case TEXT_RIGHT: textstyle=(textstyle&~TEXT_RIGHT)|TEXT_LEFT; break;
					  default: return 0;
				}
				DBG cerr <<" justify after:"<<(textstyle&(TEXT_LEFT|TEXT_RIGHT|TEXT_CENTER))<<endl;
				Black(0,cy-textascent,win_w,textheight);
				needtodraw=1;
				Setpixwide();
				findcaret(); 
				makeinwindow();
				return 0;

		case 't': textstyle^=TEXT_TABS_STOPS; needtodraw=1; return 0;
		case '.': tabwidth++; findcaret(); needtodraw=1; return 0;
		case ',': tabwidth--; if (tabwidth<5) tabwidth=5; findcaret(); needtodraw=1; return 0;
		case 'a': // select all
			if (((selstart==0 && curpos==textlen) || (selstart==textlen && curpos==0)) && sellen) sellen=0;
			else { selstart=0; curpos=textlen; sellen=textlen; findcaret(); }
			makeinwindow(); needtodraw=1; return 0;

		case 'x':
		case 'X': ch=LAX_Del; state=ShiftMask; break; // ^x = cut ^x=+del 
		case 'c':
		case 'C': ch=LAX_Ins; state=ControlMask; break; // ^c = copy =^ins
		case 'v':
		case 'V': ch=LAX_Ins; state=ShiftMask; break; // ^v = paste
		case '\\': // toggle how to display control chars 
			DBG cerr <<"toggle hex"<<endl;
			if (textstyle&TEXT_CNTL_BANG) {
				textstyle=(textstyle&~TEXT_CNTL_BANG)|TEXT_CNTL_HEX;
				cntlchar='\\';
			} else if (textstyle&TEXT_CNTL_HEX) {
				textstyle=(textstyle&~TEXT_CNTL_HEX)|TEXT_CNTL_NONE;
			} else {
				textstyle=(textstyle&~TEXT_CNTL_NONE)|TEXT_CNTL_BANG;
				cntlchar='!';
			}
			SetupMetrics();
			findcaret();
			makeinwindow();
			needtodraw=1;
			return 0;

		case 'w': if (state&ShiftMask) {
					  textstyle^=TEXT_SHOW_WHITESPACE; 
					  DBG cerr << "t&ws:"<<(textstyle&TEXT_SHOW_WHITESPACE)<<endl;
					  needtodraw=1;
					  return 0;
				}
	}

	 // standard control keys (without pressing control)
	switch(ch) {
		case 0: return 0; // null
		case LAX_Tab:  // ^tab nextcontrol, ^+tab for prev
			if (state&ShiftMask) SelectPrevControl(d);
			else SelectNextControl(d);
			return 0;

		case LAX_Enter: // send
			send(1);
			if (win_style&LINEEDIT_DESTROY_ON_ENTER) app->destroywindow(this);
			return 0;

//		case LAX_Shift: return 0;// shift
//		case LAX_Control: return 0;// cntl
//		case LAX_Esc: return 0;// esc

		case LAX_Del: if (readonly()) return 0; // del
			if ((state&LAX_STATE_MASK)==ShiftMask) {	// shift+del: cut
				if (!sellen) return 0;
				Copy();
				delsel();
				needtodraw=1;
				return 0;
			}
			if (sellen) delsel(); // ***cn+del=del to endl?
			else if (curpos!=textlen) delchar(0);
			return 0;

		case LAX_Bksp: // bksp
			if (readonly()) return 0; // bksp, ***sh+bk=del line,?
			if (sellen) delsel(); // ***cn+bk=del to begl?
			else delchar(1);
			return 0;

		case LAX_Ins: // ins
			if (state&ControlMask) { // cntl+ins: copy
				if (sellen) Copy();
			} else if (state&ShiftMask) { /*shift+ins: paste*/	
				if (!readonly()) {
					if (!Paste()) selectionPaste(0,"UTF8_STRING");
				}
				needtodraw=1;
			}		
			return 0; // ins

		case LAX_Home: // home 
			if (state&ShiftMask && !sellen) selstart=curpos;
			//if (curpos==0) return 0;
			curpos=0; 
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			findcaret();
			needtodraw|=(makeinwindow()?1:3);
			return 0;

		case LAX_End: // end
			if (state&ShiftMask && !sellen) selstart=curpos;
			//if (curpos==textlen) return 0;
			curpos=textlen;
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			findcaret();
			needtodraw|=(makeinwindow()?1:3);
			return 0;

		case LAX_Left: // left
			//DBG cerr << "lbefore sellen:"<<sellen<<" textlen:"<<textlen<<" curpos:"<<curpos<<" clo:"<<curlineoffset<<" cx:"<<cx<<endl;
			if (state&ShiftMask && !sellen) selstart=curpos;
			if (state&ControlMask) {
				c=0;
				if (curpos==0) return 0;
				curpos=prevpos(curpos);
				while (curpos>0 && c<10 && !isword(curpos))
					{ curpos=prevpos(curpos); c++; }
				while (curpos>0 && isword(prevpos(curpos)))
					{ curpos=prevpos(curpos); }
			} else {
				if (curpos==0) return 0;
				curpos=prevpos(curpos);
			}
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			findcaret();
			needtodraw|=(makeinwindow()?1:3);
			//DBG cerr << "lafter  sellen:"<<sellen<<" textlen:"<<textlen<<" curpos:"<<curpos<<" clo:"<<curlineoffset<<" cx:"<<cx<<endl;
			return 0;

		case LAX_Right: // right
			//DBG cerr << "rbefore sellen:"<<sellen<<" textlen:"<<textlen<<" curpos:"<<curpos<<" clo:"<<curlineoffset<<" cx:"<<cx<<endl;
			if (state&ShiftMask && !sellen) selstart=curpos;
			if (state&ControlMask) {
				while (curpos<textlen && isword(curpos)) curpos=nextpos(curpos);
				while (curpos<textlen && c<10 && !isword(curpos)) { curpos=nextpos(curpos); c++; }
			} else {
				if (curpos==textlen) return 0;
				curpos=nextpos(curpos);
			}
			if (state&ShiftMask) sellen=curpos-selstart;
			else sellen=0;
			findcaret();
			needtodraw|=(makeinwindow()?1:3);
			//DBG cerr << "rafter  sellen:"<<sellen<<" textlen:"<<textlen<<"curpos:"<<curpos<<" clo:"<<curlineoffset<<" cx:"<<cx<<endl;
			return 0;

		default: return anXWindow::CharInput(ch,buffer,len,state,d);
	}

	return anXWindow::CharInput(ch,buffer,len,state,d);
}

//! Set curlineoffset to reflect justification.
/*!  Usually called right before a findcaret, and after a change in thetext
 *  Returns 1 if curlineoffset changed
 */
int LineEdit::Setpixwide() 
{
	DBG cerr <<"---setpixwide clo:"<<curlineoffset<<"  cx="<<cx<<" mostpixwide="<<mostpixwide<<endl;

	mostpixwide=GetExtent(0,textlen,0);
	if (mostpixwide>win_w-2*padx) { // line is too long for window
		if (win_w-padx-(mostpixwide-curlineoffset)>0) // gap on the right
			curlineoffset=win_w-padx-mostpixwide;
		else if (-curlineoffset-padx>0) // gap on the left
			curlineoffset=-padx;
		else return 0; // fits ok
		needtodraw=1;
		return 0;
	}

	 // else whole line fits in window
	if (textstyle&TEXT_LEFT) curlineoffset=-padx;
	else if (textstyle&TEXT_CENTER) curlineoffset=-(win_w-mostpixwide)/2;
	else if (textstyle&TEXT_RIGHT) curlineoffset=-(win_w-padx-mostpixwide);
	
	DBG cerr <<" -----pixwide clo:"<<curlineoffset<<"  cx="<<cx<<" mostpixwide="<<mostpixwide<<endl;
	return 1;
}

//! Changes curlineoffset, cx to be in window.
/*!  Assumes cx,cy already set accurately
 */
int LineEdit::makeinwindow()  
{
	if (cx>=textrect.x+padx && cx<=textrect.x+textrect.width-padx) return 0;
	DBG cerr <<"---makeinwindow clo:"<<curlineoffset<<" cx:"<<cx;
	 // get horizontal position
	int oldclo=curlineoffset;
	if (cx<textrect.x+padx) {
		curlineoffset+=(cx-padx-textrect.x);
		if (-curlineoffset>textrect.x+padx) curlineoffset=-padx-textrect.x;
	} else if (cx>textrect.x+textrect.width-padx-3) {
		curlineoffset+=cx-(textrect.x+textrect.width)+padx+3;
	}
	cx-=(curlineoffset-oldclo);
	needtodraw=1;
	//findcaret();
	DBG cerr <<"        new clo:"<<curlineoffset<<" cx:"<<cx<<endl;
	return 1;
}

//! Find char position at pix from left, independent of curlineoffset
long LineEdit::findpos(int pix)
{ 
	//DBG cerr <<"\nFind:"<<l<<','<<pix;
	if (pix<0) pix=0;

	long newpos = GetPos(0,pix);
	DBG cerr <<"found pos:"<<newpos<<endl;
	return newpos;
}

//! Set cx,cy, the caret coordinates. cy is at font baseline.
/*! This is called after setting curpos, before makeinwindow.
 * Assumes curlineoffset is already set right.
 */
void LineEdit::findcaret()
{ 
	cx = GetExtent(0,curpos,0) - curlineoffset;
	cy = win_h/2 - textheight/2 + textascent;

	DBG cerr <<"foundcaret: "<<cx<<','<<cy<<endl;
}

/*! \todo make finding position go to closest char division, not always to beginning of char
 */
int LineEdit::LBDown(int x,int y, unsigned int state,int count,const LaxMouse *d)
{ 
	buttondown.down(d->id, LEFTBUTTON, x,y);

	//double oldheight = -1;
	if (dp == nullptr) {
		dp = MakeCurrent();
		//oldheight = dp->textheight();
		dp->font(thefont, thefont->textheight());
	}

	if (x>win_w-textheight-padx && (win_style&LINEEDIT_CLEAR_X)) {
		SetText("");
		buttondown.up(d->id, LEFTBUTTON);
		send(0);
		return 0;
	}

	//DBG cerr <<"lineedit before lbdown curpos:"<<curpos<<"  textlen:"<<textlen<<endl;
	count = (count-1)%3+1;
	if (count>2) { // select all
		curpos=textlen;
		selstart=0;
		sellen=curpos-selstart;
		findcaret();
		needtodraw|=3;
		return 0;
	}

	if (count>1) return LBDblClick(x,y,state,d);
	 // desel, addtosel, dragdrop, repos
	long newpos = findpos(x+curlineoffset);
	//DBG cerr <<"lineedit lbdown found newpos:"<<newpos<<"  textlen:"<<textlen<<endl;

	if (state&ShiftMask) { // add to sel
		if (sellen==0) selstart=curpos;
		sellen=newpos-selstart;
	} else { sellen=0; selstart=newpos; }
	//if (newpos==curpos) return 0;
	curpos=newpos;
	findcaret();
	needtodraw|=3;
	//DBG cerr <<"lineedit lbdown found newpos:"<<curpos<<"  textlen:"<<textlen<<endl;
	return 0;
}

//! Double clicking selects a whole word, or whole chunk of whitespace.
int LineEdit::LBDblClick(int x,int y, int state,const LaxMouse *d)
{
	if (dp == nullptr) {
		dp = MakeCurrent();
		dp->font(thefont, thefont->textheight());
	}

	 // select word
	int c=0;
	long newpos=findpos(x+curlineoffset);
	if (sellen) {
		sellen=0;
		curpos=newpos;
		findcaret();
		needtodraw|=3;
		return 0;
	}
	selstart=curpos=newpos;

	if (isword(selstart)) {
		while (selstart>0 && isword(selstart-1)) selstart=prevpos(selstart);
		while (curpos<textlen && isword(curpos)) curpos=nextpos(curpos);
	} else {
		while (selstart>0 && c<10 && !isword(prevpos(selstart)))
			{ selstart=prevpos(selstart); c++; }
		while (curpos<textlen && c<10 && !isword(curpos))
			{ curpos=nextpos(curpos); c++; }
	}
	findcaret();
	dpos=selstart; 
	sellen=curpos-selstart;
	needtodraw|=2;
	return 0;
}

int LineEdit::LBUp(int x,int y, unsigned int state,const LaxMouse *d )
{
	buttondown.up(d->id, LEFTBUTTON);
	return 0;
}

int LineEdit::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (dp == nullptr) {
		dp = MakeCurrent();
		dp->font(thefont, thefont->textheight());
	}

	if (win_style&(LINEEDIT_INT|LINEEDIT_FLOAT)) {
		 //wheel to change specific digits

		if (win_style&(LINEEDIT_INT)) {
			long l=GetLong(NULL);
			l++;
			SetText((int)l);
		} else {
			double d=GetDouble(NULL);
			d++;
			SetText(d);
		}
		Modified();
		return 0;

//		long newpos=findpos(x+curlineoffset);
//		//long newpos=curpos;
//
//		if (newpos==textlen) newpos=textlen-1;
//		if (newpos<0 || newpos>=textlen) return 0;
//
//		if (!isdigit(thetext[newpos])) {
//			if (win_style&(LINEEDIT_INT)) {
//				long l=GetLong(NULL);
//				l++;
//				SetText((int)l);
//			} else {
//				double d=GetDouble(NULL);
//				d++;
//				SetText(d);
//			}
//			return 0;
//		}
//
//		do {
//			int digit=thetext[newpos]-'0';
//			digit++;
//			if (digit==10) {
//				thetext[newpos]='0';
//				newpos--;
//				if (newpos<0 || !isdigit(thetext[newpos])) {
//					SetCurpos(newpos+1);
//					inschar('1');
//					break;
//				}
//			} else {
//				thetext[newpos]=digit+'0';
//			}
//		}
//
//		return 0;
//		------------------
//		int place=0, decimal=0;0
//		int s=0,e=textlen-1;
//		while (isspace(thetext[s])) s++;
//		if (thetext[s]=='-') s++;
//		while (e>0 && isspace(thetext[e])) e--;
//		if (e<s) return 0;
//
//		decimal=s;
//		while (decimal<e && thetext[decimal]!='.') decimal++;
//		if (decimal==e) place=decimal-newpos-1;
//		else if (newpos<decimal) place=decimal-newpos-1;
//		else place=decimal-newpos;
//
//		if (win_style&(LINEEDIT_INT)) {
//			long l=GetLong(NULL);
//			l+=(pow10(place)+.5);
//			SetText((int)l);
//		} else {
//			double d=GetDouble(NULL);
//			d+=pow10(place);
//			SetText(d);
//		}
//		//curpos=newpos;
//		findcaret();
//		return 0;
	}

	return 1;
}

int LineEdit::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (dp == nullptr) {
		dp = MakeCurrent();
		dp->font(thefont, thefont->textheight());
	}

	if (win_style&(LINEEDIT_INT|LINEEDIT_FLOAT)) {
		 //wheel to change specific digits

		if (win_style&(LINEEDIT_INT)) {
			long l=GetLong(NULL);
			l--;
			SetText((int)l);
		} else {
			double d=GetDouble(NULL);
			d--;
			SetText(d);
		}
		Modified();
		return 0;

//		long newpos=findpos(x+curlineoffset);
//		//long newpos=curpos;
//
//		if (newpos==textlen) newpos=textlen-1;
//		if (newpos<0 || newpos>=textlen) return 0;
//		int place=0, decimal=0;
//		int s=0,e=textlen-1;
//		while (isspace(thetext[s])) s++;
//		if (thetext[s]=='-') s++;
//		while (e>0 && isspace(thetext[e])) e--;
//		if (e<s) return 0;
//
//		decimal=s;
//		while (decimal<e && thetext[decimal]!='.') decimal++;
//		if (decimal==e) place=decimal-newpos-1;
//		else if (newpos<decimal) place=decimal-newpos-1;
//		else place=decimal-newpos;
//
//		if (win_style&(LINEEDIT_INT)) {
//			long l=GetLong(NULL);
//			l-=(pow10(place)+.5);
//			SetText((int)l);
//		} else {
//			double d=GetDouble(NULL);
//			d-=pow10(place);
//			SetText(d);
//		}
//		//curpos=newpos;
//		findcaret();
//		return 0;
	}

	return 1;
}

int LineEdit::RBDown(int x,int y, unsigned int ,int count,const LaxMouse *d)
{
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	return 0;
}

int LineEdit::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, RIGHTBUTTON);
	return 0;
}

//! For autoscroll: TODO!!
int LineEdit::Idle(int tid, double delta) 
{ //***autoscroll on right button or left button
	return 0;
//	if (!(buttondown&RIGHTBUTTON)) return 0;
//	int c;
//	if (sellen==0) selstart=curpos;
//	curpos=findpos(x+curlineoffset);
//	sellen=curpos-selstart;
//	findcaret();
//	needtodraw|=(makeinwindow()?1:4);
//	return 0;
}

/*! \todo implement right button shift view
 */
int LineEdit::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	//DBG long newpos=findpos(x+curlineoffset);
	//DBG cerr <<"x:"<<x<<"  clo:"<<curlineoffset<<"  newpos:"<<newpos<<endl;

	if (!buttondown.any()) {
		int oldhover=lasthover;
		if (x>win_w-textheight-padx && (win_style&LINEEDIT_CLEAR_X)) {
			lasthover=1;
		} else lasthover=0;
		if (lasthover!=oldhover) needtodraw=1;
		return 0;
	}

	if (dp == nullptr) {
		dp = MakeCurrent();
		dp->font(thefont, thefont->textheight());
	}

	if (buttondown.isdown(d->id,LEFTBUTTON)) {
		long newpos=findpos(x+curlineoffset);
		curpos=newpos;
		if (curpos>textlen) curpos=textlen;
//		if (state&ShiftMask) {
			if (!sellen) {
				if (selstart>textlen) selstart=textlen;
			}
			sellen=curpos-selstart;
//		} else {
//			sellen=0;
//			selstart=curpos;
//		}
		findcaret();
		needtodraw|=(makeinwindow()?1:4);
		return 0;

	} else if (buttondown.isdown(d->id,RIGHTBUTTON)) {
		//*** shift viewable
		return 0;
	}
	return 1;
					
}

int LineEdit::UseThisFont(LaxFont *newfont)
{
	TextXEditBaseUtf8::UseThisFont(newfont);
	//findcaret();
	//makeinwindow();
	return 1;
}

int LineEdit::SetupMetrics()
{ 
	TextXEditBaseUtf8::SetupMetrics();
	firsttime=0;

	Setpixwide();
	findcaret();
	makeinwindow();
	return 0;
}

void LineEdit::DrawText(int)
{
	char check=1;
	if (textstyle&TEXT_LEFT || mostpixwide>textrect.width-2*padx) {
		 //if left justified, or line fits off screen
		DrawLineOfText(-curlineoffset,textrect.height/2-textheight/2+textascent,0,textlen,check);

	} else {
		//int ext=GetExtent(0,textlen,0);
		if (textstyle&TEXT_CENTER) 
			DrawLineOfText(textrect.width/2-mostpixwide/2,textrect.height/2-textheight/2+textascent,0,textlen,check);
		else DrawLineOfText(textrect.width-padx-mostpixwide,textrect.height/2-textheight/2+textascent,0,textlen,check);
	}

	if (curlineoffset<0) {
		 //black out blank area before text (on left)
		Colors(0);
		Black(textrect.x,textrect.y+cy-textascent,-curlineoffset,textheight); 
	}
	
}

//! Set textrect.
/*! This is called after a resize event to section off textrect from window.
 * The default is to use the whole window.
 */
void LineEdit::settextrect()
{
	textrect.x=textrect.y=0;
	textrect.width= win_w;
	textrect.height=win_h;
}

int LineEdit::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	settextrect();
	Setpixwide();
	findcaret();
	needtodraw=1;
	return 0;
}


} // namespace Laxkit
