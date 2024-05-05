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
			  int maxpx=0, int maxpy=0, char delpreview=0, int nindex=0);
	virtual ~ImageData();
	virtual const char *whattype() { return "ImageData"; }
	ImageData &operator=(ImageData &i);
	virtual SomeData *duplicate(SomeData *dup);

	virtual void Flip(int horiz);
	virtual void Flip(Laxkit::flatpoint f1, Laxkit::flatpoint f2);
	virtual int SetImage(Laxkit::LaxImage *newimage, Laxkit::LaxImage *newpreview);
	virtual void SetDescription(const char *ndesc);
	virtual int UsePreview(const char *npreview, int maxpx=0, int maxpy=0, char del=0);
	virtual int LoadImage(const char *fname, const char *npreview=NULL, int maxpx=0, int maxpy=0, char del=0,char fit=0,int index=0);
	virtual const char *Filename();
	
	virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
	virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);
	virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext);

	virtual int Undo(Laxkit::UndoData *data);
	virtual int Redo(Laxkit::UndoData *data);
};

//--------------------------------- ImageInterface -------------------------------

#define IMAGEI_POPUP_INFO 1

enum ImageInterfaceActions {
	II_Normalize,
	II_Rectify,
	II_Decorations,
	II_ToggleLabels,
	II_ToggleFileName,
	II_ToggleFileSize,
	II_FlipH,
	II_FlipV,
	II_Image_Info,
	II_MAX
};

class ImageInterface : public anInterface
{
 protected:
 	enum class Mode { Normal, DragNew };
	Mode mode;
	int mousedragged;
	Laxkit::flatpoint leftp;
	int mx,my,lx,ly;
	int max_preview_x, max_preview_y;
	ImageData *data;
	ObjectContext *ioc;

	//undo caching
	Laxkit::ImageInfo *prev_info;
	double m_orig[6];

	virtual void runImageDialog();
	Laxkit::ShortcutHandler *sc;
	virtual int PerformAction(int action);

 public:
	unsigned long style;
	unsigned int controlcolor;
	int showdecs;
	bool showobj;
	// bool show_labels;
	bool show_size;
	int show_file;

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


//------------------------- ImageDataUndo -------------------------------

class ImageDataUndo : public Laxkit::UndoData
{
  public:
	enum UTypes {
		UNDO_Transform  =(1<<0),
		UNDO_Info       =(1<<1),
		UNDO_MAX = 1
	};

	int type;
	Laxkit::Affine m, m_orig;
	Laxkit::ImageInfo *info_old = nullptr;
	Laxkit::ImageInfo *info_new = nullptr;

	ImageDataUndo(ImageData *object,
			     Laxkit::Affine *m_original,
			     Laxkit::Affine *m_new,
			     Laxkit::ImageInfo *old_info,
			     Laxkit::ImageInfo *new_info,
			     int ntype, int nisauto);
	virtual ~ImageDataUndo();
	virtual const char *Description();
};

} // namespace LaxInterfaces

#endif

