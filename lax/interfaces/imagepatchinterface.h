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
//    Copyright (C) 2004-2011 by Tom Lechner
//
#ifndef _LAX_IMAGEPATCHINTERFACE_H
#define _LAX_IMAGEPATCHINTERFACE_H

#include <lax/interfaces/patchinterface.h>
#include <lax/screencolor.h>

namespace LaxInterfaces {


//------------------------------------- ImagePatchData ------------------------
class ImagePatchData : public PatchData
{
 protected:
  	
 public:
	char *filename;

	unsigned long *idata; //array of screen ready color
	double im[6];
	int idataislocal;
	int iwidth,iheight;

	ImagePatchData(int iwidth, int iheight, unsigned long *ndata,int disl,
			double nscale,unsigned int stle);
	//ImagePatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	ImagePatchData(const char *file=NULL);
	virtual ~ImagePatchData(); 
	virtual const char *whattype() { return "ImagePatchData"; }
	virtual SomeData *duplicate(SomeData *dup);
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

	virtual unsigned long WhatColorLong(double s,double t);
	virtual int WhatColor(double s,double t,Laxkit::ScreenColor *color_ret);
	virtual int hasColorData();

	virtual void zap(); // zap to image
	virtual int SetImage(const char *fname);

};


//------------------------------ ImagePatchInterface -------------------------------

#define IMGPATCHI_POPUP_INFO 1

class ImagePatchInterface : public PatchInterface
{
 protected:
	ImagePatchData *ipdata;
	virtual void runImageDialog();
	virtual int PerformAction(int action);
 public:
	ImagePatchInterface(int nid,Laxkit::Displayer *ndp);
	virtual ~ImagePatchInterface();
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual const char *IconId() { return "ImagePatch"; }
	virtual const char *Name();
	virtual const char *whattype() { return "ImagePatchInterface"; }
	virtual const char *whatdatatype() { return "ImagePatchData"; }
	virtual anInterface *duplicate(anInterface *dup);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int UseThis(anObject *newdata,unsigned int mask=0);
	virtual int UseThis(int id,int ndata);
	virtual int DrawData(anObject *ndata,anObject *a1=NULL,anObject *a2=NULL,int info=0);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch,const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);

	virtual PatchData *newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle);
	virtual void drawpatch(int roff,int coff);
	virtual void patchpoint(PatchRenderContext *context, double s0,double ds,double t0,double dt,int n);
};

} //namespace LaxInterfaces

#endif


