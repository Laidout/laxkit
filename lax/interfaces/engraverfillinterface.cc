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
//    Copyright (C) 2014,2015 by Tom Lechner
//



#include <lax/interfaces/engraverfillinterface.h>

#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/gradientinterface.h>
#include <lax/imagedialog.h>
#include <lax/transformmath.h>
#include <lax/bezutils.h>
#include <lax/iconmanager.h>
#include <lax/laxutils.h>
#include <lax/bitmaputils.h>
#include <lax/colorsliders.h>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <lax/fileutils.h>
#include <lax/filedialog.h>
#include <lax/popupmenu.h>
#include <lax/interfaces/freehandinterface.h>
#include <lax/interfaces/curvemapinterface.h>
#include <lax/interfaces/somedataref.h>


#include <lax/lists.cc>
#include <lax/refptrstack.cc>


using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {




//------------------------------ EngraverInterfaceSettings -------------------------------

/*! \class EngraverInterfaceSettings
 *
 * Settings meant to be shared across instances of the EngraverFillInterface
 */


//EngraverInterfaceSettings::EngraverInterfaceSettings()
//{ 
//	sensitive_thickness=1;
//	sensitive_turbulence=1;
//	sensitive_drag=1;
//	sensitive_pushpull=1;
//	sensitive_avoidtoward=1;
//	sensitive_twirl=1;
//}



////------------------------------ EngraverFillInterface -------------------------------


/*! \class EngraverFillInterface
 * \ingroup interfaces
 * \brief Interface for dealing with EngraverFillData objects.
 *
 * \todo *** select multiple datas to adjust. Mesh tinker only on one of them, touch up on many
 */

enum EngraveShortcuts {
	ENGRAVE_SwitchMode=PATCHA_MAX,
	ENGRAVE_SwitchModeR,
	ENGRAVE_ExportSvg,
	ENGRAVE_ExportSnapshot,
	ENGRAVE_RotateDir,
	ENGRAVE_RotateDirR,
	ENGRAVE_SpacingInc,
	ENGRAVE_SpacingDec,
	ENGRAVE_ResolutionInc,
	ENGRAVE_ResolutionDec,
	ENGRAVE_ShowPoints,
	ENGRAVE_ShowPointsN,
	ENGRAVE_MorePoints,
	ENGRAVE_ToggleShowTrace,
	ENGRAVE_TogglePanel,
	ENGRAVE_ToggleGrow,
	ENGRAVE_ToggleWarp,
	ENGRAVE_ToggleDir,
	ENGRAVE_LoadDirection,
	ENGRAVE_NextFill,
	ENGRAVE_PreviousFill,
	ENGRAVE_NextGroup,
	ENGRAVE_PreviousGroup,
	ENGRAVE_ToggleTracePanel, 
	ENGRAVE_ToggleDashPanel,  
	ENGRAVE_ToggleDirPanel,   
	ENGRAVE_ToggleSpacingPanel,
	ENGRAVE_ToggleLinked,
	ENGRAVE_ToggleGroupLinked,
	ENGRAVE_ToggleCurLinked,

	ENGRAVE_MAX
};

EngraverFillInterface::EngraverFillInterface(int nid,Displayer *ndp)
  : PatchInterface(nid,ndp),
	curvemapi(0,ndp)
{
	primary=1;

	showdecs=SHOW_Points|SHOW_Edges;
	drawrendermode=rendermode=3;
	recurse=0;
	edata=NULL;
	tracebox=NULL;


	current_group=0;
	default_spacing=1./20;
	turbulence_size=1;
	turbulence_per_line=false;
	sensitive_thickness=1;
	sensitive_turbulence=1;
	sensitive_drag=1;
	sensitive_pushpull=1;
	sensitive_avoidtoward=1;
	sensitive_twirl=1;


	show_points=0;
	submode=0;
	mode=controlmode=EMODE_Mesh;

	directionmap=NULL;

	curvemapi.owner=this;
	curvemapi.ChangeEditable(CurveMapInterface::YMax, 1);
	brush_radius=40;

	makestr(thickness.title,_("Brush Ramp"));
	thickness.SetSinusoidal(6);
	thickness.RefreshLookup();

	//DBG CurveWindow *ww=new CurveWindow(NULL,"curve","curve",0,0,0,400,400,0,NULL,0,NULL);
	//DBG ww->SetInfo(&thickness);
	//DBG app->addwindow(ww);

	whichcontrols=Patch_Coons;

	show_panel=true;
	show_trace_object=false;
	grow_lines=false;
	always_warp=true;
	auto_reline=true;
	show_direction=false;

	eventobject=0;
	eventgroup=-1;

	lasthoverindex=lasthoverdetail=-1;
	lasthovercategory=ENGRAVE_None;
	lasthover=ENGRAVE_None;

	IconManager *iconmanager=InterfaceManager::GetDefault(true)->GetIconManager();
	modes.AddItem(_("Mesh mode"),       iconmanager->GetIcon("EngraverMesh"),        EMODE_Mesh         );
	modes.AddItem(_("Thickness"),       iconmanager->GetIcon("EngraverThickness"),   EMODE_Thickness    );
	modes.AddItem(_("Blockout"),        iconmanager->GetIcon("EngraverKnockout"),    EMODE_Blockout     );
	modes.AddItem(_("Drag mode"),       iconmanager->GetIcon("EngraverDrag"),        EMODE_Drag         );
	modes.AddItem(_("Push/pull"),       iconmanager->GetIcon("EngraverPushPull"),    EMODE_PushPull     );
	modes.AddItem(_("Avoid"),           iconmanager->GetIcon("EngraverAvoid"),       EMODE_AvoidToward  );
	modes.AddItem(_("Twirl"),           iconmanager->GetIcon("EngraverTwirl"),       EMODE_Twirl        );
	modes.AddItem(_("Turbulence"),      iconmanager->GetIcon("EngraverTurbulence"),  EMODE_Turbulence   );
	//modes.AddItem(_("Resolution"),      iconmanager->GetIcon("EngraverResolution"), EMODE_Resolution   );
	modes.AddItem(_("Orientation"),     iconmanager->GetIcon("EngraverOrientation"), EMODE_Orientation  );
	modes.AddItem(_("Freehand"),        iconmanager->GetIcon("EngraverFreehand"),    EMODE_Freehand     );
	modes.AddItem(_("Trace adjustment"),iconmanager->GetIcon("EngraverTrace"),       EMODE_Trace        );
	//modes.AddItem(_("Direction"),       iconmanager->GetIcon("EngraverDirection"), EMODE_Direction    );

	fgcolor.rgbf(0.,0.,0.);
	bgcolor.rgbf(1.,1.,1.);
	activate_color  =app->color_panel->activate;
	deactivate_color=app->color_panel->deactivate;
}

//! Empty destructor.
EngraverFillInterface::~EngraverFillInterface() 
{
	DBG cerr<<"-------"<<whattype()<<","<<" destructor"<<endl;
}

const char *EngraverFillInterface::Name()
{ return _("Engraver Fill Tool"); }

anInterface *EngraverFillInterface::duplicate(anInterface *dup)//dup=NULL;
{
	EngraverFillInterface *dupp;
	if (dup==NULL) dupp=new EngraverFillInterface(id,NULL);
	else dupp=dynamic_cast<EngraverFillInterface *>(dup);
	if (!dupp) return NULL;

	dupp->recurse=recurse;
	dupp->rendermode=rendermode;
	return PatchInterface::duplicate(dupp);
}

//! Return new local EngraverFillData
PatchData *EngraverFillInterface::newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	EngraverFillData *ndata=NULL;
//	if (somedatafactory) {
//		ndata=dynamic_cast<EngraverFillData *>(somedatafactory->newObject(LAX_ENGRAVERFILLDATA));
//	} 
	ndata=dynamic_cast<EngraverFillData *>(somedatafactory()->NewObject("EngraverFillData"));
	if (!ndata) ndata=new EngraverFillData();//creates 1 count

	ndata->MakeGroupNameUnique(0);
	ndata->groups.e[0]->InstallTraceSettings(default_trace.duplicate(),1);
	//if (!ndata->groups.e[0]->trace)  ndata->groups.e[0]->trace =new EngraverTraceSettings;
	if (!ndata->groups.e[0]->dashes) {
		ndata->groups.e[0]->dashes=new EngraverLineQuality;
		ndata->groups.e[0]->dashes->SetResourceOwner(ndata->groups.e[0]);
	}

	ndata->Set(xx,yy,ww,hh,nr,nc,stle);
	//ndata->renderdepth=-recurse;
	//ndata->xaxis(3*ndata->xaxis());
	//ndata->yaxis(3*ndata->yaxis());
	//ndata->origin(flatpoint(xx,yy));
	//ndata->xaxis(flatpoint(1,0)/Getmag()*100);
	//ndata->yaxis(flatpoint(0,1)/Getmag()*100);

	ndata->DefaultSpacing((ndata->maxy-ndata->miny)/20);
	ndata->style|=PATCH_SMOOTH;
	ndata->groups.e[0]->Fill(ndata, 1./dp->Getmag());
	ndata->Sync(false);
	ndata->FindBBox();

	return ndata;
}

//! id==4 means make recurse=ndata.
int EngraverFillInterface::UseThis(int id,int ndata)
{
	if (id!=4) return PatchInterface::UseThis(id,ndata);
	char blah[100];
	if (id==4) { // recurse depth
		if (ndata>0) {
			recurse=ndata;
			sprintf(blah,_("New Recurse Depth %d: "),recurse);
			PostMessage(blah);
			if (rendermode>0) needtodraw=1; 
		}
		return 1;
	}
	return 0;
}

int EngraverFillInterface::UseThisObject(ObjectContext *oc)
{
	int status=PatchInterface::UseThisObject(oc);

	EngraverFillData *olddata=edata;
	edata=dynamic_cast<EngraverFillData *>(data);
	if (edata!=olddata) UpdatePanelAreas();

	return status;
}

/*! Accepts EngraverFillData, or LineStyle for color.
 *
 * Returns 1 to used it, 0 didn't
 *
 */
int EngraverFillInterface::UseThis(Laxkit::anObject *nobj,unsigned int mask) // assumes not use local
{
    if (!nobj) return 0;

	if (dynamic_cast<EngraverFillData *>(nobj)) { 
		int status=PatchInterface::UseThis(nobj,mask);
		edata=dynamic_cast<EngraverFillData *>(data);
		return status;

	} else if (dynamic_cast<LineStyle *>(nobj) && edata) {
		if (!edata) return 1;
        LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);

		if (current_group>=0) {
			EngraverPointGroup *group=edata->groups.e[current_group];
			group->color=nlinestyle->color;
			needtodraw=1;
		}
        return 1;
	}


	return 0;
}

void EngraverFillInterface::deletedata(bool flush_selection)
{
	PatchInterface::deletedata(flush_selection);
	edata=NULL;
}


//! Flush curpoints.
int EngraverFillInterface::InterfaceOff()
{
	PatchInterface::InterfaceOff();
	curvemapi.SetInfo(NULL);
    return 0;
}

int EngraverFillInterface::InitializeResources()
{
	InterfaceManager *imanager=InterfaceManager::GetDefault(true);

	 //install across the board interface settings object if necessary
//	ResourceManager *tools=imanager->GetSettingsManager();
//	anObject *obj=tools->FindResource(whattype(),"settings");
//	if (!obj) {
//		tools->AddResource("settings", settings_object, NULL, whattype(), whattype(), NULL, NULL, NULL);
//	}


	InstallEngraverObjectTypes(imanager->GetObjectFactory());

	InstallDefaultLineProfiles(imanager->GetObjectFactory(), imanager->GetResourceManager());

	return 0;
}


/*! Returns the panel item (x,y) is over, or ENGRAVE_None.
 * category will get set with one of the main section tags of the panel.
 * That is ENGRAVE_Groups, ENGRAVE_Tracing, ENGRAVE_Dashes, ENGRAVE_Direction,
 * or ENGRAVE_Spacing.
 */
int EngraverFillInterface::scanPanel(int x,int y, int *category, int *index_ret, int *detail_ret)
{
	if (!panelbox.boxcontains(x,y)) {
		DBG cerr <<"not in panel"<<endl;
		*category=ENGRAVE_None;
		return ENGRAVE_None;
	}
	DBG cerr <<"in panel"<<endl;

	*category=ENGRAVE_Panel;
	x-=panelbox.minx;
	y-=panelbox.miny;

	MenuItem *item, *item2;
	for (int c=0; c<panel.n(); c++) {
		item=panel.e(c);
		if (item->pointIsIn(x,y)) {
			if (item->id==ENGRAVE_Mode_Selection) {
				int i=(x-item->x)*modes.n()/item->w;
				if (i>=0 && i<modes.n()) return modes.e(i)->id;
			}

			if (item->hasSub() && item->isOpen()) {
				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					if (item2->pointIsIn(x,y)) {
						if (item2->id==ENGRAVE_Group_List) {
							double th=dp->textheight();
							//int n=NumGroupLines();
							int index=(y-item2->y)/th;
							int detail=(x-item2->x)/th;
							if (detail>3) detail=3;
							if (index_ret) *index_ret=index;
							if (detail_ret) *detail_ret=detail;

						} else if (item2->id==ENGRAVE_Trace_Curve_Line_Bar || item2->id==ENGRAVE_Trace_Curve_Value_Bar) {
							return ENGRAVE_Trace_Curve;

						} else if (item2->id==ENGRAVE_Direction_Point_Offset) {
							if (y>item2->y + .66*item2->h)
								return ENGRAVE_Direction_Point_Off_Size;

						} else if (item2->id==ENGRAVE_Direction_Profile) {
							double start=0,end=1;
							EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
							if (group) {
								start=group->direction->profile_start;
								end  =group->direction->profile_end;
							}
							start*=item2->w;
							end  *=item2->w;
							if (y>item2->y+item2->h*3/4) {
								if (fabs(start-x)<fabs(end-x)) return ENGRAVE_Direction_Profile_Start_Random;
								return ENGRAVE_Direction_Profile_End_Random;
							}
							if (fabs(start-x)<fabs(end-x)) return ENGRAVE_Direction_Profile_Start;
							return ENGRAVE_Direction_Profile_End;
						}

						return item2->id;
					}
				}
			}

			 //in blank space of the section, or on title head
			return item->id;
		}
	}
	 //else on blank area of entire panel
	return ENGRAVE_Panel;

}

/*! Scan the panel or for other onscreen controls.
 */
int EngraverFillInterface::scanEngraving(int x,int y,unsigned int state, int *category, int *index_ret, int *detail_ret)
{
	*category=0;

	if (show_panel) {
	    if (panelbox.boxcontains(x,y)) {
			return scanPanel(x,y,category,index_ret,detail_ret);
		}
	}

	if (mode==EMODE_Trace) {
		flatpoint p;

		p=screentoreal(x,y); //p is in edata->parent space
		if ((state&LAX_STATE_MASK)==ShiftMask) {
			// *** need to scan for any in selection
			if (edata && edata->pointin(p)) return ENGRAVE_Trace_Move_Mesh;
		}
		
		p=dp->screentoreal(x,y);

		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

		if (trace->traceobject && trace->traceobject->object && trace->traceobject->object->pointin(p)) {
			return ENGRAVE_Trace_Object;
		}

		p=screentoreal(x,y); //p is in edata->parent space
		if (edata && edata->pointin(p)) return ENGRAVE_Trace_Move_Mesh;

	}

	if (mode==EMODE_Orientation && edata) {
		 //note: need to coordinate with DrawOrientation
		 //     0
		 //   > \----  |  =
		*category=ENGRAVE_Orient;

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		flatpoint p(x,y);
		flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
		center=realtoscreen(edata->transformPoint(center));

		int size=30;
		//double thick=.25;

		flatpoint dir=group->directionv/norm(group->directionv)*.05;
//		flatpoint xx= realtoscreen(edata->transformPoint(edata->getPoint(.5+dir.x,.5+dir.y)))
//					- realtoscreen(edata->transformPoint(edata->getPoint(.5,      .5      )));
		flatpoint xx= realtoscreen(edata->transformPoint(edata->getPoint(.5+dir.x,.5+dir.y, false)))
					- realtoscreen(edata->transformPoint(edata->getPoint(.5,      .5      , false)));
		xx=xx/norm(xx)*size;
		flatpoint yy=-transpose(xx);

		flatpoint pp;
		pp.x=(p-center)*xx/norm2(xx);
		pp.y=(p-center)*yy/norm2(yy);

		//DBG cerr <<"orient pp:"<<pp.x<<','<<pp.y<<endl;

		if (pp.y>.5 && pp.y<1.5 && pp.x>-.5 && pp.x<.5) return ENGRAVE_Orient_Spacing;
		if (pp.y<.5 && pp.y>-.3) {
			if (pp.x>2.5 && pp.x<3.5) return ENGRAVE_Orient_Quick_Adjust;
			if (pp.x>1.5 && pp.x<2.5) return ENGRAVE_Orient_Direction;
			if (pp.x<=1.5 && pp.x>-.5) return ENGRAVE_Orient_Position;
			//if (pp.x<=-.5 && pp.x>-1.5) return ENGRAVE_Orient_Keep_Old;
		}
		if (group && pp.x>-1 && pp.x<1 && pp.y<-.25) {
			int i=floor((-pp.y-.25)*2);
			//DBG cerr <<" orientation parameter search: pp.y="<<pp.y<<"  i="<<i<<endl;

			if (i>=0 && i<group->direction->NumParameters()) {
				*index_ret=i;
				return ENGRAVE_Orient_Parameter;
			}
		}

		return ENGRAVE_None;
	}

	if (    mode==EMODE_Thickness
		 || mode==EMODE_Turbulence
		 || mode==EMODE_Drag
		 || mode==EMODE_PushPull
		 || mode==EMODE_AvoidToward
		 || mode==EMODE_Twirl
		 ) {
		if (submode==2 && sensbox.pointIsIn(x,y)) return ENGRAVE_Sensitivity;
		return ENGRAVE_None;
	}

	return ENGRAVE_None;
}

/*! oc->obj must be castable to an EngraverFillData or a PathsData.
 */
int EngraverFillInterface::AddToSelection(ObjectContext *oc)
{
    if (!oc || !oc->obj) return -2;
    if (   !dynamic_cast<EngraverFillData*>(oc->obj)
		&& !dynamic_cast<PathsData*>(oc->obj)) return -3; //not a usable object!

	if (!selection) selection=new Selection();
    return selection->AddNoDup(oc,-1);
}

//! Catch a double click to pop up an ImageDialog.
int EngraverFillInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	lasthover= scanEngraving(x,y,state, &lasthovercategory, &lasthoverindex, &lasthoverdetail);

	 //catch trace box overlay first
	if (lasthovercategory==ENGRAVE_Panel && panelbox.boxcontains(x,y)) {

		if ((state&LAX_STATE_MASK)!=0) lasthover=ENGRAVE_Panel;

		if (lasthover==ENGRAVE_Trace_Curve) {
			double pad=2;
			curvemapi.Dp(dp);
			EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
			curvemapi.SetInfo(trace->value_to_weight);

			curvemapi.SetupRect(panelbox.minx+tracebox->x,panelbox.miny+tracebox->y, tracebox->w,tracebox->h-pad);

			child=&curvemapi;
			child->inc_count();
			if (curvemapi.LBDown(x,y,state,count,d)==1) {
				child->dec_count();
				child=NULL;
				lasthover=ENGRAVE_Trace_Box;
			}

			needtodraw=1;
			return 0;
		}

		buttondown.down(d->id,LEFTBUTTON,x,y,lasthover,lasthovercategory);
		controlmode=EMODE_Controls;
		needtodraw=1;

		if (lasthover==ENGRAVE_Trace_Opacity) {
			x-=panelbox.minx;
			y-=panelbox.miny;

			EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

			MenuItem *box=panel.findid(ENGRAVE_Trace_Opacity);
			trace->traceobj_opacity=(x-box->x)/((double)box->w);
			if (trace->traceobj_opacity<0) group->trace->traceobj_opacity=0;
			else if (trace->traceobj_opacity>1) group->trace->traceobj_opacity=1;

			DBG cerr << " *** need to implement actual trace object opacity"<<endl;
			needtodraw=1;
		}

		return 0;
	}

	if (child) return 1;

	if (mode==EMODE_Direction) {
		if (count==2 || !directionmap) {
			PerformAction(ENGRAVE_LoadDirection);
			return 0;
		}

		buttondown.down(d->id,LEFTBUTTON,x,y,lasthover);
		return 0;
	}

	if (mode==EMODE_Trace) {
		 //we haven't clicked on the tracing box, so search for images to grab..
		//RectInterface *rect=new RectInterface(0,dp);
		//rect->style|= RECT_CANTCREATE | RECT_OBJECT_SHUNT;
		//dynamic_cast<RectInterface*>(child)->FakeLBDown(x,y,state,count,d);
		//rect->owner=this;
        //rect->UseThis(&base_cells,0);
        //child=rect;
        //AddChild(rect,0,1);

		if (lasthover==ENGRAVE_Trace_Object || lasthover==ENGRAVE_Trace_Move_Mesh) {
			buttondown.down(d->id,LEFTBUTTON,x,y,lasthover);
			controlmode=EMODE_Controls;
			needtodraw=1;
			return 0;
		}

		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

		if (!trace->traceobject) {
			SomeData *obj=NULL;
			ObjectContext *oc=NULL;
			int c=viewport->FindObject(x,y,NULL,NULL,1,&oc);
			if (c>0) obj=oc->obj;

			if (obj && obj!=edata) {
				 //set up proxy object
				SomeDataRef *ref=dynamic_cast<SomeDataRef*>(somedatafactory()->NewObject("SomeDataRef"));
				ref->Set(obj, false);
				ref->flags|=SOMEDATA_KEEP_ASPECT;
				double m[6]; //,m2[6],m3[6];
				viewport->transformToContext(m,oc,0,1);
				//viewport->transformToContext(m2,poc,0,1);//of current mesh
				//transform_invert(m3,m2);
				//transform_mult(m2,m,m3);
				//ref->m(m2);
				ref->m(m);

				trace->Install(TraceObject::TRACE_Object, ref); 
				trace->ClearCache(false);

				if (data->UsesPath()) ActivatePathInterface();
				needtodraw=1;
			}
		}


		return 0;
	}

	if (mode==EMODE_Freehand) {
		FreehandInterface *freehand=new FreehandInterface(this,-1,dp);
		freehand->freehand_style=FREEHAND_Path_Mesh|FREEHAND_Remove_On_Up;
		viewport->Push(freehand,-1,0);
		freehand->LBDown(x,y,state,count,d);
		if (child) RemoveChild();
		child=freehand;
		return 0;
	}
	
	if (!edata) ChangeMode(EMODE_Mesh);

	if (mode==EMODE_Mesh) {
		EngraverFillData *olddata=edata;
		int c=PatchInterface::LBDown(x,y,state,count,d);
		if (!edata && data) {
			edata=dynamic_cast<EngraverFillData*>(data);
			AddToSelection(poc);
		}
		if (edata!=olddata) UpdatePanelAreas();
		return c;
	}

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  || mode==EMODE_AvoidToward
		  ) {
		if (count==2 && (state&LAX_STATE_MASK)==ShiftMask) {
			 //edit brush ramp

			 // *** in future, should be on symmetric brush ramp editing
			CurveMapInterface *ww=new CurveMapInterface(-1,dp,_("Brush Ramp"));
			ww->SetInfo(&thickness);

			child=ww;
            ww->owner=this;
			int pad=(dp->Maxx-dp->Minx)*.1;
			ww->SetupRect(dp->Minx+pad,dp->Miny+pad, dp->Maxx-dp->Minx-2*pad,dp->Maxy-dp->Miny-2*pad);
            viewport->Push(ww,-1,0);
			submode=0;
			return 0;
		}
		buttondown.down(d->id,LEFTBUTTON, x,y, mode, (submode==2 && sensbox.pointIsIn(x,y) ? 1 : 0));
		return 0;
	}

	if (mode==EMODE_Orientation) {
		if (lasthover==ENGRAVE_Orient_Position ||
			lasthover==ENGRAVE_Orient_Direction ||
			lasthover==ENGRAVE_Orient_Keep_Old ||
			lasthover==ENGRAVE_Orient_Quick_Adjust ||
			lasthover==ENGRAVE_Orient_Spacing ||
			lasthover==ENGRAVE_Orient_Parameter)
		  {
		  buttondown.down(d->id,LEFTBUTTON, x,y, lasthover);
		}
		return 0;
	}

	return 0;
}
	
int EngraverFillInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d)
{
	if (child) {
		if (child==&curvemapi) { //to be here, curvemapi must have taken the lbdown
			child->LBUp(x,y,state,d);
			child->dec_count();
			child=NULL;
			needtodraw=1;
			return 0;
		}
	}

	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;


	 //catch trace box overlay first
	int over=0;
	int category=0;
	buttondown.getextrainfo(d->id,LEFTBUTTON, &over,&category);

	if (category==ENGRAVE_Panel) {

		int dragged=buttondown.up(d->id,LEFTBUTTON);
		controlmode=mode;

		if (lasthover!=over) return 0;
		//so all these that follow are clicking down and up over same area..

		 //open or collapse a whole section..
		if ( dragged<10
			  && (over==ENGRAVE_Tracing
			   || over==ENGRAVE_Direction
			   || over==ENGRAVE_Spacing
			   || over==ENGRAVE_Dashes)) {
			MenuItem *item=NULL;
			for (int c=0; c<panel.n(); c++) {
				item=panel.e(c);
				if (item->id==over) {
					if (item->isOpen()) item->Close();
					else item->Open();
					UpdatePanelAreas();
				}
				
			}
			needtodraw=1;
			return 0;
		}

		if (dragged<10 && over==ENGRAVE_Toggle_Group_List) {
			MenuItem *item=panel.findid(ENGRAVE_Group_List);
			if (item->isOpen()) item->Close(); else item->Open();
			UpdatePanelAreas();
			needtodraw=1;
			return 0;
		}

		if (dragged<10 && over==ENGRAVE_Group_List) {
			EngraverFillData *obj;
			int gindex=-1;
			obj=GroupFromLineIndex(lasthoverindex, &gindex);

			if (!obj) return 0;

			if ((obj!=edata && lasthoverdetail==3) || gindex<0) { //clicking down on object header
				if (obj!=edata) {
					UseThisObject(selection->e(selection->ObjectIndex(obj)));

				} else if (obj==edata && gindex<0) {
					MenuItem *item=panel.findid(over);
					double th=dp->textheight();
					int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
					DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
										yy,yy+th);
					viewport->SetupInputBox(object_id, NULL, obj->Id(), "renameobject", bounds);
					eventgroup=gindex;
					eventobject=obj->object_id;
				}

				if (gindex>=0) current_group=gindex;
				else current_group=0;
				needtodraw=1;
				return 0;

			} else { //we are clicking down on a particular group for that object
				if (lasthoverdetail==0) {
					 //active
					obj->groups.e[gindex]->active=!obj->groups.e[gindex]->active;

				} else if (lasthoverdetail==1) {
					 //linked
					obj->groups.e[gindex]->linked=!obj->groups.e[gindex]->linked;

				} else if (lasthoverdetail==2) {
					 //color
					anXWindow *w=new ColorSliders(NULL,"New Color","New Color",
								   ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS,
								   0,0,200,400,0,
								   NULL,object_id,"newcolor",
								   LAX_COLOR_RGB,1./255,
								   obj->groups.e[gindex]->color.red/65535.,
								   obj->groups.e[gindex]->color.green/65535.,
								   obj->groups.e[gindex]->color.blue/65535.,
								   obj->groups.e[gindex]->color.alpha/65535.
								);
					eventgroup=gindex;
					eventobject=edata->object_id;
					app->addwindow(w);

				} else if (lasthoverdetail==3) {
					 //name/make current
					if (obj!=edata) {
						 //make current
						int i=selection->ObjectIndex(obj);
						UseThisObject(selection->e(i));
						current_group=0;
					} 

					if (gindex>=0) {
						if (gindex<obj->groups.n) {
							if (current_group!=gindex) {
								current_group=gindex;
							} else {
								MenuItem *item=panel.findid(over);
								double th=dp->textheight();
								int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
								DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
													yy,yy+th);
								viewport->SetupInputBox(object_id, NULL, obj->groups.e[gindex]->name, "renamegroup", bounds);
								eventgroup=gindex;
								eventobject=obj->object_id;

								return 0;
							}
						}
					}
				} //lasthoverdetail switch
			}//if gindex>=0

			needtodraw=1;
			return 0;
		} //if not dragged and Group_List

		if (over>=EMODE_Mesh && over<EMODE_MAX) {
			ChangeMode(over);
			needtodraw=1;
			return 0;
		}


		//----- some options don't require an edata...
		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);

		if (over==ENGRAVE_Trace_Object) {
			 // *** not used anymore??
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
			if (!trace->Identifier()) {
				app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));
			} else {
				if (trace->traceobject) trace->ClearCache(true);
			}
		} else if (over==ENGRAVE_Trace_Object_Name) {
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
			if (!trace->traceobject) return 0;

			MenuItem *item=panel.findid(over);
			double th=dp->textheight();
			int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
			DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
								yy,yy+th);
			viewport->SetupInputBox(object_id, NULL, trace->traceobject->Id(), "renametraceobject", bounds);
			return 0;
		}

		if (!edata) return 0;

		 //------------group
		if (over==ENGRAVE_Group_Active) {
			//PerformAction(ENGRAVE_ToggleActive);
			if (group) group->active=!group->active;
			needtodraw=1;
			return 0;

		} else if (over==ENGRAVE_Group_Linked) {
			if (group) group->linked=!group->linked;
			needtodraw=1;
			return 0;

		} else if (over==ENGRAVE_Group_Color) {
            anXWindow *w=new ColorSliders(NULL,"New Color","New Color",
						   ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_OUT_CLICK_DESTROYS,
						   0,0,200,400,0,
                           NULL,object_id,"newcolor",
                           LAX_COLOR_RGB,1./255,
                           group->color.red/65535.,
                           group->color.green/65535.,
                           group->color.blue/65535.,
                           group->color.alpha/65535.
						);
			app->addwindow(w);

			eventgroup=current_group;
			eventobject=edata->object_id;
			return 0;

		} else if (over==ENGRAVE_Group_Name) {
			MenuItem *item=panel.findid(over);
			double th=dp->textheight();
			int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
			DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
								yy,yy+th);
			viewport->SetupInputBox(object_id, NULL, group->name, "renamegroup", bounds);

			eventgroup=current_group;
			eventobject=edata->object_id;
			return 0;

		} else if (over==ENGRAVE_Previous_Group) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group<=0) current_group=edata->groups.n-1;
			else current_group--;
			needtodraw=1;
			return 0;
			
		} else if (over==ENGRAVE_Next_Group) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group>=edata->groups.n-1) current_group=0;
			else current_group++;
			needtodraw=1;
			return 0; 
			
		} else if (over==ENGRAVE_Group_Down) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group==0) return 0;
			edata->groups.swap(current_group,current_group-1);
			current_group--;
			PostMessage(_("Moved down."));
			needtodraw=1;
			return 0; 

		} else if (over==ENGRAVE_Group_Up) {
			if (!edata || !edata->groups.n) return 0;
			if (current_group==edata->groups.n-1) return 0;
			edata->groups.swap(current_group,current_group+1);
			current_group++;
			PostMessage(_("Moved up."));
			needtodraw=1;
			return 0; 

		} else if (over==ENGRAVE_New_Group) {
			 //create a duplicate of the current group, add after current
			if (!edata) return 0;
			EngraverPointGroup *newgroup=new EngraverPointGroup(edata);
			newgroup->CopyFrom(group, false,false,false,false,false);
			makestr(newgroup->name,group->name);
			if (!newgroup->name) makestr(newgroup->name,"Group");

			edata->groups.push(newgroup,1,current_group+1);
			current_group++;
			if (current_group>=edata->groups.n) current_group=edata->groups.n-1;
			edata->MakeGroupNameUnique(current_group);
			UpdatePanelAreas();
			needtodraw=1;
			PostMessage(_("Group added."));
			return 0;
			
		} else if (over==ENGRAVE_Delete_Group) {
			if (current_group>=0 && edata->groups.n>1) {
				EngraverPointGroup *cur =edata->GroupFromIndex(current_group);
				if (curvemapi.GetInfo()==cur->trace->value_to_weight) curvemapi.SetInfo(NULL);

				edata->groups.remove(current_group);
				current_group--;
				if (current_group<0) current_group=0;
				UpdatePanelAreas();
				needtodraw=1;
				PostMessage(_("Group deleted."));
			}
			return 0;
			
		} else if (over==ENGRAVE_Merge_Group) { 
			if (!edata) return 0;
			edata->MergeDown(current_group);
			PostMessage(_("Merged."));
			needtodraw=1;
			return 0;


		 //------------tracing
		} else if (over==ENGRAVE_Trace_Same_As || over==ENGRAVE_Trace_Menu) {
			MenuInfo *menu=GetGroupMenu(ENGRAVE_Tracing);

	        if (menu) app->rundialog(new PopupMenu("Share Group","Share Group", 0,
                                     0,0,0,0,1,
                                     object_id,"sharetrace",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Trace_Continuous) {
			if (!group) group=edata->GroupFromIndex(-1); //shouldn't happen, trace should always be nonnull..

			group->trace->continuous_trace = !group->trace->continuous_trace;
			Trace();
			return 0;

		} else if (over==ENGRAVE_Trace_Once) {
			Trace(true);
			return 0;

		} else if (over==ENGRAVE_Trace_Set) {
			if (!group) return 0;
			group->trace->tracetype=!group->trace->tracetype;
			needtodraw=1;
			Trace();
			return 0;

		} else if (over==ENGRAVE_Trace_Load) {
			needtodraw=1;
			app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_ESCAPABLE|ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));

		} else if (over==ENGRAVE_Trace_Name) {
			MenuItem *item=panel.findid(over);
			double th=dp->textheight();
			int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
			DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + (panelbox.maxx-panelbox.minx)*.9,
								yy,yy+th);
			viewport->SetupInputBox(object_id, NULL, group->trace->Id(), "renametrace", bounds);

			eventgroup=current_group;
			eventobject=edata->object_id;

		} else if (over==ENGRAVE_Trace_Object_Menu) {
			MenuInfo *menu=new MenuInfo(_("Trace object"));

			EngraverPointGroup *group=(edata?edata->GroupFromIndex(current_group):NULL);
			menu->AddItem(_("New Linear Gradient"), ENGRAVE_Trace_Linear_Gradient,LAX_OFF,-2);
			menu->AddItem(_("New Radial Gradient"), ENGRAVE_Trace_Radial_Gradient,LAX_OFF,-2);
			menu->AddItem(_("Snapshot of current"), ENGRAVE_Trace_Snapshot,       LAX_OFF,-2);
			menu->AddItem(_("Use current"),         ENGRAVE_Trace_Current,        LAX_OFF|LAX_GRAY,-2);
			menu->AddItem(_("Load Image..."),       ENGRAVE_Trace_Load,           LAX_OFF,-2);
			menu->AddItem(_("Save Image..."),       ENGRAVE_Trace_Save,           LAX_OFF,-2);
			menu->AddItem(_("Clear trace object"),  ENGRAVE_Trace_Clear,          LAX_OFF,-2);

			if (group->trace->traceobject && group->trace->traceobject->ResourceOwner()!=group->trace)
				menu->AddItem(_("Make local"),      ENGRAVE_Make_Local,           LAX_OFF,-2);
			else
				menu->AddItem(_("Make shared resource"),ENGRAVE_Make_Shared,      LAX_OFF,-2);

			menu->AddSep(_("Resources"));
			ResourceManager *manager=InterfaceManager::GetDefault(true)->GetResourceManager();
			manager->ResourceMenu("TraceObject", true, menu);

	        app->rundialog(new PopupMenu("Trace Object","Trace Object", 0,
                                     0,0,0,0,1,
                                     object_id,"traceobjectmenu",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		 //------------dashes
		} else if (over==ENGRAVE_Dash_Same_As || over==ENGRAVE_Dash_Menu) {
			//int sharing=IsSharing(ENGRAVE_Dashes, current_group);
			MenuInfo *menu=GetGroupMenu(ENGRAVE_Dashes);

	        if (menu) app->rundialog(new PopupMenu("Share Dashes","Share Dashes", 0,
                                     0,0,0,0,1,
                                     object_id,"sharedash",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Dash_Name) {
			MenuItem *item=panel.findid(over);
			double th=dp->textheight();
			int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
			DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + (panelbox.maxx-panelbox.minx)*.9,
								yy,yy+th);
			viewport->SetupInputBox(object_id, NULL, group->dashes->Id(), "renamedash", bounds);

			eventgroup=current_group;
			eventobject=edata->object_id;

		} else if (over==ENGRAVE_Dash_Length) {
			if (dragged<10) {
				MenuItem *item=panel.findid(over);
				char str[20];
				sprintf(str,"%.10g",group->dashes->dash_length);
				double th=dp->textheight();
				int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
				DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
									yy,yy+th);
				viewport->SetupInputBox(object_id, NULL, str, "dashlength", bounds);
			}
			return 0;

		} else if (over==ENGRAVE_Dash_Seed) {
			if (dragged<10) {
				MenuItem *item=panel.findid(over);
				char str[20];
				sprintf(str,"%d",group->dashes->randomseed);
				double th=dp->textheight();
				int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
				DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
									yy,yy+th);
				viewport->SetupInputBox(object_id, NULL, str, "dashseed", bounds);
			}
			return 0;

		 //------------------- Direction
		} else if (over==ENGRAVE_Direction_Same_As || over==ENGRAVE_Direction_Menu) {
			MenuInfo *menu=GetGroupMenu(ENGRAVE_Direction);

	        if (menu) app->rundialog(new PopupMenu("Share Direction","Share Direction", 0,
                                     0,0,0,0,1,
                                     object_id,"sharedirection",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Direction_Name) {
			MenuItem *item=panel.findid(over);
			double th=dp->textheight();
			int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
			DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + (panelbox.maxx-panelbox.minx)*.9,
								yy,yy+th);
			viewport->SetupInputBox(object_id, NULL, group->direction->Id(), "renamedirection", bounds);

			eventgroup=current_group;
			eventobject=edata->object_id;

		} else if (over==ENGRAVE_Direction_Reline) {
			auto_reline=!auto_reline;
			needtodraw=1;
			return 0;

		} else if (over==ENGRAVE_Direction_Type) {
			MenuInfo *menu=new MenuInfo(_("Line type"));

			menu->AddItem(PGroupTypeName(PGROUP_Linear),PGROUP_Linear,  LAX_OFF|LAX_ISTOGGLE|(group->direction->type==PGROUP_Linear?LAX_CHECKED:0),-2);
			menu->AddItem(PGroupTypeName(PGROUP_Circular),PGROUP_Circular,  LAX_OFF|LAX_ISTOGGLE|(group->direction->type==PGROUP_Circular?LAX_CHECKED:0),-2);
			menu->AddItem(PGroupTypeName(PGROUP_Radial),PGROUP_Radial,  LAX_OFF|LAX_ISTOGGLE|(group->direction->type==PGROUP_Radial?LAX_CHECKED:0),-2);
			menu->AddItem(PGroupTypeName(PGROUP_Spiral),PGROUP_Spiral,  LAX_OFF|LAX_ISTOGGLE|(group->direction->type==PGROUP_Spiral?LAX_CHECKED:0),-2);

	        if (menu) app->rundialog(new PopupMenu("Direction","Direction", 0,
                                     0,0,0,0,1,
                                     object_id,"directiontype",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Direction_Profile_Scale) {
			if (dragged<10) {
				group->direction->scale_profile=!group->direction->scale_profile;
				if (group->direction->scale_profile) PostMessage(_("Scale profile with length"));
				else PostMessage(_("Stretch profile with max height"));
				needtodraw=1;
			}
			return 0;


		} else if (over==ENGRAVE_Direction_Profile_Menu) {
			MenuInfo *menu=new MenuInfo(_("Line Profile"));
			menu->AddSep(_("Profile Resources"));
			int n=menu->n();
			ResourceManager *manager=InterfaceManager::GetDefault(true)->GetResourceManager();
			manager->ResourceMenu("LineProfile", true, menu);
			if (n==menu->n()) {
				menu->AddItem(_("(none)"),NULL, 0,LAX_GRAY);
				//menu->AddItem(_("(none)"),0,LAX_GRAY,0,NULL);
			}
			if (group->direction->default_profile) {
				menu->AddSep();
				menu->AddItem(_("Clear"), -1,LAX_OFF,0,(MenuInfo*)NULL);
			}

	        app->rundialog(new PopupMenu("Line Profile","Line Profile", 0,
                                     0,0,0,0,1,
                                     object_id,"lineprofilemenu",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Direction_Seed) {
			if (dragged<10) {
				MenuItem *item=panel.findid(over);
				char str[20];
				sprintf(str,"%d",group->direction->seed);
				double th=dp->textheight();
				int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
				DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
									yy,yy+th);
				viewport->SetupInputBox(object_id, NULL, str, "directionseed", bounds);
			}
			return 0;

		} else if (over==ENGRAVE_Direction_Show_Dir) {
			if (dragged<10 && group) {
				show_direction=!show_direction;
				needtodraw=1;
			}
			return 0;

		} else if (over==ENGRAVE_Direction_Grow) {
			if (dragged<10 && group) {
				group->direction->grow_lines=!group->direction->grow_lines;
				Grow(true, false);
				needtodraw=1;
			}
			return 0;
			
		} else if (over==ENGRAVE_Direction_Fill) {
			if (dragged<10 && group) {
				group->direction->fill=!group->direction->fill;
				Grow(true, false);
				needtodraw=1;
			}
			return 0;

		} else if (over==ENGRAVE_Direction_Merge) {
			if (dragged<10 && group) {
				group->direction->merge=!group->direction->merge;
				Grow(true, false);
				needtodraw=1;
			}
			return 0;


		 //------------------- Spacing
		} else if (over==ENGRAVE_Spacing_Same_As || over==ENGRAVE_Spacing_Menu) {
			MenuInfo *menu=GetGroupMenu(ENGRAVE_Spacing);

	        if (menu) app->rundialog(new PopupMenu("Share Spacing","Share Spacing", 0,
                                     0,0,0,0,1,
                                     object_id,"sharespacing",
                                     d->id,
                                     menu,1,NULL,
                                     MENUSEL_LEFT));
			return 0;

		} else if (over==ENGRAVE_Spacing_Name) {
			MenuItem *item=panel.findid(over);
			double th=dp->textheight();
			int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
			DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + (panelbox.maxx-panelbox.minx)*.9,
								yy,yy+th);
			viewport->SetupInputBox(object_id, NULL, group->spacing->Id(), "renamespacing", bounds);

			eventgroup=current_group;
			eventobject=edata->object_id;

		} else if (over==ENGRAVE_Spacing_Default) {
			if (dragged<10) {
				MenuItem *item=panel.findid(over);
				char str[20];
				sprintf(str,"%.10g",group->spacing->spacing);
				double th=dp->textheight();
				int yy= panelbox.miny + item->y + ((y-panelbox.miny-item->y)/th+1)*th;
				DoubleBBox bounds(item->x+panelbox.minx+th,item->x+panelbox.minx+th + item->w,
									yy,yy+th);
				viewport->SetupInputBox(object_id, NULL, str, "defaultspacing", bounds);
			}
			return 0;

		} else if (over==ENGRAVE_Spacing_Map || over==ENGRAVE_Spacing_Map_Menu) {
			if (dragged<10) {
				MenuInfo *menu=new MenuInfo("Spacing");
#define SPCMENU_New_Snapshot (-2)
#define SPCMENU_Load_Map     (-3)
#define SPCMENU_Save_Map     (-4)
#define SPCMENU_Clear_Map    (-5)
				menu->AddItem(_("New from snapshot"),SPCMENU_New_Snapshot, LAX_OFF);
				menu->AddItem(_("Load spacing map"),SPCMENU_Load_Map, LAX_OFF);
				if (group && group->spacing->map) {
					menu->AddItem(_("Save spacing map"),SPCMENU_Save_Map, LAX_OFF);
					menu->AddItem(_("Clear map object"),SPCMENU_Clear_Map, LAX_OFF);
				}

				if (menu) app->rundialog(new PopupMenu("Share Spacing","Share Spacing", 0,
										 0,0,0,0,1,
										 object_id,"spacingmenu",
										 d->id,
										 menu,1,NULL,
										 MENUSEL_LEFT));

			}

			PostMessage("UNIMPLEMENTED!!!!");
			return 0;

		} else {
			//some are just labels...

		}

		needtodraw=1;
		return 0;
	} //if (category==ENGRAVE_Panel)


	if (mode==EMODE_Mesh) {
		PatchInterface::LBUp(x,y,state,d);
		if (!edata && data) edata=dynamic_cast<EngraverFillData*>(data);
		if (edata && always_warp) edata->Sync(false);
		return 0;
	}

	if (mode==EMODE_Freehand) {
//		if (child) {
//			RemoveChild();
//			needtodraw=1;
//		}
		return 0;
	}

	if (mode==EMODE_Direction) {
		buttondown.up(d->id,LEFTBUTTON);
		return 0;
	}

	if (mode==EMODE_Trace) {
		int ix,iy;
		buttondown.getinitial(d->id,LEFTBUTTON, &ix,&iy);
		int dragged=buttondown.up(d->id,LEFTBUTTON);

		if (over==ENGRAVE_Trace_Object) {
			if (dragged<3 && (state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
				 //align to 0/90/180/270

				EngraverPointGroup *group=(edata?edata->GroupFromIndex(current_group):NULL);
				EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

				double angle=trace->traceobject->object->xaxis().angle(); //angle is -pi..pi
				//if (angle<0) angle+=2*M_PI;
				//double newangle= (angle - M_PI/2*int(angle/(M_PI/2) + .5)) + M_PI/2;
				double newangle;
				if (angle<-M_PI/2 || angle==M_PI) newangle=-M_PI/2;
				else if (angle<0) newangle=0;
				else if (angle<M_PI/2) newangle=M_PI/2;
				else newangle=M_PI;

				trace->traceobject->object->Rotate(-(newangle-angle), dp->screentoreal(ix,iy));
				
				Trace();
				needtodraw=1;
			}
		}
		return 0;
	}

	if (mode==EMODE_Orientation) {
		int dragged=buttondown.up(d->id,LEFTBUTTON);

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		if (over==ENGRAVE_Orient_Parameter) {
			EngraverDirection::Parameter *p=group->direction->GetParameter(lasthoverindex);
			if (p && dragged<5) {
				if (p->type=='b') {
					p->value=(p->value==0 ? 1 : 0);

					if (group->direction->grow_lines) {
						Grow(true, false);
						edata->Sync(true);
					} else {
						group->Fill(edata,-1);
						edata->Sync(false);
					}

					needtodraw=1;
				}
			}
			return 0;
		}

		 //now for clicking down and up on various knobs, pop up number edits 
		if (dragged>5) return 0;

		const char *what=NULL, *whatm=NULL;
		char str[50];

		if (over==ENGRAVE_Orient_Quick_Adjust) {
			what="quickadjust";
			whatm=_("Quick Adjust");
			sprintf(str,"1.0");

		} else if (over==ENGRAVE_Orient_Spacing) {
			what="orientspacing";
			whatm=_("Spacing");
			sprintf(str,"%.10g",group->spacing->spacing);

		} else if (over==ENGRAVE_Orient_Direction) {
			what="orientdirection";
			whatm=_("Direction");
			sprintf(str,"%.10g",atan2(group->directionv.y,group->directionv.x)*180/M_PI);

		//} else if (over==ENGRAVE_Orient_Position) {
		//} else if (over==ENGRAVE_Orient_Keep_Old) {
		//} else if (over==ENGRAVE_Orient_Grow) {
		//} else if (over==ENGRAVE_Orient_Type) {
		}

		if (what) {
			double th=dp->textheight();
			DoubleBBox bounds(x-15*th/2,x-15*th/2+15*th, y+th,y+2*th);
			viewport->SetupInputBox(object_id, NULL, str, what, bounds, whatm);
			eventgroup=current_group;
			eventobject=edata->object_id;
		}
		return 0;
	}

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_Twirl
		  || mode==EMODE_AvoidToward
		  ) {
		buttondown.up(d->id,LEFTBUTTON);

		if ( mode==EMODE_Drag
		  || mode==EMODE_Turbulence
		  || mode==EMODE_PushPull
		  || mode==EMODE_AvoidToward
		  || mode==EMODE_Twirl)
			edata->ReverseSync(true);

		needtodraw=1;
	}

	buttondown.up(d->id,LEFTBUTTON);

	return 0;
}

int EngraverFillInterface::NumGroupLines()
{
	EngraverFillData *obj; 
	int n=0;
	
	if (!selection) {
		if (edata) return edata->groups.n+1;
		return 1;
	}

	for (int g=0; g<selection->n(); g++) {
		obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;
		n++;
		n+=obj->groups.n;;
	}

	return n;
}

/*! Retrun the object and the group index within object based on the visible line number i.
 * i==0 is the first object with -1 for group index, 1 is first object and 0 for group index, and so on.
 */
EngraverFillData *EngraverFillInterface::GroupFromLineIndex(int i, int *gi)
{
	if (!selection) {
		if (i==0) {
			*gi=-1;
		}
		return edata;
	}

	EngraverFillData *obj; 
	
	for (int g=0; g<selection->n(); g++) {
		obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;
		if (i==0) {
			*gi=-1;
			return obj;
		}

		i--;
		if (i<obj->groups.n) {
			*gi=i;
			return obj;
		}
		i-=obj->groups.n;
	}

	*gi=-1;
	return NULL;
}

/*! Find what is the current line index, returning the max index currently allowed.
 */
int EngraverFillInterface::CurrentLineIndex(int *maxindex)
{
	if (!edata || !selection || !selection->n()) {
		*maxindex=0;
		return -1;
	}

	EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
	EngraverFillData *obj; 
	
	int i=-1;
	int max=0;
	int cur=-1;

	for (int g=0; g<selection->n(); g++) {
		obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;

		max+=1 + obj->groups.n;
		i++;

		if (obj==edata) {
			for (int c=0; c<obj->groups.n; c++) {
				if (group==obj->groups.e[c]) {
					cur=i+c+1; break;
				}
			}
		}

		i+=obj->groups.n;
	}

	if (maxindex) *maxindex=max;
	return cur;
}


Laxkit::MenuInfo *EngraverFillInterface::GetGroupMenu(int what)
{
	if (!edata) return NULL;

	MenuInfo *menu=new MenuInfo();
	int shared;
	EngraverFillData *obj;
	int i=0;
	int current=current_group;
	

	//pull from or push to single all have info == -3
	//push to all, id 1, info == -3
	//make local,  id ENGRAVE_Make_Local, info == -4
	//make resource,  id ENGRAVE_Make_Shared, info == -4
	//resource menu favorites have info>=0
	//resource menu other items have info==-1

	if (selection->n()>1 || edata->groups.n>1) {
		 //Pull from...
		menu->AddSep(_("Use from"));
		for (int g=0; g<selection->n(); g++) {
			obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
			if (!obj) continue;

			menu->AddSep(obj->Id());
			i++;

			for (int c=0; c<obj->groups.n; c++) {
				if (obj==edata && c==current) { i++; continue; }

				shared=0;
				if (what==ENGRAVE_Tracing        && obj->groups.e[c]->trace    ==edata->groups.e[current]->trace ) shared=1;
				else if (what==ENGRAVE_Dashes    && obj->groups.e[c]->dashes   ==edata->groups.e[current]->dashes) shared=1;
				else if (what==ENGRAVE_Direction && obj->groups.e[c]->direction==edata->groups.e[current]->direction) shared=1;
				else if (what==ENGRAVE_Spacing   && obj->groups.e[c]->spacing  ==edata->groups.e[current]->spacing) shared=1;
				
				menu->AddItem(obj->groups.e[c]->name, 10000+i, LAX_ISTOGGLE|(shared ? LAX_CHECKED : 0),
								-3, NULL);
				i++;
			}
		}

		//Push to...
		menu->AddSep(_("Push to"));
		i=0;
		for (int g=0; g<selection->n(); g++) {
			obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
			if (!obj) continue;

			menu->AddSep(obj->Id());
			i++;

			for (int c=0; c<obj->groups.n; c++) {
				if (obj==edata && c==current) { i++; continue; }

				shared=0;
				if (what==ENGRAVE_Tracing        && obj->groups.e[c]->trace    ==edata->groups.e[current]->trace )    shared=1;
				else if (what==ENGRAVE_Dashes    && obj->groups.e[c]->dashes   ==edata->groups.e[current]->dashes)    shared=1;
				else if (what==ENGRAVE_Direction && obj->groups.e[c]->direction==edata->groups.e[current]->direction) shared=1;
				else if (what==ENGRAVE_Spacing   && obj->groups.e[c]->spacing  ==edata->groups.e[current]->spacing)   shared=1;
				
				menu->AddItem(obj->groups.e[c]->name, 20000+i, LAX_ISTOGGLE|(shared ? LAX_CHECKED : 0),
								-3, NULL);
				i++;
			}
		}

		menu->AddSep();
		menu->AddItem(_("Push to all..."), 1, LAX_OFF, -3,NULL);
	}

	shared=1;
	if (what==ENGRAVE_Tracing        && edata->groups.e[current]->trace    ->ResourceOwner()==edata->groups.e[current]) shared=0;
	else if (what==ENGRAVE_Dashes    && edata->groups.e[current]->dashes   ->ResourceOwner()==edata->groups.e[current]) shared=0;
	else if (what==ENGRAVE_Direction && edata->groups.e[current]->direction->ResourceOwner()==edata->groups.e[current]) shared=0;
	else if (what==ENGRAVE_Spacing   && edata->groups.e[current]->spacing  ->ResourceOwner()==edata->groups.e[current]) shared=0;

	if (shared) menu->AddItem(_("Make local"),ENGRAVE_Make_Local, LAX_OFF, -4,NULL);
	else menu->AddItem(_("Make shared resource"),ENGRAVE_Make_Shared, LAX_OFF, -4,NULL);

	menu->AddSep(_("Resources"));
	ResourceManager *manager=InterfaceManager::GetDefault(true)->GetResourceManager();

	if      (what==ENGRAVE_Tracing)   manager->ResourceMenu("EngraverTraceSettings", true, menu);
	else if (what==ENGRAVE_Dashes)    manager->ResourceMenu("EngraverLineQuality",   true, menu);
	else if (what==ENGRAVE_Direction) manager->ResourceMenu("EngraverDirection",     true, menu);
	else if (what==ENGRAVE_Spacing  ) manager->ResourceMenu("EngraverSpacing",       true, menu);

	return menu;
}

const char *EngraverFillInterface::ModeTip(int mode)
{
	if (mode==EMODE_Mesh         ) return _("Mesh mode");
	if (mode==EMODE_Thickness    ) return _("Thickness, shift for brush size, control to thin");
	if (mode==EMODE_Blockout     ) return _("Blockout mode, shift for brush size, control to turn on");
	if (mode==EMODE_Drag         ) return _("Drag mode, shift for brush size");
	if (mode==EMODE_PushPull     ) return _("Push or pull. Shift for brush size");
	if (mode==EMODE_AvoidToward  ) return _("Avoid or pull toward. Shift for brush size");
	if (mode==EMODE_Twirl        ) return _("Twirl, Shift for brush size");
	if (mode==EMODE_Turbulence   ) return _("Turbulence, randomly push sample points");
	if (mode==EMODE_Resolution   ) return _("Resolution. Add or remove sample points");
	if (mode==EMODE_Orientation  ) return _("Orientation mode");
	if (mode==EMODE_Freehand     ) return _("Freehand mode");
	if (mode==EMODE_Trace        ) return _("Trace adjustment mode");
	if (mode==EMODE_Direction    ) return _("Direction adjustment mode");

	return "";
}

void EngraverFillInterface::ChangeMessage(int forwhich)
{
	if (forwhich==ENGRAVE_Trace_Once) PostMessage(_("Trace once"));
	else if (forwhich==ENGRAVE_Trace_Load) PostMessage(_("Load an image to trace"));
	else if (forwhich==ENGRAVE_Trace_Continuous) PostMessage(_("Toggle continuous tracing"));
	else if (forwhich==ENGRAVE_Trace_Opacity) PostMessage(_("Trace object opacity"));
	else if (forwhich==ENGRAVE_Trace_Object) {
		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
		if (trace->Identifier()) PostMessage(_("Click to remove trace object"));
		else PostMessage(_("Click to load an image to trace"));

	} else if (forwhich==ENGRAVE_Orient_Direction) PostMessage(_("Drag to change direction"));
	else if (forwhich==ENGRAVE_Orient_Position) PostMessage(_("Drag to change position"));
	else if (forwhich==ENGRAVE_Orient_Spacing) PostMessage(_("Drag to change spacing"));
	else if (forwhich==ENGRAVE_Orient_Keep_Old) PostMessage(_("Toggle keeping old thickness when reorinting"));
	else if (forwhich==ENGRAVE_Orient_Quick_Adjust) PostMessage(_("Drag for quick thickness adjustment"));
	else if (forwhich>=EMODE_Mesh && forwhich<EMODE_MAX) PostMessage(ModeTip(forwhich)); 
	else if (forwhich==ENGRAVE_Direction_Point_Off_Size) PostMessage(_("Point offset noisiness size")); 
	else if (forwhich==ENGRAVE_Direction_Profile_Start) PostMessage(_("Start position of new lines")); 
	else if (forwhich==ENGRAVE_Direction_Profile_End) PostMessage(_("End position of new lines")); 
	else if (forwhich==ENGRAVE_Direction_Profile_Start_Random) PostMessage(_("Randomness of start position of new lines")); 
	else if (forwhich==ENGRAVE_Direction_Profile_End_Random) PostMessage(_("Randomness of end position of new lines")); 
	else if (forwhich==ENGRAVE_Orient_Parameter) {
		EngraverPointGroup *group=(edata?edata->GroupFromIndex(current_group):NULL);
		if (group && group->direction->NumParameters()) {
			EngraverDirection::Parameter *p=group->direction->GetParameter(lasthoverindex);
			if (p) {
				PostMessage(p->Name);
			}
		}

	} else {
		MenuItem *item=panel.findid(forwhich);
		if (item && !isblank(item->name)) {
			PostMessage(item->name);
			return;
		}
	}

	if (forwhich==ENGRAVE_None) PostMessage(" ");
}

int EngraverFillInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *mouse)
{
	if (child) {
		if (child==&curvemapi) {
			 //to be here, curvemapi must have taken the lbdown
			child->MouseMove(x,y,state,mouse);
			Trace();

			needtodraw=1;
			return 0;
		}

		 //if some other child, assume we let it just operate
		 //except for mesh path manipulations
		if (!(mode==EMODE_Mesh && data && data->UsesPath())) return 1;
	}

	 //smooth out hoverdir hint for EMODE_AvoidToward
	for (int c=0; c<9; c++) hdir[c]=hdir[c+1];
	//hdir[3].x=x-hover.x;
	//hdir[3].y=y-hover.y;
	//hoverdir=hdir[0]+hdir[1]+hdir[2]+hdir[3];
	hdir[9].x=x;
	hdir[9].y=y;
	hoverdir=hdir[9]-hdir[0];
	hover.x=x;
	hover.y=y;

	if (!buttondown.any()) {
		 //update lasthover
		int category=0, index=-1, detail=-1;
		int newhover= scanEngraving(x,y,state, &category, &index, &detail);
		if (newhover!=lasthover || index!=lasthoverindex || detail!=lasthoverdetail) {
			lasthover=newhover;
			lasthovercategory=category;
			lasthoverindex=index;
			lasthoverdetail=detail;
			ChangeMessage(lasthover);
			needtodraw=1;
		}

		if (lasthover==ENGRAVE_Trace_Curve) {
			EngraverPointGroup *group=(edata?edata->GroupFromIndex(current_group):NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
			if (curvemapi.GetInfo()!=trace->value_to_weight) {
				curvemapi.SetInfo(trace->value_to_weight);
				curvemapi.SetupRect(panelbox.minx+tracebox->x,panelbox.miny+tracebox->y, tracebox->w,tracebox->h-dp->textheight()/2);
			}
			curvemapi.MouseMove(x,y,state,mouse);
			if (curvemapi.needtodraw) needtodraw=1;
		}

		DBG cerr <<"eng lasthover: "<<lasthover<<"  index:"<<index<<"  detail"<<detail<<endl;
	}


	if (controlmode==EMODE_Controls) {
		if (buttondown.any()) {
			int over=0, lx,ly, ix,iy;
			int overcat=0;
			double th=dp->textheight();
			double pad=th/2;

			buttondown.getextrainfo(mouse->id,LEFTBUTTON, &over,&overcat);
			buttondown.move(mouse->id,x,y, &lx,&ly);

			EngraverPointGroup *group=(edata?edata->GroupFromIndex(current_group):NULL);
			EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

			if (over== ENGRAVE_Trace_Object) {
				buttondown.getinitial(mouse->id,LEFTBUTTON, &ix,&iy);

				if ((state&LAX_STATE_MASK)==ControlMask) {
					 //scale trace object
					double s=1+.01*(x-lx);
					if (s<.2) s=.2;
					trace->traceobject->object->Scale(dp->screentoreal(ix,iy),s);

					//trace->traceobject->object->Scale(dp->screentoreal(ix,iy),dp->screentoreal(lx,ly),dp->screentoreal(x,y));
//					if (s<.8) s=.8;
//					for (int c=0; c<4; c++) {
//						trace->traceobject->object->m(c,trace->traceobject->object->m(c)*s);
//					}

				} else if ((state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
					 //rotate trace object
					double s=.01*(x-lx);
					trace->traceobject->object->Rotate(s, dp->screentoreal(ix,iy));

				} else {
					 //move trace object
					flatpoint p=dp->screentoreal(x,y) - dp->screentoreal(lx,ly);
					trace->traceobject->object->origin(trace->traceobject->object->origin()+p);
				}
				Trace();
				needtodraw=1;

			} else if (over==ENGRAVE_Trace_Move_Mesh) {
				flatpoint p=screentoreal(x,y)-screentoreal(lx,ly);
				edata->origin(edata->origin()+p);

				Trace();
				needtodraw=1;

			} else if (over==ENGRAVE_Trace_Opacity) {
				double pad=2;
				//x-=panelbox.minx;
				//y-=panelbox.miny;

				double z=(x-(panelbox.minx+pad))/(panelbox.maxx-panelbox.minx-2*pad);
				if (z<0) z=0; else if (z>1) z=1;
				trace->traceobj_opacity=z;
				needtodraw=1;

			} else if (over==ENGRAVE_Panel) {
				if ((state&LAX_STATE_MASK)==ControlMask) {
					 //scale up box...
					double s=1+.01*(x-lx);
					if (s<.8) s=.8;
					panelbox.maxx=panelbox.minx+s*(panelbox.maxx-panelbox.minx);
					panelbox.maxy=panelbox.miny+s*(panelbox.maxy-panelbox.miny);
					UpdatePanelAreas();

				} else if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
					 //scale horizontally
					double s=1+.01*(x-lx);
					if (s<.8) s=.8;
					panelbox.maxx=panelbox.minx+s*(panelbox.maxx-panelbox.minx);
					UpdatePanelAreas();
					
				} else {
					 //move box
					panelbox.minx+=x-lx;
					panelbox.maxx+=x-lx;
					panelbox.miny+=y-ly;
					panelbox.maxy+=y-ly;
				}
				needtodraw=1;

			} else if (over==ENGRAVE_Direction_Profile_Max_Height) {
				if (group) {
					MenuItem *item=panel.findid(ENGRAVE_Direction_Profile_Max_Height);
					double z;
					if (buttondown.isdragged(mouse->id,LEFTBUTTON)>5) {
						double mod=((state&ShiftMask) ? .1 : ((state&ControlMask) ? .01 : 1));
						z=group->direction->max_height + mod*(x-lx)*2.0/item->w;
						if (z<0) z=0;
						group->direction->max_height=z;
					} else { 
						z=(x-(panelbox.minx+item->x))/(item->w);
						if (z<0) z=0; else if (z>1) z=1;
						group->direction->max_height=z*2;
					}
					
					if (auto_reline) {
						group->Fill(edata, -1);
						edata->Sync(false);
						Trace();
					}
					needtodraw=1;					
				}
				return 0;

			} else if (over==ENGRAVE_Direction_Seed) {
				int dx=(x-lx);
				if (labs(dx)>2) dx/=2;
				group->direction->seed+=dx;
				if (group->direction->seed<0) group->direction->seed=0;

				if (auto_reline) {
					group->Fill(edata, -1);
					edata->Sync(false);
					Trace();
				}
				needtodraw=1;

				return 0;

			} else if (over==ENGRAVE_Direction_Profile_Start
					|| over==ENGRAVE_Direction_Profile_End
					) {
				if (!group) return 0;

				double z=(x-(panelbox.minx+pad))/(panelbox.maxx-panelbox.minx-2*pad-th);
				if (z<0) z=0; else if (z>1) z=1;
				
				if (over==ENGRAVE_Direction_Profile_Start)
					group->direction->profile_start=z;

				else if (over==ENGRAVE_Direction_Profile_End)
					group->direction->profile_end=z;

				if (auto_reline) {
					group->Fill(edata, -1);
					edata->Sync(false);
					Trace();
				}
				needtodraw=1;
				return 0;

			} else if (over==ENGRAVE_Direction_Profile_Start_Random
					|| over==ENGRAVE_Direction_Profile_End_Random
					) {
				if (!group) return 0;

				double dz=(x-lx)/(panelbox.maxx-panelbox.minx-2*pad-th);
				
				if (over==ENGRAVE_Direction_Profile_Start_Random) {
					double s=dz+group->direction->start_rand_width;
					if (s<0) s=0; else if (s>1) s=1;
					group->direction->start_rand_width=s;
					if (group->direction->start_rand_width==0) group->direction->start_type=0;
 					else group->direction->start_type=1;

				} else {
					double s=dz+group->direction->end_rand_width;
					if (s<0) s=0; else if (s>1) s=1;
					group->direction->end_rand_width=s;
					if (group->direction->end_rand_width==0) group->direction->end_type=0;
 					else group->direction->end_type=1;
				}

				if (auto_reline) {
					group->Fill(edata, -1);
					edata->Sync(false);
					Trace();
				}
				needtodraw=1;
				return 0;

			} else if (over==ENGRAVE_Direction_Line_Offset
					|| over==ENGRAVE_Direction_Point_Offset
					|| over==ENGRAVE_Direction_Point_Off_Size
					) {

				if (!group) return 0;

				double z=(x-(panelbox.minx+pad))/(panelbox.maxx-panelbox.minx-2*pad);
				if (z<0) z=0; else if (z>1) z=1;

				if (over==ENGRAVE_Direction_Line_Offset)
					group->direction->line_offset=z*2;

				else if (over==ENGRAVE_Direction_Point_Offset)
					group->direction->point_offset=z*2;

				else if (over==ENGRAVE_Direction_Point_Off_Size)
					group->direction->noise_scale=z*5;

				if (auto_reline) {
					group->Fill(edata, -1);
					edata->Sync(false);
					Trace();
				}
				needtodraw=1;
				return 0;

			} else if (over==ENGRAVE_Dash_Zero_Threshhold
					|| over==ENGRAVE_Dash_Broken_Threshhold
					|| over==ENGRAVE_Dash_Random
					|| over==ENGRAVE_Dash_Taper
					|| over==ENGRAVE_Dash_Density
					) {

				if (group) {
					double z=(x-(panelbox.minx+pad))/(panelbox.maxx-panelbox.minx-2*pad);
					if (z<0) z=0; else if (z>1) z=1;


					if (over==ENGRAVE_Dash_Broken_Threshhold)
						group->dashes->broken_threshhold=z*group->spacing->spacing;

					else if (over==ENGRAVE_Dash_Zero_Threshhold)
						group->dashes->zero_threshhold=z*group->spacing->spacing;

					else if (over==ENGRAVE_Dash_Random)
						group->dashes->dash_randomness=z;

					else if (over==ENGRAVE_Dash_Density)
						group->dashes->dash_density=z;

					else group->dashes->dash_taper=z;

					UpdateDashCaches(group->dashes);
					needtodraw=1;
				}
				return 0;

			} else if (over==ENGRAVE_Dash_Length) {
				int dx=x-lx;
				if (dx>0) {
					group->dashes->dash_length*=1.+.01*dx;
				} else {
					if (dx<-50) dx=-50;
					group->dashes->dash_length*=1/(1-.01*dx);
				}

				UpdateDashCaches(group->dashes);
				needtodraw=1;

				return 0;

			} else if (over==ENGRAVE_Dash_Seed) {
				int dx=(x-lx);
				if (labs(dx)>2) dx/=2;
				group->dashes->randomseed+=dx;
				if (group->dashes->randomseed<0) group->dashes->randomseed=0;

				UpdateDashCaches(group->dashes);
				needtodraw=1;

				return 0;

			} else if (over==ENGRAVE_Spacing_Default) {
				int dx=x-lx;
				if (dx>0) {
					group->spacing->spacing*=1.+.01*dx;
				} else {
					if (dx<-50) dx=-50;
					group->spacing->spacing*=1/(1-.01*dx);
				}

				edata->Sync(false);
				edata->UpdatePositionCache();
				group->UpdateDashCache();
				Trace();

				needtodraw=1;
				return 0;

			}
		}
		return 0;
	}

	if (mode==EMODE_Freehand && !child) {
		needtodraw=1;
		return 0;
	}

	if (mode==EMODE_Mesh) {
		PatchInterface::MouseMove(x,y,state,mouse);
		if (buttondown.any()) {
			if (always_warp && curpoints.n>0) {
				edata->Sync(false);
				edata->UpdatePositionCache();

				Grow(true, true);
			}

			Trace();
		}

		return 0;

	}

	if (mode==EMODE_Orientation) {
		if (!buttondown.any()) return 0;

		int lx,ly;
		int over;
		buttondown.getextrainfo(mouse->id,LEFTBUTTON, &over);
		buttondown.move(mouse->id,x,y, &lx,&ly);

		flatpoint  p=edata->transformPointInverse(screentoreal( x, y));
		flatpoint op=edata->transformPointInverse(screentoreal(lx,ly));
		flatpoint d=screentoreal( x, y)-screentoreal(lx,ly);
		//flatpoint md=p-op;

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		if (over==ENGRAVE_Orient_Direction) {
			flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
			double angle=angle_full(op-center,p-center,0);
			group->directionv=rotate(group->directionv,angle,0);

		} else if (over==ENGRAVE_Orient_Spacing) {
			flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
			double r1=norm( p-center);
			double r2=norm(op-center);
			group->spacing->spacing*=r1/r2;

		} else if (over==ENGRAVE_Orient_Position) {
			flatpoint pp=edata->getPoint(group->position.x,group->position.y, false);
			pp+=d;
			int status;
			pp=edata->getPointReverse(pp.x,pp.y, &status);
			if (status==1) group->position=pp;

		} else if (over==ENGRAVE_Orient_Parameter) {
			EngraverDirection::Parameter *p=group->direction->GetParameter(lasthoverindex);
			if (p) {
				flatpoint center=edata->transformPoint(edata->getPoint(group->position.x,group->position.y, false));
				center=realtoscreen(center);
				flatpoint dir=group->directionv/norm(group->directionv)*.05;
				flatpoint xx= realtoscreen(edata->transformPoint(edata->getPoint(.5+dir.x,.5+dir.y, false)))
							- realtoscreen(edata->transformPoint(edata->getPoint(.5,.5, false)));
				xx=30*xx/norm(xx);

				double npos=flatpoint(x-center.x,y-center.y)*xx/norm2(xx);

				int update=0;
				if (p->type=='b') {
					if (npos<0 && p->value!=0) { p->value=0; update=1; }
					else if (npos>=0 && p->value!=1) { p->value=1; update=1; }
				} else {
					npos=(npos+1)/2;
					npos=p->min + npos*(p->max-p->min);
					if (p->type=='i') npos=floorl(npos);

					if (p->min_type==1 && npos<p->min) npos=p->min;
					else if (p->max_type==1 && npos>p->max) npos=p->max;
					if (npos!=p->value) { p->value=npos; update=1; }

					DBG cerr <<" orient parameter npos="<<npos<<endl;
				}

				if (update) {
					if (group->direction->grow_lines) {
						Grow(true, false);
						edata->Sync(true);
					} else {
						group->Fill(edata,-1);
						edata->Sync(false);
					}
					needtodraw=1;
				}
			}

		} else if (over==ENGRAVE_Orient_Quick_Adjust) {
			flatpoint dir=group->directionv/norm(group->directionv)*.05;
			flatpoint xx=dp->realtoscreen(edata->getPoint(.5+dir.x,.5+dir.y, false)) - dp->realtoscreen(edata->getPoint(.5,.5, false));
			xx=xx/norm(xx);
			flatpoint yy=-transpose(xx);

			double amount=.05*flatpoint(x-lx,y-ly)*yy;
			if (state&ShiftMask) amount/=2;

			double factor=1;
			if (amount>0) factor=1+amount;
			else if (amount<0) factor=1/(1-amount);

			if (factor!=1.0) {
				if (factor<1) PostMessage(_("Thinning..."));
				else if (factor>1) PostMessage(_("Thickening..."));
				group->QuickAdjust(factor);
				group->direction->default_weight*=factor;
				group->default_weight*=factor;
			}

			needtodraw=1;
			return 0;
		}

		if (group->direction->grow_lines) Grow(false, false);
		else group->Fill(edata, -1);

		edata->Sync(false);
		edata->UpdatePositionCache();
		group->UpdateDashCache();
		Trace();

		needtodraw=1;
		return 0;
	}
	
	if (    mode==EMODE_Thickness
		 || mode==EMODE_Blockout
		 || mode==EMODE_Turbulence
		 || mode==EMODE_Drag
		 || mode==EMODE_PushPull
		 || mode==EMODE_AvoidToward
		 || mode==EMODE_Twirl
		 ) {

		if (!buttondown.any()) {
			needtodraw=1;
			return 0;
		}

		int lx,ly;
		int in_sens=0;
		buttondown.move(mouse->id,x,y, &lx,&ly);
		buttondown.getextrainfo(mouse->id,LEFTBUTTON,NULL,&in_sens);

		if ((state&LAX_STATE_MASK)==ShiftMask) {
			if (in_sens) {
				double newsens=((double)x-sensbox.x)/sensbox.width*2;
				if (newsens<=0) newsens=.00001;
				else if (newsens>2) newsens=2;

				if (mode==EMODE_Thickness)        sensitive_thickness=newsens;
				else if (mode==EMODE_Turbulence)  sensitive_turbulence=newsens;
				else if (mode==EMODE_Drag)        sensitive_drag=newsens;
				else if (mode==EMODE_PushPull)    sensitive_pushpull=newsens;
				else if (mode==EMODE_AvoidToward) sensitive_avoidtoward=newsens;
				else if (mode==EMODE_Twirl)       sensitive_twirl=newsens;

			} else {
				 //change brush size
				brush_radius+=(x-lx)*2;
				if (brush_radius<5) brush_radius=5;
			}
			needtodraw=1;

		} else {
			if (lx==x && ly==y) return 0;

			Affine tr;
			if (edata) tr=edata->GetTransformToContext(false, 1);//supposed to be from edata parent to base real

			flatvector brushcenter_pg=screentoreal(x,y); //brush center at page level
			brushcenter_pg=tr.transformPoint(brushcenter_pg);//now brushcenter_pg in page level coords

			flatvector brush_at_radius=screentoreal(x+brush_radius,y);
			brush_at_radius=tr.transformPoint(brush_at_radius);

			flatpoint lastp=screentoreal(lx,ly);
			lastp=tr.transformPoint(lastp);

			flatpoint brushcenter; //local to obj brush center
			double d, a, rr;
			LinePoint *l, *lstart;
			flatpoint pp, dv;
			double nearzero=.001; // *** for when tracing makes a value at 0, thicken makes it this

			EngraverFillData *obj;
			EngraverPointGroup *group;
			int recachewhich; //1 for thickness change, 2 for blockout change, 4 for position change
			
			double pressure=1;
			const_cast<LaxMouse*>(mouse)->getInfo(NULL,NULL,NULL,NULL,NULL,NULL,&pressure,NULL,NULL); //final 2 are tiltx and tilty

			for (int o=0; o<selection->n(); o++) {
				obj=dynamic_cast<EngraverFillData *>(selection->e(o)->obj);
				if (!obj) { continue; }

				tr=obj->GetTransformToContext(true, 0);//transform base real to obj coords
				brushcenter = tr.transformPoint(brushcenter_pg);

				dv=brushcenter - tr.transformPoint(lastp); //vector from last position
				rr=norm2(tr.transformPoint(brush_at_radius) - brushcenter); //radius^2 of brush in obj coordinates


				for (int g=0; g<obj->groups.n; g++) {
					group=obj->groups.e[g];
					if (!group->linked && !(obj==edata && g==current_group)) continue; //always act on current group
					if (!group->active) continue; //never work on invisible group
					DBG cerr <<" ************ tool on :"<<group->name<<endl;

					recachewhich=0;

					for (int c=0; c<group->lines.n; c++) {
						l=lstart=group->lines.e[c];

						if (l) do {
							d=norm2(l->p - brushcenter); //distance point to brush center

							if (d<rr) { //point is within...

								if (mode==EMODE_Thickness) {
									a=sqrt(d/rr);
									a=thickness.f(a)*pressure;

									if ((state&LAX_STATE_MASK)==ControlMask) {
										 //thin
										a=1-a*.04*sensitive_thickness;
									} else {
										 //thicken
										a=1+a*.04*sensitive_thickness;
										if (l->weight<=0) l->weight=nearzero;
									}
									l->weight*=a;
									if (l->cache) l->cache->weight*=a;
									recachewhich|=1;

								} else if (mode==EMODE_Blockout) {
									if ((state&LAX_STATE_MASK)==ControlMask) 
										l->on=ENGRAVE_On;
									else l->on=ENGRAVE_Off;
									recachewhich|=2;

								} else if (mode==EMODE_Drag) {
									a=sqrt(d/rr);
									a=thickness.f(a)*sensitive_drag*pressure;
									l->p+=dv*a; //point without mesh
									l->needtosync=2;
									recachewhich|=4;

								} else if (mode==EMODE_Turbulence) {
									a=sqrt(d/rr);
									a=thickness.f(a)*.5*sensitive_turbulence*pressure;
									if (dv.norm2()>group->spacing->spacing*.5) {
										dv.normalize();
										dv*=group->spacing->spacing;
									}
									l->p+=a*rotate(dv,drand48()*2*M_PI);
									l->needtosync=2;
									recachewhich|=4;

								} else if (mode==EMODE_PushPull) {
									a=sqrt(d/rr);
									a=thickness.f(a)*sensitive_pushpull*pressure;
									pp=(l->p-brushcenter)*.015;

									if ((state&LAX_STATE_MASK)==ControlMask) {
										l->p-=pp*a*d/rr;
									} else {
										l->p+=pp*a;
									}
									l->needtosync=2;
									recachewhich|=4;

								} else if (mode==EMODE_AvoidToward) {
									a=sqrt(d/rr);
									a=thickness.f(a)*sensitive_avoidtoward*pressure;

									flatvector vt=transpose(hoverdir);
									vt.normalize();
									vt*=.005*a*((l->p-brushcenter)*vt > 0 ? 1 : -1);

									if ((state&LAX_STATE_MASK)==ControlMask) {
										l->p-=vt;
									} else {
										l->p+=vt;
									}
									l->needtosync=2;
									recachewhich|=4;

								} else if (mode==EMODE_Twirl) {
									a=sqrt(d/rr);
									a=thickness.f(a)*.006*sensitive_twirl*pressure;

									if ((state&LAX_STATE_MASK)==ControlMask) {
										l->p=brushcenter+rotate(l->p-brushcenter,a);
									} else {
										l->p=brushcenter+rotate(l->p-brushcenter,-a);
									}
									l->needtosync=2;
									recachewhich|=4;
								}
							}

							l=l->next;
						} while (l && l!=lstart); //foreach point in line
					} //foreach line

					if (recachewhich&1) { //thickness change
						group->UpdateDashCache();
					} else if (recachewhich&2) { //blockout change
						group->UpdateDashCache();
					} else if (recachewhich&4) { //position change
						group->UpdatePositionCache();
					}

					if (group->trace->continuous_trace && (mode==EMODE_Thickness)) group->trace->continuous_trace=false;
					if (group->trace->continuous_trace) Trace();
				} //foreach relevant group in object
			} //foreach object in selection

			needtodraw=1;
		} //if distortion mode, not brush adjust mode

		return 0;
	}

	return 0;
}

int EngraverFillInterface::WheelUp(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (show_panel && panelbox.boxcontains(x,y)) {
	  if ((state&ControlMask)!=0) {
		 //zoom panel
		double w=panelbox.maxx-panelbox.minx;
		double h=panelbox.maxy-panelbox.miny;
		double rx=(x-panelbox.minx)/w;
		double ry=(y-panelbox.miny)/h;
		w*=1.1;
		h*=1.1;
		panelbox.minx=x-rx*w;
		panelbox.maxx=x+(1-rx)*w;
		panelbox.miny=y-ry*h;
		panelbox.maxy=y+(1-ry)*h;

		UpdatePanelAreas();
		needtodraw=1;
		return 0;

	  } else {
		  //move panel
		double h=panelbox.maxy-panelbox.miny;

		if (panelbox.miny+h*.1 < y) {
			panelbox.miny+=h*.1;
			panelbox.maxy+=h*.1;
			needtodraw=1;
		}
		return 0;
	  }
	}

	return 1;
}

int EngraverFillInterface::WheelDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	if (show_panel && panelbox.boxcontains(x,y)) {
	  if ((state&ControlMask)!=0) {
	     //zoom panel
		double w=panelbox.maxx-panelbox.minx;
		double h=panelbox.maxy-panelbox.miny;
		double rx=(x-panelbox.minx)/w;
		double ry=(y-panelbox.miny)/h;
		w*=1/1.1;
		h*=1/1.1;
		panelbox.minx=x-rx*w;
		panelbox.maxx=x+(1-rx)*w;
		panelbox.miny=y-ry*h;
		panelbox.maxy=y+(1-ry)*h;

		UpdatePanelAreas();
		needtodraw=1;
		return 0;

	  } else {
		 //move panel
		double h=panelbox.maxy-panelbox.miny;

		if (panelbox.maxy-h*.1 > y) {
			panelbox.miny-=h*.1;
			panelbox.maxy-=h*.1;
			needtodraw=1;
		}
		return 0;
	  }
	}
	return 1;
}

//! Checks for EngraverFillData, then calls PatchInterface::DrawData(ndata,a1,a2,info).
int EngraverFillInterface::DrawData(Laxkit::anObject *ndata,anObject *a1,anObject *a2,int info) // info=0
{
	if (!ndata || dynamic_cast<EngraverFillData *>(ndata)==NULL) return 1;

	EngraverFillData *ee=edata;
	edata=dynamic_cast<EngraverFillData *>(ndata);

	int  tshow_points   =show_points;  
	bool tshow_direction=show_direction;
	bool tshow_panel    =show_panel;
	bool tshow_trace_object    =show_trace_object;

	show_points   =false;  
	show_direction=false;
	show_panel    =false;
	show_trace_object    =false;

	int c=PatchInterface::DrawData(ndata,a1,a2,info);
	edata=ee;

	show_points   =tshow_points;  
	show_direction=tshow_direction;
	show_panel    =tshow_panel;
	show_trace_object    =tshow_trace_object;

	return c;
}

int EngraverFillInterface::Refresh()
{
	if (!needtodraw) return 0;


	 //draw the trace object if necessary
	EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
	EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

	if (viewport && trace->traceobject && trace->traceobject->object && trace->traceobj_opacity>.5 // **** .5 since actual opacity not working
			) {
			//&& (mode==EMODE_Trace || show_trace_object)) {

		Affine a;
		if (edata) a=edata->GetTransformToContext(true, 0);//supposed to be inverse from edata to base real
		//dp->setSourceAlpha(trace->traceobj_opacity);
		dp->PushAndNewTransform(a.m());
		dp->PushAndNewTransform(trace->traceobject->object->m());
		viewport->DrawSomeData(trace->traceobject->object, NULL,NULL,0);
		//dp->setSourceAlpha(1);
		dp->PopAxes();
		dp->PopAxes();

	} else if (trace->traceobject) {
		 //draw outline of traceobject, but don't draw the actual trace object
		dp->NewFG(.9,.9,.9);
		dp->LineAttributes(-1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->LineWidthScreen(1);

		Affine a;
		if (edata) a=edata->GetTransformToContext(true, 0);
		SomeData *o=trace->traceobject->object;
		if (o) {
			dp->PushAndNewTransform(a.m());
			dp->moveto(transform_point(o->m(),flatpoint(o->minx,o->miny)));
			dp->lineto(transform_point(o->m(),flatpoint(o->maxx,o->miny)));
			dp->lineto(transform_point(o->m(),flatpoint(o->maxx,o->maxy)));
			dp->lineto(transform_point(o->m(),flatpoint(o->minx,o->maxy)));
			dp->closed();
			dp->stroke(0);
			dp->PopAxes();
		}
	}

	if (mode==EMODE_Freehand && !child) {
		 //draw squiggly lines near mouse
		dp->NewFG(0.,0.,1.);
		dp->DrawScreen();
		dp->LineWidthScreen(1);
		double s=10;
		for (int c=-1; c<2; c++) {
			dp->moveto(hover-flatpoint(2*s,c*s));
			dp->curveto(hover-flatpoint(s*1.5,c*s+5), hover+flatpoint(-s/2,-c*s+s/2), hover+flatpoint(0,-c*s));
			dp->stroke(0);
		}
		dp->DrawReal();
		needtodraw=0;
	}

	if (!edata) {
		if (show_panel) DrawPanel();
		needtodraw=0;
		return 0;
	}



	 //----draw the actual lines
	LinePoint *l;
	//LinePoint *last=NULL;
	LinePointCache *lc, *lcstart, *clast=NULL;

	double mag=dp->Getmag();
	double lastwidth, neww;
	double tw;
	flatpoint lp,v;


	for (int g=0; g<edata->groups.n; g++) {
		group=edata->groups.e[g];
		if (!group->active) continue;
		if (!group->lines.n) {
			DBG cerr <<" *** WARNING! engraver group #"<<g<<" is missing lines!"<<endl;
			continue;
		}

		if (!group->lines.e[0]->cache) {
			group->UpdateBezCache();
			group->UpdateDashCache();
		} 

		for (int c=0; c<group->lines.n; c++) {
			l=group->lines.e[c];
			lc=lcstart=l->cache;
			clast=NULL;
			lastwidth=-1;

			dp->NewFG(&group->color);
			dp->LineAttributes(-1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
			dp->LineWidthScreen(1);

			do { //one loop per on segment
				if (!group->PointOnDash(lc)) { lc=lc->next; continue; } //advance to first on point

				if (!clast) {
					 //establish a first point of a visible segment
					clast=lc;
					lastwidth=lc->weight*mag;
					dp->LineWidthScreen(lastwidth);

					if (lc->on==ENGRAVE_EndPoint || !lc->next || !group->PointOnDash(lc->next)) {
						 //draw just a single dot
						dp->drawline(clast->p,clast->p);
						lc=lc->next;
						clast=NULL;
						continue;
					}
					lc=lc->next;
				}

				neww=lc->weight*mag;
				if (neww!=lastwidth) {
					lp=clast->p;
					v=(lc->p-clast->p)/9.;
					for (int c2=1; c2<10; c2++) {
						 //draw 10 mini segments, each of same width to approximate the changing width
						tw=lastwidth+c2/9.*(neww-lastwidth);
						dp->LineWidthScreen(tw);
						dp->drawline(lp+v*(c2-1), lp+v*c2);
					}

					lastwidth=neww;

				} else {
					dp->drawline(clast->p,lc->p);
				}

				if (lc->on==ENGRAVE_EndPoint) clast=NULL;
				else clast=lc;
				lc=lc->next;
				if (lc && !group->PointOnDash(lc)) clast=NULL;

			//} while (lc && lc->next && lc!=lcstart);
			} while (lc && lc!=lcstart);

			if (show_points) {
				 //show little red dots for all the sample points
				flatpoint pp;
				l=group->lines.e[c];
				dp->NewFG(1.,0.,0.);
				int p=1;
				char buffer[50];
				while (l) {
					dp->drawpoint(l->p, 2, 1);
					if (show_points&2) {
						sprintf(buffer,"%d,%d",c,p);
						dp->textout(l->p.x,l->p.y, buffer,-1, LAX_BOTTOM|LAX_HCENTER);
						p++;
					}
					l=l->next;
				}
			}
		} //foreach line
	} //foreach group


	 //show Direction Map
	if (show_direction) {
		DirectionMap *map=NULL;
		if (!map && edata) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			map=group->direction->map;
			if (!map) map=group;
		}
		if (!map) map=directionmap;

		if (map) {
			dp->DrawScreen();
			dp->NewFG(.6,.6,1.);
			dp->LineWidthScreen(1);

			//double s=1;
			int step=20;
			int win_w=dp->Maxx-dp->Minx;
			int win_h=dp->Maxy-dp->Miny;
			int ww=win_w - 2*step;
			int hh=win_h - 2*step;


			flatpoint v;
			double vv;
			flatpoint p,p2;
			int in;

			//-----------when direction is mesh based:----------------------
			if (edata) for (int x=win_w/2-ww/2; x<win_w/2+ww/2; x+=step) {
				for (int y=win_h/2-hh/2; y<win_h/2+hh/2; y+=step) {
					//p.x=x-(win_w/2-ww/2);
					//p.y=y-(win_h/2-hh/2);
					p.set(x,y);
					p=dp->screentoreal(p.x,p.y);
					//p=edata->transformPointInverse(p);
					p=edata->getPointReverse(p.x,p.y, &in);
					if (!in) continue;

					//DBG cerr <<int(xx)<<','<<int(yy)<<"  ";
					v=map->Direction(p.x,p.y);
					v.normalize();
					v*=.01;
					p2=p+v;
					p2=edata->getPoint(p2.x,p2.y, false);
					p2=dp->realtoscreen(p2);
					
					v=p2-flatpoint(x,y);
					vv=norm(v);
					if (vv>step*.8) v*=step*.8/vv;
					if (vv<step*.5) v*=step*.5/vv;


					dp->drawarrow(flatpoint(x,y), v, 0, 1, 2, 3);
				}
			}
			//-----------when direction is page based, not mesh based:----------------------
			// *** TO DO
			if (!edata) for (int x=win_w/2-ww/2; x<win_w/2+ww/2; x+=step) {
				for (int y=win_h/2-hh/2; y<win_h/2+hh/2; y+=step) {
					p.x=x-(win_w/2-ww/2);
					p.y=y-(win_h/2-hh/2);
					p=dp->screentoreal(p.x,p.y);

					//DBG cerr <<int(xx)<<','<<int(yy)<<"  ";
					v=map->Direction(p.x,p.y);
					p2=dp->realtoscreen(p+v);
					v=p2-flatpoint(x,y);
					vv=norm(v);
					if (vv>step*.8) v*=step*.8/vv;
					if (vv<step*.5) v*=step*.5/vv;


					dp->drawarrow(flatpoint(x,y), v, 0, 1, 2, 3);
				}
			}
			//---------------------------------
			dp->DrawReal();
		}
	} //end show Direction Map


	 //draw other tool decorations
	dp->LineAttributes(-1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	dp->LineWidthScreen(1);

	 //always draw outline of mesh
	if (data->npoints_boundary) {
		if (always_warp) {
			dp->NewFG(150,150,150);
			dp->LineAttributes(-1, LineSolid, linestyle.capstyle,linestyle.joinstyle);
		} else {
			dp->NewFG(255,255,255);
			dp->LineAttributes(-1, LineOnOffDash, linestyle.capstyle,linestyle.joinstyle);
		}
		dp->LineWidthScreen(1);
		dp->drawbez(data->boundary_outline,data->npoints_boundary/3,1,0);
		dp->LineAttributes(-1,LineSolid,linestyle.capstyle,linestyle.joinstyle);
	}

	if (mode==EMODE_Freehand) {
		 //awaiting a click down
		if (!child && show_panel) DrawPanel();
		needtodraw=0;
		return 0;
	}

	if (mode==EMODE_Mesh) {
		if (showdecs) {
			dp->DrawScreen();
			if (always_warp) {
				dp->NewFG(0.,.78,0.);
				dp->textout(0,0, "Warp",-1, LAX_TOP|LAX_LEFT);
			} else {
				dp->NewFG(.9,0.,0.);
				dp->textout(0,0, "Don't Warp",-1, LAX_TOP|LAX_LEFT);
			}
			dp->DrawReal();
		}

		PatchInterface::Refresh();
		if (data->npoints_boundary) {
			if (always_warp) {
				dp->NewFG(150,150,150);
				dp->LineAttributes(-1, LineSolid, linestyle.capstyle,linestyle.joinstyle);
			} else {
				dp->NewFG(255,255,255);
				dp->LineAttributes(-1, LineOnOffDash, linestyle.capstyle,linestyle.joinstyle);
			}
			dp->LineWidthScreen(1);
			dp->drawbez(data->boundary_outline,data->npoints_boundary/3,1,0);
			dp->LineAttributes(-1,LineSolid,linestyle.capstyle,linestyle.joinstyle);
		}

		if (dynamic_cast<PathInterface*>(child)) {
			 //due to defered refreshing, so we can draw path between mesh and panel
			PathInterface *pathi=dynamic_cast<PathInterface*>(child);
			pathi->needtodraw=1;
			pathi->Setting(PATHI_Defer_Render, false);
			pathi->Refresh();
			pathi->Setting(PATHI_Defer_Render, true);
		}


	} else if (mode==EMODE_Orientation) {
		 //draw a burin
		DrawOrientation(lasthover);

		 //draw starter points for growth mode
		if (grow_lines && growpoints.n) {
			dp->DrawScreen();

			flatpoint p;
			for (int c=0; c<growpoints.n; c++) {
				p=growpoints.e[c]->p;
				p=edata->getPoint(p.x,p.y, false);
				p=dp->realtoscreen(p);

				if (growpoints.e[c]->godir&1) dp->NewFG(255,100,100); //should use activate/deactivate colors
				else dp->NewFG(0,200,0);
				dp->drawthing(p.x+5,p.y, 5,5, 1, THING_Triangle_Right);

				if (growpoints.e[c]->godir&2) dp->NewFG(255,100,100); //should use activate/deactivate colors
				else dp->NewFG(0,200,0);
				dp->drawthing(p.x-5,p.y, 5,5, 1, THING_Triangle_Left);
			}

			dp->DrawReal();
		}

	} else if ((mode==EMODE_Thickness
			|| mode==EMODE_Blockout
			|| mode==EMODE_Drag
			|| mode==EMODE_Turbulence
			|| mode==EMODE_PushPull
		    || mode==EMODE_AvoidToward
		    || mode==EMODE_Twirl)
			&& lasthovercategory!=ENGRAVE_Panel
			) {


		dp->DrawScreen();
		dp->LineWidthScreen(1);

		if (!(submode==2 && lasthover==ENGRAVE_Sensitivity)) {

			 //set colors
			dp->NewFG(.5,.5,.5,1.);
			if (mode==EMODE_Thickness) {
				dp->LineWidthScreen(2);
				dp->NewFG(.5,.5,.5,1.);

			} else if (mode==EMODE_Turbulence) dp->NewFG(.5,.5,.5,1.);

			else if (   mode==EMODE_Drag
					 || mode==EMODE_PushPull
					 || mode==EMODE_Twirl) {
				if (submode==2) dp->NewFG(.5,.5,.5); //brush size change
				else dp->NewFG(0.,0.,.7,1.);

			} else if (mode==EMODE_Blockout) { //blockout
				if (submode==1) dp->NewFG(0,200,0);
				else if (submode==2) dp->NewFG(.5,.5,.5);
				else dp->NewFG(255,100,100);
			}

			 //draw main circle
			if (mode==EMODE_Turbulence) {
				 //draw jagged circle
				double xx,yy, r;
				for (int c=0; c<30; c++) {
					r=1 + .2*drand48()-.1;
					xx=hover.x + brush_radius*r*cos(c*2*M_PI/30);
					yy=hover.y + brush_radius*r*sin(c*2*M_PI/30);
					dp->lineto(xx,yy);
				}
				dp->closed();
				dp->stroke(0);

			} else if (mode==EMODE_Twirl) {
				int s=1;
				if (submode==1) s=-1;
				for (int c=0; c<10; c++) {
					dp->moveto(hover.x+brush_radius*cos(s*c/10.*2*M_PI), hover.y+brush_radius*sin(s*c/10.*2*M_PI));
					dp->curveto(flatpoint(hover.x+brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+brush_radius*sin(s*(c+1)/10.*2*M_PI)),
								flatpoint(hover.x+.85*brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+.85*brush_radius*sin(s*(c+1)/10.*2*M_PI)),
								flatpoint(hover.x+.85*brush_radius*cos(s*(c+1)/10.*2*M_PI), hover.y+.85*brush_radius*sin(s*(c+1)/10.*2*M_PI)));
					dp->stroke(0);
				}

			} else if (mode==EMODE_PushPull) {
				dp->drawpoint(hover.x,hover.y, brush_radius,0);

				dp->LineAttributes(-1,LineOnOffDash, LAXCAP_Butt, LAXJOIN_Miter);
				dp->LineWidthScreen(1);
				if (submode==1) dp->drawpoint(hover.x,hover.y, brush_radius*.85,0);
				else dp->drawpoint(hover.x,hover.y, brush_radius*1.10,0);
				dp->LineAttributes(-1,LineSolid, LAXCAP_Butt, LAXJOIN_Miter);

			} else {
				 //draw plain old circle
				dp->drawpoint(hover.x,hover.y, brush_radius,0);
			}

			 //draw circle decorations
			if (mode==EMODE_Drag) {
				dp->drawarrow(hover,flatpoint(brush_radius/4,0), 0,1,2,3);
				dp->drawarrow(hover,flatpoint(-brush_radius/4,0), 0,1,2,3);
				dp->drawarrow(hover,flatpoint(0,brush_radius/4), 0,1,2,3);
				dp->drawarrow(hover,flatpoint(0,-brush_radius/4), 0,1,2,3);

			} else if (mode==EMODE_Blockout) {
				dp->drawpoint(hover.x,hover.y, brush_radius*.85,0); //second inner circle

			} else if (mode==EMODE_AvoidToward) {
				flatpoint vt(-hoverdir.y,hoverdir.x);
				vt.normalize();
				vt*=brush_radius/2;

				if (submode==1) {
					dp->drawarrow(hover+3*vt,-vt, 0,1,2,3);
					dp->drawarrow(hover-3*vt, vt, 0,1,2,3);
				} else {
					dp->drawarrow(hover+vt,vt, 0,1,2,3);
					dp->drawarrow(hover-vt,-vt, 0,1,2,3);
				}
			}
		} //if needed to draw brush circles

		dp->LineAttributes(-1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->LineWidthScreen(1);
		if (submode==2) {
			 //brush size change arrows
			if (lasthover!=ENGRAVE_Sensitivity) {
				dp->drawarrow(hover+flatpoint(brush_radius+10,0), flatpoint(20,0), 0, 20, 1, 3);
				dp->drawarrow(hover-flatpoint(brush_radius+10,0), flatpoint(-20,0), 0, 20, 1, 3);
			}

			 //sensitivity slider
			double sens=1;
			if (mode==EMODE_Thickness)        sens= sensitive_thickness;
			else if (mode==EMODE_Drag)        sens= sensitive_drag;
			else if (mode==EMODE_Turbulence)  sens= sensitive_turbulence;
			else if (mode==EMODE_PushPull)    sens= sensitive_pushpull;
		    else if (mode==EMODE_AvoidToward) sens= sensitive_avoidtoward;
		    else if (mode==EMODE_Twirl)       sens= sensitive_twirl;

			dp->NewFG(&bgcolor);
			//dp->drawellipse(100,100,50,50, 0,0,1);
			dp->drawellipse(sensbox.x+sensbox.width/2,sensbox.y+sensbox.height/2, sensbox.width/2,2*dp->textheight()+5+sensbox.height/2, 0,0,1);
			dp->NewFG(&fgcolor);
			DrawSlider(sens/2, lasthover==ENGRAVE_Sensitivity, sensbox.x,sensbox.y,sensbox.width,sensbox.height, NULL);
			dp->textout(sensbox.x+sensbox.width/2,sensbox.y-5, _("Sensitivity"),-1, LAX_HCENTER|LAX_BOTTOM); 
			char buffer[30];
			sprintf(buffer,"%.2f",sens);
			dp->textout(sensbox.x+sensbox.width/2,sensbox.y+sensbox.height+5, buffer,-1, LAX_HCENTER|LAX_TOP);
		}

		dp->DrawReal();
	}

	if (show_panel) DrawPanel();

	needtodraw=0;
	return 0;
}

/*! Called from Refresh, draws the orientation handle.
 */
void EngraverFillInterface::DrawOrientation(int over)
{
	 //note: need to coordinate with scanPanel()

	 //the burin occupies 2*xx by 1.5*yy

	EngraverPointGroup *group=edata->GroupFromIndex(current_group);

	 //draw a burin
	flatpoint center=edata->getPoint(group->position.x,group->position.y, false);
	center=dp->realtoscreen(center);

	int size=30;
	double thick=.25; //thickness of shaft, percentage of size

	flatpoint dir=group->directionv/norm(group->directionv)*.05;
	flatpoint xx=dp->realtoscreen(edata->getPoint(.5+dir.x,.5+dir.y, false)) - dp->realtoscreen(edata->getPoint(.5,.5, false));
	xx=xx/norm(xx)*size;
	flatpoint yy=-transpose(xx);

	dp->DrawScreen();

	 //draw shaft of burin
	dp->moveto(center + yy);
	dp->lineto(center + yy/2);
	dp->curveto(center + .225*yy, center + .225*xx, center+xx/2);
	dp->lineto(center +2*xx);
	dp->lineto(center + (2-thick)*xx + thick*yy);
	dp->lineto(center + 2*thick*xx   + thick*yy);
	dp->curveto(center + (1.5*thick)*xx + thick*yy, center + thick*xx+1.5*thick*yy, center + thick*xx + 2*thick*yy);
	dp->lineto(center + thick*xx + yy);
	dp->closed();

	dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	if (over==ENGRAVE_Orient_Position) dp->NewFG(.8,.8,.8); else dp->NewFG(1.,1.,1.);
	dp->fill(1);
	dp->NewFG(0.,0.,.6);
	dp->stroke(0);

	 //draw knob of burin
	if (over==ENGRAVE_Orient_Spacing)  dp->NewBG(.8,.8,.8); else dp->NewBG(1.,1.,1.);
	dp->drawpoint(center+thick/2*xx+yy, norm(xx)/3, 2);
	
	 //draw rotate indicator
	if (over==ENGRAVE_Orient_Direction) {
		dp->moveto(center + 2*xx + thick*yy);
		dp->curveto(center + (2+thick/2)*xx + thick/2*yy,
					center + (2+thick/2)*xx - thick/2*yy,
					center + 2*xx - thick*yy);

		dp->moveto(center + 2*xx + .9*thick*yy);
		dp->lineto(center + 2*xx + thick*yy);
		dp->lineto(center + 2.1*xx + thick*yy);

		dp->moveto(center + 2*xx - .9*thick*yy);
		dp->lineto(center + 2*xx - thick*yy);
		dp->lineto(center + 2.1*xx - thick*yy);

		dp->NewFG(.8,.8,.8);
		dp->LineAttributes(4,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->stroke(1);

		dp->NewFG(0.,0.,.6);
		dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->stroke(0);
	}
	
	 //draw quick adjust
	if (over==ENGRAVE_Orient_Quick_Adjust) {
		dp->NewFG(0.,0.,.6);
		dp->moveto(center + 2.5*xx + thick/2*yy);
		dp->lineto(center + 3.5*xx + thick/2*yy);
		dp->lineto(center + 3.5*xx + -thick/2*yy);
		dp->lineto(center + 2.5*xx + -thick/2*yy);
		dp->closed();
		dp->fill(1);
		dp->NewFG(.8,.8,.8);
		dp->stroke(0);
	}

	 //draw extra parameters
	int i=1;
	for (int c=0; c<group->direction->parameters.n; c++) {
		if (group->direction->parameters.e[c]->dtype!=group->direction->type) continue;
		
		EngraverDirection::Parameter *p=group->direction->parameters.e[c];
		dp->moveto(center-xx-i*yy*.5);
		dp->lineto(center+xx-i*yy*.5);
		dp->NewFG(.8,.8,.8);
		dp->LineAttributes(3,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->stroke(1);
		dp->NewFG(0.,0.,.6);
		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->stroke(0);

		if (p->type=='b') {
			 //boolean
			if (p->value==0) {
				dp->moveto(center-xx-(i-.3)*yy*.5);
				dp->lineto(center-xx/2   -i*yy*.5);
				dp->lineto(center-xx-(i+.3)*yy*.5);
			} else {
				dp->moveto(center+xx-(i-.3)*yy*.5);
				dp->lineto(center+xx/2   -i*yy*.5);
				dp->lineto(center+xx-(i+.3)*yy*.5);
			}
			dp->closed();
			dp->NewFG(.8,.8,.8);
			dp->LineAttributes(3,LineSolid,LAXCAP_Round,LAXJOIN_Round);
			dp->stroke(1);
			dp->NewFG(0.,0.,.6);
			dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
			dp->fill(0);

		} else { //is integer or real
			double pos=(p->value-p->min)/(p->max-p->min);
			if (pos>1) pos=1; else if (pos<0) pos=0;
			dp->NewFG(.8,.8,.8);
			dp->drawpoint(center-xx+pos*2*xx-i*yy*.5, 5, 1);
			dp->NewFG(0.,0.,.6);
			dp->drawpoint(center-xx+pos*2*xx-i*yy*.5, 4, 1);
		}

		i++;
	}

	 //draw keep old 

	dp->NewFG(&fgcolor);
	dp->DrawReal();
}

void EngraverFillInterface::DrawCheckBox(int on, int hovered, double x,double y,double w,double h, const char *text)
{
	if (hovered)  dp->NewFG(0.,0.,0.); 
	else dp->NewFG(.5,.5,.5);

	dp->drawrectangle(x,y+h/2-h*.7/2, h*.7,h*.7, 0);
	if (on) dp->drawthing(x+h/2,y+h/2,h/2,-h/2, 1, THING_Check);
	dp->textout(x+h,y+h/2, text,-1, LAX_LEFT|LAX_VCENTER);
}

void EngraverFillInterface::DrawNumInput(double pos,int type,int hovered, double x,double y,double w,double h, const char *text)
{
	dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
	dp->LineWidthScreen(1);
 	
	if (hovered)  dp->NewFG(0.,0.,0.); 
	else dp->NewFG(.5,.5,.5);

	double th=dp->textheight();

	if (text) {
		//if (pos<.5) dp->textout(x+w,y+h/2, text,-1, LAX_RIGHT|LAX_VCENTER);
		//else dp->textout(x,y+h/2, text,-1, LAX_LEFT|LAX_VCENTER);
		
		dp->textout(x+w-th-th/2,y+h/2, text,-1, LAX_RIGHT|LAX_VCENTER);
	}

	char str[20];
	if (type==0) sprintf(str,"%.2g", pos);
	else sprintf(str,"%d", (int)pos);
	dp->textout(x+th+th/2,y+h/2, str,-1, LAX_LEFT|LAX_VCENTER);

    dp->drawthing(x+th*.9,y+h/2, th*.3,th*.3, 0, THING_Triangle_Left);
    dp->drawthing(x+w-th*.9,y+h/2, th*.3,th*.3, 0, THING_Triangle_Right);
}

void EngraverFillInterface::DrawSlider(double pos,int hovered, double x,double y,double w,double h, const char *text)
{
	dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
	dp->LineWidthScreen(1);
 	
	dp->NewFG(.5,.5,.5);

	double ww=(text ? dp->textextent(text,-1, NULL,NULL) : 0);
	if (text) {
		//if (pos<.5) dp->textout(x+w,y+h/2, text,-1, LAX_RIGHT|LAX_VCENTER);
		//else dp->textout(x,y+h/2, text,-1, LAX_LEFT|LAX_VCENTER);
		
		dp->textout(x+w/2,y+h/2, text,-1, LAX_CENTER);
	}
	if (hovered)  dp->NewFG(0.,0.,0.); 

	if (ww>0) {
		dp->drawline(x, y+h/2, x+w/2-ww/2, y+h/2);
		dp->drawline(x+w/2+ww/2, y+h/2, x+w, y+h/2);
	} else dp->drawline(x, y+h/2, x+w, y+h/2);

	if (pos>1) dp->drawthing(x+w-h/4,y+h/2, h*.4,h*.4, 1, THING_Triangle_Right);
	else if (pos<0) dp->drawthing(x+h/4,y+h/2, h*.4,h*.4, 1, THING_Triangle_Left);
	else dp->drawpoint(flatpoint(x + h/4 + pos*(w-h/2),y+h/2), h*.4, 1);
}

/*! Draw the range of line widths in a strip, as for a curve map control.
 * Assumes already in dp->DrawScreen mode.
 */
void EngraverFillInterface::DrawLineGradient(double minx,double maxx,double miny,double maxy, int groupnum, int horizontal)
{
	double num=15;
	double sp=(maxy - miny)/num; //one unit
	if (horizontal) sp=(maxx-minx)/num;

	EngraverPointGroup *group=edata ? edata->GroupFromIndex(groupnum) : NULL;

	dp->NewFG(&fgcolor);

	double pos, rr,t, w, a, p, gapw;
	double zero   =(group ? group->dashes->zero_threshhold  /group->spacing->spacing : 0);
	double broken =(group ? group->dashes->broken_threshhold/group->spacing->spacing : 0);
	double density=(group ? group->dashes->dash_density : 0);
	double sw;

	for (int c=0; c<num; c++) {
		pos=c/num;
		if (pos<zero) continue;


		if (group && pos*group->spacing->spacing<group->dashes->broken_threshhold) {
			 //draw dashed
			rr    =group->dashes->dash_randomness;
			t     =group->dashes->dash_taper;

			a=(pos-zero)/(broken-zero);
			w=broken*a + (t*(broken-zero)+zero)*(1-a);

			a=density + (1-density)*a;
			gapw=1-a;
			p=.5 + (rr*random()/RAND_MAX - rr/2);

			sw=sp*w;
			if (horizontal) {
				gapw*=(maxy-miny);
				p*=maxy-miny-gapw;
				dp->drawrectangle(minx + (maxx-minx)*pos, miny,         sw>=1?sw:1, p, 1);
				dp->drawrectangle(minx + (maxx-minx)*pos, miny+p+gapw,  sw>=1?sw:1, (maxy-miny)-(p+gapw), 1);

			} else {
				gapw*=(maxx-minx);
				p*=maxx-minx-gapw;
				//dp->drawrectangle(minx,        miny + (maxy-miny)*pos,  maxx-minx,                    1+sp*w, 1);
				dp->drawrectangle(minx,        maxy - (maxy-miny)*pos,  p,                    sw>=1?sw:1, 1);
				dp->drawrectangle(minx+p+gapw, maxy - (maxy-miny)*pos,  (maxx-minx)-(p+gapw), sw>=1?sw:1, 1);
			}

		} else {
			 //draw solid
			if (horizontal) {
				dp->drawrectangle(minx + (maxx-minx)*pos, miny, 1+sp*pos,maxy-miny, 1);
			} else {
				dp->drawrectangle(minx, maxy - (maxy-miny)*pos, maxx-minx,sp*pos, 1);
			}
		}
	}

	//dp->drawrectangle(minx-1,miny-1, maxx-minx+2,maxy-miny+2,0);
}

/*! Draw a continuous tone gradient in a strip, as for a curve map control.
 * Assumes already in dp->DrawScreen mode.
 */
void EngraverFillInterface::DrawShadeGradient(double minx,double maxx,double miny,double maxy)
{
	ScreenColor col;

	dp->LineAttributes(-1, LineSolid, CapButt, JoinMiter);
	dp->LineWidthScreen(2);
	for (int c=minx; c<maxx; c+=2) {
		dp->NewFG(coloravg(rgbcolor(0,0,0),rgbcolor(255,255,255), 1-(c-minx)/(maxx-minx)));
		dp->drawline(c,miny, c,maxy);
	}

	//dp->drawrectangle(minx-1,miny-1, maxx-minx+2,maxy-miny+2,0);
}

/*! Define the panel, and update positions within it.
 */
void EngraverFillInterface::UpdatePanelAreas()
{
	DBG cerr <<" EngraverFillInterface::UpdatePanelAreas()..."<<endl;

	if (!panel.n()) {
		 //need to define all the panel areas
		panel.AddItem("Mode Selection",  ENGRAVE_Mode_Selection);

		//--------------- Group selection ---------------
		 // < >  _Group_Name__  [del] [new]
		 // [new] [delete]  Active: 0  [color]
		panel.AddItem("Group Selection",  ENGRAVE_Groups);
		panel.SubMenu();
		  //panel.AddItem("Toggle list" ,    ENGRAVE_Toggle_Group_List);
		  panel.AddItem("Previous Group",  ENGRAVE_Previous_Group);
		  panel.AddItem("Next Group",      ENGRAVE_Next_Group);
		  panel.AddItem("Group List",      ENGRAVE_Group_List);
		  panel.AddItem("Group Name",      ENGRAVE_Group_Name);
		  panel.AddItem("Active",          ENGRAVE_Group_Active);
		  panel.AddItem("Linked",          ENGRAVE_Group_Linked);
		  panel.AddItem("Color",           ENGRAVE_Group_Color);
		  panel.AddItem("Delete Group",    ENGRAVE_Delete_Group);
		  panel.AddItem("New Group",       ENGRAVE_New_Group);
		  panel.AddItem("Merge Group",     ENGRAVE_Merge_Group);
		  panel.AddItem("Move Group Up",   ENGRAVE_Group_Up);
		  panel.AddItem("Move Group Down", ENGRAVE_Group_Down);
		panel.EndSubMenu();
		panel.e(panel.n()-1)->state|=LAX_OPEN; //this always needs to be open
		MenuItem *grouplist=panel.findid(ENGRAVE_Group_List);
		grouplist->Open();

		//--------------- tracing  ---------------
		panel.AddItem("Tracing",    ENGRAVE_Tracing);
		panel.SubMenu();
		  panel.AddItem("Trace sharing",    ENGRAVE_Trace_Same_As);
		  panel.AddItem("Trace sharing",    ENGRAVE_Trace_Menu);
		  panel.AddItem("Trace Name",       ENGRAVE_Trace_Name);
		  panel.AddItem("Trace Curve",      ENGRAVE_Trace_Curve);
		  panel.AddItem("Trace Line Bar",   ENGRAVE_Trace_Curve_Line_Bar);
		  panel.AddItem("Trace Value Bar",  ENGRAVE_Trace_Curve_Value_Bar);
		  panel.AddItem("Thicken",          ENGRAVE_Trace_Thicken);
		  panel.AddItem("Thin",             ENGRAVE_Trace_Thin);
		  panel.AddItem("Set or multiply",  ENGRAVE_Trace_Set);
		  panel.AddItem("Using type",       ENGRAVE_Trace_Using_type);
		  panel.AddItem("Using",            ENGRAVE_Trace_Using);
		  panel.AddItem("Apply",            ENGRAVE_Trace_Apply); //<- same as Once?
		  panel.AddItem("Remove",           ENGRAVE_Trace_Remove);
		  panel.AddItem("Opacity",          ENGRAVE_Trace_Opacity);
		  panel.AddItem("Trace Once",       ENGRAVE_Trace_Once);
		  panel.AddItem("Trace Continuous", ENGRAVE_Trace_Continuous);
		  panel.AddItem("Trace Object",     ENGRAVE_Trace_Object);
		  panel.AddItem("Trace Object Name",ENGRAVE_Trace_Object_Name);
		  panel.AddItem("Trace Object Menu",ENGRAVE_Trace_Object_Menu);
		panel.EndSubMenu();
		//panel.e(panel.n()-1)->state|=LAX_OPEN;

		//--------------- Dashes  ---------------
		panel.AddItem("Dashes",     ENGRAVE_Dashes);
		panel.SubMenu();
		  panel.AddItem("Dash sharing",    ENGRAVE_Dash_Same_As);
		  panel.AddItem("Dash sharing",    ENGRAVE_Dash_Menu);
		  panel.AddItem("Dash name",       ENGRAVE_Dash_Name);
		  panel.AddItem("Zero Threshhold", ENGRAVE_Dash_Zero_Threshhold);
		  panel.AddItem("Dash Threshhold", ENGRAVE_Dash_Broken_Threshhold);
		  panel.AddItem("Random",          ENGRAVE_Dash_Random);
		  panel.AddItem("Taper",           ENGRAVE_Dash_Taper);
		  panel.AddItem("Minimum density", ENGRAVE_Dash_Density);
		  panel.AddItem("Dash Length",     ENGRAVE_Dash_Length);
		  panel.AddItem("Random Seed",     ENGRAVE_Dash_Seed);
		  panel.AddItem("Caps",            ENGRAVE_Dash_Caps);
		  panel.AddItem("Join",            ENGRAVE_Dash_Join);
		panel.EndSubMenu();

		//--------------- Direction  ---------------
		panel.AddItem("Direction",  ENGRAVE_Direction);
		panel.SubMenu();
		  panel.AddItem("Direction sharing",          ENGRAVE_Direction_Same_As);
		  panel.AddItem("Direction sharing",          ENGRAVE_Direction_Menu);
		  panel.AddItem("Direction object name",      ENGRAVE_Direction_Name);
		  panel.AddItem("Show direction field",       ENGRAVE_Direction_Show_Dir);
		  panel.AddItem("Type",                       ENGRAVE_Direction_Type); 
		  panel.AddItem("Whether to automatically reline on changes",ENGRAVE_Direction_Reline); 
		  panel.AddItem("Default line profile",       ENGRAVE_Direction_Profile);
		  panel.AddItem("Default line profile",       ENGRAVE_Direction_Profile_Menu);
		  //panel.AddItem("Profile start",              ENGRAVE_Direction_Profile_Start);
		  //panel.AddItem("Profile end",                ENGRAVE_Direction_Profile_End);
		  //panel.AddItem("Stretch or scale profile",              ENGRAVE_Direction_Profile_Scale);
		  panel.AddItem("Profile max height",         ENGRAVE_Direction_Profile_Max_Height);
		  panel.AddItem("Random line offset",         ENGRAVE_Direction_Line_Offset);
		  panel.AddItem("Random point offset",        ENGRAVE_Direction_Point_Offset);
		  //panel.AddItem("Random point offset size",   ENGRAVE_Direction_Point_Off_Size);
		  panel.AddItem("Grow lines",                 ENGRAVE_Direction_Grow);
		  panel.AddItem("Fill with grown lines",      ENGRAVE_Direction_Fill);
		  panel.AddItem("Merge lines",                ENGRAVE_Direction_Merge);
		  panel.AddItem("Spread amount",              ENGRAVE_Direction_Spread);
		  panel.AddItem("Spread depth",               ENGRAVE_Direction_Spread_Depth);
		  panel.AddItem("Angle threshhold to spread", ENGRAVE_Direction_Spread_Angle);
		  panel.AddItem("Random seed",                ENGRAVE_Direction_Seed);
		panel.EndSubMenu();

		//--------------- Spacing  ---------------
		panel.AddItem("Spacing",    ENGRAVE_Spacing);
		panel.SubMenu();
		  panel.AddItem("Spacing sharing",      ENGRAVE_Spacing_Same_As);
		  panel.AddItem("Spacing sharing",      ENGRAVE_Spacing_Menu);
		  panel.AddItem("Spacing name",         ENGRAVE_Spacing_Name);
		  panel.AddItem("Default spacing",      ENGRAVE_Spacing_Default);
		  //panel.AddItem("Use spacing map",      ENGRAVE_Spacing_Use_Map);
		  //panel.AddItem("Spacing map file",     ENGRAVE_Spacing_Map_File);
		  panel.AddItem("Spacing map",          ENGRAVE_Spacing_Map);
		  panel.AddItem("Spacing map menu",     ENGRAVE_Spacing_Map_Menu);
		  panel.AddItem("Preview",              ENGRAVE_Spacing_Preview);
		  panel.AddItem("Create from current",  ENGRAVE_Spacing_Create_From_Current);
		  panel.AddItem("Load..",               ENGRAVE_Spacing_Load);
		  panel.AddItem("Save..",               ENGRAVE_Spacing_Save);
		  panel.AddItem("Paint..",              ENGRAVE_Spacing_Paint);
		panel.EndSubMenu();

		  //--------------- Selection?  ---------------
		  // //selection management, 1 line for each selection, click x to remove?
		  //panel.AddItem("Spacing",    ENGRAVE_Spacing);
	}

	double th=dp->textheight();
	double pad=th/2;

	//now update positions
	if (!panelbox.validbounds()) {
		panelbox.minx=panelbox.miny=10;
		panelbox.maxx=8*1.5*th;
		panelbox.maxy=10*1.5*th+th+4./3*th+th+pad;
	}

	MenuItem *item, *item2;
	int y=pad;
	int pw=panelbox.boxwidth();
	int sidew=2*th; //for trace line side bar
	if (sidew>pw*.2) sidew=pw*.2;
	//int ph=panelbox.boxheight();
	//int px=panelbox.minx;

	for (int c=0; c<panel.n(); c++) {
		item=panel.e(c);
		item->y=y;

		if (item->id==ENGRAVE_Mode_Selection) {
			item->x=pad;  item->y=y;  item->w=pw-2*pad;
			if (modes.e(0)->image) item->h=modes.e(0)->image->h(); else item->h=th;
			y+=item->h+pad;

		} else if (item->id==ENGRAVE_Groups) {
			item->x=pad; item->y=y; item->w=pw-2*pad;  item->h=th;

			item2=panel.findid(ENGRAVE_Group_List);
			int lh=0;
			bool showlist=false;
			if (item2->isOpen()) {
				lh=th*NumGroupLines();
				if (lh==0) lh=th;
				showlist=true;
			} else lh=th;
			item->h+=lh;

			int nww=5;

			//< > Groupname  active linked color
			//=   +  -  ^  v  ->
			//-------------
			//--visible - objectid----
			//active color linked   group 1
			//active color linked   group 2
			//active color linked   group 3
			//+  -  ^  v  dup

			for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
				item2=item->GetSubmenu()->e(c2);

				//----first line
				if (!showlist) { //first line is abreviated group indicator, not whole list
					if (item2->id==ENGRAVE_Previous_Group) {
						item2->x=pad;  item2->y=y;  item2->w=th; item2->h=th;

					} else if (item2->id==ENGRAVE_Next_Group) {
						item2->x=pad+th;  item2->y=y;  item2->w=th; item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Name) {
						item2->x=pad+2*th;  item2->y=y;  item2->w=pw-2*pad-5*th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Active) {
						item2->x=pw-pad-3*th;  item2->y=y;  item2->w=th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Linked) {
						item2->x=pw-pad-2*th;  item2->y=y;  item2->w=th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Group_Color) {
						item2->x=pw-pad-th;  item2->y=y;  item2->w=th;  item2->h=th;
					}

				} else {
					if (       item2->id==ENGRAVE_Previous_Group
							|| item2->id==ENGRAVE_Next_Group
							|| item2->id==ENGRAVE_Group_Name
							|| item2->id==ENGRAVE_Group_Active
							|| item2->id==ENGRAVE_Group_Linked
							|| item2->id==ENGRAVE_Group_Color) 
						item2->w=item2->h=0;
				}


				//----group list, if any
				if (item2->id==ENGRAVE_Group_List) {
					if (showlist) {
						item2->x=pad; item2->y=y; item2->w=pw-2*pad; item2->h=lh;
					} else {
						item2->x=pad; item2->y=y+th; item2->w=-1; item2->h=-1;
					}


					//----second line
				} else if (item2->id==ENGRAVE_Toggle_Group_List) {
					item2->x=pad;     item2->y=y+lh;  item2->w=th; item2->h=th;

				} else if (item2->id==ENGRAVE_New_Group) {
					item2->x=th+pad;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				} else if (item2->id==ENGRAVE_Delete_Group) {
					item2->x=th+pad+item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				} else if (item2->id==ENGRAVE_Group_Down) {
					item2->x=th+pad+2*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				} else if (item2->id==ENGRAVE_Group_Up) {
					item2->x=th+pad+3*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww; item2->h=th;

				//} else if (item2->id==ENGRAVE_Duplicate_Group) {
				//	item2->x=th+pad+4*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww, item2->h=th;

				//} else if (item2->id==ENGRAVE_Merge_Group) {
				//	item2->x=th+pad+5*item->w/nww;  item2->y=y+lh;  item2->w=(item->w-th)/nww, item2->h=th;

				}
			}
			y+=item->h; 

		} else if (item->id==ENGRAVE_Tracing) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!item->isOpen()) {
				item->h=th;
				DBG cerr <<" ---tracing section is CLOSED"<<endl;

			} else {
				DBG cerr <<" ---tracing section is OPEN"<<endl;

				item->h=(item->w-2*th) +th +4*th/3 +th +th +th +pad; //+header 1st []O valuebar opacity tobject

				int sharing=IsSharing(ENGRAVE_Tracing, NULL, current_group);

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					//----first line
					if (item2->id==ENGRAVE_Trace_Same_As) {
						item2->x=pad;  item2->y=y+1*th;  item2->w=pw-2*pad;  item2->h=th; 
						if (sharing) {
							item2->w=item2->x+dp->textextent(_("With:"),-1,NULL,NULL);
						}

					} else if (item2->id==ENGRAVE_Trace_Name) {
						if (sharing) {
							item2->x=pad+dp->textextent(_("With:"),-1,NULL,NULL);  item2->y=y+1*th;  item2->w=pw-pad-item2->x-th;  item2->h=th;
						} else {
							item2->x=item2->y=item2->w=item2->h=0;
						}

					} else if (item2->id==ENGRAVE_Trace_Menu) {
						item2->x=pw-pad-th;  item2->y=y+1*th;  item2->w=th;  item2->h=th;

					//-----second line,  [] O
					} else if (item2->id==ENGRAVE_Trace_Once) {
						item2->x=pad;  item2->y=y+2*th;  item2->w=pw/2-pad;  item2->h=2+th*4/3;

					} else if (item2->id==ENGRAVE_Trace_Continuous) {
						item2->x=pw/2;  item2->y=y+2*th;  item2->w=pw/2-pad;  item2->h=2+th*4/3;

					//-----curve area (3rd line),  (line bar)(curve area)
					} else if (item2->id==ENGRAVE_Trace_Curve) {
						//item2->x=pad;  item2->y=y+(hasgroups?2:1)*th;  item2->w=pw-2*pad;  item2->h=item->w+4*th; 
						item2->x=pad+sidew;  item2->y=y+2*th+4*th/3;  item2->w=pw-2*pad-sidew;
						item2->h=item->h -th -th -4*th/3 -th -th -th -pad + 1; // - header 1st []O valuebar opacity tobject
						tracebox=item2;

					} else if (item2->id==ENGRAVE_Trace_Curve_Line_Bar) {
						item2->x=pad;  item2->y=y+2*th+4*th/3;  item2->w=sidew;
						item2->h=item->h -th -th -4*th/3-2 -th -th -th -pad; // - header 1st []O set opacity tobject

					} else if (item2->id==ENGRAVE_Trace_Curve_Value_Bar) {
						item2->x=pad+2*th;  item2->y=y+item->h-3*th-pad;  item2->w=pw-2*pad-2*th; item2->h=th;

					} else if (item2->id==ENGRAVE_Trace_Set) { //bottom left corner of curve area
						item2->x=pad;  item2->y=y+item->h-3*th-pad;  item2->w=2*th; item2->h=th;

					} else if (item2->id==ENGRAVE_Trace_Opacity) {
						item2->x=pad;  item2->y=y+item->h-2*th-pad;  item2->w=pw-2*pad; item2->h=th;

					} else if (item2->id==ENGRAVE_Trace_Object_Name) {
						item2->x=pad;        item2->y=y+item->h-th-pad;  item2->w=pw-2*pad-th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Trace_Object_Menu) {
						item2->x=pw-pad-th;  item2->y=y+item->h-th-pad;  item2->w=th; item2->h=th;

						//-------------
						//ENGRAVE_Trace_Box, //in the curve box
						//ENGRAVE_Trace_Load,
						//ENGRAVE_Trace_Clear,
						//ENGRAVE_Trace_Object,
						//ENGRAVE_Trace_Object_Menu,
						//ENGRAVE_Trace_Curve,
						//ENGRAVE_Trace_Thicken,
						//ENGRAVE_Trace_Using_type,
						//ENGRAVE_Trace_Using,
						//ENGRAVE_Trace_Apply,
						//ENGRAVE_Trace_Remove,

						 //menu options, not panel areas:
						//ENGRAVE_Trace_Snapshot,
						//-------------------

					}
				}
			}

			y+=item->h;

		} else if (item->id==ENGRAVE_Dashes) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!(item->state&LAX_OPEN)) item->h=th;
			else {
				//int hasgroups = (edata && edata->groups.n>1 ? 1 : 0);
				//if (!hasgroups && selection && selection->n()>1) hasgroups=1;

				//item->h=8*th+pad+(hasgroups ? th : 0);
				item->h=8*th+pad+th;
				int sharing=IsSharing(ENGRAVE_Dashes, NULL, current_group);

				// ...
				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					//----first line
					if (item2->id==ENGRAVE_Dash_Same_As) {
						item2->x=pad;  item2->y=y+1*th;  item2->w=pw-2*pad;  item2->h=th; 
						if (sharing) {
							item2->w=dp->textextent(_("With:"),-1,NULL,NULL);
						}

					} else if (item2->id==ENGRAVE_Dash_Name) {
						if (sharing) {
							item2->x=pad+dp->textextent(_("With:"),-1,NULL,NULL);  item2->y=y+1*th;  item2->w=pw-pad-item2->x-th;  item2->h=th;
						} else {
							item2->x=item2->y=item2->w=item2->h=0;
						}

					} else if (item2->id==ENGRAVE_Dash_Menu) {
						item2->x=pw-pad-th;  item2->y=y+1*th;  item2->w=th;  item2->h=th;

					} else if (item2->id==ENGRAVE_Dash_Broken_Threshhold) {
						item2->x=pad;  item2->y=y+1*th+th;  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Zero_Threshhold) {
						item2->x=pad;  item2->y=y+2*th+th;  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Random) {
						item2->x=pad;  item2->y=y+3*th+th;  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Taper) {
						item2->x=pad;  item2->y=y+4*th+th;  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Density) {
						item2->x=pad;  item2->y=y+5*th+th;  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Length) {
						item2->x=pad;  item2->y=y+6*th+th;  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Seed) {
						item2->x=pad;  item2->y=y+7*th+th;  item2->w=pw-2*pad;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Dash_Caps) {
						// *** todo!
					} else if (item2->id==ENGRAVE_Dash_Join) {
						// *** todo!
					}
				}
			}

			y+=item->h;

		} else if (item->id==ENGRAVE_Spacing) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!(item->state&LAX_OPEN)) item->h=th;
			else {
				item->h=4*th;

				int sharing=IsSharing(ENGRAVE_Spacing, NULL, current_group);

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					//----first line
					if (item2->id==ENGRAVE_Spacing_Same_As) {
						item2->x=pad;  item2->y=y+1*th;  item2->w=pw-2*pad;  item2->h=th; 
						if (sharing) {
							item2->w=dp->textextent(_("With:"),-1,NULL,NULL);
						}

					} else if (item2->id==ENGRAVE_Spacing_Name) {
						if (sharing) {
							item2->x=pad+dp->textextent(_("With:"),-1,NULL,NULL);  item2->y=y+1*th;  item2->w=pw-pad-item2->x-th;  item2->h=th;
						} else {
							item2->x=item2->y=item2->w=item2->h=0;
						}

					} else if (item2->id==ENGRAVE_Spacing_Menu) {
						item2->x=pw-pad-th;  item2->y=y+1*th;  item2->w=th;  item2->h=th;

					 //----second line, default spacing
					} else if (item2->id==ENGRAVE_Spacing_Default) {
						item2->x=pad;  item2->y=y+2*th;  item2->w=pw-2*pad;  item2->h=th; 

					 //----third line, spacing map
					//} else if (item2->id==ENGRAVE_Spacing_Use_Map) {
						//item2->x=pad;  item2->y=y+3*th;  item2->w=th;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Spacing_Map) {
						item2->x=pad;  item2->y=y+3*th;  item2->w=pw-2*pad-th;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Spacing_Map_Menu) {
						item2->x=pw-pad-th;  item2->y=y+3*th;  item2->w=th;  item2->h=th; 
					}
				}
			}

			y+=item->h;

		} else if (item->id==ENGRAVE_Direction) {
			item->x=pad; item->y=y; item->w=pw-2*pad; 

			if (!(item->state&LAX_OPEN)) item->h=th;
			else {
				item->h=14*th;

				int sharing=IsSharing(ENGRAVE_Direction, NULL, current_group);

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);

					//----first line
					if (item2->id==ENGRAVE_Direction_Same_As) {
						item2->x=pad;  item2->y=y+1*th;  item2->w=pw-2*pad;  item2->h=th; 
						if (sharing) {
							item2->w=dp->textextent(_("With:"),-1,NULL,NULL);
						}

					} else if (item2->id==ENGRAVE_Direction_Name) {
						if (sharing) {
							item2->x=pad+dp->textextent(_("With:"),-1,NULL,NULL);  item2->y=y+1*th;  item2->w=pw-pad-item2->x-th;  item2->h=th;
						} else {
							item2->x=item2->y=item2->w=item2->h=0;
						}

					} else if (item2->id==ENGRAVE_Direction_Menu) {
						item2->x=pw-pad-th;  item2->y=y+1*th;  item2->w=th;  item2->h=th;

					 //----second line, line type
					} else if (item2->id==ENGRAVE_Direction_Show_Dir) {
						item2->x=pad;  item2->y=y+2*th;  item2->w=th;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Direction_Type) {
						item2->x=pad+th;  item2->y=y+2*th;  item2->w=pw-2*pad-th-1.33*th+1;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Direction_Reline) {
						item2->w=1.33*th;  item2->h=th; 
						item2->x=pw-item2->w-pad;  item2->y=y+2*th; 

					 //----third line, profile
					} else if (item2->id==ENGRAVE_Direction_Profile) {
						item2->x=pad;  item2->y=y+3*th;  item2->w=pw-2*pad-th;  item2->h=2*th; 

					} else if (item2->id==ENGRAVE_Direction_Profile_Menu) {
						item2->x=pw-pad-th;  item2->y=y+3*th;  item2->w=th;  item2->h=2*th;

					 //----fourth line, profile scaling
					//} else if (item2->id==ENGRAVE_Direction_Profile_Scale) {
					//	item2->x=pad;  item2->y=y+5*th;  item2->w=2*th;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Direction_Profile_Max_Height) {
						//item2->x=pad+2*th+pad;  item2->y=y+5*th;  item2->w=pw-3*pad-2*th;  item2->h=th; 
						item2->x=pad;  item2->y=y+5*th;  item2->w=pw-2*pad;  item2->h=th; 

					 //---fifth line
					} else if (item2->id==ENGRAVE_Direction_Line_Offset) {
						item2->x=pad;  item2->y=y+6*th;  item2->w=pw-2*pad;  item2->h=th; 

					 //---sixth line
					} else if (item2->id==ENGRAVE_Direction_Point_Offset) {
						//contains ENGRAVE_Direction_Point_Off_Size
						item2->x=pad;  item2->y=y+7*th;  item2->w=pw-2*pad;  item2->h=th; 

					 //--- 7th line
					} else if (item2->id==ENGRAVE_Direction_Grow) {
						item2->x=pad;  item2->y=y+8.5*th;  item2->w=pw-2*pad;  item2->h=th; 

					 //--- 8th line
					} else if (item2->id==ENGRAVE_Direction_Fill) {
						item2->x=pad;  item2->y=y+9.5*th;  item2->w=(pw-2*pad)/2;  item2->h=th; 

					} else if (item2->id==ENGRAVE_Direction_Merge) {
						item2->x=pad+(pw-2*pad)/2;  item2->y=y+9.5*th;  item2->w=(pw-2*pad)/2;  item2->h=th; 

					 //--- 9th line
					} else if (item2->id==ENGRAVE_Direction_Spread) {
						//contains ENGRAVE_Direction_Spread_Depth, ENGRAVE_Direction_Spread_Angle
						item2->x=pad;  item2->y=y+10.5*th;  item2->w=pw-2*pad;  item2->h=2*th; 

					} else if (item2->id==ENGRAVE_Direction_Seed) {
						item2->x=pad;  item2->y=y+12.5*th;  item2->w=pw-2*pad;  item2->h=th; 
					}
				}
			}

			y+=item->h;

		}
	} 

	panelbox.maxy=panelbox.miny+y+pad;

}

void EngraverFillInterface::DrawPanel()
{
	if (panel.n()==0) UpdatePanelAreas();

	dp->DrawScreen();
	dp->LineAttributes(1,LineSolid, CapButt, JoinMiter);

	ScreenColor col;
	coloravg(&col, &fgcolor,&bgcolor, .9);
	dp->NewBG(&col);
	dp->NewFG(coloravg(fgcolor.Pixel(),bgcolor.Pixel(), .5));
	dp->drawrectangle(panelbox.minx,panelbox.miny, panelbox.maxx-panelbox.minx,panelbox.maxy-panelbox.miny, 2);
	dp->NewFG(&fgcolor);

	double th=dp->textheight();
	double pad=th/2;
	int ix,iy,iw,ih;
	int i2x,i2y,i2w,i2h;
	unsigned long hcolor=coloravg(fgcolor.Pixel(),bgcolor.Pixel(), .8);
	//char buffer[200];

	EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);

	MenuItem *item, *item2;
	for (int c=0; c<panel.n(); c++) {
		item=panel.e(c);
		ix=item->x+panelbox.minx;
		iy=item->y+panelbox.miny;
		iw=item->w;
		ih=item->h;
		
		if (item->id==ENGRAVE_Mode_Selection) {
			int which=mode;
			if (lasthover>=EMODE_Mesh && lasthover<EMODE_MAX) which=lasthover;

			for (int c2=0; c2<modes.n(); c2++) {
				item2=modes.e(c2);

				if (which==item2->id) {
					if (item2->image) {
						int imw=item2->image->w(), imh=item2->image->h();
						double imxmin=ix+imw/2, imxmax=ix+iw-imw/2;
						double dist=(imxmax-imxmin)/(modes.n()-1); //regular distance between midpoints
						int sw=dist;
						if (sw>item2->image->h()) sw=item2->image->h();

						 //display smaller icons next to it with this one enlarged
						for (int c3=0; c3<c2; c3++) { //the ones to the left
							//dp->imageout(modes.e(c3)->image, imxmin+c3*dist-sw/2, iy+ih/2+sw/2, sw,-sw);
							dp->imageout(modes.e(c3)->image, imxmin+c3*dist-sw/2, iy+ih/2-sw/2, sw,sw);
						}
						for (int c3=modes.n()-1; c3>c2; c3--) { //the ones to the right
							dp->imageout(modes.e(c3)->image, imxmin+c3*dist-sw/2, iy+ih/2-sw/2, sw,sw);
							//dp->imageout(modes.e(c3)->image, imxmin+c3*dist-sw/2, iy+ih/2+sw/2, sw,-sw);
						}

						 //finally display actual one...
						dp->NewFG(coloravg(fgcolor.Pixel(),bgcolor.Pixel(),.6));
						dp->NewBG(coloravg(fgcolor.Pixel(),bgcolor.Pixel(),.9));
						dp->drawrectangle(imxmin+c2*dist-imw/2, iy+ih/2+imh/2, imw,-imh, 2);
						dp->imageout(item2->image, imxmin+c2*dist-imw/2, iy+ih/2-imh/2, imw,imh);
						//dp->imageout(item2->image, imxmin+c2*dist-imw/2, iy+ih/2+imh/2, imw,-imh);

					} else dp->textout(ix+iw/2,iy+ih/2, modes.e(c2)->name,-1, LAX_CENTER);

					break;
				}
			}
			dp->NewFG(&fgcolor);

		} else if (item->id==ENGRAVE_Groups) {

			int ww;
			for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
				item2=item->GetSubmenu()->e(c2);
				i2x=item2->x+panelbox.minx;
				i2y=item2->y+panelbox.miny;
				i2w=item2->w;
				i2h=item2->h;
				ww=i2w; if (i2h<ww) ww=i2h;

				if (lasthover==item2->id) { //highlight hovered
					dp->NewFG(hcolor);
					dp->drawrectangle(i2x, i2y, i2w, i2h, 1);
					dp->NewFG(&fgcolor);
				}

				if (item2->id==ENGRAVE_Toggle_Group_List) {
					dp->drawrectangle(i2x+i2w/5,i2y+i2h*3/16,   i2w*.6,i2h/8, 1);
					dp->drawrectangle(i2x+i2w/5,i2y+i2h*7/16,   i2w*.6,i2h/8, 1);
					dp->drawrectangle(i2x+i2w/5,i2y+i2h*11/16,  i2w*.6,i2h/8, 1);

				} else if (item2->id==ENGRAVE_Previous_Group) {
					ww=i2w*.4;
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,ww, 0, THING_Triangle_Left);

				} else if (item2->id==ENGRAVE_Next_Group) {
					ww=i2w*.4;
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,ww, 0, THING_Triangle_Right);

				} else if (item2->id==ENGRAVE_Group_Name) {
					if (group && i2h>0) dp->textout(i2x+pad, i2y+i2h/2, 
							(isblank(group->name) ? "(unnamed)" : group->name) ,-1,
							LAX_VCENTER|LAX_LEFT);

				} else if (item2->id==ENGRAVE_Group_Active && i2h>0) {
					DrawThingTypes thing= (group && group->active) ? THING_Open_Eye : THING_Closed_Eye;
					ww=i2w*.4;
					dp->NewBG(1.,1.,1.);
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,-ww, 2, thing);

				} else if (item2->id==ENGRAVE_Group_Linked && i2h>0) {
					ww=i2w*.25;
					unsigned long color=(group && group->linked ? rgbcolor(0,200,0) : rgbcolor(255,100,100) );
					dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,-ww, THING_Circle, color,color);

				} else if (item2->id==ENGRAVE_Group_Color && i2h>0) {
					if (group) {
						dp->NewFG(&group->color);
						dp->drawrectangle(i2x, i2y, i2w, i2h, 1);
					}
					dp->NewFG(&fgcolor);
					dp->drawrectangle(i2x, i2y, i2w, i2h, 0);

				} else if (item2->id==ENGRAVE_Group_List && item2->isOpen()) {
					EngraverFillData *obj=NULL; 
					EngraverPointGroup *group2;

					dp->NewFG(&bgcolor);
					dp->drawrectangle(i2x,i2y,i2w,i2h,1);
					dp->NewFG(&fgcolor);

					int oncurobj=0;
					int yy=i2y;
					int ii=0;
					double hhh;

					if (!selection) selection=new Selection();
					DBG cerr<<"lasthoverindex="<<lasthoverindex<<endl;
					DBG cerr << "DrawPanel() selection.n="<<selection->n()<<endl;

					for (int o=0; o<selection->n(); o++) {
						//DBG cerr<<" selection #"<<o<<": "<<selection->e(o)->whattype()<<endl;
						obj=dynamic_cast<EngraverFillData *>(selection->e(o)->obj);
						if (!obj) { ii++; continue; }
						//obj=edata;

						if (obj) {
							if (obj==edata) oncurobj=1; else oncurobj=0;

							 //object header:  [eye] Object id
							ww=th*.4;
							hhh=1.0;
							if (lasthoverindex==ii || oncurobj) {
								 //highlight for current object
								ScreenColor col;
								if (lasthoverindex==ii) hhh-=.1;
								if (oncurobj) hhh-=.2;
								coloravg(&col, &fgcolor,&bgcolor, hhh);
								dp->NewFG(&col);
								dp->drawrectangle(i2x,i2y+ii*th, i2w,th,1);
								dp->NewFG(&fgcolor);
							}
							//dp->drawthing(i2x+i2w/2,i2y+i2h/2, ww,-ww, 2, obj->Visible() ? THING_Open_Eye : THING_Closed_Eye);
							dp->textout(i2x+th,yy+th/2, obj->Id(),-1, LAX_LEFT|LAX_VCENTER);
							yy+=th;

							ii++;

							 //the object's groups
							 // [eye] [color] [linked] [sharing]  group name
							for (int g=0; g<obj->groups.n; g++) {
								int xx=i2x;
								group2=obj->groups.e[g];

								hhh=1.0;
								if (lasthoverindex==ii || group==group2) {
									 //highlight bg for current group
									ScreenColor col;
									if (lasthoverindex==ii) hhh-=.1;
									if (group==group2) hhh-=.2;
									coloravg(&col, &fgcolor,&bgcolor, hhh);
									dp->NewFG(&col);
									dp->drawrectangle(i2x,i2y+ii*th, i2w,th,1);
									dp->NewFG(&fgcolor);
								}
								
								 //active eye
								dp->drawthing(xx+th/2,yy+th/2, ww,-ww, 2,  group2->active ? THING_Open_Eye : THING_Closed_Eye);
								xx+=th;


								 //linked
								unsigned long color;
								double rr=.25; //radius of inner linked circle
								if (oncurobj && g==current_group && group2->active && !group2->linked) {
									 //draw green override circle over current group linked thing regardless of actual color
									color=rgbcolor(0,200,0);
									dp->drawthing(xx+th/2,yy+th/2, th*.4,-th*.4, THING_Circle, color,color);
									rr=.2;
								}
								if (group2->linked && !group2->active) {
									//draw red override circle, since group is not modifiable while invisible
									color=rgbcolor(255,100,100);
									dp->drawthing(xx+th/2,yy+th/2, th*.4,-th*.4, THING_Circle, color,color);
									rr=.2;
								}
								 //now draw actual linked state
								color=(group2 && group2->linked ? rgbcolor(0,200,0) : rgbcolor(255,100,100) );
								dp->drawthing(xx+th/2,yy+th/2, th*rr,-th*rr, THING_Circle, color,color);
								dp->NewFG(&fgcolor);
								xx+=th;


								 //color rectangle
								dp->NewFG(&group2->color);
								dp->drawrectangle(xx+th*.1, yy+th*.1, th*.8, th*.8, 1);	
								dp->NewFG(&fgcolor);
								dp->drawrectangle(xx+th*.1, yy+th*.1, th*.8, th*.8, 0);	
								xx+=th;

								 //name
								dp->textout(xx, yy+th/2, 
									(isblank(group2->name) ? "(unnamed)" : group2->name) ,-1,
									LAX_VCENTER|LAX_LEFT);

								yy+=th;
								ii++;

							} //foreach group in obj
						} // if obj!=NULL
					} //for each object in selection

				} else if (item2->id==ENGRAVE_New_Group) {
					dp->textout(i2x+i2w/2, i2y+i2h/2, "+",-1, LAX_CENTER);

				} else if (item2->id==ENGRAVE_Delete_Group) {
					dp->textout(i2x+i2w/2, i2y+i2h/2, "-",-1, LAX_CENTER);

				} else if (item2->id==ENGRAVE_Group_Down) {
					ww*=.8;
					dp->drawthing(i2x+i2w/2, i2y+i2h/2, ww/2,ww/2, 1, THING_Arrow_Down);

				} else if (item2->id==ENGRAVE_Group_Up) {
					ww*=.8;
					dp->drawthing(i2x+i2w/2, i2y+i2h/2, ww/2,ww/2, 1, THING_Arrow_Up);

				} else if (item2->id==ENGRAVE_Merge_Group) {
					ww*=.8;
					dp->drawthing(i2x+i2w/2, i2y+i2h/2, ww/2,ww/2, 1, THING_Arrow_Right);

				} else if (item2->id==ENGRAVE_Toggle_Group_List) {
					ww*=.8; 
					dp->drawline(i2x+i2w/4, i2y+i2h*3/4, i2x+3*i2w/4,i2y+i2h*3/4);
					dp->drawline(i2x+i2w/4, i2y+i2h/2, i2x+3*i2w/4,i2y+i2h/2);
					dp->drawline(i2x+i2w/4, i2y+i2h/4, i2x+3*i2w/4,i2y+i2h/4);

				}
			}

		} else if (item->id==ENGRAVE_Tracing) {
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);

			if (item->isOpen()) { 

				EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
				dp->NewFG(&fgcolor);

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					i2x=item2->x+panelbox.minx;
					i2y=item2->y+panelbox.miny;
					i2w=item2->w;
					i2h=item2->h;

					if (lasthover==item2->id && 
						 (   lasthover==ENGRAVE_Trace_Continuous 
						  || lasthover==ENGRAVE_Trace_Once
						  || lasthover==ENGRAVE_Trace_Same_As
						  || lasthover==ENGRAVE_Trace_Name
						  || lasthover==ENGRAVE_Trace_Object
						  || lasthover==ENGRAVE_Trace_Object_Name
						  || lasthover==ENGRAVE_Trace_Set
						 )) {
						 //draw hover highlight across whole item2 box
						dp->NewFG(hcolor);
						dp->drawrectangle(i2x,i2y, i2w,i2h, 1);
						dp->NewFG(&fgcolor);
					}

					if ((item2->id==ENGRAVE_Trace_Same_As || item2->id==ENGRAVE_Trace_Name) && item2->w>0) {
						if (item2->id==ENGRAVE_Trace_Name) { 
							if (group) dp->textout(i2x+i2w/2,i2y+i2h/2, group->trace->Id(),-1, LAX_CENTER);

						} else {
							int sharing=IsSharing(ENGRAVE_Tracing, NULL, current_group);
							if (sharing) {
								dp->textout(i2x+i2w/2,i2y+i2h/2, _("With:"),-1, LAX_CENTER);
							} else dp->textout(i2x+i2w/2,i2y+i2h/2, _("(Not shared)"),-1, LAX_CENTER);
						} 

					} else if (item2->id==ENGRAVE_Trace_Menu) { 
						dp->drawthing(i2x+i2w/2, i2y+i2h/2, th*.2,th*.2,
								lasthover==ENGRAVE_Trace_Menu || lasthover==ENGRAVE_Trace_Same_As ? 1 : 0, THING_Triangle_Down); 

					} else if (item2->id==ENGRAVE_Trace_Once) {
						 //single trace square
						dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter); 
						dp->LineWidthScreen(3);
						if (lasthover==ENGRAVE_Trace_Once) dp->NewFG(activate_color); else dp->NewFG(deactivate_color);
						dp->drawrectangle(i2x+i2w-i2h-th/2, i2y+1,  i2h-2,i2h-2, 0);
						dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter);
						dp->LineWidthScreen(1);
						dp->NewFG(&fgcolor);

					} else if (item2->id==ENGRAVE_Trace_Continuous) {
						 //continuous trace circle
						dp->LineAttributes(-1,LineSolid, CapButt, JoinMiter); 
						dp->LineWidthScreen(3);
						if (group && group->trace->continuous_trace) dp->NewFG(activate_color); else dp->NewFG(deactivate_color);
						dp->drawellipse(i2x+i2h/2+th/2,i2y+i2h/2,
											i2h/2-1,i2h/2-1,
											0,2*M_PI,
											0);
						dp->LineAttributes(1,LineSolid, CapButt, JoinMiter); 
						dp->NewFG(&fgcolor);

					} else if (item2->id==ENGRAVE_Trace_Curve) {
						curvemapi.Dp(dp);
						if (group) curvemapi.SetInfo(group->trace ->value_to_weight);
						else       curvemapi.SetInfo(default_trace. value_to_weight);
						curvemapi.SetupRect(i2x,i2y, i2w,i2h-pad);
						curvemapi.needtodraw=1;
						curvemapi.Refresh();
						dp->DrawScreen();
						dp->NewFG(&fgcolor);

					} else if (item2->id==ENGRAVE_Trace_Curve_Line_Bar) {
						DrawLineGradient(i2x,i2x+i2w, i2y,i2y+i2h, current_group, 0);

					} else if (item2->id==ENGRAVE_Trace_Curve_Value_Bar) {
						DrawShadeGradient(i2x+th,i2x+i2w, i2y,i2y+i2h);

					} else if (item2->id==ENGRAVE_Trace_Set) { //bottom left corner of curve area
						if (trace->tracetype==0) {
							dp->drawrectangle(i2x+i2w/2-i2h/2, i2y+i2h/2-i2h*.25, i2h, i2h*.5, 1);
						} else {
							double xx=i2x+i2w/2-i2h/2, yy=i2y+i2h/2;
							dp->moveto(xx, yy);
							dp->curveto(flatpoint(xx+i2h/3,  yy),
										flatpoint(xx+i2h/3,  yy+i2h/4),
										flatpoint(xx+i2h/2,  yy+i2h/4));
							dp->curveto(flatpoint(xx+2*i2h/3,yy+i2h/4),
										flatpoint(xx+2*i2h/3,yy),
										flatpoint(xx+i2h,    yy));
							dp->curveto(flatpoint(xx+2*i2h/3,yy),
										flatpoint(xx+2*i2h/3,yy-i2h/4),
										flatpoint(xx+i2h/2,  yy-i2h/4));
							dp->curveto(flatpoint(xx+i2h/3,  yy-i2h/4),
										flatpoint(xx+i2h/3,  yy),
										flatpoint(xx,        yy));
							dp->closed();
							dp->fill(0);
						}

					} else if (item2->id==ENGRAVE_Trace_Opacity) {
						DrawSlider(trace->traceobj_opacity, lasthover==ENGRAVE_Trace_Opacity,
								   i2x,i2y, i2w,i2h, "");

					} else if (item2->id==ENGRAVE_Trace_Object_Name) {
						dp->textout(i2x,i2y+i2h/2, 
								trace->Identifier() ? trace->Identifier() : "...",-1,
								LAX_LEFT|LAX_VCENTER);

					} else if (item2->id==ENGRAVE_Trace_Object_Menu) {
						dp->drawthing(i2x+i2w/2, i2y+i2h/2, th*.2,th*.2,
								lasthover==ENGRAVE_Trace_Object_Menu ? 1 : 0, THING_Triangle_Down); 
					}
				} 
			}

		} else if (item->id==ENGRAVE_Dashes) {
			dp->NewFG(&fgcolor);
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);
			if (item->isOpen()) {
				//dp->textout(ix+iw/2,iy+th+(ih-th)/2, "Todo!",-1,LAX_CENTER);

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					i2x=item2->x+panelbox.minx;
					i2y=item2->y+panelbox.miny;
					i2w=item2->w;
					i2h=item2->h;

					if (lasthover==item2->id && 
						 (   item2->id==ENGRAVE_Dash_Same_As
						  || item2->id==ENGRAVE_Dash_Name
						 )) {
						 //draw hover highlight across whole item2 box
						dp->NewFG(hcolor);
						dp->drawrectangle(i2x,i2y, i2w,i2h, 1);
						dp->NewFG(&fgcolor);
					}

					if ((item2->id==ENGRAVE_Dash_Same_As || item2->id==ENGRAVE_Dash_Name) && item2->w>0) {
						if (item2->id==ENGRAVE_Dash_Name) { 
							if (group) dp->textout(i2x+i2w/2,i2y+i2h/2, group->dashes->Id(),-1, LAX_CENTER);

						} else {
							int sharing=IsSharing(ENGRAVE_Dashes, NULL, current_group);
							if (sharing) {
								dp->textout(i2x+i2w/2,i2y+i2h/2, _("With:"),-1, LAX_CENTER);
							} else dp->textout(i2x+i2w/2,i2y+i2h/2, _("(Not shared)"),-1, LAX_CENTER);
						} 

					} else if (item2->id==ENGRAVE_Dash_Menu) { 
						dp->drawthing(i2x+i2w/2, i2y+i2h/2, th*.2,th*.2,
								lasthover==ENGRAVE_Dash_Menu || lasthover==ENGRAVE_Dash_Same_As ? 1 : 0, THING_Triangle_Down); 

					} else if (item2->id==ENGRAVE_Dash_Broken_Threshhold) {
						 //draw this to span broken and zero areas
						DrawLineGradient(i2x,i2x+i2w, i2y+pad/4,i2y+i2h*2-pad/2, current_group, 1);

						int hh=i2h*.4;
						if (group) {
							unsigned long col=fgcolor.Pixel();
							if (lasthover==ENGRAVE_Dash_Broken_Threshhold) col=rgbcolorf(.5,.5,.5);
							dp->drawthing(i2x+i2w*group->dashes->broken_threshhold/group->spacing->spacing,
									i2y+hh/2, pad*2/3.,hh*.6, THING_Triangle_Down, bgcolor.Pixel(),col); 

							col=bgcolor.Pixel();
							if (lasthover==ENGRAVE_Dash_Zero_Threshhold) col=rgbcolorf(.5,.5,.5);
							dp->drawthing(i2x+i2w*group->dashes->zero_threshhold/group->spacing->spacing,
									i2y+i2h+i2h-hh/2, pad/2.,hh/2, THING_Triangle_Up, fgcolor.Pixel(),col);
						}

					} else if (item2->id==ENGRAVE_Dash_Zero_Threshhold) {
						 // this is drawn with ENGRAVE_Dash_Broken_Threshhold

					} else if (item2->id==ENGRAVE_Dash_Random) {
						DrawSlider((group ? group->dashes->dash_randomness : .5), lasthover==ENGRAVE_Dash_Random,
								i2x,i2y,i2w,i2h, _("Random"));

					} else if (item2->id==ENGRAVE_Dash_Density) {
						DrawSlider((group ? group->dashes->dash_density : .5), lasthover==ENGRAVE_Dash_Density,
								i2x,i2y,i2w,i2h, _("Density"));

					} else if (item2->id==ENGRAVE_Dash_Taper) {
						DrawSlider((group ? group->dashes->dash_taper : .5), lasthover==ENGRAVE_Dash_Taper,
								i2x,i2y,i2w,i2h, _("Taper"));

					} else if (item2->id==ENGRAVE_Dash_Length) {
						DrawNumInput((group ? group->dashes->dash_length : 2), 0, lasthover==ENGRAVE_Dash_Length,
								i2x,i2y,i2w,i2h, _("Length"));

					} else if (item2->id==ENGRAVE_Dash_Seed) {
						DrawNumInput((group ? group->dashes->randomseed : -1), 1, lasthover==ENGRAVE_Dash_Seed,
								i2x,i2y,i2w,i2h, _("Seed"));

					} else if (item2->id==ENGRAVE_Dash_Caps) {
						// *** todo!
					} else if (item2->id==ENGRAVE_Dash_Join) {
						// *** todo!
					}
				}
			}

		} else if (item->id==ENGRAVE_Spacing) {
			dp->NewFG(&fgcolor);
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);
			if (item->isOpen()) {

				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					i2x=item2->x+panelbox.minx;
					i2y=item2->y+panelbox.miny;
					i2w=item2->w;
					i2h=item2->h;

					if (lasthover==item2->id && 
						 (   item2->id==ENGRAVE_Spacing_Same_As
						  || item2->id==ENGRAVE_Spacing_Name
						  || item2->id==ENGRAVE_Spacing_Map
						 )) {
						 //draw hover highlight across whole item2 box
						dp->NewFG(hcolor);
						dp->drawrectangle(i2x,i2y, i2w,i2h, 1);
						dp->NewFG(&fgcolor);
					} 

					if ((item2->id==ENGRAVE_Spacing_Same_As || item2->id==ENGRAVE_Spacing_Name) && item2->w>0) {
						if (item2->id==ENGRAVE_Spacing_Name) { 
							if (group) dp->textout(i2x+i2w/2,i2y+i2h/2, group->spacing->Id(),-1, LAX_CENTER);

						} else {
							int sharing=IsSharing(ENGRAVE_Spacing, NULL, current_group);
							if (sharing) {
								dp->textout(i2x+i2w/2,i2y+i2h/2, _("With:"),-1, LAX_CENTER);
							} else dp->textout(i2x+i2w/2,i2y+i2h/2, _("(Not shared)"),-1, LAX_CENTER);
						} 

					} else if (item2->id==ENGRAVE_Spacing_Menu) { 
						dp->drawthing(i2x+i2w/2, i2y+i2h/2, th*.2,th*.2,
								lasthover==ENGRAVE_Spacing_Menu || lasthover==ENGRAVE_Spacing_Same_As ? 1 : 0, THING_Triangle_Down); 

					} else if (item2->id==ENGRAVE_Spacing_Default) {
						DrawNumInput((group ? group->spacing->spacing : default_spacing), 0, lasthover==ENGRAVE_Spacing_Default,
								i2x,i2y,i2w,i2h, NULL);

					} else if (item2->id==ENGRAVE_Spacing_Map) {
						if (group && group->spacing->map) {
							dp->textout(i2x+i2w/2,i2y+i2h/2, group->spacing->map->Id(),-1, LAX_CENTER);
						} else dp->textout(i2x+i2w/2,i2y+i2h/2, "...",-1, LAX_CENTER);

					} else if (item2->id==ENGRAVE_Spacing_Map_Menu) {
						dp->drawthing(i2x+i2w/2, i2y+i2h/2, th*.2,th*.2,
								lasthover==ENGRAVE_Spacing_Map_Menu ? 1 : 0, THING_Triangle_Down); 

					}
				}
			}

		} else if (item->id==ENGRAVE_Direction) {
			dp->NewFG(&fgcolor);
			DrawPanelHeader(item->isOpen(), lasthover==item->id, item->name, ix,iy,iw,ih);

			if (item->isOpen()) {
				for (int c2=0; c2<item->GetSubmenu()->n(); c2++) {
					item2=item->GetSubmenu()->e(c2);
					i2x=item2->x+panelbox.minx;
					i2y=item2->y+panelbox.miny;
					i2w=item2->w;
					i2h=item2->h;

					if (lasthover==item2->id && 
						 (   item2->id==ENGRAVE_Direction_Same_As
						  || item2->id==ENGRAVE_Direction_Name
						  || item2->id==ENGRAVE_Direction_Type
						  || item2->id==ENGRAVE_Direction_Show_Dir
						  || item2->id==ENGRAVE_Direction_Grow
						  || item2->id==ENGRAVE_Direction_Fill
						  || item2->id==ENGRAVE_Direction_Merge
						  || item2->id==ENGRAVE_Direction_Reline
						 )) {
						 //draw hover highlight across whole item2 box
						dp->NewFG(hcolor);
						dp->drawrectangle(i2x,i2y, i2w,i2h, 1);
						dp->NewFG(&fgcolor);
					} 

					if ((item2->id==ENGRAVE_Direction_Same_As || item2->id==ENGRAVE_Direction_Name) && item2->w>0) {
						if (item2->id==ENGRAVE_Direction_Name) { 
							if (group) dp->textout(i2x+i2w/2,i2y+i2h/2, group->direction->Id(),-1, LAX_CENTER);

						} else {
							int sharing=IsSharing(ENGRAVE_Direction, NULL, current_group);
							if (sharing) {
								dp->textout(i2x+i2w/2,i2y+i2h/2, _("With:"),-1, LAX_CENTER);
							} else dp->textout(i2x+i2w/2,i2y+i2h/2, _("(Not shared)"),-1, LAX_CENTER);
						} 

					} else if (item2->id==ENGRAVE_Direction_Menu) { 
						dp->drawthing(i2x+i2w/2, i2y+i2h/2, th*.2,th*.2,
								lasthover==ENGRAVE_Direction_Menu || lasthover==ENGRAVE_Direction_Same_As ? 1 : 0, THING_Triangle_Down); 

					} else if (item2->id==ENGRAVE_Direction_Type) { 
						dp->NewFG(&fgcolor);
						const char *str=group ? group->direction->TypeName() : _("Linear");
						dp->textout(i2x+i2w/2,i2y+i2h/2, str,-1, LAX_CENTER);

					} else if (item2->id==ENGRAVE_Direction_Reline) { 
						dp->NewFG(auto_reline ? activate_color : deactivate_color);
						dp->LineAttributes(-1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
						dp->LineWidthScreen(2);
						dp->drawthing(i2x+i2w/2,i2y+i2h/2, i2h*.4,i2h*.4, 0, THING_Circle);
						//dp->drawline(i2x,i2y+i2h/5, i2x+i2w/2, i2y+i2h/5);
						////dp->drawline(i2x,i2y+i2h/2, i2x+i2w/2, i2y+i2h/2);
						//dp->drawarrow(flatpoint(i2x,i2y+i2h/2),flatpoint(i2w,0), 0,1,2,3);
						//dp->drawline(i2x,i2y+i2h*.8, i2x+i2w/2, i2y+i2h*.8);
						//dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
						dp->NewFG(&fgcolor);

					} else if (item2->id==ENGRAVE_Direction_Show_Dir) {
						DrawCheckBox(show_direction, lasthover==item2->id, i2x,i2y,i2w,i2h, NULL);

					} else if (item2->id==ENGRAVE_Direction_Profile) {
						// *** to do to do to do to do to do to do to do to do to do to do 
						// --|-------|--- <- start and end positions
						//   -      ---   <- random spread

						double start=0, end=1, startspread=0, endspread=0;
						if (group) {
							start=group->direction->profile_start;
							end  =group->direction->profile_end;
							if (group->direction->start_type==1) startspread=group->direction->start_rand_width;
							if (group->direction->end_type  ==1) endspread  =group->direction->end_rand_width;
						}
						start=i2x+i2w*start;
						end  =i2x+i2w*end;
						startspread*=i2w;
						endspread  *=i2w;

						 //blank out profile area
						dp->NewFG(1.0,1.0,1.0);
						dp->drawrectangle(start, i2y+i2h/4, (end-start), i2h/2, 1);
						dp->NewFG(.5,.5,.5);
						dp->drawline(i2x,i2y+i2h/2, i2x+i2w,i2y+i2h/2);

						 //draw in base profile
						dp->NewFG(&fgcolor);
						if (group && group->direction->default_profile) {
							dp->imageout(group->direction->default_profile->Preview(), start,i2y+i2h/4, end-start,i2h/2);
						} else { 
							dp->drawrectangle(start, i2y+i2h/3, end-start,i2h/3, 1);
						} 

						 //start
						if (lasthover==ENGRAVE_Direction_Profile_Start) {
							dp->LineAttributes(3,LineSolid,LAXCAP_Round,LAXJOIN_Round);
							dp->NewFG(&fgcolor);
						}
						dp->drawline(start,i2y, start,i2y+i2h);
						dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
						dp->NewFG(.5,.5,.5);

						 //end
						if (lasthover==ENGRAVE_Direction_Profile_End) {
							dp->LineAttributes(3,LineSolid,LAXCAP_Round,LAXJOIN_Round);
							dp->NewFG(&fgcolor);
						}
						dp->drawline(end,i2y, end,i2y+i2h);
						dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
						dp->NewFG(.5,.5,.5);

						 //start spread
						if (lasthover==ENGRAVE_Direction_Profile_Start_Random) dp->NewFG(&fgcolor);
						if (startspread==0) {
							dp->drawthing(start, i2y+3*i2h/4, th*.2,th*.2,
								lasthover==ENGRAVE_Direction_Profile_Start_Random ? 1 : 0, THING_Triangle_Up); 
						} else {
							double starts=start-startspread, starte=start+startspread;
							if (starts<i2x) starts=i2x;
							if (starte>i2x+i2w) starte=i2x+i2w;
							dp->drawrectangle(starts, i2y+3*i2h/4, starte-starts,th*.2,
								lasthover==ENGRAVE_Direction_Profile_Start_Random ? 1 : 0); 
						}

						 //end spread
						dp->NewFG(.5,.5,.5);
						if (lasthover==ENGRAVE_Direction_Profile_End_Random) dp->NewFG(&fgcolor);
						if (endspread==0) {
							dp->drawthing(end, i2y+3*i2h/4, th*.2,th*.2,
								lasthover==ENGRAVE_Direction_Profile_End_Random ? 1 : 0, THING_Triangle_Up); 
						} else {
							double ends=end-endspread, ende=end+endspread;
							if (ends<i2x) ends=i2x;
							if (ende>i2x+i2w) ende=i2x+i2w;
							dp->drawrectangle(ends, i2y+3*i2h/4, ende-ends,th*.2,
								lasthover==ENGRAVE_Direction_Profile_End_Random ? 1 : 0); 
						}

						//dp->NewFG(1.0,1.0,1.0);
						//dp->textout(i2x+i2w/2-1,i2y+i2h/2-1, "Profile Todo!",-1,LAX_CENTER); 
						//dp->textout(i2x+i2w/2-1,i2y+i2h/2+1, "Profile Todo!",-1,LAX_CENTER); 
						//dp->textout(i2x+i2w/2+1,i2y+i2h/2-1, "Profile Todo!",-1,LAX_CENTER); 
						//dp->textout(i2x+i2w/2+1,i2y+i2h/2+1, "Profile Todo!",-1,LAX_CENTER); 
						//dp->NewFG(&fgcolor);
						//dp->textout(i2x+i2w/2,i2y+i2h/2, "Profile Todo!",-1,LAX_CENTER); 


					} else if (item2->id==ENGRAVE_Direction_Profile_Menu) {
						dp->drawthing(i2x+i2w/2, i2y+i2h/2, th*.2,th*.2,
								lasthover==ENGRAVE_Direction_Profile_Menu ? 1 : 0, THING_Triangle_Down); 

					} else if (item2->id==ENGRAVE_Direction_Profile_Scale) {
						 //draw connected double arrows for scale, else disconnected double arrows
						if (lasthover==ENGRAVE_Direction_Profile_Scale) dp->NewFG(&fgcolor);
						else dp->NewFG(.5,.5,.5);

						dp->drawarrow(flatpoint(i2x+i2w/2,i2y+i2h/2),flatpoint(0,i2h/2), 0,1,2,3);
						dp->drawarrow(flatpoint(i2x+i2w/2,i2y+i2h/2),flatpoint(0,-i2h/2), 0,1,2,3);

						int on=(group ? group->direction->scale_profile : 0);
						if (on) {
							dp->drawarrow(flatpoint(i2x+i2w/2,i2y+i2h/2),flatpoint(i2w/2,0), 0,1,2,3);
							dp->drawarrow(flatpoint(i2x+i2w/2,i2y+i2h/2),flatpoint(-i2w/2,0), 0,1,2,3);
						} else {
							dp->drawarrow(flatpoint(i2x+i2w/4,i2y+i2h/2),flatpoint(-i2w/4,0), 0,1,2,3);
							dp->drawarrow(flatpoint(i2x+3*i2w/4,i2y+i2h/2),flatpoint(i2w/4,0), 0,1,2,3);
						} 

					} else if (item2->id==ENGRAVE_Direction_Profile_Max_Height) {
						DrawSlider((group ? group->direction->max_height/2 : .5), lasthover==ENGRAVE_Direction_Profile_Max_Height,
								i2x,i2y,i2w,i2h, _("Height"));

					} else if (item2->id==ENGRAVE_Direction_Line_Offset) {
						 //draw normal slider, but with dashed line just above regular line
						if (lasthover==ENGRAVE_Direction_Line_Offset) dp->NewFG(&fgcolor);
						else dp->NewFG(.5,.5,.5);
						dp->LineAttributes(1,LineOnOffDash,LAXCAP_Round,LAXJOIN_Round);
						dp->drawline(i2x,i2y+i2h/2-4, i2x+i2w,i2y+i2h/2-4);
						dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
						dp->NewFG(&fgcolor);

						DrawSlider((group ? group->direction->line_offset/2 : 0), lasthover==ENGRAVE_Direction_Line_Offset,
								i2x,i2y,i2w,i2h, NULL);

					} else if (item2->id==ENGRAVE_Direction_Point_Offset) {
						 //draw line increasingly random in + x dir
						 //with little bar for random feature size
						if (lasthover==ENGRAVE_Direction_Point_Offset) dp->NewFG(&fgcolor);
						else dp->NewFG(.5,.5,.5);

						if (group && group->direction->seed>0) noise.Seed(group->direction->seed);
						else noise.Seed(random());
						dp->moveto(i2x,i2y+i2h/2);
						double featuresize=(group ? group->direction->noise_scale : 1);
						if (featuresize<1) featuresize=1;
						featuresize*=featuresize;

						for (int x=i2x; x<i2x+i2w; x+=5) {
							dp->lineto(x, i2y+i2h/2 + (x-i2x)/(double)i2w * i2h * (noise.Evaluate((x-i2x)/5/featuresize, 0)*.6));
						}
						dp->stroke(0);

						 //contains ENGRAVE_Direction_Point_Off_Size:
						if (lasthover==ENGRAVE_Direction_Point_Off_Size) {
							dp->NewFG(&fgcolor);
							dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
						} else dp->NewFG(.5,.5,.5);

						double pos=(group ? group->direction->noise_scale*i2w/5 : 0);
						dp->drawline(i2x,i2y+3*i2h/4, i2x+pos,i2y+3*i2h/4);
						dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);

						 //draw the little knob on top
						if (lasthover==ENGRAVE_Direction_Point_Offset) dp->NewFG(&fgcolor);
						else dp->NewFG(.5,.5,.5);
						pos=(group ? group->direction->point_offset : 0)/2;
						if (pos<0) pos=0;
						if (pos>1) pos=1;
						dp->drawpoint(flatpoint(i2x + i2h/4 + pos*(i2w-i2h/2),i2y+i2h/2), i2h*.4, 1);

						dp->NewFG(&fgcolor);

					} else if (item2->id==ENGRAVE_Direction_Grow) {
						DrawCheckBox((group ? group->direction->grow_lines : 0), 1, i2x,i2y,i2w,i2h, _("Grow"));

					} else if (item2->id==ENGRAVE_Direction_Fill) {
						DrawCheckBox((group ? group->direction->fill : 0),
									(group && group->direction->grow_lines ? 1 : 0), 
									i2x,i2y,i2w,i2h, _("Fill"));

					} else if (item2->id==ENGRAVE_Direction_Merge) {
						DrawCheckBox((group ? group->direction->merge : 0),
									(group && group->direction->grow_lines ? 1 : 0), 
									i2x,i2y,i2w,i2h, _("Merge"));

					} else if (item2->id==ENGRAVE_Direction_Spread) {
						//contains ENGRAVE_Direction_Spread_Depth, ENGRAVE_Direction_Spread_Angle
						// *** to do to do to do to do to do to do to do to do to do to do 
						//        depth
						// spread |>--|---/ angle

						dp->NewFG(&fgcolor);
						dp->moveto(i2x,i2y);
						dp->curveto(flatpoint(i2x,i2y+i2h/2), flatpoint(i2x+i2w/4,i2y+3*i2h/7), flatpoint(i2x+i2w/4,i2y+3*i2h/7));
						dp->curveto(flatpoint(i2x+.75*i2w,i2y+3*i2h/7), flatpoint(i2x+i2w,i2y), flatpoint(i2x+i2w,i2y));
						dp->curveto(flatpoint(i2x+i2w,i2y), flatpoint(i2x+.75*i2w,i2y+4*i2h/7), flatpoint(i2x+.5*i2w,i2y+4*i2h/7));
						dp->curveto(flatpoint(i2x+i2w/4,i2y+4*i2h/7), flatpoint(i2x,i2y+i2h/2), flatpoint(i2x,i2y+i2h));
						dp->closed();
						dp->fill(0);

						dp->NewFG(1.0,1.0,1.0);
						dp->textout(i2x+i2w/2-1,i2y+i2h/2-1, "Merge spread Todo!",-1,LAX_CENTER);
						dp->textout(i2x+i2w/2+1,i2y+i2h/2-1, "Merge spread Todo!",-1,LAX_CENTER);
						dp->textout(i2x+i2w/2,i2y+i2h/2+1, "Merge spread Todo!",-1,LAX_CENTER);
						dp->NewFG(&fgcolor);
						dp->textout(i2x+i2w/2,i2y+i2h/2, "Merge spread Todo!",-1,LAX_CENTER);

					} else if (item2->id==ENGRAVE_Direction_Seed) {
						DrawNumInput((group ? group->direction->seed : -1), 1, lasthover==ENGRAVE_Direction_Seed,
								i2x,i2y,i2w,i2h, _("Seed")); 


					} else {
						// DBG draw X through box
						dp->NewFG(.5,.5,.5);
						dp->drawline(i2x,i2y, i2x+i2w,i2y+i2h);
						dp->drawline(i2x,i2y+i2h, i2x+i2w,i2y);
					}
				}
			}
		}
	} 

	dp->DrawReal();
}

/*! Return if the item type is shared with another group.
 * It will return 1 if group is shared somewhere else in selection.
 * Else 0 for not sharing with anyone.
 */
int EngraverFillInterface::IsSharing(int what, EngraverPointGroup *group, int curgroup)
{
	if (!group) group=(edata ? edata->GroupFromIndex(curgroup) : NULL);
	if (!group) return 0;

	for (int g=0; g<selection->n(); g++) {
		EngraverFillData *obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;

		if (obj->IsSharing(what, group, -1)) return 1;;
	}

	return 0;
}

/*! Draw one of the panel sections, just a line around a name, with a little triangle.
 */
void EngraverFillInterface::DrawPanelHeader(int open, int hover, const char *name, int x,int y,int w, int hh)
{
	int h=dp->textheight(); //hh is whole section height, we can ignore.. maybe one day draw box around whole section
	double ww=dp->textextent(name,-1,NULL,NULL)+2*dp->textextent(" ",1,NULL,NULL);
	if (ww<w) {
		dp->LineAttributes(2,LineSolid,LAXCAP_Round,LAXJOIN_Round);
		dp->drawline(x,y+h/2, x+(w-ww)/2,y+h/2);
		dp->drawline(x+w,y+h/2, x+w-(w-ww)/2,y+h/2);
		dp->LineAttributes(1,LineSolid,LAXCAP_Round,LAXJOIN_Round);
	}
	dp->textout(x+w/2,y+h/2, name,-1, LAX_CENTER);
	dp->drawthing(x+w-h,y+h/2,h*.3,h*.3, open ? THING_Triangle_Down : THING_Triangle_Right,
			fgcolor.Pixel(),hover?(coloravg(fgcolor.Pixel(),bgcolor.Pixel(),.6)):bgcolor.Pixel());
}

int EngraverFillInterface::PerformAction(int action)
{
	if (action==PATCHA_RenderMode) {
		if (rendermode==0) rendermode=1;
		else if (rendermode==1) rendermode=2;
		else rendermode=0;

		if (rendermode==0) PostMessage(_("Render with grid"));
		else if (rendermode==1) PostMessage(_("Render with preview"));
		else if (rendermode==2) PostMessage(_("Render recursively"));

		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_SwitchMode || action==ENGRAVE_SwitchModeR) {

		int i=modes.findIndex(mode);

		if (action==ENGRAVE_SwitchMode) {
			i++;
			if (i>=modes.n()) i=0;
		} else {
			i--;
			if (i<0) i=modes.n()-1;
		}

		ChangeMode(modes.e(i)->id);

		submode=0;
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_NextGroup) {
		if (!edata) return 0;

		int cur=CurrentLineIndex(NULL);

		cur++;

		int gindex;
		EngraverFillData *obj = GroupFromLineIndex(cur, &gindex);
		if (!obj) {
			cur=1;
			obj=GroupFromLineIndex(cur,&gindex);
		} else {
			if (gindex<0) { cur++; gindex=0; }
		} 

		UseThisObject(selection->e(selection->ObjectIndex(obj)));
		current_group=gindex;
		UpdatePanelAreas();
		needtodraw=1;

		return 0;

	} else if (    action==ENGRAVE_ToggleTracePanel
				|| action==ENGRAVE_ToggleDashPanel
				|| action==ENGRAVE_ToggleDirPanel   
				|| action==ENGRAVE_ToggleSpacingPanel) {

		int what=ENGRAVE_Tracing;
		if (action==ENGRAVE_ToggleDashPanel) what=ENGRAVE_Dashes;
		else if (action==ENGRAVE_ToggleDirPanel) what=ENGRAVE_Direction;
		else if (action==ENGRAVE_ToggleSpacingPanel) what=ENGRAVE_Spacing;

		MenuItem *item=panel.findid(what);

		if (item->isOpen()) item->Close();
		else item->Open();
		UpdatePanelAreas();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_PreviousGroup) {
		if (!edata) return 0;

		int max=0;
		int cur=CurrentLineIndex(&max);

		cur--;
		if (cur<1) cur=max-1;

		int gindex;
		EngraverFillData *obj = GroupFromLineIndex(cur, &gindex);
		if (gindex<0) {
			cur--;
			if (cur<1) cur=max-1;
			obj=GroupFromLineIndex(cur,&gindex);
		}

		UseThisObject(selection->e(selection->ObjectIndex(obj)));
		current_group=gindex;
		UpdatePanelAreas();
		needtodraw=1;

		return 0;


	} else if (action==ENGRAVE_ExportSvg) {
		app->rundialog(new FileDialog(NULL,"Export Svg",_("Export engraving to svg"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"exportsvg",FILES_SAVE, "out.svg"));
		return 0;

	} else if (action==ENGRAVE_ExportSnapshot) {
		app->rundialog(new FileDialog(NULL,"Export Snapshot",_("Export snapshot of group to image"),ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"exportsnapshot",FILES_SAVE, "out.png"));
		return 0;

	} else if (action==ENGRAVE_RotateDir || action==ENGRAVE_RotateDirR) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		group->directionv=rotate(group->directionv, (action==ENGRAVE_RotateDir ? M_PI/12 : -M_PI/12), 0);
		group->Fill(edata, -1);
		edata->Sync(false);
		Trace();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_SpacingInc || action==ENGRAVE_SpacingDec) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		if (action==ENGRAVE_SpacingInc) group->spacing->spacing*=1.1; else group->spacing->spacing*=.9;
		group->Fill(edata, -1);
		edata->Sync(false);
		Trace();
		DBG cerr <<"new spacing: "<<group->spacing<<endl;
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ResolutionInc || action==ENGRAVE_ResolutionDec) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		if (action==ENGRAVE_ResolutionInc) group->direction->resolution*=1.1; else group->direction->resolution/=1.1;
		group->Fill(edata, -1);
		edata->Sync(false);
		Trace();

		char buffer[75];
		sprintf(buffer,_("Resolution: %.5g"),group->direction->resolution);
		PostMessage(buffer);

		DBG cerr <<"new resolution: "<<group->direction->resolution<<endl;
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ShowPoints || action==ENGRAVE_ShowPointsN) {
		if (show_points) show_points=0;
		else if (action==ENGRAVE_ShowPoints) show_points=1;
 		else show_points=3;

		if (show_points&2) PostMessage(_("Show sample points with numbers"));
		else if (show_points&1) PostMessage(_("Show sample points"));
		else PostMessage(_("Don't show sample points"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_MorePoints) {
		edata->MorePoints(current_group);
		edata->ReverseSync(true);
		Trace();
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleShowTrace) {
		show_trace_object=!show_trace_object;
		//if (show_trace_object) continuous_trace=false;
		if (show_trace_object) PostMessage(_("Show tracing object"));
		else PostMessage(_("Don't show tracing object"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_TogglePanel) {
		show_panel=!show_panel;
		if (show_panel) PostMessage(_("Show control panel"));
		else PostMessage(_("Don't show control panel"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleGrow) {
		EngraverPointGroup *group=edata ? edata->GroupFromIndex(current_group) : NULL;
		if (!group) return 0;

		group->direction->grow_lines=!group->direction->grow_lines;
		if (group->direction->grow_lines) PostMessage(_("Grow lines after warp"));
		else PostMessage(_("Don't grow lines after warp"));

		if (group->direction->grow_lines) Grow(true, false);
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleDir) {
		show_direction=!show_direction;
		if (show_direction) PostMessage(_("Show direction map"));
		else PostMessage(_("Don't show direction map"));
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_LoadDirection) {
		const char *file=NULL;
		if (directionmap && directionmap->normal_map && directionmap->normal_map->filename)
			file=directionmap->normal_map->filename;
		app->rundialog(new FileDialog(NULL,"Load normal",_("Load normal map for direction"),
						  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
						  object_id,"loadnormal",
						  FILES_OPEN_ONE|FILES_PREVIEW, 
						  file));
		return 0;

	} else if (action==ENGRAVE_ToggleWarp) {
		always_warp=!always_warp;
		if (always_warp) PostMessage(_("Always remap points when modifying mesh"));
		else PostMessage(_("Don't remap points when modifying mesh"));

		if (always_warp) {
			edata->ReverseSync(false);
		}
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_NextFill || action==ENGRAVE_PreviousFill) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		int ntype=PGROUP_Linear;
		if (action==ENGRAVE_NextFill) {
			if      (group->direction->type==PGROUP_Linear)   ntype=PGROUP_Radial;
			else if (group->direction->type==PGROUP_Radial)   ntype=PGROUP_Circular;
			else if (group->direction->type==PGROUP_Circular) ntype=PGROUP_Spiral;
			else if (group->direction->type==PGROUP_Spiral)   ntype=PGROUP_Linear;
		} else {
			if      (group->direction->type==PGROUP_Linear)   ntype=PGROUP_Spiral;
			else if (group->direction->type==PGROUP_Radial)   ntype=PGROUP_Linear;
			else if (group->direction->type==PGROUP_Circular) ntype=PGROUP_Radial;
			else if (group->direction->type==PGROUP_Spiral)   ntype=PGROUP_Circular;
		}
		group->direction->SetType(ntype);

		if (group->direction->grow_lines) {
			Grow(true, false);
			edata->Sync(true);
		} else {
			group->Fill(edata,-1);
			edata->Sync(false);
		}

		PostMessage(group->direction->TypeName());

		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleLinked) {     
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		bool tolink=!group->linked;
		EngraverFillData *obj;

		for (int c=0; c<selection->n(); c++) {
			obj=dynamic_cast<EngraverFillData*>(selection->e(c)->obj);
			if (!obj) continue;
			for (int c=0; c<obj->groups.n; c++) {
				obj->groups.e[c]->linked=tolink;
			}
		}
		needtodraw=1;
		return 0;

	} else if (action==ENGRAVE_ToggleGroupLinked) {
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		bool tolink=!group->linked;
		for (int c=0; c<edata->groups.n; c++) {
			edata->groups.e[c]->linked=tolink;
		}
		needtodraw=1;
		return 0; 

	} else if (action==ENGRAVE_ToggleCurLinked) {  
		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		group->linked=!group->linked;
		needtodraw=1;
		return 0; 
	}


	return PatchInterface::PerformAction(action);
}

void EngraverFillInterface::UpdateDashCaches(EngraverLineQuality *dash)
{
	if (!edata) return;
	if (!selection) {
		AddToSelection(poc);
	}

	for (int g=0; g<selection->n(); g++) {
		EngraverFillData *obj=dynamic_cast<EngraverFillData *>(selection->e(g)->obj);
		if (!obj) continue;

		for (int c=0; c<obj->groups.n; c++) {
			if (obj->groups.e[c]->dashes==dash) obj->groups.e[c]->UpdateDashCache();
		}
	}
}

/*! If !all, do only current group. If all, then do all groups with the same direction as current group?
 */
int EngraverFillInterface::Grow(bool all, bool allindata)
{
	DBG cerr <<"EngraverFillInterface::Grow("<<(all?"true":"false")<<','<<(allindata?"true":"false")<<endl;
	return 1;
	// *** *** ToDO!!!! needs work
	
//	EngraverPointGroup *group=edata ? edata->GroupFromIndex(current_group) : NULL;
//	if (!group) return 1;
//	if (!group->grow_lines) return 2;
//
//	growpoints.flush();
//	group->GrowLines(edata,
//					 group->spacing->spacing/3,
//					 group->spacing->spacing, NULL,
//					 .01, NULL,
//					 group->directionv,group,
//					 &growpoints,
//					 1000 //iteration limit
//					);
//
//	return 0;
}

/*! For any group in any selected object with the same trace settings as the current group, trace.
 */
int EngraverFillInterface::Trace(bool do_once)
{
	if (!edata) return 1;

	EngraverFillData *obj;
	EngraverPointGroup *group;
	EngraverTraceSettings *curtrace=NULL;
	if (do_once) {
		group=edata->GroupFromIndex(current_group);
		curtrace=group->trace;
	}

	for (int c=0; c<selection->n(); c++) {
		obj=dynamic_cast<EngraverFillData*>(selection->e(c)->obj);
		if (!obj) continue;

		for (int g=0; g<obj->groups.n; g++) {
			group=obj->groups.e[g]; 
			if (!group->trace) continue; //shouldn't happen, but just in case..  
			if (!group->trace->traceobject) continue; //incomplete trace setup

			if (!group->active) continue;
			if (!do_once && !group->trace->continuous_trace) continue;
			if (curtrace && group->trace!=curtrace) continue;

			Affine aa=obj->GetTransformToContext(false, 0);//supposed to be from obj to same "parent" as traceobject
			group->Trace(&aa, viewport);
		}
	}

	needtodraw=1;
	return 0;
}

/*! For any group in any selected object with the same direction settings as the current group, reline.
 * Calls Trace() after relining.
 *
 * which&1 means direction was changed, which&2 means spacing was changed.
 */
int EngraverFillInterface::Reline(bool do_once, int which)
{
	if (!edata) return 1;

	EngraverFillData *obj=NULL;
	EngraverPointGroup *group=NULL;
	EngraverDirection *curdir=NULL;
	EngraverSpacing *curspc=NULL;

	if (do_once) {
		group=edata->GroupFromIndex(current_group);
		curdir=group->direction;
		curspc=group->spacing;
	}

	for (int c=0; c<selection->n(); c++) {
		obj=dynamic_cast<EngraverFillData*>(selection->e(c)->obj);
		if (!obj) continue;

		for (int g=0; g<obj->groups.n; g++) {
			group=obj->groups.e[g]; 
			if (!group->direction) continue; //shouldn't happen, but just in case..  

			if (!group->active) continue;
			//if (!do_once && !group->direction->continuous_trace) continue;

			int yes=0;
			if (curdir && (which%1) && group->direction==curdir) yes=1;
			if (curspc && (which%2) && group->spacing  ==curspc) yes=1;
			if (!yes) continue;

			group->Fill(edata, -1);
			edata->Sync(false);
		}
	}

	Trace();
	needtodraw=1;
	return 0;
}

/*! Return old value of mode.
 * newmode is an id of an item in this->modes.
 */
int EngraverFillInterface::ChangeMode(int newmode)
{
	if (newmode==mode) return mode;

	int c=0;
	for (c=0; c<modes.n(); c++) {
		if (modes.e(c)->id==newmode) break;
	}
	if (c==modes.n()) return mode;

	int oldmode=mode;
	mode=newmode;

	if (newmode!=EMODE_Mesh && data) {
		if (child && data->UsesPath()) RemoveChild();
	}

	if (newmode==EMODE_Mesh) {
		if (data && data->UsesPath()) ActivatePathInterface();
	}


	needtodraw=1;
	PostMessage(ModeTip(mode));
	return oldmode;

}

int EngraverFillInterface::ActivatePathInterface()
{
	int status=PatchInterface::ActivatePathInterface();
	if (dynamic_cast<PathInterface*>(child)) {
		dynamic_cast<PathInterface*>(child)->Setting(PATHI_Defer_Render, true);
	}
	return status;
}

int EngraverFillInterface::Event(const Laxkit::EventData *e_data, const char *mes)
{
	if (!strcmp(mes,"menuevent")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int i     =s->info2; //id of menu item
		unsigned int interf=s->info4; //is curvemapi.object_id if from there

		DBG cerr <<"Engraver Event: i=="<<i<<"  interf="<<interf<<endl;
		
		if (interf==curvemapi.object_id) return curvemapi.Event(e_data,mes);
		if (child && !strcmp(child->whattype(),"CurveMapInterface")) return child->Event(e_data,mes);
		if (i<PATCHA_MAX) return PatchInterface::Event(e_data,mes);

		if ( i==EMODE_Mesh
		  || i==EMODE_Thickness
		  || i==EMODE_Orientation
		  || i==EMODE_Freehand
		  || i==EMODE_Blockout
		  || i==EMODE_Drag 
		  || i==EMODE_PushPull
		  || i==EMODE_AvoidToward
		  || i==EMODE_Twirl
		  || i==EMODE_Turbulence
		  || i==EMODE_Trace
		  || i==EMODE_Resolution) {

			ChangeMode(i);
			return 0;
		}

		if (i==ENGRAVE_Trace_Load) {
			app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));
			return 0;

		} else if (i==ENGRAVE_Trace_Clear) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			if (group->trace->traceobject) group->trace->ClearCache(true);
			return 0;

		} else if (i==ENGRAVE_Trace_Snapshot) {
			if (!edata) return 0;
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			group->TraceFromSnapshot();
			needtodraw=1;
			return 0;
		}

		return 0;

	} else if (!strcmp(mes,"traceobjectmenu")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int id  =s->info2; //id of menu item
		int info=s->info4; //is menuitem info

		if (!edata) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		if (!group) return 0;
		ResourceManager *manager=InterfaceManager::GetDefault(true)->GetResourceManager();

		if (info==-2) {
			if (id==ENGRAVE_Trace_Linear_Gradient) {
				group->InstallTraceGradient('l', NULL, 1);
				needtodraw=1;
				return 0;

			} else if (id==ENGRAVE_Trace_Radial_Gradient) {
				group->InstallTraceGradient('r', NULL, 1);
				needtodraw=1;
				return 0;

			} else if (id==ENGRAVE_Trace_Snapshot) {       
				group->TraceFromSnapshot();
				needtodraw=1;
				return 0;

			} else if (id==ENGRAVE_Trace_Current) {
				//***

			} else if (id==ENGRAVE_Trace_Load) {           
				app->rundialog(new FileDialog(NULL,"Load image",_("Load image for tracing"),
									  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"loadimage",
									  FILES_OPEN_ONE|FILES_PREVIEW, 
									  NULL));

			} else if (id==ENGRAVE_Trace_Save) {           
				app->rundialog(new FileDialog(NULL,"Save image",_("Save trace object image"),
									  ANXWIN_REMEMBER|ANXWIN_CENTER,0,0,0,0,0,
									  object_id,"savetraceimage",
									  FILES_SAVE|FILES_PREVIEW, 
									  "traceobject.jpg"));

			} else if (id==ENGRAVE_Trace_Clear) {
				if (group->trace->traceobject) group->trace->ClearCache(true);
				return 0;

			} else if (id==ENGRAVE_Make_Local) {
				if (!group->trace->traceobject) return 0;
				if (group->trace->traceobject->ResourceOwner()==group->trace)
					return 0; //already local
				
				TraceObject *to=dynamic_cast<TraceObject*>(group->trace->traceobject->duplicate(NULL));
				group->trace->Install(to);
				to->dec_count();
				//UpdatePanelAreas();
				PostMessage(_("Done."));
				needtodraw=1;
				return 0;

			} else if (id==ENGRAVE_Make_Shared) {
				InterfaceManager::GetDefault(true)->Resourcify(group->trace->traceobject);
				//UpdatePanelAreas();
				PostMessage(_("Shared."));
				return 0;
			}

		} else if (info==-1 || info>=0) {
			 //was a favorite resource or another resource
			TraceObject *obj=dynamic_cast<TraceObject*>(manager->FindResource(s->str, "TraceObject", NULL));
			if (obj) {
				//int up=0;
				//if (group->trace->ResourceOwner()==group->trace) up=1;
				group->trace->Install(obj);
				//if (up) UpdatePanelAreas();
			} 
		}

		needtodraw=1;
		return 0;


	} else if (!strcmp(mes,"PathInterface")) {
        if (data) {
            data->UpdateFromPath();

			//if (always_warp && curpoints.n>0) {
			if (always_warp) {
				edata->Sync(false);
				edata->UpdatePositionCache();
			}

			Trace();
            needtodraw=1;
        }
        return 0;

	} else if (!strcmp(mes,"dashlength")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		char *endptr=NULL;
		double d=group->dashes->dash_length=strtod(s->str, &endptr);
		if (endptr!=s->str) {
			group->dashes->dash_length=d;
			group->UpdateDashCache();
			needtodraw=1;
		}
 		return 0;

	} else if (!strcmp(mes,"dashseed")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		char *endptr=NULL;
		int i=strtol(s->str, &endptr, 10);
		if (endptr==s->str || i<=0) i=0;

		group->dashes->randomseed=i;
		group->UpdateDashCache();
		needtodraw=1;

 		return 0;

	} else if (!strcmp(mes,"defaultspacing")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		char *endptr=NULL;
		double d=strtod(s->str, &endptr);
		if (endptr==s->str || d<=0) d=.1;
		group->spacing->spacing=d;

		edata->Sync(false);
		edata->UpdatePositionCache();
		group->UpdateDashCache();
		Trace();

		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"newcolor")) {
 		//got a new color for current group
    	const SimpleColorEventData *ce=dynamic_cast<const SimpleColorEventData *>(e_data);
        if (!ce) return 1;

         // apply message as new current color, pass on to viewport
         // (sent from color box)
        LineStyle linestyle;
        float max=ce->max;
        linestyle.color.red=  (unsigned short) (ce->channels[0]/max*65535);
        linestyle.color.green=(unsigned short) (ce->channels[1]/max*65535);
        linestyle.color.blue= (unsigned short) (ce->channels[2]/max*65535);
        if (ce->numchannels>3) linestyle.color.alpha=(unsigned short) (ce->channels[3]/max*65535);
        else linestyle.color.alpha=65535;

		EngraverFillData *obj=edata;
		SomeData *somedata;
		if (eventobject>0 && eventobject!=edata->object_id && selection) {
			for (int c=0; c<selection->n(); c++) {
				if (eventobject==selection->e(c)->obj->object_id) {
					somedata=selection->e(c)->obj;
					obj=dynamic_cast<EngraverFillData*>(somedata);
					break;
				}
			}
		}

		EngraverPointGroup *group=(obj ? obj->GroupFromIndex(eventgroup) : NULL);
		if (group) group->color=linestyle.color;

		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"renameobject")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		if (eventobject==edata->object_id) {
			edata->Id(s->str);
		}
		eventobject=0;
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"renamegroup")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;

		EngraverFillData *obj=edata;
		if (eventobject>0 && eventobject!=edata->object_id && selection) {
			for (int c=0; c<selection->n(); c++) {
				if (eventobject==selection->e(c)->obj->object_id) {
					obj=dynamic_cast<EngraverFillData*>(selection->e(c)->obj);
					break;
				}
			}
		}

		EngraverPointGroup *group=(obj ? obj->GroupFromIndex(eventgroup) : NULL);
		makestr(group->name,s->str);
		obj->MakeGroupNameUnique(eventgroup);
		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"renametraceobject")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;

		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);
		if (!trace->traceobject) return 0;
		trace->traceobject->Id(s->str);
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"renametrace")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;

		EngraverFillData *obj=edata;
		if (eventobject>0 && eventobject!=edata->object_id && selection) {
			for (int c=0; c<selection->n(); c++) {
				if (eventobject==selection->e(c)->obj->object_id) {
					obj=dynamic_cast<EngraverFillData*>(selection->e(c)->obj);
					break;
				}
			}
		}

		EngraverPointGroup *group=(obj ? obj->GroupFromIndex(eventgroup) : NULL);
		group->trace->Id(s->str);
		//obj->MakeGroupNameUnique(eventgroup); *** NEED TO ENFORCE RESOURCE UNIQUE NAMES
		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"exportsvg")) {
        if (!edata) return 0;

        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
        if (!s) return 1;
        if (!isblank(s->str)) {
			edata->dump_out_svg(s->str);
			PostMessage(_("Exported."));
		}
        return 0;

	} else if (!strcmp(mes,"exportsnapshot")) {
        if (!edata) return 0;

        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
        if (!s) return 0;
        if (isblank(s->str)) return 0;
		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		if (!group) return 0;
		ImageData *idata=group->CreateFromSnapshot();
		if (!idata) return 0;
		save_image(idata->image,s->str,NULL);
		idata->dec_count();

		// *** note: no check for writability... maybe save failed!!
		PostMessage(_("Snapshot exported."));
        return 0;

	} else if (!strcmp(mes,"savetraceimage")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) return 0;

		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		if (!group) return 0;
		LaxImage *img=NULL;
		ImageData *idata=NULL;
		if (group->trace->traceobject) idata=dynamic_cast<ImageData*>(group->trace->traceobject->object);
		if (!idata) { PostMessage(_("Can't save that type of trace object. Lazy programmers.")); return 0; }
		img=idata->image;
		if (!img) { PostMessage(_("Missing image for trace object!")); return 0; }
		if (save_image(img, s->str, NULL)==0) PostMessage(_("Saved."));
		else PostMessage(_("Unable to save."));

		return 0;

	} else if (!strcmp(mes,"loadimage")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) return 0;
		LaxImage *img=load_image(s->str);
		const char *bname=lax_basename(s->str);
		if (!img) {
			char buf[strlen(_("Could not load %s"))+strlen(bname)+1];
			sprintf(buf,_("Could not load %s"),bname);
			PostMessage(buf);
			return 0;
		}

		int sx=dp->Maxx-dp->Minx;
		int sy=dp->Maxy-dp->Miny;
		flatpoint p1=screentoreal(dp->Minx+sx*.1,dp->Miny+sy*.1);
		flatpoint p2=screentoreal(dp->Maxx-sx*.1,dp->Maxy-sy*.1);
		DoubleBBox box;
		box.addtobounds(p1);
		box.addtobounds(p2);

		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		EngraverTraceSettings *trace=(group ? group->trace : &default_trace);

		if (trace->traceobject) {
			trace->ClearCache(true);
			//trace->traceobject->dec_count();
			//trace->traceobject=NULL;
		}
		ImageData *idata=new ImageData();
		idata->SetImage(img, NULL);
		img->dec_count();
		trace->Install(TraceObject::TRACE_ImageFile, idata);
		trace->traceobject->object->fitto(NULL,&box,50,50,2);
		traceobjects.push(trace->traceobject);

		trace->ClearCache(false);

		needtodraw=1;
		PostMessage(_("Image to trace loaded."));
		return 0;

	} else if (!strcmp(mes,"loadnormal")) {
        const StrEventData *s=dynamic_cast<const StrEventData *>(e_data);
		if (!s || isblank(s->str)) return 0;

		bool newmap=(directionmap?false:true);
		if (!directionmap) directionmap=new NormalDirectionMap();
		int status=directionmap->Load(s->str);
		if (status!=0) {
			if (newmap) { delete directionmap; directionmap=NULL; }

			const char *bname=lax_basename(s->str);
			char buf[strlen(_("Could not load %s"))+strlen(bname)+1];
			sprintf(buf,_("Could not load %s"),bname);
			PostMessage(buf);
			return 0;
		}

		 //fit into a box 80% the size of viewport
		int sx=dp->Maxx-dp->Minx;
		int sy=dp->Maxy-dp->Miny;
		flatpoint p1=screentoreal(dp->Minx+sx*.1,dp->Miny+sy*.1);
		flatpoint p2=screentoreal(dp->Maxx-sx*.1,dp->Maxy-sy*.1);
		DoubleBBox box;
		box.addtobounds(p1);
		box.addtobounds(p2);

		ImageData *idata=new ImageData();
		idata->SetImage(directionmap->normal_map, NULL);
		idata->fitto(NULL,&box,50,50,2);
		directionmap->m.set(*idata);
		directionmap->m.Invert();
		idata->dec_count();

		needtodraw=1;
		PostMessage(_("Normal map loaded."));
		return 0;

	} else if (!strcmp(mes,"sharedirection")
				|| !strcmp(mes,"sharedash")
				|| !strcmp(mes,"sharespacing")
				|| !strcmp(mes,"sharetrace")
					) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		int i =s->info2; //id of menu item
		int info =s->info4; //info of menu item

		int what=0;
		if (!strcmp(mes,"sharedirection"))    what=ENGRAVE_Direction;
		else if (!strcmp(mes,"sharedash"))    what=ENGRAVE_Dashes;
		else if (!strcmp(mes,"sharespacing")) what=ENGRAVE_Spacing;
		else if (!strcmp(mes,"sharetrace"))   what=ENGRAVE_Tracing;


		if (info==-3) {
			int update=0;
			if (i==1) {
				 //Push to all
				PushToAll(what, NULL,-1);
				update=1;

			} else if (i>=20000) { //push to
				i-=20000;
				PushSettings(what, NULL,-1, NULL,i);
				update=1;

			} else if (i>=10000 && i<20000) { //share from
				i-=10000;
				PushSettings(what, NULL,i, NULL,-1);
				update=1;
			}
			if (update) UpdatePanelAreas();
			return 0;

		} else if (info==-4) {
			if (i==ENGRAVE_Make_Local) { //new based on currently ref'd
				EngraverPointGroup *cur =edata->GroupFromIndex(current_group);
				if (cur) {
					if (what==ENGRAVE_Direction)    cur->InstallDirection(    cur->direction->duplicate(),1);
					else if (what==ENGRAVE_Dashes)  cur->InstallDashes(       cur->dashes   ->duplicate(),1);
					else if (what==ENGRAVE_Spacing) cur->InstallSpacing(      cur->spacing  ->duplicate(),1);
					else if (what==ENGRAVE_Tracing) cur->InstallTraceSettings(cur->trace    ->duplicate(),1);

					UpdatePanelAreas();
					needtodraw=1;
				}
				return 0;

			} else if (i==ENGRAVE_Make_Shared) {
				 //make resource
				EngraverPointGroup *group=edata->GroupFromIndex(current_group);
				if (group) {
					if (what==ENGRAVE_Direction)    InterfaceManager::GetDefault(true)->Resourcify(group->direction);
					else if (what==ENGRAVE_Dashes)  InterfaceManager::GetDefault(true)->Resourcify(group->dashes);
					else if (what==ENGRAVE_Spacing) InterfaceManager::GetDefault(true)->Resourcify(group->spacing);
					else if (what==ENGRAVE_Tracing) InterfaceManager::GetDefault(true)->Resourcify(group->trace);
				}
				UpdatePanelAreas();
				PostMessage(_("Done."));
				return 0;
			}
			return 0;

		} else {
			 //was resource selection
			InterfaceManager *imanager=InterfaceManager::GetDefault(true);
			ResourceManager *rm=imanager->GetResourceManager();
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);

			const char *whats;
			if (what==ENGRAVE_Direction)    whats="EngraverDirection";
			else if (what==ENGRAVE_Dashes)  whats="EngraverLineQuality";
			else if (what==ENGRAVE_Spacing) whats="EngraverSpacing";
			else if (what==ENGRAVE_Tracing) whats="EngraverTraceSettings";

			anObject *obj=rm->FindResource(s->str,whats);
			if (obj && group) {
				if (what==ENGRAVE_Direction)    group->InstallDirection(    dynamic_cast<EngraverDirection*>(obj),0);
				else if (what==ENGRAVE_Dashes)  group->InstallDashes(       dynamic_cast<EngraverLineQuality*>(obj),0);
				else if (what==ENGRAVE_Spacing) group->InstallSpacing(      dynamic_cast<EngraverSpacing*>(obj),0);
				else if (what==ENGRAVE_Tracing) group->InstallTraceSettings(dynamic_cast<EngraverTraceSettings*>(obj),0);

				PostMessage(_("Installed."));
				UpdatePanelAreas();
				needtodraw=1;
			} else PostMessage(_("Could not find resource."));
		}

		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"renamedash")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		if (group) group->dashes->Id(s->str);

		//obj->MakeGroupNameUnique(eventgroup); *** NEED TO ENFORCE RESOURCE UNIQUE NAMES
		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"renamespacing")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		if (group) group->spacing->Id(s->str);

		//obj->MakeGroupNameUnique(eventgroup); *** NEED TO ENFORCE RESOURCE UNIQUE NAMES
		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"renamedirection")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		if (group) group->direction->Id(s->str);

		//obj->MakeGroupNameUnique(eventgroup); *** NEED TO ENFORCE RESOURCE UNIQUE NAMES
		needtodraw=1;
 		return 0;

	} else if (!strcmp(mes,"quickadjust")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		
		double factor;
		if (DoubleAttribute(s->str, &factor, NULL)) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			group->QuickAdjust(factor);
		}
		return 0; 

	} else if (!strcmp(mes,"orientspacing")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		
		double spacing;
		if (DoubleAttribute(s->str, &spacing, NULL)) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			group->spacing->spacing=spacing;

			if (group->direction->grow_lines) Grow(false, false);
			else group->Fill(edata, -1);

			edata->Sync(false);
			edata->UpdatePositionCache();
			group->UpdateDashCache();
			Trace();

			needtodraw=1;
		}
		return 0;

	} else if (!strcmp(mes,"orientdirection")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		
		double angle;
		if (DoubleAttribute(s->str, &angle, NULL)) {
			EngraverPointGroup *group=edata->GroupFromIndex(current_group);
			angle*=M_PI/180.;
			group->directionv=flatvector(cos(angle),sin(angle));

			if (group->direction->grow_lines) Grow(false, false);
			else group->Fill(edata, -1);

			edata->Sync(false);
			edata->UpdatePositionCache();
			group->UpdateDashCache();
			Trace();

			needtodraw=1;
		}
		return 0;

		//-----------------------Direction related
	} else if (!strcmp(mes,"directiontype")) {
		EngraverPointGroup *group=(edata ? edata->GroupFromIndex(current_group) : NULL);
		if (!group) return 0;

    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
		group->direction->SetType(s->info2);

		if (group->direction->grow_lines) {
			Grow(true, false);
			edata->Sync(true);
		} else {
			group->Fill(edata,-1);
			edata->Sync(false);
		}

		PostMessage(group->direction->TypeName());
		needtodraw=1;
		return 0;

	} else if (!strcmp(mes,"lineprofilemenu")) {
    	const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);

		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		if (group) {
			InterfaceManager *imanager=InterfaceManager::GetDefault(true);
			ResourceManager *rm=imanager->GetResourceManager();

			if (s->info2==-1) {
				group->direction->InstallProfile(NULL,0);
			} else {
				LineProfile *obj=dynamic_cast<LineProfile*>(rm->FindResource(s->str,"LineProfile"));
				if (obj) {
					group->direction->InstallProfile(obj, 0);
				}
			}

			Reline(true,1);
			needtodraw=1;
		}
		return 0;

	} else if (!strcmp(mes,"directionseed")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);
		char *endptr=NULL;
		int i=strtol(s->str, &endptr, 10);
		if (endptr==s->str || i<=0) i=-1;

		group->direction->seed=i;
		needtodraw=1;

 		return 0;


		//-----------------------Spacing related
	} else if (!strcmp(mes,"spacingmenu")) {
        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
        if (!edata || isblank(s->str)) return 0;
		EngraverPointGroup *group=edata->GroupFromIndex(current_group);

		int i=s->info2;

		if (i==SPCMENU_New_Snapshot) {
			ImageData *idata=group->SpacingSnapshot();

			DBG cerr <<" Saving spacing snapshot to test-spacing.png.."<<endl;
			DBG save_image(idata->image, "test-spacing.png", NULL);

			idata->dec_count(); 

		} else if (i==SPCMENU_Load_Map) {
		} else if (i==SPCMENU_Save_Map) {
		} else if (i==SPCMENU_Clear_Map) {
		}

		return 0;


		//-----------------------other
	} else if (!strcmp(mes,"FreehandInterface")) {
		 //got new freehand mesh

        const RefCountedEventData *s=dynamic_cast<const RefCountedEventData *>(e_data);
		if (!s) return 1;

		PatchData *patch=dynamic_cast<PatchData*>(const_cast<anObject*>(s->TheObject()));
		if (!patch) return 1;


		deletedata(false);
		EngraverFillData *newdata=dynamic_cast<EngraverFillData*>(newPatchData(0,0,1,1,1,1,PATCH_SMOOTH));
		newdata->m(patch->m());
		newdata->CopyMeshPoints(patch, true);
		DBG newdata->dump_out(stderr,0,0,NULL);

		//newdata->UpdateFromPath();
		newdata->FindBBox();
		newdata->DefaultSpacing(-1);
		newdata->groups.e[0]->Fill(newdata, 1./dp->Getmag());
		//newdata->UpdateDashCache();
		for (int c=0; c<newdata->groups.n; c++) {
			newdata->groups.e[c]->UpdateBezCache();
			newdata->groups.e[c]->UpdateDashCache();
		} 
		newdata->Sync(false);

		if (viewport) {
			ObjectContext *oc=NULL;
			viewport->NewData(newdata,&oc);

			if (oc) poc=oc->duplicate();
		}
		data=newdata;
		data->linestyle=linestyle;
		data->FindBBox();
		curpoints.flush();

		edata=dynamic_cast<EngraverFillData*>(data);

		AddToSelection(poc);
		ChangeMode(EMODE_Mesh);

		needtodraw=1;
		return 0;
    }

    return 1;
}

 /*! Returns 0 for success or nonzero for error.
 */
int EngraverFillInterface::PushToAll(int what, EngraverPointGroup *from,int fromi)
{
	if (!from) {
		if (fromi<0 && edata) from=edata->GroupFromIndex(current_group);
		else if (fromi>=0) { 
			int gindex=-1;
			EngraverFillData *obj=GroupFromLineIndex(fromi, &gindex);
			if (!obj || gindex<0) return 1;
			from=obj->GroupFromIndex(gindex);
		}
		if (!from) return 1;
	}

	if (!selection) return 2;
	EngraverFillData *obj=NULL;

	for (int c=0; c<selection->n(); c++) {
		obj=dynamic_cast<EngraverFillData*>(selection->e(c)->obj);
		if (!obj) continue;

		for (int c2=0; c2<obj->groups.n; c2++) {
			PushSettings(what, from,-1, obj->groups.e[c2],-1);			
		} 
	}

	return 0;
}

/*! If from==NULL and fromi<0, then use current group for from. If from==NULL and fromi>=0, then fromi
 * is the line index as you would see it in the group list.
 * If to==NULL and toi<0, then use current group.
 *
 * what should be one of ENGRAVE_Tracing, ENGRAVE_Dashes, ENGRAVE_Direction, or ENGRAVE_Spacing.
 *
 * Returns 0 for success or nonzero for error.
 */
int EngraverFillInterface::PushSettings(int what, EngraverPointGroup *from,int fromi, EngraverPointGroup *to,int toi)
{
	if (!from) {
		if (fromi<0 && edata) from=edata->GroupFromIndex(current_group);
		else if (fromi>=0) { 
			int gindex=-1;
			EngraverFillData *obj=GroupFromLineIndex(fromi, &gindex);
			if (!obj || gindex<0) return 1;
			from=obj->GroupFromIndex(gindex);
		}
		if (!from) return 1;
	}

	if (!to) {
		if (toi==-1 && edata) to=edata->GroupFromIndex(current_group);
		else if (toi>=0) { 
			int gindex=-1;
			EngraverFillData *obj=GroupFromLineIndex(toi, &gindex);
			if (!obj || gindex<0) return 1;
			to=obj->GroupFromIndex(gindex);
		}
	}

	if (!from || !to) return 3;

	if (what==ENGRAVE_Tracing) {
		if (from->trace!=to->trace) {
			 //trace->value_to_weight is not fully ref counted, so we must beware
			if (curvemapi.GetInfo()==to->trace->value_to_weight) curvemapi.SetInfo(NULL);

			if (from->trace->ResourceOwner()==from) {
				 //trace was owned by from, need to make from->trace be a shared resource
				from->trace->SetResourceOwner(NULL);
				ResourceManager *resourcemanager=InterfaceManager::GetDefault(true)->GetResourceManager();
				resourcemanager->AddResource(from->trace->whattype(), from->trace, NULL, 
						from->trace->Id(), from->trace->Id(), NULL, NULL, NULL);
				DBG cerr <<"Trace Resource added:"<<endl;
				DBG resourcemanager->dump_out(stderr, 2, 0, NULL); 
			}
			to->InstallTraceSettings(from->trace,0);

		}

	} else if (what==ENGRAVE_Dashes) {
		if (from->dashes!=to->dashes) {
			if (from->dashes->ResourceOwner()==from) {
				 //dashes was owned by from, need to make from->dashes be a shared resource
				from->dashes->SetResourceOwner(NULL);
				ResourceManager *resourcemanager=InterfaceManager::GetDefault(true)->GetResourceManager();
				resourcemanager->AddResource(from->dashes->whattype(), from->dashes, NULL, 
						from->dashes->Id(), from->dashes->Id(), NULL, NULL, NULL);
				DBG cerr <<"Dashes Resource added "<<from->dashes->Id()<<endl;
			}
			to->InstallDashes(from->dashes,0);

		}

	} else if (what==ENGRAVE_Direction) {
		if (from->direction!=to->direction) {
			if (from->direction->ResourceOwner()==from) {
				 //direction was owned by from, need to make from->direction be a shared resource
				from->direction->SetResourceOwner(NULL);
				ResourceManager *resourcemanager=InterfaceManager::GetDefault(true)->GetResourceManager();
				resourcemanager->AddResource(from->direction->whattype(), from->direction, NULL, 
						from->direction->Id(), from->direction->Id(), NULL, NULL, NULL);
				DBG cerr <<"Dashes Resource added "<<from->direction->Id()<<endl;
			}
			to->InstallDirection(from->direction,0); 
		}

	} else if (what==ENGRAVE_Spacing) {
		if (from->spacing!=to->spacing) {
			if (from->spacing->ResourceOwner()==from) {
				 //spacing was owned by from, need to make from->spacing be a shared resource
				from->spacing->SetResourceOwner(NULL);
				ResourceManager *resourcemanager=InterfaceManager::GetDefault(true)->GetResourceManager();
				resourcemanager->AddResource(from->spacing->whattype(), from->spacing, NULL, 
						from->spacing->Id(), from->spacing->Id(), NULL, NULL, NULL);
				DBG cerr <<"Dashes Resource added "<<from->spacing->Id()<<endl;
			}
			to->InstallSpacing(from->spacing,0); 
		}

	} else return 5;

	needtodraw=1;
	return 0;
}

Laxkit::MenuInfo *EngraverFillInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	if (lasthover==ENGRAVE_Trace_Curve) {
		MenuInfo *m=curvemapi.ContextMenu(x,y,deviceid, menu);
		int oldn=m->n();
		for (int c=oldn; c<m->n(); c++) {
			m->e(c)->info=curvemapi.object_id;
		}
		return m;
	}

	if (child && !strcmp(child->whattype(),"CurveMapInterface")) {
		menu=child->ContextMenu(x,y,deviceid,menu);
		if (menu) return menu;
	}

	if (!menu) menu=new MenuInfo();

	if (mode==EMODE_Mesh) {
		menu=PatchInterface::ContextMenu(x,y,deviceid,menu);
//		if (child) {
//			if (!strcmp(child->whattype(),"PathInterface")) {
//				int oldn=(menu ? menu->n() : 0);
//				menu=child->ContextMenu(x,y,deviceid, menu);
//				for (int c=oldn; c<menu->n(); c++) {
//					menu->e(c)->info=child->object_id;
//				}
//			}
//		}
	}

	if (menu->n()!=0) menu->AddSep(_("Engraver"));

	int category=0, index=-1, detail=-1;
	int where=scanEngraving(x,y,0, &category, &index, &detail);
	if (mode==EMODE_Trace
		 || where==ENGRAVE_Trace_Box
		 || where==ENGRAVE_Trace_Once
		 || where==ENGRAVE_Trace_Load
		 || where==ENGRAVE_Trace_Continuous) {
		menu->AddSep(_("Trace"));
		menu->AddItem(_("Load image to trace..."),ENGRAVE_Trace_Load);
		menu->AddItem(_("Clear trace object"), ENGRAVE_Trace_Clear);
		menu->AddItem(_("Snapshot from current"), ENGRAVE_Trace_Snapshot);
	}

//	if (mode==EMODE_Orientation) {
//		menu->AddSep(_("Line type"));
//		menu->AddItem(_("Linear"),ENGRAVE_, LAX_OFF);
//		menu->AddItem(_("Radial"),ENGRAVE_, LAX_OFF);
//		menu->AddItem(_("Circles"),ENGRAVE_, LAX_OFF);
//		menu->AddItem(_("Spiral"),ENGRAVE_, LAX_OFF);
//		menu->AddItem(_("Shell"),ENGRAVE_, LAX_OFF);
//		menu->AddItem(_("S"),ENGRAVE_, LAX_OFF);
//		menu->AddItem(_("Manual"),ENGRAVE_, LAX_OFF);
//		menu->AddItem(_(""),ENGRAVE_, LAX_OFF);
//	}

	menu->AddSep(_("Mode"));
	MenuItem *i;
	for (int c=0; c<modes.n(); c++) {
		i=modes.e(c);
		menu->AddItem(i->name, i->id, LAX_OFF|LAX_ISTOGGLE|(mode==i->id ? LAX_CHECKED : 0), 0);
	}

	return menu;
}


Laxkit::ShortcutHandler *EngraverFillInterface::GetShortcuts()
{
    if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

	PatchInterface::GetShortcuts();

	 //convert all patch shortcuts to EMODE_Mesh mode
	WindowAction *a;
	ShortcutDef *s;
	for (int c=0; c<sc->NumActions(); c++)   { a=sc->Action(c);   a->mode=EMODE_Mesh; }
	for (int c=0; c<sc->NumShortcuts(); c++) { s=sc->Shortcut(c); s->mode=EMODE_Mesh; }

	 //any mode shortcuts
	sc->Add(ENGRAVE_SwitchMode,     'm',0,0,          "SwitchMode",  _("Switch edit mode"),NULL,0);
	sc->Add(ENGRAVE_SwitchModeR,    'M',ShiftMask,0,  "SwitchModeR", _("Switch to previous edit mode"),NULL,0);
	sc->Add(ENGRAVE_ExportSvg,      'f',0,0,          "ExportSvg",   _("Export Svg"),NULL,0);
	sc->Add(ENGRAVE_ExportSnapshot, 'F',ShiftMask,0, "ExportSnapshot",   _("Export snapshot"),NULL,0);
	sc->Add(ENGRAVE_RotateDir,      'r',0,0,          "RotateDir",   _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_RotateDirR,     'R',ShiftMask,0,  "RotateDirR",  _("Rotate default line direction"),NULL,0);
	sc->Add(ENGRAVE_SpacingInc,     's',0,0,          "SpacingInc",  _("Increase default spacing"),NULL,0);
	sc->Add(ENGRAVE_SpacingDec,     'S',ShiftMask,0,  "SpacingDec",  _("Decrease default spacing"),NULL,0);
	sc->Add(ENGRAVE_ResolutionInc,  'r',ControlMask,0,"ResolutionInc",  _("Increase default resolution"),NULL,0);
	sc->Add(ENGRAVE_ResolutionDec,  'R',ControlMask|ShiftMask,0,"ResolutionDec",_("Decrease default resolution"),NULL,0);
	sc->Add(ENGRAVE_ShowPoints,     'p',0,0,          "ShowPoints",  _("Toggle showing sample points"),NULL,0);
	sc->Add(ENGRAVE_ShowPointsN,    'p',ControlMask,0,"ShowPointsN", _("Toggle showing sample point numbers"),NULL,0);
	sc->Add(ENGRAVE_MorePoints,     'P',ControlMask|ShiftMask,0,"MorePoints",  _("Subdivide all lines to have more sample points"),NULL,0);
	sc->Add(ENGRAVE_TogglePanel,    'c',0,0,          "TogglePanel", _("Toggle showing control panel"),NULL,0);
	sc->Add(ENGRAVE_ToggleGrow,     'g',0,0,          "ToggleGrow",  _("Toggle grow mode"),NULL,0);
	sc->Add(ENGRAVE_ToggleWarp,     'w',0,0,          "ToggleWarp",  _("Toggle warping when modifying mesh"),NULL,0);
	sc->Add(ENGRAVE_ToggleDir,      'd',0,0,          "ToggleDir",   _("Toggle showing direction map"),NULL,0);
	sc->Add(ENGRAVE_ToggleShowTrace,']',0,0,          "ToggleShowTrace",_("Toggle showing the trace object"),NULL,0);
	sc->Add(ENGRAVE_LoadDirection,  'd',ControlMask,0,"LoadDir",      _("Load a normal map for direction"),NULL,0);
	sc->Add(ENGRAVE_NextGroup,      LAX_Pgdown,0,0,   "NextGroup",    _("Next group"),NULL,0);
	sc->Add(ENGRAVE_PreviousGroup,  LAX_Pgup,0,0,     "PreviousGroup",_("Previous group"),NULL,0);
	sc->Add(ENGRAVE_NextFill,     LAX_Left, 0,EMODE_Orientation,  "NextFillType",     _("Switch to next fill type"),NULL,0);
	sc->Add(ENGRAVE_PreviousFill, LAX_Right,0,EMODE_Orientation,  "PreviousFillType", _("Switch to previous fill type"),NULL,0);

	sc->Add(ENGRAVE_ToggleTracePanel,  '1',0,0,     "ToggleTracePanel",  _("Toggle trace panel"),NULL,0);
	sc->Add(ENGRAVE_ToggleDashPanel,   '2',0,0,     "ToggleDashPanel",   _("Toggle dash panel"),NULL,0);
	sc->Add(ENGRAVE_ToggleDirPanel,    '3',0,0,     "ToggleDirPanel",    _("Toggle direction panel"),NULL,0);
	sc->Add(ENGRAVE_ToggleSpacingPanel,'4',0,0,     "ToggleSpacingPanel",_("Toggle spacing panel"),NULL,0);

	sc->Add(ENGRAVE_ToggleLinked,     'l',0,0,          "ToggleLinked",     _("Toggle if all are linked or unlinked"),NULL,0);
	sc->Add(ENGRAVE_ToggleGroupLinked,'L',ShiftMask,0,  "ToggleGroupLinked",_("Toggle if all in group are linked or not"),NULL,0);
	sc->Add(ENGRAVE_ToggleCurLinked,  'l',ControlMask,0,"ToggleCurLinked",  _("Toggle current group being linked"),NULL,0);

	return sc;
}

int EngraverFillInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	DBG cerr <<"in EngraverFillInterface::CharInput"<<endl;
	
	if (child) {
		if (ch==LAX_Esc) {
			if (child) RemoveChild(); 
			return 0;
		}
	}

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_AvoidToward
		  || mode==EMODE_Twirl
		  ) {

		if (ch==LAX_Control) {
			submode=1;
			//if (state&ShiftMask) submode=3;
			needtodraw=1;
			return 0;
		} else if (ch==LAX_Shift) {
			submode=2;
			sensbox.width =(dp->Maxx-dp->Minx)/4;
			sensbox.height=dp->textheight();
			sensbox.x = (dp->Maxx+dp->Minx-sensbox.width)/2;
			sensbox.y = dp->Miny*.2 + dp->Maxy*.8;
			//if (state&ControlMask) submode=3;
			needtodraw=1;
			return 0;
		}
	}

    if (!sc) GetShortcuts();
    int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
    if (action>=0) {
        return PerformAction(action);
    }


	if (mode==EMODE_Mesh && lasthovercategory!=ENGRAVE_Panel) return PatchInterface::CharInput(ch,buffer,len,state,d);


	return 1;
}

int EngraverFillInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d)
{
	if (child) return 1;

	if (mode==EMODE_Mesh && lasthovercategory!=ENGRAVE_Panel) return PatchInterface::KeyUp(ch,state,d);

	if (	 mode==EMODE_Thickness
		  || mode==EMODE_Blockout
		  || mode==EMODE_Turbulence
		  || mode==EMODE_Drag
		  || mode==EMODE_PushPull
		  || mode==EMODE_AvoidToward
		  || mode==EMODE_Twirl
		  ) {

		if (ch==LAX_Control || ch==LAX_Shift) {
			submode=0;
			needtodraw=1;
			return 0;
		}
	}

	return 1;
}


} // namespace LaxInterfaces

