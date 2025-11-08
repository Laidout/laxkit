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
//    Copyright (C) 2004-2011 by Tom Lechner
//


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/interfacemanager.h>

#include <lax/interfaces/somedata.h>
#include <lax/transformmath.h>
#include <lax/misc.h>
#include <lax/strmanip.h>
#include <lax/language.h>


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
/*! \var int SomeData::usepreview
 * \brief Flag for whether to use SomeData::preview rather than rendering.
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
	usepreview = 0;
	modified   = 0;

	locks      = 0;
	visible    = true;
	selectable = true;

	minx = maxx = miny = maxy = 0;

	bboxstyle = 0;
	flags     = 0;
	iid       = 0;
	nameid    = NULL;

	DBG cerr <<"SomeData "<<object_id<<" created, count=1"<<endl;
}

//! Constructor, setting the bounds.
/*! Object is created with a count of 1.
 */
SomeData::SomeData(double nminx,double nmaxx,double nminy,double nmaxy)
{
	usepreview = 0;

	locks      = 0;
	visible    = true;
	selectable = true;

	setbounds(nminx, nmaxx, nminy, nmaxy);
	bboxstyle = 0;
	iid       = 0;
	flags     = 0;
	nameid    = NULL;

	DBG cerr <<"SomeData "<<object_id<<" created, count=1"<<endl;
}

SomeData::~SomeData()
{
	delete[] nameid;
}

/*! If nameid is blank, then try to create a unique one.
 * Otherwise, just return nameid.
 */
const char *SomeData::Id()
{
    if (!nameid) {
		if (object_idstr) makestr(nameid,object_idstr);
		else {
			makestr(nameid, anObject::Id());
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


/*! Return selectable | (locks&OBJLOCK_Selectable);
 */
int SomeData::Selectable()
{ return selectable | (locks&OBJLOCK_Selectable); }

/*! Return the new visible state (which is show, by default). */
bool SomeData::Visible(bool show)
{
	visible = show;
	return visible;
}

int SomeData::Visible()
{ return visible; }

bool SomeData::VisibleInTree(SomeData **topmost_hidden_ret)
{
	SomeData *v = nullptr;
	SomeData *o = this;
	while (o) {
		if (!o->visible) v = o;
		o = o->GetParent();
	}
	if (topmost_hidden_ret) *topmost_hidden_ret = v;
	return v != nullptr;
}

/*! If which==0, default to OBJLOCK_Selectable.
 */
int SomeData::IsLocked(int which)
{
	if (which == 0) which = OBJLOCK_Selectable;
	return (locks&which);
}

/*! or which into locks. 
 */
void SomeData::Lock(int which)
{ locks |= which; }

/*! Remove which from locks.
 */
void SomeData::Unlock(int which)
{ locks &= ~which; }


/*! Compute axis aligned bounding box of content transformed by transform.
 * Where FindBBox() finds bounds in object space, this function is designed to wrap
 * content to another space. Default is to just transform the normal bbox, and add that.
 */
void SomeData::ComputeAABB(const double *transform, DoubleBBox &box)
{
	if (!validbounds()) return;
	box.addtobounds(transform_point(transform, BBoxPoint(0,0, false)));
	box.addtobounds(transform_point(transform, BBoxPoint(1,0, false)));
	box.addtobounds(transform_point(transform, BBoxPoint(0,1, false)));
	box.addtobounds(transform_point(transform, BBoxPoint(1,1, false)));
}

/*! Like Laxkit::DoubleBBox::BBoxPoint(x,y), but if transform_to_parent==true,
 * then transform the point by m().
 * Point (0,0) is returned as (minx,miny)
 * and (1,1) is returned as (maxx, maxy).
 */
flatpoint SomeData::BBoxPoint(double x,double y, bool transform_to_parent)
{
	flatpoint p=DoubleBBox::BBoxPoint(x,y);
	if (transform_to_parent) return transformPoint(p);
	return p;
}

/*! which is one of Laxkit::BBoxReferencePoint.
 * If transform_to_parent, return the point transformed by m().
 */
flatpoint SomeData::ReferencePoint(int which, bool transform_to_parent)
{
	double x=0,y=0;

	if (which==LAX_TOP_LEFT)           { x=0;  y=1;  }
	else if (which==LAX_TOP_MIDDLE)    { x=.5; y=1;  }
	else if (which==LAX_TOP_RIGHT)     { x=1;  y=1;  }
	else if (which==LAX_MIDDLE_LEFT)   { x=0;  y=.5; }
	else if (which==LAX_MIDDLE)        { x=.5; y=.5; }
	else if (which==LAX_MIDDLE_RIGHT)  { x=1 ; y=.5; }
	else if (which==LAX_BOTTOM_LEFT)   { x=0;  y=0;  }
	else if (which==LAX_BOTTOM_MIDDLE) { x=.5; y=0;  }
	else if (which==LAX_BOTTOM_RIGHT)  { x=1;  y=0;  }

	return BBoxPoint(x,y,transform_to_parent);

}

void SomeData::FlipV()
{
	Flip(transformPoint(flatpoint(minx,(miny+maxy)/2)), transformPoint(flatpoint(maxx,(miny+maxy)/2)));
}

void SomeData::FlipH()
{
	Flip(transformPoint(flatpoint((minx+maxx)/2, miny)), transformPoint(flatpoint((minx+maxx)/2, maxy)));
}

//! Render the object to a buffer.
/*! This draws onto buffer such that the object's whole bounding box maps to the whole buffer.
 *
 * This is mainly used to create screen previews in ARGB 8 bit format.
 * bufdepth and bufchannels are not currently used by Laxkit.
 * 
 * Default here is just a do nothing placeholder, and only returns 1.
 * On success, it should return 0.
 */
int SomeData::renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels)
{
	return 1;
}

/*! Render the object to an existing image.
 * 
 *  This draws onto buffer such that the object's whole bounding box maps to the whole image.
 *
 * This is currently used to create screen previews.
 * 
 * Default here uses InterfaceManager::GetDefault()->DrawDataStraight(), returning > 0 for error,
 * or 0 on success.
 */
int SomeData::renderToBufferImage(LaxImage *image)
{
	if (!image) return 1;
	if (maxx-minx<=0 || maxy-miny<=0) return 2;
	if (image->w()<=0 || image->h()<=0) return 3;

	DBG cerr <<"^v^v^v^v^V^v^v^v^v SomeData::renderToBufferImage preview on "<<(Id() ? Id() : whattype())<<"image size: "<<image->w()<<" x "<<image->h()<<endl;

	InterfaceManager *imanager = InterfaceManager::GetDefault(true);
	Displayer *dp = imanager->GetPreviewDisplayer();


	if (dp->MakeCurrent(image) != 0) return 3;
	dp->ClearTransparent();

	DBG dp->NewFG(0.0,1.0,0.0);
	//DBG dp->NewTransform(1,0,0,1,0,0);
	//DBG dp->drawline(0,image->h(), image->w(),0);
	//DBG dp->NewFG(1.0,1.0,1.0);
	//DBG dp->drawline(minx,miny, maxx,maxy);

	dp->NewTransform(image->w()/(maxx-minx), 0, 0, -image->h()/(maxy-miny), -minx*image->w()/(maxx-minx), image->h()*(1+miny/(maxy-miny)));
	//DBG dp->NewFG(1.0,0.0,1.0);
	//DBG dp->drawline(0,image->h(), image->w(),0);

	int oldusepreview = usepreview;
	usepreview = 0;
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

// //! Create a preview image with transparency for a cached screen preview.
// /*! Will create regardless of usepreview. 
//  *  This will return immediately if the bounds are not valid.
//  *  If w or h are less than or equal to 0, then use default or previous values for them.
//  *  The default is to fit within a box 200x200, with aspect approximately the same
//  *  as the object's bounding box.
//  *
//  *  Subclasses need not redefine this function. They need only redefine renderToBuffer().
//  */
// int SomeData::GeneratePreview(int maxdim)
// {
// 	if (maxx <= minx || maxy <= miny) //bad bounds so return
// 		return (previewtime >= modtime);

// 	DBG cerr <<"...SomeData::GeneratePreview()"<<endl;


// 	if (maxdim <= 0) maxdim = 500;
// 	InterfaceManager *im = InterfaceManager::GetDefault(true);
// 	if (im) maxdim = im->PreviewSize();

// 	int w = 0;
// 	int h = 0;

// 	if (maxx - minx > maxy - miny) {
// 		w = maxdim;
// 		h = (maxy - miny) * w / (maxx - minx);
// 	} else {
// 		h = maxdim;
// 		w = (maxx - minx) * h / (maxy - miny);
// 	}

// 	if (w <= 0) w = 1;
// 	if (h <= 0) h = 1;

// 	// protect against growing sizes...
// 	if (w > maxdim) {
// 		double aspect = (double)h / w;
// 		w = maxdim;
// 		h = maxdim * aspect;
// 		if (h <= 0) h = 1;
// 	}
// 	if (h > maxdim) {
// 		double aspect = (double)w / h;
// 		h = maxdim;
// 		w = maxdim * aspect;
// 		if (w <= 0) w = 1;
// 	}

// 	//if (preview && (w!=preview->w() || h!=preview->h())) {
// 	if (preview && ((float)w/preview->w()>1.05 || (float)w/preview->w()<.95 ||
// 					(float)h/preview->h()>1.05 || (float)h/preview->h()<.95)) {
// 		 //delete old preview and make new only when changing size of preview more that 5%-ish in x or y
// 		DBG cerr <<"removing old preview..."<<endl;
// 		preview->dec_count(); 
// 		preview = nullptr;
// 	} else if (preview) { //use previous bounds, which might not be the ideal w,h
// 		w = preview->w();
// 		h = preview->h();
// 	}

// 	if (!preview) {
// 		DBG cerr <<"old preview didn't exist, so creating new one... w,h: "<<w<<", "<<h<<endl;
// 		preview = ImageLoader::NewImage(w,h);
// 	}

// 	DBG cerr << "Render to buffer w,h="<<w<<','<<h<<", preview says: "<<preview->w()<<','<<preview->h()<<endl;

// 	if (renderToBufferImage(preview) != 0) { 
// 		 //render direct to image didn't work, so try the old style render to char[] buffer...
// 		DBG cerr << "SomeData::GeneratePreview: Attempt render preview direct to buffer, w,h="<<w<<','<<h<<":"<<endl;
// 		unsigned char *buffer = preview->getImageBuffer(); 
// 		renderToBuffer(buffer,w,h,w*4,8,4); 
// 		if (preview->doneWithBuffer(buffer) == 0) {
// 			tms tms_;
// 			previewtime = times(&tms_);
// 		}

// 	} else  {
// 		tms tms_;
// 		previewtime = times(&tms_);
// 	}

// 	return (previewtime >= modtime);
// }

/*! Set previewtime to 0 to force a preview refresh, and modtime=times(NULL).
 * If GetParent() is not null, then call touchContents() on it.
 */
void SomeData::touchContents()
{ 
	Previewable::touchContents();
	if (GetParent()) GetParent()->touchContents();
}

//! If usepreview==1 and preview, then return preview.
/*! If previewtime>modtime, then call GeneratePreview(-1,-1) before returning preview.
 */
Laxkit::LaxImage *SomeData::GetPreview()
{
	// if (usepreview==1) {
	// 	if (previewtime<modtime || !preview) GeneratePreview(-1,-1);
	// 	return preview;
	// }
	// return NULL;

	if (previewtime < modtime || !preview || (modtime == 0 && previewtime == 0)) GeneratePreview(-1,-1);
	return preview;
}

//! Dump in an Attribute, then call dump_in_atts(thatatt,0).
/*! If Att!=NULL, then return the attribute used to read in the stuff.
 * This allows holding classes to have extra attributes within the spot field to
 * exist and not be discarded.
 */
void SomeData::dump_in(FILE *f, int indent, int what, Laxkit::DumpContext *context, Attribute **Att)
{
	Attribute *att = new Attribute;
	att->dump_in(f, indent);
	dump_in_atts(att, 0, context);
	if (Att) *Att = att;
	else delete att;
}

//! Reverse of dump_out().
void SomeData::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
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

		} else if (!strcmp(name,"visible")) {
			visible = BooleanAttribute(value);
		} else if (!strcmp(name,"selectable")) {
			selectable = BooleanAttribute(value);

		} else if (!strcmp(name,"locks")) {
			if (isblank(value)) continue; 

			int n=0;
			char **strs = splitspace(value, &n);
			for (int c=0; c<n; c++) {
				if      (!strcmp(strs[c],"contents"  )) locks |= OBJLOCK_Contents  ;
				else if (!strcmp(strs[c],"position"  )) locks |= OBJLOCK_Position  ;
				else if (!strcmp(strs[c],"rotation"  )) locks |= OBJLOCK_Rotation  ;
				else if (!strcmp(strs[c],"scale"     )) locks |= OBJLOCK_Scale     ;
				else if (!strcmp(strs[c],"shear"     )) locks |= OBJLOCK_Shear     ;
				else if (!strcmp(strs[c],"kids"      )) locks |= OBJLOCK_Kids      ;
				else if (!strcmp(strs[c],"selectable")) locks |= OBJLOCK_Selectable;
			}
			deletestrs(strs, n);
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
 *  visible
 *  selectable
 *  locks contents position rotation scale shear kids selectable
 * </pre>
 * 
 * Ignores what. Uses 0 for it.
 */
void SomeData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
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
	//fprintf(f,"%sbboxstyle %d\n",spc,bboxstyle);

	if (visible)    fprintf(f,"%svisible\n",spc);
	if (selectable) fprintf(f,"%sselectable\n",spc);
	fprintf(f,"%slocks ",spc);
	if (locks & OBJLOCK_Contents  ) fprintf(f,"contents ");
	if (locks & OBJLOCK_Position  ) fprintf(f,"position ");
	if (locks & OBJLOCK_Rotation  ) fprintf(f,"rotation ");
	if (locks & OBJLOCK_Scale     ) fprintf(f,"scale ");
	if (locks & OBJLOCK_Shear     ) fprintf(f,"shear ");
	if (locks & OBJLOCK_Kids      ) fprintf(f,"kids ");
	if (locks & OBJLOCK_Selectable) fprintf(f,"selectable ");
	fprintf(f,"\n");
}

Attribute *SomeData::dump_out_atts(Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att=new Laxkit::Attribute("SomeData",NULL);

	if (what==-1) {
		att->push("matrix", "1 0 0 1 0 0", _("An affine matrix of 6 numbers"));
		att->push("minx", "0", _("Minimum x bound"));
		att->push("maxx", "1", _("Maximum x bound"));
		att->push("miny", "0", _("Minimum y bound"));
		att->push("maxy", "1", _("Maximum y bound"));
		//att->push("bboxstyle", _("Style for this box"));
		att->push("selectable", "true", "Whether object is user selectable");
		att->push("visible", "true", "Whether object is visible");
		att->push("locks", "position", "combination of: contents position rotation scale shear kids selectable");
		return att;
	}

	const double *matrix=m();
	char s[200];
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

	//sprintf(s,"%d",bboxstyle);
	//att->push("bboxstyle", s);

	if (visible) att->push("visible");

	if (selectable) att->push("selectable");

	if (locks) {
		char scratch[15*7];
		scratch[0]='\0';

		if (locks & OBJLOCK_Contents  ) strcat(scratch, "contents ");
		if (locks & OBJLOCK_Position  ) strcat(scratch, "position ");
		if (locks & OBJLOCK_Rotation  ) strcat(scratch, "rotation ");
		if (locks & OBJLOCK_Scale     ) strcat(scratch, "scale ");
		if (locks & OBJLOCK_Shear     ) strcat(scratch, "shear ");
		if (locks & OBJLOCK_Kids      ) strcat(scratch, "kids ");
		if (locks & OBJLOCK_Selectable) strcat(scratch, "selectable "); 

		att->push("locks", scratch);
	}

	return att;
}

//! Duplicate *this.
/*! If dup!=NULL, then copy all relevant portions of this to dup. This might occur when
 * taking a shortcut to duplicate components of super classes.
 *
 * If dup==NULL, then return a brand new copy of this object. In this case,
 * subclasses should first call interfacemanager->NewObject(object_type, reference) and return
 * that if any, before creating one of itself.
 *
 * bboxstyle, locks, visible, selectable, and the matrix are copied over here.
 */
SomeData *SomeData::duplicateData(SomeData *dup)
{
	SomeData *ndata=dynamic_cast<SomeData*>(dup);
	if (!ndata && !dup) return NULL;

	if (!ndata) {
		ndata=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_SOMEDATA));
		if (ndata) ndata->setbounds(minx,maxx,miny,maxy);
	} 
	if (!ndata) ndata=new SomeData(minx,maxx,miny,maxy);

	ndata->locks = locks;
	ndata->bboxstyle=bboxstyle;
	ndata->visible = visible;
	ndata->selectable = selectable;
	ndata->m(m());

	return ndata;
}

/*! Return if parent space pp is within the bounds.
 * Default transforms to object space and returns if in bounding box, and ignores the pin options.
 * Child classes should implement pin.
 */
int SomeData::pointin(flatpoint pp,int pin)
{ 
	double x,y,mm[6];
	transform_invert(mm,m());
	pp = transform_point(mm,pp);
	x = pp.x;
	y = pp.y;
	if (x >= minx && x <= maxx && y >= miny && y <= maxy) return 1;
	return 0;
}

//! Fit this data to box.
/*! This adjusts this->matrix so that this's bounds lie within the box transformed
 * by boxm. To put it differently, make this fit in another SomeData's bounding box.
 * This will take on the box's orientation, and then the scaling and origin are adjusted
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
  */
int SomeData::AlignAndFit(double *boxm,DoubleBBox *box,double alignx,double aligny, int whentoscale)
{
	if (!box) return 1;
	alignx /= 100;
	aligny /= 100;
	if (boxm) m(boxm); //else setIdentity();
	double scale = 1;
	flatpoint banchor(box->minx*(1-alignx) + box->maxx*alignx, box->miny*(1-aligny) + box->maxy*aligny),
	          manchor(minx*(1-alignx) + maxx*alignx, miny*(1-aligny) + maxy*aligny);
	banchor = transform_point(m(),banchor);

	DBG cerr << "SomeData::AlignAndFit box: "<<box->minx<<" "<<box->miny<<" "<<box->maxx<<" "<<box->maxy<<endl;
	DBG cerr << "                this bbox: "<<minx<<" "<<miny<<" "<<maxx<<" "<<maxy<<endl;

	if (whentoscale==2 || (whentoscale==1 && (maxx-minx > box->maxx-box->minx || maxy-miny > box->maxy-box->miny))) {
		 // make new scaling only when it doesn't already fit in box
		scale = (box->maxx-box->minx)/(maxx-minx);
		double yscale = (box->maxy-box->miny)/(maxy-miny);
		DBG cerr << "     scalex: "<<scale<<" scaley: "<<yscale<<endl;
		if (yscale < scale) scale = yscale;

		xaxis(xaxis()*scale);
		yaxis(yaxis()*scale);
	}
	origin(origin() + banchor - transform_point(m(), manchor));

	return 0;
}

/*! Concatenate depth number of parent transforms.
 * For instance, depth==2 is `parent->m * this->m`.
 */
Laxkit::Affine SomeData::GetTransforms(int depth, bool invert)
{
	Affine a;

	SomeData *d = this;
	for (int c=0; c<depth; c++) {
		a.PreMultiply(d->m());
		d = d->GetParent();
	}

	if (invert) a.Invert();
    return a;
}

/*! Assumes that ascestor is in parent chain, then return a concatenation of transform to it.
 * So, say you have parent1, whose parent is parent2, then return a transform `parent2 * parent1 * self`.
 */
Laxkit::Affine SomeData::GetTransforms(SomeData *ancestor, bool invert)
{
	Affine a;

	SomeData *d = this;
	do {
		a.PreMultiply(d->m());
		if (d == ancestor) break;
		d = d->GetParent();
	} while (d);

	if (invert) a.Invert();
    return a;
}

/*! Return concatenation of parent transforms (parent->m * parent->parent->m * ...).
 * Note this is not valid beyond containing page.
 *  
 * If partial>0, then do not use that many upper transforms. For instance, partial==1
 * means get the transform from the lowest base to the parent space, not to the object space.
 */ 
Laxkit::Affine SomeData::GetTransformToContext(bool invert, int partial)
{
    SomeData *d=this;
    while (d && partial>0) { d=d->GetParent(); partial--; }
    
    Affine a;
    while (d) { 
        a.Multiply(*dynamic_cast<Affine*>(d));
        d=d->GetParent();
    }
    
    if (invert) a.Invert();
    return a;
}   

/*! Only set the parent. If NULL, then clear the parent.
 * Default is to do nothing, since parenting is currently not stored by default.
 * Return what is the parent value after setting.
 */
SomeData *SomeData::SetParent(SomeData *newparent)
{
	return NULL;
}

/*! Return how many parents this has. If parent==NULL, then this is 0.
 */
int SomeData::NestedDepth()
{
	int n=0;
    SomeData *d=this->GetParent();
    while (d) { d=d->GetParent(); n++; }
	return n;
}

SomeData *SomeData::FindCommonParent(SomeData *other)
{
	if (!other) return nullptr;
	if (other == this) return this;

	SomeData *p1 = this;
	while (p1) {
		SomeData *p2 = other;
		while (p2) {
			if (p2 == p1) return p1;
			p2 = p2->GetParent();
		}
		p1 = p1->GetParent();
	}

	return nullptr;
}

/*! Return the number up from *this until reaching obj.
 * If not found, return -1.
 */
int SomeData::IsChildOf(SomeData *obj)
{
	SomeData *o = this;
	int n = 0;
	while (o) {
		if (o == obj) return n;
		o = o->GetParent();
	}
	return -1;
}


//------------------------- Undo stuff:
   
/*! \class SomeDataUndo 
 * Class to hold basic bounds and transform undo data for SomeData objects.
 */

SomeDataUndo::SomeDataUndo(SomeData *object,
						   Laxkit::Affine *mo, Laxkit::DoubleBBox *boxo,
						   Laxkit::Affine *nm, Laxkit::DoubleBBox *nbox,
						   int ntype, int nisauto)
  : UndoData(nisauto)
{
	if (nm) m=*nm;
	if (nbox) box.setbounds(nbox);
	type=ntype;

	if (mo) m_orig=*mo;
	if (boxo) box_orig.setbounds(boxo);

	context=object;
	if (object) object->inc_count();

	switch(type) {
		case SomeDataUndo::SDUNDO_Bounds   : msg.Sprintf("%s: %s", object->Id(), _("New bounds"));    break;
		case SomeDataUndo::SDUNDO_Transform: msg.Sprintf("%s: %s", object->Id(), _("New transform")); break;
		case SomeDataUndo::SDUNDO_Shift    : msg.Sprintf("%s: %s", object->Id(), _("Shift"));         break;
		case SomeDataUndo::SDUNDO_Rotation : msg.Sprintf("%s: %s", object->Id(), _("Rotation"));      break;
		case SomeDataUndo::SDUNDO_Scale    : msg.Sprintf("%s: %s", object->Id(), _("Scale"));         break;
		case SomeDataUndo::SDUNDO_Shear    : msg.Sprintf("%s: %s", object->Id(), _("Shear"));         break;
		case SomeDataUndo::SDUNDO_Flip     : msg.Sprintf("%s: %s", object->Id(), _("Flip"));          break;
	}
}

const char *SomeDataUndo::Description()
{
	return msg.c_str();
	// if      (type==SomeDataUndo::SDUNDO_Bounds   ) return _("New bounds");
	// else if (type==SomeDataUndo::SDUNDO_Transform) return _("New transform");
	// else if (type==SomeDataUndo::SDUNDO_Shift    ) return _("Shift");
	// else if (type==SomeDataUndo::SDUNDO_Rotation ) return _("Rotation");
	// else if (type==SomeDataUndo::SDUNDO_Scale    ) return _("Scale");
	// else if (type==SomeDataUndo::SDUNDO_Shear    ) return _("Shear");
	// else if (type==SomeDataUndo::SDUNDO_Flip     ) return _("Flip");
	//
	// return NULL;
}

int SomeData::Undo(UndoData *data)
{
	SomeDataUndo *u = dynamic_cast<SomeDataUndo*>(data);
	if (!u) return 1;

	if (u->type == SomeDataUndo::SDUNDO_Bounds)    setbounds(&u->box_orig); 
	else if (u->type == SomeDataUndo::SDUNDO_Transform) set(u->m_orig);
	else set(u->m_orig);

	touchContents();
	return 0;
}

int SomeData::Redo(UndoData *data)
{
	SomeDataUndo *u = dynamic_cast<SomeDataUndo*>(data);
	if (!u) return 1;

	if (u->type == SomeDataUndo::SDUNDO_Bounds)    { setbounds(&u->box); } 
	else if (u->type == SomeDataUndo::SDUNDO_Transform) { set(u->m); }
	else set(u->m);
	
	touchContents();
	return 0;
}

/*! Default return 0. Can be used to specify that this object uses some object
 * that is meant to be accessible by other objects.
 *
 * See also ResourceInfo().
 */
int SomeData::NumResources()
{
	return 0;
}

/*! Return 1 for not found, or 0 for success, and the resource in question is returned in resource_ret.
 *
 * See also NumResources().
 */
int SomeData::ResourceInfo(int index, Laxkit::anObject **resource_ret)
{
	*resource_ret=NULL;
	return 1;
}


} // namespace LaxInterfaces

