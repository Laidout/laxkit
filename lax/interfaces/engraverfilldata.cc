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
//    Copyright (C) 2014,2015 by Tom Lechner
//



#include <lax/interfaces/engraverfilldata.h>

#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/imagedialog.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>
#include <lax/iconmanager.h>
#include <lax/laxutils.h>
#include <lax/bitmaputils.h>
#include <lax/colorsliders.h>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <lax/fileutils.h>
#include <lax/filedialog.h>
#include <lax/utf8string.h>
#include <lax/popupmenu.h>
#include <lax/interfaces/freehandinterface.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/interfaces/somedataref.h>


using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {



//-------------------------- Engraver object creation stuff (see ObjectFactory)  ------------------------
anObject *NewTraceObject(int p, anObject *refobj)           { return new TraceObject;           }
anObject *NewEngraverLineQuality(int p, anObject *refobj)   { return new EngraverLineQuality;   }
anObject *NewEngraverTraceSettings(int p, anObject *refobj) { return new EngraverTraceSettings; }
anObject *NewEngraverTraceStack(int p, anObject *refobj)    { return new EngraverTraceStack;    }
anObject *NewNormalDirectionMap(int p, anObject *refobj)    { return new NormalDirectionMap;    }
anObject *NewEngraverDirection(int p, anObject *refobj)     { return new EngraverDirection;     }
anObject *NewEngraverSpacing(int p, anObject *refobj)       { return new EngraverSpacing;       }
anObject *NewEngraverFillStyle(int p, anObject *refobj)     { return new EngraverFillStyle;     }


void InstallEngraverObjectTypes(ObjectFactory *factory)
{
	factory->DefineNewObject(ENGTYPE_TraceObject,          "TraceObject"          , NewTraceObject,           NULL, 0);
	factory->DefineNewObject(ENGTYPE_EngraverLineQuality,  "EngraverLineQuality"  , NewEngraverLineQuality,   NULL, 0);
	factory->DefineNewObject(ENGTYPE_EngraverTraceSettings,"EngraverTraceSettings", NewEngraverTraceSettings, NULL, 0);
	factory->DefineNewObject(ENGTYPE_EngraverTraceStack,   "EngraverTraceStack"   , NewEngraverTraceStack,    NULL, 0);
	factory->DefineNewObject(ENGTYPE_NormalDirectionMap,   "NormalDirectionMap"   , NewNormalDirectionMap,    NULL, 0);
	factory->DefineNewObject(ENGTYPE_EngraverDirection,    "EngraverDirection"    , NewEngraverDirection,     NULL, 0);
	factory->DefineNewObject(ENGTYPE_EngraverSpacing,      "EngraverSpacing"      , NewEngraverSpacing,       NULL, 0);
	factory->DefineNewObject(ENGTYPE_EngraverFillStyle,    "EngraverFillStyle"    , NewEngraverFillStyle,     NULL, 0);
}


//------------------------------------- LinePointCache ------------------------

/*! \class LinePointCache
 * A cache point for use by LinePoint and EngraverFillData.
 */

LinePointCache::LinePointCache(int ntype)
{
	type=ntype;
	weight=0;
	on=dashon=ENGRAVE_On;
	bt=0;

	original=NULL;
	next=NULL;
	prev=NULL;
}

LinePointCache::LinePointCache(LinePointCache *prevpoint)
{
	type=ENGRAVE_Original;
	weight=0;
	on=dashon=ENGRAVE_On;
	bt=0;

	original=NULL;
	next=NULL;
	prev=prevpoint;
	if (prevpoint) prevpoint->next=this;
}

//! Deletes next.
LinePointCache::~LinePointCache()
{
	if (prev) {
		prev->next=NULL;
		prev=NULL;
	}
	if (next) {
		next->prev=NULL;
		delete next;
	}
	if (original) original->cache=NULL;
}

LinePoint *LinePointCache::PrevOriginal()
{
	LinePointCache *lc=this;
	while (lc && !lc->original) lc=lc->prev;
	if (lc) return lc->original;
	return NULL;
}

//! Add an OPEN line segment right after *this.
void LinePointCache::Add(LinePointCache *np)
{
	LinePointCache *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	np->next=next;
	if (next) next->prev=np;

	pp->prev=this;
	next=pp;
}

//! Add an OPEN line segment right before *this.
void LinePointCache::AddBefore(LinePointCache *np)
{
	LinePointCache *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	pp->prev=prev;
	if (prev) prev->next=pp;

	prev=np;
	np->next=this;
}

/*! Place np at a proper point after this. If np->bt==this->t, place right after this.
 * Returns np.
 */
LinePointCache *LinePointCache::InsertAfter(LinePointCache *np)
{
	LinePointCache *p=this;
	double t=np->bt;

	DBG if (t<0) {
	DBG 	cerr <<" ***\n *** t=="<<t<<" is BAD!!!!!\n ***"<<endl;
	DBG     LinePointCache *cc=NULL; cerr <<"--force segfault--"<<cc->bt;
	DBG     exit(1);
	DBG }

	if (t>=1) {

		while (p->type!=ENGRAVE_Original && p->prev) p=p->prev;
		LinePoint *lp=p->original;
		while (t>=1 && lp->next) {
			t--;
			lp=lp->next;
		}
	}
	while (t > p->bt && p->next && p->next->type!=ENGRAVE_Original) {
		if (t<p->next->bt) break;
		p=p->next;
		if (p->type==ENGRAVE_Original) {
			t-=1;
			if (t<=0) break;
		}
	}
	p->Add(np);
	return np;
}


/*! Remove *this from the chain.
 * Returns this->prev. If this->prev does not exist, then return this->next.
 */
LinePointCache *LinePointCache::Detach()
{
	LinePointCache *p=prev;
	if (prev) {
		prev->next=next;
	} else {
		p=next;
	}
	if (this->next) this->next->prev=prev;

	next=prev=NULL;
	original=NULL;
	return p;
}


//------------------------------------- LinePoint ------------------------
/*! \class LinePoint
 * Kind of a temp node for EngraverFillData.
 *
 * Each line point is a definite sample point that lies on an engraver line.
 * Between each LinePoint can be any number of cached points, to flesh
 * out and delineate dashes, and sections that are off, but don't fall cleanly
 * on LinePoint boundaries.
 */

LinePoint::LinePoint()
{
	type=ENGRAVE_Original;
	bt=0;
	on=ENGRAVE_On;
	row=col=0;
	s=t=0;
	weight=1;
	weight_orig=1;
	spacing=-1;
	next=prev=NULL;
	needtosync=1;

	cache=NULL;
}

LinePoint::LinePoint(double ss, double tt, double ww)
{
	type=ENGRAVE_Original;
	bt=0;
	on=ENGRAVE_On;
	next=prev=NULL;
	s=ss;
	t=tt;
	weight_orig=ww;
	weight=ww;
	spacing=-1;
	needtosync=1;

	cache=NULL;
}

LinePoint::~LinePoint()
{
	if (prev) prev->next=NULL;
	if (next) {
		next->prev=NULL;
		delete next;
	}

	if (cache) delete cache;
}


void LinePoint::Clear()
{
	if (next) delete next;
	next=NULL;
}

//! Add an OPEN line segment right after *this.
void LinePoint::Add(LinePoint *np)
{
	LinePoint *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	np->next=next;
	if (next) next->prev=np;

	pp->prev=this;
	next=pp;
}

//! Add an OPEN line segment right before *this.
void LinePoint::AddBefore(LinePoint *np)
{
	LinePoint *pp=np;
	while (pp->prev) pp=pp->prev;
	while (np->next) np=np->next;

	pp->prev=prev;
	if (prev) prev->next=pp;

	prev=np;
	np->next=this;
}

/*! Create initial this->cache, which is simple mirror of current line.
 * If cache already exists, it is verified to correspond to current line.
 */
void LinePoint::BaselineCache()
{
	LinePoint *l=this, *start=this;
	LinePointCache *lc=NULL;

	if (!l->cache) {
		 //need to create new cache
		do {
			l->cache=new LinePointCache(ENGRAVE_Original);
			l->cache->original=l;
			if (lc) lc->Add(l->cache);

			l->cache->p=l->p;
			l->cache->weight=l->weight;
			l->cache->on=l->on;

			lc=l->cache;
			l=l->next;
		} while (l && l!=start);

		if (l && l==start) l->cache->AddBefore(l->prev->cache);

	} else {
		 //need to validate existing cache
		DBG cerr <<" *** LinePoint::BaselineCache(), must implement validate existing cache!"<<endl;
	}
}

/*! Update the bez handles for this point only.
 */
void LinePoint::UpdateBezHandles()
{
	LinePoint *pp, *nn;
	pp=(prev ? prev : this);
	nn=(next ? next : this);

	flatpoint v=nn->p - pp->p;
	v.normalize();

	double sx=norm(p - pp->p)*.333;
	bez_before = p - v*sx;

	sx=norm(nn->p - p)*.333;
	bez_after = p + v*sx;
}

/*! Copy over everything except next and prev.
 */
void LinePoint::Set(LinePoint *pp)
{
	s=pp->s;
	t=pp->t;
	row=pp->row;
	col=pp->col;
	weight=pp->weight;
	weight_orig=pp->weight_orig;
	on=pp->on;
	needtosync=pp->needtosync;
	p=pp->p;
}


//--------------------------- EngraverLine -----------------------------
/*! \class EngraverLine
 * Holds info about individual lines in an EngraverPointGroup.
 */

EngraverLine::EngraverLine()
{
	startcap = endcap = 0;
	startspread = endspread = 1;
	startangle = endangle = 0;
	color=NULL;
	line=NULL;
}

EngraverLine::~EngraverLine()
{
	if (color) color->dec_count();
	delete line;
}


//--------------------------- EngraverDirection -----------------------------
/*! \class EngraverDirection
 */

EngraverDirection::EngraverDirection()
{
	type = PGROUP_Linear;
	map  = NULL;

	// spacing=1; //default
	resolution     = 1;   // default samples per spacing unit, default is 1
	default_weight = .1;  // a fraction of spacing
	position.x = position.y = .5;
	direction.x    = 1;  // default

	// line generation tinkering settings
	seed         = 0;  // for any randomness
	line_offset  = 0;  // 0..1 for random offset per line
	point_offset = 0;  // 0..1 for random offset per point
	noise_scale  = 1;  // applied per sample point, but offset per random line, not random at each point

	default_profile  = NULL;
	start_type       = 0;  // 0=normal, 1=random
	end_type         = 0;
	start_rand_width = end_rand_width = 0;
	profile_start    = 0;
	profile_end      = 1;
	max_height       = 1;
	scale_profile    = false;

	grow_lines   = false;
	merge        = true;
	fill         = true;
	spread       = 1.5;
	spread_depth = 3;
}

EngraverDirection::~EngraverDirection()
{
	if (default_profile) default_profile->dec_count();
}

/*! Override from anObject to produce a less verbose default id..
 */
const char *EngraverDirection::Id()
{
    if (object_idstr) return object_idstr;
    else object_idstr=make_id("Dir");
    return object_idstr; 
}

const char *EngraverDirection::Id(const char *str)
{
	anObject::Id(str);

	Resource *r=dynamic_cast<Resource*>(ResourceOwner());
	if (r) {
		makestr(r->name, str);
		makestr(r->Name, str);
	}

	return anObject::Id();
}

/*! Convenience function to return (translated) string of name of line direction type.
 */
const char *PGroupTypeName(int type)
{
	if (type==PGROUP_Linear)   return _("Linear");
	if (type==PGROUP_Radial)   return _("Radial");
	if (type==PGROUP_Spiral)   return _("Spiral");
	if (type==PGROUP_Circular) return _("Circular");
	if (type==PGROUP_Shell)    return _("Shell");
	if (type==PGROUP_S)        return _("S");
	if (type==PGROUP_Contour)  return _("Contour");
	if (type==PGROUP_Map)      return _("Map");
	if (type==PGROUP_Manual)   return _("Manual");
	if (type==PGROUP_Function) return _("Function");
	return _("Mystery direction");
}

/*! Convenience function to return (translated) string of name of line direction type.
 */
const char *PGroupTypeNameUntrans(int type)
{
	if (type==PGROUP_Linear)   return "Linear";
	if (type==PGROUP_Radial)   return "Radial";
	if (type==PGROUP_Spiral)   return "Spiral";
	if (type==PGROUP_Circular) return "Circular";
	if (type==PGROUP_Shell)    return "Shell";
	if (type==PGROUP_S)        return "S";
	if (type==PGROUP_Contour)  return "Contour";
	if (type==PGROUP_Map)      return "Map";
	if (type==PGROUP_Manual)   return "Manual";
	if (type==PGROUP_Function) return "Function";
	return "Mystery direction";
} 

/*! Note: untranslated.
 */
int PGroupTypeId(const char *value)
{
	if      (!strcasecmp(value,"Linear"))   return PGROUP_Linear;
	else if (!strcasecmp(value,"Radial"))   return PGROUP_Radial;
	else if (!strcasecmp(value,"Spiral"))   return PGROUP_Spiral;
	else if (!strcasecmp(value,"Circular")) return PGROUP_Circular;
	else if (!strcasecmp(value,"Shell"))    return PGROUP_Shell;
	else if (!strcasecmp(value,"S"))        return PGROUP_S;
	else if (!strcasecmp(value,"Contour"))  return PGROUP_Contour;
	else if (!strcasecmp(value,"Map"))      return PGROUP_Map;
	else if (!strcasecmp(value,"Manual"))   return PGROUP_Manual;
	else if (!strcasecmp(value,"Function")) return PGROUP_Function;

	return PGROUP_Unknown;
}

/*! Convenience function to return (translated) string of name of line direction type.
 */
const char *EngraverDirection::TypeName()
{
	return PGroupTypeName(type);
}

/*! Number of parameters in this->parameters that are marked for this->type.
 */
int EngraverDirection::NumParameters()
{
	int i=0;
	for (int c=0; c<parameters.n; c++) {
		if (parameters.e[c]->dtype==type) i++;
	}
	return i;
}

/*! Return the index'th parameter that pertains to this->type.
 */
EngraverDirection::Parameter *EngraverDirection::GetParameter(int index)
{
	for (int c=0; c<parameters.n; c++) {
		if (parameters.e[c]->dtype==type) {
			if (index==0) return parameters.e[c];
			index--;
		}
	}
	return NULL;
}

/*! Modify p to conform to expected min/max setup of known parameters.
 * Currently, this only means the ones for spiral.
 *
 * Return -1 for don't know how to validate. Else 0 for updated.
 */
int EngraverDirection::ValidateParameter(EngraverDirection::Parameter *p)
{
	if (p->dtype!=PGROUP_Spiral) return -1;

	if (!strcmp(p->name,"arms")) {
		p->type='i';
		p->min=1;
		p->min_type=1;
		p->max=10;
		p->max_type=0;
		return 0;

	} else if (!strcmp(p->name,"spin")) {
		p->type='b';
		p->min=0;
		p->min_type=1;
		p->max=1;
		p->max_type=1;
		return 0;
	}

	return -1;
}

EngraverDirection::Parameter *EngraverDirection::FindParameter(const char *name)
{
	if (!name) return NULL;
	for (int c=0; c<parameters.n; c++) {
		if (!strcmp(name,parameters.e[c]->name)) return parameters.e[c];
	}
	return NULL;
}

/*! \class Engraver::Direction::Parameter
 * Holds extra values to be used in EngraverPointGroup::Fill().
 *
 * nmint and maxt is the type of bounds for the min and max values. 0 is unbounded, 1 is bounded.
 */
EngraverDirection::Parameter::Parameter(const char *nname, const char *nName, int ndtype, char ntype,
				double nmin,int nmint, double nmax,int nmaxt, double nmingap, double nvalue)
{
	name     = newstr(nname);
	Name     = newstr(nName);
	type     = ntype;
	dtype    = ndtype;
	min      = nmin;
	max      = nmax;
	min_type = nmint;
	max_type = nmaxt;
	mingap   = nmingap;
	value    = nvalue;
}

EngraverDirection::Parameter::Parameter()
{
	name = Name = NULL;
	type        = 0;
	dtype       = 0;
	min = max   = 0;
	min_type    = max_type = 0;
	mingap      = 0;
	value       = 0;
}

EngraverDirection::Parameter::~Parameter()
{
	delete[] name;
	delete[] Name;
}

int EngraverDirection::SetType(int newtype)
{
	// *** need to adjust parameters as necessary
	type=newtype;

	Parameter *p;
	if (type==PGROUP_Spiral) {
		 //install extra parameters if necessary
		p=FindParameter("arms");
		if (!p) parameters.push(new Parameter("arms",_("Arms"),PGROUP_Spiral,'i', 1,1, 10,0, 1, 2));

		p=FindParameter("spin");
		if (!p) parameters.push(new Parameter("spin",_("Spin direction"),PGROUP_Spiral,'b', 0,1, 1,1, 1, 0));
	}
	return 0;
}

Laxkit::anObject *EngraverDirection::duplicate()
{
	EngraverDirection *dup=new EngraverDirection();

	dup->type      =type;
	//dup->map       =map->duplicate();
	dup->resolution = resolution;
	dup->default_weight = default_weight;
	dup->position = position;
	dup->direction = direction;
	dup->seed = seed;
	dup->line_offset = line_offset;
	dup->point_offset = point_offset;
	dup->noise_scale = noise_scale;

	//dup->default_profile = default_profile;
	dup->start_type = start_type;
	dup->end_type = end_type;
	dup->start_rand_width = start_rand_width;
	dup->end_rand_width = end_rand_width;
	dup->profile_start = profile_start;
	dup->profile_end = profile_end;

	dup->grow_lines = grow_lines;
	dup->merge = merge;
	dup->fill = fill;
	dup->spread = spread;
	dup->spread_depth = spread_depth;
	dup->merge_angle = merge_angle;

	if (parameters.n) {
		for (int c=0; c<parameters.n; c++) {
			Parameter *p=parameters.e[c];
			dup->parameters.push(new Parameter(p->name,p->Name,p->dtype,p->type,
								p->min,p->min_type, p->max,p->max_type, p->mingap, p->value));
		}
	}

	return dup;
}

void EngraverDirection::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *EngraverDirection::dump_out_atts(Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {
		att->push("type",      "linear",  "or radial, circular");
		att->push("position",  "(.5,.5)", "default origin for the pattern ");
		att->push("direction", "(1,0)",   "default direction for the pattern ");
		att->push("grow",      "true",    "whether to grow lines, or have predetermined lines");
		att->push("parameter", nullptr,   "extra parameters for the line type");
		return att;
	}


	att->push("id", Id());

	const char *str=PGroupTypeNameUntrans(type);
	if (str) att->push("type", str);

	char buffer[50];
	sprintf(buffer, "(%.10g, %.10g)", position.x,position.y);
	att->push("position",buffer);

	sprintf(buffer, "(%.10g, %.10g)", direction.x,direction.y);
	att->push("direction",buffer);

	//att->push("spacing", spacing);
	att->push("resolution", resolution);

	//if (map) ***

	att->push("default_weight",default_weight); //a fraction of spacing 

	
	EngraverDirection::Parameter *p;
	for (int c=0; c<parameters.n; c++) {
		p=parameters.e[c];
		Attribute *patt=att->pushSubAtt("parameter");

		patt->push("name",p->name);
		patt->push("Name",p->Name);
		patt->push("dtype", PGroupTypeNameUntrans(p->dtype));

		if (p->type=='b') {
			patt->push("type","boolean");
			patt->push("value",p->value==0 ? "false" : "true");

		} else if (p->type=='i') {
			patt->push("type","int");
			patt->push("value",(int)p->value);

		} else if (p->type=='r') {
			patt->push("type","real");
			patt->push("value",p->value);
		}

		if (p->type!='b') {
			patt->push("min",p->min);
			patt->push("min_bounded",p->min_type==0 ? "no" : "yes");
			patt->push("max",p->max);
			patt->push("max_bounded",p->max_type==0 ? "no" : "yes");
			patt->push("mingap",p->mingap);
		}
	}

	 //line generation tinkering settings
	att->push("seed", seed); //for any randomness
	att->push("line_offset", line_offset); //0..1 for random offset per line
	att->push("point_offset", point_offset); //0..1 for random offset per point
	att->push("noise_scale", noise_scale); ; //applied per sample point, but offset per random line, not random at each point

	if (default_profile) {
		if (default_profile->ResourceOwner()!=this) {
			 //resource'd default_profile
			char str[11+strlen(default_profile->Id())];
			sprintf(str,"resource: %s",default_profile->Id());
			att->push("default_profile", str);

		} else {
			Attribute *t=att->pushSubAtt("default_profile");
			default_profile->dump_out_atts(t, 0,savecontext);
		}
	}

    att->push("start_type", start_type==0 ? "normal" : "random");
    att->push("start_rand_width",start_rand_width);
    att->push("start",profile_start);
    att->push("end_type", end_type==0 ? "normal" : "random");
    att->push("end_rand_width",end_rand_width);
    att->push("end",profile_end);
    att->push("max_height",max_height);
    att->push("scale_profile", scale_profile ? "yes" : "no");

	att->push("grow", grow_lines ? "yes" : "no");
	att->push("fill", fill       ? "yes" : "no");
	att->push("merge", merge     ? "yes" : "no");
	att->push("spread", spread);
	att->push("spread_depth", spread_depth);
	att->push("merge_angle", merge_angle);

	return att;
}

void EngraverDirection::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) Id(value);

		} else if (!strcmp(name,"type")) {
			if (value) type=PGroupTypeId(value);

		} else if (!strcmp(name,"map")) {
			// *** map

		} else if (!strcmp(name,"parameter")) {
			const char *pp=att->attributes.e[c]->findValue("name");
			EngraverDirection::Parameter *p=FindParameter(pp);

			if (p) {
				pp=att->attributes.e[c]->findValue("value");

				if (p->type=='b') p->value=BooleanAttribute(pp);
				else DoubleAttribute(pp,&p->value, NULL);

			} else {
				p=new EngraverDirection::Parameter();

				for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
					name= att->attributes.e[c]->attributes.e[c2]->name;
					value=att->attributes.e[c]->attributes.e[c2]->value;

					if (!strcmp(name,"name")) {
						if (FindParameter(value)) {
							delete p;
							p=FindParameter(value);
						}
						makestr(p->name,value);

					} else if (!strcmp(name,"Name")) {
						makestr(p->Name,value);

					} else if (!strcmp(name,"value")) {
						if (p->type=='b') p->value=BooleanAttribute(value);
						else DoubleAttribute(value,&p->value, NULL);

					} else if (!strcmp(name,"mingap")) {
						DoubleAttribute(value,&p->mingap, NULL);

					} else if (!strcmp(name,"type")) {
						if (!value) p->type=0;
						else p->type=*value;

					} else if (!strcmp(name,"dtype")) {
						if (value) p->dtype=PGroupTypeId(value);

					} else if (!strcmp(name,"min")) {
						DoubleAttribute(value,&p->min, NULL);

					} else if (!strcmp(name,"min_bounded")) {
						p->min_type=!BooleanAttribute(value);

					} else if (!strcmp(name,"max")) {
						DoubleAttribute(value,&p->max, NULL);

					} else if (!strcmp(name,"max_bounded")) {
						p->max_type=!BooleanAttribute(value);
					}
				}

				ValidateParameter(p);
				parameters.push(p);
			}

		} else if (!strcmp(name,"position")) {
			FlatvectorAttribute(value,&position);

		} else if (!strcmp(name,"direction")) {
			FlatvectorAttribute(value,&direction);

		} else if (!strcmp(name,"resolution")) {
			DoubleAttribute(value,&resolution, NULL);

		//} else if (!strcmp(name,"spacing")) {
		//	DoubleAttribute(value,&spacing, NULL);

		} else if (!strcmp(name,"default_weight")) {
			DoubleAttribute(value,&default_weight, NULL);

		} else if (!strcmp(name,"line_offset")) {
			DoubleAttribute(value,&line_offset, NULL);

		} else if (!strcmp(name,"point_offset")) {
			DoubleAttribute(value,&point_offset, NULL);

		} else if (!strcmp(name,"noise_scale")) {
			DoubleAttribute(value,&noise_scale, NULL);

		} else if (!strcmp(name,"seed")) {
			IntAttribute(value,&seed, NULL);

		} else if (!strcmp(name,"default_profile")) {
			if (value && strstr(value,"resource:")==value) {
				value+=9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager=InterfaceManager::GetDefault(true);
				ResourceManager *rm=imanager->GetResourceManager();
				LineProfile *obj=dynamic_cast<LineProfile*>(rm->FindResource(value,"LineProfile"));
				if (obj) {
					if (default_profile) default_profile->dec_count();
					default_profile=obj;
					default_profile->inc_count();
				}

			} else { //not resourced
				LineProfile *obj=new LineProfile;
				obj->dump_in_atts(att->attributes.e[c], flag,context);
				obj->SetResourceOwner(this);
				if (default_profile) default_profile->dec_count();
				default_profile=obj;
			}

        } else if (!strcmp(name,"max_height")) {
            DoubleAttribute(value, &max_height, NULL);

		} else if (!strcmp(name,"scale_profile")) {
			scale_profile=BooleanAttribute(value);

        } else if (!strcmp(name,"start_type")) {
            if (value && !strcasecmp(value,"normal")) start_type=0;
            else start_type=1;

        } else if (!strcmp(name,"start_rand_width")) {
            DoubleAttribute(value, &start_rand_width, NULL);

        } else if (!strcmp(name,"start")) {
            DoubleAttribute(value, &profile_start, NULL);

        } else if (!strcmp(name,"end_type")) {
            if (value && !strcasecmp(value,"normal")) end_type=0;
            else end_type=1;

        } else if (!strcmp(name,"end_rand_width")) {
            DoubleAttribute(value, &end_rand_width, NULL);

        } else if (!strcmp(name,"end")) {
            DoubleAttribute(value, &profile_end, NULL); 

		} else if (!strcmp(name,"grow")) {
			grow_lines=BooleanAttribute(value);

		} else if (!strcmp(name,"fill")) {
			fill=BooleanAttribute(value);

		} else if (!strcmp(name,"merge")) {
			merge=BooleanAttribute(value);

		} else if (!strcmp(name,"spread")) {
			DoubleAttribute(value,&spread, NULL);

		} else if (!strcmp(name,"spread_depth")) {
			DoubleAttribute(value,&spread_depth, NULL);

		} else if (!strcmp(name,"merge_angle")) {
			DoubleAttribute(value,&merge_angle, NULL);

		}
	}
}

/*! Get values for start and end in range 0..1. If keep_increasing, ensure that start<end.
 * These values are based on profile_start, etc.
 *
 * Return value is 1 for end>start, 0 for end==start, -1 for end<start.
 */
int EngraverDirection::GetStartEnd(double *start_ret, double *end_ret, bool keep_increasing)
{
	if (profile_start==0 && profile_end==1
			&& start_rand_width==0 && end_rand_width==0) {
		*start_ret=0;
		*end_ret=1;
		return 1;
	}

	double start=profile_start;
	if (start_rand_width) start+=start_rand_width * (2*(double)random()/RAND_MAX - 1);
	if (start<0) start=0; else if (start>1) start=1;

	double end=profile_end;
	if (end_rand_width) end+=end_rand_width * (2*(double)random()/RAND_MAX - 1);
	if (end<0) end=0; else if (end>1) end=1;

	if (keep_increasing && start>end) {
		double dd=start;
		start=end;
		end=dd;
	}

	*start_ret=start;
	*end_ret=end;

	if (end==start) return 0;
	if (end>start) return 1;
	return -1;
}

/*! It's ok for profile to be NULL. It will remove any current profile.
 */
int EngraverDirection::InstallProfile(LineProfile *profile, int absorbcount)
{
	if (profile==default_profile) return 0;

	if (default_profile) default_profile->dec_count();
	default_profile=profile;
	if (default_profile && !absorbcount) default_profile->inc_count();

	return 0;
}


//----------------------------- EngraverSpacing -----------------------------------

/*! \class EngraverSpacing
 *
 * Holds basic spacing information about an engraving field.
 * Currently, this is just a default spacing value, or a value map of spacing.
 */


EngraverSpacing::EngraverSpacing()
{
	type=0;
	spacing=-1;
	map=NULL;
}

EngraverSpacing::~EngraverSpacing()
{
	if (map) map->dec_count();
}

/*! Override from anObject to produce a less verbose default id..
 */
const char *EngraverSpacing::Id()
{
    if (object_idstr) return object_idstr;
    else object_idstr=make_id("Space");
    return object_idstr; 
}

const char *EngraverSpacing::Id(const char *str)
{
	anObject::Id(str);

	Resource *r=dynamic_cast<Resource*>(ResourceOwner());
	if (r) {
		makestr(r->name, str);
		makestr(r->Name, str);
	}

	return anObject::Id();
}

Laxkit::anObject *EngraverSpacing::duplicate()
{
	EngraverSpacing *dup = new EngraverSpacing;

	dup->type = type;
	dup->spacing = spacing;
	//dup->map = map->duplicate();

	return dup;
}


void EngraverSpacing::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}


Laxkit::Attribute *EngraverSpacing::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {
		att->push("type",      nullptr, "default or map");
		att->push("spacing",   nullptr, "The default spacing");
		att->push("map (todo)",nullptr, "a value map for spacing");
		return att;
	}

	att->push("id", Id());
	att->push("type", type==0 ? "default" : "map");
	att->push("spacing",spacing); 

	if (map) {
		att->push("map","# ToDO!!");
		// ***
	}

	return att;
}

void EngraverSpacing::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) Id(value);

		} else if (!strcmp(name,"type")) {
			if (isblank(value)) type=0;
			else if (strcmp(value,"default")) type=0;
			else if (strcmp(value,"map")) type=1;

		} else if (!strcmp(name,"spacing")) {
			DoubleAttribute(value,&spacing, NULL);

		} else if (!strcmp(name,"map")) {
			// ***
			cerr <<" *** need to implement map for EngraverSpacing::dump_in_atts()"<<endl;
		}
	}
}


//----------------------------- EngraverLineQuality -----------------------------------

/*! \class EngraverLineQuality
 * Class to hold info about how to render broken lines, caps, and custom mappings of
 * stored weights to actual widths.
 */

EngraverLineQuality::EngraverLineQuality()
{
	//dash_length=spacing*2;
	dash_length=2; //a multiple of group->spacing

	dash_density=0;
	dash_randomness=0;
	randomseed=0;
	zero_threshhold=0;
	broken_threshhold=0;
	dash_taper=0;

	indashcaps =0;
	outdashcaps=0;
	startcaps  =0;
	endcaps    =0;
}

EngraverLineQuality::~EngraverLineQuality()
{
}

/*! Override from anObject to produce a less verbose default id..
 */
const char *EngraverLineQuality::Id()
{
    if (object_idstr) return object_idstr;
    else object_idstr=make_id("Dashes");
    return object_idstr; 
}

const char *EngraverLineQuality::Id(const char *str)
{
	anObject::Id(str);

	Resource *r=dynamic_cast<Resource*>(ResourceOwner());
	if (r) {
		makestr(r->name, str);
		makestr(r->Name, str);
	}

	return anObject::Id();
}

Laxkit::anObject *EngraverLineQuality::duplicate()
{
	EngraverLineQuality *dup = new EngraverLineQuality();

	dup->dash_length       = dash_length;
	dup->dash_density      = dash_density;
	dup->dash_randomness   = dash_randomness;
	dup->randomseed        = randomseed;
	dup->zero_threshhold   = zero_threshhold;
	dup->broken_threshhold = broken_threshhold;
	dup->dash_taper        = dash_taper; 
	dup->indashcaps        = indashcaps;
	dup->outdashcaps       = outdashcaps;
	dup->startcaps         = startcaps;
	dup->endcaps           = endcaps;

	return dup;
}

void EngraverLineQuality::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *EngraverLineQuality::dump_out_atts(Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {

		att->push("dash_length",      "2", "This times group->spacing is the length dashes should be between breaks.");
		att->push("dash_randomness",  "0", "0 for regular spacing, up to 1 how much to randomize spacing.");
		att->push("zero_threshhold",  "0", "Weights below this value are considered off");
		att->push("broken_threshhold","0", "Weights below this value are rendered as broken lines.");
		att->push("dash_taper",       "0", "How much to shrink weight of dashes as zero weight approaches. 0 is all the way, 1 is not at all.");
		att->push("density",          "0", "The minimum dash density. 0 for all blank at 0, 1 for all solid at 0 thickness.");
		att->push("indashcaps",       "0", "Line cap at the inside start of a dash. Todo!");
		att->push("outdashcaps",      "0", "Line cap at the inside end of a dash. Todo!");
		att->push("startcaps",        "0", "Line cap at the start of a whole line. Todo!");
		att->push("endcaps",          "0", "Line cap at the end of a whole line. Todo!");
		return att;
	}

	att->push("id", Id());
	att->push("dash_length",dash_length); 
	att->push("dash_randomness",dash_randomness); 
	att->push("zero_threshhold",zero_threshhold);
	att->push("broken_threshhold",broken_threshhold); 
	att->push("dash_taper",dash_taper); 
	att->push("density",dash_density); 
	att->push("indashcaps",indashcaps);
	att->push("outdashcaps",outdashcaps); 
	att->push("startcaps",startcaps); 
	att->push("endcaps",endcaps);

	return att;
}

void EngraverLineQuality::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) Id(value);

		} if (!strcmp(name,"dash_length")) {
			DoubleAttribute(value,&dash_length, NULL);

		} else if (!strcmp(name,"dash_randomness")) {
			DoubleAttribute(value,&dash_randomness, NULL);

		} else if (!strcmp(name,"density")) {
			DoubleAttribute(value,&dash_density, NULL);

		} else if (!strcmp(name,"zero_threshhold")) {
			DoubleAttribute(value,&zero_threshhold, NULL);

		} else if (!strcmp(name,"broken_threshhold")) {
			DoubleAttribute(value,&broken_threshhold, NULL);

		} else if (!strcmp(name,"dash_taper")) {
			DoubleAttribute(value,&dash_taper, NULL);

		} else if (!strcmp(name,"indashcaps")) {
			IntAttribute(value,&indashcaps, NULL);

		} else if (!strcmp(name,"outdashcaps")) {
			IntAttribute(value,&outdashcaps, NULL);

		} else if (!strcmp(name,"startcaps")) {
			IntAttribute(value,&startcaps, NULL);

		} else if (!strcmp(name,"endcaps")) {
			IntAttribute(value,&endcaps, NULL);
		}
	}
}

/*! From a weight value, translate through the dash settings to get a new weight value.
 * It will differ from original weight when dash_taper!=0.
 *
 * If weight>=broken, return 2.
 * If weight<=zero, return 1.
 * Otherwise return 0.
 */
int EngraverLineQuality::GetNewWeight(double weight, double *weight_ret)
{
	if (weight>=broken_threshhold) { *weight_ret=weight; return 2; }
	if (weight<=zero_threshhold)   { *weight_ret=weight; return 1; } 

	 //now we figure out the length, gap placement, and width of a dash.
	 //taper means dash width varies from broken to zero+taper*(broken-zero)
	 //actual width varies from broken down to zero

	double a=(weight-zero_threshhold)/(broken_threshhold-zero_threshhold); //0..1, how long between zero and broken
	*weight_ret   = broken_threshhold*a + (dash_taper*(broken_threshhold-zero_threshhold)+zero_threshhold)*(1-a);
	return 0;
}



//--------------------------- NormalDirectionMap -----------------------------
    
NormalDirectionMap::NormalDirectionMap()
{   
    normal_map=NULL;
	data=NULL;
	width=height=0;
	angle=0;
}

NormalDirectionMap::NormalDirectionMap(const char *file)
{   
    normal_map = ImageLoader::LoadImage(file);
	data=NULL;
	width=height=0;
	angle=0;

    if (normal_map) {
        width=normal_map->w();
        height=normal_map->h();
		unsigned char *dd=normal_map->getImageBuffer();
		data=new unsigned char[width*height*4];
		memcpy(data, dd, width*height*4);
		normal_map->doneWithBuffer(dd);
    }
} 

NormalDirectionMap::~NormalDirectionMap()
{
	delete[] data;
    if (normal_map) normal_map->dec_count();
}

void NormalDirectionMap::Clear()
{
	if (normal_map) normal_map->dec_count();
	normal_map = NULL;
	delete[] data;
	data = NULL;
	width = height = 0;
}

/*! Calling with NULL just calls Clear().
 * If image loading fails, return 1.
 * Success returns 0.
 */
int NormalDirectionMap::Load(const char *file)
{
	if (file==NULL) {
		Clear();
		return 0;
	}

	LaxImage *img = ImageLoader::LoadImage(file);
	if (!img) return 1;

	if (normal_map) normal_map->dec_count();
	normal_map=img;

	delete[] data;
	data=NULL;

	width=normal_map->w();
	height=normal_map->h();
	unsigned char *dd=normal_map->getImageBuffer();
	data=new unsigned char[width*height*4];
	memcpy(data, dd, width*height*4);
	normal_map->doneWithBuffer(dd);

	return 0;
}

flatpoint NormalDirectionMap::Direction(double x,double y)
{
    flatpoint p=m.transformPoint(flatpoint(x,y));

    if (p.x<0 || p.x>=width || p.y<0 || p.y>=height) return flatpoint(0,0);

    int i=((int)p.y*width+(int)p.x)*4;
    //cerr <<"p: "<<(int)p.x<<","<<(int)p.y<<" i:"<<i<<"  "<<endl;
    //p=flatpoint(data[i]-128,data[i+1]-128);
    //p=flatpoint(data[i+2]-128,data[i+3]-128);
    p=flatpoint((int)(data[i+1])-128,(int)(data[i+2])-128);

    return p;
}




//------------------------------ TraceObject -------------------------------

/*! \class TraceObject
 *
 * Class to hold information about a tracing source, for use in EngraverTraceSettings.
 */

TraceObject::TraceObject()
{
	DBG cerr << "TraceObject constructor, object_id="<<object_id<<endl;

	type=TRACE_None;

	object=NULL;
	image_file=NULL;

	samplew=sampleh=0;
	trace_sample_cache=NULL;
	cachetime=0;

	 //black and white cache:
	tw=th=0; //dims of trace_ref_bw
	trace_ref_bw=NULL;
}

TraceObject::~TraceObject()
{
	if (object) object->dec_count();
	delete[] image_file;
	delete[] trace_sample_cache;
	delete[] trace_ref_bw;
}

Laxkit::anObject *TraceObject::duplicate()
{
	TraceObject *dup = new TraceObject;
	dup->type = type;
	makestr(dup->object_idstr, object_idstr);
	makestr(dup->image_file, image_file);
	dup->object = object;
	if (object) object->inc_count();

	return dup;
}

void TraceObject::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *TraceObject::dump_out_atts(Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {
		att->push("id",       "blah",  "Id of the object");
		att->push("snapshot", nullptr, "a list of points from which to build a snapshot image");
		att->push("current",  nullptr, "Use the current settings to base adjustments on");
		att->push("gradient", nullptr, "GradientData to use");
		att->push("image",    nullptr, "ImageData to use");
		att->push("object",   nullptr, "A SomeDataRef to use");
		return att;
	}


	att->push("id", Id());

	if (type==TraceObject::TRACE_Snapshot) {
		att->push("snapshot");
		cerr << " *** need to implement TraceObject::dump_out_atts for snapshot!!"<<endl;

	} else if (type==TraceObject::TRACE_Current) {
		att->push("current");

	} else if (type==TraceObject::TRACE_LinearGradient || type==TraceObject::TRACE_RadialGradient) {
		att->push("gradient", type==TraceObject::TRACE_LinearGradient ? "linear" : "radial");
		object->dump_out_atts(att->attributes.e[att->attributes.n-1], what, savecontext);

	} else if (type==TraceObject::TRACE_ImageFile) {
		att->push("image");
		object->dump_out_atts(att->attributes.e[att->attributes.n-1], what, savecontext); 

	} else if (type==TraceObject::TRACE_Object) {
		att->push("object", object->whattype());
		object->dump_out_atts(att->attributes.e[att->attributes.n-1], what, savecontext); 
	}

	return att;
}

void TraceObject::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id") && !isblank(value)) {
			Id(value);

		} else if (!strcmp(name, "current")) {
			Install(TRACE_Current, NULL);

		} else if (!strcmp(name, "snapshot")) {
			cerr << " *** need to implement TraceObject::dump_in_atts for snapshot!!"<<endl;
			//*** read in point list and regenerate an image
			Install(TRACE_Snapshot, NULL);

		} else if (!strcmp(name, "gradient")) {
			GradientData *gradient=new GradientData;
			gradient->dump_in_atts(att->attributes.e[c], flag, context);
			Install(gradient->IsRadial() ? TRACE_RadialGradient : TRACE_LinearGradient, gradient);
			gradient->dec_count();

		} else if (!strcmp(name, "image")) {
			ImageData *img=new ImageData;
			img->dump_in_atts(att->attributes.e[c], flag,context);
			Install(TRACE_ImageFile, img);
			img->dec_count();

		} else if (!strcmp(name, "object")) {
			SomeDataRef *ref=dynamic_cast<SomeDataRef*>(somedatafactory()->NewObject("SomeDataRef"));
			ref->dump_in_atts(att->attributes.e[c], flag,context);
			Install(TRACE_Object, ref);
			ref->dec_count();

		} 
	}

	ClearCache(false);
}

/*! Note this is more for one time lookups. Not very efficient for mass lookups.
 * Returns -1 for point outside of trace object.
 *
 * trace_sample_cache MUST be set up properly. TRACE_Current is just pass through for p->weight.
 *
 * If transform!=NULL, then transform p->p by transform before using.
 */
double TraceObject::GetValue(LinePoint *p, double *transform)
{
	if (type==TRACE_Current) return p->weight;

	int x,y,i;
	int sample, samplea;
	flatpoint pp=p->p;
	if (transform) pp=transform_point(transform,pp);

	x=samplew*(pp.x-object->minx)/(object->maxx-object->minx);
	y=sampleh*(pp.y-object->miny)/(object->maxy-object->miny);

	if (x>=0 && x<samplew && y>=0 && y<sampleh) {
		i=4*(x+(sampleh-y)*samplew);

		samplea=trace_sample_cache[i+3];
		if (samplea==0) return -1; //transparent sample!

		sample=0.3*trace_sample_cache[i] + 0.59*trace_sample_cache[i+1] + 0.11*trace_sample_cache[i+2];
		if (sample>255) {
			sample=255;
		}

		return (255-sample)/255.; 
	}
	
	// else point outside sample area
	return -1;
}

/*! Count on obj will be incremented, unless obj is alread object.
 *
 * If TRACE_ImageFile, then obj must be an ImageData.
 */
void TraceObject::Install(TraceObjectType ntype, SomeData *obj)
{
	if (object!=obj) {
		if (object) object->dec_count();
		object=obj;
		if (object) object->inc_count();
	}

	type=ntype;


	char *identifier=NULL;
	if (type==TRACE_Current) {
		makestr(identifier, _("current"));

	} else if (type==TRACE_Snapshot) {
		char buffer[100];
		sprintf(buffer,_("snap "));
		struct tm t;
		time_t tt=time(NULL);
		localtime_r(&tt,&t);
		strftime(buffer+strlen(buffer),100-strlen(buffer), "%l:%M %p", &t);
		makestr(identifier, buffer);

	} else if (type==TRACE_ImageFile) {
		delete[] identifier;
		ImageData *img=dynamic_cast<ImageData*>(obj);
		const char *bname=lax_basename(img->Filename());
		identifier=new char[strlen(_("img: %s"))+strlen(bname)+1];
		sprintf(identifier,_("img: %s"),bname);

	} else if (type==TRACE_LinearGradient || type==TRACE_RadialGradient) {
		makestr(identifier, _("Gradient"));

	} else if (type==TRACE_Object) { 
		delete[] identifier;
		SomeDataRef *ref=dynamic_cast<SomeDataRef*>(object);
		identifier=new char[strlen(_("ref: %s"))+strlen(ref->thedata_id)+1];
		sprintf(identifier, _("ref: %s"),ref->thedata_id);
	}

	if (isblank(object_idstr)) makestr(object_idstr, identifier);
	delete[] identifier;
}

void TraceObject::ClearCache(bool obj_too)
{
	delete[] trace_sample_cache;
	trace_sample_cache=NULL;
	samplew=sampleh=0;
	cachetime=0;

	if (obj_too) {
		delete[] object_idstr;
		object_idstr=NULL;
		object->dec_count();
		object=NULL;
	}
}

int TraceObject::NeedsUpdating()
{
	if (!trace_sample_cache) return 1;
	if (type==TRACE_Object) {
		if (!object) return 1;
		if (object->modtime>cachetime) return 1;
	}
	
	return 0;
}

/*! Calling this will always force a redrawing of the cache.
 * trace_sample_cache will only be reallocated if it is not currently large enough to hold data from object.
 *
 */
int TraceObject::UpdateCache()
{
	//we need to render the trace object to a grayscale sample board
	

	if (!object) {
		ClearCache(false);
		return 0;
	}

	double w,h;
	w=object->maxx - object->minx;
	h=object->maxy - object->miny;
	if (w<500 && h<500) {
		if (w<h) {
			double a=w/h;
			h=500;
			w=h*a;
		} else {
			double a=h/w;
			w=500;
			h=500*a;
		}
	}


	Displayer *ddp=newDisplayer(NULL);
	ddp->CreateSurface((int)w,(int)h);

	 // setup ddp to have proper scaling...
	ddp->NewTransform(1.,0.,0.,-1.,0.,0.);
	ddp->defaultRighthanded(true);
	//ddp->NewTransform(1.,0.,0.,1.,0.,0.);
	DoubleBBox bbox;
	bbox.addtobounds(object);
	ddp->SetSpace(bbox.minx,bbox.maxx,bbox.miny,bbox.maxy);
	ddp->Center(bbox.minx,bbox.maxx,bbox.miny,bbox.maxy);

	ddp->NewBG(255,255,255); // *** this should be the paper color for paper the page is on...
	ddp->NewFG(0,0,0,255);
	//ddp->m()[4]=0;
	//ddp->m()[5]=2*h;
	//ddp->Newmag(w/(bbox.maxx-bbox.minx));
	ddp->ClearWindow();

	InterfaceManager *imanager = InterfaceManager::GetDefault(true);
	imanager->DrawDataStraight(ddp, object, NULL,NULL,0);
	//-----
	//viewport->DrawSomeData(ddp,object, NULL,NULL,0);
	LaxImage *img=ddp->GetSurface();
	ddp->EndDrawing();

	if (!img) {
		DBG cerr <<"could not render trace object"<<endl;
		return 1;
	}

	DBG img->Save("DBG-trace.png", "png");

	delete ddp;

	if (trace_sample_cache) {
		if (img->w()*img->h()>samplew*sampleh) {
			 //old isn't big enough to hold the new
			delete[] trace_sample_cache;
			trace_sample_cache=NULL;
		}
	}
	samplew=img->w();
	sampleh=img->h();
	if (trace_sample_cache==NULL) trace_sample_cache=new unsigned char[4*samplew*sampleh];

	unsigned char *data=img->getImageBuffer();
	memcpy(trace_sample_cache, data, 4*samplew*sampleh);
	img->doneWithBuffer(data);
	cachetime=time(NULL);
	img->dec_count();

	return 0;
}


//------------------------------ EngraverTraceSettings -------------------------------

/*! \class EngraverTraceSettings
 * Holds settings about tracing points from objects for EngraverFillData.
 */

EngraverTraceSettings::EngraverTraceSettings()
{
	lock_ref_to_obj=true;
	continuous_trace=false; 
	show_trace=true;
	group=-1;
	traceobj_opacity=1;
	tracetype=TRACE_Set;
	traceobject=NULL;
	value_to_weight=new CurveInfo;
}

EngraverTraceSettings::~EngraverTraceSettings()
{
	if (traceobject) traceobject->dec_count();
	if (value_to_weight) value_to_weight->dec_count();
}

/*! Override from anObject to produce a less verbose default id..
 */
const char *EngraverTraceSettings::Id()
{
    if (object_idstr) return object_idstr;
    else object_idstr=make_id("Trace");
    return object_idstr; 
}

const char *EngraverTraceSettings::Id(const char *str)
{
	anObject::Id(str);

	Resource *r=dynamic_cast<Resource*>(ResourceOwner());
	if (r) {
		makestr(r->name, str);
		makestr(r->Name, str);
	}

	return anObject::Id();
}

/*! Warning: will link, NOT duplicate dashes, trace, etc.
 */
Laxkit::anObject *EngraverTraceSettings::duplicate()
{
	EngraverTraceSettings *dup = new EngraverTraceSettings;

	dup->continuous_trace = continuous_trace;
	dup->traceobject = traceobject;
	if (traceobject) traceobject->inc_count();

	dup->lock_ref_to_obj = lock_ref_to_obj;
	dup->traceobj_opacity = traceobj_opacity;
	dup->tracetype = tracetype;

	return dup;
}

/*! Just returns traceobject->Id() or NULL if there is no traceobject.
 */
const char *EngraverTraceSettings::Identifier()
{
	if (traceobject) return traceobject->Id();
	return NULL;
}

/*! Installs an existing traceobject.
 */
void EngraverTraceSettings::Install(TraceObject *nobject)
{
	if (traceobject==nobject) {
		if (traceobject->ResourceOwner()==NULL) traceobject->SetResourceOwner(this);
		return;
	}

	if (traceobject) {
		if (traceobject->ResourceOwner()==this) traceobject->SetResourceOwner(NULL);
		traceobject->dec_count();
	}
	traceobject=nobject;
	if (traceobject->ResourceOwner()==NULL) traceobject->SetResourceOwner(this);
	traceobject->inc_count();
}

/*! Creates brand new traceobject, dec_counts old.
 */
void EngraverTraceSettings::Install(TraceObject::TraceObjectType ntype, SomeData *obj)
{
	if (traceobject) {
		traceobject->SetResourceOwner(NULL);
		traceobject->dec_count();
	}
	traceobject=new TraceObject;
	traceobject->SetResourceOwner(this);
	traceobject->Install(ntype,obj);
}

/*! Remove traceobject if obj_too, else clear the object within traceobject.
 */
void EngraverTraceSettings::ClearCache(bool obj_too)
{
	if (traceobject) {
		if (obj_too) {
			traceobject->dec_count();
			traceobject=NULL;
		} else {
			traceobject->ClearCache(obj_too);
		}
	}
}

void EngraverTraceSettings::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *EngraverTraceSettings::dump_out_atts(Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {
		att->push("curve",           nullptr, "The value to weight curve");
		att->push("view_opacity",    nullptr, "Opacity of background reference");
		att->push("show_trace true", nullptr, "Whether to show the trace object at all");
		att->push("continuous true", nullptr, "Whether to trace continuously");
		att->push("trace",           nullptr, "What to trace from");
		return att;
	}

	char buffer[50];

	att->push("id", Id());

	sprintf(buffer,"%.10g",traceobj_opacity);
	att->push("view_opacity", buffer);
	att->push("show_trace", show_trace?"true":"false" );
	att->push("continuous", continuous_trace?"true":"false" );

	if (traceobject) {
		if (traceobject->ResourceOwner()!=this) {
			 //resource'd traceobject
			char str[11+strlen(traceobject->Id())];
			sprintf(str,"resource: %s",traceobject->Id());
			att->push("traceobject",str);

		} else {
			Attribute *t=att->pushSubAtt("traceobject");
			traceobject->dump_out_atts(t, 0,savecontext);
		}
	}

	Attribute *att2=att->pushSubAtt("curve");
	value_to_weight->dump_out_atts(att2,what,savecontext);

	return att;
}

void EngraverTraceSettings::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) anObject::Id(value);

		} else if (!strcmp(name,"curve")) {
			value_to_weight->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"view_opacity")) {
			DoubleAttribute(value,&traceobj_opacity, NULL);

		} else if (!strcmp(name,"show_trace")) {
			show_trace=BooleanAttribute(value);

		} else if (!strcmp(name,"continuous")) {
			continuous_trace=BooleanAttribute(value);

		} else if (!strcmp(name,"traceobject")) {
			if (value && strstr(value,"resource:")==value) {
				value+=9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager=InterfaceManager::GetDefault(true);
				ResourceManager *rm=imanager->GetResourceManager();
				TraceObject *obj=dynamic_cast<TraceObject*>(rm->FindResource(value,"TraceObject"));
				if (obj) {
					if (traceobject) traceobject->dec_count();
					traceobject=obj;
					traceobject->inc_count();
				}

			} else { //not resourced
				TraceObject *obj=new TraceObject;
				obj->dump_in_atts(att->attributes.e[c], flag,context);
				obj->SetResourceOwner(this);
				if (traceobject) traceobject->dec_count();
				traceobject=obj;
				//traceobject->inc_count(); 
			}

		} else if (!strcmp(name,"trace")) {
			 //DEPRECATED AS OF 0.096
			if (!strcmp(value,"current")) {
				Install(TraceObject::TRACE_Current, NULL);

			} else if (!strcmp(value,"gradient")) {
				GradientData *grad=new GradientData;
				grad->dump_in_atts(att->attributes.e[c], flag,context);
				if (grad->IsRadial()) Install(TraceObject::TRACE_RadialGradient, grad);
				else Install(TraceObject::TRACE_LinearGradient, grad);
				grad->dec_count();

			} else if (!strcmp(value,"object")) {
				SomeDataRef *ref=dynamic_cast<SomeDataRef*>(somedatafactory()->NewObject("SomeDataRef"));
				ref->dump_in_atts(att->attributes.e[c], flag,context);
				Install(TraceObject::TRACE_Object, ref);
				ref->dec_count();

			} else if (!strcmp(value,"image")) {
				ImageData *img=new ImageData;
				img->dump_in_atts(att->attributes.e[c], flag,context);

				const char *bname=lax_basename(img->Filename());
				if (bname) {
					Install(TraceObject::TRACE_ImageFile, img);
				}

				img->dec_count();

			} else {
				cerr << " *** unknown TraceObject type on EngraverTraceSettings::dump_in_att!"<<endl;
			}
		}
	}

	//Id(); //force creation of id if blank
}

//---------------------------------------- EngraverTraceStack 
/*! \class EngraverTraceStack
 */

EngraverTraceStack::TraceNode::TraceNode(EngraverTraceSettings *nsettings, double namount, bool nvis)
{
	visible = nvis;
	settings = nsettings;
	if (settings) settings->inc_count();
	amount = namount;
}

EngraverTraceStack::TraceNode::~TraceNode()
{
	if (settings) settings->dec_count();
}

EngraverTraceStack::EngraverTraceStack()
{
}

EngraverTraceStack::~EngraverTraceStack()
{
}


/*! Return the specified settings.
 */
EngraverTraceStack::TraceNode *EngraverTraceStack::Settings(int which)
{
	if (which<0 || which >= nodes.n) return NULL;
	return nodes.e[which];
}

/*! Return the node's amount value.
 */
double EngraverTraceStack::Amount(int which)
{
	if (which<0 || which >= nodes.n) return 0;
	return nodes.e[which]->amount;
}

/*! Set the node's amount value.
 */
double EngraverTraceStack::Amount(int which, double newamount)
{
	if (which<0 || which >= nodes.n) return 0;
	nodes.e[which]->amount=newamount;
	return newamount;
}

bool EngraverTraceStack::Visible(int which)
{
	if (which<0 || which >= nodes.n) return false;
	return nodes.e[which]->visible;
}

bool EngraverTraceStack::Visible(int which, bool newvisible)
{
	if (which<0 || which >= nodes.n) return false;
	nodes.e[which]->visible=newvisible;
	return newvisible;
}

int EngraverTraceStack::PushSettings(EngraverTraceSettings *settings, double amount, bool nvisible, int where)
{
	return nodes.push(new TraceNode(settings, amount, nvisible), 1, where);
}

int EngraverTraceStack::Move(int which, int towhere)
{
	if (which<0 || which >= nodes.n) return 1;
	if (towhere<0 || towhere >= nodes.n) return 1;
	nodes.slide(which, towhere);
	return 0;
}

/*! Stack slide
 */
int EngraverTraceStack::Remove(int which)
{
	return nodes.remove(which);
}


void EngraverTraceStack::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Laxkit::Attribute *EngraverTraceStack::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute();

	if (what==-1) {
		att->push("id", "SomeName", "id for this stack");
		Attribute *att2 = att->pushSubAtt("node", nullptr, "One or more blocks for trace settings objects plus weight");
		att2->push("visible",nullptr, "Whether to use this node");
		att2->push("amount", "1",     "How much to weight this node (usually 0..1)");
		return att;
	}

	att->push("id", Id());
	for (int c=0; c<nodes.n; c++) {
		Attribute *att2 = att->pushSubAtt("node");

		if (nodes.e[c]->settings) {
			if (nodes.e[c]->settings->ResourceOwner()!=this) {
				 //resource'd traceobject
				char str[11+strlen(nodes.e[c]->settings->Id())];
				sprintf(str,"resource: %s",nodes.e[c]->settings->Id());
				att2->push("settings",str);

			} else {
				Attribute *t=att2->pushSubAtt("settings");
				nodes.e[c]->settings->dump_out_atts(t, 0,savecontext);
			}
		}
	}

	return att;
}

void EngraverTraceStack::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{ 
	if (!att) return;

	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) anObject::Id(value);

		} else if (!strcmp(name,"node")) {

			EngraverTraceSettings *nsettings=NULL;
			double amount=0;
			bool vis=true;

			for (int c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
				name= att->attributes.e[c]->attributes.e[c2]->name;
				value=att->attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"amount")) {
					DoubleAttribute(value, &amount, NULL);

				} else if (!strcmp(name,"visible")) {
					vis = BooleanAttribute(value);

				} else if (!strcmp(name,"settings")) {

					if (value && strstr(value,"resource:")==value) {
						value+=9;
						while (isspace(*value)) value ++;
						InterfaceManager *imanager=InterfaceManager::GetDefault(true);
						ResourceManager *rm=imanager->GetResourceManager();
						EngraverTraceSettings *obj=dynamic_cast<EngraverTraceSettings*>(rm->FindResource(value,"EngraverTraceSettings"));
						if (obj) {
							nsettings=obj;
							nsettings->inc_count();
						}

					} else { //not resourced
						EngraverTraceSettings *obj=new EngraverTraceSettings;
						obj->dump_in_atts(att->attributes.e[c]->attributes.e[c2], flag,context);
						obj->SetResourceOwner(this);
						nsettings=obj;
					}
				}
			}

			if (nsettings) {
				PushSettings(nsettings, amount, vis, -1);
				nsettings->dec_count();
			}
		}
	}
}


//------------------------------------- EngraverPointGroup ------------------------

/*! \class EngraverPointGroup
 *
 * Info for groups of points in an EngraverFillData.
 *
 * Built in generator types are: linear, radial, spiral, circular
 */


/*! Default to linear. Warning: leaves trace==NULL!
 */
EngraverPointGroup::EngraverPointGroup(EngraverFillData *nowner)
{	
	owner=nowner;

	trace=NULL;
	dashes=NULL;
	direction=NULL;
	spacing=NULL;

	id=getUniqueNumber(); //the group number in LinePoint
	name=NULL;

	active=true;
	linked=false;

	position.x=position.y=.5;
	directionv.x=1; //default

	default_weight=-1;

	iorefs=NULL;

	needtotrace=false;
	needtoreline=false;
	needtodash=false;
}

/*! Creates a unique new number for id if nid<0.
 */
EngraverPointGroup::EngraverPointGroup(EngraverFillData *nowner,
										int nid,const char *nname,
										int ntype, flatpoint npos, flatpoint ndir,
										EngraverTraceSettings *newtrace,
										EngraverLineQuality   *newdash,
										EngraverDirection     *newdir,
										EngraverSpacing       *newspacing)
{
	owner=nowner;

	trace=newtrace;
	if (trace) trace->inc_count();
	else trace=new EngraverTraceSettings();
	if (trace && !trace->ResourceOwner()) trace->SetResourceOwner(this);

	dashes=newdash;
	if (dashes) dashes->inc_count();
	else dashes=new EngraverLineQuality();
	if (dashes && !dashes->ResourceOwner()) dashes->SetResourceOwner(this);

	direction=newdir;
	if (direction) direction->inc_count();
	else direction=new EngraverDirection();
	if (direction && !direction->ResourceOwner()) direction->SetResourceOwner(this);

	spacing=newspacing;
	if (spacing) spacing->inc_count();
	else spacing=new EngraverSpacing();
	if (spacing && !spacing->ResourceOwner()) spacing->SetResourceOwner(this);


	id=nid;
	if (id<0) id=getUniqueNumber(); //the group number in LinePoint
	name=newstr(nname);

	active=true;
	linked=false;

	default_weight=-1;

	if (!newdir) {
		direction->type          =ntype; //what manner of lines
		direction->position      =npos;
		direction->direction     =directionv =ndir; 
		direction->resolution    =1./3;
		direction->default_weight=default_weight =-1;
		direction->position.x    =direction->position.y =.5;
		direction->direction.x   =1;
	}

	if (!newspacing) {
		spacing->spacing =.1;
	}

	iorefs = NULL;

	needtotrace  = false;
	needtoreline = false;
	needtodash   = false;
}

EngraverPointGroup::~EngraverPointGroup()
{
	if (trace)     trace->dec_count();
	if (dashes)    dashes->dec_count();
	if (direction) direction->dec_count();
	if (spacing)   spacing  ->dec_count();
	delete grow_cache;
	delete[] name;
	delete[] iorefs;
}

Laxkit::anObject *EngraverPointGroup::ObjectOwner()
{
	return dynamic_cast<Laxkit::anObject*>(owner);
}

/*! Sets the modification time of various aspects to the current time.
 */
void EngraverPointGroup::Modified(int what)
{
	//std::time_t modtime;
	//modtime=time(NULL);
	cerr << " *** need to properly mplement EngraverPointGroup::Modified!"<<endl;

	if (owner) owner->touchContents();
}

/*! If keep_name, then do not copy id and name.
 * if link_trace or link_dash, then inc_count those objects instead of making a copy.
 */
void EngraverPointGroup::CopyFrom(EngraverPointGroup *orig, bool keep_name, bool link_trace, bool link_dash, bool link_dir, bool link_spacing)
{
	if (keep_name) {
		id=orig->id;
		makestr(name,orig->name);
	}

	active    =orig->active;
	color     =orig->color;

	position       =orig->position;
	directionv     =orig->directionv;

	 //probably needs to be more complete here...
	if (trace) { trace->dec_count(); trace=NULL; } 
	if (link_trace) { trace=orig->trace; if (trace) trace->inc_count(); }
	else if (orig->trace!=NULL) {
		trace = dynamic_cast<EngraverTraceSettings*>(orig->trace->duplicate());
		trace->SetResourceOwner(this);
	}

	if (dashes) { dashes->dec_count(); dashes=NULL; }
	if (link_dash) { dashes=orig->dashes; if (dashes) dashes->inc_count(); }
	else if (orig->dashes) {
		dashes = dynamic_cast<EngraverLineQuality*>(orig->dashes->duplicate());
		dashes->SetResourceOwner(this);
	}

	if (direction) { direction->dec_count(); direction=NULL; }
	if (link_dir) { direction=orig->direction; if (direction) direction->inc_count(); }
	else if (orig->direction) {
		direction = dynamic_cast<EngraverDirection*>(orig->direction->duplicate());
		direction->SetResourceOwner(this);
	}

	if (spacing) { spacing->dec_count(); spacing = nullptr; } 
	if (link_spacing) { spacing = orig->spacing; if (spacing) spacing->inc_count(); }
	else if (orig->spacing != nullptr) {
		spacing = dynamic_cast<EngraverSpacing*>(orig->spacing->duplicate());
		spacing->SetResourceOwner(this);
	}


	 //lastly, copy lines
	LinePoint *pp, *lp;

	for (int c=0; c<orig->lines.n; c++) {
		pp=new LinePoint();
		pp->Set(orig->lines.e[c]);
		lines.push(pp);

		lp=orig->lines.e[c]->next;		
		while (lp) {
			pp->next=new LinePoint();
			pp->next->prev=pp;
			pp->next->Set(lp);
			pp=pp->next;
			lp=lp->next;
		}
	}
}

void EngraverPointGroup::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 

	if (what==-1) {
		fprintf(f,"%sid 1             #group id number\n", spc);
		fprintf(f,"%sname Name        #some name for \n", spc);

		fprintf(f,"%sactive yes       #yes|no, whether to use this group or not\n", spc);
		fprintf(f,"%slinked yes       #yes|no, whether to warp along with other linked groups\n", spc);
		fprintf(f,"%stype linear      #or radial, circular, spiral\n", spc);
		fprintf(f,"%sposition (.5,.5) #default origin for the pattern, range 0..1, spans whole mesh\n", spc);
		fprintf(f,"%sdirectionv (1,0) #default direction for the pattern \n", spc);
		fprintf(f,"%scolor rgbaf(1.,1.,1.,1.)  #color of lines in this group\n", spc);
		fprintf(f,"%sdefault_spacing  .1 #default spacing, in object space, not s,t space \n", spc);

		fprintf(f,"%strace            #trace settings. If sharing with a previous group, use \"trace with: pgroup-name\" \n", spc);
		if (trace) trace->dump_out(f,indent+2,-1,context);
		else {
			EngraverTraceSettings t;
			t.dump_out(f,indent+2,-1,context);
		}

		fprintf(f,"%sdashes           #Dash settings\n", spc);
		if (dashes) dashes->dump_out(f,indent+2,-1,context);
		else {
			EngraverLineQuality d;
			d.dump_out(f,indent+2,-1,context);
		}

		fprintf(f,"%sdirection        #Direction settings for auto line creation\n", spc);
		if (direction) direction->dump_out(f,indent+2,-1,context);
		else {
			EngraverDirection d;
			d.dump_out(f,indent+2,-1,context);
		}

		fprintf(f,"%sspacing        #Spacing settings\n", spc);
		if (spacing) spacing->dump_out(f,indent+2,-1,context);
		else {
			EngraverSpacing d;
			d.dump_out(f,indent+2,-1,context);
		} 

		fprintf(f,"%sline \\           #One for each defined line\n",spc);
		fprintf(f,"%s  (0.5,0.75) 2 on #Any number of points, 1 point per file line. Coordinate is within mesh. Format is: (x,y) weight on|off|end|start\n",spc);
		fprintf(f,"%s  ...\n",spc);
		return;
	}

	fprintf(f,"%sid %d\n", spc, id);
	if (!isblank(name)) fprintf(f,"%sname %s\n", spc, name);
	//fprintf(f,"%strace    #trace settings.. output TODO!! \n", spc);

	fprintf(f,"%sactive %s\n",spc, active?"yes":"no");
	fprintf(f,"%slinked %s\n",spc, linked?"yes":"no");

	fprintf(f,"%sdefault_weight  %.10g\n", spc, default_weight);

	fprintf(f,"%sposition (%.10g, %.10g)\n", spc,position.x,position.y);
	fprintf(f,"%sdirectionv (%.10g, %.10g)\n", spc,directionv.x,directionv.y);


	fprintf(f,"%scolor rgbaf(%.10g,%.10g,%.10g,%.10g)\n",spc, 
			color.red/65535.,
			color.green/65535.,
			color.blue/65535.,
			color.alpha/65535.);
	
	if (trace) {
		if (trace->ResourceOwner()!=this) {
			 //resource'd trace
			fprintf(f,"%strace resource: %s\n",spc,trace->Id());

		} else { 
			fprintf(f,"%strace\n",spc);
			trace->dump_out(f,indent+2,what,context);
		}
	}

	if (dashes) {
		if (dashes->ResourceOwner()!=this) {
			 //resource'd dashes
			fprintf(f,"%sdashes resource: %s\n",spc,dashes->Id());

		} else {
			fprintf(f,"%sdashes\n",spc);
			dashes->dump_out(f,indent+2,what,context);
		}
	}

	if (direction) {
		if (direction->ResourceOwner()!=this) {
			 //resource'd direction
			fprintf(f,"%sdirection resource: %s\n",spc,direction->Id());

		} else {
			fprintf(f,"%sdirection\n",spc);
			direction->dump_out(f,indent+2,what,context);
		}
	}

	if (spacing) {
		if (spacing->ResourceOwner()!=this) {
			 //resource'd spacing
			fprintf(f,"%sspacing resource: %s\n",spc,spacing->Id());

		} else {
			fprintf(f,"%sspacing\n",spc);
			spacing->dump_out(f,indent+2,what,context);
		}
	}

	if (default_weight<=0 && direction->default_weight>0) default_weight=direction->default_weight;

	 //finally output the lines
	LinePoint *p;
	const char *ons, *dash;
	for (int c=0; c<lines.n; c++) {
		fprintf(f,"%sline \\ #%d\n",spc,c);

		p=lines.e[c];
		while (p) {
			if (p->on==ENGRAVE_Off) ons="off";
			else if (p->on==ENGRAVE_On) ons="on";
			else if (p->on==ENGRAVE_EndPoint) ons="end";
			else if (p->on==ENGRAVE_StartPoint) ons="start";
			fprintf(f,"%s  (%.10g, %.10g) %.10g %s\n",spc, p->s,p->t,p->weight, ons);

			p=p->next;
		}
		
		if (lines.e[c]->cache) {
			fprintf(f,"%slinecache \\ #%d\n",spc,c);
			LinePointCache *cc=lines.e[c]->cache;
			LinePointCache *ccstart=cc;
			const char *onsd;

			do {
				if (cc->on==ENGRAVE_Off) ons="off";
				else if (cc->on==ENGRAVE_On) ons="on";
				else if (cc->on==ENGRAVE_EndPoint) ons="end";
				else if (cc->on==ENGRAVE_StartPoint) ons="start";

				if (cc->dashon==ENGRAVE_Off) onsd="off";
				else if (cc->dashon==ENGRAVE_On) onsd="on";
				else if (cc->dashon==ENGRAVE_EndPoint) onsd="end";
				else if (cc->dashon==ENGRAVE_StartPoint) onsd="start";

				if (cc->type==ENGRAVE_Original) dash="orig";
				else if (cc->type==ENGRAVE_BlockStart ) dash="blockstart";
				else if (cc->type==ENGRAVE_BlockEnd   ) dash="blockend";
				else if (cc->type==ENGRAVE_VisualCache) dash="sample";
				else if (cc->type==ENGRAVE_EndDash    ) dash="dashend";
				else if (cc->type==ENGRAVE_StartDash  ) dash="dashstart";
				else dash="unknown";

				fprintf(f,"%s  %s %.10g (%.10g, %.10g) %.10g %s %s\n",spc, dash, cc->bt, cc->p.x,cc->p.y,cc->weight, ons, onsd);

				cc=cc->next;
			} while (cc && cc!=ccstart);
		}
	}

}

Laxkit::Attribute *EngraverPointGroup::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att = new Attribute;
	Attribute *att2;

	if (what == -1) {
		att->push("id 1             #group id number");
		att->push("name Name        #some name for ");

		att->push("active yes       #yes|no, whether to use this group or not");
		att->push("linked yes       #yes|no, whether to warp along with other linked groups");
		att->push("type linear      #or radial, circular, spiral");
		att->push("position (.5,.5) #default origin for the pattern, range 0..1, spans whole mesh");
		att->push("directionv (1,0) #default direction for the pattern ");
		att->push("color rgbaf(1.,1.,1.,1.)  #color of lines in this group");
		att->push("default_spacing  .1 #default spacing, in object space, not s,t space ");

		att2 = att->pushSubAtt("trace", nullptr, "trace settings. If sharing with a previous group, use \"trace with: pgroup-name\"");
		if (trace) trace->dump_out_atts(att2,-1,context);
		else {
			EngraverTraceSettings t;
			t.dump_out_atts(att2, -1, context);
		}
		
		att2 = att->pushSubAtt("dashes", "Dash settings");
		if (dashes) dashes->dump_out_atts(att2,-1,context);
		else {
			EngraverLineQuality d;
			d.dump_out_atts(att2,-1,context);
		}

		att2 = att->pushSubAtt("direction", nullptr, "Direction settings for auto line creation");
		if (direction) direction->dump_out_atts(att2,-1,context);
		else {
			EngraverDirection d;
			d.dump_out_atts(att2,-1,context);
		}

		att2 = att->pushSubAtt("spacing", nullptr, "Spacing settings");
		if (spacing) spacing->dump_out_atts(att2,-1,context);
		else {
			EngraverSpacing d;
			d.dump_out_atts(att2,-1,context);
		} 

		att2 = att->pushSubAtt("line", nullptr, "One for each defined line\n"
								"Any number of points, 1 point per file line. Coordinate is within mesh. Format is: (x,y) weight on|off|end|start");
		att2->value = newstr("(0.5,0.75) 2 on\n...");

		return att;
	}

	att->push("id", id);
	if (!isblank(name)) att->push("name", name);
	
	att->push("active", active ? "yes" : "no");
	att->push("linked", linked ? "yes" : "no");

	att->push("default_weight", default_weight);

	att->pushStr("position",   -1, "(%.10g, %.10g)\n", position.x,position.y);
	att->pushStr("directionv", -1, "(%.10g, %.10g)\n", directionv.x,directionv.y);


	att->pushStr("color", -1, "rgbaf(%.10g,%.10g,%.10g,%.10g)",
			color.red/65535.,
			color.green/65535.,
			color.blue/65535.,
			color.alpha/65535.);
	
	if (trace) {
		if (trace->ResourceOwner()!=this) {
			 //resource'd trace
			att->pushStr("trace", -1, "resource: %s",trace->Id());

		} else { 
			att2 = att->pushSubAtt("trace");
			trace->dump_out_atts(att2,what,context);
		}
	}

	if (dashes) {
		if (dashes->ResourceOwner()!=this) {
			 //resource'd dashes
			att->pushStr("dashes", -1, "resource: %s",dashes->Id());

		} else {
			att2 = att->pushSubAtt("dashes");
			dashes->dump_out_atts(att2,what,context);
		}
	}

	if (direction) {
		if (direction->ResourceOwner()!=this) {
			 //resource'd dashes
			att->pushStr("direction", -1, "resource: %s",direction->Id());

		} else {
			att2 = att->pushSubAtt("direction");
			direction->dump_out_atts(att2,what,context);
		}
	}

	if (spacing) {
		if (spacing->ResourceOwner()!=this) {
			 //resource'd dashes
			att->pushStr("spacing", -1, "resource: %s",spacing->Id());

		} else {
			att2 = att->pushSubAtt("spacing");
			spacing->dump_out_atts(att2,what,context);
		}
	}

	if (default_weight<=0 && direction->default_weight>0) default_weight=direction->default_weight;

	 //finally output the lines
	LinePoint *p;
	Utf8String s,s2;
	const char *ons, *dash;
	for (int c = 0; c < lines.n; c++) {
		s.Sprintf("%d", c);
		att2 = att->pushSubAtt("line", nullptr, s.c_str());

		p = lines.e[c];
		s.SetToNone();
		while (p) {
			if      (p->on == ENGRAVE_Off       ) ons = "off";
			else if (p->on == ENGRAVE_On        ) ons = "on";
			else if (p->on == ENGRAVE_EndPoint  ) ons = "end";
			else if (p->on == ENGRAVE_StartPoint) ons = "start";
			s2.Sprintf("(%.10g, %.10g) %.10g %s\n", p->s,p->t, p->weight, ons);
			s.Append(s2);

			p = p->next;
		}
		att2->value = s.ExtractBytes(nullptr, nullptr, nullptr);
		
		if (lines.e[c]->cache) {
			s.Sprintf("%d", c);
			att2 = att->pushSubAtt("linecache", nullptr, s.c_str());
			s.SetToNone();

			LinePointCache *cc = lines.e[c]->cache;
			LinePointCache *ccstart = cc;
			const char *onsd;

			do {
				if      (cc->on == ENGRAVE_Off       ) ons = "off";
				else if (cc->on == ENGRAVE_On        ) ons = "on";
				else if (cc->on == ENGRAVE_EndPoint  ) ons = "end";
				else if (cc->on == ENGRAVE_StartPoint) ons = "start";

				if      (cc->dashon == ENGRAVE_Off       ) onsd = "off";
				else if (cc->dashon == ENGRAVE_On        ) onsd = "on";
				else if (cc->dashon == ENGRAVE_EndPoint  ) onsd = "end";
				else if (cc->dashon == ENGRAVE_StartPoint) onsd = "start";

				if      (cc->type == ENGRAVE_Original   ) dash = "orig";
				else if (cc->type == ENGRAVE_BlockStart ) dash = "blockstart";
				else if (cc->type == ENGRAVE_BlockEnd   ) dash = "blockend";
				else if (cc->type == ENGRAVE_VisualCache) dash = "sample";
				else if (cc->type == ENGRAVE_EndDash    ) dash = "dashend";
				else if (cc->type == ENGRAVE_StartDash  ) dash = "dashstart";
				else dash = "unknown";

				s2.Sprintf("%s %.10g (%.10g, %.10g) %.10g %s %s\n", dash, cc->bt, cc->p.x,cc->p.y,cc->weight, ons, onsd);
				s.Append(s2);

				cc = cc->next;
			} while (cc && cc != ccstart);
			att2->value = s.ExtractBytes(nullptr, nullptr, nullptr);
		}
	}

	return att;
}

void EngraverPointGroup::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *name,*value;
	int c;

	const char *tracekey=NULL;
	const char *dashkey=NULL;
	int type=PGROUP_Unknown;
	double resolution;
	double default_spacing=-1;

	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			IntAttribute(value, &id, NULL);

		} else if (!strcmp(name,"name")) {
			delete[] this->name;
			this->name=newstr(value);

		} else if (!strcmp(name,"active")) {
			active=BooleanAttribute(value);

		} else if (!strcmp(name,"linked")) {
			linked=BooleanAttribute(value);

		} else if (!strcmp(name,"needtotrace")) {
			needtotrace=BooleanAttribute(value);

		} else if (!strcmp(name,"needtoreline")) {
			needtoreline=BooleanAttribute(value);

		} else if (!strcmp(name,"type")) {
			 //deprecated as of after Laidout 0.095!!
			if (!strcasecmp(value,"linear")) type=PGROUP_Linear;
			else if (!strcasecmp(value,"radial")) type=PGROUP_Radial;
			else if (!strcasecmp(value,"spiral")) type=PGROUP_Spiral;
			else if (!strcasecmp(value,"circular")) type=PGROUP_Circular;

		} else if (!strcmp(name,"position")) {
			FlatvectorAttribute(value,&position);

		} else if (!strcmp(name,"directionv")) {
			FlatvectorAttribute(value,&directionv);

		} else if (!strcmp(name,"color")) {
			SimpleColorAttribute(value, NULL, &color, NULL);

		} else if (!strcmp(name,"resolution")) {
			 //deprecated as of after Laidout 0.095!!
			DoubleAttribute(value,&resolution, NULL);

		} else if (!strcmp(name,"default_spacing")) {
			DoubleAttribute(value,&default_spacing, NULL);

		} else if (!strcmp(name,"default_weight")) {
			DoubleAttribute(value,&default_weight, NULL);

		} else if (!strcmp(name,"dashes")) {
			if (value && strstr(value,"resource:")==value) {
				value+=9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager=InterfaceManager::GetDefault(true);
				ResourceManager *rm=imanager->GetResourceManager();
				EngraverLineQuality *ndash=dynamic_cast<EngraverLineQuality*>(rm->FindResource(value,"EngraverLineQuality"));

				if (ndash) {
					if (dashes) dashes->dec_count();
					dashes=ndash;
					dashes->inc_count();
				} else {
					DBG cerr << " *** Warning! Missing dashes resource "<<value<<"!"<<endl;
				}

			} else if (value && strstr(value,"with:")==value) {
				dashkey=value+5;
				while (isspace(*dashkey)) dashkey++;
			}

			//if (dashes) dashes->dec_count();
			if (!dashes) {
				dashes=new EngraverLineQuality();
				dashes->SetResourceOwner(this);
			}
			dashes->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"trace")) {
			if (value && strstr(value,"resource:")==value) {
				value+=9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager=InterfaceManager::GetDefault(true);
				ResourceManager *rm=imanager->GetResourceManager();
				EngraverTraceSettings *ntrace=dynamic_cast<EngraverTraceSettings*>(rm->FindResource(value,"EngraverTraceSettings"));

				if (ntrace) {
					if (trace) trace->dec_count();
					trace=ntrace;
					trace->inc_count();
				} else {
					DBG cerr << " *** Warning! Missing trace resource "<<value<<"!"<<endl;
				}

			} else if (value && strstr(value,"with:")==value) {
				tracekey=value+5;
				while (isspace(*tracekey)) tracekey++;
			}

			//if (trace) trace->dec_count();
			if (!trace) {
				trace=new EngraverTraceSettings();
				trace->SetResourceOwner(this);
			}
			trace->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"direction")) {
			if (value && *value=='(') {
				 //backwards compatible with Laidout 0.095
				FlatvectorAttribute(value,&directionv);

			} else if (value && strstr(value,"resource:")==value) {
				value+=9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager=InterfaceManager::GetDefault(true);
				ResourceManager *rm=imanager->GetResourceManager();
				EngraverDirection *ndirection=dynamic_cast<EngraverDirection*>(rm->FindResource(value,"EngraverDirection"));

				if (ndirection) {
					if (direction) direction->dec_count();
					direction=ndirection;
					direction->inc_count();
				} else {
					DBG cerr << " *** Warning! Missing direction resource "<<value<<"!"<<endl;
				}

			}

			if (!direction) {
				direction=new EngraverDirection();
				direction->SetResourceOwner(this);
			}
			direction->dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"spacing")) {
			if (value && (isdigit(*value) || *value=='.')) {
				 //for backwards compatibility with Laidout 0.095
				DoubleAttribute(value, &default_spacing); 
			}

			if (value && strstr(value,"resource:")==value) {
				value+=9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager=InterfaceManager::GetDefault(true);
				ResourceManager *rm=imanager->GetResourceManager();
				EngraverSpacing *nspacing=dynamic_cast<EngraverSpacing*>(rm->FindResource(value,"EngraverSpacing"));

				if (nspacing) {
					if (spacing) spacing->dec_count();
					spacing=nspacing;
					spacing->inc_count();
				} else {
					DBG cerr << " *** Warning! Missing spacing resource "<<value<<"!"<<endl;
				}

			}

			if (!spacing) {
				spacing=new EngraverSpacing();
				spacing->SetResourceOwner(this);
			}
			spacing->dump_in_atts(att->attributes.e[c],flag,context);


		} else if (!strcmp(name,"line")) {
			char *end_ptr=NULL;
			flatpoint v;
			int status;
			double w;
			int on=ENGRAVE_On;
			LinePoint *lstart=NULL, *ll=NULL;

			do { //one iteration for each point
				 //each line has: (x,y) weight on|off groupid

				while (isspace(*value)) value++;
				if (!strncmp(value,"cache",5)) {
					 //ignore cache values, they are regenerated
					value=strchr(value,'\n');
					if (!value) break;
					continue;
				}

				 //get (s,t) point
				status=FlatvectorAttribute(value, &v, &end_ptr);
				if (status==0) break;

				 //get weight
				value=end_ptr;
				status=DoubleAttribute(value, &w, &end_ptr);
				if (status==0) break;

				value=end_ptr;
				while (isspace(*value)) value++;
				if (*value=='o' && value[1]=='n') { on=ENGRAVE_On; value+=2; }
				else if (*value=='o' && value[1]=='f' && value[2]=='f') { on=ENGRAVE_Off; value+=3; }
				else if (*value=='e' && value[1]=='n' && value[2]=='d') { on=ENGRAVE_EndPoint; value+=3; }
				else if (*value=='s' && value[1]=='t' && value[2]=='a' && value[2]=='r' && value[2]=='t') { on=ENGRAVE_StartPoint; value+=5; }

				 //skip everything until the next line
				while (*value!='\0' && *value!='\n') value++;
				if (*value=='\n') value++;

				if (!lstart) { lstart=ll=new LinePoint(v.x,v.y, w); ll->on=on; }
				else {
					ll->next=new LinePoint(v.x,v.y, w);
					ll->next->prev=ll;
					ll->next->on=on;
					ll=ll->next;
				}

				while (isspace(*value)) value++;

			} while (*value!='\0');

			if (lstart) lines.push(lstart);
		}
	}

	if (!trace)     { trace    =new EngraverTraceSettings(); trace->SetResourceOwner(this); }
	if (!dashes)    { dashes   =new EngraverLineQuality();  dashes->SetResourceOwner(this); }
	if (!direction) { direction=new EngraverDirection(); direction->SetResourceOwner(this); }
	if (!spacing)   { spacing  =new EngraverSpacing();     spacing->SetResourceOwner(this); }

	if (spacing->spacing<=0 && default_spacing>0) spacing->spacing=default_spacing;
	if (direction->resolution<=0 && resolution>0) direction->resolution=resolution;
	if (type!=PGROUP_Unknown) direction->SetType(type);

	if (isblank(this->name)) makestr(this->name,"Group");

	 //this stuff is deprecated!!! kept only to read in Laidout 0.095 files
	delete[] iorefs;
	iorefs=NULL;
	if (tracekey) {
		appendstr(iorefs,"|trace:");
		appendstr(iorefs,tracekey);
	}
	if (dashkey) {
		appendstr(iorefs,"|dash:");
		appendstr(iorefs,dashkey);
	}
}


/*! Return 1 if a point is considered on by the criteria of the settings in dashes, else 0 if confirmed off.
 * Default is point must be on, and weight>=zero_threshhold.
 */
int EngraverPointGroup::PointOn(LinePoint *p)
{
	if (!p) return 0;
	if (p->on==ENGRAVE_Off) return 0;
	if (dashes && p->weight < dashes->zero_threshhold) return 0;

	if (p->on==ENGRAVE_EndPoint) return ENGRAVE_EndPoint;
	if (p->on==ENGRAVE_StartPoint) return ENGRAVE_StartPoint;
	return ENGRAVE_On;
}

/*! Return nonzero if a point is considered on by the criteria of the settings in dashes, else 0 if confirmed off.
 * Default is point must be dashon, and weight>=zero_threshhold.
 */
int EngraverPointGroup::PointOnDash(LinePointCache *p)
{
	if (!p) return 0;
	if (p->dashon==ENGRAVE_Off || p->on==ENGRAVE_Off) return 0;
	if (dashes && p->weight < dashes->zero_threshhold) return 0;

	if (p->dashon==ENGRAVE_EndPoint)   return ENGRAVE_EndPoint;
	if (p->dashon==ENGRAVE_StartPoint) return ENGRAVE_StartPoint;
	return ENGRAVE_On;
}

/*! Return nonzero if a point is considered on by the criteria of the settings in dashes, else 0 if confirmed off.
 * Default is point must be dashon, and weight>=zero_threshhold.
 */
int EngraverPointGroup::CachePointOn(LinePointCache *p)
{
	if (!p) return 0;
	if (p->dashon==ENGRAVE_Off || p->on==ENGRAVE_Off) return 0;
	if (dashes && p->weight < dashes->zero_threshhold) return 0;

	if (p->dashon==ENGRAVE_EndPoint)   return ENGRAVE_EndPoint;
	if (p->dashon==ENGRAVE_StartPoint) return ENGRAVE_StartPoint;
	return ENGRAVE_On;
}

void EngraverPointGroup::StripDashes()
{
	LinePointCache *cache, *start, *cc;

	for (int c=0; c<lines.n; c++) {
		start=cache=lines.e[c]->cache;

		if (!cache) continue;

		do {
			while (cache->next && (cache->next->type==ENGRAVE_EndDash || cache->next->type==ENGRAVE_StartDash)) {
				cc=cache->next;
				cc->Detach();
				delete cc;
			}

			if (cache->original) {
				cache->weight=cache->original->weight;
				cache->on=cache->original->on;
			}
			cache=cache->next;
		} while (cache && cache!=start);
	}
}

void render_bez_line_recurse(unsigned char *img, int mapwidth, int mapheight, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2, double t)
{
	flatpoint nc1, npp, npm, npn, nc2;
	bez_subdivide_decasteljau(p1,c1,c2,p2, nc1, npp, npm, npn, nc2);

	 //render 2x2 pixel npm:
	int x=npm.x, y=npm.y;
	if (x>=0 && x<mapwidth-1 && y>=0 && y<=mapheight-1) {
		int i=4*(y*mapwidth+x);
		img[i]= img[i+4]= img[i+4*mapwidth]= img[i+4*(mapwidth+1)]=0;
		img[i+1]= img[i+1+4]= img[i+1+4*mapwidth]= img[i+1+4*(mapwidth+1)]=0;
		img[i+2]= img[i+2+4]= img[i+2+4*mapwidth]= img[i+2+4*(mapwidth+1)]=0;
	}

	t/=2;
	if (fabs(npm.x-p1.x)+fabs(npm.y-p1.y)>1) render_bez_line_recurse(img,mapwidth,mapheight, p1,nc1,npp,npm, t);
	if (fabs(npm.x-p2.x)+fabs(npm.y-p2.y)>1) render_bez_line_recurse(img,mapwidth,mapheight, npm,npn,nc2,p2, t);
}

void render_bez_line(unsigned char *img, int mapwidth, int mapheight, flatpoint p1,flatpoint c1,flatpoint c2,flatpoint p2)
{
	 //render 2x2 pixel p1:
	int x=p1.x, y=p1.y;
	if (x>=0 && x<mapwidth-1 && y>=0 && y<=mapheight-1) {
		int i=4*(y*mapwidth+x);
		img[i]= img[i+4]= img[i+4*mapwidth]= img[i+4*(mapwidth+1)]=0;//b
		img[i+1]= img[i+1+4]= img[i+1+4*mapwidth]= img[i+1+4*(mapwidth+1)]=0;//g
		img[i+2]= img[i+2+4]= img[i+2+4*mapwidth]= img[i+2+4*(mapwidth+1)]=0;//r
	}

	 //render 2x2 pixel p2:
	x=p2.x, y=p2.y;
	if (x>=0 && x<mapwidth-1 && y>=0 && y<=mapheight-1) {
		int i=4*(y*mapwidth+x);
		img[i]= img[i+4]= img[i+4*mapwidth]= img[i+4*(mapwidth+1)]=0;
		img[i+1]= img[i+1+4]= img[i+1+4*mapwidth]= img[i+1+4*(mapwidth+1)]=0;
		img[i+2]= img[i+2+4]= img[i+2+4*mapwidth]= img[i+2+4*(mapwidth+1)]=0;
	}

	render_bez_line_recurse(img,mapwidth,mapheight, p1,c1,c2,p2, 1);
}


ImageData *EngraverPointGroup::SpacingSnapshot()
{
	// render base lines with minimal thickness on a pixmap
	// walk from each point tangent to its line, until intersecting adjacent lines
	// average thicknesses found? or just use one for speed?
	//
	// now each point has unique spacing map, render a value snapshot based on those spacing values


	UpdateBezCache();

	Affine aa(owner->GetTransformToContext(false,0));
	DoubleBBox bbox;
	bbox.addtobounds(aa.m(), owner);

	int mapwidth, mapheight;
	//int blur=0;
	int resolution=100;
	double aspect=(bbox.maxx-bbox.minx)/(bbox.maxy-bbox.miny);
	if (aspect>1) { mapwidth=(bbox.maxx-bbox.minx)/spacing->spacing*resolution; mapheight=mapwidth/aspect; }
	else { mapheight=(bbox.maxy-bbox.miny)/spacing->spacing*resolution; mapwidth=mapheight*aspect; }

	if (mapwidth<=0 || mapheight<=0) {
		if (aspect>1) { mapwidth=100; mapheight=mapwidth/aspect; }
		else { mapheight=100; mapwidth=mapheight*aspect; }
	}

	//Affine abounds(mapwidth/(bbox.maxx-bbox.minx), 0,0, mapheight/(bbox.maxy-bbox.miny), -bbox.minx,-bbox.miny);
	//Affine abounds(mapwidth/(bbox.maxx-bbox.minx), 0,0, mapheight/(bbox.maxy-bbox.miny), bbox.minx,bbox.miny);
	//aa.Multiply(abounds);

	Affine abounds;
	abounds.m(1, 0,0, 1, -bbox.minx,-bbox.miny);
	aa.Multiply(abounds);
	abounds.m(mapwidth/(bbox.maxx-bbox.minx), 0,0, mapheight/(bbox.maxy-bbox.miny), 0,0);
	//abounds.m(100, 0,0, 100, 0,0);
	aa.Multiply(abounds);

	LinePoint *l,*lstart;
	flatpoint p1,c1,c2,p2;

	LaxImage *image = ImageLoader::NewImage(mapwidth,mapheight);
	unsigned char *data=image->getImageBuffer();
	memset(data, 255, mapwidth*mapheight*4);

	 //create bitmap rendering of base lines
	for (int c=0; c<lines.n; c++) {
		l=lstart=lines.e[c];
		p1=aa.transformPoint(l->p);

		do {
			c1=aa.transformPoint(l->bez_after);
			if (l->next) {
				c2=aa.transformPoint(l->next->bez_before);
				p2=aa.transformPoint(l->next->p);
			
				render_bez_line(data, mapwidth,mapheight, p1,c1,c2,p2);
			}

			l=l->next;
			p1=p2;

		} while (l && l!=lstart);
	}

	 //for each point, walk perpendicular to it to find distance to next line
	// ***
	


	image->doneWithBuffer(data);

	 //create and return new ImageData mapped to correspond to edata bounding box
	ImageData *idata=new ImageData;
	idata->SetImage(image, NULL);
	image->dec_count();

	idata->AlignAndFit(NULL,&bbox,50,50,2);

	return idata;
}

/*! Take a kind of blurred snapshot of current line arrangement, and install that as the traceobject.
 */
int EngraverPointGroup::TraceFromSnapshot()
{
	ImageData *idata=CreateFromSnapshot();
	if (!idata) return 1;
	trace->Install(TraceObject::TRACE_Snapshot, idata);
	idata->dec_count();
	return 0;
}

ImageData *EngraverPointGroup::CreateFromSnapshot()
{
	NumStack<flatpoint> points;
	LinePoint *l,*lstart;
	Affine aa(owner->GetTransformToContext(false,0));
	for (int c=0; c<lines.n; c++) {
		l=lstart=lines.e[c];
		do {
			points.push(aa.transformPoint(l->p));
			points.e[points.n-1].info=255-l->weight/spacing->spacing*255;
			l=l->next;
		} while (l && l!=lstart);
	}
	DoubleBBox bbox;
	bbox.addtobounds(aa.m(), owner);

	ImageProcessor *proc=ImageProcessor::GetDefault(true);
	int mapwidth, mapheight;
	int blur=0;
	double aspect=(bbox.maxx-bbox.minx)/(bbox.maxy-bbox.miny);
	if (aspect>1) { mapwidth=(bbox.maxx-bbox.minx)/spacing->spacing*5; mapheight=mapwidth/aspect; }
	else { mapheight=(bbox.maxy-bbox.miny)/spacing->spacing*5; mapwidth=mapheight*aspect; }
	if (mapwidth<=0 || mapheight<=0) {
		if (aspect>1) { mapwidth=100; mapheight=mapwidth/aspect; }
		else { mapheight=100; mapwidth=mapheight*aspect; }
		blur=5;
	} else {
		//blur=mapwidth/5*.5;
		blur=3;
	}

	unsigned char img[mapwidth*mapheight];
	//proc->MakeValueMap(img, mapwidth,mapheight, 0, bbox, points.e,points.n);
	proc->MakeValueMap(img, mapwidth,mapheight, blur, bbox, points.e,points.n, true);

	 //convert img to actual image and install 
	LaxImage *image = ImageLoader::NewImage(mapwidth,mapheight);
	unsigned char *data=image->getImageBuffer();
	int ii=0,i=0;
	for (int y=0; y<mapheight; y++) {
		for (int x=0; x<mapwidth; x++) {
			//DBG cerr <<"i,ii:"<<i<<','<<ii<<" ";
			data[i]=data[i+1]=data[i+2]=img[ii];
			data[i+3]=255;
			i+=4;
			ii++;
		}
	}
	//DBG cerr <<endl;
	image->doneWithBuffer(data);

	DBG image->Save("snapshot.png", "png");

	 //create and return new ImageData mapped to correspond to edata bounding box
	ImageData *idata=new ImageData;
	idata->SetImage(image, NULL);
	image->dec_count();

	idata->AlignAndFit(NULL,&bbox,50,50,2);

	return idata;
}

/*! This will trace regardless of active or continuous_trace.
 * aa is a transform from object coordinates to the same coords that traceobject sits in.
 * For instance, this is usually (parent engraver object)->GetTransformToContext(false, 0).
 *
 * Returns 0 for success, or nonzero for error.
 */
int EngraverPointGroup::Trace(Affine *aa)
{
	if (!trace->traceobject) return 1;


	 //update cache if necessary
	if (!trace->traceobject->trace_sample_cache || trace->traceobject->NeedsUpdating())
		trace->traceobject->UpdateCache();

	int samplew=trace->traceobject->samplew;
	int sampleh=trace->traceobject->sampleh;

	int x,y, i;
	int sample, samplea;
	double me[6],mti[6];
	unsigned char *rgb;
	flatpoint pp;
	double a;

	SomeData *to=trace->traceobject->object;
	if (to) {
		transform_invert(mti,to->m());
		if (aa) transform_mult(me, aa->m(),mti);
		else transform_copy(me,mti);
	} else transform_identity(me);


	for (int c=0; c<lines.n; c++) {
		LinePoint *l=lines.e[c];

		while (l) {
			pp=transform_point(me,l->p);
			//pp=l->p;
			//pp=transform_point(mti,pp);

			if (to) {
				x=samplew*(pp.x-to->minx)/(to->maxx-to->minx);
				y=sampleh*(pp.y-to->miny)/(to->maxy-to->miny);

				if (x>=0 && x<samplew && y>=0 && y<sampleh) {
					i=4*(x+(sampleh-y)*samplew);
					rgb=trace->traceobject->trace_sample_cache+i;

					samplea=rgb[3];
					sample=0.3*rgb[0] + 0.59*rgb[1] + 0.11*rgb[2];
					if (sample>255) {
						sample=255;
					}

					a=(255-sample)/255.;
					a=trace->value_to_weight->f(a);
					l->weight=spacing->spacing*a; // *** this seems off
					l->on = samplea>0 ? ENGRAVE_On : ENGRAVE_Off;
				} else {
					l->weight=0;
					l->on=ENGRAVE_Off;
				}

			} else { //use current
				a=spacing->spacing * trace->value_to_weight->f(l->weight_orig/spacing->spacing);
				l->weight=a;
			}

			l=l->next;
		}
	} //each line

	UpdateDashCache();
	return 0;
}

/*! Call UpdateBezCache(), then update any LinePointCache that run along the actual lines.
 * 
 * This does NOT recreate cache points. Use UpdateDashCache() for that.
 * Does NOT update on/off state, which is also done in UpdateDashCache().
 */
void EngraverPointGroup::UpdatePositionCache()
{
	DBG cerr <<" *** EngraverPointGroup::UpdatePositionCache() needs to be optimized"<<endl;
	
	UpdateBezCache();
	
	LinePointCache *cache, *start;
	LinePoint *l;

	for (int c=0; c<lines.n; c++) {
		start=cache=lines.e[c]->cache;

		if (!cache) continue;
		l=cache->original;

		do {
			if (cache->original) {
				l=cache->original;
				cache->p=l->p;
			} else {
				if (l->next) cache->p=bez_point(cache->bt, l->p, l->bez_after, l->next->bez_before, l->next->p);
			}

			cache=cache->next;
		} while (cache && cache!=start);
	}
}

/*! Update the bez handles and bez length of all points.
 * This dose NOT create, install, or update any LinePointCache.
 */
void EngraverPointGroup::UpdateBezCache()
{
	LinePoint *p, *start;

	for (int c=0; c<lines.n; c++) {
		start=p=lines.e[c];

		if (!p) continue;

		do {
			p->UpdateBezHandles();
			if (p->prev) p->prev->length=bez_segment_length(p->prev->p, p->prev->bez_after, p->bez_before,p->p, 8);
			else p->length=0;
			if (!p->next) p->length=0;

			p=p->next;
		} while (p && p!=start);
	}
}

/*! Update (or create) any additional points added to the lines.
 *
 * Returns number of dashes.
 */
int EngraverPointGroup::UpdateDashCache()
{
	DBG cerr <<"UpdateDashCache..."<<endl;

	int numdashes=0;
	if (!dashes || (dashes && dashes->zero_threshhold==0 && dashes->broken_threshhold<=dashes->zero_threshhold)) {
		int hascache=0;
		for (int c=0; c<lines.n; c++) {
			if (lines.e[c]->cache) hascache=1;
			else lines.e[c]->BaselineCache();
		}
		if (hascache) StripDashes();
		return 1;
	}


	LinePointCache *lc=NULL, *lcstart;
	LinePoint *l, *lnext, *lstart; 

	double broken =dashes->broken_threshhold;
	double zero   =dashes->zero_threshhold;
	double dashlen=dashes->dash_length*spacing->spacing; 

	if (broken<zero) broken=zero;

	double next[4];
	int nexton[4];
	int laston; //should always be either ENGRAVE_On or ENGRAVE_Off
	int nextmax;
	double lasts;
	double t, endt;
	double s;
	double weight,dashweight, ww;
	double maxs, maxt;
	int indash=0; //0=no, 1=on initial dash, 2=on gap, 3=on final dash
	int hastoend=0;

	if (dashes->randomseed>0) srandom(dashes->randomseed);

	PtrStack<LinePointCache> unused;
	unused.Allocate(10);

	for (int c=0; c<lines.n; c++) {
		lstart=l=lines.e[c];
		if (!l->cache) l->BaselineCache();
		lc=NULL;
		lcstart=NULL;
		t=endt=-1;
		s=-1;
		indash=0;
		laston=ENGRAVE_Off;
		lasts=0;

		 //special treatment when starting off line
		l->cache->dashon=l->cache->on=ENGRAVE_Off;
		laston=ENGRAVE_Off;

		do { //once per l
			lc=l->cache;
			lc->dashon=lc->on=laston;

			if (!l->next) break;
			lnext=l->next;


			 //remove old dash points for this segment
			lcstart=lc->next;
			while (lcstart && lcstart->type!=ENGRAVE_Original) {
				if (lcstart->type==ENGRAVE_EndDash || lcstart->type==ENGRAVE_StartDash) {
					unused.push(lcstart, 1); //1 means remove and flush will delete lcstart
					lcstart=lcstart->Detach();
				}
				lcstart=lcstart->next;
			}


			if (!indash) {
				if (lc->dashon==ENGRAVE_EndPoint || lc->dashon==ENGRAVE_Off)
					 laston=ENGRAVE_Off;
				else laston=ENGRAVE_On; 

				if (l->weight >= broken && lnext->weight >= broken) {
					 //this point and next are too thick for dashes, so skip. We should not be 
					 //indash if this is true
					lc->dashon=lc->on=ENGRAVE_On;
					l->cache->weight=l->weight;
					l=lnext;
					laston=ENGRAVE_On;
					lasts=0;
					// *** need to blot any other cache points to have the proper on
					continue;
				}

				if (l->weight <= zero && lnext->weight <= zero) {
					 //this point and next are too thin for anything, so skip
					lc->dashon=ENGRAVE_Off;
					l->cache->weight=l->weight;
					l=lnext;
					laston=ENGRAVE_Off;
					lasts=0;
					// *** need to blot any other cache points to have the proper on
					continue;
				}


				 //find starting point and metrics of dash. We need to have a weight value for the dash
				 //that ideally is somewhere between but not on zero or broken.
	
				if (l->weight <= broken) {
					 //starts thin. maybe gets thicker, maybe gets thinner
					 //dash will start right on l
					 
					if (l->weight <= zero) { //lnext->weight must be >zero here
						 //starts below zero, must start dash at zero threshhold
						t=(zero - l->weight)/(lnext->weight - l->weight);
						s=t*l->length;
						weight=(zero+lnext->weight)/2; //pick a weight that will produce some kind of dash
						if (weight>broken) weight=broken-.1*(broken-zero);

					} else {
						 //weight starts between zero and broken,
						 //next weight might be anything
						t=0;
						s=0;
						ww=(lnext->weight>broken ? broken : (lnext->weight<zero ? zero : lnext->weight));
						weight=(l->weight+ww)/2;
					}

				} else {
					 //current point is > broken, but next point is < broken, so actual threshhold is
					 //somewhere within l..lnext
					t=(l->weight - broken)/(l->weight - lnext->weight);
					s=t*l->length;
					weight=(broken+lnext->weight)/2;
					if (weight<zero) weight=zero+.05*(broken-zero); //arbitrarily pick a non-zero weight
				}


				 //now we figure out the length, gap placement, and width of a dash.
			     //taper means dash width varies from broken to zero+taper*(broken-zero)
				 //original width varies from broken down to zero
				EstablishDashMetrics(s,weight, next,nexton,nextmax, l,lc, laston,lasts, dashweight, dashlen, unused);

				indash=1; 
				numdashes++; 

			} //if !indash



			if (indash) {
				 //figure out if the dash has to end before the next point from the line
				 //being either too thick or too thin

				if (lnext->weight >= broken) {
					 //need to determine absolute end of dash creation when line gets too thick...
					maxt=(broken - l->weight)/(lnext->weight - l->weight);
					maxs=maxt*l->length;
					hastoend=1;

				} else if (lnext->weight <= zero) {
					 //need to determine absolute end of dash creation when line gets too thin...
					maxt=(l->weight - zero)/(l->weight - lnext->weight);
					maxs=maxt*l->length;
					hastoend=-1;

				} else if (!lnext->next) {
					 //end of the line! don't go beyond
					maxt=.9999;
					maxs=.9999*l->length;
					hastoend=-1;

				} else {
					maxs=-1;
					maxt=-1;
					hastoend=0;
				}

			}

			while (indash) {
				if (hastoend && maxs<=next[indash]) {
					 //line either gets too thick or too thin before dash ends

					if (hastoend<0) {
						 //line gets too thin, need to insert an end point maybe
						if (nexton[indash]==ENGRAVE_EndPoint) {
							lc=AddPoint(maxs, ENGRAVE_EndPoint, ENGRAVE_EndDash, dashweight,dashlen, l,lc, unused);
							laston=ENGRAVE_Off;
							lasts=maxs;

						} //else already off, nothing required!

					} else { //else line gets too thick, needs to become on...
						if (nexton[indash]==ENGRAVE_StartPoint) {
							lc=AddPoint(maxs, ENGRAVE_StartPoint, ENGRAVE_StartDash, dashweight,dashlen, l,lc, unused);
							laston=ENGRAVE_On;
							lasts=maxs;

						} //else already on, nothing required!
					}

					indash=0;
					break;

				} else { //does not havetoend
					 //dash continues, but maybe need to start a new dash/gap span...
		
					if (next[indash] < l->length) {
						 //dash part ends within segment, so must add a change point
						if (indash!=nextmax-1) {
							lc=AddPoint(next[indash], nexton[indash], -1, dashweight,dashlen, l,lc, unused);
							if (nexton[indash]==ENGRAVE_StartPoint) laston=ENGRAVE_On;
							else laston=ENGRAVE_Off;
						}

						indash++;
						if (indash==nextmax) { 
							 //reset dash metrics to start at next[indash-1]
							EstablishDashMetrics(next[nextmax-1],-1, next,nexton,nextmax,  l,lc, laston,lasts, dashweight, dashlen,unused);
							indash=1;
							numdashes++;
						}

					} else {
						 //current dash part ends after current segment
						 //need to advance l, need to break from this loop
						//gaplen and dashweight updated if necessary below, outside of this dash loop
						break;
					} 

				} //if dash didn't have to end
			} //while indash



			 //lastly, advance to next segment...
			l=l->next;
			lasts-=l->prev->length;

			if (indash) {
				 //we need to make sure all the stuff in next[] is still current for this next segment...
				for (int c=indash; c<nextmax; c++) next[c]-=l->prev->length;

				//now need to update dashweight, etc

				if (l->weight!=l->prev->weight) {
					 //need to remap dashlen, gaplen, dashonlen, dashweight
					dashes->GetNewWeight(l->weight, &dashweight);
					//dash metrics are reevaluated at each dash boundary, so just update weight here
				}

				l->cache->weight=dashweight;

			} else { //outside of dash, just update cache weight
				l->cache->weight=l->weight;
			}


		} while (l && l!=lstart);

		ApplyBlockout(lines.e[c]);
	} //foreach line

	DBG cerr <<"end UpdateDashCache: "<<numdashes<<endl;
	return numdashes;
}

/*! Install a proper LinePointCache, with all fields set to proper values.
 *
 * If type==-1, the make type be ENGRAVE_EndDash or ENGRAVE_StartDash, according to if on==ENGRAVE_EndPoint or ENGRAVE_StartPoint.
 */
LinePointCache *EngraverPointGroup::AddPoint(double s, int on, int type, double dashweight,double dashlen,
									LinePoint *l,LinePointCache *&lc, Laxkit::PtrStack<LinePointCache> &unused)
{
	LinePointCache *lcc;
	if (unused.n) {
		lcc=unused.pop();
	} else lcc=new LinePointCache(0);

	if (type==-1) {
		if (on==ENGRAVE_EndPoint) type=ENGRAVE_EndDash;
		else if (on==ENGRAVE_StartPoint) type=ENGRAVE_StartDash;
	}

	lcc->type=type;

	lcc->bt=s/l->length;
	lcc->p=bez_point(lcc->bt, l->p, l->bez_after, l->next->bez_before, l->next->p);
	lcc->weight=dashweight;
	lcc->dashon=on;
	lcc->on    =on;

	return lc->InsertAfter(lcc);
}

/*! Will insert a LinePointCache after lc if mandated by conflict with laston.
 *
 * \todo implement lasts so as to keep minimum dash lengths intact.
 */
void EngraverPointGroup::EstablishDashMetrics(double s,double weight, double *next,int *nexton,int &nextmax,
								LinePoint *l,LinePointCache *&lc, int &laston, double &lasts,
								double &dashweight, double &dashlen, Laxkit::PtrStack<LinePointCache> &unused)
{
	if (weight<0) weight=l->weight + s/l->length * (l->next->weight - l->weight);

	 //now we figure out the length, gap placement, and width of a dash.
     //taper means dash width varies from broken to zero+taper*(broken-zero)
	 //original width varies from broken down to zero
	//double a=(weight-zero)/(broken-zero); //0..1, how long between zero and broken
	//dashweight = broken*a + (taper*(broken-zero)+zero)*(1-a);

	double a=(weight - dashes->zero_threshhold)/(dashes->broken_threshhold - dashes->zero_threshhold); //0..1, how long between zero and broken
	dashweight   = dashes->broken_threshhold*a + (dashes->dash_taper*(dashes->broken_threshhold - dashes->zero_threshhold)+dashes->zero_threshhold)*(1-a);

	double dashonlen = dashlen * (dashes->dash_density + (1 - dashes->dash_density)*a);
	double gaplen    = dashlen-dashonlen;
	double gapstart  = dashlen/2 + dashonlen/2 + dashlen * (dashes->dash_randomness*random()/RAND_MAX);
	while (gapstart>=dashlen) gapstart-=dashlen;


	if (gapstart==0) {
		 // ---***  gap starts at beginning
		next[0]=s;
		nexton[0]=ENGRAVE_EndPoint;
		next[1]=s+gaplen;
		nexton[1]=ENGRAVE_StartPoint;
		next[2]=s+dashlen;
		nexton[2]=ENGRAVE_EndPoint;
		nextmax=3; 

	} else if (gapstart+gaplen==dashlen) {
		 // ***---  whole gap fills final portion
		next[0]=s;
		nexton[0]=ENGRAVE_StartPoint;
		next[1]=s+gapstart;
		nexton[1]=ENGRAVE_EndPoint;
		next[2]=s+dashlen;
		nexton[2]=ENGRAVE_StartPoint;
		nextmax=3; 

	} else if (gapstart+gaplen>dashlen) {
		 // --****---  gap runs past end of dash, so need to wrap around
		next[0]=s;
		nexton[0]=ENGRAVE_EndPoint;
		next[1]=s+gapstart+gaplen-dashlen;
		nexton[1]=ENGRAVE_StartPoint;
		next[2]=next[1]+dashonlen;
		nexton[2]=ENGRAVE_EndPoint;
		next[3]=s+dashlen;
		nexton[3]=ENGRAVE_StartPoint;
		nextmax=4; 

	} else {
		 // **----***  gap within dashlen, so solid portion wraps around
		next[0]=s;
		nexton[0]=ENGRAVE_StartPoint;
		next[1]=s+gapstart;
		nexton[1]=ENGRAVE_EndPoint;
		next[2]=s+gapstart+gaplen;
		nexton[2]=ENGRAVE_StartPoint;
		next[3]=s+dashlen;
		nexton[3]=ENGRAVE_EndPoint;
		nextmax=4; 
	}

	 //add initial dash point if necessary
	if (   (laston==ENGRAVE_Off && nexton[0]==ENGRAVE_StartPoint)
	    || (laston==ENGRAVE_On  && nexton[0]==ENGRAVE_EndPoint)
	   ) {

		lc=AddPoint(next[0], nexton[0], -1, dashweight,dashlen, l,lc, unused);
		laston = (nexton[0]==ENGRAVE_EndPoint ? ENGRAVE_Off : ENGRAVE_On);
		lasts  = next[0];
	}
}

/*! l must have cache and dashes already properly installed and processed.
 */
int EngraverPointGroup::ApplyBlockout(LinePoint *l)
{
	LinePoint *lstart=l;
	LinePointCache *lc;

	//lcstart=l->cache;
	//int laston=l->on;

	do {
		if (l->on==ENGRAVE_Off) l->cache->on=ENGRAVE_Off;
		if (!l->next) break;
		
		if (l->on==ENGRAVE_Off || l->next->on==ENGRAVE_Off) {
			lc=l->cache;
			while (lc!=l->next->cache) {
				lc->on=ENGRAVE_Off;
				lc=lc->next;
			}
		}
		
		
		l=l->next;
	} while (l && l!=lstart);

	return 0;
}

/*! If gradient==NULL, then install a default one.
 *
 * type must be 'l' for linear, or 'r' for radial.
 */
void EngraverPointGroup::InstallTraceGradient(char type, GradientData *ngradient, int absorbcount)
{
	GradientData *gradient=ngradient;
	if (!gradient) {
		absorbcount=1;

		flatpoint p1(0,0), p2(1,0);
		if (type=='r') p2.x=0;
		ScreenColor col1(1.0,1.0,1.0,1.0), col2(0.0,0.0,0.0,1.0);
		gradient = new GradientData(p1,p2,0,1, &col1,&col2, type=='l' ? GradientData::GRADIENT_LINEAR : GradientData::GRADIENT_RADIAL);
		gradient->FindBBox();

		Affine aa(owner->GetTransformToContext(false,0));
		DoubleBBox bbox;
		bbox.addtobounds(aa.m(), owner);
		gradient->AlignAndFit(NULL,&bbox,50,50,2);
	}

	trace->Install(type=='l' ? TraceObject::TRACE_LinearGradient : TraceObject::TRACE_RadialGradient, gradient);
	if (absorbcount) gradient->dec_count();
}

/*! If newtrace is NULL, then dec_count() the old one and install a fresh default one.
 * Otherwise decs count on old, incs count on newtrace.
 */
void EngraverPointGroup::InstallTraceSettings(EngraverTraceSettings *newtrace, int absorbcount)
{
	if (!newtrace) {
		if (trace) trace->dec_count();
		trace=new EngraverTraceSettings();
		trace->SetResourceOwner(this); 
		return;
	}
	if (newtrace==trace) return;
	if (trace) trace->dec_count();
	trace=newtrace;
	if (!absorbcount) trace->inc_count();

	if (trace->ResourceOwner()==NULL) trace->SetResourceOwner(this);
}

/*! If newdash is NULL, then dec_count() the old one and install a fresh default one.
 * Otherwise decs count on old, incs count on newdash.
 */
void EngraverPointGroup::InstallDashes(EngraverLineQuality *newdash, int absorbcount)
{
	if (!newdash) {
		if (dashes) dashes->dec_count();
		dashes=new EngraverLineQuality();
		dashes->SetResourceOwner(this); 
		return;
	}
	if (newdash==dashes) return;
	if (dashes) dashes->dec_count();
	dashes=newdash;
	if (!absorbcount) dashes->inc_count();
	if (dashes->ResourceOwner()==NULL) dashes->SetResourceOwner(this);

	//UpdateDashCache();
}

void EngraverPointGroup::InstallDirection(EngraverDirection *newdir, int absorbcount)
{
	if (!newdir) {
		if (direction) direction->dec_count();
		direction=new EngraverDirection();
		direction->SetResourceOwner(this); 
		return;
	}
	
	if (newdir==direction) return;
	if (direction) direction->dec_count();
	direction=newdir;
	if (!absorbcount) direction->inc_count();

	if (direction->ResourceOwner()==NULL) direction->SetResourceOwner(this);
}

void EngraverPointGroup::InstallSpacing(EngraverSpacing *newspace, int absorbcount)
{
	if (!newspace) {
		if (spacing) spacing->dec_count();
		spacing=new EngraverSpacing();
		spacing->SetResourceOwner(this); 
		return;
	}

	if (newspace==spacing) return;
	if (spacing) spacing->dec_count();
	spacing=newspace;
	if (!absorbcount) spacing->inc_count();

	if (spacing->ResourceOwner()==NULL) spacing->SetResourceOwner(this);
}

/*! Provide a direction vector for specified point. This is used to grow lines
 * in EngraverFillData objects. If you need exact lines, you will want to use
 * LineFrom(), since building from Direction() here will introduce too many
 * rounding errors. For instance, you will never build exact circles only
 * from a direction field.
 */
flatpoint EngraverPointGroup::Direction(double s,double t)
{
	if (direction->type==PGROUP_Linear) {
		return directionv;

	} else if (direction->type==PGROUP_Circular) {
		return transpose(flatpoint(s,t)-position);

	} else if (direction->type==PGROUP_Radial) {
		return flatpoint(s,t)-position;

	} else if (direction->type==PGROUP_Spiral) {
		int numarms=2;
		int spin=1;
		EngraverDirection::Parameter *p=direction->FindParameter("spin");
		if (p) spin=(p->value==0 ? 1 : -1);
		p=direction->FindParameter("arms");
		if (p) numarms=p->value+.5;

		double r=sqrt((s-position.x)*(s-position.x)+(t-position.y)*(t-position.y));
		double theta=r/numarms/spacing->spacing;

		flatpoint v(spin*(cos(theta)+theta*sin(theta)), sin(theta)-theta*cos(theta));
		v.normalize();
		return v;
	}

	return flatpoint();
}

/*! Create a line extending from coordinate s,t.
 */
LinePoint *EngraverPointGroup::LineFrom(double s,double t)
{
	if (direction->type==PGROUP_Linear) {

	} else if (direction->type==PGROUP_Circular) {
	} else if (direction->type==PGROUP_Radial) {
	} else if (direction->type==PGROUP_Spiral) {
	}

	return NULL;
}

/*! Scale up each linepoint->weight by factor.
 * If factor<=0 or factor==1.0, then nothing is done.
 */
void EngraverPointGroup::QuickAdjust(double factor)
{
	if (factor<=0 || factor==1.0) return;

	for (int c=0; c<lines.n; c++) {
		LinePoint *l=lines.e[c];
		LinePoint *start=l;

		do {
			l->weight*=factor;
			if (l->cache) l->cache->weight*=factor;
			l=l->next;
		} while (l && l!=start);
	}

	UpdateDashCache(); 

	if (owner) owner->touchContents();
}

/*! fill in x,y = 0..1,0..1
 */
void EngraverPointGroup::Fill(EngraverFillData *data, double nweight)
{
	if (directionv.isZero()) directionv.x=1;
	if (nweight>0) direction->default_weight=default_weight=nweight;
	if (nweight<=0) nweight=direction->default_weight;

	lines.flush();

	if (direction->type==PGROUP_Circular) {
		FillCircular(data,nweight);

	} else if (direction->type==PGROUP_Radial) {
		FillRadial(data,nweight);

	} else if (direction->type==PGROUP_Spiral) {
		int numarms=2;
		int spin=1;

		EngraverDirection::Parameter *p=direction->FindParameter("spin");
		if (p) spin=(p->value==0 ? 1 : -1);
		p=direction->FindParameter("arms");
		if (p) numarms=p->value+.5;

		FillSpiral(data,nweight, numarms,0,0,spin); //arms, r0, b, spin

	} else { //default: if (type==PGROUP_Linear) {
		FillRegularLines(data,nweight);

	}

	if (direction->default_weight<0) {
		direction->default_weight=spacing->spacing/10;
		default_weight=direction->default_weight;
	}

	if (owner) owner->touchContents();
}

/*! spacing is an object distance (not in s,t space) to be used as the distance between line centers.
 * If spacing<0, then use 1/20 of the x or y dimension, whichever is smaller
 * If weight<0, then use spacing/10.
 * Inserts lines follow	a,<<<ing this->direction, which is in (s,t) space.
 */
void EngraverPointGroup::FillRegularLines(EngraverFillData *data, double nweight)
{
	double thisspacing=spacing->spacing;
	if (thisspacing<=0) thisspacing=(data->maxy-data->miny)/20;
	spacing->spacing=thisspacing;

	double weight=nweight;
	if (weight<=0) weight=thisspacing/10; //remember, weight is actual distance, not s,t!!
	thisspacing=spacing->spacing/data->getScaling(.5,.5,false);


	LinePoint *p;

	flatvector v=directionv; //this is s,t space
	//if (v.x<0) v=-v;
	v.normalize();
	v*=thisspacing;
	flatvector vt=transpose(v);

	 //we need to find the s,t equivalent of spacing along direction


	// *** when mesh is not a single square, currently spacing is irrational
	//if (xsize>4) s_spacing/= xsize/3;
	//if (ysize>4) t_spacing/= ysize/3;


	 //set up starters along an appropriate diagonal
	flatpoint sv,sp;
	flatvector pp;
	if ((v.y>=0 && v.x>=0) || (v.y<=0 && v.x<=0)) {
		sp.set(0,1);
		sv.set(1,-1);
	} else {
		sp.set(0,0);
		sv.set(1,1);
	}
	sv.normalize();
	double d=thisspacing*thisspacing/(sv*vt);
	sv*=d;

	flatpoint o;
	intersection(flatline(sp,sp+sv), flatline(position,position+v), o);

	 //starters forward from position
	pp=o;
	do {
		lines.push(new LinePoint(pp.x,pp.y, weight));
		pp+=sv;
	} while (pp.x>=0 && pp.y>=0 && pp.x<=1 && pp.y<=1);

	 //starters backward from position
	pp=o-sv;
	while (pp.x>=0 && pp.y>=0 && pp.x<=1 && pp.y<=1) {
		lines.push(new LinePoint(pp.x,pp.y, weight));
		pp-=sv;
	}

	 //initialize noise
	OpenSimplexNoise noise;
	if (direction->seed>0) { noise.Seed(direction->seed); srandom(direction->seed); }
	else { noise.Seed(random()); }
	double featuresize=direction->noise_scale;
	if (featuresize<.8) featuresize=.8;
	featuresize*=featuresize;

	 //grow lines
	flatvector curp;
	double rnd;

	v*=direction->resolution;

	for (int c=0; c<lines.n; c++) {
		p=lines.e[c];
		curp.set(p->s,p->t);

		 //apply any whole line randomness at beginning...
		if (direction->line_offset) {
			rnd=(double)random()/RAND_MAX - .5;
			curp += vt * direction->line_offset * rnd;

			p->s=curp.x;
			p->t=curp.y;
		}

		pp=curp;

		 //go foward
		while (1) {
			curp=curp + v;
			pp=curp; 

			if (pp.x<0 || pp.y<0 || pp.x>1 || pp.y>1) break;

			p->next=new LinePoint(pp.x, pp.y, weight);
			p->next->prev=p;
			p=p->next;
		}

		 //go backward
		p=lines.e[c];
		curp.set(p->s,p->t);

		while (1) { 
			curp=curp - v;
			pp=curp;

			if (pp.x<0 || pp.y<0 || pp.x>1 || pp.y>1) break;

			p->prev=new LinePoint(pp.x, pp.y, weight);
			p->prev->next=p;
			p=p->prev;

		}
		if (p!=lines.e[c]) lines.e[c]=p;

		if (direction->point_offset) {
			while (p) {
			 //apply point randomness if necessary
				pp.set(p->s,p->t);
				rnd=(noise.Evaluate((pp.x-.5)/thisspacing/featuresize, (pp.y-.5)/thisspacing/featuresize) - .5);
				pp += vt * direction->point_offset * rnd;

				p->s=pp.x;
				p->t=pp.y;

				p=p->next;
			}
		}
	}


	 //apply line profile stuff
	if (direction->default_profile || direction->profile_start!=0 || direction->profile_end!=1
			|| direction->start_rand_width!=0 || direction->end_rand_width!=0) {

		double start,ss, end, ee, d, dd, segd;
		//double tt;
		LinePoint *tp;
		v.normalize(); 

		for (int c=0; c<lines.n; c++) {
			p=lines.e[c];
			d=0;
			while (p) {
				if (p->next) d+=norm(flatpoint(p->next->s - p->s, p->next->t - p->t));
				p=p->next;
			}

			direction->GetStartEnd(&start,&end, true);

			ss=start*d;
			ee=end*d;

			 //position and weight start, turning off all points before start
			p=lines.e[c];
			dd=0;
			while (p && p->next) {
				if (start>0) p->on=ENGRAVE_Off;

				v=flatpoint(p->next->s - p->s, p->next->t - p->t);
				segd=norm(v);

				if (ss<dd+segd) {
					 //add point in between for start
					v*=(ss-dd)/segd;
					v+=flatpoint(p->s,p->t);

					if (direction->default_profile) {
						direction->default_profile->GetWeight(0, &weight, NULL,NULL);
						weight*=direction->max_height*spacing->spacing;
					}

					if (start==0) {
						p->weight=weight;
					} else {
						tp=new LinePoint(v.x, v.y, weight);
						tp->prev=p;
						tp->next=p->next;
						if (p->next) p->next->prev=tp;
						p->next=tp;
						p=p->next;
					}

					dd=ss;
					break;
				}

				dd+=segd;
				p=p->next;
			}

			 //position and weight end
			while (p && p->next) {

				v=flatpoint(p->next->s - p->s, p->next->t - p->t);
				segd=norm(flatpoint(p->next->s - p->s, p->next->t - p->t));

				if (ee<dd+segd) {
					 //add point in between for end
					v*=(ee-dd)/segd;
					v+=flatpoint(p->s,p->t);

					if (direction->default_profile) {
						direction->default_profile->GetWeight(1, &weight, NULL,NULL);
						weight*=direction->max_height*spacing->spacing;
					}
					tp=new LinePoint(v.x, v.y, weight);
					tp->prev=p;
					tp->next=p->next;
					if (p->next) p->next->prev=tp;
					p->next=tp; 

					p=tp->next;
					break;
				}

				dd+=segd;
				p=p->next;
				if (p && direction->default_profile) {
					direction->default_profile->GetWeight((dd-ss)/(ee-ss), &p->weight, NULL,NULL);
					p->weight*=direction->max_height*spacing->spacing;
				}
			}

			 //turn off all after end
			while (p) {
				//p->weight=0;
				if (p->next || (!p->next && end!=1)) p->on=ENGRAVE_Off;
				p=p->next;
			}
		}
	}
}

/*! If weight<0, then use spacing/10.
 */
void EngraverPointGroup::FillRadial(EngraverFillData *data, double nweight)
{
	double thisspacing=spacing->spacing;
	if (thisspacing<=0) thisspacing=(data->maxy-data->miny)/20;
	spacing->spacing=thisspacing;
		

	double weight=nweight;
	if (weight<=0) weight=spacing->spacing/10; //remember, weight is actual distance, not s,t!!
	thisspacing=spacing->spacing/data->getScaling(.5,.5,false);
	thisspacing*=2;


	int numpoints=2*M_PI/thisspacing;
	if (numpoints<3) numpoints=3;
	double dangle=2*M_PI/numpoints;

	OpenSimplexNoise noise;
	if (direction->seed>0) { noise.Seed(direction->seed); srandom(direction->seed); }
	else { noise.Seed(random()); }
	double featuresize=direction->noise_scale;
	if (featuresize<.8) featuresize=.8;
	featuresize*=featuresize;

	LinePoint *p;
	flatpoint pp, lastp;
	flatpoint v, vt;
	double ang_offset;
	double rnd;
	double curr, dr;

	for (int c=0; c<numpoints; c++) {
		 //apply any whole line randomness at beginning...
		ang_offset=0;
		if (direction->line_offset) {
			rnd=(double)random()/RAND_MAX - .5;
			ang_offset = rnd * 2*M_PI/numpoints * direction->line_offset/2;
		}
		v= direction->resolution * rotate(.05*directionv/norm(directionv),ang_offset + 2*M_PI*c/numpoints);
		dr=norm(v);

		if (direction->point_offset) {
			vt=transpose(v);
			vt.normalize();
		}

		pp=position;
		p=new LinePoint(position.x, position.y, weight);
		lines.push(p);
		curr=0;

		lastp=position;
		
		while (1) {
			pp=flatpoint(p->s,p->t) + v;
			if (pp.x<0 || pp.y<0 || pp.x>1 || pp.y>1) break;

			curr+=dr;

			if (direction->point_offset) {
				rnd=(noise.Evaluate(pp.x/thisspacing/featuresize, pp.y/thisspacing/featuresize) - .5);
				pp += vt * curr*dangle * direction->point_offset * rnd;
			}

			p->next=new LinePoint(pp.x, pp.y, weight);
			p->next->prev=p;
			p=p->next;
		}
	}
}

/*! If weight<0, then use spacing/10.
 *
 * r = spin * (r0 + spacing->spacing*numarms*theta * exp(b*theta))
 */
void EngraverPointGroup::FillSpiral(EngraverFillData *data, double nweight, int numarms, double r0,double b, int spin)
{
	if (spin<0) spin=-1; else spin=1;

	double thisspacing=spacing->spacing;
	if (thisspacing<=0) thisspacing=(data->maxy-data->miny)/20;
	spacing->spacing=thisspacing;

	double weight=nweight;
	if (weight<=0) weight=spacing->spacing/10; //remember, weight is actual distance, not s,t!!

	thisspacing=spacing->spacing/data->getScaling(.5,.5,false)/2/M_PI;
	double dist= thisspacing * direction->resolution*5;


	int numpoints=2*M_PI/thisspacing;
	if (numpoints<3) numpoints=3;

	LinePoint *p=NULL;
	flatpoint pp, lastp;
	flatpoint v;

	double angle0=atan2(directionv.y,directionv.x);
	if (numarms<=0) numarms=1;
	double armangle=2*M_PI/numarms;

	double r=0, rr, lastr;
	double theta=0, dtheta;
	int on=0;


	 //initialize noise
	OpenSimplexNoise noise;
	if (direction->seed>0) { noise.Seed(direction->seed); srandom(direction->seed); }
	else { noise.Seed(random()); }
	double featuresize=direction->noise_scale;
	if (featuresize<.8) featuresize=.8;
	featuresize*=featuresize*1.5;
	double rnd;


	 //find largest radius
	double largestr=0;
	r=largestr=norm(flatpoint(0,0)-position);
	r=norm(flatpoint(1,0)-position);
	if (r>largestr) largestr=r;
	r=norm(flatpoint(0,1)-position);
	if (r>largestr) largestr=r;
	r=norm(flatpoint(1,1)-position);
	if (r>largestr) largestr=r;

	//direction->line_offset ignored, since there are so few spiral arms

	for (int c=0; c<numarms; c++) { 
		on=0;
		r=0;
		lastr=0;
		p=NULL;
		theta=0;

		DBG int iter=0;

		do {
			//r = thisspacing * numarms*theta * exp(b * numarms*theta);
			r = thisspacing * numarms * theta;
			rr = (r0 + r);

			if (direction->point_offset) {
				rnd=(noise.Evaluate(pp.x/thisspacing/featuresize, pp.y/thisspacing/featuresize) - .5);
				//rr += thisspacing*2 * direction->point_offset * rnd;
				rr += (r<thisspacing ? r/thisspacing : 1) * thisspacing*2 * direction->point_offset * rnd;
			}


			pp.x = position.x + spin*rr*cos(theta + angle0 + c*armangle);
			pp.y = position.y + rr*sin(theta + angle0 + c*armangle);

			if (r==0 || lastr==0) {
				//DBG cerr <<"  ~~~~~~~~iter=:"<<iter<<"  assigning dtheta = pi/10 "<<endl;

				theta+=M_PI/10;

			} else {
				//dtheta = dist/r;
				double inside=dist*dist - (r-lastr)*(r-lastr);
				if (inside<=0) dtheta=M_PI/10;
				else dtheta = sqrt(inside) / lastr;

				//DBG cerr <<"  ~~~~~~~~~~~inside: "<<dist*dist - (r-lastr)*(r-lastr)<<"  dist="<<dist<<endl;
				//DBG cerr <<"  ~~~~~~~~iter=:"<<iter<<"  dtheta: "<<dtheta<<"  lastr="<<lastr<<"  r="<<r<<"  theta="<<theta<<endl;

				if (dtheta>.3) dtheta=.3;
				theta+=dtheta;
			}

			lastr=r;

			if (pp.x>=0 && pp.y>=0 && pp.x<=1 && pp.y<=1) { //point in bounds
				if (on) {
					 //continue existing line
					p->next=new LinePoint(pp.x, pp.y, weight);
					p->next->prev=p;
					p=p->next;
				} else {
					 //start a new line
					p=new LinePoint(pp.x, pp.y, weight);
					lines.push(p);
					on=1;
				}
			} else { //point out of bounds
				if (on) {
					 //end line, start new on next one maybe
					on=0;
					p=NULL;
				} else {
					//nothing to do, already off!
				}
			}

			DBG iter++;

		} while (r<largestr);
	}
}


/*! If weight<0, then use spacing/10.
 */
void EngraverPointGroup::FillCircular(EngraverFillData *data, double nweight)
{
	double thisspacing=spacing->spacing;
	if (thisspacing<=0) thisspacing=(data->maxy-data->miny)/20;
	spacing->spacing=thisspacing;

	double weight=nweight;
	if (weight<=0) weight=spacing->spacing/10; //remember, weight is actual distance, not s,t!!
	thisspacing=spacing->spacing/data->getScaling(.5,.5,false);


	int numpoints=30;

	LinePoint *p;
	flatpoint pp;

	 //find largest radius
	double r=0, rr, largestr=0;
	r=largestr=norm(flatpoint(0,0)-position);
	r=norm(flatpoint(1,0)-position);
	if (r>largestr) largestr=r;
	r=norm(flatpoint(0,1)-position);
	if (r>largestr) largestr=r;
	r=norm(flatpoint(1,1)-position);
	if (r>largestr) largestr=r;

	OpenSimplexNoise noise;
	if (direction->seed>0) { noise.Seed(direction->seed); srandom(direction->seed); }
	else { noise.Seed(random()); }
	double featuresize=direction->noise_scale;
	if (featuresize<.8) featuresize=.8;
	featuresize*=featuresize;


	r=-thisspacing/2;
	LinePoint *sp=NULL; //segment start
	int first=-1;
	DBG int circle=0;
	double rnd;
	flatpoint v;
	double angle;

	while (r<largestr) { //one loop per radius
		r+=thisspacing;
		numpoints=10+2*r*M_PI/thisspacing;
		sp=p=NULL;
		first=-1;
		rr=r;

		double start=0, end=1;
		bool fullcircles=true;
		direction->GetStartEnd(&start,&end, false);
		if ((start==0 && end==1) || (start==1 && end==0)) { start=0; end=1; }
		if (end<start) end+=1;
		if (end!=start+1) fullcircles=false;
		start*=2*M_PI;
		end  *=2*M_PI;

		 //apply any whole line randomness at beginning...
		if (direction->line_offset) {
			rnd=(double)random()/RAND_MAX - .5;
			rr += thisspacing * direction->line_offset * rnd;
		}

		DBG circle++;

		for (int c=0; c<=numpoints+1; c++) { //for each circle
			 //apply point randomness if necessary
			angle=start+(end-start)*c/numpoints;;
			pp=rr*flatpoint(cos(angle),sin(angle));

			v=pp;
			v.normalize();
			v*=thisspacing;

			if (direction->point_offset) {
				rnd=(noise.Evaluate(pp.x/thisspacing/featuresize, pp.y/thisspacing/featuresize) - .5);
				pp += v * direction->point_offset * rnd;
			}

			pp+=position;

			if (pp.x<0 || pp.x>1 || pp.y<0 || pp.y>1) {
				 //point out of bounds
				if (!sp) continue; //no current segment, so just skip

				//else we had a current segment, so we have to terminate
				sp=NULL;
				continue;
			}

			//now we have a point to add

			if (c==numpoints+1) {
				 //we have come full circle, the previous point was on
				if (first>=0 && sp && fullcircles) {
					 //first point of circle was on, so since this last point is also on,
					 //we need to connect first and last
					lines.e[first]->prev=p;
					p->next=lines.e[first];
					lines.e[first]=sp;
					if (sp->prev) sp->prev->next=NULL;
					sp->prev=NULL;

					if (first!=lines.n-1) { //we connected two different segments
						lines.e[lines.n-1]=NULL;
						lines.n--;
					}
				}
				first=-1;
				sp=NULL;
				break;
			}

			if (sp==NULL) {
				 //starting new segment
				p=new LinePoint(pp.x, pp.y, weight);
				lines.push(p);

				 //we need to remember the start of the circle, in case we need to
				 //connect the final segment of the circle to the first segment
				sp=p;
				if (c==0) first=lines.n-1;

			} else {
				p->next=new LinePoint(pp.x, pp.y, weight);
				p->next->prev=p;
				p=p->next;
			}

		} //foreach point in circle
	} //for each radius

}

//------------------------- class StarterPoint, used for growing lines --------------------
/*! \class StarterPoint
 * Class to aid growing engraver lines.
 */
 
StarterPoint::StarterPoint(flatpoint p, int indir, double weight,int groupid, int nlineref)
{
	first = last = line = new LinePoint(p.x, p.y, weight);
	iteration  = 0;
	piteration = 0;
	dodir      = indir;
	lineref    = nlineref;
}


//------------------------ class GrowContext --------------------------
/*! \class GrowContext
 * Holds cached data so that processing for growing lines can be spread across multiple frames.
 */

GrowContext::GrowContext()
{}

GrowContext::~GrowContext()
{
	delete[] scratch_data;
	if (spacingmap)   spacingmap  ->dec_count();
	if (weightmap)    weightmap   ->dec_count();
	if (directionmap) directionmap->dec_count();
}


/*! Initialize growing points. 
 * If growpoint_ret already has points in it, use those, don't create automatically along edges.
 */
GrowContext *EngraverPointGroup::GrowLines_Init(EngraverFillData *data,
									double resolution, 
									double defaultspace,  	ValueMap *spacingmap,
									double defaultweight,   ValueMap *weightmap, 
									flatpoint directionv,   DirectionMap *directionmap,
									int iteration_limit,
									Laxkit::PtrStack<GrowPointInfo> *custom_starters
									)
{
	//draw on a scratch space, each pixel gets:
	//  group number
	//  line number
	//  point number
	//  direction for that point

	//Each iteration:
	//  - Compute next point based on current direction
	//  - Computer whether to merge or split based on neighborhood of point
	//  - stack push/pulls of adjacent points

	// init scratch space
	//unsigned char pixels[***];
	

	 //remove any old lines from same group
	lines.flush();

	if (grow_cache) delete grow_cache;
	grow_cache = new GrowContext();
	GrowContext *context = grow_cache;

	context->resolution = resolution;
	context->defaultspace = defaultspace;
	context->spacingmap = spacingmap;
	if (spacingmap) spacingmap->inc_count();
	context->defaultweight = defaultweight;
	context->weightmap = weightmap;
	if (weightmap) weightmap->inc_count();
	context->directionv = directionv;
	context->directionmap = directionmap;
	if (directionmap) directionmap->inc_count();
	context->iteration_limit = iteration_limit;



	 //----Initialize point generators
	DoubleBBox bounds(0,data->xsize/3, 0,data->ysize/3);

	double weight=defaultweight;
	double curspace=defaultspace/data->getScaling(.5,.5,false);

	PtrStack<StarterPoint> &generators = context->generators;
	StarterPoint *g;

	if (custom_starters && custom_starters->n > 0) {
		 //use supplied points
		for (int c=0; c<custom_starters->n; c++) {
			g = new StarterPoint(custom_starters->e[c]->p, custom_starters->e[c]->godir, weight, id, generators.n);
			g->line->p = data->getPoint(g->line->s, g->line->t, true);
			g->line->needtosync = 0;
			lines.push(g->line);
			context->generators.push(g,1);
		}

	} else if (directionmap && directionmap != this) {
		 //try to trace out starters based on directionmap
		// *** begin at center, radiate away

	} else {
		 //need to populate with default starters per direction type

		if (direction->type == PGROUP_Linear) {
			flatpoint p1, p2, pp;
			double ds;
			flatpoint v = directionv;
			//flatpoint v = direction->direction;
			v.normalize();
			flatpoint vt = transpose(v);

			if ((v.x>0 && v.y>0) || (v.x<0 && v.y<0)) {
				p1.set(bounds.minx, bounds.maxy);
				p2.set(bounds.maxx, bounds.miny);
			} else {
				p1.set(bounds.minx, bounds.miny);
				p2.set(bounds.maxx, bounds.maxy);
			}

			v=p2-p1;
			double dist=norm(v);
			v.normalize();
			flatpoint p=p1;
			double cosa = fabs(v*vt);
			ds=curspace/(cosa);

			for (double c=0; c<dist; ) {
				pp=data->getPoint(p.x,p.y, true);
				if (spacingmap) {
					curspace = spacingmap->GetValue(pp)/data->getScaling(p.x,p.y,true); //else spacing is constant
					ds = curspace/cosa;
				}
				if (weightmap)  weight  =weightmap ->GetValue(pp); //else weight is constant

				if (c>0) {
					g=new StarterPoint(p, 3, weight, id, generators.n);
					g->line->p=pp;
					g->line->needtosync=0;
					lines.push(g->line);
					context->generators.push(g,1);
					//if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
				}

				c += ds;
				p += ds*v;
			}

		} else if (direction->type == PGROUP_Circular || direction->type == PGROUP_Spiral) {
			 // along largest line from center to any edge
			//flatpoint center=direction->position;
			flatpoint center=position;
			flatpoint outside;
			
			if (center.x<.5) {
				if (center.y<.5) {
					outside=bounds.BBoxPoint(1,1);
				} else {
					outside=bounds.BBoxPoint(1,0);
				} 
			} else {
				if (center.y<.5) {
					outside=bounds.BBoxPoint(0,1);
				} else {
					outside=bounds.BBoxPoint(0,0);
				} 
			} 

			center=bounds.BBoxPoint(center.x,center.y);
			flatpoint p=center, pp;
			double cosa=1;
			flatpoint v=outside-center;
			double dist=norm(v);
			v.normalize();
			double ds=curspace;

			for (double c=0; c<dist; ) {
				pp=data->getPoint(p.x,p.y, true);
				//if (direction->type == PGROUP_Spiral) {
				//	cosa = ***;
				//}
				if (spacingmap) {
					curspace = spacingmap->GetValue(pp)/data->getScaling(p.x,p.y,true); //else spacing is constant
					ds = curspace/cosa;
				}
				if (weightmap)  weight  =weightmap ->GetValue(pp); //else weight is constant

				if (c>0) {
					g=new StarterPoint(p, 3, weight, id, generators.n);
					g->line->p=pp;
					g->line->needtosync=0;
					lines.push(g->line);
					context->generators.push(g,1);
					//if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
				}

				c += ds;
				p += ds*v;
			}

		} else if (direction->type == PGROUP_Radial) {
			 //along biggest circle at center
			flatpoint center=position;
			center=bounds.BBoxPoint(center.x,center.y);

			double radius=center.x;
			if (bounds.maxx-center.x<radius) radius=bounds.maxx-center.x;
			if (center.y<radius) radius=center.y;
			if (bounds.maxy-center.y<radius) radius=bounds.maxy-center.y;

			flatpoint p=center, pp;
			double da=2*M_PI/int(2*M_PI*radius/curspace);

			for (double a=0; a<2*M_PI; a+=da) {
				p=center+radius*flatpoint(cos(a),sin(a));
				pp=data->getPoint(p.x,p.y, true);

				if (spacingmap) {
					curspace = spacingmap->GetValue(pp)/data->getScaling(p.x,p.y,true); //else spacing is constant
					da=2*M_PI/int(2*M_PI*radius/curspace);
				}

				if (weightmap)  weight  =weightmap ->GetValue(pp); //else weight is constant

				g=new StarterPoint(p, 3, weight, id, generators.n);
				g->line->p=pp;
				g->line->needtosync=0;
				lines.push(g->line);
				context->generators.push(g,1);
				//if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			} 
		}
	}

	return context;
}

/*! Return true if there is more to iterate.
 */
bool EngraverPointGroup::GrowLines_Iterate()
{
	if (!grow_cache) return false;
	return false; //TODO!
}

void EngraverPointGroup::GrowLines_Finish()
{
	if (!grow_cache) return;

	 //Finish off lines
//	LinePoint *lp, *ll;
//	for (int c=0; c<lines.n; c++) {
//		lp=lines.e[c];
//
//		 //need to normalize all points
//		ll=lp;
//		while (ll) {
//			ll->s = (ll->s - bounds.minx) / (bounds.maxx - bounds.minx);
//			ll->t = (ll->t - bounds.miny) / (bounds.maxy - bounds.miny);
//
//			if (ll->s >= 1 || ll->t >= 1 || ll->s <= 0 || ll->t <= 0) {
//				ll->p = data->getPoint(ll->s, ll->t, false);
//				ll->needtosync = 0;
//			}
//			ll = ll->next;
//		}
//	}

	grow_cache->active = false;
	//delete grow_cache;
	//grow_cache = nullptr;
	UpdateDashCache();	
}

/*! If growpoint_ret already has points in it, use those, don't create automatically along edges.
 */
void EngraverPointGroup::GrowLines_OLD(EngraverFillData *data,
									double resolution, 
									double defaultspace,  	ValueMap *spacingmap,
									double defaultweight,   ValueMap *weightmap, 
									flatpoint direction,    DirectionMap *directionmap,
									Laxkit::PtrStack<GrowPointInfo> *growpoint_ret,
									int iteration_limit)
{
	 //remove any old lines from same group
	lines.flush();


	if (directionmap==NULL) directionmap=this;

	//double dir_rotation=0;
	DoubleBBox bounds(0,data->xsize/3, 0,data->ysize/3);
	PtrStack<StarterPoint> generators;
	StarterPoint *g;
	flatpoint v=resolution*directionv/norm(directionv);
	flatpoint vv;
	double weight=defaultweight;
	double curspace=defaultspace/data->getScaling(.5,.5,false);
	resolution/=data->getScaling(.5,.5,false);

	double VEPSILON=1e-6;
	flatpoint p(bounds.minx,bounds.miny);
	flatpoint p2;


	 //create initial generators 

	if (growpoint_ret && growpoint_ret->n>0) {
		 //use supplied points
		for (int c=0; c<growpoint_ret->n; c++) {
			g=new StarterPoint(growpoint_ret->e[c]->p, growpoint_ret->e[c]->godir, weight, id, generators.n);
			g->line->p=data->getPoint(g->line->s, g->line->t, true);
			g->line->needtosync=0;
			generators.push(g,1);
		}

	} else {
		 //and add initial line points along each of the 4 edges of bounds

		 //start at origin, add points around bounds, starting with lower x edge
		while (p.x<bounds.maxx) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.y>0 && ((v.x>0 && p.x<bounds.maxx) || (v.x<0 && p.x>bounds.minx))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.y)<VEPSILON) { p.x=bounds.maxx; break; }

			p.x+=fabs(resolution*curspace/v.y);
		}

		 //find initial starter along maxx segment
		if (fabs(v.y)<VEPSILON) { //like horizontal lines
			p.x=bounds.maxx;
			p.y=bounds.miny+spacing->spacing;
		} else {
			if (fabs(v.x)<VEPSILON) {
				p.x=bounds.maxx; //need to skip traverse up maxx edge
				p.y=bounds.maxy;
			} else {
				p.y=bounds.miny + (-p.x+bounds.maxx)*v.y/v.x;
				while (p.y<bounds.miny) p.y+=fabs(spacing->spacing*v.y/v.x);
				p.x=bounds.maxx;
			}
		}

		 //cruise up maxx side
		while (p.y<bounds.maxy) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.x<0 && ((v.y>0 && p.y>bounds.miny) || (v.y<0 && p.y<bounds.maxy))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.x)<VEPSILON) { p.y=bounds.maxy; break; }

			p.y+=fabs(resolution*curspace/v.x);
		}

		 //find initial starter on maxy edge
		if (fabs(v.x)<VEPSILON) { //like horizontal lines
			p.x=bounds.maxx-spacing->spacing;
			p.y=bounds.maxy;
		} else {
			if (fabs(v.y)<VEPSILON) {
				p.x=bounds.minx; //need to skip traverse
				p.y=bounds.maxy;
			} else {
				p.x=bounds.maxx - (p.y-bounds.maxy)*v.x/v.y;
				while (p.x>bounds.maxx) p.x-=fabs(spacing->spacing*v.x/v.y);
				p.y=bounds.maxy;
			}
		}


		 //cruise down maxy side
		while (p.x>bounds.minx) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.y<0 && ((v.x>0 && p.x<bounds.maxx) || (v.x<0 && p.x>bounds.minx))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.y)<VEPSILON) { p.x=bounds.minx; break; }

			p.x-=fabs(resolution*curspace/v.y);
		}

		 //find initial starter on minx edge
		if (fabs(v.y)<VEPSILON) { //like horizontal lines
			p.x=bounds.minx;
			p.y=bounds.maxy-spacing->spacing;
		} else {
			if (fabs(v.x)<VEPSILON) {
				p.x=bounds.minx; //need to skip traverse down minx edge
				p.y=bounds.miny;
			} else {
				p.y=bounds.maxy + (-p.x+bounds.minx)*v.y/v.x;
				while (p.y>bounds.maxy) p.y-=fabs(spacing->spacing*v.y/v.x);
				p.x=bounds.minx;
			}
		}


		 //cruise down minx side
		while (p.y>bounds.miny) {
			if (directionmap) {
				v=directionmap->Direction(p);
				v*=resolution/norm(v);
			}
			p2=data->getPoint(p.x,p.y, true);
			if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
			if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

			 //add new generator if we are pointing toward inside bounds
			if (v.x>0 && ((v.y>=0 && p.y>bounds.miny) || (v.y<0 && p.y<bounds.maxy))) {
				g=new StarterPoint(p, 1, weight, id, generators.n);
				g->line->p=p2;
				g->line->needtosync=0;
				lines.push(g->line);
				generators.push(g,1);
				if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(p,g->dodir));
			}

			 //advance p to next point along x
			if (fabs(v.x)<VEPSILON) { p.y=bounds.maxy; break; }

			p.y-=fabs(resolution*curspace/v.x);
		}

		for (int c=0; c<lines.n; c++) {
			lines.e[c]->p=data->getPoint(lines.e[c]->s,lines.e[c]->t, true);
			lines.e[c]->needtosync=0;
		}

	} //if adding starters along edges

	// 
	// Now all initial starter points positioned on edges,
	// we must grow them...
	//


	 //grow points until generators depleted
	int iteration=0;
	bool maybefill=true;
	double xx=bounds.minx; //for fill point search below
	double yy=bounds.miny;

	do {
		DBG cerr <<" iter: "<<iteration<<" g.n:"<<generators.n<<"  ";
		iteration++;
		if (iteration==iteration_limit) {
			DBG cerr <<"Warning! EngraverPointGroup GrowLines() hit iteration limit of: "<<iteration_limit<<endl;
			generators.flush();
			break;
		}

		 //----advance each generator according to directionmap
		for (int c=0; c<generators.n; c++) {
			g=generators.e[c];

			 //advance forward
			if (g->dodir&1) {
				if (directionmap) {
					v=directionmap->Direction(g->last->s, g->last->t);
					v*=resolution/norm(v);
				} //else v already set;

				p=flatpoint(g->last->s + v.x, g->last->t + v.y);
				p2=data->getPoint(p.x,p.y, true);
				if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
				if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

				g->last->Add(new LinePoint(p.x,p.y, weight));
				g->last=g->last->next;
				g->last->p=p2;
				g->last->needtosync=0;
			}

			 //advance backwards
		   	if (g->dodir&2) {
				if (directionmap) {
					v=directionmap->Direction(g->first->s, g->first->t);
					v*=resolution/norm(v);
				}

				p=flatpoint(g->first->s - v.x, g->first->t - v.y);
				p2=data->getPoint(p.x,p.y, true);
				if (spacingmap) curspace=spacingmap->GetValue(p2)/data->getScaling(p.x,p.y,true); //else spacing is constant
				if (weightmap)  weight  =weightmap ->GetValue(p2); //else weight is constant

				g->first->AddBefore(new LinePoint(p.x,p.y, weight));
				g->first=g->first->prev;
				g->first->p=p2;
				g->first->needtosync=0;
			}
		}


		 //-----terminate lines now out of bounds
		 // *** todo: don't stretch so far out of bounds, interpolate to edge
		for (int c=generators.n-1; c>=0; c--) {
			g=generators.e[c];

			if (g->dodir&1) if (!bounds.boxcontains(g->last->s, g->last->t))  g->dodir&=~1;
			if (g->dodir&2) if (!bounds.boxcontains(g->first->s,g->first->t)) g->dodir&=~2;
			DBG cerr <<" gen dir:"<<g->dodir<<endl;

			if (g->dodir==0) {
				//all done with this generator! make sure the associated line starts at a point with no prev points
				if (g->first!=g->line) {
					int i=lines.findindex(g->line);
					lines.e[i]=g->first;
				}
				generators.remove(c);
			}

		}


		 //-----search for merges and splits
		for (int c=generators.n-1; c>=0; c--) {
			g=generators.e[c];


			if (g->dodir&1) {
				//search for g->last->p within curspace*.75 distance to any other sample points,
				//terminate if found

				 //find square of transformed spacing
				if (spacingmap) curspace=spacingmap->GetValue(g->last->p)/data->getScaling(g->last->s,g->last->t,true); //else spacing is constant
				double lsp=curspace*.95; //least space threshhold
				double ls2=lsp*lsp;
				double msp=curspace*1.5; //most space threshhold
				//double msp2=msp*msp;
				double ld=1e+10, d2;
				//LinePoint *lclosest=NULL;
				LinePoint *lp;
				//LinePoint *gg;
				p=g->last->p;

				for (int cc=0; cc<lines.n; cc++) {
					lp=lines.e[cc];
					while (lp->prev) lp=lp->prev;

					//gg=NULL;
					//if (cc==g->lineref) {
					//	 //is same line as current generator, need a special guard to not check against adjacent points to current points
					//	gg=g->last;
					//	for (int c2=0; c2<int(curspace/resolution)+1 && gg; c2++) gg=gg->prev; //skip at least a curspace worth of points
					//	if (!gg) continue;
					//}
					for ( ; lp; lp=lp->next) {
						//if (gg && gg==lp) break;
						if (lp==g->last) continue;

						if (lp->p.x < p.x-msp) continue; //skip out of range so we don't waste time on a lot of multiplications
						if (lp->p.x > p.x+msp) continue;
						if (lp->p.y < p.y-msp) continue;
						if (lp->p.y > p.y+msp) continue;

						d2=(lp->p.x - p.x)*(lp->p.x - p.x) + (lp->p.y - p.y)*(lp->p.y - p.y);  //norm2(lp->p - p);
						if ((cc==g->lineref && d2<resolution/2) || (cc!=g->lineref && d2<ld)) {
							//lclosest=lp;
							ld=d2;
						}
					}
				}

				if (ld<ls2) {
					//g->last->Add(new LinePoint(lclosest->s,lclosest->t, weight));
					//g->last=g->last->next;
					//g->last->p=lclosest->p;
					//g->last->needtosync=0;
					g->dodir&=~1;
				}
			} //search in next direction

			if (g->dodir&2) {
				//search for g->first->p within curspace*.75 distance to any other sample points
				//terminate if found

				if (spacingmap) curspace=spacingmap->GetValue(g->first->p)/data->getScaling(g->first->s,g->first->t,true); //else spacing is constant
				double lsp=curspace*.75; //least space threshhold
				double ls2=lsp*lsp;
				double msp=curspace*2; //most space threshhold
				//double msp2=msp*msp;
				double ld=1e+10, d2;
				//LinePoint *lclosest=NULL;
				LinePoint *lp;
				//LinePoint *gg;
				p=g->first->p;

				for (int cc=0; cc<lines.n; cc++) {
					lp=lines.e[cc];
					while (lp->next) lp=lp->next;

					//gg=NULL;
					//if (cc==g->lineref) {
					//	 //is same line as current generator, need a special guard to not check against adjacent points to current points
					//	gg=g->first;
					//	for (int c2=0; c2<int(curspace/resolution)+1 && gg; c2++) gg=gg->next; //skip at least a curspace worth of points
					//	if (!gg) continue;
					//}
					for ( ; lp; lp=lp->prev) {
						//if (gg && gg==lp) break;
						if (lp==g->first) continue;

						if (lp->p.x < p.x-msp) continue; //skip out of range so we don't waste time on a lot of multiplications
						if (lp->p.x > p.x+msp) continue;
						if (lp->p.y < p.y-msp) continue;
						if (lp->p.y > p.y+msp) continue;

						d2=(lp->p.x-p.x)*(lp->p.x-p.x) + (lp->p.y-p.y)*(lp->p.y-p.y);  //norm2(lp->p - p);
						if ((cc==g->lineref && d2<resolution/2) || (cc!=g->lineref && d2<ld)) {
							//lclosest=lp;
							ld=d2;
						}
					}
				}

				if (ld<ls2) {
					//g->first->AddBefore(new LinePoint(lclosest->s,lclosest->t, weight));
					//g->first=g->first->prev;
					//g->first->p=lclosest->p;
					//g->first->needtosync=0;
					g->dodir&=~2;
				}
			} //search in previous direction
			 
//			*** //if merging with an endpoint, move both endpoints to midpoint
//				//if merging against middle of line, collide into it, don't modify original line?

			if (g->dodir==0) {
				//all done with this generator! make sure the associated line starts at a point with no prev points
				if (g->first!=g->line) {
					int i=lines.findindex(g->line);
					lines.e[i]=g->first;
				}
				generators.remove(c);
			}
		} //foreach generator, merges and splits



		//*** //need to equalize somehow after merging and splitting


		 //-----no more generators, search for holes to fill! this might happen with specialized direction maps
		//add one generator per iteration
		if (generators.n==0 && maybefill) {
			//maybefill=false; //only fill in once
			DBG cerr <<" searching for empty space during iteration "<<iteration<<"..."<<endl;

			flatpoint closest;
			double d=1e+10;
			double d2, s2, s;
			LinePoint *lp;
			flatpoint p2;
			int maxline=lines.n;

			for ( ; xx<bounds.maxx; xx+=curspace) {

				if (yy>=bounds.maxy) yy=bounds.miny;
				for ( ; yy<bounds.maxy; yy+=curspace) {
					 //find closest point to xx,yy

					p=data->getPoint(xx,yy,true);
					if (spacingmap) curspace=spacingmap->GetValue(p)/data->getScaling(xx,yy,true); //else spacing is constant

					 //find square of transformed spacing
					s=curspace*2;
					s2=curspace*curspace*4;
					d=1e+10;

					for (int c=0; c<maxline; c++) {
						lp=lines.e[c];
						for ( ; lp; lp=lp->next) {
							//if (pb.boxcontains(lp->p)) {...

							if (lp->p.x<p.x-s) continue;
							if (lp->p.x>p.x+s) continue;
							if (lp->p.y<p.y-s) continue;
							if (lp->p.y>p.y+s) continue;

							d2=(lp->p.x-p.x)*(lp->p.x-p.x) + (lp->p.y-p.y)*(lp->p.y-p.y);  //norm2(lp->p - p);
							if (d2 < d) {
								closest.x=lp->s; closest.y=lp->t;
								d=d2;
							}
						}
					}

					if (d>s2) {
						 //nothing was very close
						if (weightmap)  weight = weightmap->GetValue(p); //else weight is constant
						g=new StarterPoint(flatpoint(xx,yy), 3, weight, id, generators.n);
						g->line->p=p;
						g->line->needtosync=0;
						lines.push(g->line);
						generators.push(g,1);
						if (growpoint_ret) growpoint_ret->push(new GrowPointInfo(flatpoint(xx,yy),g->dodir));
						DBG cerr <<"Add fill point at "<<xx<<','<<yy<<endl;
						yy+=curspace;
						break;
					}
				}
				if (yy<bounds.maxy) break;
			}
			if (xx>=bounds.maxx && yy>=bounds.maxy) break;
		}
	} while (generators.n);


	 //Add lines to data
	LinePoint *lp, *ll;
	for (int c=0; c<lines.n; c++) {
		lp=lines.e[c];

		 //need to normalize all points
		ll=lp;
		while (ll) {
			ll->s=(ll->s-bounds.minx)/(bounds.maxx-bounds.minx);
			ll->t=(ll->t-bounds.miny)/(bounds.maxy-bounds.miny);
			if (ll->s>=1 || ll->t>=1 || ll->s<=0 || ll->t<=0) {
				ll->p=data->getPoint(ll->s,ll->t, false);
				ll->needtosync=0;
			}
			ll=ll->next;
		}
	}
}


//------------------------------------- EngraverFillData ------------------------


/*! \class EngraverFillData
 * \ingroup interfaces
 *
 * See EngraverFillInterface.
 */


EngraverFillData::EngraverFillData()
  : PatchData(0,0,1,1,1,1,0)
{
	//usepreview=0;
	usepreview=1;

	MakeDefaultGroup();
	
	maxx=maxy=1;
}

EngraverFillData::~EngraverFillData()
{
}

/*! Override from anObject to produce a less verbose default id..
 */
const char *EngraverFillData::Id()
{
    if (object_idstr) return object_idstr;
    else object_idstr=make_id("Engraving");
    return object_idstr; 
}

const char *EngraverFillData::Id(const char *str)
{
	anObject::Id(str);

	Resource *r=dynamic_cast<Resource*>(ResourceOwner());
	if (r) {
		makestr(r->name, str);
		makestr(r->Name, str);
	}

	return anObject::Id();
}

int EngraverFillData::renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels)
{
//	// ***
//	if (maxx-minx<=0 || maxy-miny<=0) return 1;
//
//	InterfaceManager *imanager=InterfaceManager::GetDefault();
//	Displayer *dp = imanager->GetPreviewDisplayer();
//	if (dp->ResizeSurface(bufw,bufh)!=0) dp->CreateSurface(bufw,bufh);
//
//	dp->NewTransform(bufw/(maxx-minx), 0, 0, bufh/(maxy-miny), minx,miny);
//	imanager->DrawDataStraight(dp, this, NULL,NULL, 0);
//
//	dp->CopyToBuffer(buffer, bufw,bufh,bufstride,bufdepth,bufchannels, 0,0,bufw,bufh);

	return 1;
}

/*! Return nonzero for unable to render, such as image==NULL, or invalid object bounds.
 * Else return 0.
 */
int EngraverFillData::renderToBufferImage(LaxImage *image)
{
	if (!image) return 1;
	if (maxx-minx<=0 || maxy-miny<=0) return 2;
	if (image->w()<=0 || image->h()<=0) return 3;

	DBG cerr <<"^v^v^v^v^V^v^v^v^v EngraverFillData::renderToBufferImage preview image size: "<<image->w()<<" x "<<image->h()<<endl;

	InterfaceManager *imanager=InterfaceManager::GetDefault(true);
	Displayer *dp = imanager->GetPreviewDisplayer();

	int oldusepreview = usepreview;
	usepreview = 0;

	if (dp->MakeCurrent(image)!=0) return 3;
	dp->ClearTransparent();

	DBG dp->NewFG(0.0,1.0,0.0);
	//DBG dp->NewTransform(1,0,0,1,0,0);
	//DBG dp->drawline(0,image->h(), image->w(),0);
	//DBG dp->NewFG(1.0,1.0,1.0);
	//DBG dp->drawline(minx,miny, maxx,maxy);

	dp->NewTransform(image->w()/(maxx-minx), 0, 0, -image->h()/(maxy-miny), -minx*image->w()/(maxx-minx), image->h()*(1+miny/(maxy-miny)));
	//DBG dp->NewFG(1.0,0.0,1.0);
	//DBG dp->drawline(0,image->h(), image->w(),0);

	imanager->DrawDataStraight(dp, this, NULL,NULL, 0);

	//DBG dp->NewFG(1.0,0.0,1.0);
	//DBG dp->DrawScreen();
	//DBG dp->LineWidthScreen(2);
	//DBG dp->drawline(dp->realtoscreen(minx,miny), dp->realtoscreen(maxx,maxy));
	//DBG dp->drawline(dp->realtoscreen(minx,maxy), dp->realtoscreen(maxx,miny));
	//DBG dp->DrawReal();

	//DBG save_image(image, "DBG.png", "png");

	usepreview = oldusepreview;
	return 0;
}


/*! Make sure there is at least one defined group.
 * If there are already groups, then do nothing.
 */
void EngraverFillData::MakeDefaultGroup()
{
	if (groups.n) return;

	EngraverPointGroup *group=new EngraverPointGroup(this);
	group->id=0;
	group->color.red=0;
	group->color.green=0;
	group->color.blue=65535;
	group->color.alpha=65535;

	if (!group->trace)     { group->trace    =new EngraverTraceSettings(); group->trace ->SetResourceOwner(group); }
	if (!group->dashes)    { group->dashes   =new EngraverLineQuality();   group->dashes->SetResourceOwner(group); }
	if (!group->direction) { group->direction=new EngraverDirection();  group->direction->SetResourceOwner(group); }
	if (!group->spacing)   { group->spacing  =new EngraverSpacing();      group->spacing->SetResourceOwner(group); }

	group->spacing->spacing=(maxy-miny)/20;

	groups.push(group);
}

/*! Changes the name of group index which to be unique.
 *
 * Return 1 if name had to be changed, else 0.
 */
int EngraverFillData::MakeGroupNameUnique(int which)
{
	EngraverPointGroup *group=GroupFromIndex(which);

	if (!group->name) makestr(group->name,"Group");

	 //need to make a unique new name
	int c;
	int changed=0;

	do {
		for (c=0; c<groups.n; c++) {
			if (c==which) continue;
			if (!strcmp(group->name, groups.e[c]->name)) {
				char *str=increment_file(group->name);
				makestr(group->name,str);
				delete[] str;
				changed=1;
				break;
			}
		}
	} while (c!=groups.n);

	return changed;
}

/*! Check whether what in group is shared within this object.
 * If group==NULL, then use groups.e[curgroup]. Will check against all groups in the groups stack,
 * but will skip any that are from group itself.
 *
 * Returns 0 for not shared, or nonzero for is shared. When shared the return value is the index+1 of
 * the first group found to be sharing.
 */
int EngraverFillData::IsSharing(int what, EngraverPointGroup *group, int curgroup)
{
	if (!group && curgroup>=0 && curgroup<groups.n) group=groups.e[curgroup];
	if (!group) return 0;

	if (what==ENGRAVE_Tracing   && group->trace    ->ResourceOwner()!=group)  return -1;
	if (what==ENGRAVE_Dashes    && group->dashes   ->ResourceOwner()!=group)  return -1;
	if (what==ENGRAVE_Direction && group->direction->ResourceOwner()!=group)  return -1;
	if (what==ENGRAVE_Spacing   && group->spacing  ->ResourceOwner()!=group)  return -1;

	for (int c=0; c<groups.n; c++) {
		if (groups.e[c]==group) continue;

		if (what==ENGRAVE_Tracing   && groups.e[c]->trace    ==group->trace )    return c+1;
		if (what==ENGRAVE_Dashes    && groups.e[c]->dashes   ==group->dashes)    return c+1;
		if (what==ENGRAVE_Direction && groups.e[c]->direction==group->direction) return c+1;
		if (what==ENGRAVE_Spacing   && groups.e[c]->spacing  ==group->spacing  ) return c+1;
	}

	return 0;
}


/*! Return point to group corresponding to the given id. 
 * If found and err_ret!=NULL, then set to 1.
 * If not found, then return pointer to the first group, or NULL if no groups, and set err_ret to 0.
 */
EngraverPointGroup *EngraverFillData::FindGroup(int id, int *err_ret)
{
	if (err_ret) *err_ret=1;
	for (int c=0; c<groups.n; c++) {
		if (groups.e[c]->id==id) return groups.e[c];
	}
	if (err_ret) *err_ret=0;
	return groups.n ? groups.e[0] : NULL;
}

/*! Return point to group corresponding to the given index in groups, or group 0 if index out of range.
 * Thus it will ALWAYS return non-null, provided groups.n>0.
 *
 * If found and err_ret!=NULL, then set to 1.
 * If index out of range, then return pointer to the first group, or NULL if no groups, and set err_ret to 0.
 */
EngraverPointGroup *EngraverFillData::GroupFromIndex(int index, int *err_ret)
{
	if (err_ret) *err_ret=1;
	if (index>=0 && index<groups.n) return groups.e[index];

	if (err_ret) *err_ret=0;
	return groups.n ? groups.e[0] : NULL;
}

/*! Merge the specified group with the group of higher index in the group stack.
 * Only the lines are merged into the other group. All other settings are ignored.
 * The old group's name is appended to the group we merge with.
 *
 * Return 0 for success, nonzero for could not merge.
 */
int EngraverFillData::MergeDown(int which_group)
{
	if (which_group>=groups.n-1) return 1;

	EngraverPointGroup *from=groups.e[which_group];
	EngraverPointGroup *to  =groups.e[which_group+1];

	LinePoint *l;
	while (from->lines.n) {
		l=from->lines.pop(0);
		to->lines.push(l,1);
	}

	appendstr(to->name,"+");
	appendstr(to->name,from->name);

	groups.remove(which_group);
	return 0;
}

void EngraverFillData::UpdatePositionCache()
{
	for (int c=0; c<groups.n; c++) {
		groups.e[c]->UpdatePositionCache();
	}
}

/*! Set the default spacing in group 0.
 */
double EngraverFillData::DefaultSpacing(double nspacing)
{
	double oldspacing=groups.e[0]->spacing->spacing;
	groups.e[0]->spacing->spacing=nspacing;
	return oldspacing;
}

SomeData *EngraverFillData::duplicateData(SomeData *dup)
{
	EngraverFillData *p=dynamic_cast<EngraverFillData*>(dup);
	if (!p && !dup) return NULL; //was not EngraverFillData!

	//char set=1;
	if (!dup) {
		//dup=somedatafactory->newObject(LAX_ENGRAVERFILLDATA,this);
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject("EngraverFillData"));
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			//set=0;
			p=dynamic_cast<EngraverFillData*>(dup);
		}
	} 
	if (!p) {
		p=new EngraverFillData();
		dup=p;
	}

	p->groups.flush();
	p->NeedToUpdateCache(0,0,-1,-1);

	for (int c=0; c<groups.n; c++) {
		EngraverPointGroup *group=new EngraverPointGroup(this);
		group->CopyFrom(groups.e[c], false, false, false, false, false);
		makestr(group->name, groups.e[c]->name);
		p->groups.push(group);
	}

	return PatchData::duplicateData(dup);
}


/*! Make lines->p be the transformed s,t.
 */
void EngraverFillData::Sync(bool asneeded)
{
	LinePoint *l;
	EngraverPointGroup *group;

	for (int g=0; g<groups.n; g++) {
		group=groups.e[g];

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];

			while (l) {
				if (!asneeded || (asneeded && l->needtosync==1))
					l->p=getPoint(l->s,l->t, false); // *** note this is hideously inefficient, matrices are not cached with getPoint!!!
				l->needtosync=0;

				l=l->next;
			}
		}
	}
}

/*! Assume lines->p are accurate, and we need to map back to s,t mesh coordinates.
 */
void EngraverFillData::ReverseSync(bool asneeded)
{
	LinePoint *l;
	flatpoint pp;
	int in=0;
	EngraverPointGroup *group;

	for (int g=0; g<groups.n; g++) {
		group=groups.e[g];

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];

			while (l) {
				if (!asneeded || (asneeded && l->needtosync==2)) {
					pp=getPointReverse(l->p.x,l->p.y, &in); // *** note this is hideously inefficient
					if (in) {
						l->s=pp.x;
						l->t=pp.y;
						l->needtosync=0;
					}
				}

				l=l->next;
			}
		}
	}
}


/*! This is basically a flush of all points groups, and make default group be linear.
 *
 * spacing is an object distance (not in s,t space) to be used as the distance between line centers.
 * If spacing<0, then use 1/25 of the x or y dimension, whichever is smaller
 */
void EngraverFillData::FillRegularLines(double weight, double spacing)
{
	if (spacing<=0) {
		if (maxx-minx<maxy-miny) spacing=(maxx-minx)/20;
		else spacing=(maxy-miny)/20;
	}

	while (groups.n>1) groups.remove(-1);
	DefaultSpacing(spacing);

	groups.e[0]->direction->type=PGROUP_Linear;
	groups.e[0]->Fill(this, -1);
	Sync(false);
}

void EngraverFillData::Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	PatchData::Set(xx,yy,ww,hh,nr,nc,stle);
	NeedToUpdateCache(0,0,-1,-1);
}

/*! \ingroup interfaces
 * Dump out an EngraverFillData.
 *
 * Something like:
 * <pre>
 *  filename blah.jpg
 *  iwidth 100
 *  iheight 100
 *  matrix 1 0 0 1 0 0
 *  xsize 4
 *  ysize 4
 *  points \
 *    1 2
 *    3 4
 *    5 6
 * </pre>
 * 
 * \todo *** assumes data is from filename. It shouldn't. It might be random buffer information.
 *
 * Calls PatchData::dump_out(f,indent,0,context), then puts out filename, and the
 * pixel dimensions of the image in filename.
 * If what==-1, then output a pseudocode mockup of the format. Otherwise
 * output the format as above.
 */
void EngraverFillData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 

	if (what==-1) {
		fprintf(f,"%smesh             #The mesh used to encase the lines\n",spc);
		PatchData::dump_out(f,indent+2,-1,context);

		fprintf(f,"%sgroup            #(1 or more)\n",spc);
		groups.e[0]->dump_out(f,indent+2,-1,context);

		return;
	}

	fprintf(f, "%spreview_size %d\n", spc, preview_size);
	fprintf(f, "%smesh\n", spc);
	PatchData::dump_out(f,indent+2,what,context);

	for (int c=0; c<groups.n; c++) {

		fprintf(f,"%sgroup\n",spc);
		groups.e[c]->dump_out(f,indent+2,what,context);
	}


	return;
}

Laxkit::Attribute *EngraverFillData::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att = new Attribute;
	Attribute *att2;

	if (what==-1) {
		att2 = att->pushSubAtt("mesh", nullptr, "The mesh used to encase the lines");
		PatchData::dump_out_atts(att2, -1, context);

		att2 = att->pushSubAtt("group", nullptr, "(1 or more)");
		groups.e[0]->dump_out_atts(att2, -1,context);

		return att;
	}

	att->push("preview_size", preview_size);

	att2 = att->pushSubAtt("mesh");
	PatchData::dump_out_atts(att2, what,context);

	for (int c=0; c<groups.n; c++) {
		att2 = att->pushSubAtt("group");
		groups.e[c]->dump_out_atts(att2,what,context);
	}

	return att;
}

//! Reverse of dump_out.
void EngraverFillData::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	char *name;
	char *value;
	int c;

	bool groups_flushed=false;

	for (c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name,"preview_size")) {
			int i = 0;
			IntAttribute(value, &i);
			if (i > 0 && i < 2048) preview_size = i;

		} else if (!strcmp(name,"mesh")) {
			PatchData::dump_in_atts(att->attributes.e[c],flag,context);

		} else if (!strcmp(name,"group")) {
			if (groups.n && !groups_flushed) {
				 //only flush groups when there are actual groups being read in
				groups.flush();
				groups_flushed=true;
			}
			EngraverPointGroup *group=new EngraverPointGroup(this);
			group->dump_in_atts(att->attributes.e[c],flag,context);
			groups.push(group);

		}
	}

	 //now we need to account for block sharing....
	char *s,*e;
	char type=0; //'t', 'd', 'D', 's'
	for (int c=0; c<groups.n; c++) {
		if (isblank(groups.e[c]->iorefs)) continue;

		s=groups.e[c]->iorefs+1;
		while (*s) {
			if (!strncmp(s,"trace:",6)) { type='t'; s+=6; }
			else if (!strncmp(s,"dash:",5)) { type='d'; s+=5; }
			else if (!strncmp(s,"direction:",10)) { type='D'; s+=10; }
			else if (!strncmp(s,"spacing:",8)) { type='s'; s+=8; }
			else type=0;

			e=strchr(s,'|');
			if (!e) e=s+strlen(s);

			if (type!=0) {
				for (int c2=0; c2<groups.n; c2++) {
					if (c2==c) continue;
					if (!strncmp(groups.e[c2]->name,s,e-s+1)) {
						if (type=='t') {
							if (groups.e[c2]->trace->ResourceOwner()==groups.e[c2]) {
								 //need to convert trace to a resource
								InterfaceManager::GetDefault(true)->Resourcify(groups.e[c2]->trace);
							}
							groups.e[c]->InstallTraceSettings(groups.e[c2]->trace, 0);

						} else if (type=='d') {
							if (groups.e[c2]->dashes->ResourceOwner()==groups.e[c2]) {
								InterfaceManager::GetDefault(true)->Resourcify(groups.e[c2]->dashes);
							}
							groups.e[c]->InstallDashes(groups.e[c2]->dashes, 0);

						} else if (type=='D') {
							if (groups.e[c2]->direction->ResourceOwner()==groups.e[c2]) {
								InterfaceManager::GetDefault(true)->Resourcify(groups.e[c2]->direction);
							}
							groups.e[c]->InstallDirection(groups.e[c2]->direction,0);

						} else if (type=='s') {
							if (groups.e[c2]->spacing->ResourceOwner()==groups.e[c2]) {
								InterfaceManager::GetDefault(true)->Resourcify(groups.e[c2]->spacing);
							}
							groups.e[c]->InstallSpacing(groups.e[c2]->spacing,0);
						}
					} 
				}
			}
			s=e;
			if (*s=='|') s++;
		}
		
		delete[] groups.e[c]->iorefs;
		groups.e[c]->iorefs=NULL;
	}

	FindBBox();

	for (int c=0; c<groups.n; c++) {
		if (groups.e[c]->needtoreline) {
			groups.e[c]->Fill(this, -1);
			groups.e[c]->needtoreline=false;
		}
	}
	Sync(false);

	for (int c=0; c<groups.n; c++) {
		groups.e[c]->UpdateBezCache();

		if (groups.e[c]->needtotrace) {
			// *** need to clarify this: group.e[c]->Trace(aa);
		}

		groups.e[c]->UpdateDashCache();
	} 
}

/*! Based on groups' needtoreline, needtotrace, and needtodash, update.
 * needtoreline implies needtotrace and needtodash.
 * needtotrace implies needtodash.
 */
void EngraverFillData::Update()
{
	bool changed = false;

	for (int c=0; c<groups.n; c++) {
		EngraverPointGroup *group = groups.e[c];

		if (group->needtoreline) {
			if (group->direction->grow_lines) {
				if (!group->grow_cache) {
					//initialize growing lines.. this will start iterating growth over many frames
					group->growpoints.flush();
					group->GrowLines_Init(this,
								 group->spacing->spacing/3,
								 group->spacing->spacing, NULL,
								 .01, NULL,
								 group->directionv,group,
								 1000, //iteration limit
								 &group->growpoints
								);
				} else {
					if (!group->GrowLines_Iterate()) {
						group->GrowLines_Finish();
						group->needtoreline = false;
					}
				}
			} else {
				group->Fill(this, -1);
				group->needtoreline = false;
			}

			group->UpdateBezCache();

			if (group->trace->continuous_trace) group->needtotrace =true;
			group->needtodash  =true;

			changed=true;
		}

		if (group->needtotrace) {
			if (group->active && group->trace && group->trace->traceobject) { 
				Affine aa = GetTransformToContext(false, 0);//supposed to be from obj to same "parent" as traceobject
				group->Trace(&aa); //NOTE: this forces a trace when needtotrace!!
				changed=true;
			}

			group->needtotrace =false;
			group->needtodash  =true;
		}

		if (group->needtodash) {
			group->UpdateDashCache();
			group->needtodash=false;
			changed=true;
		}

	} 

	if (changed) touchContents();
}

/*! Return whether a point is considered on by the criteria of the settings in dashes.
 * Default is point must be on, and weight>=zero_threshhold.
 */
int EngraverFillData::PointOn(LinePoint *p, EngraverPointGroup *g)
{
	if (!p->on) return 0; 
	if (!groups.n) return 0;
	if (!g) g=groups.e[0]; 
	return g->PointOn(p);
}

/*! Creates a single PathsData as outline.
 * If whichgroup<0, then pack all groups. But be warned doing this might not respect separate colors
 */
PathsData *EngraverFillData::MakePathsData(int whichgroup)
{
	PathsData *paths=NULL;
    //if (somedatafactory) paths=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA,NULL));
    paths=dynamic_cast<PathsData*>(somedatafactory()->NewObject("PathsData",NULL));
    if (!paths) paths=new PathsData();
	paths->m(m());

	paths->line(0,-1,-1,&groups.e[0]->color); //set default paths color, maybe overridden by individual paths
	paths->fill(&groups.e[0]->color); //set default paths color, maybe overridden by individual paths
	// *** should we make it ordered so that all lines from same group are next to each other in paths??

	//currently, makes a PathsData with the outline of all the strokes...

	//Todo: does not currently handly 0 weight segments properly

	NumStack<flatvector> points;
	NumStack<flatvector> points2;

	LinePoint *l;
	LinePointCache *lc, *lcstart;
	flatvector t, tp;
	flatvector p1,p2;
	EngraverPointGroup *group;

	int s=0, e=groups.n;
	if (whichgroup>=0 && whichgroup<groups.n) { s=e=whichgroup; }

	for (int g=s; g<=e; g++) {
		group=groups.e[g];

		if (!group->active) continue;

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];
			lc=l->cache;
			lcstart=lc;

			 //make points be a list of points:
			 //   2  4  6 
			 // 1 *--*--*--8   gets rearranged to: 1 2 4 6 8 3 5 7
			 //   3  5  7
			 //points 1 and 8 are cap point references, converted to rounded ends later
			do { //one loop per connected segment
				if (!group->PointOnDash(lc)) { lc=lc->next; continue; } //skip off points

				 //get tangent vector at lc
				if (lc->original!=NULL) {
					l=lc->original;
					if (l->next) tp=bez_visual_tangent(0, l->p,l->bez_after,l->next->bez_before,l->next->p);
					else if (l->prev) tp=bez_visual_tangent(1, l->prev->p,l->prev->bez_after,l->bez_before,l->p);
					else tp=flatpoint(1,0);

				} else {
					l=lc->PrevOriginal();
					tp=bez_visual_tangent(lc->bt, l->p,l->bez_after,l->next->bez_before,l->next->p);
				}

				tp.normalize(); 
				t=transpose(tp);

				if (points.n==0) {
					 //add a first point cap
					points.push(lc->p - lc->weight/2*tp);
				}

				 //add top and bottom points for l
				p1=lc->p + lc->weight/2*t;
				p2=lc->p - lc->weight/2*t;

				points.push(p1);
				points.push(p2);


				if (!lc->next || !group->PointOnDash(lc->next) || lc->dashon==ENGRAVE_EndPoint) {
					 //need to add a path!
		
					 //add last cap
					tp=points.e[points.n-1]-points.e[points.n-2];
					tp.normalize();
					tp=transpose(tp);
					points.push(lc->p + lc->weight/2*tp);


					 //convert to bez approximation
					 //make points2 be points rearranged according to outline
					points2.push(points.e[0]); //initial cap
					for (int c2=1; c2<points.n-1; c2+=2)    points2.push(points.e[c2]);

					points2.push(points.e[points.n-1]); //final cap
					for (int c2=points.n-2; c2>0; c2-=2) points2.push(points.e[c2]);
					

					BezApproximate(points,points2);

					paths->moveTo(points.e[1]);
					for (int c2=1; c2<points.n; c2+=3) {
						paths->curveTo(points.e[c2+1], points.e[(c2+2)%points.n], points.e[(c2+3)%points.n]);
					}

					paths->close();

					points.flush_n();
					points2.flush_n();
				}

				lc=lc->next;
			} while (lc && lc!=lcstart);

		}
	}

	 //finally, install color for this line
	//paths->paths.e[paths->paths.n-1]->LineColor(&group->color);
	paths->fill(&group->color);
	paths->fillstyle->fillrule=LAXFILL_Nonzero;
	paths->line(0,-1,-1,&group->color);
	paths->linestyle->function=LAXOP_None;

	return paths;
}

void EngraverFillData::dump_out_svg(const char *file)
{
	FILE *f=fopen(file,"w");
	if (!f) return;

	setlocale(LC_ALL,"C");

	//powerstroke path effect goes in defs, BUT the outline is in the path->d,
	//the original path is in an attribute of the path...
	

	fprintf(f,  "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
				"<!-- Created with Inkscape (http://www.inkscape.org/) -->\n"
				"\n"
				"<svg\n"
				"   xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
				"   xmlns:cc=\"http://creativecommons.org/ns#\"\n"
				"   xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n"
				"   xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
				"   xmlns=\"http://www.w3.org/2000/svg\"\n"
				"   xmlns:sodipodi=\"http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd\"\n"
				"   xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n"
				//"   width=\"%.10g\"\n"
				//"   height=\"%.10g\"\n"
				"   width=\"1000\"\n"
				"   height=\"1000\"\n"
				"   id=\"svg2\"\n"
				"   version=\"1.1\"\n"
				"   inkscape:version=\"0.48+devel r custom\"\n"
				"   viewBox=\"0 0 1000 1000\"\n"
				"   sodipodi:docname=\"test-powerstroke.svg\">\n"
				"  <defs id=\"defs4\">\n" 
					//1.1*(maxx-minx), 1.1*(maxy-miny)
			);


	// *** output path effect defs
	//    <inkscape:path-effect
	//       effect="powerstroke"
	//       id="path-effect4064"
	//       is_visible="true"
	//       offset_points="0,1 | 3.0017209,-52.700851 | 2.2287256,-40.548946 | 5,-2.3889435"
	//       sort_points="true"
	//       interpolator_type="CubicBezierJohan"
	//       interpolator_beta="0.2"
	//       start_linecap_type="round"
	//       linejoin_type="round"
	//       miter_limit="4"
	//       end_linecap_type="round" />


	fprintf(f,  "  </defs>\n");
	fprintf(f,  " <g\n"
				"     inkscape:label=\"Layer 1\"\n"
				"     inkscape:groupmode=\"layer\"\n"
				"     id=\"layer1\">\n");
	double s;
	if (maxx-minx>maxy-miny) s=900/(maxx-minx);
	else s=900/(maxy-miny);
	double neww=s*(maxx-minx);
	double newh=s*(maxy-miny);

	fprintf(f,  "  <g id=\"themesh\" "
	                 //"transform=\"matrix(1 0 0 -1 0 0)\" \n   >"
	                 "transform=\"matrix(%.10g 0 0 -%.10g %.10g %.10g)\" \n   >",
						s,s,
						//-(500-neww/2),miny*s+(500+newh/2) //-m(4),-m(5)
						(500-neww/2)-minx*s,miny*s+(500+newh/2) //-m(4),-m(5)
		         );
 
	// *** output paths
	//  <path d="....the full outline....."
	//        inkscape:path-effect="path-effect4064"
	//        inkscape:original-d="....original path...."

	NumStack<flatvector> points;
	NumStack<flatvector> points2;

	LinePoint *l;
	LinePointCache *lc, *lcstart;
	flatvector t, tp;
	flatvector p1,p2;
	EngraverPointGroup *group;

	char stylestr[150];
	

	for (int g=0; g<groups.n; g++) {
		group=groups.e[g];

		sprintf(stylestr," style=\"fill:#%02x%02x%02x; fill-opacity:%.10g; stroke:none; stroke-opacity:%.10g;\" ",
						group->color.red>>8,group->color.green>>8,group->color.blue>>8,
						group->color.alpha/65535.,
						group->color.alpha/65535.
						);


		if (!group->active) continue;

		fprintf(f,"  <g>\n"); //encase each point group in its own svg group

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];
			lc=lcstart=l->cache;


			 //make points be a list of points:
			 //   2  4  6 
			 // 1 *--*--*--8   gets rearranged to: 1 2 4 6 8 3 5 7
			 //   3  5  7
			 //points 1 and 8 are cap point references, converted to rounded ends later
			do { //one loop per on segment
				if (!group->PointOnDash(lc)) { lc=lc->next; continue; }

				 //get tangent vector at lc
				if (lc->original!=NULL) {
					l=lc->original;
					if (l->next) tp=bez_visual_tangent(0, l->p,l->bez_after,l->next->bez_before,l->next->p);
					else if (l->prev) tp=bez_visual_tangent(1, l->prev->p,l->prev->bez_after,l->bez_before,l->p);
					else tp=flatpoint(1,0);

				} else {
					l=lc->PrevOriginal();
					tp=bez_visual_tangent(lc->bt, l->p,l->bez_after,l->next->bez_before,l->next->p);
				}

				tp.normalize(); 
				t=transpose(tp);

				if (points.n==0) {
					 //add a first point cap
					points.push(lc->p - lc->weight/2*tp);
				}

				 //add top and bottom points for lc
				p1=lc->p + lc->weight/2*t;
				p2=lc->p - lc->weight/2*t;

				points.push(p1);
				points.push(p2);

				if (!lc->next || !group->PointOnDash(lc->next) || lc->dashon==ENGRAVE_EndPoint) {
					 //end of segment, need to add a path to file!
		
					 //add last cap
					tp=points.e[points.n-1]-points.e[points.n-2];
					tp.normalize();
					tp=transpose(tp);
					points.push(lc->p + lc->weight/2*tp);


					 //convert to bez approximation
					 //make points2 be points rearranged according to outline
					points2.push(points.e[0]); //initial cap
					for (int c2=1; c2<points.n-1; c2+=2)    points2.push(points.e[c2]);

					points2.push(points.e[points.n-1]); //final cap
					for (int c2=points.n-2; c2>0; c2-=2) points2.push(points.e[c2]);
					

					BezApproximate(points,points2);

					fprintf(f,"    <path %s d=\"", stylestr);
					fprintf(f,"M %f %f ", points.e[1].x,points.e[1].y);
					for (int c2=1; c2<points.n; c2+=3) {
						fprintf(f,"C %f %f %f %f %f %f ",
							points.e[c2+1].x,points.e[c2+1].y,
							points.e[(c2+2)%points.n].x,points.e[(c2+2)%points.n].y,
							points.e[(c2+3)%points.n].x,points.e[(c2+3)%points.n].y);
					}


					fprintf(f,"z \" />\n");

					points.flush();
					points2.flush();
				}

				lc=lc->next;
			} while (lc && lc!=lcstart);
		} //foreach line

		fprintf(f,"  </g>\n");
	} //foreach group


	fprintf(f,  "  </g>\n"
				" </g>\n"
				"</svg>\n");

	setlocale(LC_ALL,"");
	fclose(f);
}

/*! Add more sample points between all existing points.
 *
 * Approximate each line with a bezier curve, and grab the center of each segment.
 *
 * Afterwards, you will need to call ReverseSync(true).
 *
 * group is an index into groups, or -1 for default group.
 */
void EngraverFillData::MorePoints(int curgroup)
{
	NumStack<flatpoint> pts;
	flatpoint *bez=NULL;
	int n,i;
	LinePoint *ll;
	LinePoint *l;

	EngraverPointGroup *group;
	if (curgroup<0 || curgroup>=groups.n) group=groups.e[0];
	else group=groups.e[curgroup];

	for (int c=0; c<group->lines.n; c++) {
		l=group->lines.e[c];
		n=0;

		pts.flush();
		while (l) { pts.push(l->p); l=l->next; }

		if (3*pts.n>n) {
			if (bez) delete[] bez;
			bez=new flatpoint[3*pts.n];
			n=3*pts.n;
		}
		bez_from_points(bez, pts.e,pts.n);

		i=1;
		l=group->lines.e[c];
		while (l) {
			if (!l->next) break;
	
			ll=new LinePoint();
			ll->weight=(l->weight+l->next->weight)/2;
			ll->spacing=(l->spacing+l->next->spacing)/2;
			ll->s=(l->s+l->next->s)/2;
			ll->t=(l->t+l->next->t)/2; //just in case reverse map doesn't work
			ll->p=bez_point(.5, l->p, bez[i+1], bez[i+2], l->next->p);

			if (l->on==ENGRAVE_EndPoint) l->on=ENGRAVE_Off;
			else ll->on=(l->on || l->next->on ? ENGRAVE_On : ENGRAVE_Off);

			ll->next=l->next;
			l->next->prev=ll;
			l->next=ll;
			ll->prev=l;
			ll->needtosync=2;

			i+=3;
			l=l->next->next;
		}
	}
}

/*! points is a special list of sample points, meaning points that lie on the line.
 * This makes fauxpoints be a bezier list: c-p-c-c-p-c-...-c-p-c smoothly connecting all the points.
 *
 * points is assumed to be a list of top points, then of matched bottom points, so
 * points[0] is on top top points[points.n/2], and so on.
 * Currently only round caps are applied.
 */
void EngraverFillData::BezApproximate(Laxkit::NumStack<flatvector> &fauxpoints, Laxkit::NumStack<flatvector> &points)
{
	// There are surely better ways to do this. Not sure how powerstroke does it.
	// This is not simplied/optimized at all. Each point gets control points to smooth it out.
	// no fancy corner handling done yet

    fauxpoints.flush();

    flatvector v,p, pp,pn;
	flatvector opn, opp;


    double sx;
	//caps are at points index 0 and points.n/2
	
    for (int c=0; c<points.n; c++) {
        p=points.e[c];

		if (c==0) {
			 //on first cap, apply round
			opp=p+3*.5522*(points.e[points.n-1]-points.e[1])/2;
			opn=p+3*.5522*(points.e[1]-points.e[points.n-1])/2;

		} else if (c==points.n/2) {
			 //on final cap, apply round
			opp=p+3*.5522*(points.e[c-1]-points.e[c+1])/2;
			opn=p+3*.5522*(points.e[c+1]-points.e[c-1])/2;

		} else {
			if (c==1 || c==points.n/2+1) {
				pp=(p+points.e[points.n-c])/2;
				v=points.e[c-1]-pp;
				opp=p+3*.5522*v;
			} else opp=points.e[c-1];

			if (c==points.n-1 || c==points.n/2-1) {
				pp=(p+points.e[points.n-c])/2;
				v=points.e[(c+1)%points.n]-pp;
				opn=p+3*.5522*v;
			} else opn=points.e[c+1];
		}

        v=opn-opp;
        v.normalize();

        sx=norm(p-opp)*.333;
        pp=p - v*sx;

        sx=norm(opn-p)*.333;
        pn=p + v*sx;

        fauxpoints.push(pp);
        fauxpoints.push(p);
        fauxpoints.push(pn);

    }
}




} // namespace LaxInterfaces

