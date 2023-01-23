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
//    Copyright (C) 2004-2006,2011,2014 by Tom Lechner
//



#include <lax/interfaces/freehandinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/language.h>

//template implementation:
#include <lax/refptrstack.cc>
#include <lax/lists.cc>

#include <unistd.h>

#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class FreehandInterface
 * \ingroup interfaces
 * \brief Interface to create bezier or straight lines from freehand motion.
 *
 *   On mouse down, this records all mouse movement, and converts
 *   the points into a bez curve (linear or cubic) on mouse up.
 *
 * \todo *** make closed when final point is close to first point
 */


FreehandInterface::FreehandInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	//freehand_style = FREEHAND_Poly_Path;
	//freehand_style = FREEHAND_Bez_Path;
	//freehand_style = FREEHAND_Bez_Outline;
	//freehand_style = FREEHAND_Color_Mesh;
	freehand_style = FREEHAND_Double_Mesh;
	mode = MODE_Normal;

	linecolor .rgbf(0,0,.5); //control line color
	pointcolor.rgbf(.5,.5,.5);

	default_fillstyle.color.rgbf(.6,0.,0.);
	default_linestyle.color.rgbf(1.0, 0., 0.);
	
	smooth_pixel_threshhold = 2;
	brush_size              = .1; //real units
	close_threshhold        = -1;
	ignore_tip_time         = 10;
	ignore_clock_t          = ignore_tip_time * sysconf(_SC_CLK_TCK) / 1000;

	showdecs   = 1;
	needtodraw = 1;
	size_dragged = false;

	shape_brush  = nullptr;
	line_profile = nullptr;

	linestyle = nullptr;
	fillstyle = nullptr;

	default_gradient.AddColor(0,  1., 0., 0., 1.);
	default_gradient.AddColor(.5, 0., 1., 0., 1.);
	default_gradient.AddColor(1,  0., 0., 1., 1.);
	default_gradient.SetLinear(flatpoint(0,0), flatpoint(1,0), 1,1);

	sc = nullptr;
}

FreehandInterface::~FreehandInterface()
{
	if (shape_brush) shape_brush->dec_count();
	if (line_profile) line_profile->dec_count();
	if (linestyle) linestyle->dec_count();
	if (fillstyle) fillstyle->dec_count();
	if (sc) sc->dec_count();
}

const char *FreehandInterface::Name()
{ return _("Freehand Lines"); }


//! Return new FreehandInterface.
/*! If dup!=NULL and it cannot be cast to ImageInterface, then return NULL.
 */
anInterface *FreehandInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new FreehandInterface(NULL,id,NULL);
	else if (!dynamic_cast<FreehandInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

/*! Returns old number of milliseconds.
 */
int FreehandInterface::IgnoreTipTime(int milliseconds)
{
	//int old=ignore_clock_t=1000*ignore_clock_t/sysconf(_SC_CLK_TCK);
	int old=ignore_tip_time;
	ignore_tip_time=milliseconds;
	if (milliseconds==0) ignore_clock_t=0;
	else ignore_clock_t= milliseconds*sysconf(_SC_CLK_TCK)/1000;
	return old;
}
int FreehandInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;
	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
	if (ls!=NULL) {
		if (mask & LINESTYLE_Color) {
			if (linestyle) linestyle->color = ls->color;
			default_linestyle.color = ls->color;
		}
		if (mask & LINESTYLE_Color2) {
			if (fillstyle) fillstyle->color = ls->color;
			default_fillstyle.color = ls->color;
		}
		if (mask & LINESTYLE_Width) {
			if (linestyle) linestyle->width = ls->width;
			default_linestyle.width = ls->width;
		}
		needtodraw=1;
		return 1;
	}
	return 0;
}

int FreehandInterface::InterfaceOn()
{ 
	showdecs=1;
	needtodraw=1;
	if (close_threshhold <= 0) close_threshhold = NearThreshhold();
	return 0;
}

int FreehandInterface::InterfaceOff()
{ 
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void FreehandInterface::Clear(SomeData *d)
{
	lines.flush();
	deviceids.flush();
}

Laxkit::MenuInfo *FreehandInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (freehand_style&FREEHAND_Lock_Type) return NULL;

	if (!menu) menu=new MenuInfo;
	else menu->AddSep(_("Freehand"));

	menu->AddToggleItem(_("Create raw points"),               FREEHAND_Raw_Path ,    0, (freehand_style & FREEHAND_Raw_Path )   != 0);
	menu->AddToggleItem(_("Create simplified polyline"),      FREEHAND_Poly_Path ,   0, (freehand_style & FREEHAND_Poly_Path )  != 0);
	menu->AddToggleItem(_("Create bezier line"),              FREEHAND_Bez_Path ,    0, (freehand_style & FREEHAND_Bez_Path )   != 0);
	menu->AddToggleItem(_("Create bezier outline"),           FREEHAND_Bez_Outline,  0, (freehand_style & FREEHAND_Bez_Outline) != 0);
	menu->AddToggleItem(_("Create bezier with weight nodes"), FREEHAND_Bez_Weighted, 0, (freehand_style & FREEHAND_Bez_Weighted)!= 0);
	menu->AddToggleItem(_("Create color mesh"),               FREEHAND_Color_Mesh,   0, (freehand_style & FREEHAND_Color_Mesh)  != 0);
	menu->AddToggleItem(_("Create symmetric color mesh"),     FREEHAND_Double_Mesh,  0, (freehand_style & FREEHAND_Double_Mesh) != 0);
	menu->AddToggleItem(_("Create grid mesh"),                FREEHAND_Grid_Mesh,    0, (freehand_style & FREEHAND_Grid_Mesh)   != 0);

	// menu->AddItem(_("Use shape brush"), FREEHAND_Use_Shape, LAX_ISTOGGLE|((freehand_style&FREEHAND_Bez_Weighted)?LAX_CHECKED:0), 0);
	// menu->AddItem(_("Select shape for shape brush..."), FREEHAND_Select_Shape, LAX_ISTOGGLE|((freehand_style&FREEHAND_Bez_Weighted)?LAX_CHECKED:0), 0);

	return menu;
}


int FreehandInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
        int i =s->info2; //id of menu item
        //int ii=s->info4; //extra id, 1 for direction
		DBG cerr <<" ********* freehand got menuevent..."<<i<<endl;

		if (i==FREEHAND_Raw_Path    ) {
			freehand_style=FREEHAND_Raw_Path;

		} else if (i==FREEHAND_Poly_Path   ) {
			freehand_style=FREEHAND_Poly_Path;

		} else if (i==FREEHAND_Bez_Path    ) {
			freehand_style=FREEHAND_Bez_Path;

		} else if (i==FREEHAND_Bez_Outline ) {
			freehand_style=FREEHAND_Bez_Outline;

		} else if (i==FREEHAND_Bez_Weighted ) {
			freehand_style=FREEHAND_Bez_Weighted;

		} else if (i==FREEHAND_Grid_Mesh        ) {
			freehand_style=FREEHAND_Grid_Mesh;

		} else if (i==FREEHAND_Color_Mesh        ) {
			freehand_style=FREEHAND_Color_Mesh;

		} else if (i==FREEHAND_Double_Mesh        ) {
			freehand_style=FREEHAND_Double_Mesh;
		}

		return 0;

	} else if (!strcmp(mes, "brushsize")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);

		double d=0;
		if (DoubleAttribute(s->str, &d)) {
			if (d < 0) d = 0;
			brush_size = d;
			needtodraw =1;
			return 0;
		}
	}
	
	return 1;
}

int FreehandInterface::Refresh()
{ 
	//DBG cerr <<"  Freehand trying to startdrawing"<<endl;

	if (needtodraw==0) return 0;
	needtodraw=0;

	if (mode == MODE_BrushSize) {
		dp->NewFG(linecolor);
		dp->DrawScreen();
		dp->LineWidthScreen(ScreenLine());
		dp->drawcircle(size_center, brush_size/2 * Getmag(), 0);
		dp->font(app->defaultlaxfont);
		char str[30];
		sprintf(str, "%g", brush_size);
		dp->textout(size_center.x, size_center.y - brush_size/2 * Getmag(), str,-1, LAX_HCENTER|LAX_BOTTOM);
		dp->DrawReal();
		return 0;
	}

	if (mode == MODE_Settings) {
		RefreshSettings();
		return 0;
	}

	if (lines.n==0) return 0;
	
	dp->LineAttributes(-1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->LineWidthScreen(1);

	RawPointLine *line;
	for (int c=0; c<lines.n; c++) {
		line=lines.e[c];
		if (line->n==0) continue;

		 // draw curve
		dp->NewFG(&linecolor);
		for (int c2=0; c2<line->n; c2++) {
			if (c2==0) dp->moveto(line->e[c2]->p);
			else dp->lineto(line->e[c2]->p);
		}
		dp->stroke(0);

		if (freehand_style&(FREEHAND_Bez_Outline|FREEHAND_Bez_Weighted|FREEHAND_Mesh)) {
			 //draw pressure indicator
			dp->NewFG(1.,0.,1.,1.);
			flatvector vt;
			dp->moveto(line->e[0]->p);
			for (int c2=1; c2<line->n-1; c2++) {
				if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;
				vt=line->e[c2+1]->p - line->e[c2-1]->p;
				vt=transpose(vt);
				vt.normalize();
				vt*=brush_size*line->e[c2]->pressure;
				//dp->drawline(line->e[c2]->p + vt, line->e[c2]->p - vt);
				dp->lineto(line->e[c2]->p + vt);
			}
			dp->stroke(0);

			dp->moveto(line->e[0]->p);
			for (int c2=1; c2<line->n-1; c2++) {
				if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;
				vt=line->e[c2+1]->p - line->e[c2-1]->p;
				vt=transpose(vt);
				vt.normalize();
				vt*=brush_size*line->e[c2]->pressure;
				//dp->drawline(line->e[c2]->p + vt, line->e[c2]->p - vt);
				dp->lineto(line->e[c2]->p - vt);
			}
			dp->stroke(0);
		}


		 // draw control points
		if (showdecs) {
			dp->NewFG(&pointcolor);
			 // draw little circles
			for (int c2=0; c2<line->n; c2++) {
				dp->drawpoint(line->e[c2]->p,2,1);
			}
		}
	}

	//DBG cerr <<"   Freehand all done drawing.\n";
	return 0;
}

void FreehandInterface::SetupSettings()
{
	flatpoint icon_raw[10];
	for (int c=0; c<10; c++) {
		icon_raw[c].set(4 * c/10. -2, (1 + .05*rand()/(float(RAND_MAX))) * sin(c/10. * 2*M_PI)); //-2..2, -1..1
	}

	flatpoint icon_polyline[7];
	for (int c=0; c<7; c++) {
		icon_polyline[c].set(4 * c/7. -2, sin(c/7. * 2*M_PI)); //-2..2, -1..1
	}

	flatpoint icon_bezline[4];
	icon_bezline[0] = flatpoint(-2,0);
	icon_bezline[1] = flatpoint(-2,0);
	icon_bezline[2] = flatpoint(-2,0);
	icon_bezline[3] = flatpoint(2,0);
}

void FreehandInterface::RefreshSettings()
{

}

int FreehandInterface::findLine(int id)
{
	for (int c=0; c<deviceids.n; c++) if (deviceids[c]==id) return c;
	return -1;
}

//! Start a new freehand line.
int FreehandInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"freehandlbd:"<<x<<','<<y<<"..";
	buttondown.down(d->id,LEFTBUTTON,x,y);

	if (mode == MODE_BrushSize) {
		return 0;
	}

	int i=findLine(d->id);
	if (i>=0) {
		lines.remove(i);
		deviceids.remove(i);
	}

	DBG cerr <<"../freehand Lbd\n";
	return 0;
}

//! Finish a new freehand line by calling newData with it.
int FreehandInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"  freehandLbup..";

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	if (mode == MODE_BrushSize) {
		Mode(MODE_Normal);
		if (!size_dragged && dragged < DraggedThreshhold()) {
			// *** pop up number input for brush_size
			char str[30];
			sprintf(str, "%g", brush_size);
			double th = app->defaultlaxfont->textheight();
			DoubleBBox bounds(x-4*th, x+4*th, y-th/2, y+th/2);
			viewport->SetupInputBox(object_id, NULL, str, "brushsize", bounds);
		}
		return 0;
	}

	// else MODE_Normal
	int i=findLine(d->id);
	if (i>=0) {
		DBG cerr <<"  *** FreehandInterface should check for closed path???"<<endl;

		if (dragged && lines.e[i]->n>1) {
			if (ignore_clock_t) {
				clock_t toptime=lines.e[i]->e[lines.e[i]->n-1]->time;
				while (toptime-lines.e[i]->e[lines.e[i]->n-1]->time<ignore_clock_t)
					lines.e[i]->pop();
			}
			if (lines.e[i]->n>1) send(i);
		}

		deviceids.remove(i);
		lines.remove(i);

	} //else line missing! do nothing

	if (freehand_style&FREEHAND_Remove_On_Up) {
		if (owner) owner->RemoveChild();
		else if (viewport) viewport->Pop(this,1);
	}

	needtodraw=1;
	return 0;
}

/*! \todo *** this isn't very sophisticated, for elegance, should use some kind of 
 * bez curve fitting to cut down on unnecessary points should use a timer so 
 * stopping makes sharp corners and closer spaced points?
 */
int FreehandInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{

	if (mode == MODE_BrushSize) {
		size_dragged = true;
		double oldr = (last_screen_pos - size_center).norm();
		double newr = (flatpoint(x,y) - size_center).norm();
		brush_size += 2*(newr - oldr) / Getmag();
		if (brush_size < 0) brush_size = 0;
		last_screen_pos.set(x,y);
		needtodraw = 1;
		return 0;
	}

	last_screen_pos.set(x,y);

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int mx=0,my=0;
	buttondown.move(d->id, x,y, &mx,&my);



	// MODE_Normal
	int i=findLine(d->id);
	if (i<0) {
		RawPointLine *line=new RawPointLine;
		lines.push(line);
		deviceids.push(d->id);
		i=lines.n-1;
	}

	flatpoint p=dp->screentoreal(x,y);
	RawPointLine *line=lines.e[i];

	RawPoint *pp=new RawPoint(p);

	double xx,yy;
	const_cast<LaxMouse*>(d)->getInfo(NULL,NULL,NULL,&xx,&yy,NULL,&pp->pressure,&pp->tiltx,&pp->tilty,NULL);

	tms tms_;
	pp->time=times(&tms_);
	p=dp->screentoreal(xx,yy);
	if (pp->pressure<0 || pp->pressure>1) pp->pressure=1; //non-pressure sensitive map to full pressure
	line->push(pp);


	needtodraw=1;
	return 0;
}


/*! Returns a new RawPointLine, simplified based on lines.e[i].
 *
 * This implements the Ramer–Douglas–Peucker algorithm for reducing the number of points
 * in a polyline based on being within a given threshhold distance to a base line.
 * Start with the end points, l is the segment between them. Find the point between the endpoints
 * that is the farthest from l. If that point p is < e (the threshhold distance), then all
 * points are assumed to belong to l. Otherwise, keep p, and recursively call with new line segments
 * [start,p] and [p,end].
 */
RawPointLine *FreehandInterface::Reduce(int i, double epsilon)
{
	if (i<0 || i>=lines.n) return NULL;

	RawPointLine *l_orig=lines.e[i];
	for (int c=0; c<l_orig->n; c++) l_orig->e[c]->flag=0;
	RecurseReduce(l_orig, 0,l_orig->n-1, epsilon);

	RawPointLine *l=new RawPointLine;
	RawPoint *p;
	for (int c=0; c<l_orig->n; c++) {
		p=new RawPoint;
		*p=*l_orig->e[c];
		if (l_orig->e[c]->flag!=0) l->push(p);
	}

	return l;
}

/*! Returns a new RawPointLine with only points with non zero flags.
 * \todo *** need to artifically add points on bez line for flag==-1 points.
 */
RawPointLine *FreehandInterface::ReducePressure(int i, double epsilon)
{
	if (i<0 || i>=lines.n) return NULL;

	RawPointLine *l_orig=lines.e[i];
	for (int c=0; c<l_orig->n; c++) l_orig->e[c]->flag=0;
	RecurseReducePressure(l_orig, 0,l_orig->n-1, epsilon);

	RawPointLine *l=new RawPointLine;
	RawPoint *p;
	for (int c=0; c<l_orig->n; c++) {
		p=new RawPoint;
		*p=*l_orig->e[c];
		if (l_orig->e[c]->flag!=0) l->push(p);
	}

	return l;
}

//! Marks any points it thinks should be in the line with flag=1.
/*! If a point is added from pressure, but flag already is one (from a previous call to Reduce(),
 * then flag is made -1. 
 */
void FreehandInterface::RecurseReducePressure(RawPointLine *l, int start, int end, double epsilon)
{
	if (end<=start+1) return; 

	flatvector v=flatpoint(end-start, l->e[end]->pressure - l->e[start]->pressure);
	flatvector vt=transpose(v);
	vt.normalize();

	if (l->e[start]->flag==0) l->e[start]->flag=-1;
	if (l->e[end  ]->flag==0) l->e[end  ]->flag=-1;

	int i=-1;
	double d=0, dd;
	for (int c=start+1; c<end; c++) {
		dd=fabs(flatpoint(c-start, l->e[c]->pressure - l->e[start]->pressure)*vt);
		if (dd>d) { d=dd; i=c; }
	}

	if (d<epsilon) {
		;
		//for (int c=start+1; c<end; c++) l->e[c]->flag=0;
	} else {
		RecurseReduce(l, start,i, epsilon);
		RecurseReduce(l, i,end,   epsilon);
	}
}

/*! Marks any points it thinks should be in the line with flag=1.
 * Assumes points are all flag=0 to start. start and end always get marked 1.
 */
void FreehandInterface::RecurseReduce(RawPointLine *l, int start, int end, double epsilon)
{
	if (end<=start+1) return; 

	flatvector v=l->e[end]->p - l->e[start]->p;
	flatvector vt=transpose(v);
	vt.normalize();

	l->e[start]->flag = 1;
	l->e[end]->flag   = 1;

	//find point most distant from segment start-end
	int    i = -1;
	double d = 0, dd;
	for (int c = start + 1; c < end; c++) {
		dd = fabs((l->e[c]->p - l->e[start]->p) * vt);
		if (dd > d) {
			d = dd;
			i = c;
		}
	}

	if (d<epsilon) {
		;
		//for (int c=start+1; c<end; c++) l->e[c]->flag=0;
	} else {
		RecurseReduce(l, start,i, epsilon);
		RecurseReduce(l, i,end,   epsilon);
	}
}

/*! Return a new Coordinate list with bezier path: c-p-c-c-p-c-...-c-p-c
 */
Coordinate *FreehandInterface::BezApproximate(RawPointLine *l)
{
	// There are surely better ways to do this. Not sure how powerstroke does it.
	// It is not simplied/optimized at all. Each point gets control points to smooth it out.
	//
	// tangents at points are || to (p+1)-(p-1).
	// Lengths of control rods are 1/3 of distance to adjacent points

	Coordinate *coord=NULL;
	Coordinate *curp=NULL;

    flatvector v,p, pp,pn;
	flatvector opn, opp;
    double sx;
	
    for (int c=0; c<l->n; c++) {
        p=l->e[c]->p;

		if (c==0)      opp=p; else opp=l->e[c-1]->p;
		if (c==l->n-1) opn=p; else opn=l->e[c+1]->p;

        v=opn-opp;
        v.normalize();

        sx=norm(p-opp)*.333;
        pp=p - v*sx;

        sx=norm(opn-p)*.333;
        pn=p + v*sx;

		if (!curp) coord=curp=new Coordinate(pp,POINT_TONEXT,NULL);
		else {
			curp->next=new Coordinate(pp,POINT_TONEXT,NULL);
			curp->next->prev=curp;
			curp=curp->next;
		}

		curp->next=new Coordinate(p, POINT_SMOOTH|POINT_VERTEX, NULL);
		curp->next->prev=curp;
		curp=curp->next;

		curp->next=new Coordinate(pn,POINT_TOPREV,NULL);
		curp->next->prev=curp;
		curp=curp->next;
    }

	return coord;
}

/*! If owner!=NULL, send a RefCountedEventData with the object, and info1=i.
 */
void FreehandInterface::sendObject(LaxInterfaces::SomeData *tosend, int i)
{
	if (!tosend) return;

	if (owner) {
		RefCountedEventData *data=new RefCountedEventData(tosend);
		data->info1=i;
		app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

	} else {
		if (viewport) viewport->NewData(tosend,NULL);
	}
	tosend->dec_count();
}

int FreehandInterface::send(int i)
{
	if (i<0 || i>=lines.n) return 1;
	if (lines.e[i]->n == 0) return 1;


	if (freehand_style&FREEHAND_Raw_Path) {
		RawPointLine *line=lines.e[i];
	
		PathsData *paths=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
        if (!paths) paths=new PathsData();

		bool closed = (realtoscreen(line->e[0]->p) - realtoscreen(line->e[line->n-1]->p)).norm() < close_threshhold;

		for (int c=0; c<line->n; c++) {
			paths->append(line->e[c]->p);
		}
		if (closed) paths->fill(fillstyle ? &fillstyle->color : &default_fillstyle.color);
		paths->line(brush_size,-1,-1,linestyle ? &linestyle->color : &default_linestyle.color);
		paths->FindBBox();
		sendObject(paths,FREEHAND_Raw_Path);
	}
	

	if (freehand_style&FREEHAND_Poly_Path) {
		 //return a reduced polyline
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());

		PathsData *paths=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
        if (!paths) paths=new PathsData();

		for (int c=0; c<line->n; c++) {
			paths->append(line->e[c]->p);
		}
		bool closed = (realtoscreen(line->e[0]->p) - realtoscreen(line->e[line->n-1]->p)).norm() < close_threshhold;
		if (closed) paths->fill(fillstyle ? &fillstyle->color : &default_fillstyle.color);
		paths->line(brush_size,-1,-1,linestyle ? &linestyle->color : &default_linestyle.color);
		paths->FindBBox();
		sendObject(paths,FREEHAND_Poly_Path);
		delete line;
	}


	if (freehand_style&FREEHAND_Bez_Path) {
		 //return a bezierified line, based on a reduced polyline
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());
		Coordinate *coord=BezApproximate(line);

		PathsData *paths=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
        if (!paths) paths=new PathsData();

		paths->appendCoord(coord);

		bool closed = (realtoscreen(line->e[0]->p) - realtoscreen(line->e[line->n-1]->p)).norm() < close_threshhold;
		if (closed) paths->fill(fillstyle ? &fillstyle->color : &default_fillstyle.color);
		paths->line(brush_size,-1,-1,linestyle ? &linestyle->color : &default_linestyle.color);
		paths->FindBBox();
		sendObject(paths,FREEHAND_Bez_Path);
		delete line;
	}


	if (freehand_style&FREEHAND_Bez_Outline) {
		// return a weighted path stroke outline (not a weighted path)
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());
		//RawPointLine *line=ReducePressure(i, .1);

		NumStack<flatpoint> points;

		 //top of line
		flatvector vt, pp,pn;
		for (int c2=0; c2<line->n; c2++) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;

			if (c2==0) pp=line->e[c2]->p; else pp=line->e[c2-1]->p;
			if (c2==line->n-1) pn=line->e[c2]->p; else pn=line->e[c2+1]->p;

			vt=pn-pp;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size*line->e[c2]->pressure;
			points.push(line->e[c2]->p + vt);
		}

		 //bottom of line
		for (int c2=line->n-1; c2>=0; c2--) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;

			if (c2==0) pp=line->e[c2]->p; else pp=line->e[c2-1]->p;
			if (c2==line->n-1) pn=line->e[c2]->p; else pn=line->e[c2+1]->p;

			vt=pn-pp;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size*line->e[c2]->pressure;
			points.push(line->e[c2]->p - vt);
		}


		PathsData *paths=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
        if (!paths) paths=new PathsData();

		Coordinate *coord=LaxInterfaces::BezApproximate(points.e,points.n);

		paths->appendCoord(coord);
		paths->close();
		paths->FindBBox();
		paths->fill(fillstyle ? &fillstyle->color : &default_fillstyle.color);
		paths->line(-1,-1,-1,linestyle ? &linestyle->color : &default_linestyle.color);
		sendObject(paths,FREEHAND_Bez_Outline);

		delete line;
	}

	if (freehand_style&(FREEHAND_Bez_Weighted|FREEHAND_Path_Mesh)) {
		 //return a weighted bezierified line, which is based on a reduced polyline
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());

		 //each point in line gets 3 coords in:
		Coordinate *coord=BezApproximate(line);
		Path *path=new Path(coord);

		Coordinate *cc=coord->next;
		NumStack<flatpoint> points_top,points_bottom;
		flatvector pp,pn;
		int i=0;
		while (cc) {
			if (i==0) pp=cc->fp; else pp=cc->prev->fp;
			if (i==line->n-1) pn=cc->fp; else pn=cc->next->fp;

			path->AddWeightNode(i, 0, 2*brush_size*line->e[i]->pressure, 0);

			i++;
			cc=cc->next->next;
			if (cc) { cc=cc->next; }
		}
		delete line;


		PathsData *pdata=dynamic_cast<PathsData*>(somedatafactory()->NewObject(LAX_PATHSDATA));
        if (!pdata) pdata=new PathsData(0);

		pdata->paths.push(path);
		//pdata->fill(fillstyle ? &fillstyle->color : &default_fillstyle.color);
		pdata->line(brush_size,-1,-1,linestyle ? &linestyle->color : &default_linestyle.color);
		pdata->FindBBox();

		if (freehand_style&FREEHAND_Bez_Weighted) {
			sendObject(pdata,FREEHAND_Bez_Weighted);

			if (freehand_style&FREEHAND_Path_Mesh) {
				pdata=dynamic_cast<PathsData*>(pdata->duplicate(NULL)); //if both, we need to do this!
			}
		}

		 //return a mesh based on a weighted path
		if (freehand_style&FREEHAND_Path_Mesh) {
			PatchData *mesh=dynamic_cast<PatchData*>(somedatafactory()->NewObject(LAX_PATCHDATA));
			if (!mesh) mesh=new PatchData; 

			mesh->InstallPath(pdata);
			pdata->dec_count();

			sendObject(mesh,FREEHAND_Path_Mesh);
		}
	}


	if (freehand_style&(FREEHAND_Color_Mesh | FREEHAND_Double_Mesh)) {
		 //return a mesh based on a bezierified line, which is based on a reduced polyline
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());

		 //each point in line gets 3 coords in:
		Coordinate *coord=BezApproximate(line);

		Coordinate *cc=coord->next;
		NumStack<flatpoint> points_top,points_bottom;
		flatvector vt, pp,pn;
		int i=0;
		while (cc) {
			if (line->e[i]->pressure>=0 && line->e[i]->pressure<=1) {
				if (i==0) pp=cc->fp; else pp=cc->prev->fp;
				if (i==line->n-1) pn=cc->fp; else pn=cc->next->fp;

				vt=pn-pp;
				vt=transpose(vt);
				vt.normalize();
				vt*=brush_size*line->e[i]->pressure;
				points_top.   push(line->e[i]->p + vt);
				points_bottom.push(line->e[i]->p - vt);
			}

			i++;
			cc=cc->next->next;
			if (cc) { cc=cc->next; }
		}
		delete line;

		 //create coordinate list, should have 3*points_top.n coords in each
		Coordinate *coord_t=LaxInterfaces::BezApproximate(points_top.e,    points_top.n);
		Coordinate *coord_b=LaxInterfaces::BezApproximate(points_bottom.e, points_bottom.n);

		int nn=0;
		cc=coord_t;
		while (cc) { nn++; cc=cc->next; }

		GradientStrip *gradient = nullptr;
		if (gradients.n) gradient = dynamic_cast<GradientStrip*>(gradients.e[0]->duplicate(nullptr));
		else gradient = dynamic_cast<GradientStrip*>(default_gradient.duplicate(nullptr));
		
		if (freehand_style&(FREEHAND_Double_Mesh)) {
			int ncol = gradient->NumColors();
			for (int c=0; c<ncol-1; c++) {
				gradient->AddColor(gradient->colors.e[ncol-1]->t+(gradient->colors.e[ncol-1]->t-gradient->colors.e[ncol-2-c]->t),
								  &gradient->colors.e[ncol-2-c]->color->screen);
			}
		}

		 //create and populate mesh object
		ColorPatchData *mesh=dynamic_cast<ColorPatchData*>(somedatafactory()->NewObject(LAX_COLORPATCHDATA));
		if (!mesh) mesh=new ColorPatchData;


		//mesh->Set(0,0,1,1, 1,points_top.n-1, Patch_Coons); //create as 1 row, subdivide later
		mesh->Set(0,0,1,1, gradient->NumColors()-1,points_top.n-1, Patch_Coons); //create as 1 row, subdivide later
		Coordinate *cct=coord_t->next;
		Coordinate *ccb=coord_b->next;
		int r;
		flatpoint p,v;
		for (int c=0; c<mesh->xsize; c+=3) {
			 //top points
			r=0;
			p=mesh->points[r*mesh->xsize+c  ]=cct->fp;
			cct=cct->next; //cct at next
			if (c<mesh->xsize-2) {
				mesh->points[r*mesh->xsize+c+1]=cct->fp;
				cct=cct->next; //cct at prev
				mesh->points[r*mesh->xsize+c+2]=cct->fp;
				cct=cct->next; //cct at v
			}

			 //bottom points
			r=mesh->ysize-1;
			v=mesh->points[r*mesh->xsize+c  ]=ccb->fp;   ccb=ccb->next;
			v-=p;
			if (c<mesh->xsize-1) {
				mesh->points[r*mesh->xsize+c+1]=ccb->fp; ccb=ccb->next;
				mesh->points[r*mesh->xsize+c+2]=ccb->fp; ccb=ccb->next;
			}

			 //middle points and colors
			mesh->SetColor(0,c/3, &gradient->colors.e[0]->color->screen);
			double rr=0, lrr=0;
			for (int row=0; row<gradient->NumColors()-1; row++) {
			  lrr=rr;
			  rr=gradient->GetNormalizedT(row+1);

			  for (int col=c; col<c+3; col++) {
				  p=mesh->points[col];
				  v=mesh->points[(mesh->ysize-1)*mesh->xsize+col]-p;
				  for (r=0; r<3; r++) {
					if (row==0 && r==0) continue;
					if (row==gradient->NumColors()-1 && r==2) continue;
					if (col%3!=0 && r!=0) continue;
					if (c==mesh->xsize-1 && col>c) continue;

					mesh->points[(row*3+r)*mesh->xsize+col] = p + v*(lrr+r*(rr-lrr)/3);
					if (r==0) mesh->SetColor(row,col/3, &gradient->colors.e[row]->color->screen);
				  }
			  }
			}
			mesh->SetColor(mesh->ysize/3,c/3, &gradient->colors.e[gradient->colors.n-1]->color->screen);
		}
		mesh->InterpolateControls(Patch_Coons);
		mesh->FindBBox();

		int ii=0;
		if (freehand_style&FREEHAND_Color_Mesh) ii=FREEHAND_Color_Mesh;
		else if (freehand_style&FREEHAND_Double_Mesh) ii=FREEHAND_Double_Mesh;
		sendObject(mesh,ii);

		delete coord_t;
		delete coord_b;
		gradient->dec_count();
	}


	return 0;
}

/*! If type==0, then default to FREEHAND_Poly_Path.
 * Returns the old types.
 */
unsigned int FreehandInterface::SendType(unsigned int type)
{
	unsigned int old=(freehand_style&FREEHAND_All_Types);
	freehand_style&=~old;
	if (type==0) type=FREEHAND_Poly_Path;
	freehand_style|=type;
	return old;
}

Laxkit::ShortcutHandler *FreehandInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager = GetDefaultShortcutManager();
    sc = manager->NewHandler(whattype());
    if (sc) return sc;

    sc=new ShortcutHandler(whattype());

    sc->Add(FH_Settings, 'l',0,0, "Settings", _("Settings"), NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

int FreehandInterface::PerformAction(int action)
{
	if (action == FH_Settings) {
		if (mode == MODE_Settings) Mode(MODE_Normal);
		else Mode(MODE_Normal);

		if (mode == MODE_Settings) {
			PostMessage("Edit settings.. TODO!!!!!");
		}
		needtodraw = 1;
		return 0;
	}

	return 1;
}

/*! return 1 if mode changed. */
int FreehandInterface::Mode(EditMode newmode)
{
	if (newmode == mode) return 0;
	mode = newmode;

	if (mode == MODE_BrushSize) {
		size_dragged = false;
		size_center = last_screen_pos - flatpoint(brush_size/2 * dp->Getmag(), 0);
	}

	needtodraw = 1;
	return 1;
}

int FreehandInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if (ch == LAX_Esc && mode != MODE_Normal) {
		mode = MODE_Normal;
		needtodraw = 1;
		return 0;
	}

	if (ch == LAX_Shift && mode == MODE_Normal && !buttondown.any()) {
		Mode(MODE_BrushSize);
		needtodraw = 1;
		return 0;
	}

	if (!sc) GetShortcuts();
	int action = sc->FindActionNumber(ch, state & LAX_STATE_MASK, 0);
	if (action >= 0) {
		return PerformAction(action);
	}

	return 1;
}

int FreehandInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if (ch == LAX_Shift && mode == MODE_BrushSize && !buttondown.any()) {
		Mode(MODE_Normal);
		needtodraw = 1;
		return 0;
	}

	return 1;
}

void FreehandInterface::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *loadcontext)
{
	char *name, *value;
	for (int c=0; c<att->attributes.n; c++) {
		name  = att->attributes.e[c]->name;
		value = att->attributes.e[c]->value;

		if (!strcmp(name, "brush_size")) {
			DoubleAttribute(value, &brush_size);

		} else if (!strcmp(name, "close_threshhold")) {
			DoubleAttribute(value, &close_threshhold);

		} else if (!strcmp(name, "smooth_threshhold")) {
			DoubleAttribute(value, &smooth_pixel_threshhold);

		} else if (!strcmp(name, "ignore_tip_time")) {
			double d = 0;
			if (DoubleAttribute(value, &d)) ignore_tip_time = d;

		} else if (!strcmp(name, "control_line_color")) {
			SimpleColorAttribute(value, nullptr, &linecolor, nullptr);

		} else if (!strcmp(name, "point_color")) {
			SimpleColorAttribute(value, nullptr, &pointcolor, nullptr);

		} else if (!strcmp(name, "linestyle")) {
		} else if (!strcmp(name, "fillstyle")) {
		} else if (!strcmp(name, "shape_brush")) {
		} else if (!strcmp(name, "line_profile")) {
		} else if (!strcmp(name, "gradients")) {
			//gradient pos [resource:name]
			//   ...gradient atts if not resource...
			
		}
	}
}

Laxkit::Attribute *FreehandInterface::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *savecontext)
{
	if (!att) att = new Attribute();
	if (what == -1) {
		return att;
	}

	att->push("brush_size", brush_size);
	att->push("close_threshhold", close_threshhold);
	att->push("smooth_threshhold", smooth_pixel_threshhold);
	att->push("ignore_tip_time", ignore_tip_time);

	// att->push("control_line_color", str.c_str());
	// att->push("point_color", str.c_str());

	// Attribute *att2 = att->pushSubAtt("linestyle");
	// if (linestyle) att2->push("resource", linestyle->Id());
	// else linestyle->dump_out_atts(att2, what, savecontext);

	// Attribute *att2 = att->pushSubAtt("fillstyle");
	// if (fillstyle) att2->push("resource", fillstyle->Id());
	// else fillstyle->dump_out_atts(att2, what, savecontext);

	// att->push("shape_brush", );
	// att->push("line_profile", );
	// att->push("gradients", );

	return att;
}


} // namespace LaxInterfaces

