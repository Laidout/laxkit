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
//    Copyright (C) 2016 by Tom Lechner
//
#ifndef _LAX_PERSPECTIVEINTERFACE_H
#define _LAX_PERSPECTIVEINTERFACE_H

#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces { 


//--------------------------- PerspectiveData -------------------------------------

//class PointTransform
//{
//  public:
//	virtual bool IsValid() = 0;
//	virtual Laxkit::flatpoint transform(Laxkit::flatpoint p)              = 0;
//	virtual Laxkit::flatpoint transform(double x,double y)        = 0;
//	virtual Laxkit::flatpoint transformInverse(double x,double y) = 0;
//	virtual Laxkit::flatpoint transformInverse(Laxkit::flatpoint p)       = 0;
//};

class PerspectiveTransform : public Laxkit::anObject
{
	double *getNormalizationCoefficients(double *src, double *dst, bool isInverse);

  public:
	double coeffs[9];
	double coeffsInv[9];

	double srcPts[8]; //xyxy...
	double dstPts[8];

	Laxkit::flatpoint from_ll, from_lr, from_ul, from_ur;
	Laxkit::flatpoint to_ll,   to_lr,   to_ul,   to_ur;

	PerspectiveTransform();
	PerspectiveTransform(Laxkit::flatpoint *nsrcPts, Laxkit::flatpoint *ndstPts);

	virtual int SetPoints(Laxkit::flatpoint *nsrcPts, Laxkit::flatpoint *ndstPts);
	virtual int SetFrom(Laxkit::flatpoint nfrom_ll, Laxkit::flatpoint nfrom_lr, Laxkit::flatpoint nfrom_ul, Laxkit::flatpoint nfrom_ur);
	virtual int SetTo  (Laxkit::flatpoint nfrom_ll, Laxkit::flatpoint nfrom_lr, Laxkit::flatpoint nfrom_ul, Laxkit::flatpoint nfrom_ur);

	virtual void ResetTransform();
	virtual void ComputeTransform();
	virtual bool IsValid();

	Laxkit::flatpoint transform(Laxkit::flatpoint p);
	Laxkit::flatpoint transform(double x,double y);
	Laxkit::flatpoint transformInverse(double x,double y);
	Laxkit::flatpoint transformInverse(Laxkit::flatpoint p);

	virtual int MapImage(SomeData *obj, Laxkit::LaxImage *initial, Laxkit::LaxImage *persped, int direction);

    virtual void dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context);
    virtual Laxkit::Attribute *dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context);
    virtual void dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context);

};

//--------------------------- PerspectiveInterface -------------------------------------

class PerspectiveInterface : public anInterface
{
  protected:
	int showdecs;
	bool show_preview;

	Laxkit::ShortcutHandler *sc;

	SomeData *data; //points to dataoc->obj
	ObjectContext *dataoc;

	Laxkit::LaxImage *initial, *persped;

	// *** dbg:
	Laxkit::flatpoint mousep, initialp;

	bool edit_from, edit_to;	
	bool show_grid;
	bool continuous_update;
	bool dont_update_transform;

	PerspectiveTransform *transform;

	int hover;
	int needtoremap;
	virtual void ComputeTransform();
	virtual void ResetTransform();
	virtual Laxkit::flatpoint ComputePoint(double x,double y);

	virtual int send();
	virtual int OtherObjectCheck(int x,int y,unsigned int state);
	virtual int SetupPreviewImages();

	virtual void Modified();

  public:
	enum PerspPoints {
		PERSP_None=0,
		PERSP_ll,
		PERSP_lr,
		PERSP_ul,
		PERSP_ur,

		PERSP_From_ll,
		PERSP_From_lr,
		PERSP_From_ul,
		PERSP_From_ur,

		PERSP_Move,
		PERSP_Reset,
		PERSP_Grid,
		PERSP_Preview,

		PERSP_MAX
	};

	enum PerspFlags { // for interface_flags
		PERSP_Dont_Change_Object = (1<<0),
		PERSP_Editable_From      = (1<<1),
		PERSP_Parent_Space       = (1<<2), //otherwise default to object space
		PERSP_FLAGS_MAX = 2
	};

	unsigned int interface_flags;
	Laxkit::ScreenColor inpoints, outpoints;

	PerspectiveInterface(anInterface *nowner, int nid,Laxkit::Displayer *ndp);
	virtual ~PerspectiveInterface();
	virtual anInterface *duplicateInterface(anInterface *dup);
	virtual const char *IconId() { return "Perspective"; }
	const char *Name();
	const char *whattype() { return "PerspectiveInterface"; }
	const char *whatdatatype();
	virtual ObjectContext *Context();
	virtual Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();

	virtual int scan(double x, double y);
};

} // namespace LaxInterfaces

#endif

