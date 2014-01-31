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


#include <lax/interfaces/somedatafactory.h>

#define _LAX_SOMEDATA_SRC
#include <lax/interfaces/somedata.h>
#undef _LAX_SOMEDATA_SRC

#include <lax/transformmath.h>
#include <lax/misc.h>
#include <lax/strmanip.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {



//------------------------------ SomeData ---------------------------------

/*! \class SomeData
 * \brief Basic data type for interfaces.
 *
 * Defines a 2 dimensional basis, plus min and max bounds.
 * These bounds are in the object's own space, not transformed by matrix.
 * For instance, (actual x,y)=(object x,y)*transform, where:
 * <pre>
 * transform = [ m[0] m[2] 0 ],  and points are [ x ]
 *             [ m[1] m[3] 0 ]                  [ y ]
 *             [ m[4] m[5] 1 ]                  [ 1 ]
 * </pre>
 * 
 * \todo ***???make scalex(),scaley(),angle(),shear(),translatex(),translatey()? that would allow objects to
 * respond when a change in the transformation takes place. There is only limited support for that
 * kind of thing now: it can respond when single elements of the matrix are modified 
 * via m(int,double).
 * 
 */
/*! \var Laxkit::LaxImage *SomeData::preview
 * \brief A preview image potentially to be used to not have to rerender on each refresh.
 *
 * This is an image that scales to the bounds of the data without skewing. Thus, matrix
 * is not applied before rendered. Usually the Displayer will apply any additional transform
 * before compositing to its surface.
 */
/*! \var int SomeData::usepreview
 * \brief Flag for whether to use SomeData::preview rather than rendering.
 */
/*! \var time_t SomeData::previewtime
 * \brief The time the preview was generated, if at all, as returned by time().
 *
 * Beware that this is a time in seconds.
 */
/*! \var time_t SomeData::modtime
 * \brief The time of last modification through a SomeData function.
 *
 * Beware that this is a time in seconds.
 * Please note that of course if you bypass the SomeData functions, then the modtime will
 * not be updated.
 */
/*! \var int SomeData::iid
 * \brief Id of the interface that should handle this data.
 *
 * This is only a hint. Nothing in Laxkit depends on it.
 */
/*! \fn void SomeData::FindBBox()
 * \brief Theoretically, find and set the bounds of the data.
 *
 * As this usually requires further knowledge of the particular interface
 * used, this function is perhaps not very useful...
 */


//typedef void (*SomeDataDumpFunc)(SomeData *d,FILE *f,int indent);

/*! Object is created with a count of 1.
 */
SomeData::SomeData()
{
	//***if (defaultNewSomeDataFunc!=NULL) defaultNewSomeDataFunc(this);
		
	preview=NULL;
	usepreview=0;
	previewtime=0;
	modtime=0;

	minx=maxx=miny=maxy=0;
	bboxstyle=0;
	flags=0;
	iid=0;
	nameid=NULL;

	DBG cerr <<"SomeData "<<object_id<<" created, count=1"<<endl;
}

//! Constructor, setting the bounds.
/*! Object is created with a count of 1.
 */
SomeData::SomeData(double nminx,double nmaxx,double nminy,double nmaxy)
{
	//***if (defaultNewSomeDataFunc!=NULL) defaultNewSomeDataFunc(this);
		
	preview=NULL;
	usepreview=0;
	previewtime=0;
	modtime=0;

	setbounds(nminx,nmaxx,nminy,nmaxy);
	bboxstyle=0;
	iid=0;
	flags=0;
	nameid=NULL;

	DBG cerr <<"SomeData "<<object_id<<" created, count=1"<<endl;
}

SomeData::~SomeData()
{
	if (preview) preview->dec_count();
	if (nameid) delete[] nameid;
}

/*! If id is blank, then try to create a unique one.
 */
const char *SomeData::Id()
{
    if (!nameid) {
		if (object_idstr) makestr(nameid,object_idstr);
		else {
			nameid=make_id(whattype());
			makestr(object_idstr,nameid);
		}
	}
    return nameid; 
}

const char *SomeData::Id(const char *newid)
{   
    makestr(nameid,newid);
	makestr(object_idstr,newid);
	return nameid;
} 

//! Render the object to a buffer.
/*! This draws onto buffer such that the object's whole bounding box maps to the whole buffer.
 *
 * This is mainly used to create screen previews in ARGB 8 bit format. Maybe someday it will be
 * more powerful!!
 * 
 * Default here is just a do nothing placeholder, and only returns 1.
 * On success, it should return 0.
 *
 * \todo perhaps create a version where one may apply a transform before writing to buffer, then
 *   blend onto it? ...maybe not, better off using some kind of Displayer for that
 */
int SomeData::renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels)
{
	return 1;
}

//! Create a preview image with transparency for a cached screen preview.
/*! This will return immediately if usepreview!=1 or the bounds are not valid.
 *  If w or h are less than or equal to 0, then use default or previous values for them.
 *  The default is to fit within a box 200x200, with aspect approximately the same
 *  as the object's bounding box.
 *
 *  Subclasses need not redefine this function. They need only redefine renderToBuffer().
 */
void SomeData::GeneratePreview(int w,int h)
{
	if (usepreview!=1 || maxx<minx || maxy<miny) return;

	DBG cerr <<"...SomeData::GeneratePreview()"<<endl;

	if (preview) {
		if (w<=0 && h<=0) {
			w=preview->w();
			h=preview->h();
		} 
		 //make the desired preview dimensions approximately sympatico with
		 //the actual dimensions of the object
		if ((w>h && (maxx-minx)<(maxy-miny))) h=0;
		else if ((w<h && (maxx-minx)>(maxy-miny))) w=0;
	}
	if (preview && (w!=preview->w() || h!=preview->h())) {
		 //delete old preview and make new only when changing size of preview
		preview->dec_count(); 
		preview=NULL;
		DBG cerr <<"removed old preview..."<<endl;
	}

	if (w<=0 && h<=0) { if (maxx-minx>maxy-miny) w=200; else h=200; }
	if (w<=0 && h>0) w=(maxx-minx)*h/(maxy-miny);
	else if (w>0 && h<=0) h=(maxy-miny)*w/(maxx-minx);
	if (w==0) w=1;
	if (h==0) h=1;

	unsigned char *buffer=NULL;
	if (!preview) {
		DBG cerr <<"old preview didn't exist, so creating new one..."<<endl;
		preview=create_new_image(w,h);
	} 
	buffer=preview->getImageBuffer();

	renderToBuffer(buffer,w,h,w*4,8,4);
	previewtime=time(NULL); 

	preview->doneWithBuffer(buffer);
}

void SomeData::touchContents()
{ 
	previewtime=0; //time() doesn't change often enough, so we have to force this to 0..
	modtime=time(NULL); 
}

//! If usepreview==1 and preview, then return preview.
/*! If previewtime>modtime, then call GeneratePreview(-1,-1) before returning preview.
 */
Laxkit::LaxImage *SomeData::GetPreview()
{
	if (usepreview==1) {
		if (previewtime<modtime || !preview) GeneratePreview(-1,-1);
		return preview;
	}
	return NULL;
}

//! Dump in an attribute, then call dump_in_atts(thatatt,0).
/*! If Att!=NULL, then return the attribute used to read in the stuff.
 * This allows holding classes to have extra attributes within the spot field to
 * exist and not be discarded.
 *
 * ***perhaps?: The fields actually used are removed from the attribute?
 */
void SomeData::dump_in(FILE *f,int indent,Laxkit::anObject *context, Attribute **Att)
{
	Attribute *att=new Attribute;
	att->dump_in(f,indent);
	dump_in_atts(att,0,context);
	if (Att) *Att=att;
	else delete att;
}

//! Reverse of dump_out().
void SomeData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	double mm[6];
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			DoubleListAttribute(value,mm,6);
			m(mm);
		} else if (!strcmp(name,"minx")) {
			DoubleAttribute(value,&minx);
		} else if (!strcmp(name,"maxx")) {
			DoubleAttribute(value,&maxx);
		} else if (!strcmp(name,"miny")) {
			DoubleAttribute(value,&miny);
		} else if (!strcmp(name,"maxy")) {
			DoubleAttribute(value,&maxy);
		} else if (!strcmp(name,"bboxstyle")) {
			IntAttribute(value,&bboxstyle);
		}
	}
}

/*! \ingroup interfaces
 * Dump out a SomeData, assume f is open already.
 * prepend indent number of spaces before each line.
 *
 * outputs something like:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  minx 0
 *  maxx 1
 *  miny 0
 *  maxy 1
 *  bboxstyle 0
 * </pre>
 * 
 * Ignores what. Uses 0 for it.
 */
void SomeData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+1];
	memset(spc,' ',indent);
	spc[indent]='\0';
	const double *matrix=m();
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	fprintf(f,"%sminx %.10g\n",spc,minx);
	fprintf(f,"%smaxx %.10g\n",spc,maxx);
	fprintf(f,"%sminy %.10g\n",spc,miny);
	fprintf(f,"%smaxy %.10g\n",spc,maxy);
	fprintf(f,"%sbboxstyle %d\n",spc,bboxstyle);
}

Attribute *SomeData::dump_out_atts(Attribute *att,int what,Laxkit::anObject *savecontext)
{
	if (!att) att=new LaxFiles::Attribute("SomeData",NULL);

	if (what==-1) {
		att->push("matrix", _("An affine matrix of 6 numbers"));
		att->push("minx", _("Minimum x bound"));
		att->push("maxx", _("Maximum x bound"));
		att->push("miny", _("Minimum y bound"));
		att->push("maxy", _("Maximum y bound"));
		att->push("bboxstyle", _("Style for this box"));
		return att;
	}

	const double *matrix=m();
	char s[100];
	sprintf(s,"%.10g %.10g %.10g %.10g %.10g %.10g",
			matrix[0],matrix[1],matrix[2],matrix[3],matrix[4],matrix[5]);
	att->push("matrix", s);

	sprintf(s,"%.10g",minx);
	att->push("minx", s);

	sprintf(s,"%.10g",maxx);
	att->push("maxx", s);

	sprintf(s,"%.10g",miny);
	att->push("miny", s);

	sprintf(s,"%.10g",maxy);
	att->push("maxy", s);

	sprintf(s,"%d",bboxstyle);
	att->push("bboxstyle", s);


	return att;
}

//! Duplicate *this.
/*! If dup!=NULL, then copy all relevant portions of this to dup. This might occur when
 * taking a shortcut to duplicate components of super classes.
 *
 * If dup==NULL, then return a brand new copy of this object. In this case,
 * subclasses should first call somedatafactory->newObject(object_type, reference) and return
 * that if any, before creating one of itself.
 *
 * bboxstyle and the matrix are copied over here.
 */
SomeData *SomeData::duplicate(SomeData *dup)
{
	SomeData *ndata=dynamic_cast<SomeData*>(dup);
	if (!ndata && !dup) return NULL;

	if (!ndata && somedatafactory) {
		ndata=somedatafactory->newObject(LAX_SOMEDATA);
		if (ndata) ndata->setbounds(minx,maxx,miny,maxy);
	} 
	if (!ndata) ndata=new SomeData(minx,maxx,miny,maxy);

	ndata->bboxstyle=bboxstyle;
	ndata->m(m());
	return ndata;
}

//! Return if pp transformed to data coords is within the bounds.
/*! unimplemented: in=1 | on=2 | out=0 */
int SomeData::pointin(flatpoint pp,int pin)
{ 
//	double x=((pp-origin())*xaxis())/sqrt(xaxis()*xaxis()), 
//		   y=((pp-origin())*yaxis())/sqrt(yaxis()*yaxis());
	double x,y,mm[6];
	transform_invert(mm,m());
	pp=transform_point(mm,pp);
	x=pp.x;
	y=pp.y;
	//DBG cerr <<"------ SomeData::pointin "<<x<<','<<y<<endl;
	if (x>=minx && x<=maxx && y>=miny && y<=maxy) return 1;
	return 0;
}

//! Fit this data to box.
/*! This adjusts this->matrix so that this's bounds lie within the box transformed
 * by boxm. To put it differently, make this fit in another SomeData's bounding box.
 * This will take on the box's matrix, and then the scaling and origin are adjusted
 * so this's bounding box fits inside box. Note that this is very different than merely
 * visually translating and scaling this so that it fits in the transformed box, because
 * this is rotated to have axes parallel to box.
 *
 * this->m() transforms this to its parent space, and it is assumed that boxm
 * transforms the box also to this's parent space. 
 *
 * If boxm==NULL, then assume the axes are aligned properly already, and we only need
 * to adjust the origin and scaling. It has the same effect as when boxm==this->m().
 *
 * If this does not fit exactly, then the alignx and aligny specify how to fit in
 * the leftover space. 0 means at bottom or left, 100 means at top or right, 50 means center, 
 * and other values are everything else in between.
 *
 * If whentoscale==0, then never scale, just align. If ==1, then scale down if necessary, but not up
 * if smaller than box. Else scale up and down to fit to bounds
 *
 * Return 0 for success, or 1 for error, for instance if box or this had invalid bounds.
 *
 * \todo ***this should maybe be fitto_aligned(), and a fitto() should be shifting and
 *    scaling so that this fits in the transformed box, which is also useful sometimes..
 */
int SomeData::fitto(double *boxm,DoubleBBox *box,double alignx,double aligny, int whentoscale)
{
	if (!box) return 1;
	alignx/=100;
	aligny/=100;
	if (boxm) m(boxm);
	double scale=1;
	flatpoint banchor(box->minx*(1-alignx)+box->maxx*alignx, box->miny*(1-aligny)+box->maxy*aligny),
	          manchor(minx*(1-alignx)+maxx*alignx, miny*(1-aligny)+maxy*aligny);
	banchor=transform_point(m(),banchor);

	if (whentoscale==2 || (whentoscale==1 && (maxx-minx>box->maxx-box->minx || maxy-miny>box->maxy-box->miny))) {
		 // make new scaling only when it doesn't already fit in box
		scale=(box->maxx-box->minx)/(maxx-minx);
		double yscale=(box->maxy-box->miny)/(maxy-miny);
		if (yscale<scale) scale=yscale;

		xaxis(xaxis()*scale);
		yaxis(yaxis()*scale);
	}
	origin(origin()+banchor-transform_point(m(),manchor));

	return 0;
}



} // namespace LaxInterfaces

