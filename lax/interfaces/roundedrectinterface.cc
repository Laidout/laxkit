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
//    Copyright (C) 2025 by Tom Lechner
//


#include <lax/interfaces/roundedrectinterface.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/somedataref.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/bezutils.h>
#include <lax/language.h>

#include <lax/debug.h>
#include <lax/vectors-out.h>

using namespace Laxkit;


namespace LaxInterfaces {


const char *PointName(RRectPoints p)
{
	switch (p) {
		case RRECT_None:        return _("None");
		case RRECT_Center:      return _("Center");
		default: {
			DBGL("PointName unknown for: "<<p);
			return "?";
		}
	}
	return "?";
}

const char *PointMessage(RRectPoints p)
{
	switch (p) {
		case RRECT_None:        return _("None");
		case RRECT_Center:      return _("Center");
		default: {
			DBGL("PointName unknown for: "<<p);
			return "?";
		}
	}
	return "?";
}

/*! \class RoundedRectData
 * \ingroup interfaces
 * \brief Data class for RoundedRectInterface.
 */


RoundedRectData::RoundedRectData()
{
	linestyle = nullptr;
	fillstyle = nullptr;

	for (int i=0; i<8; i++) { round[i] = .5; }
}

RoundedRectData::~RoundedRectData()
{
	if (linestyle) linestyle->dec_count();
	if (fillstyle) fillstyle->dec_count();
}

/*! Incs count on style, unless it is already installed (takes ownership of newlinestyle).
 */
void RoundedRectData::InstallLineStyle(LineStyle *newlinestyle)
{
	if (linestyle == newlinestyle) return;
	if (linestyle) linestyle->dec_count();
	linestyle = newlinestyle;
	if (newlinestyle) newlinestyle->inc_count();
}

/*! Incs count on style, unless it is already installed.
 */
void RoundedRectData::InstallFillStyle(FillStyle *newfillstyle)
{
	if (fillstyle==newfillstyle) return;
	if (fillstyle) fillstyle->dec_count();
	fillstyle=newfillstyle;
	if (newfillstyle) newfillstyle->inc_count();
}

//! Fill with this color, or none if color==nullptr.
/*! If no fillstyle exists, a new one is created. If one does exist, its old color is overwritten. */
int RoundedRectData::fill(Laxkit::ScreenColor *color)
{
	if (!color) {
		if (fillstyle) fillstyle->fillstyle = LAXFILL_None;
		return 0;
	}
	if (!fillstyle) fillstyle = new FillStyle();
	fillstyle->Color(color->red,color->green,color->blue,color->alpha);
	fillstyle->fillstyle = LAXFILL_Solid;
	if (fillstyle->function == LAXOP_None) fillstyle->function = LAXOP_Over;

	return 0;
}

SomeData *RoundedRectData::duplicate(SomeData *dup)
{
	RoundedRectData *p=dynamic_cast<RoundedRectData*>(dup);
	if (!p && !dup) return nullptr;

	if (!dup) {
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject("RoundedRectData"));
		if (dup) {
			p=dynamic_cast<RoundedRectData*>(dup);
		}
	} 

	if (!p) {
		p = new RoundedRectData();
		dup = p;
	}

	if (linestyle) p->linestyle = dynamic_cast<LineStyle*>(linestyle->duplicate(nullptr));
	if (fillstyle) p->fillstyle = dynamic_cast<FillStyle*>(fillstyle->duplicate(nullptr));

	return dup;
}

// bool RoundedRectData::GetStyle(unsigned int s)
// { return style&s; }

// void RoundedRectData::SetStyle(unsigned int s, bool on)
// {
// 	if (on) style|=s;
// 	else style&=~s;
// }

void RoundedRectData::FindBBox()
{
	minx = -(1.0 - x_align/100) * width;
	maxx = x_align/100 * width;
	miny = -(1.0 - y_align/100) * height;
	maxy = y_align/100 * height;

	if (width < 0) {
		double t = minx;
		minx = maxx;
		maxx = t;
	}

	if (height < 0) {
		double t = miny;
		miny = maxy;
		maxy = t;
	}
}

/*! Get a bezier representation of the path.
 * 
 * Returned points are c-v-c-c-v-c-c-v-c-c-v-c...
 * Return value is the number of points written to pts_ret.
 */
int RoundedRectData::GetPath(flatpoint *pts_ret)
{
	NumStack<flatpoint> pts;
	flatpoint r[4];
	r[0] = flatpoint(round[0], round[1]);
	r[1] = flatpoint(round[2], round[3]);
	r[2] = flatpoint(round[4], round[5]);
	r[3] = flatpoint(round[6], round[7]);
	MakeRoundedRect(minx, miny, maxx-minx, maxy-miny, r, 4, pts); // returns v-c-c-v-...-c-c
	for (int c=0; c<pts.n; c++) {
		pts_ret[c] = pts[(c+pts.n-1)%pts.n];
	}

	// //TODO: IMPLEMENT ME!!
	// pts_ret[0] = pts_ret[1] = pts_ret[2] = flatpoint(minx,miny);
	// pts_ret[3] = pts_ret[4] = pts_ret[5] = flatpoint(maxx,miny);
	// pts_ret[6] = pts_ret[7] = pts_ret[8] = flatpoint(maxx,maxy);
	// pts_ret[9] = pts_ret[10] = pts_ret[11] = flatpoint(minx,maxy);
	// return 12;
	return pts.n;
}

PathsData *RoundedRectData::ToPath()
{
	PathsData *paths = dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
	if (!paths) paths = new PathsData();

	flatpoint pts[24];
	int n = GetPath(pts);
	for (int c=0; c<n; c++) {
		paths->append(pts[c], c%3 == 0 ? POINT_TONEXT : (c%3 == 1 ? POINT_VERTEX : POINT_TOPREV));
		if (pts[c].info & LINE_Open) paths->pushEmpty();
		else if (pts[c].info & LINE_Closed) {
			paths->close();
			paths->pushEmpty();
		}
	}

	paths->m(m());
	return paths;
}

flatpoint RoundedRectData::getpoint(RRectPoints c, bool transform_to_parent)
{
	flatpoint p((maxx+minx)/2, (maxy+miny)/2);
	flatpoint center = p;
	double a = width/2;
	double b = height/2;
	flatvector x(1,0);
	flatvector y(0,1);

	if        (c==RRECT_Center) {      p= center;
	} else if (c==RRECT_Right) {       p= center + a*x; 
	} else if (c==RRECT_BottomRight) { p= center + a*x - b*y; 
	} else if (c==RRECT_Bottom) {      p= center       - b*y; 
	} else if (c==RRECT_BottomLeft) {  p= center - a*x - b*y;
	} else if (c==RRECT_Left) {        p= center - a*x; 
	} else if (c==RRECT_TopLeft) {     p= center - a*x + b*y; 
	} else if (c==RRECT_Top) {         p= center       + b*y; 
	} else if (c==RRECT_TopRight) {    p= center + a*x + b*y;

	// } else if (c==RRECT_Right)  { p= center + a*x;
	// } else if (c==RRECT_Left) { p= center - a*x;
	// } else if (c==RRECT_Top)  { p= center + b*y;
	// } else if (c==RRECT_Bottom) { p= center - b*y;

	}

	if (transform_to_parent) return transformPoint(p);
	return p;
}

void RoundedRectData::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent);
}

Attribute *RoundedRectData::dump_out_atts(Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att=new Attribute();

	if (what==-1) { 
		att->push("id", "somename","String id");
		att->push("width", "1","Width to be aligned according to x_align");
		att->push("height","1","height to be aligned according to y_align");
		// ***
		att->push("FINISH ME!!!!!");
		att->push("linestyle",nullptr,"(optional) Line style for this shape");
		att->push("fillstyle",nullptr,"(optional) Fill style for this shape");
		return att;
	}

	att->push("id", Id());
	att->pushStr("matrix",-1, "%.10g %.10g %.10g %.10g %.10g %.10g",
			m(0),m(1),m(2),m(3),m(4),m(5));

	if (is_square) att->push("flags", "square");

	att->push("x_align", x_align);
	att->push("y_align", y_align);
	att->push("width", width);
	att->push("height", height);
	att->push("round_style", (round_style == 2 ? "custom" : (round_style == 1 ? "squircle" : "ellipsoidal")));
	att->push("round_type", round_type);
	att->pushStr("round",-1, "%.10g %.10g %.10g %.10g %.10g %.10g %.10g %.10g",
		round[0], round[1], round[2], round[3], round[4], round[5], round[6], round[7]);

	if (linestyle) {
		if (linestyle->IsResourced()) {
			att->pushStr("linestyle", -1, "resource:%s", linestyle->Id());
		} else {
			Attribute *att2 = att->pushSubAtt("linestyle");
			linestyle->dump_out_atts(att2, what, context);
		}
	}

	if (fillstyle) {
		if (fillstyle->IsResourced()) {
			att->pushStr("fillstyle", -1, "resource:%s", fillstyle->Id());
		} else {
			Attribute *att2 = att->pushSubAtt("fillstyle");
			fillstyle->dump_out_atts(att2, what, context);
		}
	}
	
	return att;
}

void RoundedRectData::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

	SomeData::dump_in_atts(att,flag,context);

	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"id")) {
			if (!isblank(value)) Id(value);

		} else if (!strcmp(name,"x_align")) {
		} else if (!strcmp(name,"y_align")) {
		} else if (!strcmp(name,"width")) {
		} else if (!strcmp(name,"height")) {
		} else if (!strcmp(name,"round_style")) {
			if (!strcasecmp_safe(value, "custom")) round_style = 2;
			else if (!strcasecmp_safe(value, "squircle")) round_style = 1;
			else round_style = 0;
		} else if (!strcmp(name,"round_type")) {
			IntAttribute(value, &round_type);
		} else if (!strcmp(name,"round")) {
			double d[8];
			int n = DoubleListAttribute(value, d, 8);
			if (n == 8) memcpy(round, d, 8*sizeof(double));

		} else if (!strcmp(name,"linestyle")) {
			if (linestyle) linestyle->dec_count();

			if (value && strstr(value,"resource:") == value) {
				value += 9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager = InterfaceManager::GetDefault(true);
				ResourceManager *rm = imanager->GetResourceManager();
				LineStyle *obj = dynamic_cast<LineStyle*>(rm->FindResource(value,"LineStyle"));
				if (obj) {
					linestyle = obj;
					linestyle->inc_count();
				}
			} else {
				linestyle = new LineStyle();
				linestyle->dump_in_atts(att->attributes.e[c],flag,context);
			}

		} else if (!strcmp(name,"fillstyle")) {
			if (fillstyle) fillstyle->dec_count();

			if (value && strstr(value,"resource:") == value) {
				value += 9;
				while (isspace(*value)) value ++;
				InterfaceManager *imanager = InterfaceManager::GetDefault(true);
				ResourceManager *rm = imanager->GetResourceManager();
				FillStyle *obj = dynamic_cast<FillStyle*>(rm->FindResource(value,"FillStyle"));
				if (obj) {
					fillstyle = obj;
					fillstyle->inc_count();
				}
			} else {
				fillstyle = new FillStyle();
				fillstyle->dump_in_atts(att->attributes.e[c],flag,context);
			}
		
		// } else if (!strcmp(name,"flags")) {
			// ***
		}
	}

	FindBBox();
}

//----------------------------- RoundedRectInterface ------------------------

/*! \class RoundedRectInterface
 * \ingroup interfaces
 * \brief Interface for RoundedRectData objects.
 */


//! Constructor.
/*! This keeps an internal RectInterface. Does not push onto viewport.
 */
RoundedRectInterface::RoundedRectInterface(anInterface *nowner, int nid,Displayer *ndp)
  : anInterface(nowner, nid,ndp)
{
	default_linestyle = nullptr;
	default_fillstyle = nullptr;

	controlcolor.rgbf(.5, .5, .5, 1);
	standoutcolor(controlcolor, true, &controlcolor_rim);
	data = nullptr;
	roc  = nullptr;

	showdecs        = 1;
	curpoint        = RRECT_None; //point clicked on
	hover_point     = RRECT_None; //point hovered over, changes when !mouse down
	creationstyle   = 0;
	createfrompoint = 0;  // 0=lbd=ulc, 1=lbd=center
	createangle     = 0;
	createx         = flatpoint(1, 0);
	createy         = flatpoint(0, 1);

	mode = RRECT_None;

	needtodraw = 1;

	sc = nullptr;
}

//! Destructor
RoundedRectInterface::~RoundedRectInterface()
{
	deletedata();
	if (sc) sc->dec_count();
	if (roc) delete roc;
	if (default_linestyle) default_linestyle->dec_count();
	if (default_fillstyle) default_fillstyle->dec_count();
	DBGL("----in RoundedRectInterface destructor");
}

void RoundedRectInterface::Dp(Laxkit::Displayer *ndp)
{
	anInterface::Dp(ndp);
}

const char *RoundedRectInterface::Name()
{
	return _("Rounded Rectangle");
}

//! Return new RoundedRectInterface.
/*! If dup!=nullptr and it cannot be cast to ImageInterface, then return nullptr.
 */
anInterface *RoundedRectInterface::duplicate(anInterface *dup)
{
	RoundedRectInterface *edup = dynamic_cast<RoundedRectInterface *>(dup);
	if (dup == nullptr) dup = edup = new RoundedRectInterface(nullptr,id,nullptr);
	else if (!dup) return nullptr;
	edup->default_linestyle = DefaultLineStyle();
	if (edup->default_linestyle) edup->default_linestyle->inc_count();
	edup->default_fillstyle = DefaultFillStyle();
	if (edup->default_fillstyle) edup->default_fillstyle->inc_count();
	return anInterface::duplicate(dup);
}

LineStyle *RoundedRectInterface::DefaultLineStyle()
{
	if (!default_linestyle) {
		default_linestyle = new LineStyle();
		default_linestyle->color.rgbf(1., 0, 0);
		default_linestyle->width = .1;
	}

	return default_linestyle;
}

FillStyle *RoundedRectInterface::DefaultFillStyle()
{
	if (!default_fillstyle) {
		default_fillstyle = new FillStyle();
		default_fillstyle->color.rgbf(1., 1., 1., 0.0);
	}

	return default_fillstyle;
}

/*! Update the viewport color box */
void RoundedRectInterface::UpdateViewportColor()
{
	if (!data) return;

	ScreenColor *stroke = nullptr;
	ScreenColor *fill = nullptr;

	if (!data->linestyle) {
		LineStyle *style = dynamic_cast<LineStyle*>(DefaultLineStyle()->duplicate(nullptr));
		data->InstallLineStyle(style);
		style->dec_count();
	}
	if (!data->fillstyle) {
		FillStyle *style = dynamic_cast<FillStyle*>(DefaultFillStyle()->duplicate(nullptr));
		data->InstallFillStyle(style);
		style->dec_count();
	}
	stroke = &data->linestyle->color;
	fill = &data->fillstyle->color;
	
	anInterface::UpdateViewportColor(stroke, fill, COLOR_StrokeFill);
}

//! Accepts LineStyle. For LineStyle, just copies over width and color.
int RoundedRectInterface::UseThis(anObject *nobj,unsigned int mask)
{
	if (!nobj) return 0;

	if (dynamic_cast<LineStyle *>(nobj)) { 
		LineStyle *nlinestyle = dynamic_cast<LineStyle *>(nobj);
		if (mask & LINESTYLE_Color) {
			if (data) {
				if (!data->linestyle) {
					LineStyle *style = dynamic_cast<LineStyle*>(DefaultLineStyle()->duplicate(nullptr));
					data->InstallLineStyle(style);
					style->dec_count();
				}
				data->linestyle->color = nlinestyle->color;

			} else DefaultLineStyle()->color = nlinestyle->color;
		}
		if (mask & LINESTYLE_Color2) {
			if (data) {
				if (!data->fillstyle) {
					FillStyle *style = dynamic_cast<FillStyle*>(DefaultFillStyle()->duplicate(nullptr));
					data->InstallFillStyle(style);
					style->dec_count();
				}
				data->fill(&nlinestyle->color);
			} else {
				DefaultFillStyle()->color = nlinestyle->color;
			}
		}
		if (mask & LINESTYLE_Width) {
			if (data && data->linestyle) data->linestyle->width = nlinestyle->width;
			else DefaultLineStyle()->width = nlinestyle->width;
		}
		
		return 1;
	}

	return 0;
}

/*! Use oc for the context, but other_object for the final object.
 * If other_object == null, use oc->obj instead.
 * Incs count of other_object.
 *
 * Return 1 for used, 0 for not used.
 */
int RoundedRectInterface::UseThisObject(ObjectContext *oc, SomeData *other_object)
{
	if (!other_object) other_object = oc->obj;

	RoundedRectData *ndata = dynamic_cast<RoundedRectData*>(other_object);
	if (!ndata && dynamic_cast<SomeDataRef*>(other_object)) {
		SomeData *o = dynamic_cast<SomeDataRef*>(oc->obj)->GetFinalObject();
		ndata = dynamic_cast<RoundedRectData *>(o);
		if (!ndata) return 0;
	}

	if (!ndata) return 0;

	if (data && data != ndata) deletedata();
	if (roc) delete roc;
	roc = oc->duplicate();

	if (data != ndata) {
		data = ndata;
		data->inc_count();
		UpdateViewportColor();
	}

	return 1;
}

int RoundedRectInterface::UseThisObject(ObjectContext *oc)
{
	return UseThisObject(oc, nullptr);
}

/*! \todo *** should push the RectInterface onto the viewport interfaces stack...
 */
int RoundedRectInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! should  Pop the RectInterface onto the viewport interfaces stack.
 */
int RoundedRectInterface::InterfaceOff()
{
	Clear(nullptr);
	showdecs   = 0;
	needtodraw = 1;
	curpoint   = RRECT_None;
	hover_point = RRECT_None;
	return 0;
}

void RoundedRectInterface::Clear(SomeData *d)
{
	if (!d || d == data) deletedata();
}

void RoundedRectInterface::deletedata()
{
	if (data) data->dec_count();
	data     = nullptr;
	curpoint = RRECT_None;
	hover_point = RRECT_None;
}

int RoundedRectInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
	if (!ndata || dynamic_cast<RoundedRectData *>(ndata) == nullptr) return 1;

	RoundedRectData *bzd = data;
	data = dynamic_cast<RoundedRectData *>(ndata);
	int td = showdecs;
	int ntd = needtodraw;
	showdecs   = 0;
	needtodraw = 1;

	Refresh();

	needtodraw = ntd;
	showdecs   = td;
	data       = bzd;
	return 1;
}

flatpoint _p1, _p2;

int RoundedRectInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;



	if (!data) {
		if (needtodraw) needtodraw=0;
		return 1;
	}

	if (roc && roc->obj && data != roc->obj) {
		double m[6];
		double m2[6];
		transform_invert(m, roc->obj->m());
		transform_mult(m2, m, data->m());
		dp->PushAndNewTransform(m2);
	}

	// DBG cerr <<"  RRect::Refresh showdecs: "<<showdecs<<"  width: "<<(data->linestyle ? data->linestyle->width : 0)<<endl;
		
	LineStyle *lstyle = data->linestyle;
	if (!lstyle) lstyle = DefaultLineStyle();
	FillStyle *fstyle = DefaultFillStyle();
	if (fstyle && fstyle->function == LAXOP_None) fstyle = nullptr;

	dp->NewFG(&lstyle->color);
	double thin = ScreenLine();
	dp->LineAttributes(lstyle->width, LineSolid, lstyle->capstyle, lstyle->joinstyle);

	// draw just box
	flatpoint pts[24];
	int n = data->GetPath(pts); // usually: c-v-c - c-v-c ...
	dp->moveto(pts[1]);
	for (int c=2; c<n-1; c+= 3) dp->curveto(pts[c], pts[c+1], pts[c+2]);
	dp->closed();

	if (fstyle) {
		dp->NewFG(&fstyle->color);
		dp->fill(1);
		dp->NewFG(&lstyle->color);
		dp->stroke(0);
	} else {
		dp->stroke(0);
	}
	
	 // draw control points;
	if (showdecs) {
		dp->LineWidthScreen(thin);
		dp->NewFG(&controlcolor);
		flatpoint p, center, start, end;


		// draw a plus at the center
		center = getpoint(RRECT_Center, false);
		dp->LineWidthScreen(thin * (curpoint == RRECT_Center ? 3 : 1));
		// double l = thin*10/dp->Getmag();
		// dp->drawline(center - data->x*l, data->center + data->x*l);
		// dp->drawline(center - data->y*l, data->center + data->y*l);
		//dp->drawthing(center, thin*10/dp->Getmag(), thin*10/dp->Getmag(), 0, THING_Plus);
		dp->LineWidthScreen(thin);

		if (1) {
			dp->drawrectangle(_p1.x, _p1.y, _p2.x - _p1.x, _p2.y - _p1.y, 0);
		}
	}

	if (roc && roc->obj && data != roc->obj) {
		dp->PopAxes();
	}

	needtodraw=0;
	return 0;
}

flatpoint RoundedRectInterface::getpoint(RRectPoints c, bool inparent)
{
	// if (!data || c==RRECT_WildPoint) return dp->screentoreal(hover_x,hover_y);
	return data->getpoint(c,inparent);
}


RRectPoints RoundedRectInterface::scan(double x,double y, unsigned int state)
{
	if (!data) return RRECT_None;

	flatpoint p2;
	flatpoint p = screentoreal(x,y);
	
	double dd, d = 1e+10, threshhold = NearThreshhold()/dp->Getmag();
	// DBGL("scan d="<<d<<"(x,y)="<<p.x<<','<<p.y);
	
	RRectPoints closest = RRECT_None;

	// first check normal points:
	static const RRectPoints pts[] = {
		 RRECT_Center,
		 // RRECT_Right,
		 // RRECT_Left,
		 // RRECT_Top,
		 // RRECT_Bottom,
		 RRECT_None
		};

	for (int i = 0; pts[i] != RRECT_None; i++) {
		p2 = getpoint(pts[i], true);
		dd = (p2-p).norm();
		if (dd < d) {
			d = dd;
			closest = pts[i];
		}
	}
	if (d < 3*threshhold) return closest;
	
	// flatpoint center = data->getpoint(RRECT_Center, true);


	
	DBG std::cerr << "rrect closest: d: "<<d<<"  "<<PointName(closest)<<std::endl;
	return closest;
}

int RoundedRectInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	DBGL("  in rrect lbd..");

	hover_x = x;
	hover_y = y;

	RRectPoints over = scan(x,y, state);

	if (data && over != RRECT_None) {
		buttondown.down(d->id,LEFTBUTTON,x,y, over);
		curpoint = over;

		// if (curpoint == RRECT_Right) 	    ref_point = data->getpoint(RRECT_Left, true);
		// else if (curpoint == RRECT_Top)  ref_point = data->getpoint(RRECT_Bottom, true);
		
		ref_point2 = data->getpoint(RRECT_Center, true);
		
		return 0; // other click with data
	}
	DBGL("  no rrect point found  ");

	deletedata();
	
	 // make new one
	RoundedRectData *obj = nullptr;
	ObjectContext *oc = nullptr;
	int c = viewport->FindObject(x,y,whatdatatype(),nullptr,1,&oc);
	if (c>0) obj = dynamic_cast<RoundedRectData *>(oc->obj);

	if (obj) { 
		 // found another RoundedRectData to work on.
		 // If this is primary, then it is ok to work on other rrect objs, but not click onto
		 // other types of objects.
		data = obj;
		data->inc_count();
		if (roc) delete roc;
		roc = oc->duplicate();

		if (viewport) viewport->ChangeObject(oc,0,true);
		buttondown.up(d->id,LEFTBUTTON);
		needtodraw=1;
		return 0;

	} else if (c < 0) {
		 // If there is some other non-image data underneath (x,y) and
		 // this is not primary, then switch objects, and switch tools to deal
		 // with that object.
		if (!primary && c==-1 && viewport->ChangeObject(oc,1,true)) {
			buttondown.up(d->id,LEFTBUTTON);
			return 1;
		}
	}


	buttondown.down(d->id,LEFTBUTTON,x,y, RRECT_DragRect);
	
	// To be here, must want brand new data plopped into the viewport context
	if (viewport) viewport->ChangeContext(x,y,nullptr);

	RoundedRectData *ndata = nullptr; //creates with 1 count
	ndata = dynamic_cast<RoundedRectData *>(somedatafactory()->NewObject(LAX_RRECTDATA));
	if (!ndata) ndata = new RoundedRectData;

	LineStyle *style = dynamic_cast<LineStyle*>(DefaultLineStyle()->duplicate(nullptr));
	ndata->InstallLineStyle(style);
	style->dec_count();
	
	FillStyle *fstyle = dynamic_cast<FillStyle*>(DefaultFillStyle()->duplicate(nullptr));
	ndata->InstallFillStyle(fstyle);
	fstyle->dec_count();
	
	if (viewport) {
		oc = nullptr;
		viewport->NewData(ndata,&oc);//viewport adds only its own counts
		if (roc) delete roc;
		if (oc) roc = oc->duplicate();
	}

	data = ndata;

	curpoint = RRECT_DragRect;
	flatpoint p = screentoreal(x,y);
	data->origin(p);
	// data->x = createx;
	// data->y = createy;
	data->width = data->height = 0;
	data->FindBBox();
	createp = p;

	needtodraw = 1;
	
	return 0;
}

int RoundedRectInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	int action = RRECT_None;
	int dragged = buttondown.up(d->id,LEFTBUTTON, &action);

	if (dragged < 5 && action != RRECT_None) {
		// no drag clicking, perhaps numerical edit...

		if (action == RRECT_DragRect) {
			// didn't drag enough, we shouldn't add a new object to the viewport
			PostMessage("IMPLEMENT NO NEW OBJECT ON NO NEW DRAG");
		}

		// else {
		// // *** number input if click on various things
		// const char *label = nullptr;
		// const char *msg = nullptr;
		// double f = 0;

		// if (action == RRECT_Right) {
		// 	f = data->a;
		// 	msg = "a_num";
		// 	label = _("X radius");

		// } else if (action == RRECT_Top) {
		// 	f = data->b;
		// 	msg = "b_num";
		// 	label = _("Y radius");

		// }

		// if (msg) {
		// 	char str[30];
		// 	sprintf(str, "%f", f);
		// 	LaxFont *font = curwindow->win_themestyle->normal;
		// 	double th = font->textheight();
		// 	DoubleBBox box(x-5*th, x+5*th, y-.75*th, y+.75*th);
		// 	viewport->SetupInputBox(object_id, label, str, msg, box);
		// }
		// }
	}

	curpoint = RRECT_None;
	needtodraw=1;
	return 0;
}

Laxkit::MenuInfo *RoundedRectInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!data) return menu;

	if (!menu) menu = new MenuInfo();

	ResourceManager *rm = GetResourceManager();

	ResourceType *resources = rm->FindType("LineStyle");
	resources->AppendResourceMenu(menu, _("Line Styles"), _("Use line style"), nullptr,
					RRECT_MAX, RRECT_UseLineStyle, RRECT_UseLineStyle,
					_("Make line style a resource"), RRECT_MakeLineResource,
					_("Make line style unique"), RRECT_MakeLineLocal,
					data->linestyle);

	resources = rm->FindType("FillStyle");
	resources->AppendResourceMenu(menu, _("Fill Styles"), _("Use fill style"), nullptr,
					RRECT_MAX, RRECT_UseFillStyle, RRECT_UseFillStyle,
					_("Make fill style a resource"), RRECT_MakeFillResource,
					_("Make fill style unique"), RRECT_MakeFillLocal,
					data->fillstyle);

	return menu;
}

int RoundedRectInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
	if (!data || !s || isblank(s->str)) return 1;
	double num = strtod(s->str, nullptr);

	// if (!strcmp(mes,"a_num")) {
	// 	if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
	// 	data->a = num;

	// } else if (!strcmp(mes,"b_num")) {
	// 	if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
	// 	data->b = num;

	// } else if (!strcmp(mes,"r_num")) {
	// 	// double old = (ref_point - data->center).norm();
	// 	// double scale = num/old;
	// 	if (num <= 0) { PostMessage(_("Number must be positive")); return 0; }
	// 	data->a *= num;
	// 	data->b *= num;

	// } else 
	if (!strcmp(mes,"set_width")) {
		if (num < 0) { PostMessage(_("Number must be non-negative")); return 0; }
		data->linestyle->width = num;

	} else if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i = s->info2; //id of menu item

		if (i > RRECT_None && i < RRECT_MAX) {
			PerformAction(i);
		} else if (i > RRECT_MAX) {
			//is a resource, shape brush?? line profile
			unsigned int obj_id = i - RRECT_MAX;
			ResourceManager *rm = GetResourceManager();

			if (s->info4 == RRECT_UseLineStyle) {
				Resource *resource = rm->FindResourceFromRID(obj_id, "LineStyle");
				if (resource) {
					LineStyle *style = dynamic_cast<LineStyle*>(resource->GetObject());
					if (style) {
						if (data) {
							data->InstallLineStyle(style);
							needtodraw = 1;
						}
					}
				}
			} else if (s->info4 == RRECT_UseFillStyle) {
				Resource *resource = rm->FindResourceFromRID(obj_id, "FillStyle");
				if (resource) {
					FillStyle *style = dynamic_cast<FillStyle*>(resource->GetObject());
					if (style) {
						if (data) {
							data->InstallFillStyle(style);
							needtodraw = 1;
						}
					}
				}
			}
		}

	} else return 1;

	data->FindBBox();
	Modified();
	needtodraw = 1;
	return 0;
}

int RoundedRectInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse) 
{
	hover_x = x;
	hover_y = y;

	if (!data) return 1;

	if (!buttondown.any()) {
		RRectPoints over = scan(x,y,state);
		if (over != curpoint) {
			curpoint = over;
			PostMessage(curpoint != RRECT_None ? PointMessage(curpoint) : nullptr);
			needtodraw = 1;
		}
		hover_point = curpoint;
		return 1;
	}

	int lx,ly;
	buttondown.move(mouse->id, x,y, &lx,&ly);

	flatpoint d = screentoreal(x,y)-screentoreal(lx,ly);

	flatpoint center = (flatvector(data->minx,data->miny) + flatvector(data->maxx, data->maxy))/2;
	double a = data->width/2;
	double b = data->height/2;
	flatvector xx(1,0);
	flatvector yy(0,1);

	if (curpoint != RRECT_None) {
		if (curpoint == RRECT_Center) {
			data->origin(data->origin() + data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(lx,ly)));
			Modified();
			needtodraw = 1;

		} else if (curpoint == RRECT_Right || curpoint == RRECT_Left
				|| curpoint == RRECT_Top || curpoint == RRECT_Bottom) {

			int do_x = (curpoint == RRECT_Right ? 1 : (curpoint == RRECT_Left ? -1 : 0));
			int do_y = (curpoint == RRECT_Top ? 1 : (curpoint == RRECT_Bottom ? -1 : 0));
			if (state & ShiftMask) {
				if (do_x) do_y = -1;
				else if (do_y) do_x = 1;
			}

			if (do_x) {
				flatpoint v = data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(lx,ly));
				double adiff = a;
				// data->width += (v*data->x)/data->x.norm() * do_x / 2.0;
				data->width += v.x * do_x / 2.0;
				if (data->width < 0) data->width = 0;
				adiff -= data->width;
				if (adiff && (state & ControlMask)) { //move origin


				}
				needtodraw = 1;
			}

			if (do_y) {
				flatpoint v = data->transformPointInverse(screentoreal(x,y)) - data->transformPointInverse(screentoreal(lx,ly));
				// data->height += (v*data->y)/data->y.norm() * do_y / 2.0;
				data->height += v.y * do_y / 2.0;
				if (data->height < 0) data->height = 0;
				needtodraw = 1;
			}

			if ((state & ControlMask) == 0) { //preserve opposite point
				flatpoint constant;
				if      (curpoint == RRECT_Right)  constant = data->getpoint(RRECT_Left, true);
				else if (curpoint == RRECT_Left)   constant = data->getpoint(RRECT_Right, true);
				else if (curpoint == RRECT_Top)    constant = data->getpoint(RRECT_Bottom, true);
				else if (curpoint == RRECT_Bottom) constant = data->getpoint(RRECT_Top, true);
				data->origin(data->origin() + ref_point - constant);
			} else { //preserve original center
				data->origin(data->origin() + ref_point2 - data->getpoint(RRECT_Center, true));
			}

			Modified();

		} else if (curpoint == RRECT_DragRect) {
			//shift: centered, control: circle
			int ix,iy;
			buttondown.getinitial(mouse->id,LEFTBUTTON,&ix,&iy);
			flatpoint p1 = screentoreal(ix,iy);
			flatpoint p2 = screentoreal(x,y);
			double mult = 1;
			if (state & ShiftMask) { // center new rect on initial point down
				mult = 2;
				data->origin(p1);
			} else {
				data->origin((p1+p2)/2);
			}
			p1 = data->transformPointInverse(p1);
			p2 = data->transformPointInverse(p2);
			_p1 = p1;
			_p2 = p2;
			double w = fabs((p2.x-p1.x) * mult);
			double h = fabs((p2.y-p1.y) * mult);
			if (state & ControlMask) { // square
				double d = MAX(w, h);
				data->width = data->height = d;

				// #define sgn(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
				// if ((state & ShiftMask) == 0)
				// 	data->origin(data->transformPoint(p1 + flatvector(sgn(p2.x-p1.x) * data->width/2, sgn(p2.y - p1.y) * data->height/2)));

			} else {
				data->width  = w;
				data->height = h;
			}

			DBGL("**************** width x height: "<<data->width<<" x "<< data->height);
			data->FindBBox();
			Modified();
			needtodraw = 1;

		}

		hover_x = x;
		hover_y = y;
		if (needtodraw) data->FindBBox();
	}

	return 0;
}

int RoundedRectInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{
	if (ch == LAX_Shift || ch == LAX_Control) { // intercept to update mouse overs
		DBG std::cerr << "fake mouse for ch "<<ch<<"  shift: "<<(state & ShiftMask)<<"  control: "<<(state&ControlMask)<<std::endl;
		LaxMouse *mouse = app->devicemanager->findMouse(0);
		MouseMove(hover_x, hover_y, state | (ch == LAX_Shift ? ShiftMask : 0) | (ch==LAX_Control ? ControlMask : 0), mouse);

	} else if (data && (ch == LAX_Bksp || ch == LAX_Del)) {
		bool found = false;
		
		// if (curpoint == RRECT_Right || curpoint == RRECT_Left) {
		// 	data->width = data->height;
		// 	found = true;
		// }

		// if (curpoint == RRECT_Top || curpoint == RRECT_Bottom) {
		// 	data->height = data->width;
		// 	found = true;
		// }

		if (found) {
			buttondown.clear();
			data->FindBBox();
			Modified();
			needtodraw = 1;
			curpoint = hover_point = RRECT_None;
		}

		return 0; //don't let delete propagate
	}

	 //check shortcuts
	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action >= 0) {
		return PerformAction(action);
	}
	
	return 1; 
}

int RoundedRectInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *kb) 
{
	if (ch == LAX_Shift || ch == LAX_Control) { // intercept to update mouse overs
		LaxMouse *mouse = app->devicemanager->findMouse(0);
		if (ch == LAX_Shift) state &= ~ShiftMask;
		else if (ch == LAX_Control) state &= ~ControlMask;
		MouseMove(hover_x, hover_y, 0, mouse);
		return 0;
	}

	return 1; 
}

Laxkit::ShortcutHandler *RoundedRectInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager = GetDefaultShortcutManager();
	sc = manager->NewHandler(whattype());
	if (sc) return sc;

	sc = new ShortcutHandler(whattype());

	sc->Add(RRECT_ToggleDecs,  'd',0,0, "ToggleDecs",    _("Toggle decorations"),nullptr,0);
	sc->Add(RRECT_ToggleSquare,'q',0,0, "ToggleSquare",  _("Toggle editing as a square"),nullptr,0);
	sc->Add(RRECT_SetWidth,    'w',0,0, "LineWidthEdit", _("Toggle line width edit"),nullptr,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

int RoundedRectInterface::PerformAction(int action)
{
	if (action == RRECT_ToggleDecs) {
		showdecs = !showdecs;
		needtodraw = 1;
		return 0;

	} else if (action == RRECT_ToggleSquare) {
		if (!data) return 0;
		if (data->width != data->height) {
			double max = MAX(data->width, data->height);
			data->width = data->height = max;
			Modified();
			needtodraw=1;
		}
		return 0;

	} else if (action == RRECT_SetWidth) {
		if (!data || !data->linestyle) return 0;
		char str[30];
		sprintf(str, "%f", data->linestyle->width);
		LaxFont *font = curwindow->win_themestyle->normal;
		double th = font->textheight();
		double x = (dp->Maxx + dp->Minx)/2;
		double y = (dp->Maxy + dp->Miny)/2;
		DoubleBBox box(x-5*th, x+5*th, y-.75*th, y+.75*th);
		viewport->SetupInputBox(object_id, _("Width"), str, "set_width", box);
		return 0;

	} else if (action == RRECT_MakeLineResource) {
		if (!data) return 0;
		if (data->linestyle->IsResourced()) {
			PostMessage(_("Line style is already a resource"));
		} else {
			InterfaceManager *imanager = InterfaceManager::GetDefault(true);
			ResourceManager *rm = imanager->GetResourceManager();
			rm->AddResource("LineStyle", data->linestyle, nullptr, data->linestyle->Id(), data->linestyle->Id(),
							nullptr, nullptr, nullptr, false);
			PostMessage(_("Resourced."));
		}
		return 0;

	} else if (action == RRECT_MakeLineLocal) {
		if (!data) return 0;
		LineStyle *ls = dynamic_cast<LineStyle*>(data->linestyle->duplicate(nullptr));
		data->InstallLineStyle(ls);
		ls->dec_count();
		PostMessage(_("Done."));
		return 0;

	} else if (action == RRECT_MakeFillResource) {
		if (!data) return 0;
		if (data->fillstyle->IsResourced()) {
			PostMessage(_("Fill style is already a resource"));
		} else {
			InterfaceManager *imanager = InterfaceManager::GetDefault(true);
			ResourceManager *rm = imanager->GetResourceManager();
			rm->AddResource("FillStyle", data->fillstyle, nullptr, data->fillstyle->Id(), data->fillstyle->Id(),
							nullptr, nullptr, nullptr, false);
			PostMessage(_("Resourced."));
		}
		return 0;

	} else if (action == RRECT_MakeFillLocal) {
		if (!data) return 0;
		FillStyle *s = dynamic_cast<FillStyle*>(data->fillstyle->duplicate(nullptr));
		data->InstallFillStyle(s);
		s->dec_count();
		PostMessage(_("Done."));
		return 0;
	}

	return 1;
}


} // namespace LaxInterfaces 

