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
//    Copyright (C) 2004-2007,2010,2014 by Tom Lechner
//
#ifndef _LAX_FILEDIALOG2_H
#define _LAX_FILEDIALOG2_H

#include <lax/rowframe.h>
#include <lax/menuinfo.h>
#include <lax/lineinput.h>
#include <lax/treeselector.h>
#include <lax/filepreviewer.h>
#include <lax/button.h>


#define FILES_GLOBAL_BOOKMARK  (1<<15)

#define FILES_MINIMAL          (1<<16)
#define FILES_NO_BOOKMARKS     (1<<17)
#define FILES_DETAILS          (1<<18)
#define FILES_NO_FOLLOW_LINKS  (1<<19)

#define FILES_FILES_ONLY       (1<<20)

 // NEW is really same as OPEN_ONE, except file must not already exist?
 // SELECT_DIR is same as OPEN_ONE, but the selected thing must be a directory, 
 // 	or if none selected, then it is the current directory
 // OPEN allows multiple files (not directories) to be selected and returned,
 // 	and they don't have to all be in the same directory (be careful when selecting!!)
#define FILES_FROM_ONE_DIR     (1<<21)
#define FILES_NEW              (1<<22)
#define FILES_SELECT_DIR       (1<<23)
#define FILES_OPEN_ONE         (1<<24)
#define FILES_OPEN_MANY        (1<<25)
#define FILES_OPENING          ((1<<21)|(1<<22)|(1<<23)|(1<<24)|(1<<25))

 // SAVE and SAVE_AS allow the same things, but "Save" or "Save As" is a label somewhere
#define FILES_SAVE             (1<<26)
#define FILES_SAVE_AS          (1<<27)
#define FILES_ASK_TO_OVERWRITE (1<<28)
#define FILES_SAVING           ((1<<26)|(1<<27)|(1<<28))

 // Have the preview alongside the list of files.
#define FILES_PREVIEW          (1<<29)

#define FILES_NO_CANCEL        (1<<31)




namespace Laxkit {


//-------------------------------- FileDialog2 ------------------------------
class FileDialog2 : public RowFrame
{
  protected:
	PtrStack<char> history;
	int curhistory;

	MenuInfo files;

	LineInput *path,*mask,*file;
	TreeSelector *filelist;
	FilePreviewer *previewer;
	unsigned long dialog_style;
	Button *ok;
	int finalbuttons;

	char *recentgroup;
	MenuInfo recentmenu;
	bool showing_recent;
	bool showing_icons;

	int getDirectory(const char *npath);
	virtual int newBookmark(const char *pth);
	virtual MenuInfo *BuildBookmarks();
	virtual int closeWindow();
	virtual int send(int id);
	virtual void UpdateGray();
	virtual int ShowRecent();

  public:
 	FileDialog2(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder, 
			unsigned long nowner,const char *nsend,
			unsigned long ndstyle,
			const char *nfile=NULL,const char *npath=NULL,const char *nmask=NULL,
			const char *nrecentgroup=NULL);
	virtual ~FileDialog2();
	virtual const char *whattype() { return "FileDialog2"; }
	virtual int preinit();
	virtual int init();
	virtual int Event(const EventData *e,const char *mes);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const LaxKeyboard *d);

	virtual void OkButton(const char *textforok, const char *ttip);
	virtual void AddFinalButton(const char *text, const char *ttip, int id, int position);
	virtual int ClearFinalButton(int position);
	virtual void RecentGroup(const char *group);
	virtual void GoUp();
	virtual void GoBack();
	virtual void GoForward();
	virtual void Cd(const char *to);
	virtual void RefreshDir();
	virtual void SetFile(const char *f);
	virtual char *fullFilePath(const char *f);
	virtual int NewHistory(const char *npath);
};

} //namespace Laxkit

#endif

