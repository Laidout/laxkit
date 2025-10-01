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
//    Copyright (C) 2004-2006,2010,2012-2013 by Tom Lechner
//

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/somedataref.h>
#include <lax/strmanip.h>
#include <lax/transformmath.h>
#include <lax/misc.h>
#include <lax/language.h>


using namespace Laxkit;


namespace LaxInterfaces {

/*! \class SomeDataRef
 * \brief A reference to a SomeData.
 *
 * This class is useful for having clones of objects.
 *  
 * The matrix in SomeDataRef is meant to overwrite
 * the matrix of thedata. This is unlike a group with 1 object
 * in that with groups, the matrices are multiplied.
 */


SomeDataRef::SomeDataRef()
{ 
	thedata=NULL;
	thedata_id=NULL;
	clone_group=0;
}

/*! Increments count of d.
 */
SomeDataRef::SomeDataRef(SomeData *d)
{ 
	clone_group = 0;
	thedata = d;
	thedata_id = (d ? newstr(d->Id()) : NULL);
	if (thedata) {
		m(thedata->m());
		d->inc_count();
		FindBBox();
	}
}

/*! Decrement count of thedata, if any.
 */
SomeDataRef::~SomeDataRef()
{
	if (thedata_id) delete[] thedata_id;
	if (thedata) thedata->dec_count();
}

/*! Replace current object with d.
 * If ignore_matrix, then do not change our matrix,
 * otherwise copy matrix from d.
 */
int SomeDataRef::Set(SomeData *d, int ignore_matrix)
{
	if (d == thedata) {
		return 0;
	}
	if (thedata) {
		thedata->dec_count();
		makestr(thedata_id, NULL);
	}
	thedata = d;
	if (thedata) {
		thedata_id = newstr(d->Id());
		if (!ignore_matrix) m(thedata->m());
		d->inc_count();
		FindBBox();
		modtime = thedata->modtime;
	}
	return 0;
}

/*! Return an actual object at the end, if we are pointing to another clone.
 * If final object is an empty ref, then return NULL.
 */
SomeData *SomeDataRef::GetFinalObject()
{
	if (!thedata) return NULL;
	SomeData *obj=thedata;

	while (dynamic_cast<SomeDataRef*>(obj)) {
		obj=dynamic_cast<SomeDataRef*>(obj)->GetObject();
	}

	return obj;
}

void SomeDataRef::FindBBox()
{
	if (!thedata) return;
	if (modtime > thedata->modtime) return;

	// if (thedata->istype("SomeDataRef")) thedata->FindBBox();
	thedata->FindBBox();

	maxx = thedata->maxx;
	maxy = thedata->maxy;
	minx = thedata->minx;
	miny = thedata->miny;
	modtime = thedata->modtime;
}

SomeData *SomeDataRef::duplicateData(SomeData *dup)
{
	if (dup) {
		if (!dynamic_cast<SomeDataRef*>(dup)) return NULL;
	} else dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_SOMEDATAREF,this));

	SomeDataRef *ref=dynamic_cast<SomeDataRef*>(dup);

	ref->Set(thedata,0);
	ref->setbounds(minx,maxx,miny,maxy);
	ref->m(m());
	return dup;
}

void SomeDataRef::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	if (what==-1) {
		fprintf(f,"%sreference objectid  #a string id of a drawable object\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #overriding matrix\n",spc);
		fprintf(f,"%sminx 0 #the min/max are just hints, actual object will overwrite\n",spc);
		fprintf(f,"%smaxx 1\n",spc);
		fprintf(f,"%sminy 0\n",spc);
		fprintf(f,"%smaxy 1\n",spc);
		return;
	}
	const char *id= thedata ? thedata->Id() : NULL;
	if (!id) id=thedata_id;

	if (id) fprintf(f,"%sreference %s\n",spc, id);
	const double *matrix=m();
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	fprintf(f,"%sminx %.10g\n",spc,minx);
	fprintf(f,"%smaxx %.10g\n",spc,maxx);
	fprintf(f,"%sminy %.10g\n",spc,miny);
	fprintf(f,"%smaxy %.10g\n",spc,maxy);
}

Laxkit::Attribute *SomeDataRef::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att=new Attribute("SomeDataRef",NULL);

	if (what==-1) {
		att->push("matrix", _("An affine matrix of 6 numbers"));
		att->push("reference", _("String id of the object referred to"));
		return att;
	}

	const char *id= thedata ? thedata->Id() : NULL;
	if (id) att->push("reference",id);

	const double *matrix=m();
	char s[120];
	sprintf(s,"%.10g %.10g %.10g %.10g %.10g %.10g",
			matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	att->push("matrix",s);

	return att;
}

/*! Only read in the id. It is up to the program reading in to substitute references for actual objects.
 */
void SomeDataRef::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	SomeData::dump_in_atts(att,flag,context);

	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"reference")) {
			makestr(thedata_id, value);
		}
	}
	FindBBox();
}

int SomeDataRef::pointin(Laxkit::flatpoint pp,int pin)
{
	if (thedata) {
		pp=transform_point_inverse(m(),pp);
		pp=transform_point(thedata->m(),pp);
		return thedata->pointin(pp,pin);
	}
	return SomeData::pointin(pp,pin);
}


} //namespace LaxInterfaces;

