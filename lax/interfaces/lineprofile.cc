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
//    Copyright (C) 2015 by Tom Lechner
//


#include <lax/interfaces/lineprofile.h>
#include <lax/attributes.h>
#include <lax/language.h>

#include <lax/lists.cc>


using namespace LaxFiles;
using namespace Laxkit;


namespace LaxInterfaces {

//--------------------------- LineProfile -----------------------------
/*! \class LineProfile
 *
 * Stores the shape of a line, abstracted from the actual windings of the path.
 * This shape is defined by any number of weight nodes, which define the
 * offset, width, and angle at that point.
 */


LineProfile::LineProfile()
{
	mint=0;
	maxt=1;
	max_height=0;
	defaultwidth=1;
	wrap=0;

	preview=NULL;
	needtorecache=1;

	start=0;
	start_type=0;
	start_rand_width=0;

	end=1;
	end_type=0;
	end_rand_width=0;

	width=offset=angle=NULL;
}

LineProfile::~LineProfile()
{
	if (preview) preview->dec_count();
	if (width)   width  ->dec_count();
	if (offset)  offset ->dec_count();
	if (angle)   angle  ->dec_count();
}

LaxImage *LineProfile::CreatePreview(int pwidth,int pheight)
{
	// *** should preview angle too

	if (preview) {
		if (preview->w()!=pwidth || preview->h()!=pheight) {
			preview->dec_count();
			preview=NULL;
		}
	}
	if (!preview) {
		preview=create_new_image(pwidth,pheight);
	}

	unsigned char *data=preview->getImageBuffer();
	memset(data, 255, pwidth*pheight*4); //all white

	double w,off,angle;
	for (int x=0; x<pwidth; x++) {
		GetWeight((double)x/pwidth, &w,&off,&angle);
		w*=pheight/2;
		off=(off+1)*pheight;
		for (int y=off-w/2; y<off+w/2; y++) {
			data[y*pwidth*4 + x]=0;
		}
	}

	preview->doneWithBuffer(data);

	return preview;
}

/*! Returns this->preview, or generate a new default one and return that.
 */
LaxImage *LineProfile::Preview()
{
	if (preview) return preview;
	CreatePreview(150,20);
	return preview;
}

/*! t must range from 0 to 1.
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

void LineProfile::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att,what,context);
	att.dump_out(f,indent); 
}

LaxFiles::Attribute *LineProfile::dump_out_atts(LaxFiles::Attribute *att,int what,LaxFiles::DumpContext *context)
{
	if (!att) att=new Attribute();

	if (what==-1) {

		//att->push("resource_id",      "#unique number for this object, usually readonly, created internally to simplify shared resources upon read in");
		att->push("max_height","1 #In coordinates spanning mint to maxt along path, max_height messures maximum width of the nodes");
		att->push("default_width","1 #In same coordinates as max_height");
		att->push("mint","#Hint about the natural span of the weight points");
		att->push("maxt","#Hint about the natural span of the weight points");
		att->push("wrap","true #Whether the profile is designed for closed paths or not");
		att->push("start_type", "normal #normal or random");
		att->push("start_rand_width","0 #When random, the spread around start to randomize");
		att->push("start","Default start position to apply the profile");
		att->push("end_type", "normal #normal or random");
		att->push("end_rand_width", "0 #When random, the spread around end to randomize");
		att->push("end","Default end position to apply the profile");

		att->push("weight", "(t) (offset) (width) (angle) #One or more of these");
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

void LineProfile::dump_in_atts(LaxFiles::Attribute *att,int flag,LaxFiles::DumpContext *context)
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



//--------------------------- standard LineProfiles -----------------------------


/*! Return NULL terminated list of new instances for built in LineProfile objects.
 */
LineProfile **MakeStandardLineProfiles()
{
	Laxkit::PtrStack<LineProfile> profiles;

	LineProfile *profile;
	
	 //flat
	profile=new LineProfile;
	profile->AddNode(0,0,1,0);
	profile->Id(_("Flat"));
	profiles.push(profile,1);

	//rising
	profile=new LineProfile;
	profile->AddNode(0,0,0,0);
	profile->AddNode(1,0,1,0);
	profile->Id(_("Rising"));
	profiles.push(profile,1);

	//falling
	profile=new LineProfile;
	profile->AddNode(0,0,1,0);
	profile->AddNode(1,0,0,0);
	profile->Id(_("Falling"));
	profiles.push(profile,1);

	//eye
	profile=new LineProfile;
	profile->AddNode( 0,0,0,0);
	profile->AddNode(.5,0,1,0);
	profile->AddNode( 1,0,0,0);
	profile->Id(_("Eye"));
	profiles.push(profile,1);

	//eye slit, kind of sine wave with bump in the middle


	profiles.push(NULL);
	LineProfile **pf=profiles.extractArrays(NULL,NULL);
	return pf;
}



} // namespace LaxInterfaces



