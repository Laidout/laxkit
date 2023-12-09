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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//
#ifndef _LAX_PALETTEWINDOW_H
#define _LAX_PALETTEWINDOW_H

#include <lax/dump.h>
#include <lax/anxapp.h>
#include <lax/anobject.h>
#include <lax/rectangles.h>
#include <lax/lists.h>
#include <lax/buttondowninfo.h>
#include <lax/gradientstrip.h>

namespace Laxkit {


//-------------------------------- PaletteWindow -----------------------------


class PaletteWindow : public anXWindow
{
 protected:
	int xn,yn;
	double dx,dy;
	ButtonDownInfo buttondown;
	bool show_locked_hint = false;
	Laxkit::ShortcutHandler *sc = nullptr;

	//bool waiting_for_new = false; //flag to transition from new to edit
	//ColorSliders *color_popup;

 public:
 	enum Actions {
 		ACTION_Export = 1,
 		ACTION_Import,
 		ACTION_RenamePalette,
 		ACTION_DuplicatePalette,
 		ACTION_DeletePalette,
 		ACTION_SaveAsResource,
 		ACTION_RenameColor,
 		ACTION_NewColor,
 		ACTION_RemoveColor,
 		ACTION_EditColor,
 		ACTION_DuplicateColor,
 		ACTION_NewPalette,
 		ACTION_PaletteResource = 1000, //this needs to be > all the above
 		ACTION_MAX
 	};

 	enum Hover {
 		HOVER_None = -1,
 		HOVER_Lock = -1000,
 		HOVER_PaletteName,
 		HOVER_ColorName,
 		HOVER_Color,
 		HOVER_ColorSelect,
 		HOVER_MAX
 	};

	Palette *palette;
	int pad;
	int curcolor; // actual current color
	int hover;
	IntRectangle inrect; // box that contains only the color grid. pname/curcolor/cname not included

	PaletteWindow(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder,
		anXWindow *prev,unsigned long nowner,const char *nsend);
	virtual ~PaletteWindow();
	virtual const char *whattype() { return "PaletteWindow"; }
	virtual int Event(const EventData *e,const char *mes);
	virtual int send();
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int PerformAction(int action);
	virtual ShortcutHandler *GetShortcuts();

	virtual void Refresh();
	virtual void findInrect();
	virtual int MoveResize(int nx,int ny,int nw,int nh);
	virtual int Resize(int nw,int nh);

	// color editing
	virtual int findColorIndex(int x,int y);
	virtual void LaunchEditColor();
	virtual void LaunchNewColor();
	virtual void RemoveColor();
	virtual void LaunchRenamePalette();
	virtual void LaunchRenameColor();

	// palette import / export
	virtual bool UseThis(Palette *new_palette, bool absorb_count);
	virtual const char *PaletteDir();
	virtual void LaunchImportPaletteDialog();
	virtual void LaunchExportDialog();
	virtual int LoadPalette(const char *file);
	virtual int ExportPalette(const char *file, int type);
};


} //namespace Laxkit;

#endif

