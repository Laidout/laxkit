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
//    Copyright (C) 2005-2007 by Tom Lechner
//




#include <lax/promptedit.h>
#include <lax/strmanip.h>


#include <iostream>
using namespace std;
#define DBG 


namespace Laxkit {


//---------------------------------- HistoryNode -------------------------------------
/*! \class HistoryNode
 * \brief Node class for a stack of history for PromptEdit.
 *
 * \todo *** should make this internal to prompt?
 */
/*! \var long HistoryNode::inputstart
 * \brief Start of the input.
 */
/*! \var long HistoryNode::outputstart
 * \brief Start of the output.
 */
/*! \var int HistoryNode::inputlen
 * \brief Length of the input.
 */
/*! \var int HistoryNode::outputlen
 * \brief Length of the output.
 */

HistoryNode::~HistoryNode()
{
	if (prev) prev->next=NULL;
	if (next && next!=this) delete next;
}

//---------------------------------- PromptEdit -------------------------------------
/*! \class PromptEdit
 * \brief An editor for a command prompt.
 *
 * This is everything the standard MultiLineEdit is, but it keeps track
 * of inputs and outputs. A prompt string is displayed, and input
 * can be processed, and the output is appended to the edit's text.
 *
 * \todo tab completion
 */
/*! \var int PromptEdit::maxhistory
 * \brief This number of inputs and outputs are stored.
 */
/*! \var char *PromptEdit::start
 * \brief Points to the start of the command line.
 */


PromptEdit::PromptEdit(anXWindow *prnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner,const char *nsend,
						unsigned int ntstyle,const char *newtext)
		: MultiLineEdit(prnt,nname,ntitle,nstyle,xx,yy,ww,hh,brder,prev,nowner,nsend,ntstyle,newtext)
{
	promptstring=newstr("> ");

	history=NULL;
	char *t=newstr(newtext);
	appendstr(t,promptstring);
	SetText(t);
	start=textlen;

	LaxFont *font=app->fontmanager->MakeFontFromStr("Courier",getUniqueNumber());
	//LaxFont *font=app->fontmanager->MakeFontFromStr(":spacing=100",getUniqueNumber());
	UseThisFont(font);
	font->dec_count();
}

//! Delete history list.
PromptEdit::~PromptEdit()
{
	if (history) delete history; 
}

//! Takes an in, and returns a new char[] out.
/*! This out should not have pre and post newlines. Those would be inserted in ProcessInput().
 */
char *PromptEdit::process(const char *in)
{
	DBG cerr <<"Process: "<<(in?in:"(null in)")<<endl;
	return newstr("Processed.");
}

//! Process input. Usually called on an enter from CharInput, but can be called anytime.
/*! This takes the expression, finds an output expression, then inserts the output, and 
 * a new prompt into the edit.
 *
 * If thisexpression==NULL then the text between the prompt and the end
 * is processed. Otherwise, thisexpression is processed, and whatever
 * is on the current command line is ignored.
 */
int PromptEdit::ProcessInput(const char *thisexpression)
{
	DBG cerr <<"--------- start ProcessInput"<<endl;
	HistoryNode *newentry=new HistoryNode();
	newentry->inputstart=start;
	newentry->inputlen=textlen-start;

	if (thisexpression==NULL) thisexpression=thetext+start;
	//int local;
	char *outexpres=process(thisexpression);

	SetCurpos(-1); // jump to eof
	sellen=0; // unselect any selection

	newentry->outputstart=nextpos(curpos);
	inschar('\n');
	if (outexpres) {
		insstring(outexpres,1);
		delete[] outexpres;
	}
	newentry->outputlen=curpos-newentry->outputstart;
	inschar('\n');
	
	if (!history) {
		history=newentry;
		history->next=history->prev=history;
		numhistory=1;
	} else {
		if (maxhistory>1 && numhistory+1>maxhistory) {
			 // del oldest history
			HistoryNode *temp=history->next;
			temp->prev=history->prev;
			history->prev->next=temp;
			history->next=history->prev=NULL;
			delete history;
			history=temp;
			numhistory--;
		}
		newentry->prev=history->prev;
		newentry->next=history;
		history->prev->next=newentry;
		history->prev=newentry;
		numhistory++;
	}
	
	insstring(GetPromptString(),1);
	start=curpos;
	
	
	//**** this is cheap and dirty refreshing:
	needtodraw=1;
	DBG cerr <<"--------- end ProcessInput"<<endl;
	return 0;
}

//! Anything before the start of the command line is readonly.
int PromptEdit::readonly(long pos)
{
	if (pos<0) pos=curpos;
	if (curpos<start) return 1;
	if (sellen && selstart<start) return 1;
	return 0;
}

//! Set the prompt string.
void PromptEdit::SetPromptString(const char *nstr)
{
	if (nstr==NULL) makestr(promptstring,"");
	else makestr(promptstring,nstr);
}

//! Return a const pointer to the current prompstring.
const char *PromptEdit::GetPromptString()
{
	return promptstring;
}

//! Intercept for a reverse search, and to traverse the in/out queues.
/*! 
 * \todo *** clear to start of command line
 * \todo *** clear to end of command line
 * \todo *** clear whole command line
 * \todo *** other readline-y things like searching? tab completion?
 */
int PromptEdit::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d)
{
//	if (ch=='r' && state&ControlMask) {
//		*** reverse search
//	} else {
//		move up and down through outputs/inputs
//	} else return MultiLineEdit::CharInput(ch,buffer,len,state);
//------------------------
	if (ch==LAX_Enter) {
		ProcessInput();
		return 0;
	}
	return MultiLineEdit::CharInput(ch,buffer,len,state,d);
}

} // namespace Laxkit

