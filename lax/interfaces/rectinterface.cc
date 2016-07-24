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
//    Copyright (C) 2004-2007,2010-2011 by Tom Lechner
//


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/rectinterface.h>
#include <lax/interfaces/rectpointdefs.h>
#include <lax/transformmath.h>
#include <lax/laxutils.h>
#include <lax/language.h>

using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {
	
#define RP_Faux_Center1     1000
#define RP_Faux_Center2     1001
#define RP_Faux_Shearpoint  1002




//----------------------------- RectData --------------------------------
/*! \class RectData
 * \ingroup interfaces
 * \brief Basic rectangular selection, data type for RectInterface.
 *
 * Useful for selecting things, these rectangles can be rotated, scaled, and sheared.
 *
 * Perhaps this class could be made into a SomeDataRef subclass?
 * that would make implementing the ObjectInterface slightly easier.
 * 
 * The affine style allows shearing as well as resizing.
 * \code
 *  #define RECT_ISSQUARE        (1<<0)
 *  #define RECT_OFF             (1<<1)
 *  #define RECT_DOTTED          (1<<2)
 *  #define RECT_SOLID           (1<<3)
 *  #define RECT_INVISIBLECENTER (1<<4)
 *  #define RECT_CANTCREATE      (1<<5)
 *  #define RECT_ALLOW_SHEAR     (1<<6)
 * \endcode
 */

RectData::RectData()
{
	centertype=LAX_MIDDLE;
	setbounds(0,1, 0,1);
	style=0; 
	linestyle=NULL;
	griddivisions=10;
	DBG cerr <<"RectData "<<object_id<<" created"<<endl;
} 

RectData::RectData(flatpoint pp,double ww,double hh,int ct,unsigned int stle)
{
	centertype=ct;
	center=pp;

	origin(pp);
	maxx=ww;
	maxy=hh;
	griddivisions=10;
	
	linestyle=NULL;
	style=stle;
	centercenter(); 

	DBG cerr <<"RectData "<<object_id<<" created"<<endl;
}

/*! If linestyle, delete linestyle.
 *
 * \todo decide whether to even have a linestyle in here.
 */
RectData::~RectData()
{
	if (linestyle) delete linestyle;
}

//! Make center be at position defined by centertype.
void RectData::centercenter() 
{	
	if (centertype!=LAX_CUSTOM_ALIGNMENT)
		center=ReferencePoint(centertype,false);
}


//----------------------------- RectInterface --------------------------------


/*! \class RectInterface
 * \ingroup interfaces
 * \brief Handles RectData or works directly on the matrix of any SomeData.
 * 
 * This is a very souped up version of select by rectangle, you can rotate, 
 * scale, adjust by edges, etc. Included is a 3-point transform, wherein you
 * can select an arbitrary center of scaling or rotation, and click and drag
 * so the object is scaled and rotated to keep up with the dragging. Also, you
 * can define another anchor point, and drag a third point which shears the
 * object, with the first two anchor points staying in the same spot.
 * 
 * The idea here is for this class to be the base class of a potential selection
 * rectangle interface, and also for an object transform modification interface.
 * For instance, EllipseInterface is built using a RectInterface to trace out
 * a rectangle, inside of which the ellipse is placed.
 * 
 * Also, ObjectInterface is subclassed to use a dotted rectangle for selecting objects
 * in a ViewportWindow. 
 *
 * This interface will modify only the matrix of its data and not the bounds,
 * unless it is a RectData, in which case the attributes of that are fair game.
 * 
 * \todo *** update docs here!
 * \todo *** differentiate move preserve aspect, and move with aspect==1 (as square)
 *   ***draw only whats necessary
 *   ***general debug, ISSQUARE
 *   *** a style to allow movement only, not resize, except on cntl???
 *   VISIBLECENTER still allows a center for rotations
 *   if center is on a control point, be able to sequentially select point(s) beneath
 * \todo *** should probably have a style where you just section off a plain, non
 *   rotated rectangle with no control points indicated. like for cropping
 * \todo *** need mode to distinguish between modifying the min/max bounds, and modifying
 *   the matrix. When the matrix is adjusted so that w==0 or h==0, there are special
 *   problems to check for!! causes matrix to be degenerate...
 * 
 * Note that somedata (a SomeData) holds the basic object, and if the object happens to be a RectData,
 * then data is the RectData cast of somedata. When new data is created internally, the viewport
 * is not notified to insert this new data, unlike in most of the other interfaces. This 
 * makes it easier for other classes to use this class to trace out areas.
 */
/*! \var char RectInterface::dragmode
 * 
 * 0 means normal.
 * dragmode&1 means do not show the little arrow handles while dragging.
 *
 * This variable can be used by subclasses to signal that dragging is to be used
 * for creating or adding to a selection, for instance.
 */
/*! \var int RectInterface::maxtouchlen
 * \brief The maximum screen length in pixels that a transform handle is allowed to have.
 */



RectInterface::RectInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	controlcolor=rgbcolor(0,148,178);
	somedata=data=NULL;
	showdecs=SHOW_OUTER_HANDLES|SHOW_INNER_HANDLES;
	mousetarget=0; //whether to show a circle-target underneath mice
	//style=0;
	style=RECT_FLIP_AT_SIDES;
	creationstyle=RECT_ALLOW_SHEAR;
	createfrompoint=0; // 0=lbd=ulc, 1=lbd=center
	createx=flatpoint(1,0);
	createy=flatpoint(0,1);
	griddivisions=10;

	rotatestep=M_PI/12;
	hover=RP_None;

	dragmode=0; //should be normal operations. ignored by RectInterface, but can be used by subclasses
	shiftmode=0; //0 for normal, 1 for was doing single mouse cntl zoom/rotate

	maxtouchlen=15;
	extrapoints=0;

	sc=NULL;
	
	needtodraw=1;
}

RectInterface::~RectInterface() 
{
	deletedata();
	if (sc) sc->dec_count();
	DBG cerr <<"---- in RectInterface destructor"<<endl;
}
		
const char *RectInterface::Name()
{ return _("Affine Tool"); }

anInterface *RectInterface::duplicate(anInterface *dup)//dup=NULL
{
	RectInterface *r;
	if (dup==NULL) r=new RectInterface(id,dp);
	else {r=dynamic_cast<RectInterface *>(dup);
		if (r==NULL) return NULL;
	}
	r->creationstyle=creationstyle;
	r->createx=createx;
	r->createy=createy;
	return anInterface::duplicate(r);
}

int RectInterface::InterfaceOn()
{
	showdecs=SHOW_OUTER_HANDLES|SHOW_INNER_HANDLES;
	needtodraw=1;
	return 0;
}

int RectInterface::InterfaceOff()
{
	if (style&RECT_FLIP_LINE) style=(style&~RECT_FLIP_LINE)|RECT_FLIP_AT_SIDES;
	showdecs=0;
	needtodraw=1;
	return 0;
}

void RectInterface::Clear(SomeData *d)
{
	if (d && d!=somedata) return;
	deletedata();
}

/*! Dec count data, setting data and somedata to NULL.
 */
void RectInterface::deletedata()
{
	if (somedata) somedata->dec_count();
	extrapoints=0;
	somedata=data=NULL;
}

/*! This accepts RectData OR just any SomeData. If it is a SomeData but
 * not a rectdata, then data will be NULL, but somedata will not be NULL.
 *
 * \todo *** figure out how this jives with viewport window, especially when this is
 * working on the same data that another interface is working on.. often this will
 * be a child of another interface...
 */
int RectInterface::UseThis(anObject *newdata,unsigned int) // assumes not use local
{
	if (!newdata) return 0;
	if (dynamic_cast<RectData *>(newdata) || dynamic_cast<SomeData *>(newdata)) {
		if (data) deletedata();
		somedata=dynamic_cast<SomeData *>(newdata);
		somedata->inc_count();
		data=dynamic_cast<RectData *>(newdata);
		extrapoints=0;
		syncFromData(1);
		needtodraw=1;
		return 1;
	}
	return 0;
}

//! Set up xaxislen, yaxislen, xdir, and ydir from somedata.
/*! If first==1 then the data is newly installed. Otherwise, this
 * is called from an LBDown, and compensates for any changes made
 * from outside the interface (which are assumed to not have made
 * the transform invalid *** maybe do that check anyway?).
 *
 * These variables are used to help maintain orientations when using the
 * transform handles. For instance, when scaling along an egde, you can flip
 * the whole data around, making the somedata potentially pass through a zero width state,
 * which would destroy all orientation data.
 */
void RectInterface::syncFromData(int first)
{
	if (!somedata) return;

	xdir=somedata->xaxis();
	ydir=somedata->yaxis();
	origin=somedata->origin();
	xaxislen=norm(xdir);
	if (xaxislen) xdir/=xaxislen;
	yaxislen=norm(ydir);
	if (yaxislen) ydir/=yaxislen;
	
	// zero check done only when first initializing from data, not on just any lbdown
	if (first) {
		if (fabs(yaxislen)<1e-10 && fabs(xaxislen)<1e-10) { 
			xdir=flatpoint(1,0);
			ydir=flatpoint(0,1);
		} else if (xaxislen && fabs(yaxislen)<1e-10) {
			xdir=-transpose(ydir);
		} else if (fabs(xaxislen)<1e-10 && yaxislen) {
			ydir=transpose(xdir);
		}
	}
}

//! Set the somedata's axes and origin from xdir, etc.
void RectInterface::syncToData()
{
	if (!somedata) return;
	somedata->origin(origin);
	somedata->xaxis((fabs(xaxislen)>1e-10?xaxislen:1e-10)*xdir);
	somedata->yaxis((fabs(yaxislen)>1e-10?yaxislen:1e-10)*ydir);
}

int RectInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int)
{
    if (!ndata || dynamic_cast<RectData *>(ndata)==NULL) return 1;
    RectData *bzd=data;
	SomeData *sdt=somedata;
    somedata=data=dynamic_cast<RectData *>(ndata);
    int td=showdecs,ntd=needtodraw;
    showdecs=0;
    needtodraw=1;
    Refresh();
    needtodraw=ntd;
    showdecs=td;
    data=bzd;
	somedata=sdt;
    return 1;
}

/*! Return a rectangle and transform rotated, translated, and sheared in screen coordinates.
 * The x and y axes are of unit length, thus it visually overlaps the actual data, but
 * does NOT correspond to the data's actual transform.
 *
 * The rectangle is oriented such that the first and second points are the "top most" edge.
 */
void RectInterface::GetOuterRect(DoubleBBox *box, double *mm)
{
	flatpoint ll(somedata->minx,somedata->miny);
	flatpoint lr(somedata->maxx,somedata->miny);
	flatpoint ur(somedata->maxx,somedata->maxy);
	flatpoint ul(somedata->minx,somedata->maxy);

	ll=dp->realtoscreen(ll);
	lr=dp->realtoscreen(lr);
	ur=dp->realtoscreen(ur);
	ul=dp->realtoscreen(ul);

	 //for now, just return 
	box->addtobounds(ll);
	box->addtobounds(lr);
	box->addtobounds(ur);
	box->addtobounds(ul);
}

/*! Default refresh is to draw a dashed line around the bounding box.
 * If showdecs, then also draw the common stretchy handles.
 */
int RectInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!somedata) {
		needtodraw=0;
		return 1;
	}

	DBG cerr <<"  RectRefresh-";
	//DBG dp->drawaxes(10);
	dp->NewFG(controlcolor);
	dp->PushAndNewTransform(somedata->m());
		
	flatpoint ll(somedata->minx,somedata->miny);
	flatpoint lr(somedata->maxx,somedata->miny);
	flatpoint ur(somedata->maxx,somedata->maxy);
	flatpoint ul(somedata->minx,somedata->maxy);

	ll=dp->realtoscreen(ll);
	lr=dp->realtoscreen(lr);
	ur=dp->realtoscreen(ur);
	ul=dp->realtoscreen(ul);

	//DBG dp->LineAttributes(1,LineSolid,LAXCAP_Butt,LAXJOIN_Round);
	//DBG dp->drawline(0,0, 100,100);
	//DBG dp->drawline(0,0, 100,100);
	//DBG dp->drawline(0,0, 100,100);


	 // draw dotted box
	//if (!(data->style&RECT_OFF)) {
		dp->LineAttributes(1,LineDoubleDash,LAXCAP_Butt,LAXJOIN_Miter);
		dp->DrawScreen();
		flatpoint pn[4];
		dp->drawline(ll,lr);
		dp->drawline(lr,ur);
		dp->drawline(ur,ul);
		dp->drawline(ul,ll);
		pn[0]=ll;
		pn[1]=lr;
		pn[2]=ur;
		pn[3]=ul;
		//dp->drawlines(pn,4,1,0);
		dp->LineAttributes(1,LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
		dp->DrawReal();
	//}

	 // draw gridlines
	//dp->NewFG(data->linestyle.color);
//	if (data && data->griddivisions>0 && data->griddivisions<50) {
//		dp->NewFG(controlcolor);
//		double x,y,w,h;
//		w=somedata->maxx-somedata->minx;
//		h=somedata->maxy-somedata->miny;
//		DBG cerr <<" data->griddivisions="<<data->griddivisions<<" w/gd:"<<w/data->griddivisions<<endl;
//		if (w>=0) 
//			for (x=0; x<=w; x+=w/data->griddivisions) {
//				dp->drawrline(flatpoint(somedata->minx+x,somedata->miny),flatpoint(somedata->minx+x,somedata->miny+h));
//				DBG cerr <<"x: "<<x<<endl;
//			}
//		if (h>=0)
//			for (y=0; y<=h; y+=h/data->griddivisions) {
//				dp->drawrline(flatpoint(somedata->minx,somedata->miny+y),flatpoint(somedata->minx+x,somedata->miny+y));
//				DBG cerr <<"y: "<<y<<endl;
//			}
//	}

	 // draw control points;
	if (showdecs && !(style&RECT_HIDE_CONTROLS)) { 

		dp->NewFG(controlcolor);
		dp->DrawScreen();
		flatpoint p;
		//DBG cerr <<"3";
//		------------------------
		
		if ((showdecs&SHOW_OUTER_HANDLES) && (extrapoints==0 || extrapoints==HAS_CENTER1)) {
			int xtouchlen, ytouchlen;
			xtouchlen=norm(ll-lr)/4;
			ytouchlen=norm(ul-ll)/4;
			if (xtouchlen>maxtouchlen) xtouchlen=maxtouchlen;
			if (ytouchlen>maxtouchlen) ytouchlen=maxtouchlen;

			flatpoint vx=lr-ll;
			flatpoint vy=ul-ll;
			vx/=norm(vx);
			vy/=norm(vy);

			 //outer horizontals
			dp->drawline(ll+xtouchlen*vx-ytouchlen/2*vy, lr-xtouchlen*vx-ytouchlen/2*vy);
			dp->drawline(ul+xtouchlen*vx+ytouchlen/2*vy, ur-xtouchlen*vx+ytouchlen/2*vy);

			 //outer verticals
			dp->drawline(ll-xtouchlen/2*vx+ytouchlen*vy, ul-xtouchlen/2*vx-ytouchlen*vy);
			dp->drawline(lr+xtouchlen/2*vx+ytouchlen*vy, ur+xtouchlen/2*vx-ytouchlen*vy);

			if (showdecs&SHOW_INNER_HANDLES) {
				 //inner horizontals
				dp->drawline(ll+ytouchlen*vy, lr+ytouchlen*vy);
				dp->drawline(ul-ytouchlen*vy, ur-ytouchlen*vy);

				 //inner verticals
				dp->drawline(ll+xtouchlen*vx, ul+xtouchlen*vx);
				dp->drawline(lr-xtouchlen*vx, ur-xtouchlen*vx);
			}

			 //rotation handles
			flatpoint b[9];
			double ff=1/3.;
			flatpoint v1= xtouchlen*ff*vx+ytouchlen*ff*vy;
			flatpoint v2=-xtouchlen*ff*vx+ytouchlen*ff*vy;

			b[1]=ll + xtouchlen  *vx - ytouchlen/2*vy;
			b[4]=ll - xtouchlen/2*vx - ytouchlen/2*vy;
			b[7]=ll - xtouchlen/2*vx + ytouchlen*vy;
			b[2]=b[1]-v1;
			b[3]=b[4]-v2;
			b[5]=b[4]+v2;
			b[6]=b[7]-v1;
			dp->drawbez(b,3, 0,0);

			b[1]=lr + xtouchlen/2*vx + ytouchlen  *vy;
			b[4]=lr + xtouchlen/2*vx - ytouchlen/2*vy;
			b[7]=lr - xtouchlen  *vx - ytouchlen/2*vy;
			b[2]=b[1]-v2;
			b[3]=b[4]+v1;
			b[5]=b[4]-v1;
			b[6]=b[7]-v2;
			dp->drawbez(b,3, 0,0);

			b[1]=ur - xtouchlen  *vx + ytouchlen/2*vy;
			b[4]=ur + xtouchlen/2*vx + ytouchlen/2*vy;
			b[7]=ur + xtouchlen/2*vx - ytouchlen*vy  ;
			b[2]=b[1]+v1;
			b[3]=b[4]+v2;
			b[5]=b[4]-v2;
			b[6]=b[7]+v1;
			dp->drawbez(b,3, 0,0);

			b[1]=ul - xtouchlen/2*vx - ytouchlen  *vy;
			b[4]=ul - xtouchlen/2*vx + ytouchlen/2*vy;
			b[7]=ul + xtouchlen  *vx + ytouchlen/2*vy;
			b[2]=b[1]+v2;
			b[3]=b[4]-v1;
			b[5]=b[4]+v1;
			b[6]=b[7]+v2;
			dp->drawbez(b,3, 0,0);
		} //show scale/rotate/shear handles

		if ((extrapoints&HAS_CENTER1) || hover==RP_Faux_Center1) {
			if (hover==RP_Faux_Center1) p=hoverpoint;
			else p=dp->realtoscreen(getpoint(RP_Center1,0));
			dp->drawthing((int)p.x,(int)p.y,5,5,0,THING_Circle_Plus);
		}
		if ((extrapoints&HAS_CENTER2) || hover==RP_Faux_Center2) {
			if (hover==RP_Faux_Center2) p=hoverpoint;
			else p=dp->realtoscreen(getpoint(RP_Center2,0));
			flatpoint v=dp->realtoscreen(getpoint(RP_Center1,0))-p;
			dp->drawthing((int)p.x,(int)p.y,3,3,0,THING_Circle);
			dp->drawarrow(p,v,0,10,0);
		}
		if ((extrapoints&HAS_SHEARPOINT) || hover==RP_Faux_Shearpoint) {
			if (hover==RP_Faux_Shearpoint) p=hoverpoint;
			else p=dp->realtoscreen(getpoint(RP_Shearpoint,0));
			flatpoint v=dp->realtoscreen(getpoint(RP_Center1,0))-dp->realtoscreen(getpoint(RP_Center2,0));
			dp->drawthing((int)p.x,(int)p.y,3,3,0,THING_Circle);
			dp->drawarrow(p,v,0,10,0);
			dp->drawarrow(p,-v,0,10,0);
		} 

		if (hover!=RP_None) {
			 //scale and shear show double arrows pointing in various directions
			 //rotate shows rotate arrows
			flatpoint m,mid=getpoint(RP_Middle,0);
			flatpoint p,dir;
			int type=0;
			if (hover==RP_Scale_N) { type=1; p=getpoint(RP_N,0); m=mid; }
			else if (hover==RP_Scale_NE) { type=1; p=getpoint(RP_NE,0); m=mid; }
			else if (hover==RP_Scale_E) { type=1; p=getpoint(RP_E,0); m=mid; }
			else if (hover==RP_Scale_SE) { type=1; p=getpoint(RP_SE,0); m=mid; }
			else if (hover==RP_Scale_S ) { type=1; p=getpoint(RP_S ,0); m=mid; }
			else if (hover==RP_Scale_SW) { type=1; p=getpoint(RP_SW,0); m=mid; }
			else if (hover==RP_Scale_W ) { type=1; p=getpoint(RP_W ,0); m=mid; }
			else if (hover==RP_Scale_NW) { type=1; p=getpoint(RP_NW,0); m=mid; }

			else if (hover==RP_Shear_N) { type=1; p=getpoint(RP_N,0); m=getpoint(RP_NE,0); }
			else if (hover==RP_Shear_S) { type=1; p=getpoint(RP_S,0); m=getpoint(RP_SE,0); }
			else if (hover==RP_Shear_E) { type=1; p=getpoint(RP_E,0); m=getpoint(RP_NE,0); }
			else if (hover==RP_Shear_W) { type=1; p=getpoint(RP_W,0); m=getpoint(RP_NW,0); }

			else if (hover==RP_Rotate_NW) { type=2; p=getpoint(RP_NW,0); m=mid; }
			else if (hover==RP_Rotate_NE) { type=2; p=getpoint(RP_NE,0); m=mid; }
			else if (hover==RP_Rotate_SE) { type=2; p=getpoint(RP_SE,0); m=mid; }
			else if (hover==RP_Rotate_SW) { type=2; p=getpoint(RP_SW,0); m=mid; }

			if (type!=0) {
				p=dp->realtoscreen(p);
				m=dp->realtoscreen(m);
			}
			int arrowlen=10;
			if (type==1) {
				 //draw double arrows
				dp->drawarrow(p,p-m,0,arrowlen,0);
				dp->drawarrow(p,m-p,0,arrowlen,0);
			} else if (type==2) {
				 // ***  draw arc !!! not double arrows
				m=transpose(p-m);
				dp->drawarrow(p,m,0,arrowlen,0);
				dp->drawarrow(p,-m,0,arrowlen,0);
			}
		}

		if ((style&RECT_FLIP_AT_SIDES) && (showdecs&SHOW_OUTER_HANDLES)) {
			int fill;
			//flatpoint pts[10];
			//int n=0;
			//double h=15;
			//DoubleBBox box(-h/3,h/3, -h,h);
			//draw_thing_coordinates(THING_Double_Arrow_Vertical, pts,10, n, 1,&box);

			double xmag=norm(dp->realtoscreen(flatpoint(1,0))
							-dp->realtoscreen(flatpoint(0,0)));
			double ymag=norm(dp->realtoscreen(flatpoint(0,1))
							-dp->realtoscreen(flatpoint(0,0)));

			flatpoint p=dp->realtoscreen(somedata->maxx+maxtouchlen/xmag, (somedata->miny+somedata->maxy)/2);
			flatpoint p2=dp->realtoscreen(somedata->maxx+maxtouchlen/xmag, 1+(somedata->miny+somedata->maxy)/2);
			flatpoint v=p2-p; v*=7/norm(v);

			if (hover==RP_Flip_V) fill=1; else fill=0;
			dp->LineAttributes(fill?3:1, LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
			dp->drawarrow(p,v,0,1,2);
			dp->drawarrow(p,-v,0,1,2);

			p=dp->realtoscreen((somedata->minx+somedata->maxx)/2, somedata->maxy+maxtouchlen/ymag);
			p2=dp->realtoscreen(1+(somedata->minx+somedata->maxx)/2, somedata->maxy+maxtouchlen/ymag);
			v=p2-p; v*=7/norm(v);

			if (hover==RP_Flip_H) fill=1; else fill=0;
			dp->LineAttributes(fill?3:1, LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
			dp->drawarrow(p,v,0,1,2);
			dp->drawarrow(p,-v,0,1,2);

			dp->LineAttributes(1, LineSolid,LAXCAP_Butt,LAXJOIN_Miter);

		} else if (style&RECT_FLIP_LINE) {
			int fill;
			if (hover==RP_Flip1) fill=1; else fill=0;
			flatpoint p1=dp->realtoscreen(flip1);
			dp->drawpoint((int)p1.x,(int)p1.y,5,fill);

			if (hover==RP_Flip2) fill=1; else fill=0;
			flatpoint p2=dp->realtoscreen(flip2);
			dp->drawpoint((int)p2.x,(int)p2.y,5,fill);

			dp->LineAttributes(hover==RP_Flip_Go?3:1, LineSolid,LAXCAP_Butt,LAXJOIN_Miter);
			dp->drawline(p1,p2);
		}

		//if (mousetarget) {
		//	p=hoverpoint;
		//	dp->drawthing((int)p.x,(int)p.y,5,5,0,THING_Circle_Plus);
		//}


		dp->DrawReal(); 
	}

	dp->PopAxes();

	DBG cerr <<"end rect draw"<<endl;
	needtodraw=0;
	return 0;
}

/*! If (trans) then transform the data space point by somedata->m().
 * <pre>
 *  x+---- >
 *  y 1    8    7
 *  + 2    9    6
 *  | 3    4    5
 *  v
 * </pre>
 */
flatpoint RectInterface::getpoint(int c,int trans)
{
	if (!somedata) return flatpoint();
	flatpoint p;
	switch (c) {
		case RP_Middle:
		case RP_Move: p=flatpoint((somedata->minx+somedata->maxx)/2,(somedata->miny+somedata->maxy)/2); break;

		case RP_Rotate_SW:
		case RP_Scale_SW:
		case RP_SW: p=flatpoint(somedata->minx,somedata->miny); break;

		case RP_Scale_W:
		case RP_Shear_W:
		case RP_W : p=flatpoint(somedata->minx,(somedata->miny+somedata->maxy)/2); break;

		case RP_Rotate_NW:
		case RP_Scale_NW:
		case RP_NW: p=flatpoint(somedata->minx,somedata->maxy); break;

		case RP_Scale_N:
		case RP_Shear_N:
		case RP_N : p=flatpoint((somedata->minx+somedata->maxx)/2,somedata->maxy); break;

		case RP_Rotate_NE:
		case RP_Scale_NE:
		case RP_NE: p=flatpoint(somedata->maxx,somedata->maxy); break;

		case RP_Scale_E:
		case RP_Shear_E:
		case RP_E : p=flatpoint(somedata->maxx,(somedata->miny+somedata->maxy)/2); break;

		case RP_Rotate_SE:
		case RP_Scale_SE:
		case RP_SE: p=flatpoint(somedata->maxx,somedata->miny); break;

		case RP_Scale_S:
		case RP_Shear_S:
		case RP_S : p=flatpoint((somedata->minx+somedata->maxx)/2,somedata->miny); break;

		case RP_Center1: p=center1; break;    //center handle
		case RP_Center2: p=center2; break;    //rotate handle
		case RP_Shearpoint: p=shearpoint; break; //shear handle
	}
	if (trans) p=transform_point(somedata->m(),p);
	return p;
}

Laxkit::MenuInfo *RectInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	return menu;
	//----------
	//MenuInfo *menu=new MenuInfo;
	//menu->AddItem(_("Reset")   ,RECT_Reset);
	//menu->AddSep(_("Constraints"));
	//menu->AddItem(_("Aspect")  ,CONSTRAIN_Aspect  ,MENU_ISTOGGLE);
	//menu->AddItem(_("Scale")   ,CONSTRAIN_Scale   ,MENU_ISTOGGLE);
	//menu->AddItem(_("Rotation"),CONSTRAIN_Rotation,MENU_ISTOGGLE);
	//menu->AddItem(_("Shear")   ,CONSTRAIN_Shear   ,MENU_ISTOGGLE);
	//menu->AddItem(_("Position"),CONSTRAIN_Position,MENU_ISTOGGLE);
	//return menu;
}

/*! This is called from the ordinary scan(), but with some things computed already.
 * Use this when adding extra doodads for object control. Custom controls must return
 * values greater than RP_MAX.
 *
 * Default is return RP_None.
 */
int RectInterface::AlternateScan(flatpoint sp, flatpoint p, double xmag,double ymag, double onepix)
{ return RP_None; }

//! Return the point clicked on. RP_Move for on data, but not point, RP_None for nothing.
/*! 
 * If clicked in the data, and extra points only has center1, then return RP_Center2, and
 * for when there is center1 and center2, return RP_Shearpoint. This allows creation of temporary drag points, 
 * dependent on which of the center point, scale-drag point or shear points
 * are defined.
 */
int RectInterface::scan(int x,int y)
{
	if (!somedata) return -1;
	flatpoint p,p2;
	p=dp->screentoreal(x,y);
	p=transform_point_inverse(somedata->m(),p);
	double xx=p.x, yy=p.y;
	double xmag=norm(dp->realtoscreen(transform_point(somedata->m(),flatpoint(1,0)))
					-dp->realtoscreen(transform_point(somedata->m(),flatpoint(0,0))));
	double ymag=norm(dp->realtoscreen(transform_point(somedata->m(),flatpoint(0,1)))
					-dp->realtoscreen(transform_point(somedata->m(),flatpoint(0,0))));
	double d=-1,dd=(xmag>ymag?xmag:ymag);
	dd=5/dd; //5 pixel radius points for extrapoints
	dd*=dd;
	double fivepix2=dd;
	int match=RP_None;

	 //check against extrapoints if any
	if (extrapoints&HAS_CENTER1) {
		d=(p-center1)*(p-center1);
		if (d<dd) {
			dd=d;
			match=RP_Center1;
		}
	}
	if (extrapoints&HAS_CENTER2) {
		d=(p-center2)*(p-center2);
		if (d<dd) {
			dd=d;
			match=RP_Center2;
		}
	}
	if (extrapoints&HAS_SHEARPOINT) {
		d=(p-shearpoint)*(p-shearpoint);
		if (d<dd) {
			dd=d;
			match=RP_Shearpoint;
		}
	}
	if (match!=RP_None) {
		DBG cerr <<"recti hover scan: "<<match<<endl;
		return match;
	}

	 //check against flip controls
	if (style&RECT_FLIP_AT_SIDES) {
		flatpoint pp=flatpoint(somedata->maxx+maxtouchlen/xmag, (somedata->miny+somedata->maxy)/2);
		if (norm2(p-pp)<fivepix2) return RP_Flip_V;

		pp=flatpoint((somedata->minx+somedata->maxx)/2, somedata->maxy+maxtouchlen/ymag);
		if (norm2(p-pp)<fivepix2) return RP_Flip_H;

	} else if (style&RECT_FLIP_LINE) {
		d=(p-flip1)*(p-flip1);
		if (d<fivepix2) return RP_Flip1;

		d=(p-flip2)*(p-flip2);
		if (d<fivepix2) return RP_Flip2;

		d=distance(p, flip1,flip2);
		if (d<sqrt(fivepix2)) return RP_Flip_Go;
	}

	match=AlternateScan(flatpoint(x,y), p, xmag,ymag, dd/25); //p is in object coordinates
	if (match!=RP_None) return match;

	 //check against drag handles
	double xtouchlen, ytouchlen; //in real coords, not screen
	double maxtlen;
	double maxx=somedata->maxx;
	double maxy=somedata->maxy;
	double minx=somedata->minx;
	double miny=somedata->miny;

	maxtlen=maxtouchlen/xmag;
	xtouchlen=(somedata->maxx-somedata->minx)/4;
	if (xtouchlen>maxtlen) xtouchlen=maxtlen;

	maxtlen=maxtouchlen/ymag;
	ytouchlen=(somedata->maxy-somedata->miny)/4;
	if (ytouchlen>maxtlen) ytouchlen=maxtlen;

	DBG cerr <<"box x:"<<minx<<"--"<<maxx<<", y:"<<miny<<"--"<<maxy<<"  xtl:"<<xtouchlen<<"  ytl:"<<ytouchlen
	DBG      <<"  x,y="<<xx<<','<<yy<<endl;

	if (xx>=minx && xx<=maxx && yy>=miny && yy<=maxy) { //might be an inner handle
		if (xx<minx+xtouchlen) {
			if (yy<miny+ytouchlen) match=RP_Scale_SW;
			else if (yy>maxy-ytouchlen) match=RP_Scale_NW;
			else match=RP_Scale_W;

		} else if (xx>maxx-xtouchlen) {
			if (yy<miny+ytouchlen) match=RP_Scale_SE;
			else if (yy>maxy-ytouchlen) match=RP_Scale_NE;
			else match=RP_Scale_E;

		} else if (yy<miny+ytouchlen) match=RP_Scale_S;
		else if (yy>maxy-ytouchlen) match=RP_Scale_N;
		else match=RP_Move;

	} else if (xx>minx+xtouchlen && xx<maxx-xtouchlen) {
		if (yy<miny && yy>miny-ytouchlen/2) match=RP_Shear_S;
		else if (yy>maxy && yy<maxy+ytouchlen/2) match=RP_Shear_N;

	} else if (yy>miny+ytouchlen && yy<maxy-ytouchlen) {
		if (xx<minx && xx>minx-xtouchlen/2) match=RP_Shear_W;
		else if (xx>maxx && xx<maxx+xtouchlen/2) match=RP_Shear_E;

	} else if (xx<minx+xtouchlen && xx>minx-xtouchlen) {
		if (yy<miny+ytouchlen && yy>miny-ytouchlen) match=RP_Rotate_SW;
		else if (yy>maxy-ytouchlen && yy<maxy+ytouchlen) match=RP_Rotate_NW;

	} else if (xx>maxx-xtouchlen && xx<maxx+xtouchlen) {
		if (yy<miny+ytouchlen && yy>miny-ytouchlen) match=RP_Rotate_SE;
		else if (yy>maxy-ytouchlen && yy<maxy+ytouchlen) match=RP_Rotate_NE;
	}

	
	DBG cerr <<"recti hover scan: "<<match<<endl;
	return match;
}

//! Tries to set curpoint and flags for redrawing. Returns the curpoint.
/*! Does nothing if button not down.
 */
int RectInterface::SelectPoint(int c)
{
	if (!buttondown.any()) return RP_None;
	if (!data) return RP_None;
	if (c<=RP_None || c>=RP_MAX) return RP_None;

	int device=buttondown.whichdown(0,LEFTBUTTON);
	buttondown.moveinfo(device,LEFTBUTTON, c);
	needtodraw|=2;
	return c;
}

/*! Used when transferring control to a child RectInterface, and default to
 * the left button being down, and RP_Move. If !somedata, nothing is done.
 */
int RectInterface::FakeLBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (!somedata) return 1;

	buttondown.down(d->id,LEFTBUTTON,x,y,RP_Move);
	needtodraw=1;
	return 0;
}

/*! If style&RECT_OBJECT_SHUNT, then do not transfer control to another
 *  interface if clicking on another object. This allows subclassed object
 *  interfaces to stay in charge.
 */
int RectInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	//if (buttondown.isdown(d->id,LEFTBUTTON)) return 0;

	DBG cerr << "  in rect lbd..";

	shiftmode=0;

	if (somedata) {
		 //check for interaction with the current data

		int anchors=0;
		if (extrapoints&HAS_CENTER1) anchors++;
		if (extrapoints&HAS_CENTER2) anchors++;
		if (anchors+buttondown.any(0,LEFTBUTTON)>3) return 0; //do not allow more than 3 active points

		leftp=transform_point_inverse(somedata->m(),dp->screentoreal(x,y));
		//leftp=screentoreal(x,y);
		int c=scan(x,y);
		int curpoint=RP_None;
		DBG cerr <<"scan found: "<<c<<endl;

		if ((state&LAX_STATE_MASK)==ControlMask) {
//				&& (leftp.x<somedata->minx || leftp.x>somedata->maxx
//					|| leftp.y<somedata->miny || leftp.y>somedata->maxy)) {

			 //from here, you might be single-click scaling, or defining anchors.. we won't know
			 //until MouseMove

//			if (extrapoints==0) {
//				//curpoint=RP_Center1;
//				curpoint=RP_Move;
//				center1=leftp;
//				extrapoints|=HAS_CENTER1;
//			} else if (extrapoints==HAS_CENTER1) {
//				//curpoint=RP_Center2;
//				curpoint=RP_Move;
//				center2=leftp;
//				extrapoints|=HAS_CENTER2;
//			} else if (extrapoints==(HAS_CENTER1|HAS_CENTER2)) {
//				//curpoint=RP_Shearpoint;
//				curpoint=RP_Move;
//				shearpoint=leftp;
//				extrapoints|=HAS_SHEARPOINT;
//			} else curpoint=RP_Move;

			curpoint=RP_Move;

			if (curpoint!=RP_None) {
				buttondown.down(d->id,LEFTBUTTON,x,y,curpoint);
				needtodraw|=2;
			}

			DBG cerr <<"========  1"<<endl;
			DBG cerr <<"========lbd: center1 dev:"<<(extrapoints&HAS_CENTER1)<<endl;
			DBG cerr <<"========lbd: center2 dev:"<<(extrapoints&HAS_CENTER2)<<endl;
			DBG cerr <<"========lbd:   shear dev:"<<(extrapoints&HAS_SHEARPOINT)<<endl;

			return 0;

		} else if (extrapoints!=0
					&& c!=RP_Center1 && c!=RP_Center2 && c!=RP_Shearpoint
					&& leftp.x>=somedata->minx && leftp.x<=somedata->maxx
					&& leftp.y>=somedata->miny && leftp.y<=somedata->maxy) {
			 //we have a center defined, and we are clicking down inside the object,
			 //but not on an existing anchor. The device will now act as the
			 //next free anchor point.


			curpoint=RP_Move;
			buttondown.down(d->id,LEFTBUTTON,x,y,curpoint);
			needtodraw|=2;

			DBG cerr <<"========  2"<<endl;
			DBG cerr <<"========lbd: center1 dev:"<<(extrapoints&HAS_CENTER1)<<endl;
			DBG cerr <<"========lbd: center2 dev:"<<(extrapoints&HAS_CENTER2)<<endl;
			DBG cerr <<"========lbd:   shear dev:"<<(extrapoints&HAS_SHEARPOINT)<<endl;

			return 0;

		} else if (c!=RP_None) {
			 // straight click, must occur after some of the previous code
			curpoint=c;
			buttondown.down(d->id,LEFTBUTTON,x,y,curpoint);

			needtodraw|=2;
			//syncFromData(0);
			DBG cerr <<"========  3"<<endl;
			DBG cerr <<"========lbd: center1 dev:"<<(extrapoints&HAS_CENTER1)<<endl;
			DBG cerr <<"========lbd: center2 dev:"<<(extrapoints&HAS_CENTER2)<<endl;
			DBG cerr <<"========lbd:   shear dev:"<<(extrapoints&HAS_SHEARPOINT)<<endl;

			return 0;
		}
	}


	 //If this is a superclass of an ObjectInterface, this forces the rect interface
	 //to not transfer control to some other object and tool... bit of a hack
	if (style&RECT_OBJECT_SHUNT) return 0;

	 //! Get rid of old data if not clicking in it.
	if (somedata && !somedata->pointin(screentoreal(x,y))) deletedata();

	 // search for another viewport object to transfer control to
	if (!data && viewport) {
		ObjectContext *oc=NULL;
		SomeData *obj=NULL;
		int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
		if (c>0) obj=oc->obj;
		if (obj) {
			somedata=obj;
			somedata->inc_count();
			data=dynamic_cast<RectData *>(somedata);
			if (viewport) viewport->ChangeObject(oc,0); //incs count
			showdecs|=SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES;
			showdecs&=~SHOW_TARGET;
			needtodraw=1;
			return 0;
		}
		if (!primary && c==-1 && viewport->ChangeObject(oc,1)) {
			buttondown.up(d->id,LEFTBUTTON);
			deletedata();
			return 0;
		}
	}
   
	if (style&RECT_CANTCREATE) return 1;

	 // make new one
	deletedata(); //makes data=somedata=NULL
	extrapoints=0;

	somedata=NULL;
	somedata=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_RECTDATA));
	if (!somedata) somedata=new RectData;
	data=dynamic_cast<RectData *>(somedata); //has count=1
	if (!data) return 0;

	if (viewport) viewport->ChangeContext(x,y,NULL);
	//*** note that viewport is not notified of the new data

	buttondown.down(d->id,LEFTBUTTON,x,y,RP_Scale_NE);
	flatpoint p=screentoreal(x,y);
	data->style=creationstyle; 
	data->origin(flatpoint(p.x,p.y));
	data->xaxis(flatpoint(1e-6,0));
	data->yaxis(flatpoint(0,1e-6));
	data->setbounds(-100,100,-100,100);
	data->centercenter();
	center1=flatpoint((data->minx+data->maxx)/2,(data->miny+data->maxy)/2);
	createp=p;


	DBG flatpoint xxx=somedata->xaxis();
	DBG flatpoint yyy=somedata->yaxis();
	DBG cerr <<" rect axes b4: x:"<<xxx.x<<','<<xxx.y<<"  y:"<<yyy.x<<','<<yyy.y<<endl;
	syncFromData(1);

	DBG xxx=somedata->xaxis();
	DBG yyy=somedata->yaxis();
	DBG cerr <<" rect axes aft: x:"<<xxx.x<<','<<xxx.y<<"  y:"<<yyy.x<<','<<yyy.y<<endl;

	needtodraw=1;
	DBG cerr <<"..rectlbd done   ";

	return 0;
}

void RectInterface::Rotate(double angle)
{
	if (!somedata) return;
	flatpoint p;
	
	if (extrapoints&HAS_CENTER1) p=somedata->transformPoint(center1);
	else p=(somedata->transformPoint(flatpoint(somedata->minx,somedata->miny))+
				 somedata->transformPoint(flatpoint(somedata->maxx,somedata->maxy)))/2;
	somedata->Rotate(angle,p);

	syncFromData(0);
	needtodraw=1;
}

//! Flip the rectangle. type must be one of RP_Flip_Go, RP_Flip_H, or RP_Flip_V.
void RectInterface::Flip(int type)
{
	if (!somedata) return;

	DBG cerr <<" RectInterface::Flip()..."<<endl;
	if (type!=RP_Flip_Go && type!=RP_Flip_H && type!=RP_Flip_V) return;
	
	if (type==RP_Flip_H) {
		flip1.x=flip2.x=(somedata->minx+somedata->maxx)/2;
		flip1.y=somedata->miny;
		flip2.y=somedata->maxy;
	} else if (type==RP_Flip_V) {
		flip1.y=flip2.y=(somedata->miny+somedata->maxy)/2;
		flip1.x=somedata->minx;
		flip2.x=somedata->maxx;
	}

	flatpoint f1=transform_point(somedata->m(),flip1);
	flatpoint f2=transform_point(somedata->m(),flip2);

	double mf[6],mfinv[6],f[6];
	double t[6],tt[6];
	transform_set(f, 1,0,0,-1,0,0); //the flip transform, in y direction
	transform_from_basis(mf, f1, f2-f1, transpose(f2-f1)); //basis, p=flip1, x=flip2-flip1
	transform_invert(mfinv,mf);

	transform_mult(t, mfinv,f);
	transform_mult(tt, t,mf);
	transform_mult(t, somedata->m(),tt);
	somedata->m(t);

	syncFromData(1);
	needtodraw=1;
}

int RectInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	 //click down and up without dragging can toggle the existence of anchor points

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	int curpoint=RP_None;
	int dragged=buttondown.up(d->id,LEFTBUTTON, &curpoint);
	//if (somedata && viewport) viewport->ObjectMoved(somedata);

	if (!somedata) return 0;

	DBG cerr <<"========lbd: center1 dev:"<<(extrapoints&HAS_CENTER1)<<endl;
	DBG cerr <<"========lbd: center2 dev:"<<(extrapoints&HAS_CENTER2)<<endl;
	DBG cerr <<"========lbd:   shear dev:"<<(extrapoints&HAS_SHEARPOINT)<<endl;
	DBG cerr <<"========        curpoint:"<<curpoint<<endl;

	if (dragged && shiftmode==1) {
		extrapoints=0;
		shiftmode=0;
		needtodraw=1;
		return 0;
	}

	if (dragged==0) {
		if (curpoint==RP_Flip_Go || curpoint==RP_Flip_H || curpoint==RP_Flip_V) {
			Flip(curpoint);
			if ((state&LAX_STATE_MASK)==0) {
				 //remove flip controls
				style=(style&~RECT_FLIP_LINE)|RECT_FLIP_AT_SIDES;
			}
			needtodraw=1;
			return 0;
		}

		 // maybe create or remove an anchor point

		if ((extrapoints&HAS_CENTER1) && curpoint==RP_Center1) {
			 //toggle off center1
			extrapoints=0;
			needtodraw|=2;
			return 0;
		}

		if ((extrapoints&HAS_CENTER2) && curpoint==RP_Center2) {
			 //toggle off center2
			extrapoints=HAS_CENTER1;
			needtodraw|=2;
			return 0;
		}

		if ((extrapoints&HAS_SHEARPOINT) && curpoint==RP_Shearpoint) {
			 //toggle off shearpoint
			extrapoints=HAS_CENTER1|HAS_CENTER2;
			needtodraw|=2;
			return 0;
		}

		 //create anchor points only with control down
		if ((state&LAX_STATE_MASK)==ControlMask) {
			 //create 1st anchor point
			if (extrapoints==0 && curpoint==RP_Move) {
				extrapoints|=HAS_CENTER1;
				center1=transform_point_inverse(somedata->m(),dp->screentoreal(x,y));
				needtodraw|=2;
				return 0;
			}

			 // Create 2nd anchor point
			if (extrapoints==HAS_CENTER1 && curpoint==RP_Move) {
				extrapoints|=HAS_CENTER2;
				center2=transform_point_inverse(somedata->m(),dp->screentoreal(x,y));
				needtodraw|=2;
				return 0;
			}

			 // Create a shear point
			if (extrapoints==(HAS_CENTER1|HAS_CENTER2) && curpoint==RP_Move) {
				extrapoints|=HAS_SHEARPOINT;
				shearpoint=transform_point_inverse(somedata->m(),dp->screentoreal(x,y));
				needtodraw|=2;
				return 0;
			}
		}
	}

	return 0;
}

//! Return a string of a message for hovering over type p.
const char *RectInterface::hoverMessage(int p)
{
	if (p==RP_Flip1 || p==RP_Flip2) return _("Drag to move flip axis");
	if (p==RP_Flip_H || p==RP_Flip_V) return _("Click to flip, or drag to move axis");
	if (p==RP_Flip_Go) return _("Click to flip, shift-click to keep line");
	return NULL;
}

/*! move drags point, control-move rotates and shears
 * <pre>
 *     + is preserve aspect, todo is have some other toggle for square..
 *     ^ on corner is rotate
 *     ^ on midpoint is shear
 *   x,w+---->
 *  y,h 1    8    7
 *  +   2    9    6
 *  |   3    4    5
 *  v
 * </pre>
 *
 * \todo *** preserve aspect on a shift-resize
 */
int RectInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse) 
{
	if (!somedata) return 1;

	if (!buttondown.isdown(mouse->id,LEFTBUTTON)) {
		int oldhover=hover;
		hover=scan(x,y);

		if ((state&LAX_STATE_MASK)==ControlMask) {
			if (extrapoints==0) hover=RP_Faux_Center1;
			else if (extrapoints==HAS_CENTER1) hover=RP_Faux_Center2;
			else if (extrapoints==(HAS_CENTER1|HAS_CENTER2)) hover=RP_Faux_Shearpoint;
			hoverpoint=flatpoint(x,y);
			needtodraw|=2;

		} else if (oldhover!=hover) {
			needtodraw|=2;
			const char *mes=hoverMessage(hover);
			PostMessage(mes?mes:" ");
		}

		return 0;
	}

	int curpoint=RP_None,mx,my;
	buttondown.move(mouse->id,x,y,&mx,&my);
	buttondown.getextrainfo(mouse->id,LEFTBUTTON,&curpoint);

	if (curpoint==RP_None) { return 1; }
	if (mx==x && my==y) return 0;

	if (mousetarget) mousetarget=0;

	DBG flatpoint xx=somedata->xaxis();
	DBG flatpoint yy=somedata->yaxis();
	DBG cerr <<" rect axes: x:"<<xx.x<<','<<xx.y<<"  y:"<<yy.x<<','<<yy.y<<endl;

	DBG cerr <<"========lbd: center1 dev:"<<(extrapoints&HAS_CENTER1)<<endl;
	DBG cerr <<"========lbd: center2 dev:"<<(extrapoints&HAS_CENTER2)<<endl;
	DBG cerr <<"========lbd:   shear dev:"<<(extrapoints&HAS_SHEARPOINT)<<endl;
	DBG cerr <<"            mm: curpoint:"<<curpoint<<endl;


	 // move:    plain-move on center point
	 // resize:  plain-move on any border point
	 // shear: control-move on a mid point
	 // rotate: control-move on a corner point
 
	flatpoint d=dp->screentoreal(x,y) - dp->screentoreal(mx,my);
	flatpoint op;

	if (curpoint==RP_Flip_H || curpoint==RP_Flip_V || curpoint==RP_Flip1 || curpoint==RP_Flip2) {
		flatpoint np=transform_point_inverse(somedata->m(),dp->screentoreal( x, y));
		op=transform_point_inverse(somedata->m(),dp->screentoreal(mx,my));
		d=np-op;

		if (curpoint==RP_Flip_H || curpoint==RP_Flip_V) {
			style&=~RECT_FLIP_AT_SIDES;
			style|=RECT_FLIP_LINE;
			flip1=op;
			flip2=np;
			buttondown.moveinfo(mouse->id,LEFTBUTTON, RP_Flip2,0);
			needtodraw|=2;
			return 0;

		} else if (curpoint==RP_Flip1) {
			flip1+=d;
			needtodraw|=2;
			return 0;

		} else if (curpoint==RP_Flip2) {
			flip2+=d;
			needtodraw|=2;
			return 0;
		}
	}

	if (curpoint==RP_Move && extrapoints==0 && buttondown.any(0,LEFTBUTTON)==1) {
		 // moving, but there are no defined anchor points, and no other devices down.
		 // this shifts with no modifiers or shift, scales on control, rotates on shift-control

		shiftmode=1;
		int ix,iy;
		buttondown.getinitial(mouse->id,LEFTBUTTON,&ix,&iy);
		flatpoint leftp=dp->screentoreal(ix,iy); //real constant point
		leftp=transform_point_inverse(somedata->m(),leftp);
		DBG cerr <<"  initial scr:"<<ix<<","<<iy<<"  real:"<<leftp.x<<','<<leftp.y<<"   "<<somedata->whattype()<<endl;

		if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
			somedata->origin(somedata->origin()+d);

		} else if ((state&LAX_STATE_MASK)==ControlMask) {
			 //scale, with leftp as center
			double dd=double(x-mx);
			dd=1+.02*dd;
			if (dd<0.1) dd=0.1;
			flatpoint p=transform_point(somedata->m(),leftp); //screen constant point
			somedata->xaxis(dd*somedata->xaxis());
			somedata->yaxis(dd*somedata->yaxis());
			somedata->origin(somedata->origin()+p-transform_point(somedata->m(),leftp));

		} else if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
			 // rotate around leftp based on x movement
			double a;
			a=(x-mx)/180.0*M_PI;
			flatpoint p=transform_point(somedata->m(),leftp);
			somedata->xaxis(rotate(somedata->xaxis(),a,0));
			somedata->yaxis(rotate(somedata->yaxis(),a,0));
			somedata->origin(somedata->origin()+p-transform_point(somedata->m(),leftp));
		}

		needtodraw=1;
		syncFromData(0);
		return 0; 
	}

	//if (buttondown.any(0,LEFTBUTTON)>=2 || (extrapoints && buttondown.any(0,LEFTBUTTON))) {
	if (curpoint==RP_Move) {
		 //if we are moving, this could mean many things.
		 //If engaged with more than just a single anchor point, then we ignore
		 //the drag handles, and try to map each device to a proper point, and
		 //react accordingly

		int action=RP_Move;
		int devicepoint=0;
		flatpoint c1,c2,sh;
		if (extrapoints) { //map existing anchors
			c1=transform_point(somedata->m(),center1);
			devicepoint++;
			if (extrapoints&HAS_CENTER2) {
				c2=transform_point(somedata->m(),center2);
				devicepoint++;
			}
		}
		if (buttondown.any(0,LEFTBUTTON)>1) { //map other devices to anchors
			int dev=0;
			int mx,my;
			do {
				dev=buttondown.whichdown(dev);
				if (!dev) break;
				if (dev==mouse->id) continue;
				buttondown.getcurrent(dev,LEFTBUTTON,&mx,&my);
				flatpoint fp=dp->screentoreal(mx,my);
				if (devicepoint==0) {
					c1=fp;
					devicepoint++;
				} else if (devicepoint==1) {
					c2=fp;
					devicepoint++;
				} else {
					break;
				}
			} while (dev);
		}

		 //at this point, devicepoint is 0, 1, or 2, corresponding to center1, center2, or shearpoint.
		 //The current mouse will move that point.
		if (devicepoint==1) {
			action=RP_Center2;
			c2=dp->screentoreal(x,y);
		} else if (devicepoint==2) {
			action=RP_Shearpoint;
			sh=dp->screentoreal(x,y);
		} else {
			action=RP_Move;
		}

		if (action==RP_Move) {
			somedata->origin(somedata->origin()+d);
			needtodraw|=2;
			return 0; 
		}
		
		if (action==RP_Shearpoint) {
			 // on the shear handle
			DBG cerr <<"==============shear: curpoint="<<curpoint<<endl;

			if ((state&LAX_STATE_MASK)!=0) {
				 // only reposition shearpoint when modifiers on
				shearpoint=transform_point_inverse(somedata->m(),getpoint(RP_Shearpoint,1)+d);
				needtodraw|=2;

			} else {
				 //keep center1 and center2 constant, move shearpoint
				flatpoint o,x,y1,y2,p;
				o=c1;
				x=c2-o;
				y1=sh-o;
				y2=y1+d;

				double M[6],  // matrix before
					   M2[6], // matrix after 
					   N[6],  // M * N = M2, or M * N * M2^-1 == I
					   T[6];  // temp matrix
				 // Basic linear algebra: Transform a generic M to M2 with N.
				 //  We know M and M2, so find N. This same N
				 //  transforms any other affine matrix. so the somedata->m() (sm,sm2) is
				 //  found with: SM * N * SM2 == I, so SM2 = (SM * N)^-1
				transform_from_basis(M ,o,x,y1);
				transform_from_basis(M2,o,x,y2);
				transform_invert(T,M);
				transform_mult(N,T,M2);
				transform_mult(T,somedata->m(),N);
				somedata->m(T);

				needtodraw|=1;
				syncFromData(0);
			}

			needtodraw=1;
			return 0; 
		}

		 // on the rotate handle
		 // plain drag rotates and scales, 
		 // + moves all
		 // ^ scales only
		 // +^ rotates only
		if (action==RP_Center2) { 
			DBG cerr <<"==============rotate around center2: curpoint="<<curpoint<<endl;
			if ((state&LAX_STATE_MASK)==ShiftMask) {
				DBG cerr <<"==============move"<<endl;
				 //shift whole around
				somedata->origin(somedata->origin()+d);

			} else if ((state&LAX_STATE_MASK)==ControlMask) {
				 //scale only around RP_Center1
				double dd=double(x-mx);
				dd=1+.02*dd;
				if (dd<0.1) dd=0.1;
				DBG cerr <<"==============scale only "<<dd<<endl;
				flatpoint p=transform_point_inverse(somedata->m(),c1);
				somedata->xaxis(dd*somedata->xaxis());
				somedata->yaxis(dd*somedata->yaxis());
				somedata->origin(somedata->origin()+c1-transform_point(somedata->m(),p));

			} else if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
				 //rotate only around RP_Center1
				DBG cerr <<"==============rotate only"<<endl;
				double a;
				a=(x-mx)/180.0*M_PI;
				flatpoint p=transform_point_inverse(somedata->m(),c1);
				somedata->xaxis(rotate(somedata->xaxis(),a,0));
				somedata->yaxis(rotate(somedata->yaxis(),a,0));
				somedata->origin(somedata->origin()+c1-transform_point(somedata->m(),p));

			} else if ((state&LAX_STATE_MASK)==0) {
				DBG cerr <<"==============rotate and scale"<<endl;
				 //rotate and scale around RP_Center1
				flatpoint o,x1,x2,y1,y2,p;
				o=c1;
				x1=c2-o;
				x2=x1+d;
				y1=transpose(x1);
				y2=transpose(x2);

				double M[6],M2[6],N[6],T[6];
				transform_from_basis(M ,o,x1,y1);
				transform_from_basis(M2,o,x2,y2);
				transform_invert(T,M);
				transform_mult(N,T,M2);
				transform_mult(T,somedata->m(),N);
				somedata->m(T);
			}
			needtodraw=1;
			syncFromData(0);
			return 0; 
		}
	} //if dragging around anchor points

	if (curpoint==RP_Shearpoint) {
		 //just shift shearpoint
		shearpoint=transform_point_inverse(somedata->m(),getpoint(RP_Shearpoint,1)+d);
		needtodraw|=2;
		return 0;
	}

	if (curpoint==RP_Center2) {
		 //just shift center2
		center2=transform_point_inverse(somedata->m(),getpoint(RP_Center2,1)+d);
		needtodraw|=2;
		return 0;
	}

	if (curpoint==RP_Center1) {
		 //just shift center1
		center1=transform_point_inverse(somedata->m(),getpoint(RP_Center1,1)+d);
		needtodraw|=2;
		return 0;
	}

	//--------------deal with shifting handles

	 // handle rotating from dragging on corner point
	if (curpoint==RP_Rotate_NW || curpoint==RP_Rotate_NE || curpoint==RP_Rotate_SE || curpoint==RP_Rotate_SW) {
		flatpoint center,np;
		if (extrapoints) center=transform_point(somedata->m(),center1);
		else center=getpoint(RP_Middle,1);

		op=dp->screentoreal(mx,my) - center;
	 	np=op + d;
		if ((op*op)*(np*np)!=0) {
			double a=asin((op.x*np.y-op.y*np.x)/sqrt((op*op)*(np*np)));
			xdir=rotate(xdir,a,0);
			ydir=rotate(ydir,a,0);
			somedata->xaxis((fabs(xaxislen)>1e-10?xaxislen:1e-10)*xdir);
			somedata->yaxis((fabs(yaxislen)>1e-10?yaxislen:1e-10)*ydir);
			if (extrapoints) origin-=transform_point(somedata->m(),center1) - center;
			else origin-=getpoint(RP_Middle,1) - center;
			somedata->origin(origin);
			needtodraw|=2;
			syncFromData(0);
			return 0;
		}
		return 0;
	}

	 //define the transformed midpoints, used by shearing and simple resize
	flatpoint ql=transform_point(somedata->m(),flatpoint(somedata->minx,0)),
			  qr=transform_point(somedata->m(),flatpoint(somedata->maxx,0)),
			  qb=transform_point(somedata->m(),flatpoint(0,somedata->miny)),
			  qt=transform_point(somedata->m(),flatpoint(0,somedata->maxy));

	int keepaspect=(somedata->flags&(SOMEDATA_KEEP_ASPECT|SOMEDATA_KEEP_1_TO_1)); //***<--treats them the same
	if ((state&LAX_STATE_MASK)&ControlMask) keepaspect=!keepaspect;

	 // simple resizing by dragging border points
	//if (((state&LAX_STATE_MASK)|ShiftMask)==ShiftMask) {
	if (curpoint==RP_Scale_SW || curpoint==RP_Scale_W || curpoint==RP_Scale_NW || curpoint==RP_Scale_N
			|| curpoint==RP_Scale_NE || curpoint==RP_Scale_E || curpoint==RP_Scale_SE || curpoint==RP_Scale_S) {
		 // state is shift or plain

		int dl=0, //these get set to 1 if there is a change for that edge (left, right, top, bottom)
			dr=0,
			dt=0,
			db=0,
			ip=0, //the invariant point
			cfp=createfrompoint; //whether to scale symmetrically around center1
		switch (curpoint) {
			case RP_Scale_SW:
				db=dl=1;
				ip=RP_Scale_NE; 
				if (cfp&2) dt=-1;
				if (cfp&1) dr=-1; 
				if (cfp==1) ip=RP_Scale_N; else if (cfp==2) ip=RP_Scale_E; else if (cfp==3) ip=RP_Center1;
				break;
			case RP_Scale_W:
				dl=1;
				if (cfp&1) { dr=-1; ip=RP_Center1; } else ip=RP_Scale_E;
				break;
			case RP_Scale_NW:
				dl=dt=1;
				ip=RP_Scale_SE;
				if (cfp&2) db=-1;
				if (cfp&1) dr=-1;
				if (cfp==1) ip=RP_Scale_S; else if (cfp==2) ip=RP_Scale_E; else if (cfp==3) ip=RP_Center1;
				break;
			case RP_Scale_N:
				dt=1;
				if (cfp&2) { db=-1; ip=RP_Center1; } else ip=RP_Scale_S;
				break;
			case RP_Scale_NE:
				dt=dr=1;
				ip=RP_Scale_SW;
				if (cfp&2) db=-1;
				if (cfp&1) dl=-1;
				if (cfp==1) ip=RP_Scale_S; else if (cfp==2) ip=RP_Scale_W; else if (cfp==3) ip=RP_Center1;
				break; 
			case RP_Scale_E:
				dr=1;
				if (cfp&1) { dl=-1; ip=RP_Center1; } else ip=RP_Scale_W;
				break;
			case RP_Scale_SE:
				db=dr=1;
				ip=RP_Scale_NW;
				if (cfp&2) dt=-1;
				if (cfp&1) dl=-1;
				if (cfp==1) ip=RP_Scale_N; else if (cfp==2) ip=RP_Scale_W; else if (cfp==3) ip=RP_Center1;
				break;
			case RP_Scale_S:
				db=1;
				if (cfp&1) { dt=-1; ip=RP_Center1; } else ip=RP_Scale_N;
				break;
		}
		//double m[6];
		//transform_invert(m,somedata->m());
		//d=transform_vector(m,d);
		op=getpoint(ip,1);
		//double oldw=(qr-ql)*xdir,
		       //oldh=(qt-qb)*ydir;
		if (dl) ql+=dl*d||xdir;
		if (dr) qr+=dr*d||xdir;
		if (db) qb+=db*d||ydir;
		if (dt) qt+=dt*d||ydir;
		double oldaspect=norm(somedata->yaxis())/norm(somedata->xaxis());

		double neww=(qr-ql)*xdir, //old span between right and left
			   newh=(qt-qb)*ydir; //old span between top and bottom
		if (dl || dr) {
			 // correct xaxis
			xaxislen=neww/(somedata->maxx-somedata->minx);
			somedata->xaxis((fabs(xaxislen)>1e-10?xaxislen:1e-10)*xdir);
		}
		if (dt || db) {
			 // correct yaxis
			yaxislen=newh/(somedata->maxy-somedata->miny);
			somedata->yaxis((fabs(yaxislen)>1e-10?yaxislen:1e-10)*ydir);
		}

		 //// ensure preserving aspect
		 //SOMEDATA_KEEP_ASPECT
		 //SOMEDATA_KEEP_1_TO_1  ***<--treats them the same
		if (keepaspect) {
			//if ((dl||dr) && !(dt||db)) {
			if ((dl||dr)) {
				 //left and right changed, but not top or bottom
				somedata->yaxis(somedata->yaxis()/norm(somedata->yaxis())*norm(somedata->xaxis())*oldaspect);
			} else if ((dt||db) && !(dl||dr)) {
				 // top or bottom changed, but not left and right
				somedata->xaxis(somedata->xaxis()/norm(somedata->xaxis())*norm(somedata->yaxis())/oldaspect);
			}
			//if (fabs(data->w)>fabs(data->h)) data->w=fabs(data->h)*(data->w>0?1:-1);
			//else data->h=fabs(data->w)*(data->h>0?1:-1);
		}

		 // now fix origin based on ip, the invariant point
		if (dl || dr || dt || db) {
			somedata->origin(somedata->origin()-getpoint(ip,1)+op);
			origin=somedata->origin();
		}

		if (data) data->centercenter();
		needtodraw|=2;
		syncFromData(0);
		return 0;
	}

	if (curpoint!=RP_Shear_E && curpoint!=RP_Shear_W && curpoint!=RP_Shear_S && curpoint!=RP_Shear_N)
		return 0;
	
	 // else is shearing on mid point
	int ip=0;
	flatpoint v;
	double n;
	switch (curpoint) {
		case RP_Shear_E:
		case RP_Shear_W: {
				double inw=somedata->maxx-somedata->minx;
				v=inw*xaxislen*xdir;
				if (state&ShiftMask) d=d||ydir;
				if (curpoint==RP_Shear_W) { ip=RP_Shear_E; v-=d; }
				else { ip=RP_Shear_W; v+=d; }
				op=getpoint(ip,1);
				n=sqrt(v*v);
				if (!n) break;
				xdir=v/n;
				xaxislen=n/(somedata->maxx-somedata->minx);

				DBG cerr <<" SHEAR e|w === "<< (fabs(xdir.x*ydir.y-ydir.x*xdir.y)) <<"  xaxis: "<<fabs(xaxislen)<<endl;

				if (fabs(xdir.x*ydir.y-ydir.x*xdir.y)>1e-5) //make sure x and y are not collinear
					somedata->xaxis((fabs(xaxislen)>1e-10 ? xaxislen : 1e-10)*xdir);
			} break;
		case RP_Shear_S:
		case RP_Shear_N: {
				double inh=somedata->maxy-somedata->miny;
				v=inh*yaxislen*ydir;
				if (state&ShiftMask) d=d||xdir;
				if (curpoint==RP_Shear_S) { ip=RP_Shear_N; v-=d; }
				else { ip=RP_Shear_S; v+=d; }
				op=getpoint(ip,1);
				n=sqrt(v*v);
				if (!n) break;
				ydir=v/n;
				yaxislen=n/(somedata->maxy-somedata->miny);

				DBG cerr <<" SHEAR s|n === "<< (fabs(xdir.x*ydir.y-ydir.x*xdir.y)) <<"  yaxis: "<<fabs(yaxislen)<<endl;

				if (fabs(xdir.x*ydir.y-ydir.x*xdir.y)>1e-5) //make sure x and y are not collinear
					somedata->yaxis((fabs(yaxislen)>1e-10 ? yaxislen : 1e-10)*ydir);
			} break;
	}

	if (ip) {
		somedata->origin(somedata->origin()-getpoint(ip,1)+op);
		origin=somedata->origin();
		if (data) data->centercenter();
		needtodraw=1;
	}

	syncFromData(0);
	return 0;
}

Laxkit::ShortcutHandler *RectInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(RIA_Decorations,   'D',ShiftMask,0,"Decorations",   _("Toggle decorations"),NULL,0);
	sc->Add(RIA_Normalize,     'n',0,0,        "Normalize",     _("Normalize scale, clear skew"),NULL,0);
	sc->Add(RIA_Rectify,       'N',ShiftMask,0,"Rectify",       _("Normalize scale, clear skew and rotation"),NULL,0);
	sc->Add(RIA_Constrain,     'c',0,0,        "Constrain",     _("Toggle drag constraint"),NULL,0);
	sc->AddAction(RIA_MoveCenter,              "MoveCenter",    _("Move rotation center to common points"),NULL,0,0);
	sc->Add(RIA_ExpandHandle,  '>',ShiftMask,0,"ExpandHandle",  _("Expand handle size"),NULL,0);
	sc->Add(RIA_ContractHandle,'<',ShiftMask,0,"ContractHandle",_("Contract handle size"),NULL,0);

	sc->Add(RIA_RotateCW,      'r',0,0,        "RotateCW",      _("Rotate clockwise"),NULL,0);
	sc->Add(RIA_RotateCCW,     'R',ShiftMask,0,"RotateCCW",     _("Rotate counter clockwise"),NULL,0);

	sc->Add(RIA_FlipHorizontal    ,'h',0,0,    "FlipH",         _("Flip horizontally"),NULL,0);
	sc->Add(RIA_FlipVertical      ,'v',0,0,    "FlipV",         _("Flip vertically"),NULL,0);
	sc->Add(RIA_ToggleFlipControls,'f',0,0,    "FlipToggleShow",_("Toggle showing flip controls"),NULL,0);


	manager->AddArea(whattype(),sc);
	return sc;
}

int RectInterface::PerformAction(int action)
{
	if (action==RIA_Decorations) {
		//int IO=(SHOW_OUTER_HANDLES|SHOW_INNER_HANDLES);
		//int d=showdecs&~IO;
		//IO=(showdecs&IO);

		//if (IO==(SHOW_OUTER_HANDLES|SHOW_INNER_HANDLES)) d|=SHOW_OUTER_HANDLES;
		//else if (IO==SHOW_OUTER_HANDLES) ; //show no handles
		//else d|=SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES;
		//showdecs=d;

		int sshowdecs = (showdecs&(SHOW_OUTER_HANDLES|SHOW_INNER_HANDLES));

		if ((sshowdecs&SHOW_OUTER_HANDLES) && (sshowdecs&SHOW_INNER_HANDLES)) {
			sshowdecs=SHOW_OUTER_HANDLES;
			PostMessage(_("Show outer handles"));

		} else if (sshowdecs&SHOW_OUTER_HANDLES) {
			sshowdecs=SHOW_INNER_HANDLES;
			PostMessage(_("Show inner handles"));

		} else if ((sshowdecs&SHOW_INNER_HANDLES)) {
			sshowdecs=0;
			PostMessage(_("Don't show control handles"));

		} else {
			sshowdecs = SHOW_OUTER_HANDLES|SHOW_INNER_HANDLES;
			PostMessage(_("Show inner and outer handles"));
		}
		showdecs = (showdecs&~(SHOW_OUTER_HANDLES|SHOW_INNER_HANDLES))|sshowdecs;

		needtodraw=1;
		return 0;

	} else if (action==RIA_FlipHorizontal) {
		Flip(RP_Flip_H);
		return 0;

	} else if (action==RIA_FlipVertical) {
		Flip(RP_Flip_V);
		return 0;

	} else if (action==RIA_RotateCW) {
		if (!somedata) return 1;
		Rotate(-rotatestep);
		return 0;

	} else if (action==RIA_RotateCCW) {
		if (!somedata) return 1;
		Rotate(rotatestep);
		return 0;

	} else if (action==RIA_ToggleFlipControls) {
		style=(style&~RECT_FLIP_LINE)|RECT_FLIP_AT_SIDES;
		needtodraw=1;
		return 0;

	} else if (action==RIA_Normalize || action==RIA_Rectify) {
		if (!somedata) return 1;
		if (action==RIA_Rectify) {
			double x=norm(somedata->xaxis());
			somedata->xaxis(flatpoint(x,0));
		}
		flatpoint center=getpoint(RP_Center1,1);
		somedata->yaxis(transpose(somedata->xaxis()));
		center=center-getpoint(RP_Center1,1);
		somedata->origin(somedata->origin()+center);
		syncFromData(0);
		needtodraw=1;
		return 0;

	} else if (action==RIA_Constrain) {
		createfrompoint=(createfrompoint+1)%4;

		DBG switch(createfrompoint) {
		DBG 	case 0: app->postmessage("createfrompoint=0"); return 0;
		DBG 	case 1: app->postmessage("createfrompoint=x"); return 0;
		DBG 	case 2: app->postmessage("createfrompoint=y"); return 0;
		DBG 	case 3: app->postmessage("createfrompoint=xy"); return 0;
		DBG }
		return 0;

	} else if (action==RIA_MoveCenter) {
		if (!data) return 1;
		if (++data->centertype>LAX_BOTTOM_RIGHT) data->centertype=LAX_CUSTOM_ALIGNMENT;
		data->centercenter();
		needtodraw=1;
		return 0;

	} else if (action==RIA_ExpandHandle) {
		maxtouchlen--;
		if (maxtouchlen<3) maxtouchlen=3;
		needtodraw=1;
		return 0;

	} else if (action==RIA_ContractHandle) {
		maxtouchlen++;
		needtodraw=1;
		return 0;
	}

	return 1;
}

/*! Default is return 0. This is so you don't have to redefine CharInput when implementing
 * more complicated interface modes. You just add the shortcuts, and it works.
 */
int RectInterface::GetMode()
{ return 0; }

int RectInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{
	showdecs|=SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES; //counteract control down removing decorations
	showdecs&=~SHOW_TARGET;

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,GetMode());
	if (action>=0) {
		return PerformAction(action);
	}
	
	if (ch==LAX_Shift) { // shift
		if (!buttondown.any() || (state&ControlMask)) {
			if (showdecs==0) { showdecs|=SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES; needtodraw=1; }
			return 1;
		}
		if (data) data->style|=RECT_ISSQUARE;
		//***data->h=data->w; //***should be done according to curpoint
		//*** redo measurements
		return 0;

	} else if (ch==LAX_Control) {
		showdecs&=~(SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES);
		showdecs|=SHOW_TARGET;
		mousetarget++;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Esc) {
		if (style&RECT_FLIP_LINE) {
			style=(style&~RECT_FLIP_LINE)|RECT_FLIP_AT_SIDES;
			needtodraw|=2;
			return 0;
		}
		if (!extrapoints) return 1;
		extrapoints=0;
		needtodraw|=2;
		return 0;

	} else if ((ch==LAX_Del || ch==LAX_Bksp) && (state&LAX_STATE_MASK)==0) { 
		if (lastpoint==RP_Shearpoint || lastpoint==RP_Center1 || lastpoint==RP_Center2) {
			if (lastpoint==RP_Shearpoint) extrapoints=RP_Center1|RP_Center2;
			else if (lastpoint==RP_Center2) extrapoints=RP_Center1;
			else extrapoints=0;
			needtodraw|=2;
			return 0;
		}
		return 1;

	}

	return 1;
}
 
void RectInterface::Unmapped()
{
	showdecs |= (SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES);
}

/*! Shift/noshift toggles end/start point
 */
int RectInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d) 
{
	if (ch==LAX_Shift) { // shift
		if (!buttondown.any()) return 0;
		if (state&ControlMask) return 0;
		if (data) data->style&=~RECT_ISSQUARE; // toggle off circle

	} else if (ch==LAX_Control) { // shift
		showdecs|=SHOW_INNER_HANDLES|SHOW_OUTER_HANDLES;
		showdecs&=~SHOW_TARGET;
		hover=RP_None;
		if (mousetarget) mousetarget--;
		needtodraw=1;
		return 0;
	}

	return 1; 
}


} //namespace LaxInterfaces


