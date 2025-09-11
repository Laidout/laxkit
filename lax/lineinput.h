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
#ifndef _LAX_LINEINPUT_H
#define _LAX_LINEINPUT_H


#include <lax/labeluser.h>
#include <lax/lineedit.h>
#include <lax/strmanip.h>

#define LINP_ONLEFT    (1<<16)
#define LINP_ONRIGHT   (1<<17)
#define LINP_ONTOP     (1<<18)
#define LINP_ONBOTTOM  (1<<19)
#define LINP_LEFT      (1<<20)
#define LINP_RIGHT     (1<<21)
#define LINP_CENTER    (1<<22)
#define LINP_POPUP     (1<<23)
#define LINP_INT       (1<<24)
#define LINP_FLOAT     (1<<25)
#define LINP_FILE      (1<<26)
#define LINP_FILESAVE  (1<<27)
#define LINP_DIRECTORY (1<<28)
#define LINP_SEND_ANY  (1<<29)
#define LINP_STYLEMASK ((1<<16)|(1<<17)|(1<<18)|(1<<19)|(1<<20)|(1<<21)|(1<<22)|(1<<23)|(1<<24)|(1<<25)|(1<<26)|(1<<27)|(1<<28)|(1<<29))



namespace Laxkit {

class LineInput : public anXWindow, public LabelUser
{
  protected:
	anXWindow *helper;
	LineEdit *le;
	char *label;
	int labelw; //preferred width of the label
	bool auto_labelw;
	int lx,ly,lew,leh;
	double padx,pady,padlx,padly; //fraction of textheight

  public:
	LineInput(anXWindow *parnt,const char *nname,const char *ntitle,unsigned int nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner=0,const char *nsend=NULL,
			const char *newlabel=NULL,const char *newtext=NULL,unsigned int ntstyle=0,
			int nlew=0,int nleh=0, double npadx=-1,double npady=-1,double npadlx=-1,double npadly=-1);
	virtual ~LineInput();
	virtual const char *whattype() { return "LineInput"; }
	virtual const char *tooltip(const char *ntip);
	virtual anXWindow *GetController() { return le; }
	virtual int init();
	virtual int Event(const EventData *data, const char *mes);
	virtual int FocusOn(const FocusChangeData *e);
	virtual int FocusOff(const FocusChangeData *e);
	virtual void Refresh();
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);
	virtual void UIScaleChanged();

	//from LabelUser
	virtual double LabelWidth();
	virtual double LabelWidth(double newwidth);
	virtual void Label(const char *newLabel);
	virtual const char *Label();
	virtual bool LabelOnLeft(); //true: (label) (stuff),  false: (stuff) (label)
	virtual bool LabelOnLeft(bool onLeft);
	virtual int LabelAlignment(); //how to align label with the label width. -1 == left, 0 == center, 1 == right
	virtual int LabelAlignment(int newAlignment);

	virtual char *GetText() { if (le) return le->GetText(); else return NULL; }
	virtual const char *GetCText() { if (le) return le->GetCText(); else return NULL; }
	virtual long   GetLong(int *error_ret=NULL);
	virtual double GetDouble(int *error_ret=NULL);
	virtual void SetText(const char *newtext);
	virtual void SetText(int newtext);
	virtual void SetText(double newtext);
	virtual void SetOwner(anXWindow *nowner,const char *mes=NULL) { if (le) le->SetOwner(nowner,mes); }
	virtual void SetPlacement(); // this must follow any change to label
	virtual void WrapToExtent();
	virtual int send() { return 0; }
	virtual int CloseControlLoop() { if (le) return le->CloseControlLoop(); return 0; }
	virtual LineEdit *GetLineEdit();
	virtual void Qualifier(const char *nqualifier);
    virtual void SetType(unsigned long what);

    virtual Attribute *dump_out_atts(Attribute *att,int what,DumpContext *context);
    virtual void dump_in_atts(Attribute *att,int flag,DumpContext *context);
};



} // namespace Laxkit

#endif

