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
#ifndef _LAX_LINEPROFILE_H
#define _LAX_LINEPROFILE_H

#include <lax/lists.h>
#include <lax/resources.h>
#include <lax/objectfactory.h>
#include <lax/curveinfo.h>
#include <lax/previewable.h>

#include <lax/interfaces/pathinterface.h>


namespace LaxInterfaces {

class PathWeightNode;

//--------------------------- LineProfile -----------------------------

//! NOTE! This define must coexist with EngraverObjectTypes and LaxInterfaceDataTypes
#define OBJTYPE_LineProfile 200

// There is a macro Absolute in XI.h
#ifdef Absolute
	#undef Absolute
#endif

class LineProfile : public Laxkit::Resourceable, virtual public Laxkit::Previewable, public LaxFiles::DumpUtility
{
  public:
	double max_height; 
	double defaultwidth;
	double mint, maxt;
	bool wrap;

	Laxkit::PtrStack<PathWeightNode> pathweights;

	Laxkit::CurveInfo *width, *offset, *angle; //these are cache values made from pathweights
	int needtorecache;
	std::time_t nodes_mod_time;
	std::time_t cache_mod_time;
	virtual void UpdateCache();

	 //instance data:
	int start_type, end_type; //0=normal, 1=random
	enum class RepeatMode { NoRepeat, Number, Absolute, Fit };
	RepeatMode repeat_mode = RepeatMode::NoRepeat; //0==no repeat, !=0 is fit close to 
	double repeat_length = 1;
	double start_rand_width, end_rand_width; //actual start, end is start+-start_rand_width, end+-end_rand_width
	double start, end; //range 0..1, and start < end

	LineProfile();
	virtual ~LineProfile();
	virtual const char *whattype() { return "LineProfile"; } 
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

	virtual void AddNode(double nt,double no,double nw,double nangle);
	virtual int NeedToRecache() { return needtorecache || nodes_mod_time>cache_mod_time || nodes_mod_time==0; }

	virtual int GetWeight(double t, double *width, double *offset, double *angle);
	virtual bool Angled();
	virtual bool HasOffset();
	virtual bool ConstantWidth();

	//from Previewable:
	virtual int renderToBufferImage(Laxkit::LaxImage *image);
	virtual bool CanRenderPreview() { return true; }
};


//------------Default LineProfile resources

int InstallDefaultLineProfiles(Laxkit::ObjectFactory *factory, Laxkit::ResourceManager *resources);
//LineProfile **MakeStandardLineProfiles();




} // namespace LaxInterfaces


#endif

