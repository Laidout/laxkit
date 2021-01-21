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
//    Copyright (C) 2019 by Tom Lechner
//


#include <lax/spiro/spiro.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/laxutils.h>
#include <lax/utf8string.h>
#include <lax/language.h>

#include "simplepathinterface.h"


//template implementation:
#include <lax/lists.cc>



#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxFiles;

namespace LaxInterfaces {


//--------------------------- SimplePathData -------------------------------------

/*! \class SimplePathData
 * \ingroup interfaces
 * \brief Simple class to describe a line with various interpolations.
 */

SimplePathData::SimplePathData()
{
	color.grayf(.5);
	closed = 0;
	linewidth = 4;
	interpolation = Linear;

	needtoupdate = true;
	cache = nullptr;
	spiro_cache_n = 0;
	spiro_points = nullptr;
}

SimplePathData::~SimplePathData()
{
	if (cache) cache->dec_count();
	delete[] spiro_points;
}

const char *SimplePathData::PathTypeName(SimplePathData::Interpolation interpolation)
{
	switch(interpolation) {
		case SimplePathData::Linear:    return "Linear";
		case SimplePathData::Quadratic: return "Quadratic";
		case SimplePathData::Cubic:     return "Cubic";
		case SimplePathData::Spiro:     return "Spiro";
		case SimplePathData::NewSpiro:  return "NewSpiro";
	}
	return nullptr;
}

const char *SimplePathData::PointTypeName(SimplePathData::PointType pointtype)
{
	switch(pointtype) {
		case SimplePathData::Default     :    return "Default"     ;
		case SimplePathData::Smooth      :    return "Smooth"      ;
		case SimplePathData::Corner      :    return "Corner"      ;
		case SimplePathData::Spiro_G4    :    return "Spiro_G4"    ;
		case SimplePathData::Spiro_G2    :    return "Spiro_G2"    ;
		case SimplePathData::Spiro_Left  :    return "Spiro_Left"  ;
		case SimplePathData::Spiro_Right :    return "Spiro_Right" ;
		case SimplePathData::Spiro_Anchor:    return "Spiro_Anchor";
		case SimplePathData::Spiro_Handle:    return "Spiro_Handle";
	}
	return nullptr;
}

//-------------vvvvvvvvvvvvvvv-------- Old Spiro Callbacks --------------------
struct SimplePathSpiroCtx
{
	bezctx base;
	SimplePathData *data;
};

/* Called by spiro to start a contour */
void spd_moveto(bezctx *bc, double x, double y, int is_open)
{
	cerr << " moveto  "<<x<<", "<<y<<"  "<<is_open<<endl;

	SimplePathSpiroCtx *ctx = (SimplePathSpiroCtx*)bc;
	ctx->data->pointcache.push(flatpoint(x,y, LINE_Vertex));
}

/* Called by spiro to move from the last point to the next one on a straight line */
void spd_lineto(bezctx *bc, double x, double y)
{
	cerr << " lineto  "<<x<<", "<<y<<"  "<<endl;

	SimplePathSpiroCtx *ctx = (SimplePathSpiroCtx*)bc;
	ctx->data->pointcache.push(flatpoint(x,y, LINE_Vertex));
}

/* Called by spiro to move from the last point to the next along a quadratic bezier spline */
/* (x1,y1) is the quadratic bezier control point and (x2,y2) will be the new end point */
void spd_quadto(bezctx *bc, double x1, double y1, double x2, double y2)
{
	cerr << " quadto  "<<x1<<", "<<y1<<"  "
		 			   <<x2<<", "<<y2<<"  "<<endl;

	SimplePathSpiroCtx *ctx = (SimplePathSpiroCtx*)bc;
	ctx->data->pointcache.push(flatpoint(x1,y1, LINE_Bez));
	ctx->data->pointcache.push(flatpoint(x1,y1, LINE_Bez));
	ctx->data->pointcache.push(flatpoint(x2,y2, LINE_Vertex));
}

/* Called by spiro to move from the last point to the next along a cubic bezier spline */
/* (x1,y1) and (x2,y2) are the two off-curve control point and (x3,y3) will be the new end point */
void spd_curveto(bezctx *bc, double x1, double y1, double x2, double y2, double x3, double y3)
{
	cerr << " quadto  "<<x1<<", "<<y1<<"  "
		 			   <<x2<<", "<<y2<<"  "
		 			   <<x3<<", "<<y3<<"  "<<endl;

	SimplePathSpiroCtx *ctx = (SimplePathSpiroCtx*)bc;
	ctx->data->pointcache.push(flatpoint(x1,y1, LINE_Bez));
	ctx->data->pointcache.push(flatpoint(x2,y2, LINE_Bez));
	ctx->data->pointcache.push(flatpoint(x3,y3, LINE_Vertex));
}

void spd_mark_knot(bezctx *bc, int knot_idx)
{
	cerr << " mark knot  "<<knot_idx<<endl;
}

//-------------^^^^^^^^^^^^-------- Old Spiro Callbacks --------------------

// *** implement the full controls:
 /* Possible values of the "ty" field. */
#define SPIRO_CORNER	'v'
#define SPIRO_G4	'o'
#define SPIRO_G2	'c'
#define SPIRO_LEFT	'['
#define SPIRO_RIGHT	']'
#define SPIRO_ANCHOR	'a'
#define SPIRO_HANDLE	'h'

    /* For a closed contour add an extra cp with a ty set to */
#define SPIRO_END		'z'
    /* For an open contour the first cp must have a ty set to*/
#define SPIRO_OPEN_CONTOUR	'{'
    /* For an open contour the last cp must have a ty set to */
#define SPIRO_END_OPEN_CONTOUR	'}'

void SimplePathData::BuildSpiro()
{
	if (mpoints.n == 0) return;
	
	if (mpoints.n > spiro_cache_n) {
		delete[] spiro_points;
		spiro_points = new spiro_cp[mpoints.n];
		spiro_cache_n = mpoints.n;
	}

	//build list of spiro points to build bez from
	for (int c=0; c<mpoints.n; c++) {
		spiro_points[c].x = mpoints.e[c]->p.x;
		spiro_points[c].y = mpoints.e[c]->p.y;
		if (c == 0)
			spiro_points[c].ty = SPIRO_OPEN_CONTOUR;
		else if (c == mpoints.n-1)
			spiro_points[c].ty = SPIRO_END_OPEN_CONTOUR;
		else
			spiro_points[c].ty = SPIRO_G4;

	}

	//convert the spiro points to a bez line
	pointcache.flush();
	SimplePathSpiroCtx bc;
	bc.data = this;
	bc.base.moveto    = spd_moveto;
    bc.base.lineto    = spd_lineto;
    bc.base.quadto    = spd_quadto;
    bc.base.curveto   = spd_curveto;
    bc.base.mark_knot = spd_mark_knot;

	////run_spiro(spiro_points, mpoints.n);

	//SpiroCPsToBezier0(spiro_points, mpoints.n, closed, (_bezctx*)&bc);
	////bezctx_ps_close(bc);

	spiro_seg *seg = run_spiro(spiro_points, mpoints.n);
	if (seg) {
		spiro_to_bpath(seg, mpoints.n, (bezctx*)&bc);
		free_spiro(seg);
	}
}

void SimplePathData::BuildNewSpiro()
{
	if (mpoints.n == 0) return;
	
	newspiro.Clear();
	newspiro.isClosed = closed;

	//build list of spiro points to build bez from
	for (int c=0; c<mpoints.n; c++) {
		Point *p = mpoints.e[c];
		newspiro.AddControlPoint(p->p.x, p->p.y, p->type == Smooth ? 's' : 'c',
				p->ltype == Spiro_Left,  p->ltheta,
				p->rtype == Spiro_Right, p->rtheta
				);
	}

	newspiro.solve();
    newspiro.computeCurvatureBlending();
	for (unsigned int c=0; c<newspiro.ctrlPts.size(); c++) {
		NewSpiro::ControlPoint &pt = newspiro.ctrlPts[c];
		mpoints[c]->ltheta = pt.lth;
		mpoints[c]->rtheta = pt.rth;
	}
    std::shared_ptr<NewSpiro::BezPath> bezpath = newspiro.render();

	//convert the spiro points to a bez line
	pointcache.flush();

#define MOVETO     'M'
#define LINETO     'L'
#define CURVETO    'C'
#define CLOSEPATH  'Z'
#define MARK       '#'

	for (NewSpiro::BezPath::BezCommand &cmd : bezpath->commands) {
        char op = cmd.command;
        if (op == MOVETO) {
			if (pointcache.n) pointcache.e[pointcache.n-1].info |= LINE_Open;
			pointcache.push(flatvector(cmd.val[0], cmd.val[1]));

        } else if (op == LINETO) {
			pointcache.push(flatvector(cmd.val[0], cmd.val[1]));

        } else if (op == CURVETO) {
			pointcache.push(flatvector(cmd.val[0], cmd.val[1], LINE_Bez));
			pointcache.push(flatvector(cmd.val[2], cmd.val[3], LINE_Bez));
			pointcache.push(flatvector(cmd.val[4], cmd.val[5]));

        } else if (op == CLOSEPATH) {
			if (pointcache.n) pointcache.e[pointcache.n-1].info |= LINE_Closed;
        }
	}
}


void SimplePathData::Interpolate(Interpolation newInterpolation)
{
	if (interpolation == newInterpolation) return;
	pointcache.flush();
	interpolation = newInterpolation;
	UpdateInterpolation();
}

void SimplePathData::UpdateInterpolation()
{
	if (interpolation == Spiro) BuildSpiro();
	else if (interpolation == NewSpiro) BuildNewSpiro();
	needtoupdate = false;
}

void SimplePathData::FindBBox()
{
    //compute bounds in minx,maxx, miny,maxy.

	ClearBBox(); 

	for (int c=0; c<mpoints.n; c++) {
		addtobounds(mpoints.e[c]->p);
	}
}

SomeData *SimplePathData::duplicate(SomeData *dup)
{
	SimplePathData *newpath = dynamic_cast<SimplePathData*>(dup);
    if (!newpath && dup) return NULL; //was not an SimplePathData!

    if (!dup) {
        dup = dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_BEZDATA));
        newpath = dynamic_cast<SimplePathData*>(dup);
    }
    if (!newpath) {
        newpath = new SimplePathData();
        dup = newpath;
    }

	newpath->interpolation = interpolation;
	
	for (int c=0; c<mpoints.n; c++) {
		//newpath->points.push(points.e[c]);
		Point *p = mpoints.e[c];
		newpath->AddPoint(p->p, p->type, p->ltype, p->ltheta, p->rtype, p->rtheta, -1);
	}

	return newpath;
}

int SimplePathData::AddPoint(flatvector p, int where)
{
	return AddPoint(p, Smooth, Default,0, Default,0, where);
}

/*! Return index of new point.
 */
int SimplePathData::AddPoint(flatvector p, int type, int ltype, double ltheta, int rtype, double rtheta, int where)
{
	if (where < 0 || where > mpoints.n) where = mpoints.n;
	mpoints.push(new Point(p,type, ltype,ltheta, rtype,rtheta), 1, where);
	needtoupdate = true;
	return where;
}

void SimplePathData::RemoveAt(int index)
{
	mpoints.remove(index);
	needtoupdate = true;
}

void SimplePathData::TangentAtIndex(int index, flatpoint &prev, flatpoint &next)
{
	if (index < 0 || index >= mpoints.n) return;
	Point *point = mpoints.e[index];

	//ltheta and rtheta should have been updated during UpdateInterpolation()
	prev = flatpoint(cos(point->ltheta), sin(point->ltheta));
	next = flatpoint(cos(point->rtheta), sin(point->rtheta));

//	if (point->ltype == Spiro_Left) prev = flatpoint(cos(point->ltheta), sin(point->ltheta));
//	else {
//		// *** compute tangent...
//		prev = flatpoint(-1,0);
//	}
//
//	if (point->rtype == Spiro_Right) next = flatpoint(cos(point->rtheta), sin(point->rtheta));
//	else {
//		// *** compute tangent...
//		next = flatpoint(0,1);
//	}
}

void SimplePathData::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{
}

void SimplePathData::dump_out(FILE *f,int indent,int what,DumpContext *context)
{
    Attribute att;
    dump_out_atts(&att,what,context);
    att.dump_out(f,indent);
}

LaxFiles::Attribute *SimplePathData::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *savecontext)
{
	if (what == -1) {
		return att;
	}

	if (!att) att = new Attribute();

	Utf8String str;

	att->push("type", PathTypeName(interpolation));
	att->push("linewidth", linewidth);
	str.Sprintf("rgbaf(%.10g, %.10g, %.10g, %.10g)", color.Red(), color.Green(), color.Blue(), color.Alpha());
	att->push("color", str.c_str());
	if (closed) att->push("closed");

	if (mpoints.n) {
		Attribute *att2 = att->pushSubAtt("points");

		for (int c=0; c<mpoints.n; c++) {
			Point *point = mpoints.e[c];

			str.Sprintf("(%.10g, %.10g) %s %.10g %s %.10g", point->p.x, point->p.y,
					PointTypeName((SimplePathData::PointType)point->ltype), point->ltheta,
					PointTypeName((SimplePathData::PointType)point->rtype), point->rtheta);
			att2->push(PointTypeName((SimplePathData::PointType)point->type), str.c_str());
		}
	}

	return att;
}

char *SimplePathData::GetSvgPath()
{
	// *** BROKEN FIX ME!!! combine from Displayer::drawFormattedPoints
	
	if (!pointcache.n) return nullptr;

	Utf8String str, str2;

	int i=0;
	if ((pointcache.e[0].info & LINE_Bez) == 0) i=0;
	else if ((pointcache.e[1].info & LINE_Bez) == 0) i=1;
	else if ((pointcache.e[2].info & LINE_Bez) == 0) i=2;
		
	str2.Sprintf("M %.10g %.10g ", pointcache[i].x, pointcache[i].y);
	str.Append(str2);

	for (int c=i+1; c<pointcache.n; c++) {
		if ((pointcache[c].info & LINE_Bez) == 0) {
			//line
			str2.Sprintf("L %.10g %.10g ", pointcache[c].x, pointcache[c].y);
			str.Append(str2);
		} else {
			//curve, assume two control points
			str2.Sprintf("C %.10g %.10g %.10g %.10g %.10g %.10g ", pointcache[c].x, pointcache[c].y, pointcache[c+1].x, pointcache[c+1].y, pointcache[c+2].x, pointcache[c+2].y);
			str.Append(str2);
			c += 2;
		}

		if (pointcache[c].info & (LINE_Closed | LINE_Open)) {
			str.Append("z");
		}
	}

	char *sstr = str.ExtractBytes(nullptr, nullptr, nullptr);
	return sstr;
}


//--------------------------- SimplePathInterface -------------------------------------

/*! \class SimplePathInterface
 * \ingroup interfaces
 * \brief Exciting interface that does stuff.
 */


SimplePathInterface::SimplePathInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	interface_flags = 0;

	showdecs   = 1;
	showbez    = true;
	needtodraw = 1;

	dataoc     = NULL;
	data       = NULL;

	double ui_scale = app->theme->GetDouble(THEME_UI_Scale);
	if (ui_scale <= 0) ui_scale = 1;
	select_radius = 10 * ui_scale;
	point_radius  = 10 * ui_scale;
	theta_radius  = 40 * ui_scale; //expands when rotating handles
	min_theta_radius = theta_radius;
	max_theta_radius = theta_radius * 2;

	timerid       = 0;

	curpoint     = -1;
	curcontrol   = -1;
	hover        = -1;
	hover_handle = 0;

	sc = NULL; //shortcut list, define as needed in GetShortcuts()
}

SimplePathInterface::~SimplePathInterface()
{
	if (dataoc) delete dataoc;
	if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
}

const char *SimplePathInterface::whatdatatype()
{
	return "SimplePathData";
}

/*! Name as displayed in menus, for instance.
 */
const char *SimplePathInterface::Name()
{ return _("Simple Path"); }


//! Return new SimplePathInterface.
/*! If dup!=NULL and it cannot be cast to SimplePathInterface, then return NULL.
 */
anInterface *SimplePathInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new SimplePathInterface(NULL,id,NULL);
	else if (!dynamic_cast<SimplePathInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

//! Use the object at oc if it is an SimplePathData.
int SimplePathInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	SimplePathData *ndata=dynamic_cast<SimplePathData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) deletedata();
	if (dataoc) delete dataoc;
	dataoc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}
	needtodraw=1;
	return 1;
}

//! Decs count of data, and Sets to NULL.
void SimplePathInterface::deletedata()
{
    if (data) { data->dec_count(); data=NULL; }
    if (dataoc) { delete dataoc; dataoc=NULL; }
	curpoints.flush();
	curpoint = -1;
}


/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int SimplePathInterface::UseThis(anObject *nobj, unsigned int mask)
{
//	if (!nobj) return 1;
//	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
//	if (ls!=NULL) {
//		if (mask & (LINESTYLE_Color | LINESTYLE_Color2)) { 
//			linecolor=ls->color;
//		}
////		if (mask & LINESTYLE_Width) {
////			linecolor.width=ls->width;
////		}
//		needtodraw=1;
//		return 1;
//	}
	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *SimplePathInterface::Context()
{
	return dataoc;
}

/*! Called when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int SimplePathInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Called when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int SimplePathInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

/*! Clear references to d within the interface.
 */
void SimplePathInterface::Clear(SomeData *d)
{
	if (dataoc) { delete dataoc; dataoc=NULL; }
	if (data) { data->dec_count(); data=NULL; }
}

/*! Return a new cur that is between min and max.
 * delta is on a scale between 0 and 1.
 */
double EaseIn(double cur, double min, double max, double delta) 
{
	double x = asin((cur - min)/(max-min)) * 2 / M_PI;
	x += delta;
	double y = min + (max-min) * sin(x * M_PI / 2);
	if (x >= 1) y = max;
	return y;
}

int SimplePathInterface::Idle(int tid, double delta)
{
	if (tid != timerid) return 1; //1 means remove timer

	double speed = 5;
	double duration = .25;

	if (buttondown.any()) {
		//increase theta_radius
		DBG cerr << " Idle, delta: "<<delta<<"  radius: "<<theta_radius<<"  max: "<<max_theta_radius<<"  min: "<<min_theta_radius<<endl;
		if (theta_radius < max_theta_radius) {
			theta_radius = EaseIn(theta_radius, min_theta_radius, max_theta_radius, delta/duration);
			//theta_radius += (max_theta_radius - min_theta_radius) * delta * speed;
			needtodraw = 1;
			if (theta_radius > max_theta_radius - 1e-2) {
				theta_radius = max_theta_radius;
				timerid = 0;
				return 1;
			}
		}
		return 0;
	}

	//decrease theta_radius
	if (theta_radius > min_theta_radius) {
		theta_radius -= 2 * (max_theta_radius - min_theta_radius) * delta * speed;
		DBG cerr << " Idle, delta: "<<delta<<"  radius: "<<theta_radius<<"  max: "<<max_theta_radius<<"  min: "<<min_theta_radius<<endl;
		needtodraw = 1;
		if (theta_radius < min_theta_radius) {
			theta_radius = min_theta_radius;
			timerid = 0;
			return 1;
		}
	}
	
	return 0;
}

void SimplePathInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

/*! Return a context specific menu, typically in response to a right click.
 */
Laxkit::MenuInfo *SimplePathInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!data) return menu;

	if (!menu) menu = new MenuInfo;

	menu->AddSep(_("Interpolation"));
	menu->AddToggleItem(_("Linear   "), SIMPLEPATH_Linear   , 0, data->interpolation == SimplePathData::Linear   /*on*/);
	menu->AddToggleItem(_("Quadratic"), SIMPLEPATH_Quadratic, 0, data->interpolation == SimplePathData::Quadratic/*on*/);
	menu->AddToggleItem(_("Cubic    "), SIMPLEPATH_Cubic    , 0, data->interpolation == SimplePathData::Cubic    /*on*/);
	menu->AddToggleItem(_("Spiro    "), SIMPLEPATH_Spiro    , 0, data->interpolation == SimplePathData::Spiro    /*on*/);
	menu->AddToggleItem(_("NewSpiro "), SIMPLEPATH_NewSpiro , 0, data->interpolation == SimplePathData::NewSpiro /*on*/);

	if (curpoints.n > 0 && data->interpolation == SimplePathData::Spiro) {
		menu->AddSep(_("Point Type"));

		menu->AddItem(_("Corner"), SIMPLEPATH_Spiro_Corner);
		menu->AddItem(_("G4    "), SIMPLEPATH_Spiro_G4    );
		menu->AddItem(_("G2    "), SIMPLEPATH_Spiro_G2    );
		menu->AddItem(_("Left  "), SIMPLEPATH_Spiro_Left  );
		menu->AddItem(_("Right "), SIMPLEPATH_Spiro_Right );
		menu->AddItem(_("Anchor"), SIMPLEPATH_Spiro_Anchor);
		menu->AddItem(_("Handle"), SIMPLEPATH_Spiro_Handle);
		
		//    /* For a closed contour add an extra cp with a ty set to */
		//#define SPIRO_END		'z'
		//    /* For an open contour the first cp must have a ty set to*/
		//#define SPIRO_OPEN_CONTOUR	'{'
		//    /* For an open contour the last cp must have a ty set to */
		//#define SPIRO_END_OPEN_CONTOUR	'}'

	} else if (curpoints.n > 0 && data->interpolation == SimplePathData::NewSpiro) {
		menu->AddSep(_("Point Type"));

		menu->AddItem(_("Corner"), SIMPLEPATH_Spiro_Corner);
		menu->AddItem(_("Smooth"), SIMPLEPATH_Spiro_Smooth);
		menu->AddItem(_("Auto"  ), SIMPLEPATH_Spiro_Auto  );
	}

	if (data->mpoints.n) {
		menu->AddSep();
		menu->AddItem(_("Load"), SIMPLEPATH_Load);
		menu->AddItem(_("Save"), SIMPLEPATH_Save);
		menu->AddItem(_("Save as bezier"), SIMPLEPATH_Save_Bezier);
	}

	return menu;
}

/*! Intercept events if necessary, such as from the ContextMenu().
 */
int SimplePathInterface::Event(const Laxkit::EventData *evdata, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
		 //these are sent by the ContextMenu popup
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(evdata);
		int i	= s->info2; //id of menu item
		//int info = s->info4; //info of menu item

		if ( data) {
		  if ((i == (int)SIMPLEPATH_Linear   
			|| i == (int)SIMPLEPATH_Quadratic
			|| i == (int)SIMPLEPATH_Cubic    
			|| i == (int)SIMPLEPATH_Spiro    
			|| i == (int)SIMPLEPATH_NewSpiro)) {
			switch (i) {
				case SIMPLEPATH_Linear   : data->Interpolate(SimplePathData::Linear); break;
				case SIMPLEPATH_Quadratic: data->Interpolate(SimplePathData::Quadratic); break;
				case SIMPLEPATH_Cubic    : data->Interpolate(SimplePathData::Cubic); break;
				case SIMPLEPATH_Spiro    : data->Interpolate(SimplePathData::Spiro); break;
				case SIMPLEPATH_NewSpiro : data->Interpolate(SimplePathData::NewSpiro); break;
			}

			PostMessage(SimplePathData::PathTypeName(data->interpolation));
			needtodraw = 1;

		  } else if (i == SIMPLEPATH_Spiro_Corner
				  || i == SIMPLEPATH_Spiro_Smooth
				  || i == SIMPLEPATH_Spiro_Auto) {
			  PerformAction(i);

		  } else if (i == SIMPLEPATH_Save_Bezier) {
			  PostMessage(_("NEED TO IMPLEMENT SAVE BEZ"));
			  return 0;

		  } else if (i == SIMPLEPATH_Save) {
			  FILE *f = fopen("test.path", "w");
			  if (f) {
				  fprintf(f, "#SimplePathData\n");
				  data->dump_out(f, 0, 0, nullptr);
				  fclose(f);
				  PostMessage(_("Saved."));
			  }
			  return 0;

		  } else if (i == SIMPLEPATH_Load) {
			  PostMessage(_("NEED TO IMPLEMENT LOAD"));
			  return 0;

		  }

		}

		return 0; 
	}

	return 1; //event not absorbed
}


/*! Draw some data other than the current data.
 * This is called during screen refreshes.
 */
int SimplePathInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<SimplePathData *>(ndata) == nullptr) return 1;

	SimplePathData *bzd=data;
	data=dynamic_cast<SimplePathData *>(ndata);

	 // store any other state we need to remember
	 // and update to draw just the temporary object, no decorations
	int td=showdecs,ntd=needtodraw;
	showdecs=0;
	needtodraw=1;

	Refresh();

	 //now restore the old state
	needtodraw=ntd;
	showdecs=td;
	data=bzd;
	return 1;
}


int SimplePathInterface::Refresh()
{

	if (needtodraw == 0) return 0;
	needtodraw = 0;

	DBG cerr << "REFRESH "<<theta_radius<<endl;

	if (!data || data->mpoints.n == 0) return 0;

	if (data->needtoupdate) {
		data->UpdateInterpolation();
		data->FindBBox();
	}

//	 //draw some text name
//	dp->DrawScreen();
//	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
//	dp->NewFG(curwindow->win_colors->fg);
//	dp->textout((dp->Maxx+dp->Minx)/2,(dp->Maxy+dp->Miny)/2, "Blah!",,-1, LAX_CENTER);
//	dp->drawline(dp->Minx,dp->Miny, dp->Maxx,dp->Maxy);
//	dp->DrawReal();

	dp->NewFG(&data->color);
	dp->LineWidth(data->linewidth);

	if (data->interpolation == SimplePathData::Quadratic) {
//		dp->moveto(data->mpoints.e[0]);
//		for (int c=1; c<data->mpoints.n; c+=2) {
//			flatpoint cp = 
//			dp->curveto(data->mpoints.e[c], data->mpoints.e[c], data->mpoints.e[c+1]);
//
//		}

	} else if (data->interpolation == SimplePathData::Cubic) {
	} else if (data->interpolation == SimplePathData::Spiro) {
		if (data->pointcache.n) {
			dp->drawFormattedPoints(data->pointcache.e, data->pointcache.n, 0);
		}

	} else if (data->interpolation == SimplePathData::NewSpiro) {
		if (data->pointcache.n) {
			dp->drawFormattedPoints(data->pointcache.e, data->pointcache.n, 0);
		}

	} else { //just do linear
		dp->moveto(data->mpoints.e[0]->p);
		for (int c=1; c<data->mpoints.n; c++) {
			dp->lineto(data->mpoints.e[c]->p);
		}
		if (data->closed) dp->lineto(data->mpoints.e[0]->p);
		dp->stroke(0);
	}


	if (showdecs) {
		if (selection.validbounds()) {
		}

		if (showbez) {
			if (data->pointcache.n) {
				dp->NewFG(.2,.2,1.);
				dp->DrawScreen();
				// draw interface decorations on top of interface data
				for (int c=0; c<data->pointcache.n; c++) {
					dp->drawpoint(dp->realtoscreen(data->pointcache.e[c]), point_radius/2 * ((data->pointcache.e[c].info & LINE_Bez) ? .5 : 1), 1);
				}
				dp->DrawReal();

			}
		}

		dp->NewFG(&data->color);
		dp->DrawScreen();
		// draw interface decorations on top of interface data
		flatpoint p;
		double thin = InterfaceManager::GetDefault(true)->ScreenLine();
		dp->LineWidth(thin);

		for (int c=0; c<data->mpoints.n; c++) {
			bool sel = IsSelected(c);
			p = dp->realtoscreen(data->mpoints.e[c]->p);
			if (data->mpoints.e[c]->type == SimplePathData::Corner)
				dp->drawrectangle(p.x - point_radius, p.y - point_radius, 2*point_radius,2*point_radius, sel ? 1 : 0);
			else dp->drawpoint(p, point_radius, sel ? 1 : 0);

			if (sel && hover == c && hover_handle != 0) {
				//draw rotation handle indicator
				dp->drawpoint(p, theta_radius , 0);

				flatvector prev,next;
				data->TangentAtIndex(c, prev, next);
				dp->LineWidth(thin * (hover_handle == SIMPLEPATH_Handle || hover_handle == SIMPLEPATH_Left_Handle ? 3 : 1));
				dp->drawline(p, p - prev * theta_radius);
				dp->LineWidth(thin * (hover_handle == SIMPLEPATH_Handle || hover_handle == SIMPLEPATH_Right_Handle ? 3 : 1));
				dp->drawline(p, p + next * theta_radius);
				dp->LineWidth(thin);
			}
		}
		dp->DrawReal();
	}

	return 0;
}

bool SimplePathInterface::IsSelected(int i)
{
	if (!data) return false;
	for (int c=0; c<curpoints.n; c++) {
		if (curpoints.e[c] == i) return true;
	}
	return false;
}

/*! Check for clicking down on other objects, possibly changing control to that other object.
 *
 * Return 1 for changed object to another of same type.
 * Return 2 for changed to object of another type (switched tools).
 * Return 0 for nothing found at x,y.
 */
int SimplePathInterface::OtherObjectCheck(int x,int y,unsigned int state) 
{
	ObjectContext *oc=NULL;
	int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
	SomeData *obj=NULL;
	if (c>=0 && oc && oc->obj && draws(oc->obj->whattype())) obj=oc->obj;

	if (obj) { 
		 // found another SimplePathData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		UseThisObject(oc); 
		if (viewport) viewport->ChangeObject(oc,0,true);
		needtodraw=1;
		return 1;

	} else if (c<0) {
		 // If there is some other type of data underneath (x,y).
		 // if *this is not primary, then switch objects, and switch tools to deal
		 // with that object.
		//******* need some way to transfer the LBDown to the new tool
		if (!primary && c==-1 && viewport->ChangeObject(oc,1,true)) {
			buttondown.clear();
			//buttondown.up(d->id,LEFTBUTTON);
			return 2;
		}
	}

	return 0;
}

int SimplePathInterface::scan(double x, double y, unsigned int state, int *handle_ret, int *closest_i_ret, double *closest_dist_ret)
{
	if (!data || data->mpoints.n == 0) return -1;

	flatpoint p = flatpoint(x,y);
	double d;
	double closest_dist = 1e+10;
	int closest_i = -1;

	for (int c=0; c<data->mpoints.n; c++) {
		flatpoint fp = realtoscreen(data->transformPoint(data->mpoints.e[c]->p));
		d = (fp-p).norm();
		if (d < closest_dist) {
			closest_dist = d;
			closest_i = c;
		}
	}

	if (closest_i_ret) *closest_i_ret = closest_i;
	if (closest_dist_ret) *closest_dist_ret = closest_dist;
	//int mult = (buttondown.any() ? 2 : 1);
	if (handle_ret) {
		if (closest_dist < theta_radius && closest_dist >= select_radius) {
			SimplePathData::Point *point = data->mpoints.e[closest_i];

			if (point->type == SimplePathData::Corner) {
				flatvector prev,next;
				data->TangentAtIndex(closest_i, prev, next);
				flatpoint pp = data->mpoints.e[closest_i]->p;
				flatpoint fp = data->transformPointInverse(screentoreal(x,y));
				double dp = (fp - (pp - prev)).norm2();
				double dn = (fp - (pp + next)).norm2();
				DBG cerr << " scan handle dist: dp: "<<dp <<"  dn: "<<dn<<endl;
				if (dp < dn) *handle_ret = SIMPLEPATH_Left_Handle;
				else *handle_ret = SIMPLEPATH_Right_Handle;

			} else *handle_ret = SIMPLEPATH_Handle;

		} else *handle_ret = 0;
	}

	if (data->interpolation == SimplePathData::NewSpiro) {
		if (closest_dist < theta_radius) return closest_i;
	} else if (closest_dist < select_radius) return closest_i;
	return -1;
}

int SimplePathInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{

	int nhandlehover = 0;
	double closest_dist;
	int nhover = scan(x,y,state, &nhandlehover, nullptr, &closest_dist);
	if (nhover != hover || nhandlehover != hover_handle) {
		hover_handle = nhandlehover;
		hover = nhover;
		needtodraw=1;
	}

	if (nhover >= 0) {
		buttondown.down(d->id,LEFTBUTTON,x,y, nhover, hover_handle);

		if (IsSelected(nhover) && closest_dist > select_radius && closest_dist < theta_radius) {
			SimplePathData::Point *point = data->mpoints.e[nhover];
			point->ltype = SimplePathData::Spiro_Left;
			point->rtype = SimplePathData::Spiro_Right;

			if (!timerid) timerid = app->addtimer(this, 1./30*1000, 1/30.*1000, -1);

		} else {
			if ((state & ShiftMask) != 0) curpoints.pushnodup(nhover);
			else {
				curpoints.flush();
				curpoints.push(nhover);
			}
		}

		needtodraw=1;
		return 0;
	}

//	 // Check for clicking down on controls for existing data
//	if (data && data->pointin(screentoreal(x,y))) {
//		int action1 = something;
//		int action2 = something_else;
//		buttondown.down(d->id,LEFTBUTTON,x,y, action1, action2);
//
//		if ((state&LAX_STATE_MASK)==0) {
//			//plain click in object. do something!
//			return 0;
//		}
//
//		//do something else for non-plain clicks!
//		return 0;
//	}

	if (data) {
		//append a new point
		flatpoint p = data->transformPointInverse(screentoreal(x,y));
		int index = data->AddPoint(p, curpoint >= 0 ? curpoints.e[curpoint]+1 : 0);
		curpoint = 0;
		curpoints.flush();
		curpoints.push(index);
		buttondown.down(d->id,LEFTBUTTON,x,y, nhover);
		needtodraw = 1;
		return 0;
	}

	 //clicked down on nothing, release current data if it exists
	deletedata();

	
	 // So, was clicked outside current object or on blank space, make new one or find other one.
	int other = OtherObjectCheck(x,y,state);
	if (other==2) return 0; //control changed to some other tool
	if (other==1) return 0; //object changed via UseThisObject().. nothing more to do here!

	 //OtherObjectCheck:
	 //  change to other type of object if not primary
	 //  change to other of same object always ok


	 // To be here, must want brand new data plopped into the viewport context
	if (true) {
		//NewDataAt(x,y,state);

		if (viewport) viewport->ChangeContext(x,y,NULL);
		data = newData();
		needtodraw=1;
		if (!data) return 0;

		if (viewport) {
			ObjectContext *oc=NULL;
			viewport->NewData(data,&oc);//viewport adds only its own counts
			if (dataoc) delete dataoc;
			if (oc) dataoc = oc->duplicate();
		}
		
		flatpoint p = data->transformPointInverse(screentoreal(x,y));
		data->AddPoint(p, -1);
		curpoint = 0;
		curpoints.push(data->mpoints.n-1);
		buttondown.down(d->id,LEFTBUTTON,x,y, nhover);

		 //for instance...
		//leftp = screentoreal(x,y);
		//data->origin(leftp);
		//data->xaxis(flatpoint(1,0)/Getmag()/2);
		//data->yaxis(flatpoint(0,1)/Getmag()/2);
		//DBG data->dump_out(stderr,6,0,NULL);

	} else {
		//we have some other control operation in mind...
	}

	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

SimplePathData *SimplePathInterface::newData()
{

	SimplePathData *newpath = dynamic_cast<SimplePathData*>(somedatafactory()->NewObject(LAX_BEZDATA));
    if (!newpath) {
        newpath = new SimplePathData();
    }
	newpath->interpolation = SimplePathData::NewSpiro;
	return newpath;
}


int SimplePathInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	needtodraw = 1;
	//hover = -1;
	//hover_handle = 0;

	//theta_radius = min_theta_radius;
	if (!timerid) timerid = app->addtimer(this, 1./30*1000, 1/30.*1000, -1);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int SimplePathInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		// update any mouse over state
		int nhandlehover = 0;
		int nhover = scan(x,y,state, &nhandlehover);
		if (nhover != hover || nhandlehover != hover_handle) {
			hover_handle = nhandlehover;
			hover = nhover;
			DBG cerr << "hover: "<<hover<<"  handle: "<<hover_handle<<endl;

			//PostMessage(_("Something based on new hover value"));
			needtodraw = 1;
			return 0;
		}
		return 1;
	}

	//else deal with mouse dragging...

	int oldx, oldy;
    int over, handle;
    buttondown.move(d->id,x,y, &oldx,&oldy);
    buttondown.getextrainfo(d->id,LEFTBUTTON, &over, &handle);

	flatpoint oldp = data->transformPointInverse(screentoreal(oldx,oldy));
	flatpoint newp = data->transformPointInverse(screentoreal(x,y));
	
	if (handle != 0) {
		flatvector p = data->mpoints.e[over]->p;
		flatvector v = oldp - p;
		double aOld = atan2(v.y, v.x);
		double dOld = v.norm();
		v = newp - p;
		double aNew = atan2(v.y, v.x);
		double dNew = v.norm();
		double da = aNew - aOld;

		for (int c=0; c<curpoints.n; c++) {
			SimplePathData::Point *point = data->mpoints.e[curpoints.e[c]];

			double rad = theta_radius;
			if (dOld < rad && dNew >= rad) {
				//make auto points
				if (handle == SIMPLEPATH_Handle || handle == SIMPLEPATH_Left_Handle)
					point->ltype = SimplePathData::Default;
				if (handle == SIMPLEPATH_Handle || handle == SIMPLEPATH_Right_Handle)
					point->rtype = SimplePathData::Default;

			} else if (dOld >= rad && dNew < rad) {
				//make manual points
				point->ltype = SimplePathData::Spiro_Left;
				point->rtype = SimplePathData::Spiro_Right;
			} 

			if (point->ltype == SimplePathData::Spiro_Left) {
				if (handle == SIMPLEPATH_Handle || handle == SIMPLEPATH_Left_Handle)
					point->ltheta += da;
			}
			if (point->rtype == SimplePathData::Spiro_Right) {
				if (handle == SIMPLEPATH_Handle || handle == SIMPLEPATH_Right_Handle)
					point->rtheta += da;
			}
		}

	} else { //normal move

		flatpoint dv = newp - oldp;
		DBG cerr << "move "<<dv.x<<','<<dv.y<<endl;
		for (int c=0; c<curpoints.n; c++) {
			data->mpoints.e[curpoints.e[c]]->p += dv;
		}
	}

	data->UpdateInterpolation();
	data->FindBBox();
	needtodraw=1;
	return 0; //MouseMove is always called for all interfaces, return value doesn't inherently matter
}

//int SimplePathInterface::WheelUp(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
//{ ***
//	return 1; //wheel up ignored
//}
//
//int SimplePathInterface::WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
//{ ***
//	return 1; //wheel down ignored
//}


int SimplePathInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"SimplePathInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

void SimplePathInterface::ClearSelection()
{
	curpoints.flush();
	curpoint = -1;
	needtodraw = 1;
}

int SimplePathInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
//	if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask|AltMask|MetaMask)) {
//		//deal with various modified keys...
//	}

	if (ch == LAX_Esc) { //the various possible keys beyond normal ascii printable chars are defined in lax/laxdefs.h
		if (curpoints.n == 0) return 1; //need to return on plain escape, so that default switching to Object tool happens
		
		 //else..
		ClearSelection();
		needtodraw=1;
		return 0;

	} else if (data && curpoints.n && (ch == LAX_Bksp || ch == LAX_Del)) {
		for (int c=0; c<curpoints.n; c++) {
			data->RemoveAt(curpoints.e[c]);
			for (int c2=c+1; c2<curpoints.n; c2++) {
				if (curpoints.e[c2] > curpoints.e[c]) curpoints.e[c2]--;
			}
		}
		curpoints.flush();

		data->UpdateInterpolation();
		data->FindBBox();
		needtodraw=1;
		return 0;
	
	} else {
		 //default shortcut processing

		if (!sc) GetShortcuts();
		if (!sc) return 1;
		int action = sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
		if (action >= 0) {
			return PerformAction(action);
		}
	}

	return 1; //key not dealt with, propagate to next interface
}


Laxkit::ShortcutHandler *SimplePathInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager = GetDefaultShortcutManager();
    sc = manager->NewHandler(whattype());
    if (sc) return sc;

    sc=new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);

    sc->Add(SIMPLEPATH_ShowBez,  'b',0,0, "ShowBez", _("Show cached bezier points"),NULL,0);
    sc->Add(SIMPLEPATH_ToggleClosed,  'c',0,0, "ToggleClosed", _("Toggle closed path"),NULL,0);

//    sc->Add(SIMPLEPATH_Something2, 'b',ControlMask,0, "BottomJustify"  , _("Bottom Justify"  ),NULL,0);
//    sc->Add(SIMPLEPATH_Something3, 'd',ControlMask,0, "Decorations"    , _("Toggle Decorations"),NULL,0);
//	sc->Add(SIMPLEPATH_Something4, '+',ShiftMask,0,   "ZoomIn"         , _("Zoom in"),NULL,0);
//	sc->AddShortcut('=',0,0, SIMPLEPATH_Something); //add key to existing action

    manager->AddArea(whattype(),sc);

    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int SimplePathInterface::PerformAction(int action)
{
	if (action == SIMPLEPATH_ShowBez) {
		PostMessage(_("Toggle show cache."));
		showbez = !showbez;
		needtodraw=1;
		return 0;

	} else if (action == SIMPLEPATH_ToggleClosed) {
		PostMessage(_("Toggle closed."));
		data->closed = !data->closed;
		data->UpdateInterpolation();
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action == SIMPLEPATH_Spiro_Corner) {
		if (!data) return 0;
		for (int c=0; c<curpoints.n; c++) {
			data->mpoints.e[curpoints.e[c]]->type = SimplePathData::Corner;
		}
		data->UpdateInterpolation();
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action == SIMPLEPATH_Spiro_Smooth) {
		if (!data) return 0;
		for (int c=0; c<curpoints.n; c++) {
			data->mpoints.e[curpoints.e[c]]->type = SimplePathData::Smooth;
			data->mpoints.e[curpoints.e[c]]->rtheta = data->mpoints.e[curpoints.e[c]]->ltheta;
			data->mpoints.e[curpoints.e[c]]->rtype = data->mpoints.e[curpoints.e[c]]->ltype = SimplePathData::Default;
		}
		data->UpdateInterpolation();
		data->FindBBox();
		needtodraw=1;
		return 0;

	} else if (action == SIMPLEPATH_Spiro_Auto) {
		if (!data) return 0;
		for (int c=0; c<curpoints.n; c++) {
			SimplePathData::Point *point = data->mpoints.e[curpoints.e[c]];
			point->type = SimplePathData::Smooth;
			point->ltype = point->rtype = SimplePathData::Default;
		}
		data->UpdateInterpolation();
		data->FindBBox();
		needtodraw=1;
		return 0;
	}
	return 1;
}

} // namespace LaxInterfaces

