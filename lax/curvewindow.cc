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
//    Copyright (C) 2013-2014 by Tom Lechner
//

#include <lax/curvewindow.h>
#include <lax/laxutils.h>
#include <lax/strmanip.h>
#include <lax/bezutils.h>

#include <iostream>
using namespace std;
#define DBG 


using namespace LaxFiles;


namespace Laxkit {

//-------------------------------- CurveWindow ----------------------------------
/*! \class CurveWindow
 * Window to edit curves that map one range, one-to-one, to another range.
 */

CurveWindow::CurveWindow(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder, 
						anXWindow *prev, unsigned long nowner, const char *nsend,
						const char *nctitle,
						const char *xl, double nxmin, double nxmax,
						const char *yl, double nymin, double nymax)
				: anXWindow(pwindow,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER, nx,ny, nw,nh,brder, prev,nowner,nsend) 
{
	firsttime=1;

	padouter=padinner=5;

	smallnumbers=app->fontmanager->MakeFont("Sans","",app->defaultlaxfont->textheight()/2,0);
	DBG smallnumbers->suppress_debug=1;

	editable=0;
	always_refresh_lookup=1;
	highlighteditable=0;

	show_label_ranges=1;
	show_labels=1;

	win_colors=app->color_panel;
	win_colors->inc_count();
	curve_color=win_colors->fg;
	graph_color=app->color_edits->bg;

	curveinfo=new CurveInfo(nctitle, xl,nxmin,nxmax, yl,nymin,nymax);

	histogram=NULL;
	hist_n=0;
}

CurveWindow::~CurveWindow()
{
	if (smallnumbers) smallnumbers->dec_count();
	if (curveinfo) curveinfo->dec_count();

	if (histogram) delete[] histogram;
}

//! Set values in rect (the rectangle that the actual curve is drawn in) to be reasonable.
void CurveWindow::SetupRect()
{
	rect.x=rect.y=padouter;
	rect.width=win_w-2*padouter;
	rect.height=win_h-2*padouter;


	Displayer *dp=GetDefaultDisplayer();
	int textheight=dp->textheight();

	if (show_labels) {
		if (curveinfo->ylabel) {
			 //write on top
			rect.y+=textheight+padinner;
			rect.height-=textheight+padinner;
		}

	}

	if (show_labels || show_label_ranges) {
		if (curveinfo->xlabel) {
			rect.height-=padinner+smallnumbers->textheight(); //remove number height from bottom
		}
	}

	if (show_label_ranges) {
		char scratch[100];
		int extent=0, tmp=0;

		if (!show_labels) rect.height-=padinner+smallnumbers->textheight(); //remove number height

		sprintf(scratch, "%.5g",curveinfo->ymin);
		extent=dp->textextent(smallnumbers, scratch,-1, NULL,NULL,NULL,NULL,0);

		sprintf(scratch, "%.5g",curveinfo->ymax);
		tmp=dp->textextent(smallnumbers, scratch,-1, NULL,NULL,NULL,NULL,0);
		if (tmp>extent) extent=tmp;

		extent+=padinner;
		rect.x+=extent;
		rect.width-=extent;
	}
}

/*! Sends a SimpleMessage.
 * which==0 means curve changed, which>1 means a CurveWindowEditable changed.
 * which is put in event->info1.
 */
void CurveWindow::send(int which)
{
	if (win_owner) {
		SimpleMessage *ev=new SimpleMessage;
		ev->info1=which;
		app->SendMessage(ev,win_owner,win_sendthis,object_id);
	}
}

double CurveWindow::f(double x)
{
	return curveinfo->f_linear(x);
}

int CurveWindow::Event(const EventData *e,const char *mes)
{
    if (e->type==LAX_onMouseOut && highlighteditable) {
		highlighteditable=0;
		needtodraw=1;
	}

	return anXWindow::Event(e,mes);
}

/*! Adds reference to info.
 */
int CurveWindow::SetInfo(CurveInfo *info)
{
	if (!info) return 1;
	if (curveinfo) curveinfo->dec_count();
	curveinfo=info;
	info->inc_count();
	return 0;
}

/*! This basically does *curveinfo=*info, making sure curveinfo!=NULL first.
 */
int CurveWindow::CopyInfo(CurveInfo *info)
{
	if (!info) return 1;
	if (!curveinfo) curveinfo=new CurveInfo();
	(*curveinfo)=(*info);
	return 0;
}

void CurveWindow::Refresh()
{
	if (!win_on || !needtodraw) return;
	
	Displayer *dp=GetDefaultDisplayer();
	dp->StartDrawing(this);
	dp->ClearWindow();
	//dp->MakeCurrent(this);

	if (firsttime) {
		SetupRect();
		firsttime=0;
	}


	//--------------
	dp->NewBG(win_colors->bg);
	dp->NewFG(win_colors->fg);
	dp->ClearWindow();

	 //blank out graph area
	dp->NewFG(graph_color);
	dp->drawrectangle(rect.x,rect.y,rect.width,rect.height, 1);
	if (histogram) {
		dp->NewFG(coloravg(graph_color,win_colors->fg,.9));
		int y;
		for (int x=rect.x; x<rect.x+rect.width; x++) {
			y=(rect.height-histogram[int(double(x-rect.x)/rect.width * hist_n)]/1000.*rect.height) + rect.y;
			dp->drawline(x,rect.y+rect.width, x,y);
		}
	}


	dp->NewFG(win_colors->fg);

	 //fill highlight rectangle for any hovers
	if (highlighteditable) {
		dp->NewFG(coloravg(win_colors->bg,win_colors->fg,.1));

		if (highlighteditable==YUnits)
			dp->drawrectangle(0,0,rect.x,rect.y,1);
		else if (highlighteditable==XUnits)
			dp->drawrectangle(rect.x+rect.width*.25,rect.y+rect.height, rect.width*.5,win_h-(rect.y+rect.height),1);
		else if (highlighteditable==XMax)
			dp->drawrectangle(rect.x+rect.width*.75,rect.y+rect.height, rect.width*.25,win_h-(rect.y+rect.height),1);
		else if (highlighteditable==XMin)
			dp->drawrectangle(rect.x,rect.y+rect.height, rect.width*.25,win_h-(rect.y+rect.height),1);
		else if (highlighteditable==YMax)
			dp->drawrectangle(0,rect.y,rect.x,rect.height*.3,1);
		else if (highlighteditable==YMin)
			dp->drawrectangle(0,rect.y+rect.height*.7,rect.x,rect.height*.3,1);

		dp->NewFG(win_colors->fg);
	}

	 //draw various labels
	if (show_labels) {
		if (curveinfo->title) dp->textout(win_w/2, padouter, curveinfo->title,-1, LAX_HCENTER|LAX_TOP);

		dp->font(smallnumbers);
		if (curveinfo->ylabel) dp->textout(padouter, rect.y-padinner, curveinfo->ylabel,-1, LAX_LEFT|LAX_BOTTOM);
		if (curveinfo->xlabel) dp->textout(rect.x+rect.width/2,rect.y+rect.height+padinner, curveinfo->xlabel,-1, LAX_HCENTER|LAX_TOP);
		dp->font(anXApp::app->defaultlaxfont,-1);
	}

	if (show_label_ranges) {
		dp->font(smallnumbers);

		char scratch[100];
		int e;
		int x;

		if (buttondown.any()) {
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
		dp->drawpoint(p1, 3,0);
	}



	SwapBuffers();
	needtodraw=0;
	return;
} //Refresh()

//! Make any CurveWindowEditable masked in editable editable (on==1) or not (on==0).
void CurveWindow::ChangeEditable(unsigned int which, int on)
{
	if (on) editable|=which;
	else editable&=~which;
}

int CurveWindow::scaneditable(int x,int y)
{
	int found=0;
	if (x<rect.x) {
		if (y<rect.y) found=YUnits;
		else if (y<rect.y+rect.height/3) found=YMax;
		else if (y>rect.y+rect.height*2/3) found=YMin;

	} else if (y>rect.y+rect.height) {
		if (x<rect.x+rect.width*.25) found=XMin;
		else if (x<rect.x+rect.width*.75) found=XUnits;
		else if (x>=rect.x+rect.width*.75) found=XMax;
	}
	found=(found&editable);
	return found;
}

//! Scan for existing point, return index in curveinfo->points.
int CurveWindow::scan(int x,int y)
{
	double scandistance=5;
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

int CurveWindow::scannear(int x,int y, flatpoint *p_ret, int *index)
{

	flatpoint p(((double)x-rect.x)/rect.width, ((double)rect.y+rect.height-y)/rect.height);
	flatpoint lp;
	flatpoint v,p1,p2;

	double scandistance=10./rect.height;

	for (int c=0; c<curveinfo->points.n-1; c++) {
		p1=curveinfo->points.e[c]; //is point in range 0..1
		p2=curveinfo->points.e[c+1]; //is point in range 0..1
		if (p.x>=p2.x) continue;
		if (p.x<p1.x) continue;

		v=p2-p1;
		if (v.x==0) continue; //don't allow dividing vertical lines

		lp=p1+v*((p.x-p1.x)/v.x);

		if (fabs(lp.y-p.y)<scandistance) {
			*p_ret=lp;
			*index=c+1;
			return 1;
		}
	}
	return 0;
}

int CurveWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	int pp=scan(x,y);
	DBG cerr <<"scan: "<<pp<<endl;

	draglimbo=-1;
	if (pp>=0) {

		 //found existing
		buttondown.down(d->id, LEFTBUTTON, x,y, pp);
		return 0;
	}

	flatpoint p;
	if (scannear(x,y, &p, &pp)) {
		curveinfo->points.push(p,pp);
		buttondown.down(d->id, LEFTBUTTON, x,y, pp);
		needtodraw=1;
		return 0;
	}
	
	int e=scaneditable(x,y);
	if (e) {
		buttondown.down(d->id, LEFTBUTTON, x,y, -e);
		needtodraw=1;
		return 0;
	}

	return 1;
}

int CurveWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
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
int CurveWindow::MouseMove(int x,int y,unsigned int state,const LaxMouse *m)
{
	//DBG int ppp=scan(x,y);
	//DBG cerr <<"scan: "<<ppp<<endl;
	//DBG flatpoint fpp;
	//DBG int ii=-1;
	//DBG scannear(x,y, &fpp,&ii);
	//DBG cerr <<"scannear: "<<ii<<"  fp:"<<fpp.x<<','<<fpp.y<<endl;

	if (!buttondown.any(m->id,LEFTBUTTON)) {
		int e=scaneditable(x,y);
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
		return 1;
	}

	int ox,oy;
	buttondown.move(m->id, x,y, &ox, &oy);
	
	int pp=-5;
	buttondown.getextrainfo(m->id,LEFTBUTTON, &pp);

	if (pp<0 && pp!=-5) {
		pp=-pp;
		//double dx=x-ox;
		double dy=y-oy;
		//YMax  =(1<<0),
		//YMin  =(1<<1),
		//XMax  =(1<<2),
		//XMin  =(1<<3),
		//YUnits=(1<<4),
		//XUnits=(1<<5)

		if (pp==YMax) {
			double nymax=curveinfo->ymax+ dy*(curveinfo->ymax-curveinfo->ymin)/win_h;
			curveinfo->SetYBounds(curveinfo->ymin,nymax);
			needtodraw=1;
		}
		return 0;
	}


	if (pp<0) return 0;

	if (draglimbo<0 && (x<rect.x || x>rect.x+rect.width)) {
		if (pp>0 && pp<curveinfo->points.n-1) {
			 //remove point if drag outside
			draglimbo=pp;
			curveinfo->points.pop(pp);
			curveinfo->MakeFakeCurve();
			needtodraw=1;
			if (always_refresh_lookup) curveinfo->RefreshLookup();

			send();
			return 0;
		}

	}
	if (draglimbo>=0 && rect.pointIsIn(x,y)) {
		 //add point if drag to inside
		flatpoint p(((double)x-rect.x)/rect.width,((double)rect.y+rect.height-y)/rect.height);
		curveinfo->points.push(p,pp);
		p=ClampPoint(p,draglimbo);
		curveinfo->points.e[pp]=p;
		

		curveinfo->MakeFakeCurve();
		draglimbo=-1;
		needtodraw=1;
		if (always_refresh_lookup) curveinfo->RefreshLookup();

		send();
		return 0;
	}
	
	if (draglimbo>=0) return 0;

	flatpoint d=flatpoint(x,y)-flatpoint(ox,oy);
	d.x/=rect.width;
	d.y/=-rect.height;
	
	curveinfo->points.e[pp]+=d;

	 //clamp to y boundaries
	if (curveinfo->points.e[pp].y<0) curveinfo->points.e[pp].y=0;
	if (curveinfo->points.e[pp].y>1) curveinfo->points.e[pp].y=1;

	 //clamp to x boundaries of current segment
	if (pp==0) {
		curveinfo->points.e[pp].x=0;
	} else if (pp==curveinfo->points.n-1) {
		curveinfo->points.e[pp].x=1;
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

flatpoint CurveWindow::ClampPoint(flatpoint p, int pp)
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

int CurveWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	// *** implement changing point types?
	needtodraw=1;
	return 0;
}

int CurveWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	// *** implement changing point types?
	needtodraw=1;
	return 0;
}

/*! Append to att if att!=NULL, else return a new Attribute whose name is whattype().
 *
 * Default is to add attributes for "text", and whatever anXWindow adds.
 */
Attribute *CurveWindow::dump_out_atts(Attribute *att,int what,anObject *context)
{ //***
	if (!att) att=new Attribute(whattype(),NULL);
	anXWindow::dump_out_atts(att,what,context);
	return att;
	
//	if (what==-1) {
//		att->push("text","string");
//		att->push("halign","one of: left center right");
//		att->push("valign","one of: left center right");
//		return att;
//	}
//
//	char *txt=GetText();
//	att->push("text",txt);
//	if (txt) delete[] txt;
//
//	char align[20];
//	if (win_style&MB_LEFT) strcpy(align,"left");
//	else if (win_style&MB_CENTERX) strcpy(align,"center");
//	else if (win_style&MB_RIGHT) strcpy(align,"right");
//	att->push("halign",align);
//
//	if (win_style&MB_TOP) strcpy(align,"top");
//	else if (win_style&MB_CENTERY) strcpy(align,"center");
//	else if (win_style&MB_BOTTOM) strcpy(align,"bottom");
//	att->push("valign",align);
//
//	return att;
}

/*! Default is to read in text, and whatever anXWindow reads.
 */
void CurveWindow::dump_in_atts(LaxFiles::Attribute *att,int flag,anObject *context)
{ //***
	anXWindow::dump_in_atts(att,flag,context);

//	char *name,*value;
//	for (int c=0; c<att->attributes.n; c++) {
//		name= att->attributes.e[c]->name;
//		value=att->attributes.e[c]->value;
//
//		if (!strcmp(name,"text")) {
//			SetText(value);
//
//		} if (!strcmp(name,"halign")) {
//			win_style&=~(MB_LEFT|MB_CENTERX|MB_RIGHT);
//			if (!strcmp(value,"left")) win_style|=MB_LEFT;
//			else if (!strcmp(value,"center")) win_style|=MB_CENTERX;
//			else if (!strcmp(value,"right")) win_style|=MB_RIGHT;
//			if ((win_style&(MB_LEFT|MB_CENTERX|MB_RIGHT))==0) win_style|=MB_CENTERX;
//
//		} if (!strcmp(name,"valign")) {
//			win_style&=~(MB_TOP|MB_CENTERY|MB_BOTTOM);
//			if (!strcmp(value,"left")) win_style|=MB_TOP;
//			else if (!strcmp(value,"center")) win_style|=MB_CENTERY;
//			else if (!strcmp(value,"right")) win_style|=MB_BOTTOM;
//			if ((win_style&(MB_TOP|MB_CENTERY|MB_BOTTOM))==0) win_style|=MB_CENTERY;
//		}
//	}
}

int CurveWindow::MakeLookupTable(int *table,int numentries, int minvalue, int maxvalue)
{
	return curveinfo->MakeLookupTable(table,numentries,minvalue,maxvalue);
}

int CurveWindow::AddPoint(double x,double y)
{
	curveinfo->AddPoint(x,y);
	needtodraw=1;
	return 0;
}

int CurveWindow::MovePoint(int index, double x,double y)
{
	curveinfo->MovePoint(index,x,y);
	needtodraw=1;
	return 0;
}

int CurveWindow::MoveResize(int nx,int ny,int nw,int nh)
{
	//*** should have an extra track here
	anXWindow::MoveResize(nx,ny,nw,nh);
	SetupRect();
	return 0;
}

int CurveWindow::Resize(int nw,int nh)
{
	//*** should have an extra track here
	anXWindow::Resize(nw,nh);
	SetupRect();
	return 0;
}

/*! Resets to 2 point linear, 0 to 1 (unmapped).
 * Does NOT change bounds.
 */
void CurveWindow::Reset()
{
	curveinfo->Reset();
	needtodraw=1;
}


} // namespace Laxkit

