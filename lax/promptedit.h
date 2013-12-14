//
//	
//    The Laxkit, a windowing toolkit
//    Copyright (C) 2004-2006 by Tom Lechner
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
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
#ifndef _LAX_PROMPTEDIT_H
#define _LAX_PROMPTEDIT_H

#include <lax/multilineedit.h>



namespace Laxkit {
	
class HistoryNode
{
 public:
	long inputstart,outputstart;
	int inputlen,outputlen;
	HistoryNode *next,*prev;
	HistoryNode() { next=prev=NULL; inputstart=outputstart=0; inputlen=outputlen=0; }
	~HistoryNode();
};

class PromptEdit : public MultiLineEdit
{
 protected:
	char *promptstring;
	long start;
	HistoryNode *history;
	int numhistory,maxhistory;
	virtual char *process(const char *in);
 public:
	PromptEdit(anXWindow *prnt,const char *nname,const char *ntitle,unsigned long nstyle,
						int xx,int yy,int ww,int hh,int brder,
						anXWindow *prev,unsigned long nowner=None,const char *nsend=NULL,
						unsigned int ntstyle=0,const char *newtext=NULL);
	virtual ~PromptEdit();
	virtual int readonly(long pos);
	virtual int ProcessInput(const char *thisexpression=NULL);
	virtual void SetPromptString(const char *nstr);
	virtual const char *GetPromptString();
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);
};

} // namespace Laxkit;

#endif

