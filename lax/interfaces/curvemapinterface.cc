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
//    Copyright (C) 2013 by Tom Lechner
//

#include <lax/interfaces/curvemapinterface.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/bezutils.h>
#include <lax/language.h>

#include <iostream>
using namespace std;
#define DBG 


using namespace Laxkit;
using namespace LaxFiles;


namespace LaxInterfaces {


enum CurveMapIA {
	CURVEM_Invert,
	CURVEM_Reset,
	CURVEM_Select,
	CURVEM_Toggle_Wrap,
	CURVEM_MAX
};


//-------------------------------- CurveMapInterface ----------------------------------
/*! \class CurveMapInterface
 * Window to edit curves that map one range, one-to-one, to another range.
 */

CurveMapInterface::CurveMapInterface(int nid, Laxkit::Displayer *ndp,
						const char *nctitle,
						const char *xl, double nxmin, double nxmax,
						const char *yl, double nymin, double nymax)
				: anInterface(nid,ndp)
{
	firsttime = 1;

	padouter = padinner = 5;

	smallnumbers = app->fontmanager->MakeFont("Sans","",app->defaultlaxfont->textheight()/2,0);
	DBG smallnumbers->suppress_debug=1;

	style    = 0;
	editable = 0;
	always_refresh_lookup = 1;
	highlighteditable = 0;
	lasthover = -1;

	show_label_ranges = 1;
	show_labels = 1;
	show_hover  = 1;

	win_themestyle = app->theme->GetStyle(THEME_Panel);
	win_themestyle->inc_count();
	curve_color = win_themestyle->fg.Pixel();
	graph_color = app->theme->GetStyle(THEME_Edit)->bg.Pixel();

	curveinfo=new CurveInfo(nctitle, xl,nxmin,nxmax, yl,nymin,nymax);

	histogram=NULL;
	hist_n=0;

	sc=NULL;
}

CurveMapInterface::~CurveMapInterface()
{
	if (win_themestyle) win_themestyle->dec_count();
	if (smallnumbers) smallnumbers->dec_count();
	if (curveinfo) curveinfo->dec_count();

	if (histogram) delete[] histogram;

	if (sc) sc->dec_count();
}

anObject *NewCurveInfo(int p, anObject *refobj) { return new CurveInfo; }

int CurveMapInterface::InitializeResources()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);

	ObjectFactory *factory=imanager->GetObjectFactory(); 
	factory->DefineNewObject(-1, "CurveInfo", NewCurveInfo, NULL, 0);

	 //create builtin
	ResourceManager *resources=imanager->GetResourceManager();
	CurveInfo *curve;
	
	int type=CurveInfo::CURVE_Rising;
	while ((CurveInfo::CurveDefaults)type!=CurveInfo::CURVEMAX) {
		curve=new CurveInfo;
		curve->SetDefault((CurveInfo::CurveDefaults)type, true);
		curve->Id(curve->title);
		type++;
		resources->AddResource("CurveInfo", curve, NULL, curve->Id(), curve->Id(), NULL, NULL, NULL, true);
	}

	return 0;
}

void CurveMapInterface::Clear(SomeData *d)
{
}

//! Set values in rect to fit within rectangle x,y,w,h.
void CurveMapInterface::SetupRect(double x,double y,double w,double h)
{
	if (w>0 && h>0) {
		bounds.x=x;
		bounds.y=y;
		bounds.width=w;
		bounds.height=h;
	}

	rect.x      = x + padouter;
	rect.y      = y + padouter;
	rect.width  = w-2*padouter;
	rect.height = h-2*padouter;


	int textheight=dp->textheight();

	if (show_labels) {
		if (curveinfo->ylabel) {
			rect.y+=textheight+padinner;
			rect.height-=textheight+padinner;
		}

		if (curveinfo->xlabel) {
			rect.height -= padinner+smallnumbers->textheight(); //remove number height
		}
	}

	if (show_label_ranges) {
		if (!show_labels || !curveinfo->xlabel) rect.height -= padinner + smallnumbers->textheight(); //remove number height

		//char scratch[100];
		int extent=0;
		//int tmp=0;

		//sprintf(scratch, "%.5g",curveinfo->ymin);
		//extent=dp->textextent(smallnumbers, scratch,-1, NULL,NULL,NULL,NULL,0);

		//sprintf(scratch, "%.5g",curveinfo->ymax);
		//tmp=dp->textextent(smallnumbers, scratch,-1, NULL,NULL,NULL,NULL,0);
		//if (tmp>extent) extent=tmp;

		//extent+=padinner;
		//rect.x+=extent;
		extent=dp->textextent(smallnumbers, "5.5",-1, NULL,NULL,NULL,NULL,0);
		rect.x     += padinner + extent;
		rect.width -= padinner + extent;
	}

	needtodraw=1;
}

/*! Sends a SimpleMessage.
 * which==0 means curve changed, which>1 means a CurveMapInterfaceEditable changed.
 * which is put in event->info1.
 */
void CurveMapInterface::send(int which)
{
	if (owner && owner_message) {
		SimpleMessage *ev=new SimpleMessage;
		ev->info1=which;
		app->SendMessage(ev,owner->object_id,owner_message,object_id);
	}
}

double CurveMapInterface::f(double x)
{
	return curveinfo->f_linear(x);
}

Laxkit::MenuInfo *CurveMapInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (!menu) menu=new MenuInfo(_("Curves"));
	else if (menu->n()==0) menu->AddSep(_("Curves"));

	menu->AddItem(_("Invert"),    CURVEM_Invert, object_id);
	menu->AddItem(_("Reset"),     CURVEM_Reset,  object_id);
	menu->AddItem(_("Select..."), CURVEM_Select, object_id);
	return menu;
}

int CurveMapInterface::Event(const EventData *e,const char *mes)
{
    if (e->type==LAX_onMouseOut && highlighteditable) {
		highlighteditable=0;
		needtodraw=1;
	}

	if (!strcmp(mes,"menuevent")) {
		const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e);
		int i =s->info2; //id of menu item

		if (i==CURVEM_Invert || i==CURVEM_Reset) {
			PerformAction(i);
			return 0;

		} else if (i==CURVEM_Select) {

			//PerformAction(CURVEM_ToggleSelect);
			//
			//Builtin would be:
			//  0..1 linear, 1..0 linear
			//  sinusoid 0..1 and 1..0
			//  flat 0, flat .5, flat 1
			//need to figure out good way to store recent
			//
			//
			PostMessage(" *** need to implement curvemap Select!!");
			cerr <<" *** curvemap select todo!!!"<<endl;
			return 0;

		}

		return 0;
	}

	return anInterface::Event(e,mes);
}

/*! copies info into this->curveinfo
 */
int CurveMapInterface::CopyInfo(CurveInfo *info)
{
	if (!info) return 1;
	if (!curveinfo) curveinfo=new CurveInfo();
	(*curveinfo)=(*info);
	return 0;
}

/*! If info==NULL, dec_count current, and install a default.
 * Otherwise, dec_count old info, and inc_count new info.
 */
int CurveMapInterface::SetInfo(CurveInfo *info)
{
	if (!info) {
		if (curveinfo) curveinfo->dec_count();
		curveinfo=new CurveInfo();
		return 1;
	}
	if (info==curveinfo) return 0;
	if (curveinfo) curveinfo->dec_count();
	curveinfo=info;
	info->inc_count();
	return 0;
}

int CurveMapInterface::DrawData(Laxkit::anObject *ndata, Laxkit::anObject *a1, Laxkit::anObject *a2, int info)
{
	CurveInfo *curve = dynamic_cast<CurveInfo*>(ndata);
	if (!curve) return 0;

	CurveInfo *_curveinfo = curveinfo;
	curveinfo = curve;
	int _lasthover = lasthover;   lasthover = -1;
	int _showhover = show_hover;  show_hover = 0;

	int status = Refresh();

	lasthover  = _lasthover;
	curveinfo  = _curveinfo;
	show_hover = _showhover;

	return status;
}

int CurveMapInterface::Refresh()
{
	if (!needtodraw) return 0;
	

	if (firsttime) {
		if (bounds.width<=0 || bounds.height<=0) SetupRect(0,0,-1,-1);
		firsttime=0;
	}


	if ((style & RealSpace) == 0) dp->DrawScreen();

	dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

	//--------------
	dp->NewBG(win_themestyle->bg);
	dp->NewFG(win_themestyle->fg);
	//dp->ClearWindow();

	 //blank out graph area
	dp->NewFG(curve_color);
	dp->NewBG(graph_color);
	dp->drawrectangle(rect.x,rect.y,rect.width,rect.height, 2);

	DBG if (show_hover && rect.pointIsIn(hoverpoint.x,hoverpoint.y)) {
	DBG 	dp->drawline(hoverpoint.x-5,hoverpoint.y, hoverpoint.x+5,hoverpoint.y);
	DBG 	dp->drawline(hoverpoint.x,hoverpoint.y-5, hoverpoint.x,hoverpoint.y+5);
	DBG }


	 //draw histogram, if any
	if (histogram) {
		dp->NewFG(coloravg(graph_color,win_themestyle->fg,.9));
		int y;
		for (int x=rect.x; x<rect.x+rect.width; x++) {
			y = (rect.height-histogram[int(double(x-rect.x)/rect.width * hist_n)]/1000.*rect.height) + rect.y;
			dp->drawline(x,rect.y+rect.width, x,y);
		}
	}


	dp->NewFG(win_themestyle->fg);

	 //draw various highlighted backgrounds of editables
	if (highlighteditable) {
		dp->NewFG(coloravg(win_themestyle->bg,win_themestyle->fg,.1));

		if (highlighteditable == YUnits)
			dp->drawrectangle(0,0,rect.x,rect.y,1);

		else if (highlighteditable == XUnits)
			dp->drawrectangle(rect.x+rect.width*.25,rect.y+rect.height, rect.width*.5,bounds.height-(rect.y+rect.height),1);

		else if (highlighteditable == XMax)
			dp->drawrectangle(rect.x+rect.width*.75,rect.y+rect.height, rect.width*.25,bounds.height-(rect.y+rect.height),1);

		else if (highlighteditable == XMin)
			dp->drawrectangle(rect.x,rect.y+rect.height, rect.width*.25,bounds.height-(rect.y+rect.height),1);

		else if (highlighteditable == YMax)
			dp->drawrectangle(bounds.x,rect.y, rect.x-bounds.x,rect.height*.3,1);

		else if (highlighteditable == YMin)
			dp->drawrectangle(bounds.x,rect.y+rect.height*.7,rect.x,rect.height*.3,1);

		dp->NewFG(win_themestyle->fg);
	}

	 //draw various labels
	if (show_labels) {
		if (curveinfo->title) dp->textout(bounds.x+bounds.width/2, bounds.y+padouter, curveinfo->title,-1, LAX_HCENTER|LAX_TOP);

		dp->font(smallnumbers);
		if (curveinfo->ylabel) dp->textout(bounds.x+padouter, rect.y-padinner, curveinfo->ylabel,-1, LAX_LEFT|LAX_BOTTOM);
		if (curveinfo->xlabel) dp->textout(rect.x+rect.width/2,rect.y+rect.height+padinner, curveinfo->xlabel,-1, LAX_HCENTER|LAX_TOP);
		dp->font(anXApp::app->defaultlaxfont,-1);
	}

	if (show_label_ranges) {
		dp->font(smallnumbers);

		char scratch[100];
		int e;
		int x;

		if (buttondown.any() && show_hover) {
			sprintf(scratch,  "%.4g, %.4g",lastpoint.x,lastpoint.y);
			dp->textout(rect.x,rect.y, scratch,-1, LAX_LEFT|LAX_TOP);
		}

		sprintf(scratch, "%.3g",curveinfo->ymax); //ymax
		e=dp->textextent(scratch,-1, NULL,NULL);
		if (rect.x-padinner-e<0) x=e; else x=rect.x-padinner;
		dp->textout(x,rect.y, scratch,-1, LAX_RIGHT|LAX_TOP);

		sprintf(scratch, "%.3g",curveinfo->ymin); //ymin
		dp->textout(rect.x-padinner,rect.y+rect.height, scratch,-1, LAX_RIGHT|LAX_BOTTOM);

		sprintf(scratch, "%.5g",curveinfo->xmin); //xmin
		dp->textout(rect.x,rect.y+rect.height+padinner, scratch,-1, LAX_LEFT|LAX_TOP);

		sprintf(scratch, "%.5g",curveinfo->xmax); //xmax
		dp->textout(rect.x+rect.width,rect.y+rect.height+padinner, scratch,-1, LAX_RIGHT|LAX_TOP);

		dp->font(anXApp::app->defaultlaxfont,-1);
	}

	 //Debugging: show lookup
	//dp->NewFG(1.,0.,0.);
	//if (curveinfo->lookup) for (int c=rect.x; c<rect.x+rect.width; c++) {
	//	int y=rect.height-curveinfo->lookup[(c-rect.x)*255/rect.width]*rect.height/255 + rect.y;
	//	dp->drawpoint(c,y, 2,0);
	//}


	 //finally draw the curve, scale points to be in rect
	flatpoint p1,p2;
	dp->NewFG(curve_color);
	if (curveinfo->curvetype==CurveInfo::Linear) {
		for (int c=0; c<curveinfo->points.n; c++) {
			 //draw line
			p1=curveinfo->points.e[c]; //is point in range 0..1
			p1.y=1-p1.y;
			p1.x=rect.x + p1.x*rect.width;
			p1.y=rect.y + p1.y*rect.height;

			 //draw points
			dp->drawpoint(p1, 3,0);

			if (c==0) { p2=p1; continue; }
			dp->drawline(p2,p1);
			p2=p1;
		}

	} else {
		flatpoint *pts;
		int n;
		if (curveinfo->curvetype==CurveInfo::Autosmooth) {
			if (!curveinfo->fauxpoints.n) curveinfo->MakeFakeCurve();
			pts=curveinfo->fauxpoints.e;
			n  =curveinfo->fauxpoints.n;
		} else {
			pts=curveinfo->points.e;
			n  =curveinfo->points.n;
		}

		flatpoint fc[n];
		memcpy(fc, pts, n*sizeof(flatpoint));

		for (int c=0; c<n; c++) {
			fc[c].x=  fc[c].x  *rect.width  + rect.x;
			fc[c].y=(1-fc[c].y)*rect.height + rect.y;
		}
		//dp->NewFG(0.,1.,0.);
		dp->drawbez(fc,n/3,0,0);

	}


	 //draw control points
	for (int c=0; c<curveinfo->points.n; c++) {
		p1=curveinfo->points.e[c]; //is point in range 0..1
		p1.y=1-p1.y;
		p1.x=rect.x + p1.x*rect.width;
		p1.y=rect.y + p1.y*rect.height;

		 //draw points
		if (c==lasthover) dp->drawpoint(p1, 5,1);
		else dp->drawpoint(p1, 3,0);
	}


	if ((style & RealSpace) == 0) dp->DrawReal();
	needtodraw=0;
	return 0;

} //Refresh()

//! Make any CurveMapInterfaceEditable masked in editable editable (on==1) or not (on==0).
void CurveMapInterface::ChangeEditable(unsigned int which, int on)
{
	if (on) editable|=which;
	else editable&=~which;
}

/*! Scan for the various editable lables.
 */
int CurveMapInterface::scaneditable(double x,double y)
{
	int found=0;
	if (x<rect.x) {
		if (y<rect.y) found = YUnits;
		else if (y<rect.y+rect.height/3) found = YMax;
		else if (y>rect.y+rect.height*2/3) found = YMin;

	} else if (y>rect.y+rect.height) {
		if (x<rect.x+rect.width*.25) found = XMin;
		else if (x<rect.x+rect.width*.75) found = XUnits;
		else if (x>=rect.x+rect.width*.75) found = XMax;
	}
	found=(found&editable);

	return found;
}

int CurveMapInterface::scanExpandable(double x,double y)
{
	if (style & Expandable) {
		if (x > rect.x + rect.width*.8 && x < rect.x + rect.width) {
			if (y > rect.y + rect.height*.8 && y < rect.y + rect.height) return ExpandLR;
			if (y > rect.y && y < rect.y + .2*rect.height) return ExpandUR;

		} else if (x > rect.x && x < rect.x + .2*rect.width) {
			if (y > rect.y + rect.height*.8 && y < rect.y + rect.height) return ExpandLL;
			if (y > rect.y && y < rect.y + .2*rect.height) return ExpandUL;
		}
	}

	return 0;
}

//! Scan for existing point, return index in curveinfo->points, or -1 for not found.
int CurveMapInterface::scan(double x,double y)
{
	double scandistance=10;
	flatpoint fp(x,y);
	flatpoint p1;

	 //scan for existing points
	for (int c=0; c<curveinfo->points.n; c++) {
		p1=curveinfo->points.e[c]; //is point in range 0..1
		p1.y=1-p1.y;
		p1.x=rect.x + p1.x*rect.width;
		p1.y=rect.y + p1.y*rect.height;

		if (norm(fp-p1)<scandistance) return c;
	}

	return -1;
}

/*! Return a potential point to add to the curve. This is a point near the curve,
 * but not an existing point.
 * If no point is near 0 is returned, else 1.
 *
 * If found, the point should be inserted at position returned in index. The
 * point returned in p_ret is a unit point (in range 0..1).
 *
 * If x,y is not within a certain scan distance, then point is not detected.
 */
int CurveMapInterface::scannear(double x,double y, flatpoint *p_ret, int *index)
{

	flatpoint p((x-rect.x)/rect.width, (rect.y+rect.height-y)/rect.height);
	flatpoint lp;
	flatpoint v,p1,p2;
	double scandistance=20./rect.height;

	if (p.x<0 || p.x>1 || p.y<0 || p.y>1) return 0;

	lp.x=p.x;
	lp.y=curveinfo->f(p.x);
	lp=curveinfo->MapToUnitPoint(lp);

	if (fabs(lp.y-p.y)<scandistance) {
		int c=0;
		for (c=0; c<curveinfo->points.n; c++) {
			if (lp.x<curveinfo->points.e[c].x) break;
		}

		*index=c;
		*p_ret=lp;
		return 1;
	}

	return 0;
}

int CurveMapInterface::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	double xx=x, yy=y;
	if (style & RealSpaceMouse) {
		flatpoint p = dp->screentoreal(x,y);
		xx = p.x;
		yy = p.y;
	}

	int pp = scan(xx,yy);
	DBG cerr <<"scan: "<<pp<<endl;

	draglimbo = -1;
	if (pp >= 0) {

		 //found existing
		buttondown.down(d->id, LEFTBUTTON, x,y, pp);
		return 0;
	}

	flatpoint p;
	if (scannear(xx,yy, &p, &pp)) {
		curveinfo->points.push(p,pp);
		buttondown.down(d->id, LEFTBUTTON, x,y, pp);
		needtodraw=1;
		return 0;
	}
	
	int e = scaneditable(xx,yy);
	if (e) {
		buttondown.down(d->id, LEFTBUTTON, x,y, -e);
		needtodraw=1;
		return 0;
	}

	return 1;
}

int CurveMapInterface::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	int pp=-5;
	int dragged=buttondown.up(d->id, LEFTBUTTON, &pp);
	
	if (pp<0 && pp!=-5) {
		 //was on editable thing
		if (!dragged) {
//			LineEdit *le=new LineEdit(this, "edit","edit",LINEEDIT_GRAB_ON_MAP|LINEEDIT_SEND_FOCUS_OFF, 0,0,0,0,0,
//										NULL,object_id,"edit", scratch);
		}
	}

	return 0;
}

/*! If allowed with MB_MOVE, this shifts around text that is smaller
 * than the window only within the window boundaries, and shifts 
 * around larger text only just enough so that you can see it.
 */
int CurveMapInterface::MouseMove(int x,int y,unsigned int state,const LaxMouse *m)
{
	double xx=x, yy=y;
	flatpoint rp;
	if (style & RealSpaceMouse) {
		rp = dp->screentoreal(x,y);
		xx = rp.x;
		yy = rp.y;
	}
	DBG cerr <<"x,y: "<<x<<','<<y<<"  rect: "<<rect.x<<','<<rect.y<<" "<<rect.width<<"x"<<rect.height<<endl;

	hoverpoint = flatpoint(xx,yy);
	DBG if (rect.pointIsIn(xx,yy)) needtodraw=1;

	//DBG flatpoint fpp;
	//DBG int ii=-1;
	//DBG scannear(xx,yy, &fpp,&ii);
	//DBG cerr <<"scannear: "<<ii<<"  fp:"<<fpp.x<<','<<fpp.y<<endl;

	if (!buttondown.any(m->id,LEFTBUTTON)) {
		int e = scaneditable(xx,yy);
		if (e) {
			highlighteditable=e;		
			needtodraw=1;
			return 0;
		}
		if (highlighteditable) {
			highlighteditable=0;
			needtodraw=1;
			return 0;
		}

		int hover = scan(xx,yy);
		if (hover != lasthover) { lasthover=hover; needtodraw=1; }
		DBG cerr <<"curvemap scan: "<<lasthover<<endl;

		if (hover != 0) return 1;

		hover = scanExpandable(xx,yy);
		if (hover != lasthover) { lasthover=hover; needtodraw=1; }

		return 1;
	}


	int oxi,oyi;
	buttondown.move(m->id, x,y, &oxi, &oyi);
	double ox=oxi, oy=oyi;
	if (style & RealSpaceMouse) {
		rp = dp->screentoreal(oxi,oyi);
		ox = rp.x;
		oy = rp.y;
	}

	int pp = -5;
	buttondown.getextrainfo(m->id,LEFTBUTTON, &pp);

	if (pp<0 && pp!=-5) {
		pp = -pp;
		//double dx=xx-ox;
		double dy=yy-oy;
		//YMax  =(1<<0),
		//YMin  =(1<<1),
		//XMax  =(1<<2),
		//XMin  =(1<<3),
		//YUnits=(1<<4),
		//XUnits=(1<<5)

		if (pp==YMax) {
			double nymax=curveinfo->ymax+ dy*(curveinfo->ymax-curveinfo->ymin)/bounds.height;
			curveinfo->SetYBounds(curveinfo->ymin,nymax, NULL,true);
			needtodraw=1;
		}
		return 0;
	}


	if (pp<0) return 0;

	if (draglimbo<0 && (xx<rect.x || xx>rect.x+rect.width)) {
		if (pp>0 && pp<curveinfo->points.n-1) {
			 //remove point if drag outside
			draglimbo = pp;
			curveinfo->points.pop(pp);
			curveinfo->MakeFakeCurve();
			needtodraw=1;
			if (always_refresh_lookup) curveinfo->RefreshLookup();

			send();
			return 0;
		}

	}
	if (draglimbo>=0 && rect.pointIsIn(xx,yy)) {
		 //add point if drag to inside
		flatpoint p(((double)xx-rect.x)/rect.width,((double)rect.y+rect.height-yy)/rect.height);
		curveinfo->points.push(p,pp);
		p = ClampPoint(p,draglimbo);
		curveinfo->points.e[pp]=p;
		

		curveinfo->MakeFakeCurve();
		draglimbo=-1;
		needtodraw=1;
		if (always_refresh_lookup) curveinfo->RefreshLookup();

		send();
		return 0;
	}
	
	if (draglimbo>=0) return 0;

	flatpoint d=flatpoint(xx,yy) - flatpoint(ox,oy);
	d.x /= rect.width;
	d.y /= -rect.height;
	
	curveinfo->points.e[pp] += d;

	 //clamp to y boundaries
	if (curveinfo->points.e[pp].y<0) curveinfo->points.e[pp].y=0;
	if (curveinfo->points.e[pp].y>1) curveinfo->points.e[pp].y=1;

	 //clamp to x boundaries of current segment
	if (pp == 0) {
		if (curveinfo->points.e[pp].x<0) curveinfo->points.e[pp].x=0;
		else if (curveinfo->points.n>1 && curveinfo->points.e[pp].x>curveinfo->points.e[pp+1].x) 
			curveinfo->points.e[pp].x=curveinfo->points.e[pp+1].x;

	} else if (pp==curveinfo->points.n-1) {
		if (curveinfo->points.e[pp].x>1) curveinfo->points.e[pp].x=1;
		else if (curveinfo->points.n>1 && curveinfo->points.e[pp].x<curveinfo->points.e[pp-1].x) 
			curveinfo->points.e[pp].x=curveinfo->points.e[pp-1].x;

	} else {
		if (curveinfo->points.e[pp].x<curveinfo->points.e[pp-1].x) 
			curveinfo->points.e[pp].x=curveinfo->points.e[pp-1].x;
		else if (curveinfo->points.e[pp].x>curveinfo->points.e[pp+1].x) 
			curveinfo->points.e[pp].x=curveinfo->points.e[pp+1].x;
	}

	lastpoint=curveinfo->MapUnitPoint(curveinfo->points.e[pp]);
	curveinfo->MakeFakeCurve();
	if (always_refresh_lookup) curveinfo->RefreshLookup();

	needtodraw=1;
	send();
	return 0;
}

flatpoint CurveMapInterface::ClampPoint(flatpoint p, int pp)
{
	 //clamp to y boundaries
	if (p.y<0) p.y=0;
	if (p.y>1) p.y=1;

	 //clamp to x boundaries of current segment
	if (pp==0) {
		p.x=0;
	} else if (pp==curveinfo->points.n-1) {
		p.x=1;
	} else {
		if (p.x<curveinfo->points.e[pp-1].x) 
			p.x=curveinfo->points.e[pp-1].x;
		else if (p.x>curveinfo->points.e[pp+1].x) 
			p.x=curveinfo->points.e[pp+1].x;
	}

	return p;
}

int CurveMapInterface::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	// *** implement changing point types?
	needtodraw=1;
	return 0;
}

int CurveMapInterface::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	// *** implement changing point types?
	needtodraw=1;
	return 0;
}

Attribute *CurveMapInterface::dump_out_atts(Attribute *att,int what,LaxFiles::DumpContext *context)
{ //***
	//if (!att) att=new Attribute(whattype(),NULL);
	//anInterface::dump_out_atts(att,what,context);
	//return att;
	return NULL;
}

void CurveMapInterface::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
{ //***
	//anInterface::dump_in_atts(att,flag,context);
}

int CurveMapInterface::MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue)
{
	return curveinfo->MakeLookupTable(table,numentries,minvalue,maxvalue);
}

int CurveMapInterface::AddPoint(double x,double y)
{
	curveinfo->AddPoint(x,y);
	needtodraw=1;
	return 0;
}

int CurveMapInterface::MovePoint(int index, double x,double y)
{
	curveinfo->MovePoint(index,x,y);
	needtodraw=1;
	return 0;
}

int CurveMapInterface::MoveResize(int nx,int ny,int nw,int nh)
{
	bounds.x=nx;
	bounds.y=ny;
	bounds.width=nw;
	bounds.height=nh;
	SetupRect(0,0,-1,-1);
	return 0;
}

/*! Resets to 2 point linear, 0 to 1 (unmapped).
 * Does NOT change bounds.
 */
void CurveMapInterface::Reset()
{
	curveinfo->Reset(true);
	needtodraw=1;
}

/*! Catches escape if (style&Escapable) to remove.
 */
int CurveMapInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{
	if (ch==LAX_Esc && (style&Escapable)) {
		if (owner) owner->RemoveChild();
		else if (viewport) viewport->Pop(this,1);
	}

     //check shortcuts
    if (!sc) GetShortcuts();
    int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
    if (action>=0) {
        return PerformAction(action);
    }

	return 1;
}

int CurveMapInterface::PerformAction(int action)
{
	if (action==CURVEM_Toggle_Wrap) {
		curveinfo->Wrap(!curveinfo->wrap);
		needtodraw=1;
		return 0;

	} else if (action==CURVEM_Invert) {
		curveinfo->InvertY();
		needtodraw=1;
		return 0;

	} else if (action==CURVEM_Reset) {
		curveinfo->Reset(false);
		needtodraw=1;
		return 0;
	}

	return 1;
}

Laxkit::ShortcutHandler *CurveMapInterface::GetShortcuts()
{
   if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc=new ShortcutHandler(whattype());

    sc->Add(CURVEM_Toggle_Wrap,      'w',0,0,        "ToggleWrap",  _("Toggle wrapping"),NULL,0);
    sc->Add(CURVEM_Invert,           'i',0,0,        "Invert",      _("Invert y values"),NULL,0);
    sc->Add(CURVEM_Reset,            'z',0,0,        "Reset",      _("Reset to 1:1"),NULL,0);
    //sc->Add(CURVEM_ToggleBrushRamp, 'b',0,0,        "ToggleBrushRamp", _("Toggle brush ramp edit mode"),NULL,0);


    manager->AddArea(whattype(),sc);
    return sc;
}


} // namespace LaxInterfaces

