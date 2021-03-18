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
//    Copyright (C) 2015 by Tom Lechner
//
#ifndef _LAX_INTERFACES_INTERFACEMANAGER_H
#define _LAX_INTERFACES_INTERFACEMANAGER_H

#include <lax/resources.h>
#include <lax/undo.h>
#include <lax/iconmanager.h>
#include <lax/displayer.h>
#include <lax/fontmanager.h>
#include <lax/objectfactory.h>
#include <lax/interfaces/somedata.h>
#include <lax/interfaces/aninterface.h>


namespace LaxInterfaces {


//-------------------------- InterfaceManager --------------------------
class InterfaceManager : public Laxkit::anObject
{
  private:
	Laxkit::Displayer *previewer;

  public:
  	//note anInterface will multiply these by UIScale()
	int preview_size;
	double thin_line;
	double near_threshhold;
	double near_threshhold2;
	double dragged_threshhold;

	InterfaceManager();
	virtual ~InterfaceManager();

	static InterfaceManager *GetDefault(bool create=true);
	static void SetDefault(InterfaceManager *nmanager, int absorb_count);


	Laxkit::ObjectFactory *datafactory;
	Laxkit::ResourceManager *tools;
	Laxkit::ResourceManager *resources;

	virtual Laxkit::ResourceManager *GetResourceManager();
	virtual Laxkit::ResourceManager *GetTools();
	virtual anInterface *GetTool(const char *tool);
	virtual Laxkit::FontManager *GetFontManager();
	virtual Laxkit::IconManager *GetIconManager();
	virtual Laxkit::UndoManager *GetUndoManager();
	virtual Laxkit::ObjectFactory *GetObjectFactory(); 
	virtual Laxkit::Displayer *GetPreviewDisplayer();
	virtual Laxkit::Displayer *GetDisplayer(int purpose);

	virtual anObject *NewObject(const char *type);
	virtual anObject *NewObject(int type);
	virtual SomeData *NewDataObject(const char *type);
	virtual SomeData *NewDataObject(int type);

	virtual int PreviewSize();
	virtual double ScreenLine() { return thin_line; }                 //anInterface funcs mult by their UIScale()
	virtual double NearThreshhold() { return near_threshhold; }       //anInterface funcs mult by their UIScale()
	virtual double NearThreshhold2() { return near_threshhold2; }     //anInterface funcs mult by their UIScale()
	virtual double DraggedThreshhold() { return dragged_threshhold; } //anInterface funcs mult by their UIScale()

	virtual int Resourcify(Laxkit::anObject *resource, const char *type=NULL);

	enum ManagerDrawFlags {
		DRAW_No_Preview = (1<<0),
		DRAW_MAX =0
	};

	virtual int DrawData(Laxkit::Displayer *dp, LaxInterfaces::SomeData *ndata,
							Laxkit::anObject *a1=NULL, Laxkit::anObject *a2=NULL, unsigned int info=0);
	virtual int DrawDataStraight(Laxkit::Displayer *dp, LaxInterfaces::SomeData *ndata,
							Laxkit::anObject *a1=NULL, Laxkit::anObject *a2=NULL, unsigned int info=0);

};



} //namespace LaxInterfaces




#endif

