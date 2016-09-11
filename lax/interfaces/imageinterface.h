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
//    Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef _LAX_IMAGEINTERFACE_H
#define _LAX_IMAGEINTERFACE_H

#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/somedata.h>
#include <lax/imageinfo.h>
#include <lax/laximages.h>
#include <lax/screencolor.h>



namespace LaxInterfaces {


//--------------------------------- ImageData -------------------------------
class ImageData : public Laxkit::ImageInfo, virtual public SomeData
{
 public:
	char previewflag;
	Laxkit::LaxImage *image;
	Laxkit::LaxImage *previewimage;


	ImageData(const char *nfilename=NULL, const char *npreview=NULL, 
			  int maxpx=0, int maxpy=0, char delpreview=0);
	virtual ~ImageData();
	virtual const char *whattype() { return "ImageData"; }
	ImageData &operator=(ImageData &i);
	virtual SomeData *duplicate(SomeData *dup);

	virtual void Flip(int horiz);
	virtual void Flip(flatpoint f1,flatpoint f2);
	virtual int SetImage(Laxkit::LaxImage *newimage, Laxkit::LaxImage *newpreview);
	virtual void SetDescription(const char *ndesc);
	virtual int UsePreview(const char *npreview, int maxpx=0, int maxpy=0, char del=0);
	virtual int LoadImage(const char *fname, const char *npreview=NULL, int maxpx=0, int maxpy=0, char del=0,char fit=0);
	virtual const char *Filename();
	
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext);
};

//--------------------------------- ImageInterface -------------------------------

#define IMAGEI_POPUP_INFO 1

enum ImageInterfaceActions {
	II_Normalize,
	II_Rectify,
	II_Decorations,
	II_ToggleLabels,
	II_FlipH,
	II_FlipV,
	II_Image_Info,
	II_MAX
};

class ImageInterface : public anInterface
{
 protected:
	int mode,mousedragged;
	flatpoint leftp;
	int mx,my,lx,ly;
	int max_preview_x, max_preview_y;
	ImageData *data;
	ObjectContext *ioc;

	virtual void runImageDialog();
	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

 public:
	unsigned long style;
	unsigned int controlcolor;
	int showdecs;
	bool showobj;
	int showfile;
	ImageInterface(int nid,Laxkit::Displayer *ndp,int nstyle=IMAGEI_POPUP_INFO);
	virtual ~ImageInterface();
	virtual const char *IconId() { return "Image"; }
	virtual const char *Name();
	virtual const char *whattype() { return "ImageInterface"; }
	virtual const char *whatdatatype() { return "ImageData"; }
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual anInterface *duplicate(anInterface *dup);
	virtual void deletedata();
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual ObjectContext *Context() { return ioc; }
	virtual int UseThis(Laxkit::anObject *nobj,unsigned int mask=0);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d);
	virtual int Refresh();

	virtual ImageData *newData();
};


} // namespace LaxInterfaces

#endif

