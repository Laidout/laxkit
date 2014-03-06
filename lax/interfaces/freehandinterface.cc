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
//    Copyright (C) 2004-2006,2011,2014 by Tom Lechner
//



#include <lax/interfaces/freehandinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/interfaces/colorpatchinterface.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {


//----------------------------------------------------------------

/*! \class FreehandInterface
 * \ingroup interfaces
 * \brief Interface to create bezier or straight lines from freehand motion.
 *
 *   On mouse down, this records all mouse movement, and converts
 *   the points into a bez curve (linear or cubic) on mouse up.
 *
 * \todo ***Currently, this sucks. Should figure out how Sodipodi/inkscape does it.
 *  
 * \todo *** make closed when final point is close to first point
 */


FreehandInterface::FreehandInterface(anInterface *nowner, int nid, Displayer *ndp)
 : anInterface(nowner,nid,ndp)
{
	//freehand_style=FREEHAND_Poly_Path;
	//freehand_style=FREEHAND_Bez_Path;
	//freehand_style=FREEHAND_Bez_Outline;
	//freehand_style=FREEHAND_Color_Mesh;
	freehand_style=FREEHAND_Double_Mesh;

	linecolor .rgbf(0,0,.5);
	pointcolor.rgbf(.5,.5,.5);

	linestyle.color.red  =linestyle.color.alpha=0xffff;
	linestyle.color.green=linestyle.color.blue =0;

	smooth_pixel_threshhold=2;
	brush_size=60;

	showdecs=1;
	needtodraw=1;

	sc=NULL;
}

FreehandInterface::~FreehandInterface()
{
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

int FreehandInterface::UseThis(anObject *nobj, unsigned int mask)
{
	if (!nobj) return 1;
	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
	if (ls!=NULL) {
		if (mask&GCForeground) { 
			linestyle.color=ls->color;
		}
		if (mask&GCLineWidth) {
			linestyle.width=ls->width;
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

Laxkit::MenuInfo *FreehandInterface::ContextMenu(int x,int y,int deviceid)
{
	MenuInfo *menu=new MenuInfo;
	//menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path, (freehand_style&FREEHAND_Raw_Path)?LAX_CHECKED:0);
	//menu->AddItem(_("Create simplified polyline"), FREEHAND_Poly_Path, (freehand_style&FREEHAND_Poly_Path)?LAX_CHECKED:0);
	//menu->AddItem(_("Create bezier line"), FREEHAND_Bez_Path, (freehand_style&FREEHAND_Bez_Path)?LAX_CHECKED:0);
	//menu->AddItem(_("Create bezier outline"), FREEHAND_Bez_Outline, (freehand_style&FREEHAND_Bez_Outline)?LAX_CHECKED:0);
	//menu->AddItem(_("Create mesh"), FREEHAND_Color_Mesh, (freehand_style&FREEHAND_Color_Mesh)?LAX_CHECKED:0);
	//menu->AddItem(_("Create symmetric mesh"), FREEHAND_Double_Mesh, (freehand_style&FREEHAND_Color_Mesh)?LAX_CHECKED:0);

	menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path );
	menu->AddItem(_("Create simplified polyline"), FREEHAND_Poly_Path );
	menu->AddItem(_("Create bezier line"), FREEHAND_Bez_Path );
	menu->AddItem(_("Create bezier outline"), FREEHAND_Bez_Outline);
	menu->AddItem(_("Create mesh"), FREEHAND_Color_Mesh);
	menu->AddItem(_("Create symmetric mesh"), FREEHAND_Double_Mesh);
	return menu;
}


int FreehandInterface::Event(const Laxkit::EventData *e,const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
        int i =s->info2; //id of menu item
        //int ii=s->info4; //extra id, 1 for direction

		if (i==FREEHAND_Raw_Path    ) {
			freehand_style=FREEHAND_Raw_Path;

		} else if (i==FREEHAND_Poly_Path   ) {
			freehand_style=FREEHAND_Poly_Path;

		} else if (i==FREEHAND_Bez_Path    ) {
			freehand_style=FREEHAND_Bez_Path;

		} else if (i==FREEHAND_Bez_Outline ) {
			freehand_style=FREEHAND_Bez_Outline;

		} else if (i==FREEHAND_Color_Mesh        ) {
			freehand_style=FREEHAND_Color_Mesh;

		} else if (i==FREEHAND_Double_Mesh        ) {
			freehand_style=FREEHAND_Double_Mesh;
		}
	}
	
	return 1;
}

int FreehandInterface::Refresh()
{ 
	//DBG cerr <<"  Freehand trying to startdrawing"<<endl;

	if (needtodraw==0) return 0;
	needtodraw=0;

	if (lines.n==0) return 0;
	
	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

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

		if (freehand_style&(FREEHAND_Bez_Outline|FREEHAND_Color_Mesh|FREEHAND_Double_Mesh)) {
			 //draw pressure indicator
			dp->NewFG(1.,0.,1.,1.);
			flatvector vt;
			dp->moveto(line->e[0]->p);
			for (int c2=1; c2<line->n-1; c2++) {
				if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;
				vt=line->e[c2+1]->p - line->e[c2-1]->p;
				vt=transpose(vt);
				vt.normalize();
				vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
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
				vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
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

	int i=findLine(d->id);
	if (i>=0) {
		lines.remove(i);
		deviceids.remove(i);
	}

	DBG cerr <<"../freehandiLbd\n";
	return 0;
}

//! Finish a new freehand line by calling newData with it.
int FreehandInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	DBG cerr <<"  freehandLbup..";

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int dragged=buttondown.up(d->id,LEFTBUTTON);

	int i=findLine(d->id);
	if (i<0) return 0; //line missing! do nothing

	DBG cerr <<"  *** FreehandInterface should check for closed path???"<<endl;

	if (dragged && lines.e[i]->n>1) {
		send(i);
	}

	deviceids.remove(i);
	lines.remove(i);

	needtodraw=1;
	return 0;
}

/*! \todo *** this isn't very sophisticated, for elegance, should use some kind of 
 * bez curve fitting to cut down on unnecessary points should use a timer so 
 * stopping makes sharp corners and closer spaced points?
 */
int FreehandInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;

	int i=findLine(d->id);
	if (i<0) {
		RawPointLine *line=new RawPointLine;
		lines.push(line);
		deviceids.push(d->id);
		i=lines.n-1;
	}
	

	int mx=0,my=0;
	buttondown.move(d->id, x,y, &mx,&my);

	flatpoint p=dp->screentoreal(x,y);
	RawPointLine *line=lines.e[i];

	RawPoint *pp=new RawPoint(p);

	double xx,yy;
	const_cast<LaxMouse*>(d)->getInfo(NULL,NULL,NULL,&xx,&yy,NULL,&pp->pressure,&pp->tiltx,&pp->tilty);

	pp->time=times(NULL);
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

//! Marks any points it thinks should be in the line with flag=1.
void FreehandInterface::RecurseReduce(RawPointLine *l, int start, int end, double epsilon)
{
	if (end<=start+1) return; 

	flatvector v=l->e[end]->p - l->e[start]->p;
	flatvector vt=transpose(v);
	vt.normalize();

	l->e[start]->flag=1;
	l->e[end  ]->flag=1;

	int i=-1;
	double d=0, dd;
	for (int c=start+1; c<end; c++) {
		dd=fabs((l->e[c]->p - l->e[start]->p)*vt);
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

		curp->next=new Coordinate(p);
		curp->next->prev=curp;
		curp=curp->next;

		curp->next=new Coordinate(pn,POINT_TOPREV,NULL);
		curp->next->prev=curp;
		curp=curp->next;
    }

	return coord;
}

int FreehandInterface::send(int i)
{
	if (i<0 || i>=lines.n) return 1;

	if (freehand_style&FREEHAND_Raw_Path) {
		RawPointLine *line=lines.e[i];
	
		PathsData *paths=new PathsData;
		for (int c=0; c<line->n; c++) {
			paths->append(line->e[c]->p);
		}
		paths->FindBBox();

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(paths);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(paths,NULL);
		}

		paths->dec_count();
	}
	

	if (freehand_style&FREEHAND_Poly_Path) {
		 //return a reduced polyline
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());

		PathsData *paths=new PathsData;
		for (int c=0; c<line->n; c++) {
			paths->append(line->e[c]->p);
		}
		paths->FindBBox();

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(paths);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(paths,NULL);
		}

		paths->dec_count();
		delete line;
	}


	if (freehand_style&FREEHAND_Bez_Path) {
		 //return a bezierified line, based on a reduced polyline
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());
		Coordinate *coord=BezApproximate(line);

		PathsData *paths=new PathsData;
		paths->appendCoord(coord);
		paths->FindBBox();

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(paths);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(paths,NULL);
		}

		paths->dec_count();
		delete line;
	}


	if (freehand_style&FREEHAND_Bez_Outline) {
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());
		//RawPointLine *line=ReducePressure(i, .1);

		NumStack<flatpoint> points;

		flatvector vt, pp,pn;
		for (int c2=0; c2<line->n; c2++) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;

			if (c2==0) pp=line->e[c2]->p; else pp=line->e[c2-1]->p;
			if (c2==line->n-1) pn=line->e[c2]->p; else pn=line->e[c2+1]->p;

			vt=pn-pp;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
			points.push(line->e[c2]->p + vt);
		}

		for (int c2=line->n-1; c2>=0; c2--) {
			if (line->e[c2]->pressure<0 || line->e[c2]->pressure>1) continue;

			if (c2==0) pp=line->e[c2]->p; else pp=line->e[c2-1]->p;
			if (c2==line->n-1) pn=line->e[c2]->p; else pn=line->e[c2+1]->p;

			vt=pn-pp;
			vt=transpose(vt);
			vt.normalize();
			vt*=brush_size/dp->Getmag()*line->e[c2]->pressure;
			points.push(line->e[c2]->p - vt);
		}


		Coordinate *coord=LaxInterfaces::BezApproximate(points.e,points.n);

		PathsData *paths=new PathsData;
		paths->appendCoord(coord);
		paths->close();
		paths->FindBBox();
		paths->fill(&linestyle.color);

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(paths);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(paths,NULL);
		}

		paths->dec_count();
		delete line;
	}

	if (freehand_style&(FREEHAND_Color_Mesh | FREEHAND_Double_Mesh)) {
		 //return a mesh based on a bezierified line, which is based on a reduced polyline
		RawPointLine *line=Reduce(i, smooth_pixel_threshhold/dp->Getmag());
		//RawPointLine *line=lines.e[i];

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
				vt*=brush_size/dp->Getmag()*line->e[i]->pressure;
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

		 // *** set up test gradient:
		ScreenColor col1(1.,0.,0.,1.), col2(0.,0.,1.,1.);
		GradientData gradient(flatpoint(0,0),flatpoint(1,0),0,1,&col1,&col2,GRADIENT_LINEAR);
		gradient.AddColor(.5, 0,65535,0,65535);

		if (freehand_style&(FREEHAND_Double_Mesh)) {
			int ncol=gradient.colors.n;
			for (int c=0; c<ncol-1; c++) {
				gradient.AddColor(gradient.colors.e[ncol-1]->t+(gradient.colors.e[ncol-1]->t-gradient.colors.e[ncol-2-c]->t),
								  &gradient.colors.e[ncol-2-c]->color);
			}
		}

		 //create and populate mesh object
		ColorPatchData *mesh=new ColorPatchData;
		//mesh->Set(0,0,1,1, 1,points_top.n-1, Patch_Coons); //create as 1 row, subdivide later
		mesh->Set(0,0,1,1, gradient.colors.n-1,points_top.n-1, Patch_Coons); //create as 1 row, subdivide later
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
			mesh->SetColor(0,c/3, &gradient.colors.e[0]->color);
			double rr=0, lrr=0;
			for (int row=0; row<gradient.colors.n-1; row++) {
			  lrr=rr;
			  rr=gradient.GetNormalizedT(row+1);

			  for (int col=c; col<c+3; col++) {
				  p=mesh->points[col];
				  v=mesh->points[(mesh->ysize-1)*mesh->xsize+col]-p;
				  for (r=0; r<3; r++) {
					if (row==0 && r==0) continue;
					if (row==gradient.colors.n-1 && r==2) continue;
					if (col%3!=0 && r!=0) continue;
					if (c==mesh->xsize-1 && col>c) continue;

					mesh->points[(row*3+r)*mesh->xsize+col] = p + v*(lrr+r*(rr-lrr)/3);
					if (r==0) mesh->SetColor(row,col/3, &gradient.colors.e[row]->color);
				  }
			  }
			}
			mesh->SetColor(mesh->ysize/3,c/3, &gradient.colors.e[gradient.colors.n-1]->color);
		}
		mesh->InterpolateControls(Patch_Coons);
		mesh->FindBBox();

		if (owner) {
			RefCountedEventData *data=new RefCountedEventData(mesh);
			app->SendMessage(data,owner->object_id,"FreehandInterface", object_id);

		} else {
			if (viewport) viewport->NewData(mesh,NULL);
		}

		mesh->dec_count();
		delete coord_t;
		delete coord_b;
	}


	return 0;
}

Laxkit::ShortcutHandler *FreehandInterface::GetShortcuts()
{
	return NULL;
}

int FreehandInterface::PerformAction(int action)
{
	return 1;
}

} // namespace LaxInterfaces

