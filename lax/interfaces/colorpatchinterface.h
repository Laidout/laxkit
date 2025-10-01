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
//    Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef _LAX_COLORPATCHINTERFACE_H
#define _LAX_COLORPATCHINTERFACE_H

#include <lax/interfaces/patchinterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/linestyle.h>
#include <lax/screencolor.h>


namespace LaxInterfaces {


//-------------------------------------- ColorPatchData -------------------------

class ColorPatchData : public PatchData
{
  protected:
	virtual int TransferColors(int oldxsize, int oldysize);

  public:
	Laxkit::ScreenColor *colors;

	ColorPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	ColorPatchData();
	virtual ~ColorPatchData();
	const char *whattype() { return "ColorPatchData"; }
	virtual SomeData *duplicateData(SomeData *dup);

	virtual void CopyMeshPoints(PatchData *patch, bool usepath);
	virtual void Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual void SetColor(int pr,int pc,int red=0,int green=0,int blue=0,int alpha=0xffff);
	virtual void SetColor(int pr,int pc, Laxkit::ScreenColor *col);
	virtual void collapse(int rr,int cc);
	virtual void grow(int where, const double *tr, bool smooth);
	virtual int subdivide(int r,double rt,int c,double ct);
	virtual int subdivide(int xn=2,int yn=2);
	virtual int UpdateFromPath();
	virtual void FlipColorsV();
	virtual void FlipColorsH();

	virtual int WhatColor(double s,double t,Laxkit::ScreenColor *color_ret);
	virtual int hasColorData();

	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
};



//-------------------------------------- ColorPatchInterface -------------------------

enum ColorMeshActions {
	COLORMESH_FlipH = PATCHA_MAX,
	COLORMESH_FlipV,
	COLORMESH_MAX
};

class ColorPatchInterface : public PatchInterface
{
 protected:
	double Cx[16],Cy[16];
	Laxkit::ScreenColor *col1,*col2,*col3,*col4,col,cola,colb;
	virtual void patchpoint(PatchRenderContext *context,double s1,double t1, double s2,double t2); // p1,p2 are s1,t1, s2,t2
	virtual void patchpoint2(PatchRenderContext *context);
	virtual int sendcolor(Laxkit::ScreenColor *col);
	virtual int PerformAction(int action);
 public:
	ColorPatchData *cdata; // just a dummy so not have to dynamic_cast<ColorPatchData *> data

	ColorPatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~ColorPatchInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual const char *IconId() { return "ColorPatch"; }
	virtual const char *Name();
	virtual const char *whattype() { return "ColorPatchInterface"; }
	virtual const char *whatdatatype() { return "ColorPatchData"; }

	virtual int Refresh();
	virtual void drawpatch(int roff,int coff);
	virtual void drawpatch2(int roff,int coff);
	virtual void drawControlPoint(int i, bool hovered);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
//	virtual int Refresh();
	virtual int DrawData(anObject *ndata,anObject *a1=NULL,anObject *a2=NULL,int info=0);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int UseThis(anObject *newdata,unsigned int mask=0);
	virtual int UseThis(int id,int ndata);
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *e_data, const char *mes);
	virtual PatchData *newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual int SelectPoint(int c,unsigned int state);
};

} // namespace LaxInterfaces;

#endif

