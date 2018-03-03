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

#include <lax/interfaces/pathinterface.h>


namespace LaxInterfaces {


//--------------------------- LineProfile -----------------------------

//! NOTE! This define must coexist with EngraverObjectTypes and LaxInterfaceDataTypes
#define OBJTYPE_LineProfile 200


class LineProfile : public Laxkit::Resourceable, public LaxFiles::DumpUtility
{
  public:
	Laxkit::LaxImage *preview;

	double max_height; 
	double defaultwidth;
	double mint, maxt;
	bool wrap;

	Laxkit::PtrStack<PathWeightNode> pathweights;
	std::time_t nodes_mod_time;
	int needtorecache;
	virtual void UpdateCache();

	Laxkit::CurveInfo *width, *offset, *angle; //these are cache values made from pathweights
	std::time_t cache_mod_time;

	 //instance data:
	int start_type, end_type; //0=normal, 1=random
	double start_rand_width, end_rand_width; //zone around start and end to randomize
	double start, end;

	LineProfile();
	virtual ~LineProfile();
	virtual const char *whattype() { return "LineProfile"; } 
	virtual void dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context);
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context);

	virtual Laxkit::LaxImage *CreatePreview(int pwidth,int pheight);
	virtual Laxkit::LaxImage *Preview();
	virtual void touchNodes() { nodes_mod_time=times(NULL); }

	virtual void AddNode(double nt,double no,double nw,double nangle);
	virtual int NeedToRecache() { return nodes_mod_time>cache_mod_time || nodes_mod_time==0; }

	virtual int GetWeight(double t, double *width, double *offset, double *angle);
	virtual bool Angled();
	virtual bool HasOffset();
	virtual bool ConstantWidth();
};


//------------Default LineProfile resources

int InstallDefaultLineProfiles(Laxkit::ObjectFactory *factory, Laxkit::ResourceManager *resources);
//LineProfile **MakeStandardLineProfiles();




} // namespace LaxInterfaces


#endif

