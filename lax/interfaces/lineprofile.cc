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
//    Copyright (C) 2015 by Tom Lechner
//


#include <lax/interfaces/lineprofile.h>
#include <lax/interfaces/interfacemanager.h>
#include <lax/interfaces/somedatafactory.h>

#include <lax/attributes.h>
#include <lax/language.h>


using namespace Laxkit;


namespace LaxInterfaces {



//--------------------------- standard LineProfiles -----------------------------


anObject *NewLineProfile(int p, anObject *refobj) { return new LineProfile; }


/*! Create and install LineProfile type (if factory!=NULL), and also install to default resource manager
 * default LineProfile objects (if resources!=NULL), like flat, rising, falling, etc.
 *
 * If the type "LineProfile" already exists in factory, then assume everything is already initialized
 * and return -1.
 *
 * Else return 0 for success.
 */
int InstallDefaultLineProfiles(ObjectFactory *factory, ResourceManager *resources)
{
	if (factory) {
		int status=factory->DefineNewObject(OBJ_LineProfile, "LineProfile", NewLineProfile, NULL, 0);
		if (status==-1) return -1;
	}


	if (resources) {
		LineProfile *profile;
		resources->AddResourceType("LineProfile", _("Line Profile"), NULL, NULL);

		 //flat
		profile=new LineProfile;
		profile->AddNode(0,0,1,0);
		profile->Id(_("Flat"));
		resources->AddResource("LineProfile", profile, NULL, profile->Id(), profile->Id(), NULL, NULL, NULL, true);
		profile->dec_count();		

		//rising
		profile=new LineProfile;
		profile->AddNode(0,0,0,0);
		profile->AddNode(1,0,1,0);
		profile->Id(_("Rising"));
		resources->AddResource("LineProfile", profile, NULL, profile->Id(), profile->Id(), NULL, NULL, NULL, true);
		profile->dec_count();		

		//falling
		profile=new LineProfile;
		profile->AddNode(0,0,1,0);
		profile->AddNode(1,0,0,0);
		profile->Id(_("Falling"));
		resources->AddResource("LineProfile", profile, NULL, profile->Id(), profile->Id(), NULL, NULL, NULL, true);
		profile->dec_count();		

		//eye
		profile=new LineProfile;
		profile->AddNode( 0,0,0,0);
		profile->AddNode(.5,0,1,0);
		profile->AddNode( 1,0,0,0);
		profile->Id(_("Eye"));
		resources->AddResource("LineProfile", profile, NULL, profile->Id(), profile->Id(), NULL, NULL, NULL, true);
		profile->dec_count();		

		//eye slit, kind of sine wave with bump in the middle
		// ***
	}



	//profiles.push(NULL);
	////LineProfile **pf=profiles.extractArrays(NULL,NULL);
	//return pf;

	return 0;
}

//--------------------------- LineProfile -----------------------------
/*! \class LineProfile
 *
 * Stores the shape of a line, abstracted from the actual windings of the path.
 * This shape is defined by any number of weight nodes, which define the
 * offset, width, and angle at that point.
 */


LineProfile::LineProfile()
{
	//for preview size:
	maxx = 150;
	maxy = 20;

	mint         = 0;
	maxt         = 1;
	max_height   = 0;
	defaultwidth = 1;
	wrap         = 0;

	needtorecache  = 1;
	nodes_mod_time = 0;
	cache_mod_time = 0;

	start            = 0;
	start_type       = 0;
	start_rand_width = 0;

	end            = 1;
	end_type       = 0;
	end_rand_width = 0;

	width = offset = angle = NULL;
}

LineProfile::~LineProfile()
{
	if (width)   width  ->dec_count();
	if (offset)  offset ->dec_count();
	if (angle)   angle  ->dec_count();
}

int LineProfile::renderToBufferImage(LaxImage *image)
{
	// *** should preview angle too

	// unsigned char *data = image->getImageBuffer();
	// int pwidth = image->w();
	// int pheight = image->h();
	// memset(data, 255, pwidth*pheight*4); //all white

	// double w,doff,angle;
	// int v, off;
	// for (int x=0; x<pwidth; x++) {
	// 	GetWeight((double)x/pwidth, &w,&doff,&angle);
	// 	w *= pheight/2;
	// 	off = pheight/2 + doff*pheight/2;

	// 	int lower = off-w;
	// 	v = .5 + 255 * (1-(w - int(w))); //antialias edges
	// 	if (v < 0) v = 0; else if (v > 255) v = 255;
	// 	if (lower>=0 && lower<pheight) {
	// 		data[(lower*pwidth + x)*4  ] = v;//b
	// 		data[(lower*pwidth + x)*4+1] = v;//g
	// 		data[(lower*pwidth + x)*4+2] = v;//r
	// 	}
	// 	int upper = off+w;
	// 	if (upper>=0 && upper<pheight) {
	// 		data[(upper*pwidth + x)*4  ] = v;//b
	// 		data[(upper*pwidth + x)*4+1] = v;//g
	// 		data[(upper*pwidth + x)*4+2] = v;//r
	// 	}
	// 	for (int y = lower+1; y<upper; y++) {
	// 		if (y>=0 && y<pheight) {
	// 			data[(y*pwidth + x)*4  ]=0;//b
	// 			data[(y*pwidth + x)*4+1]=0;//g
	// 			data[(y*pwidth + x)*4+2]=0;//r
	// 		}
	// 	}
	// }

	// image->doneWithBuffer(data);

	// ------------
	
	Displayer *dp = InterfaceManager::GetDefault(true)->GetPreviewDisplayer();
	dp->MakeCurrent(image);

	int pwidth = image->w();
	int pheight = image->h();
	dp->NewBG(1.,1.,1.);
	dp->NewBG(0.,0.,0.);
	
	double w,off,angle;
	for (int x=0; x<pwidth; x++) {
		GetWeight((double)x/pwidth, &w,&off,&angle);
		w *= pheight/2;
		off = pheight/2 + off*pheight/2;

		// double lower = off-w;
		double upper = off+w;

		if (x == 0) dp->moveto(x,upper);
		else dp->lineto(x,upper);
	}
	for (int x=pwidth-1; x >= 0; x--) {
		GetWeight((double)x/pwidth, &w,&off,&angle);
		w *= pheight/2;
		off = pheight/2 + off*pheight/2;

		double lower = off-w;
		// double upper = off+w;

		dp->lineto(x,lower);
	}
	dp->fill(0);

	return 0;
}

/*! t must range from 0 to 1.
 *
 * Return value is 0 for success.
 */
int LineProfile::GetWeight(double t, double *width_ret, double *offset_ret, double *angle_ret)
{
	if (needtorecache) UpdateCache();

	if (pathweights.n==0) {
		if (width_ret)  *width_ret =defaultwidth;
		if (offset_ret) *offset_ret=0;
		if (angle_ret)  *angle_ret =0;

	} else {
		if (width_ret || offset_ret) {
			if (width_ret)  *width_ret  = width ->f(t); 
			if (offset_ret) *offset_ret = offset->f(t); 
		}    
		if (angle_ret) *angle_ret=angle->f(t);
	}

	return 0;

}

void LineProfile::UpdateCache()
{ 
	if (!angle ) angle =new CurveInfo;
	if (!offset) offset=new CurveInfo;
	if (!width ) width =new CurveInfo;

	angle->Reset(true); //removes all points and leaves blank
	offset->Reset(true);
	width ->Reset(true);

	flatpoint wtop,wbottom;
	flatpoint woffset,wwidth;

	double ymax=0,ymin=0;
	double amax=0,amin=0;

	bool isclosed=wrap;
	bool hasangle=Angled();

	int n=2; 
	if (!isclosed) n--; //path is open, so num_segments = num_vertices - 1

	//set the x range for the cached curves
	offset->SetXBounds(0,n,NULL,false);
	width ->SetXBounds(0,n,NULL,false);
	angle->SetXBounds(0,n,NULL,false);

	offset->Wrap(isclosed);
	width ->Wrap(isclosed);
	angle->Wrap(isclosed);

	//gather bounds and points to define the offset curves
	if (pathweights.n==0) {
		//no actual weight nodes
		woffset=flatpoint(0,0);
		wwidth =flatpoint(0,defaultwidth);
		ymin=ymax=woffset.y;
		amin=amax=0;

	} else {
		amin=amax=pathweights.e[0]->angle;

		for (int c=0; c<pathweights.n; c++) {
			woffset=flatpoint(pathweights.e[c]->t, pathweights.e[c]->offset);
			wwidth =flatpoint(pathweights.e[c]->t, pathweights.e[c]->width);

			if (woffset.y>ymax) ymax=woffset.y;
			if (woffset.y<ymin) ymin=woffset.y;
			if (wwidth.y>ymax)  ymax=wwidth.y;
			if (wwidth.y<ymin)  ymin=wwidth.y;

			if (hasangle) {
				if (pathweights.e[c]->angle>amax) amax=pathweights.e[c]->angle;
				if (pathweights.e[c]->angle<amin) amin=pathweights.e[c]->angle;
			}
		}
	}

	//finalize width and offset bounds
	if (ymax==ymin) ymin=ymax-1;
	ymax+=(ymax-ymin)*.25;
	ymin-=(ymax-ymin)*.25;

	offset->SetYBounds(ymin,ymax,NULL,true);
	width ->SetYBounds(ymin,ymax,NULL,true);

	//finalize bounds of angle
	if (amax==amin) amin=amax-1;
	double aamax=amax;
	amax+=(amax-amin)*.25;
	amin-=(amax-amin)*.25;
	angle->SetYBounds(amin,amax,NULL,false);

	//Now we have the bounds of the curve info, now need to push the actual points
	if (!hasangle) angle->SetFlat(aamax);

	if (pathweights.n==0) {
		if (hasangle) angle->AddPoint(0,0);
		width ->AddPoint(0,defaultwidth);
		offset->AddPoint(0,0);

	} else {
		for (int c=0; c<pathweights.n; c++) {
			if (hasangle) angle->AddPoint(pathweights.e[c]->t,pathweights.e[c]->angle);

			offset->AddPoint(pathweights.e[c]->t,pathweights.e[c]->offset);
			width ->AddPoint(pathweights.e[c]->t,pathweights.e[c]->width); 
		}
	}

//	if (!isclosed && zero_caps) {
//		//add zero width to start and end for this cap style
//		width->AddPoint(0,0);
//		width->AddPoint(n,0);
//	}
}

/*! Always true if absoluteangle is true.
 * If not absoluteangle, then true if any weight node angle is nonzero.
 */
bool LineProfile::Angled()
{
	if (pathweights.n==0) return false;
	//if (absoluteangle==true) return true;
	for (int c=0; c<pathweights.n; c++) {
		if (pathweights.e[c]->angle!=0) return true;
	}
	return false;
}

/*! true if any weight node has nonzero offset.
 */
bool LineProfile::HasOffset()
{
	for (int c=0; c<pathweights.n; c++) {
		if (pathweights.e[c]->offset!=0) return true;
	}
	return false;
}

/*! True if there are no weight nodes, or the width is the same in all nodes.
 */
bool LineProfile::ConstantWidth()
{
	if (pathweights.n==0) return true;

	for (int c=1; c<pathweights.n; c++) {
		if (pathweights.e[c]->width!=pathweights.e[0]->width) return false;
	}
	return true;
}

/*! Add a weight node at nt with the given offset, width and angle.
 */
void LineProfile::AddNode(double nt,double no,double nw,double nangle)
{
	PathWeightNode *w=new PathWeightNode(nt,no,nw,PathWeightNode::Default);

	//insert sorted... *** this doesn't have to be a necessity!! could do some interesting path on path stuff
	int c2;
	for (c2=0; c2<pathweights.n; c2++) {
		if (w->t>pathweights.e[c2]->t) continue;
		if (w->t<pathweights.e[c2]->t) break;

		// else (w->t == pathweights.e[c2])
		//overwrite!
		pathweights.e[c2]->t=nt;
		pathweights.e[c2]->offset=no;
		pathweights.e[c2]->width=nw;
		pathweights.e[c2]->angle=nangle;
		delete w;
		w=pathweights.e[c2];
		c2=-1;
		break;
	}

	if (c2>=0) pathweights.push(w,1,c2);

	needtorecache=1;
}

void LineProfile::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent); 
}

Laxkit::Attribute *LineProfile::dump_out_atts(Laxkit::Attribute *att,int what,Laxkit::DumpContext *context)
{
	if (!att) att=new Attribute();

	if (what==-1) {

		//att->push("resource_id",      "#unique number for this object, usually readonly, created internally to simplify shared resources upon read in");
		att->push("max_height","1", "In coordinates spanning mint to maxt along path, max_height messures maximum width of the nodes");
		att->push("default_width","1", "In same coordinates as max_height");
		att->push("mint",nullptr,"Hint about the natural span of the weight points");
		att->push("maxt",nullptr,"Hint about the natural span of the weight points");
		att->push("wrap","true", "Whether the profile is designed for closed paths or not");
		att->push("start_type", "normal", "normal or random");
		att->push("start_rand_width","0","When random, the spread around start to randomize");
		att->push("start","0", "Default start position to apply the profile");
		att->push("end_type", "normal", "normal or random");
		att->push("end_rand_width", "0", "When random, the spread around end to randomize");
		att->push("end","1", "Default end position to apply the profile");

		att->push("weight", "0 0 .1 0", "One or more of these: (t) (offset) (width) (angle)");
		return att;
	}

	att->push("max_height",max_height);
	att->push("default_width",defaultwidth);
	att->push("mint",mint);
	att->push("maxt",maxt);
	att->push("wrap",wrap ? "yes" : "no");
	att->push("start_type", start_type==0 ? "normal" : "random");
	att->push("start_rand_width",start_rand_width);
	att->push("start",start);
	att->push("end_type", end_type==0 ? "normal" : "random");
	att->push("end_rand_width",end_rand_width);
	att->push("end",end);

	char buffer[100];
    for (int c=0; c<pathweights.n; c++) {
        sprintf(buffer,"weight %.10g %.10g %.10g %.10g\n",
                pathweights.e[c]->t,pathweights.e[c]->offset,pathweights.e[c]->width,pathweights.e[c]->angle);
		att->push("weight",buffer);
    }


	return att;
}

void LineProfile::dump_in_atts(Laxkit::Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;

    char *name,*value;
    int c;

    for (c=0; c<att->attributes.n; c++) {
        name= att->attributes.e[c]->name;
        value=att->attributes.e[c]->value;

        if (!strcmp(name,"max_height")) {
			DoubleAttribute(value, &max_height, NULL);

        } else if (!strcmp(name,"default_width")) {
			DoubleAttribute(value, &defaultwidth, NULL);

		} else if (!strcmp(name,"mint")) {
			DoubleAttribute(value, &mint, NULL);

		} else if (!strcmp(name,"maxt")) {
			DoubleAttribute(value, &maxt, NULL);

		} else if (!strcmp(name,"wrap")) {
			wrap=BooleanAttribute(value);

		} else if (!strcmp(name,"start_type")) {
			if (value && !strcasecmp(value,"normal")) start_type=0;
			else start_type=1;

		} else if (!strcmp(name,"start_rand_width")) {
			DoubleAttribute(value, &start_rand_width, NULL);

		} else if (!strcmp(name,"start")) {
			DoubleAttribute(value, &start, NULL);

		} else if (!strcmp(name,"end_type")) {
			if (value && !strcasecmp(value,"normal")) end_type=0;
			else end_type=1;

		} else if (!strcmp(name,"end_rand_width")) {
			DoubleAttribute(value, &end_rand_width, NULL);

		} else if (!strcmp(name,"end")) {
			DoubleAttribute(value, &end, NULL);

		} else if (!strcmp(name,"weight")) {
			double d[4];
            int c2=DoubleListAttribute(value,d,4,NULL);
            if (c2==3) { d[3]=0; c2=4; } //add angle if not there
            if (c2!=4) continue;

            AddNode(d[0],d[1],d[2],d[3]);
		}
	}
}





} // namespace LaxInterfaces



