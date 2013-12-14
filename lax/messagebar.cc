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
//    Copyright (C) 2004-2012 by Tom Lechner
//


#include <cstring>
#include <lax/messagebar.h>
#include <lax/laxutils.h>

#include <iostream>
using namespace std;
#define DBG 


using namespace LaxFiles;


namespace Laxkit {

/*! \class MessageBar
 * \brief A very basic text displayer window.
 * Pass in a chunk of text and this window will display it.
 * You can have multiple lines by putting '\\n' characters in 
 * the text.
 *
 * If there is too much text to fit in the window, you can optionally
 * allow the user to drag the text around. Simply define MB_MOVE, and
 * right clicking will shift around the text. Shift and/or Control 
 * right drag speeds up the shifting.
 *
 * Left click to copy to clipboard if MB_COPY is set.
 *
 * \todo *** need to implement wrapping style
 * \todo *** SetText(char *s1,char *s2, ...)
 * 
 * \code
 *   // Note to developer: these must not clobber the xymesbar defines:
 *   enum MessageBarTypes;
 * \endcode
 */
/*! \var int MessageBar::ex
 * \brief The x extent of the text.
 */
/*! \var int MessageBar::ey
 * \brief The y extent of the text.
 */
/*! \var int MessageBar::ox
 * \brief The x coordinate of the left edge of the text extent.
 */
/*! \var int MessageBar::oy
 * \brief The y coordinate of the baseline of the top line of text.
 */


//! Pass in the new text after the regular window parameters.
/*! Multi-line text is simply a char[] with '\\n' characters in it.
 *  The text is then copied and stored by line in thetext.
 *
 *  If win_w or win_h are 0, and newtext is supplied here, then win_w and
 *  win_h are set to the extent of the message plus pads.
 */
MessageBar::MessageBar(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder,const char *newtext) 
				: anXWindow(pwindow,nname,ntitle,nstyle, nx,ny, nw,nh,brder, NULL,0,NULL) 
{
	needtodraw=1;
	firsttime=1;
	thetext=NULL;
	nlines=0;
	indents=NULL;
	DBG if (!newtext) cerr <<WindowTitle()<<": no new text"<<endl;
	DBG else cerr <<"New Mesbartext:"<<newtext<<endl;

	SetText(newtext);
	msx=msy=ox=oy=ex=ey=padx=pady=0;

	if ((win_style&(MB_CENTER|MB_LEFT|MB_RIGHT|MB_TOP|MB_BOTTOM))==0) { // default center
		win_style=(win_style&(~(MB_CENTER|MB_LEFT|MB_RIGHT|MB_TOP|MB_BOTTOM)))|MB_CENTER;
	}
	SetupMetrics(); //clears up if h==0*** what the hell

	win_colors=app->color_panel;
	win_colors->inc_count();
}

MessageBar::~MessageBar()
{
	for (int c=0; c<nlines; c++) delete[] thetext[c];
	if (nlines) {
		delete[] indents;
		delete[] thetext;
	}
}

int MessageBar::Event(const EventData *e,const char *mes)
{
	if (e->type==LAX_onMouseOut && (win_style&MB_LEAVE_DESTROYS)) {
		app->destroywindow(this);
	}
	return anXWindow::Event(e,mes);
}

int MessageBar::init()
{
	return 0;
}

//! Redefine the message text.
/*! \todo this can use split()
 * \todo *** implement MB_WRAP
 */
int MessageBar::SetText(const char *newtext)
{
	if (!newtext) return 1;
	//DBG cerr <<WindowTitle()<<" SetText: "<<newtext<<endl;
	for (int c=0; c<nlines; c++) delete[] thetext[c];
	if (nlines) {
		delete[] indents;
		delete[] thetext;
	} 

//	*** lines ending with \n are actually newlines. otherwise, they are
//		inserted breaks, due to wrapping

	if (!newtext) {
		thetext=new char*[1];
		thetext[0]=new char[2];
		thetext[0][0]=' '; thetext[0][1]='\0';
		indents=new double[1];
		nlines=1;
		firsttime=1;
		needtodraw=1;
		return 0;
	}
	nlines=1;
	int nl=0;
	int c,p;
	int tl=strlen(newtext);
	for (c=0; c<tl; c++) if (newtext[c]=='\n') nlines++;
	indents=new double[nlines];
	thetext=new char*[nlines];
	
	 // split text by \n
	c=0;
	for (nl=0; nl<nlines; nl++) {
		p=c;
		while (p<tl && newtext[p]!='\n') p++;
		if (p==c) { // blank line
			thetext[nl]=new char[2];
			thetext[nl][0]=' ';
			thetext[nl][1]='\0';
			c++;
			continue;
		}
		thetext[nl]=new char[p-c+1];
		strncpy(thetext[nl],newtext+c,p-c);
		thetext[nl][p-c]='\0';
		//DBG cerr <<"line "<<nl<<"("<<strlen(thetext[nl])<<"): "<<thetext[nl]<<endl;
		c=p+1;
	}
		
	ox=oy=0;
	firsttime=1;
	needtodraw=1;
	return 0;
}

 //!  If window dimensions are initially == 0 this sets them to extent of the message plus pads
int MessageBar::SetupMetrics()
{
	if (!thetext || !app) return 1;
	int n;
	ex=0;
	for (n=0; n<nlines; n++) {
		getextent(thetext[n],strlen(thetext[n]), &indents[n], NULL, &fasc, &fdes, 0);
		if (indents[n]>ex) ex=indents[n];
	}
	ey=nlines*(fasc+fdes);
	
	if (win_w==0) win_w=ex+2*padx;
	if (win_h==0) win_h=ey+2*pady;
	
	if (win_style&MB_RIGHT) ox=win_w-ex-2*padx;
	else if (win_style&MB_LEFT) ox=padx; 
	else ox=(win_w-ex)/2; //center

	for (n=0; n<nlines; n++) {
		if (win_style&MB_RIGHT) indents[n]=win_w-indents[n]-2*padx-ox;
		else if (win_style&MB_LEFT) indents[n]=padx-ox;
		else indents[n]=(win_w-indents[n])/2-ox; //center
	}
	
	if (win_style&MB_TOP) oy=fasc+pady; 
	else if (win_style&MB_BOTTOM) oy=win_h-ey+fasc-2*pady;
	else oy=(win_h-ey)/2+fasc; //center
	
	firsttime=0;
	return 0;
}
	
void MessageBar::Refresh()
{
	if (!win_on || !needtodraw) return;
	if (firsttime) {
		if (SetupMetrics()) {
			if (!thetext) needtodraw=0;
			return;
		}
	}
	//DBG cerr <<"mesbar("<<WindowTitle()<<")drawing..";
	
	background_color(win_colors->bg);
	foreground_color(win_colors->fg);
	clear_window(this);
	int l=0;
	for (int c=0; c<nlines; c++) {
		l=strlen(thetext[c]);
		if (thetext[c][l-1]=='\n') l--;
		textout(this,thetext[c],l, ox+indents[c],oy+c*(fasc+fdes), LAX_LEFT|LAX_BASELINE);
	}

	needtodraw=0;
	//DBG cerr <<"done  ";
	return;
}

int MessageBar::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, LEFTBUTTON, x,y);
	msx=x;
	msy=y;
	//DBG cerr << "thetext:"<<thetext;
	return 1;
}

int MessageBar::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, MIDDLEBUTTON, x,y);
	msx=x;
	msy=y;
	return 1;
}

//! Get a char[] copy of the message text, with '\\n' for the linebreaks.
char *MessageBar::GetText()
{
	if (!thetext) return NULL;
	int nl=0;
	for (int c=0; c<nlines; c++) nl+=strlen(thetext[c]);
	char *str=new char[nl+nlines+1];
	str[0]='\0';
	for (int c=0; c<nlines; c++) {
		strcat(str,thetext[c]); //actual newlines are embedded in the lines
	}
	DBG cerr <<"len of GetText:"<<strlen(str)<<endl;
	return str;
}

int MessageBar::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, LEFTBUTTON);

	if (!(win_style&MB_COPY))return 1;
	 //	copy to clipboard
	char *str=GetText();
	if (str){
		app->CopytoBuffer(str,strlen(str)); 
		DBG cerr <<"(mb copy: "<<str<<")\n";
		delete[] str;
	}
	return 0;
}

int MessageBar::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	msx=x;
	msy=y;
	return 0;
}

int MessageBar::MBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, MIDDLEBUTTON);
	return 0;
}

int MessageBar::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, RIGHTBUTTON);
	return 0;
}

/*! If allowed with MB_MOVE, this shifts around text that is smaller
 * than the window only within the window boundaries, and shifts 
 * around larger text only just enough so that you can see it.
 */
int MessageBar::MouseMove(int x,int y,unsigned int state,const LaxMouse *d)
{
	//DBG cerr<<"\nMB:";
	if (!buttondown.any(d->id) || !(win_style&MB_MOVE)) return 1;
	buttondown.move(d->id, x,y);
	buttondown.average(-1, &x,&y);

	int mult=1;
	if ((state&LAX_STATE_MASK)==ShiftMask || (state&LAX_STATE_MASK)==ControlMask) mult=4;
	else if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) mult=8;

	if (win_style&MB_MOVE) { ox+=mult*(x-msx); oy+=mult*(y-msy); }
	//DBG cerr <<ox<<','<<oy;

	if (x-msx>0) { // moving right
		if (ox-padx>0 && ox+padx+ex>win_w) { if (ex+2*padx<win_w) ox=win_w-ex-padx; else ox=padx; }
	} else if (x-msx<0) { // moving left
		if (ox-padx<0 && ox+padx+ex<win_w) { if (ex+2*padx<win_w) ox=padx; else ox=win_w-ex-padx; }
	}

	if (y-msy>0) { // move down
	//DBG cerr <<" ============ ey="<<ey<<" oy-fasc="<<oy-fasc<<" pady="<<pady<<" win_h="<<win_h<<endl;
		if (oy-fasc-pady>0 && pady+oy-fasc+ey+pady>win_h) { // box is below upper pad, lower edge is below lower pad
			if (ey+2*pady<win_h) oy=win_h-ey-pady+fasc; // box fits in window
			else oy=fasc+pady; // box does not fit in window
		}
	} else if (y-msy<0) { // move up
		if (oy-fasc-pady<=0 && oy-fasc+pady+ey<win_h) { if (ey+2*pady<win_h) oy=fasc+pady; else oy=win_h-ey-pady+fasc; }
	}

	msx=x; msy=y;
	needtodraw=1;
	return 0;
}

int MessageBar::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&(ShiftMask|ControlMask)) oy+=win_h*3/4; else oy+=20;
	if (oy-fasc-pady>0 && pady+oy-fasc+ey+pady>win_h) { // box is below upper pad, lower edge is below lower pad
		if (ey+2*pady<win_h) oy=win_h-ey-pady+fasc; // box fits in window
		else oy=fasc+pady; // box does not fit in window
	}
	needtodraw=1;
	return 0;
}

int MessageBar::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (state&(ShiftMask|ControlMask)) oy-=win_h*3/4; else oy-=20;
	if (oy-fasc-pady<=0 && oy-fasc+pady+ey<win_h) { if (ey+2*pady<win_h) oy=fasc+pady; else oy=win_h-ey-pady+fasc; }
	needtodraw=1;
	return 0;
}

int MessageBar::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	SetupMetrics();
	needtodraw=1;
	return 0;
}

int MessageBar::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	SetupMetrics();
	needtodraw=1;
	return 0;
}


/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *MessageBar::dump_out_atts(Attribute *att,int what,anObject *context)
{
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	if (what==-1) {
		att->push("text","string");
		att->push("halign","one of: left center right");
		att->push("valign","one of: left center right");
		return att;
	}

	char *txt=GetText();
	att->push("text",txt);
	if (txt) delete[] txt;

	char align[20];
	if (win_style&MB_LEFT) strcpy(align,"left");
	else if (win_style&MB_CENTERX) strcpy(align,"center");
	else if (win_style&MB_RIGHT) strcpy(align,"right");
	att->push("halign",align);

	if (win_style&MB_TOP) strcpy(align,"top");
	else if (win_style&MB_CENTERY) strcpy(align,"center");
	else if (win_style&MB_BOTTOM) strcpy(align,"bottom");
	att->push("valign",align);

	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void MessageBar::dump_in_atts(Attribute *att,int flag,anObject *context)
{
	anXWindow::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"text")) {
			SetText(value);

		} if (!strcmp(name,"halign")) {
			win_style&=~(MB_LEFT|MB_CENTERX|MB_RIGHT);
			if (!strcmp(value,"left")) win_style|=MB_LEFT;
			else if (!strcmp(value,"center")) win_style|=MB_CENTERX;
			else if (!strcmp(value,"right")) win_style|=MB_RIGHT;
			if ((win_style&(MB_LEFT|MB_CENTERX|MB_RIGHT))==0) win_style|=MB_CENTERX;

		} if (!strcmp(name,"valign")) {
			win_style&=~(MB_TOP|MB_CENTERY|MB_BOTTOM);
			if (!strcmp(value,"left")) win_style|=MB_TOP;
			else if (!strcmp(value,"center")) win_style|=MB_CENTERY;
			else if (!strcmp(value,"right")) win_style|=MB_BOTTOM;
			if ((win_style&(MB_TOP|MB_CENTERY|MB_BOTTOM))==0) win_style|=MB_CENTERY;
		}
	}
}


} // namespace Laxkit

