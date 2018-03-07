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


#include <lax/funcframe.h>
#include <lax/strmanip.h>
#include <iostream>
#include <cctype>


#define LAX_LISTS_SOURCE_TOO
#include <lax/lists.cc>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

/*! \class FuncFrame
 * \brief A frame window that uses rpn (reverse polish notation) expressions to define the subwindow positions and dimensions.
 *
 * You can define extra (integer) variables to aid in expressions. "w" and "h" are always
 * defined to be the frame's current win_w and win_h respectively. Operators you can use
 * are '+', '-', '*', '/', and '%' (for modulus).
 *
 * <pre>
 *  NOTES
 *  -----
 *  var names must be less than 20 chars, and can be letters or numbers, but start with a letter
 *  w= window width, always varval[0]
 *  h= window height, always varval[1]
 *  does hardly any error checking, does not check for name already in use, just returns first var of given name
 * 
 *  AddWin(--) does not assume that win->xlib_window exists, 
 *  SyncWin() should be called in this->init(), app->addwindow(all sub windows) does not have to have happened
 *    actual x,y,w,h are applied in SyncWin, calls MoveResize only if win->xlib_window exists
 * </pre>
 *
 * \todo needs a flag to say need to update sub boxes. else with each resize, all gets recomputed,
 *   and for mouse drag resizes, this causes way too much unnecessary computing
 */ 
/*! \struct FuncFrame::WinDef
 * This is a FuncFrame internal class to hold the child anXWindows and the associated x,y,w,h definitions.
 */


FuncFrame::FuncFrame(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
								int xx,int yy,int ww,int hh,int brder,
								anXWindow *prev,unsigned long owner,const char *mes)
						: anXWindow(parnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,owner,mes)
{
	vars=NULL;
	varval=NULL;
	nvars=0;
	AddVar("w",win_w);
	AddVar("h",win_h);
	arrangedstate=0;
}

FuncFrame::~FuncFrame()
{
	if (!nvars) return;
	for (int c=0; c<nvars; c++) delete[] vars[c];
	delete[] vars;
	delete[] varval;
}

FuncFrame::WinDef::WinDef(anXWindow *nwin,const char *xx,const char *yy,const char *ww,const char *hh)
{
	x=y=w=h=NULL;
	makestr(x,xx);
	makestr(y,yy);
	makestr(w,ww);
	makestr(h,hh);
	win=nwin;
}

//! Add a variable to the frame.
/*! Variables must be less than 20 characters long (because of a char[20] in getval()).
 *
 * Sets the variable var to have the value val.
 *
 * \todo no previous existence check is done! must implement that..
 */
void FuncFrame::AddVar(const char *var,int val)
{
	DBG cerr <<"Add var:"<<var<<"="<<val<<endl;
	if (nvars==0) {
		vars=new char*[1];
		varval=new int[1];
	} else {
		char **tvars=new char*[nvars+1];
		int *tvarval=new int[nvars+1];
		for (int c=0; c<nvars; c++) {
			tvars[c]=vars[c];
			tvarval[c]=varval[c];
		}
		delete[] vars;
		delete[] varval;
		vars=tvars;
		varval=tvarval;
	}
	vars[nvars]=NULL;
	makestr(vars[nvars],var);
	varval[nvars]=val;
	nvars++;
}

 //! Adds a child window with the expressions here.
 /*! If any of the x,y,w,h expressions are NULL, then no window is added.
  */
void FuncFrame::AddWin(anXWindow *win,const char *x,const char *y,const char *w,const char *h)
{
	if (win==NULL || x==NULL || y==NULL || w==NULL || h==NULL) return;
	windefs.push(new WinDef(win,x,y,w,h));
}

 //! Basic RPN calculator
 /*! This evaluates the expression def, substituting the variables as it goes.
  *  The only operators allowed are '+', '-', '*', '/', and '\%' (for modulo).
  *  Returns the result of the expression or 0 if there is an error.
  */
int FuncFrame::getval(const char *def) // basic RPN calculator
{
	if (!def) { 
		DBG cerr <<"null def"<<endl;
		return 0;
	}
	DBG cerr <<"def:"<<def;

	NumStack<int> vals;
	int p=0,tl=strlen(def);
	int c,c2,c3;
	char varname[20];
	while (p<tl) {
		c3=p;
		while (isspace(def[p])) p++;
		if (isdigit(def[p])) { // read in and push int
			c=0;
			for (c=0; isdigit(def[p]); p++) c=c*10+(int)(def[p]-'0');
			vals.push(c);
		} else if (isalpha(def[p])) { // read in var and push value
			c2=0;
			while (isalnum(def[p+c2])) { varname[c2]=def[p+c2]; c2++; }
			varname[c2]='\0';
			p+=c2;
			for (c=0; c<nvars; c++) {
				if (!strcmp(varname,vars[c])) { vals.push(varval[c]); break; }
			}
		} else {
			switch (def[p]) {
				case '+': vals.push(vals.pop()+vals.pop()); p++; break;
				case '-': c2=vals.pop(); vals.push(vals.pop()-c2); p++; break;
				case '*': vals.push(vals.pop()*vals.pop()); p++; break;
				case '/': c2=vals.pop(); vals.push(vals.pop()/c2); p++; break;
				case '%': c2=vals.pop(); vals.push(vals.pop()%c2); p++; break;
			}
		}
		if (p==c3) break;
	}
	DBG cerr <<" = "<<vals.e[vals.n-1]<<endl;
	return vals.pop();
}

 // this is called on any resizing, and should be called after all windows have been AddWin'd
 // typically, call SyncWin(1) from aframeinstance->init() after all AddWin'd
 // SyncWin(1) only app-addwindow's if win->window is 0
void FuncFrame::SyncWin(int addwindow) //addwindow=0
{
	for (int c=0; c<windefs.n; c++) {
		DBG cerr <<"Sync:"<<windefs.e[c]->win->WindowTitle()<<endl;
		DBG cerr <<"  x:"<<windefs.e[c]->x<<endl;
		DBG cerr <<"  y:"<<windefs.e[c]->y<<endl;
		DBG cerr <<"  w:"<<windefs.e[c]->w<<endl;
		DBG cerr <<"  h:"<<windefs.e[c]->h<<endl;

		if (addwindow) app->addwindow(windefs.e[c]->win);

		if (windefs.e[c]->win->xlib_window) {
			windefs.e[c]->win->MoveResize(getval(windefs.e[c]->x),
									getval(windefs.e[c]->y),
									getval(windefs.e[c]->w),
									getval(windefs.e[c]->h));
		} 
	}
	arrangedstate=1;
}

//! Calls SyncWin(1) if arrangedstate!=1.
void FuncFrame::Refresh()
{
	DBG cerr <<"****************************"<<endl;
	if (!win_on || !needtodraw || !windefs.n) return;
	if (arrangedstate!=1) SyncWin(1);
	needtodraw=0;
}

//! Set varval[0] and [1] to the new win_w and win_h. Also set arrangedstate=0.
int FuncFrame::Resize(int nw,int nh)
{
	anXWindow::Resize(nw,nh);
	varval[0]=win_w;
	varval[1]=win_h;
	arrangedstate=0;
	needtodraw=1;
	return 0;
}

//! Calls anXWindow::MoveResize, then Sync(0).
int FuncFrame::MoveResize(int nx,int ny,int nw,int nh)
{
	anXWindow::MoveResize(nx,ny,nw,nh);
	varval[0]=win_w;
	varval[1]=win_h;
	arrangedstate=0;
	needtodraw=1;
	return 0;
}

} // namespace Laxkit

